//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: wtscale.h,v 1.2 2004/01/11 18:55:37 wschweer Exp $
//  (C) Copyright 2000 Werner Schweer (ws@seh.de)
//=========================================================

#ifndef __MTSCALE_H__
#define __MTSCALE_H__

#include "view.h"
//Added by qt3to4:
#include <QMouseEvent>
#include <QEvent>

class QPainter;

//---------------------------------------------------------
//   WTScale
//    scale for wave track
//---------------------------------------------------------

class WTScale : public View {
      Q_OBJECT
      int* raster;
      unsigned pos[4];
      int button;
      bool barLocator;

   private slots:
      void songChanged(int);

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
      WTScale(int* raster, QWidget* parent, int xscale);
      void setBarLocator(bool f) { barLocator = f; }
      };
#endif

