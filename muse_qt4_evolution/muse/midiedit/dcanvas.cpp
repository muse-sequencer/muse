//=============================================================================
//  MusE
//  Linux Music Editor
//  $Id:$
//
//  Copyright (C) 2002-2006 by Werner Schweer and others
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License version 2.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//=============================================================================

#include "dcanvas.h"
#include "song.h"
#include "midieditor.h"
#include "drummap.h"
#include "audio.h"
#include "velocity.h"
#include "gconfig.h"
#include "part.h"

#define CARET   12
#define CARET2   6

//---------------------------------------------------------
//   drumMap
//---------------------------------------------------------

DrumMap* DrumCanvas::drumMap() const
      {
	DrumMap* drumMap = track()->drumMap();
      if (drumMap == 0)
            drumMap = &noDrumMap;
      return drumMap;
      }

//---------------------------------------------------------
//   y2pitch
//---------------------------------------------------------

int DrumCanvas::y2pitch(int y) const
	{
      return drumMap()->anote(EventCanvas::y2pitch(y));
      }

//---------------------------------------------------------
//   pitch2y
//---------------------------------------------------------

int DrumCanvas::pitch2y(int pitch) const
	{
      return EventCanvas::pitch2y(drumMap()->outmap(pitch));
      }

//---------------------------------------------------------
//   DrumCanvas
//---------------------------------------------------------

DrumCanvas::DrumCanvas(MidiEditor* pr)
   : EventCanvas(pr, TIME_CANVAS_DRUMEDIT)
      {
      singlePitch = -1;
      verticalScrollBar()->setSingleStep(drumHeight/2);
      canvasTools = PointerTool | PencilTool | RubberTool;
      songChanged(SC_TRACK_INSERTED);
      connect(track(), SIGNAL(drumMapChanged()), widget(), SLOT(update()));
      }

//---------------------------------------------------------
//   addItem
//---------------------------------------------------------

void DrumCanvas::addItem(Part* part, const Event& event)
      {
      CItem* item = new CItem(event, part);
      unsigned tick = event.tick() + part->tick();
      item->pos = Pos(tick);
      items.add(item);
      }

//---------------------------------------------------------
//   paint
//---------------------------------------------------------

void DrumCanvas::paint(QPainter& p, QRect r)
      {
      p.setPen(QPen(Qt::black, 0.0));

      QPainterPath pa;
      pa.moveTo(-CARET2, 0);
      pa.lineTo(0, - CARET2);
      pa.lineTo(CARET2, 0);
      pa.lineTo(0, CARET2);
      pa.closeSubpath();

      Pos p1(pix2pos(r.x() - CARET2));
      Pos p2(pix2pos(r.x() + r.width() + CARET2));
      iCItem from(items.lower_bound(p1.tick()));
      iCItem to(items.upper_bound(p2.tick()));

      for (iCItem i = from; i != to; ++i) {
            CItem* e = i->second;
            int x    = pos2pix(AL::Pos(i->first));
            int y    = pitch2y(e->event.pitch()) + drumHeight/2;
            QPoint pt(x, y);
            Event me(e->event);

            DrumMapEntry* dm = drumMap()->entry(me.pitch()); //Get the drum item
            QColor color;
            int velo = me.velo();
            if (velo < dm->lv1)
                  color.setRgb(240, 240, 255);
            else if (velo < dm->lv2)
                  color.setRgb(200, 200, 255);
            else if (velo < dm->lv3)
                  color.setRgb(170, 170, 255);
            else
                  color.setRgb(0, 0, 255);

            if (e->part != curPart)
                  p.setBrush(Qt::lightGray);
            else if (e->isMoving) {
                  p.setBrush(Qt::gray);
                  p.translate(pt);
                  p.drawPath(pa);
                  p.translate(-pt);
                  p.setBrush(Qt::black);

                  int x = pos2pix(e->moving);
                  int y = e->my + drumHeight/2;
                  pt = QPoint(x, y);
                  }
            else if (e->isSelected())
                  p.setBrush(Qt::black);
            else
                  p.setBrush(color);
            p.translate(pt);
            p.drawPath(pa);
            p.translate(-pt);
            }

      //---------------------------------------------------
      //    draw lasso
      //---------------------------------------------------

      if (drag == DRAG_LASSO) {
            p.setPen(Qt::blue);
            p.setBrush(Qt::NoBrush);
            p.drawRect(lasso);
            }
      }

