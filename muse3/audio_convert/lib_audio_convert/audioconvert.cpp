//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: audioconvert.cpp,v 1.1.1.1 2009/12/28 16:07:33 terminator356 Exp $
//
//  (C) Copyright 1999-2009 Werner Schweer (ws@seh.de)
//
//  Audio converter module created by Tim
//  (C) Copyright 2009-2016 Tim E. Real (terminator356 A T sourceforge D O T net)
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

#include <math.h>
#include <stdio.h>

// #include "src_converter.h"
// #include "zita_resampler_converter.h"
// #include "rubberband_converter.h"

#include "wave.h"
#include "globals.h"
#include "audioconvert.h"
#include "time_stretch.h"
#include "xml.h"

#define ERROR_AUDIOCONVERT(dev, format, args...)  fprintf(dev, format, ##args)

// REMOVE Tim. samplerate. Enabled.
// For debugging output: Uncomment the fprintf section.
#define DEBUG_AUDIOCONVERT(dev, format, args...)  fprintf(dev, format, ##args)

namespace MusECore {

// Static variables:
//AudioConverterSettingsGroup AudioConverter::defaultSettings;
//const char* AudioConverterSettings::modeNames[] = { "offline", "realtime", "gui" };

  
//---------------------------------------------------------
//   AudioConverter
//---------------------------------------------------------

AudioConverter::AudioConverter()
{
  DEBUG_AUDIOCONVERT(stderr, "AudioConverter::AudioConverter this:%p\n", this);

  _refCount = 1;
  _channels = 0;
//   _sampleRateRatio = 1.0;
//   _stretchRatio = 1.0;
//   _pitchRatio = 1.0;
  _sfCurFrame = 0;
}

AudioConverter::~AudioConverter()
{
  DEBUG_AUDIOCONVERT(stderr, "AudioConverter::~AudioConverter this:%p\n", this);
}

// // Static.
// AudioConverterHandle AudioConverter::create(int /*converterID*/, int /*channels*/)
// {
// //   switch(converterID)
// //   {
// //     case SRCResampler:
// //       return new SRCAudioConverter(channels, SRC_SINC_MEDIUM_QUALITY);
// //       
// // //#ifdef ZITA_RESAMPLER_SUPPORT
// //     case ZitaResampler:
// //         // Todo...
// //       return 0;
// // //#endif
// // 
// // //#ifdef RUBBERBAND_SUPPORT
// //     case RubberBand:
// //       return new RubberBandAudioConverter(channels, RubberBand::RubberBandStretcher::OptionProcessRealTime |
// //                                              RubberBand::RubberBandStretcher::OptionThreadingAuto);
// // //#endif
// //       
// // //#ifdef SOUNDTOUCH_SUPPORT
// //     case SoundTouch:
// //         // Todo...
// //       return 0;
// // //#endif
// //       
// //     default:
// //       //DEBUG_AUDIOCONVERT(stderr, "AudioConverter::create: unavailable or unknown converter ID:%d\n", converterID);
// //       DEBUG_AUDIOCONVERT(stderr, "AudioConverter::create: unknown converter ID:%d\n", converterID);
// //     break;
// //   }
//   return 0;
// }

AudioConverterHandle AudioConverter::reference()
{
  _refCount += 1;
  DEBUG_AUDIOCONVERT(stderr, "AudioConverter::reference this:%p current refcount:%d\n", this, _refCount);
  return this;
}

// Static.
AudioConverterHandle AudioConverter::release(AudioConverter* cv)
{
  if(!cv)
    return 0;
  cv->_refCount -= 1;
  DEBUG_AUDIOCONVERT(stderr, "AudioConverter::release converter:%p current refcount:%d\n", cv, cv->_refCount);
  if(cv->_refCount <= 0)
  {
    DEBUG_AUDIOCONVERT(stderr, "AudioConverter::release deleting converter:%p\n", cv);
    delete cv;
    cv = 0;
  }
  return cv;  
}

// // Static.
// int AudioConverter::availableConverters()
// {
//   int res = SRCResampler;
//   
// #ifdef ZITA_RESAMPLER_SUPPORT
//   res |= ZitaResampler;
// #endif
// 
// #ifdef RUBBERBAND_SUPPORT
//   res |= RubberBand;
// #endif
//       
// #ifdef SOUNDTOUCH_SUPPORT
//   res |= SoundTouch;
// #endif
//       
//   return res;
// }

// // Static.
// bool AudioConverter::isValidID(int converterID)
// {
//   switch(converterID)
//   {
//     case SRCResampler:
//       
// //#ifdef ZITA_RESAMPLER_SUPPORT
//     case ZitaResampler:
// //#endif
// 
// //#ifdef RUBBERBAND_SUPPORT
//     case RubberBand:
// //#endif
//       
// //#ifdef SOUNDTOUCH_SUPPORT
//     //case SoundTouch:  // TODO
// //#endif
//       return true;
//       
//     default:
//       //DEBUG_AUDIOCONVERT(stderr, "AudioConverter::isValidConverterID: unavailable or unknown converter ID:%d\n", converterID);
//       DEBUG_AUDIOCONVERT(stderr, "AudioConverter::isValidConverterID: unknown converter ID:%d\n", converterID);
//     break;
//   }
//   return false;
// }

// // Static
// void AudioConverter::readDefaults(Xml& xml)
// {
//       for (;;) {
//             Xml::Token token = xml.parse();
//             const QString& tag = xml.s1();
//             switch (token) {
//                   case Xml::Error:
//                   case Xml::End:
//                         return;
//                   case Xml::TagStart:
//                         if (tag == "SRCSettings")
//                               SRCAudioConverter::defaultSettings.read(xml);
//                         else if (tag == "zitaResamplerSettings")
//                               ZitaResamplerAudioConverter::defaultSettings.read(xml);
//                         else if (tag == "rubberbandSettings")
//                               RubberBandAudioConverter::defaultSettings.read(xml);
//                         //else if (tag == "soundTouchSettings")
//                         //      SoundTouchAudioConverter::defaultSettings.read(xml);  // TODO
//                         else
//                               xml.unknown("audioConverterSettings");
//                         break;
//                   case Xml::Attribut:
//                               fprintf(stderr, "audioConverterSettings unknown tag %s\n", tag.toLatin1().constData());
//                         break;
//                   case Xml::TagEnd:
//                         if (tag == "audioConverterSettings") {
//                               return;
//                               }
//                   default:
//                         break;
//                   }
//             }
// }
// 
// // Static
// void AudioConverter::writeDefaults(int level, Xml& xml)
// {
//   if(SRCAudioConverter::defaultSettings.isSet()
//      || ZitaResamplerAudioConverter::defaultSettings.isSet()
//      || RubberBandAudioConverter::defaultSettings.isSet()
//      //|| SoundTouchAudioConverter::defaultSettings.isSet()  // TODO
//     )
//   {  
//     //xml.tag(level++, "audioConverterDefaultSettings");
//     xml.tag(level++, "audioConverterSettings");
//   
//     SRCAudioConverter::defaultSettings.write(level, xml);
//       
// //#ifdef ZITA_RESAMPLER_SUPPORT
//     ZitaResamplerAudioConverter::defaultSettings.write(level, xml);
// //#endif
// 
// //#ifdef RUBBERBAND_SUPPORT
//     RubberBandAudioConverter::defaultSettings.write(level, xml);
// //#endif
//       
// //#ifdef SOUNDTOUCH_SUPPORT
//     //SoundTouchAudioConverter::defaultSettings.write(level, xml);  // TODO
// //#endif
//     
//     //xml.tag(--level, "/audioConverterDefaultSettings");
//     xml.tag(--level, "/audioConverterSettings");
//   }
// }



// AudioConverterDescriptor* AudioConverter::converterDescriptor(int converterID)
// {
//   switch(converterID)
//   {
//     case SRCResampler:
//       return SRCAudioConverter::name();
//       
// #ifdef ZITA_RESAMPLER_SUPPORT
//     case ZitaResampler:
//       return ZitaResamplerAudioConverter::name();
// #endif
// 
// #ifdef RUBBERBAND_SUPPORT
//     case RubberBand:
//       return RubberBandAudioConverter::name();
// #endif
//       
// #ifdef SOUNDTOUCH_SUPPORT
//     case SoundTouch:
//       return SoundTouchAudioConverter::name();
// #endif
//       
//     default:
//       DEBUG_AUDIOCONVERT(stderr, "AudioConverter::converterDescriptor: unavailable or unknown converter ID:%d\n", converterID);
//     break;
//   }
//   return 0;
// }
// 
// 
// 
// 
// // Static.
// const char* AudioConverter::name(int converterID)
// {
//   switch(converterID)
//   {
//     case SRCResampler:
//       return SRCAudioConverter::name();
//       
// #ifdef ZITA_RESAMPLER_SUPPORT
//     case ZitaResampler:
//       return ZitaResamplerAudioConverter::name();
// #endif
// 
// #ifdef RUBBERBAND_SUPPORT
//     case RubberBand:
//       return RubberBandAudioConverter::name();
// #endif
//       
// #ifdef SOUNDTOUCH_SUPPORT
//     case SoundTouch:
//       return SoundTouchAudioConverter::name();
// #endif
//       
//     default:
//       DEBUG_AUDIOCONVERT(stderr, "AudioConverter::name: unavailable or unknown converter ID:%d\n", converterID);
//     break;
//   }
//   return 0;
// }
// 
// // Static.
// int AudioConverter::capabilities(int converterID)
// {
//   switch(converterID)
//   {
//     case SRCResampler:
//       return SRCAudioConverter::capabilities();
//       
// #ifdef ZITA_RESAMPLER_SUPPORT
//     case ZitaResampler:
//       return ZitaResamplerAudioConverter::capabilities();
// #endif
// 
// #ifdef RUBBERBAND_SUPPORT
//     case RubberBand:
//       return RubberBandAudioConverter::capabilities();
// #endif
//       
// #ifdef SOUNDTOUCH_SUPPORT
//     case SoundTouch:
//       return SoundTouchAudioConverter::capabilities();
// #endif
//       
//     default:
//       DEBUG_AUDIOCONVERT(stderr, "AudioConverter::capabilities: unavailable or unknown converter ID:%d\n", converterID);
//     break;
//   }
//   return 0;
// }
// 
// // Static.
// int AudioConverter::maxChannels(int converterID)
// {
//   switch(converterID)
//   {
//     case SRCResampler:
//       return SRCAudioConverter::maxChannels();
//       
// #ifdef ZITA_RESAMPLER_SUPPORT
//     case ZitaResampler:
//       return ZitaResamplerAudioConverter::maxChannels();
// #endif
// 
// #ifdef RUBBERBAND_SUPPORT
//     case RubberBand:
//       return RubberBandAudioConverter::maxChannels();
// #endif
//       
// #ifdef SOUNDTOUCH_SUPPORT
//     case SoundTouch:
//       return SoundTouchAudioConverter::maxChannels();
// #endif
//       
//     default:
//       DEBUG_AUDIOCONVERT(stderr, "AudioConverter::maxChannels: unavailable or unknown converter ID:%d\n", converterID);
//     break;
//   }
//   return 0;
// }


// sf_count_t AudioConverter::seekAudio(MusECore::SndFileR& f, sf_count_t pos)
//sf_count_t AudioConverter::seekAudio(MusECore::SndFileR f, sf_count_t pos)
// The offset is the offset into the sound file and is NOT converted.
sf_count_t AudioConverter::seekAudio(SndFile* f, sf_count_t frame, int offset)
{
//   if(f->isNull())
//     return _sfCurFrame;
  
//   DEBUG_AUDIOCONVERT(stderr, "AudioConverter::seekAudio %s audConv:%p sfCurFrame:%ld pos:%lu\n", 
//           f.name().toLatin1().constData(), this, _sfCurFrame, pos);
  
//   //off_t frame     = offset;  // _spos is added before the call.
// //   const unsigned fsrate = f.samplerate();
// //   const bool resample   = isValid() && ((unsigned)MusEGlobal::sampleRate != fsrate);  
//   const bool resample   = isValid() && f.sampleRateDiffers();  
  const sf_count_t smps = f->samples();
  
  if(frame < 0)
    frame = 0;
  
//   // No resampling needed?
//   if(!resample)
//   {
//     // Sample rates are the same. Just a regular seek, no conversion.
// //     _sfCurFrame = f.seek(offset, 0);
// //     return _sfCurFrame;
//     
//     if(pos > smps)
//       pos = smps;
//     _sfCurFrame = f.seek(pos, SEEK_SET);
//     return _sfCurFrame;
//   }
//   
// //   const double srcratio = (double)fsrate / (double)MusEGlobal::sampleRate;
// //   //long inSize = long((double)frames * _src_ratio) + 1     // From MusE-2 file converter. DELETETHIS ???
// // //   off_t newfr = (off_t)floor(((double)frame * srcratio));    // From simplesynth.
// //   sf_count_t newfr = (sf_count_t)floor(((double)pos * srcratio));    // From simplesynth.
// 
  sf_count_t newfr = f->convertPosition(frame);
  
  // Do not convert the offset.
  newfr += offset;
  if(newfr < 0)
    newfr = 0;
  
  // Clamp it at 'one past the end' in other words EOF.
  if(newfr > smps)
    newfr = smps;
  
//   _sfCurFrame = f.seek(newfr, 0);
  _sfCurFrame = f->seek(newfr, SEEK_SET);
  //_sfCurFrame = f.seek(pos, SEEK_SET);
  
  // Added by Tim. p3.3.17 DELETETHIS 3 or comment it in
  DEBUG_AUDIOCONVERT(stderr, "AudioConverter::process Seek pos:%ld converted to frame:%ld sfCurFrame:%ld\n", frame, newfr, _sfCurFrame);
  
  // Reset the converter. Its current state is meaningless now.
  reset();
  
  return _sfCurFrame;
}

// This one...
// // sf_count_t AudioConverter::readAudio(MusECore::SndFileR& f, unsigned offset, float** buffer, int channel, int n, bool /*doSeek*/, bool overwrite)
// //sf_count_t AudioConverter::readAudio(MusECore::SndFileR f, unsigned offset, float** buffer, int channel, int n, bool /*doSeek*/, bool overwrite)
// sf_count_t AudioConverter::readAudio(SndFile* f, unsigned /*offset*/, float** buffer, int channel, int n, bool /*doSeek*/, bool overwrite)
// {
// //   if(f.isNull())
// //     return _sfCurFrame;
//   
//   DEBUG_AUDIOCONVERT(stderr, "AudioConverter::process %s audConv:%p sfCurFrame:%ld offset:%u channel:%d fchan:%d n:%d\n", 
//           f->name().toLatin1().constData(), this, _sfCurFrame, offset, channel, f->channels(), n);
//   
// // REMOVE Tim. samplerate. Removed.   
// //   off_t frame     = offset;  // _spos is added before the call.
// //   unsigned fsrate = f.samplerate();
// //   bool resample   = isValid() && ((unsigned)MusEGlobal::sampleRate != fsrate);  
// //   
// //   // No resampling needed?
// //   if(!resample)
// //   {
// //     // Sample rates are the same. Just a regular seek + read, no conversion.
// // // REMOVE Tim. samplerate. Removed. Should avoid seeking during read - just let it roll on its own. TODO: REINSTATE?
// // //     _sfCurFrame = f.seek(frame, 0);
// //     return _sfCurFrame + f.read(channel, buffer, n, overwrite);
// //   }
// //   
// //   // Is a 'transport' seek requested? (Not to be requested with every read! Should only be for 'first read' seeks, or positional 'transport' seeks.)
// //   // Due to the support of sound file references in MusE, seek must ALWAYS be done before read, as before,
// //   //  except now we alter the seek position if sample rate conversion is being used and remember the seek positions. 
// //   if(doSeek)
// //   {
// //     // Sample rates are different. Seek to a calculated 'sample rate ratio factored' position.
// //     
// //     double srcratio = (double)fsrate / (double)MusEGlobal::sampleRate;
// //     //long inSize = long((double)frames * _src_ratio) + 1     // From MusE-2 file converter. DELETETHIS ???
// //     off_t newfr = (off_t)floor(((double)frame * srcratio));    // From simplesynth.
// //   
// //     _sfCurFrame = f.seek(newfr, 0);
// //     
// //     // Added by Tim. p3.3.17 DELETETHIS 3 or comment it in
// //     //DEBUG_AUDIOCONVERT(stderr, "AudioConverter::process Seek frame:%ld converted to frame:%ld sfCurFrame:%ld\n", frame, newfr, sfCurFrame);
// //     
// //     // Reset the converter. Its current state is meaningless now.
// //     reset();
// //   }
// //   else  
// //   {
// //     // No seek requested. 
// //     // Added by Tim. p3.3.17 DELETETHIS 3 or comment it in
// //     //DEBUG_AUDIOCONVERT(stderr, "AudioConverter::process No 'transport' seek, rates different. Seeking to sfCurFrame:%ld\n", sfCurFrame);
// //     
// //     // Sample rates are different. We can't just tell seek to go to an absolute calculated position, 
// //     //  since the last position can vary - it might not be what the calculated position is. 
// //     // We must use the last position left by SRC conversion, ie. let the file position progress on its own.
// // // REMOVE Tim. samplerate. Removed. Should avoid seeking during read - just let it roll on its own. TODO: REINSTATE?      
// // //     _sfCurFrame = f.seek(_sfCurFrame, 0);
// //   }
//   
//   /* DELETETHIS 5
//   int   fchan              = f.channels();
//   long  outFrames          = n;  
//   long  outSize            = outFrames * fchan;
//   float outbuffer[outSize];
//   */
// 
// 
// 
//   
//   // DELETETHIS 4
//   //sfCurFrame = process(f, sfCurFrame, offset, &outbuffer[0], channel, n);
// //  sfCurFrame = process(f, sfCurFrame, outbuffer, channel, n);
//   //sfCurFrame = process(f, sfCurFrame, buffer, channel, n, overwrite);
//   _sfCurFrame = process(f, buffer, channel, n, overwrite);
// 
// 
// 
//   
//   /* DELETETHIS 58 (whoa!)
//   float*  poutbuf = &outbuffer[0];
//   if(fchan == channel) 
//   {
//     if(overwrite)
//       //for (size_t i = 0; i < rn; ++i) 
//       for (int i = 0; i < n; ++i) 
//       {
//         for(int ch = 0; ch < channel; ++ch)
//           *(buffer[ch] + i) = *poutbuf++;
//       }
//     else
//       //for(size_t i = 0; i < rn; ++i) 
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
//       //for(size_t i = 0; i < rn; ++i)
//       for(int i = 0; i < n; ++i)
//         *(buffer[0] + i) = poutbuf[i + i] + poutbuf[i + i + 1];
//     else  
//       //for(size_t i = 0; i < rn; ++i)
//       for(int i = 0; i < n; ++i)
//         *(buffer[0] + i) += poutbuf[i + i] + poutbuf[i + i + 1];
//   }
//   else if((fchan == 1) && (channel == 2)) 
//   {
//     // mono to stereo
//     if(overwrite)
//       //for(size_t i = 0; i < rn; ++i) 
//       for(int i = 0; i < n; ++i) 
//       {
//         float data = *poutbuf++;
//         *(buffer[0]+i) = data;
//         *(buffer[1]+i) = data;
//       }
//     else  
//       //for(size_t i = 0; i < rn; ++i) 
//       for(int i = 0; i < n; ++i) 
//       {
//         float data = *poutbuf++;
//         *(buffer[0]+i) += data;
//         *(buffer[1]+i) += data;
//       }
//   }
//   else 
//   {
//     DEBUG_AUDIOCONVERT(stderr, "AudioConverter::readAudio Channel mismatch: source chans:%d -> dst chans:%d\n", fchan, channel);
//   }
//   */
//   
//   return _sfCurFrame;
// }

// off_t AudioConverter::readAudio(MusECore::SndFileR& f, unsigned offset, float** buffer, int channel, int n, bool doSeek, bool overwrite)
// {
//   if(f.isNull())
//     return _sfCurFrame;
//   
//   // Added by Tim. p3.3.17 DELETETHIS or comment it in again. it's disabled anyway
//   //#ifdef AUDIOCONVERT_DEBUG_PRC
//   //DEBUG_AUDIOCONVERT(stderr, "AudioConverter::process %s audConv:%p sfCurFrame:%ld offset:%u channel:%d fchan:%d n:%d\n", 
//   //        f.name().toLatin1(), this, sfCurFrame, offset, channel, f.channels(), n);
//   //#endif
//   
//   off_t frame     = offset;  // _spos is added before the call.
//   unsigned fsrate = f.samplerate();
//   bool resample   = isValid() && ((unsigned)MusEGlobal::sampleRate != fsrate);  
//   
//   // No resampling needed?
//   if(!resample)
//   {
//     // Sample rates are the same. Just a regular seek + read, no conversion.
//     _sfCurFrame = f.seek(frame, 0);
//     return _sfCurFrame + f.read(channel, buffer, n, overwrite);
//   }
//   
//   // Is a 'transport' seek requested? (Not to be requested with every read! Should only be for 'first read' seeks, or positional 'transport' seeks.)
//   // Due to the support of sound file references in MusE, seek must ALWAYS be done before read, as before,
//   //  except now we alter the seek position if sample rate conversion is being used and remember the seek positions. 
//   if(doSeek)
//   {
//     // Sample rates are different. Seek to a calculated 'sample rate ratio factored' position.
//     
//     double srcratio = (double)fsrate / (double)MusEGlobal::sampleRate;
//     //long inSize = long((double)frames * _src_ratio) + 1     // From MusE-2 file converter. DELETETHIS ???
//     off_t newfr = (off_t)floor(((double)frame * srcratio));    // From simplesynth.
//   
//     _sfCurFrame = f.seek(newfr, 0);
//     
//     // Added by Tim. p3.3.17 DELETETHIS 3 or comment it in
//     //#ifdef AUDIOCONVERT_DEBUG_PRC
//     //DEBUG_AUDIOCONVERT(stderr, "AudioConverter::process Seek frame:%ld converted to frame:%ld sfCurFrame:%ld\n", frame, newfr, sfCurFrame);
//     //#endif
//     
//     // Reset the converter. Its current state is meaningless now.
//     reset();
//   }
//   else  
//   {
//     // No seek requested. 
//     // Added by Tim. p3.3.17 DELETETHIS 3 or comment it in
//     //#ifdef AUDIOCONVERT_DEBUG_PRC
//     //DEBUG_AUDIOCONVERT(stderr, "AudioConverter::process No 'transport' seek, rates different. Seeking to sfCurFrame:%ld\n", sfCurFrame);
//     //#endif
//     
//     // Sample rates are different. We can't just tell seek to go to an absolute calculated position, 
//     //  since the last position can vary - it might not be what the calculated position is. 
//     // We must use the last position left by SRC conversion, ie. let the file position progress on its own.
//     _sfCurFrame = f.seek(_sfCurFrame, 0);
//   }
//   
//   /* DELETETHIS 5
//   int   fchan              = f.channels();
//   long  outFrames          = n;  
//   long  outSize            = outFrames * fchan;
//   float outbuffer[outSize];
//   */
//   
//   // DELETETHIS 4
//   //sfCurFrame = process(f, sfCurFrame, offset, &outbuffer[0], channel, n);
// //  sfCurFrame = process(f, sfCurFrame, outbuffer, channel, n);
//   //sfCurFrame = process(f, sfCurFrame, buffer, channel, n, overwrite);
//   _sfCurFrame = process(f, buffer, channel, n, overwrite);
//   
//   /* DELETETHIS 58 (whoa!)
//   float*  poutbuf = &outbuffer[0];
//   if(fchan == channel) 
//   {
//     if(overwrite)
//       //for (size_t i = 0; i < rn; ++i) 
//       for (int i = 0; i < n; ++i) 
//       {
//         for(int ch = 0; ch < channel; ++ch)
//           *(buffer[ch] + i) = *poutbuf++;
//       }
//     else
//       //for(size_t i = 0; i < rn; ++i) 
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
//       //for(size_t i = 0; i < rn; ++i)
//       for(int i = 0; i < n; ++i)
//         *(buffer[0] + i) = poutbuf[i + i] + poutbuf[i + i + 1];
//     else  
//       //for(size_t i = 0; i < rn; ++i)
//       for(int i = 0; i < n; ++i)
//         *(buffer[0] + i) += poutbuf[i + i] + poutbuf[i + i + 1];
//   }
//   else if((fchan == 1) && (channel == 2)) 
//   {
//     // mono to stereo
//     if(overwrite)
//       //for(size_t i = 0; i < rn; ++i) 
//       for(int i = 0; i < n; ++i) 
//       {
//         float data = *poutbuf++;
//         *(buffer[0]+i) = data;
//         *(buffer[1]+i) = data;
//       }
//     else  
//       //for(size_t i = 0; i < rn; ++i) 
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
//     DEBUG_AUDIOCONVERT(stderr, "AudioConverter::readAudio Channel mismatch: source chans:%d -> dst chans:%d\n", fchan, channel);
//     #endif
//   }
//   */
//   
//   return _sfCurFrame;
// }


// AudioConverterSettingsGroup::AudioConverterSettingsGroup(bool isLocal)
// {
//   
// }
// 
// void AudioConverterSettingsGroup::read(Xml& xml)
// {
//       for (;;) {
//             Xml::Token token = xml.parse();
//             const QString& tag = xml.s1();
//             switch (token) {
//                   case Xml::Error:
//                   case Xml::End:
//                         return;
//                   case Xml::TagStart:
//                         if (tag == "preferredResampler")
//                               preferredResampler = xml.parseInt();
//                         else if (tag == "preferredShifter")
//                               preferredShifter = xml.parseInt();
//                         else if (tag == "SRCSettings")
//                               SRCResamplerSettings.read(xml);
//                         else if (tag == "zitaResamplerSettings")
//                               zitaResamplerSettings.read(xml);
//                         else if (tag == "rubberbandSettings")
//                               rubberbandSettings.read(xml);
//                         //else if (tag == "soundtouchSettings")  // TODO
//                         //      soundtouchSettings.read(xml);
//                         else
//                               xml.unknown("audioConverterSettings");
//                         break;
//                   case Xml::Attribut:
//                               fprintf(stderr, "audioConverterSettings unknown tag %s\n", tag.toLatin1().constData());
//                         break;
//                   case Xml::TagEnd:
//                         if (tag == "audioConverterSettings") {
//                               return;
//                               }
//                   default:
//                         break;
//                   }
//             }
// }
// 
// void AudioConverterSettingsGroup::write(int level, Xml& xml) const
// {
//   if(!isSet())
//     return;
//   
//   xml.tag(level++, "audioConverterSettings");
//   
//   xml.intTag(level, "preferredResampler", preferredResampler);
//   xml.intTag(level, "preferredShifter", preferredShifter);
//       
//   SRCResamplerSettings.write(level, xml);
//   zitaResamplerSettings.write(level, xml);
//   rubberbandSettings.write(level, xml);
//   //soundtouchSettings.write(level, xml); // TODO
//   
//   xml.tag(--level, "/audioConverterSettings");
// }


} // namespace MusECore
