//=============================================================================
//  MusE
//  Linux Music Editor
//  $Id:$
//
//  Copyright (C) 2002-2006 by Werner Schweer and others
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License version 2.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//=============================================================================

#ifndef __FLOATLABEL_H__
#define __FLOATLABEL_H__

#include "dentry.h"

//---------------------------------------------------------
//   FloatLabel
//---------------------------------------------------------

class FloatLabel : public Dentry {
      Q_PROPERTY(float minValue READ minValue WRITE setMinValue )
      Q_PROPERTY(float maxValue READ maxValue WRITE setMaxValue )
      Q_PROPERTY(QString specialText READ specialText WRITE setSpecialText )
      Q_PROPERTY(QString suffix READ suffix WRITE setSuffix )
      Q_PROPERTY(int precision READ precision WRITE setPrecision )

      float min, max;
      QString _specialText;   // text to show if value outside min,max
      QString _suffix;
      int _precision;
      Q_OBJECT

      virtual bool setSValue(const QString&);
      virtual bool setString(double val);
      virtual void incValue(double);
      virtual void decValue(double);

   public:
      DoubleLabel(QWidget*, const char* name = 0);
      DoubleLabel(double val, double min, double max, QWidget*);
      virtual QSize sizeHint() const;
      double minValue() const               { return min; }
      double maxValue() const               { return max; }
      void setMinValue(double v)            { min = v; }
      void setMaxValue(double v)            { max = v; }
      void setRange(double a, double b)     { min = a; max = b; }
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

#endif
