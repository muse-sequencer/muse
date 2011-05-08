//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: gatetime.cpp,v 1.1.1.1 2003/10/27 18:54:37 wschweer Exp $
//  (C) Copyright 2001 Werner Schweer (ws@seh.de)
//=========================================================

#include <QButtonGroup>
#include <QDialog>

#include "gatetime.h"

#include "song.h"

//---------------------------------------------------------
//   GateTime
//---------------------------------------------------------

GateTime::GateTime(QWidget* parent)
   : QDialog(parent)
      {
      setupUi(this);
      rangeGroup = new QButtonGroup(rangeBox);
      rangeGroup->addButton(allButton, 0);
      rangeGroup->addButton(selButton, 1);
      rangeGroup->addButton(loopButton, 2);
      rangeGroup->addButton(sloopButton, 3);
      rangeGroup->setExclusive(true);
      
      pullValues();
      }

//---------------------------------------------------------
//   accept
//---------------------------------------------------------

void GateTime::accept()
      {
      pullValues();
      QDialog::accept();
      }

//---------------------------------------------------------
//   pullValues
//---------------------------------------------------------

void GateTime::pullValues()
      {
      range     = rangeGroup->checkedId();
      rateVal   = rate->value();
      offsetVal = offset->value();
      }

//---------------------------------------------------------
//   exec
//---------------------------------------------------------

int GateTime::exec()
      {
      rangeGroup->button(range)->setChecked(true);
      rate->setValue(rateVal);
      offset->setValue(offsetVal);
      
      return QDialog::exec();
      }
