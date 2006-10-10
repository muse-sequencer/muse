//=============================================================================
//  MusE
//  Linux Music Editor
//  $Id:$
//
//  Copyright (C) 2002-2006 by Werner Schweer and others
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

#include "al/al.h"
#include "ladspaplugin.h"
#include "fastlog.h"
#include "ctrl.h"

//---------------------------------------------------------
//   defaultValue
//---------------------------------------------------------

float ladspaDefaultValue(const LADSPA_Descriptor* plugin, int k)
      {
      LADSPA_PortRangeHint range = plugin->PortRangeHints[k];
      LADSPA_PortRangeHintDescriptor rh = range.HintDescriptor;
//      bool isLog = LADSPA_IS_HINT_LOGARITHMIC(rh);
      double val = 1.0;
      float m = (rh & LADSPA_HINT_SAMPLE_RATE) ? float(AL::sampleRate) : 1.0f;
      if (LADSPA_IS_HINT_DEFAULT_MINIMUM(rh)) {
            val = range.LowerBound * m;
            }
      else if (LADSPA_IS_HINT_DEFAULT_LOW(rh)) {
            if (LADSPA_IS_HINT_LOGARITHMIC(rh))
                  val = exp(fast_log10(range.LowerBound * m) * .75 +
                     log(range.UpperBound * m) * .25);
            else
                  val = range.LowerBound*.75*m + range.UpperBound*.25*m;
            }
      else if (LADSPA_IS_HINT_DEFAULT_MIDDLE(rh)) {
            if (LADSPA_IS_HINT_LOGARITHMIC(rh))
                  val = exp(log(range.LowerBound * m) * .5 +
                     log10(range.UpperBound * m) * .5);
            else
                  val = range.LowerBound*.5*m + range.UpperBound*.5*m;
            }
      else if (LADSPA_IS_HINT_DEFAULT_HIGH(rh)) {
            if (LADSPA_IS_HINT_LOGARITHMIC(rh))
                  val = exp(log(range.LowerBound * m) * .25 +
                     log(range.UpperBound * m) * .75);
            else
                  val = range.LowerBound*.25*m + range.UpperBound*.75*m;
            }
      else if (LADSPA_IS_HINT_DEFAULT_MAXIMUM(rh)) {
            val = range.UpperBound*m;
            }
      else if (LADSPA_IS_HINT_DEFAULT_0(rh))
            val = 0.0;
      else if (LADSPA_IS_HINT_DEFAULT_1(rh))
            val = 1.0;
      else if (LADSPA_IS_HINT_DEFAULT_100(rh))
            val = 100.0;
      else if (LADSPA_IS_HINT_DEFAULT_440(rh))
            val = 440.0;
      return val;
      }

//---------------------------------------------------------
//   LadpsaPlugin
//---------------------------------------------------------

LadspaPlugin::LadspaPlugin(const QFileInfo* f,
   const LADSPA_Descriptor_Function ldf,
   const LADSPA_Descriptor* d)
   : Plugin(f), ladspa(ldf), plugin(d)
      {
      _inports        = 0;
      _outports       = 0;
      _parameter      = 0;
      for (unsigned k = 0; k < plugin->PortCount; ++k) {
            LADSPA_PortDescriptor pd = d->PortDescriptors[k];
            static const int CI = LADSPA_PORT_CONTROL | LADSPA_PORT_INPUT;
            if ((pd &  CI) == CI) {
                  ++_parameter;
                  pIdx.push_back(k);
                  }
            else if (pd &  LADSPA_PORT_INPUT) {
                  ++_inports;
                  iIdx.push_back(k);
                  }
            else if (pd &  LADSPA_PORT_OUTPUT) {
                  ++_outports;
                  oIdx.push_back(k);
                  }
            }
      LADSPA_Properties properties = plugin->Properties;
      _inPlaceCapable = !LADSPA_IS_INPLACE_BROKEN(properties);
      if (_inports != _outports)
            _inPlaceCapable = false;
      }

//---------------------------------------------------------
//   instantiate
//---------------------------------------------------------

void* LadspaPlugin::instantiate()
      {
      return plugin->instantiate(plugin, AL::sampleRate);
      }

//---------------------------------------------------------
//   range
//---------------------------------------------------------

void LadspaPlugin::range(int i, double* min, double* max) const
      {
      i = pIdx[i];
      LADSPA_PortRangeHint range = plugin->PortRangeHints[i];
      LADSPA_PortRangeHintDescriptor desc = range.HintDescriptor;
      if (desc & LADSPA_HINT_TOGGLED) {
            *min = 0.0;
            *max = 1.0;
            return;
            }
      double m = (desc & LADSPA_HINT_SAMPLE_RATE) ? float(AL::sampleRate) : 1.0f;

      if (desc & LADSPA_HINT_BOUNDED_BELOW)
            *min =  range.LowerBound * m;
      else
            *min = 0.0;
      if (desc & LADSPA_HINT_BOUNDED_ABOVE)
            *max =  range.UpperBound * m;
      else
            *max = 1.0;
      }

//---------------------------------------------------------
//   createPIF
//---------------------------------------------------------

PluginIF* LadspaPlugin::createPIF(PluginI* pi)
      {
      LadspaPluginIF* pif = new LadspaPluginIF(pi);
      pif->init(pi->plugin());
      return pif;
      }

//---------------------------------------------------------
//   LadspaPluginIF
//---------------------------------------------------------

LadspaPluginIF::LadspaPluginIF(PluginI* pi)
   : PluginIF(pi)
      {
      descr  = 0;
      plugin = (LadspaPlugin*)(pi->plugin());
      }

//---------------------------------------------------------
//   init
//    return true on error
//---------------------------------------------------------

bool LadspaPluginIF::init(Plugin* pl)
      {
      handle = (LADSPA_Descriptor*) ((LadspaPlugin*)pl)->instantiate();
      plugin = (LadspaPlugin*)pl;
      descr  = plugin->ladspaDescriptor();

      int controlPorts = plugin->parameter();
      controls = new LadspaPort[controlPorts];

      for (int k = 0; k < controlPorts; ++k) {
            controls[k].val = plugin->defaultValue(k);
            descr->connect_port(handle, plugin->pIdx[k], &controls[k].val);
            }
      return handle == 0;
      }

//---------------------------------------------------------
//   apply
//---------------------------------------------------------

void LadspaPluginIF::apply(unsigned nframes, float** src, float** dst)
      {
      int iports = plugin->inports();
      int oports = plugin->outports();
      int cports = plugin->parameter();

      //
      // update parameter
      //
      for (int i = 0; i < cports; ++i)
            controls[i].val = pluginI->controllerList[i]->curVal().f;
      //
      // set connections
      //
      for (int k = 0; k < iports; ++k)
            descr->connect_port(handle, plugin->iIdx[k], src[k]);
      for (int k = 0; k < oports; ++k)
            descr->connect_port(handle, plugin->oIdx[k], dst[k]);

      descr->run(handle, nframes);
      }

//---------------------------------------------------------
//   defaultValue
//---------------------------------------------------------

double LadspaPlugin::defaultValue(int k) const
      {
      k = pIdx[k];
      return ladspaDefaultValue(plugin, k);
      }

//---------------------------------------------------------
//   activate
//---------------------------------------------------------

void LadspaPluginIF::activate()
      {
      //
      // TODO: init values?
      //
      if (descr->activate)
            descr->activate(handle);
      }

