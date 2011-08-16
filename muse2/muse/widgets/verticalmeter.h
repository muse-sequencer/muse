//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: meter.h,v 1.1.1.1.2.2 2009/05/03 04:14:00 terminator356 Exp $
//
//  (C) Copyright 2000 Werner Schweer (ws@seh.de)
//=========================================================

#ifndef __VERTICALMETER_H__
#define __VERTICALMETER_H__

#include "meter.h"

class QPaintEvent;
class QResizeEvent;
class QMouseEvent;
class QPainter;

class VerticalMeter : public Meter {
      Q_OBJECT
    
   private:
      MeterType mtype;
      bool overflow;
      double val;
      double maxVal;
      double minScale, maxScale;
      int yellowScale, redScale;

      void drawVU(QPainter& p, int, int, int);

      
      void paintEvent(QPaintEvent*);
      void resizeEvent(QResizeEvent*);

   public slots:
      void resetPeaks();
      void setVal(double);

   public:
      VerticalMeter(QWidget* parent, MeterType type = DBMeter);
      void setRange(double min, double max);
      };
#endif

