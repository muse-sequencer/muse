//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: slider.h,v 1.3.2.2 2008/01/19 13:33:47 wschweer Exp $
//
//    Copyright (C) 1997  Josef Wilgen
//    (C) Copyright 1999 Werner Schweer (ws@seh.de)
//  (C) Copyright 2011 Orcan Ogetbil (ogetbilo at sf.net)
//  (C) Copyright 2015-2016 Tim E. Real (terminator356 on sourceforge)
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

#ifndef __SLIDER_H__
#define __SLIDER_H__

#include "sclif.h"
#include "sliderbase.h"
#include "scldraw.h"

namespace MusEGui {

//---------------------------------------------------------
//   Slider
//---------------------------------------------------------

class Slider : public SliderBase, public ScaleIf
      {
  Q_OBJECT
  Q_PROPERTY( double lineStep READ lineStep WRITE setLineStep )
  Q_PROPERTY( double pageStep READ pageStep WRITE setPageStep )
  Q_PROPERTY( Qt::Orientation orientation READ orientation WRITE setOrientation )


 public:
  enum ScalePos { None, Left, Right, Top, Bottom, InsideHorizontal, InsideVertical };

 private:
  Qt::Orientation d_orient;
  ScalePos d_scalePos;
  int d_grooveWidth;
  QColor d_fillColor;
  bool d_fillThumb;
  bool d_fillEmptySide;

  QRect d_sliderRect;

  int d_thumbLength;
  int d_thumbHalf;
  int d_thumbWidth;
  int d_scaleDist;
  int d_xMargin;
  int d_yMargin;
  int d_mMargin;

  bool d_resized;
  bool d_autoResize;
  double d_scaleStep;

  int d_bgStyle;
  int markerPos;

  uint vertical_hint;
  uint horizontal_hint;

  void drawHsBgSlot(QPainter *, const QRect&, const QRect&,const QBrush&);
  void drawVsBgSlot(QPainter *, const QRect&, const QRect&,const QBrush&);

  protected:
  virtual void drawThumb (QPainter *p, const QRect &r);
  virtual void drawSlider (QPainter *p, const QRect &r);
  
  //  Determine the value corresponding to a specified mouse location.
  //  If borderless mouse is enabled p is a delta value not absolute, so can be negative.
  double getValue(const QPoint &p);
  //  Determine the value corresponding to a specified mouse movement.
  double moveValue(const QPoint& /*deltaP*/, bool /*fineMode*/ = false);
  //  Determine scrolling mode and direction.
  void getScrollMode( QPoint &p, const Qt::MouseButton &button, const Qt::KeyboardModifiers& modifiers, int &scrollMode, int &direction);
  // Adjust scale so marks are not too close together.
  void adjustScale();

  virtual void resizeEvent(QResizeEvent *e);
  virtual void paintEvent (QPaintEvent *e);
  void valueChange();
  void rangeChange();
  void scaleChange();
  void fontChange(const QFont &oldFont);

  public:
  
  Slider(QWidget *parent, const char *name = 0,
         Qt::Orientation orient = Qt::Vertical,
         ScalePos scalePos = None,
         int grooveWidth = 8, 
         QColor fillColor = QColor(), 
         ScaleDraw::TextHighlightMode textHighlightMode = ScaleDraw::TextHighlightNone);
  
  ~Slider();
  void setThumbLength(int l);
  void setThumbWidth(int w);

  void setOrientation(Qt::Orientation o);
  Qt::Orientation orientation() const;

  double lineStep() const;
  double pageStep() const;

  void setLineStep(double);
  void setPageStep(double);

  void setMargins(int x, int y);
  int grooveWidth() const { return d_grooveWidth; }
  void setGrooveWidth(int w) { d_grooveWidth = w; update(); }
  
  QColor fillColor() const { return d_fillColor; }
  void setFillColor(const QColor& color) { d_fillColor = color; update(); }
  
  bool fillThumb() const { return d_fillThumb; }
  void setFillThumb(bool v) { d_fillThumb = v; update(); }
  
  bool fillEmptySide() const { return d_fillEmptySide; }
  void setFillEmptySide(bool v) { d_fillEmptySide = v; update(); }
  
  virtual QSize sizeHint() const;
  void setSizeHint(uint w, uint h);
      };

} // namespace MusEGui

#endif
