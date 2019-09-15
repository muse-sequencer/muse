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

void AudioPrefetch::start(int priority, void *)
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
                  if(msg->_isRecTick) // Was the tick generated when audio record was on?
                  {
                        #ifdef AUDIOPREFETCH_DEBUG
                        fprintf(stderr, "AudioPrefetch::processMsg1: PREFETCH_TICK: isRecTick\n");
                        #endif
                        MusEGlobal::audio->writeTick();
                  }

                  // Indicate do not seek file before each read.
                  if(msg->_isPlayTick) // Was the tick generated when audio playback was on?
                  {
                    #ifdef AUDIOPREFETCH_DEBUG
                    fprintf(stderr, "AudioPrefetch::processMsg1: PREFETCH_TICK: isPlayTick\n");
                    #endif
                    prefetch(false);
                  }
                  
                  seekPos = ~0;     // invalidate cached last seek position
                  break;
            case PREFETCH_SEEK:
                  #ifdef AUDIOPREFETCH_DEBUG
                  fprintf(stderr, "AudioPrefetch::processMsg1 PREFETCH_SEEK msg->pos:%d\n", msg->pos);
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
      if (samplePos == seekPos && !force)
            return;

      ++seekCount;
      
      #ifdef AUDIOPREFETCH_DEBUG
      fprintf(stderr, "AudioPrefetch::msgSeek samplePos:%u force:%d seekCount:%d\n", samplePos, force, seekCount);
      #endif
      
      PrefetchMsg msg;
      msg.id  = PREFETCH_SEEK;
      msg.pos = samplePos;
      while (sendMsg1(&msg, sizeof(msg))) {
            fprintf(stderr, "AudioPrefetch::msgSeek::sleep(1)\n");
            sleep(1);
            }
      }

// REMOVE Tim. latency. Changed.
// //---------------------------------------------------------
// //   prefetch
// //---------------------------------------------------------
// 
// void AudioPrefetch::prefetch(bool doSeek)
//       {
//       unsigned lpos_frame = 0;
//       unsigned rpos_frame = 0;
//       const bool do_loops = MusEGlobal::song->loop() && !MusEGlobal::audio->bounce() && !MusEGlobal::extSyncFlag.value();
//       if(do_loops)
//       {
//         lpos_frame = MusEGlobal::song->lPos().frame();
//         rpos_frame = MusEGlobal::song->rPos().frame();
//       }
// 
//       WaveTrackList* tl = MusEGlobal::song->waves();
//       for (iWaveTrack it = tl->begin(); it != tl->end(); ++it) {
//             WaveTrack* track = *it;
//             // Save time. Don't bother if track is off. Track On/Off not designed for rapid repeated response (but mute is). (p3.3.29)
//             if(track->off())
//               continue;
// 
//             Fifo* fifo = track->prefetchFifo();
//             const int empty_count = fifo->getEmptyCount();
// 
//             // Nothing to fill?
//             if(empty_count <= 0)
//               continue;
// 
//             unsigned int write_pos = track->prefetchWritePos();
//             if (write_pos == ~0U) {
//                   fprintf(stderr, "AudioPrefetch::prefetch: invalid track write position\n");
//                   continue;
//                   }
// 
//             int ch           = track->channels();
//             float* bp[ch];
// 
//             // Fill up the empty buffers.
//             for(int i = 0; i < empty_count; ++i)
//             {
//               if(do_loops)
//               {
//                 unsigned n = rpos_frame - write_pos;
//                 if (n < MusEGlobal::segmentSize)
//                 {
//                   // adjust loop start so we get exact loop len
//                   if (n > lpos_frame)
//                         n = 0;
//                   write_pos = lpos_frame - n;
//                   // REMOVE Tim. latency. Added. Diagnostics.
//                   fprintf(stderr, "AudioPrefetch::prefetch do_loops: Calling track->setPrefetchWritePos(%u)\n", write_pos);
//                   track->setPrefetchWritePos(write_pos);
//                 }
//               }
// 
//               if (fifo->getWriteBuffer(ch, MusEGlobal::segmentSize, bp, write_pos))
//               {
//                 fprintf(stderr, "AudioPrefetch::prefetch: No write buffer!\n");
//                 break;
//               }
// 
//               // True = do overwrite.
//               track->fetchData(write_pos, MusEGlobal::segmentSize, bp, doSeek, true);
//               
//               // Only the first fetch should seek if required. Reset the flag now.
//               doSeek = false;
//               
//               write_pos += MusEGlobal::segmentSize;
//               track->setPrefetchWritePos(write_pos);
//             }
//           }
//       }

