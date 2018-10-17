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

#include "plugin_scan.h"

#define SS_PLUGIN_PARAM_MIN                  0
#define SS_PLUGIN_PARAM_MAX                127

namespace MusESimplePlugin {
  
//---------------------------------------------------------
//   Port
//---------------------------------------------------------

struct Port {
      float _val;
      };

//---------------------------------------------------------
//   Plugin base class
//---------------------------------------------------------

class PluginI;

class Plugin
   {
   public:
     // Can be Or'd together.
     enum PluginFeature { NoFeatures=0x00, FixedBlockSize=0x01, PowerOf2BlockSize=0x02, NoInPlaceProcessing=0x04 };
     typedef int PluginFeaturesType;
     
   protected:
      QFileInfo _fi;
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

      PluginFeaturesType _requiredFeatures;
      
      std::vector<unsigned long> _pIdx; //control port numbers
      std::vector<unsigned long> _poIdx; //control out port numbers
      std::vector<unsigned long> _iIdx; //input port numbers
      std::vector<unsigned long> _oIdx; //output port numbers
      
   public:
      Plugin(const QFileInfo* f) 
        : _fi(*f), _libHandle(0), _references(0), _instNo(0), _uniqueID(0),
          _portCount(0),_inports(0), _outports(0),
          _controlInPorts(0),_controlOutPorts(0),
          _requiredFeatures(NoFeatures) { }
// REMOVE Tim. scan. Added..
//       Plugin(const MusECore::PluginScanInfo& info)
//         : _fi(info._fi), _libHandle(0), _references(0), _instNo(0),
//           _uniqueID(0), _portCount(0),_inports(0), _outports(0),
//           _controlInPorts(0), _controlOutPorts(0),
//           _requiredFeatures(NoFeatures) { }
      Plugin(const MusECore::PluginScanInfo& info)
        : _fi(info._fi), _libHandle(0), _references(0), _instNo(0),
          _uniqueID(info._uniqueID), _portCount(info._portCount), _inports(info._inports), _outports(info._outports),
          _controlInPorts(info._controlInPorts), _controlOutPorts(info._controlOutPorts),
          _requiredFeatures(info._requiredFeatures) { }
      virtual ~Plugin() {}
      
      //----------------------------------------------------
      // The following methods can be called regardless of
      //  whether the libray is open or not, ie. zero or more
      //  instances exist. The information is cached.
      //----------------------------------------------------
      
      // Returns features required by the plugin.
      PluginFeaturesType requiredFeatures() const { return _requiredFeatures; }
        
      // Create and initialize a plugin instance. Returns null if failure.
      // Equivalent to calling (new ***PlugI())->initPluginInstance(this, ...).
      // The returned type depends on the derived class (LadspaPluginI*, Lv2PluginI*, etc).
      // Caller is responsible for deleting the returned object.
      virtual PluginI* createPluginI(int chans, float sampleRate, unsigned int segmentSize,
                             bool useDenormalBias, float denormalBias) = 0;
      
      int references() const            { return _references; }
      virtual int incReferences(int)    { return _references; }
      int instNo()                      { return _instNo++;   }
      virtual void* instantiate(float /*sampleRate*/, void* /*data*/) { return 0; }

      QString label() const                        { return _label; }
      QString name() const                         { return _name; }
      unsigned long id() const                     { return _uniqueID; }
      QString maker() const                        { return _maker; }
      QString copyright() const                    { return _copyright; }
      QString lib(bool complete = true) const      { return complete ? _fi.completeBaseName() : _fi.baseName(); }
      QString dirPath(bool complete = true) const  { return complete ? _fi.absolutePath() : _fi.path(); }
      QString filePath() const                     { return _fi.filePath(); }
      QString fileName() const                     { return _fi.fileName(); }
      
      // Total number of ports.
      unsigned long portCount() const       { return _portCount; }
      unsigned long parameter() const       { return _controlInPorts; }
      unsigned long parameterOut() const    { return _controlOutPorts; }
      unsigned long inports() const         { return _inports;     }
      unsigned long outports() const        { return _outports;     }
      bool inPlaceCapable() const           { return _requiredFeatures & NoInPlaceProcessing; }

      
      //----------------------------------------------------
      // The following methods require the library be open,
      //  ie. at least one instance exists.
      //----------------------------------------------------
      
      virtual bool isAudioIn(unsigned long) const { return false; }
      virtual bool isAudioOut(unsigned long) const { return false; }
      virtual bool isParameterIn(unsigned long) const { return false; }
      virtual bool isParameterOut(unsigned long) const { return false; }
            
