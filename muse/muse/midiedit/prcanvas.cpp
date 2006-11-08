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

#include "midieditor.h"
#include "prcanvas.h"
#include "cmd.h"
#include "gatetime.h"
#include "velocity.h"
#include "song.h"
#include "audio.h"
#include "part.h"

#include "velocity.h"
#include "gatetime.h"

//---------------------------------------------------------
//   PianoCanvas
//---------------------------------------------------------

PianoCanvas::PianoCanvas(MidiEditor* pr)
   : EventCanvas(pr, TIME_CANVAS_PIANOROLL)
      {
      verticalScrollBar()->setSingleStep(keyHeight/2);
      playedPitch = -1;
      colorMode   = 0;
      canvasTools = PointerTool | PencilTool | RubberTool | DrawTool;

      // register midi commands
      cmdModifyGateTime = new ModifyGateTimeCmd(pr);
      cmdModifyVelocity = new ModifyVelocityCmd(pr);

      songChanged(SC_TRACK_INSERTED);
      }

//---------------------------------------------------------
//   addItem
//---------------------------------------------------------

void PianoCanvas::addItem(Part* part, const Event& event)
      {
      CItem* item   = new CItem(event, part);
      int y         = pitch2y(event.pitch()) + keyHeight/4 + wpos.y();
      item->pos     = event.pos() + *part;
      unsigned time = item->pos.time(timeType());
      item->bbox    = QRect(time, y, event.lenTick(), keyHeight/2);
      items.add(item);
      }

//---------------------------------------------------------
//   timeTypeChanged
//---------------------------------------------------------

void PianoCanvas::timeTypeChanged()
      {
      //
      // recalculate bounding boxes
      //
      for (iCItem i = items.begin(); i != items.end(); ++i) {
            CItem* item = i->second;
            unsigned t1 = item->event.pos().time(timeType());
            unsigned t2 = item->event.end().time(timeType());
            item->bbox.setX(t1 + (item->part)->time(timeType()));
            item->bbox.setWidth(t2 - t1);
            }
      }

//---------------------------------------------------------
//   paint
//---------------------------------------------------------

void PianoCanvas::paint(QPainter& p, QRect cr)
      {
      static QColor color1[12] = {
            QColor(0xff, 0x3d, 0x39),
            QColor(0x39, 0xff, 0x39),
            QColor(0x39, 0x3d, 0xff),
            QColor(0xff, 0xff, 0x39),
            QColor(0xff, 0x3d, 0xff),
            QColor(0x39, 0xff, 0xff),
            QColor(0xff, 0x7e, 0x7a),
            QColor(0x7a, 0x7e, 0xff),
            QColor(0x7a, 0xff, 0x7a),
            QColor(0xff, 0x7e, 0xbf),
            QColor(0x7a, 0xbf, 0xff),
            QColor(0xff, 0xbf, 0x7a)
            };

      QPoint off(MAP_OFFSET - wpos.x(), -wpos.y());
      p.translate(off);
      cr.translate(-off);

      int time1 = lrint(cr.x() / _xmag);
      int w = lrint(cr.width() / _xmag);
      cr.setRect(
            time1,
            lrint(cr.y() / _ymag),
            w,
            lrint(cr.height() / _ymag)
            );
      p.scale(_xmag, _ymag);

	int time2 = time1 + w;

      //---------------------------------------------------
      //    draw Canvas Items
      //---------------------------------------------------

      p.setPen(QPen(Qt::black, 0.0));

      for (iCItem i = items.begin(); i != items.end(); ++i) {
            CItem* item = i->second;
            QRect r(item->bbox);
            if (r.x() >= time2)
                  break;
            if (!cr.intersects(r))
                  continue;
            Event event(item->event);

            QColor color;
            if (item->part != curPart)
                  p.setBrush(Qt::lightGray);
            else {
                  if (item->isMoving) {
                        p.setBrush(Qt::gray);
                        p.drawRect(r);
                        p.setBrush(Qt::NoBrush);
                        int x = item->moving.tick();
                        int y = item->my + item->bbox.height()/2;
                        int w = item->bbox.width();
                        int h = item->bbox.height();
                        p.drawRect(x, y, w, h);
                        }
                  else if (item->isSelected()) {
                        p.setBrush(Qt::black);
                        }
                  else {
                        if (colorMode == 1)
                              color = color1[event.pitch() % 12];
                        else if (colorMode == 2) {
                              int velo = event.velo();
                              if (velo < 64)
                                    color.setRgb(velo*4, 0, 0xff);
                              else
                                    color.setRgb(0xff, 0, (127-velo) * 4);
                              }
                        else
                              color.setRgb(0, 0, 255);
                        p.setBrush(color);
                        }
                  }
            p.drawRect(r);
            }

      //---------------------------------------------------
      //    draw lasso
      //---------------------------------------------------

      p.resetMatrix();
      p.translate(rCanvasA.topLeft());

      if (drag == DRAG_LASSO) {
            p.setPen(Qt::blue);
            p.setBrush(Qt::NoBrush);
            p.drawRect(lasso);
            }
      }

