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

#include <list>

#include "event.h"
#include "ctrl.h"
#include "pos.h"
#include "type_defs.h"

namespace MusECore {

// Forward declarations:
class Part;
class Track;

class TagEventStatsStruct
{
  private:

    unsigned int _notes;
    unsigned int _midiCtrls;
    unsigned int _sysexes;
    unsigned int _metas;
    unsigned int _waves;
    unsigned int _audioCtrls;

    PosLen _noteRange;
    PosLen _midiCtrlRange;
    PosLen _sysexRange;
    PosLen _metaRange;
    PosLen _waveRange;
    PosLen _audioCtrlRange;

  public:

    TagEventStatsStruct();
    // Be sure to use this instead of EventList::add(), to use the statistics feature.
    // For part midi events.
    void add(const Event& e);
    // For track audio controller events.
    void add(unsigned int frame);

    //--------------
    // Statistics:
    //--------------
    
    unsigned int notes() const;
    unsigned int mctrls() const;
    unsigned int sysexes() const;
    unsigned int metas() const;
    unsigned int waves() const;
    unsigned int actrls() const;

    PosLen noteRange() const;
    PosLen midiCtrlRange() const;
    PosLen sysexRange() const;
    PosLen metaRange() const;
    PosLen waveRange() const;
    PosLen audioCtrlRange() const;

    PosLen evrange(const RelevantSelectedEvents_t& types = AllEventsRelevant) const;
};

//-------------------------------------------
// TagEventListStruct
// Basically meant to be filled once then discarded.
// Removing items would not be of much use since the
//  statistics would have to recomputed each time.
//-------------------------------------------
  
class TagEventListStruct
{
  private:
    const Part* _part;
    AudioAutomationItemTrackMap _aaitm;

    EventList _evlist;
    TagEventStatsStruct _stats;

  public:

    TagEventListStruct();
    TagEventListStruct(const Part* part);

    const Part* part() const;
    void setPart(const Part* part);
    const Track* track() const;
    void setTrack(const Track* track);

    EventList& evlist();
    const EventList& evlist() const;
    AudioAutomationItemTrackMap& aaitm();
    const AudioAutomationItemTrackMap& aaitm() const;

    // Be sure to use this instead of EventList::add(), to use the statistics feature.
    // Returns true if successfully added.
    // For part midi events.
    bool add(const Event& e);
    // For track audio controller events.
    bool add(Track* track, CtrlList* ctrlList, unsigned int frame, double value);

    //--------------
    // Statistics:
    //--------------
    const TagEventStatsStruct& stats() const;
};

typedef std::list<TagEventListStruct> TagEventList_t;

class TagEventList : public TagEventList_t
{
  private:
  
    TagEventStatsStruct _globalStats;
    
  public:
    
    TagEventList();
    
    // Adds a part and optionally an event.
    // Returns true if successfully added.
    // For part midi events.
    bool add(const Part*, const Event& = Event());
    // For track audio controller events.
    bool add(Track* track, CtrlList* ctrlList, unsigned int frame, double value);

    //--------------
    // Statistics:
    //--------------
    const TagEventStatsStruct& globalStats() const;
    // Fills tclist with info about found controllers.
    // If findCtl is given it finds that specific controller.
    // Otherwise if findCtl -1 it finds all controllers.
    void globalCtlStats(FindMidiCtlsList_t* tclist, int findCtl = -1) const;
};

typedef TagEventList::const_iterator ciTagEventList;
typedef TagEventList::iterator iTagEventList;


//--------------------------------------------------------------
// Event tagging flags and structure for the tagging and
//  copying/pasting system.
//--------------------------------------------------------------

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
  TagDefaults = TagSelected | TagAllParts,
  EventTagAllOptions = TagSelected | TagMoving | TagAllItems | TagAllParts | TagRange
};
typedef int EventTagOptions_t;

struct EventTagOptionsStruct
{
  EventTagOptions_t _flags;
  Pos _p0;
  Pos _p1;

  EventTagOptionsStruct();
  EventTagOptionsStruct(const EventTagOptions_t& flags, Pos p0 = Pos(), Pos p1 = Pos());

  static EventTagOptionsStruct fromOptions(bool tagAllItems, bool tagAllParts, bool tagRange, Pos p0 = Pos(), Pos p1 = Pos(),
                        bool tagSelected = true, bool tagMoving = false);
  void clear();
  void appendFlags(const EventTagOptions_t& flags);
  void removeFlags(const EventTagOptions_t& flags);
};

} // namespace MusECore

#endif
