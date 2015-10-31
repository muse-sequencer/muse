//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: tscale.cpp,v 1.2 2003/12/17 11:04:14 wschweer Exp $
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

#include <stdio.h>
#include "tscale.h"
#include "globals.h"
#include "gconfig.h"

#include <QMouseEvent>
#include <QPainter>

namespace MusEGui {

//---------------------------------------------------------
//   TScale
//---------------------------------------------------------

TScale::TScale(QWidget* parent, int ymag)
   : View(parent, 1, ymag)
      {
      setFont(MusEGlobal::config.fonts[5]);
      int w = 4 * fontMetrics().width('0');
      setFixedWidth(w);
      setMouseTracking(true);
      }

//---------------------------------------------------------
//   draw
//---------------------------------------------------------

void TScale::pdraw(QPainter& p, const QRect& r)
      {
      int y = r.y();
      int h = r.height();
      QString s;
      for (int i = 30000; i <= 250000; i += 10000) {
            int yy =  mapy(280000 - i);
            if (yy < y)
                  break;
            if (yy-15 > y+h)
                  continue;
            p.drawLine(0, yy, width(), yy);
            s.setNum(i/1000);
            p.drawText(width() - fontMetrics().width(s) - 1, yy-2, s);  // Use the window font. Tim p4.0.31
            }
      }

void TScale::viewMouseMoveEvent(QMouseEvent* event)
      {
      emit tempoChanged(280000 - event->y());
      }

void TScale::leaveEvent(QEvent*)
      {
      emit tempoChanged(-1);
      }

} // namespace MusEGui
