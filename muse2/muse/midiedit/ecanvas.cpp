//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: ecanvas.cpp,v 1.8.2.6 2009/05/03 04:14:00 terminator356 Exp $
//  (C) Copyright 2001 Werner Schweer (ws@seh.de)
//=========================================================

#include <values.h>

#include "midieditor.h"
#include "ecanvas.h"
#include "song.h"
#include "event.h"
#include "shortcuts.h"
//Added by qt3to4:
#include <QKeyEvent>
#include <QEvent>

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
      sigmap.tickValues(curPart->tick(), &bar1, &xx, &x);
      sigmap.tickValues(curPart->tick() + curPart->lenTick(), &bar2, &xx, &x);

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
//   startUndo
//---------------------------------------------------------

void EventCanvas::startUndo(DragType)
      {
      song->startUndo();
      }

//---------------------------------------------------------
//   endUndo
//---------------------------------------------------------

void EventCanvas::endUndo(DragType dtype, int flags)
      {
      song->endUndo(flags | ((dtype == MOVE_COPY || dtype == MOVE_CLONE)
         ? SC_EVENT_INSERTED : SC_EVENT_MODIFIED));
      }

//---------------------------------------------------------
//   mouseMove
//---------------------------------------------------------

void EventCanvas::mouseMove(const QPoint& pos)
      {
      emit pitchChanged(y2pitch(pos.y()));
      int x = pos.x();
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
                        //if(e.tick() > len)
                        if(e.endTick() > len)
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
      CItem*   nevent  = 0;

      int n  = 0;       // count selections
      for (iCItem k = items.begin(); k != items.end(); ++k) {
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
            iCItem i = items.begin();
            CItem* nearest = i->second;

            while (i != items.end()) {
                CItem* cur=i->second;                
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
      if (event->state() & Qt::ShiftButton)
            key += Qt::SHIFT;
      if (event->state() & Qt::AltButton)
            key += Qt::ALT;
      if (event->state() & Qt::ControlButton)
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
                  Pos p1(tick_min, true);
                  Pos p2(tick_max, true);
                  song->setPos(1, p1);
                  song->setPos(2, p2);
                  }
            }
      // Select items by key (PianoRoll & DrumEditor)
      else if (key == shortcuts[SHRT_SEL_RIGHT].key || key == shortcuts[SHRT_SEL_RIGHT_ADD].key) {
            iCItem i, iRightmost;
            CItem* rightmost = NULL;
            //Get the rightmost selected note (if any)
            for (i = items.begin(); i != items.end(); ++i) {
                  if (i->second->isSelected()) {
                        iRightmost = i; rightmost = i->second;
                        }
                  }
               if (rightmost) {
                     iCItem temp = iRightmost; temp++;
                     //If so, deselect current note and select the one to the right
                     if (temp != items.end()) {
                           if (key != shortcuts[SHRT_SEL_RIGHT_ADD].key)
                                 deselectAll();

                           iRightmost++;
                           iRightmost->second->setSelected(true);
                           updateSelection();
                           }
                     }
            }
      //Select items by key: (PianoRoll & DrumEditor)
      else if (key == shortcuts[SHRT_SEL_LEFT].key || key == shortcuts[SHRT_SEL_LEFT_ADD].key) {
            iCItem i, iLeftmost;
            CItem* leftmost = NULL;
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