//---------------------------------------------------------
//   moveItem
//---------------------------------------------------------

void DrumCanvas::moveItem(CItem* nevent, DragType dtype)
      {
      Part* part  = nevent->part;
      Event event = nevent->event;
      int npitch   = y2pitch(nevent->my);

      Event newEvent = event.clone();
      newEvent.setPitch(npitch);
      newEvent.setPos(nevent->moving - *(nevent->part));
      if (dtype == MOVE_COPY)
            audio->msgAddEvent(newEvent, part, false);
      else
            audio->msgChangeEvent(event, newEvent, part, false);
      }

//---------------------------------------------------------
//   newItem
//---------------------------------------------------------

CItem* DrumCanvas::newItem(const QPoint& p, int state)
      {
      AL::Pos pos(pix2pos(p.x()));
      pos.snap(raster());
      if (pos < partPos1 || pos >= partPos2) {
            return 0;
            }

      int pitch = y2pitch(p.y());
      int instr = drumMap()->outmap(pitch);
      DrumMapEntry* dm = drumMap()->entry(instr);
      int velo  = dm->lv4;
      if (state == Qt::ShiftModifier)
            velo = dm->lv3;
      else if (state == Qt::ControlModifier)
            velo = dm->lv2;
      else if (state == (Qt::ControlModifier | Qt::ShiftModifier))
            velo = dm->lv1;

      Event e(Note);
      e.setPos(pos - *curPart);
      e.setPitch(drumMap()->anote(instr));
      e.setVelo(velo);
      e.setLenTick(dm->len);
      return new CItem(e, curPart);
      }

//---------------------------------------------------------
//   resizeItem
//---------------------------------------------------------

void DrumCanvas::resizeItem(CItem* nevent, bool)
      {
      Event ev = nevent->event;
      audio->msgDeleteEvent(ev, nevent->part);
      }

//---------------------------------------------------------
//   newItem
//---------------------------------------------------------

void DrumCanvas::newItem(CItem* nevent, bool noSnap)
      {
      Event event = nevent->event;
      Pos pos(nevent->pos);
      if (!noSnap)
            pos.snap(editor->raster());
      event.setPos(pos - *(nevent->part));
      int npitch = event.pitch();
      event.setPitch(npitch);

      //
      // check for existing event
      //    if found change command semantic from insert to delete
      //
      EventList* el = nevent->part->events();
      iEvent lower  = el->lower_bound(event.tick());
      iEvent upper  = el->upper_bound(event.tick());

      for (iEvent i = lower; i != upper; ++i) {
            Event ev = i->second;
            if (ev.pitch() == npitch) {
                  audio->msgDeleteEvent(ev, nevent->part);
                  return;
                  }
            }

      audio->msgAddEvent(event, nevent->part);
      }

//---------------------------------------------------------
//   deleteItem
//---------------------------------------------------------

bool DrumCanvas::deleteItem(CItem* item)
      {
      Event ev = item->event;
      audio->msgDeleteEvent(ev, item->part);
      return false;
      }

//---------------------------------------------------------
//   cmd
//---------------------------------------------------------

