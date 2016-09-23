//=========================================================
//  MusE
//  Linux Music Editor
//
//  popup_double_spinbox.cpp
//  (C) Copyright 2016 Tim E. Real (terminator356 on sourceforge)
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


#include "popup_double_spinbox.h"

#include <QEvent>
#include <QKeyEvent>

// For debugging output: Uncomment the fprintf section.
#define DEBUG_POPUP_DOUBLE_SPINBOX(dev, format, args...) // fprintf(dev, format, ##args);

namespace MusEGui {

//---------------------------------------------------------
//   PopupDoubleSpinBox
//---------------------------------------------------------

PopupDoubleSpinBox::PopupDoubleSpinBox(QWidget* parent)
   : QDoubleSpinBox(parent)
{
  // Reset these since our parent will typically turn them on for speed.
  setAutoFillBackground(true);
  setAttribute(Qt::WA_NoSystemBackground, false);
  setAttribute(Qt::WA_StaticContents, false);
  setAttribute(Qt::WA_OpaquePaintEvent, false);

  setAlignment(Qt::AlignRight | Qt::AlignVCenter);

  setContentsMargins(0, 0, 0, 0);
  setFrame(false);

  _closePending = false;

//   DoubleSpinBoxLineEdit* le = new DoubleSpinBoxLineEdit(this);
//   setLineEdit(le);
//   setKeyboardTracking(false);
//
//   connect(le, SIGNAL(doubleClicked()),     this, SIGNAL(doubleClicked()));
//   connect(le, SIGNAL(ctrlDoubleClicked()), this, SIGNAL(ctrlDoubleClicked()));
//   //connect(le, SIGNAL(ctrlClicked()), this, SIGNAL(ctrlClicked()));
}

bool PopupDoubleSpinBox::event(QEvent* e)
{
  switch(e->type())
  {
    case QEvent::KeyPress:
    {
      QKeyEvent* ke = static_cast<QKeyEvent*>(e);
      switch(ke->key())
      {
        // For return, we want to close the editor but don't want the
        //  parent to receive the event which will just open the box again.
        case Qt::Key_Return:
        case Qt::Key_Enter:
          //e->ignore();
//           QDoubleSpinBox::event(e);
          e->accept();
          //emit editingFinished();
          if(!_closePending)
          {
           _closePending = true;
            emit returnPressed();
          }
//           // Will emit editingFinished.
//           deleteLater();
          return true;
        break;

        case Qt::Key_Escape:
        {
          e->accept();
          //emit editingFinished();
          if(!_closePending)
          {
           _closePending = true;
            emit escapePressed();
          }
//           // Will emit editingFinished.
//           deleteLater();
          return true;
        }
        break;

        default:
        break;
      }
    }
    break;

    case QEvent::NonClientAreaMouseButtonPress:
      // FIXME: Doesn't work.
      DEBUG_POPUP_DOUBLE_SPINBOX(stderr, "PopupDoubleSpinBox::event NonClientAreaMouseButtonPress\n");
    case QEvent::FocusOut:
      e->accept();
      if(!_closePending)
      {
        _closePending = true;
        emit returnPressed();
      }
      return true;
    break;

    default:
    break;
  }

  // Do not pass ANY events on to the parent.
  QDoubleSpinBox::event(e);
  e->accept();
  return true;
}

// void PopupDoubleSpinBox::keyPressEvent(QKeyEvent* e)
// {
// //     switch (e->key()) {
// //       // For return, we want to close the editor but don't want the
// //       //  parent to receive the event which will just open the box again.
// //       case Qt::Key_Return:
// //       case Qt::Key_Escape:
// //         e->accept();
// //         //emit editingFinished(); // Already emitted
// //         return;
// //       break;
// //       default:
// //       break;
// //     }
// //     e->ignore();
//
//     // Do not pass ANY events on to the parent.
//     e->accept();
//     QDoubleSpinBox::keyPressEvent(e);
// }

// void PopupDoubleSpinBox::wheelEvent(QWheelEvent* e)
// {
//   QDoubleSpinBox::wheelEvent(e);
//   // Need this because Qt doesn't deselect the text if not focused.
//   if(!hasFocus() && lineEdit())
//     lineEdit()->deselect();
// }

// void PopupDoubleSpinBox::focusOutEvent(QFocusEvent*)
// {
//   emit editingFinished();
// }

} // namespace MusEGui
