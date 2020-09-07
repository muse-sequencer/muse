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
#include <QRegion>
#include <QPixmap>
#include <QBrush>
#include <QRect>
#include <QPoint>

// NOTE: To cure circular dependencies, of which there are many, these are
//        forward referenced and the corresponding headers included further down here.
class QDropEvent;
class QKeyEvent;
class QMouseEvent;
class QPaintEvent;
class QPainter;
class QResizeEvent;

namespace MusEGui {

//---------------------
//  ViewCoordinate
//---------------------

class ViewCoordinate
{
  public:
    enum CoordinateFlags { NoFlags = 0x0, IsVertical = 0x01, IsMapped = 0x02, IsRelative = 0x04, IsMappedAndRelative = IsMapped | IsRelative };
    typedef int CoordinateFlags_t;
    
    int _value;
    CoordinateFlags_t _flags;
    
    ViewCoordinate(int value = 0, CoordinateFlags_t flags = NoFlags) : _value(value), _flags(flags) { } 
    
    bool isMapped() const { return _flags & IsMapped; }
    void setMapped(bool m) { m ? _flags |= IsMapped : _flags &= ~IsMapped; }
    bool isRelative() const { return _flags & IsRelative; }
    bool isVertical() const { return _flags & IsVertical; }
};
  
class ViewXCoordinate : public ViewCoordinate
{
  public:
    ViewXCoordinate(int value = 0, bool is_mapped = false) : 
      ViewCoordinate(value, NoFlags | (is_mapped ? IsMapped : NoFlags)) { } 
};
  
class ViewYCoordinate : public ViewCoordinate
{
  public:
    ViewYCoordinate(int value = 0, bool is_mapped = false) : 
      ViewCoordinate(value, IsVertical | (is_mapped ? IsMapped : NoFlags)) { } 
};

class ViewWCoordinate : public ViewXCoordinate
{
  public:
    ViewWCoordinate(int value = 0, bool is_mapped = false) : 
      ViewXCoordinate(value, is_mapped) { _flags |= IsRelative; } 
};
  
class ViewHCoordinate : public ViewYCoordinate
{
  public:
    ViewHCoordinate(int value = 0, bool is_mapped = false) : 
      ViewYCoordinate(value, is_mapped) { _flags |= IsRelative; } 
};


//---------------------
//  ViewRect
//---------------------

class ViewRect
{
  public:
    ViewXCoordinate _x;
    ViewYCoordinate _y;
    ViewWCoordinate _width;
    ViewHCoordinate _height;
    
    ViewRect() { }
    
    ViewRect(const ViewXCoordinate& x, const ViewYCoordinate& y,
             const ViewWCoordinate& width, const ViewHCoordinate& height) :
      _x(x), _y(y), _width(width), _height(height) { }
      
    ViewRect(const QRect& r, bool is_mapped) :
      _x(ViewXCoordinate(r.x(), is_mapped)),
      _y(ViewYCoordinate(r.y(), is_mapped)),
      _width(ViewWCoordinate(r.width(), is_mapped)),
      _height(ViewHCoordinate(r.height(), is_mapped)) { }
      
    void dump(const char* header = 0) const;
};


//---------------------------------------------------------
//   View
//    horizontal View with double buffering
//---------------------------------------------------------

class View : public QWidget {
    Q_OBJECT

   public:   
      // CAUTION: Since one or both of the integer operands might get (un)mapped in an expanded way (multiplied), 
      //           an exact equality test might never be true! Try to avoid equality tests when in doubt, use the others.
      enum CoordinateCompareMode { CompareLess, CompareGreater, CompareLessEqual, CompareGreaterEqual, CompareEqual };
      enum CoordinateMathMode { MathAdd, MathSubtract, MathMultiply, MathDivide, MathModulo };

   private:      
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
      
      struct ScaleRetStruct
      {
        bool _drawBar;
        bool _isSmall;
      };
      ScaleRetStruct scale(bool drawText, int bar, double tpix) const;
      //void drawBarText(QPainter& p, int tick, int bar, const QRect& vr, const QColor& textColor, const QFont& font) const;
      void drawBarText(QPainter& p, int tick, int bar, const QRect& mr, const QColor& textColor, const QFont& font) const;


      virtual void keyPressEvent(QKeyEvent* event);
      virtual void keyReleaseEvent(QKeyEvent* event);
      virtual void mousePressEvent(QMouseEvent* event);
      virtual void mouseDoubleClickEvent(QMouseEvent* event);
      virtual void mouseMoveEvent(QMouseEvent* event);
      virtual void mouseReleaseEvent(QMouseEvent* event);
      virtual void dropEvent(QDropEvent* event);

