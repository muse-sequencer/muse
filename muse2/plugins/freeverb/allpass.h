//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: ./plugins/freeverb/allpass.h $
//
// Written by Jezar at Dreampoint, June 2000
// http://www.dreampoint.co.uk
// This code is public domain
//
//=========================================================
// Allpass filter declaration
//

#ifndef _allpass_
#define _allpass_
#include "denormals.h"

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
//            bufidx = ++bufidx % bufsize;
      	return output;
            }
	void	mute() {
      	for (int i=0; i<bufsize; i++)
	      	buffer[i]=0;
            }
	void	setfeedback(float val)  { feedback = val; }
	float	getfeedback()           { return feedback; }
      };


// Big to inline - but crucial for speed


#endif//_allpass
