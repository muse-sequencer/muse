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
#include "audiostream.h"

class QString;

namespace MusECore {

class Xml;
class EventBase;
class WavePart;
class Part;

// NOTICE: The values 3 and 4 (PAfter and CAfter) are reserved for the support of those two obsolete
//          channel and key pressure events in old files. They are converted to controllers upon load.
enum EventType { Note=0, Controller=1, Sysex=2, /*PAfter=3,*/ /*CAfter=4,*/ Meta=5, Wave=6 };

//---------------------------------------------------------
//   Event
//---------------------------------------------------------

class Event { // TODO FINDMICH remove this layer around *EventBase!
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
      bool isSimilarTo(const Event& other) const;

      int getRefCount() const;
      bool selected() const;
      void setSelected(bool val);
      void move(int offset);

      void read(Xml& xml);
      void write(int a, Xml& xml, const Pos& offset, bool ForceWavePaths = false) const;
      void dump(int n = 0) const;
      Event clone() const;
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
      int spos() const; // TODO FINDMICH remove the spos work from Event etc, this has to go only in AudioStream!
      void setSpos(int s);
      
      virtual void readAudio(MusECore::WavePart* part, unsigned offset, float** bpp, int channels, int nn, XTick fromXTick, XTick toXTick, bool doSeek, bool overwrite);
      QString audioFilePath() const;
      void setAudioFile(const QString& path);
      void reloadAudioFile();
      AudioStream::stretch_mode_t stretchMode() const;
      const AudioStream* getAudioStream() const;
      
      void setTick(unsigned val);
      unsigned tick() const;
      XTick xtick() const;
      unsigned frame() const;
      void setFrame(unsigned val);
      void setLenTick(unsigned val);
      void setLenFrame(unsigned val);
      unsigned lenTick() const;
      XTick lenXTick() const;
      unsigned lenFrame() const;
      Pos end() const;
      unsigned endTick() const;
      XTick endXTick() const;
      unsigned endFrame() const;
      void setPos(const Pos& p);
      bool needCopyOnWrite();
      
      void setParentalPart(Part*); // TODO implement NOW FINDMICHJETZT
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
      iEvent add(Event& event); // never call this on a part's EventList! use part->addEvent() instead!
      void move(Event& event, unsigned tick);
      void dump() const;
      };

} // namespace MusECore

#endif

