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
      }

//---------------------------------------------------------
//   accept
//---------------------------------------------------------

void GateTime::accept()
      {
      _range     = rangeGroup->checkedId();
      _rateVal   = rate->value();
      _offsetVal = offset->value();
      QDialog::accept();
      }

//---------------------------------------------------------
//   setRange
//---------------------------------------------------------

void GateTime::setRange(int id)
      {
	rangeGroup->button(id)->setChecked(true);
      }

