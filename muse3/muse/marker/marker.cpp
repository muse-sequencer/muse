//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: marker.cpp,v 1.2 2003/12/10 18:34:22 wschweer Exp $
//  (C) Copyright 2000 Werner Schweer (ws@seh.de)
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

// REMOVE Tim. clip. Added.
#include <list>

#include "marker.h"
#include "xml.h"

namespace MusECore {

// Static.
std::uint64_t Marker::_idGen = 0;

Marker* MarkerList::add(const Marker& marker)
      {
      iMarker i = insert(std::pair<const int, Marker> (marker.tick(), Marker(marker)));
      return &i->second;
      }

Marker* MarkerList::add(const QString& s, int t, bool lck)
      {
      Marker marker(s);
      marker.setType(lck ? Pos::FRAMES : Pos::TICKS);
      marker.setTick(t);
      iMarker i = insert(std::pair<const int, Marker> (t, marker));
      return &i->second;
      }

//---------------------------------------------------------
//   read
//---------------------------------------------------------

void Marker::read(Xml& xml)
      {
      for (;;) {
            Xml::Token token = xml.parse();
            const QString& tag = xml.s1();
            switch (token) {
                  case Xml::Error:
                  case Xml::End:
                        return;
                  case Xml::TagStart:
                        xml.unknown("Marker");
                        break;
                  case Xml::Attribut:
                        if (tag == "tick")
                              setTick(xml.s2().toInt());
                        else if (tag == "lock")
                              setType(xml.s2().toInt() ? FRAMES:TICKS);
                        else if (tag == "name")
                        {
                              _name = xml.s2();
                        }      
                        break;
                  case Xml::TagEnd:
                        if (xml.s1() == "marker")
                              return;
                  default:
                        break;
                  }
            }
      }

//---------------------------------------------------------
//   write
//---------------------------------------------------------

void MarkerList::write(int level, Xml& xml) const
      {
      for (ciMarker i = begin(); i != end(); ++i) {
            const Marker& m = i->second;
            xml.put(level, "<marker tick=\"%d\" lock=\"%d\" name=\"%s\" />",
               m.tick(), m.type()==Pos::FRAMES, Xml::xmlString(m.name()).toLatin1().constData());
            }
      }

//---------------------------------------------------------
//   remove
//---------------------------------------------------------

void MarkerList::remove(Marker* m)
      {
      for (iMarker i = begin(); i != end(); ++i) {
            Marker* mm = &i->second;
            if (mm == m) {
                  erase(i);
                  return;
                  }
            }
      printf("MarkerList::remove(): marker not found\n");
      }

// REMOVE Tim. clip. Added.
void MarkerList::remove(const Marker& m)
      {
      const QString& s = m.name();
      const std::uint64_t id = m.id();
      std::pair<iMarker, iMarker> rng = equal_range(m.tick());
      for(iMarker i = rng.first; i != rng.second; ++i) {
            const Marker& mm = i->second;
            if(mm.id() == id && mm.name() == s) {
                  erase(i);
                  return;
                  }
            }
      printf("MarkerList::remove(): marker not found\n");
      }

// REMOVE Tim. clip. Added.
//---------------------------------------------------------
//   rebuild
//    After any tempo changes, it is essential to rebuild the list
//     so that any 'locked' items are re-sorted properly by tick.
//    Returns true if any items were rebuilt.
//---------------------------------------------------------

bool MarkerList::rebuild()
{
  std::list<Marker> to_be_added;
  for(iMarker i = begin(); i != end(); )
  {
    const Marker& m = i->second;
    if(m.type() == Pos::FRAMES)
    {
      to_be_added.push_back(m);
      i = erase(i);
    }
    else
    {
      ++i;
    }
  }
  for(std::list<Marker>::iterator ai = to_be_added.begin(); ai != to_be_added.end(); ++ai)
  {
    const Marker& m = *ai;
    insert(std::pair<const int, Marker> (m.tick(), Marker(m)));
  }
  return !to_be_added.empty();
}

//---------------------------------------------------------
// updateCurrent
//  Sets which item is the current based on the given tick.
//---------------------------------------------------------
void MarkerList::updateCurrent(unsigned int /*tick*/)
{
  
}

} // namespace MusECore
