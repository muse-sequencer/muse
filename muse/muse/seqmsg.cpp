//=============================================================================
//  MusE
//  Linux Music Editor
//  $Id:$
//
//  Copyright (C) 2002-2006 by Werner Schweer and others
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License version 2.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//=============================================================================

#include "song.h"
#include "instruments/minstrument.h"
#include "muse.h"
#include "mixer/mixer.h"
#include "al/tempo.h"
#include "al/sig.h"
#include "audio.h"
#include "driver/audiodev.h"
#include "driver/mididev.h"
#include "audio.h"
#include "arranger/arranger.h"
#include "plugin.h"
#include "midi.h"
#include "midictrl.h"
#include "midiplugin.h"
#include "part.h"
#include "midioutport.h"
#include "midiinport.h"

//---------------------------------------------------------
//   sendMsg
//---------------------------------------------------------

void Audio::sendMsg(AudioMsg* m)
      {
      if (audioState == AUDIO_RUNNING) {
            msg = m;
            char c;
            int rv = read(fromThreadFdr, &c, 1);
            if (rv != 1)
                  perror("Audio: read pipe failed");
            }
      else {
            // if audio is not running (during initialization)
            // process commands immediatly
            msg = m;
            processMsg();
            }
      }

//---------------------------------------------------------
//   sendMessage
//    send request from gui to sequencer
//    wait until request is processed
//---------------------------------------------------------

bool Audio::sendMessage(AudioMsg* m, bool doUndo)
      {
      if (doUndo)
            song->startUndo();
      sendMsg(m);
      if (doUndo)
            song->endUndo(0);       // song->endMsgCmd();
      return false;
      }

//---------------------------------------------------------
//   msgRoute
//---------------------------------------------------------

extern bool mops;

void Audio::msgRoute(bool add, Route r)
      {
      if (add)
            msgAddRoute(r);
      else
            msgRemoveRoute(r);
      song->update(SC_ROUTE);
      }

//---------------------------------------------------------
//   msgRemoveRoute
//---------------------------------------------------------

void Audio::msgRemoveRoute(Route r)
      {
      if (r.src.type == RouteNode::AUDIOPORT) {
            AudioInput* ai = (AudioInput*)(r.dst.track);
            audioDriver->disconnect(r.src.port, ai->jackPort(r.dst.channel));
            }
      else if (r.src.type == RouteNode::JACKMIDIPORT) {
            audioDriver->disconnect(r.src.port, ((MidiInPort*)r.dst.track)->jackPort());
            }
      else if (r.src.type == RouteNode::MIDIPORT) {
            midiDriver->disconnect(r.src.port, ((MidiInPort*)r.dst.track)->alsaPort());
            }
      else if (r.dst.type == RouteNode::AUDIOPORT) {
            AudioOutput* ai = (AudioOutput*)(r.src.track);
            audioDriver->disconnect(ai->jackPort(r.src.channel), r.dst.port);
            }
      else if (r.dst.type == RouteNode::MIDIPORT) {
            midiDriver->disconnect(((MidiOutPort*)r.src.track)->alsaPort(), r.dst.port);
            }
      else if (r.dst.type == RouteNode::JACKMIDIPORT) {
            audioDriver->disconnect(((MidiOutPort*)r.src.track)->jackPort(), r.dst.port);
            }
      msgRemoveRoute1(r);
      }

//---------------------------------------------------------
//   msgRemoveRoute1
//---------------------------------------------------------

void Audio::msgRemoveRoute1(Route r)
      {
      AudioMsg msg;
      msg.id     = AUDIO_ROUTEREMOVE;
      msg.route = r;
      sendMsg(&msg);
      }

//---------------------------------------------------------
//   msgAddRoute
//---------------------------------------------------------

void Audio::msgAddRoute(Route r)
      {
      msgAddRoute1(r);
      if (r.src.type == RouteNode::AUDIOPORT) {
            AudioInput* ai = (AudioInput*)r.dst.track;
            audioDriver->connect(r.src.port, ai->jackPort(r.dst.channel));
            }
      else if (r.src.type == RouteNode::JACKMIDIPORT) {
            audioDriver->connect(r.src.port, ((MidiInPort*)r.dst.track)->jackPort());
            }
      else if (r.src.type == RouteNode::MIDIPORT) {
            midiDriver->connect(r.src.port, ((MidiInPort*)r.dst.track)->alsaPort());
            }
      else if (r.dst.type == RouteNode::AUDIOPORT) {
            AudioOutput* ao = (AudioOutput*)r.src.track;
printf("msgAddRoute to AUDIPORT %p\n", ao);
            audioDriver->connect(ao->jackPort(r.src.channel), r.dst.port);
            }
      else if (r.dst.type == RouteNode::MIDIPORT) {
            midiDriver->connect(((MidiOutPort*)r.src.track)->alsaPort(), r.dst.port);
            }
      else if (r.dst.type == RouteNode::JACKMIDIPORT) {
            audioDriver->connect(((MidiOutPort*)r.src.track)->jackPort(), r.dst.port);
            }
      }

