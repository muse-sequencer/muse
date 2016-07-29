//=========================================================
//  MusE
//  Linux Music Editor
//
//  audio_converter_settings_group.h
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

#ifndef __AUDIO_CONVERTER_SETTINGS_GROUP_H__
#define __AUDIO_CONVERTER_SETTINGS_GROUP_H__

//#include "ui_audio_converter_settings_base.h"

//#include "audio_convert/lib_audio_convert/audioconvert.h"

#include <list>

//class QWidget;
//class QDialog;
//class QListWidgetItem;


namespace MusECore {
class AudioConverterSettingsI;
//class AudioConverterSettingsGroup;
class AudioConverterPluginList;
class Xml;
class PendingOperationList;

// namespace MusEGui {
//   
// class AudioConverterSettingsDialog : public QDialog, public Ui::AudioConverterSettingsBase {
//       Q_OBJECT
// 
//    private slots:
//       virtual void accept();
//       //void sysexChanged(QListWidgetItem*, QListWidgetItem*);
// 
//    public:
//       AudioConverterSettingsDialog(QWidget* parent = NULL, MusECore::AudioConverterSettingsGroup* settings = 0, bool isLocal = false);
//       //SRCResamplerSettingsDialog(QWidget* parent = NULL, MusECore::MidiInstrument* instr = NULL);
//       };
// 
// } // namespace MusEGui


//======================================================================


// #include "src_converter.h"
// #include "zita_resampler_converter.h"
// #include "rubberband_converter.h"

//namespace MusECore {
//class Xml;

// class AudioConverterSettings
// {
//   public:
//     AudioConverterSettings() { }
//   
//     virtual int executeUI() = 0;
//     virtual void read(Xml&) = 0;
//     virtual void write(int, Xml&) const = 0;
//     // Returns whether any setting is set ie. non-default.
//     virtual bool isSet() const = 0;
// };

// } // namespace MusECore
// 
// #include "src_converter.h"
// #include "zita_resampler_converter.h"
// #include "rubberband_converter.h"
// 
// namespace MusECore {
// class Xml;
  
// //---------------------------------------------------------
// //   AudioConverterSettingsGroup
// //---------------------------------------------------------
// 
// class AudioConverterSettingsGroup
// {
//   public:
//     AudioConverterSettingsGroup(bool isLocal = false) { initOptions(isLocal); }
//     virtual ~AudioConverterSettingsGroup() { }
//   
//     // These are AudioConverter::ConverterID types, but can be -1 to indicate no override.
//     int preferredResampler;
//     int preferredShifter;
//     
//     SRCAudioConverterSettings SRCResamplerSettings;
//     ZitaResamplerAudioConverterSettings zitaResamplerSettings;
//     RubberBandAudioConverterSettings rubberbandSettings;
//     //SoundTouchAudioConverterSettings soundtouchSettings; // TODO
//       
//     virtual void initOptions(bool isLocal = false) {
//       preferredResampler = (isLocal ? -1 : AudioConverter::ZitaResampler);
//       preferredShifter      = (isLocal ? -1 : AudioConverter::RubberBand);
//       SRCResamplerSettings.initOptions(isLocal);
//       zitaResamplerSettings.initOptions(isLocal);
//       rubberbandSettings.initOptions(isLocal);
//       //soundtouchSettings.initOptions(isLocal); // TODO
//     }
//     //virtual int executeUI() = 0;
//     virtual void read(Xml&);
//     virtual void write(int, Xml&) const;
//     // Returns whether any settings are set ie. non-default.
//     virtual bool isSet() const { return SRCResamplerSettings.isSet()
//                                         || zitaResamplerSettings.isSet()
//                                         || rubberbandSettings.isSet()
//                                         //|| soundtouchSettings.isSet() // TODO
//                                         ; }
// };


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
  
  // Returns whether any option is set ie. non-default.
  //bool isSet() const { return _converterType != -1; }
  // Returns whether to use these settings or defer to default settings.
  bool useSettings() const { return _useSettings; }
  
  //bool isDefault() const { return *this == AudioConverterSettingsGroup::defaultOptions; }
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
    
  //protected:
  //  AudioConverterSettingsGroupOptions _options;
    
