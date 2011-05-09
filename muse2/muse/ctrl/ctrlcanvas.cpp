//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: ctrlcanvas.cpp,v 1.15.2.10 2009/11/14 03:37:48 terminator356 Exp $
//  (C) Copyright 1999 Werner Schweer (ws@seh.de)
//=========================================================

#include <stdio.h>
#include <values.h>

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

extern void drawTickRaster(QPainter& p, int x, int y,
   int w, int h, int quant);

static MidiCtrlValList veloList(CTRL_VELOCITY);    // dummy

//---------------------------------------------------------
//   computeVal
//---------------------------------------------------------

static int computeVal(MidiController* mc, int y, int height)
      {
      int min; int max;
      if(mc->num() == CTRL_PROGRAM)
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
      if(mc->num() != CTRL_PROGRAM)
        val += mc->bias();
      return val;
      }

//---------------------------------------------------------
//   computeY
//---------------------------------------------------------

static int computeY(const MidiController* mc, int val, int height)
      {
      int min; int max; 
      if(mc->num() == CTRL_PROGRAM)
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
      
      if(mc->num() != CTRL_PROGRAM)
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

CEvent::CEvent(Event e, MidiPart* pt, int v)
      {
      _event = e;
      //_state = Normal;
      _part  = pt;
      _val   = v;
      ex     = !e.empty() ? e.tick() : 0;
      }

//---------------------------------------------------------
//   intersects
//---------------------------------------------------------

bool CEvent::intersects(const MidiController* mc, const QRect& r, const int tickstep, const int wh) const
{
      if(_event.empty())
        return false;
      //int y1 = wh - (_val * wh / 128);
      int y1 = computeY(mc, _val, wh);
      
      int tick1 = _event.tick() + _part->tick();
      if(ex == -1)
      {
        //printf("ex:-1 tick1:%d y1:%d r.x:%d r.y:%d r.w:%d r.h:%d\n", tick1, y1, r.x(), r.y(), r.width(), r.height()); 
        //return tick1 < (r.x() + r.width()) && y1 >= r.y() && y1 < (r.y() + r.height());
        return tick1 < (r.x() + r.width()) && y1 < (r.y() + r.height());
      }
      
      int tick2 = ex + _part->tick();
      
      // Velocities don't use EX (set equal to event tick, giving zero width here),
      //  and velocities are drawn three pixels wide, so adjust the width now. 
      // Remember, each drawn pixel represents one tickstep which varies with zoom.
      // So that's 3 x tickstep for each velocity line.
      // Hmm, actually, for better pin-point accuracy just use one tickstep for now.
      if(midiControllerType(mc->num()) == MidiController::Velo)
      {
        //tick1 -= tickstep;
        //if(tick1 <= 0)
        //  tick1 = 0;  
        //tick2 += 2 * tickstep;
        
        tick2 += tickstep;
      }
      
      QRect er(tick1, y1, tick2 - tick1, wh - y1);   
      //printf("t1:%d t2:%d ex:%d r.x:%d r.y:%d r.w:%d r.h:%d  er.x:%d er.y:%d er.w:%d er.h:%d\n", 
      //        tick1, tick2, ex, r.x(), r.y(), r.width(), r.height(), er.x(), er.y(), er.width(), er.height()); 
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
         //|| (tick2 >= x1 && tick2 < x2)
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
      setFont(config.fonts[3]);  
      editor = e;
      drag   = DRAG_OFF;
      tool   = PointerTool;
      pos[0] = 0;
      pos[1] = 0;
      pos[2] = 0;
      noEvents=false;
      //_isFirstMove = true;
      //_lastDelta = QPoint(0, 0);

      ctrl   = &veloList;
      _controller = &veloCtrl;
      _panel = pnl;
      _cnum  = CTRL_VELOCITY;    
      _dnum  = CTRL_VELOCITY;    
      _didx  = CTRL_VELOCITY;      
      connect(song, SIGNAL(posChanged(int, unsigned, bool)), this, SLOT(setPos(int, unsigned, bool)));

      setMouseTracking(true);
      if (editor->parts()->empty()) {
            curPart = 0;
            curTrack = 0;
            }
      else {
            setCurTrackAndPart();
            }
      connect(song, SIGNAL(songChanged(int)), SLOT(songChanged(int)));
      connect(muse, SIGNAL(configChanged()), SLOT(configChanged()));
      
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
            switch (song->follow()) {
                  case  Song::NO:
                        break;
                  case Song::JUMP:
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
                  case Song::CONTINUOUS:
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
        if(_cnum == CTRL_VELOCITY)              
          _panel->setHWController(curTrack, &veloCtrl);
        else 
          _panel->setHWController(curTrack, _controller);
      }  
    }

//---------------------------------------------------------
//   leaveEvent
//---------------------------------------------------------

void CtrlCanvas::leaveEvent(QEvent*)
      {
      emit xposChanged(MAXINT);
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
        {
            //(*i)->setState(CEvent::Normal);
            //if(!(*i)->event().empty())
            //  (*i)->event().setSelected(false);
            (*i)->setSelected(false);
        }    
        selection.clear();
        ///update();
      }

//---------------------------------------------------------
//   selectItem
//---------------------------------------------------------

void CtrlCanvas::selectItem(CEvent* e)
      {
      //e->setState(CEvent::Selected);
      //if(!e->event().empty())
      //  e->event().setSelected(true);
      e->setSelected(true);
      selection.push_back(e);
      ///update();
      }

//---------------------------------------------------------
//   deselectItem
//---------------------------------------------------------

void CtrlCanvas::deselectItem(CEvent* e)
      {
      //e->setState(CEvent::Normal);
      //if(!e->event().empty())
      //  e->event().setSelected(false);
      e->setSelected(false);
      for (iCEvent i = selection.begin(); i != selection.end(); ++i) {
            if (*i == e) {
                  selection.erase(i);
                  break;
                  }
            }
      ///update();
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
  MidiPart* part = 0;
  MidiTrack* track = 0;
  
  if(!editor->parts()->empty()) 
  {
    Part* pt = editor->curCanvasPart();
    if(pt && pt->track())
    {
      if(pt->track()->isMidiTrack())
      {
        part = (MidiPart*)pt;
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
  //printf("CtrlCanvas::songChanged type:%x\n", type);  
  // Is it simply a midi controller value adjustment? Forget it.
  if(type == SC_MIDI_CONTROLLER)
    return;
            
  if(type & SC_CONFIG)
    setFont(config.fonts[3]);  
  
  bool changed = false;
  if(type & (SC_CONFIG | SC_PART_MODIFIED | SC_SELECTION))
    changed = setCurTrackAndPart();
            
  // Although changing the instrument/device in the
  //  config window generates a type of -1, we can eliminate
  //  some other useless calls using SC_CONFIG, which was not used 
  //  anywhere else in muse before now, except song header.
  if((type & (SC_CONFIG | SC_DRUMMAP)) || ((type & (SC_PART_MODIFIED | SC_SELECTION)) && changed))
  {
    setMidiController(_cnum);
    //return;
  }
  
  if(type & (SC_CONFIG | SC_DRUMMAP | SC_PART_MODIFIED | SC_EVENT_INSERTED | SC_EVENT_REMOVED | SC_EVENT_MODIFIED))   // p4.0.18
    updateItems();
  else
  if(type & SC_SELECTION)
    updateSelections();               // p4.0.18
}

//---------------------------------------------------------
//   partControllers
//---------------------------------------------------------

void CtrlCanvas::partControllers(const MidiPart* part, int num, int* dnum, int* didx, MidiController** mc, MidiCtrlValList** mcvl)
{
  if(num == CTRL_VELOCITY) // special case
  {    
    if(mcvl)
      *mcvl = &veloList;
    if(mc)
      *mc = &veloCtrl;
    if(dnum)
      *dnum = num;
    if(didx)
      *didx = num;
  }
  else 
  {
    MidiTrack* mt = part->track();
    MidiPort* mp;
    int di;
    int n;
    
    if((mt->type() != Track::DRUM) && curDrumInstrument != -1)
      printf("keyfilter != -1 in non drum track?\n");

    if((mt->type() == Track::DRUM) && (curDrumInstrument != -1) && ((num & 0xff) == 0xff)) 
    {
      di = (num & ~0xff) | curDrumInstrument;
      n = (num & ~0xff) | drumMap[curDrumInstrument].anote;  // construct real controller number
      //num = (num & ~0xff) | curDrumInstrument);  // construct real controller number
      mp = &midiPorts[drumMap[curDrumInstrument].port];          
    }
    else
    {
       di = num;
       n = num;
       mp = &midiPorts[mt->outPort()];          
    }
    
    if(dnum)
      *dnum = n;
          
    if(didx)
      *didx = di;
      
    if(mc)
      *mc = mp->midiController(n);
      
    if(mcvl)
    {
      MidiCtrlValList* tmcvl = 0;
      MidiCtrlValListList* cvll = mp->controller();
      for(iMidiCtrlValList i = cvll->begin(); i != cvll->end(); ++i) 
      {
        if(i->second->num() == n) 
        {
          tmcvl = i->second;
          break;
        }
      }
      *mcvl = tmcvl;
      
      // Removed by T356.
      // MidiCtrlValList not found is now an acceptable state (for multiple part editing).
      //if (i == cvll->end()) {
      //      printf("CtrlCanvas::setController(0x%x): not found\n", num);
      //      for (i = cvll->begin(); i != cvll->end(); ++i)
      //            printf("  0x%x\n", i->second->num());
      //      return;
      //      }
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
      
      /*
      if(ctrl)
      {
        for(ciMidiCtrlVal imcv = ctrl->begin(); imcv != ctrl->end(); ++imcv)
        {
          MidiPart* part = (MidiPart*)imcv->part;
          int val = imcv->val;
          
          bool edpart = false;
          if(editor->parts()->index(part) != -1)
            edpart = true;
          
          MidiController* mc;
          MidiCtrlValList* mcvl;
          partControllers(part, _cnum, 0, 0, &mc, &mcvl);

          Event e(Controller);
          
          if(_cnum == CTRL_VELOCITY && e.type() == Note)
          {
            items.add(new CEvent(e, part, e.velo()));
           
          }
          
        }
      }
      */
      
      /*
      MidiTrackList* mtl = song->midis();
      for(ciMidiTrack imt = mtl->begin(); imt != mtl->end(); ++imt)
      {
        //MidiTrack* mt = *imt;
        PartList* pl = (*imt)->parts();
        for(ciPart p = pl->begin(); p != pl->end(); ++p) 
        {
          MidiPart* part = (MidiPart*)(p->second);
          
          bool edpart = false;
          if(editor->parts()->index(part) != -1)
            edpart = true;
          
          EventList* el = part->events();
          MidiController* mc;
          MidiCtrlValList* mcvl;
          partControllers(part, _cnum, 0, 0, &mc, &mcvl);

          for(iEvent i = el->begin(); i != el->end(); ++i) 
          {
            Event e = i->second;
            if(_cnum == CTRL_VELOCITY && e.type() == Note) 
            {
              if(curDrumInstrument == -1) 
              {
                    items.add(new CEvent(e, part, e.velo()));
              }
              else if (e.dataA() == curDrumInstrument) //same note
                    items.add(new CEvent(e, part, e.velo()));
            }
            else if (e.type() == Controller && e.dataA() == _didx) 
            {
              if(mcvl && last.empty()) 
              {
                    Event le(Controller);
                    //le.setType(Controller);
                    le.setA(_didx);
                    //le.setB(e.dataB());
                    le.setB(CTRL_VAL_UNKNOWN);
                    //lastce = new CEvent(Event(), part, mcvl->value(part->tick(), part));
                    //lastce = new CEvent(le, part, mcvl->value(part->tick(), part));
                    lastce = new CEvent(le, part, mcvl->value(part->tick()));
                    items.add(lastce);
              }
              if (lastce)
                    lastce->setEX(e.tick());
              lastce = new CEvent(e, part, e.dataB());
              items.add(lastce);
              last = e;
            }    
          }
        }
      }
      */
      
      
      
      
      
      if(!editor->parts()->empty())
      {
        //Event last;
        //CEvent* lastce  = 0;
        CEvent *newev = 0;
  
        for (iPart p = editor->parts()->begin(); p != editor->parts()->end(); ++p) 
        {
              Event last;
              CEvent* lastce  = 0;
              
              MidiPart* part = (MidiPart*)(p->second);
              EventList* el = part->events();
              MidiController* mc;
              MidiCtrlValList* mcvl;
              partControllers(part, _cnum, 0, 0, &mc, &mcvl);
              unsigned len = part->lenTick();

              for (iEvent i = el->begin(); i != el->end(); ++i) 
              {
                    Event e = i->second;
                    // Added by T356. Do not add events which are past the end of the part.
                    if(e.tick() >= len)
                      break;
                    
                    if(_cnum == CTRL_VELOCITY && e.type() == Note) 
                    {
                          //printf("CtrlCanvas::updateItems CTRL_VELOCITY curDrumInstrument:%d\n", curDrumInstrument);
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
                    else if (e.type() == Controller && e.dataA() == _didx) 
                    {
                          if(mcvl && last.empty()) 
                          {
                                lastce = new CEvent(Event(), part, mcvl->value(part->tick()));
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
      start = event->pos();
      Tool activeTool = tool;
      
      bool ctrlKey = event->modifiers() & Qt::ControlModifier;
      int xpos = start.x();
      int ypos = start.y();

      MidiController::ControllerType type = midiControllerType(_controller->num());

      switch (activeTool) {
            case PointerTool:
                  drag = DRAG_LASSO_START;
                  
                  {
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
                      Event event = ev->event();
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
                      //song->update(SC_SELECTION); //
                  }
                  
                  
                  break;

            case PencilTool:
                  if (ctrlKey) {
                        if (type != MidiController::Velo) {
                              drag = DRAG_NEW;
                              song->startUndo();
                              ///newVal(xpos, xpos, ypos);
                              newVal(xpos, ypos);
                              }
                        }
                  else {
                        drag = DRAG_RESIZE;
                        song->startUndo();
                        changeVal(xpos, xpos, ypos);
                        }
                  break;

            case RubberTool:
                  if (type != MidiController::Velo) {
                        drag = DRAG_DELETE;
                        song->startUndo();
                        deleteVal(xpos, xpos, ypos);
                        }
                  break;

            case DrawTool:
                  if (drawLineMode) {
                        line2x = xpos;
                        line2y = ypos;
                        if (ctrlKey)
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
      QPoint pos  = event->pos();
      QPoint dist = pos - start;
      bool moving = dist.y() >= 3 || dist.y() <= 3 || dist.x() >= 3 || dist.x() <= 3;
      switch (drag) {
            case DRAG_LASSO_START:
                  if (!moving)
                        break;
                  drag = DRAG_LASSO;
                  // weiter mit DRAG_LASSO:
            case DRAG_LASSO:
                  lasso.setRect(start.x(), start.y(), dist.x(), dist.y());
                  redraw();
                  break;
            case DRAG_RESIZE:
                  changeVal(start.x(), pos.x(), pos.y());
                  start = pos;
                  break;

            case DRAG_NEW:
                  ///newVal(start.x(), pos.x(), pos.y());
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
      if (tool == DrawTool && drawLineMode) {
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
            ///case DRAG_RESIZE:
            ///case DRAG_NEW:
            ///case DRAG_DELETE:
                  ///song->endUndo(SC_EVENT_MODIFIED | SC_EVENT_INSERTED);
                  ///break;
            case DRAG_RESIZE:
                  song->endUndo(SC_EVENT_MODIFIED);
                  break;
            case DRAG_NEW:
                  song->endUndo(SC_EVENT_MODIFIED | SC_EVENT_INSERTED | SC_EVENT_REMOVED);
                  break;
            case DRAG_DELETE:
                  song->endUndo(SC_EVENT_REMOVED);
                  break;

            case DRAG_LASSO_START:
                  lasso.setRect(-1, -1, -1, -1);

            case DRAG_LASSO:
                  {
                    ///if (!ctrlKey)
                    ///      deselectAll();
                    lasso = lasso.normalized();
                    int h = height();
                    //bool do_redraw = false;
                    int tickstep = rmapxDev(1);
                    for (iCEvent i = items.begin(); i != items.end(); ++i) {
                          if((*i)->part() != curPart)
                            continue;
                          if ((*i)->intersects(_controller, lasso, tickstep, h)) {
                                if (ctrlKey && (*i)->selected())
                                {
                                    //if (!ctrlKey)         // ctrlKey p4.0.18
                                    {
                                      ///deselectItem(*i);
                                      //do_redraw = true;
                                      (*i)->setSelected(false);
                                    }  
                                }
                                else
                                {
                                      ///selectItem(*i);
                                      //do_redraw = true;
                                      (*i)->setSelected(true);
                                }      
                              }  
                          }
                    drag = DRAG_OFF;
                    //if(do_redraw)
                    //  redraw();                 // Let songChanged handle the redraw upon SC_SELECTION.
                      song->update(SC_SELECTION); //
                    //else
                    //  redraw();  
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
        //raster = config.division/4;
        raster = config.division/16;  // Let's use 64th notes, for a bit finer resolution. p4.0.18 Tim.
        useRaster = true;
      }  

      song->startUndo();

      // delete existing events

      unsigned curPartTick = curPart->tick();
      int lastpv = CTRL_VAL_UNKNOWN;
      for (ciCEvent i = items.begin(); i != items.end(); ++i) {
            CEvent* ev = *i;
            if(ev->part() != curPart)
              continue;
            Event event = ev->event();
            if (event.empty())
                  continue;
            int x = event.tick() + curPartTick;
            //printf("CtrlCanvas::newValRamp x:%d xx1:%d xx2:%d len:%d\n", x, xx1, xx2, curPart->lenTick());
            
            if (x < xx1)
            {
            //      if(event.dataB() != CTRL_VAL_UNKNOWN)
            //        lastpv = event.dataB();
                  continue;
            }      
            //if (x <= xx1)
            //{
            //      if(type == CTRL_PROGRAM && event.dataB() != CTRL_VAL_UNKNOWN && ((event.dataB() & 0xffffff) != 0xffffff))
            //        lastpv = event.dataB();
            //      if (x < xx1)
            //        continue;
            //}
            if (x >= xx2)
                  break;
            
            // Indicate no undo, and do port controller values and clone parts. 
            audio->msgDeleteEvent(event, curPart, false, true, true);
            }

      //if(type == CTRL_PROGRAM && lastpv == CTRL_VAL_UNKNOWN)
      if(ctrl)  
        lastpv = ctrl->hwVal();
        
      unsigned curPartLen = curPart->lenTick();
      
      // insert new events
      //for (int x = xx1; x < xx2; x += raster) {
      //      int y    = (x2==x1) ? y1 : (((y2-y1)*(x-x1))/(x2-x1))+y1;
      //      int nval = computeVal(_controller, y, height());
      for (int x = xx1, step; x < xx2 ; x += step )    
      {
            step = useRaster ? raster : editor->rasterVal2(x + 1) - x;
            
            int y    = x + step >= xx2 || x2 == x1 ? y2 : (x == xx1 ? y1 : (((y2 - y1) * (x + step/2 - x1)) / (x2 - x1)) + y1);
            int nval = computeVal(_controller, y, height());
            
            //int tick = x - curPartTick;
            unsigned tick = x - curPartTick;
            //printf("CtrlCanvas::newValRamp x:%d xx1:%d xx2:%d step:%d newtick:%d\n", x, xx1, xx2, step, tick);  
            // Do not add events which are past the end of the part.
            //if((unsigned)tick >= curPartLen)
            if(tick >= curPartLen)
              break;
            Event event(Controller);
            event.setTick(tick);
            event.setA(_didx);
            if(type == CTRL_PROGRAM)
            {
              if(lastpv == CTRL_VAL_UNKNOWN)
              {
                if(song->mtype() == MT_GM)
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
            audio->msgAddEvent(event, curPart, false, true, true);
            }
              
      ///song->update(0);
      ///redraw();
      song->endUndo(SC_EVENT_MODIFIED | SC_EVENT_INSERTED | SC_EVENT_REMOVED);
      }

//---------------------------------------------------------
//   changeValRamp
//---------------------------------------------------------

void CtrlCanvas::changeValRamp(int x1, int y1, int x2, int y2)
      {
      int h   = height();
      bool changed = false;
      int type = _controller->num();
      //int xx1 = editor->rasterVal1(x1);

      song->startUndo();
      for (ciCEvent i = items.begin(); i != items.end(); ++i) {
            if ((*i)->contains(x1, x2)) {
            //if ((*i)->contains(xx1, x2)) {
                  CEvent* ev       = *i;
                  if(ev->part() != curPart)
                    continue;
                  Event event = ev->event();
                  if (event.empty())
                        continue;

                  //MidiPart* part   = ev->part();
                  //int x    = event.tick() + ev->part()->tick();
                  int x    = event.tick() + curPart->tick();
                  int y    = (x2==x1) ? y1 : (((y2-y1)*(x-x1))/(x2-x1))+y1;
                  int nval = computeVal(_controller, y, h);
                  if(type == CTRL_PROGRAM)
                  {
                    if(event.dataB() == CTRL_VAL_UNKNOWN)
                    {
                      --nval;
                      if(song->mtype() == MT_GM)
                        nval |= 0xffff00;
                    }
                    else  
                      nval = (event.dataB() & 0xffff00) | (nval - 1);
                  }
                    
                  ev->setVal(nval);
                  
                  //MidiController::ControllerType type = midiControllerType(_controller->num());
                  //if (type == MidiController::Velo) {
                  if (type == CTRL_VELOCITY) {
                        if ((event.velo() != nval)) {
                              Event newEvent = event.clone();
                              newEvent.setVelo(nval);
                              ev->setEvent(newEvent);
                              // Indicate no undo, and do not do port controller values and clone parts. 
                              audio->msgChangeEvent(event, newEvent, curPart, false, false, false);
                              ///ev->setEvent(newEvent);
                              changed = true;
                              }
                        }
                  else {
                        if (!event.empty()) {
                              if ((event.dataB() != nval)) {
                                    Event newEvent = event.clone();
                                    newEvent.setB(nval);
                                    ev->setEvent(newEvent);
                                    // Indicate no undo, and do port controller values and clone parts. 
                                    audio->msgChangeEvent(event, newEvent, curPart, false, true, true);
                                    ///ev->setEvent(newEvent);
                                    changed = true;
                                    }
                              }
                        else {
                              //if(!ctrl)
                              //{
                              //  ctrl = 
                              //}
                                
                              // Removed by T356. Never gets here? A good thing, don't wan't auto-create values.
                              //int oval = ctrl->value(0);
                              //if (oval != nval) {
                                    // Changed by T356.
                                    //ctrl->add(0, nval);
                              //      ctrl->add(0, nval, part);
                              //      changed = true;
                              //      }
                              
                              }
                        }
                  }
            }
      ///if (changed)
      ///      redraw();
      song->endUndo(SC_EVENT_MODIFIED);
      }

//---------------------------------------------------------
//   changeVal
//---------------------------------------------------------

void CtrlCanvas::changeVal(int x1, int x2, int y)
      {
      bool changed = false;
      int newval = computeVal(_controller, y, height());
      int type = _controller->num();
      //int xx1 = editor->rasterVal1(x1);

      for (ciCEvent i = items.begin(); i != items.end(); ++i) {
            if (!(*i)->contains(x1, x2))
            //if (!(*i)->contains(xx1, x2))
                  continue;
            CEvent* ev       = *i;
            if(ev->part() != curPart)
              continue;
            Event event      = ev->event();
            //if(event.tick() >= curPart->lenTick())
            //  break;
              
            //MidiPart* part   = ev->part();
            //int nval = newval;
            //if(type == CTRL_PROGRAM)
            //{
            //  if(event.dataB() == CTRL_VAL_UNKNOWN)
            //  {
            //    --nval;
            //    if(song->mtype() == MT_GM)
            //      nval |= 0xffff00;
            //  }
            //  else  
            //    nval = (event.dataB() & 0xffff00) | (nval - 1);
            //}
            //ev->setVal(nval);
            
            //MidiController::ControllerType type = midiControllerType(_controller->num());
            //if (type == MidiController::Velo) {
            if (type == CTRL_VELOCITY) {
                  if ((event.velo() != newval)) {
                        ev->setVal(newval);
                        Event newEvent = event.clone();
                        newEvent.setVelo(newval);
                        ev->setEvent(newEvent);
                        // Indicate no undo, and do not do port controller values and clone parts. 
                        audio->msgChangeEvent(event, newEvent, curPart, false, false, false);
                        ///ev->setEvent(newEvent);
                        changed = true;
                        }
                  }
            else {
                  if (!event.empty()) {
                        int nval = newval;
                        if(type == CTRL_PROGRAM)
                        {
                          if(event.dataB() == CTRL_VAL_UNKNOWN)
                          {
                            --nval;
                            if(song->mtype() == MT_GM)
                              nval |= 0xffff00;
                          }
                          else  
                            nval = (event.dataB() & 0xffff00) | (nval - 1);
                        }
                        ev->setVal(nval);
                        
                        if ((event.dataB() != nval)) {
                              Event newEvent = event.clone();
                              newEvent.setB(nval);
                              ev->setEvent(newEvent);
                              // Indicate no undo, and do port controller values and clone parts. 
                              audio->msgChangeEvent(event, newEvent, curPart, false, true, true);
                              ///ev->setEvent(newEvent);
                              changed = true;
                              }
                        }
                  else {
                        //if(!ctrl)
                        //{
                        //  ctrl = 
                        //}
                          
                        // Removed by T356. Never gets here? A good thing, don't wan't auto-create values.
                        //int oval = ctrl->value(0);
                        //if (oval != nval) {
                              // Changed by T356.
                              //ctrl->add(0, nval);
                        //      ctrl->add(0, nval, part);
                        //      changed = true;
                        //      }
                        }
                  }
            }
      if (changed)
            redraw();
      }

/*
//---------------------------------------------------------
//   newVal
//---------------------------------------------------------

void CtrlCanvas::newVal(int x1, int x2, int y)
      {
      int xx1  = editor->rasterVal1(x1);
      int xx2  = editor->rasterVal2(x2);
      int newval = computeVal(_controller, y, height());
      int type = _controller->num();

      bool found        = false;
      bool song_changed = false;

      int lastpv = CTRL_VAL_UNKNOWN;
      if(ctrl)
        lastpv = ctrl->hwVal();
        
      for (ciCEvent i = items.begin(); i != items.end(); ++i) {
            CEvent* ev = *i;
            if(ev->part() != curPart)
              continue;
            //int partTick = ev->part()->tick();
            int partTick = curPart->tick();
            Event event = ev->event();
            if (event.empty())
                  continue;
            int ax = event.tick() + partTick;
            //printf("CtrlCanvas::newVal ax:%d xx1:%d xx2:%d len:%d\n", ax, xx1, xx2, curPart->lenTick());  
            
            if (ax < xx1)
                  continue;
            //if(ax <= xx1)
            //{
            //  if(type == CTRL_PROGRAM && event.dataB() != CTRL_VAL_UNKNOWN && ((event.dataB() & 0xffffff) != 0xffffff))
            //    lastpv = event.dataB();
            //  if(ax < xx1)
            //    continue;
            //}
            if (ax >= xx2)
                  break;
            
            // Added by T356. Do not add events which are past the end of the part.
            //if(event.tick() >= curPart->lenTick())
            //  break;
              
            int nval = newval;
            if(type == CTRL_PROGRAM)
            {
              if(event.dataB() == CTRL_VAL_UNKNOWN)
              {
                //if(lastpv == CTRL_VAL_UNKNOWN)
                //  lastpv = ctrl->hwVal();
                
                if(lastpv == CTRL_VAL_UNKNOWN)
                {
                  --nval;
                  if(song->mtype() == MT_GM)
                    nval |= 0xffff00;
                }
                else  
                  nval = (lastpv & 0xffff00) | (nval - 1);
              }
              else  
                nval = (event.dataB() & 0xffff00) | (nval - 1);
            }
              
            if (ax == xx1) {
                  // change event
                  found = true;
                  ev->setVal(nval);
                  if ((event.dataB() != nval)) {
                        Event newEvent = event.clone();
                        newEvent.setB(nval);
                        // Indicate no undo, and do port controller values and clone parts. 
                        audio->msgChangeEvent(event, newEvent, curPart, false, true, true);
                        ev->setEvent(newEvent);
                        song_changed = true;
                        }
                  }
            else if (ax < xx2) {
                  // delete event
                  
                  //printf("CtrlCanvas::newVal delete xx1:%d xx2:%d len:%d\n", xx1, xx2, curPart->lenTick()); 
                 
                  // Indicate no undo, and do port controller values and clone parts. 
                  audio->msgDeleteEvent(event, curPart, false, true, true);
                  
                  song_changed = true;
                  }
            }
      if (!found) {
            // new event
            int tick = xx1 - curPart->tick();
            // Do not add events which are past the end of the part.
            if((unsigned)tick < curPart->lenTick())
            {
              Event event(Controller);
              event.setTick(tick);
              event.setA(_didx);
              if(type == CTRL_PROGRAM)
              {
                if(lastpv == CTRL_VAL_UNKNOWN)
                {
                  if(song->mtype() == MT_GM)
                    event.setB(0xffff00 | (newval - 1));
                  else  
                    event.setB(newval - 1);
                }
                else    
                  event.setB((lastpv & 0xffff00) | (newval - 1));
              }
              else  
                event.setB(newval);
              
              //printf("CtrlCanvas::newVal add tick:%d A:%d B:%d\n", tick, event.dataA(), event.dataB()); 
              
              // Indicate no undo, and do port controller values and clone parts. 
              audio->msgAddEvent(event, curPart, false, true, true);
              
              song_changed = true;
            }  
          }
      if (song_changed) 
      {
            songChanged(0);
            return;
      }
      redraw();
      }
*/

//---------------------------------------------------------
//   newVal
//---------------------------------------------------------

void CtrlCanvas::newVal(int x1, int y)
      {
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
      //bool do_selchanged = false;
      iCEvent ice_tmp;
      iCEvent prev_ev = items.end();     // End is OK with multi-part, this is just an end 'invalid' marker at first.
      iCEvent insertPoint = items.end(); // Similar case here. 
      bool curPartFound = false;

      int lastpv = CTRL_VAL_UNKNOWN;
      if(ctrl)
        lastpv = ctrl->hwVal();
        
      int partTick = curPart->tick();
      //for (ciCEvent i = items.begin(); i != items.end(); ++i) {
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
            
            //int partTick = ev->part()->tick();
            Event event = ev->event();
            if (event.empty())
            {
              prev_ev = i;
              ++i;
              continue;
            }      
            int ax = event.tick() + partTick;
            //printf("CtrlCanvas::newVal2 ax:%d xx1:%d xx2:%d len:%d\n", ax, xx1, xx2, curPart->lenTick());  
            
            if (ax < xx1)
            {
              prev_ev = i;
              ++i;
              continue;
            }      
            //if(ax <= xx1)
            //{
            //  if(type == CTRL_PROGRAM && event.dataB() != CTRL_VAL_UNKNOWN && ((event.dataB() & 0xffffff) != 0xffffff))
            //    lastpv = event.dataB();
            //  if(ax < xx1)
            //    continue;
            //}
            if (ax >= xx2)
            {
              insertPoint = i;
              break;
            }
            
            // Do not add events which are past the end of the part.
            //if(event.tick() >= curPart->lenTick())
            //  break;
              
            int nval = newval;
            if(type == CTRL_PROGRAM)
            {
              if(event.dataB() == CTRL_VAL_UNKNOWN)
              {
                //if(lastpv == CTRL_VAL_UNKNOWN)
                //  lastpv = ctrl->hwVal();
                
                if(lastpv == CTRL_VAL_UNKNOWN)
                {
                  --nval;
                  if(song->mtype() == MT_GM)
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
                Event newEvent = event.clone();
                newEvent.setB(nval);
                //printf("CtrlCanvas::newVal2 change xx1:%d xx2:%d len:%d\n", xx1, xx2, curPart->lenTick());  
                
                ev->setEvent(newEvent);
                
                // Indicate no undo, and do port controller values and clone parts. 
                audio->msgChangeEvent(event, newEvent, curPart, false, true, true);
                
                ///ev->setEvent(newEvent);
                
                do_redraw = true;      
              }
              prev_ev = i;
              ++i;
            }
            else if (ax < xx2) 
            {
                  // delete event
                  
                  //printf("CtrlCanvas::newVal2 delete xx1:%d xx2:%d len:%d\n", xx1, xx2, curPart->lenTick()); 
                 
                  deselectItem(ev);
                  // Indicate no undo, and do port controller values and clone parts. 
                  audio->msgDeleteEvent(event, curPart, false, true, true);
                  
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
                  //do_selchanged = true;  //
                  
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
              Event event(Controller);
              event.setTick(tick);
              event.setA(_didx);
              if(type == CTRL_PROGRAM)
              {
                if(lastpv == CTRL_VAL_UNKNOWN)
                {
                  if(song->mtype() == MT_GM)
                    event.setB(0xffff00 | (newval - 1));
                  else  
                    event.setB(newval - 1);
                }
                else    
                  event.setB((lastpv & 0xffff00) | (newval - 1));
              }
              else  
                event.setB(newval);
              
              //printf("CtrlCanvas::newVal2 add tick:%d A:%d B:%d\n", tick, event.dataA(), event.dataB()); 
              
              // Indicate no undo, and do port controller values and clone parts. 
              audio->msgAddEvent(event, curPart, false, true, true);
              
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
      
      //if(do_selchanged)
      //  song->update(SC_SELECTION);      // Let songChanged handle the redraw upon SC_SELECTION.
      //else 
      if(do_redraw)                 //
        redraw();
      }

//---------------------------------------------------------
//   newVal
//---------------------------------------------------------

void CtrlCanvas::newVal(int x1, int y1, int x2, int y2)
      {
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
      // Grab the 'second' raster. Nudge by +1 and let rasterVal2 snap to the next raster.
      //int xn1 = editor->rasterVal2(xx1 + 1);
      
      int xx2 = editor->rasterVal2(x2);
      // Grab the 'second last' raster. Nudge by -1 and let rasterVal1 snap to the previous raster.
      //int xn2 = editor->rasterVal1(xx2 - 1);
      
      // If x1 and x2 happen to lie directly on the same raster, xx1 will equal xx2, 
      //  which is not good - there should always be a spread. Nudge by +1 and recompute.
      if(xx1 == xx2)
        xx2  = editor->rasterVal2(x2 + 1);
      
      int type = _controller->num();

      bool useRaster = false;
      int raster = editor->raster();
      if (raster == 1)          // set reasonable raster
      {
        //raster = config.division/4;
        raster = config.division/16;  // Let's use 64th notes, for a bit finer resolution. p4.0.18 Tim.
        useRaster = true;
      }  

      bool do_redraw = false;
      //bool do_selchanged = false;
      
      // delete existing events

      iCEvent prev_ev = items.end();     // End is OK with multi-part, this is just an end 'invalid' marker at first.
      iCEvent insertPoint = items.end(); // Similar case here. 
      iCEvent ice_tmp;
      bool curPartFound = false;
      int lastpv = CTRL_VAL_UNKNOWN;
      int partTick = curPart->tick();
      //for (ciCEvent i = items.begin(); i != items.end(); ++i) 
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
            
            Event event = ev->event();
            if (event.empty())
            {
              prev_ev = i;
              ++i;
              continue;
            }      
            int x = event.tick() + partTick;
            //printf("CtrlCanvas::newVal x:%d xx1:%d xx2:%d len:%d\n", x, xx1, xx2, curPart->lenTick());
            
            if (x < xx1)
            //if (x < (xn1 >= xx2 ? xx1 : xn1))
            {
            //  if(event.dataB() != CTRL_VAL_UNKNOWN)
            //    lastpv = event.dataB();
              prev_ev = i;
              ++i;
              continue;
            }      
            //if (x <= xx1)
            //{
            //      if(type == CTRL_PROGRAM && event.dataB() != CTRL_VAL_UNKNOWN && ((event.dataB() & 0xffffff) != 0xffffff))
            //        lastpv = event.dataB();
            //      if (x < xx1)
            //        continue;
            //}
            if (x >= xx2)
            {
              insertPoint = i;
              break;
            }
            
            deselectItem(ev);
            // Indicate no undo, and do port controller values and clone parts. 
            audio->msgDeleteEvent(event, curPart, false, true, true);
            
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
            //do_selchanged = true;  //
            
            prev_ev = i;
      }

      //if(type == CTRL_PROGRAM && lastpv == CTRL_VAL_UNKNOWN)
      if(ctrl)  
        lastpv = ctrl->hwVal();
        
      // insert new events
      //for (int x = xx1; x < xx2; x += raster) {
      // Nudge x by +1 and let rasterVal2 snap to the next raster.
      //for (int x = xx1; x < xx2;  x = useRaster ? x + raster : editor->rasterVal2(x + 1))    
      //for (int x = xn1; x < xx2;  x = useRaster ? x + raster : editor->rasterVal2(x + 1))    
      //for (int x = xn1, step; x < xx2 ; x += (step = useRaster ? raster : editor->rasterVal2(x + 1) - x) )    
      // Start from the 'second' raster - the first raster is already set in mouseDown!
      //for (int x = xn1, step; x < xx2 ; x += step )    
      //for (int x = xn1 >= xx2 ? xx1 : xn1, step; x < xx2 ; x += step )    
      for (int x = xx1, step; x < xx2 ; x += step )    
      {
            step = useRaster ? raster : editor->rasterVal2(x + 1) - x;
            
            int y    = x + step >= xx2 || x2 == x1 ? y2 : (x == xx1 ? y1 : (((y2 - y1) * (x + step/2 - x1)) / (x2 - x1)) + y1);
            int nval = computeVal(_controller, y, height());
            
            int tick = x - partTick;
            // Do not add events which are past the end of the part.
            if((unsigned)tick >= curPart->lenTick())
              break;
            Event event(Controller);
            event.setTick(tick);
            event.setA(_didx);
            if(type == CTRL_PROGRAM)
            {
              if(lastpv == CTRL_VAL_UNKNOWN)
              {
                if(song->mtype() == MT_GM)
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
            audio->msgAddEvent(event, curPart, false, true, true);
            
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
              
      //if(do_selchanged)
      //  song->update(SC_SELECTION);      // Let songChanged handle the redraw upon SC_SELECTION.  
      //else 
      if(do_redraw)                 //
        redraw();
      }

//---------------------------------------------------------
//   deleteVal
//---------------------------------------------------------

void CtrlCanvas::deleteVal(int x1, int x2, int)
      {
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
      //bool song_changed = false;
      bool do_redraw = false;
      
      //for (ciCEvent i = items.begin(); i != items.end(); ++i) 
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
            
            Event event = ev->event();
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
            audio->msgDeleteEvent(event, curPart, false, true, true);
            
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
            
      //if (song_changed) {
      //      songChanged(0);
      //      return;
      //      }
      if(do_redraw)
        redraw();                 // Let songChanged handle the redraw upon SC_SELECTION.  
        //song->update(SC_SELECTION); //  
      }

//---------------------------------------------------------
//   setTool
//---------------------------------------------------------

void CtrlCanvas::setTool(int t)
      {
      if (tool == Tool(t))
            return;
      tool = Tool(t);
      switch(tool) {
            case PencilTool:
                  setCursor(QCursor(*pencilIcon, 4, 15));
                  break;
            case DrawTool:
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

void CtrlCanvas::pdrawItems(QPainter& p, const QRect& rect, const MidiPart* part, bool velo, bool fg)
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
      //if((fg && e->part() != part) || (!fg && e->part() == part))
      if(e->part() != part)
        continue;
      Event event = e->event();
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
        //if(!event.empty() && event.selected())
          p.setPen(QPen(Qt::blue, 3));
        else
          p.setPen(QPen(config.ctrlGraphFg, 3));
      }  
      else  
        p.setPen(QPen(Qt::darkGray, 3));
      p.drawLine(tick, wh, tick, y1);
    }
  }
  else
  {
    MidiTrack* mt = part->track();
    MidiPort* mp;
    
    if((mt->type() == Track::DRUM) && (curDrumInstrument != -1) && ((_cnum & 0xff) == 0xff)) 
      mp = &midiPorts[drumMap[curDrumInstrument].port];          
    else
      mp = &midiPorts[mt->outPort()];          
    
    MidiController* mc = mp->midiController(_cnum);
    
    int min;
    int max;
    int bias;
    if(_cnum == CTRL_PROGRAM)
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
    int lval = CTRL_VAL_UNKNOWN;
    ///noEvents=false;
    bool selected = false;
    for (iCEvent i = items.begin(); i != items.end(); ++i) 
    {
      noEvents=false;
      CEvent* e = *i;
      // Draw unselected part controller events (lines) on top of selected part events (bars).
      //if((fg && (e->part() == part)) || (!fg && (e->part() != part)))
      if(e->part() != part)
      {  
        continue;
      }
      Event ev = e->event();
      int tick = mapx(!ev.empty() ? ev.tick() + e->part()->tick() : 0);
      int val = e->val();
      int pval = val;
      if(_cnum == CTRL_PROGRAM)
      {
        if((val & 0xff) == 0xff)
          // What to do here? prog = 0xff should not be allowed, but may still be encountered.
          pval = 1;
        else  
          pval = (val & 0x7f) + 1;
      }
      if (tick <= x) {
            if (val == CTRL_VAL_UNKNOWN)
                  lval = CTRL_VAL_UNKNOWN;
            else
            {
                  if(_cnum == CTRL_PROGRAM)
                    lval = wh - ((pval - min - bias) * wh / (max - min));
                  else  
                    lval = wh - ((val - min - bias) * wh / (max - min));
            }
            selected = e->selected();     
            continue;
            }
      if (tick > x+w)
            break;
      if (lval == CTRL_VAL_UNKNOWN)
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
          p.fillRect(x1, lval, tick - x1, wh - lval, selected ? Qt::blue : config.ctrlGraphFg);
      }
      
      
      x1 = tick;
      if (val == CTRL_VAL_UNKNOWN)
            lval = CTRL_VAL_UNKNOWN;
      else
      {
            if(_cnum == CTRL_PROGRAM)
              lval = wh - ((pval - min - bias) * wh / (max - min));
            else  
              lval = wh - ((val - min - bias) * wh / (max - min));
      } 
      selected = e->selected();     
      //selected = !ev.empty() && ev.selected();     
    }
    if (lval == CTRL_VAL_UNKNOWN)
    {
      if(!fg) {
        p.fillRect(x1, 0, (x+w) - x1, wh, Qt::darkGray);
	///noEvents=true;
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
        //p.fillRect(x1, lval, (x+w) - x1, wh - lval, config.ctrlGraphFg);
        p.fillRect(x1, lval, (x+w) - x1, wh - lval, selected ? Qt::blue : config.ctrlGraphFg);
    }
  }       
}

//---------------------------------------------------------
//   pdraw
//---------------------------------------------------------

void CtrlCanvas::pdraw(QPainter& p, const QRect& rect)
      {
      
      int x = rect.x() - 1;   // compensate for 3 pixel line width
      int y = rect.y();
      int w = rect.width() + 2;
      int h = rect.height();
      
      //---------------------------------------------------
      // draw Canvas Items
      //---------------------------------------------------

      bool velo = (midiControllerType(_controller->num()) == MidiController::Velo);
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
        
      for(iPart ip = editor->parts()->begin(); ip != editor->parts()->end(); ++ip) 
      {
        MidiPart* part = (MidiPart*)(ip->second);
        //if((velo && part == curPart) || (!velo && part != curPart))
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
      
      //p.setFont(config.fonts[3]);  // Use widget font instead. 
      p.setFont(font());
      
      p.setPen(Qt::black);
      
      //QFontMetrics fm(config.fonts[3]);  // Use widget font metrics instead. 
      //int y = fm.lineSpacing() + 2;
      int y = fontMetrics().lineSpacing() + 2;
      
      p.drawText(2, y, s);
      if (noEvents) {
           //p.setFont(config.fonts[3]);
           //p.setPen(Qt::black);
           //p.drawText(width()/2-100,height()/2-10, "Use ctrlKey + pencil or line tool to draw new events");
           p.drawText(2 , y * 2, "Use ctrlKey + pencil or line tool to draw new events");
           }
      }

//---------------------------------------------------------
//   overlayRect
//    returns geometry of overlay rectangle
//---------------------------------------------------------

QRect CtrlCanvas::overlayRect() const
{
      //QFontMetrics fm(config.fonts[3]);   // Use widget font metrics instead (and set a widget font) !!! 
      QFontMetrics fm(fontMetrics());
      QRect r(fm.boundingRect(_controller ? _controller->name() : QString("")));
      
      //r.translate(2, 2);                    // top/left margin
      int y = fm.lineSpacing() + 2;
      r.translate(2, y);   
      if (noEvents) 
      {
        QRect r2(fm.boundingRect(QString("Use ctrlKey + pencil or line tool to draw new events")));
        //r2.translate(width()/2-100, height()/2-10);   
        r2.translate(2, y * 2);   
        r |= r2;
      }
      
      //printf("CtrlCanvas::overlayRect x:%d y:%d w:%d h:%d\n", r.x(), r.y(), r.width(), r.height()); 
      return r;
}

//---------------------------------------------------------
//   draw
//---------------------------------------------------------

void CtrlCanvas::draw(QPainter& p, const QRect& rect)
      {
      drawTickRaster(p, rect.x(), rect.y(),
         //rect.width(), rect.height(), editor->quant());
         rect.width(), rect.height(), editor->raster());

      //---------------------------------------------------
      //    draw line tool
      //---------------------------------------------------

      if (drawLineMode && (tool == DrawTool)) {
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
      //printf("CtrlCanvas::setCurDrumInstrument curDrumInstrument:%d\n", curDrumInstrument);
      
      //
      //  check if current controller is only valid for
      //  a specific drum instrument
      //
      // Removed by T356.
      //if(curTrack && (curTrack->type() == Track::DRUM) && ((_controller->num() & 0xff) == 0xff)) {
      //if(curTrack && (curTrack->type() == Track::DRUM) && ((_cnum & 0xff) == 0xff)) {
            // reset to default
            // TODO: check, if new drum instrument has a similar controller
            //    configured
      //      _cnum = CTRL_VELOCITY;
      //      }
      // Removed by T356
      //songChanged(-1);
      }
