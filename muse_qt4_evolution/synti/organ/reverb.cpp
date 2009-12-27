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

#include "reverb.h"

//---------------------------------------------------------
//   Reverb
//---------------------------------------------------------

Reverb::Reverb()
      {
	// Tie the components to their buffers
	combL[0].setbuffer(bufcombL1,combtuningL1);
	combR[0].setbuffer(bufcombR1,combtuningR1);
	combL[1].setbuffer(bufcombL2,combtuningL2);
	combR[1].setbuffer(bufcombR2,combtuningR2);
	combL[2].setbuffer(bufcombL3,combtuningL3);
	combR[2].setbuffer(bufcombR3,combtuningR3);
	combL[3].setbuffer(bufcombL4,combtuningL4);
	combR[3].setbuffer(bufcombR4,combtuningR4);
	combL[4].setbuffer(bufcombL5,combtuningL5);
	combR[4].setbuffer(bufcombR5,combtuningR5);
	combL[5].setbuffer(bufcombL6,combtuningL6);
	combR[5].setbuffer(bufcombR6,combtuningR6);
	combL[6].setbuffer(bufcombL7,combtuningL7);
	combR[6].setbuffer(bufcombR7,combtuningR7);
	combL[7].setbuffer(bufcombL8,combtuningL8);
	combR[7].setbuffer(bufcombR8,combtuningR8);
	allpassL[0].setbuffer(bufallpassL1,allpasstuningL1);
	allpassR[0].setbuffer(bufallpassR1,allpasstuningR1);
	allpassL[1].setbuffer(bufallpassL2,allpasstuningL2);
	allpassR[1].setbuffer(bufallpassR2,allpasstuningR2);
	allpassL[2].setbuffer(bufallpassL3,allpasstuningL3);
	allpassR[2].setbuffer(bufallpassR3,allpasstuningR3);
	allpassL[3].setbuffer(bufallpassL4,allpasstuningL4);
	allpassR[3].setbuffer(bufallpassR4,allpasstuningR4);

	// Set default values
	allpassL[0].setfeedback(0.5f);
	allpassR[0].setfeedback(0.5f);
	allpassL[1].setfeedback(0.5f);
	allpassR[1].setfeedback(0.5f);
	allpassL[2].setfeedback(0.5f);
	allpassR[2].setfeedback(0.5f);
	allpassL[3].setfeedback(0.5f);
	allpassR[3].setfeedback(0.5f);

	setRoomSize(.5);
      setMix(.5);
	setdamp(.5);
	setwidth(initialwidth);

	// Buffer will be full of rubbish - so we MUST mute them

	for (int i = 0; i < numcombs; i++) {
		combL[i].mute();
		combR[i].mute();
            }
	for (int i=0;i<numallpasses;i++) {
		allpassL[i].mute();
		allpassR[i].mute();
            }
      }

//---------------------------------------------------------
//   update
//    Recalculate internal values after parameter change
//---------------------------------------------------------

void Reverb::update()
      {
      roomsize1 = roomsize;
	damp1     = damp;
	gain      = fixedgain;

	for (int i = 0; i < numcombs; i++) {
		combL[i].setfeedback(roomsize1);
		combR[i].setfeedback(roomsize1);
            }

	for (int i = 0; i < numcombs; i++) {
		combL[i].setdamp(damp1);
		combR[i].setdamp(damp1);
            }
      }

//---------------------------------------------------------
//   setroomsize
//---------------------------------------------------------

void Reverb::setRoomSize(float value)
      {
	roomsize = (value*scaleroom) + offsetroom;
	update();
      }

//---------------------------------------------------------
//   setdamp
//---------------------------------------------------------

void Reverb::setdamp(float value)
      {
	damp = value*scaledamp;
	update();
      }

//---------------------------------------------------------
//   setwidth
//---------------------------------------------------------

void Reverb::setwidth(float value)
      {
	width = value;
	update();
      }

//---------------------------------------------------------
//   setMix
//---------------------------------------------------------

void Reverb::setMix(float value)
      {
      wetLevel = value;
      }

//---------------------------------------------------------
//   process
//---------------------------------------------------------

void Reverb::process(float* l, float* r, int n)
      {
      float wet = wetLevel;
      float dry = 1.0 - wetLevel;

	for (int i = 0; i < n; ++i) {
		float outL  = 0.0;
		float outR  = 0.0;
		float input = (l[i] + r[i]) * gain;

		// Accumulate comb filters in parallel
		for (int k = 0; k < numcombs; k++) {
			outL += combL[k].process(input);
			outR += combR[k].process(input);
		      }

		// Feed through allpasses in series
		for (int k = 0; k < numallpasses; k++) {
			outL = allpassL[k].process(outL);
			outR = allpassR[k].process(outR);
		      }
		l[i] = outL * wet + l[i] * dry;
		r[i] = outR * wet + r[i] * dry;
	      }
      }