  public:
    // These are AudioConverter::ConverterID types, but can be -1 to indicate no override.
    //int preferredResampler;
    //int preferredShifter;

    AudioConverterSettingsGroupOptions _options;
    
    // Some hard-coded defaults.
    //static const int defaultPreferredResampler;
    //static const int defaultPreferredShifter;
    //static const int defaultPreferredLocalValue;
    //static const AudioConverterSettingsGroupOptions defaultOptions;
    
    AudioConverterSettingsGroup(bool isLocal) : _isLocal(isLocal)
    { 
      initOptions();
    }
    virtual ~AudioConverterSettingsGroup();
      
//     void initOptions()
//     {
//       if(_isLocal) 
//         preferredShifter = preferredResampler = -1;
//       else
//       {
//         preferredResampler = defaultPreferredResampler;
//         preferredShifter = defaultPreferredShifter;
//       }
//     }
    
    //void initOptions(bool isLocal) {
//     void initOptions() {
//       _options.initOptions( defaultOptions._useSettings,
//                             _isLocal,
//                             defaultOptions._preferredResampler,
//                             defaultOptions._preferredShifter); }
    void initOptions() {
      //_options.initOptions( _isLocal ? AudioConverterSettingsGroupOptions::defaultOptions._useSettings : true, // Force non-local to use settings.
      _options.initOptions( AudioConverterSettingsGroupOptions::defaultOptions._useSettings,
                            AudioConverterSettingsGroupOptions::defaultOptions._preferredResampler,
                            AudioConverterSettingsGroupOptions::defaultOptions._preferredShifter); }
      
    
    
    void assign(const AudioConverterSettingsGroup&);
    void populate(AudioConverterPluginList* plugList, bool isLocal = false);
    
    // Creates another new settings group object. Caller is responsible for deleting the returned object.
    // Settings will initialize normally. or with 'don't care', if isLocal is false or true resp.
//     AudioConverterSettingsGroup* createSettings(AudioConverterPluginList* plugList, bool isLocal);
    
    
//     SRCAudioConverterSettings SRCResamplerSettings;
//     ZitaResamplerAudioConverterSettings zitaResamplerSettings;
//     RubberBandAudioConverterSettings rubberbandSettings;
//     //SoundTouchAudioConverterSettings soundtouchSettings; // TODO
      
//     void initOptions(bool isLocal = false) {
//       preferredResampler = (isLocal ? -1 : AudioConverter::ZitaResampler);
//       preferredShifter      = (isLocal ? -1 : AudioConverter::RubberBand);
//       SRCResamplerSettings.initOptions(isLocal);
//       zitaResamplerSettings.initOptions(isLocal);
//       rubberbandSettings.initOptions(isLocal);
//       //soundtouchSettings.initOptions(isLocal); // TODO
//     }
    
//     void initOptions(AudioConverterPluginList* plugList, bool isLocal = false);

    //AudioConverterSettingsGroupOptions* options() const { return &_options; }
    
    AudioConverterSettingsI* find(int ID) const;
    //AudioConverterSettingsI* find(const char* name = NULL, int ID = -1) const;
   //int executeUI() = 0;
    void readItem(Xml&);
    void read(Xml&);
    void write(int, Xml&) const;
    
//     // Returns whether any settings are set ie. non-default.
//     // Mode is a combination of AudioConverterSettings::ModeType selecting
//     //  which of the settings to check. Can also be <= 0, meaning all.
//     bool isSet(int mode = -1) const;
    // Returns whether to use these settings or defer to default settings.
    // Mode is a combination of AudioConverterSettings::ModeType selecting
    //  which of the settings to check. Can also be <= 0, meaning all.
    bool useSettings(int mode = -1) const;
    
    bool isDefault() const;
    
    //bool operator==(const AudioConverterSettingsGroup& other) const;
};

typedef AudioConverterSettingsGroup::iterator iAudioConverterSettingsI;
typedef AudioConverterSettingsGroup::const_iterator ciAudioConverterSettingsI;

} // namespace MusECore


namespace MusEGlobal {
extern MusECore::AudioConverterSettingsGroup* defaultAudioConverterSettings;
extern void modifyDefaultAudioConverterSettingsOperation(MusECore::AudioConverterSettingsGroup* settings, MusECore::PendingOperationList& ops);
}

#endif

