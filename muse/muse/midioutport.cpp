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
#include "midiplugin.h"
#include "midictrl.h"
#include "al/al.h"
#include "al/tempo.h"
#include "al/xml.h"
#include "driver/mididev.h"
#include "driver/audiodev.h"
#include "audio.h"
#include "midioutport.h"
#include "midichannel.h"
#include "midiseq.h"
#include "sync.h"
#include "gconfig.h"

//---------------------------------------------------------
//   MidiOutPort
//---------------------------------------------------------

MidiOutPort::MidiOutPort()
   : MidiTrackBase(MIDI_OUT)
      {
      _instrument = genericMidiInstrument;
      for (int ch = 0; ch < MIDI_CHANNELS; ++ch)
            _channel[ch] = new MidiChannel(this, ch);
      _sendSync       = false;
      _deviceId       = 127;        // all
      addMidiController(_instrument, CTRL_MASTER_VOLUME);
      _channels = 1;
      }

//---------------------------------------------------------
//   MidiOutPort
//---------------------------------------------------------

MidiOutPort::~MidiOutPort()
      {
      for (int ch = 0; ch < MIDI_CHANNEL; ++ch)
            delete _channel[ch];
      }

//---------------------------------------------------------
//   setName
//---------------------------------------------------------

void MidiOutPort::setName(const QString& s)
      {
      Track::setName(s);
      if (!alsaPort().isZero())
            midiDriver->setPortName(alsaPort(), s);
      if (!jackPort().isZero())
            audioDriver->setPortName(jackPort(), s);
      for (int ch = 0; ch < MIDI_CHANNELS; ++ch)
            _channel[ch]->setDefaultName();
      }

//---------------------------------------------------------
//   MidiOutPort::write
//---------------------------------------------------------

void MidiOutPort::write(Xml& xml) const
      {
      xml.tag("MidiOutPort");
      MidiTrackBase::writeProperties(xml);
      if (_instrument)
            xml.strTag("instrument", _instrument->iname());
      for (int i = 0; i < MIDI_CHANNELS; ++i) {
            if (!_channel[i]->noInRoute())
                  _channel[i]->write(xml);
            }
      xml.intTag("sendSync", _sendSync);
      xml.intTag("deviceId", _deviceId);
      xml.etag("MidiOutPort");
      }

//---------------------------------------------------------
//   MidiOutPort::read
//---------------------------------------------------------

void MidiOutPort::read(QDomNode node)
      {
      for (; !node.isNull(); node = node.nextSibling()) {
            QDomElement e = node.toElement();
            QString tag(e.tagName());
            if (tag == "MidiChannel") {
                  int idx = e.attribute("idx", "0").toInt();
                  _channel[idx]->read(node.firstChild());
                  }
            else if (tag == "instrument") {
                  QString iname = e.text();
                  _instrument = registerMidiInstrument(iname);
                  }
            else if (tag == "sendSync")
                  _sendSync = e.text().toInt();
            else if (tag == "deviceId")
                  _deviceId = e.text().toInt();
            else if (MidiTrackBase::readProperties(node))
                  printf("MusE:MidiOutPort: unknown tag %s\n", tag.toLatin1().data());
            }
      }

//---------------------------------------------------------
//   putEvent
//    send event to alsa midi driver
//    called from MidiSeq::processTimerTick
//---------------------------------------------------------

#define playEvent(a) midiDriver->putEvent(alsaPort(), a);

