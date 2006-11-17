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
#include "audio.h"
#include "midiplugin.h"
#include "driver/audiodev.h"
#include "muse.h"

//---------------------------------------------------------
//   deselectTracks
//---------------------------------------------------------

void Song::deselectTracks()
      {
      for (iTrack t = _tracks.begin(); t != _tracks.end(); ++t)
            (*t)->setSelected(false);
      }

//---------------------------------------------------------
//   selectTrack
//---------------------------------------------------------

void Song::selectTrack(Track* track)
      {
      bool changed = false;
      for (iTrack t = _tracks.begin(); t != _tracks.end(); ++t) {
            bool select = *t == track;
            if ((*t)->selected() != select) {
                  (*t)->setSelected(select);
                  changed = true;
                  }
            }
      if (changed) {
            updateSelectedTrack();
            trackSelectionChanged(_selectedTrack);
            }
      }

//---------------------------------------------------------
//   updateSelectedTrack
//    set _selectedTrack to first selected track
//---------------------------------------------------------

void Song::updateSelectedTrack()
      {
      _selectedTrack = 0;
      for (iTrack t = _tracks.begin(); t != _tracks.end(); ++t) {
            bool select = (*t)->selected();
            if (select) {
                  _selectedTrack = *t;
                  break;
                  }
            }
      }

//---------------------------------------------------------
//    addTrack
//    called from GUI context
//---------------------------------------------------------

Track* Song::addTrack(QAction* action)
      {
      int t = action->data().toInt();

      deselectTracks();
      Track* track = 0;

      Track::TrackType type = (Track::TrackType) t;
      if (t >= 5000) {
            int idx = t - 5000;
            type = Track::MIDI_SYNTI;
            int k = 0;
            iMidiPlugin i;
            for (i = midiPlugins.begin(); i != midiPlugins.end(); ++i) {
                  if ((*i)->type() != MEMPI_GENERATOR)
                        continue;
                  if (k == idx)
                        break;
                  ++k;
                  }
            if (i == midiPlugins.end()) {
                  fprintf(stderr, "Song::addTrack: midi synti not found\n");
                  return 0;
                  }
            MidiPlugin* s = *i;
            MidiSynti* si = new MidiSynti();
            QString sName(s->name());
            for (k = s->instances(); k < 1000; ++k) {
                  QString instanceName = (k == 0) ? 
                     sName : instanceName.arg(sName).arg(k);

                  MidiSyntiList* sl = midiSyntis();
                  iMidiSynti sii;
                  for (sii = sl->begin(); sii != sl->end(); ++sii) {
                        if ((*sii)->name() == instanceName)
                              break;
                        }
                  if (sii == sl->end()) {
                        si->setName(instanceName);
                        break;
                        }
                  }
            if (si->initInstance(s)) {
                  delete si;
                  return 0;
                  }
            track = si;
            }
      else if (t >= 1000) {
            type = Track::AUDIO_SOFTSYNTH;
            QString sclass = synthis[t-1000]->name();

            Synth* s = findSynth(sclass);
            if (s == 0) {
                  fprintf(stderr, "synthi class <%s> not found\n", sclass.toLatin1().data());
                  return 0;
                  }

            SynthI* si = new SynthI();
            int i;
            for (i = s->instances(); i < 1000; ++i) {
                  QString instanceName = (i == 0) ? 
                     s->name() : QString("%1-%2").arg(s->name()).arg(i);

                  SynthIList* sl = syntis();
                  iSynthI sii;
                  for (sii = sl->begin(); sii != sl->end(); ++sii) {
                        if ((*sii)->name() == instanceName)
                              break;
                        }
                  if (sii == sl->end()) {
                        si->setName(instanceName);
                        break;
                        }
                  }
            if (si->initInstance(s)) {
                  delete si;
                  return 0;
                  }
            track = si;
            }
      else {
            switch (type) {
                  case Track::MIDI:
                        track = new MidiTrack();
                        break;
                  case Track::MIDI_OUT:
                        track = new MidiOutPort();
                        break;
                  case Track::MIDI_IN:
                        track = new MidiInPort();
                        break;
                  case Track::WAVE:
                        track = new WaveTrack();
                        break;
                  case Track::AUDIO_OUTPUT:
                        track = new AudioOutput();
                        break;
                  case Track::AUDIO_GROUP:
                        track = new AudioGroup();
                        break;
                  case Track::AUDIO_INPUT:
                        track = new AudioInput();
                        break;
                  case Track::AUDIO_SOFTSYNTH:
                  case Track::TRACK_TYPES:
                  default:
                        printf("Song::addTrack() illegal type %d\n", type);
                        abort();
                  }
            if (track == 0)
                  return 0;
            }
      track->setDefaultName();
      insertTrack(track, -1);
      return track;
      }

