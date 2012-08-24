//===========================================================================
//
//    DeicsOnze an emulator of the YAMAHA DX11 synthesizer
//
//    Version 0.5.5
//
//    deicsonzefilter.h
//
//
//  Copyright (c) 2004-2006 Nil Geisweiller
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

#ifndef __DEICSONZEFILTER_H
#define __DEICSONZEFILTER_H

#include <math.h>

class LowFilter {
 private:
  int _samplerate;

  double _cutoff; //frequency cutoff
  float _a;
  float _b;

  float _li; //last left input sample
  float _ri; //last right input sample
  float _lo; //last left output sample
  float _ro; //last right output sample
 public:
  LowFilter();
  ~LowFilter() {}

  void setSamplerate(int sr);
  void setCutoff(double cut);
  //int getSamplerate();
  //double getCutoff();

  void process(float* leftSamples, float* RightSamples, unsigned n);
};

#endif /* __DEICSONZEFILTER_H */
