//
// C++ Interface: plugin
//
// Description:
//
//
//  (C) Copyright 2000 Werner Schweer (ws@seh.de)
// Additions/modifications: Mathias Lundgren <lunar_shuttle@users.sf.net>, (C) 2004
//                          (C) Copyright 2011 Tim E. Real (terminator356 at users.sourceforge.net)
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

#ifndef __SIMPLER_PLUGIN_H__
#define __SIMPLER_PLUGIN_H__

#include <vector>
#include <QFileInfo>
#include <QString>

#include <ladspa.h>
#include <math.h>

#define SS_PLUGIN_PARAM_MIN                  0
#define SS_PLUGIN_PARAM_MAX                127

namespace MusESimplePlugin {
  
//---------------------------------------------------------
//   Port
//---------------------------------------------------------

struct Port {
      float val;
      };

//---------------------------------------------------------
//   Plugin
//---------------------------------------------------------

class Plugin
   {
   protected:
      QFileInfo fi;

      void* _libHandle;
      int _references;
      int _instNo;
      unsigned long _uniqueID;
      QString _label;
      QString _name;
      QString _maker;
      QString _copyright;
      
      // Total number of ports.
      unsigned long _portCount;
      unsigned long _inports;
      unsigned long _outports;
      unsigned long _controlInPorts;
      unsigned long _controlOutPorts;
      bool _inPlaceCapable;

      //PluginFeatures _requiredFeatures;
      
      std::vector<unsigned long> pIdx; //control port numbers
      std::vector<unsigned long> poIdx; //control out port numbers
      std::vector<unsigned long> iIdx; //input port numbers
      std::vector<unsigned long> oIdx; //output port numbers

   public:
      Plugin(const QFileInfo* f);
      virtual ~Plugin() {}
      
      int references() const            { return _references; }
      virtual int incReferences(int)    { return _references; }
      int instNo()                      { return _instNo++;   }
      virtual void* instantiate(int /*sampleRate*/) { return 0; }
      
      virtual int sampleRate() const { return 44100; }
      virtual void setSampleRate(int) { }
      
      QString label() const                        { return _label; }
      QString name() const                         { return _name; }
      unsigned long id() const                     { return _uniqueID; }
      QString maker() const                        { return _maker; }
      QString copyright() const                    { return _copyright; }
      QString lib(bool complete = true) const      { return complete ? fi.completeBaseName() : fi.baseName(); }
      QString dirPath(bool complete = true) const  { return complete ? fi.absolutePath() : fi.path(); }
      QString filePath() const                     { return fi.filePath(); }
      QString fileName() const                     { return fi.fileName(); }
      
      // Total number of ports.
      unsigned long portCount() const       { return _portCount; }
      unsigned long parameter() const       { return _controlInPorts; }
      unsigned long parameterOut() const    { return _controlOutPorts; }
      unsigned long inports() const         { return _inports;     }
      unsigned long outports() const        { return _outports;     }
      bool inPlaceCapable() const           { return _inPlaceCapable; }

      virtual bool isAudioIn(unsigned long) const { return false; }
      virtual bool isAudioOut(unsigned long) const { return false; }
      virtual bool isParameterIn(unsigned long) const { return false; }
      virtual bool isParameterOut(unsigned long) const { return false; }
            
      virtual bool isLog(unsigned long) const         { return false; }
      virtual bool isBool(unsigned long) const        { return false; }
      virtual bool isInt(unsigned long) const         { return false; }
      virtual bool isLinear(unsigned long) const      { return false; }
      virtual float defaultValue(unsigned long) const { return 0.0f;  }
      virtual void range(unsigned long, float* min, float* max) const {
            if(min) *min = 0.0f;
            if(max) *max = 1.0f;
            }
      virtual void rangeOut(unsigned long, float* min, float* max) const {
            if(min) *min = 0.0f;
            if(max) *max = 1.0f;
            }
      virtual const char* getParameterName(unsigned long /*param*/) const    { return ""; }
      virtual const char* getParameterOutName(unsigned long /*param*/) const { return ""; }

