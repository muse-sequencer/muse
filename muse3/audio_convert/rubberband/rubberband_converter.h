//=========================================================
//  MusE
//  Linux Music Editor
//
//  rubberband_converter.h
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

#ifndef __RUBBERBAND_CONVERTER_H__
#define __RUBBERBAND_CONVERTER_H__

#include "config.h"

// REMOVE Tim. samplerate. TESTING. Remove later.
//#define RUBBERBAND_SUPPORT

#ifdef RUBBERBAND_SUPPORT
//#include <RubberBandStretcher.h>
#include <rubberband/RubberBandStretcher.h>
#endif

#include <sndfile.h>

#include "ui_rubberband_settings_base.h"
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
//   RubberBandAudioConverter
//---------------------------------------------------------

struct RubberBandAudioConverterOptions
{
  // Some hard-coded defaults.
  static const RubberBandAudioConverterOptions defaultOfflineOptions;
  static const RubberBandAudioConverterOptions defaultRealtimeOptions;
  static const RubberBandAudioConverterOptions defaultGuiOptions;
  
  int _mode;
  // Whether to use these settings or defer to 
  //  some higher default settings if available.
  // The highest level (defaults) will ignore this value.
  bool _useSettings;
  
//   // When used as local options, for all options -1 means default.
//   // These options may not be changed after construction:
//   int _optionProcess;
//   int _optionStretch;
//   int _optionThreading;
//   int _optionWindow;
//   int _optionSmoothing;
//   int _optionChannels;
//   
//   // These options may be changed after construction only in realtime mode:
//   int _optionTransients;
//   int _optionDetector;
//   
//   // These options may be changed any time after construction:
//   int _optionPhase;
//   int _optionFormant;
//   int _optionPitch;

  int _options;
  
  RubberBandAudioConverterOptions(bool useSettings = false,
                           int mode = AudioConverterSettings::OfflineMode,
#ifdef RUBBERBAND_SUPPORT
                           int options = RubberBand::RubberBandStretcher::DefaultOptions)
#else
                           int options = 0)
#endif
  {
    initOptions(useSettings, mode, options);
  }
  
//   void initOptions(bool isLocal = false)
//   {
//     //const int val = (isLocal ? -1 : RubberBand::RubberBandStretcher::DefaultOptions);
//     //_optionProcess = _optionStretch = _optionThreading = 
//     //  _optionWindow = _optionSmoothing = _optionChannels = 
//     //  _optionTransients = _optionDetector = _optionPhase = 
//     //  _optionFormant = _optionPitch = val;
// #ifdef RUBBERBAND_SUPPORT
//     _options = (isLocal ? -1 : RubberBand::RubberBandStretcher::DefaultOptions);
// #else
//     _options = -1;
// #endif
//   }
  void initOptions(bool useSettings,
                   int mode,
                   int options)
  {
    _mode = mode;
    
    //_useSettings = isLocal;
    //_useSettings = false; // Start with false
    _useSettings = useSettings;
    
    //_converterType = (isLocal ? -1 : SRC_SINC_MEDIUM_QUALITY);
    //_converterType = SRC_SINC_MEDIUM_QUALITY;
    _options = options;
  }
  
  void read(Xml&);
  void write(int, Xml&) const;
  
  // Returns whether any option is set ie. non-default.
  //bool isSet() const { return _options != -1; }
  // Returns whether to use these settings or defer to default settings.
  bool useSettings() const { return _useSettings; }
  
  bool isDefault() const { return *this == defaultOfflineOptions; }
  
  bool operator==(const RubberBandAudioConverterOptions& other) const
  {
    return other._useSettings == _useSettings && other._options == _options;
  }
};

class RubberBandAudioConverterSettings : public AudioConverterSettings
{
  protected:
//     // These options may not be changed after construction:
//     RubberBand::RubberBandStretcher::Options _optionProcess;
//     RubberBand::RubberBandStretcher::Options _optionStretch;
//     RubberBand::RubberBandStretcher::Options _optionThreading;
//     RubberBand::RubberBandStretcher::Options _optionWindow;
//     RubberBand::RubberBandStretcher::Options _optionSmoothing;
//     RubberBand::RubberBandStretcher::Options _optionChannels;
//     
//     // These options may be changed after construction only in realtime mode:
//     RubberBand::RubberBandStretcher::Options _optionTransients;
//     RubberBand::RubberBandStretcher::Options _optionDetector;
//     
//     // These options may be changed any time after construction:
//     RubberBand::RubberBandStretcher::Options _optionPhase;
//     RubberBand::RubberBandStretcher::Options _optionFormant;
//     RubberBand::RubberBandStretcher::Options _optionPitch;
    
    RubberBandAudioConverterOptions _realtimeOptions;
    RubberBandAudioConverterOptions _offlineOptions;
    RubberBandAudioConverterOptions _guiOptions;
    
  public:
//     // Some hard-coded defaults.
//     static const RubberBandAudioConverterOptions defaultOfflineOptions;
//     static const RubberBandAudioConverterOptions defaultRealtimeOptions;
//     static const RubberBandAudioConverterOptions defaultGuiOptions;
    
