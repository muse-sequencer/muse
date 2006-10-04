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

#include "ctrleditor.h"
#include "track.h"
#include "awl/tcanvas.h"
#include "ctrl.h"
#include "midictrl.h"
#include "gconfig.h"
#include "song.h"
#include "part.h"
#include "tools.h"
#include "muse.h"
#include "gui.h"

static const int HANDLE1 = 6;
static const int HANDLE2 = 3;
static const int veloWidth = 3;

//---------------------------------------------------------
//   CtrlEditor
//---------------------------------------------------------

CtrlEditor::CtrlEditor()
      {
      _drawCtrlName = false;
      dragy         = -1;
      lselected     = 0;
      singlePitch   = -1;
      drawRuler     = false;
      }

//---------------------------------------------------------
//   drawHandle
//---------------------------------------------------------

inline static void drawHandle(QPainter& p, int x, int y, int lselected)
      {
      p.fillRect(x-HANDLE2, y-HANDLE2, HANDLE1, HANDLE1,
         x == lselected ? Qt::red : Qt::yellow);
      }

//---------------------------------------------------------
//   paint
//---------------------------------------------------------

void CtrlEditor::paint(QPainter& p, const QRect& r)
      {
      if (!ctrl())
            return;
      int from = r.x() - HANDLE1;
      if (from < 0)
            from = 0;
      int to = r.x() + r.width() + HANDLE1;
      int th = cheight();

      p.save();
      p.setRenderHint(QPainter::Antialiasing, true);
      bool aR = ctrlTrack()->autoRead();
      p.setPen(QPen(aR ? Qt::white : Qt::gray, 2));

      TType tt = track()->timeType();

      MidiController::ControllerType type = midiControllerType(ctrl()->id());
      if (type == MidiController::Velo) {
            p.setPen(QPen(Qt::blue, veloWidth));
            PartList* pl = track()->parts();
            for (iPart ip = pl->begin(); ip != pl->end(); ++ip) {
                  Part* part = ip->second;
                  int pos1   = tc()->pos2pix(*part);
                  int pos2   = tc()->pos2pix(part->end());

                  if (pos2 <= from)
                        continue;
                  if (pos1 > to)
                        break;

                  EventList* events = part->events();
                  for (iEvent e = events->begin(); e != events->end(); ++e) {
                        if (e->second.type() != Note)
                              continue;
                        int pos = tc()->pos2pix(e->second.pos() + *part);
                        if (pos <= from)
                              continue;
                        if (pos > to)
                              break;
                        int id = ctrl()->id();
                        if (id == CTRL_VELOCITY ||
                          (CTRL_SVELOCITY && e->second.pitch() == singlePitch)) {
                        	int y1 = ctrl()->val2pixelR(e->second.velo(), th);
                        	p.drawLine(pos, th, pos, y1);
                        	}
                        }
                  }
            }
      else {
            if (!ctrl()->empty()) {
                  int x1 = from, y1 = 0, x2 = 0, y2 = 0;
                  ciCtrlVal i = ctrl()->begin();
                  if (i != ctrl()->end()) {
                        x1 = tc()->pos2pix(Pos(i->first, tt));
                        if (dragy != -1 && lselected == x1)
                              y1 = dragy;
                        else
                              y1 = ctrl()->val2pixelR(i->second, th);
                        if (x1 >= from)
                              drawHandle(p, x1, y1, lselected);
                        }
                  for (; i != ctrl()->end(); ++i) {
                        x2 = tc()->pos2pix(Pos(i->first, tt));
                        if (dragy != -1 && lselected == x2)
                              y2 = dragy;
                        else {
                              y2 = ctrl()->val2pixelR(i->second, th);
                              }
                        if (x2 >= to)
                              break;
                        if (x2 >= from) {
                              if (ctrl()->type() & Ctrl::DISCRETE) {
                                    p.drawLine(x1, y1, x2, y1);
                                    p.drawLine(x2, y1, x2, y2);
                                    }
                              else
                                    p.drawLine(x1, y1, x2, y2);
                              drawHandle(p, x1, y1, lselected);
                              }
                        x1 = x2;
                        y1 = y2;
                        }
                  if (x1 < to) {
                        if (i == ctrl()->end())
                              x2 = to;
                        if (ctrl()->type() & Ctrl::DISCRETE)
                              y2 = y1;
                        p.drawLine(x1, y1, x2, y2);
                        drawHandle(p, x1, y1, lselected);
                        }
                  }
            if (!aR) {
                  p.setPen(QPen(Qt::white, 2));
                  int y = ctrl()->val2pixelR(ctrl()->schedValRaw(), th);
                  p.drawLine(r.x(), y, r.x() + r.width(), y);
                  }
            }

      if (drawRuler) {
            p.setPen(QPen(Qt::white, 2));
      	p.setRenderHint(QPainter::Antialiasing, true);
            p.drawLine(ruler1, ruler2);
            }
      //
      // this does not work well a.t.m:
      //
      if (_drawCtrlName) {
            QString s(ctrl()->name());
            p.setFont(*config.fonts[3]);
            p.setPen(Qt::black);
            QFontMetrics fm(*config.fonts[3]);
            int ly = fm.lineSpacing() + 2;
            p.drawText(2, ly, s);
            }
      p.restore();
      }

