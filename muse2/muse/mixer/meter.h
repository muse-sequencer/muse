//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: meter.h,v 1.1.1.1.2.2 2009/05/03 04:14:00 terminator356 Exp $
//
//  (C) Copyright 2000 Werner Schweer (ws@seh.de)
//=========================================================

#ifndef __METER_H__
#define __METER_H__

#include <QFrame>

class QResizeEvent;
class QMouseEvent;
class QPainter;

class Meter : public QFrame {
   public:
      enum MeterType {DBMeter, LinMeter};
   private:  
      MeterType mtype;
      bool overflow;
      double val;
      double maxVal;
      double minScale, maxScale;
      int yellowScale, redScale;
	  QColor green;
	  QColor red;
	  QColor yellow;
	  QColor bgColor;

      void drawVU(QPainter& p, int, int, int);

      Q_OBJECT
      void paintEvent(QPaintEvent*);
      virtual void resizeEvent(QResizeEvent*);
      virtual void mousePressEvent(QMouseEvent*);

   public slots:
      void resetPeaks();
      void setVal(double, double, bool);

   signals:
      void mousePress();

   public:
      Meter(QWidget* parent, MeterType type = DBMeter);
      void setRange(double min, double max);
      };
#endif

