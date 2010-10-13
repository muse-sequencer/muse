//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: view.cpp,v 1.3.2.2 2009/04/06 01:24:55 terminator356 Exp $
//  (C) Copyright 1999 Werner Schweer (ws@seh.de)
//=========================================================

#include "view.h"
#include <cmath>
#include <stdio.h>
#include <QPainter>
//Added by qt3to4:
#include <QPixmap>
#include <QResizeEvent>
#include <QDropEvent>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QPaintEvent>

//---------------------------------------------------------
//   View::View
//    double xMag = (xmag < 0) ? 1.0/-xmag : double(xmag)
//---------------------------------------------------------

View::View(QWidget* w, int xm, int ym, const char* name)
   : QWidget(w, name, Qt::WNoAutoErase | Qt::WResizeNoErase)
      {
      xmag  = xm;
      ymag  = ym;
      xpos  = 0;
      ypos  = 0;
      xorg  = 0;
      yorg  = 0;
      _virt = true;
      setBackgroundMode(Qt::NoBackground);
      brush.setStyle(Qt::SolidPattern);
      brush.setColor(Qt::lightGray);
      pmValid = false;
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
          bitBlt(&pm,  0, 0, &pm,  -delta, 0, w + delta, h, true); //CopyROP, true); // ddskrjo
            r = QRect(w + delta, 0, -delta, h);
            }
      else {                  // shift right
            bitBlt(&pm,  delta, 0, &pm,     0, 0, w-delta, h, true); //CopyROP, true); // ddskrjo
            r = QRect(0, 0, delta, h);
            }
      QRect olr = overlayRect();
      QRect olr1(olr);
      olr1.moveBy(delta, 0);

      r |= olr;
      r |= olr1;
      
      //printf("View::setXPos x:%d w:%d delta:%d r.x:%d r.w:%d\n", x, w, delta, r.x(), r.width());
      
      paint(r);
      update();
      }

//---------------------------------------------------------
//   setYPos
//---------------------------------------------------------

void View::setYPos(int y)
      {
      int delta  = ypos - y;         // -  -> shift up
      ypos  = y;
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
            bitBlt(&pm,  0, 0, &pm, 0, -delta, w, h + delta, true); //CopyROP, true); ddskrjo
            r = QRect(0, h + delta, w, -delta);
            }
      else {                  // shift down
            bitBlt(&pm,  0, delta, &pm, 0, 0, w, h-delta, true); // CopyROP, true); ddskrjo
            
            // NOTE: June 2 2010: On my machine with an old NV V8200 + prop drivers (curr 96.43.11),
            //  this is a problem. There is severe graphical corruption in some of the view-based windows.
            // Not just here but several other windows (ex. ladspa browser). 
            // I believe (?) I saw other QT3 apps exhibit this problem, too. QT4 apps don't do it. 
            // Neither does it happen when xorg drivers used. 
            //
            // However, there is one type of MusE corruption which ALL drivers seem to show, and that is
            //  the arranger 'grey' non-part-based tracks (Input, Output, Group etc.).
            // It is also observed on another machine with an ATI card and a different linux distro.
            // This change also fixes that problem, although the fact that xorg drivers show the problem
            //  had long made me believe that it was our drawing technique, not particularly this line.
            // Meaning that perhaps this line is not the right way to fix that problem.
            //
            // On the other hand the two problems may be related, and only one shows with xorg drivers... 
            // Ultimately it could just be my NV card, as a request for similar experience in mail list
            //  returned all negative.
            //
            // FIXME: This change cures it for me, but we shouldn't leave this in - shouldn't need to do this...
            //
            //r = QRect(0, 0, w, delta);
            // Changed p3.3.43
            r = QRect(0, 0, w, h);
            
            }
      QRect olr = overlayRect();
      QRect olr1(olr);
      olr1.moveBy(0, delta);

      r |= olr;
      r |= olr1;
      paint(r);
      update();
      }

//---------------------------------------------------------
//   resizeEvent
//---------------------------------------------------------

void View::resizeEvent(QResizeEvent* ev)
      {
      pm.resize(ev->size());
      pmValid = false;
      }