    //RubberBandAudioConverterSettings(int converterID, bool isLocal = false) :
    //  AudioConverterSettings(converterID)
    //  { initOptions(isLocal); }
    RubberBandAudioConverterSettings(bool isLocal);
    
    // Creates another new settings object. Caller is responsible for deleting the returned object.
    // Settings will initialize normally. or with 'don't care', if isLocal is false or true resp.
//     virtual AudioConverterSettings* createSettings(bool isLocal)
//     {
//       return new MusECore::RubberBandAudioConverterSettings(isLocal);
//     }
    
    void assign(const AudioConverterSettings&);
    
    void initOptions(bool /*isLocal*/) {
      //_offlineOptions.initOptions( isLocal ? RubberBandAudioConverterOptions::defaultOfflineOptions._useSettings : true, // Force non-local to use settings.
      _offlineOptions.initOptions( RubberBandAudioConverterOptions::defaultOfflineOptions._useSettings,
                                   AudioConverterSettings::OfflineMode,
                                   RubberBandAudioConverterOptions::defaultOfflineOptions._options);
      
      //_realtimeOptions.initOptions(isLocal ? RubberBandAudioConverterOptions::defaultRealtimeOptions._useSettings : true, // Force non-local to use settings.
      _realtimeOptions.initOptions(RubberBandAudioConverterOptions::defaultRealtimeOptions._useSettings,
                                   AudioConverterSettings::RealtimeMode,
                                   RubberBandAudioConverterOptions::defaultRealtimeOptions._options);
      
      //_guiOptions.initOptions(     isLocal ? RubberBandAudioConverterOptions::defaultGuiOptions._useSettings : true, // Force non-local to use settings.
      _guiOptions.initOptions(     RubberBandAudioConverterOptions::defaultGuiOptions._useSettings,
                                   AudioConverterSettings::GuiMode,
                                   RubberBandAudioConverterOptions::defaultGuiOptions._options);}
      
    RubberBandAudioConverterOptions* offlineOptions() { return &_offlineOptions; }
    RubberBandAudioConverterOptions* realtimeOptions() { return &_realtimeOptions; }
    RubberBandAudioConverterOptions* guiOptions() { return &_guiOptions; }
    
    int executeUI(int mode, QWidget* parent = NULL, bool isLocal = false);
    void read(Xml&);
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
      return _offlineOptions == RubberBandAudioConverterOptions::defaultOfflineOptions &&
             _realtimeOptions == RubberBandAudioConverterOptions::defaultRealtimeOptions &&
             _guiOptions == RubberBandAudioConverterOptions::defaultGuiOptions;
    }
};

class RubberBandAudioConverter : public AudioConverter
{
  private:
//       static AudioConverterDescriptor _descriptor;
      //RubberBandAudioConverterSettings _localSettings;
      int _options;
#ifdef RUBBERBAND_SUPPORT
      RubberBand::RubberBandStretcher* _rbs;
#endif
   
   public:   
      // Mode is an AudioConverterSettings::ModeType selecting which of the settings to use.
      RubberBandAudioConverter(int channels, 
                               AudioConverterSettings* settings, 
                               int mode);
      ~RubberBandAudioConverter();
      
      //static RubberBandAudioConverterSettings defaultSettings;
      
//       static int converterID() { return RubberBand; }
//       static const char* name() { return "Rubberband Resampler/Stretcher/Shifter"; }
//       static int capabilities() { return SampleRate | Stretch | Pitch; }
//       // -1 means infinite, don't care.
//       static int maxChannels() { return -1; }
      
//       static const AudioConverterDescriptor* converterDescriptor() { return &_descriptor; }
//       virtual const AudioConverterDescriptor* descriptor() const { return converterDescriptor(); }

      virtual bool isValid() 
#ifdef RUBBERBAND_SUPPORT
      { return _rbs != 0; }
#else
      { return false; }
#endif
      virtual void reset();
      virtual void setChannels(int ch);
      virtual int process(SndFile* sf, SNDFILE* handle, sf_count_t pos, float** buffer, 
                            int channels, int frames, bool overwrite);
      
      //virtual void read(Xml&);
      //virtual void write(int, Xml&) const;
};


} // namespace MusECore


//=============================================================================


namespace MusEGui {

//---------------------------------------------------------
//   RubberbandSettingsDialog
//---------------------------------------------------------

class RubberbandSettingsDialog : public QDialog, public Ui::RubberbandSettingsBase {
      Q_OBJECT

      MusECore::RubberBandAudioConverterOptions* _options;
      QSignalMapper* _signalMapper;
      
      void setControls(int opts);
     
   private slots:
      virtual void buttonClicked(int);
      virtual void accept();

   public:
      enum buttonId { DefaultsButtonId, ConverterButtonId, OkButtonId, CancelButtonId, DefaultPresetId, PercussionPresetId, MaxPresetId };
      RubberbandSettingsDialog(int mode,
                               QWidget* parent = NULL,
                               MusECore::AudioConverterSettings* settings = NULL,
                               bool isLocal = false);
      };

} // namespace MusEGui


#endif

