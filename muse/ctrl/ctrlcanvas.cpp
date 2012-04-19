//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: ctrlcanvas.cpp,v 1.15.2.10 2009/11/14 03:37:48 terminator356 Exp $
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
#include <limits.h>

#include <QPainter>
#include <QCursor>
#include <QMouseEvent>

#include "app.h"
#include "globals.h"
#include "ctrledit.h"
#include "midieditor.h"
#include "icons.h"
#include "midiport.h"
#include "song.h"
#include "midictrl.h"
#include "audio.h"
#include "gconfig.h"
#include "ctrlpanel.h"
#include "midiedit/drummap.h"

static MusECore::MidiCtrlValList veloList(MusECore::CTRL_VELOCITY);    // dummy

namespace MusEGui {

//---------------------------------------------------------
//   computeVal
//---------------------------------------------------------

static int computeVal(MusECore::MidiController* mc, int y, int height)
      {
      int min; int max;
      if(mc->num() == MusECore::CTRL_PROGRAM)
      {
        min = 1;
        max = 128;
      }
      else
      {
        min = mc->minVal();
        max = mc->maxVal();
      }
      int val = max - (y * (max-min) / height);
      if (val < min)
            val = min;
      if (val > max)
            val = max;
      if(mc->num() != MusECore::CTRL_PROGRAM)
        val += mc->bias();
      return val;
      }

//---------------------------------------------------------
//   computeY
//---------------------------------------------------------

static int computeY(const MusECore::MidiController* mc, int val, int height)
      {
      int min; int max; 
      if(mc->num() == MusECore::CTRL_PROGRAM)
      {
        min = 1;
        max = 128;
      }
      else
      {
        min = mc->minVal();
        max = mc->maxVal();
      }
      
      //printf("computeY #1 min:%d max:%d val:%d bias:%d height:%d\n", min, max, val, mc->bias(), height);
      
      if(mc->num() != MusECore::CTRL_PROGRAM)
        val -= mc->bias();
      
      if (val < min)
            val = min;
      if (val > max)
            val = max;
      
      int y = min == max ? 0 : (max-val)*height/(max-min);
      
      //printf("computeY #2 y:%d min:%d max:%d val:%d bias:%d height:%d\n", y, min, max, val, mc->bias(), height);
      
      return y;
      }

//---------------------------------------------------------
//   CEvent
//---------------------------------------------------------

CEvent::CEvent(MusECore::Event e, MusECore::MidiPart* pt, int v)
      {
      _event = e;
      _part  = pt;
      _val   = v;
      ex     = !e.empty() ? e.tick() : 0;
      }

//---------------------------------------------------------
//   intersects
//---------------------------------------------------------

bool CEvent::intersects(const MusECore::MidiController* mc, const QRect& r, const int tickstep, const int wh) const
{
      if(_event.empty())
        return false;
      int y1 = computeY(mc, _val, wh);
      
      int tick1 = _event.tick() + _part->tick();
      if(ex == -1)
        return tick1 < (r.x() + r.width()) && y1 < (r.y() + r.height());
      
      int tick2 = ex + _part->tick();
      
      // Velocities don't use EX (set equal to event tick, giving zero width here),
      //  and velocities are drawn three pixels wide, so adjust the width now. 
      // Remember, each drawn pixel represents one tickstep which varies with zoom.
      // So that's 3 x tickstep for each velocity line.
      // Hmm, actually, for better pin-point accuracy just use one tickstep for now.
      if(MusECore::midiControllerType(mc->num()) == MusECore::MidiController::Velo)
        tick2 += tickstep;
      
      QRect er(tick1, y1, tick2 - tick1, wh - y1);   

      return r.intersects(er);
}

//---------------------------------------------------------
//   contains
//---------------------------------------------------------

bool CEvent::contains(int x1, int x2) const
      {
      int tick1 = !_event.empty() ? _event.tick() + _part->tick() : 0;
      if(ex == -1)
        return (tick1 < x2);
      
      int tick2 = ex + _part->tick();
      return ((tick1 >= x1 && tick1 < x2)
         //|| (tick2 >= x1 && tick2 < x2) DELETETHIS?
         || (tick2 > x1 && tick2 < x2)
         || (tick1 < x1 && tick2 >= x2));
      }

//---------------------------------------------------------
//   clearDelete
//---------------------------------------------------------

void CEventList::clearDelete()
{
  for(ciCEvent i = begin(); i != end(); ++i) 
  {
    CEvent* ce = *i;
    if(ce)
      delete ce;
  }
  clear();
}

//---------------------------------------------------------
//   CtrlCanvas
//---------------------------------------------------------

CtrlCanvas::CtrlCanvas(MidiEditor* e, QWidget* parent, int xmag,
   const char* name, CtrlPanel* pnl) : View(parent, xmag, 1, name)
      {
      setBg(Qt::white);
      setFont(MusEGlobal::config.fonts[3]);  
      editor = e;
      drag   = DRAG_OFF;
      tool   = MusEGui::PointerTool;
      pos[0] = 0;
      pos[1] = 0;
      pos[2] = 0;
      noEvents=false;

      ctrl   = &veloList;
      _controller = &MusECore::veloCtrl;
      _panel = pnl;
      _cnum  = MusECore::CTRL_VELOCITY;    
      _dnum  = MusECore::CTRL_VELOCITY;    
      _didx  = MusECore::CTRL_VELOCITY;      
      connect(MusEGlobal::song, SIGNAL(posChanged(int, unsigned, bool)), this, SLOT(setPos(int, unsigned, bool)));

      setMouseTracking(true);
      curPart = 0;
      curTrack = 0;
      if (!editor->parts()->empty())
            setCurTrackAndPart();

      connect(MusEGlobal::song, SIGNAL(songChanged(int)), SLOT(songChanged(int)));
      connect(MusEGlobal::muse, SIGNAL(configChanged()), SLOT(configChanged()));
      
      curDrumInstrument = editor->curDrumInstrument();
      //printf("CtrlCanvas::CtrlCanvas curDrumInstrument:%d\n", curDrumInstrument);
                          
      connect(editor, SIGNAL(curDrumInstrumentChanged(int)), SLOT(setCurDrumInstrument(int)));
      updateItems();
      }

CtrlCanvas::~CtrlCanvas()
{
  items.clearDelete();
}
   
//---------------------------------------------------------
//   setPos
//    set one of three markers
//    idx   - 0-cpos  1-lpos  2-rpos
//    flag  - emit followEvent()
//---------------------------------------------------------

void CtrlCanvas::setPos(int idx, unsigned val, bool adjustScrollbar)
      {
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
                              int ppos =  val - rmapxDev(width()/4);
                              if (ppos < 0)
                                    ppos = 0;
                              emit followEvent(ppos);
                              opos = mapx(pos[idx]);
                              npos = mapx(val);
                              }
                        else if (npos < 0) {
                              int ppos =  val - rmapxDev(width()*3/4);
                              if (ppos < 0)
                                    ppos = 0;
                              emit followEvent(ppos);
                              opos = mapx(pos[idx]);
                              npos = mapx(val);
                              }
                        break;
                  case MusECore::Song::CONTINUOUS:
                        if (npos > (width()*5)/8) {
                              int ppos =  pos[idx] - rmapxDev(width()*5/8);
                              if (ppos < 0)
                                    ppos = 0;
                              emit followEvent(ppos);
                              opos = mapx(pos[idx]);
                              npos = mapx(val);
                              }
                        else if (npos < (width()*3)/8) {
                              int ppos =  pos[idx] - rmapxDev(width()*3/8);
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
      redraw(QRect(x, 0, w, height()));
      }

//---------------------------------------------------------
//   setMidiController
//---------------------------------------------------------

void CtrlCanvas::setMidiController(int num)
      {
      _cnum = num;    
      partControllers(curPart, _cnum, &_dnum, &_didx, &_controller, &ctrl);
      
      if(_panel)
      {
        if(_cnum == MusECore::CTRL_VELOCITY)              
          _panel->setHWController(curTrack, &MusECore::veloCtrl);
        else 
          _panel->setHWController(curTrack, _controller);
      }  
    }

//---------------------------------------------------------
//   leaveEvent
//---------------------------------------------------------

void CtrlCanvas::leaveEvent(QEvent*)
      {
      emit xposChanged(INT_MAX);
      emit yposChanged(-1);
      }

//---------------------------------------------------------
//   raster
//---------------------------------------------------------

QPoint CtrlCanvas::raster(const QPoint& p) const
      {
      return p;
      }

//---------------------------------------------------------
//   deselectAll
//---------------------------------------------------------

void CtrlCanvas::deselectAll()
      {
        for(iCEvent i = selection.begin(); i != selection.end(); ++i)
            (*i)->setSelected(false);

        selection.clear();
      }

//---------------------------------------------------------
//   selectItem
//---------------------------------------------------------

void CtrlCanvas::selectItem(CEvent* e)
      {
      e->setSelected(true);
      selection.push_back(e);
      }

//---------------------------------------------------------
//   deselectItem
//---------------------------------------------------------

void CtrlCanvas::deselectItem(CEvent* e)
      {
      e->setSelected(false);
      for (iCEvent i = selection.begin(); i != selection.end(); ++i) {
            if (*i == e) {
                  selection.erase(i);
                  break;
                  }
            }
      }

//---------------------------------------------------------
//   setController
//---------------------------------------------------------

void CtrlCanvas::setController(int num)
{
  setMidiController(num);
  updateItems();
}


//---------------------------------------------------------
//   setCurTrackAndPart
//---------------------------------------------------------

bool CtrlCanvas::setCurTrackAndPart()
{
  bool changed = false;
  MusECore::MidiPart* part = 0;
  MusECore::MidiTrack* track = 0;
  
  if(!editor->parts()->empty()) 
  {
    MusECore::Part* pt = editor->curCanvasPart();
    if(pt && pt->track())
    {
      if(pt->track()->isMidiTrack())
      {
        part = (MusECore::MidiPart*)pt;
        track = part->track();
      }  
    }
  }
    
  if(part != curPart)
  {
    curPart = part;
    changed = true;
  }  
  
  if(track != curTrack)
  {
    curTrack = track;
    changed = true;
  }  
  
  return changed;
}

//---------------------------------------------------------
//   configChanged
//---------------------------------------------------------

void CtrlCanvas::configChanged()    
{ 
  songChanged(SC_CONFIG); 
}

//---------------------------------------------------------
//   songChanged
//    all marked parts are added to the internal event list
//---------------------------------------------------------

void CtrlCanvas::songChanged(int type)
{
  if(editor->deleting())  // Ignore while while deleting to prevent crash.
    return; 
  
  //printf("CtrlCanvas::songChanged type:%x\n", type);  
  // Is it simply a midi controller value adjustment? Forget it.
  if(type == SC_MIDI_CONTROLLER)
    return;
            
  if(type & SC_CONFIG)
    setFont(MusEGlobal::config.fonts[3]);  
  
  bool changed = false;
  if(type & (SC_CONFIG | SC_PART_MODIFIED | SC_SELECTION))
    changed = setCurTrackAndPart();
            
  // Although changing the instrument/device in the
  //  config window generates a type of -1, we can eliminate
  //  some other useless calls using SC_CONFIG, which was not used 
  //  anywhere else in muse before now, except song header.
  if((type & (SC_CONFIG | SC_DRUMMAP)) || ((type & (SC_PART_MODIFIED | SC_SELECTION)) && changed))
    setMidiController(_cnum);
  
  if(!curPart)         
    return;
              
  if(type & (SC_CONFIG | SC_DRUMMAP | SC_PART_MODIFIED | SC_EVENT_INSERTED | SC_EVENT_REMOVED | SC_EVENT_MODIFIED))   
    updateItems();
  else if(type & SC_SELECTION)
    updateSelections();               
}

//---------------------------------------------------------
//   partControllers
//---------------------------------------------------------

void CtrlCanvas::partControllers(const MusECore::MidiPart* part, int num, int* dnum, int* didx, MusECore::MidiController** mc, MusECore::MidiCtrlValList** mcvl)
{
  if(num == MusECore::CTRL_VELOCITY) // special case
  {    
    if(mcvl)
      *mcvl = &veloList;
    if(mc)
      *mc = &MusECore::veloCtrl;
    if(dnum)
      *dnum = num;
    if(didx)
      *didx = num;
  }
  else 
  {
    if(!part)         
    {
      if(mcvl)
        *mcvl = 0;
      if(mc)
        *mc = 0;
      if(dnum)
        *dnum = 0;
      if(didx)
        *didx = 0;
      return;
    }
    
    MusECore::MidiTrack* mt = part->track();
    MusECore::MidiPort* mp;
    int di;
    int n;
    
    if((mt->type() != MusECore::Track::DRUM) && curDrumInstrument != -1)
      printf("keyfilter != -1 in non drum track?\n");

    if((mt->type() == MusECore::Track::DRUM) && (curDrumInstrument != -1) && ((num & 0xff) == 0xff)) 
    {
      di = (num & ~0xff) | curDrumInstrument;
      n = (num & ~0xff) | MusEGlobal::drumMap[curDrumInstrument].anote;  // construct real controller number
      mp = &MusEGlobal::midiPorts[MusEGlobal::drumMap[curDrumInstrument].port];          
    }
    else
    {
       di = num;
       n = num;
       mp = &MusEGlobal::midiPorts[mt->outPort()];          
    }
    
    if(dnum)
      *dnum = n;
          
    if(didx)
      *didx = di;
      
    if(mc)
      *mc = mp->midiController(n);
      
    if(mcvl)
    {
      MusECore::MidiCtrlValList* tmcvl = 0;
      MusECore::MidiCtrlValListList* cvll = mp->controller();
      for(MusECore::iMidiCtrlValList i = cvll->begin(); i != cvll->end(); ++i) 
      {
        if(i->second->num() == n) 
        {
          tmcvl = i->second;
          break;
        }
      }
      *mcvl = tmcvl;
      
    }    
  }
}

//---------------------------------------------------------
//   updateItems
//---------------------------------------------------------

void CtrlCanvas::updateItems()
      {
      selection.clear();
      items.clearDelete();
      
      
      if(!editor->parts()->empty())
      {
        CEvent *newev = 0;
  
        for (MusECore::iPart p = editor->parts()->begin(); p != editor->parts()->end(); ++p) 
        {
              MusECore::Event last;
              CEvent* lastce  = 0;
              
              MusECore::MidiPart* part = (MusECore::MidiPart*)(p->second);
              MusECore::EventList* el = part->events();
              MusECore::MidiCtrlValList* mcvl;
              partControllers(part, _cnum, 0, 0, 0, &mcvl);
              unsigned len = part->lenTick();

              for (MusECore::iEvent i = el->begin(); i != el->end(); ++i) 
              {
                    MusECore::Event e = i->second;
                    // Added by T356. Do not add events which are past the end of the part.
                    if(e.tick() >= len)
                      break;
                    
                    if(_cnum == MusECore::CTRL_VELOCITY && e.type() == MusECore::Note) 
                    {
                          newev = 0;
                          if(curDrumInstrument == -1) 
                          {
                                // This is interesting - it would allow ALL drum note velocities to be shown.
                                // But currently the drum list ALWAYS has a selected item so this is not supposed to happen.
                                items.add(newev = new CEvent(e, part, e.velo()));
                          }
                          else if (e.dataA() == curDrumInstrument) //same note
                                items.add(newev = new CEvent(e, part, e.velo()));
                          if(newev && e.selected())
                            selection.push_back(newev);
                    }
                    else if (e.type() == MusECore::Controller && e.dataA() == _didx) 
                    {
                          if(mcvl && last.empty()) 
                          {
                                lastce = new CEvent(MusECore::Event(), part, mcvl->value(part->tick()));
                                items.add(lastce);
                          }
                          if (lastce)
                                lastce->setEX(e.tick());
                          lastce = new CEvent(e, part, e.dataB());
                          lastce->setEX(-1);
                          items.add(lastce);
                          if(e.selected())
                            selection.push_back(lastce);
                          last = e;
                    }    
              }
        }
      }  
      redraw();
    }

//---------------------------------------------------------
//   updateSelections
//---------------------------------------------------------

void CtrlCanvas::updateSelections()
{
  selection.clear();
  for(ciCEvent i = items.begin(); i != items.end(); ++i) 
  {
    CEvent* e = *i;
    //if(e->part() != part)
    //  continue;
    if(e->selected())
      selection.push_back(e);
  }  
  redraw();
}

//---------------------------------------------------------
//   viewMousePressEvent
//---------------------------------------------------------

void CtrlCanvas::viewMousePressEvent(QMouseEvent* event)
      {
      if(!_controller)  
        return;
        
      start = event->pos();
      MusEGui::Tool activeTool = tool;
      
      bool ctrlKey = event->modifiers() & Qt::ControlModifier;
      int xpos = start.x();
      int ypos = start.y();

      MusECore::MidiController::ControllerType type = MusECore::midiControllerType(_controller->num());

      switch (activeTool) {
            case MusEGui::PointerTool:
                  if(curPart)      
                  {
                    drag = DRAG_LASSO_START;
                    bool do_redraw = false;
                    if (!ctrlKey)
                    {
                      deselectAll();
                      do_redraw = true;      
                    }      
                    int h = height();
                    int tickstep = rmapxDev(1);
                    QRect r(xpos, ypos, tickstep, rmapyDev(1));
                    int endTick = xpos + tickstep;
                    int partTick = curPart->tick();
                    for (iCEvent i = items.begin(); i != items.end(); ++i) 
                    {
                      CEvent* ev = *i;
                      if(ev->part() != curPart)
                        continue;
                      MusECore::Event event = ev->event();
                      if(event.empty())
                        continue;
                      int ax = event.tick() + partTick;
                      //if (ax < xpos)
                      //      continue;
                      if (ax >= endTick)
                        break;
                      if (ev->intersects(_controller, r, tickstep, h)) 
                      {
                        if (ctrlKey && ev->selected())
                              deselectItem(ev);
                        else
                              selectItem(ev);
                        do_redraw = true;      
                        //break;
                      }  
                    }
                    if(do_redraw)
                      redraw();                 // Let songChanged handle the redraw upon SC_SELECTION.
                  }
                  
                  
                  break;

           case MusEGui::PencilTool:
                  if ((!ctrlKey) && (type != MusECore::MidiController::Velo)) {
                              drag = DRAG_NEW;
                              MusEGlobal::song->startUndo();
                              newVal(xpos, ypos);
                        }
                  else {
                        drag = DRAG_RESIZE;
                        MusEGlobal::song->startUndo();
                        changeVal(xpos, xpos, ypos);
                        }
                  break;

            case MusEGui::RubberTool:
                  if (type != MusECore::MidiController::Velo) {
                        drag = DRAG_DELETE;
                        MusEGlobal::song->startUndo();
                        deleteVal(xpos, xpos, ypos);
                        }
                  break;

            case MusEGui::DrawTool:
                  if (drawLineMode) {
                        line2x = xpos;
                        line2y = ypos;
                        if ((!ctrlKey) && (type != MusECore::MidiController::Velo))
                              newValRamp(line1x, line1y, line2x, line2y);
                        else
                              changeValRamp(line1x, line1y, line2x, line2y);
                        drawLineMode = false;
                        }
                  else {
                        line2x = line1x = xpos;
                        line2y = line1y = ypos;
                        drawLineMode = true;
                        }
                  redraw();
                  break;

            default:
                  break;
            }
      }

//---------------------------------------------------------
//   viewMouseMoveEvent
//---------------------------------------------------------

void CtrlCanvas::viewMouseMoveEvent(QMouseEvent* event)
      {
      if(!_controller)  
        return;
        
      QPoint pos  = event->pos();
      QPoint dist = pos - start;
      bool moving = dist.y() >= 3 || dist.y() <= 3 || dist.x() >= 3 || dist.x() <= 3;
      switch (drag) {
            case DRAG_LASSO_START:
                  if (!moving)
                        break;
                  drag = DRAG_LASSO;
                  // fallthrough
            case DRAG_LASSO:
                  lasso.setRect(start.x(), start.y(), dist.x(), dist.y());
                  redraw();
                  break;
            case DRAG_RESIZE:
                  changeVal(start.x(), pos.x(), pos.y());
                  start = pos;
                  break;

            case DRAG_NEW:
                  newVal(start.x(), start.y(), pos.x(), pos.y());
                  start = pos;
                  break;

            case DRAG_DELETE:
                  deleteVal(start.x(), pos.x(), pos.y());
                  start = pos;
                  break;

            default:
                  break;
            }
      if (tool == MusEGui::DrawTool && drawLineMode) {
            line2x = pos.x();
            line2y = pos.y();
            redraw();
            }
      emit xposChanged(pos.x());
      
      
      int val = computeVal(_controller, pos.y(), height());
      emit yposChanged(val);
      }

//---------------------------------------------------------
//   viewMouseReleaseEvent
//---------------------------------------------------------

void CtrlCanvas::viewMouseReleaseEvent(QMouseEvent* event)
      {
      bool ctrlKey = event->modifiers() & Qt::ControlModifier;

      switch (drag) {
            case DRAG_RESIZE:
                  MusEGlobal::song->endUndo(SC_EVENT_MODIFIED);
                  break;
            case DRAG_NEW:
                  MusEGlobal::song->endUndo(SC_EVENT_MODIFIED | SC_EVENT_INSERTED | SC_EVENT_REMOVED);
                  break;
            case DRAG_DELETE:
                  MusEGlobal::song->endUndo(SC_EVENT_REMOVED);
                  break;

            case DRAG_LASSO_START:
                  lasso.setRect(-1, -1, -1, -1);

            case DRAG_LASSO:
                  if(_controller)  
                  {
                    lasso = lasso.normalized();
                    int h = height();
                    int tickstep = rmapxDev(1);
                    for (iCEvent i = items.begin(); i != items.end(); ++i) {
                          if((*i)->part() != curPart)
                            continue;
                          if ((*i)->intersects(_controller, lasso, tickstep, h)) {
                                if (ctrlKey && (*i)->selected())
                                  (*i)->setSelected(false);
                                else
                                  (*i)->setSelected(true);
                              }  
                          }
                    drag = DRAG_OFF;
                      // Let songChanged handle the redraw upon SC_SELECTION.
                      MusEGlobal::song->update(SC_SELECTION);
                  }
                  break;

            default:
                  break;
            }
      drag = DRAG_OFF;
      }

//---------------------------------------------------------
//   newValRamp
//---------------------------------------------------------

void CtrlCanvas::newValRamp(int x1, int y1, int x2, int y2)
      {
      if(!curPart || !_controller)         
        return;
      
      if(x2 - x1 < 0)
      {
        int a = x1;
        x1 = x2;
        x2 = a;
        a = y1;
        y1 = y2;
        y2 = a;
      }
      
      int xx1 = editor->rasterVal1(x1);
      int xx2 = editor->rasterVal2(x2);
      // If x1 and x2 happen to lie directly on the same raster, xx1 will equal xx2, 
      //  which is not good - there should always be a spread. Nudge by +1 and recompute.
      if(xx1 == xx2)
        xx2  = editor->rasterVal2(x2 + 1);
      
      int type = _controller->num();

      bool useRaster = false;
      int raster = editor->raster();
      if (raster == 1)          // set reasonable raster
      {
        raster = MusEGlobal::config.division/16;  // Let's use 64th notes, for a bit finer resolution. Tim.
        useRaster = true;
      }  

      MusECore::Undo operations;

      // delete existing events

      unsigned curPartTick = curPart->tick();
      int lastpv = MusECore::CTRL_VAL_UNKNOWN;
      for (ciCEvent i = items.begin(); i != items.end(); ++i) {
            CEvent* ev = *i;
            if (ev->part() != curPart)
              continue;
            MusECore::Event event = ev->event();
            if (event.empty())
                  continue;
            int x = event.tick() + curPartTick;
            
            if (x < xx1)
                  continue;

            if (x >= xx2)
                  break;
            
            // Do port controller values and clone parts. 
            operations.push_back(MusECore::UndoOp(MusECore::UndoOp::DeleteEvent, event, curPart, true, true));
            }

      if (ctrl)  
        lastpv = ctrl->hwVal();
        
      unsigned curPartLen = curPart->lenTick();
      
      // insert new events
      for (int x = xx1, step; x < xx2 ; x += step )    
      {
            step = useRaster ? raster : editor->rasterVal2(x + 1) - x;
            
            int y    = x + step >= xx2 || x2 == x1 ? y2 : (x == xx1 ? y1 : (((y2 - y1) * (x + step/2 - x1)) / (x2 - x1)) + y1);
            int nval = computeVal(_controller, y, height());
            
            unsigned tick = x - curPartTick;
            // Do not add events which are past the end of the part.
            if (tick >= curPartLen)
              break;
            MusECore::Event event(MusECore::Controller);
            event.setTick(tick);
            event.setA(_didx);
            if (type == MusECore::CTRL_PROGRAM)
            {
              if (lastpv == MusECore::CTRL_VAL_UNKNOWN)
              {
                if (MusEGlobal::song->mtype() == MT_GM)
                  event.setB(0xffff00 | (nval - 1));
                else  
                  event.setB(nval - 1);
              }
              else  
                event.setB((lastpv & 0xffff00) | (nval - 1));
            }
            else  
              event.setB(nval);
            
            // Do port controller values and clone parts. 
            operations.push_back(MusECore::UndoOp(MusECore::UndoOp::AddEvent, event, curPart, true, true));
            }
              
      MusEGlobal::song->applyOperationGroup(operations);
      }

//---------------------------------------------------------
//   changeValRamp
//---------------------------------------------------------

void CtrlCanvas::changeValRamp(int x1, int y1, int x2, int y2)
      {
      if(!curPart || !_controller)
        return;
      
      int h   = height();
      int type = _controller->num();

      MusECore::Undo operations;
      for (ciCEvent i = items.begin(); i != items.end(); ++i) {
            if ((*i)->contains(x1, x2)) {
                  CEvent* ev       = *i;
                  if (ev->part() != curPart)
                    continue;

                  MusECore::Event event = ev->event();
                  if (event.empty())
                        continue;

                  int x    = event.tick() + curPart->tick();
                  int y    = (x2==x1) ? y1 : (((y2-y1)*(x-x1))/(x2-x1))+y1;
                  int nval = computeVal(_controller, y, h);
                  if (type == MusECore::CTRL_PROGRAM)
                  {
                    if (event.dataB() == MusECore::CTRL_VAL_UNKNOWN)
                    {
                      --nval;
                      if(MusEGlobal::song->mtype() == MT_GM)
                        nval |= 0xffff00;
                    }
                    else  
                      nval = (event.dataB() & 0xffff00) | (nval - 1);
                  }
                    
                  ev->setVal(nval);
                  
                  if (type == MusECore::CTRL_VELOCITY) {
                        if ((event.velo() != nval)) {
                              MusECore::Event newEvent = event.clone();
                              newEvent.setVelo(nval);
                              ev->setEvent(newEvent);
                              // Do not do port controller values and clone parts. 
                              operations.push_back(MusECore::UndoOp(MusECore::UndoOp::ModifyEvent, newEvent, event, curPart, false, false));
                              }
                        }
                  else {
                        if (!event.empty()) {
                              if ((event.dataB() != nval)) {
                                    MusECore::Event newEvent = event.clone();
                                    newEvent.setB(nval);
                                    ev->setEvent(newEvent);
                                    // Do port controller values and clone parts. 
                                    operations.push_back(MusECore::UndoOp(MusECore::UndoOp::ModifyEvent, newEvent, event, curPart, true, true));
                                    }
                              }
                        }
                  }
            }

      MusEGlobal::song->applyOperationGroup(operations);
      }

//---------------------------------------------------------
//   changeVal
//---------------------------------------------------------

void CtrlCanvas::changeVal(int x1, int x2, int y)
      {
      if(!curPart || !_controller)         
        return;
      
      bool changed = false;
      int newval = computeVal(_controller, y, height());
      int type = _controller->num();

      for (ciCEvent i = items.begin(); i != items.end(); ++i) {
            if (!(*i)->contains(x1, x2))
                  continue;
            CEvent* ev       = *i;
            if(ev->part() != curPart)
              continue;
            MusECore::Event event      = ev->event();

            if (type == MusECore::CTRL_VELOCITY) {
                  if ((event.velo() != newval)) {
                        ev->setVal(newval);
                        MusECore::Event newEvent = event.clone();
                        newEvent.setVelo(newval);
                        ev->setEvent(newEvent);
                        // Indicate no undo, and do not do port controller values and clone parts. 
                        MusEGlobal::audio->msgChangeEvent(event, newEvent, curPart, false, false, false);
                        changed = true;
                        }
                  }
            else {
                  if (!event.empty()) {
                        int nval = newval;
                        if(type == MusECore::CTRL_PROGRAM)
                        {
                          if(event.dataB() == MusECore::CTRL_VAL_UNKNOWN)
                          {
                            --nval;
                            if(MusEGlobal::song->mtype() == MT_GM)
                              nval |= 0xffff00;
                          }
                          else  
                            nval = (event.dataB() & 0xffff00) | (nval - 1);
                        }
                        ev->setVal(nval);
                        
                        if ((event.dataB() != nval)) {
                              MusECore::Event newEvent = event.clone();
                              newEvent.setB(nval);
                              ev->setEvent(newEvent);
                              // Indicate no undo, and do port controller values and clone parts. 
                              MusEGlobal::audio->msgChangeEvent(event, newEvent, curPart, false, true, true);
                              changed = true;
                              }
                        }
                  }
            }
      if (changed)
            redraw();
      }

//---------------------------------------------------------
//   newVal
//---------------------------------------------------------

void CtrlCanvas::newVal(int x1, int y)
      {
      if(!curPart || !_controller)         
        return;
      
      int xx1  = editor->rasterVal1(x1);
      int xx2  = editor->rasterVal2(x1);
      // If x1 happens to lie directly on a raster, xx1 will equal xx2, 
      //  which is not good - there should always be a spread. Nudge by +1 and recompute.
      if(xx1 == xx2)
        xx2  = editor->rasterVal2(x1 + 1);
        
      int newval = computeVal(_controller, y, height());
      int type = _controller->num();

      bool found        = false;
      bool do_redraw = false;
      iCEvent ice_tmp;
      iCEvent prev_ev = items.end();     // End is OK with multi-part, this is just an end 'invalid' marker at first.
      iCEvent insertPoint = items.end(); // Similar case here. 
      bool curPartFound = false;

      int lastpv = MusECore::CTRL_VAL_UNKNOWN;
      if(ctrl)
        lastpv = ctrl->hwVal();
        
      int partTick = curPart->tick();
      for (iCEvent i = items.begin(); i != items.end() ; ) 
      {
            CEvent* ev = *i;
            if(ev->part() != curPart)
            {
              if(curPartFound)
              {
                // Make sure insertPoint doesn't lie beyond next part's first event.
                if(insertPoint == items.end())
                  insertPoint = i;
                // Speed-up: If the current part was already processed, we are done.
                break;
              }
              ++i;
              continue;
            }  
            curPartFound = true;
            
            MusECore::Event event = ev->event();
            if (event.empty())
            {
              prev_ev = i;
              ++i;
              continue;
            }      
            int ax = event.tick() + partTick;
            
            if (ax < xx1)
            {
              prev_ev = i;
              ++i;
              continue;
            }      
            if (ax >= xx2)
            {
              insertPoint = i;
              break;
            }
            
            int nval = newval;
            if(type == MusECore::CTRL_PROGRAM)
            {
              if(event.dataB() == MusECore::CTRL_VAL_UNKNOWN)
              {
                if(lastpv == MusECore::CTRL_VAL_UNKNOWN)
                {
                  --nval;
                  if(MusEGlobal::song->mtype() == MT_GM)
                    nval |= 0xffff00;
                }
                else  
                  nval = (lastpv & 0xffff00) | (nval - 1);
              }
              else  
                nval = (event.dataB() & 0xffff00) | (nval - 1);
            }
              
            if (ax == xx1) 
            {
              // change event
              found = true;
              ev->setVal(nval);
              if ((event.dataB() != nval)) 
              {
                MusECore::Event newEvent = event.clone();
                newEvent.setB(nval);
                
                ev->setEvent(newEvent);
                
                // Indicate no undo, and do port controller values and clone parts. 
                MusEGlobal::audio->msgChangeEvent(event, newEvent, curPart, false, true, true);
                
                do_redraw = true;      
              }
              prev_ev = i;
              ++i;
            }
            else if (ax < xx2) 
            {
                  // delete event
                  
                  deselectItem(ev);
                  // Indicate no undo, and do port controller values and clone parts. 
                  MusEGlobal::audio->msgDeleteEvent(event, curPart, false, true, true);
                  
                  delete (ev);
                  i = items.erase(i);           
                  ev = *i;
                  // Is there a previous item?
                  if(prev_ev != items.end())
                  {
                    // Have we reached the end of the list, or the end of the current part's items within the item list?
                    // Then it's current part's last drawn item. EX is 'infinity'. (As far as the part knows. Another part may overlay later.) 
                    // Else EX is current item's tick. (By now, ((*i)->event() should be valid - it must be not empty to call tick()).
                    (*prev_ev)->setEX(i == items.end() || ev->part() != curPart ? -1 : ev->event().tick()); 
                  }
                  
                  do_redraw = true;    // Let songChanged handle the redraw upon SC_SELECTION.
                  
                  prev_ev = i;
            }
      }
      
      if (!found) 
      {
            // new event
            int tick = xx1 - curPart->tick();
            // Do not add events which are past the end of the part.
            if((unsigned)tick < curPart->lenTick())
            {
              MusECore::Event event(MusECore::Controller);
              event.setTick(tick);
              event.setA(_didx);
              if(type == MusECore::CTRL_PROGRAM)
              {
                if(lastpv == MusECore::CTRL_VAL_UNKNOWN)
                {
                  if(MusEGlobal::song->mtype() == MT_GM)
                    event.setB(0xffff00 | (newval - 1));
                  else  
                    event.setB(newval - 1);
                }
                else    
                  event.setB((lastpv & 0xffff00) | (newval - 1));
              }
              else  
                event.setB(newval);
              
              // Indicate no undo, and do port controller values and clone parts. 
              MusEGlobal::audio->msgAddEvent(event, curPart, false, true, true);
              
              CEvent* newev = new CEvent(event, curPart, event.dataB());
              insertPoint = items.insert(insertPoint, newev);
              //selectItem(newev); // TODO: There are advantages, but do we really want new items to be selected?
              
              if(insertPoint != items.begin())
              {
                ice_tmp = insertPoint;
                --ice_tmp;
                (*ice_tmp)->setEX(tick);
              }  
              ice_tmp = insertPoint;
              ++ice_tmp;
              (*insertPoint)->setEX(ice_tmp == items.end() || 
                                    (*ice_tmp)->part() != curPart ? 
                                    -1 : (*ice_tmp)->event().tick());
              do_redraw = true;              
            }  
      }
      
      if(do_redraw)                 // Let songChanged handle the redraw upon SC_SELECTION.
        redraw();
      }

//---------------------------------------------------------
//   newVal
//---------------------------------------------------------

void CtrlCanvas::newVal(int x1, int y1, int x2, int y2)
      {
      if(!curPart || !_controller)         
        return;
      
      if(x2 - x1 < 0)
      {
        int a = x1;
        x1 = x2;
        x2 = a;
        a = y1;
        y1 = y2;
        y2 = a;
      }
      
      // I wanted to avoid redundantly drawing the first x position since it is already done via mouseDown.
      // But with the raster 'off' there were some problems not catching some items, and jagged drawing.
      // Possibly there are slight differences between rasterVal1() and rasterVal2() - not reciprocals of each other? 
      // Also the loops below were getting complicated to ignore that first position.
      // So I draw from the first x. (TODO The idea may work now since I wrote this - more work was done.) p4.0.18 Tim.
      
      int xx1 = editor->rasterVal1(x1);
      
      int xx2 = editor->rasterVal2(x2);
      
      // If x1 and x2 happen to lie directly on the same raster, xx1 will equal xx2, 
      //  which is not good - there should always be a spread. Nudge by +1 and recompute.
      if(xx1 == xx2)
        xx2  = editor->rasterVal2(x2 + 1);
      
      int type = _controller->num();

      bool useRaster = false;
      int raster = editor->raster();
      if (raster == 1)          // set reasonable raster
      {
        raster = MusEGlobal::config.division/16;  // Let's use 64th notes, for a bit finer resolution. Tim.
        useRaster = true;
      }  

      bool do_redraw = false;
      
      // delete existing events

      iCEvent prev_ev = items.end();     // End is OK with multi-part, this is just an end 'invalid' marker at first.
      iCEvent insertPoint = items.end(); // Similar case here. 
      iCEvent ice_tmp;
      bool curPartFound = false;
      int lastpv = MusECore::CTRL_VAL_UNKNOWN;
      int partTick = curPart->tick();
      for (iCEvent i = items.begin(); i != items.end() ; ) 
      {
            CEvent* ev = *i;
            if(ev->part() != curPart)
            {
              if(curPartFound)
              {
                // Make sure insertPoint doesn't lie beyond next part's first event.
                if(insertPoint == items.end())
                  insertPoint = i;
                // Speed-up: If the current part was already processed, we are done.
                break;
              }
              ++i;
              continue;
            }  
            curPartFound = true;
            
            MusECore::Event event = ev->event();
            if (event.empty())
            {
              prev_ev = i;
              ++i;
              continue;
            }      
            int x = event.tick() + partTick;
            
            if (x < xx1)
            {
              prev_ev = i;
              ++i;
              continue;
            }      
            if (x >= xx2)
            {
              insertPoint = i;
              break;
            }
            
            deselectItem(ev);
            // Indicate no undo, and do port controller values and clone parts. 
            MusEGlobal::audio->msgDeleteEvent(event, curPart, false, true, true);
            
            delete (ev);
            i = items.erase(i);           
            ev = *i;
            // Is there a previous item?
            if(prev_ev != items.end())
            {
              // Have we reached the end of the list, or the end of the current part's items within the item list?
              // Then it's current part's last drawn item. EX is 'infinity'. (As far as the part knows. Another part may overlay later.) 
              // Else EX is current item's tick. (By now, ((*i)->event() should be valid - it must be not empty to call tick()).
              (*prev_ev)->setEX(i == items.end() || ev->part() != curPart ? -1 : ev->event().tick()); 
            }
            
            do_redraw = true;    // Let songChanged handle the redraw upon SC_SELECTION.
            
            prev_ev = i;
      }

      if(ctrl)  
        lastpv = ctrl->hwVal();
        
      // insert new events
      for (int x = xx1, step; x < xx2 ; x += step )    
      {
            step = useRaster ? raster : editor->rasterVal2(x + 1) - x;
            
            int y    = x + step >= xx2 || x2 == x1 ? y2 : (x == xx1 ? y1 : (((y2 - y1) * (x + step/2 - x1)) / (x2 - x1)) + y1);
            int nval = computeVal(_controller, y, height());
            
            int tick = x - partTick;
            // Do not add events which are past the end of the part.
            if((unsigned)tick >= curPart->lenTick())
              break;
            MusECore::Event event(MusECore::Controller);
            event.setTick(tick);
            event.setA(_didx);
            if(type == MusECore::CTRL_PROGRAM)
            {
              if(lastpv == MusECore::CTRL_VAL_UNKNOWN)
              {
                if(MusEGlobal::song->mtype() == MT_GM)
                  event.setB(0xffff00 | (nval - 1));
                else  
                  event.setB(nval - 1);
              }
              else  
                event.setB((lastpv & 0xffff00) | (nval - 1));
            }
            else  
              event.setB(nval);
            
            // Indicate no undo, and do port controller values and clone parts. 
            MusEGlobal::audio->msgAddEvent(event, curPart, false, true, true);
            
            CEvent* newev = new CEvent(event, curPart, event.dataB());
            insertPoint = items.insert(insertPoint, newev);
            //selectItem(newev); // TODO: There are advantages, but do we really want new items to be selected?
            
            if(insertPoint != items.begin())
            {
              ice_tmp = insertPoint;
              --ice_tmp;
              (*ice_tmp)->setEX(tick);
            }  
            ice_tmp = insertPoint;
            ++ice_tmp;
            (*insertPoint)->setEX(ice_tmp == items.end() || 
                                  (*ice_tmp)->part() != curPart ? 
                                  -1 : (*ice_tmp)->event().tick());
            ++insertPoint;
            do_redraw = true;
            }
              
      if(do_redraw)                 //
        redraw();
      }

//---------------------------------------------------------
//   deleteVal
//---------------------------------------------------------

void CtrlCanvas::deleteVal(int x1, int x2, int)
      {
      if(!curPart)         
        return;
      
      if(x2 - x1 < 0)
      {
        int a = x1;
        x1 = x2;
        x2 = a;
      }
      
      int xx1 = editor->rasterVal1(x1);
      int xx2 = editor->rasterVal2(x2);
      // If x1 and x2 happen to lie directly on the same raster, xx1 will equal xx2, 
      //  which is not good - there should always be a spread. Nudge by +1 and recompute.
      if(xx1 == xx2)
        xx2  = editor->rasterVal2(x2 + 1);

      int partTick = curPart->tick();
      xx1 -= partTick;
      xx2 -= partTick;

      iCEvent prev_ev = items.end();
      bool curPartFound = false;
      bool do_redraw = false;
      
      for (iCEvent i = items.begin(); i != items.end() ;) 
      {
            CEvent* ev = *i;
            if(ev->part() != curPart)
            {
              // Speed-up: If the current part was already processed, we are done.
              if(curPartFound)
                break;
              ++i;
              continue;
            }
            curPartFound = true;
            
            MusECore::Event event = ev->event();
            if (event.empty())
            {
              prev_ev = i;
              ++i;
              continue;
            }      
            int x = event.tick();
            if (x < xx1)
            {
              prev_ev = i;
              ++i;
              continue;
            }      
            if (x >= xx2)
              break;
            
            deselectItem(ev);
            // Indicate no undo, and do port controller values and clone parts. 
            MusEGlobal::audio->msgDeleteEvent(event, curPart, false, true, true);
            
            delete (ev);
            i = items.erase(i);           
            ev = *i;
            // Is there a previous item?
            if(prev_ev != items.end())
            {
              // Have we reached the end of the list, or the end of the current part's items within the item list?
              // Then it's current part's last drawn item. EX is 'infinity'. (As far as the part knows. Another part may overlay later.) 
              // Else EX is current item's tick. (By now, ((*i)->event() should be valid - it must be not empty to call tick()).
              (*prev_ev)->setEX(i == items.end() || ev->part() != curPart ? -1 : ev->event().tick()); 
            }
            do_redraw = true;              
            
            prev_ev = i;
      }
            
      if(do_redraw)
        redraw();                 // Let songChanged handle the redraw upon SC_SELECTION.  
      }

//---------------------------------------------------------
//   setTool
//---------------------------------------------------------

void CtrlCanvas::setTool(int t)
      {
      if (tool == MusEGui::Tool(t))
            return;
      tool = MusEGui::Tool(t);
      switch(tool) {
            case MusEGui::PencilTool:
                  setCursor(QCursor(*pencilIcon, 4, 15));
                  break;
            case MusEGui::DrawTool:
                  drawLineMode = false;
                  break;
            default:
                  setCursor(QCursor(Qt::ArrowCursor));
                  break;
            }
      }

//---------------------------------------------------------
//   pdrawItems
//---------------------------------------------------------

void CtrlCanvas::pdrawItems(QPainter& p, const QRect& rect, const MusECore::MidiPart* part, bool velo, bool fg)
{
  int x = rect.x() - 1;   // compensate for 3 pixel line width
  int w = rect.width() + 2;
  int wh = height();
  
  noEvents=true;

  if(velo) 
  {
    noEvents=false;
    for(iCEvent i = items.begin(); i != items.end(); ++i) 
    {
      CEvent* e = *i;
      // Draw selected part velocity events on top of unselected part events.
      if(e->part() != part)
        continue;
      MusECore::Event event = e->event();
      int tick = mapx(event.tick() + e->part()->tick());
      if (tick <= x)
            continue;
      if (tick > x+w)
            break;
      int y1 = wh - (e->val() * wh / 128);
      // fg means 'draw selected parts'.
      if(fg)
      {
        if(e->selected())
          p.setPen(QPen(Qt::blue, 3));
        else
          p.setPen(QPen(MusEGlobal::config.ctrlGraphFg, 3));
      }  
      else  
        p.setPen(QPen(Qt::darkGray, 3));
      p.drawLine(tick, wh, tick, y1);
    }
  }
  else
  {
    if(!part)         
      return;
    
    MusECore::MidiTrack* mt = part->track();
    MusECore::MidiPort* mp;
    
    if((mt->type() == MusECore::Track::DRUM) && (curDrumInstrument != -1) && ((_cnum & 0xff) == 0xff)) 
      mp = &MusEGlobal::midiPorts[MusEGlobal::drumMap[curDrumInstrument].port];          
    else
      mp = &MusEGlobal::midiPorts[mt->outPort()];          
    
    MusECore::MidiController* mc = mp->midiController(_cnum);
    
    int min;
    int max;
    int bias;
    if(_cnum == MusECore::CTRL_PROGRAM)
    {
      min = 1;
      max = 128;
      bias = 0;
    }
    else
    {
      min  = mc->minVal();
      max  = mc->maxVal();
      bias  = mc->bias();
    }
    int x1   = rect.x();
    int lval = MusECore::CTRL_VAL_UNKNOWN;
    bool selected = false;
    for (iCEvent i = items.begin(); i != items.end(); ++i) 
    {
      noEvents=false;
      CEvent* e = *i;
      // Draw unselected part controller events (lines) on top of selected part events (bars).
      if(e->part() != part)
      {  
        continue;
      }
      MusECore::Event ev = e->event();
      int tick = mapx(!ev.empty() ? ev.tick() + e->part()->tick() : 0);
      int val = e->val();
      int pval = val;
      if(_cnum == MusECore::CTRL_PROGRAM)
      {
        if((val & 0xff) == 0xff)
          // What to do here? prog = 0xff should not be allowed, but may still be encountered.
          pval = 1;
        else  
          pval = (val & 0x7f) + 1;
      }
      if (tick <= x) {
            if (val == MusECore::CTRL_VAL_UNKNOWN)
                  lval = MusECore::CTRL_VAL_UNKNOWN;
            else
            {
                  if(_cnum == MusECore::CTRL_PROGRAM)
                    lval = wh - ((pval - min - bias) * wh / (max - min));
                  else  
                    lval = wh - ((val - min - bias) * wh / (max - min));
            }
            selected = e->selected();     
            continue;
            }
      if (tick > x+w)
            break;
      if (lval == MusECore::CTRL_VAL_UNKNOWN)
      {
        // fg means 'draw unselected parts'.
        if(!fg)
          p.fillRect(x1, 0, tick - x1, wh, Qt::darkGray);
      }
      else
      {
        if(fg)
        {  
          p.setPen(Qt::gray);
          p.drawLine(x1, lval, tick, lval);
        }  
        else
          p.fillRect(x1, lval, tick - x1, wh - lval, selected ? Qt::blue : MusEGlobal::config.ctrlGraphFg);
      }
      
      
      x1 = tick;
      if (val == MusECore::CTRL_VAL_UNKNOWN)
            lval = MusECore::CTRL_VAL_UNKNOWN;
      else
      {
            if(_cnum == MusECore::CTRL_PROGRAM)
              lval = wh - ((pval - min - bias) * wh / (max - min));
            else  
              lval = wh - ((val - min - bias) * wh / (max - min));
      } 
      selected = e->selected();     
    }
    if (lval == MusECore::CTRL_VAL_UNKNOWN)
    {
      if(!fg) {
        p.fillRect(x1, 0, (x+w) - x1, wh, Qt::darkGray);
      }
    }
    else
    {
      if(fg)
      {  
        p.setPen(Qt::gray);
        p.drawLine(x1, lval, x + w, lval);
      }  
      else
        p.fillRect(x1, lval, (x+w) - x1, wh - lval, selected ? Qt::blue : MusEGlobal::config.ctrlGraphFg);
    }
  }       
}

//---------------------------------------------------------
//   pdraw
//---------------------------------------------------------

void CtrlCanvas::pdraw(QPainter& p, const QRect& rect)
      {
      if(!_controller)   
        return;
       
      int x = rect.x() - 1;   // compensate for 3 pixel line width
      int y = rect.y();
      int w = rect.width() + 2;
      int h = rect.height();
      
      //---------------------------------------------------
      // draw Canvas Items
      //---------------------------------------------------

      bool velo = (MusECore::midiControllerType(_controller->num()) == MusECore::MidiController::Velo);
      if(velo) 
      {
        //---------------------------------------------------
        // draw the grid and markers now - before velocity items
        //---------------------------------------------------
        p.save();
        View::pdraw(p, rect);
        p.restore();
        
        int xp = mapx(pos[0]);
        if (xp >= x && xp < x+w) {
              p.setPen(Qt::red);
              p.drawLine(xp, y, xp, y+h);
              }
        xp = mapx(pos[1]);
        if (xp >= x && xp < x+w) {
              p.setPen(Qt::blue);
              p.drawLine(xp, y, xp, y+h);
              }
        xp = mapx(pos[2]);
        if (xp >= x && xp < x+w) {
              p.setPen(Qt::blue);
              p.drawLine(xp, y, xp, y+h);
              }
      }  
      else
        // Draw non-fg non-velocity items for the current part
        pdrawItems(p, rect, curPart, false, false);
        
      for(MusECore::iPart ip = editor->parts()->begin(); ip != editor->parts()->end(); ++ip) 
      {
        MusECore::MidiPart* part = (MusECore::MidiPart*)(ip->second);
        if(part == curPart)
          continue;
        // Draw items for all parts - other than current part
        pdrawItems(p, rect, part, velo, !velo);
      }
      if(velo) 
      {
        // Draw fg velocity items for the current part
        pdrawItems(p, rect, curPart, true, true);
      }
      else
      {
        //---------------------------------------------------
        // draw the grid and markers now - after non-velocity items
        //---------------------------------------------------
        p.save();
        View::pdraw(p, rect);
        p.restore();
        
        int xp = mapx(pos[0]);
        if (xp >= x && xp < x+w) {
              p.setPen(Qt::red);
              p.drawLine(xp, y, xp, y+h);
              }
        xp = mapx(pos[1]);
        if (xp >= x && xp < x+w) {
              p.setPen(Qt::blue);
              p.drawLine(xp, y, xp, y+h);
              }
        xp = mapx(pos[2]);
        if (xp >= x && xp < x+w) {
              p.setPen(Qt::blue);
              p.drawLine(xp, y, xp, y+h);
              }
      }
      
      //---------------------------------------------------
      //    draw lasso
      //---------------------------------------------------

      if (drag == DRAG_LASSO) {
            setPainter(p);
            p.setPen(Qt::blue);
            p.setBrush(Qt::NoBrush);
            p.drawRect(lasso);
            }
      }

//---------------------------------------------------------
//   drawOverlay
//---------------------------------------------------------

void CtrlCanvas::drawOverlay(QPainter& p)
      {
      QString s(_controller ? _controller->name() : QString(""));
      
      //p.setFont(MusEGlobal::config.fonts[3]);  // Use widget font instead. 
      p.setFont(font());
      
      p.setPen(Qt::black);
      
      //QFontMetrics fm(MusEGlobal::config.fonts[3]);  // Use widget font metrics instead. 
      //int y = fm.lineSpacing() + 2;
      int y = fontMetrics().lineSpacing() + 2;
      
      p.drawText(2, y, s);
      if (noEvents)
           p.drawText(2 , y * 2, tr("Drawing hint: Hold Ctrl to affect only existing events"));
      }

//---------------------------------------------------------
//   overlayRect
//    returns geometry of overlay rectangle
//---------------------------------------------------------

QRect CtrlCanvas::overlayRect() const
{
      //QFontMetrics fm(MusEGlobal::config.fonts[3]);   // Use widget font metrics instead (and set a widget font) !!! 
      QFontMetrics fm(fontMetrics());
      QRect r(fm.boundingRect(_controller ? _controller->name() : QString("")));
      
      int y = fm.lineSpacing() + 2;
      r.translate(2, y);   
      if (noEvents) 
      {
        QRect r2(fm.boundingRect(QString(tr("Use shift + pencil or line tool to draw new events"))));
        //r2.translate(width()/2-100, height()/2-10);   
        r2.translate(2, y * 2);   
        r |= r2;
      }
      return r;
}

//---------------------------------------------------------
//   draw
//---------------------------------------------------------

void CtrlCanvas::draw(QPainter& p, const QRect& rect)
      {
      drawTickRaster(p, rect.x(), rect.y(),
         rect.width(), rect.height(), editor->raster());

      //---------------------------------------------------
      //    draw line tool
      //---------------------------------------------------

      if ((tool == MusEGui::DrawTool) && drawLineMode) {
            p.setPen(Qt::black);
            p.drawLine(line1x, line1y, line2x, line2y);
            }
      }

//---------------------------------------------------------
//   setCurDrumInstrument
//---------------------------------------------------------

void CtrlCanvas::setCurDrumInstrument(int di)
      {
      curDrumInstrument = di;
      // DELETETHIS
      //printf("CtrlCanvas::setCurDrumInstrument curDrumInstrument:%d\n", curDrumInstrument);
      
      //
      //  check if current controller is only valid for
      //  a specific drum instrument
      //
      // Removed by T356.
      //if(curTrack && (curTrack->type() == MusECore::Track::DRUM) && ((_controller->num() & 0xff) == 0xff)) {
      //if(curTrack && (curTrack->type() == MusECore::Track::DRUM) && ((_cnum & 0xff) == 0xff)) {
            // reset to default
            // TODO: check, if new drum instrument has a similar controller
            //    configured
      //      _cnum = MusECore::CTRL_VELOCITY;
      //      }
      // Removed by T356
      //songChanged(-1);
      }

} // namespace MusEGui
