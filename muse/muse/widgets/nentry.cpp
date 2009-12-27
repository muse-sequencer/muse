//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: nentry.cpp,v 1.1.1.1.2.1 2008/05/21 00:28:54 terminator356 Exp $
//  (C) Copyright 1999 Werner Schweer (ws@seh.de)
//=========================================================

#include <qlayout.h>
#include <qlabel.h>
#include "nentry.h"
#include <stdio.h>
#include <qtimer.h>
#include <qevent.h>
#include "globals.h"
#include <qapplication.h>
#include <qcursor.h>

#define TIMER1    400
#define TIMER2    200
#define TIMEC     7
#define TIMER3    100
#define TIMEC2    20
#define TIMER4    50

NentryFilter::NentryFilter(QObject* parent)
   : QObject(parent)
      {
      }

void Nentry::setText(const QString& s)
      {
      edit->setText(s);
      }

//---------------------------------------------------------
//   eventFilter
//---------------------------------------------------------

bool NentryFilter::eventFilter(QObject*, QEvent* event)
      {
      Nentry* e = (Nentry*)parent();
      if (event->type() == QEvent::MouseButtonPress) {
            e->mousePress((QMouseEvent*)event);
            return true;
            }
      if (event->type() == QEvent::MouseMove) {
            e->mouseMove((QMouseEvent*)event);
            return true;
            }
      if (event->type() == QEvent::MouseButtonDblClick) {
            e->mouseDoubleClick((QMouseEvent*)event);
            return true;
            }
      if (event->type() == QEvent::MouseButtonRelease) {
            e->mouseRelease((QMouseEvent*)event);
            return true;
            }
      if (event->type() == QEvent::Wheel) {
            e->wheel((QWheelEvent*)event);
            return true;
            }
      if (event->type() == QEvent::KeyPress) {
            return e->keyPress((QKeyEvent*)event);
            }
      if (event->type() == QEvent::ContextMenu) {
            return e->contextMenu((QContextMenuEvent*)event);
            }
      return false;
      }

//---------------------------------------------------------
//   Nentry
//    lineedit int values
//---------------------------------------------------------

Nentry::Nentry(QWidget* parent, const QString& txt,
   int _lPos, bool dark) : QFrame(parent)
      {
      focusW     = 0;
      lPos       = _lPos;
      edit       = new QLineEdit(this);
      timer      = new QTimer(this);
      filter     = new NentryFilter(this);
      drawFrame  = false;
      edit->installEventFilter(filter);
      edit->setFrame(drawFrame);

      connect(timer, SIGNAL(timeout()), SLOT(repeat()));
      connect(edit, SIGNAL(returnPressed()), SLOT(endEdit()));
      edit->setCursor(QCursor(arrowCursor));
//      edit->setFont(font3);
      val = 0;
      layout = new QHBoxLayout(this);
      if (txt == "") {
            layout->addWidget(edit, 1, AlignHCenter);
            }
      else {
            label = new QLabel(txt, this);
            if (lPos == 0) {
                  layout->addStretch(5);
                  layout->addSpacing(5);
                  layout->addWidget(label);
                  layout->addSpacing(5);
                  layout->addWidget(edit);
                  layout->addSpacing(5);
                  layout->addStretch(5);
                  }
            else {
                  label->setAlignment(AlignLeft);
                  layout->addWidget(edit, 0, AlignRight);
                  layout->addSpacing(5);
                  layout->addWidget(label, 100, AlignRight|AlignVCenter);
                  }
            }
      if (dark) {
            setDark();
            }
      edit->setFocusPolicy(NoFocus);
      }

void Nentry::setFocusPolicy(FocusPolicy policy)
      {
      edit->setFocusPolicy(policy);
      }

void Nentry::setDark()
      {
      const QPalette& oldpalette = edit->palette();
      QColorGroup cg1 = oldpalette.active();
      cg1.setColor(QColorGroup::Base, cg1.background());
      QPalette newpalette(cg1, cg1, cg1);
      edit->setPalette(newpalette);
      }

//---------------------------------------------------------
//   setSize
//---------------------------------------------------------

void Nentry::setSize(int n)
      {
      QString s("0000000000000000");
      QFontMetrics fm = edit->fontMetrics();
      int w;
      if (n <= 16)
            w = fm.width(s, n);
      else
            w = fm.width('0') * n;

      edit->setFixedWidth(w + 14);
      }

//---------------------------------------------------------
//   setFrame
//---------------------------------------------------------

void Nentry::setFrame(bool flag)
      {
      drawFrame = flag;
      edit->setFrame(drawFrame);
      }

