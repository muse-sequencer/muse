//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: view.cpp,v 1.3.2.2 2009/04/06 01:24:55 terminator356 Exp $
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

#include "view.h"
#include <cmath>
#include <stdio.h>
#include <QPainter>
#include <QPixmap>
#include <QResizeEvent>
#include <QDropEvent>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QPaintEvent>

#include "math.h"

#include "al/sig.h"  

// Don't use this, it was just for debugging. 
// It's much slower than muse-1 no matter how hard I tried.
// The left/right pixmap shifters in seXPos setYPos 
//  just ate up all the time no matter what I tried.
//#defines VIEW_USE_DOUBLE_BUFFERING 1

namespace MusEGui {

//---------------------------------------------------------
//   View::View
//    double xMag = (xmag < 0) ? 1.0/-xmag : double(xmag)
//---------------------------------------------------------

View::View(QWidget* w, int xm, int ym, const char* name)
   : QWidget(w)
      {
      setAttribute(Qt::WA_NoSystemBackground);
      setAttribute(Qt::WA_StaticContents);
      // This is absolutely required for speed! Otherwise painfully slow because we get 
      //  full rect paint events even on small scrolls! See help on QPainter::scroll().
      setAttribute(Qt::WA_OpaquePaintEvent);
          
      setObjectName(QString(name));
      xmag  = xm;
      ymag  = ym;
      xpos  = 0;
      ypos  = 0;
      xorg  = 0;
      yorg  = 0;
      _virt = true;
      setBackgroundRole(QPalette::NoRole);
      brush.setStyle(Qt::SolidPattern);
      brush.setColor(Qt::lightGray);
      #ifdef VIEW_USE_DOUBLE_BUFFERING
      pmValid = false;
      #endif
      }

//---------------------------------------------------------
//   setOrigin
//---------------------------------------------------------

void View::setOrigin(int x, int y)
      {
      xorg = x;
      yorg = y;
      redraw();
      }

//---------------------------------------------------------
//   setXMag
//---------------------------------------------------------

void View::setXMag(int xs)
      {
      xmag = xs;
      redraw();
      }

//---------------------------------------------------------
//   seqYMag
//---------------------------------------------------------

void View::setYMag(int ys)
      {
      ymag = ys;
      redraw();
      }

//---------------------------------------------------------
//   setXPos
//    x - phys offset
//---------------------------------------------------------

void View::setXPos(int x)
      {
      int delta  = xpos - x;         // -  -> shift left
      xpos  = x;
      
      #ifdef VIEW_USE_DOUBLE_BUFFERING
      if (pm.isNull())
            return;
      if (!pmValid) {
            //printf("View::setXPos !pmValid x:%d width:%d delta:%d\n", x, width(), delta);
            redraw();
            return;
            }
            
      int w = width();
      int h = height();

      QRect r;
      if (delta >= w || delta <= -w)
            r = QRect(0, 0, w, h);
      else if (delta < 0) {   // shift left
            //bitBlt(&pm,  0, 0, &pm,  -delta, 0, w + delta, h, CopyROP, true);
            QPainter p(&pm);
            p.setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform | QPainter::HighQualityAntialiasing, false);
            //printf("View::setXPos x:%d w:%d delta:%d r.x:%d r.w:%d\n", 
            //  x, w, delta, r.x(), r.width());                    
            p.drawPixmap(0, 0, pm, -delta, 0, w + delta, h);
            r = QRect(w + delta, 0, -delta, h);
            }
      else {                  // shift right
            //bitBlt(&pm,  delta, 0, &pm,     0, 0, w-delta, h, CopyROP, true);
            QPainter p(&pm);
            p.setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform | QPainter::HighQualityAntialiasing, false);
            //printf("View::setXPos x:%d w:%d delta:%d r.x:%d r.w:%d\n", 
            //  x, w, delta, r.x(), r.width());                    
            p.drawPixmap(delta, 0, pm,     0, 0, w-delta, h);
            r = QRect(0, 0, delta, h);
            }
      QRect olr = overlayRect();
      QRect olr1(olr);
      olr1.translate(delta, 0);

      r |= olr;
      r |= olr1;
      
      //printf("View::setXPos x:%d w:%d delta:%d r.x:%d r.w:%d\n", x, w, delta, r.x(), r.width());
      //printf("View::setXPos paint delta:%d r.x:%d r.y:%d r.w:%d r.h:%d\n", delta, r.x(), r.y(), r.width(), r.height());  
      
      paint(r);
      update();
      
      #else
      scroll(delta, 0);
      QRect olr = overlayRect();
      // Is there an overlay?
      if(!olr.isNull())
      {
        // Are we shifting right (moving left)?
        if(delta >= 0)
        {
          // Translate not good - need to set x to delta.
          //olr.translate(delta, 0);
          olr.setX(delta);
          olr.setWidth(olr.x() + olr.width() + delta);
        }
        else
        // We are shifting left (moving right).
        {
          // Translate not good - need to limit x to 0.
          //olr.translate(delta, 0);
          olr.setX(olr.x() + delta);
        }
        
        if(olr.x() < 0)
          olr.setX(0);
        if(olr.right() > width())
          olr.setRight(width());
        
        if(olr.y() < 0)
          olr.setY(0);
        if(olr.bottom() > height())
          olr.setBottom(height());
        
        //printf("scroll X update: x:%d y:%d w:%d h:%d\n", olr.x(), olr.y(), olr.width(), olr.height()); 
        update(olr);
      }  
      #endif
      }

