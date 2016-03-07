//=========================================================
//  MusE
//  Linux Music Editor
//    trackinfo_layout.h
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

#ifndef __TRACKINFO_LAYOUT_H__
#define __TRACKINFO_LAYOUT_H__

#include <QLayout>

class QWidget;
class QLayoutItem;
class QSize;
class QRect;

namespace MusEGui {

class Splitter;
class ScrollBar;
class WidgetStack;

class TrackInfoLayout : public QLayout
      {
      Q_OBJECT

      bool _inSetGeometry;
      WidgetStack* _stack;
      ScrollBar* _sb;
      QLayoutItem* _stackLi;
      QLayoutItem* _sbLi;
      Splitter* _splitter;

    public:
      static const int numItems = 2;
      TrackInfoLayout(QWidget *parent, WidgetStack* stack, ScrollBar* sb, Splitter* splitter = 0);
      ~TrackInfoLayout() { clear(); }

      void addItem(QLayoutItem*) { }   // Do nothing, it's a custom layout.
      virtual Qt::Orientations expandingDirections() const { return 0; }
      virtual bool hasHeightForWidth() const { return false; }
      virtual int count() const { return numItems; }
      void clear();

      virtual QSize sizeHint() const;
      virtual QSize minimumSize() const;
      virtual QSize maximumSize() const;
      virtual void setGeometry(const QRect &rect);

      virtual QLayoutItem* itemAt(int) const;
      virtual QLayoutItem* takeAt(int);
      };
}

#endif

