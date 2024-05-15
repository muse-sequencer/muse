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
// REMOVE Tim. wave. Added.
//#include "muse_time.h"

// Forward declarations:
namespace MusECore {
class Part;
class Pos;
}

namespace MusEGui {

//---------------------------------------------------------
//   CItem
//    virtuelle Basisklasse fr alle Canvas Item's
//---------------------------------------------------------

class CItem {
   protected:
      bool _isSelected;
      bool _isMoving;

   public:
      CItem();
      virtual ~CItem();

      virtual bool isObjectInRange(const MusECore::Pos&, const MusECore::Pos&) const;
      
      bool isMoving() const;
      void setMoving(bool f);
      bool isSelected() const;
      void setSelected(bool f);
      virtual bool objectIsSelected() const;

      virtual int width() const;
      virtual void setWidth(int);
      virtual void setHeight(int);
      virtual void setMp(const QPoint&);
      virtual const QPoint mp() const;
      virtual int x() const;
      virtual int y() const;
      virtual void setY(int);
      virtual QPoint pos() const;
      virtual void setPos(const QPoint&);
      virtual int height() const;
      virtual QRect bbox() const;
      virtual void setBBox(const QRect&);
      virtual void move(const QPoint&);
      virtual void setTopLeft(const QPoint&);
      virtual bool contains(const QPoint&) const;
      virtual bool intersects(const QRect&) const;
// REMOVE Tim. wave. Added.
      // left is whether we are resizing the right or left border.
      //virtual void horizResize(int pos, bool left);
// REMOVE Tim. wave. Added.
      // Initializes the temporary values in the item. Each canvas item type can implement this accordingly.
      // For example call this at mouse-down. After that, call the canvas's adjustItemTempValues() at mouse-move.
      virtual void initItemTempValues();

// REMOVE Tim. wave. Added.
      virtual double tmpPartPos() const;
      virtual double tmpPartLen() const;
      virtual double tmpOffset() const;
      virtual double tmpPos() const;
      virtual double tmpLen() const;
      virtual double tmpWaveSPos() const;
      virtual void setTmpPartPos(const double);
      virtual void setTmpPartLen(const double);
      virtual void setTmpOffset(const double);
      virtual void setTmpPos(const double);
      virtual void setTmpLen(const double);
      virtual void setTmpWaveSPos(const double);

      virtual MusECore::Event event() const;
      virtual void setEvent(const MusECore::Event&);
      virtual MusECore::Part* part() const;
      virtual void setPart(MusECore::Part*);
      };


//---------------------------------------------------------
//   BItem
//    Boxed canvas item.
//---------------------------------------------------------

class BItem : public CItem {
   protected:
      QPoint moving;
      QRect  _bbox;
      QPoint _pos;

   public:
      BItem(const QPoint& p, const QRect& r);
      BItem();

