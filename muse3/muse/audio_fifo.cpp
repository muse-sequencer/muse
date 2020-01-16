//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: node.cpp,v 1.36.2.25 2009/12/20 05:00:35 terminator356 Exp $
//
//  (C) Copyright 2000-2004 Werner Schweer (ws@seh.de)
//  (C) Copyright 2011-2013 Tim E. Real (terminator356 on sourceforge)
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

#include "audio_fifo.h"
#include "globals.h"
#include "al/dsp.h"

//#define FIFO_DEBUG

namespace MusECore {

//---------------------------------------------------------
//   Fifo
//---------------------------------------------------------

Fifo::Fifo()
      {
      muse_atomic_init(&count);
      nbuffer = MusEGlobal::fifoLength;
      buffer  = new FifoBuffer*[nbuffer];
      for (int i = 0; i < nbuffer; ++i)
            buffer[i]  = new FifoBuffer;
      clear();
      }

Fifo::~Fifo()
      {
      for (int i = 0; i < nbuffer; ++i)
      {
        if(buffer[i]->buffer)
          free(buffer[i]->buffer);

        delete buffer[i];
      }

      delete[] buffer;
      muse_atomic_destroy(&count);
      }

void Fifo::clear()
{
  #ifdef FIFO_DEBUG
  fprintf(stderr, "FIFO::clear count:%d\n", muse_atomic_read(&count));
  #endif

  ridx = 0;
  widx = 0;
  muse_atomic_set(&count, 0);
}

//---------------------------------------------------------
//   put
//    return true if fifo full
//---------------------------------------------------------

bool Fifo::put(int segs, MuseCount_t samples, float** src, MuseCount_t pos, float latency)
      {
      #ifdef FIFO_DEBUG
      fprintf(stderr, "FIFO::put segs:%d samples:%ld pos:%ld count:%d\n", segs, samples, pos, muse_atomic_read(&count));
      #endif

      if (muse_atomic_read(&count) == nbuffer) {
            fprintf(stderr, "FIFO %p overrun... %d\n", this, muse_atomic_read(&count));
            return true;
            }
      FifoBuffer* b = buffer[widx];
      MuseCount_t n         = segs * samples;
      if (b->maxSize < n) {
            if (b->buffer)
            {
              free(b->buffer);
              b->buffer = 0;
            }
#ifdef _WIN32
            b->buffer = (float *) _aligned_malloc(16, sizeof(float *) * n);
            if(b->buffer == NULL)
            {
               fprintf(stderr, "Fifo::put could not allocate buffer segs:%d samples:%lu pos:%u\n", segs, samples, pos);
               return true;
            }
#else
            int rv = posix_memalign((void**)&(b->buffer), 16, sizeof(float) * n);
            if(rv != 0 || !b->buffer)
            {
              fprintf(stderr, "Fifo::put could not allocate buffer segs:%d samples:%ld pos:%ld\n", segs, samples, pos);
              return true;
            }
#endif
            b->maxSize = n;
            }
      if(!b->buffer)
      {
        fprintf(stderr, "Fifo::put no buffer! segs:%d samples:%ld pos:%ld\n", segs, samples, pos);
        return true;
      }

      b->size = samples;
      b->segs = segs;
      b->pos  = pos;
      b->latency = latency;
      for (int i = 0; i < segs; ++i)
            AL::dsp->cpy(b->buffer + i * samples, src[i], samples);
      add();
      return false;
      }

//---------------------------------------------------------
//   peek
//    return true if fifo empty
//---------------------------------------------------------

bool Fifo::peek(int segs, MuseCount_t samples, float** dst, MuseCount_t* pos, float* latency) //const
      {
      #ifdef FIFO_DEBUG
      fprintf(stderr, "FIFO::peek/get segs:%d samples:%ld count:%d\n", segs, samples, muse_atomic_read(&count));
      #endif

      // Non-const peek required because of this.
      if (muse_atomic_read(&count) == 0) {
            fprintf(stderr, "FIFO %p underrun\n", this);
            return true;
            }
      FifoBuffer* b = buffer[ridx];
      if(!b->buffer)
      {
        fprintf(stderr, "Fifo::peek/get no buffer! segs:%d samples:%ld b->pos:%ld\n", segs, samples, b->pos);
        return true;
      }

      if (pos)
            *pos = b->pos;
      if (latency)
            *latency = b->latency;

      for (int i = 0; i < segs; ++i)
            dst[i] = b->buffer + samples * (i % b->segs);
      return false;
      }

//---------------------------------------------------------
//   get
//    return true if fifo empty
//---------------------------------------------------------

bool Fifo::get(int segs, MuseCount_t samples, float** dst, MuseCount_t* pos, float* latency)
      {
      if(peek(segs, samples, dst, pos, latency))
        return true;
      remove();
      return false;
      }

int Fifo::getCount()
      {
      return muse_atomic_read(&count);
      }

int Fifo::getEmptyCount()
{
  return nbuffer - muse_atomic_read(&count);
}

bool Fifo::isEmpty()
      {
      return muse_atomic_read(&count) == 0;
      }

//---------------------------------------------------------
//   remove
//---------------------------------------------------------

void Fifo::remove()
      {
      #ifdef FIFO_DEBUG
      fprintf(stderr, "Fifo::remove count:%d\n", muse_atomic_read(&count));
      #endif

      ridx = (ridx + 1) % nbuffer;
      muse_atomic_dec(&count);
      }

//---------------------------------------------------------
//   getWriteBuffer
//---------------------------------------------------------

bool Fifo::getWriteBuffer(int segs, MuseCount_t samples, float** buf, MuseCount_t pos)
      {
      #ifdef FIFO_DEBUG
      fprintf(stderr, "Fifo::getWriteBuffer segs:%d samples:%ld pos:%ld\n", segs, samples, pos);
      #endif

      if (muse_atomic_read(&count) == nbuffer)
            return true;
      FifoBuffer* b = buffer[widx];
      MuseCount_t n = segs * samples;
      if (b->maxSize < n) {
            if (b->buffer)
            {
              free(b->buffer);
              b->buffer = 0;
            }

            int rv = posix_memalign((void**)&(b->buffer), 16, sizeof(float) * n);
            if(rv != 0 || !b->buffer)
            {
              fprintf(stderr, "Fifo::getWriteBuffer could not allocate buffer segs:%d samples:%ld pos:%ld\n", segs, samples, pos);
              return true;
            }

            b->maxSize = n;
            }
      if(!b->buffer)
      {
        fprintf(stderr, "Fifo::getWriteBuffer no buffer! segs:%d samples:%ld pos:%ld\n", segs, samples, pos);
        return true;
      }

      for (int i = 0; i < segs; ++i)
            buf[i] = b->buffer + i * samples;

      b->size = samples;
      b->segs = segs;
      b->pos  = pos;
      return false;
      }

//---------------------------------------------------------
//   add
//---------------------------------------------------------

void Fifo::add()
      {
      #ifdef FIFO_DEBUG
      fprintf(stderr, "Fifo::add count:%d\n", muse_atomic_read(&count));
      #endif

      widx = (widx + 1) % nbuffer;
      muse_atomic_inc(&count);
      }

} // namespace MusECore
