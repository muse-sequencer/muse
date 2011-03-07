//=========================================================
//  MusE
//  Linux Music Editor
//  $Id:$
//
//  based on "freeverb" written by Jezar at Dreampoint,
//    June 2000
//
//  (C) Copyright 2007 Werner Schweer (ws@seh.de)
//=========================================================

#ifndef __REVERB_H__
#define __REVERB_H__

#define undenormalise(sample)       \
      do {                             \
            volatile float __forced_float = 1e-29 + sample;  \
            sample = __forced_float - 1e-29;      \
      } while (0)

const int	numcombs		= 8;
const int	numallpasses	= 4;
const float	muted			= 0;
const float	fixedgain		= 0.015f;
const float scaledamp		= 0.4f;
const float scaleroom		= 0.28f;
const float offsetroom		= 0.7f;
const float initialwidth	= 1.0;
const int	stereospread	= 23;

// These values assume 44.1KHz sample rate
// they will probably be OK for 48KHz sample rate
// but would need scaling for 96KHz (or other) sample rates.
// The values were obtained by listening tests.
const int combtuningL1		= 1116;
const int combtuningR1		= 1116+stereospread;
const int combtuningL2		= 1188;
const int combtuningR2		= 1188+stereospread;
const int combtuningL3		= 1277;
const int combtuningR3		= 1277+stereospread;
const int combtuningL4		= 1356;
const int combtuningR4		= 1356+stereospread;
const int combtuningL5		= 1422;
const int combtuningR5		= 1422+stereospread;
const int combtuningL6		= 1491;
const int combtuningR6		= 1491+stereospread;
const int combtuningL7		= 1557;
const int combtuningR7		= 1557+stereospread;
const int combtuningL8		= 1617;
const int combtuningR8		= 1617+stereospread;
const int allpasstuningL1	= 556;
const int allpasstuningR1	= 556+stereospread;
const int allpasstuningL2	= 441;
const int allpasstuningR2	= 441+stereospread;
const int allpasstuningL3	= 341;
const int allpasstuningR3	= 341+stereospread;
const int allpasstuningL4	= 225;
const int allpasstuningR4	= 225+stereospread;

//---------------------------------------------------------
//   allpass
//---------------------------------------------------------

class allpass
      {
	float	feedback;
	float	*buffer;
	int bufsize;
	int bufidx;

   public:
      allpass() { bufidx = 0; }
	void	setbuffer(float *buf, int size) {
      	buffer = buf;
	      bufsize = size;
            }
      float process(float input) {
	      float bufout = buffer[bufidx];
      	undenormalise(bufout);
	      float output = -input + bufout;
      	buffer[bufidx] = input + (bufout*feedback);
            if (++bufidx >= bufsize)
                  bufidx = 0;
      	return output;
            }
	void	mute() {
            memset(buffer, 0, sizeof(float) * bufsize);
            }
	void	setfeedback(float val)  { feedback = val; }
	float	getfeedback()           { return feedback; }
      };

//---------------------------------------------------------
//   comb
//---------------------------------------------------------

class comb
      {
	float	feedback;
	float	filterstore;
	float	damp1;
	float	damp2;
	float	*buffer;
	int bufsize;
	int bufidx;

   public:
      comb() {
	      filterstore = 0;
	      bufidx = 0;
            }
	void	setbuffer(float *buf, int size) {
	      buffer = buf;
	      bufsize = size;
            }
      float process(float input) {
      	float output = buffer[bufidx];
	      undenormalise(output);
      	filterstore = (output*damp2) + (filterstore*damp1);
	      undenormalise(filterstore);
      	buffer[bufidx] = input + (filterstore*feedback);
            if (++bufidx >= bufsize)
                  bufidx = 0;
      	return output;
            }
	void	mute() {
      	for (int i=0; i<bufsize; i++)
	      	buffer[i]=0;
            }
	void	setdamp(float val) {
	      damp1 = val;
	      damp2 = 1-val;
            }
	float	getdamp()              { return damp1; }
	void	setfeedback(float val) { feedback = val; }
	float	getfeedback()          { return feedback; }
      };


//---------------------------------------------------------
//   Reverb
//---------------------------------------------------------

class Reverb {
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

      float roomSize;
      float damping;
      float wetLevel;

      void update();

   public:
      Reverb();
	void	process(float* l, float* r, int numsamples);
	void	setdamp(float value);
	void	setwidth(float value);

	void	setRoomSize(float value);
	void	setMix(float value);
      };

#endif

