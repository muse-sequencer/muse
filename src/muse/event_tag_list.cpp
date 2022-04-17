//=========================================================
//  MusE
//  Linux Music Editor
//
//  event_tag_list.cpp
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

#include "event_tag_list.h"

// Forwards from header:
#include "part.h"

namespace MusECore {

//----------------------------------
//  TagEventStatsStruct
//----------------------------------

TagEventStatsStruct::TagEventStatsStruct() : _notes(0), _midiCtrls(0), _sysexes(0), _metas(0), _waves(0),
      _waveRange(false /* use Pos::FRAMES */), _audioCtrlRange(false /* use Pos::FRAMES */) {}

unsigned int TagEventStatsStruct::notes() const { return _notes; }
unsigned int TagEventStatsStruct::mctrls() const { return _midiCtrls; }
unsigned int TagEventStatsStruct::sysexes() const { return _sysexes; }
unsigned int TagEventStatsStruct::metas() const { return _metas; }
unsigned int TagEventStatsStruct::waves() const { return _waves; }
unsigned int TagEventStatsStruct::actrls() const { return _audioCtrls; }

PosLen TagEventStatsStruct::noteRange() const { return  _noteRange; }
PosLen TagEventStatsStruct::midiCtrlRange() const { return _midiCtrlRange; }
PosLen TagEventStatsStruct::sysexRange() const { return _sysexRange; }
PosLen TagEventStatsStruct::metaRange() const { return  _metaRange; }
PosLen TagEventStatsStruct::waveRange() const { return  _waveRange; }
PosLen TagEventStatsStruct::audioCtrlRange() const { return _audioCtrlRange; }

void TagEventStatsStruct::add(const Event& e)
{
  //if(e.empty())
  //  return;

  switch(e.type())
  {
    case Note:
      if(_notes == 0 || (e.pos() < _noteRange))
        _noteRange.setPos(e.pos());
      if(_notes == 0 || (e.end() > _noteRange.end()))
        _noteRange.setEnd(e.end());

      ++_notes;
    break;

    case Wave:
      if(_waves == 0 || (e.pos() < _waveRange))
        _waveRange.setPos(e.pos());
      if(_waves == 0 || (e.end() > _waveRange.end()))
        _waveRange.setEnd(e.end());

      ++_waves;
    break;

    case Controller:
      if(_midiCtrls == 0 || (e.pos() < _midiCtrlRange))
        _midiCtrlRange.setPos(e.pos());
      // For these events, minimum 1 unit time, to qualify as a valid 'range'.
      if(_midiCtrls == 0 || (e.posValue() + 1 > _midiCtrlRange.endValue()))
        _midiCtrlRange.setEndValue(e.posValue() + 1);

      ++_midiCtrls;
    break;

    case Sysex:
      if(_sysexes == 0 || (e.pos() < _sysexRange))
        _sysexRange.setPos(e.pos());
      // For these events, minimum 1 unit time, to qualify as a valid 'range'.
      if(_sysexes == 0 || (e.posValue() + 1 > _sysexRange.endValue()))
        _sysexRange.setEndValue(e.posValue() + 1);

      ++_sysexes;
    break;

    case Meta:
      if(_metas == 0 || (e.pos() < _metaRange))
        _metaRange.setPos(e.pos());
      // For these events, minimum 1 unit time, to qualify as a valid 'range'.
      if(_metas == 0 || (e.posValue() + 1 > _metaRange.endValue()))
        _metaRange.setEndValue(e.posValue() + 1);

      ++_metas;
    break;
  }
}

void TagEventStatsStruct::add(unsigned int frame)
{
  if(_audioCtrls == 0 || (frame < _audioCtrlRange.frame()))
    _audioCtrlRange.setFrame(frame);
  // For these events, minimum 1 unit time, to qualify as a valid 'range'.
  if(_audioCtrls == 0 || (frame + 1 > _audioCtrlRange.endValue()))
    _audioCtrlRange.setEndValue(frame + 1);

  ++_audioCtrls;
}

PosLen TagEventStatsStruct::evrange(const RelevantSelectedEvents_t& types) const
{
  // If wave events are included the result is in FRAMES.
  // Otherwise the result is in TICKS.
  PosLen pl((_waves == 0 && _audioCtrls == 0) || !(types & (WaveRelevant | AudioControllersRelevant)));
  bool first = true;
  if(types & NotesRelevant)
  {
    if(_notes > 0)
    {
      if(first || pl > _noteRange)
        pl.setPos(_noteRange);
      if(first || pl.end() < _noteRange.end())
        pl.setEnd(_noteRange.end());
      first = false;
    }
  }
  if(types & ControllersRelevant)
  {
    if(_midiCtrls > 0)
    {
      if(first || pl > _midiCtrlRange)
        pl.setPos(_midiCtrlRange);
      if(first || pl.end() < _midiCtrlRange.end())
        pl.setEnd(_midiCtrlRange.end());
      first = false;
    }
  }
  if(types & SysexRelevant)
  {
    if(_sysexes > 0)
    {
      if(first || pl > _sysexRange)
        pl.setPos(_sysexRange);
      if(first || pl.end() < _sysexRange.end())
        pl.setEnd(_sysexRange.end());
      first = false;
    }
  }
  if(types & MetaRelevant)
  {
    if(_metas > 0)
    {
      if(first || pl > _metaRange)
        pl.setPos(_metaRange);
      if(first || pl.end() < _metaRange.end())
        pl.setEnd(_metaRange.end());
      first = false;
    }
  }
  if(types & WaveRelevant)
  {
    if(_waves > 0)
    {
      if(first || pl > _waveRange)
        pl.setPos(_waveRange);
      if(first || pl.end() < _waveRange.end())
        pl.setEnd(_waveRange.end());
      first = false;
    }
  }
  if(types & AudioControllersRelevant)
  {
    if(_audioCtrls > 0)
    {
      if(first || pl > _audioCtrlRange)
        pl.setPos(_audioCtrlRange);
      if(first || pl.end() < _audioCtrlRange.end())
        pl.setEnd(_audioCtrlRange.end());
      first = false;
    }
  }

  return pl;
}

//----------------------------------
//  TagEventListStruct
//----------------------------------

TagEventListStruct::TagEventListStruct()  {}
TagEventListStruct::TagEventListStruct(const Part* part)  { _part = part; }
const Part* TagEventListStruct::part() const { return _part; }
void TagEventListStruct::setPart(const Part* part) { _part = part; }

EventList& TagEventListStruct::evlist() { return _evlist; }
const EventList& TagEventListStruct::evlist() const { return _evlist; }
AudioAutomationItemTrackMap& TagEventListStruct::aaitm() {  return _aaitm; }
const AudioAutomationItemTrackMap& TagEventListStruct::aaitm() const { return _aaitm; }

bool TagEventListStruct::add(const Event& e)
{
  //if(e.empty())
  //  return _evlist.end();
  const bool res = _evlist.add(e) != _evlist.end();
  if(res)
    _stats.add(e);
  return res;
}

bool TagEventListStruct::add(Track* track, CtrlList* ctrlList, unsigned int frame, double value)
{
  const bool res = _aaitm.addSelected(track, ctrlList, frame, AudioAutomationItem(frame, value));
  if(res)
    _stats.add(frame);
  return res;
}

const TagEventStatsStruct& TagEventListStruct::stats() const { return _stats; }

//----------------------------------
//  TagEventList
//----------------------------------

TagEventList::TagEventList() { }

bool TagEventList::add(const Part* part, const Event& event)
{
  // If the event is given, do not allow clone events to be added.
  // We allow clone parts to be in the list here just in case by
  //  some mistake an event is included in one part but not another.
  // But normally we should ignore simply if any clone part is found,
  //  because they're all supposed to be IDENTICAL, but just to be safe
  //  we'll check the event lists so we don't miss anything ...
  TagEventListStruct* found_part_el = nullptr;
  iTagEventList itl = begin();
  for( ; itl != end(); ++itl)
  {
    const Part* p = itl->part();
    // Is the event or a clone of the event already listed in this part?
    if(!event.empty())
    {
      const EventList& el = itl->evlist();
      ciEvent ie = el.findWithId(event);
      if(ie != el.cend())
        return false;
    }

    if(p == part)
      found_part_el = &(*itl);
  }

  if(found_part_el && event.empty())
    return false;

  if(!found_part_el)
  {
    iTagEventList itel = insert(end(), TagEventListStruct(part));
    found_part_el = &(*itel);
  }

  if(!event.empty())
  {
    if(!found_part_el->add(event))
      return false;
    _globalStats.add(event);
  }

  return true;
}

bool TagEventList::add(Track* track, CtrlList* ctrlList, unsigned int frame, double value)
{
  // One single tag structure (the first one found) is sufficient to hold all
  //  required values on all audio controllers on all tracks.
  if(empty())
  {
    TagEventListStruct tels;
    tels.add(track, ctrlList, frame, value);
    insert(end(), tels);
    return true;
  }
  else
  {
    iTagEventList itl = begin();
    AudioAutomationItemTrackMap& aaitm = itl->aaitm();
    return aaitm.addSelected(track, ctrlList, frame, AudioAutomationItem(frame, value));
  }
}

const TagEventStatsStruct& TagEventList::globalStats() const { return _globalStats; }

void TagEventList::globalCtlStats(FindMidiCtlsList_t* tclist, int findCtl) const
{
  for(ciTagEventList itl = cbegin(); itl != cend(); ++itl)
  {
    const TagEventListStruct& tel = *itl;
    tel.evlist().findControllers(false, tclist, findCtl);
  }
}

EventTagOptionsStruct::EventTagOptionsStruct() :
    _flags(TagDefaults) { }

EventTagOptionsStruct::EventTagOptionsStruct(const EventTagOptions_t& flags, Pos p0, Pos p1) :
    _flags(flags), _p0(p0), _p1(p1) { }

// static
EventTagOptionsStruct EventTagOptionsStruct::fromOptions(bool tagAllItems, bool tagAllParts, bool tagRange, Pos p0, Pos p1,
                      bool tagSelected, bool tagMoving)
{
  return EventTagOptionsStruct(
    (tagAllItems ? TagAllItems : TagNoOptions) |
    (tagAllParts ? TagAllParts : TagNoOptions) |
    (tagRange    ? TagRange    : TagNoOptions) |
    (tagSelected ? TagSelected : TagNoOptions) |
    (tagMoving   ? TagMoving   : TagNoOptions),
    p0, p1);
}
void EventTagOptionsStruct::clear() { _flags = TagNoOptions; _p0 = Pos(); _p1 = Pos(); }
void EventTagOptionsStruct::appendFlags(const EventTagOptions_t& flags) { _flags |= flags; }
void EventTagOptionsStruct::removeFlags(const EventTagOptions_t& flags) { _flags &= ~flags; }

} // namespace MusECore

