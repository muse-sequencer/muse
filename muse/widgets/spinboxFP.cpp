//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: spinboxFP.cpp,v 1.1.1.1 2003/10/27 18:55:03 wschweer Exp $
//    (C) Copyright 2001 Werner Schweer (ws@seh.de)
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

#include <stdio.h>
#include <cmath>

//#include <QtGui>
//#include <QDoubleValidator>
//#include <QLineEdit>

#include "spinboxFP.h"

namespace MusEGui {

//---------------------------------------------------------
//   SpinBoxFP
//---------------------------------------------------------

SpinBoxFP::SpinBoxFP(QWidget* parent)
   //: QSpinBox(parent)
   : QDoubleSpinBox(parent)
      {
        //validator = new QDoubleValidator(this);
        //lineEdit()->setValidator(validator = new QDoubleValidator(this));
        //validator->setNotation(QDoubleValidator::StandardNotation);
        
        //_decimals = 0;
        setDecimals(0);
        
        connect(this, SIGNAL(valueChanged(double)), SLOT(valueChange(double)));
      }

SpinBoxFP::SpinBoxFP(int minValue, int maxValue, int step, QWidget* parent)
//SpinBoxFP::SpinBoxFP(double minValue, double maxValue, double step, QWidget* parent)
   //: QSpinBox(parent)
   : QDoubleSpinBox(parent)
      {
        //validator = new QDoubleValidator(this);
        //lineEdit()->setValidator(validator = new QDoubleValidator(this));
        //validator->setNotation(QDoubleValidator::StandardNotation);
        
        //_decimals = 0;
        QDoubleSpinBox::setDecimals(0);
        
        setRange(minValue, maxValue);
        setSingleStep(step);
        
        connect(this, SIGNAL(valueChanged(double)), SLOT(valueChange(double)));
      }

//---------------------------------------------------------
//   valueChange
//---------------------------------------------------------

void SpinBoxFP::valueChange(double)
{
        double div = exp10(decimals());
        emit valueChanged(int(value() * div));
}

//---------------------------------------------------------
//   setValue
//---------------------------------------------------------

void SpinBoxFP::setValue(int val)
      {
        double div = exp10(decimals());
        QDoubleSpinBox::setValue(double(val) /  div );
      }

//---------------------------------------------------------
//   intValue
//---------------------------------------------------------

int SpinBoxFP::intValue()
      {
        double div = exp10(decimals());
        return int(value() * div);
      }

//---------------------------------------------------------
//   setDecimals
//---------------------------------------------------------

void SpinBoxFP::setDecimals(int val)
      {
      //_decimals = val;
      
      //updateDisplay();
      //interpretText();  // TODO: Check - is this what we need? Will send out signals?
      //setValue(value());    // Try this. "setValue() will emit valueChanged() if the new value is different from the old one."
      
      QDoubleSpinBox::setDecimals(val);
      double step = 1.0 / exp10(val);
      setSingleStep(step);
      }

/*
//---------------------------------------------------------
//   validate
//---------------------------------------------------------

QValidator::State SpinBoxFP::validate(QString& input, int& pos) const
{
  // Must set these dynamically as settings may have changed.
  validator->setRange(minimum(), maximum(), _decimals);
  
  QValidator::State s = validator->validate(input, pos);
  return s;
}

//---------------------------------------------------------
//   mapValueToText
//---------------------------------------------------------

QString SpinBoxFP::textFromValue(int value) const
      {
      if (_decimals) {
            QString s;
            int div = int(exp10(_decimals));
//            printf("val %d, prec %d, div %d\n", value, _precision, div);
            
            s.sprintf("%d.%0*d", value/div, _decimals, value%div);
            //s.sprintf("%0*f", value, _decimals);
            
            return s;
            }
      return QSpinBox::textFromValue(value);
      }

//---------------------------------------------------------
//   mapTextToValue
//---------------------------------------------------------

int SpinBoxFP::valueFromText(const QString& text) const
      {
      //QString qs = cleanText();
      if (_decimals) {
            //const char* s = qs.toLatin1();
            //const char* s = cleanText().toAscii().data();
            
            //int a, b;
            bool ok;
            double f = text.toDouble(&ok);
            
            //int n = sscanf(s, "%d.%d", &a, &b);
            //int n = sscanf(s, "%f", &f);
            
            //if (n != 2) {
            //if (n != 1) {
            if (!ok) {
            
                  // *ok = false;
                  //return 0;
                  // TODO: Check - Hmm, no OK parameter. Why return 0? Let's try: 
                  // Keep returning the current value until something valid comes in...
                  return value();
                  }
            
            //int div = int(exp10(_decimals));
            double div = int(exp10(_decimals));
            
            //return a * div + b;
            return (f * div);
            
            }
      return QSpinBox::valueFromText(text);
      }

*/

} // namespace MusEGui
