#include "sigspinbox.h"
#include "al/sig.h"
#include <QKeyEvent>
#include <QMouseEvent>
#include <stdio.h>
#include <QLineEdit>

class MyLineEdit : public QLineEdit
{
  public:
    MyLineEdit(QWidget* parent = 0) : QLineEdit(parent) {};
    
  protected:
    virtual void mousePressEvent (QMouseEvent* e)
    {
      QLineEdit::mousePressEvent(e);
      selectAll();
    }
};

SigSpinBox::SigSpinBox(QWidget *parent) :
    QSpinBox(parent)
{
  setKeyboardTracking(false);
  _denominator=false;
  setLineEdit(new MyLineEdit(this));
}

void SigSpinBox::keyPressEvent(QKeyEvent* ev)
{
    switch (ev->key()) {
      case Qt::Key_Return:
        QSpinBox::keyPressEvent(ev);
        emit returnPressed();
        return;
        break;
      case Qt::Key_Escape:
        emit escapePressed();
        return;
        break;
      case Qt::Key_Left:
      case Qt::Key_Right:
      case Qt::Key_Slash:
        emit moveFocus();
        return;
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

void SigSpinBox::stepBy(int step)
{
  if (!_denominator) {
    setValue(value() + step);
    return;
  }

  AL::TimeSignature sig(4, value());
  if (step == 1) {
    // make sure that sig is valid then increase
    if (sig.isValid())
      setValue(value() * 2);
  }
  else if (step == -1) {
    // make sure that sig is valid then increase
    if (sig.isValid()) {
      int v = value() / 2;
      if (v < 2)
        v = 2;
      setValue(v);
    }
  }
}
