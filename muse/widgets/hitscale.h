//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: hitscale.h,v 1.2 2004/01/11 18:55:37 wschweer Exp $
//  (C) Copyright 1999 Werner Schweer (ws@seh.de)
//=========================================================

#ifndef __HITSCALE_H__
#define __HITSCALE_H__

#include "view.h"

class MidiEditor;

//---------------------------------------------------------
//   HitScale
//    scale for midi track
//---------------------------------------------------------

class HitScale : public View {
      Q_OBJECT
      int* raster;
      unsigned pos[3];
      int button;

   signals:
//      void posChanged(int, int);

   protected:
      virtual void pdraw(QPainter&, const QRect&);
      virtual void viewMousePressEvent(QMouseEvent* event);
      virtual void viewMouseMoveEvent(QMouseEvent* event);
      virtual void viewMouseReleaseEvent(QMouseEvent* event);
      virtual void leaveEvent(QEvent*e);

   signals:
      void timeChanged(int);

   public slots:
      void setPos(int, unsigned, bool);

   public:
      HitScale(int* raster, QWidget* parent, int xscale);
      };
#endif

