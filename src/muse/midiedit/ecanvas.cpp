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
//#include <errno.h>
#include <limits.h>
//#include <sys/stat.h>
//#include <sys/types.h>
#ifndef _WIN32
#include <sys/mman.h>
#endif

#include <QByteArray>
//#include <QSet>
#include <QMimeData>

//#include "xml.h"
#include "ecanvas.h"
#include "song.h"
#include "shortcuts.h"
#include "audio.h"
#include "functions.h"
#include "midi_consts.h"
#include "gconfig.h"
#include "globals.h"
#include "midiport.h"
#include "config.h"

// Forwards from header:
//#include <QDropEvent>
#include <QEvent>
#include <QKeyEvent>
#include <QToolTip>
#include "track.h"
#include "part.h"
#include "event.h"
#include "undo.h"
#include "midieditor.h"
#include "citem.h"

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
      _playEventsMode = PlayEventsSingleNote;
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
        stopPlayEvents();
}
      
//---------------------------------------------------------
//   getCaption
//---------------------------------------------------------

QString EventCanvas::getCaption() const
{
    int bar1, bar2, xx;
    unsigned x;
    MusEGlobal::sigmap.tickValues(curPart->tick(), &bar1, &xx, &x);
    MusEGlobal::sigmap.tickValues(curPart->tick() + curPart->lenTick(), &bar2, &xx, &x);

    QString s;
    if (editor->parts()->size() > 1)
        s = curPart->name()
                + QString(" (%1-%2) [%3:%4]")
                .arg(bar1+1)
                .arg(bar2+1)
                .arg(editor->parts()->index(curPart) + 1)
                .arg(editor->parts()->size());
    else
        s = curPart->name() + QString(" (%1-%2)").arg(bar1+1).arg(bar2+1);
    return s;
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
    if(x < 0)
        x = 0;
    emit timeChanged(editor->rasterVal(x));
}

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
  curItem=nullptr;
  
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
#ifdef ALLOW_LEFT_HIDDEN_EVENTS
              if((int)e.tick() /*+ (int)e.lenTick()*/ < 0)
                continue;
              if((int)e.tick() >= (int)len)
                break;
#else
              if(e.tick() > len)   
                break;
#endif
              
              if (e.isNote()) {
                    CItem* temp = addItem(part, e);
                    
                    if(temp)
                      temp->setSelected(e.selected());
                    
                    if (temp && curItemNeedsRestore && e==storedEvent && part->sn()==partSn)
                    {
                        if (curItem!=NULL)
                          fprintf(stderr, "THIS SHOULD NEVER HAPPEN: curItemNeedsRestore=true, event fits, but there was already a fitting event!?\n");
                        
                        curItem=temp;
                        }
                    }
              }
        }
}

//---------------------------------------------------------
//   itemSelectionsChanged
//---------------------------------------------------------

bool EventCanvas::itemSelectionsChanged(MusECore::Undo* operations, bool deselectAll)
{
      MusECore::Undo ops;
      MusECore::Undo* opsp = operations ? operations : &ops;
      
      bool item_selected;
      bool obj_selected;
      bool changed=false;
      
      // If we are deselecting all, globally deselect all events,
      //  and don't bother individually deselecting objects, below.
      if(deselectAll)
      {
        opsp->push_back(MusECore::UndoOp(MusECore::UndoOp::GlobalSelectAllEvents, false, 0, 0));
        changed = true;
      }
      
      
      for (iCItem i = items.begin(); i != items.end(); ++i) {
            CItem* item = i->second;
            item_selected = item->isSelected();
            obj_selected = item->objectIsSelected();
            // Don't bother deselecting objects if we have already deselected all, above.
            if((item_selected || !deselectAll) &&
                ((item_selected != obj_selected) ||
                // Need to force this because after the 'deselect all events' command executes,
                //  if the item is selected another select needs to be executed even though it
                //  appears nothing changed here.
                (item_selected && deselectAll)))
              
            {
              opsp->push_back(MusECore::UndoOp(MusECore::UndoOp::SelectEvent,
                  item->event(), item->part(), item_selected, obj_selected));

              changed=true;
            }
      }

      if (!operations && changed)
      {
            // Set the 'sender' to this so that we can ignore self-generated songChanged signals.
            // Here we have a choice of whether to allow undoing of selections.
            if(MusEGlobal::config.selectionsUndoable)
              MusEGlobal::song->applyOperationGroup(ops, MusECore::Song::OperationUndoMode, this);
            else
              MusEGlobal::song->applyOperationGroup(ops, MusECore::Song::OperationExecuteUpdate, this);
            
// For testing...
//               fprintf(stderr, "EventCanvas::updateSelection: Applied SelectPart operations, redrawing\n");
      }

      return changed;
}


bool EventCanvas::stuckNoteExists(int port, int channel, int pitch) const
{
  const int sz = _stuckNotes.size();
  for(int i = 0; i < sz; ++i)
  {
    const MusECore::MidiPlayEvent& s_ev(_stuckNotes.at(i));
    if(s_ev.type() == MusECore::ME_NOTEON &&
       port == s_ev.port() &&
       channel == s_ev.channel() &&
       pitch == s_ev.dataA())
      return true;
  }
  return false;
}

