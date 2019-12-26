//=========================================================
//  MusE
//  Linux Music Editor
//  (C) Copyright 2001 Werner Schweer (ws@seh.de)
//
//  audio_fifo.h
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

#ifndef __AUDIO_FIFO_H__
#define __AUDIO_FIFO_H__

#include "muse_atomic.h"
#include "muse_time.h"

namespace MusECore {
  
//---------------------------------------------------------
//   Fifo
//---------------------------------------------------------

struct FifoBuffer {
      float* buffer;
      MuseCount_t size;
      MuseCount_t maxSize;
      MuseCount_t pos;
      int segs;
      float latency;

      FifoBuffer() {
            buffer  = 0;
            size    = 0;
            maxSize = 0;
            pos     = 0;
            segs    = 0;
            latency = 0.0f;
            }
      };

class Fifo {
      int nbuffer;
      int ridx;               // read index; only touched by reader
      int widx;               // write index; only touched by writer
      muse_atomic_t count;    // buffer count; writer increments, reader decrements
      FifoBuffer** buffer;

   public:
      Fifo();
      ~Fifo();
      void clear();
      bool put(int segs, MuseCount_t samples, float** buffer, MuseCount_t pos, float latency);
      bool getWriteBuffer(int, MuseCount_t, float** buffer, MuseCount_t pos);
      void add();
      bool peek(int segs, MuseCount_t samples, float** buffer, MuseCount_t* pos = 0, float* latency = 0); // const;
      bool get(int segs, MuseCount_t samples, float** buffer, MuseCount_t* pos = 0, float* latency = 0);
      void remove();
      int getCount();
      int getEmptyCount();
      bool isEmpty();
      };
  
} // namespace MusECore

#endif


