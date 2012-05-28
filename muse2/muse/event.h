//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: event.h,v 1.7.2.4 2009/12/20 05:00:35 terminator356 Exp $
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

#ifndef __EVENT_H__
#define __EVENT_H__

#include <map>
#include <sys/types.h>

#include "pos.h"
#include "evdata.h"
#include "wave.h" // for SndFileR

class QString;

namespace MusECore {

class Xml;
class EventBase;
class WavePart;

enum EventType { Note, Controller, Sysex, PAfter, CAfter, Meta, Wave };


//---------------------------------------------------------
//   Event
//---------------------------------------------------------

class Event {
      EventBase* ev;

   public:
      Event();
      Event(EventType t);
      Event(const Event& e);
      Event(EventBase* eb);
      
      virtual ~Event();

      bool empty() const;
      EventType type() const;

      void setType(EventType t);
      Event& operator=(const Event& e);
      bool operator==(const Event& e) const;

      int getRefCount() const;
      bool selected() const;
      void setSelected(bool val);
      void move(int offset);

      void read(Xml& xml);
      void write(int a, Xml& xml, const Pos& offset, bool ForceWavePaths = false) const;
      void dump(int n = 0) const;
      Event clone();
      Event mid(unsigned a, unsigned b);

      bool isNote() const;
      bool isNoteOff() const;
      bool isNoteOff(const Event& e) const;
      int dataA() const;
      int pitch() const;
      void setA(int val);
      void setPitch(int val);
      int dataB() const;
      int velo() const;
      void setB(int val);
      void setVelo(int val);
      int dataC() const;
      int veloOff() const;
      void setC(int val);
      void setVeloOff(int val);

      const unsigned char* data() const;
      int dataLen() const;
      void setData(const unsigned char* data, int len);
      const EvData eventData() const;

      const QString name() const;
      void setName(const QString& s);
      int spos() const;
      void setSpos(int s);
      MusECore::SndFileR sndFile() const;
      virtual void setSndFile(MusECore::SndFileR& sf);
      
      virtual void readAudio(MusECore::WavePart* part, unsigned offset, float** bpp, int channels, int nn, bool doSeek, bool overwrite);
      
      void setTick(unsigned val);
      unsigned tick() const;
      unsigned frame() const;
      void setFrame(unsigned val);
      void setLenTick(unsigned val);
      void setLenFrame(unsigned val);
      unsigned lenTick() const;
      unsigned lenFrame() const;
      Pos end() const;
      unsigned endTick() const;
      unsigned endFrame() const;
      void setPos(const Pos& p);
      };

typedef std::multimap <unsigned, Event, std::less<unsigned> > EL;
typedef EL::iterator iEvent;
typedef EL::reverse_iterator riEvent;
typedef EL::const_iterator ciEvent;
typedef std::pair <iEvent, iEvent> EventRange;

//---------------------------------------------------------
//   EventList
//    tick sorted list of events
//---------------------------------------------------------

class EventList : public EL {
      int ref;          // number of references to this EventList
      int aref;         // number of active references (exclude undo list)
      void deselect();

   public:
      EventList()           { ref = 0; aref = 0;  }
      ~EventList()          {}

      void incRef(int n)    { ref += n;    }
      int refCount() const  { return ref;  }
      void incARef(int n)   { aref += n;   }
      int arefCount() const { return aref; }

      iEvent find(const Event&);
      iEvent add(Event& event);
      void move(Event& event, unsigned tick);
      void dump() const;
      void read(Xml& xml, const char* name, bool midi);
      };

} // namespace MusECore

#endif

