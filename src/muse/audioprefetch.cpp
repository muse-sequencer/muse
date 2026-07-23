//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: audioprefetch.cpp,v 1.14.2.7 2009/12/20 05:00:35 terminator356 Exp $
//
//  (C) Copyright 2001 Werner Schweer (ws@seh.de)
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

#ifndef _WIN32
#include <poll.h>
#endif
#include <stdio.h>
#include <unistd.h>
//#include <limits.h>

#include "audioprefetch.h"
#include "globals.h"
#include "track.h"
#include "song.h"
#include "audio.h"
#include "sync.h"
#include "rtlog.h"

// For debugging transport timing: Uncomment the fprintf section.
#define AUDIO_PREFETCH_DEBUG_TRANSPORT_SYNC(dev, format, args...) // fprintf(dev, format, ##args);

namespace MusEGlobal {
MusECore::AudioPrefetch* audioPrefetch = nullptr;
}

namespace MusECore {

void initAudioPrefetch()  
{
  MusEGlobal::audioPrefetch = new AudioPrefetch("Prefetch");
}

// Diagnostics.
//#define AUDIOPREFETCH_DEBUG

enum { PREFETCH_TICK, PREFETCH_SEEK
      };

//---------------------------------------------------------
//   PrefetchMsg
//---------------------------------------------------------

struct PrefetchMsg : public ThreadMsg {
      int pos;
      bool _isPlayTick;
      bool _isRecTick;
      };

//---------------------------------------------------------
//   AudioPrefetch
//---------------------------------------------------------

AudioPrefetch::AudioPrefetch(const char* name)
   : Thread(name)
      {
      seekPos  = ~0;
      seekCount.store(0);
      }

//---------------------------------------------------------
//   readMsg
//---------------------------------------------------------

static void readMsgP(void* p, void*)
      {
      AudioPrefetch* at = (AudioPrefetch*)p;
      at->readMsg1(sizeof(PrefetchMsg));
      }

//---------------------------------------------------------
//   start
//---------------------------------------------------------

void AudioPrefetch::start(int priority, void *)
      {
      clearPollFd();
      seekCount.store(0);
      addPollFd(toThreadFdr, POLLIN, MusECore::readMsgP, this, 0);
      Thread::start(priority);
      }

//---------------------------------------------------------
//   ~AudioPrefetch
//---------------------------------------------------------

AudioPrefetch::~AudioPrefetch()
      {
      }

//---------------------------------------------------------
//   processMsg
//---------------------------------------------------------

void AudioPrefetch::processMsg1(const void* m)
      {
      const PrefetchMsg* msg = (PrefetchMsg*)m;
      switch(msg->id) {
            case PREFETCH_TICK:
                  if(msg->_isRecTick) // Was the tick generated when audio record was on?
                  {
                        #ifdef AUDIOPREFETCH_DEBUG
                        fprintf(stderr, "AudioPrefetch::processMsg1: PREFETCH_TICK: isRecTick running:%d seekCount:%d\n",
                                isRunning(), seekCount.load());
                        #endif
                        MusEGlobal::audio->writeTick();
                  }

                  // Indicate do not seek file before each read.
                  if(msg->_isPlayTick) // Was the tick generated when audio playback was on?
                  {
                    #ifdef AUDIOPREFETCH_DEBUG
                    fprintf(stderr, "AudioPrefetch::processMsg1: PREFETCH_TICK: isPlayTick running:%d seekCount:%d\n",
                            isRunning(), seekCount.load());
                    #endif
                    prefetch(false);
                  }
                  
                  seekPos = ~0;     // invalidate cached last seek position
                  break;
            case PREFETCH_SEEK:
                  #ifdef AUDIOPREFETCH_DEBUG
                  fprintf(stderr, "AudioPrefetch::processMsg1 PREFETCH_SEEK msg->pos:%d running:%d seekCount:%d\n",
                          msg->pos, isRunning(), seekCount.load());
                  #endif
                  
                  // process seek in background
                  seek(msg->pos);
                  break;
            default:
                  fprintf(stderr, "AudioPrefetch::processMsg1: unknown message\n");
            }
      }

//---------------------------------------------------------
//   msgTick
//---------------------------------------------------------

void AudioPrefetch::msgTick(bool isRecTick, bool isPlayTick)
      {
      #ifdef AUDIOPREFETCH_DEBUG
      fprintf(stderr, "AudioPrefetch::msgTick: isRecTick:%d isPlayTick:%d running:%d seekCount:%d\n",
              isRecTick, isPlayTick, isRunning(), seekCount.load());
      #endif

      // Zero-init: PrefetchMsg has tail padding after the two bools that
      //  member-by-member assignment never touches. sendMsg1() writes
      //  sizeof(msg) raw bytes through the pipe, so leftover stack garbage
      //  in that padding was going out uninitialised (valgrind: Thread::
      //  sendMsg1 write(buf) uninitialised, traced back to this stack alloc).
      PrefetchMsg msg{};
      msg.id  = PREFETCH_TICK;
      msg.pos = 0; // seems to be unused, was uninitialized.
      msg._isRecTick = isRecTick;
      msg._isPlayTick = isPlayTick;
      // Bounded like msgSeek() above: don't spam fprintf on every failed
      // retry if the pipe stays full for a while.
      const int max_tick_retries = 64;
      int tries = 0;
      while (sendMsg1(&msg, sizeof(msg))) {
            if(++tries >= max_tick_retries)
            {
              MusECore::rtLog("AudioPrefetch::msgTick(): send failed after %d tries, dropping tick", tries);
              return;
            }
            }
      }

//---------------------------------------------------------
//   msgSeek
//    called from audio RT context
//---------------------------------------------------------

void AudioPrefetch::msgSeek(unsigned samplePos, bool force)
      {
      #ifdef AUDIOPREFETCH_DEBUG
      fprintf(stderr, "AudioPrefetch::msgSeek: samplePos:%d force:%d seekPos:%d running:%d seekCount:%d\n",
              samplePos, force, seekPos, isRunning(), seekCount.load());
      #endif

      if (samplePos == seekPos && !force)
            return;

      ++seekCount;
      
      #ifdef AUDIOPREFETCH_DEBUG
      fprintf(stderr, " ... seekCount incremented:%d\n", seekCount.load());
      #endif
      
      // Zero-init: this message type doesn't use _isPlayTick/_isRecTick at
      //  all, so without {} those two bools (plus tail padding) went out
      //  over sendMsg1() completely uninitialised on every seek message.
      PrefetchMsg msg{};
      msg.id  = PREFETCH_SEEK;
      msg.pos = samplePos;
      // This function runs on the RT audio thread (see comment above), so it
      // must never block: no sleep(), and no unbounded fprintf spam if the
      // pipe stays full. Retry a bounded number of times without sleeping;
      // if it still hasn't gone through, drop this seek and report once -
      // a later seek (this is superseded-seek-collapsing territory, see
      // seek()'s seekCount handling) or the next PREFETCH_TICK will recover.
      const int max_rt_retries = 64;
      int tries = 0;
      while (sendMsg1(&msg, sizeof(msg))) {
            if(++tries >= max_rt_retries)
            {
              MusECore::rtLog("AudioPrefetch::msgSeek: send failed after %d tries, dropping seek (samplePos:%u)",
                      tries, samplePos);
              --seekCount; // Undo: this message was never actually sent.
              return;
            }
            }
      }

//---------------------------------------------------------
//   prefetch
//---------------------------------------------------------

void AudioPrefetch::prefetch(bool doSeek)
      {
      unsigned lpos_frame = 0;
      unsigned rpos_frame = 0;
      const bool do_loops = MusEGlobal::song->loop() && !MusEGlobal::audio->bounce() && !MusEGlobal::extSyncFlag;
      if(do_loops)
      {
        lpos_frame = MusEGlobal::song->lPos().frame();
        rpos_frame = MusEGlobal::song->rPos().frame();
      }

      WaveTrackList* tl = MusEGlobal::song->waves();
      for (iWaveTrack it = tl->begin(); it != tl->end(); ++it) {
            WaveTrack* track = *it;
            // Save time. Don't bother if track is off. Track On/Off not designed for rapid repeated response (but mute is). (p3.3.29)
            if(track->off())
              continue;

            Fifo* fifo = track->prefetchFifo();
            const int empty_count = fifo->getEmptyCount();

            // Diagnostics: this runs on the prefetch thread (not the RT audio
            // thread), so an fprintf here is safe and won't itself provoke a
            // dropout. Unlike the downstream "fifo underrun" message in
            // WaveTrack::getPrefetchData() - which just reports the FIFO was
            // empty when the audio thread asked - this tells us WHY: the
            // producer is chronically behind and hasn't caught up over
            // multiple prefetch ticks.
            if(empty_count >= 256)
              fprintf(stderr, "WARNING: AudioPrefetch::prefetch: track:%s falling behind, empty_count:%d\n", track->name().toUtf8().constData(), empty_count);

            // Nothing to fill?
            if(empty_count <= 0)
            {
              AUDIO_PREFETCH_DEBUG_TRANSPORT_SYNC(stderr, "AudioPrefetch::prefetch: empty_count <= 0!\n");
              continue;
            }

            unsigned int write_pos = track->prefetchWritePos();
            if (write_pos == ~0U) {
                  fprintf(stderr, "AudioPrefetch::prefetch: invalid track write position\n");
                  continue;
                  }

            int ch           = track->channels();
            float* bp[ch];

            AUDIO_PREFETCH_DEBUG_TRANSPORT_SYNC(stderr, "AudioPrefetch::prefetch: Filling empty_count:%d do_loops:%d lpos_frame:%d rpos_frame:%d\n",
                    empty_count, do_loops, lpos_frame, rpos_frame);

            // Fill up the empty buffers.
            for(int i = 0; i < empty_count; ++i)
            {
              if(do_loops)
              {
                // Signed arithmetic: write_pos can legitimately be >= rpos_frame
                // right after a seek/scrub to or past the loop-out marker
                // (prefetchWritePos() is set directly from the seek target,
                // with no clamping to the loop range). With unsigned n, that
                // case underflowed to a huge value, which silently skipped
                // the wrap-around below for the rest of the session for this
                // track - the prefetch position then permanently diverged
                // from what the looping consumer expects, causing chronic
                // FIFO underrun. Fixes CRASH_10.md report.
                const int64_t n_signed = (int64_t)rpos_frame - (int64_t)write_pos;

                AUDIO_PREFETCH_DEBUG_TRANSPORT_SYNC(stderr, "  do loops: write_pos:%d n:%ld segmentSize:%d\n",
                        write_pos, (long)n_signed, MusEGlobal::segmentSize);

                if (n_signed < (int64_t)MusEGlobal::segmentSize)
                {
                  // adjust loop start so we get exact loop len
                  // (n_signed <= 0 here means we're already at/past rpos_frame -
                  //  treat that the same as "no remainder", i.e. wrap now.)
                  unsigned n = (n_signed > 0) ? (unsigned)n_signed : 0;
                  if (n > lpos_frame)
                        n = 0;
                  write_pos = lpos_frame - n;
                  AUDIO_PREFETCH_DEBUG_TRANSPORT_SYNC(stderr, "  looping: new write_pos:%d\n", write_pos);

                  track->setPrefetchWritePos(write_pos);
                  track->seekData(write_pos);
                }
              }

              if (fifo->getWriteBuffer(ch, MusEGlobal::segmentSize, bp, write_pos))
              {
                fprintf(stderr, "AudioPrefetch::prefetch: No write buffer!\n");
                break;
              }

              // True = do overwrite.
              track->fetchData(write_pos, MusEGlobal::segmentSize, bp, doSeek, true);
              
              // Only the first fetch should seek if required. Reset the flag now.
              doSeek = false;
              
              write_pos += MusEGlobal::segmentSize;
              track->setPrefetchWritePos(write_pos);
            }
          }
      }

//---------------------------------------------------------
//   seek
//---------------------------------------------------------

void AudioPrefetch::seek(unsigned seekTo)
      {
      #ifdef AUDIOPREFETCH_DEBUG
      fprintf(stderr, "AudioPrefetch::seek to:%u running:%d seekCount:%d\n", seekTo, isRunning(), seekCount.load());
      #endif

      // Speedup: More than one seek message pending?
      // Eat up seek messages until we get to the very LATEST one,
      //  because all the rest which came before it are irrelevant now,
      //  and processing them all was taking extreme time, especially with
      //  resampling enabled.
      // In particular, when the user 'slides' the play cursor back and forth
      //  there are MANY seek messages in the pipe, and with resampling enabled
      //  it was taking minutes to finish seeking. If the user hit play during that time,
      //  things were messed up (FIFO underruns, choppy intermittent sound etc).
      // Added by Tim. p3.3.20
      if(seekCount.load() > 1)
      {
        --seekCount;
        return;
      }

      WaveTrackList* tl = MusEGlobal::song->waves();
      for (iWaveTrack it = tl->begin(); it != tl->end(); ++it) {
            WaveTrack* track = *it;
            track->clearPrefetchFifo();
            track->setPrefetchWritePos(seekTo);
            track->seekData(seekTo);
            }

      // Indicate do a seek command before read (only on the first fetch).
      prefetch(true);

      // To help speed things up even more, check the count again. Return if more seek messages are pending. (p3.3.20)
      if(seekCount.load() > 1)
      {
        --seekCount;
        return;
      }

      seekPos  = seekTo;
      --seekCount;

      #ifdef AUDIOPREFETCH_DEBUG
      fprintf(stderr, " ... final seekCount:%d\n", seekCount.load());
      #endif
      }

bool AudioPrefetch::seekDone() const { return seekCount.load() == 0; }

} // namespace MusECore
