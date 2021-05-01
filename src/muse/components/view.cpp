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
#include "gconfig.h"
#include <stdio.h>
#include <QVector>
#include "tempo.h"

#include "muse_math.h"

#include "sig.h"  

// Forwards from header:
#include <QDropEvent>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QPaintEvent>
#include <QPainter>
#include <QResizeEvent>

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
            paint(ev->rect(), ev->region());
      
      //bitBlt(this, ev->rect().topLeft(), &pm, ev->rect(), CopyROP, true);
      QPainter p(this);
      //p.setCompositionMode(QPainter::CompositionMode_Source);
      p.drawPixmap(ev->rect().topLeft(), pm, ev->rect());
      
      #else
      paint(ev->rect(), ev->region());
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
      //fprintf(stderr, "View::redraw(QRect& r) r.x:%d r.w:%d\n", r.x(), r.width());  
      
      #ifdef VIEW_USE_DOUBLE_BUFFERING
      paint(r);
      #endif
      
      update(r);
      }

//---------------------------------------------------------
//   redraw
//---------------------------------------------------------

void View::redraw(const QRegion& r)
      {
      #ifdef VIEW_USE_DOUBLE_BUFFERING
      paint(r);
      #endif
      
      update(r);
      }

//---------------------------------------------------------
//   paint
//    r - phys coord system
//---------------------------------------------------------

void View::paint(const QRect& r, const QRegion& rg)
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

// For testing...
//       const QVector<QRect> rects = rg.rects();
//       const int rg_sz = rects.size();
//       int rg_r_cnt = 0;
//       fprintf(stderr, "View::paint: virt:%d rect: x:%d y:%d w:%d h:%d region rect count:%d\n",
//               virt(), r.x(), r.y(), r.width(), r.height(), rg_sz);
//       for(int i = 0; i < rg_sz; ++i, ++rg_r_cnt)
//       {
//         const QRect& rg_r = rects.at(i);
//         fprintf(stderr, "  #%d: x:%d y:%d w:%d h:%d\n", rg_r_cnt, rg_r.x(), rg_r.y(), rg_r.width(), rg_r.height());
//       }   

      p.setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform, false);
      
      if (bgPixmap.isNull())
            p.fillRect(rr, brush);
      else
            p.drawTiledPixmap(rr, bgPixmap, QPoint(xpos + xorg
               + rr.x(), ypos + yorg + rr.y()));
      
      p.setClipRegion(rg);

      //printf("View::paint r.x:%d w:%d\n", rr.x(), rr.width());
      pdraw(p, rr, rg);       // draw into pixmap

      //p.resetMatrix();      // Q3 support says use resetMatrix instead, but resetMatrix advises resetTransform instead...
      p.resetTransform();     // resetMatrix() is deprecated in Qt 5.13
      
      drawOverlay(p, r, rg);
      }

//---------------------------------------------------------
//   keyPressEvent
//---------------------------------------------------------

void View::keyPressEvent(QKeyEvent* event)
      {
      viewKeyPressEvent(event);
      }

//---------------------------------------------------------
//   keyReleaseEvent
//---------------------------------------------------------

void View::keyReleaseEvent(QKeyEvent* event)
      {
      viewKeyReleaseEvent(event);
      }

//---------------------------------------------------------
//   viewKeyPressEvent
//---------------------------------------------------------

void View::viewKeyPressEvent(QKeyEvent* event)
      {
      event->ignore();
      }

//---------------------------------------------------------
//   viewKeyReleaseEvent
//---------------------------------------------------------

void View::viewKeyReleaseEvent(QKeyEvent* event)
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
//   devToVirt
//---------------------------------------------------------

QRect View::devToVirt(const QRect& r) const
{
  return mapDev(r);
}

void View::devToVirt(const QRegion& rg_in, QRegion& rg_out) const
{
  mapDev(rg_in, rg_out);
}

//---------------------------------------------------------
//   pdraw
//    r - phys coords
//---------------------------------------------------------

void View::pdraw(QPainter& p, const QRect& r, const QRegion& rg)
      {
      //printf("View::pdraw virt:%d x:%d width:%d y:%d height:%d\n", virt(), r.x(), r.width(), r.y(), r.height());  
      
      if (virt()) {
            setPainter(p);
            draw(p, r, rg);
            }
      else
            draw(p, r, rg);
      }

//---------------------------------------------------------
//   setPainter
//---------------------------------------------------------

void View::setPainter(QPainter& p)
      {
      //p.resetMatrix();      // Q3 support says use resetMatrix instead, but resetMatrix advises resetTransform instead...
      p.resetTransform();     // resetMatrix() is deprecated in Qt 5.13
      
      p.translate( -(double(xpos) + double(xorg)) , -(double(ypos) + double(yorg)));
      double xMag = (xmag < 0) ? 1.0/double(-xmag) : double(xmag);
      double yMag = (ymag < 0) ? 1.0/double(-ymag) : double(ymag);
      p.scale(xMag, yMag);
      }

void View::drawBarText(QPainter& p, int tick, int bar, const QRect& mr, const QColor& textColor, const QFont& font) const
{
  QPen pen;
  pen.setCosmetic(true);
  pen.setColor(textColor);
  p.setPen(pen);
  p.setFont(font);
  QString s;
  s.setNum(bar + 1);
    
  ViewRect r(mr, true);
  
  const int mw1000 = 1000;
  const int mw2 = 2;
  const int my = mr.y();
  const int mh_m1 = mr.height() - 1;
  
  int mbrw = p.fontMetrics().boundingRect(s).width();
  
  if(mbrw > mw1000)
    mbrw = mw1000;
  
  const ViewRect br_txt = ViewRect(
      mathXCoordinates(ViewXCoordinate(tick, false), ViewWCoordinate(mw2, true), MathAdd),
      ViewYCoordinate(my, true),
      ViewWCoordinate(mbrw, true),
      ViewHCoordinate(mh_m1, true));
  
// For testing...
//     fprintf(stderr,
//         "drawBarText: Bar text: bar:%d vx:%d vy:%d"
//         " vw:%d vh:%d tick:%d br x:%d y:%d w:%d h:%d\n",
//         bar, vr.x(), vr.y(), vr.width(), vr.height(), tick,
//         br_txt.x(), br_txt.y(), br_txt.width(), br_txt.height());
//     fprintf(stderr,
//         "drawBarText: Bar text: bar:%d mx:%d my:%d"
//         " mw:%d mh:%d tick:%d br x:%d y:%d w:%d h:%d\n",
//         bar, mr.x(), mr.y(), mr.width(), mr.height(), tick,
//         br_txt._x._value, br_txt._y._value, br_txt._width._value, br_txt._height._value);
  
  if(intersects(br_txt, r))
  {
    const QRect br_text_qr = asQRectMapped(br_txt);
// For testing...
//       fprintf(stderr, "...bar text within range. xorg:%d xmag:%d xpos:%d"
//                       " Drawing bar text at x:%d y:%d w:%d h:%d\n",
//               xorg, xmag, xpos, br_text_qr.x(), br_text_qr.y(), br_text_qr.width(), br_text_qr.height());
    
//       p.drawText(mbr_txt, Qt::AlignLeft|Qt::AlignVCenter|Qt::TextDontClip, s);
    p.drawText(br_text_qr, Qt::AlignLeft|Qt::AlignVCenter|Qt::TextDontClip, s);
  }
}

