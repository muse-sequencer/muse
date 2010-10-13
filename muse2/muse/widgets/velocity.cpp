//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: velocity.cpp,v 1.1.1.1 2003/10/27 18:55:04 wschweer Exp $
//  (C) Copyright 2001 Werner Schweer (ws@seh.de)
//=========================================================

#include <q3buttongroup.h>
#include <qspinbox.h>
//#include <qbutton.h>
#include <QtGui>

#include "velocity.h"

#include "song.h"

//---------------------------------------------------------
//   Velocity
//---------------------------------------------------------

Velocity::Velocity(QWidget* parent, const char* name)
   : VelocityBase(parent, name, true)
      {
      }

//---------------------------------------------------------
//   accept
//---------------------------------------------------------

void Velocity::accept()
      {
      _range     = rangeGroup->id(rangeGroup->selected());
      _rateVal   = rate->value();
      _offsetVal = offset->value();
      VelocityBase::accept();
      }

//---------------------------------------------------------
//   setRange
//---------------------------------------------------------

void Velocity::setRange(int id)
      {
      rangeGroup->setButton(id);
      }

