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

#include "genset.h"
#include "muse.h"
#include "gconfig.h"
#include "audio.h"
#include "globals.h"
#include "mixer/mixer.h"
#include "icons.h"
#include "song.h"
#include "midirc.h"
#include "driver/alsamidi.h"
#include "instruments/minstrument.h"
#include "midiedit/pianoroll.h"
#include "midiedit/drumedit.h"

static int rtcResolutions[] = {
      1024, 2048, 4096, 8192
      };
static int divisions[] = {
      48, 96, 192, 384, 768, 1536, 3072, 6144, 12288
      };

//---------------------------------------------------------
//   GlobalSettingsConfig
//---------------------------------------------------------

GlobalSettingsConfig::GlobalSettingsConfig(QWidget* parent)
   : QDialog(parent)
      {
      setupUi(this);
      for (unsigned i = 0; i < sizeof(rtcResolutions)/sizeof(*rtcResolutions); ++i) {
            if (rtcResolutions[i] == config.rtcTicks) {
                  rtcResolutionSelect->setCurrentIndex(i);
                  break;
                  }
            }
      for (unsigned i = 0; i < sizeof(divisions)/sizeof(*divisions); ++i) {
            if (divisions[i] == config.division) {
                  midiDivisionSelect->setCurrentIndex(i);
                  break;
                  }
            }
      std::list<PortName>* ol = midiDriver->inputPorts();
      int i = 0;
      for (std::list<PortName>::iterator ip = ol->begin(); ip != ol->end(); ++ip, ++i) {
            preferredInput->addItem(ip->name);
            if (ip->name == config.defaultMidiInputDevice)
                  preferredInput->setCurrentIndex(i);
            }
      ol = midiDriver->outputPorts();
      i = 0;
      for (std::list<PortName>::iterator ip = ol->begin(); ip != ol->end(); ++ip, ++i) {
            preferredOutput->addItem(ip->name);
            if (ip->name == config.defaultMidiOutputDevice)
                  preferredOutput->setCurrentIndex(i);
            }

      i = 0;
      for (iMidiInstrument mi = midiInstruments.begin(); mi != midiInstruments.end(); ++mi, ++i) {
            preferredInstrument->addItem((*mi)->iname());
            if ((*mi)->iname() == config.defaultMidiInstrument)
                  preferredInstrument->setCurrentIndex(i);
            }

      connectToAllDevices->setChecked(config.connectToAllMidiDevices);
      connectToAllTracks->setChecked(config.connectToAllMidiTracks);
      createDefaultInput->setChecked(config.createDefaultMidiInput);

      guiRefreshSelect->setValue(config.guiRefresh);
      minSliderSelect->setValue(int(config.minSlider));
      minMeterSelect->setValue(config.minMeter);
      peakHoldTime->setValue(config.peakHoldTime);
      helpBrowser->setText(config.helpBrowser);
      startSongEntry->setText(config.startSong);

      startSongGroup = new QButtonGroup(this);
      startSongGroup->addButton(startLast);
      startSongGroup->addButton(startTemplate);
      startSongGroup->addButton(startSong);

      switch(config.startMode) {
            case 0: startLast->setChecked(true); break;
            case 1: startTemplate->setChecked(true); break;
            case 2: startSong->setChecked(true); break;
            }

      showTransport->setChecked(config.transportVisible);
      showBigtime->setChecked(config.bigTimeVisible);
      showMixer1->setChecked(config.mixer1Visible);
      showMixer2->setChecked(config.mixer2Visible);

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

      mixerX1->setValue(config.mixer1.geometry.x());
      mixerY1->setValue(config.mixer1.geometry.y());
      mixerW1->setValue(config.mixer1.geometry.width());
      mixerH1->setValue(config.mixer1.geometry.height());

      mixerX2->setValue(config.mixer2.geometry.x());
      mixerY2->setValue(config.mixer2.geometry.y());
      mixerW2->setValue(config.mixer2.geometry.width());
      mixerH2->setValue(config.mixer2.geometry.height());

      setMixerCurrent1->setEnabled(muse->mixer1Window());
      setMixerCurrent1->setEnabled(muse->mixer2Window());

      setBigtimeCurrent->setEnabled(muse->bigtimeWindow());
      setTransportCurrent->setEnabled(muse->transportWindow());
      freewheelMode->setChecked(config.useJackFreewheelMode);
      showSplash->setChecked(config.showSplashScreen);

      stopActive->setChecked(midiRCList.isActive(RC_STOP));
      playActive->setChecked(midiRCList.isActive(RC_PLAY));
      gotoLeftMarkActive->setChecked(midiRCList.isActive(RC_GOTO_LEFT_MARK));
      recordActive->setChecked(midiRCList.isActive(RC_RECORD));

      connect(applyButton, SIGNAL(clicked()), SLOT(apply()));
      connect(okButton, SIGNAL(clicked()), SLOT(ok()));
      connect(cancelButton, SIGNAL(clicked()), SLOT(cancel()));
      connect(setMixerCurrent1, SIGNAL(clicked()), SLOT(mixerCurrent1()));
      connect(setMixerCurrent2, SIGNAL(clicked()), SLOT(mixerCurrent2()));
      connect(setBigtimeCurrent, SIGNAL(clicked()), SLOT(bigtimeCurrent()));
      connect(setArrangerCurrent, SIGNAL(clicked()), SLOT(arrangerCurrent()));
      connect(setTransportCurrent, SIGNAL(clicked()), SLOT(transportCurrent()));

      recordStop->setChecked(false);
      recordRecord->setChecked(false);
      recordGotoLeftMark->setChecked(false);
      recordPlay->setChecked(false);
      rcGroup->setChecked(rcEnable);

      pianorollWidth->setValue(PianoRoll::initWidth);
      pianorollHeight->setValue(PianoRoll::initHeight);
      pianorollRaster->setRaster(PianoRoll::initRaster);
      pianorollQuant->setQuant(PianoRoll::initQuant);

      drumEditorWidth->setValue(DrumEdit::initWidth);
      drumEditorHeight->setValue(DrumEdit::initHeight);

      connect(recordStop,         SIGNAL(clicked(bool)), SLOT(recordStopToggled(bool)));
      connect(recordRecord,       SIGNAL(clicked(bool)), SLOT(recordRecordToggled(bool)));
      connect(recordGotoLeftMark, SIGNAL(clicked(bool)), SLOT(recordGotoLeftMarkToggled(bool)));
      connect(recordPlay,         SIGNAL(clicked(bool)), SLOT(recordPlayToggled(bool)));
      }

