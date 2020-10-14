//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: sigscale.cpp,v 1.6 2004/04/11 13:03:32 wschweer Exp $
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

#include <limits.h>

#include <QWidget>
#include <QMouseEvent>
#include <QEvent>
#include <QPainter>
#include <QStyle>
#include <QStyleOption>
#include <QRect>

#include "sig.h"
#include "globals.h"
#include "sigscale.h"
#include "song.h"
#include "gconfig.h"

namespace MusEGui {

//---------------------------------------------------------
//   SigScale
//---------------------------------------------------------

SigScale::SigScale(int* r, QWidget* parent, int xs)
   : View(parent, xs, 1)
      {
      setToolTip(tr("Signature scale"));
      raster = r;
      pos[0] = MusEGlobal::song->cpos();
      pos[1] = MusEGlobal::song->lpos();
      pos[2] = MusEGlobal::song->rpos();
      button = Qt::NoButton;
      setMouseTracking(true);
      connect(MusEGlobal::song, SIGNAL(posChanged(int, unsigned, bool)), this, SLOT(setPos(int, unsigned, bool)));
      setFixedHeight(18);
      }

//---------------------------------------------------------
//   setPos
//---------------------------------------------------------

void SigScale::setPos(int idx, unsigned val, bool)
      {
      if (val == pos[idx])
            return;
      unsigned opos = mapx(pos[idx]);
      pos[idx] = val;
      if (!isVisible())
            return;
      val = mapx(val);
      int x = -9;
      int w = 18;
      if (opos > val) {
            w += opos - val;
            x += val;
            }
      else {
            w += val - opos;
            x += opos;
            }
      redraw(QRect(x, 0, w, height()));
      }

void SigScale::viewMousePressEvent(QMouseEvent* event)
      {
      button = event->button();
      viewMouseMoveEvent(event);
      }

void SigScale::viewMouseReleaseEvent(QMouseEvent*)
      {
      button = Qt::NoButton;
      }

void SigScale::viewMouseMoveEvent(QMouseEvent* event)
      {
      int x = event->x();
      if(x < 0)
        x = 0;
      x = MusEGlobal::sigmap.raster(x, *raster);
      emit timeChanged(x);

      MusECore::Song::POSTYPE posType;

      switch (button) {
      case Qt::LeftButton:
          if ((MusEGlobal::config.rangeMarkersSet == MusEGlobal::CONF_SET_MARKERS_CTRL_LEFT_CTRL_RIGHT) && (event->modifiers() & Qt::ControlModifier))
              posType = MusECore::Song::LPOS;
          else
              posType = MusECore::Song::CPOS;
          break;
      case Qt::MidButton:
          posType = MusECore::Song::LPOS;
          break;
      case Qt::RightButton:
          if ((MusEGlobal::config.rangeMarkersSet == MusEGlobal::CONF_SET_MARKERS_CTRL_LEFT_CTRL_RIGHT) && (event->modifiers() & Qt::ControlModifier))
              posType = MusECore::Song::RPOS;
          else if ((MusEGlobal::config.rangeMarkersSet == MusEGlobal::CONF_SET_MARKERS_CTRL_RIGHT_RIGHT) && (event->modifiers() & Qt::ControlModifier))
              posType = MusECore::Song::LPOS;
          else
              posType = MusECore::Song::RPOS;
          break;
      default:
          return; // if no button is pressed the function returns here
      }
      MusECore::Pos p(x, true);
      MusEGlobal::song->setPos(posType, p);
      }

//---------------------------------------------------------
//   leaveEvent
//---------------------------------------------------------

void SigScale::leaveEvent(QEvent*)
      {
//      emit timeChanged(INT_MAX);
      }

//---------------------------------------------------------
//   draw
//---------------------------------------------------------

void SigScale::pdraw(QPainter& p, const QRect& r, const QRegion&)
      {
    // seems the only way to draw the correct background...
    QStyleOption opt;
    opt.init(this);
    style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);

      int x = r.x();
      int w = r.width();
      int h = height();

      if (x < 0)
            x = 0;
      p.setFont(MusEGlobal::config.fonts[3]);
      p.setPen(palette().color(QPalette::WindowText));
      for (MusECore::ciSigEvent si = MusEGlobal::sigmap.begin(); si != MusEGlobal::sigmap.end(); ++si) {
            MusECore::SigEvent* e = si->second;
            int xp = mapx(e->tick);
            if (xp > x+w)
                  break;
            if (xp+40 < x)
                  continue;
            p.drawLine(xp, 0, xp, h/2);
            p.drawLine(xp, h/2, xp+5, h/2);
            QString s = QString("%1/%2").arg(e->sig.z).arg(e->sig.n);
            p.drawText(xp+8, h-6, s);
            }

      //---------------------------------------------------
      //    draw location marker
      //---------------------------------------------------

      p.setPen(MusEGlobal::config.rangeMarkerColor);
      int xp = mapx(pos[1]);
      if (xp >= x && xp < x+w)
            p.drawLine(xp, 0, xp, h);
      xp = mapx(pos[2]);
      if (xp >= x && xp < x+w)
            p.drawLine(xp, 0, xp, h);
      // Draw the red main position cursor last, on top of the others.
      p.setPen(MusEGlobal::config.positionMarkerColor);
      xp = mapx(pos[0]);
      if (xp >= x && xp < x+w)
            p.drawLine(xp, 0, xp, h);
      }

} // namespace MusEGui
