//=========================================================
//  MusE
//  Linux Music Editor
//
//  rubberband_converter.cpp
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
//
//=========================================================

#include <QDialog>
#include <QWidget>
#include <QRadioButton>
#include <QSignalMapper>
#include <QList>
// #include <QListWidgetItem>
// #include <QVariant>
// #include <qtextstream.h>

#include <math.h>
#include <stdio.h>

//#include "rubberband_converter.h"
#include "wave.h"
// #include "globals.h"
#include "time_stretch.h"
#include "xml.h"

#include "rubberband_converter.h"

#define ERROR_AUDIOCONVERT(dev, format, args...)  fprintf(dev, format, ##args)

// REMOVE Tim. samplerate. Enabled.
// For debugging output: Uncomment the fprintf section.
#define DEBUG_AUDIOCONVERT(dev, format, args...)  fprintf(dev, format, ##args)


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

  //_localSettings.initOptions(true);
  
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
  _latencyCompPending = true; // Set to compensate at the first process.
  
#ifdef RUBBERBAND_SUPPORT
//   _rbs = new RubberBand::RubberBandStretcher(MusEGlobal::sampleRate, _channels, _options);
  _rbs = new RubberBand::RubberBandStretcher(_systemSampleRate, _channels, _options);
  // , initialTimeRatio = 1.0, initialPitchScale = 1.0 DELETETHIS
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
//   _rbs = new RubberBand::RubberBandStretcher(MusEGlobal::sampleRate, _channels, _options);
  _rbs = new RubberBand::RubberBandStretcher(_systemSampleRate, _channels, _options);
  // , initialTimeRatio = 1.0, initialPitchScale = 1.0
#endif
}

void RubberBandAudioConverter::reset()
{
  DEBUG_AUDIOCONVERT(stderr, "RubberBandAudioConverter::reset this:%p\n", this);
#ifdef RUBBERBAND_SUPPORT
  if(!_rbs)
    return;
  _rbs->reset();
  _latencyCompPending = true;
  return;  
#endif
}

