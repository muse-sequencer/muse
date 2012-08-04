//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: waveevent.h,v 1.6.2.4 2009/12/20 05:00:35 terminator356 Exp $
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

#ifndef __WAVE_EVENT_H__
#define __WAVE_EVENT_H__

#include <sys/types.h>

#include "eventbase.h"

class AudioConverter;

namespace MusECore {

class WavePart;

//---------------------------------------------------------
//   WaveEvent
//---------------------------------------------------------

class WaveEventBase : public EventBase {
      QString _name;
      SndFileR f;
      int _spos;        // start sample position in WaveFile
      bool deleted;

      virtual EventBase* clone();

   public:
      WaveEventBase(EventType t);
      virtual ~WaveEventBase() {}

      virtual void read(Xml&);
      virtual void write(int, Xml&, const Pos& offset, bool forcePath = false) const;
      virtual EventBase* mid(unsigned, unsigned);
      
      virtual void dump(int n = 0) const;

      virtual const QString name() const       { return _name;  }
      virtual void setName(const QString& s)   { _name = s;     }
      virtual int spos() const                 { return _spos;  }
      virtual void setSpos(int s)              { _spos = s;     }
      virtual SndFileR sndFile() const         { return f;      }
      virtual void setSndFile(SndFileR& sf)    { f = sf;        }
      
      virtual void readAudio(WavePart* part, unsigned offset, 
                             float** bpp, int channels, int nn, bool doSeek, bool overwrite);
      };
      
} // namespace MusECore

#endif

