//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: genset.cpp,v 1.7.2.8 2009/12/01 03:52:40 terminator356 Exp $
//
//  (C) Copyright 2001-2004 Werner Schweer (ws@seh.de)
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

#include <stdio.h>

#include <QFileDialog>
#include <QRect>
#include <QShowEvent>

#include "genset.h"
#include "app.h"
#include "gconfig.h"
#include "midiseq.h"
#include "globals.h"
#include "icons.h"

static int rtcResolutions[] = {
      1024, 2048, 4096, 8192, 16384, 32768
      };
static int divisions[] = {
      48, 96, 192, 384, 768, 1536, 3072, 6144, 12288
      };
static int dummyAudioBufSizes[] = {
      16, 32, 64, 128, 256, 512, 1024, 2048
      };
static unsigned long minControlProcessPeriods[] = {
      1, 2, 4, 8, 16, 32, 64, 128, 256, 512, 1024, 2048
      };

//---------------------------------------------------------
//   GlobalSettingsConfig
//---------------------------------------------------------

GlobalSettingsConfig::GlobalSettingsConfig(QWidget* parent)
   : QDialog(parent)
      {
      setupUi(this);
      startSongGroup = new QButtonGroup(this);
      startSongGroup->addButton(startLastButton, 0);
      startSongGroup->addButton(startEmptyButton, 1);
      startSongGroup->addButton(startSongButton, 2);
      for (unsigned i = 0; i < sizeof(rtcResolutions)/sizeof(*rtcResolutions); ++i) {
            if (rtcResolutions[i] == MusEConfig::config.rtcTicks) {
                  rtcResolutionSelect->setCurrentIndex(i);
                  break;
                  }
            }
      for (unsigned i = 0; i < sizeof(divisions)/sizeof(*divisions); ++i) {
            if (divisions[i] == MusEConfig::config.division) {
                  midiDivisionSelect->setCurrentIndex(i);
                  break;
                  }
            }
      for (unsigned i = 0; i < sizeof(divisions)/sizeof(*divisions); ++i) {
            if (divisions[i] == MusEConfig::config.guiDivision) {
                  guiDivisionSelect->setCurrentIndex(i);
                  break;
                  }
            }
      for (unsigned i = 0; i < sizeof(dummyAudioBufSizes)/sizeof(*dummyAudioBufSizes); ++i) {
            if (dummyAudioBufSizes[i] == MusEConfig::config.dummyAudioBufSize) {
                  dummyAudioSize->setCurrentIndex(i);
                  break;
                  }
            }

      for (unsigned i = 0; i < sizeof(minControlProcessPeriods)/sizeof(*minControlProcessPeriods); ++i) {
            if (minControlProcessPeriods[i] == MusEConfig::config.minControlProcessPeriod) {
                  minControlProcessPeriodComboBox->setCurrentIndex(i);
                  break;
                  }
            }

      userInstrumentsPath->setText(MusEConfig::config.userInstrumentsDir);
      selectInstrumentsDirButton->setIcon(*openIcon);
      defaultInstrumentsDirButton->setIcon(*undoIcon);
      connect(selectInstrumentsDirButton, SIGNAL(clicked()), SLOT(selectInstrumentsPath()));
      connect(defaultInstrumentsDirButton, SIGNAL(clicked()), SLOT(defaultInstrumentsPath()));

      guiRefreshSelect->setValue(MusEConfig::config.guiRefresh);
      minSliderSelect->setValue(int(MusEConfig::config.minSlider));
      minMeterSelect->setValue(MusEConfig::config.minMeter);
      freewheelCheckBox->setChecked(MusEConfig::config.freewheelMode);
      denormalCheckBox->setChecked(MusEConfig::config.useDenormalBias);
      outputLimiterCheckBox->setChecked(MusEConfig::config.useOutputLimiter);
      vstInPlaceCheckBox->setChecked(MusEConfig::config.vstInPlace);
      dummyAudioRate->setValue(MusEConfig::config.dummyAudioSampleRate);
      
      //DummyAudioDevice* dad = dynamic_cast<DummyAudioDevice*>(audioDevice);
      //dummyAudioRealRate->setText(dad ? QString().setNum(sampleRate) : "---");
      //dummyAudioRealRate->setText(QString().setNum(sampleRate));  // Not used any more. p4.0.20 
      // Just a record of what the gensetbase.ui file contained for dummyAudioRate whats this:
      /*  <property name="whatsThis">
             <string>Actual rate used depends on limitations of
 timer used. If a high rate timer is available,
 short periods can be used with high sample rates. 
Period affects midi playback resolution. 
Shorter periods are desirable.</string>
            </property>                       */
      
      startSongEntry->setText(MusEConfig::config.startSong);
      startSongGroup->button(MusEConfig::config.startMode)->setChecked(true);

      showTransport->setChecked(MusEConfig::config.transportVisible);
      showBigtime->setChecked(MusEConfig::config.bigTimeVisible);
      //showMixer->setChecked(MusEConfig::config.mixerVisible);
      showMixer->setChecked(MusEConfig::config.mixer1Visible);
      showMixer2->setChecked(MusEConfig::config.mixer2Visible);

      arrangerX->setValue(MusEConfig::config.geometryMain.x());
      arrangerY->setValue(MusEConfig::config.geometryMain.y());
      arrangerW->setValue(MusEConfig::config.geometryMain.width());
      arrangerH->setValue(MusEConfig::config.geometryMain.height());

      transportX->setValue(MusEConfig::config.geometryTransport.x());
      transportY->setValue(MusEConfig::config.geometryTransport.y());

      bigtimeX->setValue(MusEConfig::config.geometryBigTime.x());
      bigtimeY->setValue(MusEConfig::config.geometryBigTime.y());
      bigtimeW->setValue(MusEConfig::config.geometryBigTime.width());
      bigtimeH->setValue(MusEConfig::config.geometryBigTime.height());

      //mixerX->setValue(MusEConfig::config.geometryMixer.x());
      //mixerY->setValue(MusEConfig::config.geometryMixer.y());
      //mixerW->setValue(MusEConfig::config.geometryMixer.width());
      //mixerH->setValue(MusEConfig::config.geometryMixer.height());
      mixerX->setValue(MusEConfig::config.mixer1.geometry.x());
      mixerY->setValue(MusEConfig::config.mixer1.geometry.y());
      mixerW->setValue(MusEConfig::config.mixer1.geometry.width());
      mixerH->setValue(MusEConfig::config.mixer1.geometry.height());
      mixer2X->setValue(MusEConfig::config.mixer2.geometry.x());
      mixer2Y->setValue(MusEConfig::config.mixer2.geometry.y());
      mixer2W->setValue(MusEConfig::config.mixer2.geometry.width());
      mixer2H->setValue(MusEConfig::config.mixer2.geometry.height());

      //setMixerCurrent->setEnabled(MusEGlobal::muse->mixerWindow());
      setMixerCurrent->setEnabled(MusEGlobal::muse->mixer1Window());
      setMixer2Current->setEnabled(MusEGlobal::muse->mixer2Window());
      
      setBigtimeCurrent->setEnabled(MusEGlobal::muse->bigtimeWindow());
      setTransportCurrent->setEnabled(MusEGlobal::muse->transportWindow());

      showSplash->setChecked(MusEConfig::config.showSplashScreen);
      showDidYouKnow->setChecked(MusEConfig::config.showDidYouKnow);
      externalWavEditorSelect->setText(MusEConfig::config.externalWavEditor);
      oldStyleStopCheckBox->setChecked(MusEConfig::config.useOldStyleStopShortCut);
      moveArmedCheckBox->setChecked(MusEConfig::config.moveArmedCheckBox);
      projectSaveCheckBox->setChecked(MusEConfig::config.useProjectSaveDialog);
      popsDefStayOpenCheckBox->setChecked(MusEConfig::config.popupsDefaultStayOpen);
      
      //updateSettings();    // TESTING
      
      connect(applyButton, SIGNAL(clicked()), SLOT(apply()));
      connect(okButton, SIGNAL(clicked()), SLOT(ok()));
      connect(cancelButton, SIGNAL(clicked()), SLOT(cancel()));
      connect(setMixerCurrent, SIGNAL(clicked()), SLOT(mixerCurrent()));
      connect(setMixer2Current, SIGNAL(clicked()), SLOT(mixer2Current()));
      connect(setBigtimeCurrent, SIGNAL(clicked()), SLOT(bigtimeCurrent()));
      connect(setArrangerCurrent, SIGNAL(clicked()), SLOT(arrangerCurrent()));
      connect(setTransportCurrent, SIGNAL(clicked()), SLOT(transportCurrent()));
      }

