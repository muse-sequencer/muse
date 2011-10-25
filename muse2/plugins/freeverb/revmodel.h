//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: ./plugins/freeverb/revmodel.h $
//
//
// Written by Jezar at Dreampoint, June 2000
// http://www.dreampoint.co.uk
// This code is public domain
//
//=========================================================
// Reverb model declaration

#ifndef _revmodel_
#define _revmodel_

#include "comb.h"
#include "allpass.h"
#include "tuning.h"
#include "../../muse/ladspa.h"

//---------------------------------------------------------
//   Revmodel
//---------------------------------------------------------

class Revmodel {
      float	gain;
      float	roomsize,roomsize1;
      float	damp,damp1;
      float	width;
      float	mode;

      // Comb filters
      comb combL[numcombs];
      comb combR[numcombs];

      // Allpass filters
      allpass allpassL[numallpasses];
      allpass allpassR[numallpasses];

      // Buffers for the combs
      float	bufcombL1[combtuningL1];
      float	bufcombR1[combtuningR1];
      float	bufcombL2[combtuningL2];
      float	bufcombR2[combtuningR2];
      float	bufcombL3[combtuningL3];
      float	bufcombR3[combtuningR3];
      float	bufcombL4[combtuningL4];
      float	bufcombR4[combtuningR4];
      float	bufcombL5[combtuningL5];
      float	bufcombR5[combtuningR5];
      float	bufcombL6[combtuningL6];
      float	bufcombR6[combtuningR6];
      float	bufcombL7[combtuningL7];
      float	bufcombR7[combtuningR7];
      float	bufcombL8[combtuningL8];
      float	bufcombR8[combtuningR8];

      // Buffers for the allpasses
      float	bufallpassL1[allpasstuningL1];
      float	bufallpassR1[allpasstuningR1];
      float	bufallpassL2[allpasstuningL2];
      float	bufallpassR2[allpasstuningR2];
      float	bufallpassL3[allpasstuningL3];
      float	bufallpassR3[allpasstuningR3];
      float	bufallpassL4[allpasstuningL4];
      float	bufallpassR4[allpasstuningR4];
      void update();

   public:
      LADSPA_Data* port[7];
      float param[3];

      Revmodel();
	void	processmix(long numsamples);
	void	processreplace(long numsamples);
	void	setroomsize(float value);
	float	getroomsize();
	void	setdamp(float value);
	void	setwidth(float value);
	void	setmode(float value);
	float	getmode();
      void activate();
      };

#endif
