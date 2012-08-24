//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: wavetrack.cpp,v 1.15.2.12 2009/12/20 05:00:35 terminator356 Exp $
//
//  (C) Copyright 2003 Werner Schweer (ws@seh.de)
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

#include "track.h"
#include "event.h"
#include "audio.h"
#include "wave.h"
#include "xml.h"
#include "song.h"
#include "globals.h"
#include "gconfig.h"
#include "al/dsp.h"

// Added by Tim. p3.3.18
//#define WAVETRACK_DEBUG

namespace MusECore {

//---------------------------------------------------------
//   fetchData
//    called from prefetch thread
//---------------------------------------------------------

void WaveTrack::fetchData(unsigned pos, unsigned samples, float** bp, bool doSeek)
      {
      // Added by Tim. p3.3.17
      #ifdef WAVETRACK_DEBUG
      printf("WaveTrack::fetchData %s samples:%lu pos:%u\n", name().toLatin1().constData(), samples, pos);
      #endif
      
      // reset buffer to zero
      for (int i = 0; i < channels(); ++i)
            memset(bp[i], 0, samples * sizeof(float));
      
      // Process only if track is not off.
      if(!off())
      {  
        
        PartList* pl = parts();
        unsigned n = samples;
        for (iPart ip = pl->begin(); ip != pl->end(); ++ip) {
              WavePart* part = (WavePart*)(ip->second);
              if (part->mute())
                  continue;
              
              unsigned p_spos = part->frame();
              unsigned p_epos = p_spos + part->lenFrame();
              if (pos + n < p_spos)
                break;
              if (pos >= p_epos)
                continue;
  
              EventList* events = part->events();
              for (iEvent ie = events->begin(); ie != events->end(); ++ie) {
                    Event& event = ie->second;
                    unsigned e_spos  = event.frame() + p_spos;
                    unsigned nn      = event.lenFrame();
                    unsigned e_epos  = e_spos + nn;
                    
                    if (pos + n < e_spos) 
                      break;
                    if (pos >= e_epos) 
                      continue;
  
                    int offset = e_spos - pos;
  
                    unsigned srcOffset, dstOffset;
                    if (offset > 0) {
                          nn = n - offset;
                          srcOffset = 0;
                          dstOffset = offset;
                          }
                    else {
                          srcOffset = -offset;
                          dstOffset = 0;
                          
                          nn += offset;
                          if (nn > n)
                                nn = n;
                          }
                    float* bpp[channels()];
                    for (int i = 0; i < channels(); ++i)
                          bpp[i] = bp[i] + dstOffset;
  
                    event.readAudio(part, srcOffset, bpp, channels(), nn, doSeek, false);
                    
                    }
              }
      }
              
      if(MusEGlobal::config.useDenormalBias) {
            // add denormal bias to outdata
            for (int i = 0; i < channels(); ++i)
                  for (unsigned int j = 0; j < samples; ++j)
                      bp[i][j] +=MusEGlobal::denormalBias;
            }
      
      _prefetchFifo.add();
      }

//---------------------------------------------------------
//   write
//---------------------------------------------------------

void WaveTrack::write(int level, Xml& xml) const
      {
      xml.tag(level++, "wavetrack");
      AudioTrack::writeProperties(level, xml);
      const PartList* pl = cparts();
      for (ciPart p = pl->begin(); p != pl->end(); ++p)
            p->second->write(level, xml);
      xml.etag(level, "wavetrack");
      }

//---------------------------------------------------------
//   read
//---------------------------------------------------------

void WaveTrack::read(Xml& xml)
      {
      for (;;) {
            Xml::Token token = xml.parse();
            const QString& tag = xml.s1();
            switch (token) {
                  case Xml::Error:
                  case Xml::End:
                        return;
                  case Xml::TagStart:
                        if (tag == "part") {
                              Part* p = 0;
                              p = readXmlPart(xml, this);
                              if(p)
                                parts()->add(p);
                              }
                        else if (AudioTrack::readProperties(xml, tag))
                              xml.unknown("WaveTrack");
                        break;
                  case Xml::Attribut:
                        break;
                  case Xml::TagEnd:
                        if (tag == "wavetrack") {
                              mapRackPluginsToControllers();
                              return;
                              }
                  default:
                        break;
                  }
            }
      }

//---------------------------------------------------------
//   newPart
//---------------------------------------------------------

Part* WaveTrack::newPart(Part*p, bool clone)
      {
      WavePart* part = clone ? new WavePart(this, p->events()) : new WavePart(this);
      if (p) {
            part->setName(p->name());
            part->setColorIndex(p->colorIndex());

            *(PosLen*)part = *(PosLen*)p;
            part->setMute(p->mute());
            }
      
      if(clone)
        chainClone(p, part);
      
      return part;
      }

//---------------------------------------------------------
//   getData
//---------------------------------------------------------

bool WaveTrack::getData(unsigned framePos, int channels, unsigned nframe, float** bp)
      {
      if ((MusEGlobal::song->bounceTrack != this) && !noInRoute()) {
            RouteList* irl = inRoutes();
            ciRoute i = irl->begin();
            if(i->track->isMidiTrack())
              return false;

            ((AudioTrack*)i->track)->copyData(framePos, channels, 
                                              i->channel, 
                                              i->channels,
                                              nframe, bp);
            
            ++i;
            for (; i != irl->end(); ++i)
            {
              if(i->track->isMidiTrack())
                continue;

              ((AudioTrack*)i->track)->addData(framePos, channels, 
                                               i->channel, 
                                               i->channels,
                                               nframe, bp);
              
            }
            if (recordFlag()) {
                  if (MusEGlobal::audio->isRecording() && recFile()) {
                        if (MusEGlobal::audio->freewheel()) {
                              }
                        else {
#ifdef _AUDIO_USE_TRUE_FRAME_
                              // TODO: Tested: This is the line that would be needed for Audio Inputs, 
                              //  because the data arrived in the previous period! Test OK, the waves are in sync.
                              // So we need to do Audio Inputs separately above, AND find a way to mix two overlapping
                              //  periods into the file! Nothing wrong with the FIFO per se, we could stamp overlapping
                              //  times. But the soundfile just writes, does not mix.
                              //if (fifo.put(channels, nframe, bp, MusEGlobal::audio->previousPos().frame()))
                              //
                              // Tested: This line is OK for track-to-track recording, the waves are in sync:
#endif                              
                              if (fifo.put(channels, nframe, bp, MusEGlobal::audio->pos().frame()))  
                                    printf("WaveTrack::getData(%d, %d, %d): fifo overrun\n",
                                       framePos, channels, nframe);
                              }
                        }
                  return true;
                  }
            }
      if (!MusEGlobal::audio->isPlaying())
            return false;
      
      // DELETETHIS 43
      // Removed by T356. Multiple out route cacheing now handled by AudioTrack::copyData and ::addData.
      /*
      if (outRoutes()->size() > 1) {
            if (bufferPos != framePos) {
                  // Added by Tim. p3.3.16
                  printf("WaveTrack::getData bufferPos:%d != framePos\n", bufferPos);
                  
                  bufferPos = framePos;
                  if (MusEGlobal::audio->freewheel()) {
                        // when freewheeling, read data direct from file:
                        fetchData(bufferPos, nframe, outBuffers);
                        }
                  else {
                        unsigned pos;
                        if (_prefetchFifo.get(channels, nframe, outBuffers, &pos)) {
                              printf("WaveTrack::getData(%s) fifo underrun\n",
                                 name().toLatin1().constData());
                              return false;
                              }
                        if (pos != framePos) {
                              printf("fifo get error expected %d, got %d\n",
                                 framePos, pos);
                              if (MusEGlobal::debugMsg)
                                    printf("fifo get error expected %d, got %d\n",
                                       framePos, pos);
                              while (pos < framePos) {
                                    if (_prefetchFifo.get(channels, nframe, bp, &pos)) {
                                          printf("WaveTrack::getData(%s) fifo underrun\n",
                                             name().toLatin1().constData());
                                          return false;
                                          }
                                    }
                              }
                        }
                  }
            for (int i = 0; i < channels; ++i)
                  //memcpy(bp[i], outBuffers[i], nframe * sizeof(float));
                  AL::dsp->cpy(bp[i], outBuffers[i], nframe);
            }
      else {
      */
      
            
            if (MusEGlobal::audio->freewheel()) {
                  
                  // when freewheeling, read data direct from file:
                  // Indicate do not seek file before each read.
                  fetchData(framePos, nframe, bp, false);
                  
                  }
            else {
                  unsigned pos;
                  if (_prefetchFifo.get(channels, nframe, bp, &pos)) {
                        printf("WaveTrack::getData(%s) fifo underrun\n",
                           name().toLatin1().constData());
                        return false;
                        }
                  if (pos != framePos) {
                        if (MusEGlobal::debugMsg)
                              printf("fifo get error expected %d, got %d\n",
                                 framePos, pos);
                        while (pos < framePos) {
                              if (_prefetchFifo.get(channels, nframe, bp, &pos)) {
                                    printf("WaveTrack::getData(%s) fifo underrun\n",
                                       name().toLatin1().constData());
                                    return false;
                                    }
                              }
                        }
                  }
      return true;
      }

//---------------------------------------------------------
//   setChannels
//---------------------------------------------------------

void WaveTrack::setChannels(int n)
      {
      AudioTrack::setChannels(n);
      SndFileR sf = recFile();
      if (sf) {
            if (sf->samples() == 0) {
                  sf->remove();
                  sf->setFormat(sf->format(), _channels,
                     sf->samplerate());
                  sf->openWrite();
                  }
            }
      }

} // namespace MusECore
