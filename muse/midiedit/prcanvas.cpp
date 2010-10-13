//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: prcanvas.cpp,v 1.20.2.19 2009/11/16 11:29:33 lunar_shuttle Exp $
//  (C) Copyright 1999-2004 Werner Schweer (ws@seh.de)
//=========================================================

#include <qapplication.h>
#include <qclipboard.h>
#include <qpainter.h>
#include <q3dragobject.h>
#include <qmessagebox.h>
//Added by qt3to4:
#include <QDragLeaveEvent>
#include <Q3CString>
#include <QDragEnterEvent>
#include <QDragMoveEvent>
#include <QDropEvent>
#include <QMouseEvent>

#include <values.h>
#include <stdio.h>
#include <math.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <dirent.h>

#include "xml.h"
#include "prcanvas.h"
#include "midiport.h"
#include "event.h"
#include "mpevent.h"
#include "globals.h"
#include "cmd.h"
#include "gatetime.h"
#include "velocity.h"
#include "song.h"
#include "audio.h"

//---------------------------------------------------------
//   NEvent
//---------------------------------------------------------

NEvent::NEvent(Event& e, Part* p, int y) : CItem(e, p)
      {
      y = y - KH/4;
      unsigned tick = e.tick() + p->tick();
      setPos(QPoint(tick, y));
      setBBox(QRect(tick, y, e.lenTick(), KH/2));
      }

//---------------------------------------------------------
//   addItem
//---------------------------------------------------------

void PianoCanvas::addItem(Part* part, Event& event)
      {
      if (signed(event.tick())<0) {
            printf("ERROR: trying to add event before current part!\n");
            return;
      }

      NEvent* ev = new NEvent(event, part, pitch2y(event.pitch()));
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
//   PianoCanvas
//---------------------------------------------------------

PianoCanvas::PianoCanvas(MidiEditor* pr, QWidget* parent, int sx, int sy)
   : EventCanvas(pr, parent, sx, sy)
      {
      colorMode = 0;
      cmdRange  = 0;     // all Events
      playedPitch = -1;

      songChanged(SC_TRACK_INSERTED);
      connect(song, SIGNAL(midiNote(int, int)), SLOT(midiNote(int,int)));
      }

//---------------------------------------------------------
//   pitch2y
//---------------------------------------------------------

int PianoCanvas::pitch2y(int pitch) const
      {
      int tt[] = {
            5, 12, 19, 26, 33, 44, 51, 58, 64, 71, 78, 85
            };
      int y = (75 * KH) - (tt[pitch%12] + (7 * KH) * (pitch/12));
      if (y < 0)
            y = 0;
      return y;
      }

//---------------------------------------------------------
//   y2pitch
//---------------------------------------------------------

int PianoCanvas::y2pitch(int y) const
      {
      const int total = (10 * 7 + 5) * KH;       // 75 Ganztonschritte
      y = total - y;
      int oct = (y / (7 * KH)) * 12;
      char kt[] = {
            0, 0, 0, 0, 0,  0,   0, 0, 0, 0,          // 5
            1, 1, 1,      1,   1, 1, 1,               // 13
            2, 2,         2,   2, 2, 2,               // 19
            3, 3, 3,      3,   3, 3, 3,               // 26
            4, 4, 4, 4,   4,   4, 4, 4, 4,            // 34
            5, 5, 5, 5,   5,   5, 5, 5, 5, 5,         // 43
            6, 6, 6,      6,   6, 6, 6,               // 52
            7, 7,         7,   7, 7, 7,               // 58
            8, 8, 8,      8,   8, 8, 8,               // 65
            9, 9,         9,   9, 9, 9,               // 71
            10, 10, 10,  10,   10, 10, 10,            // 78
            11, 11, 11, 11, 11,   11, 11, 11, 11, 11  // 87
            };
      return kt[y % 91] + oct;
      }

//---------------------------------------------------------
//   drawEvent
//    draws a note
//---------------------------------------------------------

void PianoCanvas::drawItem(QPainter& p, const CItem* item,
   const QRect& rect)
      {
      QRect r = item->bbox();
      if(!virt())
        r.moveCenter(map(item->pos()));
      r = r.intersect(rect);
      if(!r.isValid())
        return;
      p.setPen(Qt::black);
      struct Triple {
            int r, g, b;
            };

      static Triple myColors /*Qt::color1*/[12] = {  // ddskrjp
            { 0xff, 0x3d, 0x39 },
            { 0x39, 0xff, 0x39 },
            { 0x39, 0x3d, 0xff },
            { 0xff, 0xff, 0x39 },
            { 0xff, 0x3d, 0xff },
            { 0x39, 0xff, 0xff },
            { 0xff, 0x7e, 0x7a },
            { 0x7a, 0x7e, 0xff },
            { 0x7a, 0xff, 0x7a },
            { 0xff, 0x7e, 0xbf },
            { 0x7a, 0xbf, 0xff },
            { 0xff, 0xbf, 0x7a }
            };

      NEvent* nevent   = (NEvent*) item;
      Event event = nevent->event();
      if (nevent->part() != curPart){
            if(item->isMoving()) 
              p.setBrush(Qt::gray);
            else if(item->isSelected()) 
              p.setBrush(Qt::black);
            else  
              p.setBrush(Qt::lightGray);
      }      
      else {
            if (item->isMoving()) {
                    p.setBrush(Qt::gray);
                }
            else if (item->isSelected()) {
                  p.setBrush(Qt::black);
                  }
            else {
                  QColor color;
                  color.setRgb(0, 0, 255);
                  switch(colorMode) {
                        case 0:
                              break;
                        case 1:     // pitch
                              {
                              Triple* c = &myColors/*Qt::color1*/[event.pitch() % 12];
                              color.setRgb(c->r, c->g, c->b);
                              }
                              break;
                        case 2:     // velocity
                              {
                              int velo = event.velo();
                              if (velo < 64)
                                    color.setRgb(velo*4, 0, 0xff);
                              else
                                    color.setRgb(0xff, 0, (127-velo) * 4);
                              }
                              break;
                        }
                  p.setBrush(color);
                  }
            }
      p.drawRect(r);
      }


//---------------------------------------------------------
//   drawMoving
//    draws moving items
//---------------------------------------------------------

void PianoCanvas::drawMoving(QPainter& p, const CItem* item, const QRect& rect)
    {
      //if(((NEvent*)item)->part() != curPart)
      //  return;
      //if(!item->isMoving()) 
      //  return;
      QRect mr = QRect(item->mp().x(), item->mp().y() - item->height()/2, item->width(), item->height());
      mr = mr.intersect(rect);
      if(!mr.isValid())
        return;
      p.setPen(Qt::black);
      p.setBrush(Qt::NoBrush);
      p.drawRect(mr);
    }

//---------------------------------------------------------
//   viewMouseDoubleClickEvent
//---------------------------------------------------------

void PianoCanvas::viewMouseDoubleClickEvent(QMouseEvent* event)
      {
      if ((_tool != PointerTool) && (event->button() != Qt::LeftButton)) {
            mousePress(event);
            return;
            }
      }

//---------------------------------------------------------
//   moveCanvasItems
//---------------------------------------------------------

void PianoCanvas::moveCanvasItems(CItemList& items, int dp, int dx, DragType dtype, int* pflags)
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
      NEvent* nevent = (NEvent*) ci;
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
      // Create new part...
      // if there are several events that are moved outside the part, it will be recreated for each
      // so the part _in_ the event will not be valid, ask the authority.
//      Part* newPart = part->clone();
      //Part* newPart = Canvas::part()->clone();

//      newPart->setLenTick(newPart->lenTick() + npartoffset);
      //audio->msgChangePart(part, newPart,false);

//      modified = SC_PART_MODIFIED;

      // BUG FIX: #1650953
      // Added by T356.
      // Fixes posted "select and drag past end of part - crashing" bug
//      for(iPart ip = editor->parts()->begin(); ip != editor->parts()->end(); ++ip)
//      {
//        if(ip->second == part)
//        {
//          editor->parts()->erase(ip);
//          break;
//        }
//      }
      
//      editor->parts()->add(newPart);
//      audio->msgChangePart(part, newPart,false);
      
      //if(parts2change.find(part) == parts2change.end())
      //  parts2change.insert(std::pair<Part*, Part*> (part, newPart));
      iPartToChange ip2c = parts2change.find(part);
      if(ip2c == parts2change.end())
      {
        PartToChange p2c = {0, npartoffset};
        parts2change.insert(std::pair<Part*, PartToChange> (part, p2c));
      }
      else
        ip2c->second.xdiff = npartoffset;
      
      //part = newPart; // reassign
      //item->setPart(part);
      //item->setEvent(newEvent);
      //curPart = part;
      //curPartId = curPart->sn();

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
    //if(moveItem(ci, newpos, dtype))
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
          
    if(moving.size() == 1) 
          itemReleased(curItem, newpos);
    if(dtype == MOVE_COPY || dtype == MOVE_CLONE)
          selectItem(ci, false);
  }  
      
  if(pflags)
    *pflags = modified;
}
      
