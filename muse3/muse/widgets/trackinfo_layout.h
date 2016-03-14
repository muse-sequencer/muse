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
class QGridLayout;
class QLayoutItem;
class QSize;
class QRect;

namespace MusEGui {

class Splitter;
class ScrollBar;
class WidgetStack;
class ScrollScale;

//---------------------------------------------------------
//   TrackInfoLayout
//   For laying out a widget stack and scrollbar which always appears 
//    to the right of the stack instead of intruding into its space.
//   An optional Splitter will be resized when the scrollbar appears.
//---------------------------------------------------------

class TrackInfoLayout : public QLayout
      {
      Q_OBJECT

      bool _inSetGeometry;
      WidgetStack* _stack;
      ScrollBar* _sb;
      QLayoutItem* _stackLi;
      QLayoutItem* _sbLi;

      // This is not actually in the layout, but used and/or adjusted anyway.
      Splitter* _splitter;

    public:
      static const int numItems = 2;
      TrackInfoLayout(QWidget *parent, WidgetStack* stack, ScrollBar* sb, Splitter* splitter = 0);
      virtual ~TrackInfoLayout() { clear(); }

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

      
//---------------------------------------------------------
//   ArrangerCanvasLayout
//   For laying out a canvas as a last splitter widget and
//    automatically adjusting the width of its corresponding
//    horizontal scrollbar which is in another layout.
//---------------------------------------------------------
      
class ArrangerCanvasLayout : public QGridLayout
      {
      Q_OBJECT
      ScrollScale* _sb;
    public:
      ArrangerCanvasLayout(QWidget *parent, ScrollScale* sb) : QGridLayout(parent), _sb(sb) { };
      virtual void setGeometry(const QRect &rect);
      };

}

#endif