//---------------------------------------------------------
//   viewMouseDoubleClickEvent
//---------------------------------------------------------

void PianoCanvas::viewMouseDoubleClickEvent(QMouseEvent* event)
      {
      if ((_tool != PointerTool) && (event->button() != Qt::LeftButton)) {
//            mousePress(event);
            return;
            }
      }

//---------------------------------------------------------
//   moveItem
//    called after moving an object
//---------------------------------------------------------

void PianoCanvas::moveItem(CItem* item, DragType dtype)
      {
      Part* part  = item->part;
      Event event = item->event;
      int npitch  = y2pitch(item->my - wpos.y() + item->bbox.height()/2);
      if (event.pitch() != npitch && editor->playEvents()) {
            // release note:
            MidiEvent ev1(0, 0, 0x90, event.pitch() + track()->transposition(), 0);
            track()->playMidiEvent(&ev1);
            MidiEvent ev2(0, 0, 0x90, npitch + track()->transposition(), event.velo());
            track()->playMidiEvent(&ev2);
            }

      Event newEvent = event.clone();
      newEvent.setPitch(npitch);
      newEvent.setPos(item->moving - *part);

      if (dtype == MOVE_COPY)
            audio->msgAddEvent(newEvent, part, false);
      else
            audio->msgChangeEvent(event, newEvent, part, false);
      }

//---------------------------------------------------------
//   newItem(p, state)
//---------------------------------------------------------

CItem* PianoCanvas::newItem(const QPoint& p, int)
      {
      Pos opos(pix2pos(p.x()));
      Pos pos(opos);
      pos.downSnap(raster());

      if (pos < partPos1 || pos >= partPos2)
            return 0;

      int pitch = y2pitch(p.y());
      Event e(Note);
      e.setPitch(pitch);
      e.setVelo(curVelo);
      e.setPos(pos - *curPart);

      CItem* i = new CItem(e, curPart);
      int l    = timeType() == AL::TICKS ? e.lenTick() : e.lenFrame();
      int x    = pos.time(timeType());
      int y    = pitch2y(pitch) + keyHeight/4 + wpos.y();
      i->bbox  = QRect(x, y, l, keyHeight/2);

      return i;
      }

void PianoCanvas::newItem(CItem* item, bool noSnap)
      {
      Event event = item->event;
      Pos p1(item->bbox.x(), timeType());
      Pos p2(item->bbox.x() + item->bbox.width(), timeType());
      int tickLen;

      if (noSnap)
            tickLen = p2.tick() - p1.tick();
      else {
            p1.downSnap(raster());
            tickLen = editor->quantVal(p2.tick() - p1.tick());
            }

      Part* part = item->part;
      event.setPos(p1 - *part);
      event.setLenTick(tickLen);
      audio->msgAddEvent(event, part);
      }

//---------------------------------------------------------
//   resizeItem
//---------------------------------------------------------

void PianoCanvas::resizeItem(CItem* item, bool noSnap)
      {
      Event event    = item->event;
      Event newEvent = event.clone();
      int len;
      if (noSnap)
            len = item->bbox.width();
      else
            len = editor->quantVal(item->bbox.width());
      newEvent.setLenTick(len);
      audio->msgChangeEvent(event, newEvent, item->part);
      }