//---------------------------------------------------------
//   moveItem
//    called after moving an object
//---------------------------------------------------------

// Changed by T356. 
//bool PianoCanvas::moveItem(CItem* item, const QPoint& pos, DragType dtype, int* pflags)
bool PianoCanvas::moveItem(CItem* item, const QPoint& pos, DragType dtype)
      {
      NEvent* nevent = (NEvent*) item;
      Event event    = nevent->event();
      int npitch     = y2pitch(pos.y());
      Event newEvent = event.clone();
      int x          = pos.x();
      if (x < 0)
            x = 0;
      if (event.pitch() != npitch && _playEvents) {
            int port    = track()->outPort();
            int channel = track()->outChannel();
            // release note:
            MidiPlayEvent ev1(0, port, channel, 0x90, event.pitch() + track()->transposition, 0);
            audio->msgPlayMidiEvent(&ev1);
            MidiPlayEvent ev2(0, port, channel, 0x90, npitch + track()->transposition, event.velo());
            audio->msgPlayMidiEvent(&ev2);
            }
      
      // Changed by T356. 
      Part* part = nevent->part(); //
      //Part * part = Canvas::part();  // part can be dynamically recreated, ask the authority
      
      newEvent.setPitch(npitch);
      int ntick = editor->rasterVal(x) - part->tick();
      if (ntick < 0)
            ntick = 0;
      newEvent.setTick(ntick);
      newEvent.setLenTick(event.lenTick());

      // Removed by T356. 
      /*
      int modified=0;
      //song->startUndo();
      int diff = newEvent.endTick()-part->lenTick();
      if (diff > 0){// too short part? extend it
            // if there are several events that are moved outside the part, it will be recreated for each
            // so the part _in_ the event will not be valid, ask the authority.
            //Part* newPart = part->clone();
            Part* newPart = Canvas::part()->clone();

            newPart->setLenTick(newPart->lenTick()+diff);
            audio->msgChangePart(Canvas::part(), newPart,false);
      
            modified = SC_PART_MODIFIED;
            part = newPart; // reassign

            // BUG FIX: #1650953
            // Added by T356.
            // Fixes posted "select and drag past end of part - crashing" bug
            for(iPart i = editor->parts()->begin(); i != editor->parts()->end(); ++i)
            {
              if(i->second == Canvas::part())
              {
                editor->parts()->erase(i);
                break;
              }
            }
            editor->parts()->add(part);
            item->setPart(part);
            item->setEvent(newEvent);
            curPart = part;
            curPartId = curPart->sn();

            }
      */

      // Added by T356. 
      // msgAddEvent and msgChangeEvent (below) will set these, but set them here first?
      //item->setPart(part);
      item->setEvent(newEvent);
      
      // Added by T356. 
      if(((int)newEvent.endTick() - (int)part->lenTick()) > 0)  
        printf("PianoCanvas::moveItem Error! New event end:%d exceeds length:%d of part:%s\n", newEvent.endTick(), part->lenTick(), part->name().latin1());
      
      if (dtype == MOVE_COPY || dtype == MOVE_CLONE)
            // Indicate no undo, and do not do port controller values and clone parts. 
            //audio->msgAddEvent(newEvent, part, false);
            audio->msgAddEvent(newEvent, part, false, false, false);
      else
            // Indicate no undo, and do not do port controller values and clone parts. 
            //audio->msgChangeEvent(event, newEvent, part, false);
            audio->msgChangeEvent(event, newEvent, part, false, false, false);
      //song->endUndo(modified);

      // Removed by T356. 
      //if(pflags)
      //  *pflags = modified;
      
      return true;
      }

