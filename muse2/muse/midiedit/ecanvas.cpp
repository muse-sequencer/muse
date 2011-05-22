//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: ecanvas.cpp,v 1.8.2.6 2009/05/03 04:14:00 terminator356 Exp $
//  (C) Copyright 2001 Werner Schweer (ws@seh.de)
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
//   update
//---------------------------------------------------------

void EventCanvas::updateSong(DragType dtype, int flags)
      {
      song->update(flags | ((dtype == MOVE_COPY || dtype == MOVE_CLONE)
         ? SC_EVENT_INSERTED : SC_EVENT_MODIFIED));
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
               //if (rightmost && mapx(rightmost->event().tick()) > width()) for some reason this doesn't this doesnt move the event in view
               //   emit followEvent(rightmost->x());

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
                    //if (leftmost && mapx(leftmost->event().tick())< 0 ) for some reason this doesn't this doesnt move the event in view
                    //  emit followEvent(leftmost->x());
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
//   getTextDrag
//---------------------------------------------------------

//QDrag* EventCanvas::getTextDrag(QWidget* parent)
QMimeData* EventCanvas::getTextDrag()
      {
      //---------------------------------------------------
      //   generate event list from selected events
      //---------------------------------------------------

      EventList el;
      unsigned startTick = MAXINT;
      for (iCItem i = items.begin(); i != items.end(); ++i) {
            if (!i->second->isSelected())
                  continue;
            ///NEvent* ne = (NEvent*)(i->second);
            CItem* ne = i->second;
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
            fprintf(stderr, "EventCanvas::getTextDrag() fopen failed: %s\n",
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
      //    read tmp file into drag Object
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
      
      QByteArray data(fbuf);
      QMimeData* md = new QMimeData();
      //QDrag* drag = new QDrag(parent);
      
      md->setData("text/x-muse-eventlist", data);
      //drag->setMimeData(md);
      
      munmap(fbuf, n);
      fclose(tmp);
      
      //return drag;
      return md;
      }

//---------------------------------------------------------
//   pasteAt
//---------------------------------------------------------

void EventCanvas::pasteAt(const QString& pt, int pos)
      {
      QByteArray ba = pt.toLatin1();
      const char* p = ba.constData();
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
                                            audio->msgChangePart(curPart, newPart, false, true, false);
                                            modified=modified|SC_PART_MODIFIED;
                                            curPart = newPart; // reassign
                                            }
                                    // Indicate no undo, and do not do port controller values and clone parts. 
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
      if (event->mimeData()->hasFormat("text/x-muse-eventlist")) {
            text = QString(event->mimeData()->data("text/x-muse-eventlist"));
      
            int x = editor->rasterVal(event->pos().x());
            if (x < 0)
                  x = 0;
            pasteAt(text, x);
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
      
      
      
      int modified = 0;
      
      Undo operations = moveCanvasItems(moving, dp, dx, dragtype, &modified);
      song->applyOperationGroup(operations);
      updateSong(dragtype, modified);
      
      moving.clear();
      updateSelection();
      redraw();
      }
