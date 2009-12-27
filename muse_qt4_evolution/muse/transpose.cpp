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

#include "transpose.h"
#include "track.h"
#include "song.h"
#include "event.h"
#include "audio.h"
#include "part.h"

//---------------------------------------------------------
//   Transpose
//---------------------------------------------------------

Transpose::Transpose(QWidget* parent)
   : QDialog(parent)
      {
      setupUi(this);
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

      song->startUndo();
      for (iTrack t = tracks->begin(); t != tracks->end(); ++t) {
            if (((*t)->type() != Track::MIDI)
               || !(parts_all->isChecked() || (*t)->selected()))
                  continue;

            PartList *pl = (*t)->parts();
            for (iPart p = pl->begin(); p != pl->end(); ++p) {
                  Part *mp = p->second;
                  EventList* el = mp->events();
                  for (iEvent i = el->begin(); i != el->end(); ++i) {
                        Event oe = i->second;
                        int tick = oe.tick();
                        if (tick > right)
                              break;
                        if (tick < left)
                              continue;
                        Event ne = oe.clone();
                        ne.setA(oe.dataA() + dv );
                        audio->msgChangeEvent(oe, ne, mp, false);
                        }
                  }
            }
      song->endUndo(SC_EVENT_MODIFIED);
      close();
      }