//---------------------------------------------------------
//   newItem(p, state)
//---------------------------------------------------------

CItem* PianoCanvas::newItem(const QPoint& p, int)
      {
      //printf("newItem point\n");
      int pitch = y2pitch(p.y());
      int tick  = editor->rasterVal1(p.x());
      int len   = p.x() - tick;
      tick     -= curPart->tick();
      if (tick < 0)
            tick=0;
      Event e =  Event(Note);
      e.setTick(tick);
      e.setPitch(pitch);
      e.setVelo(curVelo);
      e.setLenTick(len);
      return new NEvent(e, curPart, pitch2y(pitch));
      }

void PianoCanvas::newItem(CItem* item, bool noSnap)
      {
      //printf("newItem citem\n");
      NEvent* nevent = (NEvent*) item;
      Event event    = nevent->event();
      int x = item->x();
      if (x<0)
            x=0;
      int w = item->width();

      if (!noSnap) {
            x = editor->rasterVal1(x); //round down
            w = editor->rasterVal(x + w) - x;
            if (w == 0)
                  w = editor->raster();
            }
      Part* part = nevent->part();
      event.setTick(x - part->tick());
      event.setLenTick(w);
      event.setPitch(y2pitch(item->y()));

      song->startUndo();
      int modified=SC_EVENT_MODIFIED;
      int diff = event.endTick()-part->lenTick();
      if (diff > 0)  {// too short part? extend it
            //printf("extend Part!\n");
            Part* newPart = part->clone();
            newPart->setLenTick(newPart->lenTick()+diff);
            // Indicate no undo, and do port controller values but not clone parts. 
            //audio->msgChangePart(part, newPart,false);
            audio->msgChangePart(part, newPart, false, true, false);
            modified=modified|SC_PART_MODIFIED;
            part = newPart; // reassign
            }
      // Indicate no undo, and do not do port controller values and clone parts. 
      //audio->msgAddEvent(event, part,false);
      audio->msgAddEvent(event, part, false, false, false);
      song->endUndo(modified);
      }

//---------------------------------------------------------
//   resizeItem
//---------------------------------------------------------

void PianoCanvas::resizeItem(CItem* item, bool noSnap)         // experimental changes to try dynamically extending parts
      {
      //printf("resizeItem!\n");
      NEvent* nevent = (NEvent*) item;
      Event event    = nevent->event();
      Event newEvent = event.clone();
      int len;

      Part* part = nevent->part();

      if (noSnap)
            len = nevent->width();
      else {
            //Part* part = nevent->part();
            unsigned tick = event.tick() + part->tick();
            len = editor->rasterVal(tick + nevent->width()) - tick;
            if (len <= 0)
                  len = editor->raster();
      }
      song->startUndo();
      int modified=SC_EVENT_MODIFIED;
      //printf("event.tick()=%d len=%d part->lenTick()=%d\n",event.endTick(),len,part->lenTick());
      int diff = event.tick()+len-part->lenTick();
      if (diff > 0)  {// too short part? extend it
            //printf("extend Part!\n");
            Part* newPart = part->clone();
            newPart->setLenTick(newPart->lenTick()+diff);
            // Indicate no undo, and do port controller values but not clone parts. 
            //audio->msgChangePart(part, newPart,false);
            audio->msgChangePart(part, newPart, false, true, false);
            modified=modified|SC_PART_MODIFIED;
            part = newPart; // reassign
            }

      newEvent.setLenTick(len);
      // Indicate no undo, and do not do port controller values and clone parts. 
      //audio->msgChangeEvent(event, newEvent, nevent->part(),false);
      audio->msgChangeEvent(event, newEvent, nevent->part(), false, false, false);
      song->endUndo(modified);
      }

//---------------------------------------------------------
//   deleteItem
//---------------------------------------------------------

bool PianoCanvas::deleteItem(CItem* item)
      {
      NEvent* nevent = (NEvent*) item;
      if (nevent->part() == curPart) {
            Event ev = nevent->event();
            // Indicate do undo, and do not do port controller values and clone parts. 
            //audio->msgDeleteEvent(ev, curPart);
            audio->msgDeleteEvent(ev, curPart, true, false, false);
            return true;
            }
      return false;
      }

//---------------------------------------------------------
//   pianoCmd
//---------------------------------------------------------

