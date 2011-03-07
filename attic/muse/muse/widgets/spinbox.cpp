//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: spinbox.cpp,v 1.1.2.3 2009/07/09 18:27:11 terminator356 Exp $
//    (C) Copyright 2001 Werner Schweer (ws@seh.de)
//=========================================================
#include <qlineedit.h>
#include "spinbox.h"

//---------------------------------------------------------
//   SpinBox
//---------------------------------------------------------

SpinBox::SpinBox(QWidget* parent, const char* name)
   : QSpinBox(parent, name)
{
  _clearFocus = true;
}

SpinBox::SpinBox(int minValue, int maxValue, int step, QWidget* parent, const char* name)
   : QSpinBox(minValue, maxValue, step, parent, name)
{
  _clearFocus = true;
}

bool SpinBox::eventFilter(QObject* o, QEvent* ev)
{
    if (o != (QObject*)editor())
        return QSpinBox::eventFilter(o,ev);
    
    bool retval = FALSE; 
    if(ev->type() == QEvent::KeyPress) 
    {
        QKeyEvent* k = (QKeyEvent*)ev;
        if(k->key() == Key_Up || k->key() == Key_Down) 
        {
          // stepUp/stepDown will be called. Set this now.
          _clearFocus = false;
        }  
        else if (k->key() == Key_Enter || k->key() == Key_Return) 
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

