//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: gatetime.cpp,v 1.1.1.1 2003/10/27 18:54:37 wschweer Exp $
//  (C) Copyright 2001 Werner Schweer (ws@seh.de)
//=========================================================

#include <q3buttongroup.h>
#include <qspinbox.h>
//#include <qbutton.h>
#include <QtGui>

#include "gatetime.h"

#include "song.h"

//---------------------------------------------------------
//   GateTime
//---------------------------------------------------------

GateTime::GateTime(QWidget* parent, const char* name)
   : GateTimeBase(parent, name, true)
      {
      }

//---------------------------------------------------------
//   accept
//---------------------------------------------------------

void GateTime::accept()
      {
      _range     = rangeGroup->id(rangeGroup->selected());
      _rateVal   = rate->value();
      _offsetVal = offset->value();
      GateTimeBase::accept();
      }

//---------------------------------------------------------
//   setRange
//---------------------------------------------------------

void GateTime::setRange(int id)
      {
      rangeGroup->setButton(id);
      }

