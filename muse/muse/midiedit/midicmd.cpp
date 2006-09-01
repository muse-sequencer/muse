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
#include "widgets/gatetime.h"
#include "audio.h"

//---------------------------------------------------------
//   processEvents
//---------------------------------------------------------

void MidiCmd::processEvents(CItemList* items)
      {
      modified = 0;
      song->startUndo();
      range = 0; //editor->applyTo();
      process(items, &range);
//      editor->setApplyTo(range);
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

//---------------------------------------------------------
//   ModifyGateTimeCmd
//---------------------------------------------------------

class ModifyGateTimeCmd : public MidiCmd
      {
   protected:
      virtual void process(CItemList* items, int* range);

   public:
      ModifyGateTimeCmd() {}
      };

//---------------------------------------------------------
//   process
//    return true if events are modified
//---------------------------------------------------------

void ModifyGateTimeCmd::process(CItemList* items, int* range)
      {
      GateTime w(0);
      w.setRange(*range);
      if (!w.exec())
            return;
      *range     = w.range();        // all, selected, looped, sel+loop
      int rate   = w.rateVal();
      int offset = w.offsetVal();

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

