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
// danvd: change i386 to more common __i386__ though boths are known to gcc and clang
// danvd: replace pthreads calls with builtin atomic operations
// danvd: turn on asm atomic operations for both i386 and x86_64

typedef struct { int counter; } muse_atomic_t;

static inline int muse_atomic_read(muse_atomic_t *v) {
#ifndef __i386__
      return __sync_fetch_and_add(&v->counter, 0);
#else
      return v->counter;
#endif
}

static inline void muse_atomic_set(muse_atomic_t *v, int i) {
#ifndef __i386__
      __sync_val_compare_and_swap(&v->counter, v->counter, i);
#else
      v->counter = i;
#endif
}
static inline void muse_atomic_inc(muse_atomic_t *v) {
#if !defined(__i386__) && !defined(__x86_64__)
      __sync_fetch_and_add(&v->counter, 1);
#else
        __asm__ __volatile__(
                "lock ; " "incl %0"
                :"=m" (v->counter)
                :"m" (v->counter));
#endif
}
static inline void muse_atomic_dec(muse_atomic_t *v) {
#if !defined(__i386__) && !defined(__x86_64__)
      __sync_fetch_and_sub(&v->counter, 1);
#else
        __asm__ __volatile__(
                "lock ; " "decl %0"
                :"=m" (v->counter)
                :"m" (v->counter));
#endif
}

static inline void muse_atomic_init(muse_atomic_t*) {}

static inline void muse_atomic_destroy(muse_atomic_t*) {}

} // namespace MusECore

#endif

