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
#include "mmath.h"
#include "utils.h"

namespace MusEGui {

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
      // Commented out for now. Orcan 20110911
      //setAttribute(Qt::WA_OpaquePaintEvent);
      
      mtype = type;
      overflow    = false;
      val         = 0.0;
      maxVal      = 0.0;
      minScale    = mtype == DBMeter ? MusEGlobal::config.minMeter : 0.0;      // min value in dB or int
      maxScale    = mtype == DBMeter ? 10.0 : 127.0;
      yellowScale = -10;
      redScale    = 0;
      xrad = 4;
      yrad = 4;

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

void VerticalMeter::paintEvent(QPaintEvent* /*ev*/)
      {
      // TODO: Could make better use of event rectangle, for speed.
      
      QPainter p(this);
      p.setRenderHint(QPainter::Antialiasing);
      
      double range = maxScale - minScale;

      int fw = frameWidth();
      int w  = width() - 2*fw;
      int h  = height() - 2*fw;

      /*
      QRect rect = ev->rect();
      int w = rect.width() - 2*fw;
      int h = rect.height() - 2*fw;
      */
      int xv;


      if(mtype == DBMeter) 
        xv = int(((maxScale - (MusECore::fast_log10(val) * 20.0)) * w)/range);
      else {
        xv = int(((maxScale - val) * w)/range);
      }
      
      if(xv > w)
          xv = w;
      
      // Draw the red, green, and yellow sections.
      drawVU(p, w, h, xv);
      
      // Draw the peak white line.
      /*
      int xcenter;
      if(mtype == DBMeter) 
        xcenter = maxVal == 0 ? 0 : int(((maxScale - (MusECore::fast_log10(0) * 20.0)) * w)/range);
      else
        xcenter = maxVal == 0 ? 0 : int(((maxVal) * w)/range);
      p.setPen(peak_color);
      p.drawLine(xcenter, 0, xcenter, h);
      */

      // Draw the transparent layer on top of everything to give a 3d look
      QPainterPath round_path = roundedPath(0, 0, w, h,
                                            xrad, yrad,
                                            (MusECore::Corner) (MusECore::UpperLeft | MusECore::UpperRight | MusECore::LowerLeft | MusECore::LowerRight ) );
      maskGrad.setStart(QPointF(0, 0));
      maskGrad.setFinalStop(QPointF(0, h));
      p.fillPath(round_path, QBrush(maskGrad));
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

        QPainterPath p_left = roundedPath(0, 0, x1, h,
                                           xrad, yrad,
                                           (MusECore::Corner) (MusECore::UpperLeft | MusECore::LowerLeft ) );

        QPainterPath p_right = roundedPath(x2, 0, w-x2, h,
                                            xrad, yrad,
                                            (MusECore::Corner) (MusECore::LowerRight | MusECore::UpperRight ) );
        
        if(xv < x1)
        {

	  QPainterPath p_light_green = roundedPath(0, 0, xv, h,
						   xrad, yrad,
						   (MusECore::Corner) (MusECore::UpperLeft | MusECore::LowerLeft ) );

	  p_left = p_left.subtracted(p_light_green);

          // Green section:
          p.fillPath(p_light_green,       QBrush(lightGradGreen));     // light green
          p.fillPath(p_left,              QBrush(darkGradGreen));      // dark green

          // Yellow section:
          p.fillRect(x1, 0, x2-x1, h,     QBrush(darkGradYellow));     // dark yellow
          
          // Red section:
          p.fillPath(p_right,             QBrush(darkGradRed));        // dark red
        }
        else
        if(xv < x2)
        {
          // Green section:
          p.fillPath(p_left,              QBrush(lightGradGreen));       // light green
          
          // Yellow section:
          p.fillRect(x1, 0, xv-x1, h,     QBrush(lightGradYellow));    // light yellow
          p.fillRect(xv, 0, x2-xv, h,     QBrush(darkGradYellow));     // dark yellow
          
          // Red section:
          p.fillPath(p_right,             QBrush(darkGradRed));        // dark red
        }
        else
        //if(xv <= x3)
        {
	  QPainterPath p_dark_red = roundedPath(xv, 0, w-xv, h,
                                                xrad, yrad,
                                                (MusECore::Corner) (MusECore::LowerRight | MusECore::UpperRight ) );

          p_right = p_right.subtracted(p_dark_red);

          // Green section:
          p.fillPath(p_left,              QBrush(lightGradGreen));     // light green
          
          // Yellow section:
          p.fillRect(x1, 0, x2-x1, h,     QBrush(lightGradYellow));    // light yellow
          
          // Red section:
          p.fillPath(p_right,             QBrush(lightGradRed));       // light red
          p.fillPath(p_dark_red,          QBrush(darkGradRed));        // dark red

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

        // We need to draw the meter in two parts. The cutoff for the left rectangle can be
        // anywhere between xrad and w-xrad. Without loss of generality we pick the lower limit.
        int cut = xrad;

        QPainterPath p_left = roundedPath(0, 0, cut, h,
                                          xrad, yrad,
                                          (MusECore::Corner) (MusECore::UpperLeft | MusECore::LowerLeft ) );

        QPainterPath p_right = roundedPath(cut, 0, w-cut, h,
                                           xrad, yrad,
                                           (MusECore::Corner) (MusECore::LowerRight | MusECore::UpperRight ) );

        if(xv < cut)
	  {

	    QPainterPath p_light = roundedPath(0, 0, xv, h,
                                               xrad, yrad,
                                               (MusECore::Corner) (MusECore::UpperLeft | MusECore::LowerLeft ) );

	    p_left = p_left.subtracted(p_light);

	    // left section:
	    p.fillPath(p_left,            QBrush(darkGradGreen));       // dark green
	    p.fillPath(p_light,           QBrush(lightGradGreen));      // light green

	    // bottom section:
	    p.fillPath(p_right,           QBrush(darkGradGreen));       // dark green
	  }
        else
	  {
	    QPainterPath p_dark = roundedPath(xv, 0, w-xv, h,
                                              xrad, yrad,
                                              (MusECore::Corner) (MusECore::UpperRight | MusECore::LowerRight ) );
	    p_right = p_right.subtracted(p_dark);

	    // left section:
	    p.fillPath(p_left,            QBrush(lightGradGreen));      // light green

	    // right section:
	    p.fillPath(p_dark,            QBrush(darkGradGreen));       // dark green
	    p.fillPath(p_right,           QBrush(lightGradGreen));      // light green
	  }

      }

}

//---------------------------------------------------------
//   resizeEvent
//---------------------------------------------------------

void VerticalMeter::resizeEvent(QResizeEvent* ev)
    {
      Meter::resizeEvent(ev);
    }

} // namespace MusEGui
