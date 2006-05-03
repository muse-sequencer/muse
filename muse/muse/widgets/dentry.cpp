//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: dentry.cpp,v 1.5 2005/11/04 12:03:48 wschweer Exp $
//  (C) Copyright 1999 Werner Schweer (ws@seh.de)
//=========================================================

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

