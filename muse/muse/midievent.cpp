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

#include "midievent.h"

#include "helper.h"
#include "event.h"
#include "midictrl.h"
#include "muse/midi.h"

//---------------------------------------------------------
//   MidiEvent
//---------------------------------------------------------

MidiEvent::MidiEvent(unsigned t, int tpe, const unsigned char* data, int len)
      {
      _time = t;
      edata.setData(data, len);
      _type = tpe;
      }

MidiEvent::MidiEvent(unsigned tick, int channel, const Event& e)
      {
      setChannel(channel);
      setTime(tick);
      switch(e.type()) {
            case Note:
                  setType(ME_NOTEON);
                  setA(e.dataA());
                  setB(e.dataB());
                  break;
            case Controller:
                  setType(ME_CONTROLLER);
                  setA(e.dataA());  // controller number
                  setB(e.dataB());  // controller value
                  break;
            case PAfter:
                  setType(ME_POLYAFTER);
                  setA(e.dataA());
                  setB(e.dataB());
                  break;
            case CAfter:
                  setType(ME_AFTERTOUCH);
                  setA(e.dataA());
                  setB(0);
                  break;
            case Sysex:
                  setType(ME_SYSEX);
                  setData(e.eventData());
                  break;
            default:
                  printf("MEvent::MEvent(): event type %d not implemented\n",
                     type());
                  break;
            }
      }

//---------------------------------------------------------
//   dump
//---------------------------------------------------------

void MidiEvent::dump() const
      {
      printf("chan:%d ", _channel+1);
      if (_type == 0x90) {   // NoteOn
            QString s = pitch2string(_a);
            printf("NoteOn %3s(0x%02x) %3d\n", s.toLatin1().data(), _a, _b);
           }
      else if (_type == 0xf0) {
            printf("SysEx  len %d ", len());
            int n = len() < 7 ? len() : 7;
            unsigned char* p = data();
            for (int i = 0; i < n; ++i)
                  printf("%02x ", *p++);
            printf("\n");
            }
      else if (_type == 0xb0)
            printf("Ctrl   %d(0x%02x) %d(0x%02x)\n", _a, _a, _b, _b);
      else if (_type == 0xc0)
            printf("Prog   %d(0x%02x)\n", _a, _a);
      else if (_type == 0xd0)
            printf("Aftertouch %d\n", _a);
      else
            printf("type:0x%02x a=%d(0x%02x) b=%d(0x%02x)\n", _type, _a, _a, _b, _b);
      }

//---------------------------------------------------------
//   operator <
//---------------------------------------------------------

bool MidiEvent::operator<(const MidiEvent& e) const
      {
      if (time() != e.time())
            return time() < e.time();

      // play note off events first to prevent overlapping
      // notes

      if (channel() == e.channel())
            return type() == ME_NOTEOFF
               || (type() == ME_NOTEON && dataB() == 0);

      int map[16] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 0, 10, 11, 12, 13, 14, 15 };
      return map[channel()] < map[e.channel()];
      }
