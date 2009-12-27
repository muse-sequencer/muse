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

#include "marker.h"
#include "xml.h"

namespace AL {

//---------------------------------------------------------
//   add
//---------------------------------------------------------

Marker* MarkerList::add(const Marker& marker)
      {
      iMarker i = insert(std::pair<const int, Marker> (marker.tick(), Marker(marker)));
      return &i->second;
      }

Marker* MarkerList::add(const QString& s, const Pos& pos)
      {
      Marker marker(pos);
  	marker.setName(s);
      iMarker i = insert(std::pair<const int, Marker> (pos.tick(), marker));
      return &i->second;
      }

//---------------------------------------------------------
//   read
//---------------------------------------------------------

void Marker::read(QDomNode node)
      {
  	Pos::read(node);
      QDomElement e = node.toElement();
      _name = e.attribute("name");
      }

//---------------------------------------------------------
//   write
//---------------------------------------------------------

void MarkerList::write(Xml& xml) const
      {
      for (ciMarker i = begin(); i != end(); ++i) {
            const Marker& m = i->second;
      	if (m.type() == TICKS)
            	xml.tagE(QString("marker tick=\"%1\" name=\"%2\"").arg(m.tick()).arg(m.name()));
      	else
            	xml.tagE(QString("marker sample=\"%1\" name=\"%2\"").arg(m.frame()).arg(m.name()));
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
}

