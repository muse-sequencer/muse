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

#include <stdio.h>

#include <QWidget>
#include <QWheelEvent>
#include <QMouseEvent>
#include <QContextMenuEvent>
#include <QKeyEvent>
#include <QString>
#include <QTimer>

#include "sliderbase.h"
#include "dentry.h"
#include "globals.h"
#include "gconfig.h"

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
      connect(timer, SIGNAL(timeout()), SLOT(repeat()));
      val = 0.01;
      connect(this, SIGNAL(returnPressed()), SLOT(endEdit()));
      setCursor(QCursor(Qt::ArrowCursor));
      evx = 1;
      }

//---------------------------------------------------------
//   contextMenuEvent
//---------------------------------------------------------

void Dentry::contextMenuEvent(QContextMenuEvent * e)
{
  e->accept();
}

//---------------------------------------------------------
//   endEdit
//---------------------------------------------------------

void Dentry::endEdit()
      {
      if (isModified()) {
            if (setSValue(text())) {
                  setString(val);
                  return;
                  }
            }
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

      if(m_button == Qt::LeftButton)
        QLineEdit::mousePressEvent(event);

      button = m_button;
      starty = event->y();
      evx    = event->x();
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
            case Qt::MidButton:
                  if(_slider)
                    _slider->stepPages(-1);
                  else  
                    decValue(evx);
                  break;
            case Qt::RightButton:
                  if(_slider)
                    _slider->stepPages(1);
                  else  
                    incValue(evx);
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
            starty = event->y();
            evx    = event->x();
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
  bool inc = true;
  switch (e->key())
  {
    case Qt::Key_Escape:
      if(isModified())
      {
        // Restore the displayed current value.
        blockSignals(true);
        setString(val);
        blockSignals(false);
      }
      // Let ancestor have it, such as for yielding focus.
      e->ignore();
      return;
    break;

    case Qt::Key_Up:
      inc = true;
    break;

    case Qt::Key_Down:
      inc = false;
    break;

    default:
      // Let ancestor handle it.
      e->ignore();
      QLineEdit::keyPressEvent(e);
      return;
    break;
  }

  if(e->modifiers() & (Qt::AltModifier | Qt::MetaModifier | Qt::ControlModifier))
  {
    // Let ancestor handle it.
    e->ignore();
    QLineEdit::keyPressEvent(e);
    return;
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

//---------------------------------------------------------
//   setValue
//---------------------------------------------------------

void Dentry::setValue(double v)
      {
      if (v == val)
           return;
      setString(v);
#if 0
      if (setString(v)) {
            clearFocus();
            if (!drawFrame)
                  QLineEdit::setFrame(false);
            setEnabled(false);
            }
      else {
            setEnabled(true);
            }
#endif
      val = v;
      }

} // namespace MusEGui