//---------------------------------------------------------
//   deleteItem
//---------------------------------------------------------

bool PianoCanvas::deleteItem(CItem* item)
      {
      if (item->part == curPart) {
            Event ev = item->event;
            audio->msgDeleteEvent(ev, curPart);
            return true;
            }
      return false;
      }

//---------------------------------------------------------
//   pianoCmd
//---------------------------------------------------------

void PianoCanvas::pianoCmd(int cmd)
      {
      switch(cmd) {
            case MCMD_LEFT:
                  {
                  int frames = pos[0].tick() - editor->rasterStep(pos[0].tick());
                  if (frames < 0)
                        frames = 0;
                  Pos p(frames, AL::TICKS);
                  song->setPos(0, p, true, true, true); //CDW
                  }
                  break;
            case MCMD_RIGHT:
                  {
                  Pos p(pos[0].tick() + editor->rasterStep(pos[0].tick()), AL::TICKS);
                  //if (p > part->tick())
                  //      p = part->tick();
                  song->setPos(0, p, true, true, true); //CDW
                  }
                  break;
            case MCMD_INSERT:
                  {
                  if (pos[0].tick() < startTick || pos[0].tick() >= endTick)
                        break;
                  Part* part = curPart;

                  if (part == 0)
                        break;
                  song->startUndo();
                  EventList* el = part->events();

                  std::list <Event> elist;
                  for (iEvent e = el->lower_bound(pos[0].tick() - part->tick()); e != el->end(); ++e)
                        elist.push_back((Event)e->second);
                  for (std::list<Event>::iterator i = elist.begin(); i != elist.end(); ++i) {
                        Event event = *i;
                        Event newEvent = event.clone();
                        newEvent.setTick(event.tick() + editor->raster());
                        audio->msgChangeEvent(event, newEvent, part, false);
                        }
                  song->endUndo(SC_EVENT_MODIFIED);
                  Pos p(editor->rasterVal(pos[0].tick() + editor->rasterStep(pos[0].tick())), AL::TICKS);
                  song->setPos(0, p, true, false, true);
                  }
                  return;
            case MCMD_DELETE:
                  if (pos[0].tick() < startTick || pos[0].tick() >= endTick)
                        break;
                  {
                  Part* part = curPart;
                  if (part == 0)
                        break;
                  song->startUndo();
                  EventList* el = part->events();

                  std::list<Event> elist;
                  for (iEvent e = el->lower_bound(pos[0].tick()); e != el->end(); ++e)
                        elist.push_back((Event)e->second);
                  for (std::list<Event>::iterator i = elist.begin(); i != elist.end(); ++i) {
                        Event event = *i;
                        Event newEvent = event.clone();
                        newEvent.setTick(event.tick() - editor->raster() - part->tick());
                        audio->msgChangeEvent(event, newEvent, part, false);
                        }
                  song->endUndo(SC_EVENT_MODIFIED);
                  Pos p(editor->rasterVal(pos[0].tick() - editor->rasterStep(pos[0].tick())), AL::TICKS);
                  song->setPos(0, p, true, false, true);
                  }
                  break;
            }
      }

//---------------------------------------------------------
//   cmd
//    pulldown menu commands
//---------------------------------------------------------

