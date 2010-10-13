//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: dentry.cpp,v 1.1.1.1.2.3 2008/08/18 00:15:26 terminator356 Exp $
//  (C) Copyright 1999 Werner Schweer (ws@seh.de)
//=========================================================

#include "dentry.h"
#include <stdio.h>
#include <qtimer.h>
#include <qcursor.h>
//Added by qt3to4:
#include <QWheelEvent>
#include <QMouseEvent>
#include <QContextMenuEvent>
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

Dentry::Dentry(QWidget* parent, const char* name) : QLineEdit(parent, name)
      {
      _slider = 0;      
      _id = -1;
      drawFrame = false;
      QLineEdit::setFrame(drawFrame);
      timer = new QTimer(this);
      connect(timer, SIGNAL(timeout()), SLOT(repeat()));
      val = 0.01;
      connect(this, SIGNAL(returnPressed()), SLOT(endEdit()));
      setCursor(QCursor(Qt::arrowCursor));
      evx = 1.0;
      }

//---------------------------------------------------------
//   contextMenuEvent
//---------------------------------------------------------

void Dentry::contextMenuEvent(QContextMenuEvent * e)
{
  e->accept();
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
      if (edited()) {
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
      // Avoid unwanted wheel events from outside the control.
      // Tested: No go, can't seem to determine where event came from.
      /*
      const QPoint gp = mapToGlobal(event->pos());
      const QRect gr = QRect(mapToGlobal(rect().topLeft()), mapToGlobal(rect().bottomRight()));
      if(!gr.contains(gp))
      */
      //if(sender() != this)
      //{
      //  event->ignore();
      //  return;
      //}
      
      event->accept();
      
      int delta = event->delta();

      if (delta < 0)
      {
        if(_slider)
          _slider->stepPages(-1);
        else
          decValue(-1.0);
      }      
      else if (delta > 0)
      {
        if(_slider)
          _slider->stepPages(1);
        else
          incValue(1.0);
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
                  return;
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
      emit doubleClicked(_id);
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

