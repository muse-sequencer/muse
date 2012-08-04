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

#include <poll.h>
#include <stdio.h>
#include <unistd.h>
#include <limits.h>

#include "audioprefetch.h"
#include "globals.h"
#include "track.h"
#include "song.h"
#include "audio.h"
#include "sync.h"

namespace MusEGlobal {
MusECore::AudioPrefetch* audioPrefetch;
}

namespace MusECore {

void initAudioPrefetch()  
{
  MusEGlobal::audioPrefetch = new AudioPrefetch("Prefetch");
}

//#define AUDIOPREFETCH_DEBUG

enum { PREFETCH_TICK, PREFETCH_SEEK
      };

//---------------------------------------------------------
//   PrefetchMsg
//---------------------------------------------------------

struct PrefetchMsg : public ThreadMsg {
      int pos;
      };

//---------------------------------------------------------
//   AudioPrefetch
//---------------------------------------------------------

AudioPrefetch::AudioPrefetch(const char* name)
   : Thread(name)
      {
      seekPos  = ~0;
      writePos = ~0;
      seekCount = 0;
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

void AudioPrefetch::start(int priority)
      {
      clearPollFd();
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
                  if (MusEGlobal::audio->isRecording()) 
                        MusEGlobal::audio->writeTick();

                  // Indicate do not seek file before each read.
                  prefetch(false);
                  
                  seekPos = ~0;     // invalidate cached last seek position
                  break;
            case PREFETCH_SEEK:
                  #ifdef AUDIOPREFETCH_DEBUG
                  printf("AudioPrefetch::processMsg1 PREFETCH_SEEK msg->pos:%d\n", msg->pos); 
                  #endif
                  
                  // process seek in background
                  seek(msg->pos);
                  break;
            default:
                  printf("AudioPrefetch::processMsg1: unknown message\n");
            }
      }

//---------------------------------------------------------
//   msgTick
//---------------------------------------------------------

void AudioPrefetch::msgTick()
      {
      PrefetchMsg msg;
      msg.id  = PREFETCH_TICK;
      msg.pos = 0; // seems to be unused, was uninitalized.
      while (sendMsg1(&msg, sizeof(msg))) {
            printf("AudioPrefetch::msgTick(): send failed!\n");
            }
      }

//---------------------------------------------------------
//   msgSeek
//    called from audio RT context
//---------------------------------------------------------

void AudioPrefetch::msgSeek(unsigned samplePos, bool force)
      {
      if (samplePos == seekPos && !force)
            return;

      ++seekCount;
      
      #ifdef AUDIOPREFETCH_DEBUG
      printf("AudioPrefetch::msgSeek samplePos:%u force:%d seekCount:%d\n", samplePos, force, seekCount); 
      #endif
      
      PrefetchMsg msg;
      msg.id  = PREFETCH_SEEK;
      msg.pos = samplePos;
      while (sendMsg1(&msg, sizeof(msg))) {
            printf("AudioPrefetch::msgSeek::sleep(1)\n");
            sleep(1);
            }
      }

//---------------------------------------------------------
//   prefetch
//---------------------------------------------------------

void AudioPrefetch::prefetch(bool doSeek)
      {
      if (writePos == ~0U) {
            printf("AudioPrefetch::prefetch: invalid write position\n");
            return;
            }
      if (MusEGlobal::song->loop() && !MusEGlobal::audio->bounce() && !MusEGlobal::extSyncFlag.value()) {
            const Pos& loop = MusEGlobal::song->rPos();
            unsigned n = loop.frame() - writePos;
            if (n < MusEGlobal::segmentSize) {
                  unsigned lpos = MusEGlobal::song->lPos().frame();
                  // adjust loop start so we get exact loop len
                  if (n > lpos)
                        n = 0;
                  writePos = lpos - n;
                  }
            }
      WaveTrackList* tl = MusEGlobal::song->waves();
      for (iWaveTrack it = tl->begin(); it != tl->end(); ++it) {
            WaveTrack* track = *it;
            // Save time. Don't bother if track is off. Track On/Off not designed for rapid repeated response (but mute is). (p3.3.29)
            if(track->off())
              continue;
            
            int ch           = track->channels();
            float* bp[ch];
            if (track->prefetchFifo()->getWriteBuffer(ch, MusEGlobal::segmentSize, bp, writePos))
                  continue;

            track->fetchData(writePos, MusEGlobal::segmentSize, bp, doSeek);
            
            }
      writePos += MusEGlobal::segmentSize;
      }

//---------------------------------------------------------
//   seek
//---------------------------------------------------------

void AudioPrefetch::seek(unsigned seekTo)
      {
      #ifdef AUDIOPREFETCH_DEBUG
      printf("AudioPrefetch::seek to:%u seekCount:%d\n", seekTo, seekCount); 
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
      if(seekCount > 1)
      {
        --seekCount;
        return;
      }
      
      writePos = seekTo;
      WaveTrackList* tl = MusEGlobal::song->waves();
      for (iWaveTrack it = tl->begin(); it != tl->end(); ++it) {
            WaveTrack* track = *it;
            track->clearPrefetchFifo();
            }
      
      bool isFirstPrefetch = true;
      for (unsigned int i = 0; i < (MusEGlobal::fifoLength)-1; ++i)//prevent compiler warning: comparison of signed/unsigned
      {      
            // Indicate do a seek command before read, but only on the first pass. 
            prefetch(isFirstPrefetch);
            
            isFirstPrefetch = false;
            
            // To help speed things up even more, check the count again. Return if more seek messages are pending. (p3.3.20)
            if(seekCount > 1)
            {
              --seekCount;
              return;
            }  
      }
            
      seekPos  = seekTo;
      --seekCount;
      }

} // namespace MusECore

