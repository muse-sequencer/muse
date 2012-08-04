//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: ecanvas.cpp,v 1.8.2.6 2009/05/03 04:14:00 terminator356 Exp $
//  (C) Copyright 2001 Werner Schweer (ws@seh.de)
//  (C) Copyright 2011 Tim E. Real (terminator356 on sourceforge)
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
#include <errno.h>
#include <limits.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/mman.h>

#include <QKeyEvent>
#include <QDropEvent>
#include <QEvent>
#include <QMimeData>
#include <QByteArray>
#include <QDrag>

#include "xml.h"
#include "midieditor.h"
#include "ecanvas.h"
#include "song.h"
#include "event.h"
#include "shortcuts.h"
#include "audio.h"
#include "functions.h"
#include "pcanvas.h"

// Item drawing layer indices:
#define _NONCUR_PART_UNSEL_EVENT_LAYER_  0
#define _NONCUR_PART_SEL_EVENT_LAYER_    1
#define _UNSELECTED_EVENT_LAYER_         2
#define _SELECTED_EVENT_LAYER_           3

namespace MusEGui {

//---------------------------------------------------------
//   EventCanvas
//---------------------------------------------------------

EventCanvas::EventCanvas(MidiEditor* pr, QWidget* parent, int sx,
   int sy, const char* name)
   : Canvas(parent, sx, sy, name)
      {
      _curPart    = NULL;
      _curPartId  = -1;
      editor      = pr;
      _steprec    = false;
      _midiin     = false;
      _playEvents = false;
      _setCurPartIfOnlyOneEventIsSelected = true;
      curVelo     = 70;

      // Create one item layer for notes.
      items.push_back(CItemList());

      // Create four item drawing layers. One for unselected events from non-current parts,
      //  one for selected events from non-current parts, one for unselected events from current part,
      //  one for selected events from current part.
      itemLayers.push_back(std::vector<CItem*>() );
      itemLayers.push_back(std::vector<CItem*>() );
      itemLayers.push_back(std::vector<CItem*>() );
      itemLayers.push_back(std::vector<CItem*>() );
      
      setBg(Qt::white);
      setAcceptDrops(true);
      setFocusPolicy(Qt::StrongFocus);
      setMouseTracking(true);

      _curPart   = (MusECore::MidiPart*)(editor->parts()->begin()->second);
      _curPartId = _curPart->sn();
      }

//---------------------------------------------------------
//   getCaption
//---------------------------------------------------------

QString EventCanvas::getCaption() const
      {
      int bar1, bar2, xx;
      unsigned x;
      AL::sigmap.tickValues(_curPart->tick(), &bar1, &xx, &x);
      AL::sigmap.tickValues(_curPart->tick() + _curPart->lenTick(), &bar2, &xx, &x);

      return QString("MusE: Part <") + _curPart->name()
         + QString("> %1-%2").arg(bar1+1).arg(bar2+1);
      }

//---------------------------------------------------------
//   leaveEvent
//---------------------------------------------------------

void EventCanvas::leaveEvent(QEvent*)
      {
      emit pitchChanged(-1);
      emit timeChanged(INT_MAX);
      }

//---------------------------------------------------------
//   enterEvent
//---------------------------------------------------------

void EventCanvas::enterEvent(QEvent*)
      {
      emit enterCanvas();
      }

//---------------------------------------------------------
//   raster
//---------------------------------------------------------

QPoint EventCanvas::raster(const QPoint& p) const
      {
      int x = p.x();
      if (x < 0)
            x = 0;
      x = editor->rasterVal(x);
      int pitch = y2pitch(p.y());
      int y = pitch2y(pitch);
      return QPoint(x, y);
      }

//---------------------------------------------------------
//   mouseMove
//---------------------------------------------------------

void EventCanvas::mouseMove(QMouseEvent* event)
      {
      emit pitchChanged(y2pitch(event->pos().y()));
      int x = event->pos().x();
      emit timeChanged(editor->rasterVal(x));
      }

//---------------------------------------------------------
//   updateSelection
//---------------------------------------------------------

void EventCanvas::updateSelection()
      {
      MusEGlobal::song->update(SC_SELECTION);
      }

//---------------------------------------------------------
//   songChanged(type)
//---------------------------------------------------------

void EventCanvas::songChanged(int flags)
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

// REMOVE Tim. TEST: Flo's original addition:
//             if (curItem)
//             {
//               curItemNeedsRestore=true;
//               storedEvent=curItem->event();
//               partSn=curItem->part()->sn();
//             }
//             curItem=NULL;
//            
            // Only handle midi items for now. p4.1.0 
            if (curItem && (curItem->type() == CItem::MEVENT || curItem->type() == CItem::DEVENT))
            {
              curItemNeedsRestore=true;
              MCItem* mci = static_cast<MCItem*>(curItem);
              storedEvent = mci->event();
              partSn      = mci->part()->sn();
              curItem=NULL;
            }
            
