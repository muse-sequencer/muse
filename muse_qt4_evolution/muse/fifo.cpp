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

#include "fifo.h"
#include "globals.h"
#include "al/dsp.h"

//---------------------------------------------------------
//   clear
//---------------------------------------------------------

void FifoBase::clear()
	{
	ridx    = 0;
      widx    = 0;
      counter = 0;
      }

//---------------------------------------------------------
//   push
//---------------------------------------------------------

void FifoBase::push()
      {
      widx = (widx + 1) % FIFO_BUFFER;
//      q_atomic_increment(&counter);
      ++counter;
      }

//---------------------------------------------------------
//   pop
//---------------------------------------------------------

void FifoBase::pop()
      {
      ridx = (ridx + 1) % FIFO_BUFFER;
      // q_atomic_decrement(&counter);
      --counter;
      }

//---------------------------------------------------------
//   Fifo
//---------------------------------------------------------

Fifo::Fifo()
      {
      nbuffer = FIFO_BUFFER;
      buffer  = new FifoBuffer*[nbuffer];
      for (int i = 0; i < nbuffer; ++i)
            buffer[i]  = new FifoBuffer;
      clear();
      }

Fifo::~Fifo()
      {
      for (int i = 0; i < nbuffer; ++i)
            delete buffer[i];
      delete[] buffer;
      }

//---------------------------------------------------------
//   put
//    return true if fifo full
//---------------------------------------------------------

bool Fifo::put(int segs, unsigned long samples, float** src, unsigned pos)
      {
      if (counter == nbuffer) {
            printf("FIFO %p overrun at 0x%x\n", this, pos);
            return true;
            }
      FifoBuffer* b = buffer[widx];
      int n         = segs * samples;
      if (b->maxSize < n) {
            if (b->buffer)
                 free(b->buffer);
            posix_memalign((void**)&(b->buffer), 16, sizeof(float) * n);
            b->maxSize = n;
            }
      b->size = samples;
      b->segs = segs;
      b->pos  = pos;
      for (int i = 0; i < segs; ++i)
            AL::dsp->cpy(b->buffer + i * samples, src[i], samples);
      push();
      return false;
      }

//---------------------------------------------------------
//   get
//    return true if fifo empty
//---------------------------------------------------------

bool Fifo::get(int segs, unsigned long samples, float** dst, unsigned pos)
      {
      FifoBuffer* b;
      bool errMsg = true;
      for (;;) {
            if (counter == 0) {
			printf("FIFO %p underrun at 0x%x\n", this, pos);
                  return true;
                  }
            b = buffer[ridx];
            if (pos == b->pos)
                  break;
            //
            // skip all buffer until we get the rigth one or the
            // fifo is empty
            //
//            if (errMsg)
			printf("Fifo %p::get(0x%x) n=%d, discard wrong prefetch block(s) 0x%x\n",
                     this, pos, counter, b->pos);
            pop();
            errMsg = false;
            }
      for (int i = 0; i < segs; ++i)
            dst[i] = b->buffer + samples * (i % b->segs);
      pop();
      return false;
      }

//---------------------------------------------------------
//   get
//    return true if fifo empty
//---------------------------------------------------------

bool Fifo::get(int segs, unsigned long samples, float** dst)
      {
      FifoBuffer* b;
	if (counter == 0) {
		printf("FIFO %p underrun --cannot happen!\n", this);
		return true;
		}
  	b = buffer[ridx];
      for (int i = 0; i < segs; ++i)
            dst[i] = b->buffer + samples * (i % b->segs);
      pop();
      return false;
      }

//---------------------------------------------------------
//   getWriteBuffer
//    return true, if no more buffer available
//    (overflow)
//---------------------------------------------------------

bool Fifo::getWriteBuffer(int segs, unsigned long samples, float** buf, unsigned pos)
      {
      if (counter == nbuffer)
            return true;
      FifoBuffer* b = buffer[widx];
      int n = segs * samples;
      if (b->maxSize < n) {
            if (b->buffer)
                 free(b->buffer);
            posix_memalign((void**)&(b->buffer), 16, sizeof(float) * n);
            b->maxSize = n;
            }
      for (int i = 0; i < segs; ++i)
            buf[i] = b->buffer + i * samples;
      b->size = samples;
      b->segs = segs;
      b->pos  = pos;
      return false;
      }

