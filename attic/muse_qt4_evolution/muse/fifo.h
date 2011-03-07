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

#ifndef __FIFO_H__
#define __FIFO_H__

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

//---------------------------------------------------------
//   FifoBase
//    - works only for one reader/writer
//    - reader writes ridx
//    - writer writes widx
//    - reader decrements counter
//    - writer increments counter
//    - counter increment/decrement must be atomic
//---------------------------------------------------------

class FifoBase {

   protected:
      int ridx;               // read index
      int widx;               // write index
      volatile int counter;   // objects in fifo

   public:
      FifoBase()          { clear(); }
      virtual ~FifoBase() {}
      void clear();
      virtual void push();    // put object on fifo
      virtual void pop();     // remove object from fifo
      int count() const     { return counter; }
      int readIndex() const { return ridx; }
      };

//---------------------------------------------------------
//   Fifo
//---------------------------------------------------------

class Fifo : public FifoBase {
      int nbuffer;            // max buffer size (fifo-size)
      FifoBuffer** buffer;

   public:
      Fifo();
      ~Fifo();
      bool put(int, unsigned long, float** buffer, unsigned pos);
      bool getWriteBuffer(int, unsigned long, float** buffer, unsigned pos);
      bool get(int, unsigned long, float** buffer, unsigned pos);
      bool get(int, unsigned long, float** buffer);
      };

//---------------------------------------------------------
//   Fifo1
//---------------------------------------------------------

class Fifo1 : public FifoBase {
   public:
      unsigned positions[FIFO_BUFFER];

      Fifo1() : FifoBase() {}
      int setWritePos(unsigned pos) {
            positions[widx] = pos;
            return widx;
            }
      unsigned readPos() const { return positions[ridx]; }
      };

#endif