      virtual void activate(void* /*handle*/) { }
      virtual void deactivate(void* /*handle*/) { }
      virtual void cleanup(void* /*handle*/) { }
      virtual void connectInport(void* /*handle*/, unsigned long, void* /*datalocation*/) { }
      virtual void connectOutport(void* /*handle*/, unsigned long, void* /*datalocation*/) { }
      virtual void connectCtrlInport(void* /*handle*/, unsigned long, void* /*datalocation*/) { }
      virtual void connectCtrlOutport(void* /*handle*/, unsigned long, void* /*datalocation*/) { }
      virtual void connectPort(void* /*handle*/, unsigned long /*port*/, float* /*value*/) { }
      virtual void apply(void* /*handle*/, unsigned long /*n*/) { }
   };

//---------------------------------------------------------
//   LadspaPlugin
//---------------------------------------------------------

#define IS_AUDIO_IN (LADSPA_PORT_AUDIO  | LADSPA_PORT_INPUT)
#define IS_AUDIO_OUT (LADSPA_PORT_AUDIO | LADSPA_PORT_OUTPUT)
#define IS_PARAMETER_IN (LADSPA_PORT_CONTROL  | LADSPA_PORT_INPUT)
#define IS_PARAMETER_OUT (LADSPA_PORT_CONTROL | LADSPA_PORT_OUTPUT)

class LadspaPlugin : public Plugin
   {
private:
#ifdef __clang__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-private-field"
#endif
      LADSPA_Descriptor_Function ladspa;
#ifdef __clang__
#pragma GCC diagnostic pop
#endif
      const LADSPA_Descriptor* plugin;
      
      // Accepts a master port index.
      void port_range(unsigned long i, float*, float*) const;
      
   protected:
      int _sampleRate;

   public:
      LadspaPlugin(const QFileInfo* f, const LADSPA_Descriptor_Function, const LADSPA_Descriptor* d);
      virtual ~LadspaPlugin() { }

      virtual int incReferences(int);
      
      virtual int sampleRate() const { return _sampleRate; }
      virtual void setSampleRate(int rate) { _sampleRate = rate; }
      
      virtual bool isAudioIn(unsigned long k) const {
            return (plugin->PortDescriptors[k] & IS_AUDIO_IN) == IS_AUDIO_IN;
            }
      virtual bool isAudioOut(unsigned long k) const {
            return (plugin->PortDescriptors[k] & IS_AUDIO_OUT) == IS_AUDIO_OUT;
            }
      virtual bool isParameterIn(unsigned long k) const {
            return (plugin->PortDescriptors[k] & IS_PARAMETER_IN) == IS_PARAMETER_IN;
            }
      virtual bool isParameterOut(unsigned long k) const {
            return (plugin->PortDescriptors[k] & IS_PARAMETER_OUT) == IS_PARAMETER_OUT;
            }
      
      virtual bool isLog(unsigned long k) const {
            LADSPA_PortRangeHint r = plugin->PortRangeHints[pIdx[k]];
            return LADSPA_IS_HINT_LOGARITHMIC(r.HintDescriptor);
            }
      virtual bool isBool(unsigned long k) const {
            return LADSPA_IS_HINT_TOGGLED(plugin->PortRangeHints[pIdx[k]].HintDescriptor);
            }
      virtual bool isInt(unsigned long k) const {
            LADSPA_PortRangeHint r = plugin->PortRangeHints[pIdx[k]];
            return LADSPA_IS_HINT_INTEGER(r.HintDescriptor);
            }
      virtual bool isLinear(unsigned long k) const {
            LADSPA_PortRangeHint r = plugin->PortRangeHints[pIdx[k]];
            return !LADSPA_IS_HINT_INTEGER(r.HintDescriptor) &&
                   !LADSPA_IS_HINT_LOGARITHMIC(r.HintDescriptor) &&
                   !LADSPA_IS_HINT_TOGGLED(r.HintDescriptor);
            }
      virtual void range(unsigned long i, float*, float*) const;
      virtual void rangeOut(unsigned long i, float*, float*) const;
      virtual const char* getParameterName(unsigned long i) const {
            return plugin->PortNames[pIdx[i]];
            }
      virtual const char* getParameterOutName(unsigned long i) const {
            return plugin->PortNames[poIdx[i]];
            }
      virtual float defaultValue(unsigned long) const;

