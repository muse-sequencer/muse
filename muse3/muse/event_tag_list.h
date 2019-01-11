//=========================================================
//  MusE
//  Linux Music Editor
//
//  event_tag_list.h
//  (C) Copyright 2019 Tim E. Real (terminator356 on users dot sourceforge dot net)
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

#ifndef __EVENT_TAG_LIST_H__
#define __EVENT_TAG_LIST_H__

#include <map>
#include <set>

#include "part.h"
#include "event.h"
#include "pos.h"

namespace MusECore {

//typedef std::set<const Event> TagEvents_t;
//typedef std::pair<const Part*, TagEvents_t> TagPartsPair_t;
typedef std::pair<const Part*, EventList> TagEventListPair_t;

typedef std::map< const Part*, EventList, std::less<const Part*> > TagEventList_t;

class TagEventList : public TagEventList_t
{
  private:
  
    bool _startPosValid;
    Pos _startPos;
    
  public:
    
    TagEventList() : _startPosValid(false) { }
    
    // Adds a part and optionally an event.
// //     // If no event is given, does not add the part if a clone
// //     //  or the part itself already exists in the list.
    // If no event is given, does not add the part if the part
    //  already exists in the list.
    // Adding a part alone with an empty event list means to add
    //  ALL of its events, which is an optimization so that all events
    //  do not have to added and the reader will know to use all of
    //  the part's event list rather than this event list.
// //     // If the event is given, does not allow clone events to be added,
    // If the event is given, does not allow duplicate events to be added,
    //  but does allow clone parts in order to catch 'rogue' events
    //  that might be missing from other clone parts by mistake.
    // The cached starting position is set to the 'soonest' event.
    // If resetStartPos is true, resets the cached starting position,
    //  and recomputes it from scratch, effectively the same as calling
    //  getStartPos() but faster and more convenient.
    // Returns true if successfully added.
    bool add(const Part*, const Event* = NULL, bool resetStartPos = false);
    // Returns the cached position of the leftmost event.
    Pos startPos() const { return _startPosValid ? _startPos : Pos(); }
    // Recomputes cached position of the leftmost event and returns the position.
    Pos getStartPos();
};

// typedef TagEvents_t::const_iterator ciTagEvents;
// typedef TagEvents_t::iterator iTagEvents;
typedef TagEventList::const_iterator ciTagEventList;
typedef TagEventList::iterator iTagEventList;


//--------------------------------------------------------------
// Event tagging flags and structure for the tagging and
//  copying/pasting system.
//--------------------------------------------------------------

// REMOVE Tim. citem. Added.
// enum EventTagFlags { NoEventTagFlags = 0x0,
//   EventTagged = 0x01,
//   // Whether the EventTagStruct 'width' member is valid.
//   EventTagWidthValid = 0x02
//   // REMOVE Tim. citem. Added.
//   // This is the last event in a tagged series of events - controllers for example.
//   // ie. a range was selected and then another range after it with a gap in between,
//   //  and this is the last event in that first group before the gap.
//   //EventTagLastInGroup = 0x04
// };
// typedef int EventTagFlags_t;
// 
// struct EventTagStruct
// {
//   EventTagFlags_t _flags;
//   // A value intended for controller events which carries
//   //  a width of a controller 'bar' and gives the position
//   //  of the next controller event (the same as the 'ex' 
//   //  value in CItem). The value is the width of a
//   //  controller 'bar' as visualized on controller graphs.
//   // It is valid only if the EventTagWidthValid flag is set.
//   unsigned int _width;
//   
//   EventTagStruct(EventTagFlags_t flags = NoEventTagFlags, unsigned int width = 0) : _flags(flags), _width(width) { }
//   void clear() { _flags = NoEventTagFlags; _width = 0; }
//   void setTagged(bool v) { v ? _flags |= EventTagged : _flags &= ~EventTagged; }
//   bool isTagged() const { return _flags & EventTagged; }
//   void appendFlags(EventTagFlags_t flags) { _flags |= flags; }
// };

enum EventTagOptions {
  TagNoOptions = 0x00,
  // Tag selected items.
  TagSelected = 0x01,
  // Tag moving items.
  TagMoving = 0x02,
  TagSelectedAndMoving = TagSelected | TagMoving,
  // Whether to tag all items regardless of selection or moving.
  TagAllItems = 0x04,
  // Whether to tag all parts regardless of selection or moving.
  TagAllParts = 0x08,
  // Whether the range parameters are valid.
  TagRange = 0x10,
  EventTagAllOptions = TagSelected | TagMoving | TagAllItems | TagAllParts | TagRange
};
typedef int EventTagOptions_t;

struct EventTagOptionsStruct
{
  EventTagOptions_t _flags;
  Pos _p0;
  Pos _p1;

  EventTagOptionsStruct(const EventTagOptions_t& flags, Pos p0 = Pos(), Pos p1 = Pos()) :
    _flags(flags), _p0(p0), _p1(p1) { }
  EventTagOptionsStruct(bool tagAllItems, bool tagAllParts, bool tagRange, Pos p0 = Pos(), Pos p1 = Pos(),
                        bool tagSelected = true, bool tagMoving = false) :
    _flags((tagAllItems ? TagAllItems : TagNoOptions) |
           (tagAllParts ? TagAllParts : TagNoOptions) |
           (tagRange    ? TagRange    : TagNoOptions) |
           (tagSelected ? TagSelected : TagNoOptions) |
           (tagMoving   ? TagMoving   : TagNoOptions)),
    _p0(p0), _p1(p1) { }
  void clear() { _flags = TagNoOptions; _p0 = Pos(); _p1 = Pos(); }
  void appendFlags(const EventTagOptions_t& flags) { _flags |= flags; }
  void removeFlags(const EventTagOptions_t& flags) { _flags &= ~flags; }
};

} // namespace MusECore

#endif
