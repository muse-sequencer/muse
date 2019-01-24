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

#include <math.h>

#include "track.h"
#include "event.h"
#include "audio.h"
#include "wave.h"
#include "xml.h"
#include "song.h"
#include "globals.h"
#include "gconfig.h"
#include "al/dsp.h"

// REMOVE Tim. samplerate. Enabled.
//#define WAVETRACK_DEBUG

namespace MusECore {

//---------------------------------------------------------
//   WaveTrack
//---------------------------------------------------------

WaveTrack::WaveTrack() : AudioTrack(Track::WAVE)
{
  setChannels(1);
}

WaveTrack::WaveTrack(const WaveTrack& wt, int flags) : AudioTrack(wt, flags)
{
  internal_assign(wt, flags | Track::ASSIGN_PROPERTIES);
}

void WaveTrack::internal_assign(const Track& t, int flags)
{
      if(t.type() != WAVE)
        return;
      //const WaveTrack& wt = (const WaveTrack&)t;

      const bool dup = flags & ASSIGN_DUPLICATE_PARTS;
      const bool cpy = flags & ASSIGN_COPY_PARTS;
      const bool cln = flags & ASSIGN_CLONE_PARTS;
      if(dup || cpy || cln)
      {
        const PartList* pl = t.cparts();
        for (ciPart ip = pl->begin(); ip != pl->end(); ++ip) {
              Part* spart = ip->second;
              Part* dpart = 0;
              if(dup)
                dpart = spart->hasClones() ? spart->createNewClone() : spart->duplicate();
              else if(cpy)
                dpart = spart->duplicate();
              else if(cln)
                dpart = spart->createNewClone();
              if(dpart)
              {
                dpart->setTrack(this);
                parts()->add(dpart);
              }
              }
      }

}

void WaveTrack::assign(const Track& t, int flags)
{
      AudioTrack::assign(t, flags);
      internal_assign(t, flags);
}

// REMOVE Tim. samplerate. Added.
//---------------------------------------------------------
//   seekData
//    called from prefetch thread
//---------------------------------------------------------

void WaveTrack::seekData(sf_count_t pos)
      {
      #ifdef WAVETRACK_DEBUG
      fprintf(stderr, "WaveTrack::seekData %s pos:%ld\n", name().toLatin1().constData(), pos);
      #endif
      
//       // reset buffer to zero
//       for (int i = 0; i < channels(); ++i)
//             memset(bp[i], 0, samples * sizeof(float));
      
      // Process only if track is not off.
//       if(!off())
      {  
        
        PartList* pl = parts();
//         unsigned n = samples;
        for (iPart ip = pl->begin(); ip != pl->end(); ++ip) {
              WavePart* part = (WavePart*)(ip->second);
//               if (part->mute())
//                   continue;
              
              unsigned p_spos = part->frame();
//               unsigned p_epos = p_spos + part->lenFrame();
//               if (pos + n < p_spos)
//                 break;
//               if (pos >= p_epos)
//                 continue;
  
              EventList& el = part->nonconst_events();
              for (iEvent ie = el.begin(); ie != el.end(); ++ie) {
                    Event& event = ie->second;
                    unsigned e_spos  = event.frame() + p_spos;
//                     unsigned nn      = event.lenFrame();
//                     unsigned e_epos  = e_spos + nn;
//                     
//                     if (pos + n < e_spos) 
//                       break;
//                     if (pos >= e_epos) 
//                       continue;
//   
//                     int offset = e_spos - pos;
  
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
//                     float* bpp[channels()];
//                     for (int i = 0; i < channels(); ++i)
//                           bpp[i] = bp[i] + dstOffset;
//   
//                     event.readAudio(part, srcOffset, bpp, channels(), nn, doSeek, false);
                    
//                     unsigned srcOffset;
//                     if (offset > 0) {
//                           srcOffset = 0;
//                           }
//                     else {
//                           srcOffset = -offset;
//                           }
                          
                          
                    sf_count_t offset = pos - e_spos;
                    if(offset < 0)
                      offset = 0;
                    event.seekAudio(offset);
                    
                    }
              }
      }
              
//       if(MusEGlobal::config.useDenormalBias) {
//             // add denormal bias to outdata
//             for (int i = 0; i < channels(); ++i)
//                   for (unsigned int j = 0; j < samples; ++j)
//                       bp[i][j] +=MusEGlobal::denormalBias;
//             }
//       
//       _prefetchFifo.add();
      }
      
//---------------------------------------------------------
//   fetchData
//    called from prefetch thread
//---------------------------------------------------------

void WaveTrack::fetchData(unsigned pos, unsigned samples, float** bp, bool doSeek, bool overwrite)
      {
      #ifdef WAVETRACK_DEBUG
      fprintf(stderr, "WaveTrack::fetchData %s samples:%u pos:%u overwrite:%d\n", name().toLatin1().constData(), samples, pos, overwrite);
      #endif

      // reset buffer to zero
      if(overwrite)
        for (int i = 0; i < channels(); ++i)
            memset(bp[i], 0, samples * sizeof(float));

      // Process only if track is not off.
      if(!off())
      {
        bool do_overwrite = overwrite;
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
  
              EventList& el = part->nonconst_events();
              for (iEvent ie = el.begin(); ie != el.end(); ++ie) {
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

// REMOVE Tim. samplerate. Changed.
//                     event.readAudio(part, srcOffset, bpp, channels(), nn, doSeek, do_overwrite);
// REMOVE Tim. autoconnect. Changed.
//                     event.readAudio(srcOffset, bpp, channels(), nn, doSeek, false);
                    event.readAudio(srcOffset, bpp, channels(), nn, doSeek, do_overwrite);
                    do_overwrite = false;
                    }
              }
      }