//---------------------------------------------------------
//   setYPos
//---------------------------------------------------------

void View::setYPos(int y)
      {
      int delta  = ypos - y;         // -  -> shift up
      ypos  = y;
      
      #ifdef VIEW_USE_DOUBLE_BUFFERING
      if (pm.isNull())
            return;
      if (!pmValid) {
            //printf("View::setYPos !pmValid y:%d height:%d delta:%d\n", y, height(), delta);
            
            redraw();
            return;
            }
      
      int w = width();
      int h = height();
      
      QRect r;
      if (delta >= h || delta <= -h)
            r = QRect(0, 0, w, h);
      else if (delta < 0) {   // shift up
            //bitBlt(&pm,  0, 0, &pm, 0, -delta, w, h + delta, CopyROP, true);
            QPainter p(&pm);
            p.drawPixmap(0, 0, pm, 0, -delta, w, h + delta);
            r = QRect(0, h + delta, w, -delta);
            }
      else {                  // shift down
            //bitBlt(&pm,  0, delta, &pm, 0, 0, w, h-delta, CopyROP, true);
            QPainter p(&pm);
            p.drawPixmap(0, delta, pm, 0, 0, w, h-delta);
            r = QRect(0, 0, w, delta);
            }
      QRect olr = overlayRect();
      QRect olr1(olr);
      olr1.translate(0, delta);

      r |= olr;
      r |= olr1;

      //printf("View::setYPos paint delta:%d r.x:%d r.y:%d r.w:%d r.h:%d\n", delta, r.x(), r.y(), r.width(), r.height());  
      
      paint(r);
      update();
      
      #else
      scroll(0, delta);
      QRect olr = overlayRect();
      // Is there an overlay?
      if(!olr.isNull())
      {
        // Are we shifting down (moving up)?
        if(delta >= 0)
        {
          // Translate not good - need to set y to delta.
          //olr.translate(0, delta);
          olr.setY(delta);
          olr.setHeight(olr.y() + olr.height() + delta);
        }
        else
        // We are shifting up (moving down).
        {
          // Translate not good - need to limit y to 0.
          //olr.translate(0, delta);
          olr.setY(olr.y() + delta);
        }
        
        if(olr.x() < 0)
          olr.setX(0);
        if(olr.right() > width())
          olr.setRight(width());
        
        if(olr.y() < 0)
          olr.setY(0);
        if(olr.bottom() > height())
          olr.setBottom(height());
        
        //printf("scroll Y update: x:%d y:%d w:%d h:%d\n", olr.x(), olr.y(), olr.width(), olr.height()); 
        update(olr);
      }  
      #endif
      }

//---------------------------------------------------------
//   resizeEvent
//---------------------------------------------------------

