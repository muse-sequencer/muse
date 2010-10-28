//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: master.h,v 1.3 2004/04/11 13:03:32 wschweer Exp $
//  (C) Copyright 1999 Werner Schweer (ws@seh.de)
//=========================================================

#ifndef __MASTER_H__
#define __MASTER_H__

#include <QPoint>
//Added by qt3to4:
#include <QMouseEvent>
#include <QEvent>

#include "view.h"
#include "song.h"
#include "tools.h"

class ScrollScale;
class QPainter;
class QRect;
class MidiEditor;

//---------------------------------------------------------
//   Master
//---------------------------------------------------------

class Master : public View {
      enum DragMode { DRAG_OFF, DRAG_NEW, DRAG_MOVE_START, DRAG_MOVE,
            DRAG_DELETE, DRAG_COPY_START, DRAG_COPY,
            DRAG_RESIZE, DRAG_LASSO_START, DRAG_LASSO
            };
      ScrollScale* vscroll;
      unsigned pos[3];
      QPoint start;
      Tool tool;
      DragMode drag;
      MidiEditor* editor;

      Q_OBJECT
      virtual void pdraw(QPainter&, const QRect&);
      virtual void viewMouseMoveEvent(QMouseEvent* event);
      virtual void leaveEvent(QEvent*e);
      virtual void viewMousePressEvent(QMouseEvent* event);
      virtual void viewMouseReleaseEvent(QMouseEvent*);

      void draw(QPainter&, const QRect&);
      void newVal(int x1, int x2, int y);
      bool deleteVal1(unsigned int x1, unsigned int x2);
      void deleteVal(int x1, int x2);

   signals:
      void followEvent(int);
      void xposChanged(int);
      void yposChanged(int);
      void timeChanged(unsigned);
      void tempoChanged(int);

   public slots:
      void setPos(int, unsigned, bool adjustScrollbar);
      void setTool(int t);

   public:
      Master(MidiEditor*, QWidget* parent, int xmag, int ymag);
      };

#endif

