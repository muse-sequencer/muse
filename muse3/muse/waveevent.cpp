//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: waveevent.cpp,v 1.9.2.6 2009/12/20 05:00:35 terminator356 Exp $
//
//  (C) Copyright 2000-2003 Werner Schweer (ws@seh.de)
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

// REMOVE Tim. samplerate. Added.
//#include "audio_convert/lib_audio_convert/audioconvert.h"
#include "audio_convert/audio_converter_settings_group.h"
#include "audio_convert/audio_converter_plugin.h"
#include "node.h"
#include "part.h"
#include "gconfig.h"
#include "time_stretch.h"

#include "globals.h"
#include "event.h"
#include "waveevent.h"
#include "xml.h"
#include "wave.h"

#include <iostream>
#include <math.h>

// REMOVE Tim. samplerate. Enabled.
#define USE_SAMPLERATE

// REMOVE Tim. samplerate. Enabled.
#define WAVEEVENT_DEBUG
//#define WAVEEVENT_DEBUG_PRC

namespace MusECore {

//---------------------------------------------------------
//   WaveEvent
//---------------------------------------------------------

WaveEventBase::WaveEventBase(EventType t)
   : EventBase(t)
      {
      _spos = 0;
      // REMOVE Tim. samplerate. Added.
      //_stretchList = new StretchList();
      _prefetchFifo = new Fifo();
      _prefetchWritePos = ~0;
      _lastSeekPos  = ~0;
      //_audioConverterSettings = new AudioConverterSettingsGroup();
      }

WaveEventBase::WaveEventBase(const WaveEventBase& ev, bool duplicate_not_clone)
   : EventBase(ev, duplicate_not_clone)
{
      _name = ev._name;
      _spos = ev._spos;

      // REMOVE Tim. samplerate. Added.
      //_stretchList = new StretchList();
      // Make a new Fifo.
      _prefetchFifo = new Fifo();
      _prefetchWritePos = ~0;
      _lastSeekPos  = ~0;
      //*_audioConverterSettings = *ev._audioConverterSettings; // TODO? Or just keep the existing settings?

// REMOVE Tim. samplerate. Added.
// #ifdef USE_SAMPLERATE
//       // Create a new converter.
//       _audConv = new SRCAudioConverter(f.channels(), SRC_SINC_MEDIUM_QUALITY);
// #endif      
      
      // NOTE: It is necessary to create copies always. Unlike midi events, no shared data is allowed for 
      //        wave events because sndfile handles and audio stretchers etc. ABSOLUTELY need separate instances always. 
      //       So duplicate_not_clone is not used here. 
      if(!ev.f.isNull() && !ev.f.canonicalPath().isEmpty())
      {
// REMOVE Tim. samplerate. Changed.
//         f = getWave(ev.f.canonicalPath(), !ev.f.isWritable(), ev.f.isOpen(), false); // Don't show error box.
        // Don't show error box, and assign the audio converter settings and stretch list.
        f = getWave(ev.f.canonicalPath(), !ev.f.isWritable(), ev.f.isOpen(), false, ev.f.audioConverterSettings(), ev.f.stretchList());
        
// REMOVE Tim. samplerate. Added.
// #ifdef USE_SAMPLERATE
//         if(f.isNull())
//         {
//           // Do we have a valid audio converter?
//           if(_audConv)
//             _audConv = AudioConverter::release(_audConv);
//         }
//         else
//         {
//           // Do we already have a valid audio converter?
//           if(_audConv)
//             // Just set the channels.
//             _audConv->setChannels(f.channels());
//           else
//             // Create a new converter.
//             _audConv = new SRCAudioConverter(f.channels(), SRC_SINC_MEDIUM_QUALITY);
//         }
// #endif      
      }
}

// REMOVE Tim. samplerate. Added.
WaveEventBase::~WaveEventBase()
{
  //_audConv = AudioConverter::release(_audConv);
  delete _prefetchFifo;
  //delete _stretchList;
  //delete _audioConverterSettings;
}

//---------------------------------------------------------
//   assign
//---------------------------------------------------------

void WaveEventBase::assign(const EventBase& ev)  
{
  if(ev.type() != type())
    return;
  EventBase::assign(ev);

  _name = ev.name();
  _spos = ev.spos();

  SndFileR sf = ev.sndFile();
  setSndFile(sf);
  
  // REMOVE Tim. samplerate. Added.
  _prefetchWritePos = ~0;
  _lastSeekPos  = ~0;
}

bool WaveEventBase::isSimilarTo(const EventBase& other_) const
{
	const WaveEventBase* other = dynamic_cast<const WaveEventBase*>(&other_);
	if (other==NULL) // dynamic cast hsa failed: "other_" is not of type WaveEventBase.
		return false;
	
	return f.dirPath()==other->f.dirPath() && _spos==other->_spos && this->PosLen::operator==(*other);
}

// REMOVE Tim. samplerate. Added.
// void WaveEventBase::setSndFile(SndFileR& sf)
// { 
//   f = sf;
//   // REMOVE Tim. samplerate. Added.
// #ifdef USE_SAMPLERATE
//   if(f.isNull())
//   {
//     // Do we have a valid audio converter?
//     if(_audConv)
//       _audConv = AudioConverter::release(_audConv);
//   }
//   else
//   {
//     // Do we already have a valid audio converter?
//     if(_audConv)
//       // Just set the channels.
//       _audConv->setChannels(f.channels());
//     else
//       // Create a new converter.
//       _audConv = new SRCAudioConverter(f.channels(), SRC_SINC_MEDIUM_QUALITY);
//   }
// #endif
// }

//---------------------------------------------------------
//   WaveEvent::mid
//---------------------------------------------------------

EventBase* WaveEventBase::mid(unsigned b, unsigned e) const
      {
      WaveEventBase* ev = new WaveEventBase(*this);
      unsigned fr = frame();
      unsigned start = fr - b;
      if(b > fr)
      {  
        start = 0;
        ev->setSpos(spos() + b - fr);
      }
      unsigned end = endFrame();
      
      if (e < end)
            end = e;

      ev->setFrame(start);
      ev->setLenFrame(end - b - start);
      return ev;
      }

//---------------------------------------------------------
//   dump
//---------------------------------------------------------

void WaveEventBase::dump(int n) const
      {
      EventBase::dump(n);
      }

//---------------------------------------------------------
//   WaveEventBase::read
//---------------------------------------------------------

void WaveEventBase::read(Xml& xml)
      {
      StretchList sl;
      AudioConverterSettingsGroup settings(true); // Local non-default settings.
      settings.populate(&MusEGlobal::audioConverterPluginList, true);  // Local non-default settings.
      QString filename;
      for (;;) {
            Xml::Token token = xml.parse();
            const QString& tag = xml.s1();
            switch (token) {
                  case Xml::Error:
                  case Xml::End:
                  case Xml::Attribut:
                        return;
                  case Xml::TagStart:
                        if (tag == "poslen")
                              PosLen::read(xml, "poslen");
                        else if (tag == "frame")
                              _spos = xml.parseInt();
                        else if (tag == "file") {
//                               SndFileR wf = getWave(xml.parse1(), true);
// // REMOVE Tim. samplerate. Changed.
// //                               if (wf) f = wf;
//                               if (wf) setSndFile(wf);
                              filename = xml.parse1();
                              }
                              
// REMOVE Tim. samplerate. Added.
                        else if (tag == "stretchlist")
                        {
//                           if(f.stretchList())
//                             f.stretchList()->read(xml);
                          sl.read(xml);
                        }
                        else if (tag == "audioConverterSettingsGroup")
                        {
//                           if(f.audioConverterSettings())
//                             f.audioConverterSettings()->read(xml);
                          settings.read(xml);
                        }
                        
                        else
                              xml.unknown("Event");
                        break;
                  case Xml::TagEnd:
                        if (tag == "event") {
                              Pos::setType(FRAMES);   // DEBUG
                              
                              // REMOVE Tim.samplerate. Added.
                              if(!filename.isEmpty())
                              {
                                SndFileR wf = getWave(filename, true, true, true, &settings, &sl);
                                if(wf) 
                                  setSndFile(wf);
                              }
                              
                              return;
                              }
                  default:
                        break;
                  }
            }
      }

//---------------------------------------------------------
//   write
//---------------------------------------------------------

//void WaveEventBase::write(int level, Xml& xml, const Pos& offset) const
void WaveEventBase::write(int level, Xml& xml, const Pos& offset, bool forcePath) const
      {
      if (f.isNull())
            return;
      xml.tag(level++, "event");
      PosLen wpos(*this);
      wpos += offset;
      wpos.write(level, xml, "poslen");
      xml.intTag(level, "frame", _spos);  // offset in wave file

      //
      // waves in the project dirctory are stored
      // with relative path name, others with absolute path
      //
      QString path = f.dirPath();

      if (!forcePath && path.contains(MusEGlobal::museProject)) {
            // extract MusEGlobal::museProject.
            QString newName = f.path().remove(MusEGlobal::museProject+"/");
            xml.strTag(level, "file", newName);
            }
      else
            xml.strTag(level, "file", f.path());
      
      // REMOVE Tim. samplerate. Added.
      if(f.stretchList())
        f.stretchList()->write(level, xml);
      //stretchList()->write(level, xml);
      
      // REMOVE Tim. samplerate. Added.
      //if(f.staticAudioConverter())
      //  f.staticAudioConverter()->write(level, xml);
      //_audioConverterSettings->write(level, xml);
      if(f.audioConverterSettings())
        f.audioConverterSettings()->write(level, xml);
      
      xml.etag(level, "event");
      }

// REMOVE Tim. samplerate. Added.
//---------------------------------------------------------
//   seekAudio
//---------------------------------------------------------

// void WaveEventBase::seekAudio(sf_count_t offset)
// {
//   #ifdef WAVEEVENT_DEBUG_PRC
//   printf("WaveEventBase::seekAudio audConv:%p offset:%lu\n", _audConv, offset);
//   #endif
// 
// 
//   sf_count_t newfr = offset + _spos;
//   
// #ifdef USE_SAMPLERATE
//   // If we have a valid audio converter then use it to do the seek. Otherwise just a normal seek.
//   if(_audConv)
//   {
//     _audConv->seekAudio(f, newfr);
//     //_audConv->seekAudio(f, offset);   // Hm, just offset not spos? Check...
//   }
//   else 
// #endif
//   // TODO: May want a 'current frame' variable, just like audio converter, to keep track of the latest position.
//   if(!f.isNull())
//   {  
//     const sf_count_t smps = f.samples();
//     if(newfr < 0)
//       newfr = 0;
//     else if(newfr > smps)
//       newfr = smps;
//     f.seek(newfr, SEEK_SET);
//   }
// }
void WaveEventBase::seekAudio(sf_count_t frame)
{
  #ifdef WAVEEVENT_DEBUG_PRC
  //printf("WaveEventBase::seekAudio audConv:%p offset:%lu\n", _audConv, offset);
  printf("WaveEventBase::seekAudio offset:%lu\n", offset);
  #endif

#ifdef USE_SAMPLERATE
  
  //sf_count_t newfr = offset + _spos;
  
  if(!f.isNull())
  {
//     if(f.sampleRateDiffers() && _audConv && _audConv->isValid())
//       newfr = f.convertPosition(newfr);
//     
//     const sf_count_t smps = f.samples();
//     
//     if(newfr < 0)
//       newfr = 0;
//     else if(newfr > smps)
//       newfr = smps;
//     
//     //if(f.sampleRateDiffers() && _audConv && _audConv->isValid())
//     //  _audConv->seekAudio(f, newfr);
//     //else
//       f.seek(newfr, SEEK_SET);
//       
//     // Reset the converter. Its current state is meaningless now.
//     if(f.sampleRateDiffers() && _audConv && _audConv->isValid())
//       _audConv->reset();
    
    f.seekConverted(frame, SEEK_SET, _spos);
  }
  
// #ifdef USE_SAMPLERATE
//   // If we have a valid audio converter then use it to do the seek. Otherwise just a normal seek.
//   if(_audConv)
//   {
//     _audConv->seekAudio(f, newfr);
//     //_audConv->seekAudio(f, offset);   // Hm, just offset not spos? Check...
//   }
//   else 
// #endif
//     

#else

  // TODO: May want a 'current frame' variable, just like audio converter, to keep track of the latest position.
  if(!f.isNull())
  {  
    sf_count_t newfr = offset + _spos;
    const sf_count_t smps = f.samples();
    if(newfr < 0)
      newfr = 0;
    else if(newfr > smps)
      // Clamp it at 'one past the end' in other words EOF.
      newfr = smps;
    f.seek(newfr, SEEK_SET);
  }
#endif
}
      
// REMOVE Tim. samplerate. Changed.
//void WaveEventBase::readAudio(WavePart* /*part*/, unsigned /*offset*/, float** buffer, int channel, int n, bool /*doSeek*/, bool overwrite)
void WaveEventBase::readAudio(unsigned frame, float** buffer, int channel, int n, bool /*doSeek*/, bool overwrite)
{
  // Added by Tim. p3.3.17
  #ifdef WAVEEVENT_DEBUG_PRC
// REMOVE Tim. samplerate. Changed.
//   printf("WaveEventBase::readAudio audConv:%p sfCurFrame:%ld offset:%u channel:%d n:%d\n", audConv, sfCurFrame, offset, channel, n);
  //printf("WaveEventBase::readAudio audConv:%p offset:%u channel:%d n:%d\n", _audConv, offset, channel, n);
  printf("WaveEventBase::readAudio offset:%u channel:%d n:%d overwrite:%d\n", offset, channel, n, overwrite);
  #endif
  
  // DELETETHIS 270. all the below stuff hasn't changed since revision 462, and 
  // will not compile, and has a TODO in it.
  // will this ever be done, or is it completely obsolete?
  // even if we keep the #ifdef branch, there's a huge
  // comment in it. delete that?
  // Changed by Tim. p3.3.18 
  #ifdef USE_SAMPLERATE
  
//   // If we have a valid audio converter then use it to do the processing. Otherwise just a normal seek + read.
//   if(_audConv)
//     //sfCurFrame = audConv->process(f, sfCurFrame, offset + _spos, buffer, channel, n, doSeek, overwrite); DELETETHIS
// // REMOVE Tim. samplerate. Changed.
// //     sfCurFrame = audConv->readAudio(f, sfCurFrame, offset, buffer, channel, n, doSeek, overwrite);
//     _audConv->readAudio(f, offset, buffer, channel, n, doSeek, overwrite); // Hm, just offset not spos? Check...
//   else
//   {
//     if(!f.isNull())
//     {  
// // REMOVE Tim. samplerate. Changed.
// //       sfCurFrame = f.seek(offset + _spos, 0);
// //       sfCurFrame += f.read(channel, buffer, n, overwrite);
//       
// // REMOVE Tim. samplerate. Removed. Should avoid seeking during read - just let it roll on its own. TODO: REINSTATE?      
// //       off_t e_off = offset + _spos;
// //       if(e_off < 0)
// //         e_off = 0;
// //       f.seek(e_off, 0);
//       
//       f.read(channel, buffer, n, overwrite);
//     }  
//   }
//   return;


  if(!f.isNull())
  {  
    // If we have a valid audio converter then use it to do the processing. Otherwise just a normal seek + read.
//     if(f.sampleRateDiffers() && _audConv && _audConv->isValid())
//     if(f.sampleRateDiffers())
//     {
      //sfCurFrame = audConv->process(f, sfCurFrame, offset + _spos, buffer, channel, n, doSeek, overwrite); DELETETHIS
  // REMOVE Tim. samplerate. Changed.
  //     sfCurFrame = audConv->readAudio(f, sfCurFrame, offset, buffer, channel, n, doSeek, overwrite);
      //_audConv->readAudio(f, offset, buffer, channel, n, doSeek, overwrite); // Hm, just offset not spos? Check...
      //_audConv->readAudio(f, f.convertPosition(offset), buffer, channel, n, doSeek, overwrite); // Hm, just offset not spos? Check...
//       _audConv->process(f, buffer, channel, n, overwrite); // Hm, just offset not spos? Check...
      f.readConverted(frame, channel, buffer, n, overwrite);
//     }
//     else
//     {
// // REMOVE Tim. samplerate. Changed.
// //       sfCurFrame = f.seek(offset + _spos, 0);
// //       sfCurFrame += f.read(channel, buffer, n, overwrite);
//       
// // REMOVE Tim. samplerate. Removed. Should avoid seeking during read - just let it roll on its own. TODO: REINSTATE?      
// //       off_t e_off = offset + _spos;
// //       if(e_off < 0)
// //         e_off = 0;
// //       f.seek(e_off, 0);
//         
//       f.read(channel, buffer, n, overwrite);
//     }  
  }
  return;


  
  /* DELETETHIS 250
  unsigned fsrate = f.samplerate();
  int fchan       = f.channels();
  off_t frame     = offset + _spos;
  //bool resample   = src_state && ((unsigned)sampleRate != fsrate);  
  bool resample   = audConv && audConv->isValid() && ((unsigned)sampleRate != fsrate);  
  
  // Is a 'transport' seek requested? (Not to be requested with every read! Should only be for 'first read' seeks, or positional 'transport' seeks.)
  // Due to the support of sound file references in MusE, seek must ALWAYS be done before read, as before,
  //  except now we alter the seek position if sample rate conversion is being used and remember the seek positions. 
  if(doSeek)
  {
    if(!resample)
    {
      // Sample rates are the same. Just a regular seek, no conversion.
      sfCurFrame = f.seek(frame, 0);
    }
    else
    {
      // Sample rates are different. Seek to a calculated 'sample rate ratio factored' position.
      
      double srcratio = (double)fsrate / (double)sampleRate;
      //long inSize = long((double)frames * _src_ratio) + 1     // From MusE-2 file converter.
      off_t newfr = (off_t)floor(((double)frame * srcratio));    // From simplesynth.
    
      //_sfCurFrame = sf_seek(sf, newfr, 0);
      sfCurFrame = f.seek(newfr, 0);
      
      // Added by Tim. p3.3.17
      #ifdef WAVEEVENT_DEBUG_PRC
      printf("WaveEventBase::readAudio Seek frame:%ld converted to frame:%ld _sfCurFrame:%ld\n", frame, newfr, sfCurFrame);
      #endif
      
      // Reset the src converter. It's current state is meaningless now.
      //int srcerr = src_reset(src_state);
      int srcerr = audConv->reset();
      if(srcerr != 0)      
        printf("WaveEventBase::readAudio Converter reset failed: %s\n", src_strerror(srcerr));
    }
  }
  else  
  {
    // No seek requested. Are the rates the same?
    if(!resample)
      // Sample rates are the same. Just a regular seek, no conversion.
      sfCurFrame = f.seek(frame, 0);
    else  
    {
      // Added by Tim. p3.3.17
      #ifdef WAVEEVENT_DEBUG_PRC
      printf("WaveEventBase::readAudio No 'transport' seek, rates different. Seeking to _sfCurFrame:%ld\n", sfCurFrame);
      #endif
      
      // Sample rates are different. We can't just tell seek to go to an absolute calculated position, 
      //  since the last position can vary - it might not be what the calculated position is. 
      // We must use the last position left by SRC conversion, ie. let the file position progress on its own.
      sfCurFrame = f.seek(sfCurFrame, 0);
    }  
  }
  
  // Do we not need to resample?
  if(!resample)
  {
    return sfCurFrame + f.read(channel, buffer, n, overwrite);
  }
  
  size_t rn;
  
  if((sampleRate == 0) || (fsrate == 0))
  {  
    if(debugMsg)
      printf("WaveEventBase::readAudio Using SRC: Error: sampleRate or file samplerate is zero!\n");
    return sfCurFrame;
  }  
  
  // Ratio is defined as output sample rate over input samplerate.
  double srcratio = (double)sampleRate / (double)fsrate;
  long outFrames = n;  
  //long outSize = outFrames * channel;
  long outSize = outFrames * fchan;
  
  //long inSize = long(outSize * srcratio) + 1                      // From MusE-2 file converter.
  //long inSize = (long)floor(((double)outSize / srcratio));        // From simplesynth.
  //long inFrames = (long)floor(((double)outFrames / srcratio));    // From simplesynth.
  long inFrames = (long)ceil(((double)outFrames / srcratio));    // From simplesynth.
  //long inFrames = (long)floor(double(outFrames * sfinfo.samplerate) / double(sampleRate));    // From simplesynth.
  
  // Extra input compensation - sometimes src requires more input frames than expected in order to
  //  always get a reliable number of used out frames !
  //inFrames = inFrames / (srcratio / 2.0);
  long inComp = 10;
  inFrames += inComp;
  
  long inSize = inFrames * fchan;
  //long inSize = inFrames * channel;
  
  float inbuffer[inSize];
  float outbuffer[outSize];
      
  //float* poutbuf;
  
  // If the number of file channels is the same as the process channels AND we want overwrite, we can get away with direct copying. 
  //if(overwrite && channel == fchan) 
    // Point the out buffer directly at the return buffers.
  //  poutbuf = buffer;
  //else
    // Point the out buffer at our local buffers.
  //  poutbuf = &outbuffer[0];
  
  // Converter channels are fixed at creation time! Can't change them on the fly. Can't use 'channel' paramter.
  //rn = f.read(inbuffer, inFrames);
  rn = f.readDirect(inbuffer, inFrames);
  
  // convert
  SRC_DATA srcdata;
  srcdata.data_in       = inbuffer;
  srcdata.data_out      = outbuffer;
  //srcdata.data_out      = poutbuf;
  //srcdata.input_frames  = inSize;
  srcdata.input_frames  = rn;
  srcdata.output_frames = outFrames;
  srcdata.end_of_input  = ((long)rn != inFrames);
  srcdata.src_ratio     = srcratio;

  #ifdef WAVEEVENT_DEBUG_PRC
  printf("WaveEventBase::readAudio %s processing converter... inFrames:%ld inSize:%ld outFrames:%ld outSize:%ld rn:%d", 
    f.name().toLatin1(), inFrames, inSize, outFrames, outSize, rn);
  #endif
  
  //int srcerr = src_process(src_state, &srcdata);
  int srcerr = audConv->process(&srcdata);
  if(srcerr != 0)      
  {
    printf("\nWaveEventBase::readAudio SampleRate converter process failed: %s\n", src_strerror(srcerr));
    return sfCurFrame += rn;
  }
  
  #ifdef WAVEEVENT_DEBUG_PRC
  printf(" frames used in:%ld out:%ld\n", srcdata.input_frames_used, srcdata.output_frames_gen);
  #endif
  
  // If the number of frames read by the soundfile equals the input frames, go back.
  // Otherwise we have reached the end of the file, so going back is useless since
  //  there shouldn't be any further calls. (Definitely get buffer underruns if further calls!)
  if((long)rn == inFrames)
  {
    // Go back by the amount of unused frames.
    sf_count_t seekn = inFrames - srcdata.input_frames_used;
    if(seekn != 0)
    {
      #ifdef WAVEEVENT_DEBUG_PRC
      printf("WaveEventBase::readAudio Seek-back by:%d\n", seekn);
      #endif
      sfCurFrame = f.seek(-seekn, SEEK_CUR);
    }
    else  
      sfCurFrame += rn;
  }
  else
    sfCurFrame += rn;
  
  if(debugMsg)
  {
    if(srcdata.output_frames_gen != outFrames)
      printf("WaveEventBase::readAudio %s output_frames_gen:%ld != outFrames:%ld outSize:%ld inFrames:%ld srcdata.input_frames_used:%ld inSize:%ld rn:%d\n", 
        f.name().toLatin1(), srcdata.output_frames_gen, outFrames, outSize, inFrames, srcdata.input_frames_used, inSize, rn); 
  }
  
  if(inFrames != (long)rn)
  {
    if(debugMsg)
      printf("WaveEventBase::readAudio %s rn:%zd != inFrames:%ld output_frames_gen:%ld outFrames:%ld outSize:%ld srcdata.input_frames_used:%ld inSize:%ld\n", 
        f.name().toLatin1(), rn, inFrames, srcdata.output_frames_gen, outFrames, outSize, srcdata.input_frames_used, inSize);
    
    // We've reached the end of the file. Convert the number of frames read.
    //rn = (double)rn * srcratio + 1; 
    rn = (long)floor((double)rn * srcratio); 
    if(rn > (size_t)outFrames)
      rn = outFrames;  
  }
  else 
  if(srcdata.output_frames_gen != outFrames)
  {
    // SRC didn't give us the number of frames we requested. 
    // This can occasionally be radically different from the requested frames, or zero,
    //  even when ample excess input frames are supplied.
    // We're not done converting yet - we haven't reached the end of the file. 
    // We must do something with the buffer. So let's zero whatever SRC didn't fill.
    // FIXME: Instead of zeroing, try processing more input data until the out buffer is full.
    long b = srcdata.output_frames_gen * channel;
    long e = outFrames * channel;
    for(long i = b; i < e; ++i)
      outbuffer[i] = 0.0f;
      //poutbuf[i] = 0.0f;
    rn = outFrames;  
  }  
  else  
    rn = outFrames;
  
  float*  poutbuf = &outbuffer[0];
  if(fchan == channel) 
  {
    if(overwrite)
      for (size_t i = 0; i < rn; ++i) 
      {
        for(int ch = 0; ch < channel; ++ch)
          *(buffer[ch] + i) = *poutbuf++;
      }
    else
      for(size_t i = 0; i < rn; ++i) 
      {
        for(int ch = 0; ch < channel; ++ch)
          *(buffer[ch] + i) += *poutbuf++;
      }
  }
  else if((fchan == 2) && (channel == 1)) 
  {
    // stereo to mono
    if(overwrite)
      for(size_t i = 0; i < rn; ++i)
        *(buffer[0] + i) = poutbuf[i + i] + poutbuf[i + i + 1];
    else  
      for(size_t i = 0; i < rn; ++i)
        *(buffer[0] + i) += poutbuf[i + i] + poutbuf[i + i + 1];
  }
  else if((fchan == 1) && (channel == 2)) 
  {
    // mono to stereo
    if(overwrite)
      for(size_t i = 0; i < rn; ++i) 
      {
        float data = *poutbuf++;
        *(buffer[0]+i) = data;
        *(buffer[1]+i) = data;
      }
    else  
      for(size_t i = 0; i < rn; ++i) 
      {
        float data = *poutbuf++;
        *(buffer[0]+i) += data;
        *(buffer[1]+i) += data;
      }
  }
  else 
  {
    if(debugMsg)
      printf("WaveEventBase::readAudio Channel mismatch: source chans:%d -> dst chans:%d\n", fchan, channel);
  }
  
  return sfCurFrame;
  */
  
  
  #else
  if(f.isNull())
    return;
  
  //sfCurFrame = f.seek(offset + _spos, 0); DELETETHIS 2
  //sfCurFrame += f.read(channel, buffer, n, overwrite);
// // REMOVE Tim. samplerate. Removed. Should avoid seeking during read - just let it roll on its own. TODO: REINSTATE?      
//   off_t e_off = offset + _spos;
//   if(e_off < 0)
//     e_off = 0;
//   f.seek(e_off, 0);
  f.read(channel, buffer, n, overwrite);
      
  return;
  #endif
  
}

// // REMOVE Tim. samplerate. Added.
// void WaveEventBase::clearPrefetchFifo()
// { 
//   _prefetchFifo->clear(); 
// }

// REMOVE Tim. samplerate. Added.
// //---------------------------------------------------------
// //   fetchData
// //    called from prefetch thread
// //---------------------------------------------------------
// 
// void WaveEventBase::fetchAudioData(WavePart* part, sf_count_t pos, int channels, bool off, sf_count_t frames, float** bp, bool doSeek, bool overwrite)
//       {
//       #ifdef WAVEEVENT_DEBUG
//       printf("WaveEventBase::fetchData %s channels:%d samples:%lu pos:%ld\n", name().toLatin1().constData(), channels, frames, pos);
//       #endif
//       
//       if(overwrite)
//       {
//         // reset buffer to zero
//         for (int i = 0; i < channels; ++i)
//               memset(bp[i], 0, frames * sizeof(float));
//       }
//       
//       // Process only if track is not off.
//       if(!off)
//       {  
//         
// //         PartList* pl = parts();
//         unsigned n = frames;
// //         for (iPart ip = pl->begin(); ip != pl->end(); ++ip) 
// //         {
// //               WavePart* part = (WavePart*)(ip->second);
// //               if (part->mute())
// //                   continue;
// //               
//               unsigned p_spos = part->frame();
//               unsigned p_epos = p_spos + part->lenFrame();
//               if (pos + n < p_spos)
//                 break;
//               if (pos >= p_epos)
//                 continue;
// //   
// //               for (iEvent ie = part->nonconst_events().begin(); ie != part->nonconst_events().end(); ++ie) 
// //               {
// //                     Event& event = ie->second;
//                     
//                     
//                     
// //                     unsigned e_spos  = event.frame() + p_spos;
// //                     unsigned nn      = event.lenFrame();
//                     unsigned e_spos  = frame() + p_spos;
//                     unsigned nn      = lenFrame();
//                     unsigned e_epos  = e_spos + nn;
//                     
//                     if (pos + n < e_spos) 
//                       break;
//                     if (pos >= e_epos) 
//                       continue;
//   
//                     int offset = e_spos - pos;
//   
//                     unsigned srcOffset, dstOffset;
//                     if (offset > 0) {
//                           nn = n - offset;
//                           srcOffset = 0;
//                           dstOffset = offset;
//                           }
//                     else {
//                           srcOffset = -offset;
//                           dstOffset = 0;
//                           
//                           nn += offset;
//                           if (nn > n)
//                                 nn = n;
//                           }
//                     float* bpp[channels];
//                     for (int i = 0; i < channels; ++i)
//                           bpp[i] = bp[i] + dstOffset;
//   
// //                     event.readAudio(part, srcOffset, bpp, channels(), nn, doSeek, false);
//                     readAudio(part, srcOffset, bpp, channels, nn, doSeek, overwrite);
//                     
// //                     }
// //               }
//       }
//               
//       if(MusEGlobal::config.useDenormalBias) {
//             // add denormal bias to outdata
//             for (int i = 0; i < channels; ++i)
//                   for (unsigned int j = 0; j < frames; ++j)
//                       bp[i][j] +=MusEGlobal::denormalBias;
//             }
//       
//       _prefetchFifo->add();
//       }


      
// REMOVE Tim. samplerate. Added.
//---------------------------------------------------------
//   prefetchAudio
//---------------------------------------------------------

//void WaveEventBase::prefetchAudio(WavePart* part, sf_count_t writePos, int channels, bool off, sf_count_t frames)
void WaveEventBase::prefetchAudio(Part* part, sf_count_t frames)
{        
//   float* bp[channels];
//   if (_prefetchFifo->getWriteBuffer(channels, frames, bp, writePos))
//         continue;
// 
//   fetchAudioData(part, writePos, channels, off, frames, bp, false, false);
  
  
  Fifo* fifo = audioPrefetchFifo();
  
  if(!fifo)
    return;
  
  SndFileR sf = sndFile();
  if(sf.isNull())
    return;


  const sf_count_t p_spos  = part->frame();
  const sf_count_t p_epos  = p_spos + part->lenFrame();

  const sf_count_t e_spos  = frame() + p_spos;
  sf_count_t nn            = lenFrame();
  const sf_count_t e_epos  = e_spos + nn;
  
  if(_prefetchWritePos + frames >= p_spos && _prefetchWritePos < p_epos &&
      _prefetchWritePos + frames >= e_spos && _prefetchWritePos < e_epos)
  {  
    const sf_count_t offset = e_spos - _prefetchWritePos;

//     sf_count_t srcOffset, dstOffset;
    //sf_count_t dstOffset;
    if(offset > 0) 
    {
      nn = frames - offset;
//       srcOffset = 0;
      //dstOffset = offset;
    }
    else 
    {
//       srcOffset = -offset;
      //dstOffset = 0;
      
      nn += offset;
      if(nn > frames)
        nn = frames;
    }
    
    //e.prefetchAudio(writePos, nn);
    //e.prefetchAudio(part, nn);
    //e.prefetchAudio(part, frames);
    
    
    //sf_count_t sf_smps = sf.samples();
    //if(nn > sf_smps)
    //  nn = sf_smps;






  
    const int chans = sf.channels();
    const sf_count_t  samples = chans * frames;
    //const sf_count_t  samples = chans * nn;
    
    float* bp;
    
    // Here we allocate ONE interleaved buffer which is fed with direct interleaved soundfile data.
    if(fifo->getWriteBuffer(1, samples, &bp, _prefetchWritePos))
      return;

    // Clear the buffer.  TODO: Optimize this by only clearing what's required, and merge with the denormal code below.
    memset(bp, 0, samples * sizeof(float));

    //sf.readDirect(bp, frames);
    sf.readDirect(bp, nn);
    
    // Add denormal bias to outdata.
    if(MusEGlobal::config.useDenormalBias) 
    {
      for(sf_count_t i = 0; i < samples; ++i)
        bp[i] += MusEGlobal::denormalBias;
    }
    
    // Increment the prefetch buffer to a new position.
    fifo->add();
    _prefetchWritePos += nn;
  }
}

// bool WaveEventBase::getAudioPrefetchBuffer(int segs, unsigned long samples, float** dst, unsigned* pos)
// {
//   return _prefetchFifo->get(segs, samples, dst, pos);
// }


} // namespace MusECore
