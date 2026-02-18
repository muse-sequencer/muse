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

//#include <stdio.h>

#include <QFileDialog>
//#include <QFileInfo>
#include <QRect>
#include <QShowEvent>
#include <QString>
#include <QLocale>
#include <QCollator>
#include <QMessageBox>

#include "genset.h"
#include "app.h"
#include "gconfig.h"
#include "midiseq.h"
#include "globals.h"
#include "icons.h"
#include "helper.h"
#include "filedialog.h"
//#include "al/al.h"
#include "undo.h"
#include "cobject.h"

#include "song.h"
#include "operations.h"
#include "audio.h"
#include "audio_converter_settings.h"
//#include "audio_convert/audio_converter_plugin.h"
#include "audio_convert/audio_converter_settings_group.h"

// Forwards from header:
#include <QButtonGroup>
//#include <QShowEvent>
#include <QWidget>

namespace MusEGui {

static int rtcResolutions[] = {
      1024, 2048, 4096, 8192, 16384, 32768
      };
static int divisions[] = {
// REMOVE Tim. div. Changed
//       48, 96, 192, 384, 768, 1536, 3072, 6144, 12288
      48, 96, 120, 192, 240, 384, 480, 768, 960, 1536, 1920, 3072, 3840, 6144, 7680, 12288, 15360
      };
static int selectableAudioBufSizes[] = {
      16, 32, 64, 128, 256, 512, 1024, 2048
      };
static unsigned long minControlProcessPeriods[] = {
      1, 2, 4, 8, 16, 32, 64, 128, 256, 512, 1024, 2048
      };

static int NOTE_FIRST_NAME_COL = 0;
static int NOTE_SECOND_NAME_COL = 1;

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
      
      recDrumGroup = new QButtonGroup(this);
      recDrumGroup->addButton(recordAllButton, MusECore::REC_ALL);
      recDrumGroup->addButton(dontRecHiddenButton, MusECore::DONT_REC_HIDDEN);
      recDrumGroup->addButton(dontRecMutedButton, MusECore::DONT_REC_MUTED);
      recDrumGroup->addButton(dontRecBothButton, MusECore::DONT_REC_MUTED_OR_HIDDEN);
      
      updateSettings();
      
      projDirOpenToolButton->setIcon(*fileopenSVGIcon);
      connect(projDirOpenToolButton, SIGNAL(clicked()), SLOT(browseProjDir()));
      startSongFileOpenToolButton->setIcon(*fileopenSVGIcon);
      connect(startSongFileOpenToolButton, SIGNAL(clicked()), SLOT(browseStartSongFile()));
      startSongResetToolButton->setIcon(*undoSVGIcon);
      connect(startSongResetToolButton, SIGNAL(clicked()), SLOT(startSongReset()));
      
      connect(applyButton, SIGNAL(clicked()), SLOT(apply()));
      connect(okButton, SIGNAL(clicked()), SLOT(ok()));
      connect(cancelButton, SIGNAL(clicked()), SLOT(cancel()));
      
      connect(pluginPathAdd, SIGNAL(clicked()), SLOT(addPluginPath()));
      connect(pluginPathEdit, SIGNAL(clicked()), SLOT(editPluginPath()));
      connect(pluginPathRemove, SIGNAL(clicked()), SLOT(removePluginPath()));
      connect(pluginPathMoveUp, SIGNAL(clicked()), SLOT(movePluginPathUp()));
      connect(pluginPathMoveDown, SIGNAL(clicked()), SLOT(movePluginPathDown()));
      
      connect(audioConvertersButton, SIGNAL(clicked()), SLOT(showAudioConverterSettings()));
      
      connect(noteNamesLoad,    &QPushButton::clicked, [this]() { loadNoteNames(); } );
      connect(noteNamesSave,    &QPushButton::clicked, [this]() { saveNoteNames(); } );
      connect(noteNameAdd,      &QPushButton::clicked, [this]() { addNoteName(); } );
      connect(noteNameInsert,   &QPushButton::clicked, [this]() { insertNoteName(); } );
      connect(noteNameDel,      &QPushButton::clicked, [this]() { delNoteName(); } );
      connect(noteNameMoveUp,   &QPushButton::clicked, [this]() { moveNoteNameUp(); } );
      connect(noteNameMoveDown, &QPushButton::clicked, [this]() { moveNoteNameDown(); } );
      connect(noteNameTable,    &QTableWidget::itemChanged, [this](QTableWidgetItem *item) { noteNamesItemChanged(item); } );

      connect(deviceAudioBackendComboBox, SIGNAL(currentIndexChanged(int)), SLOT(updateBackendDeviceSettings()));

      for (int i = 0; i < MusEGlobal::numRtAudioDevices; i++){
        deviceAudioBackendComboBox->addItem(MusEGlobal::selectableAudioBackendDevices[i],i);
      }
#ifndef HAVE_RTAUDIO
      deviceAudioBackendComboBox->setDisabled(true);
#endif

      for (int i = 0; i < MusEGlobal::numAudioSampleRates; i++){
        deviceAudioRate->addItem(QString::number(MusEGlobal::selectableAudioSampleRates[i]),i);
      }
      updateBackendDeviceSettings();
}

void GlobalSettingsConfig::updateBackendDeviceSettings()
{
    int currentDevice = deviceAudioBackendComboBox->currentIndex();

    if (currentDevice == MusEGlobal::JackAudio)
    {
        deviceAudioSize->setDisabled(true);
        deviceAudioRate->setDisabled(true);
    }
    else {
        deviceAudioSize->setDisabled(false);
        deviceAudioRate->setDisabled(false);

    }
}

