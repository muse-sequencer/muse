//=============================================================================
//  MusE
//  Linux Music Editor
//  $Id:$
//
//  Copyright (C) 2002-2006 by Werner Schweer and others
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License version 2.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//=============================================================================

#include <poll.h>

#include "audioprefetch.h"
#include "globals.h"
#include "track.h"
#include "song.h"
#include "audio.h"
#include "sync.h"
#include "fifo.h"

enum { PREFETCH_TICK, PREFETCH_SEEK };

//---------------------------------------------------------
//   PrefetchMsg
//---------------------------------------------------------

struct PrefetchMsg : public ThreadMsg {
      int pos;
      };

AudioPrefetch* audioPrefetch;

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
      addPollFd(toThreadFdr, POLLIN, ::readMsgP, this, 0);
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
                  prefetch(false);
                  seekPos = ~0;     // invalidate cached last seek position
                  break;

            case PREFETCH_SEEK:
                  // process seek in background
                  seek(msg->pos);
                  break;

            default:
                  printf("AudioPrefetch::processMsg1: unknown message\n");
            }
      }

//---------------------------------------------------------
//   msgTick
//    trigger audio prefetch with next buffer position
//---------------------------------------------------------

void AudioPrefetch::msgTick()
      {
      if (fifo.count() < FIFO_BUFFER/3) {
            PrefetchMsg msg;
            msg.id  = PREFETCH_TICK;
            while (sendMsg1(&msg, sizeof(msg))) {
                  printf("AudioPrefetch::msgTick(): send failed!\n");
                  }
            }
      }

//---------------------------------------------------------
//   msgSeek
//    called from audio RT context
//---------------------------------------------------------

void AudioPrefetch::msgSeek(unsigned samplePos)
      {
      //
      // optimize unecessary seeks; seekPos is invalidated on
      // first prefetch tick
      //
      if (samplePos == seekPos)
            return;

      //q_atomic_increment(&seekCount);
      ++seekCount;

      PrefetchMsg msg;
      msg.id  = PREFETCH_SEEK;
      msg.pos = samplePos;
      msg.serialNo = 0;
      while (sendMsg1(&msg, sizeof(msg))) {
            printf("AudioPrefetch::msgSeek::sleep(1)\n");
            sleep(1);
            }
      }

//---------------------------------------------------------
//   prefetch
//---------------------------------------------------------

void AudioPrefetch::prefetch(bool seekFlag)
      {
      WaveTrackList* tl = song->waves();
      if (tl->empty())
            return;
      while (fifo.count() < FIFO_BUFFER) {
            if (song->loop() && !audio->bounce() && !extSyncFlag) {
                  unsigned rpos = song->rPos().frame();
                  unsigned n    = rpos - writePos;
                  //
                  // dont loop if we are beyond right marker
                  //
                  if (writePos <= rpos && (n < segmentSize)) {
                        unsigned lpos = song->lPos().frame();
                        // adjust loop start so we get exact loop len
                        if (n > lpos)
                              n = 0;
                        writePos = lpos - n;
                        }
                  }
            int widx = fifo.setWritePos(writePos);
            for (iWaveTrack it = tl->begin(); it != tl->end(); ++it) {
                  WaveTrack* track = *it;
                  if (!seekFlag && ((audio->isRecording() && track->recordFlag()) || !audio->isPlaying()))
                      continue;
                  track->fetchData(writePos, segmentSize, widx);
                  }
            writePos += segmentSize;
            fifo.push();
            }
      }

//---------------------------------------------------------
//   seek
//---------------------------------------------------------

void AudioPrefetch::seek(unsigned seekTo)
      {
      fifo.clear();
      writePos = seekTo;
	prefetch(true);
      seekPos = seekTo;
      // q_atomic_decrement(&seekCount);
      --seekCount;
      }

