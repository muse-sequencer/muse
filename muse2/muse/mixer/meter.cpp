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
      
      double range = maxScale - minScale;

      int fw = frameWidth();
      int w = width() - 2*fw;
      int h  = height() - 2*fw;
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
      p.setPen(QColor(1,149,176));//floating vu levels
      p.drawLine(0, ymax, w, ymax);
      int y1 = int((maxScale - redScale) * h / range);
      int y2 = int((maxScale - yellowScale) * h / range);
      p.setPen(QColor(209,0,0));//0 db
	  p.drawLine(0, y1, w, y1);
      p.setPen(QColor(209,197,0));//-10 db
	  p.drawLine(0, y2, w, y2);
}

//---------------------------------------------------------
//   drawVU
//---------------------------------------------------------

void Meter::drawVU(QPainter& p, int w, int h, int yv)
{
	  QColor bgColor = QColor(0,12,16);
      if(mtype == DBMeter) 
      {
        double range = maxScale - minScale;
        int y1 = int((maxScale - redScale) * h / range);
        int y2 = int((maxScale - yellowScale) * h / range);
	    QLinearGradient vuGrad(QPointF(0, 0), QPointF(0, h));
	    vuGrad.setColorAt(1, Qt::white);
	    vuGrad.setColorAt(0.9, Qt::blue);
	    vuGrad.setColorAt(0, Qt::red);
	    p.fillRect(0, yv,  w, h,        QBrush(vuGrad));
        
       // if(yv < y1)
       // {
       //   // Red section:
       p.fillRect(0, 0,  w, yv,        QBrush(bgColor));     // dark red  
       //   //p.fillRect(0, yv, w, y1-yv,     QBrush(0xff0000));     // light red
       //   
       //   // Yellow section:
       //   //p.fillRect(0, y1, w, y2-y1,     QBrush(0xffff00));     // light yellow
       //   
       //   // Green section:
       //   //p.fillRect(0, y2, w, h-y2,      QBrush(0x00ff00));     // light green

	   //   QLinearGradient vuGrad(QPointF(0, 0), QPointF(0, yv));
	   //   vuGrad.setColorAt(1, Qt::white);
	   //   vuGrad.setColorAt(0.9, Qt::blue);
	   //   vuGrad.setColorAt(0, Qt::red);
	   //   p.fillRect(0, 0,  w, y1,        QBrush(vuGrad));
       // }
       // else
       // if(yv < y2)
       // {
       //   // Red section:
       //   p.fillRect(0, 0,  w, y1,        QBrush(bgColor));     // dark red  
       //   
       //   // Yellow section:
       //   p.fillRect(0, y1, w, yv-y1,     QBrush(bgColor));     // dark yellow
       //   //p.fillRect(0, yv, w, y2-yv,     QBrush(0xffff00));     // light yellow
       //   
       //   // Green section:
       //   //p.fillRect(0, y2, w, h-y2,      QBrush(0x00ff00));     // light green
	   //   QLinearGradient vuGrad(QPointF(0, 0), QPointF(0, yv));
	   //   vuGrad.setColorAt(1, Qt::white);
	   //   vuGrad.setColorAt(0.9, Qt::blue);
	   //   vuGrad.setColorAt(0, Qt::red);
	   //   p.fillRect(0, 0,  w, y1,        QBrush(vuGrad));
       // }
       // else
       // //if(yv <= y3)   
       // {
       //   // Red section:
       //   p.fillRect(0, 0,  w, y1,        QBrush(bgColor));     // dark red  
       //   
       //   // Yellow section:
       //   p.fillRect(0, y1, w, y2-y1,     QBrush(bgColor));     // dark yellow
       //   
       //   // Green section:
       //   p.fillRect(0, y2, w, yv-y2,     QBrush(bgColor));     // dark green
       //   //p.fillRect(0, yv, w, h-yv,      QBrush(0x00ff00));     // light green
	   //   QLinearGradient vuGrad(QPointF(0, yv), QPointF(0, h));
	   //   vuGrad.setColorAt(1, Qt::white);
	   //   vuGrad.setColorAt(0.9, Qt::blue);
	   //   vuGrad.setColorAt(0, Qt::red);
	   //   p.fillRect(0, yv,  w, h-yv,        QBrush(vuGrad));
       // }
      }  
      else
      {
        p.fillRect(0, 0,  w, yv,   QBrush(bgColor));   // dark green
        p.fillRect(0, yv, w, h-yv, QBrush(0x00ff00));   // light green
      }
}

//---------------------------------------------------------
//   resizeEvent
//---------------------------------------------------------

void Meter::resizeEvent(QResizeEvent* /*ev*/)
    {
    
    }

//---------------------------------------------------------
//   mousePressEvent
//---------------------------------------------------------

void Meter::mousePressEvent(QMouseEvent*)
      {
      emit mousePress();
      }

