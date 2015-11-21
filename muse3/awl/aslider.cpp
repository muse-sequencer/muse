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

#include "fastlog.h"
#include "aslider.h"

#include <QKeyEvent>
#include <QWheelEvent>

namespace Awl {

//---------------------------------------------------------
//   AbstractSlider
//---------------------------------------------------------

AbstractSlider::AbstractSlider(QWidget* parent)
   : QWidget(parent), _scaleColor(Qt::black), _scaleValueColor(Qt::blue)
      {
      _id         = 0;
      _value      = 0.5;
      _minValue   = 0.0;
      _maxValue   = 1.0;
      _lineStep   = 0.1;
      _pageStep   = 0.2;
      _center     = false;
      _invert     = false;
      _scaleWidth = 4;
      _log        = false;
      _integer    = false;
      }

//---------------------------------------------------------
//   setEnabled
//---------------------------------------------------------

void AbstractSlider::setEnabled(bool val)
      {
      QWidget::setEnabled(val);
      update();
      }

//---------------------------------------------------------
//   setCenter
//!   If the center flag is set, a notch is drawn to
//!   show the center position.
//---------------------------------------------------------

void AbstractSlider::setCenter(bool val)
      {
      if (val != _center) {
            _center = val;
            update();
            }
      }

//!--------------------------------------------------------
//   setScaleWidth
//---------------------------------------------------------

void AbstractSlider::setScaleWidth(int val)
      {
      if (val != _scaleWidth) {
            _scaleWidth = val;
            update();
            }
      }

//---------------------------------------------------------
//   setScaleColor
//---------------------------------------------------------

void AbstractSlider::setScaleColor(const QColor& c)
      {
      if (c != _scaleColor) {
            _scaleColor = c;
            update();
            }
      }

//---------------------------------------------------------
//   setScaleValueColor
//---------------------------------------------------------

void AbstractSlider::setScaleValueColor(const QColor& c)
      {
      if (c != _scaleValueColor) {
            _scaleValueColor = c;
            update();
            }
      }

//---------------------------------------------------------
//   wheelEvent
//---------------------------------------------------------

void AbstractSlider::wheelEvent(QWheelEvent* ev)
      {
      double div = 120.0;
      if (ev->modifiers() & Qt::ShiftModifier)
            _value += (ev->delta() * pageStep()) / div;
      else
            _value += (ev->delta() * lineStep()) / div;
      if (_value < _minValue)
            _value = _minValue;
      else if (_value > _maxValue)
            _value = _maxValue;
      valueChange();
      update();
      }

//---------------------------------------------------------
//   keyPressEvent
//---------------------------------------------------------

void AbstractSlider::keyPressEvent(QKeyEvent* ev)
      {
      double oval = _value;

      switch (ev->key()) {
            case Qt::Key_Home:    _value = _minValue; break;
            case Qt::Key_End:     _value = _maxValue; break;
            case Qt::Key_Up:
            case Qt::Key_Left:    _value += lineStep(); break;
            case Qt::Key_Down:
            case Qt::Key_Right:    _value -= lineStep(); break;
            case Qt::Key_PageDown: _value -= pageStep(); break;
            case Qt::Key_PageUp:   _value += pageStep(); break;
            default:
                  break;
            }
      if (_value < _minValue)
            _value = _minValue;
      else if (_value > _maxValue)
            _value = _maxValue;

      if (oval != _value) {
            if (_integer && (rint(oval) == rint(_value)))
                  return;
            valueChange();
            update();
            }
      }

//---------------------------------------------------------
//   setValue
//---------------------------------------------------------

void AbstractSlider::setValue(double val)
      {
      if (_log) {
            if (val == 0.0f)
                  _value = _minValue;
            else {
                  _value = fast_log10(val) * 20.0f;
       		if (_value < _minValue)
            		_value = _minValue;
                 	}
            }
      else if (_integer)
            _value = rint(val);
      else
            _value = val;
      update();
      }

//---------------------------------------------------------
//   valueChange
//---------------------------------------------------------

void AbstractSlider::valueChange()
      {
      emit valueChanged(value(), _id);
      }

//---------------------------------------------------------
//   value
//---------------------------------------------------------

double AbstractSlider::value() const
      {
      if (_log)
            return pow(10.0, _value*0.05f);
      if (_integer)
            return rint(_value);
      return _value;
      }

//---------------------------------------------------------
//   minLogValue
//---------------------------------------------------------

//double AbstractSlider::minValue() const {
//  return _log ? pow(10.0, _minValue*0.05f) : _minValue;
//}

//---------------------------------------------------------
//   setMinLogValue
//---------------------------------------------------------

void AbstractSlider::setMinLogValue(double val) {
  if (_log) {
    if (val == 0.0f) _minValue = -100;
    else _minValue = fast_log10(val) * 20.0f;
  }
  else _minValue = val;
}

//---------------------------------------------------------
//   maxLogValue
//---------------------------------------------------------

//double AbstractSlider::maxValue() const {
//  return _log ? pow(10.0, _maxValue*0.05f) : _maxValue;
//}

//---------------------------------------------------------
//   setMaxLogValue
//---------------------------------------------------------

void AbstractSlider::setMaxLogValue(double val) {
  if (_log) {
    _maxValue = fast_log10(val) * 20.0f;
  }
  else _maxValue = val;
}

}
