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

#include <QString>

#include <map>

#include "xml.h"
#include "pos.h"
#include "type_defs.h"
#include "mixed_pos_list.h"

namespace MusECore {

//---------------------------------------------------------
//   Marker
//---------------------------------------------------------

class Marker : public Pos {
      static EventID_t _idGen;
      EventID_t newId() { return _idGen++; }

      EventID_t _id;
      QString _name;
      bool _current;

   public:
      Marker() : _id(newId()), _name(""), _current(false) {}
      Marker(const QString& s, bool cur = false)
         : _id(newId()), _name(s), _current(cur) {}
      // Assigns the members of the given marker to this one, EXCEPT for the ID.
      // Returns this marker.
      Marker& assign(const Marker&);
      // Creates a copy of this marker but with a new ID.
      Marker copy() const;
      void read(Xml&);
      EventID_t id() const { return _id; }
      const QString name() const     { return _name; }
      void setName(const QString& s) { _name = s;    }
      bool current() const           { return _current; }
      void setCurrent(bool f)        { _current = f; }
      };

//---------------------------------------------------------
//   MarkerList
//---------------------------------------------------------

class MarkerList : public MixedPosList_t<unsigned, Marker, std::less<unsigned> >
{
   public:
      MarkerList() : MixedPosList_t(Pos::TICKS) { }

      // Normally to be called from the audio thread only.
      Marker* add(const Marker& m);
      Marker* add(const QString& s, unsigned t, bool lck);
      void remove(Marker*);
      void remove(const Marker&);

      MarkerList::const_iterator findId(EventID_t id) const; // Slow, index t is not known
      MarkerList::iterator findId(EventID_t id);             // Slow, index t is not known

      void write(int, Xml&) const;
      };

typedef MarkerList::iterator iMarker;
typedef MarkerList::const_iterator ciMarker;

} // namespace MusECore

#endif

