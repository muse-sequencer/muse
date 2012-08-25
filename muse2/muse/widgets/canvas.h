//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: canvas.h,v 1.3.2.8 2009/02/02 21:38:01 terminator356 Exp $
//  (C) Copyright 1999 Werner Schweer (ws@seh.de)
//  Additions, modifications (C) Copyright 2011 Tim E. Real (terminator356 on users DOT sourceforge DOT net)
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

#ifndef __CANVAS_H__
#define __CANVAS_H__

#include "citem.h"
#include "view.h"
#include "tools.h"
#include "undo.h"

#include <QWheelEvent>
#include <QMouseEvent>
#include <QKeyEvent>

class QMenu;

namespace MusECore {
class Rasterizer;  
}

namespace MusEGui {

//---------------------------------------------------------
//   Canvas
//---------------------------------------------------------

class Canvas : public View {
      Q_OBJECT
      int canvasTools;
      QTimer *scrollTimer;
      
      bool doScroll;
      int scrollSpeed;

      QPoint ev_pos;
      bool canScrollLeft;
      bool canScrollRight;
      bool canScrollUp;
      bool canScrollDown;
   protected:
      enum DragMode {
            DRAG_OFF, DRAG_NEW,
            DRAG_MOVE_START, DRAG_MOVE,
            DRAG_COPY_START, DRAG_COPY,
            DRAG_CLONE_START, DRAG_CLONE,
            DRAGX_MOVE, DRAGY_MOVE,
            DRAGX_COPY, DRAGY_COPY,
            DRAGX_CLONE, DRAGY_CLONE,
            DRAG_DELETE,
            DRAG_RESIZE, DRAG_LASSO_START, DRAG_LASSO
            };

      enum DragType {
            MOVE_MOVE, MOVE_COPY, MOVE_CLONE
            };

      enum HScrollDir {
            HSCROLL_NONE, HSCROLL_LEFT, HSCROLL_RIGHT
            };
      enum VScrollDir {
            VSCROLL_NONE, VSCROLL_UP, VSCROLL_DOWN
            };
      
      //MusECore::Pos::TType _timeType;  // REMOVE Tim
      //MusECore::Rasterizer _raster;
      CItemList items;
      CItemList moving;
      CItem* curItem;
      MusECore::Part* curPart;
      int curPartId;

      DragMode drag;
      QRect lasso;
      QPoint start;
      Tool _tool;
      //unsigned pos[3];
      MusECore::Pos pos[3];
      
      HScrollDir hscrollDir;
      VScrollDir vscrollDir;
      int button;
      Qt::KeyboardModifiers keyState;
      QMenu* itemPopupMenu;
      QMenu* canvasPopupMenu;

      void setCursor();
      virtual void viewKeyPressEvent(QKeyEvent* event);
      virtual void viewMousePressEvent(QMouseEvent* event);
      virtual void viewMouseMoveEvent(QMouseEvent*);
      virtual void viewMouseReleaseEvent(QMouseEvent*);
      //virtual void draw(QPainter&, const QRect&);  // REMOVE Tim.
      virtual void draw(QPainter&, const QRect&, const MusECore::Rasterizer& rasterizer);
      virtual void wheelEvent(QWheelEvent* e);

      virtual bool mousePress(QMouseEvent*) { return true; }
      virtual void keyPress(QKeyEvent*);
      virtual void mouseMove(QMouseEvent* event) = 0;
      virtual void mouseRelease(const QPoint&) {}
      virtual void drawCanvas(QPainter&, const QRect&) = 0;
      virtual void drawTopItem(QPainter& p, const QRect& rect) = 0;

      virtual void drawParts(QPainter&, const QRect&, bool do_cur_part) { };
      virtual void drawItem(QPainter&, const CItem*, const QRect&) = 0;
      virtual void drawMoving(QPainter&, const CItem*, const QRect&) = 0;
      virtual void updateSelection() = 0;
      virtual QPoint rasterPoint(const QPoint&) const = 0;
      virtual int y2pitch(int) const = 0; //CDW
      virtual int pitch2y(int) const = 0; //CDW