#ifdef RUBBERBAND_SUPPORT
int RubberBandAudioConverter::process(SndFile* sf, SNDFILE* handle, sf_count_t pos, 
                                      float** buffer, int channels, int frames, bool overwrite)
{
  if(!_rbs)
    return 0;
  
  //return src_process(_src_state, sd); DELETETHIS
  
//   if(f.isNull())
//     return _sfCurFrame;
  
  // Added by Tim. p3.3.17 DELETETHIS 4
  //#ifdef AUDIOCONVERT_DEBUG_PRC
  //DEBUG_AUDIOCONVERT(stderr, "AudioConverter::process %s audConv:%p sfCurFrame:%ld offset:%u channel:%d fchan:%d n:%d\n", 
  //        f.name().toLatin1(), this, sfCurFrame, offset, channel, f.channels(), n);
  //#endif
  
//  off_t frame     = offset;  // _spos is added before the call. DELETETHIS
  
//   unsigned fsrate = f.samplerate();
  
  //bool resample   = src_state && ((unsigned)MusEGlobal::sampleRate != fsrate);   DELETETHIS 2
//  bool resample   = isValid() && ((unsigned)MusEGlobal::sampleRate != fsrate);  
  
//   if((MusEGlobal::sampleRate == 0) || (fsrate == 0))
//   if((MusEGlobal::sampleRate == 0) || (f.samplerate() == 0))
//   if((MusEGlobal::sampleRate == 0) || (sf->samplerate() == 0))
  if((_systemSampleRate <= 0) || (sf->samplerate() <= 0))
  {  
    DEBUG_AUDIOCONVERT(stderr, "RubberBandAudioConverter::process Error: _systemSampleRate or file samplerate <= 0!\n");
//     return _sfCurFrame;
    return 0;
  }  
  
  StretchList* stretch_list = sf->stretchList();

  //const double stretch = stretch_list->stretchAt(_sfCurFrame * srcratio);
  //const double stretch = stretch_list->stretchAt(_sfCurFrame);
//   const double stretchVal    = stretch_list->stretchAt(pos);
//   const double samplerateVal = stretch_list->samplerateAt(pos);
  //const MuseFrame_t new_frame = sf->convertPosition(pos);
  //const MuseFrame_t new_frame = stretch_list->unStretch(pos);
  const MuseFrame_t new_frame = stretch_list->unSquish(pos);
  const double stretchVal    = stretch_list->ratioAt(StretchListItem::StretchEvent, new_frame);
  const double samplerateVal = stretch_list->ratioAt(StretchListItem::SamplerateEvent, new_frame);
  //DEBUG_AUDIOCONVERT(stderr,
  //  "RubberBandAudioConverter::process: frame:%ld new_frame:%ld stretchRatio:%f samplerateRatio:%f\n", pos, new_frame, stretchVal, samplerateVal);
  
  const double fin_samplerateRatio = sf->sampleRateRatio() + samplerateVal - 1.0;
  
  if(fin_samplerateRatio < 0.0001)
  {  
    DEBUG_AUDIOCONVERT(stderr, "RubberBandAudioConverter::process Error: fin_samplerateRatio ratio is near zero!\n");
//     return _sfCurFrame;
    return 0;
  }  
  
  const double inv_fin_samplerateRatio = 1.0 / fin_samplerateRatio;
  
//  SRC_DATA srcdata;
  const int fchan       = sf->channels();
  
//   // Ratio is defined as output sample rate over input samplerate.
//   double srcratio = (double)MusEGlobal::sampleRate / (double)fsrate;
  
  // Extra input compensation.
//   sf_count_t inComp = 1;
  
//   sf_count_t outFrames  = frames;
//   sf_count_t outSize    = outFrames * fchan;
  
  //long inSize = long(outSize * srcratio) + 1                      // From MusE-2 file converter.  DELETETHIS 3
  //long inSize = (long)floor(((double)outSize / srcratio));        // From simplesynth.
  //long inFrames = (long)floor(((double)outFrames / srcratio));    // From simplesynth.
//   long inFrames = (long)ceil(((double)outFrames / srcratio)) + inComp;    // From simplesynth.
//   sf_count_t inFrames = ceil(((double)outFrames * srcratio)) + inComp;    // From simplesynth.
  
//   sf_count_t inSize = inFrames * fchan;
  //sf_count_t inSize = inFrames * channel; DELETETHIS
  
  // Start with buffers at expected sizes. We won't need anything larger than this, but add 4 for good luck.
//   float inbuffer[inSize]; // +4
//  float outbuffer[outSize]; DELETETHIS
      
  //float* rbinbuffer[fchan]; DELETETHIS 4
  //float rbindata[inSize];
  //for (int i = 0; i < fchan; ++i)
  //      rbinbuffer[i] = rbindata + i * inFrames;
      
  //int cur_lat = _rbs->getLatency();

  int debug_min_pos = 256;

  int old_lat = _rbs->getLatency();
  // For just sample rate conversion, apply complimentary ratio to time and pitch.
  _rbs->setTimeRatio(inv_fin_samplerateRatio * stretchVal);
  _rbs->setPitchScale(fin_samplerateRatio);

  int new_lat = _rbs->getLatency();
//   cur_lat -= 16; // HACK: Incorrect latency is always +1 more ?
//   if(cur_lat < 0)
//     cur_lat = 0;

  // Force a latency compensation step if latency changed.
  //if(new_lat != cur_lat)
  //  _latencyCompPending = true;

  sf_count_t outFrames  = frames;
  if(_latencyCompPending)
    //outFrames += new_lat;
    outFrames += old_lat;
  sf_count_t outSize    = outFrames * fchan;
  float* rboutbuffer[fchan];
  float rboutdata[outSize];
  for(int i = 0; i < fchan; ++i)
    rboutbuffer[i] = rboutdata + i * outFrames;
      
  sf_count_t rn           = 0;
//   sf_count_t totalOutFrames = 0;
  
//  srcdata.data_in       = inbuffer; DELETETHIS 3
  //srcdata.data_out      = outbuffer;
//  srcdata.data_out      = buffer;
//   float** data_out       = rboutbuffer;
  
  // Set some kind of limit on the number of attempts to completely fill the output buffer, 
  //  in case something is really screwed up - we don't want to get stuck in a loop here.
  //int attempts = 10;
  //for(int attempt = 0; attempt < attempts; ++attempt)
  //int savail;
//   size_t sreq;
  //int frame = 0;
  
  // Tested: Latency will change after the above setting of ratios. Also affected by converter options.
  //fprintf(stderr, "RubberBandAudioConverter::process latency:%lu\n", _rbs->getLatency());


//   if(_latencyCompPending)
//   {
//     int lat = _rbs->getLatency() - 1; // HACK: Incorrect latency is always +1 more ?
//     //int lat = 64;
//     while(_rbs->available() < lat)
//     {
//       size_t sreq = _rbs->getSamplesRequired();
//       DEBUG_AUDIOCONVERT(stderr, "   Latency: required:%d avail:%d\n", int(sreq), _rbs->available());
//       if(sreq <= 0)
//           break;
//       size_t rbinSize = sreq * fchan;
//       float rbindata[rbinSize];
//       // Must de-interleave data to feed to rubberband.
//       float* rbinbuffer[fchan];
//       for(int i = 0; i < fchan; ++i)
//         rbinbuffer[i] = rbindata + i * sreq;
//       memset(rbindata, 0, rbinSize * sizeof(float));
//
//       _rbs->process(rbinbuffer, sreq, false);
//     }
//
//     int savail = _rbs->available();
//     DEBUG_AUDIOCONVERT(stderr, "   Latency: final avail:%d\n", _rbs->available());
//     //if(savail > lat)
//     //  savail = lat;
//
//
//     float* outbuf[fchan];
//     //float outdat[lat];
//     float outdat[savail * fchan];
//     for(int i = 0; i < fchan; ++i)
//       //outbuf[i] = outdat + i * lat;
//       outbuf[i] = outdat + i * savail;
//
//
//     // Retrieves de-interleaved data.
//     size_t retrieved = _rbs->retrieve(outbuf, savail);
//     if((int)retrieved < savail)
//     {
//       DEBUG_AUDIOCONVERT(stderr, "RubberBandAudioConverter::process: Latency retrieved_count:%d is less than savail:%d\n", int(retrieved), savail);
//       savail = retrieved;
//     }
//
//     if(savail < lat)
//     {
//       DEBUG_AUDIOCONVERT(stderr, "RubberBandAudioConverter::process: Latency available() is less than needed. Converter is finished ???\n");
//       // We didn't get the total required frames. Zero the rest.
//       //const size_t zeros = lat - savail;
//       //for(int i = 0; i < fchan; ++i)
//       //  memset(outbuf[i] + savail, 0, zeros * sizeof(float));
//     }
//
// //     _latencyCompPending = false;
//   }


  //if(_latencyCompPending && new_lat > 0)
  if(_latencyCompPending && old_lat > 0)
  {
    //size_t rbinSize = new_lat * fchan;
    size_t rbinSize = old_lat * fchan;
    float rbindata[rbinSize];
    memset(rbindata, 0, rbinSize * sizeof(float));
    // Must de-interleave data to feed to rubberband.
    float* rbinbuffer[fchan];
    for(int i = 0; i < fchan; ++i)
      //rbinbuffer[i] = rbindata + i * new_lat;
      rbinbuffer[i] = rbindata + i * old_lat;
    if(pos <= debug_min_pos)
      //DEBUG_AUDIOCONVERT(stderr, "   Latency: new_lat:%d\n", new_lat);
      DEBUG_AUDIOCONVERT(stderr, "   Latency: old_lat:%d new_lat:%d\n", old_lat, new_lat);
    //_rbs->process(rbinbuffer, new_lat, false);
    _rbs->process(rbinbuffer, old_lat, false);
  }

  while(_rbs->available() < outFrames)
  {
    size_t sreq = _rbs->getSamplesRequired();
    if(sreq <= 0)
        break;
    
    size_t rbinSize = sreq * fchan;
    float sfdata[rbinSize];
    rn = sf_readf_float(handle,  sfdata, sreq);
    _sfCurFrame += rn;
    // Zero any buffer portion not filled by the file read. TODO: De-normals required here?
    if((size_t)rn != sreq)
    {
      const size_t zeros = (sreq - rn) * fchan;
      const size_t zero_start = rn * fchan;
      memset(&sfdata[zero_start], 0, zeros * sizeof(float));
    }
    
    // Must de-interleave soundfile data to feed to rubberband.
    // TODO: Optimizations! Would like fast in-place de-interleaving, but it's "harder than it looks". 
    //       There are even patents on this kind of thing!
    //       If optimized, be sure to move it to our AL-DSP (assembly) library. With SSE etc, it's easier. 
    //       Alas, for now use out-of-place memory since stack *should* be cheap here without much nesting.
    float rbindata[rbinSize];
    float* rbinbuffer[fchan];
    for(int i = 0; i < fchan; ++i)
      rbinbuffer[i] = rbindata + i * sreq;
    float* sfptr = sfdata;
    for(size_t i = 0; i < sreq; ++i) 
    {
      for(int ch = 0; ch < fchan; ++ch)
        *(rbinbuffer[ch] + i) = *sfptr++;
    }
    if(pos <= debug_min_pos)
      DEBUG_AUDIOCONVERT(stderr, "   Normal: required:%d avail:%d\n", int(sreq), _rbs->available());
    _rbs->process(rbinbuffer, sreq, false);
  }
  
  int savail = _rbs->available();
  if(pos <= debug_min_pos)
    DEBUG_AUDIOCONVERT(stderr, "   Normal: final avail:%d\n", _rbs->available());
  if(savail > outFrames)
    savail = outFrames;
  
  //if(_latencyCompPending && new_lat > 0)
  if(_latencyCompPending && old_lat > 0)
  {
    //size_t retrieved = _rbs->retrieve(rboutbuffer, new_lat);
    size_t retrieved = _rbs->retrieve(rboutbuffer, old_lat);
    //if((int)retrieved < new_lat)
    if((int)retrieved < old_lat)
    //{ DEBUG_AUDIOCONVERT(stderr, "RubberBandAudioConverter::process: Latency retrieved_count:%d is less than requested:%d\n", int(retrieved), new_lat); }
    { DEBUG_AUDIOCONVERT(stderr, "RubberBandAudioConverter::process: Latency retrieved_count:%d is less than requested:%d\n", int(retrieved), old_lat); }
    //savail -= new_lat;
    savail -= old_lat;
  }
  _latencyCompPending = false;

  // Retrieves de-interleaved data.
  size_t retrieved = _rbs->retrieve(rboutbuffer, savail);
  if((int)retrieved < savail)
  {
    DEBUG_AUDIOCONVERT(stderr, "RubberBandAudioConverter::process: retrieved_count:%d is less than savail:%d\n", int(retrieved), savail);
    savail = retrieved;
  }
  
  if(savail < frames)
  {
    DEBUG_AUDIOCONVERT(stderr, "RubberBandAudioConverter::process: available() is less than needed. Converter is finished ???\n");
    // We didn't get the total required frames. Zero the rest.
    const size_t zeros = frames - savail;
    for(int i = 0; i < fchan; ++i)
      memset(rboutbuffer[i] + savail, 0, zeros * sizeof(float));
  }
  
  if(fchan == channels)
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
  else if((fchan == 2) && (channels == 1)) 
  {
    // stereo to mono
    if(overwrite)
      for(int i = 0; i < frames; ++i)
        *(buffer[0] + i) = *(rboutbuffer[0] + i) + *(rboutbuffer[1] + i);
    else  
      for(int i = 0; i < frames; ++i)
        *(buffer[0] + i) += *(rboutbuffer[0] + i) + *(rboutbuffer[1] + i);
  }
  else if((fchan == 1) && (channels == 2)) 
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
    DEBUG_AUDIOCONVERT(stderr, "RubberBandAudioConverter::process Channel mismatch: source chans:%d -> dst chans:%d\n", fchan, channels);
  }
  
  return frames;
}

