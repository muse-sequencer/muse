//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: mtscale.cpp,v 1.8.2.7 2009/05/03 04:14:01 terminator356 Exp $
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

#include <QMouseEvent>
#include <QPainter>

#include "mtscale.h"
#include "song.h"
#include "app.h"
#include "icons.h"
#include "gconfig.h"

namespace MusEGui {

//---------------------------------------------------------
//   MTScale
//    Midi Time Scale
//---------------------------------------------------------

MTScale::MTScale(int* r, QWidget* parent, int xs, bool _mode)
   : View(parent, xs, 1)
      {
      waveMode = _mode;
      setToolTip(tr("bar scale"));
      barLocator = false;
      raster = r;
      if (waveMode) {
            pos[0] = MusEGlobal::tempomap.tick2frame(MusEGlobal::song->cpos());
            pos[1] = MusEGlobal::tempomap.tick2frame(MusEGlobal::song->lpos());
            pos[2] = MusEGlobal::tempomap.tick2frame(MusEGlobal::song->rpos());
            }
      else {
            pos[0] = MusEGlobal::song->cpos();
            pos[1] = MusEGlobal::song->lpos();
            pos[2] = MusEGlobal::song->rpos();
            }
      pos[3] = INT_MAX;            // do not show
      button = Qt::NoButton;
      setMouseTracking(true);
      connect(MusEGlobal::song, SIGNAL(posChanged(int, unsigned, bool)), SLOT(setPos(int, unsigned, bool)));
      connect(MusEGlobal::song, SIGNAL(songChanged(MusECore::SongChangedStruct_t)), SLOT(songChanged(MusECore::SongChangedStruct_t)));
      connect(MusEGlobal::song, SIGNAL(markerChanged(int)), SLOT(redraw()));
      connect(MusEGlobal::muse, SIGNAL(configChanged()), SLOT(configChanged()));

      setFixedHeight(28);

      setBg(MusEGlobal::config.rulerBg);
      }

void MTScale::configChanged()
      {
      setBg(MusEGlobal::config.rulerBg);


      }

//---------------------------------------------------------
//   songChanged
//---------------------------------------------------------

void MTScale::songChanged(MusECore::SongChangedStruct_t type)
      {
// REMOVE Tim. clip. Changed.
//       if (type & (SC_SIG|SC_TEMPO)) {
      if (type & (SC_SIG|SC_TEMPO|SC_MARKERS_REBUILT|SC_MARKER_INSERTED|SC_MARKER_REMOVED|SC_MARKER_MODIFIED)) {
           if ((type & SC_TEMPO) && waveMode) {
                  pos[0] = MusEGlobal::tempomap.tick2frame(MusEGlobal::song->cpos());
                  pos[1] = MusEGlobal::tempomap.tick2frame(MusEGlobal::song->lpos());
                  pos[2] = MusEGlobal::tempomap.tick2frame(MusEGlobal::song->rpos());
                  }
            redraw();
            }
      }

//---------------------------------------------------------
//   setPos
//---------------------------------------------------------

void MTScale::setPos(int idx, unsigned val, bool)
      {
      if (val == INT_MAX) {
            if (idx == 3) {
                  pos[3] = INT_MAX;
                  redraw(QRect(0, 0, width(), height()));
                  }
            return;
            }
      if (waveMode)
            val = MusEGlobal::tempomap.tick2frame(val);
      if (val == pos[idx])
            return;
      int opos = mapx(pos[idx] == INT_MAX ? val : pos[idx]);
      pos[idx] = val;
      if (!isVisible())
            return;

      int tval   = mapx(val);
      int x = -9;
      int w = 18;

      if (tval < 0) { // tval<0 occurs whenever the window is scrolled left, so I switched to signed int (ml)
            //printf("MTScale::setPos - idx:%d val:%d tval:%d opos:%d w:%d h:%d\n", idx, val, tval, opos, width(), height());
      
            redraw(QRect(0,0,width(),height()));
            return;
            }
      if (opos > tval) { 
            w += opos - tval;
            x += tval;
            }
      else {
            w += tval - opos;
            x += opos;
            }
      //printf("MTScale::setPos idx:%d val:%d tval:%d opos:%d x:%d w:%d h:%d\n", idx, val, tval, opos, x, w, height());
      
      redraw(QRect(x, 0, w, height()));
      }

//---------------------------------------------------------
//   viewMousePressEvent
//---------------------------------------------------------

void MTScale::viewMousePressEvent(QMouseEvent* event)
      {
      button = event->button();
      viewMouseMoveEvent(event);
      }

//---------------------------------------------------------
//   viewMouseReleaseEvent
//---------------------------------------------------------

void MTScale::viewMouseReleaseEvent(QMouseEvent*)
      {
      button = Qt::NoButton;
      }

//---------------------------------------------------------
//   viewMouseMoveEvent
//---------------------------------------------------------

void MTScale::viewMouseMoveEvent(QMouseEvent* event)
      {
      if (event->modifiers() & Qt::ShiftModifier )
            setCursor(QCursor(Qt::PointingHandCursor));
      else
            setCursor(QCursor(Qt::ArrowCursor));
      
      int x = event->x();
      if (x < 0)
            x = 0;
      if (waveMode)
            // Normally frame to tick methods round down. But here we need it to 'snap'
            //  the frame from either side of a tick to the tick. So round to nearest.
            x = MusEGlobal::tempomap.frame2tick(x, 0, MusECore::LargeIntRoundNearest);
      x = MusEGlobal::sigmap.raster(x, *raster);
      //printf("MTScale::viewMouseMoveEvent\n");  
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
                  return; // if no button is pressed the function returns here
            }
      MusECore::Pos p(x, true);
      
