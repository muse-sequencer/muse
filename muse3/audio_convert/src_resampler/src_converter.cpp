//=========================================================
//  MusE
//  Linux Music Editor
//
//  src_converter.cpp
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

#include <QRadioButton>
// #include <QListWidgetItem>
// #include <QVariant>
// #include <qtextstream.h>

#include <math.h>
#include <stdio.h>

//#include "src_converter.h"
// #include "globals.h"
#include "time_stretch.h"

#include "src_converter.h"
// #include "audio_convert/audio_converter_settings_group.h"
// #include "audio_convert/audio_converter_plugin.h"

// For debugging output: Uncomment the fprintf section.
#define ERROR_AUDIOCONVERT(dev, format, args...) // fprintf(dev, format, ##args)
#define DEBUG_AUDIOCONVERT(dev, format, args...) // fprintf(dev, format, ##args)

// Fixed audio input buffer size.
#define SRC_IN_BUFFER_FRAMES 1024

//namespace MusECore {
  
// Create a new instance of the plugin.  
// Mode is an AudioConverterSettings::ModeType selecting which of the settings to use.
MusECore::AudioConverter* instantiate(int systemSampleRate,
                                      const MusECore::AudioConverterDescriptor* /*Descriptor*/,
                                      int channels, 
                                      MusECore::AudioConverterSettings* settings, 
                                      int mode)
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

SRCAudioConverter::SRCAudioConverter(int systemSampleRate, int channels, AudioConverterSettings* settings, int mode) 
  : AudioConverter(systemSampleRate)
{
  DEBUG_AUDIOCONVERT(stderr, "SRCAudioConverter::SRCAudioConverter this:%p channels:%d mode:%d\n", 
                     this, channels, mode);

  //_localSettings.initOptions(true);
  
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

  //_inBufferSize = MusEGlobal::segmentSize;
  _inBufferSize = SRC_IN_BUFFER_FRAMES * _channels;
  _inbuffer = new float[_inBufferSize];
  _curInBufferFrame = 0;
  _needBuffer = true;
  //_endPending = false;
  //_outBufferSize = MusEGlobal::segmentSize;
  //_outbuffer = new float[_outBufferSize];

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

  //if(_outbuffer)
  //  delete[] _outbuffer;

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
//   _srcdata.data_in = _inbuffer;
//   //_srcdata.data_out = _outbuffer;
//   _srcdata.data_out = NULL;
//
//   //_srcdata.input_frames = _inBufferSize;
//   _srcdata.input_frames = 0;
//   //_srcdata.output_frames = _outBufferSize;
//   _srcdata.output_frames = 0;
//
//   _srcdata.input_frames_used = 0;
//   _srcdata.output_frames_gen = 0;
//
//   _srcdata.end_of_input = false;
//
//   _srcdata.src_ratio = 1.0;

  _curInBufferFrame = 0;
  _needBuffer = true;
}