View::ScaleRetStruct View::scale(bool drawText, int bar, double tpix, int raster) const
{
  ScaleRetStruct ret;
  ret._drawBar = true;
  
  int n = 1;
  if(drawText)
  {
    //fprintf(stderr, "drawText: tpix < 64:%f\n", tpix);
    if (tpix <= 2)
    {
      //fprintf(stderr, "drawText: tpix <= 2\n");
      n <<= 5;
    }
    else if (tpix <= 4)
    {
      //fprintf(stderr, "drawText: tpix <= 4\n");
      n <<= 4;
    }
    else if (tpix < 8)
    {
      //fprintf(stderr, "drawText: tpix < 8\n");
      n <<= 3;
    }
    else if (tpix <= 16)
    {
      //fprintf(stderr, "drawText: tpix <= 16\n");
      n <<= 2;
    }
    else if (tpix < 32)
    {
      //fprintf(stderr, "drawText: tpix < 32\n");
      n <<= 1;
    }

    ret._newRaster = raster * n;

    ret._isSmall = tpix < 64;
    if(!ret._isSmall)
      return ret;

    if (bar % n)
      ret._drawBar = false;
  }
  else
  {
    //fprintf(stderr, "tpix < 32:%f\n", tpix);
    n = 1;
    if (tpix <= 0.01)
    {
      //fprintf(stderr, "tpix <= 0.01\n");
      n <<= 11;
    }
    else if (tpix <= 0.03125)
    {
      //fprintf(stderr, "tpix <= 0.03125\n");
      n <<= 10;
    }
    else if (tpix <= 0.0625)
    {
      //fprintf(stderr, "tpix <= 0.0625\n");
      n <<= 9;
    }
    else if (tpix <= 0.125)
    {
      //fprintf(stderr, "tpix <= 0.125\n");
      n <<= 8;
    }
    else if (tpix <= 0.25)
    {
      //fprintf(stderr, "tpix <= 0.25\n");
      n <<= 7;
    }
    else if (tpix <= 0.5)
    {
      //fprintf(stderr, "tpix <= 0.5\n");
      n <<= 6;
    }
    else if (tpix <= 1.0)
    {
      //fprintf(stderr, "tpix <= 1.0\n");
      n <<= 5;
    }
    else if (tpix <= 2.0)
    {
      //fprintf(stderr, "tpix <= 2.0\n");
      n <<= 4;
    }
    else if (tpix <= 4.0)
    {
      //fprintf(stderr, "tpix <= 4.0\n");
      n <<= 3;
    }
    else if (tpix <= 8.0)
    {
      //fprintf(stderr, "tpix <= 8.0\n");
      n <<= 2;
    }
    else if (tpix <= 32.0)
    {
      //fprintf(stderr, "tpix <= 32.0\n");
      n <<= 1;
    }

    ret._newRaster = raster * n;

    ret._isSmall = tpix < 32;
    if(!ret._isSmall)
      return ret;

    // FIXME: Hack?
    if (bar % (n > 1 ? (n >> 1) : n))
      ret._drawBar = false;
  }

  return ret;
}

//---------------------------------------------------------
//   drawTickRaster
//---------------------------------------------------------

