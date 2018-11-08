#include "sigspinbox.h"
#include "sig.h"
#include <QKeyEvent>
#include <QMouseEvent>
#include <stdio.h>
#include <QLineEdit>
#include <QStyle>
#include <QStyleOption>
#include <QApplication>

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

  MusECore::TimeSignature sig(4, value());
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

//---------------------------------------------------------
//   sizeHint
//---------------------------------------------------------

QSize SigSpinBox::sizeHint() const
      {
      if(const QStyle* st = style())
      {
        st = st->proxy();
        
        QStyleOptionSpinBox option;
        option.initFrom(this);
        option.rect = rect();
        option.state = QStyle::State_Active | QStyle::State_Enabled;
        const QRect b_rect = st->subControlRect(QStyle::CC_SpinBox, &option, QStyle::SC_SpinBoxUp);
        
        QFontMetrics fm(font());
        const int fw = st->pixelMetric(QStyle::PM_SpinBoxFrameWidth);
        int h  = fm.height() + fw * 2;
        int w  = fw * 2 + b_rect.width() + fm.width(QString("00"));
        return QSize(w, h).expandedTo(QApplication::globalStrut());
      }
      return QSize(20, 20).expandedTo(QApplication::globalStrut());      
      }
      
