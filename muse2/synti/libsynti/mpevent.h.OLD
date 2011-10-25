//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: mpevent.h,v 1.1 2004/02/12 18:30:31 wschweer Exp $
//
//  (C) Copyright 1999-2002 Werner Schweer (ws@seh.de)
//=========================================================

#ifndef __MPEVENT_H__
#define __MPEVENT_H__

#include "evdata.h"

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

   public:
      MEvent() {}
      MEvent(unsigned tm, int p, int c, int t, int a, int b)
        : _time(tm), _port(p), _channel(c & 0xf), _type(t), _a(a), _b(b) {}
      MEvent(unsigned t, int p, int type, const unsigned char* data, int len);
      MEvent(unsigned t, int p, int tpe, EvData d) : _time(t), edata(d), _port(p), _type(tpe) {}

      ~MEvent()         {}

      int port()    const      { return _port;    }
      int channel() const      { return _channel; }
      int type()    const      { return _type;    }
      int dataA()   const      { return _a;       }
      int dataB()   const      { return _b;       }
      unsigned time() const    { return _time;    }

      void setPort(int val)    { _port = val;     }
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

//---------------------------------------------------------
//   MidiRecordEvent
//    allocated and deleted in midiseq thread context
//---------------------------------------------------------

class MidiPlayEvent;

class MidiRecordEvent : public MEvent {
   public:
      MidiRecordEvent() {}
      MidiRecordEvent(const MEvent& e) : MEvent(e) {}
      MidiRecordEvent(unsigned tm, int p, int c, int t, int a, int b)
        : MEvent(tm, p, c, t, a, b) {}
      MidiRecordEvent(unsigned t, int p, int tpe, const unsigned char* data, int len)
        : MEvent(t, p, tpe, data, len) {}
      MidiRecordEvent(unsigned t, int p, int type, EvData data)
        : MEvent(t, p, type, data) {}
      ~MidiRecordEvent() {}
      };

//---------------------------------------------------------
//   MidiPlayEvent
//    allocated and deleted in audio thread context
//---------------------------------------------------------

class MidiPlayEvent : public MEvent {
   public:
      MidiPlayEvent() {}
      MidiPlayEvent(const MEvent& e) : MEvent(e) {}
      MidiPlayEvent(unsigned tm, int p, int c, int t, int a, int b)
        : MEvent(tm, p, c, t, a, b) {}
      MidiPlayEvent(unsigned t, int p, int type, const unsigned char* data, int len)
        : MEvent(t, p, type, data, len) {}
      MidiPlayEvent(unsigned t, int p, int type, EvData data)
        : MEvent(t, p, type, data) {}
      ~MidiPlayEvent() {}
      };

#endif

