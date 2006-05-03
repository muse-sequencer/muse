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
#include "globals.h"
#include "midi.h"
#include "instruments/minstrument.h"
#include "al/xml.h"
#include "driver/alsamidi.h"
#include "audio.h"
#include "midiedit/drummap.h"

//---------------------------------------------------------
//   midiPortsPopup
//---------------------------------------------------------

QMenu* midiPortsPopup(QWidget* parent)
      {
      QMenu* p = new QMenu(parent);
      MidiOutPortList* mpl = song->midiOutPorts();
      for (iMidiOutPort i = mpl->begin(); i != mpl->end(); ++i) {
            MidiOutPort* port = *i;
            p->addAction(port->name());
            }
      return p;
      }

//---------------------------------------------------------
//   MidiOutPort
//---------------------------------------------------------

MidiOutPort::MidiOutPort()
   : MidiTrackBase(MIDI_OUT)
      {
      _instrument = genericMidiInstrument;
      for (int ch = 0; ch < MIDI_CHANNELS; ++ch)
            _channel[ch] = new MidiChannel(this, ch);
      alsaPort        = 0;
      _nextPlayEvent  = _playEvents.end();
      _sendSync       = false;
      addMidiController(_instrument, CTRL_MASTER_VOLUME);
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
      if (alsaPort)
            midiDriver->unregisterPort(alsaPort);
      for (int ch = 0; ch < MIDI_CHANNEL; ++ch) {
            delete _channel[ch];
            }
      }

//---------------------------------------------------------
//   setName
//---------------------------------------------------------

void MidiOutPort::setName(const QString& s)
      {
      Track::setName(s);
      if (alsaPort)
            midiDriver->setPortName(alsaPort, s);
      else
            alsaPort = midiDriver->registerInPort(s);
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
            else if (MidiTrackBase::readProperties(node))
                  printf("MusE:MidiOutPort: unknown tag %s\n", tag.toLatin1().data());
            }
      }

//---------------------------------------------------------
//   activate
//---------------------------------------------------------

void MidiOutPort::activate1()
      {
      if (alsaPort == 0) {
            _outRoutes.clear();
            return;
            }
      for (iRoute i = _outRoutes.begin(); i != _outRoutes.end();) {
            iRoute ni = i;
            ++ni;
            Route r = *i;
            if (r.type == Route::MIDIPORT) {
                  if (!midiDriver->connect(alsaPort, i->port))
                        _outRoutes.erase(i);
                  }
            i = ni;
            }
      }

//---------------------------------------------------------
//   deactivate
//---------------------------------------------------------

void MidiOutPort::deactivate()
      {
      for (ciRoute i = _outRoutes.begin(); i != _outRoutes.end(); ++i)
            midiDriver->disconnect(alsaPort, i->port);
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
                  midiDriver->putEvent(alsaPort, MidiEvent(0, chn, ME_PITCHBEND, b, 0));
                  return;
                  }
            if (a == CTRL_PROGRAM) {
                  // don't output program changes for GM drum channel
//                  if (!(song->mtype() == MT_GM && chn == 9)) {
                        int hb = (b >> 16) & 0xff;
                        int lb = (b >> 8) & 0xff;
                        int pr = b & 0x7f;
                        if (hb != 0xff)
                              midiDriver->putEvent(alsaPort, MidiEvent(0, chn, ME_CONTROLLER, CTRL_HBANK, hb));
                        if (lb != 0xff)
                              midiDriver->putEvent(alsaPort, MidiEvent(0, chn, ME_CONTROLLER, CTRL_LBANK, lb));
                        midiDriver->putEvent(alsaPort, MidiEvent(0, chn, ME_PROGRAM, pr, 0));
                        return;
//                        }
                  }
            if (a == CTRL_MASTER_VOLUME) {
                  unsigned char sysex[] = {
                        0x7f, 0x7f, 0x04, 0x01, 0x00, 0x00
                        };
                  sysex[4] = b & 0x7f;
                  sysex[5] = (b >> 7) & 0x7f;
                  MidiEvent e(ev.time(), ME_SYSEX, sysex, 6);
                  midiDriver->putEvent(alsaPort, e);
                  return;
                  }

