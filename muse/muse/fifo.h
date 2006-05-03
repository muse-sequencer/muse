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

#ifndef __AUDIONODE_H__
#define __AUDIONODE_H__

class Pipeline;
class SndFile;

const int FIFO_BUFFER = 256;

//---------------------------------------------------------
//   Fifo
//---------------------------------------------------------

struct FifoBuffer {
      float* buffer;
      int size;
      int maxSize;
      unsigned pos;
      int segs;

      FifoBuffer() {
            buffer  = 0;
            size    = 0;
            maxSize = 0;
            }
      };

class Fifo {
      int nbuffer;
      int ridx;               // read index; only touched by reader
      int widx;               // write index; only touched by writer
      volatile int counter;   // buffer count; writer increments, reader decrements
      FifoBuffer** buffer;

   public:
      Fifo();
      ~Fifo();
      void clear();
      bool put(int, unsigned long, float** buffer, unsigned pos);
      bool getWriteBuffer(int, unsigned long, float** buffer, unsigned pos);
      void add();
      bool get(int, unsigned long, float** buffer, unsigned pos);
      bool get(int, unsigned long, float** buffer);
      void remove();
      int count() const { return counter; }
      };


//---------------------------------------------------------
//   Fifo1
//---------------------------------------------------------

class Fifo1 {
   public:
      unsigned positions[FIFO_BUFFER];
      int ridx;               // read index; only touched by reader
      int widx;               // write index; only touched by writer
      volatile int counter;  // buffer count; writer increments, reader decrements

      Fifo1();
      ~Fifo1();
      void clear();
      int setWritePos(unsigned pos) {
            positions[widx] = pos;
            return widx;
            }
      int count() const        { return counter; }
      unsigned readPos() const { return positions[ridx]; }
      void put();
      void get();
      };

#endif

