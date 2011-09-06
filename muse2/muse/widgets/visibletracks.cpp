//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: tools.cpp,v 1.2 2004/04/28 21:56:13 spamatica Exp $
//  (C) Copyright 1999 Werner Schweer (ws@seh.de)
//  (C) Copyright 2011 Robert Jonsson (rj@spamatica.se)
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

#include <QActionGroup>

#include <stdio.h>
#include "visibletracks.h"
#include "icons.h"
#include "action.h"
#include "track.h"
#include "synth.h"

const char* waveTrack = QT_TRANSLATE_NOOP("@default", "Show wave tracks");
const char* groupTrack = QT_TRANSLATE_NOOP("@default", "Show group tracks");
const char* auxTrack = QT_TRANSLATE_NOOP("@default", "Show aux tracks");
const char* inputTrack = QT_TRANSLATE_NOOP("@default", "Show input tracks");
const char* outputTrack = QT_TRANSLATE_NOOP("@default", "Show output tracks");
const char* midiTrack = QT_TRANSLATE_NOOP("@default", "Show midi tracks");
const char* synthTrack = QT_TRANSLATE_NOOP("@default", "Show synth tracks");

VisibleToolB visTrackList[] = {
      {&addtrack_wavetrackIcon,   QT_TRANSLATE_NOOP("@default", "Show wave tracks"),     waveTrack   },
      {&addtrack_audiogroupIcon,  QT_TRANSLATE_NOOP("@default", "Show group tracks"),    groupTrack  },
      {&addtrack_auxsendIcon,     QT_TRANSLATE_NOOP("@default", "Show aux tracks"),      auxTrack    },
      {&addtrack_audioinputIcon,  QT_TRANSLATE_NOOP("@default", "Show input tracks"),    inputTrack  },
      {&addtrack_audiooutputIcon, QT_TRANSLATE_NOOP("@default", "Show output tracks"),   outputTrack },
      {&addtrack_addmiditrackIcon,QT_TRANSLATE_NOOP("@default", "Show midi tracks"),     midiTrack   },
      {&synthIcon,                QT_TRANSLATE_NOOP("@default", "Show synth tracks"),    midiTrack   },
      };

//---------------------------------------------------------
//   VisibleTracks
//---------------------------------------------------------


VisibleTracks::VisibleTracks(QWidget* parent, const char*)
   : QToolBar(tr("Visible track types"), parent)
      {
      setObjectName("Visible track types");
      QActionGroup* action = new QActionGroup(parent);  // Parent needed.
      action->setExclusive(false);

      actions = new Action*[sizeof(visTrackList)];
      int n = 0;
      for (unsigned i = 0; i < sizeof(visTrackList)/sizeof(*visTrackList); ++i) {
            VisibleToolB* t = &visTrackList[i];

            Action* a = new Action(action, i, t->tip, true);
            actions[n] = a;
            //a->setIconSet(QIcon(**(t->icon)));
            a->setIcon(QIcon(**(t->icon)));
            a->setToolTip(tr(t->tip));
            a->setWhatsThis(tr(t->ltip));
            a->setChecked(true);
            ++n;
            }
      action->setVisible(true);
      //action->addTo(this);
      // Note: Does not take ownership.
      addActions(action->actions());

      connect(action, SIGNAL(selected(QAction*)), SLOT(visibilityChanged(QAction*)));
      }


//---------------------------------------------------------
//   updateVisibleTracksButtons
//---------------------------------------------------------
void VisibleTracks::updateVisibleTracksButtons()
{
    actions[0]->setChecked(WaveTrack::visible());
    actions[1]->setChecked(AudioGroup::visible());
    actions[2]->setChecked(AudioAux::visible());
    actions[3]->setChecked(AudioInput::visible());
    actions[4]->setChecked(AudioOutput::visible());
    actions[5]->setChecked(MidiTrack::visible());
    actions[6]->setChecked(SynthI::visible());
}
//---------------------------------------------------------
//   toolChanged
//---------------------------------------------------------

void VisibleTracks::visibilityChanged(QAction* action)
{
//      printf("update visibility\n");
      switch (((Action*)action)->id()) {
          case 0:
            WaveTrack::setVisible(action->isChecked());
            break;
          case 1:
            AudioGroup::setVisible(action->isChecked());
            break;
          case 2:
            AudioAux::setVisible(action->isChecked());
            break;
          case 3:
            AudioInput::setVisible(action->isChecked());
            break;
          case 4:
            AudioOutput::setVisible(action->isChecked());
            break;
          case 5:
            MidiTrack::setVisible(action->isChecked());
            break;
          case 6:
            SynthI::setVisible(action->isChecked());
            break;
      default:
            break;
      }
      emit visibilityChanged();
}

//---------------------------------------------------------
//   ~VisibleTracks
//---------------------------------------------------------

VisibleTracks::~VisibleTracks()
      {
      delete actions;
      }

