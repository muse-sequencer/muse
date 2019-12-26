//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: midievent.h,v 1.7.2.1 2009/05/24 21:43:44 terminator356 Exp $
//
//  (C) Copyright 1999-2004 Werner Schweer (ws@seh.de)
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

#ifndef __MIDI_EVENT_H__
#define __MIDI_EVENT_H__

#include "eventbase.h"

namespace MusECore {

//---------------------------------------------------------
//   MidiEventBase
//---------------------------------------------------------

class MidiEventBase : public EventBase {
      int a, b, c;                  // pitch, velo-on, velo-off
      // Sysex event data.
      // NOTE: Even non-shared clone events ALWAYS share edata. edata does NOT currently require
      //        separate instances, unlike wave events which absolutely do.
      //       Be aware when iterating or modifying clones for example. (It can save time.)
      EvData edata;

      // Creates a non-shared clone (copies event base), including the same 'group' id.
      virtual EventBase* clone() const { return new MidiEventBase(*this); } 
      // Creates a copy of the event base, excluding the 'group' _id.
      virtual EventBase* duplicate() const { return new MidiEventBase(*this, true); }  

   public:
      MidiEventBase(EventType t);
      // Creates a non-shared clone with same id, or duplicate with unique id, and 0 ref count and invalid Pos sn. 
      MidiEventBase(const MidiEventBase& ev, bool duplicate_not_clone = false);
      virtual ~MidiEventBase() {}
      
      virtual void assign(const EventBase& ev); // Assigns to this event, excluding the _id. 
      
      virtual bool isSimilarTo(const EventBase& other) const;

      virtual bool isNote() const                   { return type() == Note; }
      virtual bool isNoteOff() const;
      virtual int pitch() const                     { return a;      }
      virtual int program() const                   { return a;      }
      virtual int cntrl() const                     { return a;      }
      virtual int dataA() const                     { return a;      }
      virtual void setA(int val)                    { a = val;       }
      virtual void setPitch(int v)                  { a = v;         }

      virtual int cntrlVal() const                  { return b;      }
      virtual int dataB() const                     { return b;      }
      virtual int velo() const                      { return b;      }
      virtual void setB(int val)                    { b = val;       }
      virtual void setVelo(int v)                   { b = v;         }

      virtual int veloOff() const                   { return c;      }
      virtual int dataC() const                     { return c;      }
      virtual void setC(int val)                    { c = val;       }
      virtual void setVeloOff(int v)                { c = v;         }

      virtual const unsigned char* constData() const { return edata.constData();  }
      virtual int dataLen() const                   { return edata.dataLen(); }
      virtual void setData(const unsigned char* data, int len) { edata.setData(data, len); }
      virtual const EvData eventData() const        { return edata; }

      virtual void dump(int n = 0) const;
      virtual void read(Xml&);
      virtual void write(int, Xml&, const Pos& offset, bool forcePath = false) const;
      virtual EventBase* mid(unsigned, unsigned) const;
      };

} // namespace MusECore

#endif