void GlobalSettingsConfig::updateStartingMidiNote(const MusECore::NoteNameList &nnl)
{
  const int sz = nnl.size();
  noteNamesMidiStartingNote->setMinimum(0);
  noteNamesMidiStartingNote->setMaximum(sz > 0 ? sz - 1 : 0);
  noteNamesMidiStartingNote->setValue(nnl.startingMidiNote());
}

void GlobalSettingsConfig::updateNoteNames(const MusECore::NoteNameList &nnl)
{
  // Update the backup mirror note list. Make it a copy of the given note list.
  _noteNamesBackup.clear();
  _noteNamesBackup = nnl;

  noteNameListName->setText(nnl.displayName());

  // Prevent notification.
  noteNameTable->blockSignals(true);
  noteNameTable->clearContents();
  noteNameTable->setRowCount(nnl.size());
  noteNameTable->blockSignals(false);

  for(MusECore::NoteNameList::const_iterator i = nnl.cbegin(); i != nnl.cend(); ++i)
  {
    const MusECore::NoteName &nn = *i;
    setupNoteNameRow(nn.noteNum(), nn.firstName(), nn.secondName());
  }

  // Update the note number column.
  updateNoteNumbers();

  // Update the midi starting note range.
  updateStartingMidiNote(nnl);

  noteNamesMidiStartingOctave->setMinimum(-10);
  noteNamesMidiStartingOctave->setMaximum(10);
  noteNamesMidiStartingOctave->setValue(nnl.startingMidiOctave());

//   noteNamesShowPiano->setChecked(nnl.showPiano());
  noteNamesShowPiano->setChecked(MusEGlobal::config.globalShowPiano);
  noteNamesShowNoteColors->setChecked(MusEGlobal::config.pianoShowNoteColors);
}

void GlobalSettingsConfig::applyNoteNames(MusECore::NoteNameList &nnl)
{
  nnl.clear();
  nnl.setDisplayName(noteNameListName->text());
  const int rows = noteNameTable->rowCount();
  for(int i = 0; i < rows; ++i)
  {
    const QString fnn = noteNameTable->item(i, NOTE_FIRST_NAME_COL)->text();
    const QString snn = noteNameTable->item(i, NOTE_SECOND_NAME_COL)->text();
    const MusECore::NoteName nn(i, fnn, snn);
    nnl.insert(i, nn);
  }
  nnl.setStartingMidiNote(noteNamesMidiStartingNote->value());
  nnl.setStartingMidiOctave(noteNamesMidiStartingOctave->value());
  MusEGlobal::config.globalShowPiano = noteNamesShowPiano->isChecked();
  MusEGlobal::config.pianoShowNoteColors = noteNamesShowNoteColors->isChecked();
}

void GlobalSettingsConfig::loadNoteNames()
{
  // Temporary note name list.
  MusECore::NoteNameList nnl;
  // Load the temporary note name list.
  const MusECore::NoteNameList::ReturnResult res = nnl.load();
  if(res == MusECore::NoteNameList::ResultCancelled)
    return;
  if(res == MusECore::NoteNameList::ResultError)
  {
    QMessageBox::critical(this, tr("Load Note Names"), tr("Error reading note name list."));
    return;
  }
  if(res == MusECore::NoteNameList::ResultSuccess)
    // Update the note name table from the temporary list.
    updateNoteNames(nnl);
}

void GlobalSettingsConfig::saveNoteNames()
{
  // Temporary note name list.
  MusECore::NoteNameList nnl;
  // Apply the note name table to the temporary note name list.
  applyNoteNames(nnl);
  // Save the temporary note name list. True on error.
  const MusECore::NoteNameList::ReturnResult res = nnl.save();
  if(res == MusECore::NoteNameList::ResultCancelled)
    return;
  if(res == MusECore::NoteNameList::ResultError)
    QMessageBox::critical(this, tr("Save Note Names"), tr("Error saving note name list."));
}

void GlobalSettingsConfig::addNoteName()
{
  if(!newNoteNameRow(noteNameTable->rowCount()))
    QMessageBox::critical(this, tr("Add Note Name"), tr("Error appending new row."));
}

void GlobalSettingsConfig::insertNoteName()
{
  const int row = noteNameTable->currentRow();
  if(row < 0)
    return;

  if(!newNoteNameRow(row))
    QMessageBox::critical(this, tr("Insert Note Name"), tr("Error inserting new row."));
}

void GlobalSettingsConfig::delNoteName()
{
  const int row = noteNameTable->currentRow();
  if(row < 0)
    return;

  noteNameTable->removeRow(row);

  // Update the note number column starting at row.
  updateNoteNumbers(row);

  // Rebuild the backup mirror note name list.
  applyNoteNames(_noteNamesBackup);

  // Update the midi starting note range.
  updateStartingMidiNote(_noteNamesBackup);
}

void GlobalSettingsConfig::moveNoteNameUp()
{
  const int row = noteNameTable->currentRow();
  if(row <= 0)
    return;

  if(!moveNoteName(row, row - 1))
  {
    QMessageBox::critical(this, tr("Move Note Names"), tr("Error moving note names up."));
    return;
  }

  // Move the selection.
  noteNameTable->selectRow(row - 1);
}

void GlobalSettingsConfig::moveNoteNameDown()
{
  const int row = noteNameTable->currentRow();
  if(row < 0 || row + 1 >= noteNameTable->rowCount())
    return;

  if(!moveNoteName(row, row + 1))
  {
    QMessageBox::critical(this, tr("Move Note Names"), tr("Error moving note names down."));
    return;
  }

  // Move the selection.
  noteNameTable->selectRow(row + 1);
}

