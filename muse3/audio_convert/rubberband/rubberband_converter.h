//=========================================================
//  MusE
//  Linux Music Editor
//
//  rubberband_converter.h
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

#ifndef __RUBBERBAND_CONVERTER_H__
#define __RUBBERBAND_CONVERTER_H__

#include "config.h"

#ifdef RUBBERBAND_SUPPORT
#include <rubberband/RubberBandStretcher.h>
#endif

#include <QDialog>
#include <QWidget>

#include "xml.h"

#include "ui_rubberband_settings_base.h"
#include "audio_convert/lib_audio_convert/audioconvert.h"

namespace MusECore {

//---------------------------------------------------------
//   RubberBandAudioConverter
//---------------------------------------------------------

struct RubberBandAudioConverterOptions
{
  // Some hard-coded defaults.
  static const RubberBandAudioConverterOptions defaultOfflineOptions;
  static const RubberBandAudioConverterOptions defaultRealtimeOptions;
  static const RubberBandAudioConverterOptions defaultGuiOptions;
  
  AudioConverterSettings::ModeType _mode;
  // Whether to use these settings or defer to 
  //  some higher default settings if available.
  // The highest level (defaults) will ignore this value.
  bool _useSettings;
  int _options;
  
  RubberBandAudioConverterOptions(bool useSettings = false,
                           AudioConverterSettings::ModeType mode = AudioConverterSettings::OfflineMode,
#ifdef RUBBERBAND_SUPPORT
                           int options = RubberBand::RubberBandStretcher::DefaultOptions)
#else
                           int options = 0)
#endif
  {
    initOptions(useSettings, mode, options);
  }
  
  void initOptions(bool useSettings,
                   AudioConverterSettings::ModeType mode,
                   int options)
  {
    _mode = mode;
    _useSettings = useSettings;
    _options = options;
  }
  
  void read(Xml&);
  void write(int, Xml&) const;
  
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
    RubberBandAudioConverterOptions _realtimeOptions;
    RubberBandAudioConverterOptions _offlineOptions;
    RubberBandAudioConverterOptions _guiOptions;
    
  public:
    RubberBandAudioConverterSettings(bool isLocal);
    
    void assign(const AudioConverterSettings&);
    
    void initOptions(bool /*isLocal*/) {
      _offlineOptions.initOptions(
        RubberBandAudioConverterOptions::defaultOfflineOptions._useSettings,
        AudioConverterSettings::OfflineMode,
        RubberBandAudioConverterOptions::defaultOfflineOptions._options);
      
      _realtimeOptions.initOptions(
        RubberBandAudioConverterOptions::defaultRealtimeOptions._useSettings,
        AudioConverterSettings::RealtimeMode,
        RubberBandAudioConverterOptions::defaultRealtimeOptions._options);
      
      _guiOptions.initOptions(
        RubberBandAudioConverterOptions::defaultGuiOptions._useSettings,
        AudioConverterSettings::GuiMode,
        RubberBandAudioConverterOptions::defaultGuiOptions._options);}
      
    RubberBandAudioConverterOptions* offlineOptions() { return &_offlineOptions; }
    RubberBandAudioConverterOptions* realtimeOptions() { return &_realtimeOptions; }
    RubberBandAudioConverterOptions* guiOptions() { return &_guiOptions; }
    
    int executeUI(ModeType mode, QWidget* parent = NULL, bool isLocal = false);
    void read(Xml&);
    void write(int, Xml&) const;
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
      bool _latencyCompPending;
      int _options;
#ifdef RUBBERBAND_SUPPORT
      RubberBand::RubberBandStretcher* _rbs;
#endif
   
   public:   
      // Mode is an AudioConverterSettings::ModeType selecting which of the settings to use.
      RubberBandAudioConverter(int systemSampleRate,
                               int channels, 
                               AudioConverterSettings* settings, 
                               AudioConverterSettings::ModeType mode);
      ~RubberBandAudioConverter();
      
      bool isValid() const
#ifdef RUBBERBAND_SUPPORT
      { return _rbs != 0; }
#else
      { return false; }
#endif
      void reset();
      void setChannels(int ch);

      AudioConverterSettings::ModeType mode() const;

      // Make sure beforehand that sf samplerate is not <= 0.
      int process(
        SNDFILE* sf_handle,
        const int sf_chans, const double sf_sr_ratio, const StretchList* sf_stretch_list,
        const sf_count_t pos,
        float** buffer, const int channels, const int frames, const bool overwrite);
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
      
      void setControls(int opts);
     
   private slots:
      virtual void buttonClicked(int);
      virtual void accept();

   public:
      enum buttonId { DefaultsButtonId, ConverterButtonId, OkButtonId, CancelButtonId, DefaultPresetId, PercussionPresetId, MaxPresetId };
      RubberbandSettingsDialog(MusECore::AudioConverterSettings::ModeType mode,
                               QWidget* parent = NULL,
                               MusECore::AudioConverterSettings* settings = NULL,
                               bool isLocal = false);
      };

} // namespace MusEGui


#endif

