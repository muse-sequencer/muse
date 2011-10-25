//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: wtscale.cpp,v 1.3 2004/04/11 13:03:32 wschweer Exp $
//  (C) Copyright 2000 Werner Schweer (ws@seh.de)
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

#include <values.h>

#include <QPainter>
#include <QRect>
#include <QToolTip>

#include "wtscale.h"
#include "midieditor.h"
#include "globals.h"
#include "gconfig.h"
#include "song.h"
#include "../marker/marker.h"
#include "icons.h"

namespace MusEGui {

//---------------------------------------------------------
//   WTScale
//    Wave Time Scale
//---------------------------------------------------------

WTScale::WTScale(int* r, QWidget* parent, int xs)
   : View(parent, xs, 1)
      {
      QToolTip::add(this, tr("bar scale"));
      barLocator = false;
      raster = r;
      pos[0] = int(MusEGlobal::song->tempomap()->tick2time(MusEGlobal::song->cpos()) * sampleRate);
      pos[1] = int(MusEGlobal::song->tempomap()->tick2time(MusEGlobal::song->lpos()) * sampleRate);
      pos[2] = int(MusEGlobal::song->tempomap()->tick2time(MusEGlobal::song->rpos()) * sampleRate);
      pos[3] = -1;            // do not show
      button = Qt::NoButton;
      setMouseTracking(true);
      connect(MusEGlobal::song, SIGNAL(posChanged(int, unsigned, bool)), SLOT(setPos(int, unsigned, bool)));
      connect(MusEGlobal::song, SIGNAL(songChanged(int)), SLOT(songChanged(int)));
      connect(MusEGlobal::song, SIGNAL(markerChanged(int)), SLOT(redraw()));
      setFixedHeight(28);
      setBg(QColor(0xe0, 0xe0, 0xe0));
      }

//---------------------------------------------------------
//   songChanged
//---------------------------------------------------------

void WTScale::songChanged(int /*type*/)
      {
      }

//---------------------------------------------------------
//   setPos
//---------------------------------------------------------

void WTScale::setPos(int idx, unsigned val, bool adjustScrollbar)
      {
      val = int(MusEGlobal::song->tempomap()->tick2time(val) * sampleRate);
      if (val == pos[idx])
            return;
      int opos = mapx(pos[idx] == -1 ? val : pos[idx]);
      pos[idx] = val;
      if (!isVisible())
            return;
      val   = mapx(val);
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

//---------------------------------------------------------
//   viewMousePressEvent
//---------------------------------------------------------

void WTScale::viewMousePressEvent(QMouseEvent* event)
      {
      button = event->button();
      viewMouseMoveEvent(event);
      }

//---------------------------------------------------------
//   viewMouseReleaseEvent
//---------------------------------------------------------

void WTScale::viewMouseReleaseEvent(QMouseEvent* event)
      {
      button = Qt::NoButton;
      }

//---------------------------------------------------------
//   viewMouseMoveEvent
//---------------------------------------------------------

void WTScale::viewMouseMoveEvent(QMouseEvent* event)
      {
      int x= MusEGlobal::song->tempomap()->time2tick(double(event->x())/double(sampleRate));
      x = MusEGlobal::song->raster(x, *raster);
      if (x < 0)
            x = 0;
      emit timeChanged(x);
      int i;
      switch (button) {
            case Qt::LeftButton:
                  i = 0;
                  break;
            case Qt::MidButton:
                  i = 1;
                  break;
            case Qt::RightButton:
                  if ((MusEGlobal::config.rangeMarkerWithoutMMB) && (event->modifiers() & Qt::ControlModifier))
                      i = 1;
                  else
                      i = 2;
                  break;
            default:
                  return;
            }
      MusEGlobal::song->setPos(i, x);
      }

//---------------------------------------------------------
//   leaveEvent
//---------------------------------------------------------

void WTScale::leaveEvent(QEvent*)
      {
//      emit timeChanged(MAXINT);
      }

//---------------------------------------------------------
//   draw
//---------------------------------------------------------

void WTScale::pdraw(QPainter& p, const QRect& r)
      {
      int x = r.x();
      int w = r.width();

      x -= 20;
      w += 40;    // wg. Text

      //
      //    draw Marker
      //
      int y = 12;
      p.setPen(Qt::black);
      p.setFont(font4);
      p.drawLine(r.x(), y+1, r.x() + r.width(), y+1);
      QRect tr(r);
      tr.setHeight(12);
      MarkerList* marker = MusEGlobal::song->marker();
      for (iMarker m = marker->begin(); m != marker->end(); ++m) {
            int xp = mapx(int(m->second.time() * sampleRate));
            if (xp > x+w)
                  break;
            int xe = r.x() + r.width();
            iMarker mm = m;
            ++mm;
            if (mm != marker->end()) {
                  xe = mapx(mm->first);
                  }
            QRect tr(xp, 0, xe-xp, 13);
            if (m->second.current()) {
                  p.fillRect(tr, Qt::white);
                  }
            if (r.intersects(tr)) {
                  int x2;
                  iMarker mm = m;
                  ++mm;
                  if (mm != marker->end())
                        x2 = mapx(mm->first);
                  else
                        x2 = xp+200;
                  QRect r  = QRect(xp+10, 0, x2-xp, 12);
                  p.drawPixmap(xp, 0, *flagIconS);
                  p.drawText(r, Qt::AlignLeft|Qt::AlignVCenter, m->second.name());
                  }
            }

      //---------------------------------------------------
      //    draw location marker
      //---------------------------------------------------

      int h = height()-12;

      if (barLocator) {
            p.setPen(Qt::red);
            int xp = mapx(pos[0]);
            if (xp >= x && xp < x+w)
                  p.drawLine(xp, y, xp, h);
            p.setPen(Qt::blue);
            xp = mapx(pos[1]);
            if (xp >= x && xp < x+w)
                  p.drawLine(xp, y, xp, h);
            xp = mapx(pos[2]);
            if (xp >= x && xp < x+w)
                  p.drawLine(xp, y, xp, h);
            }
      else {
            for (int i = 0; i < 3; ++i) {
                  int xp = mapx(pos[i]);
                  if (xp >= x && xp < x+w) {
                        QPixmap* pm = markIcon[i];
                        p.drawPixmap(xp - pm->width()/2, y-1, *pm);
                        }
                  }
            }
      p.setPen(Qt::black);
      if (pos[3] != -1) {
            int xp = mapx(pos[3]);
            if (xp >= x && xp < x+w)
                  p.drawLine(xp, 0, xp, height());
            }

      int ctick = MusEGlobal::song->samples2tick(mapxDev(x));
      int bar1, bar2, beat, tick;
      MusEGlobal::song->tickValues(ctick, &bar1, &beat, &tick);
      MusEGlobal::song->tickValues(MusEGlobal::song->samples2tick(mapxDev(x+w)), &bar2, &beat, &tick);

//printf("bar %d  %d-%d=%d\n", bar, ntick, stick, ntick-stick);

      int stick = MusEGlobal::song->bar2tick(bar1, 0, 0);
      int ntick;
      for (int bar = bar1; bar <= bar2; bar++, stick = ntick) {
            ntick     = MusEGlobal::song->bar2tick(bar+1, 0, 0);
            int a = MusEGlobal::song->tick2samples(ntick);
            int b = MusEGlobal::song->tick2samples(stick);
            int tpix  = rmapx(a - b);
            if (tpix < 64) {
                  // don´t show beats if measure is this small
                  int n = 1;
                  if (tpix < 32)
                        n = 2;
                  if (tpix <= 16)
                        n = 4;
                  if (tpix < 8)
                        n = 8;
                  if (tpix <= 4)
                        n = 16;
                  if (tpix <= 2)
                        n = 32;
                  if (bar % n)
                        continue;
                  p.setFont(font3);
                  int x = mapx(b);
                  QString s;
                  s.setNum(bar + 1);
                  p.drawLine(x, y+1, x, y+1+h);
                  QRect r = QRect(x+2, y, 0, h);
                  p.drawText(r, Qt::AlignLeft|Qt::AlignVCenter|Qt::TextDontClip, s);
                  }
            else {
                  int z, n;
                  MusEGlobal::song->timesig(stick, z, n);
                  for (int beat = 0; beat < z; beat++) {
                        int xx = MusEGlobal::song->tick2samples(MusEGlobal::song->bar2tick(bar, beat, 0));
                        int xp = mapx(xx);
                        QString s;
                        QRect r(xp+2, y, 0, h);
                        int y1;
                        int num;
                        if (beat == 0) {
                              num = bar + 1;
                              y1  = y + 1;
                              p.setFont(font3);
                              }
                        else {
                              num = beat + 1;
                              y1  = y + 7;
                              p.setFont(font1);
                              r.setY(y+3);
                              }
                        s.setNum(num);
                        p.drawLine(xp, y1, xp, y+1+h);
                        p.drawText(r, Qt::AlignLeft|Qt::AlignVCenter|Qt::TextDontClip, s);
                        }
                  }
            }
      }

} // namespace MusEGui

