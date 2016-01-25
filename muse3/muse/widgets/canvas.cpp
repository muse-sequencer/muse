//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: canvas.cpp,v 1.10.2.17 2009/05/03 04:14:01 terminator356 Exp $
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

#include <stdio.h>
#include <math.h>

#include "canvas.h"

#include <QApplication>
#include <QDesktopWidget>
#include <QMenu>
#include <QPainter>
#include <QCursor>
#include <QTimer>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QRect>

#include <vector>

#include "gconfig.h"
#include "song.h"
#include "event.h"
#include "citem.h"
#include "icons.h"
#include "../marker/marker.h"
#include "part.h"
#include "fastlog.h"
#include "menutitleitem.h"
#include "shortcuts.h"

#define ABS(x)  ((x) < 0) ? -(x) : (x)

namespace MusEGui {

//---------------------------------------------------------
//   Canvas
//---------------------------------------------------------

Canvas::Canvas(QWidget* parent, int sx, int sy, const char* name)
   : View(parent, sx, sy, name)
      {
      _cursorOverrideCount = 0;
      canvasTools = 0;
      itemPopupMenu = 0;
      
      button = Qt::NoButton;
      keyState = 0;
      _mouseGrabbed = false;

      canScrollLeft = true;
      canScrollRight = true;
      canScrollUp = true;
      canScrollDown = true;
      hscrollDir = HSCROLL_NONE;
      vscrollDir = VSCROLL_NONE;
      scrollTimer=NULL;
      ignore_mouse_move = false;

      supportsResizeToTheLeft = false;
      
      scrollSpeed=30;    // hardcoded scroll jump

      drag    = DRAG_OFF;
      _tool   = PointerTool;
      pos[0]  = MusEGlobal::song->cpos();
      pos[1]  = MusEGlobal::song->lpos();
      pos[2]  = MusEGlobal::song->rpos();
      curPart = NULL;
      curPartId = -1;
      curItem = NULL;
      newCItem = NULL;
      connect(MusEGlobal::song, SIGNAL(posChanged(int, unsigned, bool)), this, SLOT(setPos(int, unsigned, bool)));
      }

Canvas::~Canvas()
{
  // Just in case the ref count is not 0. This is our last chance to clear 
  //  our contribution to QApplication::setOverrideCursor references.
  showCursor();
  
  items.clearDelete();

  if(newCItem)
  {
    if(newCItem->event().empty() && newCItem->part()) // Was it a new part, with no event?
      delete newCItem->part();
    delete newCItem;
  }
}

void Canvas::showCursor(bool show) 
{ 
  if(_cursorOverrideCount > 1)
    fprintf(stderr, "MusE Warning: _cursorOverrideCount > 1 in Canvas::showCursor(%d)\n", show);

  if(show)
  {  
    while(_cursorOverrideCount > 0)
    {
      QApplication::restoreOverrideCursor();
      _cursorOverrideCount--;
    }
  }
  else
  {
    _cursorOverrideCount++;
    QApplication::setOverrideCursor(Qt::BlankCursor); // CAUTION
  }
}

void Canvas::setMouseGrab(bool grabbed)
{
  if(grabbed && !_mouseGrabbed)
  {
    _mouseGrabbed = true;
    grabMouse(); // CAUTION
  }
  else if(!grabbed && _mouseGrabbed)
  {
    releaseMouse();
    _mouseGrabbed = false;
  }
}

//---------------------------------------------------------
//   setPos
//    set one of three markers
//    idx   - 0-cpos  1-lpos  2-rpos
//    flag  - emit followEvent()
//---------------------------------------------------------

void Canvas::setPos(int idx, unsigned val, bool adjustScrollbar)
      {
      //if (pos[idx] == val) // Seems to be some refresh problems here, pos[idx] might be val but the gui not updated.
      //    return;          // skipping this return forces update even if values match. Matching values only seem
                             // to occur when initializing
      int opos = mapx(pos[idx]);
      int npos = mapx(val);

      if (adjustScrollbar && idx == 0) {
            switch (MusEGlobal::song->follow()) {
                  case  MusECore::Song::NO:
                        break;
                  case MusECore::Song::JUMP:
                        if (npos >= width()) {
                              int ppos =  val - xorg - rmapxDev(width()/8);
                              if (ppos < 0)
                                    ppos = 0;
                              emit followEvent(ppos);
                              opos = mapx(pos[idx]);
                              npos = mapx(val);
                              }
                        else if (npos < 0) {
                              int ppos =  val - xorg - rmapxDev(width()*3/4);
                              if (ppos < 0)
                                    ppos = 0;
                              emit followEvent(ppos);
                              opos = mapx(pos[idx]);
                              npos = mapx(val);
                              }
                        break;
                  case MusECore::Song::CONTINUOUS:
                        if (npos > (width()/2)) {
                              int ppos =  pos[idx] - xorg - rmapxDev(width()/2);
                              if (ppos < 0)
                                    ppos = 0;
                              emit followEvent(ppos);
                              opos = mapx(pos[idx]);
                              npos = mapx(val);
                              }
                        else if (npos < (width()/2)) {
                              int ppos =  pos[idx] - xorg - rmapxDev(width()/2);
                              if (ppos < 0)
                                    ppos = 0;
                              emit followEvent(ppos);
                              opos = mapx(pos[idx]);
                              npos = mapx(val);
                              }
                        break;
                  }
            }

      int x;
      int w = 1;
      if (opos > npos) {
            w += opos - npos;
            x = npos;
            }
      else {
            w += npos - opos;
            x = opos;
            }
      pos[idx] = val;
      redraw(QRect(x-1, 0, w+2, height()));
      }

//---------------------------------------------------------
//   draw
//---------------------------------------------------------

void Canvas::draw(QPainter& p, const QRect& rect)
{
//      printf("draw canvas %x virt %d\n", this, virt());

      int x = rect.x();
      int y = rect.y();
      int w = rect.width();
      int h = rect.height();
      int x2 = x + w;

      std::vector<CItem*> list1;
      std::vector<CItem*> list2;
      std::vector<CItem*> list4;

      if (virt()) {
            drawCanvas(p, rect);

            //---------------------------------------------------
            // draw Canvas Items
            //---------------------------------------------------

            iCItem to(items.lower_bound(x2));
            for(iCItem i = items.begin(); i != to; ++i)
            { 
              CItem* ci = i->second;
              // NOTE Optimization: For each item call this once now, then use cached results later via cachedHasHiddenEvents().
              // Not required for now.
              //ci->part()->hasHiddenEvents();
              
              // Draw items from other parts behind all others.
              // Only for items with events (not arranger parts).
              if(!ci->event().empty() && ci->part() != curPart)
                list1.push_back(ci);    
              else if(!ci->isMoving() && (ci->event().empty() || ci->part() == curPart))
              {
                // Draw selected parts in front of all others.
                if(ci->isSelected()) 
                  list4.push_back(ci);
                else  
                  // Draw unselected parts.
                  list2.push_back(ci);
              }  
            }
            int i;
            int sz = list1.size();
            for(i = 0; i != sz; ++i) 
              drawItem(p, list1[i], rect);
            sz = list2.size();
            for(i = 0; i != sz; ++i) 
              drawItem(p, list2[i], rect);
            sz = list4.size();
            for(i = 0; i != sz; ++i) 
              drawItem(p, list4[i], rect);
            
            // Draw items being moved, a special way in their original location.
            to = moving.lower_bound(x2);
            for (iCItem i = moving.begin(); i != to; ++i) 
                  drawItem(p, i->second, rect);

            // Draw special top item for new recordings etc.
            drawTopItem(p,rect);

            // Draw special new item for first-time placement.
            // It is not in the item list yet. It will be added when mouse released.
            if(newCItem)
              drawItem(p, newCItem, rect);
      }
      else {  
            p.save();
            setPainter(p);
           
            if (xmag <= 0) {
                  x -= 1;
                  w += 2;
                  x = (x + xpos + rmapx(xorg)) * (-xmag);
                  w = w * (-xmag);
                  }
            else {
                  x = (x + xpos + rmapx(xorg)) / xmag;
                  w = (w + xmag - 1) / xmag;
                  x -= 1;
                  w += 2;
                  }
            if (ymag <= 0) {
                  y -= 1;
                  h += 2;
                  y = (y + ypos + rmapy(yorg)) * (-ymag);
                  h = h * (-ymag);
                  }
            else {
                  y = (rect.y() + ypos + rmapy(yorg))/ymag;
                  h = (rect.height()+ymag-1)/ymag;
                  y -= 1;
                  h += 2;
                  }

            if (x < 0)
                  x = 0;
            if (y < 0)
                  y = 0;
            x2 = x + w;

            QRect new_rect(x, y, w, h);
            drawCanvas(p, new_rect);
            p.restore();

            //---------------------------------------------------
            // draw Canvas Items
            //---------------------------------------------------
            
            for(iCItem i = items.begin(); i != items.end(); ++i)
            { 
              CItem* ci = i->second;
              // NOTE Optimization: For each item call this once now, then use cached results later via cachedHasHiddenEvents().
              // Not required for now.
              //ci->part()->hasHiddenEvents();
              
              // Draw items from other parts behind all others.
              // Only for items with events (not arranger parts).
              if(!ci->event().empty() && ci->part() != curPart)
                list1.push_back(ci);    
              else if(!ci->isMoving() && (ci->event().empty() || ci->part() == curPart))
              {
                // Draw selected parts in front of all others.
                if(ci->isSelected()) 
                  list4.push_back(ci);
                else  
                  // Draw unselected parts.
                  list2.push_back(ci);
              }  
            }
            int i;
            int sz = list1.size();
            for(i = 0; i != sz; ++i) 
              drawItem(p, list1[i], rect);
            sz = list2.size();
            for(i = 0; i != sz; ++i) 
              drawItem(p, list2[i], rect);
            sz = list4.size();
            for(i = 0; i != sz; ++i) 
              drawItem(p, list4[i], rect);

            // Draw items being moved, a special way in their original location.
            for (iCItem i = moving.begin(); i != moving.end(); ++i) 
                        drawItem(p, i->second, rect);
            
            // Draw special top item for new recordings etc.
            drawTopItem(p, new_rect);

            // Draw special new item for first-time placement.
            // It is not in the item list yet. It will be added when mouse released.
            if(newCItem)
              drawItem(p, newCItem, rect);
            
            p.save();
            setPainter(p);
      }

      //---------------------------------------------------
      //    draw marker
      //---------------------------------------------------

      //p.save();
      bool wmtxen = p.worldMatrixEnabled();
      p.setWorldMatrixEnabled(false);
      
      int my = mapy(y);
      int my2 = mapy(y + h);
      MusECore::MarkerList* marker = MusEGlobal::song->marker();
      for (MusECore::iMarker m = marker->begin(); m != marker->end(); ++m) {
            int xp = m->second.tick();
            if (xp >= x && xp < x+w) {
                  p.setPen(Qt::green);
                  p.drawLine(mapx(xp), my, mapx(xp), my2);
                  }
            }

      //---------------------------------------------------
      //    draw location marker
      //---------------------------------------------------

      p.setPen(Qt::blue);
      int mx;
      if (pos[1] >= unsigned(x) && pos[1] < unsigned(x2)) {
            mx = mapx(pos[1]);
            p.drawLine(mx, my, mx, my2);
            }
      if (pos[2] >= unsigned(x) && pos[2] < unsigned(x2)) {
            mx = mapx(pos[2]);
            p.drawLine(mx, my, mx, my2);
            }
      p.setPen(Qt::red);
      if (pos[0] >= unsigned(x) && pos[0] < unsigned(x2)) {
            mx = mapx(pos[0]);
            p.drawLine(mx, my, mx, my2);
            }
      
      if(drag == DRAG_ZOOM)
        p.drawPixmap(mapFromGlobal(global_start), *zoomAtIcon);
      
      //p.restore();
      //p.setWorldMatrixEnabled(true);
      p.setWorldMatrixEnabled(wmtxen);
      
      //---------------------------------------------------
      //    draw lasso
      //---------------------------------------------------

      if (drag == DRAG_LASSO) {
            p.setWorldMatrixEnabled(false);
            p.setPen(Qt::blue);
            p.setBrush(Qt::NoBrush);
            QRect _r(mapx(lasso.topLeft().x()), mapy(lasso.topLeft().y()), mapx(lasso.topRight().x()) - mapx(lasso.topLeft().x()), mapy(lasso.bottomLeft().y()) - mapy(lasso.topLeft().y()));
            p.drawRect(_r);
            p.setWorldMatrixEnabled(wmtxen);
            }
      
      //---------------------------------------------------
      //    draw outlines of potential drop places of moving items
      //---------------------------------------------------
      
      if(virt()) 
      {
        for(iCItem i = moving.begin(); i != moving.end(); ++i) 
          drawMoving(p, i->second, rect);
      }
      else 
      {  
        p.restore();
        for(iCItem i = moving.begin(); i != moving.end(); ++i) 
          drawMoving(p, i->second, rect);
        setPainter(p);
      }
}

#define WHEEL_STEPSIZE 40
#define WHEEL_DELTA   120

//---------------------------------------------------------
//   wheelEvent
//---------------------------------------------------------
void Canvas::wheelEvent(QWheelEvent* ev)
{
    int keyState = ev->modifiers();

    bool shift      = keyState & Qt::ShiftModifier;
    bool ctrl       = keyState & Qt::ControlModifier;

    if (shift) { // scroll horizontally
        int delta       = -ev->delta() / WHEEL_DELTA;
        int xpixelscale = 5*MusECore::fast_log10(rmapxDev(1));
        if (xpixelscale <= 0)
              xpixelscale = 1;
        int scrollstep = WHEEL_STEPSIZE * (delta);
        scrollstep = scrollstep / 10;
        int newXpos = xpos + xpixelscale * scrollstep;
        if (newXpos < 0)
              newXpos = 0;
        emit horizontalScroll((unsigned)newXpos);

    } else if (ctrl) {  // zoom horizontally
      emit horizontalZoom(ev->delta()>0, ev->globalPos());
    } else { // scroll vertically
        int delta       = ev->delta() / WHEEL_DELTA;
        int ypixelscale = rmapyDev(1);
        if (ypixelscale <= 0)
              ypixelscale = 1;
        int scrollstep = WHEEL_STEPSIZE * (-delta);
        scrollstep = scrollstep / 2;
        int newYpos = ypos + ypixelscale * scrollstep;
        if (newYpos < 0)
              newYpos = 0;
        emit verticalScroll((unsigned)newYpos);
    }
}

void Canvas::redirectedWheelEvent(QWheelEvent* ev)
      {
      wheelEvent(ev);
      }

//---------------------------------------------------------
//   deselectAll
//---------------------------------------------------------

void Canvas::deselectAll()
      {
      for (iCItem i = items.begin(); i != items.end(); ++i)
            i->second->setSelected(false);
      }

//---------------------------------------------------------
//   selectItem
//---------------------------------------------------------

void Canvas::selectItem(CItem* e, bool flag)
      {
      e->setSelected(flag);
      }

//---------------------------------------------------------
//   startMoving
//    copy selection-List to moving-List
//---------------------------------------------------------

void Canvas::startMoving(const QPoint& pos, DragType, bool rasterize)
      {
      for (iCItem i = items.begin(); i != items.end(); ++i) {
            if (i->second->isSelected()) {
                  i->second->setMoving(true);
                  moving.add(i->second);
                  }
            }
      moveItems(pos, 0, rasterize);
      }

//---------------------------------------------------------
//   moveItems
//    dir = 0     move in all directions
//          1     move only horizontal
//          2     move only vertical
//---------------------------------------------------------

void Canvas::moveItems(const QPoint& pos, int dir, bool rasterize)
      {
      int dp = y2pitch(pos.y()) - y2pitch(start.y());
      int dx = pos.x() - start.x();
      if (dir == 1)
            dp = 0;
      else if (dir == 2)
            dx = 0;
      for (iCItem i = moving.begin(); i != moving.end(); ++i) {
            int x = i->second->pos().x();
            int y = i->second->pos().y();
            int nx = x + dx;
            int ny;
            QPoint mp;
            ny = pitch2y(y2pitch(y) + dp);
            if(rasterize)
              mp = raster(QPoint(nx, ny));
            else
              mp = QPoint(nx, ny);
            
            if (i->second->mp() != mp) {
                  i->second->setMp(mp);
                  itemMoved(i->second, mp);
                  }
            }
      redraw();
      }

//---------------------------------------------------------
//   viewKeyPressEvent
//---------------------------------------------------------

void Canvas::viewKeyPressEvent(QKeyEvent* event)
      {
      keyPress(event);
      }

//---------------------------------------------------------
//   viewKeyReleaseEvent
//---------------------------------------------------------

void Canvas::viewKeyReleaseEvent(QKeyEvent* event)
      {
      keyRelease(event);
      }

//---------------------------------------------------------
//   viewMousePressEvent
//---------------------------------------------------------

void Canvas::viewMousePressEvent(QMouseEvent* event)
      {
      showCursor();  
      
      if (!mousePress(event))
      {
          setMouseGrab(false);
          return;
      }
      keyState = event->modifiers();
      button = event->button();
      //printf("viewMousePressEvent buttons:%x mods:%x button:%x\n", (int)event->buttons(), (int)keyState, event->button());
      
      // special events if right button is clicked while operations
      // like moving or drawing lasso is performed.
      if (event->buttons() & Qt::RightButton & ~(button)) {
          //printf("viewMousePressEvent special buttons:%x mods:%x button:%x\n", (int)event->buttons(), (int)keyState, event->button());
          setMouseGrab(false);
          switch (drag) {
              case DRAG_LASSO:
                drag = DRAG_OFF;
                redraw();
                return;
              case DRAG_MOVE:
                drag = DRAG_OFF;
                endMoveItems (start, MOVE_MOVE, 0);
                return;
              default:
                break;
          }
      }

      // ignore event if (another) button is already active:
//       if (event->buttons() & (Qt::LeftButton|Qt::RightButton|Qt::MidButton) & ~(button)) {  // REMOVE Tim. Trackinfo. Changed.
      if (event->buttons() ^ button) {
            //printf("viewMousePressEvent ignoring buttons:%x mods:%x button:%x\n", (int)event->buttons(), (int)keyState, event->button());
          setMouseGrab(false);
            return;
            }
            
      bool alt        = keyState & Qt::AltModifier;
      bool ctrl       = keyState & Qt::ControlModifier;
      
      start           = event->pos();
      ev_pos          = start;
      global_start    = event->globalPos();
      ev_global_pos   = global_start;
      
      curItem = findCurrentItem(start);

      if (curItem && (button == Qt::MidButton)) {
            deleteItem(start); // changed from "start drag" to "delete" by flo93
            drag = DRAG_DELETE;
            setCursor();
            }
      else if (button == Qt::RightButton) {
            if (curItem) {
                  if (ctrl && virt()) {       // Non-virt width is meaningless, such as drums.
                        drag = DRAG_RESIZE;
                        setCursor();
                        int dx = start.x() - curItem->x();
                        curItem->setWidth(dx);
                        start.setX(curItem->x());
                        deselectAll();
                        selectItem(curItem, true);
                        updateSelection();
                        redraw();
                        }
                  else {
                        itemPopupMenu = genItemPopup(curItem);
                        if (itemPopupMenu) {
                              QAction *act = itemPopupMenu->exec(QCursor::pos());
                              if (act && act->data().isValid())
                                    itemPopup(curItem, act->data().toInt(), start);
                              delete itemPopupMenu;
                              }
                        }
                  }
            else {
                  canvasPopupMenu = genCanvasPopup();
                  if (canvasPopupMenu) {
                        QAction *act = canvasPopupMenu->exec(QCursor::pos(), 0);
                        if (act)
                              canvasPopup(act->data().toInt());
                        delete canvasPopupMenu;
                        }
                  }
            }
      else if (button == Qt::LeftButton) {
            switch (_tool) {
                  case PointerTool:
                        if (curItem) {
                              itemPressed(curItem);
                              // Alt alone is usually reserved for moving a window in X11. Ignore shift + alt.
                              if (ctrl && !alt)
                                    drag = DRAG_COPY_START;
                              else if (ctrl && alt) 
                                    drag = DRAG_CLONE_START;
                              else if (!ctrl && !alt)
                                    drag = DRAG_MOVE_START;
                              }
                        else
                              drag = DRAG_LASSO_START;
                        setCursor();
                        break;

                  case RubberTool:
                        deleteItem(start);
                        drag = DRAG_DELETE;
                        setCursor();
                        break;

                  case PencilTool:
                        if (curItem) {
                                if(!virt()) { // Non-virt width is meaningless, such as drums.
                                  itemPressed(curItem);
                                  // Alt alone is usually reserved for moving a window in X11. Ignore shift + alt.
                                  if (ctrl && !alt)
                                        drag = DRAG_COPY_START;
                                  else if (ctrl && alt)
                                        drag = DRAG_CLONE_START;
                                  else if (!ctrl && !alt)
                                        drag = DRAG_MOVE_START;
                                  setCursor();
                                  break;
                                }
                                else {
                                  drag = DRAG_RESIZE;
                                  resizeDirection = RESIZE_TO_THE_RIGHT;
                                  if(supportsResizeToTheLeft){
                                     if(curItem->x() + (curItem->width() / 2) > ev_pos.x()){
                                        resizeDirection = RESIZE_TO_THE_LEFT;
                                     }
                                  }
                                  setCursor();
                                  if(resizeDirection == RESIZE_TO_THE_RIGHT){
                                    int dx = start.x() - curItem->x();
                                    curItem->setWidth(dx);
                                  }else{
                                    int endX = curItem->x() + curItem->width();
                                    end = QPoint(endX, curItem->y());
                                    resizeToTheLeft(ev_pos);
                                  }
                                  start = curItem->pos();
                                }
                                deselectAll();
                                if (curItem)
                                      selectItem(curItem, true);
                              }
                        else {
                              drag = DRAG_NEW;
                              setCursor();
                              curItem = newItem(start, keyState);
                              if (curItem)
                                    newCItem = curItem;
                              else {
                                    drag = DRAG_OFF;
                                    setCursor();
                                    }
                              deselectAll();
                              // selectItem() will be called in viewMouseReleaseEvent().
                              }
                        updateSelection();
                        redraw();
                        break;

                  case PanTool:
                        {
                          drag = DRAG_PAN;
                          setCursor();
                          if(MusEGlobal::config.borderlessMouse)
                          {
                            //  "It is almost never necessary to grab the mouse when using Qt, as Qt grabs 
                            //   and releases it sensibly. In particular, Qt grabs the mouse when a mouse 
                            //   button is pressed and keeps it until the last button is released."
                            //
                            // Apparently not. For some reason this was necessary. When the cursor is dragged
                            //  outside the window, holding left then pressing right mouse button COMPLETELY 
                            //  bypasses us, leaving the app's default right-click handler to popup, and leaving 
                            //  us in a really BAD state: mouse is grabbed (and hidden) and no way out !
                            //
                            // That is likely just how QWidget works, but here using global cursor overrides 
                            //  it is disasterous. TESTED: Yes, that is how other controls work. Hitting another 
                            //  button while the mouse has been dragged outside causes it to bypass us !
                            setMouseGrab(true); // CAUTION
                            
                            QRect r = QApplication::desktop()->screenGeometry();
                            ignore_mouse_move = true;      // Avoid recursion.
                            QCursor::setPos( QPoint(r.width()/2, r.height()/2) );
                            //ignore_mouse_move = false;
                          }
                        }
                        break;
                        
                  case ZoomTool:
                        {
                          drag = DRAG_ZOOM;
                          setCursor();
                          if(MusEGlobal::config.borderlessMouse)
                          {
                            setMouseGrab(true); // CAUTION
                            
                            QRect r = QApplication::desktop()->screenGeometry();
                            ignore_mouse_move = true;      // Avoid recursion.
                            QCursor::setPos( QPoint(r.width()/2, r.height()/2) );
                            //ignore_mouse_move = false;
                          }
                          // Update the small zoom drawing area
                          QPoint pt = mapFromGlobal(global_start);
                          update(pt.x(), pt.y(), zoomIcon->width(), zoomIcon->height());
                        }
                        break;

                  default:
                        break;
                  }
            }
      }

void Canvas::scrollTimerDone()
{
      //printf("Canvas::scrollTimerDone drag:%d doScroll:%d\n", drag, doScroll);
      if (doScroll && drag != DRAG_OFF && drag != DRAG_ZOOM)
      {
        //printf("Canvas::scrollTimerDone drag != DRAG_OFF && doScroll\n");
        int modifiers = QApplication::keyboardModifiers();
        bool ctrl  = modifiers & Qt::ControlModifier;
        bool meta  = modifiers & Qt::MetaModifier;
        bool alt   = modifiers & Qt::AltModifier;
        bool right_button = QApplication::mouseButtons() & Qt::RightButton;
        bool scrollDoResize = ((!ctrl && !right_button) || meta || alt) && virt();  // Non-virt width is meaningless, such as drums.
        int dx = 0;
        int dy = 0;
        bool doHMove = false;
        bool doVMove = false;
        switch(hscrollDir)
        {  
          case HSCROLL_RIGHT:
            switch(drag) 
            {
              case DRAG_NEW:
              case DRAG_RESIZE:
              case DRAGX_MOVE:
              case DRAGX_COPY:
              case DRAGX_CLONE:
              case DRAGY_MOVE:
              case DRAGY_COPY:
              case DRAGY_CLONE:
              case DRAG_MOVE:
              case DRAG_COPY:
              case DRAG_CLONE:
              case DRAG_PAN:
                emit horizontalScrollNoLimit(xpos + scrollSpeed);
                canScrollLeft = true;
                dx = rmapxDev(scrollSpeed);
                ev_pos.setX(ev_pos.x() + dx);
                doHMove = true;
              break;
              default:  
                if(canScrollRight)
                {
                  int curxpos = xpos;
                  emit horizontalScroll(xpos + scrollSpeed);
                  if(xpos <= curxpos)
                    canScrollRight = false;
                  else
                  {
                    canScrollLeft = true;
                    dx = rmapxDev(scrollSpeed);
                    ev_pos.setX(ev_pos.x() + dx);
                    doHMove = true;
                  }  
                }  
              break;
            }
          break;  
          case HSCROLL_LEFT:
            if(canScrollLeft)
            {
              int curxpos = xpos;
              emit horizontalScroll(xpos - scrollSpeed);
              if(xpos >= curxpos)
                canScrollLeft = false;
              else
              {
                canScrollRight = true;
                dx = -rmapxDev(scrollSpeed);
                ev_pos.setX(ev_pos.x() + dx);
                doHMove = true;
              }
            }    
          break; 
          default:
          break;   
        }
        switch(vscrollDir)
        {
          case VSCROLL_DOWN:
            if(canScrollDown)
            {
              int curypos = ypos;
              emit verticalScroll(ypos + scrollSpeed);
              if(ypos <= curypos)
                canScrollDown = false;
              else
              {
                canScrollUp = true;
                dy = rmapyDev(scrollSpeed);
                ev_pos.setY(ev_pos.y() + dy);
                doVMove = true;
              }
            }    
          break;  
          case VSCROLL_UP:
            if(canScrollUp)
            {
              int curypos = ypos;
              emit verticalScroll(ypos - scrollSpeed);
              if(ypos >= curypos)
                canScrollUp = false;
              else
              {
                canScrollDown = true;
                dy = -rmapyDev(scrollSpeed);
                ev_pos.setY(ev_pos.y() + dy);
                doVMove = true;
              } 
            }   
          break;
          default:
          break;
        }
        
        //printf("Canvas::scrollTimerDone doHMove:%d doVMove:%d\n", doHMove, doVMove);
        
        if(!doHMove && !doVMove)
        {
          delete scrollTimer;
          scrollTimer=NULL;
          doScroll = false;
          return;
        }  
        QPoint dist = ev_pos - start;
        switch(drag) 
        {
          case DRAG_MOVE:
          case DRAG_COPY:
          case DRAG_CLONE:
                moveItems(ev_pos, 0, false);
                break;
          case DRAGX_MOVE:
          case DRAGX_COPY:
          case DRAGX_CLONE:
                moveItems(ev_pos, 1, false);
                break;
          case DRAGY_MOVE:
          case DRAGY_COPY:
          case DRAGY_CLONE:
                moveItems(ev_pos, 2, false);
                break;
          case DRAG_LASSO:
                lasso = QRect(start.x(), start.y(), dist.x(), dist.y());
                redraw();
                break;

          case DRAG_NEW:
                if(newCItem)
                {
                  if((doHMove && !scrollDoResize) || doVMove)
                  {
                    int nx = newCItem->x();
                    int ny = newCItem->y();
                    if(doHMove && !scrollDoResize)
                      nx += dx;
                    if(nx < 0)
                      nx = 0;
                    if(doVMove)
                      ny += dy;
                    if(ny < 0)
                      ny = 0;
                    newCItem->move(QPoint(nx, ny));
                  }
                  if(scrollDoResize && doHMove)
                  {
                    int w = ev_pos.x() - newCItem->x();
                    if(w < 1)
                      w = 1;
                    newCItem->setWidth(w);
                  }
                  redraw();
                }
                break;
                
          case DRAG_RESIZE:
                if (curItem && doHMove) {
                      int w = ev_pos.x() - curItem->x();
                      if(w < 1)
                        w = 1;
                      curItem->setWidth(w);
                      redraw();
                      }
                break;
          default:  
                break;
        }
        //printf("Canvas::scrollTimerDone starting scrollTimer: Currently active?%d\n", scrollTimer->isActive());
        
        // Make sure to yield to other events, otherwise other events take a long time to reach us,
        //  causing scrolling to take a painfully long time to stop. Try up to 100 ms for each yield: 
        //qApp->processEvents(100);       // FIXME: Didn't help at all.
        scrollTimer->setSingleShot(true);
        scrollTimer->start(80);           // OK, setting a timeout 80 helped.
      }
      else 
      {
          //printf("Canvas::scrollTimerDone !(drag != DRAG_OFF && doScroll) deleting scrollTimer\n");
          delete scrollTimer;
          scrollTimer=NULL;
      }
}

//---------------------------------------------------------
//   viewMouseMoveEvent
//---------------------------------------------------------

void Canvas::viewMouseMoveEvent(QMouseEvent* event)
      {
      if(ignore_mouse_move)
      {
        ignore_mouse_move = false;
        event->accept();
        return;
      }
      //fprintf(stderr, "xpos=%d xorg=%d xmag=%d event->x=%d ->gx:%d mapx(xorg)=%d rmapx0=%d xOffset=%d rmapx(xOffset()=%d\n",
      //                 xpos,   xorg,   xmag,   event->x(), event->globalX(), mapx(xorg), rmapx(0), xOffset(), rmapx(xOffset()));
      //fprintf(stderr, "ypos=%d yorg=%d ymag=%d event->y=%d ->gy:%d mapy(yorg)=%d rmapy0=%d yOffset=%d rmapy(yOffset()=%d\n",
      //                 ypos,   yorg,   ymag,   event->y(), event->globalY(), mapy(yorg), rmapy(0), yOffset(), rmapy(yOffset()));

      QRect  screen_rect    = QApplication::desktop()->screenGeometry();
      QPoint screen_center  = QPoint(screen_rect.width()/2, screen_rect.height()/2);
      QPoint glob_dist      = event->globalPos() - ev_global_pos;
      QPoint glob_zoom_dist = MusEGlobal::config.borderlessMouse ? (event->globalPos() - screen_center) : glob_dist;
      QPoint last_dist      = event->pos() - ev_pos;
      
      ev_pos     = event->pos();
      QPoint dist  = ev_pos - start;
      int ax       = ABS(rmapx(dist.x()));
      int ay       = ABS(rmapy(dist.y()));
      bool isMoving  = (ax >= 2) || (ay > 2);
      int modifiers = event->modifiers();
      bool ctrl  = modifiers & Qt::ControlModifier;
      bool shift = modifiers & Qt::ShiftModifier;
      bool meta  = modifiers & Qt::MetaModifier;
      bool alt   = modifiers & Qt::AltModifier;
      bool right_button = event->buttons() & Qt::RightButton;

      // set scrolling variables: doScroll, scrollRight
      // No auto scroll in zoom mode or normal pan mode.
      if (drag != DRAG_OFF && drag != DRAG_ZOOM && (drag != DRAG_PAN || !MusEGlobal::config.borderlessMouse)) {  
            int ex = rmapx(event->x())+mapx(0);
            if(ex < 15 && (canScrollLeft || drag == DRAG_PAN))
              hscrollDir = (drag == DRAG_PAN ? HSCROLL_RIGHT : HSCROLL_LEFT);
            else  
            if(ex > (width() - 15))
              switch(drag) 
              {
                case DRAG_NEW:
                case DRAG_RESIZE:
                case DRAGX_MOVE:
                case DRAGX_COPY:
                case DRAGX_CLONE:
                case DRAGY_MOVE:
                case DRAGY_COPY:
                case DRAGY_CLONE:
                case DRAG_MOVE:
                case DRAG_COPY:
                case DRAG_CLONE:
                case DRAG_PAN:
                    hscrollDir = (drag == DRAG_PAN ? HSCROLL_LEFT : HSCROLL_RIGHT);
                break;
                default:
                  if(canScrollRight)
                    hscrollDir = (drag == DRAG_PAN ? HSCROLL_LEFT : HSCROLL_RIGHT);
                  else  
                    hscrollDir = HSCROLL_NONE;
                break;
              }
            else  
              hscrollDir = HSCROLL_NONE;
            
            int ey = rmapy(event->y())+mapy(0);
            if(ey < 15 && (canScrollUp || drag == DRAG_PAN))
              vscrollDir = (drag == DRAG_PAN ? VSCROLL_DOWN : VSCROLL_UP);
            else  
            if(ey > (height() - 15) && (canScrollDown || drag == DRAG_PAN))
              vscrollDir = (drag == DRAG_PAN ? VSCROLL_UP : VSCROLL_DOWN);
            else  
              vscrollDir = VSCROLL_NONE;

            if(hscrollDir != HSCROLL_NONE || vscrollDir != VSCROLL_NONE)
            {
              doScroll=true;
              if (!scrollTimer) 
              {
                  scrollTimer= new QTimer(this);
                  connect( scrollTimer, SIGNAL(timeout()), SLOT(scrollTimerDone()) );
                  scrollTimer->setSingleShot(true); // single-shot timer
                  scrollTimer->start(0); 
              }
            }
            else
                doScroll=false;
                
          }
      else
      {
        doScroll=false;
        
        canScrollLeft = true;
        canScrollRight = true;
        canScrollUp = true;
        canScrollDown = true;
      }

      switch (drag) {
            case DRAG_LASSO_START:
                  if (!isMoving)
                        break;
                  drag = DRAG_LASSO;
                  setCursor();
                  // proceed with DRAG_LASSO:
            case DRAG_LASSO:
                  {
                  lasso = QRect(start.x(), start.y(), dist.x(), dist.y());
                  // printf("xorg=%d xmag=%d event->x=%d, mapx(xorg)=%d rmapx0=%d xOffset=%d rmapx(xOffset()=%d\n",
                  //         xorg, xmag, event->x(),mapx(xorg), rmapx(0), xOffset(),rmapx(xOffset()));
                  }
                  redraw();
                  break;

            case DRAG_MOVE_START:
            case DRAG_COPY_START:
            case DRAG_CLONE_START:
                  if (!isMoving)
                        break;
                  if (keyState & Qt::ShiftModifier) {
                        if (ax > ay) {
                              if (drag == DRAG_MOVE_START)
                                    drag = DRAGX_MOVE;
                              else if (drag == DRAG_COPY_START)
                                    drag = DRAGX_COPY;
                              else
                                    drag = DRAGX_CLONE;
                              }
                        else {
                              if (drag == DRAG_MOVE_START)
                                    drag = DRAGY_MOVE;
                              else if (drag == DRAG_COPY_START)
                                    drag = DRAGY_COPY;
                              else
                                    drag = DRAGY_CLONE;
                              }
                        }
                  else {
                        if (drag == DRAG_MOVE_START)
                              drag = DRAG_MOVE;
                        else if (drag == DRAG_COPY_START)
                              drag = DRAG_COPY;
                        else
                              drag = DRAG_CLONE;
                        }
                  setCursor();
                  if (curItem && !curItem->isSelected()) {
                        if (drag == DRAG_MOVE)
                              deselectAll();
                        selectItem(curItem, true);
                        updateSelection();
                        redraw();
                        }
                  DragType dt;
                  if (drag == DRAG_MOVE)
                        dt = MOVE_MOVE;
                  else if (drag == DRAG_COPY)
                        dt = MOVE_COPY;
                  else
                        dt = MOVE_CLONE;
                  
                  startMoving(ev_pos, dt, !(keyState & Qt::ShiftModifier));
                  break;

            case DRAG_MOVE:
            case DRAG_COPY:
            case DRAG_CLONE:
                  if(!scrollTimer)
                    moveItems(ev_pos, 0, !shift);
                  break;

            case DRAGX_MOVE:
            case DRAGX_COPY:
            case DRAGX_CLONE:
                  if(!scrollTimer)
                    moveItems(ev_pos, 1, false);
                  break;

            case DRAGY_MOVE:
            case DRAGY_COPY:
            case DRAGY_CLONE:
                  if(!scrollTimer)
                    moveItems(ev_pos, 2, false);
                  break;

            case DRAG_NEW:
                  if(newCItem) {
                    if (last_dist.x()) {
                          if(((ctrl || right_button) && !meta && !alt) || !virt())  // Non-virt width is meaningless, such as drums.
                          {
                            int nx = ev_pos.x() - newCItem->width();  // Keep the cursor at the right edge.
                            if(nx < 0)
                              nx = 0;
                            if(!shift)
                            {
                              nx = raster(QPoint(nx, 0)).x();  // 0 is dummy, we want only x
                              if(nx < 0)
                                nx = 0;
                            }
                            newCItem->move(QPoint(nx, newCItem->y()));
                          }
                          else
                          {
                            int w = ev_pos.x() - newCItem->x();
                            if(w < 1)
                              w = 1;
                            newCItem->setWidth(w);
                          }
                          }
                    if (last_dist.y()) {
                          int x = newCItem->x();
                          int y = ev_pos.y();
                          int ny = pitch2y(y2pitch(y)) - yItemOffset();
                          QPoint pt = QPoint(x, ny);
                          newCItem->move(pt);
                          newCItem->setHeight(y2height(y));
                          itemMoved(newCItem, pt);
                          }
                    if (last_dist.x() || last_dist.y())
                      redraw();
                  }
                  break;

            case DRAG_RESIZE:
                  if (curItem && last_dist.x()) {
                        if(resizeDirection == RESIZE_TO_THE_RIGHT){
                           int w = ev_pos.x() - curItem->x();
                           if(w < 1)
                             w = 1;
                           curItem->setWidth(w);
                        }else{
                           resizeToTheLeft(ev_pos);
                        }
                        redraw();
                        }
                  break;
                  
            case DRAG_DELETE:
                  deleteItem(ev_pos);
                  break;

            case DRAG_PAN:
                  {
                    bool changed = false;
                    if((!shift || (shift && ctrl)) && glob_zoom_dist.x() != 0 && (!doScroll || hscrollDir == HSCROLL_NONE))  // Don't interfere if auto-scrolling
                    {
                      emit horizontalScroll(xpos - glob_zoom_dist.x());
                      changed = true;
                    }
                    if((!ctrl || (shift && ctrl)) && glob_zoom_dist.y() != 0 && (!doScroll || vscrollDir == VSCROLL_NONE))   // Don't interfere if auto-scrolling
                    {
                      emit verticalScroll(ypos - glob_zoom_dist.y());
                      changed = true;
                    }
                    if(MusEGlobal::config.borderlessMouse && changed)
                    {
                      ignore_mouse_move = true;      // Avoid recursion.
                      QCursor::setPos(screen_center);
                      //ignore_mouse_move = false;
                    }
                  }
                  break;
                  
            case DRAG_ZOOM:
                  if(glob_zoom_dist.x() != 0)   
                      emit horizontalZoom(glob_zoom_dist.x(), global_start);
                  //if(glob_zoom_dist.y() != 0)
                  //    emit verticalZoom(glob_zoom_dist.y(), global_start);  // TODO
                  if(MusEGlobal::config.borderlessMouse && (glob_zoom_dist.x() != 0 || glob_zoom_dist.y() != 0))
                  {
                    ignore_mouse_move = true;      // Avoid recursion.
                    QCursor::setPos(screen_center);
                    //ignore_mouse_move = false;
                  }
                  break;
                  
            case DRAG_OFF:
                  if(_tool == PencilTool){
                     if(findCurrentItem(ev_pos)){
                        QWidget::setCursor(QCursor(Qt::SizeHorCursor));
                        break;
                     }
                  }
                  setCursor();
                  break;
            }

      ev_global_pos = event->globalPos();

      if(drag != DRAG_ZOOM && (drag != DRAG_PAN || !MusEGlobal::config.borderlessMouse))
        mouseMove(event);
      }

//---------------------------------------------------------
//   viewMouseReleaseEvent
//---------------------------------------------------------

void Canvas::viewMouseReleaseEvent(QMouseEvent* event)
{
      doScroll = false;
      canScrollLeft = true;
      canScrollRight = true;
      canScrollUp = true;
      canScrollDown = true;
      if (event->buttons() & (Qt::LeftButton|Qt::RightButton|Qt::MidButton) & ~(event->button())) 
      {
        // Make sure this is done. See mousePressEvent.
        showCursor();
        setMouseGrab(false);
        return;
      }

      QPoint pos = event->pos();
      bool ctrl = event->modifiers() & Qt::ControlModifier;
      bool shift = event->modifiers() & Qt::ShiftModifier;
      bool redrawFlag = false;

      switch (drag) {
            case DRAG_MOVE_START:
            case DRAG_COPY_START:
            case DRAG_CLONE_START:
                  if (curItem && curItem->part() != curPart) {
                        curPart = curItem->part();
                        curPartId = curPart->sn();
                        curPartChanged();
                        }
                  if (!ctrl)
                        deselectAll();
                  if(curItem)
                  {
                    if (!shift) { //Select or deselect only the clicked item
                        selectItem(curItem, !(ctrl && curItem->isSelected()));
                        }
                    else { //Select or deselect all on the same pitch (e.g. same y-value)
                        bool selectionFlag = !(ctrl && curItem->isSelected());
                        for (iCItem i = items.begin(); i != items.end(); ++i)
                              if (i->second->y() == curItem->y() )
                                    selectItem(i->second, selectionFlag);
                        }
                  }

                  updateSelection();
                  redrawFlag = true;
                  if(curItem)
                    itemReleased(curItem, curItem->pos());
                  break;
            case DRAG_COPY:
                  endMoveItems(pos, MOVE_COPY, 0, !shift);
                  break;
            case DRAGX_COPY:
                  endMoveItems(pos, MOVE_COPY, 1, false);
                  break;
            case DRAGY_COPY:
                  endMoveItems(pos, MOVE_COPY, 2, false);
                  break;
            case DRAG_MOVE:
                  endMoveItems(pos, MOVE_MOVE, 0, !shift);
                  break;
            case DRAGX_MOVE:
                  endMoveItems(pos, MOVE_MOVE, 1, false);
                  break;
            case DRAGY_MOVE:
                  endMoveItems(pos, MOVE_MOVE, 2, false);
                  break;
            case DRAG_CLONE:
                  endMoveItems(pos, MOVE_CLONE, 0, !shift);
                  break;
            case DRAGX_CLONE:
                  endMoveItems(pos, MOVE_CLONE, 1, false);
                  break;
            case DRAGY_CLONE:
                  endMoveItems(pos, MOVE_CLONE, 2, false);
                  break;
            case DRAG_OFF:
                  break;
            case DRAG_RESIZE:
                  if(curItem){                    
                    if(resizeDirection == RESIZE_TO_THE_LEFT){
                       QPoint rpos = QPoint(raster(pos).x(), curItem->y());
                       resizeToTheLeft(rpos);
                       curItem->move(start);
                    }
                    resizeItem(curItem, shift, ctrl);
                    updateSelection();
                    redraw();
                  }
                  break;
            case DRAG_NEW:
                  if(newCItem)
                  {
                    items.add(newCItem);
                    curItem = newCItem;
                    newCItem = NULL;
                    itemReleased(curItem, curItem->pos());
                    newItem(curItem, shift);
                    redrawFlag = true;
                  }
                  break;
            case DRAG_LASSO_START:
                  lasso.setRect(-1, -1, -1, -1);
                  if (!ctrl)
                        deselectAll();
                  updateSelection();
                  redrawFlag = true;
                  break;

            case DRAG_LASSO:
                  if (!ctrl)
                        deselectAll();
                  lasso = lasso.normalized();
                  selectLasso(ctrl);
                  updateSelection();
                  redrawFlag = true;
                  break;

            case DRAG_DELETE:
                  break;
                  
            case DRAG_PAN:
                  if(MusEGlobal::config.borderlessMouse)
                  {
                    pos = global_start;
                    ignore_mouse_move = true;      // Avoid recursion.
                    QCursor::setPos(global_start);
                    //ignore_mouse_move = false;
                  }
                  break;
                  
            case DRAG_ZOOM:
                  if(MusEGlobal::config.borderlessMouse)
                  {
                    pos = global_start;
                    ignore_mouse_move = true;      // Avoid recursion.
                    QCursor::setPos(global_start);
                    //ignore_mouse_move = false;
                  }
                  break;
            }
      //printf("Canvas::viewMouseReleaseEvent setting drag to DRAG_OFF\n");

      // Just in case it was somehow forgotten:
      if(newCItem)
      {
        if(newCItem->event().empty() && newCItem->part()) // Was it a new part, with no event?
          delete newCItem->part();
        delete newCItem;
        newCItem = NULL;
      }
      
      if(drag == DRAG_ZOOM) // Update the small zoom drawing area
      {
        drag = DRAG_OFF;
        QPoint pt = mapFromGlobal(global_start);
        update(pt.x(), pt.y(), zoomIcon->width(), zoomIcon->height());
      }
      
      drag = DRAG_OFF;
      if (redrawFlag)
            redraw();

      // Make sure this is done. See mousePressEvent.
      setCursor(); // Calls showCursor().
      setMouseGrab(false);

      mouseRelease(pos);
}

//---------------------------------------------------------
//   selectLasso
//---------------------------------------------------------

void Canvas::selectLasso(bool toggle)
      {
      int n = 0;
      if (virt()) {
            for (iCItem i = items.begin(); i != items.end(); ++i) {
                  if (i->second->intersects(lasso)) {
                        selectItem(i->second, !(toggle && i->second->isSelected()));
                        ++n;
                        }
                  }
            }
      else {
            for (iCItem i = items.begin(); i != items.end(); ++i) {
                  QRect box = i->second->bbox();
                  int x = rmapxDev(box.x());
                  int y = rmapyDev(box.y());
                  int w = rmapxDev(box.width());
                  int h = rmapyDev(box.height());
                  QRect r(x, y, w, h);
                  r.translate(i->second->pos().x(), i->second->pos().y());
                  if (r.intersects(lasso)) {
                        selectItem(i->second, !(toggle && i->second->isSelected()));
                        ++n;
                        }
                  }
            }

      if (n) {
            updateSelection();
            redraw();
            }
      }


//---------------------------------------------------------
//   getCurrentDrag
//   returns 0 if there is no drag operation
//---------------------------------------------------------

int Canvas::getCurrentDrag()
      {
      //printf("getCurrentDrag=%d\n", drag);
      return drag;
      }

//---------------------------------------------------------
//   deleteItem
//---------------------------------------------------------

void Canvas::deleteItem(const QPoint& p)
      {
      if (virt()) {
            for (iCItem i = items.begin(); i != items.end(); ++i) {
                  if (i->second->contains(p)) {
                        selectItem(i->second, false);
                        if (!deleteItem(i->second)) {
                              if (drag == DRAG_DELETE)
                                    drag = DRAG_OFF;
                              }
                        break;
                        }
                  }
            }
      else {
            for (iCItem i = items.begin(); i != items.end(); ++i) {
                  QRect box = i->second->bbox();
                  int x = rmapxDev(box.x());
                  int y = rmapyDev(box.y());
                  int w = rmapxDev(box.width());
                  int h = rmapyDev(box.height());
                  QRect r(x, y, w, h);
                  r.translate(i->second->pos().x(), i->second->pos().y());
                  if (r.contains(p)) {
                        if (deleteItem(i->second)) {
                              selectItem(i->second, false);
                              }
                        break;
                        }
                  }
            }
      }

//---------------------------------------------------------
//   setTool
//---------------------------------------------------------

void Canvas::setTool(int t)
      {
      if (_tool == Tool(t))
            return;
      _tool = Tool(t);
      setCursor();
      }

//---------------------------------------------------------
//   setCursor
//---------------------------------------------------------

CItem *Canvas::findCurrentItem(const QPoint &cStart)
{
   //---------------------------------------------------
   //    set curItem to item mouse is pointing
   //    (if any)
   //---------------------------------------------------

   CItem *item = 0;
   if (virt())
      item = items.find(cStart);
   else {
      for (ciCItem i = items.begin(); i != items.end(); ++i) {
         QRect box = i->second->bbox();
         int x = rmapxDev(box.x());
         int y = rmapyDev(box.y());
         int w = rmapxDev(box.width());
         int h = rmapyDev(box.height());
         QRect r(x, y, w, h);
         r.translate(i->second->pos().x(), i->second->pos().y());
         if (r.contains(cStart)) {
            if(i->second->isSelected())
              return i->second;
            else
            {
              if(!item)
                item = i->second;
            }
         }
      }
   }
   return item;
}

void Canvas::resizeToTheLeft(const QPoint &pos)
{
   int newX = pos.x();
   if(end.x() - newX < 1)
      newX = end.x() - 1;
   int dx = end.x() - newX;
   curItem->setWidth(dx);
   QPoint mp(newX, curItem->y());
   curItem->setMp(mp);
   curItem->move(mp);
   //fprintf(stderr, "newX=%d, dx=%d\n", newX, dx);
}

void Canvas::setCursor()
      {
      showCursor();
      switch (drag) {
            case DRAGX_MOVE:
            case DRAGX_COPY:
            case DRAGX_CLONE:
                  QWidget::setCursor(QCursor(Qt::SizeHorCursor));
                  break;

            case DRAGY_MOVE:
            case DRAGY_COPY:
            case DRAGY_CLONE:
                  QWidget::setCursor(QCursor(Qt::SizeVerCursor));
                  break;

            case DRAG_MOVE:
            case DRAG_COPY:
            case DRAG_CLONE:
	          // Bug in KDE cursor theme? On some distros this cursor is actually another version of a closed hand! From 'net:
                  // "It might be a problem in the distribution as Qt uses the cursor that is provided by X.org/xcursor extension with name "size_all".
	          //  We fixed this issue by setting the KDE cursor theme to "System theme" "
                  QWidget::setCursor(QCursor(Qt::SizeAllCursor));  
                  break;

            case DRAG_RESIZE:
                  QWidget::setCursor(QCursor(Qt::SizeHorCursor));
                  break;

            case DRAG_PAN:
                  if(MusEGlobal::config.borderlessMouse)
                    //QWidget::setCursor(QCursor(Qt::BlankCursor));  // Hide it.  // REMOVE Tim. Trackinfo.
                    showCursor(false); // CAUTION
                  else
                    QWidget::setCursor(QCursor(Qt::ClosedHandCursor));
                  break;
                  
            case DRAG_ZOOM:
                  if(MusEGlobal::config.borderlessMouse)
                    //QWidget::setCursor(QCursor(Qt::BlankCursor));  // Hide it.  // REMOVE Tim. Trackinfo.
                    showCursor(false); // CAUTION
                  break;
                  
            case DRAG_DELETE:
            case DRAG_COPY_START:
            case DRAG_CLONE_START:
            case DRAG_MOVE_START:
            case DRAG_NEW:
            case DRAG_LASSO_START:
            case DRAG_LASSO:
            case DRAG_OFF:
                  switch(_tool) {
                        case PencilTool:
                              QWidget::setCursor(QCursor(*pencilIcon, 4, 15));
                              break;
                        case RubberTool:
                              QWidget::setCursor(QCursor(*deleteIcon, 4, 15));
                              break;
                        case GlueTool:
                              QWidget::setCursor(QCursor(*glueIcon, 4, 15));
                              break;
                        case CutTool:
                              QWidget::setCursor(QCursor(*cutIcon, 4, 15));
                              break;
                        case MuteTool:
                              QWidget::setCursor(QCursor(*editmuteIcon, 4, 15));
                              break;
                        case AutomationTool:
                              QWidget::setCursor(QCursor(Qt::PointingHandCursor));
                              break;
                        case PanTool:
                              QWidget::setCursor(QCursor(Qt::OpenHandCursor));
                              break;
                        case ZoomTool:
                              QWidget::setCursor(QCursor(*zoomAtIcon, 0, 0));
                              break;
                        default:
                              QWidget::setCursor(QCursor(Qt::ArrowCursor));
                              break;
                        }
                  break;
            }
      }

//---------------------------------------------------------
//   keyPress
//---------------------------------------------------------

void Canvas::keyPress(QKeyEvent* event)
      {
      event->ignore();
      }

//---------------------------------------------------------
//   keyRelease
//---------------------------------------------------------

void Canvas::keyRelease(QKeyEvent* event)
      {
      event->ignore();
      }

//---------------------------------------------------------
//   isSingleSelection
//---------------------------------------------------------

bool Canvas::isSingleSelection() const
      {
      return selectionSize() == 1;
      }

//---------------------------------------------------------
//   itemsAreSelected
//---------------------------------------------------------

bool Canvas::itemsAreSelected() const
      {
      for (ciCItem i = items.begin(); i != items.end(); ++i) {
            if (i->second->isSelected())
                  return true;
            }
      return false;
      }

//---------------------------------------------------------
//   selectionSize
//---------------------------------------------------------

int Canvas::selectionSize() const
      {
      int n = 0;
      for (ciCItem i = items.begin(); i != items.end(); ++i) {
            if (i->second->isSelected())
                  ++n;
            }
      return n;
      }

//---------------------------------------------------------
//   genCanvasPopup
//   Add the list of available tools to a popup menu
//   menu parameter can be NULL meaning create a menu here
//---------------------------------------------------------

QMenu* Canvas::genCanvasPopup(QMenu* menu)
      {
      if (canvasTools == 0)
            return 0;
      QMenu* r_menu = menu;
      if(!r_menu)
        r_menu = new QMenu(this);
      QAction* act0 = 0;

      r_menu->addAction(new MenuTitleItem(tr("Tools:"), r_menu));
      
      for (unsigned i = 0; i < gNumberOfTools; ++i) {
            if ((canvasTools & (1 << i))==0)
                  continue;
            QAction* act = r_menu->addAction(QIcon(**toolList[i].icon), tr(toolList[i].tip));

            if (MusEGui::toolShortcuts.contains(1 << i)) {
                act->setShortcut(MusEGui::shortcuts[MusEGui::toolShortcuts[1 << i]].key);
            }
            //
            act->setData(TOOLS_ID_BASE + i);
            act->setCheckable(true);
            act->setChecked((1 << i) == _tool);
            if (!act0)
                  act0 = act;
            }
      if(!menu)  // Don't interefere with supplied menu's current item
        r_menu->setActiveAction(act0);
      return r_menu;
      }

//---------------------------------------------------------
//   canvasPopup
//---------------------------------------------------------

void Canvas::canvasPopup(int n)
      {
        if(n >= TOOLS_ID_BASE)
        {
          n -= TOOLS_ID_BASE;
          int t = 1 << n;
          setTool(t);
          emit toolChanged(t);
        }
      }

void Canvas::setCurrentPart(MusECore::Part* part)
{
  curItem = NULL;
  deselectAll();
  curPart = part;
  curPartId = curPart->sn();
  curPartChanged();
}

} // namespace MusEGui