void View::drawTickRaster(
  QPainter& p, const QRect& mr, const QRegion& /*mrg*/, int raster,
  bool waveMode, 
  bool /*useGivenColors*/,
  bool /*drawText*/,
  const QColor& bar_color,
  const QColor& beat_color,
  const QColor& fine_color,
  const QColor& coarse_color,
  const QColor& /*text_color*/,
  const QFont& /*large_font*/,
  const QFont& /*small_font*/
  )
{
      const ViewRect r(mr, true);
      const ViewXCoordinate& x = r._x;
      const ViewXCoordinate x_2(mr.x() + mr.width(), true);
      
      //p.save();
      bool wmtxen = p.worldMatrixEnabled();
      p.setWorldMatrixEnabled(false);

      // Limiter required because MusEGlobal::sigmap.tickValues takes unsigned only !
      const ViewXCoordinate lim_v0(0, false);
      const ViewXCoordinate x_lim = compareXCoordinates(x, lim_v0, CompareLess) ? lim_v0 : x;
      const ViewXCoordinate x_2lim = compareXCoordinates(x_2, lim_v0, CompareLess) ? lim_v0 : x_2;

      const int my = mr.y();
      const int mh = mr.height();

      const int mbottom = mr.bottom();

      const ViewYCoordinate bottom(mr.bottom(), true);

      const ViewWCoordinate w2(2, true);
      const ViewHCoordinate h2(2, true);
      const ViewHCoordinate h_m1(mh - 1, true);
      const ViewHCoordinate h_m3(mh - 3, true);
      
      int bar1, bar2, beat, beatBar;
      ScaleRetStruct scale_info;
      int rast_x, next_rast_xx, rast_xx, raster_scaled;
      unsigned tick, conv_rast_xx, conv_next_rast_xx;
      double rast_mapx;

      QPen pen;
      pen.setCosmetic(true);

// For testing...
//       fprintf(stderr, "View::drawTickRaster_new(): virt:%d drawText:%d mx:%d my:%d mw:%d mh:%draster%d\n",
//               virt(), drawText, mx, my, mw, mh, raster);  
      
      if (waveMode) {
            MusEGlobal::sigmap.tickValues(MusEGlobal::tempomap.frame2tick(asUnmapped(x_lim)._value), &bar1, &beat, &tick);
            MusEGlobal::sigmap.tickValues(MusEGlobal::tempomap.frame2tick(asUnmapped(x_2lim)._value), &bar2, &beat, &tick);
            }
      else {
            MusEGlobal::sigmap.tickValues(asUnmapped(x_lim)._value, &bar1, &beat, &tick);
            MusEGlobal::sigmap.tickValues(asUnmapped(x_2lim)._value, &bar2, &beat, &tick);
            }

      //fprintf(stderr, "bar %d  %d-%d=%d\n", bar, ntick, stick, ntick-stick);

      int stick = MusEGlobal::sigmap.bar2tick(bar1, 0, 0);
      int ntick, deltaTick;
      for (int bar = bar1; bar <= bar2; bar++, stick = ntick) {
            ntick     = MusEGlobal::sigmap.bar2tick(bar+1, 0, 0);
            deltaTick = ntick - stick;
            int a, b=0;
            double tpix;
            if (waveMode) {
                  a = MusEGlobal::tempomap.tick2frame(ntick);
                  b = MusEGlobal::tempomap.tick2frame(stick);
                     tpix  = rmapx_f(a - b);
               }
            else {
                  tpix  = rmapx_f(deltaTick);
                  }
                  
            scale_info = scale(false, bar, tpix, raster);
            
            if(scale_info._drawBar)
            {
              const int tick_sm = waveMode ? b : stick;
              
              const ViewXCoordinate x_sm(tick_sm, false);
              if(compareXCoordinates(x_sm, x_2, CompareGreaterEqual))
                break;
              if(compareXCoordinates(x_sm, x, CompareGreaterEqual))
              //if(isXInRange(x_sm, x, x_2))
              {
                const int mx_sm = asMapped(x_sm)._value;
                
                const ScaleRetStruct scale_info_text_lines = scale(true, bar, tpix, raster);
                if (scale_info_text_lines._drawBar) {
                  // highlight lines drawn with text
                  pen.setColor(coarse_color);
                } else {
                  pen.setColor(bar_color);
                }
                p.setPen(pen);
              
// For testing...
//                 fprintf(stderr,
//                   "is_small:%d tpix:%.15f Coarse line: tick_sm:%d mx_sm:%d stick:%d ntick:%d bar1:%d bar2:%d bar:%d\n",
//                   scale_info._isSmall, tpix, tick_sm, mx_sm, stick, ntick, bar1, bar2, bar);
//                 fprintf(stderr, "is_small:%d Coarse Line is within range."
//                       " Drawing line at mx_sm:%d my:%d mx_sm:%d mbottom:%d tpix:%.15f\n",
//                       scale_info._isSmall, mx_sm, my, mx_sm, mbottom, tpix);
                
                p.drawLine(mx_sm, my, mx_sm, mbottom);
              }
            }
            
            if(!scale_info._isSmall)
            {
              // If the raster is on 'bar' or is greater than a full bar, we limit the raster to a full bar.
              if (raster >= 4 && raster < deltaTick)
              {
                for (rast_xx = stick; rast_xx < ntick; /*rast_xx += raster*/)
                {
                  next_rast_xx = rast_xx + raster;
                  if(waveMode)
                  {
                    conv_rast_xx = MusEGlobal::tempomap.tick2frame(rast_xx);
                    conv_next_rast_xx = MusEGlobal::tempomap.tick2frame(next_rast_xx);
                  }
                  else
                  {
                    conv_rast_xx = rast_xx;
                    conv_next_rast_xx = next_rast_xx;
                  }

                  rast_mapx = rmapx_f(conv_next_rast_xx - conv_rast_xx);
                  // NOTE: had to add this 4.0 magic value to draw the restart closer to how it used to be
                  //       without it not every point where you can put a note would have a line on many zoom levels
                  rast_mapx = rast_mapx * 4.0;
                  const ScaleRetStruct scale_info_fine_lines = scale(false, bar, rast_mapx, raster);
                  raster_scaled = scale_info_fine_lines._newRaster;
                  // Infinite loop protection.
                  if(raster_scaled < 1)
                    raster_scaled = 1;

                  const ViewXCoordinate x_sm(conv_rast_xx, false);
                  if(compareXCoordinates(x_sm, x_2, CompareGreaterEqual))
                    break;
                  if(compareXCoordinates(x_sm, x, CompareGreaterEqual))
                  //if(isXInRange(x_sm, x, x_2))
                  {

                    // Ignore already drawn bar lines.
                    if(!scale_info._drawBar || rast_xx != stick)
                    {
                      // If the line happens to be exactly on a beat, emphasize that it's a beat line.
                      // Don't bother drawing the beat lines if we want to always draw them,
                      //  since that is already done below.
                      // FIXME: This loop doesn't know EXACTLY whether all beat lines will be drawn below.
                      // So it is possible that in some low raster conditions the lines won't be drawn.
                      // But to avoid redundant duplicate drawing of beat lines we do this anyway.
                      // If you really want all the lines drawn, remove the second part of this conditional.
                      // Since canvasShowGridBeatsAlways is false by default, most users won't notice.
                      MusEGlobal::sigmap.tickValues(rast_xx, &beatBar, &beat, &tick);
                      if(tick != 0 || (tick == 0 && !MusEGlobal::config.canvasShowGridBeatsAlways))
                      {
                        if(tick == 0)
                          pen.setColor(beat_color);
                        else
                          pen.setColor(fine_color);
                        p.setPen(pen);

                        rast_x = mapx(conv_rast_xx);

// For testing...
//                           fprintf(stderr,
//                           "is_small:%d Fine line: raster:%d raster_scaled:%d rast_xx:%d"
//                           " rast_x:%d stick:%d ntick:%d bar1:%d bar2:%d bar:%d\n",
//                           scale_info._isSmall, raster, raster_scaled, rast_xx, 
//                           rast_x, stick, ntick, bar1, bar2, bar);

                        p.drawLine(rast_x, my, rast_x, mbottom);
                      }
                    }
                  }
                  rast_xx += raster_scaled;
                }
              }
            }

            // If we want to show beat lines always, do them here instead of above.
            // If the raster is on 'bar' or is greater than a full bar, we limit the raster to a full bar.
            if(MusEGlobal::config.canvasShowGridBeatsAlways &&
               raster > 0 /*&& raster < deltaTick*/ && !scale_info._isSmall) {
                  int z, n;
                  MusEGlobal::sigmap.timesig(stick, z, n);
                  
                  for (int beat = 0; beat < z; /*beat++*/) {
                        int xx = MusEGlobal::sigmap.bar2tick(bar, beat, 0);
                        int xx_e = MusEGlobal::sigmap.bar2tick(bar, beat + 1, 0);
                        if(waveMode)
                        {
                          xx = MusEGlobal::tempomap.tick2frame(xx);
                          xx_e = MusEGlobal::tempomap.tick2frame(xx_e);
                        }
                        
                        rast_mapx = rmapx_f(xx_e - xx);
                        // FIXME: Arbitrary value, hope this is satisfactory.
                        rast_mapx = rast_mapx * 4.0;
                        const ScaleRetStruct scale_info_beat_lines = scale(false, bar, rast_mapx, 1);
                        raster_scaled = scale_info_beat_lines._newRaster;
                        // Infinite loop protection.
                        if(raster_scaled < 1)
                          raster_scaled = 1;

                        // Ignore already drawn bar lines.
                        if(!scale_info._drawBar || beat != 0)
                        {
                          const ViewXCoordinate xx_v(xx, false);
                          if(compareXCoordinates(xx_v, x_2, CompareGreaterEqual))
                            break;
                          if(compareXCoordinates(xx_v, x, CompareGreaterEqual))
                          //if(isXInRange(xx_v, x, x_2))
                          {
                            const int mxx = asMapped(xx_v)._value;
                            
// For testing...
//                             fprintf(stderr, "is_small:%d Beat Line is within range."
//                                   " Drawing line at mxx:%d my:%d mbottom:%d...\n",
//                                   scale_info._isSmall, mxx, my, mbottom);
                            
                            pen.setColor(beat_color);
                            p.setPen(pen);
                            p.drawLine(mxx, my, mxx, mbottom);
                          }
                        }
                        
                        beat += raster_scaled;
                        }
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
    rmapxDev(r.width(), true), rmapyDev(r.height(), true));
}

QPoint View::mapDev(const QPoint& r) const
{
  return QPoint(mapxDev(r.x()), mapyDev(r.y()));
}

void View::mapDev(const QRegion& rg_in, QRegion& rg_out) const
{
  for (const auto& it : rg_in)
    rg_out += mapDev(it);
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
            x = r.x()/(-xmag) - (xpos + xorg);  // round down
            w = (r.width()-xmag-1)  / (-xmag);  // round up
            }
      else {
            x = r.x()*xmag - (xpos + xorg);
            w = r.width() * xmag;
            }
      if (ymag < 0) {
            y = r.y()/-ymag - (ypos + yorg);
            h = (r.height()-ymag-1) / (-ymag);
            }
      else {
            y = r.y() * ymag - (ypos + yorg);
            h = r.height() * ymag;
            }
      return QRect(x, y, w, h);
      }

