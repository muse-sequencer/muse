//=========================================================
//  MusE
//  Linux Music Editor
//    wavecanvas.cpp
//  (C) Copyright 2012 Tim E. Real (terminator356 on users dot sourceforge dot net)
//
//  Based on WaveView.cpp and PianoCanvas.cpp
//  (C) Copyright 2000 Werner Schweer (ws@seh.de)
//   and others.
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


#include <QApplication>
#include <QClipboard>
#include <QPainter>
#include <QDrag>
#include <QDragLeaveEvent>
#include <QDragEnterEvent>
#include <QDragMoveEvent>
#include <QDropEvent>
#include <QFile>
#include <QInputDialog>
#include <QMouseEvent>
#include <QList>
#include <QPair>
#include <QMessageBox>
#include <QDir>
#include <QLine>
#include <QVector>

#include <set>

#include <limits.h>
#include <stdio.h>
#include <math.h>
#include <errno.h>
#include <sys/wait.h>
#include <set>

#include "app.h"
#include "icons.h"
#include "xml.h"
#include "wavecanvas.h"
#include "event.h"
#include "globals.h"
#include "cmd.h"
#include "song.h"
#include "audio.h"
#include "functions.h"
#include "gconfig.h"
#include "shortcuts.h"
#include "editgain.h"
#include "wave.h"
#include "waveedit.h"
#include "fastlog.h"
#include "utils.h"
#include "tools.h"
#include "copy_on_write.h"
#include "helper.h"

