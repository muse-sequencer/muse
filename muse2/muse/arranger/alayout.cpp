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
#include "arranger.h"

#include <QScrollBar>

namespace MusEGui {

//---------------------------------------------------------
//   wadd
//---------------------------------------------------------

void TLLayout::wadd(int idx, QWidget* w)
      {
      li[idx] = new QWidgetItem(w);
      if (idx == 0)
            stack = (WidgetStack*)w;
      if (idx == 1)
            sb = (QScrollBar*)w;
      addItem(li[idx]);
      }

#if 0 // DELETETHIS 36
//---------------------------------------------------------
//   TLLayoutIterator
//---------------------------------------------------------

class TLLayoutIterator
      {
      int idx;
      QList<QLayoutItem*> list;

   public:
      TLLayoutIterator(QList<QLayoutItem*> l) : idx(0), list(l) {}
      QLayoutItem *current()     { return idx < int(list->count()) ? list->at(idx) : 0; }
      QLayoutItem *next()        { idx++; return current(); }
      QLayoutItem *takeCurrent() { return list->take( idx ); }
      };

//---------------------------------------------------------
//   iterator
//---------------------------------------------------------

QLayoutIterator TLLayout::iterator()
      {
      return QLayoutIterator(0);
      }

void TLLayout::addItem(QLayoutItem *item)
      {
      ilist.append(item);
      }

TLLayout::~TLLayout()
      {
      deleteAllItems();
      }

#endif

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

void TLLayout::setGeometry(const QRect &rect)
      {
      int w = rect.width();
      int h = rect.height();

      QSize s0;
      if (stack->visibleWidget()) {
            s0 = stack->visibleWidget()->minimumSizeHint();
            if (!s0.isValid())   // widget has no geometry management
                  s0 = stack->visibleWidget()->size();
            }
      else
            s0 = stack->minimumSizeHint();

      QSize s1 = li[1]->sizeHint();
      QSize s2 = li[2]->sizeHint();
      //QSize s3 = li[3]->sizeHint(); DELETETHIS huh?
      QSize s4 = li[4]->sizeHint();
      QSize s5 = li[5]->sizeHint();

      int y1 = 30;  // fixed header height
      int ah = h - s5.height() - s4.height() - y1;   // list height
      int aw = w - s1.width() - s0.width();          // list width

      int y2 = ah + s2.height();
      int y3 = y2 + s4.height();
      int x1 = s0.width();
      int x2 = x1 + s1.width();

      li[0]->setGeometry(QRect(0,  0,  s0.width(), y2));  

      QWidget* widget = stack->visibleWidget();
      int range = s0.height() - y2;
      if (range < 0)
            range = 0;

      if (range)
            sb->setMaximum(range);

      if (widget) {
            QSize r(s0.width(), y2 < s0.height() ? s0.height() : y2);   // p4.0.11 Tim
            widget->setGeometry(0, 0, r.width(), r.height()); 
            }

      li[1]->setGeometry(QRect(x1, 0,  s1.width(), y2));
      li[2]->setGeometry(QRect(x2, 0,  aw,         s2.height()));
      li[3]->setGeometry(QRect(x2, y1, aw,        ah));
      li[4]->setGeometry(QRect(0,  y2,  w,        s4.height()));
      li[5]->setGeometry(QRect(3,  y3,  s5.width(), s5.height()));
      
      // Fix for non-appearing scrollbar. Yes, we must allow the recursive call, but try it here, not above.    p4.0.44 Tim
      sb->setVisible(range != 0);
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

QSize TLLayout::minimumSize() const
      {
      int w = stack->minimumSizeHint().width();
      w += li[1]->sizeHint().width();
      
      return QSize(w, 50);
      }

//---------------------------------------------------------
//   maximumSize
//---------------------------------------------------------

QSize TLLayout::maximumSize() const
      {
      return QSize(440, 100000);
      }

//---------------------------------------------------------
//   takeAt
//---------------------------------------------------------

QLayoutItem* TLLayout::takeAt(int i)
      {
      if (i >= 0 && i < ilist.size())
            return ilist.takeAt(i);
      else
            return 0;
      }

//---------------------------------------------------------
//   clear
//---------------------------------------------------------

void TLLayout::clear()
      {
      QLayoutItem* child;
      while ((child = takeAt(0)) != 0) {
            delete child->widget();
            delete child;
            }
      }

} // namespace MusEGui
