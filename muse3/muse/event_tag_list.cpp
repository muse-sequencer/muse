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

namespace MusECore {

//----------------------------------
//  TagEventStatsStruct
//----------------------------------

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
  
PosLen TagEventStatsStruct::evrange(const RelevantSelectedEvents_t& types) const
{
  // If wave events are included the result is in FRAMES.
  // Otherwise the result is in TICKS.
  PosLen pl(_waves == 0 || !(types & WaveRelevant));
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
  return pl;
}  


//----------------------------------
//  TagEventListStruct
//----------------------------------

bool TagEventListStruct::add(const Event& e)
{
  //if(e.empty())
  //  return _evlist.end();
// REMOVE Tim. Ctrl. Changed.
//   _stats.add(e);
//   return _evlist.add(e) != _evlist.end();
  const bool res = _evlist.add(e) != _evlist.end();
  if(res)
    _stats.add(e);
  return res;
}


//----------------------------------
//  TagEventList
//----------------------------------

bool TagEventList::add(const Part* part, const Event& event)
{
  // If the event is given, do not allow clone events to be added.
  // We allow clone parts to be in the list here just in case by
  //  some mistake an event is included in one part but not another.
  // But normally we should ignore simply if any clone part is found,
  //  because they're all supposed to be IDENTICAL, but just to be safe
  //  we'll check the event lists so we don't miss anything ...
  if(!event.empty())
  {
    TagEventListStruct* found_part_el = NULL;
    iTagEventList itl = begin();
    for( ; itl != end(); ++itl)
    {
      const Part* p = itl->first;
      // Is the event or a clone of the event already listed in this part?
      const EventList& el = itl->second.evlist();
      ciEvent ie = el.findWithId(event);
      if(ie != el.cend())
        return false;

      if(p == part)
        found_part_el = &itl->second;
    }

    if(!found_part_el)
    {
            TagEventListInsertResultPair_t ires = insert(TagEventListPair_t(part, TagEventListStruct()));
      if(!ires.second)
        return false;
      found_part_el = &ires.first->second;
    }

    if(!found_part_el->add(event))
      return false;
    _globalStats.add(event);
    return true;
  }
  else
  {
    TagEventListInsertResultPair_t ires = insert(TagEventListPair_t(part, TagEventListStruct()));
    return ires.second;
  }

  return false;
}
  
void TagEventList::globalCtlStats(FindMidiCtlsList_t* tclist, int findCtl) const
{
  for(ciTagEventList itl = cbegin(); itl != cend(); ++itl)
  {
    const TagEventListStruct& tel = itl->second;
    tel.evlist().findControllers(false, tclist, findCtl);
  }
}

} // namespace MusECore