void DrumCanvas::cmd(QAction* a)
      {
      QString cmd(a->data().toString());

      if (cmd == "paste")
            paste();
      else if (cmd == "sel_all") {
            for (iCItem k = items.begin(); k != items.end(); ++k) {
                  if (!k->second->isSelected())
                        selectItem(k->second, true);
                  }
            }
      else if (cmd == "sel_none")
            deselectAll();
      else if (cmd == "sel_inv") {
            for (iCItem k = items.begin(); k != items.end(); ++k)
                  selectItem(k->second, !k->second->isSelected());
            }
      else if (cmd == "sel_ins_loc") {
            for (iCItem k = items.begin(); k != items.end(); ++k) {
                  CItem* nevent = k->second;
                  Part* part = nevent->part;
                  Event event = nevent->event;
                  unsigned tick  = event.tick() + part->tick();
                  if (tick < song->lpos() || tick >= song->rpos())
                        selectItem(k->second, false);
                  else
                        selectItem(k->second, true);
                  }
            }
      else if (cmd == "sel_out_loc") {
            for (iCItem k = items.begin(); k != items.end(); ++k) {
                  CItem* nevent = k->second;
                  Part* part    = nevent->part;
                  Event event   = nevent->event;
                  unsigned tick = event.tick() + part->tick();
                  if (tick < song->lpos() || tick >= song->rpos())
                        selectItem(k->second, true);
                  else
                        selectItem(k->second, false);
                  }
            }
      else if (cmd == "delete") {
            if (selectionSize()) {
                  song->startUndo();
                  for (iCItem i = items.begin(); i != items.end(); ++i) {
                        if (!i->second->isSelected())
                              continue;
                        Event ev = i->second->event;
                        audio->msgDeleteEvent(ev, i->second->part, false);
                        }
                  song->endUndo(SC_EVENT_REMOVED);
                  }
            return;
            }
      else if (cmd == "midi_fixed_len") {
            if (selectionSize()) {
                  song->startUndo();
                  for (iCItem k = items.begin(); k != items.end(); ++k) {
                        if (k->second->isSelected()) {
                              CItem* devent  = k->second;
                              Event event    = devent->event;
                              Event newEvent = event.clone();
                              newEvent.setLenTick(drumMap()->entry(event.pitch())->len);
                              audio->msgChangeEvent(event, newEvent, devent->part, false);
                              }
                        }
                  song->endUndo(SC_EVENT_MODIFIED);
                  }
            }
      else if (cmd == "goto_left") {
            int frames = pos[0].tick() - editor->rasterStep(pos[0].tick());
            if (frames < 0)
                  frames = 0;
            Pos p(frames, AL::TICKS);
            song->setPos(0, p, true, true, true);
            }
      else if (cmd == "goto_right") {
            Pos p(pos[0].tick() + editor->rasterStep(pos[0].tick()), AL::TICKS);
            song->setPos(0, p, true, true, true);
            }
      else if (cmd == "mid_mod_velo") {
            Velocity w(this);
            w.setRange(editor->applyTo());
            if (w.exec()) {
                  editor->setApplyTo(w.range());
                  int rate   = w.rateVal();
                  int offset = w.offsetVal();

                  song->startUndo();
                  for (iCItem k = items.begin(); k != items.end(); ++k) {
                        CItem* devent = k->second;
                        Event event   = devent->event;
                        if (event.type() != Note)
                              continue;
                        unsigned tick = event.tick();
                        bool selected = devent->isSelected();
                        bool inLoop   = (tick >= song->lpos()) && (tick < song->rpos());

                        int range = editor->applyTo();
                        if ((range == RANGE_ALL)
                           || (range == RANGE_SELECTED && selected)
                           || (range == RANGE_LOOPED && inLoop)
                           || (range == (RANGE_LOOPED | RANGE_SELECTED) && selected && inLoop)) {
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
                                    audio->msgChangeEvent(event, newEvent, devent->part, false);
                                    }
                              }
                        }
                  song->endUndo(SC_EVENT_MODIFIED);
                  }
            }
      widget()->update();
      }

//---------------------------------------------------------
//   paste
//    paste events
//---------------------------------------------------------

void DrumCanvas::paste()
      {
	QString stype("x-muse-eventlist");
      QString s = QApplication::clipboard()->text(stype, QClipboard::Selection);
      pasteAt(s, song->cpos());
      }

//---------------------------------------------------------
//   startDrag
//---------------------------------------------------------

