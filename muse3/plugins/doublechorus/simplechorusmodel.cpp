//===========================================================================
//
//    simplechorus
//
//    Version 0.0.1
//
//
//
//
//  Copyright (c) 2006 Nil Geisweiller
//
//
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
// 02111-1301, USA or point your web browser to http://www.gnu.org.
//===========================================================================

#include "simplechorusmodel.h"
#include <math.h>
#include <stdio.h>

#define ABS(x) (x>=0?x:-x)

// Linearly interpolate [ = a * (1 - f) + b * f]
inline float lin_interp(float f, float a, float b) {
  return a + f * (b - a);
}

// Cubic interpolation function
inline float cube_interp(const float fr,
			 const float inm1,
			 const float in,
			 const float inp1,
			 const float inp2) {
  return in + 0.5f * fr * (inp1 - inm1 +
			   fr * (4.0f * inp1 + 2.0f * inm1 - 5.0f * in - inp2 +
				 fr * (3.0f * (in - inp1) - inm1 + inp2)));
}

float SimpleChorusModel::sinus[MAXSINUSRESOLUTION];
int SimpleChorusModel::useCount = 0;

SimpleChorusModel::SimpleChorusModel(float samplerate) {
  _sampleRate = samplerate;
  //sinus
  if (useCount++ == 0)
    for(int i = 0; i < MAXSINUSRESOLUTION; i++)
      sinus[i] = (float)(sin(((double)i * 2.0 * M_PI) /
			      (double)MAXSINUSRESOLUTION));
  _index = 0.0;
  //init buffer
  for(int i = 0; i < MAXBUFFERLENGTH; i++) {
    _leftBuffer[i] = 0.0;
    _rightBuffer[i] = 0.0;
  }
  _position = 0;
  //initial parameters
  _pan = 0.5;
  _LFOFreq = 1.0;
  _depth = 0.5;
  setChorus();
}

SimpleChorusModel::~SimpleChorusModel() {
}

void SimpleChorusModel::process_chorus(float leftInput, float rightInput,
				       float* leftOutput, float* rightOutput) {
  float ocsDiff;

  _ocsDistance = _depthAmp * sinus[(int)_index];
  
  ocsDiff = _ocsDistance - floorf(_ocsDistance);

  _past_position_left = MAXBUFFERLENGTH //to be sure that _past_position_left>0
    + _position - _leftMidDistance + (int)_ocsDistance;
  _past_position_right = MAXBUFFERLENGTH
    + _position - _rightMidDistance + (int)_ocsDistance;

  *leftOutput = _leftAmp *
    lin_interp(ocsDiff, _leftBuffer[_past_position_left%MAXBUFFERLENGTH],
	       _leftBuffer[(_past_position_left+1)%MAXBUFFERLENGTH]);
  *rightOutput = _rightAmp *
    lin_interp(ocsDiff, _rightBuffer[_past_position_right%MAXBUFFERLENGTH],
	       _rightBuffer[(_past_position_right+1)%MAXBUFFERLENGTH]);

  _leftBuffer[_position] = leftInput;
  _rightBuffer[_position] = rightInput;
  
  _position++;
  _position %= MAXBUFFERLENGTH;
  
  _index += _inct;
  _index = (_index<MAXSINUSRESOLUTION?_index:_index-MAXSINUSRESOLUTION);
}

void SimpleChorusModel::setPan(float p) {
  _pan = p;
  setChorus();
}
void SimpleChorusModel::setLFOFreq(float l) {
  _LFOFreq = l;
  setChorus();
}
void SimpleChorusModel::setDepth(float d) {
  _depth = d;
  setChorus();
}
void SimpleChorusModel::setSampleRate(float s) {
  _sampleRate = s;
  setChorus();
}

float SimpleChorusModel::getPan() {
  return _pan;
}
float SimpleChorusModel::getLFOFreq() {
  return _LFOFreq;
}
float SimpleChorusModel::getDepth() {
  return _depth;
}

void SimpleChorusModel::setChorus() {
  //inct
  _inct = (float)MAXSINUSRESOLUTION/_sampleRate * _LFOFreq;
  //left & right amp
  _leftAmp = lin_interp(1.0 - _pan, 1.0 - PANAMP, 1.0 + PANAMP);
  _rightAmp = lin_interp(_pan, 1.0 - PANAMP, 1.0 + PANAMP);
  //left & right midDistance
  float leftmdm; //left mid distance in meter
  float rightmdm; //right mid distance in meter
  leftmdm = MIDSOURCEDISTANCE - EARSDISTANCE * (0.5 - _pan);
  rightmdm = MIDSOURCEDISTANCE + EARSDISTANCE * (0.5 - _pan);

  _leftMidDistance = (int)(_sampleRate * leftmdm / SOUNDSPEED);
  _rightMidDistance = (int)(_sampleRate * rightmdm / SOUNDSPEED);

  //depthAmp
  _depthAmp =
    _sampleRate * (MAXDEPTH * _depth) /SOUNDSPEED;
  //filter coef
  _filterCoef1 = 1 - COEFFILTER;
  _filterCoef2 = COEFFILTER;
}
