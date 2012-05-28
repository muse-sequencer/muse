//
// C++ Implementation: plugin
//
// Description:
//
//  (C) Copyright 2000 Werner Schweer (ws@seh.de)
//
// Additions/modifications: Mathias Lundgren <lunar_shuttle@users.sf.net>, (C) 2004
//                          (C) Copyright 2011 Tim E. Real (terminator356 at users.sourceforge.net)
//
//
//  This program is free software; you can redistribute it and/or
//  modify it under the terms of the GNU General Public License
//  as published by the Free Software Foundation; version 2 of
//  the License, or (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
//
//

#include <QtCore>
#include <QtGui>
#include <stdlib.h>
#include <unistd.h>
#include <dlfcn.h>
#include "ssplugin.h"
#include "common.h"

PluginList plugins;


Plugin::Plugin(const QFileInfo* f)
   : fi(*f)
      {
      }

//---------------------------------------------------------
//   loadPluginLib
//---------------------------------------------------------

static void loadPluginLib(QFileInfo* fi)
      {
      SS_TRACE_IN
      if (SS_DEBUG_LADSPA) {
            printf("loadPluginLib: %s\n", fi->fileName().toLatin1().constData());
            }
      void* handle = dlopen(fi->filePath().toAscii().data(), RTLD_NOW);
      if (handle == 0) {
            fprintf(stderr, "dlopen(%s) failed: %s\n",
              fi->filePath().toAscii().data(), dlerror());
            return;
            }
      LADSPA_Descriptor_Function ladspa = (LADSPA_Descriptor_Function)dlsym(handle, "ladspa_descriptor");

      if (!ladspa) {
            const char *txt = dlerror();
            if (txt) {
                  fprintf(stderr,
                        "Unable to find ladspa_descriptor() function in plugin "
                        "library file \"%s\": %s.\n"
                        "Are you sure this is a LADSPA plugin file?\n",
                        fi->filePath().toAscii().data(),
                        txt);
                  return;//exit(1);
                  }
            }
      const LADSPA_Descriptor* descr;
      for (int i = 0;; ++i) {
            descr = ladspa(i);
            if (descr == NULL)
                  break;
            plugins.push_back(new LadspaPlugin(fi, ladspa, descr));
            }
      SS_TRACE_OUT
      }

//---------------------------------------------------------
//   loadPluginDir
//---------------------------------------------------------

static void loadPluginDir(const QString& s)
      {
      SS_TRACE_IN
      QDir pluginDir(s, QString("*.so"), 0, QDir::Files);
      if (pluginDir.exists()) {
            QFileInfoList list = pluginDir.entryInfoList();
            int n = list.size();
            for (int i = 0; i < n; ++i) {
                  QFileInfo fi = list.at(i);
                  loadPluginLib(&fi);
                  }
            }
      SS_TRACE_OUT
      }

//---------------------------------------------------------
//   initPlugins
//    search for LADSPA plugins
//---------------------------------------------------------

void SS_initPlugins()
      {
      SS_TRACE_IN
      //loadPluginDir(museGlobalLib + QString("/plugins"));

      const char* ladspaPath = getenv("LADSPA_PATH");
      if (ladspaPath == 0)
            ladspaPath = "/usr/lib/ladspa:/usr/local/lib/ladspa:/usr/lib64/ladspa:/usr/local/lib64/ladspa";

      const char* p = ladspaPath;
      while (*p != '\0') {
            const char* pe = p;
            while (*pe != ':' && *pe != '\0')
                  pe++;

            int n = pe - p;
            if (n) {
                  char* buffer = new char[n + 1];
                  strncpy(buffer, p, n);
                  buffer[n] = '\0';
                  loadPluginDir(QString(buffer));
                  delete[] buffer;
                  }
            p = pe;
            if (*p == ':')
                  p++;
            }
      SS_TRACE_OUT
      }


//---------------------------------------------------------
//   LadspaPlugin
//---------------------------------------------------------

LadspaPlugin::LadspaPlugin(const QFileInfo* f,
   const LADSPA_Descriptor_Function ldf,
   const LADSPA_Descriptor* d)
   : Plugin(f), ladspa(ldf), plugin(d)
      {
      SS_TRACE_IN
      _inports        = 0;
      _outports       = 0;
      _parameter      = 0;
      handle          = 0;
      active          = false;
      controls        = 0;
      inputs          = 0;
      outputs         = 0;

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

      /*if (SS_DEBUG_LADSPA) {
            printf("Label: %s\tLib: %s\tPortCount: %d\n", this->label().toLatin1().constData(), this->lib().toLatin1().constData(), plugin->PortCount);
            printf("LADSPA_PORT_CONTROL|LADSPA_PORT_INPUT: %d\t", pIdx.size());
            printf("Input ports: %d\t", iIdx.size());
            printf("Output ports: %d\n\n", oIdx.size());
            }*/

      LADSPA_Properties properties = plugin->Properties;
      _inPlaceCapable = !LADSPA_IS_INPLACE_BROKEN(properties);
      if (_inports != _outports)
            _inPlaceCapable = false;
      SS_TRACE_OUT
      }

