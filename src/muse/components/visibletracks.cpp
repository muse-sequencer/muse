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
#include "track.h"
#include "synth.h"
#include "app.h"
#include "globals.h"

namespace MusEGui {

static const char* midiTrack = QT_TRANSLATE_NOOP("MusEGui::VisibleTracks", "Show midi tracks");
static const char* waveTrack = QT_TRANSLATE_NOOP("MusEGui::VisibleTracks", "Show wave tracks");
static const char* outputTrack = QT_TRANSLATE_NOOP("MusEGui::VisibleTracks", "Show output tracks");
static const char* groupTrack = QT_TRANSLATE_NOOP("MusEGui::VisibleTracks", "Show group tracks");
static const char* inputTrack = QT_TRANSLATE_NOOP("MusEGui::VisibleTracks", "Show input tracks");
static const char* auxTrack = QT_TRANSLATE_NOOP("MusEGui::VisibleTracks", "Show aux tracks");
static const char* synthTrack = QT_TRANSLATE_NOOP("MusEGui::VisibleTracks", "Show synth tracks");

const QVector<VisibleToolB> visTrackList = {
    {&pianorollSVGIcon,    QT_TRANSLATE_NOOP("MusEGui::VisibleTracks", "Show midi tracks"),     midiTrack   },
    {&waveeditorSVGIcon,   QT_TRANSLATE_NOOP("MusEGui::VisibleTracks", "Show wave tracks"),     waveTrack   },
    {&trackOutputSVGIcon,  QT_TRANSLATE_NOOP("MusEGui::VisibleTracks", "Show output tracks"),   outputTrack },
    {&trackGroupVGIcon,    QT_TRANSLATE_NOOP("MusEGui::VisibleTracks", "Show group tracks"),    groupTrack  },
    {&trackInputSVGIcon,   QT_TRANSLATE_NOOP("MusEGui::VisibleTracks", "Show input tracks"),    inputTrack  },
    {&trackAuxSVGIcon,     QT_TRANSLATE_NOOP("MusEGui::VisibleTracks", "Show aux tracks"),      auxTrack    },
    {&synthSVGIcon,        QT_TRANSLATE_NOOP("MusEGui::VisibleTracks", "Show synth tracks"),    synthTrack   },
};

//---------------------------------------------------------
//   VisibleTracks
//---------------------------------------------------------


VisibleTracks::VisibleTracks(QWidget* parent, const char*)
    : QToolBar(tr("Visible Track Types"), parent)
{
    setObjectName("Visible track types");
    actions = new QActionGroup(parent);  // Parent needed.
    actions->setExclusive(false);

    int i = 0;
    for (const auto& t : visTrackList) {
        QAction* a = new QAction(tr(t.tip).toLatin1().data(), actions);
        a->setData(i++);
        a->setIcon(QIcon(**(t.icon)));
        a->setToolTip(tr(t.tip));
        a->setWhatsThis(tr(t.ltip));
        a->setStatusTip(tr(t.ltip));
        a->setCheckable(true);
        a->setChecked(true);
    }
    actions->setVisible(true);
    //action->addTo(this);
    // Note: Does not take ownership.
    addActions(actions->actions());

    connect(actions, SIGNAL(triggered(QAction*)), SLOT(visibilityChanged(QAction*)));
}


//---------------------------------------------------------
//   updateVisibleTracksButtons
//---------------------------------------------------------
void VisibleTracks::updateVisibleTracksButtons()
{
    actions->actions().at(0)->setChecked(MusECore::MidiTrack::visible());
    actions->actions().at(1)->setChecked(MusECore::WaveTrack::visible());
    actions->actions().at(2)->setChecked(MusECore::AudioOutput::visible());
    actions->actions().at(3)->setChecked(MusECore::AudioGroup::visible());
    actions->actions().at(4)->setChecked(MusECore::AudioInput::visible());
    actions->actions().at(5)->setChecked(MusECore::AudioAux::visible());
    actions->actions().at(6)->setChecked(MusECore::SynthI::visible());
}
//---------------------------------------------------------
//   toolChanged
//---------------------------------------------------------

void VisibleTracks::visibilityChanged(QAction* action)
{
    //      printf("update visibility\n");
    switch (action->data().toInt()) {
    case 0:
        MusECore::MidiTrack::setVisible(action->isChecked());
        break;
    case 1:
        MusECore::WaveTrack::setVisible(action->isChecked());
        break;
    case 2:
        MusECore::AudioOutput::setVisible(action->isChecked());
        break;
    case 3:
        MusECore::AudioGroup::setVisible(action->isChecked());
        break;
    case 4:
        MusECore::AudioInput::setVisible(action->isChecked());
        break;
    case 5:
        MusECore::AudioAux::setVisible(action->isChecked());
        break;
    case 6:
        MusECore::SynthI::setVisible(action->isChecked());
        break;
    default:
        break;
    }
    // Save settings. Use simple version - do NOT set style or stylesheet, this has nothing to do with that.
    MusEGlobal::muse->changeConfig(true);
    emit visibilityChanged();
}

//---------------------------------------------------------
//   ~VisibleTracks
//---------------------------------------------------------

VisibleTracks::~VisibleTracks()
{
}

} // namespace MusEGui
