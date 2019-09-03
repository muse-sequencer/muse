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

enum LargeIntRoundMode {
  // Round down and remainder zero.
  LargeIntRoundDown,
  // Round up and remainder zero.
  LargeIntRoundUp,
  // Round to nearest and remainder zero.
  LargeIntRoundNearest,
  // No rounding and remainder is valid.
  LargeIntRoundNone };
  
  
static inline uint64_t muse_multiply_64_div_64_to_64(
  uint64_t a, uint64_t b, uint64_t c, LargeIntRoundMode round_mode = LargeIntRoundDown, uint64_t* remainder = nullptr)
{ 
#if defined(LARGE_INT_VC) && defined(LARGE_INT_IS_X86_64)
    uint64_t t_hi;
    const uint64_t t_lo = _umul128(a, b, &t_hi);
    uint64_t q, r;
    q = libdivide::libdivide_128_div_64_to_64(t_hi, t_lo, c, &r);
    switch(round_mode)
    {
      case LargeIntRoundDown:
        if(remainder)
          *remainder = 0;
      break;
      case LargeIntRoundUp:
        if(r)
          q++;
        if(remainder)
          *remainder = 0;
      break;
      case LargeIntRoundNearest:
        if(r >= (c / 2))
          q++;
        if(remainder)
          *remainder = 0;
      break;
      case LargeIntRoundNone:
        if(remainder)
          *remainder = r;
      break;
    }
    return q;
#elif defined(LARGE_INT_HAS_INT128_T)
    switch(round_mode)
    {
      case LargeIntRoundDown:
      break;

      case LargeIntRoundUp:
      {
        const __uint128_t dividend = (__uint128_t)a * (__uint128_t)b;
        const __uint128_t cc = (__uint128_t)c;
        if(remainder)
          *remainder = 0;
        // FIXME Too bad this is slowed by divide and mod. No 128-bit support for div() in stdlib. Go asm ?
        return dividend / cc + (dividend % cc ? 1 : 0);
      }
      break;

      case LargeIntRoundNearest:
      {
        const __uint128_t dividend = (__uint128_t)a * (__uint128_t)b;
        const __uint128_t cc = (__uint128_t)c;
        if(remainder)
          *remainder = 0;
        // FIXME Too bad this is slowed by divide and mod. No 128-bit support for div() in stdlib. Go asm ?
        return dividend / cc + ((dividend % cc >= (cc / 2)) ? 1 : 0);
      }
      break;

      case LargeIntRoundNone:
        const __uint128_t dividend = (__uint128_t)a * (__uint128_t)b;
        const __uint128_t cc = (__uint128_t)c;
        const __uint128_t q = dividend / cc;
        const __uint128_t r = dividend % cc;
        if(remainder)
          *remainder = r;
        return q;
      break;
    }
    
    if(remainder)
      *remainder = 0;
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
  switch(round_mode)
  {
    case LargeIntRoundDown:
      if(remainder)
        *remainder = 0;
    break;
    case LargeIntRoundUp:
      if(r)
        q++;
      if(remainder)
        *remainder = 0;
    break;
    case LargeIntRoundNearest:
      if(r >= (c / 2))
        q++;
      if(remainder)
        *remainder = 0;
    break;
    case LargeIntRoundNone:
      if(remainder)
        *remainder = r;
    break;
  }
  return q;
  
#endif
}
  
// class Fixed_U64
// {
// public:
//   uint64_t _dividend;
//   uint64_t _divisor;
//   //uint64_t _quotient;
//   uint64_t _remainder;
//   
//   //Fixed_U64() : _dividend(0), _divisor(1), _quotient(0), _remainder(0) { }
//   Fixed_U64() : _dividend(0), _divisor(1), _remainder(0) { }
//   //Fixed_U64(uint64_t dividend, uint64_t divisor = 1, uint64_t quotient = 0, uint64_t remainder = 0) :
//   //  _dividend(dividend), _divisor(divisor), _quotient(quotient), _remainder(remainder) { }
//   Fixed_U64(uint64_t dividend, uint64_t divisor = 1, uint64_t remainder = 0) :
//     _dividend(dividend), _divisor(divisor), _remainder(remainder) { }
//   
//   Fixed_U64 operator+(const Fixed_U64& val) 
//   { 
//     uint64_t dd = _dividend + val._dividend;
//     uint64_t dv;
//     uint64_t r;
//     if(val._divisor != _divisor)
//     {
//       dv = _dividend * val._dividend;
//       r = _remainder * val._dividend;
//     }
//     
//     //uint64_t q = _dividend + val._dividend;
//     //uint64_t r = _dividend + val._dividend;
//     //return Fixed_U64(dd, dv, q, r);
//     return Fixed_U64(dd, dv, r);
//   }
// };
  
} // namespace MusECore

#endif

