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

#include "definemidictrl.h"
#include "miditrack.h"
#include "instruments/minstrument.h"

//---------------------------------------------------------
//   DefineMidiCtrlDialog
//    controllerName                      QLineEdit
//    controllerType                      QComboBox
//    msbId lsbId minVal maxVal initVal   QSpinBox
//    moveWithPart                        QCheckBox
//---------------------------------------------------------

DefineMidiCtrl::DefineMidiCtrl(MidiTrack* t, QWidget* parent)
   : QDialog(parent)
      {
      setupUi(this);
      track = t;
      connect(controllerType, SIGNAL(currentIndexChanged(int)), SLOT(typeChanged(int)));
      }

//---------------------------------------------------------
//   done
//    val == 1  -> OK
//---------------------------------------------------------

void DefineMidiCtrl::done(int val)
      {
      if (val) {
            ctrl.setName(controllerName->text());
            ctrl.setComment(controllerComment->toPlainText());
            int num = MidiController::genNum(
               MidiController::ControllerType(controllerType->currentIndex()),
               msbId->value(), lsbId->value());
            ctrl.setNum(num);
            ctrl.setMinVal(minVal->value());
            ctrl.setMaxVal(maxVal->value());
            ctrl.setInitVal(initVal->value());
            ctrl.setMoveWithPart(moveWithPart->isChecked());

            //
            // add controller to instrument
            //
            MidiInstrument* instrument = track->instrument();
            MidiControllerList* mcl = instrument->controller();
            MidiController* c = new MidiController(ctrl);
            mcl->append(c);
            }

      QDialog::done(val);
      }

//---------------------------------------------------------
//   typeChanged
//---------------------------------------------------------

void DefineMidiCtrl::typeChanged(int val)
      {
      MidiController::ControllerType t = (MidiController::ControllerType)val;
      switch (t) {
            case MidiController::RPN:
            case MidiController::NRPN:
            case MidiController::Controller7:
                  msbId->setEnabled(false);
                  lsbId->setEnabled(true);
                  maxVal->setRange(0, 127);
                  maxVal->setValue(127);
                  initVal->setRange(0, 127);
                  break;
            case MidiController::Controller14:
            case MidiController::RPN14:
            case MidiController::NRPN14:
                  msbId->setEnabled(true);
                  lsbId->setEnabled(true);
                  maxVal->setRange(0, 128*128-1);
                  maxVal->setValue(128*128-1);
                  initVal->setRange(0, 128*128-1);
                  break;
            case MidiController::Pitch:
            case MidiController::Program:
                  msbId->setEnabled(false);
                  lsbId->setEnabled(false);
                  maxVal->setRange(0,  128*128-1);
                  initVal->setRange(0, 128*128-1);
                  maxVal->setValue(128*128-1);
                  break;
            default:
                  break;
            }
      }

