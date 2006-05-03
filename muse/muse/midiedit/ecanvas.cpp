//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: ecanvas.cpp,v 1.65 2006/03/24 21:41:16 a-lin Exp $
//  (C) Copyright 2001-2005 Werner Schweer (ws@seh.de)
//=========================================================

#include "midieditor.h"
#include "ecanvas.h"
#include "song.h"
#include "shortcuts.h"
#include "widgets/simplebutton.h"
#include "widgets/tools.h"
#include "widgets/utils.h"
#include "part.h"
#include "audio.h"
#include "midi.h"

//---------------------------------------------------------
//   EventCanvas
//---------------------------------------------------------

EventCanvas::EventCanvas(MidiEditor* pr, TimeCanvasType type)
   : TimeCanvas(type)
      {
      setMarkerList(song->marker());
      canvasTools = 0;
      curItem     = 0;
      curSplitter = -1;
      dragSplitter = false;

      keyDown     = -1;

      itemPopupMenu   = 0;
      canvasPopupMenu = 0;
      drag            = DRAG_OFF;

      editor      = pr;
      curVelo     = 70;

      setAcceptDrops(true);
      setFocusPolicy(Qt::StrongFocus);
      setMouseTracking(true);

	curPart = editor->parts()->begin()->second;
      connect(song, SIGNAL(midiEvent(const MidiEvent&)), SLOT(midiNote(const MidiEvent&)));
      }

//---------------------------------------------------------
//   range
//---------------------------------------------------------

