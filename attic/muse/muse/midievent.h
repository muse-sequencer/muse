//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: midievent.h,v 1.7.2.1 2009/05/24 21:43:44 terminator356 Exp $
//
//  (C) Copyright 1999-2004 Werner Schweer (ws@seh.de)
//=========================================================

#ifndef __MIDI_EVENT_H__
#define __MIDI_EVENT_H__

#include "eventbase.h"

//---------------------------------------------------------
//   MidiEventBase
//---------------------------------------------------------

class MidiEventBase : public EventBase {
      int a, b, c;                  // pitch, velo-on, velo-off
      EvData edata;

      virtual EventBase* clone() { return new MidiEventBase(*this); }

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
      virtual void read(Xml&);
      //virtual void write(int, Xml&, const Pos& offset) const;
      virtual void write(int, Xml&, const Pos& offset, bool forcePath = false) const;
      virtual EventBase* mid(unsigned, unsigned);
      };

#endif

