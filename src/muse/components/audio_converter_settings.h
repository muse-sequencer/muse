//=========================================================
//  MusE
//  Linux Music Editor
//
//  audio_converter_settings.h
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

#ifndef __AUDIO_CONVERTER_SETTINGS_H__
#define __AUDIO_CONVERTER_SETTINGS_H__

#include "ui_audio_converter_settings_base.h"

#include "audio_convert/audio_converter_plugin.h"
#include "audio_convert/audio_converter_settings_group.h"

namespace MusEGui {
  
class AudioConverterSettingsDialog : public QDialog, public Ui::AudioConverterSettingsBase {
      Q_OBJECT

   private:
     MusECore::AudioConverterPluginList* _pluginList;
     MusECore::AudioConverterSettingsGroup* _settings;
     bool _isLocal;
     
     void fillList();
     void enableSettingsButtons();
     void showSettings(MusECore::AudioConverterSettings::ModeType mode);
     
   private slots:
      void useDefaultsClicked();
      void okClicked();
      void cancelClicked();
      void accept();
      void converterSelectionChanged();
      void preferredResamplerChanged(int);
      void preferredShifterChanged(int);
      void offlineSettingsClicked();
      void realtimeSettingsClicked();
      void guiSettingsClicked();
      
   public:
      AudioConverterSettingsDialog(QWidget* parent = nullptr,
                                   MusECore::AudioConverterPluginList* pluginList = nullptr,
                                   MusECore::AudioConverterSettingsGroup* settings = nullptr, 
                                   bool isLocal = false);
      
      };

} // namespace MusEGui

#endif