//---------------------------------------------------------
//   mousePress
//---------------------------------------------------------

void CtrlEditor::mousePress(const QPoint& pos, int button)
      {
      Tool tool = tc()->tool();
      if (button & Qt::RightButton) {
            QMenu* pop = new QMenu(tc());
            QAction* a;
            for (int i = 0; i < TOOLS; ++i) {
                  if ((arrangerTools & (1 << i)) == 0)
                        continue;
                  a = pop->addAction(**toolList[i].icon, tc()->tr(toolList[i].tip));
                  int id = 1 << i;
                  a->setData(id);
                  a->setCheckable(true);
                  if (id == (int)tool)
                        a->setChecked(true);
                  }
            a = pop->exec(tc()->mapToGlobal(pos));
            if (a) {
                  int n = a->data().toInt();
                  // tc()->setTool(n);
                  muse->setTool(n);
                  }
            return;
            }

      int wh = cheight();
      int y  = pos.y();
      int x  = pos.x();

      int cid   = ctrl()->id();

      if (tool == PencilTool) {
            selected  = tc()->pix2pos(x);
            lselected = x;
            dragy     = y;
            dragx     = x;
            dragYoffset = 0;
            if (cid == CTRL_VELOCITY || cid == CTRL_SVELOCITY)
                  song->startUndo();
            else {
                  // add controller:
                  CVal val = ctrl()->pixel2val(dragy, wh);
                  song->addControllerVal(ctrlTrack(), ctrl(), selected, val);
                  tc()->widget()->update();
                  }
            }
      else if (tool == PointerTool || tool == RubberTool) {
      	if (cid == CTRL_VELOCITY || cid == CTRL_SVELOCITY) {
                  dragx = x;
                  song->startUndo();
                  return;
                  }

            Pos pos1(tc()->pix2pos(x - HANDLE2));
            Pos pos2(tc()->pix2pos(x + HANDLE2));

            TType tt = track()->timeType();
            ciCtrlVal s = ctrl()->upper_bound(pos1.time(tt));
            ciCtrlVal e = ctrl()->upper_bound(pos2.time(tt));
            for (ciCtrlVal i = s; i != e; ++i) {
                  int yy = ctrl()->val2pixelR(i->second, wh);
                  startY = yy;
                  if ((yy >= (y-HANDLE2)) && (yy < (y + HANDLE2))) {
                        if (tt == AL::TICKS)
                              selected.setTick(i->first);
                        else
                              selected.setFrame(i->first);
                        lselected = tc()->pos2pix(selected);
                        if (tool == RubberTool || button == Qt::RightButton) {
                              song->removeControllerVal(ctrlTrack(), ctrl()->id(), i->first);
                              dragy = -1;
                              }
                        else {
                              dragy = yy;
                              dragYoffset = dragy - y;
                              }
                        tc()->widget()->update();
                        break;
                        }
                  }
            }
      else if (tool == DrawTool) {
            ruler1 = ruler2 = pos;
            drawRuler = true;
            }
      }

//---------------------------------------------------------
//   mouseRelease
//---------------------------------------------------------