//---------------------------------------------------------
//   insertTrack
//---------------------------------------------------------

void Song::insertTrack(Track* track, int idx)
      {
      //
      //  add default routes
      //
      OutputList* ol = outputs();
      AudioOutput* ao = 0;
      if (!ol->empty())
            ao = ol->front();
      MidiOutPortList* mol = midiOutPorts();
      MidiOutPort* mo = 0;
      if (!mol->empty())
            mo = mol->front();

      switch (track->type()) {
            case Track::TRACK_TYPES:
            case Track::MIDI_OUT:
            case Track::MIDI_IN:
            case Track::MIDI_SYNTI:
                  break;
            case Track::MIDI:
                  //
                  // connect to all midi inputs, if there is not already
                  // a route
                  //
                  if (!track->noInRoute()) {
                        MidiInPortList* mi = midiInPorts();
                        for (iMidiInPort i = mi->begin(); i != mi->end(); ++i) {
                              for (int ch = 0; ch < MIDI_CHANNELS; ++ch) {
                                    RouteNode src(*i, ch, RouteNode::TRACK);
                                    RouteNode dst(track, -1, RouteNode::TRACK);
                                    Route r = Route(src, dst);
                                    track->inRoutes()->push_back(r);
                                    }
                              }
                        }
                  break;
            case Track::AUDIO_SOFTSYNTH:
            case Track::WAVE:
            case Track::AUDIO_GROUP:
                  if (ao)
                        track->outRoutes()->push_back(Route(RouteNode(track), RouteNode(ao)));
                  break;

            case Track::AUDIO_INPUT:
                  {
                  // connect first input channel to first available jack output
                  // etc.
                  QList<PortName> op = audioDriver->outputPorts(false);
                  QList<PortName>::iterator is = op.begin();
                  for (int ch = 0; ch < track->channels(); ++ch) {
                        if (is != op.end()) {
                              RouteNode src(is->port, -1, RouteNode::AUDIOPORT);
                              RouteNode dst(track, ch, RouteNode::TRACK);
                              Route r = Route(src, dst);
                              track->inRoutes()->push_back(r);
                              ++is;
                              }
                        }
//                  if (ao)
//                        track->outRoutes()->push_back(Route(ao));
                  }
                  break;
            case Track::AUDIO_OUTPUT:
                  {
                  QList<PortName> op = audioDriver->inputPorts(false);
                  QList<PortName>::iterator is = op.begin();
                  for (int ch = 0; ch < track->channels(); ++ch) {
                        if (is != op.end()) {
                              RouteNode src(track, ch, RouteNode::TRACK);
                              RouteNode dst(is->port, -1, RouteNode::AUDIOPORT);
                              Route r = Route(src, dst);
                              track->outRoutes()->push_back(r);
                              ++is;
                              }
                        }
                  }
                  break;
            }
      insertTrack1(track, idx);

      startUndo();
      undoOp(UndoOp::AddTrack, idx, track);
      AudioMsg msg;
      msg.id    = SEQM_ADD_TRACK;
      msg.track = track;
      msg.ival  = idx;
      audio->sendMsg(&msg);
      endUndo(SC_TRACK_INSERTED | SC_ROUTE);

      emit trackAdded(track, idx);
      selectTrack(track);
      }

//---------------------------------------------------------
//   insertTrack0
//    can only be called if sequencer is idle
//    (during song load)
//---------------------------------------------------------

void Song::insertTrack0(Track* track, int idx)
      {
      insertTrack1(track, idx);
      insertTrack2(track);
      }

//---------------------------------------------------------
//   insertTrack1
//    non realtime part of insertTrack
//---------------------------------------------------------

void Song::insertTrack1(Track* track, int idx)
      {
      if (idx == -1)
            idx = _tracks.size();
      _tracks.insert(idx, track);
      if (track->type() == Track::AUDIO_SOFTSYNTH) {
            SynthI* s = (SynthI*)track;
            Synth* sy = s->synth();
            if (!s->isActivated())
                  s->initInstance(sy);
            }
      if (audioState == AUDIO_RUNNING) {
            track->activate1();
            track->activate2();
            }
      }

//---------------------------------------------------------
//   insertTrack2
//    realtime part
//---------------------------------------------------------

