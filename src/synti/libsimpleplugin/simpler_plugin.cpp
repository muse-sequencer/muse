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
//#include <unistd.h>
#include <dlfcn.h>
#include <string>

#include "simpler_plugin.h"
#include "plugin_cache_reader.h"

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
//   initPlugins
//    search for LADSPA plugins
//---------------------------------------------------------

void SS_initPlugins(const QString& hostCachePath)
{
  SP_TRACE_IN
  
  MusEPlugin::PluginScanList scan_list;
  // Read host plugin cache file. We only want ladspa plugins for now...
  MusEPlugin::readPluginCacheFile(hostCachePath + "/scanner",
                                  &scan_list,
                                  false,
                                  false,
                                  MusEPlugin::PluginScanInfoStruct::PluginTypeLADSPA);
  for(MusEPlugin::ciPluginScanList isl = scan_list.begin(); isl != scan_list.end(); ++isl)
  {
    const MusEPlugin::PluginScanInfoRef inforef = *isl;
    const MusEPlugin::PluginScanInfoStruct& info = inforef->info();
    switch(info._type)
    {
      case MusEPlugin::PluginScanInfoStruct::PluginTypeLADSPA:
      {
        //if(MusEGlobal::loadPlugins)
        {
          // Make sure it doesn't already exist.
          if(/*Plugin* pl =*/ plugins.find(PLUGIN_GET_QSTRING(info._completeBaseName),
             PLUGIN_GET_QSTRING(info._label)))
          {
            //fprintf(stderr, "Ignoring LADSPA effect label:%s path:%s duplicate of path:%s\n",
            //        info._label.toLatin1().constData(),
            //        info._fi.filePath().toLatin1().constData(),
            //        pl->filePath().toLatin1().constData());
          }
          else
          {
            plugins.push_back(new LadspaPlugin(info));
          }
        }
      }
      break;
      
      case MusEPlugin::PluginScanInfoStruct::PluginTypeDSSI:
      case MusEPlugin::PluginScanInfoStruct::PluginTypeDSSIVST:
      case MusEPlugin::PluginScanInfoStruct::PluginTypeVST:
      case MusEPlugin::PluginScanInfoStruct::PluginTypeLV2:
      case MusEPlugin::PluginScanInfoStruct::PluginTypeLinuxVST:
      case MusEPlugin::PluginScanInfoStruct::PluginTypeMESS:
      case MusEPlugin::PluginScanInfoStruct::PluginTypeUnknown:
      case MusEPlugin::PluginScanInfoStruct::PluginTypeNone:
      case MusEPlugin::PluginScanInfoStruct::PluginTypeAll:
      break;
    }
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
//   Plugin
//---------------------------------------------------------

Plugin::Plugin(const MusEPlugin::PluginScanInfoStruct& info)
  : _fi(PLUGIN_GET_QSTRING(info.filePath())),
    _libHandle(0),
    _references(0),
    _instNo(0),
    _uniqueID(info._uniqueID), 
    _label(PLUGIN_GET_QSTRING(info._label)),
    _name(PLUGIN_GET_QSTRING(info._name)),
    _maker(PLUGIN_GET_QSTRING(info._maker)),
    _copyright(PLUGIN_GET_QSTRING(info._copyright)),
    _portCount(info._portCount),
    _inports(info._inports),
    _outports(info._outports),
    _controlInPorts(info._controlInPorts),
    _controlOutPorts(info._controlOutPorts),
    _requiredFeatures(info._requiredFeatures) { }

//---------------------------------------------------------
//   LadspaPlugin
//---------------------------------------------------------

LadspaPlugin::LadspaPlugin(const QFileInfo* f,
   const LADSPA_Descriptor_Function /*ldf*/,
   const LADSPA_Descriptor* d)
   : Plugin(f)
      {
      SP_TRACE_IN
      
      _plugin = nullptr;
      
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
          }
          else
          if(pd & LADSPA_PORT_OUTPUT)
          {
            ++_outports;
          }
        }
        else
        if(pd & LADSPA_PORT_CONTROL)
        {
          if(pd & LADSPA_PORT_INPUT)
          {
            ++_controlInPorts;
          }
          else
          if(pd & LADSPA_PORT_OUTPUT)
          {
            ++_controlOutPorts;
          }
        }
      }
  
      /*if (SP_DEBUG_LADSPA) {
            printf("Label: %s\tLib: %s\tPortCount: %d\n", this->label().toLatin1().constData(), this->lib().toLatin1().constData(), plugin->PortCount);
            printf("LADSPA_PORT_CONTROL|LADSPA_PORT_INPUT: %d\t", pIdx.size());
            printf("Input ports: %d\t", iIdx.size());
            printf("Output ports: %d\n\n", oIdx.size());
            }*/

      if ((_inports != _outports) || (LADSPA_IS_INPLACE_BROKEN(d->Properties)))
        _requiredFeatures |= MusECore::PluginNoInPlaceProcessing;
      
      SP_TRACE_OUT
      }