void GlobalSettingsConfig::noteNamesItemChanged(QTableWidgetItem *item)
{
  if(!item)
    return;

  const int col = item->column();
  if(col == NOTE_FIRST_NAME_COL || col == NOTE_SECOND_NAME_COL)
  {
    const int row = item->row();
    const QString txt = item->text();

    // Check if the cell's text is empty or is duplicated in any other row's first or second note name.
    bool revert = false;
    // Blank second names are allowed.
    if(col == NOTE_FIRST_NAME_COL && txt.isEmpty())
    {
      revert = true;
      QMessageBox::critical(this, tr("Edit Note Name"), tr("Note first name cannot be blank. Try again."));
    }
    else if(!txt.isEmpty())
    {
      const QLocale loc;
      QCollator coll(loc);
      coll.setCaseSensitivity(Qt::CaseInsensitive);
      const int rows = noteNameTable->rowCount();
      for(int i = 0; i < rows; ++i)
      {
        // Skip the edited table row.
        if(i == row)
          continue;
        const QString dfnn = noteNameTable->item(i, NOTE_FIRST_NAME_COL)->text();
        const QString dsnn = noteNameTable->item(i, NOTE_SECOND_NAME_COL)->text();
        if((!dfnn.isEmpty() && coll.compare(txt, dfnn) == 0) || (!dsnn.isEmpty() && coll.compare(txt, dsnn) == 0))
        {
          revert = true;
          QMessageBox::critical(this, tr("Edit Note Name"),
            tr("The note name entered already exists in another row. Try again."));
          break;
        }
      }
    }
    if(revert)
    {
      // Find the cell's mirror backup note.
      MusECore::NoteNameList::const_iterator bnni = _noteNamesBackup.find(row);
      if(bnni == _noteNamesBackup.cend())
      {
        // This shouldn't happen.
        fprintf(stderr, "GlobalSettingsConfig::noteNamesItemChanged: ERROR: Backup note not found! row:%d col:%d\n",
                item->row(), item->column());
        const QString newname = newNoteName();
        // Prevent notification.
        noteNameTable->blockSignals(true);
        item->setText(newname);
        noteNameTable->blockSignals(false);
        // TODO: What to put here. Create a new backup note list item?
        MusECore::NoteName nn(row, newname, newname);
        _noteNamesBackup.insert(row, nn);
      }
      else
      {
        const MusECore::NoteName &bnn = *bnni;
        // Prevent notification.
        noteNameTable->blockSignals(true);
        // Restore the cell's previous text from the text in the backup note name list.
        if(col == NOTE_FIRST_NAME_COL)
          item->setText(bnn.firstName());
        else if(col == NOTE_SECOND_NAME_COL)
          item->setText(bnn.secondName());
        noteNameTable->blockSignals(false);
      }
    }
    else
    {
      // OK, text not blank and no duplicates found.
      // Find the cell's mirror backup note.
      MusECore::NoteNameList::iterator bnni = _noteNamesBackup.find(row);
      if(bnni == _noteNamesBackup.end())
      {
        // This shouldn't happen.
        fprintf(stderr, "GlobalSettingsConfig::noteNamesItemChanged: ERROR: Backup note not found! row:%d col:%d\n",
                item->row(), item->column());
        // TODO: What to put here. Create a new backup note list item?
        MusECore::NoteName nn(row, txt, txt);
        _noteNamesBackup.insert(row, nn);
      }
      else
      {
        MusECore::NoteName &bnn = *bnni;
        // Update the cell's mirror backup text in the backup note name list.
        if(col == NOTE_FIRST_NAME_COL)
          bnn.setFirstName(txt);
        else if(col == NOTE_SECOND_NAME_COL)
          bnn.setSecondName(txt);
      }
    }
  }
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
      for (unsigned i = 0; i < sizeof(selectableAudioBufSizes)/sizeof(*selectableAudioBufSizes); ++i) {
            if (selectableAudioBufSizes[i] == MusEGlobal::config.deviceAudioBufSize) {
                  deviceAudioSize->setCurrentIndex(i);
                  break;
                  }
            }
      
      for (int i = 0; i < MusEGlobal::numAudioSampleRates; ++i) {
            if (MusEGlobal::selectableAudioSampleRates[i] == MusEGlobal::config.deviceAudioSampleRate) {
                  deviceAudioRate->setCurrentIndex(i);
                  break;
                  }
            }
      for (unsigned i = 0; i < sizeof(minControlProcessPeriods)/sizeof(*minControlProcessPeriods); ++i) {
            if (minControlProcessPeriods[i] == MusEGlobal::config.minControlProcessPeriod) {
                  minControlProcessPeriodComboBox->setCurrentIndex(i);
                  break;
                  }
            }

      autoSaveCheckBox->setChecked(MusEGlobal::config.autoSave);
      scrollableSubmenusCheckbox->setChecked(MusEGlobal::config.scrollableSubMenus);
      liveWaveUpdateCheckBox->setChecked(MusEGlobal::config.liveWaveUpdate);
      preferKnobsVsSlidersCheckBox->setChecked(MusEGlobal::config.preferKnobsVsSliders);
      showControlValuesCheckBox->setChecked(MusEGlobal::config.showControlValues);
      monitorOnRecordCheckBox->setChecked(MusEGlobal::config.monitorOnRecord);
      momentaryMuteCheckBox->setChecked(MusEGlobal::config.momentaryMute);
      momentarySoloCheckBox->setChecked(MusEGlobal::config.momentarySolo);
      lineEditStyleHackCheckBox->setChecked(MusEGlobal::config.lineEditStyleHack);
      showNoteNamesCheckBox->setChecked(MusEGlobal::config.showNoteNamesInPianoRoll);
      showNoteTooltipsCheckBox->setChecked(MusEGlobal::config.showNoteTooltips);
      showTimeScaleBeatNumbersCheckBox->setChecked(MusEGlobal::config.showTimeScaleBeatNumbers);
      preferMidiVolumeDbCheckBox->setChecked(MusEGlobal::config.preferMidiVolumeDb);
      warnIfBadTimingCheckBox->setChecked(MusEGlobal::config.warnIfBadTiming);
      warnOnFileVersionsCheckBox->setChecked(MusEGlobal::config.warnOnFileVersions);
      midiSendInit->setChecked(MusEGlobal::config.midiSendInit);      
      midiWarnInitPending->setChecked(MusEGlobal::config.warnInitPending);      
      midiSendCtlDefaults->setChecked(MusEGlobal::config.midiSendCtlDefaults);      
      sendNullParamsCB->setChecked(MusEGlobal::config.midiSendNullParameters);      
      optimizeControllersCB->setChecked(MusEGlobal::config.midiOptimizeControllers);      
      guiRefreshSelect->setValue(MusEGlobal::config.guiRefresh);
      audioAutomationPointRadius->setValue(MusEGlobal::config.audioAutomationPointRadius);
      minSliderSelect->setValue(int(MusEGlobal::config.minSlider));
      minMeterSelect->setValue(MusEGlobal::config.minMeter);
      freewheelCheckBox->setChecked(MusEGlobal::config.freewheelMode);
      denormalCheckBox->setChecked(MusEGlobal::config.useDenormalBias);
      outputLimiterCheckBox->setChecked(MusEGlobal::config.useOutputLimiter);
      vstInPlaceCheckBox->setChecked(MusEGlobal::config.vstInPlace);
      revertPluginNativeGUIScalingCheckBox->setChecked(MusEGlobal::config.noPluginScaling);
//      openMDIWinMaximizedCheckBox->setChecked(MusEGlobal::config.openMDIWinMaximized);
      keepTransportWindowOnTopCheckBox->setChecked(MusEGlobal::config.keepTransportWindowOnTop);
      showStatusBarCheckBox->setChecked(MusEGlobal::config.showStatusBar);

      deviceAudioBackendComboBox->setCurrentIndex(MusEGlobal::config.deviceAudioBackend);

      enableLatencyCorrectionButton->setChecked(MusEGlobal::config.enableLatencyCorrection);
      latencyInBranchUntermButton->setChecked(MusEGlobal::config.correctUnterminatedInBranchLatency);
      latencyOutBranchUntermButton->setChecked(MusEGlobal::config.correctUnterminatedOutBranchLatency);
      latencyProjectCommonButton->setChecked(MusEGlobal::config.commonProjectLatency);
      latencyMonitorAffectingButton->setChecked(MusEGlobal::config.monitoringAffectsLatency);

      projDirEntry->setText(MusEGlobal::config.projectBaseFolder);

      startSongEntry->setText(MusEGlobal::config.startSong == "" ? "<default>" : MusEGlobal::config.startSong);
      startSongGroup->button(MusEGlobal::config.startMode)->setChecked(true);
      readMidiConfigFromSongCheckBox->setChecked(MusEGlobal::config.startSongLoadConfig);
      
      recDrumGroup->button(MusEGlobal::config.newDrumRecordCondition)->setChecked(true);

      cbEnableLash->setChecked(MusEGlobal::config.enableLash);
#ifndef HAVE_LASH
      cbEnableLash->setEnabled(false);
#endif

      showSplash->setChecked(MusEGlobal::config.showSplashScreen);
      showDidYouKnow->setChecked(MusEGlobal::config.showDidYouKnow);
      externalWavEditorSelect->setText(MusEGlobal::config.externalWavEditor);
      moveArmedCheckBox->setChecked(MusEGlobal::config.moveArmedCheckBox);
      projectSaveCheckBox->setChecked(MusEGlobal::config.useProjectSaveDialog);
      popsDefStayOpenCheckBox->setChecked(MusEGlobal::config.popupsDefaultStayOpen);
      lmbDecreasesCheckBox->setChecked(MusEGlobal::config.leftMouseButtonCanDecrease);
//      rangeMarkerWithoutMMBCheckBox->setChecked(MusEGlobal::config.rangeMarkerWithoutMMB);
      smartFocusCheckBox->setChecked(MusEGlobal::config.smartFocus);
      borderlessMouseCheckBox->setChecked(MusEGlobal::config.borderlessMouse);
      velocityPerNoteCheckBox->setChecked(MusEGlobal::config.velocityPerNote);
      
      addHiddenCheckBox->setChecked(MusEGlobal::config.addHiddenTracks);
      unhideTracksCheckBox->setChecked(MusEGlobal::config.unhideTracks);

      trackHeight->setValue(MusEGlobal::config.trackHeight);
      trackHeightAlternate->setValue(MusEGlobal::config.trackHeightAlternate);

      lv2UiBehaviorComboBox->setCurrentIndex(static_cast<int>(MusEGlobal::config.lv2UiBehavior));

      pluginLadspaPathList->clear();
      pluginLadspaPathList->addItems(MusEGlobal::config.pluginLadspaPathList);

      pluginDssiPathList->clear();
      pluginDssiPathList->addItems(MusEGlobal::config.pluginDssiPathList);

      pluginVstPathList->clear();
      pluginVstPathList->addItems(MusEGlobal::config.pluginVstPathList);

      pluginLinuxVstPathList->clear();
      pluginLinuxVstPathList->addItems(MusEGlobal::config.pluginLinuxVstPathList);

      pluginLv2PathList->clear();
      pluginLv2PathList->addItems(MusEGlobal::config.pluginLv2PathList);

      pluginRescanButton->setChecked(MusEGlobal::config.pluginCacheTriggerRescan);

      cbTabPianoroll->setChecked(TopWin::_openTabbed[TopWin::PIANO_ROLL]);
      cbTabDrum->setChecked(TopWin::_openTabbed[TopWin::DRUM]);
      cbTabWave->setChecked(TopWin::_openTabbed[TopWin::WAVE]);
      cbTabScore->setChecked(TopWin::_openTabbed[TopWin::SCORE]);
      cbTabMaster->setChecked(TopWin::_openTabbed[TopWin::MASTER]);

      cbAMixerDocked->setChecked(MusEGlobal::config.mixerDockedA);
      cbBMixerDocked->setChecked(MusEGlobal::config.mixerDockedB);

      updateNoteNames(MusEGlobal::config.noteNameList);
      noteNamesOctaveOffset->setMinimum(-10);
      noteNamesOctaveOffset->setMaximum(10);
      noteNamesOctaveOffset->setValue(MusEGlobal::config.globalOctaveSuffixOffset);
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
      MusEGlobal::config.audioAutomationPointRadius  = audioAutomationPointRadius->value();
      MusEGlobal::config.minSlider   = minSliderSelect->value();
      MusEGlobal::config.minMeter    = minMeterSelect->value();
      MusEGlobal::config.freewheelMode = freewheelCheckBox->isChecked();
      MusEGlobal::config.useDenormalBias = denormalCheckBox->isChecked();
      MusEGlobal::config.useOutputLimiter = outputLimiterCheckBox->isChecked();
      MusEGlobal::config.vstInPlace  = vstInPlaceCheckBox->isChecked();
      MusEGlobal::config.rtcTicks    = rtcResolutions[rtcticks];
      MusEGlobal::config.warnIfBadTiming = warnIfBadTimingCheckBox->isChecked();
      MusEGlobal::config.warnOnFileVersions = warnOnFileVersionsCheckBox->isChecked();
      MusEGlobal::config.midiSendInit = midiSendInit->isChecked();
      MusEGlobal::config.warnInitPending = midiWarnInitPending->isChecked();
      MusEGlobal::config.midiSendCtlDefaults = midiSendCtlDefaults->isChecked();
      MusEGlobal::config.midiSendNullParameters = sendNullParamsCB->isChecked();
      MusEGlobal::config.midiOptimizeControllers = optimizeControllersCB->isChecked();
      
      MusEGlobal::config.projectBaseFolder = projDirEntry->text();
      
      MusEGlobal::config.enableLatencyCorrection = enableLatencyCorrectionButton->isChecked();
      MusEGlobal::config.correctUnterminatedInBranchLatency = latencyInBranchUntermButton->isChecked();
      MusEGlobal::config.correctUnterminatedOutBranchLatency = latencyOutBranchUntermButton->isChecked();
      MusEGlobal::config.commonProjectLatency = latencyProjectCommonButton->isChecked();
      MusEGlobal::config.monitoringAffectsLatency = latencyMonitorAffectingButton->isChecked();
      
      MusEGlobal::config.startSong   = startSongEntry->text() == "<default>" ? "" : startSongEntry->text();
      MusEGlobal::config.startMode   = startSongGroup->checkedId();
      MusEGlobal::config.startSongLoadConfig = readMidiConfigFromSongCheckBox->isChecked();

      MusEGlobal::config.newDrumRecordCondition = MusECore::newDrumRecordCondition_t(recDrumGroup->checkedId());

      
      int das = deviceAudioSize->currentIndex();
      MusEGlobal::config.deviceAudioBufSize = selectableAudioBufSizes[das];
      MusEGlobal::config.deviceAudioSampleRate = MusEGlobal::selectableAudioSampleRates[deviceAudioRate->currentIndex()];

      MusEGlobal::config.deviceAudioBackend = deviceAudioBackendComboBox->currentIndex();

      int mcp = minControlProcessPeriodComboBox->currentIndex();
      MusEGlobal::config.minControlProcessPeriod = minControlProcessPeriods[mcp];

      int div            = midiDivisionSelect->currentIndex();
      const int new_div  = divisions[div];
      
      MusEGlobal::config.autoSave = autoSaveCheckBox->isChecked();
      MusEGlobal::config.scrollableSubMenus = scrollableSubmenusCheckbox->isChecked();
      MusEGlobal::config.liveWaveUpdate = liveWaveUpdateCheckBox->isChecked();
      MusEGlobal::config.preferKnobsVsSliders = preferKnobsVsSlidersCheckBox->isChecked();
      MusEGlobal::config.showControlValues = showControlValuesCheckBox->isChecked();
      MusEGlobal::config.monitorOnRecord = monitorOnRecordCheckBox->isChecked();
      MusEGlobal::config.momentaryMute = momentaryMuteCheckBox->isChecked();
      MusEGlobal::config.momentarySolo = momentarySoloCheckBox->isChecked();
      MusEGlobal::config.lineEditStyleHack = lineEditStyleHackCheckBox->isChecked();
      MusEGlobal::config.showNoteNamesInPianoRoll = showNoteNamesCheckBox->isChecked();
      MusEGlobal::config.showNoteTooltips = showNoteTooltipsCheckBox->isChecked();
      MusEGlobal::config.showTimeScaleBeatNumbers = showTimeScaleBeatNumbersCheckBox->isChecked();
      MusEGlobal::config.preferMidiVolumeDb = preferMidiVolumeDbCheckBox->isChecked();

      MusEGlobal::config.showSplashScreen = showSplash->isChecked();
      MusEGlobal::config.enableLash = cbEnableLash->isChecked();
      MusEGlobal::config.showDidYouKnow   = showDidYouKnow->isChecked();
      MusEGlobal::config.externalWavEditor = externalWavEditorSelect->text();
      MusEGlobal::config.moveArmedCheckBox = moveArmedCheckBox->isChecked();
      MusEGlobal::config.useProjectSaveDialog = projectSaveCheckBox->isChecked();
      MusEGlobal::config.popupsDefaultStayOpen = popsDefStayOpenCheckBox->isChecked();
      MusEGlobal::config.leftMouseButtonCanDecrease = lmbDecreasesCheckBox->isChecked();
//      MusEGlobal::config.rangeMarkerWithoutMMB = rangeMarkerWithoutMMBCheckBox->isChecked();
      MusEGlobal::config.smartFocus = smartFocusCheckBox->isChecked();
      MusEGlobal::config.borderlessMouse = borderlessMouseCheckBox->isChecked();
      MusEGlobal::config.velocityPerNote = velocityPerNoteCheckBox->isChecked();
      MusEGlobal::config.noPluginScaling = revertPluginNativeGUIScalingCheckBox->isChecked();
      MusEGlobal::config.keepTransportWindowOnTop = keepTransportWindowOnTopCheckBox->isChecked();
      MusEGlobal::config.showStatusBar = showStatusBarCheckBox->isChecked();

      MusEGlobal::config.addHiddenTracks = addHiddenCheckBox->isChecked();
      MusEGlobal::config.unhideTracks = unhideTracksCheckBox->isChecked();

      MusEGlobal::muse->setHeartBeat();        // set guiRefresh
      if(MusEGlobal::midiSeq)
        MusEGlobal::midiSeq->msgSetRtc();        // set midi tick rate
      
      MusEGlobal::config.trackHeight = trackHeight->value();
      MusEGlobal::config.trackHeightAlternate = trackHeightAlternate->value();

      MusEGlobal::config.lv2UiBehavior = static_cast<MusEGlobal::CONF_LV2_UI_BEHAVIOR>(lv2UiBehaviorComboBox->currentIndex());

      
      MusEGlobal::config.pluginLadspaPathList.clear();
      for (int i = 0; i < pluginLadspaPathList->count(); ++i)
            MusEGlobal::config.pluginLadspaPathList << pluginLadspaPathList->item(i)->text();

      MusEGlobal::config.pluginDssiPathList.clear();
      for (int i = 0; i < pluginDssiPathList->count(); ++i)
            MusEGlobal::config.pluginDssiPathList << pluginDssiPathList->item(i)->text();

      MusEGlobal::config.pluginVstPathList.clear();
      for (int i = 0; i < pluginVstPathList->count(); ++i)
            MusEGlobal::config.pluginVstPathList << pluginVstPathList->item(i)->text();

      MusEGlobal::config.pluginLinuxVstPathList.clear();
      for (int i = 0; i < pluginLinuxVstPathList->count(); ++i)
            MusEGlobal::config.pluginLinuxVstPathList << pluginLinuxVstPathList->item(i)->text();

      MusEGlobal::config.pluginLv2PathList.clear();
      for (int i = 0; i < pluginLv2PathList->count(); ++i)
            MusEGlobal::config.pluginLv2PathList << pluginLv2PathList->item(i)->text();
      
      MusEGlobal::config.pluginCacheTriggerRescan = pluginRescanButton->isChecked();
      
      applyNoteNames(MusEGlobal::config.noteNameList);
      MusEGlobal::config.globalOctaveSuffixOffset = noteNamesOctaveOffset->value();

//      applyMdiSettings();
      TopWin::_openTabbed[TopWin::PIANO_ROLL] = cbTabPianoroll->isChecked();
      TopWin::_openTabbed[TopWin::DRUM] = cbTabDrum->isChecked();
      TopWin::_openTabbed[TopWin::WAVE] = cbTabWave->isChecked();
      TopWin::_openTabbed[TopWin::SCORE] = cbTabScore->isChecked();
      TopWin::_openTabbed[TopWin::MASTER] = cbTabMaster->isChecked();

      MusEGlobal::config.mixerDockedA = cbAMixerDocked->isChecked();
      MusEGlobal::config.mixerDockedB = cbBMixerDocked->isChecked();

      // If the division is to be changed, this will RE-NORMALIZE things like the tempo and signature lists,
      //  and RE-FILL the various editor rasterization (snap) table values.
      // The various editors will respond and deal with changing each of their local raster values.
      if(new_div != MusEGlobal::config.division)
        // True = Operation is non-undoable here.
        MusEGlobal::song->applyOperation(MusECore::UndoOp(MusECore::UndoOp::ModifyMidiDivision, new_div, 0, 0, true));
      
      // Save settings. Use simple version - do NOT set style or stylesheet, this has nothing to do with that.
      MusEGlobal::muse->changeConfig(true);
      raise();
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

void GlobalSettingsConfig::showAudioConverterSettings()
{
  if(!MusEGlobal::defaultAudioConverterSettings)
    return;
  MusECore::AudioConverterSettingsGroup* wrk_set = new MusECore::AudioConverterSettingsGroup(false); // Default, non-local settings.
  wrk_set->assign(*MusEGlobal::defaultAudioConverterSettings);
  AudioConverterSettingsDialog dialog(this, 
                                      &MusEGlobal::audioConverterPluginList, 
                                      wrk_set, 
                                      false); // Default, non-local settings.
  if(dialog.exec() == QDialog::Accepted)
  {
    MusECore::PendingOperationList operations;
    MusEGlobal::song->modifyDefaultAudioConverterSettingsOperation(wrk_set, operations);
    if(!operations.empty())
    {
      MusEGlobal::audio->msgExecutePendingOperations(operations, true);
      //MusEGlobal::song->update(SC_);
    }
  }
  else
    delete wrk_set;
}

void GlobalSettingsConfig::addPluginPath()
{
  QString path; 
  switch(pluginPathsTabs->currentIndex())
  {
    case LadspaTab:
      if(pluginLadspaPathList->currentItem())
        path = pluginLadspaPathList->currentItem()->text();
    break;
    
    case DssiTab:
      if(pluginDssiPathList->currentItem())
        path = pluginDssiPathList->currentItem()->text();
    break;
    
    case VstTab:
      if(pluginVstPathList->currentItem())
        path = pluginVstPathList->currentItem()->text();
    break;
    
    case LinuxVstTab:
      if(pluginLinuxVstPathList->currentItem())
        path = pluginLinuxVstPathList->currentItem()->text();
    break;
    
    case Lv2Tab:
      if(pluginLv2PathList->currentItem())
        path = pluginLv2PathList->currentItem()->text();
    break;
    
    default:
    break;
  }
  
  QString new_path = browsePluginPath(path);
  
  if(new_path.isEmpty())
    return;
  
  switch(pluginPathsTabs->currentIndex())
  {
    case LadspaTab:
      pluginLadspaPathList->addItem(new_path);
    break;
    
    case DssiTab:
      pluginDssiPathList->addItem(new_path);
    break;
    
    case VstTab:
      pluginVstPathList->addItem(new_path);
    break;
    
    case LinuxVstTab:
      pluginLinuxVstPathList->addItem(new_path);
    break;
    
    case Lv2Tab:
      pluginLv2PathList->addItem(new_path);
    break;
    
    default:
    break;
  }
}

void GlobalSettingsConfig::editPluginPath()
{
  QString path; 
  switch(pluginPathsTabs->currentIndex())
  {
    case LadspaTab:
      if(pluginLadspaPathList->currentItem())
        path = pluginLadspaPathList->currentItem()->text();
    break;
    
    case DssiTab:
      if(pluginDssiPathList->currentItem())
        path = pluginDssiPathList->currentItem()->text();
    break;
    
    case VstTab:
      if(pluginVstPathList->currentItem())
        path = pluginVstPathList->currentItem()->text();
    break;
    
    case LinuxVstTab:
      if(pluginLinuxVstPathList->currentItem())
        path = pluginLinuxVstPathList->currentItem()->text();
    break;
    
    case Lv2Tab:
      if(pluginLv2PathList->currentItem())
        path = pluginLv2PathList->currentItem()->text();
    break;
    
    default:
    break;
  }
  
  QString new_path = browsePluginPath(path);
  
  if(new_path.isEmpty())
    return;
  
  switch(pluginPathsTabs->currentIndex())
  {
    case LadspaTab:
      if(pluginLadspaPathList->currentItem())
        pluginLadspaPathList->currentItem()->setText(new_path);
    break;
    
    case DssiTab:
      if(pluginDssiPathList->currentItem())
        pluginDssiPathList->currentItem()->setText(new_path);
    break;
    
    case VstTab:
      if(pluginVstPathList->currentItem())
        pluginVstPathList->currentItem()->setText(new_path);
    break;
    
    case LinuxVstTab:
      if(pluginLinuxVstPathList->currentItem())
        pluginLinuxVstPathList->currentItem()->setText(new_path);
    break;
    
    case Lv2Tab:
      if(pluginLv2PathList->currentItem())
        pluginLv2PathList->currentItem()->setText(new_path);
    break;
    
    default:
    break;
  }
}

QString GlobalSettingsConfig::browsePluginPath(const QString& path)
{
  QString dir = QFileDialog::getExistingDirectory(this, 
                                                  qApp->translate("@default", 
                                                    QT_TRANSLATE_NOOP("@default", 
                                                                      "Select plugin directory")), 
                                                  path);
  return dir;
}

void GlobalSettingsConfig::removePluginPath()
{
  switch(pluginPathsTabs->currentIndex())
  {
    case LadspaTab:
      foreach(QListWidgetItem* item, pluginLadspaPathList->selectedItems())
        delete item;
    break;
    
    case DssiTab:
      foreach(QListWidgetItem* item, pluginDssiPathList->selectedItems())
        delete item;
    break;
    
    case VstTab:
      foreach(QListWidgetItem* item, pluginVstPathList->selectedItems())
        delete item;
    break;
    
    case LinuxVstTab:
      foreach(QListWidgetItem* item, pluginLinuxVstPathList->selectedItems())
        delete item;
    break;
    
    case Lv2Tab:
      foreach(QListWidgetItem* item, pluginLv2PathList->selectedItems())
        delete item;
    break;
    
    default:
    return;
  }
}

void GlobalSettingsConfig::movePluginPathUp()
{
  QListWidget* list = 0;
  switch(pluginPathsTabs->currentIndex())
  {
    case LadspaTab:
      list = pluginLadspaPathList;
    break;
    
    case DssiTab:
      list = pluginDssiPathList;
    break;
    
    case VstTab:
      list = pluginVstPathList;
    break;
    
    case LinuxVstTab:
      list = pluginLinuxVstPathList;
    break;
    
    case Lv2Tab:
      list = pluginLv2PathList;
    break;
    
    default:
    break;
  }

  if(list)
  {
    int row = list->currentRow();
    if(row > 0)
    {
      list->insertItem(row - 1, list->takeItem(row));
      list->setCurrentRow(row - 1);
    }
  }
}

void GlobalSettingsConfig::movePluginPathDown()
{
  QListWidget* list = 0;
  switch(pluginPathsTabs->currentIndex())
  {
    case LadspaTab:
      list = pluginLadspaPathList;
    break;
    
    case DssiTab:
      list = pluginDssiPathList;
    break;
    
    case VstTab:
      list = pluginVstPathList;
    break;
    
    case LinuxVstTab:
      list = pluginLinuxVstPathList;
    break;
    
    case Lv2Tab:
      list = pluginLv2PathList;
    break;
    
    default:
    break;
  }

  if(list)
  {
    int row = list->currentRow();
    if(row + 1 < list->count())
    {
      list->insertItem(row + 1, list->takeItem(row));
      list->setCurrentRow(row + 1);
    }
  }
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
  startSongEntry->setText("<default>");
  readMidiConfigFromSongCheckBox->setChecked(false);
}

QString GlobalSettingsConfig::newNoteName() const
{
  const QString newname(tr("NewNote"));
  int suf;
  QString n;
  const QTableWidgetItem *item;
  const int rows = noteNameTable->rowCount();
  QLocale loc;
  QCollator coll(loc);
  for(suf = 1; suf < 129; ++suf)
  {
    n = newname;
    if(suf != 1)
      n = newname + loc.toString(suf);

    bool found = false;
    for(int i = 0; i < rows; ++i)
    {
      QString fnn;
      QString snn;
      item = noteNameTable->item(i, NOTE_FIRST_NAME_COL);
      if(item)
        fnn = item->text();
      item = noteNameTable->item(i, NOTE_SECOND_NAME_COL);
      if(item)
        snn = item->text();

      if((!fnn.isEmpty() && coll.compare(fnn, n) == 0) || (!snn.isEmpty() && coll.compare(snn, n) == 0))
      {
        found = true;
        break;
      }
    }
    if(!found)
      break;
  }
  return n;
}

bool GlobalSettingsConfig::setupNoteNameRow(int row, const QString &sharpName, const QString &flatName)
{
  if(row >= noteNameTable->rowCount())
    return false;

  // Prevent notification.
  noteNameTable->blockSignals(true);

  QTableWidgetItem *wi = new QTableWidgetItem(sharpName);
  wi->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable | Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled);
  wi->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
  noteNameTable->setItem(row, NOTE_FIRST_NAME_COL, wi);

  wi = new QTableWidgetItem(flatName);
  wi->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable | Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled);
  wi->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
  noteNameTable->setItem(row, NOTE_SECOND_NAME_COL, wi);

  noteNameTable->blockSignals(false);

  return true;
}

