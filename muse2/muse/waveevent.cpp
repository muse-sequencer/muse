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

#include "audioconvert.h"
#include "globals.h"
#include "event.h"
#include "waveevent.h"
#include "xml.h"
#include "wave.h"
#include <iostream>
#include <math.h>

// Added by Tim. p3.3.18
//#define USE_SAMPLERATE

//#define WAVEEVENT_DEBUG
//#define WAVEEVENT_DEBUG_PRC

namespace MusECore {

//---------------------------------------------------------
//   WaveEvent
//---------------------------------------------------------

WaveEventBase::WaveEventBase(EventType t)
   : EventBase(t)
      {
      deleted = false;
      }

//---------------------------------------------------------
//   WaveEventBase::clone
//---------------------------------------------------------

EventBase* WaveEventBase::clone() 
{ 
  return new WaveEventBase(*this); 
}

//---------------------------------------------------------
//   WaveEvent::mid
//---------------------------------------------------------

EventBase* WaveEventBase::mid(unsigned b, unsigned e)
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
                              SndFileR wf = getWave(xml.parse1(), true);
                              if (wf) f = wf;
                              }
                        else
                              xml.unknown("Event");
                        break;
                  case Xml::TagEnd:
                        if (tag == "event") {
                              Pos::setType(FRAMES);   // DEBUG
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
      xml.etag(level, "event");
      }

void WaveEventBase::readAudio(WavePart* /*part*/, unsigned offset, float** buffer, int channel, int n, bool /*doSeek*/, bool overwrite)
{
  // Added by Tim. p3.3.17
  #ifdef WAVEEVENT_DEBUG_PRC
  printf("WaveEventBase::readAudio audConv:%p sfCurFrame:%ld offset:%u channel:%d n:%d\n", audConv, sfCurFrame, offset, channel, n);
  #endif
  
  // DELETETHIS 270. all the below stuff hasn't changed since revision 462, and 
  // will not compile, and has a TODO in it.
  // will this ever be done, or is it completely obsolete?
  // even if we keep the #ifdef branch, there's a huge
  // comment in it. delete that?
  // Changed by Tim. p3.3.18 
  #ifdef USE_SAMPLERATE
  
  // TODO: 
  >>>>>>>>>>>+++++++++++++++++++++++++++++
  // If we have a valid audio converter then use it to do the processing. Otherwise just a normal seek + read.
  if(audConv)
    //sfCurFrame = audConv->process(f, sfCurFrame, offset + _spos, buffer, channel, n, doSeek, overwrite); DELETETHIS
    sfCurFrame = audConv->readAudio(f, sfCurFrame, offset, buffer, channel, n, doSeek, overwrite);
  else
  {
    if(!f.isNull())
    {  
      sfCurFrame = f.seek(offset + _spos, 0);
      sfCurFrame += f.read(channel, buffer, n, overwrite);
    }  
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
  f.seek(offset + _spos, 0);
  f.read(channel, buffer, n, overwrite);
      
  return;
  #endif
  
}

} // namespace MusECore
