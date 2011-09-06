//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: meter.cpp,v 1.4.2.2 2009/05/03 04:14:00 terminator356 Exp $
//  redesigned by oget on 2011/08/15
//
//  (C) Copyright 2000 Werner Schweer (ws@seh.de)
//  (C) Copyright 2011 Orcan Ogetbil (ogetbilo at sf.net)
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

#include <cmath>

#include <QMouseEvent>
#include <QPainter>
#include <QResizeEvent>

#include "meter.h"
#include "gconfig.h"
#include "fastlog.h"

//---------------------------------------------------------
//   Meter
//---------------------------------------------------------

Meter::Meter(QWidget* parent, MeterType type)
   : QFrame(parent) //Qt::WNoAutoErase
      {
      setBackgroundRole(QPalette::NoRole);
      setAttribute(Qt::WA_NoSystemBackground);
      setAttribute(Qt::WA_StaticContents);
      // This is absolutely required for speed! Otherwise painfully slow because we get 
      //  full rect paint events even on small scrolls! See help on QPainter::scroll().
      setAttribute(Qt::WA_OpaquePaintEvent);
      //setFrameStyle(QFrame::Raised | QFrame::StyledPanel);

      mtype = type;
      overflow    = false;
      val         = 0.0;
      maxVal      = 0.0;
      minScale    = mtype == DBMeter ? config.minMeter : 0.0;      // min value in dB or int
      maxScale    = mtype == DBMeter ? 10.0 : 127.0;
      yellowScale = -10;
      redScale    = 0;
      setLineWidth(0);
      setMidLineWidth(0);

      dark_red_end = QColor(0x8e0000);
      dark_red_begin = QColor(0x8e3800);

      dark_yellow_end = QColor(0x8e6800);
      dark_yellow_center = QColor(0x8e8e00);
      dark_yellow_begin = QColor(0x6a8400);

      dark_green_end = QColor(0x467800);
      dark_green_begin = QColor(0x007000);

      light_red_end = QColor(0xff0000);
      light_red_begin = QColor(0xdd8800);

      light_yellow_end = QColor(0xddcc00);
      light_yellow_center = QColor(0xffff00);
      light_yellow_begin = QColor(0xddff00);

      light_green_end = QColor(0x88ff00);
      light_green_begin = QColor(0x00ff00);

      mask_center = QColor(225, 225, 225, 64);
      mask_edge = QColor(30, 30, 30, 64);

      separator_color = QColor(0x666666);
      peak_color = QColor(0xeeeeee);

      darkGradGreen.setColorAt(1, dark_green_begin);
      darkGradGreen.setColorAt(0, dark_green_end);

      darkGradYellow.setColorAt(1, dark_yellow_begin);
      darkGradYellow.setColorAt(0.5, dark_yellow_center);
      darkGradYellow.setColorAt(0, dark_yellow_end);

      darkGradRed.setColorAt(1, dark_red_begin);
      darkGradRed.setColorAt(0, dark_red_end);

      lightGradGreen.setColorAt(1, light_green_begin);
      lightGradGreen.setColorAt(0, light_green_end);

      lightGradYellow.setColorAt(1, light_yellow_begin);
      lightGradYellow.setColorAt(0.5, light_yellow_center);
      lightGradYellow.setColorAt(0, light_yellow_end);

      lightGradRed.setColorAt(1, light_red_begin);
      lightGradRed.setColorAt(0, light_red_end);

      maskGrad.setColorAt(0, mask_edge);
      maskGrad.setColorAt(0.5, mask_center);
      maskGrad.setColorAt(1, mask_edge);

      }

//---------------------------------------------------------
//   setVal
//---------------------------------------------------------

void Meter::setVal(double v, double max, bool ovl)
      {
      overflow = ovl;
      bool ud = false;

      if(mtype == DBMeter)
      {
        double minScaleLin = pow(10.0, minScale/20.0);
        if((v >= minScaleLin && val != v) || val >= minScaleLin)
        {
          val = v;
          ud = true;
        }
      }
      else
      {
        if(val != v)
        {
          val = v;
          ud = true;
        }
      }  
      
      if(maxVal != max)
      {
        maxVal = max;
        ud = true;
      }
      
      if(ud)
        update();
    }
//---------------------------------------------------------
//   resetPeaks
//    reset peak and overflow indicator
//---------------------------------------------------------

void Meter::resetPeaks()
      {
      maxVal   = val;
      overflow = val > 0.0;
      update();
      }

//---------------------------------------------------------
//   setRange
//---------------------------------------------------------

void Meter::setRange(double min, double max)
      {
      minScale = min;
      maxScale = max;
      update();
      }

//---------------------------------------------------------
//   paintEvent
//---------------------------------------------------------

void Meter::paintEvent(QPaintEvent* /*ev*/)
      {
      // TODO: Could make better use of event rectangle, for speed.
      
      QPainter p(this);
      //p.setRenderHint(QPainter::Antialiasing);
      
      double range = maxScale - minScale;
      
      int fw = frameWidth();
      int w  = width() - 2*fw;
      int h  = height() - 2*fw;
      
      // FIXME (Orcan): With the event rectangle we get corruption when we toggle the mono/stereo switch. Why?
      /*
      QRect rect = ev->rect();
      int w = rect.width();
      int h = rect.height();
      */
      int yv;
      
      if(mtype == DBMeter)
        yv = val == 0 ? h : int(((maxScale - (fast_log10(val) * 20.0)) * h)/range);
      else
        yv = val == 0 ? h : int(((maxScale - val) * h)/range);
      
      if(yv > h) yv = h;
      
      // Draw the red, green, and yellow sections.
      drawVU(p, w, h, yv);
      
      // Draw the peak white line.
      int ymax;
      if(mtype == DBMeter)
        ymax = maxVal == 0 ? 0 : int(((maxScale - (fast_log10(maxVal) * 20.0)) * h)/range);
      else
        ymax = maxVal == 0 ? 0 : int(((maxScale - maxVal) * h)/range);
      p.setPen(peak_color);
      p.drawLine(0, ymax, w, ymax);

      // Draw the transparent layer on top of everything to give a 3d look
      maskGrad.setStart(QPointF(0, 0));
      maskGrad.setFinalStop(QPointF(w, 0));
      p.fillRect(0, 0, w, h, QBrush(maskGrad));

      }


