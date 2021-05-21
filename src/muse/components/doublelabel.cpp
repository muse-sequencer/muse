//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: doublelabel.cpp,v 1.1.1.1.2.2 2008/08/18 00:15:26 terminator356 Exp $
//  (C) Copyright 1999 Werner Schweer (ws@seh.de)
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

#include "muse_math.h"

#include <QWidget>
#include <QKeyEvent>

#include "doublelabel.h"

namespace MusEGui {

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
      QString sTrimmed = s.trimmed();
      if (sTrimmed.contains(_suffix))
          sTrimmed = sTrimmed.remove(_suffix).trimmed();
      double v = sTrimmed.toDouble(&ok);
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

void DoubleLabel::incValue(int steps)
      {
      if(val >= max)
        return;
      double inc = calcIncrement() * double(steps);
      if(val + inc >= max)
        setValue(max);
      else  
        setValue(val + inc);
      emit valueChanged(val, _id);
      }

//---------------------------------------------------------
//   decValue
//---------------------------------------------------------

void DoubleLabel::decValue(int steps)
      {
      if(val <= min)
        return;
      double inc = calcIncrement() * double(steps);
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
      updateGeometry();
      setString(val);
      }

//---------------------------------------------------------
//   sizeHint
//---------------------------------------------------------

QSize DoubleLabel::sizeHint() const
      {
      QFontMetrics fm = fontMetrics();
      int h           = fm.height() + 9;
      int n = _precision;

      ++n;  // For some reason I have to add one digit. Shouldn't have to.
      double aval = fmax(fabs(max), fabs(min));
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

// Width() is obsolete. Qt >= 5.11 use horizontalAdvance().
#if QT_VERSION >= 0x050b00
      int w = fm.horizontalAdvance(QString("-0.")) + fm.horizontalAdvance('0') * n + 6;
#else
      int w = fm.width(QString("-0.")) + fm.width('0') * n + 6;
#endif
      if(!_suffix.isEmpty())
      {
// Width() is obsolete. Qt >= 5.11 use horizontalAdvance().
#if QT_VERSION >= 0x050b00
        w += fm.horizontalAdvance(QString(" ")) + fm.horizontalAdvance(_suffix);
#else
        w += fm.width(QString(" ")) + fm.width(_suffix);
#endif
      }
      return QSize(w, h);
      }

QSize DoubleLabel::minimumSizeHint() const
{
  return sizeHint();
}

void DoubleLabel::keyPressEvent(QKeyEvent* e)
{
  switch (e->key())
  {
    case Qt::Key_Space:
      {
      // This is mostly a work around to allow SPACE to be forwarded to parents, allowing
      // the start/stop shortcut to work even when this widget has focus.
      // Also, truthfylly, the input to this widget should only allow numbers
      e->ignore();
      return;
      }
    default:
      // Let ancestor handle it.
      e->ignore();
      Dentry::keyPressEvent(e);
      return;
    break;
  }
}


} // namespace MusEGui
