//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: floatlabel.h,v 1.1 2004/09/14 18:17:47 wschweer Exp $
//  (C) Copyright 1999 Werner Schweer (ws@seh.de)
//=========================================================

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
