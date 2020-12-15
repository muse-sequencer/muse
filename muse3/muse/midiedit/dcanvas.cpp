//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: dcanvas.cpp,v 1.16.2.10 2009/10/15 22:45:50 terminator356 Exp $
//  (C) Copyright 1999 Werner Schweer (ws@seh.de)
//  (C) Copyright 2016 Tim E. Real (terminator356 on users dot sourceforge dot net)
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
#include <QMessageBox>
#include <QClipboard>
#include <QDrag>
#include <QDragLeaveEvent>
#include <QPolygon>
#include <QDragEnterEvent>
#include <QDragMoveEvent>
#include <QDropEvent>
#include <QResizeEvent>
#include <QList>
#include <QPair>
#include <QToolTip>

#include <stdio.h>
#include <limits.h>
#include <errno.h>

#include "dcanvas.h"
#include "midieditor.h"
#include "drumedit.h"
#include "drummap.h"
#include "drum_ordering.h"
#include "event.h"
#include "mpevent.h"
#include "xml.h"
#include "globals.h"
#include "midiport.h"
#include "audio.h"
#include "midi_consts.h"
#include "shortcuts.h"
#include "icons.h"
#include "functions.h"
#include "helper.h"
#include "operations.h"
#include "gconfig.h"
#include "app.h"

#define CARET   10
#define CARET2   5

using MusEGlobal::debugMsg;
using MusEGlobal::heavyDebugMsg;
using MusECore::Track;
using MusECore::MidiTrack;

namespace MusEGui {

//---------------------------------------------------------
//   DEvent
//---------------------------------------------------------

DEvent::DEvent(MusECore::Event e, MusECore::Part* p, int instr)
  : EItem(e, p)
      {
      int y  = instr * TH + TH/2;
      int tick = e.tick() + p->tick();
      setPos(QPoint(tick, y));
      setBBox(QRect(-CARET2, -CARET2, CARET, CARET));
      // Give the moving point an initial value.
      setMp(pos());
      }

//---------------------------------------------------------
//   addItem
//---------------------------------------------------------

CItem* DrumCanvas::addItem(MusECore::Part* part, const MusECore::Event& event)
      {
      if (signed(event.tick())<0) {
            printf("ERROR: trying to add event before current part!\n");
            return nullptr;
      }
      
      int instr=pitch_and_track_to_instrument(event.pitch(), part->track());
      if (instr<0)
      {
        if (heavyDebugMsg) printf("trying to add event which is hidden or not in any part known to me\n");
        return nullptr;
      }
      
      DEvent* ev = new DEvent(event, part, instr);
      items.add(ev);
      
      int diff = event.endTick()-part->lenTick();
      if (diff > 0)  {// too short part? extend it
            part->setLenTick(part->lenTick()+diff);
            }
      
      return ev;
      }

//---------------------------------------------------------
//   DrumCanvas
//---------------------------------------------------------

DrumCanvas::DrumCanvas(MidiEditor* pr, QWidget* parent, int sx,
   int sy, const char* name)
   : EventCanvas(pr, parent, sx, sy, name)
      {
      setObjectName("DrumCanvas");

      drumEditor=static_cast<DrumEdit*>(pr);
      
      setStatusTip(tr("Drum canvas: Use Pencil tool to create and edit events, Pointer tool to select, Cursor tool for special keyboard entry mode (arrow keys, V, B, N, M, Del). Press F1 for help."));

      ourDrumMap=nullptr;
      rebuildOurDrumMap();
      
      setVirt(false);
      cursorPos= QPoint(0,0);
      _stepSize=1;
      
      steprec=new MusECore::StepRec(nullptr);
      
      songChanged(SC_TRACK_INSERTED);
      connect(MusEGlobal::song, SIGNAL(midiNote(int, int)), SLOT(midiNote(int,int)));
      }

DrumCanvas::~DrumCanvas()
{
  if (must_delete_our_drum_map && ourDrumMap!=nullptr)
    delete [] ourDrumMap;
  
  delete steprec;
}

//---------------------------------------------------------
//   index2Note
//   Return false if invalid index
//---------------------------------------------------------

bool DrumCanvas::index2Note(int index, int* port, int* channel, int* note)
{
      if ((index<0) || (index>=getOurDrumMapSize()))
        return false;

      int mport, ch;
      // Default to track port if -1 and track channel if -1.
      MusECore::Track* track = nullptr;
      MusECore::MidiTrack* mt = nullptr;
      if(ourDrumMap[index].port == -1)
      {
        track = *instrument_map[index].tracks.begin();
        if(!track->isMidiTrack())
          return false;
        mt = static_cast<MusECore::MidiTrack*>(track);
        mport = mt->outPort();
      }
      else
        mport = ourDrumMap[index].port;

      if(ourDrumMap[index].channel == -1)
      {
        if(!track)
        {
          track = *instrument_map[index].tracks.begin();
          if(!track->isMidiTrack())
            return false;
          mt = static_cast<MusECore::MidiTrack*>(track);
        }
        ch = mt->outChannel();
      }
      else
        ch = ourDrumMap[index].channel;

      if(port)
        *port = mport;
      if(channel)
        *channel = ch;
      if(note)
        //*note = old_style_drummap_mode ? ourDrumMap[index].anote : instrument_map[index].pitch;
        *note = ourDrumMap[index].anote;

      return true;
}

//---------------------------------------------------------
//   moveCanvasItems
//---------------------------------------------------------

MusECore::Undo DrumCanvas::moveCanvasItems(CItemMap& items, int dp, int dx, DragType dtype, bool rasterize)
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
      CItem* ci = ici->second;
      ci->setMoving(false);

      if(ci->part() != part)
        continue;
      
      int x = ci->pos().x() + dx;
      int y = pitch2y(y2pitch(ci->pos().y()) + dp);
      QPoint newpos = QPoint(x, y);
      if(rasterize)
        newpos = raster(newpos);
      
      // Test moving the item...
      DEvent* nevent = (DEvent*) ci;
      MusECore::Event event    = nevent->event();
      x              = newpos.x();
      if(x < 0)
        x = 0;
      int ntick = (rasterize ? editor->rasterVal(x) : x) - part->tick();
      if(ntick < 0)
        ntick = 0;
      int diff = ntick + event.lenTick() - part->lenTick();
      
