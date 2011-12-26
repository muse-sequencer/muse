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
#include "app.h"

namespace MusEGui {

const char* waveTrack = QT_TRANSLATE_NOOP("MusEGui::VisibleTracks", "Show wave tracks");
const char* groupTrack = QT_TRANSLATE_NOOP("MusEGui::VisibleTracks", "Show group tracks");
const char* auxTrack = QT_TRANSLATE_NOOP("MusEGui::VisibleTracks", "Show aux tracks");
const char* inputTrack = QT_TRANSLATE_NOOP("MusEGui::VisibleTracks", "Show input tracks");
const char* outputTrack = QT_TRANSLATE_NOOP("MusEGui::VisibleTracks", "Show output tracks");
const char* midiTrack = QT_TRANSLATE_NOOP("MusEGui::VisibleTracks", "Show midi tracks");
const char* synthTrack = QT_TRANSLATE_NOOP("MusEGui::VisibleTracks", "Show synth tracks");

VisibleToolB visTrackList[] = {
      {&addtrack_wavetrackIcon,   QT_TRANSLATE_NOOP("MusEGui::VisibleTracks", "Show wave tracks"),     waveTrack   },
      {&addtrack_audiogroupIcon,  QT_TRANSLATE_NOOP("MusEGui::VisibleTracks", "Show group tracks"),    groupTrack  },
      {&addtrack_auxsendIcon,     QT_TRANSLATE_NOOP("MusEGui::VisibleTracks", "Show aux tracks"),      auxTrack    },
      {&addtrack_audioinputIcon,  QT_TRANSLATE_NOOP("MusEGui::VisibleTracks", "Show input tracks"),    inputTrack  },
      {&addtrack_audiooutputIcon, QT_TRANSLATE_NOOP("MusEGui::VisibleTracks", "Show output tracks"),   outputTrack },
      {&addtrack_addmiditrackIcon,QT_TRANSLATE_NOOP("MusEGui::VisibleTracks", "Show midi tracks"),     midiTrack   },
      {&synthIcon,                QT_TRANSLATE_NOOP("MusEGui::VisibleTracks", "Show synth tracks"),    midiTrack   },
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

            Action* a = new Action(action, i, tr(t->tip).toAscii().data(), true);
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

      connect(action, SIGNAL(triggered(QAction*)), SLOT(visibilityChanged(QAction*)));
      }


//---------------------------------------------------------
//   updateVisibleTracksButtons
//---------------------------------------------------------
void VisibleTracks::updateVisibleTracksButtons()
{
    actions[0]->setChecked(MusECore::WaveTrack::visible());
    actions[1]->setChecked(MusECore::AudioGroup::visible());
    actions[2]->setChecked(MusECore::AudioAux::visible());
    actions[3]->setChecked(MusECore::AudioInput::visible());
    actions[4]->setChecked(MusECore::AudioOutput::visible());
    actions[5]->setChecked(MusECore::MidiTrack::visible());
    actions[6]->setChecked(MusECore::SynthI::visible());
}
//---------------------------------------------------------
//   toolChanged
//---------------------------------------------------------

void VisibleTracks::visibilityChanged(QAction* action)
{
//      printf("update visibility\n");
      switch (((Action*)action)->id()) {
          case 0:
            MusECore::WaveTrack::setVisible(action->isChecked());
            break;
          case 1:
            MusECore::AudioGroup::setVisible(action->isChecked());
            break;
          case 2:
            MusECore::AudioAux::setVisible(action->isChecked());
            break;
          case 3:
            MusECore::AudioInput::setVisible(action->isChecked());
            break;
          case 4:
            MusECore::AudioOutput::setVisible(action->isChecked());
            break;
          case 5:
            MusECore::MidiTrack::setVisible(action->isChecked());
            break;
          case 6:
            MusECore::SynthI::setVisible(action->isChecked());
            break;
      default:
            break;
      }
      MusEGlobal::muse->changeConfig(true);    // save settings
      emit visibilityChanged();
}

//---------------------------------------------------------
//   ~VisibleTracks
//---------------------------------------------------------

VisibleTracks::~VisibleTracks()
      {
      delete [] actions;
      }

} // namespace MusEGui