void View::resizeEvent(QResizeEvent* ev)
      {
      QWidget::resizeEvent(ev);  
      #ifdef VIEW_USE_DOUBLE_BUFFERING
      //pm.resize(ev->size());
      //printf("View::resizeEvent width:%d height:%d\n", 
      //  ev->size().width(), ev->size().height());  
      
      if(pm.isNull())
      {
        //printf("View::resizeEvent pixmap is null\n"); 
        pm = QPixmap(ev->size().width(), ev->size().height());
      }  
      else  
        pm = pm.copy(QRect(QPoint(0, 0), ev->size()));
      pmValid = false;
      #endif
      }

//---------------------------------------------------------
//   paintEvent
//---------------------------------------------------------

void View::paintEvent(QPaintEvent* ev)
      {
      //printf("View::paintEvent x:%d width:%d y:%d height:%d\n", 
      //  ev->rect().x(), ev->rect().width(), ev->rect().y(), ev->rect().height());  
        
      #ifdef VIEW_USE_DOUBLE_BUFFERING
      if (!pmValid)
            paint(ev->rect());
      
      //bitBlt(this, ev->rect().topLeft(), &pm, ev->rect(), CopyROP, true);
      QPainter p(this);
      //p.setCompositionMode(QPainter::CompositionMode_Source);
      p.drawPixmap(ev->rect().topLeft(), pm, ev->rect());
      
      #else
      paint(ev->rect());
      #endif
      }

//---------------------------------------------------------
//   redraw
//---------------------------------------------------------

void View::redraw()
      {
      //printf("View::redraw()\n");  
      
      #ifdef VIEW_USE_DOUBLE_BUFFERING
      QRect r(0, 0, pm.width(), pm.height());
      //printf("View::redraw() r.x:%d r.w:%d\n", r.x(), r.width()); 
      paint(r);
      #endif
      
      update();
      }

//---------------------------------------------------------
//   redraw
//---------------------------------------------------------

void View::redraw(const QRect& r)
      {
      //printf("View::redraw(QRect& r) r.x:%d r.w:%d\n", r.x(), r.width());  
      
      #ifdef VIEW_USE_DOUBLE_BUFFERING
      paint(r);
      #endif
      
      update(r);
      }

//---------------------------------------------------------
//   paint
//    r - phys coord system
//---------------------------------------------------------

void View::paint(const QRect& r)
      {
      #ifdef VIEW_USE_DOUBLE_BUFFERING
      if (pm.isNull())
            return;
      #endif
      
      QRect rr(r);
      
      //printf("View::paint x:%d width:%d y:%d height:%d\n", r.x(), r.width(), r.y(), r.height());   
      
      #ifdef VIEW_USE_DOUBLE_BUFFERING
      if (!pmValid) {
            pmValid = true;
            rr = QRect(0, 0, pm.width(), pm.height());
            }
      
      QPainter p(&pm);
      #else
      QPainter p(this);
      #endif
      
      p.setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform | QPainter::HighQualityAntialiasing, false);
      
      if (bgPixmap.isNull())
            p.fillRect(rr, brush);
      else
            p.drawTiledPixmap(rr, bgPixmap, QPoint(xpos + rmapx(xorg)
               + rr.x(), ypos + rmapy(yorg) + rr.y()));
      
      p.setClipRegion(rr);

      //printf("View::paint r.x:%d w:%d\n", rr.x(), rr.width());
      pdraw(p, rr);       // draw into pixmap

      p.resetMatrix();      // Q3 support says use resetMatrix instead, but resetMatrix advises resetTransform instead...
      //p.resetTransform();
      
      drawOverlay(p);
      }

//---------------------------------------------------------
//   keyPressEvent
//---------------------------------------------------------

void View::keyPressEvent(QKeyEvent* event)
      {
      viewKeyPressEvent(event);
      }

//---------------------------------------------------------
//   viewKeyPressEvent
//---------------------------------------------------------

void View::viewKeyPressEvent(QKeyEvent* event)
      {
      event->ignore();
      }

//---------------------------------------------------------
//   mousePressEvent
//---------------------------------------------------------

void View::mousePressEvent(QMouseEvent* ev)
      {
      QMouseEvent e(ev->type(), mapDev(ev->pos()),
         ev->globalPos(), ev->button(), ev->buttons(), ev->modifiers());
      viewMousePressEvent(&e);
      }

//---------------------------------------------------------
//   mouseDoubleClickEvent
//---------------------------------------------------------

