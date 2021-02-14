//=========================================================
//  MusE
//  Linux Music Editor
//    doublespinbox.cpp (C) Copyright 2012 Tim E. Real (terminator356 at users dot sourceforge dot net)  
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
#include <QLineEdit>
#include <QMouseEvent>
#include "doublespinbox.h"

namespace MusEGui {

void DoubleSpinBoxLineEdit::mouseDoubleClickEvent(QMouseEvent* e)
{
  QLineEdit::mouseDoubleClickEvent(e);
  emit doubleClicked();
  if((e->buttons() & Qt::LeftButton) && (e->modifiers() & Qt::ControlModifier))
    emit ctrlDoubleClicked();
}

//void DoubleSpinBoxLineEdit::mousePressEvent(QMouseEvent* e)
//{
//  QLineEdit::mousePressEvent(e);
  //selectAll();
//  if((e->buttons() & Qt::LeftButton) && (e->modifiers() & Qt::ControlModifier))
//    emit ctrlClicked();
//}

//---------------------------------------------------------
//   DoubleSpinBox
//---------------------------------------------------------

DoubleSpinBox::DoubleSpinBox(QWidget* parent)
   : QDoubleSpinBox(parent)
{
  DoubleSpinBoxLineEdit* le = new DoubleSpinBoxLineEdit(this);
  setLineEdit(le);
  setKeyboardTracking(false);
  
  connect(le, SIGNAL(doubleClicked()),     this, SIGNAL(doubleClicked()));
  connect(le, SIGNAL(ctrlDoubleClicked()), this, SIGNAL(ctrlDoubleClicked()));
  //connect(le, SIGNAL(ctrlClicked()), this, SIGNAL(ctrlClicked()));
}

DoubleSpinBox::DoubleSpinBox(double minValue, double maxValue, double step, QWidget* parent)
   : QDoubleSpinBox(parent)
{
  DoubleSpinBoxLineEdit* le = new DoubleSpinBoxLineEdit(this);
  setLineEdit(le);
  setRange(minValue, maxValue);
  setSingleStep(step);
  setKeyboardTracking(false);

  connect(le, SIGNAL(doubleClicked()),     this, SIGNAL(doubleClicked()));
  connect(le, SIGNAL(ctrlDoubleClicked()), this, SIGNAL(ctrlDoubleClicked()));
  //connect(le, SIGNAL(ctrlClicked()), this, SIGNAL(ctrlClicked()));
}

void DoubleSpinBox::keyPressEvent(QKeyEvent* ev)
{
    switch (ev->key()) {
      case Qt::Key_Return:
        QDoubleSpinBox::keyPressEvent(ev);
        emit returnPressed();
        return;
      break;
      case Qt::Key_Escape:
        emit escapePressed();
        return;
      break;
      default:
      break;
    }
    QDoubleSpinBox::keyPressEvent(ev);
}

void DoubleSpinBox::wheelEvent(QWheelEvent* e)
{
  QDoubleSpinBox::wheelEvent(e);
  // Need this because Qt doesn't deselect the text if not focused.
  if(!hasFocus() && lineEdit())
    lineEdit()->deselect();
}

} // namespace MusEGui

