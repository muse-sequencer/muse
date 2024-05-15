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
//#include <stdio.h>

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

CItem::~CItem() {}

bool CItem::isObjectInRange(const MusECore::Pos&, const MusECore::Pos&) const { return false; }

bool CItem::isMoving() const        { return _isMoving;  }
void CItem::setMoving(bool f)       { _isMoving = f;     }
bool CItem::isSelected() const { return _isSelected; }
void CItem::setSelected(bool f) { _isSelected = f; }
bool CItem::objectIsSelected() const { return false; }

int CItem::width() const            { return 0; }
void CItem::setWidth(int)           { }
void CItem::setHeight(int)          { }
void CItem::setMp(const QPoint&)    { }
const QPoint CItem::mp() const      { return QPoint(); }
int CItem::x() const                { return 0; }
int CItem::y() const                { return 0; }
void CItem::setY(int)               { }
QPoint CItem::pos() const           { return QPoint(); }
void CItem::setPos(const QPoint&)   { }
int CItem::height() const           { return 0; }
QRect CItem::bbox() const           { return QRect(); }
void CItem::setBBox(const QRect&)   { }
void CItem::move(const QPoint&)     { }
void CItem::setTopLeft(const QPoint&)     { }
bool CItem::contains(const QPoint&) const  { return false; }
bool CItem::intersects(const QRect&) const { return false; }
// REMOVE Tim. wave. Added.
//void CItem::horizResize(int /*pos*/, bool /*left*/) { }

// REMOVE Tim. wave. Added.
double CItem::tmpPartPos() const { return 0; }
double CItem::tmpPartLen() const { return 0; }
double CItem::tmpOffset() const { return 0; }
double CItem::tmpPos() const { return 0; }
double CItem::tmpLen() const { return 0; }
double CItem::tmpWaveSPos() const { return 0; }
void CItem::setTmpPartPos(const double) { }
void CItem::setTmpPartLen(const double) { }
void CItem::setTmpOffset(const double) { }
void CItem::setTmpPos(const double) { }
void CItem::setTmpLen(const double) { }
void CItem::setTmpWaveSPos(const double) { }

// REMOVE Tim. wave. Added.
//---------------------------------------------------------
//   initItemTempValues
//---------------------------------------------------------

void CItem::initItemTempValues()
{
  MusECore::Part* p = part();
  MusECore::Event e = event();
  setTmpOffset(0);
  setTmpPartPos(p ? p->posValue() : 0);
  setTmpPartLen(p ? p->lenValue() : 0);
  setTmpPos(e.posValue());
  setTmpLen(e.lenValue());
  setTmpWaveSPos(e.spos());
}

MusECore::Event CItem::event() const   { return MusECore::Event();  }
void CItem::setEvent(const MusECore::Event&) { }
MusECore::Part* CItem::part() const    { return nullptr; }
void CItem::setPart(MusECore::Part*)   { }

//---------------------------------------------------------
//   BItem
//---------------------------------------------------------

BItem::BItem() : CItem() { }

BItem::BItem(const QPoint&p, const QRect& r) : CItem()
      {
      _pos   = p;
      _bbox  = r;
      }

int BItem::width() const            { return _bbox.width(); }
void BItem::setWidth(int l)         { _bbox.setWidth(l); }
void BItem::setHeight(int l)        { _bbox.setHeight(l); }
void BItem::setMp(const QPoint&p)   { moving = p;    }
const QPoint BItem::mp() const      { return moving; }
int BItem::x() const                { return _pos.x(); }
int BItem::y() const                { return _pos.y(); }
void BItem::setY(int y)             { _bbox.setY(y); }
QPoint BItem::pos() const           { return _pos; }
void BItem::setPos(const QPoint& p) { _pos = p;    }
int BItem::height() const           { return _bbox.height(); }
QRect BItem::bbox() const           { return _bbox; }
void BItem::setBBox(const QRect& r) { _bbox = r; }
void BItem::move(const QPoint& tl)  {
      _bbox.moveTopLeft(tl);
      _pos = tl;
      }
void BItem::setTopLeft(const QPoint &tl) {
    _bbox.setTopLeft(tl);
    _pos = tl;
}
bool BItem::contains(const QPoint& p) const  { return _bbox.contains(p); }
bool BItem::intersects(const QRect& r) const { return r.intersects(_bbox); }

// REMOVE Tim. wave. Added.
// void BItem::horizResize(int newPos, bool left)
// {
//   if(left)
//   {
//     const int endX = x() + width();
//     int newX = newPos;
//     if(endX - newX < 2)
//         newX = endX - 2;
//     const int dx = endX - newX;
//     setWidth(dx);
//     const QPoint mp(newX, y());
//     move(mp);
//   }
//   else
//   {
//     int dx = newPos - x();
//     if(dx < 2)
//       dx = 2;
//     setWidth(dx);
//   }
// }


