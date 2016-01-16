//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: sliderbase.cpp,v 1.4.2.4 2007/01/27 14:52:43 spamatica Exp $

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

#include <cmath>
#include "sliderbase.h"
#include "mmath.h"
#include <QWheelEvent>
#include <QMouseEvent>
#include <QTimerEvent>
#include <QApplication>
#include <QDesktopWidget>
#include <QCursor>

namespace MusEGui {

//   DESCRIPTION
// SliderBase is a base class for
// slider widgets. QwtSliderBase handles the mouse events
// and updates the slider's value accordingly. Derived classes
// only have to implement the @QwtSliderBase::getValue@ and
// @QwtSliderBase::getScrollMode@ members, and should react to a
// @QwtSliderbase::valueChange@, which normally requires repainting.

//------------------------------------------------------------
//.F  SliderBase::SliderBase
//  Constructor
//
//.u  Syntax
//.f   SliderBase::SliderBase(QWidget *parent, const char *name)
//
//.u  Parameters
//.p  QWidget *parent, const char *name
//
//------------------------------------------------------------

SliderBase::SliderBase(QWidget *parent, const char *name)
: QWidget(parent)
      {
      setObjectName(name);
      _id           = -1;
      _cursorHoming    = false;
      _borderlessMouse = false;
      _ignoreMouseMove = false;
      _mouseGrabbed = false;
      _pressed = false;
      _firstMouseMoveAfterPress = false;
      _cursorOverrideCount = 0;
      d_tmrID       = 0;
      d_updTime     = 150;
      d_mass        = 0.0;
      d_tracking    = true;
      d_mouseOffset = 0.0;
      d_scrollMode  = ScrNone;
      setRange(0.0, 1.0, 0.1);
      }

//------------------------------------------------------------
//.F  SliderBase::~SliderBase
//  Destructor
//
//.u  Syntax
//.f   SliderBase::~SliderBase()
//------------------------------------------------------------

SliderBase::~SliderBase()
      {
      // Just in case the ref count is not 0. This is our last chance to clear 
      //  our contribution to QApplication::setOverrideCursor references.
      showCursor();
  
      if (d_tmrID)
            killTimer(d_tmrID);
      }

void SliderBase::showCursor(bool show) 
{ 
  if(_cursorOverrideCount > 1)
    fprintf(stderr, "MusE Warning: _cursorOverrideCount > 1 in SliderBase::showCursor(%d)\n", show);
  if(show)
  {  
    while(_cursorOverrideCount > 0)
    {
      QApplication::restoreOverrideCursor();
      _cursorOverrideCount--;
    }
  }
  else
  {
    _cursorOverrideCount++;
    QApplication::setOverrideCursor(Qt::BlankCursor);
  }
}

void SliderBase::setMouseGrab(bool grabbed)
{
  if(grabbed && !_mouseGrabbed)
  {
    _mouseGrabbed = true;
    grabMouse(); // CAUTION
  }
  else if(!grabbed && _mouseGrabbed)
  {
    releaseMouse();
    _mouseGrabbed = false;
  }
}

//------------------------------------------------------------
//.F  void SliderBase::wheelEvent(QWheelEvent *e)
//  Add wheel event handling
//
//.u  Syntax
//.f  void SliderBase::wheelEvent(QWheelEvent *e)
//------------------------------------------------------------
void SliderBase::wheelEvent(QWheelEvent *e)
{
      // Avoid unwanted wheel events from outside the control.
      // Just in case it grabs focus somehow.
      // Tested: No go, can't seem to determine where event came from.
      /*
      const QPoint gp = mapToGlobal(e->pos());
      const QRect gr = QRect(mapToGlobal(rect().topLeft()), mapToGlobal(rect().bottomRight()));
      if(!gr.contains(gp))
      {
        e->ignore();
        return;
      } */
      
      e->accept();
      // REMOVE Tim. Trackinfo. Added.
      // Do not allow setting value from the external while mouse is pressed.
      if(_pressed)
        return;
      
//       float inc = (maxValue() - minValue()) / 40;
      float inc = (maxValue(ConvertNone) - minValue(ConvertNone)) / 40;
      if (e->modifiers() == Qt::ShiftModifier)
            inc = inc / 10;

      if(inc < step())
        inc = step();
      
      if(e->delta() > 0)
//             setValue(value()+inc);
            setValue(value(ConvertNone)+inc, ConvertNone);
      else
//             setValue(value()-inc);
            setValue(value(ConvertNone)-inc, ConvertNone);

     emit sliderMoved(value(), _id);
     emit sliderMoved(value(), _id, (bool)(e->modifiers() & Qt::ShiftModifier));
}


//------------------------------------------------------------
//.F  SliderBase::stopMoving
//  Stop updating if automatic scrolling is active
//
//.u  Syntax
//.f  void SliderBase::stopMoving()
//------------------------------------------------------------

void SliderBase::stopMoving()
      {
      if(d_tmrID) {
            killTimer(d_tmrID);
            d_tmrID = 0;
            }
      }

//------------------------------------------------------------
//.F  SliderBase::setUpdateTime
//  Specify the update interval for automatic scrolling
//
//.u  Syntax
//.f  void SliderBase::setUpdateTime(int t)
//
//.u  Parameters
//.p  int t -- update interval in milliseconds
//
//.u  See also
//  @SliderBase::getScrollMode@
//------------------------------------------------------------

void SliderBase::setUpdateTime(int t)
      {
      if (t < 50)
            t = 50;
      d_updTime = t;
      }

//------------------------------------------------------------
//.F  SliderBase::mousePressEvent
//  Mouse press event handler
//
//.u  Syntax
//.f  void SliderBase::mousePressEvent(QMouseEvent *e)
//
//.u  Parameters
//.p  QMouseEvent *e -- Qt Mouse press event
//------------------------------------------------------------

void SliderBase::mousePressEvent(QMouseEvent *e)
      {
      e->accept();
      QPoint p = e->pos();
      
      const Qt::MouseButton button = e->button();
      const Qt::MouseButtons buttons = e->buttons();
      //fprintf(stderr, "SliderBase::mousePressEvent button:%d buttons:%d\n", button, int(buttons)); // REMOVE Tim. Trackinfo.
      d_timerTick = 0;
      _pressed = true;

      getScrollMode(p, button, d_scrollMode, d_direction);
      stopMoving();
      showCursor();
      
      // Only one mouse button at a time! Otherwise bad things happen.
      if(buttons ^ button)
      {
        // Clear everything.
        setMouseGrab(false);
        d_scrollMode = ScrNone;
        d_direction = 0;
        _pressed = false;
        return;
      }

      switch(d_scrollMode) {
            case ScrPage:
            case ScrTimer:
                  d_mouseOffset = 0;
                  DoubleRange::incPages(d_direction);
                  emit sliderMoved(value(), _id);
                  emit sliderMoved(value(), _id, (bool)(e->modifiers() & Qt::ShiftModifier));
                  d_tmrID = startTimer(MusECore::qwtMax(250, 2 * d_updTime));
                  break;
  
            case ScrMouse:
                  d_speed = 0;
                  if(!pagingButtons().testFlag(Qt::RightButton) && button == Qt::RightButton)
                  {
                    d_scrollMode = ScrNone;
                    d_direction = 0;
                    emit sliderRightClicked(e->globalPos(), _id);
                    break;
                  }  
                  d_time.start();
                  if(_cursorHoming && button == Qt::LeftButton)
                  {
                    _ignoreMouseMove = true; // Avoid recursion.
                    d_mouseOffset = 0.0;
                  }  
                  else if(_borderlessMouse && button == Qt::LeftButton)
                  {
                    d_mouseOffset = 0.0;
                    _lastGlobalMousePos = e->globalPos();

                    //  "It is almost never necessary to grab the mouse when using Qt, as Qt grabs 
                    //   and releases it sensibly. In particular, Qt grabs the mouse when a mouse 
                    //   button is pressed and keeps it until the last button is released."
                    //
                    // Apparently not. For some reason this was necessary. When the cursor is dragged
                    //  outside the window, holding left then pressing right mouse button COMPLETELY 
                    //  bypasses us, leaving the app's default right-click handler to popup, and leaving 
                    //  us in a really BAD state: mouse is grabbed (and hidden) and no way out !
                    //
                    // That is likely just how QWidget works, but here using global cursor overrides 
                    //  it is disasterous. TESTED: Yes, that is how other controls work. Hitting another 
                    //  button while the mouse has been dragged outside causes it to bypass us !
                    setMouseGrab(true); // CAUTION
                    
                    showCursor(false); // CAUTION
                    
                    _firstMouseMoveAfterPress = true;  // Prepare for the first mouse move event after this press.
//                     _ignoreMouseMove = true; // Avoid recursion.
//                     const QRect r = QApplication::desktop()->screenGeometry();
//                     const QPoint pt(r.width()/2, r.height()/2);
//                     QCursor::setPos(pt);
                  }  
                  else  
//                     d_mouseOffset = getValue(p) - value();
                    d_mouseOffset = getValue(p) - value(ConvertNone);
                  
                  emit sliderPressed(_id);
                  break;
  
            default:
                  d_mouseOffset = 0;
                  d_direction = 0;
            break;
            }
      }


//------------------------------------------------------------
//.-
//.F  SliderBase::buttonRelease
//  Emit a valueChanged() signal if necessary
//
//.u  Syntax
//.f  void SliderBase::buttonReleased()
//
//------------------------------------------------------------
void SliderBase::buttonReleased()
{
//     if ((!d_tracking) || (value() != prevValue()))
    if ((!d_tracking) || valHasChanged())
       emit valueChanged(value(), _id);
}


//------------------------------------------------------------
//
//.F  SliderBase::mouseReleaseEvent
//  Mouse Release Event handler
//
//.u  Syntax
//.f  void SliderBase::mouseReleaseEvent(QMouseEvent *e)
//
//.u  Parameters
//.p  QMouseEvent *e -- Qt Mouse Event
//
//------------------------------------------------------------
void SliderBase::mouseReleaseEvent(QMouseEvent *e)
{
  int ms = 0;
  /*double inc = step(); */ // prevent compiler warning: unused variable 
  _ignoreMouseMove = false;
  const Qt::MouseButton button = e->button();
  //fprintf(stderr, "SliderBase::mouseReleaseEvent button:%d\n", button); // REMOVE Tim. Trackinfo.
  
  _pressed = e->buttons() != Qt::NoButton;

  e->accept();
  switch(d_scrollMode)
  {
    case ScrMouse:

      if(button == Qt::RightButton)
      {
        d_scrollMode = ScrNone;
        break;
      }
      
      if(_cursorHoming && button == Qt::LeftButton)
      {
        d_scrollMode = ScrNone;
      }
      else if(_borderlessMouse && button == Qt::LeftButton)
      {
        d_scrollMode = ScrNone;
        if(!_firstMouseMoveAfterPress)
        {
          _ignoreMouseMove = true;      // Avoid recursion.
          QCursor::setPos(_lastGlobalMousePos);
          //_ignoreMouseMove = false;
        }
//         showCursor();
      }
      else
      {
        setPosition(e->pos());
        d_direction = 0;
        d_mouseOffset = 0;
        if (d_mass > 0.0)
        {
            ms = d_time.elapsed();
            if ((fabs(d_speed) >  0.0) && (ms < 50))
              d_tmrID = startTimer(d_updTime);
        }
        else
        {
            d_scrollMode = ScrNone;
            buttonReleased();
        }
      }
      emit sliderReleased(_id);
  
    break;

    case ScrDirect:
      setPosition(e->pos());
      d_direction = 0;
      d_mouseOffset = 0;
      d_scrollMode = ScrNone;
      buttonReleased();
    break;

    case ScrPage:
      stopMoving();
      d_timerTick = 0;
      buttonReleased();
      d_scrollMode = ScrNone;
    break;

    case ScrTimer:
      stopMoving();
      d_timerTick = 0;
      buttonReleased();
      d_scrollMode = ScrNone;
    break;

    default:
      d_scrollMode = ScrNone;
      buttonReleased();
  }
  
  // Make sure this is done. See mousePressEvent.
  showCursor();
  setMouseGrab(false);
}


//------------------------------------------------------------
//
//.F  SliderBase::setPosition
//  Move the slider to a specified point, adjust the value
//  and emit signals if necessary
//
//.u  Syntax
//.f  void SliderBase::setPosition(const QPoint &p)
//
//.u  Parameters
//.p  const QPoint &p
//
//------------------------------------------------------------
void SliderBase::setPosition(const QPoint &p)
{
    DoubleRange::fitValue(getValue(p) - d_mouseOffset);
}


//------------------------------------------------------------
//
//.F SliderBase::setTracking
//
//  Enables or disables tracking.
//
//.u Syntax
//.f  void SliderBase::setTracking(bool enable)
//
//.u Parameters
//.p  bool enable -- enable (TRUE) or disable (FALSE) tracking
//
//.u Description
//
//  If tracking is enabled, the slider emits a
//  valueChanged() signal whenever its value
//  changes (the default behaviour). If tracking
//  is disabled, the value changed() signal will only
//  be emitted if
//.i  -- the user releases the mouse
//  button and the value has changed or
//  -- at the end of automatic scrolling.
//.P
//  Tracking is enabled by default.
//------------------------------------------------------------
void SliderBase::setTracking(bool enable)
{
    d_tracking = enable;
}

//------------------------------------------------------------
//.-
//.F  SliderBase::mouseMoveEvent
//  Mouse Move Event handler
//
//.u  Syntax
//.f  void SliderBase::mouseMoveEvent(QMouseEvent *e)
//
//.u  Parameters
//.p  QMouseEvent *e  -- Qt Mouse Move Event
//
//------------------------------------------------------------
void SliderBase::mouseMoveEvent(QMouseEvent *e)
{
    //fprintf(stderr, "SliderBase::mouseMoveEvent _ignoreMouseMove:%d\n", _ignoreMouseMove); // REMOVE Tim. Trackinfo.
    
    e->accept();

    if(_ignoreMouseMove)
    {
      _ignoreMouseMove = false;
      return;
    }
    
// //     QPoint p = e->pos();
//     const Qt::MouseButtons buttons = e->buttons();
// //     int cur_scr_mode = d_scrollMode;
// //     getScrollMode(p, button, d_scrollMode, d_direction);
//     
//     // Check the mode: If it changed from ScrMouse it's an error, 
//     //  it means mouse release was not called.
// //     if(borderlessMouse() && d_scrollMode != ScrMouse && cur_scr_mode == ScrMouse)
//     if(button == Qt::NoButton)
//     {
//       d_scrollMode = ScrNone;
//       d_direction = 0;
//       _firstMouseMoveAfterPress = false;
//       showCursor();
//       return;
//     }
    
    if (d_scrollMode == ScrMouse )
    {
      if(borderlessMouse())
      {
        const QRect r = QApplication::desktop()->screenGeometry();
        const QPoint scrn_cntr(r.width()/2, r.height()/2);
        QPoint delta;
        if(_firstMouseMoveAfterPress)
        {
          _firstMouseMoveAfterPress = false;
          delta = e->globalPos() - _lastGlobalMousePos;
        }
        else
          delta = e->globalPos() - scrn_cntr;
        setPosition(delta);
        _ignoreMouseMove = true;
        QCursor::setPos(scrn_cntr);
        //_ignoreMouseMove = false;
      }
      else
        setPosition(e->pos());
      
      if (d_mass > 0.0)
      {
          double ms = double(d_time.elapsed());
          if (ms < 1.0) ms = 1.0;
//           d_speed = (exactValue() - exactPrevValue()) / ms;
          d_speed = (exactValue(ConvertNone) - exactPrevValue(ConvertNone)) / ms;
          d_time.start();
      }
//       if (value() != prevValue())
      if (valHasChanged())
      {
        emit sliderMoved(value(), _id);
        emit sliderMoved(value(), _id, (bool)(e->modifiers() & Qt::ShiftModifier));
      }
    }

}

void SliderBase::mouseDoubleClickEvent(QMouseEvent* e)
{
  //fprintf(stderr, "mouseDoubleClickEvent::mouseDoubleClickEvent\n"); // REMOVE Tim. Trackinfo.
  emit sliderDoubleClicked(e->pos(), _id, e->buttons(), e->modifiers());
  e->ignore();
  QWidget::mouseDoubleClickEvent(e);
}

//------------------------------------------------------------
//
//.F  SliderBase::timerEvent
//  Timer event handler
//
//.u  Syntax
//.f  void SliderBase::timerEvent(QTimerEvent *e)
//
//.u  Parameters
//.p  QTimerEvent *e  --  Qt timer event
//
//------------------------------------------------------------

void SliderBase::timerEvent(QTimerEvent*)
{
    double newval;
    double inc = step();

    switch (d_scrollMode)
    {
    case ScrMouse:
  if (d_mass > 0.0)
  {
      d_speed *= exp( - double(d_updTime) * 0.001 / d_mass );
//       newval = exactValue() + d_speed * double(d_updTime);
      newval = exactValue(ConvertNone) + d_speed * double(d_updTime);
      DoubleRange::fitValue(newval);
      // stop if d_speed < one step per second
      if (fabs(d_speed) < 0.001 * fabs(step()))
      {
    d_speed = 0;
    stopMoving();
    buttonReleased();
      }

  }
  else
     stopMoving();

  break;

    case ScrPage:
  DoubleRange::incPages(d_direction);
  
//   if (value() != prevValue())
  if(valHasChanged())
  {
     emit sliderMoved(value(), _id);
     emit sliderMoved(value(), _id, false);
  }
  
  if (!d_timerTick)
  {
      killTimer(d_tmrID);
      d_tmrID = startTimer(d_updTime);
  }
  break;
    case ScrTimer:
//   DoubleRange::fitValue(value() +  double(d_direction) * inc);
  DoubleRange::fitValue(value(ConvertNone) +  double(d_direction) * inc);
  
//   if (value() != prevValue())
  if (valHasChanged())
  {
     emit sliderMoved(value(), _id);
     emit sliderMoved(value(), _id, false);
  }
  
  if (!d_timerTick)
  {
      killTimer(d_tmrID);
      d_tmrID = startTimer(d_updTime);
  }
  break;
    default:
  stopMoving();
  break;
    }

    d_timerTick = 1;
}


//------------------------------------------------------------
//
//.F  SliderBase::valueChange
//  Notify change of value
//
//.u  Syntax
//.f  void SliderBase::valueChange()
//
//.u  Parameters
//.p  double x  --    new value
//
//.u  Description
//  This function can be reimplemented by derived classes
//  in order to keep track of changes, i.e. repaint the widget.
//  The default implementation emits a valueChanged() signal
//  if tracking is enabled.
//
//------------------------------------------------------------
void SliderBase::valueChange()
{
    if (d_tracking)
       emit valueChanged(value(), _id); 
}

//------------------------------------------------------------
//
//.F SliderBase::setMass
//  Set the slider's mass for flywheel effect.
//
//.u Syntax
//.f  void SliderBase::setMass(double val)
//
//.u Parameters
//.p      double val    --      new mass in kg
//
//.u Description
//
//     If the slider's mass is greater then 0, it will continue
//     to move after the mouse button has been released. Its speed
//     decreases with time at a rate depending on the slider's mass.
//     A large mass means that it will continue to move for a
//     long time.
//
//     Limits: If the mass is smaller than 1g, it is set to zero.
//     The maximal mass is limited to 100kg.
//
//     Derived widgets may overload this function to make it public.
//
//------------------------------------------------------------
void SliderBase::setMass(double val)
{
    if (val < 0.001)
       d_mass = 0.0;
    else if (val > 100.0)
       d_mass = 100.0;
    else
       d_mass = val;
}


//------------------------------------------------------------
//
//.F  SliderBase::setValue
//  Move the slider to a specified value
//
//.u  Syntax
//.f  void SliderBase::setValue(double val)
//
//.u  Parameters
//.p  double val  --  new value
//
//.u  Description
//  This function can be used to move the slider to a value
//  which is not an integer multiple of the step size.
//
//.u  See also
//  @SliderBase::fitValue@
//------------------------------------------------------------

void SliderBase::setValue(double val, ConversionMode mode)
      {
      // Do not allow setting value from the external while mouse is pressed.
      if(_pressed)  // REMOVE Tim. Trackinfo. Added.
        return;
      if (d_scrollMode == ScrMouse)
            stopMoving();
      DoubleRange::setValue(val, mode);
      }


//------------------------------------------------------------
//
//.F  QSlider::fitValue
//  Set the slider's value to the nearest integer multiple
//      of the step size.
//
//.u  Syntax
//.f  void SliderBase::fitValue(double val)
//
//.u  See also:
//  @SliderBase::setValue@
//------------------------------------------------------------
void SliderBase::fitValue(double val, ConversionMode mode)
{
    // Do not allow setting value from the external while mouse is pressed.
    if(_pressed)  // REMOVE Tim. Trackinfo. Added.
      return;
    if (d_scrollMode == ScrMouse) stopMoving();
    DoubleRange::fitValue(val, mode);
}


//------------------------------------------------------------
//
//.F  SliderBase::incValue
//  Increment the value by a specified number of steps
//
//.u  Syntax
//.f  void SliderBase::incValue(int steps)
//
//.u  Parameters
//.p  int steps --   number of steps
//
//------------------------------------------------------------
void SliderBase::incValue(int steps)
{
    // Do not allow setting value from the external while mouse is pressed.
    if(_pressed)  // REMOVE Tim. Trackinfo. Added.
      return;
    if (d_scrollMode == ScrMouse) stopMoving();
    DoubleRange::incValue(steps);
}


//------------------------------------------------------------
//
//.F  SliderBase::stepPage
//  Increment the value by a specified number of steps
//
//.u  Syntax
//.f  void SliderBase::stepPages(int pages)
//
//.u  Parameters
//.p  int pages --   +/- number of pages
//
//.u  Description
//  Steps the control as if pager was clicked.
//  Designed to be called from outside (like from a buddy label), rather than from  
//  the control itself. Calls DoubleRange::incPages, which normally causes valueChange() 
//  (and emits valueChanged), but also emits sliderMoved.
//
//------------------------------------------------------------
void SliderBase::stepPages(int pages)
{
  // Do not allow setting value from the external while mouse is pressed.
  if(_pressed)  // REMOVE Tim. Trackinfo. Added.
    return;
  DoubleRange::incPages(pages);
  emit sliderMoved(value(), _id);
  emit sliderMoved(value(), _id, false);
}


//------------------------------------------------------------
//
//.F  SliderBase::getValue
//  Determine the value corresponding to a specified poind
//
//.u  Syntax
//.f  void SliderBase::getValue(const QPoint &p)
//
//.u  Parameters
//.p  const QPoint &p -- point
//
//.u  Description
//  This is an abstract virtual function which is called when
//  the user presses or releases a mouse button or moves the
//  mouse. It has to be implemented by the derived class.
//
//------------------------------------------------------------

//------------------------------------------------------------
//
//.F  SliderBase::getScrollMode
//  Determine what to do when the user presses a mouse button.
//
//.u  Syntax
//.f  void SliderBase::getScrollMode(const QPoint &p, int &scrollMode, int &direction)
//
//.u  Input Parameters
//.p  const QPoint &p --  point where the mouse was pressed
//
//.u  Output parameters
//  int &scrollMode --  The scrolling mode
//  int &direction  --  direction: 1, 0, or -1.
//
//.u  Description
//  This function is abstract and has to be implemented by derived classes.
//  It is called on a mousePress event. The derived class can determine
//  what should happen next in dependence of the position where the mouse
//  was pressed by returning scrolling mode and direction. SliderBase
//  knows the following modes:
//.t
//  SliderBase::ScrNone -- Scrolling switched off. Don't change the value.
//  SliderBase::ScrMouse -- Change the value while the user keeps the
//          button pressed and moves the mouse.
//  SliderBase::ScrTimer -- Automatic scrolling. Increment the value
//          in the specified direction as long as
//          the user keeps the button pressed.
//  SliderBase::ScrPage -- Automatic scrolling. Same as ScrTimer, but
//          increment by page size.
//
//
//------------------------------------------------------------

//------------------------------------------------------------
//
//.F  SliderBase::valueChanged
//  Notify a change of value.
//
//.u  Syntax
//.f  void SliderBase::valueChanged(double value, int id)
//
//.u  Parameters
//.p  double value -- new value
//
//.u  Description
//      In the default setting
//  (tracking enabled), this signal will be emitted every
//  time the value changes ( see setTracking() ).
//------------------------------------------------------------

//------------------------------------------------------------
//
//.F  SliderBase::sliderPressed
//  This signal is emitted when the user presses the
//  movable part of the slider (start ScrMouse Mode).
//
//.u  Syntax
//.f  void SliderBase::sliderPressed()
//
//------------------------------------------------------------

//------------------------------------------------------------
//
//.F  SliderBase::SliderReleased
//  This signal is emitted when the user releases the
//  movable part of the slider.
//
//.u  Syntax
//.f   void QwtSliderbase::SliderReleased()
//
//------------------------------------------------------------


//------------------------------------------------------------
//
//.F  SliderBase::sliderMoved
//  This signal is emitted when the user moves the
//  slider with the mouse.
//
//.u  Syntax
//.f  void SliderBase::sliderMoved(double value, int _id [, bool shift])
//
//.u  Parameters
//.p  double value  -- new value
//
//------------------------------------------------------------

} // namespace MusEGui








