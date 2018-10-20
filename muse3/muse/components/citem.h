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

#include <list>
#include <map>
#include <set>
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
   protected:
// REMOVE Tim. citem. Removed.
//       MusECore::Event _event;
//       MusECore::Part* _part;
      
      bool _isSelected;
      bool _isMoving;
// REMOVE Tim. citem. Removed.
//       QPoint moving;
//       QRect  _bbox;
//       QPoint _pos;

   public:
// REMOVE Tim. citem. Removed.
//       CItem(const QPoint& p, const QRect& r);
      CItem();
// REMOVE Tim. citem. Removed.
//       CItem(const MusECore::Event& e, MusECore::Part* p);
      // REMOVE Tim. citem. Added.
      virtual ~CItem() {}

      // REMOVE Tim. citem. Added.
//       virtual bool operator<(const CItem&) const { return false; }
//       virtual bool operator==(const CItem&) const { return false; }
//       virtual bool objectEquals(const CItem&) const { return false; }
      virtual bool isObjectTagged() const { return false; }
      virtual void setObjectTagged(bool)  { }
      virtual bool isObjectInRange(const MusECore::Pos&, const MusECore::Pos&) const { return false; }
      
      bool isMoving() const        { return _isMoving;  }
      void setMoving(bool f)       { _isMoving = f;     }
// REMOVE Tim. citem. Changed.
//       bool isSelected() const;
//       void setSelected(bool f);
      bool isSelected() const { return _isSelected; }
      void setSelected(bool f) { _isSelected = f; }
      virtual bool objectIsSelected() const { return false; }

// REMOVE Tim. citem. Changed.
//       int width() const            { return _bbox.width(); }
//       void setWidth(int l)         { _bbox.setWidth(l); }
//       void setHeight(int l)        { _bbox.setHeight(l); }
//       void setMp(const QPoint&p)   { moving = p;    }
//       const QPoint mp() const      { return moving; }
//       int x() const                { return _pos.x(); }
//       int y() const                { return _pos.y(); }
//       void setY(int y)             { _bbox.setY(y); }
//       QPoint pos() const           { return _pos; }
//       void setPos(const QPoint& p) { _pos = p;    }
//       int height() const           { return _bbox.height(); }
//       const QRect& bbox() const    { return _bbox; }
//       void setBBox(const QRect& r) { _bbox = r; }
//       void move(const QPoint& tl)  {
//             _bbox.moveTopLeft(tl);
//             _pos = tl;
//             }
//       bool contains(const QPoint& p) const  { return _bbox.contains(p); }
//       bool intersects(const QRect& r) const { return r.intersects(_bbox); }
      virtual int width() const            { return 0; }
      virtual void setWidth(int)           { }
      virtual void setHeight(int)          { }
      virtual void setMp(const QPoint&)    { }
      virtual const QPoint mp() const      { return QPoint(); }
      virtual int x() const                { return 0; }
      virtual int y() const                { return 0; }
      virtual void setY(int)               { }
      virtual QPoint pos() const           { return QPoint(); }
      virtual void setPos(const QPoint&)   { }
      virtual int height() const           { return 0; }
//       virtual const QRect& bbox() const    { return QRect(); }
      virtual QRect bbox() const           { return QRect(); }
      virtual void setBBox(const QRect&)   { }
      virtual void move(const QPoint&)     { }
      virtual bool contains(const QPoint&) const  { return false; }
      virtual bool intersects(const QRect&) const { return false; }

// REMOVE Tim. citem. Removed.
//       MusECore::Event event() const         { return _event;  }
//       void setEvent(MusECore::Event& e)     { _event = e;     }
//       MusECore::Part* part() const          { return _part; }
//       void setPart(MusECore::Part* p)       { _part = p; }
      virtual MusECore::Event event() const   { return MusECore::Event();  }
      virtual void setEvent(const MusECore::Event&) { }
      virtual MusECore::Part* part() const    { return NULL; }
      virtual void setPart(MusECore::Part*)   { }
      };


// REMOVE Tim. citem. Added.
//---------------------------------------------------------
//   BItem
//    Boxed canvas item.
//---------------------------------------------------------

class BItem : public CItem {
   protected:
// REMOVE Tim. citem. Removed.
//       MusECore::Event _event;
//       MusECore::Part* _part;
      
//       bool _isSelected;
//       bool _isMoving;
      QPoint moving;
      QRect  _bbox;
      QPoint _pos;

   public:
      BItem(const QPoint& p, const QRect& r);
      BItem() { }
// REMOVE Tim. citem. Removed.
//       CItem(const MusECore::Event& e, MusECore::Part* p);
      // REMOVE Tim. citem. Added.
//       virtual ~BItem() {}

