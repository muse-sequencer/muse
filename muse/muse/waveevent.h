//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: waveevent.h,v 1.6.2.4 2009/12/20 05:00:35 terminator356 Exp $
//
//  (C) Copyright 1999-2004 Werner Schweer (ws@seh.de)
//=========================================================

#ifndef __WAVE_EVENT_H__
#define __WAVE_EVENT_H__

#include "eventbase.h"

//---------------------------------------------------------
//   WaveEvent
//---------------------------------------------------------

class WaveEventBase : public EventBase {
      QString _name;
      SndFileR f;
      int _spos;        // start sample position in WaveFile
      bool deleted;

      virtual EventBase* clone() { return new WaveEventBase(*this); }

   public:
      WaveEventBase(EventType t);
      virtual ~WaveEventBase() {}

      virtual void read(Xml&);
      //virtual void write(int, Xml&, const Pos& offset) const;
      virtual void write(int, Xml&, const Pos& offset, bool forcePath = false) const;
      virtual EventBase* mid(unsigned, unsigned);
      
      virtual void dump(int n = 0) const;

      virtual const QString name() const       { return _name;  }
      virtual void setName(const QString& s)   { _name = s;     }
      virtual int spos() const                 { return _spos;  }
      virtual void setSpos(int s)              { _spos = s;     }
      virtual SndFileR sndFile() const         { return f;      }
      virtual void setSndFile(SndFileR& sf)    { f = sf;        }
      
      // Changed by Tim. p3.3.17
      //virtual void read(unsigned offset, float** bpp, int channels, int nn, bool overwrite = true);
      virtual void readAudio(unsigned /*offset*/, float** /*bpp*/, int /*channels*/, int /*nn*/, bool /*doSeek*/, bool /*overwrite*/);
      };
#endif