void PianoCanvas::cmd(int cmd, int quantStrength, int quantLimit, bool quantLen)
      {
      switch (cmd) {
            case CMD_PASTE:
                  paste();
                  break;
            case CMD_DEL:
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
            case CMD_OVER_QUANTIZE:     // over quantize
                  quantize(100, 1, quantLen);
                  break;
            case CMD_ON_QUANTIZE:     // note on quantize
                  quantize(50, 1, false);
                  break;
            case CMD_ONOFF_QUANTIZE:     // note on/off quantize
                  quantize(50, 1, true);
                  break;
            case CMD_ITERATIVE_QUANTIZE:     // Iterative Quantize
                  quantize(quantStrength, quantLimit, quantLen);
                  break;
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
                        CItem* item   = k->second;
                        Event event   = item->event;
                        unsigned tick = event.tick();
                        if (tick < song->lpos() || tick >= song->rpos())
                              selectItem(k->second, false);
                        else
                              selectItem(k->second, true);
                        }
                  break;
            case CMD_SELECT_OLOOP:     // select outside loop
                  for (iCItem k = items.begin(); k != items.end(); ++k) {
                        CItem* item   = k->second;
                        Event event   = item->event;
                        unsigned tick = event.tick();
                        if (tick < song->lpos() || tick >= song->rpos())
                              selectItem(k->second, true);
                        else
                              selectItem(k->second, false);
                        }
                  break;
            case CMD_MODIFY_GATE_TIME:
                  cmdModifyGateTime->processEvents(&items);
                  break;

            case CMD_MODIFY_VELOCITY:
                  cmdModifyVelocity->processEvents(&items);
                  break;

            case CMD_CRESCENDO:
            case CMD_TRANSPOSE:
            case CMD_THIN_OUT:
            case CMD_ERASE_EVENT:
            case CMD_NOTE_SHIFT:
            case CMD_MOVE_CLOCK:
            case CMD_COPY_MEASURE:
            case CMD_ERASE_MEASURE:
            case CMD_DELETE_MEASURE:
            case CMD_CREATE_MEASURE:
                  break;
            default:
//                  printf("unknown ecanvas cmd %d\n", cmd);
                  break;
            }
      updateSelection();
      widget()->update();
      }

//---------------------------------------------------------
//   quantize
//---------------------------------------------------------

void PianoCanvas::quantize(int strength, int limit, bool quantLen)
      {
      song->startUndo();
      for (iCItem k = items.begin(); k != items.end(); ++k) {
            CItem* item = k->second;
            Event event    = item->event;
            Part* part     = item->part;
            if (event.type() != Note)
                  continue;

            if ((editor->applyTo() & CMD_RANGE_SELECTED) && !k->second->isSelected())
                  continue;

            unsigned tick = event.tick() + part->tick();

            if ((editor->applyTo() & CMD_RANGE_LOOP)
               && ((tick < song->lpos() || tick >= song->rpos())))
                  continue;

            unsigned len   = event.lenTick();
            int tick2 = tick + len;

            // quant start position
            int diff  = editor->rasterVal(tick) - tick;
            if (abs(diff) > limit)
                  tick += ((diff * strength) / 100);

            // quant len
            diff = editor->rasterVal(tick2) - tick2;
            if (quantLen && (abs(diff) > limit))
                  len += ((diff * strength) / 100);

            // something changed?
            if (((event.tick() + part->tick()) != tick) || (event.lenTick() != len)) {
                  Event newEvent = event.clone();
                  newEvent.setTick(tick - part->tick());
                  newEvent.setLenTick(len);
                  audio->msgChangeEvent(event, newEvent, part, false);
                  }
            }
      song->endUndo(SC_EVENT_MODIFIED);
      }

//---------------------------------------------------------
//   paste
//    paste events
//---------------------------------------------------------

void PianoCanvas::paste()
      {
	QString stype("x-muse-eventlist");
      QString s = QApplication::clipboard()->text(stype, QClipboard::Selection);
      pasteAt(s, song->cpos());
      }

//---------------------------------------------------------
//   startDrag
//---------------------------------------------------------

void PianoCanvas::startDrag(CItem* /*item*/, bool /*copymode*/)
      {
      QMimeData* drag = getTextDrag();
      if (drag) {
		QApplication::clipboard()->setMimeData(drag);
#if 0
            if (copymode)
                  drag->dragCopy();
            else
                  drag->dragMove();
#endif
            }
      }

//---------------------------------------------------------
//   dragEnterEvent
//---------------------------------------------------------

void PianoCanvas::dragEnterEvent(QDragEnterEvent*)
      {
//TD      event->accept(Q3TextDrag::canDecode(event));
      }

//---------------------------------------------------------
//   dragMoveEvent
//---------------------------------------------------------

void PianoCanvas::dragMoveEvent(QDragMoveEvent*)
      {
//      printf("drag move %x\n", this);
      }

//---------------------------------------------------------
//   dragLeaveEvent
//---------------------------------------------------------

void PianoCanvas::dragLeaveEvent(QDragLeaveEvent*)
      {
//      printf("drag leave\n");
      }

//---------------------------------------------------------
//   dropEvent
//---------------------------------------------------------