void PianoCanvas::pianoCmd(int cmd)
      {
      switch(cmd) {
            case CMD_LEFT:
                  {
                  int frames = pos[0] - editor->rasterStep(pos[0]);
                  if (frames < 0)
                        frames = 0;
                  Pos p(frames,true);
                  song->setPos(0, p, true, true, true); //CDW
                  }
                  break;
            case CMD_RIGHT:
                  {
                  Pos p(pos[0] + editor->rasterStep(pos[0]), true);
                  //if (p > part->tick())
                  //      p = part->tick();
                  song->setPos(0, p, true, true, true); //CDW
                  }
                  break;
            case CMD_INSERT:
                  {
                  if (pos[0] < start() || pos[0] >= end())
                        break;
                  MidiPart* part = (MidiPart*)curPart;

                  if (part == 0)
                        break;
                  song->startUndo();
                  EventList* el = part->events();

                  std::list <Event> elist;
                  for (iEvent e = el->lower_bound(pos[0] - part->tick()); e != el->end(); ++e)
                        elist.push_back((Event)e->second);
                  for (std::list<Event>::iterator i = elist.begin(); i != elist.end(); ++i) {
                        Event event = *i;
                        Event newEvent = event.clone();
                        newEvent.setTick(event.tick() + editor->raster());// - part->tick());
                        // Indicate no undo, and do not do port controller values and clone parts. 
                        //audio->msgChangeEvent(event, newEvent, part, false);
                        audio->msgChangeEvent(event, newEvent, part, false, false, false);
                        }
                  song->endUndo(SC_EVENT_MODIFIED);
                  Pos p(editor->rasterVal(pos[0] + editor->rasterStep(pos[0])), true);
                  song->setPos(0, p, true, false, true);
                  }
                  return;
            case CMD_DELETE:
                  if (pos[0] < start() || pos[0] >= end())
                        break;
                  {
                  MidiPart* part = (MidiPart*)curPart;
                  if (part == 0)
                        break;
                  song->startUndo();
                  EventList* el = part->events();

                  std::list<Event> elist;
                  for (iEvent e = el->lower_bound(pos[0]); e != el->end(); ++e)
                        elist.push_back((Event)e->second);
                  for (std::list<Event>::iterator i = elist.begin(); i != elist.end(); ++i) {
                        Event event = *i;
                        Event newEvent = event.clone();
                        newEvent.setTick(event.tick() - editor->raster() - part->tick());
                        // Indicate no undo, and do not do port controller values and clone parts. 
                        //audio->msgChangeEvent(event, newEvent, part, false);
                        audio->msgChangeEvent(event, newEvent, part, false, false, false);
                        }
                  song->endUndo(SC_EVENT_MODIFIED);
                  Pos p(editor->rasterVal(pos[0] - editor->rasterStep(pos[0])), true);
                  song->setPos(0, p, true, false, true);
                  }
                  break;
            }
      }

//---------------------------------------------------------
//   pianoPressed
//---------------------------------------------------------

void PianoCanvas::pianoPressed(int pitch, int velocity, bool shift)
      {
      int port      = track()->outPort();
      int channel   = track()->outChannel();
      pitch        += track()->transposition;

      // play note:
      //MidiPlayEvent e(0, port, channel, 0x90, pitch, 127);
      MidiPlayEvent e(0, port, channel, 0x90, pitch, velocity);
      audio->msgPlayMidiEvent(&e);

      if (_steprec && pos[0] >= start_tick && pos[0] < end_tick) {
            if (curPart == 0)
                  return;
            int len  = editor->raster();
            unsigned tick = pos[0] - curPart->tick(); //CDW
            if (shift)
                  tick -= editor->rasterStep(tick);
            Event e(Note);
            e.setTick(tick);
            e.setPitch(pitch);
            e.setVelo(127);
            e.setLenTick(len);
            // Indicate do undo, and do not do port controller values and clone parts. 
            //audio->msgAddEvent(e, curPart);
            audio->msgAddEvent(e, curPart, true, false, false);
            tick += editor->rasterStep(tick) + curPart->tick();
            if (tick != song->cpos()) {
                  Pos p(tick, true);
                  song->setPos(0, p, true, false, true);
                  }
            }
      }

//---------------------------------------------------------
//   pianoReleased
//---------------------------------------------------------

void PianoCanvas::pianoReleased(int pitch, bool)
      {
      int port    = track()->outPort();
      int channel = track()->outChannel();
      pitch      += track()->transposition;

      // release key:
      MidiPlayEvent e(0, port, channel, 0x90, pitch, 0);
      audio->msgPlayMidiEvent(&e);
      }

//---------------------------------------------------------
//   drawTickRaster
//---------------------------------------------------------

void drawTickRaster(QPainter& p, int x, int y, int w, int h, int raster)
      {
      int bar1, bar2, beat;
      unsigned tick;
      sigmap.tickValues(x, &bar1, &beat, &tick);
      sigmap.tickValues(x+w, &bar2, &beat, &tick);
      ++bar2;
      int y2 = y + h;
      for (int bar = bar1; bar < bar2; ++bar) {
            unsigned x = sigmap.bar2tick(bar, 0, 0);
            p.setPen(Qt::black);
            p.drawLine(x, y, x, y2);
            int z, n;
            sigmap.timesig(x, z, n);
            int q = p.xForm(QPoint(raster, 0)).x() - p.xForm(QPoint(0, 0)).x();
            int qq = raster;
            if (q < 8)        // grid too dense
                  qq *= 2;
            //switch (quant) {
            //      case 32:
            //      case 48:
            //      case 64:
            //      case 96:
            //      case 192:         // 8tel
            //      case 128:         // 8tel Triolen
            //      case 288:
            p.setPen(Qt::lightGray);
            if (raster>=4) {
                        int xx = x + qq;
                        int xxx = sigmap.bar2tick(bar, z, 0);
                        while (xx <= xxx) {
                               p.drawLine(xx, y, xx, y2);
                               xx += qq;
                               }
                        xx = xxx;
                        }
            //            break;
            //      default:
            //            break;
            // }
            p.setPen(Qt::gray);
            for (int beat = 1; beat < z; beat++) {
                        int xx = sigmap.bar2tick(bar, beat, 0);
                        p.drawLine(xx, y, xx, y2);
                        }

            }
      }

//---------------------------------------------------------
//   draw
//---------------------------------------------------------

void PianoCanvas::drawCanvas(QPainter& p, const QRect& rect)
      {
      int x = rect.x();
      int y = rect.y();
      int w = rect.width();
      int h = rect.height();

      //---------------------------------------------------
      //  horizontal lines
      //---------------------------------------------------

      int yy  = ((y-1) / KH) * KH + KH;
      int key = 75 - (yy / KH);
      for (; yy < y + h; yy += KH) {
            switch (key % 7) {
                  case 0:
                  case 3:
                        p.setPen(Qt::black);
                        p.drawLine(x, yy, x + w, yy);
                        break;
                  default:
                        //p.setPen(lightGray);
                        p.fillRect(x, yy-3, w, 6, QBrush(QColor(230,230,230)));
                        //p.drawLine(x, yy, x + w, yy);
                        break;
                  }
            --key;
            }

      //---------------------------------------------------
      // vertical lines
      //---------------------------------------------------

      drawTickRaster(p, x, y, w, h, editor->raster());
      }

