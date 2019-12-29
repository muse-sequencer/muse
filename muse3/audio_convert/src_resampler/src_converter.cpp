//=========================================================
//  MusE
//  Linux Music Editor
//
//  src_converter.cpp
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
//
//=========================================================

#include <QRadioButton>

#include <stdio.h>

#include "muse_math.h"
#include "time_stretch.h"
#include "src_converter.h"

// For debugging output: Uncomment the fprintf section.
#define ERROR_AUDIOCONVERT(dev, format, args...) fprintf(dev, format, ##args)
#define DEBUG_AUDIOCONVERT(dev, format, args...) // fprintf(dev, format, ##args)

// Fixed audio input buffer size.
#define SRC_IN_BUFFER_FRAMES 1024

// Create a new instance of the plugin.  
// Mode is an AudioConverterSettings::ModeType selecting which of the settings to use.
MusECore::AudioConverter* instantiate(int systemSampleRate,
                                      const MusECore::AudioConverterDescriptor* /*Descriptor*/,
                                      int channels, 
                                      MusECore::AudioConverterSettings* settings, 
                                      MusECore::AudioConverterSettings::ModeType mode)
{
  return new MusECore::SRCAudioConverter(systemSampleRate, channels, settings, mode);
}

// Destroy the instance after usage.
void cleanup(MusECore::AudioConverterHandle instance)
{
  MusECore::AudioConverter::release(instance);
}
  
// Destroy the instance after usage.
void cleanupSettings(MusECore::AudioConverterSettings* instance)
{
  delete instance;
}
  
// Creates a new settings instance. Caller is responsible for deleting the returned object.
// Settings will initialize normally. or with 'don't care', if isLocal is false or true resp.
MusECore::AudioConverterSettings* createSettings(bool isLocal)
{
  return new MusECore::SRCAudioConverterSettings(isLocal);
}


extern "C" 
{
  static MusECore::AudioConverterDescriptor descriptor = {
    1001,
    MusECore::AudioConverter::SampleRate,
    "SRC Resampler",
    "SRC",
    -1,
    1.0,
    1.0,
    // "SRC is capable of ... downsampling by a factor of 256 to upsampling by the same factor."
    0.004,
    256.0,
    1.0,
    1.0,
    instantiate,
    cleanup,
    createSettings,
    cleanupSettings
  };

  // We must compile with -fvisibility=hidden to avoid namespace
  // conflicts with global variables.
  // Only visible symbol is "audio_converter_descriptor".
  // (TODO: all plugins should be compiled this way)
  __attribute__ ((visibility("default")))
  
  const MusECore::AudioConverterDescriptor* audio_converter_descriptor(unsigned long i) {
    return (i == 0) ? &descriptor : 0;
  }
}



//======================================================================