void PianoCanvas::viewDropEvent(QDropEvent*)
      {
#if 0 //TD
      QString text;
      if (event->source() == this) {
            printf("local DROP\n");
            return;
            }
      if (Q3TextDrag::decode(event, text)) {
            int x = editor->rasterVal(event->pos().x());
            if (x < 0)
                  x = 0;
            pasteAt(text, x);
            }
      else {
            printf("cannot decode drop\n");
            }
#endif
      }


//---------------------------------------------------------
//   itemPressed
//---------------------------------------------------------

void PianoCanvas::itemPressed(const CItem* item)
      {
      if (!editor->playEvents())
            return;
      Event event    = item->event;
      playedPitch    = event.pitch() + track()->transposition();
      int velo       = event.velo();

      // play note:
      MidiEvent e(0, 0, 0x90, playedPitch, velo);
      track()->playMidiEvent(&e);
      }

//---------------------------------------------------------
//   itemReleased
//---------------------------------------------------------

void PianoCanvas::itemReleased()
      {
      if (!editor->playEvents())
            return;

      // release note:
      MidiEvent ev(0, 0, 0x90, playedPitch, 0);
      track()->playMidiEvent(&ev);
      playedPitch = -1;
      }

//---------------------------------------------------------
//   itemMoved
//---------------------------------------------------------

void PianoCanvas::itemMoved(const CItem* item)
      {
      int npitch = y2pitch(item->my + wpos.y());
      if ((playedPitch != -1) && (playedPitch != npitch)) {
            Event event = item->event;
            // release note:
            MidiEvent ev1(0, 0, 0x90, playedPitch, 0);
            track()->playMidiEvent(&ev1);
            // play note:
            MidiEvent e2(0, 0, 0x90, npitch + track()->transposition(), event.velo());
            track()->playMidiEvent(&e2);
            playedPitch = npitch + track()->transposition();
            }
      }

//---------------------------------------------------------
//   modifySelected
//---------------------------------------------------------

void PianoCanvas::modifySelected(NoteInfo::ValType type, int delta)
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
                        {
                        int len = event.lenTick() + delta;
                        if (len < 1)
                              len = 1;
                        newEvent.setLenTick(len);
                        }
                        break;
                  case NoteInfo::VAL_VELON:
                        {
                        int velo = event.velo() + delta;
                        if (velo > 127)
                              velo = 127;
                        else if (velo < 0)
                              velo = 0;
                        newEvent.setVelo(velo);
                        }
                        break;
                  case NoteInfo::VAL_VELOFF:
                        {
                        int velo = event.veloOff() + delta;
                        if (velo > 127)
                              velo = 127;
                        else if (velo < 0)
                              velo = 0;
                        newEvent.setVeloOff(velo);
                        }
                        break;
                  case NoteInfo::VAL_PITCH:
                        {
                        int pitch = event.pitch() + delta;
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
//   setColorMode
//---------------------------------------------------------

void PianoCanvas::setColorMode(int mode)
      {
      colorMode = mode;
      widget()->update();
      }

//---------------------------------------------------------
//   searchItem
//---------------------------------------------------------

CItem* PianoCanvas::searchItem(const QPoint& pt) const
      {
      QPoint p(
         lrint((pt.x() - MAP_OFFSET + wpos.x()) / _xmag),
         lrint((pt.y() + wpos.y()) / _ymag)
         );
      return items.find(p);
      }

//---------------------------------------------------------
//   selectLasso
//---------------------------------------------------------

void PianoCanvas::selectLasso(bool toggle)
      {
      QRect r(
//         lrint((lasso.x() - MAP_OFFSET + wpos.x()) / _xmag),
         lrint((lasso.x() + wpos.x()) / _xmag),
         lrint((lasso.y() + wpos.y()) / _ymag),
         lrint(lasso.width() / _xmag),
         lrint(lasso.height() / _ymag)
         );

      int n = 0;
      for (iCItem i = items.begin(); i != items.end(); ++i) {
            if (i->second->intersects(r)) {
                  selectItem(i->second, !(toggle && i->second->isSelected()));
                  ++n;
                  }
            }
      if (n) {
            updateSelection();
            widget()->update();
            }
      }

