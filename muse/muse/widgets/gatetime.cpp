//=============================================================================
//  MusE
//  Linux Music Editor
//  $Id:$
//
//  Copyright (C) 2002-2006 by Werner Schweer and others
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License version 2.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//=============================================================================

#include "gatetime.h"
#include "song.h"
#include "tb1.h"

//---------------------------------------------------------
//   GateTime
//---------------------------------------------------------

GateTime::GateTime(QWidget* parent)
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
      if (rangeGroup->button(id))
            rangeGroup->button(id)->setChecked(true);
      else
            printf("setRange: not button %d!\n", id);
      }