void EventCanvas::range(int* s, int* e) const
      {
      *s = startTick;
      *e = endTick;
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
//   startUndo
//---------------------------------------------------------

void EventCanvas::startUndo(DragType)
      {
      song->startUndo();
      }

//---------------------------------------------------------
//   endUndo
//---------------------------------------------------------

void EventCanvas::endUndo(DragType dtype)
      {
      song->endUndo((dtype == MOVE_COPY) ? SC_EVENT_INSERTED : SC_EVENT_MODIFIED);
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
//	called from MidiEditor
//---------------------------------------------------------

void EventCanvas::songChanged(int flags)
      {
      if ((flags & ~SC_SELECTION) && !editor->parts()->empty()) {
            items.clear();
            startTick = MAXINT;
            endTick   = 0;
            for (iPart p = editor->parts()->begin(); p != editor->parts()->end(); ++p) {
                  Part* part = p->second;
                  unsigned stick = part->tick();
                  unsigned etick = stick + part->lenTick();
                  if (stick < startTick)
                        startTick = stick;
                  if (etick > endTick)
                        endTick = etick;

                  EventList* el = part->events();
                  for (iEvent i = el->begin(); i != el->end(); ++i) {
                        Event e = i->second;
                        if (e.isNote())
                              addItem(part, e);
                        }
                  }
            }

      Event event;
      Part* part     = 0;
      int x          = 0;
      CItem* nevent  = 0;

      int n  = 0;       // count selections
      for (iCItem k = items.begin(); k != items.end(); ++k) {
            Event ev = k->second->event;
            bool selected = ev.selected();
            if (selected) {
                  k->second->setSelected(true);
                  ++n;
                  if (!nevent) {
                        nevent   =  k->second;
                        Event mi = nevent->event;
                        curVelo  = mi.velo();
                        }
                  }
            }
      startTick = song->roundDownBar(startTick);
      endTick   = song->roundUpBar(endTick);

      if (n == 1) {
            x     = nevent->bbox.x();
            event = nevent->event;
            part  = nevent->part;
            }
      emit selectionChanged(x, event, part);
      setPart(*curPart, curPart->end());
      widget()->update();
      }

//---------------------------------------------------------
//   selectFirst
//---------------------------------------------------------

void EventCanvas::selectFirst()
      {
      //Select leftmost note if none selected and there are any
      if (!items.empty() && selectionSize() == 0) {
            iCItem i = items.begin();
            if (!i->second->isSelected()) {
                  selectItem(i->second, true);
                  songChanged(SC_SELECTION);
                  }
            }
      }

//---------------------------------------------------------
//   track
//---------------------------------------------------------

MidiTrack* EventCanvas::track() const
      {
      return (MidiTrack*)(curPart->track());
      }

//---------------------------------------------------------
//   keyPress
//---------------------------------------------------------

void EventCanvas::keyPress(QKeyEvent* event)
      {
      int key = event->key();
      if (event->modifiers() & Qt::ShiftModifier)
            key += Qt::SHIFT;
      if (event->modifiers() & Qt::AltModifier)
            key += Qt::ALT;
      if (event->modifiers() & Qt::ControlModifier)
            key += Qt::CTRL;

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

                  int tick = i->second->bbox.x();
                  int len = i->second->event.lenTick();
                  found = true;
                  if (tick + len > tick_max)
                        tick_max = tick + len;
                  if (tick < tick_min)
                        tick_min = tick;
                  }
            if (found) {
                  Pos p1(tick_min, AL::TICKS);
                  Pos p2(tick_max, AL::TICKS);
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
                  	iRightmost = i;
                        rightmost = i->second;
                        }
                  }
		if (rightmost) {
            	iCItem temp = iRightmost;
                  temp++;
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
            for (i = items.end(), i--; i != items.begin(); i--) {
                  if (i->second->isSelected()) {
                        iLeftmost = i;
                        leftmost = i->second;
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
      else
            event->ignore();
      }

//---------------------------------------------------------
//   mousePressCanvasA
//---------------------------------------------------------

void EventCanvas::mousePressCanvasA(QMouseEvent* me)
      {
      QPoint p(me->pos() - rCanvasA.topLeft());

      // special events if right button is clicked while operations
      // like moving or drawing lasso is performed.

      if (button == Qt::RightButton) {
            switch (drag) {
	            case DRAG_LASSO:
	    		      drag = DRAG_OFF;
      			widget()->update();
	      		return;
      		case DRAG_MOVE:
	      		drag = DRAG_OFF;
		      	endMoveItems(MOVE_MOVE);
			      return;
      		default:
	      		break;
                  }
            }
      // ignore event if (another) button is already active:
      if (me->buttons() & (Qt::LeftButton | Qt::RightButton | Qt::MidButton)
         & ~button)
            return;

      bool shift = keyState & Qt::ShiftModifier;
      bool ctrl  = keyState & Qt::ControlModifier;
      start      = p;

      //---------------------------------------------------
      //    set curItem to item mouse is pointing
      //    (if any)
      //---------------------------------------------------

      curItem = searchItem(start);
      if (curItem && editor->playEvents()) {
	      MidiEvent e(0, 0, ME_NOTEON, curItem->event.pitch(), curItem->event.velo());
      	track()->playMidiEvent(&e);
            }

      if (curItem && (button == Qt::MidButton)) {
            if (!curItem->isSelected()) {
                  selectItem(curItem, true);
                  updateSelection();
                  widget()->update();
                  }
            startDrag(curItem, shift);
            }
      else if (button == Qt::RightButton) {
            if (curItem) {
                  if (shift) {
                        drag = DRAG_RESIZE;
                        setCursor();
				Pos p1(curItem->bbox.x(), timeType());
      			Pos p2(pix2pos(start.x()));
                        curItem->bbox.setWidth(p2.time(timeType())-p1.time(timeType()));

                        start.setX(curItem->bbox.x());
                        deselectAll();
                        selectItem(curItem, true);
                        updateSelection();
                        widget()->update();
                        }
                  else {
                        itemPopupMenu = genItemPopup(curItem);
                        if (itemPopupMenu) {
                              QAction* a = itemPopupMenu->exec(QCursor::pos());
                              if (a) {
                                    int n = a->data().toInt();
                                    itemPopup(curItem, n, start);
                                    }
                              delete itemPopupMenu;
                              }
                        }
                  }
            else {
                  canvasPopupMenu = genCanvasPopup();
                  if (canvasPopupMenu) {
                        QAction* a = canvasPopupMenu->exec(QCursor::pos(), 0);
                        if (a) {
                              int n = a->data().toInt();
                              canvasPopup(n);
                              }
                        delete canvasPopupMenu;
                        }
                  }
            }
      else if (button == Qt::LeftButton) {
            switch (_tool) {
                  case PointerTool:
                        if (curItem) {
                              itemPressed(curItem);
                              if (curItem->part != curPart)
                                    setCurPart(curItem->part);
                              if (shift && !ctrl)
                                    drag = DRAG_COPY_START;
                              else if (!shift && ctrl) {
                                    //Select all on the same pitch
                                    deselectAll();
                                    int pitch = curItem->event.pitch();
                                    for (iCItem i = items.begin(); i != items.end(); ++i) {
                                          if (i->second->event.pitch() == pitch)
                                                selectItem(i->second, true);
                                         }
                                    updateSelection();
                                    widget()->update();
                              	drag = DRAG_MOVE_START;
                                    }
                              else
                                    drag = DRAG_MOVE_START;
                              }
                        else
                              drag = DRAG_LASSO_START;
                        setCursor();
                        break;

                  case RubberTool:
                        deleteItem(p);
                        drag = DRAG_DELETE;
                        setCursor();
                        break;

                  case PencilTool:
                        if (curItem) {
                              drag = DRAG_RESIZE;
                              setCursor();
                        	
      				Pos p1(curItem->bbox.x(), timeType());
      				Pos p2(pix2pos(start.x()));
                              int w = p2.time(timeType()) - p1.time(timeType());
                              curItem->bbox.setWidth(w);
                              start.setX(curItem->bbox.x());
                              start.setY(curItem->bbox.y());
                              }
                        else {
                              drag = DRAG_NEW;
                              setCursor();
                              curItem = newItem(start, keyState);
                              if (curItem) {
                                    items.add(curItem);
      					if (editor->playEvents()) {
	      					MidiEvent e(0, 0, ME_NOTEON, curItem->event.pitch(), curItem->event.velo());
      						track()->playMidiEvent(&e);
                                    	}
            				}
                              else {
                                    drag = DRAG_OFF;
                                    setCursor();
                                    }
                              }
                        deselectAll();
                        if (curItem)
                              selectItem(curItem, true);
                        updateSelection();
                        widget()->update();
                        break;

                  default:
                        break;
                  }
            }
      }

//---------------------------------------------------------
//   mouseMoveCanvasA
//	pos is relative to CanvasA
//---------------------------------------------------------

void EventCanvas::mouseMoveCanvasA(QPoint pos)
      {
      QPoint dist = pos - start;
      bool isMoving = dist.manhattanLength() >= QApplication::startDragDistance();

      switch (drag) {
            case DRAG_LASSO_START:
                  if (!isMoving)
                        break;
                  drag = DRAG_LASSO;
                  setCursor();
                  // proceed with DRAG_LASSO:

            case DRAG_LASSO:
                  lasso = QRect(start, QSize(dist.x(), dist.y()));
                  widget()->update();
                  break;

            case DRAG_MOVE_START:
            case DRAG_COPY_START:
                  if (!isMoving)
                        break;
            	{
      		bool shift = keyState & Qt::ShiftModifier;
      		bool ctrl  = keyState & Qt::ControlModifier;
                  if (shift && ctrl) {
                        if (std::abs(dist.x()) > std::abs(dist.y())) {
                              if (drag == DRAG_MOVE_START)
                                    drag = DRAGX_MOVE;
                              else
                                    drag = DRAGX_COPY;
                              }
                        else {
                              if (drag == DRAG_MOVE_START)
                                    drag = DRAGY_MOVE;
                              else
                                    drag = DRAGY_COPY;
                              }
                        }
                  else {
                        if (drag == DRAG_MOVE_START)
                              drag = DRAG_MOVE;
                        else
                              drag = DRAG_COPY;
                        }
                  setCursor();
                  if (!curItem->isSelected()) {
                        if (drag == DRAG_MOVE)
                              deselectAll();
                        selectItem(curItem, true);
                        updateSelection();
                        widget()->update();
                        }
                  for (iCItem i = items.begin(); i != items.end(); ++i) {
                        if (i->second->isSelected()) {
                              i->second->isMoving = true;
                              moving.add(i->second);
                              }
                        }
                  moveItems(pos, 0);
                  }
                  break;

            case DRAG_MOVE:
            case DRAG_COPY:
                  moveItems(pos, 0);
                  break;

            case DRAGX_MOVE:
            case DRAGX_COPY:
                  moveItems(pos, 1);
                  break;

            case DRAGY_MOVE:
            case DRAGY_COPY:
                  moveItems(pos, 2);
                  break;

            case DRAG_NEW:
            case DRAG_RESIZE:
                  if (dist.x()) {
      			Pos p1(curItem->bbox.x(), timeType());
      			Pos p2(pix2pos(pos.x()));
                        int w = p2.time(timeType()) - p1.time(timeType());
                  	if (w < 1)
                        	w = 1;
                        curItem->bbox.setWidth(w);
                        widget()->update();
                        }
                  break;
            case DRAG_DELETE:
                  deleteItem(pos);
                  break;

            case DRAG_OFF:
                  break;
            }
      }

//---------------------------------------------------------
//   mouseReleaseCanvasA
//---------------------------------------------------------

void EventCanvas::mouseReleaseCanvasA(QMouseEvent* me)
      {
      if (curItem && editor->playEvents()) {
	      MidiEvent e(0, 0, ME_NOTEON, curItem->event.pitch(), 0);
      	track()->playMidiEvent(&e);
            }
      // ignore event if (another) button is already active:

      if (me->buttons() & (Qt::LeftButton | Qt::RightButton | Qt::MidButton)
         & ~button)
            return;

      bool shift = keyState & Qt::ShiftModifier;
      bool ctrl  = keyState & Qt::ControlModifier;
      bool redrawFlag = false;

      switch (drag) {
            case DRAG_MOVE_START:
            case DRAG_COPY_START:
                  if (!(shift || ctrl))
                        deselectAll();
            	if (!ctrl)
                  	selectItem(curItem, !(shift && curItem->isSelected()));
                  updateSelection();
                  redrawFlag = true;
                  itemReleased();
                  break;
            case DRAG_COPY:
            case DRAGX_COPY:
            case DRAGY_COPY:
                  endMoveItems(MOVE_COPY);
                  break;
            case DRAG_MOVE:
            case DRAGX_MOVE:
            case DRAGY_MOVE:
                  endMoveItems(MOVE_MOVE);
                  break;
            case DRAG_OFF:
                  break;
            case DRAG_RESIZE:
                  resizeItem(curItem, false);
                  break;
            case DRAG_NEW:
                  newItem(curItem, false);
                  redrawFlag = true;
                  break;
            case DRAG_LASSO_START:
                  lasso.setRect(-1, -1, -1, -1);
                  if (!shift)
                        deselectAll();
                  updateSelection();
                  redrawFlag = true;
                  break;

            case DRAG_LASSO:
                  if (!shift)
                        deselectAll();
                  lasso = lasso.normalized();
                  selectLasso(shift);
                  updateSelection();
                  redrawFlag = true;
                  break;

            case DRAG_DELETE:
                  break;
            }
      drag = DRAG_OFF;
      if (redrawFlag)
            widget()->update();
      setCursor();
      }


//---------------------------------------------------------
//   endMoveItems
//---------------------------------------------------------

void EventCanvas::endMoveItems(DragType dragtype)
      {
      startUndo(dragtype);

      for (iCItem i = moving.begin(); i != moving.end(); ++i) {
            selectItem(i->second, true);
            moveItem(i->second, dragtype);
//TD            if (moving.size() == 1) {
//                  itemReleased();
//                  }
            if (dragtype == MOVE_COPY)
                  selectItem(i->second, false);
            }
      endUndo(dragtype);
      moving.clear();
      updateSelection();
      widget()->update();
      }

//---------------------------------------------------------
//   selectItem
//---------------------------------------------------------

void EventCanvas::selectItem(CItem* e, bool flag)
      {
      e->setSelected(flag);
      }

//---------------------------------------------------------
//   deselectAll
//---------------------------------------------------------

void EventCanvas::deselectAll()
      {
      for (iCItem i = items.begin(); i != items.end(); ++i)
            i->second->setSelected(false);
      }

//---------------------------------------------------------
//   genCanvasPopup
//---------------------------------------------------------

QMenu* EventCanvas::genCanvasPopup()
      {
      if (canvasTools == 0)
            return 0;
      QMenu* canvasPopup = new QMenu(this);

      for (int i = 0; i < 9; ++i) {
            int data = 1 << i;
            if ((canvasTools & data) == 0)
                  continue;
            QAction* a = canvasPopup->addAction(**toolList[i].icon, tr(toolList[i].tip));
            a->setData(data);
            a->setCheckable(true);
            if (data == int(_tool))
                  a->setChecked(true);
            }
      return canvasPopup;
      }

//---------------------------------------------------------
//   canvasPopup
//---------------------------------------------------------

void EventCanvas::canvasPopup(int n)
      {
      setTool(n);
      }

//---------------------------------------------------------
//   setCursor
//---------------------------------------------------------

void EventCanvas::setCursor()
      {
      if (curSplitter != -1) {
            widget()->setCursor(Qt::SplitVCursor);
            return;
            }
      switch (drag) {
            case DRAGX_MOVE:
            case DRAGX_COPY:
                  widget()->setCursor(QCursor(Qt::SizeHorCursor));
                  break;

            case DRAGY_MOVE:
            case DRAGY_COPY:
                  widget()->setCursor(QCursor(Qt::SizeVerCursor));
                  break;

            case DRAG_MOVE:
            case DRAG_COPY:
                  widget()->setCursor(QCursor(Qt::SizeAllCursor));
                  break;

            case DRAG_RESIZE:
                  widget()->setCursor(QCursor(Qt::SizeHorCursor));
                  break;

            case DRAG_DELETE:
            case DRAG_COPY_START:
            case DRAG_MOVE_START:
            case DRAG_NEW:
            case DRAG_LASSO_START:
            case DRAG_LASSO:
            case DRAG_OFF:
                  TimeCanvas::setCursor();
                  break;
            }
      }

//---------------------------------------------------------
//   deleteItem
//	p is relative to CanvasA
//---------------------------------------------------------

void EventCanvas::deleteItem(const QPoint& p)
      {
      Pos pos(pix2pos(p.x()));
      int pitch(y2pitch(p.y()));

// printf("%d %d  - %d %d\n", p.x(), p.y(), pos.tick(), pitch);

      pos -= *curPart;
      for (iCItem i = items.begin(); i != items.end(); ++i) {
            Event& event = i->second->event;
            if (event.pitch() == pitch
               && (pos >= event.pos())
               && (pos < event.end())) {
                  deleteItem(i->second);
                  break;
                  }
            }
      }

//---------------------------------------------------------
//   moveItems
//    dir = 0     move in all directions
//          1     move only horizontal
//          2     move only vertical
//---------------------------------------------------------

void EventCanvas::moveItems(const QPoint& pos, int dir)
      {
      int dy = pos.y() - start.y();

      Pos sp(pix2pos(start.x()));
      Pos cp(pix2pos(pos.x()));

      bool left = sp > cp;
      Pos dx(left ? sp - cp : cp - sp);

      if (dir == 2)
            dx.setTick(0);

      for (iCItem i = moving.begin(); i != moving.end(); ++i) {
            CItem* item = i->second;
            Pos p;
            if (left) {
                  //
                  // restrict movement to pos >= 0
                  //
                  if (dx > item->pos)
                        p.setTick(0);
                  else
	                  p = item->pos - dx;
                  }
            else
                  p = item->pos + dx;
            p.snap(raster());
            int ny = pitch2y(y2pitch(pitch2y(item->event.pitch()) + dy));

            if (p < *curPart)
                  p = *curPart;

            if (item->moving != p || (item->my - wpos.y()) != ny) {
                  item->moving = p;
                  if (dir != 1)
                        item->my = ny + wpos.y();
                  itemMoved(item);
                  }
            }
      widget()->update();
      }

//---------------------------------------------------------
//   selectionSize
//---------------------------------------------------------

int EventCanvas::selectionSize() const
      {
      int n = 0;
      for (ciCItem i = items.begin(); i != items.end(); ++i) {
            if (i->second->isSelected())
                  ++n;
            }
      return n;
      }

//---------------------------------------------------------
//   layout
//	called after resize
//---------------------------------------------------------

void EventCanvas::layout()
      {
      int n = ctrlEditList.size();
      if (n == 0)
            return;
      if (ctrlHeight == 0) {
            int wh = widget()->height();
            resizeController(wh < 120 ? wh / 2 : 100);
            }

      // check, if layout is ok already; this happens after
      // song load
      int h = 0;
      for (iCtrlEdit i = ctrlEditList.begin(); i != ctrlEditList.end(); ++i) {
            CtrlEdit* c = *i;
            h += c->height();
            }
      if (h == ctrlHeight) {
      	for (iCtrlEdit i = ctrlEditList.begin(); i != ctrlEditList.end(); ++i)
            	layoutPanelB(*i);
            return;
            }

      int y = 0;
      int sch = ctrlHeight / n;
      for (iCtrlEdit i = ctrlEditList.begin(); i != ctrlEditList.end(); ++i) {
            CtrlEdit* c = *i;
            c->y = y;
	      c->setHeight(sch);
      	layoutPanelB(c);
            y   += sch;
            }
      }

//---------------------------------------------------------
//   layout1
//	called after read song
//---------------------------------------------------------

void EventCanvas::layout1()
      {
      int n = ctrlEditList.size();
      if (n == 0)
            return;
      int y = 0;
      for (iCtrlEdit i = ctrlEditList.begin(); i != ctrlEditList.end(); ++i) {
            CtrlEdit* c = *i;
            c->y = y;
            y += c->height();
            }
      resizeController(y);
      }

//---------------------------------------------------------
//   layoutPanelB
//---------------------------------------------------------

void EventCanvas::layoutPanelB(CtrlEdit* c)
      {
      int y = c->y;
      int h = c->height();
      int bx = rPanelB.x() + rPanelB.width() - 23;
      int by = rPanelB.y() + y + h - 19;
      c->minus->setGeometry(bx, by, 18, 18);
      bx = rPanelB.x() + 1;
      by = rPanelB.y() + y + 5;
      c->sel->setGeometry(bx, by, rPanelB.width()-5, 18);
      }

//---------------------------------------------------------
//   addController
//---------------------------------------------------------

void EventCanvas::addController()
      {
      int n = ctrlEditList.size();
      CtrlEdit* ce = new CtrlEdit(widget(), this, track());
      ce->setHeight(50);
      ctrlEditList.push_back(ce);

      ce->minus->defaultAction()->setData(n);
      connect(ce->minus, SIGNAL(triggered(QAction*)), SLOT(removeController(QAction*)));
      ce->minus->show();
      ce->sel->show();

      layout();
      widget()->update();
      updatePartControllerList();
      }

void EventCanvas::addController(int id, int h)
      {
      ctrlHeight += h;
      int n = ctrlEditList.size();
      CtrlEdit* ce = new CtrlEdit(widget(), this, track());
      ce->setHeight(h);
      Ctrl* ctrl = track()->getController(id);
      if (ctrl)
	      ce->setCtrl(ctrl);
      ctrlEditList.push_back(ce);

      ce->minus->defaultAction()->setData(n);
      connect(ce->minus, SIGNAL(triggered(QAction*)), SLOT(removeController(QAction*)));
      }

//---------------------------------------------------------
//   paintVLine
//---------------------------------------------------------

void paintVLine(QPainter& p, int y1, int y2, int x)
      {
      static QColor color[splitWidth] = {
            QColor(0x55, 0x55, 0x52),
            QColor(0xff, 0xff, 0xff),
            };
      x -= 2;
      for (int i = 0; i < 2; ++i) {
            p.setPen(color[i]);
            p.drawLine(x, y1, x, y2);
            ++x;
            }
      }

//---------------------------------------------------------
//   paintControllerCanvas
//    r(0, 0) is PanelB topLeft()
//---------------------------------------------------------

void EventCanvas::paintControllerCanvas(QPainter& p, QRect r)
      {
      int x1 = r.x();
      int x2 = x1 + r.width();

      int xx2 = rCanvasB.width();
      if (xx2 >= x2)
            x2 = xx2 - 2;
      for (iCtrlEdit i = ctrlEditList.begin(); i != ctrlEditList.end(); ++i) {
            CtrlEdit* c = *i;
            int         y = c->y;
            paintHLine(p, x1, x2, y);
            p.setPen(lineColor[0]);
            p.drawLine(xx2-1, 1, xx2-1, splitWidth-2);

            QRect  rc(0, y + splitWidth, rCanvasB.width(), c->cheight());
            QPoint pt(rc.topLeft());
            rc &= r;
            if (!rc.isEmpty()) {
                  p.translate(pt);
                  c->paint(p, rc.translated(-pt));
                  p.translate(-pt);
                  }
            }
      }

//---------------------------------------------------------
//   paintControllerPanel
//	panelB
//---------------------------------------------------------

void EventCanvas::paintControllerPanel(QPainter& p, QRect r)
      {
      p.fillRect(r, QColor(0xe0, 0xe0, 0xe0));
      int x1 = r.x();
      int x2 = x1 + r.width();

      paintVLine(p, r.y() + splitWidth, r.y() + r.height(),
         rPanelB.x() + rPanelB.width());

      if (x1 == 0)
            x1 = 1;
      for (iCtrlEdit i = ctrlEditList.begin(); i != ctrlEditList.end(); ++i) {
            CtrlEdit* c = *i;
            paintHLine(p, x1, x2, c->y);
            p.setPen(lineColor[0]);
            p.drawLine(0, 1, 0, splitWidth-2);
            }
      }

//---------------------------------------------------------
//   removeController
//---------------------------------------------------------

void EventCanvas::removeController(QAction* a)
      {
      int id = a->data().toInt();

      int k = 0;
      for (iCtrlEdit i = ctrlEditList.begin(); i != ctrlEditList.end(); ++i, ++k) {
            if (k == id) {
                  CtrlEdit* c = *i;
                  delete c;
                  ctrlEditList.erase(i);
                  break;
                  }
            }
      k = 0;
      for (iCtrlEdit i = ctrlEditList.begin(); i != ctrlEditList.end(); ++i, ++k) {
            CtrlEdit* c = *i;
            c->minus->defaultAction()->setData(k);
            }

      if (ctrlEditList.empty())
            resizeController(0);
      else
            layout();
      widget()->update();
      updatePartControllerList();
      }

//---------------------------------------------------------
//   enterB
//---------------------------------------------------------

void EventCanvas::enterB()
      {
      if ((button == 0) && curSplitter != -1) {
            curSplitter = -1;
            setCursor();
            }
      }

//---------------------------------------------------------
//   leaveB
//---------------------------------------------------------

void EventCanvas::leaveB()
      {
      if ((button == 0) && (curSplitter != -1)) {
            curSplitter = -1;
            setCursor();
            }
      }

//---------------------------------------------------------
//   noteOn
//---------------------------------------------------------

void EventCanvas::noteOn(int pitch, int velocity, bool shift)
      {
      DrumMap* dm = track()->drumMap();
      if (!dm)
	      pitch += track()->transposition;

      // play note:
      if (editor->playEvents()) {
	      MidiEvent e(0, 0, ME_NOTEON, pitch, velocity);
      	track()->playMidiEvent(&e);
            }

      if (curPart && editor->stepRec()
         && pos[0].tick() >= startTick
         && pos[0].tick() < endTick) {
            int len  = editor->quant();
            unsigned tick = pos[0].tick() - curPart->tick(); //CDW
            if (shift)
                  tick -= editor->rasterStep(tick);
            Event e(Note);
            e.setTick(tick);
            e.setPitch(pitch);
            e.setVelo(127);
            e.setLenTick(len);
            audio->msgAddEvent(e, curPart);
            tick += editor->rasterStep(tick) + curPart->tick();
            if (tick != song->cpos()) {
                  Pos p(tick, AL::TICKS);
                  song->setPos(0, p, true, false, true);
                  }
            }
      }

//---------------------------------------------------------
//   noteOff
//---------------------------------------------------------

void EventCanvas::noteOff(int pitch)
      {
      if (!editor->playEvents())
            return;
      DrumMap* dm = track()->drumMap();
      if (!dm)
	      pitch += track()->transposition;

      // release key:
      MidiEvent e(0, 0, ME_NOTEON, pitch, 0);
      track()->playMidiEvent(&e);
      }

//---------------------------------------------------------
//   mouseDoubleClick
//---------------------------------------------------------

void EventCanvas::mouseDoubleClick(QMouseEvent* me)
      {
	mousePress(me);
      }

//---------------------------------------------------------
//   mousePress
//---------------------------------------------------------

void EventCanvas::mousePress(QMouseEvent* me)
      {
      QPoint pos(me->pos());
      if (rPanelA.contains(pos)) {
            bool shift = keyState & Qt::ShiftModifier;
            if (keyDown != -1) {
                  noteOff(keyDown);
                  keyDown = -1;
                  }
            int y = pos.y() - rCanvasA.y();
	    keyDown = y2pitch(y);
            int velocity = me->x()*127/40;
            if (keyDown != -1)
                  noteOn(keyDown, velocity, shift);
            return;
            }
      if (rCanvasA.contains(pos)) {
            mousePressCanvasA(me);
            return;
            }
      if (curSplitter != -1) {
            dragSplitter = true;
            splitterY = pos.y();
            return;
            }
      if (rCanvasB.contains(pos)) {
            for (iCtrlEdit i = ctrlEditList.begin(); i != ctrlEditList.end(); ++i) {
                  CtrlEdit* c = *i;
                  QRect r(rCanvasB.x(), rCanvasB.y() + c->y + splitWidth,
                     rCanvasB.width(), c->cheight());
                  if (r.contains(pos)) {
                        c->mousePress(pos - r.topLeft(), me->button());
                        break;
                        }
                  }
            }
      }

//---------------------------------------------------------
//   mouseMove
//---------------------------------------------------------

void EventCanvas::mouseMove(QPoint pos)
      {
      if (rPanelA.contains(pos) || rCanvasA.contains(pos)) {
            int y = pos.y() - rCanvasA.y();
            int pitch = y2pitch(y);
            if (curPitch != pitch) {
                  curPitch = pitch;
                  widget()->update(rPanelA);    // update keyboard
                  emit pitchChanged(curPitch);
                  if (button != Qt::NoButton) {
                        if (keyDown != -1 && curPitch != -1) {
                              bool shift = keyState & Qt::ShiftModifier;
                              if (curPitch != keyDown)
                                    noteOff(keyDown);
                              keyDown = curPitch;
			      int velocity = pos.x()*127/40;
                              noteOn(keyDown, velocity, shift);
                              }
                        }
                  }
            }
      if (dragSplitter) {
            int deltaY = pos.y() - splitterY;

            iCtrlEdit i = ctrlEditList.begin();
            int y = 0;
            if (curSplitter > 0) {
                  int k = 0;
                  CtrlEdit* c;
                  for (; i != ctrlEditList.end(); ++i, ++k) {
                        c = *i;
                        y += c->height();
                        if ((k+1) == curSplitter)
                              break;
                        }
                  if (i == ctrlEditList.end()) {
                        printf("unexpected edit list end, curSplitter %d\n", curSplitter);
                        return;
                        }
                  if (c->height() + deltaY < splitWidth)
                        deltaY = splitWidth - c->height();
                  ++i;
                  int rest = 0;
                  for (iCtrlEdit ii = i; ii != ctrlEditList.end(); ++ii)
                        rest += (*ii)->cheight();
                  if (rest < deltaY)
                        deltaY = rest;
                  c->setHeight(c->height() + deltaY);
                  layoutPanelB(c);
                  y += deltaY;
                  }
            //
            //    layout rest, add deltaY vertical
            //
            int rest = 0;
            for (iCtrlEdit ii = i; ii != ctrlEditList.end(); ++ii) {
                  CtrlEdit* c = *ii;
                  rest += c->cheight();
                  }
            if (rest < deltaY)
                  deltaY = rest;
            rest = deltaY;
            for (; i != ctrlEditList.end(); ++i) {
                  CtrlEdit* c = *i;
                  int d = c->cheight();
                  if (d > deltaY)
                        d = deltaY;
	            c->setHeight(c->height() - d);
      	      c->y = y;
                  layoutPanelB(c);
              	y += c->height();
                  deltaY -= d;
                  if (deltaY == 0)
                        break;
                  }
            if (i != ctrlEditList.end())
                  ++i;
            for (; i != ctrlEditList.end(); ++i) {
                  CtrlEdit* c = *i;
	            c->y = y;
                  y += c->height();
                  }
            if (curSplitter == 0)
                  resizeController(ctrlHeight - rest);
            else
                  widget()->update(rPanelB | rCanvasB);
            splitterY = pos.y();
      	updatePartControllerList();
            return;
            }
      if (rCanvasA.contains(pos)) {
            mouseMoveCanvasA(pos - rCanvasA.topLeft());
            return;
            }
      if (button == 0) {
            if (rPanelB.contains(pos) || rCanvasB.contains(pos)) {
                  int y = pos.y() - rPanelB.y();
                  int k = 0;
                  for (iCtrlEdit i = ctrlEditList.begin(); i != ctrlEditList.end(); ++i, ++k) {
                        CtrlEdit* c = *i;
                        if (y >= c->y && y < (c->y + splitWidth)) {
                              curSplitter = k;
                              setCursor();
                              return;
                              }
                        int ypos = y - c->y - splitWidth;
                        if (ypos >= 0)
                              emit yChanged(c->pixel2val(ypos));
                        }
                  }
            if (curSplitter != -1) {
                  curSplitter = -1;
                  setCursor();
                  }
            return;
            }
      if (rCanvasB.contains(pos)) {
            for (iCtrlEdit i = ctrlEditList.begin(); i != ctrlEditList.end(); ++i) {
                  CtrlEdit* c = *i;
                  QRect r(rCanvasB.x(), rCanvasB.y() + c->y + splitWidth,
                     rCanvasB.width(), c->cheight());
                  if (r.contains(pos)) {
                        c->mouseMove(pos - r.topLeft());
                        break;
                        }
                  }
            }
      }

//---------------------------------------------------------
//   mouseRelease
//---------------------------------------------------------

void EventCanvas::mouseRelease(QMouseEvent* me)
      {
      if (keyDown != -1) {
            noteOff(keyDown);
            keyDown = -1;
            }
      if (dragSplitter) {
            dragSplitter = false;
            return;
            }
      QPoint pos(me->pos());
      if (rCanvasA.contains(pos)) {
            mouseReleaseCanvasA(me);
            return;
            }
      if (rCanvasB.contains(pos)) {
            for (iCtrlEdit i = ctrlEditList.begin(); i != ctrlEditList.end(); ++i) {
                  CtrlEdit* c = *i;
                  QRect r(rCanvasB.x(), rCanvasB.y() + c->y + splitWidth,
                     rCanvasB.width(), c->cheight());
                  if (r.contains(pos)) {
                        c->mouseRelease();
                        break;
                        }
                  }
            }
      }

//---------------------------------------------------------
//   setCurPart
//---------------------------------------------------------

void EventCanvas::setCurPart(Part* part)
      {
      curPart = part;

      if (curPart->raster() != -1)
            editor->setRaster(curPart->raster());
      if (curPart->quant() != -1)
            editor->setQuant(curPart->quant());
      if (curPart->xmag() != 0.0)
            editor->setXmag(curPart->xmag());
      curPart->setRaster(editor->raster());
      curPart->setQuant(editor->quant());
      curPart->setXmag(editor->xmag());
      setPart(*curPart, curPart->end());
      editor->setWindowTitle(getCaption());
      }

//---------------------------------------------------------
//   getTextDrag
//---------------------------------------------------------

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
            CItem* ne = i->second;
            Event    e = ne->event;
            if (startTick == MAXINT)
                  startTick = e.tick();
            el.add(e);
            }
      QBuffer buffer;
      buffer.open(QIODevice::WriteOnly);
      Xml xml(&buffer);
      xml.header();

      xml.tag("eventlist");
      for (ciEvent e = el.begin(); e != el.end(); ++e)
            e->second.write(xml, -startTick);
      xml.etag("eventlist");

      QByteArray data = buffer.buffer();
      QMimeData* drag = new QMimeData;
      drag->setData("text/x-muse-eventlist", data);
      buffer.close();
      return drag;
      }

//---------------------------------------------------------
//   pasteAt
//---------------------------------------------------------

void EventCanvas::pasteAt(const QString& pt, unsigned pos)
      {
      QDomDocument doc;

      if (!doc.setContent(pt, false)) {
            printf("MusE:pasteAt(): syntax error\n");
            printf(">>%s<<\n", pt.toLatin1().data());
            return;
            }
      for (QDomNode node = doc.documentElement(); !node.isNull(); node = node.nextSibling()) {
            QDomElement e = node.toElement();
            if (e.isNull())
                  continue;
            if (e.tagName() == "eventlist") {
                  EventList el;
                  el.read(node.firstChild(), true);
                  song->startUndo();
                  for (iEvent i = el.begin(); i != el.end(); ++i) {
                        Event e = i->second;
                        e.setTick(e.tick() + pos - curPart->tick());
                        audio->msgAddEvent(e, curPart, false);
                        }
                  song->endUndo(SC_EVENT_INSERTED);
                  }
            else
                  printf("MusE:pasteAt(): tag %s not supported\n", e.tagName().toLatin1().data());
            }
      }

//---------------------------------------------------------
//   midiNote
//---------------------------------------------------------

void EventCanvas::midiNote(const MidiEvent& me)
      {
      if (me.type() != ME_NOTEON)
            return;
      int pitch = me.dataA();
      int velo  = me.dataB();

      if (editor->midiIn() && editor->stepRec() && curPart
         && !audio->isPlaying() && velo && pos[0].tick() >= startTick
         && pos[0].tick() < endTick
         && !(keyState & Qt::AltModifier)) {
            int len            = editor->quant();
            unsigned tick      = pos[0].tick(); //CDW
            unsigned starttick = tick;
            if (keyState & Qt::ShiftModifier)
                  tick -= editor->rasterStep(tick);

            //
            // extend len of last note?
            //
            EventList* events = curPart->events();
            if (keyState & Qt::ControlModifier) {
                  for (iEvent i = events->begin(); i != events->end(); ++i) {
                        Event ev = i->second;
                        if (!ev.isNote())
                              continue;
                        if (ev.pitch() == pitch && ((ev.tick() + ev.lenTick()) == starttick)) {
                              Event e = ev.clone();
                              e.setLenTick(ev.lenTick() + editor->rasterStep(starttick));
                              audio->msgChangeEvent(ev, e, curPart);
                              tick += editor->rasterStep(tick);
                              if (tick != song->cpos()) {
                                    Pos p(tick, AL::TICKS);
                                    song->setPos(0, p, true, false, true);
                                    }
                              return;
                              }
                        }
                  }

            //
            // if we already entered the note, delete it
            //
            EventRange range = events->equal_range(tick);
            for (iEvent i = range.first; i != range.second; ++i) {
                  Event ev = i->second;
                  if (ev.isNote() && ev.pitch() == pitch) {
                        audio->msgDeleteEvent(ev, curPart);
                        if (keyState & Qt::ShiftModifier)
                              tick += editor->rasterStep(tick);
                        return;
                        }
                  }
            Event e(Note);
            e.setTick(tick - curPart->tick());
            e.setPitch(pitch);
            e.setVelo(velo);
            e.setLenTick(len);
            audio->msgAddEvent(e, curPart);
            tick += editor->rasterStep(tick);
            if (tick != song->cpos()) {
                  Pos p(tick, AL::TICKS);
                  song->setPos(0, p, true, false, true);
                  }
            }
      }

//---------------------------------------------------------
//   magChanged
//---------------------------------------------------------

void EventCanvas::magChanged()
      {
      if (part()) {
            part()->setXmag(_xmag);
            }
      }

//---------------------------------------------------------
//   updatePartControllerList
//---------------------------------------------------------

void EventCanvas::updatePartControllerList()
      {
	if (curPart == 0)
      	return;
      CtrlCanvasList* cl = curPart->getCtrlCanvasList();
      cl->clear();
      for (iCtrlEdit i = ctrlEditList.begin(); i != ctrlEditList.end(); ++i) {
            CtrlCanvas cc;
            cc.ctrlId = (*i)->ctrlId;
            cc.height = (*i)->height();
            cl->push_back(cc);
            }
      }

