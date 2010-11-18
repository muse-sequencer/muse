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

Velocity::Velocity(QDialog* parent)
   : VelocityBaseWidget(parent)
      {
      rangeGroup = new QButtonGroup;
      rangeGroup->addButton(allEvents,0);
      rangeGroup->addButton(selectedEvents,1);
      rangeGroup->addButton(loopedEvents,2);
      rangeGroup->addButton(selectedLooped,3);
      }

//---------------------------------------------------------
//   accept
//---------------------------------------------------------

void Velocity::accept()
      {
      _range     = rangeGroup->checkedId();
      _rateVal   = rate->value();
      _offsetVal = offset->value();
      VelocityBaseWidget::accept();
      }

//---------------------------------------------------------
//   setRange
//---------------------------------------------------------

void Velocity::setRange(int id)
      {
      rangeGroup->button(id)->setChecked(true);
      }

