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

#include <stdio.h>

#include <QWidget>
#include <QLayoutItem>
#include <QGridLayout>
#include <QSize>
#include <QRect>
#include <QResizeEvent>

#include "trackinfo_layout.h"
#include "splitter.h"
#include "scrollbar.h"
#include "widget_stack.h"
#include "scrollscale.h"
#include "ttoolbutton.h"

namespace MusEGui {

TrackInfoLayout::TrackInfoLayout(QWidget *parent, WidgetStack* stack, ScrollBar* sb, Splitter* splitter)
                : QHBoxLayout(parent), _stack(stack), _sb(sb), _splitter(splitter)
{ 
  _inSetGeometry = false;
  setContentsMargins(0, 0, 0, 0);
  setSpacing(0);
  _sbShowPending = false;
  _stackLi = new QWidgetItem(_stack);
  _sbLi = new QWidgetItem(_sb);
  
  addItem(_stackLi);
  addItem(_sbLi);
}

TrackInfoLayout::~TrackInfoLayout() 
{ 
}

//---------------------------------------------------------
//   setGeometry
//---------------------------------------------------------

void TrackInfoLayout::setGeometry(const QRect &rect)
      {
      QHBoxLayout::setGeometry(rect);
//       return;

// TODO REMOVE Tim. Was experiment? Or not needed now?
// 
// 
//
//       if(_inSetGeometry)
//         return;
// 
//       int w = rect.width();
//       int h = rect.height();
// 
//       QSize s0;
//       QWidget* widget = _stack->visibleWidget();
//       if(widget) {
//           s0 = widget->minimumSizeHint();
//           if (!s0.isValid())   // widget has no geometry management
//                 s0 = widget->size();
//           }
//       else
//           s0 = _stack->minimumSizeHint();
//       
//       int range = s0.height() - h;
//       if (range < 0)
//             range = 0;
// //       fprintf(stderr, "TrackInfoLayout::setGeometry sb w:%d visible:%d split count:%d w:%d h:%d s0 height:%d range:%d\n",
// //                      _sb->width(), _sb->isVisible(), _splitter->count(), w, h, s0.height(), range);
//       
//       if (range)
//       {
//         _sb->blockSignals(true);
//         _sb->setMaximum(range);
//         _sb->blockSignals(false);
//       }
// 
//       const bool vis = range != 0;
//       
//       // Was a show pending and the scrollbar is now visible? Reset the pending flag.
//       if(_sbShowPending && _sb->isVisible())
//         _sbShowPending = false;
//       
//       if(_sb->isVisible() != vis)
//       {
//         if(_sb->isVisible())
//         {
//           int sw = w - _sb->width();
//           if(sw < 0)
//             sw = 0;
//           _sb->setVisible(false);
//           if(_splitter)
//           {
//             //fprintf(stderr, "TrackInfoLayout::setGeometry hide sb: split pos:%d\n", sw);
//             _inSetGeometry = true; 
//             _splitter->setPosition(1, sw);   // FIXME: Causes too wide on first startup, also when maximizing.
//             _inSetGeometry = false; 
//           }
//           _stackLi->setGeometry(QRect(0,  0,  sw, h));
//           //fprintf(stderr, "TrackInfoLayout::setGeometry hide sb: widget:%p w:%d\n", widget, sw);
//           if(widget) 
//           {
// //             QSize r(sw, y2 < s0.height() ? s0.height() : y2);
// //             //fprintf(stderr, "TrackInfoLayout::setGeometry hide sb: widget w:%d\n",
// //             //                r.width());
// //             //widget->setGeometry(0, -_sb->value(), r.width(), r.height()); 
// //             widget->setGeometry(0, 0, r.width(), r.height()); 
//             widget->move(0, 0);
//           }
//         }
//         else
//         {
//           // If an ancestor is NOT visible this will not happen until the ancestor becomes visible.
//           // Simply reading isVisible() immediately afterwards (below) would return FALSE.
//           _sb->setVisible(true);
//           _sbShowPending = true;
//           
//           const int sbw = _sb->isVisible() ? _sb->width() : _sbLi->sizeHint().width();
//           
//           if(_splitter)
//           {
//             //fprintf(stderr, "TrackInfoLayout::setGeometry show sb: split pos:%d\n",
//             //               w + sbw);
//             _inSetGeometry = true; 
//             _splitter->setPosition(1, w + sbw); // FIXME: Causes too wide on first startup, also when maximizing.
//             _inSetGeometry = false; 
//           }
//           _stackLi->setGeometry(QRect(0,  0,  w, h));  
//             //fprintf(stderr, "TrackInfoLayout::setGeometry show sb: widget:%p w:%d\n", widget, w);
// //           _stackLi->setGeometry(QRect(0,  0,  w, y2 < s0.height() ? s0.height() : y2));  
//           if(widget) 
//           {
// //             QSize r(w, y2 < s0.height() ? s0.height() : y2);
// //             widget->setGeometry(0, -_sb->value(), r.width(), r.height());
//             widget->move(0, -_sb->value());
//           }
//         }
//       }
//       else
//       {
//         int ww = w; 
//         if(_sb->isVisible() || _sbShowPending)
//           ww -= _sb->isVisible() ? _sb->width() : _sbLi->sizeHint().width();
//           
//         //fprintf(stderr, "TrackInfoLayout::setGeometry not show/hide sb: widget:%p w:%d\n", widget, ww);
//         _stackLi->setGeometry(QRect(0,  0,  ww, h));  
// //         _stackLi->setGeometry(QRect(0,  0,  ww, h < s0.height() ? s0.height() : h));
//         if(widget) 
//         {
// //           QSize r(ww, y2 < s0.height() ? s0.height() : y2);
// //           widget->setGeometry(0, -_sb->value(), r.width(), r.height()); 
//           if(_sb->isVisible() || _sbShowPending)
//             widget->move(0, -_sb->value());
//         }
//       }
//       
//       if(_sb->isVisible() || _sbShowPending)
//       {
//         const int sbw = _sb->isVisible() ? _sb->width() : _sbLi->sizeHint().width();
//         int sbx = w + (_sb->isVisible() ? -sbw : sbw); 
//         if(sbx < 0)
//           sbx = 0;
//         //fprintf(stderr, "TrackInfoLayout::setGeometry: sb visible or pending: setting _sbLi: x:%d w:%d\n", sbx, sbw);
//         _sbLi->setGeometry(QRect(sbx, 0,  sbw, h));
//       }
//       else
//       {
//         //fprintf(stderr, "TrackInfoLayout::setGeometry: sb not visible nor pending: setting _sbLi: x:%d w:%d\n", w, 0);
//         _sbLi->setGeometry(QRect(w, 0,  0, h));
//       }
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
//   ArrangerCanvasLayout
//---------------------------------------------------------

void ArrangerCanvasLayout::setGeometry(const QRect &rect) 
{ 
  QGridLayout::setGeometry(rect);
  // Tell the hbox to update as well, as if it was part of this layout.
  //_hBox->activate();
  _hBox->update();
}

//---------------------------------------------------------
//   ArrangerHScrollLayout
//---------------------------------------------------------

ArrangerHScrollLayout::ArrangerHScrollLayout(QWidget *parent, 
                      CompactToolButton* trackinfoButton, 
                      CompactToolButton* trackinfoAltButton,
                      ScrollScale* sb, 
                      QWidget* editor) 
  : QHBoxLayout(parent),
    _trackinfoButton(trackinfoButton),
    _trackinfoAltButton(trackinfoAltButton),
    _sb(sb), 
    _editor(editor),
    _trackinfoButtonLi(0),
    _trackinfoAltButtonLi(0)
{ 
  _trackinfoButtonLi = new QWidgetItem(_trackinfoButton);
  if(_trackinfoAltButton)
    _trackinfoAltButtonLi = new QWidgetItem(_trackinfoAltButton);
  _spacerLi = new QSpacerItem(0, 0);
  _sbLi = new QWidgetItem(_sb);
  
  addItem(_trackinfoButtonLi);
  if(_trackinfoAltButtonLi)
    addItem(_trackinfoAltButtonLi);
  addItem(_spacerLi);
  addItem(_sbLi);
};

ArrangerHScrollLayout::~ArrangerHScrollLayout()
{
}

void ArrangerHScrollLayout::setGeometry(const QRect &rect) 
{ 
  _trackinfoButtonLi->setGeometry(QRect(rect.x() , 
                                        rect.y(), 
                                        _trackinfoButton->sizeHint().width(), 
                                        rect.height()));
  
  if(_trackinfoAltButtonLi)
    _trackinfoAltButtonLi->setGeometry(QRect(_trackinfoButton->sizeHint().width() + spacing(), 
                                           rect.y(), 
                                           _trackinfoAltButton->sizeHint().width(), 
                                           rect.height()));
  
  const int ti_w = _trackinfoButtonLi->sizeHint().width() + spacing() + 
                   (_trackinfoAltButtonLi ? (_trackinfoAltButtonLi->sizeHint().width() + spacing()) : 0);
                   
  if(_editor->width() > 0)
  {
    _sb->setVisible(true);
    int x = _editor->x();
    
    if(x < ti_w)
      x = ti_w;
    
    int w = rect.width() - x;
    
    if(w < _sb->minimumSizeHint().width())
    {
      w = _sb->minimumSizeHint().width();
      x = rect.width() - w;
    } 
    
    QRect r(x, rect.y(), w, rect.height());
    _sbLi->setGeometry(r);
    _spacerLi->setGeometry(QRect(ti_w, rect.y(), rect.width() - ti_w - w, rect.height()));
  }
  else
  {
    _sb->setVisible(false);
    _spacerLi->setGeometry(QRect(ti_w, rect.y(), rect.width() - ti_w, rect.height()));
  }
}

//---------------------------------------------------------
//   TrackInfoWidget
//---------------------------------------------------------

TrackInfoWidget::TrackInfoWidget(QWidget* parent, Qt::WindowFlags f)
  : QWidget(parent, f)
{
  _stack = new WidgetStack(this, "trackInfoStack", WidgetStack::VisibleHint);
  _scrollBar = new ScrollBar(Qt::Vertical, true, this);
  _scrollBar->setObjectName("infoScrollBar");
  _trackInfoLayout = new TrackInfoLayout(this, _stack, _scrollBar);
  connect(_scrollBar, SIGNAL(valueChanged(int)), SLOT(scrollValueChanged(int)));
  connect(_stack, SIGNAL(redirectWheelEvent(QWheelEvent*)), _scrollBar, SLOT(redirectedWheelEvent(QWheelEvent*)));
}

void TrackInfoWidget::scrollValueChanged(int val)
{
  if(_stack->visibleWidget())
    _stack->visibleWidget()->move(0, -val);
}

void TrackInfoWidget::doResize(const QSize& newSize)
{
  if(QWidget* widget = _stack->visibleWidget())
  {
    QSize wsz = widget->minimumSizeHint();
    if(!wsz.isValid())
      wsz = widget->minimumSize();
    
    QSize sz(newSize);
    
    if(sz.width() < wsz.width())
      sz.setWidth(wsz.width());
    if(sz.height() < wsz.height())
      sz.setHeight(wsz.height());
    
    if(_scrollBar)
    {
      int range = sz.height() - height();
      if(range < 0)
        range = 0;
      if(range)
      {
        //fprintf(stderr, "TrackInfoWidget::doResize sb range:%d\n", range);
        _scrollBar->blockSignals(true);
        _scrollBar->setMaximum(range);
        _scrollBar->blockSignals(false);
      }
      const bool vis = range != 0;
   
      // We can't do this check. An ancestor might not be visible yet.
      //if(_scrollBar->isVisible() != vis)
      {
        
        //fprintf(stderr, "TrackInfoWidget::doResize before setting sb visible:%d\n", vis);
        _scrollBar->setVisible(vis);
        //fprintf(stderr, "TrackInfoWidget::doResize after setting sb visible:%d\n", vis);
        
      }
        
    }
  }
}

void TrackInfoWidget::doMove()
{
  if(QWidget* widget = _stack->visibleWidget())
  {
    if(_scrollBar->isVisible())
      widget->move(0, -_scrollBar->value());
    else
      widget->move(0, 0);
  }
}

void TrackInfoWidget::resizeEvent(QResizeEvent* e)
{
  e->ignore();
  QWidget::resizeEvent(e);
  //doResize(e->size());
  doResize(_stack->size());
  doMove();
}


void TrackInfoWidget::raiseWidget(int idx)
{
  _stack->raiseWidget(idx);
  doResize(_stack->size());
  doMove();
  _trackInfoLayout->activate();
  _trackInfoLayout->update();
}

void TrackInfoWidget::addWidget(QWidget* w, unsigned int idx)
{
  _stack->addWidget(w, idx);
  doResize(_stack->size());
}

QWidget* TrackInfoWidget::getWidget(unsigned int idx)
{
  return _stack->getWidget(idx);
}

QWidget* TrackInfoWidget::visibleWidget() const
{
  return _stack->visibleWidget();
}

int TrackInfoWidget::curIdx() const 
{
  return _stack->curIdx(); 
}


} // namespace MusEGui
