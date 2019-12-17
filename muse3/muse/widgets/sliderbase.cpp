//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: sliderbase.cpp,v 1.4.2.4 2007/01/27 14:52:43 spamatica Exp $

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

#include <stdio.h>
#include "muse_math.h"
#include "sliderbase.h"
#include "mmath.h"
#include <QWheelEvent>
#include <QMouseEvent>
#include <QTimerEvent>
#include <QApplication>
#include <QDesktopWidget>
#include <QCursor>
#include <QToolTip>
#include <QScreen>
// For debugging output: Uncomment the fprintf section.
#define DEBUG_SLIDER_BASE(dev, format, args...)  //fprintf(dev, format, ##args);


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

// NOTE: Avoid using borderlessMouse for now because it may jump when clicking from another active window:
      _borderlessMouse = false;

// REMOVE Tim Trackinfo.      
// // [ Audio strip: Press and move: When switching from another active window... ]
// // ---------------------------------------------------------------------------
// //     
// // AudioComponentRack::controllerPressed id:1
// //     val:0.03000000000000002665 calling enableController(false)
// //     
// // SliderBase::mouseMoveEvent firstMouseMoveAfterPress
// //    e->globalPos() x:1042 y:602 _lastGlobalMousePos x:1041 y:602 calling setPosition(delta x:1, y:0)
// // SliderBase::setPosition calling fitValue(delta x:1, y:0) d_mouseOffset:0.00000000000000000000
// // CompactSlider::getValue value:0.03000000000000002665 p x:1 y:0 step:0.01000000000000000021 x change:0.01000000000000000021
// // DoubleRange::setNewValue TOP val:0.04000000000000002859 d_prevValue:0.02000000000000001776 d_value:0.03000000000000002665
// //                          BOTTOM val:0.04000000000000002859 d_prevValue:0.03000000000000002665 d_value:0.04000000000000003553
// //   not equal, calling valueChange
// // AudioComponentRack::controllerChanged id:1 val:0.04000000000000003553
// // AudioComponentRack::controllerMoved id:1 val:0.04000000000000003553
// // 
// // --------------------------------------------------------------------------------------
// // [[ ignoreMouseMove is now set to true, and the cursor is now told to move to screen centre ... ]]
// // --------------------------------------------------------------------------------------
// // 
// // SliderBase::mouseMoveEvent ignoring mouse move
// // 
// // --------------------------------------------------------------------------------------
// // [[ Here's the trouble: The cursor has not yet moved to screen centre !!! It's fine if MusE is already active, see below... ]]
// // --------------------------------------------------------------------------------------
// // 
// // SliderBase::mouseMoveEvent not firstMouseMoveAfterPress
// //    e->globalPos() x:1044 y:601 scrn_cntr x:683 y:384 calling setPosition(delta x:361, y:217)
// // SliderBase::setPosition calling fitValue(delta x:361, y:217) d_mouseOffset:0.00000000000000000000
// // CompactSlider::getValue value:0.04000000000000003553 p x:361 y:217 step:0.01000000000000000021 x change:3.60999999999999987566
// // DoubleRange::setNewValue TOP val:3.64999999999999991118 d_prevValue:0.03000000000000002665 d_value:0.04000000000000003553
// //                          BOTTOM val:3.64999999999999991118 d_prevValue:0.04000000000000003553 d_value:1.00000000000000000000
// //   not equal, calling valueChange
// // AudioComponentRack::controllerChanged id:1 val:1.00000000000000000000
// // AudioComponentRack::controllerMoved id:1 val:1.00000000000000000000
// // 
// // SliderBase::mouseMoveEvent ignoring mouse move
// // 
// // SliderBase::mouseMoveEvent not firstMouseMoveAfterPress
// //    e->globalPos() x:683 y:384 scrn_cntr x:683 y:384 calling setPosition(delta x:0, y:0)
// // SliderBase::setPosition calling fitValue(delta x:0, y:0) d_mouseOffset:0.00000000000000000000
// // CompactSlider::getValue value:1.00000000000000000000 p x:0 y:0 step:0.01000000000000000021 x change:0.00000000000000000000
// // DoubleRange::setNewValue TOP val:1.00000000000000000000 d_prevValue:0.04000000000000003553 d_value:1.00000000000000000000
// //                          BOTTOM val:1.00000000000000000000 d_prevValue:1.00000000000000000000 d_value:1.00000000000000000000
      
      
      _ignoreMouseMove = false;
      _mouseGrabbed = false;
      _pressed = false;
      _pagingButtons = Qt::RightButton;
      _firstMouseMoveAfterPress = false;
      _cursorOverrideCount = 0;
      d_tmrID       = 0;
      d_updTime     = 150;
      d_mass        = 0.0;
      d_tracking    = true;
      d_trackingTempDisable = false;
      d_mouseOffset = 0.0;
      d_valAccum = 0.0;
      d_enableValueToolTips = false;
      d_showValueToolTipsOnHover = false;
      d_valueAtPress = 0.0;
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
      e->accept();
      // Do not allow setting value from the external while mouse is pressed.
      if(_pressed)
        return;
      
      float inc = (maxValue(ConvertNone) - minValue(ConvertNone)) / 40;
      if (e->modifiers() == Qt::ShiftModifier)
            inc = inc / 10;

      if(inc < step())
        inc = step();
      
      if(e->delta() > 0)
            setValue(value(ConvertNone)+inc, ConvertNone);
      else
            setValue(value(ConvertNone)-inc, ConvertNone);

      // Show a handy tooltip value box.
      if(d_enableValueToolTips)
        showValueToolTip(e->globalPos());
      
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
      const bool shift = e->modifiers() & Qt::ShiftModifier;
      //const bool ctrl = e->modifiers() & Qt::ControlModifier;
      const bool meta = e->modifiers() & Qt::MetaModifier;
      
      const bool clicked = (button == Qt::LeftButton || button == Qt::MidButton);
      
      d_timerTick = 0;
      _pressed = true;

      _mouseDeltaAccum = QPoint(); // Reset.
      _lastGlobalMousePos = e->globalPos();
      d_valueAtPress = value(ConvertNone);
      d_valAccum = d_valueAtPress; // Reset.
      
      d_trackingTempDisable = meta; // Generate automation graph recording straight lines if modifier held.
      
      getScrollMode(p, button, e->modifiers(), d_scrollMode, d_direction);
      _lastMousePos = p;
      stopMoving();
      showCursor();
      
      DEBUG_SLIDER_BASE(stderr, "SliderBase::mousePressEvent x:%d, y:%d d_mouseOffset:%.20f d_valueAtPress:%.20f d_scrollMode:%d\n", p.x(), p.y(), d_mouseOffset, d_valueAtPress, d_scrollMode);
      
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
                  
                  // Show a handy tooltip value box.
                  if(d_enableValueToolTips)
                    showValueToolTip(e->globalPos());
                  
                  emit sliderMoved(value(), _id);
                  emit sliderMoved(value(), _id, shift);
                  d_tmrID = startTimer(MusECore::qwtMax(250, 2 * d_updTime));
                  break;
  
            case ScrMouse:
            case ScrDirect:
                  d_speed = 0;
                  //if(!pagingButtons().testFlag(Qt::RightButton) && button == Qt::RightButton)
                  if(button == Qt::RightButton)
                  {
                    // Clear everything.
                    setMouseGrab(false);
                    d_scrollMode = ScrNone;
                    d_direction = 0;
                    _pressed = false;
                    DEBUG_SLIDER_BASE(stderr, "SliderBase::mousePressEvent _pressed:%d\n", _pressed);
                    emit sliderRightClicked(e->globalPos(), _id);
                    break;
                  }  
                  
                  if(d_scrollMode != ScrDirect)
                    d_time.start();
                  
                  if(_cursorHoming && clicked && d_scrollMode != ScrDirect)
                  {
                    _ignoreMouseMove = true; // Avoid recursion.
                    d_mouseOffset = 0.0;
                  }  
                  else if(_borderlessMouse && clicked)
                  {
                    d_mouseOffset = 0.0;
//                     _lastGlobalMousePos = e->globalPos();

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
                    //  it is disastrous. TESTED: Yes, that is how other controls work. Hitting another 
                    //  button while the mouse has been dragged outside causes it to bypass us !
                    setMouseGrab(true); // CAUTION
                    
                    showCursor(false); // CAUTION
                    
                    _firstMouseMoveAfterPress = true;  // Prepare for the first mouse move event after this press.
                    
                    // The problem with this approach of moving the cursor on click is that 
                    //  we can't get a double click signal (ex. to open an editor).
                    //_ignoreMouseMove = true; // Avoid recursion.
                    //const QRect r = QApplication::desktop()->screenGeometry();
                    //const QPoint pt(r.width()/2, r.height()/2);
                    //QCursor::setPos(pt);

                  }  
                  else  
                    d_mouseOffset = getValue(p) - d_valueAtPress;
                  
                  // If direct mode, jump to the pressed location, which also calls valueChange() if anything changed.
                  if(d_scrollMode == ScrDirect)
                  {
                    d_mouseOffset = 0.0;
                    //setPosition(p); // No, it subtracts d_mouseOffset which leaves net zero in case of last line above.
                    DoubleRange::fitValue(getValue(p));
                    // Must set this so that mouseReleaseEvent reads the right value.
                    d_valAccum = value(ConvertNone);
                  }
                  
                  // HACK
                  // In direct mode let the inherited classes call these in their valueChange() methods, 
                  //  so that they may be called BEFORE valueChanged signal is emitted by the setPosition() call above.
                  // ScrDirect mode only happens once upon press with a modifier. After that, another mode is set.
                  // Hack: Since valueChange() is NOT called if nothing changed, call these in that case.
                  if(d_scrollMode != ScrDirect || 
                     value(ConvertNone) == d_valueAtPress)
                  {
                    processSliderPressed(_id);
                    emit sliderPressed(value(), _id);
                  }
                  
                  // Show a handy tooltip value box.
                  if(d_enableValueToolTips)
                    showValueToolTip(e->globalPos());

                  // If direct mode, now set the mode to a regular mouse mode.
                  if(d_scrollMode == ScrDirect)
                    d_scrollMode = ScrMouse;

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
    if (!trackingIsActive() && valueHasChangedAtRelease())
    {
       emit valueChanged(value(), _id);
       emit valueChanged(value(), _id, d_scrollMode); 
    }
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
  _ignoreMouseMove = false;
  const Qt::MouseButton button = e->button();
  const bool clicked = (button == Qt::LeftButton || button == Qt::MidButton);
  const bool shift = e->modifiers() & Qt::ShiftModifier;
  
  _pressed = e->buttons() != Qt::NoButton;
  DEBUG_SLIDER_BASE(stderr, "SliderBase::mouseReleaseEvent pos x:%d y:%d last x:%d y:%d e->buttons():%d button:%d _pressed:%d val:%.20f d_valueAtPress:%.20f\n", 
                    e->pos().x(), e->pos().y(), _lastMousePos.x(), _lastMousePos.y(), int(e->buttons()), button, _pressed, value(), d_valueAtPress);

  e->accept();
  switch(d_scrollMode)
  {
    case ScrMouse:

      if(button == Qt::RightButton)
      {
        d_scrollMode = ScrNone;
        break;
      }
      
      if(_borderlessMouse && clicked)
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
//         setPosition(e->pos());
        movePosition(e->pos() - _lastMousePos, shift);
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
      
      DEBUG_SLIDER_BASE(stderr, " Calling processSliderReleased val:%.20f d_valueAtPress:%.20f\n", value(), d_valueAtPress);
      processSliderReleased(_id);
      emit sliderReleased(value(), _id);
  
    break;

    case ScrDirect:
//       setPosition(e->pos());
      movePosition(e->pos() - _lastMousePos, shift);
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
  
  d_trackingTempDisable = false; // Reset generating automation graph recording straight lines if ctrl held.
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
    DEBUG_SLIDER_BASE(stderr, "SliderBase::setPosition calling fitValue(x:%d, y:%d) d_mouseOffset:%.20f\n", p.x(), p.y(), d_mouseOffset);
    DoubleRange::fitValue(getValue(p) - d_mouseOffset);
}

