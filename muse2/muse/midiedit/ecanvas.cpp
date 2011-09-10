//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: ecanvas.cpp,v 1.8.2.6 2009/05/03 04:14:00 terminator356 Exp $
//  (C) Copyright 2001 Werner Schweer (ws@seh.de)
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

#include <errno.h>
#include <values.h>
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
      _playEvents = false;
      curVelo     = 70;

      setBg(Qt::white);
      setAcceptDrops(true);
      setFocusPolicy(Qt::StrongFocus);
      setMouseTracking(true);

      curPart   = (MidiPart*)(editor->parts()->begin()->second);
      curPartId = curPart->sn();
      }

//---------------------------------------------------------
//   getCaption
//---------------------------------------------------------

QString EventCanvas::getCaption() const
      {
      int bar1, bar2, xx;
      unsigned x;
      ///sigmap.tickValues(curPart->tick(), &bar1, &xx, &x);
      AL::sigmap.tickValues(curPart->tick(), &bar1, &xx, &x);
      ///sigmap.tickValues(curPart->tick() + curPart->lenTick(), &bar2, &xx, &x);
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
      emit timeChanged(MAXINT);
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
      song->update(SC_SELECTION);
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
            items.clear();
            start_tick  = MAXINT;
            end_tick    = 0;
            curPart = 0;
            for (iPart p = editor->parts()->begin(); p != editor->parts()->end(); ++p) {
                  MidiPart* part = (MidiPart*)(p->second);
                  if (part->sn() == curPartId)
                        curPart = part;
                  unsigned stick = part->tick();
                  unsigned len = part->lenTick();
                  unsigned etick = stick + len;
                  if (stick < start_tick)
                        start_tick = stick;
                  if (etick > end_tick)
                        end_tick = etick;

                  EventList* el = part->events();
                  for (iEvent i = el->begin(); i != el->end(); ++i) {
                        Event e = i->second;
                        // Added by T356. Do not add events which are either past, or extend past the end of the part.
                        // Reverted to just events which are past. p4.0.24 
                        if(e.tick() > len)      
                        //if(e.endTick() > len)
                          break;
                        
                        if (e.isNote()) {
                              addItem(part, e);
                              }
                        }
                  }
            }

      Event event;
      MidiPart* part   = 0;
      int x            = 0;
      MusEWidget::CItem*   nevent  = 0;

      int n  = 0;       // count selections
      for (MusEWidget::iCItem k = items.begin(); k != items.end(); ++k) {
            Event ev = k->second->event();
            bool selected = ev.selected();
            if (selected) {
                  k->second->setSelected(true);
                  ++n;
                  if (!nevent) {
                        nevent   =  k->second;
                        Event mi = nevent->event();
                        curVelo  = mi.velo();
                        }
                  }
            }
      start_tick = song->roundDownBar(start_tick);
      end_tick   = song->roundUpBar(end_tick);

      if (n == 1) {
            x     = nevent->x();
            event = nevent->event();
            part  = (MidiPart*)nevent->part();
            if (curPart != part) {
                  curPart = part;
                  curPartId = curPart->sn();
                  curPartChanged();
                  }
            }
      emit selectionChanged(x, event, part);
      if (curPart == 0)
            curPart = (MidiPart*)(editor->parts()->begin()->second);
      redraw();
      }

