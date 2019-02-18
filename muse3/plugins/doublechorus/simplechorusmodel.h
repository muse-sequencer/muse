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

#ifndef __SIMPLECHORUSMODEL_H
#define __SIMPLECHORUSMODEL_H

#define MAXBUFFERLENGTH 192000
#define MAXSINUSRESOLUTION 192000
#define MINFREQ 0.05 //in Hz
#define MAXFREQ 5.0 //in Hz
#define EARSDISTANCE 0.12 //in meter
#define MIDSOURCEDISTANCE 2.0 //in meter
#define MAXDEPTH 1.0 //in meter, radius
#define SOUNDSPEED 330.0 //in meter per second
#define MINDELAYSEC 0.01 //in second
#define MAXDELAYSEC 1.0 //in second
#define COEFFILTER 0.97576 //0.26795
#define PANAMP 0.75
//with cutoff = samplerate/256
//following (2-cos(x)) - sqrt((2-cos(x))^2 - 1) with x = 2*pi*cutoff/samplerate
//#define M_PI 3.14159265358979

class SimpleChorusModel {
 private :
  //parameters
  float _pan;
  float _LFOFreq;
  float _depth;
  //parameter state
  float _sampleRate;
  float _depthAmp; 
  float _leftAmp;
  float _rightAmp;
  float _filterCoef1;
  float _filterCoef2;
  int _leftMidDistance; //distance of the left micro in samples
  int _rightMidDistance; //distance of the right micro in samples
  //state
  float _inct;
  float _index; //time at the scale of sampleRate
  float _leftBuffer[MAXBUFFERLENGTH];
  float _rightBuffer[MAXBUFFERLENGTH];
  float _ocsDistance; //in sample, distance of the micro with initial position 
  int _past_position_left;
  int _past_position_right;
  int _position;
 public :
  static int useCount;
  static float sinus[MAXSINUSRESOLUTION];


  void process_chorus(float leftInput, float rightInput,
		      float* leftOutput, float* rightOutput);

  void setPan(float);
  void setLFOFreq(float);
  void setDepth(float);
  void setSampleRate(float);
  float getPan();
  float getLFOFreq();
  float getDepth();

  void setChorus();

  SimpleChorusModel(float samplerate);
  ~SimpleChorusModel();
  
};

#endif
