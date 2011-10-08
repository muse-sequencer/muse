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
#include <stdio.h>

namespace MusEGui {

//---------------------------------------------------------
//   CItem
//---------------------------------------------------------

CItem::CItem()
      {
      _isMoving = false;
      }

CItem::CItem(const QPoint&p, const QRect& r)
      {
      _pos   = p;
      _bbox  = r;
      _isMoving = false;
      }

// Changed by Tim. p3.3.20
//CItem::CItem(MusECore::Event e, Part* p)
CItem::CItem(const MusECore::Event& e, MusECore::Part* p)
      {
      _event = e;
      _part  = p;
      _isMoving = false;
      }

//---------------------------------------------------------
//   isSelected
//---------------------------------------------------------

bool CItem::isSelected() const
      {
      return _event.empty() ? _part->selected() : _event.selected();
      }

//---------------------------------------------------------
//   setSelected
//---------------------------------------------------------

void CItem::setSelected(bool f)
      {
      _event.empty() ? _part->setSelected(f) : _event.setSelected(f);
      }

//---------------------------------------------------------
//   CItemList
//---------------------------------------------------------

CItem* CItemList::find(const QPoint& pos) const
      {
      rciCItem ius;
      bool usfound = false;
      for (rciCItem i = rbegin(); i != rend(); ++i) {
            if (i->second->contains(pos))
            {
              if(i->second->isSelected()) 
                  return i->second;
              
              else
              {
                if(!usfound)
                { 
                  ius = i;    
                  usfound = true;
                }  
              }  
            }      
          }
      if(usfound)
        return ius->second;
      else
        return 0;
      }

//---------------------------------------------------------
//   CItemList
//---------------------------------------------------------

void CItemList::add(CItem* item)
      {
      std::multimap<int, CItem*, std::less<int> >::insert(std::pair<const int, CItem*> (item->bbox().x(), item));
      }

} // namespace MusEGui
