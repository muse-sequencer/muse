//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: spinbox.cpp,v 1.1.2.3 2009/07/09 18:27:11 terminator356 Exp $
//    (C) Copyright 2001 Werner Schweer (ws@seh.de)
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

#include <QKeyEvent>
#include <QEvent>
#include <QLineEdit>
#include <QMouseEvent>
#include "spinbox.h"

namespace MusEGui {

void SpinBoxLineEdit::mouseDoubleClickEvent(QMouseEvent* e)
{
  QLineEdit::mouseDoubleClickEvent(e);
  emit doubleClicked();
  if((e->buttons() & Qt::LeftButton) && (e->modifiers() & Qt::ControlModifier))
    emit ctrlDoubleClicked();
}

//void SpinBoxLineEdit::mousePressEvent(QMouseEvent* e)
//{
//  QLineEdit::mousePressEvent(e);
  //selectAll();
//  if((e->buttons() & Qt::LeftButton) && (e->modifiers() & Qt::ControlModifier))
//    emit ctrlClicked();
//}

//---------------------------------------------------------
//   SpinBox
//---------------------------------------------------------

SpinBox::SpinBox(QWidget* parent)
   : QSpinBox(parent)
{
  _returnMode = false;
  SpinBoxLineEdit* le = new SpinBoxLineEdit(this);
  setLineEdit(le);
  setKeyboardTracking(false);
  
  connect(le, SIGNAL(doubleClicked()),     this, SIGNAL(doubleClicked()));
  connect(le, SIGNAL(ctrlDoubleClicked()), this, SIGNAL(ctrlDoubleClicked()));
  //connect(le, SIGNAL(ctrlClicked()), this, SIGNAL(ctrlClicked()));
}

SpinBox::SpinBox(int minValue, int maxValue, int step, QWidget* parent)
   : QSpinBox(parent)
{
  _returnMode = false;
  SpinBoxLineEdit* le = new SpinBoxLineEdit(this);
  setLineEdit(le);
  setRange(minValue, maxValue);
  setSingleStep(step);
  setKeyboardTracking(false);

  connect(le, SIGNAL(doubleClicked()),     this, SIGNAL(doubleClicked()));
  connect(le, SIGNAL(ctrlDoubleClicked()), this, SIGNAL(ctrlDoubleClicked()));
  //connect(le, SIGNAL(ctrlClicked()), this, SIGNAL(ctrlClicked()));
}

void SpinBox::keyPressEvent(QKeyEvent* ev)
{
    switch (ev->key()) {
      case Qt::Key_Return:
        {
          bool mod =  lineEdit()->isModified();
          QSpinBox::keyPressEvent(ev);
          if(_returnMode && !mod)        // Force valueChanged if return mode set, even if not modified.
            emit valueChanged(value());
          emit returnPressed();
        }  
        return;
      break;
      case Qt::Key_Escape:
        emit escapePressed();
        return;
      break;
      default:
      break;
    }
    QSpinBox::keyPressEvent(ev);
}

void SpinBox::wheelEvent(QWheelEvent* e)
{
  QSpinBox::wheelEvent(e);
  // Need this because Qt doesn't deselect the text if not focused.
  if(!hasFocus() && lineEdit())
    lineEdit()->deselect();
}

} // namespace MusEGui

