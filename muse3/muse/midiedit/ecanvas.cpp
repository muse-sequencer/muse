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
#include <QSet>

#include "xml.h"
#include "midieditor.h"
#include "ecanvas.h"
#include "song.h"
#include "event.h"
#include "shortcuts.h"
#include "audio.h"
#include "functions.h"
#include "midi.h"
#include "gconfig.h"

namespace MusEGui {


//---------------------------------------------------------
//   EventCanvas
//---------------------------------------------------------

EventCanvas::EventCanvas(MidiEditor* pr, QWidget* parent, int sx,
   int sy, const char* name)
   : Canvas(parent, sx, sy, name)
      {
      editor      = pr;
      _steprec    = false;
      _midiin     = false;
      _playEvents = true;
      _setCurPartIfOnlyOneEventIsSelected = true;
      curVelo     = 70;

      setBg(MusEGlobal::config.midiCanvasBg);
      setAcceptDrops(true);
      setFocusPolicy(Qt::StrongFocus);
      setMouseTracking(true);

      curPart   = (MusECore::MidiPart*)(editor->parts()->begin()->second);
      curPartId = curPart->sn();
      }

EventCanvas::~EventCanvas()
{
  if(_playEvents)
    stopPlayEvent();
}
      
//---------------------------------------------------------
//   getCaption
//---------------------------------------------------------

QString EventCanvas::getCaption() const
      {
      int bar1, bar2, xx;
      unsigned x;
      AL::sigmap.tickValues(curPart->tick(), &bar1, &xx, &x);
      AL::sigmap.tickValues(curPart->tick() + curPart->lenTick(), &bar2, &xx, &x);

      return QString("MusE: Part <") + curPart->name()
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

// REMOVE Tim. citem. Added.
//---------------------------------------------------------
//   updateItems
//---------------------------------------------------------

void EventCanvas::updateItems()
{
  bool curItemNeedsRestore=false;
  MusECore::Event storedEvent;
  int partSn = 0xDEADBEEF; // to prevent compiler warning; partSn is unused anyway if curItemNeedsRestore==false.
  if (curItem)
  {
    curItemNeedsRestore=true;
    storedEvent=curItem->event();
    partSn=curItem->part()->sn();
  }
  curItem=NULL;
  
  items.clearDelete();
  start_tick  = INT_MAX;
  end_tick    = 0;
  curPart = 0;
  for (MusECore::iPart p = editor->parts()->begin(); p != editor->parts()->end(); ++p) {
        MusECore::MidiPart* part = (MusECore::MidiPart*)(p->second);
        if (part->sn() == curPartId)
              curPart = part;
        unsigned stick = part->tick();
        unsigned len = part->lenTick();
        unsigned etick = stick + len;
        if (stick < start_tick)
              start_tick = stick;
        if (etick > end_tick)
              end_tick = etick;

        for (MusECore::ciEvent i = part->events().begin(); i != part->events().end(); ++i) {
              MusECore::Event e = i->second;
              // Do not add events which are past the end of the part.
              if(e.tick() > len)      
                break;
              
              if (e.isNote()) {
                    CItem* temp = addItem(part, e);
                    
                    // REMOVE Tim. citem. Added.
                    if(temp)
                      temp->setSelected(e.selected());
                    
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

// REMOVE Tim. citem. Added.
// //---------------------------------------------------------
// //   updateItemSelections
// //---------------------------------------------------------
// 
// void EventCanvas::updateItemSelections()
//       {
//       bool item_selected;
//       bool part_selected;
//       for (iCItem i = items.begin(); i != items.end(); ++i) {
// //             NPart* npart = static_cast<NPart*>(i->second);
//             CItem* item = i->second;
// //             item_selected = i->second->isSelected();
// //             part_selected = npart->part()->selected();
//             item_selected = item->isSelected();
//             part_selected = item->objectIsSelected();
// //             if (item_selected != part_selected)
//             if (item_selected != part_selected)
//             {
//               // REMOVE Tim. citem. Added. Shouldn't be required.
//               // If the track is not visible, deselect all parts just to keep things tidy.
//               //if(!npart->part()->track()->isVisible())
//               //{
//               //  i->second->setSelected(false);
//               //  continue;
//               //}
//               i->second->setSelected(part_selected);
//             }
//       }
//       redraw();
// }

//---------------------------------------------------------
//   itemSelectionsChanged
//---------------------------------------------------------

// REMOVE Tim. citem. Changed.
// void EventCanvas::itemSelectionsChanged()
//       {
//       MusEGlobal::song->update(SC_SELECTION);
//       }
bool EventCanvas::itemSelectionsChanged(MusECore::Undo* operations, bool deselectAll)
{
      MusECore::Undo ops;
      MusECore::Undo* opsp = operations ? operations : &ops;
      
//       MusECore::Undo operations;
      bool item_selected;
      bool obj_selected;
      bool changed=false;
      
      //  and don't bother individually deselecting objects, below.
      if(deselectAll)
      {
        //opsp->push_back(MusECore::UndoOp(MusECore::UndoOp::GlobalSelectAllEvents, false, 0, 0, false));
        opsp->push_back(MusECore::UndoOp(MusECore::UndoOp::GlobalSelectAllEvents, false, 0, 0));
        changed = true;
      }
      
      
      for (iCItem i = items.begin(); i != items.end(); ++i) {
//             NPart* npart = (NPart*)(i->second);
            CItem* item = i->second;
//             operations.push_back(UndoOp(UndoOp::SelectPart, part->part(), i->second->isSelected(), part->part()->selected()));
            item_selected = item->isSelected();
            obj_selected = item->objectIsSelected();
//             if (i->second->isSelected() != item->part()->selected())
//             if (item->isSelected() != item->objectIsSelected())
            if (item_selected != obj_selected)
            {
                // REMOVE Tim. citem. Added.
                // Here we have a choice of whether to allow undoing of selections.
                // Disabled for now, it's too tedious in use. Possibly make the choice user settable.
              
              // Don't bother deselecting objects if we have already deselected all, above.
              if(item_selected || !deselectAll)
              {
                opsp->push_back(MusECore::UndoOp(MusECore::UndoOp::SelectEvent,
// REMOVE Tim. citem. Changed.
//                  item->part(), i->second->isSelected(), item->part()->selected()));
//                     item->event(), item->part(), item_selected, obj_selected, false));
                    item->event(), item->part(), item_selected, obj_selected));

                changed=true;
              }
            }
      }

      if (!operations && changed)
      {
//             MusEGlobal::song->applyOperationGroup(operations);

            // Set the 'sender' to this so that we can ignore self-generated songChanged signals.
            // Here we have a choice of whether to allow undoing of selections.
            if(MusEGlobal::config.selectionsUndoable)
              MusEGlobal::song->applyOperationGroup(ops, MusECore::Song::OperationUndoMode, this);
            else
              MusEGlobal::song->applyOperationGroup(ops, MusECore::Song::OperationExecuteUpdate, this);
            
            //{
              // REMOVE Tim. citem. Added.
              fprintf(stderr, "EventCanvas::updateSelection: Applied SelectPart operations, redrawing\n");
                
              redraw();
            //}
      }

// REMOVE Tim. citem. Removed. Unused.
//       // TODO FIXME: this must be emitted always, because CItem is broken by design:
//       //             CItems hold an Event smart-pointer which allows write access.
//       //             This means, that items can (and will!) be selected bypassing the
//       //             UndoOp::SelectEvent message! FIX THAT! (flo93)
//       emit selectionChanged();

      return changed;
}


bool EventCanvas::stuckNoteExists(int port, int channel, int pitch) const
{
  const int sz = _stuckNotes.size();
  for(int i = 0; i < sz; ++i)
  {
    MusECore::MidiPlayEvent s_ev(_stuckNotes.at(i));
    if(s_ev.type() == MusECore::ME_NOTEON &&
       port == s_ev.port() &&
       channel == s_ev.channel() &&
       pitch == s_ev.dataA())
      return true;
  }
  return false;
}

//---------------------------------------------------------
//   songChanged(type)
//---------------------------------------------------------

void EventCanvas::songChanged(MusECore::SongChangedStruct_t flags)
      {
      if (flags._flags & ~(SC_SELECTION | SC_PART_SELECTION | SC_TRACK_SELECTION)) {
            // TODO FIXME: don't we actually only want SC_PART_*, and maybe SC_TRACK_DELETED?
            //             (same in waveview.cpp)
// REMOVE Tim. citem. Changed.
//             bool curItemNeedsRestore=false;
//             MusECore::Event storedEvent;
//             int partSn = 0xDEADBEEF; // to prevent compiler warning; partSn is unused anyway if curItemNeedsRestore==false.
//             if (curItem)
//             {
//               curItemNeedsRestore=true;
//               storedEvent=curItem->event();
//               partSn=curItem->part()->sn();
//             }
//             curItem=NULL;
//             
//             items.clearDelete();
//             start_tick  = INT_MAX;
//             end_tick    = 0;
//             curPart = 0;
//             for (MusECore::iPart p = editor->parts()->begin(); p != editor->parts()->end(); ++p) {
//                   MusECore::MidiPart* part = (MusECore::MidiPart*)(p->second);
//                   if (part->sn() == curPartId)
//                         curPart = part;
//                   unsigned stick = part->tick();
//                   unsigned len = part->lenTick();
//                   unsigned etick = stick + len;
//                   if (stick < start_tick)
//                         start_tick = stick;
//                   if (etick > end_tick)
//                         end_tick = etick;
// 
//                   for (MusECore::ciEvent i = part->events().begin(); i != part->events().end(); ++i) {
//                         MusECore::Event e = i->second;
//                         // Do not add events which are past the end of the part.
//                         if(e.tick() > len)      
//                           break;
//                         
//                         if (e.isNote()) {
//                               CItem* temp = addItem(part, e);
//                               
//                               if (temp && curItemNeedsRestore && e==storedEvent && part->sn()==partSn)
//                               {
//                                   if (curItem!=NULL)
//                                     printf("THIS SHOULD NEVER HAPPEN: curItemNeedsRestore=true, event fits, but there was already a fitting event!?\n");
//                                   
//                                   curItem=temp;
//                                   }
//                               }
//                         }
//                   }
            updateItems();
            }

      MusECore::Event event;
      MusECore::MidiPart* part   = 0;
      int x            = 0;
      CItem*   nevent  = 0;

      int n  = 0;       // count selections
      for (iCItem k = items.begin(); k != items.end(); ++k) {
            MusECore::Event ev = k->second->event();
            
            if (ev.selected()) {
                  ++n;
                  if (!nevent) {
                        nevent   =  k->second;
                        curVelo = ev.velo();
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
            if (_setCurPartIfOnlyOneEventIsSelected && n == 1 && curPart != part) {
                  curPart = part;
                  curPartId = curPart->sn();
                  curPartChanged();
                  }
      }

      // REMOVE Tim. citem. Added.
      if(flags._flags & (SC_SELECTION))
      {
        // Prevent race condition: Ignore if the change was ultimately sent by the canvas itself.
        if(flags._sender != this)
          updateItemSelections();
      }
      
      bool f1 = flags._flags & (SC_EVENT_INSERTED | SC_EVENT_MODIFIED | SC_EVENT_REMOVED | 
                         SC_PART_INSERTED | SC_PART_MODIFIED | SC_PART_REMOVED |
                         SC_TRACK_INSERTED | SC_TRACK_REMOVED | SC_TRACK_MODIFIED |
                         SC_SIG | SC_TEMPO | SC_KEY | SC_MASTER | SC_CONFIG | SC_DRUMMAP); 
      bool f2 = flags._flags & SC_SELECTION;
      
      // Try to avoid all unnecessary emissions.
      if(f1 || f2)
        emit selectionChanged(x, event, part, !f1);
      
      if (curPart == 0)
            curPart = (MusECore::MidiPart*)(editor->parts()->begin()->second);
      redraw();
      }

//---------------------------------------------------------
//   selectAtTick
//---------------------------------------------------------
void EventCanvas::selectAtTick(unsigned int tick)
{
   //Select note nearest tick, if none selected and there are any
   if (!items.empty() && selectionSize() == 0) {
      iCItem i = items.begin();
      CItem* nearest = i->second;

      while (i != items.end()) {
         CItem* cur=i->second;
         unsigned int curtk=abs(cur->x() + (int)cur->part()->tick() - (int)tick);
         unsigned int neartk=abs(nearest->x() + (int)nearest->part()->tick() - (int)tick);

         if (curtk < neartk) {
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
//   track
//---------------------------------------------------------

MusECore::MidiTrack* EventCanvas::track() const
      {
      if(!curPart)
        return 0;
      return ((MusECore::MidiPart*)curPart)->track();
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
                  MusECore::Pos p1(tick_min, true);
                  MusECore::Pos p2(tick_max, true);
                  MusEGlobal::song->setPos(1, p1);
                  MusEGlobal::song->setPos(2, p2);
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
// REMOVE Tim. citem. Changed.
//                 itemSelectionsChanged();
                redraw();
                
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
// REMOVE Tim. citem. Changed.
//                 itemSelectionsChanged();
                redraw();
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


// REMOVE Tim. citem. Added.
//---------------------------------------------------------
//   keyRelease
//---------------------------------------------------------

void EventCanvas::keyRelease(QKeyEvent* event)
{
      const int key = event->key();
      
      // We do not want auto-repeat events.
      // It does press and release repeatedly. Wait till the last release comes.
      if(!event->isAutoRepeat())
      {
        // REMOVE Tim. citem. Added.
        fprintf(stderr, "EventCanvas::keyRelease not isAutoRepeat\n");
      
        //event->accept();
      
        // Select part to the right
        if(key == shortcuts[SHRT_SEL_RIGHT].key || key == shortcuts[SHRT_SEL_RIGHT_ADD].key ||
        // Select part to the left
          key == shortcuts[SHRT_SEL_LEFT].key || key == shortcuts[SHRT_SEL_LEFT_ADD].key)
        {
            itemSelectionsChanged();
        }
        return;
      }
      
  Canvas::keyRelease(event);
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
            paste_at(text,x,3072,false,false,curPart);
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

void EventCanvas::endMoveItems(const QPoint& pos, DragType dragtype, int dir, bool rasterize)
      {
      int dp = y2pitch(pos.y()) - y2pitch(Canvas::start.y());
      int dx = pos.x() - Canvas::start.x();

      if (dir == 1)
            dp = 0;
      else if (dir == 2)
            dx = 0;
      
      
      
      MusECore::Undo operations = moveCanvasItems(moving, dp, dx, dragtype, rasterize);
      if (operations.empty())
        songChanged(SC_EVENT_MODIFIED); //this is a hack to force the canvas to repopulate
      	                                //itself. otherwise, if a moving operation was forbidden,
      	                                //the canvas would still show the movement
      else
        MusEGlobal::song->applyOperationGroup(operations);
      
      moving.clear();
      itemSelectionsChanged();
      redraw();
      }

// REMOVE Tim. citem. Removed. Let Canvas::deselectAll() do it.
// //---------------------------------------------------------
// //   deselectAll
// //---------------------------------------------------------
// 
// void EventCanvas::deselectAll()
// {
//   QSet<MusECore::Part*> already_done;
//   MusECore::Part* p;
//   for(iCItem i = items.begin(); i != items.end(); ++i)
//   {
//     p = i->second->part();
//     if(already_done.contains(p) || !p)
//       continue;
//     MusEGlobal::song->selectAllEvents(p, false);
//     already_done.insert(p);
//   }
// }

//---------------------------------------------------------
//   startPlayEvent
//---------------------------------------------------------

void EventCanvas::startPlayEvent(int note, int velocity, int port, int channel)
      {
      // REMOVE Tim. Noteoff. Added. Zero note on vel is not allowed now.
      if(velocity == 0)
      {
        fprintf(stderr, "EventCanvas::startPlayEvent: Warning: Zero note on velocity!\n");
        velocity = 1;
      }
      
      if (MusEGlobal::debugMsg)
        printf("EventCanvas::startPlayEvent %d %d %d %d\n", note, velocity, port, channel);

      // Release any current note.
      stopPlayEvent();
      
      if(!track())
        return;
      
      int playedPitch        = note;
      // Apply track transposition, but only for midi tracks, not drum tracks.
      if(track()->isMidiTrack() && !track()->isDrumTrack())
        playedPitch += track()->transposition;
      
      // play note:
      if(stuckNoteExists(port, channel, playedPitch))
        return;
      const MusECore::MidiPlayEvent e(MusEGlobal::audio->curFrame(), port, channel, MusECore::ME_NOTEON, playedPitch, velocity);
      _stuckNotes.push_back(e);
      // Send to the port and device.
      MusEGlobal::midiPorts[port].putEvent(e);
      }

void EventCanvas::startPlayEvent(int note, int velocity)
      {
      if(!track())
        return;
      int port         = track()->outPort();
      int channel      = track()->outChannel();
      startPlayEvent(note, velocity, port, channel);
}

//---------------------------------------------------------
//   stopPlayEvent
//---------------------------------------------------------

void EventCanvas::stopPlayEvent()
      {
      // Stop all currently playing notes.
      unsigned int frame = MusEGlobal::audio->curFrame();
      int port;
      const int sz = _stuckNotes.size();
      for(int i = 0; i < sz; ++i)
      {
        MusECore::MidiPlayEvent ev(_stuckNotes.at(i));
        port = ev.port();
        if(port < 0 || port >= MIDI_PORTS)
          continue;
        ev.setType(MusECore::ME_NOTEOFF);
        ev.setTime(frame);
        if(ev.dataB() == 0)
          ev.setB(64);
        
        // Send to the port and device.
        MusEGlobal::midiPorts[port].putEvent(ev);
      }
      // Clear the stuck notes list.
      _stuckNotes.clear();
      }

} // namespace MusEGui