      // REMOVE Tim. citem. Added.
// //       virtual bool operator<(const CItem&) const { return false; }
// //       virtual bool operator==(const CItem&) const { return false; }
// //       virtual bool objectEquals(const CItem&) const { return false; }
//       virtual bool isObjectTagged() const { return false; }
//       virtual void setObjectTagged(bool)  { }
//       virtual bool isObjectInRange(const MusECore::Pos&, const MusECore::Pos&) const { return false; }
      
//       bool isMoving() const        { return _isMoving;  }
//       void setMoving(bool f)       { _isMoving = f;     }
// REMOVE Tim. citem. Changed.
//       bool isSelected() const;
//       void setSelected(bool f);
//       bool isSelected() const { return _isSelected; }
//       void setSelected(bool f) { _isSelected = f; }
//       virtual bool objectIsSelected() const { return false; }

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
//       const QRect& bbox() const    { return _bbox; }
      QRect bbox() const           { return _bbox; }
      void setBBox(const QRect& r) { _bbox = r; }
      void move(const QPoint& tl)  {
            _bbox.moveTopLeft(tl);
            _pos = tl;
            }
      bool contains(const QPoint& p) const  { return _bbox.contains(p); }
      bool intersects(const QRect& r) const { return r.intersects(_bbox); }

// REMOVE Tim. citem. Removed.
// //       MusECore::Event event() const         { return _event;  }
// //       void setEvent(MusECore::Event& e)     { _event = e;     }
// //       MusECore::Part* part() const          { return _part; }
// //       void setPart(MusECore::Part* p)       { _part = p; }
//       virtual MusECore::Event event() const   { return MusECore::Event();  }
//       //virtual void setEvent(MusECore::Event&) { }
//       virtual MusECore::Part* part() const    { return NULL; }
//       //virtual void setPart(MusECore::Part*)   { }
      };

// REMOVE Tim. citem. Added.
//---------------------------------------------------------
//   PItem
//    Event canvas item with a boxed part only.
//---------------------------------------------------------

class PItem : public BItem {
   protected:
//       MusECore::Event _event;
      MusECore::Part* _part;
      
//       bool _isSelected;
//       bool _isMoving;
//       QPoint moving;
//       QRect  _bbox;
//       QPoint _pos;

   public:
      PItem(const QPoint& p, const QRect& r);
      PItem();
      PItem(MusECore::Part* p);
      // REMOVE Tim. citem. Added.
      //virtual ~EItem() {}

      // REMOVE Tim. citem. Added.
//       bool operator<(const CItem& i) const { return _bbox.x() < i.bbox().x(); }
//       bool operator==(const CItem& i) const { return _bbox.x() < i.bbox().x(); }
//       bool objectEquals(const CItem& i) const { return _part == i.part() && _event == i.event(); }
//       bool isObjectInRange(const MusECore::Pos&, const MusECore::Pos&) const;
      virtual bool objectIsSelected() const;
      virtual bool isObjectTagged() const;
      virtual void setObjectTagged(bool v);
      
//       bool isMoving() const        { return _isMoving;  }
//       void setMoving(bool f)       { _isMoving = f;     }
//       bool isSelected() const { return _isSelected; }
//       void setSelected(bool f) { _isSelected = f; }

//       int width() const            { return _bbox.width(); }
//       void setWidth(int l)         { _bbox.setWidth(l); }
//       void setHeight(int l)        { _bbox.setHeight(l); }
//       void setMp(const QPoint&p)   { moving = p;    }
//       const QPoint mp() const      { return moving; }
//       int x() const                { return _pos.x(); }
//       int y() const                { return _pos.y(); }
//       void setY(int y)             { _bbox.setY(y); }
//       QPoint pos() const           { return _pos; }
//       void setPos(const QPoint& p) { _pos = p;    }
//       int height() const           { return _bbox.height(); }
//       const QRect& bbox() const    { return _bbox; }
//       void setBBox(const QRect& r) { _bbox = r; }
//       void move(const QPoint& tl)  {
//             _bbox.moveTopLeft(tl);
//             _pos = tl;
//             }
//       bool contains(const QPoint& p) const  { return _bbox.contains(p); }
//       bool intersects(const QRect& r) const { return r.intersects(_bbox); }

//       MusECore::Event event() const         { return _event;  }
//       void setEvent(MusECore::Event& e)     { _event = e;     }
      MusECore::Part* part() const          { return _part; }
      void setPart(MusECore::Part* p)       { _part = p; }
      };
      
      