//---------------------------------------------------------
//   cmd
//    pulldown menu commands
//---------------------------------------------------------

void PianoCanvas::cmd(int cmd, int quantStrength,
   int quantLimit, bool quantLen, int range)
      {
      cmdRange = range;
      switch (cmd) {
            case CMD_CUT:
                  copy();
                  song->startUndo();
                  for (iCItem i = items.begin(); i != items.end(); ++i) {
                        if (!(i->second->isSelected()))
                              continue;
                        NEvent* e = (NEvent*)(i->second);
                        Event ev  = e->event();
                        // Indicate no undo, and do not do port controller values and clone parts. 
                        //audio->msgDeleteEvent(ev, e->part(), false);
                        audio->msgDeleteEvent(ev, e->part(), false, false, false);
                        }
                  song->endUndo(SC_EVENT_REMOVED);
                  break;
            case CMD_COPY:
                  copy();
                  break;
            case CMD_PASTE:
                  paste();
                  break;
            case CMD_DEL:
                  if (selectionSize()) {
                        song->startUndo();
                        for (iCItem i = items.begin(); i != items.end(); ++i) {
                              if (!i->second->isSelected())
                                    continue;
                              Event ev = i->second->event();
                              // Indicate no undo, and do not do port controller values and clone parts. 
                              //audio->msgDeleteEvent(ev, i->second->part(), false);
                              audio->msgDeleteEvent(ev, i->second->part(), false, false, false);
                              }
                        song->endUndo(SC_EVENT_REMOVED);
                        }
                  return;
            case CMD_OVER_QUANTIZE:     // over quantize
                  quantize(100, 1, quantLen);
                  break;
            case CMD_ON_QUANTIZE:     // note on quantize
                  quantize(50, 1, false);
                  break;
            case CMD_ONOFF_QUANTIZE:     // note on/off quantize
                  quantize(50, 1, true);
                  break;
            case CMD_ITERATIVE_QUANTIZE:     // Iterative Quantize
                  quantize(quantStrength, quantLimit, quantLen);
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
                        NEvent* nevent = (NEvent*)(k->second);
                        Part* part     = nevent->part();
                        Event event    = nevent->event();
                        unsigned tick  = event.tick() + part->tick();
                        if (tick < song->lpos() || tick >= song->rpos())
                              selectItem(k->second, false);
                        else
                              selectItem(k->second, true);
                        }
                  break;
            case CMD_SELECT_OLOOP:     // select outside loop
                  for (iCItem k = items.begin(); k != items.end(); ++k) {
                        NEvent* nevent = (NEvent*)(k->second);
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
            case CMD_MODIFY_GATE_TIME:
                  {
                  GateTime w(this);
                  w.setRange(range);
                  if (!w.exec())
                        break;
                  int range  = w.range();        // all, selected, looped, sel+loop
                  int rate   = w.rateVal();
                  int offset = w.offsetVal();

                  song->startUndo();
                  for (iCItem k = items.begin(); k != items.end(); ++k) {
                        NEvent* nevent =(NEvent*)(k->second);
                        Event event    = nevent->event();
                        if (event.type() != Note)
                              continue;
                        unsigned tick = event.tick();
                        bool selected = k->second->isSelected();
                        bool inLoop   = (tick >= song->lpos()) && (tick < song->rpos());

                        if ((range == 0)
                           || (range == 1 && selected)
                           || (range == 2 && inLoop)
                           || (range == 3 && selected && inLoop)) {
                              unsigned int len   = event.lenTick(); //prevent compiler warning: comparison singed/unsigned

                              len = rate ? (len * 100) / rate : 1;
                              len += offset;
                              if (len < 1)
                                    len = 1;

                              if (event.lenTick() != len) {
                                    Event newEvent = event.clone();
                                    newEvent.setLenTick(len);
                                    // Indicate no undo, and do not do port controller values and clone parts. 
                                    //audio->msgChangeEvent(event, newEvent, nevent->part(), false);
                                    audio->msgChangeEvent(event, newEvent, nevent->part(), false, false, false);
                                    }
                              }
                        }
                  song->endUndo(SC_EVENT_MODIFIED);
                  }
                  break;

            case CMD_MODIFY_VELOCITY:
                  {
                  Velocity w(this);
                  w.setRange(range);
                  if (!w.exec())
                        break;
                  int range  = w.range();        // all, selected, looped, sel+loop
                  int rate   = w.rateVal();
                  int offset = w.offsetVal();

                  song->startUndo();
                  for (iCItem k = items.begin(); k != items.end(); ++k) {
                        NEvent* nevent = (NEvent*)(k->second);
                        Event event    = nevent->event();
                        if (event.type() != Note)
                              continue;
                        unsigned tick      = event.tick();
                        bool selected = k->second->isSelected();
                        bool inLoop   = (tick >= song->lpos()) && (tick < song->rpos());

                        if ((range == 0)
                           || (range == 1 && selected)
                           || (range == 2 && inLoop)
                           || (range == 3 && selected && inLoop)) {
                              int velo = event.velo();

                              //velo = rate ? (velo * 100) / rate : 64;
                              velo = (velo * rate) / 100;
                              velo += offset;

                              if (velo <= 0)
                                    velo = 1;
                              if (velo > 127)
                                    velo = 127;
                              if (event.velo() != velo) {
                                    Event newEvent = event.clone();
                                    newEvent.setVelo(velo);
                                    // Indicate no undo, and do not do port controller values and clone parts. 
                                    //audio->msgChangeEvent(event, newEvent, nevent->part(), false);
                                    audio->msgChangeEvent(event, newEvent, nevent->part(), false, false, false);
                                    }
                              }
                        }
                  song->endUndo(SC_EVENT_MODIFIED);
                  }
                  break;

            case CMD_FIXED_LEN: //Set notes to the length specified in the drummap
                  if (!selectionSize())
                        break;
                  song->startUndo();
                  for (iCItem k = items.begin(); k != items.end(); ++k) {
                        if (k->second->isSelected()) {
                              NEvent* nevent = (NEvent*)(k->second);
                              Event event = nevent->event();
                              Event newEvent = event.clone();
                              newEvent.setLenTick(editor->raster());
                              // Indicate no undo, and do not do port controller values and clone parts. 
                              //audio->msgChangeEvent(event, newEvent, nevent->part() , false);
                              audio->msgChangeEvent(event, newEvent, nevent->part(), false, false, false);
                              }
                        }
                  song->endUndo(SC_EVENT_MODIFIED);
                  break;

            case CMD_DELETE_OVERLAPS:
                  if (!selectionSize())
                        break;

                  song->startUndo();
                  for (iCItem k = items.begin(); k != items.end(); k++) {
                      if (k->second->isSelected() == false)
                          continue;

                      NEvent* e1 = (NEvent*) (k->second); // first note
                      NEvent* e2 = NULL; // ptr to next selected note (which will be checked for overlap)
                      Event ce1  = e1->event();
                      Event ce2;

                      if (ce1.type() != Note)
                          continue;

                      // Find next selected item on the same pitch
                      iCItem l = k; l++;
                      for (; l != items.end(); l++) {
                          if (l->second->isSelected() == false)
                              continue;

                          e2 = (NEvent*) l->second;
                          ce2  = e2->event();

                          // Same pitch?
                          if (ce1.dataA() == ce2.dataA())
                              break;

                          // If the note has the same len and place we treat it as a duplicate note and not a following note
                          // The best thing to do would probably be to delete the duplicate note, we just want to avoid
                          // matching against the same note
                          if (   ce1.tick() + e1->part()->tick() == ce2.tick() + e2->part()->tick() 
                              && ce1.lenTick() + e1->part()->tick() == ce2.lenTick() + e2->part()->tick())
                          {
                              e2 = NULL; // this wasn't what we were looking for
                              continue;
                          }

                          }

                      if (e2 == NULL) // None found
                          break;

                      Part* part1 = e1->part();
                      Part* part2 = e2->part();
                      if (ce2.type() != Note)
                          continue;


                      unsigned event1pos = ce1.tick() + part1->tick();
                      unsigned event1end = event1pos + ce1.lenTick();
                      unsigned event2pos = ce2.tick() + part2->tick();

                      //printf("event1pos %u event1end %u event2pos %u\n", event1pos, event1end, event2pos);
                      if (event1end > event2pos) {
                          Event newEvent = ce1.clone();
                          unsigned newlen = ce1.lenTick() - (event1end - event2pos);
                          //printf("newlen: %u\n", newlen);
                          newEvent.setLenTick(newlen);
                          // Indicate no undo, and do not do port controller values and clone parts. 
                          //audio->msgChangeEvent(ce1, newEvent, e1->part(), false);
                          audio->msgChangeEvent(ce1, newEvent, e1->part(), false, false, false);
                          }
                      }
                  song->endUndo(SC_EVENT_MODIFIED);
                  break;


            case CMD_CRESCENDO:
            case CMD_TRANSPOSE:
            case CMD_THIN_OUT:
            case CMD_ERASE_EVENT:
            case CMD_NOTE_SHIFT:
            case CMD_MOVE_CLOCK:
            case CMD_COPY_MEASURE:
            case CMD_ERASE_MEASURE:
            case CMD_DELETE_MEASURE:
            case CMD_CREATE_MEASURE:
                  break;
            default:
//                  printf("unknown ecanvas cmd %d\n", cmd);
                  break;
            }
      updateSelection();
      redraw();
      }

