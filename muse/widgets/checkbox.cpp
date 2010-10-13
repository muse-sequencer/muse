//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: checkbox.cpp,v 1.2.2.2 2006/10/29 07:54:52 terminator356 Exp $
//  (C) Copyright 2004 Werner Schweer (ws@seh.de)
//=========================================================

#include "checkbox.h"
//Added by qt3to4:
#include <QMouseEvent>

//---------------------------------------------------------
//   CheckBox
//---------------------------------------------------------

CheckBox::CheckBox(QWidget* parent, int i, const char* name)
   : QCheckBox(parent, name)
      {
      _id = i;
      connect(this, SIGNAL(toggled(bool)), SLOT(hasToggled(bool)));
      }

void CheckBox::hasToggled(bool val)
      {
      emit toggleChanged(val, _id);
      }

//------------------------------------------------------------
//  mousePressEvent
//------------------------------------------------------------

void CheckBox::mousePressEvent(QMouseEvent *e)
{
  if(e->button() == Qt::RightButton)
    emit checkboxRightClicked(e->globalPos(), _id);
  else
  {
    if(isChecked())
      setChecked(false);
    else
      setChecked(true);
    emit checkboxPressed(_id);
  }
}

//------------------------------------------------------------
//  mouseReleaseEvent
//------------------------------------------------------------

void CheckBox::mouseReleaseEvent(QMouseEvent *e)
{
  if(e->button() == Qt::RightButton)
    return;
    
  emit checkboxReleased(_id);
}