void Song::insertTrack2(Track* track)
      {
      switch(track->type()) {
            case Track::MIDI_SYNTI:
                  _midiSyntis.push_back((MidiSynti*)track);
                  break;
            case Track::MIDI:
                  _midis.push_back((MidiTrack*)track);
                  break;
            case Track::MIDI_OUT:
                  _midiOutPorts.push_back((MidiOutPort*)track);
                  break;
            case Track::MIDI_IN:
                  _midiInPorts.push_back((MidiInPort*)track);
                  break;
            case Track::WAVE:
                  _waves.push_back((WaveTrack*)track);
                  break;
            case Track::AUDIO_OUTPUT:
                  _outputs.push_back((AudioOutput*)track);
                  break;
            case Track::AUDIO_GROUP:
                  _groups.push_back((AudioGroup*)track);
                  break;
            case Track::AUDIO_INPUT:
                  _inputs.push_back((AudioInput*)track);
                  break;
            case Track::AUDIO_SOFTSYNTH:
                  {
                  SynthI* s = (SynthI*)track;
                  midiInstruments.push_back(s->instrument());
                  _synthIs.push_back(s);
                  }
                  break;
            default:
                  fprintf(stderr, "insertTrack2: unknown track type %d\n", track->type());
                  // abort();
                  return;
            }

      //
      //  connect routes
      //
      if (track->type() == Track::AUDIO_OUTPUT || track->type() == Track::MIDI_OUT) {
            foreach(Route r, *(track->inRoutes())) {
                  if (r.src.type != RouteNode::AUXPLUGIN) {
                        r.src.track->outRoutes()->push_back(r);
                        }
                  }
            }
      else if (track->type() == Track::AUDIO_INPUT || track->type() == Track::MIDI_IN) {
            foreach(Route r, *(track->outRoutes())) {
                  if (r.dst.type != RouteNode::AUXPLUGIN) {
                        r.dst.track->inRoutes()->push_back(r);
                        }
                  }
            }
      else {
            foreach(Route r, *(track->inRoutes())) {
                  if (r.src.type != RouteNode::AUXPLUGIN) {
                        r.src.track->outRoutes()->push_back(r);
                        }
                  }
            foreach(Route r, *(track->outRoutes())) {
                  if (r.dst.type != RouteNode::AUXPLUGIN) {
                        r.dst.track->inRoutes()->push_back(r);
                        }
                  }
            }
      }

//---------------------------------------------------------
//   removeTrack
//    called from gui context
//---------------------------------------------------------

void Song::removeTrack(Track* track)
      {
      startUndo();
      int idx = _tracks.indexOf(track);
      undoOp(UndoOp::DeleteTrack, idx, track);
      removeTrack1(track);
      audio->msgRemoveTrack(track);
      removeTrack3(track);

      endUndo(SC_TRACK_REMOVED | SC_ROUTE);
      }

//---------------------------------------------------------
//   removeTrack1
//    non realtime part of removeTrack
//---------------------------------------------------------

void Song::removeTrack1(Track* track)
      {
      track->deactivate();
      _tracks.removeAt(_tracks.indexOf(track));
      }

//---------------------------------------------------------
//   removeTrack2
//    called from RT context
//---------------------------------------------------------

void Song::removeTrack2(Track* track)
      {
      switch (track->type()) {
            case Track::MIDI_SYNTI:
                  _midiSyntis.removeAt(_midiSyntis.indexOf((MidiSynti*)track));
                  break;
            case Track::MIDI:
                  _midis.removeAt(_midis.indexOf((MidiTrack*)track));
                  break;
            case Track::MIDI_OUT:
                  _midiOutPorts.removeAt(_midiOutPorts.indexOf((MidiOutPort*)track));
                  break;
            case Track::MIDI_IN:
                  _midiInPorts.removeAt(_midiInPorts.indexOf((MidiInPort*)track));
                  break;
            case Track::WAVE:
                  _waves.removeAt(_waves.indexOf((WaveTrack*)track));
                  break;
            case Track::AUDIO_OUTPUT:
                  _outputs.removeAt(_outputs.indexOf((AudioOutput*)track));
                  break;
            case Track::AUDIO_INPUT:
                  _inputs.removeAt(_inputs.indexOf((AudioInput*)track));
                  break;
            case Track::AUDIO_GROUP:
                  _groups.removeAt(_groups.indexOf((AudioGroup*)track));
                  break;
            case Track::AUDIO_SOFTSYNTH:
                  {
                  SynthI* s = (SynthI*) track;
                  s->deactivate2();
                  _synthIs.removeAt(_synthIs.indexOf(s));
                  }
                  break;
            case Track::TRACK_TYPES:
                  return;
            }
      //
      //  remove routes
      //
      foreach (const Route r, *(track->inRoutes())) {
            if (r.src.type != RouteNode::TRACK)
                  continue;
            int idx = r.src.track->outRoutes()->indexOf(r);
            if (idx != -1)
                  r.src.track->outRoutes()->removeAt(idx);
            else
                  printf("Song::removeTrack2(): input route not found\n");
            }
      foreach (const Route r, *(track->outRoutes())) {
            if (r.dst.type != RouteNode::TRACK)
                  continue;
            int idx = r.dst.track->inRoutes()->indexOf(r);
            if (idx != -1)
                  r.dst.track->inRoutes()->removeAt(idx);
            else {
                  printf("Song::removeTrack2(): output route not found\n");
                  }
            }
      }

