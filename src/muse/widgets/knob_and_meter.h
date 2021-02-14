//=========================================================
//  MusE
//  Linux Music Editor
//  knob_and_meter.h
//  (C) Copyright 2012 Tim E. Real (terminator356 on users dot sourceforge dot net)
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
#ifndef __KNOB_AND_METER_H__
#define __KNOB_AND_METER_H__

#include "knob.h"
#include "sclif.h"
#include <QColor>
#include <QResizeEvent>
#include <QPaintEvent>

namespace MusEGui {

//---------------------------------------------------------
//   KnobWithMeter
//---------------------------------------------------------

class KnobWithMeter : public Knob
      {
  Q_OBJECT

//    public:
//       enum Symbol { Line, Dot };
// 
//       enum KnobType {
//           panType,
//           auxType,
//           gainType,
//       };
// 
//    private:
//       bool hasScale;
// 
//       int d_borderWidth;
//       int d_shineWidth;
//       int d_scaleDist;
//       int d_maxScaleTicks;
//       int d_newVal;
//       int d_knobWidth;
//       int d_dotWidth;
// 
//       Symbol d_symbol;
//       double d_angle;
//       double d_oldAngle;
//       double d_totalAngle;
//       double d_nTurns;
// 
//       double l_const;
//       double l_slope;
// 
//       QRect  kRect;
//       bool _faceColSel;
//       QColor d_faceColor;
//       QColor d_shinyColor;
//       QColor d_rimColor;
//       QColor d_curFaceColor;
//       QColor d_altFaceColor;
//       QColor d_markerColor;
// 
//       void recalcAngle();
//       void valueChange();
//       void rangeChange();
      void drawKnob(QPainter *p, const QRect &r);
//       void drawMarker(QPainter *p, double arc, const QColor &c);

      virtual void paintEvent(QPaintEvent *e);
      virtual void resizeEvent(QResizeEvent *e);
      virtual void mousePressEvent(QMouseEvent *e);
//       double getValue(const QPoint &p);
//       void getScrollMode( QPoint &p, const Qt::MouseButton &button, const Qt::KeyboardModifiers& modifiers, int &scrollMode, int &direction );
//       void scaleChange()             { repaint(); }
//       void fontChange(const QFont &) { repaint(); }

   public:
      KnobWithMeter(QWidget* parent = 0, const char *name = 0);
      ~KnobWithMeter() {}

//       void setRange(double vmin, double vmax, double vstep = 0.0,
// 		    int pagesize = 1);
//       void setKnobWidth(int w);
//       void setTotalAngle (double angle);
//       void setBorderWidth(int bw);
//       void selectFaceColor(bool alt);
//       bool selectedFaceColor() { return _faceColSel; }
//       QColor faceColor() { return d_faceColor; }
//       void setFaceColor(const QColor c);
//       QColor altFaceColor() { return d_altFaceColor; }
//       void setAltFaceColor(const QColor c);
//       QColor markerColor() { return d_markerColor; }
//       void setMarkerColor(const QColor c);
      };

} // namespace MusEGui

#endif
