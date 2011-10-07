//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: view.h,v 1.2.2.1 2008/01/26 07:23:21 terminator356 Exp $
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

#ifndef __VIEW_H__
#define __VIEW_H__

#include <QWidget>

class QDropEvent;
class QKeyEvent;
class QMouseEvent;
class QPaintEvent;
class QPainter;
class QPixmap;
class QResizeEvent;

namespace MusEGui {

//---------------------------------------------------------
//   View
//    horizontal View with double buffering
//---------------------------------------------------------

class View : public QWidget {
    Q_OBJECT
    
      QPixmap pm;             // for double buffering
      bool pmValid;
      QPixmap bgPixmap;       // background Pixmap
      QBrush brush;
      bool _virt;
      

   protected:
      int xorg;
      int yorg;
      int xpos, ypos;
      int xmag, ymag;

      virtual void keyPressEvent(QKeyEvent* event);
      virtual void mousePressEvent(QMouseEvent* event);
      virtual void mouseDoubleClickEvent(QMouseEvent* event);
      virtual void mouseMoveEvent(QMouseEvent* event);
      virtual void mouseReleaseEvent(QMouseEvent* event);
      virtual void dropEvent(QDropEvent* event);

      virtual void draw(QPainter&, const QRect&) {}
      virtual void drawOverlay(QPainter&) {}
      virtual QRect overlayRect() const { return QRect(0, 0, 0, 0); }
      virtual void drawTickRaster(QPainter& p, int x, int y, int w, int h, int raster);

      virtual void pdraw(QPainter&, const QRect&);

      virtual void paintEvent(QPaintEvent* ev);
      void redraw(const QRect&);

      void paint(const QRect& r);

      virtual void resizeEvent(QResizeEvent*);
      virtual void viewKeyPressEvent(QKeyEvent*);
      virtual void viewMousePressEvent(QMouseEvent*) {}
      virtual void viewMouseDoubleClickEvent(QMouseEvent*) {}
      virtual void viewMouseMoveEvent(QMouseEvent*) {}
      virtual void viewMouseReleaseEvent(QMouseEvent*) {}
      virtual void viewDropEvent(QDropEvent*) {}

      QRect map(const QRect&) const;
      QPoint map(const QPoint&) const;
      QRect mapDev(const QRect&) const;
      QPoint mapDev(const QPoint&) const;

      int mapx(int x) const;
      int mapy(int y) const;
      int mapyDev(int y) const;
      int mapxDev(int x) const;
      int rmapy(int y) const;
      int rmapyDev(int y) const;
      //QRect devToVirt(const QRect&);
      double rmapx_f(double x) const;
      double rmapy_f(double y) const;
      double rmapxDev_f(double x) const;
      double rmapyDev_f(double y) const;

      void setPainter(QPainter& p);

   public slots:
      void setXPos(int);
      void setYPos(int);
      void setXMag(int xs);
      void setYMag(int ys);
      void redraw();

   public:
      View(QWidget*, int, int, const char* name = 0);
      void setBg(const QPixmap& pm);
      void setBg(const QColor& color) { brush.setColor(color); redraw(); }
      void setXOffset(int v)   { setXPos(mapx(v)); }
      int xOffset() const      { return mapxDev(xpos)-xorg; }
      int xOffsetDev() const   { return xpos-rmapx(xorg); }
      
      int yOffset() const      { return mapyDev(ypos)-yorg; }
      int getXScale() const    { return xmag; }
      int getYScale() const    { return ymag; }
      void setOrigin(int x, int y);
      void setVirt(bool flag)  { _virt = flag; }
      bool virt() const        { return _virt; }
      int rmapxDev(int x) const;
      int rmapx(int x) const;
      };

} // namespace MusEGui

#endif