#if 1 // if ALSA cannot handle RPN NRPN etc.
            if (a < 0x1000) {          // 7 Bit Controller
                  //putMidiEvent(MidiEvent(0, chn, ME_CONTROLLER, a, b));
                  midiDriver->putEvent(alsaPort, ev);
                  }
            else if (a < 0x20000) {     // 14 bit high resolution controller
                  int ctrlH = (a >> 8) & 0x7f;
                  int ctrlL = a & 0x7f;
                  int dataH = (b >> 7) & 0x7f;
                  int dataL = b & 0x7f;
                  midiDriver->putEvent(alsaPort, MidiEvent(0, chn, ME_CONTROLLER, ctrlH, dataH));
                  midiDriver->putEvent(alsaPort, MidiEvent(0, chn, ME_CONTROLLER, ctrlL, dataL));
                  }
            else if (a < 0x30000) {     // RPN 7-Bit Controller
                  int ctrlH = (a >> 8) & 0x7f;
                  int ctrlL = a & 0x7f;
                  midiDriver->putEvent(alsaPort, MidiEvent(0, chn, ME_CONTROLLER, CTRL_HRPN, ctrlH));
                  midiDriver->putEvent(alsaPort, MidiEvent(0, chn, ME_CONTROLLER, CTRL_LRPN, ctrlL));
                  midiDriver->putEvent(alsaPort, MidiEvent(0, chn, ME_CONTROLLER, CTRL_HDATA, b));
                  }
            else if (a < 0x40000) {     // NRPN 7-Bit Controller
                  int ctrlH = (a >> 8) & 0x7f;
                  int ctrlL = a & 0x7f;
                  midiDriver->putEvent(alsaPort, MidiEvent(0, chn, ME_CONTROLLER, CTRL_HNRPN, ctrlH));
                  midiDriver->putEvent(alsaPort, MidiEvent(0, chn, ME_CONTROLLER, CTRL_LNRPN, ctrlL));
                  midiDriver->putEvent(alsaPort, MidiEvent(0, chn, ME_CONTROLLER, CTRL_HDATA, b));
                  }
            else if (a < 0x60000) {     // RPN14 Controller
                  int ctrlH = (a >> 8) & 0x7f;
                  int ctrlL = a & 0x7f;
                  int dataH = (b >> 7) & 0x7f;
                  int dataL = b & 0x7f;
                  midiDriver->putEvent(alsaPort, MidiEvent(0, chn, ME_CONTROLLER, CTRL_HRPN, ctrlH));
                  midiDriver->putEvent(alsaPort, MidiEvent(0, chn, ME_CONTROLLER, CTRL_LRPN, ctrlL));
                  midiDriver->putEvent(alsaPort, MidiEvent(0, chn, ME_CONTROLLER, CTRL_HDATA, dataH));
                  midiDriver->putEvent(alsaPort, MidiEvent(0, chn, ME_CONTROLLER, CTRL_LDATA, dataL));
                  }
            else if (a < 0x70000) {     // NRPN14 Controller
                  int ctrlH = (a >> 8) & 0x7f;
                  int ctrlL = a & 0x7f;
                  int dataH = (b >> 7) & 0x7f;
                  int dataL = b & 0x7f;
                  midiDriver->putEvent(alsaPort, MidiEvent(0, chn, ME_CONTROLLER, CTRL_HNRPN, ctrlH));
                  midiDriver->putEvent(alsaPort, MidiEvent(0, chn, ME_CONTROLLER, CTRL_LNRPN, ctrlL));
                  midiDriver->putEvent(alsaPort, MidiEvent(0, chn, ME_CONTROLLER, CTRL_HDATA, dataH));
                  midiDriver->putEvent(alsaPort, MidiEvent(0, chn, ME_CONTROLLER, CTRL_LDATA, dataL));
                  }
            else {
                  printf("putEvent: unknown controller type 0x%x\n", a);
                  }
#endif
            }
      midiDriver->putEvent(alsaPort, ev);
      }

//---------------------------------------------------------
//   playEventList
//---------------------------------------------------------