void DrumCanvas::startDrag(CItem* /* item*/, bool /*copymode*/)
      {
printf("DrumCanvas: startDrag\n");
#if 0 //TD
      QMimeData* drag = getTextDrag();
      if (drag) {
            QApplication::clipboard()->setMimeData(drag, QClipboard::Selection);
            if (copymode)
                  drag->dragCopy();
            else
                  drag->dragMove();
            }
#endif
      }

//---------------------------------------------------------
//   dragEnterEvent
//---------------------------------------------------------

void DrumCanvas::dragEnterEvent(QDragEnterEvent* /*event*/)
      {
printf("DrumCanvas: dragEnterEvent\n");
//TD      event->accept(Q3TextDrag::canDecode(event));
      }

//---------------------------------------------------------
//   dragMoveEvent
//---------------------------------------------------------

void DrumCanvas::dragMoveEvent(QDragMoveEvent*)
      {
      printf("drag move %p\n", this);
      }

//---------------------------------------------------------
//   dragLeaveEvent
//---------------------------------------------------------

void DrumCanvas::dragLeaveEvent(QDragLeaveEvent*)
      {
      printf("drag leave\n");
      }

//---------------------------------------------------------
//   dropEvent
//---------------------------------------------------------

void DrumCanvas::viewDropEvent(QDropEvent* /*event*/)
      {
printf("DrumCanvas: viewDropEvent\n");
#if 0 //TD
      QString text;
      if (event->source() == this) {
            printf("local DROP\n");
            return;
            }
      if (Q3TextDrag::decode(event, text)) {
//            printf("drop <%s>\n", text.ascii());
            int x = editor->rasterVal(event->pos().x());
            if (x < 0)
                  x = 0;
            pasteAt(text, x);
            }
#endif
      }

//---------------------------------------------------------
//   keyPressed
//---------------------------------------------------------

void DrumCanvas::keyPressed(int index, bool)
      {
      int pitch = drumMap()->entry(index)->anote;

      // play note:
      MidiEvent e(0, 0, 0x90, pitch, 127);
      track()->playMidiEvent(&e);
      }

//---------------------------------------------------------
//   keyReleased
//---------------------------------------------------------

void DrumCanvas::keyReleased(int index, bool)
      {
      int pitch = drumMap()->entry(index)->anote;

      // release note:
      MidiEvent e(0, 0, 0x90, pitch, 0);
      track()->playMidiEvent(&e);
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
      audio->msgIdle(true);
      song->startUndo();
      for (iCItem i = items.begin(); i != items.end(); ++i) {
            if (!(i->second->isSelected()))
                  continue;
            CItem* e    = i->second;
            Event event = e->event;
            if (event.type() != Note)
                  continue;

            Part* part = e->part;
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
                        /*
                        {
                        int len = event.lenTick() + delta;
                        if (len < 1)
                              len = 1;
                        newEvent.setLenTick(len);
                        }
                        */
                        printf("DrumCanvas::modifySelected - NoteInfo::VAL_LEN not implemented\n");
                        break;
                  case NoteInfo::VAL_VELON:
                        /*
                        {
                        int velo = event->velo() + delta;
                        if (velo > 127)
                              velo = 127;
                        else if (velo < 0)
                              velo = 0;
                        newEvent.setVelo(velo);
                        }
                        */
                        printf("DrumCanvas::modifySelected - NoteInfo::VAL_VELON not implemented\n");
                        break;
                  case NoteInfo::VAL_VELOFF:
                        /*
                        {
                        int velo = event.veloOff() + delta;
                        if (velo > 127)
                              velo = 127;
                        else if (velo < 0)
                              velo = 0;
                        newEvent.setVeloOff(velo);
                        }
                        */
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
            song->changeEvent(event, newEvent, part);
            song->undoOp(UndoOp::ModifyEvent, newEvent, event, part);
            }
      song->endUndo(SC_EVENT_MODIFIED);
      audio->msgIdle(false);
      }

//---------------------------------------------------------
//   paintDrumList
//---------------------------------------------------------

