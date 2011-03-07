//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: spinboxFP.cpp,v 1.1.1.1 2003/10/27 18:55:03 wschweer Exp $
//    (C) Copyright 2001 Werner Schweer (ws@seh.de)
//=========================================================

#include <stdio.h>
#include <cmath>

#include <qvalidator.h>
#include "spinboxFP.h"

//---------------------------------------------------------
//   SpinBoxFP
//---------------------------------------------------------

SpinBoxFP::SpinBoxFP(QWidget* parent, const char* name)
   : QSpinBox(parent, name)
      {
      _precision = 0;
      setValidator(new QDoubleValidator(this));
      }

SpinBoxFP::SpinBoxFP(int minValue, int maxValue, int step, QWidget* parent, const char* name)
   : QSpinBox(minValue, maxValue, step, parent, name)
      {
      _precision = 0;
      setValidator(new QDoubleValidator(this));
      }

//---------------------------------------------------------
//   setPrecision
//---------------------------------------------------------

void SpinBoxFP::setPrecision(int val)
      {
      _precision = val;
      updateDisplay();
      }

//---------------------------------------------------------
//   mapValueToText
//---------------------------------------------------------

QString SpinBoxFP::mapValueToText(int value)
      {
      if (_precision) {
            QString s;
            int div = int(exp10(_precision));
//            printf("val %d, prec %d, div %d\n", value, _precision, div);
            s.sprintf("%d.%0*d", value/div, _precision, value%div);
            return s;
            }
      return QSpinBox::mapValueToText(value);
      }

//---------------------------------------------------------
//   mapTextToValue
//---------------------------------------------------------

int SpinBoxFP::mapTextToValue(bool* ok)
      {
      QString qs = cleanText();
      if (_precision) {
            const char* s = qs.latin1();
            int a, b;
            int n = sscanf(s, "%d.%d", &a, &b);
            if (n != 2) {
                  *ok = false;
                  return 0;
                  }
            int div = int(exp10(_precision));
            return a * div + b;
            }
      return QSpinBox::mapTextToValue(ok);
      }