#else // RUBBERBAND_SUPPORT

int RubberBandAudioConverter::process(SndFile* /*sf*/, SNDFILE* /*handle*/, sf_count_t /*pos*/, 
                                      float** /*buffer*/, int /*channels*/, int /*frames*/, bool /*overwrite*/)
{
  return 0;
}

#endif // RUBBERBAND_SUPPORT

// void RubberBandAudioConverter::read(Xml&)
// {
//   
// }
// 
// void RubberBandAudioConverter::write(int, Xml&) const
// {
//   
// }


//---------------------------------------------------------
//   RubberBandAudioConverterSettings
//---------------------------------------------------------

void RubberBandAudioConverterOptions::write(int level, Xml& xml) const
      {
      //xml.tag(level++, "settings");
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
      
// MusECore::AudioConverterSettings* RubberBandAudioConverterSettings::createSettings(bool isLocal)
// {
//   return new MusECore::RubberBandAudioConverterSettings(isLocal);
// }

void RubberBandAudioConverterSettings::assign(const AudioConverterSettings& other)
{
  const RubberBandAudioConverterSettings& rb_other = 
    (const RubberBandAudioConverterSettings&)other;
  _offlineOptions  = rb_other._offlineOptions;
  _realtimeOptions = rb_other._realtimeOptions;
  _guiOptions      = rb_other._guiOptions;
}

// bool RubberBandAudioConverterSettings::isSet(int mode) const 
// { 
//   if(mode & ~(AudioConverterSettings::OfflineMode | 
//               AudioConverterSettings::RealtimeMode | 
//               AudioConverterSettings::GuiMode))
//     fprintf(stderr, "RubberBandAudioConverterSettings::isSet() Warning: Unknown modes included:%d\n", mode);
//   
//   if((mode <= 0 || (mode & AudioConverterSettings::OfflineMode)) && _offlineOptions.isSet())
//     return true;
// 
//   if((mode <= 0 || (mode & AudioConverterSettings::RealtimeMode)) && _realtimeOptions.isSet())
//     return true;
// 
//   if((mode <= 0 || (mode & AudioConverterSettings::GuiMode)) && _guiOptions.isSet())
//     return true;
//     
//   return false;
// }

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
//       if(!useSettings())
//         return;
//       
//       xml.tag(level++, "rubberbandSettings");
//       //xml.tag(level++, "audioConverterSettings converterID=\"%d\" name=\"%s\"", descriptor->_ID, descriptor->_name);
//       
// //       if(_offlineOptions._optionProcess != -1)
// //         xml.intTag(level, "offlineProcess", _offlineOptions._optionProcess);
// //       if(_realtimeOptions._optionProcess != -1)
// //         xml.intTag(level, "realtimeProcess", _realtimeOptions._optionProcess);
// //       
// //       if(_offlineOptions._optionStretch != -1)
// //         xml.intTag(level, "offlineStretch", _offlineOptions._optionStretch);
// //       if(_realtimeOptions._optionStretch != -1)
// //         xml.intTag(level, "realtimeStretch", _realtimeOptions._optionStretch);
// //       
// //       if(_offlineOptions._optionThreading != -1)
// //         xml.intTag(level, "offlineThreading", _offlineOptions._optionThreading);
// //       if(_realtimeOptions._optionThreading != -1)
// //         xml.intTag(level, "realtimeThreading", _realtimeOptions._optionThreading);
// //       
// //       if(_offlineOptions._optionWindow != -1)
// //         xml.intTag(level, "offlineWindow", _offlineOptions._optionWindow);
// //       if(_realtimeOptions._optionWindow != -1)
// //         xml.intTag(level, "realtimeWindow", _realtimeOptions._optionWindow);
// //       
// //       if(_offlineOptions._optionSmoothing != -1)
// //         xml.intTag(level, "offlineSmoothing", _offlineOptions._optionSmoothing);
// //       if(_realtimeOptions._optionSmoothing != -1)
// //         xml.intTag(level, "realtimeSmoothing", _realtimeOptions._optionSmoothing);
// //       
// //       if(_offlineOptions._optionChannels != -1)
// //         xml.intTag(level, "offlineChannels", _offlineOptions._optionChannels);
// //       if(_realtimeOptions._optionChannels != -1)
// //         xml.intTag(level, "realtimeChannels", _realtimeOptions._optionChannels);
// //       
// //       if(_offlineOptions._optionTransients != -1)
// //         xml.intTag(level, "offlineTransients", _offlineOptions._optionTransients);
// //       if(_realtimeOptions._optionTransients != -1)
// //         xml.intTag(level, "realtimeTransients", _realtimeOptions._optionTransients);
// //       
// //       if(_offlineOptions._optionDetector != -1)
// //         xml.intTag(level, "offlineDetector", _offlineOptions._optionDetector);
// //       if(_realtimeOptions._optionDetector != -1)
// //         xml.intTag(level, "realtimeDetector", _realtimeOptions._optionDetector);
// //       
// //       if(_offlineOptions._optionPhase != -1)
// //         xml.intTag(level, "offlinePhase", _offlineOptions._optionPhase);
// //       if(_realtimeOptions._optionPhase != -1)
// //         xml.intTag(level, "realtimePhase", _realtimeOptions._optionPhase);
// //       
// //       if(_offlineOptions._optionFormant != -1)
// //         xml.intTag(level, "offlineFormant", _offlineOptions._optionFormant);
// //       if(_realtimeOptions._optionFormant != -1)
// //         xml.intTag(level, "realtimeFormant", _realtimeOptions._optionFormant);
// //       
// //       if(_offlineOptions._optionPitch != -1)
// //         xml.intTag(level, "offlinePitch", _offlineOptions._optionPitch);
// //       if(_realtimeOptions._optionPitch != -1)
// //         xml.intTag(level, "realtimePitch", _realtimeOptions._optionPitch);
//       
//       if(_offlineOptions._options != -1)
//         xml.intTag(level, "offlineOptions", _offlineOptions._options);
//       if(_realtimeOptions._options != -1)
//         xml.intTag(level, "realtimeOptions", _realtimeOptions._options);
//       
//       xml.tag(--level, "/rubberbandSettings");
//       //xml.tag(--level, "/audioConverterSettings");
  
  
  
//   const bool use_off = !(_offlineOptions == defaultOfflineOptions);
//   const bool use_rt  = !(_realtimeOptions == defaultRealtimeOptions);
//   const bool use_gui = !(_guiOptions == defaultGuiOptions);
// 
//   if(use_off | use_rt || use_gui)
//   {
//     xml.tag(level++, "rubberbandSettings");
//     
//     if(use_off)
//     {
//       xml.tag(level++, "offline");
//       _offlineOptions.write(level, xml);
//       xml.tag(--level, "/offline");
//     }
//     
//     if(use_rt)
//     {
//       xml.tag(level++, "realtime");
//       _realtimeOptions.write(level, xml);
//       xml.tag(--level, "/realtime");
//     }
//     
//     if(use_gui)
//     {
//       xml.tag(level++, "gui");
//       _guiOptions.write(level, xml);
//       xml.tag(--level, "/gui");
//     }
//     
//     xml.tag(--level, "/rubberbandSettings");
//   }
  
  const bool use_off = !(_offlineOptions == RubberBandAudioConverterOptions::defaultOfflineOptions);
  const bool use_rt  = !(_realtimeOptions == RubberBandAudioConverterOptions::defaultRealtimeOptions);
  const bool use_gui = !(_guiOptions == RubberBandAudioConverterOptions::defaultGuiOptions);

  if(use_off | use_rt || use_gui)
  {
    //xml.tag(level++, "audioConverterSetting id=\"%d\"", descriptor._ID);
    //xml.tag(level++, "audioConverterSetting name=\"%s\"", descriptor._name);
    xml.tag(level++, "audioConverterSetting name=\"%s\"", Xml::xmlString(descriptor._name).toLatin1().constData());
    //xml.tag(level++, descriptor._name);
    
    if(use_off)
    {
      //xml.tag(level++, "offline");
      //xml.tag(level, "audioConverterSetting id=\"%d\" mode=\"%d\"", descriptor._ID, AudioConverterSettings::OfflineMode);
      //xml.tag(level, "audioConverterSetting mode=\"%d\"", AudioConverterSettings::OfflineMode);
      _offlineOptions.write(level, xml);
      //xml.tag(--level, "/offline");
      //xml.tag(--level, "/audioConverterSetting");
    }
    
    if(use_rt)
    {
      //xml.tag(level++, "realtime");
      //xml.tag(level, "audioConverterSetting id=\"%d\" mode=\"%d\"", descriptor._ID, AudioConverterSettings::RealtimeMode);
      //xml.tag(level, "audioConverterSetting mode=\"%d\"", AudioConverterSettings::RealtimeMode);
      _realtimeOptions.write(level, xml);
      //xml.tag(--level, "/realtime");
      //xml.tag(--level, "/audioConverterSetting");
    }
    
    if(use_gui)
    {
      //xml.tag(level++, "gui");
      //xml.tag(level, "audioConverterSetting id=\"%d\" mode=\"%d\"", descriptor._ID, AudioConverterSettings::GuiMode);
      //xml.tag(level, "audioConverterSetting mode=\"%d\"", AudioConverterSettings::GuiMode);
      _guiOptions.write(level, xml);
      //xml.tag(--level, "/gui");
      //xml.tag(--level, "/audioConverterSetting");
    }
    
    xml.tag(--level, "/audioConverterSetting");
  }
}

