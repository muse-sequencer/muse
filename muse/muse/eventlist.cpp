//=============================================================================
//  MusE
//  Linux Music Editor
//  $Id:$
//
//  Copyright (C) 2002-2006 by Werner Schweer and others
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License version 2.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//=============================================================================

#include "event.h"
#include "al/xml.h"

//---------------------------------------------------------
//   readEventList
//---------------------------------------------------------

void EventList::read(QDomNode node, bool midi)
      {
      for (; !node.isNull(); node = node.nextSibling()) {
            QDomElement e = node.toElement();
            QString tag(e.tagName());
            if (tag.isEmpty())
                  continue;
            if (tag == "event") {
                  Event e(midi ? Note : Wave);
                  e.read(node);
                  add(e);
                  }
            else
                  printf("EventListData:read(): unknown tag %s\n", tag.toAscii().data());
            }
      }

//---------------------------------------------------------
//   add
//---------------------------------------------------------

iEvent EventList::add(Event& event, unsigned tick)
      {
      return std::multimap<unsigned, Event, std::less<unsigned> >::insert(std::pair<const unsigned, Event> (tick, event));
      }

iEvent EventList::add(Event& event)
      {
      return add(event, event.tick());
      }

//---------------------------------------------------------
//   move
//---------------------------------------------------------

void EventList::move(Event& event, unsigned tick)
      {
      iEvent i = find(event);
      erase(i);
      std::multimap<unsigned, Event, std::less<unsigned> >::insert(std::pair<const unsigned, Event> (tick, event));
      }

//---------------------------------------------------------
//   find
//---------------------------------------------------------

iEvent EventList::find(const Event& event)
      {
      EventRange range = equal_range(event.tick());
      for (iEvent i = range.first; i != range.second; ++i) {
            if (i->second == event)
                  return i;
            }
      return end();
      }

//---------------------------------------------------------
//   dump
//---------------------------------------------------------

void EventList::dump() const
      {
      for (ciEvent i = begin(); i != end(); ++i)
            i->second.dump();
      }

