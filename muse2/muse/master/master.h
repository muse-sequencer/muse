//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: master.h,v 1.3 2004/04/11 13:03:32 wschweer Exp $
//  (C) Copyright 1999 Werner Schweer (ws@seh.de)
//
//  This program is free software; you can redistribute it and/or
//  modify it under the terms of the GNU General Public License
//  as published by the Free Software Foundation; version 2 of
//  the License, or (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
//
//=========================================================

#ifndef __MASTER_H__
#define __MASTER_H__

#include "view.h"
#include "song.h"
#include "tools.h"

class QMouseEvent;
class QPainter;
class QPoint;
class QRect;
class QToolBar;

namespace MusEGui {

class MidiEditor;
class ScrollScale;

//---------------------------------------------------------
//   Master
//---------------------------------------------------------

class Master : public MusEGui::View {
      Q_OBJECT
      enum DragMode { DRAG_OFF, DRAG_NEW, DRAG_MOVE_START, DRAG_MOVE,
            DRAG_DELETE, DRAG_COPY_START, DRAG_COPY,
            DRAG_RESIZE, DRAG_LASSO_START, DRAG_LASSO
            };
      ScrollScale* vscroll;
      unsigned pos[3];
      QPoint start;
      MusEGui::Tool tool;
      DragMode drag;
      MidiEditor* editor;

      
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

} // namespace MusEGui

#endif

