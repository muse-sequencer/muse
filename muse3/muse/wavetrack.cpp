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

#include <stdint.h>

#include "muse_math.h"
#include "track.h"
#include "event.h"
#include "audio.h"
#include "wave.h"
#include "xml.h"
#include "song.h"
#include "globals.h"
#include "gconfig.h"
#include "al/dsp.h"
#include "audioprefetch.h"
#include "latency_compensator.h"

// Turn on some cool terminal 'peak' meters for debugging
//  presence of actual audio at various places
// #define NODE_DEBUG_TERMINAL_PEAK_METERS

// For debugging output: Uncomment the fprintf section.
#define WAVETRACK_DEBUG(dev, format, args...) // fprintf(dev, format, ##args)

namespace MusECore {

//---------------------------------------------------------
//   WaveTrack
//---------------------------------------------------------

// Default 1 channel for wave tracks.
WaveTrack::WaveTrack() : AudioTrack(Track::WAVE, 1)
{
  _prefetchWritePos = ~0;
}

WaveTrack::WaveTrack(const WaveTrack& wt, int flags) : AudioTrack(wt, flags)
{
  _prefetchWritePos = ~0;

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

//---------------------------------------------------------
//   seekData
//    called from prefetch thread
//---------------------------------------------------------

void WaveTrack::seekData(sf_count_t pos)
      {
      WAVETRACK_DEBUG(stderr, "WaveTrack::seekData %s pos:%ld\n", name().toLatin1().constData(), pos);
      
      PartList* pl = parts();
      for (iPart ip = pl->begin(); ip != pl->end(); ++ip) {
            WavePart* part = (WavePart*)(ip->second);
            unsigned p_spos = part->frame();
            EventList& el = part->nonconst_events();
            for (iEvent ie = el.begin(); ie != el.end(); ++ie) {
                  Event& event = ie->second;
                  unsigned e_spos  = event.frame() + p_spos;
                  sf_count_t offset = pos - e_spos;
                  if(offset < 0)
                    offset = 0;
                  event.seekAudio(offset);
                  }
            }
      }
      
//---------------------------------------------------------
//   fetchData
//    called from prefetch thread
//---------------------------------------------------------

void WaveTrack::fetchData(unsigned pos, unsigned samples, float** bp, bool doSeek, bool overwrite, int latency_correction)
      {
      WAVETRACK_DEBUG(stderr, "WaveTrack::fetchData %s samples:%u pos:%u overwrite:%d\n",
                      name().toLatin1().constData(), samples, pos, overwrite);

      // reset buffer to zero
      if(overwrite)
        for (int i = 0; i < channels(); ++i)
            memset(bp[i], 0, samples * sizeof(float));

      // Process only if track is not off.
      if(!off())
      {
        const bool use_latency_corr = useLatencyCorrection();
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

                    if(use_latency_corr)
                    {
                      // Don't bother trying to read anything that comes before sample zero,
                      //  or limiting to zero which would just repeat the same beginning section over.
                      
                      // REMOVE Tim. latency. Added. Comment.
                      // TODO: Change this: Insert blanks and use what we can from the buffer!
                      
                      if(latency_correction > 0 && (unsigned int)latency_correction > srcOffset)
                        continue;
                      // Move the source FORWARD by an amount necessary for latency correction.
                      // i_correction will be negative for correction.
                      srcOffset -= latency_correction;
                    }

                    float* bpp[channels()];
                    for (int i = 0; i < channels(); ++i)
                          bpp[i] = bp[i] + dstOffset;

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

bool WaveTrack::getPrefetchData(
     bool have_data, sf_count_t framePos, int dstChannels, sf_count_t nframe, float** bp, bool do_overwrite)
{
  const bool use_latency_corr = useLatencyCorrection();

  float* pf_buf[dstChannels];

  int i_correction = 0;
  if(use_latency_corr)
  {
    const TrackLatencyInfo& li = getLatencyInfo(false);
    const float correction = li._sourceCorrectionValue;
    i_correction = correction;
  }

  // First, fetch any track-wide pre-mixed data, such as straight unmodified data
  //  or from prefetch-based samplerate converters, but NOT stretchers or pitch shifters -
  //  those belong POST-prefetch, below, so they can have fast response to changes.
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
      fetchData(framePos, nframe, pf_buf, false, do_overwrite, i_correction);
      // Just return whether we have input sources data.
      return have_data;
    }
    else
    {
      // Not muted. Fetch the data into the given buffers.
      // Indicate do not seek file before each read.
      fetchData(framePos, nframe, bp, false, do_overwrite, i_correction);
      // We have data.
      return true;
    }
  }
  else
  {
    bool ret_val = have_data;
    MuseCount_t pos;
    if(_prefetchFifo.peek(dstChannels, nframe, pf_buf, &pos))
    {
      fprintf(stderr, "WaveTrack::getPrefetchData(%s) (prefetch peek A) fifo underrun\n", name().toLocal8Bit().constData());
      return have_data;
    }

    //fprintf(stderr, "WaveTrack::getData(%s) (prefetch peek A) pos:%d\n", name().toLocal8Bit().constData(), pos);

    const int64_t frame_pos          = framePos;
    const int64_t corr_frame_pos     = framePos - i_correction;
    const int64_t corr_frame_end_pos = framePos - i_correction + nframe;

    // Do we need to RETARD, or ADVANCE, the stream?
    if(corr_frame_end_pos <= pos)
    {
      // Allow the stream to RETARD. (That is, let our requested frame catch up to the stream.)
      return have_data;
    }
    else
    {
      // Allow the stream to ADVANCE if necessary. (That is, let the stream catch up to our requested frame.)
      while(corr_frame_pos >= pos + nframe)
      {
        // Done with buffer, remove it.
        _prefetchFifo.remove();

        if(_prefetchFifo.peek(dstChannels, nframe, pf_buf, &pos))
        {
          fprintf(stderr, "WaveTrack::getPrefetchData(%s) (prefetch peek B) fifo underrun\n", name().toLocal8Bit().constData());
          return have_data;
        }

        if(corr_frame_end_pos <= pos)
        {
          if(MusEGlobal::debugMsg)
            fprintf(stderr, "fifo get(%s) (A) error expected %ld, got %ld\n", name().toLocal8Bit().constData(), frame_pos, pos);
          return have_data;
        }
      }
    }

    if(corr_frame_pos <= pos)
    {
      if(!isMute())
      {
        const unsigned blanks = pos - corr_frame_pos;
        const unsigned buf2_frames = nframe - blanks;
        if(do_overwrite)
        {
          if(blanks != 0)
          {
            for(int i = 0; i < dstChannels; ++i)
              AL::dsp->clear(bp[i], blanks, MusEGlobal::config.useDenormalBias);
          }
          for(int i = 0; i < dstChannels; ++i)
            AL::dsp->cpy(bp[i] + blanks, pf_buf[i], buf2_frames, MusEGlobal::config.useDenormalBias);
        }
        else
        {
          for(int i = 0; i < dstChannels; ++i)
            AL::dsp->mix(bp[i] + blanks, pf_buf[i], buf2_frames);
        }
        // We have data.
        ret_val = true;
      }
      // If the entire buffer was used, we are done with it.
      if(corr_frame_pos == pos)
      {
        // Done with buffer, remove it.
        _prefetchFifo.remove();
      }
    }
    else
    {
      // This will always be at least 1, ie. corr_frame_pos > pos.
      const unsigned buf1_pos = corr_frame_pos - pos;
      const unsigned buf1_frames = nframe - buf1_pos;
      const unsigned buf2_pos = buf1_frames;
      const unsigned buf2_frames = buf1_pos;
      if(!isMute())
      {
        if(do_overwrite)
        {
          for(int i = 0; i < dstChannels; ++i)
            AL::dsp->cpy(bp[i], pf_buf[i] + buf1_pos, buf1_frames, MusEGlobal::config.useDenormalBias);
        }
        else
        {
          for(int i = 0; i < dstChannels; ++i)
            AL::dsp->mix(bp[i], pf_buf[i] + buf1_pos, buf1_frames);
        }
      }

      // Done with buffer, remove it.
      _prefetchFifo.remove();

      // We are expecting the next buffer.
      const MuseCount_t expect_nextpos = pos + nframe;

      // Peek the next buffer but do not remove it,
      //  since the rest of it will be required next cycle.
      if(_prefetchFifo.peek(dstChannels, nframe, pf_buf, &pos))
      {
        fprintf(stderr, "WaveTrack::getPrefetchData(%s) (prefetch peek C) fifo underrun\n", name().toLocal8Bit().constData());
        return have_data;
      }
      
      if(pos != expect_nextpos)
      {
        if(MusEGlobal::debugMsg)
          fprintf(stderr, "fifo get(%s) (B) error expected %ld, got %ld\n", name().toLocal8Bit().constData(), expect_nextpos, pos);
        return have_data;
      }

      if(!isMute())
      {
        if(do_overwrite)
        {
          for(int i = 0; i < dstChannels; ++i)
            AL::dsp->cpy(bp[i] + buf2_pos, pf_buf[i], buf2_frames, MusEGlobal::config.useDenormalBias);
        }
        else
        {
          for(int i = 0; i < dstChannels; ++i)
            AL::dsp->mix(bp[i] + buf2_pos, pf_buf[i], buf2_frames);
        }
        // We have data.
        ret_val = true;
      }        
    }

    return ret_val;
  }
}

//---------------------------------------------------------
//   getDataPrivate
//    return false if no data available
//---------------------------------------------------------

bool WaveTrack::getInputData(unsigned pos, int channels, unsigned nframes,
                             bool* usedInChannelArray, float** buffer)
      {
      // use supplied buffers
      const RouteList* rl = inRoutes();
      const bool use_latency_corr = useLatencyCorrection();

      #ifdef NODE_DEBUG_PROCESS
      fprintf(stderr, "AudioTrack::getData name:%s channels:%d inRoutes:%d\n", name().toLatin1().constData(), channels, int(rl->size()));
      #endif

      int dst_ch, dst_chs, src_ch, src_chs, fin_dst_chs, next_chan, i;
      unsigned long int l;
      
      bool have_data = false;
      
      for (ciRoute ir = rl->begin(); ir != rl->end(); ++ir) {
            if(ir->type != Route::TRACK_ROUTE || !ir->track || ir->track->isMidiTrack())
              continue;

            // Only this track knows how many destination channels there are,
            //  while only the route track knows how many source channels there are.
            // So take care of the destination channels here, and let the route track handle the source channels.
            dst_ch = ir->channel <= -1 ? 0 : ir->channel;
            if(dst_ch >= channels)
              continue;
            dst_chs = ir->channels <= -1 ? channels : ir->channels;
            src_ch = ir->remoteChannel <= -1 ? 0 : ir->remoteChannel;
            src_chs = ir->channels;

            fin_dst_chs = dst_chs;
            if(dst_ch + fin_dst_chs > channels)
              fin_dst_chs = channels - dst_ch;

            #ifdef NODE_DEBUG_PROCESS
            fprintf(stderr, "    calling copy/addData on %s dst_ch:%d dst_chs:%d fin_dst_chs:%d src_ch:%d src_chs:%d ...\n",
                    ir->track->name().toLatin1().constData(),
                    dst_ch, dst_chs, fin_dst_chs,
                    src_ch, src_chs);
            #endif

            static_cast<AudioTrack*>(ir->track)->copyData(pos,
                                                          dst_ch, dst_chs, fin_dst_chs,
                                                          src_ch, src_chs,
                                                          nframes, buffer,
                                                          false, use_latency_corr ? nullptr : usedInChannelArray);

            
#ifdef NODE_DEBUG_TERMINAL_PEAK_METERS
            if(MusEGlobal::audio->isPlaying())
            {
              fprintf(stderr, "WaveTrack::getInputData() name:%s ir->latency:%lu latencyCompWriteOffset:%lu total:%lu\n",
                      name().toLatin1().constData(), l, latencyCompWriteOffset(), l + latencyCompWriteOffset());
              for(int ch = 0; ch < channels; ++ch)
              {
                fprintf(stderr, "channel:%d peak:", ch);
                float val;
                float peak = 0.0f;
                const float* buf = buffer[ch];
                for(unsigned int smp = 0; smp < nframes; ++smp)
                {
                  val = buf[smp];
                  if(val > peak)
                    peak = val;
                }
                const int dots = peak * 20;
                for(int d = 0; d < dots; ++d)
                  fprintf(stderr, "*");
                fprintf(stderr, "\n");
              }
            }
#endif

            // Prepare the latency value to be passed to the compensator's writer,
            //  by adjusting each route latency value. ie. the route with the worst-case
            //  latency will get ZERO delay, while routes having smaller latency will get
            //  MORE delay, to match all the signal timings together.
            // The route's audioLatencyOut should have already been calculated and
            //  conveniently stored in the route.
            if((long int)ir->audioLatencyOut < 0)
              l = 0;
            else
              l = ir->audioLatencyOut;
            
            next_chan = dst_ch + fin_dst_chs;
            for(i = dst_ch; i < next_chan; ++i)
            {
              if(use_latency_corr)
              {
                // Write the buffers to the latency compensator.
                // By now, each copied channel should have the same latency. 
                _latencyComp->write(i, nframes, l + latencyCompWriteOffset(), buffer[i]);
              }
              usedInChannelArray[i] = true;
            }
            have_data = true;
            }

      return have_data;
      }

//---------------------------------------------------------
//   getData
//---------------------------------------------------------

bool WaveTrack::getData(unsigned framePos, int dstChannels, unsigned nframe, float** bp)
{
  bool have_data = false;
  
  const bool track_rec_flag = recordFlag();
  const bool track_rec_monitor = recMonitor();        // Separate monitor and record functions.
  const bool is_playing = MusEGlobal::audio->isPlaying();
  const bool use_latency_corr = useLatencyCorrection();

  //---------------------------------------------
  // Note that the supplied buffers (bp) are at first
  //  used as temporary storage but are later written
  //  with final data. The reading and writing of fifo
  //  file data wants linear memory whereas the latency
  //  compensator uses wrap-around memory.
  //---------------------------------------------
  
  //---------------------------------------------
  // Contributions to data from input sources:
  //---------------------------------------------

  // Gather input data from connected routes.
  if((MusEGlobal::song->bounceTrack != this) && !noInRoute())
  {
    bool used_in_chan_array[dstChannels];
    for(int i = 0; i < dstChannels; ++i)
      used_in_chan_array[i] = false;
    
    // The data retrieved by this will already be latency compensated.
    have_data = getInputData(framePos, dstChannels, nframe, used_in_chan_array, bp);
    
    // Do we want to record the incoming data?
    if(have_data && track_rec_flag && 
      (MusEGlobal::audio->isRecording() ||
       (MusEGlobal::song->record() && MusEGlobal::extSyncFlag && MusEGlobal::midiSyncContainer.isPlaying())) && 
      recFile())
    {
      if(MusEGlobal::audio->freewheel())
      {
      }
      else
      {
        for(int i = 0; i < dstChannels; ++i)
        {
          if(used_in_chan_array[i])
          {
            // Read back the latency compensated signals, using the buffers in-place.
            if(use_latency_corr)
              _latencyComp->peek(i, nframe, bp[i]);
          }
          else
          {
            // Fill unused channels with silence.
            // Channel is unused. Zero the supplied buffer.
            // REMOVE Tim. latency. Added. Maybe not required. The latency compensator already automatically clears to zero.
            AL::dsp->clear(bp[i], nframe, MusEGlobal::denormalBias);
          }
        }

        //fprintf(stderr, "WaveTrack::getData: name:%s RECORD: Putting to fifo: framePos:%d audio pos frame:%d\n",
        //        name().toLatin1().constData(),
        //        framePos, MusEGlobal::audio->pos().frame());

        // This will adjust for the latency before putting.
        putFifo(dstChannels, nframe, bp);
      }
    }

    // Advance any peeked compensator channels now.
    if(use_latency_corr)
      _latencyComp->advance(nframe);
  }

  //---------------------------------------------
  // Contributions to data from playback sources:
  //---------------------------------------------

  if(!is_playing)
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

  const bool have_pf_data = getPrefetchData(have_data, framePos, dstChannels, nframe, bp, do_overwrite);
  return have_data || have_pf_data;
}

inline bool WaveTrack::canDominateOutputLatency() const
{
  // The wave track's own wave file contributions can never dominate latency.
  return false;
}

inline bool WaveTrack::canCorrectOutputLatency() const
{
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
                  sf->setFormat(sf->format(), channels(),
                  sf->samplerate());
                  sf->openWrite();
                  }
            }
      }

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

      e.prefetchAudio(part, frames);
    }
  }
}



} // namespace MusECore
