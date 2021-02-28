//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: scrollscale.h,v 1.2.2.3 2009/11/04 17:43:26 lunar_shuttle Exp $
//  (C) Copyright 1999 Werner Schweer (ws@seh.de)
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

#ifndef __SCROLLSCALE_H__
#define __SCROLLSCALE_H__

#include <QSlider>

class QBoxLayout;
class QLabel;
class QResizeEvent;
class QScrollBar;
class QToolButton;

namespace MusEGui {

//---------------------------------------------------------
//   ScrollScale
//---------------------------------------------------------

class ScrollScale : public QWidget {
      Q_OBJECT
    
      QSlider* scale;
      QScrollBar* scroll;
      int minVal, maxVal;
      int scaleVal, scaleMin, scaleMax;
      bool showMagFlag;
      QBoxLayout* box;
      bool noScale;
      bool pageButtons;
      int _page;
      int _pages;
      QToolButton* up;
      QToolButton* down;
      QLabel* pageNo;
      bool invers;
      double logbase;
      QToolButton *scaleUp, *scaleDown;

      virtual void resizeEvent(QResizeEvent*);

   private slots:
      void pageUp();
      void pageDown();

   public slots:
      void setPos(unsigned);
      void setPosNoLimit(unsigned); 
      void setMag(int val, int pos_offset = 0);
      void setOffset(int val);
      void setScale(int val, int pos_offset = 0);
      void stepScale(bool up);

   signals:
      void scaleChanged(int);
      void scrollChanged(int);
      void newPage(int);

   public:
      ScrollScale(int, int, int, int max, Qt::Orientation,
         QWidget*, int min = 0, bool i=false, double vv = 10.0);
      int xmag() const      { return scale->value(); }
      void setXmag(int val) { scale->setValue(val); }
      void setRange(int, int);
      void showMag(bool);
      void setNoScale(bool flag) { noScale = flag; }
      void setPageButtons(bool flag);
      void setPage(int n) { _page = n; }
      int page() const { return _page; }
      int pages() const { return _pages; }
      void setPages(int n);
      int pos() const;
      int mag() const;
      int getScaleValue() const { return scaleVal; }
      void range(int* b, int* e) const { *b = minVal; *e = maxVal; }
      int scaleMinimum() const;
      int scaleMaximum() const;
      void setScaleMinimum(int min);
      void setScaleMaximum(int max);
      void setScaleRange(int min, int max);
      
      int offset() const;
      int pos2offset(int pos) const;
      int offset2pos(int off) const;
      int mag2scale(int mag) const;
      int scale2mag(int scale) const;
      static int getQuickZoomLevel(int mag);
      static int convertQuickZoomLevelToMag(int zoomlvl);
      const static int zoomLevels = 38;
      };

} // namespace MusEGui

#endif

