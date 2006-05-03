//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: citem.cpp,v 1.5 2006/01/07 16:29:13 wschweer Exp $
//  (C) Copyright 1999 Werner Schweer (ws@seh.de)
//=========================================================

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

