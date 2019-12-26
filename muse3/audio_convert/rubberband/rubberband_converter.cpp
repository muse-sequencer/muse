//=========================================================
//  MusE
//  Linux Music Editor
//
//  rubberband_converter.cpp
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
#include <QList>

#include <stdio.h>

#include "muse_math.h"
#include "time_stretch.h"
#include "rubberband_converter.h"

// For debugging output: Uncomment the fprintf section.
#define ERROR_AUDIOCONVERT(dev, format, args...) fprintf(dev, format, ##args)
#define DEBUG_AUDIOCONVERT(dev, format, args...) // fprintf(dev, format, ##args)


// Create a new instance of the plugin.  
// Mode is an AudioConverterSettings::ModeType selecting which of the settings to use.
MusECore::AudioConverter* instantiate(int systemSampleRate,
                            const MusECore::AudioConverterDescriptor* /*Descriptor*/,
                            int channels, 
                            MusECore::AudioConverterSettings* settings, 
                            int mode)
{
  return new MusECore::RubberBandAudioConverter(systemSampleRate, channels, settings, mode);
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
  return new MusECore::RubberBandAudioConverterSettings(isLocal);
}


extern "C" 
{
  static MusECore::AudioConverterDescriptor descriptor = {
    1003,
    MusECore::AudioConverter::SampleRate | MusECore::AudioConverter::Stretch | MusECore::AudioConverter::Pitch,
    "Rubberband Stretcher",
    "Rubberband",
    -1,
    0.0,
    -1.0,
    0.0,
    -1.0,
    0.0,
    -1.0,
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

//=================================================================

namespace MusECore {

//---------------------------------------------------------
//   RubberBandAudioConverter
//---------------------------------------------------------

RubberBandAudioConverter::RubberBandAudioConverter(int systemSampleRate,
                                                   int channels, 
                                                   AudioConverterSettings* settings, 
                                                   int mode) : AudioConverter(systemSampleRate)
{
  DEBUG_AUDIOCONVERT(stderr, "RubberBandAudioConverter::RubberBandAudioConverter this:%p channels:%d mode:%d\n", 
                     this, channels, mode);

#ifdef RUBBERBAND_SUPPORT
  const int mask      = ~(RubberBand::RubberBandStretcher::OptionProcessOffline | RubberBand::RubberBandStretcher::OptionProcessRealTime);
  const int of_flags  = RubberBand::RubberBandStretcher::OptionProcessOffline;
  const int rt_flags  = RubberBand::RubberBandStretcher::OptionProcessRealTime;
  const int gui_flags = RubberBand::RubberBandStretcher::OptionProcessRealTime;
#else
  const int mask      = ~0;
  const int of_flags  = 0;
  const int rt_flags  = 0;
  const int gui_flags = 0;
#endif
  
  RubberBandAudioConverterSettings* rb_settings = static_cast<RubberBandAudioConverterSettings*>(settings);
  switch(mode)
  {
    case AudioConverterSettings::OfflineMode:
      _options = ((rb_settings ? rb_settings->offlineOptions()->_options : 0) & mask) | of_flags;
    break;
    
    case AudioConverterSettings::RealtimeMode:
      _options = ((rb_settings ? rb_settings->realtimeOptions()->_options : 0) & mask) | rt_flags;
    break;
    
    case AudioConverterSettings::GuiMode:
      _options = ((rb_settings ? rb_settings->guiOptions()->_options : 0) & mask) | gui_flags;
    break;
  
    default:
      _options = 0;
    break;
  }
  
  _channels = channels;
// REMOVE Tim. samplerate. Changed. 12/19 Removed. TESTING Reinstate.
//   _latencyCompPending = true; // Set to compensate at the first process.
  _latencyCompPending = false; // Set to compensate at the first process.
  
#ifdef RUBBERBAND_SUPPORT
  _rbs = new RubberBand::RubberBandStretcher(_systemSampleRate, _channels, _options);
#endif
}

RubberBandAudioConverter::~RubberBandAudioConverter()
{
  DEBUG_AUDIOCONVERT(stderr, "RubberBandAudioConverter::~RubberBandAudioConverter this:%p\n", this);
#ifdef RUBBERBAND_SUPPORT
  if(_rbs)
    delete _rbs;
#endif
}

void RubberBandAudioConverter::setChannels(int ch)
{
  DEBUG_AUDIOCONVERT(stderr, "RubberBandAudioConverter::setChannels this:%p channels:%d\n", this, ch);
  _channels = ch;

#ifdef RUBBERBAND_SUPPORT
  if(_rbs)
    delete _rbs;
  _rbs = new RubberBand::RubberBandStretcher(_systemSampleRate, _channels, _options);
#endif
}

void RubberBandAudioConverter::reset()
{
  DEBUG_AUDIOCONVERT(stderr, "RubberBandAudioConverter::reset this:%p\n", this);
#ifdef RUBBERBAND_SUPPORT
  if(!_rbs)
    return;
  _rbs->reset();
// REMOVE Tim. samplerate. Changed. 12/19 Removed. TESTING Reinstate.
//   _latencyCompPending = true;
  return;  
#endif
}

#ifdef RUBBERBAND_SUPPORT
int RubberBandAudioConverter::process(
  SNDFILE* sf_handle,
  const int sf_chans, const double sf_sr_ratio, const StretchList* sf_stretch_list,
  const sf_count_t pos,
  float** buffer, const int channels, const int frames, const bool overwrite)
{
  if(!_rbs)
    return 0;
  
  if(_systemSampleRate <= 0)
  {  
    DEBUG_AUDIOCONVERT(stderr, "RubberBandAudioConverter::process Error: systemSampleRate <= 0!\n");
    //return _sfCurFrame;
    return 0;
  }  

  const double d_pos = sf_sr_ratio * double(pos);
  const MuseFrame_t new_frame = sf_stretch_list->unSquish(d_pos);
  
  const double stretchVal    = sf_stretch_list->ratioAt(StretchListItem::StretchEvent, new_frame);
  const double samplerateVal = sf_stretch_list->ratioAt(StretchListItem::SamplerateEvent, new_frame);
  
  const double fin_samplerateRatio = sf_sr_ratio * samplerateVal;
  
  if(fin_samplerateRatio < 0.0001)
  {  
    DEBUG_AUDIOCONVERT(stderr, "RubberBandAudioConverter::process Error: fin_samplerateRatio ratio is near zero!\n");
    //return _sfCurFrame;
    return 0;
  }  
  
  const double inv_fin_samplerateRatio = 1.0 / fin_samplerateRatio;
  const double debug_min_pos = 256;

  int old_lat = _rbs->getLatency();
  // For just sample rate conversion, apply complimentary ratio to time and pitch.
  _rbs->setTimeRatio(inv_fin_samplerateRatio * stretchVal);
  _rbs->setPitchScale(fin_samplerateRatio);

  sf_count_t outFrames  = frames;
  if(_latencyCompPending)
    outFrames += old_lat;
  sf_count_t outSize    = outFrames * sf_chans;
  float* rboutbuffer[sf_chans];
  float rboutdata[outSize];
  for(int i = 0; i < sf_chans; ++i)
    rboutbuffer[i] = rboutdata + i * outFrames;
      
  sf_count_t rn           = 0;
  
  // Tested: Latency will change after the above setting of ratios. Also affected by converter options.
  //DEBUG_AUDIOCONVERT(stderr, "RubberBandAudioConverter::process latency:%lu\n", _rbs->getLatency());

  if(_latencyCompPending && old_lat > 0)
  {
    size_t rbinSize = old_lat * sf_chans;
    float rbindata[rbinSize];
    memset(rbindata, 0, rbinSize * sizeof(float));
    // Must de-interleave data to feed to rubberband.
    float* rbinbuffer[sf_chans];
    for(int i = 0; i < sf_chans; ++i)
      rbinbuffer[i] = rbindata + i * old_lat;
    if(d_pos <= debug_min_pos)
    {
      DEBUG_AUDIOCONVERT(stderr, "   Latency: old_lat:%d new_lat:%d\n", old_lat, (int)_rbs->getLatency());
    }
    _rbs->process(rbinbuffer, old_lat, false);
  }

  while(_rbs->available() < outFrames)
  {
    size_t sreq = _rbs->getSamplesRequired();
    if(sreq <= 0)
        break;
    
    size_t rbinSize = sreq * sf_chans;
    float sfdata[rbinSize];
    rn = sf_readf_float(sf_handle,  sfdata, sreq);
    // Zero any buffer portion not filled by the file read. TODO: De-normals required here?
    if((size_t)rn != sreq)
    {
      const size_t zeros = (sreq - rn) * sf_chans;
      const size_t zero_start = rn * sf_chans;
      memset(&sfdata[zero_start], 0, zeros * sizeof(float));
    }
    
    // Must de-interleave soundfile data to feed to rubberband.
    // TODO: Optimizations! Would like fast in-place de-interleaving, but it's "harder than it looks". 
    //       There are even patents on this kind of thing!
    //       If optimized, be sure to move it to our AL-DSP (assembly) library. With SSE etc, it's easier. 
    //       Alas, for now use out-of-place memory since stack *should* be cheap here without much nesting.
    float rbindata[rbinSize];
    float* rbinbuffer[sf_chans];
    for(int i = 0; i < sf_chans; ++i)
      rbinbuffer[i] = rbindata + i * sreq;
    float* sfptr = sfdata;
    for(size_t i = 0; i < sreq; ++i) 
    {
      for(int ch = 0; ch < sf_chans; ++ch)
        *(rbinbuffer[ch] + i) = *sfptr++;
    }
    if(d_pos <= debug_min_pos)
    {
      DEBUG_AUDIOCONVERT(stderr, "   Normal: required:%d avail:%d\n", int(sreq), _rbs->available());
    }
    _rbs->process(rbinbuffer, sreq, false);
  }
  
  int savail = _rbs->available();
  if(d_pos <= debug_min_pos)
  {
    DEBUG_AUDIOCONVERT(stderr, "   Normal: final avail:%d\n", _rbs->available());
  }
  if(savail > outFrames)
    savail = outFrames;
  
  if(_latencyCompPending && old_lat > 0)
  {
    size_t retrieved = _rbs->retrieve(rboutbuffer, old_lat);
    if((int)retrieved < old_lat)
    { 
      DEBUG_AUDIOCONVERT(stderr,
      "RubberBandAudioConverter::process: Latency retrieved_count:%d is less than requested:%d\n",
      int(retrieved), old_lat);
    }
    savail -= old_lat;
  }
  _latencyCompPending = false;

  // Retrieves de-interleaved data.
  size_t retrieved = _rbs->retrieve(rboutbuffer, savail);
  if((int)retrieved < savail)
  {
    DEBUG_AUDIOCONVERT(stderr, "RubberBandAudioConverter::process: retrieved_count:%d is less than savail:%d\n",
                       int(retrieved), savail);
    savail = retrieved;
  }
  
  if(savail < frames)
  {
    DEBUG_AUDIOCONVERT(stderr,
      "RubberBandAudioConverter::process: savail:%d is less than needed frames:%d. Converter is finished ???\n",
      savail, frames);
    // We didn't get the total required frames. Zero the rest.
    const size_t zeros = frames - savail;
    for(int i = 0; i < sf_chans; ++i)
      memset(rboutbuffer[i] + savail, 0, zeros * sizeof(float));
  }
  
  if(sf_chans == channels)
  {
    if(overwrite)
      for(int ch = 0; ch < channels; ++ch)
        for(int i = 0; i < frames; ++i) 
          *(buffer[ch] + i) = *(rboutbuffer[ch] + i);
    else
      for(int ch = 0; ch < channels; ++ch)
        for(int i = 0; i < frames; ++i) 
          *(buffer[ch] + i) += *(rboutbuffer[ch] + i);
  }
  else if((sf_chans == 2) && (channels == 1)) 
  {
    // stereo to mono
    if(overwrite)
      for(int i = 0; i < frames; ++i)
        *(buffer[0] + i) = *(rboutbuffer[0] + i) + *(rboutbuffer[1] + i);
    else  
      for(int i = 0; i < frames; ++i)
        *(buffer[0] + i) += *(rboutbuffer[0] + i) + *(rboutbuffer[1] + i);
  }
  else if((sf_chans == 1) && (channels == 2)) 
  {
    // mono to stereo
    float data;
    if(overwrite)
      for(int i = 0; i < frames; ++i) 
      {
        data = *(rboutbuffer[0] + i);
        *(buffer[0]+i) = data;
        *(buffer[1]+i) = data;
      }
    else  
      for(int i = 0; i < frames; ++i) 
      {
        data = *(rboutbuffer[0] + i);
        *(buffer[0]+i) += data;
        *(buffer[1]+i) += data;
      }
  }
  else 
  {
    DEBUG_AUDIOCONVERT(stderr, "RubberBandAudioConverter::process Channel mismatch: source chans:%d -> dst chans:%d\n",
                       sf_chans, channels);
  }
  
  return frames;
}

#else // RUBBERBAND_SUPPORT

int RubberBandAudioConverter::process(
  SNDFILE* /*sf_handle*/,
  const int /*sf_chans*/, const double /*sf_sr_ratio*/, const StretchList* /*sf_stretch_list*/,
  const sf_count_t /*pos*/,
  float** /*buffer*/, const int /*channels*/, const int /*frames*/, const bool /*overwrite*/)
{
  return 0;
}

#endif // RUBBERBAND_SUPPORT


//---------------------------------------------------------
//   RubberBandAudioConverterSettings
//---------------------------------------------------------

void RubberBandAudioConverterOptions::write(int level, Xml& xml) const
      {
      xml.tag(level++, "settings mode=\"%d\"", _mode);
      
      xml.intTag(level, "useSettings", _useSettings);
      xml.intTag(level, "options", _options);
      
      xml.tag(--level, "/settings");
      
      }

void RubberBandAudioConverterOptions::read(Xml& xml)
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
                        else if (tag == "options")
                              _options = xml.parseInt();
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
//   RubberBandAudioConverterSettings
//---------------------------------------------------------

// Some hard-coded defaults.
#ifdef RUBBERBAND_SUPPORT
const RubberBandAudioConverterOptions RubberBandAudioConverterOptions::
      defaultOfflineOptions(false, AudioConverterSettings::OfflineMode, RubberBand::RubberBandStretcher::DefaultOptions);
const RubberBandAudioConverterOptions RubberBandAudioConverterOptions::
      defaultRealtimeOptions(false, AudioConverterSettings::RealtimeMode, RubberBand::RubberBandStretcher::DefaultOptions);
const RubberBandAudioConverterOptions RubberBandAudioConverterOptions::
      defaultGuiOptions(false, AudioConverterSettings::GuiMode, RubberBand::RubberBandStretcher::DefaultOptions);
#else
const RubberBandAudioConverterOptions RubberBandAudioConverterOptions::
      defaultOfflineOptions(false, AudioConverterSettings::OfflineMode, 0);
const RubberBandAudioConverterOptions RubberBandAudioConverterOptions::
      defaultRealtimeOptions(false, AudioConverterSettings::RealtimeMode, 0);
const RubberBandAudioConverterOptions RubberBandAudioConverterOptions::
      defaultGuiOptions(false, AudioConverterSettings::GuiMode, 0);
#endif

RubberBandAudioConverterSettings::RubberBandAudioConverterSettings(
  bool isLocal) 
  : AudioConverterSettings(descriptor._ID)
{ 
  initOptions(isLocal); 
}
      
void RubberBandAudioConverterSettings::assign(const AudioConverterSettings& other)
{
  const RubberBandAudioConverterSettings& rb_other = 
    (const RubberBandAudioConverterSettings&)other;
  _offlineOptions  = rb_other._offlineOptions;
  _realtimeOptions = rb_other._realtimeOptions;
  _guiOptions      = rb_other._guiOptions;
}

bool RubberBandAudioConverterSettings::useSettings(int mode) const 
{ 
  if(mode > 0 && 
     (mode & ~(AudioConverterSettings::OfflineMode | 
               AudioConverterSettings::RealtimeMode | 
               AudioConverterSettings::GuiMode)))
    fprintf(stderr, "RubberBandAudioConverterSettings::useSettings() Warning: Unknown modes included:%d\n", mode);
  
  if((mode <= 0 || (mode & AudioConverterSettings::OfflineMode)) && _offlineOptions.useSettings())
    return true;

  if((mode <= 0 || (mode & AudioConverterSettings::RealtimeMode)) && _realtimeOptions.useSettings())
    return true;

  if((mode <= 0 || (mode & AudioConverterSettings::GuiMode)) && _guiOptions.useSettings())
    return true;
    
  return false;
}

int RubberBandAudioConverterSettings::executeUI(int mode, QWidget* parent, bool isLocal) 
{
  MusEGui::RubberbandSettingsDialog dlg(mode, parent, this, isLocal);
  return dlg.exec(); 
}

void RubberBandAudioConverterSettings::write(int level, Xml& xml) const
{
  const bool use_off = !(_offlineOptions == RubberBandAudioConverterOptions::defaultOfflineOptions);
  const bool use_rt  = !(_realtimeOptions == RubberBandAudioConverterOptions::defaultRealtimeOptions);
  const bool use_gui = !(_guiOptions == RubberBandAudioConverterOptions::defaultGuiOptions);

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

void RubberBandAudioConverterSettings::read(Xml& xml)
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
                    RubberBandAudioConverterOptions* opts = NULL;
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
                        if(tag == "options")
                        opts->_options = xml.parseInt();
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


//=====================================================================


namespace MusEGui {
  
RubberbandSettingsDialog::RubberbandSettingsDialog(
  int mode,
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
    MusECore::RubberBandAudioConverterSettings* rb_settings = 
      static_cast<MusECore::RubberBandAudioConverterSettings*>(settings);
    
    switch(mode)
    {
      case MusECore::AudioConverterSettings::OfflineMode:
        _options = rb_settings->offlineOptions();
      break;

      case MusECore::AudioConverterSettings::RealtimeMode:
        _options = rb_settings->realtimeOptions();
      break;

      case MusECore::AudioConverterSettings::GuiMode:
        _options = rb_settings->guiOptions();
      break;
      
      default:
        // Disable everything and return.
      break;
    }
  }

#ifdef RUBBERBAND_SUPPORT
  //if(isLocal)
    useDefaultSettings->setChecked(!_options || !_options->_useSettings);
  useDefaultSettings->setEnabled(isLocal && _options);
  useDefaultSettings->setVisible(isLocal && _options);
  groupScrollArea->setEnabled(!isLocal || (_options && _options->_useSettings));
  presetsGroup->setEnabled(!isLocal || (_options && _options->_useSettings));
  warningLabel->setVisible(false);
  if(_options)
    setControls(_options->_options);
#else
  useDefaultSettings->setEnabled(false);
  useDefaultSettings->setVisible(isLocal && _options);
  groupScrollArea->setEnabled(false);
  presetsGroup->setEnabled(false);
  warningLabel->setVisible(true);
#endif
  
  QList<QRadioButton*> allButtons = groupScrollArea->findChildren<QRadioButton*>();
  foreach(QRadioButton* button, allButtons)
  {
    connect(button, &QRadioButton::clicked, [this]() { buttonClicked(ConverterButtonId); } );
  }
  connect(defaultPreset, &QRadioButton::clicked, [this]() { buttonClicked(DefaultPresetId); } );
  connect(percussionPreset, &QRadioButton::clicked, [this]() { buttonClicked(PercussionPresetId); } );
  connect(maxPreset, &QRadioButton::clicked, [this]() { buttonClicked(MaxPresetId); } );
  connect(useDefaultSettings, &QCheckBox::clicked, [this]() { buttonClicked(DefaultsButtonId); } );
  connect(OKButton, &QPushButton::clicked, [this]() { buttonClicked(OkButtonId); } );
  connect(cancelButton, &QPushButton::clicked, [this]() { buttonClicked(CancelButtonId); } );
}
  
void RubberbandSettingsDialog::setControls(int opts)
{
#ifdef RUBBERBAND_SUPPORT
  
  if(opts & RubberBand::RubberBandStretcher::OptionStretchPrecise)
  {
    stretchPrecise->blockSignals(true);
    stretchPrecise->setChecked(true);
    stretchPrecise->blockSignals(false);
  }
  else
  {
    stretchElastic->blockSignals(true);
    stretchElastic->setChecked(true);
    stretchElastic->blockSignals(false);
  }


  if(opts & RubberBand::RubberBandStretcher::OptionTransientsMixed)
  {
    transientsMixed->blockSignals(true);
    transientsMixed->setChecked(true);
    transientsMixed->blockSignals(false);
  }
  else if(opts & RubberBand::RubberBandStretcher::OptionTransientsSmooth)
  {
    transientsSmooth->blockSignals(true);
    transientsSmooth->setChecked(true);
    transientsSmooth->blockSignals(false);
  }
  else
  {
    transientsCrisp->blockSignals(true);
    transientsCrisp->setChecked(true);
    transientsCrisp->blockSignals(false);
  }


  if(opts & RubberBand::RubberBandStretcher::OptionDetectorPercussive)
  {
    detectorPercussive->blockSignals(true);
    detectorPercussive->setChecked(true);
    detectorPercussive->blockSignals(false);
  }
  else if(opts & RubberBand::RubberBandStretcher::OptionDetectorSoft)
  {
    detectorSoft->blockSignals(true);
    detectorSoft->setChecked(true);
    detectorSoft->blockSignals(false);
  }
  else
  {
    detectorCompound->blockSignals(true);
    detectorCompound->setChecked(true);
    detectorCompound->blockSignals(false);
  }


  if(opts & RubberBand::RubberBandStretcher::OptionPhaseIndependent)
  {
    phaseIndependent->blockSignals(true);
    phaseIndependent->setChecked(true);
    phaseIndependent->blockSignals(false);
  }
  else
  {
    phaseLaminar->blockSignals(true);
    phaseLaminar->setChecked(true);
    phaseLaminar->blockSignals(false);
  }


  // if(opts & RubberBand::RubberBandStretcher::OptionThreadingNever)
  // {
  //   threadingNever->blockSignals(true);
  //   threadingNever->setChecked(true);
  //   threadingNever->blockSignals(false);
  // }
  // else if(opts & RubberBand::RubberBandStretcher::OptionThreadingAlways)
  // {
  //   threadingAlways->blockSignals(true);
  //   threadingAlways->setChecked(true);
  //   threadingAlways->blockSignals(false);
  // }
  // else
  // {
  //   threadingAuto->blockSignals(true);
  //   threadingAuto->setChecked(true);
  //   threadingAuto->blockSignals(false);
  // }


  if(opts & RubberBand::RubberBandStretcher::OptionWindowShort)
  {
    windowShort->blockSignals(true);
    windowShort->setChecked(true);
    windowShort->blockSignals(false);
  }
  else if(opts & RubberBand::RubberBandStretcher::OptionWindowLong)  
  {
    windowLong->blockSignals(true);
    windowLong->setChecked(true);
    windowLong->blockSignals(false);
  }
  else
  {
    windowStandard->blockSignals(true);
    windowStandard->setChecked(true);
    windowStandard->blockSignals(false);
  }


  if(opts & RubberBand::RubberBandStretcher::OptionSmoothingOn)
  {
    smoothingOn->blockSignals(true);
    smoothingOn->setChecked(true);
    smoothingOn->blockSignals(false);
  }
  else
  {
    smoothingOff->blockSignals(true);
    smoothingOff->setChecked(true);
    smoothingOff->blockSignals(false);
  }

    
  if(opts & RubberBand::RubberBandStretcher::OptionFormantPreserved)
  {
    formantPreserved->blockSignals(true);
    formantPreserved->setChecked(true);
    formantPreserved->blockSignals(false);
  }
  else
  {
    formantShifted->blockSignals(true);
    formantShifted->setChecked(true);
    formantShifted->blockSignals(false);
  }


  if(opts & RubberBand::RubberBandStretcher::OptionPitchHighQuality)
  {
    pitchHighQuality->blockSignals(true);
    pitchHighQuality->setChecked(true);
    pitchHighQuality->blockSignals(false);
  }
  else if(opts & RubberBand::RubberBandStretcher::OptionPitchHighConsistency)
  {
    pitchHighConsistency->blockSignals(true);
    pitchHighConsistency->setChecked(true);
    pitchHighConsistency->blockSignals(false);
  }
  else
  {
    pitchHighSpeed->blockSignals(true);
    pitchHighSpeed->setChecked(true);
    pitchHighSpeed->blockSignals(false);
  }

  if(opts & RubberBand::RubberBandStretcher::OptionChannelsTogether)
  {
    channelsTogether->blockSignals(true);
    channelsTogether->setChecked(true);
    channelsTogether->blockSignals(false);
  }
  else
  {
    channelsApart->blockSignals(true);
    channelsApart->setChecked(true);
    channelsApart->blockSignals(false);
  }
  
#endif
}

void RubberbandSettingsDialog::buttonClicked(int idx)
{
  switch(idx)
  {
    case DefaultsButtonId:
      OKButton->setEnabled(true);
      groupScrollArea->setEnabled(!useDefaultSettings->isChecked());
      presetsGroup->setEnabled(!useDefaultSettings->isChecked());
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
    
#ifdef RUBBERBAND_SUPPORT
    case DefaultPresetId:
      setControls(RubberBand::RubberBandStretcher::DefaultOptions);
      OKButton->setEnabled(true);
    break;
    
    case PercussionPresetId:
      setControls(RubberBand::RubberBandStretcher::PercussiveOptions);
      OKButton->setEnabled(true);
    break;
    
    case MaxPresetId:
      setControls(RubberBand::RubberBandStretcher::OptionStretchPrecise | 
                  RubberBand::RubberBandStretcher::OptionTransientsSmooth |
                  RubberBand::RubberBandStretcher::OptionDetectorSoft |
                  RubberBand::RubberBandStretcher::OptionPhaseIndependent |
                  RubberBand::RubberBandStretcher::OptionWindowLong |
                  RubberBand::RubberBandStretcher::OptionSmoothingOn |
                  RubberBand::RubberBandStretcher::OptionFormantPreserved |
                  RubberBand::RubberBandStretcher::OptionPitchHighConsistency |
                  RubberBand::RubberBandStretcher::OptionChannelsTogether);
      OKButton->setEnabled(true);
    break;
#endif
    
    default:
    break;
  }
}

//---------------------------------------------------------
//   accept
//---------------------------------------------------------

void RubberbandSettingsDialog::accept()
{
  if(!_options)
  {
    QDialog::accept();
    return;
  }
  
#ifdef RUBBERBAND_SUPPORT

  int opts = 0;

  if(stretchPrecise->isChecked())
    opts |= RubberBand::RubberBandStretcher::OptionStretchPrecise;
  else if(stretchElastic->isChecked())
    opts |= RubberBand::RubberBandStretcher::OptionStretchElastic;
  

  if(transientsMixed->isChecked())
    opts |= RubberBand::RubberBandStretcher::OptionTransientsMixed;
  else if(transientsSmooth->isChecked())
    opts |= RubberBand::RubberBandStretcher::OptionTransientsSmooth;
  else if(transientsCrisp->isChecked())
    opts |= RubberBand::RubberBandStretcher::OptionTransientsCrisp;
  

  if(detectorPercussive->isChecked())
    opts |= RubberBand::RubberBandStretcher::OptionDetectorPercussive;
  else if(detectorSoft->isChecked())
    opts |= RubberBand::RubberBandStretcher::OptionDetectorSoft;
  else if(detectorCompound->isChecked())
    opts |= RubberBand::RubberBandStretcher::OptionDetectorCompound;


  if(phaseIndependent->isChecked())
    opts |= RubberBand::RubberBandStretcher::OptionPhaseIndependent;
  else if(phaseLaminar->isChecked())
    opts |= RubberBand::RubberBandStretcher::OptionPhaseLaminar;
  

//   if(threadingNever->isChecked())
//     opts |= RubberBand::RubberBandStretcher::OptionThreadingNever;
//   else if(threadingAlways->isChecked())
//     opts |= RubberBand::RubberBandStretcher::OptionThreadingAlways;
//   else if(threadingAuto->isChecked())
//     opts |= RubberBand::RubberBandStretcher::OptionThreadingAuto;


  if(windowShort->isChecked())
    opts |= RubberBand::RubberBandStretcher::OptionWindowShort;
  else if(windowLong->isChecked())
    opts |= RubberBand::RubberBandStretcher::OptionWindowLong;
  else if(windowStandard->isChecked())
    opts |= RubberBand::RubberBandStretcher::OptionWindowStandard;


  if(smoothingOn->isChecked())
    opts |= RubberBand::RubberBandStretcher::OptionSmoothingOn;
  else if(smoothingOff->isChecked())
    opts |= RubberBand::RubberBandStretcher::OptionSmoothingOff;
  
    
  if(formantPreserved->isChecked())
    opts |= RubberBand::RubberBandStretcher::OptionFormantPreserved;
  else if(formantShifted->isChecked())
    opts |= RubberBand::RubberBandStretcher::OptionFormantShifted;
  

  if(pitchHighQuality->isChecked())
    opts |= RubberBand::RubberBandStretcher::OptionPitchHighQuality;
  else if(pitchHighConsistency->isChecked())
    opts |= RubberBand::RubberBandStretcher::OptionPitchHighConsistency;
  else if(pitchHighSpeed->isChecked())
    opts |= RubberBand::RubberBandStretcher::OptionPitchHighSpeed;


  if(channelsTogether->isChecked())
    opts |= RubberBand::RubberBandStretcher::OptionChannelsTogether;
  else if(channelsApart->isChecked())
    opts |= RubberBand::RubberBandStretcher::OptionChannelsApart;
  
  _options->_options = opts;
  
  _options->_useSettings = !useDefaultSettings->isChecked();
  
#endif
  
  QDialog::accept();
}


} // namespace MusEGui


