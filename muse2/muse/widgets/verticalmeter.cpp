//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: meter.cpp,v 1.4.2.2 2009/05/03 04:14:00 terminator356 Exp $
//
//  (C) Copyright 2000 Werner Schweer (ws@seh.de)
//=========================================================

#include <stdio.h>
#include <cmath>

#include <QMouseEvent>
#include <QPainter>
#include <QResizeEvent>

#include "verticalmeter.h"
#include "gconfig.h"
#include "fastlog.h"

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
      minScale    = mtype == DBMeter ? config.minMeter : 0.0;      // min value in dB or int
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

void VerticalMeter::paintEvent(QPaintEvent* /*ev*/)
      {
      // TODO: Could make better use of event rectangle, for speed.
      
      QPainter p(this);
      
      double range = maxScale - minScale;

      int fw = frameWidth();
      int w  = width() - 2*fw;
      int h  = height() - 2*fw;
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
      p.setPen(Qt::white);
      p.drawLine(xcenter, 0, xcenter, h);
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
        
        if(xv < x1)
        {
          // Red section:
          p.fillRect(0, 0,  xv, h,        QBrush(0x8e0000));     // dark red
          p.fillRect(xv, 0, x1-xv, h,     QBrush(0xff0000));     // light red
          
          // Yellow section:
          p.fillRect(x1, 0, x2-x1, h,     QBrush(0xffff00));     // light yellow
          
          // Green section:
          p.fillRect(x2, 0, w-x2, h,      QBrush(0x00ff00));     // light green
        }
        else
        if(xv < x2)
        {
          // Red section:
          p.fillRect(0, 0,  x1, h,        QBrush(0x8e0000));     // dark red
          
          // Yellow section:
          p.fillRect(x1, 0, xv-x1, h,     QBrush(0x8e8e00));     // dark yellow
          p.fillRect(xv, 0, x2-xv, h,     QBrush(0xffff00));     // light yellow
          
          // Green section:
          p.fillRect(x2, 0, w-x2, h,      QBrush(0x00ff00));     // light green
        }
        else
        //if(yv <= y3)   
        {
          // Red section:
          p.fillRect(0, 0,  x1, h,        QBrush(0x8e0000));     // dark red
          
          // Yellow section:
          p.fillRect(x1, 0, x2-x1, h,     QBrush(0x8e8e00));     // dark yellow
          
          // Green section:
          p.fillRect(x2, 0, xv-x2, h,     QBrush(0x007000));     // dark green
          p.fillRect(xv, 0, w-xv, h,      QBrush(0x00ff00));     // light green
        }
      }  
      else
      {
        p.fillRect(0, 0,  xv, h,   QBrush(0x00ff00));   // dark green
        p.fillRect(xv, 0, w-xv, h, QBrush(0x007000));   // light green
      }
}

//---------------------------------------------------------
//   resizeEvent
//---------------------------------------------------------

void VerticalMeter::resizeEvent(QResizeEvent* /*ev*/)
    {

    }