      // If moving the item would require a new part size...
      if(diff > npartoffset)
        npartoffset = diff;
    }
        
    if(npartoffset > 0)
    {    
      MusECore::iPartToChange ip2c = parts2change.find(part);
      if(ip2c == parts2change.end())
      {
        MusECore::PartToChange p2c = {nullptr, npartoffset};
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
		std::vector< CItem* > doneList;
		typedef std::vector< CItem* >::iterator iDoneList;
		
		for(iCItem ici = items.begin(); ici != items.end(); ++ici) 
		{
			CItem* ci = ici->second;
			
      const QPoint oldpos(ci->pos());
			int x = ci->pos().x();
			int y = ci->pos().y();
			int nx = x + dx;
			int ny = pitch2y(y2pitch(y) + dp);
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
				if (moveItem(operations, ci, newpos, dtype, rasterize) == false) //error?
        {
          QMessageBox::warning(this, tr("Moving items failed"), tr("The selection couldn't be moved, because at least one note would be moved into a track which is different from both the original track and the current part's track.\nChanging the current part with ALT+LEFT/RIGHT may help."));
          return MusECore::Undo(); //return empty list
        }
				doneList.push_back(ci);
			}
			ci->move(newpos);
      
			itemReleased(ci, oldpos);

			if(dtype == MOVE_COPY || dtype == MOVE_CLONE)
						selectItem(ci, false);
		}  

		itemsReleased();

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

bool DrumCanvas::moveItem(MusECore::Undo& operations, CItem* item, const QPoint& pos, DragType dtype, bool rasterize)
      {
      DEvent* nevent   = (DEvent*) item;
      
      MusECore::MidiPart* part   = (MusECore::MidiPart*)nevent->part();
      MusECore::MidiPart* dest_part   = part;
      
      int instrument        = y2pitch(pos.y());
      if (instrument<0) instrument=0;
      if (instrument>=getOurDrumMapSize()) instrument=getOurDrumMapSize()-1;
      MusECore::Event event = nevent->event();
      if (!instrument_map[instrument].tracks.contains(dest_part->track()))
      {
        if (debugMsg)
          printf("trying to move an event into a different track. checking if curPart is set correctly...\n");
        
        if (!instrument_map[instrument].tracks.contains(curPart->track()))
        {
          printf ("ERROR: tried to move an event into a track which is different from both the initial part's and the curPart's track! ignoring this one...\n");
          return false;
        }
        else
          dest_part=(MusECore::MidiPart*)curPart;
      }

      int x            = pos.x();
      if (x < 0)
            x = 0;
      int ntick        = (rasterize ? editor->rasterVal(x) : x) - dest_part->tick();
      if (ntick < 0)
            ntick = 0;

      event.setSelected(false);
      MusECore::Event newEvent   = event.clone();
      newEvent.setSelected(true);

      int ev_pitch = instrument_map[instrument].pitch;
      newEvent.setPitch(ev_pitch);
      newEvent.setTick(ntick);

      // don't check, whether the new event is within the part
      // at this place. with operation groups, the part isn't
      // resized yet. (flo93)
      
      if (dtype == MOVE_COPY || dtype == MOVE_CLONE)
            operations.push_back(MusECore::UndoOp(MusECore::UndoOp::AddEvent, newEvent, dest_part, false, false));
      else
      {
            if (dest_part == part)
                operations.push_back(MusECore::UndoOp(MusECore::UndoOp::ModifyEvent, newEvent, event, part, false, false));
            else
            {
                operations.push_back(MusECore::UndoOp(MusECore::UndoOp::DeleteEvent, event, part, false, false));
                operations.push_back(MusECore::UndoOp(MusECore::UndoOp::AddEvent, newEvent, dest_part, false, false));
            }
      }
      return true;
}

//---------------------------------------------------------
//   newItem
//---------------------------------------------------------

CItem* DrumCanvas::newItem(const QPoint& p, int state)
      {
      int instr = y2pitch(p.y());
      if ((instr<0) || (instr>=getOurDrumMapSize()))
        return nullptr;

      int k4  = (Qt::MetaModifier | Qt::AltModifier);
      //int nk4 = Qt::ControlModifier;
      
      int k2  = Qt::MetaModifier;
      int nk2 = (Qt::ControlModifier | Qt::AltModifier);
      
      int k1  = (Qt::ControlModifier | Qt::MetaModifier);
      int nk1 = Qt::AltModifier;
      
      int velo  = ourDrumMap[instr].lv3;
      if      ((state & k4) == k4) // && !(state & nk4))
            velo = ourDrumMap[instr].lv4;
      else if ((state & k2) == k2 && !(state & nk2))
            velo = ourDrumMap[instr].lv2;
      else if ((state & k1) == k1 && !(state & nk1))
            velo = ourDrumMap[instr].lv1;
      int tick = p.x();
      if(tick < 0)
        tick = 0;
      if(!(state & Qt::ShiftModifier))
        tick = editor->rasterVal(tick);
      return newItem(tick, instr, velo);
      }

//---------------------------------------------------------
//   newItem
//---------------------------------------------------------

CItem* DrumCanvas::newItem(int tick, int instrument, int velocity)
{
  if ((instrument<0) || (instrument>=getOurDrumMapSize()))
    return nullptr;

  if (!instrument_map[instrument].tracks.contains(curPart->track()))
  {
    if (debugMsg)
      printf("tried to create a new Item which cannot be inside the current track. looking for destination part...\n");
    
    QSet<MusECore::Part*> parts = parts_at_tick(tick, instrument_map[instrument].tracks);
    
    if (parts.count() != 1)
    {
      QMessageBox::warning(this, tr("Creating event failed"), tr("Couldn't create the event, because the currently selected part isn't the same track, and the selected instrument could be either on no or on multiple parts, which is ambiguous.\nSelect the destination part, then try again."));
      return nullptr;
    }
    else
    {
      setCurrentPart(*parts.begin());
    }
  }
  // else or if we found an alternative part (which has now been set as curPart)

  tick    -= curPart->tick();
  if (tick < 0)
        return nullptr;
  MusECore::Event e(MusECore::Note);
  e.setTick(tick);
  e.setPitch(instrument_map[instrument].pitch);
  e.setVelo(velocity);
  e.setLenTick(ourDrumMap[instrument].len);
  if(_playEvents)
  {
    int pitch, port, channel;
    if(index2Note(instrument, &port, &channel, &pitch))
      startPlayEvent(pitch, e.velo(), port, channel);
  }
  return new DEvent(e, curPart, instrument);
}

//---------------------------------------------------------
//   newItem
//---------------------------------------------------------
void DrumCanvas::newItem(CItem* item, bool noSnap) {
     newItem(item, noSnap,true);
}

void DrumCanvas::newItem(CItem* item, bool noSnap, bool replace)
{
   if(!item)
   {
     printf("THIS SHOULD NEVER HAPPEN: DrumCanvas::newItem called with nullptr item!\n");
     return;
   }
     
    DEvent* nevent = (DEvent*) item;
    MusECore::Event event    = nevent->event();
    MusECore::Part* part = nevent->part();
    int ptick = part->tick();
    int x = item->x();
    if (x<ptick)
          x=ptick;
    if (!noSnap)
          x = editor->rasterVal(x);
    if (x<ptick)
          x=ptick;
    event.setTick(x - ptick);
    int npitch = y2pitch(item->y());
    if ((npitch<0) || (npitch>=getOurDrumMapSize()))
      return;
    npitch = instrument_map[npitch].pitch;
    event.setPitch(npitch);
    event.setSelected(true);
    // check for existing event
    //    if found change command semantic from insert to delete
    MusECore::Undo operations;
    std::pair<MusECore::ciEvent,MusECore::ciEvent> range = 
      part->events().equal_range(event.type() == MusECore::Wave ? event.frame() : event.tick());
    MusECore::Event oev;
    bool found = false;
    for(MusECore::ciEvent i = range.first; i != range.second; ++i)
    {
      oev = i->second;
      if(!oev.isNote())
        continue;
      if(oev.pitch() == npitch) 
      {
        found = true;
        break;
      }
    }
    
    // Indicate do undo, and do not do port controller values and clone parts.
    int diff = event.endTick()-part->lenTick();
    if (! ((diff > 0) && part->hasHiddenEvents()) ) //operation is allowed
    {
      if(found)
      {
        if(replace)
          operations.push_back(MusECore::UndoOp(MusECore::UndoOp::ModifyEvent,event, oev, part, false, false));
        else
          operations.push_back(MusECore::UndoOp(MusECore::UndoOp::DeleteEvent,oev, part, false, false));
      }
      else
        operations.push_back(MusECore::UndoOp(MusECore::UndoOp::AddEvent,event, part, false, false));
      
      if (diff > 0) // part must be extended?
      {
            schedule_resize_all_same_len_clone_parts(part, event.endTick(), operations);
            printf("newItem: extending\n");
      }
    }
    else // forbid action by not applying it
    {
      if(found)
        operations.push_back(MusECore::UndoOp(MusECore::UndoOp::DeleteEvent,oev, part, false, false));
    }
      
    if(!operations.empty())
      MusEGlobal::song->applyOperationGroup(operations);
    else
      songChanged(SC_EVENT_INSERTED); //this forces an update of the itemlist, which is necessary
                                      //to remove "forbidden" events from the list again
}

//---------------------------------------------------------
//   deleteItem
//---------------------------------------------------------

bool DrumCanvas::deleteItem(CItem* item)
      {
      MusECore::Event ev = ((DEvent*)item)->event();
      // Indicate do undo, and do not do port controller values and clone parts. 
      MusEGlobal::song->applyOperation(MusECore::UndoOp(MusECore::UndoOp::DeleteEvent,
                      ev, ((DEvent*)item)->part(), false, false));
      return false;
      }

//---------------------------------------------------------
//   itemPressed
//---------------------------------------------------------

void DrumCanvas::itemPressed(const CItem* item)
      {
      if (!_playEvents)
            return;
      MusECore::Event e = ((DEvent*)item)->event();
      int index = e.pitch();
      // play note:

      for (int i = 0; i < instrument_map.size(); ++i) {
          if (instrument_map.at(i).pitch == index) {
              index = i;
              break;
          }
      }
      int pitch, port, channel;
      if(index2Note(index, &port, &channel, &pitch))
        startPlayEvent(pitch, e.velo(), port, channel);
      }

//---------------------------------------------------------
//   itemReleased
//---------------------------------------------------------

void DrumCanvas::itemReleased(const CItem* item, const QPoint&)
      {
      const int oindex = y2pitch(item->mp().y());
//       for (int i = 0; i < instrument_map.size(); ++i) {
//           if (instrument_map.at(i).pitch == index) {
//               index = i;
//               break;
//           }
//       }

      int opitch, oport, ochannel;
      if(!index2Note(oindex, &oport, &ochannel, &opitch))
      {
        // Stop any playing notes:
        stopPlayEvents();
        return;
      }
      
      // stop note:
      stopStuckNote(oport, ochannel, opitch);
      }


//---------------------------------------------------------
//   itemMoving
//---------------------------------------------------------

void DrumCanvas::itemMoving(const CItem* item, const QPoint& newMP)
{
      const int oindex = y2pitch(item->mp().y());
      const int index = y2pitch(newMP.y());
      int opitch, oport, ochannel, pitch, port, channel;
      if(!index2Note(oindex, &oport, &ochannel, &opitch))
      {
        // Stop any playing notes:
        stopPlayEvents();
        return;
      }
      if(!index2Note(index, &port, &channel, &pitch))
      {
        // Stop any playing notes:
        stopPlayEvents();
        return;
      }
      
      if(port == oport && channel == ochannel && pitch == opitch)
        return;

      // Stop any playing note:
      stopStuckNote(port, channel, opitch);
}

//---------------------------------------------------------
//   itemMoved
//---------------------------------------------------------

void DrumCanvas::itemMoved(const CItem* item, const QPoint& oldMP)
      {
      const int oindex = y2pitch(oldMP.y());
      const int index = y2pitch(item->mp().y());
      int opitch, oport, ochannel, pitch, port, channel;
      if(!index2Note(oindex, &oport, &ochannel, &opitch))
      {
        // Stop any playing notes:
        stopPlayEvents();
        return;
      }
      if(!index2Note(index, &port, &channel, &pitch))
      {
        // Stop any playing notes:
        stopPlayEvents();
        return;
      }
      
      if(port == oport && channel == ochannel && pitch == opitch)
        return;

      if(_playEvents)
      {
        // Unlike with PianoRoll::itemMoved(), don't play multiple notes (no concept of chords).
        if(item == curItem)
        {
            const MusECore::Event e = ((DEvent*)item)->event();
            // play note:
            startPlayEvent(pitch, e.velo(), port, channel);
            }
      }
      }

//---------------------------------------------------------
//   drawItem
//---------------------------------------------------------

void DrumCanvas::drawItem(QPainter&p, const CItem*item, const QRect& mr, const QRegion&)
      {
      DEvent* e   = (DEvent*) item;
      int mx = 0, my = 0;
      mx = mapx(item->pos().x());
      my = mapy(item->pos().y());
      
      QPolygon pa(4);
      pa.setPoint(0, mx - CARET2, my);
      pa.setPoint(1, mx,          my - CARET2);
      pa.setPoint(2, mx + CARET2, my);
      pa.setPoint(3, mx,          my + CARET2);
      QRect r(pa.boundingRect());
      r = r.intersected(mr);
      if(!r.isValid())
        return;
      
      QPen pen;
      pen.setCosmetic(true);
      pen.setColor(Qt::black);
      p.setPen(pen);
      
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
            p.setBrush(MusEGlobal::config.midiItemSelectedColor);
//            p.setBrush(Qt::black);
      }
      else
      {
            int velo    = e->event().velo();
            MusECore::DrumMap* dm = &ourDrumMap[y2pitch(my)]; //Get the drum item
            QColor color;
            if (velo < dm->lv1)
                  color.setRgb(240, 240, 255);
            else if (velo < dm->lv2)
                  color.setRgb(200, 200, 255);
            else if (velo < dm->lv3)
                  color.setRgb(170, 170, 255);
            else
                  color = MusEGlobal::config.midiItemColor;
//            color.setRgb(0, 0, 255);
            p.setBrush(color);
      }
            
      p.drawPolygon(pa);
      }
      