void View::mouseDoubleClickEvent(QMouseEvent* ev)
      {
      QMouseEvent e(ev->type(), mapDev(ev->pos()),
         ev->globalPos(), ev->button(), ev->buttons(), ev->modifiers());
      viewMouseDoubleClickEvent(&e);
      }

//---------------------------------------------------------
//   mouseMoveEvent
//---------------------------------------------------------

void View::mouseMoveEvent(QMouseEvent* ev)
      {
      QMouseEvent e(ev->type(), mapDev(ev->pos()),
         ev->globalPos(), ev->button(), ev->buttons(), ev->modifiers());
      viewMouseMoveEvent(&e);
      }

//---------------------------------------------------------
//   mouseReleaseEvent
//---------------------------------------------------------

void View::mouseReleaseEvent(QMouseEvent* ev)
      {
      QMouseEvent e(ev->type(), mapDev(ev->pos()),
         ev->globalPos(), ev->button(), ev->buttons(), ev->modifiers());
      viewMouseReleaseEvent(&e);
      }

//---------------------------------------------------------
//   dropEvent
//---------------------------------------------------------

void View::dropEvent(QDropEvent* ev)
      {
      // From Q3 support:
      // "Sets the drop to happen at the given point. You do not normally need to use this 
      //  as it will be set internally before your widget receives the drop event."     
      // But we need to remap it here...
      //ev->setPoint(mapDev(ev->pos())); 
      QDropEvent nev(mapDev(ev->pos()), ev->possibleActions(), ev->mimeData(), ev->mouseButtons(), ev->keyboardModifiers(), ev->type());     
      //viewDropEvent(ev);
      viewDropEvent(&nev);
      }

//---------------------------------------------------------
//   setBg
//---------------------------------------------------------

void View::setBg(const QPixmap& bgpm)
      {
      bgPixmap = bgpm;
      redraw();
      }

//---------------------------------------------------------
//   pdraw
//    r - phys coords
//---------------------------------------------------------

void View::pdraw(QPainter& p, const QRect& r)
      {
      //printf("View::pdraw virt:%d x:%d width:%d y:%d height:%d\n", virt(), r.x(), r.width(), r.y(), r.height());  
      
      if (virt()) {
            setPainter(p);
            int x = r.x();
            int y = r.y();
            int w = r.width();
            int h = r.height();
            if (xmag <= 0) {
                  // TODO These adjustments are required, otherwise gaps. Tried, unable to remove them for now.  p4.0.30
                  x -= 1;   
                  w += 2;
                  //x = (x + xpos + rmapx(xorg)) * (-xmag);
                  x = lrint((double(x + xpos) + rmapx_f(xorg)) * double(-xmag));
                  w = w * (-xmag);
                  }
            else {
                  //x = (x + xpos + rmapx(xorg)) / xmag;
                  x = lrint((double(x + xpos) + rmapx_f(xorg)) / double(xmag));
                  //w = (w + xmag - 1) / xmag;
                  w = lrint(double(w) / double(xmag));
                  x -= 1;
                  w += 2;
                  }
            if (ymag <= 0) {
                  y -= 1;
                  h += 2;
                  //y = (y + ypos + rmapy(yorg)) * (-ymag);
                  y = lrint((double(y + ypos) + rmapy_f(yorg)) * double(-ymag));
                  h = h * (-ymag);
                  }
            else {
                  //y = (y + ypos + rmapy(yorg)) / ymag;
                  y = lrint((double(y + ypos) + rmapy_f(yorg)) / double(ymag));
                  //h = (h + ymag - 1) / ymag;
                  h = lrint(double(h) / double(ymag));
                  y -= 1;
                  h += 2;
                  }

            if (x < 0)
                  x = 0;
            if (y < 0)
                  y = 0;
            
            draw(p, QRect(x, y, w, h));
            }
      else
            draw(p, r);
      }

//---------------------------------------------------------
//   setPainter
//---------------------------------------------------------