// sf_count_t SRCAudioConverter::process(MusECore::SndFileR& f, float** buffer, int channel, int n, bool overwrite)
// {
//   //return src_process(_src_state, sd); DELETETHIS
//   
//   if(f.isNull())
//     return _sfCurFrame;
//   
//   // Added by Tim. p3.3.17 DELETETHIS 4
//   //#ifdef AUDIOCONVERT_DEBUG_PRC
//   //DEBUG_AUDIOCONVERT(stderr, "AudioConverter::process %s audConv:%p sfCurFrame:%ld offset:%u channel:%d fchan:%d n:%d\n", 
//   //        f.name().toLatin1(), this, sfCurFrame, offset, channel, f.channels(), n);
//   //#endif
//   
// //  off_t frame     = offset;  // _spos is added before the call. DELETETHIS
//   unsigned fsrate = f.samplerate();
//   //bool resample   = src_state && ((unsigned)MusEGlobal::sampleRate != fsrate);   DELETETHIS 2
// //  bool resample   = isValid() && ((unsigned)MusEGlobal::sampleRate != fsrate);  
//   
//   if((MusEGlobal::sampleRate == 0) || (fsrate == 0))
//   {  
//     #ifdef AUDIOCONVERT_DEBUG
//     DEBUG_AUDIOCONVERT(stderr, "SRCAudioConverter::process Error: MusEGlobal::sampleRate or file samplerate is zero!\n");
//     #endif
//     return _sfCurFrame;
//   }  
//   
//   SRC_DATA srcdata;
//   int fchan       = f.channels();
//   // Ratio is defined as output sample rate over input samplerate.
//   double srcratio = (double)MusEGlobal::sampleRate / (double)fsrate;
//   // Extra input compensation.
//   long inComp = 1;
//   
//   long outFrames  = n;  
//   //long outSize   = outFrames * channel; DELETETHIS
//   long outSize    = outFrames * fchan;
//   
//   //long inSize = long(outSize * srcratio) + 1                      // From MusE-2 file converter. DELETETHIS3
//   //long inSize = (long)floor(((double)outSize / srcratio));        // From simplesynth.
//   //long inFrames = (long)floor(((double)outFrames / srcratio));    // From simplesynth.
//   long inFrames = (long)ceil(((double)outFrames / srcratio)) + inComp;    // From simplesynth.
//   // DELETETHIS
//   //long inFrames = (long)floor(double(outFrames * sfinfo.samplerate) / double(MusEGlobal::sampleRate));    // From simplesynth.
//   
//   long inSize = inFrames * fchan;
//   //long inSize = inFrames * channel; DELETETHIS
//   
//   // Start with buffers at expected sizes. We won't need anything larger than this, but add 4 for good luck.
//   float inbuffer[inSize + 4];
//   float outbuffer[outSize];
//       
//   //size_t sfTotalRead  = 0; DELETETHIS
//   size_t rn           = 0;
//   long totalOutFrames = 0;
//   
//   srcdata.data_in       = inbuffer;
//   srcdata.data_out      = outbuffer;
// //  srcdata.data_out      = buffer; DELETETHIS
//   
//   // Set some kind of limit on the number of attempts to completely fill the output buffer, 
//   //  in case something is really screwed up - we don't want to get stuck in a loop here.
//   int attempts = 10;
//   for(int attempt = 0; attempt < attempts; ++attempt)
//   {
//     rn = f.readDirect(inbuffer, inFrames);
//     //sfTotalRead += rn; DELETETHIS
//     
//     // convert
//     //srcdata.data_in       = inbuffer; DELETETHIS 4
//     //srcdata.data_out      = outbuffer;
//     //srcdata.data_out      = poutbuf;
//     //srcdata.input_frames  = inSize;
//     srcdata.input_frames  = rn;
//     srcdata.output_frames = outFrames;
//     srcdata.end_of_input  = ((long)rn != inFrames);
//     srcdata.src_ratio     = srcratio;
//   
//     //#ifdef AUDIOCONVERT_DEBUG_PRC DELETETHIS or comment it in, or maybe add an additional if (heavyDebugMsg)?
//     //DEBUG_AUDIOCONVERT(stderr, "AudioConverter::process attempt:%d inFrames:%ld outFrames:%ld rn:%d data in:%p out:%p", 
//     //  attempt, inFrames, outFrames, rn, srcdata.data_in, srcdata.data_out);
//     //#endif
//     
//     int srcerr = src_process(_src_state, &srcdata);
//     if(srcerr != 0)      
//     {
//       DEBUG_AUDIOCONVERT(stderr, "\nSRCAudioConverter::process SampleRate converter process failed: %s\n", src_strerror(srcerr));
//       return _sfCurFrame += rn;
//     }
//     
//     totalOutFrames += srcdata.output_frames_gen;
//     
//     //#ifdef AUDIOCONVERT_DEBUG_PRC DELETETHIS or comment in or heavyDebugMsg
//     //DEBUG_AUDIOCONVERT(stderr, " frames used in:%ld out:%ld totalOutFrames:%ld data in:%p out:%p\n", srcdata.input_frames_used, srcdata.output_frames_gen, totalOutFrames, srcdata.data_in, srcdata.data_out);
//     //#endif
//     
//     #ifdef AUDIOCONVERT_DEBUG
//     if(srcdata.output_frames_gen != outFrames)
//       DEBUG_AUDIOCONVERT(stderr, "SRCAudioConverter::process %s output_frames_gen:%ld != outFrames:%ld inFrames:%ld srcdata.input_frames_used:%ld rn:%lu\n", 
//         f.name().toLatin1().constData(), srcdata.output_frames_gen, outFrames, inFrames, srcdata.input_frames_used, rn); 
//     #endif
//     
//     // If the number of frames read by the soundfile equals the input frames, go back.
//     // Otherwise we have reached the end of the file, so going back is useless since
//     //  there shouldn't be any further calls. 
//     if((long)rn == inFrames)
//     {
//       // Go back by the amount of unused frames.
//       sf_count_t seekn = inFrames - srcdata.input_frames_used;
//       if(seekn != 0)
//       {
//         #ifdef AUDIOCONVERT_DEBUG_PRC
//         DEBUG_AUDIOCONVERT(stderr, "SRCAudioConverter::process Seek-back by:%ld\n", seekn);
//         #endif
//         _sfCurFrame = f.seek(-seekn, SEEK_CUR);
//       }
//       else  
//         _sfCurFrame += rn;
//       
//       if(totalOutFrames == n)
//       {
//         // We got our desired number of output frames. Stop attempting.
//         break;
//       }
//       else  
//       {
//         // No point in continuing if on last attempt.
//         if(attempt == (attempts - 1))
//           break;
//           
//         #ifdef AUDIOCONVERT_DEBUG
//         DEBUG_AUDIOCONVERT(stderr, "SRCAudioConverter::process %s attempt:%d totalOutFrames:%ld != n:%d try again\n", f.name().toLatin1().constData(), attempt, totalOutFrames, n);
//         #endif
//         
//         // SRC didn't give us the number of frames we requested. 
//         // This can occasionally be radically different from the requested frames, or zero,
//         //  even when ample excess input frames are supplied.
//         // Move the src output pointer to a new position.
//         srcdata.data_out += srcdata.output_frames_gen * channel;
//         // Set new number of maximum out frames.
//         outFrames -= srcdata.output_frames_gen;
//         // Calculate the new number of file input frames required.
//         inFrames = (long)ceil(((double)outFrames / srcratio)) + inComp;
//         // Keep trying.
//         continue;
//       }  
//     }
//     else
//     {
//       _sfCurFrame += rn;
//       #ifdef AUDIOCONVERT_DEBUG
//       DEBUG_AUDIOCONVERT(stderr, "SRCAudioConverter::process %s rn:%zd != inFrames:%ld output_frames_gen:%ld outFrames:%ld srcdata.input_frames_used:%ld\n", 
//         f.name().toLatin1().constData(), rn, inFrames, srcdata.output_frames_gen, outFrames, srcdata.input_frames_used);
//       #endif
//       
//       // We've reached the end of the file. Convert the number of frames read.
//       //rn = (double)rn * srcratio + 1;   DELETETHIS 5
//       //rn = (long)floor((double)rn * srcratio); 
//       //if(rn > (size_t)outFrames)
//       //  rn = outFrames;  
//       // Stop attempting.
//       break;  
//     }
//   }
//   
//   // If we still didn't get the desired number of output frames.
//   if(totalOutFrames != n)
//   {
//     #ifdef AUDIOCONVERT_DEBUG
//     DEBUG_AUDIOCONVERT(stderr, "SRCAudioConverter::process %s totalOutFrames:%ld != n:%d\n", f.name().toLatin1().constData(), totalOutFrames, n);
//     #endif
//           
//     // Let's zero the rest of it.
//     long b = totalOutFrames * channel;
//     long e = n * channel;
//     for(long i = b; i < e; ++i)
//       outbuffer[i] = 0.0f;
//   }
//   
//   float*  poutbuf = outbuffer;
//   if(fchan == channel) 
//   {
//     if(overwrite)
//       for (int i = 0; i < n; ++i) 
//       {
//         for(int ch = 0; ch < channel; ++ch)
//           *(buffer[ch] + i) = *poutbuf++;
//       }
//     else
//       for(int i = 0; i < n; ++i) 
//       {
//         for(int ch = 0; ch < channel; ++ch)
//           *(buffer[ch] + i) += *poutbuf++;
//       }
//   }
//   else if((fchan == 2) && (channel == 1)) 
//   {
//     // stereo to mono
//     if(overwrite)
//       for(int i = 0; i < n; ++i)
//         *(buffer[0] + i) = poutbuf[i + i] + poutbuf[i + i + 1];
//     else  
//       for(int i = 0; i < n; ++i)
//         *(buffer[0] + i) += poutbuf[i + i] + poutbuf[i + i + 1];
//   }
//   else if((fchan == 1) && (channel == 2)) 
//   {
//     // mono to stereo
//     if(overwrite)
//       for(int i = 0; i < n; ++i) 
//       {
//         float data = *poutbuf++;
//         *(buffer[0]+i) = data;
//         *(buffer[1]+i) = data;
//       }
//     else  
//       for(int i = 0; i < n; ++i) 
//       {
//         float data = *poutbuf++;
//         *(buffer[0]+i) += data;
//         *(buffer[1]+i) += data;
//       }
//   }
//   else 
//   {
//     #ifdef AUDIOCONVERT_DEBUG
//     DEBUG_AUDIOCONVERT(stderr, "SRCAudioConverter::process Channel mismatch: source chans:%d -> dst chans:%d\n", fchan, channel);
//     #endif
//   }
//   
//   return _sfCurFrame;
// }
// sf_count_t SRCAudioConverter::process(MusECore::SndFileR& f, float** buffer, int channel, int n, bool overwrite)
//sf_count_t SRCAudioConverter::process(MusECore::SndFileR f, float** buffer, int channel, int n, bool overwrite)
// sf_count_t SRCAudioConverter::process(SndFile* f, float** buffer, int channel, int n, bool overwrite)
int SRCAudioConverter::process(SndFile* sf, SNDFILE* handle, sf_count_t pos,
                               float** buffer, int channels, int frames, bool overwrite)
{
  if(!_src_state)
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
  
//   unsigned fsrate = sf->samplerate();
  
  //bool resample   = src_state && ((unsigned)MusEGlobal::sampleRate != fsrate);   DELETETHIS 2
//  bool resample   = isValid() && ((unsigned)MusEGlobal::sampleRate != fsrate);  
  
//   if((MusEGlobal::sampleRate == 0) || (fsrate == 0))
//   if((MusEGlobal::sampleRate == 0) || (sf->samplerate() == 0))
  if((_systemSampleRate <= 0) || (sf->samplerate() <= 0))
  {  
    DEBUG_AUDIOCONVERT(stderr, "SRCAudioConverter::process Error: systemSampleRate or file samplerate <= 0!\n");
//     return _sfCurFrame;
    return 0;
  }  

  const int fchan           = sf->channels();
  const double sf_sr_ratio  = sf->sampleRateRatio();
  StretchList* stretch_list = sf->stretchList();



//   for(ciStretchListItem isli = stretch_list->begin(); isli != stretch_list->end(); ++isli)
//   {
//     const StretchListItem& sli = *isli;
//   }



  const MuseFrame_t unsquished_pos = stretch_list->unSquish(pos);
//   //const double stretchVal    = stretch_list->ratioAt(StretchListItem::StretchEvent, new_frame);
//   const double samplerateVal = stretch_list->ratioAt(StretchListItem::SamplerateEvent, unsquished_pos);
//   DEBUG_AUDIOCONVERT(stderr,
//     //"SRCAudioConverter::process: frame:%ld new_frame:%ld stretchRatio:%f samplerateRatio:%f\n", pos, new_frame, stretchVal, samplerateVal);
//     "SRCAudioConverter::process: frame:%ld new_frame:%ld samplerateRatio:%f\n", pos, new_frame, samplerateVal);
//
//   const double fin_samplerateRatio = sf_sr_ratio + samplerateVal - 1.0;
//
//   if(fin_samplerateRatio < 0.0001)
//   {
//     DEBUG_AUDIOCONVERT(stderr, "SRCAudioConverter::process Error: fin_samplerateRatio ratio is near zero!\n");
// //     return _sfCurFrame;
//     return 0;
//   }
//
//   const double inv_fin_samplerateRatio = 1.0 / fin_samplerateRatio;







//   const double srcratio = sf->sampleRateRatio();
//   const double inv_srcratio = 1.0 / srcratio;
  
//   SRC_DATA srcdata;

//   // Ratio is defined as output sample rate over input samplerate.
//   double srcratio = (double)MusEGlobal::sampleRate / (double)fsrate;
  
  // Extra input compensation.
  //sf_count_t inComp = 1;
  
  sf_count_t outSize    = frames * fchan;
  
  //long inSize = long(outSize * srcratio) + 1                      // From MusE-2 file converter. DELETETHIS3
  //long inSize = (long)floor(((double)outSize / srcratio));        // From simplesynth.
  //long inFrames = (long)floor(((double)outFrames / srcratio));    // From simplesynth.
  //long inFrames = (long)ceil(((double)outFrames / srcratio)) + inComp;    // From simplesynth.
  //long inFrames = (long)ceil(((double)outFrames * srcratio)) + inComp;    // From simplesynth.
//   sf_count_t inFrames = ceil(((double)outFrames * fin_samplerateRatio)) + inComp;    // From simplesynth.
  
  //long inSize = inFrames * fchan;
//   sf_count_t inSize = inFrames * fchan;
  
  // Start with buffers at expected sizes. We won't need anything larger than this, but add 4 for good luck.
//   float inbuffer[inSize + 4];
  float outbuffer[outSize];
      
  //sf_count_t rn           = 0;
  sf_count_t totalOutFrames = 0;
  
//   srcdata.data_in       = inbuffer;
  _srcdata.data_out      = outbuffer;

  const int debug_min_pos = 120;
  bool need_slice = true;

  double cur_ratio = 1.0;
  iStretchListItem next_isli = stretch_list->upper_bound(unsquished_pos);
  if(next_isli != stretch_list->begin())
  {
    --next_isli;
    cur_ratio = next_isli->second._samplerateRatio;
  }
  if(next_isli != stretch_list->end())
    next_isli = stretch_list->nextEvent(StretchListItem::SamplerateEvent, next_isli);
  double fin_samplerateRatio, inv_fin_samplerateRatio;

  MuseFrame_t cur_squished_pos;
  //MuseFrame_t cur_unsquished_pos;
  const MuseFrame_t end_squished_pos = pos + frames;
  //const MuseFrame_t end_unsquished_pos = stretch_list->unSquish(end_squished_pos);
  sf_count_t outFrames = frames - totalOutFrames;
  // Set some kind of limit on the number of attempts to completely fill the output buffer,
  //  in case something is really screwed up - we don't want to get stuck in a loop here.
  int attempts = 20;
  if(pos <= debug_min_pos)
  {
    DEBUG_AUDIOCONVERT(stderr, "SRCAudioConverter::process pos:%ld end_pos:%ld unsquished_pos:%ld\n",
                       pos, end_squished_pos, unsquished_pos);
  }
  while(totalOutFrames < frames && attempts > 0)
  {
    cur_squished_pos = pos + totalOutFrames;
    //cur_unsquished_pos = unsquished_pos + totalOutFrames;
    if(pos <= debug_min_pos)
    {
      DEBUG_AUDIOCONVERT(stderr, "   cur_squished_pos:%ld outFrames:%ld totalOutFrames:%ld\n",
                         cur_squished_pos, outFrames, totalOutFrames);
    }

    if(_needBuffer)
    {
// Oops! Caused crash with stereo.
//       _srcdata.input_frames = sf_readf_float(handle, _inbuffer, _inBufferSize);
      _srcdata.input_frames = sf_readf_float(handle, _inbuffer, SRC_IN_BUFFER_FRAMES);
      
      _srcdata.end_of_input = _srcdata.input_frames != SRC_IN_BUFFER_FRAMES;
      //DEBUG_AUDIOCONVERT(stderr, "SRCAudioConverter::process _needBuffer: _srcdata.input_frames:%ld\n", _srcdata.input_frames);
      // Zero any unread portion of the input buffer.
      for(int i = _srcdata.input_frames * fchan; i < _inBufferSize; ++i)
        *(_inbuffer + i) = 0.0f;
      _needBuffer = false;
    }
    else
    {
      _srcdata.input_frames = SRC_IN_BUFFER_FRAMES - _curInBufferFrame;
      //_srcdata.end_of_input = false;
      //DEBUG_AUDIOCONVERT(stderr, "SRCAudioConverter::process not _needBuffer: _srcdata.input_frames:%ld\n", _srcdata.input_frames);
    }
    _srcdata.data_in = _inbuffer + fchan * _curInBufferFrame;



    if(need_slice)
    {
      if(pos <= debug_min_pos)
      {
        DEBUG_AUDIOCONVERT(stderr, "   new slice: cur_ratio:%f\n", cur_ratio);
      }

      fin_samplerateRatio = sf_sr_ratio + cur_ratio - 1.0;
      if(fin_samplerateRatio < 0.000001)
      {
        DEBUG_AUDIOCONVERT(stderr, "SRCAudioConverter::process Error: fin_samplerateRatio ratio is near zero!\n");
        fin_samplerateRatio = 0.000001;
      }
      inv_fin_samplerateRatio = 1.0 / fin_samplerateRatio;

      if(next_isli != stretch_list->end() &&
         //next_isli->first >= cur_unsquished_pos &&
         //next_isli->first < end_unsquished_pos)
         next_isli->second._finSquishedFrame >= cur_squished_pos &&
         next_isli->second._finSquishedFrame < end_squished_pos)
      {
        cur_ratio = next_isli->second._samplerateRatio;
        //outFrames = next_isli->first - cur_unsquished_pos;
        outFrames = next_isli->second._finSquishedFrame - cur_squished_pos;
        if(pos <= debug_min_pos)
        {
          DEBUG_AUDIOCONVERT(stderr, "   new next_isli: next cur_ratio:%f outFrames:%ld\n", cur_ratio, outFrames);
        }
        next_isli = stretch_list->nextEvent(StretchListItem::SamplerateEvent, next_isli);
      }
      else
        outFrames = frames - totalOutFrames;

      //DEBUG_AUDIOCONVERT(stderr, "SRCAudioConverter::process need_slice: outFrames:%ld\n", outFrames);
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
    _srcdata.data_out      = outbuffer + fchan * totalOutFrames;



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
    ERROR_AUDIOCONVERT(stderr, "SRCAudioConverter::process %s Too may attempts to process! totalOutFrames:%ld frames:%d\n",
                       sf->name().toLatin1().constData(), totalOutFrames, frames);
  }

  // If we didn't get the desired number of output frames.
  if(totalOutFrames != frames)
  {
    DEBUG_AUDIOCONVERT(stderr, "SRCAudioConverter::process %s totalOutFrames:%ld != frames:%d\n", sf->name().toLatin1().constData(), totalOutFrames, frames);

    // Let's zero the rest of it.
    sf_count_t b = totalOutFrames * channels;
    sf_count_t e = frames * channels;
    for(sf_count_t i = b; i < e; ++i)
      outbuffer[i] = 0.0f;
  }







//   while(totalOutFrames < outFrames)
//   {
//     if(_srcdata.input_frames_used >= SRC_IN_BUFFER_FRAMES)
//       //_srcdata.input_frames_used = _srcdata.input_frames;
//       _srcdata.input_frames_used = 0;
//
//     if(_srcdata.input_frames_used == 0)
//     {
//       _srcdata.input_frames = sf_readf_float(handle, _inbuffer, _inBufferSize);
//       _srcdata.end_of_input = _srcdata.input_frames != SRC_IN_BUFFER_FRAMES;
//     }
//     else
//     {
//       //_srcdata.input_frames = SRC_IN_BUFFER_FRAMES - _srcdata.input_frames_used;
//       _srcdata.input_frames -= _srcdata.input_frames_used;
//       _srcdata.end_of_input = false;
//     }
//
//     //_srcdata.data_in = _inbuffer + fchan * _srcdata.input_frames_used;
//     _srcdata.data_in += fchan * _srcdata.input_frames_used;
//
//     _srcdata.output_frames = outFrames;
//     _srcdata.src_ratio     = inv_fin_samplerateRatio;
//
//     if(_srcdata.input_frames == 0)
//       break;
//
//     int srcerr = src_process(_src_state, &_srcdata);
//     if(srcerr != 0)
//     {
//       ERROR_AUDIOCONVERT(stderr, "\nSRCAudioConverter::process SampleRate converter process failed: %s\n", src_strerror(srcerr));
//       return 0;
//     }
//     totalOutFrames += _srcdata.output_frames_gen;
//   }




/*



  // Set some kind of limit on the number of attempts to completely fill the output buffer, 
  //  in case something is really screwed up - we don't want to get stuck in a loop here.
  int attempts = 10;
  for(int attempt = 0; attempt < attempts; ++attempt)
  {
    //rn = f->readDirect(inbuffer, inFrames);
//     rn = sf_readf_float(handle,  inbuffer, inFrames);
    rn = sf_readf_float(handle,  _inbuffer, SRC_IN_BUFFER_FRAMES);

    // convert
    _srcdata.input_frames  = rn;
    _srcdata.output_frames = outFrames;
//     _srcdata.end_of_input  = (rn != inFrames);
    _srcdata.end_of_input  = (rn != SRC_IN_BUFFER_FRAMES);
    // Ratio is defined as output sample rate over input samplerate.
    _srcdata.src_ratio     = inv_fin_samplerateRatio;

    //#ifdef AUDIOCONVERT_DEBUG_PRC DELETETHIS or comment it in, or maybe add an additional if (heavyDebugMsg)?
    //DEBUG_AUDIOCONVERT(stderr, "AudioConverter::process attempt:%d inFrames:%ld outFrames:%ld rn:%d data in:%p out:%p", 
    //  attempt, inFrames, outFrames, rn, srcdata.data_in, srcdata.data_out);
    //#endif
    
    int srcerr = src_process(_src_state, &_srcdata);
    if(srcerr != 0)
    {
      ERROR_AUDIOCONVERT(stderr, "\nSRCAudioConverter::process SampleRate converter process failed: %s\n", src_strerror(srcerr));
//       return _sfCurFrame += rn;
      return rn;
    }
    
    totalOutFrames += _srcdata.output_frames_gen;

    //#ifdef AUDIOCONVERT_DEBUG_PRC DELETETHIS or comment in or heavyDebugMsg
    //DEBUG_AUDIOCONVERT(stderr, " frames used in:%ld out:%ld totalOutFrames:%ld data in:%p out:%p\n", srcdata.input_frames_used, srcdata.output_frames_gen, totalOutFrames, srcdata.data_in, srcdata.data_out);
    //#endif
    
    if(_srcdata.output_frames_gen != outFrames)
      DEBUG_AUDIOCONVERT(stderr, "SRCAudioConverter::process %s output_frames_gen:%ld != outFrames:%ld inFrames:%ld srcdata.input_frames_used:%ld rn:%lu\n",
        sf->name().toLatin1().constData(), _srcdata.output_frames_gen, outFrames, inFrames, _srcdata.input_frames_used, rn);
    
    // If the number of frames read by the soundfile equals the input frames, go back.
    // Otherwise we have reached the end of the file, so going back is useless since
    //  there shouldn't be any further calls. 
    //if((long)rn == inFrames)
//     if(rn == inFrames)
    if(rn == SRC_IN_BUFFER_FRAMES)
    {
      // Go back by the amount of unused frames.
      sf_count_t seekn = inFrames - _srcdata.input_frames_used;
      if(seekn != 0)
      {
        DEBUG_AUDIOCONVERT(stderr, "SRCAudioConverter::process Seek-back by:%ld\n", seekn);
//         _sfCurFrame = f->seek(-seekn, SEEK_CUR);
        _sfCurFrame = sf_seek(handle, -seekn, SEEK_CUR);
      }
      else  
        _sfCurFrame += rn;
      
      if(totalOutFrames == frames)
      {
        // We got our desired number of output frames. Stop attempting.
        break;
      }
      else  
      {
        // No point in continuing if on last attempt.
        if(attempt == (attempts - 1))
          break;
          
        DEBUG_AUDIOCONVERT(stderr, "SRCAudioConverter::process %s attempt:%d totalOutFrames:%ld != frames:%d try again\n",
                           sf->name().toLatin1().constData(), attempt, totalOutFrames, frames);
        
        // SRC didn't give us the number of frames we requested. 
        // This can occasionally be radically different from the requested frames, or zero,
        //  even when ample excess input frames are supplied.
        // Move the src output pointer to a new position.
        _srcdata.data_out += _srcdata.output_frames_gen * channels;
        // Set new number of maximum out frames.
        outFrames -= _srcdata.output_frames_gen;
        // Calculate the new number of file input frames required.
//         inFrames = (long)ceil(((double)outFrames / srcratio)) + inComp;
//         inFrames = (long)ceil(((double)outFrames * srcratio)) + inComp;
        inFrames = ceil(((double)outFrames * fin_samplerateRatio)) + inComp;
        // Keep trying.
        continue;
      }  
    }
    else
    {
      _sfCurFrame += rn;
      DEBUG_AUDIOCONVERT(stderr, "SRCAudioConverter::process %s rn:%zd != inFrames:%ld output_frames_gen:%ld outFrames:%ld srcdata.input_frames_used:%ld\n", 
        sf->name().toLatin1().constData(), rn, inFrames, _srcdata.output_frames_gen, outFrames, _srcdata.input_frames_used);
      
      // We've reached the end of the file. Convert the number of frames read.
      //rn = (double)rn * srcratio + 1;   DELETETHIS 5
      //rn = (long)floor((double)rn * srcratio); 
      //if(rn > (size_t)outFrames)
      //  rn = outFrames;  
      // Stop attempting.
      break;  
    }
  }
  
  // If we still didn't get the desired number of output frames.
  if(totalOutFrames != frames)
  {
    DEBUG_AUDIOCONVERT(stderr, "SRCAudioConverter::process %s totalOutFrames:%ld != frames:%d\n", sf->name().toLatin1().constData(), totalOutFrames, frames);
          
    // Let's zero the rest of it.
//     long b = totalOutFrames * channel;
//     long e = n * channel;
//     for(long i = b; i < e; ++i)
//       outbuffer[i] = 0.0f;
    sf_count_t b = totalOutFrames * channels;
    sf_count_t e = frames * channels;
    for(sf_count_t i = b; i < e; ++i)
      outbuffer[i] = 0.0f;
  }*/


  float*  poutbuf = outbuffer;
  if(fchan == channels)
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
  else if((fchan == 2) && (channels == 1))
  {
    // stereo to mono
    if(overwrite)
      for(sf_count_t i = 0; i < frames; ++i)
        *(buffer[0] + i) = poutbuf[i + i] + poutbuf[i + i + 1];
    else  
      for(sf_count_t i = 0; i < frames; ++i)
        *(buffer[0] + i) += poutbuf[i + i] + poutbuf[i + i + 1];
  }
  else if((fchan == 1) && (channels == 2))
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
    DEBUG_AUDIOCONVERT(stderr, "SRCAudioConverter::process Channel mismatch: source chans:%d -> dst chans:%d\n", fchan, channels);
  }
  
//   return _sfCurFrame;
  return frames;
}

