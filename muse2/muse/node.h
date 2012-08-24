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

#include <list>

#ifndef i386
#include <pthread.h>
typedef struct { pthread_mutex_t lock; int counter; } muse_atomic_t;
#else
typedef struct { int counter; } muse_atomic_t;
#endif

namespace MusECore {

static inline int muse_atomic_read(muse_atomic_t *v) {
#ifndef i386
      int ret;
      pthread_mutex_lock(&v->lock);
      ret = v->counter;
      pthread_mutex_unlock(&v->lock);
      return ret;
#else
      return v->counter;
#endif
}

static inline void muse_atomic_set(muse_atomic_t *v, int i) {
#ifndef i386
      pthread_mutex_lock(&v->lock);
      v->counter = i;
      pthread_mutex_unlock(&v->lock);
#else
      v->counter = i;
#endif
}
static inline void muse_atomic_inc(muse_atomic_t *v) {
#ifndef i386
      pthread_mutex_lock(&v->lock);
      v->counter++;
      pthread_mutex_unlock(&v->lock);
#else
	__asm__ __volatile__(
		"lock ; " "incl %0"
		:"=m" (v->counter)
		:"m" (v->counter));
#endif
}
static inline void muse_atomic_dec(muse_atomic_t *v) {
#ifndef i386
      pthread_mutex_lock(&v->lock);
      v->counter--;
      pthread_mutex_unlock(&v->lock);
#else
	__asm__ __volatile__(
		"lock ; " "decl %0"
		:"=m" (v->counter)
		:"m" (v->counter));
#endif
}
#ifndef i386
static inline void muse_atomic_init(muse_atomic_t *v) {
      pthread_mutex_init(&v->lock, NULL);
      }
#else
static inline void muse_atomic_init(muse_atomic_t*) {}
#endif

#ifndef i386
static inline void muse_atomic_destroy(muse_atomic_t *v) {
      pthread_mutex_destroy(&v->lock);
      }
#else
static inline void muse_atomic_destroy(muse_atomic_t*) {}
#endif

class Xml;
class Pipeline;


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
      muse_atomic_t count;         // buffer count; writer increments, reader decrements
      FifoBuffer** buffer;

   public:
      Fifo();
      ~Fifo();
      void clear() {
            ridx = 0;
            widx = 0;
            muse_atomic_set(&count, 0);
            }
      bool put(int, unsigned long, float** buffer, unsigned pos);
      bool getWriteBuffer(int, unsigned long, float** buffer, unsigned pos);
      void add();
      bool get(int, unsigned long, float** buffer, unsigned* pos);
      void remove();
      int getCount();
      };

} // namespace MusECore

#endif

