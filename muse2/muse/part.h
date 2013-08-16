//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: part.h,v 1.5.2.4 2009/05/24 21:43:44 terminator356 Exp $
//
//  (C) Copyright 1999/2000 Werner Schweer (ws@seh.de)
//  Additions, modifications (C) Copyright 2011 Tim E. Real (terminator356 on users DOT sourceforge DOT net)
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

#ifndef __PART_H__
#define __PART_H__

#include <map>

// Added by T356.
#include <uuid/uuid.h>

#include "event.h"
#include "audioconvert.h"

class QString;

namespace MusECore {

class MidiTrack;
class Track;
class Xml;
class Part;
class WaveTrack;

struct ClonePart {
      const Part* cp;
      int id;
      uuid_t uuid;
      ClonePart(const Part*, int i = -1);
      };

typedef std::list<ClonePart> CloneList;
typedef CloneList::iterator iClone;

//---------------------------------------------------------
//   Part
//---------------------------------------------------------

class Part : public PosLen {
   public:
      enum HiddenEventsType { NoEventsHidden = 0, LeftEventsHidden, RightEventsHidden };
      
      static Part* readFromXml(Xml&, Track*, bool doClone = false, bool toTrack = true);
   
   private:
      static int snGen;
      int _sn;

      QString _name;
      bool _selected;
      bool _mute;
      int _colorIndex;
                   
   protected:
      Track* _track;
      EventList _events;
      Part* _prevClone;
      Part* _nextClone;
      Part* _backupClone; // when a part gets removed, it's still there; and for undo-ing the remove, it must know about where it was clone-chained to.
      int _hiddenEvents;   // Combination of HiddenEventsType.

   public:
      Part(Track*);
      virtual ~Part();
      virtual Part* duplicate() = 0;
      virtual Part* duplicateEmpty() = 0;
      virtual Part* createNewClone() = 0;
      
      int sn()                         { return _sn; }
      void setSn(int n)                { _sn = n; }
      int newSn()                      { return snGen++; }

      const QString& name() const      { return _name; }
      void setName(const QString& s)   { _name = s; }
      bool selected() const            { return _selected; }
      void setSelected(bool f)         { _selected = f; }
      bool mute() const                { return _mute; }
      void setMute(bool b)             { _mute = b; }
      Track* track() const             { return _track; }
      void setTrack(Track*t)           { _track = t; }
      const EventList& events() const  { return _events; }
      EventList& nonconst_events()     { return _events; }
      int colorIndex() const           { return _colorIndex; }
      void setColorIndex(int idx)      { _colorIndex = idx; }
      
      bool hasClones()                 { return _prevClone!=this || _nextClone!=this; }
      int nClones();
      Part* prevClone()                { return _prevClone; }
      Part* nextClone()                { return _nextClone; }
      
      void unchainClone();
      void chainClone(Part* p); // *this is made a sibling of p! p is not touched (except for its clone-chain), whereas this->events will get altered
      void rechainClone(); // re-chains the part to the same clone chain it was unchained before
      
      // Returns combination of HiddenEventsType enum.
      virtual int hasHiddenEvents() = 0;
      // If repeated calls to hasHiddenEvents() are desired, then to avoid re-iteration of the event list, 
      //  call this after hasHiddenEvents().
      int cachedHasHiddenEvents() const { return _hiddenEvents; }
      
      iEvent addEvent(Event& p); // DEPRECATED. requires the part to be NOT a clone. FIXME remove!

      virtual void write(int, Xml&, bool isCopy = false, bool forceWavePaths = false) const;
      
      virtual void dump(int n = 0) const;
      };


//---------------------------------------------------------
//   MidiPart
//---------------------------------------------------------

class MidiPart : public Part {

   public:
      MidiPart(MidiTrack* t) : Part((Track*)t) {}
      virtual ~MidiPart() {}
      virtual MidiPart* duplicate();
      virtual MidiPart* duplicateEmpty();
      virtual MidiPart* createNewClone();

      
      MidiTrack* track() const   { return (MidiTrack*)Part::track(); }
      // Returns combination of HiddenEventsType enum.
      int hasHiddenEvents();
      
      virtual void dump(int n = 0) const;
      };


//---------------------------------------------------------
//   WavePart
//---------------------------------------------------------

class WavePart : public Part {

      // p3.3.31
      AudioConvertMap _converters;

   public:
      WavePart(WaveTrack* t);
      WavePart(WaveTrack* t, EventList* ev);
      virtual ~WavePart() {}
      virtual WavePart* duplicate();
      virtual WavePart* duplicateEmpty();
      virtual WavePart* createNewClone();

      WaveTrack* track() const   { return (WaveTrack*)Part::track(); }
      // Returns combination of HiddenEventsType enum.
      int hasHiddenEvents();

      virtual void dump(int n = 0) const;
      };


//---------------------------------------------------------
//   PartList
//---------------------------------------------------------

typedef std::multimap<int, Part*, std::less<unsigned> >::iterator iPart;
typedef std::multimap<int, Part*, std::less<unsigned> >::reverse_iterator riPart;
typedef std::multimap<int, Part*, std::less<unsigned> >::const_iterator ciPart;

class PartList : public std::multimap<int, Part*, std::less<unsigned> > {
   public:
      iPart findPart(unsigned tick);
      iPart add(Part*);
      void remove(Part* part);
      int index(Part*);
      Part* find(int idx);
      void clearDelete() {
            for (iPart i = begin(); i != end(); ++i)
                  delete i->second;
            clear();
            }
      };

extern void chainClone(Part* p1, Part* p2);
extern void unchainClone(Part* p);
extern void chainCheckErr(Part* p);
extern void unchainTrackParts(Track* t, bool decRefCount);
extern void chainTrackParts(Track* t, bool incRefCount);
extern void addPortCtrlEvents(Part* part, bool doClones);
extern void addPortCtrlEvents(Event& event, Part* part, bool doClones);
extern void removePortCtrlEvents(Part* part, bool doClones);
extern void removePortCtrlEvents(Event& event, Part* part, bool doClones);

} // namespace MusECore

namespace MusEGlobal {
extern MusECore::CloneList cloneList;
}

#endif

