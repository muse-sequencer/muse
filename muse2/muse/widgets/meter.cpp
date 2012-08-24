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

// Just an experiment. Some undesirable effects, see below...
//#define _USE_CLIPPER 1 

namespace MusEGui {

//---------------------------------------------------------
//   Meter
//---------------------------------------------------------

Meter::Meter(QWidget* parent, MeterType type)
   : QFrame(parent) //Qt::WNoAutoErase
      {
      setBackgroundRole(QPalette::NoRole);
      setAttribute(Qt::WA_NoSystemBackground);
      setAttribute(Qt::WA_StaticContents);
      // This is absolutely required for speed! Otherwise painfully slow because of full background 
      //  filling, even when requesting small udpdates! Background is drawn by us. (Just small corners.)
      setAttribute(Qt::WA_OpaquePaintEvent);    
      //setFrameStyle(QFrame::Raised | QFrame::StyledPanel);

      mtype = type;
      overflow    = false;
      cur_yv      = -1;     // Flag as -1 to initialize in paint.
      last_yv     = 0;
      cur_ymax    = 0;
      last_ymax   = 0;
      val         = 0.0;
      maxVal      = 0.0;
      minScale    = mtype == DBMeter ? MusEGlobal::config.minMeter : 0.0;      // min value in dB or int
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
      
      double range = maxScale - minScale;
      int fw = frameWidth();
      int w  = width() - 2*fw;
      int h  = height() - 2*fw;
      QRect udRect;
      bool udPeak = false;
      
      if(maxVal != max)
      {
        maxVal = max;
        if(mtype == DBMeter)
          cur_ymax = maxVal == 0 ? fw : int(((maxScale - (MusECore::fast_log10(maxVal) * 20.0)) * h)/range);
        else
          cur_ymax = maxVal == 0 ? fw : int(((maxScale - maxVal) * h)/range);
        if(cur_ymax > h) cur_ymax = h;
        // Not using regions. Just lump them together.
        udRect = QRect(fw, last_ymax, w, 1) | QRect(fw, cur_ymax, w, 1);
        //printf("Meter::setVal peak cur_ymax:%d last_ymax:%d\n", cur_ymax, last_ymax); 
        last_ymax = cur_ymax; 
        ud = true;
        udPeak = true;
      }
      
      if(ud)        
      {
        if(mtype == DBMeter)
          cur_yv = val == 0 ? h : int(((maxScale - (MusECore::fast_log10(val) * 20.0)) * h)/range);
        else
          cur_yv = val == 0 ? h : int(((maxScale - val) * h)/range);
        if(cur_yv > h) cur_yv = h;

        //printf("Meter::setVal cur_yv:%d last_yv:%d\n", cur_yv, last_yv); 
        int y1, y2;
        if(last_yv < cur_yv) { y1 = last_yv; y2 = cur_yv; } else { y1 = cur_yv; y2 = last_yv; }
        last_yv = cur_yv;
        
        if(udPeak)
          update(udRect | QRect(fw, y1, w, y2 - y1 + 1)); 
          //repaint(udRect | QRect(fw, y1, w, y2 - y1 + 1)); 
        else
          update(QRect(fw, y1, w, y2 - y1 + 1)); 
          //repaint(QRect(fw, y1, w, y2 - y1 + 1)); 
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
      cur_yv = -1;  // Force re-initialization.
      update();               
      }

//---------------------------------------------------------
//   setRange
//---------------------------------------------------------

void Meter::setRange(double min, double max)
      {
      if(min == minScale && max == maxScale)   // p4.0.45
        return;
      
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
      // For some reason upon resizing we get double calls here and in resizeEvent.

      QPainter p(this);
      p.setRenderHint(QPainter::Antialiasing);  

      double range = maxScale - minScale;
      int fw = frameWidth();
      int w  = width() - 2*fw;
      int h  = height() - 2*fw;
      const QRect& rect = ev->rect();
      //printf("Meter::paintEvent rx:%d ry:%d rw:%d rh:%d w:%d h:%d\n", rect.x(), rect.y(), rect.width(), rect.height(), w, h); 

      QPainterPath drawingPath, updatePath, finalPath, cornerPath;
      //bool updFull = false;
      
      // Initialize. Can't do in ctor, must be done after layouts have been done. Most reliable to do it here.
      if(cur_yv == -1) 
      {
        if(mtype == DBMeter)
        {  
          cur_yv = val == 0 ? h : int(((maxScale - (MusECore::fast_log10(val) * 20.0)) * h)/range);
          cur_ymax = maxVal == 0 ? fw : int(((maxScale - (MusECore::fast_log10(maxVal) * 20.0)) * h)/range);
        }  
        else
        {  
          cur_yv = val == 0 ? h : int(((maxScale - val) * h)/range);
          cur_ymax = maxVal == 0 ? fw : int(((maxScale - maxVal) * h)/range);
        }  
        if(cur_yv > h) cur_yv = h;
        last_yv = cur_yv;
        if(cur_ymax > h) cur_ymax = h;
        last_ymax = cur_ymax;
        //updFull = true;
        updatePath.addRect(fw, fw, w, h);  // Update the whole thing
      }
      else
        updatePath.addRect(rect.x(), rect.y(), rect.width(), rect.height());  // Update only the requested rectangle
      
      drawingPath.addRoundedRect(fw, fw, w, h, xrad, yrad);  // The actual desired shape of the meter
      finalPath = drawingPath & updatePath;

      // Draw corners as normal background colour.
      cornerPath = updatePath - finalPath;            // Elegantly simple. Path subtraction! Wee...
      if(!cornerPath.isEmpty())
        p.fillPath(cornerPath, palette().window());  
      
#ifdef _USE_CLIPPER
      p.setClipPath(finalPath);       //  Meh, nice but not so good. Clips at edge so antialising has no effect! Can it be done ?
#endif
      
      // Draw the red, green, and yellow sections.
      drawVU(p, rect, finalPath, cur_yv);
      
      // Draw the peak white line.
      //if(updFull || (cur_ymax >= rect.y() && cur_ymax < rect.height()))
      {
        p.setRenderHint(QPainter::Antialiasing, false);  // No antialiasing. Makes the line fuzzy, double height, or not visible at all.

        //p.setPen(peak_color);
        //p.drawLine(fw, cur_ymax, w, cur_ymax); // Undesirable. Draws outside the top rounded corners.
        //
        //QPainterPath path; path.moveTo(fw, cur_ymax); path.lineTo(w, cur_ymax);  // ? Didn't work. No line at all.
        //p.drawPath(path & finalPath);
        QPainterPath path; path.addRect(fw, cur_ymax, w, 1); path &= finalPath;
        if(!path.isEmpty())
          p.fillPath(path, QBrush(peak_color));
      }
      
      // Draw the transparent layer on top of everything to give a 3d look
      p.setRenderHint(QPainter::Antialiasing);  
      maskGrad.setStart(QPointF(fw, fw));
      maskGrad.setFinalStop(QPointF(w, fw));
#ifdef _USE_CLIPPER
      p.fillRect(rect, QBrush(maskGrad));
#else
      //QPainterPath path; path.addRect(fw, fw, w);
      //p.fillPath(finalPath & path, QBrush(maskGrad));
      p.fillPath(finalPath, QBrush(maskGrad));
#endif      
      
      }

//---------------------------------------------------------
//   drawVU
//---------------------------------------------------------

void Meter::drawVU(QPainter& p, const QRect& rect, const QPainterPath& drawPath, int yv)
{
      int fw = frameWidth();
      int w  = width() - 2*fw;
      int h  = height() - 2*fw;
      
      // Test OK. We are passed small rectangles on small value changes.
      //printf("Meter::drawVU rx:%d ry:%d rw:%d rh:%d w:%d h:%d\n", rect.x(), rect.y(), rect.width(), rect.height(), w, h); 

      QRect pr(0, 0,  w, 0);
      if(mtype == DBMeter)     // Meter type is dB...
      {
        double range = maxScale - minScale;
        int y1 = int((maxScale - redScale) * h / range);
        int y2 = int((maxScale - yellowScale) * h / range);

        darkGradGreen.setStart(QPointF(fw, y2));
        darkGradGreen.setFinalStop(QPointF(fw, h));
        darkGradYellow.setStart(QPointF(fw, y1));
        darkGradYellow.setFinalStop(QPointF(fw, y2));
        darkGradRed.setStart(QPointF(fw, fw));
        darkGradRed.setFinalStop(QPointF(fw, y1));

        lightGradGreen.setStart(QPointF(fw, y2));
        lightGradGreen.setFinalStop(QPointF(fw, h));
        lightGradYellow.setStart(QPointF(fw, y1));
        lightGradYellow.setFinalStop(QPointF(fw, y2));
        lightGradRed.setStart(QPointF(fw, fw));
        lightGradRed.setFinalStop(QPointF(fw, y1));

#ifdef _USE_CLIPPER
        if(yv < y1)
        {
          // Red section:
          pr.setTop(fw); pr.setHeight(yv);
          p.fillRect(pr, QBrush(darkGradRed));     // dark red  
          pr.setTop(yv); pr.setHeight(y1-yv);
          p.fillRect(pr & rect, QBrush(lightGradRed));     // light red
          
          // Yellow section:
          pr.setTop(y1); pr.setHeight(y2-y1);
          p.fillRect(pr & rect, QBrush(lightGradYellow));     // light yellow
          
          // Green section:
          pr.setTop(y2); pr.setHeight(h-y2);
          p.fillRect(pr & rect, QBrush(lightGradGreen));     // light green
        }
        else
        if(yv < y2)
        {
          // Red section:
          pr.setTop(fw); pr.setHeight(y1);
          p.fillRect(pr & rect, QBrush(darkGradRed));     // dark red  
          
          // Yellow section:
          pr.setTop(y1); pr.setHeight(yv-y1);
          p.fillRect(pr & rect, QBrush(darkGradYellow));     // dark yellow
          pr.setTop(yv); pr.setHeight(y2-yv);
          p.fillRect(pr & rect, QBrush(lightGradYellow));     // light yellow
          
          // Green section:
          pr.setTop(y2); pr.setHeight(h-y2);
          p.fillRect(pr & rect, QBrush(lightGradGreen));     // light green
        }
        else
        //if(yv <= y3)   
        {
          // Red section:
          pr.setTop(fw); pr.setHeight(y1);
          p.fillRect(pr & rect, QBrush(darkGradRed));     // dark red  
          
          // Yellow section:
          pr.setTop(y1); pr.setHeight(y2-y1);
          p.fillRect(pr & rect, QBrush(darkGradYellow));     // dark yellow
          
          // Green section:
          pr.setTop(y2); pr.setHeight(yv-y2);
          p.fillRect(pr & rect, QBrush(darkGradGreen));     // dark green
          pr.setTop(yv); pr.setHeight(h-yv);
          p.fillRect(pr & rect, QBrush(lightGradGreen));     // light green
        }
      }  
      else     // Meter type is linear...
      {
        pr.setTop(fw); pr.setHeight(yv);
        p.fillRect(pr & rect, QBrush(darkGradGreen));   // dark green
        pr.setTop(yv); pr.setHeight(h-yv);
        p.fillRect(pr & rect, QBrush(lightGradGreen));   // light green
      }

#else   // NOT    _USE_CLIPPER

        if(yv < y1)
        {
          // Red section:
          {
            QPainterPath path; path.addRect(fw, fw, w, yv); path &= drawPath;
            if(!path.isEmpty())
              p.fillPath(path, QBrush(darkGradRed));       // dark red
          }
          {
            QPainterPath path; path.addRect(fw, yv, w, y1-yv); path &= drawPath;
            if(!path.isEmpty())
              p.fillPath(path, QBrush(lightGradRed));       // light red
          }
          
          // Yellow section:
          {
            QPainterPath path; path.addRect(fw, y1, w, y2-y1); path &= drawPath;
            if(!path.isEmpty())
              p.fillPath(path, QBrush(lightGradYellow));   // light yellow
          }
          
          // Green section:
          {
            QPainterPath path; path.addRect(fw, y2, w, h-y2); path &= drawPath;
            if(!path.isEmpty())
              p.fillPath(path, QBrush(lightGradGreen));   // light green
          }
        }
        else
        if(yv < y2)
        {
          // Red section:
          {
            QPainterPath path; path.addRect(fw, fw, w, y1); path &= drawPath;
            if(!path.isEmpty())
              p.fillPath(path, QBrush(darkGradRed));       // dark red
          }
          
          // Yellow section:
          {
            QPainterPath path; path.addRect(fw, y1, w, yv-y1); path &= drawPath;
            if(!path.isEmpty())
              p.fillPath(path, QBrush(darkGradYellow));   // dark yellow
          }
          {
            QPainterPath path; path.addRect(fw, yv, w, y2-yv); path &= drawPath;
            if(!path.isEmpty())
              p.fillPath(path, QBrush(lightGradYellow));   // light yellow
          }
          
          // Green section:
          {
            QPainterPath path; path.addRect(fw, y2, w, h-y2); path &= drawPath;
            if(!path.isEmpty())
              p.fillPath(path, QBrush(lightGradGreen));   // light green
          }
        }
        else
        //if(yv <= y3)   
        {
          // Red section:
          {
            QPainterPath path; path.addRect(fw, fw, w, y1); path &= drawPath;
            if(!path.isEmpty())
              p.fillPath(path, QBrush(darkGradRed));       // dark red
          }
          
          // Yellow section:
          {
            QPainterPath path; path.addRect(fw, y1, w, y2-y1); path &= drawPath;
            if(!path.isEmpty())
              p.fillPath(path, QBrush(darkGradYellow));   // dark yellow
          }
          
          // Green section:
          {
            QPainterPath path; path.addRect(fw, y2, w, yv-y2); path &= drawPath;
            if(!path.isEmpty())
              p.fillPath(path, QBrush(darkGradGreen));   // dark green
          }
          {
            QPainterPath path; path.addRect(fw, yv, w, h-yv); path &= drawPath;
            if(!path.isEmpty())
              p.fillPath(path, QBrush(lightGradGreen));   // light green
          }
        }

        // Separators: 
        {
          QRect r(0, y1, w, 1); r &= rect;
          if(!r.isNull())
            p.fillRect(r, separator_color);  
        }  
        {
          QRect r(0, y2, w, 1); r &= rect;
          if(!r.isNull())
            p.fillRect(r, separator_color);  
        }  
      }  
      else      // Meter type is linear...
      {
        darkGradGreen.setStart(QPointF(fw, fw));
        darkGradGreen.setFinalStop(QPointF(fw, h));

        lightGradGreen.setStart(QPointF(fw, fw));
        lightGradGreen.setFinalStop(QPointF(fw, h));

        {
          QPainterPath path; path.addRect(fw, fw, w, yv); path &= drawPath;
          if(!path.isEmpty())
            p.fillPath(path, QBrush(darkGradGreen));   // dark green
        }
        {
          QPainterPath path; path.addRect(fw, yv, w, h-yv); path &= drawPath;
          if(!path.isEmpty())
            p.fillPath(path, QBrush(lightGradGreen));   // light green
        }
      }  

#endif  // NOT   _USE_CLIPPER

}

//---------------------------------------------------------
//   resizeEvent
//---------------------------------------------------------

void Meter::resizeEvent(QResizeEvent* ev)
    {  
      // For some reason upon resizing we get double calls here and in paintEvent.
      //printf("Meter::resizeEvent w:%d h:%d\n", ev->size().width(), ev->size().height());  
      cur_yv = -1;  // Force re-initialization.
      QFrame::resizeEvent(ev);
      update();
    }

//---------------------------------------------------------
//   mousePressEvent
//---------------------------------------------------------

void Meter::mousePressEvent(QMouseEvent*)
      {
      emit mousePress();
      }

} // namespace MusEGui