      float convertGuiControlValue(unsigned long parameter, int val) const;

      virtual void* instantiate(int sampleRate);
      virtual void connectInport(void* handle, unsigned long k, void* datalocation);
      virtual void connectOutport(void* handle, unsigned long k, void* datalocation);
      virtual void connectCtrlInport(void* handle, unsigned long k, void* datalocation);
      virtual void connectCtrlOutport(void* handle, unsigned long k, void* datalocation);
      
      virtual void activate(void* handle) {
            if (plugin && plugin->activate)
                  plugin->activate((LADSPA_Handle)handle);
            }
      virtual void deactivate(void* handle) {
            if (plugin && plugin->deactivate)
                  plugin->deactivate((LADSPA_Handle)handle);
            }
      virtual void cleanup(void* handle) {
            if (plugin && plugin->cleanup)
                  plugin->cleanup((LADSPA_Handle)handle);
            }
      virtual void connectPort(void* handle, unsigned long port, float* datalocation) {
            if(plugin)
              plugin->connect_port((LADSPA_Handle)handle, port, datalocation);
            }
      virtual void apply(void* handle, unsigned long n) {
            if(plugin && plugin->run)
              plugin->run((LADSPA_Handle)handle, n);
            }
   };

class PluginI {
      Plugin* _plugin;
      int channel;
      int instances;
      int _id;

      LADSPA_Handle* handle; // per instance
      Port* controls;
      Port* controlsOut;
      Port* controlsOutDummy;

      unsigned long audioInPorts;
      unsigned long audioOutPorts;
      unsigned long controlPorts;
      unsigned long controlOutPorts;
      
      bool          _hasLatencyOutPort;
      unsigned long _latencyOutPort;

      float *_audioInSilenceBuf; // Just all zeros all the time, so we don't have to clear for silence.
      float *_audioOutDummyBuf;  // A place to connect unused outputs.
      
      bool _on;
      QString _name;
      QString _label;

      void init();

   public:
      PluginI();
      virtual ~PluginI();

      Plugin* plugin() const { return _plugin; }

      //virtual Plugin::PluginFeatures requiredFeatures() const { return _plugin->requiredFeatures(); }
      
      bool on() const        { return _on; }
      void setOn(bool val)   { _on = val; }

      unsigned long pluginID()      { return _plugin->id(); }
      void setID(int i);
      int id()                      { return _id; }

      bool inPlaceCapable() const { 
        if(!_plugin) return false; 
        return _plugin->inPlaceCapable();
        }

      // Returns true on error.
      bool initPluginInstance(Plugin* plug, int channels, 
                              int sampleRate, unsigned int segmentSize,
                              bool useDenormalBias, float denormalBias);
      void setChannels(int);
      // Runs the plugin for frames. Any ports involved must already be connected.
      void process(unsigned long frames);
      // Runs the plugin for frames. This automatically connects the given 
      //  data locations to the ports each time it is called.
      void apply(unsigned pos, unsigned long frames, unsigned long ports, float** bufIn, float** bufOut);

      // Connects ports with data locations. Multiple sources and destinations, with offset.
      void connect(unsigned long ports, unsigned long offset, float** src, float** dst);
//       // Connects a single audio input port to a data location. 
//       void connectInport(unsigned long k, void* datalocation);
//       // Connects a single audio output port to a data location. 
//       void connectOutport(unsigned long k, void* datalocation);
//       // Connects a single control parameter input port to a data location. 
//       void connectCtrlInport(unsigned long k, void* datalocation);
//       // Connects a single control parameter output port to a data location. 
//       void connectCtrlOutport(unsigned long k, void* datalocation);
      
      // Returns true on success.
      bool start();
      // Returns true on success.
      bool stop();
      
