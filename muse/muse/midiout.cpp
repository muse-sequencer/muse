//=============================================================================
//  MusE
//  Linux Music Editor
//  $Id:$
//
//  Copyright (C) 2006 by Werner Schweer and others
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

#include "midiout.h"
#include "midictrl.h"
#include "track.h"
#include "al/tempo.h"
#include "event.h"
#include "midichannel.h"
#include "sync.h"
#include "audio.h"
#include "gconfig.h"

static const unsigned char mmcDeferredPlayMsg[] = { 0x7f, 0x7f, 0x06, 0x03 };

//---------------------------------------------------------
//   sendGmOn
//    send GM-On message to midi device and keep track
//    of device state
//---------------------------------------------------------

void MidiOut::sendGmOn()
      {
      sendSysex(gmOnMsg, gmOnMsgLen);
      track->setHwCtrlState(CTRL_PROGRAM,      0);
      track->setHwCtrlState(CTRL_PITCH,        0);
      track->setHwCtrlState(CTRL_VOLUME,     100);
      track->setHwCtrlState(CTRL_PANPOT,      64);
      track->setHwCtrlState(CTRL_REVERB_SEND, 40);
      track->setHwCtrlState(CTRL_CHORUS_SEND,  0);
      track->setMeter(0, 0.0);
      }

//---------------------------------------------------------
//   sendGsOn
//    send Roland GS-On message to midi device and keep track
//    of device state
//---------------------------------------------------------

void MidiOut::sendGsOn()
      {
      static unsigned char data2[] = { 0x41, 0x10, 0x42, 0x12, 0x40, 0x01, 0x33, 0x50, 0x3c };
      static unsigned char data3[] = { 0x41, 0x10, 0x42, 0x12, 0x40, 0x01, 0x34, 0x50, 0x3b };

      sendSysex(data2, sizeof(data2));
      sendSysex(data3, sizeof(data3));
      }

//---------------------------------------------------------
//   sendXgOn
//    send Yamaha XG-On message to midi device and keep track
//    of device state
//---------------------------------------------------------

void MidiOut::sendXgOn()
      {
      sendSysex(xgOnMsg, xgOnMsgLen);
      track->setHwCtrlState(CTRL_PROGRAM, 0);
      track->setHwCtrlState(CTRL_MODULATION, 0);
      track->setHwCtrlState(CTRL_PORTAMENTO_TIME, 0);
      track->setHwCtrlState(CTRL_VOLUME, 0x64);
      track->setHwCtrlState(CTRL_PANPOT, 0x40);
      track->setHwCtrlState(CTRL_EXPRESSION, 0x7f);
      track->setHwCtrlState(CTRL_SUSTAIN, 0x0);
      track->setHwCtrlState(CTRL_PORTAMENTO, 0x0);
      track->setHwCtrlState(CTRL_SOSTENUTO, 0x0);
      track->setHwCtrlState(CTRL_SOFT_PEDAL, 0x0);
      track->setHwCtrlState(CTRL_HARMONIC_CONTENT, 0x40);
      track->setHwCtrlState(CTRL_RELEASE_TIME, 0x40);
      track->setHwCtrlState(CTRL_ATTACK_TIME, 0x40);
      track->setHwCtrlState(CTRL_BRIGHTNESS, 0x40);
      track->setHwCtrlState(CTRL_REVERB_SEND, 0x28);
      track->setHwCtrlState(CTRL_CHORUS_SEND, 0x0);
      track->setHwCtrlState(CTRL_VARIATION_SEND, 0x0);
      track->setMeter(0, 0.0);
      }

//---------------------------------------------------------
//   sendSysex
//    send SYSEX message to midi device
//---------------------------------------------------------

void MidiOut::sendSysex(const unsigned char* p, int n)
      {
      MidiEvent event(0, ME_SYSEX, p, n);
      track->routeEvent(event);
      }

//---------------------------------------------------------
//   sendStart
//---------------------------------------------------------

void MidiOut::sendStart()
      {
      MidiEvent event(0, 0, ME_START, 0, 0);
      track->routeEvent(event);
      }

//---------------------------------------------------------
//   sendStop
//---------------------------------------------------------

void MidiOut::sendStop()
      {
      MidiEvent event(0, 0, ME_STOP, 0, 0);
      track->routeEvent(event);
      }

