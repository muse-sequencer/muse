//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: mrconfig.cpp,v 1.1.1.1 2003/10/27 18:52:43 wschweer Exp $
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

#include "mrconfig.h"
#include "globals.h"
#include "globaldefs.h"
#include "midi_consts.h"
#include "mididev.h"
#include "midiport.h"
#include "app.h"
#include "song.h"
#include "audio.h"
#include "operations.h"

#include <QMessageBox>

// Forwards from header:
#include "pitchedit.h"
#include <QCloseEvent>
#include <QShowEvent>
#include <QWidget>
#include <QComboBox>
#include <QSpinBox>
#include <QCheckBox>
#include <QPushButton>

namespace MusEGui {

//---------------------------------------------------------
//   MRConfig
//    Midi Remote Control Config
//---------------------------------------------------------

MRConfig::MRConfig(QWidget* parent, Qt::WindowFlags fl)
    : QWidget(parent, fl)
{
    setupUi(this);

    updateValues();

    // Special for these: Need qt helper overload for these lambdas.
    connect(stopNotePort, QOverload<int>::of(&QComboBox::currentIndexChanged), [=]() { settingChanged(); } );
    connect(playNotePort, QOverload<int>::of(&QComboBox::currentIndexChanged), [=]() { settingChanged(); } );
    connect(recNotePort, QOverload<int>::of(&QComboBox::currentIndexChanged), [=]() { settingChanged(); } );
    connect(goLMarkNotePort, QOverload<int>::of(&QComboBox::currentIndexChanged), [=]() { settingChanged(); } );
    connect(ffNotePort, QOverload<int>::of(&QComboBox::currentIndexChanged), [=]() { settingChanged(); } );
    connect(rewNotePort, QOverload<int>::of(&QComboBox::currentIndexChanged), [=]() { settingChanged(); } );
    connect(stepRecPort, QOverload<int>::of(&QComboBox::currentIndexChanged), [=]() { settingChanged(); } );
    connect(stepRecRestNotePort, QOverload<int>::of(&QComboBox::currentIndexChanged), [=]() { settingChanged(); } );

    connect(stopCCPort, QOverload<int>::of(&QComboBox::currentIndexChanged), [=]() { settingChanged(); } );
    connect(playCCPort, QOverload<int>::of(&QComboBox::currentIndexChanged), [=]() { settingChanged(); } );
    connect(recCCPort, QOverload<int>::of(&QComboBox::currentIndexChanged), [=]() { settingChanged(); } );
    connect(goLMarkCCPort, QOverload<int>::of(&QComboBox::currentIndexChanged), [=]() { settingChanged(); } );
    connect(ffCCPort, QOverload<int>::of(&QComboBox::currentIndexChanged), [=]() { settingChanged(); } );
    connect(rewCCPort, QOverload<int>::of(&QComboBox::currentIndexChanged), [=]() { settingChanged(); } );
    connect(stepRecRestCCPort, QOverload<int>::of(&QComboBox::currentIndexChanged), [=]() { settingChanged(); } );

    connect(stopNoteChan, QOverload<int>::of(&QComboBox::currentIndexChanged), [=]() { settingChanged(); } );
    connect(playNoteChan, QOverload<int>::of(&QComboBox::currentIndexChanged), [=]() { settingChanged(); } );
    connect(recNoteChan, QOverload<int>::of(&QComboBox::currentIndexChanged), [=]() { settingChanged(); } );
    connect(goLMarkNoteChan, QOverload<int>::of(&QComboBox::currentIndexChanged), [=]() { settingChanged(); } );
    connect(ffNoteChan, QOverload<int>::of(&QComboBox::currentIndexChanged), [=]() { settingChanged(); } );
    connect(rewNoteChan, QOverload<int>::of(&QComboBox::currentIndexChanged), [=]() { settingChanged(); } );
    connect(stepRecChan, QOverload<int>::of(&QComboBox::currentIndexChanged), [=]() { settingChanged(); } );
    connect(stepRecRestNoteChan, QOverload<int>::of(&QComboBox::currentIndexChanged), [=]() { settingChanged(); } );

    connect(stopCCChan, QOverload<int>::of(&QComboBox::currentIndexChanged), [=]() { settingChanged(); } );
    connect(playCCChan, QOverload<int>::of(&QComboBox::currentIndexChanged), [=]() { settingChanged(); } );
    connect(recCCChan, QOverload<int>::of(&QComboBox::currentIndexChanged), [=]() { settingChanged(); } );
    connect(goLMarkCCChan, QOverload<int>::of(&QComboBox::currentIndexChanged), [=]() { settingChanged(); } );
    connect(ffCCChan, QOverload<int>::of(&QComboBox::currentIndexChanged), [=]() { settingChanged(); } );
    connect(rewCCChan, QOverload<int>::of(&QComboBox::currentIndexChanged), [=]() { settingChanged(); } );
    connect(stepRecRestCCChan, QOverload<int>::of(&QComboBox::currentIndexChanged), [=]() { settingChanged(); } );

    connect(stopCCNum, QOverload<int>::of(&QSpinBox::valueChanged), [=]() { settingChanged(); } );
    connect(playCCNum, QOverload<int>::of(&QSpinBox::valueChanged), [=]() { settingChanged(); } );
    connect(recCCNum, QOverload<int>::of(&QSpinBox::valueChanged), [=]() { settingChanged(); } );
    connect(goLMarkCCNum, QOverload<int>::of(&QSpinBox::valueChanged), [=]() { settingChanged(); } );
    connect(ffCCNum, QOverload<int>::of(&QSpinBox::valueChanged), [=]() { settingChanged(); } );
    connect(rewCCNum, QOverload<int>::of(&QSpinBox::valueChanged), [=]() { settingChanged(); } );
    connect(stepRecRestCCNum, QOverload<int>::of(&QSpinBox::valueChanged), [=]() { settingChanged(); } );

    connect(playNoteValType, QOverload<int>::of(&QComboBox::currentIndexChanged), [=]() { settingChanged(); } );
    connect(recNoteValType, QOverload<int>::of(&QComboBox::currentIndexChanged), [=]() { settingChanged(); } );
    connect(ffNoteValType, QOverload<int>::of(&QComboBox::currentIndexChanged), [=]() { settingChanged(); } );
    connect(rewNoteValType, QOverload<int>::of(&QComboBox::currentIndexChanged), [=]() { settingChanged(); } );

    connect(playCCValType, QOverload<int>::of(&QComboBox::currentIndexChanged), [=]() { settingChanged(); } );
    connect(recCCValType, QOverload<int>::of(&QComboBox::currentIndexChanged), [=]() { settingChanged(); } );
    connect(ffCCValType, QOverload<int>::of(&QComboBox::currentIndexChanged), [=]() { settingChanged(); } );
    connect(rewCCValType, QOverload<int>::of(&QComboBox::currentIndexChanged), [=]() { settingChanged(); } );

    connect(stopNote, QOverload<int>::of(&PitchEdit::valueChanged), [=]() { settingChanged(); } );
    connect(playNote, QOverload<int>::of(&PitchEdit::valueChanged), [=]() { settingChanged(); } );
    connect(recNote, QOverload<int>::of(&PitchEdit::valueChanged), [=]() { settingChanged(); } );
    connect(goLMarkNote, QOverload<int>::of(&PitchEdit::valueChanged), [=]() { settingChanged(); } );
    connect(ffNote, QOverload<int>::of(&PitchEdit::valueChanged), [=]() { settingChanged(); } );
    connect(rewNote, QOverload<int>::of(&PitchEdit::valueChanged), [=]() { settingChanged(); } );
    connect(stepRecRestNote, QOverload<int>::of(&PitchEdit::valueChanged), [=]() { settingChanged(); } );

    connect(stopUseNote, &QCheckBox::toggled, [this]() { settingChanged(); } );
    connect(playUseNote, &QCheckBox::toggled, [this]() { settingChanged(); } );
    connect(recUseNote, &QCheckBox::toggled, [this]() { settingChanged(); } );
    connect(goLMarkUseNote, &QCheckBox::toggled, [this]() { settingChanged(); } );
    connect(ffUseNote, &QCheckBox::toggled, [this]() { settingChanged(); } );
    connect(rewUseNote, &QCheckBox::toggled, [this]() { settingChanged(); } );
    connect(stepRecRestUseNote, &QCheckBox::toggled, [this]() { settingChanged(); } );

    connect(stopUseCC, &QCheckBox::toggled, [this]() { settingChanged(); } );
    connect(playUseCC, &QCheckBox::toggled, [this]() { settingChanged(); } );
    connect(recUseCC, &QCheckBox::toggled, [this]() { settingChanged(); } );
    connect(goLMarkUseCC, &QCheckBox::toggled, [this]() { settingChanged(); } );
    connect(ffUseCC, &QCheckBox::toggled, [this]() { settingChanged(); } );
    connect(rewUseCC, &QCheckBox::toggled, [this]() { settingChanged(); } );
    connect(stepRecRestUseCC, &QCheckBox::toggled, [this]() { settingChanged(); } );


    connect(learnStopNote, &QPushButton::clicked, [this](bool v) { learnChanged(learnStopNote, v); } );
    connect(learnPlayNote, &QPushButton::clicked, [this](bool v) { learnChanged(learnPlayNote, v); } );
    connect(learnRecNote, &QPushButton::clicked, [this](bool v) { learnChanged(learnRecNote, v); } );
    connect(learnGoLMarkNote, &QPushButton::clicked, [this](bool v) { learnChanged(learnGoLMarkNote, v); } );
    connect(learnFFNote, &QPushButton::clicked, [this](bool v) { learnChanged(learnFFNote, v); } );
    connect(learnRewNote, &QPushButton::clicked, [this](bool v) { learnChanged(learnRewNote, v); } );
    connect(learnStepRecNote, &QPushButton::clicked, [this](bool v) { learnChanged(learnStepRecNote, v); } );
    connect(learnStepRecRestNote, &QPushButton::clicked, [this](bool v) { learnChanged(learnStepRecRestNote, v); } );

    connect(learnStopCC, &QPushButton::clicked, [this](bool v) { learnChanged(learnStopCC, v); } );
    connect(learnPlayCC, &QPushButton::clicked, [this](bool v) { learnChanged(learnPlayCC, v); } );
    connect(learnRecCC, &QPushButton::clicked, [this](bool v) { learnChanged(learnRecCC, v); } );
    connect(learnGoLMarkCC, &QPushButton::clicked, [this](bool v) { learnChanged(learnGoLMarkCC, v); } );
    connect(learnFFCC, &QPushButton::clicked, [this](bool v) { learnChanged(learnFFCC, v); } );
    connect(learnRewCC, &QPushButton::clicked, [this](bool v) { learnChanged(learnRewCC, v); } );
    connect(learnStepRecRestCC, &QPushButton::clicked, [this](bool v) { learnChanged(learnStepRecRestCC, v); } );

    connect(globalSettingsButton, &QRadioButton::toggled, [this]() { switchSettings(); } );

    connect(buttonApply, &QPushButton::clicked, [this]() { apply(); } );
    connect(buttonOk, &QPushButton::clicked, [this]() { accept(); } );
    connect(buttonCancel, &QPushButton::clicked, [this]() { reject(); } );

    connect(initializeSettings, &QPushButton::clicked, [this]() { resetPressed(); } );
    connect(copySettings, &QPushButton::clicked, [this]() { copyPressed(); } );

    _songChangedConn =
       connect(MusEGlobal::song, &MusECore::Song::songChanged, [this](MusECore::SongChangedStruct_t type) { songChanged(type); } );
    _configChangedConn = connect(MusEGlobal::muse, &MusEGui::MusE::configChanged, this, &MRConfig::configChanged);
    _learnReceivedConn = connect(MusEGlobal::song, &MusECore::Song::midiLearnReceived, this, &MRConfig::midiLearnReceived);
}

MRConfig::~MRConfig()
{
  // Clear all the learn settings.
  // TODO: The widget is not modal and can stay open during use.
  //       This destructor is only called at program end.
  //       Therefore we don't want (or need) to call this because it sends a command
  //        and the command processing will have been shut down by now.
  //       On the other hand, if we ever make this widget self-deleting on close,
  //        we will want this to clear the learn settings before re-opening.
  //clearLearnSettings();
  MusEGlobal::midiRemoteIsLearning = false;

  disconnect(_songChangedConn);
  disconnect(_configChangedConn);
  disconnect(_learnReceivedConn);
}

//---------------------------------------------------------
//   showEvent
//---------------------------------------------------------

void MRConfig::showEvent(QShowEvent* ev)
{
    // "Note: A widget receives spontaneous show and hide events when its mapping status is changed
    //   by the window system, e.g. a spontaneous hide event when the user minimizes the window,
    //   and a spontaneous show event when the window is restored again." - Qt help.
    // We only want 'real' show events here, not restoration-caused events.
    if(!ev->spontaneous())
    {
      // Clear all the learn settings.
      clearLearnSettings();
      // Make sure what is on screen matches what is in the structures which might have changed by now.
      updateValues();
    }
    QWidget::showEvent(ev);
}

//---------------------------------------------------------
//   closeEvent
//---------------------------------------------------------

void MRConfig::closeEvent(QCloseEvent* ev)
{
    // Tested: closeEvent is NOT called if closing the app while the window is open.

    // Clear all the learn buttons and settings.
    clearLearnSettings();

    emit hideWindow();
    QWidget::closeEvent(ev);
}

void MRConfig::configChanged()
{
  updateDialog();
}

void MRConfig::setupPortList(QComboBox *cb, int curPort)
{
  cb->blockSignals(true);
  cb->clear();

  cb->addItem(tr("Any"), -1);

  for (int i = 0; i < MusECore::MIDI_PORTS; ++i)
  {
    const MusECore::MidiDevice* md = MusEGlobal::midiPorts[i].device();
    if(!md)  // In the case of this combo box, don't bother listing empty ports.
      continue;
    //if(!(md->rwFlags() & 1 || md->isSynti()) && (i != outPort))
    if(!(md->rwFlags() & 2) && (i != curPort))   // Only readable ports, or current one.
      continue;
    const QString name = QString("%1:%2").arg(i + 1).arg(MusEGlobal::midiPorts[i].portname());
    cb->addItem(name, i);
  }

  const int idx = cb->findData(curPort);
  if(idx == -1)
    fprintf(stderr, "MRConfig::setupPortList: Port not found!:%d\n", curPort);
  else
    cb->setCurrentIndex(idx);

  cb->blockSignals(false);
}

void MRConfig::setupChannelList(QComboBox *cb, int curChan)
{
  cb->blockSignals(true);
  cb->clear();

  cb->addItem(tr("Any"), -1);

  for (int i = 0; i < MusECore::MUSE_MIDI_CHANNELS; ++i)
    cb->addItem(QString::number(i + 1), i);

  const int idx = cb->findData(curChan);
  if(idx == -1)
    fprintf(stderr, "MRConfig::setupChannelList: Channel not found!:%d\n", curChan);
  else
    cb->setCurrentIndex(idx);

  cb->blockSignals(false);
}

void MRConfig::setupCCNumList(QSpinBox *sb, int curCCNum)
{
  sb->blockSignals(true);
  sb->setRange(0, 127);
  sb->setValue(curCCNum);
  sb->blockSignals(false);
}

void MRConfig::setupValTypeList(QComboBox *cb, int curValType)
{
  cb->blockSignals(true);
  cb->clear();

  cb->addItem(tr("Trigger"), MusECore::MidiRemoteStruct::MidiRemoteValTrigger);
  cb->addItem(tr("Toggle"), MusECore::MidiRemoteStruct::MidiRemoteValToggle);
  cb->addItem(tr("Momentary"), MusECore::MidiRemoteStruct::MidiRemoteValMomentary);

  const int idx = cb->findData(curValType);
  if(idx == -1)
    fprintf(stderr, "MRConfig::setupValTypeList: Type not found!:%d\n", curValType);
  else
    cb->setCurrentIndex(idx);

  cb->blockSignals(false);
}

void MRConfig::settingChanged()
{
  buttonApply->setEnabled(true);
  buttonOk->setEnabled(true);
}

void MRConfig::learnChanged(QPushButton *pb, bool /*newval*/)
{
  // NOTE: Setting all the learn buttons to 'exclusive' does not help because
  //        we need them to be uncheckable, which 'exclusive' does not allow.

  learnStopNote->blockSignals(true);
  learnPlayNote->blockSignals(true);
  learnRecNote->blockSignals(true);
  learnGoLMarkNote->blockSignals(true);
  learnFFNote->blockSignals(true);
  learnRewNote->blockSignals(true);
  learnStepRecNote->blockSignals(true);
  learnStepRecRestNote->blockSignals(true);

  learnStopCC->blockSignals(true);
  learnPlayCC->blockSignals(true);
  learnRecCC->blockSignals(true);
  learnGoLMarkCC->blockSignals(true);
  learnFFCC->blockSignals(true);
  learnRewCC->blockSignals(true);
  learnStepRecRestCC->blockSignals(true);

  if(pb != learnStopNote)
    learnStopNote->setChecked(false);
  if(pb != learnPlayNote)
    learnPlayNote->setChecked(false);
  if(pb != learnRecNote)
    learnRecNote->setChecked(false);
  if(pb != learnGoLMarkNote)
    learnGoLMarkNote->setChecked(false);
  if(pb != learnFFNote)
    learnFFNote->setChecked(false);
  if(pb != learnRewNote)
    learnRewNote->setChecked(false);
  if(pb != learnStepRecNote)
    learnStepRecNote->setChecked(false);
  if(pb != learnStepRecRestNote)
    learnStepRecRestNote->setChecked(false);

  if(pb != learnStopCC)
    learnStopCC->setChecked(false);
  if(pb != learnPlayCC)
    learnPlayCC->setChecked(false);
  if(pb != learnRecCC)
    learnRecCC->setChecked(false);
  if(pb != learnGoLMarkCC)
    learnGoLMarkCC->setChecked(false);
  if(pb != learnFFCC)
    learnFFCC->setChecked(false);
  if(pb != learnRewCC)
    learnRewCC->setChecked(false);
  if(pb != learnStepRecRestCC)
    learnStepRecRestCC->setChecked(false);

  learnStopNote->blockSignals(false);
  learnPlayNote->blockSignals(false);
  learnRecNote->blockSignals(false);
  learnGoLMarkNote->blockSignals(false);
  learnFFNote->blockSignals(false);
  learnRewNote->blockSignals(false);
  learnStepRecNote->blockSignals(false);
  learnStepRecRestNote->blockSignals(false);

  learnStopCC->blockSignals(false);
  learnPlayCC->blockSignals(false);
  learnRecCC->blockSignals(false);
  learnGoLMarkCC->blockSignals(false);
  learnFFCC->blockSignals(false);
  learnRewCC->blockSignals(false);
  learnStepRecRestCC->blockSignals(false);

  MusEGlobal::midiRemoteIsLearning =
    learnStopNote->isChecked() ||
    learnPlayNote->isChecked() ||
    learnRecNote->isChecked() ||
    learnGoLMarkNote->isChecked() ||
    learnFFNote->isChecked() ||
    learnRewNote->isChecked() ||
    learnStepRecNote->isChecked() ||
    learnStepRecRestNote->isChecked() ||

    learnStopCC->isChecked() ||
    learnPlayCC->isChecked() ||
    learnRecCC->isChecked() ||
    learnGoLMarkCC->isChecked() ||
    learnFFCC->isChecked() ||
    learnRewCC->isChecked() ||
    learnStepRecRestCC->isChecked();
}

void MRConfig::updateDialog()
{
  setupPortList(stopNotePort, _curMidiRemote->_stop._noteport);
  setupPortList(playNotePort, _curMidiRemote->_play._noteport);
  setupPortList(recNotePort, _curMidiRemote->_rec._noteport);
  setupPortList(goLMarkNotePort, _curMidiRemote->_gotoLeftMark._noteport);
  setupPortList(ffNotePort, _curMidiRemote->_forward._noteport);
  setupPortList(rewNotePort, _curMidiRemote->_backward._noteport);
  setupPortList(stepRecPort, _curMidiRemote->_stepRecPort);
  setupPortList(stepRecRestNotePort, _curMidiRemote->_stepRecRest._noteport);

  setupPortList(stopCCPort, _curMidiRemote->_stop._ccport);
  setupPortList(playCCPort, _curMidiRemote->_play._ccport);
  setupPortList(recCCPort, _curMidiRemote->_rec._ccport);
  setupPortList(goLMarkCCPort, _curMidiRemote->_gotoLeftMark._ccport);
  setupPortList(ffCCPort, _curMidiRemote->_forward._ccport);
  setupPortList(rewCCPort, _curMidiRemote->_backward._ccport);
  setupPortList(stepRecRestCCPort, _curMidiRemote->_stepRecRest._ccport);
}

void MRConfig::clearLearnSettings() const
{
  MusEGlobal::midiRemoteIsLearning = false;

  learnStopCC->blockSignals(true);
  learnPlayCC->blockSignals(true);
  learnRecCC->blockSignals(true);
  learnGoLMarkCC->blockSignals(true);
  learnFFCC->blockSignals(true);
  learnRewCC->blockSignals(true);
  learnStepRecRestCC->blockSignals(true);

  learnStopNote->setChecked(false);
  learnPlayNote->setChecked(false);
  learnRecNote->setChecked(false);
  learnGoLMarkNote->setChecked(false);
  learnFFNote->setChecked(false);
  learnRewNote->setChecked(false);
  learnStepRecNote->setChecked(false);
  learnStepRecRestNote->setChecked(false);

  learnStopCC->setChecked(false);
  learnPlayCC->setChecked(false);
  learnRecCC->setChecked(false);
  learnGoLMarkCC->setChecked(false);
  learnFFCC->setChecked(false);
  learnRewCC->setChecked(false);
  learnStepRecRestCC->setChecked(false);

  learnStopNote->blockSignals(false);
  learnPlayNote->blockSignals(false);
  learnRecNote->blockSignals(false);
  learnGoLMarkNote->blockSignals(false);
  learnFFNote->blockSignals(false);
  learnRewNote->blockSignals(false);
  learnStepRecNote->blockSignals(false);
  learnStepRecRestNote->blockSignals(false);

  learnStopCC->blockSignals(false);
  learnPlayCC->blockSignals(false);
  learnRecCC->blockSignals(false);
  learnGoLMarkCC->blockSignals(false);
  learnFFCC->blockSignals(false);
  learnRewCC->blockSignals(false);
  learnStepRecRestCC->blockSignals(false);
}

void MRConfig::midiLearnReceived(const MusECore::MidiRecordEvent& ev)
{
  if(learnStopNote->isChecked())
        assignLearnNote(ev,
      stopUseNote, stopNotePort, stopNoteChan, stopNote);

  else if(learnPlayNote->isChecked())
        assignLearnNote(ev,
      playUseNote, playNotePort, playNoteChan, playNote);

  else if(learnRecNote->isChecked())
        assignLearnNote(ev,
      recUseNote, recNotePort, recNoteChan, recNote);

  else if(learnGoLMarkNote->isChecked())
        assignLearnNote(ev,
      goLMarkUseNote, goLMarkNotePort, goLMarkNoteChan, goLMarkNote);

  else if(learnFFNote->isChecked())
        assignLearnNote(ev,
      ffUseNote, ffNotePort, ffNoteChan, ffNote);

  else if(learnRewNote->isChecked())
        assignLearnNote(ev,
      rewUseNote, rewNotePort, rewNoteChan, rewNote);

  else if(learnStepRecRestNote->isChecked())
        assignLearnNote(ev,
      stepRecRestUseNote, stepRecRestNotePort, stepRecRestNoteChan, stepRecRestNote);

  else if(learnStepRecNote->isChecked())
        assignLearnNote(ev,
      nullptr, stepRecPort, stepRecChan, nullptr);


  else if(learnStopCC->isChecked())
        assignLearnCC(ev,
      stopUseCC, stopCCPort, stopCCChan, stopCCNum);

  else if(learnPlayCC->isChecked())
        assignLearnCC(ev,
      playUseCC, playCCPort, playCCChan, playCCNum);

  else if(learnRecCC->isChecked())
        assignLearnCC(ev,
      recUseCC, recCCPort, recCCChan, recCCNum);

  else if(learnGoLMarkCC->isChecked())
        assignLearnCC(ev,
      goLMarkUseCC, goLMarkCCPort, goLMarkCCChan, goLMarkCCNum);

  else if(learnFFCC->isChecked())
        assignLearnCC(ev,
      ffUseCC, ffCCPort, ffCCChan, ffCCNum);

  else if(learnRewCC->isChecked())
        assignLearnCC(ev,
      rewUseCC, rewCCPort, rewCCChan, rewCCNum);

  else if(learnStepRecRestCC->isChecked())
        assignLearnCC(ev,
      stepRecRestUseCC, stepRecRestCCPort, stepRecRestCCChan, stepRecRestCCNum);
}

void MRConfig::assignLearnNote(const MusECore::MidiRecordEvent& ev,
  QCheckBox *noteEn, QComboBox *notePort, QComboBox *noteChan, PitchEdit *notePitch)
{
  const int type = ev.type();
  const int port = ev.port();
  const int chan = ev.channel();
  const int dataA = ev.dataA();

  if(type == MusECore::ME_NOTEON || type == MusECore::ME_NOTEOFF)
  {
    if(notePort)
    {
      selectPort(notePort, port);
      settingChanged();
    }

    if(noteChan)
    {
      selectChannel(noteChan, chan);
      settingChanged();
    }

    if(notePitch)
    {
      notePitch->blockSignals(true);
      notePitch->setValue(dataA);
      notePitch->blockSignals(false);
      settingChanged();
    }

    if(noteEn)
    {
      noteEn->blockSignals(true);
      noteEn->setChecked(true);
      noteEn->blockSignals(false);
      settingChanged();
    }
  }
}

void MRConfig::assignLearnCC(const MusECore::MidiRecordEvent& ev,
  QCheckBox *ccEn, QComboBox *ccPort, QComboBox *ccChan, QSpinBox *ccNum)
{
  const int type = ev.type();
  const int port = ev.port();
  const int chan = ev.channel();
  const int dataA = ev.dataA();

  if(type == MusECore::ME_CONTROLLER)
  {
    if(ccPort)
    {
      selectPort(ccPort, port);
      settingChanged();
    }

    if(ccChan)
    {
      selectChannel(ccChan, chan);
      settingChanged();
    }

    if(ccNum)
    {
      ccNum->blockSignals(true);
      ccNum->setValue(dataA);
      ccNum->blockSignals(false);
      settingChanged();
    }

    if(ccEn)
    {
      ccEn->blockSignals(true);
      ccEn->setChecked(true);
      ccEn->blockSignals(false);
      settingChanged();
    }
  }
}

void MRConfig::selectPort(QComboBox *cb, int port)
{
  if(port < 0 || port >= MusECore::MIDI_PORTS)
  {
    fprintf(stderr, "MRConfig::selectPort: Invalid port:%d\n", port);
    return;
  }
  int idx = cb->findData(port);
  // If the port is not found in the list, add it now.
  if(idx == -1)
  {
    const QString pname = MusEGlobal::midiPorts[port].portname();
    const QString name = QString("%1:%2").arg(port + 1).arg(pname);
    cb->addItem(name, port);
  }
  idx = cb->findData(port);
  if(idx == -1)
  {
    fprintf(stderr, "MRConfig::selectPort: Port not found!:%d\n", port);
  }
  else
  {
    cb->blockSignals(true);
    cb->setCurrentIndex(idx);
    cb->blockSignals(false);
  }
}

void MRConfig::selectChannel(QComboBox *cb, int chan)
{
  if(chan < 0 || chan >= MusECore::MUSE_MIDI_CHANNELS)
  {
    fprintf(stderr, "MRConfig::selectChannel: Invalid channel:%d\n", chan);
    return;
  }
  const int idx = cb->findData(chan);
  if(idx == -1)
  {
    fprintf(stderr, "MRConfig::selectChannel: Channel not found!:%d\n", chan);
  }
  else
  {
    cb->blockSignals(true);
    cb->setCurrentIndex(idx);
    cb->blockSignals(false);
  }
}

void MRConfig::switchSettings()
{
  // Clear all learn settings.
  clearLearnSettings();

  //const bool new_val = !MusEGlobal::midiRemoteUseSongSettings;
  const bool new_val = songSettingsButton->isChecked();
  MusECore::PendingOperationList operations;
  operations.add(MusECore::PendingOperationItem(
    &MusEGlobal::midiRemoteUseSongSettings, new_val,
    MusECore::PendingOperationItem::SwitchMidiRemoteSettings));

  // Let it do a songChanged update.
  // Let the songChanged handler update all the values and buttons...
  MusEGlobal::audio->msgExecutePendingOperations(operations, true);
}

void MRConfig::resetPressed()
{
  const QMessageBox::StandardButton b = QMessageBox::warning(this, tr("Reset midi remote:"),
      tr("Resets either global or song midi remote settings to the defaults.\nProceed?"),
      QMessageBox::Ok | QMessageBox::Cancel, QMessageBox::Cancel);
  if(b != QMessageBox::Ok)
    return;

  // Clear the learn settings.
  clearLearnSettings();

  // Make a new struct with default settings, to replace the original.
  MusECore::MidiRemote* new_remote = new MusECore::MidiRemote();

  MusECore::PendingOperationList operations;
  // Takes ownership of the replacement which it deletes at the end of the operation.
  operations.add(MusECore::PendingOperationItem(
    _curMidiRemote, new_remote,
    MusECore::PendingOperationItem::ModifyMidiRemote));
  MusEGlobal::audio->msgExecutePendingOperations(operations, true);
}

void MRConfig::copyPressed()
{
  const QMessageBox::StandardButton b = QMessageBox::warning(this, tr("Copy midi remote:"),
      tr("Copies either global or song midi remote settings to these song or global settings .\nProceed?"),
      QMessageBox::Ok | QMessageBox::Cancel, QMessageBox::Cancel);
  if(b != QMessageBox::Ok)
    return;

  // Clear the learn settings.
  clearLearnSettings();

  // Make a new copy of the OTHER struct, to replace the original.
  MusECore::MidiRemote* new_remote = new MusECore::MidiRemote(
    MusEGlobal::midiRemoteUseSongSettings ? MusEGlobal::midiRemote : *MusEGlobal::song->midiRemote());

  MusECore::PendingOperationList operations;
  // Takes ownership of the replacement which it deletes at the end of the operation.
  operations.add(MusECore::PendingOperationItem(
    _curMidiRemote, new_remote,
    MusECore::PendingOperationItem::ModifyMidiRemote));
  MusEGlobal::audio->msgExecutePendingOperations(operations, true);
}

void MRConfig::songChanged(MusECore::SongChangedStruct_t type)
{
  if(type & SC_MIDI_REMOTE)
    updateValues();
}

void MRConfig::apply()
{
  // Clear the learn settings for convenience.
  clearLearnSettings();

  // Make a new copy of the struct, to replace the original.
  MusECore::MidiRemote* new_remote = new MusECore::MidiRemote(*_curMidiRemote);

  new_remote->_stop._noteport = stopNotePort->currentIndex() == -1 ? -1 : stopNotePort->currentData().toInt();
  new_remote->_play._noteport = playNotePort->currentIndex() == -1 ? -1 : playNotePort->currentData().toInt();
  new_remote->_rec._noteport = recNotePort->currentIndex() == -1 ? -1 : recNotePort->currentData().toInt();
  new_remote->_gotoLeftMark._noteport = goLMarkNotePort->currentIndex() == -1 ? -1 : goLMarkNotePort->currentData().toInt();
  new_remote->_forward._noteport = ffNotePort->currentIndex() == -1 ? -1 : ffNotePort->currentData().toInt();
  new_remote->_backward._noteport = rewNotePort->currentIndex() == -1 ? -1 : rewNotePort->currentData().toInt();
  new_remote->_stepRecPort = stepRecPort->currentIndex() == -1 ? -1 : stepRecPort->currentData().toInt();
  new_remote->_stepRecRest._noteport = stepRecRestNotePort->currentIndex() == -1 ? -1 : stepRecRestNotePort->currentData().toInt();

  new_remote->_stop._ccport = stopCCPort->currentIndex() == -1 ? -1 : stopCCPort->currentData().toInt();
  new_remote->_play._ccport = playCCPort->currentIndex() == -1 ? -1 : playCCPort->currentData().toInt();
  new_remote->_rec._ccport = recCCPort->currentIndex() == -1 ? -1 : recCCPort->currentData().toInt();
  new_remote->_gotoLeftMark._ccport = goLMarkCCPort->currentIndex() == -1 ? -1 : goLMarkCCPort->currentData().toInt();
  new_remote->_forward._ccport = ffCCPort->currentIndex() == -1 ? -1 : ffCCPort->currentData().toInt();
  new_remote->_backward._ccport = rewCCPort->currentIndex() == -1 ? -1 : rewCCPort->currentData().toInt();
  new_remote->_stepRecRest._ccport = stepRecRestCCPort->currentIndex() == -1 ? -1 : stepRecRestCCPort->currentData().toInt();

  new_remote->_stop._notechannel = stopNoteChan->currentIndex() == -1 ? -1 : stopNoteChan->currentData().toInt();
  new_remote->_play._notechannel = playNoteChan->currentIndex() == -1 ? -1 : playNoteChan->currentData().toInt();
  new_remote->_rec._notechannel = recNoteChan->currentIndex() == -1 ? -1 : recNoteChan->currentData().toInt();
  new_remote->_gotoLeftMark._notechannel = goLMarkNoteChan->currentIndex() == -1 ? -1 : goLMarkNoteChan->currentData().toInt();
  new_remote->_forward._notechannel = ffNoteChan->currentIndex() == -1 ? -1 : ffNoteChan->currentData().toInt();
  new_remote->_backward._notechannel = rewNoteChan->currentIndex() == -1 ? -1 : rewNoteChan->currentData().toInt();
  new_remote->_stepRecChan = stepRecChan->currentIndex() == -1 ? -1 : stepRecChan->currentData().toInt();
  new_remote->_stepRecRest._notechannel = stepRecRestNoteChan->currentIndex() == -1 ? -1 : stepRecRestNoteChan->currentData().toInt();

  new_remote->_stop._ccchannel = stopCCChan->currentIndex() == -1 ? -1 : stopCCChan->currentData().toInt();
  new_remote->_play._ccchannel = playCCChan->currentIndex() == -1 ? -1 : playCCChan->currentData().toInt();
  new_remote->_rec._ccchannel = recCCChan->currentIndex() == -1 ? -1 : recCCChan->currentData().toInt();
  new_remote->_gotoLeftMark._ccchannel = goLMarkCCChan->currentIndex() == -1 ? -1 : goLMarkCCChan->currentData().toInt();
  new_remote->_forward._ccchannel = ffCCChan->currentIndex() == -1 ? -1 : ffCCChan->currentData().toInt();
  new_remote->_backward._ccchannel = rewCCChan->currentIndex() == -1 ? -1 : rewCCChan->currentData().toInt();
  new_remote->_stepRecRest._ccchannel = stepRecRestCCChan->currentIndex() == -1 ? -1 : stepRecRestCCChan->currentData().toInt();

  new_remote->_stop._ccnum = stopCCNum->value();
  new_remote->_play._ccnum = playCCNum->value();
  new_remote->_rec._ccnum = recCCNum->value();
  new_remote->_gotoLeftMark._ccnum = goLMarkCCNum->value();
  new_remote->_forward._ccnum = ffCCNum->value();
  new_remote->_backward._ccnum = rewCCNum->value();
  new_remote->_stepRecRest._ccnum = stepRecRestCCNum->value();

  // Unused. Fix at trigger type.
  new_remote->_stop._noteValType = MusECore::MidiRemoteStruct::MidiRemoteValTrigger;
  new_remote->_play._noteValType =
   MusECore::MidiRemoteStruct::MidiRemoteValType(playNoteValType->currentIndex() == -1 ? 0 : playNoteValType->currentData().toInt());
  new_remote->_rec._noteValType =
   MusECore::MidiRemoteStruct::MidiRemoteValType(recNoteValType->currentIndex() == -1 ? 0 : recNoteValType->currentData().toInt());
  // Unused. Fix at trigger type.
  new_remote->_gotoLeftMark._noteValType = MusECore::MidiRemoteStruct::MidiRemoteValTrigger;
  new_remote->_forward._noteValType =
   MusECore::MidiRemoteStruct::MidiRemoteValType(ffNoteValType->currentIndex() == -1 ? 0 : ffNoteValType->currentData().toInt());
  new_remote->_backward._noteValType =
   MusECore::MidiRemoteStruct::MidiRemoteValType(rewNoteValType->currentIndex() == -1 ? 0 : rewNoteValType->currentData().toInt());

  // Unused. Fix at trigger type.
  new_remote->_stop._ccValType = MusECore::MidiRemoteStruct::MidiRemoteValTrigger;
  new_remote->_play._ccValType =
   MusECore::MidiRemoteStruct::MidiRemoteValType(playCCValType->currentIndex() == -1 ? 0 : playCCValType->currentData().toInt());
  new_remote->_rec._ccValType =
   MusECore::MidiRemoteStruct::MidiRemoteValType(recCCValType->currentIndex() == -1 ? 0 : recCCValType->currentData().toInt());
  // Unused. Fix at trigger type.
  new_remote->_gotoLeftMark._ccValType = MusECore::MidiRemoteStruct::MidiRemoteValTrigger;
  new_remote->_forward._ccValType =
   MusECore::MidiRemoteStruct::MidiRemoteValType(ffCCValType->currentIndex() == -1 ? 0 : ffCCValType->currentData().toInt());
  new_remote->_backward._ccValType =
   MusECore::MidiRemoteStruct::MidiRemoteValType(rewCCValType->currentIndex() == -1 ? 0 : rewCCValType->currentData().toInt());

  new_remote->_stop._noteenable = stopUseNote->isChecked();
  new_remote->_play._noteenable = playUseNote->isChecked();
  new_remote->_rec._noteenable = recUseNote->isChecked();
  new_remote->_gotoLeftMark._noteenable = goLMarkUseNote->isChecked();
  new_remote->_forward._noteenable = ffUseNote->isChecked();
  new_remote->_backward._noteenable = rewUseNote->isChecked();
  new_remote->_stepRecRest._noteenable = stepRecRestUseNote->isChecked();

  new_remote->_stop._ccenable = stopUseCC->isChecked();
  new_remote->_play._ccenable = playUseCC->isChecked();
  new_remote->_rec._ccenable = recUseCC->isChecked();
  new_remote->_gotoLeftMark._ccenable = goLMarkUseCC->isChecked();
  new_remote->_forward._ccenable = ffUseCC->isChecked();
  new_remote->_backward._ccenable = rewUseCC->isChecked();
  new_remote->_stepRecRest._ccenable = stepRecRestUseCC->isChecked();

  new_remote->_stop._note = stopNote->value();
  new_remote->_play._note = playNote->value();
  new_remote->_rec._note = recNote->value();
  new_remote->_gotoLeftMark._note = goLMarkNote->value();
  new_remote->_forward._note = ffNote->value();
  new_remote->_backward._note = rewNote->value();
  new_remote->_stepRecRest._note = stepRecRestNote->value();

  MusECore::PendingOperationList operations;
  // Takes ownership of the replacement which it deletes at the end of the operation.
  operations.add(MusECore::PendingOperationItem(
    _curMidiRemote, new_remote,
    MusECore::PendingOperationItem::ModifyMidiRemote));
  MusEGlobal::audio->msgExecutePendingOperations(operations, true);
}

void MRConfig::accept()
{
  apply();
  // Closing will clear the learn settings.
  close();
}

void MRConfig::reject()
{
  // Closing will clear the learn settings.
  close();
}

void MRConfig::updateValues()
{
    if(MusEGlobal::midiRemoteUseSongSettings)
    {
      _curMidiRemote = MusEGlobal::song->midiRemote();
      // Since these buttons are exclusive, we must block both.
      songSettingsButton->blockSignals(true);
      globalSettingsButton->blockSignals(true);
      songSettingsButton->setChecked(true);
      songSettingsButton->blockSignals(false);
      globalSettingsButton->blockSignals(false);
    }
    else
    {
      _curMidiRemote = &MusEGlobal::midiRemote;
      // Since these buttons are exclusive, we must block both.
      songSettingsButton->blockSignals(true);
      globalSettingsButton->blockSignals(true);
      globalSettingsButton->setChecked(true);
      songSettingsButton->blockSignals(false);
      globalSettingsButton->blockSignals(false);
    }

    setupPortList(stopNotePort, _curMidiRemote->_stop._noteport);
    setupPortList(playNotePort, _curMidiRemote->_play._noteport);
    setupPortList(recNotePort, _curMidiRemote->_rec._noteport);
    setupPortList(goLMarkNotePort, _curMidiRemote->_gotoLeftMark._noteport);
    setupPortList(ffNotePort, _curMidiRemote->_forward._noteport);
    setupPortList(rewNotePort, _curMidiRemote->_backward._noteport);
    setupPortList(stepRecPort, _curMidiRemote->_stepRecPort);
    setupPortList(stepRecRestNotePort, _curMidiRemote->_stepRecRest._noteport);

    setupPortList(stopCCPort, _curMidiRemote->_stop._ccport);
    setupPortList(playCCPort, _curMidiRemote->_play._ccport);
    setupPortList(recCCPort, _curMidiRemote->_rec._ccport);
    setupPortList(goLMarkCCPort, _curMidiRemote->_gotoLeftMark._ccport);
    setupPortList(ffCCPort, _curMidiRemote->_forward._ccport);
    setupPortList(rewCCPort, _curMidiRemote->_backward._ccport);
    setupPortList(stepRecRestCCPort, _curMidiRemote->_stepRecRest._ccport);

    setupChannelList(stopNoteChan, _curMidiRemote->_stop._notechannel);
    setupChannelList(playNoteChan, _curMidiRemote->_play._notechannel);
    setupChannelList(recNoteChan, _curMidiRemote->_rec._notechannel);
    setupChannelList(goLMarkNoteChan, _curMidiRemote->_gotoLeftMark._notechannel);
    setupChannelList(ffNoteChan, _curMidiRemote->_forward._notechannel);
    setupChannelList(rewNoteChan, _curMidiRemote->_backward._notechannel);
    setupChannelList(stepRecChan, _curMidiRemote->_stepRecChan);
    setupChannelList(stepRecRestNoteChan, _curMidiRemote->_stepRecRest._notechannel);

    setupChannelList(stopCCChan, _curMidiRemote->_stop._ccchannel);
    setupChannelList(playCCChan, _curMidiRemote->_play._ccchannel);
    setupChannelList(recCCChan, _curMidiRemote->_rec._ccchannel);
    setupChannelList(goLMarkCCChan, _curMidiRemote->_gotoLeftMark._ccchannel);
    setupChannelList(ffCCChan, _curMidiRemote->_forward._ccchannel);
    setupChannelList(rewCCChan, _curMidiRemote->_backward._ccchannel);
    setupChannelList(stepRecRestCCChan, _curMidiRemote->_stepRecRest._ccchannel);

    setupCCNumList(stopCCNum, _curMidiRemote->_stop._ccnum);
    setupCCNumList(playCCNum, _curMidiRemote->_play._ccnum);
    setupCCNumList(recCCNum, _curMidiRemote->_rec._ccnum);
    setupCCNumList(goLMarkCCNum, _curMidiRemote->_gotoLeftMark._ccnum);
    setupCCNumList(ffCCNum, _curMidiRemote->_forward._ccnum);
    setupCCNumList(rewCCNum, _curMidiRemote->_backward._ccnum);
    setupCCNumList(stepRecRestCCNum, _curMidiRemote->_stepRecRest._ccnum);

    setupValTypeList(playNoteValType, _curMidiRemote->_play._noteValType);
    setupValTypeList(recNoteValType, _curMidiRemote->_rec._noteValType);
    setupValTypeList(ffNoteValType, _curMidiRemote->_forward._noteValType);
    setupValTypeList(rewNoteValType, _curMidiRemote->_backward._noteValType);

    setupValTypeList(playCCValType, _curMidiRemote->_play._ccValType);
    setupValTypeList(recCCValType, _curMidiRemote->_rec._ccValType);
    setupValTypeList(ffCCValType, _curMidiRemote->_forward._ccValType);
    setupValTypeList(rewCCValType, _curMidiRemote->_backward._ccValType);

    stopUseNote->blockSignals(true);
    playUseNote->blockSignals(true);
    recUseNote->blockSignals(true);
    goLMarkUseNote->blockSignals(true);
    ffUseNote->blockSignals(true);
    rewUseNote->blockSignals(true);
    stepRecRestUseNote->blockSignals(true);

    stopUseCC->blockSignals(true);
    playUseCC->blockSignals(true);
    recUseCC->blockSignals(true);
    goLMarkUseCC->blockSignals(true);
    ffUseCC->blockSignals(true);
    rewUseCC->blockSignals(true);
    stepRecRestUseCC->blockSignals(true);

    stopNote->blockSignals(true);
    playNote->blockSignals(true);
    recNote->blockSignals(true);
    goLMarkNote->blockSignals(true);
    ffNote->blockSignals(true);
    rewNote->blockSignals(true);
    stepRecRestNote->blockSignals(true);

    stopUseNote->setChecked(_curMidiRemote->_stop._noteenable);
    playUseNote->setChecked(_curMidiRemote->_play._noteenable);
    recUseNote->setChecked(_curMidiRemote->_rec._noteenable);
    goLMarkUseNote->setChecked(_curMidiRemote->_gotoLeftMark._noteenable);
    ffUseNote->setChecked(_curMidiRemote->_forward._noteenable);
    rewUseNote->setChecked(_curMidiRemote->_backward._noteenable);
    stepRecRestUseNote->setChecked(_curMidiRemote->_stepRecRest._noteenable);

    stopUseCC->setChecked(_curMidiRemote->_stop._ccenable);
    playUseCC->setChecked(_curMidiRemote->_play._ccenable);
    recUseCC->setChecked(_curMidiRemote->_rec._ccenable);
    goLMarkUseCC->setChecked(_curMidiRemote->_gotoLeftMark._ccenable);
    ffUseCC->setChecked(_curMidiRemote->_forward._ccenable);
    rewUseCC->setChecked(_curMidiRemote->_backward._ccenable);
    stepRecRestUseCC->setChecked(_curMidiRemote->_stepRecRest._ccenable);

    stopNote->setValue(_curMidiRemote->_stop._note);
    playNote->setValue(_curMidiRemote->_play._note);
    recNote->setValue(_curMidiRemote->_rec._note);
    goLMarkNote->setValue(_curMidiRemote->_gotoLeftMark._note);
    ffNote->setValue(_curMidiRemote->_forward._note);
    rewNote->setValue(_curMidiRemote->_backward._note);
    stepRecRestNote->setValue(_curMidiRemote->_stepRecRest._note);

    stopUseNote->blockSignals(false);
    playUseNote->blockSignals(false);
    recUseNote->blockSignals(false);
    goLMarkUseNote->blockSignals(false);
    ffUseNote->blockSignals(false);
    rewUseNote->blockSignals(false);
    stepRecRestUseNote->blockSignals(false);

    stopUseCC->blockSignals(false);
    playUseCC->blockSignals(false);
    recUseCC->blockSignals(false);
    goLMarkUseCC->blockSignals(false);
    ffUseCC->blockSignals(false);
    rewUseCC->blockSignals(false);
    stepRecRestUseCC->blockSignals(false);

    stopNote->blockSignals(false);
    playNote->blockSignals(false);
    recNote->blockSignals(false);
    goLMarkNote->blockSignals(false);
    ffNote->blockSignals(false);
    rewNote->blockSignals(false);
    stepRecRestNote->blockSignals(false);

    // Reset these.
    buttonApply->setEnabled(false);
    buttonOk->setEnabled(false);
}

} // namespace MusEGui