//---------------------------------------------------------
//   apply
//---------------------------------------------------------

void GlobalSettingsConfig::apply()
      {
      rcEnable     = rcGroup->isChecked();
      int rtcticks = rtcResolutionSelect->currentIndex();
      int div      = midiDivisionSelect->currentIndex();

      config.connectToAllMidiDevices = connectToAllDevices->isChecked();
      config.connectToAllMidiTracks  = connectToAllTracks->isChecked();
      config.createDefaultMidiInput  = createDefaultInput->isChecked();
      config.defaultMidiInputDevice  = preferredInput->currentText();
      config.defaultMidiOutputDevice = preferredOutput->currentText();
      config.defaultMidiInstrument   = preferredInstrument->currentText();

      config.guiRefresh   = guiRefreshSelect->value();
      config.minSlider    = minSliderSelect->value();
      config.minMeter     = minMeterSelect->value();
      config.peakHoldTime = peakHoldTime->value();
      config.rtcTicks     = rtcResolutions[rtcticks];
      config.guiDivision  = divisions[div];
      config.helpBrowser  = helpBrowser->text();
      config.startSong    = startSongEntry->text();

      if (startLast->isChecked())
            config.startMode = 0;
      else if (startTemplate->isChecked())
            config.startMode = 1;
      else if (startSong->isChecked())
            config.startMode = 2;

      config.transportVisible = showTransport->isChecked();
      config.bigTimeVisible   = showBigtime->isChecked();
      config.mixer1Visible    = showMixer1->isChecked();
      config.mixer2Visible    = showMixer2->isChecked();

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

      config.mixer1.geometry.setX(mixerX1->value());
      config.mixer1.geometry.setY(mixerY1->value());
      config.mixer1.geometry.setWidth(mixerW1->value());
      config.mixer1.geometry.setHeight(mixerH1->value());

      config.mixer2.geometry.setX(mixerX2->value());
      config.mixer2.geometry.setY(mixerY2->value());
      config.mixer2.geometry.setWidth(mixerW2->value());
      config.mixer2.geometry.setHeight(mixerH2->value());

      config.useJackFreewheelMode = freewheelMode->isChecked();
      config.showSplashScreen = showSplash->isChecked();

      PianoRoll::initWidth  = pianorollWidth->value();
      PianoRoll::initHeight = pianorollHeight->value();
      PianoRoll::initRaster = pianorollRaster->raster();
      PianoRoll::initQuant  = pianorollQuant->quant();

      DrumEdit::initWidth   = drumEditorWidth->value();
      DrumEdit::initHeight  = drumEditorHeight->value();

      muse->showMixer1(config.mixer1Visible);
      muse->showMixer2(config.mixer2Visible);
      muse->showBigtime(config.bigTimeVisible);
      muse->showTransport(config.transportVisible);
      QWidget* w = muse->transportWindow();
      if (w) {
            w->resize(config.geometryTransport.size());
            w->move(config.geometryTransport.topLeft());
            }
      w = muse->mixer1Window();
      if (w) {
            w->resize(config.mixer1.geometry.size());
            w->move(config.mixer1.geometry.topLeft());
            }
      w = muse->mixer2Window();
      if (w) {
            w->resize(config.mixer2.geometry.size());
            w->move(config.mixer2.geometry.topLeft());
            }
      w = muse->bigtimeWindow();
      if (w) {
            w->resize(config.geometryBigTime.size());
            w->move(config.geometryBigTime.topLeft());
            }

      muse->resize(config.geometryMain.size());
      muse->move(config.geometryMain.topLeft());

      muse->setHeartBeat();        // set guiRefresh
      audio->msgSetRtc();          // set midi tick rate
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
//   mixerCurrent1
//---------------------------------------------------------

void GlobalSettingsConfig::mixerCurrent1()
      {
      QWidget* w = muse->mixer1Window();
      if (!w)
            return;
      QRect r(w->frameGeometry());
      mixerX1->setValue(r.x());
      mixerY1->setValue(r.y());
      mixerW1->setValue(r.width());
      mixerH1->setValue(r.height());
      }

//---------------------------------------------------------
//   mixerCurrent2
//---------------------------------------------------------

void GlobalSettingsConfig::mixerCurrent2()
      {
      QWidget* w = muse->mixer2Window();
      if (!w)
            return;
      QRect r(w->frameGeometry());
      mixerX2->setValue(r.x());
      mixerY2->setValue(r.y());
      mixerW2->setValue(r.width());
      mixerH2->setValue(r.height());
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

//---------------------------------------------------------
//   recordStopToggled
//---------------------------------------------------------

void GlobalSettingsConfig::recordStopToggled(bool f)
      {
      recordStop->setChecked(!f);
      if (!f) {
            recordRecord->setChecked(false);
            recordGotoLeftMark->setChecked(false);
            recordPlay->setChecked(false);
            connect(song, SIGNAL(midiEvent(MidiEvent)), SLOT(midiEventReceived(MidiEvent)));
            }
      else
            disconnect(song, SIGNAL(midiEvent(MidiEvent)), this, SLOT(midiEventReceived(MidiEvent)));
      }

//---------------------------------------------------------
//   recordRecordToggled
//---------------------------------------------------------

void GlobalSettingsConfig::recordRecordToggled(bool f)
      {
      recordRecord->setChecked(!f);
      if (!f) {
            recordStop->setChecked(false);
            recordGotoLeftMark->setChecked(false);
            recordPlay->setChecked(false);
            connect(song, SIGNAL(midiEvent(MidiEvent)), SLOT(midiEventReceived(MidiEvent)));
            }
      else
            disconnect(song, SIGNAL(midiEvent(MidiEvent)), this, SLOT(midiEventReceived(MidiEvent)));
      }

//---------------------------------------------------------
//   recordGotoLeftMarkToggled
//---------------------------------------------------------

void GlobalSettingsConfig::recordGotoLeftMarkToggled(bool f)
      {
      recordGotoLeftMark->setChecked(!f);
      if (!f) {
            recordStop->setChecked(false);
            recordRecord->setChecked(false);
            recordPlay->setChecked(false);
            connect(song, SIGNAL(midiEvent(MidiEvent)), SLOT(midiEventReceived(MidiEvent)));
            }
      else
            disconnect(song, SIGNAL(midiEvent(MidiEvent)), this, SLOT(midiEventReceived(MidiEvent)));
      }

//---------------------------------------------------------
//   recordPlayToggled
//---------------------------------------------------------

void GlobalSettingsConfig::recordPlayToggled(bool f)
      {
      recordPlay->setChecked(!f);
      if (!f) {
            recordStop->setChecked(false);
            recordRecord->setChecked(false);
            recordGotoLeftMark->setChecked(false);
            connect(song, SIGNAL(midiEvent(MidiEvent)), SLOT(midiEventReceived(MidiEvent)));
            }
      else
            disconnect(song, SIGNAL(midiEvent(MidiEvent)), this, SLOT(midiEventReceived(MidiEvent)));
      }

//---------------------------------------------------------
//   midiEventReceived
//---------------------------------------------------------

void GlobalSettingsConfig::midiEventReceived(MidiEvent event)
      {
      printf("event received\n");
      if (recordPlay->isChecked()) {
            recordPlay->setChecked(false);
            playActive->setChecked(true);
            midiRCList.setAction(event, RC_PLAY);
            }
      else if (recordStop->isChecked()) {
            recordStop->setChecked(false);
            stopActive->setChecked(true);
            midiRCList.setAction(event, RC_STOP);
            }
      else if (recordRecord->isChecked()) {
            recordRecord->setChecked(false);
            recordActive->setChecked(true);
            midiRCList.setAction(event, RC_RECORD);
            }
      else if (recordGotoLeftMark->isChecked()) {
            recordGotoLeftMark->setChecked(false);
            gotoLeftMarkActive->setChecked(true);
            midiRCList.setAction(event, RC_GOTO_LEFT_MARK);
            }
      // only one shot
      disconnect(song, SIGNAL(midiEvent(MidiEvent)), this, SLOT(midiEventReceived(MidiEvent)));
      }

