//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: mpevent.cpp,v 1.6.2.2 2009/11/25 09:09:43 terminator356 Exp $
//
//  (C) Copyright 2002-2004 Werner Schweer (ws@seh.de)
//  (C) Copyright 2012 Tim E. Real (terminator356 on users dot sourceforge dot net)
//
//  This program is free software; you can redistribute it and/or
//  modify it under the terms of the GNU General Public License
//  as published by the Free Software Foundation; version 2 of
//  the License, or (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
//=========================================================

#include <stdio.h>

#include "mpevent.h"

#include "helper.h"
#include "event.h"
#include "midictrl.h"
#include "midiport.h"
#include "muse/midi.h"

namespace MusECore {

//---------------------------------------------------------
//   MEvent
//---------------------------------------------------------

MEvent::MEvent(unsigned t, int port, int tpe, const unsigned char* data, int len)
      {
      _time = t;
      _port = port;
      edata.setData(data, len);
      _type = tpe;
      _loopNum = 0;
      setChannel(0);
      }

MEvent::MEvent(unsigned tick, int port, int channel, const Event& e)
      {
      setChannel(channel);
      setTime(tick);
      setPort(port);
      setLoopNum(0);
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
            case Sysex:
                  setType(ME_SYSEX);
                  setData(e.eventData());
                  break;
            default:
                  fprintf(stderr, "MEvent::MEvent(): event type %d not implemented\n",
                     type());
                  break;
            }
      }

//---------------------------------------------------------
//   dump
//---------------------------------------------------------

void MEvent::dump() const
      {
      fprintf(stderr, "time:%d port:%d chan:%d ", _time, _port, _channel+1);
      if (_type == ME_NOTEON) {   
            QString s = pitch2string(_a);
            fprintf(stderr, "NoteOn %s(0x%x) %d\n", s.toLatin1().constData(), _a, _b);
           }
      else if (_type == ME_NOTEOFF) {  
            QString s = pitch2string(_a);
            fprintf(stderr, "NoteOff %s(0x%x) %d\n", s.toLatin1().constData(), _a, _b);
           }
      else if (_type == ME_SYSEX) {
            fprintf(stderr, "SysEx len %d 0x%0x ...\n", len(), data()[0]);
            }
      else
            fprintf(stderr, "type:0x%02x a=%d b=%d\n", _type, _a, _b);
      }

//---------------------------------------------------------
//   sortingWeight
//---------------------------------------------------------

int MEvent::sortingWeight() const
{
  // Sorting weight initially worked out by Tim E. Real
  // Sorted here by most popular for quickest reponse.
  
  switch(_type)
  {
    case ME_NOTEON:
      if(_b == 0)  // Is it really a note off?
        return 7;  
      return 98;  
    case ME_NOTEOFF:
      return 7;
      
    case ME_PITCHBEND:
      return 25;  
    case ME_CONTROLLER:
      switch(_a)
      {
        case CTRL_PROGRAM:
          return 21;  
        default:
          return 24;
      }
    case ME_PROGRAM:
      return 20;  
      
    case ME_CLOCK:
      return 0;  
    case ME_MTC_QUARTER:
      return 1;  
    case ME_TICK:
      return 2;  
    case ME_SENSE:
      return 3;  

    case ME_SYSEX_END:
      return 4;  
    case ME_AFTERTOUCH:
      return 5;  
    case ME_POLYAFTER:
      return 6;  
    case ME_STOP:
      return 8;  

    case ME_SONGSEL:
      return 9;  
    case ME_SYSEX:
      return 18;  
    case ME_META:
      switch(_a)
      {
        case ME_META_TEXT_2_COPYRIGHT:
          return 10;
        case ME_META_TEXT_1_COMMENT:
          return 11; 
        case ME_META_PORT_CHANGE:
          return 12; 
        case ME_META_TEXT_9_DEVICE_NAME:
          return 13; 
        case ME_META_CHANNEL_CHANGE:
          return 14;
          
        case ME_META_TEXT_3_TRACK_NAME:
          return 15; 
        case ME_META_TEXT_F_TRACK_COMMENT:  
          return 16; 
        case ME_META_TEXT_0_SEQUENCE_NUMBER:
          return 17; 

        case ME_META_TEXT_4_INSTRUMENT_NAME:
          return 19; 
        case ME_META_END_OF_TRACK:
          return 99; 
        default:  
          return 97;
      }

    case ME_TUNE_REQ:
      return 22;  
    case ME_SONGPOS:
      return 23;  

    case ME_START:
      return 26;  
    case ME_CONTINUE:
      return 27;  
  }
  
  fprintf(stderr, "FIXME: MEvent::sortingWeight: unknown event type:%d\n", _type);
  return 100;
}
      
//---------------------------------------------------------
//   operator <
//---------------------------------------------------------

bool MEvent::operator<(const MEvent& e) const
      {
      if (time() != e.time())
            return time() < e.time();
      if (port() != e.port())
            return port() < e.port();

      // play note off events first to prevent overlapping
      // notes

      if (channel() == e.channel())
        return sortingWeight() < e.sortingWeight();

      int map[16] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 0, 10, 11, 12, 13, 14, 15 };
      return map[channel()] < map[e.channel()];
      }

//---------------------------------------------------------
//   put
//    return true on fifo overflow
//---------------------------------------------------------

bool MidiFifo::put(const MidiPlayEvent& event)
      {
      if (size < MIDI_FIFO_SIZE) {
            fifo[wIndex] = event;
            wIndex = (wIndex + 1) % MIDI_FIFO_SIZE;
            // q_atomic_increment(&size);
            ++size;
            return false;
            }
      return true;
      }

//---------------------------------------------------------
//   get
//---------------------------------------------------------

MidiPlayEvent MidiFifo::get()
      {
      MidiPlayEvent event(fifo[rIndex]);
      rIndex = (rIndex + 1) % MIDI_FIFO_SIZE;
      --size;
      return event;
      }

//---------------------------------------------------------
//   peek
//---------------------------------------------------------

const MidiPlayEvent& MidiFifo::peek(int n)
      {
      int idx = (rIndex + n) % MIDI_FIFO_SIZE;
      return fifo[idx];
      }

//---------------------------------------------------------
//   remove
//---------------------------------------------------------

void MidiFifo::remove()
      {
      rIndex = (rIndex + 1) % MIDI_FIFO_SIZE;
      --size;
      }


//---------------------------------------------------------
//   put
//    return true on fifo overflow
//---------------------------------------------------------

bool MidiRecFifo::put(const MidiRecordEvent& event)
      {
      if (size < MIDI_REC_FIFO_SIZE) {
            fifo[wIndex] = event;
            wIndex = (wIndex + 1) % MIDI_REC_FIFO_SIZE;
            ++size;
            return false;
            }
      return true;
      }

//---------------------------------------------------------
//   get
//---------------------------------------------------------

MidiRecordEvent MidiRecFifo::get()
      {
      MidiRecordEvent event(fifo[rIndex]);
      rIndex = (rIndex + 1) % MIDI_REC_FIFO_SIZE;
      --size;
      return event;
      }

//---------------------------------------------------------
//   peek
//---------------------------------------------------------

const MidiRecordEvent& MidiRecFifo::peek(int n)
      {
      int idx = (rIndex + n) % MIDI_REC_FIFO_SIZE;
      return fifo[idx];
      }

//---------------------------------------------------------
//   remove
//---------------------------------------------------------

void MidiRecFifo::remove()
      {
      rIndex = (rIndex + 1) % MIDI_REC_FIFO_SIZE;
      --size;
      }

} // namespace MusECore
