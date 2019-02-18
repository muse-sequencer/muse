//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: marker.h,v 1.2 2003/12/15 11:41:00 wschweer Exp $
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

#ifndef __MARKER_H__
#define __MARKER_H__

#include <map>
#include <cstdint>

#include "xml.h"
#include "pos.h"

class QString;

namespace MusECore {

//---------------------------------------------------------
//   Marker
//---------------------------------------------------------

class Marker : public Pos {
      static std::uint64_t _idGen;
      std::uint64_t newId() { return _idGen++; }

      std::uint64_t _id;
      QString _name;
      bool _current;

   public:
      Marker() : _id(newId()), _name(""), _current(false) {}
      Marker(const QString& s, bool cur = false)
         : _id(newId()), _name(s), _current(cur) {}
      void read(Xml&);
      std::uint64_t id() const { return _id; }
      const QString name() const     { return _name; }
      void setName(const QString& s) { _name = s;    }
      bool current() const           { return _current; }
      void setCurrent(bool f)        { _current = f; }
      };

//---------------------------------------------------------
//   MarkerList
//---------------------------------------------------------

class MarkerList : public std::multimap<unsigned, Marker, std::less<unsigned> > {
   public:
      Marker* add(const Marker& m);
      Marker* add(const QString& s, int t, bool lck);
      void write(int, Xml&) const;
      void remove(Marker*);
      // REMOVE Tim. clip. Added.
      void remove(const Marker&);
      // REMOVE Tim. clip. Added.
      // After any tempo changes, it is essential to rebuild the list
      //  so that any 'locked' items are re-sorted properly by tick.
      // Returns true if any items were rebuilt.
      bool rebuild();
      // Sets which item is the current based on the given tick.
      void updateCurrent(unsigned int tick);
      };

// REMOVE Tim. clip. Changed.
// typedef std::multimap<unsigned, Marker, std::less<unsigned> >::iterator iMarker;
// typedef std::multimap<unsigned, Marker, std::less<unsigned> >::const_iterator ciMarker;
typedef MarkerList::iterator iMarker;
typedef MarkerList::const_iterator ciMarker;

} // namespace MusECore

#endif

