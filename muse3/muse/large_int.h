//=========================================================
//  MusE
//  Linux Music Editor
//  
//  large_int.h
//  (C) Copyright 2018 Tim E. Real (terminator356 on sourceforge)
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

#ifndef __LARGE_INT_H__
#define __LARGE_INT_H__

#include "libdivide.h"

#if defined(_MSC_VER)
#define LARGE_INT_VC
#endif

#ifdef __cplusplus
#include <cstdlib>
#include <cstdio>
#else
#include <stdlib.h>
#include <stdio.h>
#endif

#include <stdint.h>

#if defined(LARGE_INT_VC)
#include <intrin.h>
#endif

#if defined(__SIZEOF_INT128__)
#define LARGE_INT_HAS_INT128_T
#endif

#if defined(__x86_64__) || defined(_WIN64) || defined(_M_X64)
#define LARGE_INT_IS_X86_64
#endif

namespace MusECore {

static inline uint64_t muse_multiply_64_div_64_to_64(
  uint64_t a, uint64_t b, uint64_t c, bool round_up = false)
{ 
#if defined(LARGE_INT_VC) && defined(LARGE_INT_IS_X86_64)
    uint64_t t_hi;
    const uint64_t t_lo = _umul128(a, b, &t_hi);
    uint64_t q, r;
    q = libdivide::libdivide_128_div_64_to_64(t_hi, t_lo, c, &r);
    if(round_up && r)
      q++;
    return q;
#elif defined(LARGE_INT_HAS_INT128_T)
    if(round_up)
    {
      __uint128_t dividend = (__uint128_t)a * (__uint128_t)b;
      // FIXME Too bad this is slowed by divide and mod. No 128-bit support for div() in stdlib. Go asm ?
      return dividend / (__uint128_t)c + (dividend % (__uint128_t)c ? 1 : 0);
    }
    return ((__uint128_t)a * (__uint128_t)b / (__uint128_t)c);
#else
  
  uint64_t u1 = (a & 0xffffffff);
  uint64_t v1 = (b & 0xffffffff);
  uint64_t t = (u1 * v1);
  uint64_t w3 = (t & 0xffffffff);
  uint64_t k = (t >> 32);

  a >>= 32;
  t = (a * v1) + k;
  k = (t & 0xffffffff);
  uint64_t w1 = (t >> 32);

  b >>= 32;
  t = (u1 * b) + k;
  k = (t >> 32);

  uint64_t q, r;
  q = libdivide::libdivide_128_div_64_to_64(
    (a * b) + w1 + k,
    (t << 32) + w3,
    c, &r);
  if(round_up && r)
    q++;
  return q;
  
#endif
}
  
} // namespace MusECore

#endif

