//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: audioprefetch.cpp,v 1.14.2.7 2009/12/20 05:00:35 terminator356 Exp $
//
//  (C) Copyright 2001 Werner Schweer (ws@seh.de)
//=========================================================

#include <poll.h>
#include <stdio.h>
#include <unistd.h>
#include <values.h>

#include "audioprefetch.h"
#include "globals.h"
#include "track.h"
#include "song.h"
#include "audio.h"
#include "sync.h"

enum { PREFETCH_TICK, PREFETCH_SEEK
      };

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

//AudioPrefetch::AudioPrefetch(int prio, const char* name)
//   : Thread(prio,name)
AudioPrefetch::AudioPrefetch(const char* name)
   : Thread(name)
      {
      seekPos  = ~0;
      writePos = ~0;
      seekDone = true;
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

//void AudioPrefetch::start()
void AudioPrefetch::start(int priority)
      {
      clearPollFd();
      addPollFd(toThreadFdr, POLLIN, ::readMsgP, this, 0);
      //Thread::start();
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
                  if (audio->isRecording()) {
                  //puts("writeTick");
                        audio->writeTick();
                        }
                  // Indicate do not seek file before each read.
                  // Changed by Tim. p3.3.17 
                  //prefetch();
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
//---------------------------------------------------------

void AudioPrefetch::msgTick()
      {
      PrefetchMsg msg;
      msg.id  = PREFETCH_TICK;
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
      if (samplePos == seekPos && !force) {
            seekDone = true;
            return;
            }
      seekDone = false;
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

//void AudioPrefetch::prefetch()
void AudioPrefetch::prefetch(bool doSeek)
      {
      if (writePos == ~0U) {
            printf("prefetch(): invalid write position\n");
            return;
            }
      if (song->loop() && !audio->bounce() && !extSyncFlag.value()) {
            const Pos& loop = song->rPos();
            unsigned n = loop.frame() - writePos;
            if (n < segmentSize) {
                  unsigned lpos = song->lPos().frame();
                  // adjust loop start so we get exact loop len
                  if (n > lpos)
                        n = 0;
// printf("prefetch seek %d\n", writePos);
                  writePos = lpos - n;
                  }
            }
      WaveTrackList* tl = song->waves();
      for (iWaveTrack it = tl->begin(); it != tl->end(); ++it) {
            WaveTrack* track = *it;
            int ch           = track->channels();
            float* bp[ch];
// printf("prefetch %d\n", writePos);
            if (track->prefetchFifo()->getWriteBuffer(ch, segmentSize, bp, writePos)) {
                  // Too many of these. Chokes muse. Turn on later. (muse works OK anyway).
                  //printf("Prefetch: NO BUFFER\n");
                  continue;
                  }
            //track->fetchData(writePos, segmentSize, bp);
            track->fetchData(writePos, segmentSize, bp, doSeek);
            }
      writePos += segmentSize;
      }

//---------------------------------------------------------
//   seek
//---------------------------------------------------------

void AudioPrefetch::seek(unsigned seekTo)
      {
// printf("seek %d\n", seekTo);
      writePos = seekTo;
      WaveTrackList* tl = song->waves();
      for (iWaveTrack it = tl->begin(); it != tl->end(); ++it) {
            WaveTrack* track = *it;
            track->clearPrefetchFifo();
            }
      
      bool isFirstPrefetch = true;
      for (unsigned int i = 0; i < (fifoLength)-1; ++i)//prevent compiler warning: comparison of signed/unsigned
      {      
            // Indicate do a seek command before read, but only on the first pass. 
            // Changed by Tim. p3.3.17 
            //prefetch();
            prefetch(isFirstPrefetch);
            
            isFirstPrefetch = false;
      }
            
      seekPos  = seekTo;
      seekDone = true;
      }