LadspaPlugin::LadspaPlugin(const MusEPlugin::PluginScanInfoStruct& info)
  : Plugin(info), _plugin(NULL)
{
   SP_TRACE_IN

  SP_TRACE_OUT
}
      
//---------------------------------------------------------
//   createPluginI
//---------------------------------------------------------

PluginI* LadspaPlugin::createPluginI(int chans, float sampleRate, unsigned int segmentSize,
                                           bool useDenormalBias, float denormalBias)
{
   LadspaPluginI* plug_i = new LadspaPluginI();
   if(plug_i->initPluginInstance(
      this,
      chans,
      sampleRate, 
      segmentSize,
      useDenormalBias,
      denormalBias))
   {
     fprintf(stderr, "LadspaPlugin::createPluginI: cannot instantiate plugin <%s>\n",
       name().toLatin1().constData());
     // Make sure to delete the PluginI.
     delete plug_i;
     return 0;
   }
   return plug_i;
}
      
      
//---------------------------------------------------------
//   incReferences
//---------------------------------------------------------

int LadspaPlugin::incReferences(int val)
{
  #ifdef PLUGIN_DEBUGIN
  fprintf(stderr, "LadspaPlugin::incReferences _references:%d val:%d\n", _references, val);
  #endif

  int newref = _references + val;

  if(newref <= 0)
  {
    _references = 0;
    if(_libHandle)
    {
      #ifdef PLUGIN_DEBUGIN
      fprintf(stderr, "LadspaPlugin::incReferences no more instances, closing library\n");
      #endif

      dlclose(_libHandle);
    }

    _libHandle = 0;
    _plugin = nullptr;
    _pIdx.clear();
    _poIdx.clear();
    _iIdx.clear();
    _oIdx.clear();
    _requiredFeatures = MusECore::PluginNoFeatures;

    return 0;
  }

  if(_libHandle == 0)
  {
    _libHandle = dlopen(_fi.filePath().toLatin1().constData(), RTLD_NOW);

    if(_libHandle == 0)
    {
      fprintf(stderr, "LadspaPlugin::incReferences dlopen(%s) failed: %s\n",
                    _fi.filePath().toLatin1().constData(), dlerror());
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
          _plugin = descr;
          break;
        }
      }
    }

    if(_plugin != NULL)
    {
      _uniqueID = _plugin->UniqueID;

      _label = QString(_plugin->Label);
      _name = QString(_plugin->Name);
      _maker = QString(_plugin->Maker);
      _copyright = QString(_plugin->Copyright);

      _portCount = _plugin->PortCount;
      _inports = 0;
      _outports = 0;
      _controlInPorts = 0;
      _controlOutPorts = 0;
      
      for(unsigned long k = 0; k < _portCount; ++k)
      {
        LADSPA_PortDescriptor pd = _plugin->PortDescriptors[k];
        if(pd & LADSPA_PORT_AUDIO)
        {
          if(pd & LADSPA_PORT_INPUT)
          {
            ++_inports;
            _iIdx.push_back(k);
          }
          else
          if(pd & LADSPA_PORT_OUTPUT)
          {
            ++_outports;
            _oIdx.push_back(k);
          }
        }
        else
        if(pd & LADSPA_PORT_CONTROL)
        {
          if(pd & LADSPA_PORT_INPUT)
          {
            ++_controlInPorts;
            _pIdx.push_back(k);
          }
          else
          if(pd & LADSPA_PORT_OUTPUT)
          {
            ++_controlOutPorts;
            _poIdx.push_back(k);
          }
        }
      }
    }
  }

  if(_plugin == NULL)
  {
    dlclose(_libHandle);
    _libHandle = 0;
    _references = 0;
    fprintf(stderr, "LadspaPlugin::incReferences Error: %s no plugin!\n", _fi.filePath().toLatin1().constData());
    return 0;
  }

  if ((_inports != _outports) || (LADSPA_IS_INPLACE_BROKEN(_plugin->Properties)))
    _requiredFeatures |= MusECore::PluginNoInPlaceProcessing;
  
  _references = newref;

  return _references;
}