void View::setPainter(QPainter& p)
      {
      p.resetMatrix();      // Q3 support says use resetMatrix instead, but resetMatrix advises resetTransform instead...
      //p.resetTransform();
      
      //p.translate(double(-(xpos+rmapx(xorg))), double(-(ypos+rmapy(yorg))));
      p.translate( -(double(xpos) + rmapx_f(xorg)) , -(double(ypos) + rmapy(yorg)));
      //double xMag = (xmag < 0) ? 1.0/(-xmag) : double(xmag);
      //double yMag = (ymag < 0) ? 1.0/(-ymag) : double(ymag);
      double xMag = (xmag < 0) ? 1.0/double(-xmag) : double(xmag);
      double yMag = (ymag < 0) ? 1.0/double(-ymag) : double(ymag);
      p.scale(xMag, yMag);
      }

//---------------------------------------------------------
//   drawTickRaster
//---------------------------------------------------------

void View::drawTickRaster(QPainter& p, int x, int y, int w, int h, int raster)
      {
      // Changed to draw in device coordinate space instead of virtual, transformed space.     Tim. p4.0.30  
      
      //int mx = mapx(x);
      int my = mapy(y);
      //int mw = mapx(x + w) - mx;
      //int mw = mapx(x + w) - mx - 1;
      //int mh = mapy(y + h) - my;
      //int mh = mapy(y + h) - my - 1;
      
      //p.save();
      bool wmtxen = p.worldMatrixEnabled();
      p.setWorldMatrixEnabled(false);
      
      int xx,bar1, bar2, beat;
      unsigned tick;
      AL::sigmap.tickValues(x, &bar1, &beat, &tick);
      AL::sigmap.tickValues(x+w, &bar2, &beat, &tick);
      ++bar2;
      ///int y2 = y + h;
      //int y2 = my + mh;
      int y2 = mapy(y + h) - 1;
      //printf("View::drawTickRaster x:%d y:%d w:%d h:%d mx:%d my:%d mw:%d mh:%d y2:%d bar1:%d bar2:%d\n", x, y, w, h, mx, my, mw, mh, y2, bar1, bar2);  
      //printf("View::drawTickRaster x:%d y:%d w:%d h:%d my:%d mh:%d y2:%d bar1:%d bar2:%d\n", x, y, w, h, my, mh, y2, bar1, bar2);  
      for (int bar = bar1; bar < bar2; ++bar) {
            ///unsigned x = AL::sigmap.bar2tick(bar, 0, 0);
            unsigned xb = AL::sigmap.bar2tick(bar, 0, 0);
            int xt = mapx(xb);
            p.setPen(Qt::black);
            ///p.drawLine(x, y, x, y2);
            p.drawLine(xt, my, xt, y2);
            int z, n;
            ///AL::sigmap.timesig(x, z, n);
            AL::sigmap.timesig(xb, z, n);
            ///int q = p.xForm(QPoint(raster, 0)).x() - p.xForm(QPoint(0, 0)).x();
            ///int q = p.combinedTransform().map(QPoint(raster, 0)).x() - p.combinedTransform().map(QPoint(0, 0)).x();
            //int q = rmapx(raster);
            int qq = raster;
            //if (q < 8)        // grid too dense
            if (rmapx(raster) < 8)        // grid too dense
                  qq *= 2;
            p.setPen(Qt::lightGray);
            if (raster>=4) {
                        ///int xx = x + qq;
                        //int xx = mapx(xb + qq);
                        xx = xb + qq;
                        int xxx = AL::sigmap.bar2tick(bar, z, 0);
                        //int xxx = mapx(AL::sigmap.bar2tick(bar, z, 0));
                        while (xx <= xxx) {
                               ///p.drawLine(xx, y, xx, y2);
                               int x = mapx(xx);
                               p.drawLine(x, my, x, y2);
                               xx += qq;
                               //xx += rmapx(qq);
                               }
                        //xx = xxx;
                        }
            p.setPen(Qt::gray);
            for (int beat = 1; beat < z; beat++) {
                        ///int xx = AL::sigmap.bar2tick(bar, beat, 0);
                        xx = mapx(AL::sigmap.bar2tick(bar, beat, 0));
                        //printf(" bar:%d z:%d beat:%d xx:%d\n", bar, z, beat, xx);  
                        ///p.drawLine(xx, y, xx, y2);
                        p.drawLine(xx, my, xx, y2);
                        }

            }
      //p.setWorldMatrixEnabled(true);
      p.setWorldMatrixEnabled(wmtxen);
      //p.restore();      
      }

//---------------------------------------------------------
//   map
//---------------------------------------------------------

