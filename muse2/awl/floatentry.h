//=============================================================================
//  Awl
//  Audio Widget Library
//  $Id:$
//
//  Copyright (C) 1999-2011 by Werner Schweer and others
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License
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
//=============================================================================

#ifndef __FLOATLABEL_H__
#define __FLOATLABEL_H__

#include <QLineEdit>

class QMouseEvent;
class QTimer;
class QWheelEvent;

namespace Awl {

//---------------------------------------------------------
//   FloatEntry
//---------------------------------------------------------

class FloatEntry : public QLineEdit {
      Q_OBJECT
      Q_PROPERTY(int id READ id WRITE setId)

      Q_PROPERTY(double minValue READ minValue WRITE setMinValue)
      Q_PROPERTY(double maxValue READ maxValue WRITE setMaxValue)

      Q_PROPERTY(QString specialText READ specialText WRITE setSpecialText)
      Q_PROPERTY(QString suffix      READ suffix      WRITE setSuffix)
      Q_PROPERTY(int precision       READ precision   WRITE setPrecision)
      Q_PROPERTY(bool log            READ log         WRITE setLog)

      int button;
      int starty;
      QTimer* timer;
      double evx;
      int timecount;
      double _minValue, _maxValue;
      QString _specialText;   // text to show if value outside min,max
      QString _suffix;
      int _precision;
      bool _log;

      virtual void wheelEvent(QWheelEvent*);
      virtual void mousePressEvent(QMouseEvent*);
      virtual void mouseMoveEvent(QMouseEvent*);
      virtual void mouseDoubleClickEvent(QMouseEvent*);
      virtual void mouseReleaseEvent(QMouseEvent*);
      virtual void setSValue(const QString&);
      virtual bool setString(double);
      virtual void incValue(double);
      virtual void decValue(double);

      void updateValue();

   protected:
      int _id;
      double _value;
      virtual void valueChange();

   private slots:
      void repeat();

   protected slots:
      void endEdit();

   public slots:
      virtual void setValue(double);

   signals:
      void valueChanged(double, int);

   public:
      FloatEntry(QWidget*);
      virtual QSize sizeHint() const;
      virtual double value() const;
      int id() const                        { return _id; }
      void setId(int i)                     { _id = i; }
      double minValue() const               { return _minValue; }
      double maxValue() const               { return _maxValue; }
      void setMinValue(double v)            { _minValue = v; }
      void setMaxValue(double v)            { _maxValue = v; }
      void setRange(double a, double b) {
            _minValue = a;
            _maxValue = b;
            }
      int precision() const                 { return _precision; }
      void setPrecision(int val);
      QString specialText() const           { return _specialText; }
      void setSpecialText(const QString& s) {
            _specialText = s;
            update();
            }
      QString suffix() const                { return _suffix; }
      void setSuffix(const QString& s)      { _suffix = s; }
      bool log() const                      { return _log;      }
      void setLog(bool v)                   { _log = v;         }
      };

}

#endif

