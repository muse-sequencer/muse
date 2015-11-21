//===========================================================================
//
//    doublechorusmodel
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

#include "doublechorusmodel.h"

//---------------------------------------------------------
// DoubleChorusModel
//---------------------------------------------------------

DoubleChorusModel::DoubleChorusModel(unsigned long samplerate) {
  _simpleChorus1 = new SimpleChorusModel((float)samplerate);
  _simpleChorus2 = new SimpleChorusModel((float)samplerate);

  param[0] = getPan1();
  param[1] = getLFOFreq1();
  param[2] = getDepth1();
  param[3] = getPan2();
  param[4] = getLFOFreq2();
  param[5] = getDepth2();
  param[6] = getDryWet();
}

DoubleChorusModel::~DoubleChorusModel() {
  delete(_simpleChorus1);
  delete(_simpleChorus2);
}

//---------------------------------------------------------
//   activate
//---------------------------------------------------------

void DoubleChorusModel::activate() {
  *port[4] = param[0];
  *port[5] = param[1];
  *port[6] = param[2];
  *port[7] = param[3];
  *port[8] = param[4];
  *port[9] = param[5];
  *port[10] = param[6];
}

//---------------------------------------------------------
//   processReplace
//---------------------------------------------------------

void DoubleChorusModel::processReplace(long n) {
  float tmpLeftOutput1;
  float tmpRightOutput1;
  float tmpLeftOutput2;
  float tmpRightOutput2;
  //update parameters
  if (param[0] != *port[4]) {
    param[0] = *port[4];
    setPan1(param[0]);
  }
  if (param[1] != *port[5]) {
    param[1] = *port[5];
    setLFOFreq1(param[1]);
  }
  if (param[2] != *port[6]) {
    param[2] = *port[6];
    setDepth1(param[2]);
  }
  if (param[3] != *port[7]) {
    param[3] = *port[7];
    setPan2(param[3]);
  }
  if (param[4] != *port[8]) {
    param[4] = *port[8];
    setLFOFreq2(param[4]);
  }
  if (param[5] != *port[9]) {
    param[5] = *port[9];
    setDepth2(param[5]);
  }
  if (param[6] != *port[10]) {
    param[6] = *port[10];
    setDryWet(param[6]);
  }
  //process the effect
  for (int i = 0; i < n; ++i) {
    _simpleChorus1->process_chorus(port[0][i], port[1][i],
				   &tmpLeftOutput1, &tmpRightOutput1);  
    _simpleChorus2->process_chorus(port[0][i], port[1][i],
				   &tmpLeftOutput2, &tmpRightOutput2);
    port[2][i] = _dryWet * (tmpLeftOutput1 + tmpLeftOutput2)
      + (1.0 - _dryWet) * port[0][i];
    port[3][i] = _dryWet * (tmpRightOutput1 + tmpRightOutput2)
      + (1.0 - _dryWet) * port[1][i];
  }
}

void DoubleChorusModel::processMix(long n) {
  float tmpLeftOutput1;
  float tmpRightOutput1;
  float tmpLeftOutput2;
  float tmpRightOutput2;
  //update parameters
  if (param[0] != *port[4]) {
    param[0] = *port[4];
    setPan1(param[0]);
  }
  if (param[1] != *port[5]) {
    param[1] = *port[5];
    setLFOFreq1(param[1]);
  }
  if (param[2] != *port[6]) {
    param[2] = *port[6];
    setDepth1(param[2]);
  }
  if (param[3] != *port[7]) {
    param[3] = *port[7];
    setPan2(param[3]);
  }
  if (param[4] != *port[8]) {
    param[4] = *port[8];
    setLFOFreq2(param[4]);
  }
  if (param[5] != *port[9]) {
    param[5] = *port[9];
    setDepth2(param[5]);
  }
  if (param[6] != *port[10]) {
    param[6] = *port[10];
    setDryWet(param[6]);
  }
  //process the effect
  for (int i = 0; i < n; ++i) {
    _simpleChorus1->process_chorus(port[0][i], port[1][i],
				   &tmpLeftOutput1, &tmpRightOutput1);  
    _simpleChorus2->process_chorus(port[0][i], port[1][i],
				   &tmpLeftOutput2, &tmpRightOutput2);
    port[2][i] += _dryWet * (tmpLeftOutput1 + tmpLeftOutput2)
      + (1.0 - _dryWet) * port[0][i];
    port[3][i] += _dryWet * (tmpRightOutput1 + tmpRightOutput2)
      + (1.0 - _dryWet) * port[1][i];
  }
}

//------------------------------------------------------------------
// set parameters
//------------------------------------------------------------------
void DoubleChorusModel::setPan1(float value) {
  _simpleChorus1->setPan(value);
}
void DoubleChorusModel::setLFOFreq1(float value) {
  _simpleChorus1->setLFOFreq(value);
}
void DoubleChorusModel::setDepth1(float value) {
  _simpleChorus1->setDepth(value);
}
void DoubleChorusModel::setPan2(float value) {
  _simpleChorus2->setPan(value);
}
void DoubleChorusModel::setLFOFreq2(float value) {
  _simpleChorus2->setLFOFreq(value);
}
void DoubleChorusModel::setDepth2(float value) {
  _simpleChorus2->setDepth(value);
}
void DoubleChorusModel::setDryWet(float value) {
  _dryWet = value;
}

//----------------------------------------------------------------
// get parameters
//----------------------------------------------------------------
float DoubleChorusModel::getPan1() {
  return _simpleChorus1->getPan();
}
float DoubleChorusModel::getLFOFreq1() {
  return _simpleChorus1->getLFOFreq();
}
float DoubleChorusModel::getDepth1() {
  return _simpleChorus1->getDepth();
}
float DoubleChorusModel::getPan2() {
  return _simpleChorus2->getPan();
}
float DoubleChorusModel::getLFOFreq2() {
  return _simpleChorus2->getLFOFreq();
}
float DoubleChorusModel::getDepth2() {
  return _simpleChorus2->getDepth();
}
float DoubleChorusModel::getDryWet() {
  return _dryWet;
}
