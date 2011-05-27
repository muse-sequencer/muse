//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: tscale.h,v 1.1.1.1 2003/10/27 18:52:36 wschweer Exp $
//  (C) Copyright 1999 Werner Schweer (ws@seh.de)
//=========================================================

#ifndef __TSCALE_H__
#define __TSCALE_H__

#include "view.h"

//---------------------------------------------------------
//   Tscale
//---------------------------------------------------------

class TScale : public View {
      Q_OBJECT
        
      double curTempo;

      virtual void viewMouseMoveEvent(QMouseEvent* event);
      virtual void leaveEvent(QEvent*e);

   protected:
      virtual void pdraw(QPainter&, const QRect&);

   signals:
      void tempoChanged(int);

   public:
      TScale(QWidget*, int);
      };

#endif

