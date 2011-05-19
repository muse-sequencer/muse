//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: mtscale_flo.h,v 1.3 2011/05/19 22:27:06 flo Exp $
//  (C) Copyright 1999 Werner Schweer (ws@seh.de)
//=========================================================

#ifndef __MTSCALE_FLO_H__
#define __MTSCALE_FLO_H__

#include "view.h"


class ScoreCanvas;

//---------------------------------------------------------
//   MTScaleFlo
//    scale for midi track
//---------------------------------------------------------

class MTScaleFlo : public View {
      Q_OBJECT
      unsigned pos[3];
      int button;
      ScoreCanvas* parent;
      int xpos;
      int xoffset;

   private slots:
      void songChanged(int);

   protected:
      virtual void draw(QPainter&, const QRect&);
      virtual void mousePressEvent(QMouseEvent* event);
      virtual void mouseMoveEvent(QMouseEvent* event);
      virtual void mouseReleaseEvent(QMouseEvent* event);

   signals:
      void timeChanged(unsigned);

   public slots:
      void setPos(int, unsigned, bool);
      void set_xpos(int);
      void pos_add_changed();
      void set_xoffset(int);

   public:
      MTScaleFlo(ScoreCanvas* parent_editor, QWidget* parent_widget);
      };
#endif

