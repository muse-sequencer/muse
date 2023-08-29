//=========================================================
//  MusE
//  Linux Music Editor
//
//  midi_audio_control.cpp
//  Copyright (C) 2012 by Tim E. Real (terminator356 at users.sourceforge.net)
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
#include "midi_audio_control.h"

#include "globals.h"
#include "globaldefs.h"
#include "midi_consts.h"
#include "mididev.h"
#include "midiport.h"
#include "midictrl.h"
#include "audio.h"
#include "app.h"
#include "song.h"

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

// -----------------------------------
//   MidiAudioControl
//   Set port to -1 to automatically set it to the port of 
//    the first combo box item (the first readable port).
// -----------------------------------

MidiAudioControl::MidiAudioControl(bool enableAssignType, bool assignToSong, int port, int chan, int ctrl, QWidget* parent)
  : QDialog(parent)
{
  setupUi(this);

  controlTypeComboBox->addItem(tr("Control7"), MusECore::MidiController::Controller7);
  controlTypeComboBox->addItem(tr("Control14"), MusECore::MidiController::Controller14);
  controlTypeComboBox->addItem(tr("RPN"), MusECore::MidiController::RPN);
  controlTypeComboBox->addItem(tr("NPRN"), MusECore::MidiController::NRPN);
  controlTypeComboBox->addItem(tr("RPN14"), MusECore::MidiController::RPN14);
  controlTypeComboBox->addItem(tr("NRPN14"), MusECore::MidiController::NRPN14);
  controlTypeComboBox->addItem(tr("Pitch"), MusECore::MidiController::Pitch);
  controlTypeComboBox->addItem(tr("Program"), MusECore::MidiController::Program);
  //controlTypeComboBox->addItem(tr("PolyAftertouch"), MusECore::MidiController::PolyAftertouch); // Not supported yet. Need a way to select pitch.
  controlTypeComboBox->addItem(tr("Aftertouch"), MusECore::MidiController::Aftertouch);
  controlTypeComboBox->setCurrentIndex(0);
  
  _port = port;
  _chan = chan;
  _ctrl = ctrl;
  MusEGlobal::midiToAudioAssignIsLearning = false;
  _enableAssignType = enableAssignType;
  _assignToSong = assignToSong;
  
  assignTypeGroupBox->setEnabled(_enableAssignType);
  assignTypeGroupBox->setVisible(_enableAssignType);

  if(_enableAssignType)
  {
    if(_assignToSong)
      typeSongButton->setChecked(true);
    else
      typeTrackButton->setChecked(true);
  }

  updateDialog();
  
  // Special for these: Need qt helper overload for these lambdas.
  connect(portComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), [=](int idx) { portChanged(idx); } );
  connect(controlTypeComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), [=](int idx) { ctrlTypeChanged(idx); } );
  connect(channelSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), [=]() { chanChanged(); } );
  connect(ctrlHiSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), [=]() { ctrlHChanged(); } );
  connect(ctrlLoSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), [=]() { ctrlLChanged(); } );

  connect(learnPushButton, &QPushButton::clicked, [this](bool v) { learnChanged(v); } );

  connect(typeTrackButton, &QRadioButton::clicked, [=]() { assignTrackTriggered(); } );
  connect(typeSongButton, &QRadioButton::clicked, [=]() { assignSongTriggered(); } );

  _configChangedConn = connect(MusEGlobal::muse, &MusEGui::MusE::configChanged, this, &MidiAudioControl::configChanged);
  _learnReceivedConn = connect(MusEGlobal::song, &MusECore::Song::midiLearnReceived, this, &MidiAudioControl::midiLearnReceived);
}

MidiAudioControl::~MidiAudioControl()
{
  // Clear the learn settings.
  MusEGlobal::midiToAudioAssignIsLearning = false;
  disconnect(_configChangedConn);
  disconnect(_learnReceivedConn);
}

int MidiAudioControl::port() const { return _port; }
int MidiAudioControl::chan() const { return _chan; }
int MidiAudioControl::ctrl() const { return _ctrl; }
bool MidiAudioControl::enableAssignType() const { return _enableAssignType; }
bool MidiAudioControl::assignToSong() const { return _assignToSong; }

