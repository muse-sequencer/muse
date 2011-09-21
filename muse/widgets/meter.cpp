//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: meter.cpp,v 1.4.2.2 2009/05/03 04:14:00 terminator356 Exp $
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

#include <cmath>

#include <QMouseEvent>
#include <QPainter>
#include <QResizeEvent>

#include "meter.h"
#include "utils.h"
#include "gconfig.h"
#include "fastlog.h"

// Uncomment to use the new meters. Warning: They are currently very time-consuming and unoptimized.
#define _USE_NEW_METERS 1 

namespace MusEWidget {

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
      // Commented out for now. Orcan 20110911
      #ifndef _USE_NEW_METERS
      setAttribute(Qt::WA_OpaquePaintEvent);    
      #endif
      //setFrameStyle(QFrame::Raised | QFrame::StyledPanel);

      mtype = type;
      overflow    = false;
      cur_yv      = -1;     // Flag as -1 to initialize in paint.
      last_yv     = 0;
      val         = 0.0;
      maxVal      = 0.0;
      minScale    = mtype == DBMeter ? MusEConfig::config.minMeter : 0.0;      // min value in dB or int
      maxScale    = mtype == DBMeter ? 10.0 : 127.0;
      yellowScale = -10;
      redScale    = 0;
      setLineWidth(0);
      setMidLineWidth(0);

      // rounding radii
      xrad = 4;
      yrad = 4;

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
      {
        #ifdef _USE_NEW_METERS
        update(); 
        
        #else
        double range = maxScale - minScale;
        int fw = frameWidth();
        int w  = width() - 2*fw;
        int h  = height() - 2*fw;
        
        if(mtype == DBMeter)
          cur_yv = val == 0 ? h : int(((maxScale - (fast_log10(val) * 20.0)) * h)/range);
        else
          cur_yv = val == 0 ? h : int(((maxScale - val) * h)/range);
        
        if(cur_yv > h) cur_yv = h;
        
        int y1, y2;
        if(last_yv < cur_yv)
        {
          y1 = last_yv;
          y2 = cur_yv;
        }
        else
        {
          y1 = cur_yv;
          y2 = last_yv;
        }
        last_yv = cur_yv;
        update(fw, y1, w, y2); 
        #endif
      }
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
      cur_yv = -1;  // Force re-initialization.
      update();
      }

//---------------------------------------------------------
//   paintEvent
//---------------------------------------------------------

void Meter::paintEvent(QPaintEvent* ev)
      {
      // TODO: Could make better use of event rectangle, for speed.
      
      QPainter p(this);
      #ifdef _USE_NEW_METERS
      p.setRenderHint(QPainter::Antialiasing);  
      #endif
      
      double range = maxScale - minScale;
      
      int fw = frameWidth();
      int w  = width() - 2*fw;
      int h  = height() - 2*fw;

      const QRect& rect = ev->rect();
      //printf("Meter::paintEvent rx:%d ry:%d rw:%d rh:%d w:%d h:%d\n", rect.x(), rect.y(), rect.width(), rect.height(), w, h); 
      //p.setClipRect(rect); // Nope, didn't help (I think it's already clipped. So check why we bother to do it in View).
      
      // Initialize. Can't do in ctor, must be done after layouts have been done. Most reliable to do it here.
      #ifndef _USE_NEW_METERS
      if(cur_yv == -1) 
      #endif    
      {
        if(mtype == DBMeter)
          cur_yv = val == 0 ? h : int(((maxScale - (fast_log10(val) * 20.0)) * h)/range);
        else
          cur_yv = val == 0 ? h : int(((maxScale - val) * h)/range);
        if(cur_yv > h) cur_yv = h;
        //last_yv = cur_yv;
      }
      
      // Draw the red, green, and yellow sections.
      drawVU(p, rect, cur_yv);
      
      // Draw the peak white line.
      int ymax;
      if(mtype == DBMeter)
        ymax = maxVal == 0 ? 0 : int(((maxScale - (fast_log10(maxVal) * 20.0)) * h)/range);
      else
        ymax = maxVal == 0 ? 0 : int(((maxScale - maxVal) * h)/range);
      if(ymax >= rect.y() && ymax < rect.height())
      {
        p.setPen(peak_color);
        p.drawLine(0, ymax, w, ymax);
      }
      
      #ifdef _USE_NEW_METERS
      // Draw the transparent layer on top of everything to give a 3d look
      QPainterPath round_path = MusEUtil::roundedPath(0, 0, w, h,
                                            xrad, yrad,
                                            (MusEUtil::Corner) (MusEUtil::UpperLeft | MusEUtil::UpperRight | MusEUtil::LowerLeft | MusEUtil::LowerRight ) );
      maskGrad.setStart(QPointF(0, 0));
      maskGrad.setFinalStop(QPointF(w, 0));
      p.fillPath(round_path, QBrush(maskGrad));
      #endif
      
      }