      virtual void draw(QPainter&, const QRect&, const QRegion& = QRegion()) {}
      virtual void drawOverlay(QPainter&, const QRect&, const QRegion& = QRegion()) {}
      virtual QRect overlayRect() const { return QRect(0, 0, 0, 0); }
      virtual void drawTickRaster(QPainter& p, const QRect&, const QRegion& = QRegion(), int raster = 0,
                                      bool waveMode = false,
                                      bool useGivenColors = false,
                                      bool drawText = false,
                                      const QColor& bar_color = Qt::cyan, // default to very obvious color to visualize uninitialized drawing
                                      const QColor& beat_color = Qt::cyan,
                                      const QColor& fine_color = Qt::cyan,
                                      const QColor& coarse_color = Qt::cyan,
                                      const QColor& text_color = Qt::cyan,
                                      const QFont& large_font = QFont(),
                                      const QFont& small_font = QFont()
                                     );

      virtual void pdraw(QPainter&, const QRect&, const QRegion& = QRegion());

      virtual void paintEvent(QPaintEvent* ev);
      void redraw(const QRect&);
      void redraw(const QRegion&);

      void paint(const QRect&, const QRegion& = QRegion());

      virtual void resizeEvent(QResizeEvent*);
      virtual void viewKeyPressEvent(QKeyEvent*);
      virtual void viewKeyReleaseEvent(QKeyEvent*);
      virtual void viewMousePressEvent(QMouseEvent*) {}
      virtual void viewMouseDoubleClickEvent(QMouseEvent*) {}
      virtual void viewMouseMoveEvent(QMouseEvent*) {}
      virtual void viewMouseReleaseEvent(QMouseEvent*) {}
      virtual void viewDropEvent(QDropEvent*) {}

      QRect map(const QRect&) const;
      QPoint map(const QPoint&) const;
      void map(const QRegion& rg_in, QRegion& rg_out) const;
      QRect mapDev(const QRect&) const;
      QPoint mapDev(const QPoint&) const;
      void mapDev(const QRegion& rg_in, QRegion& rg_out) const;

      int mapx(int x) const;
      int mapy(int y) const;
      int mapyDev(int y) const;
      int mapxDev(int x) const;
      int rmapy(int y, bool round = false) const;
      int rmapyDev(int y, bool round = false) const;
      QPoint rmapDev(const QPoint&, bool round = false) const;
      QRect devToVirt(const QRect&) const;
      void devToVirt(const QRegion& rg_in, QRegion& rg_out) const;
      double rmapx_f(double x) const;
      double rmapy_f(double y) const;
      double rmapxDev_f(double x) const;
      double rmapyDev_f(double y) const;

      void setPainter(QPainter& p);

      inline int doCoordinateMath(const int& v1, const int& v2, const CoordinateMathMode& mode) const
      {
        switch(mode) {
          case MathAdd: return v1 + v2; break;
          case MathSubtract: return v1 - v2; break;
          case MathMultiply: return v1 * v2; break;
          case MathDivide: return v1 / v2; break;
          case MathModulo: return v1 % v2; break;
          }
        return 0;
      }  

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
      QRect rmap(const QRect&) const;
      int rmapxDev(int x, bool round = false) const;
      int rmapx(int x, bool round = false) const;

      
      //--------------------------------------------------
      // Lossless intersections (in x and/or y, depending)
      //  for any magnification.
      //--------------------------------------------------

      // Returns true if the mapped rectangle intersects the unmapped rectangle.
      bool intersects(const QRect& mapped_r, const QRect& unmapped_r) const;
      // Returns true if the rectangles intersect.
      bool intersects(const ViewRect& r1, const ViewRect& r2) const;
      // Returns the mapped intersection of the mapped rectangle and the unmapped rectangle.
      QRect intersectedMap(const QRect& mapped_r, const QRect& unmapped_r) const;
      // Returns the unmapped intersection of the mapped rectangle and the unmapped rectangle.
      QRect intersectedUnmap(const QRect& mapped_r, const QRect& unmapped_r) const;
      // Returns the intersection of the rectangles. The coordinate x, y, and w, h are mapped
      //  if the view's xmag > 0, and unmapped if not. The coordinate w, h are mapped if the
      //  view's ymag > 0, and unmapped if not.
      ViewRect intersected(const ViewRect& r1, const ViewRect& r2) const;

      //--------------------------------------------
      // Lossless math for any magnification.
      //--------------------------------------------