//---------------------------------------------------------
//   removeTrack3
//    non realtime part of removeTrack
//---------------------------------------------------------

void Song::removeTrack3(Track* track)
      {
      if (track->type() == Track::AUDIO_SOFTSYNTH) {
            SynthI* s = (SynthI*) track;
            s->deactivate3();
            }
      emit trackRemoved(track);
      }

//---------------------------------------------------------
//   synthesizer
//---------------------------------------------------------

std::vector<QString>* Song::synthesizer() const
      {
      std::vector<QString>* l = new std::vector<QString>;

      for (std::vector<Synth*>::const_iterator i = synthis.begin();
         i != synthis.end(); ++i) {
            l->push_back((*i)->name());
            }
      return l;
      }

//---------------------------------------------------------
//   setRecordFlag
//---------------------------------------------------------

void Song::setRecordFlag(Track* track, bool val)
      {
      if (track->type() == Track::AUDIO_OUTPUT) {
            if (!val && track->recordFlag() == false) {
                  muse->bounceToFile();
                  }
            }
      track->setRecordFlag(val);
      }

//---------------------------------------------------------
//   setMute
//---------------------------------------------------------

void Song::setMute(Track* track, bool val)
      {
      track->setMute(val);
      emit muteChanged(track, track->mute());
      }

//---------------------------------------------------------
//   setMonitor
//---------------------------------------------------------

void Song::setMonitor(Track* track, bool val)
      {
      track->setMonitor(val);
//      emit monitorChanged(track, track->mute());
      }

//---------------------------------------------------------
//   setOff
//---------------------------------------------------------

void Song::setOff(Track* track, bool val)
      {
      track->setOff(val);
      emit offChanged(track, track->off());
      }

//---------------------------------------------------------
//   setAutoRead
//---------------------------------------------------------

void Song::setAutoRead(Track* track, bool val)
      {
      track->setAutoRead(val);
      emit autoReadChanged(track, track->autoRead());
      }

//---------------------------------------------------------
//   setAutoWrite
//---------------------------------------------------------

void Song::setAutoWrite(Track* track, bool val)
      {
      track->setAutoWrite(val);
      emit autoWriteChanged(track, track->autoRead());
      }

//---------------------------------------------------------
//   setSolo
//---------------------------------------------------------

void Song::setSolo(Track* track, bool val)
      {
      if (!track->setSolo(val))
            return;
      emit soloChanged(track, track->solo());
      soloFlag = false;
      for (iTrack i = _tracks.begin(); i != _tracks.end(); ++i) {
            if ((*i)->solo()) {
                  soloFlag = true;
                  break;
                  }
            }
      for (iTrack i = _tracks.begin(); i != _tracks.end(); ++i) {
            (*i)->updateMute();
            }
      }

//---------------------------------------------------------
//   moveTrack
//---------------------------------------------------------

void Song::moveTrack(Track* src, Track* dst)
      {
      iTrack si = qFind(_tracks.begin(), _tracks.end(), src);
      iTrack di = qFind(_tracks.begin(), _tracks.end(), dst);
      if (si == _tracks.end() || di == _tracks.end()) {
            printf("Song::moveTrack() track not found\n");
            return;
            }
      _tracks.erase(si);
      _tracks.insert(di, src);
      }

//---------------------------------------------------------
//   changeTrackName
//---------------------------------------------------------

void Song::changeTrackName(Track* t, const QString& s)
      {
      startUndo();
      undoOp(UndoOp::RenameTrack, t, t->name(), s);
      t->setName(s);
      endUndo(SC_TRACK_MODIFIED);
      }

//---------------------------------------------------------
//   trackExists
//---------------------------------------------------------

bool Song::trackExists(Track* t) const
      {
      return findTrack(t->name()) != 0;
      }

//---------------------------------------------------------
//   findTrack
//---------------------------------------------------------

Track* Song::findTrack(const QString& name) const
      {
      for (int i = 0; i < _tracks.size(); ++i) {
            if (_tracks[i]->name() == name)
                  return _tracks[i];
            }
      return 0;
      }

