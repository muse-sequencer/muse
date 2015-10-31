//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: nentry.cpp,v 1.1.1.1.2.1 2008/05/21 00:28:54 terminator356 Exp $
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

#include <QApplication>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMouseEvent>
#include <QTimer>

#include "nentry.h"
#include "gconfig.h"

#define TIMER1    400
#define TIMER2    200
#define TIMEC     7
#define TIMER3    100
#define TIMEC2    20
#define TIMER4    50

namespace MusEGui {

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
      edit->setCursor(QCursor(Qt::ArrowCursor));
//      edit->setFont(font3);
      val = 0;
      layout = new QHBoxLayout(this);
      if (txt == "") {
            layout->addWidget(edit, 1, Qt::AlignHCenter);
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
                  label->setAlignment(Qt::AlignLeft);
                  layout->addWidget(edit, 0, Qt::AlignRight);
                  layout->addSpacing(5);
                  layout->addWidget(label, 100, Qt::AlignRight|Qt::AlignVCenter);
                  }
            }
      if (dark) {
            setDark();
            }
      edit->setFocusPolicy(Qt::NoFocus);
      }

void Nentry::setFocusPolicy(Qt::FocusPolicy policy)
      {
      edit->setFocusPolicy(policy);
      }

void Nentry::setDark()
      {
      const QPalette& oldpalette = edit->palette();
 
      const QColor& newcolor = oldpalette.color(QPalette::Window);
      QPalette newpalette(oldpalette);
      newpalette.setColor(QPalette::Base, newcolor);

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
      if (edit->isModified()) {
            if (setSValue(edit->text())) {
                  setString(val, false);
                  return;
                  }
            edit->setModified(false);
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
      if (event->button() == Qt::LeftButton) {
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
            case Qt::LeftButton:
                  if (!MusEGlobal::config.leftMouseButtonCanDecrease)
                    return;
                  // else fall through
            case Qt::MidButton:
                  decValue(evx);
                  break;
            case Qt::RightButton:
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
      button = Qt::NoButton;
      timer->stop();
      if (event->button() != Qt::LeftButton) {
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
//   mouseDoubleClick
//---------------------------------------------------------

void Nentry::mouseDoubleClick(QMouseEvent* event)
      {
      if (event->button() != Qt::LeftButton) {
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
      bool shift = event->modifiers() & Qt::ShiftModifier;
      bool ctrl  = event->modifiers() & Qt::ControlModifier;
      int key    = event->key();

      if (shift) {
            switch(key) {
                  case Qt::Key_Left:
                  case Qt::Key_Right:
                        return false;
                  default:
                        return true;
                  }
            return true;
            }
      if (ctrl) {
            switch(key) {
                  case Qt::Key_A:
                  case Qt::Key_B:
                  case Qt::Key_C:
                  case Qt::Key_D:
                  case Qt::Key_E:
                  case Qt::Key_F:
                  case Qt::Key_H:
                  case Qt::Key_V:
                  case Qt::Key_X:
                  case Qt::Key_Z:
                  case Qt::Key_Y:
                        return false;
                  default:
                        return true;
                  }
            return true;
            }
      if (event->modifiers())
            return true;
      switch (key) {
            case Qt::Key_Up:   incValue(0); return true;
            case Qt::Key_Down: decValue(0); return true;
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
            case Qt::Key_Minus:
            case Qt::Key_Left:
            case Qt::Key_Right:
            case Qt::Key_Backspace:
            case Qt::Key_Home:
            case Qt::Key_End:
            case Qt::Key_Delete:
            case Qt::Key_Return:
                  return false;
            default:
                  break;
            }
      return true;
      }

} // namespace MusEGui