//------------------------------------------------------------
//
//.F  SliderBase::movePosition
//  Move the slider to a specified point, adjust the value
//  and emit signals if necessary
//
//.u  Syntax
//.f  void SliderBase::movePosition(const QPoint &deltaP, bool fineMode)
//
//.u  Parameters
//.p  const QPoint &deltaP -- Change in position
//.p  bool fineMode -- Fine mode if true, coarse mode if false.
//
//.u  Description
//    Coarse mode (the normal mode) maps pixels to values depending on range and width,
//     such that the slider follows the mouse cursor. Fine mode maps one step() value per pixel.
//------------------------------------------------------------
void SliderBase::movePosition(const QPoint &deltaP, bool fineMode)
{
    DEBUG_SLIDER_BASE(stderr, "SliderBase::movePosition calling fitValue(delta x:%d, y:%d) fineMode:%d\n", deltaP.x(), deltaP.y(), fineMode);
    DoubleRange::fitValue(moveValue(deltaP, fineMode));
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
  DEBUG_SLIDER_BASE(stderr, "SliderBase::mouseMoveEvent _ignoreMouseMove:%d\n", _ignoreMouseMove);
  
  e->accept();

  const bool shift = e->modifiers() & Qt::ShiftModifier;
  //const bool ctrl = e->modifiers() & Qt::ControlModifier;
  const bool meta = e->modifiers() & Qt::MetaModifier;

  //d_trackingTempDisable = ctrl; // Generate automation graph recording straight lines if modifier held.
  
  if(_ignoreMouseMove)
  {
    DEBUG_SLIDER_BASE(stderr, "SliderBase::mouseMoveEvent ignoring mouse move\n");
    _ignoreMouseMove = false;
    return;
  }
  
  const double prevValue = value(ConvertNone);
  
  if (d_scrollMode == ScrMouse )
  {
    d_trackingTempDisable = meta; // Generate automation graph recording straight lines if modifier held.
    if(borderlessMouse())
    {
      const QRect r = QApplication::primaryScreen()->geometry();
      const QPoint scrn_cntr(r.width()/2, r.height()/2);
      QPoint delta;
      if(_firstMouseMoveAfterPress)
      {
        _firstMouseMoveAfterPress = false;
        delta = e->globalPos() - _lastGlobalMousePos;
        DEBUG_SLIDER_BASE(stderr, 
          "SliderBase::mouseMoveEvent firstMouseMoveAfterPress\n   e->globalPos() x:%d y:%d _lastGlobalMousePos x:%d y:%d calling setPosition(delta x:%d, y:%d)\n", 
          e->globalPos().x(), e->globalPos().y(), _lastGlobalMousePos.x(), _lastGlobalMousePos.y(), delta.x(), delta.y());
      }
      else
      {
        delta = e->globalPos() - scrn_cntr;
        DEBUG_SLIDER_BASE(stderr, 
          "SliderBase::mouseMoveEvent not firstMouseMoveAfterPress\n   e->globalPos() x:%d y:%d scrn_cntr x:%d y:%d calling setPosition(delta x:%d, y:%d)\n", 
          e->globalPos().x(), e->globalPos().y(), scrn_cntr.x(), scrn_cntr.y(), delta.x(), delta.y());
      }
      setPosition(delta);
      _ignoreMouseMove = true;
      QCursor::setPos(scrn_cntr);
      //_ignoreMouseMove = false;
    }
    else
    {
      //if(shift)
      //  movePosition(e->pos() - _lastMousePos, true);
      //else
      //  setPosition(e->pos());
      movePosition(e->pos() - _lastMousePos, shift);
    }

    _mouseDeltaAccum += (e->pos() - _lastMousePos);
    _lastMousePos = e->pos();
    _lastGlobalMousePos = e->globalPos();
    
    if (d_mass > 0.0)
    {
        double ms = double(d_time.elapsed());
        if (ms < 1.0) ms = 1.0;
        d_speed = (exactValue(ConvertNone) - exactPrevValue(ConvertNone)) / ms;
        d_time.start();
    }

    //const bool valch = (valueHasChangedAtRelease());
    const bool valch = value(ConvertNone) != prevValue;

    // Show a handy tooltip value box.
    if(d_enableValueToolTips && valch)
      //showValueToolTip(mapToGlobal(pos()));
      showValueToolTip(e->globalPos());

    if(valch)
    {
      emit sliderMoved(value(), _id);
      emit sliderMoved(value(), _id, shift);
    }
  }
  else if(d_scrollMode == ScrNone)
  {
    // Show a handy tooltip value box.
    if(d_enableValueToolTips && d_showValueToolTipsOnHover)
      //showValueToolTip(mapToGlobal(pos()));
      showValueToolTip(e->globalPos());
  }
}

