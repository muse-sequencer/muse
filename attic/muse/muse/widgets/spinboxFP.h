//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: spinboxFP.h,v 1.1.1.1.2.1 2008/05/21 00:28:54 terminator356 Exp $
//    (C) Copyright 2001 Werner Schweer (ws@seh.de)
//=========================================================

#ifndef __SPINBOXFP_H__
#define __SPINBOXFP_H__

#include <qspinbox.h>

//---------------------------------------------------------
//   SpinBoxFP
//---------------------------------------------------------

class SpinBoxFP : public QSpinBox {
      Q_OBJECT
      Q_PROPERTY( int precision READ precision WRITE setPrecision )

      int _precision; 

   protected:
      virtual QString mapValueToText(int value);
      virtual int mapTextToValue(bool* ok);

   public:
      SpinBoxFP(QWidget* parent=0, const char* name = 0);
      SpinBoxFP(int minValue, int maxValue, int step = 1, QWidget* parent=0, const char* name = 0);
      void setPrecision(int val);
      int precision() const { return _precision; }
      };

#endif