//---------------------------------------------------------
//   drawMoving
//    draws moving items
//---------------------------------------------------------

void DrumCanvas::drawMoving(QPainter& p, const CItem* item, const QRect& rect, const QRegion&)
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
      mr = mr.intersected(rect);
      if(!mr.isValid())
        return;
      QPen pen;
      pen.setCosmetic(true);
      pen.setColor(Qt::black);
      p.setPen(pen);
      p.setBrush(Qt::black);
      p.drawPolygon(pa);
    }

//---------------------------------------------------------
//   drawCanvas
//---------------------------------------------------------

void DrumCanvas::drawCanvas(QPainter& p, const QRect& mr, const QRegion& rg)
      {
      const QRect ur = mapDev(mr);

      int ux = ur.x();
      if(ux < 0)
        ux = 0;
      const int uy = ur.y();
      const int uw = ur.width();
      const int uh = ur.height();
      const int ux_2 = ux + uw;
      const int uy_2 = uy + uh;

      QPen pen;
      pen.setCosmetic(true);
      pen.setColor(MusEGlobal::config.midiDividerColor);
      p.setPen(pen);
      
      if (MusEGlobal::config.canvasShowGrid || MusEGlobal::config.canvasShowGridHorizontalAlways)
      {
        //---------------------------------------------------
        //  horizontal lines
        //---------------------------------------------------

        int uyy  = ((uy-1) / TH) * TH + TH;
        
  // For testing...
  //       fprintf(stderr, "DrumCanvas::drawCanvas ux:%d uy:%d uw:%d uh:%d uyy:%d\n", ux, uy, uw, uh, uyy);
        
        for (; uyy < uy_2; uyy += TH) {
              p.drawLine(ux, uyy, ux_2, uyy);
              }
      }

      if (MusEGlobal::config.canvasShowGrid)
      {
        //---------------------------------------------------
        // vertical lines
        //---------------------------------------------------

        if (MusEGlobal::config.canvasShowGrid)
          drawTickRaster(p, mr, rg, editor->raster(), false, false, false,
                        MusEGlobal::config.midiCanvasBeatColor,
                        MusEGlobal::config.midiCanvasBeatColor,
                        MusEGlobal::config.midiCanvasFineColor,
                        MusEGlobal::config.midiCanvasBarColor);
      }
}