void SliderBase::mouseDoubleClickEvent(QMouseEvent* e)
{
  DEBUG_SLIDER_BASE(stderr, "mouseDoubleClickEvent::mouseDoubleClickEvent\n");
  emit sliderDoubleClicked(e->pos(), _id, e->buttons(), e->modifiers());
  e->ignore();
  QWidget::mouseDoubleClickEvent(e);
}

void SliderBase::keyPressEvent(QKeyEvent* e)
{
  int val = 0;
  switch (e->key())
  {
    case Qt::Key_Up:
      val = 1;
    break;

    case Qt::Key_Down:
      val = -1;
    break;

    default:
      // Let ancestor handle it.
      e->ignore();
      QWidget::keyPressEvent(e);
      return;
    break;
  }

  if(e->modifiers() & (Qt::AltModifier | Qt::MetaModifier | Qt::ControlModifier))
  {
    // Let ancestor handle it.
    e->ignore();
    QWidget::keyPressEvent(e);
    return;
  }

  e->accept();
  // Do not allow setting value from the external while mouse is pressed.
  if(_pressed)
    return;

  if(e->modifiers() == Qt::ShiftModifier)
    //incPages(val);
    incValue(val * 5);
  else
    incValue(val);

  // Show a handy tooltip value box.
  //if(d_enableValueToolTips)
  //  showValueToolTip(e->globalPos());

  emit sliderMoved(value(), _id);
  emit sliderMoved(value(), _id, (bool)(e->modifiers() & Qt::ShiftModifier));
}

