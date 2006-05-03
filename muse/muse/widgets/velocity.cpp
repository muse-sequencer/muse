//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: velocity.cpp,v 1.5 2006/01/25 16:24:33 wschweer Exp $
//  (C) Copyright 2001 Werner Schweer (ws@seh.de)
//=========================================================

#include "velocity.h"
#include "song.h"
#include "tb1.h"

//---------------------------------------------------------
//   Velocity
//---------------------------------------------------------

Velocity::Velocity(QWidget* parent)
   : QDialog(parent)
      {
      setupUi(this);
      rangeGroup = new QButtonGroup(this);
      rangeGroup->setExclusive(true);
      rangeGroup->addButton(allEventsButton, RANGE_ALL);
      rangeGroup->addButton(selectedEventsButton, RANGE_SELECTED);
      rangeGroup->addButton(loopedEventsButton, RANGE_LOOPED);
      rangeGroup->addButton(selectedLoopedButton, RANGE_SELECTED | RANGE_LOOPED);
      allEventsButton->setChecked(true);
      }

//---------------------------------------------------------
//   accept
//---------------------------------------------------------

void Velocity::accept()
      {
      _range     = rangeGroup->checkedId();
      _rateVal   = rate->value();
      _offsetVal = offset->value();
      QDialog::accept();
      }

//---------------------------------------------------------
//   setRange
//---------------------------------------------------------

void Velocity::setRange(int id)
      {
      if (rangeGroup->button(id))
            rangeGroup->button(id)->setChecked(true);
      else
            printf("setRange: not button %d!\n", id);
      }

