//=============================================================================
//  MusE
//  Linux Music Editor
//  $Id:$
//
//  Copyright (C) 2002-2006 by Werner Schweer and others
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License version 2.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//=============================================================================

#include "dentry.h"
#include "globals.h"

#define TIMER1    400
#define TIMER2    200
#define TIMEC     7
#define TIMER3    100
#define TIMEC2    20
#define TIMER4    50

//---------------------------------------------------------
//   Dentry
//    lineedit double values
//---------------------------------------------------------

Dentry::Dentry(QWidget* parent)
   : QLineEdit(parent)
      {
      _id = -1;
      drawFrame = false;
      QLineEdit::setFrame(drawFrame);
      timer = new QTimer(this);
      connect(timer, SIGNAL(timeout()), SLOT(repeat()));
      val = 0.01;
      connect(this, SIGNAL(returnPressed()), SLOT(endEdit()));
      setCursor(QCursor(Qt::ArrowCursor));
      evx = 1.0;
      }

//---------------------------------------------------------
//   setFrame
//---------------------------------------------------------

void Dentry::setFrame(bool flag)
      {
      drawFrame = flag;
      QLineEdit::setFrame(drawFrame);
      update();
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
      clearFocus();
      if (!drawFrame)
            QLineEdit::setFrame(false);
      }

//---------------------------------------------------------
//   mousePressEvent
//---------------------------------------------------------

void Dentry::mousePressEvent(QMouseEvent* event)
      {
      button = event->button();
      starty = event->y();
      evx    = double(event->x());
      timecount = 0;
      repeat();
      timer->start(TIMER1);
      }

//---------------------------------------------------------
//   wheelEvent
//---------------------------------------------------------

void Dentry::wheelEvent(QWheelEvent* event)
      {
      int delta = event->delta();

      if (delta < 0)
            decValue(-1.0);
      else if (delta > 0)
            incValue(1.0);
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
                  return;
            case Qt::MidButton:
                  decValue(evx);
                  break;
            case Qt::RightButton:
                  incValue(evx);
                  break;
            default:
                  break;
            }
      }

//---------------------------------------------------------
//   mouseReleaseEvent
//---------------------------------------------------------

void Dentry::mouseReleaseEvent(QMouseEvent*)
      {
      button = Qt::NoButton;
      timer->stop();
      }

//---------------------------------------------------------
//   mouseMoveEvent
//---------------------------------------------------------

void Dentry::mouseMoveEvent(QMouseEvent*)
      {
      switch (button) {
            case Qt::LeftButton:
                  break;
            case Qt::MidButton:
                  break;
            case Qt::RightButton:
                  break;
            default:
                  break;
            }
      }

//---------------------------------------------------------
//   mouseDoubleClickEvent
//---------------------------------------------------------

void Dentry::mouseDoubleClickEvent(QMouseEvent* event)
      {
      if (event->button() != Qt::LeftButton) {
            mousePressEvent(event);
            return;
            }
      setFocus();
      QLineEdit::setFrame(true);
      update();
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

