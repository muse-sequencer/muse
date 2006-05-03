//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: citem.h,v 1.6 2006/01/27 21:12:10 wschweer Exp $
//  (C) Copyright 1999-2006 Werner Schweer (ws@seh.de)
//=========================================================

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