namespace MusECore {

//---------------------------------------------------------
//   SRCAudioConverter
//---------------------------------------------------------

SRCAudioConverter::SRCAudioConverter(
  int systemSampleRate, int channels,
  AudioConverterSettings* settings, AudioConverterSettings::ModeType mode)
  : AudioConverter(systemSampleRate, mode)
{
  DEBUG_AUDIOCONVERT(stderr, "SRCAudioConverter::SRCAudioConverter this:%p channels:%d mode:%d\n", 
                     this, channels, mode);

  SRCAudioConverterSettings* src_settings = static_cast<SRCAudioConverterSettings*>(settings);
  switch(mode)
  {
    case AudioConverterSettings::OfflineMode:
      _type = (src_settings ? src_settings->offlineOptions()->_converterType : 0);
    break;
    
    case AudioConverterSettings::RealtimeMode:
      _type = (src_settings ? src_settings->realtimeOptions()->_converterType : 0);
    break;
    
    case AudioConverterSettings::GuiMode:
      _type = (src_settings ? src_settings->guiOptions()->_converterType : 0);
    break;
    
    default:
      _type = 0;
    break;
  }
  
  _src_state = 0;
  _channels = channels;

  _inBufferSize = SRC_IN_BUFFER_FRAMES * _channels;
  _inbuffer = new float[_inBufferSize];
  _curInBufferFrame = 0;
  _needBuffer = true;

  // Reset the SRC_DATA structure.
  resetSrcData();

  int srcerr;
  DEBUG_AUDIOCONVERT(stderr, "SRCAudioConverter::SRCaudioConverter Creating samplerate converter type:%d with %d channels\n", _type, _channels);
  _src_state = src_new(_type, _channels, &srcerr); 
  if(!_src_state)
  {
    ERROR_AUDIOCONVERT(stderr, "SRCAudioConverter::SRCaudioConverter Creation of samplerate converter type:%d with %d channels failed:%s\n", _type, _channels, src_strerror(srcerr));
  }
}

SRCAudioConverter::~SRCAudioConverter()
{
  DEBUG_AUDIOCONVERT(stderr, "SRCAudioConverter::~SRCAudioConverter this:%p\n", this);

  if(_inbuffer)
    delete[] _inbuffer;

  if(_src_state)
    src_delete(_src_state);
}

void SRCAudioConverter::setChannels(int ch)
{
  DEBUG_AUDIOCONVERT(stderr, "SRCAudioConverter::setChannels this:%p channels:%d\n", this, ch);
  if(_src_state)
    src_delete(_src_state);
  _src_state = 0;
  
  _channels = ch;

  if(_inbuffer)
    delete[] _inbuffer;
  _inBufferSize = SRC_IN_BUFFER_FRAMES * _channels;
  _inbuffer = new float[_inBufferSize];

  int srcerr;
  DEBUG_AUDIOCONVERT(stderr, "SRCAudioConverter::setChannels Creating samplerate converter type:%d with %d channels\n", _type, ch);
  _src_state = src_new(_type, ch, &srcerr);  
  if(!_src_state)
  {
    ERROR_AUDIOCONVERT(stderr, "SRCAudioConverter::setChannels of samplerate converter type:%d with %d channels failed:%s\n", _type, ch, src_strerror(srcerr));
  }
  return;  
}

void SRCAudioConverter::reset()
{
  resetSrcData();
  if(!_src_state)
    return;
  DEBUG_AUDIOCONVERT(stderr, "SRCAudioConverter::reset this:%p\n", this);
  int srcerr = src_reset(_src_state);
  if(srcerr != 0)
  {
    ERROR_AUDIOCONVERT(stderr, "SRCAudioConverter::reset Converter reset failed: %s\n", src_strerror(srcerr));
  }
  return;  
}

void SRCAudioConverter::resetSrcData()
{
  _curInBufferFrame = 0;
  _needBuffer = true;
}

AudioConverterSettings::ModeType SRCAudioConverter::mode() const
{ 
  return _mode;
}

int SRCAudioConverter::process(
  SNDFILE* sf_handle,
  const int sf_chans, const double sf_sr_ratio, const StretchList* sf_stretch_list,
  const sf_count_t pos,
  float** buffer, const int channels, const int frames, const bool overwrite)
{
  if(!_src_state)
    return 0;
  
  if(_systemSampleRate <= 0)
  {  
    DEBUG_AUDIOCONVERT(stderr, "SRCAudioConverter::process Error: systemSampleRate <= 0!\n");
    //return _sfCurFrame;
    return 0;
  }  

  const double d_pos = sf_sr_ratio * double(pos);
  const MuseFrame_t unsquished_pos = sf_stretch_list->unSquish(d_pos);

  sf_count_t outSize    = frames * sf_chans;
  
  // Start with buffers at expected sizes.
  float outbuffer[outSize];
      
  sf_count_t totalOutFrames = 0;
  
  _srcdata.data_out      = outbuffer;

  const double debug_min_pos = 120;
  bool need_slice = true;

  double cur_ratio = 1.0;
  ciStretchListItem next_isli = sf_stretch_list->upper_bound(unsquished_pos);

//   fprintf(stderr, "SRCAudioConverter::process pos:%ld unsquished_pos:%ld\n",
//                      pos, unsquished_pos);

  if(next_isli != sf_stretch_list->begin())
  {
    --next_isli;
    cur_ratio = next_isli->second._samplerateRatio;
  }
  if(next_isli != sf_stretch_list->end())
    next_isli = sf_stretch_list->cNextEvent(StretchListItem::SamplerateEvent, next_isli);
  double fin_samplerateRatio, inv_fin_samplerateRatio;

  double d_cur_squished_pos;
  const double d_end_squished_pos = d_pos + frames;

  sf_count_t outFrames = frames - totalOutFrames;
  // Set some kind of limit on the number of attempts to completely fill the output buffer,
  //  in case something is really screwed up - we don't want to get stuck in a loop here.
  int attempts = 20;
  if(d_pos <= debug_min_pos)
  {
    DEBUG_AUDIOCONVERT(stderr, "SRCAudioConverter::process d_pos:%f d_end_pos:%f unsquished_pos:%ld\n",
                       d_pos, d_end_squished_pos, unsquished_pos);
  }
  while(totalOutFrames < frames && attempts > 0)
  {
    d_cur_squished_pos = d_pos + totalOutFrames;

    if(d_pos <= debug_min_pos)
    {
      DEBUG_AUDIOCONVERT(stderr, "   d_cur_squished_pos:%ld outFrames:%ld totalOutFrames:%ld\n",
                         d_cur_squished_pos, outFrames, totalOutFrames);
    }

    if(_needBuffer)
    {
      _srcdata.input_frames = sf_readf_float(sf_handle, _inbuffer, SRC_IN_BUFFER_FRAMES);
      _srcdata.end_of_input = _srcdata.input_frames != SRC_IN_BUFFER_FRAMES;
      // Zero any unread portion of the input buffer.
      for(int i = _srcdata.input_frames * sf_chans; i < _inBufferSize; ++i)
        *(_inbuffer + i) = 0.0f;
      _needBuffer = false;
    }
    else
    {
      _srcdata.input_frames = SRC_IN_BUFFER_FRAMES - _curInBufferFrame;
    }
    _srcdata.data_in = _inbuffer + sf_chans * _curInBufferFrame;



    if(need_slice)
    {
      if(d_pos <= debug_min_pos)
      {
        DEBUG_AUDIOCONVERT(stderr, "   new slice: cur_ratio:%f\n", cur_ratio);
      }

      fin_samplerateRatio = sf_sr_ratio * cur_ratio;
      if(fin_samplerateRatio < 0.000001)
      {
        DEBUG_AUDIOCONVERT(stderr, "SRCAudioConverter::process Error: fin_samplerateRatio ratio is near zero!\n");
        fin_samplerateRatio = 0.000001;
      }
      inv_fin_samplerateRatio = 1.0 / fin_samplerateRatio;

      if(next_isli != sf_stretch_list->end() &&
         next_isli->second._finSquishedFrame >= d_cur_squished_pos &&
         next_isli->second._finSquishedFrame < d_end_squished_pos)
      {
        cur_ratio = next_isli->second._samplerateRatio;
        outFrames = next_isli->second._finSquishedFrame - d_cur_squished_pos;
        if(d_pos <= debug_min_pos)
        {
          DEBUG_AUDIOCONVERT(stderr, "   new next_isli: next cur_ratio:%f outFrames:%ld\n", cur_ratio, outFrames);
        }
        next_isli = sf_stretch_list->cNextEvent(StretchListItem::SamplerateEvent, next_isli);
      }
      else
        outFrames = frames - totalOutFrames;

      need_slice = false;
    }
    // "When using the src_process or src_callback_process APIs and
    //   updating the src_ratio field of the SRC_STATE struct,
    //   the library will try to smoothly transition between the
    //   conversion ratio of the last call and the conversion ratio of the current call.
    //  If the user want to bypass this smooth transition and achieve a step response
    //   in the conversion ratio, the src_set_ratio function can be used to set the
    //   starting conversion ratio of the next call to src_process or src_callback_process."
    _srcdata.src_ratio = inv_fin_samplerateRatio;
    //_srcdata.src_ratio = 1.0; // Set it to neutral I guess?
    src_set_ratio(_src_state, inv_fin_samplerateRatio);
    _srcdata.output_frames = outFrames;
    _srcdata.data_out      = outbuffer + sf_chans * totalOutFrames;

    DEBUG_AUDIOCONVERT(stderr, "SRCAudioConverter::process Calling src_process inv_fin_samplerateRatio:%f _curInBufferFrame:%d outFrames:%ld totalOutFrames:%ld\n",
                       inv_fin_samplerateRatio, _curInBufferFrame, outFrames, totalOutFrames);

    int srcerr = src_process(_src_state, &_srcdata);
    if(srcerr != 0)
    {
      ERROR_AUDIOCONVERT(stderr, "SRCAudioConverter::process SampleRate converter process failed: %s\n", src_strerror(srcerr));
      return 0;
    }

    DEBUG_AUDIOCONVERT(stderr, "   input_frames_used:%ld output_frames_gen:%ld\n\n",
                       _srcdata.input_frames_used, _srcdata.output_frames_gen);


    // Note that if SRC is fed more than it needs, the next process call(s)
    //  will generate output but will report NO frames used!
    // If some input frames were used:
    if(_srcdata.input_frames_used > 0)
    {
      // Advance the current input frame.
      _curInBufferFrame += _srcdata.input_frames_used;
      // Rollover?
      if(_curInBufferFrame >= SRC_IN_BUFFER_FRAMES)
      {
        // Reset the counter.
        _curInBufferFrame = 0;
        // Request a new buffer.
        _needBuffer = true;
      }
    }

    totalOutFrames += _srcdata.output_frames_gen;

    // If we got the number of slice frames, ask for a new slice.
    if(_srcdata.output_frames_gen >= outFrames)
      need_slice = true;
    else
      outFrames -= _srcdata.output_frames_gen;

    if(_srcdata.input_frames == 0)
      break;

    // Countdown the attempts only if no output was generated. If at least something was generated allow it to keep trying indefinitely.
    if(_srcdata.output_frames_gen == 0)
      --attempts;
  }

  if(attempts == 0)
  {
    ERROR_AUDIOCONVERT(stderr,
      "SRCAudioConverter::process handle:%p stretch list:%p Too may attempts to process! totalOutFrames:%ld frames:%d\n",
      sf_handle, sf_stretch_list, totalOutFrames, frames);
  }

  // If we didn't get the desired number of output frames.
  if(totalOutFrames != frames)
  {
    DEBUG_AUDIOCONVERT(stderr, "SRCAudioConverter::process %s totalOutFrames:%ld != frames:%d\n",
                       sf->name().toLatin1().constData(), totalOutFrames, frames);

    // Let's zero the rest of it.
    sf_count_t b = totalOutFrames * channels;
    sf_count_t e = frames * channels;
    for(sf_count_t i = b; i < e; ++i)
      outbuffer[i] = 0.0f;
  }

  float*  poutbuf = outbuffer;
  if(sf_chans == channels)
  {
    if(overwrite)
      for (sf_count_t i = 0; i < frames; ++i)
      {
        for(sf_count_t ch = 0; ch < channels; ++ch)
          *(buffer[ch] + i) = *poutbuf++;
      }
    else
      for(sf_count_t i = 0; i < frames; ++i)
      {
        for(sf_count_t ch = 0; ch < channels; ++ch)
          *(buffer[ch] + i) += *poutbuf++;
      }
  }
  else if((sf_chans == 2) && (channels == 1))
  {
    // stereo to mono
    if(overwrite)
      for(sf_count_t i = 0; i < frames; ++i)
        *(buffer[0] + i) = poutbuf[i + i] + poutbuf[i + i + 1];
    else  
      for(sf_count_t i = 0; i < frames; ++i)
        *(buffer[0] + i) += poutbuf[i + i] + poutbuf[i + i + 1];
  }
  else if((sf_chans == 1) && (channels == 2))
  {
    // mono to stereo
    if(overwrite)
      for(sf_count_t i = 0; i < frames; ++i)
      {
        float data = *poutbuf++;
        *(buffer[0]+i) = data;
        *(buffer[1]+i) = data;
      }
    else  
      for(sf_count_t i = 0; i < frames; ++i)
      {
        float data = *poutbuf++;
        *(buffer[0]+i) += data;
        *(buffer[1]+i) += data;
      }
  }
  else 
  {
    DEBUG_AUDIOCONVERT(stderr, "SRCAudioConverter::process Channel mismatch: source chans:%d -> dst chans:%d\n",
                       sf_chans, channels);
  }
  
  //return _sfCurFrame;
  return frames;
}

//---------------------------------------------------------
//   SRCAudioConverterSettings
//---------------------------------------------------------

void SRCAudioConverterOptions::write(int level, Xml& xml) const
      {
      xml.tag(level++, "settings mode=\"%d\"", _mode);
      
      xml.intTag(level, "useSettings", _useSettings);
      xml.intTag(level, "converterType", _converterType);
      
      xml.tag(--level, "/settings");
      
      }

void SRCAudioConverterOptions::read(Xml& xml)
      {
      for (;;) {
            Xml::Token token = xml.parse();
            const QString& tag = xml.s1();
            switch (token) {
                  case Xml::Error:
                  case Xml::End:
                        return;
                  case Xml::TagStart:
                        if (tag == "useSettings")
                              _useSettings = xml.parseInt();
                        else 
                          if (tag == "converterType")
                              _converterType = xml.parseInt();
                        else
                              xml.unknown("settings");
                        break;
                  case Xml::Attribut:
                              fprintf(stderr, "settings unknown tag %s\n", tag.toLatin1().constData());
                        break;
                  case Xml::TagEnd:
                        if (tag == "settings") {
                              return;
                              }
                  default:
                        break;
                  }
            }
      }


//---------------------------------------------------------
//   SRCAudioConverterSettings
//---------------------------------------------------------

// Some hard-coded defaults.
const SRCAudioConverterOptions SRCAudioConverterOptions::defaultOfflineOptions(
  false, AudioConverterSettings::OfflineMode, SRC_SINC_BEST_QUALITY);
const SRCAudioConverterOptions SRCAudioConverterOptions::defaultRealtimeOptions(
  false, AudioConverterSettings::RealtimeMode, SRC_SINC_MEDIUM_QUALITY);
const SRCAudioConverterOptions SRCAudioConverterOptions::defaultGuiOptions(
  false, AudioConverterSettings::GuiMode, SRC_SINC_FASTEST);
    
SRCAudioConverterSettings::SRCAudioConverterSettings(
  bool isLocal) 
  : AudioConverterSettings(descriptor._ID)
{ 
  initOptions(isLocal); 
}
      
void SRCAudioConverterSettings::assign(const AudioConverterSettings& other)
{
  const SRCAudioConverterSettings& src_other = 
    (const SRCAudioConverterSettings&)other;
  _offlineOptions  = src_other._offlineOptions;
  _realtimeOptions = src_other._realtimeOptions;
  _guiOptions      = src_other._guiOptions;
}

bool SRCAudioConverterSettings::useSettings(int mode) const 
{ 
  if(mode > 0 &&
     (mode & ~(AudioConverterSettings::OfflineMode |
              AudioConverterSettings::RealtimeMode | 
              AudioConverterSettings::GuiMode)))
    fprintf(stderr, "SRCAudioConverterSettings::useSettings() Warning: Unknown modes included:%d\n", mode);
  
  if((mode <= 0 || (mode & AudioConverterSettings::OfflineMode)) && _offlineOptions.useSettings())
    return true;

  if((mode <= 0 || (mode & AudioConverterSettings::RealtimeMode)) && _realtimeOptions.useSettings())
    return true;

  if((mode <= 0 || (mode & AudioConverterSettings::GuiMode)) && _guiOptions.useSettings())
    return true;
    
  return false;
}

int SRCAudioConverterSettings::executeUI(ModeType mode, QWidget* parent, bool isLocal) 
{
  MusEGui::SRCResamplerSettingsDialog dlg(mode, parent, this, isLocal);
  return dlg.exec(); 
}

void SRCAudioConverterSettings::write(int level, Xml& xml) const
{
  const bool use_off = !(_offlineOptions == SRCAudioConverterOptions::defaultOfflineOptions);
  const bool use_rt  = !(_realtimeOptions == SRCAudioConverterOptions::defaultRealtimeOptions);
  const bool use_gui = !(_guiOptions == SRCAudioConverterOptions::defaultGuiOptions);

  if(use_off | use_rt || use_gui)
  {
    xml.tag(level++, "audioConverterSetting name=\"%s\"", Xml::xmlString(descriptor._name).toLatin1().constData());
    
    if(use_off)
    {
      _offlineOptions.write(level, xml);
    }
    
    if(use_rt)
    {
      _realtimeOptions.write(level, xml);
    }
    
    if(use_gui)
    {
      _guiOptions.write(level, xml);
    }
    
    xml.tag(--level, "/audioConverterSetting");
  }
}

void SRCAudioConverterSettings::read(Xml& xml)
{
  int mode = -1;
  for (;;) {
      Xml::Token token = xml.parse();
      const QString& tag = xml.s1();
      switch (token) {
            case Xml::Error:
            case Xml::End:
                  return;
            case Xml::TagStart:
                  if(mode != -1)
                  {
                    SRCAudioConverterOptions* opts = NULL;
                    switch(mode)
                    {
                      case AudioConverterSettings::OfflineMode:
                        opts = &_offlineOptions;
                      break;

                      case AudioConverterSettings::RealtimeMode:
                        opts = &_realtimeOptions;
                      break;
                      
                      case AudioConverterSettings::GuiMode:
                        opts = &_guiOptions;
                      break;
                    }
                    
                    if(opts)
                    {
                      if(tag == "useSettings")
                        opts->_useSettings = xml.parseInt();
                      else 
                        if(tag == "converterType")
                        opts->_converterType = xml.parseInt();
                    }
                  }
                  
                  else
                        xml.unknown("settings");
                  break;
            case Xml::Attribut:
                    if (tag == "mode")
                      mode = xml.s2().toInt();
                  else
                      fprintf(stderr, "settings unknown tag %s\n", tag.toLatin1().constData());
                  break;
            case Xml::TagEnd:
                  if (tag == "settings") {
                        return;
                        }
            default:
                  break;
            }
      }
}


} // namespace MusECore


