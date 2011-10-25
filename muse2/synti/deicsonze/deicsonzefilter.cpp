//===========================================================================
//
//    DeicsOnze an emulator of the YAMAHA DX11 synthesizer
//
//    Version 0.5.5
//
//    deicsonzefilter.cpp
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

#include "deicsonzefilter.h"

LowFilter::LowFilter() {
  _li = 0.0;
  _ri = 0.0;
  _lo = 0.0;
  _ro = 0.0;
}

void LowFilter::setSamplerate(int sr) {
  _samplerate = sr;
}

void LowFilter::setCutoff(double cut) {
  _cutoff = cut;
  float w = 2.0 * (float)_samplerate;
  float fCut = _cutoff * 2.0 * M_PI;
  float norm = 1.0 / (fCut + w);
  _a = fCut * norm;
  _b = (w - fCut) * norm;
}

void LowFilter::process(float* leftSamples, float* rightSamples, unsigned n) {
  float cl, cr;
  for(unsigned i = 0; i < n; i++) {
    cl = leftSamples[i];
    cr = rightSamples[i];

    leftSamples[i] = _a * (cl + _li) + _b * _lo;
    rightSamples[i] = _a * (cr + _ri) + _b * _ro;

    _li = cl;
    _ri = cr;
    _lo = leftSamples[i];
    _ro = rightSamples[i];
  }
}