//---------------------------------------------------------
//   drawTopItem
//---------------------------------------------------------

void DrumCanvas::drawTopItem(QPainter& p, const QRect&, const QRegion&)
{
  // draw cursor
  if (_tool == CursorTool) {
    QPen pen;
    pen.setCosmetic(true);
    pen.setColor(Qt::black);
    p.setPen(pen);

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
      else if (pitch<0)
            pitch = 0;
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
                  for (iCItem k = items.begin(); k != items.end(); ++k) {
                        DEvent* nevent = (DEvent*)(k->second);
                        MusECore::Part* part     = nevent->part();
                        MusECore::Event event    = nevent->event();
                        unsigned tick  = event.tick() + part->tick();
                        if (tick < MusEGlobal::song->lpos() || tick >= MusEGlobal::song->rpos())
                              selectItem(k->second, true);
                        else
                              selectItem(k->second, false);
                        }
                  break;
            case CMD_RANGE_TO_SELECTION:
                setRangeToSelection();
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

            case CMD_FIXED_LEN: //Set notes to the length specified in the drummap
                  if (!selectionSize())
                        break;
                  MusEGlobal::song->startUndo();
                  for (iCItem k = items.begin(); k != items.end(); ++k) {
                        if (k->second->isSelected()) {
                              DEvent* devent = (DEvent*)(k->second);
                              MusECore::Event event    = devent->event();
                              MusECore::Event newEvent = event.clone();
                              // newEvent.setLenTick(drumMap[event.pitch()].len);
                              newEvent.setLenTick(ourDrumMap[y2pitch(devent->y())].len);
                              // Operation is undoable but do not start/end undo.
                              // Indicate do not do port controller values and clone parts.
                              MusEGlobal::song->applyOperation(MusECore::UndoOp(MusECore::UndoOp::ModifyEvent,
                                                newEvent, event, devent->part(), false, false), MusECore::Song::OperationUndoable);
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
                        spos = MusEGlobal::sigmap.raster1(spos, editor->rasterStep(pos[0]));
                      }
                      if(spos < 0)
                        spos = 0;
                      MusECore::Pos p(spos,true);
                      MusEGlobal::song->setPos(MusECore::Song::CPOS, p, true, true, true);
                  }
                  break;
            case CMD_RIGHT:
                  {
                      int spos = MusEGlobal::sigmap.raster2(pos[0] + 1, editor->rasterStep(pos[0]));    // Nudge by +1, then snap up with raster2.
                      MusECore::Pos p(spos,true);
                      MusEGlobal::song->setPos(MusECore::Song::CPOS, p, true, true, true);
                  }
                  break;
            case CMD_LEFT_NOSNAP:
                  {
                    printf("left no snap\n");
                  int spos = pos[0] - editor->rasterStep(pos[0]);
                  if (spos < 0)
                        spos = 0;
                  MusECore::Pos p(spos,true);
                  MusEGlobal::song->setPos(MusECore::Song::CPOS, p, true, true, true); //CDW
                  }
                  break;
            case CMD_RIGHT_NOSNAP:
                  {
                  MusECore::Pos p(pos[0] + editor->rasterStep(pos[0]), true);
                  MusEGlobal::song->setPos(MusECore::Song::CPOS, p, true, true, true); //CDW
                  }
                  break;
            }
      itemSelectionsChanged();
      redraw();
      }


//---------------------------------------------------------
//   startDrag
//---------------------------------------------------------