void SliderBase::focusOutEvent(QFocusEvent* e)
{
  DEBUG_SLIDER_BASE(stderr, "SliderBase::focusOutEvent _pressed:%d\n", _pressed);
  e->ignore();
  QWidget::focusOutEvent(e);

  // Was a mouse button already pressed before focus was lost?
  // We will NOT get a mouseReleaseEvent! Take care of it here.
  // Typically this happens when popping up a menu in response to a click.
  if(_pressed)
  {
    // Clear everything.
    _ignoreMouseMove = false;
    d_scrollMode = ScrNone;
    d_direction = 0;
    _pressed = false;

    // Make sure this is done. See mousePressEvent.
    showCursor();
    setMouseGrab(false);
  }
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
  const double prevValue = value(ConvertNone);
  double newval;
  double inc = step();

  switch (d_scrollMode)
  {
    case ScrMouse:
      if (d_mass > 0.0)
      {
        d_speed *= exp( - double(d_updTime) * 0.001 / d_mass );
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
  
      if(value(ConvertNone) != prevValue)
      {
        // Show a handy tooltip value box.
        if(d_enableValueToolTips)
          showValueToolTip(cursor().pos());
        
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
      DoubleRange::fitValue(value(ConvertNone) +  double(d_direction) * inc);
  
      if(value(ConvertNone) != prevValue)
      {
        // Show a handy tooltip value box.
        if(d_enableValueToolTips)
          showValueToolTip(cursor().pos());
        
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
    if (trackingIsActive())
    {
       emit valueChanged(value(), _id); 
       emit valueChanged(value(), _id, d_scrollMode); 
    }
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
      if(_pressed)
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
    if(_pressed)
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
    if(_pressed)
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
  if(_pressed)
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








