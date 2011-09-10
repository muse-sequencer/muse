//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: meter.cpp,v 1.4.2.2 2009/05/03 04:14:00 terminator356 Exp $
//  redesigned by oget on 2011/08/15
//
//  (C) Copyright 2000 Werner Schweer (ws@seh.de)
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

#include "verticalmeter.h"
#include "gconfig.h"
#include "fastlog.h"

namespace MusEWidget {

//---------------------------------------------------------
//   VerticalMeter
//---------------------------------------------------------

VerticalMeter::VerticalMeter(QWidget* parent, MeterType type)
   : Meter(parent, type) //Qt::WNoAutoErase
      {
      setBackgroundRole(QPalette::NoRole);
      setAttribute(Qt::WA_NoSystemBackground);
      setAttribute(Qt::WA_StaticContents);
      // This is absolutely required for speed! Otherwise painfully slow because we get 
      //  full rect paint events even on small scrolls! See help on QPainter::scroll().
      setAttribute(Qt::WA_OpaquePaintEvent);
      
      mtype = type;
      overflow    = false;
      val         = 0.0;
      maxVal      = 0.0;
      minScale    = mtype == DBMeter ? MusEConfig::config.minMeter : 0.0;      // min value in dB or int
      maxScale    = mtype == DBMeter ? 10.0 : 127.0;
      yellowScale = -10;
      redScale    = 0;
      setLineWidth(0);
      setMidLineWidth(0);
      }


//---------------------------------------------------------
//   setVal
//---------------------------------------------------------

void VerticalMeter::setVal(double v) //, double max, bool ovl)
      {
      //overflow = ovl;
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

//      if(maxVal != max)
//      {
//        maxVal = max;
//        ud = true;
//      }

      if(ud)
        update();
    }
//---------------------------------------------------------
//   resetPeaks
//    reset peak and overflow indicator
//---------------------------------------------------------

void VerticalMeter::resetPeaks()
      {
      maxVal   = val;
      overflow = val > 0.0;
      update();
      }

//---------------------------------------------------------
//   setRange
//---------------------------------------------------------

void VerticalMeter::setRange(double min, double max)
      {
      minScale = min;
      maxScale = max;
      update();
      }

//---------------------------------------------------------
//   paintEvent
//---------------------------------------------------------

void VerticalMeter::paintEvent(QPaintEvent* ev)
      {
      // TODO: Could make better use of event rectangle, for speed.
      
      QPainter p(this);
      
      double range = maxScale - minScale;

      /*
      int fw = frameWidth();
      int w  = width() - 2*fw;
      int h  = height() - 2*fw;
      */

      QRect rect = ev->rect();
      int w = rect.width();
      int h = rect.height();
      int xv;


      if(mtype == DBMeter) 
        xv = int(((maxScale - (fast_log10(val) * 20.0)) * w)/range);
      else {
        xv = int(((maxScale - val) * w)/range);
      }
      
      if(xv > w)
          xv = w;
      
      // Draw the red, green, and yellow sections.
      drawVU(p, w, h, xv);
      
      // Draw the peak white line.
      int xcenter;
      if(mtype == DBMeter) 
        xcenter = maxVal == 0 ? 0 : int(((maxScale - (fast_log10(0) * 20.0)) * w)/range);
      else
        xcenter = maxVal == 0 ? 0 : int(((0) * w)/range);
      p.setPen(peak_color);
      p.drawLine(xcenter, 0, xcenter, h);

      // Draw the transparent layer on top of everything to give a 3d look
      maskGrad.setStart(QPointF(0, 0));
      maskGrad.setFinalStop(QPointF(0, h));
      p.fillRect(0, 0, w, h, QBrush(maskGrad));
      }

//---------------------------------------------------------
//   drawVU
//---------------------------------------------------------

void VerticalMeter::drawVU(QPainter& p, int w, int h, int xv)
{
      if(mtype == DBMeter) 
      {
        double range = maxScale - minScale;
        int x1 = int((maxScale - redScale) * w / range);
        int x2 = int((maxScale - yellowScale) * w / range);

        darkGradGreen.setStart(QPointF(x2, 0));
        darkGradGreen.setFinalStop(QPointF(w, 0));
        darkGradYellow.setStart(QPointF(x1, 0));
        darkGradYellow.setFinalStop(QPointF(x2, 0));
        darkGradRed.setStart(QPointF(0, 0));
        darkGradRed.setFinalStop(QPointF(x1, 0));

        lightGradGreen.setStart(QPointF(x2, 0));
        lightGradGreen.setFinalStop(QPointF(w, 0));
        lightGradYellow.setStart(QPointF(x1, 0));
        lightGradYellow.setFinalStop(QPointF(x2, 0));
        lightGradRed.setStart(QPointF(0, 0));
        lightGradRed.setFinalStop(QPointF(x1, 0));

        
        if(xv < x1)
        {
          // Red section:
          p.fillRect(0, 0,  xv, h,        QBrush(darkGradRed));     // dark red
          p.fillRect(xv, 0, x1-xv, h,     QBrush(lightGradRed));     // light red
          
          // Yellow section:
          p.fillRect(x1, 0, x2-x1, h,     QBrush(lightGradYellow));     // light yellow
          
          // Green section:
          p.fillRect(x2, 0, w-x2, h,      QBrush(lightGradGreen));     // light green
        }
        else
        if(xv < x2)
        {
          // Red section:
          p.fillRect(0, 0,  x1, h,        QBrush(darkGradRed));     // dark red
          
          // Yellow section:
          p.fillRect(x1, 0, xv-x1, h,     QBrush(darkGradYellow));     // dark yellow
          p.fillRect(xv, 0, x2-xv, h,     QBrush(lightGradYellow));     // light yellow
          
          // Green section:
          p.fillRect(x2, 0, w-x2, h,      QBrush(lightGradGreen));     // light green
        }
        else
        //if(yv <= y3)   
        {
          // Red section:
          p.fillRect(0, 0,  x1, h,        QBrush(darkGradRed));     // dark red
          
          // Yellow section:
          p.fillRect(x1, 0, x2-x1, h,     QBrush(darkGradYellow));     // dark yellow
          
          // Green section:
          p.fillRect(x2, 0, xv-x2, h,     QBrush(darkGradGreen));     // dark green
          p.fillRect(xv, 0, w-xv, h,      QBrush(lightGradGreen));     // light green
        }

      p.fillRect(x1,0, 1, h, separator_color);
      p.fillRect(x2,0, 1, h, separator_color);

      }  
      else
      {
        darkGradGreen.setStart(QPointF(0, 0));
        darkGradGreen.setFinalStop(QPointF(w, 0));

        lightGradGreen.setStart(QPointF(0, 0));
        lightGradGreen.setFinalStop(QPointF(w, 0));

        p.fillRect(0, 0,  xv, h,   QBrush(lightGradGreen));   // light green
        p.fillRect(xv, 0, w-xv, h, QBrush(darkGradGreen));   // dark green
      }

}

//---------------------------------------------------------
//   resizeEvent
//---------------------------------------------------------

void VerticalMeter::resizeEvent(QResizeEvent* ev)
    {
    // Round corners of the widget.

    QSize size = ev->size();
    int w = size.width();
    int h = size.height();
    QPainterPath rounded_rect;
    rounded_rect.addRoundedRect(0,0,w,h, h/3, h/2.5);
    QRegion maskregion(rounded_rect.toFillPolygon().toPolygon());
    setMask(maskregion);
    }

} // namespace MusEWidget