      if(overwrite && MusEGlobal::config.useDenormalBias) {
            // add denormal bias to outdata
            for (int i = 0; i < channels(); ++i)
                  for (unsigned int j = 0; j < samples; ++j)
                      bp[i][j] +=MusEGlobal::denormalBias;
            }

      _prefetchFifo.add();
      }

// REMOVE Tim. samplerate. Added.
// //---------------------------------------------------------
// //   fetchAudioData
// //---------------------------------------------------------
// 
// void WaveTrack::fetchAudioData(unsigned pos, int channels, bool off, unsigned frames, float** bp, bool doSeek, bool overwrite)
// {
//   for(iPart ip = parts()->begin(); ip != parts()->end(); ++ip)
//   {
//     Part* part = ip->second;
//     if(part->mute())
//       continue;
//     
//     unsigned p_spos = part->frame();
//     unsigned p_epos = p_spos + part->lenFrame();
//     if(pos + frames < p_spos)
//       break;
//     if(pos >= p_epos)
//       continue;
//     
//     EventList& el = part->nonconst_events();
//     for(iEvent ie = el.begin(); ie != el.end(); ++ie)
//     {
//       Event& e = *ie;
//       
//       unsigned e_spos  = e.frame() + p_spos;
//       unsigned nn      = e.lenFrame();
//       unsigned e_epos  = e_spos + nn;
//       
//       if(pos + frames < e_spos) 
//         break;
//       if(pos >= e_epos) 
//         continue;
// 
//       int offset = e_spos - pos;
//       
//       unsigned srcOffset, dstOffset;
//       if (offset > 0) {
//             nn = frames - offset;
//             srcOffset = 0;
//             dstOffset = offset;
//             }
//       else {
//             srcOffset = -offset;
//             dstOffset = 0;
//             
//             nn += offset;
//             if (nn > frames)
//                   nn = frames;
//             }
//             
//       float* bpp[channels];
//       for (int i = 0; i < channels; ++i)
//             bpp[i] = bp[i] + dstOffset;
//       
//       
// //       e.readAudio(part, srcOffset, bpp, channels(), nn, doSeek, false);
//       // TODO: LEFT OFF HERE: Do bpp !!!
//       e.fetchAudioData(part, pos, channels, off, frames, bp, doSeek, overwrite);
//     }
//   }
//   
// // TODO?  
// //   if(MusEGlobal::config.useDenormalBias) {
// //         // add denormal bias to outdata
// //         for (int i = 0; i < channels(); ++i)
// //               for (unsigned int j = 0; j < samples; ++j)
// //                   bp[i][j] +=MusEGlobal::denormalBias;
// //         }
// //   
// //   _prefetchFifo.add();
//   
// }
      
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
                        goto out_of_WaveTrackRead_forloop;
                  case Xml::TagStart:
                        if (tag == "part") {
                              Part* p = 0;
                              p = Part::readFromXml(xml, this);
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
                              goto out_of_WaveTrackRead_forloop;
                              }
                  default:
                        break;
                  }
            }
out_of_WaveTrackRead_forloop:
      chainTrackParts(this);
      }

//---------------------------------------------------------
//   newPart
//---------------------------------------------------------

Part* WaveTrack::newPart(Part*p, bool clone)
      {
      WavePart* part;
      if(!p)
      {
        part = new WavePart(this);
      }
      else
      {
        part = clone ? (WavePart*)p->createNewClone() : (WavePart*)p->duplicate();
        part->setTrack(this);
      }
      return part;
      }

bool WaveTrack::openAllParts()
{
  bool opened = false;
  const PartList* pl = parts();
  for(ciPart ip = pl->begin(); ip != pl->end(); ++ip)
  {
    if(ip->second->openAllEvents())
      opened = true;
  }
  return opened;
}
      
bool WaveTrack::closeAllParts()
{
  bool closed = false;
  const PartList* pl = parts();
  for(ciPart ip = pl->begin(); ip != pl->end(); ++ip)
  {
    if(ip->second->closeAllEvents())
      closed = true;
  }
  return closed;
}

// REMOVE Tim. samplerate. Added.
//---------------------------------------------------------
//   getPrefetchData
//---------------------------------------------------------

