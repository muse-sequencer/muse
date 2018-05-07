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
#include <stdio.h>
#include <unistd.h>
#include <dlfcn.h>
#include <string>
#include "simpler_plugin.h"

#define SS_LOG_MAX   0
#define SS_LOG_MIN -10
#define SS_LOG_OFFSET SS_LOG_MIN


#define SP_TRACE_FUNC   0
#define SP_DEBUG_LADSPA 0   

#define SP_TRACE_IN if (SP_TRACE_FUNC) fprintf (stderr, "->%s:%d\n", __PRETTY_FUNCTION__, __LINE__);
#define SP_TRACE_OUT if (SP_TRACE_FUNC) fprintf (stderr, "<-%s:%d\n", __PRETTY_FUNCTION__, __LINE__);
#define SP_ERROR(string) fprintf(stderr, "SimplePlugin error: %s\n", string)
#define SP_DBG_LADSPA(string1) if (SP_DEBUG_LADSPA) fprintf(stderr, "%s:%d:%s: %s\n", __FILE__ , __LINE__ , __PRETTY_FUNCTION__, string1);
#define SP_DBG_LADSPA2(string1, string2) if (SP_DEBUG_LADSPA) fprintf(stderr, "%s:%d:%s: %s: %s\n", __FILE__ , __LINE__ , __PRETTY_FUNCTION__, string1, string2);

// Turn on debugging messages.
//#define PLUGIN_DEBUGIN

// Turn on constant stream of debugging messages.
//#define PLUGIN_DEBUGIN_PROCESS

namespace MusESimplePlugin {

PluginList plugins;
  
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

//---------------------------------------------------------
//   loadPluginLib
//---------------------------------------------------------

static void loadPluginLib(QFileInfo* fi)
      {
      SP_TRACE_IN
      if (SP_DEBUG_LADSPA) {
            fprintf(stderr, "loadPluginLib: %s\n", fi->fileName().toLatin1().constData());
            }
      void* handle = dlopen(fi->filePath().toLatin1().constData(), RTLD_NOW);
      if (handle == 0) {
            fprintf(stderr, "dlopen(%s) failed: %s\n",
              fi->filePath().toLatin1().constData(), dlerror());
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
                        fi->filePath().toLatin1().constData(),
                        txt);
                  return;//exit(1);
                  }
            }
      const LADSPA_Descriptor* descr;
      for (int i = 0;; ++i) {
            descr = ladspa(i);
            if (descr == NULL)
                  break;
            
            // Make sure it doesn't already exist.
            if(plugins.find(fi->completeBaseName(), QString(descr->Label)) != 0)
              continue;

            plugins.push_back(new LadspaPlugin(fi, ladspa, descr));
            }
      SP_TRACE_OUT
      }

//---------------------------------------------------------
//   loadPluginDir
//---------------------------------------------------------

static void loadPluginDir(const QString& s)
      {
      SP_TRACE_IN
      QDir pluginDir(s, QString("*.so"), 0, QDir::Files);
      if (pluginDir.exists()) {
            QFileInfoList list = pluginDir.entryInfoList();
            int n = list.size();
            for (int i = 0; i < n; ++i) {
                  QFileInfo fi = list.at(i);
                  loadPluginLib(&fi);
                  }
            }
      SP_TRACE_OUT
      }

//---------------------------------------------------------
//   initPlugins
//    search for LADSPA plugins
//---------------------------------------------------------

void SS_initPlugins(const QString& globalLibPath)
      {
      SP_TRACE_IN

      loadPluginDir(globalLibPath + QString("/plugins"));
      
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
      SP_TRACE_OUT
      }


//---------------------------------------------------------
//   find
//---------------------------------------------------------

Plugin* PluginList::find(const QString& file, const QString& name)
      {
      SP_TRACE_IN
      for (iPlugin i = begin(); i != end(); ++i) {
            if ((file == (*i)->lib()) && (name == (*i)->label())) {
                  SP_TRACE_OUT
                  return *i;
                  }
            }
      //fprintf(stderr, "Plugin <%s> not found\n", name.toLatin1().constData());
      SP_TRACE_OUT
      return 0;
      }