      virtual bool isLog(unsigned long) const         { return false; }
      virtual bool isBool(unsigned long) const        { return false; }
      virtual bool isInt(unsigned long) const         { return false; }
      virtual bool isLinear(unsigned long) const      { return false; }
      virtual float defaultValue(unsigned long) const { return 0.0f;  }
      virtual bool range(unsigned long, float /*sampleRate*/, float* min, float* max) const = 0;
      virtual bool rangeOut(unsigned long, float /*sampleRate*/, float* min, float* max) const = 0;
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
      const LADSPA_Descriptor* _plugin;
      
      // Accepts a master port index.
      bool port_range(unsigned long k, float sampleRate, float* min, float* max) const;
      
   public:
      LadspaPlugin(const QFileInfo* f, const LADSPA_Descriptor_Function, const LADSPA_Descriptor* d);
      LadspaPlugin(const MusECore::PluginScanInfo& info);
      virtual ~LadspaPlugin() { }

      // Create and initialize a LADSPA plugin instance. Returns null if failure.
      // Equivalent to calling (new LadspaPlugI())->initPluginInstance(this, ...).
      // The returned type depends on the this class (LadspaPluginI*, Lv2PluginI*, etc).
      // Caller is responsible for deleting the returned object.
      PluginI* createPluginI(int chans, float sampleRate, unsigned int segmentSize,
                             bool useDenormalBias, float denormalBias);

      int incReferences(int);
      
      bool isAudioIn(unsigned long k) const {
            if(!_plugin)
              return false;
            return (_plugin->PortDescriptors[k] & IS_AUDIO_IN) == IS_AUDIO_IN;
            }
      bool isAudioOut(unsigned long k) const {
            if(!_plugin)
              return false;
            return (_plugin->PortDescriptors[k] & IS_AUDIO_OUT) == IS_AUDIO_OUT;
            }
      bool isParameterIn(unsigned long k) const {
            if(!_plugin)
              return false;
            return (_plugin->PortDescriptors[k] & IS_PARAMETER_IN) == IS_PARAMETER_IN;
            }
      bool isParameterOut(unsigned long k) const {
            if(!_plugin)
              return false;
            return (_plugin->PortDescriptors[k] & IS_PARAMETER_OUT) == IS_PARAMETER_OUT;
            }
      
      bool isLog(unsigned long k) const {
            if(!_plugin)
              return false;
            LADSPA_PortRangeHint r = _plugin->PortRangeHints[_pIdx[k]];
            return LADSPA_IS_HINT_LOGARITHMIC(r.HintDescriptor);
            }
      bool isBool(unsigned long k) const {
            if(!_plugin)
              return false;
            return LADSPA_IS_HINT_TOGGLED(_plugin->PortRangeHints[_pIdx[k]].HintDescriptor);
            }
      bool isInt(unsigned long k) const {
            if(!_plugin)
              return false;
            LADSPA_PortRangeHint r = _plugin->PortRangeHints[_pIdx[k]];
            return LADSPA_IS_HINT_INTEGER(r.HintDescriptor);
            }
      bool isLinear(unsigned long k) const {
            if(!_plugin)
              return false;
            LADSPA_PortRangeHint r = _plugin->PortRangeHints[_pIdx[k]];
            return !LADSPA_IS_HINT_INTEGER(r.HintDescriptor) &&
                   !LADSPA_IS_HINT_LOGARITHMIC(r.HintDescriptor) &&
                   !LADSPA_IS_HINT_TOGGLED(r.HintDescriptor);
            }
      bool range(unsigned long k, float sampleRate, float*, float*) const;
      bool rangeOut(unsigned long k, float sampleRate, float*, float*) const;
      const char* getParameterName(unsigned long k) const {
            if(!_plugin)
              return 0;
            return _plugin->PortNames[_pIdx[k]];
            }
      const char* getParameterOutName(unsigned long k) const {
            if(!_plugin)
              return 0;
            return _plugin->PortNames[_poIdx[k]];
            }
      float defaultValue(unsigned long k) const;

      float convertGuiControlValue(unsigned long k, float sampleRate, int val) const;

      void* instantiate(float sampleRate, void* /*data*/);
      void connectInport(void* handle, unsigned long k, void* datalocation);
      void connectOutport(void* handle, unsigned long k, void* datalocation);
      void connectCtrlInport(void* handle, unsigned long k, void* datalocation);
      void connectCtrlOutport(void* handle, unsigned long k, void* datalocation);
      
      void activate(void* handle) {
            if (_plugin && _plugin->activate)
                  _plugin->activate((LADSPA_Handle)handle);
            }
      void deactivate(void* handle) {
            if (_plugin && _plugin->deactivate)
                  _plugin->deactivate((LADSPA_Handle)handle);
            }
      void cleanup(void* handle) {
            if (_plugin && _plugin->cleanup)
                  _plugin->cleanup((LADSPA_Handle)handle);
            }
      void connectPort(void* handle, unsigned long port, float* datalocation) {
            if(_plugin)
              _plugin->connect_port((LADSPA_Handle)handle, port, datalocation);
            }
      void apply(void* handle, unsigned long n) {
            if(_plugin && _plugin->run)
              _plugin->run((LADSPA_Handle)handle, n);
            }
   };

//--------------------------------
//  PluginI
//  Plugin Instance base class
//--------------------------------
      
class PluginI {
   private:
      void init();
      
