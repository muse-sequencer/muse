//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: eventlist.cpp,v 1.7.2.3 2009/11/05 03:14:35 terminator356 Exp $
//
//  (C) Copyright 2000-2003 Werner Schweer (ws@seh.de)
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

#include "tempo.h"
#include "event.h"
#include "xml.h"

namespace MusECore {

//---------------------------------------------------------
//   readEventList
//---------------------------------------------------------

void EventList::read(Xml& xml, const char* name, bool midi)
      {
      for (;;) {
            Xml::Token token = xml.parse();
            const QString& tag = xml.s1();
            switch (token) {
                  case Xml::Error:
                  case Xml::End:
                        return;
                  case Xml::TagStart:
                        if (tag == "event") {
                              Event e(midi ? Note : Wave);
                              e.read(xml);
                              add(e);
                              }
                        else
                              xml.unknown("readEventList");
                        break;
                  case Xml::TagEnd:
                        if (tag == name)
                              return;
                  default:
                        break;
                  }
            }
      }

//---------------------------------------------------------
//   add
//---------------------------------------------------------

iEvent EventList::add(Event event)
      {
      // Changed by Tim. An event list containing wave events should be sorted by
      //  frames. WaveTrack::fetchData() relies on the sorting order, and
      //  there was a bug that waveparts were sometimes muted because of
      //  incorrect sorting order (by ticks).
      // Also, when the tempo map is changed, every wave event would have to be
      //  re-added to the event list so that the proper sorting order (by ticks)
      //  could be achieved.
      // Note that in a med file, the tempo list is loaded AFTER all the tracks.
      // There was a bug that all the wave events' tick values were not correct,
      // since they were computed BEFORE the tempo map was loaded.
      
      // From cppreference.com about the insert hint parameter (we now use C++11):
      // hint -
      // iterator, used as a suggestion as to where to start the search (until C++11)
      // iterator to the position before which the new element will be inserted (since C++11)
      //                          ------

      if(event.type() == Wave)
        return insert(std::pair<const unsigned, Event> (event.frame(), event));          

      unsigned key = event.tick();
      if(event.type() == Note)      // Place notes after controllers.
      {
        iEvent i = upper_bound(key);
        return insert(i, std::pair<const unsigned, Event> (key, event));   
      }
      else
      {
        if(event.type() == Controller)
        {
          EventRange er = equal_range(key);
          iEvent i = er.second;
          iEvent loc = i;
          const int data_a = event.dataA();
          while(i != er.first)
          {
            --i;
            // Special: There must be only ONE value per controller per position.
            // If there is already a controller value for this controller number
            //  at this position, just replace it and return.
            if(i->second.type() == Controller && i->second.dataA() == data_a)
            {
              i->second.setB(event.dataB());
              return i;
            }
            if(i->second.type() == Note)
              loc = i;
          }
          return insert(loc, std::pair<const unsigned, Event> (key, event));   
        }
        else
        {        
          iEvent i = lower_bound(key);
          while(i != end() && i->first == key && i->second.type() != Note)
            ++i;
          return insert(i, std::pair<const unsigned, Event> (key, event));   
        }
      }
      return end();
      }

//---------------------------------------------------------
//   move
//---------------------------------------------------------

void EventList::move(Event& event, unsigned tick)
      {
      iEvent i = find(event);
      erase(i);
      
      if(event.type() == Wave)
      {
        insert(std::pair<const unsigned, Event> (MusEGlobal::tempomap.tick2frame(tick), event));  
        return;
      }
      
      if(event.type() == Note)      // Place notes after controllers.
      {
        iEvent i = upper_bound(tick);
        insert(i, std::pair<const unsigned, Event> (tick, event));   
        return;
      }
      else
      {
        iEvent i = lower_bound(tick);
        while(i != end() && i->first == tick && i->second.type() != Note)
          ++i;
        insert(i, std::pair<const unsigned, Event> (tick, event));   
        return;
      }
      }

//---------------------------------------------------------
//   find
//---------------------------------------------------------

iEvent EventList::find(const Event& event)
{
      EventRange range = equal_range(event.posValue());

      for (iEvent i = range.first; i != range.second; ++i) {
            if (i->second == event)
                  return i;
            }
      return end();
}

ciEvent EventList::find(const Event& event) const
      {
      cEventRange range = equal_range(event.posValue());

      
      for (ciEvent i = range.first; i != range.second; ++i) {
            if (i->second == event)
                  return i;
            }
      return end();
      }

iEvent EventList::findSimilar(const Event& event)
{
      EventRange range = equal_range(event.posValue());

      for (iEvent i = range.first; i != range.second; ++i) {
            if (i->second.isSimilarTo(event))
                  return i;
            }
      return end();
}

ciEvent EventList::findSimilar(const Event& event) const
      {
      cEventRange range = equal_range(event.posValue());

      
      for (ciEvent i = range.first; i != range.second; ++i) {
            if (i->second.isSimilarTo(event))
                  return i;
            }
      return end();
      }

// REMOVE Tim. citem. Added.
int EventList::findSimilarType(const Event& event, EventList& list,
                              bool compareTime,
                              bool compareA, bool compareB, bool compareC,
                              bool compareWavePath, bool compareWavePos, bool compareWaveStartPos) const
{
  int cnt = 0;
  cEventRange range = compareTime ? equal_range(event.posValue()) : cEventRange(begin(), end());
  for(ciEvent i = range.first; i != range.second; ++i)
  {
    const Event& e = i->second;
    if(e.isSimilarType(event,
          false, // Do not compare time, that's handled above.
          compareA, compareB, compareC,
          compareWavePath, compareWavePos, compareWaveStartPos))
    {
      ++cnt;
      list.add(e);
    }
  }
  return cnt;
}

// REMOVE Tim. citem. Added.
// int EventList::findSimilarType(const Event& event, EventList& list,
//                               bool compareA, bool compareB, bool compareC,
//                               bool compareWavePath, bool compareWavePos, bool compareWaveStartPos) const
//       {
//       cEventRange range = equal_range(event.posValue());
// 
//       
//       for (ciEvent i = range.first; i != range.second; ++i) {
//             if (i->second.isSimilarTo(event))
//                   return i;
//             }
//       return end();
//       }

iEvent EventList::findId(const Event& event)
{
      EventRange range = equal_range(event.posValue());

      for (iEvent i = range.first; i != range.second; ++i) {
            if (i->second.id() == event.id())
                  return i;
            }
      return end();
}

ciEvent EventList::findId(const Event& event) const
      {
      cEventRange range = equal_range(event.posValue());

      
      for (ciEvent i = range.first; i != range.second; ++i) {
            if (i->second.id() == event.id())
                  return i;
            }
      return end();
      }

iEvent EventList::findId(unsigned t, EventID_t id)
{
      EventRange range = equal_range(t);
      for (iEvent i = range.first; i != range.second; ++i) {
            if (i->second.id() == id)
                  return i;
            }
      return end();
}

ciEvent EventList::findId(unsigned t, EventID_t id) const
      {
      cEventRange range = equal_range(t);
      for (ciEvent i = range.first; i != range.second; ++i) {
            if (i->second.id() == id)
                  return i;
            }
      return end();
      }

iEvent EventList::findId(EventID_t id)
{
      for (iEvent i = begin(); i != end(); ++i) {
            if (i->second.id() == id)
                  return i;
            }
      return end();
}

ciEvent EventList::findId(EventID_t id) const
      {
      for (ciEvent i = begin(); i != end(); ++i) {
            if (i->second.id() == id)
                  return i;
            }
      return end();
      }

iEvent EventList::findWithId(const Event& event)
{
      EventRange range = equal_range(event.posValue());

      for (iEvent i = range.first; i != range.second; ++i) {
            if (i->second == event || i->second.id() == event.id())
                  return i;
            }
      return end();
}

ciEvent EventList::findWithId(const Event& event) const
      {
      cEventRange range = equal_range(event.posValue());

      
      for (ciEvent i = range.first; i != range.second; ++i) {
            if (i->second == event || i->second.id() == event.id())
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

} // namespace MusECore
