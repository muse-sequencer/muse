//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: master.cpp,v 1.3 2004/04/11 13:03:32 wschweer Exp $
//  (C) Copyright 2000 Werner Schweer (ws@seh.de)
//  (C) Copyright 2012 Tim E. Real (terminator356 on users dot sourceforge dot net)
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
#include <limits.h>

#include <QCursor>
#include <QEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QPoint>
#include <QList>
#include <QPair>
#include <QRect>
#include <QToolBar>
#include <QMenu>

#include "globals.h"
#include "master.h"
#include "song.h"
#include "scrollscale.h"
#include "midi_consts.h"
#include "midieditor.h"
#include "icons.h"
#include "audio.h"
#include "gconfig.h"
#include "shortcuts.h"
#include "menutitleitem.h"
#include "masteredit.h"

namespace MusEGui {

//---------------------------------------------------------
//   Master
//---------------------------------------------------------

Master::Master(MidiEditor* e, QWidget* parent, int xmag, int ymag)
   : View(parent, xmag, ymag)
      {
      editor = e;
      setBg(MusEGlobal::config.midiCanvasBg);
      vscroll = 0;
      pos[0]  = MusEGlobal::song->cpos();
      pos[1]  = MusEGlobal::song->lpos();
      pos[2]  = MusEGlobal::song->rpos();
      drag = DRAG_OFF;
      setTool(MusEGui::PencilTool);
      setFocusPolicy(Qt::StrongFocus);
      setMouseTracking(true);
      connect(MusEGlobal::song, SIGNAL(posChanged(int, unsigned, bool)), this, SLOT(setPos(int, unsigned, bool)));
      connect(MusEGlobal::song, SIGNAL(songChanged(MusECore::SongChangedStruct_t)), this, SLOT(songChanged(MusECore::SongChangedStruct_t)));
      }

//---------------------------------------------------------
//   songChanged
//---------------------------------------------------------

void Master::songChanged(MusECore::SongChangedStruct_t type)
{
  //if(_isDeleting) return; // todo: If things get complicated don't forget some mechanism to ignore while while deleting to prevent possible crash.
  
  if (type & (SC_SIG | SC_TEMPO | SC_KEY ))  // TEST: Reasonable to start with, may need more.
    redraw();
}

//---------------------------------------------------------
//   setPos
//---------------------------------------------------------

void Master::setPos(int idx, unsigned val, bool adjustScrollbar)
      {
      if (pos[idx] == val)
            return;

      int opos = mapx(pos[idx]);
      int npos = mapx(val);

      if (adjustScrollbar && idx == 0) {
            switch (MusEGlobal::song->follow()) {
                  case  MusECore::Song::NO:
                        break;
                  case MusECore::Song::JUMP:
                        if (npos >= width()) {
                              int ppos =  val - rmapxDev(width()/8);
                              if (ppos < 0)
                                    ppos = 0;
                              emit followEvent(ppos);
                              opos = mapx(pos[idx]);
                              npos = mapx(val);
                              }
                        else if (npos < 0) {
                              int ppos =  val - rmapxDev(width()*3/4);
                              if (ppos < 0)
                                    ppos = 0;
                              emit followEvent(ppos);
                              opos = mapx(pos[idx]);
                              npos = mapx(val);
                              }
                        break;
                  case MusECore::Song::CONTINUOUS:
                        if (npos > (width()/2)) {
                              int ppos =  pos[idx] - rmapxDev(width()/2);
                              if (ppos < 0)
                                    ppos = 0;
                              emit followEvent(ppos);
                              opos = mapx(pos[idx]);
                              npos = mapx(val);
                              }
                        else if (npos < (width()/2)) {
                              int ppos =  pos[idx] - rmapxDev(width()/2);
                              if (ppos < 0)
                                    ppos = 0;
                              emit followEvent(ppos);
                              opos = mapx(pos[idx]);
                              npos = mapx(val);
                              }
                        break;
                  }
            }

      int x;
      int w = 1;
      if (opos > npos) {
            w += opos - npos;
            x = npos;
            }
      else {
            w += npos - opos;
            x = opos;
            }
      pos[idx] = val;
      redraw(QRect(x-1, 0, w+2, height()));
      }

//---------------------------------------------------------
//   leaveEvent
//---------------------------------------------------------

void Master::leaveEvent(QEvent*)
      {
      emit tempoChanged(-1);
      emit timeChanged(INT_MAX);
      }

//---------------------------------------------------------
//   pdraw
//---------------------------------------------------------

void Master::pdraw(QPainter& p, const QRect& rect, const QRegion&)
      {
      View::pdraw(p, rect);   // calls draw()
      p.resetTransform();

      int x = rect.x();
      int y = rect.y();
      int w = rect.width() + 2;
      int h = rect.height();

      int wh = height();

      QColor graph_fg_color = MusEGlobal::config.ctrlGraphFg;
      graph_fg_color.setAlpha(MusEGlobal::config.globalAlphaBlend);
      QPen pen;
      pen.setCosmetic(true);

      //---------------------------------------------------
      // draw Canvas Items
      //---------------------------------------------------

      const MusECore::TempoList* tl = &MusEGlobal::tempomap;
      for (MusECore::ciTEvent i = tl->begin(); i != tl->end(); ++i) {
            MusECore::TEvent* e = i->second;
            int etick = mapx(i->first);
            int stick = mapx(i->second->tick);
            int tempo = mapy(280000 - int(60000000000.0/(e->tempo)));

            if (tempo < 0)
                  tempo = 0;
            if (tempo < wh)
                p.fillRect(stick, tempo, etick-stick, wh, graph_fg_color);
            }

      //---------------------------------------------------
      //    draw marker
      //---------------------------------------------------

      pen.setColor(MusEGlobal::config.rangeMarkerColor);
      p.setPen(pen);
      int xp = mapx(pos[1]);
      if (xp >= x && xp < x+w) {
            p.drawLine(xp, y, xp, y+h);
            }
      xp = mapx(pos[2]);
      if (xp >= x && xp < x+w) {
            p.drawLine(xp, y, xp, y+h);
            }
      pen.setColor(MusEGlobal::config.positionMarkerColor);
      p.setPen(pen);
      // Draw the red main position cursor last, on top of the others.
      xp = mapx(pos[0]);
      if (xp >= x && xp < x+w) {
            p.drawLine(xp, y, xp, y+h);
            }
    }

//---------------------------------------------------------
//   draw
//---------------------------------------------------------

void Master::draw(QPainter& p, const QRect& rect, const QRegion& rg)
      {
      if (MusEGlobal::config.canvasShowGrid)
      {
        drawTickRaster(p, rect, rg, editor->raster(),
                      false, false, false,
                      MusEGlobal::config.midiCanvasBeatColor,
                      MusEGlobal::config.midiCanvasBeatColor,
                      MusEGlobal::config.midiCanvasFineColor,
                      MusEGlobal::config.midiCanvasBarColor
                      );
      }

      if ((tool == MusEGui::DrawTool) && drawLineMode) {
          QPen pen;
          pen.setCosmetic(true);
          pen.setColor(Qt::black);
          p.setPen(pen);
          p.drawLine(line1x, line1y, line2x, line2y);
        }
      }

//---------------------------------------------------------
//   newValRamp
//---------------------------------------------------------

void Master::newValRamp(int x1, int y1, int x2, int y2, MusECore::Undo& operations)
{
// loop through all tick positions between x1 and x2
// remove all tempo changes and add new ones for changed

  if(x1 < 0)
    x1 = 0;
  if(x2 < 0)
    x2 = 0;
  
  // line drawn from right to left...
  if (x1 > x2) {
      qSwap(x1, x2);
      qSwap(y1, y2);
  }

  int tickStart = editor->rasterVal1(x1);
  int tickEnd = editor->rasterVal2(x2);

  const MusECore::TempoList* tl = &MusEGlobal::tempomap;
  for (MusECore::ciTEvent i = tl->begin(); i != tl->end(); ++i) {
    MusECore::TEvent* e = i->second;
    int startOldTick = i->second->tick;
    //if (startOldTick > tickStart && startOldTick <= tickEnd ) {    // REMOVE Tim. Sharing. Wrong comparison?
    if (startOldTick >= tickStart && startOldTick > 0 && startOldTick < tickEnd ) {  // Erasure at tick 0 forbidden.
        operations.push_back(MusECore::UndoOp(MusECore::UndoOp::DeleteTempo, startOldTick, e->tempo));
        //printf("tempo = %f %d %d\n", 60000000.0/e->tempo, endOldTick, startOldTick);
    }
  }

  int priorTick = editor->rasterVal1(x1);
  int tempoVal = int(60000000000.0/(280000 - y1));
  operations.push_back(MusECore::UndoOp(MusECore::UndoOp::AddTempo, tickStart, tempoVal));
  int tick = editor->rasterVal1(x1);
  for (int i = x1; tick < tickEnd; i++) {
    tick = editor->rasterVal1(i);
    if (tick > priorTick) {
        double xproportion = double(tick-tickStart)/double(tickEnd-tickStart);
        int yproportion = double(y2 - y1) * xproportion;
        int yNew = y1 + yproportion;
        int tempoVal = int(60000000000.0/(280000 - yNew));
        //printf("tickStart %d tickEnd %d yNew %d xproportion %f yproportion %d\n", tickStart, tickEnd, yNew, xproportion, yproportion);
        operations.push_back(MusECore::UndoOp(MusECore::UndoOp::AddTempo, tick, tempoVal));
        priorTick = tick;
    }
  }
}


//---------------------------------------------------------
//   viewMousePressEvent
//---------------------------------------------------------

void Master::viewMousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::RightButton) {
        callContextMenu();
        return;
    }

    start = event->pos();
    int xpos = start.x();
    int ypos = start.y();

    _operations.clear();

    MusEGui::Tool activeTool = tool;

    switch (activeTool) {
    case MusEGui::PencilTool:
        drag = DRAG_NEW;
        MusEGlobal::song->startUndo();
        newVal(start.x(), start.x(), start.y(), _operations);
        break;

    case MusEGui::RubberTool:
        drag = DRAG_DELETE;
        MusEGlobal::song->startUndo();
        deleteVal(start.x(), start.x(), _operations);
        break;

    case MusEGui::DrawTool:
        if (drawLineMode) {
            line2x = xpos;
            line2y = ypos;
            newValRamp(line1x, line1y, line2x, line2y, _operations);
            // Operation is undoable.
            MusEGlobal::song->applyOperationGroup(_operations);
            _operations.clear();
            drawLineMode = false;
        }
        else {
            line2x = line1x = xpos;
            line2y = line1y = ypos;
            drawLineMode = true;
        }
        redraw();
        return;

        break;
    default:
        break;
    }

    // Operation is undoable but do not start/end undo.
    MusEGlobal::song->applyOperationGroup(_operations, MusECore::Song::OperationUndoable);
    redraw();
}