bool GlobalSettingsConfig::newNoteNameRow(int row)
{
  const int rows = noteNameTable->rowCount();
  // More than 128 notes not allowed.
  if(rows >= 128)
    return false;
  if(row >= rows)
    row = rows;

  const QString newname = newNoteName();
  if(newname.isEmpty())
  {
    QMessageBox::critical(this, tr("Note Names"), tr("Error creating new unique note name."));
    return false;
  }

  // Append a new row if past the end. Prevent notification.
  // Tested: This works OK if it's at the end, but not some distance past the end.
  noteNameTable->blockSignals(true);
  noteNameTable->insertRow(row);
  noteNameTable->blockSignals(false);

  // Check if a new row was created.
  const int newrows = noteNameTable->rowCount();
  if(newrows != rows + 1)
  {
    QMessageBox::critical(this, tr("Note Names"), tr("Error setting up note name row."));
    return false;
  }

  if(!setupNoteNameRow(row, newname, newname))
  {
    noteNameTable->blockSignals(true);
    noteNameTable->removeRow(row);
    noteNameTable->blockSignals(false);
    QMessageBox::critical(this, tr("Note Names"), tr("Error setting up new note name row."));
    return false;
  }

  // Update the note number column starting at row.
  updateNoteNumbers(row);

  // Rebuild the backup mirror note name list.
  applyNoteNames(_noteNamesBackup);

  // Update the midi starting note range.
  updateStartingMidiNote(_noteNamesBackup);

  return true;
}

