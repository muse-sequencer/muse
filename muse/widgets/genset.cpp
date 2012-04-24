//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: genset.cpp,v 1.7.2.8 2009/12/01 03:52:40 terminator356 Exp $
//
//  (C) Copyright 2001-2004 Werner Schweer (ws@seh.de)
//  (C) Copyright 2011 Tim E. Real (terminator356 on sourceforge)
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
#include <QFileInfo>
#include <QRect>
#include <QShowEvent>
#include <QString>

#include "genset.h"
#include "app.h"
#include "gconfig.h"
#include "midiseq.h"
#include "globals.h"
#include "icons.h"
#include "helper.h"
#include "filedialog.h"

namespace MusEGui {

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

      updateSettings();
      
      projDirOpenToolButton->setIcon(*openIcon); // FINDMICH
      connect(projDirOpenToolButton, SIGNAL(clicked()), SLOT(browseProjDir()));
      startSongFileOpenToolButton->setIcon(*openIcon); 
      connect(startSongFileOpenToolButton, SIGNAL(clicked()), SLOT(browseStartSongFile()));
      startSongResetToolButton->setIcon(*undoIcon);
      connect(startSongResetToolButton, SIGNAL(clicked()), SLOT(startSongReset()));
      
      connect(applyButton, SIGNAL(clicked()), SLOT(apply()));
      connect(okButton, SIGNAL(clicked()), SLOT(ok()));
      connect(cancelButton, SIGNAL(clicked()), SLOT(cancel()));
      connect(setMixerCurrent, SIGNAL(clicked()), SLOT(mixerCurrent()));
      connect(setMixer2Current, SIGNAL(clicked()), SLOT(mixer2Current()));
      connect(setBigtimeCurrent, SIGNAL(clicked()), SLOT(bigtimeCurrent()));
      connect(setMainCurrent, SIGNAL(clicked()), SLOT(mainCurrent()));
      connect(setTransportCurrent, SIGNAL(clicked()), SLOT(transportCurrent()));
      
      connect(buttonTraditionalPreset, SIGNAL(clicked()), SLOT(traditionalPreset()));
      connect(buttonMDIPreset, SIGNAL(clicked()), SLOT(mdiPreset()));
      connect(buttonBorlandPreset, SIGNAL(clicked()), SLOT(borlandPreset()));
      
      addMdiSettings(TopWin::ARRANGER);
      addMdiSettings(TopWin::SCORE);
      addMdiSettings(TopWin::PIANO_ROLL);
      addMdiSettings(TopWin::DRUM);
      addMdiSettings(TopWin::LISTE);
      addMdiSettings(TopWin::WAVE);
      addMdiSettings(TopWin::MASTER);
      addMdiSettings(TopWin::LMASTER);
      addMdiSettings(TopWin::CLIPLIST);
      addMdiSettings(TopWin::MARKER);
      
      }

void GlobalSettingsConfig::addMdiSettings(TopWin::ToplevelType t)
{
  MdiSettings* temp = new MdiSettings(t, this);
  layoutMdiSettings->addWidget(temp);
  mdisettings.push_back(temp);
}

//---------------------------------------------------------
//   updateSettings
//---------------------------------------------------------