//---------------------------------------------------------
//   updateSettings
//---------------------------------------------------------

void GlobalSettingsConfig::updateSettings()
{
      for (unsigned i = 0; i < sizeof(rtcResolutions)/sizeof(*rtcResolutions); ++i) {
            if (rtcResolutions[i] == MusEConfig::config.rtcTicks) {
                  rtcResolutionSelect->setCurrentIndex(i);
                  break;
                  }
            }
      for (unsigned i = 0; i < sizeof(divisions)/sizeof(*divisions); ++i) {
            if (divisions[i] == MusEConfig::config.division) {
                  midiDivisionSelect->setCurrentIndex(i);
                  break;
                  }
            }
      for (unsigned i = 0; i < sizeof(divisions)/sizeof(*divisions); ++i) {
            if (divisions[i] == MusEConfig::config.guiDivision) {
                  guiDivisionSelect->setCurrentIndex(i);
                  break;
                  }
            }
      for (unsigned i = 0; i < sizeof(dummyAudioBufSizes)/sizeof(*dummyAudioBufSizes); ++i) {
            if (dummyAudioBufSizes[i] == MusEConfig::config.dummyAudioBufSize) {
                  dummyAudioSize->setCurrentIndex(i);
                  break;
                  }
            }
      
      for (unsigned i = 0; i < sizeof(minControlProcessPeriods)/sizeof(*minControlProcessPeriods); ++i) {
            if (minControlProcessPeriods[i] == MusEConfig::config.minControlProcessPeriod) {
                  minControlProcessPeriodComboBox->setCurrentIndex(i);
                  break;
                  }
            }

      guiRefreshSelect->setValue(MusEConfig::config.guiRefresh);
      minSliderSelect->setValue(int(MusEConfig::config.minSlider));
      minMeterSelect->setValue(MusEConfig::config.minMeter);
      freewheelCheckBox->setChecked(MusEConfig::config.freewheelMode);
      denormalCheckBox->setChecked(MusEConfig::config.useDenormalBias);
      outputLimiterCheckBox->setChecked(MusEConfig::config.useOutputLimiter);
      vstInPlaceCheckBox->setChecked(MusEConfig::config.vstInPlace);
      dummyAudioRate->setValue(MusEConfig::config.dummyAudioSampleRate);
      
      //DummyAudioDevice* dad = dynamic_cast<DummyAudioDevice*>(audioDevice);
      //dummyAudioRealRate->setText(dad ? QString().setNum(sampleRate) : "---");
      //dummyAudioRealRate->setText(QString().setNum(sampleRate));   // Not used any more. p4.0.20 
      
      startSongEntry->setText(MusEConfig::config.startSong);
      startSongGroup->button(MusEConfig::config.startMode)->setChecked(true);

      showTransport->setChecked(MusEConfig::config.transportVisible);
      showBigtime->setChecked(MusEConfig::config.bigTimeVisible);
      //showMixer->setChecked(MusEConfig::config.mixerVisible);
      showMixer->setChecked(MusEConfig::config.mixer1Visible);
      showMixer2->setChecked(MusEConfig::config.mixer2Visible);

      arrangerX->setValue(MusEConfig::config.geometryMain.x());
      arrangerY->setValue(MusEConfig::config.geometryMain.y());
      arrangerW->setValue(MusEConfig::config.geometryMain.width());
      arrangerH->setValue(MusEConfig::config.geometryMain.height());

      transportX->setValue(MusEConfig::config.geometryTransport.x());
      transportY->setValue(MusEConfig::config.geometryTransport.y());

      bigtimeX->setValue(MusEConfig::config.geometryBigTime.x());
      bigtimeY->setValue(MusEConfig::config.geometryBigTime.y());
      bigtimeW->setValue(MusEConfig::config.geometryBigTime.width());
      bigtimeH->setValue(MusEConfig::config.geometryBigTime.height());

      //mixerX->setValue(MusEConfig::config.geometryMixer.x());
      //mixerY->setValue(MusEConfig::config.geometryMixer.y());
      //mixerW->setValue(MusEConfig::config.geometryMixer.width());
      //mixerH->setValue(MusEConfig::config.geometryMixer.height());
      mixerX->setValue(MusEConfig::config.mixer1.geometry.x());
      mixerY->setValue(MusEConfig::config.mixer1.geometry.y());
      mixerW->setValue(MusEConfig::config.mixer1.geometry.width());
      mixerH->setValue(MusEConfig::config.mixer1.geometry.height());
      mixer2X->setValue(MusEConfig::config.mixer2.geometry.x());
      mixer2Y->setValue(MusEConfig::config.mixer2.geometry.y());
      mixer2W->setValue(MusEConfig::config.mixer2.geometry.width());
      mixer2H->setValue(MusEConfig::config.mixer2.geometry.height());

      //setMixerCurrent->setEnabled(MusEGlobal::muse->mixerWindow());
      setMixerCurrent->setEnabled(MusEGlobal::muse->mixer1Window());
      setMixer2Current->setEnabled(MusEGlobal::muse->mixer2Window());
      
      setBigtimeCurrent->setEnabled(MusEGlobal::muse->bigtimeWindow());
      setTransportCurrent->setEnabled(MusEGlobal::muse->transportWindow());

      showSplash->setChecked(MusEConfig::config.showSplashScreen);
      showDidYouKnow->setChecked(MusEConfig::config.showDidYouKnow);
      externalWavEditorSelect->setText(MusEConfig::config.externalWavEditor);
      oldStyleStopCheckBox->setChecked(MusEConfig::config.useOldStyleStopShortCut);
      moveArmedCheckBox->setChecked(MusEConfig::config.moveArmedCheckBox);
      projectSaveCheckBox->setChecked(MusEConfig::config.useProjectSaveDialog);
      popsDefStayOpenCheckBox->setChecked(MusEConfig::config.popupsDefaultStayOpen);
}