//---------------------------------------------------------
//   viewMouseMoveEvent
//---------------------------------------------------------

void Master::viewMouseMoveEvent(QMouseEvent* event)
      {
      QPoint pos = event->pos();
      if (tool == MusEGui::DrawTool && drawLineMode) {
            line2x = pos.x();
            line2y = pos.y();
            redraw();
            return;
            }

      _operations.clear();

      switch (drag) {
            case DRAG_NEW:
                  newVal(start.x(), pos.x(), pos.y(), _operations);
                  start = pos;
                  break;

            case DRAG_DELETE:
                  deleteVal(start.x(), pos.x(), _operations);
                  start = pos;
                  break;

            default:
                  break;
            }

      // Operation is undoable but do not start/end undo.
      MusEGlobal::song->applyOperationGroup(_operations, MusECore::Song::OperationUndoable);

      emit tempoChanged(280000 - event->position().toPoint().y());
      int x = pos.x();
      if (x < 0)
            x = 0;
      emit timeChanged(editor->rasterVal(x));
      }

//---------------------------------------------------------
//   viewMouseReleaseEvent
//---------------------------------------------------------

void Master::viewMouseReleaseEvent(QMouseEvent*)
      {
      _operations.clear();

      switch (drag) {
            case DRAG_RESIZE:
            case DRAG_NEW:
            case DRAG_DELETE:
                  MusEGlobal::song->endUndo(SC_TEMPO);
                  break;
            default:
                  break;
            }
      drag = DRAG_OFF;
      }

