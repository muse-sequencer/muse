//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: wavetrack.cpp,v 1.15.2.12 2009/12/20 05:00:35 terminator356 Exp $
//
//  (C) Copyright 2003 Werner Schweer (ws@seh.de)
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

//---------------------------------------------------------
//   fetchData
//    called from prefetch thread
//---------------------------------------------------------

//void WaveTrack::fetchData(unsigned pos, unsigned samples, float** bp)
void WaveTrack::fetchData(unsigned pos, unsigned samples, float** bp, bool doSeek)
      {
      // Added by Tim. p3.3.17
      #ifdef WAVETRACK_DEBUG
      printf("WaveTrack::fetchData %s samples:%lu pos:%u\n", name().toLatin1().constData(), samples, pos);
      #endif
      
      // reset buffer to zero
      for (int i = 0; i < channels(); ++i)
            memset(bp[i], 0, samples * sizeof(float));
      
      // p3.3.29
      // Process only if track is not off.
      if(!off())
      {  
        
        PartList* pl = parts();
        unsigned n = samples;
        for (iPart ip = pl->begin(); ip != pl->end(); ++ip) {
              WavePart* part = (WavePart*)(ip->second);
              // Changed by Tim. p3.3.17
              //if (part->mute() || isMute())
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
  
                    // By T356. Allow overlapping parts or events to mix together !
                    // Since the buffers are cleared above, just read and add (don't overwrite) the samples.
                    //event.read(srcOffset, bpp, channels(), nn);
                    //event.read(srcOffset, bpp, channels(), nn, false);
                    //event.readAudio(srcOffset, bpp, channels(), nn, doSeek, false);
                    // p3.3.33
                    event.readAudio(part, srcOffset, bpp, channels(), nn, doSeek, false);
                    
                    }
              }
      }
              
      if(config.useDenormalBias) {
            // add denormal bias to outdata
            for (int i = 0; i < channels(); ++i)
                  for (int j = 0; j < samples; ++j)
                  {
                      bp[i][j] +=denormalBias;
                      
                      /*
                      // p3.3.41
                      if(j & 1)
                        bp[i][j] -=denormalBias;
                      else  
                        bp[i][j] +=denormalBias;
                      */  
                  }      
            }
      
      // p3.3.41
      //fprintf(stderr, "WaveTrack::fetchData data: samples:%ld %e %e %e %e\n", samples, bp[0][0], bp[0][1], bp[0][2], bp[0][3]);
      
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
                              //Part* p = newPart();
                              //p->read(xml);
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
        //p->chainClone(part);
        chainClone(p, part);
      
      return part;
      }

//---------------------------------------------------------
//   getData
//---------------------------------------------------------

bool WaveTrack::getData(unsigned framePos, int channels, unsigned nframe, float** bp)
      {
      //if(debugMsg)
      //  printf("WaveTrack::getData framePos:%u channels:%d nframe:%u processed?:%d\n", framePos, channels, nframe, processed());
      
      if ((song->bounceTrack != this) && !noInRoute()) {
            RouteList* irl = inRoutes();
            iRoute i = irl->begin();
            if(i->track->isMidiTrack())
            {
              if(debugMsg)
                printf("WaveTrack::getData: Error: First route is a midi track route!\n");
              return false;
            }
            // p3.3.38
            //((AudioTrack*)i->track)->copyData(framePos, channels, nframe, bp);
            ((AudioTrack*)i->track)->copyData(framePos, channels, 
                                              //(i->track->type() == Track::AUDIO_SOFTSYNTH && i->channel != -1) ? i->channel : 0, 
                                              i->channel, 
                                              i->channels,
                                              nframe, bp);
            
            ++i;
            for (; i != irl->end(); ++i)
            {
              if(i->track->isMidiTrack())
              {
                if(debugMsg)
                  printf("WaveTrack::getData: Error: Route is a midi track route!\n");
                //return false;
                continue;
              }
              // p3.3.38
              //((AudioTrack*)i->track)->addData(framePos, channels, nframe, bp);
              ((AudioTrack*)i->track)->addData(framePos, channels, 
                                               //(i->track->type() == Track::AUDIO_SOFTSYNTH && i->channel != -1) ? i->channel : 0, 
                                               i->channel, 
                                               i->channels,
                                               nframe, bp);
              
            }
            if (recordFlag()) {
                  if (audio->isRecording() && recFile()) {
                        if (audio->freewheel()) {
                              }
                        else {
                              if (fifo.put(channels, nframe, bp, audio->pos().frame()))
                                    printf("WaveTrack::getData(%d, %d, %d): fifo overrun\n",
                                       framePos, channels, nframe);
                              }
                        }
                  return true;
                  }
            }
      if (!audio->isPlaying())
            return false;
      
      // Removed by T356. Multiple out route cacheing now handled by AudioTrack::copyData and ::addData.
      /*
      if (outRoutes()->size() > 1) {
            if (bufferPos != framePos) {
                  // Added by Tim. p3.3.16
                  printf("WaveTrack::getData bufferPos:%d != framePos\n", bufferPos);
                  
                  bufferPos = framePos;
                  if (audio->freewheel()) {
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
                              if (debugMsg)
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
      
            //printf("WaveTrack::getData no out routes\n");
            
            if (audio->freewheel()) {
                  
                  // when freewheeling, read data direct from file:
                  // Indicate do not seek file before each read.
                  // Changed by Tim. p3.3.17
                  //fetchData(framePos, nframe, bp);
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
                        if (debugMsg)
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
                        
                        // p3.3.41
                        //fprintf(stderr, "WaveTrack::getData %s data: nframe:%ld %e %e %e %e\n", name().toLatin1().constData(), nframe, bp[0][0], bp[0][1], bp[0][2], bp[0][3]);
                        
                  }
            //}
      return true;
      }

//---------------------------------------------------------
//   setChannels
//---------------------------------------------------------

void WaveTrack::setChannels(int n)
      {
      AudioTrack::setChannels(n);
      SndFile* sf = recFile();
      if (sf) {
            if (sf->samples() == 0) {
                  sf->remove();
                  sf->setFormat(sf->format(), _channels,
                     sf->samplerate());
                  sf->openWrite();
                  }
            }
      }

