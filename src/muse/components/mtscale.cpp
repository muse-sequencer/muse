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
#include "globals.h"

namespace MusEGui {

//---------------------------------------------------------
//   MTScale
//    Midi Time Scale
//---------------------------------------------------------

MTScale::MTScale(int r, QWidget* parent, int xs, bool _mode)
   : View(parent, xs, 1)
      {
      waveMode = _mode;
      setStatusTip(tr("Time scale: Set position (LMB) and range markers (left: CTRL+LMB or MMB, right: CTRL+RMB or RMB). Hold SHIFT to set (LMB) or delete (RMB) custom markers."));
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
      // Why don't we just rasterize here instead of in the callers?
      // Possibly because we may need fine control over this PROGRAMMED position
      //  versus the position EMITTED by MTScale?
      //val = MusEGlobal::sigmap.raster(val, raster);
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
//   setRaster
//---------------------------------------------------------

void MTScale::setRaster(int rast)
{
  raster = rast;
  redraw();
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
      x = MusEGlobal::sigmap.raster(x, raster);
      //printf("MTScale::viewMouseMoveEvent\n");  
      emit timeChanged(x);

      MusECore::Song::POSTYPE posType;

      switch (button) {
      case Qt::LeftButton:
          if (event->modifiers() & Qt::ControlModifier)
              posType = MusECore::Song::LPOS;
          else
              posType = MusECore::Song::CPOS;
          break;
      case Qt::MidButton:
          posType = MusECore::Song::LPOS;
          break;
      case Qt::RightButton:
          posType = MusECore::Song::RPOS;
          break;
      default:
          return; // if no button is pressed the function returns here
      }

      MusECore::Pos p(x, true);
      
      if(posType == MusECore::Song::CPOS && (event->modifiers() & Qt::ShiftModifier )) {        // If shift +LMB we add a marker
            const MusECore::iMarker alreadyExists = MusEGlobal::song->getMarkerAt(x);
            if (alreadyExists == MusEGlobal::song->marker()->end()) {
                  MusEGlobal::song->addMarker(QString(""), x, false);         
                  // Note: Song::addMarker() already emits a 'markerChanged'.
                  //emit addMarker(x);
                  }
            }
      else if (posType == MusECore::Song::RPOS && (event->modifiers() & Qt::ShiftModifier )) {  // If shift +RMB we remove a marker
            const MusECore::iMarker toRemove = MusEGlobal::song->getMarkerAt(x);
            if (toRemove != MusEGlobal::song->marker()->end())
              MusEGlobal::song->removeMarker(toRemove->second);
            else
              fprintf(stderr, "No marker to remove\n");
            }
      else
            MusEGlobal::song->setPos(posType, p);                             // all other cases: relocating one of the locators
      }

//---------------------------------------------------------
//   leaveEvent
//---------------------------------------------------------

void MTScale::leaveEvent(QEvent*)
      {
      emit timeChanged(INT_MAX);
      }

//---------------------------------------------------------
//   drawTickRaster
//---------------------------------------------------------

void MTScale::drawTickRaster(
  QPainter& p, const QRect& mr, const QRegion& /*mrg*/, int /*raster*/,
  bool waveMode, 
  bool /*useGivenColors*/,
  bool /*drawText*/,
  const QColor& /*bar_color*/,
  const QColor& /*beat_color*/,
  const QColor& /*fine_color*/,
  const QColor& /*coarse_color*/,
  const QColor& text_color,
  const QFont& large_font,
  const QFont& small_font
  )
{
      const ViewRect r(mr, true);
      const ViewXCoordinate& x = r._x;
      const ViewXCoordinate x_2(mr.x() + mr.width(), true);
      
      //p.save();
      bool wmtxen = p.worldMatrixEnabled();
      p.setWorldMatrixEnabled(false);

      // Limiter required because MusEGlobal::sigmap.tickValues takes unsigned only !
      const ViewXCoordinate lim_v0(0, false);
      const ViewXCoordinate x_lim = compareXCoordinates(x, lim_v0, CompareLess) ? lim_v0 : x;
      const ViewXCoordinate x_2lim = compareXCoordinates(x_2, lim_v0, CompareLess) ? lim_v0 : x_2;

      const int my = mr.y();
      const int mh = mr.height();

      const int mbottom = mr.bottom();

      const ViewYCoordinate bottom(mr.bottom(), true);

      const int mw1000      = 1000;
      const int mh2         = 2;

      const ViewWCoordinate w2(2, true);
      const ViewHCoordinate h2(2, true);
      const ViewHCoordinate h_m1(mh - 1, true);
      const ViewHCoordinate h_m3(mh - 3, true);
      
      int bar1, bar2, beat;
      ScaleRetStruct scale_info;
      int beat_start_beat;
      unsigned tick;

      QPen pen;
      pen.setCosmetic(true);

// For testing...
//       fprintf(stderr, "View::drawTickRaster_new(): virt:%d drawText:%d mx:%d my:%d mw:%d mh:%draster%d\n",
//               virt(), drawText, mx, my, mw, mh, raster);  
      
      if (waveMode) {
            MusEGlobal::sigmap.tickValues(MusEGlobal::tempomap.frame2tick(asUnmapped(x_lim)._value), &bar1, &beat, &tick);
            MusEGlobal::sigmap.tickValues(MusEGlobal::tempomap.frame2tick(asUnmapped(x_2lim)._value), &bar2, &beat, &tick);
            }
      else {
            MusEGlobal::sigmap.tickValues(asUnmapped(x_lim)._value, &bar1, &beat, &tick);
            MusEGlobal::sigmap.tickValues(asUnmapped(x_2lim)._value, &bar2, &beat, &tick);
            }

      //fprintf(stderr, "bar %d  %d-%d=%d\n", bar, ntick, stick, ntick-stick);

      int stick = MusEGlobal::sigmap.bar2tick(bar1, 0, 0);
      int ntick, deltaTick;
      ScaleRetStruct prev_scale_info;
      for (int bar = bar1; bar <= bar2; bar++, stick = ntick) {
            ntick     = MusEGlobal::sigmap.bar2tick(bar+1, 0, 0);
            deltaTick = ntick - stick;
            int a, b=0;
            double tpix;
            if (waveMode) {
                  a = MusEGlobal::tempomap.tick2frame(ntick);
                  b = MusEGlobal::tempomap.tick2frame(stick);
                     tpix  = rmapx_f(a - b);
               }
            else {
                  tpix  = rmapx_f(deltaTick);
                  }
                  
            scale_info = scale(true, bar, tpix);
            
            if(!scale_info._drawBar)
            {
              // Only check this on the first starting bar.
              if(bar == bar1)
              {
                int prev_a, prev_b=0;
                double prev_tpix;
                int prev_stick;
                int prev_ntick = stick;
                int prev_bar = bar1 - 1;
                for( ; prev_bar >= 0; --prev_bar, prev_ntick = prev_stick)
                {
                  prev_stick = MusEGlobal::sigmap.bar2tick(prev_bar, 0, 0);
                  if (waveMode) {
                        prev_a = MusEGlobal::tempomap.tick2frame(prev_ntick);
                        prev_b = MusEGlobal::tempomap.tick2frame(prev_stick);
                        prev_tpix  = rmapx_f(prev_a - prev_b);
                    }
                  else {
                        prev_tpix  = rmapx_f(prev_ntick - prev_stick);
                        }
                  
                  prev_scale_info = scale(true, prev_bar, prev_tpix);

// For testing...
//                   fprintf(stderr,
//                       "drawTickRaster: Bar check: bar1:%d bar2:%d bar:%d prev_bar:%d"
//                       " stick:%d prev_stick:%d prev_ntick:%d prev_tpix:%f drawBar:%d\n",
//                       bar1, bar2, bar, prev_bar, stick, prev_stick, prev_ntick, prev_tpix,
//                       prev_scale_info._drawBar);

                  if(prev_scale_info._drawBar)
                    break;
                }
                
                if(prev_bar >= 0)
                {
                  const int prev_tick = waveMode ? prev_b : prev_stick;
                  drawBarText(p, prev_tick, prev_bar, mr, text_color, large_font);
                }
              }
                
              continue;
            }
            
            if(scale_info._isSmall)
            {
                  const int tick_sm = waveMode ? b : stick;
                  
                  const ViewXCoordinate x_sm(tick_sm, false);
                  const int mx_sm = asMapped(x_sm)._value;
                  
                  pen.setColor(text_color);
                  p.setPen(pen);
                  
                  if(scale_info._drawBar)
                  {
// For testing...
//                     fprintf(stderr,
//                      "is_small:%d Coarse line: tick_sm:%d mx_sm:%d stick:%d ntick:%d bar1:%d bar2:%d bar:%d\n",
//                      scale_info._isSmall, tick_sm, mx_sm, stick, ntick, bar1, bar2, bar);
                    
                    if(isXInRange(x_sm, x, x_2))
                    {
// For testing...
//                       fprintf(stderr, "is_small:%d Coarse Line is within range."
//                             " Drawing line at mx_sm:%d my:%d mx_sm:%d mbottom:%d...\n",
//                             scale_info._isSmall, mx_sm, my, mx_sm, mbottom);
                      
                      p.drawLine(mx_sm, my, mx_sm, mbottom);
                    }
                  }
                  
// For testing...
//                   fprintf(stderr,
//                     "is_small:%d Coarse text: tick_sm:%d mx_sm:%d stick:%d ntick:%d bar1:%d bar2:%d bar:%d\n",
//                     scale_info._isSmall, tick_sm, mx_sm, stick, ntick, bar1, bar2, bar);

                  drawBarText(p, tick_sm, bar, mr, text_color, large_font);
            }

            if(!scale_info._isSmall) {
                  int z, n;
                  MusEGlobal::sigmap.timesig(stick, z, n);
                  
                  beat_start_beat = 0;
                  for (int beat = beat_start_beat; beat < z; beat++) {
                        int xx = MusEGlobal::sigmap.bar2tick(bar, beat, 0);
                        
//                        int xx_e = MusEGlobal::sigmap.bar2tick(bar, beat + 1, 0);

                        if (waveMode)
                              xx = MusEGlobal::tempomap.tick2frame(xx);
//                        if (waveMode)
//                              xx_e = MusEGlobal::tempomap.tick2frame(xx_e);
                        
                        const ViewXCoordinate xx_v(xx, false);
                        const int mxx = asMapped(xx_v)._value;
                        
                        int y1;
                        int num;
                        int m_yt = my;
                        ViewHCoordinate h_txt;
                        
                        if (beat == 0) {
                              num = bar + 1;
                              y1  = my + 1;
                              h_txt = h_m1;
                              p.setFont(large_font);
                              }
                        else {
                              num = beat + 1;
                              y1  = my + 6;
                              m_yt = my + mh2;
                              h_txt = h_m3;
                              p.setFont(small_font);
                              }

                        const ViewYCoordinate yt(m_yt, true);
                        
                        pen.setColor(text_color);
                        p.setPen(pen);

                        if(isXInRange(xx_v, x, x_2))
                        {
// For testing...
//                           fprintf(stderr, "is_small:%d Beat Line is within range."
//                                 " Drawing line at mxx:%d y1:%d mbottom:%d...\n",
//                                 scale_info._isSmall, mxx, y1, mbottom);
                          
                          p.drawLine(mxx, y1, mxx, mbottom);
                        }

                        if (beat != 0 && !MusEGlobal::config.showTimeScaleBeatNumbers)
                            continue;

                        QString s;
                        s.setNum(num);
                        int brw = p.fontMetrics().boundingRect(s).width();
                        if(brw > mw1000)
                          brw = mw1000;
                        
                        const ViewWCoordinate brw_v(brw, true);
                        const ViewXCoordinate vxx_v = 
                          mathXCoordinates(xx_v, w2, MathAdd);
                        const ViewRect br_txt(vxx_v, yt, brw_v, h_txt);
                              
// For testing...
//                           fprintf(stderr,
//                               "drawBarText: Bar text: bar:%d beat:%d vx:%d vy:%d"
//                               " vw:%d vh:%d tick:%d br x:%d y:%d w:%d h:%d\n",
//                               bar, beat, vr.x(), vr.y(), vr.width(), vr.height(), tick,
//                               br_txt.x(), br_txt.y(), br_txt.width(), br_txt.height());

                        if(intersects(br_txt, r))
                        {
// For testing...
//                           fprintf(stderr,
//                           "is_small:%d Beat text: s:%s stick:%d ntick:%d bar1:%d bar2:%d bar:%d\n",
//                           scale_info._isSmall, s.toLatin1().constData(), stick, ntick, bar1, bar2, bar);

                          p.drawText(asQRectMapped(br_txt), Qt::AlignLeft|Qt::AlignVCenter|Qt::TextDontClip, s);
                        }
                  }
            }
      }

      //p.setWorldMatrixEnabled(true);
      p.setWorldMatrixEnabled(wmtxen);
      //p.restore();      
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
//      const int my12_m1 = my12 - 1;
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
      p.setRenderHint(QPainter::Antialiasing);
      
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
                    xe = mm->second.frame();
                  else
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


              const int pmw = rmapxDev(7);
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

                  p.setBrush(MusEGlobal::config.markerColor);
                  p.setPen(MusEGlobal::config.markerColor);
                  p.drawPolygon( QVector<QPointF>{ {static_cast<qreal>(mapx(pmx)), 1.},
                                                   {static_cast<qreal>(mapx(pmx2)), 6.},
                                                   {static_cast<qreal>(mapx(pmx)), 11.} } );
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

                pen.setColor(MusEGlobal::config.markerColor);
                p.setPen(pen);
                p.drawLine(mxp, my12, mxp, mh);
              }  
            }  
      }

      //---------------------------------------------------
      //    draw location marker
      //---------------------------------------------------

      if (barLocator) {
            pen.setColor(MusEGlobal::config.rangeMarkerColor);
            p.setPen(pen);
            int xp = pos[1];
            int mxp = mapx(xp);
            if (xp >= vx && xp < vx_2)
                  p.drawLine(mxp, my, mxp, mh_m12);
            xp = pos[2];
            mxp = mapx(xp);
            if (xp >= vx && xp < vx_2)
                  p.drawLine(mxp, my, mxp, mh_m12);
            pen.setColor(MusEGlobal::config.positionMarkerColor);
            p.setPen(pen);
            // Draw the red main position cursor last, on top of the others.
            xp = pos[0];
            mxp = mapx(xp);
            if (xp >= vx && xp < vx_2)
                  p.drawLine(mxp, my, mxp, mh_m12);
            }
      else {

          // For testing...
          //fprintf(stderr,
          //  "MTScale::pdraw: Pos mark: x:%d y:%d w:%d h:%d mx:%d my:%d mw:%d mh:%d x2:%d x2_right:%d h_m12:%d i:%d xp:%d pmw:%d pmw_d2:%d pmx:%d pmx2:%d\n",
          //  r.x(), r.y(), r.width(), r.height(), mx, my, mw, mh, x2, x2_right, h_m12, i, xp, pm->width(), pmw_d2, pmx, pmx2);

          // For testing...
          // fprintf(stderr, "...position mark within range. xorg:%d xmag:%d xpos:%d Drawing mark at:%d\n", xorg, xmag, xpos, mapx(pmx));

          const qreal mtop = 18.;
          const qreal mbottom = 26.;
          const int radius = 8;

          // draw left range marker
          {
              const int xp = static_cast<int>(pos[1]);
              const int pmw_d2 = rmapxDev(radius);
              const int pmx = xp - pmw_d2;
              const int pmx2 = xp + pmw_d2;

              if ((pmx >= vx && pmx < vx_2) || (pmx2 > vx && pmx2 <= vx_2) ||
                      (vx >= pmx && vx < pmx2) || (vx_2 > pmx && vx_2 <= pmx2)) {

                  p.setBrush(MusEGlobal::config.rangeMarkerColor);
                  p.setPen(MusEGlobal::config.rangeMarkerColor);
                  p.drawPolygon( QVector<QPointF>{ {static_cast<qreal>(mapx(xp)), mtop},
                                                   {static_cast<qreal>(mapx(pmx)), mtop},
                                                   {static_cast<qreal>(mapx(xp)), mbottom} } );
              }
          }

          // draw right range marker
          {
              const int xp = static_cast<int>(pos[2]);
              const int pmw_d2 = rmapxDev(radius);
              const int pmx = xp - pmw_d2;
              const int pmx2 = xp + pmw_d2;

              if ((pmx >= vx && pmx < vx_2) || (pmx2 > vx && pmx2 <= vx_2) ||
                      (vx >= pmx && vx < pmx2) || (vx_2 > pmx && vx_2 <= pmx2)) {

                  p.setBrush(MusEGlobal::config.rangeMarkerColor);
                  p.setPen(MusEGlobal::config.rangeMarkerColor);
                  p.drawPolygon( QVector<QPointF>{ {static_cast<qreal>(mapx(xp)), mtop},
                                                   {static_cast<qreal>(mapx(pmx2)), mtop},
                                                   {static_cast<qreal>(mapx(xp)), mbottom} } );
              }
          }

          if (pos[2] > pos[1]) {
              QColor c(MusEGlobal::config.rangeMarkerColor);
              c.setAlpha(96);
              QRect rr(QPoint(mapx(static_cast<int>(pos[1])), static_cast<int>(mtop)), QPoint(mapx(static_cast<int>(pos[2])), static_cast<int>(mbottom)));
              p.fillRect(mr & rr, c);
          }


          // draw position marker
          {
              const int xp = static_cast<int>(pos[0]);
              const int pmw_d2 = rmapxDev(radius);
              const int pmx = xp - pmw_d2;
              const int pmx2 = xp + pmw_d2;

              if ((pmx >= vx && pmx < vx_2) || (pmx2 > vx && pmx2 <= vx_2) ||
                      (vx >= pmx && vx < pmx2) || (vx_2 > pmx && vx_2 <= pmx2)) {

                  p.setBrush(MusEGlobal::config.positionMarkerColor);
                  p.setPen(MusEGlobal::config.positionMarkerColor);
                  p.drawPolygon( QVector<QPointF>{ {static_cast<qreal>(mapx(pmx)), mtop},
                                                   {static_cast<qreal>(mapx(pmx2)), mtop},
                                                   {static_cast<qreal>(mapx(xp)), mbottom} } );
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
              pen.setColor(MusEGlobal::config.currentPositionColor);
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
                         Qt::red, // dummy color, initialize to a bold color so it will be evident if it is used
                         Qt::red, // -"-
                         Qt::red, // dummy color, initialize to a bold color so it will be evident if it is used
                         Qt::red, // -"-
                         MusEGlobal::config.rulerFg,
                         MusEGlobal::config.fonts[3], MusEGlobal::config.fonts[4]);
      }


} // namespace MusEGui