//---------------------------------------------------------
//   deleteVal
//---------------------------------------------------------

bool Master::deleteVal1(unsigned int x1, unsigned int x2, MusECore::Undo& operations)
      {
      QList< QPair<int,int> > stuff_to_do;
      
      MusECore::TempoList* tl = &MusEGlobal::tempomap;
      for (MusECore::iTEvent i = tl->begin(); i != tl->end(); ++i) {
            if (i->first < x1)
                  continue;
            if (i->first >= x2)
                  break;
            MusECore::iTEvent ii = i;
            ++ii;
            if (ii != tl->end()) {
                  int tempo = ii->second->tempo;
                  // changed by flo: postpone the actual delete operation
                  // to avoid race conditions and invalidating the iterator
                  //MusEGlobal::audio->msgDeleteTempo(i->first, tempo, false);
                  stuff_to_do.append(QPair<int,int>(i->first, tempo));
                  }
            }
      
      for (QList< QPair<int,int> >::iterator it=stuff_to_do.begin(); it!=stuff_to_do.end(); it++)
        operations.push_back(MusECore::UndoOp(MusECore::UndoOp::DeleteTempo, it->first, it->second));
      
      return !stuff_to_do.empty();
      }

void Master::deleteVal(int x1, int x2, MusECore::Undo& operations)
      {
      if(x1 < 0)
        x1 = 0;
      if(x2 < 0)
        x2 = 0;
      if (deleteVal1(editor->rasterVal1(x1), x2, operations))
            redraw();
      }

