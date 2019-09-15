//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: node.h,v 1.8.2.2 2006/04/13 19:09:48 spamatica Exp $
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

#ifndef __AUDIONODE_H__
#define __AUDIONODE_H__

#include "muse_atomic.h"

namespace MusECore {
  
//---------------------------------------------------------
//   Fifo
//---------------------------------------------------------

struct FifoBuffer {
      float* buffer;
      int size;
      int maxSize;
      unsigned pos;
      // It is possible a loop (jump) may need to occur in the middle of a buffer.
      // This holds the new write position of that jump.
      // If it is equal to 'pos', there is NO jump.
      unsigned jump_pos;
      // If the buffer is split, this holds the size of the first section.
      // The size of the second section is 'size - split_size'.
      // The second section's data position is jump_pos.
      unsigned split_size;
      int segs;
      float latency;
      // If we are looping, this holds how many loops have occured so far.
      unsigned loop_count;
      // If the buffer is split, this holds the loop count at the second section.
      unsigned jump_loop_count;

      FifoBuffer() {
            buffer     = 0;
            size       = 0;
            maxSize    = 0;
            pos        = 0;
            jump_pos   = 0;
            split_size = 0;
            segs       = 0;
            latency    = 0.0f;
            loop_count = 0;
            jump_loop_count = 0;
            }
      };

class Fifo {
      int nbuffer;
      int ridx;               // read index; only touched by reader
      int widx;               // write index; only touched by writer
      muse_atomic_t count;         // buffer count; writer increments, reader decrements
      FifoBuffer** buffer;

   public:
      Fifo();
      ~Fifo();
      void clear();
      bool put(int segs, unsigned long samples, float** buffer, unsigned pos, float latency);
      bool getWriteBuffer(int segs, unsigned long samples, float** buf, unsigned pos,
                          unsigned split_size, unsigned jump_pos, unsigned loop_count, unsigned jump_loop_count);
      void add();
// REMOVE Tim. latency. Changed.
//       bool peek(int segs, unsigned long samples, float** dst,
//                 unsigned* pos = nullptr, unsigned* split_size = nullptr, unsigned* jump_pos = nullptr,
//                 unsigned* loop_count = nullptr, unsigned* jump_loop_count = nullptr, float* latency = nullptr); // const;
      bool peek(int segs, float** dst, unsigned* pos = nullptr, unsigned* samples = nullptr,
                unsigned* split_size = nullptr, unsigned* jump_pos = nullptr,
                unsigned* loop_count = nullptr, unsigned* jump_loop_count = nullptr, float* latency = nullptr); // const;
//       bool get(int segs, unsigned long samples, float** dst,
//                unsigned* pos = nullptr, unsigned* split_size = nullptr, unsigned* jump_pos = nullptr,
//                unsigned* loop_count = nullptr, unsigned* jump_loop_count = nullptr, float* latency = nullptr); // const;
      bool get(int segs, float** dst, unsigned* pos = nullptr, unsigned* samples = nullptr,
               unsigned* split_size = nullptr, unsigned* jump_pos = nullptr,
               unsigned* loop_count = nullptr, unsigned* jump_loop_count = nullptr, float* latency = nullptr); // const;
      void remove();
      int getCount();
      int getEmptyCount();
      bool isEmpty();
      };

} // namespace MusECore

#endif