      int width() const;
      void setWidth(int l);
      void setHeight(int l);
      void setMp(const QPoint&p);
      const QPoint mp() const;
      int x() const;
      int y() const;
      void setY(int y);
      QPoint pos() const;
      void setPos(const QPoint& p);
      int height() const;
      QRect bbox() const;
      void setBBox(const QRect& r);
      void move(const QPoint& tl);
      void setTopLeft(const QPoint &tl);
      bool contains(const QPoint& p) const;
      bool intersects(const QRect& r) const;
// REMOVE Tim. wave. Added.
      // left is whether we are resizing the right or left border.
      //virtual void horizResize(int newPos, bool left = false);
      };

//---------------------------------------------------------
//   PItem
//    Event canvas item with a boxed part only.
//---------------------------------------------------------

class PItem : public BItem {
   protected:
      MusECore::Part* _part;
// REMOVE Tim. wave. Added.
      // These temp values can be used during graphical item resizing to draw
      //  what the item WILL look like once the mouse button is released
      //  and the actual underlying operation takes place. In other words
      //  'live' redrawing as the user is resizing borders, for example.
      // The values are 'double' because fractional values may be required
      //  if sample rate or time stretch converters are active on the item.
      // Ultimately, if we could guarantee that the temp values reflect
      //  the object values even when the mouse is up, the temp values
      //  could be directly used for drawing all the time.
      // TODO: That requires some additional support in moveItem() and others.
      // So during resizing, for now the painting methods look at these temp
      //  values ONLY when the mouse is down (ie the drag mode is RESIZE),
      //  and when the mouse is up it looks at the object values.
      //
      // New part position applied when drawing.
      double _tmpPartPos;
      // New part length applied when drawing.
      double _tmpPartLen;
      // Wholesale offset applied to things in this item, such as a part's events.
      // The offset is in units of the part's time type (frames, ticks).
      double _tmpOffset;
      // New position applied when drawing a single wave event in a part for example.
      double _tmpPos;
      // New length applied when drawing a single wave event in a part for example.
      double _tmpLen;
      // New wave starting position applied when drawing a single wave event in a part for example.
      double _tmpWaveSPos;

   public:
      PItem(const QPoint& p, const QRect& r);
      PItem();
      PItem(MusECore::Part* p);

      virtual bool objectIsSelected() const;
      MusECore::Part* part() const;
      void setPart(MusECore::Part* p);

// REMOVE Tim. wave. Added.
      double tmpPartPos() const;
      double tmpPartLen() const;
      double tmpOffset() const;
      double tmpPos() const;
      double tmpLen() const;
      double tmpWaveSPos() const;
      void setTmpPartPos(const double v);
      void setTmpPartLen(const double v);
      void setTmpOffset(const double v);
      void setTmpPos(const double v);
      void setTmpLen(const double v);
      void setTmpWaveSPos(const double v);
      };
      
//---------------------------------------------------------
//   EItem
//    Event canvas item base class with a boxed event in a part.
//---------------------------------------------------------

class EItem : public PItem {
   protected:
      MusECore::Event _event;

   public:
      EItem(const QPoint& p, const QRect& r);
      EItem();
      EItem(const MusECore::Event& e, MusECore::Part* p);

      bool isObjectInRange(const MusECore::Pos&, const MusECore::Pos&) const;
      
      bool objectIsSelected() const;

      MusECore::Event event() const;
      void setEvent(const MusECore::Event& e);
      };

      
//---------------------------------------------------------
//   CItemMap
//    Canvas Item map
//---------------------------------------------------------

typedef std::multimap<int, CItem*, std::less<int> >::iterator iCItem;
typedef std::multimap<int, CItem*, std::less<int> >::const_iterator ciCItem;
typedef std::multimap<int, CItem*, std::less<int> >::const_reverse_iterator rciCItem;
typedef std::pair<iCItem, iCItem> iCItemRange;

class CItemMap: public std::multimap<int, CItem*, std::less<int> > {
   public:
      void add(CItem*);
      CItem* find(const QPoint& pos) const;
      void clearDelete();
      };

//---------------------------------------------------------
//   CItemList
//   Simple list of CItem pointers.
//---------------------------------------------------------

typedef std::list<CItem*>::iterator iCItemList;
typedef std::list<CItem*>::const_iterator ciCItemList;

class CItemList: public std::list<CItem*> {
   public:
      void add(CItem* item);
      void clearDelete();
      iCItemList find(const CItem* item);
      ciCItemList cfind(const CItem* item) const;
};

//---------------------------------------------------------
//   CItemSet
//   Simple set of unique CItem pointers.
//---------------------------------------------------------

typedef std::set<CItem*>::iterator iCItemSet;
typedef std::set<CItem*>::const_iterator ciCItemSet;

class CItemSet: public std::set<CItem*> {
   public:
      void add(CItem* item);
      void clearDelete();
};

} // namespace MusEGui

#endif

