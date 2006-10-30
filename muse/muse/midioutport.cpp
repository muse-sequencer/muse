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
#include "instruments/minstrument.h"

//---------------------------------------------------------
//   MidiOutPort
//---------------------------------------------------------

MidiOutPort::MidiOutPort()
   : MidiTrackBase()
      {
      track      = this;
      _instrument = genericMidiInstrument;
      for (int ch = 0; ch < MIDI_CHANNELS; ++ch)
            _channel[ch] = new MidiChannel(this, ch);
      setDeviceId(127);        // all
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
      xml.intTag("sendSync", sendSync());
      xml.intTag("deviceId", deviceId());
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
                  setSendSync(e.text().toInt());
            else if (tag == "deviceId")
                  setDeviceId(e.text().toInt());
            else if (MidiTrackBase::readProperties(node))
                  printf("MusE:MidiOutPort: unknown tag %s\n", tag.toLatin1().data());
            }
      }

//---------------------------------------------------------
//    routeEvent
//---------------------------------------------------------

void MidiOutPort::routeEvent(const MidiEvent& event)
      {
      if (event.type() == ME_CONTROLLER) {
            int a   = event.dataA();
            int b   = event.dataB();
            int chn = event.channel();
            if (chn == 255) {
                  // port controller
                  if (hwCtrlState(a) == event.dataB()) {
//                        printf(" controller change optimized away 1\n");
                        return;
                        }
                  setHwCtrlState(a, b);
                  }
            else {
                  MidiChannel* mc = channel(chn);
                  //
                  //  optimize controller settings
                  //
                  if (mc->hwCtrlState(a) == event.dataB()) {
//                        printf(" controller change optimized away 2\n");
                        return;
                        }
                  mc->setHwCtrlState(a, b);
                  }
            }

      for (iRoute r = _outRoutes.begin(); r != _outRoutes.end(); ++r) {
            switch (r->type) {
                  case Route::MIDIPORT:
                        queueAlsaEvent(event);
                        break;
#if 0
                  case Route::SYNTIPORT: 
                  case Route::TRACK: 
                        ((SynthI*)(r->track))->playEvents()->insert(event);
                        break;
#endif
                  case Route::JACKMIDIPORT:
                        queueJackEvent(event);
                        break;
                  default:
                        fprintf(stderr, "MidiOutPort::process(): invalid routetype\n");
                        break;
                  }
            }
      }

//---------------------------------------------------------
//    queueAlsaEvent
//    called from MidiSeq
//---------------------------------------------------------

