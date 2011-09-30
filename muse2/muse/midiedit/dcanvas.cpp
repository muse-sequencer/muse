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
//#include <sys/stat.h>
//#include <sys/mman.h>

#include "dcanvas.h"
#include "midieditor.h"
#include "drumedit.h"
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

using MusEGlobal::debugMsg;
using MusEGlobal::heavyDebugMsg;

//---------------------------------------------------------
//   DEvent
//---------------------------------------------------------

DEvent::DEvent(Event e, Part* p, int instr)
  : CItem(e, p)
      {
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
      
      int instr=pitch_and_track_to_instrument(event.pitch(), part->track());
      if (instr<0)
      {
        if (heavyDebugMsg) printf("trying to add event which is hidden or not in any part known to me\n");
        return;
      }
      
      DEvent* ev = new DEvent(event, part, instr);
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
      old_style_drummap_mode = dynamic_cast<DrumEdit*>(pr)->old_style_drummap_mode();

      if (old_style_drummap_mode)
      {
        if (debugMsg) printf("DrumCanvas in old style drummap mode\n");
        ourDrumMap = drumMap;
        must_delete_our_drum_map=false;
        
        instrument_number_mapping_t temp;
        for (ciPart it=pr->parts()->begin(); it!=pr->parts()->end(); it++)
          temp.tracks.insert(it->second->track());

        for (int i=0;i<DRUM_MAPSIZE;i++)
        {
          temp.pitch=i;
          temp.track_dlist_index=i; // actually unneeded, but who knows...
          instrument_map.append(temp);
        }
      }
      else
      {
        if (debugMsg) printf("DrumCanvas in new style drummap mode\n");
        // FINDMICHJETZT
        TrackList* tl=song->tracks();
        
        QList< QSet<Track*> > track_groups;
        
        for (ciTrack track = tl->begin(); track!=tl->end(); track++)
        {
          ciPart p_it;
          for (p_it=pr->parts()->begin(); p_it!=pr->parts()->end(); p_it++)
            if (p_it->second->track() == *track)
              break;
          
          if (p_it!=pr->parts()->end()) // if *track is represented by some part in this editor
          {
            QSet<Track*> temp;
            temp.insert(*track);
            track_groups.push_back(temp);
          }
        }
        
        // from now, we assume that every track_group's entry only groups tracks with identical
        // drum maps, but not necessarily identical hide-lists together.

        for (QList< QSet<Track*> >::iterator group=track_groups.begin(); group!=track_groups.end(); group++)
        {
          for (int i=0;i<128;i++) // find out if instrument is hidden and make "mute" 
          {                       // consistent across groups for non-hidden instruments
            bool mute=true;
            bool hidden=true;
            for (QSet<Track*>::iterator track=group->begin(); track!=group->end() && (mute || hidden); track++)
            {
              if (dynamic_cast<MidiTrack*>(*track)->drummap()[i].mute == false)
                mute=false;
              
              if (dynamic_cast<MidiTrack*>(*track)->drummap_hidden()[i] == false)
                hidden=false;
            }

            if (!hidden)
            {
              for (QSet<Track*>::iterator track=group->begin(); track!=group->end(); track++)
                dynamic_cast<MidiTrack*>(*track)->drummap()[i].mute=mute; 
              
              instrument_map.append(instrument_number_mapping_t(*group, dynamic_cast<MidiTrack*>(*group->begin())->drummap()[i].anote, i));
            }
          }
        }
        


        // populate ourDrumMap
        
        int size = instrument_map.size();
        ourDrumMap=new DrumMap[size];
        must_delete_our_drum_map=true;
        
        for (int i=0;i<size;i++)
          ourDrumMap[i] = dynamic_cast<MidiTrack*>(*instrument_map[i].tracks.begin())->drummap()[instrument_map[i].track_dlist_index];
          //ourDrumMap[i] = idrumMap[instrument_map[i%128].pitch]; //FINDMICH dummy!
      }
      
      
      
      setVirt(false);
      cursorPos= QPoint(0,0);
      _stepSize=1;
      
      steprec=new StepRec(NULL);
      
      songChanged(SC_TRACK_INSERTED);
      connect(song, SIGNAL(midiNote(int, int)), SLOT(midiNote(int,int)));
      }

