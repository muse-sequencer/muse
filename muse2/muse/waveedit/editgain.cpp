//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: ./muse/waveedit/editgain.cpp $
//
//  Copyright (C) 1999-2011 by Werner Schweer and others
//
// Author: Mathias Lundgren <lunar_shuttle@users.sf.net>, (C) 2005
//
// Copyright: See COPYING file that comes with this distribution
//
//
//  This program is free software; you can redistribute it and/or
//  modify it under the terms of the GNU General Public License
//  as published by the Free Software Foundation; version 2 of
//  the License, or (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
//
//=========================================================
//
// C++ Implementation: editgain
//
// Description:
//
//

#include <QDialog>

#include "editgain.h"

namespace MusEGui {

EditGain::EditGain(QWidget* parent, int initGainValue)
 : QDialog(parent)
      {
         setupUi(this);
         sliderGain->setValue(sliderGain->maximum() - initGainValue);
         connect(buttonReset, SIGNAL(pressed()), this, SLOT(resetPressed()));
         connect(buttonApply, SIGNAL(pressed()), this, SLOT(applyPressed()));
         connect(buttonCancel,SIGNAL(pressed()), this, SLOT(cancelPressed()));
         connect(sliderGain,  SIGNAL(valueChanged(int)), this, SLOT(gainChanged(int)));
         if (sliderGain->value() != 100)
               buttonReset->setEnabled(true);
      }


EditGain::~EditGain()
      {
      }


/*!
    \fn EditGain::resetPressed
 */
void EditGain::resetPressed()
      {
      sliderGain->blockSignals(true);
      sliderGain->setValue(100);
      sliderGain->blockSignals(false);
      buttonReset->setEnabled(false);
      buttonApply->setEnabled(false);
      }


/*!
    \fn EditGain::applyPressed()
 */
void EditGain::applyPressed()
      {
      done(QDialog::Accepted);
      }


/*!
    \fn EditGain::cancelPressed()
 */
void EditGain::cancelPressed()
      {
      done(QDialog::Rejected);
      }



/*!
    \fn EditGain::gainChanged(int value)
 */
void EditGain::gainChanged(int value)
      {
      gain = sliderGain->maximum() - value;
      if (sliderGain->value() != 100) {
            buttonReset->setEnabled(true);
            buttonApply->setEnabled(true);
            }
      else {
            buttonReset->setEnabled(false);
            buttonApply->setEnabled(false);
            }
      }


/*!
    \fn EditGain::getGain()
 */
int EditGain::getGain()
      {
      return gain;
      }

} // namespace MusEGui
