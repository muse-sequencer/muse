//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: audioconvert.cpp,v 1.1.1.1 2009/12/28 16:07:33 terminator356 Exp $
//
//  (C) Copyright 1999-2009 Werner Schweer (ws@seh.de)
//
//  Audio converter module created by Tim
//  (C) Copyright 2009-2011 Tim E. Real (terminator356 A T sourceforge D O T net)
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

#include "wave.h"
#include "globals.h"
#include "audioconvert.h"
#include "eventbase.h"

//#define AUDIOCONVERT_DEBUG
//#define AUDIOCONVERT_DEBUG_PRC

namespace MusECore {

//---------------------------------------------------------
//   AudioConvertMap
//---------------------------------------------------------

void AudioConvertMap::remapEvents(const EventList* /*el*/)  
{

}

iAudioConvertMap AudioConvertMap::addEvent(EventBase* eb)
{
  iAudioConvertMap iacm = getConverter(eb);
  if(iacm == end())
  {
    AudioConverter* cv = 0;
    if(!eb->sndFile().isNull())
      cv = new SRCAudioConverter(eb->sndFile().channels(), SRC_SINC_MEDIUM_QUALITY);
    
    // Use insert with hint for speed.
    return insert(iacm, std::pair<EventBase*, AudioConverter*> (eb, cv));
  }
  else
    // Adopt a policy of returning an already existing item to enforce no-duplicates.
    return iacm;
}

void AudioConvertMap::removeEvent(EventBase* eb)
{
  iAudioConvertMap iacm = find(eb);
  if(iacm != end())
  {
    AudioConverter* cv = iacm->second;
    if(cv)
      delete cv;
    erase(iacm);  
  }    
}

iAudioConvertMap AudioConvertMap::getConverter(EventBase* eb)
{
  return find(eb);
}

//---------------------------------------------------------
//   AudioConverter
//---------------------------------------------------------

AudioConverter::AudioConverter()
{
  #ifdef AUDIOCONVERT_DEBUG
  printf("AudioConverter::AudioConverter this:%p\n", this);
  #endif

  _refCount = 1;
  _sfCurFrame = 0;
}

AudioConverter::~AudioConverter()
{
  #ifdef AUDIOCONVERT_DEBUG
  printf("AudioConverter::~AudioConverter this:%p\n", this);
  #endif
}

AudioConverter* AudioConverter::reference()
{
  _refCount += 1;
  #ifdef AUDIOCONVERT_DEBUG
  printf("AudioConverter::reference this:%p current refcount:%d\n", this, _refCount);
  #endif
  return this;
}

AudioConverter* AudioConverter::release(AudioConverter* cv)
{
  if(!cv)
    return 0;
  cv->_refCount -= 1;
  #ifdef AUDIOCONVERT_DEBUG
  printf("AudioConverter::release converter:%p current refcount:%d\n", cv, cv->_refCount);
  #endif
  if(cv->_refCount <= 0)
  {
    #ifdef AUDIOCONVERT_DEBUG
    printf("AudioConverter::release deleting converter:%p\n", cv);
    #endif
    delete cv;
    cv = 0;
  }
  return cv;  
}

off_t AudioConverter::readAudio(MusECore::SndFileR& f, unsigned offset, float** buffer, int channel, int n, bool doSeek, bool overwrite)
{
  if(f.isNull())
    return _sfCurFrame;
  
  // Added by Tim. p3.3.17 DELETETHIS or comment it in again. it's disabled anyway
  //#ifdef AUDIOCONVERT_DEBUG_PRC
  //printf("AudioConverter::process %s audConv:%p sfCurFrame:%ld offset:%u channel:%d fchan:%d n:%d\n", 
  //        f.name().toLatin1(), this, sfCurFrame, offset, channel, f.channels(), n);
  //#endif
  
  off_t frame     = offset;  // _spos is added before the call.
  unsigned fsrate = f.samplerate();
  bool resample   = isValid() && ((unsigned)MusEGlobal::sampleRate != fsrate);  
  
  // No resampling needed?
  if(!resample)
  {
    // Sample rates are the same. Just a regular seek + read, no conversion.
    _sfCurFrame = f.seek(frame, 0);
    return _sfCurFrame + f.read(channel, buffer, n, overwrite);
  }
  
  // Is a 'transport' seek requested? (Not to be requested with every read! Should only be for 'first read' seeks, or positional 'transport' seeks.)
  // Due to the support of sound file references in MusE, seek must ALWAYS be done before read, as before,
  //  except now we alter the seek position if sample rate conversion is being used and remember the seek positions. 
  if(doSeek)
  {
    // Sample rates are different. Seek to a calculated 'sample rate ratio factored' position.
    
    double srcratio = (double)fsrate / (double)MusEGlobal::sampleRate;
    //long inSize = long((double)frames * _src_ratio) + 1     // From MusE-2 file converter. DELETETHIS ???
    off_t newfr = (off_t)floor(((double)frame * srcratio));    // From simplesynth.
  
    _sfCurFrame = f.seek(newfr, 0);
    
    // Added by Tim. p3.3.17 DELETETHIS 3 or comment it in
    //#ifdef AUDIOCONVERT_DEBUG_PRC
    //printf("AudioConverter::process Seek frame:%ld converted to frame:%ld sfCurFrame:%ld\n", frame, newfr, sfCurFrame);
    //#endif
    
    // Reset the converter. Its current state is meaningless now.
    reset();
  }
  else  
  {
    // No seek requested. 
    // Added by Tim. p3.3.17 DELETETHIS 3 or comment it in
    //#ifdef AUDIOCONVERT_DEBUG_PRC
    //printf("AudioConverter::process No 'transport' seek, rates different. Seeking to sfCurFrame:%ld\n", sfCurFrame);
    //#endif
    
    // Sample rates are different. We can't just tell seek to go to an absolute calculated position, 
    //  since the last position can vary - it might not be what the calculated position is. 
    // We must use the last position left by SRC conversion, ie. let the file position progress on its own.
    _sfCurFrame = f.seek(_sfCurFrame, 0);
  }
  
  /* DELETETHIS 5
  int   fchan              = f.channels();
  long  outFrames          = n;  
  long  outSize            = outFrames * fchan;
  float outbuffer[outSize];
  */
  
  // DELETETHIS 4
  //sfCurFrame = process(f, sfCurFrame, offset, &outbuffer[0], channel, n);
//  sfCurFrame = process(f, sfCurFrame, outbuffer, channel, n);
  //sfCurFrame = process(f, sfCurFrame, buffer, channel, n, overwrite);
  _sfCurFrame = process(f, buffer, channel, n, overwrite);
  
  /* DELETETHIS 58 (whoa!)
  float*  poutbuf = &outbuffer[0];
  if(fchan == channel) 
  {
    if(overwrite)
      //for (size_t i = 0; i < rn; ++i) 
      for (int i = 0; i < n; ++i) 
      {
        for(int ch = 0; ch < channel; ++ch)
          *(buffer[ch] + i) = *poutbuf++;
      }
    else
      //for(size_t i = 0; i < rn; ++i) 
      for(int i = 0; i < n; ++i) 
      {
        for(int ch = 0; ch < channel; ++ch)
          *(buffer[ch] + i) += *poutbuf++;
      }
  }
  else if((fchan == 2) && (channel == 1)) 
  {
    // stereo to mono
    if(overwrite)
      //for(size_t i = 0; i < rn; ++i)
      for(int i = 0; i < n; ++i)
        *(buffer[0] + i) = poutbuf[i + i] + poutbuf[i + i + 1];
    else  
      //for(size_t i = 0; i < rn; ++i)
      for(int i = 0; i < n; ++i)
        *(buffer[0] + i) += poutbuf[i + i] + poutbuf[i + i + 1];
  }
  else if((fchan == 1) && (channel == 2)) 
  {
    // mono to stereo
    if(overwrite)
      //for(size_t i = 0; i < rn; ++i) 
      for(int i = 0; i < n; ++i) 
      {
        float data = *poutbuf++;
        *(buffer[0]+i) = data;
        *(buffer[1]+i) = data;
      }
    else  
      //for(size_t i = 0; i < rn; ++i) 
      for(int i = 0; i < n; ++i) 
      {
        float data = *poutbuf++;
        *(buffer[0]+i) += data;
        *(buffer[1]+i) += data;
      }
  }
  else 
  {
    #ifdef AUDIOCONVERT_DEBUG
    printf("AudioConverter::readAudio Channel mismatch: source chans:%d -> dst chans:%d\n", fchan, channel);
    #endif
  }
  */
  
  return _sfCurFrame;
}

//---------------------------------------------------------
//   SRCAudioConverter
//---------------------------------------------------------

SRCAudioConverter::SRCAudioConverter(int channels, int type) : AudioConverter()
{
  #ifdef AUDIOCONVERT_DEBUG
  printf("SRCAudioConverter::SRCAudioConverter this:%p channels:%d type:%d\n", this, channels, type);
  #endif

  _type = type;
  _src_state = 0;
  _channels = channels;
  
  int srcerr;
  #ifdef AUDIOCONVERT_DEBUG
  printf("SRCAudioConverter::SRCaudioConverter Creating samplerate converter type:%d with %d channels\n", _type, _channels);
  #endif
  _src_state = src_new(_type, _channels, &srcerr); 
  if(!_src_state)      
    printf("SRCAudioConverter::SRCaudioConverter Creation of samplerate converter type:%d with %d channels failed:%s\n", _type, _channels, src_strerror(srcerr));
}

SRCAudioConverter::~SRCAudioConverter()
{
  #ifdef AUDIOCONVERT_DEBUG
  printf("SRCAudioConverter::~SRCAudioConverter this:%p\n", this);
  #endif
  if(_src_state)
    src_delete(_src_state);
}

void SRCAudioConverter::setChannels(int ch)
{
  #ifdef AUDIOCONVERT_DEBUG
  printf("SRCAudioConverter::setChannels this:%p channels:%d\n", this, ch);
  #endif
  if(_src_state)
    src_delete(_src_state);
  _src_state = 0;
  
  _channels = ch;
  int srcerr;
  #ifdef AUDIOCONVERT_DEBUG
  printf("SRCAudioConverter::setChannels Creating samplerate converter type:%d with %d channels\n", _type, ch);
  #endif  
  _src_state = src_new(_type, ch, &srcerr);  
  if(!_src_state)      
    printf("SRCAudioConverter::setChannels of samplerate converter type:%d with %d channels failed:%s\n", _type, ch, src_strerror(srcerr));
  return;  
}

void SRCAudioConverter::reset()
{
  if(!_src_state)
    return;
  #ifdef AUDIOCONVERT_DEBUG
  printf("SRCAudioConverter::reset this:%p\n", this);
  #endif
  int srcerr = src_reset(_src_state);
  if(srcerr != 0)      
    printf("SRCAudioConverter::reset Converter reset failed: %s\n", src_strerror(srcerr));
  return;  
}

off_t SRCAudioConverter::process(MusECore::SndFileR& f, float** buffer, int channel, int n, bool overwrite)
{
  //return src_process(_src_state, sd); DELETETHIS
  
  if(f.isNull())
    return _sfCurFrame;
  
  // Added by Tim. p3.3.17 DELETETHIS 4
  //#ifdef AUDIOCONVERT_DEBUG_PRC
  //printf("AudioConverter::process %s audConv:%p sfCurFrame:%ld offset:%u channel:%d fchan:%d n:%d\n", 
  //        f.name().toLatin1(), this, sfCurFrame, offset, channel, f.channels(), n);
  //#endif
  
//  off_t frame     = offset;  // _spos is added before the call. DELETETHIS
  unsigned fsrate = f.samplerate();
  //bool resample   = src_state && ((unsigned)MusEGlobal::sampleRate != fsrate);   DELETETHIS 2
//  bool resample   = isValid() && ((unsigned)MusEGlobal::sampleRate != fsrate);  
  
  if((MusEGlobal::sampleRate == 0) || (fsrate == 0))
  {  
    #ifdef AUDIOCONVERT_DEBUG
    printf("SRCAudioConverter::process Error: MusEGlobal::sampleRate or file samplerate is zero!\n");
    #endif
    return _sfCurFrame;
  }  
  
  SRC_DATA srcdata;
  int fchan       = f.channels();
  // Ratio is defined as output sample rate over input samplerate.
  double srcratio = (double)MusEGlobal::sampleRate / (double)fsrate;
  // Extra input compensation.
  long inComp = 1;
  
  long outFrames  = n;  
  //long outSize   = outFrames * channel; DELETETHIS
  long outSize    = outFrames * fchan;
  
  //long inSize = long(outSize * srcratio) + 1                      // From MusE-2 file converter. DELETETHIS3
  //long inSize = (long)floor(((double)outSize / srcratio));        // From simplesynth.
  //long inFrames = (long)floor(((double)outFrames / srcratio));    // From simplesynth.
  long inFrames = (long)ceil(((double)outFrames / srcratio)) + inComp;    // From simplesynth.
  // DELETETHIS
  //long inFrames = (long)floor(double(outFrames * sfinfo.samplerate) / double(MusEGlobal::sampleRate));    // From simplesynth.
  
  long inSize = inFrames * fchan;
  //long inSize = inFrames * channel; DELETETHIS
  
  // Start with buffers at expected sizes. We won't need anything larger than this, but add 4 for good luck.
  float inbuffer[inSize + 4];
  float outbuffer[outSize];
      
  //size_t sfTotalRead  = 0; DELETETHIS
  size_t rn           = 0;
  long totalOutFrames = 0;
  
  srcdata.data_in       = inbuffer;
  srcdata.data_out      = outbuffer;
//  srcdata.data_out      = buffer; DELETETHIS
  
  // Set some kind of limit on the number of attempts to completely fill the output buffer, 
  //  in case something is really screwed up - we don't want to get stuck in a loop here.
  int attempts = 10;
  for(int attempt = 0; attempt < attempts; ++attempt)
  {
    rn = f.readDirect(inbuffer, inFrames);
    //sfTotalRead += rn; DELETETHIS
    
    // convert
    //srcdata.data_in       = inbuffer; DELETETHIS 4
    //srcdata.data_out      = outbuffer;
    //srcdata.data_out      = poutbuf;
    //srcdata.input_frames  = inSize;
    srcdata.input_frames  = rn;
    srcdata.output_frames = outFrames;
    srcdata.end_of_input  = ((long)rn != inFrames);
    srcdata.src_ratio     = srcratio;
  
    //#ifdef AUDIOCONVERT_DEBUG_PRC DELETETHIS or comment it in, or maybe add an additional if (heavyDebugMsg)?
    //printf("AudioConverter::process attempt:%d inFrames:%ld outFrames:%ld rn:%d data in:%p out:%p", 
    //  attempt, inFrames, outFrames, rn, srcdata.data_in, srcdata.data_out);
    //#endif
    
    int srcerr = src_process(_src_state, &srcdata);
    if(srcerr != 0)      
    {
      printf("\nSRCAudioConverter::process SampleRate converter process failed: %s\n", src_strerror(srcerr));
      return _sfCurFrame += rn;
    }
    
    totalOutFrames += srcdata.output_frames_gen;
    
    //#ifdef AUDIOCONVERT_DEBUG_PRC DELETETHIS or comment in or heavyDebugMsg
    //printf(" frames used in:%ld out:%ld totalOutFrames:%ld data in:%p out:%p\n", srcdata.input_frames_used, srcdata.output_frames_gen, totalOutFrames, srcdata.data_in, srcdata.data_out);
    //#endif
    
    #ifdef AUDIOCONVERT_DEBUG
    if(srcdata.output_frames_gen != outFrames)
      printf("SRCAudioConverter::process %s output_frames_gen:%ld != outFrames:%ld inFrames:%ld srcdata.input_frames_used:%ld rn:%d\n", 
        f.name().toLatin1(), srcdata.output_frames_gen, outFrames, inFrames, srcdata.input_frames_used, rn); 
    #endif
    
    // If the number of frames read by the soundfile equals the input frames, go back.
    // Otherwise we have reached the end of the file, so going back is useless since
    //  there shouldn't be any further calls. 
    if((long)rn == inFrames)
    {
      // Go back by the amount of unused frames.
      sf_count_t seekn = inFrames - srcdata.input_frames_used;
      if(seekn != 0)
      {
        #ifdef AUDIOCONVERT_DEBUG_PRC
        printf("SRCAudioConverter::process Seek-back by:%d\n", seekn);
        #endif
        _sfCurFrame = f.seek(-seekn, SEEK_CUR);
      }
      else  
        _sfCurFrame += rn;
      
      if(totalOutFrames == n)
      {
        // We got our desired number of output frames. Stop attempting.
        break;
      }
      else  
      {
        // No point in continuing if on last attempt.
        if(attempt == (attempts - 1))
          break;
          
        #ifdef AUDIOCONVERT_DEBUG
        printf("SRCAudioConverter::process %s attempt:%d totalOutFrames:%ld != n:%d try again\n", f.name().toLatin1(), attempt, totalOutFrames, n);
        #endif
        
        // SRC didn't give us the number of frames we requested. 
        // This can occasionally be radically different from the requested frames, or zero,
        //  even when ample excess input frames are supplied.
        // Move the src output pointer to a new position.
        srcdata.data_out += srcdata.output_frames_gen * channel;
        // Set new number of maximum out frames.
        outFrames -= srcdata.output_frames_gen;
        // Calculate the new number of file input frames required.
        inFrames = (long)ceil(((double)outFrames / srcratio)) + inComp;
        // Keep trying.
        continue;
      }  
    }
    else
    {
      _sfCurFrame += rn;
      #ifdef AUDIOCONVERT_DEBUG
      printf("SRCAudioConverter::process %s rn:%zd != inFrames:%ld output_frames_gen:%ld outFrames:%ld srcdata.input_frames_used:%ld\n", 
        f.name().toLatin1(), rn, inFrames, srcdata.output_frames_gen, outFrames, srcdata.input_frames_used);
      #endif
      
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
  if(totalOutFrames != n)
  {
    #ifdef AUDIOCONVERT_DEBUG
    printf("SRCAudioConverter::process %s totalOutFrames:%ld != n:%d\n", f.name().toLatin1(), totalOutFrames, n);
    #endif
          
    // Let's zero the rest of it.
    long b = totalOutFrames * channel;
    long e = n * channel;
    for(long i = b; i < e; ++i)
      outbuffer[i] = 0.0f;
  }
  
  float*  poutbuf = outbuffer;
  if(fchan == channel) 
  {
    if(overwrite)
      for (int i = 0; i < n; ++i) 
      {
        for(int ch = 0; ch < channel; ++ch)
          *(buffer[ch] + i) = *poutbuf++;
      }
    else
      for(int i = 0; i < n; ++i) 
      {
        for(int ch = 0; ch < channel; ++ch)
          *(buffer[ch] + i) += *poutbuf++;
      }
  }
  else if((fchan == 2) && (channel == 1)) 
  {
    // stereo to mono
    if(overwrite)
      for(int i = 0; i < n; ++i)
        *(buffer[0] + i) = poutbuf[i + i] + poutbuf[i + i + 1];
    else  
      for(int i = 0; i < n; ++i)
        *(buffer[0] + i) += poutbuf[i + i] + poutbuf[i + i + 1];
  }
  else if((fchan == 1) && (channel == 2)) 
  {
    // mono to stereo
    if(overwrite)
      for(int i = 0; i < n; ++i) 
      {
        float data = *poutbuf++;
        *(buffer[0]+i) = data;
        *(buffer[1]+i) = data;
      }
    else  
      for(int i = 0; i < n; ++i) 
      {
        float data = *poutbuf++;
        *(buffer[0]+i) += data;
        *(buffer[1]+i) += data;
      }
  }
  else 
  {
    #ifdef AUDIOCONVERT_DEBUG
    printf("SRCAudioConverter::process Channel mismatch: source chans:%d -> dst chans:%d\n", fchan, channel);
    #endif
  }
  
  return _sfCurFrame;
}

#ifdef RUBBERBAND_SUPPORT

//---------------------------------------------------------
//   RubberBandAudioConverter
//---------------------------------------------------------

RubberBandAudioConverter::RubberBandAudioConverter(int channels, int options) : AudioConverter()
{
  #ifdef AUDIOCONVERT_DEBUG
  printf("RubberBandAudioConverter::RubberBandAudioConverter this:%p channels:%d options:%x\n", this, channels, options);
  #endif

  _options = options;
  _rbs = 0;
  _channels = channels;
  
  _rbs = new RubberBandStretcher(MusEGlobal::sampleRate, _channels, _options);  // , initialTimeRatio = 1.0, initialPitchScale = 1.0 DELETETHIS
}

RubberBandAudioConverter::~RubberBandAudioConverter()
{
  #ifdef AUDIOCONVERT_DEBUG
  printf("RubberBandAudioConverter::~RubberBandAudioConverter this:%p\n", this);
  #endif
  if(_rbs)
    delete _rbs;
}

void RubberBandAudioConverter::setChannels(int ch)
{
  #ifdef AUDIOCONVERT_DEBUG
  printf("RubberBandAudioConverter::setChannels this:%p channels:%d\n", this, ch);
  #endif
  if(_rbs)
    delete _rbs;
  _rbs = 0;
  
  _channels = ch;
  _rbs = new RubberBandStretcher(MusEGlobal::sampleRate, _channels, _options);  // , initialTimeRatio = 1.0, initialPitchScale = 1.0
}

void RubberBandAudioConverter::reset()
{
  if(!_rbs)
    return;
  #ifdef AUDIOCONVERT_DEBUG
  printf("RubberBandAudioConverter::reset this:%p\n", this);
  #endif
  _rbs->reset();
  return;  
}

/////////////////////////////////
// TODO: Not finished yet..
////////////////////////////////
// DELETETHIS well then... but maybe we can clean it up anyway? remove the below for example ;)?
off_t RubberBandAudioConverter::process(MusECore::SndFileR& f, float** buffer, int channel, int n, bool overwrite)
{
  //return src_process(_src_state, sd); DELETETHIS
  
  if(f.isNull())
    return _sfCurFrame;
  
  // Added by Tim. p3.3.17 DELETETHIS 4
  //#ifdef AUDIOCONVERT_DEBUG_PRC
  //printf("AudioConverter::process %s audConv:%p sfCurFrame:%ld offset:%u channel:%d fchan:%d n:%d\n", 
  //        f.name().toLatin1(), this, sfCurFrame, offset, channel, f.channels(), n);
  //#endif
  
//  off_t frame     = offset;  // _spos is added before the call. DELETETHIS
  unsigned fsrate = f.samplerate();
  //bool resample   = src_state && ((unsigned)MusEGlobal::sampleRate != fsrate);   DELETETHIS 2
//  bool resample   = isValid() && ((unsigned)MusEGlobal::sampleRate != fsrate);  
  
  if((MusEGlobal::sampleRate == 0) || (fsrate == 0))
  {  
    #ifdef AUDIOCONVERT_DEBUG
    printf("RubberBandAudioConverter::process Error: MusEGlobal::sampleRate or file samplerate is zero!\n");
    #endif
    return _sfCurFrame;
  }  
  
//  SRC_DATA srcdata;
  int fchan       = f.channels();
  // Ratio is defined as output sample rate over input samplerate.
  double srcratio = (double)MusEGlobal::sampleRate / (double)fsrate;
  // Extra input compensation.
  long inComp = 1;
  
  long outFrames  = n;  
  //long outSize   = outFrames * channel; DELETETHIS
  long outSize    = outFrames * fchan;
  
  //long inSize = long(outSize * srcratio) + 1                      // From MusE-2 file converter.  DELETETHIS 3
  //long inSize = (long)floor(((double)outSize / srcratio));        // From simplesynth.
  //long inFrames = (long)floor(((double)outFrames / srcratio));    // From simplesynth.
  long inFrames = (long)ceil(((double)outFrames / srcratio)) + inComp;    // From simplesynth.
  // DELETETHIS
  //long inFrames = (long)floor(double(outFrames * sfinfo.samplerate) / double(MusEGlobal::sampleRate));    // From simplesynth.
  
  long inSize = inFrames * fchan;
  //long inSize = inFrames * channel; DELETETHIS
  
  // Start with buffers at expected sizes. We won't need anything larger than this, but add 4 for good luck.
  float inbuffer[inSize]; // +4
//  float outbuffer[outSize]; DELETETHIS
      
  //float* rbinbuffer[fchan]; DELETETHIS 4
  //float rbindata[inSize];
  //for (int i = 0; i < fchan; ++i)
  //      rbinbuffer[i] = rbindata + i * inFrames;
      
  float* rboutbuffer[fchan];
  float rboutdata[outSize];
  for (int i = 0; i < fchan; ++i)
        rboutbuffer[i] = rboutdata + i * outFrames;
      
  //size_t sfTotalRead  = 0; DELETETHIS
  size_t rn           = 0;
  long totalOutFrames = 0;
  
//  srcdata.data_in       = inbuffer; DELETETHIS 3
  //srcdata.data_out      = outbuffer;
//  srcdata.data_out      = buffer;
  float** data_out       = rboutbuffer;
  
  // For just sample rate conversion, apply same ratio to both time and pitch.
  _rbs->setTimeRatio(srcratio);
  _rbs->setPitchScale(srcratio);
  
  // Set some kind of limit on the number of attempts to completely fill the output buffer, 
  //  in case something is really screwed up - we don't want to get stuck in a loop here.
  int attempts = 10;
  for(int attempt = 0; attempt < attempts; ++attempt)
  {
    size_t sreq = _rbs->getSamplesRequired();
    
    size_t rbinSize = sreq * fchan;
    float* rbinbuffer[fchan];
    float rbindata[rbinSize];
    for(int i = 0; i < fchan; ++i)
      rbinbuffer[i] = rbindata + i * sreq;
    
//    rn = f.readDirect(inbuffer, inFrames); DELETETHIS
    rn = f.readDirect(inbuffer, sreq);
    //sfTotalRead += rn; DELETETHIS
    
    // Must de-interleave soundfile data to feed to rubberband.
    for(size_t i = 0; i < rn; ++i) 
    {
      for(int ch = 0; ch < fchan; ++ch)
        *(rbinbuffer[ch] + i) = *inbuffer++;
    }
           
    _rbs->process(rbinbuffer, rn, (long)rn != inFrames);
    
    // "This function returns -1 if all data has been fully processed and all output read, and the stretch process is now finished." 
    int savail = _rbs->available(); 
    
    
    // convert
    //srcdata.data_in       = inbuffer; DELETETHIS 4
    //srcdata.data_out      = outbuffer;
    //srcdata.data_out      = poutbuf;
    //srcdata.input_frames  = inSize;
    srcdata.input_frames  = rn;
    srcdata.output_frames = outFrames;
    srcdata.end_of_input  = ((long)rn != inFrames);
    srcdata.src_ratio     = srcratio;
  
    //#ifdef AUDIOCONVERT_DEBUG_PRC DELETETHIS or comment in
    //printf("AudioConverter::process attempt:%d inFrames:%ld outFrames:%ld rn:%d data in:%p out:%p", 
    //  attempt, inFrames, outFrames, rn, srcdata.data_in, srcdata.data_out);
    //#endif
    
    int srcerr = src_process(_src_state, &srcdata);
    if(srcerr != 0)      
    {
      printf("\RubberBandAudioConverter::process SampleRate converter process failed: %s\n", src_strerror(srcerr));
      return _sfCurFrame += rn;
    }
    
    totalOutFrames += srcdata.output_frames_gen;
    
    //#ifdef AUDIOCONVERT_DEBUG_PRC DELETETHIS or comment in
    //printf(" frames used in:%ld out:%ld totalOutFrames:%ld data in:%p out:%p\n", srcdata.input_frames_used, srcdata.output_frames_gen, totalOutFrames, srcdata.data_in, srcdata.data_out);
    //#endif
    
    #ifdef AUDIOCONVERT_DEBUG
    if(srcdata.output_frames_gen != outFrames)
      printf("RubberBandAudioConverter::process %s output_frames_gen:%ld != outFrames:%ld inFrames:%ld srcdata.input_frames_used:%ld rn:%d\n", 
        f.name().toLatin1(), srcdata.output_frames_gen, outFrames, inFrames, srcdata.input_frames_used, rn); 
    #endif
    
    // If the number of frames read by the soundfile equals the input frames, go back.
    // Otherwise we have reached the end of the file, so going back is useless since
    //  there shouldn't be any further calls. 
    if((long)rn == inFrames)
    {
      // Go back by the amount of unused frames.
      sf_count_t seekn = inFrames - srcdata.input_frames_used;
      if(seekn != 0)
      {
        #ifdef AUDIOCONVERT_DEBUG_PRC
        printf("RubberBandAudioConverter::process Seek-back by:%d\n", seekn);
        #endif
        _sfCurFrame = f.seek(-seekn, SEEK_CUR);
      }
      else  
        _sfCurFrame += rn;
      
      if(totalOutFrames == n)
      {
        // We got our desired number of output frames. Stop attempting.
        break;
      }
      else  
      {
        // No point in continuing if on last attempt.
        if(attempt == (attempts - 1))
          break;
          
        #ifdef AUDIOCONVERT_DEBUG
        printf("RubberBandAudioConverter::process %s attempt:%d totalOutFrames:%ld != n:%d try again\n", f.name().toLatin1(), attempt, totalOutFrames, n);
        #endif
        
        // We didn't get the number of frames we requested. 
        // This can occasionally be radically different from the requested frames, or zero,
        //  even when ample excess input frames are supplied.
        // Move the src output pointer to a new position.
        srcdata.data_out += srcdata.output_frames_gen * channel;
        // Set new number of maximum out frames.
        outFrames -= srcdata.output_frames_gen;
        // Calculate the new number of file input frames required.
        inFrames = (long)ceil(((double)outFrames / srcratio)) + inComp;
        // Keep trying.
        continue;
      }  
    }
    else
    {
      _sfCurFrame += rn;
      #ifdef AUDIOCONVERT_DEBUG
      printf("RubberBandAudioConverter::process %s rn:%zd != inFrames:%ld output_frames_gen:%ld outFrames:%ld srcdata.input_frames_used:%ld\n", 
        f.name().toLatin1(), rn, inFrames, srcdata.output_frames_gen, outFrames, srcdata.input_frames_used);
      #endif
      
      // We've reached the end of the file. Convert the number of frames read.
      //rn = (double)rn * srcratio + 1;  DELETETHIS 5
      //rn = (long)floor((double)rn * srcratio); 
      //if(rn > (size_t)outFrames)
      //  rn = outFrames;  
      // Stop attempting.
      break;  
    }
  }
  
  // If we still didn't get the desired number of output frames.
  if(totalOutFrames != n)
  {
    #ifdef AUDIOCONVERT_DEBUG
    printf("RubberBandAudioConverter::process %s totalOutFrames:%ld != n:%d\n", f.name().toLatin1(), totalOutFrames, n);
    #endif
          
    // Let's zero the rest of it.
    long b = totalOutFrames * channel;
    long e = n * channel;
    for(long i = b; i < e; ++i)
      buffer[i] = 0.0f;
  }
  
  float*  poutbuf = outbuffer;
  if(fchan == channel) 
  {
    if(overwrite)
      for (int i = 0; i < n; ++i) 
      {
        for(int ch = 0; ch < channel; ++ch)
          *(buffer[ch] + i) = *poutbuf++;
      }
    else
      for(int i = 0; i < n; ++i) 
      {
        for(int ch = 0; ch < channel; ++ch)
          *(buffer[ch] + i) += *poutbuf++;
      }
  }
  else if((fchan == 2) && (channel == 1)) 
  {
    // stereo to mono
    if(overwrite)
      for(int i = 0; i < n; ++i)
        *(buffer[0] + i) = poutbuf[i + i] + poutbuf[i + i + 1];
    else  
      for(int i = 0; i < n; ++i)
        *(buffer[0] + i) += poutbuf[i + i] + poutbuf[i + i + 1];
  }
  else if((fchan == 1) && (channel == 2)) 
  {
    // mono to stereo
    if(overwrite)
      for(int i = 0; i < n; ++i) 
      {
        float data = *poutbuf++;
        *(buffer[0]+i) = data;
        *(buffer[1]+i) = data;
      }
    else  
      for(int i = 0; i < n; ++i) 
      {
        float data = *poutbuf++;
        *(buffer[0]+i) += data;
        *(buffer[1]+i) += data;
      }
  }
  else 
  {
    #ifdef AUDIOCONVERT_DEBUG
    printf("RubberBandAudioConverter::process Channel mismatch: source chans:%d -> dst chans:%d\n", fchan, channel);
    #endif
  }
  
  return _sfCurFrame;
}

#endif // RUBBERBAND_SUPPORT

} // namespace MusECore
