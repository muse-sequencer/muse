//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: slider.h,v 1.3.2.2 2008/01/19 13:33:47 wschweer Exp $
//
//    Copyright (C) 1997  Josef Wilgen
//    (C) Copyright 1999 Werner Schweer (ws@seh.de)
//  (C) Copyright 2011 Orcan Ogetbil (ogetbilo at sf.net)
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

 public:
  enum ScalePos { None, Left, Right, Top, Bottom };

   private:
      Q_PROPERTY( double lineStep READ lineStep WRITE setLineStep )
      Q_PROPERTY( double pageStep READ pageStep WRITE setPageStep )
      Q_PROPERTY( Qt::Orientation orientation READ orientation WRITE setOrientation )

  QRect d_sliderRect;

  int d_thumbLength;
  int d_thumbHalf;
  int d_thumbWidth;
  int d_scaleDist;
  int d_xMargin;
  int d_yMargin;
  int d_mMargin;

  QColor d_fillColor;
  
  int d_resized;
  bool d_autoResize;
  double d_scaleStep;

  Qt::Orientation d_orient;
  ScalePos d_scalePos;
  int d_bgStyle;
  int markerPos;

  uint vertical_hint;
  uint horizontal_hint;

  void drawHsBgSlot(QPainter *, const QRect&, const QRect&,const QBrush&);
  void drawVsBgSlot(QPainter *, const QRect&, const QRect&,const QBrush&);

  protected:
  virtual void drawSlider (QPainter *p, const QRect &r);
  double getValue(const QPoint &p);
  void getScrollMode( QPoint &p, const Qt::MouseButton &button, int &scrollMode, int &direction);
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
         QColor fillColor = QColor(100, 100, 255));
  
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
  virtual QSize sizeHint() const;
  void setSizeHint(uint w, uint h);
      };

} // namespace MusEGui

#endif
