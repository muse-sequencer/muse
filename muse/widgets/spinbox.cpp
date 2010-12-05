//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: spinbox.cpp,v 1.1.2.3 2009/07/09 18:27:11 terminator356 Exp $
//    (C) Copyright 2001 Werner Schweer (ws@seh.de)
//=========================================================

#include <QEvent>
#include <QKeyEvent>
#include <QMouseEvent>

#include "spinbox.h"

//---------------------------------------------------------
//   SpinBox
//---------------------------------------------------------

SpinBox::SpinBox(QWidget* parent)
   : QSpinBox(parent)
{
  _clearFocus = true;
}

SpinBox::SpinBox(int minValue, int maxValue, int step, QWidget* parent)
   : QSpinBox(parent)
{
  setRange(minValue, maxValue);
  setSingleStep(step);
  _clearFocus = true;
  upEnabled = StepUpEnabled;
  downEnabled = StepDownEnabled;
}

QAbstractSpinBox::StepEnabled SpinBox::stepEnabled() const
{
  return upEnabled | downEnabled;
}

void SpinBox::setStepEnabled(bool up, bool down)
{
  upEnabled = up ? StepUpEnabled : StepNone;
  downEnabled = down ? StepDownEnabled : StepNone;
}

int SpinBox::arrowWidth() const
{
  QStyleOptionSpinBox styleOpt;
  styleOpt.initFrom(this);
  QRect upArrowRect = QApplication::style()->subControlRect(QStyle::CC_SpinBox, &styleOpt, QStyle::SC_SpinBoxUp, this);
  return upArrowRect.width();
}

void SpinBox::setEditor(QLineEdit* ed)
{
  setLineEdit(ed);
}

void SpinBox::mousePressEvent ( QMouseEvent * event )
{
  // FIXME: I couldn't find a way to access the arrow buttons directly. Hence I am using a QRect::contains method.
  // Unfortunately this is not 100% accurate with the Oxygen style; one needs to push to the right hand side of the 
  // buttons. But it works perfect with the QtCurve style - Orcan
  QStyleOptionSpinBox styleOpt;
  styleOpt.initFrom(this);
  QRect upArrowRect = QApplication::style()->subControlRect(QStyle::CC_SpinBox, &styleOpt, QStyle::SC_SpinBoxUp, this);
  QRect downArrowRect = QApplication::style()->subControlRect(QStyle::CC_SpinBox, &styleOpt, QStyle::SC_SpinBoxDown, this);
 
  if (upArrowRect.contains(event->pos()))
    emit(stepUpPressed());
  else if (downArrowRect.contains(event->pos()))
    emit(stepDownPressed());
  QSpinBox::mousePressEvent(event);
}


bool SpinBox::eventFilter(QObject* o, QEvent* ev)
{
    // if (o != (QObject*)editor()) ddskrjo can't find editor()
    //    return QSpinBox::eventFilter(o,ev);
    
    bool retval = FALSE; 
    if(ev->type() == QEvent::KeyPress) 
    {
        QKeyEvent* k = (QKeyEvent*)ev;
        if(k->key() == Qt::Key_Up || k->key() == Qt::Key_Down) 
        {
          // stepUp/stepDown will be called. Set this now.
          _clearFocus = false;
        }  
        else if (k->key() == Qt::Key_Enter || k->key() == Qt::Key_Return) 
        {
          // With this line, two enter presses after an edit will clear focus.
          // Without, just one enter press clears the focus.
          //if(!editor()->isModified())
          {  
            clearFocus();
            return TRUE;
          }
        }
   }
   else
   if(ev->type() == QEvent::MouseButtonDblClick) 
   {
     emit doubleClicked();
     return TRUE;
   }
   
   retval = QSpinBox::eventFilter(o, ev);
     
   return retval;
}

void SpinBox::stepUp()
{
  QSpinBox::stepUp();
  if(_clearFocus)
    clearFocus();
  else  
    _clearFocus = true;
}

void SpinBox::stepDown()
{
  QSpinBox::stepDown();
  if(_clearFocus)
    clearFocus();
  else  
    _clearFocus = true;
}