      // Returns a ViewXCoordinate of the math operation on x1 and x2. The result is mapped if the view's xmag > 0, and unmapped if not.
      ViewXCoordinate mathXCoordinates(const ViewXCoordinate& x1, const ViewXCoordinate& x2,
                                       const CoordinateMathMode& mode = MathAdd) const;
      // Returns a ViewXCoordinate of the math operation on x1 and w1. The result is mapped if the view's xmag > 0, and unmapped if not.
      ViewXCoordinate mathXCoordinates(const ViewXCoordinate& x1, const ViewWCoordinate& w1,
                                       const CoordinateMathMode& mode) const;
      // Returns a ViewWCoordinate of the math operation on w1 and w2. The result is mapped if the view's xmag > 0, and unmapped if not.
      ViewWCoordinate mathXCoordinates(const ViewWCoordinate& w1, const ViewWCoordinate& w2,
                                       const CoordinateMathMode& mode) const;
                                       
      // Returns a ViewYCoordinate of the math operation on y1 and y2. The result is mapped if the view's ymag > 0, and unmapped if not.
      ViewYCoordinate mathYCoordinates(const ViewYCoordinate& y1, const ViewYCoordinate& y2,
                                       const CoordinateMathMode& mode = MathAdd) const;
      // Returns math operation on y1 and h1. The result is mapped if the view's ymag > 0, and unmapped if not.
      ViewYCoordinate mathYCoordinates(const ViewYCoordinate& y1, const ViewHCoordinate& h1,
                                            const CoordinateMathMode& mode) const;
      // Returns a ViewHCoordinate of the math math operation on h1 and h2. The result is mapped if the view's ymag > 0, and unmapped if not.
      ViewHCoordinate mathYCoordinates(const ViewHCoordinate& h1, const ViewHCoordinate& h2,
                                            const CoordinateMathMode& mode) const;
                                            
//       // Returns a ViewWCoordinate of the math operation on x1 and x2. The result is mapped if the view's xmag > 0, and unmapped if not.
//       ViewWCoordinate mathWCoordinates(const ViewXCoordinate& x1, const ViewXCoordinate& x2,
//                                        const CoordinateMathMode& mode = MathAdd) const;
//       // Returns a ViewHCoordinate of the math operation on y1 and y2. The result is mapped if the view's ymag > 0, and unmapped if not.
//       ViewHCoordinate mathHCoordinates(const ViewYCoordinate& y1, const ViewYCoordinate& y2,
//                                        const CoordinateMathMode& mode = MathAdd) const;

      // Performs math operation on x1. The result is mapped if the view's xmag > 0, and unmapped if not.
      // Returns a reference to x1.
      ViewXCoordinate& mathRefXCoordinates(ViewXCoordinate& x1, const ViewXCoordinate& x2,
                                        const CoordinateMathMode& mode = MathAdd) const;
      // Performs math operation on x1. The result is mapped if the view's xmag > 0, and unmapped if not.
      // Returns a reference to x1.
      ViewXCoordinate& mathRefXCoordinates(ViewXCoordinate& x1, const ViewWCoordinate& w1,
                                        const CoordinateMathMode& mode) const;
      // Performs math operation on w1. The result is mapped if the view's xmag > 0, and unmapped if not.
      // Returns a reference to x1.
      ViewWCoordinate& mathRefXCoordinates(ViewWCoordinate& w1, const ViewWCoordinate& w2,
                                        const CoordinateMathMode& mode) const;
      // Performs math operation on y1. The result is mapped if the view's ymag > 0, and unmapped if not.
      // Returns a reference to y1.
      ViewYCoordinate& mathRefYCoordinates(ViewYCoordinate& y1, const ViewYCoordinate& y2,
                                        const CoordinateMathMode& mode = MathAdd) const;
      // Performs math operation on y1. The result is mapped if the view's ymag > 0, and unmapped if not.
      // Returns a reference to y1.
      ViewYCoordinate& mathRefYCoordinates(ViewYCoordinate& y1, const ViewHCoordinate& h1,
                                        const CoordinateMathMode& mode) const;
      // Performs math operation on h1. The result is mapped if the view's ymag > 0, and unmapped if not.
      // Returns a reference to h1.
      ViewHCoordinate& mathRefYCoordinates(ViewHCoordinate& h1, const ViewHCoordinate& h2,
                                        const CoordinateMathMode& mode) const;
//       // Performs math operation on w1. The result is relative mapped if the view's xmag > 0, and relative unmapped if not.
//       // Returns a reference to w1.
//       ViewWCoordinate& mathWCoordinates(ViewWCoordinate& w1, const ViewXCoordinate& x2,
//                                         const CoordinateMathMode& mode = MathAdd) const;
//       // Performs math operation on h1. The result is relative mapped if the view's ymag > 0, and relative unmapped if not.
//       // Returns a reference to h1.
//       ViewHCoordinate& mathHCoordinates(ViewHCoordinate& h1, const ViewYCoordinate& y2,
//                                         const CoordinateMathMode& mode = MathAdd) const;