//---------------------------------------------------------
//   drawVU
//---------------------------------------------------------

void Meter::drawVU(QPainter& p, int w, int h, int yv)
{
      if(mtype == DBMeter)
      {
        double range = maxScale - minScale;
        int y1 = int((maxScale - redScale) * h / range);
        int y2 = int((maxScale - yellowScale) * h / range);

        darkGradGreen.setStart(QPointF(0, y2));
        darkGradGreen.setFinalStop(QPointF(0, h));
        darkGradYellow.setStart(QPointF(0, y1));
        darkGradYellow.setFinalStop(QPointF(0, y2));
        darkGradRed.setStart(QPointF(0, 0));
        darkGradRed.setFinalStop(QPointF(0, y1));

        lightGradGreen.setStart(QPointF(0, y2));
        lightGradGreen.setFinalStop(QPointF(0, h));
        lightGradYellow.setStart(QPointF(0, y1));
        lightGradYellow.setFinalStop(QPointF(0, y2));
        lightGradRed.setStart(QPointF(0, 0));
        lightGradRed.setFinalStop(QPointF(0, y1));

        if(yv < y1)
        {
          // Red section:
          p.fillRect(0, 0,  w, yv,        QBrush(darkGradRed));     // dark red  
          p.fillRect(0, yv, w, y1-yv,     QBrush(lightGradRed));     // light red
          
          // Yellow section:
          p.fillRect(0, y1, w, y2-y1,     QBrush(lightGradYellow));     // light yellow
          
          // Green section:
          p.fillRect(0, y2, w, h-y2,      QBrush(lightGradGreen));     // light green
        }
        else
        if(yv < y2)
        {
          // Red section:
          p.fillRect(0, 0,  w, y1,        QBrush(darkGradRed));     // dark red  
          
          // Yellow section:
          p.fillRect(0, y1, w, yv-y1,     QBrush(darkGradYellow));     // dark yellow
          p.fillRect(0, yv, w, y2-yv,     QBrush(lightGradYellow));     // light yellow
          
          // Green section:
          p.fillRect(0, y2, w, h-y2,      QBrush(lightGradGreen));     // light green
        }
        else
        //if(yv <= y3)   
        {
          // Red section:
          p.fillRect(0, 0,  w, y1,        QBrush(darkGradRed));     // dark red  
          
          // Yellow section:
          p.fillRect(0, y1, w, y2-y1,     QBrush(darkGradYellow));     // dark yellow
          
          // Green section:
          p.fillRect(0, y2, w, yv-y2,     QBrush(darkGradGreen));     // dark green
          p.fillRect(0, yv, w, h-yv,      QBrush(lightGradGreen));     // light green
        }

	p.fillRect(0,y1, w, 1, separator_color);
	p.fillRect(0,y2, w, 1, separator_color);

      }  
      else
      {
        darkGradGreen.setStart(QPointF(0, 0));
        darkGradGreen.setFinalStop(QPointF(0, h));

        lightGradGreen.setStart(QPointF(0, 0));
        lightGradGreen.setFinalStop(QPointF(0, h));

        p.fillRect(0, 0,  w, yv,   QBrush(darkGradGreen));   // dark green
        p.fillRect(0, yv, w, h-yv, QBrush(lightGradGreen));   // light green
      }

}

//---------------------------------------------------------
//   resizeEvent
//---------------------------------------------------------

void Meter::resizeEvent(QResizeEvent* ev)
    {
    // Round corners of the widget.
      
    QSize size = ev->size();
    int w = size.width();
    int h = size.height();
    QPainterPath rounded_rect;
    rounded_rect.addRoundedRect(0,0,w,h, w/2.5, w/3);
    QRegion maskregion(rounded_rect.toFillPolygon().toPolygon());
    setMask(maskregion);
      
    /*
    // Another method to do the above. I don't know yet which one is more efficient - Orcan
    QRect rect(0,0,w,h);
    int r = 6;

    QRegion region;
    // middle and borders
    region += rect.adjusted(r, 0, -r, 0);
    region += rect.adjusted(0, r, 0, -r);
    // top left
    QRect corner(rect.topLeft(), QSize(r*2, r*2));
    region += QRegion(corner, QRegion::Ellipse);
    // top right
    corner.moveTopRight(rect.topRight());
    region += QRegion(corner, QRegion::Ellipse);
    // bottom left
    corner.moveBottomLeft(rect.bottomLeft());
    region += QRegion(corner, QRegion::Ellipse);
    // bottom right
    corner.moveBottomRight(rect.bottomRight());
    region += QRegion(corner, QRegion::Ellipse);
    //      return region;
    setMask(region);
    */
  
    }

//---------------------------------------------------------
//   mousePressEvent
//---------------------------------------------------------

void Meter::mousePressEvent(QMouseEvent*)
      {
      emit mousePress();
      }

