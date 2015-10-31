//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: doublelabel.h,v 1.2.2.3 2008/08/18 00:15:26 terminator356 Exp $
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

#ifndef __DOUBLELABEL_H__
#define __DOUBLELABEL_H__

#include "dentry.h"

namespace MusEGui {

//---------------------------------------------------------
//   DoubleLabel
//---------------------------------------------------------

class DoubleLabel : public Dentry {
      Q_OBJECT

      Q_PROPERTY( double minValue READ minValue WRITE setMinValue )
      Q_PROPERTY( double maxValue READ maxValue WRITE setMaxValue )
      Q_PROPERTY( double offValue READ off WRITE setOff )
      Q_PROPERTY( QString specialText READ specialText WRITE setSpecialText )
      Q_PROPERTY( QString suffix READ suffix WRITE setSuffix )
      Q_PROPERTY( int precision READ precision WRITE setPrecision )

      double min, max, _off;
      QString _specialText;   // text to show if value outside min,max
      QString _suffix;
      int _precision;

      double calcIncrement() const;

      virtual bool setSValue(const QString&);
      virtual bool setString(double val);
      virtual void incValue(double);
      virtual void decValue(double);

   public:
      DoubleLabel(QWidget* parent = 0, const char* name = 0);
      DoubleLabel(double val, double min, double max, QWidget* parent = 0);
      virtual QSize sizeHint() const;
      virtual QSize minimumSizeHint () const;
      double minValue() const               { return min; }
      double maxValue() const               { return max; }
      double off()      const               { return _off; }
      void setMinValue(double v)            { min = v; }
      void setMaxValue(double v)            { max = v; }
      void setRange(double a, double b)     { _off = a - (min - _off); min = a; max = b; }
      void setOff(double v);
      int precision() const                 { return _precision; }
      void setPrecision(int val);
      QString specialText() const           { return _specialText; }
      void setSpecialText(const QString& s) {
            _specialText = s;
            update();
            }
      QString suffix() const                { return _suffix; }
      void setSuffix(const QString& s)      { _suffix = s; }
      };

} // namespace MusEGui

#endif