// //---------------------------------------------------------
// //   prefetch
// //---------------------------------------------------------
// 
// void AudioPrefetch::prefetch(bool doSeek)
//       {
//       unsigned lpos_frame = 0;
//       unsigned rpos_frame = 0;
//       unsigned loop_width = 0;
//       const bool do_loops = MusEGlobal::song->loop() && !MusEGlobal::audio->bounce() && !MusEGlobal::extSyncFlag.value();
//       if(do_loops)
//       {
//         const unsigned lp = MusEGlobal::song->lPos().frame();
//         const unsigned rp = MusEGlobal::song->rPos().frame();
//         loop_width = rp - lp;
//         // It is not possible to have more than one transport relocation per cycle,
//         //  and the relocation notification (sync callback) will take effect in two cycles.
//         // Therefore loops must have a minimum two cycle width.
//         if(loop_width < 2 * MusEGlobal::segmentSize)
//           loop_width = 2 * MusEGlobal::segmentSize;
// 
//         lpos_frame = lp;
//         rpos_frame = lpos_frame + loop_width;
//       }
// 
//       WaveTrackList* tl = MusEGlobal::song->waves();
//       for (iWaveTrack it = tl->begin(); it != tl->end(); ++it) {
//             WaveTrack* track = *it;
//             // Save time. Don't bother if track is off. Track On/Off not designed for rapid repeated response (but mute is). (p3.3.29)
//             if(track->off())
//               continue;
// 
//             Fifo* fifo = track->prefetchFifo();
//             const int empty_count = fifo->getEmptyCount();
// 
//             // Nothing to fill?
//             if(empty_count <= 0)
//               continue;
// 
//             unsigned int write_pos = track->prefetchWritePos();
//             if (write_pos == ~0U) {
//                   fprintf(stderr, "AudioPrefetch::prefetch: invalid track write position\n");
//                   continue;
//                   }
// 
//             unsigned int loop_count = track->loopCount();
// 
//             const unsigned write_pos_end = write_pos + MusEGlobal::segmentSize;
// 
//             int ch           = track->channels();
//             float* bp[ch];
// 
//             // Fill up the empty buffers.
//             for(int i = 0; i < empty_count; ++i)
//             {
//               // If the current write position is past the right loop point,
//               //  we have 'missed the boat' so to speak - we must not
//               //  jump back and instead let it keep moving forward
//               //  from that right loop point, especially if the user
//               //  deliberately moved the cursor past the right loop point.
//               if(do_loops && write_pos < rpos_frame && rpos_frame < write_pos_end)
//               {
//                 // Include the loop position and loop count here.
//                 if (fifo->getWriteBuffer(ch, MusEGlobal::segmentSize, bp, write_pos, lpos_frame, loop_count))
//                 {
//                   fprintf(stderr, "AudioPrefetch::prefetch: No write buffer!\n");
//                   break;
//                 }
// 
//                 //--------------------------------------------------------------------
//                 // First portion from current write position to right loop point:
//                 //--------------------------------------------------------------------
// 
//                 const unsigned first_width = rpos_frame - write_pos;
//                 // True = do overwrite.
//                 track->fetchData(write_pos, first_width, bp, doSeek, true);
// 
//                 //--------------------------------------------------------------------
//                 // Second portion from right loop point to current write position end:
// //                 // This will involve multiple loops if the loop width is less than
// //                 //  the remaining buffer width.
//                 //--------------------------------------------------------------------
// 
//                 // This will always be at least 1.
//                 const unsigned sec_width = write_pos_end - rpos_frame;
// //                 const unsigned num = sec_width / loop_width;
// //                 const unsigned rem_width = sec_width % loop_width;
// 
// //                 for(unsigned num_cnt = 0; num_cnt < num; ++num_cnt)
// //                 {
// //                   float* sec_bp[ch];
// //                   for(int i = 0; i < ch; ++i)
// //                     sec_bp[i] = bp[i] + first_width + i * loop_width;
// // 
// //                   // True = we must seek here.
// //                   // True = do overwrite.
// //                   // TODO: Economize by not re-reading the same portion:
// //                   //       'Borrow' it from an already existing copy
// //                   //        in this buffer, say the previous copy.
// //                   track->fetchData(lpos_frame, loop_width, sec_bp, true, true);
// //                 }
// // 
// //                 if(rem_width > 0)
// //                 {
// //                   float* sec_bp[ch];
// //                   for(int i = 0; i < ch; ++i)
// //                     sec_bp[i] = bp[i] + first_width + num * loop_width;
// // 
// //                   // True = we must seek here.
// //                   // True = do overwrite.
// //                   track->fetchData(lpos_frame, rem_width, sec_bp, true, true);
// //                 }
// // 
// //                 write_pos = lpos_frame + rem_width;
// //                 track->setPrefetchWritePos(write_pos);
// 
//                 if(sec_width > 0)
//                 {
//                   float* sec_bp[ch];
//                   for(int i = 0; i < ch; ++i)
//                     sec_bp[i] = bp[i] + first_width;
// 
//                   // True = we must seek here.
//                   // True = do overwrite.
//                   track->fetchData(lpos_frame, sec_width, sec_bp, true, true);
//                 }
// 
//                 write_pos = lpos_frame + sec_width;
//                 track->setPrefetchWritePos(write_pos);
//                 ++loop_count;
//                 track->setLoopCount(loop_count);
//               }
//               else
//               {
//                 if (fifo->getWriteBuffer(ch, MusEGlobal::segmentSize, bp, write_pos, write_pos, 0))
//                 {
//                   fprintf(stderr, "AudioPrefetch::prefetch: No write buffer!\n");
//                   break;
//                 }
// 
//                 // True = do overwrite.
//                 track->fetchData(write_pos, MusEGlobal::segmentSize, bp, doSeek, true);
// 
//                 write_pos += MusEGlobal::segmentSize;
//                 track->setPrefetchWritePos(write_pos);
//                 track->setLoopCount(0);
//               }
// 
//               // Only the first fetch should seek if required. Reset the flag now.
//               doSeek = false;
//             }
//           }
//       }

