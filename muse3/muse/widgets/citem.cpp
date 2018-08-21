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

#include "part.h"
#include "citem.h"
#include "undo.h"
#include "song.h"
#include <stdio.h>

namespace MusEGui {

//---------------------------------------------------------
//   CItem
//---------------------------------------------------------

CItem::CItem()
      {
// REMOVE Tim. citem. Removed.
//       _part = NULL;
      _isSelected = false;
      _isMoving = false;
      }

CItem::CItem(const QPoint&p, const QRect& r)
      {
// REMOVE Tim. citem. Removed.
//       _part = NULL;  
      _pos   = p;
      _bbox  = r;
      _isSelected = false;
      _isMoving = false;
      }

// REMOVE Tim. citem. Removed.
// CItem::CItem(const MusECore::Event& e, MusECore::Part* p)
//       {
//       _event = e;
//       _part  = p;
//       _isSelected = false;
//       _isMoving = false;
//       }

// REMOVE Tim. citem. Changed.
// //---------------------------------------------------------
// //   isSelected
// //---------------------------------------------------------
// 
// bool CItem::isSelected() const
//       {
//       return _event.empty() ? _part->selected() : _event.selected();
//       }
// 
// //---------------------------------------------------------
// //   setSelected
// //---------------------------------------------------------
// 
// void CItem::setSelected(bool f)
//       {
//       _event.empty() ? _part->setSelected(f) : MusEGlobal::song->selectEvent(_event, _part, f);
//       }


//---------------------------------------------------------
//   EItem
//---------------------------------------------------------

EItem::EItem() : CItem()
      {
      _part = NULL;
      }

EItem::EItem(const QPoint&p, const QRect& r) : CItem(p, r)
      {
      _part = NULL;  
      }

EItem::EItem(const MusECore::Event& e, MusECore::Part* p) : CItem()
      {
      _event = e;
      _part  = p;
      }

void EItem::setObjectTagged(bool v)
{
  _event.setTagged(v);
  if(_part)
    _part->setEventsTagged(true);
}

bool EItem::isObjectInRange(const MusECore::Pos& p0, const MusECore::Pos& p1) const
{
  return _event.pos() >= p0 && _event.pos() < p1;
}

//---------------------------------------------------------
//   CItemList
//---------------------------------------------------------

CItem* CItemList::find(const QPoint& pos) const
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
//   CItemList
//---------------------------------------------------------

void CItemList::add(CItem* item)
      {
      std::multimap<int, CItem*, std::less<int> >::insert(std::pair<const int, CItem*> (item->bbox().x(), item));
      }

} // namespace MusEGui
