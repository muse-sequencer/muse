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

#ifndef __DOUBLECHORUSMODEL_H
#define __DOUBLECHORUSMODEL_H

#include "simplechorusmodel.h"
#include "../../muse/ladspa.h"

#define NBRPARAM 7

class SimpleChorusModel;

class DoubleChorusModel {
  SimpleChorusModel* _simpleChorus1;
  SimpleChorusModel* _simpleChorus2;

  float _dryWet; //0.0 : dry, 1.0 : wet
  
 public:
  LADSPA_Data* port[NBRPARAM + 4];
  float param[NBRPARAM];
  
  DoubleChorusModel(unsigned long samplerate);
  ~DoubleChorusModel();
  void processMix(long numsamples);
  void processReplace(long numsamples);
  void setPan1(float value);
  void setLFOFreq1(float value);
  void setDepth1(float value);
  void setPan2(float value);
  void setLFOFreq2(float value);
  void setDepth2(float value);
  void setDryWet(float value);
  float	getPan1();
  float	getLFOFreq1();
  float	getDepth1();
  float	getPan2();
  float	getLFOFreq2();
  float	getDepth2();
  float getDryWet();

  void activate();
};

#endif
