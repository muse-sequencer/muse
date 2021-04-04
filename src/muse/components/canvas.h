//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: canvas.h,v 1.3.2.8 2009/02/02 21:38:01 terminator356 Exp $
//  (C) Copyright 1999 Werner Schweer (ws@seh.de)
//  Additions, modifications (C) Copyright 2011-2013 Tim E. Real (terminator356 on users DOT sourceforge DOT net)
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
#include "type_defs.h"

#include "event_tag_list.h"

#include <QPoint>
#include <QRegion>
#include <QRect>


// Forward declarations:
class QMenu;
class QPainter;
class QTimer;
class QWheelEvent;
class QMouseEvent;
class QKeyEvent;

namespace MusECore {
class Undo;
}

namespace MusEGui {

//---------------------------------------------------------
//   Canvas
//---------------------------------------------------------

class Canvas : public View {
      Q_OBJECT
      QTimer *scrollTimer;
      
      bool doScroll;
      int scrollSpeed;
      
      QPoint ev_pos;
      QPoint ev_global_pos;
      bool ignore_mouse_move;
      bool canScrollLeft;
      bool canScrollRight;
      bool canScrollUp;
      bool canScrollDown;

//      CItem *findCurrentItem(const QPoint &cStart);
      
      // Whether we have grabbed the mouse.
      bool _mouseGrabbed;
      // The number of times we have called QApplication::setOverrideCursor().
      // This should always be one or zero, anything else is an error, but unforeseen 
      //  events might cause us to miss a decrement with QApplication::restoreOverrideCursor().
      int _cursorOverrideCount;
      
   protected:
      enum DragMode {
            DRAG_OFF=0, DRAG_NEW,
            DRAG_MOVE_START, DRAG_MOVE,
            DRAG_COPY_START, DRAG_COPY,
            DRAG_CLONE_START, DRAG_CLONE,
            DRAGX_MOVE, DRAGY_MOVE,
            DRAGX_COPY, DRAGY_COPY,
            DRAGX_CLONE, DRAGY_CLONE,
            DRAG_DELETE,
            DRAG_RESIZE, DRAG_LASSO_START, DRAG_LASSO,
            DRAG_PAN, DRAG_ZOOM
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
      
      enum MenuIdBase {
            TOOLS_ID_BASE=10000
            };

      CItemMap items;
      CItemMap moving;
      CItem* newCItem;
      CItem* curItem;
      MusECore::Part* curPart;
      int curPartId;      

      int canvasTools;
      DragMode drag;
      QRect lasso;
      QRegion lassoRegion;
      QPoint start;
      QPoint end;
      QPoint global_start;
      Tool _tool;
      unsigned pos[3];
      MusECore::ResizeDirection resizeDirection;
      
      HScrollDir hscrollDir;
      VScrollDir vscrollDir;
      int button;
      Qt::KeyboardModifiers keyState;
      QMenu* itemPopupMenu;
      QMenu* canvasPopupMenu;

      bool supportsResizeToTheLeft;
      bool supportsMultipleResize;

      void setLasso(const QRect& r);
      void resizeToTheLeft(const QPoint &pos);
      void resizeSelected(const int &dist, const bool left = false);
      virtual void setCursor();
      virtual void setMouseOverItemCursor();
      virtual void viewKeyPressEvent(QKeyEvent* event);
      virtual void viewKeyReleaseEvent(QKeyEvent* event);
      virtual void viewMousePressEvent(QMouseEvent* event);
      virtual void viewMouseMoveEvent(QMouseEvent*);
      virtual void viewMouseReleaseEvent(QMouseEvent*);
      virtual void draw(QPainter& p, const QRect& mr, const QRegion& mrg = QRegion());
      virtual void wheelEvent(QWheelEvent* e);

      virtual void keyPress(QKeyEvent*);
      virtual void keyRelease(QKeyEvent*);
      virtual bool mousePress(QMouseEvent*) { return true; }
      virtual void mouseMove(QMouseEvent* event) = 0;
//       virtual void mouseRelease(const QPoint&) {}
      virtual void mouseRelease(QMouseEvent*) {}
      // Resets all mouse operations if detecting missed mouseRelease event (which DOES happen).
      // Returns true if reset was actually done.
      virtual bool cancelMouseOps();
      virtual void drawParts(QPainter&, bool /*do_cur_part*/, const QRect&, const QRegion& = QRegion()) { }
      virtual void drawCanvas(QPainter&, const QRect&, const QRegion& = QRegion()) = 0;
      virtual void drawTopItem(QPainter& p, const QRect& rect, const QRegion& = QRegion()) = 0;
      virtual void drawMarkers(QPainter& p, const QRect& mr, const QRegion& mrg = QRegion());
      virtual void drawItem(QPainter&, const CItem*, const QRect&, const QRegion& = QRegion()) = 0;
      virtual void drawMoving(QPainter&, const CItem*, const QRect&, const QRegion& = QRegion()) = 0;
      virtual bool itemSelectionsChanged(MusECore::Undo* operations = 0, bool deselectAll = false) = 0;
      virtual QPoint raster(const QPoint&) const = 0;
      virtual int y2pitch(int) const = 0; //CDW
      virtual int pitch2y(int) const = 0; //CDW
      virtual int y2height(int) const = 0; 
      virtual int yItemOffset() const = 0;

