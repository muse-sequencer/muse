//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: meter.cpp,v 1.4.2.2 2009/05/03 04:14:00 terminator356 Exp $
//
//  (C) Copyright 2000 Werner Schweer (ws@seh.de)
//=========================================================

#include <stdio.h>
#include <cmath>
#include <qpainter.h>
//Added by qt3to4:
#include <QResizeEvent>
#include <QMouseEvent>
#include <QFrame>

#include "meter.h"
#include "gconfig.h"
#include "fastlog.h"

//---------------------------------------------------------
//   Meter
//---------------------------------------------------------

Meter::Meter(QWidget* parent, MeterType type)
   : QFrame(parent) //Qt::WNoAutoErase
      {
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

//void Meter::setVal(int v, int max, bool ovl)
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
      drawVU(width(), height());
      update();
      }

//---------------------------------------------------------
//   paintEvent
//---------------------------------------------------------

void Meter::paintEvent(QPaintEvent*)
      {
      QPainter p;
      p.begin(this);
      double range = maxScale - minScale;

      int fw = frameWidth();
      int h  = height() - 2* fw;
      int yv;
      if(mtype == DBMeter) 
        yv = val == 0 ? h : int(((maxScale - (fast_log10(val) * 20.0)) * h)/range);
      else
        yv = val == 0 ? h : int(((maxScale - val) * h)/range);
      
      // Orcan - check
      //void bitBlt ( QImage * dst, int dx, int dy, const QImage * src, int sx = 0, int sy = 0, int sw = -1, int sh = -1, Qt::ImageConversionFlags flags = Qt::AutoColor )

      //bitBlt(this, fw, fw,    &bgPm, 0, 0,  -1, yv, true); //   CopyROP, true); ddskrjo
      //bitBlt(this, fw, fw+yv, &fgPm, 0, yv, -1, h-yv, true); //CopyROP, true); ddskrjo

      p.drawImage(fw, fw,    bgPm.toImage(), 0, 0,   -1, yv  );
      p.drawImage(fw, fw+yv, fgPm.toImage(), 0, yv,  -1, h-yv);

      int ymax;
      if(mtype == DBMeter) 
        ymax = maxVal == 0 ? 0 : int(((maxScale - (fast_log10(maxVal) * 20.0)) * h)/range);
      else
        ymax = maxVal == 0 ? 0 : int(((maxScale - maxVal) * h)/range);
      p.setPen(Qt::white);
      p.drawLine(0, ymax, width()-2*fw, ymax);
      }

//---------------------------------------------------------
//   drawVU
//---------------------------------------------------------

void Meter::drawVU(int w, int h)
      {
      double range = maxScale - minScale;
      int fw = frameWidth();
      w -= 2*fw;
      h -= 2*fw;

      bgPm = QPixmap(QSize(w, h));
      fgPm = QPixmap(QSize(w, h));

      QPainter p1(&fgPm);
      QPainter p2(&bgPm);
      
      if(mtype == LinMeter)
      {
        p1.fillRect(0, 0, w, h, QBrush(0x00ff00));  // green
        p2.fillRect(0, 0, w, h, QBrush(0x007000));  // green
      }  
      else
      {
        int y1 = int((maxScale - redScale) * h / range);
        int y2 = int((maxScale - yellowScale) * h / range);
        int y3 = h;
        p1.fillRect(0, 0,  w, y1,    QBrush(0xff0000));  // red
        p1.fillRect(0, y1, w, y2-y1, QBrush(0xffff00));  // yellow
        p1.fillRect(0, y2, w, y3-y2, QBrush(0x00ff00));  // green
  
        p2.fillRect(0, 0,  w, y1,    QBrush(0x8e0000));  // red
        p2.fillRect(0, y1, w, y2-y1, QBrush(0x8e8e00));  // yellow
        p2.fillRect(0, y2, w, y3-y2, QBrush(0x007000));  // green
      }
    }

//---------------------------------------------------------
//   resizeEvent
//---------------------------------------------------------

void Meter::resizeEvent(QResizeEvent* ev)
      {
      int h  = ev->size().height();
      int w  = ev->size().width();
      drawVU(w, h);
    }

//---------------------------------------------------------
//   mousePressEvent
//---------------------------------------------------------

void Meter::mousePressEvent(QMouseEvent*)
      {
      emit mousePress();
      }