void GlobalSettingsConfig::updateSettings()
{
      for (unsigned i = 0; i < sizeof(rtcResolutions)/sizeof(*rtcResolutions); ++i) {
            if (rtcResolutions[i] == MusEGlobal::config.rtcTicks) {
                  rtcResolutionSelect->setCurrentIndex(i);
                  break;
                  }
            }
      for (unsigned i = 0; i < sizeof(divisions)/sizeof(*divisions); ++i) {
            if (divisions[i] == MusEGlobal::config.division) {
                  midiDivisionSelect->setCurrentIndex(i);
                  break;
                  }
            }
      for (unsigned i = 0; i < sizeof(divisions)/sizeof(*divisions); ++i) {
            if (divisions[i] == MusEGlobal::config.guiDivision) {
                  guiDivisionSelect->setCurrentIndex(i);
                  break;
                  }
            }
      for (unsigned i = 0; i < sizeof(dummyAudioBufSizes)/sizeof(*dummyAudioBufSizes); ++i) {
            if (dummyAudioBufSizes[i] == MusEGlobal::config.dummyAudioBufSize) {
                  dummyAudioSize->setCurrentIndex(i);
                  break;
                  }
            }
      
      for (unsigned i = 0; i < sizeof(minControlProcessPeriods)/sizeof(*minControlProcessPeriods); ++i) {
            if (minControlProcessPeriods[i] == MusEGlobal::config.minControlProcessPeriod) {
                  minControlProcessPeriodComboBox->setCurrentIndex(i);
                  break;
                  }
            }

      guiRefreshSelect->setValue(MusEGlobal::config.guiRefresh);
      minSliderSelect->setValue(int(MusEGlobal::config.minSlider));
      minMeterSelect->setValue(MusEGlobal::config.minMeter);
      freewheelCheckBox->setChecked(MusEGlobal::config.freewheelMode);
      denormalCheckBox->setChecked(MusEGlobal::config.useDenormalBias);
      outputLimiterCheckBox->setChecked(MusEGlobal::config.useOutputLimiter);
      vstInPlaceCheckBox->setChecked(MusEGlobal::config.vstInPlace);
      dummyAudioRate->setValue(MusEGlobal::config.dummyAudioSampleRate);
      
      //DummyAudioDevice* dad = dynamic_cast<DummyAudioDevice*>(audioDevice);
      //dummyAudioRealRate->setText(dad ? QString().setNum(sampleRate) : "---");
      //dummyAudioRealRate->setText(QString().setNum(sampleRate));   // Not used any more. p4.0.20  DELETETHIS?
      
      projDirEntry->setText(MusEGlobal::config.projectBaseFolder);

      startSongEntry->setText(MusEGlobal::config.startSong);
      startSongGroup->button(MusEGlobal::config.startMode)->setChecked(true);
      readMidiConfigFromSongCheckBox->setChecked(MusEGlobal::config.startSongLoadConfig);
      
      showTransport->setChecked(MusEGlobal::config.transportVisible);
      showBigtime->setChecked(MusEGlobal::config.bigTimeVisible);
      showMixer->setChecked(MusEGlobal::config.mixer1Visible);
      showMixer2->setChecked(MusEGlobal::config.mixer2Visible);

      mainX->setValue(MusEGlobal::config.geometryMain.x());
      mainY->setValue(MusEGlobal::config.geometryMain.y());
      mainW->setValue(MusEGlobal::config.geometryMain.width());
      mainH->setValue(MusEGlobal::config.geometryMain.height());

      transportX->setValue(MusEGlobal::config.geometryTransport.x());
      transportY->setValue(MusEGlobal::config.geometryTransport.y());

      bigtimeX->setValue(MusEGlobal::config.geometryBigTime.x());
      bigtimeY->setValue(MusEGlobal::config.geometryBigTime.y());
      bigtimeW->setValue(MusEGlobal::config.geometryBigTime.width());
      bigtimeH->setValue(MusEGlobal::config.geometryBigTime.height());

      mixerX->setValue(MusEGlobal::config.mixer1.geometry.x());
      mixerY->setValue(MusEGlobal::config.mixer1.geometry.y());
      mixerW->setValue(MusEGlobal::config.mixer1.geometry.width());
      mixerH->setValue(MusEGlobal::config.mixer1.geometry.height());
      mixer2X->setValue(MusEGlobal::config.mixer2.geometry.x());
      mixer2Y->setValue(MusEGlobal::config.mixer2.geometry.y());
      mixer2W->setValue(MusEGlobal::config.mixer2.geometry.width());
      mixer2H->setValue(MusEGlobal::config.mixer2.geometry.height());

      setMixerCurrent->setEnabled(MusEGlobal::muse->mixer1Window());
      setMixer2Current->setEnabled(MusEGlobal::muse->mixer2Window());
      
      setBigtimeCurrent->setEnabled(MusEGlobal::muse->bigtimeWindow());
      setTransportCurrent->setEnabled(MusEGlobal::muse->transportWindow());

      showSplash->setChecked(MusEGlobal::config.showSplashScreen);
      showDidYouKnow->setChecked(MusEGlobal::config.showDidYouKnow);
      externalWavEditorSelect->setText(MusEGlobal::config.externalWavEditor);
      oldStyleStopCheckBox->setChecked(MusEGlobal::config.useOldStyleStopShortCut);
      moveArmedCheckBox->setChecked(MusEGlobal::config.moveArmedCheckBox);
      projectSaveCheckBox->setChecked(MusEGlobal::config.useProjectSaveDialog);
      popsDefStayOpenCheckBox->setChecked(MusEGlobal::config.popupsDefaultStayOpen);
      lmbDecreasesCheckBox->setChecked(MusEGlobal::config.leftMouseButtonCanDecrease);
      rangeMarkerWithoutMMBCheckBox->setChecked(MusEGlobal::config.rangeMarkerWithoutMMB);
      smartFocusCheckBox->setChecked(MusEGlobal::config.smartFocus);
      
      addHiddenCheckBox->setChecked(MusEGlobal::config.addHiddenTracks);
      unhideTracksCheckBox->setChecked(MusEGlobal::config.unhideTracks);

      updateMdiSettings();
}