void MidiOutPort::putEvent(const MidiEvent& ev)
      {
      if (ev.type() == ME_CONTROLLER) {
            int a   = ev.dataA();
            int b   = ev.dataB();
            int chn = ev.channel();
            if (chn == 255) {
                  // port controller
                  if (hwCtrlState(a) == ev.dataB())
                        return;
                  setHwCtrlState(a, b);
                  }
            else {
                  MidiChannel* mc = channel(chn);
                  //
                  //  optimize controller settings
                  //
                  if (mc->hwCtrlState(a) == ev.dataB())
                        return;
                  mc->setHwCtrlState(a, b);
                  }

            if (a == CTRL_PITCH) {
                  playEvent(MidiEvent(0, chn, ME_PITCHBEND, b, 0));
                  return;
                  }
            if (a == CTRL_PROGRAM) {
                  // don't output program changes for GM drum channel
//                  if (!(song->mtype() == MT_GM && chn == 9)) {
                        int hb = (b >> 16) & 0xff;
                        int lb = (b >> 8) & 0xff;
                        int pr = b & 0x7f;
                        if (hb != 0xff)
                              playEvent(MidiEvent(0, chn, ME_CONTROLLER, CTRL_HBANK, hb));
                        if (lb != 0xff)
                              playEvent(MidiEvent(0, chn, ME_CONTROLLER, CTRL_LBANK, lb));
                        playEvent(MidiEvent(0, chn, ME_PROGRAM, pr, 0));
                        return;
//                        }
                  }
            if (a == CTRL_MASTER_VOLUME) {
                  unsigned char sysex[] = {
                        0x7f, 0x7f, 0x04, 0x01, 0x00, 0x00
                        };
                  sysex[1] = _deviceId;
                  sysex[4] = b & 0x7f;
                  sysex[5] = (b >> 7) & 0x7f;
                  MidiEvent e(ev.time(), ME_SYSEX, sysex, 6);
                  playEvent(e);
                  return;
                  }

#if 1 // if ALSA cannot handle RPN NRPN etc.
            if (a < 0x1000) {          // 7 Bit Controller
                  //putMidiEvent(MidiEvent(0, chn, ME_CONTROLLER, a, b));
                  playEvent(ev);
                  }
            else if (a < 0x20000) {     // 14 bit high resolution controller
                  int ctrlH = (a >> 8) & 0x7f;
                  int ctrlL = a & 0x7f;
                  int dataH = (b >> 7) & 0x7f;
                  int dataL = b & 0x7f;
                  playEvent(MidiEvent(0, chn, ME_CONTROLLER, ctrlH, dataH));
                  playEvent(MidiEvent(0, chn, ME_CONTROLLER, ctrlL, dataL));
                  }
            else if (a < 0x30000) {     // RPN 7-Bit Controller
                  int ctrlH = (a >> 8) & 0x7f;
                  int ctrlL = a & 0x7f;
                  playEvent(MidiEvent(0, chn, ME_CONTROLLER, CTRL_HRPN, ctrlH));
                  playEvent(MidiEvent(0, chn, ME_CONTROLLER, CTRL_LRPN, ctrlL));
                  playEvent(MidiEvent(0, chn, ME_CONTROLLER, CTRL_HDATA, b));
                  }
            else if (a < 0x40000) {     // NRPN 7-Bit Controller
                  int ctrlH = (a >> 8) & 0x7f;
                  int ctrlL = a & 0x7f;
                  playEvent(MidiEvent(0, chn, ME_CONTROLLER, CTRL_HNRPN, ctrlH));
                  playEvent(MidiEvent(0, chn, ME_CONTROLLER, CTRL_LNRPN, ctrlL));
                  playEvent(MidiEvent(0, chn, ME_CONTROLLER, CTRL_HDATA, b));
                  }
            else if (a < 0x60000) {     // RPN14 Controller
                  int ctrlH = (a >> 8) & 0x7f;
                  int ctrlL = a & 0x7f;
                  int dataH = (b >> 7) & 0x7f;
                  int dataL = b & 0x7f;
                  playEvent(MidiEvent(0, chn, ME_CONTROLLER, CTRL_HRPN, ctrlH));
                  playEvent(MidiEvent(0, chn, ME_CONTROLLER, CTRL_LRPN, ctrlL));
                  playEvent(MidiEvent(0, chn, ME_CONTROLLER, CTRL_HDATA, dataH));
                  playEvent(MidiEvent(0, chn, ME_CONTROLLER, CTRL_LDATA, dataL));
                  }
            else if (a < 0x70000) {     // NRPN14 Controller
                  int ctrlH = (a >> 8) & 0x7f;
                  int ctrlL = a & 0x7f;
                  int dataH = (b >> 7) & 0x7f;
                  int dataL = b & 0x7f;
                  playEvent(MidiEvent(0, chn, ME_CONTROLLER, CTRL_HNRPN, ctrlH));
                  playEvent(MidiEvent(0, chn, ME_CONTROLLER, CTRL_LNRPN, ctrlL));
                  playEvent(MidiEvent(0, chn, ME_CONTROLLER, CTRL_HDATA, dataH));
                  playEvent(MidiEvent(0, chn, ME_CONTROLLER, CTRL_LDATA, dataL));
                  }
            else {
                  printf("putEvent: unknown controller type 0x%x\n", a);
                  }
#endif
            }
      playEvent(ev);
      }

//---------------------------------------------------------
//   sendGmOn
//    send GM-On message to midi device and keep track
//    of device state
//---------------------------------------------------------

