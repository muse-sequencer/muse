//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: alayout.h,v 1.3.2.1 2008/01/19 13:33:46 wschweer Exp $
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

#ifndef __ALAYOUT_H__
#define __ALAYOUT_H__

#include <QLayout>
#include <QGridLayout>
//#include <QList>

class QLayoutItem;
//class QScrollBar;
//class QVBoxLayout;

class QWidget;

namespace MusEGui {

class WidgetStack;
class ScrollBar;
class Splitter;
//class Header;
//class TList;

//---------------------------------------------------------
//   TLLayout
//    arranger trackList layout manager
//---------------------------------------------------------

// REMOVE Tim. Trackinfo. Changed.
// class TLLayout : public QLayout
//       {
//       Q_OBJECT
// 
//       bool _inSetGeometry;
//       QList<QLayoutItem*> ilist;
//       QLayoutItem* li[6];
//       QScrollBar* sb;
//       WidgetStack* stack;
// 
//     public:
//       TLLayout(QWidget *parent) : QLayout(parent) { _inSetGeometry = false; setContentsMargins(0, 0, 0, 0); setSpacing(-1); }
//       ~TLLayout() { clear(); }
// 
//       void addItem(QLayoutItem *item) { ilist.append(item); }
//       virtual Qt::Orientations expandingDirections() const { return 0; }
//       virtual bool hasHeightForWidth() const { return false; }
//       virtual int count() const { return ilist.size(); }
//       void clear();
// 
//       void wadd(int idx, QWidget* w);
//       virtual QSize sizeHint() const;
//       virtual QSize minimumSize() const;
//       virtual QSize maximumSize() const;
//       virtual void setGeometry(const QRect &rect);
// 
//       virtual QLayoutItem* itemAt(int i) const { return ilist.value(i);} 
//       virtual QLayoutItem* takeAt(int);
//       };

class TLLayout : public QLayout
      {
      Q_OBJECT

      bool _inSetGeometry;

      WidgetStack* _stack;
      ScrollBar* _sb;
      //QWidget* _button;
      //QWidget* _hline;
      Splitter* _splitter; // This is not actually in the layout, but used and/or adjusted anyway.
      //QVBoxLayout* _trackListLayout;
      //Header* _trackListHeader;
      //TList* _trackList;
      
      QLayoutItem* _stackLi;
      QLayoutItem* _sbLi;
      //QLayoutItem* _buttonLi;
      //QLayoutItem* _hlineLi;
      //QLayoutItem* _trackListHeaderLi;
      //QLayoutItem* _trackListLi;
      
      
    public:
      static const int numItems = 2;
      //TLLayout(QWidget *parent, WidgetStack* stack, ScrollBar* sb, QWidget* button, QWidget* hline, Splitter* splitter, Header* header, TList* tlist);
      //TLLayout(QWidget *parent, WidgetStack* stack, ScrollBar* sb, QWidget* button, Splitter* splitter, Header* header, TList* tlist);
      //TLLayout(QWidget *parent, WidgetStack* stack, ScrollBar* sb, QWidget* button, Splitter* splitter, QVBoxLayout* _trackListLayout);
      TLLayout(QWidget *parent, WidgetStack* stack, ScrollBar* sb, Splitter* splitter);
      ~TLLayout() { clear(); }

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

} // namespace MusEGui

#endif