void GlobalSettingsConfig::updateMdiSettings()
{
  for (std::list<MdiSettings*>::iterator it = mdisettings.begin(); it!=mdisettings.end(); it++)
    (*it)->update_settings();
}

void GlobalSettingsConfig::applyMdiSettings()
{
  for (std::list<MdiSettings*>::iterator it = mdisettings.begin(); it!=mdisettings.end(); it++)
    (*it)->apply_settings();
}

//---------------------------------------------------------
//   showEvent
//---------------------------------------------------------

void GlobalSettingsConfig::showEvent(QShowEvent* e)
{
  QDialog::showEvent(e);
  updateSettings();
}

//---------------------------------------------------------
//   apply
//---------------------------------------------------------

void GlobalSettingsConfig::apply()
      {
      int rtcticks       = rtcResolutionSelect->currentIndex();
      MusEGlobal::config.guiRefresh  = guiRefreshSelect->value();
      MusEGlobal::config.minSlider   = minSliderSelect->value();
      MusEGlobal::config.minMeter    = minMeterSelect->value();
      MusEGlobal::config.freewheelMode = freewheelCheckBox->isChecked();
      MusEGlobal::config.useDenormalBias = denormalCheckBox->isChecked();
      MusEGlobal::config.useOutputLimiter = outputLimiterCheckBox->isChecked();
      MusEGlobal::config.vstInPlace  = vstInPlaceCheckBox->isChecked();
      MusEGlobal::config.rtcTicks    = rtcResolutions[rtcticks];
      
      MusEGlobal::config.projectBaseFolder = projDirEntry->text();
      
      MusEGlobal::config.startSong   = startSongEntry->text();
      MusEGlobal::config.startMode   = startSongGroup->checkedId();
      MusEGlobal::config.startSongLoadConfig = readMidiConfigFromSongCheckBox->isChecked();
      
      int das = dummyAudioSize->currentIndex();
      MusEGlobal::config.dummyAudioBufSize = dummyAudioBufSizes[das];
      MusEGlobal::config.dummyAudioSampleRate = dummyAudioRate->value();
      int mcp = minControlProcessPeriodComboBox->currentIndex();
      MusEGlobal::config.minControlProcessPeriod = minControlProcessPeriods[mcp];

      int div            = midiDivisionSelect->currentIndex();
      MusEGlobal::config.division    = divisions[div];
      div                = guiDivisionSelect->currentIndex();
      MusEGlobal::config.guiDivision = divisions[div];
      
      MusEGlobal::config.transportVisible = showTransport->isChecked();
      MusEGlobal::config.bigTimeVisible   = showBigtime->isChecked();
      MusEGlobal::config.mixer1Visible     = showMixer->isChecked();
      MusEGlobal::config.mixer2Visible     = showMixer2->isChecked();

      MusEGlobal::config.geometryMain.setX(mainX->value());
      MusEGlobal::config.geometryMain.setY(mainY->value());
      MusEGlobal::config.geometryMain.setWidth(mainW->value());
      MusEGlobal::config.geometryMain.setHeight(mainH->value());

      MusEGlobal::config.geometryTransport.setX(transportX->value());
      MusEGlobal::config.geometryTransport.setY(transportY->value());
      MusEGlobal::config.geometryTransport.setWidth(0);
      MusEGlobal::config.geometryTransport.setHeight(0);

      MusEGlobal::config.geometryBigTime.setX(bigtimeX->value());
      MusEGlobal::config.geometryBigTime.setY(bigtimeY->value());
      MusEGlobal::config.geometryBigTime.setWidth(bigtimeW->value());
      MusEGlobal::config.geometryBigTime.setHeight(bigtimeH->value());

      MusEGlobal::config.mixer1.geometry.setX(mixerX->value());
      MusEGlobal::config.mixer1.geometry.setY(mixerY->value());
      MusEGlobal::config.mixer1.geometry.setWidth(mixerW->value());
      MusEGlobal::config.mixer1.geometry.setHeight(mixerH->value());
      MusEGlobal::config.mixer2.geometry.setX(mixer2X->value());
      MusEGlobal::config.mixer2.geometry.setY(mixer2Y->value());
      MusEGlobal::config.mixer2.geometry.setWidth(mixer2W->value());
      MusEGlobal::config.mixer2.geometry.setHeight(mixer2H->value());

      MusEGlobal::config.showSplashScreen = showSplash->isChecked();
      MusEGlobal::config.showDidYouKnow   = showDidYouKnow->isChecked();
      MusEGlobal::config.externalWavEditor = externalWavEditorSelect->text();
      MusEGlobal::config.useOldStyleStopShortCut = oldStyleStopCheckBox->isChecked();
      MusEGlobal::config.moveArmedCheckBox = moveArmedCheckBox->isChecked();
      MusEGlobal::config.useProjectSaveDialog = projectSaveCheckBox->isChecked();
      MusEGlobal::config.popupsDefaultStayOpen = popsDefStayOpenCheckBox->isChecked();
      MusEGlobal::config.leftMouseButtonCanDecrease = lmbDecreasesCheckBox->isChecked();
      MusEGlobal::config.rangeMarkerWithoutMMB = rangeMarkerWithoutMMBCheckBox->isChecked();
      MusEGlobal::config.smartFocus = smartFocusCheckBox->isChecked();

      MusEGlobal::config.addHiddenTracks = addHiddenCheckBox->isChecked();
      MusEGlobal::config.unhideTracks = unhideTracksCheckBox->isChecked();

      MusEGlobal::muse->showMixer1(MusEGlobal::config.mixer1Visible);
      MusEGlobal::muse->showMixer2(MusEGlobal::config.mixer2Visible);
      
      MusEGlobal::muse->showBigtime(MusEGlobal::config.bigTimeVisible);
      MusEGlobal::muse->showTransport(MusEGlobal::config.transportVisible);
      QWidget* w = MusEGlobal::muse->transportWindow();
      if (w) {
            w->resize(MusEGlobal::config.geometryTransport.size());
            w->move(MusEGlobal::config.geometryTransport.topLeft());
            }
      w = MusEGlobal::muse->mixer1Window();
      if (w) {
            w->resize(MusEGlobal::config.mixer1.geometry.size());
            w->move(MusEGlobal::config.mixer1.geometry.topLeft());
            }
      w = MusEGlobal::muse->mixer2Window();
      if (w) {
            w->resize(MusEGlobal::config.mixer2.geometry.size());
            w->move(MusEGlobal::config.mixer2.geometry.topLeft());
            }
      w = MusEGlobal::muse->bigtimeWindow();
      if (w) {
            w->resize(MusEGlobal::config.geometryBigTime.size());
            w->move(MusEGlobal::config.geometryBigTime.topLeft());
            }
      MusEGlobal::muse->resize(MusEGlobal::config.geometryMain.size());
      MusEGlobal::muse->move(MusEGlobal::config.geometryMain.topLeft());

      MusEGlobal::muse->setHeartBeat();        // set guiRefresh
      MusEGlobal::midiSeq->msgSetRtc();        // set midi tick rate
      
      applyMdiSettings();
      
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
      mixerW->setValue(w->width());
      mixerH->setValue(w->height());
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
      mixer2W->setValue(w->width());
      mixer2H->setValue(w->height());
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
      bigtimeW->setValue(w->width());
      bigtimeH->setValue(w->height());
      }

