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

#include "floatentry.h"
#include "fastlog.h"
#include "gconfig.h"

#include <QLineEdit>
#include <QMouseEvent>
#include <QTimer>

#define TIMER1    400
#define TIMER2    200
#define TIMEC     7
#define TIMER3    100
#define TIMEC2    20
#define TIMER4    50

namespace Awl {

//---------------------------------------------------------
//   FloatEntry
//---------------------------------------------------------

FloatEntry::FloatEntry(QWidget* parent)
   : QLineEdit(parent)
      {
      _id        = 0;
      _minValue  = 0.0;
      _maxValue  = 1.0;
      _log       = false;
      evx        = 1.0;
      _precision = 3;
      timer      = new QTimer(this);
      connect(timer, SIGNAL(timeout()), SLOT(repeat()));
      _value = 0.0f;
      connect(this, SIGNAL(returnPressed()), SLOT(endEdit()));
      setCursor(QCursor(Qt::ArrowCursor));
      updateValue();
      }

//---------------------------------------------------------
//   setString
//---------------------------------------------------------

bool FloatEntry::setString(double v)
      {
      QString s;
//      if (v < _minValue || v > _maxValue) {
      if (v < _minValue) {
            if (!_specialText.isEmpty())
                  setText(_specialText);
            return true;
            }
      s.setNum(v, 'f', _precision);
      if (!_suffix.isEmpty()) {
            // s += " ";
            s += _suffix;
            }
      setText(s);
      return false;
      }

//---------------------------------------------------------
//   setSValue
//---------------------------------------------------------

void FloatEntry::setSValue(const QString& s)
      {
      bool ok;
      double v = s.toFloat(&ok);
      if (ok && (v != _value)) {
            if (v < _minValue)
                  v = _minValue;
            if (v > _maxValue)
                  v = _maxValue;
            _value = v;
            updateValue();
            valueChange();
            }
      }

//---------------------------------------------------------
//   valueChange
//---------------------------------------------------------

void FloatEntry::valueChange()
      {
      emit valueChanged(value(), _id);
      }

//---------------------------------------------------------
//   incValue
//---------------------------------------------------------

void FloatEntry::incValue(double)
      {
      if (_value + 1.0 < _maxValue) {
            _value = _value + 1.0;
            updateValue();
            valueChange();
            }
      }

//---------------------------------------------------------
//   decValue
//---------------------------------------------------------

void FloatEntry::decValue(double)
      {
      if (_value - 1.0 > _minValue) {
            _value = _value - 1.0;
            updateValue();
            valueChange();
            }
      }

//---------------------------------------------------------
//   setPrecision
//---------------------------------------------------------

void FloatEntry::setPrecision(int v)
      {
      _precision = v;
      setString(_value);
      }

//---------------------------------------------------------
//   sizeHint
//---------------------------------------------------------

QSize FloatEntry::sizeHint() const
      {
      QFontMetrics fm = fontMetrics();
      int h           = fm.height() + 4;
      int n = _precision + 3;
      int w = fm.width(QString("-0.")) + fm.width('0') * n + 6;
      return QSize(w, h);
      }

//---------------------------------------------------------
//   endEdit
//---------------------------------------------------------

void FloatEntry::endEdit()
      {
      if (QLineEdit::isModified())
            setSValue(text());
      clearFocus();
      }

//---------------------------------------------------------
//   mousePressEvent
//---------------------------------------------------------

void FloatEntry::mousePressEvent(QMouseEvent* event)
      {
      button = event->button();
      starty = event->y();
      evx    = double(event->x());
      timecount = 0;
      repeat();
      timer->start(TIMER1);
      }

//---------------------------------------------------------
//   wheelEvent
//---------------------------------------------------------

void FloatEntry::wheelEvent(QWheelEvent* event)
      {
      int delta = event->delta();

      if (delta < 0)
            decValue(-1.0);
      else if (delta > 0)
            incValue(1.0);
      }

//---------------------------------------------------------
//   repeat
//---------------------------------------------------------

void FloatEntry::repeat()
      {
      if (timecount == 1) {
           ++timecount;
            timer->stop();
            timer->start(TIMER2);
            return;
            }
      ++timecount;
      if (timecount == TIMEC) {
            timer->stop();
            timer->start(TIMER3);
            }
      if (timecount == TIMEC2) {
            timer->stop();
            timer->start(TIMER4);
            }

      switch (button) {
            case Qt::LeftButton:
                  if (!MusEGlobal::config.leftMouseButtonCanDecrease)
                    return;
                  // else fall through
            case Qt::MidButton:
                  decValue(evx);
                  break;
            case Qt::RightButton:
                  incValue(evx);
                  break;
            default:
                  break;
            }
      }

//---------------------------------------------------------
//   mouseReleaseEvent
//---------------------------------------------------------

void FloatEntry::mouseReleaseEvent(QMouseEvent*)
      {
      button = Qt::NoButton;
      timer->stop();
      }

//---------------------------------------------------------
//   mouseMoveEvent
//---------------------------------------------------------

void FloatEntry::mouseMoveEvent(QMouseEvent*)
      {
      switch (button) {
            case Qt::LeftButton:
                  break;
            case Qt::MidButton:
                  break;
            case Qt::RightButton:
                  break;
            default:
                  break;
            }
      }

//---------------------------------------------------------
//   mouseDoubleClickEvent
//---------------------------------------------------------

void FloatEntry::mouseDoubleClickEvent(QMouseEvent* event)
      {
      if (event->button() != Qt::LeftButton) {
            mousePressEvent(event);
            return;
            }
      setFocus();
      QLineEdit::setFrame(true);
      update();
      }

//---------------------------------------------------------
//   setValue
//---------------------------------------------------------

void FloatEntry::setValue(double val)
      {
      if (_log) {
            if (val == 0.0f)
                  _value = _minValue;
            else
                  _value = fast_log10(val) * 20.0f;
            }
      else
            _value = val;
      updateValue();
      }

//---------------------------------------------------------
//   updateValue
//---------------------------------------------------------

void FloatEntry::updateValue()
      {
      if (setString(_value)) {
            // value is out of range:
            if (_value > _maxValue)
                  _value = _maxValue;
            else if (_value < _minValue)
                  _value = _minValue;
            }
      }

//---------------------------------------------------------
//   value
//---------------------------------------------------------

double FloatEntry::value() const
      {
      double rv;
      if (_log)
            rv = pow(10.0, _value * 0.05f);
      else
            rv = _value;
      return rv;
      }
}