//---------------------------------------------------------
//   quantize
//---------------------------------------------------------

void PianoCanvas::quantize(int strength, int limit, bool quantLen)
      {
      song->startUndo();
      for (iCItem k = items.begin(); k != items.end(); ++k) {
            NEvent* nevent = (NEvent*)(k->second);
            Event event    = nevent->event();
            Part* part     = nevent->part();
            if (event.type() != Note)
                  continue;

            if ((cmdRange & CMD_RANGE_SELECTED) && !k->second->isSelected())
                  continue;

            unsigned tick = event.tick() + part->tick();

            if ((cmdRange & CMD_RANGE_LOOP)
               && ((tick < song->lpos() || tick >= song->rpos())))
                  continue;

            unsigned int len   = event.lenTick(); //prevent compiler warning: comparison singed/unsigned
            int tick2 = tick + len;

            // quant start position
            int diff  = sigmap.raster(tick, editor->quant()) - tick;
            if (abs(diff) > limit)
                  tick += ((diff * strength) / 100);

            // quant len
            diff = sigmap.raster(tick2, editor->quant()) - tick2;
            if (quantLen && (abs(diff) > limit))
                  len += ((diff * strength) / 100);

            // something changed?
            if (((event.tick() + part->tick()) != tick) || (event.lenTick() != len)) {
                  Event newEvent = event.clone();
                  newEvent.setTick(tick - part->tick());
                  newEvent.setLenTick(len);
                  // Indicate no undo, and do not do port controller values and clone parts. 
                  //audio->msgChangeEvent(event, newEvent, part, false);
                  audio->msgChangeEvent(event, newEvent, part, false, false, false);
                  }
            }
      song->endUndo(SC_EVENT_MODIFIED);
      }

//---------------------------------------------------------
//   midiNote
//---------------------------------------------------------