// //---------------------------------------------------------
// //   prefetch
// //---------------------------------------------------------
// 
// void AudioPrefetch::prefetch(bool doSeek)
// {
//   unsigned lpos_frame = 0;
//   unsigned rpos_frame = 0;
//   unsigned loop_width = 0;
//   const bool do_loops = MusEGlobal::song->loop() && !MusEGlobal::audio->bounce() && !MusEGlobal::extSyncFlag.value();
//   if(do_loops)
//   {
//     const unsigned lp = MusEGlobal::song->lPos().frame();
//     const unsigned rp = MusEGlobal::song->rPos().frame();
//     loop_width = rp - lp;
//     // It is not possible to have more than one transport relocation per cycle,
//     //  and the relocation notification (sync callback) will take effect in two cycles.
//     // Therefore loops must have a minimum two cycle width.
//     if(loop_width < 2 * MusEGlobal::segmentSize)
//       loop_width = 2 * MusEGlobal::segmentSize;
// 
//     lpos_frame = lp;
//     rpos_frame = lpos_frame + loop_width;
//   }
// 
//   WaveTrackList* tl = MusEGlobal::song->waves();
//   for(iWaveTrack it = tl->begin(); it != tl->end(); ++it)
//   {
//     WaveTrack* track = *it;
//     // Save time. Don't bother if track is off. Track On/Off not designed for rapid repeated response (but mute is). (p3.3.29)
//     if(track->off())
//       continue;
// 
//     Fifo* fifo = track->prefetchFifo();
//     const int empty_count = fifo->getEmptyCount();
// 
//     // Nothing to fill?
//     if(empty_count <= 0)
//       continue;
// 
//     unsigned int write_pos = track->prefetchWritePos();
//     if(write_pos == ~0U)
//     {
//       fprintf(stderr, "AudioPrefetch::prefetch: invalid track write position\n");
//       continue;
//     }
// 
//     unsigned int loop_count = track->loopCount();
// 
//     const int ch = track->channels();
//     float* bp[ch];
// 
//     // Fill up the empty buffers.
//     for(int i = 0; i < empty_count; ++i)
//     {
//       const unsigned n = rpos_frame - write_pos;
//       const unsigned nn = n + MusEGlobal::segmentSize;
//       if(do_loops)
//       {
//         // Remember the current write position, we may be about to change it.
//         const unsigned data_pos = write_pos;
//         
//         // This code switches between two different schemes depending on
//         //  whether the left loop marker is too close to the left edge to
//         //  provide the ideal alignment for other clients.
//         // The LAST buffer before a loop may need to be split. 
//         // One scheme puts the new transport position slightly BEFORE the
//         //  left loop marker, and the transport relocation notification
//         //  (sync callback) at the start of the LAST buffer.
//         // The other scheme puts the new transport position slightly AFTER
//         //  the left loop marker, and the relocation notification at the
//         //  start of the NEXT buffer.
//         if (n < MusEGlobal::segmentSize && n <= lpos_frame)
//           write_pos = lpos_frame - n;
//         else if (nn < MusEGlobal::segmentSize && nn > lpos_frame)
//           write_pos = lpos_frame + MusEGlobal::segmentSize - nn;
//         
//         // Include the loop position and loop count here.
//         if(fifo->getWriteBuffer(ch, MusEGlobal::segmentSize, bp, write_pos, lpos_frame, loop_count))
//         {
//           fprintf(stderr, "AudioPrefetch::prefetch (looping): No write buffer!\n");
//           break;
//         }
// 
//         // Special: Is the current write position within the last segment before a loop?
//         // Then we may need to SPLIT the buffer between portions of the remaining segment
//         //  and the new location segment.
//         if(n < MusEGlobal::segmentSize)
//         {
//           //------------------------------------------------------------------------------
//           // First portion (if required), from current write position to right loop point:
//           //------------------------------------------------------------------------------
//           if(n > 0)
//           {
//             // True = do overwrite.
//             track->fetchData(data_pos, n, bp, doSeek, true);
//           }
//           //--------------------------------------------------------------------
//           // Second portion from right loop point to current write position end:
//           //--------------------------------------------------------------------
//           const unsigned sec_width = MusEGlobal::segmentSize - n;
//           float* sec_bp[ch];
//           for(int i = 0; i < ch; ++i)
//             sec_bp[i] = bp[i] + n;
//           // True = We MUST seek to a new file position here. True = Do overwrite.
//           track->fetchData(lpos_frame, sec_width, sec_bp, true, true);
// 
//           // We have started a(nother) loop. Increment the track's loop counter.
//           ++loop_count;
//           track->setLoopCount(loop_count);
//         }
//         // Otherwise no SPLIT buffer is required. Proceed normally.
//         else
//         {
//           // True = do overwrite.
//           track->fetchData(write_pos, MusEGlobal::segmentSize, bp, doSeek, true);
//         }
// 
//         // We are done with any special SPLIT buffer. Continue on as usual,
//         //  incrementing the current write position by segment size.
//         write_pos += MusEGlobal::segmentSize;
//         track->setPrefetchWritePos(write_pos);
//       }
//       // Not looping. Proceed normally.
//       else
//       {
//         if(fifo->getWriteBuffer(ch, MusEGlobal::segmentSize, bp, write_pos, write_pos, 0))
//         {
//           fprintf(stderr, "AudioPrefetch::prefetch: No write buffer!\n");
//           break;
//         }
// 
//         // True = do overwrite.
//         track->fetchData(write_pos, MusEGlobal::segmentSize, bp, doSeek, true);
// 
//         write_pos += MusEGlobal::segmentSize;
//         track->setPrefetchWritePos(write_pos);
//         track->setLoopCount(0);
//       }
// 
//       // Only the first fetch should seek if required. Reset the flag now.
//       doSeek = false;
//     }
//   }
// }

