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

#include "midicmd.h"
#include "song.h"
#include "gatetime.h"
#include "velocity.h"
#include "audio.h"
#include "midieditor.h"

//==========================================================
//     class MidiCmd is an attempt to formalize the 
//     interface between midi command code and MusE.
//     Maybe this can be extended to an specialized
//     plugin interface
//==========================================================

//---------------------------------------------------------
//   MidiCmdDialog
//---------------------------------------------------------

MidiCmdDialog::MidiCmdDialog()
   : QDialog(0)
      {
      }

//---------------------------------------------------------
//   MidiCmd
//---------------------------------------------------------

MidiCmd::MidiCmd(MidiEditor* e)
      {
      editor = e;      
      }

//---------------------------------------------------------
//   processEvents
//---------------------------------------------------------

void MidiCmd::processEvents(CItemList* items)
      {
      MidiCmdDialog* dialog = guiDialog();
      editor->applyTo();
      dialog->setRange(range);
      bool rv = dialog->exec();
      if (!rv) {
            delete dialog;
            return;
            }
      range = dialog->range();        // all, selected, looped, sel+loop
      editor->setApplyTo(range);
      delete dialog;
      
      modified = 0;
      song->startUndo();
      process(items);
      song->endUndo(modified);
      }

//---------------------------------------------------------
//   eventInRange
//---------------------------------------------------------

bool MidiCmd::itemInRange(CItem* item)
      {
      unsigned tick = item->event.tick();
      bool selected = item->isSelected();
      bool inLoop   = (tick >= song->lpos()) && (tick < song->rpos());
      return (
            (range == 0)
         || (range == 1 && selected)
         || (range == 2 && inLoop)
         || (range == 3 && selected && inLoop)
         );
      }

//---------------------------------------------------------
//   changeEvent
//---------------------------------------------------------

void MidiCmd::changeEvent(Event oldEvent, Event newEvent, Part* part)
      {
      audio->msgChangeEvent(oldEvent, newEvent, part, false);
      modified = SC_EVENT_MODIFIED;
      }

//==================================================================
//    process midi event lists
//==================================================================

//---------------------------------------------------------
//   ModifyGateTimeCmd
//---------------------------------------------------------

class ModifyGateTimeCmd : public MidiCmd
      {
      GateTime* dialog;
      virtual MidiCmdDialog* guiDialog();
      virtual void process(CItemList* items);

   public:
      ModifyGateTimeCmd(MidiEditor* e);
      };

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

//---------------------------------------------------------
//   ModifyVelocityCmd
//---------------------------------------------------------

class ModifyVelocityCmd : public MidiCmd
      {
      Velocity* dialog;
      virtual MidiCmdDialog* guiDialog();
      virtual void process(CItemList* items);

   public:
      ModifyVelocityCmd(MidiEditor* e);
      };

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

