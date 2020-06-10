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

#include <QWidget>
#include <QLayout>
#include <QGridLayout>
#include <QHBoxLayout>

class QLayoutItem;
class QSize;
class QRect;

namespace MusEGui {

class Splitter;
class ScrollBar;
class WidgetStack;
class ScrollScale;
class CompactToolButton;

//---------------------------------------------------------
//   TrackInfoLayout
//   For laying out a widget stack and scrollbar which always appears 
//    to the right of the stack instead of intruding into its space.
//   An optional Splitter will be resized when the scrollbar appears.
//---------------------------------------------------------

class TrackInfoLayout : public QHBoxLayout
      {
      Q_OBJECT

      bool _inSetGeometry;
      WidgetStack* _stack;
      ScrollBar* _sb;
      QLayoutItem* _stackLi;
      QLayoutItem* _sbLi;

      bool _sbShowPending;
      
      // This is not actually in the layout, but used and/or adjusted anyway.
      Splitter* _splitter;

    public:
      TrackInfoLayout(QWidget *parent, WidgetStack* stack, ScrollBar* sb, Splitter* splitter = 0);
      virtual ~TrackInfoLayout();

      virtual QSize sizeHint() const;
//      virtual QSize minimumSize() const;
//      virtual QSize maximumSize() const;
      
      virtual void setGeometry(const QRect &rect);
      };

//---------------------------------------------------------
//   TrackInfoWidget
//   Widget for containing a trackinfo layout.
//---------------------------------------------------------
      
class TrackInfoWidget : public QWidget
{
  Q_OBJECT
  
  private:
    WidgetStack* _stack;
    ScrollBar* _scrollBar;
    TrackInfoLayout* _trackInfoLayout;
  
   void doResize(const QSize&);
   void doMove();
   
  private slots:
    void scrollValueChanged(int);
    
  protected:
    //virtual void wheelEvent(QWheelEvent* e);
    virtual void resizeEvent(QResizeEvent*);
    
    
  public:
    TrackInfoWidget(QWidget* parent = 0, Qt::WindowFlags f = Qt::Widget);
    
    // Wrappers/catchers for stack functions:
    void raiseWidget(int idx);
    void addWidget(QWidget* w, unsigned int idx);
    QWidget* getWidget(unsigned int idx);
    QWidget* visibleWidget() const;
    int curIdx() const;
};

}

#endif