//---------------------------------------------------------
//   PItem
//---------------------------------------------------------

PItem::PItem(const QPoint& p, const QRect& r) : BItem(p, r)
{
  _part = nullptr;
// REMOVE Tim. wave. Added.
  _tmpPartPos = 0;
  _tmpPartLen = 0;
  _tmpOffset = 0;
  _tmpPos = 0;
  _tmpLen = 0;
  _tmpWaveSPos = 0;
}

PItem::PItem() : BItem()
{
  _part = nullptr;
// REMOVE Tim. wave. Added.
  _tmpPartPos = 0;
  _tmpPartLen = 0;
  _tmpOffset = 0;
  _tmpPos = 0;
  _tmpLen = 0;
  _tmpWaveSPos = 0;
}

PItem::PItem(MusECore::Part* p) : BItem()
{
  _part = p;
// REMOVE Tim. wave. Added.
  _tmpPartPos = _part ? _part->posValue() : 0;
  _tmpPartLen = _part ? _part->lenValue() : 0;
  _tmpOffset = 0;
  _tmpPos = 0;
  _tmpLen = 0;
  _tmpWaveSPos = 0;
}

bool PItem::objectIsSelected() const
{
  return _part->selected();
}

MusECore::Part* PItem::part() const          { return _part; }
void PItem::setPart(MusECore::Part* p)       { _part = p; }

// REMOVE Tim. wave. Added.
double PItem::tmpPartPos() const { return _tmpPartPos; }
double PItem::tmpPartLen() const { return _tmpPartLen; }
double PItem::tmpOffset() const { return _tmpOffset; }
double PItem::tmpPos() const { return _tmpPos; }
double PItem::tmpLen() const { return _tmpLen; }
double PItem::tmpWaveSPos() const { return _tmpWaveSPos; }
void PItem::setTmpPartPos(const double v) { _tmpPartPos = v; }
void PItem::setTmpPartLen(const double v) { _tmpPartLen = v; }
void PItem::setTmpOffset(const double v) { _tmpOffset = v; }
void PItem::setTmpPos(const double v) { _tmpPos = v; }
void PItem::setTmpLen(const double v) { _tmpLen = v; }
void PItem::setTmpWaveSPos(const double v) { _tmpWaveSPos = v; }

//---------------------------------------------------------
//   EItem
//---------------------------------------------------------

EItem::EItem() : PItem() { }

EItem::EItem(const QPoint&p, const QRect& r) : PItem(p, r)
      {
      }

EItem::EItem(const MusECore::Event& e, MusECore::Part* p) : PItem(p)
      {
      _event = e;
// REMOVE Tim. wave. Added.
      // (Done by PItem.)
      //setTmpPartPos(p->posValue());
      //setTmpPartLen(p->lenValue());
      //setTmpOffset(0);
      setTmpPos(e.posValue());
      setTmpLen(e.lenValue());
      setTmpWaveSPos(e.spos());
      }

bool EItem::isObjectInRange(const MusECore::Pos& p0, const MusECore::Pos& p1) const
{
  MusECore::Pos pos = _event.pos();
  if(_part)
    pos += (*_part);
  return pos >= p0 && pos < p1;
}

bool EItem::objectIsSelected() const { return _event.selected(); }

MusECore::Event EItem::event() const               { return _event;  }
void EItem::setEvent(const MusECore::Event& e)     { _event = e;     }

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

void CItemMap::clearDelete() {
      for (iCItem i = begin(); i != end(); ++i)
            delete i->second;
      clear();
      }

//---------------------------------------------------------
//   CItemMap
//---------------------------------------------------------

void CItemMap::add(CItem* item)
      {
      std::multimap<int, CItem*, std::less<int> >::insert(std::pair<const int, CItem*> (item->bbox().x(), item));
      }


void CItemList::add(CItem* item) { push_back(item); }
void CItemList::clearDelete() {
  for(ciCItemList i = begin(); i != end(); ++i) {
    CItem* ce = *i;
    if(ce)
      delete ce;
  }
  clear();
}
iCItemList CItemList::find(const CItem* item) {
  for(iCItemList i = begin(); i != end(); ++i) {
    if(*i == item)
      return i;
  }
  return end();
}
ciCItemList CItemList::cfind(const CItem* item) const {
  for(ciCItemList i = cbegin(); i != cend(); ++i) {
    if(*i == item)
      return i;
  }
  return cend();
}


void CItemSet::add(CItem* item) { insert(item); }
void CItemSet::clearDelete() {
  for(ciCItemSet i = begin(); i != end(); ++i) {
    CItem* ce = *i;
    if(ce)
      delete ce;
  }
  clear();
}

} // namespace MusEGui