//==========================================================


namespace MusEGui {
  
SRCResamplerSettingsDialog::SRCResamplerSettingsDialog(
  MusECore::AudioConverterSettings::ModeType mode,
  QWidget* parent, 
  MusECore::AudioConverterSettings* settings, 
  bool isLocal)
  : QDialog(parent)
{
  setupUi(this);

  OKButton->setEnabled(false);
  
  _options = NULL;
  if(settings)
  {
    MusECore::SRCAudioConverterSettings* src_settings = 
      static_cast<MusECore::SRCAudioConverterSettings*>(settings);
    
    switch(mode)
    {
      case MusECore::AudioConverterSettings::OfflineMode:
        _options = src_settings->offlineOptions();
      break;

      case MusECore::AudioConverterSettings::RealtimeMode:
        _options = src_settings->realtimeOptions();
      break;

      case MusECore::AudioConverterSettings::GuiMode:
        _options = src_settings->guiOptions();
      break;
      
      default:
        // Disable everything and return.
      break;
    }
  }

  //if(isLocal)
    useDefaultSettings->setChecked(!_options || !_options->_useSettings);
  useDefaultSettings->setEnabled(isLocal && _options);
  useDefaultSettings->setVisible(isLocal && _options);
  typeGroup->setEnabled(!isLocal || (_options && _options->_useSettings));
  setControls();

  connect(typeSINCBestQuality, &QRadioButton::clicked, [this]() { buttonClicked(ConverterButtonId); } );
  connect(typeSINCMedium, &QRadioButton::clicked, [this]() { buttonClicked(ConverterButtonId); } );
  connect(typeSINCFastest, &QRadioButton::clicked, [this]() { buttonClicked(ConverterButtonId); } );
  connect(typeZeroOrderHold, &QRadioButton::clicked, [this]() { buttonClicked(ConverterButtonId); } );
  connect(typeLinear, &QRadioButton::clicked, [this]() { buttonClicked(ConverterButtonId); } );
  connect(useDefaultSettings, &QCheckBox::clicked, [this]() { buttonClicked(DefaultsButtonId); } );
  connect(OKButton, &QPushButton::clicked, [this]() { buttonClicked(OkButtonId); } );
  connect(cancelButton, &QPushButton::clicked, [this]() { buttonClicked(CancelButtonId); } );
}
  
void SRCResamplerSettingsDialog::setControls()
{
  if(!_options)
    return;

  switch(_options->_converterType)
  {
    case SRC_SINC_BEST_QUALITY:
      typeSINCBestQuality->blockSignals(true);
      typeSINCBestQuality->setChecked(true);
      typeSINCBestQuality->blockSignals(false);
    break;
    
    case SRC_SINC_MEDIUM_QUALITY:
      typeSINCMedium->blockSignals(true);
      typeSINCMedium->setChecked(true);
      typeSINCMedium->blockSignals(false);
    break;
    
    case SRC_SINC_FASTEST:
      typeSINCFastest->blockSignals(true);
      typeSINCFastest->setChecked(true);
      typeSINCFastest->blockSignals(false);
    break;
    
    case SRC_ZERO_ORDER_HOLD:
      typeZeroOrderHold->blockSignals(true);
      typeZeroOrderHold->setChecked(true);
      typeZeroOrderHold->blockSignals(false);
    break;
    
    case SRC_LINEAR:
      typeLinear->blockSignals(true);
      typeLinear->setChecked(true);
      typeLinear->blockSignals(false);
    break;
   
    default:
    break;
  }
}

void SRCResamplerSettingsDialog::buttonClicked(int idx)
{
  switch(idx)
  {
    case DefaultsButtonId:
      OKButton->setEnabled(true);
      typeGroup->setEnabled(!useDefaultSettings->isChecked());
    break;
    
    case ConverterButtonId:
      OKButton->setEnabled(true);
    break;
    
    case OkButtonId:
      accept();
    break;
    
    case CancelButtonId:
      reject();
    break;
    
    default:
    break;
  }
}

//---------------------------------------------------------
//   accept
//---------------------------------------------------------

void SRCResamplerSettingsDialog::accept()
{
  if(!_options)
  {
    QDialog::accept();
    return;
  }
  int type = -1;
  if(typeSINCBestQuality->isChecked())
    type = SRC_SINC_BEST_QUALITY;
  else if(typeSINCMedium->isChecked())
    type = SRC_SINC_MEDIUM_QUALITY;
  else if(typeSINCFastest->isChecked())
    type = SRC_SINC_FASTEST;
  else if(typeZeroOrderHold->isChecked())
    type = SRC_ZERO_ORDER_HOLD;
  else if(typeLinear->isChecked())
    type = SRC_LINEAR;
  
  if(type != -1)
    _options->_converterType = type;
    
  _options->_useSettings = !useDefaultSettings->isChecked();
  
  QDialog::accept();
}

} // namespace MusEGui
