//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: meter.h,v 1.1.1.1.2.2 2009/05/03 04:14:00 terminator356 Exp $
//  redesigned by oget on 2011/08/15
//
//  (C) Copyright 2000 Werner Schweer (ws@seh.de)
//  (C) Copyright 2011 Orcan Ogetbil (ogetbilo at sf.net)
//  (C) Copyright 2011-2023 Tim E. Real (terminator356 on users DOT sourceforge DOT net)
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
#include <QResizeEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QBitmap>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QSpacerItem>
#include <QSize>
#include <QMargins>

#include "sclif.h"
#include "scldraw.h"

namespace MusEGui {

// -----------------------------------------------
//   MeterLayout:
//   Convenience class that can align the ends of meters
//    with class Slider scale end points, for example.
// -----------------------------------------------

class MeterLayout : public QVBoxLayout {
    Q_OBJECT

    QHBoxLayout* _hlayout;

  public:
    MeterLayout(QWidget* parent = nullptr);
    // This is the horizontal layout where meters can be added.
    QHBoxLayout* hlayout();
};

// -----------------------------------------------
//   Meter:
// -----------------------------------------------

class Meter : public QFrame, public ScaleIf {
    Q_OBJECT

    Q_PROPERTY(int radius READ radius WRITE setRadius)
    Q_PROPERTY(bool vu3d READ vu3d WRITE setVu3d)

    int _radius;
    int _vu3d;

   public:
      enum ScalePos { ScaleNone, ScaleLeftOrTop, ScaleRightOrBottom, ScaleInside };

   private:
     QColor _primaryColor;
     QColor _bgColor;
     bool _frame;
     QColor _frameColor;
     
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

      virtual void resizeEvent(QResizeEvent*);
      virtual void paintEvent(QPaintEvent*);
      virtual void mousePressEvent(QMouseEvent*);
      
      // Adjust scale so marks are not too close together.
      void adjustScale();
      
   private:
      bool _isLog;
      bool _isInteger;
      bool _logCanZero;
      double _dBFactor, _dBFactorInv, _logFactor;
      QSize _VUSizeHint;
      Qt::Orientation _orient;
      bool _reverseDirection;
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
      double minScaleLog, maxScaleLog;
      int yellowScale, redScale;
      int cur_pixv, last_pixv, cur_pixmax, last_pixmax;
      bool _showText;
      QString _text;
      QRect _textRect;
      QRect _VURect;
      QRect _scaleRect;
      QRect _scaleGeom;
      QRect _spacerRect;
      QRect _VUFrameRect;
      QMargins _alignmentMargins;
      QPainterPath _bkgPath;
      QPainterPath _VUPath;
      QPainterPath _VUFramePath;

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
            bool isInteger = false, bool isLog = true,
            Qt::Orientation orient = Qt::Vertical, 
            double scaleMin = -60.0, double scaleMax = 10.0,
            ScalePos scalePos = ScaleNone,
            const QColor& primaryColor = QColor(0, 255, 0),
            ScaleDraw::TextHighlightMode textHighlightMode = ScaleDraw::TextHighlightNone,
            int refreshRate = 20);

//      QColor primaryColor() const { return _primaryColor; }
      void setPrimaryColor(const QColor& color, const QColor& bgColor = Qt::black);
      
      void setRange(double min, double max, bool isInteger = false, bool isLog = true);

      void setRefreshRate(int rate);
      
      bool showText() const;
      void setShowText(bool v);
      
      Qt::Orientation orientation() const;
      void setOrientation(Qt::Orientation o);

      // TODO: Support for reverse direction is not complete yet.
      bool reverseDirection() const;
      void setReverseDirection(bool);

      ScalePos scalePos() const;
      void setScalePos(const ScalePos&);
      // Returns the space between the VU and the scale.
      int scaleDist() const;
      // Sets the space between the VU and the scale.
      void setScaleDist(int);

      ScaleDraw::TextHighlightMode textHighlightMode() const;
      void setTextHighlightMode(ScaleDraw::TextHighlightMode = ScaleDraw::TextHighlightNone);

      QSize VUSizeHint() const;
      void setVUSizeHint(const QSize&);
      virtual QSize sizeHint() const;

      int radius() const;
      void setRadius(int radius);
      int vu3d() const;
      void setVu3d(int vu3d);

      void setFrame(bool frame, const QColor& color);
      // Sets an extra margin that helps align the VU rectangle top/bottom or left/right
      //  with an external rectangle's top/bottom or left/right such as the groove of a slider.
      // The parameter is an amount from the edge of for example a slider to the edge of its groove.
      // This will attempt to ensure that the meter's VU rectangle precisely aligns with the slider groove.
      void setAlignmentMargins(const QMargins&);

      bool log() const;
      void setLog(bool v);
      bool integer() const;
      void setInteger(bool v);
      // In log mode, sets the dB factor when conversions are done.
      // For example 20 * log10() for signals, 10 * log10() for power, and 40 * log10() for MIDI volume.
      void setDBFactor(double v = 20.0);
      // Sets the scale of a log range. For example a MIDI volume control can set a logFactor = 127
      //  so that the range can conveniently be set to 0-127. (With MIDI volume, dBFactor would be
      //  set to 40.0, as per MMA specs.) Min, max, off, input and output values are all scaled by this factor,
      //  but step is not.
      void setLogFactor(double v = 1.0);
      };

} // namespace MusEGui

#endif

