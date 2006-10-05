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

#ifndef __CITEM_H__
#define __CITEM_H__

#include "al/pos.h"
#include "event.h"

class Part;

//---------------------------------------------------------
//   CItem
//    Canvas Item
//---------------------------------------------------------

struct CItem {
      Event event;
      Part* part;

      bool isMoving;
      AL::Pos moving;
      int my;

      QRect  bbox;
      AL::Pos pos;

      CItem();
      CItem(const Event& e, Part* p);

      bool isSelected() const;
      void setSelected(bool f);
      bool contains(const QPoint& p) const  { return bbox.contains(p); }
      bool intersects(const QRect& r) const { return r.intersects(bbox); }
      };

typedef std::multimap<int, CItem*, std::less<int> >::iterator iCItem;
typedef std::multimap<int, CItem*, std::less<int> >::const_iterator ciCItem;
typedef std::multimap<int, CItem*, std::less<int> >::const_reverse_iterator rciCItem;

//---------------------------------------------------------
//   CItemList
//    Canvas Item List
//---------------------------------------------------------

class CItemList: public std::multimap<int, CItem*, std::less<int> > {
   public:
      void add(CItem*);
      CItem* find(const QPoint& pos) const;
      };

#endif

