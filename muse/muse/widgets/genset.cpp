//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: genset.cpp,v 1.7.2.8 2009/12/01 03:52:40 terminator356 Exp $
//
//  (C) Copyright 2001-2004 Werner Schweer (ws@seh.de)
//=========================================================

#include <stdio.h>
#include <qpushbutton.h>
#include <qcombobox.h>
#include <qspinbox.h>
#include <qlineedit.h>
#include <qbuttongroup.h>
#include <qcheckbox.h>

#include "genset.h"
#include "app.h"
#include "gconfig.h"
#include "midiseq.h"
#include "globals.h"

static int rtcResolutions[] = {
      1024, 2048, 4096, 8192, 16384, 32768
      };
static int divisions[] = {
      48, 96, 192, 384, 768, 1536, 3072, 6144, 12288
      };

//---------------------------------------------------------
//   GlobalSettingsConfig
//---------------------------------------------------------

GlobalSettingsConfig::GlobalSettingsConfig(QWidget* parent, const char* name)
   : GlobalSettingsDialogBase(parent, name)
      {
      for (unsigned i = 0; i < sizeof(rtcResolutions)/sizeof(*rtcResolutions); ++i) {
            if (rtcResolutions[i] == config.rtcTicks) {
                  rtcResolutionSelect->setCurrentItem(i);
                  break;
                  }
            }
      for (unsigned i = 0; i < sizeof(divisions)/sizeof(*divisions); ++i) {
            if (divisions[i] == config.division) {
                  midiDivisionSelect->setCurrentItem(i);
                  break;
                  }
            }
      for (unsigned i = 0; i < sizeof(divisions)/sizeof(*divisions); ++i) {
            if (divisions[i] == config.guiDivision) {
                  guiDivisionSelect->setCurrentItem(i);
                  break;
                  }
            }
      
      guiRefreshSelect->setValue(config.guiRefresh);
      minSliderSelect->setValue(int(config.minSlider));
      minMeterSelect->setValue(config.minMeter);
      freewheelCheckBox->setChecked(config.freewheelMode);
      denormalCheckBox->setChecked(config.useDenormalBias);
      outputLimiterCheckBox->setChecked(config.useOutputLimiter);
      vstInPlaceCheckBox->setChecked(config.vstInPlace);
      
      helpBrowser->setText(config.helpBrowser);
      startSongEntry->setText(config.startSong);
      startSongGroup->setButton(config.startMode);

      showTransport->setChecked(config.transportVisible);
      showBigtime->setChecked(config.bigTimeVisible);
      showMixer->setChecked(config.mixerVisible);

      arrangerX->setValue(config.geometryMain.x());
      arrangerY->setValue(config.geometryMain.y());
      arrangerW->setValue(config.geometryMain.width());
      arrangerH->setValue(config.geometryMain.height());

      transportX->setValue(config.geometryTransport.x());
      transportY->setValue(config.geometryTransport.y());

      bigtimeX->setValue(config.geometryBigTime.x());
      bigtimeY->setValue(config.geometryBigTime.y());
      bigtimeW->setValue(config.geometryBigTime.width());
      bigtimeH->setValue(config.geometryBigTime.height());

      mixerX->setValue(config.geometryMixer.x());
      mixerY->setValue(config.geometryMixer.y());
      mixerW->setValue(config.geometryMixer.width());
      mixerH->setValue(config.geometryMixer.height());

      setMixerCurrent->setEnabled(muse->mixerWindow());
      setBigtimeCurrent->setEnabled(muse->bigtimeWindow());
      setTransportCurrent->setEnabled(muse->transportWindow());

      showSplash->setChecked(config.showSplashScreen);
      showDidYouKnow->setChecked(config.showDidYouKnow);
      externalWavEditorSelect->setText(config.externalWavEditor);
      oldStyleStopCheckBox->setChecked(config.useOldStyleStopShortCut);

      connect(applyButton, SIGNAL(clicked()), SLOT(apply()));
      connect(okButton, SIGNAL(clicked()), SLOT(ok()));
      connect(cancelButton, SIGNAL(clicked()), SLOT(cancel()));
      connect(setMixerCurrent, SIGNAL(clicked()), SLOT(mixerCurrent()));
      connect(setBigtimeCurrent, SIGNAL(clicked()), SLOT(bigtimeCurrent()));
      connect(setArrangerCurrent, SIGNAL(clicked()), SLOT(arrangerCurrent()));
      connect(setTransportCurrent, SIGNAL(clicked()), SLOT(transportCurrent()));
      }

