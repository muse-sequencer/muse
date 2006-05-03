//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: master.h,v 1.12 2005/12/09 17:54:16 wschweer Exp $
//  (C) Copyright 1999 Werner Schweer (ws@seh.de)
//=========================================================

#ifndef __MASTER_H__
#define __MASTER_H__

#include "awl/tcanvas.h"

class GraphMidiEditor;

//---------------------------------------------------------
//   MasterCanvas
//---------------------------------------------------------

class MasterCanvas : public TimeCanvas {
      QPoint start;

      enum {
            DRAG_OFF, DRAG_LASSO_START, DRAG_RESIZE, DRAG_NEW,
            DRAG_DELETE
            } drag;

      Q_OBJECT
      virtual void mouseMove(QPoint);
      virtual void mousePress(QMouseEvent* event);
      virtual void mouseRelease(QMouseEvent*);

      virtual void paint(QPainter&, QRect);

      void newVal(int x1, int x2, int y);
      bool deleteVal1(const AL::Pos&, const AL::Pos&);
      void deleteVal(int x1, int x2);

      int pix2tempo(int) const;
      int tempo2pix(int) const;

   signals:
      void tempoChanged(int);

   public:
      MasterCanvas();
      virtual ~MasterCanvas() {}
      };

#endif

