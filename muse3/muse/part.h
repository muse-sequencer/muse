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
#include <vector>
#include <climits>

#include <QUuid>

#include "event.h"
#include "audio_convert/lib_audio_convert/audioconvert.h"

class QString;

namespace MusECore {

class MidiTrack;
class Track;
class Xml;
class Part;
class WaveTrack;
class PendingOperationList;

//---------------------------------------------------------
//   MidiCtrlViewState
//   Stores the initial controller view state
//---------------------------------------------------------

struct MidiCtrlViewState
{
  // Midi controller number.
  int _num;
  // Per note velocity display.
  bool _perNoteVel;
  
  MidiCtrlViewState() : _num(0), _perNoteVel(false) { }
  MidiCtrlViewState(int num, bool perNoteVel = false) : _num(num), _perNoteVel(perNoteVel) { }

  void read(Xml&);
  void write(int, Xml&) const;
};

typedef std::vector<MidiCtrlViewState> MidiCtrlViewStateList;
typedef MidiCtrlViewStateList::iterator iMidiCtrlViewState;
typedef MidiCtrlViewStateList::const_iterator ciMidiCtrlViewState;

//---------------------------------------------------------
//   MidiPartViewState
//   Stores the initial view state
//---------------------------------------------------------

class MidiPartViewState
{
private:
  int _xscroll;
  int _yscroll;
  int _xscale;
  int _yscale;
  MidiCtrlViewStateList _controllers;

public:
  MidiPartViewState() : _xscroll(INT_MAX), _yscroll(INT_MAX), _xscale(INT_MAX), _yscale(INT_MAX) { }
  MidiPartViewState(int xscroll, int yscroll, int xscale, int yscale) :
        _xscroll(xscroll), _yscroll(yscroll), _xscale(xscale), _yscale(yscale) { }
  MidiPartViewState(const MidiPartViewState& vs) : 
    _xscroll(vs._xscroll), _yscroll(vs._yscroll),
    _xscale(vs._xscale), _yscale(vs._yscale),
    _controllers(vs._controllers) { }
  MidiPartViewState& operator=(const MidiPartViewState& vs) {
    _xscroll = vs._xscroll; _yscroll = vs._yscroll;
    _xscale = vs._xscale; _yscale = vs._yscale;
    _controllers = vs._controllers; return *this;
    }

  bool isValid() const { return _xscroll != INT_MAX && _yscroll != INT_MAX && _xscale != INT_MAX && _yscale != INT_MAX; }

  int xscroll() const { return _xscroll; }
  int yscroll() const { return _yscroll; }
  int xscale() const { return _xscale; }
  int yscale() const { return _yscale; }

  void setXScroll(int x) { _xscroll = x; }
  void setYScroll(int y) { _yscroll = y; }
  void setXScale(int xs) { _xscale = xs; }
  void setYScale(int ys) { _yscale = ys; }

  const MidiCtrlViewStateList& controllers() { return _controllers; }
  void addController(const MidiCtrlViewState& viewState) { _controllers.push_back(viewState); }
  void clearControllers() { _controllers.clear(); }

  void read(Xml&);
  void write(int, Xml&) const;
};


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
      enum PartType { MidiPartType = 0x01, WavePartType = 0x02 };
      
      static Part* readFromXml(Xml&, Track*, bool doClone = false, bool toTrack = true);
   
   private:
      static int snGen;
      int _sn;
      int _clonemaster_sn; // the serial number of some clone in the chain. every member of the chain has the same value here.

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
      mutable int _hiddenEvents;   // Combination of HiddenEventsType.
      MidiPartViewState _viewState;

   public:
      Part(Track*);
      virtual ~Part();
      
      virtual PartType partType() const = 0;
      
      virtual Part* duplicate() const;
      virtual Part* duplicateEmpty() const = 0;
      virtual Part* createNewClone() const; // this does NOT chain clones yet. Chain is updated only when the part is really added!
      virtual void splitPart(unsigned int tickpos, Part*& p1, Part*& p2) const;
      
      void setSn(int n)                { _sn = n; }
      int clonemaster_sn() const       { return _clonemaster_sn; }
      int sn() const                   { return _sn; }
      int newSn()                      { return snGen++; }

      const QString& name() const      { return _name; }
      void setName(const QString& s)   { _name = s; }
      bool selected() const            { return _selected; }
      void setSelected(bool f)         { _selected = f; }
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

      const MidiPartViewState& viewState() const { return _viewState; }
      MidiPartViewState& viewState() { return _viewState; }
      virtual void setViewState(const MidiPartViewState& vs);
      };


//---------------------------------------------------------
//   MidiPart
//---------------------------------------------------------

class MidiPart : public Part {

   public:
      MidiPart(MidiTrack* t) : Part((Track*)t) {}
      virtual ~MidiPart() {}
      
      virtual PartType partType() const { return MidiPartType; }
      
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
      //AudioConvertMap _converters;

   public:
      WavePart(WaveTrack* t);
      virtual ~WavePart() {}

      virtual PartType partType() const { return WavePartType; }

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

typedef std::pair<unsigned int, Part*> PartListInsertPair_t;
typedef std::multimap<unsigned int, Part*, std::less<unsigned int> > PartList_t;

class PartList : public PartList_t {
   public:
      iterator findPart(unsigned tick);
      iterator add(Part*);
      void remove(Part* part);
      int index(const Part*) const;
      Part* find(int idx);
      void clearDelete() {
            for (iterator i = begin(); i != end(); ++i)
                  delete i->second;
            clear();
            }
            
      void addOperation(Part* part, PendingOperationList& ops); 
      void delOperation(Part* part, PendingOperationList& ops);
      void movePartOperation(Part* part, unsigned int new_pos, PendingOperationList& ops, Track* track = 0);
      };

typedef PartList_t::iterator iPart;
typedef PartList_t::reverse_iterator riPart;
typedef PartList_t::const_iterator ciPart;

extern void chainCheckErr(Part* p);
extern void unchainTrackParts(Track* t);
extern void chainTrackParts(Track* t);
extern void addPortCtrlEvents(Part* part, bool doClones);
extern void addPortCtrlEvents(const Event& event, Part* part, unsigned int tick, unsigned int len, Track* track, PendingOperationList& ops);
extern void addPortCtrlEvents(Part* part, unsigned int tick, unsigned int len, Track* track, PendingOperationList& ops);
extern void removePortCtrlEvents(Part* part, bool doClones);
extern void removePortCtrlEvents(Part* part, Track* track, PendingOperationList& ops);
extern bool removePortCtrlEvents(const Event& event, Part* part, Track* track, PendingOperationList& ops);
extern void modifyPortCtrlEvents(const Event& old_event, const Event& event, Part* part, PendingOperationList& ops);

} // namespace MusECore

namespace MusEGlobal {
extern MusECore::CloneList cloneList;
}

#endif

