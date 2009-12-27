//=============================================================================
//  AL
//  Audio Utility Library
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

#ifndef __MARKER_H__
#define __MARKER_H__

#include "pos.h"

namespace AL {
      class Xml;

//---------------------------------------------------------
//   Marker
//---------------------------------------------------------

class Marker : public Pos {
      QString _name;
      bool _current;

   public:
      Marker() : _current(false) {}
  	Marker(const Pos& m) : Pos(m), _current(false) {}
      Marker(const QString& s, bool cur = false)
         : _name(s), _current(cur) {}
      void read(QDomNode);
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
      Marker* add(const QString& s, const Pos&);
      void write(Xml&) const;
      void remove(Marker*);
      };

typedef std::multimap<unsigned, Marker, std::less<unsigned> >::iterator iMarker;
typedef std::multimap<unsigned, Marker, std::less<unsigned> >::const_iterator ciMarker;

} // end namespace AL

#endif

