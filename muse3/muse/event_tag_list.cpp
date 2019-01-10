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

bool TagEventList::add(const Part* part, const Event* event, bool resetStartPos)
{
  if(resetStartPos)
  {
    // Reset these.
    _startPosValid = false;
    _startPos = Pos();
  }
  
  // If the event is given, do not allow clone events to be added.
  // We allow clone parts to be in the list here just in case by
  //  some mistake an event is included in one part but not another.
  // But normally we should ignore simply if any clone part is found,
  //  because they're all supposed to be IDENTICAL, but just to be safe
  //  we'll check the event lists so we don't miss anything ...
  if(event)
  {
    const Event& e = *event;
    iTagEventList itl = begin();
    for( ; itl != end(); ++itl)
    {
      const Part* p = itl->first;
      // Stop if we found the given part.
      if(p == part)
        break;
      // From here on we're looking for clone parts.
      if(!p->isCloneOf(part))
        continue;

      // Is the event already listed in this clone part?
      // FIXME TODO: Avoid duplicate events or clone events.
      const EventList& el = itl->second;
      ciEvent ie = el.findWithId(e);
      if(ie != el.end())
        return false;
    }
    
    if(!_startPosValid || e.pos() < _startPos)
    {
      _startPosValid = true;
      _startPos = e.pos();
    }

    if(itl == end())
    {
      EventList el;
      el.add(*event);
      insert(TagEventListPair_t(part, el));
    }
    else
    {
      EventList& el = itl->second;
      el.add(*event);
    }
  }
  else
  // No event was given. Do not add the part if a clone
  //  or the part itself already exists in the list.
  {
    for(iTagEventList itl = begin(); itl != end(); ++itl)
    {
      const Part* p = itl->first;
      // Is the given part already listed?
      if(p == part)
        return false;
      // Is a clone part already listed?
      if(p->isCloneOf(part))
        return false;
    }

    EventList el;
    insert(TagEventListPair_t(part, el));
  }

  return true;
}
  
Pos TagEventList::getStartPos()
{
  // Reset these.
  _startPosValid = false;
  _startPos = Pos();
  for(ciTagEventList itl = begin(); itl != end(); ++itl)
  {
    const EventList& el = itl->second;
    for(ciEvent ie = el.begin(); ie != el.end(); ++ie)
    {
      const Event& e = ie->second;
      if(!_startPosValid || e.pos() < _startPos)
      {
        _startPosValid = true;
        _startPos = e.pos();
      }
    }
  }
  return _startPos;
}
  
  
} // namespace MusECore
