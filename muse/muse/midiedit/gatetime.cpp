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

GateTime::GateTime(QWidget*)
   : MidiCmdDialog()
      {
      setWindowTitle(tr("MusE: Modify Gate Time"));
      QWidget* gateTime = new QWidget;
      gt.setupUi(gateTime);
      layout->addWidget(gateTime);
      layout->addStretch(10);
      _rateVal = 0;
      _offsetVal = 0;
      gt.rate->setValue(_rateVal);
      gt.offset->setValue(_offsetVal);
      }

//---------------------------------------------------------
//   accept
//---------------------------------------------------------

void GateTime::accept()
      {
      _rateVal   = gt.rate->value();
      _offsetVal = gt.offset->value();
      MidiCmdDialog::accept();
      }

//---------------------------------------------------------
//   MidifyGateTime
//---------------------------------------------------------

ModifyGateTimeCmd::ModifyGateTimeCmd(MidiEditor* e)
   : MidiCmd(e)
      {
      dialog = 0;      
      }

//---------------------------------------------------------
//   guiDialog
//---------------------------------------------------------

MidiCmdDialog* ModifyGateTimeCmd::guiDialog()
      {
      if (dialog == 0)
            dialog = new GateTime(0);
      return dialog;
      }

//---------------------------------------------------------
//   process
//---------------------------------------------------------

void ModifyGateTimeCmd::process(CItemList* items)
      {
      int rate   = dialog->rateVal();
      int offset = dialog->offsetVal();

      for (iCItem k = items->begin(); k != items->end(); ++k) {
            CItem* item = k->second;
            Event event = item->event;
            if (event.type() != Note)
                  continue;

            if (itemInRange(item)) {
                  unsigned len   = event.lenTick();
                  len = rate ? (len * 100) / rate : 1;
                  len += offset;
                  if (len <= 1)
                        len = 1;

                  if (event.lenTick() != len) {
                        Event newEvent = event.clone();
                        newEvent.setLenTick(len);
                        changeEvent(event, newEvent, item->part);
                        }
                  }
            }
      }

