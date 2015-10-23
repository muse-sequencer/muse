//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: intlabel.cpp,v 1.1.1.1.2.1 2008/08/18 00:15:26 terminator356 Exp $
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

#include <stdio.h>

#include "intlabel.h"
#include "utils.h"

namespace MusEGui {

//---------------------------------------------------------
//   IntLabel
//---------------------------------------------------------

IntLabel::IntLabel(int _val, int _min, int _max, QWidget* parent,
   int _off, const QString& str, int lPos)
      : Nentry(parent, str, lPos)
      {
      specialValue = "off";
      min = _min;
      max = _max;
      val = _val+1;           // dont optimize away
      off = _off;
      setValue(_val);
      int len = MusECore::num2cols(min, max);
      setSize(len);
      }

void IntLabel::setSpecialValueText(const QString& s)
      {
      specialValue = s;
      setString(val);
      }

//---------------------------------------------------------
//   setRange
//---------------------------------------------------------

void IntLabel::setRange(int mn, int mx)
{
  min = mn;
  max = mx;
  setSize(MusECore::num2cols(min, max));
  int v = val;
  if(val < mn)
    v = mn;
  else
  if(val > mx)
    v = mx;  
  setValue(v);
}

//---------------------------------------------------------
//   setString
//---------------------------------------------------------

bool IntLabel::setString(int v, bool editable)
      {
      if (v < min || v > max) {
            setText(QString("---"));
            return true;
            }
      else if (v == off) {
            if (editable)
                  setText(QString(""));
            else
                  setText(specialValue);
            }
      else {
            QString s;
            s.setNum(v);
            if (!editable)
                 s += suffix;
            setText(s);
            }
      return false;
      }

//---------------------------------------------------------
//   setSValue
//---------------------------------------------------------

bool IntLabel::setSValue(const QString& s)
      {
      int v;
      if (s == specialValue)
            v = off;
      else {
            bool ok;
            v = s.toInt(&ok);
            if (!ok)
                  return true;
            if (v < min)
                  v = min;
            if (v > max)
                  v = max;
            }
      if (v != val) {
            setValue(v);
            emit valueChanged(val);
            }
      return false;
      }

//---------------------------------------------------------
//   incValue
//---------------------------------------------------------

void IntLabel::incValue(int)
      {
      if (val < max) {
            setValue(val+1);
            emit valueChanged(val);
            }
      }

//---------------------------------------------------------
//   decValue
//---------------------------------------------------------

void IntLabel::decValue(int)
      {
      if (val > min) {
            setValue(val-1);
            emit valueChanged(val);
            }
      }

//---------------------------------------------------------
//   setOff
//---------------------------------------------------------

void IntLabel::setOff(int v)
      {
      off = v;
      setString(val);
      }

} // namespace MusEGui