// REMOVE Tim. citem. Added.
//---------------------------------------------------------
//   EItem
//    Event canvas item base class with a boxed event in a part.
//---------------------------------------------------------

class EItem : public PItem {
   protected:
      MusECore::Event _event;
//       MusECore::Part* _part;
      
//       bool _isSelected;
//       bool _isMoving;
//       QPoint moving;
//       QRect  _bbox;
//       QPoint _pos;

   public:
      EItem(const QPoint& p, const QRect& r);
      EItem() { }
      EItem(const MusECore::Event& e, MusECore::Part* p);
      // REMOVE Tim. citem. Added.
      //virtual ~EItem() {}

      // REMOVE Tim. citem. Added.
//       bool operator<(const CItem& i) const { return _bbox.x() < i.bbox().x(); }
//       bool operator==(const CItem& i) const { return _bbox.x() < i.bbox().x(); }
//       bool objectEquals(const CItem& i) const { return _part == i.part() && _event == i.event(); }
      bool isObjectTagged() const { return _event.tagged(); }
      void setObjectTagged(bool v);
      bool isObjectInRange(const MusECore::Pos&, const MusECore::Pos&) const;
      
//       bool isMoving() const        { return _isMoving;  }
//       void setMoving(bool f)       { _isMoving = f;     }
//       bool isSelected() const { return _isSelected; }
//       void setSelected(bool f) { _isSelected = f; }
      bool objectIsSelected() const { return _event.selected(); }

//       int width() const            { return _bbox.width(); }
//       void setWidth(int l)         { _bbox.setWidth(l); }
//       void setHeight(int l)        { _bbox.setHeight(l); }
//       void setMp(const QPoint&p)   { moving = p;    }
//       const QPoint mp() const      { return moving; }
//       int x() const                { return _pos.x(); }
//       int y() const                { return _pos.y(); }
//       void setY(int y)             { _bbox.setY(y); }
//       QPoint pos() const           { return _pos; }
//       void setPos(const QPoint& p) { _pos = p;    }
//       int height() const           { return _bbox.height(); }
//       const QRect& bbox() const    { return _bbox; }
//       void setBBox(const QRect& r) { _bbox = r; }
//       void move(const QPoint& tl)  {
//             _bbox.moveTopLeft(tl);
//             _pos = tl;
//             }
//       bool contains(const QPoint& p) const  { return _bbox.contains(p); }
//       bool intersects(const QRect& r) const { return r.intersects(_bbox); }

      MusECore::Event event() const               { return _event;  }
      void setEvent(const MusECore::Event& e)     { _event = e;     }
//       MusECore::Part* part() const          { return _part; }
//       void setPart(MusECore::Part* p)       { _part = p; }
      };

      
//---------------------------------------------------------
//   CItemMap
//    Canvas Item List
//---------------------------------------------------------

typedef std::multimap<int, CItem*, std::less<int> >::iterator iCItem;
typedef std::multimap<int, CItem*, std::less<int> >::const_iterator ciCItem;
typedef std::multimap<int, CItem*, std::less<int> >::const_reverse_iterator rciCItem;
typedef std::pair<iCItem, iCItem> iCItemRange;

class CItemMap: public std::multimap<int, CItem*, std::less<int> > {
   public:
      void add(CItem*);
      CItem* find(const QPoint& pos) const;
      void clearDelete() {
            for (iCItem i = begin(); i != end(); ++i)
                  delete i->second;
            clear();
            }
      };

//---------------------------------------------------------
//   CItemList
//   Simple list of CItem pointers.
//---------------------------------------------------------

typedef std::list<CItem*>::iterator iCItemList;
typedef std::list<CItem*>::const_iterator ciCItemList;

class CItemList: public std::list<CItem*> {
   public:
      void add(CItem* item) { push_back(item); }
      void clearDelete() {
        for(ciCItemList i = begin(); i != end(); ++i) {
          CItem* ce = *i;
          if(ce)
            delete ce;
        }
        clear();
      }
};

//---------------------------------------------------------
//   CItemSet
//   Simple set of unique CItem pointers.
//---------------------------------------------------------

typedef std::set<CItem*>::iterator iCItemSet;
typedef std::set<CItem*>::const_iterator ciCItemSet;

class CItemSet: public std::set<CItem*> {
   public:
      void add(CItem* item) { insert(item); }
      void clearDelete() {
        for(ciCItemSet i = begin(); i != end(); ++i) {
          CItem* ce = *i;
          if(ce)
            delete ce;
        }
        clear();
      }
};

} // namespace MusEGui

#endif