void PianoCanvas::midiNote(int pitch, int velo)
      {
      if (_midiin && _steprec && curPart
         && !audio->isPlaying() && velo && pos[0] >= start_tick
         && pos[0] < end_tick
         && !(globalKeyState & Qt::AltButton)) {
            unsigned int len   = editor->quant();//prevent compiler warning: comparison singed/unsigned
            unsigned tick      = pos[0]; //CDW
            unsigned starttick = tick;
            if (globalKeyState & Qt::ShiftButton)
                  tick -= editor->rasterStep(tick);

            //
            // extend len of last note?
            //
            EventList* events = curPart->events();
            if (globalKeyState & Qt::ControlButton) {
                  for (iEvent i = events->begin(); i != events->end(); ++i) {
                        Event ev = i->second;
                        if (!ev.isNote())
                              continue;
                        if (ev.pitch() == pitch && ((ev.tick() + ev.lenTick()) == /*(int)*/starttick)) {
                              Event e = ev.clone();
                              e.setLenTick(ev.lenTick() + editor->rasterStep(starttick));
                              // Indicate do undo, and do not do port controller values and clone parts. 
                              //audio->msgChangeEvent(ev, e, curPart);
                              audio->msgChangeEvent(ev, e, curPart, true, false, false);
                              tick += editor->rasterStep(tick);
                              if (tick != song->cpos()) {
                                    Pos p(tick, true);
                                    song->setPos(0, p, true, false, true);
                                    }
                              return;
                              }
                        }
                  }

            //
            // if we already entered the note, delete it
            //
            EventRange range = events->equal_range(tick);
            for (iEvent i = range.first; i != range.second; ++i) {
                  Event ev = i->second;
                  if (ev.isNote() && ev.pitch() == pitch) {
                        // Indicate do undo, and do not do port controller values and clone parts. 
                        //audio->msgDeleteEvent(ev, curPart);
                        audio->msgDeleteEvent(ev, curPart, true, false, false);
                        if (globalKeyState & Qt::ShiftButton)
                              tick += editor->rasterStep(tick);
                        return;
                        }
                  }
            Event e(Note);
            e.setTick(tick - curPart->tick());
            e.setPitch(pitch);
            e.setVelo(velo);
            e.setLenTick(len);
            // Indicate do undo, and do not do port controller values and clone parts. 
            //audio->msgAddEvent(e, curPart);
            audio->msgAddEvent(e, curPart, true, false, false);
            tick += editor->rasterStep(tick);
            if (tick != song->cpos()) {
                  Pos p(tick, true);
                  song->setPos(0, p, true, false, true);
                  }
            }
      }

//---------------------------------------------------------
//   getTextDrag
//---------------------------------------------------------

Q3TextDrag* PianoCanvas::getTextDrag(QWidget* parent)
      {
      //---------------------------------------------------
      //   generate event list from selected events
      //---------------------------------------------------

      EventList el;
      unsigned startTick = MAXINT;
      for (iCItem i = items.begin(); i != items.end(); ++i) {
            if (!i->second->isSelected())
                  continue;
            NEvent* ne = (NEvent*)(i->second);
            Event e   = ne->event();
            if (startTick == MAXINT)
                  startTick = e.tick();
            el.add(e);
            }

      //---------------------------------------------------
      //    write events as XML into tmp file
      //---------------------------------------------------

      FILE* tmp = tmpfile();
      if (tmp == 0) {
            fprintf(stderr, "PianoCanvas::copy() fopen failed: %s\n",
               strerror(errno));
            return 0;
            }
      Xml xml(tmp);

      int level = 0;
      xml.tag(level++, "eventlist");
      for (ciEvent e = el.begin(); e != el.end(); ++e)
            e->second.write(level, xml, -startTick);
      xml.etag(--level, "eventlist");

      //---------------------------------------------------
      //    read tmp file into QTextDrag Object
      //---------------------------------------------------

      fflush(tmp);
      struct stat f_stat;
      if (fstat(fileno(tmp), &f_stat) == -1) {
            fprintf(stderr, "PianoCanvas::copy() fstat failes:<%s>\n",
               strerror(errno));
            fclose(tmp);
            return 0;
            }
      int n = f_stat.st_size;
      char* fbuf  = (char*)mmap(0, n+1, PROT_READ|PROT_WRITE,
         MAP_PRIVATE, fileno(tmp), 0);
      fbuf[n] = 0;
      Q3TextDrag* drag = new Q3TextDrag(QString(fbuf), parent);
      drag->setSubtype("eventlist");
      munmap(fbuf, n);
      fclose(tmp);
      return drag;
      }

//---------------------------------------------------------
//   copy
//    cut copy paste
//---------------------------------------------------------

void PianoCanvas::copy()
      {
      Q3TextDrag* drag = getTextDrag(0);
      if (drag)
            QApplication::clipboard()->setData(drag, QClipboard::Clipboard);
      }

//---------------------------------------------------------
//   paste
//---------------------------------------------------------

void PianoCanvas::pasteAt(const QString& pt, int pos)
      {
      const char* p = pt.latin1();
      Xml xml(p);
      for (;;) {
            Xml::Token token = xml.parse();
            const QString& tag = xml.s1();
            switch (token) {
                  case Xml::Error:
                  case Xml::End:
                        return;
                  case Xml::TagStart:
                        if (tag == "eventlist") {
                              song->startUndo();
                              EventList* el = new EventList();
                              el->read(xml, "eventlist", true);
                              int modified = SC_EVENT_INSERTED;
                              for (iEvent i = el->begin(); i != el->end(); ++i) {
                                    Event e = i->second;
                                    int tick = e.tick() + pos - curPart->tick();
                                    if (tick<0) {
                                            printf("ERROR: trying to add event before current part!\n");
                                            song->endUndo(SC_EVENT_INSERTED);
                                            delete el;
                                            return;
                                            }

                                    e.setTick(tick);
                                    int diff = e.endTick()-curPart->lenTick();
                                    if (diff > 0)  {// too short part? extend it
                                            Part* newPart = curPart->clone();
                                            newPart->setLenTick(newPart->lenTick()+diff);
                                            // Indicate no undo, and do port controller values but not clone parts. 
                                            //audio->msgChangePart(curPart, newPart,false);
                                            audio->msgChangePart(curPart, newPart, false, true, false);
                                            modified=modified|SC_PART_MODIFIED;
                                            curPart = newPart; // reassign
                                            }
                                    // Indicate no undo, and do not do port controller values and clone parts. 
                                    //audio->msgAddEvent(e, curPart, false);
                                    audio->msgAddEvent(e, curPart, false, false, false);
                                    }
                              song->endUndo(modified);
                              delete el;
                              return;
                              }
                        else
                              xml.unknown("pasteAt");
                        break;
                  case Xml::Attribut:
                  case Xml::TagEnd:
                  default:
                        break;
                  }
            }
      }