      // Adjusts the x, y, w and h of the rectangle. Returns a reference to the rectangle.
      ViewRect& adjustRect(ViewRect& r, const ViewWCoordinate& dx, const ViewHCoordinate& dy,
                           const ViewWCoordinate& dw, const ViewHCoordinate& dh) const;
      // Returns a rectangle with the x, y, w and h adjusted.
      ViewRect adjustedRect(const ViewRect& r, const ViewWCoordinate& dx, const ViewHCoordinate& dy,
                           const ViewWCoordinate& dw, const ViewHCoordinate& dh) const;
      
      //--------------------------------------------
      // Lossless comparisons for any magnification.
      //--------------------------------------------

      // Compares x1 and x2. x1 is the left operand and x2 is the right. For example x1 < x2.
      bool compareXCoordinates(const ViewXCoordinate& x1, const ViewXCoordinate& x2, const CoordinateCompareMode& mode = CompareEqual) const;
      // Compares y1 and y2. y1 is the left operand and y2 is the right. For example y1 < y2.
      bool compareYCoordinates(const ViewYCoordinate& y1, const ViewYCoordinate& y2, const CoordinateCompareMode& mode = CompareEqual) const;
      // Compares w1 and w2. w1 is the left operand and w2 is the right. For example w1 < w2.
      bool compareWCoordinates(const ViewWCoordinate& w1, const ViewWCoordinate& w2, const CoordinateCompareMode& mode = CompareEqual) const;
      // Compares h1 and h2. h1 is the left operand and h2 is the right. For example h1 < h2.
      bool compareHCoordinates(const ViewHCoordinate& h1, const ViewHCoordinate& h2, const CoordinateCompareMode& mode = CompareEqual) const;
      // Returns true if x is >= x1 and x < x2.
      bool isXInRange(ViewXCoordinate x, ViewXCoordinate x1, ViewXCoordinate x2) const;
      // Returns true if y is >= y1 and y < y2.
      bool isYInRange(ViewYCoordinate y, ViewYCoordinate y1, ViewYCoordinate y2) const;
      
      //--------------------------------------------
      // Conversions, utilities.
      //--------------------------------------------
      
      bool isViewRectEmpty(const ViewRect& r) const { return r._width._value <= 0 || r._height._value <= 0; };
      
      // Returns a mapped version of the x coordinate.
      inline ViewXCoordinate asMapped(const ViewXCoordinate& x) const
      { return ViewXCoordinate(x.isMapped() ? x._value : mapx(x._value), true); }
      inline int asIntMapped(const ViewXCoordinate& x) const
      { return x.isMapped() ? x._value : mapx(x._value); }

      // Returns an unmapped version of the y coordinate.
      inline ViewYCoordinate asMapped(const ViewYCoordinate& y) const
      { return ViewYCoordinate(y.isMapped() ? y._value : mapy(y._value), true); }
      inline int asIntMapped(const ViewYCoordinate& y) const
      { return y.isMapped() ? y._value : mapy(y._value); }
            
      // Returns a relative mapped version of the w coordinate.
      inline ViewWCoordinate asMapped(const ViewWCoordinate& w) const
      { return ViewWCoordinate(w.isMapped() ? w._value : rmapx(w._value, true), true); }
      inline int asIntMapped(const ViewWCoordinate& w) const
      { return w.isMapped() ? w._value : rmapx(w._value, true); }

      // Returns a relative mapped version of the h coordinate.
      inline ViewHCoordinate asMapped(const ViewHCoordinate& h) const
      { return ViewHCoordinate(h.isMapped() ? h._value : rmapy(h._value, true), true); }
      inline int asIntMapped(const ViewHCoordinate& h) const
      { return h.isMapped() ? h._value : rmapy(h._value, true); }

      
      // Returns an unmapped version of the x coordinate.
      inline ViewXCoordinate asUnmapped(const ViewXCoordinate& x) const
      { return ViewXCoordinate(x.isMapped() ? mapxDev(x._value) : x._value, false); }
      inline int asIntUnmapped(const ViewXCoordinate& x) const
      { return x.isMapped() ? mapxDev(x._value) : x._value; }