//---------------------------------------------------------
//   selectAtTick
//---------------------------------------------------------
void EventCanvas::selectAtTick(unsigned int tick)
      {
      //Select note nearest tick, if none selected and there are any
      if (!items.empty() && selectionSize() == 0) {
            MusEWidget::iCItem i = items.begin();
	    MusEWidget::CItem* nearest = i->second;

            while (i != items.end()) {
                MusEWidget::CItem* cur=i->second;                
                unsigned int curtk=abs(cur->x() + cur->part()->tick() - tick);
                unsigned int neartk=abs(nearest->x() + nearest->part()->tick() - tick);

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

MidiTrack* EventCanvas::track() const
      {
      return ((MidiPart*)curPart)->track();
      }


//---------------------------------------------------------
//   keyPress
//---------------------------------------------------------

void EventCanvas::keyPress(QKeyEvent* event)
      {
      int key = event->key();
      ///if (event->state() & Qt::ShiftButton)
      if (((QInputEvent*)event)->modifiers() & Qt::ShiftModifier)
            key += Qt::SHIFT;
      ///if (event->state() & Qt::AltButton)
      if (((QInputEvent*)event)->modifiers() & Qt::AltModifier)
            key += Qt::ALT;
      ///if (event->state() & Qt::ControlButton)
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

            for (MusEWidget::iCItem i= items.begin(); i != items.end(); i++) {
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
                  Pos p1(tick_min, true);
                  Pos p2(tick_max, true);
                  song->setPos(1, p1);
                  song->setPos(2, p2);
                  }
            }
      // Select items by key (PianoRoll & DrumEditor)
      else if (key == shortcuts[SHRT_SEL_RIGHT].key || key == shortcuts[SHRT_SEL_RIGHT_ADD].key) {
            MusEWidget::iCItem i, iRightmost;
	    MusEWidget::CItem* rightmost = NULL;
            //Get the rightmost selected note (if any)
            for (i = items.begin(); i != items.end(); ++i) {
                  if (i->second->isSelected()) {
                        iRightmost = i; rightmost = i->second;
                        }
                  }
               if (rightmost) {
                     MusEWidget::iCItem temp = iRightmost; temp++;
                     //If so, deselect current note and select the one to the right
                     if (temp != items.end()) {
                           if (key != shortcuts[SHRT_SEL_RIGHT_ADD].key)
                                 deselectAll();

                           iRightmost++;
                           iRightmost->second->setSelected(true);
                           updateSelection();
                           }
                     }
               //if (rightmost && mapx(rightmost->event().tick()) > width()) for some reason this doesn't this doesnt move the event in view
               //   emit followEvent(rightmost->x());

            }
      //Select items by key: (PianoRoll & DrumEditor)
      else if (key == shortcuts[SHRT_SEL_LEFT].key || key == shortcuts[SHRT_SEL_LEFT_ADD].key) {
            MusEWidget::iCItem i, iLeftmost;
            MusEWidget::CItem* leftmost = NULL;
            if (items.size() > 0 ) {
                  for (i = items.end(), i--; i != items.begin(); i--) {
                        if (i->second->isSelected()) {
                              iLeftmost = i; leftmost = i->second;
                              }
                        }
                    if (leftmost) {
                          if (iLeftmost != items.begin()) {
                                //Add item
                                if (key != shortcuts[SHRT_SEL_LEFT_ADD].key)
                                      deselectAll();
      
                                iLeftmost--;
                                iLeftmost->second->setSelected(true);
                                updateSelection();
                                }
                          }
                    //if (leftmost && mapx(leftmost->event().tick())< 0 ) for some reason this doesn't this doesnt move the event in view
                    //  emit followEvent(leftmost->x());
                  }
            }
      else if (key == shortcuts[SHRT_INC_PITCH].key) {
            modifySelected(MusEWidget::NoteInfo::VAL_PITCH, 1);
            }
      else if (key == shortcuts[SHRT_DEC_PITCH].key) {
            modifySelected(MusEWidget::NoteInfo::VAL_PITCH, -1);
            }
      else if (key == shortcuts[SHRT_INC_POS].key) {
            // TODO: Check boundaries
            modifySelected(MusEWidget::NoteInfo::VAL_TIME, editor->raster());
            }
      else if (key == shortcuts[SHRT_DEC_POS].key) {
            // TODO: Check boundaries
            modifySelected(MusEWidget::NoteInfo::VAL_TIME, 0 - editor->raster());
            }

      else if (key == shortcuts[SHRT_INCREASE_LEN].key) {
            // TODO: Check boundaries
            modifySelected(MusEWidget::NoteInfo::VAL_LEN, editor->raster());
            }
      else if (key == shortcuts[SHRT_DECREASE_LEN].key) {
            // TODO: Check boundaries
            modifySelected(MusEWidget::NoteInfo::VAL_LEN, 0 - editor->raster());
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
            paste_at(text,x); //(curPart, text, x); TODO FINDMICHJETZT
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
      
      
      
      Undo operations = moveCanvasItems(moving, dp, dx, dragtype);
      if (operations.empty())
        songChanged(SC_EVENT_MODIFIED); //this is a hack to force the canvas to repopulate
      	                                //itself. otherwise, if a moving operation was forbidden,
      	                                //the canvas would still show the movement
      else
        song->applyOperationGroup(operations);
      
      moving.clear();
      updateSelection();
      redraw();
      }