//---------------------------------------------------------
//   setTool
//---------------------------------------------------------

void Master::setTool(int t)
{
    if (tool == MusEGui::Tool(t))
        return;
    tool = MusEGui::Tool(t);
    switch(tool) {
    case MusEGui::PencilTool:
        setCursor(*pencilCursor);
        break;
    case MusEGui::RubberTool:
        setCursor(*deleteCursor);
        break;
    case MusEGui::DrawTool:
        setCursor(*drawCursor);
        drawLineMode = false;
        break;
    default:
        setCursor(QCursor(Qt::ArrowCursor));
        break;
    }
}

//---------------------------------------------------------
//   newVal
//---------------------------------------------------------

void Master::newVal(int x1, int x2, int y, MusECore::Undo& operations)
      {
      if(x1 < 0)
        x1 = 0;
      if(x2 < 0)
        x2 = 0;
      int xx1 = editor->rasterVal1(x1);
      int xx2 = editor->rasterVal2(x2);

      if (xx1 > xx2) {
            int tmp = xx2;
            xx2 = xx1;
            xx1 = tmp;
            }
      deleteVal1(xx1, xx2, operations);
      operations.push_back(MusECore::UndoOp(MusECore::UndoOp::AddTempo,
                    xx1, int(60000000000.0/(280000 - y))));
      redraw();
      }

QMenu* Master::toolContextMenu()
{
    QMenu* r_menu = new QMenu(this);
    QAction* act0 = nullptr;

    r_menu->addAction(new MenuTitleItem(tr("Tools"), r_menu));

    int editTools = static_cast<MasterEdit*>(editor)->getEditTools();
    for (unsigned i = 0; i < static_cast<unsigned>(EditToolBar::toolList.size()); ++i) {
        if ((editTools & (1 << i)) == 0)
            continue;
        QAction* act = r_menu->addAction(QIcon(**EditToolBar::toolList[i].icon), tr(EditToolBar::toolList[i].tip));

        if (MusEGui::EditToolBar::toolShortcuts.contains(1 << i)) {
            act->setShortcut(MusEGui::shortcuts[MusEGui::EditToolBar::toolShortcuts[1 << i]].key);
        }

        act->setData(editTools & (1 << i));
        act->setCheckable(true);
        act->setChecked((1 << i) == tool);
        if (!act0)
            act0 = act;
    }

    r_menu->setActiveAction(act0);
    return r_menu;
}

void Master::callContextMenu()
{
    QMenu * cm = toolContextMenu();
    if (cm) {
        QAction *act = cm->exec(QCursor::pos());
        if (act && act->data().isValid()) {
            int selTool = act->data().toInt();
            static_cast<MasterEdit*>(editor)->setEditTool(selTool);
        }
        delete cm;
    }
}

} // namespace MusEGui
