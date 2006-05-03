//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: floatlabel.cpp,v 1.1 2004/09/14 18:17:47 wschweer Exp $
//  (C) Copyright 1999 Werner Schweer (ws@seh.de)
//=========================================================

#include "doublelabel.h"
#include "utils.h"

//---------------------------------------------------------
//   DoubleLabel
//---------------------------------------------------------

DoubleLabel::DoubleLabel(QWidget* parent, const char* name)
   : Dentry(parent, name), _specialText("---")
      {
      min        = 0.0;
      max        = 1.0;
      _precision = 3;
      setValue(0.0);
      }

DoubleLabel::DoubleLabel(double _val, double m, double mx, QWidget* parent)
   : Dentry(parent), _specialText("---")
      {
      min = m;
      max = mx;
      _precision = 3;
      setValue(_val);
      }

//---------------------------------------------------------
//   setString
//---------------------------------------------------------

bool DoubleLabel::setString(double v)
      {
      QString s;
      if (v < min || v > max) {
            setText(_specialText);
            return true;
            }
      s.setNum(v, 'f', _precision);
      if (!_suffix.isEmpty()) {
            s += " ";
            s += _suffix;
            }

      setText(s);
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
      if (val + 1.0 < max) {
            setValue(val + 1.0);
            emit valueChanged(val, _id);
            }
      }

//---------------------------------------------------------
//   decValue
//---------------------------------------------------------

void DoubleLabel::decValue(double)
      {
      if (val - 1.0 > min) {
            setValue(val - 1.0);
            emit valueChanged(val, _id);
            }
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
      int n = _precision + 3;
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
