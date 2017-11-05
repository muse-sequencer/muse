//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: metronome.cpp,v 1.2.2.1 2009/12/20 05:00:35 terminator356 Exp $
//
//  (C) Copyright 2001 Werner Schweer (ws@seh.de)
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

#include <QMenu>
#include <QDir>
#include <QFileInfoList>

#include <stdio.h>
#include "metronome.h"
#include "midi.h"
#include "ticksynth.h"

#include "globals.h"
#include "gconfig.h"
#include "song.h"
#include "track.h"
#include "audio.h"

namespace MusEGui {

//---------------------------------------------------------
//   MetronomeConfig
//---------------------------------------------------------

MetronomeConfig::MetronomeConfig(QDialog* parent)
   : QDialog(parent)
{
      setupUi(this);

      volumeSlider->setValue(MusEGlobal::audioClickVolume*100);
      measVolumeSlider->setValue(MusEGlobal::measClickVolume*100);
      beatVolumeSlider->setValue(MusEGlobal::beatClickVolume*100);
      accent1VolumeSlider->setValue(MusEGlobal::accent1ClickVolume*100);
      accent2VolumeSlider->setValue(MusEGlobal::accent2ClickVolume*100);
      if (MusEGlobal::clickSamples == MusEGlobal::origSamples)
          radioSamples2->setChecked(true);
      else
          radioSamples4->setChecked(true);
      switchSamples(); // to disable gui elements

      fillSoundFiles();

      volumeLabel->setText(QString::number(int(MusEGlobal::audioClickVolume*99)));
      measVolumeLabel->setText(QString::number(int(MusEGlobal::measClickVolume*99)));
      beatVolumeLabel->setText(QString::number(int(MusEGlobal::beatClickVolume*99)));
      accent1VolumeLabel->setText(QString::number(int(MusEGlobal::accent1ClickVolume*99)));
      accent2VolumeLabel->setText(QString::number(int(MusEGlobal::accent2ClickVolume*99)));

      connect(buttonApply, SIGNAL(clicked()), SLOT(apply()));
      connect(midiClick, SIGNAL(toggled(bool)), SLOT(midiClickChanged(bool)));
      connect(precountEnable, SIGNAL(toggled(bool)), SLOT(precountEnableChanged(bool)));
      connect(precountFromMastertrack, SIGNAL(toggled(bool)),
         SLOT(precountFromMastertrackChanged(bool)));
      connect(audioBeepRoutesButton, SIGNAL(clicked()), SLOT(audioBeepRoutesClicked()));
      connect(volumeSlider, SIGNAL(valueChanged(int)), SLOT(volumeChanged(int)));
      connect(measVolumeSlider, SIGNAL(valueChanged(int)), SLOT(measVolumeChanged(int)));
      connect(beatVolumeSlider, SIGNAL(valueChanged(int)), SLOT(beatVolumeChanged(int)));
      connect(accent1VolumeSlider, SIGNAL(valueChanged(int)), SLOT(accent1VolumeChanged(int)));
      connect(accent2VolumeSlider, SIGNAL(valueChanged(int)), SLOT(accent2VolumeChanged(int)));
      connect(radioSamples2, SIGNAL(toggled(bool)),SLOT(switchSamples()));

      measureNote->setValue(MusEGlobal::measureClickNote);
      measureVelocity->setValue(MusEGlobal::measureClickVelo);
      beatNote->setValue(MusEGlobal::beatClickNote);
      beatVelocity->setValue(MusEGlobal::beatClickVelo);
      midiChannel->setValue(MusEGlobal::clickChan+1);
      midiPort->setValue(MusEGlobal::clickPort+1);


      precountBars->setValue(MusEGlobal::preMeasures);
      precountEnable->setChecked(MusEGlobal::precountEnableFlag);
      precountFromMastertrack->setChecked(MusEGlobal::precountFromMastertrackFlag);
      precountSigZ->setValue(MusEGlobal::precountSigZ);
      precountSigN->setValue(MusEGlobal::precountSigN);
      precountPrerecord->setChecked(MusEGlobal::precountPrerecord);
      precountPreroll->setChecked(MusEGlobal::precountPreroll);


      midiClick->setChecked(MusEGlobal::midiClickFlag);
      audioBeep->setChecked(MusEGlobal::audioClickFlag);
}

//---------------------------------------------------------
//    fillSoundFiles
//---------------------------------------------------------

void MetronomeConfig::fillSoundFiles()
{
    QDir metroPath(MusEGlobal::museGlobalShare+"/metronome");
    QStringList filters;
    filters.append("*.wav");
    QStringList klickfiles = metroPath.entryList(filters);

    measSampleCombo->addItems(klickfiles);
    beatSampleCombo->addItems(klickfiles);
    accent1SampleCombo->addItems(klickfiles);
    accent2SampleCombo->addItems(klickfiles);

    measSampleCombo->setCurrentIndex(klickfiles.indexOf(MusEGlobal::config.measSample));
    beatSampleCombo->setCurrentIndex(klickfiles.indexOf(MusEGlobal::config.beatSample));
    accent1SampleCombo->setCurrentIndex(klickfiles.indexOf(MusEGlobal::config.accent1Sample));
    accent2SampleCombo->setCurrentIndex(klickfiles.indexOf(MusEGlobal::config.accent2Sample));
}

//---------------------------------------------------------
//   audioBeepRoutesClicked
//---------------------------------------------------------

void MetronomeConfig::audioBeepRoutesClicked()
{
      if(MusEGlobal::song->outputs()->size() == 0)
        return;
        
      QMenu* pup = new QMenu;
      
      MusECore::OutputList* ol = MusEGlobal::song->outputs();

      int nn = 0;
      for(MusECore::iAudioOutput iao = ol->begin(); iao != ol->end(); ++iao)
      {
        QAction* action = pup->addAction((*iao)->name());
        action->setCheckable(true);
        action->setData(nn);
        if((*iao)->sendMetronome())
          action->setChecked(true);
        ++nn;  
      }  
      
      QAction* clickaction = pup->exec(QCursor::pos());
      if (clickaction)
      {
        nn = 0;
        for(MusECore::iAudioOutput iao = ol->begin(); iao != ol->end(); ++iao)
        {
          if (nn == clickaction->data())
          {
            MusEGlobal::audio->msgSetSendMetronome(*iao, clickaction->isChecked());
            break;
          }
          ++nn;
        }
      }
      
      delete pup;
      audioBeepRoutesButton->setDown(false);     // pup->exec() catches mouse release event
}

//---------------------------------------------------------
//   accept
//---------------------------------------------------------

void MetronomeConfig::accept()
      {
      apply();
      QDialog::accept();
      }

//---------------------------------------------------------
//   apply
//---------------------------------------------------------

void MetronomeConfig::apply()
{
      MusEGlobal::measureClickNote   = measureNote->value();
      MusEGlobal::measureClickVelo   = measureVelocity->value();
      MusEGlobal::beatClickNote      = beatNote->value();
      MusEGlobal::beatClickVelo      = beatVelocity->value();
      MusEGlobal::clickChan          = midiChannel->value() - 1;
      MusEGlobal::clickPort          = midiPort->value() - 1;
      MusEGlobal::preMeasures        = precountBars->value();

      MusEGlobal::precountEnableFlag = precountEnable->isChecked();
      MusEGlobal::precountFromMastertrackFlag = precountFromMastertrack->isChecked();
      MusEGlobal::precountSigZ     = precountSigZ->value();
      MusEGlobal::precountSigN     = precountSigN->value();
      MusEGlobal::precountPrerecord = precountPrerecord->isChecked();
      MusEGlobal::precountPreroll  = precountPreroll->isChecked();

      MusEGlobal::midiClickFlag      = midiClick->isChecked();
      MusEGlobal::audioClickFlag     = audioBeep->isChecked();

      MusEGlobal::config.measSample = measSampleCombo->currentText();
      MusEGlobal::config.beatSample = beatSampleCombo->currentText();
      MusEGlobal::config.accent1Sample = accent1SampleCombo->currentText();
      MusEGlobal::config.accent2Sample = accent2SampleCombo->currentText();

      MusECore::MidiPlayEvent ev(0, MusEGlobal::clickPort, MusEGlobal::clickChan, MusECore::ME_NOTEON, MusEGlobal::beatClickNote, MusEGlobal::beatClickVelo);
      ev.setA(MusECore::reloadClickSounds);
// REMOVE Tim. autoconnect. Changed.
//       MusECore::metronome->addScheduledEvent(ev);
      MusECore::metronome->putEvent(ev, MusECore::MidiDevice::NotLate);

}

//---------------------------------------------------------
//   reject
//---------------------------------------------------------

void MetronomeConfig::reject()
      {
      QDialog::reject();
      }

//---------------------------------------------------------
//   midiClickChanged
//---------------------------------------------------------

void MetronomeConfig::midiClickChanged(bool flag)
      {
      measureNote->setEnabled(flag);
      measureVelocity->setEnabled(flag);
      beatNote->setEnabled(flag);
      beatVelocity->setEnabled(flag);
      midiChannel->setEnabled(flag);
      midiPort->setEnabled(flag);
      }

void MetronomeConfig::precountEnableChanged(bool flag)
      {
      precountBars->setEnabled(flag);
      precountFromMastertrack->setEnabled(flag);
      precountSigZ->setEnabled(flag && !precountFromMastertrack->isChecked());
      precountSigN->setEnabled(flag && !precountFromMastertrack->isChecked());

      }

void MetronomeConfig::precountFromMastertrackChanged(bool flag)
      {

      precountSigZ->setEnabled(!flag);
      precountSigN->setEnabled(!flag);

      }

// these values are directly applied, not using th Apply button, it just seems more usable this way.
void MetronomeConfig::volumeChanged(int volume) {
      MusEGlobal::audioClickVolume=volume/100.0;
      volumeLabel->setText(QString::number(int(MusEGlobal::audioClickVolume*99)));
}
void MetronomeConfig::measVolumeChanged(int volume) {
      MusEGlobal::measClickVolume=volume/100.0;
      measVolumeLabel->setText(QString::number(int(MusEGlobal::measClickVolume*99)));
}
void MetronomeConfig::beatVolumeChanged(int volume) {
      MusEGlobal::beatClickVolume=volume/100.0;
      beatVolumeLabel->setText(QString::number(int(MusEGlobal::beatClickVolume*99)));
}
void MetronomeConfig::accent1VolumeChanged(int volume) {
      MusEGlobal::accent1ClickVolume=volume/100.0;
      accent1VolumeLabel->setText(QString::number(int(MusEGlobal::accent1ClickVolume*99)));
}
void MetronomeConfig::accent2VolumeChanged(int volume) {
      MusEGlobal::accent2ClickVolume=volume/100.0;
      accent2VolumeLabel->setText(QString::number(int(MusEGlobal::accent2ClickVolume*99)));
}

void MetronomeConfig::switchSamples() {
    if (radioSamples2->isChecked()) {
        MusEGlobal::clickSamples = MusEGlobal::origSamples;
        accent1VolumeSlider->setDisabled(true);
        accent2VolumeSlider->setDisabled(true);
    }
    else {
        MusEGlobal::clickSamples = MusEGlobal::newSamples;
        accent1VolumeSlider->setDisabled(false);
        accent2VolumeSlider->setDisabled(false);
       }
}

} // namespace MusEGui