//---------------------------------------------------------
//   apply
//---------------------------------------------------------

void GlobalSettingsConfig::apply()
      {
      int rtcticks       = rtcResolutionSelect->currentItem();
      config.guiRefresh  = guiRefreshSelect->value();
      config.minSlider   = minSliderSelect->value();
      config.minMeter    = minMeterSelect->value();
      config.freewheelMode = freewheelCheckBox->isChecked();
      config.useDenormalBias = denormalCheckBox->isChecked();
      config.useOutputLimiter = outputLimiterCheckBox->isChecked();
      config.vstInPlace  = vstInPlaceCheckBox->isChecked();
      config.rtcTicks    = rtcResolutions[rtcticks];
      config.helpBrowser = helpBrowser->text();
      config.startSong   = startSongEntry->text();
      config.startMode   = startSongGroup->selectedId();

      int div            = midiDivisionSelect->currentItem();
      config.division    = divisions[div];
      div                = guiDivisionSelect->currentItem();
      config.guiDivision = divisions[div];
      
      config.transportVisible = showTransport->isChecked();
      config.bigTimeVisible   = showBigtime->isChecked();
      config.mixerVisible     = showMixer->isChecked();

      config.geometryMain.setX(arrangerX->value());
      config.geometryMain.setY(arrangerY->value());
      config.geometryMain.setWidth(arrangerW->value());
      config.geometryMain.setHeight(arrangerH->value());

      config.geometryTransport.setX(transportX->value());
      config.geometryTransport.setY(transportY->value());
      config.geometryTransport.setWidth(0);
      config.geometryTransport.setHeight(0);

      config.geometryBigTime.setX(bigtimeX->value());
      config.geometryBigTime.setY(bigtimeY->value());
      config.geometryBigTime.setWidth(bigtimeW->value());
      config.geometryBigTime.setHeight(bigtimeH->value());

      config.geometryMixer.setX(mixerX->value());
      config.geometryMixer.setY(mixerY->value());
      config.geometryMixer.setWidth(mixerW->value());
      config.geometryMixer.setHeight(mixerH->value());

      config.showSplashScreen = showSplash->isChecked();
      config.showDidYouKnow   = showDidYouKnow->isChecked();
      config.externalWavEditor = externalWavEditorSelect->text();
      config.useOldStyleStopShortCut = oldStyleStopCheckBox->isChecked();
      muse->showMixer(config.mixerVisible);
      muse->showBigtime(config.bigTimeVisible);
      muse->showTransport(config.transportVisible);
      QWidget* w = muse->transportWindow();
      if (w) {
            w->resize(config.geometryTransport.size());
            w->move(config.geometryTransport.topLeft());
            }
      w = muse->mixerWindow();
      if (w) {
            w->resize(config.geometryMixer.size());
            w->move(config.geometryMixer.topLeft());
            }
      w = muse->bigtimeWindow();
      if (w) {
            w->resize(config.geometryBigTime.size());
            w->move(config.geometryBigTime.topLeft());
            }
      muse->resize(config.geometryMain.size());
      muse->move(config.geometryMain.topLeft());

      muse->setHeartBeat();        // set guiRefresh
      midiSeq->msgSetRtc();        // set midi tick rate
      muse->changeConfig(true);    // save settings
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
      QWidget* w = muse->mixerWindow();
      if (!w)
            return;
      QRect r(w->frameGeometry());
      mixerX->setValue(r.x());
      mixerY->setValue(r.y());
      mixerW->setValue(r.width());
      mixerH->setValue(r.height());
      }

//---------------------------------------------------------
//   bigtimeCurrent
//---------------------------------------------------------

void GlobalSettingsConfig::bigtimeCurrent()
      {
      QWidget* w = muse->bigtimeWindow();
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
      QRect r(muse->frameGeometry());
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
      QWidget* w = muse->transportWindow();
      if (!w)
            return;
      QRect r(w->frameGeometry());
      transportX->setValue(r.x());
      transportY->setValue(r.y());
      }

