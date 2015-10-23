//=============================================================================
//  AL
//  Audio Utility Library
//  $Id: dsp.cpp,v 1.1.2.1 2009/12/06 01:39:33 terminator356 Exp $
//
//  Copyright (C) 1999-2011 by Werner Schweer and others
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License
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
//=============================================================================

#include <stdio.h>
#include <stdint.h>
#include "dsp.h"
#include "config.h"
#include "../globals.h"

namespace AL {

Dsp* dsp = 0;

#ifdef __i386__

//---------------------------------------------------------
//   DspSSE86
//---------------------------------------------------------

extern "C" {
extern float x86_sse_compute_peak(float*, unsigned, float);
extern void x86_sse_apply_gain_to_buffer(float*, unsigned, float);
extern void x86_sse_mix_buffers_with_gain(float*, float*, unsigned, float);
extern void x86_sse_mix_buffers_no_gain(float*, float*, unsigned);
   };

class DspSSE86 : public Dsp {
   public:
      DspSSE86() {}
      virtual ~DspSSE86() {}

      virtual float peak(float* buf, unsigned n, float current) {
            if ( ((intptr_t)buf % 16) != 0) {
                  fprintf(stderr, "peak(): buffer unaligned! (%p)\n", buf);
                  return Dsp::peak(buf, n, current);
                  }
            return x86_sse_compute_peak(buf, n, current);
            }

      virtual void applyGainToBuffer(float* buf, unsigned n, float gain) {
            if ( ((intptr_t)buf % 16) != 0) {
                  fprintf(stderr, "applyGainToBuffer(): buffer unaligned! (%p)\n", buf);
                  Dsp::applyGainToBuffer(buf, n, gain);
                  }
            else
                  x86_sse_apply_gain_to_buffer(buf, n, gain);
            }

      virtual void mixWithGain(float* dst, float* src, unsigned n, float gain) {
            if ( ((intptr_t)dst & 15) != 0)
                  fprintf(stderr, "mixWithGainain(): dst unaligned! (%p)\n", dst);
            if (((intptr_t)dst & 15) != ((intptr_t)src & 15) ) {
                  fprintf(stderr, "mixWithGain(): dst & src don't have the same alignment!\n");
                  Dsp::mixWithGain(dst, src,n, gain);
                  }
            else
                  x86_sse_mix_buffers_with_gain(dst, src, n, gain);
            }
      virtual void mix(float* dst, float* src, unsigned n) {
            if ( ((intptr_t)dst & 15) != 0)
                  fprintf(stderr, "mix_buffers_no_gain(): dst unaligned! %p\n", dst);
            if ( ((intptr_t)dst & 15) != ((intptr_t)src & 15) ) {
                  fprintf(stderr, "mix_buffers_no_gain(): dst & src don't have the same alignment!\n");
                  Dsp::mix(dst, src, n);
                  }
            else
                  x86_sse_mix_buffers_no_gain(dst, src, n);
            }
      };
#endif

//---------------------------------------------------------
//   initDsp
//---------------------------------------------------------

void initDsp()
      {
#if 0    // Disabled for now.
#if defined(__i386__) || defined(__x86_64__)
      if(debugMsg)
        printf("Muse: __i386__ or __x86_64__ defined. Using optimized float buffer copying (asm movsl).\n");
#else
      if(debugMsg)
        printf("Muse: __i386__ or __x86_64__ not defined. Using non-optimized memcpy for float buffer copying.\n");
#endif
#endif

#if defined(__i386__) && defined(USE_SSE)
      unsigned long useSSE = 0;
      if(debugMsg)
        printf("initDsp: __i386__ and USE_SSE defined\n");

// FIXME: 64? Shouldn't these routines work on 32 bit?
#ifdef __x86_64__
      useSSE = 1 << 25;       // we know the platform has SSE
      if(debugMsg)
        printf("initDsp: __x86_64__ defined\n");
#else
      if(debugMsg)
        printf("initDsp: getting cpuid via asm\n");
      asm (
         "mov $1, %%eax\n"
         "pushl %%ebx\n"
         "cpuid\n"
         "movl %%edx, %0\n"
         "popl %%ebx\n"
         : "=r" (useSSE)
         :
         : "%eax", "%ecx", "%edx", "memory");
#endif
      if(debugMsg)
        printf("initDsp: checking for bit 25 SSE support:%lX\n", useSSE);
      useSSE &= (1 << 25); // bit 25 = SSE support
      if (useSSE) {
            printf("Using SSE optimized routines\n");
            dsp = new DspSSE86();
            return;
            }
      // fall through to not hardware optimized routines
#endif
      if(MusEGlobal::debugMsg)
        printf("Muse: using unoptimized non-SSE dsp routines\n");
      dsp = new Dsp();
      }

//---------------------------------------------------------
//   exitDsp
//---------------------------------------------------------

void exitDsp()
{
  if(dsp)
    delete dsp;
  dsp = 0;  
}

void Dsp::cpy(float* dst, float* src, unsigned n)
{
// FIXME: Changed by T356. Not defined. Where are these???
//#if defined(ARCH_X86) || defined(ARCH_X86_64)
///#if defined(__i386__) || defined(__x86_64__)
#if 0  // Disabled for now.
            //printf("Dsp: using asm cpy\n");
            // Changed by T356. Get To and From not declared in scope compile errors.
            //register unsigned long int dummy;
            //__asm__ __volatile__ ("rep; movsl" :"=&D"(dst), "=&S"(src), "=&c"(dummy) :"0" (to), "1" (from),"2" (n) : "memory");
            // FIXME: FIXME: I don't think this is correct but it works so far...
            // Tried clobbering, get "Can't find a register in class `CREG' while reloading `asm'"
            __asm__ __volatile__ (                              
                       "cld\n\t"                                
                       "rep\n\t"                                
                       "movsl"                                  
                       :                                        
                       : "S" (src), "D" (dst), "c" (n)       
                       : "memory"                
                       );

                       //: "%ecx", "%esi", "%edi", "memory"                
#else
            //printf("Dsp: using memcpy\n");
            memcpy(dst, src, sizeof(float) * n);
#endif
}

} // namespace AL