//---------------------------------------------------------
//   instantiate
//---------------------------------------------------------

void* LadspaPlugin::instantiate(float sampleRate, void*)
{
  if(!_plugin)
    return nullptr;
  bool success = false;
  LADSPA_Handle h = _plugin->instantiate(_plugin, sampleRate);
  success = (h != NULL);
  if(success)
  {
    SP_DBG_LADSPA2("LadspaPlugin instantiated", label().toLatin1().constData());
  }
  return h;
}

//---------------------------------------------------------
//   range
//---------------------------------------------------------

bool LadspaPlugin::port_range(unsigned long i, float sampleRate, float* min, float* max) const
      {
      if(!_plugin)
        return false;
      LADSPA_PortRangeHint range = _plugin->PortRangeHints[i];
      LADSPA_PortRangeHintDescriptor desc = range.HintDescriptor;
      if (desc & LADSPA_HINT_TOGGLED) {
            *min = 0.0;
            *max = 1.0;
            return true;
            }
      float m = 1.0;
      if (desc & LADSPA_HINT_SAMPLE_RATE)
            m = sampleRate;

      if (desc & LADSPA_HINT_BOUNDED_BELOW)
            *min =  range.LowerBound * m;
      else
            *min = 0.0;
      if (desc & LADSPA_HINT_BOUNDED_ABOVE)
            *max =  range.UpperBound * m;
      else
            *max = 1.0;
      return true;
      }

//---------------------------------------------------------
//   range
//---------------------------------------------------------

bool LadspaPlugin::range(unsigned long k, float sampleRate, float* min, float* max) const
      {
      SP_TRACE_IN
      k = _pIdx[k];
      SP_TRACE_OUT
      return port_range(k, sampleRate, min, max);
      }

//---------------------------------------------------------
//   range
//---------------------------------------------------------

bool LadspaPlugin::rangeOut(unsigned long k, float sampleRate, float* min, float* max) const
      {
      SP_TRACE_IN
      k = _poIdx[k];
      SP_TRACE_OUT
      return port_range(k, sampleRate, min, max);
      }

//---------------------------------------------------------
//   defaultValue
//---------------------------------------------------------

float LadspaPlugin::defaultValue(unsigned long k) const
      {
      SP_TRACE_IN
      if(!_plugin)
        return 0.0f;
      k = _pIdx[k];
      LADSPA_PortRangeHint range = _plugin->PortRangeHints[k];
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
      if(!_plugin)
        return;
      _plugin->connect_port((LADSPA_Handle)handle, _iIdx[k], (LADSPA_Data*)datalocation);
      SP_TRACE_OUT
      }