//---------------------------------------------------------
//   paste
//    paste events
//---------------------------------------------------------

void PianoCanvas::paste()
      {
      //Q3CString subtype("eventlist"); ddskrjo
      QString subtype("eventlist");
      QMimeSource* ms = QApplication::clipboard()->data(QClipboard::Clipboard);
      QString pt;
      if (!Q3TextDrag::decode(ms, pt, subtype)) {
            printf("cannot paste: bad data type\n");
            return;
            }
      pasteAt(pt, song->cpos());
      }

//---------------------------------------------------------
//   startDrag
//---------------------------------------------------------

void PianoCanvas::startDrag(CItem* /* item*/, bool copymode)
      {
      Q3TextDrag* drag = getTextDrag(this);
      if (drag) {
//            QApplication::clipboard()->setData(drag, QClipboard::Clipboard);

            if (copymode)
                  drag->dragCopy();
            else
                  drag->dragMove();
            }
      }

//---------------------------------------------------------
//   dragEnterEvent
//---------------------------------------------------------

void PianoCanvas::dragEnterEvent(QDragEnterEvent* event)
      {
      event->accept(Q3TextDrag::canDecode(event));
      }

//---------------------------------------------------------
//   dragMoveEvent
//---------------------------------------------------------

void PianoCanvas::dragMoveEvent(QDragMoveEvent*)
      {
//      printf("drag move %x\n", this);
      }

//---------------------------------------------------------
//   dragLeaveEvent
//---------------------------------------------------------

void PianoCanvas::dragLeaveEvent(QDragLeaveEvent*)
      {
//      printf("drag leave\n");
      }

//---------------------------------------------------------
//   dropEvent
//---------------------------------------------------------

void PianoCanvas::viewDropEvent(QDropEvent* event)
      {
      QString text;
      if (event->source() == this) {
            printf("local DROP\n");
            return;
            }
      if (Q3TextDrag::decode(event, text)) {
            int x = editor->rasterVal(event->pos().x());
            if (x < 0)
                  x = 0;
            pasteAt(text, x);
            }
      else {
            printf("cannot decode drop\n");
            }
      }

//---------------------------------------------------------
//   itemPressed
//---------------------------------------------------------

void PianoCanvas::itemPressed(const CItem* item)
      {
      if (!_playEvents)
            return;

      int port         = track()->outPort();
      int channel      = track()->outChannel();
      NEvent* nevent   = (NEvent*) item;
      Event event      = nevent->event();
      playedPitch      = event.pitch() + track()->transposition;
      int velo         = event.velo();

      // play note:
      MidiPlayEvent e(0, port, channel, 0x90, playedPitch, velo);
      audio->msgPlayMidiEvent(&e);
      }

//---------------------------------------------------------
//   itemReleased
//---------------------------------------------------------

void PianoCanvas::itemReleased(const CItem*, const QPoint&)
      {
      if (!_playEvents)
            return;
      int port    = track()->outPort();
      int channel = track()->outChannel();

      // release note:
      MidiPlayEvent ev(0, port, channel, 0x90, playedPitch, 0);
      audio->msgPlayMidiEvent(&ev);
      playedPitch = -1;
      }

//---------------------------------------------------------
//   itemMoved
//---------------------------------------------------------

void PianoCanvas::itemMoved(const CItem* item, const QPoint& pos)
      {
      int npitch = y2pitch(pos.y());
      if ((playedPitch != -1) && (playedPitch != npitch)) {
            int port         = track()->outPort();
            int channel      = track()->outChannel();
            NEvent* nevent   = (NEvent*) item;
            Event event      = nevent->event();

            // release note:
            MidiPlayEvent ev1(0, port, channel, 0x90, playedPitch, 0);
            audio->msgPlayMidiEvent(&ev1);
            // play note:
            MidiPlayEvent e2(0, port, channel, 0x90, npitch + track()->transposition, event.velo());
            audio->msgPlayMidiEvent(&e2);
            playedPitch = npitch + track()->transposition;
            }
      }

//---------------------------------------------------------
//   curPartChanged
//---------------------------------------------------------

void PianoCanvas::curPartChanged()
      {
      editor->setCaption(getCaption());
      }

//---------------------------------------------------------
//   modifySelected
//---------------------------------------------------------

void PianoCanvas::modifySelected(NoteInfo::ValType type, int delta)
      {
      audio->msgIdle(true);
      song->startUndo();
      for (iCItem i = items.begin(); i != items.end(); ++i) {
            if (!(i->second->isSelected()))
                  continue;
            NEvent* e   = (NEvent*)(i->second);
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
                        {
                        int len = event.lenTick() + delta;
                        if (len < 1)
                              len = 1;
                        newEvent.setLenTick(len);
                        }
                        break;
                  case NoteInfo::VAL_VELON:
                        {
                        int velo = event.velo() + delta;
                        if (velo > 127)
                              velo = 127;
                        else if (velo < 0)
                              velo = 0;
                        newEvent.setVelo(velo);
                        }
                        break;
                  case NoteInfo::VAL_VELOFF:
                        {
                        int velo = event.veloOff() + delta;
                        if (velo > 127)
                              velo = 127;
                        else if (velo < 0)
                              velo = 0;
                        newEvent.setVeloOff(velo);
                        }
                        break;
                  case NoteInfo::VAL_PITCH:
                        {
                        int pitch = event.pitch() + delta;
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
            //song->undoOp(UndoOp::ModifyEvent, newEvent, event, part);
            song->undoOp(UndoOp::ModifyEvent, newEvent, event, part, false, false);
            }
      song->endUndo(SC_EVENT_MODIFIED);
      audio->msgIdle(false);
      }

