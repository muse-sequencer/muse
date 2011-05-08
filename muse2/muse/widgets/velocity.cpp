//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: velocity.cpp,v 1.1.1.1 2003/10/27 18:55:04 wschweer Exp $
//  (C) Copyright 2001 Werner Schweer (ws@seh.de)
//=========================================================

#include <QButtonGroup>
#include "velocity.h"

//---------------------------------------------------------
//   Velocity
//---------------------------------------------------------

Velocity::Velocity(QWidget* parent)
   : QDialog(parent)
      {
      setupUi(this);
      rangeGroup = new QButtonGroup;
      rangeGroup->addButton(allEvents,0);
      rangeGroup->addButton(selectedEvents,1);
      rangeGroup->addButton(loopedEvents,2);
      rangeGroup->addButton(selectedLooped,3);
      
      pullValues();
      }

//---------------------------------------------------------
//   accept
//---------------------------------------------------------

void Velocity::accept()
      {
      pullValues();
      QDialog::accept();
      }

//---------------------------------------------------------
//   pullValues
//---------------------------------------------------------

void Velocity::pullValues()
      {
      range     = rangeGroup->checkedId();
      rateVal   = rate->value();
      offsetVal = offset->value();
      }

//---------------------------------------------------------
//   exec
//---------------------------------------------------------

int Velocity::exec()
      {
      rangeGroup->button(range)->setChecked(true);
      rate->setValue(rateVal);
      offset->setValue(offsetVal);
      
      return QDialog::exec();
      }
