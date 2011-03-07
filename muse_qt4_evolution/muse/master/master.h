//=============================================================================
//  MusE
//  Linux Music Editor
//  $Id:$
//
//  Copyright (C) 2002-2006 by Werner Schweer and others
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License version 2.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//=============================================================================

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

