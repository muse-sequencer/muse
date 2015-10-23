//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: ./plugins/freeverb/revmodel.cpp $
//
// Written by Jezar at Dreampoint, June 2000
// http://www.dreampoint.co.uk
// This code is public domain
//
//=========================================================
// Reverb model implementation
//

#include <stdio.h>
#include "revmodel.h"

//---------------------------------------------------------
//   Revmodel
//---------------------------------------------------------

Revmodel::Revmodel()
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

      param[0] = initialroom;
      param[1] = initialdamp;
      param[2] = initialwet;

	setroomsize(initialroom);
	setdamp(initialdamp);
	setwidth(initialwidth);
	setmode(initialmode);

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
//   activate
//---------------------------------------------------------

void Revmodel::activate()
      {
      *port[4] = param[0];
      *port[5] = param[1];
      *port[6] = param[2];
      }

//---------------------------------------------------------
//   processreplace
//---------------------------------------------------------

void Revmodel::processreplace(long n)
      {
      if (param[0] != *port[4]) {
            param[0] = *port[4];
            setroomsize(param[0]);
            }
      if (param[1] != *port[5]) {
            param[1] = *port[5];
            setdamp(param[1]);
            }

      float wet  = (1.0f - *port[6]) * scalewet;
      float dry  = *port[6] * scaledry;
	float wet1 = wet * (width/2 + 0.5f);
	float wet2 = wet * ((1-width)/2);

	for (int i = 0; i < n; ++i) {
		float outL  = 0;
		float outR  = 0;
		float input = (port[0][i] + port[1][i]) * gain;

		// Accumulate comb filters in parallel
		for (int k = 0; k < numcombs; k++) {
			outL += combL[k].process(input);
			outR += combR[k].process(input);
		      }

		// Feed through allpasses in series
		for (int k=0; k < numallpasses; k++) {
			outL = allpassL[k].process(outL);
			outR = allpassR[k].process(outR);
		      }

		// Calculate output REPLACING anything already there
		port[2][i] = outL*wet1 + outR*wet2 + port[0][i]*dry;
		port[3][i] = outR*wet1 + outL*wet2 + port[1][i]*dry;
	      }
      }

void Revmodel::processmix(long n)
      {
      if (param[0] != *port[4]) {
            param[0] = *port[4];
            setroomsize(param[0]);
            }
      if (param[1] != *port[5]) {
            param[1] = *port[5];
            setdamp(param[1]);
            }

      float wet  = (1.0f - *port[6]) * scalewet;
      float dry  = *port[6] * scaledry;
	float wet1 = wet * (width/2 + 0.5f);
	float wet2 = wet * ((1-width)/2);

	for (int i = 0; i < n; ++i) {
		float outL  = 0;
		float outR  = 0;
		float input = (port[0][i] + port[1][i]) * gain;

		// Accumulate comb filters in parallel
		for (int k = 0; k < numcombs; k++) {
			outL += combL[k].process(input);
			outR += combR[k].process(input);
		      }

		// Feed through allpasses in series
		for (int k=0; k < numallpasses; k++) {
			outL = allpassL[k].process(outL);
			outR = allpassR[k].process(outR);
		      }

		// Calculate output REPLACING anything already there
		port[2][i] += outL*wet1 + outR*wet2 + port[0][i]*dry;
		port[3][i] += outR*wet1 + outL*wet2 + port[1][i]*dry;
	      }
      }

//---------------------------------------------------------
//   update
//    Recalculate internal values after parameter change
//---------------------------------------------------------

void Revmodel::update()
      {
	if (mode >= freezemode) {
		roomsize1 = 1;
		damp1     = 0;
		gain      = muted;
            }
	else {
            roomsize1 = roomsize;
		damp1     = damp;
		gain      = fixedgain;
            }

	for (int i = 0; i < numcombs; i++) {
		combL[i].setfeedback(roomsize1);
		combR[i].setfeedback(roomsize1);
            }

	for (int i = 0; i < numcombs; i++) {
		combL[i].setdamp(damp1);
		combR[i].setdamp(damp1);
            }
      }

// The following get/set functions are not inlined, because
// speed is never an issue when calling them, and also
// because as you develop the reverb model, you may
// wish to take dynamic action when they are called.

void Revmodel::setroomsize(float value)
      {
	roomsize = (value*scaleroom) + offsetroom;
	update();
      }

float Revmodel::getroomsize()
      {
	return (roomsize-offsetroom)/scaleroom;
      }

void Revmodel::setdamp(float value)
      {
	damp = value*scaledamp;
	update();
      }

void Revmodel::setwidth(float value)
      {
	width = value;
	update();
      }

void Revmodel::setmode(float value)
      {
	mode = value;
	update();
      }

float Revmodel::getmode()
      {
	return (mode >= freezemode) ? 1 : 0;
      }
