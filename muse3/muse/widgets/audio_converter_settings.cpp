//=========================================================
//  MusE
//  Linux Music Editor
//
//  audio_converter_settings.cpp
//  (C) Copyright 2016 Tim E. Real (terminator356 A T sourceforge D O T net)
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

#include <QDialog>
// #include <QWidget>
#include <QListWidget>
#include <QListWidgetItem>
#include <QPushButton>

#include "audio_convert/audio_converter_settings_group.h"
#include "audio_convert/audio_converter_plugin.h"

#include "audio_converter_settings.h"

namespace MusEGui {
  
AudioConverterSettingsDialog::AudioConverterSettingsDialog(
  QWidget* parent,
  MusECore::AudioConverterPluginList* pluginList,
  MusECore::AudioConverterSettingsGroup* settings, 
  bool isLocal)
  : QDialog(parent), _pluginList(pluginList), _settings(settings), _isLocal(isLocal)
{
  setupUi(this);

  OKButton->setEnabled(false);
    
  fillList();
  if(_settings)
  {
    int idx;
    idx = sampleratePreferenceComboBox->findData(_settings->_options._preferredResampler);
    //if(_isLocal || idx >= 0)
    if(idx >= 0)
      sampleratePreferenceComboBox->setCurrentIndex(idx);
    idx = shiftingPreferenceComboBox->findData(_settings->_options._preferredShifter);
    //if(_isLocal || idx >= 0)
    if(idx >= 0)
      shiftingPreferenceComboBox->setCurrentIndex(idx);
  }
  
  preferencesGroup->setEnabled(!isLocal || (_settings && _settings->_options._useSettings));
  
  //if(_isLocal)
    useDefaultPreferences->setChecked(!_settings || !_settings->_options._useSettings);
  useDefaultPreferences->setEnabled(isLocal && _settings);
  useDefaultPreferences->setVisible(isLocal && _settings);
  
  
  connect(converterList, SIGNAL(currentItemChanged(QListWidgetItem*,QListWidgetItem*)),
      SLOT(currentConverterChanged(QListWidgetItem*, QListWidgetItem*)));
   
  connect(offlineSettingsButton, SIGNAL(clicked()), SLOT(offlineSettingsClicked()));
  connect(realtimeSettingsButton, SIGNAL(clicked()), SLOT(realtimeSettingsClicked()));
  connect(guiSettingsButton, SIGNAL(clicked()), SLOT(guiSettingsClicked()));


  connect(sampleratePreferenceComboBox, SIGNAL(currentIndexChanged(int)), SLOT(preferredResamplerChanged(int)));
  connect(shiftingPreferenceComboBox, SIGNAL(currentIndexChanged(int)), SLOT(preferredShifterChanged(int)));
  
  connect(useDefaultPreferences, SIGNAL(clicked()), SLOT(useDefaultsClicked()));
  connect(OKButton, SIGNAL(clicked()), SLOT(okClicked()));
  connect(cancelButton, SIGNAL(clicked()), SLOT(cancelClicked()));
  
  currentConverterChanged(converterList->item(0), 0);
}

void AudioConverterSettingsDialog::fillList()
{
  converterList->blockSignals(true);
  converterList->clear();
  converterList->blockSignals(false);
  
//   if(_isLocal)
//   {
//     sampleratePreferenceComboBox->addItem(tr("Use default settings"), -1);
//     shiftingPreferenceComboBox->addItem(tr("Use default settings"), -1);
//   }
  
  if(_pluginList)
  {
    for(MusECore::ciAudioConverterPlugin ip = _pluginList->begin(); ip != _pluginList->end(); ++ip)
    {
      MusECore::AudioConverterPlugin* plugin = *ip;
      QListWidgetItem* item = new QListWidgetItem(plugin->name(), converterList);
      item->setData(Qt::UserRole, plugin->id());
      
      const int caps = plugin->capabilities();
      if(caps & MusECore::AudioConverter::SampleRate)
        sampleratePreferenceComboBox->addItem(plugin->name(), plugin->id());
      if(caps & MusECore::AudioConverter::Stretch) // TODO: Separate pitch preference?
        shiftingPreferenceComboBox->addItem(plugin->name(), plugin->id());
    }
  }
}

void AudioConverterSettingsDialog::currentConverterChanged(QListWidgetItem* /*cur_item*/, QListWidgetItem* /*prev_item*/)
{
  enableSettingsButtons();
}

void AudioConverterSettingsDialog::enableSettingsButtons()
{
  bool enable = false;
  if(_pluginList)
  {
    if(QListWidgetItem* cur_item = converterList->currentItem())
    {
      int id = cur_item->data(Qt::UserRole).toInt();
      if(id >= 0 && _pluginList->find(NULL, id))
        enable = true;
    }
  }
  offlineSettingsButton->setEnabled(enable);
  realtimeSettingsButton->setEnabled(enable);
  guiSettingsButton->setEnabled(enable);
}

void AudioConverterSettingsDialog::preferredResamplerChanged(int /*idx*/)
{
//   if(idx < 0 || !_settings)
//     return;
//   const int id = sampleratePreferenceComboBox->itemData(idx).toInt();
//   _settings->_options._preferredResampler = id;
  OKButton->setEnabled(true);
}

void AudioConverterSettingsDialog::preferredShifterChanged(int /*idx*/)
{
//   if(idx < 0 || !_settings)
//     return;
//   const int id = shiftingPreferenceComboBox->itemData(idx).toInt();
//   _settings->_options._preferredShifter = id;
  OKButton->setEnabled(true);
}


void AudioConverterSettingsDialog::offlineSettingsClicked()
{
  showSettings(MusECore::AudioConverterSettings::OfflineMode);
}

void AudioConverterSettingsDialog::realtimeSettingsClicked()
{
  showSettings(MusECore::AudioConverterSettings::RealtimeMode);
}

void AudioConverterSettingsDialog::guiSettingsClicked()
{
  showSettings(MusECore::AudioConverterSettings::GuiMode);
}

void AudioConverterSettingsDialog::showSettings(int mode)
{
  if(!_settings)
    return;
  
  QListWidgetItem* cur_item = converterList->currentItem();
  if(!cur_item)
    return;
  
  int id = cur_item->data(Qt::UserRole).toInt();
  if(id < 0)
    return;
  
  MusECore::AudioConverterSettingsI* setI = _settings->find(id);
  if(!setI)
    return;
  
  if(setI->executeUI(mode, this, _isLocal) == QDialog::Accepted)
    OKButton->setEnabled(true);
}

void AudioConverterSettingsDialog::useDefaultsClicked()
{
  OKButton->setEnabled(true);
  preferencesGroup->setEnabled(!useDefaultPreferences->isChecked());
}

void AudioConverterSettingsDialog::okClicked()
{
  accept();
}

void AudioConverterSettingsDialog::cancelClicked()
{
  reject();
}

//---------------------------------------------------------
//   accept
//---------------------------------------------------------

void AudioConverterSettingsDialog::accept()
{
  if(!_settings)
  {
    QDialog::accept();
    return;
  }
  
  QVariant d;
  d = sampleratePreferenceComboBox->currentData();
  if(d.isValid())
  {
    const int id = d.toInt();
    _settings->_options._preferredResampler = id;
  }
  d = shiftingPreferenceComboBox->currentData();
  if(d.isValid())
  {
    const int id = d.toInt();
    _settings->_options._preferredShifter = id;
  }
  
  _settings->_options._useSettings = !useDefaultPreferences->isChecked();

  QDialog::accept();
}

} // namespace MusEGui

