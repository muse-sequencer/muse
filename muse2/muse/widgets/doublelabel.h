//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: doublelabel.h,v 1.2.2.3 2008/08/18 00:15:26 terminator356 Exp $
//  (C) Copyright 1999 Werner Schweer (ws@seh.de)
//=========================================================

#ifndef __DOUBLELABEL_H__
#define __DOUBLELABEL_H__

#include "dentry.h"

//---------------------------------------------------------
//   DoubleLabel
//---------------------------------------------------------

class DoubleLabel : public Dentry {
      Q_OBJECT

      Q_PROPERTY( double minValue READ minValue WRITE setMinValue )
      Q_PROPERTY( double maxValue READ maxValue WRITE setMaxValue )
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
      DoubleLabel(QWidget*, const char* name = 0);
      DoubleLabel(double val, double min, double max, QWidget*);
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

#endif