//---------------------------------------------------------
//   msgAddRoute1
//---------------------------------------------------------

void Audio::msgAddRoute1(Route r)
      {
      AudioMsg msg;
      msg.id = AUDIO_ROUTEADD;
      msg.route = r;
      sendMsg(&msg);
      }

//---------------------------------------------------------
//   msgAddMidiPlugin
//---------------------------------------------------------

void Audio::msgAddMidiPlugin(MidiTrackBase* track, int idx, MidiPluginI* plugin)
      {
      AudioMsg msg;
      msg.id      = AUDIO_ADDMIDIPLUGIN;
      msg.track   = track;
      msg.ival    = idx;
      msg.mplugin = plugin;
      MidiPluginI* oldPlugin = plugin ? 0 : track->plugin(idx);
      sendMsg(&msg);
      if (oldPlugin)
            delete oldPlugin;
      }

//---------------------------------------------------------
//   msgAddPlugin
//---------------------------------------------------------

void Audio::msgAddPlugin(AudioTrack* track, int idx, PluginI* plugin,
   bool prefader)
      {
      AudioMsg msg;
      msg.id     = AUDIO_ADDPLUGIN;
      msg.track  = track;
      msg.ival   = idx;
      msg.iival  = prefader;
      msg.plugin = plugin;
      sendMsg(&msg);
      }

//---------------------------------------------------------
//   msgSetChannels
//---------------------------------------------------------

void Audio::msgSetChannels(AudioTrack* node, int n)
      {
      if (n == node->channels())
            return;
      QString name = node->name();
      int mc       = std::max(n, node->channels());

      if (!name.isEmpty()) {
            if (node->type() == Track::AUDIO_INPUT) {
                  AudioInput* ai = (AudioInput*)node;
                  for (int i = 0; i < mc; ++i) {
                        if (i < n && ai->jackPort(i).isZero()) {
                              char buffer[128];
                              snprintf(buffer, 128, "%s-%d", name.toLatin1().data(), i);
                              ai->setJackPort(audioDriver->registerInPort(QString(buffer), false), i);
                              }
                        else if ((i >= n) && ai->jackPort(i).isZero()) {
                              RouteList* ir = node->inRoutes();
                              for (iRoute ii = ir->begin(); ii != ir->end(); ++ii) {
                                    Route r = *ii;
                                    if ((r.src.type == RouteNode::AUDIOPORT) && (r.src.channel == i)) {
                                          msgRemoveRoute(r);
                                          break;
                                          }
                                    }
                              audioDriver->unregisterPort(ai->jackPort(i));
                              ai->setJackPort(Port(), i);
                              }
                        }
                  }
            else if (node->type() == Track::AUDIO_OUTPUT) {
                  AudioOutput* ao = (AudioOutput*)node;
                  for (int i = 0; i < mc; ++i) {
                        Port port = ao->jackPort(i);
                        if (i < n && port.isZero()) {
                              char buffer[128];
                              snprintf(buffer, 128, "%s-%d", name.toLatin1().data(), i);
                              ao->setJackPort(audioDriver->registerOutPort(QString(buffer), false), i);
                              }
                        else if (i >= n && !port.isZero()) {
                              RouteList* ir = node->outRoutes();
                              for (iRoute ii = ir->begin(); ii != ir->end(); ++ii) {
                                    Route r = *ii;
                                    if ((r.src.type == RouteNode::AUDIOPORT) && (r.src.channel == i)) {
                                          msgRemoveRoute(r);
                                          break;
                                          }
                                    }
                              audioDriver->unregisterPort(ao->jackPort(i));
                              ao->setJackPort(Port(), i);
                              }
                        }
                  }
            }
      AudioMsg msg;
      msg.id    = AUDIO_SET_CHANNELS;
      msg.track = node;
      msg.ival  = n;
      sendMsg(&msg);
      }

//---------------------------------------------------------
//   msgSetSegSize
//---------------------------------------------------------

void Audio::msgSetSegSize(int bs, int sr)
      {
      AudioMsg msg;
      msg.id = AUDIO_SET_SEG_SIZE;
      msg.ival = bs;
      msg.iival = sr;
      sendMsg(&msg);
      }

//---------------------------------------------------------
//   msgSeek
//---------------------------------------------------------