//---------------------------------------------------------
//   showEvent
//---------------------------------------------------------

void GlobalSettingsConfig::showEvent(QShowEvent* e)
{
  QDialog::showEvent(e);
  //updateSettings();     // TESTING
}

//---------------------------------------------------------
//   apply
//---------------------------------------------------------

void GlobalSettingsConfig::apply()
      {
      int rtcticks       = rtcResolutionSelect->currentIndex();
      MusEConfig::config.guiRefresh  = guiRefreshSelect->value();
      MusEConfig::config.minSlider   = minSliderSelect->value();
      MusEConfig::config.minMeter    = minMeterSelect->value();
      MusEConfig::config.freewheelMode = freewheelCheckBox->isChecked();
      MusEConfig::config.useDenormalBias = denormalCheckBox->isChecked();
      MusEConfig::config.useOutputLimiter = outputLimiterCheckBox->isChecked();
      MusEConfig::config.vstInPlace  = vstInPlaceCheckBox->isChecked();
      MusEConfig::config.rtcTicks    = rtcResolutions[rtcticks];
      MusEConfig::config.userInstrumentsDir = userInstrumentsPath->text();
      MusEConfig::config.startSong   = startSongEntry->text();
      MusEConfig::config.startMode   = startSongGroup->checkedId();
      int das = dummyAudioSize->currentIndex();
      MusEConfig::config.dummyAudioBufSize = dummyAudioBufSizes[das];
      MusEConfig::config.dummyAudioSampleRate = dummyAudioRate->value();
      int mcp = minControlProcessPeriodComboBox->currentIndex();
      MusEConfig::config.minControlProcessPeriod = minControlProcessPeriods[mcp];

      int div            = midiDivisionSelect->currentIndex();
      MusEConfig::config.division    = divisions[div];
      div                = guiDivisionSelect->currentIndex();
      MusEConfig::config.guiDivision = divisions[div];
      
      MusEConfig::config.transportVisible = showTransport->isChecked();
      MusEConfig::config.bigTimeVisible   = showBigtime->isChecked();
      //MusEConfig::config.mixerVisible     = showMixer->isChecked();
      MusEConfig::config.mixer1Visible     = showMixer->isChecked();
      MusEConfig::config.mixer2Visible     = showMixer2->isChecked();

      MusEConfig::config.geometryMain.setX(arrangerX->value());
      MusEConfig::config.geometryMain.setY(arrangerY->value());
      MusEConfig::config.geometryMain.setWidth(arrangerW->value());
      MusEConfig::config.geometryMain.setHeight(arrangerH->value());

      MusEConfig::config.geometryTransport.setX(transportX->value());
      MusEConfig::config.geometryTransport.setY(transportY->value());
      MusEConfig::config.geometryTransport.setWidth(0);
      MusEConfig::config.geometryTransport.setHeight(0);

      MusEConfig::config.geometryBigTime.setX(bigtimeX->value());
      MusEConfig::config.geometryBigTime.setY(bigtimeY->value());
      MusEConfig::config.geometryBigTime.setWidth(bigtimeW->value());
      MusEConfig::config.geometryBigTime.setHeight(bigtimeH->value());

      //MusEConfig::config.geometryMixer.setX(mixerX->value());
      //MusEConfig::config.geometryMixer.setY(mixerY->value());
      //MusEConfig::config.geometryMixer.setWidth(mixerW->value());
      //MusEConfig::config.geometryMixer.setHeight(mixerH->value());
      MusEConfig::config.mixer1.geometry.setX(mixerX->value());
      MusEConfig::config.mixer1.geometry.setY(mixerY->value());
      MusEConfig::config.mixer1.geometry.setWidth(mixerW->value());
      MusEConfig::config.mixer1.geometry.setHeight(mixerH->value());
      MusEConfig::config.mixer2.geometry.setX(mixer2X->value());
      MusEConfig::config.mixer2.geometry.setY(mixer2Y->value());
      MusEConfig::config.mixer2.geometry.setWidth(mixer2W->value());
      MusEConfig::config.mixer2.geometry.setHeight(mixer2H->value());

      MusEConfig::config.showSplashScreen = showSplash->isChecked();
      MusEConfig::config.showDidYouKnow   = showDidYouKnow->isChecked();
      MusEConfig::config.externalWavEditor = externalWavEditorSelect->text();
      MusEConfig::config.useOldStyleStopShortCut = oldStyleStopCheckBox->isChecked();
      MusEConfig::config.moveArmedCheckBox = moveArmedCheckBox->isChecked();
      MusEConfig::config.useProjectSaveDialog = projectSaveCheckBox->isChecked();
      MusEConfig::config.popupsDefaultStayOpen = popsDefStayOpenCheckBox->isChecked();

      //MusEGlobal::muse->showMixer(MusEConfig::config.mixerVisible);
      MusEGlobal::muse->showMixer1(MusEConfig::config.mixer1Visible);
      MusEGlobal::muse->showMixer2(MusEConfig::config.mixer2Visible);
      
      MusEGlobal::muse->showBigtime(MusEConfig::config.bigTimeVisible);
      MusEGlobal::muse->showTransport(MusEConfig::config.transportVisible);
      QWidget* w = MusEGlobal::muse->transportWindow();
      if (w) {
            w->resize(MusEConfig::config.geometryTransport.size());
            w->move(MusEConfig::config.geometryTransport.topLeft());
            }
      //w = MusEGlobal::muse->mixerWindow();
      //if (w) {
      //      w->resize(MusEConfig::config.geometryMixer.size());
      //      w->move(MusEConfig::config.geometryMixer.topLeft());
      //      }
      w = MusEGlobal::muse->mixer1Window();
      if (w) {
            w->resize(MusEConfig::config.mixer1.geometry.size());
            w->move(MusEConfig::config.mixer1.geometry.topLeft());
            }
      w = MusEGlobal::muse->mixer2Window();
      if (w) {
            w->resize(MusEConfig::config.mixer2.geometry.size());
            w->move(MusEConfig::config.mixer2.geometry.topLeft());
            }
      w = MusEGlobal::muse->bigtimeWindow();
      if (w) {
            w->resize(MusEConfig::config.geometryBigTime.size());
            w->move(MusEConfig::config.geometryBigTime.topLeft());
            }
      MusEGlobal::muse->resize(MusEConfig::config.geometryMain.size());
      MusEGlobal::muse->move(MusEConfig::config.geometryMain.topLeft());

      MusEGlobal::museUserInstruments = MusEConfig::config.userInstrumentsDir;

      MusEGlobal::muse->setHeartBeat();        // set guiRefresh
      midiSeq->msgSetRtc();        // set midi tick rate
      MusEGlobal::muse->changeConfig(true);    // save settings
      }

