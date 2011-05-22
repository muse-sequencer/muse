//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: dcanvas.cpp,v 1.16.2.10 2009/10/15 22:45:50 terminator356 Exp $
//  (C) Copyright 1999 Werner Schweer (ws@seh.de)
//=========================================================

#include <QPainter>
#include <QApplication>
#include <QClipboard>
#include <QDrag>
#include <QDragLeaveEvent>
#include <QPolygon>
#include <QDragEnterEvent>
#include <QDragMoveEvent>
#include <QDropEvent>
#include <QResizeEvent>

#include <stdio.h>
#include <values.h>
#include <errno.h>
#include <set>
//#include <sys/stat.h>
//#include <sys/mman.h>

#include "dcanvas.h"
#include "midieditor.h"
#include "drummap.h"
#include "event.h"
#include "mpevent.h"
#include "xml.h"
#include "globals.h"
#include "midiport.h"
#include "audio.h"
#include "shortcuts.h"
#include "icons.h"

#define CARET   10
#define CARET2   5

//---------------------------------------------------------
//   DEvent
//---------------------------------------------------------

DEvent::DEvent(Event e, Part* p)
  : CItem(e, p)
      {
      int instr = e.pitch();
      int y  = instr * TH + TH/2;
      int tick = e.tick() + p->tick();
      setPos(QPoint(tick, y));
      setBBox(QRect(-CARET2, -CARET2, CARET, CARET));
      }

//---------------------------------------------------------
//   addItem
//---------------------------------------------------------

void DrumCanvas::addItem(Part* part, Event& event)
      {
      if (signed(event.tick())<0) {
            printf("ERROR: trying to add event before current part!\n");
            return;
      }
      
      DEvent* ev = new DEvent(event, part);
      items.add(ev);
      
      int diff = event.endTick()-part->lenTick();
      if (diff > 0)  {// too short part? extend it
            //printf("addItem - this code should not be run!\n");
            //Part* newPart = part->clone();
            //newPart->setLenTick(newPart->lenTick()+diff);
            //audio->msgChangePart(part, newPart,false);
            //part = newPart;
            part->setLenTick(part->lenTick()+diff);
            }
      }

//---------------------------------------------------------
//   DrumCanvas
//---------------------------------------------------------

DrumCanvas::DrumCanvas(MidiEditor* pr, QWidget* parent, int sx,
   int sy, const char* name)
   : EventCanvas(pr, parent, sx, sy, name)
      {
      setVirt(false);
      cursorPos= QPoint(0,0);
      _stepSize=1;
      songChanged(SC_TRACK_INSERTED);
      }

//---------------------------------------------------------
//   moveCanvasItems
//---------------------------------------------------------

