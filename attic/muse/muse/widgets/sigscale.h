//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: sigscale.h,v 1.2 2004/01/11 18:55:37 wschweer Exp $
//  (C) Copyright 1999 Werner Schweer (ws@seh.de)
//=========================================================

#ifndef __SIGSCALE_H__
#define __SIGSCALE_H__

#include "view.h"

class QPainter;
class MidiEditor;

//---------------------------------------------------------
//   SigScale
//    Time Signature Scale
//---------------------------------------------------------

class SigScale : public View {
      Q_OBJECT
      int* raster;
      unsigned pos[3];
      int button;

   signals:
      void posChanged(unsigned, unsigned);

   protected:
      virtual void pdraw(QPainter&, const QRect&);
      virtual void viewMousePressEvent(QMouseEvent* event);
      virtual void viewMouseMoveEvent(QMouseEvent* event);
      virtual void viewMouseReleaseEvent(QMouseEvent* event);
      virtual void leaveEvent(QEvent*e);

   signals:
      void timeChanged(unsigned);

   public slots:
      void setPos(int, unsigned, bool);

   public:
      SigScale(int* raster, QWidget* parent, int xscale);
      };
#endif