// bool WaveTrack::getPrefetchData(sf_count_t framePos, int channels, sf_count_t nframe, float** bp)
// {
//   // First, fetch any track-wide pre-mixed data, such as straight unmodified data
//   //  or from prefetch-based samplerate converters, but NOT stretchers or pitch shifters -
//   //  those belong POST-prefetch, below, so they can have fast response to changes.
//   if(MusEGlobal::audio->freewheel())
//   {
//
//     // when freewheeling, read data direct from file:
//     // Indicate do not seek file before each read.
//     // This ALREADY clears the buffer!
//     fetchData(framePos, nframe, bp, false);
//
//     // REMOVE Tim. samplerate. Added.
//     //fetchAudioData(framePos, channels, off(), nframe, bp, false, true);
//
//   }
//   else
//   {
//     // Clear the buffer.
//     for(int i = 0; i < channels; ++i)
//       memset(bp[i], 0, nframe * sizeof(float));
//
//     float* bpp[channels];
//
//     bool fail = false;
//     sf_count_t pos;
//     if(_prefetchFifo.get(channels, nframe, bpp, &pos))
//     {
//       fail = true;
//       fprintf(stderr, "WaveTrack::getPrefetchData(%s) fifo underrun (A)\n",
//         name().toLocal8Bit().constData());
//     }
//     else
//     //if(pos != framePos)
//     if(pos < framePos)
//     {
//       if(MusEGlobal::debugMsg)
//         fprintf(stderr, "fifo get error expected %ld, got %ld\n", framePos, pos);
//       while(pos < framePos)
//       {
//         if(_prefetchFifo.get(channels, nframe, bpp, &pos))
//         {
//           fail = true;
//           fprintf(stderr, "WaveTrack::getPrefetchData(%s) fifo underrun (B)\n",
//             name().toLocal8Bit().constData());
//           break;
//         }
//       }
//     }
//
//     if(!fail && pos >= framePos)
//     {
//       for(int i = 0; i < channels; ++i)
//         for(sf_count_t j = 0; j < nframe; ++j)
//           bp[i][j] += bpp[i][j];
//     }
//   }
//
//   //
//   // Now fetch data from individual wave prefetch data, such as straight unmodified data
//   //  or from POST-prefetch-based stretchers/pitch shifters (or samplerate converters).
//   //
//
// // Removed temporarily. TODO: Reinstate.
// //   // Process only if track is not off.
// //   if(!off())
// //   {
// //     PartList* pl = parts();
// //     for(iPart ip = pl->begin(); ip != pl->end(); ++ip)
// //     {
// //       Part* part = ip->second;
// //       if(part->mute())
// //         continue;
// //
// //       unsigned p_spos = part->frame();
// //       unsigned p_epos = p_spos + part->lenFrame();
// //       if(framePos + nframe < p_spos)
// //         break;
// //       if(framePos >= p_epos)
// //         continue;
// //
// //       EventList& el = part->nonconst_events();
// //       for(iEvent ie = el.begin(); ie != el.end(); ++ie)
// //       {
// //         Event& e = ie->second;
// //         unsigned e_spos  = e.frame() + p_spos;
// //         unsigned nn      = e.lenFrame();
// //         unsigned e_epos  = e_spos + nn;
// //
// //         if(framePos + nframe < e_spos)
// //           break;
// //         if(framePos >= e_epos)
// //           continue;
// //
// //         int offset = e_spos - framePos;
// //
// //         unsigned srcOffset, dstOffset;
// //         if(offset > 0)
// //         {
// //           nn = nframe - offset;
// //           srcOffset = 0;
// //           dstOffset = offset;
// //         }
// //         else
// //         {
// //           srcOffset = -offset;
// //           dstOffset = 0;
// //
// //           nn += offset;
// //           if(nn > nframe)
// //             nn = nframe;
// //         }
// //
// //         float* bpp[channels];
// //
// //         // When freewheeling, read data direct from file:
// //         if(MusEGlobal::audio->freewheel())
// //         {
// //           for(int i = 0; i < channels; ++i)
// //             bpp[i] = bp[i] + dstOffset;
// //
// //           //e.fetchAudioData(part, framePos, channels, off(), nframe, bp, false, false);
// //           //e.readAudio(part, srcOffset, bpp, channels, nn, false, false);
// //           e.readAudio(srcOffset, bpp, channels, nn, false, false);
// //
// //           if(MusEGlobal::config.useDenormalBias)
// //           {
// //             // Add denormal bias to outdata.
// //             for(int i = 0; i < channels; ++i)
// //               for(unsigned int j = 0; j < nframe; ++j)
// //                 bp[i][j] += MusEGlobal::denormalBias;
// //           }
// //           //if(e.audioPrefetchFifo())
// //           //  e.audioPrefetchFifo()->add(); // TODO ? Why was this necessary during a freewheel direct read?
// //         }
// //         else
// //         {
// //           Fifo* fifo = e.audioPrefetchFifo();
// //           if(fifo && !fifo->isEmpty())
// //           {
// //             SndFileR sf = e.sndFile();
// //             if(!sf.isNull())
// //             {
// //               const int sf_chans = sf.channels();
// //               const sf_count_t  samples = sf_chans * nframe;
// //               float* prefetch_buf;
// //               sf_count_t pos;
// //               //if(e.audioPrefetchFifo()->get(channels, nframe, bpp, &pos))
// //               //if(e.getAudioPrefetchBuffer(channels, nframe, bp, &pos))
// //
// //               // Get the ONE allocated interleaved buffer which is fed with direct interleaved soundfile data.
// //               if(fifo->get(1, samples, &prefetch_buf, &pos))
// //               {
// //                 #ifdef WAVETRACK_DEBUG
// //                 fprintf(stderr, "WaveTrack::getPrefetchData(%s) event fifo underrun (A)\n", name().toLocal8Bit().constData());
// //                 #endif
// //               }
// //               else if(pos != framePos)
// //               {
// //                 if(MusEGlobal::debugMsg)
// //                   fprintf(stderr, "event fifo get error expected %ld, got %ld\n", framePos, pos);
// //
// //                 while(pos < framePos)
// //                 {
// //                   //if(e.audioPrefetchFifo()->get(channels, nframe, bpp, &pos))
// //                   //if(e.getAudioPrefetchBuffer(channels, nframe, bp, &pos))
// //
// //                   if(fifo->get(1, samples, &prefetch_buf, &pos))
// //                   {
// //                     #ifdef WAVETRACK_DEBUG
// //                     fprintf(stderr, "WaveTrack::getPrefetchData(%s) event fifo underrun (B)\n", name().toLocal8Bit().constData());
// //                     #endif
// //                     break;
// //                   }
// //                 }
// //               }
// //
// //               if(pos >= framePos)
// //               {
// // //                 for(int i = 0; i < channels; ++i)
// // //                   for(unsigned int j = 0; j < nframe; ++j)
// // //                     bp[i][j] += bpp[i][j];
// //
// //
// //
// //                 sf_count_t rn = nframe;
// //                 float* src      = prefetch_buf;
// //                 const int dstChannels = sf_chans;
// //                 //if(srcChannels == dstChannels)
// //                 if(channels == dstChannels)
// //                 {
// // //                   if(overwrite)
// // //                     for(size_t i = 0; i < rn; ++i)
// // //                     {
// // //                       for(int ch = 0; ch < srcChannels; ++ch)
// // //                         *(dst[ch]+i) = *src++;
// // //                     }
// // //                   else
// //                     for(sf_count_t i = 0; i < rn; ++i)
// //                     {
// // //                       for(int ch = 0; ch < srcChannels; ++ch)
// // //                         *(dst[ch]+i) += *src++;
// //                       for(int ch = 0; ch < channels; ++ch)
// //                         *(bp[ch]+i) += *src++;
// //                     }
// //                 }
// // //                 else if((srcChannels == 1) && (dstChannels == 2))
// //                 else if((channels == 1) && (dstChannels == 2))
// //                 {
// //                   // stereo to mono
// // //                   if(overwrite)
// // //                     for(size_t i = 0; i < rn; ++i)
// // //                       *(dst[0] + i) = src[i + i] + src[i + i + 1];
// // //                   else
// //                     for(sf_count_t i = 0; i < rn; ++i)
// // //                       *(dst[0] + i) += src[i + i] + src[i + i + 1];
// //                       *(bp[0] + i) += src[i + i] + src[i + i + 1];
// //                 }
// // //                 else if((srcChannels == 2) && (dstChannels == 1))
// //                 else if((channels == 2) && (dstChannels == 1))
// //                 {
// //                   // mono to stereo
// // //                   if(overwrite)
// // //                     for(size_t i = 0; i < rn; ++i)
// // //                     {
// // //                       float data = *src++;
// // //                       *(dst[0]+i) = data;
// // //                       *(dst[1]+i) = data;
// // //                     }
// // //                   else
// //                     for(sf_count_t i = 0; i < rn; ++i)
// //                     {
// //                       float data = *src++;
// // //                       *(dst[0]+i) += data;
// // //                       *(dst[1]+i) += data;
// //                       *(bp[0]+i) += data;
// //                       *(bp[1]+i) += data;
// //                     }
// //                 }
// //                 else
// //                 {
// //                   //fprintf(stderr, "WaveTrack::getPrefetchData(%s) channel mismatch %d -> %d\n", name().toLocal8Bit().constData(), srcChannels, dstChannels);
// //                   fprintf(stderr, "WaveTrack::getPrefetchData(%s) channel mismatch %d -> %d\n", name().toLocal8Bit().constData(), channels, dstChannels);
// //                 }
// //
// //                 //return rn;
// //
// //
// //
// //
// //
// //
// //               }
// //             }
// //           }
// //         }
// //       }
// //     }
// //   }
//
//   return true;
// }

