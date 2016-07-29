//=========================================================
//  MusE
//  Linux Music Editor
//
//  src_converter.h
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

#ifndef __SRC_CONVERTER_H__
#define __SRC_CONVERTER_H__

//#include "config.h"

#include <sndfile.h>
#include <samplerate.h>

#include "ui_src_resampler_settings_base.h"
#include "audio_convert/lib_audio_convert/audioconvert.h"

//class QWidget;
class QDialog;
class QWidget;
class QSignalMapper;
//class QListWidgetItem;


namespace MusECore {
class SndFile;
class Xml;

//---------------------------------------------------------
//   SRCAudioConverter
//---------------------------------------------------------

struct SRCAudioConverterOptions
{
  // Some hard-coded defaults.
  static const SRCAudioConverterOptions defaultOfflineOptions;
  static const SRCAudioConverterOptions defaultRealtimeOptions;
  static const SRCAudioConverterOptions defaultGuiOptions;
  
  int _mode;
  // Whether to use these settings or defer to 
  //  some higher default settings if available.
  // The highest level (defaults) will ignore this value.
  bool _useSettings;
  
  int _converterType;
  
  SRCAudioConverterOptions(bool useSettings = defaultOfflineOptions._useSettings, 
                           int mode = AudioConverterSettings::OfflineMode,
                           int converterType = defaultOfflineOptions._converterType)
  {
    initOptions(useSettings, mode, converterType);
  }
  
  void initOptions(bool useSettings, 
                   int mode,
                   int converterType)
  {
    _mode = mode;
    
    //_useSettings = isLocal;
    //_useSettings = false; // Start with false
    _useSettings = useSettings;
    
    //_converterType = (isLocal ? -1 : SRC_SINC_MEDIUM_QUALITY);
    //_converterType = SRC_SINC_MEDIUM_QUALITY;
    _converterType = converterType;
  }
  
  void read(Xml&);
  void write(int, Xml&) const;
  
  // Returns whether any option is set ie. non-default.
  //bool isSet() const { return _converterType != -1; }
  // Returns whether to use these settings or defer to default settings.
  bool useSettings() const { return _useSettings; }
  
  bool isDefault() const { return *this == defaultOfflineOptions; }
  
  bool operator==(const SRCAudioConverterOptions& other) const
  {
    return other._useSettings == _useSettings && other._converterType == _converterType;
  }
};

class SRCAudioConverterSettings : public AudioConverterSettings
{
  protected:
    SRCAudioConverterOptions _offlineOptions;
    SRCAudioConverterOptions _realtimeOptions;
    SRCAudioConverterOptions _guiOptions;
    
  public:
    // Some hard-coded defaults.
    //static const SRCAudioConverterOptions defaultOfflineOptions;
    //static const SRCAudioConverterOptions defaultRealtimeOptions;
    //static const SRCAudioConverterOptions defaultGuiOptions;
    
    //SRCAudioConverterSettings(int converterID, bool isLocal = false) : 
    //  AudioConverterSettings(converterID)
    //  { initOptions(isLocal); }
    SRCAudioConverterSettings(bool isLocal);
    
    // Creates another new settings object. Caller is responsible for deleting the returned object.
    // Settings will initialize normally. or with 'don't care', if isLocal is false or true resp.
//     virtual AudioConverterSettings* createSettings(bool isLocal)
//     {
//       return new MusECore::SRCAudioConverterSettings(isLocal);
//     }
    
    void assign(const AudioConverterSettings&);
    
    void initOptions(bool /*isLocal*/) {
      //_offlineOptions.initOptions( isLocal ? SRCAudioConverterOptions::defaultOfflineOptions._useSettings : true, // Force non-local to use settings.
      _offlineOptions.initOptions( SRCAudioConverterOptions::defaultOfflineOptions._useSettings,
                                   AudioConverterSettings::OfflineMode,
                                   SRCAudioConverterOptions::defaultOfflineOptions._converterType);
      
      _realtimeOptions.initOptions(SRCAudioConverterOptions::defaultRealtimeOptions._useSettings,
                                   AudioConverterSettings::RealtimeMode,
                                   SRCAudioConverterOptions::defaultRealtimeOptions._converterType);
      
      _guiOptions.initOptions(     SRCAudioConverterOptions::defaultGuiOptions._useSettings,
                                   AudioConverterSettings::GuiMode,
                                   SRCAudioConverterOptions::defaultGuiOptions._converterType);}
                                   
