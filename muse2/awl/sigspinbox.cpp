#include "sigspinbox.h"
#include "al/sig.h"
#include <QKeyEvent>
#include <stdio.h>

SigSpinBox::SigSpinBox(QWidget *parent) :
    QSpinBox(parent)
{
  _denominator=false;
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
      case Qt::Key_Up:
        if (_denominator) {
          // make sure that sig is valid then increase
          AL::TimeSignature sig(4,value());
          if (sig.isValid())
            setValue(value()*2);
          return;
          }
          break;
    case Qt::Key_Down:
      if (_denominator) {
        // make sure that sig is valid then increase
        AL::TimeSignature sig(4,value());
        if (sig.isValid()) {
          int v = value()/2;
          if (v<2)
            v=2;
          setValue(v);
          return;
        }
        return;
        }
        break;
      default:
        break;
    }
    QSpinBox::keyPressEvent(ev);
}

void SigSpinBox::setDenominator()
{
  _denominator=true;
}