void DrumCanvas::moveCanvasItems(CItemList& items, int dp, int dx, DragType dtype, int* pflags)
{      
  if(editor->parts()->empty())
    return;
    
  PartsToChangeMap parts2change;
  
  int modified = 0;
  for(iPart ip = editor->parts()->begin(); ip != editor->parts()->end(); ++ip)
  {
    Part* part = ip->second;
    if(!part)
      continue;
    
    int npartoffset = 0;
    for(iCItem ici = items.begin(); ici != items.end(); ++ici) 
    {
      CItem* ci = ici->second;
      if(ci->part() != part)
        continue;
      
      int x = ci->pos().x() + dx;
      int y = pitch2y(y2pitch(ci->pos().y()) + dp);
      QPoint newpos = raster(QPoint(x, y));
      
      // Test moving the item...
      DEvent* nevent = (DEvent*) ci;
      Event event    = nevent->event();
      x              = newpos.x();
      if(x < 0)
        x = 0;
      int ntick = editor->rasterVal(x) - part->tick();
      if(ntick < 0)
        ntick = 0;
      int diff = ntick + event.lenTick() - part->lenTick();
      
      // If moving the item would require a new part size...
      if(diff > npartoffset)
        npartoffset = diff;
    }
        
    if(npartoffset > 0)
    {    
      iPartToChange ip2c = parts2change.find(part);
      if(ip2c == parts2change.end())
      {
        PartToChange p2c = {0, npartoffset};
        parts2change.insert(std::pair<Part*, PartToChange> (part, p2c));
      }
      else
        ip2c->second.xdiff = npartoffset;
    }
  }
    
  for(iPartToChange ip2c = parts2change.begin(); ip2c != parts2change.end(); ++ip2c)
  {
    Part* opart = ip2c->first;
    int diff = ip2c->second.xdiff;
    
    Part* newPart = opart->clone();
    
    newPart->setLenTick(newPart->lenTick() + diff);
    
    modified = SC_PART_MODIFIED;
    
    // BUG FIX: #1650953
    // Added by T356.
    // Fixes posted "select and drag past end of part - crashing" bug
    for(iPart ip = editor->parts()->begin(); ip != editor->parts()->end(); ++ip)
    {
      if(ip->second == opart)
      {
        editor->parts()->erase(ip);
        break;
      }
    }
      
    editor->parts()->add(newPart);
    // Indicate no undo, and do port controller values but not clone parts. 
    audio->msgChangePart(opart, newPart, false, true, false);
    
    ip2c->second.npart = newPart;
    
  }
    
  iPartToChange icp = parts2change.find(curPart);
  if(icp != parts2change.end())
  {
    curPart = icp->second.npart;
    curPartId = curPart->sn();
  }  
    
  std::vector< CItem* > doneList;
  typedef std::vector< CItem* >::iterator iDoneList;
  
  for(iCItem ici = items.begin(); ici != items.end(); ++ici) 
  {
    CItem* ci = ici->second;
    
    // If this item's part is in the parts2change list, change the item's part to the new part.
    Part* pt = ci->part();
    iPartToChange ip2c = parts2change.find(pt);
    if(ip2c != parts2change.end())
      ci->setPart(ip2c->second.npart);
    
    int x = ci->pos().x();
    int y = ci->pos().y();
    int nx = x + dx;
    int ny = pitch2y(y2pitch(y) + dp);
    QPoint newpos = raster(QPoint(nx, ny));
    selectItem(ci, true);
    
    iDoneList idl;
    for(idl = doneList.begin(); idl != doneList.end(); ++idl)
      // This compares EventBase pointers to see if they're the same...
      if((*idl)->event() == ci->event())
        break;
      
    // Do not process if the event has already been processed (meaning it's an event in a clone part)...
    if(idl != doneList.end())
      // Just move the canvas item.
      ci->move(newpos);
    else
    {
      // Currently moveItem always returns true.
      if(moveItem(ci, newpos, dtype))
      {
        // Add the canvas item to the list of done items.
        doneList.push_back(ci);
        // Move the canvas item.
        ci->move(newpos);
      }  
    }
          
    if(moving.size() == 1) {
          itemReleased(curItem, newpos);
          }
    if(dtype == MOVE_COPY || dtype == MOVE_CLONE)
          selectItem(ci, false);
  }  
      
  if(pflags)
    *pflags = modified;
}
      
//---------------------------------------------------------
//   moveItem
//---------------------------------------------------------

bool DrumCanvas::moveItem(CItem* item, const QPoint& pos, DragType dtype)
      {
      DEvent* nevent   = (DEvent*) item;
      
      MidiPart* part   = (MidiPart*)nevent->part();   
      
      Event event      = nevent->event();
      int x            = pos.x();
      if (x < 0)
            x = 0;
      int ntick        = editor->rasterVal(x) - part->tick();
      if (ntick < 0)
            ntick = 0;
      int npitch       = y2pitch(pos.y());
      Event newEvent   = event.clone();

      newEvent.setPitch(npitch);
      newEvent.setTick(ntick);

      // msgAddEvent and msgChangeEvent (below) will set these, but set them here first?
      //item->setPart(part);
      item->setEvent(newEvent);
      
      // Added by T356. 
      if(((int)newEvent.endTick() - (int)part->lenTick()) > 0)  
        printf("DrumCanvas::moveItem Error! New event end:%d exceeds length:%d of part:%s\n", newEvent.endTick(), part->lenTick(), part->name().toLatin1().constData());
      
      if (dtype == MOVE_COPY || dtype == MOVE_CLONE) {
            // Indicate no undo, and do not do port controller values and clone parts. 
            audio->msgAddEvent(newEvent, part, false, false, false);
            }
      else {
            // Indicate no undo, and do not do port controller values and clone parts. 
            audio->msgChangeEvent(event, newEvent, part, false, false, false);
            }
        
      return true;
      }

//---------------------------------------------------------
//   newItem
//---------------------------------------------------------

CItem* DrumCanvas::newItem(const QPoint& p, int state)
      {
      int instr = y2pitch(p.y());         //drumInmap[y2pitch(p.y())];
      int velo  = drumMap[instr].lv4;
      if (state == Qt::ShiftModifier)
            velo = drumMap[instr].lv3;
      else if (state == Qt::ControlModifier)
            velo = drumMap[instr].lv2;
      else if (state == (Qt::ControlModifier | Qt::ShiftModifier))
            velo = drumMap[instr].lv1;
      int tick = editor->rasterVal(p.x());
      return newItem(tick, instr, velo);
      }

//---------------------------------------------------------
//   newItem
//---------------------------------------------------------