bool WaveTrack::getPrefetchData(sf_count_t framePos, int dstChannels, sf_count_t nframe, float** bp, bool overwrite)
{
  float* pf_buf[dstChannels];
  
  // First, fetch any track-wide pre-mixed data, such as straight unmodified data
  //  or from prefetch-based samplerate converters, but NOT stretchers or pitch shifters -
  //  those belong POST-prefetch, below, so they can have fast response to changes.
  if(MusEGlobal::audio->freewheel())
  {

//     // when freewheeling, read data direct from file:
//     // Indicate do not seek file before each read.
//     // This ALREADY clears the buffer!
// //     fetchData(framePos, nframe, bp, false);
//     fetchData(framePos, nframe, bp, false, overwrite);
// 
//     // REMOVE Tim. samplerate. Added.
//     //fetchAudioData(framePos, channels, off(), nframe, bp, false, true);

    
    // when freewheeling, read data direct from file:
    if(isMute())
    {
      // We are muted. We need to let the fetching progress, but discard the data.
      for(int i = 0; i < dstChannels; ++i)
        // Set to the audio dummy buffer.
        pf_buf[i] = audioOutDummyBuf;
      // Indicate do not seek file before each read.
      fetchData(framePos, nframe, pf_buf, false, overwrite);
      // Just return whether we have input sources data.
//       return have_data;
      return false;
    }
    else
    {
      // Not muted. Fetch the data into the given buffers.
      // Indicate do not seek file before each read.
      fetchData(framePos, nframe, bp, false, overwrite);
      // We have data.
      return true;
    }
    
    
  }
  else
  {
//     // Clear the buffer.
//     for(int i = 0; i < channels; ++i)
//       memset(bp[i], 0, nframe * sizeof(float));
//
//     float* bpp[channels];
//
//     bool fail = false;
//     sf_count_t pos;
//     if(_prefetchFifo.get(channels, nframe, bpp, &pos))
//     {
//       fail = true;
//       fprintf(stderr, "WaveTrack::getPrefetchData(%s) fifo underrun (A)\n",
//         name().toLocal8Bit().constData());
//     }
//     else
//     //if(pos != framePos)
//     if(pos < framePos)
//     {
//       if(MusEGlobal::debugMsg)
//         fprintf(stderr, "fifo get error expected %ld, got %ld\n", framePos, pos);
//       while(pos < framePos)
//       {
//         if(_prefetchFifo.get(channels, nframe, bpp, &pos))
//         {
//           fail = true;
//           fprintf(stderr, "WaveTrack::getPrefetchData(%s) fifo underrun (B)\n",
//             name().toLocal8Bit().constData());
//           break;
//         }
//       }
//     }


    sf_count_t pos;

    if(_prefetchFifo.get(dstChannels, nframe, pf_buf, &pos))
    {
      fprintf(stderr, "WaveTrack::getPrefetchData(%s) (A) fifo underrun\n", name().toLocal8Bit().constData());
      return false;
    }
//     if(pos != framePos)
    if(pos < framePos)
    {
      if(MusEGlobal::debugMsg)
        fprintf(stderr, "fifo get error expected %ld, got %ld\n", framePos, pos);
      while (pos < framePos)
      {
        if(_prefetchFifo.get(dstChannels, nframe, pf_buf, &pos))
        {
          fprintf(stderr, "WaveTrack::getPrefetchData(%s) (B) fifo underrun\n",
              name().toLocal8Bit().constData());
          return false;
        }
      }
    }

    if(isMute())
    {
      // We are muted. We need to let the fetching progress, but discard the data.
      // Just return whether we have input sources data.
//       return have_data;
      return false;
    }

//     if(!fail && pos >= framePos)
//     {
//       for(int i = 0; i < channels; ++i)
//         for(sf_count_t j = 0; j < nframe; ++j)
//           bp[i][j] += bpp[i][j];
//     }




    if(overwrite)
    {
      for(int i = 0; i < dstChannels; ++i)
        AL::dsp->cpy(bp[i], pf_buf[i], nframe, MusEGlobal::config.useDenormalBias);
    }
    else
    {
      for(int i = 0; i < dstChannels; ++i)
        AL::dsp->mix(bp[i], pf_buf[i], nframe);
    }
  }

  //
  // Now fetch data from individual wave prefetch data, such as straight unmodified data
  //  or from POST-prefetch-based stretchers/pitch shifters (or samplerate converters).
  //

// Removed temporarily. TODO: Reinstate.
//   // Process only if track is not off.
//   if(!off())
//   {
//     PartList* pl = parts();
//     for(iPart ip = pl->begin(); ip != pl->end(); ++ip)
//     {
//       Part* part = ip->second;
//       if(part->mute())
//         continue;
//
//       unsigned p_spos = part->frame();
//       unsigned p_epos = p_spos + part->lenFrame();
//       if(framePos + nframe < p_spos)
//         break;
//       if(framePos >= p_epos)
//         continue;
//
//       EventList& el = part->nonconst_events();
//       for(iEvent ie = el.begin(); ie != el.end(); ++ie)
//       {
//         Event& e = ie->second;
//         unsigned e_spos  = e.frame() + p_spos;
//         unsigned nn      = e.lenFrame();
//         unsigned e_epos  = e_spos + nn;
//
//         if(framePos + nframe < e_spos)
//           break;
//         if(framePos >= e_epos)
//           continue;
//
//         int offset = e_spos - framePos;
//
//         unsigned srcOffset, dstOffset;
//         if(offset > 0)
//         {
//           nn = nframe - offset;
//           srcOffset = 0;
//           dstOffset = offset;
//         }
//         else
//         {
//           srcOffset = -offset;
//           dstOffset = 0;
//
//           nn += offset;
//           if(nn > nframe)
//             nn = nframe;
//         }
//
//         float* bpp[channels];
//
//         // When freewheeling, read data direct from file:
//         if(MusEGlobal::audio->freewheel())
//         {
//           for(int i = 0; i < channels; ++i)
//             bpp[i] = bp[i] + dstOffset;
//
//           //e.fetchAudioData(part, framePos, channels, off(), nframe, bp, false, false);
//           //e.readAudio(part, srcOffset, bpp, channels, nn, false, false);
//           e.readAudio(srcOffset, bpp, channels, nn, false, false);
//
//           if(MusEGlobal::config.useDenormalBias)
//           {
//             // Add denormal bias to outdata.
//             for(int i = 0; i < channels; ++i)
//               for(unsigned int j = 0; j < nframe; ++j)
//                 bp[i][j] += MusEGlobal::denormalBias;
//           }
//           //if(e.audioPrefetchFifo())
//           //  e.audioPrefetchFifo()->add(); // TODO ? Why was this necessary during a freewheel direct read?
//         }
//         else
//         {
//           Fifo* fifo = e.audioPrefetchFifo();
//           if(fifo && !fifo->isEmpty())
//           {
//             SndFileR sf = e.sndFile();
//             if(!sf.isNull())
//             {
//               const int sf_chans = sf.channels();
//               const sf_count_t  samples = sf_chans * nframe;
//               float* prefetch_buf;
//               sf_count_t pos;
//               //if(e.audioPrefetchFifo()->get(channels, nframe, bpp, &pos))
//               //if(e.getAudioPrefetchBuffer(channels, nframe, bp, &pos))
//
//               // Get the ONE allocated interleaved buffer which is fed with direct interleaved soundfile data.
//               if(fifo->get(1, samples, &prefetch_buf, &pos))
//               {
//                 #ifdef WAVETRACK_DEBUG
//                 fprintf(stderr, "WaveTrack::getPrefetchData(%s) event fifo underrun (A)\n", name().toLocal8Bit().constData());
//                 #endif
//               }
//               else if(pos != framePos)
//               {
//                 if(MusEGlobal::debugMsg)
//                   fprintf(stderr, "event fifo get error expected %ld, got %ld\n", framePos, pos);
//
//                 while(pos < framePos)
//                 {
//                   //if(e.audioPrefetchFifo()->get(channels, nframe, bpp, &pos))
//                   //if(e.getAudioPrefetchBuffer(channels, nframe, bp, &pos))
//
//                   if(fifo->get(1, samples, &prefetch_buf, &pos))
//                   {
//                     #ifdef WAVETRACK_DEBUG
//                     fprintf(stderr, "WaveTrack::getPrefetchData(%s) event fifo underrun (B)\n", name().toLocal8Bit().constData());
//                     #endif
//                     break;
//                   }
//                 }
//               }
//
//               if(pos >= framePos)
//               {
// //                 for(int i = 0; i < channels; ++i)
// //                   for(unsigned int j = 0; j < nframe; ++j)
// //                     bp[i][j] += bpp[i][j];
//
//
//
//                 sf_count_t rn = nframe;
//                 float* src      = prefetch_buf;
//                 const int dstChannels = sf_chans;
//                 //if(srcChannels == dstChannels)
//                 if(channels == dstChannels)
//                 {
// //                   if(overwrite)
// //                     for(size_t i = 0; i < rn; ++i)
// //                     {
// //                       for(int ch = 0; ch < srcChannels; ++ch)
// //                         *(dst[ch]+i) = *src++;
// //                     }
// //                   else
//                     for(sf_count_t i = 0; i < rn; ++i)
//                     {
// //                       for(int ch = 0; ch < srcChannels; ++ch)
// //                         *(dst[ch]+i) += *src++;
//                       for(int ch = 0; ch < channels; ++ch)
//                         *(bp[ch]+i) += *src++;
//                     }
//                 }
// //                 else if((srcChannels == 1) && (dstChannels == 2))
//                 else if((channels == 1) && (dstChannels == 2))
//                 {
//                   // stereo to mono
// //                   if(overwrite)
// //                     for(size_t i = 0; i < rn; ++i)
// //                       *(dst[0] + i) = src[i + i] + src[i + i + 1];
// //                   else
//                     for(sf_count_t i = 0; i < rn; ++i)
// //                       *(dst[0] + i) += src[i + i] + src[i + i + 1];
//                       *(bp[0] + i) += src[i + i] + src[i + i + 1];
//                 }
// //                 else if((srcChannels == 2) && (dstChannels == 1))
//                 else if((channels == 2) && (dstChannels == 1))
//                 {
//                   // mono to stereo
// //                   if(overwrite)
// //                     for(size_t i = 0; i < rn; ++i)
// //                     {
// //                       float data = *src++;
// //                       *(dst[0]+i) = data;
// //                       *(dst[1]+i) = data;
// //                     }
// //                   else
//                     for(sf_count_t i = 0; i < rn; ++i)
//                     {
//                       float data = *src++;
// //                       *(dst[0]+i) += data;
// //                       *(dst[1]+i) += data;
//                       *(bp[0]+i) += data;
//                       *(bp[1]+i) += data;
//                     }
//                 }
//                 else
//                 {
//                   //fprintf(stderr, "WaveTrack::getPrefetchData(%s) channel mismatch %d -> %d\n", name().toLocal8Bit().constData(), srcChannels, dstChannels);
//                   fprintf(stderr, "WaveTrack::getPrefetchData(%s) channel mismatch %d -> %d\n", name().toLocal8Bit().constData(), channels, dstChannels);
//                 }
//
//                 //return rn;
//
//
//
//
//
//
//               }
//             }
//           }
//         }
//       }
//     }
//   }

  return true;
}

