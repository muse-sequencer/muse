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

#include "type_defs.h"
#include "pos.h"
#include "evdata.h"
#include "mpevent.h"
#include "wave.h" // for SndFileR

class QString;

namespace MusECore {

class Xml;
class EventBase;
class WavePart;

// NOTICE: The values 3 and 4 (PAfter and CAfter) are reserved for the support of those two obsolete
//          channel and key pressure events in old files. They are converted to controllers upon load.
enum EventType { Note=0, Controller=1, Sysex=2, /*PAfter=3,*/ /*CAfter=4,*/ Meta=5, Wave=6 };

//---------------------------------------------------------
//   Event
//---------------------------------------------------------

class Event {
      EventBase* ev;

   public:
      Event();
      Event(EventType t);
      Event(const Event& e); // Creates a true shared clone of the event. They share the same event base pointer.
      Event(EventBase* eb);  // Wraps an event base in an event and increases the event base's ref count.
      
      virtual ~Event();

      MidiPlayEvent asMidiPlayEvent(unsigned time, int port, int channel) const;
      
      bool empty() const;
      EventType type() const;
      // Shared and non-shared clone events have the same id. An empty event returns MUSE_INVALID_EVENT_ID.
      EventID_t id() const; 
      void shareId(const Event& e); // Makes id same as given event's. Effectively makes the events non-shared clones.

      void setType(EventType t);
      Event& operator=(const Event& e); // Makes the two events true shared clones. They share the same event base pointer.
      
      virtual void assign(const Event&); // Assigns to this event, excluding the id. Including its clones.
      
      bool operator==(const Event& e) const;
      bool isSimilarTo(const Event& other) const;
      
      int getRefCount() const;
      bool selected() const;
      void setSelected(bool val);
      void move(int offset);

      void read(Xml& xml);
      void write(int a, Xml& xml, const Pos& offset, bool ForceWavePaths = false) const;
      void dump(int n = 0) const;
      // Creates a non-shared clone of the event base, having the same 'group' id.
      // NOTE: Certain pointer members may still be SHARED. Such as the sysex MidiEventBase::edata.
      //       Be aware when iterating or modifying clones.
      Event clone() const; 
      
      // Restores id to original uniqueId, removing the event from any clone 'group'. 
      void deClone(); 
      // Creates a copy of the event base, excluding the 'group' _id. 
      Event duplicate() const; 

      Event mid(unsigned a, unsigned b) const;

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
      unsigned posValue() const;
      void setFrame(unsigned val);
      void setLenTick(unsigned val);
      void setLenFrame(unsigned val);
      unsigned lenTick() const;
      unsigned lenFrame() const;
      unsigned lenValue() const;
      Pos end() const;
      unsigned endTick() const;
      unsigned endFrame() const;
      void setPos(const Pos& p);
      };

typedef std::multimap <unsigned, Event, std::less<unsigned> > EL;
typedef EL::iterator iEvent;
typedef EL::reverse_iterator riEvent;
typedef EL::const_iterator ciEvent;
typedef std::pair <ciEvent, ciEvent> EventRange;

//---------------------------------------------------------
//   EventList
//    tick sorted list of events
//---------------------------------------------------------

class EventList : public EL {
      void deselect();

   public:
      ciEvent find(const Event&) const;
      iEvent find(const Event&);
      ciEvent findSimilar(const Event&) const;
      iEvent findSimilar(const Event&);
      ciEvent findId(const Event&) const;             // Fast, index t is known.
      iEvent findId(const Event&);                    // Fast, index t is known.
      ciEvent findId(unsigned t, EventID_t id) const; // Fast, index t is known.
      iEvent findId(unsigned t, EventID_t id);        // Fast, index t is known.
      ciEvent findId(EventID_t id) const;             // Slow, index t is not known
      iEvent findId(EventID_t id);                    // Slow, index t is not known
      ciEvent findWithId(const Event&) const; // Finds event base or event id. Fast, index t is known.
      iEvent findWithId(const Event&);        // Finds event base or event id. Fast, index t is known.
      
      iEvent add(Event event);
      void move(Event& event, unsigned tick);
      void dump() const;
      void read(Xml& xml, const char* name, bool midi);
      };

} // namespace MusECore

#endif