      bool isAudioIn(unsigned long k) {
            if(!_plugin) return false;
            return _plugin->isAudioIn(k);
            }
      
      bool isAudioOut(unsigned long k) {
            if(!_plugin) return false;
            return _plugin->isAudioOut(k);
            }
      
      bool isLog(unsigned long k) const {
            if(!_plugin) return false;
            return _plugin->isLog(k);
            }
      bool isBool(unsigned long k) const {
            if(!_plugin) return false;
            return _plugin->isBool(k);
            }
      bool isInt(unsigned long k) const {
            if(!_plugin) return false;
            return _plugin->isInt(k);
            }
      bool isLinear(unsigned long k) const {
            if(!_plugin) return false;
            return _plugin->isLinear(k);
            }
            
      void range(unsigned long i, float* min, float* max) const {
            if(!_plugin) return;
            _plugin->range(i, min, max);
            }
      void rangeOut(unsigned long i, float* min, float* max) const {
            if(!_plugin) return;
            _plugin->rangeOut(i, min, max);
            }
      const char* getParameterName(unsigned long i) const {
            if(!_plugin) return 0;
            return _plugin->getParameterName(i);
            }
      const char* getParameterOutName(unsigned long i) const {
            if(!_plugin) return 0;
            return _plugin->getParameterOutName(i);
            }
      float defaultValue(unsigned long i) const {
            if(!_plugin) return 0.0;
            return _plugin->defaultValue(i);
            }
      
      // Return true on success.
      bool activate();
      // Return true on success.
      bool deactivate();
      QString pluginLabel() const    { return _plugin->label(); }
      QString label() const          { return _label; }
      QString name() const           { return _name; }
      QString lib() const            { return _plugin->lib(); }
      QString dirPath() const        { return _plugin->dirPath(); }
      QString fileName() const       { return _plugin->fileName(); }

      bool setControl(const QString& s, float val);

      unsigned long inports() const           { return audioInPorts; }
      unsigned long outports() const          { return audioOutPorts; }
      unsigned long parameters() const        { return controlPorts; }
      unsigned long parametersOut() const     { return controlOutPorts; }
      
      void setParam(unsigned long i, float val);
      float param(unsigned long i) const { if(i >= controlPorts) return 0.0; return controls[i].val; }
      float paramOut(unsigned long i) const { if(i >= controlOutPorts) return 0.0; return controlsOut[i].val; }
      
      // Alias for getParameterName.
      const char* paramName(unsigned long i) const    { return getParameterName(i); }
      const char* paramOutName(unsigned long i) const { return getParameterOutName(i); }
      
      float latency() const; 
      //CtrlValueType ctrlValueType(unsigned long i) const { return _plugin->ctrlValueType(controls[i].idx); }
      //CtrlList::Mode ctrlMode(unsigned long i) const { return _plugin->ctrlMode(controls[i].idx); }
      
      int   getGuiControlValue(unsigned long parameter) const;
      float convertGuiControlValue(unsigned long parameter, int val) const;
      };

static inline float fast_log2 (float val)
      {
      /* don't use reinterpret_cast<> because that prevents this
         from being used by pure C code (for example, GnomeCanvasItems)
      */
      int* const exp_ptr = (int *)(&val);
      int x              = *exp_ptr;
      const int log_2    = ((x >> 23) & 255) - 128;
      x &= ~(255 << 23);
      x += 127 << 23;
      *exp_ptr = x;
      val = ((-1.0f/3) * val + 2) * val - 2.0f/3;   // (1)
      return (val + log_2);
      }

static inline float fast_log10 (const float val)
      {
      return fast_log2(val) / 3.312500f;
      }

//---------------------------------------------------------
//   PluginList
//---------------------------------------------------------

typedef std::list<Plugin*>::iterator iPlugin;

class PluginList : public std::list<Plugin*> {
   public:
      Plugin* find(const QString& file, const QString& name);
      PluginList() {}
      ~PluginList();
      };

extern void SS_initPlugins(const QString& globalLibPath);
extern PluginList plugins;

} // namespace MusESimplePlugin

#endif
