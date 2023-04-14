//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: dentry.cpp,v 1.1.1.1.2.3 2008/08/18 00:15:26 terminator356 Exp $
//  (C) Copyright 1999 Werner Schweer (ws@seh.de)
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

#include <QWidget>
#include <QWheelEvent>
#include <QMouseEvent>
#include <QContextMenuEvent>
#include <QKeyEvent>
#include <QString>
#include <QTimer>
#include <QFocusEvent>

#include "sliderbase.h"
#include "dentry.h"
#include "globals.h"
#include "gconfig.h"

// For debugging output: Uncomment the fprintf section.
//#include <stdio.h>
#define DEBUG_DENTRY(dev, format, args...)  // fprintf(dev, format, ##args);

#define TIMER1    400
#define TIMER2    200
#define TIMEC     7
#define TIMER3    100
#define TIMEC2    20
#define TIMER4    50

namespace MusEGui {

//---------------------------------------------------------
//   Dentry
//    lineedit double values
//---------------------------------------------------------

Dentry::Dentry(QWidget* parent, const char* name) : QLineEdit(parent)
      {
      setObjectName(name);
      _slider = 0;
      _id = -1;
      //setAutoFillBackground(true);
      timer = new QTimer(this);
      connect(timer, &QTimer::timeout, [this]() { repeat(); } );
      val = 0.01;
      connect(this, &QLineEdit::editingFinished, [this]() { endEdit(); } );
      setCursor(QCursor(Qt::ArrowCursor));
      _mousePressed = _keyUpPressed = _keyDownPressed = false;
      }

//---------------------------------------------------------
//   contextMenuEvent
//---------------------------------------------------------

void Dentry::contextMenuEvent(QContextMenuEvent * e)
{
  e->accept();
}

void Dentry::focusOutEvent(QFocusEvent* e)
{
  DEBUG_DENTRY(stderr, "Dentry::focusOutEvent\n");

  e->ignore();
  QLineEdit::focusOutEvent(e);

  // Clear the undo history.
  // Don't allow the changed signal.
  blockSignals(true);
  setString(val);
  blockSignals(false);
}

//---------------------------------------------------------
//   endEdit
//---------------------------------------------------------

void Dentry::endEdit()
      {
      _mousePressed = _keyUpPressed = _keyDownPressed = false;

      // NOTE: DO NOT rely on isModified() to determine if the text changed.
      //       Especially with a validator in place, the line editor can reset
      //        the modified flag. Tested: Possibly a slight QUIRK or BUG in the way
      //        QWidgetLineControl::finishChange() and QWidgetLineControl::internalSetText()
      //        call each other, resulting in two calls to our validate(), and on the
      //        second call the modified flag has been reset.
      //       Especially when using the validator's fixup(). It resets the flag.
      //if (isModified())
      {
            DEBUG_DENTRY(stderr, "Dentry::endEdit: modified:%d\n", isModified());
            bool changed;
            if (setSValue(text(), &changed)) {
                  // If the value was not changed, it is important to re-update the text.
                  // For example, text like "1.0u" might be converted (log clipped) to 0.0 and if that
                  //  is already the value it won't be changed, so the text must be updated to 0.0 again.
                  if(!changed)
                    setString(val);
                  return;
                  }
      }
      DEBUG_DENTRY(stderr, "Dentry::endEdit: Conversion FAILED. Restoring value.\n");

      // The text to value conversion failed. Restore the text from the current value.
      setString(val);
      }

//---------------------------------------------------------
//   mousePressEvent
//---------------------------------------------------------

void Dentry::mousePressEvent(QMouseEvent* event)
      {
      const Qt::MouseButton m_button = event->button();
      const Qt::MouseButtons m_buttons = event->buttons();

      event->accept();

      // Only one mouse button at a time! Otherwise bad things happen.
      if(m_buttons ^ m_button)
      {
        button = Qt::NoButton;
        timer->stop();
        return;
      }

      _mousePressed = true;

      if(m_button == Qt::LeftButton)
        QLineEdit::mousePressEvent(event);

      button = m_button;
      timecount = 0;
      repeat();
      timer->start(TIMER1);
      }

//---------------------------------------------------------
//   wheelEvent
//---------------------------------------------------------

void Dentry::wheelEvent(QWheelEvent* event)
      {
      event->accept();
      
      const QPoint pixelDelta = event->pixelDelta();
      const QPoint angleDegrees = event->angleDelta() / 8;
      int delta = 0;
      if(!pixelDelta.isNull())
        delta = pixelDelta.y();
      else if(!angleDegrees.isNull())
        delta = angleDegrees.y() / 15;
      else
        return;

      if (delta < 0)
      {
        if(_slider)
          _slider->stepPages(-1);
        else
          decValue(1);
      }      
      else if (delta > 0)
      {
        if(_slider)
          _slider->stepPages(1);
        else
          incValue(1);
      }      
      }

//---------------------------------------------------------
//   repeat
//---------------------------------------------------------

void Dentry::repeat()
      {
      if (timecount == 1) {
           ++timecount;
            timer->stop();
            timer->start(TIMER2);
            return;
            }
      ++timecount;
      if (timecount == TIMEC) {
            timer->stop();
            timer->start(TIMER3);
            }
      if (timecount == TIMEC2) {
            timer->stop();
            timer->start(TIMER4);
            }

      switch (button) {
            case Qt::LeftButton:
                  if (!MusEGlobal::config.leftMouseButtonCanDecrease)
                    return;
                  // else fall through
            case Qt::MiddleButton:
                  if(_slider)
                    _slider->stepPages(-1);
                  else  
                    decValue(1);
                  break;
            case Qt::RightButton:
                  if(_slider)
                    _slider->stepPages(1);
                  else  
                    incValue(1);
                  break;
            default:
                  break;
            }
      }

//---------------------------------------------------------
//   mouseReleaseEvent
//---------------------------------------------------------

void Dentry::mouseReleaseEvent(QMouseEvent* ev)
      {
      ev->accept();
      // Don't call ancestor to avoid middle button pasting.
      //LineEdit::mouseReleaseEvent(ev);

      _mousePressed = false;

      button = Qt::NoButton;
      timer->stop();
      }

//---------------------------------------------------------
//   mouseDoubleClickEvent
//---------------------------------------------------------

void Dentry::mouseDoubleClickEvent(QMouseEvent* event)
      {
      event->accept();
      if (event->button() != Qt::LeftButton) {
            //mousePressEvent(event);
            button = event->button();
            timecount = 0;
            repeat();
            timer->start(TIMER1);
            return;
            }
      update();
      emit doubleClicked(_id);
      if(event->modifiers() & Qt::ControlModifier)
        emit ctrlDoubleClicked(_id);
      else
        QLineEdit::mouseDoubleClickEvent(event);
      }

void Dentry::keyPressEvent(QKeyEvent* e)
{
  DEBUG_DENTRY(stderr, "Dentry::keyPressEvent: key:%d modified:%d\n", e->key(), isModified());

  // Check common key sequences first, over specific keys...
  if(e->matches(QKeySequence::Cancel))
  {
    if(isModified())
    {
      // Restore the displayed current value.
      blockSignals(true);
      setString(val);
      blockSignals(false);
    }
    // Let the app or some higher-up use it, such as for yielding focus.
    e->ignore();
    return;
  }

  bool inc = true;
  switch (e->key())
  {
    case Qt::Key_Enter:
    case Qt::Key_Return:
      // Forward it, and let ancestor have it, such as for yielding focus.
      QLineEdit::keyPressEvent(e);
      e->ignore();
      return;
    break;

    case Qt::Key_Up:
      inc = true;
      _keyUpPressed = true;
    break;

    case Qt::Key_Down:
      inc = false;
      _keyDownPressed = true;
    break;

    default:
      // Let the app or some higher-up use it.
      e->ignore();
      // Let ancestor use it.
      QLineEdit::keyPressEvent(e);
      return;
    break;
  }

  e->accept();
  // Do not allow setting value from the external while mouse is pressed.
  //if(_pressed)
  //  return;

  const bool shift = e->modifiers() == Qt::ShiftModifier;
  int val = 1;
  if(shift)
    val *= 10;

  if(inc)
  {
    if(_slider)
      _slider->stepPages(val);
    else
      incValue(val);
  }
  else
  {
    if(_slider)
      _slider->stepPages(-val);
    else
      decValue(val);
  }

  // Show a handy tooltip value box.
  //if(d_enableValueToolTips)
  //  showValueToolTip(e->globalPos());
}

void Dentry::keyReleaseEvent(QKeyEvent* e)
{
  DEBUG_DENTRY(stderr, "Dentry::keyReleaseEvent: key:%d modified:%d\n", e->key(), isModified());
//   e->ignore();

  switch (e->key())
  {
    case Qt::Key_Up:
      _keyUpPressed = false;
    break;

    case Qt::Key_Down:
      _keyDownPressed = false;
    break;

    default:
      QLineEdit::keyReleaseEvent(e);;
    break;
  }
}

//---------------------------------------------------------
//   setValue
//---------------------------------------------------------

void Dentry::setValue(double v)
      {
        // Unless linked to a slider, do not allow setting value from the external
        //  while mouse is pressed or key up/down is pressed.
        if(!_slider && (_mousePressed || _keyUpPressed || _keyDownPressed))
          return;
        setNewValue(v);
      }

double Dentry::value() const { return val; };
int Dentry::id() const    { return _id; }
void Dentry::setId(int i) { _id = i; }
SliderBase* Dentry::slider() const            { return _slider; }
void Dentry::setSlider(SliderBase* s)         { _slider = s; }

} // namespace MusEGui
