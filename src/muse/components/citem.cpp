//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: citem.cpp,v 1.2.2.3 2008/01/26 07:23:21 terminator356 Exp $
//  (C) Copyright 1999 Werner Schweer (ws@seh.de)
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

#include "citem.h"
#include "undo.h"
#include "song.h"
#include <stdio.h>

// Forwards from header:
#include "part.h"
#include "pos.h"

namespace MusEGui {

//---------------------------------------------------------
//   CItem
//---------------------------------------------------------

CItem::CItem()
      {
      _isSelected = false;
      _isMoving = false;
      }

//---------------------------------------------------------
//   BItem
//---------------------------------------------------------

BItem::BItem(const QPoint&p, const QRect& r) : CItem()
      {
      _pos   = p;
      _bbox  = r;
      }

//---------------------------------------------------------
//   PItem
//---------------------------------------------------------

PItem::PItem(const QPoint& p, const QRect& r) : BItem(p, r)
{
  _part = nullptr;
}

PItem::PItem() : BItem()
{
  _part = nullptr;
}

PItem::PItem(MusECore::Part* p) : BItem()
{
  _part = p;
}

bool PItem::objectIsSelected() const
{
  return _part->selected();
}

//---------------------------------------------------------
//   EItem
//---------------------------------------------------------

EItem::EItem(const QPoint&p, const QRect& r) : PItem(p, r)
      {
      }

EItem::EItem(const MusECore::Event& e, MusECore::Part* p) : PItem(p)
      {
      _event = e;
      }

bool EItem::isObjectInRange(const MusECore::Pos& p0, const MusECore::Pos& p1) const
{
  MusECore::Pos pos = _event.pos();
  if(_part)
    pos += (*_part);
  return pos >= p0 && pos < p1;
}

//---------------------------------------------------------
//   CItemMap
//---------------------------------------------------------

CItem* CItemMap::find(const QPoint& pos) const
      {
      CItem* item = 0;
      for (rciCItem i = rbegin(); i != rend(); ++i) {
            if (i->second->contains(pos))
            {
              if(i->second->isSelected()) 
                  return i->second;
              
              else
              {
                if(!item)
                  item = i->second;    
              }  
            }      
          }
      return item;
      }

//---------------------------------------------------------
//   CItemMap
//---------------------------------------------------------

void CItemMap::add(CItem* item)
      {
      std::multimap<int, CItem*, std::less<int> >::insert(std::pair<const int, CItem*> (item->bbox().x(), item));
      }

} // namespace MusEGui
