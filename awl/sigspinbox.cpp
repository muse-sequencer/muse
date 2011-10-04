#include "sigspinbox.h"
#include <QKeyEvent>
#include <stdio.h>

SigSpinBox::SigSpinBox(QWidget *parent) :
    QSpinBox(parent)
{
}
void SigSpinBox::keyPressEvent(QKeyEvent*ev)
{
    switch (ev->key()) {
      case Qt::Key_Return:
        emit returnPressed();
        break;
      case Qt::Key_Left:
      case Qt::Key_Right:
      case Qt::Key_Slash:
        emit moveFocus();
        break;
      default:
        break;
    }
    QSpinBox::keyPressEvent(ev);
}
