//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: meter.h,v 1.1.1.1.2.2 2009/05/03 04:14:00 terminator356 Exp $
//  redesigned by oget on 2011/08/15
//
//  (C) Copyright 2000 Werner Schweer (ws@seh.de)
//  (C) Copyright 2011 Orcan Ogetbil (ogetbilo at sf.net)
//  (C) Copyright 2011-2016 Tim E. Real (terminator356 on users DOT sourceforge DOT net)
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

#ifndef __METER_H__
#define __METER_H__

#include <QFrame>
#include <QTimer>

class QResizeEvent;
class QMouseEvent;
class QPainter;
class QPainterPath;

#include <QBitmap>

#include "sclif.h"
#include "scldraw.h"

namespace MusEGui {

class Meter : public QFrame, public ScaleIf {
    Q_OBJECT

    Q_PROPERTY(int radius READ radius WRITE setRadius)
    Q_PROPERTY(bool vu3d READ vu3d WRITE setVu3d)

    int _radius;
    int _vu3d;

   public:
      enum MeterType {DBMeter, LinMeter};
      enum ScalePos { None, Left, Right, Top, Bottom, InsideHorizontal, InsideVertical };

   private:
     QColor _primaryColor;
     QColor _bgColor;
     
   protected:
      QLinearGradient darkGradRed;
      QColor dark_red_end;
      QColor dark_red_begin;

      QLinearGradient darkGradYellow;
      QColor dark_yellow_end;
      QColor dark_yellow_center;
      QColor dark_yellow_begin;

      QLinearGradient darkGradGreen;
      QColor dark_green_end;
      QColor dark_green_begin;

      QLinearGradient lightGradRed;
      QColor light_red_end;
      QColor light_red_begin;

      QLinearGradient lightGradYellow;
      QColor light_yellow_end;
      QColor light_yellow_center;
      QColor light_yellow_begin;

      QLinearGradient lightGradGreen;
      QColor light_green_end;
      QColor light_green_begin;

      QLinearGradient maskGrad;
      QColor mask_center;
      QColor mask_edge;

      QColor separator_color;
      QColor peak_color;
//      int xrad, yrad;

      virtual void resizeEvent(QResizeEvent*);
      virtual void paintEvent(QPaintEvent*);
      virtual void mousePressEvent(QMouseEvent*);
      
      // Adjust scale so marks are not too close together.
      void adjustScale();
      
   private:
      MeterType mtype;
      Qt::Orientation _orient;
      ScalePos _scalePos;
      int _refreshRate;
      int _scaleDist;
      bool overflow;
      double val;
      double targetVal;
      double targetValStep;
      double maxVal;
      double targetMaxVal;
      double minScale, maxScale;
      int yellowScale, redScale;
      int cur_pixv, last_pixv, cur_pixmax, last_pixmax;
      bool _showText;
      QString _text;
      QRect _textRect;
      void updateText(double val);

      void drawVU(QPainter& p, const QRect&, const QPainterPath&, int);

      void scaleChange();
      
      QTimer fallingTimer;

   public slots:
      void resetPeaks();
      void setVal(double, double, bool);
      void updateTargetMeterValue();

   signals:
      void mousePress();      

   public:
      Meter(QWidget* parent, 
            MeterType type = DBMeter, 
            Qt::Orientation orient = Qt::Vertical, 
            double scaleMin = -60.0, double scaleMax = 10.0,
            ScalePos scalePos = None, 
            const QColor& primaryColor = QColor(0, 255, 0),
            ScaleDraw::TextHighlightMode textHighlightMode = ScaleDraw::TextHighlightNone,
            int refreshRate = 20);
      
      QColor primaryColor() const { return _primaryColor; }
      void setPrimaryColor(const QColor& color);
      
      void setRange(double min, double max);

      void setRefreshRate(int rate);
      
      bool showText() const { return _showText; }
      void setShowText(bool v) { _showText = v; update(); }
      
      Qt::Orientation orientation() const { return _orient; }
      void setOrientation(Qt::Orientation o) { _orient = o; update(); }
      
      virtual QSize sizeHint() const;

      int radius() const { return _radius; }
      void setRadius(int radius) { _radius = radius; }
      int vu3d() const { return _vu3d; }
      void setVu3d(int vu3d) { _vu3d = vu3d; }
      };

} // namespace MusEGui

#endif