namespace MusEGui {

//---------------------------------------------------------
//   WEvent
//---------------------------------------------------------

WEvent::WEvent(MusECore::Event& e, MusECore::Part* p, int height) : MusEGui::CItem(e, p)
      {
      unsigned frame = e.frame() + p->frame();
      setPos(QPoint(frame, 0));
      unsigned len = e.lenFrame();
      if(e.frame() + e.lenFrame() >= p->lenFrame())
        len = p->lenFrame() - e.frame();
      setBBox(QRect(frame, 0, len, height));    
      }

//---------------------------------------------------------
//   addItem
//---------------------------------------------------------

CItem* WaveCanvas::addItem(MusECore::Part* part, MusECore::Event& event)
      {
      if (signed(event.frame())<0) {
            printf("ERROR: trying to add event before current part!\n");
            return NULL;
      }

      WEvent* ev = new WEvent(event, part, height());  
      items.add(ev);

      int diff = event.frame()-part->lenFrame();
      if (diff > 0)  {// too short part? extend it
            part->setLenFrame(part->lenFrame()+diff);
            }
      
      return ev;
      }

//---------------------------------------------------------
//   WaveCanvas
//---------------------------------------------------------

WaveCanvas::WaveCanvas(MidiEditor* pr, QWidget* parent, int sx, int sy)
   : EventCanvas(pr, parent, sx, 1)
      {
      colorMode = 0;
      button = 0;
      
      editor = pr;
      setVirt(true);
      
      setBg(QColor());
      
      pos[0] = MusEGlobal::tempomap.tick2frame(MusEGlobal::song->cpos());
      pos[1] = MusEGlobal::tempomap.tick2frame(MusEGlobal::song->lpos());
      pos[2] = MusEGlobal::tempomap.tick2frame(MusEGlobal::song->rpos());
      yScale = sy;
      mode = NORMAL;
      selectionStart = XTick::createInvalidXTick();
      selectionStop  = XTick::createInvalidXTick();
      lastGainvalue = 100;

      songChanged(SC_TRACK_INSERTED);
      }

WaveCanvas::~WaveCanvas()
{
  //delete steprec;
}

//---------------------------------------------------------
//   songChanged(type)
//---------------------------------------------------------

void WaveCanvas::songChanged(MusECore::SongChangedFlags_t flags)
      {
      // Is it simply a midi controller value adjustment? Forget it.
      if(flags == SC_MIDI_CONTROLLER)
        return;
    
      if (flags & ~SC_SELECTION) {
            // TODO FIXME: don't we actually only want SC_PART_*, and maybe SC_TRACK_DELETED?
            //             (same in waveview.cpp)
            bool curItemNeedsRestore=false;
            MusECore::Event storedEvent;
            int partSn;
            if (curItem)
            {
              curItemNeedsRestore=true;
              storedEvent=curItem->event();
              partSn=curItem->part()->sn();
            }
            curItem=NULL;
            
            items.clearDelete();
            startSample  = INT_MAX;
            endSample    = 0;
            curPart = 0;
            for (MusECore::iPart p = editor->parts()->begin(); p != editor->parts()->end(); ++p) {
                  MusECore::WavePart* part = (MusECore::WavePart*)(p->second);
                  if (part->sn() == curPartId)
                        curPart = part;
                  unsigned ssample = part->frame();
                  unsigned len = part->lenFrame();
                  unsigned esample = ssample + len;
                  if (ssample < startSample)
                        startSample = ssample;
                  if (esample > endSample)
                        endSample = esample;

                  MusECore::EventList* el = part->events();
                  for (MusECore::iEvent i = el->begin(); i != el->end(); ++i) {
                        MusECore::Event e = i->second;
                        // Do not add events which are past the end of the part.
                        if(e.frame() > len)      
                          break;
                        
                        if (e.type() == MusECore::Wave) {
                              CItem* temp = addItem(part, e);
                              
                              if (temp && curItemNeedsRestore && e==storedEvent && part->sn()==partSn)
                              {
                                  if (curItem!=NULL)
                                    printf("THIS SHOULD NEVER HAPPEN: curItemNeedsRestore=true, event fits, but there was already a fitting event!?\n");
                                  
                                  curItem=temp;
                                  }
                              }
                        }
                  }
            }

      MusECore::Event event;
      MusECore::WavePart* part   = 0;
      int x            = 0;
      CItem*   nevent  = 0;

      int n  = 0;       // count selections
      for (iCItem k = items.begin(); k != items.end(); ++k) {
            MusECore::Event ev = k->second->event();
            bool selected = ev.selected();
            if (selected) {
                  k->second->setSelected(true);
                  ++n;
                  if (!nevent) {
                        nevent   =  k->second;
                        MusECore::Event mi = nevent->event();
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
      
      if (n >= 1)    
      {
            x     = nevent->x();
            event = nevent->event();
            part  = (MusECore::WavePart*)nevent->part();
            if (_setCurPartIfOnlyOneEventIsSelected && n == 1 && curPart != part) {
                  curPart = part;
                  curPartId = curPart->sn();
                  curPartChanged();
                  }
      }
      
      bool f1 = flags & (SC_EVENT_INSERTED | SC_EVENT_MODIFIED | SC_EVENT_REMOVED | 
                         SC_PART_INSERTED | SC_PART_MODIFIED | SC_PART_REMOVED |
                         SC_TRACK_INSERTED | SC_TRACK_REMOVED | SC_TRACK_MODIFIED |
                         SC_SIG | SC_TEMPO | SC_KEY | SC_MASTER | SC_CONFIG | SC_DRUMMAP); 
      bool f2 = flags & SC_SELECTION;
      if(f1 || f2)   // Try to avoid all unnecessary emissions.
        emit selectionChanged(x, event, part, !f1);
      
      if (curPart == 0)
            curPart = (MusECore::WavePart*)(editor->parts()->begin()->second);
      redraw();
      }

//---------------------------------------------------------
//   selectAtTick
//---------------------------------------------------------

void WaveCanvas::selectAtTick(unsigned int tick)
      {
      selectAtFrame(MusEGlobal::tempomap.tick2frame(tick));
      }

//---------------------------------------------------------
//   selectAtFrame
//---------------------------------------------------------

void WaveCanvas::selectAtFrame(unsigned int frame)
      {
      //Select event nearest frame, if none selected and there are any
      if (!items.empty() && selectionSize() == 0) {
            iCItem i = items.begin();
            CItem* nearest = i->second;

            while (i != items.end()) {
                CItem* cur=i->second;                
                unsigned int curf=abs(cur->x() + cur->part()->frame() - frame);
                unsigned int nearf=abs(nearest->x() + nearest->part()->frame() - frame);

                if (curf < nearf) {
                    nearest=cur;
                    }

                i++;
                }

            if (!nearest->isSelected()) {
                  selectItem(nearest, true);
                  songChanged(SC_SELECTION);
                  }
            }
      }

//---------------------------------------------------------
//   getCaption
//---------------------------------------------------------

QString WaveCanvas::getCaption() const
      {
      int bar1, bar2, xx;
      unsigned x;
      AL::sigmap.tickValues(curPart->tick(), &bar1, &xx, &x);
      AL::sigmap.tickValues(curPart->tick() + curPart->lenTick(), &bar2, &xx, &x);

      return QString("MusE: Part <") + curPart->name()
         + QString("> %1-%2").arg(bar1+1).arg(bar2+1);
      }

//---------------------------------------------------------
//   track
//---------------------------------------------------------

MusECore::WaveTrack* WaveCanvas::track() const
      {
      return ((MusECore::WavePart*)curPart)->track();
      }


//---------------------------------------------------------
//   keyPress
//---------------------------------------------------------

void WaveCanvas::keyPress(QKeyEvent* event)
      {
      int key = event->key();
      if (((QInputEvent*)event)->modifiers() & Qt::ShiftModifier)
            key += Qt::SHIFT;
      if (((QInputEvent*)event)->modifiers() & Qt::AltModifier)
            key += Qt::ALT;
      if (((QInputEvent*)event)->modifiers() & Qt::ControlModifier)
            key+= Qt::CTRL;

      // TODO: New WaveCanvas: Convert these to frames, and remove unneeded functions.
            
      //
      //  Shortcut for DrumEditor & PianoRoll
      //  Sets locators to selected events
      //
      if (key == shortcuts[SHRT_LOCATORS_TO_SELECTION].key) {
            int tick_max = 0;
            int tick_min = INT_MAX;
            bool found = false;

            for (iCItem i= items.begin(); i != items.end(); i++) {
                  if (!i->second->isSelected())
                        continue;

                  int tick = i->second->x();
                  int len = i->second->event().lenTick();
                  found = true;
                  if (tick + len > tick_max)
                        tick_max = tick + len;
                  if (tick < tick_min)
                        tick_min = tick;
                  }
            if (found) {
                  MusEGlobal::song->setPos(1, MusECore::Pos(MusECore::XTick(tick_min)));
                  MusEGlobal::song->setPos(2, MusECore::Pos(MusECore::XTick(tick_max)));
                  }
            }
      // Select items by key (PianoRoll & DrumEditor)
      else if (key == shortcuts[SHRT_SEL_RIGHT].key || key == shortcuts[SHRT_SEL_RIGHT_ADD].key) {
              rciCItem i;

              if (items.empty())
                  return;
              for (i = items.rbegin(); i != items.rend(); ++i) 
                if (i->second->isSelected()) 
                  break;

              if(i == items.rend())
                i = items.rbegin();
              
              if(i != items.rbegin())
                --i;
              if(i->second)
              {
                if (key != shortcuts[SHRT_SEL_RIGHT_ADD].key)
                      deselectAll();
                CItem* sel = i->second;
                sel->setSelected(true);
                updateSelection();
                if (sel->x() + sel->width() > mapxDev(width())) 
                {  
                  int mx = rmapx(sel->x());  
                  int newx = mx + rmapx(sel->width()) - width();
                  // Leave a bit of room for the specially-drawn drum notes. But good for piano too.
                  emit horizontalScroll( (newx > mx ? mx - 10: newx + 10) - rmapx(xorg) );
                }  
              }
            }
      //Select items by key: (PianoRoll & DrumEditor)
      else if (key == shortcuts[SHRT_SEL_LEFT].key || key == shortcuts[SHRT_SEL_LEFT_ADD].key) {
              ciCItem i;
              if (items.empty())
                  return;
              for (i = items.begin(); i != items.end(); ++i)
                if (i->second->isSelected()) 
                  break;

              if(i == items.end())
                i = items.begin();
              
              if(i != items.begin())
                --i;
              if(i->second)
              {
                if (key != shortcuts[SHRT_SEL_LEFT_ADD].key)
                      deselectAll();
                CItem* sel = i->second;
                sel->setSelected(true);
                updateSelection();
                if (sel->x() <= mapxDev(0)) 
                  emit horizontalScroll(rmapx(sel->x() - xorg) - 10);  // Leave a bit of room.
              }
            }
      //else if (key == shortcuts[SHRT_INC_PITCH].key) {
      //      modifySelected(NoteInfo::VAL_PITCH, 1);
      //      }
      //else if (key == shortcuts[SHRT_DEC_PITCH].key) {
      //      modifySelected(NoteInfo::VAL_PITCH, -1);
      //      }
      else if (key == shortcuts[SHRT_INC_POS].key) {
            // TODO: Check boundaries
            modifySelected(NoteInfo::VAL_TIME, editor->raster());
            }
      else if (key == shortcuts[SHRT_DEC_POS].key) {
            // TODO: Check boundaries
            modifySelected(NoteInfo::VAL_TIME, 0 - editor->raster());
            }

      else if (key == shortcuts[SHRT_INCREASE_LEN].key) {
            // TODO: Check boundaries
            modifySelected(NoteInfo::VAL_LEN, editor->raster());
            }
      else if (key == shortcuts[SHRT_DECREASE_LEN].key) {
            // TODO: Check boundaries
            modifySelected(NoteInfo::VAL_LEN, 0 - editor->raster());
            }

      else
            event->ignore();
      }


//---------------------------------------------------------
//   setPos
//    set one of three markers
//    idx   - 0-cpos  1-lpos  2-rpos
//    flag  - emit followEvent()
//---------------------------------------------------------

void WaveCanvas::setPos(int idx, unsigned val, bool adjustScrollbar)
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
      redraw(QRect(x-1, 0, w+2, height()));    // From Canvas::draw (is otherwise identical). Fix for corruption. (TEST: New WaveCanvas: Still true?)
      }

//---------------------------------------------------------
//   setYScale
//---------------------------------------------------------

void WaveCanvas::setYScale(int val)
      {
      yScale = val;
      redraw();
      }

// TODO: Overridden because markers need tick2frame. 
//       After BBT/frame mode is added to Canvas, remove this override and let Canvas::draw do it.
//       Also add the drawParts calls to Canvas::draw, and add a dummy Canvas::drawParts { }.
//---------------------------------------------------------
//   draw
//---------------------------------------------------------

void WaveCanvas::draw(QPainter& p, const QRect& r)
      {
      int x = r.x() < 0 ? 0 : r.x();
      int y = r.y() < 0 ? 0 : r.y();
      int w = r.width();
      int h = r.height();
      int x2 = x + w;

      std::vector<CItem*> list1;
      std::vector<CItem*> list2;
      //std::vector<CItem*> list3;
      std::vector<CItem*> list4;

      drawCanvas(p, r);

      //---------------------------------------------------
      // draw Canvas Items
      //---------------------------------------------------

      iCItem to(items.lower_bound(x2));
      
      for(iCItem i = items.begin(); i != to; ++i)
      { 
        CItem* ci = i->second;
        // NOTE Optimization: For each item call this once now, then use cached results later via cachedHasHiddenEvents().
        // Not required for now.
        //ci->part()->hasHiddenEvents();
        
        // Draw items from other parts behind all others. Only for items with events (not arranger parts).
        if(!ci->event().empty() && ci->part() != curPart)
          list1.push_back(ci);    
        else if(!ci->isMoving() && (ci->event().empty() || ci->part() == curPart))
        {
          // Draw selected parts in front of all others.
          if(ci->isSelected()) 
            list4.push_back(ci);
          // Draw clone parts, and parts with hidden events, in front of others all except selected.
          //else if(ci->event().empty() && (ci->part()->events()->arefCount() > 1 || ci->part()->cachedHasHiddenEvents()))
          // Draw clone parts in front of others all except selected.
          //else if(ci->event().empty() && (ci->part()->events()->arefCount() > 1))
          //  list3.push_back(ci);
          else  
            // Draw unselected parts.
            list2.push_back(ci);
        }  
      }

      // Draw non-current part backgrounds behind all others:
      drawParts(p, r, false);
      
      int i;
      int sz = list1.size();
      for(i = 0; i != sz; ++i) 
        drawItem(p, list1[i], r);
      
      // Draw current part background in front of all others:
      drawParts(p, r, true);
      
      sz = list2.size();
      for(i = 0; i != sz; ++i) 
        drawItem(p, list2[i], r);

      //sz = list3.size();
      //for(i = 0; i != sz; ++i) 
      //  drawItem(p, list3[i], rect);
      
      sz = list4.size();
      for(i = 0; i != sz; ++i) 
        drawItem(p, list4[i], r);
      
      to = moving.lower_bound(x2);
      for (iCItem i = moving.begin(); i != to; ++i) 
      {
            drawItem(p, i->second, r);
      }

      drawTopItem(p,r);


      //---------------------------------------------------
      //    draw marker
      //---------------------------------------------------

      bool wmtxen = p.worldMatrixEnabled();
      p.setWorldMatrixEnabled(false);
      
      int my = mapy(y);
      int my2 = mapy(y + h);
      
      MusECore::MarkerList* marker = MusEGlobal::song->marker();
      for (MusECore::iMarker m = marker->begin(); m != marker->end(); ++m) {
            int xp = MusEGlobal::tempomap.tick2frame(m->second.tick());
            if (xp >= x && xp < x2) {
                  p.setPen(Qt::green);
                  p.drawLine(mapx(xp), my, mapx(xp), my2);
                  }
            }

      //---------------------------------------------------
      //    draw location marker
      //---------------------------------------------------

      // Tip: These positions are already in units of frames.
      p.setPen(Qt::blue);
      int mx;
      if (pos[1] >= unsigned(x) && pos[1] < unsigned(x2)) {
            mx = mapx(pos[1]);
            p.drawLine(mx, my, mx, my2);
            }
      if (pos[2] >= unsigned(x) && pos[2] < unsigned(x2)) {
            mx = mapx(pos[2]);
            p.drawLine(mx, my, mx, my2);
            }
      p.setPen(Qt::red);
      if (pos[0] >= unsigned(x) && pos[0] < unsigned(x2)) {
            mx = mapx(pos[0]);
            p.drawLine(mx, my, mx, my2);
            }
            
      if(drag == DRAG_ZOOM)
        p.drawPixmap(mapFromGlobal(global_start), *zoomAtIcon);
        
      //p.restore();
      //p.setWorldMatrixEnabled(true);
      p.setWorldMatrixEnabled(wmtxen);
      
      //---------------------------------------------------
      //    draw lasso
      //---------------------------------------------------

      if (drag == DRAG_LASSO) {
            p.setPen(Qt::blue);
            p.setBrush(Qt::NoBrush);
            p.drawRect(lasso);
            }
      
      //---------------------------------------------------
      //    draw moving items
      //---------------------------------------------------
      
      for(iCItem i = moving.begin(); i != moving.end(); ++i) 
        drawMoving(p, i->second, r);

      }

//---------------------------------------------------------
//   drawWaveParts
//---------------------------------------------------------

void WaveCanvas::drawParts(QPainter& p, const QRect& r, bool do_cur_part)
{
      //QRect rr = p.transform().mapRect(r);  // Gives inconsistent positions. Source shows wrong operation for our needs.
      QRect rr = map(r);                      // Use our own map instead.        

      bool wmtxen = p.worldMatrixEnabled();
      p.setWorldMatrixEnabled(false);

      if(do_cur_part)
      {
        // Draw current part:
        if(curPart)
        {
              QRect mwpr  = map(QRect(curPart->frame(), 0, curPart->lenFrame(), height()));
              QRect mpbgr = rr & mwpr;
              if(!mpbgr.isNull())
              {
                QColor c;
                switch(colorMode)
                {
                  default:
                  case 0:
                    c = MusEGlobal::config.partColors[curPart->colorIndex()];
                    break;
                  case 1:
                    c = Qt::lightGray;
                    break;
                }
                c.setAlpha(MusEGlobal::config.globalAlphaBlend);
                QBrush part_bg_brush(MusECore::gGradientFromQColor(c, mwpr.topLeft(), mwpr.bottomLeft()));
                p.fillRect(mpbgr, part_bg_brush);
              }
        }     
      }
      else
      {
        // Draw non-current parts:
        for (MusECore::iPart ip = editor->parts()->begin(); ip != editor->parts()->end(); ++ip) 
        {
              MusECore::WavePart* wp = (MusECore::WavePart*)(ip->second);
              if(wp == curPart)
                continue;
              
              QRect mwpr  = map(QRect(wp->frame(), 0, wp->lenFrame(), height()));
              QRect mpbgr = rr & mwpr;
              if(!mpbgr.isNull())
              {
                //int cidx = wp->colorIndex();
                //QColor c(MusEGlobal::config.partColors[cidx]);
                QColor c(MusEGlobal::config.waveNonselectedPart);
                c.setAlpha(MusEGlobal::config.globalAlphaBlend);
                QBrush part_bg_brush(MusECore::gGradientFromQColor(c, mwpr.topLeft(), mwpr.bottomLeft()));
                p.fillRect(mpbgr, part_bg_brush);
              }
        }     
      }
      
      p.setWorldMatrixEnabled(wmtxen);
}
      
// TODO: Overridden because we're in units of frames. 
//       After BBT/frame mode is added to Canvas, remove this override and let View or Canvas do it.
//---------------------------------------------------------
//   drawTickRaster
//---------------------------------------------------------

void WaveCanvas::drawTickRaster(QPainter& p, int x, int y, int w, int h, int raster)
      {
      // Changed to draw in device coordinate space instead of virtual, transformed space. Tim. 
      
      //int mx = mapx(x);
      int my = mapy(y);
      //int mw = mapx(x + w) - mx;
      //int mw = mapx(x + w) - mx - 1;
      //int mh = mapy(y + h) - my;
      //int mh = mapy(y + h) - my - 1;
      
      //p.save();
      bool wmtxen = p.worldMatrixEnabled();
      p.setWorldMatrixEnabled(false);
      
      int xx,bar1, bar2, beat;
      unsigned tick;
//      AL::sigmap.tickValues(x, &bar1, &beat, &tick);
//      AL::sigmap.tickValues(x+w, &bar2, &beat, &tick);
      AL::sigmap.tickValues(MusEGlobal::tempomap.frame2tick(x), &bar1, &beat, &tick);
      AL::sigmap.tickValues(MusEGlobal::tempomap.frame2tick(x+w), &bar2, &beat, &tick);
      ++bar2;
      ///int y2 = y + h;
      //int y2 = my + mh;
      int y2 = mapy(y + h) - 1;
      //printf("View::drawTickRaster x:%d y:%d w:%d h:%d mx:%d my:%d mw:%d mh:%d y2:%d bar1:%d bar2:%d\n", x, y, w, h, mx, my, mw, mh, y2, bar1, bar2);  
      //printf("View::drawTickRaster x:%d y:%d w:%d h:%d my:%d mh:%d y2:%d bar1:%d bar2:%d\n", x, y, w, h, my, mh, y2, bar1, bar2);  
      for (int bar = bar1; bar < bar2; ++bar) {
        
//            unsigned xb = AL::sigmap.bar2tick(bar, 0, 0);
            unsigned xb = AL::sigmap.bar2tick(bar, 0, 0);
            int xt = mapx(MusEGlobal::tempomap.tick2frame(xb));
            p.setPen(Qt::black);
            p.drawLine(xt, my, xt, y2);
            
            int z, n;
            AL::sigmap.timesig(xb, z, n);
            int qq = raster;
            if (rmapx(raster) < 8)        // grid too dense
                  qq *= 2;
            p.setPen(Qt::lightGray);
            if (raster>=4) {
                        xx = xb + qq;
                        int xxx = MusEGlobal::tempomap.tick2frame(AL::sigmap.bar2tick(bar, z, 0));
                        //while (MusEGlobal::tempomap.tick2frame(xx) <= xxx) {
                        while (1) {
                               int xxf = MusEGlobal::tempomap.tick2frame(xx);
                               if(xxf > xxx)
                                 break;
                               //int x = mapx(MusEGlobal::tempomap.tick2frame(xx));
                               int x = mapx(xxf);
                               p.drawLine(x, my, x, y2);
                               xx += qq;
                               }
                        }
            p.setPen(Qt::gray);
            for (int beat = 1; beat < z; beat++) {
                        xx = mapx(MusEGlobal::tempomap.tick2frame(AL::sigmap.bar2tick(bar, beat, 0)));
                        //printf(" bar:%d z:%d beat:%d xx:%d\n", bar, z, beat, xx);  
                        p.drawLine(xx, my, xx, y2);
                        }

            }
      p.setWorldMatrixEnabled(wmtxen);
      }

// TODO: Overridden because we're in units of frames. 
//       After BBT/frame mode is added to Canvas, remove this override and let Canvas do it.
//---------------------------------------------------------
//   raster
//---------------------------------------------------------

QPoint WaveCanvas::raster(const QPoint& p) const
      {
      int x = p.x();
      if (x < 0)
            x = 0;
      //x = editor->rasterVal(x);
      x = MusEGlobal::tempomap.tick2frame(editor->rasterVal(MusEGlobal::tempomap.frame2xtick(x)));
      int pitch = y2pitch(p.y());
      int y = pitch2y(pitch);
      return QPoint(x, y);
      }

#define WHEEL_STEPSIZE 40
#define WHEEL_DELTA   120
//---------------------------------------------------------
//   wheelEvent
//---------------------------------------------------------
void WaveCanvas::wheelEvent(QWheelEvent* ev)
{
  int keyState = ev->modifiers();

  bool shift      = keyState & Qt::ShiftModifier;
  bool ctrl       = keyState & Qt::ControlModifier;

  if (shift) { // scroll horizontally
      int delta       = -ev->delta() / WHEEL_DELTA;
      int xpixelscale = 5*MusECore::fast_log10(rmapxDev(1));
      if (xpixelscale <= 0)
            xpixelscale = 1;
      int scrollstep = WHEEL_STEPSIZE * (delta);
      scrollstep = scrollstep / 10;
      int newXpos = xpos + xpixelscale * scrollstep;
      if (newXpos < 0)
            newXpos = 0;
      emit horizontalScroll((unsigned)newXpos);
  } else if (ctrl) {  // zoom horizontally
      emit horizontalZoom(ev->delta()>0, ev->globalPos());
  } else { // scroll horizontally
      emit mouseWheelMoved(ev->delta() / 10);
  }
}

//---------------------------------------------------------
//   viewMousePressEvent
//---------------------------------------------------------

bool WaveCanvas::mousePress(QMouseEvent* event)
      {
    if (event->modifiers() & Qt::ControlModifier) {
            return true;
            }
      button = event->button();
      QPoint pt = event->pos();
      //CItem* item = items.find(pt);
      unsigned x = event->x();

      switch (_tool) {
            default:
                  break;
             case RangeTool:
                  switch (button) {
                        case Qt::LeftButton:
                              if (mode == NORMAL) {
                                    // redraw and reset:
                                    if (!selectionStart.invalid) {
                                          selectionStart = selectionStop = XTick::createInvalidXTick();
                                          redraw();
                                          }
                                    mode = DRAG;
                                    dragstartx = MusEGlobal::tempomap.frame2xtick(x);
                                    selectionStart = selectionStop = dragstartx;
                                    drag = DRAG_LASSO_START;
                                    Canvas::start = pt;
                                    return false;
                                    }
                              break;

                        case Qt::MidButton:
                        case Qt::RightButton:
                        default:
                              break;
                        }

                   break;
            }
      return true;
      }

//---------------------------------------------------------
//   viewMouseReleaseEvent
//---------------------------------------------------------

void WaveCanvas::mouseRelease(const QPoint&)
      {
      button = Qt::NoButton;
      if (mode == DRAG) {
            mode = NORMAL;
            }
      }

//---------------------------------------------------------
//   viewMousevent
//---------------------------------------------------------

void WaveCanvas::mouseMove(QMouseEvent* event)
      {
      int x = event->x();
      if (x < 0)
            x = 0;
      emit timeChanged(x);
      //emit timeChanged(editor->rasterVal(x));
      //emit timeChanged(AL::sigmap.raster(x, *_raster));

      XTick x_xtick = MusEGlobal::tempomap.frame2xtick(x);
      
      switch (button) {
            case Qt::LeftButton:
                  if (mode == DRAG) {
                        int mx      = mapx(x);
                        int mstart  = mapx(MusEGlobal::tempomap.tick2frame(selectionStart));
                        int mstop   = mapx(MusEGlobal::tempomap.tick2frame(selectionStop));
                        QRect r(0, 0, 0, height());
                        
                        if (x_xtick < dragstartx) {
                              if(x_xtick < selectionStart)
                              {
                                r.setLeft(mx);
                                r.setWidth((selectionStop >= dragstartx ? mstop : mstart) - mx);
                              }
                              else
                              {
                                r.setLeft(mstart);
                                r.setWidth(mx - mstart);
                              }
                              selectionStart = x_xtick;
                              selectionStop = dragstartx;
                              }
                        else {
                              if(x_xtick >= selectionStop)
                              {
                                r.setLeft(selectionStart < dragstartx ? mstart : mstop);
                                r.setWidth(mx - (selectionStart < dragstartx ? mstart : mstop));
                              }
                              else
                              {
                                r.setLeft(mx);
                                r.setWidth(mstop - mx);
                              }
                              selectionStart = dragstartx;
                              selectionStop = x_xtick;
                              }
                        update(r);
                        }
                  break;
            case Qt::MidButton:
                  break;
            case Qt::RightButton:
                  break;
            default:
                  return;
            }
      }
      
//---------------------------------------------------------
//   pitch2y
//---------------------------------------------------------

int WaveCanvas::pitch2y(int) const
      {
      return 0;
      }

//---------------------------------------------------------
//   y2pitch
//---------------------------------------------------------

int WaveCanvas::y2pitch(int) const
      {
      return 0;
      }

//---------------------------------------------------------
//   drawItem
//    draws a wave
//---------------------------------------------------------

void WaveCanvas::drawItem(QPainter& p, const MusEGui::CItem* item, const QRect& rect)
{
      MusECore::WavePart* wp = (MusECore::WavePart*)(item->part());
      if(!wp || !wp->track())
        return;

      //QRect rr = p.transform().mapRect(rect);  // Gives inconsistent positions. Source shows wrong operation for our needs.
      QRect rr = map(rect);                      // Use our own map instead.        

      QRect mwpr  = map(QRect(wp->frame(), 0, wp->lenFrame(), height()));
      
      QRect r = item->bbox();
      QRect mer = map(r);                              
      QRect mr = rr & mer & mwpr;
      if(mr.isNull())
        return;
      
      MusECore::Event event  = item->event();
      if(event.empty())
        return;
      
      int x1 = mr.x();
      int x2 = x1 + mr.width();
      if (x1 < 0)
            x1 = 0;
      if (x2 > width())
            x2 = width();
      int hh = height();
      int y1 = mr.y();
      int y2 = y1 + mr.height();

      int xScale = xmag;
      if (xScale < 0)
            xScale = -xScale;

      //int t_channels = wp->track()->channels();
      int px = wp->frame();

      bool wmtxen = p.worldMatrixEnabled();
      p.setWorldMatrixEnabled(false);

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
      
      QBrush brush;
      if (item->isMoving()) 
      {
            QColor c(Qt::gray);
            c.setAlpha(MusEGlobal::config.globalAlphaBlend);
            QLinearGradient gradient(r.topLeft(), r.bottomLeft());
            gradient.setColorAt(0, c);
            gradient.setColorAt(1, c.darker());
            brush = QBrush(gradient);
            p.fillRect(sx, y1, ex - sx + 1, y2, brush);
      }
      else 
      if (item->isSelected()) 
      {
          QColor c(Qt::black);
          c.setAlpha(MusEGlobal::config.globalAlphaBlend);
          QLinearGradient gradient(r.topLeft(), r.bottomLeft());
          // Use a colour only about 20% lighter than black, rather than the 50% we use in MusECore::gGradientFromQColor
          //  and is used in darker()/lighter(), so that it is distinguished a bit better from grey non-part tracks.
          gradient.setColorAt(0, QColor(51, 51, 51, MusEGlobal::config.globalAlphaBlend));
          gradient.setColorAt(1, c);
          brush = QBrush(gradient);
          p.fillRect(sx, y1, ex - sx + 1, y2, brush);
      }

      const MusECore::AudioStream* stream = event.getAudioStream();
      if(stream)
      {
        int ev_channels = stream->get_n_input_channels();
        if (ev_channels == 0) {
              p.setWorldMatrixEnabled(wmtxen);
              printf("WaveCanvas::drawItem: ev_channels==0! %s\n", event.audioFilePath().toLatin1().constData());
              return;
              }

        int h   = hh / (ev_channels * 2);
        int cc  = hh % (ev_channels * 2) ? 0 : 1;

        unsigned peoffset = px + event.frame() - event.spos();

        for (int i = sx; i < ex; i++) {
              int y = h;
              MusECore::SampleV sa[ev_channels];
              stream->readPeakRms(sa, xScale, pos);
              pos += xScale;
              if (pos < event.spos())
                    continue;

              int selectionStartPos = MusEGlobal::tempomap.tick2frame(selectionStart) - peoffset; // Offset transformed to event coords
              int selectionStopPos  = MusEGlobal::tempomap.tick2frame(selectionStop)  - peoffset;

              for (int k = 0; k < ev_channels; ++k) {
                    int peak = (sa[k].peak * (h - 1)) / yScale;
                    int rms  = (sa[k].rms  * (h - 1)) / yScale;
                    if (peak > h)
                          peak = h;
                    if (rms > h)
                          rms = h;
                    QColor peak_color = MusEGlobal::config.wavePeakColor;
                    QColor rms_color  = MusEGlobal::config.waveRmsColor;

                    if (pos > selectionStartPos && pos < selectionStopPos) {
                          peak_color = MusEGlobal::config.wavePeakColorSelected;
                          rms_color  = MusEGlobal::config.waveRmsColorSelected;
                          QLine l_inv = clipQLine(i, y - h + cc, i, y + h - cc, mr);
                          if(!l_inv.isNull())
                          {
                            // Draw inverted
                            p.setPen(QColor(Qt::black));
                            p.drawLine(l_inv);
                          }
                        }

                    QLine l_peak = clipQLine(i, y - peak - cc, i, y + peak, mr);
                    if(!l_peak.isNull())
                    {
                      p.setPen(peak_color);
                      p.drawLine(l_peak);
                    }

                    QLine l_rms = clipQLine(i, y - rms - cc, i, y + rms, mr);
                    if(!l_rms.isNull())
                    {
                      p.setPen(rms_color);
                      p.drawLine(l_rms);
                    }

                    y += 2 * h;
                  }
              }
            
        int hn = hh / ev_channels;
        int hhn = hn / 2;
        for (int i = 0; i < ev_channels; ++i) {
              int h2     = hn * i;
              int center = hhn + h2;
              if(center >= y1 && center < y2)
              {
                p.setPen(QColor(i & 1 ? Qt::red : Qt::blue));
                p.drawLine(sx, center, ex, center);
              }
              if(i != 0 && h2 >= y1 && h2 < y2)
              {
                p.setPen(QColor(Qt::black));
                p.drawLine(sx, h2, ex, h2);
              }
            }
      }

      //      
      // Draw custom dashed borders around the wave event
      //

      QColor color(item->isSelected() ? Qt::white : Qt::black);
      QPen penH(color);
      QPen penV(color);
      penH.setCosmetic(true);
      penV.setCosmetic(true);
      QVector<qreal> customDashPattern;
      customDashPattern << 4.0 << 6.0;
      penH.setDashPattern(customDashPattern);
      penV.setDashPattern(customDashPattern);
      penV.setDashOffset(2.0);
      // FIXME: Some shifting still going on. Values likely not quite right here.
      //int xdiff = sx - r.x();
      int xdiff = sx - mer.x();
      if(xdiff > 0)
      {
        int doff = xdiff % 10;
        penH.setDashOffset(doff);
      }
      // Tested OK. Each segment drawn only when necessary.
      if(y1 <= 0)
      {
        p.setPen(penH);
        p.drawLine(sx, 0, ex, 0);
      }
      if(y2 >= hh - 1)
      {
        p.setPen(penH);
        p.drawLine(sx, hh - 1, ex, hh - 1);
      }
      if(x1 <= mer.x())
      {
        p.setPen(penV);
        p.drawLine(mer.x(), y1, mer.x(), y2);
      }
      if(x2 >= mer.x() + mer.width())
      {
        p.setPen(penV);
        p.drawLine(mer.x() + mer.width(), y1, mer.x() + mer.width(), y2);
      }

      // Done. Restore and return.
      p.setWorldMatrixEnabled(wmtxen);
}

//---------------------------------------------------------
//   drawTopItem
//---------------------------------------------------------
void WaveCanvas::drawTopItem(QPainter& , const QRect&)
{}

//---------------------------------------------------------
//   drawMoving
//    draws moving items
//---------------------------------------------------------

void WaveCanvas::drawMoving(QPainter& p, const MusEGui::CItem* item, const QRect& rect)
    {
      QRect mr = QRect(item->mp().x(), item->mp().y(), item->width(), item->height());
      mr = mr.intersected(rect);
      if(!mr.isValid())
        return;
      p.setPen(Qt::black);
      p.setBrush(QColor(0, 128, 0, 128));  // TODO: Pick a better colour, or use part colours, or grey?
      p.drawRect(mr);
    }

//---------------------------------------------------------
//   viewMouseDoubleClickEvent
//---------------------------------------------------------

void WaveCanvas::viewMouseDoubleClickEvent(QMouseEvent* event)
      {
      if ((_tool != MusEGui::PointerTool) && (event->button() != Qt::LeftButton)) {
            mousePress(event);
            return;
            }
      }

//---------------------------------------------------------
//   moveCanvasItems
//---------------------------------------------------------

MusECore::Undo WaveCanvas::moveCanvasItems(MusEGui::CItemList& items, int /*dp*/, int dx, DragType dtype, bool rasterize)
{      
  if(editor->parts()->empty())
    return MusECore::Undo(); //return empty list
  
  MusECore::PartsToChangeMap parts2change;
  MusECore::Undo operations;  
  
  for(MusECore::iPart ip = editor->parts()->begin(); ip != editor->parts()->end(); ++ip)
  {
    MusECore::Part* part = ip->second;
    if(!part)
      continue;
    
    int npartoffset = 0;
    for(MusEGui::iCItem ici = items.begin(); ici != items.end(); ++ici) 
    {
      MusEGui::CItem* ci = ici->second;
      if(ci->part() != part)
        continue;
      
      int x = ci->pos().x() + dx;
      //int y = pitch2y(y2pitch(ci->pos().y()) + dp);
      int y = 0;
      QPoint newpos = QPoint(x, y);
      if(rasterize)
        newpos = raster(newpos);
      
      // Test moving the item...
      WEvent* wevent = (WEvent*) ci;
      MusECore::Event event    = wevent->event();
      x              = newpos.x();
      if(x < 0)
        x = 0;
      int nframe = (rasterize ? MusEGlobal::tempomap.tick2frame(editor->rasterVal(MusEGlobal::tempomap.frame2xtick(x))) : x) - part->frame();
      if(nframe < 0)
        nframe = 0;
      int diff = nframe + event.lenFrame() - part->lenFrame();
      
      // If moving the item would require a new part size...
      if(diff > npartoffset)
        npartoffset = diff;
    }
        
    if(npartoffset > 0)
    {    
      MusECore::iPartToChange ip2c = parts2change.find(part);
      if(ip2c == parts2change.end())
      {
        MusECore::PartToChange p2c = {0, npartoffset};
        parts2change.insert(std::pair<MusECore::Part*, MusECore::PartToChange> (part, p2c));
      }
      else
        ip2c->second.xdiff = npartoffset;
    }
  }
  
  bool forbidden=false;
  for(MusECore::iPartToChange ip2c = parts2change.begin(); ip2c != parts2change.end(); ++ip2c)
  {
    MusECore::Part* opart = ip2c->first;
    if (opart->hasHiddenEvents())
    {
                        forbidden=true;
                        break;
                }
  }    

        
        if (!forbidden)
        {
                std::vector< MusEGui::CItem* > doneList;
                typedef std::vector< MusEGui::CItem* >::iterator iDoneList;
                
                for(MusEGui::iCItem ici = items.begin(); ici != items.end(); ++ici) 
                {
                        MusEGui::CItem* ci = ici->second;
                        
                        int x = ci->pos().x();
                        //int y = ci->pos().y();
                        int nx = x + dx;
                        //int ny = pitch2y(y2pitch(y) + dp);
                        int ny = 0;
                        QPoint newpos = QPoint(nx, ny);
                        if(rasterize)
                          newpos = raster(newpos);
                        selectItem(ci, true);
                        
                        iDoneList idl;
                        for(idl = doneList.begin(); idl != doneList.end(); ++idl)
                                // This compares EventBase pointers to see if they're the same...
                                if((*idl)->event() == ci->event())
                                        break;
                                
                        // Do not process if the event has already been processed (meaning it's an event in a clone part)...
                        if (idl == doneList.end())
                        {
                                moveItem(operations, ci, newpos, dtype, rasterize); // always returns true. if not, change is necessary here!
                                doneList.push_back(ci);
                        }
                        ci->move(newpos);
                                                
                        if(moving.size() == 1) 
                                                itemReleased(curItem, newpos);

                        if(dtype == MOVE_COPY || dtype == MOVE_CLONE)
                                                selectItem(ci, false);
                }  

    for(MusECore::iPartToChange ip2c = parts2change.begin(); ip2c != parts2change.end(); ++ip2c)
    {
      MusECore::Part* opart = ip2c->first;
      int diff = ip2c->second.xdiff;
      
      //schedule_resize_all_same_len_clone_parts(opart, opart->lenTick() + diff, operations);
      schedule_resize_all_same_len_clone_parts(opart, opart->lenFrame() + diff, MusECore::Pos::FRAMES, operations);
    }    
                                        
        return operations;
  }
  else
  {
                return MusECore::Undo(); //return empty list
        }
}
      
//---------------------------------------------------------
//   moveItem
//    called after moving an object
//---------------------------------------------------------

bool WaveCanvas::moveItem(MusECore::Undo& operations, MusEGui::CItem* item, const QPoint& pos, DragType dtype, bool rasterize)
      {
      WEvent* wevent = (WEvent*) item;
      MusECore::Event event    = wevent->event();
      //int npitch     = y2pitch(pos.y());
      MusECore::Event newEvent = event.clone();
      int x          = pos.x();
      if (x < 0)
            x = 0;
      
      MusECore::Part* part = wevent->part();
      int nframe = (rasterize ? MusEGlobal::tempomap.tick2frame(editor->rasterVal(MusEGlobal::tempomap.frame2xtick(x))) : x) - part->frame();
      if (nframe < 0)
            nframe = 0;
      newEvent.setFrame(nframe);
      newEvent.setLenFrame(event.lenFrame());

      // don't check, whether the new event is within the part
      // at this place. with operation groups, the part isn't
      // resized yet. (flo93)
      
      if (dtype == MOVE_COPY || dtype == MOVE_CLONE)
            operations.push_back(MusECore::UndoOp(MusECore::UndoOp::AddEvent, newEvent, part, false, false));
      else
            operations.push_back(MusECore::UndoOp(MusECore::UndoOp::ModifyEvent, newEvent, event, part, false, false));
      
      return true;
}

//---------------------------------------------------------
//   newItem(p, state)
//---------------------------------------------------------

MusEGui::CItem* WaveCanvas::newItem(const QPoint& p, int key_modifiers)
      {
      int frame  = p.x();
      if(!(key_modifiers & Qt::ShiftModifier))
        frame = MusEGlobal::tempomap.tick2frame(editor->rasterVal1(MusEGlobal::tempomap.frame2xtick(frame)));
      int len   = p.x() - frame;
      frame     -= curPart->frame();
      if (frame < 0)
            return 0;
      MusECore::Event e =  MusECore::Event(MusECore::Wave);
      e.setFrame(frame);
      e.setLenFrame(len);
      WEvent* we = new WEvent(e, curPart, height());
      return we;
      }

void WaveCanvas::newItem(MusEGui::CItem* item, bool noSnap)
      {
      WEvent* wevent = (WEvent*) item;
      MusECore::Event event    = wevent->event();
      MusECore::Part* part = wevent->part();
      int pframe = part->frame();
      int x = item->x();
      if (x<pframe)
            x=pframe;
      int w = item->width();

      if (!noSnap) {
            //x = editor->rasterVal1(x); //round down
            x = MusEGlobal::tempomap.tick2frame(editor->rasterVal1(MusEGlobal::tempomap.frame2xtick(x))); //round down
            //w = editor->rasterVal(x + w) - x;
            w = MusEGlobal::tempomap.tick2frame(editor->rasterVal(MusEGlobal::tempomap.frame2xtick(x + w))) - x;
            if (w == 0)
                  //w = editor->raster();
                  w = MusEGlobal::tempomap.tick2frame(editor->raster());
            }
      if (x<pframe)
            x=pframe;
      event.setFrame(x - pframe);
      event.setLenFrame(w);

      MusECore::Undo operations;
      int diff = event.endFrame() - part->lenFrame();
      
      if (! ((diff > 0) && part->hasHiddenEvents()) ) //operation is allowed
      {
        operations.push_back(MusECore::UndoOp(MusECore::UndoOp::AddEvent,event, part, false, false));
        
        if (diff > 0)// part must be extended?
        {
              //schedule_resize_all_same_len_clone_parts(part, event.endTick(), operations);
              schedule_resize_all_same_len_clone_parts(part, event.endFrame(), MusECore::Pos::FRAMES, operations);
              printf("newItem: extending\n");
        }
        
        MusEGlobal::song->applyOperationGroup(operations);
      }
      else // forbid action by not applying it   
          songChanged(SC_EVENT_INSERTED); //this forces an update of the itemlist, which is neccessary
                                          //to remove "forbidden" events from the list again
      }

//---------------------------------------------------------
//   resizeItem
//---------------------------------------------------------

void WaveCanvas::resizeItem(MusEGui::CItem* item, bool noSnap, bool)         // experimental changes to try dynamically extending parts
      {
      WEvent* wevent = (WEvent*) item;
      MusECore::Event event    = wevent->event();
      MusECore::Event newEvent = event.clone();
      int len;

      MusECore::Part* part = wevent->part();

      if (noSnap)
            len = wevent->width();
      else {
            unsigned frame = event.frame() + part->frame();
            //len = editor->rasterVal(tick + wevent->width()) - tick;
            len = MusEGlobal::tempomap.tick2frame(editor->rasterVal(MusEGlobal::tempomap.frame2xtick(frame + wevent->width()))) - frame;
            if (len <= 0)
                  //len = editor->raster();
                  len = MusEGlobal::tempomap.tick2frame(editor->raster());
      }

      MusECore::Undo operations;
      //int diff = event.tick()+len-part->lenTick();
      int diff = event.frame() + len - part->lenFrame();
      
      if (! ((diff > 0) && part->hasHiddenEvents()) ) //operation is allowed
      {
        //newEvent.setLenTick(len);
        newEvent.setLenFrame(len);
        operations.push_back(MusECore::UndoOp(MusECore::UndoOp::ModifyEvent,newEvent, event, wevent->part(), false, false));
        
        if (diff > 0)// part must be extended?
        {
              //schedule_resize_all_same_len_clone_parts(part, event.tick()+len, operations);
              schedule_resize_all_same_len_clone_parts(part, event.frame() + len, MusECore::Pos::FRAMES, operations);
              printf("resizeItem: extending\n");
        }
      }
      //else forbid action by not performing it
      MusEGlobal::song->applyOperationGroup(operations);
      songChanged(SC_EVENT_MODIFIED); //this forces an update of the itemlist, which is neccessary
                                      //to remove "forbidden" events from the list again
      }

//---------------------------------------------------------
//   deleteItem
//---------------------------------------------------------

bool WaveCanvas::deleteItem(MusEGui::CItem* item)
      {
      WEvent* wevent = (WEvent*) item;
      if (wevent->part() == curPart) {
            MusECore::Event ev = wevent->event();
            // Indicate do undo, and do not do port controller values and clone parts. 
            MusEGlobal::audio->msgDeleteEvent(ev, curPart, true, false, false);
            return true;
            }
      return false;
      }

//---------------------------------------------------------
//   adjustWaveOffset
//---------------------------------------------------------

void WaveCanvas::adjustWaveOffset()
{
  bool have_selected = false;
  int init_offset = 0;
  
  for (MusEGui::iCItem k = items.begin(); k != items.end(); ++k) 
  {
    if (k->second->isSelected())
    {
      have_selected = true;
      init_offset = k->second->event().spos();
      break;
    }
  }

  if(!have_selected)
  {
    QMessageBox::information(this, 
        QString("MusE"),
        QWidget::tr("No wave events selected."));
    return;
  }

  bool ok = false;
  int offset = QInputDialog::getInt(this, 
                                    tr("Adjust Wave Offset"), 
                                    tr("Wave offset (frames)"), 
                                    init_offset, 
                                    0, 2147483647, 1, 
                                    &ok);
  if(!ok)
    return;    
  
  MusECore::Undo operations;

  // FIXME: Respect clones! If operating on two selected clones of the same part, an extra event is created!
  //        Check - Is it really this code's problem? Seems so, other operations like moving an event seem OK.
  for(MusEGui::iCItem ici = items.begin(); ici != items.end(); ++ici) 
  {
    if(ici->second->isSelected())
    {
      MusECore::Event oldEvent = ici->second->event();
      if(oldEvent.spos() != offset)
      {
        MusECore::Part* part = ici->second->part();
        MusECore::Event newEvent = oldEvent.clone();
        newEvent.setSpos(offset);
        // Do not do port controller values and clone parts. 
        operations.push_back(MusECore::UndoOp(MusECore::UndoOp::ModifyEvent, newEvent, oldEvent, part, false, false));
      }
    }
  }
  
  MusEGlobal::song->applyOperationGroup(operations);
  
  redraw();
}
      
//---------------------------------------------------------
//   draw
//---------------------------------------------------------

void WaveCanvas::drawCanvas(QPainter& p, const QRect& rect)
      {
      int x = rect.x();
      int y = rect.y();
      int w = rect.width();
      int h = rect.height();

      //---------------------------------------------------
      // vertical lines
      //---------------------------------------------------

      drawTickRaster(p, x, y, w, h, editor->raster());
      }

//---------------------------------------------------------
//   waveCmd
//---------------------------------------------------------

void WaveCanvas::waveCmd(int cmd)
      {
      // TODO FINDMICH: New WaveCanvas: Convert this routine to frames.  
      switch(cmd) {
            case CMD_LEFT:
                  {
                  int spos = pos[0];
                  if(spos > 0) 
                  {
                    spos -= 1;     // Nudge by -1, then snap down with raster1.
                    spos = AL::sigmap.raster1(spos, editor->rasterStep(pos[0]).to_uint());
                  }  
                  if(spos < 0)
                    spos = 0;
                  MusEGlobal::song->setPos(0, MusECore::Pos(MusECore::XTick(spos)), true, true, true);
                  }
                  break;
            case CMD_RIGHT:
                  {
                  int spos = AL::sigmap.raster2(pos[0] + 1, editor->rasterStep(pos[0]).to_uint());    // Nudge by +1, then snap up with raster2.
                  MusEGlobal::song->setPos(0, MusECore::Pos(MusECore::XTick(spos)), true, true, true); 
                  }
                  break;
            case CMD_LEFT_NOSNAP:
                  {
                  int spos = pos[0] - editor->rasterStep(pos[0]).to_uint();
                  if (spos < 0)
                        spos = 0;
                  MusEGlobal::song->setPos(0, MusECore::Pos(MusECore::XTick(spos)), true, true, true); //CDW
                  }
                  break;
            case CMD_RIGHT_NOSNAP:
                  {
                  int spos = pos[0] + editor->rasterStep(pos[0]).to_uint();
                  MusEGlobal::song->setPos(0, MusECore::Pos(MusECore::XTick(spos)), true, true, true); //CDW
                  }
                  break;
            case CMD_INSERT:
                  {
                  if (pos[0] < start() || pos[0] >= end())
                        break;
                  MusECore::MidiPart* part = (MusECore::MidiPart*)curPart;

                  if (part == 0)
                        break;

                  MusECore::EventList* el = part->events();
                  MusECore::Undo operations;

                  std::list <MusECore::Event> elist;
                  for (MusECore::iEvent e = el->lower_bound(pos[0] - part->tick()); e != el->end(); ++e)
                        elist.push_back((MusECore::Event)e->second);
                  for (std::list<MusECore::Event>::iterator i = elist.begin(); i != elist.end(); ++i) {
                        MusECore::Event event = *i;
                        MusECore::Event newEvent = event.clone();
                        newEvent.setTick(event.tick() + editor->raster());// - part->tick()); DELETETHIS
                        // Do not do port controller values and clone parts. 
                        operations.push_back(MusECore::UndoOp(MusECore::UndoOp::ModifyEvent, newEvent, event, part, false, false));
                        }
                  MusEGlobal::song->applyOperationGroup(operations);
                  
                  int spos = editor->rasterVal(pos[0] + editor->rasterStep(pos[0]).to_uint()).to_uint();
                  MusEGlobal::song->setPos(0, MusECore::Pos(MusECore::XTick(spos)), true, false, true);
                  }
                  return;
            case CMD_BACKSPACE:
                  if (pos[0] < start() || pos[0] >= end())
                        break;
                  {
                  MusECore::MidiPart* part = (MusECore::MidiPart*)curPart;
                  if (part == 0)
                        break;
                  
                  MusECore::Undo operations;
                  MusECore::EventList* el = part->events();

                  std::list<MusECore::Event> elist;
                  for (MusECore::iEvent e = el->lower_bound(pos[0]); e != el->end(); ++e)
                        elist.push_back((MusECore::Event)e->second);
                  for (std::list<MusECore::Event>::iterator i = elist.begin(); i != elist.end(); ++i) {
                        MusECore::Event event = *i;
                        MusECore::Event newEvent = event.clone();
                        newEvent.setTick(event.tick() - editor->raster() - part->tick());
                        // Do not do port controller values and clone parts. 
                        operations.push_back(MusECore::UndoOp(MusECore::UndoOp::ModifyEvent, newEvent, event, part, false, false));
                        }
                  MusEGlobal::song->applyOperationGroup(operations);
                  int spos=editor->rasterVal(pos[0] - editor->rasterStep(pos[0]).to_uint()).to_uint();
                  MusEGlobal::song->setPos(0, MusECore::Pos(MusECore::XTick(spos)), true, false, true);
                  }
                  break;
            }
      }

//---------------------------------------------------------
//   cmd
//    pulldown menu commands
//---------------------------------------------------------

void WaveCanvas::cmd(int cmd)
      {
      int modifyoperation = -1;
      double paramA = 0.0;
      switch (cmd) {
            case CMD_SELECT_ALL:     // select all
                  if (tool() == MusEGui::RangeTool) 
                  {
                    if (!editor->parts()->empty()) {
                          MusECore::iPart iBeg = editor->parts()->begin();
                          MusECore::iPart iEnd = editor->parts()->end();
                          iEnd--;
                          MusECore::WavePart* beg = (MusECore::WavePart*) iBeg->second;
                          MusECore::WavePart* end = (MusECore::WavePart*) iEnd->second;
                          selectionStart = beg->xtick();
                          selectionStop  = end->endXTick();
                          redraw();
                          }
                  }
                  for (MusEGui::iCItem k = items.begin(); k != items.end(); ++k) {
                        if (!k->second->isSelected())
                              selectItem(k->second, true);
                        }
                  break;
            case CMD_SELECT_NONE:     // select none
                  selectionStart = selectionStop = XTick::createInvalidXTick();
                  deselectAll();
                  break;
            case CMD_SELECT_INVERT:     // invert selection
                  for (MusEGui::iCItem k = items.begin(); k != items.end(); ++k) {
                        selectItem(k->second, !k->second->isSelected());
                        }
                  break;
            case CMD_SELECT_ILOOP:     // select inside loop
                  for (MusEGui::iCItem k = items.begin(); k != items.end(); ++k) {
                        WEvent* wevent = (WEvent*)(k->second);
                        MusECore::Part* part     = wevent->part();
                        MusECore::Event event    = wevent->event();
                        unsigned tick  = event.tick() + part->tick();
                        if (tick < MusEGlobal::song->lpos() || tick >= MusEGlobal::song->rpos())
                              selectItem(k->second, false);
                        else
                              selectItem(k->second, true);
                        }
                  break;
            case CMD_SELECT_OLOOP:     // select outside loop
                  for (MusEGui::iCItem k = items.begin(); k != items.end(); ++k) {
                        WEvent* wevent = (WEvent*)(k->second);
                        MusECore::Part* part     = wevent->part();
                        MusECore::Event event    = wevent->event();
                        unsigned tick  = event.tick() + part->tick();
                        if (tick < MusEGlobal::song->lpos() || tick >= MusEGlobal::song->rpos())
                              selectItem(k->second, true);
                        else
                              selectItem(k->second, false);
                        }
                  break;
            case CMD_SELECT_PREV_PART:     // select previous part
                  {
                    MusECore::Part* pt = editor->curCanvasPart();
                    MusECore::Part* newpt = pt;
                    MusECore::PartList* pl = editor->parts();
                    for(MusECore::iPart ip = pl->begin(); ip != pl->end(); ++ip)
                      if(ip->second == pt) 
                      {
                        if(ip == pl->begin())
                          ip = pl->end();
                        --ip;
                        newpt = ip->second;
                        break;    
                      }
                    if(newpt != pt)
                      editor->setCurCanvasPart(newpt);
                  }
                  break;
            case CMD_SELECT_NEXT_PART:     // select next part
                  {
                    MusECore::Part* pt = editor->curCanvasPart();
                    MusECore::Part* newpt = pt;
                    MusECore::PartList* pl = editor->parts();
                    for(MusECore::iPart ip = pl->begin(); ip != pl->end(); ++ip)
                      if(ip->second == pt) 
                      {
                        ++ip;
                        if(ip == pl->end())
                          ip = pl->begin();
                        newpt = ip->second;
                        break;    
                      }
                    if(newpt != pt)
                      editor->setCurCanvasPart(newpt);
                  }
                  break;
                 
            case CMD_ADJUST_WAVE_OFFSET:
                  adjustWaveOffset();
                  break;

            case CMD_EDIT_EXTERNAL:
                  modifyoperation = EDIT_EXTERNAL;
                  break;

            case CMD_EDIT_COPY:
                  modifyoperation = COPY;
                  break;
            case CMD_EDIT_CUT:
                  modifyoperation = CUT;
                  break;
            case CMD_EDIT_PASTE:
                  modifyoperation = PASTE;
                  break;

            case CMD_MUTE:
                  modifyoperation = MUTE;
                  break;

            case CMD_NORMALIZE:
                  modifyoperation = NORMALIZE;
                  break;

            case CMD_FADE_IN:
                  modifyoperation = FADE_IN;
                  break;

            case CMD_FADE_OUT:
                  modifyoperation = FADE_OUT;
                  break;

            case CMD_REVERSE:
                  modifyoperation = REVERSE;
                  break;

            case CMD_GAIN_FREE: {
                  EditGain* editGain = new EditGain(this, lastGainvalue);
                  if (editGain->exec() == QDialog::Accepted) {
                        lastGainvalue = editGain->getGain();
                        modifyoperation = GAIN;
                        paramA = (double)lastGainvalue / 100.0;
                        }
                  delete editGain;
                  }
                  break;

            case CMD_GAIN_200:
                  modifyoperation = GAIN;
                  paramA = 2.0;
                  break;

            case CMD_GAIN_150:
                  modifyoperation = GAIN;
                  paramA = 1.5;
                  break;

            case CMD_GAIN_75:
                  modifyoperation = GAIN;
                  paramA = 0.75;
                  break;

            case CMD_GAIN_50:
                  modifyoperation = GAIN;
                  paramA = 0.5;
                  break;

            case CMD_GAIN_25:
                  modifyoperation = GAIN;
                  paramA = 0.25;
                  break;

            case CMD_CREATE_PART_REGION:
                  {
                      // create a new part and put in the copy buffer
                      MusECore::Part* pt = editor->curCanvasPart();
                      if (pt == 0 || pt->track()->type() != MusECore::Track::WAVE)
                          return;
                      MusECore::WavePart *origPart = (MusECore::WavePart*)pt;
                      if (MusEGlobal::song->lpos() < origPart->tick() || MusEGlobal::song->rpos() > origPart->endTick())
                      {
                          QMessageBox::warning(this, tr("Part creation failed"),
                                       tr("Left and right position markers must be placed inside the current part."),
                                       QMessageBox::Ok, QMessageBox::Ok);
                          return;
                      }
                      MusECore::WavePart *tempPart = new MusECore::WavePart(origPart->track());
                      unsigned origFrame = origPart->frame();
                      unsigned frameDistance = MusEGlobal::song->lPos().frame() - origFrame;
                      tempPart->setPos(XTick(MusEGlobal::song->lpos()));
                      tempPart->setLenTick(MusEGlobal::song->rpos() - MusEGlobal::song->lpos());
                      // loop through the events and set them accordingly
                      for (MusECore::iEvent iWaveEvent = origPart->events()->begin(); iWaveEvent != origPart->events()->end(); iWaveEvent++)
                      {
                          // TODO: handle multiple events correctly,
                          // the math for subsequent events isn't correct
                          MusECore::Event &ev = iWaveEvent->second;
                          MusECore::Event *newEvent = new MusECore::Event(ev.clone());
                          newEvent->setSpos(ev.spos() + frameDistance);
                          newEvent->setLenTick(MusEGlobal::song->rpos() - MusEGlobal::song->lpos());
                          tempPart->addEvent(*newEvent);
                      }
                      std::set<MusECore::Part*> partList;
                      partList.insert(tempPart);

                      QMimeData *mimeData =  MusECore::parts_to_mime(partList);
                      QApplication::clipboard()->setMimeData(mimeData, QClipboard::Clipboard);
                      QMessageBox::information(this, tr("Part created"),
                                   tr("The selected region has been copied to the clipboard and can be pasted in the arranger."),
                                   QMessageBox::Ok, QMessageBox::Ok);
                  }
                  break;
            case CMD_ERASE_MEASURE:
            case CMD_DELETE_MEASURE:
            case CMD_CREATE_MEASURE:
                  break;
            default:
//                  printf("unknown ecanvas cmd %d\n", cmd);
                  break;
            }
            
      if (modifyoperation != -1) {
            if ((selectionStart.invalid || selectionStart == selectionStop) && modifyoperation!=PASTE) {
                  printf("No selection. Ignoring\n"); //@!TODO: Disable menu options when no selection
                  QMessageBox::information(this, 
                     QString("MusE"),
                     QWidget::tr("No selection. Ignoring"));

                  return;
                  }
            
            //if(!modifyWarnedYet)
            //{
            //  modifyWarnedYet = true;
            //  if(QMessageBox::warning(this, QString("Muse"),
            //     tr("Warning! Muse currently operates directly on the sound file.\n"
            //        "Undo is supported, but NOT after exit, WITH OR WITHOUT A SAVE!"), tr("&Ok"), tr("&Cancel"),
            //     QString::null, 0, 1 ) != 0)
            //   return;
            //}
            modifySelection(modifyoperation, selectionStart, selectionStop, paramA);
            }
            
      updateSelection();
      redraw();
      }

//---------------------------------------------------------
//   getSelection
//---------------------------------------------------------
MusECore::WaveSelectionList WaveCanvas::getSelection(XTick startpos, XTick stoppos)
{
	MusECore::WaveSelectionList selection;
	
	for (MusECore::iPart ip = editor->parts()->begin(); ip != editor->parts()->end(); ++ip) {
		MusECore::WavePart* wp = (MusECore::WavePart*)(ip->second);
		XTick part_offset = wp->xtick();
		
		MusECore::EventList* el = wp->events();
		
		for (MusECore::iEvent e = el->begin(); e != el->end(); ++e) {
			MusECore::Event event  = e->second;
			if (event.empty())
				continue;
			
			if (event.getAudioStream()==NULL) // TODO sollte passen
				continue;
			
			// Respect part end: Don't modify stuff outside of part boundary.
			XTick event_length = event.lenXTick();
			if(event.xtick() + event.lenXTick() >= wp->lenXTick())
			{
				// Adjust apparent operation length:
				if(event.xtick() > wp->lenXTick())
					continue; // ignore this event
				else
					               event_length = wp->lenXTick() - event.xtick();
			}
			
			XTick event_begin = event.xtick() + part_offset;
			XTick event_end    = event_begin + event_length;
			
			if (!(event_end <= startpos || event_begin > stoppos)) { // if event is affected by selection
				XTick temp_startpos = (startpos < event_begin) ? event_begin : startpos;
				XTick temp_stoppos  = (stoppos  > event_end)   ? event_end   : stoppos;
				
				MusECore::WaveEventSelection s;
				s.event = event;
				s.startframe = event.getAudioStream()->relTick2FrameInFile(temp_startpos);
				s.endframe   = event.getAudioStream()->relTick2FrameInFile(temp_stoppos)+1;
				selection.push_back(s);
			}
		}
	}
	
	return selection;
}

      
//---------------------------------------------------------
//   modifySelection
//---------------------------------------------------------
void WaveCanvas::modifySelection(int operation, XTick startpos, XTick stoppos, double paramA)
      {
	         
    
        if (operation == PASTE) { // TODO FINDMICHJETZT migrate PASTE
          /* // we need to redefine startpos and stoppos
          if (copiedPart =="")
            return;
          MusECore::SndFile pasteFile(copiedPart);
          pasteFile.openRead();
          startpos = pos[0];
          stoppos = startpos+ pasteFile. DONOTCOMPILE samples(); // this is WRONG! this will cast samples implicitly into XTicks and then run havoc!
          pasteFile.close();
          pos[0]=stoppos; */
        }

        //
        // Copy on Write: Check if some files need to be copied, either because they are not 
        //  writable, or more than one independent (non-clone) wave event shares a wave file.
        //
        
        MusECore::WaveSelectionList selection = getSelection(startpos, stoppos);
        std::vector<QString> copy_files_proj_dir;
        for(MusECore::iWaveSelection wavesel_it = selection.begin(); wavesel_it != selection.end(); wavesel_it++) 
        {
          if(wavesel_it->event.needCopyOnWrite())
          {
            QString canonical_path = QFileInfo(wavesel_it->event.audioFilePath()).canonicalPath();
            std::vector<QString>::iterator i = copy_files_proj_dir.begin();
            for( ; i != copy_files_proj_dir.end(); ++i)
            {
              if(*i == canonical_path)
                break; 
            }  
            if(i == copy_files_proj_dir.end())
              copy_files_proj_dir.push_back(canonical_path);
          }
        }
        if(!copy_files_proj_dir.empty())
        {
          CopyOnWriteDialog* dlg = new CopyOnWriteDialog();
          for(std::vector<QString>::iterator i = copy_files_proj_dir.begin(); i != copy_files_proj_dir.end(); ++i)
          {
            qint64 sz = QFile(*i).size();
            QString s;
            if(sz > 1048576)
              s += QString::number(sz / 1048576) + "MB ";
            else
            if(sz > 1024)
              s += QString::number(sz / 1024) + "KB ";
            else
              s += QString::number(sz) + "B ";
            s += *i;
            dlg->addProjDirFile(s);
          }
          int rv = dlg->exec();
          delete dlg;
          if(rv != QDialog::Accepted)
            return;
          // Has a project been created yet?
          if(MusEGlobal::museProject == MusEGlobal::museProjectInitPath) // && MusEGlobal::config.useProjectSaveDialog
          { 
            // No project, we need to create one.
            if(!MusEGlobal::muse->saveAs())
              return; // No project, don't want to copy without one.
            //setFocus(); // For some reason focus is given away to Arranger
          }
          for(MusECore::iWaveSelection wavesel_it = selection.begin(); wavesel_it != selection.end(); wavesel_it++)
          {
            if(!wavesel_it->event.needCopyOnWrite()) // Make sure to re-check
              continue;
            
            QString filePath = MusEGlobal::museProject + QString("/") + QFileInfo(wavesel_it->event.audioFilePath()).fileName();
            QString newFilePath;
            if(MusECore::getUniqueFileName(filePath, newFilePath))
            {
              {
                QFile qf(QFileInfo(wavesel_it->event.audioFilePath()).canonicalPath());
                if(!qf.copy(newFilePath)) // Copy the file
                {
                  printf("MusE Error: Could not copy to new sound file (file exists?): %s\n", newFilePath.toLatin1().constData());
                  continue;  // Let's not overwrite an existing file
                }
              }  
              QFile nqf(newFilePath);
              // Need to make sure some permissions are set...
              QFile::Permissions pm = nqf.permissions();
              if(!(pm & QFile::ReadOwner))
              {
                pm |= QFile::ReadOwner;
                if(!nqf.setPermissions(pm))
                {
                  printf("MusE Error: Could not set read owner permissions on new sound file: %s\n", newFilePath.toLatin1().constData());
                  continue; 
                }
              }
              if(!(pm & QFile::WriteOwner))
              {
                pm |= QFile::WriteOwner;
                if(!nqf.setPermissions(pm))
                {
                  printf("MusE Error: Could not set write owner permissions on new sound file: %s\n", newFilePath.toLatin1().constData());
                  continue; 
                }
              }
              if(!(pm & QFile::ReadUser))
              {
                pm |= QFile::ReadUser;
                if(!nqf.setPermissions(pm))
                {
                  printf("MusE Error: Could not set read user permissions on new sound file: %s\n", newFilePath.toLatin1().constData());
                  continue; 
                }
              }
              if(!(pm & QFile::WriteUser))
              {
                pm |= QFile::WriteUser;
                if(!nqf.setPermissions(pm))
                {
                  printf("MusE Error: Could not set write user permissions on new sound file: %s\n", newFilePath.toLatin1().constData());
                  continue; 
                }
              }

              MusEGlobal::audio->msgIdle(true); 
              // NOTE: For now, don't bother providing undo for this. Reason: If the user undoes
              //  and then modifies again, it will prompt to create new copies each time. There is
              //  no mechanism ("touched"?) to tell if an existing copy would be suitable to just 'drop in'.
              // It would help if we deleted the wave file copies upon undo, but not too crazy about that. 
              // So since the copy has already been created and "there it is", we might as well use it.
              // It means that events and even undo items BEFORE this operation will point to this 
              //  NEW wave file (as if they always did). It also means the user CANNOT change back 
              //  to the old file...    Oh well, this IS Copy On Write.
              // FIXME: Find a conceptual way to make undo work with or without deleting the copies. 
              wavesel_it->event.setAudioFile(newFilePath);            // Set the new file.
              MusEGlobal::audio->msgIdle(false); 
            }
          }
        }
         
         MusEGlobal::song->startUndo();
         for (MusECore::iWaveSelection i = selection.begin(); i != selection.end(); i++) {
               MusECore::WaveEventSelection w = *i;
               MusECore::SndFile file         = MusECore::SndFile(w.event.audioFilePath());
               if (file.openRead()) {
                     MusEGlobal::audio->msgIdle(false);
                     printf("Could not open original file...\n");
                     break;
                     }
	       
               unsigned file_channels = file.channels();

               QString tmpWavFile = QString::null;
               if (!MusEGlobal::getUniqueTmpfileName("tmp_musewav",".wav", tmpWavFile)) {
                     break;
                     }

               MusEGlobal::audio->msgIdle(true); // Not good with playback during operations
               MusECore::SndFile tmpFile(tmpWavFile);
               tmpFile.setFormat(file.format(), file_channels, file.samplerate());
               if (tmpFile.openWrite()) {
                     MusEGlobal::audio->msgIdle(false);
                     printf("Could not open temporary file...\n");
                     break;
                     }

               //
               // Write out original data that will be changed to temp file (i.e. save it, for being able to undo it later)
               //
               unsigned tmpdatalen = w.endframe - w.startframe;
               off_t    tmpdataoffset = w.startframe;
               float*   tmpdata[file_channels];

               for (unsigned i=0; i<file_channels; i++) {
                     tmpdata[i] = new float[tmpdatalen];
                     }
               file.seek(tmpdataoffset, 0);
               file.readWithHeap(file_channels, tmpdata, tmpdatalen);
               file.close();
               tmpFile.write(file_channels, tmpdata, tmpdatalen);
               tmpFile.close();

               switch(operation)
               {
                     case MUTE:
                           muteSelection(file_channels, tmpdata, tmpdatalen);
                           break;

                     case NORMALIZE:
                           normalizeSelection(file_channels, tmpdata, tmpdatalen);
                           break;

                     case FADE_IN:
                           fadeInSelection(file_channels, tmpdata, tmpdatalen);
                           break;

                     case FADE_OUT:
                           fadeOutSelection(file_channels, tmpdata, tmpdatalen);
                           break;

                     case REVERSE:
                           reverseSelection(file_channels, tmpdata, tmpdatalen);
                           break;

                     case GAIN:
                           applyGain(file_channels, tmpdata, tmpdatalen, paramA);
                           break;
                     case CUT:
                           copySelection(file_channels, tmpdata, tmpdatalen, true, file.format(), file.samplerate());
                           break;
                     case COPY:
                           copySelection(file_channels, tmpdata, tmpdatalen, false, file.format(), file.samplerate());
                           break;
                     case PASTE:
                           {
                           MusECore::SndFile pasteFile(copiedPart);
                           pasteFile.openRead();
                           pasteFile.seek(tmpdataoffset, 0);
                           pasteFile.readWithHeap(file_channels, tmpdata, tmpdatalen);
                           }
                           break;

                     case EDIT_EXTERNAL:
                           editExternal(file.format(), file.samplerate(), file_channels, tmpdata, tmpdatalen);
                           break;

                     default:
                           printf("Error: Default state reached in modifySelection\n");
                           break;

               }

               file.openWrite();
               file.seek(tmpdataoffset, 0);
               file.write(file_channels, tmpdata, tmpdatalen);
               file.update();
               file.close();
               
               w.event.reloadAudioFile();

               for (unsigned i=0; i<file_channels; i++) {
                     delete[] tmpdata[i];
                     }

               // Undo handling
               MusEGlobal::song->cmdChangeWave(file.dirPath() + "/" + file.name(), tmpWavFile, w.startframe, w.endframe);
               MusEGlobal::audio->msgIdle(false); // Not good with playback during operations
               }
         MusEGlobal::song->endUndo(SC_CLIP_MODIFIED);
         redraw();
      }

//---------------------------------------------------------
//   copySelection
//---------------------------------------------------------
void WaveCanvas::copySelection(unsigned file_channels, float** tmpdata, unsigned length, bool blankData, unsigned format, unsigned sampleRate)
{
      if (copiedPart!="") {
        QFile::remove(copiedPart);
      }
      if (!MusEGlobal::getUniqueTmpfileName("tmp_musewav",".wav", copiedPart)) {
            return;
            }

      MusECore::SndFile tmpFile(copiedPart);
      tmpFile.setFormat(format, file_channels, sampleRate);
      tmpFile.openWrite();
      tmpFile.write(file_channels, tmpdata, length);
      tmpFile.close();

      if (blankData) {
        // Set everything to 0!
        for (unsigned i=0; i<file_channels; i++) {
              for (unsigned j=0; j<length; j++) {
                    tmpdata[i][j] = 0;
                    }
              }
        }
}

//---------------------------------------------------------
//   muteSelection
//---------------------------------------------------------
void WaveCanvas::muteSelection(unsigned channels, float** data, unsigned length)
      {
      // Set everything to 0!
      for (unsigned i=0; i<channels; i++) {
            for (unsigned j=0; j<length; j++) {
                  data[i][j] = 0;
                  }
            }
      }

//---------------------------------------------------------
//   normalizeSelection
//---------------------------------------------------------
void WaveCanvas::normalizeSelection(unsigned channels, float** data, unsigned length)
      {
      float loudest = 0.0;

      for (unsigned i=0; i<channels; i++) {
            for (unsigned j=0; j<length; j++) {
                  if (data[i][j]  > loudest)
                        loudest = data[i][j];
                  }
            }

      double scale = 0.99 / (double)loudest;

      for (unsigned i=0; i<channels; i++) {
            for (unsigned j=0; j<length; j++) {
                  data[i][j] = (float) ((double)data[i][j] * scale);
                  }
            }
      }

//---------------------------------------------------------
//   fadeInSelection
//---------------------------------------------------------
void WaveCanvas::fadeInSelection(unsigned channels, float** data, unsigned length)
      {
      for (unsigned i=0; i<channels; i++) {
            for (unsigned j=0; j<length; j++) {
                  double scale = (double) j / (double)length ;
                  data[i][j] = (float) ((double)data[i][j] * scale);
                  }
            }
      }

//---------------------------------------------------------
//   fadeOutSelection
//---------------------------------------------------------
void WaveCanvas::fadeOutSelection(unsigned channels, float** data, unsigned length)
      {
      for (unsigned i=0; i<channels; i++) {
            for (unsigned j=0; j<length; j++) {
                  double scale = (double) (length - j) / (double)length ;
                  data[i][j] = (float) ((double)data[i][j] * scale);
                  }
            }
      }

//---------------------------------------------------------
//   reverseSelection
//---------------------------------------------------------
void WaveCanvas::reverseSelection(unsigned channels, float** data, unsigned length)
      {
      if(length <= 1)    
        return;
      for (unsigned i=0; i<channels; i++) {
            for (unsigned j=0; j<length/2; j++) {
                  float tmpl = data[i][j];
                  float tmpr = data[i][length - j - 1];
                  data[i][j] = tmpr;
                  data[i][length - j - 1] = tmpl;
                  }
            }
      }
//---------------------------------------------------------
//   applyGain
//---------------------------------------------------------
void WaveCanvas::applyGain(unsigned channels, float** data, unsigned length, double gain)
      {
      for (unsigned i=0; i<channels; i++) {
            for (unsigned j=0; j<length; j++) {
                  data[i][j] = (float) ((double)data[i][j] * gain);
                  }
            }
      }

//---------------------------------------------------------
//   editExternal
//---------------------------------------------------------
void WaveCanvas::editExternal(unsigned file_format, unsigned file_samplerate, unsigned file_channels, float** tmpdata, unsigned tmpdatalen)
      {
      // Create yet another tmp-file
      QString exttmpFileName;
      if (!MusEGlobal::getUniqueTmpfileName("tmp_musewav",".wav", exttmpFileName)) {
            printf("Could not create temp file - aborting...\n");
            return;
            }

      MusECore::SndFile exttmpFile(exttmpFileName);
      exttmpFile.setFormat(file_format, file_channels, file_samplerate);
      if (exttmpFile.openWrite()) {
            printf("Could not open temporary file...\n");
            return;
            }
      // Write out change-data to this file:
      exttmpFile.write(file_channels, tmpdata, tmpdatalen);
      exttmpFile.close();

      // Forkaborkabork
      int pid = fork();
      if (pid == 0) {
            if (execlp(MusEGlobal::config.externalWavEditor.toLatin1().constData(), MusEGlobal::config.externalWavEditor.toLatin1().constData(), exttmpFileName.toLatin1().constData(), NULL) == -1) {
                  perror("Failed to launch external editor");
                  // Get out of here
                  
                   
                  // cannot report error through gui, we are in another fork!
                  //@!TODO: Handle unsuccessful attempts
                  exit(99);
                  }
            exit(0);
            }
      else if (pid == -1) {
            perror("fork failed");
            }
      else {
            int status;
            waitpid(pid, &status, 0);
            //printf ("status=%d\n",status);
            if( WEXITSTATUS(status) != 0 ){
                   QMessageBox::warning(this, tr("MusE - external editor failed"),
                         tr("MusE was unable to launch the external editor\ncheck if the editor setting in:\n"
                         "Global Settings->Audio:External Waveditor\nis set to a valid editor."));
            }
            
            if (exttmpFile.openRead()) {
                printf("Could not reopen temporary file!\n");
                }
            else {
                // Re-read file again
                exttmpFile.seek(0, 0);
                size_t sz = exttmpFile.readWithHeap(file_channels, tmpdata, tmpdatalen);
                if (sz != tmpdatalen) {
                        // File must have been shrunken - not good. Alert user.
                        QMessageBox::critical(this, tr("MusE - file size changed"),
                            tr("When editing in external editor - you should not change the filesize\nsince it must fit the selected region.\n\nMissing data is muted"));
                        for (unsigned i=0; i<file_channels; i++) {
                            for (unsigned j=sz; j<tmpdatalen; j++) {
                                    tmpdata[i][j] = 0;
                                    }
                            }
                        }
                }
            QDir dir = exttmpFile.dirPath();
            dir.remove(exttmpFileName);
            dir.remove(exttmpFile.basename() + ".wca");
            }
      }

      
      
//---------------------------------------------------------
//   startDrag
//---------------------------------------------------------

void WaveCanvas::startDrag(MusEGui::CItem* /* item*/, bool copymode)
      {
      QMimeData* md = MusECore::selected_events_to_mime(MusECore::partlist_to_set(editor->parts()), 1);
      
      if (md) {
            // "Note that setMimeData() assigns ownership of the QMimeData object to the QDrag object. 
            //  The QDrag must be constructed on the heap with a parent QWidget to ensure that Qt can 
            //  clean up after the drag and drop operation has been completed. "
            QDrag* drag = new QDrag(this);
            drag->setMimeData(md);
            
            if (copymode)
                  drag->exec(Qt::CopyAction);
            else
                  drag->exec(Qt::MoveAction);
            }
      }

//---------------------------------------------------------
//   dragEnterEvent
//---------------------------------------------------------

void WaveCanvas::dragEnterEvent(QDragEnterEvent* event)
      {
      //event->accept(Q3TextDrag::canDecode(event));
      event->acceptProposedAction();  // TODO CHECK Tim.
      }

//---------------------------------------------------------
//   dragMoveEvent
//---------------------------------------------------------

void WaveCanvas::dragMoveEvent(QDragMoveEvent*)
      {
      //printf("drag move %x\n", this); DELETETHIS (whole function?)
      //event->acceptProposedAction();  
      }

//---------------------------------------------------------
//   dragLeaveEvent
//---------------------------------------------------------

void WaveCanvas::dragLeaveEvent(QDragLeaveEvent*)
      {
      //printf("drag leave\n");         DELETETHIS (whole function?)
      //event->acceptProposedAction();  
      }

//---------------------------------------------------------
//   itemPressed
//---------------------------------------------------------

void WaveCanvas::itemPressed(const MusEGui::CItem*)
      {
      }

//---------------------------------------------------------
//   itemReleased
//---------------------------------------------------------

void WaveCanvas::itemReleased(const MusEGui::CItem*, const QPoint&)
      {
      }

//---------------------------------------------------------
//   itemMoved
//---------------------------------------------------------

void WaveCanvas::itemMoved(const MusEGui::CItem*, const QPoint&)
      {
      }

//---------------------------------------------------------
//   curPartChanged
//---------------------------------------------------------

void WaveCanvas::curPartChanged()
      {
      EventCanvas::curPartChanged();
      editor->setWindowTitle(getCaption());
      }

//---------------------------------------------------------
//   modifySelected
//---------------------------------------------------------

void WaveCanvas::modifySelected(MusEGui::NoteInfo::ValType type, int val, bool delta_mode)
      {
      // TODO: New WaveCanvas: Convert this routine to frames and remove unneeded operations. 
      QList< QPair<MusECore::EventList*,MusECore::Event> > already_done;
      MusEGlobal::audio->msgIdle(true);
      MusEGlobal::song->startUndo();
      for (MusEGui::iCItem i = items.begin(); i != items.end(); ++i) {
            if (!(i->second->isSelected()))
                  continue;
            WEvent* e   = (WEvent*)(i->second);
            MusECore::Event event = e->event();
            if (event.type() != MusECore::Note)
                  continue;

            MusECore::WavePart* part = (MusECore::WavePart*)(e->part());
            
            if (already_done.contains(QPair<MusECore::EventList*,MusECore::Event>(part->events(), event)))
              continue;
            
            MusECore::Event newEvent = event.clone();

            switch (type) {
                  case MusEGui::NoteInfo::VAL_TIME:
                        {
                        int newTime = val;
                        if(delta_mode)
                          newTime += event.tick();
                        else
                          newTime -= part->tick();
                        if (newTime < 0)
                           newTime = 0;
                        newEvent.setTick(newTime);
                        }
                        break;
                  case MusEGui::NoteInfo::VAL_LEN:
                        {
                        int len = val;
                        if(delta_mode)
                          len += event.lenTick();
                        if (len < 1)
                              len = 1;
                        newEvent.setLenTick(len);
                        }
                        break;
                  case MusEGui::NoteInfo::VAL_VELON:
                        {
                        int velo = val;
                        if(delta_mode)
                          velo += event.velo();
                        if (velo > 127)
                              velo = 127;
                        else if (velo < 0)
                              velo = 0;
                        newEvent.setVelo(velo);
                        }
                        break;
                  case MusEGui::NoteInfo::VAL_VELOFF:
                        {
                        int velo = val;
                        if(delta_mode)
                          velo += event.veloOff();
                        if (velo > 127)
                              velo = 127;
                        else if (velo < 0)
                              velo = 0;
                        newEvent.setVeloOff(velo);
                        }
                        break;
                  case MusEGui::NoteInfo::VAL_PITCH:
                        {
                        int pitch = val;
                        if(delta_mode)
                          pitch += event.pitch();
                        if (pitch > 127)
                              pitch = 127;
                        else if (pitch < 0)
                              pitch = 0;
                        newEvent.setPitch(pitch);
                        }
                        break;
                  }
            
            MusEGlobal::song->changeEvent(event, newEvent, part);
            // Indicate do not do port controller values and clone parts. 
            MusEGlobal::song->addUndo(MusECore::UndoOp(MusECore::UndoOp::ModifyEvent, newEvent, event, part, false, false));

            already_done.append(QPair<MusECore::EventList*,MusECore::Event>(part->events(), event));
            }
      MusEGlobal::song->endUndo(SC_EVENT_MODIFIED);
      MusEGlobal::audio->msgIdle(false);
      }

//---------------------------------------------------------
//   resizeEvent
//---------------------------------------------------------

void WaveCanvas::resizeEvent(QResizeEvent* ev)
      {
      // Readjust all wave canvas item heights  
      bool do_redraw = false;
      for (iCItem k = items.begin(); k != items.end(); ++k) 
      {
        if(k->second->height() != ev->size().height())
        {
          k->second->setHeight(ev->size().height());
          do_redraw = true;
        }
      }
  
      if (ev->size().width() != ev->oldSize().width())
            emit newWidth(ev->size().width());
      EventCanvas::resizeEvent(ev);
  
      if(do_redraw)
        redraw();
      }

} // namespace MusEGui