QPoint View::map(const QPoint& p) const
      {
      int x, y;
      if (xmag < 0) {
            x = p.x()/(-xmag) - (xpos + xorg);  // round down
            }
      else {
            x = p.x()*xmag - (xpos + xorg);
            }
      if (ymag < 0) {
            y = p.y()/-ymag - (ypos + yorg);
            }
      else {
            y = p.y() * ymag - (ypos + yorg);
            }
      return QPoint(x, y);
      }

int View::mapx(int x) const
      {
      if (xmag < 0) {
            return (x-xmag/2)/(-xmag) - (xpos + xorg);  // round
            }
      else {
            return (x * xmag) - (xpos + xorg);
            }
      }
int View::mapy(int y) const
      {
      if (ymag < 0) {
            return (y-ymag/2)/(-ymag) - (ypos + yorg);  // round
            }
      else {
            return (y * ymag) - (ypos + yorg);
            }
      }
int View::mapxDev(int x) const
      {
      int val;
      if (xmag <= 0)
            val = (x + xpos + xorg) * (-xmag);
      else
            val = (x + xpos + xorg + xmag / 2) / xmag;
      if (val < 0)            // DEBUG
            val = 0;
      return val;
      }

int View::mapyDev(int y) const
      {
      if (ymag <= 0)
            return (y + ypos + yorg) * (-ymag);
      else
            return (y + ypos + yorg + ymag / 2) / ymag;
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
  // Calculations using more accurate methods...
  //

QRect View::map(const QRect& r) const
{
  return QRect(mapx(r.x()), mapy(r.y()),
    rmapx(r.width(), true), rmapy(r.height(), true));
}

QPoint View::map(const QPoint& p) const
      {
      return QPoint(mapx(p.x()), mapy(p.y()));
      }

void View::map(const QRegion& rg_in, QRegion& rg_out) const
{
  for (const auto& it : rg_in)
    rg_out += map(it);
}
      
int View::mapx(int x) const
      {
      if (xmag < 0)
            return floor(double(x) / double(-xmag)) - xpos - xorg;

      else 
            return x * xmag - xpos - xorg;

      }
int View::mapy(int y) const
      {
      if (ymag < 0)
            return floor(double(y) / double(-ymag)) - ypos - yorg;
      else
            return y * ymag - ypos - yorg;
      }
int View::mapxDev(int x) const
      {
      if (xmag <= 0)
            return  (x + xpos + xorg) * -xmag;
      else
            return floor(double(x + xpos + xorg) / double(xmag));
      }

int View::mapyDev(int y) const
      {
      if (ymag <= 0)
            return (y + ypos + yorg) * -ymag;
      else
            return floor(double(y + ypos + yorg) / double(ymag));
      }

//-----------------------------
// r == relative conversion
//-----------------------------

QRect View::rmap(const QRect& r) const
{
  return QRect(rmapx(r.x()), rmapy(r.y()),
    rmapx(r.width(), true), rmapy(r.height(), true));
}

int View::rmapx(int x, bool round) const
      {
      if (xmag < 0)
      {
            if(round)
              return ceil(double(x) / double(-xmag));
            else
              return floor(double(x) / double(-xmag));
      }
      else
            return x * xmag;
      }
int View::rmapy(int y, bool round) const
      {
      if (ymag < 0)
      {
            if(round)
              return ceil(double(y) / double(-ymag));
            else
              return floor(double(y) / double(-ymag));
      }
      else
            return y * ymag;
      }
int View::rmapxDev(int x, bool round) const
      {
      if (xmag <= 0)
            return x * (-xmag);
      else
      {
            if(round)
              return ceil(double(x) / double(xmag));
            else
              return floor(double(x) / double(xmag));
      }
      }
int View::rmapyDev(int y, bool round) const
      {
      if (ymag <= 0)
            return y * (-ymag);
      else
      {
            if(round)
              return ceil(double(y) / double(ymag));
            else
              return floor(double(y) / double(ymag));
      }
      }
QPoint View::rmapDev(const QPoint& p, bool round) const
{
  int x, y;

  if (xmag <= 0)
    x = p.x() * (-xmag);
  else
  {
    if(round)
      x = ceil(double(p.x()) / double(xmag));
    else
      x = floor(double(p.x()) / double(xmag));
  }

  if (ymag <= 0)
    y = p.y() * (-ymag);
  else
  {
    if(round)
      y = ceil(double(p.y()) / double(ymag));
    else
      y = floor(double(p.y()) / double(ymag));
  }

  return QPoint(x, y);
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

//--------------------------------------------------
// Lossless intersections (in x and/or y, depending)
//  for any magnification.
//--------------------------------------------------

void ViewRect::dump(const char* header) const
{
  if(header)
    fprintf(stderr, "%s\n", header);
  else
    fprintf(stderr, "ViewRect dump:\n");
  
  fprintf(stderr, "x:%8d  map:%d rel:%d\t  y:%8d  map:%d rel:%d\t  w:%8d  map:%d rel:%d\t  h:%8d  map:%d rel:%d\n\n",
          _x._value, _x.isMapped(), _x.isRelative(),
          _y._value, _y.isMapped(), _y.isRelative(),
          _width._value, _width.isMapped(), _width.isRelative(),
          _height._value, _height.isMapped(), _height.isRelative());
}


// Returns the intersection of the rectangles. The coordinate x, y, and w, h are mapped
//  if the view's xmag > 0, and unmapped if not. The coordinate w, h are mapped if the
//  view's ymag > 0, and unmapped if not.
ViewRect View::intersected(const ViewRect& r1, const ViewRect& r2) const
{
  int fx_1, fw, fy_1, fh;
  const bool xunmap = xmag <= 0;
  const bool yunmap = ymag <= 0;
  if (xunmap)
  {
    const int vr1x = r1._x.isMapped() ? mapxDev(r1._x._value) : r1._x._value;
    const int vr1x_2 = vr1x + (r1._width.isMapped() ? rmapxDev(r1._width._value) : r1._width._value);
    
    const int vr2x = r2._x.isMapped() ? mapxDev(r2._x._value) : r2._x._value;
    const int vr2x_2 = vr2x + (r2._width.isMapped() ? rmapxDev(r2._width._value) : r2._width._value);
    
    fx_1 = vr1x > vr2x ? vr1x : vr2x;
    const int fx_2 = vr1x_2 < vr2x_2 ? vr1x_2 : vr2x_2;
    fw = fx_2 - fx_1;
    
// For testing...
//     fprintf(stderr, "View::intersected: xmag:%d vr1x:%d vr1x_2:%d vr2x:%d vr2x_2:%d fx_1:%d fx_2:%d\n",
//             xmag, vr1x, vr1x_2, vr2x, vr2x_2, fx_1, fx_2);
  }
  else
  {
    const int mr1x = r1._x.isMapped() ? r1._x._value : mapx(r1._x._value);
    const int mr1x_2 = mr1x + (r1._width.isMapped() ? r1._width._value : rmapx(r1._width._value));
    
    const int mr2x = r2._x.isMapped() ? r2._x._value : mapx(r2._x._value);
    const int mr2x_2 = mr2x + (r2._width.isMapped() ? r2._width._value : rmapx(r2._width._value));
    
    fx_1 = mr1x > mr2x ? mr1x : mr2x;
    const int fx_2 = mr1x_2 < mr2x_2 ? mr1x_2 : mr2x_2;
    fw = fx_2 - fx_1;
    
// For testing...
//     fprintf(stderr, "View::intersected: xmag:%d mr1x:%d mr1x_2:%d mr2x:%d mr2x_2:%d fx_1:%d fx_2:%d\n",
//             xmag, mr1x, mr1x_2, mr2x, mr2x_2, fx_1, fx_2);
  }
  
  if (ymag <= 0)
  {
    const int vr1y = r1._y.isMapped() ? mapyDev(r1._y._value) : r1._y._value;
    const int vr1y_2 = vr1y + (r1._height.isMapped() ? rmapyDev(r1._height._value) : r1._height._value);
    
    const int vr2y = r2._y.isMapped() ? mapyDev(r2._y._value) : r2._y._value;
    const int vr2y_2 = vr2y + (r2._height.isMapped() ? rmapyDev(r2._height._value) : r2._height._value);
    
    fy_1 = vr1y > vr2y ? vr1y : vr2y;
    const int fy_2 = vr1y_2 < vr2y_2 ? vr1y_2 : vr2y_2;
    fh = fy_2 - fy_1;
    
// For testing...
//     fprintf(stderr, "View::intersected: ymag:%d vr1y:%d vr1y_2:%d vr2y:%d vr2y_2:%d fy_1:%d fy_2:%d\n",
//             ymag, vr1y, vr1y_2, vr2y, vr2y_2, fy_1, fy_2);
  }
  else
  {
    const int mr1y = r1._y.isMapped() ? r1._y._value : mapy(r1._y._value);
    const int mr1y_2 = mr1y + (r1._height.isMapped() ? r1._height._value : rmapy(r1._height._value));
    
    const int mr2y = r2._y.isMapped() ? r2._y._value : mapy(r2._y._value);
    const int mr2y_2 = mr2y + (r2._height.isMapped() ? r2._height._value : rmapy(r2._height._value));
    
    fy_1 = mr1y > mr2y ? mr1y : mr2y;
    const int fy_2 = mr1y_2 < mr2y_2 ? mr1y_2 : mr2y_2;
    fh = fy_2 - fy_1;
    
// For testing...
//     fprintf(stderr, "View::intersected: ymag:%d mr1y:%d mr1y_2:%d mr2y:%d mr2y_2:%d fy_1:%d fy_2:%d\n",
//             ymag, mr1y, mr1y_2, mr2y, mr2y_2, fy_1, fy_2);
  }
  
  return ViewRect(ViewXCoordinate(fx_1, !xunmap),
                  ViewYCoordinate(fy_1, !yunmap),
                  ViewWCoordinate(fw,   !xunmap),
                  ViewHCoordinate(fh,   !yunmap));
}

// Returns the mapped intersection of the mapped rectangle and the unmapped rectangle.
QRect View::intersectedMap(const QRect& mapped_r, const QRect& unmapped_r) const
{
  int mfx_1, mfw, mfy_1, mfh;
  if (xmag <= 0)
  {
    const int vmx = mapxDev(mapped_r.x());
    const int vmx_2 = vmx + rmapxDev(mapped_r.width());
    const int vx = unmapped_r.x();
    const int vx_2 = vx + unmapped_r.width();
    const int vfx_1 = vmx > vx ? vmx : vx;
    const int vfx_2 = vmx_2 < vx_2 ? vmx_2 : vx_2;
    mfx_1 = mapx(vfx_1);
    mfw = rmapx(vfx_2 - vfx_1);
  }
  else
  {
    const int mx = mapped_r.x();
    const int mx_2 = mx + mapped_r.width();
    const int mvx = mapx(unmapped_r.x());
    const int mvx_2 = mvx + rmapx(unmapped_r.width());
    mfx_1 = mx > mvx ? mx : mvx;
    const int mfx_2 = mx_2 < mvx_2 ? mx_2 : mvx_2;
    mfw = mfx_2 - mfx_1;
  }
  
  if (ymag <= 0)
  {
    const int vmy = mapyDev(mapped_r.y());
    const int vmy_2 = vmy + rmapyDev(mapped_r.height());
    const int vy = unmapped_r.y();
    const int vy_2 = vy + unmapped_r.height();
    const int vfy_1 = vmy > vy ? vmy : vy;
    const int vfy_2 = vmy_2 < vy_2 ? vmy_2 : vy_2;
    mfy_1 = mapy(vfy_1);
    mfh = rmapy(vfy_2 - vfy_1);
  }
  else
  {
    const int my = mapped_r.y();
    const int my_2 = my + mapped_r.height();
    const int mvy = mapy(unmapped_r.y());
    const int mvy_2 = mvy + rmapy(unmapped_r.height());
    
    mfy_1 = my > mvy ? my : mvy;
    const int mfy_2 = my_2 < mvy_2 ? my_2 : mvy_2;
    mfh = mfy_2 - mfy_1;
  }
  
  return QRect(mfx_1, mfy_1, mfw, mfh);
}

// Returns the unmapped intersection of the mapped rectangle and the unmapped rectangle.
QRect View::intersectedUnmap(const QRect& mapped_r, const QRect& unmapped_r) const
{
  int vfx_1, vfw, vfy_1, vfh;
  if (xmag <= 0)
  {
    const int vmx = mapxDev(mapped_r.x());
    const int vmx_2 = vmx + rmapxDev(mapped_r.width());
    const int vx = unmapped_r.x();
    const int vx_2 = vx + unmapped_r.width();
    
    vfx_1 = vmx > vx ? vmx : vx;
    const int vfx_2 = vmx_2 < vx_2 ? vmx_2 : vx_2;
    vfw = vfx_2 - vfx_1;
  }
  else
  {
    const int mx = mapped_r.x();
    const int mx_2 = mx + mapped_r.width();
    const int mvx = mapx(unmapped_r.x());
    const int mvx_2 = mvx + rmapx(unmapped_r.width());
    const int mfx_1 = mx > mvx ? mx : mvx;
    const int mfx_2 = mx_2 < mvx_2 ? mx_2 : mvx_2;
    vfx_1 = mapxDev(mfx_1);
    vfw = rmapxDev(mfx_2 - mfx_1);
  }
  
  if (ymag <= 0)
  {
    const int vmy = mapyDev(mapped_r.y());
    const int vmy_2 = vmy + rmapyDev(mapped_r.height());
    const int vy = unmapped_r.y();
    const int vy_2 = vy + unmapped_r.height();

    vfy_1 = vmy > vy ? vmy : vy;
    const int vfy_2 = vmy_2 < vy_2 ? vmy_2 : vy_2;
    vfh = vfy_2 - vfy_1;
  }
  else
  {
    const int my = mapped_r.y();
    const int my_2 = my + mapped_r.height();
    const int mvy = mapy(unmapped_r.y());
    const int mvy_2 = mvy + rmapy(unmapped_r.height());
    
    const int mfy_1 = my > mvy ? my : mvy;
    const int mfy_2 = my_2 < mvy_2 ? my_2 : mvy_2;
    vfy_1 = mapyDev(mfy_1);
    vfh = rmapyDev(mfy_2 - mfy_1);
  }
  
  return QRect(vfx_1, vfy_1, vfw, vfh);
}

// Returns true if the mapped rectangle intersects the unmapped rectangle.
bool View::intersects(const QRect& mapped_r, const QRect& unmapped_r) const
{
  int vfx_1, vfx_2, vfy_1, vfy_2;
  if (xmag <= 0)
  {
    const int vmx = mapxDev(mapped_r.x());
    const int vmx_2 = vmx + rmapxDev(mapped_r.width());
    const int vx = unmapped_r.x();
    const int vx_2 = vx + unmapped_r.width();
    
    vfx_1 = vmx > vx ? vmx : vx;
    vfx_2 = vmx_2 < vx_2 ? vmx_2 : vx_2;
  }
  else
  {
    const int mx = mapped_r.x();
    const int mx_2 = mx + mapped_r.width();
    const int mvx = mapx(unmapped_r.x());
    const int mvx_2 = mvx + rmapx(unmapped_r.width());
    
    vfx_1 = mx > mvx ? mx : mvx;
    vfx_2 = mx_2 < mvx_2 ? mx_2 : mvx_2;
  }
  
  if (ymag <= 0)
  {
    const int vmy = mapyDev(mapped_r.y());
    const int vmy_2 = vmy + rmapyDev(mapped_r.height());
    const int vy = unmapped_r.y();
    const int vy_2 = vy + unmapped_r.height();

    vfy_1 = vmy > vy ? vmy : vy;
    vfy_2 = vmy_2 < vy_2 ? vmy_2 : vy_2;
  }
  else
  {
    const int my = mapped_r.y();
    const int my_2 = my + mapped_r.height();
    const int mvy = mapy(unmapped_r.y());
    const int mvy_2 = mvy + rmapy(unmapped_r.height());
    
    vfy_1 = my > mvy ? my : mvy;
    vfy_2 = my_2 < mvy_2 ? my_2 : mvy_2;
  }
  
  return vfx_1 < vfx_2 && vfy_1 < vfy_2;
}
      
// Returns true if the rectangles intersect.
bool View::intersects(const ViewRect& r1, const ViewRect& r2) const
{
  int fx_1, fx_2, fy_1, fy_2;
  if (xmag <= 0)
  {
    const int vr1x = r1._x.isMapped() ? mapxDev(r1._x._value) : r1._x._value;
    const int vr1x_2 = vr1x + (r1._width.isMapped() ? rmapxDev(r1._width._value) : r1._width._value);
    
    const int vr2x = r2._x.isMapped() ? mapxDev(r2._x._value) : r2._x._value;
    const int vr2x_2 = vr2x + (r2._width.isMapped() ? rmapxDev(r2._width._value) : r2._width._value);
    
    fx_1 = vr1x > vr2x ? vr1x : vr2x;
    fx_2 = vr1x_2 < vr2x_2 ? vr1x_2 : vr2x_2;
    
// For testing...
//     fprintf(stderr, "View::intersects: xmag:%d vr1x:%d vr1x_2:%d vr2x:%d vr2x_2:%d fx_1:%d fx_2:%d\n",
//             xmag, vr1x, vr1x_2, vr2x, vr2x_2, fx_1, fx_2);
  }
  else
  {
    const int mr1x = r1._x.isMapped() ? r1._x._value : mapx(r1._x._value);
    const int mr1x_2 = mr1x + (r1._width.isMapped() ? r1._width._value : rmapx(r1._width._value));
    
    const int mr2x = r2._x.isMapped() ? r2._x._value : mapx(r2._x._value);
    const int mr2x_2 = mr2x + (r2._width.isMapped() ? r2._width._value : rmapx(r2._width._value));
    
    fx_1 = mr1x > mr2x ? mr1x : mr2x;
    fx_2 = mr1x_2 < mr2x_2 ? mr1x_2 : mr2x_2;
    
// For testing...
//     fprintf(stderr, "View::intersects: xmag:%d mr1x:%d mr1x_2:%d mr2x:%d mr2x_2:%d fx_1:%d fx_2:%d\n",
//             xmag, mr1x, mr1x_2, mr2x, mr2x_2, fx_1, fx_2);
  }
  
  if (ymag <= 0)
  {
    const int vr1y = r1._y.isMapped() ? mapyDev(r1._y._value) : r1._y._value;
    const int vr1y_2 = vr1y + (r1._height.isMapped() ? rmapyDev(r1._height._value) : r1._height._value);
    
    const int vr2y = r2._y.isMapped() ? mapyDev(r2._y._value) : r2._y._value;
    const int vr2y_2 = vr2y + (r2._height.isMapped() ? rmapyDev(r2._height._value) : r2._height._value);
    
    fy_1 = vr1y > vr2y ? vr1y : vr2y;
    fy_2 = vr1y_2 < vr2y_2 ? vr1y_2 : vr2y_2;
    
// For testing...
//     fprintf(stderr, "View::intersects: ymag:%d vr1y:%d vr1y_2:%d vr2y:%d vr2y_2:%d fy_1:%d fy_2:%d\n",
//             ymag, vr1y, vr1y_2, vr2y, vr2y_2, fy_1, fy_2);
  }
  else
  {
    const int mr1y = r1._y.isMapped() ? r1._y._value : mapy(r1._y._value);
    const int mr1y_2 = mr1y + (r1._height.isMapped() ? r1._height._value : rmapy(r1._height._value));
    
    const int mr2y = r2._y.isMapped() ? r2._y._value : mapy(r2._y._value);
    const int mr2y_2 = mr2y + (r2._height.isMapped() ? r2._height._value : rmapy(r2._height._value));
    
    fy_1 = mr1y > mr2y ? mr1y : mr2y;
    fy_2 = mr1y_2 < mr2y_2 ? mr1y_2 : mr2y_2;
    
// For testing...
//     fprintf(stderr, "View::intersects: ymag:%d mr1y:%d mr1y_2:%d mr2y:%d mr2y_2:%d fy_1:%d fy_2:%d\n",
//             ymag, mr1y, mr1y_2, mr2y, mr2y_2, fy_1, fy_2);
  }
  
  return fx_1 < fx_2 && fy_1 < fy_2;
}

//--------------------------------------------
// Lossless comparisons for any magnification.
//--------------------------------------------

// Compares x1 and x2. x1 is the left operand and x2 is the right. For example x1 < x2.
bool View::compareXCoordinates(const ViewXCoordinate& x1, const ViewXCoordinate& x2, const CoordinateCompareMode& mode) const
{
  int xx1, xx2;
  if (xmag <= 0)
  {
    xx1 = x1.isMapped() ? mapxDev(x1._value) : x1._value;
    xx2 = x2.isMapped() ? mapxDev(x2._value) : x2._value;
  }
  else
  {
    xx1 = x1.isMapped() ? x1._value : mapx(x1._value);
    xx2 = x2.isMapped() ? x2._value : mapx(x2._value);
  }
  switch(mode)
  {
    case CompareLess:
      return xx1 < xx2;
    case CompareGreater:
      return xx1 > xx2;
    case CompareLessEqual:
      return xx1 <= xx2;
    case CompareGreaterEqual:
      return xx1 >= xx2;
    case CompareEqual:
      return xx1 == xx2;
  }
  return false;
}

// Compares y1 and y2. y1 is the left operand and y2 is the right. For example y1 < y2.
bool View::compareYCoordinates(const ViewYCoordinate& y1, const ViewYCoordinate& y2, const CoordinateCompareMode& mode) const
{
  int yy1, yy2;
  if (ymag <= 0)
  {
    yy1 = y1.isMapped() ? mapyDev(y1._value) : y1._value;
    yy2 = y2.isMapped() ? mapyDev(y2._value) : y2._value;
  }
  else
  {
    yy1 = y1.isMapped() ? y1._value : mapy(y1._value);
    yy2 = y2.isMapped() ? y2._value : mapy(y2._value);
  }
  switch(mode)
  {
    case CompareLess:
      return yy1 < yy2;
    case CompareGreater:
      return yy1 > yy2;
    case CompareLessEqual:
      return yy1 <= yy2;
    case CompareGreaterEqual:
      return yy1 >= yy2;
    case CompareEqual:
      return yy1 == yy2;
  }
  return false;
}

// Compares w1 and w2. w1 is the left operand and w2 is the right. For example w1 < w2.
bool View::compareWCoordinates(const ViewWCoordinate& w1, const ViewWCoordinate& w2, const CoordinateCompareMode& mode) const
{
  int ww1, ww2;
  if (xmag <= 0)
  {
    ww1 = w1.isMapped() ? rmapxDev(w1._value) : w1._value;
    ww2 = w2.isMapped() ? rmapxDev(w2._value) : w2._value;
  }
  else
  {
    ww1 = w1.isMapped() ? w1._value : rmapx(w1._value);
    ww2 = w2.isMapped() ? w2._value : rmapx(w2._value);
  }
  switch(mode)
  {
    case CompareLess:
      return ww1 < ww2;
    case CompareGreater:
      return ww1 > ww2;
    case CompareLessEqual:
      return ww1 <= ww2;
    case CompareGreaterEqual:
      return ww1 >= ww2;
    case CompareEqual:
      return ww1 == ww2;
  }
  return false;
}

// Compares h1 and h2. h1 is the left operand and h2 is the right. For example h1 < h2.
bool View::compareHCoordinates(const ViewHCoordinate& h1, const ViewHCoordinate& h2, const CoordinateCompareMode& mode) const
{
  int hh1, hh2;
  if (ymag <= 0)
  {
    hh1 = h1.isMapped() ? rmapyDev(h1._value) : h1._value;
    hh2 = h2.isMapped() ? rmapyDev(h2._value) : h2._value;
  }
  else
  {
    hh1 = h1.isMapped() ? h1._value : rmapy(h1._value);
    hh2 = h2.isMapped() ? h2._value : rmapy(h2._value);
  }
  switch(mode)
  {
    case CompareLess:
      return hh1 < hh2;
    case CompareGreater:
      return hh1 > hh2;
    case CompareLessEqual:
      return hh1 <= hh2;
    case CompareGreaterEqual:
      return hh1 >= hh2;
    case CompareEqual:
      return hh1 == hh2;
  }
  return false;
}

// Returns true if x is >= x1 and x < x2.
bool View::isXInRange(ViewXCoordinate x, ViewXCoordinate x1, ViewXCoordinate x2) const
{
  return compareXCoordinates(x, x1, CompareGreaterEqual) &&
         compareXCoordinates(x, x2, CompareLess);
}

// Returns true if y is >= y1 and y < y2.
bool View::isYInRange(ViewYCoordinate y, ViewYCoordinate y1, ViewYCoordinate y2) const
{
  return compareYCoordinates(y, y1, CompareGreaterEqual) &&
         compareYCoordinates(y, y2, CompareLess);
}

//--------------------------------------------
// Lossless math for any magnification.
//--------------------------------------------

// Returns a ViewXCoordinate of the math operation on x1 and x2. The result is mapped if the view's xmag > 0, and unmapped if not.
ViewXCoordinate View::mathXCoordinates(const ViewXCoordinate& x1, const ViewXCoordinate& x2,
                                       const CoordinateMathMode& mode) const
{
  if(xmag <= 0)
    return ViewXCoordinate(doCoordinateMath(asIntUnmapped(x1), asIntUnmapped(x2), mode), false);
  return ViewXCoordinate(doCoordinateMath(asIntMapped(x1), asIntMapped(x2), mode), true);
}

// Returns a ViewXCoordinate of the math operation on x1 and w1. The result is mapped if the view's xmag > 0, and unmapped if not.
ViewXCoordinate View::mathXCoordinates(const ViewXCoordinate& x1, const ViewWCoordinate& w1,
                                       const CoordinateMathMode& mode) const
{
  if(xmag <= 0)
    return ViewXCoordinate(doCoordinateMath(asIntUnmapped(x1), asIntUnmapped(w1), mode), false);
  return ViewXCoordinate(doCoordinateMath(asIntMapped(x1), asIntMapped(w1), mode), true);
}

// Returns a ViewWCoordinate of the math operation on w1 and w2. The result is mapped if the view's xmag > 0, and unmapped if not.
ViewWCoordinate View::mathXCoordinates(const ViewWCoordinate& w1, const ViewWCoordinate& w2,
                                       const CoordinateMathMode& mode) const
{
  if(xmag <= 0)
    return ViewWCoordinate(doCoordinateMath(asIntUnmapped(w1), asIntUnmapped(w2), mode), false);
  return ViewWCoordinate(doCoordinateMath(asIntMapped(w1), asIntMapped(w2), mode), true);
}



// Returns a ViewYCoordinate of the math operation on y1 and y2. The result is mapped if the view's ymag > 0, and unmapped if not.
ViewYCoordinate View::mathYCoordinates(const ViewYCoordinate& y1, const ViewYCoordinate& y2,
                                       const CoordinateMathMode& mode) const
{
  if(ymag <= 0)
    return ViewYCoordinate(doCoordinateMath(asIntUnmapped(y1), asIntUnmapped(y2), mode), false);
  return ViewYCoordinate(doCoordinateMath(asIntMapped(y1), asIntMapped(y2), mode), true);
}

// Returns a ViewYCoordinate of the math math operation on y1 and h1. The result is mapped if the view's ymag > 0, and unmapped if not.
ViewYCoordinate View::mathYCoordinates(const ViewYCoordinate& y1, const ViewHCoordinate& h1,
                                       const CoordinateMathMode& mode) const
{
  if(ymag <= 0)
    return ViewYCoordinate(doCoordinateMath(asIntUnmapped(y1), asIntUnmapped(h1), mode), false);
  return ViewYCoordinate(doCoordinateMath(asIntMapped(y1), asIntMapped(h1), mode), true);
}

// Returns a ViewHCoordinate of the math math operation on h1 and h2. The result is mapped if the view's ymag > 0, and unmapped if not.
ViewHCoordinate View::mathYCoordinates(const ViewHCoordinate& h1, const ViewHCoordinate& h2,
                                       const CoordinateMathMode& mode) const
{
  if(ymag <= 0)
    return ViewHCoordinate(doCoordinateMath(asIntUnmapped(h1), asIntUnmapped(h2), mode), false);
  return ViewHCoordinate(doCoordinateMath(asIntMapped(h1), asIntMapped(h2), mode), true);
}



// Performs math operation on x1. The result is mapped if the view's xmag > 0, and unmapped if not.
// Returns a reference to x1.
ViewXCoordinate& View::mathRefXCoordinates(ViewXCoordinate& x1, const ViewXCoordinate& x2,
                                  const CoordinateMathMode& mode) const
{
  if(xmag <= 0)
    x1 = ViewXCoordinate(doCoordinateMath(asIntUnmapped(x1), asIntUnmapped(x2), mode), false);
  else
    x1 = ViewXCoordinate(doCoordinateMath(asIntMapped(x1), asIntMapped(x2), mode), true);
  return x1;
}

// Performs math operation on x1. The result is mapped if the view's xmag > 0, and unmapped if not.
// Returns a reference to x1.
ViewXCoordinate& View::mathRefXCoordinates(ViewXCoordinate& x1, const ViewWCoordinate& w1,
                                  const CoordinateMathMode& mode) const
{
  if(xmag <= 0)
    x1 = ViewXCoordinate(doCoordinateMath(asIntUnmapped(x1), asIntUnmapped(w1), mode), false);
  else
    x1 = ViewXCoordinate(doCoordinateMath(asIntMapped(x1), asIntMapped(w1), mode), true);
  return x1;
}

// Performs math operation on w1. The result is mapped if the view's xmag > 0, and unmapped if not.
// Returns a reference to w1.
ViewWCoordinate& View::mathRefXCoordinates(ViewWCoordinate& w1, const ViewWCoordinate& w2,
                                  const CoordinateMathMode& mode) const
{
  if(xmag <= 0)
    w1 = ViewWCoordinate(doCoordinateMath(asIntUnmapped(w1), asIntUnmapped(w2), mode), false);
  else
    w1 = ViewWCoordinate(doCoordinateMath(asIntMapped(w1), asIntMapped(w2), mode), true);
  return w1;
}


// Performs math operation on y1. The result is mapped if the view's ymag > 0, and unmapped if not.
// Returns a reference to y1.
ViewYCoordinate& View::mathRefYCoordinates(ViewYCoordinate& y1, const ViewYCoordinate& y2,
                                  const CoordinateMathMode& mode) const
{
  if(ymag <= 0)
    y1 = ViewYCoordinate(doCoordinateMath(asIntUnmapped(y1), asIntUnmapped(y2), mode), false);
  else
    y1 = ViewYCoordinate(doCoordinateMath(asIntMapped(y1), asIntMapped(y2), mode), true);
  return y1;
}

// Performs math operation on y1. The result is mapped if the view's ymag > 0, and unmapped if not.
// Returns a reference to y1.
ViewYCoordinate& View::mathRefYCoordinates(ViewYCoordinate& y1, const ViewHCoordinate& h1,
                                  const CoordinateMathMode& mode) const
{
  if(ymag <= 0)
    y1 = ViewYCoordinate(doCoordinateMath(asIntUnmapped(y1), asIntUnmapped(h1), mode), false);
  else
    y1 = ViewYCoordinate(doCoordinateMath(asIntMapped(y1), asIntMapped(h1), mode), true);
  return y1;
}

// Performs math operation on h1. The result is mapped if the view's ymag > 0, and unmapped if not.
// Returns a reference to h1.
ViewHCoordinate& View::mathRefYCoordinates(ViewHCoordinate& h1, const ViewHCoordinate& h2,
                                  const CoordinateMathMode& mode) const
{
  if(ymag <= 0)
    h1 = ViewHCoordinate(doCoordinateMath(asIntUnmapped(h1), asIntUnmapped(h2), mode), false);
  else
    h1 = ViewHCoordinate(doCoordinateMath(asIntMapped(h1), asIntMapped(h2), mode), true);
  return h1;
}

// Adjusts the x, y, w and h of the rectangle. Returns a reference to the rectangle.
ViewRect& View::adjustRect(ViewRect& r, const ViewWCoordinate& dx, const ViewHCoordinate& dy,
                      const ViewWCoordinate& dw, const ViewHCoordinate& dh) const
{
    mathRefXCoordinates(r._x, dx, MathAdd);
    mathRefYCoordinates(r._y, dy, MathAdd);
    mathRefXCoordinates(r._width, dw, MathAdd);
    mathRefYCoordinates(r._height, dh, MathAdd);
  return r;
}

// Returns a rectangle with the x, y, w and h adjusted.
ViewRect View::adjustedRect(const ViewRect& r, const ViewWCoordinate& dx, const ViewHCoordinate& dy,
                      const ViewWCoordinate& dw, const ViewHCoordinate& dh) const
{
  return ViewRect(mathXCoordinates(r._x, dx, MathAdd),
                  mathYCoordinates(r._y, dy, MathAdd),
                  mathXCoordinates(r._width, dw, MathAdd),
                  mathYCoordinates(r._height, dh, MathAdd));
}

} // namespace MusEGui
