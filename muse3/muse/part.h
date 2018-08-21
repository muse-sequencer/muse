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

#include <QUuid>

#include "event.h"
#include "audioconvert.h"

class QString;

namespace MusECore {

class MidiTrack;
class Track;
class Xml;
class Part;
class WaveTrack;
class PendingOperationList;

struct ClonePart {
      const Part* cp;
      int id;
      QUuid _uuid;
      bool is_deleted;
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
      int _clonemaster_sn; // the serial number of some clone in the chain. every member of the chain has the same value here.

      QString _name;
      bool _selected;
      // REMOVE Tim. citem. Added.
      // Temporary general purpose tag. Used for example when iterating
      //  midi controller canvases for selected canvas items to copy to the clipboard
      //  and needing to see if the event was already put into the selected item list.
      // This greatly saves from having to check the selected item list for duplicates
      //  for every item encountered. Just make sure to reset the tag at some point!
      bool _tagged;
      bool _eventsTagged;
      bool _mute;
      int _colorIndex;
                   
   protected:
      Track* _track;
      EventList _events;
      Part* _prevClone;
      Part* _nextClone;
      Part* _backupClone; // when a part gets removed, it's still there; and for undo-ing the remove, it must know about where it was clone-chained to.
      mutable int _hiddenEvents;   // Combination of HiddenEventsType.

   public:
      Part(Track*);
      virtual ~Part();
      virtual Part* duplicate() const;
      virtual Part* duplicateEmpty() const = 0;
      virtual Part* createNewClone() const; // this does NOT chain clones yet. Chain is updated only when the part is really added!
      virtual void splitPart(int tickpos, Part*& p1, Part*& p2) const;
      
      void setSn(int n)                { _sn = n; }
      int clonemaster_sn() const       { return _clonemaster_sn; }
      int sn() const                   { return _sn; }
      int newSn()                      { return snGen++; }

      const QString& name() const      { return _name; }
      void setName(const QString& s)   { _name = s; }
      bool selected() const            { return _selected; }
      void setSelected(bool f)         { _selected = f; }
      bool tagged() const              { return _tagged; }
      void setTagged(bool v)           { _tagged = v; }
      bool eventsTagged() const        { return _eventsTagged; }
      void setEventsTagged(bool v)     { _eventsTagged = v; }
      // Select or unselect a range of events. If t0 == t1, ALL events will be affected.
      // The t0 and t1 can be ticks or frames depending on the type of events. Unused for now.
      // Returns true if anything changed.
      bool selectEvents(bool select, unsigned long t0 = 0, unsigned long t1 = 0);
      bool mute() const                { return _mute; }
      void setMute(bool b)             { _mute = b; }
      Track* track() const             { return _track; }
      void setTrack(Track*t)           { _track = t; }
      const EventList& events() const  { return _events; }
      EventList& nonconst_events()     { return _events; }
      int colorIndex() const           { return _colorIndex; }
      void setColorIndex(int idx)      { _colorIndex = idx; }
      
      bool isCloneOf(const Part*) const;
      bool hasClones() const           { return _prevClone!=this || _nextClone!=this; }
      int nClones() const;
      Part* prevClone() const          { return _prevClone; } // FINDMICHJETZT make it const Part*!
      Part* nextClone() const          { return _nextClone; }
      Part* backupClone() const        { return _backupClone; }
      
      void unchainClone();
      void chainClone(Part* p); // *this is made a sibling of p! p is not touched (except for its clone-chain), whereas this->events will get altered
      void rechainClone(); // re-chains the part to the same clone chain it was unchained before
      
      // Returns combination of HiddenEventsType enum.
      virtual int hasHiddenEvents() const { return _hiddenEvents; }
      
      iEvent addEvent(Event& p); // this does not care about clones! If the part is a clone, be sure to execute this on all clones (with duplicated Events, that is!)
      // Returns true if any event was opened. Does not operate on the part's clones, if any.
      virtual bool openAllEvents() { return false; };
      // Returns true if any event was closed. Does not operate on the part's clones, if any.
      virtual bool closeAllEvents() { return false; };

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
      virtual MidiPart* duplicate() const;
      virtual MidiPart* duplicateEmpty() const;
      virtual MidiPart* createNewClone() const;

      
      MidiTrack* track() const   { return (MidiTrack*)Part::track(); }
      // Returns combination of HiddenEventsType enum.
      int hasHiddenEvents() const;
      
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
      virtual ~WavePart() {}
      virtual WavePart* duplicate() const;
      virtual WavePart* duplicateEmpty() const;
      virtual WavePart* createNewClone() const;

      WaveTrack* track() const   { return (WaveTrack*)Part::track(); }
      // Returns combination of HiddenEventsType enum.
      int hasHiddenEvents() const;
      // Returns true if any event was opened. Does not operate on the part's clones, if any.
      bool openAllEvents();
      // Returns true if any event was closed. Does not operate on the part's clones, if any.
      bool closeAllEvents();

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
      int index(const Part*) const;
      Part* find(int idx);
      void clearDelete() {
            for (iPart i = begin(); i != end(); ++i)
                  delete i->second;
            clear();
            }
            
      void addOperation(Part* part, PendingOperationList& ops); 
      void delOperation(Part* part, PendingOperationList& ops);
      void movePartOperation(Part* part, int new_pos, PendingOperationList& ops, Track* track = 0);
      };

extern void chainCheckErr(Part* p);
extern void unchainTrackParts(Track* t);
extern void chainTrackParts(Track* t);
extern void addPortCtrlEvents(Part* part, bool doClones);
extern void addPortCtrlEvents(const Event& event, Part* part, unsigned int tick, unsigned int len, Track* track, PendingOperationList& ops);
extern void addPortCtrlEvents(Event& event, Part* part);
extern void addPortCtrlEvents(Part* part, unsigned int tick, unsigned int len, Track* track, PendingOperationList& ops);
extern void removePortCtrlEvents(Part* part, bool doClones);
extern void removePortCtrlEvents(Part* part, Track* track, PendingOperationList& ops);
extern void removePortCtrlEvents(Event& event, Part* part);
extern void removePortCtrlEvents(const Event& event, Part* part, Track* track, PendingOperationList& ops);
extern void modifyPortCtrlEvents(const Event& old_event, const Event& event, Part* part, PendingOperationList& ops);

} // namespace MusECore

namespace MusEGlobal {
extern MusECore::CloneList cloneList;
}

#endif

