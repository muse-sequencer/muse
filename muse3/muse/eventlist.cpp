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
// REMOVE Tim. citem. ctl. Added. It seems we must allow multiple controller events
//  if they are to be moved, dragged, and dropped.
//         if(event.type() == Controller)
//         {
//           EventRange er = equal_range(key);
//           iEvent i = er.second;
//           iEvent loc = i;
//           const int data_a = event.dataA();
//           while(i != er.first)
//           {
//             --i;
//             // Special: There must be only ONE value per controller per position.
//             // If there is already a controller value for this controller number
//             //  at this position, just replace it and return.
//             //
//             // This is meant as a last line of defense against accidental multiple
//             //  controller values at a given time. The rule of thumb when executing
//             //  add event commands is you must check beforehand whether an event
//             //  exists and tell the command system to delete it so that the undo
//             //  system can remember what was replaced.
//             // In some cases the command/undo system may do that for you.
//             // But simply relying on this low-level catch-all is not good, the undo
//             //  system won't remember what was deleted.
//             if(i->second.type() == Controller && i->second.dataA() == data_a)
//             {
//               i->second.setB(event.dataB());
//               return i;
//             }
//             if(i->second.type() == Note)
//               loc = i;
//           }
//           return insert(loc, std::pair<const unsigned, Event> (key, event));   
//         }
//         else
//         {        
          iEvent i = lower_bound(key);
          while(i != end() && i->first == key && i->second.type() != Note)
            ++i;
          return insert(i, std::pair<const unsigned, Event> (key, event));   
//         }
      }
      return end();
      }

//---------------------------------------------------------
//   move
//---------------------------------------------------------

void EventList::move(Event& event, unsigned tick)
      {
      iEvent i = find(event);
      if(i != end())
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

PosLen EventList::evrange(bool wave, RelevantSelectedEvents_t relevant, int* numEvents, int ctrlNum) const
{
  PosLen res;
  res.setType(wave ? Pos::FRAMES : Pos::TICKS);

  int e_found = 0;
  bool first_found = false;
  unsigned start_time = 0;
  unsigned end_time = 0;
  for(ciEvent ie = begin(); ie != end(); ++ie)
  {
    const Event& e = ie->second;
    // Only events of the given type are considered.
    const EventType et = e.type();
    switch(et)
    {
      case Note:
        if(wave || (relevant & NotesRelevant) == NoEventsRelevant)
          continue;
        // Don't add Note event types if they have no length.
        // Hm, it is possible for zero-length events to be
        //  accidentally present in the list. So the user should
        //  at least be allowed to cut and paste them. The EventList
        //  class will probably be correcting this condition anyway
        //  when adding the event to the list.
        //if(e.lenValue() == 0)
        //  continue;
        if(!first_found)
        {
          start_time = e.posValue();
          first_found = true;
        }
        if(e.endPosValue() > end_time)
          end_time = e.endPosValue();
        ++e_found;
      break;
      
      case Wave:
        if(!wave || (relevant & WaveRelevant) == NoEventsRelevant)
          continue;
        // Don't add Wave event types if they have no length.
        //if(e.lenValue() == 0)
        //  continue;
        if(!first_found)
        {
          start_time = e.posValue();
          first_found = true;
        }
        if(e.endPosValue() > end_time)
          end_time = e.endPosValue();
        ++e_found;
      break;
      
      case Controller:
      case Meta:
      case Sysex:
        if(wave)
          continue;
        switch(et)
        {
          case Controller:
            if((relevant & ControllersRelevant) == NoEventsRelevant)
              continue;
            if(ctrlNum >= 0 && e.dataA() != ctrlNum) 
              continue;
          break;

          case Meta:
            if((relevant & MetaRelevant) == NoEventsRelevant)
              continue;
          break;

          case Sysex:
            if((relevant & SysexRelevant) == NoEventsRelevant)
              continue;
          break;

          default:
            continue;
          break;
        }
        // For these events, even if duplicates are already found at this position,
        //  the range is still the same, which simplifies this code - go ahead and count it...
        
        if(!first_found)
        {
          start_time = e.posValue();
          first_found = true;
        }
        // For these events, minimum 1 unit time, to qualify as a valid 'range'.
        if((e.posValue() + 1) > end_time)
          end_time = e.posValue() + 1;
        ++e_found;
      break;
    }
  }

  res.setPosValue(start_time);
  res.setLenValue(end_time - start_time);
  *numEvents = e_found;
  return res;
}

void EventList::findControllers(bool wave, FindMidiCtlsList_t* outList, int findCtl) const
{
  for(ciEvent ie = cbegin(); ie != cend(); ++ie)
  {
    const Event& e = ie->second;
    const EventType et = e.type();
    switch(et)
    {
      case Note:
      case Meta:
      case Sysex:
          continue;
      break;
      
      case Wave:
        if(!wave)
          continue;
        // TODO Audio controllers.
        //list->insert( ? );
      break;
      
      case Controller:
        if(wave)
          continue;
        if(findCtl < 0 || findCtl == e.dataA())
        {
          const PosLen epl = e.posLen();
          FindMidiCtlsListInsResPair_t pres = outList->insert(FindMidiCtlsPair_t(e.dataA(), epl));
          if(!pres.second)
          {
            iFindMidiCtlsList ifml = pres.first;
            PosLen& fml = ifml->second;
            if(fml > epl)
              fml = epl;
          }
        }
      break;
    }
  }
}

} // namespace MusECore
