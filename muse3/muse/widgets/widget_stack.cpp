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

#include <QWheelEvent>

namespace MusEGui {

//---------------------------------------------------------
//   WidgetStack
//---------------------------------------------------------

WidgetStack::WidgetStack(QWidget* parent, const char* name)
   : QWidget(parent)
      {
      setObjectName(name);
      top = -1;
      }

//---------------------------------------------------------
//   raiseWidget
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
            stack[idx]->show();
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
            stack[n] = w;
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

} // namespace MusEGui
