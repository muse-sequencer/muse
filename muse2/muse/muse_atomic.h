//=========================================================
//  MusE
//  Linux Music Editor
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

#ifndef __MUSE_ATOMIC_H__
#define __MUSE_ATOMIC_H__

namespace MusECore {

// FIXME: Compiler knows this define but where is it? Is compiler recognizing it? __i386__ and __i386 work.
#ifndef i386
#include <pthread.h>
typedef struct { pthread_mutex_t lock; int counter; } muse_atomic_t;
#else
typedef struct { int counter; } muse_atomic_t;
#endif

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

} // namespace MusECore

#endif