void Audio::msgSeek(const Pos& pos)
      {
      audioDriver->seekTransport(pos.frame());
      }

//---------------------------------------------------------
//   msgUndo
//---------------------------------------------------------

void Audio::msgUndo()
      {
      AudioMsg msg;
      msg.id = SEQM_UNDO;
      sendMsg(&msg);
      }

//---------------------------------------------------------
//   msgRedo
//---------------------------------------------------------

void Audio::msgRedo()
      {
      AudioMsg msg;
      msg.id = SEQM_REDO;
      sendMsg(&msg);
      }

//---------------------------------------------------------
//   msgPlay
//---------------------------------------------------------

void Audio::msgPlay(bool val)
      {
      if (val) {
            if (audioDriver)
                audioDriver->startTransport();
            }
      else
            if (audioDriver)
                audioDriver->stopTransport();
      }

//---------------------------------------------------------
//   msgRemoveTrack
//---------------------------------------------------------

void Audio::msgRemoveTrack(Track* track)
      {
      AudioMsg msg;
      msg.id = SEQM_REMOVE_TRACK;
      msg.track = track;
      sendMessage(&msg, true);
      }

//---------------------------------------------------------
//   msgRemoveTracks
//    remove all selected tracks
//---------------------------------------------------------

void Audio::msgRemoveTracks()
      {
      TrackList tl;
	TrackList* tl2 = song->tracks();
	for (iTrack t = tl2->begin(); t != tl2->end(); ++t) {
		if ((*t)->selected())
                  tl.push_back(*t);
            }
      for (iTrack t = tl.begin(); t != tl.end(); ++t) {
            Track* track = *t;
      	int idx = song->tracks()->indexOf(track);
      	song->undoOp(UndoOp::DeleteTrack, idx, track);
      	song->removeTrack1(track);
      	msgRemoveTrack(track);
      	song->removeTrack3(track);
            }
      }

//---------------------------------------------------------
//   msgMoveTrack
//---------------------------------------------------------

void Audio::msgMoveTrack(Track* src, Track* dst)
      {
      AudioMsg msg;
      msg.id = SEQM_MOVE_TRACK;
      msg.p1 = src;
      msg.p2 = dst;
      sendMessage(&msg, false);
      }

//---------------------------------------------------------
//   msgAddEvent
//---------------------------------------------------------

void Audio::msgAddEvent(const Event& event, Part* part, bool doUndoFlag)
      {
      AudioMsg msg;
      msg.id = SEQM_ADD_EVENT;
      msg.ev1 = event;
      msg.p2 = part;
      sendMessage(&msg, doUndoFlag);
      }

//---------------------------------------------------------
//   msgAddEvents
//---------------------------------------------------------

void Audio::msgAddEvents(QList<Event>* el, Part* part)
      {
      AudioMsg msg;
      msg.id = SEQM_ADD_EVENTS;
      msg.el = el;
      msg.p2 = part;
      sendMessage(&msg, false);
      }

//---------------------------------------------------------
//   msgDeleteEvent
//---------------------------------------------------------

void Audio::msgDeleteEvent(const Event& event, Part* part, bool doUndoFlag)
      {
      AudioMsg msg;
      msg.id = SEQM_REMOVE_EVENT;
      msg.ev1 = event;
      msg.p2 = part;
      sendMessage(&msg, doUndoFlag);
      }

//---------------------------------------------------------
//   msgChangeEvent
//---------------------------------------------------------

void Audio::msgChangeEvent(const Event& oe, const Event& ne, Part* part, bool doUndoFlag)
      {
      AudioMsg msg;
      msg.id = SEQM_CHANGE_EVENT;
      msg.ev1 = oe;
      msg.ev2 = ne;
      msg.p3 = part;
      sendMessage(&msg, doUndoFlag);
      }

//---------------------------------------------------------
//   msgAddTempo
//---------------------------------------------------------

void Audio::msgAddTempo(int tick, int tempo, bool doUndoFlag)
      {
      AudioMsg msg;
      msg.id = SEQM_ADD_TEMPO;
      msg.a = tick;
      msg.b = tempo;
      sendMessage(&msg, doUndoFlag);
      }

//---------------------------------------------------------
//   msgSetTempo
//---------------------------------------------------------

void Audio::msgSetTempo(int tick, int tempo, bool doUndoFlag)
      {
      AudioMsg msg;
      msg.id = SEQM_SET_TEMPO;
      msg.a = tick;
      msg.b = tempo;
      sendMessage(&msg, doUndoFlag);
      }

//---------------------------------------------------------
//   msgSetGlobalTempo
//---------------------------------------------------------

void Audio::msgSetGlobalTempo(int val)
      {
      AudioMsg msg;
      msg.id = SEQM_SET_GLOBAL_TEMPO;
      msg.a = val;
      sendMessage(&msg, false);
      }

