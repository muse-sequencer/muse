//=============================================================================
//  AL
//  Audio Utility Library
//  $Id: dsp.h,v 1.1.2.1 2009/12/06 01:39:33 terminator356 Exp $
//
//  Copyright (C) 1999-2011 by Werner Schweer and others
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License
//  as published by the Free Software Foundation; version 2 of
//  the License, or (at your option) any later version
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
//=============================================================================

#ifndef __DSP_H__
#define __DSP_H__

#include <string.h>
#include <math.h>

namespace AL {

//---------------------------------------------------------
//   f_max
//---------------------------------------------------------

static inline float f_max(float x, float a)
      {
      x -= a;
      x += fabsf(x);
      x *= 0.5f;
      x += a;
      return x;
      }

//---------------------------------------------------------
//   Dsp
//    standard version of all dsp routines without any
//    hw acceleration
//---------------------------------------------------------

class Dsp {
   public:
      Dsp() {}
      virtual ~Dsp() {}

      virtual float peak(float* buf, unsigned n, float current) {
            for (unsigned i = 0; i < n; ++i)
                  current = f_max(current, fabsf(buf[i]));
            return current;
            }
      virtual void applyGainToBuffer(float* buf, unsigned n, float gain) {
            for (unsigned i = 0; i < n; ++i)
                  buf[i] *= gain;
            }
      virtual void mixWithGain(float* dst, float* src, unsigned n, float gain) {
            for (unsigned i = 0; i < n; ++i)
                  dst[i] += src[i] * gain;
            }
      virtual void mix(float* dst, float* src, unsigned n) {
            for (unsigned i = 0; i < n; ++i)
                  dst[i] += src[i];
            }
      virtual void cpy(float* dst, float* src, unsigned n);
/*      
      {
// Changed by T356. Not defined. Where are these???
//#if defined(ARCH_X86) || defined(ARCH_X86_64)
#if defined(__i386__) || defined(__x86_64__)
            printf("Dsp: using asm cpy\n");
            // Changed by T356. Get To and From not declared in scope compile errors.
            register unsigned long int dummy;
            //__asm__ __volatile__ ("rep; movsl" :"=&D"(dst), "=&S"(src), "=&c"(dummy) :"0" (to), "1" (from),"2" (n) : "memory");
            // From http://www.ibiblio.org/gferg/ldp/GCC-Inline-Assembly-HOWTO.html            
            __asm__ __volatile__ (                              \
                       "cld\n\t"                                \
                       "rep\n\t"                                \
                       "movsl"                                  \
                       :                                        \
                       : "S" (src), "D" (dst), "c" (n)       \
                       : "memory"                
                       );

                       //: "%ecx", "%esi", "%edi", "memory"                
#else
            printf("Dsp: using memcpy\n");
            memcpy(dst, src, sizeof(float) * n);
#endif
            }
*/            
            
      };

extern void initDsp();
extern void exitDsp();
extern Dsp* dsp;

}

#endif