//---------------------------------------------------------
//   mainCurrent
//---------------------------------------------------------

void GlobalSettingsConfig::mainCurrent()
      {
      QRect r(MusEGlobal::muse->frameGeometry());
      mainX->setValue(r.x());
      mainY->setValue(r.y());
      mainW->setValue(MusEGlobal::muse->width());  //this is intendedly not the frameGeometry, but
      mainH->setValue(MusEGlobal::muse->height()); //the "non-frame-geom." to avoid a sizing bug
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

void GlobalSettingsConfig::traditionalPreset()
{
  for (std::list<MdiSettings*>::iterator it = mdisettings.begin(); it!=mdisettings.end(); it++)
  {
    TopWin::ToplevelType type = (*it)->type();
    TopWin::_sharesWhenFree[type]=false;
    TopWin::_defaultSubwin[type]=false;
  }
  TopWin::_defaultSubwin[TopWin::ARRANGER]=true;
  
  updateMdiSettings();
}

void GlobalSettingsConfig::mdiPreset()
{
  for (std::list<MdiSettings*>::iterator it = mdisettings.begin(); it!=mdisettings.end(); it++)
  {
    TopWin::ToplevelType type = (*it)->type();
    TopWin::_sharesWhenSubwin[type]=true;
    TopWin::_defaultSubwin[type]=true;
  }
  
  updateMdiSettings();
}

void GlobalSettingsConfig::borlandPreset()
{
  for (std::list<MdiSettings*>::iterator it = mdisettings.begin(); it!=mdisettings.end(); it++)
  {
    TopWin::ToplevelType type = (*it)->type();
    TopWin::_sharesWhenFree[type]=true;
    TopWin::_defaultSubwin[type]=false;
  }
  
  updateMdiSettings();
}

void GlobalSettingsConfig::browseProjDir()
{
  QString dir = MusEGui::browseProjectFolder(this);
  if(!dir.isEmpty())
    projDirEntry->setText(dir);
}

void GlobalSettingsConfig::browseStartSongFile()
{
  bool doReadMidiPorts;
  QString sstr = startSongGroup->button(1)->isChecked() ? QString("templates") : QString("");

  QString fn = MusEGui::getOpenFileName(sstr, MusEGlobal::med_file_pattern, this,
      tr("MusE: Choose start template or song"), &doReadMidiPorts, MusEGui::MFileDialog::GLOBAL_VIEW);
  if (!fn.isEmpty()) {
        startSongEntry->setText(fn);
        readMidiConfigFromSongCheckBox->setChecked(doReadMidiPorts);
        }
}

void GlobalSettingsConfig::startSongReset()
{
  startSongEntry->setText(MusEGlobal::museGlobalShare + QString("/templates/default.med"));
  readMidiConfigFromSongCheckBox->setChecked(false);
}

} // namespace MusEGui