//---------------------------------------------------------
//   msgDeleteTempo
//---------------------------------------------------------

void Audio::msgDeleteTempo(int tick, int tempo, bool doUndoFlag)
      {
      AudioMsg msg;
      msg.id = SEQM_REMOVE_TEMPO;
      msg.a = tick;
      msg.b = tempo;
      sendMessage(&msg, doUndoFlag);
      }

//---------------------------------------------------------
//   msgAddSig
//---------------------------------------------------------

void Audio::msgAddSig(int tick, const AL::TimeSignature& sig, bool doUndoFlag)
      {
      AudioMsg msg;
      msg.id = SEQM_ADD_SIG;
      msg.a = tick;
      msg.b = sig.z;
      msg.c = sig.n;
      sendMessage(&msg, doUndoFlag);
      }

//---------------------------------------------------------
//   msgRemoveSig
//! sends remove tempo signature message
//---------------------------------------------------------

void Audio::msgRemoveSig(int tick, int z, int n, bool doUndoFlag)
      {
      AudioMsg msg;
      msg.id = SEQM_REMOVE_SIG;
      msg.a = tick;
      msg.b = z;
      msg.c = n;
      sendMessage(&msg, doUndoFlag);
      }

//---------------------------------------------------------
//   msgResetMidiDevices
//---------------------------------------------------------

void Audio::msgResetMidiDevices()
      {
      AudioMsg msg;
      msg.id = SEQM_RESET_DEVICES;
      sendMessage(&msg, false);
      }

//---------------------------------------------------------
//   msgInitMidiDevices
//---------------------------------------------------------

void Audio::msgInitMidiDevices()
      {
      AudioMsg msg;
      msg.id = SEQM_INIT_DEVICES;
      sendMessage(&msg, false);
      }

//---------------------------------------------------------
//   panic
//---------------------------------------------------------

void Audio::msgPanic()
      {
      MidiEvent ev1(0, 0, ME_CONTROLLER, CTRL_ALL_SOUNDS_OFF, 0);
      MidiEvent ev2(0, 0, ME_CONTROLLER, CTRL_RESET_ALL_CTRL, 0);

      MidiTrackList* cl = song->midis();
      for (iMidiTrack i = cl->begin(); i != cl->end(); ++i) {
            (*i)->playMidiEvent(&ev1);
            (*i)->playMidiEvent(&ev2);
            }
      }

//---------------------------------------------------------
//   localOff
//---------------------------------------------------------

void Audio::msgLocalOff()
      {
      MidiEvent ev1(0, 0, ME_CONTROLLER, CTRL_LOCAL_OFF, 0);

      MidiTrackList* cl = song->midis();
      for (iMidiTrack i = cl->begin(); i != cl->end(); ++i)
            (*i)->playMidiEvent(&ev1);
      }

//---------------------------------------------------------
//   msgBounce
//    start bounce operation
//---------------------------------------------------------

void Audio::msgBounce()
      {
      _bounce = true;
      audioDriver->seekTransport(song->lPos().frame());
      }

//---------------------------------------------------------
//   msgIdle
//---------------------------------------------------------

void Audio::msgIdle(bool on)
      {
      AudioMsg msg;
      msg.id = SEQM_IDLE;
      msg.a  = on;
      sendMessage(&msg, false);
      }

//---------------------------------------------------------
//   msgAddController
//    add controller value
//---------------------------------------------------------

void Audio::msgAddController(Track* track, int id, unsigned time, CVal val)
      {
      AudioMsg msg;
      msg.id    = SEQM_ADD_CTRL;
      msg.track = track;
      msg.a     = id;
      msg.time  = time;
      msg.cval1  = val;
      song->startUndo();
      song->undoOp(UndoOp::AddCtrl, track, id, time, val, CVal());
      sendMessage(&msg, false);
      song->endUndo(0);
      }

//---------------------------------------------------------
//   msgRemoveController
//---------------------------------------------------------

void Audio::msgRemoveController(Track* track, int id, unsigned time)
      {
      AudioMsg msg;
      msg.id    = SEQM_REMOVE_CTRL;
      msg.track = track;
      msg.a     = id;
      msg.time  = time;
      song->startUndo();
      CVal a, b;
      song->undoOp(UndoOp::RemoveCtrl, track, id, time, a, b);
      sendMessage(&msg, false);
      song->endUndo(0);
      }

//---------------------------------------------------------
//   msgSetRtc
//---------------------------------------------------------

void Audio::msgSetRtc()
      {
      AudioMsg msg;
      msg.id = MS_SET_RTC;
      sendMsg(&msg);
      }

