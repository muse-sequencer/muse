//=============================================================================
//  Awl
//  Audio Widget Library
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

#include "al/al.h"
#include "awl.h"
#include "sigedit.h"
#include "al/sig.h"

namespace Awl {

      using AL::sigmap;

//---------------------------------------------------------
//   SigEdit
//---------------------------------------------------------

SigEdit::SigEdit(QWidget* parent)
   : QAbstractSpinBox(parent)
      {
      initialized = false;
      setReadOnly(false);
      setMinimumWidth(100);   //TD: sizeHint
      lineEdit()->setInputMask("99/99");
      }

SigEdit::~SigEdit()
      {
      }

//---------------------------------------------------------
//   event
//    filter Tab and Backtab key events
//---------------------------------------------------------

bool SigEdit::event(QEvent* event)
      {
      if (event->type() == QEvent::KeyPress) {
            QKeyEvent* ke = static_cast<QKeyEvent*>(event);
            int segment = curSegment();
            if (ke->key() == Qt::Key_Backtab) {
                  if (segment == 2) {
                        lineEdit()->setSelection(5, 2);
                        return true;
                        }
                  if (segment == 1) {
                        lineEdit()->setSelection(0, 4);
                        return true;
                        }
                  }
            if (ke->key() == Qt::Key_Tab) {
                  if (segment == 0) {
                        lineEdit()->setSelection(5, 2);
                        return true;
                        }
                  if (segment == 1) {
                        lineEdit()->setSelection(8, 3);
                        return true;
                        }
                  }
            }
      else if (event->type() == QEvent::FocusIn) {
            QFocusEvent* fe = static_cast<QFocusEvent*>(event);
            QAbstractSpinBox::focusInEvent(fe);
            int segment = curSegment();
            switch(segment) {
                  case 0:  lineEdit()->setSelection(0,4); break;
                  case 1:  lineEdit()->setSelection(5,2); break;
                  case 2:  lineEdit()->setSelection(8,3); break;
                  }
            return true;
            }
      return QAbstractSpinBox::event(event);
      }

//---------------------------------------------------------
//   setValue
//---------------------------------------------------------

void SigEdit::setValue(const AL::TimeSignature& s)
      {
      _sig = s;
      updateValue();
      }

//---------------------------------------------------------
//   updateValue
//---------------------------------------------------------

void SigEdit::updateValue()
      {
      char buffer[64];
      sprintf(buffer, "%d/%d", _sig.z, _sig.n);
      lineEdit()->setText(buffer);
      }

//---------------------------------------------------------
//   stepEnables
//---------------------------------------------------------

QAbstractSpinBox::StepEnabled SigEdit::stepEnabled() const
      {
      int segment = curSegment();
      QAbstractSpinBox::StepEnabled en = QAbstractSpinBox::StepUpEnabled | QAbstractSpinBox::StepDownEnabled;

      switch (segment) {
            case 0:
                  if (_sig.z == 1)
                        en &= ~QAbstractSpinBox::StepDownEnabled;
                  break;
            case 1:
                  if (_sig.n == 1)
                        en &= ~QAbstractSpinBox::StepDownEnabled;
                  break;
            }
      return en;
      }

//---------------------------------------------------------
//   fixup
//---------------------------------------------------------

void SigEdit::fixup(QString& input) const
      {
      printf("fixup <%s>\n", input.toLatin1().data());
      }

//---------------------------------------------------------
//   validate
//---------------------------------------------------------

QValidator::State SigEdit::validate(QString&,int&) const
      {
      // TODO
      // printf("validate\n");
      return QValidator::Acceptable;
      }

//---------------------------------------------------------
//   curSegment
//---------------------------------------------------------

int SigEdit::curSegment() const
      {
      QLineEdit* le = lineEdit();
      int pos = le->cursorPosition();
      int segment = -1;

      if (pos >= 0 && pos <= 4)
            segment = 0;
      else if (pos >= 5 && pos <= 7)
            segment = 1;
      else if (pos >= 8)
            segment = 2;
      else
            printf("curSegment = -1, pos %d\n", pos);
      return segment;
      }

//---------------------------------------------------------
//   stepBy
//---------------------------------------------------------

void SigEdit::stepBy(int steps)
      {
      int segment = curSegment();
      int selPos;
      int selLen;

      bool changed = false;
      AL::TimeSignature osig(_sig);

      switch(segment) {
            case 0:
                  _sig.z += steps;
                  if (_sig.z < 1)
                        _sig.z = 1;
                  selPos = 0;
                  selLen = 2;
                  break;
            case 1:
                  _sig.n += steps;
                  if (_sig.n < 1)
                        _sig.n = 1;
                  selPos = 3;
                  selLen = 2;
                  break;
            default:
                  return;
            }
      if (osig.z != _sig.z || osig.n != _sig.n) {
            changed = true;
            }
      if (changed) {
            updateValue();
            emit valueChanged(_sig);
            }
      lineEdit()->setSelection(selPos, selLen);
      }

      void SigEdit::paintEvent(QPaintEvent* event) {
            if (!initialized)
                  updateValue();
            initialized = true;
            QAbstractSpinBox::paintEvent(event);
            }
}