   protected:
      Plugin* _plugin;
      // Some APIs may need to point to a float or double samplerate, or both (LV2).
      float _sampleRate;
      double _dSampleRate;
      unsigned int _segmentSize;
      int _channel;
      int _instances;
      int _id;

      Port* _controls;
      Port* _controlsOut;
      Port* _controlsOutDummy;

      unsigned long _audioInPorts;
      unsigned long _audioOutPorts;
      unsigned long _controlPorts;
      unsigned long _controlOutPorts;
      
      bool          _hasLatencyOutPort;
      unsigned long _latencyOutPort;

      float *_audioInSilenceBuf; // Just all zeros all the time, so we don't have to clear for silence.
      float *_audioOutDummyBuf;  // A place to connect unused outputs.
      
      bool _on;
      QString _name;
      QString _label;

   public:
      PluginI();
      virtual ~PluginI();

      Plugin* plugin() const { return _plugin; }

      Plugin::PluginFeaturesType requiredFeatures() const {
        if(!_plugin) return Plugin::NoFeatures; 
        return _plugin->requiredFeatures(); }
      
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
      virtual bool initPluginInstance(Plugin* plug, int channels, 
                              float sampleRate, unsigned int segmentSize,
                              bool useDenormalBias, float denormalBias) = 0;
      float sampleRate() const { return _sampleRate; }
      void setSampleRate(float rate) { _sampleRate = rate; _dSampleRate = rate; }
      unsigned int segmentSize() const { return _segmentSize; }
      int channels() const { return _channel; }
      virtual void setChannels(int chans) = 0;
      // Runs the plugin for frames. Any ports involved must already be connected.
      virtual void process(unsigned long frames) = 0;
      // Runs the plugin for frames. This automatically connects the given 
      //  data locations to the ports each time it is called.
      void apply(unsigned pos, unsigned long frames, unsigned long ports, float** bufIn, float** bufOut);

      // Connects ports with data locations. Multiple sources and destinations, with offset.
      virtual void connect(unsigned long ports, unsigned long offset, float** src, float** dst) = 0;
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
              _plugin->range(i, _sampleRate, min, max);
            }
      void rangeOut(unsigned long i, float* min, float* max) const {
            if(!_plugin) return;
              _plugin->rangeOut(i, _sampleRate, min, max);
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
      virtual bool activate() = 0;
      // Return true on success.
      virtual bool deactivate() = 0;
      QString pluginLabel() const    { return _plugin->label(); }
      QString label() const          { return _label; }
      QString name() const           { return _name; }
      QString lib() const            { return _plugin->lib(); }
      QString dirPath() const        { return _plugin->dirPath(); }
      QString fileName() const       { return _plugin->fileName(); }

      bool setControl(const QString& s, float val);

      unsigned long inports() const           { return _audioInPorts; }
      unsigned long outports() const          { return _audioOutPorts; }
      unsigned long parameters() const        { return _controlPorts; }
      unsigned long parametersOut() const     { return _controlOutPorts; }
      
      void setParam(unsigned long i, float val);
      float param(unsigned long i) const { if(i >= _controlPorts) return 0.0; return _controls[i]._val; }
      float paramOut(unsigned long i) const { if(i >= _controlOutPorts) return 0.0; return _controlsOut[i]._val; }
      
      // Alias for getParameterName.
      const char* paramName(unsigned long i) const    { return getParameterName(i); }
      const char* paramOutName(unsigned long i) const { return getParameterOutName(i); }
      
      float latency() const; 
      //CtrlValueType ctrlValueType(unsigned long i) const { return _plugin->ctrlValueType(controls[i].idx); }
      //CtrlList::Mode ctrlMode(unsigned long i) const { return _plugin->ctrlMode(controls[i].idx); }
      
      int   getGuiControlValue(unsigned long parameter) const;
      float convertGuiControlValue(unsigned long parameter, int val) const;
      };

      
//--------------------------------
//  LadspaPluginI
//  Ladspa Plugin Instance class
//--------------------------------
      
class LadspaPluginI : public PluginI {
   private:
      LADSPA_Handle* _handle; // per instance
      void init();

   public:
      LadspaPluginI();
      virtual ~LadspaPluginI();

      // Returns true on error.
      bool initPluginInstance(Plugin* plug, int channels, 
                              float sampleRate, unsigned int segmentSize,
                              bool useDenormalBias, float denormalBias);
      void setChannels(int chans);
      // Runs the plugin for frames. Any ports involved must already be connected.
      void process(unsigned long frames);

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
      
      // Return true on success.
      bool activate();
      // Return true on success.
      bool deactivate();
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