//---------------------------------------------------------
//   prefetch
//---------------------------------------------------------

void AudioPrefetch::prefetch(bool doSeek)
{
  unsigned lpos_frame = 0;
  unsigned rpos_frame = 0;
  unsigned loop_width = 0;
  const bool do_loops = MusEGlobal::song->loop() && !MusEGlobal::audio->bounce() && !MusEGlobal::extSyncFlag.value();
  if(do_loops)
  {
    const unsigned lp = MusEGlobal::song->lPos().frame();
    const unsigned rp = MusEGlobal::song->rPos().frame();
    loop_width = rp - lp;
    // It is not possible to have more than one transport relocation per cycle,
    //  and the relocation notification (sync callback) will take effect in two cycles.
    // Therefore loops must have a minimum two cycle width.
    if(loop_width < 2 * MusEGlobal::segmentSize)
      loop_width = 2 * MusEGlobal::segmentSize;

    lpos_frame = lp;
    rpos_frame = lpos_frame + loop_width;
  }

  int empty_count;
  int ch;
  unsigned int write_pos;
  unsigned n;
  unsigned int loop_count;
  unsigned split_frame;
  unsigned split_loop_count;
  unsigned sec_width;

  WaveTrackList* tl = MusEGlobal::song->waves();
  for(iWaveTrack it = tl->begin(); it != tl->end(); ++it)
  {
    WaveTrack* track = *it;
    // Save time. Don't bother if track is off. Track On/Off not designed for rapid repeated response (but mute is). (p3.3.29)
    if(track->off())
      continue;

    Fifo* fifo = track->prefetchFifo();
    empty_count = fifo->getEmptyCount();

    // Nothing to fill?
    if(empty_count <= 0)
      continue;

    write_pos = track->prefetchWritePos();
    if(write_pos == ~0U)
    {
      fprintf(stderr, "AudioPrefetch::prefetch: invalid track write position\n");
      continue;
    }

    loop_count = track->loopCount();

    ch = track->channels();
    float* bp[ch];

    // Fill up the empty buffers.
    for(int i = 0; i < empty_count; ++i)
    {
      if(do_loops)
      {
        n = rpos_frame - write_pos;
        if(n < MusEGlobal::segmentSize)
        {
          split_frame = n;
          split_loop_count = loop_count + 1;
        }
        else
        {
          split_frame = 0;
          split_loop_count = loop_count;
        }

        // Include the loop position and loop count here.
        if(fifo->getWriteBuffer(
            ch, MusEGlobal::segmentSize, bp, write_pos,
            split_frame, lpos_frame,
            loop_count, split_loop_count))
        {
          fprintf(stderr, "AudioPrefetch::prefetch (looping): No write buffer!\n");
          break;
        }

        // Special: Is the current write position within the last segment before a loop?
        // Then we may need to SPLIT the buffer between portions of the remaining segment
        //  and the new location segment.
        if(n < MusEGlobal::segmentSize)
        {
          //------------------------------------------------------------------------------
          // First portion (if required), from current write position to right loop point:
          //------------------------------------------------------------------------------
          if(n > 0)
          {
            // True = do overwrite.
            track->fetchData(write_pos, n, bp, doSeek, true);
          }
          //--------------------------------------------------------------------
          // Second portion from right loop point to current write position end:
          //--------------------------------------------------------------------
          sec_width = MusEGlobal::segmentSize - n;
          float* sec_bp[ch];
          for(int i = 0; i < ch; ++i)
            sec_bp[i] = bp[i] + n;
          // True = We MUST seek to a new file position here. True = Do overwrite.
          track->fetchData(lpos_frame, sec_width, sec_bp, true, true);

          // We have started a(nother) loop. Increment the track's loop counter.
          ++loop_count;
          track->setLoopCount(loop_count);

          // Set a new write position.
          write_pos = lpos_frame + sec_width;
        }
        // Otherwise no SPLIT buffer is required. Proceed normally.
        else
        {
          // True = do overwrite.
          track->fetchData(write_pos, MusEGlobal::segmentSize, bp, doSeek, true);
          // Increment the write position.
          write_pos += MusEGlobal::segmentSize;
        }

        track->setPrefetchWritePos(write_pos);
      }
      // Not looping. Proceed normally.
      else
      {
        if(fifo->getWriteBuffer(ch, MusEGlobal::segmentSize, bp, write_pos, 0, write_pos, 0, 0))
        {
          fprintf(stderr, "AudioPrefetch::prefetch: No write buffer!\n");
          break;
        }

        // True = do overwrite.
        track->fetchData(write_pos, MusEGlobal::segmentSize, bp, doSeek, true);

        write_pos += MusEGlobal::segmentSize;
        track->setPrefetchWritePos(write_pos);
        track->setLoopCount(0);
      }

      // Only the first fetch should seek if required. Reset the flag now.
      doSeek = false;
    }
  }
}

//---------------------------------------------------------
//   seek
//---------------------------------------------------------

void AudioPrefetch::seek(unsigned seekTo)
      {
      #ifdef AUDIOPREFETCH_DEBUG
      fprintf(stderr, "AudioPrefetch::seek to:%u seekCount:%d\n", seekTo, seekCount);
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
      
      WaveTrackList* tl = MusEGlobal::song->waves();
      for (iWaveTrack it = tl->begin(); it != tl->end(); ++it) {
            WaveTrack* track = *it;
            track->clearPrefetchFifo();
            track->setPrefetchWritePos(seekTo);
            }

        // REMOVE Tim. latency. Added. Diagnostics.
        fprintf(stderr, "AudioPrefetch::seek: Calling prefetch(true) seekTo:%u\n", seekTo);

      // Indicate do a seek command before read (only on the first fetch).
      prefetch(true);
      
      // To help speed things up even more, check the count again. Return if more seek messages are pending. (p3.3.20)
      if(seekCount > 1)
      {
        --seekCount;
        return;
      }  
            
      seekPos  = seekTo;
      --seekCount;
      }

} // namespace MusECore

