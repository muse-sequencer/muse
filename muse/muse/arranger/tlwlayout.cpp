//=============================================================================
//  MusE
//  Linux Music Editor
//  $Id:$
//
//  Copyright (C) 2002-2006 by Werner Schweer and others
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License version 2.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//=============================================================================

#include "tlwlayout.h"
#include "arranger.h"
#include "gui.h"

//---------------------------------------------------------
//   TLWidgetLayout
//---------------------------------------------------------

TLWidgetLayout::TLWidgetLayout(QWidget *parent)
   : QLayout(parent)
      {
      setMargin(0);
      setSpacing(0);
      }

TLWidgetLayout::TLWidgetLayout()
      {
      setMargin(0);
      setSpacing(0);
      }

//---------------------------------------------------------
//   takeAt
//---------------------------------------------------------

QLayoutItem *TLWidgetLayout::takeAt(int index)
      {
      if (index >= 0 && index < itemList.size())
            return itemList.takeAt(index);
      else
            return 0;
      }

//---------------------------------------------------------
//   setGeometry
//---------------------------------------------------------

void TLWidgetLayout::setGeometry(const QRect &rect)
      {
// printf("TLWidgetLayout::setGeometry\n");
      int n = itemList.size();
      if (n < 2)
            return;

      static const int labelWidth = 50;
      int x1 = rect.x() + labelWidth;
      int x2 = rect.x() + rect.width();
      int y  = rect.y() + 1;
      int y2 = y + rect.height() - splitWidth;
      int dh = trackRowHeight;

      QLayoutItem* item = itemList.at(0);
      QSize size(item->sizeHint());
      item->setGeometry(QRect(rect.x(), rect.y(), size.width(), size.height()));

      item = itemList.at(1);
      item->setGeometry(QRect(x1 - 18, y2 - 19, 18, 18));

      int x = x1;
      for (int i = 2; i < n; ++i) {
            QLayoutItem *item = itemList.at(i);
            QSize size(item->sizeHint());

            if ((x > x1) && ((x + size.width()) > x2)) {
                  x  = x1;
                  y += dh;
                  if ((y + size.height()) > y2) {
                        for (; i < n; ++i)
                              itemList.at(i)->setGeometry(QRect(-1000, -1000, 0, 0));
                        return;
                        }
                  }
            item->setGeometry(QRect(x, y, size.width(), dh));
            x += size.width();
            }
      }

//---------------------------------------------------------
//   sizeHint
//---------------------------------------------------------

QSize TLWidgetLayout::sizeHint() const
      {
      return minimumSize();
      }

//---------------------------------------------------------
//   minimumSize
//---------------------------------------------------------

QSize TLWidgetLayout::minimumSize() const
      {
      QSize size;
      QLayoutItem *item;
      foreach (item, itemList)
            size = size.expandedTo(item->minimumSize());

      size += QSize(2*margin(), 2*margin());
      return size;
      }

//---------------------------------------------------------
//   clear
//---------------------------------------------------------

void TLWidgetLayout::clear()
      {
      QLayoutItem* child;
      while ((child = takeAt(0)) != 0) {
            delete child->widget();
            delete child;
            }
      }

