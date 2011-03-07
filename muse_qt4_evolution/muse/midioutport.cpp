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
#include "audiodev.h"
#include "audio.h"
#include "midioutport.h"
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
      setDeviceId(127);        // all
      addMidiController(_instrument, CTRL_MASTER_VOLUME);
      _channels = 1;
      }

//---------------------------------------------------------
//   MidiOutPort
//---------------------------------------------------------

MidiOutPort::~MidiOutPort()
      {
      }

//---------------------------------------------------------
//   setName
//---------------------------------------------------------

void MidiOutPort::setName(const QString& s)
      {
      Track::setName(s);
      if (!jackPort().isZero())
            audioDriver->setPortName(jackPort(), s);
      }

//---------------------------------------------------------
//   MidiOutPort::write
//---------------------------------------------------------

void MidiOutPort::write(Xml& xml) const
      {
      xml.stag("MidiOutPort");
      MidiTrackBase::writeProperties(xml);
      if (_instrument)
            xml.tag("instrument", _instrument->iname());
      xml.tag("sendSync", sendSync());
      xml.tag("deviceId", deviceId());
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
            if (tag == "instrument") {
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
      for (iRoute r = _outRoutes.begin(); r != _outRoutes.end(); ++r) {
            if (r->dst.type == RouteNode::JACKMIDIPORT)
                  queueJackEvent(event);
            else
                  fprintf(stderr, "MidiOutPort::process(): invalid routetype\n");
            }
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

            if (a == CTRL_PITCH) {
                  int v = b + 8192;
                  audioDriver->putEvent(jackPort(0), MidiEvent(t, chn, ME_PITCHBEND, v & 0x7f, (v >> 7) & 0x7f));
                  }
            else if (a == CTRL_PROGRAM) {
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
//   setInstrument
//---------------------------------------------------------

void MidiOutPort::setInstrument(MidiInstrument* i)
      {
      _instrument = i;
      emit instrumentChanged();
      }

//-------------------------------------------------------------------
//   process
//    Collect all midi events for the current process cycle and put
//    into _schedEvents queue. For note on events create the proper
//    note off events. The note off events maybe played after the
//    current process cycle.
//    From _schedEvents queue copy all events for the current cycle
//    to all output routes.
//-------------------------------------------------------------------

void MidiOutPort::processMidi(const SeqTime* t)
      {
      if (track->mute())
            return;

      MidiEventList el;
      MidiOut::processMidi(el, t);

      pipeline()->apply(t->curTickPos, t->nextTickPos, &el, &_schedEvents);

      //
      // route events to destination
      //

      int portVelo      = 0;
      unsigned endFrame = t->lastFrameTime + segmentSize;
      iMidiEvent i      = _schedEvents.begin();

      for (; i != _schedEvents.end(); ++i) {
            if (i->time() >= endFrame)
                  break;
            routeEvent(*i);
            if (i->type() == ME_NOTEON)
                  portVelo += i->dataB();
            }
      _schedEvents.erase(_schedEvents.begin(), i);
      addMidiMeter(portVelo);
      }

