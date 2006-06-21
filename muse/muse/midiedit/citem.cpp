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

#include "part.h"
#include "citem.h"

//---------------------------------------------------------
//   CItem
//---------------------------------------------------------

CItem::CItem()
      {
      isMoving = false;
      }

CItem::CItem(const Event& e, Part* p)
      {
      event = e;
      part  = p;
      isMoving = false;
      pos   = e.pos() + *p;
      }

//---------------------------------------------------------
//   isSelected
//---------------------------------------------------------

bool CItem::isSelected() const
      {
      return event.empty() ? part->selected() : event.selected();
      }

//---------------------------------------------------------
//   setSelected
//---------------------------------------------------------

void CItem::setSelected(bool f)
      {
      event.empty() ? part->setSelected(f) : event.setSelected(f);
      }

//---------------------------------------------------------
//   CItemList
//---------------------------------------------------------

CItem* CItemList::find(const QPoint& pos) const
      {
      for (ciCItem i = begin(); i != end(); ++i) {
            if (i->second->contains(pos))
                  return i->second;
            }
      return 0;
      }

//---------------------------------------------------------
//   CItemList
//---------------------------------------------------------

void CItemList::add(CItem* item)
      {
      std::multimap<int, CItem*, std::less<int> >::insert(
         std::pair<const unsigned int, CItem*> (item->pos.tick(), item)
         );
      }

