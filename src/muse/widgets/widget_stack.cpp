//=========================================================
//  MusE
//  Linux Music Editor
//    widget_stack.cpp
//  (C) Copyright 1999-2004 Werner Schweer (ws@seh.de)
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

#include "widget_stack.h"
//#include "scrollbar.h"

#include <QWheelEvent>
#include <QVBoxLayout>

namespace MusEGui {

//---------------------------------------------------------
//   WidgetStack
//---------------------------------------------------------

WidgetStack::WidgetStack(QWidget* parent, const char* name, SizeHintMode sizeHintMode)
   : QWidget(parent), _sizeHintMode(sizeHintMode)
{
    setObjectName(name);
    top = -1;
}

//---------------------------------------------------------
//  raiseWidget
//---------------------------------------------------------

void WidgetStack::raiseWidget(int idx)
      {
      if (top != -1) {
            if (stack[top])
                  stack[top]->hide();
            }
      top = idx;
      if (idx == -1)
            return;
      int n = stack.size();
      if (idx >= n)
            return;
      if (stack[idx])
      {
            resizeStack(size());
            stack[idx]->show();
      }
      }

//---------------------------------------------------------
//   addWidget
//---------------------------------------------------------

void WidgetStack::addWidget(QWidget* w, unsigned int n)
      {
      if (w)
            w->hide();
      if (stack.size() <= n )
            stack.push_back(w);
      else
      {
            stack[n] = w;
            resizeStack(size());
      }
      }

QWidget* WidgetStack::getWidget(unsigned int n)
      {
      if (stack.size() <= n )
            return 0;
      return stack[n];
      }

//---------------------------------------------------------
//   visibleWidget
//---------------------------------------------------------

QWidget* WidgetStack::visibleWidget() const
      {
      if (top != -1)
            return stack[top];
      return 0;
      }

//---------------------------------------------------------
//   minimumSizeHint
//---------------------------------------------------------

QSize WidgetStack::minimumSizeHint() const
      {
      if (top == -1)
            return (QSize(0, 0));

      QSize s(0,0);
      
      // Check if we want only the visible widget...
      if(sizeHintMode() == VisibleHint && stack[top])
      {
        QSize ss = stack[top]->minimumSizeHint();
        if (!ss.isValid())
        {
//              fprintf(stderr, "WidgetStack::minimumSizeHint: minimumSizeHint invalid, getting minimumSize\n");
              ss = stack[top]->minimumSize();
        }
//        fprintf(stderr, "WidgetStack::minimumSizeHint w:%d h:%d\n", ss.width(), ss.height());
        return ss;
      }
      
      for (unsigned int i = 0; i < stack.size(); ++i) {
            if (stack[i]) {
                  QSize ss = stack[i]->minimumSizeHint();
                  if (!ss.isValid())
                        ss = stack[i]->minimumSize();
                  s = s.expandedTo(ss);
                  }
            }

      return s;
      }

//---------------------------------------------------------
//   wheelEvent
//---------------------------------------------------------

void WidgetStack::wheelEvent(QWheelEvent* ev)
      {
      emit redirectWheelEvent(ev);
      }

void WidgetStack::resizeStack(const QSize& newSize)
{
  if(QWidget* widget = visibleWidget())
  {
    QSize wsz = widget->minimumSizeHint();
    if(!wsz.isValid())
      wsz = widget->minimumSize();
    
    QSize sz(newSize);
    if(sz.width() < wsz.width())
      sz.setWidth(wsz.width());
    if(sz.height() < wsz.height())
      sz.setHeight(wsz.height());
    
    widget->resize(sz);
  }
}
      
void WidgetStack::resizeEvent(QResizeEvent* e)
{
  e->ignore();
  QWidget::resizeEvent(e);
  resizeStack(e->size());
}

QSize WidgetStack::sizeHint() const 
{
  QSize s(0,0);
  // Check if we want only the visible widget...
  if(sizeHintMode() == VisibleHint)
  {
    if(top == -1 || !stack[top])
      return s;
    
    QSize ss = stack[top]->sizeHint();
    if(ss.isValid())
      return ss;
    else
      return s;
  } 
  
  for(unsigned int i = 0; i < stack.size(); ++i) 
  {
    if(stack[i]) 
    {
      QSize ss = stack[i]->sizeHint();
      if(ss.isValid())
        s = s.expandedTo(ss);
    }
  }
  return s;
}

      
} // namespace MusEGui