CItem* DrumCanvas::newItem(int tick, int instrument, int velocity)
{
  tick    -= curPart->tick();
  Event e(Note);
  e.setTick(tick);
  e.setPitch(instrument);
  e.setVelo(velocity);
  e.setLenTick(drumMap[instrument].len);
  return new DEvent(e, curPart);
}

//---------------------------------------------------------
//   resizeItem
//---------------------------------------------------------

void DrumCanvas::resizeItem(CItem* item, bool)
      {
      DEvent* nevent = (DEvent*) item;
      Event ev = nevent->event();
      // Indicate do undo, and do not do port controller values and clone parts. 
      audio->msgDeleteEvent(ev, nevent->part(), true, false, false);
      }

//---------------------------------------------------------
//   newItem
//---------------------------------------------------------
void DrumCanvas::newItem(CItem* item, bool noSnap) {
     newItem(item, noSnap,false);
}

void DrumCanvas::newItem(CItem* item, bool noSnap, bool replace)
      {
      DEvent* nevent = (DEvent*) item;
      Event event    = nevent->event();
      int x = item->x();
      if (!noSnap)
            x = editor->rasterVal(x);
      event.setTick(x - nevent->part()->tick());
      int npitch = event.pitch();
      event.setPitch(npitch);

      //
      // check for existing event
      //    if found change command semantic from insert to delete
      //
      EventList* el = nevent->part()->events();
      iEvent lower  = el->lower_bound(event.tick());
      iEvent upper  = el->upper_bound(event.tick());

      for (iEvent i = lower; i != upper; ++i) {
            Event ev = i->second;
            // Added by T356. Only do notes.
            if(!ev.isNote())
              continue;
              
            if (ev.pitch() == npitch) {
                  // Indicate do undo, and do not do port controller values and clone parts. 
                  audio->msgDeleteEvent(ev, nevent->part(), true, false, false);
                  if (replace)
                    break;
                  else
                    return;

              }
            }

      // Added by T356.
      Part* part = nevent->part();
      song->startUndo();
      int modified=SC_EVENT_MODIFIED;
      int diff = event.endTick()-part->lenTick();
      if (diff > 0)  {// too short part? extend it
            Part* newPart = part->clone();
            newPart->setLenTick(newPart->lenTick()+diff);
            // Indicate no undo, and do port controller values but not clone parts. 
            audio->msgChangePart(part, newPart, false, true, false);
            modified=modified|SC_PART_MODIFIED;
            part = newPart; // reassign
            }
      // Indicate no undo, and do not do port controller values and clone parts. 
      audio->msgAddEvent(event, part, false, false, false); 
      song->endUndo(modified);
      
      }

//---------------------------------------------------------
//   deleteItem
//---------------------------------------------------------

bool DrumCanvas::deleteItem(CItem* item)
      {
      Event ev = ((DEvent*)item)->event();
      // Indicate do undo, and do not do port controller values and clone parts. 
      audio->msgDeleteEvent(ev, ((DEvent*)item)->part(), true, false, false);
      return false;
      }

//---------------------------------------------------------
//   drawItem
//---------------------------------------------------------

void DrumCanvas::drawItem(QPainter&p, const CItem*item, const QRect& rect)
      {
      DEvent* e   = (DEvent*) item;
      int x = 0, y = 0;
        x = mapx(item->pos().x());
        y = mapy(item->pos().y());
      QPolygon pa(4);
      pa.setPoint(0, x - CARET2, y);
      pa.setPoint(1, x,          y - CARET2);
      pa.setPoint(2, x + CARET2, y);
      pa.setPoint(3, x,          y + CARET2);
      QRect r(pa.boundingRect());
      r = r.intersect(rect);
      if(!r.isValid())
        return;
      
      p.setPen(Qt::black);
      
      if (e->part() != curPart)
      {
            if(item->isMoving()) 
              p.setBrush(Qt::gray);
            else if(item->isSelected()) 
              p.setBrush(Qt::black);
            else  
              p.setBrush(Qt::lightGray);
      }      
      else if (item->isMoving()) {
              p.setBrush(Qt::gray);
            }
      else if (item->isSelected())
      {
            p.setBrush(Qt::black);
      }
      else
      {
            int velo    = e->event().velo();
            DrumMap* dm = &drumMap[y2pitch(y)]; //Get the drum item
            QColor color;
            if (velo < dm->lv1)
                  color.setRgb(240, 240, 255);
            else if (velo < dm->lv2)
                  color.setRgb(200, 200, 255);
            else if (velo < dm->lv3)
                  color.setRgb(170, 170, 255);
            else
                  color.setRgb(0, 0, 255);
            p.setBrush(color);
      }
            
      p.drawPolygon(pa);
      }

//---------------------------------------------------------
//   drawMoving
//    draws moving items
//---------------------------------------------------------

