//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: meter.h,v 1.1.1.1.2.2 2009/05/03 04:14:00 terminator356 Exp $
//
//  (C) Copyright 2000 Werner Schweer (ws@seh.de)
//=========================================================

#ifndef __METER_H__
#define __METER_H__

#include <q3frame.h>
#include <qpixmap.h>
//Added by qt3to4:
#include <QResizeEvent>
#include <QMouseEvent>

class Meter : public Q3Frame {
   public:
      enum MeterType {DBMeter, LinMeter};
   private:  
      MeterType mtype;
      bool overflow;
      double val;
      double maxVal;
      double minScale, maxScale;
      int yellowScale, redScale;

      QPixmap bgPm;
      QPixmap fgPm;  // for double buffering
      
      void drawVU(int w, int h);

      Q_OBJECT
      virtual void drawContents(QPainter* p);
      virtual void resizeEvent(QResizeEvent*);
      virtual void mousePressEvent(QMouseEvent*);

   public slots:
      void resetPeaks();
      //void setVal(int, int, bool);
      void setVal(double, double, bool);

   signals:
      void mousePress();

   public:
      Meter(QWidget* parent, MeterType type = DBMeter);
      void setRange(double min, double max);
      };
#endif

