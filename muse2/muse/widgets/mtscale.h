//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: mtscale.h,v 1.3 2004/04/27 22:27:06 spamatica Exp $
//  (C) Copyright 1999 Werner Schweer (ws@seh.de)
//=========================================================

#ifndef __MTSCALE_H__
#define __MTSCALE_H__

#include "view.h"
//Added by qt3to4:
#include <QMouseEvent>
#include <QEvent>

class QPainter;

//---------------------------------------------------------
//   MTScale
//    scale for midi track
//---------------------------------------------------------

class MTScale : public View {
      Q_OBJECT
      int* raster;
      unsigned pos[4];
      int button;
      bool barLocator;
      bool waveMode;

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
      //void addMarker(int);

   public slots:
      void setPos(int, unsigned, bool);

   public:
      MTScale(int* raster, QWidget* parent, int xscale, bool f = false);
      void setBarLocator(bool f) { barLocator = f; }
      };
#endif

