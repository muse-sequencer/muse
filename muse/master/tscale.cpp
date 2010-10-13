//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: tscale.cpp,v 1.2 2003/12/17 11:04:14 wschweer Exp $
//  (C) Copyright 1999 Werner Schweer (ws@seh.de)
//=========================================================

#include <stdio.h>
#include "tscale.h"
#include "globals.h"
#include "gconfig.h"
//Added by qt3to4:
#include <QMouseEvent>
#include <QEvent>

//---------------------------------------------------------
//   TScale
//---------------------------------------------------------

TScale::TScale(QWidget* parent, int ymag)
   : View(parent, 1, ymag)
      {
      setFont(config.fonts[3]);
      int w = 4 * QFontMetrics(config.fonts[4]).width('0');
      setFixedWidth(w);
      setMouseTracking(true);
      }

//---------------------------------------------------------
//   draw
//---------------------------------------------------------

void TScale::pdraw(QPainter& p, const QRect& r)
      {
      int y = r.y();
      int h = r.height();
      p.setFont(config.fonts[4]);
      QString s;
      for (int i = 30000; i <= 250000; i += 10000) {
            int yy =  mapy(280000 - i);
            if (yy < y)
                  break;
            if (yy-15 > y+h)
                  continue;
            p.drawLine(0, yy, width(), yy);
            s.setNum(i/1000);
            QFontMetrics fm(config.fonts[4]);
            p.drawText(width() - fm.width(s) - 1, yy-2, s);
            }
      }

void TScale::viewMouseMoveEvent(QMouseEvent* event)
      {
      emit tempoChanged(280000 - event->y());
      }

void TScale::leaveEvent(QEvent*)
      {
      emit tempoChanged(-1);
      }

