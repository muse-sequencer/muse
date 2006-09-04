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

#include "velocity.h"
#include "song.h"
#include "tb1.h"
#include "audio.h"

//---------------------------------------------------------
//   Velocity
//---------------------------------------------------------

Velocity::Velocity(QWidget*)
   : MidiCmdDialog()
      {
      setWindowTitle(tr("MusE: Modify Velocity"));
      QWidget* velocityWidget = new QWidget;
      velo.setupUi(velocityWidget);
      layout->addWidget(velocityWidget);
      layout->addStretch(10);
      _rateVal   = 0;
      _offsetVal = 0;
      velo.rate->setValue(_rateVal);
      velo.offset->setValue(_offsetVal);
      }

//---------------------------------------------------------
//   accept
//---------------------------------------------------------

void Velocity::accept()
      {
      _rateVal   = velo.rate->value();
      _offsetVal = velo.offset->value();
      MidiCmdDialog::accept();
      }

//---------------------------------------------------------
//   ModifyVelocityCmd
//---------------------------------------------------------

ModifyVelocityCmd::ModifyVelocityCmd(MidiEditor* e)
   : MidiCmd(e)
      {
      dialog = 0;      
      }

//---------------------------------------------------------
//   guiDialog
//---------------------------------------------------------

MidiCmdDialog* ModifyVelocityCmd::guiDialog()
      {
      if (dialog == 0)
            dialog = new Velocity(0);
      return dialog;
      }

//---------------------------------------------------------
//   process
//---------------------------------------------------------

void ModifyVelocityCmd::process(CItemList* items)
      {
      int rate   = dialog->rateVal();
      int offset = dialog->offsetVal();

      for (iCItem k = items->begin(); k != items->end(); ++k) {
            CItem* item = k->second;
            Event event = item->event;
            if (event.type() != Note)
                  continue;
            if (itemInRange(item)) {
                  int velo = event.velo();
                  velo = (velo * rate) / 100;
                  velo += offset;

                  if (velo <= 0)
                        velo = 1;
                  if (velo > 127)
                        velo = 127;
                  if (event.velo() != velo) {
                        Event newEvent = event.clone();
                        newEvent.setVelo(velo);
                        audio->msgChangeEvent(event, newEvent, item->part, false);
                        }
                  }
            }
      }