      if(i== 0 && (event->modifiers() & Qt::ShiftModifier )) {        // If shift +LMB we add a marker 
// REMOVE Tim. clip. Changed.
//             MusECore::Marker *alreadyExists = MusEGlobal::song->getMarkerAt(x);
//             if (!alreadyExists) {
            const MusECore::iMarker alreadyExists = MusEGlobal::song->getMarkerAt(x);
            if (alreadyExists == MusEGlobal::song->marker()->end()) {
                  MusEGlobal::song->addMarker(QString(""), x, false);         
                  // Note: Song::addMarker() already emits a 'markerChanged'.
                  //emit addMarker(x);
                  }
            }
      else if (i== 2 && (event->modifiers() & Qt::ShiftModifier )) {  // If shift +RMB we remove a marker 
//             MusECore::Marker *toRemove = MusEGlobal::song->getMarkerAt(x);
//             if (toRemove)
//               MusEGlobal::song->removeMarker(toRemove);
// REMOVE Tim. clip. Changed.
            const MusECore::iMarker toRemove = MusEGlobal::song->getMarkerAt(x);
            if (toRemove != MusEGlobal::song->marker()->end())
              MusEGlobal::song->removeMarker(toRemove->second);
            else
              fprintf(stderr, "No marker to remove\n");
            }
      else
            MusEGlobal::song->setPos(i, p);                             // all other cases: relocating one of the locators
      }

//---------------------------------------------------------
//   leaveEvent
//---------------------------------------------------------

void MTScale::leaveEvent(QEvent*)
      {
      emit timeChanged(INT_MAX);
      }

//---------------------------------------------------------
//   pdraw
//---------------------------------------------------------

