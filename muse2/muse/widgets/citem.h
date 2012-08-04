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
#include <vector>
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
   ///private:
      ///MusECore::Event _event;
      ///MusECore::Part* _part;

   protected:
      bool _isMoving;
      QPoint moving;
      QRect  _bbox;
      QPoint _pos;

   public:
      //
      // Canvas Item type. When creating new CItem sub-classes, be sure to add a new type enum here.
      //
      enum Type { //TRACK=0,        // Possible future use for tracks.
                  PART=1,           // Part.
                  MEVENT=2,         // Midi event.
                  DEVENT=3,         // Drum event.
                  //WEVENT=4,       // Possible future use for waves.
                  CTRL=5            // Audio controller. Future including midi controller.
                  //MIDI_CTRL=6     // Possible use for midi controller for now, until merge with audio controller.
                };

      CItem(const QPoint& p, const QRect& r);
      CItem();
      ///CItem(const MusECore::Event& e, MusECore::Part* p);

      virtual Type type() const = 0; 
      bool isMoving() const        { return _isMoving;  }
      void setMoving(bool f)       { _isMoving = f;     }
      ///bool isSelected() const;
      ///void setSelected(bool f);
      virtual bool isSelected() const = 0;
      virtual void setSelected(bool f) = 0;

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

      ///MusECore::Event event() const         { return _event;  }
      ///void setEvent(MusECore::Event& e)     { _event = e;     }
      ///MusECore::Part* part() const          { return _part; }
      ///void setPart(MusECore::Part* p)       { _part = p; }
      };

//---------------------------------------------------------
//   MCItem
//    Base for note items and drum items
//---------------------------------------------------------

class MCItem : public CItem {
   private:
      MusECore::Event _event;
      MusECore::Part* _part;
     
   public:
      MCItem(const MusECore::Event& e, MusECore::Part* p) : _event(e) , _part(p) { } 
      //Type type() const { return MEVENT; } 
      
      bool isSelected() const   { return _event.selected(); }
      void setSelected(bool f)  { _event.setSelected(f);    }
      
      const MusECore::Event& event() const     { return _event;  }
      void setEvent(const MusECore::Event& e)  { _event = e;     }
      MusECore::Part* part() const             { return _part;   }
      void setPart(MusECore::Part* p)          { _part = p;      }
      };

//---------------------------------------------------------
//   CItemList
//    Canvas Item List
//---------------------------------------------------------

typedef std::multimap<int, CItem*, std::less<int> >::iterator iCItem;
typedef std::multimap<int, CItem*, std::less<int> >::const_iterator ciCItem;
typedef std::multimap<int, CItem*, std::less<int> >::const_reverse_iterator rciCItem;

class CItemList: public std::multimap<int, CItem*, std::less<int> > {
   public:
      void add(CItem*);
      CItem* find(const QPoint&, int type = -1) const;  
      void clearDelete() {
            for (iCItem i = begin(); i != end(); ++i)
                  delete i->second;
            clear();
            }
      };

//---------------------------------------------------------
//   CItemLayers
//    Canvas Layer Item List
//---------------------------------------------------------

typedef std::vector<CItemList>::iterator iCItemLayer;
typedef std::vector<CItemList>::const_iterator ciCItemLayer;
typedef std::vector<CItemList>::const_reverse_iterator rciCItemLayer;

class CItemLayers: public std::vector<CItemList> {
   public:
      CItem* find(const QPoint&, int layer = -1, int type = -1) const;  
      
      void clearDeleteLayers() {
            for (iCItemLayer i = begin(); i != end(); ++i)
                  i->clearDelete();
            //clear();
            }
      };

} // namespace MusEGui

#endif