PluginList::~PluginList()
{
   //fprintf(stderr, "~PluginList\n");
   //Cleanup plugins:
   for (iPlugin i = plugins.begin(); i != plugins.end(); ++i)
   {
     if((*i)->references() != 0)
     {
       fprintf(stderr, "~PluginList: Plugin <%s> reference count not zero! Cannot delete.\n",
              (*i)->name().toLatin1().constData());
       continue;
     }
     //fprintf(stderr, "~PluginList: deleting plugin <%s>\n",
     //       (*i)->name().toLatin1().constData());
     delete (*i);
   }
}

//---------------------------------------------------------
//   LadspaPlugin
//---------------------------------------------------------

LadspaPlugin::LadspaPlugin(const QFileInfo* f,
   const LADSPA_Descriptor_Function ldf,
   const LADSPA_Descriptor* d)
   : Plugin(f), ladspa(ldf), plugin(d)
      {
      SP_TRACE_IN
      
      _label = QString(d->Label);
      _name = QString(d->Name);
      _uniqueID = d->UniqueID;
      _maker = QString(d->Maker);
      _copyright = QString(d->Copyright);
      _portCount = d->PortCount;

      for(unsigned long k = 0; k < _portCount; ++k)
      {
        LADSPA_PortDescriptor pd = d->PortDescriptors[k];
        if(pd & LADSPA_PORT_AUDIO)
        {
          if(pd & LADSPA_PORT_INPUT)
          {
            ++_inports;
            iIdx.push_back(k);
          }
          else
          if(pd & LADSPA_PORT_OUTPUT)
          {
            ++_outports;
            oIdx.push_back(k);
          }
        }
        else
        if(pd & LADSPA_PORT_CONTROL)
        {
          if(pd & LADSPA_PORT_INPUT)
          {
            ++_controlInPorts;
            pIdx.push_back(k);
          }
          else
          if(pd & LADSPA_PORT_OUTPUT)
          {
            ++_controlOutPorts;
            poIdx.push_back(k);
          }
        }
      }
  
      /*if (SP_DEBUG_LADSPA) {
            printf("Label: %s\tLib: %s\tPortCount: %d\n", this->label().toLatin1().constData(), this->lib().toLatin1().constData(), plugin->PortCount);
            printf("LADSPA_PORT_CONTROL|LADSPA_PORT_INPUT: %d\t", pIdx.size());
            printf("Input ports: %d\t", iIdx.size());
            printf("Output ports: %d\n\n", oIdx.size());
            }*/

      if ((_inports != _outports) || (LADSPA_IS_INPLACE_BROKEN(plugin->Properties)))
        _requiredFeatures |= NoInPlaceProcessing;
      
      SP_TRACE_OUT
      }

//---------------------------------------------------------
//   incReferences
//---------------------------------------------------------

