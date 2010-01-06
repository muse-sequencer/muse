//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: eventbase.h,v 1.3.2.3 2009/12/20 05:00:35 terminator356 Exp $
//
//  (C) Copyright 1999-2004 Werner Schweer (ws@seh.de)
//=========================================================

#ifndef __EVENTBASE_H__
#define __EVENTBASE_H__

//#include <samplerate.h>
#include <sys/types.h>

#include "pos.h"

class AudioConverter;

//---------------------------------------------------------
//   EventBase
//---------------------------------------------------------

class EventBase : public PosLen {
      EventType _type;

   protected:
      int refCount;
      bool _selected;

   public:
      EventBase(EventType t);
      EventBase(const EventBase& ev);

      virtual ~EventBase() {}

      int getRefCount() const    { return refCount; }
      EventType type() const     { return _type;  }
      void setType(EventType t)  { _type = t;  }
      bool selected() const      { return _selected; }
      void setSelected(bool val) { _selected = val; }

      void move(int offset);

      virtual void read(Xml&) = 0;
      //virtual void write(int, Xml&, const Pos& offset) const = 0;
      virtual void write(int, Xml&, const Pos& offset, bool forcePath = false) const = 0;
      virtual void dump(int n = 0) const;
      virtual EventBase* mid(unsigned, unsigned) = 0;
      friend class Event;

      virtual bool isNote() const                   { return false; }
      virtual bool isNoteOff() const                { return false; }
      virtual bool isNoteOff(const Event&) const    { return false; }
      virtual int pitch() const                     { return 0;      }
      virtual int program() const                   { return 0;      }
      virtual int cntrl() const                     { return 0;      }
      virtual int dataA() const                     { return 0;      }
      virtual void setA(int)                        { }
      virtual void setPitch(int)                    { }

      virtual int cntrlVal() const                  { return 0;      }
      virtual int dataB() const                     { return 0;      }
      virtual int velo() const                      { return 0;      }
      virtual void setB(int)                        { }
      virtual void setVelo(int)                     { }

      virtual int veloOff() const                   { return 0;      }
      virtual int dataC() const                     { return 0;      }
      virtual void setC(int)                        { }
      virtual void setVeloOff(int)                  { }

      virtual const unsigned char* data() const     { return 0; }
      virtual int dataLen() const                   { return 0; }
      virtual void setData(const unsigned char*, int) { }
      virtual const EvData eventData() const        { return EvData(); }

      virtual const QString name() const            { return QString("?");  }
      virtual void setName(const QString&)          { }
      virtual int spos() const                      { return 0;  }
      virtual void setSpos(int)                     { }
      virtual SndFileR sndFile() const              { return 0;      }
      virtual void setSndFile(SndFileR&)            { }
      virtual EventBase* clone() = 0;
      
      //virtual void read(unsigned /*offset*/, float** /*bpp*/, int /*channels*/, int /*nn*/, bool /*doSeek*/, bool overwrite = true) {}
      //virtual void readAudio(unsigned /*offset*/, float** /*bpp*/, int /*channels*/, int /*nn*/, bool /*doSeek*/, bool /*overwrite*/) {}
      //virtual off_t readAudio(SRC_STATE* /*src_state*/, off_t /*sfCurFrame*/, unsigned /*offset*/, 
      //                       float** /*bpp*/, int /*channels*/, int /*nn*/, bool /*doSeek*/, bool /*overwrite*/) { return 0; }
      virtual off_t readAudio(AudioConverter* /*audConv*/, off_t /*sfCurFrame*/, unsigned /*offset*/, 
                             float** /*bpp*/, int /*channels*/, int /*nn*/, bool /*doSeek*/, bool /*overwrite*/) { return 0; }
      };
#endif

