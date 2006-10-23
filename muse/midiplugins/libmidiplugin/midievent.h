//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: mpevent.h,v 1.3 2005/06/06 14:24:53 wschweer Exp $
//
//  (C) Copyright 1999-2002 Werner Schweer (ws@seh.de)
//=========================================================

#ifndef __MIDIEVENT_H__
#define __MIDIEVENT_H__

#include <set>
#include "evdata.h"
#include <ext/mt_allocator.h>

class Event;
class EvData;

//---------------------------------------------------------
//   MidiEvent
//---------------------------------------------------------

class MidiEvent {
      unsigned _time;
      EvData edata;
      unsigned char _channel, _type;
      int _a, _b;

   public:
      MidiEvent() {}
      MidiEvent(unsigned tm, int c, int t, int a, int b)
        : _time(tm), _channel(c & 0xf), _type(t), _a(a), _b(b) {}
      MidiEvent(unsigned t, int type, const unsigned char* data, int len);
      MidiEvent(unsigned t, int tpe, EvData d) : _time(t), edata(d), _type(tpe) {}
      MidiEvent(unsigned t, int channel, const Event& e);

      ~MidiEvent()         {}

      MidiEvent& operator=(const MidiEvent& ed) {
            _time    = ed._time;
            edata    = ed.edata;
            _channel = ed._channel;
            _type    = ed._type;
            _a       = ed._a;
            _b       = ed._b;
            return *this;
            }

      int channel() const      { return _channel; }
      int type()    const      { return _type;    }
      int dataA()   const      { return _a;       }
      int dataB()   const      { return _b;       }
      unsigned time() const    { return _time;    }

      void setChannel(int val) { _channel = val;  }
      void setType(int val)    { _type = val;     }
      void setA(int val)       { _a = val;        }
      void setB(int val)       { _b = val;        }
      void setTime(unsigned val) { _time = val;     }

      const EvData& eventData() const { return edata; }
      unsigned char* data() const     { return edata.data; }
      int len() const                 { return edata.dataLen; }
      void setData(const EvData& e)   { edata = e; }
      void setData(const unsigned char* p, int len) { edata.setData(p, len); }
      void dump() const;
      bool isNote() const      { return _type == 0x90; }
      bool isNoteOff() const   { return (_type == 0x80)||(_type == 0x90 && _b == 0); }
      bool operator<(const MidiEvent&) const;
      };

//---------------------------------------------------------
//   MPEventList
//---------------------------------------------------------

// typedef std::multiset<MidiEvent, std::less<MidiEvent>,
//   __gnu_cxx::__mt_alloc<MidiEvent> > MPEL;

typedef std::multiset<MidiEvent, std::less<MidiEvent> > MPEL;

struct MPEventList : public MPEL {
      void add(const MidiEvent& ev) { MPEL::insert(ev); }
      };

typedef MPEventList::iterator iMPEvent;
typedef MPEventList::const_iterator ciMPEvent;

//---------------------------------------------------------
//   MREventList
//---------------------------------------------------------

// typedef std::list<MidiEvent, __gnu_cxx::__mt_alloc<MidiEvent> > MREL;

typedef std::list<MidiEvent> MREL;

struct MREventList : public MREL {
      void add(const MidiEvent& ev) { MREL::push_back(ev); }
      };

typedef MREventList::iterator iMREvent;
typedef MREventList::const_iterator ciMREvent;

#endif