//---------------------------------------------------------
//   connectOutport
//---------------------------------------------------------
void LadspaPlugin::connectOutport(void* handle, unsigned long k, void* datalocation)
      {
      SP_TRACE_IN
      if(!_plugin)
        return;
      _plugin->connect_port((LADSPA_Handle)handle, _oIdx[k], (LADSPA_Data*)datalocation);
      SP_TRACE_OUT
      }

//---------------------------------------------------------
//   connectCtrlInport
//---------------------------------------------------------
void LadspaPlugin::connectCtrlInport(void* handle, unsigned long k, void* datalocation)
      {
      SP_TRACE_IN
      if(!_plugin)
        return;
      _plugin->connect_port((LADSPA_Handle)handle, _pIdx[k], (LADSPA_Data*)datalocation);
      SP_TRACE_OUT
      }

//---------------------------------------------------------
//   connectCtrlOutport
//---------------------------------------------------------
void LadspaPlugin::connectCtrlOutport(void* handle, unsigned long k, void* datalocation)
      {
      SP_TRACE_IN
      if(!_plugin)
        return;
      _plugin->connect_port((LADSPA_Handle)handle, _poIdx[k], (LADSPA_Data*)datalocation);
      SP_TRACE_OUT
      }

//---------------------------------------------------------
//   convertGuiControlValue
//  scale control value to gui-slider/checkbox representation
//---------------------------------------------------------