      virtual CItem* newItem(const QPoint&, int state) = 0;
      virtual void resizeItem(CItem*, bool noSnap=false, bool ctrl=false) = 0;
      virtual void newItem(CItem*, bool noSnap=false) = 0;
      virtual bool deleteItem(CItem*) = 0;

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
      QMenu* genCanvasPopup(QMenu* menu = 0);

      /*!
         \brief Virtual member

         This is the function called when the user has selected an option in the popupmenu generated by genItemPopup()
         \param item the canvas item the whole thing is about
         \param n Command type
         \param pt I think this is the position of the pointer when right mouse button was pressed
         */
      virtual void itemPopup(CItem* /*item */, int /*n*/, const QPoint& /*pt*/) {}
      void canvasPopup(int);

      virtual void startDrag(CItem*, DragType) = 0;// {}

      // selection
      virtual void deselectAll();
      virtual void selectItem(CItem* e, bool);

      virtual void deleteItem(const QPoint&);

      // moving
      void startMoving(const QPoint&, int dir, DragType, bool rasterize = true);
      void moveItems(const QPoint&, int dir = 0, bool rasterize = true);
      virtual void endMoveItems(const QPoint&, DragType, int dir, bool rasterize = true) = 0;

      // Returns true if anything was selected.
      virtual bool selectLasso(bool toggle);

      virtual void itemPressed(const CItem*) {}
      virtual void itemReleased(const CItem*, const QPoint&) {}
      virtual void itemsReleased() {}
      // Called to inform before an item will be moved.
      // When multiple items are moving, all itemMoving() are called before
      //  all itemMoved(), rather than in pairs for each item. (That helps with
      //  the note playing routines.) The current item will inform first before others.
      // The item's current moving point contains the position before moving,
      //  while newMP contains the position it will be moved to.
      virtual void itemMoving(const CItem*, const QPoint& /*newMP*/) { }
      // Called to inform that an item has just been moved.
      // The item's current moving point contains the position after moving,
      //  while oldMP contains the old position before it was moved.
      virtual void itemMoved(const CItem*, const QPoint& /*oldMP*/) {}

      virtual void curPartChanged() { emit curPartHasChanged(curPart); }

      // If show is true, calls QApplication::restoreOverrideCursor() until _cursorOverrideCount-- is <= 0.
      // If show is false, calls QApplication::setOverrideCursor with a blank cursor.
      void showCursor(bool show = true);
      // Sets or resets the _mouseGrabbed flag and grabs or releases the mouse.
      void setMouseGrab(bool grabbed = false);
      CItem *findCurrentItem(const QPoint &cStart);
      
   public slots:
      void setTool(int t);
      virtual void setPos(int, unsigned, bool adjustScrollbar);
      void scrollTimerDone(void);
      void redirectedWheelEvent(QWheelEvent*);

   signals:
      void followEvent(int);
      void toolChanged(int);
      void verticalScroll(unsigned);
      void horizontalScroll(unsigned);
      void horizontalScrollNoLimit(unsigned);
      void horizontalZoom(bool zoom_in, const QPoint& glob_pos);
      void horizontalZoom(int mag, const QPoint& glob_pos);
      void curPartHasChanged(MusECore::Part*);
      
   public:
      Canvas(QWidget* parent, int sx, int sy, const char* name = 0);
      virtual ~Canvas();
      
      // Converts a lasso-style (one-pixel thick) rectangle to a 
      //  four-rectangle region union - enough to cover the four sides.
      // Clears the given region first.
      void lassoToRegion(const QRect& r_in, QRegion& rg_out) const;
      
      // Whether we have grabbed the mouse.
      bool mouseGrabbed() const { return _mouseGrabbed; }
      bool isSingleSelection() const;
      int selectionSize() const;
      bool itemsAreSelected() const;
      // Appends given tag list with item objects according to options. Avoids duplicate events or clone events.
      // Special: We 'abuse' a controller event's length, normally 0, to indicate visual item length.
      virtual void tagItems(MusECore::TagEventList* tag_list, const MusECore::EventTagOptionsStruct& options) const;

      Tool tool() const { return _tool; }
      MusECore::Part* part() const { return curPart; }
      void setCurrentPart(MusECore::Part*); 
      void setCanvasTools(int n) { canvasTools = n; }
      int getCurrentDrag();
      virtual void updateItems() = 0;
      virtual void updateItemSelections();
      };

} // namespace MusEGui

#endif