//---------------------------------------------------------
//   ~LadspaPlugin
//---------------------------------------------------------
LadspaPlugin::~LadspaPlugin()
      {
      SS_TRACE_IN
      if (active) {
            stop();
            }
      if (handle) {
         SS_DBG_LADSPA2("Cleaning up ", this->label().toLatin1().constData());
         plugin->cleanup(handle);
         }

      //Free ports:
      if (controls)
            delete controls;
      if (inputs)
            delete inputs;
      if (outputs)
            delete outputs;
      SS_TRACE_OUT
      }

//---------------------------------------------------------
//   instantiate
//---------------------------------------------------------

bool LadspaPlugin::instantiate()
      {
      bool success = false;
      handle = plugin->instantiate(plugin, SS_samplerate);
      success = (handle != NULL);
      if (success)
            SS_DBG_LADSPA2("Plugin instantiated", label().toLatin1().constData());
      return success;
      }

//---------------------------------------------------------
//   start
// activate and connect control ports
//---------------------------------------------------------

bool LadspaPlugin::start()
      {
      SS_TRACE_IN
      if (handle) {
            if (plugin->activate) {
                  plugin->activate(handle);
                  SS_DBG_LADSPA("Plugin activated");
                  }
            active = true;
            }
      else {
            SS_DBG_LADSPA("Error trying to activate plugin - plugin not instantiated!");
            SS_TRACE_OUT
            return false;
            }

      //Connect ports:
      controls = new Port[_parameter];

      for (int k = 0; k < _parameter; ++k) {
            double val = defaultValue(k);
            controls[k].val    = val;
            plugin->connect_port(handle, pIdx[k], &controls[k].val);
            }

      outputs  = new Port[_outports];
      inputs   = new Port[_inports];

      SS_TRACE_OUT
      return true;
      }

//---------------------------------------------------------
//   stop
// deactivate
//---------------------------------------------------------
void LadspaPlugin::stop()
      {
      SS_TRACE_IN
      if (handle) {
            SS_DBG_LADSPA2("Trying to stop plugin", label().toLatin1().constData());
            if (plugin->deactivate) {
                  SS_DBG_LADSPA2("Deactivating ", label().toLatin1().constData());
                  plugin->deactivate(handle);
                  active = false;
                  }
            }
      else
            SS_DBG_LADSPA("Warning - tried to stop plugin, but plugin was never started...\n");
      SS_TRACE_OUT
      }

//---------------------------------------------------------
//   range
//---------------------------------------------------------

void LadspaPlugin::range(int i, float* min, float* max) const
      {
      SS_TRACE_IN
      i = pIdx[i];
      LADSPA_PortRangeHint range = plugin->PortRangeHints[i];
      LADSPA_PortRangeHintDescriptor desc = range.HintDescriptor;
      if (desc & LADSPA_HINT_TOGGLED) {
            *min = 0.0;
            *max = 1.0;
            return;
            }
      float m = 1.0;
      if (desc & LADSPA_HINT_SAMPLE_RATE)
            m = (float) SS_samplerate;

      if (desc & LADSPA_HINT_BOUNDED_BELOW)
            *min =  range.LowerBound * m;
      else
            *min = 0.0;
      if (desc & LADSPA_HINT_BOUNDED_ABOVE)
            *max =  range.UpperBound * m;
      else
            *max = 1.0;
      SS_TRACE_OUT
      }

//---------------------------------------------------------
//   defaultValue
//---------------------------------------------------------

