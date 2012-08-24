//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: citem.h,v 1.2.2.1 2006/10/04 18:45:35 spamatica Exp $
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

#ifndef __CITEM_H__
#define __CITEM_H__

#include <map>
#include <QPoint>
#include <QRect>

#include "event.h"

namespace MusECore {
class Part;
}

namespace MusEGui {

//---------------------------------------------------------
//   CItem
//    virtuelle Basisklasse fr alle Canvas Item's
//---------------------------------------------------------

class CItem {
   private:
      MusECore::Event _event;
      MusECore::Part* _part;

   protected:
      bool _isMoving;
      QPoint moving;
      QRect  _bbox;
      QPoint _pos;

   public:
      CItem(const QPoint& p, const QRect& r);
      CItem();
      // Changed by Tim. p3.3.20
      //CItem(MusECore::Event e, MusECore::Part* p);
      CItem(const MusECore::Event& e, MusECore::Part* p);

      bool isMoving() const        { return _isMoving;  }
      void setMoving(bool f)       { _isMoving = f;     }
      bool isSelected() const;
      void setSelected(bool f);

      int width() const            { return _bbox.width(); }
      void setWidth(int l)         { _bbox.setWidth(l); }
      void setHeight(int l)        { _bbox.setHeight(l); }
      void setMp(const QPoint&p)   { moving = p;    }
      const QPoint mp() const      { return moving; }
      int x() const                { return _pos.x(); }
      int y() const                { return _pos.y(); }
      void setY(int y)             { _bbox.setY(y); }
      QPoint pos() const           { return _pos; }
      void setPos(const QPoint& p) { _pos = p;    }
      int height() const           { return _bbox.height(); }
      const QRect& bbox() const    { return _bbox; }
      void setBBox(const QRect& r) { _bbox = r; }
      void move(const QPoint& tl)  {
            _bbox.moveTopLeft(tl);
            _pos = tl;
            }
      bool contains(const QPoint& p) const  { return _bbox.contains(p); }
      bool intersects(const QRect& r) const { return r.intersects(_bbox); }

      MusECore::Event event() const         { return _event;  }
      void setEvent(MusECore::Event& e)     { _event = e;     }
      MusECore::Part* part() const          { return _part; }
      void setPart(MusECore::Part* p)       { _part = p; }
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
      void clearDelete() {
            for (iCItem i = begin(); i != end(); ++i)
                  delete i->second;
            clear();
            }
      };

} // namespace MusEGui

#endif