//---------------------------------------------------------
//   endEdit
//---------------------------------------------------------

void Nentry::endEdit()
      {
      if (edit->edited()) {
            if (setSValue(edit->text())) {
                  setString(val, false);
                  return;
                  }
            edit->setEdited(false);
            }
      if (focusW)
            focusW->setFocus();
      focusW = 0;
      edit->clearFocus();
      if (!drawFrame)
            edit->setFrame(false);
      setString(val, false);
      }

//---------------------------------------------------------
//   contextMenuEvent
//---------------------------------------------------------

bool Nentry::contextMenu(QContextMenuEvent *e)
{
  e->accept();
  return true;
}
//---------------------------------------------------------
//   mousePress
//---------------------------------------------------------

void Nentry::mousePress(QMouseEvent* event)
      {
      button = event->button();
      starty = event->y();
      evx    = event->x();
      if (event->button() == QMouseEvent::LeftButton) {
            focusW = qApp->focusWidget();
            edit->setFocus();
            edit->setFrame(true);
            setString(val, true);
            }
      else {
            timecount = 0;
            repeat();
            timer->start(TIMER1);
            }
      }

//---------------------------------------------------------
//   repeat
//---------------------------------------------------------

void Nentry::repeat()
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
            case QMouseEvent::LeftButton:
                  return;
            case QMouseEvent::MidButton:
                  decValue(evx);
                  break;
            case QMouseEvent::RightButton:
                  incValue(evx);
                  break;
            default:
                  break;
            }
      if (focusW)
            focusW->setFocus();
      edit->clearFocus();
      }

//---------------------------------------------------------
//   mouseRelease
//---------------------------------------------------------

void Nentry::mouseRelease(QMouseEvent* event)
      {
      button = QMouseEvent::NoButton;
      timer->stop();
      if (event->button() != QMouseEvent::LeftButton) {
            if (focusW)
                  focusW->setFocus();
            edit->clearFocus();
            }
      }

//---------------------------------------------------------
//   mouseMove
//---------------------------------------------------------

void Nentry::mouseMove(QMouseEvent*)
      {
      switch (button) {
            case QMouseEvent::LeftButton:
                  break;
            case QMouseEvent::MidButton:
                  break;
            case QMouseEvent::RightButton:
                  break;
            default:
                  break;
            }
      }

//---------------------------------------------------------
//   mouseDoubleClick
//---------------------------------------------------------

void Nentry::mouseDoubleClick(QMouseEvent* event)
      {
      if (event->button() != QMouseEvent::LeftButton) {
            mousePress(event);
            return;
            }
      }

//---------------------------------------------------------
//   wheel
//---------------------------------------------------------

void Nentry::wheel(QWheelEvent* event)
      {
      int n = event->delta();
      if (n > 0)
            incValue(n);
      else
            decValue(-n);
      event->accept();
      }

//---------------------------------------------------------
//   setValue
//---------------------------------------------------------

void Nentry::setValue(int v)
      {
      if (v == val)
           return;
      if (setString(v)) {
            if (!drawFrame)
                  edit->setFrame(false);
            edit->setEnabled(false);
            }
      else {
            edit->setEnabled(true);
            }
      val = v;
      }

//---------------------------------------------------------
//   keyPress
//---------------------------------------------------------

bool Nentry::keyPress(QKeyEvent* event)
      {
      bool shift = event->state() & ShiftButton;
      bool ctrl  = event->state() & ControlButton;
      int key    = event->key();

      if (shift) {
            switch(key) {
                  case Key_Left:
                  case Key_Right:
                        return false;
                  default:
                        return true;
                  }
            return true;
            }
      if (ctrl) {
            switch(key) {
                  case Key_A:
                  case Key_B:
                  case Key_C:
                  case Key_D:
                  case Key_E:
                  case Key_F:
                  case Key_H:
                  case Key_V:
                  case Key_X:
                  case Key_Z:
                  case Key_Y:
                        return false;
                  default:
                        return true;
                  }
            return true;
            }
      if (event->state())
            return true;
      switch (key) {
            case Key_Up:   incValue(0); return true;
            case Key_Down: decValue(0); return true;
            case 0x30:
            case 0x31:
            case 0x32:
            case 0x33:
            case 0x34:
            case 0x35:
            case 0x36:
            case 0x37:
            case 0x38:
            case 0x39:
            case Key_Minus:
            case Key_Left:
            case Key_Right:
            case Key_Backspace:
            case Key_Home:
            case Key_End:
            case Key_Delete:
            case Key_Return:
                  return false;
            default:
                  break;
            }
      return true;
      }

