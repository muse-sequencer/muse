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
#include <QtWidgets>
#include <stdlib.h>
#include <unistd.h>
#include <string>
#include "ssplugin.h"
#include "common.h"

#define SS_LOG_MAX   0
#define SS_LOG_MIN -10
#define SS_LOG_OFFSET SS_LOG_MIN

// Turn on debugging messages.
//#define PLUGIN_DEBUGIN

//
// Map plugin parameter on domain [SS_PLUGIN_PARAM_MIN, SS_PLUGIN_PARAM_MAX] to domain [SS_LOG_MIN, SS_LOG_MAX] (log domain)
//
float SS_map_pluginparam2logdomain(int pluginparam_val)
{
   float scale = (float) (SS_LOG_MAX - SS_LOG_MIN)/ (float) SS_PLUGIN_PARAM_MAX;
   float scaled = (float) pluginparam_val * scale;
   float mapped = scaled + SS_LOG_OFFSET;
   return mapped;
}
//
// Map plugin parameter on domain to domain [SS_LOG_MIN, SS_LOG_MAX] to [SS_PLUGIN_PARAM_MIN, SS_PLUGIN_PARAM_MAX]  (from log-> [0,127])
// (inverse func to the above)
int SS_map_logdomain2pluginparam(float pluginparam_log)
{
   float mapped = pluginparam_log - SS_LOG_OFFSET;
   float scale = (float) SS_PLUGIN_PARAM_MAX / (float) (SS_LOG_MAX - SS_LOG_MIN);
   int scaled  = (int) round(mapped * scale);
   return scaled;
}

PluginList plugins;


Plugin::Plugin(const QFileInfo* f)
   : fi(*f)
      {
  _references = 0;
  _uniqueID = 0;
      }

bool Plugin::reference() { return false; }
int Plugin::release() { return _references; }

unsigned long Plugin::id() const                  { return _uniqueID; }
QString Plugin::uri() const                       { return _uri; }
QString Plugin::name() const                      { return _name; }
QString Plugin::maker() const                     { return _maker; }
QString Plugin::description() const               { return _description; }
QString Plugin::version() const                   { return _version; }
QString Plugin::label() const                     { return _label; }
QString Plugin::copyright() const                 { return _copyright; }

//---------------------------------------------------------
//   loadPluginLib
//---------------------------------------------------------

static void loadPluginLib(QFileInfo* fi)
      {
      SS_TRACE_IN
      if (SS_DEBUG_LADSPA) {
            printf("loadPluginLib: %s\n", fi->fileName().toLocal8Bit().constData());
            }

      QLibrary qlib;
      qlib.setFileName(fi->filePath());
      // Same as dlopen RTLD_NOW.
      qlib.setLoadHints(QLibrary::ResolveAllSymbolsHint);
      if(!qlib.load())
      {
        fprintf(stderr, "ssplugin: loadPluginLib: load (%s) failed: %s\n",
          qlib.fileName().toLocal8Bit().constData(), qlib.errorString().toLocal8Bit().constData());
        return;
      }

      LADSPA_Descriptor_Function ladspa = (LADSPA_Descriptor_Function)qlib.resolve("ladspa_descriptor");
      if (!ladspa) {
            qlib.unload();
            QString txt = qlib.errorString();
                  fprintf(stderr,
                        "Unable to find ladspa_descriptor() function in plugin "
                        "library file \"%s\": %s.\n"
                        "Are you sure this is a LADSPA plugin file?\n",
                        fi->filePath().toLocal8Bit().constData(),
                        txt.toLocal8Bit().constData());
                  return;//exit(1);
            }

      const LADSPA_Descriptor* descr;
      for (int i = 0;; ++i) {
            descr = ladspa(i);
            if (descr == nullptr)
                  break;
            plugins.push_back(new LadspaPlugin(fi, ladspa, descr));
            }
      qlib.unload();
      SS_TRACE_OUT
      }

//---------------------------------------------------------
//   loadPluginDir
//---------------------------------------------------------

