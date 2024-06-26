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

      PrefetchMsg msg;
      msg.id  = PREFETCH_TICK;
      msg.pos = 0; // seems to be unused, was uninitialized.
      msg._isRecTick = isRecTick;
      msg._isPlayTick = isPlayTick;
      while (sendMsg1(&msg, sizeof(msg))) {
            fprintf(stderr, "AudioPrefetch::msgTick(): send failed!\n");
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
      
      PrefetchMsg msg;
      msg.id  = PREFETCH_SEEK;
      msg.pos = samplePos;
      while (sendMsg1(&msg, sizeof(msg))) {
            fprintf(stderr, "AudioPrefetch::msgSeek::sleep(1)\n");
            sleep(1);
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

            // Diagnostics.
            //if(empty_count >= 256)
            //  fprintf(stderr, "WARNING: AudioPrefetch::prefetch: track:%s empty_count:%d >= 512\n", track->name().toUtf8().constData(), empty_count);

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
                unsigned n = rpos_frame - write_pos;

                AUDIO_PREFETCH_DEBUG_TRANSPORT_SYNC(stderr, "  do loops: write_pos:%d n:%d segmentSize:%d\n",
                        write_pos, n, MusEGlobal::segmentSize);

                if (n < MusEGlobal::segmentSize)
                {
                  // adjust loop start so we get exact loop len
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

