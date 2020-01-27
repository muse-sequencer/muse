//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: sliderbase.h,v 1.4.2.3 2006/11/14 06:28:37 terminator356 Exp $

//    Copyright (C) 1997  Josef Wilgen
//    (C) Copyright 1999 Werner Schweer (ws@seh.de)
//    (C) Copyright 2016 Tim E. Real (terminator356 on sourceforge)
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

#include <QElapsedTimer>
#include <QWidget>

class QFocusEvent;

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

 public:
  enum SliderBaseScrollMode { ScrNone, ScrMouse, ScrTimer, ScrDirect, ScrPage };
  
  int _id;
  int d_tmrID;
  int d_updTime;
  int d_timerTick;
  QElapsedTimer d_time;
  double d_speed;
  double d_mass;
  bool _cursorHoming;
  bool _borderlessMouse;
  double d_valAccum;
  QPoint _mouseDeltaAccum;
  QPoint _lastMousePos;
  QPoint _lastGlobalMousePos;
  Qt::MouseButtons _pagingButtons;

  // Internal flags.
  bool _ignoreMouseMove;
  bool _firstMouseMoveAfterPress;
  // Whether we have grabbed the mouse.
  bool _mouseGrabbed;
  // The number of times we have called QApplication::setOverrideCursor().
  // This should always be one or zero, anything else is an error, but unforeseen 
  //  events might cause us to miss a decrement with QApplication::restoreOverrideCursor().
  int _cursorOverrideCount;
  
  // If show is true, calls QApplication::restoreOverrideCursor() until _cursorOverrideCount-- is <= 0.
  // If show is false, calls QApplication::setOverrideCursor with a blank cursor.
  void showCursor(bool show = true);
  // Sets or resets the _mouseGrabbed flag and grabs or releases the mouse.
  void setMouseGrab(bool grabbed = false);
  
  void buttonReleased();

 protected:
  bool d_enableValueToolTips;
  bool d_showValueToolTipsOnHover;
  int d_scrollMode;
  double d_mouseOffset;
  int d_direction;
  int d_tracking;
  bool _pressed;
  bool d_trackingTempDisable;
  double d_valueAtPress;
  
  bool valueHasChangedAtRelease() const { return value(ConvertNone) != d_valueAtPress; }
  
  virtual void setMass(double val);
  void setPosition(const QPoint &p);
  void movePosition(const QPoint &deltaP, bool fineMode);
  virtual void valueChange();
  virtual double mass() const { return d_mass; }

  void wheelEvent(QWheelEvent *e);
  void timerEvent(QTimerEvent *e);
  void mousePressEvent(QMouseEvent *e);
  void mouseReleaseEvent(QMouseEvent *e);
  virtual void mouseMoveEvent(QMouseEvent *e);
  virtual void mouseDoubleClickEvent(QMouseEvent *e);
  virtual void keyPressEvent(QKeyEvent*);
  // Required because if focus is lost while a mouse button is pressed, we do not get the mouseReleaseEvent.
  virtual void focusOutEvent(QFocusEvent*);
  
  //  Determine the value corresponding to a specified mouse location.
  //  If borderless mouse is enabled p is a delta value not absolute, so can be negative.
  virtual double getValue(const QPoint & p) = 0;
  //  Determine the value corresponding to a specified mouse movement.
  virtual double moveValue(const QPoint &deltaP, bool fineMode = false) = 0;
  //  Determine scrolling mode and direction.
  virtual void getScrollMode( QPoint &p, const Qt::MouseButton &button, const Qt::KeyboardModifiers& modifiers,
           int &scrollMode, int &direction) = 0;

  // Show a handy tooltip value box.
  virtual void showValueToolTip(QPoint) { }
  // Same as sliderPressed signal, except it's not a signal and is called before sliderPressed is emitted.
  virtual void processSliderPressed(int) { }
  // Same as sliderReleased signal, except it's not a signal and is called before sliderReleased is emitted.
  virtual void processSliderReleased(int) { }
    
 public slots:
  void setValue(double val, ConversionMode mode = ConvertDefault);
  void fitValue(double val, ConversionMode mode = ConvertDefault);
  void incValue(int steps);
  
 signals:
  void valueChanged(double value, int id);
  void valueChanged(double value, int id, int scrollMode);
  void sliderPressed(double value, int id);
  void sliderReleased(double value, int id);
  void sliderMoved(double value, int id);
  void sliderMoved(double value, int id, bool shift);
  void sliderRightClicked(QPoint p, int id);
  void sliderDoubleClicked(QPoint p, int id, Qt::MouseButtons buttons, Qt::KeyboardModifiers keys);

 public:
  
  SliderBase( QWidget *parent = 0, const char *name = 0 );
  virtual ~SliderBase();

  // Useful for signal mappers where we can't pass scroll mode.
  int scrollMode() const { return d_scrollMode; }
  bool mouseGrabbed() const { return _mouseGrabbed; }
  bool isPressed() const { return _pressed; }

  bool cursorHoming() const { return _cursorHoming; }
  void setCursorHoming(bool b) { _cursorHoming = b; }
  bool borderlessMouse() const { return _borderlessMouse; }
  void setBorderlessMouse(bool v) { _borderlessMouse = v; update(); }
  // The allowed mouse buttons which will cause a page step.
  Qt::MouseButtons pagingButtons() const { return _pagingButtons; }
  // Set the allowed mouse buttons which will cause a page step.
  void setPagingButtons(Qt::MouseButtons buttons) { _pagingButtons = buttons; }
  
  bool enableValueToolTips() const { return d_enableValueToolTips; }
  void setEnableValueToolTips(bool enable) { d_enableValueToolTips = enable; }
  bool showValueToolTipsOnHover() const { return d_showValueToolTipsOnHover; }
  void setShowValueToolTipsOnHover(bool enable) { d_showValueToolTipsOnHover = enable; }

  void setUpdateTime(int t);
  void stopMoving();
  bool tracking() const { return d_tracking; }
  void setTracking(bool enable);
  bool trackingIsActive() const { return d_tracking && !d_trackingTempDisable; }

  void stepPages(int pages);
  void setMinValue(double v, ConversionMode mode = ConvertDefault) 
    { DoubleRange::setRange(v, maxValue(mode), 0.0, 1, mode); }
  void setMaxValue(double v, ConversionMode mode = ConvertDefault) 
    { DoubleRange::setRange(minValue(mode), v, 0.0, 1, mode); }
  int id() const             { return _id; }
  void setId(int i)          { _id = i; }
  };

} // namespace MusEGui

#endif