// void SRCAudioConverter::read(Xml&)
// {
//   
// }
// 
// void SRCAudioConverter::write(int, Xml&) const
// {
//   
// }
  

//---------------------------------------------------------
//   SRCAudioConverterSettings
//---------------------------------------------------------

void SRCAudioConverterOptions::write(int level, Xml& xml) const
      {
      //xml.tag(level++, "settings mode=\"%d\" useSettings=\"%d\"", _mode, _useSettings);
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
                        //if (tag == "mode")
                        //      _mode = xml.s2().toInt();
                              //_mode = xml.s2();
                        //else 
                        //  if (tag == "useSettings")
                        //      _useSettings = xml.s2().toInt();
                        //else
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
// const SRCAudioConverterOptions SRCAudioConverterSettings::defaultOfflineOptions(
//   false, AudioConverterSettings::OfflineMode, SRC_SINC_BEST_QUALITY);
// const SRCAudioConverterOptions SRCAudioConverterSettings::defaultRealtimeOptions(
//   false, AudioConverterSettings::RealtimeMode, SRC_SINC_MEDIUM_QUALITY);
// const SRCAudioConverterOptions SRCAudioConverterSettings::defaultGuiOptions(
//   false, AudioConverterSettings::GuiMode, SRC_SINC_FASTEST);
const SRCAudioConverterOptions SRCAudioConverterOptions::defaultOfflineOptions(
  false, AudioConverterSettings::OfflineMode, SRC_SINC_BEST_QUALITY);
const SRCAudioConverterOptions SRCAudioConverterOptions::defaultRealtimeOptions(
  false, AudioConverterSettings::RealtimeMode, SRC_SINC_MEDIUM_QUALITY);
const SRCAudioConverterOptions SRCAudioConverterOptions::defaultGuiOptions(
  false, AudioConverterSettings::GuiMode, SRC_SINC_FASTEST);
    
SRCAudioConverterSettings::SRCAudioConverterSettings(
  //int converterID, 
  bool isLocal) 
  //: AudioConverterSettings(converterID)
  : AudioConverterSettings(descriptor._ID)
{ 
  initOptions(isLocal); 
}
      
// MusECore::AudioConverterSettings* SRCAudioConverterSettings::createSettings(bool isLocal)
// {
//   return new MusECore::SRCAudioConverterSettings(isLocal);
// }

void SRCAudioConverterSettings::assign(const AudioConverterSettings& other)
{
  const SRCAudioConverterSettings& src_other = 
    (const SRCAudioConverterSettings&)other;
  _offlineOptions  = src_other._offlineOptions;
  _realtimeOptions = src_other._realtimeOptions;
  _guiOptions      = src_other._guiOptions;
}

// bool SRCAudioConverterSettings::isSet(int mode) const 
// { 
//   if(mode & ~(AudioConverterSettings::OfflineMode | 
//               AudioConverterSettings::RealtimeMode | 
//               AudioConverterSettings::GuiMode))
//     fprintf(stderr, "SRCAudioConverterSettings::isSet() Warning: Unknown modes included:%d\n", mode);
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

int SRCAudioConverterSettings::executeUI(int mode, QWidget* parent, bool isLocal) 
{
  MusEGui::SRCResamplerSettingsDialog dlg(mode, parent, this, isLocal);
  return dlg.exec(); 
}

void SRCAudioConverterSettings::write(int level, Xml& xml) const
{
//       if(!useSettings())
//         return;
// 
//       xml.tag(level++, "SRCSettings");
//       //xml.tag(level++, "audioConverterSettings id=\"%d\" name=\"%s\"", descriptor._ID, descriptor._name);
//       
//       if(_offlineOptions._converterType != -1)
//         xml.intTag(level, "offlineConverterType", _offlineOptions._converterType);
//       if(_realtimeOptions._converterType != -1)
//       xml.intTag(level, "realtimeConverterType", _realtimeOptions._converterType);
//       
//       xml.tag(--level, "/SRCSettings");
// //       xml.tag(--level, "/audioConverterSettings");
      
      
  const bool use_off = !(_offlineOptions == SRCAudioConverterOptions::defaultOfflineOptions);
  const bool use_rt  = !(_realtimeOptions == SRCAudioConverterOptions::defaultRealtimeOptions);
  const bool use_gui = !(_guiOptions == SRCAudioConverterOptions::defaultGuiOptions);

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

// void SRCAudioConverterSettings::readItem(Xml& xml)
// {
//   //int id = -1;
//   int mode = -1;
//   //QString mode;
//   //bool useSettings;
//   for (;;) {
//       Xml::Token token = xml.parse();
//       const QString& tag = xml.s1();
//       switch (token) {
//             case Xml::Error:
//             case Xml::End:
//                   return;
//             case Xml::TagStart:
// //                   if (tag == "offline")
// //                         _offlineOptions.read(xml);
// //                   else if (tag == "realtime")
// //                         _realtimeOptions.read(xml);
// //                   else if (tag == "gui")
// //                         _guiOptions.read(xml);
// //                   if (tag == "SRCSettings")
// //                   {
// //                     if(mode == "offline")
// //                         _offlineOptions.read(xml);
// //                     else if(mode == "realtime")
// //                         _realtimeOptions.read(xml);
// //                     else if(mode == "gui")
// //                         _guiOptions.read(xml);
// //                   }
//                   if (tag == "settings")
//                   {
//                     if(mode != -1)
//                     {
//                       switch(mode)
//                       {
//                         case AudioConverterSettings::OfflineMode:
//                           _offlineOptions.read(xml);
//                         break;
// 
//                         case AudioConverterSettings::RealtimeMode:
//                           _realtimeOptions.read(xml);
//                         break;
//                         
//                         case AudioConverterSettings::GuiMode:
//                           _guiOptions.read(xml);
//                         break;
//                       }
//                     }
//                   }
//                   else
//                         xml.unknown("SRCSettings");
//                   break;
//             case Xml::Attribut:
//                   //if (tag == "id")
//                   //      id = xml.s2().toInt();
//                   //else 
//                     if (tag == "mode")
//                         mode = xml.s2().toInt();
//                         //mode = xml.s2();
//                   //else if (tag == "useSettings")
//                   //     useSettings = xml.s2().toInt();
//                   else
//                         fprintf(stderr, "SRCSettings unknown tag %s\n", tag.toLatin1().constData());
//                   break;
//             case Xml::TagEnd:
//                   if (tag == "SRCSettings") {
//                         return;
//                         }
//             default:
//                   break;
//             }
//       }
// }

void SRCAudioConverterSettings::read(Xml& xml)
{
  //int id = -1;
  int mode = -1;
  //QString mode;
  //bool useSettings = false;
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
              
//                   if (tag == "SRCSettings")
//                   {
// //                     if(mode == "offline")
// //                         _offlineOptions.read(xml);
// //                     else if(mode == "realtime")
// //                         _realtimeOptions.read(xml);
// //                     else if(mode == "gui")
// //                         _guiOptions.read(xml);
//                     readItem(xml);
//                   }
                  
                  
//                   if (tag == "converterSettings")
//                   {
//                     if(mode != -1)
//                     {
//                       switch(mode)
//                       {
//                         case AudioConverterSettings::OfflineMode:
//                           _offlineOptions.read(xml);
//                         break;
// 
//                         case AudioConverterSettings::RealtimeMode:
//                           _realtimeOptions.read(xml);
//                         break;
//                         
//                         case AudioConverterSettings::GuiMode:
//                           _guiOptions.read(xml);
//                         break;
//                       }
//                     }
//                   }
                  
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
                        //xml.unknown("audioConverterSetting");
                        xml.unknown("settings");
                  break;
            case Xml::Attribut:
                  //if (tag == "id")
                  //      id = xml.s2().toInt();
                  //else 
                    if (tag == "mode")
                        mode = xml.s2().toInt();
                        //mode = xml.s2();
                  //else if (tag == "useSettings")
                  //      useSettings = xml.s2().toInt();
                  else
                        //fprintf(stderr, "audioConverterSetting unknown tag %s\n", tag.toLatin1().constData());
                        fprintf(stderr, "settings unknown tag %s\n", tag.toLatin1().constData());
                  break;
            case Xml::TagEnd:
                  //if (tag == "audioConverterSetting") {
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

//   if(useDefaultSettings->isVisible())
//   {
//     useDefaultSettings->blockSignals(true);
//     useDefaultSettings->setChecked(!_options->_useSettings);
//     useDefaultSettings->blockSignals(false);
//   }
  
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
  
  //typeGroup->setEnabled(!useDefaultSettings->isVisible() || _options->_useSettings);
}

void SRCResamplerSettingsDialog::buttonClicked(int idx)
{
  switch(idx)
  {
    case DefaultsButtonId:
      OKButton->setEnabled(true);
      //setControls();
      //typeGroup->setEnabled(_options->_useSettings);
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
