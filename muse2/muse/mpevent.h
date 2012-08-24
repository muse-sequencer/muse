//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: mpevent.h,v 1.8.2.5 2009/11/25 09:09:43 terminator356 Exp $
//
//  (C) Copyright 1999-2002 Werner Schweer (ws@seh.de)
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
//
//=========================================================

#ifndef __MPEVENT_H__
#define __MPEVENT_H__

#include <set>
#include <list>
#include "evdata.h"
#include "memory.h"

// Play events ring buffer size
#define MIDI_FIFO_SIZE    4096         

// Record events ring buffer size
#define MIDI_REC_FIFO_SIZE  256

namespace MusECore {

class Event;
class EvData;

//---------------------------------------------------------
//   MEvent
//    baseclass for MidiPlayEvent and MidiRecordEvent
//---------------------------------------------------------

//---------------------------------------------------------
//   MEvent
//---------------------------------------------------------

class MEvent {
      unsigned _time;
      EvData edata;
      unsigned char _port, _channel, _type;
      int _a, _b;
      int _loopNum; // The loop count when the note was recorded.

   public:
      MEvent() { _loopNum = 0; }
      MEvent(unsigned tm, int p, int c, int t, int a, int b)
        : _time(tm), _port(p), _channel(c & 0xf), _type(t), _a(a), _b(b) { _loopNum = 0; }
      MEvent(unsigned t, int p, int type, const unsigned char* data, int len);
      MEvent(unsigned t, int p, int tpe, EvData d) : _time(t), edata(d), _port(p), _type(tpe) { _loopNum = 0; }
      MEvent(unsigned t, int port, int channel, const Event& e);

      ~MEvent()         {}

      MEvent& operator=(const MEvent& ed) {
            _time    = ed._time;
            edata    = ed.edata;
            _port    = ed._port;
            _channel = ed._channel;
            _type    = ed._type;
            _a       = ed._a;
            _b       = ed._b;
            _loopNum = ed._loopNum;
            return *this;
            }

      int port()    const      { return _port;    }
      int channel() const      { return _channel; }
      int type()    const      { return _type;    }
      int dataA()   const      { return _a;       }
      int dataB()   const      { return _b;       }
      unsigned time() const    { return _time;    }
      int loopNum() const      { return _loopNum; }

      void setPort(int val)    { _port = val;     }
      void setChannel(int val) { _channel = val;  }
      void setType(int val)    { _type = val;     }
      void setA(int val)       { _a = val;        }
      void setB(int val)       { _b = val;        }
      void setTime(unsigned val) { _time = val;   }
      void setLoopNum(int n)   { _loopNum = n;    }

      const EvData& eventData() const { return edata; }
      unsigned char* data() const     { return edata.data; }
      int len() const                 { return edata.dataLen; }
      void setData(const EvData& e)   { edata = e; }
      void setData(const unsigned char* p, int len) { edata.setData(p, len); }
      void dump() const;
      bool isNote() const      { return _type == 0x90; }
      bool isNoteOff() const   { return (_type == 0x80)||(_type == 0x90 && _b == 0); }
      bool operator<(const MEvent&) const;
      };

//---------------------------------------------------------
//   MidiRecordEvent
//    allocated and deleted in midiseq thread context
//---------------------------------------------------------

class MidiRecordEvent : public MEvent {
   private:
      unsigned int _tick; // To store tick when external sync is on, required besides frame.
   public:
      MidiRecordEvent() : MEvent() {}
      MidiRecordEvent(const MEvent& e) : MEvent(e) {}
      MidiRecordEvent(unsigned tm, int p, int c, int t, int a, int b)
        : MEvent(tm, p, c, t, a, b) {}
      MidiRecordEvent(unsigned t, int p, int tpe, const unsigned char* data, int len)
        : MEvent(t, p, tpe, data, len) {}
      MidiRecordEvent(unsigned t, int p, int type, EvData data)
        : MEvent(t, p, type, data) {}
      ~MidiRecordEvent() {}
      
      unsigned int tick() {return _tick;}
      void setTick(unsigned int tick) {_tick = tick;}
      };

//---------------------------------------------------------
//   MidiPlayEvent
//    allocated and deleted in audio thread context
//---------------------------------------------------------

class MidiPlayEvent : public MEvent {
   public:
      MidiPlayEvent() : MEvent() {}
      MidiPlayEvent(const MEvent& e) : MEvent(e) {}
      MidiPlayEvent(unsigned tm, int p, int c, int t, int a, int b)
        : MEvent(tm, p, c, t, a, b) {}
      MidiPlayEvent(unsigned t, int p, int type, const unsigned char* data, int len)
        : MEvent(t, p, type, data, len) {}
      MidiPlayEvent(unsigned t, int p, int type, EvData data)
        : MEvent(t, p, type, data) {}
      MidiPlayEvent(unsigned t, int port, int channel, const Event& e)
        : MEvent(t, port, channel, e) {}
      ~MidiPlayEvent() {}
      };

//---------------------------------------------------------
//   MPEventList
//    memory allocation in audio thread domain
//---------------------------------------------------------

typedef std::multiset<MidiPlayEvent, std::less<MidiPlayEvent>, audioRTalloc<MidiPlayEvent> > MPEL;

struct MPEventList : public MPEL {
      void add(const MidiPlayEvent& ev) { MPEL::insert(ev); }
};

typedef MPEventList::iterator iMPEvent;
typedef MPEventList::const_iterator ciMPEvent;

/* DELETETHIS 20 ??
//---------------------------------------------------------
//   MREventList
//    memory allocation in midi thread domain
//---------------------------------------------------------

// Changed by Tim. p3.3.8

// audioRTalloc? Surely this must have been a mistake?  
//typedef std::list<MidiRecordEvent, audioRTalloc<MidiRecordEvent> > MREL;
typedef std::list<MidiRecordEvent, midiRTalloc<MidiRecordEvent> > MREL;

struct MREventList : public MREL {
      void add(const MidiRecordEvent& ev) { MREL::push_back(ev); }
      };

typedef MREventList::iterator iMREvent;
typedef MREventList::const_iterator ciMREvent;
*/

//---------------------------------------------------------
//   MidiFifo
//---------------------------------------------------------

class MidiFifo {
      MidiPlayEvent fifo[MIDI_FIFO_SIZE];
      volatile int size;
      int wIndex;
      int rIndex;

   public:
      MidiFifo()  { clear(); }
      bool put(const MidiPlayEvent& event);   // returns true on fifo overflow
      MidiPlayEvent get();
      const MidiPlayEvent& peek(int = 0);
      void remove();
      bool isEmpty() const { return size == 0; }
      void clear()         { size = 0, wIndex = 0, rIndex = 0; }
      int getSize() const  { return size; }
      };

//---------------------------------------------------------
//   MidiRecFifo
//---------------------------------------------------------

class MidiRecFifo {
      MidiRecordEvent fifo[MIDI_REC_FIFO_SIZE];
      volatile int size;
      int wIndex;
      int rIndex;

   public:
      MidiRecFifo()  { clear(); }
      bool put(const MidiRecordEvent& event);   // returns true on fifo overflow
      MidiRecordEvent get();
      const MidiRecordEvent& peek(int = 0);
      void remove();
      bool isEmpty() const { return size == 0; }
      void clear()         { size = 0, wIndex = 0, rIndex = 0; }
      int getSize() const  { return size; }
      };

} // namespace MusECore

#endif