//---------------------------------------------------------
//   moveCanvasItems
//---------------------------------------------------------

Undo DrumCanvas::moveCanvasItems(MusEWidget::CItemList& items, int dp, int dx, DragType dtype)
{      
  if(editor->parts()->empty())
    return Undo(); //return empty list
  
  PartsToChangeMap parts2change;
  Undo operations;  
  
  for(iPart ip = editor->parts()->begin(); ip != editor->parts()->end(); ++ip)
  {
    Part* part = ip->second;
    if(!part)
      continue;
    
    int npartoffset = 0;
    for(MusEWidget::iCItem ici = items.begin(); ici != items.end(); ++ici) 
    {
      MusEWidget::CItem* ci = ici->second;
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
  
  bool forbidden=false;
  for(iPartToChange ip2c = parts2change.begin(); ip2c != parts2change.end(); ++ip2c)
  {
    Part* opart = ip2c->first;
    if (opart->hasHiddenEvents())
    {
			forbidden=true;
			break;
		}
  }    

	
	if (!forbidden)
	{
		std::vector< MusEWidget::CItem* > doneList;
		typedef std::vector< MusEWidget::CItem* >::iterator iDoneList;
		
		for(MusEWidget::iCItem ici = items.begin(); ici != items.end(); ++ici) 
		{
			MusEWidget::CItem* ci = ici->second;
			
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
			if (idl == doneList.end())
			{
				operations.push_back(moveItem(ci, newpos, dtype));
				doneList.push_back(ci);
			}
			ci->move(newpos);
      
			if(moving.size() == 1) 
						itemReleased(curItem, newpos);

			if(dtype == MOVE_COPY || dtype == MOVE_CLONE)
						selectItem(ci, false);
		}  

		for(iPartToChange ip2c = parts2change.begin(); ip2c != parts2change.end(); ++ip2c)
		{
			Part* opart = ip2c->first;
			int diff = ip2c->second.xdiff;
			
			schedule_resize_all_same_len_clone_parts(opart, opart->lenTick() + diff, operations); 
		}    
					
  	return operations;
  }
  else
  {
		return Undo(); //return empty list
	}
}
      
//---------------------------------------------------------
//   moveItem
//---------------------------------------------------------

UndoOp DrumCanvas::moveItem(MusEWidget::CItem* item, const QPoint& pos, DragType dtype)
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
      int nheight       = y2pitch(pos.y());
      Event newEvent   = event.clone();
      
      Track* dest_track = part->track();
      if (!instrument_map[nheight].tracks.contains(dest_track))
      {
        printf ("TODO FIXME: tried to move an event into a different track. this is not supported yet, but will be soon. ignoring this one...\n");
        //FINDMICH
        return UndoOp();
      }
      
      int ev_pitch = instrument_map[nheight].pitch;
      
      newEvent.setPitch(ev_pitch);
      newEvent.setTick(ntick);

      // Added by T356, removed by flo93: with operation groups, it happens that the
      // part is too short right now, even if it's queued for being extended
      //if(((int)newEvent.endTick() - (int)part->lenTick()) > 0)  
      //  printf("DrumCanvas::moveItem Error! New event end:%d exceeds length:%d of part:%s\n", newEvent.endTick(), part->lenTick(), part->name().toLatin1().constData());
      
      if (dtype == MOVE_COPY || dtype == MOVE_CLONE)
            return UndoOp(UndoOp::AddEvent, newEvent, part, false, false);
      else
            return UndoOp(UndoOp::ModifyEvent, newEvent, event, part, false, false);
      }

//---------------------------------------------------------
//   newItem
//---------------------------------------------------------

MusEWidget::CItem* DrumCanvas::newItem(const QPoint& p, int state)
      {
      int instr = y2pitch(p.y());         //drumInmap[y2pitch(p.y())];
      int velo  = ourDrumMap[instr].lv4;
      if (state == Qt::ShiftModifier)
            velo = ourDrumMap[instr].lv3;
      else if (state == Qt::ControlModifier)
            velo = ourDrumMap[instr].lv2;
      else if (state == (Qt::ControlModifier | Qt::ShiftModifier))
            velo = ourDrumMap[instr].lv1;
      int tick = editor->rasterVal(p.x());
      return newItem(tick, instr, velo);
      }

//---------------------------------------------------------
//   newItem
//---------------------------------------------------------

MusEWidget::CItem* DrumCanvas::newItem(int tick, int instrument, int velocity)
{
  if (!old_style_drummap_mode && !instrument_map[instrument].tracks.contains(curPart->track()))
  {
    printf("FINDMICH: tried to create a new Item which cannot be inside the current track. returning NULL\n");
    return NULL;
  }
  else
  {
    tick    -= curPart->tick();
    Event e(Note);
    e.setTick(tick);
    e.setPitch(instrument_map[instrument].pitch);
    e.setVelo(velocity);
    e.setLenTick(ourDrumMap[instrument].len);

    return new DEvent(e, curPart, instrument);
  }
}

//---------------------------------------------------------
//   resizeItem
//---------------------------------------------------------

void DrumCanvas::resizeItem(MusEWidget::CItem* item, bool, bool)
      {
      DEvent* nevent = (DEvent*) item;
      Event ev = nevent->event();
      // Indicate do undo, and do not do port controller values and clone parts. 
      audio->msgDeleteEvent(ev, nevent->part(), true, false, false);
      }

//---------------------------------------------------------
//   newItem
//---------------------------------------------------------
void DrumCanvas::newItem(MusEWidget::CItem* item, bool noSnap) {
     newItem(item, noSnap,false);
}

void DrumCanvas::newItem(MusEWidget::CItem* item, bool noSnap, bool replace)
{
   if (item)
   {
      DEvent* nevent = (DEvent*) item;
      Event event    = nevent->event();
      int x = item->x();
      if (!noSnap)
            x = editor->rasterVal(x);
      event.setTick(x - nevent->part()->tick());
      int npitch = event.pitch(); //FINDMICH
      //event.setPitch(npitch); // commented out by flo: has no effect

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
      Undo operations;
      int diff = event.endTick()-part->lenTick();
      
      if (! ((diff > 0) && part->hasHiddenEvents()) ) //operation is allowed
      {
        operations.push_back(UndoOp(UndoOp::AddEvent,event, part, false, false));
        
        if (diff > 0) // part must be extended?
        {
              schedule_resize_all_same_len_clone_parts(part, event.endTick(), operations);
              printf("newItem: extending\n");
        }
      }
      //else forbid action by not applying it
      song->applyOperationGroup(operations);
      songChanged(SC_EVENT_INSERTED); //this forces an update of the itemlist, which is neccessary
                                      //to remove "forbidden" events from the list again
   }
   else
    printf("THIS SHOULD NEVER HAPPEN: DrumCanvas::newItem called with NULL item!\n");
}

//---------------------------------------------------------
//   deleteItem
//---------------------------------------------------------

bool DrumCanvas::deleteItem(MusEWidget::CItem* item)
      {
      Event ev = ((DEvent*)item)->event();
      // Indicate do undo, and do not do port controller values and clone parts. 
      audio->msgDeleteEvent(ev, ((DEvent*)item)->part(), true, false, false);
      return false;
      }

//---------------------------------------------------------
//   drawItem
//---------------------------------------------------------

void DrumCanvas::drawItem(QPainter&p, const MusEWidget::CItem*item, const QRect& rect)
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
            DrumMap* dm = &ourDrumMap[y2pitch(y)]; //Get the drum item
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

void DrumCanvas::drawMoving(QPainter& p, const MusEWidget::CItem* item, const QRect& rect)
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
  if (_tool == MusEWidget::CursorTool) {
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
      if (pitch >= instrument_map.size())
            pitch = instrument_map.size()-1;
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
                  for (MusEWidget::iCItem k = items.begin(); k != items.end(); ++k) {
                        if (!k->second->isSelected())
                              selectItem(k->second, true);
                        }
                  break;
            case CMD_SELECT_NONE:     // select none
                  deselectAll();
                  break;
            case CMD_SELECT_INVERT:     // invert selection
                  for (MusEWidget::iCItem k = items.begin(); k != items.end(); ++k) {
                        selectItem(k->second, !k->second->isSelected());
                        }
                  break;
            case CMD_SELECT_ILOOP:     // select inside loop
                  for (MusEWidget::iCItem k = items.begin(); k != items.end(); ++k) {
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
                  for (MusEWidget::iCItem k = items.begin(); k != items.end(); ++k) {
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
                  for (MusEWidget::iCItem k = items.begin(); k != items.end(); ++k) {
                        if (k->second->isSelected()) {
                              DEvent* devent = (DEvent*)(k->second);
                              Event event    = devent->event();
                              Event newEvent = event.clone();
                              // newEvent.setLenTick(drumMap[event.pitch()].len);
                              newEvent.setLenTick(ourDrumMap[y2pitch(devent->y())].len); //FINDMICH
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
//   startDrag
//---------------------------------------------------------

void DrumCanvas::startDrag(MusEWidget::CItem* /* item*/, bool copymode)
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

void DrumCanvas::keyPressed(int index, int velocity) //FINDMICH later
      {
      // called from DList - play event
      int port = ourDrumMap[index].port;
      int channel = ourDrumMap[index].channel;
      int pitch = ourDrumMap[index].anote;

      // play note:
      MidiPlayEvent e(0, port, channel, 0x90, pitch, velocity);
      audio->msgPlayMidiEvent(&e);

      if (_steprec && pos[0] >= start_tick /* && pos[0] < end_tick [removed by flo93: this is handled in steprec->record] */ && curPart)
            steprec->record(curPart,index,ourDrumMap[index].len,editor->raster(),velocity,MusEGlobal::globalKeyState&Qt::ControlModifier,MusEGlobal::globalKeyState&Qt::ShiftModifier);
            
      }

//---------------------------------------------------------
//   keyReleased
//---------------------------------------------------------

void DrumCanvas::keyReleased(int index, bool) //FINDMICH later
      {
      // called from DList - silence playing event
      int port = ourDrumMap[index].port;
      int channel = ourDrumMap[index].channel;
      int pitch = ourDrumMap[index].anote;

      // release note:
      MidiPlayEvent e(0, port, channel, 0x90, pitch, 0);
      audio->msgPlayMidiEvent(&e);
      }

//---------------------------------------------------------
//   mapChanged
//---------------------------------------------------------

void DrumCanvas::mapChanged(int spitch, int dpitch)
{
   if (old_style_drummap_mode)
   {
      Undo operations;
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

      for (idel_ev i = delete_events.begin(); i != delete_events.end(); i++) {
            Part*  thePart = (*i).first;
            Event* theEvent = (*i).second;
            operations.push_back(UndoOp(UndoOp::DeleteEvent, *theEvent, thePart, true, false));
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
            operations.push_back(UndoOp(UndoOp::AddEvent, theEvent, thePart, true, false));
            }
      
      song->applyOperationGroup(operations, false); // do not indicate undo
      song->update(SC_DRUMMAP); //this update is neccessary, as it's not handled by applyOperationGroup()
   }
   else // if (!old_style_drummap_mode)
   {
      DrumMap dm = ourDrumMap[spitch];
      ourDrumMap[spitch] = ourDrumMap[dpitch];
      ourDrumMap[dpitch] = dm;

      instrument_number_mapping_t im = instrument_map[spitch];
      instrument_map[spitch] = instrument_map[dpitch];
      instrument_map[dpitch] = im;

      song->update(SC_DRUMMAP); //FINDMICHJETZT handle that properly!
   }
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

void DrumCanvas::modifySelected(MusEWidget::NoteInfo::ValType type, int delta)
      {
      audio->msgIdle(true);
      song->startUndo();
      for (MusEWidget::iCItem i = items.begin(); i != items.end(); ++i) {
            if (!(i->second->isSelected()))
                  continue;
            DEvent* e   = (DEvent*)(i->second);
            Event event = e->event();
            if (event.type() != Note)
                  continue;

            MidiPart* part = (MidiPart*)(e->part());
            Event newEvent = event.clone();

            switch (type) {
                  case MusEWidget::NoteInfo::VAL_TIME:
                        {
                        int newTime = event.tick() + delta;
                        if (newTime < 0)
                           newTime = 0;
                        newEvent.setTick(newTime);
                        }
                        break;
                  case MusEWidget::NoteInfo::VAL_LEN:
                        printf("DrumCanvas::modifySelected - MusEWidget::NoteInfo::VAL_LEN not implemented\n");
                        break;
                  case MusEWidget::NoteInfo::VAL_VELON:
                        printf("DrumCanvas::modifySelected - MusEWidget::NoteInfo::VAL_VELON not implemented\n");
                        break;
                  case MusEWidget::NoteInfo::VAL_VELOFF:
                        printf("DrumCanvas::modifySelected - MusEWidget::NoteInfo::VAL_VELOFF not implemented\n");
                        break;
                  case MusEWidget::NoteInfo::VAL_PITCH:
                        if (old_style_drummap_mode)
                        {
                        int pitch = event.pitch() - delta; // Reversing order since the drumlist is displayed in increasing order
                        if (pitch > 127)
                              pitch = 127;
                        else if (pitch < 0)
                              pitch = 0;
                        newEvent.setPitch(pitch);
                        }
                        else
                          printf("DrumCanvas::modifySelected - MusEWidget::NoteInfo::VAL_PITCH not implemented for new style drum editors\n");
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
  if (_tool == MusEWidget::CursorTool) {

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
          newItem(newItem(cursorPos.x(), cursorPos.y(), ourDrumMap[cursorPos.y()].lv1),false,true);
          keyPressed(cursorPos.y(), ourDrumMap[cursorPos.y()].lv1);
          keyReleased(cursorPos.y(), false);
          cursorPos.setX(getNextStep(cursorPos.x(),1, _stepSize));
          selectCursorEvent(getEventAtCursorPos());
          if (mapx(cursorPos.x()) < 0 || mapx(cursorPos.x()) > width())
            emit followEvent(cursorPos.x());
          return;
    }
    else if (key == shortcuts[SHRT_ADDNOTE_2].key) {
          newItem(newItem(cursorPos.x(), cursorPos.y(), ourDrumMap[cursorPos.y()].lv2),false,true);
          keyPressed(cursorPos.y(), ourDrumMap[cursorPos.y()].lv2);
          keyReleased(cursorPos.y(), false);
          cursorPos.setX(getNextStep(cursorPos.x(),1, _stepSize));
          selectCursorEvent(getEventAtCursorPos());
          if (mapx(cursorPos.x()) < 0 || mapx(cursorPos.x()) > width())
            emit followEvent(cursorPos.x());
          return;
    }
    else if (key == shortcuts[SHRT_ADDNOTE_3].key) {
          newItem(newItem(cursorPos.x(), cursorPos.y(), ourDrumMap[cursorPos.y()].lv3),false,true);
          keyPressed(cursorPos.y(), ourDrumMap[cursorPos.y()].lv3);
          keyReleased(cursorPos.y(), false);
          cursorPos.setX(getNextStep(cursorPos.x(),1, _stepSize));
          selectCursorEvent(getEventAtCursorPos());
          if (mapx(cursorPos.x()) < 0 || mapx(cursorPos.x()) > width())
            emit followEvent(cursorPos.x());
          return;
    }
    else if (key == shortcuts[SHRT_ADDNOTE_4].key) {
          newItem(newItem(cursorPos.x(), cursorPos.y(), ourDrumMap[cursorPos.y()].lv4),false,true);
          keyPressed(cursorPos.y(), ourDrumMap[cursorPos.y()].lv4);
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
  if (_tool == MusEWidget::CursorTool)
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
    if (_tool != MusEWidget::CursorTool)
      return 0;
    if (instrument_map[cursorPos.y()].tracks.contains(curPart->track()))
    {
      EventList* el = curPart->events();
      iEvent lower  = el->lower_bound(cursorPos.x()-curPart->tick());
      iEvent upper  = el->upper_bound(cursorPos.x()-curPart->tick());
      int curPitch = instrument_map[cursorPos.y()].pitch;
      for (iEvent i = lower; i != upper; ++i) {
        Event &ev = i->second;
        if (ev.isNote()  &&  ev.pitch() == curPitch)
          return &ev;
      }
    }
    // else or if the for loop didn't find anything
    return 0;
}
//---------------------------------------------------------
//   selectCursorEvent
//---------------------------------------------------------
void DrumCanvas::selectCursorEvent(Event *ev)
{
  for (MusEWidget::iCItem i = items.begin(); i != items.end(); ++i)
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
  if (!old_style_drummap_mode)
  {
    printf("THIS SHOULD NEVER HAPPEN: DrumCanvas::moveAwayUnused() cannot be used in new style mode\n");
    return;
  }
	
	QSet<int> used;
	for (MusEWidget::iCItem it=items.begin(); it!=items.end(); it++)
	{
		const Event& ev=it->second->event();
		
		if (ev.type()==Note)
			used.insert(ev.pitch());
	}
	
	int count=0;
	for (QSet<int>::iterator it=used.begin(); it!=used.end();)
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
void DrumCanvas::midiNote(int pitch, int velo) //FINDMICH later.
      {
      if (debugMsg) printf("DrumCanvas::midiNote: pitch=%i, velo=%i\n", pitch, velo);

      if (_midiin && _steprec && curPart
         && !audio->isPlaying() && velo && pos[0] >= start_tick
         /* && pos[0] < end_tick [removed by flo93: this is handled in steprec->record()] */
         && !(MusEGlobal::globalKeyState & Qt::AltModifier)) {
            steprec->record(curPart,drumInmap[pitch],ourDrumMap[(int)drumInmap[pitch]].len,editor->raster(),velo,MusEGlobal::globalKeyState&Qt::ControlModifier,MusEGlobal::globalKeyState&Qt::ShiftModifier);
         }
      }


int DrumCanvas::pitch_and_track_to_instrument(int pitch, Track* track)
{
  for (int i=0; i<instrument_map.size(); i++)
    if (instrument_map[i].tracks.contains(track) && instrument_map[i].pitch==pitch)
      return i;
  
  printf("ERROR: DrumCanvas::pitch_and_track_to_instrument() called with invalid arguments!\n");
  return -1;
}

void DrumCanvas::propagate_drummap_change(int instr)
//FINDMICHJETZT does that work properly?
{
  const QSet<Track*>& tracks=instrument_map[instr].tracks;
  int index=instrument_map[instr].track_dlist_index;
  
  for (QSet<Track*>::const_iterator it = tracks.begin(); it != tracks.end(); it++)
    dynamic_cast<MidiTrack*>(*it)->drummap()[index] = ourDrumMap[instr];
}
