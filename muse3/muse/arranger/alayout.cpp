//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: alayout.cpp,v 1.8 2004/02/28 14:58:24 wschweer Exp $
//  (C) Copyright 2002 Werner Schweer (ws@seh.de)
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

#include "alayout.h"
//#include "arranger.h"
#include "widget_stack.h"
#include "scrollbar.h"
#include "splitter.h"
//#include "header.h"
//#include "tlist.h"

#include <QWidget>
#include <QLayoutItem>
#include <QSize>
#include <QRect>

//#include <QScrollBar>
//#include <QVBoxLayout>

namespace MusEGui {

//TLLayout::TLLayout(QWidget *parent, WidgetStack* stack, ScrollBar* sb, QWidget* button, QWidget* hline, Splitter* splitter, Header* trackListHeader, TList* trackList)
//                : QLayout(parent), _stack(stack), _sb(sb), _button(button), _hline(hline), _splitter(splitter), _trackListHeader(trackListHeader), _trackList(trackList)
// TLLayout::TLLayout(QWidget *parent, WidgetStack* stack, ScrollBar* sb, QWidget* button, Splitter* splitter, Header* trackListHeader, TList* trackList)
//                 : QLayout(parent), _stack(stack), _sb(sb), _button(button), _splitter(splitter), _trackListHeader(trackListHeader), _trackList(trackList)
//TLLayout::TLLayout(QWidget *parent, WidgetStack* stack, ScrollBar* sb, QWidget* button, Splitter* splitter, QVBoxLayout* trackListLayout)
//                : QLayout(parent), _stack(stack), _sb(sb), _button(button), _splitter(splitter), _trackListLayout(trackListLayout)
TLLayout::TLLayout(QWidget *parent, WidgetStack* stack, ScrollBar* sb, Splitter* splitter)
                : QLayout(parent), _stack(stack), _sb(sb), _splitter(splitter)
{ 
  _inSetGeometry = false;
  setContentsMargins(0, 0, 0, 0);
//   setSpacing(-1);  // REMOVE Tim. Trackinfo. Changed.
  setSpacing(0); 
  //_stack->setParent(parent);
  //_sb->setParent(parent);
  //_button->setParent(parent);
  _stackLi = new QWidgetItem(_stack);
  _sbLi = new QWidgetItem(_sb);
  //_buttonLi = new QWidgetItem(_button);
  //_hlineLi = new QWidgetItem(_hline);
  //_trackListHeaderLi = new QWidgetItem(_trackListHeader);
  //_trackListLi = new QWidgetItem(_trackList);
}
      
  
  
// REMOVE Tim. Trackinfo. Removed.
// //---------------------------------------------------------
// //   wadd
// //---------------------------------------------------------
// 
// void TLLayout::wadd(int idx, QWidget* w)
//       {
//       li[idx] = new QWidgetItem(w);
//       if (idx == 0)
//             stack = (WidgetStack*)w;
//       if (idx == 1)
//             sb = (QScrollBar*)w;
//       addItem(li[idx]);
//       }

// REMOVE Tim. Trackinfo. Removed.
// #if 0 // DELETETHIS 36
// //---------------------------------------------------------
// //   TLLayoutIterator
// //---------------------------------------------------------
// 
// class TLLayoutIterator
//       {
//       int idx;
//       QList<QLayoutItem*> list;
// 
//    public:
//       TLLayoutIterator(QList<QLayoutItem*> l) : idx(0), list(l) {}
//       QLayoutItem *current()     { return idx < int(list->count()) ? list->at(idx) : 0; }
//       QLayoutItem *next()        { idx++; return current(); }
//       QLayoutItem *takeCurrent() { return list->take( idx ); }
//       };
// 
// //---------------------------------------------------------
// //   iterator
// //---------------------------------------------------------
// 
// QLayoutIterator TLLayout::iterator()
//       {
//       return QLayoutIterator(0);
//       }
// 
// void TLLayout::addItem(QLayoutItem *item)
//       {
//       ilist.append(item);
//       }
// 
// TLLayout::~TLLayout()
//       {
//       deleteAllItems();
//       }
// 
// #endif




//---------------------------------------------------------
//   setGeometry
//    perform geometry management for tracklist:
//
//         0         1         2
//   +-----------+--------+---------+
//   | Trackinfo | scroll | header 2|
//   |           |   bar  +---------+ y1
//   |     ^     |        |   ^     |
//   |           |        | <list>  |
//   |     0     |   1    |    3    |
//   +-----------+--------+---------+ y2
//   |             hline     4      |
//   +----------+-------------------+ y3
//   | button 5 |                   |
//   +----------+-------------------+
//---------------------------------------------------------

// REMOVE Tim. Trackinfo. Changed.
// void TLLayout::setGeometry(const QRect &rect)
//       {
//       int w = rect.width();
//       int h = rect.height();
// 
//       QSize s0;
//       if (stack->visibleWidget()) {
//             s0 = stack->visibleWidget()->minimumSizeHint();
//             if (!s0.isValid())   // widget has no geometry management
//                   s0 = stack->visibleWidget()->size();
//             }
//       else
//             s0 = stack->minimumSizeHint();
// 
//       QSize s1 = li[1]->sizeHint();
//       QSize s2 = li[2]->sizeHint();
//       //QSize s3 = li[3]->sizeHint(); DELETETHIS huh?
//       QSize s4 = li[4]->sizeHint();
//       QSize s5 = li[5]->sizeHint();
// 
//       int y1 = 30;  // fixed header height
//       int ah = h - s5.height() - s4.height() - y1;   // list height
//       int aw = w - s1.width() - s0.width();          // list width
// 
//       int y2 = ah + s2.height();
//       int y3 = y2 + s4.height();
//       int x1 = s0.width();
//       int x2 = x1 + s1.width();
// 
//       li[0]->setGeometry(QRect(0,  0,  s0.width(), y2));  
// 
//       QWidget* widget = stack->visibleWidget();
//       int range = s0.height() - y2;
//       if (range < 0)
//             range = 0;
// 
//       if (range)
//             sb->setMaximum(range);
// 
//       if (widget) {
//             QSize r(s0.width(), y2 < s0.height() ? s0.height() : y2);   // p4.0.11 Tim
//             widget->setGeometry(0, 0, r.width(), r.height()); 
//             }
// 
//       li[1]->setGeometry(QRect(x1, 0,  s1.width(), y2));
//       li[2]->setGeometry(QRect(x2, 0,  aw,         s2.height()));
//       li[3]->setGeometry(QRect(x2, y1, aw,        ah));
//       li[4]->setGeometry(QRect(0,  y2,  w,        s4.height()));
//       li[5]->setGeometry(QRect(3,  y3,  s5.width(), s5.height()));
//       
//       // Fix for non-appearing scrollbar. Yes, we must allow the recursive call, but try it here, not above.    p4.0.44 Tim
//       sb->setVisible(range != 0);
//       }

void TLLayout::setGeometry(const QRect &rect)
      {
      if(_inSetGeometry)
        return;

      int w = rect.width();
      int h = rect.height();

      QSize s0;
      QWidget* widget = _stack->visibleWidget();
      if(widget) {
          s0 = widget->minimumSizeHint();
          if (!s0.isValid())   // widget has no geometry management
                s0 = widget->size();
          }
      else
          s0 = _stack->minimumSizeHint();


      //QSize s2 = _trackListHeaderLi->sizeHint();
      //QSize s3 = _trackListLi->sizeHint();
      //QSize s4 = _hlineLi->sizeHint();
      //QSize s5 = _buttonLi->sizeHint();
      //QSize s5 = _button->height();
      
//       //QSize s3 = li[3]->sizeHint(); DELETETHIS huh?
//       QSize s4 = li[4]->sizeHint();
//       QSize s5 = li[5]->sizeHint();
// 
//       int y1 = 31;  // fixed header height
//       int ah = h - s5.height() - s4.height() - y1;   // list height
      //int ah = h - s5.height() - y1;                    // list height
//       int aw = w - s1.width() - s0.width();          // list width
// 
//       int y2 = ah + s2.height();
      //int y2 = h - s5.height();
      int y2 = h;
//       int y3 = y2 + s4.height();
      //int y3 = y2;
//       int x1 = s0.width();
//       int x2 = x1 + s1.width();

      
      //int y2 = h;

      int range = s0.height() - y2;
      if (range < 0)
            range = 0;
      //fprintf(stderr, "TrackInfoLayout::setGeometry h:%d s0 height:%d range:%d y2:%d\n",
      //                h, s0.height(), range, y2);
      if (range)
            _sb->setMaximum(range);

      const bool vis = range != 0;
      if(_sb->isVisible() != vis)
      {
        if(_sb->isVisible())
        {
          const int sbw = _sb->width();
          _sb->setVisible(false);
          if(_splitter)
          {
            _inSetGeometry = true; 
            _splitter->setPosition(1, w - sbw);
            _inSetGeometry = false; 
          }
          _stackLi->setGeometry(QRect(0,  0,  w - sbw, y2));  
          if(widget) 
          {
            QSize r(w - sbw, y2 < s0.height() ? s0.height() : y2);
            //fprintf(stderr, "TrackInfoLayout::setGeometry hide sb: widget w:%d\n",
            //                r.width());
            widget->setGeometry(0, -_sb->value(), r.width(), r.height()); 
          }
        }
        else
        {
          _sb->setVisible(true);
          if(_splitter)
          {
            _inSetGeometry = true; 
            _splitter->setPosition(1, w + _sb->width());
            _inSetGeometry = false; 
          }
          _stackLi->setGeometry(QRect(0,  0,  w, y2));  
          if(widget) 
          {
            QSize r(w, y2 < s0.height() ? s0.height() : y2);
            //fprintf(stderr, "TrackInfoLayout::setGeometry show sb: widget w:%d\n",
            //                r.width());
            widget->setGeometry(0, -_sb->value(), r.width(), r.height()); 
          }
        }
      }
      else
      {
        const int ww = _sb->isVisible() ?  w - _sb->width() : w;
        _stackLi->setGeometry(QRect(0,  0,  ww, y2));  
        if(widget) 
        {
          QSize r(ww, y2 < s0.height() ? s0.height() : y2);
          //fprintf(stderr, "TrackInfoLayout::setGeometry not show/hide sb: widget w:%d\n",
          //                r.width());
          widget->setGeometry(0, -_sb->value(), r.width(), r.height()); 
        }
      }
      if(widget)
        _sbLi->setGeometry(QRect(widget->width(), 0,  _sbLi->sizeHint().width(), y2));


      
//       li[1]->setGeometry(QRect(x1, 0,  s1.width(), y2));
      
//       li[2]->setGeometry(QRect(x2, 0,  aw,         s2.height()));
      //_trackListHeaderLi->setGeometry(QRect(0, 0, s2.width(), s2.height()));
      
//       li[3]->setGeometry(QRect(x2, y1, aw,        ah));
      //_trackListLi->setGeometry(QRect(0, y1, s3.width(), ah));
      
//       li[4]->setGeometry(QRect(0,  y2,  w,        s4.height()));
      
//       li[5]->setGeometry(QRect(3,  y3,  s5.width(), s5.height()));
      //_buttonLi->setGeometry(QRect(3,  y3,  s5.width(), s5.height()));
      
      //_trackListLayout->setContentsMargins(0, 0, 0, s5.height());

      }

//---------------------------------------------------------
//   sizeHint
//---------------------------------------------------------

QSize TLLayout::sizeHint() const
      {
      return QSize(150, 100);
      }

//---------------------------------------------------------
//   minimumSize
//---------------------------------------------------------

// REMOVE Tim. Trackinfo. Changed.
// QSize TLLayout::minimumSize() const
//       {
//       int w = stack->minimumSizeHint().width();
//       w += li[1]->sizeHint().width();
//       
//       return QSize(w, 50);
//       }
QSize TLLayout::minimumSize() const
      {
      int w = _stack->minimumSizeHint().width();
      if(_sb->isVisible())
        w += _sbLi->sizeHint().width();
      
      return QSize(w, 50);
      }

//---------------------------------------------------------
//   maximumSize
//---------------------------------------------------------

QSize TLLayout::maximumSize() const
      {
      return QSize(440, 100000);
      }

// REMOVE Tim. Trackinfo. Added.
//---------------------------------------------------------
//   itemAt
//---------------------------------------------------------

QLayoutItem* TLLayout::itemAt(int i) const 
{ 
  switch(i)
  {
    case 0: return _stackLi; break;
    case 1: return _sbLi; break;
    //case 2: return _buttonLi; break;
  }
  return 0;
} 

//---------------------------------------------------------
//   takeAt
//---------------------------------------------------------

// REMOVE Tim. Trackinfo. Changed.
// QLayoutItem* TLLayout::takeAt(int i)
//       {
//       if (i >= 0 && i < ilist.size())
//             return ilist.takeAt(i);
//       else
//             return 0;
//       }
QLayoutItem* TLLayout::takeAt(int i)
{
  switch(i)
  {
    case 0: return _stackLi; break;
    case 1: return _sbLi; break;
    //case 2: return _buttonLi; break;
  }
  return 0;
}
      
//---------------------------------------------------------
//   clear
//---------------------------------------------------------

// REMOVE Tim. Trackinfo. Changed.
// void TLLayout::clear()
//       {
//       QLayoutItem* child;
//       while ((child = takeAt(0)) != 0) {
//             delete child->widget();
//             delete child;
//             }
//       }
void TLLayout::clear()
{
  //delete _stackLi->widget();
  delete _stackLi;
  _stackLi = 0;
  
  //delete _sbLi->widget();
  delete _sbLi;
  _sbLi = 0;
  
  //delete _buttonLi->widget();
  //delete _buttonLi;
  //_buttonLi = 0;
}

} // namespace MusEGui