static void loadPluginDir(const QString& s)
      {
      SS_TRACE_IN
      QDir pluginDir(s, QString(), QDir::Name, QDir::Files);
      if (pluginDir.exists()) {
            QFileInfoList list = pluginDir.entryInfoList();
            int n = list.size();
            for (int i = 0; i < n; ++i) {
                  QFileInfo fi = list.at(i);
                  if(!QLibrary::isLibrary(fi.filePath()))
                    continue;
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

      std::string s;
      const char* ladspaPath = getenv("LADSPA_PATH");
      if (ladspaPath == 0)
      {
          const char* home = getenv("HOME");
          s = std::string(home) + std::string("/ladspa:/usr/local/lib64/ladspa:/usr/lib64/ladspa:/usr/local/lib/ladspa:/usr/lib/ladspa");
          ladspaPath = s.c_str();
      }
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
   const LADSPA_Descriptor_Function /*ldf*/,
   const LADSPA_Descriptor* d)
   : Plugin(f)
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
      ladspa          = nullptr;
      plugin          = nullptr;

      _uniqueID = d->UniqueID;
      _name = QString(d->Name);
      _label = QString(d->Label);
      _maker = QString(d->Maker);
      _copyright = QString(d->Copyright);

      for (unsigned k = 0; k < d->PortCount; ++k) {
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
            printf("Label: %s\tLib: %s\tPortCount: %d\n", this->label().toLocal8Bit().constData(), this->lib().toLocal8Bit().constData(), plugin->PortCount);
            printf("LADSPA_PORT_CONTROL|LADSPA_PORT_INPUT: %d\t", pIdx.size());
            printf("Input ports: %d\t", iIdx.size());
            printf("Output ports: %d\n\n", oIdx.size());
            }*/

      LADSPA_Properties properties = d->Properties;
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
         SS_DBG_LADSPA2("Cleaning up ", this->label().toLocal8Bit().constData());
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
//   reference
//---------------------------------------------------------
bool LadspaPlugin::reference()
{
      if (_references == 0)
      {
        _qlib.setFileName(fi.filePath());
        // Same as dlopen RTLD_NOW.
        _qlib.setLoadHints(QLibrary::ResolveAllSymbolsHint);
        if(!_qlib.load())
        {
          fprintf(stderr, "LadspaPlugin::reference(): load (%s) failed: %s\n",
            _qlib.fileName().toLocal8Bit().constData(), _qlib.errorString().toLocal8Bit().constData());
          return false;
        }

        LADSPA_Descriptor_Function ladspadf = (LADSPA_Descriptor_Function)_qlib.resolve("ladspa_descriptor");
        if(ladspadf)
        {
          const LADSPA_Descriptor* descr;
          for(unsigned long i = 0;; ++i)
          {
            descr = ladspadf(i);
            if(descr == nullptr)
              break;

            QString label(descr->Label);
            if(label == _label)
            {
              ladspa = ladspadf;
              plugin = descr;
              break;
            }
          }
        }
      }

      ++_references;

      if(plugin == nullptr)
      {
        fprintf(stderr, "LadspaPlugin::reference() Error: cannot find LADSPA plugin %s\n", label().toLocal8Bit().constData());
        release();
        return false;
      }

      return true;
}

//---------------------------------------------------------
//   release
//---------------------------------------------------------
int LadspaPlugin::release()
{
  #ifdef PLUGIN_DEBUGIN
  fprintf(stderr, "LadspaPlugin::release() references:%d\n", _references);
  #endif

  if(_references == 1)
  {
    // Attempt to unload the library.
    // It will remain loaded if the plugin has shell plugins still in use or there are other references.
    const bool ulres = _qlib.unload();
    // Dummy usage stops unused warnings.
    (void)ulres;
    #ifdef PLUGIN_DEBUGIN
    fprintf(stderr, "LadspaPlugin::release(): No more instances. Result of unloading library %s: %d\n",
      _qlib.fileName().toLocal8Bit().constData(), ulres);
    #endif

    ladspa = nullptr;
    plugin = nullptr;
    //pIdx.clear();
    //iIdx.clear();
    //oIdx.clear();
  }
  if(_references > 0)
    --_references;
  return _references;
}

//---------------------------------------------------------
//   instantiate
//---------------------------------------------------------

bool LadspaPlugin::instantiate()
      {
      if(!reference())
        return false;

      handle = plugin->instantiate(plugin, SS_samplerate);
      if(handle == nullptr)
      {
        fprintf(stderr, "LadspaPlugin::instantiate() Error: plugin:%s instantiate failed!\n", label().toLocal8Bit().constData());
        release();
        return false;
      }

      SS_DBG_LADSPA2("Plugin instantiated", label().toLocal8Bit().constData());
      return true;
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
            SS_DBG_LADSPA2("Trying to stop plugin", label().toLocal8Bit().constData());
            if (plugin->deactivate) {
                  SS_DBG_LADSPA2("Deactivating ", label().toLocal8Bit().constData());
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
      printf("Plugin <%s> not found\n", name.toLocal8Bit().constData());
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
