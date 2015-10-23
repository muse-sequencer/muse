//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: ./plugins/freeverb/comb.h $
//
// Written by Jezar at Dreampoint, June 2000
// http://www.dreampoint.co.uk
// This code is public domain
//
//=========================================================
// Comb filter class declaration
//

#ifndef _comb_
#define _comb_

#include "denormals.h"


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
//            bufidx = ++bufidx % bufsize;
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


// Big to inline - but crucial for speed


#endif //_comb_

//ends