void DrumCanvas::paintDrumList(QPainter& p, QRect r)
      {
      p.fillRect(r, QColor(0xe0, 0xe0, 0xe0));
      paintVLine(p, 0, rPanelB.y(), rPanelA.x() + rPanelA.width());
//      p.setFont(config.fonts[1]);

      int yoff = wpos.y() - rPanelA.y();
      int i = (r.y() + yoff) / drumHeight;
      if (i < 0)
            i = 0;
      if (i > 127)
            return;
      int y = i * drumHeight - yoff;
      DrumMap* dm = drumMap();

      for (; i < 128; ++i, y += drumHeight) {
            if (y > r.y() + r.height())
                  break;
            QRect r(3, y, drumWidth-4, drumHeight);
            if (dm->anote(i) == curPitch || dm->anote(i) == singlePitch)
            	p.fillRect(0, y, drumWidth-4, drumHeight, Qt::white);
            DrumMapEntry* de = dm->entry(i);
            if (de->mute) {
                  p.setPen(Qt::red);
            	p.drawText(r, Qt::AlignVCenter | Qt::AlignLeft, "m");
                  p.setPen(Qt::darkGray);
            	p.drawText(r.adjusted(16, 0, 0, 0), Qt::AlignVCenter | Qt::AlignLeft, de->name);
            	}
            else {
            	p.setPen(Qt::black);
            	p.drawText(r, Qt::AlignVCenter | Qt::AlignLeft, "m");
            	p.drawText(r.adjusted(16, 0, 0, 0), Qt::AlignVCenter | Qt::AlignLeft, de->name);
                  }
            if (i != 0)
                  p.drawLine(0, y, drumWidth-3, y);
            }
      p.setPen(QPen(Qt::darkGray, 1));
	p.drawLine(17, r.y(), 17,  r.y() + r.height());
      }

//---------------------------------------------------------
//   searchItem
//---------------------------------------------------------

CItem* DrumCanvas::searchItem(const QPoint& p) const
      {
      Pos p1(pix2pos(p.x() - CARET2));
      Pos p2(pix2pos(p.x() + CARET2));

      ciCItem from(items.lower_bound(p1.tick()));
      ciCItem to(items.upper_bound(p2.tick()));

      int pitch = y2pitch(p.y());
      for (ciCItem i = from; i != to; ++i) {
            if (pitch == i->second->event.pitch())
                  return i->second;
            }
      return 0;
      }

//---------------------------------------------------------
//   selectLasso
//---------------------------------------------------------

void DrumCanvas::selectLasso(bool toggle)
      {
      Pos p1(pix2pos(lasso.x()));
      Pos p2(pix2pos(lasso.x() + lasso.width()));

      iCItem from(items.lower_bound(p1.tick()));
      iCItem   to(items.upper_bound(p2.tick()));
	int y = lasso.y();
      int pitch1 = y2pitch(y);
      int pitch2 = y2pitch(y + lasso.height());

      int n = 0;
      for (iCItem i = from; i != to; ++i) {
            CItem* item = i->second;
            int pitch = item->event.pitch();
            if (pitch >= pitch1 && pitch <= pitch2) {
                  selectItem(item, !(toggle && item->isSelected()));
                  ++n;
                  }
            }
      if (n) {
            updateSelection();
            widget()->update();
            }
      }

//---------------------------------------------------------
//   mousePress
//---------------------------------------------------------

void DrumCanvas::mousePress(QMouseEvent* ev)
	{
      QPoint r(ev->pos());
	int pitch = y2pitch(r.y() - rPanelA.y() - wpos.y());
      if (r.x() < 20) {
            //
            // "mute" button click
            //
            DrumMap* dm = drumMap();
            int idx = dm->outmap(pitch);
            DrumMapEntry* de = dm->entry(idx);
            de->mute = !de->mute;
            widget()->update(rPanelA);
      	return;
      	}
      else if (r.x() < rCanvasA.x()) {
            if (pitch != singlePitch) {
                  singlePitch = pitch;
                  for (iCtrlEdit i = ctrlEditList.begin(); i != ctrlEditList.end(); ++i)
	            	(*i)->setSinglePitch(singlePitch);
                  widget()->update();
                  }
            }
      EventCanvas::mousePress(ev);
      }


