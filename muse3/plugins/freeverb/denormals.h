//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: ./plugins/freeverb/denormals.h $
//
// Written by Jezar at Dreampoint, June 2000
// http://www.dreampoint.co.uk
// Based on IS_DENORMAL macro by Jon Watte
// This code is public domain
//
//=========================================================
// Macro for killing denormalled numbers
//

#ifndef _denormals_
#define _denormals_

// this does not work with at least gcc3.3 and -O2:
// #define undenormalise(sample) if(((*(unsigned int*)&sample)&0x7f800000)==0) sample=0.0f
//
// from Laurent de Soras Paper: Denormal numbers in floating point
//    signal processing applications
// (ws)

#define undenormalise(sample)       \
      {                             \
      float anti_denormal = 1e-18;  \
      sample += anti_denormal;      \
      sample -= anti_denormal;      \
      }

#endif//_denormals_

//ends

