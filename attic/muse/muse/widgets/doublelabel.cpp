//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: doublelabel.cpp,v 1.1.1.1.2.2 2008/08/18 00:15:26 terminator356 Exp $
//  (C) Copyright 1999 Werner Schweer (ws@seh.de)
//=========================================================

#include <stdio.h>

#include "doublelabel.h"
#include <qvalidator.h>
#include <qpalette.h>
#include <stdio.h>
#include <values.h>

#include "utils.h"

//---------------------------------------------------------
//   DoubleLabel
//---------------------------------------------------------

DoubleLabel::DoubleLabel(QWidget* parent, const char* name)
   : Dentry(parent, name), _specialText("---")
      {
      min        = 0.0;
      max        = 1.0;
      _off        = -1.0;
      _precision = 3;
      setValue(0.0);
      }

DoubleLabel::DoubleLabel(double _val, double m, double mx, QWidget* parent)
   : Dentry(parent), _specialText("---")
      {
      min = m;
      max = mx;
      _off = m - 1.0;
      _precision = 3;
      setValue(_val);
      }

//---------------------------------------------------------
//   setOff
//---------------------------------------------------------

void DoubleLabel::setOff(double v)
{
  _off = v;
  setString(val);
}

//---------------------------------------------------------
//   calcIncrement()
//---------------------------------------------------------

double DoubleLabel::calcIncrement() const
{
  double dif;
  if(max - min > 0)
    dif = max - min;
  else  
    dif = min - max;
    
  if(dif <= 10.0)
    return 0.1;   
  else
  if(dif <= 100.0)
    return 1.0;
  else
    return 10.0;  
}

//---------------------------------------------------------
//   setString
//---------------------------------------------------------

bool DoubleLabel::setString(double v)
      {
      if(v <= _off || v > max) 
      {
            setText(_specialText);
            return true;
      }
      else
      if(v < min) 
      {
        setText(QString("---"));
        return true;
      } 
      else
      {
        QString s;
        s.setNum(v, 'f', _precision);
        if (!_suffix.isEmpty()) {
              s += " ";
              s += _suffix;
              }
  
        setText(s);
      }  
      return false;
      }

//---------------------------------------------------------
//   setSValue
//---------------------------------------------------------

bool DoubleLabel::setSValue(const QString& s)
      {
      bool ok;
      double v = s.toDouble(&ok);
      if (ok && (v != val)) {
            if (v < min)
                  v = min;
            if (v > max)
                  v = max;
            setValue(v);
            emit valueChanged(val, _id);
            }
      return false;
      }

//---------------------------------------------------------
//   incValue
//---------------------------------------------------------

void DoubleLabel::incValue(double)
      {
      if(val >= max)
        return;
      double inc = calcIncrement();
      if(val + inc >= max)
        setValue(max);
      else  
        setValue(val + inc);
      emit valueChanged(val, _id);
      }

//---------------------------------------------------------
//   decValue
//---------------------------------------------------------

void DoubleLabel::decValue(double)
      {
      if(val <= min)
        return;
      double inc = calcIncrement();
      if(val - inc <= min)
        setValue(min);
      else  
        setValue(val - inc);
      emit valueChanged(val, _id);
      }

//---------------------------------------------------------
//   setPrecision
//---------------------------------------------------------

void DoubleLabel::setPrecision(int v)
      {
      _precision = v;
      setString(val);
      }

//---------------------------------------------------------
//   sizeHint
//---------------------------------------------------------

QSize DoubleLabel::sizeHint() const
      {
      QFontMetrics fm = fontMetrics();
      int h           = fm.height() + 4;
      int n = _precision + 6;
#if 0
      double aval = fabs(val);
      if (aval >= 10.0)
            ++n;
      if (aval >= 100.0)
            ++n;
      if (aval >= 1000.0)
            ++n;
      if (aval >= 10000.0)
            ++n;
      if (aval >= 100000.0)
            ++n;
#endif
      int w = fm.width(QString("-0.")) + fm.width('0') * n + 6;
      return QSize(w, h);
      }