void MidiOutPort::queueAlsaEvent(const MidiEvent& ev)
      {
      midiBusy = true;
      if (ev.type() == ME_CONTROLLER) {
            int a      = ev.dataA();
            int b      = ev.dataB();
            int chn    = ev.channel();
            unsigned t = ev.time();

            if (a == CTRL_PROGRAM) {
                  // don't output program changes for GM drum channel
//                  if (!(song->mtype() == MT_GM && chn == 9)) {
                        int hb = (b >> 16) & 0xff;
                        int lb = (b >> 8) & 0xff;
                        int pr = b & 0x7f;
                        if (hb != 0xff)
                              _playEvents.insert(MidiEvent(t, chn, ME_CONTROLLER, CTRL_HBANK, hb));
                        if (lb != 0xff)
                              _playEvents.insert(MidiEvent(t+1, chn, ME_CONTROLLER, CTRL_LBANK, lb));
                        _playEvents.insert(MidiEvent(t+2, chn, ME_PROGRAM, pr, 0));
//                        }
                  }
            else if (a == CTRL_MASTER_VOLUME) {
                  unsigned char sysex[] = {
                        0x7f, 0x7f, 0x04, 0x01, 0x00, 0x00
                        };
                  sysex[1] = deviceId();
                  sysex[4] = b & 0x7f;
                  sysex[5] = (b >> 7) & 0x7f;
                  _playEvents.insert(MidiEvent(t, ME_SYSEX, sysex, 6));
                  }
            else if (a < CTRL_14_OFFSET)               // 7 Bit Controller
                  _playEvents.insert(ev);
            else if (a < CTRL_RPN_OFFSET) {     // 14 bit high resolution controller
                  int ctrlH = (a >> 8) & 0x7f;
                  int ctrlL = a & 0x7f;
                  int dataH = (b >> 7) & 0x7f;
                  int dataL = b & 0x7f;
                  _playEvents.insert(MidiEvent(t, chn, ME_CONTROLLER, ctrlH, dataH));
                  _playEvents.insert(MidiEvent(t+1, chn, ME_CONTROLLER, ctrlL, dataL));
                  }
            else if (a < CTRL_NRPN_OFFSET) {     // RPN 7-Bit Controller
                  int ctrlH = (a >> 8) & 0x7f;
                  int ctrlL = a & 0x7f;
                  _playEvents.insert(MidiEvent(t, chn, ME_CONTROLLER, CTRL_HRPN, ctrlH));
                  _playEvents.insert(MidiEvent(t+1, chn, ME_CONTROLLER, CTRL_LRPN, ctrlL));
                  _playEvents.insert(MidiEvent(t+2, chn, ME_CONTROLLER, CTRL_HDATA, b));
                  }
            else if (a < CTRL_RPN14_OFFSET) {     // NRPN 7-Bit Controller
                  int ctrlH = (a >> 8) & 0x7f;
                  int ctrlL = a & 0x7f;
                  _playEvents.insert(MidiEvent(t, chn, ME_CONTROLLER, CTRL_HNRPN, ctrlH));
                  _playEvents.insert(MidiEvent(t+1, chn, ME_CONTROLLER, CTRL_LNRPN, ctrlL));
                  _playEvents.insert(MidiEvent(t+2, chn, ME_CONTROLLER, CTRL_HDATA, b));
                  }
            else if (a < CTRL_NRPN14_OFFSET) {     // RPN14 Controller
                  int ctrlH = (a >> 8) & 0x7f;
                  int ctrlL = a & 0x7f;
                  int dataH = (b >> 7) & 0x7f;
                  int dataL = b & 0x7f;
                  _playEvents.insert(MidiEvent(t, chn, ME_CONTROLLER, CTRL_HRPN, ctrlH));
                  _playEvents.insert(MidiEvent(t+1, chn, ME_CONTROLLER, CTRL_LRPN, ctrlL));
                  _playEvents.insert(MidiEvent(t+2, chn, ME_CONTROLLER, CTRL_HDATA, dataH));
                  _playEvents.insert(MidiEvent(t+3, chn, ME_CONTROLLER, CTRL_LDATA, dataL));
                  }
            else if (a < CTRL_NONE_OFFSET) {     // NRPN14 Controller
                  int ctrlH = (a >> 8) & 0x7f;
                  int ctrlL = a & 0x7f;
                  int dataH = (b >> 7) & 0x7f;
                  int dataL = b & 0x7f;
                  _playEvents.insert(MidiEvent(t, chn, ME_CONTROLLER, CTRL_HNRPN, ctrlH));
                  _playEvents.insert(MidiEvent(t+1, chn, ME_CONTROLLER, CTRL_LNRPN, ctrlL));
                  _playEvents.insert(MidiEvent(t+2, chn, ME_CONTROLLER, CTRL_HDATA, dataH));
                  _playEvents.insert(MidiEvent(t+3, chn, ME_CONTROLLER, CTRL_LDATA, dataL));
                  }
            else {
                  printf("putEvent: unknown controller type 0x%x\n", a);
                  }
            }
      else
            _playEvents.insert(ev);
      midiBusy = false;
      }

//---------------------------------------------------------
//    queueJackEvent
//    called from MidiSeq
//---------------------------------------------------------

#define JO(e) audioDriver->putEvent(jackPort(0), e);

