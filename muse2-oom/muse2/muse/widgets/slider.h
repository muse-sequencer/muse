//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: slider.h,v 1.3.2.2 2008/01/19 13:33:47 wschweer Exp $
//
//    Copyright (C) 1997  Josef Wilgen
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License, version 2,
//  as published by the Free Software Foundation.
//
//    (C) Copyright 1999 Werner Schweer (ws@seh.de)
//=========================================================

#ifndef __SLIDER_H__
#define __SLIDER_H__

#include "sclif.h"
#include "sliderbase.h"
#include "scldraw.h"
#include <QPixmap>

//---------------------------------------------------------
//   Slider
//---------------------------------------------------------

class Slider : public SliderBase, public ScaleIf
      {
  Q_OBJECT

 public:
  enum ScalePos { None, Left, Right, Top, Bottom };
  enum { BgTrough = 0x1, BgSlot = 0x2 };

   private:
      Q_PROPERTY( double lineStep READ lineStep WRITE setLineStep )
      Q_PROPERTY( double pageStep READ pageStep WRITE setPageStep )
      Q_PROPERTY( Qt::Orientation orientation READ orientation WRITE setOrientation )

  QRect d_sliderRect;

  int d_thumbLength;
  int d_thumbHalf;
  int d_thumbWidth;
  int d_borderWidth;
  int d_bwTrough;
  int d_scaleDist;
  int d_xMargin;
  int d_yMargin;
  
  int d_resized;
  bool d_autoResize;
  double d_scaleStep;

  Qt::Orientation d_orient;
  ScalePos d_scalePos;
  int d_bgStyle;
  int markerPos;

  void drawHsBgSlot(QPainter *, const QRect&, const QRect&,const QBrush&);
  void drawVsBgSlot(QPainter *, const QRect&, const QRect&,const QBrush&);

  protected:
  virtual void drawSlider (QPainter *p, const QRect &r);
  double getValue(const QPoint &p);
  void getScrollMode( QPoint &p, const Qt::MouseButton &button, int &scrollMode, int &direction);
  void resizeEvent(QResizeEvent *e);
  void paintEvent (QPaintEvent *e);
  void valueChange();
  void rangeChange();
  void scaleChange();
  void fontChange(const QFont &oldFont);

  public:
  Slider(QWidget *parent, const char *name = 0,
      Qt::Orientation orient = Qt::Vertical,
      ScalePos scalePos = None,
      int bgStyle = BgTrough);
  
  ~Slider();
  void setThumbLength(int l);
  void setThumbWidth(int w);

  void setOrientation(Qt::Orientation o);
  Qt::Orientation orientation() const;

  double lineStep() const;
  double pageStep() const;

  void setLineStep(double);
  void setPageStep(double);

  void setBorderWidth(int bw);
      void setMargins(int x, int y);
  QSize sizeHint(); // const;
      };
#endif
