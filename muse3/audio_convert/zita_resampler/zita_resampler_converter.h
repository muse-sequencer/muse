//=========================================================
//  MusE
//  Linux Music Editor
//
//  zita_resampler_converter.h
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

#ifndef __ZITA_RESAMPLER_CONVERTER_H__
#define __ZITA_RESAMPLER_CONVERTER_H__

#include "config.h"

#ifdef ZITA_RESAMPLER_SUPPORT
#include <zita-resampler/resampler.h>
#include <zita-resampler/vresampler.h>
#endif

#include <QDialog>
#include <QWidget>

#include "xml.h"

#include "ui_zita_resampler_settings_base.h"
#include "audio_convert/lib_audio_convert/audioconvert.h"

namespace MusECore {

//---------------------------------------------------------
//   ZitaResamplerAudioConverter
//---------------------------------------------------------

struct ZitaResamplerAudioConverterOptions
{
  // Some hard-coded defaults.
  static const ZitaResamplerAudioConverterOptions defaultOfflineOptions;
  static const ZitaResamplerAudioConverterOptions defaultRealtimeOptions;
  static const ZitaResamplerAudioConverterOptions defaultGuiOptions;
  
  int _mode;
  // Whether to use these settings or defer to 
  //  some higher default settings if available.
  // The highest level (defaults) will ignore this value.
  bool _useSettings;
  
  ZitaResamplerAudioConverterOptions(bool useSettings = false,
                                     int mode = AudioConverterSettings::OfflineMode)
  {
    initOptions(useSettings, mode);
  }
  
  void initOptions(bool useSettings, 
                   int mode)
  {
    _mode = mode;
    
    _useSettings = useSettings;
    
#ifdef ZITA_RESAMPLER_SUPPORT
    //_converterType = (isLocal ? -1 : 0); // TODO Default setting
#else
    //_converterType = -1);
#endif
  }
  
  void read(Xml&);
  void write(int, Xml&) const;
  
  // Returns whether any option is set ie. non-default.
  //bool isSet() const { return false; } //_converterType != -1; }
  bool useSettings() const { return _useSettings; }
  
  bool isDefault() const { return *this == defaultOfflineOptions; }
  
  bool operator==(const ZitaResamplerAudioConverterOptions& other) const
  {
    return other._useSettings == _useSettings; // && other._converterType == _converterType;
  }
};

class ZitaResamplerAudioConverterSettings : public AudioConverterSettings
{
  protected:
    ZitaResamplerAudioConverterOptions _realtimeOptions;
    ZitaResamplerAudioConverterOptions _offlineOptions;
    ZitaResamplerAudioConverterOptions _guiOptions;
    
  public:
    ZitaResamplerAudioConverterSettings(bool isLocal);
    
    void assign(const AudioConverterSettings&);
    
    void initOptions(bool /*isLocal*/) {
      _offlineOptions.initOptions( ZitaResamplerAudioConverterOptions::defaultOfflineOptions._useSettings,
                                   AudioConverterSettings::OfflineMode);
      
      _realtimeOptions.initOptions(ZitaResamplerAudioConverterOptions::defaultRealtimeOptions._useSettings,
                                   AudioConverterSettings::RealtimeMode);
      
      _guiOptions.initOptions(     ZitaResamplerAudioConverterOptions::defaultGuiOptions._useSettings,
                                   AudioConverterSettings::GuiMode);
      }
      
    ZitaResamplerAudioConverterOptions* offlineOptions() { return &_offlineOptions; }
    ZitaResamplerAudioConverterOptions* realtimeOptions() { return &_realtimeOptions; }
    ZitaResamplerAudioConverterOptions* guiOptions() { return &_guiOptions; }
    
    int executeUI(int mode, QWidget* parent = NULL, bool isLocal = false);
    void read(Xml&);
    void write(int, Xml&) const;
    // Returns whether to use these settings or defer to default settings.
    // Mode is a combination of AudioConverterSettings::ModeType selecting
    //  which of the settings to check. Can also be <= 0, meaning all.
    bool useSettings(int mode = -1) const;
    
    bool isDefault() const 
    { 
      return _offlineOptions == ZitaResamplerAudioConverterOptions::defaultOfflineOptions &&
             _realtimeOptions == ZitaResamplerAudioConverterOptions::defaultRealtimeOptions &&
             _guiOptions == ZitaResamplerAudioConverterOptions::defaultGuiOptions;
    }
};

class ZitaResamplerAudioConverter : public AudioConverter
{
  private:
      int _options;
      //double _ratio;
      int _filterHLen;
#ifdef ZITA_RESAMPLER_SUPPORT
      VResampler* _rbs;
#endif
   
   public:   
      // Mode is an AudioConverterSettings::ModeType selecting which of the settings to use.
      ZitaResamplerAudioConverter(int systemSampleRate,
                                  int channels, 
                                  AudioConverterSettings* settings, 
                                  int mode);
      ~ZitaResamplerAudioConverter();
      
      virtual bool isValid()
#ifdef ZITA_RESAMPLER_SUPPORT
      { return _rbs != 0; }
#else
      { return false; }
#endif
      virtual void reset();
      virtual void setChannels(int ch);
      // Make sure beforehand that sf samplerate is not <= 0.
      virtual int process(
        SNDFILE* sf_handle,
        const int sf_chans, const double sf_sr_ratio, const StretchList* sf_stretch_list,
        const sf_count_t pos,
        float** buffer, const int channels, const int frames, const bool overwrite);
};


} // namespace MusECore


//========================================================================


namespace MusEGui {

//---------------------------------------------------------
//   ZitaResamplerSettingsDialog
//---------------------------------------------------------

class ZitaResamplerSettingsDialog : public QDialog, public Ui::ZitaResamplerSettingsBase {
      Q_OBJECT

      MusECore::ZitaResamplerAudioConverterOptions* _options;
      
      void setControls();
     
   private slots:
      virtual void buttonClicked(int);
      virtual void accept();

   public:
      enum buttonId { DefaultsButtonId, ConverterButtonId, OkButtonId, CancelButtonId };
      ZitaResamplerSettingsDialog(int mode, 
                                  QWidget* parent = NULL, 
                                  MusECore::AudioConverterSettings* settings = NULL, 
                                  bool isLocal = false);
      };

} // namespace MusEGui


#endif

