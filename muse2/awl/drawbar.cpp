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

#include "drawbar.h"

#include <QPainter>

namespace Awl {

//---------------------------------------------------------
//   Drawbar
//---------------------------------------------------------

Drawbar::Drawbar(QWidget* parent)
   : Slider(parent)
      {
      _sliderColor = Qt::darkGray;
      setOrientation(Qt::Vertical);
      setInvertedAppearance(true);
      setRange(0.0, 8.0);
      setLineStep(1.0);
      setPageStep(1.0);
      setInteger(true);
      }

Drawbar::~Drawbar()
      {
      }

//---------------------------------------------------------
//   setSliderColor
//---------------------------------------------------------

void Drawbar::setSliderColor(const QColor& c)
      {
      if (c != _sliderColor) {
            _sliderColor = c;
            update();
            }
      }

//---------------------------------------------------------
//   paint
//---------------------------------------------------------

void Drawbar::paintEvent(QPaintEvent*)
      {
      int h   = height();
      int w   = width();

      int kh    = w * 2;            // knob height
      int kw    = w;
      int pixel = h - kh;
      int ppos = int(pixel * value() / 8.0);

      QPainter p(this);

      QColor sc(Qt::darkGray);
      QColor svc(Qt::gray);

      p.setBrush(svc);

      //---------------------------------------------------
      //    draw scale
      //---------------------------------------------------

      int sx = (w + 9) / 10;
      int sw = w - 2 * sx;
      p.fillRect(sx, 0, sw, ppos, sc);
      QPen pen(Qt::white);
      int lw = 2;
      pen.setWidth(lw);
      p.setPen(pen);
      int sx1 = sx + lw/2;
      p.drawLine(sx1, 0, sx1, ppos);
      int sx2 = sx + sw - lw/2;
      p.drawLine(sx2, 0, sx2, ppos);

      //---------------------------------------------------
      //    draw numbers
      //---------------------------------------------------

      p.save();
      p.setClipRect(QRect(sx, 0, sw, ppos));
      QFont f = p.font();
      f.setPixelSize(8);

      int ch = pixel / 8;
      QString num("%1");
      for (int i = 0; i < 8; ++i) {
            p.drawText(0, i * pixel / 8 - (pixel - ppos), w, ch, Qt::AlignCenter, num.arg(8-i));
            }
      p.restore();

      //---------------------------------------------------
      //    draw slider
      //---------------------------------------------------

      p.fillRect(0, ppos, kw, kh, _sliderColor);

      pen.setWidth(1);
      pen.setColor(Qt::black);
      p.setPen(pen);

      int y1 = ppos + kh / 5 * 2;
      int y2 = ppos + kh / 5 * 3;
      p.drawLine(0, y1, kw, y1);
      p.drawLine(0, y2, kw, y2);
      }
}

