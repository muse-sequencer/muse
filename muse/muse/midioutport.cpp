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
#include "al/xml.h"
#include "driver/mididev.h"
#include "driver/audiodev.h"
#include "audio.h"
#include "midioutport.h"
#include "midichannel.h"

//---------------------------------------------------------
//   MidiOutPort
//---------------------------------------------------------

MidiOutPort::MidiOutPort()
   : MidiTrackBase(MIDI_OUT)
      {
      _instrument = genericMidiInstrument;
      for (int ch = 0; ch < MIDI_CHANNELS; ++ch)
            _channel[ch] = new MidiChannel(this, ch);
      _nextPlayEvent  = _playEvents.end();
      _sendSync       = false;
      _deviceId       = 127;        // all
      addMidiController(_instrument, CTRL_MASTER_VOLUME);
      _channels = 1;
      }

//---------------------------------------------------------
//   playFifo
//---------------------------------------------------------

void MidiOutPort::playFifo()
      {
      while (!eventFifo.isEmpty())
            putEvent(eventFifo.get());
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
      if (alsaPort())
            midiDriver->setPortName(alsaPort(), s);
      if (jackPort())
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
//    send event to midi driver
//---------------------------------------------------------

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
                  midiDriver->putEvent(alsaPort(), MidiEvent(0, chn, ME_PITCHBEND, b, 0));
                  return;
                  }
            if (a == CTRL_PROGRAM) {
                  // don't output program changes for GM drum channel
//                  if (!(song->mtype() == MT_GM && chn == 9)) {
                        int hb = (b >> 16) & 0xff;
                        int lb = (b >> 8) & 0xff;
                        int pr = b & 0x7f;
                        if (hb != 0xff)
                              midiDriver->putEvent(alsaPort(), MidiEvent(0, chn, ME_CONTROLLER, CTRL_HBANK, hb));
                        if (lb != 0xff)
                              midiDriver->putEvent(alsaPort(), MidiEvent(0, chn, ME_CONTROLLER, CTRL_LBANK, lb));
                        midiDriver->putEvent(alsaPort(), MidiEvent(0, chn, ME_PROGRAM, pr, 0));
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
                  midiDriver->putEvent(alsaPort(), e);
                  return;
                  }

#if 1 // if ALSA cannot handle RPN NRPN etc.
            if (a < 0x1000) {          // 7 Bit Controller
                  //putMidiEvent(MidiEvent(0, chn, ME_CONTROLLER, a, b));
                  midiDriver->putEvent(alsaPort(), ev);
                  }
            else if (a < 0x20000) {     // 14 bit high resolution controller
                  int ctrlH = (a >> 8) & 0x7f;
                  int ctrlL = a & 0x7f;
                  int dataH = (b >> 7) & 0x7f;
                  int dataL = b & 0x7f;
                  midiDriver->putEvent(alsaPort(), MidiEvent(0, chn, ME_CONTROLLER, ctrlH, dataH));
                  midiDriver->putEvent(alsaPort(), MidiEvent(0, chn, ME_CONTROLLER, ctrlL, dataL));
                  }
            else if (a < 0x30000) {     // RPN 7-Bit Controller
                  int ctrlH = (a >> 8) & 0x7f;
                  int ctrlL = a & 0x7f;
                  midiDriver->putEvent(alsaPort(), MidiEvent(0, chn, ME_CONTROLLER, CTRL_HRPN, ctrlH));
                  midiDriver->putEvent(alsaPort(), MidiEvent(0, chn, ME_CONTROLLER, CTRL_LRPN, ctrlL));
                  midiDriver->putEvent(alsaPort(), MidiEvent(0, chn, ME_CONTROLLER, CTRL_HDATA, b));
                  }
            else if (a < 0x40000) {     // NRPN 7-Bit Controller
                  int ctrlH = (a >> 8) & 0x7f;
                  int ctrlL = a & 0x7f;
                  midiDriver->putEvent(alsaPort(), MidiEvent(0, chn, ME_CONTROLLER, CTRL_HNRPN, ctrlH));
                  midiDriver->putEvent(alsaPort(), MidiEvent(0, chn, ME_CONTROLLER, CTRL_LNRPN, ctrlL));
                  midiDriver->putEvent(alsaPort(), MidiEvent(0, chn, ME_CONTROLLER, CTRL_HDATA, b));
                  }
            else if (a < 0x60000) {     // RPN14 Controller
                  int ctrlH = (a >> 8) & 0x7f;
                  int ctrlL = a & 0x7f;
                  int dataH = (b >> 7) & 0x7f;
                  int dataL = b & 0x7f;
                  midiDriver->putEvent(alsaPort(), MidiEvent(0, chn, ME_CONTROLLER, CTRL_HRPN, ctrlH));
                  midiDriver->putEvent(alsaPort(), MidiEvent(0, chn, ME_CONTROLLER, CTRL_LRPN, ctrlL));
                  midiDriver->putEvent(alsaPort(), MidiEvent(0, chn, ME_CONTROLLER, CTRL_HDATA, dataH));
                  midiDriver->putEvent(alsaPort(), MidiEvent(0, chn, ME_CONTROLLER, CTRL_LDATA, dataL));
                  }
            else if (a < 0x70000) {     // NRPN14 Controller
                  int ctrlH = (a >> 8) & 0x7f;
                  int ctrlL = a & 0x7f;
                  int dataH = (b >> 7) & 0x7f;
                  int dataL = b & 0x7f;
                  midiDriver->putEvent(alsaPort(), MidiEvent(0, chn, ME_CONTROLLER, CTRL_HNRPN, ctrlH));
                  midiDriver->putEvent(alsaPort(), MidiEvent(0, chn, ME_CONTROLLER, CTRL_LNRPN, ctrlL));
                  midiDriver->putEvent(alsaPort(), MidiEvent(0, chn, ME_CONTROLLER, CTRL_HDATA, dataH));
                  midiDriver->putEvent(alsaPort(), MidiEvent(0, chn, ME_CONTROLLER, CTRL_LDATA, dataL));
                  }
            else {
                  printf("putEvent: unknown controller type 0x%x\n", a);
                  }
#endif
            }
      midiDriver->putEvent(alsaPort(), ev);
      }

//---------------------------------------------------------
//   playEventList
//---------------------------------------------------------

void MidiOutPort::playEventList()
      {
      for (; _nextPlayEvent != _playEvents.end(); ++_nextPlayEvent)
            midiDriver->putEvent(alsaPort(), *_nextPlayEvent);
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
            if (i->type == Route::MIDIPORT)
                  sendToFifo = true;
            else if (i->type == Route::SYNTIPORT) {
                  SynthI* synti = (SynthI*)i->track;
	      	if (synti->eventFifo()->put(*ev))
      	      	printf("MidiOut::playMidiEvent(): synti overflow, drop event\n");
                  }
            else
                  printf("MidiOutPort::playMidiEvent: bad route type\n");
            }
      if (sendToFifo) {
	      if (eventFifo.put(*ev))
      	      printf("MidiPort::playMidiEvent(): port overflow, drop event\n");
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


