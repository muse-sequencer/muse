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

#include "master.h"
#include "icons.h"
#include "audio.h"
#include "al/tempo.h"
#include "song.h"

static const int minTempo = 50;
static const int maxTempo = 250;

//---------------------------------------------------------
//   pix2tempo
//---------------------------------------------------------

int MasterCanvas::pix2tempo(int val) const
      {
      return maxTempo*1000 - mapyDev(val);
      }

//---------------------------------------------------------
//   tempo2pix
//---------------------------------------------------------

int MasterCanvas::tempo2pix(int val) const
      {
      return mapy(maxTempo*1000 - lrint(60000000000.0/val));
      }

//---------------------------------------------------------
//   MasterCanvas
//---------------------------------------------------------

MasterCanvas::MasterCanvas()
   : TimeCanvas(TIME_CANVAS)
      {
      setMarkerList(song->marker());
      setMag(xmag(), 400.0 / (maxTempo * 1000.0));
      setYMagRange(40.0 / (maxTempo*1000), 4000.0 / (maxTempo*1000));
      setVSize((maxTempo - minTempo) * 1000);
      setYFit(true);
      verticalScrollBar()->setSingleStep(1000);
      verticalScrollBar()->setMinimum(minTempo * 1000);
      verticalScrollBar()->setMaximum(maxTempo * 1000);
      drag = DRAG_OFF;
      }

//---------------------------------------------------------
//   paint
//---------------------------------------------------------

void MasterCanvas::paint(QPainter& p, QRect rect)
      {
      int y  = rect.y();

      //---------------------------------------------------
      // draw Canvas Items
      //---------------------------------------------------

      int y2 = lrint(maxTempo * 1000 * _ymag);
      const AL::TempoList* tl = &AL::tempomap;
      for (AL::ciTEvent i = tl->begin(); i != tl->end(); ++i) {
            AL::TEvent* e = i->second;
            int x1 = pos2pix(AL::Pos(i->first));
            int x2 = pos2pix(AL::Pos(i->second->tick));
            int y1 = lrint(_ymag * (maxTempo*1000 - lrint(60000000000.0/e->tempo)));
            if (y1 < y)
                  y1 = y;
            p.fillRect(x1, y1, x2 - x1, y2 - y1, Qt::blue);
            }
      }

//---------------------------------------------------------
//   viewMousePressEvent
//---------------------------------------------------------

void MasterCanvas::mousePress(QMouseEvent* event)
      {
      start = event->pos();
      int x = start.x() - rCanvasA.x();
      int y = start.y() - rCanvasA.y();

      switch (tool()) {
            case PointerTool:
                  drag = DRAG_LASSO_START;
                  break;

            case PencilTool:
                  drag = DRAG_NEW;
                  song->startUndo();
                  newVal(x, x, y);
                  break;

            case RubberTool:
                  drag = DRAG_DELETE;
                  song->startUndo();
                  deleteVal(x, x);
                  break;

            default:
                  break;
            }
      }

//---------------------------------------------------------
//   viewMouseMoveEvent
//---------------------------------------------------------

void MasterCanvas::mouseMove(QPoint pos)
      {
      int y = pos.y() - rCanvasA.y();
      int x = pos.x() - rCanvasA.x();

      QPoint dist = pos - start;
      // bool moving = dist.y() >= 3 || dist.y() <= 3 || dist.x() >= 3 || dist.x() <= 3;

      int sx = start.x() - rCanvasA.x();
      switch (drag) {
            case DRAG_NEW:
                  newVal(sx, x, y);
                  start = pos;
                  break;

            case DRAG_DELETE:
                  deleteVal(sx, x);
                  start = pos;
                  break;

            default:
                  break;
            }
      emit tempoChanged(pix2tempo(y));
//      emit timeChanged(editor->rasterVal(x));
      }

//---------------------------------------------------------
//   mouseRelease
//---------------------------------------------------------

void MasterCanvas::mouseRelease(QMouseEvent*)
      {
      switch (drag) {
            case DRAG_RESIZE:
            case DRAG_NEW:
            case DRAG_DELETE:
                  song->endUndo(SC_TEMPO);
                  break;
            default:
                  break;
            }
      drag = DRAG_OFF;
      }

//---------------------------------------------------------
//   deleteVal
//---------------------------------------------------------

bool MasterCanvas::deleteVal1(const AL::Pos& p1, const AL::Pos& p2)
      {
      bool songChanged = false;

      AL::TempoList* tl = &AL::tempomap;
      for (AL::iTEvent i = tl->begin(); i != tl->end(); ++i) {
            if (i->first < p1.tick())
                  continue;
            if (i->first >= p2.tick())
                  break;
            AL::iTEvent ii = i;
            ++ii;
            if (ii != tl->end()) {
                  int tempo = ii->second->tempo;
                  audio->msgDeleteTempo(i->first, tempo, false);
                  songChanged = true;
                  }
            }
      return songChanged;
      }

//---------------------------------------------------------
//   deleteVal
//---------------------------------------------------------

void MasterCanvas::deleteVal(int x1, int x2)
      {
      AL::Pos p1(pix2pos(x1));
      AL::Pos p2(pix2pos(x2));
      p1.downSnap(raster());
      p2.upSnap(raster());
      if (deleteVal1(p1, p2))
            widget()->update();
      }

//---------------------------------------------------------
//   newVal
//---------------------------------------------------------

void MasterCanvas::newVal(int x1, int x2, int y)
      {
      int tempo  = lrint(60000000000.0 /  pix2tempo(y));
      AL::Pos p1 = pix2pos(x1);
      p1.downSnap(raster());
      AL::Pos p2 = pix2pos(x2);
      p2.upSnap(raster());

      if (p1 > p2) {
            AL::Pos tmp = p2;
            p2 = p1;
            p1 = tmp;
            }
      deleteVal1(p1, p2);
      audio->msgAddTempo(p1.tick(), tempo, false);
      widget()->update();
      }