void MidiAudioControl::learnChanged(bool v)
{
  MusEGlobal::midiToAudioAssignIsLearning = v;
}

void MidiAudioControl::resetLearn()
{
  MusEGlobal::midiToAudioAssignIsLearning = false;
  learnPushButton->blockSignals(true);
  learnPushButton->setChecked(false);
  learnPushButton->blockSignals(false);
}

void MidiAudioControl::portChanged(int idx)
{
  if(idx == -1)
    return;
  int port_num = portComboBox->itemData(idx).toInt();
  if(port_num < 0 || port_num >= MusECore::MIDI_PORTS)
    return;

  _port = port_num;
  resetLearn();
}

void MidiAudioControl::chanChanged()
{
  _chan = channelSpinBox->value() - 1;
  resetLearn();
}

void MidiAudioControl::updateCtrlBoxes()
{
  if(controlTypeComboBox->currentIndex() == -1)
    return;
  MusECore::MidiController::ControllerType t =
    (MusECore::MidiController::ControllerType)controlTypeComboBox->itemData(controlTypeComboBox->currentIndex()).toInt();

  switch(t)
  {
    case MusECore::MidiController::Program:
    case MusECore::MidiController::Pitch:
    case MusECore::MidiController::PolyAftertouch: // Unsupported yet. Need a way to select pitch.
    case MusECore::MidiController::Aftertouch:
      ctrlHiSpinBox->setEnabled(false);
      ctrlLoSpinBox->setEnabled(false);
      ctrlHiSpinBox->blockSignals(true);
      ctrlLoSpinBox->blockSignals(true);
      ctrlHiSpinBox->setValue(0);
      ctrlLoSpinBox->setValue(0);
      ctrlHiSpinBox->blockSignals(false);
      ctrlLoSpinBox->blockSignals(false);
      break;
    case MusECore::MidiController::Controller7:
      ctrlHiSpinBox->setEnabled(false);
      ctrlLoSpinBox->setEnabled(true);
      ctrlHiSpinBox->blockSignals(true);
      ctrlHiSpinBox->setValue(0);
      ctrlHiSpinBox->blockSignals(false);
      break;
    case MusECore::MidiController::Controller14:
    case MusECore::MidiController::RPN:
    case MusECore::MidiController::RPN14:
    case MusECore::MidiController::NRPN:
    case MusECore::MidiController::NRPN14:
      ctrlHiSpinBox->setEnabled(true);
      ctrlLoSpinBox->setEnabled(true);
      break;
    default:
      printf("FIXME: MidiAudioControl::updateCtrlBoxes: Unknown control type: %d\n", t);
      break;
  }
}

void MidiAudioControl::ctrlTypeChanged(int idx)
{
  if(idx == -1)
    return;

  updateCtrlBoxes();
  
  _ctrl = (ctrlHiSpinBox->value() << 8) + (ctrlLoSpinBox->value() & 0xff);
  _ctrl = MusECore::midiCtrlTerms2Number((MusECore::MidiController::ControllerType)controlTypeComboBox->itemData(idx).toInt(), _ctrl);

  resetLearn();
}

void MidiAudioControl::ctrlHChanged()
{
  if(controlTypeComboBox->currentIndex() == -1)
    return;
  _ctrl = (ctrlHiSpinBox->value() << 8) + (ctrlLoSpinBox->value() & 0xff);
  _ctrl = MusECore::midiCtrlTerms2Number(
    (MusECore::MidiController::ControllerType)controlTypeComboBox->itemData(controlTypeComboBox->currentIndex()).toInt(), _ctrl);

  resetLearn();
}

void MidiAudioControl::ctrlLChanged()
{
  if(controlTypeComboBox->currentIndex() == -1)
    return;
  _ctrl = (ctrlHiSpinBox->value() << 8) + (ctrlLoSpinBox->value() & 0xff);
  _ctrl = MusECore::midiCtrlTerms2Number((MusECore::MidiController::ControllerType)controlTypeComboBox->itemData(controlTypeComboBox->currentIndex()).toInt(), _ctrl);

  resetLearn();
}