//---------------------------------------------------------
//   drawVU
//---------------------------------------------------------

void Meter::drawVU(QPainter& p, const QRect& rect, int yv)
{
      int fw = frameWidth();
      int w  = width() - 2*fw;
      int h  = height() - 2*fw;
      
      // Test OK. We are passed small rectangles on small value changes.
      //printf("Meter::drawVU rx:%d ry:%d rw:%d rh:%d w:%d h:%d\n", rect.x(), rect.y(), rect.width(), rect.height(), w, h); 

  #ifdef _USE_NEW_METERS
      
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

        QPainterPath p_top = MusEUtil::roundedPath(0, 0, w, y1,
                                         xrad, yrad,
                                         (MusEUtil::Corner) (MusEUtil::UpperLeft | MusEUtil::UpperRight ) );

	QPainterPath p_bottom = MusEUtil::roundedPath(0, y2, w, h-y2,
                                            xrad, yrad,
                                            (MusEUtil::Corner) (MusEUtil::LowerLeft | MusEUtil::LowerRight ) );

        if(yv < y1)
        {

          QPainterPath p_dark_red = MusEUtil::roundedPath(0, 0, w, yv,
                                                xrad, yrad,
                                                (MusEUtil::Corner) (MusEUtil::UpperLeft | MusEUtil::UpperRight ) );

          p_top = p_top.subtracted(p_dark_red);

          // Red section:
          p.fillPath(p_dark_red,        QBrush(darkGradRed));       // dark red  
          p.fillPath(p_top,             QBrush(lightGradRed));      // light red

          // Yellow section:
          p.fillRect(0, y1, w, y2-y1,   QBrush(lightGradYellow));   // light yellow

          // Green section:
          p.fillPath(p_bottom,          QBrush(lightGradGreen));    // light green
        }
        else
        if(yv < y2)
        {
          // Red section:
          p.fillPath(p_top,             QBrush(darkGradRed));       // dark red  

          // Yellow section:
          p.fillRect(0, y1, w, yv-y1,   QBrush(darkGradYellow));    // dark yellow
          p.fillRect(0, yv, w, y2-yv,   QBrush(lightGradYellow));   // light yellow

          // Green section:
          p.fillPath(p_bottom,          QBrush(lightGradGreen));    // light green
        }
        else
        //if(yv <= y3)   
        {
          QPainterPath p_light_green = MusEUtil::roundedPath(0, yv, w, h-yv,
                                                   xrad, yrad,
                                                   (MusEUtil::Corner) (MusEUtil::LowerLeft | MusEUtil::LowerRight ) );
          p_bottom = p_bottom.subtracted(p_light_green);

          // Red section:
          p.fillPath(p_top,             QBrush(darkGradRed));      // dark red  

          // Yellow section:
          p.fillRect(0, y1, w, y2-y1,   QBrush(darkGradYellow));   // dark yellow

          // Green section:
          p.fillPath(p_bottom,          QBrush(darkGradGreen));    // dark green
          p.fillPath(p_light_green,     QBrush(lightGradGreen));   // light green
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

        // We need to draw the meter in two parts. The cutoff for upper rectangle can be
        // anywhere between yrad and h-yrad. Without loss of generality we pick the lower limit.
	int cut = yrad;

        QPainterPath p_top = MusEUtil::roundedPath(0, 0, w, cut,
                                         xrad, yrad,
                                         (MusEUtil::Corner) (MusEUtil::UpperLeft | MusEUtil::UpperRight ) );

	QPainterPath p_bottom = MusEUtil::roundedPath(0, cut, w, h-cut,
                                            xrad, yrad,
                                            (MusEUtil::Corner) (MusEUtil::LowerLeft | MusEUtil::LowerRight ) );

        if(yv < cut)
        {

          QPainterPath p_dark = MusEUtil::roundedPath(0, 0, w, yv,
                                            xrad, yrad,
                                            (MusEUtil::Corner) (MusEUtil::UpperLeft | MusEUtil::UpperRight ) );

          p_top = p_top.subtracted(p_dark);

          // top section:
          p.fillPath(p_dark,            QBrush(darkGradGreen));       // dark green
          p.fillPath(p_top,             QBrush(lightGradGreen));      // light green

          // bottom section:
          p.fillPath(p_bottom,          QBrush(lightGradGreen));      // light green
        }
        else
        {
          QPainterPath p_light = MusEUtil::roundedPath(0, yv, w, h-yv,
                                                   xrad, yrad,
                                                   (MusEUtil::Corner) (MusEUtil::LowerLeft | MusEUtil::LowerRight ) );
          p_bottom = p_bottom.subtracted(p_light);

          // top section:
          p.fillPath(p_top,             QBrush(darkGradGreen));       // dark green

          // bottom section:
          p.fillPath(p_bottom,          QBrush(darkGradGreen));       // dark green
          p.fillPath(p_light,           QBrush(lightGradGreen));      // light green
        }

      }
      
      /*  WIP...
      // TODO: Instead of drawing the whole area, make this respect the requested drawing rectangle, for speed...
      //       Done. But not correct yet. And could possibly simplify the whole drawing some more...
      QRect pr(0, 0,  w, 0);
      QPainterPath rectp;
      rectp.addRect(rect.x(), rect.y(), rect.width(), rect.height());
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

        QPainterPath p_top = MusEUtil::roundedPath(0, 0, w, y1,
                                         xrad, yrad,
                                         (MusEUtil::Corner) (MusEUtil::UpperLeft | MusEUtil::UpperRight ) );

        QPainterPath p_bottom = MusEUtil::roundedPath(0, y2, w, h-y2,
                                            xrad, yrad,
                                            (MusEUtil::Corner) (MusEUtil::LowerLeft | MusEUtil::LowerRight ) );

        if(yv < y1)
        {

          QPainterPath p_dark_red = MusEUtil::roundedPath(0, 0, w, yv,
                                                xrad, yrad,
                                                (MusEUtil::Corner) (MusEUtil::UpperLeft | MusEUtil::UpperRight ) );

          p_top = p_top.subtracted(p_dark_red);

          // Red section:
          p.fillPath(p_dark_red & rectp,        QBrush(darkGradRed));       // dark red  
          p.fillPath(p_top & rectp,             QBrush(lightGradRed));      // light red

          // Yellow section:
          pr.setTop(y1); pr.setHeight(y2-y1);
          p.fillRect(pr & rect, QBrush(lightGradYellow));   // light yellow

          // Green section:
          p.fillPath(p_bottom & rectp, QBrush(lightGradGreen));    // light green
        }
        else
        if(yv < y2)
        {
          // Red section:
          p.fillPath(p_top & rectp, QBrush(darkGradRed));       // dark red  

          // Yellow section:
          pr.setTop(y1); pr.setHeight(yv-y1);
          p.fillRect(pr & rect, QBrush(darkGradYellow));    // dark yellow
          pr.setTop(yv); pr.setHeight(y2-yv);
          p.fillRect(pr & rect, QBrush(lightGradYellow));   // light yellow

          // Green section:
          p.fillPath(p_bottom & rectp, QBrush(lightGradGreen));    // light green
        }
        else
        //if(yv <= y3)   
        {
          QPainterPath p_light_green = MusEUtil::roundedPath(0, yv, w, h-yv,
                                                   xrad, yrad,
                                                   (MusEUtil::Corner) (MusEUtil::LowerLeft | MusEUtil::LowerRight ) );
          p_bottom = p_bottom.subtracted(p_light_green);

          // Red section:
          p.fillPath(p_top & rectp, QBrush(darkGradRed));      // dark red  

          // Yellow section:
          pr.setTop(y1); pr.setHeight(y2-y1);
          p.fillRect(pr & rect, QBrush(darkGradYellow));   // dark yellow

          // Green section:
          p.fillPath(p_bottom & rectp, QBrush(darkGradGreen));    // dark green
          p.fillPath(p_light_green & rectp, QBrush(lightGradGreen));   // light green
        }

        pr.setTop(y1); pr.setHeight(1);
        p.fillRect(pr & rect, separator_color);
        pr.setTop(y2); //pr.setHeight(1);
        p.fillRect(pr & rect, separator_color);

      }  
      else
      {
        darkGradGreen.setStart(QPointF(0, 0));
        darkGradGreen.setFinalStop(QPointF(0, h));

        lightGradGreen.setStart(QPointF(0, 0));
        lightGradGreen.setFinalStop(QPointF(0, h));

        // We need to draw the meter in two parts. The cutoff for upper rectangle can be
        // anywhere between yrad and h-yrad. Without loss of generality we pick the lower limit.
        int cut = yrad;

        QPainterPath p_top = MusEUtil::roundedPath(0, 0, w, cut,
                                         xrad, yrad,
                                         (MusEUtil::Corner) (MusEUtil::UpperLeft | MusEUtil::UpperRight ) );

        QPainterPath p_bottom = MusEUtil::roundedPath(0, cut, w, h-cut,
                                            xrad, yrad,
                                            (MusEUtil::Corner) (MusEUtil::LowerLeft | MusEUtil::LowerRight ) );

        if(yv < cut)
        {

          QPainterPath p_dark = MusEUtil::roundedPath(0, 0, w, yv,
                                            xrad, yrad,
                                            (MusEUtil::Corner) (MusEUtil::UpperLeft | MusEUtil::UpperRight ) );

          p_top = p_top.subtracted(p_dark);

          // top section:
          p.fillPath(p_dark & rectp, QBrush(darkGradGreen));       // dark green
          p.fillPath(p_top & rectp, QBrush(lightGradGreen));      // light green

          // bottom section:
          p.fillPath(p_bottom & rectp, QBrush(lightGradGreen));      // light green
        }
        else
        {
          QPainterPath p_light = MusEUtil::roundedPath(0, yv, w, h-yv,
                                                   xrad, yrad,
                                                   (MusEUtil::Corner) (MusEUtil::LowerLeft | MusEUtil::LowerRight ) );
          p_bottom = p_bottom.subtracted(p_light);

          // top section:
          p.fillPath(p_top & rectp, QBrush(darkGradGreen));       // dark green

          // bottom section:
          p.fillPath(p_bottom & rectp, QBrush(darkGradGreen));       // dark green
          p.fillPath(p_light & rectp, QBrush(lightGradGreen));      // light green
        }

      }
      */
      
  #else
      
      QRect pr(0, 0,  w, 0);
      if(mtype == DBMeter)
      {
        double range = maxScale - minScale;
        int y1 = int((maxScale - redScale) * h / range);
        int y2 = int((maxScale - yellowScale) * h / range);
        
        if(yv < y1)
        {
          // Red section:
          pr.setTop(0); pr.setHeight(yv);
          p.fillRect(pr & rect, QBrush(0x8e0000));     // dark red  
          pr.setTop(yv); pr.setHeight(y1-yv);
          p.fillRect(pr & rect, QBrush(0xff0000));     // light red
          
          // Yellow section:
          pr.setTop(y1); pr.setHeight(y2-y1);
          p.fillRect(pr & rect, QBrush(0xffff00));     // light yellow
          
          // Green section:
          pr.setTop(y2); pr.setHeight(h-y2);
          p.fillRect(pr & rect, QBrush(0x00ff00));     // light green
        }
        else
        if(yv < y2)
        {
          // Red section:
          pr.setTop(0); pr.setHeight(y1);
          p.fillRect(pr & rect, QBrush(0x8e0000));     // dark red  
          
          // Yellow section:
          pr.setTop(y1); pr.setHeight(yv-y1);
          p.fillRect(pr & rect, QBrush(0x8e8e00));     // dark yellow
          pr.setTop(yv); pr.setHeight(y2-yv);
          p.fillRect(pr & rect, QBrush(0xffff00));     // light yellow
          
          // Green section:
          pr.setTop(y2); pr.setHeight(h-y2);
          p.fillRect(pr & rect, QBrush(0x00ff00));     // light green
        }
        else
        //if(yv <= y3)   
        {
          // Red section:
          pr.setTop(0); pr.setHeight(y1);
          p.fillRect(pr & rect, QBrush(0x8e0000));     // dark red  
          
          // Yellow section:
          pr.setTop(y1); pr.setHeight(y2-y1);
          p.fillRect(pr & rect, QBrush(0x8e8e00));     // dark yellow
          
          // Green section:
          pr.setTop(y2); pr.setHeight(yv-y2);
          p.fillRect(pr & rect, QBrush(0x007000));     // dark green
          pr.setTop(yv); pr.setHeight(h-yv);
          p.fillRect(pr & rect, QBrush(0x00ff00));     // light green
        }
      }  
      else
      {
        pr.setTop(0); pr.setHeight(yv);
        p.fillRect(pr & rect, QBrush(0x007000));   // dark green
        pr.setTop(yv); pr.setHeight(h-yv);
        p.fillRect(pr & rect, QBrush(0x00ff00));   // light green
      }
      
  #endif
}

//---------------------------------------------------------
//   resizeEvent
//---------------------------------------------------------

void Meter::resizeEvent(QResizeEvent* /*ev*/)
    {  
      cur_yv = -1;  // Force re-initialization.
    }

//---------------------------------------------------------
//   mousePressEvent
//---------------------------------------------------------

void Meter::mousePressEvent(QMouseEvent*)
      {
      emit mousePress();
      }

} // namespace MusEWidget