void GlobalSettingsConfig::updateNoteNumbers(int startRow)
{
  const int rows = noteNameTable->rowCount();
  if(startRow >= rows)
    return;
  if(startRow < 0)
    startRow = 0;
  const QLocale loc;
  // Prevent notification.
  noteNameTable->blockSignals(true);
  for(int i = startRow; i < rows; ++i)
    noteNameTable->setVerticalHeaderItem(i, new QTableWidgetItem(loc.toString(i)));
  noteNameTable->blockSignals(false);
}

bool GlobalSettingsConfig::moveNoteName(int from, int to)
{
  const int rows = noteNameTable->rowCount();
  if(from == to || from < 0 || to < 0 || from >= rows || to >= rows)
    return false;

  // Prevent notification.
  noteNameTable->blockSignals(true);

  // Take the items at 'from'.
  QTableWidgetItem *fwi_f = noteNameTable->takeItem(from, NOTE_FIRST_NAME_COL);
  QTableWidgetItem *fwi_s = noteNameTable->takeItem(from, NOTE_SECOND_NAME_COL);

  // The items at 'from' should be empty now.
  // Shift all the items towards 'from'.
  if(from < to)
  {
    for(int i = from; i < to; ++i)
    {
      QTableWidgetItem *wi_f = noteNameTable->takeItem(i + 1, NOTE_FIRST_NAME_COL);
      QTableWidgetItem *wi_s = noteNameTable->takeItem(i + 1, NOTE_SECOND_NAME_COL);
      noteNameTable->setItem(i, NOTE_FIRST_NAME_COL, wi_f);
      noteNameTable->setItem(i, NOTE_SECOND_NAME_COL, wi_s);
    }
  }
  else
  {
    for(int i = from; i > to; --i)
    {
      QTableWidgetItem *wi_f = noteNameTable->takeItem(i - 1, NOTE_FIRST_NAME_COL);
      QTableWidgetItem *wi_s = noteNameTable->takeItem(i - 1, NOTE_SECOND_NAME_COL);
      noteNameTable->setItem(i, NOTE_FIRST_NAME_COL, wi_f);
      noteNameTable->setItem(i, NOTE_SECOND_NAME_COL, wi_s);
    }
  }

  // The items at 'to' should be empty now. Put the items we took from 'from' at 'to'.
  noteNameTable->setItem(to, NOTE_FIRST_NAME_COL, fwi_f);
  noteNameTable->setItem(to, NOTE_SECOND_NAME_COL, fwi_s);

  noteNameTable->blockSignals(false);

  // Rebuild the backup mirror note name list.
  applyNoteNames(_noteNamesBackup);

  return true;
}

void GlobalSettingsConfig::selectTab(int tab) const
{
  TabWidget2->setCurrentIndex(tab);
}

} // namespace MusEGui

