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

//#define WAVETRACK_DEBUG
// Turn on some cool terminal 'peak' meters for debugging
//  presence of actual audio at various places
// #define NODE_DEBUG_TERMINAL_PEAK_METERS

namespace MusECore {

//---------------------------------------------------------
//   WaveTrack
//---------------------------------------------------------

// Default 1 channel for wave tracks.
WaveTrack::WaveTrack() : AudioTrack(Track::WAVE, 1)
{
  _prefetchWritePos = ~0;
  _loop_count = 0;
}

WaveTrack::WaveTrack(const WaveTrack& wt, int flags) : AudioTrack(wt, flags)
{
  _prefetchWritePos = ~0;
  _loop_count = 0;

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
//   fetchData
//    called from prefetch thread
//---------------------------------------------------------

void WaveTrack::fetchData(unsigned pos, unsigned samples, float** bp, bool doSeek, bool overwrite, int latency_correction)
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

              for (iEvent ie = part->nonconst_events().begin(); ie != part->nonconst_events().end(); ++ie) {
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

                    event.readAudio(part, srcOffset, bpp, channels(), nn, doSeek, do_overwrite);
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

  float* pf_buf[dstChannels];

  int i_correction = 0;
  if(use_latency_corr)
  {
    const TrackLatencyInfo& li = getLatencyInfo(false);
    const float correction = li._sourceCorrectionValue;
    i_correction = correction;
  }

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
    unsigned buf_pos;
    unsigned buf_size;
    unsigned buf_split_size;
    unsigned buf_loop_pos;
    unsigned buf_loop_count;
    unsigned buf_jump_loop_count;
//     if(_prefetchFifo.peek(dstChannels, nframe, pf_buf, &buf_pos, &buf_split_size, &buf_loop_pos, &buf_loop_count, &buf_jump_loop_count))
    if(_prefetchFifo.peek(dstChannels, pf_buf, &buf_pos, &buf_size, &buf_split_size, &buf_loop_pos, &buf_loop_count, &buf_jump_loop_count))
    {
      fprintf(stderr, "WaveTrack::getData(%s) framePos:%u (prefetch peek A) fifo underrun\n", name().toLocal8Bit().constData(), framePos);
      return have_data;
    }

    //fprintf(stderr, "WaveTrack::getData(%s) (prefetch peek A) pos:%d\n", name().toLocal8Bit().constData(), pos);

    const bool do_loops = MusEGlobal::song->loop() && !MusEGlobal::audio->bounce() && !MusEGlobal::extSyncFlag.value();
    const unsigned loop_count = MusEGlobal::audio->loopCount();
    // Grab the real data frame and loop count.
    const unsigned data_frame = MusEGlobal::audio->dataFrame();
    const unsigned data_loop_count = MusEGlobal::audio->dataLoopCount();

    unsigned lin_data_frame = data_frame;
    unsigned corr_data_frame = data_frame;
    int64_t i_corr_data_frame = data_frame;
    unsigned corr_data_loop_count = data_loop_count;
    unsigned lpos_frame = 0;
    unsigned rpos_frame = 0;
    unsigned loop_width = 0;
    if(do_loops)
    {
      // Rpos is always greater than or equal to Lpos.
      const unsigned lp = MusEGlobal::song->lPos().frame();
      const unsigned rp = MusEGlobal::song->rPos().frame();
      loop_width = rp - lp;
      // It is not possible to have more than one transport relocation per cycle,
      //  and the relocation notification (sync callback) will take effect in two cycles.
      // Therefore loops must have a minimum two cycle width.
      // Limit the loop width. This is good, it relieves us from needing multiple jump points in a buffer.
      if(loop_width < 2 * MusEGlobal::segmentSize)
        loop_width = 2 * MusEGlobal::segmentSize;

      lpos_frame = lp;
      rpos_frame = lpos_frame + loop_width;
      
      // 'Unroll' the loop and frame into a linear frame.
      //MusEGlobal::audio->loop2LinearFrame(data_loop_count, data_frame, lpos_frame, rpos_frame, &corr_data_frame);
      MusEGlobal::audio->loop2LinearFrame(data_loop_count, data_frame, lpos_frame, rpos_frame, &lin_data_frame);
      i_corr_data_frame = lin_data_frame;
    }

//     // Apply latency correction.
//     if(i_correction > 0 && lin_data_frame > (unsigned)i_correction)
//       lin_data_frame -= i_correction;
      
    i_corr_data_frame -= i_correction;

    // Apply latency correction. If looping, re-roll the new linear frame into a loop count and frame.
    if(i_corr_data_frame < 0)
    {
      corr_data_loop_count = 0;
    }
    else if(do_loops)
    {
      MusEGlobal::audio->linear2LoopFrame(i_corr_data_frame, lpos_frame, rpos_frame, &corr_data_loop_count, &lin_data_frame);
      i_corr_data_frame = lin_data_frame;
    }
    
    
//     // Apply latency correction. If looping, re-roll the new linear frame into a loop count and frame.
//     if(do_loops && i_correction > 0 && corr_data_frame > (unsigned)i_correction)
//       MusEGlobal::audio->linear2LoopFrame(corr_data_frame - i_correction, lpos_frame, rpos_frame, &corr_data_loop_count, &corr_data_frame);
//     else
//       corr_data_frame = corr_data_frame - i_correction;
    
    //const unsigned corr_data_frame_end = corr_data_frame + nframe;
    const int64_t i_corr_data_frame_end = i_corr_data_frame + nframe;
    
    
    
//     // Apply latency correction. Easy way out: Use an integer.
//     int64_t i_corr_data_frame = lin_data_frame - i_correction;
// 
//     // If looping, re-roll the new linear frame into a loop count and frame.
//     if(do_loops && i_corr_data_frame >= 0)
//       MusEGlobal::audio->linear2LoopFrame(i_corr_data_frame, lpos_frame, rpos_frame, &corr_data_loop_count, &corr_data_frame);


    
//     // Would the result of applying latency correction be less than zero?
//     if(i_correction >= 0 && (unsigned)i_correction > corr_data_frame)
//     {
//       // Allow the stream to RETARD. (That is, let our requested frame catch up to the stream.)
//       return have_data;
//     }
// 
//     Apply latency correction.
//     corr_data_frame -= i_correction;

//     // If looping, re-roll the new linear frame into a loop count and frame.
//     if(do_loops)
//       MusEGlobal::audio->linear2LoopFrame(corr_data_frame, lpos_frame, rpos_frame, &corr_data_loop_count, &corr_data_frame);


    unsigned buf_pos_end = buf_pos + buf_split_size;
//     unsigned buf_loop_pos_end = buf_loop_pos + MusEGlobal::segmentSize - buf_split_size;
    unsigned buf_loop_pos_end = buf_loop_pos + buf_size - buf_split_size;

//     if((buf_split_size != 0 && (loop_count < buf_loop_count || corr_data_frame < buf_pos)) ||
//        (loop_count < buf_jump_loop_count || corr_data_frame < buf_loop_pos))
//     if((buf_split_size != 0 && (corr_data_loop_count < buf_loop_count || corr_data_frame_end < buf_pos)) ||
//        (corr_data_loop_count < buf_jump_loop_count || corr_data_frame_end < buf_loop_pos))
    if((buf_split_size != 0
         && (corr_data_loop_count < buf_loop_count
             || (corr_data_loop_count == buf_loop_count && i_corr_data_frame_end <= buf_pos)))
       && (corr_data_loop_count < buf_jump_loop_count
           || (corr_data_loop_count == buf_jump_loop_count && i_corr_data_frame_end <= buf_loop_pos)))
    {
      // Allow the stream to RETARD. (That is, let our requested frame catch up to the stream.)
      return have_data;
    }
    else
    {
      // Allow the stream to ADVANCE if necessary. (That is, let the stream catch up to our requested frame.)
//       while((buf_split_size != 0 && (loop_count > buf_loop_count || corr_data_frame >= buf_pos_end)) ||
//             (loop_count > buf_jump_loop_count || corr_data_frame >= buf_loop_pos_end))
      while((buf_split_size != 0 
              && (corr_data_loop_count > buf_loop_count
                  || (corr_data_loop_count > buf_loop_count && i_corr_data_frame >= buf_pos_end)))
            && (corr_data_loop_count > buf_jump_loop_count
                || (corr_data_loop_count > buf_jump_loop_count && i_corr_data_frame >= buf_loop_pos_end)))
      {
        // Done with buffer, remove it.
        _prefetchFifo.remove();

//         if(_prefetchFifo.peek(dstChannels, nframe, pf_buf, &buf_pos, &buf_split_size, &buf_loop_pos, &buf_loop_count, &buf_jump_loop_count))
        if(_prefetchFifo.peek(dstChannels, pf_buf, &buf_pos, &buf_size, &buf_split_size, &buf_loop_pos, &buf_loop_count, &buf_jump_loop_count))
        {
          fprintf(stderr, "WaveTrack::getData(%s) framePos:%d (prefetch peek B) fifo underrun\n", name().toLocal8Bit().constData(), framePos);
          return have_data;
        }

        buf_pos_end = buf_pos + buf_split_size;
        //buf_loop_pos_end = buf_loop_pos + MusEGlobal::segmentSize - buf_split_size;
        buf_loop_pos_end = buf_loop_pos + buf_size - buf_split_size;

        // Check again whether to RETARD (which is an error - we were expecting a forward sequence).
        if((buf_split_size != 0
             && (corr_data_loop_count < buf_loop_count
                 || (corr_data_loop_count == buf_loop_count && i_corr_data_frame_end <= buf_pos)))
           && (corr_data_loop_count < buf_jump_loop_count
               || (corr_data_loop_count == buf_jump_loop_count && i_corr_data_frame_end <= buf_loop_pos)))
        {
          if(MusEGlobal::debugMsg)
            fprintf(stderr, "fifo get(%s) (A) error expected %d, got %u\n", name().toLocal8Bit().constData(), framePos, buf_pos);
          return have_data;
        }
      }
    }
    
    unsigned buf1_pos;
    //if(buf_split_size != 0 && loop_count == buf_loop_count && corr_data_frame >= buf_pos && corr_data_frame < buf_pos_end)
    if(buf_split_size != 0 && corr_data_loop_count == buf_loop_count
      && i_corr_data_frame_end > buf_pos && i_corr_data_frame_end <= buf_pos_end)
    {
      //buf1_pos = corr_data_frame - buf_pos;
      buf1_pos = i_corr_data_frame - buf_pos;
    }
    else //if(loop_count == buf_jump_loop_count && corr_data_frame >= buf_loop_pos && corr_data_frame < buf_loop_pos_end)
    {
      //buf1_pos = corr_data_frame - buf_loop_pos + buf_split_size;
      buf1_pos = i_corr_data_frame - buf_loop_pos + buf_split_size;
    }

    
    // This will always be at least 1, ie. corr_frame_pos > pos.
    //const unsigned buf1_pos = corr_frame_pos - buf_pos;
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
//       const unsigned expect_nextpos = pos + nframe;
    unsigned expect_nextpos = buf_pos + nframe;

    // Check if at or beyond loop end.
    if(do_loops && expect_nextpos >= rpos_frame)
    {
      // Rpos is always greater than or equal to Lpos.
      const unsigned loop_width = rpos_frame - lpos_frame;
      const unsigned cf_width = expect_nextpos - lpos_frame;
      // Wrap around using modulo.
      const unsigned fin_width = cf_width % loop_width;
      // Adjust requested frame to left position plus remainder.
      expect_nextpos = lpos_frame + fin_width;
    }
  
    // Peek the next buffer but do not remove it,
    //  since the rest of it will be required next cycle.
    if(_prefetchFifo.peek(dstChannels, nframe, pf_buf, &buf_pos))
    {
      fprintf(stderr, "WaveTrack::getData(%s) frame_pos:%ld (prefetch peek C) fifo underrun\n",
              name().toLocal8Bit().constData(), frame_pos);
      return have_data;
    }
    
    if(buf_pos != expect_nextpos)
    {
      if(MusEGlobal::debugMsg)
        fprintf(stderr, "fifo get(%s) frame_pos:%ld (B) error expected %u, got %u\n",
                name().toLocal8Bit().constData(), frame_pos, expect_nextpos, buf_pos);
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
    
    
    
    
    
    
    
    
    
    
    
    //const unsigned n = rpos_frame - corr_data_frame;

//     const int64_t frame_pos = framePos;
//     // REMOVE Tim. latency. Changed.
//     // const int64_t corr_frame_pos = framePos - i_correction;
//     // const int64_t corr_frame_end_pos = framePos - i_correction + nframe;
//     int64_t corr_frame_pos = framePos - i_correction;

    
    
    
    
//     // Check if at or beyond loop end.
//     if(do_loops && corr_frame_pos >= rpos_frame)
//     {
// //       // Rpos is always greater than or equal to Lpos.
// //       const unsigned loop_width = rpos_frame - lpos_frame;
//       const unsigned cf_width = corr_frame_pos - lpos_frame;
//       // Wrap around using modulo.
//       const unsigned fin_width = cf_width % loop_width;
//       // Adjust requested frame to left position plus remainder.
//       corr_frame_pos = lpos_frame + fin_width;
//     }

//     const int64_t corr_frame_end_pos = corr_frame_pos + nframe;

//     //----------------------------------------------
//     // Do we need to RETARD, or ADVANCE, the stream?
//     //----------------------------------------------
// 
//     if(corr_frame_end_pos <= buf_pos)
//     {
//       // Allow the stream to RETARD. (That is, let our requested frame catch up to the stream.)
//       return have_data;
//     }
//     else
//     {
//       // Allow the stream to ADVANCE if necessary. (That is, let the stream catch up to our requested frame.)
//       while(corr_frame_pos >= buf_pos + nframe)
//       {
//         // Done with buffer, remove it.
//         _prefetchFifo.remove();
// 
//         if(_prefetchFifo.peek(dstChannels, nframe, pf_buf, &buf_pos))
//         {
//           fprintf(stderr, "WaveTrack::getData(%s) frame_pos:%ld (prefetch peek B) fifo underrun\n", name().toLocal8Bit().constData(), frame_pos);
//           return have_data;
//         }
// 
//         if(corr_frame_end_pos <= buf_pos)
//         {
//           if(MusEGlobal::debugMsg)
//             fprintf(stderr, "fifo get(%s) (A) error expected %ld, got %u\n", name().toLocal8Bit().constData(), frame_pos, buf_pos);
//           return have_data;
//         }
//       }
//     }

    //----------------------------------------------
    // Mix the signals
    //----------------------------------------------

    if(corr_frame_pos <= buf_pos)
    {
      if(!isMute())
      {
        const unsigned blanks = buf_pos - corr_frame_pos;
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
      if(corr_frame_pos == buf_pos)
      {
        // Done with buffer, remove it.
        _prefetchFifo.remove();
      }
    }
    else
    {
      // This will always be at least 1, ie. corr_frame_pos > pos.
      const unsigned buf1_pos = corr_frame_pos - buf_pos;
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
//       const unsigned expect_nextpos = pos + nframe;
      unsigned expect_nextpos = buf_pos + nframe;

      // Check if at or beyond loop end.
      if(do_loops && expect_nextpos >= rpos_frame)
      {
        // Rpos is always greater than or equal to Lpos.
        const unsigned loop_width = rpos_frame - lpos_frame;
        const unsigned cf_width = expect_nextpos - lpos_frame;
        // Wrap around using modulo.
        const unsigned fin_width = cf_width % loop_width;
        // Adjust requested frame to left position plus remainder.
        expect_nextpos = lpos_frame + fin_width;
      }
    
      // Peek the next buffer but do not remove it,
      //  since the rest of it will be required next cycle.
      if(_prefetchFifo.peek(dstChannels, nframe, pf_buf, &buf_pos))
      {
        fprintf(stderr, "WaveTrack::getData(%s) frame_pos:%ld (prefetch peek C) fifo underrun\n",
                name().toLocal8Bit().constData(), frame_pos);
        return have_data;
      }
      
      if(buf_pos != expect_nextpos)
      {
        if(MusEGlobal::debugMsg)
          fprintf(stderr, "fifo get(%s) frame_pos:%ld (B) error expected %u, got %u\n",
                  name().toLocal8Bit().constData(), frame_pos, expect_nextpos, buf_pos);
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

  return have_data;
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

} // namespace MusECore
