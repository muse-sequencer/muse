//===========================================================================
//
//    PanDelay, panoramic rotating delay
//
//    version 0.0.1
//
//    pandelaymodel.h
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

#ifndef __PANDELAYMODEL_H
#define __PANDELAYMODEL_H

#include <math.h>

#define MAXBUFFERLENGTH 192000
#define MINFREQ 0.1 //in Hz
#define MAXFREQ 10.0 //in Hz
#define MINBPM 60.0
#define MAXBPM 255.0
#define MINBEATRATIO 0.125
#define MAXBEATRATIO 2.0
#define MINDELAYTIME 0.01 //in second
#define MAXDELAYTIME 2.0 //in second

#ifdef NBRPARAM
#undef NBRPARAM
#endif
#define NBRPARAM 5

class PanDelayModel {
 private:
  int _samplerate;

  //bool _beatFraction; //if true then the delay is calculated in beat fraction
  float _BPM;
  float _beatRatio;
  float _delayTime; //delay is calculated according to BMP and ratioBMP
  float _feedback;
  float _panLFOFreq;
  float _panLFODepth;
  float _dryWet; //0.0 : dry, 1.0 : wet

  int _delaySampleSize;
  float _lBound;
  float _rBound;
  float _inc;
  float _l;
  float _r;

  float _leftBuffer[MAXBUFFERLENGTH];
  float _rightBuffer[MAXBUFFERLENGTH];
  int _bufferPointer;

 public:
  PanDelayModel(int samplerate);
  ~PanDelayModel();
  
  void setSamplerate(int sr);
  void setBeatRatio(float br);
  void setBPM(float bpm);
  void setDelayTime(float dt);
  void setFeedback(float dt);
  void setPanLFOFreq(float pf);
  void setPanLFODepth(float pd);
  void setDryWet(float dw);
  void setPanDelay();

  void processMix(float* leftInSamples, float* rightInSamples,
		  float* leftOutSamples, float* rightOutSamples,
		  unsigned n);
  void processReplace(float* leftInSamples, float* rightInSamples,
		      float* leftOutSamples, float* rightOutSamples,
		      unsigned n);
};

#endif /* __PANDELAYMODEL_H */
