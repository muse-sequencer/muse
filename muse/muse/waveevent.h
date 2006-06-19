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

      virtual void read(QDomNode);
      virtual void write(Xml&, const Pos& offset) const;
      virtual EventBase* mid(unsigned, unsigned);
      virtual void dump(int n = 0) const;


      virtual const QString name() const       { return _name;  }
      virtual void setName(const QString& s)   { _name = s;     }
      virtual int spos() const                 { return _spos;  }
      virtual void setSpos(int s)              { _spos = s;     }
      virtual SndFileR sndFile() const         { return f;      }
      virtual void setSndFile(SndFileR& sf)    { f = sf;        }
      virtual void read(unsigned offset, float** bpp, int channels, int nn);
      };

#endif