void RubberBandAudioConverterSettings::read(Xml& xml)
      {
//       for (;;) {
//             Xml::Token token = xml.parse();
//             const QString& tag = xml.s1();
//             switch (token) {
//                   case Xml::Error:
//                   case Xml::End:
//                         return;
//                   case Xml::TagStart:
//                         if (tag == "offlineOptions")
//                               _offlineOptions._options = xml.parseInt();
//                         else if (tag == "realtimeOptions")
//                               _realtimeOptions._options = xml.parseInt();
//                         else if (tag == "guiOptions")
//                               _guiOptions._options = xml.parseInt();
//                         
//                         else
//                               xml.unknown("rubberbandSettings");
//                               //xml.unknown("audioConverterSettings");
//                         break;
//                   case Xml::Attribut:
//                               fprintf(stderr, "rubberbandSettings unknown tag %s\n", tag.toLatin1().constData());
//                               //fprintf(stderr, "audioConverterSettings unknown tag %s\n", tag.toLatin1().constData());
//                         break;
//                   case Xml::TagEnd:
//                         if (tag == "rubberbandSettings") {
//                         //if (tag == "audioConverterSettings") {
//                               return;
//                               }
//                   default:
//                         break;
//                   }
//             }
            
  int mode = -1;
  for (;;) {
      Xml::Token token = xml.parse();
      const QString& tag = xml.s1();
      switch (token) {
            case Xml::Error:
            case Xml::End:
                  return;
            case Xml::TagStart:
//                   if (tag == "offline")
//                         _offlineOptions.read(xml);
//                   else if (tag == "realtime")
//                         _realtimeOptions.read(xml);
//                   else if (tag == "gui")
//                         _guiOptions.read(xml);
                  
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
                      //xml.unknown("rubberbandSettings");
                      xml.unknown("settings");
                  break;
            case Xml::Attribut:
                  if (tag == "mode")
                        mode = xml.s2().toInt();
                  else
                        //fprintf(stderr, "rubberbandSettings unknown tag %s\n", tag.toLatin1().constData());
                        fprintf(stderr, "settings unknown tag %s\n", tag.toLatin1().constData());
                  break;
            case Xml::TagEnd:
                  //if (tag == "rubberbandSettings") {
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
  
  _signalMapper = new QSignalMapper(this);
  
  QList<QRadioButton*> allButtons = groupScrollArea->findChildren<QRadioButton*>();
  foreach(QRadioButton* button, allButtons)
  {
    connect(button, SIGNAL(clicked()), _signalMapper, SLOT(map()));
    _signalMapper->setMapping(button, ConverterButtonId);
  }
  connect(useDefaultSettings,  SIGNAL(clicked()), _signalMapper, SLOT(map()));
  connect(OKButton,            SIGNAL(clicked()), _signalMapper, SLOT(map()));
  connect(cancelButton,        SIGNAL(clicked()), _signalMapper, SLOT(map()));
  connect(defaultPreset,       SIGNAL(clicked()), _signalMapper, SLOT(map()));
  connect(percussionPreset,    SIGNAL(clicked()), _signalMapper, SLOT(map()));
  connect(maxPreset,           SIGNAL(clicked()), _signalMapper, SLOT(map()));
  _signalMapper->setMapping(useDefaultSettings,  DefaultsButtonId);
  _signalMapper->setMapping(OKButton,            OkButtonId);
  _signalMapper->setMapping(cancelButton,        CancelButtonId);
  _signalMapper->setMapping(defaultPreset,       DefaultPresetId);
  _signalMapper->setMapping(percussionPreset,    PercussionPresetId);
  _signalMapper->setMapping(maxPreset,           MaxPresetId);
  connect(_signalMapper, SIGNAL(mapped(int)), this, SLOT(buttonClicked(int)));
}
  
void RubberbandSettingsDialog::setControls(int opts)
{
#ifdef RUBBERBAND_SUPPORT
  
//   if(!_options)
//     return;
//   
// //   if(useDefaultSettings->isVisible())
// //   {
// //     useDefaultSettings->blockSignals(true);
// //     useDefaultSettings->setChecked(!_options->_useSettings);
// //     useDefaultSettings->blockSignals(false);
// //   }
// 
//   const int opts = _options->_options;

//   if(opts & RubberBand::RubberBandStretcher::OptionProcessRealTime)
//   {
//     processRealTime->blockSignals(true);
//     processRealTime->setChecked(true);
//     processRealTime->blockSignals(false);
//   }
//   else
//   {
//     processOffline->blockSignals(true);
//     processOffline->setChecked(true);
//     processOffline->blockSignals(false);
//   }


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

  //groupScrollArea->setEnabled(!useDefaultSettings->isVisible() || _options->_useSettings);
  
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

//   if(processRealTime->isChecked())
//     opts |= RubberBand::RubberBandStretcher::OptionProcessRealTime;
//   else if(processOffline->isChecked())
//     opts |= RubberBand::RubberBandStretcher::OptionProcessOffline;

  
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