void DrumCanvas::drawMoving(QPainter& p, const CItem* item, const QRect& rect)
    {
      QPolygon pa(4);
      QPoint pt = map(item->mp());
      int x = pt.x();
      int y = pt.y();
      pa.setPoint(0, x-CARET2,  y + TH/2);
      pa.setPoint(1, x,         y + TH/2+CARET2);
      pa.setPoint(2, x+CARET2,  y + TH/2);
      pa.setPoint(3, x,         y + (TH-CARET)/2);
      QRect mr(pa.boundingRect());
      mr = mr.intersect(rect);
      if(!mr.isValid())
        return;
      p.setPen(Qt::black);
      p.setBrush(Qt::black);
      p.drawPolygon(pa);
    }

//---------------------------------------------------------
//   drawCanvas
//---------------------------------------------------------

extern void drawTickRaster(QPainter& p, int, int, int, int, int);

void DrumCanvas::drawCanvas(QPainter& p, const QRect& rect)
      {
      int x = rect.x();
      int y = rect.y();
      int w = rect.width();
      int h = rect.height();

      //---------------------------------------------------
      //  horizontal lines
      //---------------------------------------------------

      int yy  = ((y-1) / TH) * TH + TH;
      for (; yy < y + h; yy += TH) {
            p.setPen(Qt::gray);
            p.drawLine(x, yy, x + w, yy);
            }

      //---------------------------------------------------
      // vertical lines
      //---------------------------------------------------

      drawTickRaster(p, x, y, w, h, editor->raster());
      }

//---------------------------------------------------------
//   drawTopItem
//---------------------------------------------------------
void DrumCanvas::drawTopItem(QPainter& p, const QRect&)
{
  // draw cursor
  if (_tool == CursorTool) {
    p.setPen(Qt::black);

    int y = mapy(TH * cursorPos.y());

    p.drawPixmap(mapx(cursorPos.x())-TH/2,y,TH,TH, *cursorIcon);
    // need to figure out a coordinate system for the cursor, complicated stuff.
  }

}
//---------------------------------------------------------
//   y2pitch
//---------------------------------------------------------

int DrumCanvas::y2pitch(int y) const
      {
      int pitch = y/TH;
      if (pitch >= DRUM_MAPSIZE)
            pitch = DRUM_MAPSIZE-1;
      return pitch;
      }

//---------------------------------------------------------
//   pitch2y
//---------------------------------------------------------

int DrumCanvas::pitch2y(int pitch) const
      {
      return pitch * TH;
      }

//---------------------------------------------------------
//   cmd
//---------------------------------------------------------