bool EventCanvas::stopStuckNote(int port, int channel, int pitch)
{
  int playedPitch = pitch;
  // Apply track transposition, but only for midi tracks, not drum tracks.
  if(track()->isMidiTrack() && !track()->isDrumTrack())
    playedPitch += track()->transposition;
  const int sz = _stuckNotes.size();
  for(int i = 0; i < sz; ++i)
  {
    MusECore::MidiPlayEvent s_ev(_stuckNotes.at(i));
    if(s_ev.type() == MusECore::ME_NOTEON &&
       port == s_ev.port() &&
       channel == s_ev.channel() &&
       playedPitch == s_ev.dataA())
    {
      unsigned int frame = MusEGlobal::audio->curFrame();
      s_ev.setType(MusECore::ME_NOTEOFF);
      s_ev.setTime(frame);
      if(s_ev.dataB() == 0)
        s_ev.setB(64);
      MusEGlobal::midiPorts[port].putEvent(s_ev);
      _stuckNotes.remove(i);
      return true;
    }
  }
  return false;
}

//---------------------------------------------------------
//   songChanged(type)
//---------------------------------------------------------

void EventCanvas::songChanged(MusECore::SongChangedStruct_t flags)
      {
      if (flags & ~(SC_SELECTION | SC_PART_SELECTION | SC_TRACK_SELECTION)) {
            // TODO FIXME: don't we actually only want SC_PART_*, and maybe SC_TRACK_DELETED?
            //             (same in waveview.cpp)
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

      if(flags & (SC_SELECTION))
      {
        // Prevent race condition: Ignore if the change was ultimately sent by the canvas itself.
        if(flags._sender != this)
          updateItemSelections();
      }
      
      bool f1 = static_cast<bool>(flags & (SC_EVENT_INSERTED | SC_EVENT_MODIFIED | SC_EVENT_REMOVED | 
                         SC_PART_INSERTED | SC_PART_MODIFIED | SC_PART_REMOVED |
                         SC_TRACK_INSERTED | SC_TRACK_REMOVED | SC_TRACK_MODIFIED |
                         SC_SIG | SC_TEMPO | SC_KEY | SC_MASTER | SC_CONFIG | SC_DRUMMAP));
      bool f2 = static_cast<bool>(flags & SC_SELECTION);
      
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

      // Select items by key (PianoRoll & DrumEditor)
      if (key == shortcuts[SHRT_SEL_RIGHT].key || key == shortcuts[SHRT_SEL_RIGHT_ADD].key) {
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
// For testing...
//         fprintf(stderr, "EventCanvas::keyRelease not isAutoRepeat\n");
      
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
            fprintf(stderr, "local DROP\n");      
            //event->acceptProposedAction();     
            //event->ignore();                     // TODO CHECK Tim.
            return;
            }
      if (event->mimeData()->hasFormat("text/x-muse-groupedeventlists")) {
            text = QString(event->mimeData()->data("text/x-muse-groupedeventlists"));
      
            int x = event->pos().x();
            if(x < 0)
              x = 0;
            x = editor->rasterVal(x);
            if (x < 0)
                  x = 0;
            paste_at(text,x,3072,false,false,curPart);
            //event->accept();  // TODO
            }
      else {
            fprintf(stderr, "cannot decode drop\n");
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
        fprintf(stderr, "EventCanvas::startPlayEvent %d %d %d %d\n", note, velocity, port, channel);

      if(!track())
      {
        stopPlayEvents();
        return;
      }
      
      int playedPitch        = note;
      // Apply track transposition, but only for midi tracks, not drum tracks.
      if(track()->isMidiTrack() && !track()->isDrumTrack())
        playedPitch += track()->transposition;
      
      // Release any current note.
      stopStuckNote(port, channel, note);

      // play note:
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

void EventCanvas::stopPlayEvents()
      {
      if (!MusEGlobal::audioDevice)
          return;

      // Stop all currently playing notes.
      unsigned int frame = MusEGlobal::audio->curFrame();
      int port;
      const int sz = _stuckNotes.size();
      for(int i = 0; i < sz; ++i)
      {
        MusECore::MidiPlayEvent ev(_stuckNotes.at(i));
        port = ev.port();
        if(port < 0 || port >= MusECore::MIDI_PORTS)
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


void EventCanvas::setRangeToSelection() {

    int tick_max = 0;
    int tick_min = INT_MAX;
    bool found = false;

    for (const auto& i : items) {
        if (!i.second->isSelected())
            continue;

        int tick = i.second->x();
        int len = i.second->event().lenTick();
        found = true;
        if (tick + len > tick_max)
            tick_max = tick + len;
        if (tick < tick_min)
            tick_min = tick;
    }

    if (found) {
        MusECore::Pos p1(tick_min, true);
        MusECore::Pos p2(tick_max, true);

        if (p1 < MusEGlobal::song->lPos()) {
            MusEGlobal::song->setPos(MusECore::Song::LPOS, p1);
            MusEGlobal::song->setPos(MusECore::Song::RPOS, p2);
        } else {
            MusEGlobal::song->setPos(MusECore::Song::RPOS, p2);
            MusEGlobal::song->setPos(MusECore::Song::LPOS, p1);
        }
    }
}



} // namespace MusEGui
