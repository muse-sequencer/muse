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

#ifndef __MIDI_EVENT_BASE_H__
#define __MIDI_EVENT_BASE_H__

#include "eventbase.h"

//---------------------------------------------------------
//   MidiEventBase
//---------------------------------------------------------

class MidiEventBase : public EventBase {
      int a, b;                  // pitch, velo-on
      int c;
      EvData edata;

      virtual EventBase* clone() const { return new MidiEventBase(*this); }

   public:
      MidiEventBase(EventType t);
      virtual ~MidiEventBase() {}

      virtual bool isNote() const                   { return type() == Note; }
      virtual bool isNoteOff() const;
      virtual bool isNoteOff(const Event&) const;
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

      virtual const unsigned char* data() const     { return edata.data;  }
      virtual int dataLen() const                   { return edata.dataLen; }
      virtual void setData(const unsigned char* data, int len) { edata.setData(data, len); }
      virtual const EvData eventData() const        { return edata; }

      virtual void dump(int n = 0) const;
      virtual void read(QDomNode);
      virtual void write(Xml&, const Pos& offset) const;
      virtual EventBase* mid(unsigned, unsigned);

      virtual bool operator==(const EventBase&) const;
      virtual bool operator==(const MidiEventBase&) const;
      };

#endif