QRect View::mapDev(const QRect& r) const
      {
      return QRect(mapxDev(r.x()), mapyDev(r.y()),
         rmapxDev(r.width()), rmapyDev(r.height()));
      }

QPoint View::mapDev(const QPoint& r) const
      {
      return QPoint(mapxDev(r.x()), mapyDev(r.y()));
      }

#if 0
  //
  // Calculations using integer rounding methods...
  //

QRect View::map(const QRect& r) const
      {
      int x, y, w, h;
      //printf("View::map xmag:%d xpos:%d xorg:%d\n", xmag, xpos, xorg);  
      if (xmag < 0) {
            x = r.x()/(-xmag) - (xpos + rmapx(xorg));  // round down
            w = (r.width()-xmag-1)  / (-xmag);  // round up
            }
      else {
            x = r.x()*xmag - (xpos + rmapx(xorg));
            w = r.width() * xmag;
            }
      if (ymag < 0) {
            y = r.y()/-ymag - (ypos + rmapy(yorg));
            h = (r.height()-ymag-1) / (-ymag);
            }
      else {
            y = r.y() * ymag - (ypos + rmapy(yorg));
            h = r.height() * ymag;
            }
      return QRect(x, y, w, h);
      }

QPoint View::map(const QPoint& p) const
      {
      int x, y;
      if (xmag < 0) {
            x = p.x()/(-xmag) - (xpos + rmapx(xorg));  // round down
            }
      else {
            x = p.x()*xmag - (xpos + rmapx(xorg));
            }
      if (ymag < 0) {
            y = p.y()/-ymag - (ypos + rmapy(yorg));
            }
      else {
            y = p.y() * ymag - (ypos + rmapy(yorg));
            }
      return QPoint(x, y);
      }

int View::mapx(int x) const
      {
      if (xmag < 0) {
            return (x-xmag/2)/(-xmag) - (xpos + rmapx(xorg));  // round
            }
      else {
            return (x * xmag) - (xpos + rmapx(xorg));
            }
      }
int View::mapy(int y) const
      {
      if (ymag < 0) {
            return (y-ymag/2)/(-ymag) - (ypos + rmapy(yorg));  // round
            }
      else {
            return (y * ymag) - (ypos + rmapy(yorg));
            }
      }
int View::mapxDev(int x) const
      {
      int val;
      if (xmag <= 0)
            val = (x + xpos + rmapx(xorg)) * (-xmag);
      else
            val = (x + xpos + rmapx(xorg) + xmag / 2) / xmag;
      if (val < 0)            // DEBUG
            val = 0;
      return val;
      }

int View::mapyDev(int y) const
      {
      if (ymag <= 0)
            return (y + ypos + rmapy(yorg)) * (-ymag);
      else
            return (y + ypos + rmapy(yorg) + ymag / 2) / ymag;
      }

// r == relative conversion
int View::rmapx(int x) const
      {
      if (xmag < 0)
            return (x-xmag/2) / (-xmag);
      else
            return x * xmag;
      }
int View::rmapy(int y) const
      {
      if (ymag < 0)
            return (y-ymag/2) / (-ymag);
      else
            return y * ymag;
      }
int View::rmapxDev(int x) const
      {
      if (xmag <= 0)
            return x * (-xmag);
      else
            return (x + xmag/2) / xmag;
      }
int View::rmapyDev(int y) const
      {
      if (ymag <= 0)
            return y * (-ymag);
      else
            return (y + ymag/2) / ymag;
      }


#else
  //
  // Calculations using more accurate methods...  p4.0.29 Tim.
  //

QRect View::map(const QRect& r) const
      {
      //int x, y, w, h;
      double x, y, w, h;
      int xx, yy, ww, hh;
      //printf("View::map xmag:%d xpos:%d xorg:%d\n", xmag, xpos, xorg);  
      if (xmag < 0) {
            //x = lrint(double(r.x())/double(-xmag) - rmapx_f(xorg)) - xpos;  
            x = double(r.x())/double(-xmag) - rmapx_f(xorg) - xpos;  
            //w = lrint(double(r.width()) / double(-xmag));  
            w = double(r.width()) / double(-xmag);  
            xx = lrint(x);
            ww = lrint(x + w) - xx;
            }
      else {
            xx = r.x()*xmag - xpos - lrint(rmapx_f(xorg));
            ww = r.width() * xmag;
            }
      if (ymag < 0) {
            //y = lrint(double(r.y())/double(-ymag) - rmapy_f(yorg)) - ypos;
            y = double(r.y())/double(-ymag) - rmapy_f(yorg) - ypos;
            //h = lrint(double(r.height()) / double(-ymag));
            h = double(r.height()) / double(-ymag);
            yy = lrint(y);
            hh = lrint(y + h) - yy;
            }
      else {
            yy = r.y()*ymag - ypos - lrint(rmapy_f(yorg));
            hh = r.height() * ymag;
            }
      return QRect(xx, yy, ww, hh);
      }