int LadspaPlugin::incReferences(int val)
{
  #ifdef PLUGIN_DEBUGIN
  fprintf(stderr, "Plugin::incReferences _references:%d val:%d\n", _references, val);
  #endif

  int newref = _references + val;

  if(newref == 0)
  {
    _references = 0;
    if(_libHandle)
    {
      #ifdef PLUGIN_DEBUGIN
      fprintf(stderr, "Plugin::incReferences no more instances, closing library\n");
      #endif

      dlclose(_libHandle);
    }

    _libHandle = 0;
    ladspa = NULL;
    plugin = NULL;
    pIdx.clear();
    poIdx.clear();
    iIdx.clear();
    oIdx.clear();

    return 0;
  }

  if(_libHandle == 0)
  {
    _libHandle = dlopen(fi.filePath().toLatin1().constData(), RTLD_NOW);

    if(_libHandle == 0)
    {
      fprintf(stderr, "Plugin::incReferences dlopen(%s) failed: %s\n",
              fi.filePath().toLatin1().constData(), dlerror());
      return 0;
    }

    LADSPA_Descriptor_Function ladspadf = (LADSPA_Descriptor_Function)dlsym(_libHandle, "ladspa_descriptor");
    if(ladspadf)
    {
      const LADSPA_Descriptor* descr;
      for(unsigned long i = 0;; ++i)
      {
        descr = ladspadf(i);
        if(descr == NULL)
          break;

        QString desc_label(descr->Label);
        if(desc_label == label())
        {
          ladspa = ladspadf;
          plugin = descr;

          break;
        }
      }
    }

    if(plugin != NULL)
    {
      _name = QString(plugin->Name);
      _uniqueID = plugin->UniqueID;
      _maker = QString(plugin->Maker);
      _copyright = QString(plugin->Copyright);

      _portCount = plugin->PortCount;

      _inports = 0;
      _outports = 0;
      _controlInPorts = 0;
      _controlOutPorts = 0;
      
      for(unsigned long k = 0; k < _portCount; ++k)
      {
        LADSPA_PortDescriptor pd = plugin->PortDescriptors[k];
        if(pd & LADSPA_PORT_AUDIO)
        {
          if(pd & LADSPA_PORT_INPUT)
          {
            ++_inports;
            iIdx.push_back(k);
          }
          else
          if(pd & LADSPA_PORT_OUTPUT)
          {
            ++_outports;
            oIdx.push_back(k);
          }
        }
        else
        if(pd & LADSPA_PORT_CONTROL)
        {
          if(pd & LADSPA_PORT_INPUT)
          {
            ++_controlInPorts;
            pIdx.push_back(k);
          }
          else
          if(pd & LADSPA_PORT_OUTPUT)
          {
            ++_controlOutPorts;
            poIdx.push_back(k);
          }
        }
      }
    }
  }

  if(plugin == NULL)
  {
    dlclose(_libHandle);
        _libHandle = 0;
    _references = 0;
    fprintf(stderr, "Plugin::incReferences Error: %s no plugin!\n", fi.filePath().toLatin1().constData());
    return 0;
  }

  if ((_inports != _outports) || (LADSPA_IS_INPLACE_BROKEN(plugin->Properties)))
    _requiredFeatures |= NoInPlaceProcessing;
  
  _references = newref;

  return _references;
}

//---------------------------------------------------------
//   instantiate
//---------------------------------------------------------

void* LadspaPlugin::instantiate(int sampleRate)
{
  bool success = false;
  LADSPA_Handle h = plugin->instantiate(plugin, sampleRate);
  success = (h != NULL);
  if (success)
        SP_DBG_LADSPA2("Plugin instantiated", label().toLatin1().constData());
  return h;
}

//---------------------------------------------------------
//   range
//---------------------------------------------------------

