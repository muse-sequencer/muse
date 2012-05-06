//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: canvas.cpp,v 1.10.2.17 2009/05/03 04:14:01 terminator356 Exp $
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

#include <stdio.h>

#include "canvas.h"

#include <QApplication>
#include <QMenu>
#include <QPainter>
#include <QCursor>
#include <QTimer>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QWheelEvent>

#include <vector>

#include "song.h"
#include "event.h"
#include "citem.h"
#include "icons.h"
#include "../marker/marker.h"
#include "part.h"
#include "fastlog.h"

#define ABS(x)  ((x) < 0) ? -(x) : (x)

namespace MusEGui {

//---------------------------------------------------------
//   Canvas
//---------------------------------------------------------

Canvas::Canvas(QWidget* parent, int sx, int sy, const char* name)
   : View(parent, sx, sy, name)
      {
      canvasTools = 0;
      itemPopupMenu = 0;
      
      button = Qt::NoButton;
      keyState = 0;

      canScrollLeft = true;
      canScrollRight = true;
      canScrollUp = true;
      canScrollDown = true;
      hscrollDir = HSCROLL_NONE;
      vscrollDir = VSCROLL_NONE;
      scrollTimer=NULL;
      
      scrollSpeed=10;    // hardcoded scroll jump

      drag    = DRAG_OFF;
      _tool   = PointerTool;
      pos[0]  = MusEGlobal::song->cpos();
      pos[1]  = MusEGlobal::song->lpos();
      pos[2]  = MusEGlobal::song->rpos();
      curPart = NULL;
      curPartId = -1;
      curItem = NULL;
      connect(MusEGlobal::song, SIGNAL(posChanged(int, unsigned, bool)), this, SLOT(setPos(int, unsigned, bool)));
      }

Canvas::~Canvas()
{
  items.clearDelete();
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
      //std::vector<CItem*> list3;
      std::vector<CItem*> list4;

      if (virt()) {
            drawCanvas(p, rect);

            //---------------------------------------------------
            // draw Canvas Items
            //---------------------------------------------------

            iCItem to(items.lower_bound(x2));
            
            /*
            // Draw items from other parts behind all others.
            // Only for items with events (not arranger parts).
            for(iCItem i = items.begin(); i != to; ++i)
            { 
              CItem* ci = i->second;
              // NOTE Optimization: For each item call this once now, then use cached results later via cachedHasHiddenEvents().
              ci->part()->hasHiddenEvents();
              if(!ci->event().empty() && ci->part() != curPart)
                drawItem(p, ci, rect);
            }
                
            // Draw unselected parts behind selected.
            for (iCItem i = items.begin(); i != to; ++i) 
            {
                  CItem* ci = i->second;
                  if((!ci->isSelected() && !ci->isMoving() && (ci->event().empty() || ci->part() == curPart))
                     && !(ci->event().empty() && (ci->part()->events()->arefCount() > 1 || ci->part()->cachedHasHiddenEvents())))  // p4.0.29 
                  {
                        drawItem(p, ci, rect);
                  }      
            }
            
            // Draw selected parts in front of unselected.
            for (iCItem i = items.begin(); i != to; ++i) 
            {
                CItem* ci = i->second;
                if(ci->isSelected() && !ci->isMoving() && (ci->event().empty() || ci->part() == curPart))
                //if((ci->isSelected() && !ci->isMoving() && (ci->event().empty() || ci->part() == curPart)) 
                //   || (ci->event().empty() && (ci->part()->events()->arefCount() > 1 || ci->part()->cachedHasHiddenEvents())))
                {
                      drawItem(p, ci, rect);
                }      
            }  
            */
            
            // p4.0.29
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
                // Draw clone parts, and parts with hidden events, in front of others all except selected.
                //else if(ci->event().empty() && (ci->part()->events()->arefCount() > 1 || ci->part()->cachedHasHiddenEvents()))
                // Draw clone parts in front of others all except selected.
                //else if(ci->event().empty() && (ci->part()->events()->arefCount() > 1))
                //  list3.push_back(ci);
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
            //sz = list3.size();
            //for(i = 0; i != sz; ++i) 
            //  drawItem(p, list3[i], rect);
            sz = list4.size();
            for(i = 0; i != sz; ++i) 
              drawItem(p, list4[i], rect);
            
            to = moving.lower_bound(x2);
            for (iCItem i = moving.begin(); i != to; ++i) 
            {
                  drawItem(p, i->second, rect);
            }

            drawTopItem(p,rect);

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

            drawCanvas(p, QRect(x, y, w, h));
            p.restore();

            //---------------------------------------------------
            // draw Canvas Items
            //---------------------------------------------------
            
            /*
            // Draw items from other parts behind all others.
            // Only for items with events (not arranger parts).
            for(iCItem i = items.begin(); i != items.end(); ++i)
            { 
              CItem* ci = i->second;
              // NOTE Optimization: For each item call this once now, then use cached results later via cachedHasHiddenEvents().
              ci->part()->hasHiddenEvents();
              if(!ci->event().empty() && ci->part() != curPart)
                drawItem(p, ci, rect);
            }
                
            // Draw unselected parts behind selected.
            for (iCItem i = items.begin(); i != items.end(); ++i) {
                  CItem* ci = i->second;
                  if(!ci->isSelected() && !ci->isMoving() && (ci->event().empty() || ci->part() == curPart))
                    {
                        drawItem(p, ci, rect);
                    }      
                  }
            
            // Draw selected parts in front of unselected.
            for (iCItem i = items.begin(); i != items.end(); ++i) {
                  CItem* ci = i->second;
                  if(ci->isSelected() && !ci->isMoving() && (ci->event().empty() || ci->part() == curPart))
                  //if((ci->isSelected() && !ci->isMoving() && (ci->event().empty() || ci->part() == curPart)) 
                  //   || (ci->event().empty() && (ci->part()->events()->arefCount() > 1 || ci->part()->cachedHasHiddenEvents())))
                  {    
                      drawItem(p, ci, rect);
                  }    
                } 
            */
            
            // p4.0.29
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
                // Draw clone parts, and parts with hidden events, in front of others all except selected.
                //else if(ci->event().empty() && (ci->part()->events()->arefCount() > 1 || ci->part()->cachedHasHiddenEvents()))
                // Draw clone parts in front of others all except selected.
                //else if(ci->event().empty() && (ci->part()->events()->arefCount() > 1))
                //  list3.push_back(ci);
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
            //sz = list3.size();
            //for(i = 0; i != sz; ++i) 
            //  drawItem(p, list3[i], rect);
            sz = list4.size();
            for(i = 0; i != sz; ++i) 
              drawItem(p, list4[i], rect);
            
            for (iCItem i = moving.begin(); i != moving.end(); ++i) 
                  {
                        drawItem(p, i->second, rect);
                  }
            drawTopItem(p, QRect(x,y,w,h));
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
      //int y2 = y + h;
      int my2 = mapy(y + h);
      MusECore::MarkerList* marker = MusEGlobal::song->marker();
      for (MusECore::iMarker m = marker->begin(); m != marker->end(); ++m) {
            int xp = m->second.tick();
            if (xp >= x && xp < x+w) {
                  p.setPen(Qt::green);
                  //p.drawLine(xp, y, xp, y2);
                  p.drawLine(mapx(xp), my, mapx(xp), my2);
                  }
            }

      //---------------------------------------------------
      //    draw location marker
      //---------------------------------------------------

      p.setPen(Qt::blue);
      int mx;
      if (pos[1] >= unsigned(x) && pos[1] < unsigned(x2)) {
            //p.drawLine(pos[1], y, pos[1], y2);
            mx = mapx(pos[1]);
            p.drawLine(mx, my, mx, my2);
            }
      if (pos[2] >= unsigned(x) && pos[2] < unsigned(x2)) {
            //p.drawLine(pos[2], y, pos[2], y2);
            mx = mapx(pos[2]);
            p.drawLine(mx, my, mx, my2);
            }
      p.setPen(Qt::red);
      if (pos[0] >= unsigned(x) && pos[0] < unsigned(x2)) {
            //p.drawLine(pos[0], y, pos[0], y2);
            mx = mapx(pos[0]);
            p.drawLine(mx, my, mx, my2);
            }
      
      //p.restore();
      //p.setWorldMatrixEnabled(true);
      p.setWorldMatrixEnabled(wmtxen);
      
      //---------------------------------------------------
      //    draw lasso
      //---------------------------------------------------

      if (drag == DRAG_LASSO) {
            p.setPen(Qt::blue);
            p.setBrush(Qt::NoBrush);
            p.drawRect(lasso);
            }
      
      //---------------------------------------------------
      //    draw moving items
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
        ///if (ev->state() == Qt::ShiftModifier)
  //      if (((QInputEvent*)ev)->modifiers() == Qt::ShiftModifier)
        scrollstep = scrollstep / 10;

        int newXpos = xpos + xpixelscale * scrollstep;

        if (newXpos < 0)
              newXpos = 0;

        //setYPos(newYpos);
        emit horizontalScroll((unsigned)newXpos);

    } else if (ctrl) {  // zoom horizontally
      if (ev->delta()>0)
        emit horizontalZoomIn();
      else
        emit horizontalZoomOut();

    } else { // scroll vertically
        int delta       = ev->delta() / WHEEL_DELTA;
        int ypixelscale = rmapyDev(1);

        if (ypixelscale <= 0)
              ypixelscale = 1;

        int scrollstep = WHEEL_STEPSIZE * (-delta);
        ///if (ev->state() == Qt::ShiftModifier)
  //      if (((QInputEvent*)ev)->modifiers() == Qt::ShiftModifier)
        scrollstep = scrollstep / 2;

        int newYpos = ypos + ypixelscale * scrollstep;

        if (newYpos < 0)
              newYpos = 0;

        //setYPos(newYpos);
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

void Canvas::startMoving(const QPoint& pos, DragType)
      {
      for (iCItem i = items.begin(); i != items.end(); ++i) {
            if (i->second->isSelected()) {
                  i->second->setMoving(true);
                  moving.add(i->second);
                  }
            }
      moveItems(pos, 0);
      }

//---------------------------------------------------------
//   moveItems
//    dir = 0     move in all directions
//          1     move only horizontal
//          2     move only vertical
//---------------------------------------------------------

void Canvas::moveItems(const QPoint& pos, int dir = 0, bool rasterize)
      {
      int dp;
      if(rasterize)
        dp = y2pitch(pos.y()) - y2pitch(start.y());
      else  
        dp = pos.y() - start.y();
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
            if(rasterize)
            {
              ny = pitch2y(y2pitch(y) + dp);
              mp = raster(QPoint(nx, ny));
            }  
            else  
            {  
              ny = y + dp;
              mp = QPoint(nx, ny);
            }  
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
//   viewMousePressEvent
//---------------------------------------------------------

void Canvas::viewMousePressEvent(QMouseEvent* event)
      {
      if (!mousePress(event))
          return;

      ///keyState = event->state();
      keyState = ((QInputEvent*)event)->modifiers();
      button = event->button();

      //printf("viewMousePressEvent buttons:%x mods:%x button:%x\n", (int)event->buttons(), (int)keyState, event->button());
      
      // special events if right button is clicked while operations
      // like moving or drawing lasso is performed.
      ///if (event->stateAfter() & Qt::RightButton) {
      if (event->buttons() & Qt::RightButton & ~(event->button())) {
          //printf("viewMousePressEvent special buttons:%x mods:%x button:%x\n", (int)event->buttons(), (int)keyState, event->button());
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
      ///if (keyState & (Qt::LeftButton|Qt::RightButton|Qt::MidButton)) {
      if (event->buttons() & (Qt::LeftButton|Qt::RightButton|Qt::MidButton) & ~(event->button())) {
            //printf("viewMousePressEvent ignoring buttons:%x mods:%x button:%x\n", (int)event->buttons(), (int)keyState, event->button());
            return;
            }
      bool alt        = keyState & Qt::AltModifier;
      bool ctrl       = keyState & Qt::ControlModifier;
      start           = event->pos();

      //---------------------------------------------------
      //    set curItem to item mouse is pointing
      //    (if any)
      //---------------------------------------------------

      if (virt())
            curItem = items.find(start);
      else {
            curItem = 0;
            iCItem ius;
            bool usfound = false;
            for (iCItem i = items.begin(); i != items.end(); ++i) {
                  QRect box = i->second->bbox();
                  int x = rmapxDev(box.x());
                  int y = rmapyDev(box.y());
                  int w = rmapxDev(box.width());
                  int h = rmapyDev(box.height());
                  QRect r(x, y, w, h);
                  ///r.moveBy(i->second->pos().x(), i->second->pos().y());
                  r.translate(i->second->pos().x(), i->second->pos().y());
                  if (r.contains(start)) {
                        if(i->second->isSelected())
                        {
                          curItem = i->second;
                          break;
                        }
                        else
                        if(!usfound)
                        {
                          ius = i;
                          usfound = true;
                        }
                     }
                  }
                  if(!curItem && usfound)
                    curItem = ius->second;
            }

      if (curItem && (event->button() == Qt::MidButton)) {
            deleteItem(start); // changed from "start drag" to "delete" by flo93
            drag = DRAG_DELETE;
            setCursor();
            }
      else if (event->button() == Qt::RightButton) {
            if (curItem) {
                  if (ctrl) {
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
      else if (event->button() == Qt::LeftButton) {
            switch (_tool) {
                  case PointerTool:
                        if (curItem) {
                              if (curItem->part() != curPart) {
                                    curPart = curItem->part();
                                    curPartId = curPart->sn();
                                    curPartChanged();
                                    }
                              itemPressed(curItem);
                              // Changed by T356. Alt is default reserved for moving the whole window in KDE. Changed to Shift-Alt.
                              // Hmm, nope, shift-alt is also reserved sometimes. Must find a way to bypass, 
                              //  why make user turn off setting? Left alone for now...
                              if (ctrl)
                                    drag = DRAG_COPY_START;
                              else if (alt) {
                                    drag = DRAG_CLONE_START;
                                    }
                              else
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
                              drag = DRAG_RESIZE;
                              setCursor();
                              int dx = start.x() - curItem->x();
                              curItem->setWidth(dx);
                              start.setX(curItem->x());
                              }
                        else {
                              drag = DRAG_NEW;
                              setCursor();
                              curItem = newItem(start, event->modifiers());
                              if (curItem)
                                    items.add(curItem);
                              else {
                                    drag = DRAG_OFF;
                                    setCursor();
                                    }
                              }
                        deselectAll();
                        if (curItem)
                              selectItem(curItem, true);
                        updateSelection();
                        redraw();
                        break;

                  default:
                        break;
                  }
            }
      }

void Canvas::scrollTimerDone()
{
      //printf("Canvas::scrollTimerDone drag:%d doScroll:%d\n", drag, doScroll);
      
      if (drag != DRAG_OFF && doScroll)
      {
        //printf("Canvas::scrollTimerDone drag != DRAG_OFF && doScroll\n");
        
        bool doHMove = false;
        bool doVMove = false;
        int hoff = rmapx(xOffset())+mapx(xorg)-1;
        int curxpos;
        switch(hscrollDir)
        {  
          case HSCROLL_RIGHT:
            hoff += scrollSpeed;
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
                emit horizontalScrollNoLimit(hoff);
                canScrollLeft = true;
                ev_pos.setX(rmapxDev(rmapx(ev_pos.x()) + scrollSpeed));
                doHMove = true;
              break;
              default:  
                if(canScrollRight)
                {
                  curxpos = xpos;
                  emit horizontalScroll(hoff);
                  if(xpos <= curxpos)
                  {  
                    canScrollRight = false;
                  }
                  else
                  {
                    canScrollLeft = true;
                    ev_pos.setX(rmapxDev(rmapx(ev_pos.x()) + scrollSpeed));
                    doHMove = true;
                  }  
                }  
                else
                {  
                }
              break;
            }
          break;  
          case HSCROLL_LEFT:
            if(canScrollLeft)
            {
              curxpos = xpos;
              hoff -= scrollSpeed;
              emit horizontalScroll(hoff);
              if(xpos >= curxpos)
              {  
                canScrollLeft = false;
              }
              else
              {
                canScrollRight = true;
                ev_pos.setX(rmapxDev(rmapx(ev_pos.x()) - scrollSpeed));
                doHMove = true;
              }
            }    
            else
            {  
            }
          break; 
          default:
          break;   
        }
        int voff = rmapy(yOffset())+mapy(yorg);
        int curypos;
        switch(vscrollDir)
        {
          case VSCROLL_DOWN:
            if(canScrollDown)
            {
              curypos = ypos;
              voff += scrollSpeed;
              emit verticalScroll(voff);
              if(ypos <= curypos)
              {  
                canScrollDown = false;
              }
              else
              {
                canScrollUp = true;
                ev_pos.setY(rmapyDev(rmapy(ev_pos.y()) + scrollSpeed));
                doVMove = true;
              }
            }    
            else
            {  
            }
          break;  
          case VSCROLL_UP:
            if(canScrollUp)
            {
              curypos = ypos;
              voff -= scrollSpeed;
              emit verticalScroll(voff);
              if(ypos >= curypos)
              {  
                canScrollUp = false;
              }
              else
              {
                canScrollDown = true;
                ev_pos.setY(rmapyDev(rmapy(ev_pos.y()) - scrollSpeed));
                doVMove = true;
              } 
            }   
            else
            {  
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
          case DRAG_RESIZE:
                if (dist.x()) {
                      if (dist.x() < 1)
                            curItem->setWidth(1);
                      else
                            curItem->setWidth(dist.x());
                      redraw();
                      }
                break;
          default:  
                break;
        }
        //printf("Canvas::scrollTimerDone starting scrollTimer: Currently active?%d\n", scrollTimer->isActive());
        
        // p3.3.43 Make sure to yield to other events (for up to 3 seconds), otherwise other events 
        //  take a long time to reach us, causing scrolling to take a painfully long time to stop.
        // FIXME: Didn't help at all.
        //qApp->processEvents();
        // No, try up to 100 ms for each yield.
        //qApp->processEvents(100);
        //
        //scrollTimer->start( 40, TRUE ); // X ms single-shot timer
        // OK, changing the timeout from 40 to 80 helped.
        //scrollTimer->start( 80, TRUE ); // X ms single-shot timer
        scrollTimer->setSingleShot(true);
        scrollTimer->start(80);
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
      
      ev_pos  = event->pos();
      QPoint dist = ev_pos - start;
      int ax      = ABS(rmapx(dist.x()));
      int ay      = ABS(rmapy(dist.y()));
      bool moving = (ax >= 2) || (ay > 2);

      // set scrolling variables: doScroll, scrollRight
      if (drag != DRAG_OFF) {
            
                
            int ex = rmapx(event->x())+mapx(0);
            if(ex < 40 && canScrollLeft)
              hscrollDir = HSCROLL_LEFT;
            else  
            if(ex > (width() - 40))
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
                    hscrollDir = HSCROLL_RIGHT;
                break;
                default:
                  if(canScrollRight)
                    hscrollDir = HSCROLL_RIGHT;
                  else  
                    hscrollDir = HSCROLL_NONE;
                break;
              }
            else  
              hscrollDir = HSCROLL_NONE;
            int ey = rmapy(event->y())+mapy(0);
            if(ey < 15 && canScrollUp)
              vscrollDir = VSCROLL_UP;
            else  
            if(ey > (height() - 15) && canScrollDown)
              vscrollDir = VSCROLL_DOWN;
            else  
              vscrollDir = VSCROLL_NONE;
            if(hscrollDir != HSCROLL_NONE || vscrollDir != VSCROLL_NONE)
            {
              doScroll=true;
              if (!scrollTimer) 
              {
                  scrollTimer= new QTimer(this);
                  connect( scrollTimer, SIGNAL(timeout()), SLOT(scrollTimerDone()) );
                  //scrollTimer->start( 0, TRUE ); // single-shot timer
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
                  if (!moving)
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
                  if (!moving)
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
                  if (!curItem->isSelected()) {
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
                  
                  startMoving(ev_pos, dt);
                  break;

            case DRAG_MOVE:
            case DRAG_COPY:
            case DRAG_CLONE:
      
                  if(!scrollTimer)
                    moveItems(ev_pos, 0);
                  break;

            case DRAGX_MOVE:
            case DRAGX_COPY:
            case DRAGX_CLONE:
                  if(!scrollTimer)
                    moveItems(ev_pos, 1);
                  break;

            case DRAGY_MOVE:
            case DRAGY_COPY:
            case DRAGY_CLONE:
                  if(!scrollTimer)
                    moveItems(ev_pos, 2);
                  break;

            case DRAG_NEW:
            case DRAG_RESIZE:
                  if (dist.x()) {
                        if (dist.x() < 1)
                              curItem->setWidth(1);
                        else
                              curItem->setWidth(dist.x());
                        redraw();
                        }
                  break;
            case DRAG_DELETE:
                  deleteItem(ev_pos);
                  break;

            case DRAG_OFF:
                  break;
            }
                  
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
      if (event->buttons() & (Qt::LeftButton|Qt::RightButton|Qt::MidButton) & ~(event->button())) {
            return;
            }

      QPoint pos = event->pos();
      bool ctrl = ((QInputEvent*)event)->modifiers() & Qt::ControlModifier;
      bool shift = ((QInputEvent*)event)->modifiers() & Qt::ShiftModifier;
      bool redrawFlag = false;

      switch (drag) {
            case DRAG_MOVE_START:
            case DRAG_COPY_START:
            case DRAG_CLONE_START:
                  if (!ctrl)
                        deselectAll();
                        
                  if (!shift) { //Select or deselect only the clicked item
                      selectItem(curItem, !(ctrl && curItem->isSelected()));
                      }
                  else { //Select or deselect all on the same pitch (e.g. same y-value)
                      bool selectionFlag = !(ctrl && curItem->isSelected());
                      for (iCItem i = items.begin(); i != items.end(); ++i)
                            if (i->second->y() == curItem->y() )
                                  selectItem(i->second, selectionFlag);
                      }

                  updateSelection();
                  redrawFlag = true;
                  itemReleased(curItem, curItem->pos());
                  break;
            case DRAG_COPY:
                  endMoveItems(pos, MOVE_COPY, 0);
                  break;
            case DRAGX_COPY:
                  endMoveItems(pos, MOVE_COPY, 1);
                  break;
            case DRAGY_COPY:
                  endMoveItems(pos, MOVE_COPY, 2);
                  break;
            case DRAG_MOVE:
                  endMoveItems(pos, MOVE_MOVE, 0);
                  break;
            case DRAGX_MOVE:
                  endMoveItems(pos, MOVE_MOVE, 1);
                  break;
            case DRAGY_MOVE:
                  endMoveItems(pos, MOVE_MOVE, 2);
                  break;
            case DRAG_CLONE:
                  endMoveItems(pos, MOVE_CLONE, 0);
                  break;
            case DRAGX_CLONE:
                  endMoveItems(pos, MOVE_CLONE, 1);
                  break;
            case DRAGY_CLONE:
                  endMoveItems(pos, MOVE_CLONE, 2);
                  break;
            case DRAG_OFF:
                  break;
            case DRAG_RESIZE:
                  resizeItem(curItem, false, ctrl);
                  break;
            case DRAG_NEW:
                  newItem(curItem, false);
                  redrawFlag = true;
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
            }
      //printf("Canvas::viewMouseReleaseEvent setting drag to DRAG_OFF\n");
      
      drag = DRAG_OFF;
      if (redrawFlag)
            redraw();
      setCursor();
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
                  ///r.moveBy(i->second->pos().x(), i->second->pos().y());
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
                  ///r.moveBy(i->second->pos().x(), i->second->pos().y());
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

void Canvas::setCursor()
      {
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
                  QWidget::setCursor(QCursor(Qt::SizeAllCursor));
                  break;

            case DRAG_RESIZE:
                  QWidget::setCursor(QCursor(Qt::SizeHorCursor));
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
//   isSingleSelection
//---------------------------------------------------------

bool Canvas::isSingleSelection()
      {
      return selectionSize() == 1;
      }

//---------------------------------------------------------
//   selectionSize
//---------------------------------------------------------

int Canvas::selectionSize()
      {
      int n = 0;
      for (iCItem i = items.begin(); i != items.end(); ++i) {
            if (i->second->isSelected())
                  ++n;
            }
      return n;
      }

//---------------------------------------------------------
//   genCanvasPopup
//---------------------------------------------------------

QMenu* Canvas::genCanvasPopup()
      {
      if (canvasTools == 0)
            return 0;
      QMenu* canvasPopup = new QMenu(this);
      QAction* act0 = 0;

      for (unsigned i = 0; i < 9; ++i) {
            if ((canvasTools & (1 << i))==0)
                  continue;
            QAction* act = canvasPopup->addAction(QIcon(**toolList[i].icon), tr(toolList[i].tip));
	    act->setData(1<<i); // ddskrjo
            if (!act0)
                  act0 = act;
            }
      canvasPopup->setActiveAction(act0);
      return canvasPopup;
      }

//---------------------------------------------------------
//   canvasPopup
//---------------------------------------------------------

void Canvas::canvasPopup(int n)
      {
      setTool(n);
      emit toolChanged(n);
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
