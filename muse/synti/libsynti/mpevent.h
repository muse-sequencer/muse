//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: mpevent.h,v 1.3 2005/05/11 14:18:48 wschweer Exp $
//
//  (C) Copyright 1999-2002 Werner Schweer (ws@seh.de)
//=========================================================

#ifndef __MPEVENT_H__
#define __MPEVENT_H__

#include "evdata.h"

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

      ~MidiEvent()         {}

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
      bool isNote() const      { return _type == 0x90; }
      bool isNoteOff() const   { return (_type == 0x80)||(_type == 0x90 && _b == 0); }
      };

#endif