void LadspaPlugin::port_range(unsigned long i, int sampleRate, float* min, float* max) const
      {
      LADSPA_PortRangeHint range = plugin->PortRangeHints[i];
      LADSPA_PortRangeHintDescriptor desc = range.HintDescriptor;
      if (desc & LADSPA_HINT_TOGGLED) {
            *min = 0.0;
            *max = 1.0;
            return;
            }
      float m = 1.0;
      if (desc & LADSPA_HINT_SAMPLE_RATE)
            m = (float) sampleRate;

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
//   range
//---------------------------------------------------------

void LadspaPlugin::range(unsigned long i, int sampleRate, float* min, float* max) const
      {
      SP_TRACE_IN
      i = pIdx[i];
      port_range(i, sampleRate, min, max);
      SP_TRACE_OUT
      }

//---------------------------------------------------------
//   range
//---------------------------------------------------------

void LadspaPlugin::rangeOut(unsigned long i, int sampleRate, float* min, float* max) const
      {
      SP_TRACE_IN
      i = poIdx[i];
      port_range(i, sampleRate, min, max);
      SP_TRACE_OUT
      }

//---------------------------------------------------------
//   defaultValue
//---------------------------------------------------------

float LadspaPlugin::defaultValue(unsigned long k) const
      {
      SP_TRACE_IN
      k = pIdx[k];
      LADSPA_PortRangeHint range = plugin->PortRangeHints[k];
      LADSPA_PortRangeHintDescriptor rh = range.HintDescriptor;
      LADSPA_Data val = 1.0;
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
      
      SP_TRACE_OUT
      return val;
      }

//---------------------------------------------------------
//   connectInport
//---------------------------------------------------------
void LadspaPlugin::connectInport(void* handle, unsigned long k, void* datalocation)
      {
      SP_TRACE_IN
      if(!plugin)
        return;
      plugin->connect_port((LADSPA_Handle)handle, iIdx[k], (LADSPA_Data*)datalocation);
      SP_TRACE_OUT
      }

//---------------------------------------------------------
//   connectOutport
//---------------------------------------------------------
void LadspaPlugin::connectOutport(void* handle, unsigned long k, void* datalocation)
      {
      SP_TRACE_IN
      if(!plugin)
        return;
      plugin->connect_port((LADSPA_Handle)handle, oIdx[k], (LADSPA_Data*)datalocation);
      SP_TRACE_OUT
      }

//---------------------------------------------------------
//   connectCtrlInport
//---------------------------------------------------------
void LadspaPlugin::connectCtrlInport(void* handle, unsigned long k, void* datalocation)
      {
      SP_TRACE_IN
      if(!plugin)
        return;
      plugin->connect_port((LADSPA_Handle)handle, pIdx[k], (LADSPA_Data*)datalocation);
      SP_TRACE_OUT
      }

//---------------------------------------------------------
//   connectCtrlOutport
//---------------------------------------------------------
void LadspaPlugin::connectCtrlOutport(void* handle, unsigned long k, void* datalocation)
      {
      SP_TRACE_IN
      if(!plugin)
        return;
      plugin->connect_port((LADSPA_Handle)handle, poIdx[k], (LADSPA_Data*)datalocation);
      SP_TRACE_OUT
      }

//---------------------------------------------------------
//   convertGuiControlValue
//  scale control value to gui-slider/checkbox representation
//---------------------------------------------------------

float LadspaPlugin::convertGuiControlValue(unsigned long parameter, int sampleRate, int val) const
      {
      SP_TRACE_IN
      float floatval = 0;
      float min, max;
      range(parameter, sampleRate, &min, &max);

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
      SP_TRACE_OUT
      return floatval;
      }

      
//---------------------------------------------------------
//   PluginI
//---------------------------------------------------------

void PluginI::init()
      {
      _plugin           = 0;
      _sampleRate       = 44100;
      _segmentSize      = 0;
      instances         = 0;
      handle            = 0;
      controls          = 0;
      controlsOut       = 0;
      controlsOutDummy  = 0;
      audioInPorts      = 0;
      audioOutPorts     = 0;
      controlPorts      = 0;
      controlOutPorts   = 0;
      _audioInSilenceBuf = 0;
      _audioOutDummyBuf  = 0;
      _hasLatencyOutPort = false;
      _latencyOutPort = 0;
      _on               = true;
      }

PluginI::PluginI()
      {
      _id = -1;
      init();
      }

//---------------------------------------------------------
//   PluginI
//---------------------------------------------------------

PluginI::~PluginI()
      {
      if (_plugin) {
            deactivate();
            _plugin->incReferences(-1);
            }

      if(_audioInSilenceBuf)
        free(_audioInSilenceBuf);
      if(_audioOutDummyBuf)
        free(_audioOutDummyBuf);

      if (controlsOutDummy)
            delete[] controlsOutDummy;
      if (controlsOut)
            delete[] controlsOut;
      if (controls)
            delete[] controls;
      if (handle)
            delete[] handle;
      }

//---------------------------------------------------------
//   setID
//---------------------------------------------------------

void PluginI::setID(int i)
{
  _id = i;
}

//---------------------------------------------------------
//   getGuiControlValue
//  scale control value to gui-slider/checkbox representation
//---------------------------------------------------------

int PluginI::getGuiControlValue(unsigned long parameter) const
      {
      SP_TRACE_IN
      float val = param(parameter);
      float min, max;
      range(parameter, &min, &max);
      int intval;
      if (isLog(parameter)) {
            intval = SS_map_logdomain2pluginparam(logf(val/(max - min) + min));
            }
      else if (isBool(parameter)) {
            intval = (int) val;
            }
      else {
            float scale = SS_PLUGIN_PARAM_MAX / (max - min);
            intval = (int) ((val - min) * scale);
            }
      SP_TRACE_OUT
      return intval;
      }

//---------------------------------------------------------
//   convertGuiControlValue
//  scale control value to gui-slider/checkbox representation
//---------------------------------------------------------

float PluginI::convertGuiControlValue(unsigned long parameter, int val) const
      {
      SP_TRACE_IN
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
      SP_TRACE_OUT
      return floatval;
      }

//---------------------------------------------------------
//   setChannels
//---------------------------------------------------------

void PluginI::setChannels(int c)
{
      channel = c;

     if(!_plugin)
       return;
     
      const unsigned long ins = _plugin->inports();
      const unsigned long outs = _plugin->outports();
      int ni = 1;
      if(outs)
      {
        ni = c / outs;
        // Ask for one more instance for remainder if required.
        const int re = c % outs;
        if(re != 0)
          ++ni;
      }
      else
      if(ins)
      {
        ni = c / ins;
        // Ask for one more instance for remainder if required.
        const int re = c % ins;
        if(re != 0)
          ++ni;
      }

      if(ni < 1)
        ni = 1;

      if (ni == instances)
            return;

      LADSPA_Handle* handles = new LADSPA_Handle[ni];

      if(ni > instances)
      {
        for(int i = 0; i < ni; ++i)
        {
          if(i < instances)
            // Transfer existing handle from old array to new array.
            handles[i] = handle[i];
          else
          {
            // Create a new plugin instance with handle.
            // Use the plugin's current sample rate.
            handles[i] = _plugin->instantiate(_sampleRate);
            if(handles[i] == NULL)
            {
              fprintf(stderr, "PluginI::setChannels: cannot instantiate instance %d\n", i);

              // Although this is a messed up state not easy to get out of (final # of channels?), try not to assert().
              // Whoever uses these will have to check instance count or null handle, and try to gracefully fix it and allow a song save.
              for(int k = i; k < ni; ++k)
                handles[i] = NULL;
              ni = i + 1;
              //channel = ?;
              break;
            }
          }
        }
      }
      else
      {
        for(int i = 0; i < instances; ++i)
        {
          if(i < ni)
            // Transfer existing handle from old array to new array.
            handles[i] = handle[i];
          else
          {
            // Delete existing plugin instance.
            // Previously we deleted all the instances and rebuilt from scratch.
            // One side effect of this: Since a GUI is constructed only on the first handle,
            //  previously the native GUI would close when changing channels. Now it doesn't, which is good.
            _plugin->deactivate(handle[i]);
            _plugin->cleanup(handle[i]);
          }
        }
      }

      // Delete the old array, and set the new array.
      delete[] handle;
      handle = handles;

      // Connect new instances' ports:
      for(unsigned long k = 0; k < controlPorts; ++k)
      {
        for(int i = instances; i < ni; ++i)
        {
          // All instances' parameter inputs share the same controls. 
          // We don't have a mechanism to expose the other instances' inputs.
          _plugin->connectCtrlInport(handles[i], k, &controls[k].val);
        }
      }
      
      for(unsigned long k = 0; k < controlOutPorts; ++k)
      {
        // Connect only the first instance's parameter output controls.
        // We don't have a mechanism to display the other instances' outputs.
        if(instances == 0 && ni > 0)
          // Only if the existing instances was zero. We are creating one(s) now.
          _plugin->connectCtrlOutport(handle[0], k, &controlsOut[k].val);
        else
        {
          // Connect the rest to dummy ports.
          for(int i = instances; i < ni; ++i)
            _plugin->connectCtrlOutport(handle[i], k, &controlsOutDummy[k].val);
        }
      }
      
      // Activate new instances.
      for(int i = instances; i < ni; ++i)
        _plugin->activate(handle[i]);

      // Finally, set the new number of instances.
      instances = ni;
}

void PluginI::process(unsigned long frames)
{
  if(!_plugin)
    return;
  for(int i = 0; i < instances; ++i)
    _plugin->apply(handle[i], frames);
}

//---------------------------------------------------------
//   apply
//   If ports is 0, just process controllers only, not audio (do not 'run').
//---------------------------------------------------------

void PluginI::apply(unsigned /*pos*/, unsigned long frames, unsigned long ports, float** bufIn, float** bufOut)
{

#ifdef PLUGIN_DEBUGIN_PROCESS
    fprintf(stderr, "PluginI::apply nsamp:%lu\n", n);
#endif

  if(!_plugin)
    return;
  
  if(ports > 0)     // Don't bother if not 'running'.
  {
    connect(ports, 0, bufIn, bufOut);
    process(frames);
  }
}

//---------------------------------------------------------
//   start
// activate and connect control ports
//---------------------------------------------------------

bool PluginI::start()
      {
      if(!_plugin)
        return false;
      // Activate all the instances.
      return activate();
      }

//---------------------------------------------------------
//   stop
// deactivate
//---------------------------------------------------------
bool PluginI::stop()
      {
      if(!_plugin)
        return false;
      // Activate all the instances.
      return deactivate();
      }
      
//---------------------------------------------------------
//   setParam
//---------------------------------------------------------

void PluginI::setParam(unsigned long i, float val)
{
  if(i >= controlPorts)
    return;
  controls[i].val = val;
}

//---------------------------------------------------------
//   initPluginInstance
//    return true on error
//---------------------------------------------------------

bool PluginI::initPluginInstance(Plugin* plug, int c, 
                                 int sampleRate, unsigned int segmentSize,
                                 bool useDenormalBias, float denormalBias)
      {
      _sampleRate = sampleRate;
      _segmentSize = segmentSize;
      channel = c;
      if(plug == 0)
      {
        fprintf(stderr, "initPluginInstance: zero plugin\n");
        return true;
      }
      _plugin = plug;

      if (_plugin->incReferences(1)==0)
        return true;

      QString inst("-" + QString::number(_plugin->instNo()));
      _name  = _plugin->name() + inst;
      _label = _plugin->label() + inst;

      const unsigned long ins = _plugin->inports();
      const unsigned long outs = _plugin->outports();
      if(outs)
      {
        instances = channel / outs;
        // Ask for one more instance for remainder if required.
        const int re = channel % outs;
        if(re != 0)
          ++instances;
        if(instances < 1)
          instances = 1;
      }
      else
      if(ins)
      {
        instances = channel / ins;
        // Ask for one more instance for remainder if required.
        const int re = channel % ins;
        if(re != 0)
          ++instances;
        if(instances < 1)
          instances = 1;
      }
      else
        instances = 1;

      handle = new LADSPA_Handle[instances];
      for(int i = 0; i < instances; ++i)
        handle[i]=NULL;

      for(int i = 0; i < instances; ++i)
      {
        #ifdef PLUGIN_DEBUGIN
        fprintf(stderr, "PluginI::initPluginInstance instance:%d\n", i);
        #endif

        handle[i] = _plugin->instantiate(_sampleRate);
        if(handle[i] == NULL)
          return true;
      }

      const unsigned long port_count = _plugin->portCount();

      audioInPorts = 0;
      audioOutPorts = 0;
      controlPorts = 0;
      controlOutPorts = 0;
      
      unsigned long port = 0;
      for (int i = 0; i < instances; ++i) {
            for (unsigned long k = 0; k < port_count; ++k) {
                  if (_plugin->isAudioIn(k)) {
                        if(port < (unsigned long)channel)
                          ++audioInPorts;
                        ++port;
                        }
                  }
            }
      port = 0;
      for (int i = 0; i < instances; ++i) {
            for (unsigned long k = 0; k < port_count; ++k) {
                  if (_plugin->isAudioOut(k)) {
                        if(port < (unsigned long)channel)
                          ++audioOutPorts;
                        ++port;
                        }
                  }
            }
            
      for(unsigned long k = 0; k < port_count; ++k)
      {
        if(_plugin->isParameterIn(k))
          ++controlPorts;
        else
        if(_plugin->isParameterOut(k))
          ++controlOutPorts;
      }

      if(controlPorts)
        controls    = new Port[controlPorts];
      if(controlOutPorts)
      {
        controlsOut = new Port[controlOutPorts];
        controlsOutDummy = new Port[controlOutPorts];
      }

      for(unsigned long k = 0; k < controlPorts; ++k)
      {
        // Set the parameter input's initial value to the default.
        const double val = _plugin->defaultValue(k);
        controls[k].val = val;
        // All instances' parameter inputs share the same controls. 
        // We don't have a mechanism to expose the other instances' inputs.
        for(int i = 0; i < instances; ++i)
          _plugin->connectCtrlInport(handle[i], k, &controls[k].val);
      }
      
      for(unsigned long k = 0; k < controlOutPorts; ++k)
      {
        // Set the parameter output's initial value to zero.
        controlsOut[k].val = 0.0;
        // Check for a latency port.
        const char* pname = _plugin->getParameterOutName(k);
        if(pname == QString("latency") || pname == QString("_latency"))
        {
          _hasLatencyOutPort = true;
          _latencyOutPort = k;
        }
        // Connect only the first instance's parameter output controls.
        // We don't have a mechanism to display the other instances' outputs.
        if(instances > 0)
        {
          _plugin->connectCtrlOutport(handle[0], k, &controlsOut[k].val);
          // Connect the rest to dummy ports.
          for(int i = 1; i < instances; ++i)
            _plugin->connectCtrlOutport(handle[i], k, &controlsOutDummy[k].val);
        }
      }

      int rv = posix_memalign((void **)&_audioInSilenceBuf, 16, sizeof(float) * _segmentSize);

      if(rv != 0)
      {
          fprintf(stderr, 
            "ERROR: PluginI::initPluginInstance: _audioInSilenceBuf posix_memalign returned error:%d. Aborting!\n", rv);
          abort();
      }

      if(useDenormalBias)
      {
          for(unsigned q = 0; q < _segmentSize; ++q)
          {
            _audioInSilenceBuf[q] = denormalBias;
          }
      }
      else
      {
          memset(_audioInSilenceBuf, 0, sizeof(float) * _segmentSize);
      }

      rv = posix_memalign((void **)&_audioOutDummyBuf, 16, sizeof(float) * _segmentSize);

      if(rv != 0)
      {
          fprintf(stderr, "ERROR: PluginI::initPluginInstance: _audioOutDummyBuf posix_memalign returned error:%d. Aborting!\n", rv);
          abort();
      }

      // Don't activate yet.
      //activate();
      return false;
      }

//---------------------------------------------------------
//   connect
//---------------------------------------------------------

void PluginI::connect(unsigned long ports, unsigned long offset, float** src, float** dst)
      {
      if(!_plugin) return;
      
      const unsigned long port_count = _plugin->portCount();
      unsigned long port = 0;
      for (int i = 0; i < instances; ++i) {
            for (unsigned long k = 0; k < port_count; ++k) {
                  if (isAudioIn(k)) {
                        if(port < ports)
                          _plugin->connectPort(handle[i], k, src[port] + offset);
                        else
                          // Connect to an input silence buffer.
                          _plugin->connectPort(handle[i], k, _audioInSilenceBuf + offset);
                        ++port;
                        }
                  }
            }
      port = 0;
      for (int i = 0; i < instances; ++i) {
            for (unsigned long k = 0; k < port_count; ++k) {
                  if (isAudioOut(k)) {
                        if(port < ports)
                          _plugin->connectPort(handle[i], k, dst[port] + offset);
                        else
                          // Connect to a dummy buffer.
                          _plugin->connectPort(handle[i], k, _audioOutDummyBuf + offset);
                        ++port;
                        }
                  }
            }
      }

// //---------------------------------------------------------
// //   connect
// //---------------------------------------------------------
// 
// void PluginI::connectInport(unsigned long k, void* datalocation)
//       {
//       if(!_plugin) return;
// //       const unsigned long ins = _plugin->inports();
// //       const unsigned long handle_idx = (k / ins) % instances;
// //       const unsigned long port_idx = k % ins;
// //       _plugin->connectInport(handle[handle_idx], port_idx, datalocation);
//       
//       const unsigned long ins = _plugin->inports();
//       for (int i = 0; i < instances; ++i) {
//         for (unsigned long k = 0; k < audio; ++k) {
//       }      
//       
//       const unsigned long port_count = _plugin->portCount();
//       unsigned long port = 0;
//       for (int i = 0; i < instances; ++i) {
//             for (unsigned long k = 0; k < port_count; ++k) {
//                   if (_plugin->isAudioIn(k)) {
//                         //if(port < ports)
//                         if(port < channel)
//                           //_plugin->connectPort(handle[i], k, src[port] + offset);
//                           _plugin->connectInport(handle[i], k, datalocation);
//                         else
//                           // Connect to an input silence buffer.
//                           _plugin->connectInport(handle[i], k, _audioInSilenceBuf);
//                         ++port;
//                         }
//                   }
//             }
//       }
// 
// void PluginI::connectOutport(unsigned long k, void* datalocation)
//       {
//       if(!_plugin) return;
// //       const unsigned long outs = _plugin->outports();
// //       const unsigned long handle_idx = (k / outs) % instances;
// //       const unsigned long port_idx = k % outs;
// //       _plugin->connectOutport(handle[handle_idx], port_idx, datalocation);
//       
//       const unsigned long port_count = _plugin->portCount();
//       unsigned long port = 0;
//       for (int i = 0; i < instances; ++i) {
//             for (unsigned long k = 0; k < port_count; ++k) {
//                   if (_plugin->isAudioOut(k)) {
//                         //if(port < ports)
//                         if(port < channel)
//                           _plugin->connectOutport(handle[i], k, datalocation);
//                         else
//                           // Connect to a dummy buffer.
//                           _plugin->connectOutport(handle[i], k, _audioOutDummyBuf);
//                         ++port;
//                         }
//                   }
//             }
//       }
// 
// void PluginI::connectCtrlInport(unsigned long k, void* datalocation)
//       {
//       if(!_plugin) return;
//       const unsigned long ctrl_ins = _plugin->parameter();
//       const unsigned long handle_idx = (k / ctrl_ins) % instances;
//       const unsigned long port_idx = k % ctrl_ins;
//       _plugin->connectCtrlInport(handle[handle_idx], port_idx, datalocation);
//       }
// 
// void PluginI::connectCtrlOutport(unsigned long k, void* datalocation)
//       {
//       if(!_plugin) return;
//       const unsigned long ctrl_outs = _plugin->parameterOut();
//       const unsigned long handle_idx = (k / ctrl_outs) % instances;
//       const unsigned long port_idx = k % ctrl_outs;
//       _plugin->connectCtrlOutport(handle[handle_idx], port_idx, datalocation);
//       }

//---------------------------------------------------------
//   deactivate
//---------------------------------------------------------

bool PluginI::deactivate()
      {
      if(!_plugin)
        return false;
      for (int i = 0; i < instances; ++i) {
            _plugin->deactivate(handle[i]);
            _plugin->cleanup(handle[i]);
            }
      return true;
      }

//---------------------------------------------------------
//   activate
//---------------------------------------------------------

bool PluginI::activate()
      {
      if(!_plugin)
        return false;
      for (int i = 0; i < instances; ++i)
            _plugin->activate(handle[i]);
      return true;
      }

//---------------------------------------------------------
//   latency
//---------------------------------------------------------

float PluginI::latency() const
{
  if(!_hasLatencyOutPort)
    return 0.0;
  return controlsOut[_latencyOutPort].val;
}


//---------------------------------------------------------
//   setControl
//    set plugin instance controller value by name
//---------------------------------------------------------

bool PluginI::setControl(const QString& s, float val)
      {
      if(!_plugin)
        return true;
      for (unsigned long i = 0; i < controlPorts; ++i) {
            if (QString(_plugin->getParameterName(i)) == s) {
                  setParam(i, val);
                  return false;
                  }
            }
      fprintf(stderr, "PluginI:setControl(%s, %f) controller not found\n",
         s.toLatin1().constData(), val);
      return true;
      }

      
} // namespace MusESimplePlugin