            items[_ECANVAS_EVENT_ITEMS_].clearDelete();
            start_tick  = INT_MAX;
            end_tick    = 0;
            _curPart = 0;
            for (MusECore::iPart p = editor->parts()->begin(); p != editor->parts()->end(); ++p) {
                  MusECore::MidiPart* part = (MusECore::MidiPart*)(p->second);
                  if (part->sn() == _curPartId)
                        _curPart = part;
                  unsigned stick = part->tick();
                  unsigned len = part->lenTick();
                  unsigned etick = stick + len;
                  if (stick < start_tick)
                        start_tick = stick;
                  if (etick > end_tick)
                        end_tick = etick;

                  MusECore::EventList* el = part->events();
                  for (MusECore::iEvent i = el->begin(); i != el->end(); ++i) {
                        MusECore::Event e = i->second;
                        // Do not add events which are past the end of the part.
                        if(e.tick() > len)      
                          break;
                        
                        if (e.isNote()) {
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
      MusECore::MidiPart* part   = 0;
      int x            = 0;
      MCItem*   nevent = 0;

      int n  = 0;       // count selections
      for (iCItem k = items[_ECANVAS_EVENT_ITEMS_].begin(); k != items[_ECANVAS_EVENT_ITEMS_].end(); ++k) {
            // p4.1.0 Only handle notes for now. 
	    if(k->second->type() != CItem::MEVENT && k->second->type() != CItem::DEVENT)
              continue;
            MCItem* mcitem = (MCItem*)k->second;
            MusECore::Event ev = mcitem->event();
            bool selected = ev.selected();
            if (selected) {
                  mcitem->setSelected(true);
                  ++n;
                  if (!nevent) {
                        nevent   =  mcitem;
                        MusECore::Event mi = nevent->event();
                        curVelo  = mi.velo();
                        }
                  }
            }
      start_tick = MusEGlobal::song->roundDownBar(start_tick);
      end_tick   = MusEGlobal::song->roundUpBar(end_tick);

      if (n >= 1)    
      {
            x     = nevent->x();
            event = nevent->event();
            part  = (MusECore::MidiPart*)nevent->part();
            if (_setCurPartIfOnlyOneEventIsSelected && n == 1 && _curPart != part) {
                  _curPart = part;
                  _curPartId = _curPart->sn();
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
      
      if (_curPart == 0)
            _curPart = (MusECore::MidiPart*)(editor->parts()->begin()->second);
      redraw();
      }

//---------------------------------------------------------
//   selectAtTick
//---------------------------------------------------------

void EventCanvas::selectAtTick(unsigned int tick)
      {
      //Select note nearest tick, if none selected and there are any
      if (!items[_ECANVAS_EVENT_ITEMS_].empty() && selectionSize(_ECANVAS_EVENT_ITEMS_) == 0) {
            iCItem i = items[_ECANVAS_EVENT_ITEMS_].begin();
            //CItem* nearest = i->second;

            // p4.1.0 Only handle notes for now. 
            while (i != items[_ECANVAS_EVENT_ITEMS_].end()) {
                if(i->second->type() == CItem::MEVENT && i->second->type() == CItem::DEVENT)
                  break;
		++i;
            }
            if(i == items[_ECANVAS_EVENT_ITEMS_].end())
              return;      
            
            MCItem* nearest = (MCItem*)i->second;
            
            while (i != items[_ECANVAS_EVENT_ITEMS_].end()) {
                // p4.1.0 Only handle notes for now. 
                if(i->second->type() != CItem::MEVENT && i->second->type() != CItem::DEVENT)
                  continue;
                //CItem* cur=i->second;                
                MCItem* cur = (MCItem*)i->second;
                unsigned int curtk=abs(cur->x() + cur->part()->tick() - tick);
                unsigned int neartk=abs(nearest->x() + nearest->part()->tick() - tick);

                if (curtk < neartk) {
                    nearest=cur;
                    }

                ++i;
                }

            if (!nearest->isSelected()) {
                  selectItem(nearest, true);
                  songChanged(SC_SELECTION);
                  }
            }
      }

//---------------------------------------------------------
//   track
//---------------------------------------------------------

MusECore::MidiTrack* EventCanvas::track() const
      {
      return ((MusECore::MidiPart*)_curPart)->track();
      }


//---------------------------------------------------------
//   keyPress
//---------------------------------------------------------

void EventCanvas::keyPress(QKeyEvent* event)
      {
      int key = event->key();
      if (((QInputEvent*)event)->modifiers() & Qt::ShiftModifier)
            key += Qt::SHIFT;
      if (((QInputEvent*)event)->modifiers() & Qt::AltModifier)
            key += Qt::ALT;
      if (((QInputEvent*)event)->modifiers() & Qt::ControlModifier)
            key+= Qt::CTRL;

      //
      //  Shortcut for DrumEditor & PianoRoll
      //  Sets locators to selected events
      //
      if (key == shortcuts[SHRT_LOCATORS_TO_SELECTION].key) {
            int tick_max = 0;
            int tick_min = INT_MAX;
            bool found = false;

            for (iCItem i= items[_ECANVAS_EVENT_ITEMS_].begin(); i != items[_ECANVAS_EVENT_ITEMS_].end(); i++) {
                  // p4.1.0 Only handle notes for now. 
                  if(i->second->type() != CItem::MEVENT && i->second->type() != CItem::DEVENT)
                    continue;

		  if (!i->second->isSelected())
                        continue;

                  MCItem* mcitem = (MCItem*)i->second;
                
                  //int tick = i->second->x();
                  //int len = i->second->event().lenTick();
                  int tick = mcitem->x();
                  int len = mcitem->event().lenTick();

                  found = true;
                  if (tick + len > tick_max)
                        tick_max = tick + len;
                  if (tick < tick_min)
                        tick_min = tick;
                  }
            if (found) {
                  MusECore::Pos p1(tick_min, true);
                  MusECore::Pos p2(tick_max, true);
                  MusEGlobal::song->setPos(1, p1);
                  MusEGlobal::song->setPos(2, p2);
                  }
            }
      // Select items by key (PianoRoll & DrumEditor)
      else if (key == shortcuts[SHRT_SEL_RIGHT].key || key == shortcuts[SHRT_SEL_RIGHT_ADD].key) {
// <<<<<<< .working
//             iCItem i, iRightmost;
// 	    CItem* rightmost = NULL;
//             //Get the rightmost selected note (if any)
//             for (i = items[_ECANVAS_EVENT_ITEMS_].begin(); i != items[_ECANVAS_EVENT_ITEMS_].end(); ++i) {
//                   // p4.1.0 Only handle notes for now. 
//                   if(i->second->type() != CItem::MEVENT && i->second->type() != CItem::DEVENT)
//                     continue;
//                   
// 		  if (i->second->isSelected()) {
//                         iRightmost = i; rightmost = i->second;
//                         }
//                   }
//                if (rightmost) {
//                      iCItem temp = iRightmost; temp++;
//                      //If so, deselect current note and select the one to the right
//                      if (temp != items[_ECANVAS_EVENT_ITEMS_].end()) {
//                            if (key != shortcuts[SHRT_SEL_RIGHT_ADD].key)
//                                  deselectAll();
// =======
              rciCItem i;
// >>>>>>> .merge-right.r1557

// REMOVE Tim. This is the merged part. Reworked.              
//               if (items.empty())
//                   return;
//               for (i = items.rbegin(); i != items.rend(); ++i) 
//                 if (i->second->isSelected()) 
//                   break;
// 
//               if(i == items.rend())
//                 i = items.rbegin();
//               
//               if(i != items.rbegin())
//                 --i;
//               if(i->second)
//               {
//                 if (key != shortcuts[SHRT_SEL_RIGHT_ADD].key)
//                       deselectAll();
//                 CItem* sel = i->second;
//                 sel->setSelected(true);
//                 updateSelection();
//                 if (sel->x() + sel->width() > mapxDev(width())) 
//                 {  
//                   int mx = rmapx(sel->x());  
//                   int newx = mx + rmapx(sel->width()) - width();
//                   // Leave a bit of room for the specially-drawn drum notes. But good for piano too.
//                   emit horizontalScroll( (newx > mx ? mx - 10: newx + 10) - rmapx(xorg) );
//                 }  
//               }
//             }
//             
//             
              //TEST Tim: Hm, is this correct and how we want it? Esp deselectAll? Maybe deselect only notes.
              // p4.1.0 Only handle notes for now.  
              if (items[_ECANVAS_EVENT_ITEMS_].empty())
                  return;
              for (i = items[_ECANVAS_EVENT_ITEMS_].rbegin(); i != items[_ECANVAS_EVENT_ITEMS_].rend(); ++i) 
                if (i->second->isSelected()) 
                  break;

              if(i == items[_ECANVAS_EVENT_ITEMS_].rend())
                i = items[_ECANVAS_EVENT_ITEMS_].rbegin();
              
              if(i != items[_ECANVAS_EVENT_ITEMS_].rbegin())
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
// <<<<<<< .working
//             iCItem i, iLeftmost;
//             CItem* leftmost = NULL;
//             //if (items.size() > 0 ) {        // I read that this may be much slower than empty().
//             if (!items[_ECANVAS_EVENT_ITEMS_].empty() ) {       
//                   for (i = items[_ECANVAS_EVENT_ITEMS_].end(), i--; i != items[_ECANVAS_EVENT_ITEMS_].begin(); i--) {
// 			// p4.1.0 Only handle notes for now. 
// 			if(i->second->type() != CItem::MEVENT && i->second->type() != CItem::DEVENT)
// 			  continue;
//                         
// 			if (i->second->isSelected()) {
//                               iLeftmost = i; leftmost = i->second;
//                               }
//                         }
//                     if (leftmost) {
//                           if (iLeftmost != items[_ECANVAS_EVENT_ITEMS_].begin()) {
//                                 //Add item
//                                 if (key != shortcuts[SHRT_SEL_LEFT_ADD].key)
//                                       deselectAll();
//       
//                                 iLeftmost--;
//                                 iLeftmost->second->setSelected(true);
//                                 updateSelection();
//                                 }
//                           }
//                     //if (leftmost && mapx(leftmost->event().tick())< 0 ) for some reason this doesn't this doesnt move the event in view
//                     //  emit followEvent(leftmost->x());
//                   }
// =======

// REMOVE Tim. This is the merged part. Reworked.              
//               ciCItem i;
//               if (items.empty())
//                   return;
//               for (i = items.begin(); i != items.end(); ++i)
//                 if (i->second->isSelected()) 
//                   break;
// 
//               if(i == items.end())
//                 i = items.begin();
//               
//               if(i != items.begin())
//                 --i;
//               if(i->second)
//               {
//                 if (key != shortcuts[SHRT_SEL_LEFT_ADD].key)
//                       deselectAll();
//                 CItem* sel = i->second;
//                 sel->setSelected(true);
//                 updateSelection();
//                 if (sel->x() <= mapxDev(0)) 
//                   emit horizontalScroll(rmapx(sel->x() - xorg) - 10);  // Leave a bit of room.
//               }
// >>>>>>> .merge-right.r1557


              //TEST Tim: Hm, is this correct and how we want it? Esp deselectAll? Maybe deselect only notes.
              // p4.1.0 Only handle notes for now.  
              ciCItem i;
              if (items[_ECANVAS_EVENT_ITEMS_].empty())
                  return;
              for (i = items[_ECANVAS_EVENT_ITEMS_].begin(); i != items[_ECANVAS_EVENT_ITEMS_].end(); ++i)
                if (i->second->isSelected()) 
                  break;

              if(i == items[_ECANVAS_EVENT_ITEMS_].end())
                i = items[_ECANVAS_EVENT_ITEMS_].begin();
              
              if(i != items[_ECANVAS_EVENT_ITEMS_].begin())
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
      else if (key == shortcuts[SHRT_INC_PITCH].key) {
            modifySelected(NoteInfo::VAL_PITCH, 1);
            }
      else if (key == shortcuts[SHRT_DEC_PITCH].key) {
            modifySelected(NoteInfo::VAL_PITCH, -1);
            }
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
//   dropEvent
//---------------------------------------------------------

void EventCanvas::viewDropEvent(QDropEvent* event)
      {
      QString text;
      if (event->source() == this) {
            printf("local DROP\n");      
            //event->acceptProposedAction();     
            //event->ignore();                     // TODO CHECK Tim.
            return;
            }
      if (event->mimeData()->hasFormat("text/x-muse-groupedeventlists")) {
            text = QString(event->mimeData()->data("text/x-muse-groupedeventlists"));
      
            int x = editor->rasterVal(event->pos().x());
            if (x < 0)
                  x = 0;
            paste_at(text,x,3072,false,false,_curPart);
            //event->accept();  // TODO
            }
      else {
            printf("cannot decode drop\n");
            //event->acceptProposedAction();     
            //event->ignore();                     // TODO CHECK Tim.
            }
      }


//---------------------------------------------------------
//   endMoveItems
//    dir = 0     move in all directions
//          1     move only horizontal
//          2     move only vertical
//---------------------------------------------------------

void EventCanvas::endMoveItems(const QPoint& pos, DragType dragtype, int dir)
      {
      int dp = y2pitch(pos.y()) - y2pitch(Canvas::start.y());
      int dx = pos.x() - Canvas::start.x();

      if (dir == 1)
            dp = 0;
      else if (dir == 2)
            dx = 0;
      
      
      
      MusECore::Undo operations = moveCanvasItems(moving, dp, dx, dragtype);
      if (operations.empty())
        songChanged(SC_EVENT_MODIFIED); //this is a hack to force the canvas to repopulate
      	                                //itself. otherwise, if a moving operation was forbidden,
      	                                //the canvas would still show the movement
      else
        MusEGlobal::song->applyOperationGroup(operations);
      
      moving.clear();
      updateSelection();
      redraw();
      }

//---------------------------------------------------------
//   setCurrentPart
//---------------------------------------------------------

void EventCanvas::setCurrentPart(MusECore::Part* part)
{
  curItem = NULL;
  deselectAll();
  _curPart = part;
  _curPartId = _curPart->sn();
  curPartChanged();
}

//---------------------------------------------------------
//   curItemChanged
//---------------------------------------------------------

void EventCanvas::curItemChanged() 
{
  if(!curItem)
  {  
    //_curPart = NULL;
    //_curPartId = -1;
    return;
  }
  
  switch(curItem->type())
  {
    case CItem::MEVENT:
    {  
      MCItem* i = (MCItem*)curItem;
      if (i->part() != _curPart) {
            _curPart = i->part();
            _curPartId = _curPart->sn();
            curPartChanged();
            }
      // TEST: This is Flo's line, I moved it here from canvas.cpp REMOVE Tim. Or not...
      emit curPartHasChanged(_curPart);
    }
    break;

    default:
    return;  
  }      
}

//---------------------------------------------------------
//   curPartChanged
//---------------------------------------------------------

void EventCanvas::curPartChanged()
{

// REMOVE Tim. TEST Tim:
// This was an attempt to move Flo's code from canvas.cpp
//  to here. But I think it is redundant since I already
//  added curItemChanged.
  
// // REMOVE Tim.   
// //       if (curItem->part() != curPart) {
// //             curPart = curItem->part();
// //             curPartId = curPart->sn();
// //             curPartChanged();
// //             }
// //       emit curPartHasChanged(curPart);
// //       
//       
//   if(!curItem)
//   {  
//     //_curPart = NULL;
//     //_curPartId = -1;
//     return;
//   }
//   
//   switch(curItem->type())
//   {
//     case CItem::MEVENT:
//     {  
//       MCItem* i = (MCItem*)curItem;
//       if (i->part() != _curPart) {
//             _curPart = i->part();
//             _curPartId = _curPart->sn();
//             curPartChanged();
//             }
//       emit curPartHasChanged(curPart);
//     }
//     break;
// 
//     default:
//     return;  
//   }      
      
}
      
//---------------------------------------------------------
//   selectLasso
//---------------------------------------------------------

void EventCanvas::selectLasso(bool toggle)
      {
      int n = 0;
      int ilayer;
      switch(_tool)
      {
        //case AutomationTool:       // p4.1.0 TODO: Handle automation.
        //  ilayer = items[ ? ];
        //break;
        default:
          ilayer = _ECANVAS_EVENT_ITEMS_;
        break;  
      }
      
      if (virt()) {
            for (ciCItem i = items[ilayer].begin(); i != items[ilayer].end(); ++i) {
                  if (i->second->intersects(lasso)) {
                        selectItem(i->second, !(toggle && i->second->isSelected()));
                        ++n;
                        }
                  }
            }
      else {
            for (ciCItem i = items[ilayer].begin(); i != items[ilayer].end(); ++i) {
                  QRect box = i->second->bbox();
                  int x = rmapxDev(box.x());
                  int y = rmapyDev(box.y());
                  int w = rmapxDev(box.width());
                  int h = rmapyDev(box.height());
                  QRect r(x, y, w, h);
                  ///r.moveBy(i->second->pos().x(), i->second->pos().y());
                  r.translate(i->second->pos().x(), i->second->pos().y());
                  if (r.intersects(lasso)) {
                        selectItem(i->second, !(toggle && i->second->isSelected()));
                        ++n;
                        }
                  }
            }

      if (n) {
            updateSelection();
            redraw();
            }
      }

//---------------------------------------------------------
//   selectItemRow
//---------------------------------------------------------

void EventCanvas::selectItemRow(bool select)
{
  switch(_tool)
  {  
    case AutomationTool: // p4.1.0 TODO Handle automation.
      return;
      
    default:  
    {  
      for (iCItem i = items[_ECANVAS_EVENT_ITEMS_].begin(); i != items[_ECANVAS_EVENT_ITEMS_].end(); ++i)
      {  
        if (i->second->type() == curItem->type() &&  i->second->y() == curItem->y() )
              selectItem(i->second, select);
      }  
      return;  
    }    
  }      
  
  /*
  // Select all similar item types in all layers.
  for(ciCItemLayer ilayer = items.begin(); ilayer != items.end(); ++ ilayer)
  {  
    for (iCItem i = ilayer->begin(); i != ilayer->end(); ++i)
    {  
      if (i->second->type() == curItem->type() &&  i->second->y() == curItem->y() )
            selectItem(i->second, select);
    }  
  }
  */
}

//---------------------------------------------------------
//   deleteItemAtPoint
//---------------------------------------------------------

void EventCanvas::deleteItemAtPoint(const QPoint& p)
      {
      int ilayer;
      switch(_tool)
      {
        //case AutomationTool:        // p4.1.0 TODO: Handle automation.
        //  ilayer = items[ ? ];
        //break;  
        default:
          ilayer = _ECANVAS_EVENT_ITEMS_;
        break;
      }
      
      if (virt()) {
            for (ciCItem i = items[ilayer].begin(); i != items[ilayer].end(); ++i) {
                  if (i->second->contains(p)) {
                        // p4.1.0 FIXME: Should we move this below? But item might be deleted by then. 
                        // But this means items from other parts might be unselected.
                        selectItem(i->second, false);
                        if (!deleteItem(i->second)) {
                              //selectItem(i->second, false);
                              if (drag == DRAG_DELETE)
                                    drag = DRAG_OFF;
                              }
                        break;
                        }
                  }
            }
      else {
            for (ciCItem i = items[ilayer].begin(); i != items[ilayer].end(); ++i) {
                  QRect box = i->second->bbox();
                  int x = rmapxDev(box.x());
                  int y = rmapyDev(box.y());
                  int w = rmapxDev(box.width());
                  int h = rmapyDev(box.height());
                  QRect r(x, y, w, h);
                  ///r.moveBy(i->second->pos().x(), i->second->pos().y());
                  r.translate(i->second->pos().x(), i->second->pos().y());
                  if (r.contains(p)) {
                        //selectItem(i->second, false);
                        if (deleteItem(i->second)) {
                                // p4.1.0 FIXME: Want to move above. Item might be deleted by now. 
                                // But it means items from other parts might be unselected.
                                selectItem(i->second, false);  
                              }
                        break;
                        }
                  }
            }
      }

/*
//---------------------------------------------------------
//   sortLayerItem
//---------------------------------------------------------

void EventCanvas::sortLayerItem(CItem* item)
{
  switch(item->type())
  {  
    case CItem::MEVENT:
    case CItem::DEVENT:
    {  
      MCItem* mi = (MCItem*)item;
      // Draw items from other parts behind all others.
      if(!mi->event().empty() && mi->part() != _curPart)
        itemLayers.at(_NONCUR_PART_EVENT_LAYER_).push_back(mi);
      else 
      if(!mi->isMoving() && (mi->event().empty() || mi->part() == _curPart))
      {
        // Draw selected parts in front of all others.
        if(mi->isSelected()) 
          itemLayers.at(_SELECTED_EVENT_LAYER_).push_back(mi);
        else  
          itemLayers.at(_UNSELECTED_EVENT_LAYER_).push_back(mi);
      }  
    }  
    break;
    
    default:
      return;  
  }
}
*/

//---------------------------------------------------------
//   drawItemLayer
//---------------------------------------------------------

void EventCanvas::drawItemLayer(QPainter& p, const QRect& r, int layer)
{ 
  iCItem to( virt() ? items[layer].lower_bound(r.x() + r.width()) : items[layer].end());
  
  switch(layer)
  {
    case _ECANVAS_EVENT_ITEMS_:
    {
      // Clear the item drawing layers.
      int draw_layers = itemLayers.size();
      for(int i = 0; i < draw_layers; ++i)
        itemLayers[i].clear();        
      
      for(iCItem i = items[layer].begin(); i != to; ++i)
      {  
        if(i->second->type() == CItem::MEVENT || i->second->type() == CItem::DEVENT)
        {
          MCItem* mcitem = (MCItem*)i->second;
          if(!mcitem->isMoving())
          {
            if(mcitem->part() != _curPart)
            {  
              // Draw unselected events from non-current parts behind all others.
              if(mcitem->isSelected())
                itemLayers[_NONCUR_PART_SEL_EVENT_LAYER_].push_back(mcitem);
              else
                itemLayers[_NONCUR_PART_UNSEL_EVENT_LAYER_].push_back(mcitem);
            }  
            else
            {  
              // Draw selected events from current part in front of all others.
              if(mcitem->isSelected())
                itemLayers[_SELECTED_EVENT_LAYER_].push_back(mcitem);
              else
                itemLayers[_UNSELECTED_EVENT_LAYER_].push_back(mcitem);
            }  
          }
        }
      }
      
      for(int i = 0; i < draw_layers; ++i)
      {  
        int sz = itemLayers[i].size();
        for (int j = 0; j < sz; ++j)
          drawItem(p, itemLayers[i][j], r, i);
      }
    }
    break;

    //case _ECANVAS_AUTOMATION_ITEMS_:
    //{
      // TODO: Draw the items as connected lines, then redraw them as vertex 'boxes' on TOP OF the lines. 
    //}
    //break;
    
    default:
      return;
  }  
}  

} // namespace MusEGui
