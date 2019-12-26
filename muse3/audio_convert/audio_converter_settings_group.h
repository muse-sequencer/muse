//=========================================================
//  MusE
//  Linux Music Editor
//
//  audio_converter_settings_group.h
//  (C) Copyright 2010-2020 Tim E. Real (terminator356 A T sourceforge D O T net)
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

#ifndef __AUDIO_CONVERTER_SETTINGS_GROUP_H__
#define __AUDIO_CONVERTER_SETTINGS_GROUP_H__

#include <list>

#include "audio_convert/audio_converter_plugin.h"

namespace MusECore {

//---------------------------------------------------------
//   AudioConverterSettingsGroup
//---------------------------------------------------------

class AudioConverterSettingsGroup;
struct AudioConverterSettingsGroupOptions
{
  // Some hard-coded defaults.
  static const AudioConverterSettingsGroupOptions defaultOptions;
  
  // Whether to use these settings or defer to 
  //  some higher default settings if available.
  // The highest level (defaults) will ignore this value.
  bool _useSettings;
  
  int _preferredResampler;
  int _preferredShifter;
  
  AudioConverterSettingsGroupOptions(bool useSettings = defaultOptions._useSettings,
                                     int preferredResampler = defaultOptions._preferredResampler,
                                     int preferredShifter = defaultOptions._preferredShifter)
  {
    initOptions(useSettings, preferredResampler, preferredShifter);
  }
  
  void initOptions(bool useSettings, 
                   int preferredResampler, 
                   int preferredShifter)
  {
    _useSettings = useSettings;
    _preferredResampler = preferredResampler;
    _preferredShifter = preferredShifter;
  }
  
  void read(Xml&);
  void write(int, Xml&) const;
  
  // Returns whether to use these settings or defer to default settings.
  bool useSettings() const { return _useSettings; }
  
  bool isDefault() const { return *this == defaultOptions; }
  
  bool operator==(const AudioConverterSettingsGroupOptions& other) const
  {
    return other._useSettings == _useSettings && 
           other._preferredResampler == _preferredResampler &&
           other._preferredShifter == _preferredShifter;
  }
};

class AudioConverterSettingsGroup : public std::list<AudioConverterSettingsI*>
{
  private:
    bool _isLocal;
    
    void clearDelete();
    
  public:
    AudioConverterSettingsGroupOptions _options;
    
    AudioConverterSettingsGroup(bool isLocal) : _isLocal(isLocal)
    { 
      initOptions();
    }
    virtual ~AudioConverterSettingsGroup();
      
    void initOptions() {
      _options.initOptions( AudioConverterSettingsGroupOptions::defaultOptions._useSettings,
                            AudioConverterSettingsGroupOptions::defaultOptions._preferredResampler,
                            AudioConverterSettingsGroupOptions::defaultOptions._preferredShifter); }
    
    void assign(const AudioConverterSettingsGroup&);
    void populate(AudioConverterPluginList* plugList, bool isLocal = false);
    
    AudioConverterSettingsI* find(int ID) const;

    void readItem(Xml&, AudioConverterPluginList* plugList);
    void read(Xml&, AudioConverterPluginList* plugList);
    void write(int, Xml&, AudioConverterPluginList* plugList) const;
    
    // Returns whether to use these settings or defer to default settings.
    // Mode is a combination of AudioConverterSettings::ModeType selecting
    //  which of the settings to check. Can also be <= 0, meaning all.
    bool useSettings(int mode = -1) const;
    
    bool isDefault() const;
};

typedef AudioConverterSettingsGroup::iterator iAudioConverterSettingsI;
typedef AudioConverterSettingsGroup::const_iterator ciAudioConverterSettingsI;

} // namespace MusECore

#endif