float LadspaPlugin::defaultValue(int k) const
      {
      SS_TRACE_IN
      k = pIdx[k];
      LADSPA_PortRangeHint range = plugin->PortRangeHints[k];
      LADSPA_PortRangeHintDescriptor rh = range.HintDescriptor;
      double val = 1.0;
      if (LADSPA_IS_HINT_DEFAULT_MINIMUM(rh))
            val = range.LowerBound;
      else if (LADSPA_IS_HINT_DEFAULT_MAXIMUM(rh))
            val = range.UpperBound;
      else if (LADSPA_IS_HINT_DEFAULT_LOW(rh))
            if (LADSPA_IS_HINT_LOGARITHMIC(range.HintDescriptor))
                  val = exp(log(range.LowerBound) * .75 +
                     log(range.UpperBound) * .25);
            else
                  val = range.LowerBound*.75 + range.UpperBound*.25;
      else if (LADSPA_IS_HINT_DEFAULT_MIDDLE(rh))
            if (LADSPA_IS_HINT_LOGARITHMIC(range.HintDescriptor))
                  val = exp(log(range.LowerBound) * .5 +
                     log(range.UpperBound) * .5);
            else
                  val = range.LowerBound*.5 + range.UpperBound*.5;
      else if (LADSPA_IS_HINT_DEFAULT_HIGH(rh))
            if (LADSPA_IS_HINT_LOGARITHMIC(range.HintDescriptor))
                  val = exp(log(range.LowerBound) * .25 +
                     log(range.UpperBound) * .75);
            else
                  val = range.LowerBound*.25 + range.UpperBound*.75;
      else if (LADSPA_IS_HINT_DEFAULT_0(rh))
            val = 0.0;
      else if (LADSPA_IS_HINT_DEFAULT_1(rh))
            val = 1.0;
      else if (LADSPA_IS_HINT_DEFAULT_100(rh))
            val = 100.0;
      else if (LADSPA_IS_HINT_DEFAULT_440(rh))
            val = 440.0;
        // No default found. Make one up...
      else if (LADSPA_IS_HINT_BOUNDED_BELOW(rh) && LADSPA_IS_HINT_BOUNDED_ABOVE(rh))
      {
        if (LADSPA_IS_HINT_LOGARITHMIC(rh))
          val = exp(log(range.LowerBound) * .5 +
                log(range.UpperBound) * .5);
        else
          val = range.LowerBound*.5 + range.UpperBound*.5;
      }
      else if (LADSPA_IS_HINT_BOUNDED_BELOW(rh))
          val = range.LowerBound;
      else if (LADSPA_IS_HINT_BOUNDED_ABOVE(rh))
      {
          // Hm. What to do here... Just try 0.0 or the upper bound if less than zero.
          //if(range.UpperBound > 0.0)
          //  val = 0.0;
          //else
          //  val = range.UpperBound;
          // Instead try this: Adopt an 'attenuator-like' policy, where upper is the default.
          val = range.UpperBound;
          return true;
      }
      
      SS_TRACE_OUT
      return val;
      }

//---------------------------------------------------------
//   find
//---------------------------------------------------------

Plugin* PluginList::find(const QString& file, const QString& name)
      {
      SS_TRACE_IN
      for (iPlugin i = begin(); i != end(); ++i) {
            if ((file == (*i)->lib()) && (name == (*i)->label())) {
                  SS_TRACE_OUT
                  return *i;
                  }
            }
      printf("Plugin <%s> not found\n", name.toLatin1().constData());
      SS_TRACE_OUT
      return 0;
      }

//---------------------------------------------------------
//   connectInport
//---------------------------------------------------------
void LadspaPlugin::connectInport(int k, LADSPA_Data* datalocation)
      {
      SS_TRACE_IN
      plugin->connect_port(handle, iIdx[k], datalocation);
      SS_TRACE_OUT
      }

//---------------------------------------------------------
//   connectOutport
//---------------------------------------------------------
void LadspaPlugin::connectOutport(int k, LADSPA_Data* datalocation)
      {
      SS_TRACE_IN
      plugin->connect_port(handle, oIdx[k], datalocation);
      SS_TRACE_OUT
      }

//---------------------------------------------------------
//   process
//---------------------------------------------------------
void LadspaPlugin::process(unsigned long frames)
      {
      plugin->run(handle, frames);
      }

//---------------------------------------------------------
//   setParam
//---------------------------------------------------------

void LadspaPlugin::setParam(int k, float val)
      {
      SS_TRACE_IN
      controls[k].val = val;
      SS_TRACE_OUT
      }

//---------------------------------------------------------
//   getGuiControlValue
//  scale control value to gui-slider/checkbox representation
//---------------------------------------------------------

int LadspaPlugin::getGuiControlValue(int param) const
      {
      SS_TRACE_IN
      float val = getControlValue(param);
      float min, max;
      range(param, &min, &max);
      int intval;
      if (isLog(param)) {
            intval = SS_map_logdomain2pluginparam(logf(val/(max - min) + min));
            }
      else if (isBool(param)) {
            intval = (int) val;
            }
      else {
            float scale = SS_PLUGIN_PARAM_MAX / (max - min);
            intval = (int) ((val - min) * scale);
            }
      SS_TRACE_OUT
      return intval;
      }

//---------------------------------------------------------
//   convertGuiControlValue
//  scale control value to gui-slider/checkbox representation
//---------------------------------------------------------

float LadspaPlugin::convertGuiControlValue(int parameter, int val) const
      {
      SS_TRACE_IN
      float floatval = 0;
      float min, max;
      range(parameter, &min, &max);

      if (isLog(parameter)) {
            if (val > 0) {
                  float logged = SS_map_pluginparam2logdomain(val);
                  float e = expf(logged) * (max - min);
                  e+=min;
                  floatval = e;
                  }
            }
      else if (isBool(parameter)) {
            floatval = (float) val;
            }
      else if (isInt(parameter)) {
            float scale = (max - min) / SS_PLUGIN_PARAM_MAX;
            floatval = (float) round((((float) val) * scale) + min);
            }
      else {
            float scale = (max - min) / SS_PLUGIN_PARAM_MAX;
            floatval = (((float) val) * scale) + min;
            }
      SS_TRACE_OUT
      return floatval;
      }