void DrumCanvas::cmd(int cmd)
      {
      switch(cmd) {
            case CMD_CUT:
                  copy();
                  song->startUndo();
                  for (iCItem i = items.begin(); i != items.end(); ++i) {
                        if (!i->second->isSelected())
                              continue;
                        DEvent* e = (DEvent*)(i->second);
                        Event event = e->event();
                        // Indicate no undo, and do not do port controller values and clone parts. 
                        audio->msgDeleteEvent(event, e->part(), false, false, false);
                        }
                  song->endUndo(SC_EVENT_REMOVED);
                  break;
            case CMD_COPY:
                  copy();
                  break;
            case CMD_PASTE:
                  paste();
                  break;
            case CMD_SELECT_ALL:     // select all
                  for (iCItem k = items.begin(); k != items.end(); ++k) {
                        if (!k->second->isSelected())
                              selectItem(k->second, true);
                        }
                  break;
            case CMD_SELECT_NONE:     // select none
                  deselectAll();
                  break;
            case CMD_SELECT_INVERT:     // invert selection
                  for (iCItem k = items.begin(); k != items.end(); ++k) {
                        selectItem(k->second, !k->second->isSelected());
                        }
                  break;
            case CMD_SELECT_ILOOP:     // select inside loop
                  for (iCItem k = items.begin(); k != items.end(); ++k) {
                        DEvent* nevent =(DEvent*)(k->second);
                        Part* part = nevent->part();
                        Event event = nevent->event();
                        unsigned tick  = event.tick() + part->tick();
                        if (tick < song->lpos() || tick >= song->rpos())
                              selectItem(k->second, false);
                        else
                              selectItem(k->second, true);
                        }
                  break;
            case CMD_SELECT_OLOOP:     // select outside loop
                  for (iCItem k = items.begin(); k != items.end(); ++k) {
                        DEvent* nevent = (DEvent*)(k->second);
                        Part* part     = nevent->part();
                        Event event    = nevent->event();
                        unsigned tick  = event.tick() + part->tick();
                        if (tick < song->lpos() || tick >= song->rpos())
                              selectItem(k->second, true);
                        else
                              selectItem(k->second, false);
                        }
                  break;
            case CMD_SELECT_PREV_PART:     // select previous part
                  {
                      Part* pt = editor->curCanvasPart();
                      Part* newpt = pt;
                      PartList* pl = editor->parts();
                      for(iPart ip = pl->begin(); ip != pl->end(); ++ip)
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
                      Part* pt = editor->curCanvasPart();
                      Part* newpt = pt;
                      PartList* pl = editor->parts();
                      for(iPart ip = pl->begin(); ip != pl->end(); ++ip)
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

            case CMD_SAVE:
            case CMD_LOAD:
                  printf("DrumCanvas:: cmd not implemented %d\n", cmd);
                  break;

            case CMD_FIXED_LEN: //Set notes to the length specified in the drummap
                  if (!selectionSize())
                        break;
                  song->startUndo();
                  for (iCItem k = items.begin(); k != items.end(); ++k) {
                        if (k->second->isSelected()) {
                              DEvent* devent = (DEvent*)(k->second);
                              Event event    = devent->event();
                              Event newEvent = event.clone();
                              newEvent.setLenTick(drumMap[event.pitch()].len);
                              // Indicate no undo, and do not do port controller values and clone parts. 
                              audio->msgChangeEvent(event, newEvent, devent->part(), false, false, false);
                              }
                        }
                  song->endUndo(SC_EVENT_MODIFIED);
                  break;
            case CMD_LEFT:
                  {
                      int spos = pos[0];
                      if(spos > 0)
                      {
                        spos -= 1;     // Nudge by -1, then snap down with raster1.
                        spos = AL::sigmap.raster1(spos, editor->rasterStep(pos[0]));
                      }
                      if(spos < 0)
                        spos = 0;
                      Pos p(spos,true);
                      song->setPos(0, p, true, true, true);
                  }
                  break;
            case CMD_RIGHT:
                  {
                      int spos = AL::sigmap.raster2(pos[0] + 1, editor->rasterStep(pos[0]));    // Nudge by +1, then snap up with raster2.
                      Pos p(spos,true);
                      song->setPos(0, p, true, true, true);
                  }
                  break;
            case CMD_LEFT_NOSNAP:
                  {
                    printf("left no snap\n");
                  int spos = pos[0] - editor->rasterStep(pos[0]);
                  if (spos < 0)
                        spos = 0;
                  Pos p(spos,true);
                  song->setPos(0, p, true, true, true); //CDW
                  }
                  break;
            case CMD_RIGHT_NOSNAP:
                  {
                  Pos p(pos[0] + editor->rasterStep(pos[0]), true);
                  song->setPos(0, p, true, true, true); //CDW
                  }
                  break;
            }
      updateSelection();
      redraw();
      }


//---------------------------------------------------------
//   copy
//    cut copy paste
//---------------------------------------------------------

void DrumCanvas::copy()
      {
      QMimeData* md = getTextDrag();
      
      if (md)
            QApplication::clipboard()->setMimeData(md, QClipboard::Clipboard);
      }


//---------------------------------------------------------
//   paste
//    paste events
//---------------------------------------------------------

void DrumCanvas::paste()
      {
      QString stype("x-muse-eventlist");
      
      //QString s = QApplication::clipboard()->text(stype, QClipboard::Selection);  
      QString s = QApplication::clipboard()->text(stype, QClipboard::Clipboard);  // TODO CHECK Tim.
      
      pasteAt(s, song->cpos());
      }

//---------------------------------------------------------
//   startDrag
//---------------------------------------------------------

void DrumCanvas::startDrag(CItem* /* item*/, bool copymode)
      {
      QMimeData* md = getTextDrag();
      
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

void DrumCanvas::dragEnterEvent(QDragEnterEvent* event)
      {
      event->acceptProposedAction();  // TODO CHECK Tim.
      }

//---------------------------------------------------------
//   dragMoveEvent
//---------------------------------------------------------

void DrumCanvas::dragMoveEvent(QDragMoveEvent*)
      {
      }

//---------------------------------------------------------
//   dragLeaveEvent
//---------------------------------------------------------

void DrumCanvas::dragLeaveEvent(QDragLeaveEvent*)
      {
      }


//---------------------------------------------------------
//   keyPressed - called from DList
//---------------------------------------------------------

void DrumCanvas::keyPressed(int index, int velocity)
      {
      // called from DList - play event
      int port = drumMap[index].port;
      int channel = drumMap[index].channel;
      int pitch = drumMap[index].anote;

      // play note:
      MidiPlayEvent e(0, port, channel, 0x90, pitch, velocity);
      audio->msgPlayMidiEvent(&e);
      }

//---------------------------------------------------------
//   keyReleased
//---------------------------------------------------------

void DrumCanvas::keyReleased(int index, bool)
      {
      // called from DList - silence playing event
      int port = drumMap[index].port;
      int channel = drumMap[index].channel;
      int pitch = drumMap[index].anote;

      // release note:
      MidiPlayEvent e(0, port, channel, 0x90, pitch, 0);
      audio->msgPlayMidiEvent(&e);
      }

//---------------------------------------------------------
//   mapChanged
//---------------------------------------------------------

void DrumCanvas::mapChanged(int spitch, int dpitch)
      {
      //TODO: Circumvent undo behaviour, since this isn't really a true change of the events,
      // but merely a change in pitch because the pitch relates to the order of the dlist.
      // Right now the sequencer spits out internalError: undoOp without startUndo() if start/stopundo is there, which is misleading
      // If start/stopundo is there, undo misbehaves since it doesn't undo but messes things up
      // Other solution: implement a specific undo-event for this (SC_DRUMMAP_MODIFIED or something) which undoes movement of
      // dlist-items (ml)

      std::vector< std::pair<Part*, Event*> > delete_events;
      std::vector< std::pair<Part*, Event> > add_events;
      
      typedef std::vector< std::pair<Part*, Event*> >::iterator idel_ev;
      typedef std::vector< std::pair<Part*, Event> >::iterator iadd_ev;
            
      MidiTrackList* tracks = song->midis();
      for (ciMidiTrack t = tracks->begin(); t != tracks->end(); t++) {
            MidiTrack* curTrack = *t;
            if (curTrack->type() != Track::DRUM)
                  continue;

            MidiPort* mp = &midiPorts[curTrack->outPort()];
            PartList* parts= curTrack->parts();
            for (iPart part = parts->begin(); part != parts->end(); ++part) {
                  EventList* events = part->second->events();
                  Part* thePart = part->second;
                  for (iEvent i = events->begin(); i != events->end(); ++i) {
                        Event event = i->second;
                        if(event.type() != Controller && event.type() != Note)
                          continue;
                        int pitch = event.pitch();
                        bool drc = false;
                        // Is it a drum controller event, according to the track port's instrument?
                        if(event.type() == Controller && mp->drumController(event.dataA()))
                        {
                          drc = true;
                          pitch = event.dataA() & 0x7f;
                        }
                        
                        if (pitch == spitch) {
                              Event* spitch_event = &(i->second);
                              delete_events.push_back(std::pair<Part*, Event*>(thePart, spitch_event));
                              Event newEvent = spitch_event->clone();
                              if(drc)
                                newEvent.setA((newEvent.dataA() & ~0xff) | dpitch);
                              else
                                newEvent.setPitch(dpitch);
                              add_events.push_back(std::pair<Part*, Event>(thePart, newEvent));
                              }
                        else if (pitch == dpitch) {
                              Event* dpitch_event = &(i->second);
                              delete_events.push_back(std::pair<Part*, Event*>(thePart, dpitch_event));
                              Event newEvent = dpitch_event->clone();
                              if(drc)
                                newEvent.setA((newEvent.dataA() & ~0xff) | spitch);
                              else
                                newEvent.setPitch(spitch);
                              add_events.push_back(std::pair<Part*, Event>(thePart, newEvent));
                              }
                        }
                  }
            }

      song->startUndo();
      for (idel_ev i = delete_events.begin(); i != delete_events.end(); i++) {
            Part*  thePart = (*i).first;
            Event* theEvent = (*i).second;
            // Indicate no undo, and do port controller values but not clone parts. 
            audio->msgDeleteEvent(*theEvent, thePart, false, true, false);
            }

      DrumMap dm = drumMap[spitch];
      drumMap[spitch] = drumMap[dpitch];
      drumMap[dpitch] = dm;
      drumInmap[int(drumMap[spitch].enote)]  = spitch;
      drumOutmap[int(drumMap[int(spitch)].anote)] = spitch;
      drumInmap[int(drumMap[int(dpitch)].enote)]  = dpitch;
      drumOutmap[int(drumMap[int(dpitch)].anote)] = dpitch;
            
      for (iadd_ev i = add_events.begin(); i != add_events.end(); i++) {
            Part*  thePart = (*i).first;
            Event& theEvent = (*i).second;
            // Indicate no undo, and do port controller values but not clone parts. 
            audio->msgAddEvent(theEvent, thePart, false, true, false);
            }
      
      song->endUndo(SC_EVENT_MODIFIED);
      song->update(SC_DRUMMAP);
      }

//---------------------------------------------------------
//   resizeEvent
//---------------------------------------------------------

void DrumCanvas::resizeEvent(QResizeEvent* ev)
      {
      if (ev->size().width() != ev->oldSize().width())
            emit newWidth(ev->size().width());
      EventCanvas::resizeEvent(ev);
      }


//---------------------------------------------------------
//   modifySelected
//---------------------------------------------------------

void DrumCanvas::modifySelected(NoteInfo::ValType type, int delta)
      {
      audio->msgIdle(true);
      song->startUndo();
      for (iCItem i = items.begin(); i != items.end(); ++i) {
            if (!(i->second->isSelected()))
                  continue;
            DEvent* e   = (DEvent*)(i->second);
            Event event = e->event();
            if (event.type() != Note)
                  continue;

            MidiPart* part = (MidiPart*)(e->part());
            Event newEvent = event.clone();

            switch (type) {
                  case NoteInfo::VAL_TIME:
                        {
                        int newTime = event.tick() + delta;
                        if (newTime < 0)
                           newTime = 0;
                        newEvent.setTick(newTime);
                        }
                        break;
                  case NoteInfo::VAL_LEN:
                        printf("DrumCanvas::modifySelected - NoteInfo::VAL_LEN not implemented\n");
                        break;
                  case NoteInfo::VAL_VELON:
                        printf("DrumCanvas::modifySelected - NoteInfo::VAL_VELON not implemented\n");
                        break;
                  case NoteInfo::VAL_VELOFF:
                        printf("DrumCanvas::modifySelected - NoteInfo::VAL_VELOFF not implemented\n");
                        break;
                  case NoteInfo::VAL_PITCH:
                        {
                        int pitch = event.pitch() - delta; // Reversing order since the drumlist is displayed in increasing order
                        if (pitch > 127)
                              pitch = 127;
                        else if (pitch < 0)
                              pitch = 0;
                        newEvent.setPitch(pitch);
                        }
                        break;
                  }
            song->changeEvent(event, newEvent, part);
            // Indicate do not do port controller values and clone parts. 
            song->addUndo(UndoOp(UndoOp::ModifyEvent, newEvent, event, part, false, false));
            }
      song->endUndo(SC_EVENT_MODIFIED);
      audio->msgIdle(false);
      }

//---------------------------------------------------------
//   curPartChanged
//---------------------------------------------------------

void DrumCanvas::curPartChanged()
      {
      editor->setWindowTitle(getCaption());
      }

//---------------------------------------------------------
//   getNextStep - gets next tick in the chosen direction
//                 when raster and stepSize are taken into account
//---------------------------------------------------------
int DrumCanvas::getNextStep(unsigned int pos, int basicStep, int stepSize)
{
  int newPos = pos;
  for (int i =0; i<stepSize;i++) {
    if (basicStep > 0) { // moving right
      newPos = AL::sigmap.raster2(newPos + basicStep, editor->rasterStep(newPos));    // Nudge by +1, then snap up with raster2.
      if (unsigned(newPos) > curPart->endTick()- editor->rasterStep(curPart->endTick()))
        newPos = curPart->tick();
    }
    else { // moving left
      newPos = AL::sigmap.raster1(newPos + basicStep, editor->rasterStep(newPos));    // Nudge by -1, then snap up with raster1.
      if (unsigned(newPos) < curPart->tick() ) {
        newPos = AL::sigmap.raster1(curPart->endTick()-1, editor->rasterStep(curPart->endTick()));
      }
    }
  }
  return newPos;
}

//---------------------------------------------------------
//   keyPress
//---------------------------------------------------------
void DrumCanvas::keyPress(QKeyEvent* event)
{
  if (_tool == CursorTool) {

    int key = event->key();
    if (((QInputEvent*)event)->modifiers() & Qt::ShiftModifier)
          key += Qt::SHIFT;
    if (((QInputEvent*)event)->modifiers() & Qt::AltModifier)
          key += Qt::ALT;
    if (((QInputEvent*)event)->modifiers() & Qt::ControlModifier)
          key+= Qt::CTRL;

    // Select items by key (PianoRoll & DrumEditor)
    if (key == shortcuts[SHRT_SEL_RIGHT].key) {
      cursorPos.setX(getNextStep(cursorPos.x(),1));

      selectCursorEvent(getEventAtCursorPos());
      if (mapx(cursorPos.x()) < 0 || mapx(cursorPos.x()) > width())
        emit followEvent(cursorPos.x());
      update();
      return;
    }
    else if (key == shortcuts[SHRT_SEL_LEFT].key) {
      cursorPos.setX(getNextStep(cursorPos.x(),-1));

      selectCursorEvent(getEventAtCursorPos());
      if (mapx(cursorPos.x()) < 0 || mapx(cursorPos.x()) > width())
        emit followEvent(cursorPos.x());
      update();
      return;
    }
    else if (key == shortcuts[SHRT_ADDNOTE_1].key) {
          newItem(newItem(cursorPos.x(), cursorPos.y(), drumMap[cursorPos.y()].lv1),false,true);
          keyPressed(cursorPos.y(), drumMap[cursorPos.y()].lv1);
          keyReleased(cursorPos.y(), false);
          cursorPos.setX(getNextStep(cursorPos.x(),1, _stepSize));
          selectCursorEvent(getEventAtCursorPos());
          if (mapx(cursorPos.x()) < 0 || mapx(cursorPos.x()) > width())
            emit followEvent(cursorPos.x());
          return;
    }
    else if (key == shortcuts[SHRT_ADDNOTE_2].key) {
          newItem(newItem(cursorPos.x(), cursorPos.y(), drumMap[cursorPos.y()].lv2),false,true);
          keyPressed(cursorPos.y(), drumMap[cursorPos.y()].lv2);
          keyReleased(cursorPos.y(), false);
          cursorPos.setX(getNextStep(cursorPos.x(),1, _stepSize));
          selectCursorEvent(getEventAtCursorPos());
          if (mapx(cursorPos.x()) < 0 || mapx(cursorPos.x()) > width())
            emit followEvent(cursorPos.x());
          return;
    }
    else if (key == shortcuts[SHRT_ADDNOTE_3].key) {
          newItem(newItem(cursorPos.x(), cursorPos.y(), drumMap[cursorPos.y()].lv3),false,true);
          keyPressed(cursorPos.y(), drumMap[cursorPos.y()].lv3);
          keyReleased(cursorPos.y(), false);
          cursorPos.setX(getNextStep(cursorPos.x(),1, _stepSize));
          selectCursorEvent(getEventAtCursorPos());
          if (mapx(cursorPos.x()) < 0 || mapx(cursorPos.x()) > width())
            emit followEvent(cursorPos.x());
          return;
    }
    else if (key == shortcuts[SHRT_ADDNOTE_4].key) {
          newItem(newItem(cursorPos.x(), cursorPos.y(), drumMap[cursorPos.y()].lv4),false,true);
          keyPressed(cursorPos.y(), drumMap[cursorPos.y()].lv4);
          keyReleased(cursorPos.y(), false);
          cursorPos.setX(getNextStep(cursorPos.x(),1, _stepSize));
          selectCursorEvent(getEventAtCursorPos());
          if (mapx(cursorPos.x()) < 0 || mapx(cursorPos.x()) > width())
            emit followEvent(cursorPos.x());
          return;
    }
  }
  EventCanvas::keyPress(event);
}


//---------------------------------------------------------
//   setTool2
//---------------------------------------------------------
void DrumCanvas::setTool2(int)
{
  if (_tool == CursorTool)
    deselectAll();
  if (unsigned(cursorPos.x()) < curPart->tick())
    cursorPos.setX(curPart->tick());
  update();
}
//---------------------------------------------------------
//   setCurDrumInstrument
//---------------------------------------------------------
void DrumCanvas::setCurDrumInstrument(int i)
{
  cursorPos.setY(i);
  update();
}

//---------------------------------------------------------
//   setStep
//---------------------------------------------------------
void DrumCanvas::setStep(int v)
{
  _stepSize=v;
}

//---------------------------------------------------------
//   getEventAtCursorPos
//---------------------------------------------------------
Event *DrumCanvas::getEventAtCursorPos()
{
    if (_tool != CursorTool)
      return 0;
    EventList* el = curPart->events();
    iEvent lower  = el->lower_bound(cursorPos.x()-curPart->tick());
    iEvent upper  = el->upper_bound(cursorPos.x()-curPart->tick());
    for (iEvent i = lower; i != upper; ++i) {
      Event &ev = i->second;
      if(!ev.isNote())
        continue;
      if (ev.pitch() == cursorPos.y()) {
        return &ev;
      }
    }
    return 0;
}
//---------------------------------------------------------
//   selectCursorEvent
//---------------------------------------------------------
void DrumCanvas::selectCursorEvent(Event *ev)
{
  for (iCItem i = items.begin(); i != items.end(); ++i)
  {
        Event e = i->second->event();

        if (ev && ev->tick() == e.tick() && ev->pitch() == e.pitch() && e.isNote())
          i->second->setSelected(true);
        else
          i->second->setSelected(false);

  }
  updateSelection();
}


void DrumCanvas::moveAwayUnused()
{
	using std::set;
	
	set<int> used;
	for (iCItem it=items.begin(); it!=items.end(); it++)
	{
		const Event& ev=it->second->event();
		
		if (ev.type()==Note)
			used.insert(ev.pitch());
	}
	
	int count=0;
	for (set<int>::iterator it=used.begin(); it!=used.end();)
	{
		while ((*it != count) && (used.find(count)!=used.end())) count++;
		
		if (*it != count)
			mapChanged(*it, count);

		count++;
		
		used.erase(it++);
	}
}