void MidiOutPort::playEventList()
      {
      for (; _nextPlayEvent != _playEvents.end(); ++_nextPlayEvent)
            midiDriver->putEvent(alsaPort, *_nextPlayEvent);
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
	      	if (((SynthI*)i->track)->eventFifo()->put(*ev))
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

//---------------------------------------------------------
//   MidiInPort
//---------------------------------------------------------

MidiInPort::MidiInPort()
   : MidiTrackBase(MIDI_IN)
      {
      alsaPort = 0;
      }

//---------------------------------------------------------
//   MidiInPort
//---------------------------------------------------------

MidiInPort::~MidiInPort()
      {
      if (alsaPort)
            midiDriver->unregisterPort(alsaPort);
      }

//---------------------------------------------------------
//   setName
//---------------------------------------------------------

void MidiInPort::setName(const QString& s)
      {
      Track::setName(s);
      if (alsaPort)
            midiDriver->setPortName(alsaPort, s);
      else
            alsaPort = midiDriver->registerOutPort(s);
      }

//---------------------------------------------------------
//   MidiInPort::write
//---------------------------------------------------------

void MidiInPort::write(Xml& xml) const
      {
      xml.tag("MidiInPort");
      MidiTrackBase::writeProperties(xml);
      xml.etag("MidiInPort");
      }

//---------------------------------------------------------
//   MidiInPort::read
//---------------------------------------------------------

void MidiInPort::read(QDomNode node)
      {
      while (!node.isNull()) {
            QDomElement e = node.toElement();
            QString tag(e.tagName());
            if (MidiTrackBase::readProperties(node))
                  printf("MusE:MidiInPort: unknown tag %s\n", tag.toLatin1().data());
            node = node.nextSibling();
            }
      }

//---------------------------------------------------------
//   activate
//---------------------------------------------------------

void MidiInPort::activate1()
      {
      if (alsaPort == 0) {
            _inRoutes.clear();
            return;
            }
      for (iRoute i = _inRoutes.begin(); i != _inRoutes.end();) {
            iRoute ni = i;
            ++ni;
            Route r = *i;
            if (!midiDriver->connect(i->port, alsaPort))
                  _inRoutes.erase(i);
            i = ni;
            }
      }

//---------------------------------------------------------
//   deactivate
//---------------------------------------------------------

void MidiInPort::deactivate()
      {
      for (ciRoute i = _inRoutes.begin(); i != _inRoutes.end(); ++i)
            midiDriver->disconnect(i->port, alsaPort);
      }

//---------------------------------------------------------
//   midiReceived
//---------------------------------------------------------

void MidiInPort::eventReceived(snd_seq_event_t* ev)
      {
      MidiEvent event;
      event.setB(0);
      event.setTime(audio->timestamp());

      switch(ev->type) {
            case SND_SEQ_EVENT_NOTEON:
                  event.setChannel(ev->data.note.channel);
                  event.setType(ME_NOTEON);
                  event.setA(ev->data.note.note);
                  event.setB(ev->data.note.velocity);
                  break;

            case SND_SEQ_EVENT_KEYPRESS:
                  event.setChannel(ev->data.note.channel);
                  event.setType(ME_POLYAFTER);
                  event.setA(ev->data.note.note);
                  event.setB(ev->data.note.velocity);
                  break;

            case SND_SEQ_EVENT_SYSEX:
                  event.setTime(0);      // mark as used
                  event.setType(ME_SYSEX);
                  event.setData((unsigned char*)(ev->data.ext.ptr)+1,
                     ev->data.ext.len-2);
                  break;

            case SND_SEQ_EVENT_NOTEOFF:
                  event.setChannel(ev->data.note.channel);
                  event.setType(ME_NOTEOFF);
                  event.setA(ev->data.note.note);
                  event.setB(ev->data.note.velocity);
                  break;

            case SND_SEQ_EVENT_CHANPRESS:
                  event.setChannel(ev->data.control.channel);
                  event.setType(ME_AFTERTOUCH);
                  event.setA(ev->data.control.value);
                  break;

            case SND_SEQ_EVENT_PGMCHANGE:
                  event.setChannel(ev->data.control.channel);
                  event.setType(ME_PROGRAM);
                  event.setA(ev->data.control.value);
                  break;

            case SND_SEQ_EVENT_PITCHBEND:
                  event.setChannel(ev->data.control.channel);
                  event.setType(ME_PITCHBEND);
                  event.setA(ev->data.control.value);
                  break;

            case SND_SEQ_EVENT_CONTROLLER:
                  event.setChannel(ev->data.control.channel);
                  event.setType(ME_CONTROLLER);
                  event.setA(ev->data.control.param);
                  event.setB(ev->data.control.value);
                  break;
            }

      if (midiInputTrace) {
            printf("MidiInput<%s>: ", name().toLatin1().data());
            event.dump();
            }
      //
      // process midi filter pipeline and add event to
      // _recordEvents
      //

      MPEventList il, ol;
      il.insert(event);
      pipeline()->apply(0, 0, &il, &ol);

      //
      // update midi meter
      // notify gui of new events
      //
      for (iMPEvent i = ol.begin(); i != ol.end(); ++i) {
            if (i->type() == ME_NOTEON)
                  addMidiMeter(i->dataB());
            song->putEvent(*i);
            _recordEvents.add(*i);
            }
      }

//---------------------------------------------------------
//   afterProcess
//    clear all recorded events after a process cycle
//---------------------------------------------------------

void MidiInPort::afterProcess()
      {
      _recordEvents.clear();
      }

//---------------------------------------------------------
//   getEvents
//---------------------------------------------------------

void MidiInPort::getEvents(unsigned, unsigned, int ch, MPEventList* dst)
      {
      for (iMPEvent i = _recordEvents.begin(); i != _recordEvents.end(); ++i) {
            if (i->channel() == ch || ch == -1)
                  dst->insert(*i);
            }
      }

//---------------------------------------------------------
//   MidiChannel
//---------------------------------------------------------

MidiChannel::MidiChannel(MidiOutPort* p, int ch)
   : MidiTrackBase(MIDI_CHANNEL)
      {
      _port       = p;
      _channelNo  = ch;
      _drumMap    = 0;
      _useDrumMap = false;
      initMidiController();

      //
      // create minimum set of managed controllers
      // to make midi mixer operational
      //
      MidiInstrument* mi = port()->instrument();
      addMidiController(mi, CTRL_PROGRAM);
      addMidiController(mi, CTRL_VOLUME);
      addMidiController(mi, CTRL_PANPOT);
      addMidiController(mi, CTRL_REVERB_SEND);
      addMidiController(mi, CTRL_CHORUS_SEND);
      addMidiController(mi, CTRL_VARIATION_SEND);
      }

//---------------------------------------------------------
//   MidiChannel
//---------------------------------------------------------

MidiChannel::~MidiChannel()
      {
      }

//---------------------------------------------------------
//   MidiChannel::write
//---------------------------------------------------------

void MidiChannel::write(Xml& xml) const
      {
      xml.tag("MidiChannel idx=\"%d\"", _channelNo);
      MidiTrackBase::writeProperties(xml);
      xml.intTag("useDrumMap", _useDrumMap);
      xml.etag("MidiChannel");
      }

//---------------------------------------------------------
//   MidiChannel::read
//---------------------------------------------------------

void MidiChannel::read(QDomNode node)
      {
      QString drumMapName;
      while (!node.isNull()) {
            QDomElement e = node.toElement();
            QString tag(e.tagName());
            if (tag == "useDrumMap")
                  _useDrumMap = e.text().toInt();
            else if (MidiTrackBase::readProperties(node))
                  printf("MusE:MidiChannel: unknown tag %s\n", tag.toLatin1().data());
            node = node.nextSibling();
            }
      MidiOutPort* op = port();
      if (op) {
            MidiInstrument* mi = op->instrument();
            int val = ctrlVal(CTRL_PROGRAM).i;
            _drumMap = mi->getDrumMap(val);
            }
      }

//---------------------------------------------------------
//   playMidiEvent
//---------------------------------------------------------

void MidiChannel::playMidiEvent(MidiEvent* ev)
      {
      if (ev->type() == ME_NOTEON) {
            _meter[0] += ev->dataB()/2;
            if (_meter[0] > 127.0f)
                  _meter[0] = 127.0f;
            }
      ev->setChannel(_channelNo);
      _port->playMidiEvent(ev);
      }

//---------------------------------------------------------
//   setUseDrumMap
//---------------------------------------------------------

void MidiChannel::setUseDrumMap(bool val)
      {
      if (_useDrumMap != val) {
            _useDrumMap = val;
            if (_useDrumMap) {
                  int val = ctrlVal(CTRL_PROGRAM).i;
                  MidiOutPort* op = port();
                  MidiInstrument* mi = op->instrument();
                  DrumMap* dm = mi->getDrumMap(val);
                  if (dm == 0)
                        dm = &gmDrumMap;
                  _drumMap = dm;
                  }
            else
                  _drumMap = &noDrumMap;
      	for (iRoute i = _inRoutes.begin(); i != _inRoutes.end(); ++i) {
                  MidiTrack* mt = (MidiTrack*)(i->track);
                  mt->changeDrumMap();
                  }
            emit useDrumMapChanged(_useDrumMap);
            }
      }

//---------------------------------------------------------
//   emitControllerChanged
//---------------------------------------------------------

void MidiChannel::emitControllerChanged(int id)
      {
      if (id == CTRL_PROGRAM && _useDrumMap) {
            int val = ctrlVal(id).i;
            MidiOutPort* op = port();
            MidiInstrument* mi = op->instrument();
            DrumMap* dm = mi->getDrumMap(val);
            if (dm == 0)
                  dm = &gmDrumMap;
            if (dm != _drumMap) {
                  _drumMap = dm;
      		for (iRoute i = _inRoutes.begin(); i != _inRoutes.end(); ++i) {
                  	MidiTrack* mt = (MidiTrack*)(i->track);
	                  mt->changeDrumMap();
      	            }
                  }
            }
      emit controllerChanged(id);
      }

//---------------------------------------------------------
//   isMute
//---------------------------------------------------------

bool MidiChannel::isMute() const
      {
      if (_solo)
            return false;
      if (song->solo())
            return true;
      return _mute;
      }

