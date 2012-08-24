//===========================================================================
//
//    PanDelay, panoramic rotating delay
//
//    version 0.0.1
//
//    pandelaymodel.cpp
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
// 02110-1301, USA or point your web browser to http://www.gnu.org.
//===========================================================================

#include "pandelaymodel.h"
#include <stdio.h>

PanDelayModel::PanDelayModel(int samplerate) {
  for(int i = 0; i < MAXBUFFERLENGTH; i++) {
    _leftBuffer[i] = 0.0;
    _rightBuffer[i] = 0.0;
  }
  _bufferPointer = 0;
  _inc = 0.0;
  _l = 1.0;
  _r = 1.0;
  
  _samplerate = samplerate;
  _beatRatio = 4;
  setBPM(120);
  setPanDelay();
}

PanDelayModel::~PanDelayModel() {
}

void PanDelayModel::setSamplerate(int sr) {
  _samplerate = sr;
  setPanDelay();
}

void PanDelayModel::setBPM(float bpm) {
  _BPM = bpm;
  _delayTime = _beatRatio * 60.0 / _BPM;
  setPanDelay();  
}

void PanDelayModel::setBeatRatio(float br) {
  _beatRatio = br;
  _delayTime = _beatRatio * 60.0 / _BPM;
  setPanDelay();
}

void PanDelayModel::setDelayTime(float dt) {
  if(dt < MINDELAYTIME) _delayTime = MINDELAYTIME;
  else if(dt > MAXDELAYTIME) _delayTime = MAXDELAYTIME;
  else _delayTime = dt;
  setPanDelay();
}

void PanDelayModel::setFeedback(float fb) {
  _feedback = fb;
  setPanDelay();
}

void PanDelayModel::setPanLFOFreq(float pf) {
  _panLFOFreq = pf;
  setPanDelay();
}

void PanDelayModel::setPanLFODepth(float pd) {
  _panLFODepth = pd;
  setPanDelay();
}

void PanDelayModel::setDryWet(float dw) {
  _dryWet = dw;
}

void PanDelayModel::setPanDelay() {
  float numLFOSample = (1.0/_panLFOFreq) * (float)_samplerate;
  _inc = 2.0 / numLFOSample;
  _delaySampleSize = (int)(_delayTime * (float)_samplerate);
  _lBound = 1.0 - _panLFODepth;
  _rBound = 1.0 + _panLFODepth;
}

void PanDelayModel::processMix(float* leftSamplesIn, float* rightSamplesIn,
			       float* leftSamplesOut, float* rightSamplesOut,
			       unsigned n) {
  float ls, rs, p;
  p = 1.0 - _dryWet;
  for(unsigned i = 0; i < n; i++) {
    //read buffer
    ls = _leftBuffer[_bufferPointer];
    rs = _rightBuffer[_bufferPointer];
    //write buffer
    _leftBuffer[_bufferPointer] *= _feedback;
    _leftBuffer[_bufferPointer] += leftSamplesIn[i];
    _rightBuffer[_bufferPointer] *= _feedback;
    _rightBuffer[_bufferPointer] += rightSamplesIn[i];
    //write out
    leftSamplesOut[i] += _l * _dryWet * ls + p * leftSamplesIn[i];
    rightSamplesOut[i] += _r * _dryWet * rs + p * rightSamplesIn[i];
    //update _bufferPointer
    _bufferPointer++;
    _bufferPointer%=_delaySampleSize;
    //update _l _r
    _r += _inc;
    _l -= _inc;
    //update _inc
    if(_r > _rBound || _r < _lBound) _inc = -_inc;
  }
}

void PanDelayModel::processReplace(float* leftSamplesIn, float* rightSamplesIn,
				   float* leftSamplesOut,
				   float* rightSamplesOut, unsigned n) {
  float ls, rs, p;
  p = 1.0 - _dryWet;
  for(unsigned i = 0; i < n; i++) {
    //read buffer
    ls = _leftBuffer[_bufferPointer];
    rs = _rightBuffer[_bufferPointer];
    //write buffer
    _leftBuffer[_bufferPointer] *= _feedback;
    _leftBuffer[_bufferPointer] += leftSamplesIn[i];
    _rightBuffer[_bufferPointer] *= _feedback;
    _rightBuffer[_bufferPointer] += rightSamplesIn[i];
    //write out
    leftSamplesOut[i] = _l * _dryWet * ls + p * leftSamplesIn[i];
    rightSamplesOut[i] = _r * _dryWet * rs + p * rightSamplesIn[i];
    //update _bufferPointer
    _bufferPointer++;
    _bufferPointer%=_delaySampleSize;
    //update _l _r
    _r += _inc;
    _l -= _inc;
    //update _inc
    if(_r > _rBound || _r < _lBound) _inc = -_inc;
  }
}
