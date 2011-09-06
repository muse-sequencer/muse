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
#include "midimeter.h"

#include <QPainter>
#include <QPaintEvent>

namespace Awl {

//---------------------------------------------------------
//   MidiMeter
//---------------------------------------------------------

MidiMeter::MidiMeter(QWidget* parent)
   : Slider(parent)
      {
      setRange(0.0f, 127.0f);
      setLineStep(2);
      setPageStep(4);

      setScaleWidth(7);
      _meterWidth = _scaleWidth * 3;
      meterval = 0.0f;
      }

//---------------------------------------------------------
//   setMeterVal
//    v -  0.0 < 1.0
//---------------------------------------------------------

void MidiMeter::setMeterVal(double v)
      {
      if (v < 0.001)
            v = .0f;
      if (meterval != v) {
            meterval = v;
            update();
            }
      }

//---------------------------------------------------------
//   mouseDoubleClickEvent
//---------------------------------------------------------

void MidiMeter::mouseDoubleClickEvent(QMouseEvent*)
      {
      _value = _minValue;
      valueChange();
      update();
      }

//---------------------------------------------------------
//   paintEvent
//---------------------------------------------------------

void MidiMeter::paintEvent(QPaintEvent* ev)
      {
      int pixel = height() - sliderSize().height();
      double range = maxValue() - minValue();
      int ppos = int(pixel * (_value - minValue()) / range);
      if (_invert)
            ppos = pixel - ppos;

      QRect rr(ev->rect());
      QPainter p(this);

      QColor sc(isEnabled() ? _scaleColor : Qt::gray);
      QColor svc(isEnabled() ? _scaleValueColor : Qt::gray);
      p.setBrush(svc);

      int h  = height();
      int kh = sliderSize().height();

      //---------------------------------------------------
      //    draw meter
      //---------------------------------------------------

      int mw = _meterWidth;
      int x  = 0;

      int y1 = kh / 2;
      int y3 = h - y1;

      int mh  = h - kh;

      p.setPen(Qt::white);
      h = lrint(meterval * mh);
      if (h < 0)
            h = 0;
      else if (h > mh)
            h = mh;
      p.fillRect(x, y3-h, mw, h,    QBrush(0x00ff00)); // green
      p.fillRect(x, y1,   mw, mh-h, QBrush(0x007000)); // dark green
      x += mw;

#if 0
      //---------------------------------------------------
      //    draw scale
      //---------------------------------------------------

      x  += _scaleWidth/2;

      p.setPen(QPen(sc, _scaleWidth));
      p.drawLine(x, y1, x, y2);
      p.setPen(QPen(svc, _scaleWidth));
      p.drawLine(x, y2, x, y3);

      //---------------------------------------------------
      //    draw slider
      //---------------------------------------------------

      x  += _scaleWidth/2;
      p.setPen(QPen(svc, 0));
      points.setPoint(0, x,      y2);
      points.setPoint(1, x + kw, y2 - kh/2);
      points.setPoint(2, x + kw, y2 + kh/2);
      p.drawConvexPolygon(points);
#endif
      }
}