      // Returns an unmapped version of the y coordinate.
      inline ViewYCoordinate asUnmapped(const ViewYCoordinate& y) const
      { return ViewYCoordinate(y.isMapped() ? mapyDev(y._value) : y._value, false); }
      inline int asIntUnmapped(const ViewYCoordinate& y) const
      { return y.isMapped() ? mapyDev(y._value) : y._value; }
            
      // Returns a relative unmapped version of the w coordinate.
      inline ViewWCoordinate asUnmapped(const ViewWCoordinate& w) const
      { return ViewWCoordinate(w.isMapped() ? rmapxDev(w._value, true) : w._value, false); }
      inline int asIntUnmapped(const ViewWCoordinate& w) const
      { return w.isMapped() ? rmapxDev(w._value, true) : w._value; }

      // Returns a relative unmapped version of the h coordinate.
      inline ViewHCoordinate asUnmapped(const ViewHCoordinate& h) const
      { return ViewHCoordinate(h.isMapped() ? rmapyDev(h._value, true) : h._value, false); }
      inline int asIntUnmapped(const ViewHCoordinate& h) const
      { return h.isMapped() ? rmapyDev(h._value, true) : h._value; }
            
      // Returns a mapped version of the rectangle.
      inline ViewRect asMapped(const ViewRect& r) const
      {
        return ViewRect(ViewXCoordinate(r._x.isMapped() ? r._x._value : mapx(r._x._value), true),
                        ViewYCoordinate(r._y.isMapped() ? r._y._value : mapy(r._y._value), true),
                        ViewWCoordinate(r._width.isMapped() ? r._width._value : rmapx(r._width._value, true), true),
                        ViewHCoordinate(r._height.isMapped() ? r._height._value : rmapy(r._height._value, true), true));
      }

      // Returns an unmapped version of the rectangle.
      inline ViewRect asUnmapped(const ViewRect& r) const
      {
        return ViewRect(ViewXCoordinate(r._x.isMapped() ? mapxDev(r._x._value) : r._x._value, false),
                        ViewYCoordinate(r._y.isMapped() ? mapyDev(r._y._value) : r._y._value, false),
                        ViewWCoordinate(r._width.isMapped() ? rmapxDev(r._width._value, true) : r._width._value, false),
                        ViewHCoordinate(r._height.isMapped() ? rmapyDev(r._height._value, true) : r._height._value, false));
      }
            
      // Returns a mapped QRect version of the rectangle.
      inline QRect asQRectMapped(const ViewRect& r) const
      {
        return QRect(r._x.isMapped() ? r._x._value : mapx(r._x._value),
                    r._y.isMapped() ? r._y._value : mapy(r._y._value),
                    r._width.isMapped() ? r._width._value : rmapx(r._width._value, true),
                    r._height.isMapped() ? r._height._value : rmapy(r._height._value, true));
      }

      // Returns an unmapped QRect version of the rectangle.
      inline QRect asQRectUnmapped(const ViewRect& r) const
      {
        return QRect(r._x.isMapped() ? mapxDev(r._x._value) : r._x._value,
                    r._y.isMapped() ? mapyDev(r._y._value) : r._y._value,
                    r._width.isMapped() ? rmapxDev(r._width._value, true) : r._width._value,
                    r._height.isMapped() ? rmapyDev(r._height._value, true) : r._height._value);
      }

      
      // Returns the x + w - (mapped 1) coordinate.
      inline ViewXCoordinate rectRightMapped(const ViewRect& r) const
      { return mathXCoordinates(mathXCoordinates(r._x, r._width, MathAdd), ViewWCoordinate(1, true), MathSubtract); }

      // Returns the y + h - (mapped 1) coordinate.
      inline ViewYCoordinate rectBottomMapped(const ViewRect& r) const
      { return mathYCoordinates(mathYCoordinates(r._y, r._height, MathAdd), ViewHCoordinate(1, true), MathSubtract); }

      // Returns the x + w - (unmapped 1) coordinate.
      inline ViewXCoordinate rectRightUnmapped(const ViewRect& r) const
      { return mathXCoordinates(mathXCoordinates(r._x, r._width, MathAdd), ViewWCoordinate(1, false), MathSubtract); }

      // Returns the y + h - (unmapped 1) coordinate.
      inline ViewYCoordinate rectBottomUnmapped(const ViewRect& r) const
      { return mathYCoordinates(mathYCoordinates(r._y, r._height, MathAdd), ViewHCoordinate(1, false), MathSubtract); }

      
      };

} // namespace MusEGui

#endif

