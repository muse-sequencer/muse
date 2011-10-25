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

//---------------------------------------------------------
//   CItemList
//    type: Default -1 is to find any item type. Otherwise it's a specific CItem::Type. 
//---------------------------------------------------------

CItem* CItemList::find(const QPoint& pos, int type) const
      {
      rciCItem ius;
      bool usfound = false;
      // Hm, why backwards here?
      for (rciCItem i = rbegin(); i != rend(); ++i) {
            if(type != -1 && i->second->type() != type)
              continue;
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

//---------------------------------------------------------
//   CItemLayers
//    layer: Layer number or -1. Default -1 is to find in any layer. 
//    type: Default -1 is to find any item type. Otherwise it's a specific CItem::Type. 
//---------------------------------------------------------

CItem* CItemLayers::find(const QPoint& pos, int layer, int type) const
{
  if(layer == -1)
  {
    // Go from top layer down !
    for(rciCItemLayer i = rbegin(); i != rend(); ++i)
    {
      CItem* item = i->find(pos, type);
      if(item)
        return item;
    }
  }
  else
  {
    if(layer >= (int)size())
      return 0;
    return at(layer).find(pos, type);
  }
  return 0;
}


} // namespace MusEGui