QPoint View::map(const QPoint& p) const
      {
      /*
      int x, y;
      if (xmag < 0) {
            x = lrint(double(p.x())/double(-xmag) - rmapx_f(xorg)) - xpos;  
            }
      else {
            x = p.x()*xmag - xpos - lrint(rmapx_f(xorg));
            }
      if (ymag < 0) {
            y = lrint(double(p.y())/double(-ymag) - rmapy_f(yorg)) - ypos;
            }
      else {
            y = p.y()*ymag - ypos - lrint(rmapy_f(yorg));
            }
      return QPoint(x, y);
      */
      return QPoint(mapx(p.x()), mapy(p.y()));
      }

int View::mapx(int x) const
      {
      if (xmag < 0) {
            return lrint(double(x)/double(-xmag) - rmapx_f(xorg)) - xpos;  
            }
      else {
            return x*xmag - xpos - lrint(rmapx_f(xorg));
            }
      }
int View::mapy(int y) const
      {
      if (ymag < 0) {
            return lrint(double(y)/double(-ymag) - rmapy_f(yorg)) - ypos;  
            }
      else {
            return y*ymag - ypos - lrint(rmapy_f(yorg));
            }
      }
int View::mapxDev(int x) const
      {
      int val;
      if (xmag <= 0)
            val = lrint((double(x + xpos) + rmapx_f(xorg)) * double(-xmag));                    
      else
            val = lrint((double(x + xpos) + rmapx_f(xorg)) / double(xmag));  
      if (val < 0)            // DEBUG
            val = 0;
      return val;
      }

int View::mapyDev(int y) const
      {
      if (ymag <= 0)
            return lrint((double(y + ypos) + rmapy_f(yorg)) * double(-ymag));                   
      else
            return lrint((double(y + ypos) + rmapy_f(yorg)) / double(ymag)); 
      }

// r == relative conversion
int View::rmapx(int x) const
      {
      if (xmag < 0)
            return lrint(double(x) / double(-xmag));
      else
            return x * xmag;
      }
int View::rmapy(int y) const
      {
      if (ymag < 0)
            return lrint(double(y) / double(-ymag));
      else
            return y * ymag;
      }
int View::rmapxDev(int x) const
      {
      if (xmag <= 0)
            return x * (-xmag);
      else
            return lrint(double(x) / double(xmag));
      }
int View::rmapyDev(int y) const
      {
      if (ymag <= 0)
            return y * (-ymag);
      else
            return lrint(double(y) / double(ymag));
      }
#endif

double View::rmapx_f(double x) const
      {
      if (xmag < 0)
            return x / double(-xmag);
      else
            return x * double(xmag);
      }
double View::rmapy_f(double y) const
      {
      if (ymag < 0)
            return y / double(-ymag);
      else
            return y * double(ymag);
      }
double View::rmapxDev_f(double x) const
      {
      if (xmag <= 0)
            return x * double(-xmag);
      else
            return x / double(xmag);
      }
double View::rmapyDev_f(double y) const
      {
      if (ymag <= 0)
            return y * double(-ymag);
      else
            return y / double(ymag);
      }



/*
QRect View::devToVirt(const QRect& r)
{
    int x = r.x();
    int y = r.y();
    int w = r.width();
    int h = r.height();
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
          y = (y + ypos + rmapy(yorg)) / ymag;
          h = (h + ymag - 1) / ymag;
          y -= 1;
          h += 2;
          }

    if (x < 0)
          x = 0;
    if (y < 0)
          y = 0;
    
    return QRect(x, y, w, h);
}
*/

} // namespace MusEGui
