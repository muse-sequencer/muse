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
#include "muse_math.h"

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
#include <QScreen>

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
#include "helper.h"

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
      resizeDirection= RESIZE_TO_THE_RIGHT;

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

//---------------------------------------------------------
//   lassoToRegion
//   static
//---------------------------------------------------------

void Canvas::lassoToRegion(const QRect& r_in, QRegion& rg_out) const
{
  const QRect mr = map(r_in);
  const int x = mr.x();
  const int y = mr.y();
  const int w = mr.width();
  const int h = mr.height();
  
  const int x_line_off = 0;
  const int y_line_off = 0;
  const int line_w = 1;
  const int line_h = 1;
  
  // Clear the given region.
  rg_out = QRegion();
  // Top line.
  rg_out += QRect(x, y - y_line_off, w + line_w, line_h);
  // Right line.
  rg_out += QRect(x + w - x_line_off, y, line_w, h + line_h);
  // Bottom line.
  rg_out += QRect(x, y + h - y_line_off, w + line_w, line_h);
  // Left line.
  rg_out += QRect(x - x_line_off, y, line_w, h + line_h);
  
// For testing...
//       const QVector<QRect> rects = rg_out.rects();
//       const int rg_sz = rects.size();
//       fprintf(stderr, "Region rect count:%d\n", rg_sz);
//       int rg_r_cnt = 0;
//       fprintf(stderr, "Region rect count:%d\n", rg_sz);
//       for(int i = 0; i < rg_sz; ++i, ++rg_r_cnt)
//       {
//         const QRect& rg_r = rects.at(i);
//         fprintf(stderr, "  #%d: x:%d y:%d w:%d h:%d\n", rg_r_cnt, rg_r.x(), rg_r.y(), rg_r.width(), rg_r.height());
//       }   
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
//   drawMarkers
//---------------------------------------------------------

void Canvas::drawMarkers(QPainter& p, const QRect& mr, const QRegion&)
{
      const int mx = mr.x();
      const int my = mr.y();
      const int mw = mr.width();
      const int mh = mr.height();
      const int my_2 = my + mh;
      
      const ViewXCoordinate vx(mx, true);
      const ViewWCoordinate vw(mw, true);
      const ViewXCoordinate vx_2(mx + mw, true);
      
      QPen pen;
      pen.setCosmetic(true);
      
      MusECore::MarkerList* marker = MusEGlobal::song->marker();
      pen.setColor(Qt::green);
      p.setPen(pen);
      for (MusECore::iMarker m = marker->begin(); m != marker->end(); ++m) {
            const ViewXCoordinate xp(m->second.tick(), false);
            if (isXInRange(xp, vx, vx_2)) {
                  const int mxp = asMapped(xp)._value;
                  p.drawLine(mxp, my, mxp, my_2);
                  }
            }
}
      
//---------------------------------------------------------
//   draw
//---------------------------------------------------------

void Canvas::draw(QPainter& p, const QRect& mr, const QRegion& mrg)
{
//      printf("draw canvas %x virt %d\n", this, virt());

// For testing...
//       const QVector<QRect> rects = mrg.rects();
//       const int rg_sz = rects.size();
//       fprintf(stderr, "Canvas::draw: virt:%d rect: x:%d y:%d w:%d h:%d region rect count:%d\n",
//               virt(), mr.x(), mr.y(), mr.width(), mr.height(), rg_sz);
//       int rg_r_cnt = 0;
//       for(int i = 0; i < rg_sz; ++i, ++rg_r_cnt)
//       {
//         const QRect& rg_r = rects.at(i);
//         fprintf(stderr, "  #%d: x:%d y:%d w:%d h:%d\n", rg_r_cnt, rg_r.x(), rg_r.y(), rg_r.width(), rg_r.height());
//       }   

      const int mx = mr.x();
      const int my = mr.y();
      const int mw = mr.width();
      const int mh = mr.height();
      const int mx_2 = mx + mw;
      const int my_2 = my + mh;
      
      const ViewXCoordinate vx(mx, true);
      const ViewWCoordinate vw(mw, true);
      const ViewXCoordinate vx_2(mx + mw, true);
      
      int ux_2lim = mapxDev(mx_2);
      if(ux_2lim <= 0)
        //x2_lim = 1;
        ux_2lim = 0;
      
      // Force it to +1 so that all the left-most items, or items immediately to the right,
      //  get a chance to update - this is important since for example the parts on the
      //  part canvas want to draw a two-pixel wide border which for an item at position x=0
      //  actually begins at x=-1 and needs to include that small adjustment during updates...
      ux_2lim += rmapxDev(1);
      
      std::vector<CItem*> list1;
      std::vector<CItem*> list2;
      std::vector<CItem*> list4;

      if (virt()) {
            drawCanvas(p, mr, mrg);

            //---------------------------------------------------
            // draw Canvas Items
            //---------------------------------------------------

            // ... and we want the upper bound, not lower bound, so that items immediately
            //  to the right can be updated.
            iCItem to(items.upper_bound(ux_2lim));
            
// For testing...
//             fprintf(stderr, "Canvas::draw: virt:%d x2:%d ux2_lim:%d\n", virt(), mx_2, ux_2lim);
//             if(to == items.end())
//               fprintf(stderr, "...item not found\n");
//             else
//               fprintf(stderr, "...item found\n");
            
            int ii = 0;
            for(iCItem i = items.begin(); i != to; ++i, ++ii)
            { 
              CItem* ci = i->second;
              // NOTE Optimization: For each item call this once now, then use cached results later via cachedHasHiddenEvents().
              // Not required for now.
              //ci->part()->hasHiddenEvents();
              
// For testing...
//               fprintf(stderr, "...item:%d bbox x:%d y:%d w:%d h:%d\n", ii, ci->bbox().x(), ci->bbox().y(), ci->bbox().width(), ci->bbox().height());
              
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
            
            // Draw non-current part backgrounds behind all others:
            drawParts(p, false, mr, mrg);

            int i;
            int sz = list1.size();
            for(i = 0; i != sz; ++i) 
              drawItem(p, list1[i], mr, mrg);

            // Draw current part background in front of all others:
            drawParts(p, true, mr, mrg);

            sz = list2.size();
            for(i = 0; i != sz; ++i) 
              drawItem(p, list2[i], mr, mrg);
            sz = list4.size();
            for(i = 0; i != sz; ++i) 
              drawItem(p, list4[i], mr, mrg);
            
            // Draw items being moved, a special way in their original location.
            to = moving.lower_bound(ux_2lim);
            for (iCItem i = moving.begin(); i != to; ++i) 
                  drawItem(p, i->second, mr, mrg);

            // Draw special top item for new recordings etc.
            drawTopItem(p,mr, mrg);

            // Draw special new item for first-time placement.
            // It is not in the item list yet. It will be added when mouse released.
            if(newCItem)
              drawItem(p, newCItem, mr, mrg);
      }
      else {  
            p.save();
            setPainter(p);
           
            drawCanvas(p, mr, mrg);
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

            // Draw non-current part backgrounds behind all others:
            drawParts(p, false, mr, mrg);

            int i;
            int sz = list1.size();
            for(i = 0; i != sz; ++i) 
              drawItem(p, list1[i], mr, mrg);

            // Draw current part background in front of all others:
            drawParts(p, true, mr, mrg);

            sz = list2.size();
            for(i = 0; i != sz; ++i) 
              drawItem(p, list2[i], mr, mrg);
            sz = list4.size();
            for(i = 0; i != sz; ++i) 
              drawItem(p, list4[i], mr, mrg);

            // Draw items being moved, a special way in their original location.
            for (iCItem i = moving.begin(); i != moving.end(); ++i) 
                        drawItem(p, i->second, mr, mrg);
            
            // Draw special top item for new recordings etc.
            drawTopItem(p, mr, mrg);

            // Draw special new item for first-time placement.
            // It is not in the item list yet. It will be added when mouse released.
            if(newCItem)
              drawItem(p, newCItem, mr, mrg);
            
            p.save();
            setPainter(p);
      }

      
      QPen pen;
      pen.setCosmetic(true);
      
      //---------------------------------------------------
      //    draw marker
      //---------------------------------------------------

      //p.save();
      bool wmtxen = p.worldMatrixEnabled();
      p.setWorldMatrixEnabled(false);
      
      drawMarkers(p, mr, mrg);
       
       
      //---------------------------------------------------
      //    draw location marker
      //---------------------------------------------------

      pen.setColor(Qt::blue);
      p.setPen(pen);
      int mlx;
      ViewXCoordinate lxp0(pos[0], false);
      ViewXCoordinate lxp1(pos[1], false);
      ViewXCoordinate lxp2(pos[2], false);
      if (isXInRange(lxp1, vx, vx_2)) {
            mlx = asMapped(lxp1)._value;
            p.drawLine(mlx, my, mlx, my_2);
            }
      if (isXInRange(lxp2, vx, vx_2)) {
            mlx = asMapped(lxp2)._value;
            p.drawLine(mlx, my, mlx, my_2);
            }
      // Draw the red main position cursor last, on top of the others.
      pen.setColor(Qt::red);
      p.setPen(pen);
      
// For testing...
//       fprintf(stderr, "...location marker: pos[0]%d x:%d x2:%d\n",
//               pos[0], x, x2);
      
      if (isXInRange(lxp0, vx, vx_2)) {
            mlx = asMapped(lxp0)._value;
            
// For testing...
//             fprintf(stderr, "...location marker in range. Drawing line at mx:%d my:%d mx:%d my2:%d\n",
//                 mx, my, mx, my2);
            
            p.drawLine(mlx, my, mlx, my_2);
            }
      
      if(drag == DRAG_ZOOM)
        p.drawPixmap(mapFromGlobal(global_start), zoomAtIconSVG->pixmap(QSize(MusEGlobal::config.cursorSize, MusEGlobal::config.cursorSize)));
      
      //p.restore();
      //p.setWorldMatrixEnabled(true);
      p.setWorldMatrixEnabled(wmtxen);
      
      //---------------------------------------------------
      //    draw lasso
      //---------------------------------------------------

      if (drag == DRAG_LASSO) {
            p.setWorldMatrixEnabled(false);
            pen.setColor(Qt::blue);
            p.setPen(pen);
            p.setBrush(Qt::NoBrush);
            QRect _r(map(lasso));
            p.drawRect(_r);
            p.setWorldMatrixEnabled(wmtxen);
            }
      
      //---------------------------------------------------
      //    draw outlines of potential drop places of moving items
      //---------------------------------------------------
      
      if(virt()) 
      {
        for(iCItem i = moving.begin(); i != moving.end(); ++i) 
          drawMoving(p, i->second, mr, mrg);
      }
      else 
      {  
        p.restore();
        for(iCItem i = moving.begin(); i != moving.end(); ++i) 
          drawMoving(p, i->second, mr, mrg);
        setPainter(p);
      }
}

#define HR_WHEEL_STEPSIZE 2
#define WHEEL_STEPSIZE 50
//#define WHEEL_DELTA   120

//---------------------------------------------------------
//   wheelEvent
//---------------------------------------------------------
void Canvas::wheelEvent(QWheelEvent* ev)
{
    // NOTE: X allows for Alt key + wheel, which changes from vertical
    //        wheel values to horizontal values! Works in any window.
  
    int keyState = ev->modifiers();

    QPoint delta       = ev->pixelDelta();  // WHEEL_DELTA;
    
    int wheel_step_sz = 0;
    if(delta.isNull())
    {
      delta = ev->angleDelta() / 8;
      if(delta.isNull())
        return;
      delta /= 15;
      wheel_step_sz = WHEEL_STEPSIZE;
    }
    else
    {
      delta /= 2;
      wheel_step_sz = HR_WHEEL_STEPSIZE;
    }

    bool shift      = keyState & Qt::ShiftModifier;
    bool ctrl       = keyState & Qt::ControlModifier;

    if (ctrl) {  // zoom horizontally

      int d = 0; 
      if(delta.x() != 0)
        d = delta.x();
      else if(delta.y() != 0)
        d = delta.y();
      if(d != 0)
        emit horizontalZoom(d > 0, ev->globalPos());
      return;
    }

    if (shift  || delta.x() != 0) { // scroll horizontally

        int scrolldelta = - delta.x();
        if (shift) {
          scrolldelta = - delta.y();
        }

        int xpixelscale = 5*MusECore::fast_log10(rmapxDev(1));
        if (xpixelscale <= 0) {
          xpixelscale = 1;
        }
        int scrollstep = wheel_step_sz * (scrolldelta);
        scrollstep = scrollstep / 10;
        int newXpos = xpos + xpixelscale * scrollstep;

        if (newXpos < 0) {
          newXpos = 0;
        }

        emit horizontalScroll((unsigned)newXpos);

    }

    if (!shift && delta.y() != 0) { // scroll vertically

        int scrolldelta = delta.y();
        int ypixelscale = rmapyDev(1);

        if (ypixelscale <= 0)
              ypixelscale = 1;

        int scrollstep = wheel_step_sz * (-scrolldelta);
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

void Canvas::startMoving(const QPoint& pos, int dir, DragType, bool rasterize)
      {
      for (iCItem i = items.begin(); i != items.end(); ++i) {
            if (i->second->isSelected()) {
                  i->second->setMoving(true);
                  moving.add(i->second);
                  }
            }
      moveItems(pos, dir, rasterize);
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
      if (!mousePress(event))
      {
          cancelMouseOps();
          return;
      }
      keyState = event->modifiers();
      button = event->button();
      //printf("viewMousePressEvent buttons:%x mods:%x button:%x\n", (int)event->buttons(), (int)keyState, event->button());
      
      // special events if right button is clicked while operations
      // like moving or drawing lasso is performed.
      if (event->buttons() & Qt::RightButton & ~(button)) {
          //printf("viewMousePressEvent special buttons:%x mods:%x button:%x\n", (int)event->buttons(), (int)keyState, event->button());
          switch (drag) {
              case DRAG_LASSO:
                drag = DRAG_OFF;
                setCursor();
                setMouseGrab(false);
          
                redraw();
                return;
              case DRAG_MOVE:
                drag = DRAG_OFF;
                setCursor();
                setMouseGrab(false);
                
                endMoveItems (start, MOVE_MOVE, 0);
                return;
              default:
                break;
          }
      }

      // ignore event if (another) button is already active:
      if (event->buttons() ^ button) {
            //printf("viewMousePressEvent ignoring buttons:%x mods:%x button:%x\n", (int)event->buttons(), (int)keyState, event->button());
            // Do nothing, even if the mouse is grabbed or we have a moving list.
            return;
            }

      // Cancel all previous mouse ops. Right now there should be no moving list and drag should be off etc.
      // If there is, it's an error. It likely means we missed a mouseRelease event.
      cancelMouseOps();
      
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
//                         itemSelectionsChanged();
                        itemSelectionsChanged(NULL, true);
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
                        setMouseGrab(true); // CAUTION
                        break;

                  case RubberTool:
                        deleteItem(start);
                        drag = DRAG_DELETE;
                        setCursor();
                        break;

                  case PencilTool:
                  {
                        bool deselect_all = false;
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
                                  setMouseGrab(true); // CAUTION
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
                                deselect_all = true;
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
                              deselect_all = true;
                              // selectItem() will be called in viewMouseReleaseEvent().
                              }
                        itemSelectionsChanged(NULL, deselect_all);
                        redraw();
                  }
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
                            //  it is disastrous. TESTED: Yes, that is how other controls work. Hitting another 
                            //  button while the mouse has been dragged outside causes it to bypass us !
                            setMouseGrab(true); // CAUTION
                            
                            QRect r = QApplication::primaryScreen()->geometry();
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
                            
// screenGeometry() is obsolete. Qt >= 5.6 ? use primaryScreen().
                            QRect r = QApplication::primaryScreen()->geometry();
                            ignore_mouse_move = true;      // Avoid recursion.
                            QCursor::setPos( QPoint(r.width()/2, r.height()/2) );
                            //ignore_mouse_move = false;
                          }
                          // Update the small zoom drawing area
                          QPoint pt = mapFromGlobal(global_start);
                          QSize cursize = zoomIconSVG->actualSize(QSize(MusEGlobal::config.cursorSize, MusEGlobal::config.cursorSize));
                          update(pt.x(), pt.y(), cursize.width(), cursize.height());
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
        
        // If lassoing, update the old lasso region.
        // Do it BEFORE scrolling.
        switch(drag) 
        {
          case DRAG_LASSO:
                // Update the old lasso region.
                redraw(lassoRegion);
                break;
                
          default:
                break;
        }
        
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
                // Set the new lasso rectangle and compute the new lasso region.
                setLasso(QRect(start.x(), start.y(), dist.x(), dist.y()));
                // Update the new lasso region.
                redraw(lassoRegion);
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
// For testing...
      //fprintf(stderr, "ypos=%d yorg=%d ymag=%d event->y=%d ->gy:%d mapy(yorg)=%d rmapy0=%d yOffset=%d rmapy(yOffset()=%d\n",
      //                 ypos,   yorg,   ymag,   event->y(), event->globalY(), mapy(yorg), rmapy(0), yOffset(), rmapy(yOffset()));

      // Drag not off and left mouse button not pressed? It's an error.
      // Meaning likely mouseRelease was not called (which CAN happen).
      if(drag != DRAG_OFF && !(event->buttons() & Qt::LeftButton))
      {
// For testing...
//         fprintf(stderr, "Canvas::viewMouseMoveEvent: calling cancelMouseOps()\n");
        
        // Be sure to cancel any relative stuff. Otherwise it's left in a bad state.
        cancelMouseOps();
      }
      
      QRect  screen_rect    = QApplication::primaryScreen()->geometry();
      QPoint screen_center  = QPoint(screen_rect.width()/2, screen_rect.height()/2);
      QPoint glob_dist      = event->globalPos() - ev_global_pos;
      QPoint glob_zoom_dist = MusEGlobal::config.borderlessMouse ? (event->globalPos() - screen_center) : glob_dist;
      QPoint last_dist      = event->pos() - ev_pos;
      
      ev_pos     = event->pos();
      QPoint dist  = ev_pos - start;
      int ax       = ABS(rmapx(dist.x()));
      int ay       = ABS(rmapy(dist.y()));
      bool isMoving  = (ax >= 2) || (ay > 2);
      Qt::KeyboardModifiers modifiers = event->modifiers();
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
                  // NOTE: Error suppressor for new gcc 7 'fallthrough' level 3 and 4:
                  // FALLTHROUGH
            case DRAG_LASSO:
                  {
                  // Update the old lasso region.
                  redraw(lassoRegion);
                  
                  // Set the new lasso rectangle and compute the new lasso region.
                  setLasso(QRect(start.x(), start.y(), dist.x(), dist.y()));
                  
                  // printf("xorg=%d xmag=%d event->x=%d, mapx(xorg)=%d rmapx0=%d xOffset=%d rmapx(xOffset()=%d\n",
                  //         xorg, xmag, event->x(),mapx(xorg), rmapx(0), xOffset(),rmapx(xOffset()));
                  
                  // Update the new lasso region.
                  redraw(lassoRegion);
                  }
                  break;

            case DRAG_MOVE_START:
            case DRAG_COPY_START:
            case DRAG_CLONE_START:
            {
                  if (!isMoving)
                        break;
                  int dir = 0;
                  if (keyState & Qt::ShiftModifier) {
                        if (ax > ay) {
                              if (drag == DRAG_MOVE_START)
                                    drag = DRAGX_MOVE;
                              else if (drag == DRAG_COPY_START)
                                    drag = DRAGX_COPY;
                              else
                                    drag = DRAGX_CLONE;
                              dir = 1;
                              }
                        else {
                              if (drag == DRAG_MOVE_START)
                                    drag = DRAGY_MOVE;
                              else if (drag == DRAG_COPY_START)
                                    drag = DRAGY_COPY;
                              else
                                    drag = DRAGY_CLONE;
                              dir = 2;
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
                        itemSelectionsChanged(NULL, drag == DRAG_MOVE);
                        redraw();
                        }
                  DragType dt;
                  if (drag == DRAG_MOVE)
                        dt = MOVE_MOVE;
                  else if (drag == DRAG_COPY)
                        dt = MOVE_COPY;
                  else
                        dt = MOVE_CLONE;
                  
                  startMoving(ev_pos, dir, dt, !(keyState & Qt::ShiftModifier));
                  break;
            }

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
                    moveItems(ev_pos, 1, !shift);
                  break;

            case DRAGY_MOVE:
            case DRAGY_COPY:
            case DRAGY_CLONE:
                  if(!scrollTimer)
                    moveItems(ev_pos, 2, !shift);
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
                  else if(_tool == AutomationTool){
                    // The PartCanvas mouseMove will take care of its own cursor.
                    // Break otherwise there is bad flickering as the 'pointing hand' competes with 'cross' etc.
                    break;
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
      // We want only the left mouse release events. Ignore anything else.
      // Do nothing, even if the mouse is grabbed or we have a moving list.
      if(event->button() != Qt::LeftButton) 
      {
        return;
      }

      // Immediately cancel any mouse grabbing.
      // Because for example there are a few message boxes that may appear
      //  in the subsequent code, and the mouse will not work in them if it
      //  is still grabbed.
      setMouseGrab(false);

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

                  itemSelectionsChanged(NULL, !ctrl);
                  redrawFlag = true;
                  if(curItem)
                    itemReleased(curItem, curItem->pos());
                  break;
            case DRAG_COPY:
                  endMoveItems(pos, MOVE_COPY, 0, !shift);
                  break;
            case DRAGX_COPY:
                  endMoveItems(pos, MOVE_COPY, 1, !shift);
                  break;
            case DRAGY_COPY:
                  endMoveItems(pos, MOVE_COPY, 2, !shift);
                  break;
            case DRAG_MOVE:
                  endMoveItems(pos, MOVE_MOVE, 0, !shift);
                  break;
            case DRAGX_MOVE:
                  endMoveItems(pos, MOVE_MOVE, 1, !shift);
                  break;
            case DRAGY_MOVE:
                  endMoveItems(pos, MOVE_MOVE, 2, !shift);
                  break;
            case DRAG_CLONE:
                  endMoveItems(pos, MOVE_CLONE, 0, !shift);
                  break;
            case DRAGX_CLONE:
                  endMoveItems(pos, MOVE_CLONE, 1, !shift);
                  break;
            case DRAGY_CLONE:
                  endMoveItems(pos, MOVE_CLONE, 2, !shift);
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
                    itemSelectionsChanged();
                    redraw();
                    resizeDirection = RESIZE_TO_THE_RIGHT; // reset to default state or ctrl+rightclick resize will cease to work
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
                  // Set the new lasso rectangle and compute the new lasso region.
                  setLasso(QRect(start.x(), start.y(), rmapxDev(1), rmapyDev(1)));
                  //fallthrough
            case DRAG_LASSO:
                  if (!ctrl)
                        deselectAll();
                  // Set the new lasso rectangle and compute the new lasso region.
                  selectLasso(ctrl);
                  itemSelectionsChanged(NULL, !ctrl);
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
                  } else
                      QWidget::setCursor(*handCursor);
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
        QSize cursize = zoomIconSVG->actualSize(QSize(MusEGlobal::config.cursorSize, MusEGlobal::config.cursorSize));
        update(pt.x(), pt.y(), cursize.width(), cursize.height());
      }
      
      // Cancel all previous mouse ops. Right now there should be no moving list and drag should be off etc.
      cancelMouseOps();

      if (redrawFlag)
            redraw();

      mouseRelease(pos);
}

//---------------------------------------------------------
//   selectLasso
//---------------------------------------------------------

bool Canvas::selectLasso(bool toggle)
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

      return n != 0;
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
//   findCurrentItem
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

void Canvas::setLasso(const QRect& r)
{
  lasso = normalizeQRect(r);
  lassoToRegion(lasso, lassoRegion);
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
            showCursor(false); // CAUTION
        else
            QWidget::setCursor(*closedHandCursor);
        break;

    case DRAG_ZOOM:
        if(MusEGlobal::config.borderlessMouse)
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
            QWidget::setCursor(*pencilCursor);
            break;
        case RubberTool:
            QWidget::setCursor(*deleteCursor);
            break;
        case GlueTool:
            QWidget::setCursor(*glueCursor);
            break;
        case CutTool:
            QWidget::setCursor(*cutterCursor);
            break;
        case MuteTool:
            QWidget::setCursor(*mutePartsCursor);
            break;
        case AutomationTool:
            QWidget::setCursor(*drawCursor);
            break;
        case DrawTool:
            // set for prcanvas/dcanvas as they inherit this w/o redefinition
            QWidget::setCursor(QCursor(Qt::ForbiddenCursor));
            break;
        case PanTool:
            QWidget::setCursor(*handCursor);
            break;
        case ZoomTool:
            QWidget::setCursor(*zoomCursor);
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
//   tagItems
//---------------------------------------------------------

void Canvas::tagItems(MusECore::TagEventList* tag_list, const MusECore::EventTagOptionsStruct& options) const
{ 
  const bool tagSelected = options._flags & MusECore::TagSelected;
  const bool tagMoving   = options._flags & MusECore::TagMoving;
  const bool tagAllItems = options._flags & MusECore::TagAllItems;
  const bool tagAllParts = options._flags & MusECore::TagAllParts;
  const bool range       = options._flags & MusECore::TagRange;
  const MusECore::Pos& p0 = options._p0;
  const MusECore::Pos& p1 = options._p1;
  
  CItem* item;
  if(range)
  {
    for(ciCItem i = items.begin(); i != items.end(); ++i)
    {
      item = i->second;
      if(!tagAllParts && item->part() != curPart)
        continue;
      if((tagAllItems
          || (tagSelected && item->isSelected())
          || (tagMoving && item->isMoving()))
         && item->isObjectInRange(p0, p1))
      {
        tag_list->add(item->part(), item->event());
      }
    }
  }
  else
  {
    for(ciCItem i = items.begin(); i != items.end(); ++i)
    {
      item = i->second;
      if(!tagAllParts && item->part() != curPart)
        continue;
      if(tagAllItems
        || (tagSelected && item->isSelected())
        || (tagMoving && item->isMoving()))
      {
        tag_list->add(item->part(), item->event());
      }
    }
  }
}

//---------------------------------------------------------
//   updateItemSelections
//---------------------------------------------------------

void Canvas::updateItemSelections()
      {
      bool item_selected;
      bool obj_selected;
      for (iCItem i = items.begin(); i != items.end(); ++i) {
            CItem* item = i->second;
            item_selected = item->isSelected();
            obj_selected = item->objectIsSelected();
            if (item_selected != obj_selected)
            {
              item->setSelected(obj_selected);
            }
      }
      redraw();
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

//---------------------------------------------------------
//   cancelMouseOps
//---------------------------------------------------------

bool Canvas::cancelMouseOps()
{
  bool changed = false;
  
  // Make sure this is done. See mousePressEvent.
  showCursor();
  setMouseGrab(false);

  // Be sure to clear the moving list and especially the item moving flags!
  if(!moving.empty())
  {
    for(iCItem i = moving.begin(); i != moving.end(); ++i)
      i->second->setMoving(false);
    moving.clear();
    changed = true;
  }
  
  if(drag != DRAG_OFF)
  {
    drag = DRAG_OFF;
    changed = true;
  }

  redraw();
  
  return changed;
}

} // namespace MusEGui

