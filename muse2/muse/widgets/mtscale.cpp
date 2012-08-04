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
      connect(MusEGlobal::song, SIGNAL(songChanged(int)), SLOT(songChanged(int)));
      connect(MusEGlobal::song, SIGNAL(markerChanged(int)), SLOT(redraw()));
	
      setFixedHeight(28);
      setBg(QColor(0xe0, 0xe0, 0xe0));
      }

//---------------------------------------------------------
//   songChanged
//---------------------------------------------------------

void MTScale::songChanged(int type)
      {
      if (type & (SC_SIG|SC_TEMPO)) {
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
      //unsigned opos = mapx(pos[idx] == INT_MAX ? val : pos[idx]);
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
      //if (opos > (unsigned int) tval) {	//prevent compiler warning: comparison signed/unsigned
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
      if (waveMode)
            x = MusEGlobal::tempomap.frame2tick(x);
      x = AL::sigmap.raster(x, *raster);
      if (x < 0)
            x = 0;
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
            MusECore::Marker *alreadyExists = MusEGlobal::song->getMarkerAt(x);
            if (!alreadyExists) {
                  MusEGlobal::song->addMarker(QString(""), x, false);         
                  // Removed p3.3.43 
                  // Song::addMarker() already emits a 'markerChanged'.
                  //emit addMarker(x);
                  }
            }
      else if (i== 2 && (event->modifiers() & Qt::ShiftModifier )) {  // If shift +RMB we remove a marker 
            MusECore::Marker *toRemove = MusEGlobal::song->getMarkerAt(x);
            if (toRemove)
              MusEGlobal::song->removeMarker(toRemove);
            else
              printf("No marker to remove\n");
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
//   draw
//---------------------------------------------------------

void MTScale::pdraw(QPainter& p, const QRect& r)
      {
      int x = r.x();
      int w = r.width();

      // Added by Tim. p3.3.6
      //printf("MTScale::pdraw x:%d w:%d\n", x, w);
      
      x -= 20;
      w += 40;    // wg. Text

      //---------------------------------------------------
      //    draw Marker
      //---------------------------------------------------

      int y = 12;
      p.setPen(Qt::black);
      p.setFont(MusEGlobal::config.fonts[5]);
      p.drawLine(r.x(), y+1, r.x() + r.width(), y+1);
      QRect tr(r);
      tr.setHeight(12);
      MusECore::MarkerList* marker = MusEGlobal::song->marker();
      for (MusECore::iMarker m = marker->begin(); m != marker->end(); ++m) {
            
            int xp;
            if(waveMode) 
              xp = mapx(m->second.frame());
            else  
              xp = mapx(m->second.tick());
            if (xp > x+w)
                  break;
            int xe = r.x() + r.width();
            MusECore::iMarker mm = m;
            ++mm;
            if (mm != marker->end()) {
                  
                  if(waveMode) 
                    xe = mapx(MusEGlobal::tempomap.tick2frame(mm->first));
                  else
                    xe = mapx(mm->first);
                  }
            
            QRect tr(xp, 0, xe-xp, 13);
            //if (m->second.current()) 
            //      p.fillRect(tr, white);
                    
            QRect wr = r.intersect(tr);
            //if (r.intersects(tr)) 
            if(!wr.isEmpty()) 
            {        
              if (m->second.current()) 
              {
                    //p.fillRect(tr, white);
                    p.fillRect(wr, Qt::white);
              }
              
              int x2;
              //MusECore::iMarker mm = m;
              //++mm;
              if (mm != marker->end())
              {
                    if(waveMode) 
                      x2 = mapx(MusEGlobal::tempomap.tick2frame(mm->first));
                    else
                      x2 = mapx(mm->first);
              }      
              else
                    x2 = xp+200;
              
              //printf("MTScale::pdraw marker %s xp:%d y:%d h:%d r.x:%d r.w:%d\n", m->second.name().toLatin1(), xp, height(), y, r.x(), r.width());
  
              // Must be reasonable about very low negative x values! With long songs > 15min
              //  and with high horizontal magnification, 'ghost' drawings appeared,
              //  apparently the result of truncation later (xp = -65006 caused ghosting
              //  at bar 245 with magnification at max.), even with correct clipping region
              //  applied to painter in View::paint(). Tim.  Apr 5 2009 
              // Quote: "Warning: Note that QPainter does not attempt to work around 
              //  coordinate limitations in the underlying window system. Some platforms may 
              //  behave incorrectly with coordinates as small as +/-4000."
              if(xp >= -32)
                p.drawPixmap(xp, 0, *flagIconS);
                
              if(xp >= -1023)
              {
                QRect r = QRect(xp+10, 0, x2-xp, 12);
                p.setPen(Qt::black);
                p.drawText(r, Qt::AlignLeft|Qt::AlignVCenter, m->second.name());
              }  
              
              if(xp >= 0)
              {
                p.setPen(Qt::green);
                p.drawLine(xp, y, xp, height());
              }  
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
      if (pos[3] != INT_MAX) {
            int xp = mapx(pos[3]);
            if (xp >= x && xp < x+w)
                  p.drawLine(xp, 0, xp, height());
            }

      unsigned ctick;
      int bar1, bar2, beat;
      unsigned tick;

      if (waveMode) {
            ctick = MusEGlobal::tempomap.frame2tick(mapxDev(x));
            AL::sigmap.tickValues(ctick, &bar1, &beat, &tick);
            AL::sigmap.tickValues(MusEGlobal::tempomap.frame2tick(mapxDev(x+w)),
               &bar2, &beat, &tick);
            }
      else {
            ctick = mapxDev(x);
            AL::sigmap.tickValues(ctick, &bar1, &beat, &tick);
            AL::sigmap.tickValues(mapxDev(x+w), &bar2, &beat, &tick);
            }

//printf("bar %d  %d-%d=%d\n", bar, ntick, stick, ntick-stick);

      int stick = AL::sigmap.bar2tick(bar1, 0, 0);
      int ntick;
      for (int bar = bar1; bar <= bar2; bar++, stick = ntick) {
            ntick     = AL::sigmap.bar2tick(bar+1, 0, 0);
            int tpix, a, b=0;
            if (waveMode) {
                  a = MusEGlobal::tempomap.tick2frame(ntick);
                  b = MusEGlobal::tempomap.tick2frame(stick);
                  tpix  = rmapx(a - b);
                  }
            else {
                  tpix  = rmapx(ntick - stick);
                  }
            if (tpix < 64) {
                  // donï¿½t show beats if measure is this small
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
                  p.setFont(MusEGlobal::config.fonts[3]);
                  int x = mapx(waveMode ? b : stick);
                  QString s;
                  s.setNum(bar + 1);
                  p.drawLine(x, y+1, x, y+1+h);
//                  QRect r = QRect(x+2, y, 0, h);
                  QRect r = QRect(x+2, y, 1000, h);
                  p.drawText(r, Qt::AlignLeft|Qt::AlignVCenter|Qt::TextDontClip, s);
                  }
            else {
                  int z, n;
                  AL::sigmap.timesig(stick, z, n);
                  for (int beat = 0; beat < z; beat++) {
                        int xx = AL::sigmap.bar2tick(bar, beat, 0);
                        if (waveMode)
                              xx = MusEGlobal::tempomap.tick2frame(xx);
                        int xp = mapx(xx);
                        QString s;
                        QRect r(xp+2, y, 1000, h);
                        int y1;
                        int num;
                        if (beat == 0) {
                              num = bar + 1;
                              y1  = y + 1;
                              p.setFont(MusEGlobal::config.fonts[3]);
                              }
                        else {
                              num = beat + 1;
                              y1  = y + 7;
                              p.setFont(MusEGlobal::config.fonts[4]);
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