void DrumCanvas::startDrag(CItem* /* item*/, DragType t)
{
   QMimeData* md = selected_events_to_mime(partlist_to_set(editor->parts()), 1);

   if (md)
   {
      // "Note that setMimeData() assigns ownership of the QMimeData object to the QDrag object.
      //  The QDrag must be constructed on the heap with a parent QWidget to ensure that Qt can
      //  clean up after the drag and drop operation has been completed. "
      QDrag* drag = new QDrag(this);
      drag->setMimeData(md);

      if (t == MOVE_COPY || t == MOVE_CLONE)
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
      if(velocity > 127)
        velocity = 127;
      else if(velocity <= 0)
        velocity = 1;
      
      // Stop all notes.
      stopPlayEvents();

      if ((index<0) || (index>=getOurDrumMapSize()))
        return;
      // called from DList - play event
      // play note:
      if(_playEvents)
      {
        int pitch, port, channel;
        if(index2Note(index, &port, &channel, &pitch))
          startPlayEvent(pitch, velocity, port, channel);
      }
      
      if (_steprec) /* && pos[0] >= start_tick && pos[0] < end_tick [removed by flo93: this is handled in steprec->record] */
      {
        if ( curPart && instrument_map[index].tracks.contains(curPart->track()) )
            steprec->record(curPart,instrument_map[index].pitch,ourDrumMap[index].len,editor->raster(),velocity,MusEGlobal::globalKeyState&Qt::ControlModifier,MusEGlobal::globalKeyState&Qt::ShiftModifier, -1 /* invalid pitch as "really played" -> the "insert rest" feature is never triggered */);
        else
        {
          QSet<MusECore::Part*> parts = parts_at_tick(pos[0], instrument_map[index].tracks);
          
          if (parts.count() != 1)
            QMessageBox::warning(this, tr("Recording event failed"), tr("Couldn't record the event, because the currently selected part isn't the same track, and the instrument to be recorded could be either on no or on multiple parts, which is ambiguous.\nSelect the destination part, then try again."));
          else
            steprec->record(*parts.begin(), instrument_map[index].pitch,ourDrumMap[index].len,editor->raster(),velocity,MusEGlobal::globalKeyState&Qt::ControlModifier,MusEGlobal::globalKeyState&Qt::ShiftModifier, -1 /* invalid pitch as "really played" -> the "insert rest" feature is never triggered */);
          
        }
      }            
}

//---------------------------------------------------------
//   keyReleased
//---------------------------------------------------------

void DrumCanvas::keyReleased(int, bool)
      {
      // release note:
      if(_playEvents)
        stopPlayEvents();
      }

//---------------------------------------------------------
//   mapChanged
//---------------------------------------------------------

void DrumCanvas::mapChanged(int spitch, int dpitch)
{
   // spitch may be the same as dpitch! and something in here must be executed
   // even if they're same (i assume it's song->update(SC_DRUMMAP)) (flo93)
   
    if (dpitch!=spitch)
    {
      using MusEGlobal::global_drum_ordering_t;
      using MusEGlobal::global_drum_ordering;
      
      for (QSet<MusECore::Track*>::iterator it=instrument_map[spitch].tracks.begin();
                                            it!=instrument_map[spitch].tracks.end(); it++)
      {
        if (dynamic_cast<MusECore::MidiTrack*>(*it))
        dynamic_cast<MusECore::MidiTrack*>(*it)->set_drummap_ordering_tied_to_patch(false);
      }
      for (QSet<MusECore::Track*>::iterator it=instrument_map[dpitch].tracks.begin();
                                            it!=instrument_map[dpitch].tracks.end(); it++)
      {
        if (dynamic_cast<MusECore::MidiTrack*>(*it))
        dynamic_cast<MusECore::MidiTrack*>(*it)->set_drummap_ordering_tied_to_patch(false);
      }
      
      MusECore::DrumMap dm_temp = ourDrumMap[spitch];
      instrument_number_mapping_t im_temp = instrument_map[spitch];

      global_drum_ordering_t order_temp;
      for (global_drum_ordering_t::iterator it=global_drum_ordering.begin(); it!=global_drum_ordering.end();)
      {
        if (im_temp.pitch==it->second && im_temp.tracks.contains(it->first))
        {
          order_temp.push_back(*it);
          it=global_drum_ordering.erase(it);
        }
        else
          it++;
      }
      
      // the instrument represented by instrument_map[dpitch] is always the instrument
      // which will be immediately AFTER our dragged instrument. or it's invalid
      if (dpitch < getOurDrumMapSize())
      {
        for (global_drum_ordering_t::iterator it=global_drum_ordering.begin(); it!=global_drum_ordering.end(); it++)
          if (instrument_map[dpitch].pitch==it->second && instrument_map[dpitch].tracks.contains(it->first))
          {
            while (!order_temp.empty())
              it=global_drum_ordering.insert(it, order_temp.takeLast());

            break;
          }
      }
      else
      {
        global_drum_ordering_t::iterator it=global_drum_ordering.end();
        while (!order_temp.empty())
          it=global_drum_ordering.insert(it, order_temp.takeLast());
      }





      if (dpitch > spitch)
      {
        for (int i=spitch; i<dpitch-1; i++)
        {
          ourDrumMap[i]=ourDrumMap[i+1];
          instrument_map[i]=instrument_map[i+1];
        }
        
        ourDrumMap[dpitch-1] = dm_temp;
        instrument_map[dpitch-1] = im_temp;
      }
      else if (spitch > dpitch)
      {
        for (int i=spitch; i>dpitch; i--)
        {
          ourDrumMap[i]=ourDrumMap[i-1];
          instrument_map[i]=instrument_map[i-1];
        }
        
        ourDrumMap[dpitch] = dm_temp;
        instrument_map[dpitch] = im_temp;
      }
    }
    
    
    MusEGlobal::song->update(SC_DRUMMAP); // this causes a complete rebuild of ourDrumMap
                                          // which also handles the changed order in all
                                          // other drum editors
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

void DrumCanvas::modifySelected(NoteInfo::ValType type, int val, bool delta_mode)
      {
      QList< QPair<int,MusECore::Event> > already_done;
      MusECore::Undo operations;
      for (iCItem i = items.begin(); i != items.end(); ++i) {
            if (!(i->second->isSelected()))
                  continue;
            DEvent* e   = (DEvent*)(i->second);
            MusECore::Event event = e->event();
            if (event.type() != MusECore::Note)
                  continue;

            MusECore::MidiPart* part = (MusECore::MidiPart*)(e->part());

            if (already_done.contains(QPair<int,MusECore::Event>(part->clonemaster_sn(), event)))
              continue;
            
            MusECore::Event newEvent = event.clone();

            switch (type) {
                  case NoteInfo::VAL_TIME:
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
                  case NoteInfo::VAL_LEN:
                        {
                        int len = val;
                        if(delta_mode)
                          len += event.lenTick();
                        if (len < 1)
                              len = 1;
                        newEvent.setLenTick(len);
                        }
                        break;
                  case NoteInfo::VAL_VELON:
                        {
                        int velo = val;
                        if(delta_mode)
                          velo += event.velo();
                        if (velo > 127)
                              velo = 127;
                        else if (velo < 0)
                              // REMOVE Tim. Noteoff. Changed. Zero note on vel is not allowed now.
//                               velo = 0;
                              velo = 1;
                        newEvent.setVelo(velo);
                        }
                        break;
                  case NoteInfo::VAL_VELOFF:
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
                  case NoteInfo::VAL_PITCH:
                        {
                          int direction = -val;
                          for (int i = 0; i < instrument_map.size(); ++i) {
                              if (instrument_map.at(i).pitch == event.pitch()) {
                                  int nextPos = i + direction;
                                  if (nextPos> -1 && nextPos < instrument_map.size())
                                    newEvent.setPitch(instrument_map.at(nextPos).pitch);
                                  break;
                              }
                          }
                        }
                        break;
                  }

            operations.push_back(MusECore::UndoOp(MusECore::UndoOp::ModifyEvent, newEvent, event, part, false, false));

            already_done.append(QPair<int,MusECore::Event>(part->clonemaster_sn(), event));
            }
      MusEGlobal::song->applyOperationGroup(operations);
      }

//---------------------------------------------------------
//   curPartChanged
//---------------------------------------------------------

void DrumCanvas::curPartChanged()
      {
      EventCanvas::curPartChanged();
      editor->setWindowTitle(getCaption());
      }

//---------------------------------------------------------
//   getNextStep - gets next tick in the chosen direction
//                 when raster and stepSize are taken into account
//---------------------------------------------------------
int DrumCanvas::getNextStep(unsigned int currentPos, int basicStep, int stepSize)
{
  int newPos = currentPos; // newPos will be updated for every loop
  for (int i = 0; i < stepSize; i++)
  {
    if (basicStep > 0) // moving right
    {
      // Nudge by +1, then snap up with raster2.
      newPos = MusEGlobal::sigmap.raster2(newPos + basicStep, editor->rasterStep(newPos));
      if (newPos > signed(curPart->endTick() - editor->rasterStep(curPart->endTick())))
      {
        newPos = curPart->tick();
      }
    }
    else // moving left
    {
      // Nudge by -1, then snap up with raster1.
      newPos = MusEGlobal::sigmap.raster1(newPos + basicStep, editor->rasterStep(newPos));

      if (newPos < signed(curPart->tick()))
      {
        newPos = MusEGlobal::sigmap.raster1(curPart->endTick()-1, editor->rasterStep(curPart->endTick()));
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
    // NOTE: The inner NewItem may play the note. But let us not stop the note so shortly after playing it.
    //       So it is up to the corresponding keyRelease() to stop the note.
    else if ( key == shortcuts[SHRT_ADDNOTE_1].key ||
              key == shortcuts[SHRT_ADDNOTE_2].key ||
              key == shortcuts[SHRT_ADDNOTE_3].key ||
              key == shortcuts[SHRT_ADDNOTE_4].key)
    {
        if (key ==shortcuts[SHRT_ADDNOTE_1].key) {
          newItem(newItem(cursorPos.x(), cursorPos.y(), ourDrumMap[cursorPos.y()].lv1),false,true);
        }
        else if(key ==shortcuts[SHRT_ADDNOTE_2].key) {
          newItem(newItem(cursorPos.x(), cursorPos.y(), ourDrumMap[cursorPos.y()].lv2),false,true);
        }
        else if(key ==shortcuts[SHRT_ADDNOTE_3].key) {
          newItem(newItem(cursorPos.x(), cursorPos.y(), ourDrumMap[cursorPos.y()].lv3),false,true);
        }
        else if(key ==shortcuts[SHRT_ADDNOTE_4].key) {
          newItem(newItem(cursorPos.x(), cursorPos.y(), ourDrumMap[cursorPos.y()].lv4),false,true);
        }

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
//   keyRelease
//---------------------------------------------------------

void DrumCanvas::keyRelease(QKeyEvent* event)
{
  if (_tool == CursorTool)
  {
    if (_playEvents)
    {
      int key = event->key();
      if (((QInputEvent*)event)->modifiers() & Qt::ShiftModifier)
            key += Qt::SHIFT;
      if (((QInputEvent*)event)->modifiers() & Qt::AltModifier)
            key += Qt::ALT;
      if (((QInputEvent*)event)->modifiers() & Qt::ControlModifier)
            key+= Qt::CTRL;
      if (key == shortcuts[SHRT_ADDNOTE_1].key ||
          key == shortcuts[SHRT_ADDNOTE_2].key ||
          key == shortcuts[SHRT_ADDNOTE_3].key ||
          key == shortcuts[SHRT_ADDNOTE_4].key)
      {
        // Must stop note that was played.
                stopPlayEvents();
        return;
      }
    }
  }
  EventCanvas::keyRelease(event);
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
const MusECore::Event* DrumCanvas::getEventAtCursorPos()
{
    if (_tool != CursorTool)
      return 0;
    if (instrument_map[cursorPos.y()].tracks.contains(curPart->track()))
    {
      MusECore::ciEvent lower  = curPart->events().lower_bound(cursorPos.x()-curPart->tick());
      MusECore::ciEvent upper  = curPart->events().upper_bound(cursorPos.x()-curPart->tick());
      int curPitch = instrument_map[cursorPos.y()].pitch;
      for (MusECore::ciEvent i = lower; i != upper; ++i) {
        const MusECore::Event& ev = i->second;
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
void DrumCanvas::selectCursorEvent(const MusECore::Event* ev)
{
  for (iCItem i = items.begin(); i != items.end(); ++i)
  {
        MusECore::Event e = i->second->event();

        if (ev && ev->tick() == e.tick() && ev->pitch() == e.pitch() && e.isNote())
          i->second->setSelected(true);
        else
          i->second->setSelected(false);

  }
  itemSelectionsChanged();
}

//---------------------------------------------------------
//   midiNote
//---------------------------------------------------------
void DrumCanvas::midiNote(int pitch, int velo)
{
      using MusECore::Track;
      using MusECore::Part;
      
      if (debugMsg) printf("DrumCanvas::midiNote: pitch=%i, velo=%i\n", pitch, velo);

      if (_midiin && _steprec && !MusEGlobal::audio->isPlaying() && velo && !(MusEGlobal::globalKeyState & Qt::AltModifier) /* && pos[0] >= start_tick && pos[0] < end_tick [removed by flo93: this is handled in steprec->record()] */ )
      {
         if (pitch == MusEGlobal::rcSteprecNote) // skip the fancy code below, simply record a rest
         {
           if (curPart)
             steprec->record(curPart,0xdead,0xbeef,editor->raster(),velo,MusEGlobal::globalKeyState&Qt::ControlModifier,MusEGlobal::globalKeyState&Qt::ShiftModifier, pitch);
         }
         else
         {
            QSet<Track*> possible_dest_tracks;
            Part* rec_part=nullptr;
            int rec_index=-1;

            int ourDrumMapSize=getOurDrumMapSize();
            int i;
            for (i=0;i<ourDrumMapSize;i++)
            {
              if ( instrument_map[i].tracks.contains(curPart->track()) && ourDrumMap[i].enote==pitch)
              {
                rec_part=curPart;
                rec_index=i;
                break;
              }
              else if (ourDrumMap[i].enote==pitch)
                possible_dest_tracks.unite(instrument_map[i].tracks);
            }
            
            if (rec_part == nullptr) // if recording to curPart isn't possible
            {
              QSet<Part*> possible_dest_parts = parts_at_tick(pos[0], possible_dest_tracks);
              
              if (possible_dest_parts.count() != 1)
                QMessageBox::warning(this, tr("Recording event failed"), tr("Couldn't record the event, because the currently selected part isn't the same track, and the instrument to be recorded could be either on no or on multiple parts, which is ambiguous.\nSelect the destination part, then try again."));
              else
              {
                rec_part = *possible_dest_parts.begin();
                Track* dest_track=rec_part->track();
                
                for (i=0;i<ourDrumMapSize;i++)
                  if ( instrument_map[i].tracks.contains(dest_track) && ourDrumMap[i].enote==pitch)
                  {
                    rec_index=i;
                    break;
                  }
                
                if (rec_index==-1)
                {
                  printf("ERROR: THIS SHOULD NEVER HAPPEN: i found a destination part for step recording, but now i can't find the instrument any more in DrumCanvas::midiNote()?!\n");
                  QMessageBox::critical(this, tr("Internal error"), tr("Wtf, some nasty internal error which is actually impossible occurred. Check console output. Nothing recorded."));
                  rec_part=nullptr;
                }
              }
            }
            
            if (rec_part != nullptr)
              steprec->record(rec_part,instrument_map[rec_index].pitch,ourDrumMap[rec_index].len,editor->raster(),velo,MusEGlobal::globalKeyState&Qt::ControlModifier,MusEGlobal::globalKeyState&Qt::ShiftModifier, pitch);
         }   
      }
}


int DrumCanvas::pitch_and_track_to_instrument(int pitch, MusECore::Track* track)
{
  for (int i=0; i<instrument_map.size(); i++)
    if (instrument_map[i].tracks.contains(track) && instrument_map[i].pitch==pitch)
      return i;
  
  if (heavyDebugMsg) printf("DrumCanvas::pitch_and_track_to_instrument() called with invalid arguments.\n");
  return -1;
}

void DrumCanvas::propagate_drummap_change(int instrument, int fields, bool isReset, bool includeDefault, bool isInstrumentMod, bool doWholeMap)
{
  //fprintf(stderr, "DrumCanvas::propagate_track_drummap_change instrument:%d fields:%x isReset:%d isInstrumentMod:%d\n",
  //        instrument, fields, isReset, isInstrumentMod);
  const QSet<MusECore::Track*>& tracks=instrument_map[instrument].tracks;
  int index=instrument_map[instrument].pitch;

  MusECore::DrumMapTrackOperation* dmop = new MusECore::DrumMapTrackOperation;
  dmop->_isReset = isReset;
  dmop->_includeDefault = includeDefault;
  dmop->_doWholeMap = doWholeMap;
  dmop->_isInstrumentMod = isInstrumentMod;

  MusECore::PendingOperationList operations;
  MusECore::Track* t;
  for(QSet<MusECore::Track*>::const_iterator it = tracks.begin(); it != tracks.end(); it++)
  {
    t = *it;
    if(!t->isDrumTrack())
      continue;
    MusECore::MidiTrack* mt = static_cast<MusECore::MidiTrack*>(t);
    dmop->_tracks.push_back(mt);
  }

  if(isReset)
    dmop->_workingItemList.add(index, MusECore::WorkingDrumMapEntry(MusECore::DrumMap(), fields)); // Fixme: Dummy map. Should just be fields.
  else
    dmop->_workingItemList.add(index, MusECore::WorkingDrumMapEntry(ourDrumMap[instrument], fields));

  operations.add(MusECore::PendingOperationItem(dmop, MusECore::PendingOperationItem::ModifyTrackDrumMapItem));
  MusEGlobal::audio->msgExecutePendingOperations(operations, true);
}

int DrumCanvas::isWorkingMapInstrument(int instr, int fields) const
{
  const QSet<MusECore::Track*>& tracks=instrument_map[instr].tracks;
  int index=instrument_map[instr].pitch;

  MusECore::Track* t;
  MusECore::MidiTrack* mt;
  int ret = MusECore::WorkingDrumMapEntry::NoOverride;
  for (QSet<MusECore::Track*>::const_iterator it = tracks.begin(); it != tracks.end(); it++)
  {
    t = *it;
    if(!t->isDrumTrack())
      continue;
    mt = static_cast<MusECore::MidiTrack*>(t);
    // Don't pass a patch - ask it to take care of patch number for us.
    ret |= mt->isWorkingMapItem(index, fields);
  }
  return ret;
}

bool DrumCanvas::hasOverrides(int instr) const
{
  const QSet<MusECore::Track*>& tracks=instrument_map[instr].tracks;
  MusECore::Track* t;
  MusECore::MidiTrack* mt;
  for (QSet<MusECore::Track*>::const_iterator it = tracks.begin(); it != tracks.end(); it++)
  {
    t = *it;
    if(!t->isDrumTrack())
      continue;
    mt = static_cast<MusECore::MidiTrack*>(t);
    if(!mt->workingDrumMap()->empty())
      return true;
  }
  return false;
}

void DrumCanvas::resetOverridesForAllPatches(int instr)
{
  if(QMessageBox::warning(this, tr("Drum map"),
     tr("Reset the track's drum map with instrument defaults?"),
     QMessageBox::Ok | QMessageBox::Cancel, QMessageBox::Ok) == QMessageBox::Ok)
  {
    MusECore::PendingOperationList operations;
    const QSet<MusECore::Track*>& tracks=instrument_map[instr].tracks;
    MusECore::Track* t;
    MusECore::MidiTrack* mt;
    MusECore::WorkingDrumMapPatchList* new_wdmpl;
    MusECore::DrumMapTrackPatchReplaceOperation* dmop;
    for (QSet<MusECore::Track*>::const_iterator it = tracks.begin(); it != tracks.end(); it++)
    {
      t = *it;
      if(!t->isDrumTrack())
        continue;
      mt = static_cast<MusECore::MidiTrack*>(t);
      if(!mt->workingDrumMap()->empty())
      {
        // Completely blank replacement list.
        new_wdmpl = new MusECore::WorkingDrumMapPatchList();
        // The allocated WorkingDrumMapPatchList wdmpl will become the new list and the
        //  original lists will be deleted, in the operation following.
        dmop = new MusECore::DrumMapTrackPatchReplaceOperation;
        dmop->_isInstrumentMod = false; // Not instrument operation.
        dmop->_workingItemPatchList = new_wdmpl;
        dmop->_track = static_cast<MusECore::MidiTrack*>(t);
        operations.add(MusECore::PendingOperationItem(dmop, MusECore::PendingOperationItem::ReplaceTrackDrumMapPatchList));
      }
    }
    if(!operations.empty())
      MusEGlobal::audio->msgExecutePendingOperations(operations, true);
  }
}

void DrumCanvas::rebuildOurDrumMap()
{
  using MusECore::drummaps_almost_equal;
  using MusECore::Track;
  using MusECore::MidiTrack;
  using MusECore::TrackList;
  using MusECore::ciTrack;
  using MusECore::ciPart;
  using MusECore::DrumMap;
  using MusEGlobal::global_drum_ordering_t;
  using MusEGlobal::global_drum_ordering;
  
  //fprintf(stderr, "DrumCanvas::rebuildOurDrumMap\n");
  bool need_update = false;
  
  TrackList* tl=MusEGlobal::song->tracks();
  QList< QSet<Track*> > track_groups;
  QVector<instrument_number_mapping_t> old_instrument_map = instrument_map;
  
  instrument_map.clear();

  for (ciTrack track = tl->begin(); track!=tl->end(); track++)
  {
    ciPart p_it;
    for (p_it=drumEditor->parts()->begin(); p_it!=drumEditor->parts()->end(); p_it++)
      if (p_it->second->track() == *track)
        break;
    
    if (p_it!=drumEditor->parts()->end()) // if *track is represented by some part in this editor
    {
      bool inserted=false;
      
      switch (drumEditor->group_mode())
      {
        case DrumEdit::GROUP_SAME_CHANNEL:
          for (QList< QSet<Track*> >::iterator group=track_groups.begin(); group!=track_groups.end(); group++)
            if ( ((MidiTrack*)*group->begin())->outChannel() == ((MidiTrack*)*track)->outChannel()  &&
                  ((MidiTrack*)*group->begin())->outPort() == ((MidiTrack*)*track)->outPort()  &&
                  (drummaps_almost_equal(((MidiTrack*)*group->begin())->drummap(), ((MidiTrack*)*track)->drummap())) )
            {
              group->insert(*track);
              inserted=true;
              break;
            }
          break;
        
        case DrumEdit::GROUP_MAX:
          for (QList< QSet<Track*> >::iterator group=track_groups.begin(); group!=track_groups.end(); group++)
            if (drummaps_almost_equal(((MidiTrack*)*group->begin())->drummap(), ((MidiTrack*)*track)->drummap()))
            {
              group->insert(*track);
              inserted=true;
              break;
            }
          break;
        
        case DrumEdit::DONT_GROUP: 
          inserted=false;
          break;
        
        default:
          printf("THIS SHOULD NEVER HAPPEN: group_mode() is invalid!\n");
          inserted=false;
      }

      if (!inserted)
      {
        QSet<Track*> temp;
        temp.insert(*track);
        track_groups.push_back(temp);
      }
    }
  }

  // from now, we assume that every track_group's entry only groups tracks with identical
  // drum maps, but not necessarily identical hide-lists together.
  QList< std::pair<MidiTrack*,int> > ignore_order_entries;
  for (global_drum_ordering_t::iterator order_it=global_drum_ordering.begin(); order_it!=global_drum_ordering.end(); order_it++)
  {
    // if this entry should be ignored, ignore it.
    if (ignore_order_entries.contains(*order_it))
      continue;
    
    // look if we have order_it->first (the MidiTrack*) in any of our track groups
    QList< QSet<Track*> >::iterator group;
    for (group=track_groups.begin(); group!=track_groups.end(); group++)
      if (group->contains(order_it->first))
        break;
    
    if (group!=track_groups.end()) // we have
    {
      int pitch=order_it->second;
      
      bool mute=true;
      bool hidden=true;
      
      if (drumEditor->ignore_hide()) hidden=false;
      
      for (QSet<Track*>::iterator track=group->begin(); track!=group->end() && (mute || hidden); track++)
      {
        if (dynamic_cast<MidiTrack*>(*track)->drummap()[pitch].mute == false)
          mute=false;
        if (dynamic_cast<MidiTrack*>(*track)->drummap()[pitch].hide == false)
          hidden=false;
      }

      if (!hidden)
      {
        for (QSet<Track*>::iterator track=group->begin(); track!=group->end(); track++)
        {
          DrumMap* dm = &dynamic_cast<MidiTrack*>(*track)->drummap()[pitch];
          if (dm->mute != mute)
          {
            dm->mute=mute; 
            need_update = true;
          }
        }
        instrument_map.append(instrument_number_mapping_t(*group, pitch));
      }
      
      for (QSet<Track*>::iterator track=group->begin(); track!=group->end(); track++)
        ignore_order_entries.append(std::pair<MidiTrack*,int>(dynamic_cast<MidiTrack*>(*track), pitch));
    }
    // else ignore it
  }


  // maybe delete and then populate ourDrumMap
  
  if (must_delete_our_drum_map && ourDrumMap!=nullptr)
    delete [] ourDrumMap;
  
  int size = instrument_map.size();
  ourDrumMap=new DrumMap[size];
  must_delete_our_drum_map=true;

  Track* t;
  MidiTrack* mt;
  int index;
  for (int i=0;i<size;i++)
  {
    t = *instrument_map[i].tracks.begin();
    if(!t->isMidiTrack())
      continue;
    mt = static_cast<MidiTrack*>(t);
    index = instrument_map[i].pitch;
    ourDrumMap[i] = mt->drummap()[index];
  }
  
  if (instrument_map!=old_instrument_map)
  {
    if (debugMsg) printf("rebuilt drummap and instrument map, size is now %i\n",size);
  
    songChanged(SC_EVENT_INSERTED); // force an update of the itemlist
    emit ourDrumMapChanged(true);
  }
  else
    emit ourDrumMapChanged(false);
  
  if (need_update)
    MusEGlobal::song->update(SC_DRUMMAP, true); // i know, this causes a recursion, which possibly
                                                // isn't the most elegant solution here. but it will
                                                // never be an infinite recursion
}

void DrumCanvas::mouseMove(QMouseEvent* event) {

    EventCanvas::mouseMove(event);

    if (MusEGlobal::config.showNoteTooltips)
        showNoteTooltip(event);

    if (MusEGlobal::config.showStatusBar)
        showStatusTip(event);
}

void DrumCanvas::showNoteTooltip(QMouseEvent* event) {

    static CItem* hoverItem = nullptr;

    if (_tool & (MusEGui::PointerTool | MusEGui::PencilTool | MusEGui::RubberTool | MusEGui::CursorTool)) {

        QString str;
        CItem* item = findCurrentItem(event->pos());
        if (item && hoverItem == item)
            return;

        int pitch = drumEditor->get_instrument_map()[y2pitch(event->pos().y())].pitch;
        if (track()->drummap()[pitch].name.isEmpty())
            str = MusECore::pitch2string(pitch) + " (" + QString::number(pitch) + ")";
        else
            str = track()->drummap()[pitch].name + " (" + MusECore::pitch2string(pitch)
                    + "/" + QString::number(pitch) + ")";

        if (item) {
            hoverItem = item;

            MusECore::Pos start(item->event().tick() + item->part()->tick());

            int bar, beat, tick, hour, min, sec, msec;

            start.mbt(&bar, &beat, &tick);
            QString str_bar = QString("%1.%2.%3")
                    .arg(bar + 1,  4, 10, QLatin1Char('0'))
                    .arg(beat + 1, 2, 10, QLatin1Char('0'))
                    .arg(tick,     3, 10, QLatin1Char('0'));

            start.msmu(&hour, &min, &sec, &msec, nullptr);
            QString str_time = QString("%1:%2:%3.%4")
                    .arg(hour,  2, 10, QLatin1Char('0'))
                    .arg(min,   2, 10, QLatin1Char('0'))
                    .arg(sec,   2, 10, QLatin1Char('0'))
                    .arg(msec,  3, 10, QLatin1Char('0'));

            str = tr("Note: ") + str + "\n"
                    + tr("Velocity: ") + QString::number(item->event().velo()) + "\n"
                    + tr("Start (bar): ") +  str_bar + "\n"
                    + tr("Start (time): ") + str_time;

        } else {
            hoverItem = nullptr;
        }

        QToolTip::showText(QPoint(event->globalX(), event->globalY() + 10), str);
    }
}

void DrumCanvas::showStatusTip(QMouseEvent* event) {

    static CItem* hoverItem = nullptr;
    static Tool localTool;

    CItem* item = findCurrentItem(event->pos());
    if (item) {
        if (hoverItem == item && localTool == _tool)
            return;

        hoverItem = item;
        localTool = _tool;

        QString s;
        if (_tool & (MusEGui::PointerTool ))
            s = tr("LMB: Select/Move | CTRL+LMB: Multi select/Move&copy | SHIFT+LMB: Select pitch | MMB: Delete");
        else if (_tool & (MusEGui::PencilTool))
            s = tr("LMB: Select | CTRL+LMB: Multi select | SHIFT+LMB: Select pitch | CTRL+SHIFT+LMB: Multi pitch select | MMB: Delete");
        else if (_tool & (MusEGui::RubberTool))
            s = tr("LMB: Delete");
        else if (_tool & (MusEGui::CursorTool))
            s = tr("Arrow keys to move cursor, V,B,N,M keys to create events with increasing velocity, Del to delete.");

        if (!s.isEmpty())
            MusEGlobal::muse->setStatusBarText(s);
    } else {
        if (hoverItem != nullptr) {
            MusEGlobal::muse->clearStatusBarText();
            hoverItem = nullptr;
        }
    }
}

void DrumCanvas::setCursor()
{
    // Avoid duplication, just do it below.
    //showCursor();

    switch (drag) {

    case DRAGX_MOVE:
    case DRAGX_COPY:
    case DRAGX_CLONE:
        // Make sure to do this.
        showCursor();
        QWidget::setCursor(*pencilMoveHorizCursor);
        break;

    case DRAGY_MOVE:
    case DRAGY_COPY:
    case DRAGY_CLONE:
        // Make sure to do this.
        showCursor();
        QWidget::setCursor(*pencilMoveVertCursor);
        break;

    case DRAG_MOVE:
    case DRAG_COPY:
    case DRAG_CLONE:
        // Make sure to do this.
        showCursor();
        QWidget::setCursor(*pencilMove4WayCursor);
        break;

    case DRAG_RESIZE:
        // Make sure to do this.
        showCursor();
        QWidget::setCursor(*pencilMoveHorizCursor);
        break;

    default:
        // Let the Canvas handle it, and call showCursor().
        Canvas::setCursor();
        break;
    }
}

//---------------------------------------------------------
//   setMouseOverItemCursor
//---------------------------------------------------------

void DrumCanvas::setMouseOverItemCursor()
{
  //showCursor();
  QWidget::setCursor(*pencilMove4WayCursor);
}


} // namespace MusEGui