//---------------------------------------------------------
//   sendClock
//---------------------------------------------------------

void MidiOut::sendClock()
      {
      MidiEvent event(0, 0, ME_CLOCK, 0, 0);
      track->routeEvent(event);
      }

//---------------------------------------------------------
//   sendContinue
//---------------------------------------------------------

void MidiOut::sendContinue()
      {
      MidiEvent event(0, 0, ME_CONTINUE, 0, 0);
      track->routeEvent(event);
      }

//---------------------------------------------------------
//   sendSongpos
//---------------------------------------------------------

void MidiOut::sendSongpos(int pos)
      {
      MidiEvent event(0, 0, ME_SONGPOS, pos, 0);
      track->routeEvent(event);
      }

//---------------------------------------------------------
//   playMidiEvent
//    called from GUI
//---------------------------------------------------------

void MidiOut::playMidiEvent(MidiEvent* ev)
      {
	if (eventFifo.put(*ev))
            printf("MidiPort::playMidiEvent(): port overflow, drop event\n");
      }

//---------------------------------------------------------
//    seek
//---------------------------------------------------------

void MidiOut::seek(unsigned tickPos, unsigned framePos)
      {
      if (genMCSync && track->sendSync()) {
            int beat = (tickPos * 4) / config.division;
            sendStop();
            sendSongpos(beat);
            sendContinue();
            }
      if (track->mute())
            return;

//    if (pos == 0 && !song->record())
//          audio->initDevices();

      //---------------------------------------------------
      //    stop all notes
      //---------------------------------------------------

      for (iMPEvent i = _schedEvents.begin(); i != _schedEvents.end(); ++i) {
            MidiEvent ev = *i;
            if (ev.isNoteOff()) {
                  ev.setTime(framePos);
                  track->routeEvent(ev);
                  }
            }
      _schedEvents.clear();

      //---------------------------------------------------
      //    set all controller
      //---------------------------------------------------

      for (int ch = 0; ch < MIDI_CHANNELS; ++ch) {
            MidiChannel* mc = channel(ch);
            if (mc->mute() || mc->noInRoute() || !mc->autoRead())
                  continue;
            CtrlList* cll = mc->controller();
            for (iCtrl ivl = cll->begin(); ivl != cll->end(); ++ivl) {
                  Ctrl* c   = ivl->second;
                  int val = c->value(tickPos).i;
                  if (val != CTRL_VAL_UNKNOWN && val != c->curVal().i) {
                        track->routeEvent(MidiEvent(0, ch, ME_CONTROLLER, c->id(), val));
                        }
                  }
            }
      }

//---------------------------------------------------------
//   stop
//---------------------------------------------------------

void MidiOut::stop()
      {
      int frame = AL::tempomap.tick2frame(audio->curTickPos());

      //---------------------------------------------------
      //    stop all notes
      //---------------------------------------------------

      for (iMPEvent i = _schedEvents.begin(); i != _schedEvents.end(); ++i) {
            MidiEvent ev = *i;
            if (ev.isNoteOff()) {
                  ev.setTime(frame);
                  track->routeEvent(ev);
                  }
            }
      _schedEvents.clear();

      //---------------------------------------------------
      //    reset sustain
      //---------------------------------------------------

      for (int ch = 0; ch < MIDI_CHANNELS; ++ch) {
            MidiChannel* mc = channel(ch);
            if (mc->noInRoute())
                  continue;
            if (mc->hwCtrlState(CTRL_SUSTAIN) != CTRL_VAL_UNKNOWN) {
                  MidiEvent ev(0, 0, ME_CONTROLLER, CTRL_SUSTAIN, 0);
                  ev.setChannel(mc->channelNo());
                  track->routeEvent(ev);
                  }
            }
      if (track->sendSync()) {
            if (genMMC) {
                  unsigned char mmcPos[] = {
                        0x7f, 0x7f, 0x06, 0x44, 0x06, 0x01,
                        0, 0, 0, 0, 0
                        };
                  MTC mtc(double(frame) / double(AL::sampleRate));
                  mmcPos[6] = mtc.h() | (AL::mtcType << 5);
                  mmcPos[7] = mtc.m();
                  mmcPos[8] = mtc.s();
                  mmcPos[9] = mtc.f();
                  mmcPos[10] = mtc.sf();
//TODO                  sendSysex(mmcStopMsg, sizeof(mmcStopMsg));
                  sendSysex(mmcPos, sizeof(mmcPos));
                  }
            if (genMCSync) {         // Midi Clock
                  // send STOP and
                  // "set song position pointer"
                  sendStop();
                  sendSongpos(audio->curTickPos() * 4 / config.division);
                  }
            }
      }

