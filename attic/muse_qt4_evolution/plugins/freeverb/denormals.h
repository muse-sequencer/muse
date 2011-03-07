// Macro for killing denormalled numbers
//
// Written by Jezar at Dreampoint, June 2000
// http://www.dreampoint.co.uk
// Based on IS_DENORMAL macro by Jon Watte
// This code is public domain

#ifndef _denormals_
#define _denormals_

// this does not work with at least gcc3.3 and -O2:
// #define undenormalise(sample) if(((*(unsigned int*)&sample)&0x7f800000)==0) sample=0.0f
//
// from Laurent de Soras Paper: Denormal numbers in floating point
//    signal processing applications
// (ws)

#if 0
#define undenormalise(sample)       \
      {                             \
      float anti_denormal = 1e-18;  \
      sample += anti_denormal;      \
      sample -= anti_denormal;      \
      }
#endif

// from beast-0.7.2  (Tim Janik/Stefan Westerfeld):

#define undenormalise(sample)       \
      do {                             \
            volatile float __forced_float = 1e-29 + sample;  \
            sample = __forced_float - 1e-29;      \
      } while (0)

#endif//_denormals_

//ends