void MidiOutPort::sendGmOn()
      {
      sendSysex(gmOnMsg, gmOnMsgLen);
      setHwCtrlState(CTRL_PROGRAM,      0);
      setHwCtrlState(CTRL_PITCH,        0);
      setHwCtrlState(CTRL_VOLUME,     100);
      setHwCtrlState(CTRL_PANPOT,      64);
      setHwCtrlState(CTRL_REVERB_SEND, 40);
      setHwCtrlState(CTRL_CHORUS_SEND,  0);
      _meter[0] = 0.0f;
      }

//---------------------------------------------------------
//   sendGsOn
//    send Roland GS-On message to midi device and keep track
//    of device state
//---------------------------------------------------------

void MidiOutPort::sendGsOn()
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

void MidiOutPort::sendXgOn()
      {
      sendSysex(xgOnMsg, xgOnMsgLen);
      setHwCtrlState(CTRL_PROGRAM, 0);
      setHwCtrlState(CTRL_MODULATION, 0);
      setHwCtrlState(CTRL_PORTAMENTO_TIME, 0);
      setHwCtrlState(CTRL_VOLUME, 0x64);
      setHwCtrlState(CTRL_PANPOT, 0x40);
      setHwCtrlState(CTRL_EXPRESSION, 0x7f);
      setHwCtrlState(CTRL_SUSTAIN, 0x0);
      setHwCtrlState(CTRL_PORTAMENTO, 0x0);
      setHwCtrlState(CTRL_SOSTENUTO, 0x0);
      setHwCtrlState(CTRL_SOFT_PEDAL, 0x0);
      setHwCtrlState(CTRL_HARMONIC_CONTENT, 0x40);
      setHwCtrlState(CTRL_RELEASE_TIME, 0x40);
      setHwCtrlState(CTRL_ATTACK_TIME, 0x40);
      setHwCtrlState(CTRL_BRIGHTNESS, 0x40);
      setHwCtrlState(CTRL_REVERB_SEND, 0x28);
      setHwCtrlState(CTRL_CHORUS_SEND, 0x0);
      setHwCtrlState(CTRL_VARIATION_SEND, 0x0);
      _meter[0] = 0.0f;
      }

//---------------------------------------------------------
//   sendSysex
//    send SYSEX message to midi device
//---------------------------------------------------------

void MidiOutPort::sendSysex(const unsigned char* p, int n)
      {
      MidiEvent event(0, ME_SYSEX, p, n);
      putEvent(event);
      }

//---------------------------------------------------------
//   sendStart
//---------------------------------------------------------

void MidiOutPort::sendStart()
      {
      MidiEvent event(0, 0, ME_START, 0, 0);
      putEvent(event);
      }

//---------------------------------------------------------
//   sendStop
//---------------------------------------------------------

void MidiOutPort::sendStop()
      {
      MidiEvent event(0, 0, ME_STOP, 0, 0);
      putEvent(event);
      }

//---------------------------------------------------------
//   sendClock
//---------------------------------------------------------

void MidiOutPort::sendClock()
      {
      MidiEvent event(0, 0, ME_CLOCK, 0, 0);
      putEvent(event);
      }

//---------------------------------------------------------
//   sendContinue
//---------------------------------------------------------

void MidiOutPort::sendContinue()
      {
      MidiEvent event(0, 0, ME_CONTINUE, 0, 0);
      putEvent(event);
      }

//---------------------------------------------------------
//   sendSongpos
//---------------------------------------------------------

void MidiOutPort::sendSongpos(int pos)
      {
      MidiEvent event(0, 0, ME_SONGPOS, pos, 0);
      putEvent(event);
      }

//---------------------------------------------------------
//   playMidiEvent
//    called from GUI
//---------------------------------------------------------