void MTScale::pdraw(QPainter& p, const QRect& mr, const QRegion& mrg)
      {
        
      //---------------------------------------------
      // Changed to draw in device coordinate space
      //  instead of virtual, transformed space.
      //---------------------------------------------

      //p.save();
      bool wmtxen = p.worldMatrixEnabled();
      p.setWorldMatrixEnabled(false);
      
      int mx = mr.x();
      int my = mr.y();
      int mw = mr.width();
      int mh = mr.height();
      
      const QRect vr = devToVirt(mr);
      
      int vx = vr.x();
      int vy = vr.y();
      int vw = vr.width();

      const int mx_2    = mx + mw;
      const int my12    = 12;
      const int my12_m1 = my12 - 1;
      const int my13    = 13;
      const int mh_m12  = height() - 12;

      const int vx_2    = vx + vw;
      const int vtop    = mapyDev(0);
      const int vw10    = rmapxDev(10);
      const int vw200   = rmapxDev(200);
      const int vh12    = rmapyDev(12);
      const int vh13    = rmapyDev(13);

      
      QPen pen;
      pen.setCosmetic(true);
      
// For testing...
      //fprintf(stderr, "MTScale::pdraw x:%d w:%d\n", x, w);
      
      //---------------------------------------------------
      //    draw Marker
      //---------------------------------------------------

      pen.setColor(MusEGlobal::config.rulerFg);
      p.setPen(pen);
      p.setFont(MusEGlobal::config.fonts[5]);
      p.drawLine(mx, my13, mx_2, my13);
      MusECore::MarkerList* marker = MusEGlobal::song->marker();
      for (MusECore::iMarker m = marker->begin(); m != marker->end(); ++m) {
            int xp;
            if(waveMode) 
              xp = m->second.frame();
            else  
              xp = m->second.tick();
            
            if (xp > vx_2)
                  break;
            
            int mxp = mapx(xp);
            
            int xe = vx_2;
            MusECore::iMarker mm = m;
            ++mm;
            if (mm != marker->end()) {
                  
                  if(waveMode)
// REMOVE Tim. clip. Changed.
//                     xe = MusEGlobal::tempomap.tick2frame(mm->first);
// Hm, mistake?
//                     xe = MusEGlobal::tempomap.tick2frame(mm->second.frame());
                    xe = mm->second.frame();
                  else
// REMOVE Tim. clip. Changed.
//                     xe = mm->first;
                    xe = mm->second.tick();
                  }
            
            const QRect tr(xp, vy, xe - xp, vh13);
            
            QRect wr = vr.intersected(tr);
            
// For testing...
//             fprintf(stderr, "MTScale::pdraw: marker fill: wr x:%d y:%d w:%d h:%d\n",
//                     wr.x(), wr.y(), wr.width(), wr.height());
            
            if(!wr.isEmpty()) 
            {        
              if (m->second.current()) 
              {
// For testing...
//                 fprintf(stderr, "...marker fill within range. Filling area at mx:%d my:%d mw:%d mh:%d\n",
//                        map(wr).x(), map(wr).y(), map(wr).width(), map(wr).height());
                
                    p.fillRect(map(wr), MusEGlobal::config.rulerCurrent);
              }
              
              int x2_time;
              if (mm != marker->end())
              {
                    if(waveMode) 
                      x2_time = MusEGlobal::tempomap.tick2frame(mm->first);
                    else
                      x2_time = mm->first;
              }      
              else
                    x2_time = xp + vw200;
              
              //printf("MTScale::pdraw marker %s xp:%d y:%d h:%d r.x:%d r.w:%d\n", m->second.name().toLatin1(), xp, height(), y, r.x(), r.width());
  
              // Must be reasonable about very low negative x values! With long songs > 15min
              //  and with high horizontal magnification, 'ghost' drawings appeared,
              //  apparently the result of truncation later (xp = -65006 caused ghosting
              //  at bar 245 with magnification at max.), even with correct clipping region
              //  applied to painter in View::paint(). Tim.  Apr 5 2009 
              // Quote: "Warning: Note that QPainter does not attempt to work around 
              //  coordinate limitations in the underlying window system. Some platforms may 
              //  behave incorrectly with coordinates as small as +/-4000."


              QPixmap* pm = flagIconS;
              const int pmw = rmapxDev(pm->width());
              const int pmx = xp;
              const int pmx2 = xp + pmw;

// For testing...
//               fprintf(stderr,
//                   "MTScale::pdraw: Marker: x:%d y:%d w:%d h:%d mx:%d my:%d mw:%d mh:%d x2:%d x2_right:%d xp:%d pmw:%d pmx:%d pmx2:%d\n",
//                   r.x(), r.y(), r.width(), r.height(), mx, my, mw, mh, x2, x2_right, xp, pm->width(), pmx, pmx2);

              if ((pmx >= vx && pmx < vx_2) || (pmx2 > vx && pmx2 <= vx_2) ||
                  (vx >= pmx && vx < pmx2) || (vx_2 > pmx && vx_2 <= pmx2)) {
// For testing...
//                     fprintf(stderr, "...marker within range. xorg:%d xmag:%d xpos:%d Drawing marker at:%d\n", xorg, xmag, xpos, mapx(pmx));

                p.drawPixmap(mapx(pmx), 0, *pm);
              }

              const QString s = m->second.name();
              const int brw = rmapxDev(p.fontMetrics().boundingRect(s).width());
              int w_txt = x2_time - xp;
              if(brw < w_txt)
                w_txt = brw;
              const QRect br_txt = QRect(xp + vw10, vtop, w_txt, vh12);

// For testing...
//               fprintf(stderr,
//                   "MTScale::pdraw: Marker text: x:%d y:%d w:%d h:%d x2:%d x2_right:%d xp:%d x2_time:%d br x:%d y:%d w:%d h:%d\n",
//                   r.x(), r.y(), r.width(), r.height(), x2, x2_right, xp, x2_time, br_txt.x(), br_txt.y(), br_txt.width(), br_txt.height());

              if(br_txt.intersects(vr))
              {
// For testing...
//                 fprintf(stderr, "...marker text within range. xorg:%d xmag:%d xpos:%d Drawing marker text at x:%d y:%d w:%d h:%d\n",
//                         xorg, xmag, xpos, map(br_txt).x(), map(br_txt).y(), map(br_txt).width(), map(br_txt).height());

                pen.setColor(MusEGlobal::config.rulerFg);
                p.setPen(pen);
                p.drawText(map(br_txt), Qt::AlignLeft|Qt::AlignVCenter, s);
              }  
              
              if(xp >= vx && xp < vx_2)
              {
// For testing...
//                 fprintf(stderr, "...marker line within range. xorg:%d xmag:%d xpos:%d Drawing marker line at mxp:%d my12:%d mheight:%d\n",
//                         xorg, xmag, xpos, mxp, my12, mheight);

                pen.setColor(Qt::green);
                p.setPen(pen);
                p.drawLine(mxp, my12, mxp, mh);
              }  
            }  
      }

      //---------------------------------------------------
      //    draw location marker
      //---------------------------------------------------

      if (barLocator) {
            pen.setColor(Qt::blue);
            p.setPen(pen);
            int xp = pos[1];
            int mxp = mapx(xp);
            if (xp >= vx && xp < vx_2)
                  p.drawLine(mxp, my, mxp, mh_m12);
            xp = pos[2];
            mxp = mapx(xp);
            if (xp >= vx && xp < vx_2)
                  p.drawLine(mxp, my, mxp, mh_m12);
            pen.setColor(Qt::red);
            p.setPen(pen);
            // Draw the red main position cursor last, on top of the others.
            xp = pos[0];
            mxp = mapx(xp);
            if (xp >= vx && xp < vx_2)
                  p.drawLine(mxp, my, mxp, mh_m12);
            }
      else {
            for (int i = 0; i < 3; ++i) {
                  const int xp = pos[i];
                  
                  QPixmap* pm = markIcon[i];
                  const int pmw_d2 = rmapxDev(pm->width()) / 2;
                  const int pmx = xp - pmw_d2;
                  const int pmx2 = xp + pmw_d2;

// For testing...
                  //fprintf(stderr,
                  //  "MTScale::pdraw: Pos mark: x:%d y:%d w:%d h:%d mx:%d my:%d mw:%d mh:%d x2:%d x2_right:%d h_m12:%d i:%d xp:%d pmw:%d pmw_d2:%d pmx:%d pmx2:%d\n",
                  //  r.x(), r.y(), r.width(), r.height(), mx, my, mw, mh, x2, x2_right, h_m12, i, xp, pm->width(), pmw_d2, pmx, pmx2);

                  if ((pmx >= vx && pmx < vx_2) || (pmx2 > vx && pmx2 <= vx_2) ||
                      (vx >= pmx && vx < pmx2) || (vx_2 > pmx && vx_2 <= pmx2)) {
// For testing...
//                         fprintf(stderr, "...position mark within range. xorg:%d xmag:%d xpos:%d Drawing mark at:%d\n", xorg, xmag, xpos, mapx(pmx));
                        
                        p.drawPixmap(mapx(pmx), my12_m1, *pm);
                        }
                  }
            }
            
      if (pos[3] != INT_MAX) {
            int xp = pos[3];
            int mxp = mapx(xp);
// For testing...
//             fprintf(stderr,
//                 "MTScale::pdraw: Ruler: x:%d y:%d w:%d h:%d  mx:%d my:%d mw:%d mh:%d h_m12:%d pos[3]:%d xp:%d mxp:%d\n",
//                 r.x(), r.y(), r.width(), r.height(), mx, my, mw, mh, h_m12, pos[3], xp, mxp);

            if (xp >= vx && xp < vx_2)
            {
                  pen.setColor(MusEGlobal::config.rulerFg);
                  p.setPen(pen);
// For testing...
//                   fprintf(stderr, "...ruler line within range. Drawing line.\n");

                  p.drawLine(mxp, my, mxp, mh);
            }
            }


        //p.setWorldMatrixEnabled(true);
        p.setWorldMatrixEnabled(wmtxen);
        //p.restore();      
      
        drawTickRaster(p, QRect(mr.x(), my13, mr.width(), mh_m12), mrg, 0,
                         waveMode, false, true,
                         MusEGlobal::config.rulerFg, 
                         MusEGlobal::config.rulerFg,
                         QColor(),
                         MusEGlobal::config.fonts[3], MusEGlobal::config.fonts[4]);
      }


} // namespace MusEGui
