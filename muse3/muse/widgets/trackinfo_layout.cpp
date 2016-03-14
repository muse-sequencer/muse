//=========================================================
//  MusE
//  Linux Music Editor
//    trackinfo_layout.cpp
//  (C) Copyright 2016 Tim E. Real (terminator356 on sourceforge)
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

//#include <stdio.h>

#include <QWidget>
#include <QLayoutItem>
#include <QGridLayout>
#include <QSize>
#include <QRect>

#include "trackinfo_layout.h"
#include "splitter.h"
#include "scrollbar.h"
#include "widget_stack.h"
#include "scrollscale.h"

namespace MusEGui {

TrackInfoLayout::TrackInfoLayout(QWidget *parent, WidgetStack* stack, ScrollBar* sb, Splitter* splitter)
                : QLayout(parent), _stack(stack), _sb(sb), _splitter(splitter)
{ 
  _inSetGeometry = false;
  setContentsMargins(0, 0, 0, 0);
  setSpacing(-1);
  //_stack->setParent(parent);
  //_sb->setParent(parent);
  _stackLi = new QWidgetItem(_stack);
  _sbLi = new QWidgetItem(_sb);
}
      
//---------------------------------------------------------
//   setGeometry
//---------------------------------------------------------

void TrackInfoLayout::setGeometry(const QRect &rect)
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
      
      int y2 = h;

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
      }

//---------------------------------------------------------
//   sizeHint
//---------------------------------------------------------

QSize TrackInfoLayout::sizeHint() const
      {
      return QSize(150, 100);
      }

//---------------------------------------------------------
//   minimumSize
//---------------------------------------------------------

QSize TrackInfoLayout::minimumSize() const
      {
      int w = _stack->minimumSizeHint().width();
      if(_sb->isVisible())
        w += _sbLi->sizeHint().width();
      
      return QSize(w, 50);
      }

//---------------------------------------------------------
//   maximumSize
//---------------------------------------------------------

QSize TrackInfoLayout::maximumSize() const
      {
      return QSize(440, 100000);
      }

//---------------------------------------------------------
//   itemAt
//---------------------------------------------------------

QLayoutItem* TrackInfoLayout::itemAt(int i) const 
{ 
  switch(i)
  {
    case 0: return _stackLi; break;
    case 1: return _sbLi; break;
  }
  return 0;
} 

//---------------------------------------------------------
//   takeAt
//---------------------------------------------------------

QLayoutItem* TrackInfoLayout::takeAt(int i)
{
  switch(i)
  {
    case 0: return _stackLi; break;
    case 1: return _sbLi; break;
  }
  return 0;
}

//---------------------------------------------------------
//   clear
//---------------------------------------------------------

void TrackInfoLayout::clear()
{
  //delete _stackLi->widget();
  delete _stackLi;
  _stackLi = 0;
  //delete _sbLi->widget();
  delete _sbLi;
  _sbLi = 0;
}

//---------------------------------------------------------
//   ArrangerCanvasLayout
//---------------------------------------------------------

void ArrangerCanvasLayout::setGeometry(const QRect &rect) 
{ 
  QGridLayout::setGeometry(rect); _sb->setFixedWidth(rect.width()); 
}


} // namespace MusEGui