void MidiAudioControl::assignTrackTriggered()
{
  _assignToSong = false;
}

void MidiAudioControl::assignSongTriggered()
{
  _assignToSong = true;
}

void MidiAudioControl::configChanged()
{
  updateDialog();
}

void MidiAudioControl::updateDialog()
{
  portComboBox->blockSignals(true);
  portComboBox->clear();

  int item_idx = 0;
  for (int i = 0; i < MusECore::MIDI_PORTS; ++i) {
        MusECore::MidiDevice* md = MusEGlobal::midiPorts[i].device(); 
        if(!md)  // In the case of this combo box, don't bother listing empty ports.             
          continue;
        //if(!(md->rwFlags() & 1 || md->isSynti()) && (i != outPort))  
        if(!(md->rwFlags() & 2) && (i != _port))   // Only readable ports, or current one.    
          continue;
        QString name = QString("%1:%2").arg(i + 1).arg(MusEGlobal::midiPorts[i].portname());
        portComboBox->insertItem(item_idx, name, i);
        if(_port == -1)
          _port = i;      // Initialize
        if(i == _port)
          portComboBox->setCurrentIndex(item_idx);
        item_idx++;
        }
  portComboBox->blockSignals(false);
  
  channelSpinBox->blockSignals(true);
  channelSpinBox->setValue(_chan + 1);
  channelSpinBox->blockSignals(false);

  int type = MusECore::midiControllerType(_ctrl);
  int idx = controlTypeComboBox->findData(type);
  if(idx != -1 && idx != controlTypeComboBox->currentIndex())
  {
    controlTypeComboBox->blockSignals(true);
    controlTypeComboBox->setCurrentIndex(idx);
    controlTypeComboBox->blockSignals(false);
  }

  int hv = (_ctrl >> 8) & 0xff;
  int lv = _ctrl & 0xff;

  switch(type)
  {
    case MusECore::MidiController::Program:
    case MusECore::MidiController::Pitch:
    case MusECore::MidiController::PolyAftertouch:  // Unsupported yet. Need a way to select pitch.
    case MusECore::MidiController::Aftertouch:
      ctrlHiSpinBox->setEnabled(false);
      ctrlLoSpinBox->setEnabled(false);
      ctrlHiSpinBox->blockSignals(true);
      ctrlLoSpinBox->blockSignals(true);
      ctrlHiSpinBox->setValue(0);
      ctrlLoSpinBox->setValue(0);
      ctrlHiSpinBox->blockSignals(false);
      ctrlLoSpinBox->blockSignals(false);
      break;
    case MusECore::MidiController::Controller7:  
      ctrlHiSpinBox->setEnabled(false);
      ctrlLoSpinBox->setEnabled(true);

      ctrlHiSpinBox->blockSignals(true);
      ctrlHiSpinBox->setValue(0);
      ctrlHiSpinBox->blockSignals(false);

      if(lv != ctrlLoSpinBox->value())
      {
        ctrlLoSpinBox->blockSignals(true);
        ctrlLoSpinBox->setValue(lv);
        ctrlLoSpinBox->blockSignals(false);
      }
      break;
    case MusECore::MidiController::Controller14:
    case MusECore::MidiController::RPN:
    case MusECore::MidiController::RPN14:
    case MusECore::MidiController::NRPN:
    case MusECore::MidiController::NRPN14:
      ctrlHiSpinBox->setEnabled(true);
      ctrlLoSpinBox->setEnabled(true);
      if(hv != ctrlHiSpinBox->value())
      {
        ctrlHiSpinBox->blockSignals(true);
        ctrlHiSpinBox->setValue(hv);
        ctrlHiSpinBox->blockSignals(false);
      }
      if(lv != ctrlLoSpinBox->value())
      {
        ctrlLoSpinBox->blockSignals(true);
        ctrlLoSpinBox->setValue(lv);
        ctrlLoSpinBox->blockSignals(false);
      }
      break;
    default:
      printf("FIXME: MidiAudioControl::updateCtrlBoxes: Unknown control type: %d\n", type);
      break;
  }
}