float LadspaPlugin::convertGuiControlValue(unsigned long k, float sampleRate, int val) const
      {
      SP_TRACE_IN
      float floatval = 0.0;
      float min, max;
      if(!range(k, sampleRate, &min, &max))
        return floatval;

      if (isLog(k)) {
            if (val > 0) {
                  float logged = SS_map_pluginparam2logdomain(val);
                  float e = expf(logged) * (max - min);
                  e+=min;
                  floatval = e;
                  }
            }
      else if (isBool(k)) {
            floatval = (float) val;
            }
      else if (isInt(k)) {
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
      _sampleRate       = 44100.0f;
      _dSampleRate      = _sampleRate;
      _segmentSize      = 0;
      _instances         = 0;
      _controls          = 0;
      _controlsOut       = 0;
      _controlsOutDummy  = 0;
      _audioInPorts      = 0;
      _audioOutPorts     = 0;
      _controlPorts      = 0;
      _controlOutPorts   = 0;
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
      if(_audioInSilenceBuf)
        free(_audioInSilenceBuf);
      if(_audioOutDummyBuf)
        free(_audioOutDummyBuf);

      if(_controlsOutDummy)
        delete[] _controlsOutDummy;
      if(_controlsOut)
        delete[] _controlsOut;
      if(_controls)
        delete[] _controls;
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
  if(i >= _controlPorts)
    return;
  _controls[i]._val = val;
}

// TODO
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
//   latency
//---------------------------------------------------------

float PluginI::latency() const
{
  if(!_hasLatencyOutPort)
    return 0.0;
  return _controlsOut[_latencyOutPort]._val;
}


//---------------------------------------------------------
//   setControl
//    set plugin instance controller value by name
//---------------------------------------------------------

bool PluginI::setControl(const QString& s, float val)
      {
      if(!_plugin)
        return true;
      for (unsigned long i = 0; i < _controlPorts; ++i) {
            if (QString(_plugin->getParameterName(i)) == s) {
                  setParam(i, val);
                  return false;
                  }
            }
      fprintf(stderr, "PluginI:setControl(%s, %f) controller not found\n",
         s.toLatin1().constData(), val);
      return true;
      }


      
//---------------------------------------------------------
//   LadspaPluginI
//---------------------------------------------------------

void LadspaPluginI::init()
{
  _handle = 0;
}

LadspaPluginI::LadspaPluginI()
  : PluginI()
{
  init();
}

//---------------------------------------------------------
//   LadspaPluginI
//---------------------------------------------------------

LadspaPluginI::~LadspaPluginI()
{
  if(_plugin) {
    // Deactivate is pure virtual, it cannot be 
    //  called from the base destructor. Do it here.
    deactivate();
    _plugin->incReferences(-1);
  }
  
  if(_handle)
    delete[] _handle;
}

//---------------------------------------------------------
//   initPluginInstance
//    return true on error
//---------------------------------------------------------

bool LadspaPluginI::initPluginInstance(Plugin* plug, int chans, 
                                 float sampleRate, unsigned int segmentSize,
                                 bool useDenormalBias, float denormalBias)
{
  _sampleRate = _dSampleRate = sampleRate;
  _segmentSize = segmentSize;
  _channel = chans;
  if(plug == 0)
  {
    fprintf(stderr, "LadspaPluginI::initPluginInstance: zero plugin\n");
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
    _instances = _channel / outs;
    // Ask for one more instance for remainder if required.
    const int re = _channel % outs;
    if(re != 0)
      ++_instances;
    if(_instances < 1)
      _instances = 1;
  }
  else
  if(ins)
  {
    _instances = _channel / ins;
    // Ask for one more instance for remainder if required.
    const int re = _channel % ins;
    if(re != 0)
      ++_instances;
    if(_instances < 1)
      _instances = 1;
  }
  else
    _instances = 1;

  _handle = new LADSPA_Handle[_instances];
  for(int i = 0; i < _instances; ++i)
    _handle[i]=nullptr;

  for(int i = 0; i < _instances; ++i)
  {
    #ifdef PLUGIN_DEBUGIN
    fprintf(stderr, "LadspaPluginI::initPluginInstance instance:%d\n", i);
    #endif

    _handle[i] = _plugin->instantiate(_sampleRate, NULL);
    if(_handle[i] == NULL)
      return true;
  }

  const unsigned long port_count = _plugin->portCount();

  _audioInPorts = 0;
  _audioOutPorts = 0;
  _controlPorts = 0;
  _controlOutPorts = 0;
  
  unsigned long port = 0;
  for (int i = 0; i < _instances; ++i) {
        for (unsigned long k = 0; k < port_count; ++k) {
              if (_plugin->isAudioIn(k)) {
                    if(port < (unsigned long)_channel)
                      ++_audioInPorts;
                    ++port;
                    }
              }
        }
  port = 0;
  for (int i = 0; i < _instances; ++i) {
        for (unsigned long k = 0; k < port_count; ++k) {
              if (_plugin->isAudioOut(k)) {
                    if(port < (unsigned long)_channel)
                      ++_audioOutPorts;
                    ++port;
                    }
              }
        }
        
  for(unsigned long k = 0; k < port_count; ++k)
  {
    if(_plugin->isParameterIn(k))
      ++_controlPorts;
    else
    if(_plugin->isParameterOut(k))
      ++_controlOutPorts;
  }

  if(_controlPorts)
    _controls = new Port[_controlPorts];
  if(_controlOutPorts)
  {
    _controlsOut = new Port[_controlOutPorts];
    _controlsOutDummy = new Port[_controlOutPorts];
  }

  for(unsigned long k = 0; k < _controlPorts; ++k)
  {
    // Set the parameter input's initial value to the default.
    const float val = _plugin->defaultValue(k);
    _controls[k]._val = val;
    // All instances' parameter inputs share the same controls. 
    // We don't have a mechanism to expose the other instances' inputs.
    for(int i = 0; i < _instances; ++i)
      _plugin->connectCtrlInport(_handle[i], k, &_controls[k]._val);
  }
  
  for(unsigned long k = 0; k < _controlOutPorts; ++k)
  {
    // Set the parameter output's initial value to zero.
    _controlsOut[k]._val = 0.0f;
    // Check for a latency port.
    const char* pname = _plugin->getParameterOutName(k);
    if(pname == QString("latency") || pname == QString("_latency"))
    {
      _hasLatencyOutPort = true;
      _latencyOutPort = k;
    }
    // Connect only the first instance's parameter output controls.
    // We don't have a mechanism to display the other instances' outputs.
    if(_instances > 0)
    {
      _plugin->connectCtrlOutport(_handle[0], k, &_controlsOut[k]._val);
      // Connect the rest to dummy ports.
      for(int i = 1; i < _instances; ++i)
        _plugin->connectCtrlOutport(_handle[i], k, &_controlsOutDummy[k]._val);
    }
  }

#ifdef _WIN32
  _audioInSilenceBuf = (float *) _aligned_malloc(16, sizeof(float) * _segmentSize);
  if(_audioInSilenceBuf == NULL)
  {
      fprintf(stderr, 
        "ERROR: LadspaPluginI::initPluginInstance: _audioInSilenceBuf _aligned_malloc returned error: NULL. Aborting!\n");
      abort();
  }
#else
  int rv = posix_memalign((void **)&_audioInSilenceBuf, 16, sizeof(float) * _segmentSize);
  if(rv != 0)
  {
      fprintf(stderr, 
        "ERROR: LadspaPluginI::initPluginInstance: _audioInSilenceBuf posix_memalign returned error:%d. Aborting!\n", rv);
      abort();
  }
#endif

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

#ifdef _WIN32
  _audioOutDummyBuf = (float *) _aligned_malloc(16, sizeof(float) * _segmentSize);
  if(_audioOutDummyBuf == NULL)
  {
      fprintf(stderr, 
        "ERROR: LadspaPluginI::initPluginInstance: _audioInSilenceBuf _aligned_malloc returned error: NULL. Aborting!\n");
      abort();
  }
#else
  rv = posix_memalign((void **)&_audioOutDummyBuf, 16, sizeof(float) * _segmentSize);
  if(rv != 0)
  {
      fprintf(stderr, "ERROR: LadspaPluginI::initPluginInstance: _audioOutDummyBuf posix_memalign returned error:%d. Aborting!\n", rv);
      abort();
  }
#endif

  // Don't activate yet.
  //activate();
  return false;
}

//---------------------------------------------------------
//   setChannels
//---------------------------------------------------------

void LadspaPluginI::setChannels(int chans)
{
     _channel = chans;

     if(!_plugin)
       return;
     
      const unsigned long ins = _plugin->inports();
      const unsigned long outs = _plugin->outports();
      int ni = 1;
      if(outs)
      {
        ni = chans / outs;
        // Ask for one more instance for remainder if required.
        const int re = chans % outs;
        if(re != 0)
          ++ni;
      }
      else
      if(ins)
      {
        ni = chans / ins;
        // Ask for one more instance for remainder if required.
        const int re = chans % ins;
        if(re != 0)
          ++ni;
      }

      if(ni < 1)
        ni = 1;

      if (ni == _instances)
            return;

      LADSPA_Handle* handles = new LADSPA_Handle[ni];

      if(ni > _instances)
      {
        for(int i = 0; i < ni; ++i)
        {
          if(i < _instances)
            // Transfer existing handle from old array to new array.
            handles[i] = _handle[i];
          else
          {
            // Create a new plugin instance with handle.
            // Use the plugin's current sample rate.
            handles[i] = _plugin->instantiate(_sampleRate, NULL);
            if(handles[i] == NULL)
            {
              fprintf(stderr, "LadspaPluginI::setChannels: cannot instantiate instance %d\n", i);

              // Although this is a messed up state not easy to get out of (final # of channels?), try not to assert().
              // Whoever uses these will have to check instance count or null handle, and try to gracefully fix it and allow a song save.
              for(int k = i; k < ni; ++k)
                handles[i] = nullptr;
              ni = i + 1;
              //channel = ?;
              break;
            }
          }
        }
      }
      else
      {
        for(int i = 0; i < _instances; ++i)
        {
          if(i < ni)
            // Transfer existing handle from old array to new array.
            handles[i] = _handle[i];
          else
          {
            // Delete existing plugin instance.
            // Previously we deleted all the instances and rebuilt from scratch.
            // One side effect of this: Since a GUI is constructed only on the first handle,
            //  previously the native GUI would close when changing channels. Now it doesn't, which is good.
            _plugin->deactivate(_handle[i]);
            _plugin->cleanup(_handle[i]);
          }
        }
      }

      // Delete the old array, and set the new array.
      delete[] _handle;
      _handle = handles;

      // Connect new instances' ports:
      for(unsigned long k = 0; k < _controlPorts; ++k)
      {
        for(int i = _instances; i < ni; ++i)
        {
          // All instances' parameter inputs share the same controls. 
          // We don't have a mechanism to expose the other instances' inputs.
          _plugin->connectCtrlInport(handles[i], k, &_controls[k]._val);
        }
      }
      
      for(unsigned long k = 0; k < _controlOutPorts; ++k)
      {
        // Connect only the first instance's parameter output controls.
        // We don't have a mechanism to display the other instances' outputs.
        if(_instances == 0 && ni > 0)
          // Only if the existing instances was zero. We are creating one(s) now.
          _plugin->connectCtrlOutport(_handle[0], k, &_controlsOut[k]._val);
        else
        {
          // Connect the rest to dummy ports.
          for(int i = _instances; i < ni; ++i)
            _plugin->connectCtrlOutport(_handle[i], k, &_controlsOutDummy[k]._val);
        }
      }
      
      // Activate new instances.
      for(int i = _instances; i < ni; ++i)
        _plugin->activate(_handle[i]);

      // Finally, set the new number of instances.
      _instances = ni;
}

//---------------------------------------------------------
//   connect
//---------------------------------------------------------

void LadspaPluginI::connect(unsigned long ports, unsigned long offset, float** src, float** dst)
      {
      if(!_plugin) return;
      
      const unsigned long port_count = _plugin->portCount();
      unsigned long port = 0;
      for (int i = 0; i < _instances; ++i) {
            for (unsigned long k = 0; k < port_count; ++k) {
                  if (isAudioIn(k)) {
                        if(port < ports)
                          _plugin->connectPort(_handle[i], k, src[port] + offset);
                        else
                          // Connect to an input silence buffer.
                          _plugin->connectPort(_handle[i], k, _audioInSilenceBuf + offset);
                        ++port;
                        }
                  }
            }
      port = 0;
      for (int i = 0; i < _instances; ++i) {
            for (unsigned long k = 0; k < port_count; ++k) {
                  if (isAudioOut(k)) {
                        if(port < ports)
                          _plugin->connectPort(_handle[i], k, dst[port] + offset);
                        else
                          // Connect to a dummy buffer.
                          _plugin->connectPort(_handle[i], k, _audioOutDummyBuf + offset);
                        ++port;
                        }
                  }
            }
      }


//---------------------------------------------------------
//   deactivate
//---------------------------------------------------------

bool LadspaPluginI::deactivate()
      {
      if(!_plugin)
        return false;
      for (int i = 0; i < _instances; ++i) {
            _plugin->deactivate(_handle[i]);
            _plugin->cleanup(_handle[i]);
            }
      return true;
      }

//---------------------------------------------------------
//   activate
//---------------------------------------------------------

bool LadspaPluginI::activate()
      {
      if(!_plugin)
        return false;
      for (int i = 0; i < _instances; ++i)
            _plugin->activate(_handle[i]);
      return true;
      }
      
void LadspaPluginI::process(unsigned long frames)
{
  if(!_plugin)
    return;
  for(int i = 0; i < _instances; ++i)
    _plugin->apply(_handle[i], frames);
}
      
} // namespace MusESimplePlugin