//---------------------------------------------------------
//   start
//---------------------------------------------------------

void MidiOut::start()
      {
      if (!(genMMC || genMCSync || track->sendSync()))
            return;
      if (genMMC)
            sendSysex(mmcDeferredPlayMsg, sizeof(mmcDeferredPlayMsg));
      if (genMCSync) {
            if (audio->curTickPos())
                  sendContinue();
            else
                  sendStart();
            }
      }

//---------------------------------------------------------
//   reset
//---------------------------------------------------------

void MidiOut::reset()
      {
/* TODO
      MidiEvent ev;
      ev.setType(0x90);
      for (int chan = 0; chan < MIDI_CHANNELS; ++chan) {
            ev.setChannel(chan);
            for (int pitch = 0; pitch < 128; ++pitch) {
                  ev.setA(pitch);
                  ev.setB(0);
                  mp->putEvent(ev);
                  }
            }
*/
      }

//-------------------------------------------------------------------
//   process
//    Collect all midi events for the current process cycle and put
//    into _schedEvents queue. For note on events create the proper
//    note off events. The note off events maybe played after the
//    current process cycle.
//    From _schedEvents queue copy all events for the current cycle
//    to all output routes. Events routed to ALSA go into the 
//    _playEvents queue which is processed by the MidiSeq thread.
//-------------------------------------------------------------------

void MidiOut::processMidi(MPEventList& el, unsigned fromTick, unsigned toTick, unsigned fromFrame, unsigned toFrame)
      {
      while (!eventFifo.isEmpty())
            el.add(eventFifo.get());

      // collect port controller
      if (fromTick != toTick) {     // if rolling
            CtrlList* cl = track->controller();
            for (iCtrl ic = cl->begin(); ic != cl->end(); ++ic) {
                  Ctrl* c = ic->second;
                  iCtrlVal is = c->lower_bound(fromTick);
                  iCtrlVal ie = c->lower_bound(toTick);
                  for (iCtrlVal ic = is; ic != ie; ++ic) {
                        unsigned frame = AL::tempomap.tick2frame(ic->first);
                        Event ev(Controller);
                        ev.setA(c->id());
                        ev.setB(ic->second.i);
                        el.add(MidiEvent(frame, -1, ev));
                        }
                  }
            }
      for (int ch = 0; ch < MIDI_CHANNELS; ++ch)  {
            MidiChannel* mc = channel(ch);

            if (mc->mute() || mc->noInRoute())
                  continue;
            // collect channel controller
            if (fromTick != toTick) {
                  CtrlList* cl = mc->controller();
                  for (iCtrl ic = cl->begin(); ic != cl->end(); ++ic) {
                        Ctrl* c = ic->second;
                        iCtrlVal is = c->lower_bound(fromTick);
                        iCtrlVal ie = c->lower_bound(toTick);
                        for (; is != ie; ++is) {
                              unsigned frame = AL::tempomap.tick2frame(is->first);
                              Event ev(Controller);
                              ev.setA(c->id());
                              ev.setB(is->second.i);
                              el.add(MidiEvent(frame, ch, ev));
                              }
                        }
                  }

            // Collect midievents from all input tracks for outport
            RouteList* rl = mc->inRoutes();
            for (iRoute i = rl->begin(); i != rl->end(); ++i) {
                  MidiTrackBase* track = (MidiTrackBase*)i->track;
                  if (track->isMute())
                        continue;
                  MPEventList ell;
                  track->getEvents(fromTick, toTick, 0, &ell);
                  int velo = 0;
                  for (iMPEvent i = ell.begin(); i != ell.end(); ++i) {
                        MidiEvent ev(*i);
                        ev.setChannel(ch);
                        el.insert(ev);
                        if (ev.type() == ME_NOTEON)
                              velo += ev.dataB();
                        }
                  mc->addMidiMeter(velo);
                  }
            }
      }

