
#include <stdio.h>
#include <qcombobox.h>
#include <qspinbox.h>
#include <q3buttongroup.h>
#include <qradiobutton.h>
#include "transpose.h"
#include "track.h"
#include "song.h"
#include "event.h"
#include "audio.h"

//---------------------------------------------------------
//   Transpose
//---------------------------------------------------------

Transpose::Transpose(QWidget* parent, const char* name)
   : TransposeDialogBase(parent, name)
      {
      if (song->lpos() != song->rpos()) {
            time_selected->setChecked(true);
            }
      else {
//            time_all->setChecked(true);
            ButtonGroup1->setEnabled(false);
            }
//      parts_all->setSelected(true);
      }

//---------------------------------------------------------
//   accept
//---------------------------------------------------------

void Transpose::accept()
      {
      int left = 0, right = 0;
      int dv = delta->value();

      TrackList *tracks = song->tracks();

      if (time_selected->isChecked()) {
            left  = song->lpos();
            right = song->rpos();
            }
      else {
            left  = 0;
            right = song->len();
            }

      std::vector< EventList* > doneList;
      typedef std::vector< EventList* >::iterator iDoneList;
  
      song->startUndo();
      for (iTrack t = tracks->begin(); t != tracks->end(); ++t) {
//         if (((*t)->type() == Track::MIDI || (*t)->type() == Track::DRUM)
            if (((*t)->type() != Track::MIDI)
               || !(parts_all->isChecked() || (*t)->selected()))
                  continue;

            PartList *pl = (*t)->parts();
            for (iPart p = pl->begin(); p != pl->end(); ++p) {
                  MidiPart *mp = (MidiPart *) p->second;
                  EventList* el = mp->events();
                  
                  // Check if the event list has already been done. Skip repeated clones.
                  iDoneList idl;
                  for(idl = doneList.begin(); idl != doneList.end(); ++idl)
                    if(*idl == el)
                      break;
                  if(idl != doneList.end())
                    break;
                  doneList.push_back(el);
                  
                  for (iEvent i = el->begin(); i != el->end(); ++i) {
                        Event oe = i->second;
                        int tick = oe.tick();
                        if (tick > right)
                              break;
                        if (tick < left)
                              continue;
                        Event ne = oe.clone();
                        ne.setA(oe.dataA() + dv );
                        // Indicate no undo, and do not do port controller values and clone parts. 
                        //audio->msgChangeEvent(oe, ne, mp, false);
                        audio->msgChangeEvent(oe, ne, mp, false, false, false);
                        }
                  }
            }
      song->endUndo(SC_EVENT_MODIFIED);
      close(true);
      }

