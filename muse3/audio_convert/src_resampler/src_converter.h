//=========================================================
//  MusE
//  Linux Music Editor
//
//  src_converter.h
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

#ifndef __SRC_CONVERTER_H__
#define __SRC_CONVERTER_H__

#include <samplerate.h>

#include <QDialog>
#include <QWidget>

#include "xml.h"

#include "ui_src_resampler_settings_base.h"
#include "audio_convert/lib_audio_convert/audioconvert.h"

namespace MusECore {

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
    _useSettings = useSettings;
    _converterType = converterType;
  }
  
  void read(Xml&);
  void write(int, Xml&) const;
  
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
    SRCAudioConverterSettings(bool isLocal);
    
    void assign(const AudioConverterSettings&);
    
    void initOptions(bool /*isLocal*/) {
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
    void read(Xml&);
    void write(int, Xml&) const;
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
      int _type;
      SRC_STATE* _src_state;
      SRC_DATA _srcdata;
      float* _inbuffer;
      int _inBufferSize;
      int _curInBufferFrame;
      bool _needBuffer;

      void resetSrcData();

   public:   
      // Mode is an AudioConverterSettings::ModeType selecting which of the settings to use.
      SRCAudioConverter(int systemSampleRate, int channels, AudioConverterSettings* settings, int mode);
      ~SRCAudioConverter();
      
      virtual bool isValid() { return _src_state != 0; }
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


//===========================================================================


namespace MusEGui {
  
//---------------------------------------------------------
//   SRCResamplerSettingsDialog
//---------------------------------------------------------

class SRCResamplerSettingsDialog : public QDialog, public Ui::SRCResamplerSettingsBase {
      Q_OBJECT

   private:
      MusECore::SRCAudioConverterOptions* _options;
      
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