//---------------------------------------------------------
//   getData
//---------------------------------------------------------

bool WaveTrack::getData(unsigned framePos, int dstChannels, unsigned nframe, float** bp)
{
  bool have_data = false;
  
  const bool track_rec_flag = recordFlag();
  const bool track_rec_monitor = recMonitor();        // Separate monitor and record functions.

  //---------------------------------------------
  // Contributions to data from input sources:
  //---------------------------------------------

  // Gather input data from connected routes.
  if((MusEGlobal::song->bounceTrack != this) && !noInRoute())
  {
    have_data = AudioTrack::getData(framePos, dstChannels, nframe, bp);
    // Do we want to record the incoming data?
    if(have_data && track_rec_flag && MusEGlobal::audio->isRecording() && recFile())
    {
      if(MusEGlobal::audio->freewheel())
      {
      }
      else
      {
        if(fifo.put(dstChannels, nframe, bp, MusEGlobal::audio->pos().frame()))
          printf("WaveTrack::getData(%d, %d, %d): fifo overrun\n", framePos, dstChannels, nframe);
      }
    }
  }

  //---------------------------------------------
  // Contributions to data from playback sources:
  //---------------------------------------------

 if(!MusEGlobal::audio->isPlaying())
  {
    if(!have_data || (track_rec_monitor && have_data))
      return have_data;
    return false;
  }

  // If there is no input source data or we do not want to monitor it,
  //  overwrite the supplied buffers rather than mixing with them.
  const bool do_overwrite = !have_data || !track_rec_monitor;

  // Set the return value.
  have_data = !have_data || (track_rec_monitor && have_data);
#if 0 // REMOVE Tim. samplerate. From master. Move into getPrefetchData().
  float* pf_buf[dstChannels];
  
  if(MusEGlobal::audio->freewheel())
  {
    // when freewheeling, read data direct from file:
    if(isMute())
    {
      // We are muted. We need to let the fetching progress, but discard the data.
      for(int i = 0; i < dstChannels; ++i)
        // Set to the audio dummy buffer.
        pf_buf[i] = audioOutDummyBuf;
      // Indicate do not seek file before each read.
      fetchData(framePos, nframe, pf_buf, false, do_overwrite);
      // Just return whether we have input sources data.
      return have_data;
    }
    else
    {
      // Not muted. Fetch the data into the given buffers.
      // Indicate do not seek file before each read.
      fetchData(framePos, nframe, bp, false, do_overwrite);
      // We have data.
      return true;
    }
  }
  else
  {
    unsigned pos;
    if(_prefetchFifo.get(dstChannels, nframe, pf_buf, &pos))
    {
      fprintf(stderr, "WaveTrack::getData(%s) (A) fifo underrun\n", name().toLocal8Bit().constData());
      return have_data;
    }
    if(pos != framePos)
    {
      if(MusEGlobal::debugMsg)
        fprintf(stderr, "fifo get error expected %d, got %d\n", framePos, pos);
      while (pos < framePos)
      {
        if(_prefetchFifo.get(dstChannels, nframe, pf_buf, &pos))
        {
          fprintf(stderr, "WaveTrack::getData(%s) (B) fifo underrun\n",
              name().toLocal8Bit().constData());
          return have_data;
        }
      }
    }

    if(isMute())
    {
      // We are muted. We need to let the fetching progress, but discard the data.
      // Just return whether we have input sources data.
      return have_data;
    }

    if(do_overwrite)
    {
      for(int i = 0; i < dstChannels; ++i)
        AL::dsp->cpy(bp[i], pf_buf[i], nframe, MusEGlobal::config.useDenormalBias);
    }
    else
    {
      for(int i = 0; i < dstChannels; ++i)
        AL::dsp->mix(bp[i], pf_buf[i], nframe);
    }
    // We have data.
    return true;
  }
#endif
  const bool have_pf_data = getPrefetchData(framePos, dstChannels, nframe, bp, do_overwrite);
//   return have_data;
  return have_data || have_pf_data;

//   if(MusEGlobal::audio->freewheel())
//   {
//     // when freewheeling, read data direct from file:
//     // Indicate do not seek file before each read.
//     fetchData(framePos, nframe, bp, false, do_overwrite);
//   }
//   else
//   {
//     unsigned pos;
//     float* pf_buf[dstChannels];
//
//     if(_prefetchFifo.get(dstChannels, nframe, pf_buf, &pos))
//     {
//       printf("WaveTrack::getData(%s) (A) fifo underrun\n", name().toLocal8Bit().constData());
//       return have_data;
//     }
//     if(pos != framePos)
//     {
//       if(MusEGlobal::debugMsg)
//         printf("fifo get error expected %d, got %d\n", framePos, pos);
//       while (pos < framePos)
//       {
//         if(_prefetchFifo.get(dstChannels, nframe, pf_buf, &pos))
//         {
//           printf("WaveTrack::getData(%s) (B) fifo underrun\n",
//               name().toLocal8Bit().constData());
//           return have_data;
//         }
//       }
//     }
//
//     if(do_overwrite)
//     {
//       for(int i = 0; i < dstChannels; ++i)
//         AL::dsp->cpy(bp[i], pf_buf[i], nframe, MusEGlobal::config.useDenormalBias);
//     }
//     else
//     {
//       for(int i = 0; i < dstChannels; ++i)
//         AL::dsp->mix(bp[i], pf_buf[i], nframe);
//     }
//   }
//
//   return have_data;
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

// REMOVE Tim. samplerate. Added.
void WaveTrack::clearPrefetchFifo()
{ 
  _prefetchFifo.clear();
  
  PartList* pl = parts();
  for(iPart ip = pl->begin(); ip != pl->end(); ++ip)
  {
    Part* part = ip->second;
    EventList& el = part->nonconst_events();
    for(iEvent ie = el.begin(); ie != el.end(); ++ie)
    {
      Event& e = ie->second;
      if(e.audioPrefetchFifo())
        e.audioPrefetchFifo()->clear();
    }
  }
}

// REMOVE Tim. samplerate. Added.
// void WaveTrack::prefetchAudio(unsigned writePos, int channels, bool off, unsigned frames)
// {
//   float* bp[channels];
//   PartList* pl = parts();
//   for(iPart ip = pl->begin(); ip != pl->end(); ++ip)
//   {
//     Part* part = ip->second;
//     EventList& el = part->nonconst_events();
//     for(iEvent ie = el.begin(); ie != el.end(); ++ie)
//     {
//       Event& e = ie->second;
//       if(e.audioPrefetchFifo())
//       {
//         //e.prefetchAudio(part, writePos, channels, off, frames);
//         
//         if(e.audioPrefetchFifo()->getWriteBuffer(channels, frames, bp, writePos))
//           continue;
// 
//         //e.fetchAudioData(part, writePos, ch, MusEGlobal::segmentSize, bp, false);
// 
//         // Reset buffer to zero.
//         // TODO: Optimize this by only clearing what's required, and merging with the denormal code below.
//         for(int i = 0; i < channels; ++i)
//           memset(bp[i], 0, frames * sizeof(float));
//         
//         unsigned p_spos = part->frame();
//         unsigned p_epos = p_spos + part->lenFrame();
// 
//         unsigned e_spos  = e.frame() + p_spos;
//         unsigned nn      = e.lenFrame();
//         unsigned e_epos  = e_spos + nn;
//         
// //         if(!off)
//         if(!off && !part->mute() && 
//           writePos + frames >= p_spos && writePos < p_epos &&
//           writePos + frames >= e_spos && writePos < e_epos)
//         {  
// //           if(part->mute())
// //             continue;
//           
// //           unsigned p_spos = part->frame();
// //           unsigned p_epos = p_spos + part->lenFrame();
// //           if(writePos + frames < p_spos)
// //             break;
// //           if(writePos >= p_epos)
// //             continue;
// 
// //             unsigned e_spos  = e.frame() + p_spos;
// //             unsigned nn      = e.lenFrame();
// //             unsigned e_epos  = e_spos + nn;
//             
// //             if(writePos + frames < e_spos) 
// //               break;
// //             if(writePos >= e_epos) 
// //               continue;
//             
//             
//             int offset = e_spos - writePos;
// 
//             unsigned srcOffset, dstOffset;
//             if(offset > 0) 
//             {
//               nn = frames - offset;
//               srcOffset = 0;
//               dstOffset = offset;
//             }
//             else 
//             {
//               srcOffset = -offset;
//               dstOffset = 0;
//               
//               nn += offset;
//               if(nn > frames)
//                 nn = frames;
//             }
//             float* bpp[channels];
//             for(int i = 0; i < channels; ++i)
//               bpp[i] = bp[i] + dstOffset;
// 
//             //e.readAudio(part, srcOffset, bpp, channels, nn, false, false);
//             e.readAudio(srcOffset, bpp, channels, nn, false, false);
//           //}
//         }
//         
//         // Add denormal bias to outdata.
//         if(MusEGlobal::config.useDenormalBias) 
//         {
//           for(int i = 0; i < channels; ++i)
//             for(unsigned int j = 0; j < frames; ++j)
//               bp[i][j] += MusEGlobal::denormalBias;
//         }
//         
//         // Increment the event's prefetch buffer to a new position.
//         e.audioPrefetchFifo()->add();
//       }
//     }
//   }
//   
//   
// }


// REMOVE Tim. samplerate. Added.
void WaveTrack::prefetchAudio(sf_count_t /*writePos*/, sf_count_t frames)
{
  if(off())
    return;
  
  PartList* pl = parts();
  for(iPart ip = pl->begin(); ip != pl->end(); ++ip)
  {
    Part* part = ip->second;
    if(part->mute())
      continue;
    
    EventList& el = part->nonconst_events();
    for(iEvent ie = el.begin(); ie != el.end(); ++ie)
    {
      Event& e = ie->second;
      
      if(!e.audioPrefetchFifo())
        continue;
      
//       const sf_count_t p_spos  = part->frame();
//       const sf_count_t p_epos  = p_spos + part->lenFrame();
// 
//       const sf_count_t e_spos  = e.frame() + p_spos;
//       sf_count_t nn            = e.lenFrame();
//       const sf_count_t e_epos  = e_spos + nn;
//       
//       if(writePos + frames >= p_spos && writePos < p_epos &&
//          writePos + frames >= e_spos && writePos < e_epos)
//       {  
//         sf_count_t offset = e_spos - writePos;
// 
// //         unsigned srcOffset, dstOffset;
//         if(offset > 0) 
//         {
//           nn = frames - offset;
// //           srcOffset = 0;
// //           dstOffset = offset;
//         }
//         else 
//         {
// //           srcOffset = -offset;
// //           dstOffset = 0;
//           
//           nn += offset;
//           if(nn > frames)
//             nn = frames;
//         }
//         
//         //e.prefetchAudio(writePos, nn);
//         //e.prefetchAudio(part, nn);
        e.prefetchAudio(part, frames);
//       }
      
//       SndFileR sf = e.sndFile();
//       if(sf.isNull())
//         continue;
//       
//       const int chans = sf.channels();
//       const long unsigned int samples = chans * frames;
//       
//       // Here we allocate ONE interleaved buffer which is fed with direct interleaved soundfile data.
//       float* bp;
//       if(e.audioPrefetchFifo()->getWriteBuffer(1, samples, &bp, writePos))
//         continue;
// 
//       // Clear the buffer.  TODO: Optimize this by only clearing what's required, and merge with the denormal code below.
//       memset(bp, 0, samples * sizeof(float));
// 
//       //if(!off)
//       if(!off && !part->mute() && 
//         writePos + frames >= p_spos && writePos < p_epos &&
//         writePos + frames >= e_spos && writePos < e_epos)
//       {  
//         int offset = e_spos - writePos;
// 
//         unsigned srcOffset, dstOffset;
//         if(offset > 0) 
//         {
//           nn = frames - offset;
//           srcOffset = 0;
//           dstOffset = offset;
//         }
//         else 
//         {
//           srcOffset = -offset;
//           dstOffset = 0;
//           
//           nn += offset;
//           if(nn > frames)
//             nn = frames;
//         }
//         
//         
//         float* bpp[channels];
//         for(int i = 0; i < channels; ++i)
//           bpp[i] = bp[i] + dstOffset;
// 
//         //e.readAudio(part, srcOffset, bpp, channels, nn, false, false);
//         e.readAudio(srcOffset, bpp, channels, nn, false, false);
//         //e.readAudioDirect(srcOffset, bpp, channels, nn, false, false);
//           
//       }
//       
//       // Add denormal bias to outdata.
//       if(MusEGlobal::config.useDenormalBias) 
//       {
//         //for(int i = 0; i < channels; ++i)
//         //  for(unsigned int j = 0; j < frames; ++j)
//           for(unsigned int i = 0; i < samples; ++i)
//             bp[i] += MusEGlobal::denormalBias;
//       }
//       
//       // Increment the event's prefetch buffer to a new position.
//       e.audioPrefetchFifo()->add();
      
    }
  }
}



} // namespace MusECore
