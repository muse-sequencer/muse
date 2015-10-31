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
#include "volslider.h"

#include <QMouseEvent>

namespace Awl {

//---------------------------------------------------------
//   VolSlider
//---------------------------------------------------------

VolSlider::VolSlider(QWidget* parent)
   : Slider(parent)
      {
      setLog(true);
      setRange(-60.0f, 10.0f);
      setScaleWidth(7);
      setLineStep(.8f);
      setPageStep(3.0f);
      }

//---------------------------------------------------------
//   mouseDoubleClickEvent
//---------------------------------------------------------

void VolSlider::mouseDoubleClickEvent(QMouseEvent* ev)
      {
      if (ev->button() == Qt::RightButton)
      	_value = 0.0;
      else
      	_value = _minValue;
      valueChange();
      update();
      }


//---------------------------------------------------------
//   setValue
//---------------------------------------------------------

void VolSlider::setValue(double val)
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
      else
            _value = val;
      update();
      }

//---------------------------------------------------------
//   value
//---------------------------------------------------------

double VolSlider::value() const
      {
      return _log ? (_value <= _minValue) ? 0.0f : pow(10.0, _value*0.05f)
                  : _value;
      }

}