//---------------------------------------------------------
//   paintEvent
//---------------------------------------------------------

void View::paintEvent(QPaintEvent* ev)
      {
      //printf("View::paintEvent pmValid:%d x:%d width:%d y:%d height:%d\n", pmValid, ev->rect().x(), ev->rect().width(), ev->rect().y(), ev->rect().height());
      if (!pmValid)
            paint(ev->rect());
      bitBlt(this, ev->rect().topLeft(), &pm, ev->rect(), true); // CopyROP, true); ddskrjo
      }

//---------------------------------------------------------
//   redraw
//---------------------------------------------------------

void View::redraw()
      {
      QRect r(0, 0, pm.width(), pm.height());
      //printf("View::redraw() r.x:%d r.w:%d\n", r.x(), r.width());
      paint(r);
      update();
      }

//---------------------------------------------------------
//   redraw
//---------------------------------------------------------

void View::redraw(const QRect& r)
      {
      //printf("View::redraw(QRect& r) r.x:%d r.w:%d\n", r.x(), r.width());
      paint(r);
      update(r);
      }

//---------------------------------------------------------
//   paint
//    r - phys coord system
//---------------------------------------------------------

void View::paint(const QRect& r)
      {
      if (pm.isNull())
            return;
      QRect rr(r);
      if (!pmValid) {
            pmValid = true;
            rr = QRect(0, 0, pm.width(), pm.height());
            }
      QPainter p(&pm);
      if (bgPixmap.isNull())
            p.fillRect(rr, brush);
      else
            p.drawTiledPixmap(rr, bgPixmap, QPoint(xpos + rmapx(xorg)
               + rr.x(), ypos + rmapy(yorg) + rr.y()));
      p.setClipRegion(rr);
      //printf("View::paint r.x:%d w:%d\n", rr.x(), rr.width());
      pdraw(p, rr);       // draw into pixmap

      p.resetXForm();
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
         ev->globalPos(), ev->button(), ev->state());
      viewMousePressEvent(&e);
      }

//---------------------------------------------------------
//   mouseDoubleClickEvent
//---------------------------------------------------------

void View::mouseDoubleClickEvent(QMouseEvent* ev)
      {
      QMouseEvent e(ev->type(), mapDev(ev->pos()),
         ev->globalPos(), ev->button(), ev->state());
      viewMouseDoubleClickEvent(&e);
      }

//---------------------------------------------------------
//   mouseMoveEvent
//---------------------------------------------------------

void View::mouseMoveEvent(QMouseEvent* ev)
      {
      QMouseEvent e(ev->type(), mapDev(ev->pos()),
         ev->globalPos(), ev->button(), ev->state());
      viewMouseMoveEvent(&e);
      }

//---------------------------------------------------------
//   mouseReleaseEvent
//---------------------------------------------------------

void View::mouseReleaseEvent(QMouseEvent* ev)
      {
      QMouseEvent e(ev->type(), mapDev(ev->pos()),
         ev->globalPos(), ev->button(), ev->state());
      viewMouseReleaseEvent(&e);
      }

//---------------------------------------------------------
//   dropEvent
//---------------------------------------------------------

void View::dropEvent(QDropEvent* ev)
      {
      ev->setPoint(mapDev(ev->pos()));
      viewDropEvent(ev);
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
      if (virt()) {
            setPainter(p);
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
      p.resetXForm();
      p.translate(double(-(xpos+rmapx(xorg))), double(-(ypos+rmapy(yorg))));
      double xMag = (xmag < 0) ? 1.0/(-xmag) : double(xmag);
      double yMag = (ymag < 0) ? 1.0/(-ymag) : double(ymag);
      p.scale(xMag, yMag);
      }

//---------------------------------------------------------
//   map
//---------------------------------------------------------

QRect View::map(const QRect& r) const
      {
      int x, y, w, h;
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

QRect View::mapDev(const QRect& r) const
      {
      return QRect(mapxDev(r.x()), mapyDev(r.y()),
         rmapxDev(r.width()), rmapyDev(r.height()));
      }

QPoint View::mapDev(const QPoint& r) const
      {
      return QPoint(mapxDev(r.x()), mapyDev(r.y()));
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

