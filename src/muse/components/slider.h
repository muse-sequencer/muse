//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: slider.h,v 1.3.2.2 2008/01/19 13:33:47 wschweer Exp $
//
//    Copyright (C) 1997  Josef Wilgen
//    (C) Copyright 1999 Werner Schweer (ws@seh.de)
//  (C) Copyright 2011 Orcan Ogetbil (ogetbilo at sf.net)
//  (C) Copyright 2015-2023 Tim E. Real (terminator356 on sourceforge)
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

#include <QWidget>
#include <QPainter>
#include <QPainterPath>
#include <QPaintEvent>
#include <QString>
#include <QResizeEvent>
#include <QSize>
#include <QPoint>
#include <QColor>
#include <QRect>
#include <QBrush>
#include <QFont>
#include <QMargins>

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
  enum ScalePos { ScaleNone, ScaleLeftOrTop, ScaleRightOrBottom, ScaleInside };

 private:
  Qt::Orientation d_orient;
  ScalePos d_scalePos;

  int d_grooveWidth;
  QColor d_fillColor;
  QColor d_handleColor;
  bool d_fillThumb;
  bool d_fillEmptySide;
  bool d_frame;
  QColor d_frameColor;

  int d_radius;
  int d_radiusHandle;
  bool d_useGradient;

  QRect d_sliderRect;
  QRect d_scaleRect;
  QRect d_scaleGeom;
  QRect d_spacerRect;
  QRect d_grooveRect;
  QPainterPath d_framePath;

  int d_thumbLength;
  int d_thumbHalf;
  int d_thumbWidth;
  int d_scaleDist;
  int d_xMargin;
  int d_yMargin;
  int d_mMargin;

  bool d_autoResize;
  double d_scaleStep;

  int d_bgStyle;
  int markerPos;

  uint vertical_hint;
  uint horizontal_hint;

  void drawHsBgSlot(QPainter *, const QRect&, const QRect&,const QBrush&);
  void drawVsBgSlot(QPainter *, const QRect&, const QRect&,const QBrush&);

  protected:
  virtual void drawThumb (QPainter *p, QPaintEvent *e);
  virtual void drawSlider (QPainter *p, QPaintEvent *e);

  //  Determine the value corresponding to a specified mouse location.
  //  If borderless mouse is enabled p is a delta value not absolute, so can be negative.
  double getValue(const QPoint &p);
  //  Determine the value corresponding to a specified mouse movement.
  double moveValue(const QPoint& /*deltaP*/, bool /*fineMode*/ = false);
  //  Determine scrolling mode and direction.
  void getScrollMode( QPoint &p, const Qt::MouseButton &button, const Qt::KeyboardModifiers& modifiers, int &scrollMode, int &direction);

  // Does a fine-grained region painting update of areas that moved or changed.
  void partialUpdate();

  // Setup all slider and scale rectangles.
  void adjustSize(const QSize& s);
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
         ScalePos scalePos = ScaleNone,
         int grooveWidth = 8,
         QColor fillColor = QColor(),
         ScaleDraw::TextHighlightMode textHighlightMode = ScaleDraw::TextHighlightNone,
         QColor handleColor = QColor());
  
  virtual ~Slider();
  void setThumbLength(int l);
  void setThumbWidth(int w);

  void setOrientation(Qt::Orientation o);
  Qt::Orientation orientation() const;

  QMargins scaleEndpointsMargins() const;

  // In log mode, dBFactor sets the dB factor when conversions are done.
  // For example 20 * log10() for signals, 10 * log10() for power, and 40 * log10() for MIDI volume.
  // logFactor sets the scale of the range. For example a MIDI volume control can set a logFactor = 127
  //  so that the range can conveniently be set to 0-127. (With MIDI volume, dBFactor would be
  //  set to 40.0, as per MMA specs.)
  void setScale (double vmin, double vmax, ScaleIf::ScaleType scaleType = ScaleIf::ScaleLinear,
    double dBFactor = 20.0, double logFactor = 1.0);
  void setScale (double vmin, double vmax, double step, ScaleIf::ScaleType scaleType = ScaleIf::ScaleLinear,
    double dBFactor = 20.0, double logFactor = 1.0);
  void setScale(const ScaleDiv &s);
  void setScaleMaxMajor( int ticks);
  void setScaleMaxMinor( int ticks);
  void setScaleBackBone(bool v);
  // Returns the space between the VU and the scale.
  int scaleDist() const;
  // Sets the space between the VU and the scale.
  void setScaleDist(int);

  double lineStep() const;
  double pageStep() const;

  void setLineStep(double);
  void setPageStep(double);

  void setMargins(int x, int y);
  int grooveWidth() const;
  void setGrooveWidth(int w);
  
//  QColor fillColor() const;
  void setFillColor(const QColor& color);
  void setHandleColor(const QColor& color);
  
  bool fillThumb() const;
  void setFillThumb(bool v);
  
  bool fillEmptySide() const;
  void setFillEmptySide(bool v);
  
  virtual QSize sizeHint() const;
  void setSizeHint(uint w, uint h);

  void setRadius(int r);
  void setRadiusHandle(int r);
  void setHandleHeight(int h);
  void setHandleWidth(int w);
  void setUseGradient(bool b);
  void setScalePos(const ScalePos& s);
  void setFrame(bool b);
  void setFrameColor(QColor c);
};

} // namespace MusEGui

#endif
