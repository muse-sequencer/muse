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
#include "widgets/tb1.h"

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
   : QDialog()
      {
      QWidget* rangeWidget = new QWidget;
      mc.setupUi(rangeWidget);
      layout = new QVBoxLayout;
      setLayout(layout);
      layout->addWidget(rangeWidget);
      rangeGroup = new QButtonGroup(this);
      rangeGroup->setExclusive(true);
      rangeGroup->addButton(mc.allEventsButton,      RANGE_ALL);
      rangeGroup->addButton(mc.selectedEventsButton, RANGE_SELECTED);
      rangeGroup->addButton(mc.loopedEventsButton,   RANGE_LOOPED);
      rangeGroup->addButton(mc.selectedLoopedButton, RANGE_SELECTED | RANGE_LOOPED);
      mc.allEventsButton->setChecked(true);
      }

//---------------------------------------------------------
//   setRange
//---------------------------------------------------------

void MidiCmdDialog::setRange(int id)
      {
      if (rangeGroup->button(id))
            rangeGroup->button(id)->setChecked(true);
      else
            printf("setRange: not button %d!\n", id);
      }

//---------------------------------------------------------
//   accept
//---------------------------------------------------------

void MidiCmdDialog::accept()
      {
      _range = rangeGroup->checkedId();
      QDialog::accept();
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
      range = editor->applyTo();
      dialog->setRange(range);
      bool rv = dialog->exec();
      if (!rv)
            return;
      range = dialog->range();        // all, selected, looped, sel+loop
      editor->setApplyTo(range);
      
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

