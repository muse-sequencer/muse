//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: canvas.h,v 1.21 2006/01/23 17:41:21 wschweer Exp $
//
//  (C) Copyright 2005 Werner Schweer (ws@seh.de)
//=========================================================

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