//---------------------------------------------------------
//   ok
//---------------------------------------------------------

void GlobalSettingsConfig::ok()
      {
      apply();
      close();
      }

//---------------------------------------------------------
//   cancel
//---------------------------------------------------------

void GlobalSettingsConfig::cancel()
      {
      close();
      }

//---------------------------------------------------------
//   mixerCurrent
//---------------------------------------------------------

void GlobalSettingsConfig::mixerCurrent()
      {
      QWidget* w = MusEGlobal::muse->mixer1Window();
      if (!w)
            return;
      QRect r(w->frameGeometry());
      mixerX->setValue(r.x());
      mixerY->setValue(r.y());
      mixerW->setValue(r.width());
      mixerH->setValue(r.height());
      }

//---------------------------------------------------------
//   mixer2Current
//---------------------------------------------------------

void GlobalSettingsConfig::mixer2Current()
      {
      QWidget* w = MusEGlobal::muse->mixer2Window();
      if (!w)
            return;
      QRect r(w->frameGeometry());
      mixer2X->setValue(r.x());
      mixer2Y->setValue(r.y());
      mixer2W->setValue(r.width());
      mixer2H->setValue(r.height());
      }

//---------------------------------------------------------
//   bigtimeCurrent
//---------------------------------------------------------

void GlobalSettingsConfig::bigtimeCurrent()
      {
      QWidget* w = MusEGlobal::muse->bigtimeWindow();
      if (!w)
            return;
      QRect r(w->frameGeometry());
      bigtimeX->setValue(r.x());
      bigtimeY->setValue(r.y());
      bigtimeW->setValue(r.width());
      bigtimeH->setValue(r.height());
      }

//---------------------------------------------------------
//   arrangerCurrent
//---------------------------------------------------------

void GlobalSettingsConfig::arrangerCurrent()
      {
      QRect r(MusEGlobal::muse->frameGeometry());
      arrangerX->setValue(r.x());
      arrangerY->setValue(r.y());
      arrangerW->setValue(r.width());
      arrangerH->setValue(r.height());
      }

//---------------------------------------------------------
//   transportCurrent
//---------------------------------------------------------

void GlobalSettingsConfig::transportCurrent()
      {
      QWidget* w = MusEGlobal::muse->transportWindow();
      if (!w)
            return;
      QRect r(w->frameGeometry());
      transportX->setValue(r.x());
      transportY->setValue(r.y());
      }

void GlobalSettingsConfig::selectInstrumentsPath()
      {
      QString dir = QFileDialog::getExistingDirectory(this, 
                                                      tr("Selects instruments directory"), 
                                                      MusEConfig::config.userInstrumentsDir);
      userInstrumentsPath->setText(dir);
      }

void GlobalSettingsConfig::defaultInstrumentsPath()
      {
      QString dir = MusEGlobal::configPath + "/instruments";
      userInstrumentsPath->setText(dir);
      }
