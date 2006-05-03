//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: tlwlayout.cpp,v 1.21 2006/01/12 14:49:13 wschweer Exp $
//
//  (C) Copyright 2005 Werner Schweer (ws@seh.de)
//=========================================================

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
//      size = item->sizeHint();
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