    SRCAudioConverterOptions* offlineOptions() { return &_offlineOptions; }
    SRCAudioConverterOptions* realtimeOptions() { return &_realtimeOptions; }
    SRCAudioConverterOptions* guiOptions() { return &_guiOptions; }
    
    int executeUI(int mode, QWidget* parent = NULL, bool isLocal = false);
    //void readItem(Xml&);
    void read(Xml&);
    //void write(int, Xml&, const AudioConverterDescriptor* descriptor) const;
    void write(int, Xml&) const;
//     // Returns whether any setting is set ie. non-default.
//     // Mode is a combination of AudioConverterSettings::ModeType selecting
//     //  which of the settings to check. Can also be <= 0, meaning all.
//     bool isSet(int mode = -1) const;
    // Returns whether to use these settings or defer to default settings.
    // Mode is a combination of AudioConverterSettings::ModeType selecting
    //  which of the settings to check. Can also be <= 0, meaning all.
    bool useSettings(int mode = -1) const;
    
    bool isDefault() const 
    { 
      return _offlineOptions == SRCAudioConverterOptions::defaultOfflineOptions &&
             _realtimeOptions == SRCAudioConverterOptions::defaultRealtimeOptions &&
             _guiOptions == SRCAudioConverterOptions::defaultGuiOptions;
    }
};

class SRCAudioConverter : public AudioConverter
{
  private:
//       static AudioConverterDescriptor _descriptor;
      //SRCAudioConverterSettings _localSettings;
      int _type;
      SRC_STATE* _src_state;
   
   public:   
      // Mode is an AudioConverterSettings::ModeType selecting which of the settings to use.
      SRCAudioConverter(int channels, AudioConverterSettings* settings, int mode);
      ~SRCAudioConverter();
      
      //static SRCAudioConverterSettings defaultSettings;
      
//       static int converterID() { return SRCResampler; }
//       static const char* name() { return "SRC Resampler"; }
//       static int capabilities() { return SampleRate; }
//       // -1 means infinite, don't care.
//       static int maxChannels() { return -1; }
//       
//       virtual int converterID() const { return _descriptor._ID; }
//       // The name of the converter.
//       virtual const char* name() const { return _descriptor._name; }
//       // Returns combination of Capabilities values.
//       virtual int capabilities() const { return _descriptor._capabilities; }
//       // Maximum available channels. -1 means infinite, don't care.
//       virtual int maxChannels() const { return _descriptor._maxChannels; }
      
//       static const AudioConverterDescriptor* converterDescriptor() { return &_descriptor; }
//       virtual const AudioConverterDescriptor* descriptor() const { return converterDescriptor(); }
      
      virtual bool isValid() { return _src_state != 0; }
      virtual void reset();
      virtual void setChannels(int ch);
//       virtual sf_count_t process(MusECore::SndFileR& sf, float** buffer, 
//                             int channels, int frames, bool overwrite); // Interleaved buffer if stereo.
//       virtual sf_count_t process(MusECore::SndFileR sf, float** buffer, 
//                             int channels, int frames, bool overwrite); // Interleaved buffer if stereo.
//       virtual sf_count_t process(SndFile* sf, float** buffer, 
//                             int channels, int frames, bool overwrite); // Interleaved buffer if stereo.
      virtual int process(SndFile* sf, SNDFILE* handle, sf_count_t pos, float** buffer, 
                            int channels, int frames, bool overwrite); // Interleaved buffer if stereo.
      
      //virtual void read(Xml&);
      //virtual void write(int, Xml&) const;
};


} // namespace MusECore


//===========================================================================


namespace MusEGui {
  
//---------------------------------------------------------
//   SRCResamplerSettingsDialog
//---------------------------------------------------------

class SRCResamplerSettingsDialog : public QDialog, public Ui::SRCResamplerSettingsBase {
      Q_OBJECT

   private:
      MusECore::SRCAudioConverterOptions* _options;
      QSignalMapper* _signalMapper;
      
      void setControls();
     
   private slots:
      virtual void buttonClicked(int);
      virtual void accept();

   public:
      enum buttonId { DefaultsButtonId, ConverterButtonId, OkButtonId, CancelButtonId };
      SRCResamplerSettingsDialog(int mode, 
                                 QWidget* parent = NULL, 
                                 MusECore::AudioConverterSettings* settings = NULL, 
                                 bool isLocal = false);
      };

} // namespace MusEGui


#endif