void MidiOutPort::playMidiEvent(MidiEvent* ev)
      {
      if (ev->type() == ME_NOTEON) {
            _meter[0] += ev->dataB()/2;
            if (_meter[0] > 127.0f)
                  _meter[0] = 127.0f;
            }
      RouteList* orl = outRoutes();
      bool sendToFifo = false;
      for (iRoute i = orl->begin(); i != orl->end(); ++i) {
            if (i->type == Route::SYNTIPORT) {
                  SynthI* synti = (SynthI*)i->track;
	      	if (synti->eventFifo()->put(*ev))
      	      	printf("MidiOut::playMidiEvent(): synti overflow, drop event\n");
                  }
            else 
                  sendToFifo = true;
            }
      if (sendToFifo) {
	      if (eventFifo.put(*ev))
      	      printf("MidiPort::playMidiEvent(): port overflow, drop event\n");
            }
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

void MidiOutPort::process(unsigned fromTick, unsigned toTick, unsigned, unsigned toFrame)
      {
      if (mute())
            return;

      MPEventList el;
      while (!eventFifo.isEmpty())
            el.add(eventFifo.get());

      // collect port controller
      if (fromTick != toTick) {     // if rolling
            CtrlList* cl = controller();
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

      int portVelo = 0;
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
                  portVelo += velo;
                  }
            }
      addMidiMeter(portVelo);

      pipeline()->apply(fromTick, toTick, &el, &_schedEvents);

      //
      // route events to destination
      //

      for (iMPEvent i = _schedEvents.begin(); i != _schedEvents.end(); ++i) {
            if (i->time() >= toFrame) {
                  _schedEvents.erase(_schedEvents.begin(), i);
                  break;
                  }
            routeEvent(*i);
            }
      }

//---------------------------------------------------------
//    routeEvent
//---------------------------------------------------------

void MidiOutPort::routeEvent(const MidiEvent& event)
      {
      for (iRoute r = _outRoutes.begin(); r != _outRoutes.end(); ++r) {
            switch (r->type) {
                  case Route::MIDIPORT:
                        midiBusy = true;
                        _playEvents.insert(event);    // schedule
                        midiBusy = false;
                        break;
                  case Route::SYNTIPORT: 
                        ((SynthI*)(r->track))->playEvents()->insert(event);
                        break;
                  case Route::JACKMIDIPORT:
                        audioDriver->putEvent(jackPort(0), event);
                        break;
                  default:
                        fprintf(stderr, "MidiOutPort::process(): invalid routetype\n");
                        break;
                  }
            }
      }

//---------------------------------------------------------
//   setInstrument
//---------------------------------------------------------

void MidiOutPort::setInstrument(MidiInstrument* i)
      {
      _instrument = i;
      emit instrumentChanged();
      }

//---------------------------------------------------------
//   setSendSync
//---------------------------------------------------------

void MidiOutPort::setSendSync(bool val)
      {
      _sendSync = val;
      emit sendSyncChanged(val);
      }

//---------------------------------------------------------
//    seek
//---------------------------------------------------------

void MidiOutPort::seek(unsigned tickPos, unsigned framePos)
      {
      if (genMCSync && sendSync()) {
            int beat = (tickPos * 4) / config.division;
            sendStop();
            sendSongpos(beat);
            sendContinue();
            }
      if (mute())
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
                  routeEvent(ev);
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
                        routeEvent(MidiEvent(0, ch, ME_CONTROLLER, c->id(), val));
                        }
                  }
            }
      }

//---------------------------------------------------------
//   stop
//---------------------------------------------------------

void MidiOutPort::stop()
      {
      int frame = AL::tempomap.tick2frame(audio->curTickPos());

      //---------------------------------------------------
      //    stop all notes
      //---------------------------------------------------

      for (iMPEvent i = _schedEvents.begin(); i != _schedEvents.end(); ++i) {
            MidiEvent ev = *i;
            if (ev.isNoteOff()) {
                  ev.setTime(frame);
                  routeEvent(ev);
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
                  routeEvent(ev);
                  }
            }

      if (sendSync()) {
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
//TODO                  mp->sendSysex(mmcStopMsg, sizeof(mmcStopMsg));
//                  mp->sendSysex(mmcPos, sizeof(mmcPos));
                  }
            if (genMCSync) {         // Midi Clock
                  // send STOP and
                  // "set song position pointer"
//                  mp->sendStop();
//                  mp->sendSongpos(audio->curTickPos() * 4 / config.division);
                  }
            }
      }

//---------------------------------------------------------
//   start
//---------------------------------------------------------

void MidiOutPort::start()
      {
#if 0
      if (!(genMMC || genMCSync))
            return;
      if (!sendSync())
            return;
      if (genMMC)
            sendSysex(mmcDeferredPlayMsg, sizeof(mmcDeferredPlayMsg));
      if (genMCSync) {
            if (audio->curTickPos())
                  sendContinue();
            else
                  sendStart();
            }
#endif
      }

//---------------------------------------------------------
//   reset
//---------------------------------------------------------

void MidiOutPort::reset()
      {
      instrument()->reset(this);
      }

