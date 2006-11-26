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

#ifndef __EVENTBASE_H__
#define __EVENTBASE_H__

#include "al/pos.h"

//---------------------------------------------------------
//   EventBase
//---------------------------------------------------------

class EventBase : public AL::PosLen {
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

      virtual void read(QDomNode) = 0;
      virtual void write(Xml&, const Pos& offset) const = 0;
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
      virtual EventBase* clone() const = 0;
      virtual void read(unsigned /*offset*/, float** /*bpp*/, int /*channels*/, int /*nn*/) {}
      };

#endif