void MidiOutPort::queueJackEvent(const MidiEvent& ev)
      {
      if (ev.type() == ME_CONTROLLER) {
            int a      = ev.dataA();
            int b      = ev.dataB();
            int chn    = ev.channel();
            unsigned t = ev.time();

            if (a == CTRL_PROGRAM) {
                  // don't output program changes for GM drum channel
//                  if (!(song->mtype() == MT_GM && chn == 9)) {
                        int hb = (b >> 16) & 0xff;
                        int lb = (b >> 8) & 0xff;
                        int pr = b & 0x7f;
                        if (hb != 0xff)
                              JO(MidiEvent(t, chn, ME_CONTROLLER, CTRL_HBANK, hb));
                        if (lb != 0xff)
                              JO(MidiEvent(t+1, chn, ME_CONTROLLER, CTRL_LBANK, lb));
                        JO(MidiEvent(t+2, chn, ME_PROGRAM, pr, 0));
//                        }
                  }
            else if (a == CTRL_MASTER_VOLUME) {
                  unsigned char sysex[] = {
                        0x7f, 0x7f, 0x04, 0x01, 0x00, 0x00
                        };
                  sysex[1] = deviceId();
                  sysex[4] = b & 0x7f;
                  sysex[5] = (b >> 7) & 0x7f;
                  JO(MidiEvent(t, ME_SYSEX, sysex, 6));
                  }
            else if (a < CTRL_14_OFFSET) {              // 7 Bit Controller
                  JO(ev);
                  }
            else if (a < CTRL_RPN_OFFSET) {     // 14 bit high resolution controller
                  int ctrlH = (a >> 8) & 0x7f;
                  int ctrlL = a & 0x7f;
                  int dataH = (b >> 7) & 0x7f;
                  int dataL = b & 0x7f;
                  JO(MidiEvent(t,   chn, ME_CONTROLLER, ctrlH, dataH));
                  JO(MidiEvent(t+1, chn, ME_CONTROLLER, ctrlL, dataL));
                  }
            else if (a < CTRL_NRPN_OFFSET) {     // RPN 7-Bit Controller
                  int ctrlH = (a >> 8) & 0x7f;
                  int ctrlL = a & 0x7f;
                  JO(MidiEvent(t,   chn, ME_CONTROLLER, CTRL_HRPN, ctrlH));
                  JO(MidiEvent(t+1, chn, ME_CONTROLLER, CTRL_LRPN, ctrlL));
                  JO(MidiEvent(t+2, chn, ME_CONTROLLER, CTRL_HDATA, b));
                  }
            else if (a < CTRL_RPN14_OFFSET) {     // NRPN 7-Bit Controller
                  int ctrlH = (a >> 8) & 0x7f;
                  int ctrlL = a & 0x7f;
                  JO(MidiEvent(t,   chn, ME_CONTROLLER, CTRL_HNRPN, ctrlH));
                  JO(MidiEvent(t+1, chn, ME_CONTROLLER, CTRL_LNRPN, ctrlL));
                  JO(MidiEvent(t+2, chn, ME_CONTROLLER, CTRL_HDATA, b));
                  }
            else if (a < CTRL_NRPN14_OFFSET) {     // RPN14 Controller
                  int ctrlH = (a >> 8) & 0x7f;
                  int ctrlL = a & 0x7f;
                  int dataH = (b >> 7) & 0x7f;
                  int dataL = b & 0x7f;
                  JO(MidiEvent(t,   chn, ME_CONTROLLER, CTRL_HRPN, ctrlH));
                  JO(MidiEvent(t+1, chn, ME_CONTROLLER, CTRL_LRPN, ctrlL));
                  JO(MidiEvent(t+2, chn, ME_CONTROLLER, CTRL_HDATA, dataH));
                  JO(MidiEvent(t+3, chn, ME_CONTROLLER, CTRL_LDATA, dataL));
                  }
            else if (a < CTRL_NONE_OFFSET) {     // NRPN14 Controller
                  int ctrlH = (a >> 8) & 0x7f;
                  int ctrlL = a & 0x7f;
                  int dataH = (b >> 7) & 0x7f;
                  int dataL = b & 0x7f;
                  JO(MidiEvent(t, chn, ME_CONTROLLER, CTRL_HNRPN, ctrlH));
                  JO(MidiEvent(t+1, chn, ME_CONTROLLER, CTRL_LNRPN, ctrlL));
                  JO(MidiEvent(t+2, chn, ME_CONTROLLER, CTRL_HDATA, dataH));
                  JO(MidiEvent(t+3, chn, ME_CONTROLLER, CTRL_LDATA, dataL));
                  }
            else {
                  printf("putEvent: unknown controller type 0x%x\n", a);
                  }
            }
      else {
            JO(ev);
            }
      }
#undef JO

//---------------------------------------------------------
//    playAlsaEvent
//    called from MidiSeq
//---------------------------------------------------------

void MidiOutPort::playAlsaEvent(const MidiEvent& event) const
      {
      midiDriver->putEvent(alsaPort(), event);
      }

//---------------------------------------------------------
//   setInstrument
//---------------------------------------------------------

void MidiOutPort::setInstrument(MidiInstrument* i)
      {
      _instrument = i;
      emit instrumentChanged();
      }

#if 0
//---------------------------------------------------------
//   processMidiClock
//---------------------------------------------------------

void MidiSeq::processMidiClock()
      {
      if (genMCSync)
            midiPorts[txSyncPort].sendClock();
      if (state == START_PLAY) {
            // start play on sync
            state      = PLAY;
            _midiTick  = playTickPos;
            midiClock  = playTickPos;

            int bar, beat, tick;
            sigmap.tickValues(_midiTick, &bar, &beat, &tick);
            midiClick      = sigmap.bar2tick(bar, beat+1, 0);

            double cpos    = tempomap.tick2time(playTickPos);
            samplePosStart = samplePos - lrint(cpos * sampleRate);
            rtcTickStart   = rtcTick - lrint(cpos * realRtcTicks);

            endSlice       = playTickPos;
            lastTickPos    = playTickPos;

            tempoSN = tempomap.tempoSN();

            startRecordPos.setPosTick(playTickPos);
            }
      midiClock += config.division/24;
      }
#endif

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

void MidiOutPort::processMidi(unsigned fromTick, unsigned toTick, unsigned fromFrame, unsigned toFrame)
      {
      if (track->mute())
            return;

      MPEventList el;
      MidiOut::processMidi(el, fromTick, toTick, fromFrame, toFrame);

      pipeline()->apply(fromTick, toTick, &el, &_schedEvents);

      //
      // route events to destination
      //

      int portVelo = 0;
      iMPEvent i = _schedEvents.begin();
      for (; i != _schedEvents.end(); ++i) {
            if (i->time() >= toFrame)
                  break;
            routeEvent(*i);
            if (i->type() == ME_NOTEON)
                  portVelo += i->dataB();
            }
      _schedEvents.erase(_schedEvents.begin(), i);
      addMidiMeter(portVelo);
      }


