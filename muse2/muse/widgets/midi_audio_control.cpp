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
#include "mididev.h"
#include "midiport.h"
#include "midictrl.h"
#include "audio.h"
#include "app.h"

#include <QTimer>

namespace MusEGui {

// -----------------------------------
//   MidiAudioControl
//   Set port to -1 to automatically set it to the port of 
//    the first combo box item (the first readable port).
// -----------------------------------

MidiAudioControl::MidiAudioControl(int port, int chan, int ctrl, QWidget* parent)
  : QDialog(parent)
{
  setupUi(this);

  _port = port;
  _chan = chan;
  _ctrl = ctrl;
  _is_learning = false;
  
  update();
  
  connect(learnPushButton, SIGNAL(clicked(bool)), SLOT(learnChanged(bool)));
  connect(portComboBox, SIGNAL(currentIndexChanged(int)), SLOT(portChanged(int)));
  connect(channelSpinBox, SIGNAL(valueChanged(int)), SLOT(chanChanged()));
  connect(controlTypeComboBox, SIGNAL(currentIndexChanged(int)), SLOT(ctrlTypeChanged(int)));
  connect(ctrlHiSpinBox, SIGNAL(valueChanged(int)), SLOT(ctrlHChanged()));
  connect(ctrlLoSpinBox, SIGNAL(valueChanged(int)), SLOT(ctrlLChanged()));
  connect(MusEGlobal::muse, SIGNAL(configChanged()), SLOT(configChanged()));
  connect(MusEGlobal::heartBeatTimer, SIGNAL(timeout()), SLOT(heartbeat()));
}

void MidiAudioControl::heartbeat()
{
  if(_is_learning)
  {
    if(MusEGlobal::midiLearnPort != -1) 
    {
      int port_item = portComboBox->findData(MusEGlobal::midiLearnPort);
      if(port_item != -1 && port_item != portComboBox->currentIndex())
      {
        _port = MusEGlobal::midiLearnPort;
        portComboBox->blockSignals(true);
        portComboBox->setCurrentIndex(port_item);
        portComboBox->blockSignals(false);
      }
    }
    
    if(MusEGlobal::midiLearnChan != -1 && (MusEGlobal::midiLearnChan + 1) != channelSpinBox->value())
    {
      _chan = MusEGlobal::midiLearnChan;
      channelSpinBox->blockSignals(true);
      channelSpinBox->setValue(_chan + 1);
      channelSpinBox->blockSignals(false);
    }

    if(MusEGlobal::midiLearnCtrl != -1)
    {
      int type = MusECore::midiControllerType(MusEGlobal::midiLearnCtrl);
      if(type < controlTypeComboBox->count() && type != controlTypeComboBox->currentIndex())
      {
        controlTypeComboBox->blockSignals(true);
        controlTypeComboBox->setCurrentIndex(type);
        controlTypeComboBox->blockSignals(false);
      }

      int hv = (MusEGlobal::midiLearnCtrl >> 8) & 0xff;
      int lv = MusEGlobal::midiLearnCtrl & 0xff;
      if(type == MusECore::MidiController::Program || type == MusECore::MidiController::Pitch)
      {
        ctrlHiSpinBox->setEnabled(false);
        ctrlLoSpinBox->setEnabled(false);
        ctrlHiSpinBox->blockSignals(true);
        ctrlLoSpinBox->blockSignals(true);
        ctrlHiSpinBox->setValue(0);
        ctrlLoSpinBox->setValue(0);
        ctrlHiSpinBox->blockSignals(false);
        ctrlLoSpinBox->blockSignals(false);
      }
      else if(type == MusECore::MidiController::Controller7)
      {
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
      }
      else
      {
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
      }  
      
      _ctrl = MusECore::midiCtrlTerms2Number(type, (ctrlHiSpinBox->value() << 8) + ctrlLoSpinBox->value());
    }
  }
}

void MidiAudioControl::learnChanged(bool v)
{
  _is_learning = v;
  if(_is_learning)
    MusEGlobal::audio->msgStartMidiLearn();  // Resets the learn values to -1.
}

void MidiAudioControl::resetLearn()
{
  _is_learning = false;
  learnPushButton->blockSignals(true);
  learnPushButton->setChecked(false);
  learnPushButton->blockSignals(false);
  MusEGlobal::audio->msgStartMidiLearn();  // Resets the learn values to -1.
}

void MidiAudioControl::portChanged(int idx)
{
  if(idx == -1)
    return;
  int port_num = portComboBox->itemData(idx).toInt();
  if(port_num < 0 || port_num >= MIDI_PORTS)
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
  int idx = controlTypeComboBox->currentIndex();
  if(idx == -1)
    return;
  
  if(idx == MusECore::MidiController::Program || idx == MusECore::MidiController::Pitch)
  {
    ctrlHiSpinBox->setEnabled(false);
    ctrlLoSpinBox->setEnabled(false);
    ctrlHiSpinBox->blockSignals(true);
    ctrlLoSpinBox->blockSignals(true);
    ctrlHiSpinBox->setValue(0);
    ctrlLoSpinBox->setValue(0);
    ctrlHiSpinBox->blockSignals(false);
    ctrlLoSpinBox->blockSignals(false);
  }
  else if(idx == MusECore::MidiController::Controller7)
  {
    ctrlHiSpinBox->setEnabled(false);
    ctrlLoSpinBox->setEnabled(true);

    ctrlHiSpinBox->blockSignals(true);
    ctrlHiSpinBox->setValue(0);
    ctrlHiSpinBox->blockSignals(false);
  }
  else
  {
    ctrlHiSpinBox->setEnabled(true);
    ctrlLoSpinBox->setEnabled(true);
  }  
}

void MidiAudioControl::ctrlTypeChanged(int idx)
{
  if(idx == -1)
    return;
  
  updateCtrlBoxes();
  
  _ctrl = (ctrlHiSpinBox->value() << 8) + ctrlLoSpinBox->value();
  _ctrl = MusECore::midiCtrlTerms2Number(idx, _ctrl);

  resetLearn();
}

void MidiAudioControl::ctrlHChanged()
{
  if(controlTypeComboBox->currentIndex() == -1)
    return;
  _ctrl = (ctrlHiSpinBox->value() << 8) + ctrlLoSpinBox->value();
  _ctrl = MusECore::midiCtrlTerms2Number(controlTypeComboBox->currentIndex(), _ctrl);

  resetLearn();
}

void MidiAudioControl::ctrlLChanged()
{
  if(controlTypeComboBox->currentIndex() == -1)
    return;
  _ctrl = (ctrlHiSpinBox->value() << 8) + ctrlLoSpinBox->value();
  _ctrl = MusECore::midiCtrlTerms2Number(controlTypeComboBox->currentIndex(), _ctrl);

  resetLearn();
}

void MidiAudioControl::configChanged()
{
  update();
}

void MidiAudioControl::update()
{
  portComboBox->blockSignals(true);
  portComboBox->clear();

  int item_idx = 0;
  for (int i = 0; i < MIDI_PORTS; ++i) {
        MusECore::MidiDevice* md = MusEGlobal::midiPorts[i].device(); 
        if(!md)  // In the case of this combo box, don't bother listing empty ports.             
          continue;
        //if(!(md->rwFlags() & 1 || md->isSynti()) && (i != outPort))  
        if(!(md->rwFlags() & 2) && (i != _port))   // Only readable ports, or current one.    
          continue;
        QString name;
        name.sprintf("%d:%s", i+1, MusEGlobal::midiPorts[i].portname().toLatin1().constData());
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
  if(type < controlTypeComboBox->count())
  {
    controlTypeComboBox->blockSignals(true);
    controlTypeComboBox->setCurrentIndex(type);
    controlTypeComboBox->blockSignals(false);
  }

  int hv = (_ctrl >> 8) & 0xff;
  int lv = _ctrl & 0xff;
  if(type == MusECore::MidiController::Program || type == MusECore::MidiController::Pitch)
  {
    ctrlHiSpinBox->setEnabled(false);
    ctrlLoSpinBox->setEnabled(false);
    ctrlHiSpinBox->blockSignals(true);
    ctrlLoSpinBox->blockSignals(true);
    ctrlHiSpinBox->setValue(0);
    ctrlLoSpinBox->setValue(0);
    ctrlHiSpinBox->blockSignals(false);
    ctrlLoSpinBox->blockSignals(false);
  }
  else if(type == MusECore::MidiController::Controller7)
  {
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
  }
  else
  {
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
  }
}

}