void CtrlEditor::mouseRelease()
      {
      if (tc()->tool() == DrawTool) {
            song->startUndo();
            drawRuler = false;
            int from, to, y1, y2;
            if (ruler1.x() > ruler2.x()) {
                  from = ruler2.x();
                  to = ruler1.x();
            	y1 = ruler2.y();
            	y2 = ruler1.y();
            	}
      	else {
            	from = ruler1.x();
            	to   = ruler2.x();
            	y1   = ruler1.y();
            	y2   = ruler2.y();
            	}
      	MidiController::ControllerType type = midiControllerType(ctrl()->id());
      	if (type == MidiController::Velo) {
            	PartList* pl = track()->parts();
            	for (iPart ip = pl->begin(); ip != pl->end(); ++ip) {
                  	Part* part = ip->second;
	                  int pos1   = tc()->pos2pix(*part);
      	            int pos2   = tc()->pos2pix(part->end());

            	      if (pos2 <= from)
                  	      continue;
	                  if (pos1 > to)
      	                  break;

	                  EventList* events = part->events();
      	            for (iEvent e = events->begin(); e != events->end(); ++e) {
	                        if (e->second.type() != Note)
      	                        continue;
	                        int pos = tc()->pos2pix(e->second.pos() + *part);
      	                  if (pos <= from)
	                              continue;
      	                  if (pos > to)
	                              break;
	                        int id = ctrl()->id();
      	                  if (id == CTRL_VELOCITY ||
	                          (CTRL_SVELOCITY && e->second.pitch() == singlePitch)) {
                                	int y = y1 + (y2 - y1) * (pos - from) / (to - from);
            				int val = (ctrl()->pixel2val(y, cheight())).i;
                  			Event clone = e->second.clone();
                  			clone.setB(val);
                  			song->changeEvent(e->second, clone, part);
                  			song->undoOp(UndoOp::ModifyEvent, clone, e->second, part);
                  	      	}
                        	}
	                  }
                  }
            else {
                  printf("ctrleditor:: not implemented\n");
                  }
            song->endUndo(SC_EVENT_MODIFIED);
         	tc()->widget()->update();
            return;
            }
      if (ctrl()->id() == CTRL_VELOCITY || ctrl()->id() == CTRL_SVELOCITY)
            song->endUndo(SC_EVENT_MODIFIED);
      else {
            if (dragy != -1 && dragy != startY) {
                  int wh   = cheight();
                  CVal val = ctrl()->pixel2val(dragy, wh);
                  // modify controller:
                  song->addControllerVal(ctrlTrack(), ctrl(), selected, val);
                  }
            dragy = -1;
            }
      }

//---------------------------------------------------------
//   mouseMove
//     called only with mouse button pressed
//---------------------------------------------------------

void CtrlEditor::mouseMove(const QPoint& pos)
      {
      Tool tool = tc()->tool();
      int cid   = ctrl()->id();

      if (tool == DrawTool) {
            ruler2 = pos;
         	tc()->widget()->update();
            return;
            }
      if (cid == CTRL_VELOCITY || cid == CTRL_SVELOCITY) {
            AL::Pos p1(tc()->pix2pos(dragx));
            AL::Pos p2(tc()->pix2pos(pos.x()));
            dragx = pos.x();

            int wh  = cheight();
            int val = (ctrl()->pixel2val(pos.y(), wh)).i;
            unsigned tick1 = p1.tick();
            Part* part     = track()->parts()->findPart(tick1);
            if (part == 0)
                  return;
            unsigned tick2 = p2.tick();
            if (tick1 > tick2) {
                  int tmp = tick2;
                  tick2 = tick1;
                  tick1 = tmp;
                  }
            if (part->tick() > tick1)
                  tick1 = 0;
            else
                  tick1 -= part->tick();   // event time is relative to part
            if (part->tick() > tick2)
                  tick2 = 0;
            else
                  tick2 -= part->tick();

            //
            // change velocity for all notes at tick time
            //
            EventList* el = part->events();
            EventList tel;
            for (iEvent ie = el->lower_bound(tick1); ie != el->lower_bound(tick2); ie++) {
                  Event e = ie->second;
                  if (!e.isNote() || e.dataB() == val)
                        continue;
                  tel.add(e);
                  }
            for (iEvent ie = tel.begin(); ie != tel.end(); ++ie) {
                  Event e     = ie->second;
                  Event clone = e.clone();
                  clone.setB(val);
                  song->changeEvent(e, clone, part);
                  song->undoOp(UndoOp::ModifyEvent, clone, e, part);
                  }
            }
      else {
            if (tool == RubberTool) {
                  int wh = cheight();
                  int y  = pos.y();
                  int x  = pos.x();

                  Pos pos1(tc()->pix2pos(x - HANDLE2));
                  Pos pos2(tc()->pix2pos(x + HANDLE2));

                  TType tt = track()->timeType();
                  ciCtrlVal s = ctrl()->upper_bound(pos1.time(tt));
                  ciCtrlVal e = ctrl()->upper_bound(pos2.time(tt));
                  for (ciCtrlVal i = s; i != e; ++i) {
                        int yy = ctrl()->val2pixelR(i->second, wh);
                        startY = yy;
                        if ((yy >= (y-HANDLE2)) && (yy < (y + HANDLE2))) {
                              if (tt == AL::TICKS)
                                    selected.setTick(i->first);
                              else
                                    selected.setFrame(i->first);
                              lselected = tc()->pos2pix(selected);
                              song->removeControllerVal(ctrlTrack(), ctrl()->id(), i->first);
                              dragy = -1;
                              break;
                              }
                        }
                  }
            if (dragy != -1) {
                  dragy = pos.y() + dragYoffset;
                  if (dragy < 0)
                        dragy = 0;
                  else if (dragy > cheight())
                        dragy = cheight();
                  }
            }
      tc()->widget()->update();
      }
