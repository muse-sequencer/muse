//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: sliderbase.h,v 1.4.2.3 2006/11/14 06:28:37 terminator356 Exp $

//    Copyright (C) 1997  Josef Wilgen
//    (C) Copyright 1999 Werner Schweer (ws@seh.de)
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

#ifndef __SLIDERBASE_H__
#define __SLIDERBASE_H__

#include "drange.h"

#include <QTime>
#include <QWidget>

namespace MusEGui {

//---------------------------------------------------------
//   SliderBase
//---------------------------------------------------------

class SliderBase : public QWidget, public DoubleRange
      {
  Q_OBJECT
      Q_PROPERTY( int id READ id WRITE setId )
      Q_PROPERTY( double minValue READ minValue WRITE setMinValue )
      Q_PROPERTY( double maxValue READ maxValue WRITE setMaxValue )
      Q_PROPERTY( double value READ value WRITE setValue )

      int _id;
  int d_tmrID;
  int d_updTime;
  int d_timerTick;
  QTime d_time;
  double d_speed;
  double d_mass;
  bool _cursorHoming;
  bool _ignoreMouseMove;

  void buttonReleased();

 protected:
  int d_scrollMode;
  double d_mouseOffset;
  int d_direction;
  int d_tracking;
  
  virtual void setMass(double val);
  void setPosition(const QPoint &p);
  virtual void valueChange();
  virtual double mass() const { return d_mass; }

      void wheelEvent(QWheelEvent *e);
  void timerEvent(QTimerEvent *e);
  void mousePressEvent(QMouseEvent *e);
  void mouseReleaseEvent(QMouseEvent *e);
  void mouseMoveEvent(QMouseEvent *e);
  virtual double getValue(const QPoint & p) = 0;
  virtual void getScrollMode( QPoint &p, const Qt::MouseButton &button, 
           int &scrollMode, int &direction) = 0;

 public slots:
  void setValue(double val);
  void fitValue(double val);
  void incValue(int steps);
  
 signals:
  void valueChanged(double value, int id);
  void sliderPressed(int id);
  void sliderReleased(int id);
  void sliderMoved(double value, int id);
  void sliderMoved(double value, int id, bool shift);
  void sliderRightClicked(const QPoint &p, int id);

 public:
  enum { ScrNone, ScrMouse, ScrTimer, ScrDirect, ScrPage };
  
  SliderBase( QWidget *parent = 0, const char *name = 0 );
  ~SliderBase();

  bool cursorHoming() const { return _cursorHoming; }
  void setCursorHoming(bool b) { _cursorHoming = b; }
  void setUpdateTime(int t);
  //  void incValue(double nSteps);
  void stopMoving();
  void setTracking(bool enable);

      double value() const       { return DoubleRange::value(); }
      void stepPages(int pages);
      double minValue() const    { return  DoubleRange::minValue(); }
      double maxValue() const    { return  DoubleRange::maxValue(); }
      void setMinValue(double v) { DoubleRange::setRange(v, maxValue(), 0.0, 1); }
      void setMaxValue(double v) { DoubleRange::setRange(minValue(), v, 0.0, 1); }
      int id() const             { return _id; }
      void setId(int i)          { _id = i; }
      };

} // namespace MusEGui

#endif
