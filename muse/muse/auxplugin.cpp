//=============================================================================
//  MusE
//  Linux Music Editor
//  $Id:$
//
//  Copyright (C) 2006 by Werner Schweer and others
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License version 2.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//=============================================================================

#include "globaldefs.h"
#include "auxplugin.h"

AuxPlugin* auxPlugin;

//---------------------------------------------------------
//   AuxPlugin
//---------------------------------------------------------

AuxPlugin::AuxPlugin()
   : Plugin(0)
      {
      }

//---------------------------------------------------------
//   range
//---------------------------------------------------------

void AuxPlugin::range(int idx, double* min, double* max) const
      {
      switch(idx) {
            case 0:           // volume
                  *min = -60.0;
                  *max = 10.0;
                  break;
            case 1:           // pan
                  *min = -1.0;
                  *max = 1.0;
                  break;
            default:
                  printf("AuxPlugin::bad index\n");
                  break;
            }
      }

//---------------------------------------------------------
//   createPIF
//---------------------------------------------------------

PluginIF* AuxPlugin::createPIF(PluginI* pi)
      {
      AuxPluginIF* pif = new AuxPluginIF(pi);
      pif->init(pi->plugin());
      return pif;
      }

//---------------------------------------------------------
//   isLog
//---------------------------------------------------------

bool AuxPlugin::isLog(int idx) const
      {
      return idx == 0 ? true : false;
      }

//---------------------------------------------------------
//   isBool
//---------------------------------------------------------

bool AuxPlugin::isBool(int) const
      {
      return false;
      }

//---------------------------------------------------------
//   isInt
//---------------------------------------------------------

bool AuxPlugin::isInt(int) const
      {
      return false;
      }

//---------------------------------------------------------
//   defaultValue
//---------------------------------------------------------

double AuxPlugin::defaultValue(int idx) const
      {
      return idx == 0 ? -70.0 : 0.0;
      }

//---------------------------------------------------------
//   AuxPluginIF
//---------------------------------------------------------

AuxPluginIF::AuxPluginIF(PluginI* pi)
   : PluginIF(pi)
      {
      buffer = new float*[MAX_CHANNELS];
      for (int i = 0; i < MAX_CHANNELS; ++i)
            buffer[i] = new float[segmentSize];
      }

//---------------------------------------------------------
//   AuxPluginIF
//---------------------------------------------------------

AuxPluginIF::~AuxPluginIF()
      {
      for (int i = 0; i < MAX_CHANNELS; ++i) {
            if (buffer[i])
            	delete[] buffer[i];
            }
      delete[] buffer;
      }

//---------------------------------------------------------
//   apply
//---------------------------------------------------------

void AuxPluginIF::apply(unsigned nframes, float** s, float** /*dst*/)
      {
      // TODO: optimize copy away if there is no route
      double vol[2];
      vol[0] = volume * (1.0 - pan);
      vol[1] = volume * (1.0 + pan);

      for (int i = 0; i < pluginI->channel(); ++i) {
            float* dst = buffer[i];
            float* src = s[i];
            double v   = vol[i];
            for (int k = 0; k < nframes; ++k)
                  *dst++ = (*src++) * v;
            }
      }

//---------------------------------------------------------
//   getParameterName
//---------------------------------------------------------

const char* AuxPluginIF::getParameterName(int i) const
      {
      if (i == 0)
            return "Volume";
      else
            return "Pan";
      }

//---------------------------------------------------------
//   setParam
//---------------------------------------------------------

void AuxPluginIF::setParam(int i, double val)
      {
      if (i == 0)
            volume = val;
      else
            pan = val;
      }

//---------------------------------------------------------
//   param
//---------------------------------------------------------

float AuxPluginIF::param(int i) const
      {
      return i == 0 ? volume : pan;
      }

//---------------------------------------------------------
//   init
//---------------------------------------------------------

bool AuxPluginIF::init(Plugin* p)
      {
      volume = p->defaultValue(0);
      pan    = p->defaultValue(1);
      return true;
      }

