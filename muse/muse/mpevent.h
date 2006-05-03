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

#ifndef __MPEVENT_H__
#define __MPEVENT_H__

#include <set>
#include "evdata.h"
#include <ext/mt_allocator.h>

#include "midi.h"

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

      ~MidiEvent() {}

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
      bool isNote() const      { return _type == ME_NOTEON; }
      bool isNoteOff() const   { return (_type == ME_NOTEOFF)||(_type == ME_NOTEON && _b == 0); }
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

