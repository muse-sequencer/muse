//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: waveview.cpp,v 1.10.2.16 2009/11/14 03:37:48 terminator356 Exp $
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

#include <stdio.h>
#include <limits.h>
#include <sys/wait.h>

#include <QPainter>
#include <QDir>
#include <QFileInfo>
#include <QMessageBox>
#include <QMouseEvent>
#include <QFile>

#include "editgain.h"
#include "globals.h"
#include "wave.h"
#include "waveview.h"
#include "song.h"
#include "event.h"
#include "waveedit.h"
#include "audio.h"
#include "gconfig.h"
#include "fastlog.h"

namespace MusEGui {

bool modifyWarnedYet = false;
//---------------------------------------------------------
//   WaveView
//---------------------------------------------------------

WaveView::WaveView(MidiEditor* pr, QWidget* parent, int xscale, int yscale)
   : View(parent, xscale, 1)
      {
      editor = pr;
      setVirt(true);
      pos[0] = MusEGlobal::tempomap.tick2frame(MusEGlobal::song->cpos());
      pos[1] = MusEGlobal::tempomap.tick2frame(MusEGlobal::song->lpos());
      pos[2] = MusEGlobal::tempomap.tick2frame(MusEGlobal::song->rpos());
      yScale = yscale;
      mode = NORMAL;
      selectionStart = 0;
      selectionStop  = 0;
      lastGainvalue = 100;

      setFocusPolicy(Qt::StrongFocus); // Tim.
      
      setMouseTracking(true);
      setBg(QColor(192, 208, 255));

      if (editor->parts()->empty()) {
            curPart = 0;
            curPartId = -1;
            }
      else {
            curPart   = (MusECore::WavePart*)(editor->parts()->begin()->second);
            curPartId = curPart->sn();
            }


      connect(MusEGlobal::song, SIGNAL(posChanged(int,unsigned,bool)), SLOT(setPos(int,unsigned,bool)));
      connect(MusEGlobal::song, SIGNAL(songChanged(MusECore::SongChangedStruct_t)), SLOT(songChanged(MusECore::SongChangedStruct_t)));
      songChanged(SC_SELECTION);
      }

//---------------------------------------------------------
//   setYScale
//---------------------------------------------------------

void WaveView::setYScale(int val)
      {
      yScale = val;
      redraw();
      }

//---------------------------------------------------------
//   draw
//---------------------------------------------------------

// REMOVE Tim. citem. Changed.
// void WaveView::pdraw(QPainter& p, const QRect& rr)
void WaveView::pdraw(QPainter& p, const QRect& rr, const QRegion&)
      {
      int x1 = rr.x();
      int x2 = rr.right() + 1;
      if (x1 < 0)
            x1 = 0;
      if (x2 > width())
            x2 = width();
      int hh = height();
      int h  = hh/2;
      int y  = rr.y() + h;

      // Added by T356.
      int xScale = xmag;
      if (xScale < 0)
            xScale = -xScale;
      
      for (MusECore::iPart ip = editor->parts()->begin(); ip != editor->parts()->end(); ++ip) {
            MusECore::WavePart* wp = (MusECore::WavePart*)(ip->second);
            int channels = wp->track()->channels();
            int px = wp->frame();

            for (MusECore::ciEvent e = wp->events().begin(); e != wp->events().end(); ++e) {
                  const MusECore::Event event&  = e->second;
                  if (event.empty())
                        continue;
                  MusECore::SndFileR f = event.sndFile();
                  if(f.isNull())
                        continue;
                  
                  unsigned peoffset = px + event.frame() - event.spos();
                  int sx, ex;
                  
                  sx = event.frame() + px + xScale/2;
                  ex = sx + event.lenFrame();
                  sx = sx / xScale - xpos;
                  ex = ex / xScale - xpos;

                  if (sx < x1)
                        sx = x1;
                  if (ex > x2)
                        ex = x2;

                  int pos = (xpos + sx) * xScale + event.spos() - event.frame() - px;
                  
                  //printf("pos=%d xpos=%d sx=%d ex=%d xScale=%d event.spos=%d event.frame=%d px=%d\n",   
                  //      pos, xpos, sx, ex, xScale, event.spos(), event.frame(), px);
                  
                  h       = hh / (channels * 2);
                  int cc  = hh % (channels * 2) ? 0 : 1;

                  for (int i = sx; i < ex; i++) {
                        y  = rr.y() + h;
                        MusECore::SampleV sa[f.channels()];
                        f.read(sa, xScale, pos);
                        pos += xScale;
                        if (pos < event.spos())
                              continue;

                        int selectionStartPos = selectionStart - peoffset; // Offset transformed to event coords
                        int selectionStopPos  = selectionStop  - peoffset;

                        for (int k = 0; k < channels; ++k) {
                              int kk = k % f.channels();
                              int peak = (sa[kk].peak * (h - 1)) / yScale;
                              int rms  = (sa[kk].rms  * (h - 1)) / yScale;
                              if (peak > h)
                                    peak = h;
                              if (rms > h)
                                    rms = h;
                              QColor peak_color = QColor(Qt::darkGray);
                              QColor rms_color  = QColor(Qt::black);
                              
                              // Changed by T356. Reduces (but not eliminates) drawing artifacts.
                              //if (pos > selectionStartPos && pos < selectionStopPos) {
                              if (pos > selectionStartPos && pos <= selectionStopPos) {
                                    
                                    peak_color = QColor(Qt::lightGray);
                                    rms_color  = QColor(Qt::white);
                                    // Draw inverted
                                    p.setPen(QColor(Qt::black));
                                    p.drawLine(i, y - h + cc, i, y + h - cc );
                                    }
                              p.setPen(peak_color);
                              p.drawLine(i, y - peak - cc, i, y + peak);
                              p.setPen(rms_color);
                              p.drawLine(i, y - rms - cc, i, y + rms);
                              y  += 2 * h;
                              }
                        }
                  }
            }
      View::pdraw(p, rr);
      }

//---------------------------------------------------------
//   draw
//---------------------------------------------------------

// REMOVE Tim. citem. Changed.
// void WaveView::draw(QPainter& p, const QRect& r)
void WaveView::draw(QPainter& p, const QRect& r, const QRegion&)
      {
      unsigned x = r.x() < 0 ? 0 : r.x();
      unsigned y = r.y() < 0 ? 0 : r.y();
      int w = r.width();
      int h = r.height();

      unsigned x2 = x + w;
      unsigned y2 = y + h;

      //
      //    draw marker & centerline
      //
      p.setPen(Qt::blue);
      if (pos[1] >= x && pos[1] < x2) {
            p.drawLine(pos[1], y, pos[1], y2);
            }
      if (pos[2] >= x && pos[2] < x2)
            p.drawLine(pos[2], y, pos[2], y2);
      p.setPen(Qt::red);
      if (pos[0] >= x && pos[0] < x2) {
            p.drawLine(pos[0], y, pos[0], y2);
            }

      int n = 1;
      if(curPart)
        n = curPart->track()->channels();
      
      int hn = h / n;
      int hh = hn / 2;
      for (int i = 0; i < n; ++i) {
            int h2     = hn * i;
            int center = hh + h2;
            p.setPen(QColor(i & i ? Qt::red : Qt::blue));
            p.drawLine(x, center, x2, center);
            p.setPen(QColor(Qt::black));
            p.drawLine(x, h2, x2, h2);
            }
      }

//---------------------------------------------------------
//   getCaption
//---------------------------------------------------------

QString WaveView::getCaption() const
      {
      if(curPart)
        return QString("Part ") + curPart->name();
      else  
        return QString("Part ");
        
      }

//---------------------------------------------------------
//   songChanged
//---------------------------------------------------------

void WaveView::songChanged(MusECore::SongChangedStruct_t flags)
      {
      if (flags & ~(SC_SELECTION | SC_PART_SELECTION | SC_TRACK_SELECTION)) {
            // TODO FIXME: don't we actually only want SC_PART_*, and maybe SC_TRACK_DELETED?
            //             (same in ecanvas.cpp)
            startSample  = INT_MAX;
            endSample    = 0;
            curPart      = 0;
            for (MusECore::iPart p = editor->parts()->begin(); p != editor->parts()->end(); ++p) {
                  MusECore::WavePart* part = (MusECore::WavePart*)(p->second);
                  if (part->sn() == curPartId)
                        curPart = part;
                  int ssample = part->frame();
                  int esample = ssample + part->lenFrame();
                  if (ssample < startSample) {
                        startSample = ssample;
                        //printf("startSample = %d\n", startSample);
                        }
                  if (esample > endSample) {
                        endSample = esample;
                        //printf("endSample = %d\n", endSample);
                        }
                  }
            }
      if (flags & SC_CLIP_MODIFIED) {
            redraw(); // Boring, but the only thing possible to do
            }
      if (flags & SC_TEMPO) {
            setPos(0, MusEGlobal::song->cpos(), false);
            setPos(1, MusEGlobal::song->lpos(), false);
            setPos(2, MusEGlobal::song->rpos(), false);
            }
      redraw();
      }

//---------------------------------------------------------
//   setPos
//    set one of three markers
//    idx   - 0-cpos  1-lpos  2-rpos
//    flag  - emit followEvent()
//---------------------------------------------------------

void WaveView::setPos(int idx, unsigned val, bool adjustScrollbar)
      {
      val = MusEGlobal::tempomap.tick2frame(val);
      if (pos[idx] == val)
            return;
      int opos = mapx(pos[idx]);
      int npos = mapx(val);

      if (adjustScrollbar && idx == 0) {
            switch (MusEGlobal::song->follow()) {
                  case  MusECore::Song::NO:
                        break;
                  case MusECore::Song::JUMP:
                        if (npos >= width()) {
                              int ppos =  val - xorg - rmapxDev(width()/4);
                              if (ppos < 0)
                                    ppos = 0;
                              emit followEvent(ppos);
                              opos = mapx(pos[idx]);
                              npos = mapx(val);
                              }
                        else if (npos < 0) {
                              int ppos =  val - xorg - rmapxDev(width()*3/4);
                              if (ppos < 0)
                                    ppos = 0;
                              emit followEvent(ppos);
                              opos = mapx(pos[idx]);
                              npos = mapx(val);
                              }
                        break;
	    case MusECore::Song::CONTINUOUS:
                        if (npos > (width()*5)/8) {
                              int ppos =  pos[idx] - xorg - rmapxDev(width()*5/8);
                              if (ppos < 0)
                                    ppos = 0;
                              emit followEvent(ppos);
                              opos = mapx(pos[idx]);
                              npos = mapx(val);
                              }
                        else if (npos < (width()*3)/8) {
                              int ppos =  pos[idx] - xorg - rmapxDev(width()*3/8);
                              if (ppos < 0)
                                    ppos = 0;
                              emit followEvent(ppos);
                              opos = mapx(pos[idx]);
                              npos = mapx(val);
                              }
                        break;
                  }
            }

      int x;
      int w = 1;
      if (opos > npos) {
            w += opos - npos;
            x = npos;
            }
      else {
            w += npos - opos;
            x = opos;
            }
      pos[idx] = val;
      //redraw(QRect(x, 0, w, height()));
      redraw(QRect(x-1, 0, w+2, height()));    // p4.0.28 From Canvas::draw (is otherwise identical). Fix for corruption.
      }

//---------------------------------------------------------
//   viewMousePressEvent
//---------------------------------------------------------

void WaveView::viewMousePressEvent(QMouseEvent* event)
      {
      button = event->button();
      unsigned x = event->x();

      switch (button) {
            case Qt::LeftButton:
                  if (mode == NORMAL) {
                        // redraw and reset:
                        if (selectionStart != selectionStop) {
                              selectionStart = selectionStop = 0;
                              redraw();
                              }
                        mode = DRAG;
                        dragstartx = x;
                        selectionStart = selectionStop = x;
                        }
                  break;

            case Qt::MidButton:
            case Qt::RightButton:
            default:
                  break;
            }
      viewMouseMoveEvent(event);
      }

#define WHEEL_STEPSIZE 50
#define WHEEL_DELTA   120
//---------------------------------------------------------
//   wheelEvent
//---------------------------------------------------------
void WaveView::wheelEvent(QWheelEvent* ev)
{
  int keyState = ev->modifiers();

  bool shift      = keyState & Qt::ShiftModifier;
  bool ctrl       = keyState & Qt::ControlModifier;

  if (shift) { // scroll vertically
      int delta       = -ev->delta() / WHEEL_DELTA;
      int xpixelscale = 5*MusECore::fast_log10(rmapxDev(1));


      if (xpixelscale <= 0)
            xpixelscale = 1;

      int scrollstep = WHEEL_STEPSIZE * (delta);
      ///if (ev->state() == Qt::ShiftModifier)
  //      if (((QInputEvent*)ev)->modifiers() == Qt::ShiftModifier)
      scrollstep = scrollstep / 10;

      int newXpos = xpos + xpixelscale * scrollstep;

      if (newXpos < 0)
            newXpos = 0;

      //setYPos(newYpos);
      emit horizontalScroll((unsigned)newXpos);


  } else if (ctrl) {  // zoom horizontally
    if (ev->delta()>0)
      emit horizontalZoomIn();
    else
      emit horizontalZoomOut();

  } else { // scroll horizontally
      emit mouseWheelMoved(ev->delta() / 10);
  }

}

//---------------------------------------------------------
//   viewMouseReleaseEvent
//---------------------------------------------------------
void WaveView::viewMouseReleaseEvent(QMouseEvent* /*event*/)
      {
      button = Qt::NoButton;

      if (mode == DRAG) {
            mode = NORMAL;
            //printf("selectionStart=%d selectionStop=%d\n", selectionStart, selectionStop);
            }
      }

//---------------------------------------------------------
//   viewMouseMoveEvent
//---------------------------------------------------------

void WaveView::viewMouseMoveEvent(QMouseEvent* event)
      {
      unsigned x = event->x();
      emit timeChanged(x);

      int i;
      switch (button) {
            case Qt::LeftButton:
                  i = 0;
                  if (mode == DRAG) {
                        if (x < dragstartx) {
                              selectionStart = x;
                              selectionStop = dragstartx;
                              }
                        else {
                              selectionStart = dragstartx;
                              selectionStop = x;
                              }
                        }
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
      MusECore::Pos p(MusEGlobal::tempomap.frame2tick(x), true);
      MusEGlobal::song->setPos(i, p);
      }

//---------------------------------------------------------
//   range
//    returns range in samples
//---------------------------------------------------------

void WaveView::range(int* s, int *e)
      {
      
      MusECore::PartList* lst = editor->parts();
      if(lst->empty())
      {
        *s = 0;
        *e = MusEGlobal::tempomap.tick2frame(MusEGlobal::song->len());
        return;  
      }
      int ps = MusEGlobal::song->len(), pe = 0;
      int tps, tpe;
      for(MusECore::iPart ip = lst->begin(); ip != lst->end(); ++ip) 
      {
        tps = ip->second->tick();
        if(tps < ps)
          ps = tps;
        tpe = tps + ip->second->lenTick();
        if(tpe > pe)
          pe = tpe;
      }
      *s = MusEGlobal::tempomap.tick2frame(ps);
      *e = MusEGlobal::tempomap.tick2frame(pe);
      }



} // namespace MusECore
