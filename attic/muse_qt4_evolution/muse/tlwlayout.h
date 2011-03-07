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

#ifndef __TLWLAYOUT_H__
#define __TLWLAYOUT_H__

//---------------------------------------------------------
//   TLWidgetLayout
//---------------------------------------------------------

class TLWidgetLayout : public QLayout {
      QList<QLayoutItem*> itemList;

      int doLayout(const QRect& rect, bool testOnly) const;

   public:
      TLWidgetLayout(QWidget* parent);
      TLWidgetLayout();
      ~TLWidgetLayout() { clear(); }

      void addItem(QLayoutItem* item) { itemList.append(item); }
      Qt::Orientations expandingDirections() const { return 0; }
      bool hasHeightForWidth() const { return false; }
      int count() const { return itemList.size(); }
      QSize minimumSize() const;
      void setGeometry(const QRect &rect);
      QSize sizeHint() const;
      QLayoutItem *itemAt(int index) const { return itemList.value(index); }
      QLayoutItem *takeAt(int index);
      void clear();
      };

#endif

