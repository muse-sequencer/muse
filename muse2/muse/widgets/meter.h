//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: meter.h,v 1.1.1.1.2.2 2009/05/03 04:14:00 terminator356 Exp $
//  redesigned by oget on 2011/08/15
//
//  (C) Copyright 2000 Werner Schweer (ws@seh.de)
//  (C) Copyright 2011 Orcan Ogetbil (ogetbilo at sf.net)
//  (C) Copyright 2011 Tim E. Real (terminator356 on users DOT sourceforge DOT net)
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

class QResizeEvent;
class QMouseEvent;
class QPainter;
class QPainterPath;

namespace MusEGui {

class Meter : public QFrame {
    Q_OBJECT
   public:
      enum MeterType {DBMeter, LinMeter};

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

      QColor separator_color;;
      QColor peak_color;
      int xrad, yrad;

      virtual void resizeEvent(QResizeEvent*);
      virtual void paintEvent(QPaintEvent*);
      virtual void mousePressEvent(QMouseEvent*);
      
   private:
      MeterType mtype;
      bool overflow;
      double val;
      double maxVal;
      double minScale, maxScale;
      int yellowScale, redScale;
      int cur_yv, last_yv, cur_ymax, last_ymax;

      void drawVU(QPainter& p, const QRect&, const QPainterPath&, int);

   public slots:
      void resetPeaks();
      void setVal(double, double, bool);

   signals:
      void mousePress();

   public:
      Meter(QWidget* parent, MeterType type = DBMeter);
      void setRange(double min, double max);
      };

} // namespace MusEGui

#endif

