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

#ifndef __CANVAS_H__
#define __CANVAS_H__

#include "awl/tcanvas.h"
#include "widgets/tools.h"

class CanvasWidget;
class Part;
class Track;
class ArrangerTrack;

static const int HANDLE1 = 6;
static const int HANDLE2 = 3;

//---------------------------------------------------------
//   PartCanvas
//---------------------------------------------------------

class PartCanvas : public TimeCanvas {
      Q_OBJECT

      // DRAG1    drag part head
      // DRAG2    drag part tail
      // DRAG3    drag whole part
      // DRAG4    Drag&Drop drag
      // DRAG5    draw part with pencil tool

      enum { S_NORMAL, S_START_DRAG1, S_START_DRAG2, S_START_DRAG3,
            S_DRAG1, S_DRAG2, S_DRAG3,
            S_DRAG4,
            S_START_DRAG5, S_DRAG5,                 // draw new Part
            S_SUBTRACK
            };
      int state;
      QPoint startDrag;
      int _dragOffset;
      QRect drag;
      QTime startDragTime;
      unsigned ppos, psize;

      // values set by searchPart():
      Track* track;
      Part* part;
      ArrangerTrack* at;

      bool _drawBackground;
      int selected;
      int lselected;    // in local coordinates
      int starty;
      int dragy;

      virtual void paint(QPainter&, QRect);
      void drawWavePart(QPainter& p, Part* part, int, int, int, int);
      void drawMidiPart(QPainter& p, Part* mp, int, int, int, int);
      virtual void mousePress(QMouseEvent*);
      virtual void mouseMove(QPoint);
      virtual void mouseRelease(QMouseEvent*);
      virtual void mouseDoubleClick(QMouseEvent*);

      virtual void dragEnter(QDragEnterEvent*);
      virtual void drop(QDropEvent*);
      virtual void dragMove(QDragMoveEvent*);
      virtual void dragLeave(QDragLeaveEvent*);

      int searchPart(const QPoint& p);

      void declonePart(Part* part);
      void renamePart(Part*);
      void splitPart(Part*, const QPoint&);
      void cutPart(Part*);
      void copyPart(Part*);

      void setCursor();
      int dragOffset() const { return _dragOffset; }
      void drawHandle(QPainter& p, int x, int y) {
            p.fillRect(x-HANDLE2, y-HANDLE2, HANDLE1, HANDLE1, x == lselected ? Qt::red : Qt::yellow);
            }
      void contextMenu(const QPoint&);

   signals:
      void kbdMovementUpdate(Track* t, Part* p);
      void startEditor(Part*, int);
      void createLRPart(Track*);
      void partChanged(Part*, unsigned, unsigned);
      void doubleClickPart(Part*);

   public:
      PartCanvas();
      void setDrawBackground(bool val) { _drawBackground = val; }
      };

#endif

