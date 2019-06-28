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
      muse_atomic_t count;         // buffer count; writer increments, reader decrements
      FifoBuffer** buffer;

   public:
      Fifo();
      ~Fifo();
      void clear();
      bool put(int segs, unsigned long samples, float** buffer, unsigned pos, float latency);
      bool getWriteBuffer(int, unsigned long, float** buffer, unsigned pos);
      void add();
      bool peek(int segs, unsigned long samples, float** buffer, unsigned* pos = 0, float* latency = 0); // const;
      bool get(int segs, unsigned long samples, float** buffer, unsigned* pos = 0, float* latency = 0);
      void remove();
      int getCount();
      int getEmptyCount();
      bool isEmpty();
      };

} // namespace MusECore

#endif