void MidiAudioControl::midiLearnReceived(const MusECore::MidiRecordEvent& ev)
{
  if(learnPushButton->isChecked())
  {
    const int type = ev.type();
    const int port = ev.port();
    const int chan = ev.channel();
    const int dataA = ev.dataA();

    if(type == MusECore::ME_CONTROLLER || type == MusECore::ME_PITCHBEND || type == MusECore::ME_PROGRAM)
    {
      selectPort(portComboBox, port);

      _chan = chan;
      channelSpinBox->blockSignals(true);
      channelSpinBox->setValue(chan + 1);
      channelSpinBox->blockSignals(false);

      int ctl;
      if(type == MusECore::ME_PITCHBEND)
        ctl = MusECore::CTRL_PITCH;
      else if(type == MusECore::ME_PROGRAM)
        ctl = MusECore::CTRL_PROGRAM;
      else
        ctl = dataA;

      selectCtrl(controlTypeComboBox, ctrlHiSpinBox, ctrlLoSpinBox, ctl);
    }
  }
}

void MidiAudioControl::selectPort(QComboBox *cb, int port)
{
  if(port < 0 || port >= MusECore::MIDI_PORTS)
  {
    fprintf(stderr, "MidiAudioControl::selectPort: Invalid port:%d\n", port);
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
    fprintf(stderr, "MidiAudioControl::selectPort: Port not found!:%d\n", port);
  }
  else
  {
    _port = port;
    cb->blockSignals(true);
    cb->setCurrentIndex(idx);
    cb->blockSignals(false);
  }
}

void MidiAudioControl::selectCtrl(QComboBox *typecb, QSpinBox *hisb, QSpinBox *losb, int ctrl)
{
  if(ctrl == -1)
    return;

  const MusECore::MidiController::ControllerType type = MusECore::midiControllerType(ctrl);
  const int idx = typecb->findData(type);
  if(idx != -1 && idx != typecb->currentIndex())
  {
    typecb->blockSignals(true);
    typecb->setCurrentIndex(idx);
    typecb->blockSignals(false);
  }

  const int hv = (ctrl >> 8) & 0xff;
  const int lv = ctrl & 0xff;

  switch(type)
  {
    case MusECore::MidiController::Program:
    case MusECore::MidiController::Pitch:
    case MusECore::MidiController::PolyAftertouch: // Unsupported yet. Need a way to select pitch.
    case MusECore::MidiController::Aftertouch:
      hisb->setEnabled(false);
      losb->setEnabled(false);
      hisb->blockSignals(true);
      losb->blockSignals(true);
      hisb->setValue(0);
      losb->setValue(0);
      hisb->blockSignals(false);
      losb->blockSignals(false);
      break;
    case MusECore::MidiController::Controller7:
      hisb->setEnabled(false);
      losb->setEnabled(true);

      hisb->blockSignals(true);
      hisb->setValue(0);
      hisb->blockSignals(false);

      if(lv != losb->value())
      {
        losb->blockSignals(true);
        losb->setValue(lv);
        losb->blockSignals(false);
      }
      break;
    case MusECore::MidiController::Controller14:
    case MusECore::MidiController::RPN:
    case MusECore::MidiController::RPN14:
    case MusECore::MidiController::NRPN:
    case MusECore::MidiController::NRPN14:
      hisb->setEnabled(true);
      losb->setEnabled(true);
      if(hv != hisb->value())
      {
        hisb->blockSignals(true);
        hisb->setValue(hv);
        hisb->blockSignals(false);
      }
      if(lv != losb->value())
      {
        losb->blockSignals(true);
        losb->setValue(lv);
        losb->blockSignals(false);
      }
      break;
    default:
      fprintf(stderr, "FIXME: MidiAudioControl::selectCtrl: Unknown control type: %d\n", type);
      break;
  }

  _ctrl = MusECore::midiCtrlTerms2Number(type, (hisb->value() << 8) + (losb->value() & 0xff));
}


}  // namespace MusEGui