      virtual CItem* newItem(const QPoint&, int state) = 0;
      virtual void resizeItem(CItem*, bool noSnap=false, bool ctrl=false) = 0;
      virtual void newItem(CItem*, bool noSnap=false) = 0;
      virtual bool deleteItem(CItem*) = 0;
      int getCurrentDrag();

      /*!
         \brief Virtual member

         Implementing class is responsible for creating a popup to be shown when the user rightclicks an item on the Canvas
         \param item The canvas item that is rightclicked
         \return A QPopupMenu*
         */
      virtual QMenu* genItemPopup(CItem* /*item*/) { return 0; }

      /*!
         \brief Pure virtual member

         Implementing class is responsible for creating a popup to be shown when the user rightclicks an empty region of the canvas
         \return A QPopupMenu*
         */
      QMenu* genCanvasPopup();

      /*!
         \brief Virtual member

         This is the function called when the user has selected an option in the popupmenu generated by genItemPopup()
         \param item the canvas item the whole thing is about
         \param n Command type
         \param pt I think this is the position of the pointer when right mouse button was pressed
         */
      virtual void itemPopup(CItem* /*item */, int /*n*/, const QPoint& /*pt*/) {}
      void canvasPopup(int);

      virtual void startDrag(CItem*, bool) {}

      // selection
      virtual void deselectAll();
      virtual void selectItem(CItem* e, bool);

      virtual void deleteItem(const QPoint&);

      // moving
      void startMoving(const QPoint&, DragType);
      
      void moveItems(const QPoint&, int dir, bool rasterize = true);
      virtual void endMoveItems(const QPoint&, DragType, int dir) = 0;

      virtual void selectLasso(bool toggle);

      virtual void itemPressed(const CItem*) {}
      virtual void itemReleased(const CItem*, const QPoint&) {}
      virtual void itemMoved(const CItem*, const QPoint&) {}
      virtual void curPartChanged() { emit curPartHasChanged(curPart); }
      // HACK: Needs rasterizer but Canvas does not carry the MidiEditor member. Transferring MidiEditor to class Canvas 
      //        is undesirable. And adding a Rasterizer parameter to setPos signals/slots is undesirable. 
      //       So this method is called by inheriters (like EventCanvas and PartCanvas), from normal sePos slot methods. 
      virtual void setPos(int idx, const MusECore::Pos& pos, bool adjustScrollbar, const MusECore::Rasterizer& rasterizer);

   public slots:
      void setTool(int t);
      //virtual void setPos(int, unsigned, bool adjustScrollbar);  // REMOVE Tim.
      //virtual void setPos(int idx, const MusECore::Pos& pos, bool adjustScrollbar);
      //virtual void setPos(int idx, const MusECore::Pos& pos, bool adjustScrollbar, const MusECore::Rasterizer& rasterizer);
      void scrollTimerDone(void);
      void redirectedWheelEvent(QWheelEvent*);
      //void setTimeType(MusECore::Pos::TType tt);  // REMOVE Tim.
      //void setFormatted(bool f); 
      //void setRasterVal(int val); 
      
   signals:
      void followEvent(int);
      void toolChanged(int);
      void verticalScroll(unsigned);
      void horizontalScroll(unsigned);
      void horizontalScrollNoLimit(unsigned);
      void horizontalZoomIn();
      void horizontalZoomOut();
      void curPartHasChanged(MusECore::Part*);
      
   public:
      Canvas(QWidget* parent, int sx, int sy, const char* name = 0);
      //Canvas(QWidget* parent, int sx, int sy, const char* name = 0, 
      //       MusECore::Pos::TType time_type = MusECore::Pos::TICKS, 
      //       bool formatted = true, int raster = 1);
      virtual ~Canvas();
      //MusECore::Pos::TType timeType() const { return _raster.timeType(); }
      //bool formatted() const { return _raster.formatted(); } 
      //int rasterVal() const { return _raster.raster(); } 
      //MusECore::Rasterizer& raster() { return _raster; }
      bool isSingleSelection();
      int selectionSize();
      Tool tool() const { return _tool; }
      MusECore::Part* part() const { return curPart; }
      void setCurrentPart(MusECore::Part*); 
      void setCanvasTools(int n) { canvasTools = n; }
      };

} // namespace MusEGui

#endif

