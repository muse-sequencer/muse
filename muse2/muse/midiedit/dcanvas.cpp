//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: dcanvas.cpp,v 1.16.2.10 2009/10/15 22:45:50 terminator356 Exp $
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
#include "functions.h"

#define CARET   10
#define CARET2   5

namespace MusEGui {

//---------------------------------------------------------
//   DCItem
//---------------------------------------------------------

DCItem::DCItem(const MusECore::Event& e, MusECore::Part* p) : MCItem(e, p)
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

void DrumCanvas::addItem(MusECore::Part* part, MusECore::Event& event)
      {
      if (signed(event.tick())<0) {
            printf("ERROR: trying to add event before current part!\n");
            return;
      }
      
      DCItem* ev = new DCItem(event, part);
      items[_ECANVAS_EVENT_ITEMS_].add(ev);
      
      int diff = event.endTick()-part->lenTick();
      if (diff > 0)  {// too short part? extend it
            //printf("addItem - this code should not be run!\n");
            //MusECore::Part* newPart = part->clone();
            //newPart->setLenTick(newPart->lenTick()+diff);
            //MusEGlobal::audio->msgChangePart(part, newPart,false);
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
      
      steprec=new MusECore::StepRec(NULL);
      
      songChanged(SC_TRACK_INSERTED);
      connect(MusEGlobal::song, SIGNAL(midiNote(int, int)), SLOT(midiNote(int,int)));
      }

DrumCanvas::~DrumCanvas()
{
  //items.clearDelete();
}

//---------------------------------------------------------
//   moveCanvasItems
//---------------------------------------------------------

MusECore::Undo DrumCanvas::moveCanvasItems(CItemList& items, int dp, int dx, DragType dtype)
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
    for(iCItem ici = items.begin(); ici != items.end(); ++ici) 
    {
      if(ici->second->type() != CItem::DEVENT)  // p4.1.0
        continue;

      DCItem* dcitem = (DCItem*) ici->second;
      if(dcitem->part() != part)
        continue;
      
      int x = dcitem->pos().x() + dx;
      int y = pitch2y(y2pitch(dcitem->pos().y()) + dp);
      QPoint newpos = raster(QPoint(x, y));
      
      // Test moving the item...
      //MusECore::Event event    = dcitem->event();
      x              = newpos.x();
      if(x < 0)
        x = 0;
      int ntick = editor->rasterVal(x) - part->tick();
      if(ntick < 0)
        ntick = 0;
      int diff = ntick + dcitem->event().lenTick() - part->lenTick();
      
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
		std::vector< DCItem* > doneList;
		typedef std::vector< DCItem* >::iterator iDoneList;
		
		for(iCItem ici = items.begin(); ici != items.end(); ++ici) 
		{
                        if(ici->second->type() != CItem::DEVENT)    // p4.1.0
                          continue;
                        DCItem* dcitem = (DCItem*)ici->second;
			int x = dcitem->pos().x();
			int y = dcitem->pos().y();
			int nx = x + dx;
			int ny = pitch2y(y2pitch(y) + dp);
			QPoint newpos = raster(QPoint(nx, ny));
			selectItem(dcitem, true);
			
			iDoneList idl;
			for(idl = doneList.begin(); idl != doneList.end(); ++idl)
				// This compares EventBase pointers to see if they're the same...
				if((*idl)->event() == dcitem->event())
					break;
				
			// Do not process if the event has already been processed (meaning it's an event in a clone part)...
			if (idl == doneList.end())
			{
				operations.push_back(moveItem(dcitem, newpos, dtype));
				doneList.push_back(dcitem);
			}
			dcitem->move(newpos);
						
			if(moving.size() == 1) 
						itemReleased(curItem, newpos);

			if(dtype == MOVE_COPY || dtype == MOVE_CLONE)
						selectItem(dcitem, false);
		}  

		for(MusECore::iPartToChange ip2c = parts2change.begin(); ip2c != parts2change.end(); ++ip2c)
		{
			MusECore::Part* opart = ip2c->first;
			int diff = ip2c->second.xdiff;
			
			schedule_resize_all_same_len_clone_parts(opart, opart->lenTick() + diff, operations); 
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
//---------------------------------------------------------

MusECore::UndoOp DrumCanvas::moveItem(CItem* item, const QPoint& pos, DragType dtype)
      {
      // p4.1.0 Only handle notes for now. 
      if(item->type() != CItem::DEVENT)
	return MusECore::UndoOp();
      
      DCItem* nevent   = (DCItem*) item;
      
      MusECore::MidiPart* part   = (MusECore::MidiPart*)nevent->part();   
      
      MusECore::Event event      = nevent->event();
      int x            = pos.x();
      if (x < 0)
            x = 0;
      int ntick        = editor->rasterVal(x) - part->tick();
      if (ntick < 0)
            ntick = 0;
      int npitch       = y2pitch(pos.y());
      MusECore::Event newEvent   = event.clone();

      newEvent.setPitch(npitch);
      newEvent.setTick(ntick);

      // Added by T356, removed by flo93: with operation groups, it happens that the
      // part is too short right now, even if it's queued for being extended
      //if(((int)newEvent.endTick() - (int)part->lenTick()) > 0)  
      //  printf("DrumCanvas::moveItem Error! New event end:%d exceeds length:%d of part:%s\n", newEvent.endTick(), part->lenTick(), part->name().toLatin1().constData());
      
      if (dtype == MOVE_COPY || dtype == MOVE_CLONE)
            return MusECore::UndoOp(MusECore::UndoOp::AddEvent, newEvent, part, false, false);
      else
            return MusECore::UndoOp(MusECore::UndoOp::ModifyEvent, newEvent, event, part, false, false);
      }

//---------------------------------------------------------
//   newItem
//---------------------------------------------------------

CItem* DrumCanvas::newItem(const QPoint& p, int state)
{
  switch (_tool) 
  {
    //case AutomationTool:
      // 4.1.0 TODO: Handle automation, if and when we add it.
    //  return 0;
    //break;
      
    //case PointerTool:
    default:
    {  
      int instr = y2pitch(p.y());         //MusEGlobal::drumInmap[y2pitch(p.y())];
      int velo  = MusEGlobal::drumMap[instr].lv4;
      if (state == Qt::ShiftModifier)
            velo = MusEGlobal::drumMap[instr].lv3;
      else if (state == Qt::ControlModifier)
            velo = MusEGlobal::drumMap[instr].lv2;
      else if (state == (Qt::ControlModifier | Qt::ShiftModifier))
            velo = MusEGlobal::drumMap[instr].lv1;
      int tick = editor->rasterVal(p.x());
      //return newItem(tick, instr, velo);
      CItem* ni = newItem(tick, instr, velo);
      items[_ECANVAS_EVENT_ITEMS_].add(ni);
      return ni;
    }
  }  
}

//---------------------------------------------------------
//   newItem
//---------------------------------------------------------

CItem* DrumCanvas::newItem(int tick, int instrument, int velocity)
{
  tick    -= _curPart->tick();
  MusECore::Event e(MusECore::Note);
  e.setTick(tick);
  e.setPitch(instrument);
  e.setVelo(velocity);
  e.setLenTick(MusEGlobal::drumMap[instrument].len);
  return new DCItem(e, _curPart);
}

//---------------------------------------------------------
//   resizeItem
//---------------------------------------------------------

void DrumCanvas::resizeItem(CItem* item, bool, bool)
      {
      // p4.1.0 Only handle notes for now. 
      if(item->type() != CItem::DEVENT)
	return;
      
      DCItem* nevent = (DCItem*) item;
      MusECore::Event ev = nevent->event();
      // Indicate do undo, and do not do port controller values and clone parts. 
      MusEGlobal::audio->msgDeleteEvent(ev, nevent->part(), true, false, false);
      }

//---------------------------------------------------------
//   newItem
//---------------------------------------------------------
void DrumCanvas::newItem(CItem* item, bool noSnap) {
     newItem(item, noSnap,false);
}

void DrumCanvas::newItem(CItem* item, bool noSnap, bool replace)
      {
      // p4.1.0 Only handle notes for now. 
      if(item->type() != CItem::DEVENT)
	return;
      
      DCItem* nevent = (DCItem*) item;
      MusECore::Event event    = nevent->event();
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
      MusECore::EventList* el = nevent->part()->events();
      MusECore::iEvent lower  = el->lower_bound(event.tick());
      MusECore::iEvent upper  = el->upper_bound(event.tick());

      for (MusECore::iEvent i = lower; i != upper; ++i) {
            MusECore::Event ev = i->second;
            // Added by T356. Only do notes.
            if(!ev.isNote())
              continue;
              
            if (ev.pitch() == npitch) {
                  // Indicate do undo, and do not do port controller values and clone parts. 
                  MusEGlobal::audio->msgDeleteEvent(ev, nevent->part(), true, false, false);
                  if (replace)
                    break;
                  else
                    return;

              }
            }

      // Added by T356.
      MusECore::Part* part = nevent->part();
      MusECore::Undo operations;
      int diff = event.endTick()-part->lenTick();
      
      if (! ((diff > 0) && part->hasHiddenEvents()) ) //operation is allowed
      {
        operations.push_back(MusECore::UndoOp(MusECore::UndoOp::AddEvent,event, part, false, false));
        
        if (diff > 0)// part must be extended?
        {
              schedule_resize_all_same_len_clone_parts(part, event.endTick(), operations);
              printf("newItem: extending\n");
        }
      }
      //else forbid action by not applying it
      MusEGlobal::song->applyOperationGroup(operations);
      songChanged(SC_EVENT_INSERTED); //this forces an update of the itemlist, which is neccessary
                                      //to remove "forbidden" events from the list again
      }

//---------------------------------------------------------
//   deleteItem
//---------------------------------------------------------

bool DrumCanvas::deleteItem(CItem* item)
      {
      // p4.1.0 Only handle notes for now. 
      if(item->type() != CItem::DEVENT)
	return false;
      
      ///MusECore::Event ev = ((DCItem*)item)->event();
      // Indicate do undo, and do not do port controller values and clone parts. 
      ///MusEGlobal::audio->msgDeleteEvent(ev, ((DCItem*)item)->part(), true, false, false);
      // p4.1.0 Borrowed from prcanvas. Correct?
      DCItem* nevent = (DCItem*)item;
      if (nevent->part() == _curPart) {
            MusECore::Event ev = nevent->event();
            // Indicate do undo, and do not do port controller values and clone parts. 
            MusEGlobal::audio->msgDeleteEvent(ev, _curPart, true, false, false);
            return true;
            }
      
      //return false;
      return true; // p4.1.0 Why was this false?
      }

//---------------------------------------------------------
//   drawItem
//---------------------------------------------------------

//void DrumCanvas::drawItem(QPainter&p, const CItem*item, const QRect& rect)
void DrumCanvas::drawItem(QPainter&p, const CItem*item, const QRect& rect, int /*layer*/)
      {
      // p4.1.0 Only handle notes for now. 
      if(item->type() != CItem::DEVENT)
	return;
      
      DCItem* e   = (DCItem*) item;
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
      
      if (e->part() != _curPart)
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
            MusECore::DrumMap* dm = &MusEGlobal::drumMap[y2pitch(y)]; //Get the drum item
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
      // p4.1.0 Only handle notes for now. 
      if(item->type() != CItem::DEVENT)
	return;
      
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
      switch (cmd) {
            case CMD_SELECT_ALL:     // select all
                  for (iCItem k = items[_ECANVAS_EVENT_ITEMS_].begin(); k != items[_ECANVAS_EVENT_ITEMS_].end(); ++k) {
			// p4.1.0 Only handle notes for now. 
			if(k->second->type() != CItem::DEVENT)
			  continue;
			
                        if (!k->second->isSelected())
                              selectItem(k->second, true);
                        }
                  break;
            case CMD_SELECT_NONE:     // select none
                  deselectAll();
                  break;
            case CMD_SELECT_INVERT:     // invert selection
                  for (iCItem k = items[_ECANVAS_EVENT_ITEMS_].begin(); k != items[_ECANVAS_EVENT_ITEMS_].end(); ++k) {
			// p4.1.0 Only handle notes for now. 
			if(k->second->type() != CItem::DEVENT)
			  continue;
                        
			selectItem(k->second, !k->second->isSelected());
                        }
                  break;
            case CMD_SELECT_ILOOP:     // select inside loop
                  for (iCItem k = items[_ECANVAS_EVENT_ITEMS_].begin(); k != items[_ECANVAS_EVENT_ITEMS_].end(); ++k) {
			// p4.1.0 Only handle notes for now. 
			if(k->second->type() != CItem::DEVENT)
			  continue;

			DCItem* nevent =(DCItem*)(k->second);
                        MusECore::Part* part = nevent->part();
                        MusECore::Event event = nevent->event();
                        unsigned tick  = event.tick() + part->tick();
                        if (tick < MusEGlobal::song->lpos() || tick >= MusEGlobal::song->rpos())
                              selectItem(k->second, false);
                        else
                              selectItem(k->second, true);
                        }
                  break;
            case CMD_SELECT_OLOOP:     // select outside loop
                  for (iCItem k = items[_ECANVAS_EVENT_ITEMS_].begin(); k != items[_ECANVAS_EVENT_ITEMS_].end(); ++k) {
			// p4.1.0 Only handle notes for now. 
			if(k->second->type() != CItem::DEVENT)
			  continue;

			DCItem* nevent = (DCItem*)(k->second);
                        MusECore::Part* part     = nevent->part();
                        MusECore::Event event    = nevent->event();
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

            case CMD_SAVE:
            case CMD_LOAD:
                  printf("DrumCanvas:: cmd not implemented %d\n", cmd);
                  break;

            case CMD_FIXED_LEN: //Set notes to the length specified in the drummap
                  if (!selectionSize())
                        break;
                  MusEGlobal::song->startUndo();
                  for (iCItem k = items[_ECANVAS_EVENT_ITEMS_].begin(); k != items[_ECANVAS_EVENT_ITEMS_].end(); ++k) {
			// p4.1.0 Only handle notes for now. 
			if(k->second->type() != CItem::DEVENT)
			  continue;
			
                        if (k->second->isSelected()) {
                              DCItem* devent = (DCItem*)(k->second);
                              MusECore::Event event    = devent->event();
                              MusECore::Event newEvent = event.clone();
                              newEvent.setLenTick(MusEGlobal::drumMap[event.pitch()].len);
                              // Indicate no undo, and do not do port controller values and clone parts. 
                              MusEGlobal::audio->msgChangeEvent(event, newEvent, devent->part(), false, false, false);
                              }
                        }
                  MusEGlobal::song->endUndo(SC_EVENT_MODIFIED);
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
                      MusECore::Pos p(spos,true);
                      MusEGlobal::song->setPos(0, p, true, true, true);
                  }
                  break;
            case CMD_RIGHT:
                  {
                      int spos = AL::sigmap.raster2(pos[0] + 1, editor->rasterStep(pos[0]));    // Nudge by +1, then snap up with raster2.
                      MusECore::Pos p(spos,true);
                      MusEGlobal::song->setPos(0, p, true, true, true);
                  }
                  break;
            case CMD_LEFT_NOSNAP:
                  {
                    printf("left no snap\n");
                  int spos = pos[0] - editor->rasterStep(pos[0]);
                  if (spos < 0)
                        spos = 0;
                  MusECore::Pos p(spos,true);
                  MusEGlobal::song->setPos(0, p, true, true, true); //CDW
                  }
                  break;
            case CMD_RIGHT_NOSNAP:
                  {
                  MusECore::Pos p(pos[0] + editor->rasterStep(pos[0]), true);
                  MusEGlobal::song->setPos(0, p, true, true, true); //CDW
                  }
                  break;
            }
      updateSelection();
      redraw();
      }


//---------------------------------------------------------
//   startDrag
//---------------------------------------------------------

void DrumCanvas::startDrag(CItem* /* item*/, bool copymode)
      {
      QMimeData* md = selected_events_to_mime(partlist_to_set(editor->parts()), 1);
      
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
      int port = MusEGlobal::drumMap[index].port;
      int channel = MusEGlobal::drumMap[index].channel;
      int pitch = MusEGlobal::drumMap[index].anote;

      // play note:
      MusECore::MidiPlayEvent e(0, port, channel, 0x90, pitch, velocity);
      MusEGlobal::audio->msgPlayMidiEvent(&e);

      if (_steprec && pos[0] >= start_tick /* && pos[0] < end_tick [removed by flo93: this is handled in steprec->record] */ && _curPart)
            steprec->record(_curPart,index,MusEGlobal::drumMap[index].len,editor->raster(),velocity,MusEGlobal::globalKeyState&Qt::ControlModifier,MusEGlobal::globalKeyState&Qt::ShiftModifier);
            
      }

//---------------------------------------------------------
//   keyReleased
//---------------------------------------------------------

void DrumCanvas::keyReleased(int index, bool)
      {
      // called from DList - silence playing event
      int port = MusEGlobal::drumMap[index].port;
      int channel = MusEGlobal::drumMap[index].channel;
      int pitch = MusEGlobal::drumMap[index].anote;

      // release note:
      MusECore::MidiPlayEvent e(0, port, channel, 0x90, pitch, 0);
      MusEGlobal::audio->msgPlayMidiEvent(&e);
      }

//---------------------------------------------------------
//   mapChanged
//---------------------------------------------------------

void DrumCanvas::mapChanged(int spitch, int dpitch)
      {
      MusECore::Undo operations;
      std::vector< std::pair<MusECore::Part*, MusECore::Event*> > delete_events;
      std::vector< std::pair<MusECore::Part*, MusECore::Event> > add_events;
      
      typedef std::vector< std::pair<MusECore::Part*, MusECore::Event*> >::iterator idel_ev;
      typedef std::vector< std::pair<MusECore::Part*, MusECore::Event> >::iterator iadd_ev;
      
      MusECore::MidiTrackList* tracks = MusEGlobal::song->midis();
      for (MusECore::ciMidiTrack t = tracks->begin(); t != tracks->end(); t++) {
            MusECore::MidiTrack* curTrack = *t;
            if (curTrack->type() != MusECore::Track::DRUM)
                  continue;

            MusECore::MidiPort* mp = &MusEGlobal::midiPorts[curTrack->outPort()];
            MusECore::PartList* parts= curTrack->parts();
            for (MusECore::iPart part = parts->begin(); part != parts->end(); ++part) {
                  MusECore::EventList* events = part->second->events();
                  MusECore::Part* thePart = part->second;
                  for (MusECore::iEvent i = events->begin(); i != events->end(); ++i) {
                        MusECore::Event event = i->second;
                        if(event.type() != MusECore::Controller && event.type() != MusECore::Note)
                          continue;
                        int pitch = event.pitch();
                        bool drc = false;
                        // Is it a drum controller event, according to the track port's instrument?
                        if(event.type() == MusECore::Controller && mp->drumController(event.dataA()))
                        {
                          drc = true;
                          pitch = event.dataA() & 0x7f;
                        }
                        
                        if (pitch == spitch) {
                              MusECore::Event* spitch_event = &(i->second);
                              delete_events.push_back(std::pair<MusECore::Part*, MusECore::Event*>(thePart, spitch_event));
                              MusECore::Event newEvent = spitch_event->clone();
                              if(drc)
                                newEvent.setA((newEvent.dataA() & ~0xff) | dpitch);
                              else
                                newEvent.setPitch(dpitch);
                              add_events.push_back(std::pair<MusECore::Part*, MusECore::Event>(thePart, newEvent));
                              }
                        else if (pitch == dpitch) {
                              MusECore::Event* dpitch_event = &(i->second);
                              delete_events.push_back(std::pair<MusECore::Part*, MusECore::Event*>(thePart, dpitch_event));
                              MusECore::Event newEvent = dpitch_event->clone();
                              if(drc)
                                newEvent.setA((newEvent.dataA() & ~0xff) | spitch);
                              else
                                newEvent.setPitch(spitch);
                              add_events.push_back(std::pair<MusECore::Part*, MusECore::Event>(thePart, newEvent));
                              }
                        }
                  }
            }

      for (idel_ev i = delete_events.begin(); i != delete_events.end(); i++) {
            MusECore::Part*  thePart = (*i).first;
            MusECore::Event* theEvent = (*i).second;
            operations.push_back(MusECore::UndoOp(MusECore::UndoOp::DeleteEvent, *theEvent, thePart, true, false));
            }

      MusECore::DrumMap dm = MusEGlobal::drumMap[spitch];
      MusEGlobal::drumMap[spitch] = MusEGlobal::drumMap[dpitch];
      MusEGlobal::drumMap[dpitch] = dm;
      MusEGlobal::drumInmap[int(MusEGlobal::drumMap[spitch].enote)]  = spitch;
      MusEGlobal::drumOutmap[int(MusEGlobal::drumMap[int(spitch)].anote)] = spitch;
      MusEGlobal::drumInmap[int(MusEGlobal::drumMap[int(dpitch)].enote)]  = dpitch;
      MusEGlobal::drumOutmap[int(MusEGlobal::drumMap[int(dpitch)].anote)] = dpitch;
            
      for (iadd_ev i = add_events.begin(); i != add_events.end(); i++) {
            MusECore::Part*  thePart = (*i).first;
            MusECore::Event& theEvent = (*i).second;
            operations.push_back(MusECore::UndoOp(MusECore::UndoOp::AddEvent, theEvent, thePart, true, false));
            }
      
      MusEGlobal::song->applyOperationGroup(operations, false); // do not indicate undo
      MusEGlobal::song->update(SC_DRUMMAP); //this update is neccessary, as it's not handled by applyOperationGroup()
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
      MusEGlobal::audio->msgIdle(true);
      MusEGlobal::song->startUndo();
      for (iCItem i = items[_ECANVAS_EVENT_ITEMS_].begin(); i != items[_ECANVAS_EVENT_ITEMS_].end(); ++i) {
	    // p4.1.0 Only handle notes for now. 
	    if(i->second->type() != CItem::DEVENT)
	      continue;
	    
            if (!(i->second->isSelected()))
                  continue;
            DCItem* e   = (DCItem*)(i->second);
            MusECore::Event event = e->event();
            if (event.type() != MusECore::Note)
                  continue;

            MusECore::MidiPart* part = (MusECore::MidiPart*)(e->part());
            MusECore::Event newEvent = event.clone();

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
            MusEGlobal::song->changeEvent(event, newEvent, part);
            // Indicate do not do port controller values and clone parts. 
            MusEGlobal::song->addUndo(MusECore::UndoOp(MusECore::UndoOp::ModifyEvent, newEvent, event, part, false, false));
            }
      MusEGlobal::song->endUndo(SC_EVENT_MODIFIED);
      MusEGlobal::audio->msgIdle(false);
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
      if (unsigned(newPos) > _curPart->endTick()- editor->rasterStep(_curPart->endTick()))
        newPos = _curPart->tick();
    }
    else { // moving left
      newPos = AL::sigmap.raster1(newPos + basicStep, editor->rasterStep(newPos));    // Nudge by -1, then snap up with raster1.
      if (unsigned(newPos) < _curPart->tick() ) {
        newPos = AL::sigmap.raster1(_curPart->endTick()-1, editor->rasterStep(_curPart->endTick()));
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
          newItem(newItem(cursorPos.x(), cursorPos.y(), MusEGlobal::drumMap[cursorPos.y()].lv1),false,true);
          keyPressed(cursorPos.y(), MusEGlobal::drumMap[cursorPos.y()].lv1);
          keyReleased(cursorPos.y(), false);
          cursorPos.setX(getNextStep(cursorPos.x(),1, _stepSize));
          selectCursorEvent(getEventAtCursorPos());
          if (mapx(cursorPos.x()) < 0 || mapx(cursorPos.x()) > width())
            emit followEvent(cursorPos.x());
          return;
    }
    else if (key == shortcuts[SHRT_ADDNOTE_2].key) {
          newItem(newItem(cursorPos.x(), cursorPos.y(), MusEGlobal::drumMap[cursorPos.y()].lv2),false,true);
          keyPressed(cursorPos.y(), MusEGlobal::drumMap[cursorPos.y()].lv2);
          keyReleased(cursorPos.y(), false);
          cursorPos.setX(getNextStep(cursorPos.x(),1, _stepSize));
          selectCursorEvent(getEventAtCursorPos());
          if (mapx(cursorPos.x()) < 0 || mapx(cursorPos.x()) > width())
            emit followEvent(cursorPos.x());
          return;
    }
    else if (key == shortcuts[SHRT_ADDNOTE_3].key) {
          newItem(newItem(cursorPos.x(), cursorPos.y(), MusEGlobal::drumMap[cursorPos.y()].lv3),false,true);
          keyPressed(cursorPos.y(), MusEGlobal::drumMap[cursorPos.y()].lv3);
          keyReleased(cursorPos.y(), false);
          cursorPos.setX(getNextStep(cursorPos.x(),1, _stepSize));
          selectCursorEvent(getEventAtCursorPos());
          if (mapx(cursorPos.x()) < 0 || mapx(cursorPos.x()) > width())
            emit followEvent(cursorPos.x());
          return;
    }
    else if (key == shortcuts[SHRT_ADDNOTE_4].key) {
          newItem(newItem(cursorPos.x(), cursorPos.y(), MusEGlobal::drumMap[cursorPos.y()].lv4),false,true);
          keyPressed(cursorPos.y(), MusEGlobal::drumMap[cursorPos.y()].lv4);
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
  if (unsigned(cursorPos.x()) < _curPart->tick())
    cursorPos.setX(_curPart->tick());
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
MusECore::Event *DrumCanvas::getEventAtCursorPos()
{
    if (_tool != CursorTool)
      return 0;
    MusECore::EventList* el = _curPart->events();
    MusECore::iEvent lower  = el->lower_bound(cursorPos.x()-_curPart->tick());
    MusECore::iEvent upper  = el->upper_bound(cursorPos.x()-_curPart->tick());
    for (MusECore::iEvent i = lower; i != upper; ++i) {
      MusECore::Event &ev = i->second;
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
void DrumCanvas::selectCursorEvent(MusECore::Event *ev)
{
  for (iCItem i = items[_ECANVAS_EVENT_ITEMS_].begin(); i != items[_ECANVAS_EVENT_ITEMS_].end(); ++i)
  {
	// p4.1.0 Only handle notes for now. 
        if(i->second->type() != CItem::DEVENT)
          continue;
        
        DCItem* dcitem = (DCItem*)i->second;
        MusECore::Event e = dcitem->event();

        if (ev && ev->tick() == e.tick() && ev->pitch() == e.pitch() && e.isNote())
          dcitem->setSelected(true);
        else
          dcitem->setSelected(false);

  }
  updateSelection();
}


void DrumCanvas::moveAwayUnused()
{
	using std::set;
	
	set<int> used;
	for (iCItem it=items[_ECANVAS_EVENT_ITEMS_].begin(); it!=items[_ECANVAS_EVENT_ITEMS_].end(); it++)
	{
		// p4.1.0 Only handle notes for now. 
		if(it->second->type() != CItem::DEVENT)
                  continue;
                DCItem* dcitem = (DCItem*)it->second;
                const MusECore::Event& ev=dcitem->event();
		
		if (ev.type()==MusECore::Note)
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


//---------------------------------------------------------
//   midiNote
//---------------------------------------------------------
void DrumCanvas::midiNote(int pitch, int velo)
      {
      if (MusEGlobal::debugMsg) printf("DrumCanvas::midiNote: pitch=%i, velo=%i\n", pitch, velo);

      if (_midiin && _steprec && _curPart
         && !MusEGlobal::audio->isPlaying() && velo && pos[0] >= start_tick
         /* && pos[0] < end_tick [removed by flo93: this is handled in steprec->record] */
         && !(MusEGlobal::globalKeyState & Qt::AltModifier)) {
                                               steprec->record(_curPart,MusEGlobal::drumInmap[pitch],MusEGlobal::drumMap[(int)MusEGlobal::drumInmap[pitch]].len,editor->raster(),velo,MusEGlobal::globalKeyState&Qt::ControlModifier,MusEGlobal::globalKeyState&Qt::ShiftModifier);
         }
      }

} // namespace MusEGui
