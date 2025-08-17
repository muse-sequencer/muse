//===================================================================
//  MusE
//  Linux Music Editor
//
//  vst_native.cpp
//  (C) Copyright 2012-2013 Tim E. Real (terminator356 on users dot sourceforge dot net)
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
//===================================================================

#include "config.h"

#ifdef VST_NATIVE_SUPPORT

#include <QDir>
#include <QMenu>

#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include "muse_math.h"
#include <set>
#include <string>
#include <jack/jack.h>
#include <sstream>

#include "globals.h"
#include "gconfig.h"
#include "audio.h"
#include "synth.h"
#include "jackaudio.h"
#include "midi_consts.h"
#include "xml.h"
#include "plugin.h"
#include "popupmenu.h"
#include "pos.h"
#include "tempo.h"
#include "sync.h"
#include "sig.h"
#include "minstrument.h"
#include "hex_float.h"
#include "song.h"

#include "vst_native.h"
#include "pluglist.h"

#define OLD_PLUGIN_ENTRY_POINT "main"
#define NEW_PLUGIN_ENTRY_POINT "VSTPluginMain"

// Enable debugging messages
//#define VST_NATIVE_DEBUG
//#define VST_NATIVE_DEBUG_PROCESS
// For debugging output: Uncomment the fprintf section.
#define DEBUG_PARAMS(dev, format, args...) // fprintf(dev, format, ##args);

#ifdef VST_VESTIGE_SUPPORT
#ifndef effGetProgramNameIndexed
#define effGetProgramNameIndexed 29
#endif
#ifndef effFlagsProgramChunks
#define effFlagsProgramChunks 32
#endif
#ifndef effGetChunk
#define effGetChunk 23
#endif
#ifndef effSetChunk
#define effSetChunk 24
#endif
#ifndef effCanBeAutomated
#define effCanBeAutomated 26
#endif
#ifndef effSetBypass
#define effSetBypass 44
#endif
#ifndef effStartProcess
#define effStartProcess 71
#endif
#ifndef effStopProcess
#define effStopProcess 72
#endif
#endif

namespace MusECore {

extern JackAudioDevice* jackAudio;

static VstIntPtr currentPluginId = 0;
static sem_t _vstIdLock;

//-----------------------------------------------------------------------------------------
//   vstHostCallback
//   This must be a function, it cannot be a class method so we dispatch to various objects from here.
//-----------------------------------------------------------------------------------------

VstIntPtr VSTCALLBACK vstNativeHostCallback(AEffect* effect, VstInt32 opcode, VstInt32 index, VstIntPtr value, void* ptr, float opt)
{
      // Is this callback for an actual instance? Hand-off to the instance if so.
      //VSTPlugin* plugin;
      if(effect && effect->user)
      {
        VstNativeSynthOrPlugin *userData = (VstNativeSynthOrPlugin*)(effect->user);
        //return ((VstNativeSynthIF*)plugin)->hostCallback(opcode, index, value, ptr, opt);
        return VstNativeSynth::pluginHostCallback(userData, opcode, index, value, ptr, opt);

      }

      // No instance found. So we are just scanning for plugins...
    
#ifdef VST_NATIVE_DEBUG      
      fprintf(stderr, "vstNativeHostCallback eff:%p opcode:%d\n", effect, opcode);
#endif      
      
      switch (opcode) {
            case audioMasterAutomate:
                  // index, value, returns 0
                  return 0;

            case audioMasterVersion:
                  // vst version, currently 2 (0 for older)
                  return 2300;

            case audioMasterCurrentId:
                  // returns the unique id of a plug that's currently
                  // loading
                  return currentPluginId;

            case audioMasterIdle:
                  // call application idle routine (this will
                  // call effEditIdle for all open editors too)
                  return 0;

            case audioMasterGetTime:
                  // returns const VstTimeInfo* (or 0 if not supported)
                  // <value> should contain a mask indicating which fields are required
                  // (see valid masks above), as some items may require extensive
                  // conversions
                  return 0;

            case audioMasterProcessEvents:
                  // VstEvents* in <ptr>
                  return 0;

            case audioMasterIOChanged:
                   // numInputs and/or numOutputs has changed
                  return 0;

            case audioMasterSizeWindow:
                  // index: width, value: height
                  return 0;

            case audioMasterGetSampleRate:
                  return MusEGlobal::sampleRate;

            case audioMasterGetBlockSize:
                  return MusEGlobal::segmentSize;

            case audioMasterGetInputLatency:
                  return 0;

            case audioMasterGetOutputLatency:
                  return 0;

            case audioMasterGetCurrentProcessLevel:
                  // returns: 0: not supported,
                  // 1: currently in user thread (gui)
                  // 2: currently in audio thread (where process is called)
                  // 3: currently in 'sequencer' thread (midi, timer etc)
                  // 4: currently offline processing and thus in user thread
                  // other: not defined, but probably pre-empting user thread.
                  return 0;

            case audioMasterGetAutomationState:
                  // returns 0: not supported, 1: off, 2:read, 3:write, 4:read/write
                  // offline
                  return 0;

            case audioMasterOfflineStart:
            case audioMasterOfflineRead:
                   // ptr points to offline structure, see below. return 0: error, 1 ok
                  return 0;

            case audioMasterOfflineWrite:
                  // same as read
                  return 0;

            case audioMasterOfflineGetCurrentPass:
            case audioMasterOfflineGetCurrentMetaPass:
                  return 0;

            case audioMasterGetVendorString:
                  // fills <ptr> with a string identifying the vendor (max 64 char)
                  strcpy ((char*) ptr, "MusE");
                  return 1;

            case audioMasterGetProductString:
                  // fills <ptr> with a string with product name (max 64 char)
                  strcpy ((char*) ptr, "NativeVST");
                  return 1;

            case audioMasterGetVendorVersion:
                  // returns vendor-specific version
                  return 2000;

            case audioMasterVendorSpecific:
                  // no definition, vendor specific handling
                  return 0;

            case audioMasterCanDo:
                  // string in ptr, see below
                  return 0;

            case audioMasterGetLanguage:
                  // see enum
                  return kVstLangEnglish;

            case audioMasterGetDirectory:
                  // get plug directory, FSSpec on MAC, else char*
                  return 0;

            case audioMasterUpdateDisplay:
                  // something has changed, update 'multi-fx' display
                  return 0;

            case audioMasterBeginEdit:
                  // begin of automation session (when mouse down), parameter index in <index>
                  return 0;

            case audioMasterEndEdit:
                  // end of automation session (when mouse up),     parameter index in <index>
                  return 0;

            case audioMasterOpenFileSelector:
                  // open a fileselector window with VstFileSelect* in <ptr>
                  return 0;

            case audioMasterCloseFileSelector:
                  return 0;
                  
#ifdef VST_FORCE_DEPRECATED
#ifndef VST_2_4_EXTENSIONS // deprecated in 2.4

            case audioMasterGetSpeakerArrangement:
                  // (long)input in <value>, output in <ptr>
                  return 0;

            case audioMasterPinConnected:
                  // inquire if an input or output is being connected;
                  // index enumerates input or output counting from zero:
                  // value is 0 for input and != 0 otherwise. note: the
                  // return value is 0 for <true> such that older versions
                  // will always return true.
                  //return 1;
                  return 0;

            // VST 2.0 opcodes...
            case audioMasterWantMidi:
                  // <value> is a filter which is currently ignored
                  return 0;

            case audioMasterSetTime:
                  // VstTimenfo* in <ptr>, filter in <value>, not supported
                  return 0;

            case audioMasterTempoAt:
                  // returns tempo (in bpm * 10000) at sample frame location passed in <value>
                  return 0;  // TODO:

            case audioMasterGetNumAutomatableParameters:
                  return 0;

            case audioMasterGetParameterQuantization:
                     // returns the integer value for +1.0 representation,
                   // or 1 if full single float precision is maintained
                     // in automation. parameter index in <value> (-1: all, any)
                  //return 0;
                  return 1;

            case audioMasterNeedIdle:
                   // plug needs idle calls (outside its editor window)
                  return 0;

            case audioMasterGetPreviousPlug:
                   // input pin in <value> (-1: first to come), returns cEffect*
                  return 0;

            case audioMasterGetNextPlug:
                   // output pin in <value> (-1: first to come), returns cEffect*
                  return 0;

            case audioMasterWillReplaceOrAccumulate:
                   // returns: 0: not supported, 1: replace, 2: accumulate
                  //return 0;
                  return 1;

            case audioMasterSetOutputSampleRate:
                  // for variable i/o, sample rate in <opt>
                  return 0;

            case audioMasterSetIcon:
                  // void* in <ptr>, format not defined yet
                  return 0;

            case audioMasterOpenWindow:
                  // returns platform specific ptr
                  return 0;

            case audioMasterCloseWindow:
                  // close window, platform specific handle in <ptr>
                  return 0;
#endif
#endif
                  
            default:
                  break;
            }

      if(MusEGlobal::debugMsg)
        fprintf(stderr, "  unknown opcode\n");

      return 0;
      }

//---------------------------------------------------------
//   initVST_Native
//---------------------------------------------------------

void initVST_Native()
{
#ifdef VST_NATIVE_SUPPORT
  #ifdef VST_VESTIGE_SUPPORT
    printf("Initializing Native VST support. Using VESTIGE compatibility implementation.\n");
  #else
    printf("Initializing Native VST support. Using Steinberg VSTSDK.\n");
  #endif
#endif
  sem_init(&_vstIdLock, 0, 1);
      
  const char* message = "scanVstNativeLib: ";
  const MusEPlugin::PluginScanList& scan_list = MusEPlugin::pluginList;
  for(MusEPlugin::ciPluginScanList isl = scan_list.begin(); isl != scan_list.end(); ++isl)
  {
    const MusEPlugin::PluginScanInfoRef inforef = *isl;
    const MusEPlugin::PluginScanInfoStruct& info = inforef->info();
    switch(info._type)
    {
      case MusEPlugin::PluginTypeLinuxVST:
      {
#ifdef VST_NATIVE_SUPPORT
        if(MusEGlobal::loadNativeVST)
        {
          const QString inf_cbname = PLUGIN_GET_QSTRING(info._completeBaseName);
          const QString inf_name   = PLUGIN_GET_QSTRING(info._name);
          const QString inf_label  = PLUGIN_GET_QSTRING(info._label);
          const QString inf_uri    = PLUGIN_GET_QSTRING(info._uri);
          const Plugin* plug_found = MusEGlobal::plugins.find(
            info._type,
            inf_cbname,
            inf_uri,
            inf_label);
          const Synth* synth_found = MusEGlobal::synthis.find(
            info._type,
            inf_cbname,
            inf_uri,
            inf_label);

          if(plug_found)
          {
            fprintf(stderr, "Ignoring LinuxVST effect name:%s uri:%s path:%s duplicate of path:%s\n",
                    inf_name.toLocal8Bit().constData(),
                    inf_uri.toLocal8Bit().constData(),
                    PLUGIN_GET_QSTRING(info.filePath()).toLocal8Bit().constData(),
                    plug_found->filePath().toLocal8Bit().constData());
          }
          if(synth_found)
          {
            fprintf(stderr, "Ignoring LinuxVST synth name:%s uri:%s path:%s duplicate of path:%s\n",
                    inf_name.toLocal8Bit().constData(),
                    inf_uri.toLocal8Bit().constData(),
                    PLUGIN_GET_QSTRING(info.filePath()).toLocal8Bit().constData(),
                    synth_found->filePath().toLocal8Bit().constData());
          }
          
          const bool is_effect = info._class & MusEPlugin::PluginClassEffect;
          const bool is_synth  = info._class & MusEPlugin::PluginClassInstrument;
          
          const bool add_plug  = (is_effect || is_synth) &&
                                 info._inports > 0 && info._outports > 0 &&
                                 !plug_found;
                                 
          // For now we allow effects as a synth track. Until we allow programs (and midi) in the effect rack.
          const bool add_synth = (is_synth || is_effect) && !synth_found;
                                 
          if(add_plug || add_synth)
          {
            VstNativeSynth* new_synth = new VstNativeSynth(info);

            if(add_synth)
            {
              if(MusEGlobal::debugMsg)
                fprintf(stderr, "scanVstNativeLib: adding vst synth plugin:%s name:%s effectName:%s vendorString:%s productString:%s vstver:%d\n",
                        PLUGIN_GET_QSTRING(info.filePath()).toLocal8Bit().constData(),
                        inf_cbname.toLocal8Bit().constData(),
                        inf_name.toLocal8Bit().constData(),
                        PLUGIN_GET_QSTRING(info._maker).toLocal8Bit().constData(),
                        PLUGIN_GET_QSTRING(info._description).toLocal8Bit().constData(),
                        info._apiVersionMajor
                        );

              MusEGlobal::synthis.push_back(new_synth);
            }

            if(add_plug)
            {
              if(MusEGlobal::debugMsg)
                PluginBase::dump(info, message);
              MusEGlobal::plugins.push_back( new VstNativePluginWrapper(new_synth, info._requiredFeatures) );
            }
          }
        }
#endif
      }
      break;
      
      case MusEPlugin::PluginTypeLADSPA:
      case MusEPlugin::PluginTypeDSSIVST:
      case MusEPlugin::PluginTypeDSSI:
      case MusEPlugin::PluginTypeVST:
      case MusEPlugin::PluginTypeLV2:
      case MusEPlugin::PluginTypeMESS:
      case MusEPlugin::PluginTypeMETRONOME:
      case MusEPlugin::PluginTypeUnknown:
      case MusEPlugin::PluginTypeNone:
      break;
    }
  }
}


//---------------------------------------------------------
//   VstNativeSynth
//---------------------------------------------------------

VstNativeSynth::VstNativeSynth(const MusEPlugin::PluginScanInfoStruct& info)
  : Synth(info)
{
  _id = info._uniqueID;
  _hasGui = info._pluginFlags & MusEPlugin::PluginHasGui;
  _inports = info._inports;
  _outports = info._outports;
  _controlInPorts = info._controlInPorts;
  _hasChunks = info._pluginFlags & MusEPlugin::PluginHasChunks;
  _vst_version = info._apiVersionMajor;
  _flags = info._vstPluginFlags;
  _usesTransportSource = info._vstPluginFlags & MusEPlugin::canReceiveVstTimeInfo;
}

//---------------------------------------------------------
//   reference
//---------------------------------------------------------

bool VstNativeSynth::reference()
{
  if(_references == 0)
  {
    _qlib.setFileName(filePath());
    // Same as dlopen RTLD_NOW.
    _qlib.setLoadHints(QLibrary::ResolveAllSymbolsHint);
    if(!_qlib.load())
    {
      fprintf(stderr, "VstNativeSynth::reference: load (%s) failed: %s\n",
        _qlib.fileName().toLocal8Bit().constData(), _qlib.errorString().toLocal8Bit().constData());
      return false;
    }
  }

  ++_references;

  return true;
}

//---------------------------------------------------------
//   release
//---------------------------------------------------------

int VstNativeSynth::release()
{
  if(_references == 1)
  {
    // Attempt to unload the library.
    // It will remain loaded if the plugin has shell plugins still in use or there are other references.
    const bool ulres = _qlib.unload();
    // Dummy usage stops unused warnings.
    (void)ulres;
    #ifdef VST_NATIVE_DEBUG
    fprintf(stderr, "VstNativeSynth::release(): No more instances. Result of unloading library %s: %d\n",
      _qlib.fileName().toLocal8Bit().constData(), ulres);
    #endif

    iIdx.clear();
    oIdx.clear();
    rpIdx.clear();
    midiCtl2PortMap.clear();
    port2MidiCtlMap.clear();
  }
  if(_references > 0)
    --_references;
  return _references;
}

//---------------------------------------------------------
//   instantiate
//---------------------------------------------------------

AEffect* VstNativeSynth::instantiate(void* userData)
{
  if(!reference())
    return nullptr;

  AEffect *(*getInstance)(audioMasterCallback);
  getInstance = (AEffect*(*)(audioMasterCallback))_qlib.resolve(NEW_PLUGIN_ENTRY_POINT);
  if(!getInstance)
  {
    if(MusEGlobal::debugMsg)
    {
      fprintf(stderr, "VST 2.4 entrypoint \"" NEW_PLUGIN_ENTRY_POINT "\" not found in library %s, looking for \""
                      OLD_PLUGIN_ENTRY_POINT "\"\n", _qlib.fileName().toLocal8Bit().constData());
    }

    getInstance = (AEffect*(*)(audioMasterCallback))_qlib.resolve(OLD_PLUGIN_ENTRY_POINT);
    if(!getInstance)
    {
      fprintf(stderr, "ERROR: VST entrypoints \"" NEW_PLUGIN_ENTRY_POINT "\" or \""
                      OLD_PLUGIN_ENTRY_POINT "\" not found in library\n");
      release();
      return nullptr;
    }
    else if(MusEGlobal::debugMsg)
    {
      fprintf(stderr, "VST entrypoint \"" OLD_PLUGIN_ENTRY_POINT "\" found\n");
    }
  }
  else if(MusEGlobal::debugMsg)
  {
    fprintf(stderr, "VST entrypoint \"" NEW_PLUGIN_ENTRY_POINT "\" found\n");
  }


  sem_wait(&_vstIdLock);

  currentPluginId = _id;

  AEffect *plugin = getInstance(vstNativeHostCallback);

  sem_post(&_vstIdLock);
  if(!plugin)
  {
    fprintf(stderr, "ERROR: Failed to instantiate plugin in VST library \"%s\"\n", _qlib.fileName().toLocal8Bit().constData());
    release();
    return nullptr;
  }
  else if(MusEGlobal::debugMsg)
    fprintf(stderr, "plugin instantiated\n");

  if(plugin->magic != kEffectMagic)
  {
    fprintf(stderr, "Not a VST plugin in library \"%s\"\n", _qlib.fileName().toLocal8Bit().constData());
    release();
    return nullptr;
  }
  else if(MusEGlobal::debugMsg)
    fprintf(stderr, "plugin is a VST\n");

  if(!(plugin->flags & effFlagsHasEditor))
  {
    if(MusEGlobal::debugMsg)
      fprintf(stderr, "Plugin has no GUI\n");
  }
  else if(MusEGlobal::debugMsg)
    fprintf(stderr, "Plugin has a GUI\n");

  if(!(plugin->flags & effFlagsCanReplacing))
    fprintf(stderr, "Plugin does not support processReplacing\n");
  else if(MusEGlobal::debugMsg)
    fprintf(stderr, "Plugin supports processReplacing\n");

  plugin->user = userData;

  return plugin;
}

//---------------------------------------------------------
//   openPlugin
//   static
//---------------------------------------------------------

bool VstNativeSynth::openPlugin(AEffect* plugin)
{
  plugin->dispatcher(plugin, effOpen, 0, 0, nullptr, 0);
  //plugin->dispatcher(plugin, effSetProgram, 0, 0, nullptr, 0.0f); // REMOVE Tim. Or keep?
  return true;
}

//---------------------------------------------------------
//   createSIF
//---------------------------------------------------------

SynthIF* VstNativeSynth::createSIF(SynthI* s)
      {
      VstNativeSynthIF* sif = new VstNativeSynthIF(s);
      if(!sif->init(this))
      {
          delete sif;
          sif = nullptr;
      }

      return sif;
      }

//---------------------------------------------------------
//   VstNativeSynthIF
//---------------------------------------------------------

VstNativeSynthIF::VstNativeSynthIF(SynthI* s) : SynthIF(s)
{
      _guiVisible = false;
      _gw = nullptr;
      _synth = nullptr;
      _plugin = nullptr;
      _editor = nullptr;
      _inProcess = false;
       _controls = nullptr;
      _audioInBuffers = nullptr;
      _audioInSilenceBuf = nullptr;
      _audioOutBuffers = nullptr;
      userData.pstate = 0;
      userData.sif = this;
      _transportLatencyCorr = 0.0f;
      // For now we are not expecting a lot of traffic here. Try 256.
      // (An exception might be if the plugin sends us automation begin or end for all of its parameters at once.)
      _ipcCallbackEvents = new LockFreeMPSCRingBuffer<PluginCallbackEventStruct>(256);
}

VstNativeSynthIF::~VstNativeSynthIF()
{
  // Just in case it wasn't removed or deactivate3 wasn't called etc...
  if(_plugin)
    fprintf(stderr, "ERROR: ~VstNativeSynthIF: _plugin is not NULL!\n");
  
  if(_audioOutBuffers)
  {
    unsigned long op = _synth->outPorts();
    for(unsigned long i = 0; i < op; ++i)
    {
      if(_audioOutBuffers[i])
        free(_audioOutBuffers[i]);
    }
    delete[] _audioOutBuffers;
  }

  if(_audioInBuffers)
  {
    unsigned long ip = _synth->inPorts();
    for(unsigned long i = 0; i < ip; ++i)
    {
      if(_audioInBuffers[i])
        free(_audioInBuffers[i]);
    }
    delete[] _audioInBuffers;
  }

  if(_audioInSilenceBuf)
    free(_audioInSilenceBuf);
    
  if(_controls)
    delete[] _controls;

  if(_gw)
    delete[] _gw;

  if(_ipcCallbackEvents)
    delete _ipcCallbackEvents;
}

//---------------------------------------------------------
//   init
//---------------------------------------------------------

bool VstNativeSynthIF::init(VstNativeSynth* s)
      {
      _synth = s;
      _plugin = _synth->instantiate(&userData);
      if(!_plugin)
        return false;

      if(!_synth->openPlugin(_plugin))
        return false;

      queryPrograms();
      
      unsigned long outports = _synth->outPorts();
      if(outports != 0)
      {
        _audioOutBuffers = new float*[outports];
        for(unsigned long k = 0; k < outports; ++k)
        {
#ifdef _WIN32
          _audioOutBuffers[k] = (float *) _aligned_malloc(16, sizeof(float *) * MusEGlobal::segmentSize);
          if(_audioOutBuffers[k] == nullptr)
          {
             fprintf(stderr, "ERROR: VstNativeSynthIF::init: _aligned_malloc returned error: NULL. Aborting!\n");
             abort();
          }
#else
          int rv = posix_memalign((void**)&_audioOutBuffers[k], 16, sizeof(float) * MusEGlobal::segmentSize);
          if(rv != 0)
          {
            fprintf(stderr, "ERROR: VstNativeSynthIF::init: posix_memalign returned error:%d. Aborting!\n", rv);
            abort();
          }
#endif
          if(MusEGlobal::config.useDenormalBias)
          {
            for(unsigned q = 0; q < MusEGlobal::segmentSize; ++q)
              _audioOutBuffers[k][q] = MusEGlobal::denormalBias;
          }
          else
            memset(_audioOutBuffers[k], 0, sizeof(float) * MusEGlobal::segmentSize);
        }
      }

      unsigned long inports = _synth->inPorts();
      if(inports != 0)
      {
        _audioInBuffers = new float*[inports];
        for(unsigned long k = 0; k < inports; ++k)
        {
#ifdef _WIN32
          _audioInBuffers[k] = (float *) _aligned_malloc(16, sizeof(float *) * MusEGlobal::segmentSize);
          if(_audioInBuffers[k] == nullptr)
          {
             fprintf(stderr, "ERROR: VstNativeSynthIF::init: _aligned_malloc returned error: NULL. Aborting!\n");
             abort();
          }
#else
          int rv = posix_memalign((void**)&_audioInBuffers[k], 16, sizeof(float) * MusEGlobal::segmentSize);
          if(rv != 0)
          {
            fprintf(stderr, "ERROR: VstNativeSynthIF::init: posix_memalign returned error:%d. Aborting!\n", rv);
            abort();
          }
#endif
          if(MusEGlobal::config.useDenormalBias)
          {
            for(unsigned q = 0; q < MusEGlobal::segmentSize; ++q)
              _audioInBuffers[k][q] = MusEGlobal::denormalBias;
          }
          else
            memset(_audioInBuffers[k], 0, sizeof(float) * MusEGlobal::segmentSize);
        }
        
#ifdef _WIN32
        _audioInSilenceBuf = (float *) _aligned_malloc(16, sizeof(float *) * MusEGlobal::segmentSize);
        if(_audioInSilenceBuf == nullptr)
        {
           fprintf(stderr, "ERROR: VstNativeSynthIF::init: _aligned_malloc returned error: NULL. Aborting!\n");
           abort();
        }
#else
        int rv = posix_memalign((void**)&_audioInSilenceBuf, 16, sizeof(float) * MusEGlobal::segmentSize);
        if(rv != 0)
        {
          fprintf(stderr, "ERROR: VstNativeSynthIF::init: posix_memalign returned error:%d. Aborting!\n", rv);
          abort();
        }
#endif
        if(MusEGlobal::config.useDenormalBias)
        {
          for(unsigned q = 0; q < MusEGlobal::segmentSize; ++q)
            _audioInSilenceBuf[q] = MusEGlobal::denormalBias;
        }
        else
          memset(_audioInSilenceBuf, 0, sizeof(float) * MusEGlobal::segmentSize);
      }

      _controls = nullptr;
      _gw = nullptr;
      unsigned long controlPorts = _synth->inControls();
      if(controlPorts != 0)
      {
        _controls = new Port[controlPorts];
        _gw       = new VstNativeGuiWidgets[controlPorts];
      }

      for(unsigned long i = 0; i < controlPorts; ++i)
      {
        _gw[i].pressed = false;
        
        _controls[i].idx = i;
        _controls[i].val    = _plugin->getParameter(_plugin, i);
        _controls[i].enCtrl  = true;

        // Support a special block for synth ladspa controllers.
        // Put the ID at a special block after plugins (far after).
        int id = genACnum(MusECore::MAX_PLUGINS, i);
        CtrlList* cl;
        CtrlListList* cll = track()->controller();
        iCtrlList icl = cll->find(id);
        if (icl == cll->end())
        {
          cl = new CtrlList(id);
          cll->add(cl);
          cl->setCurVal(_plugin->getParameter(_plugin, i));
        }
        else
        {
          cl = icl->second;
          const double v = cl->curVal();
          _controls[i].val = v;

          if(dispatch(effCanBeAutomated, i, 0, nullptr, 0.0f) == 1)
          {
            if(v != _plugin->getParameter(_plugin, i))
              _plugin->setParameter(_plugin, i, v);
          }

  #ifdef VST_NATIVE_DEBUG
          else  
            fprintf(stderr, "VstNativeSynthIF::init %s parameter:%lu cannot be automated\n",
                    name().toLocal8Bit().constData(), i);
  #endif
        }
        
        setupController(cl);
      }
      
      activate();     
      return true;
      }

//---------------------------------------------------------
//   resizeEditor
//---------------------------------------------------------

bool VstNativeSynth::resizeEditor(MusEGui::VstNativeEditor *editor, int w, int h)
{
    if(!editor || w <= 0 || h <= 0)
        return false;

    if (editor->fixScaling() && editor->devicePixelRatio() >= 1.0) {
        w = qRound((qreal)w / editor->devicePixelRatio());
        h = qRound((qreal)h / editor->devicePixelRatio());
    }

    editor->setFixedSize(w, h);
    return true;
}

// Static
void VstNativeSynth::guiUpdateWindowTitle(VstNativeSynthOrPlugin *userData)
{
  if(userData)
  {
    if(userData->pstate && userData->pstate->pluginI && userData->pstate->pluginI->track() && userData->pstate->editor)
    {
      const QString newtitle = userData->pstate->pluginI->track()->displayName() + userData->pstate->pluginI->pluginName();
      userData->pstate->editor->updateWindowTitle(newtitle);
    }
    else if(userData->sif && userData->sif->_editor)
    {
      const QString newtitle = userData->sif->displayName();
      userData->sif->_editor->updateWindowTitle(newtitle);
    }
  }
}

//---------------------------------------------------------
//   vstconfGetCustomData
//---------------------------------------------------------

QString VstNativeSynth::vstconfGetCustomData(AEffect *plugin)
{
   if(hasChunks())
   {
      //---------------------------------------------
      // dump current state of synth
      //---------------------------------------------
      unsigned long len = 0;
      void* p = 0;
      len = plugin->dispatcher(plugin, effGetChunk, 0, 0, &p, 0.0);
      if (len)
      {
         QByteArray arrOut = QByteArray::fromRawData((char *)p, len);

         // Weee! Compression!
         QByteArray outEnc64 = qCompress(arrOut).toBase64();

         QString customData(outEnc64);
         for (int pos=0; pos < customData.size(); pos+=150)
         {
            customData.insert(pos++,'\n'); // add newlines for readability
         }
         return customData;
      }
   }
   return QString();
}

//---------------------------------------------------------
//   vstconfSet
//---------------------------------------------------------

bool VstNativeSynth::vstconfSet(AEffect *plugin, const std::vector<QString> &customParams)
{
   if(customParams.size() == 0)
      return false;

   if(!hasChunks())
   {
      return false;
   }

   bool hasCustomData = false;
   for(size_t i = 0; i < customParams.size(); i++)
   {
      QString param = customParams [i];
      param.remove('\n'); // remove all linebreaks that may have been added to prettyprint the songs file
      QByteArray paramIn;
      paramIn.append(param.toUtf8());
      // Try to uncompress the data.
      QByteArray dec64 = qUncompress(QByteArray::fromBase64(paramIn));
      // Failed? Try uncompressed.
      if(dec64.isEmpty())
        dec64 = QByteArray::fromBase64(paramIn);
      
      plugin->dispatcher(plugin, effSetChunk, 0, dec64.size(), (void*)dec64.data(), 0.0); // index 0: is bank 1: is program
      hasCustomData = true;
      break; //one customData tag includes all data in base64
   }
   return hasCustomData;
}

void VstNativeSynth::setPluginEnabled(AEffect *plugin, bool en)
{
   plugin->dispatcher(plugin, effSetBypass, 0, !en, nullptr, 0.0f);
}

//---------------------------------------------------------
//   hostCallback
//---------------------------------------------------------

VstIntPtr VstNativeSynth::pluginHostCallback(VstNativeSynthOrPlugin *userData, VstInt32 opcode, VstInt32 index, VstIntPtr value, void* ptr, float opt)
{
   static VstTimeInfo _timeInfo;

#ifdef VST_NATIVE_DEBUG
   if(opcode != audioMasterGetTime)
   {
     QString name;
     if(userData->sif)
        name = userData->sif->pluginName();
     else if(userData->pstate)
        name = userData->pstate->pluginI->pluginName();
     fprintf(stderr, "VstNativeSynth::hostCallback %s opcode:%d\n", name.toLocal8Bit().constData(), opcode);
   }
#endif

   switch (opcode) {
   case audioMasterAutomate:
      DEBUG_PARAMS(stderr, "VstNativeSynth::pluginHostCallback audioMasterAutomate index:%d\n", index);
      VstNativeSynth::guiControlChanged(userData, index, opt);
      return 0;

   case audioMasterVersion:
      // vst version, currently 2 (0 for older)
      return 2300;

   case audioMasterCurrentId:
   {
      // returns the unique id of a plug that's currently
      // loading
      ///return 0;
      AEffect *vstPlug = 0;
      if(userData->sif)
         vstPlug = userData->sif->_plugin;
      else if(userData->pstate)
         vstPlug = userData->pstate->plugin;
      return vstPlug->uniqueID;
   }

   case audioMasterIdle:
   {
      // call application idle routine (this will
      // call effEditIdle for all open editors too)
      //_plugin->updateParamValues(false);
      if(userData->sif)
        userData->sif->idleEditor();
      else if(userData->pstate)
        userData->pstate->idleEditor();
      return 0;
   }

   case audioMasterGetTime:
   {
      // returns const VstTimeInfo* (or 0 if not supported)
      // <value> should contain a mask indicating which fields are required
      // (see valid masks above), as some items may require extensive
      // conversions

      // FIXME TODO: Optimizations: This may be called many times in one process call
      //              due to our multi-run slices. Some of the (costly) info will be redundant.
      //             So try to add some flag to try to only call some or all of this once per cycle.

#ifdef VST_NATIVE_DEBUG_PROCESS
      fprintf(stderr, "VstNativeSynth::hostCallback master time: valid: nanos:%d ppqpos:%d tempo:%d bars:%d cyclepos:%d sig:%d smpte:%d clock:%d\n",
              (bool)(value & kVstNanosValid),
              (bool)(value & kVstPpqPosValid),
              (bool)(value & kVstTempoValid),
              (bool)(value & kVstBarsValid),
              (bool)(value & kVstCyclePosValid),
              (bool)(value & kVstTimeSigValid),
              (bool)(value & kVstSmpteValid),
              (bool)(value & kVstClockValid));
#endif
      memset(&_timeInfo, 0, sizeof(_timeInfo));

      const bool extsync = MusEGlobal::extSyncFlag;
      unsigned int cur_frame = MusEGlobal::audio->pos().frame();
      unsigned int cur_tick = MusEGlobal::audio->tickPos();

      float latency_corr = 0.0f;
      if(userData->sif)
        latency_corr = userData->sif->transportLatencyCorr();
      else if(userData->pstate) // TODO Plugin midi
        latency_corr = userData->pstate->_latency_corr;

      unsigned int lat_offset = 0;

      //--------------------------------------------------------------------
      // Account for the latency correction and/or compensation.
      //--------------------------------------------------------------------
      // TODO How to handle when external sync is on. For now, don't try to correct.
      if(MusEGlobal::config.enableLatencyCorrection && !extsync)
      {
        // This value is negative for correction.
        if((int)latency_corr < 0)
        {
          // Convert to a positive offset.
          const unsigned int l = (unsigned int)(-latency_corr);
          if(l > lat_offset)
            lat_offset = l;
        }
        if(lat_offset != 0)
        {
          // Be sure to correct both the frame and the tick.
          // Some plugins may use the frame, others the beat.
          cur_frame += lat_offset;
          Pos ppp(cur_frame, false);
          cur_tick = ppp.tick();
          //ppp += nsamp;
          //next_tick = ppp.tick();
        }
      }

      _timeInfo.samplePos = (double)cur_frame;
      _timeInfo.sampleRate = (double)MusEGlobal::sampleRate;
      _timeInfo.flags = 0;

      //Pos p(MusEGlobal::extSyncFlag ? MusEGlobal::audio->tickPos() : cur_frame, MusEGlobal::extSyncFlag ? true : false);

      if(value & kVstBarsValid)
      {
         int p_bar, p_beat;
         unsigned p_tick;
         MusEGlobal::sigmap.tickValues(cur_tick, &p_bar, &p_beat, &p_tick);
         _timeInfo.barStartPos = (double)Pos(p_bar, 0, 0).tick() / (double)MusEGlobal::config.division;
         _timeInfo.flags |= kVstBarsValid;
      }

      if(value & kVstTimeSigValid)
      {
         int z, n;
         MusEGlobal::sigmap.timesig(cur_tick, z, n);

#ifndef VST_VESTIGE_SUPPORT
         _timeInfo.timeSigNumerator = (long)z;
         _timeInfo.timeSigDenominator = (long)n;
#else
         _timeInfo.timeSigNumerator = z;
         _timeInfo.timeSigDenominator = n;
#endif
         _timeInfo.flags |= kVstTimeSigValid;
      }

      if(value & kVstPpqPosValid)
      {
         _timeInfo.ppqPos = (double)cur_tick / (double)MusEGlobal::config.division;
         _timeInfo.flags |= kVstPpqPosValid;
      }

      if(value & kVstTempoValid)
      {
         const double tempo = MusEGlobal::tempomap.tempo(cur_tick);
         _timeInfo.tempo = ((double)MusEGlobal::tempomap.globalTempo() * 600000.0) / tempo;
         _timeInfo.flags |= kVstTempoValid;
      }

#ifdef VST_NATIVE_DEBUG_PROCESS
      fprintf(stderr, "VstNativeSynth::hostCallback master time: sample pos:%f samplerate:%f sig num:%d den:%d tempo:%f\n",
              _timeInfo.samplePos, _timeInfo.sampleRate, _timeInfo.timeSigNumerator, _timeInfo.timeSigDenominator, _timeInfo.tempo);
#endif

      if(MusEGlobal::audio->isPlaying())
         _timeInfo.flags |= (kVstTransportPlaying | kVstTransportChanged);
      // TODO
      //if(MusEGlobal::audio->isRecording())
      //  _timeInfo.flags |= (kVstTransportRecording | kVstTransportChanged);

#ifdef _WIN32
      return *((long*)(&_timeInfo));
#else
      return (long)&_timeInfo;
#endif
   }

   case audioMasterProcessEvents:
   {
     // VstEvents* in <ptr>
     VstEvents* ve = (VstEvents*)ptr;
     int num_ev = ve->numEvents;
#ifdef VST_NATIVE_DEBUG
     fprintf(stderr, "VstNativeSynth::hostCallback audioMasterProcessEvents: numEvents:%d\n", num_ev);
#endif
     for(int i = 0; i < num_ev; ++i)
     {
       // Due to incomplete vestige midi type support, cast as VstMidiEvent first in order to get the type.
       VstMidiEvent* vme = (VstMidiEvent*)ve->events[i];
       switch(vme->type)
       {
         // Is it really a midi event?
         case kVstMidiType:
         {
#ifdef VST_NATIVE_DEBUG
           fprintf(stderr, "  kVstMidiType: deltaFrames:%d midiData[0]:%u [1]:%u [2]:%u\n",
                   vme->deltaFrames,
                   (unsigned char)vme->midiData[0], (unsigned char)vme->midiData[1], (unsigned char)vme->midiData[2]);
#endif
           if(userData->sif)
             userData->sif->eventReceived(vme);
           //else if(userData->pstate) // TODO Plugin midi
             //  vstPlug = userData->pstate->plugin;
         }
         break;

#ifndef VST_VESTIGE_SUPPORT
         case kVstSysExType:
         break;
#endif

         default:
#ifdef VST_NATIVE_DEBUG
           fprintf(stderr, "  unknown event type:%d\n", vme->type);
#endif
         break;
       }
     }
     //return 0;  // TODO:
     return 1; // Supported and processed.
   }

   case audioMasterIOChanged:
      // numInputs and/or numOutputs has changed
      return 0;

   case audioMasterSizeWindow:
   {
      // index: width, value: height
      MusEGui::VstNativeEditor *editor = userData->sif ? userData->sif->_editor : userData->pstate->editor;
      if(VstNativeSynth::resizeEditor(editor, int(index), int(value)))
            return 1; // supported.
      return 0;
   }

   case audioMasterGetSampleRate:
      //return 0;
      return MusEGlobal::sampleRate;

   case audioMasterGetBlockSize:
      //return 0;
      return MusEGlobal::segmentSize;

   case audioMasterGetInputLatency:
      return 0;

   case audioMasterGetOutputLatency:
      return 0;

   case audioMasterGetCurrentProcessLevel:
   {
      // returns: 0: not supported,
      // 1: currently in user thread (gui)
      // 2: currently in audio thread (where process is called)
      // 3: currently in 'sequencer' thread (midi, timer etc)
      // 4: currently offline processing and thus in user thread
      // other: not defined, but probably pre-empting user thread.
      bool inProcessNow = userData->sif ? userData->sif->_inProcess : userData->pstate->inProcess;
      if(inProcessNow)
         return 2;
      else
         return 1;
   }

   case audioMasterGetAutomationState:
      DEBUG_PARAMS(stderr, "VstNativeSynth::pluginHostCallback audioMasterGetAutomationState\n");

      // returns 0: not supported, 1: off, 2:read, 3:write, 4:read/write
      // offline
      return 1;   // TODO:

   case audioMasterOfflineStart:
   case audioMasterOfflineRead:
      // ptr points to offline structure, see below. return 0: error, 1 ok
      return 0;

   case audioMasterOfflineWrite:
      // same as read
      return 0;

   case audioMasterOfflineGetCurrentPass:
   case audioMasterOfflineGetCurrentMetaPass:
      return 0;

   case audioMasterGetVendorString:
      // fills <ptr> with a string identifying the vendor (max 64 char)
      strcpy ((char*) ptr, "MusE");
      return 1;

   case audioMasterGetProductString:
      // fills <ptr> with a string with product name (max 64 char)
      strcpy ((char*) ptr, "MusE Sequencer");
      return 1;

   case audioMasterGetVendorVersion:
      // returns vendor-specific version
      return 2000;

   case audioMasterVendorSpecific:
      // no definition, vendor specific handling
      return 0;

   case audioMasterCanDo:
      // string in ptr, see below
      if(!strcmp((char*)ptr, "sendVstEvents") ||
            !strcmp((char*)ptr, "receiveVstMidiEvent") ||
            !strcmp((char*)ptr, "sendVstMidiEvent") ||
            !strcmp((char*)ptr, "sendVstTimeInfo") ||
            !strcmp((char*)ptr, "sizeWindow") ||
            !strcmp((char*)ptr, "midiProgramNames")
         )
         return 1;

#if 0 //ifndef VST_VESTIGE_SUPPORT
      else
         if(!strcmp((char*)ptr, "openFileSelector") ||
               !strcmp((char*)ptr, "closeFileSelector"))
            return 1;
#endif
      return 0;

   case audioMasterGetLanguage:
      // see enum
      //return 0;
      return kVstLangEnglish;

   case audioMasterGetDirectory:
      // get plug directory, FSSpec on MAC, else char*
      return 0;

   case audioMasterUpdateDisplay:
   {
      DEBUG_PARAMS(stderr, "VstNativeSynth::pluginHostCallback audioMasterUpdateDisplay\n");
      VstNativeSynth::guiAudioMasterUpdateDisplay(userData);
	    // Return 1 if supported, 0 if not supported.
      return 1;
   }

   case audioMasterBeginEdit:
      DEBUG_PARAMS(stderr, "VstNativeSynth::pluginHostCallback audioMasterBeginEdit index:%d\n", index);
      VstNativeSynth::guiAutomationBegin(userData, index);
      return 1;

   case audioMasterEndEdit:
      DEBUG_PARAMS(stderr, "VstNativeSynth::pluginHostCallback audioMasterEndEdit index:%d\n", index);
      VstNativeSynth::guiAutomationEnd(userData, index);
      return 1;

#if 0 //ifndef VST_VESTIGE_SUPPORT
   case audioMasterOpenFileSelector:
      // open a fileselector window with VstFileSelect* in <ptr>
      return 0;

   case audioMasterCloseFileSelector:
      return 0;
#endif

#ifdef VST_FORCE_DEPRECATED
#ifndef VST_2_4_EXTENSIONS // deprecated in 2.4

   case audioMasterGetSpeakerArrangement:
      // (long)input in <value>, output in <ptr>
      return 0;

   case audioMasterPinConnected:
      // inquire if an input or output is being connected;
      // index enumerates input or output counting from zero:
      // value is 0 for input and != 0 otherwise. note: the
      // return value is 0 for <true> such that older versions
      // will always return true.
      //return 1;
      return 0;

      // VST 2.0 opcodes...
   case audioMasterWantMidi:
      // <value> is a filter which is currently ignored
      return 0;

   case audioMasterSetTime:
      // VstTimenfo* in <ptr>, filter in <value>, not supported
      return 0;

   case audioMasterTempoAt:
      // returns tempo (in bpm * 10000) at sample frame location passed in <value>
      return 0;  // TODO:

   case audioMasterGetNumAutomatableParameters:
      return 0;

   case audioMasterGetParameterQuantization:
      // returns the integer value for +1.0 representation,
      // or 1 if full single float precision is maintained
      // in automation. parameter index in <value> (-1: all, any)
      //return 0;
      return 1;

   case audioMasterNeedIdle:
      // plug needs idle calls (outside its editor window)
      return 0;

   case audioMasterGetPreviousPlug:
      // input pin in <value> (-1: first to come), returns cEffect*
      return 0;

   case audioMasterGetNextPlug:
      // output pin in <value> (-1: first to come), returns cEffect*
      return 0;

   case audioMasterWillReplaceOrAccumulate:
      // returns: 0: not supported, 1: replace, 2: accumulate
      //return 0;
      return 1;

   case audioMasterSetOutputSampleRate:
      // for variable i/o, sample rate in <opt>
      return 0;

   case audioMasterSetIcon:
      // void* in <ptr>, format not defined yet
      return 0;

   case audioMasterOpenWindow:
      // returns platform specific ptr
      return 0;

   case audioMasterCloseWindow:
      // close window, platform specific handle in <ptr>
      return 0;
#endif
#endif


   default:
      DEBUG_PARAMS(stderr, "VstNativeSynth::pluginHostCallback unknown callback opcode:%d\n", opcode);
      break;
   }
   return 0;
}
      
//---------------------------------------------------------
//   idleEditor
//---------------------------------------------------------

void VstNativeSynthIF::idleEditor()
{
#ifdef VST_NATIVE_DEBUG
  fprintf(stderr, "VstNativeSynthIF::idleEditor %p\n", this);
#endif

  if(_editor)
  {
    _plugin->dispatcher(_plugin, effEditIdle, 0, 0, nullptr, 0.0f);
     _editor->update();
  }
}

//---------------------------------------------------------
//   guiHeartBeat
//---------------------------------------------------------

void VstNativeSynthIF::guiHeartBeat()
{
  SynthIF::guiHeartBeat();

#ifdef VST_NATIVE_DEBUG_PROCESS
  fprintf(stderr, "VstNativeSynthIF::guiHeartBeat %p\n", this);
#endif

  if(_plugin && _curActiveState)
  {
     if(_guiVisible)
     {
       idleEditor();
     }
  }
}

//---------------------------------------------------------
//   nativeGuiVisible
//---------------------------------------------------------

bool VstNativeSynthIF::nativeGuiVisible() const
      {
      return _guiVisible;
      }

//---------------------------------------------------------
//   showGui
//---------------------------------------------------------

void VstNativeSynthIF::showNativeGui(bool v)
      {
      PluginIBase::showNativeGui(v);

      if(!(_plugin->flags & effFlagsHasEditor)) // || v == nativeGuiVisible())
            return;
      if(v)
      {
        if(_editor)
        {
          if(!_editor->isVisible())
            _editor->show();
          _editor->raise();
          _editor->activateWindow();
        }
        else
        {
          Qt::WindowFlags wflags(Qt::Window
                  | Qt::CustomizeWindowHint
                  | Qt::WindowTitleHint
                  | Qt::WindowSystemMenuHint
                  | Qt::WindowMinMaxButtonsHint
                  | Qt::WindowCloseButtonHint);
          _editor = new MusEGui::VstNativeEditor(nullptr, wflags);
          _editor->open(this, nullptr);
        }
        updateNativeGuiWindowTitle();
      }
      else
      {
        if(_editor)
        {
          _editor->close();
          //_editor = nullptr;  // No - done in editorDeleted.
        }
      }
      _guiVisible = v;
}

//---------------------------------------------------------
//   editorOpened
//---------------------------------------------------------

void VstNativeSynthIF::getNativeGeometry(int*x, int*y, int*w, int*h) const
{
  if(!_editor)
  {
    *x=0;*y=0;*w=0;*h=0;
    return;
  }
  
  *x = _editor->x();
  *y = _editor->y();
  *w = _editor->width();
  *h = _editor->height();
}

//---------------------------------------------------------
//   editorOpened
//---------------------------------------------------------

void VstNativeSynthIF::setNativeGeometry(int x, int y, int w, int h)
{
  if(!_editor)
    return;

  _editor->setGeometry(x, y, w, h);
}

void VstNativeSynthIF::updateNativeGuiWindowTitle()
{
  if(_synth)
    _synth->guiUpdateWindowTitle(&userData);
}

//---------------------------------------------------------
//   editorOpened
//---------------------------------------------------------

void VstNativeSynthIF::editorOpened()
{
  _guiVisible = true;
}

//---------------------------------------------------------
//   editorClosed
//---------------------------------------------------------

void VstNativeSynthIF::editorClosed()
{
  _guiVisible = false;
}

//---------------------------------------------------------
//   editorDeleted
//---------------------------------------------------------

void VstNativeSynthIF::editorDeleted()
{
  _editor = nullptr;
}

//---------------------------------------------------------
//   receiveEvent
//---------------------------------------------------------

MidiPlayEvent VstNativeSynthIF::receiveEvent()
      {
      return MidiPlayEvent();
      }

//---------------------------------------------------------
//   eventReceived
//---------------------------------------------------------

void VstNativeSynthIF::eventReceived(VstMidiEvent* ev)
      {
      const int port = synti->midiPort();

      MidiRecordEvent event;
      event.setB(0);
      //event.setPort(_port);
      event.setPort(port);

      // NOTE: From muse_qt4_evolution. Not done here in Muse-2 (yet).
      // move all events 2*MusEGlobal::segmentSize into the future to get
      // jitterfree playback
      //
      //  cycle   n-1         n          n+1
      //          -+----------+----------+----------+-
      //               ^          ^          ^
      //               catch      process    play
      //

      // These Jack events arrived in the previous period, and it may not have been at the audio position before this one (after a seek).
      // This is how our ALSA driver works, events there are timestamped asynchronous of any process, referenced to the CURRENT audio
      //  position, so that by the time of the NEXT process, THOSE events have also occurred in the previous period.
      // So, technically this is correct. What MATTERS is how we adjust the times for storage, and/or simultaneous playback in THIS period,
      //  and TEST: we'll need to make sure any non-contiguous previous period is handled correctly by process - will it work OK as is?
      // If ALSA works OK than this should too...
      const unsigned int abs_ft = MusEGlobal::audio->curSyncFrame() + ev->deltaFrames;
      event.setTime(abs_ft);

      event.setTick(MusEGlobal::lastExtMidiSyncTick);

      event.setChannel(ev->midiData[0] & 0xf);
      int type = ev->midiData[0] & 0xf0;
      int a    = ev->midiData[1] & 0x7f;
      int b    = ev->midiData[2] & 0x7f;
      event.setType(type);

      //fprintf(stderr, "VstNativeSynthIF::eventReceived(): ev->deltaFrames:%d time:%d type:%x a:%d b:%d\n", ev->deltaFrames, event.time(), type, a, b);

      switch(type) {
            case ME_NOTEON:
                 // REMOVE Tim. Noteoff. Added.
                 // Convert zero-velocity note ons to note offs as per midi spec.
                 if(b == 0)
                   event.setType(ME_NOTEOFF);
                 // Fall through.

            case ME_NOTEOFF:
            case ME_CONTROLLER:
            case ME_POLYAFTER:
                  //event.setA(*(ev->buffer + 1));
                  //event.setB(*(ev->buffer + 2));
                  event.setA(ev->midiData[1]);
                  event.setB(ev->midiData[2]);
                  break;
            case ME_PROGRAM:
            case ME_AFTERTOUCH:
                  //event.setA(*(ev->buffer + 1));
                  event.setA(ev->midiData[1]);
                  break;

            case ME_PITCHBEND:
                  event.setA(((b << 7) + a) - 8192);
                  break;

            case ME_SYSEX:
                  {
                    //int type = *(ev->buffer) & 0xff;
                    int type = ev->midiData[0] & 0xff;
                    switch(type)
                    {
// TODO: Sysex NOT supported with Vestige !
//                           case ME_SYSEX:
//
//                                 // TODO: Deal with large sysex, which are broken up into chunks!
//                                 // For now, do not accept if the last byte is not EOX, meaning it's a chunk with more chunks to follow.
//                                 if(*(((unsigned char*)ev->buffer) + ev->size - 1) != ME_SYSEX_END)
//                                 {
//                                   if(MusEGlobal::debugMsg)
//                                     printf("VstNativeSynthIF::eventReceived sysex chunks not supported!\n");
//                                   return;
//                                 }
//
//                                 //event.setTime(0);      // mark as used
//                                 event.setType(ME_SYSEX);
//                                 event.setData((unsigned char*)(ev->buffer + 1), ev->size - 2);
//                                 break;
                          case ME_MTC_QUARTER:
                                //if(_port != -1)
                                if(port != -1)
                                {
                                  //MusEGlobal::midiSyncContainer.mtcInputQuarter(_port, *(ev->buffer + 1));
                                  MusEGlobal::midiSyncContainer.mtcInputQuarter(port, ev->midiData[1]);
                                }
                                return;
                          case ME_SONGPOS:
                                //if(_port != -1)
                                if(port != -1)
                                {
                                  //MusEGlobal::midiSyncContainer.setSongPosition(_port, *(ev->buffer + 1) | (*(ev->buffer + 2) << 7 )); // LSB then MSB
                                  MusEGlobal::midiSyncContainer.setSongPosition(port, ev->midiData[1] | (ev->midiData[2] << 7 )); // LSB then MSB
                                }
                                return;
                          //case ME_SONGSEL:
                          //case ME_TUNE_REQ:
                          //case ME_SENSE:
                          //      return;

// TODO: Hm, need the last frame time... Isn't that the same as audio->pos().frame() like above?
//                           case ME_CLOCK:
//                                     const jack_nframes_t abs_ft = jack_last_frame_time(jc) - MusEGlobal::segmentSize + ev->time;
//                                     midiClockInput(abs_ft);
//                                 return;
//                           case ME_TICK:
//                           case ME_START:
//                           case ME_CONTINUE:
//                           case ME_STOP:
//                           {
//                                 if(MusEGlobal::audioDevice && MusEGlobal::audioDevice->deviceType() == JACK_MIDI && _port != -1)
//                                 {
//                                   MusECore::JackAudioDevice* jad = static_cast<MusECore::JackAudioDevice*>(MusEGlobal::audioDevice);
//                                   jack_client_t* jc = jad->jackClient();
//                                   if(jc)
//                                   {
//                                     jack_nframes_t abs_ft = jack_last_frame_time(jc)  + ev->time;
//                                     double abs_ev_t = double(jack_frames_to_time(jc, abs_ft)) / 1000000.0;
//                                     MusEGlobal::midiSyncContainer.realtimeSystemInput(_port, type, abs_ev_t);
//                                   }
//                                 }
//                                 return;
//                           }

                          //case ME_SYSEX_END:
                                //break;
                          //      return;
                          default:
                                if(MusEGlobal::debugMsg)
                                  printf("VstNativeSynthIF::eventReceived unsupported system event 0x%02x\n", type);
                                return;
                    }
                  }
                  //return;
                  break;
            default:
              if(MusEGlobal::debugMsg)
                printf("VstNativeSynthIF::eventReceived unknown event 0x%02x\n", type);
                //printf("VstNativeSynthIF::eventReceived unknown event 0x%02x size:%d buf:0x%02x 0x%02x 0x%02x ...0x%02x\n", type, ev->size, *(ev->buffer), *(ev->buffer + 1), *(ev->buffer + 2), *(ev->buffer + (ev->size - 1)));
              return;
            }

      #ifdef VST_NATIVE_DEBUG
      printf("VstNativeSynthIF::eventReceived time:%d type:%d ch:%d A:%d B:%d\n", event.time(), event.type(), event.channel(), event.dataA(), event.dataB());
      #endif

      // Let recordEvent handle it from here, with timestamps, filtering, gui triggering etc.
      synti->recordEvent(event);
      }

//---------------------------------------------------------
//   hasGui
//---------------------------------------------------------

bool VstNativeSynthIF::hasNativeGui() const
      {
      return _plugin->flags & effFlagsHasEditor;
      }

//---------------------------------------------------------
//   channels
//---------------------------------------------------------

int VstNativeSynthIF::channels() const
      {
      return _plugin->numOutputs > MusECore::MAX_CHANNELS ? MusECore::MAX_CHANNELS : _plugin->numOutputs ;
      }

int VstNativeSynthIF::totalOutChannels() const
      {
      return _plugin->numOutputs;
      }

int VstNativeSynthIF::totalInChannels() const
      {
      return _plugin->numInputs;
      }

//---------------------------------------------------------
//   deactivate3
//---------------------------------------------------------

void VstNativeSynthIF::deactivate3()
      {
      if(_editor)
      {
        // Don't delete the editor directly here. Call close.
        _editor->close();
        _editor = nullptr;
        _guiVisible = false;
      }

      deactivate();
      if (_plugin) {
            _plugin->dispatcher (_plugin, effClose, 0, 0, nullptr, 0);
            _plugin = nullptr;
            }
      }

//---------------------------------------------------------
//   queryPrograms
//---------------------------------------------------------

void VstNativeSynthIF::queryPrograms()
{
  void *data = getPrograms();
  if(data)
  {
    swapPrograms(data);
    deleteProgramData(data);
  }
}

//---------------------------------------------------------
//   doSelectProgram
//---------------------------------------------------------

void VstNativeSynthIF::doSelectProgram(int bankH, int bankL, int prog)
{
  if(!_plugin)
    return;

#ifdef VST_NATIVE_DEBUG
  fprintf(stderr, "VstNativeSynthIF::doSelectProgram bankH:%d bankL:%d prog:%d\n", bankH, bankL, prog);
#endif

//    // Only if there's something to change...
//    if(bankH >= 128 && bankL >= 128 && prog >= 128)
//      return;
  
  if(bankH > 127) // Map "dont care" to 0
    bankH = 0;
  if(bankL > 127)
    bankL = 0;
  if(prog > 127)
    prog = 0;
  
  int p = (bankH << 14) | (bankL << 7) | prog;

  if(p >= _plugin->numPrograms)
  {
    fprintf(stderr, "VstNativeSynthIF::doSelectProgram program:%d out of range\n", p);
    return;
  }
  
  //for (unsigned short i = 0; i < instances(); ++i)
  //{
    // "host calls this before a new program (effSetProgram) is loaded"
#ifndef VST_VESTIGE_SUPPORT
    //if(dispatch(effBeginSetProgram, 0, 0, nullptr, 0.0f) == 1)  // TESTED: Usually it did not acknowledge. So IGNORE it.
    dispatch(effBeginSetProgram, 0, 0, nullptr, 0.0f);
    //{
#endif      
      dispatch(effSetProgram, 0, p, nullptr, 0.0f);
      //dispatch(effSetProgram, 0, prog, nullptr, 0.0f);
      // "host calls this after the new program (effSetProgram) has been loaded"
#ifndef VST_VESTIGE_SUPPORT
      dispatch(effEndSetProgram, 0, 0, nullptr, 0.0f);
    //}
    //else
    //  fprintf(stderr, "VstNativeSynthIF::doSelectProgram bankH:%d bankL:%d prog:%d Effect did not acknowledge effBeginSetProgram\n", bankH, bankL, prog);
#endif
  //}
}

//---------------------------------------------------------
//   getPatchName
//---------------------------------------------------------

QString VstNativeSynthIF::getPatchName(int /*chan*/, int prog, bool /*drum*/) const
{
  unsigned long program = prog & 0xff;
  unsigned long lbank   = (prog >> 8) & 0xff;
  unsigned long hbank   = (prog >> 16) & 0xff;
  if (program > 127)  // Map "dont care" to 0
        program = 0;
  if (lbank > 127)
        lbank = 0;
  if (hbank > 127)
        hbank = 0;
  unsigned long p = (hbank << 16) | (lbank << 8) | program;
  unsigned long vp          = (hbank << 14) | (lbank << 7) | program;
  if(vp < programs.size())
  {
    for(std::vector<VST_Program>::const_iterator i = programs.begin(); i != programs.end(); ++i)
    {
      if(i->program == p)
        return i->name;
    }
  }
  return "?";
}

//---------------------------------------------------------
//   populatePatchPopup
//---------------------------------------------------------

void VstNativeSynthIF::populatePatchPopup(MusEGui::PopupMenu* menu, int /*chan*/, bool /*drum*/)
{
  // The plugin can change the programs, patches etc.
  // So make sure we're up to date.
  // This should be somewhat redundant since we already do this is response to audioMasterUpdateDisplay,
  //  but it does no harm to do it again, and just in case something changed without notifying us.
  // Get allocated data representing the list of programs.
  // The operation will take ownership of the data and delete it when done.
  void *data = getPrograms();
  if(data)
  {
    PendingOperationList operations;
    operations.add(PendingOperationItem(this, data, PendingOperationItem::ModifyPluginPrograms));
  }

  menu->clear();

  for (std::vector<VST_Program>::const_iterator i = programs.begin(); i != programs.end(); ++i)
       {
        //int bank = i->bank;
        int prog = i->program;
        //int id   = (bank << 7) + prog;

        const int hb   = (prog >> 16) & 0xff;
        const int lb   = (prog >> 8) & 0xff;
        const int pr = prog & 0xff;
        const bool vhb = hb != 0xff;
        const bool vlb = lb != 0xff;
        const bool vpr = pr != 0xff;
        QString astr;
        if(vhb || vlb || vpr) {
          if(vhb)
            astr += QString::number(hb + 1) + QString(":");
          if(vlb)
            astr += QString::number(lb + 1) + QString(":");
          else if(vhb)
            astr += QString("--:");
          if(vpr)
            astr += QString::number(pr + 1);
          else if(vhb && vlb)
            astr += QString("--");
          astr += QString(" ");
        }
        astr += i->name;
        
        QAction *act = menu->addAction(astr);
        //act->setData(id);
        act->setData(prog);
        }

}

//---------------------------------------------------------
//   getParameter
//---------------------------------------------------------

double VstNativeSynthIF::getParameter(unsigned long idx) const
      {
        return _plugin->getParameter(_plugin, idx);
      }

//---------------------------------------------------------
//   setParameter
//---------------------------------------------------------

void VstNativeSynthIF::setParameter(unsigned long idx, double value)
      {
      addScheduledControlEvent(idx, value, MusEGlobal::audio->curFrame());
      }

//---------------------------------------------------------
//   guiAudioMasterUpdateDisplay
//---------------------------------------------------------

void VstNativeSynth::guiAudioMasterUpdateDisplay(VstNativeSynthOrPlugin *userData)
{
#ifdef VST_NATIVE_DEBUG
   fprintf(stderr, "VstNativeSynth::guiAudioMasterUpdateDisplay\n");
#endif

   // Some plugins only call audioMasterAutomate (which we treat like automation recording),
   //  some only call audioMasterUpdateDisplay, and some call BOTH !
   //
   // NOTE: Tested:
   // This callback may be called by different actions:
   // By our call to dispatch a chunk to the plugin, or when the user chooses a different 'program' in the native UI
   //  or via a midi track driving this plugin.
   // When sending a chunk to the plugins, some plugins (uhe version 131-8256) were observed returning the wrong
   //  parameter values here. That is, they returned the parameter values that existed BEFORE the chunk was sent.
   // Also, the uhe plugins were observed calling this callback TWICE in a row, each with the incorrect values.
   // The same results were seen in QTractor.
   // So... Here we will use a ring buffer to inform the audio thread of changes.

   if(userData->sif)
   {
     Song::IpcEventItem ipci;
     ipci._type = Song::IpcEventItem::QueryPrograms;
     ipci._sif = userData->sif;
     // Tell the gui thread to reload the list of programs.
     MusEGlobal::song->putIpcInEvent(ipci);
   }

   const PluginCallbackEventStruct ev(PluginCallbackEventStruct::UpdateDisplayType);
   userData->sif ? userData->sif->addIpcCallbackEvent(ev) :
                   userData->pstate->pluginI->addIpcCallbackEvent(ev);
}

//---------------------------------------------------------
//   guiAutomationBegin
//---------------------------------------------------------

void VstNativeSynth::guiAutomationBegin(VstNativeSynthOrPlugin *userData, unsigned long param_idx)
{
   const float val = userData->sif ? userData->sif->param(param_idx) : userData->pstate->pluginI->param(param_idx);
   AudioTrack* t = userData->sif ? userData->sif->track() : userData->pstate->pluginI->track();
   int plug_id = userData->sif ? userData->sif->id() : userData->pstate->pluginI->id();

   // Record automation:
   // Take care of this immediately rather than in the fifo processing.
   if(t && plug_id != -1)
   {
      plug_id = genACnum(plug_id, param_idx);
      t->startAutoRecord(plug_id, val);
   }

   const PluginCallbackEventStruct ev(PluginCallbackEventStruct::AutomationBeginType, param_idx, val);
   userData->sif ? userData->sif->addIpcCallbackEvent(ev) :
                   userData->pstate->pluginI->addIpcCallbackEvent(ev);
}

//---------------------------------------------------------
//   guiAutomationEnd
//---------------------------------------------------------

void VstNativeSynth::guiAutomationEnd(VstNativeSynthOrPlugin *userData, unsigned long param_idx)
{
   const float val = userData->sif ? userData->sif->param(param_idx) : userData->pstate->pluginI->param(param_idx);
   AutomationType at = AUTO_OFF;
   AudioTrack* t = userData->sif ? userData->sif->track() : userData->pstate->pluginI->track();
   int plug_id = userData->sif ? userData->sif->id() : userData->pstate->pluginI->id();
   if(t)
      at = t->automationType();

   // Record automation:
   // Take care of this immediately rather than in the fifo processing.
   if(t && plug_id != -1)
   {
      plug_id = genACnum(plug_id, param_idx);
      t->stopAutoRecord(plug_id, val);
   }

   if ((at == AUTO_OFF) || (at == MusECore::AUTO_READ && MusEGlobal::audio->isPlaying()) ||
       (at == AUTO_TOUCH))
   {
      const PluginCallbackEventStruct ev(PluginCallbackEventStruct::AutomationEndType, param_idx, val);
      userData->sif ? userData->sif->addIpcCallbackEvent(ev) :
                      userData->pstate->pluginI->addIpcCallbackEvent(ev);
   }
}

//---------------------------------------------------------
//   guiControlChanged
//---------------------------------------------------------

int VstNativeSynth::guiControlChanged(VstNativeSynthOrPlugin *userData, unsigned long param_idx, float value)
{
   VstNativeSynth *synth = userData->sif ? userData->sif->_synth : userData->pstate->pluginWrapper->_synth;
#ifdef VST_NATIVE_DEBUG
   fprintf(stderr, "VstNativeSynth::guiControlChanged received oscControl port:%lu val:%f\n", param_idx, value);
#endif

   if(param_idx >= synth->inControls())
   {
      fprintf(stderr, "VstNativeSynth::guiControlChanged: port number:%lu is out of range of index list size:%lu\n", param_idx, synth->inControls());
      return 0;
   }

   // Record automation:
   // Take care of this immediately rather than in the fifo processing.
   int plugId = userData->sif ? userData->sif->id() : userData->pstate->pluginI->id();
   if(plugId != -1)
   {
      unsigned long pid = genACnum(plugId, param_idx);
      if(userData->sif)
      {
         userData->sif->synti->recordAutomation(pid, value);
      }
      else
      {
         if(userData->pstate->pluginI->track())
           userData->pstate->pluginI->track()->recordAutomation(pid, value);
      }
   }

   const PluginCallbackEventStruct ev(PluginCallbackEventStruct::AutomationEditType, param_idx, value);
   userData->sif ? userData->sif->addIpcCallbackEvent(ev) :
                   userData->pstate->pluginI->addIpcCallbackEvent(ev);

   return 0;
}

//---------------------------------------------------------
//   getCustomData
//---------------------------------------------------------

std::vector<QString> VstNativeSynthIF::getCustomData() const
{
    std::vector<QString> v;
    v.push_back(_synth->vstconfGetCustomData(_plugin));
    return v;
}

//---------------------------------------------------------
//   setCustomData
//---------------------------------------------------------

bool VstNativeSynthIF::setCustomData(const std::vector< QString > &customParams)
{
  return _synth->vstconfSet(_plugin, customParams);
}

bool VstNativeSynthIF::usesTransportSource() const { return _synth->usesTransportSource(); }

// Temporary variable holds value to be passed to the callback routine.
float VstNativeSynthIF::transportLatencyCorr() const { return _transportLatencyCorr; }

void* VstNativeSynthIF::getPrograms() const
{
  VstPrograms_t *prgs = new VstPrograms_t();
  char buf[256];
  int num_progs = _plugin->numPrograms;
  int iOldIndex = dispatch(effGetProgram, 0, 0, nullptr, 0.0f);
  bool need_restore = false;
  for(int prog = 0; prog < num_progs; ++prog)
  {
    buf[0] = 0;

    // value = category. -1 = regular linear.
    if(dispatch(effGetProgramNameIndexed, prog, -1, buf, 0.0f) == 0)
    {
      dispatch(effSetProgram, 0, prog, nullptr, 0.0f);
      dispatch(effGetProgramName, 0, 0, buf, 0.0f);
      need_restore = true;
    }

    int bankH = (prog >> 14) & 0x7f;
    int bankL = (prog >> 7) & 0x7f;
    int patch = prog & 0x7f;
    VST_Program p;
    p.name    = QString(buf);
    p.program = (bankH << 16) | (bankL << 8) | patch;
    prgs->push_back(p);
  }

  // Restore current program.
  if(need_restore) // && num_progs > 0)
  {
    dispatch(effSetProgram, 0, iOldIndex, nullptr, 0.0f);
    fprintf(stderr, "FIXME: VstNativeSynthIF::getPrograms(): effGetProgramNameIndexed returned 0. Used ugly effSetProgram/effGetProgramName instead\n");
  }

  return prgs;
}

bool VstNativeSynthIF::swapPrograms(void* data)
{
  VstPrograms_t *prgs = (VstPrograms_t*)data;
  programs.swap(*prgs);
  return true;
}

bool VstNativeSynthIF::deleteProgramData(void* data) const
{
  VstPrograms_t *prgs = (VstPrograms_t*)data;
  delete prgs;
  return true;
}

//---------------------------------------------------------
//   getData
//---------------------------------------------------------

void VstNativeSynthIF::setVstEvent(VstMidiEvent* event, int a, int b, int c, int d)
{
  event->type         = kVstMidiType;
  event->byteSize     = 24;
  event->deltaFrames  = 0;
  event->flags        = 0;
  event->detune       = 0;
  event->noteLength   = 0;
  event->noteOffset   = 0;
  event->reserved1    = 0;
  event->reserved2    = 0;
  event->noteOffVelocity = 0;
  event->midiData[0]  = a;
  event->midiData[1]  = b;
  event->midiData[2]  = c;
  event->midiData[3]  = d;
}

//---------------------------------------------------------
//   getData
//---------------------------------------------------------

bool VstNativeSynthIF::processEvent(const MidiPlayEvent& e, VstMidiEvent* event)
{
  int type = e.type();
  int chn = e.channel();
  int a   = e.dataA();
  int b   = e.dataB();

  #ifdef VST_NATIVE_DEBUG
  fprintf(stderr, "VstNativeSynthIF::processEvent midi event type:%d chn:%d a:%d b:%d\n", type, chn, a, b);
  #endif

  // REMOVE Tim. Noteoff. Added.
  const MidiInstrument::NoteOffMode nom = synti->noteOffMode();
  
  switch(type)
  {
    case ME_NOTEON:
      #ifdef VST_NATIVE_DEBUG
      fprintf(stderr, "VstNativeSynthIF::processEvent midi event is ME_NOTEON\n");
      #endif
      
      // REMOVE Tim. Noteoff. Added.
      if(b == 0)
      {
        // Handle zero-velocity note ons. Technically this is an error because internal midi paths
        //  are now all 'note-off' without zero-vel note ons - they're converted to note offs.
        // Nothing should be setting a Note type Event's on velocity to zero.
        // But just in case... If we get this warning, it means there is still code to change.
        fprintf(stderr, "VstNativeSynthIF::processEvent: Warning: Zero-vel note on: time:%d type:%d (ME_NOTEON) ch:%d A:%d B:%d\n", e.time(), e.type(), chn, a, b);  
        switch(nom)
        {
          // Instrument uses note offs. Convert to zero-vel note off.
          case MidiInstrument::NoteOffAll:
            //if(MusEGlobal::midiOutputTrace)
            //  fprintf(stderr, "MidiOut: VST_Native: Following event will be converted to zero-velocity note off:\n");
            setVstEvent(event, (ME_NOTEOFF | chn) & 0xff, a & 0x7f, 0);
          break;
          
          // Instrument uses no note offs at all. Send as-is.
          case MidiInstrument::NoteOffNone:
          // Instrument converts all note offs to zero-vel note ons. Send as-is.
          case MidiInstrument::NoteOffConvertToZVNoteOn:
            setVstEvent(event, (type | chn) & 0xff, a & 0x7f, b & 0x7f);
          break;
        }
      }
      else
        
        setVstEvent(event, (type | chn) & 0xff, a & 0x7f, b & 0x7f);      
      
    break;
    case ME_NOTEOFF:
      #ifdef VST_NATIVE_DEBUG
      fprintf(stderr, "VstNativeSynthIF::processEvent midi event is ME_NOTEOFF\n");
      #endif
      
      // REMOVE Tim. Noteoff. Changed.
//       setVstEvent(event, (type | chn) & 0xff, a & 0x7f, 0);
      switch(nom)
      {
        // Instrument uses note offs. Send as-is.
        case MidiInstrument::NoteOffAll:
          setVstEvent(event, (type | chn) & 0xff, a & 0x7f, b);
        break;
        
        // Instrument uses no note offs at all. Send nothing. Eat up the event - return false.
        case MidiInstrument::NoteOffNone:
          return false;
          
        // Instrument converts all note offs to zero-vel note ons. Convert to zero-vel note on.
        case MidiInstrument::NoteOffConvertToZVNoteOn:
          //if(MusEGlobal::midiOutputTrace)
          //  fprintf(stderr, "MidiOut: VST_Native: Following event will be converted to zero-velocity note on:\n");
          setVstEvent(event, (ME_NOTEON | chn) & 0xff, a & 0x7f, 0);
        break;
      }
      
    break;
    // Synths are not allowed to receive ME_PROGRAM, CTRL_HBANK, or CTRL_LBANK alone anymore.
    case ME_PROGRAM:
    {
      #ifdef VST_NATIVE_DEBUG
      fprintf(stderr, "VstNativeSynthIF::processEvent midi event is ME_PROGRAM\n");
      #endif

      int hb, lb;
      synti->currentProg(chn, nullptr, &lb, &hb);
      synti->setCurrentProg(chn, a & 0xff, lb, hb);
      doSelectProgram(hb, lb, a);
      return false;  // Event pointer not filled. Return false.
    }
    break;
    case ME_CONTROLLER:
    {
      #ifdef VST_NATIVE_DEBUG
      fprintf(stderr, "VstNativeSynthIF::processEvent midi event is ME_CONTROLLER\n");
      #endif

      // Our internal hwCtrl controllers support the 'unknown' value.
      // Don't send 'unknown' values to the driver. Ignore and return no error.
      if(b == CTRL_VAL_UNKNOWN)
        return false;
            
      if(a == CTRL_PROGRAM)
      {
        #ifdef VST_NATIVE_DEBUG
        fprintf(stderr, "VstNativeSynthIF::processEvent midi event is ME_CONTROLLER, dataA is CTRL_PROGRAM\n");
        #endif

        int bankH = (b >> 16) & 0xff;
        int bankL = (b >> 8) & 0xff;
        int prog = b & 0xff;
        synti->setCurrentProg(chn, prog, bankL, bankH);
        doSelectProgram(bankH, bankL, prog);
        return false; // Event pointer not filled. Return false.
      }

      if(a == CTRL_HBANK)
      {
        int lb, pr;
        synti->currentProg(chn, &pr, &lb, nullptr);
        synti->setCurrentProg(chn, pr, lb, b & 0xff);
        doSelectProgram(b, lb, pr);
        // Event pointer not filled. Return false.
        return false;
      }
      
      if(a == CTRL_LBANK)
      {
        int hb, pr;
        synti->currentProg(chn, &pr, nullptr, &hb);
        synti->setCurrentProg(chn, pr, b & 0xff, hb);
        doSelectProgram(hb, b, pr);
        // Event pointer not filled. Return false.
        return false;
      }
      
      if(a == CTRL_PITCH)
      {
        #ifdef VST_NATIVE_DEBUG
        fprintf(stderr, "VstNativeSynthIF::processEvent midi event is ME_CONTROLLER, dataA is CTRL_PITCH\n");
        #endif
        int v = b + 8192;
        setVstEvent(event, (ME_PITCHBEND | chn) & 0xff, v & 0x7f, (v >> 7) & 0x7f);
        return true;
      }

      if(a == CTRL_AFTERTOUCH)
      {
        #ifdef VST_NATIVE_DEBUG
        fprintf(stderr, "VstNativeSynthIF::processEvent midi event is ME_CONTROLLER, dataA is CTRL_AFTERTOUCH\n");
        #endif
        setVstEvent(event, (ME_AFTERTOUCH | chn) & 0xff, b & 0x7f);
        return true;
      }

      if((a | 0xff)  == CTRL_POLYAFTER)
      {
        #ifdef VST_NATIVE_DEBUG
        fprintf(stderr, "VstNativeSynthIF::processEvent midi event is ME_CONTROLLER, dataA is CTRL_POLYAFTER\n");
        #endif
        setVstEvent(event, (ME_POLYAFTER | chn) & 0xff, a & 0x7f, b & 0x7f);
        return true;
      }

      #ifdef VST_NATIVE_DEBUG
      fprintf(stderr, "VstNativeSynthIF::processEvent midi event is ME_CONTROLLER, dataA is:%d\n", a);
      #endif
      
      // Regular controller. Pass it on.
      setVstEvent(event, (type | chn) & 0xff, a & 0x7f, b & 0x7f);
      
      return true; 

// REMOVE Tim. Or keep. TODO For native vsts? Or not...
//      
//       const LADSPA_Descriptor* ld = dssi->LADSPA_Plugin;
// 
//       ciMidiCtl2LadspaPort ip = synth->midiCtl2PortMap.find(a);
//       // Is it just a regular midi controller, not mapped to a LADSPA port (either by the plugin or by us)?
//       // NOTE: There's no way to tell which of these controllers is supported by the plugin.
//       // For example sustain footpedal or pitch bend may be supported, but not mapped to any LADSPA port.
//       if(ip == synth->midiCtl2PortMap.end())
//       {
//         int ctlnum = a;
//         if(midiControllerType(a) != MidiController::Controller7)
//           return false;   // Event pointer not filled. Return false.
//         else
//         {
//                 #ifdef VST_NATIVE_DEBUG
//                 fprintf(stderr, "VstNativeSynthIF::processEvent non-ladspa midi event is Controller7. Current dataA:%d\n", a);
//                 #endif
//                 a &= 0x7f;
//                 ctlnum = DSSI_CC_NUMBER(ctlnum);
//         }
// 
//         // Fill the event.
//         #ifdef VST_NATIVE_DEBUG
//        fprintf(stderr, "VstNativeSynthIF::processEvent non-ladspa filling midi event chn:%d dataA:%d dataB:%d\n", chn, a, b);
//         #endif
//         snd_seq_ev_clear(event);
//         event->queue = SND_SEQ_QUEUE_DIRECT;
//         snd_seq_ev_set_controller(event, chn, a, b);
//         return true;
//       }
// 
//       unsigned long k = ip->second;
//       unsigned long i = controls[k].idx;
//       int ctlnum = DSSI_NONE;
//       if(dssi->get_midi_controller_for_port)
//         ctlnum = dssi->get_midi_controller_for_port(handle, i);
// 
//       // No midi controller for the ladspa port? Send to ladspa control.
//       if(ctlnum == DSSI_NONE)
//       {
//         // Sanity check.
//         if(k > synth->_controlInPorts)
//           return false;
// 
//         // Simple but flawed solution: Start them at 0x60000 + 0x2000 = 0x62000. Max NRPN number is 0x3fff.
//         ctlnum = k + (CTRL_NRPN14_OFFSET + 0x2000);
//       }
//       else
//       {
//         #ifdef VST_NATIVE_DEBUG
//         fprintf(stderr, "VstNativeSynthIF::processEvent plugin requests DSSI-style ctlnum:%x(h) %d(d) be mapped to control port:%lu...\n", ctlnum, ctlnum, i);
//         #endif
// 
//         int c = ctlnum;
//         // Can be both CC and NRPN! Prefer CC over NRPN.
//         if(DSSI_IS_CC(ctlnum))
//         {
//           ctlnum = DSSI_CC_NUMBER(c);
// 
//           #ifdef VST_NATIVE_DEBUG
//           fprintf(stderr, "VstNativeSynthIF::processEvent is CC ctlnum:%d\n", ctlnum);
//           #endif
// 
//           #ifdef VST_NATIVE_DEBUG
//           if(DSSI_IS_NRPN(ctlnum))
//             fprintf(stderr, "VstNativeSynthIF::processEvent is also NRPN control. Using CC.\n");
//           #endif
//         }
//         else
//         if(DSSI_IS_NRPN(ctlnum))
//         {
//           ctlnum = DSSI_NRPN_NUMBER(c) + CTRL_NRPN14_OFFSET;
// 
//           #ifdef VST_NATIVE_DEBUG
//           fprintf(stderr, "VstNativeSynthIF::processEvent is NRPN ctlnum:%x(h) %d(d)\n", ctlnum, ctlnum);
//           #endif
//         }
// 
//       }
// 
//       float val = midi2LadspaValue(ld, i, ctlnum, b);
// 
//       #ifdef VST_NATIVE_DEBUG
//       fprintf(stderr, "VstNativeSynthIF::processEvent control port:%lu port:%lu dataA:%d Converting val from:%d to ladspa:%f\n", i, k, a, b, val);
//       #endif
// 
//       // Set the ladspa port value.
//       controls[k].val = val;
// 
//       // Need to update the automation value, otherwise it overwrites later with the last automation value.
//       if(id() != -1)
//         // We're in the audio thread context: no need to send a message, just modify directly.
//         synti->setPluginCtrlVal(genACnum(id(), k), val);
// 
//       // Since we absorbed the message as a ladspa control change, return false - the event is not filled.
//       return false;
    }
    break;
    case ME_PITCHBEND:
    {
      int v = a + 8192;
      setVstEvent(event, (type | chn) & 0xff, v & 0x7f, (v >> 7) & 0x7f);
    }
    break;
    case ME_AFTERTOUCH:
      setVstEvent(event, (type | chn) & 0xff, a & 0x7f);
    break;
    case ME_POLYAFTER:
      setVstEvent(event, (type | chn) & 0xff, a & 0x7f, b & 0x7f);
    break;
    case ME_SYSEX:
      {
        #ifdef VST_NATIVE_DEBUG
        fprintf(stderr, "VstNativeSynthIF::processEvent midi event is ME_SYSEX\n");
        #endif

        const unsigned char* data = e.constData();
        if(e.len() >= 2)
        {
          if(data[0] == MUSE_SYNTH_SYSEX_MFG_ID)
          {
            if(data[1] == VST_NATIVE_SYNTH_UNIQUE_ID)
            {
              //if(e.len() >= 9)
              if(e.len() >= 10)
              {

                //---------------------------------------------------------------------
                // NOTICE: Obsolete. Replaced by customData block. Keep for old songs!
                //         We should never arrive here with a newer song now.
                //---------------------------------------------------------------------
                if (QString((const char*)(data + 2)).startsWith("VSTSAVE"))
                {
                  if(_synth->hasChunks())
                  {
//#ifndef VST_VESTIGE_SUPPORT
                    int chunk_flags = data[9];
                    if(chunk_flags & VST_NATIVE_CHUNK_FLAG_COMPRESSED)
                      fprintf(stderr, "chunk flags:%x compressed chunks not supported yet.\n", chunk_flags);
                    else
                    {
                     fprintf(stderr, "%s: loading chunk from sysex!\n", name().toLocal8Bit().constData());
                      // 10 = 2 bytes header + "VSTSAVE" + 1 byte flags (compression etc)
                      dispatch(effSetChunk, 0, e.len()-10, (void*)(data+10), 0.0); // index 0: is bank 1: is program
                    }
//#else
//                    fprintf(stderr, "support for vst chunks not compiled in!\n");
//#endif
                  }
                  // Event not filled.
                  return false;
                }
              }
            }
          }
        }
        //else
        {
          // FIXME TODO: Sysex support.
          return false;
        }
      }
    break;
    default:
      if(MusEGlobal::debugMsg)
        fprintf(stderr, "VstNativeSynthIF::processEvent midi event unknown type:%d\n", e.type());
      // Event not filled.
      return false;
    break;
  }

  return true;
}

//---------------------------------------------------------
//   getData
//   If ports is 0, just process controllers only, not audio (do not 'run').
//---------------------------------------------------------

bool VstNativeSynthIF::getData(MidiPort* /*mp*/, unsigned pos, int ports, unsigned nframes, float** buffer)
{
  const unsigned int syncFrame = MusEGlobal::audio->curSyncFrame();

  #ifdef VST_NATIVE_DEBUG_PROCESS
  fprintf(stderr, "VstNativeSynthIF::getData: pos:%u ports:%d nframes:%u syncFrame:%lu\n", pos, ports, nframes, syncFrame);
  #endif

  // All ports must be connected to something!
  const unsigned long in_ports = _synth->inPorts();
  const unsigned long out_ports = _synth->outPorts();
  const unsigned long nop = ((unsigned long) ports) > out_ports ? out_ports : ((unsigned long) ports);

  unsigned long sample = 0;

  const bool isOn = on();
  const MusEPlugin::PluginBypassType bypassType = pluginBypassType();

  //  Normally if the plugin is inactive or off we tell it to connect to dummy audio ports.
  //  But this can change depending on detected bypass type, below.
  bool connectToDummyAudioPorts = !_curActiveState || !isOn;
  //  Normally if the plugin is inactive or off we use a fixed controller period.
  //  But this can change depending on detected bypass type, below.
  bool usefixedrate = !_curActiveState || !isOn;
  const unsigned int fin_nsamp = nframes;

  // If the plugin has a REAL enable or bypass control port, we allow the plugin
  //  a full-length run so that it can handle its own enabling or bypassing.
  if(_curActiveState)
  {
    switch(bypassType)
    {
      case MusEPlugin::PluginBypassTypeEmulatedEnableController:
      break;

      case MusEPlugin::PluginBypassTypeEnablePort:
      case MusEPlugin::PluginBypassTypeBypassPort:
          connectToDummyAudioPorts = false;
          usefixedrate = false;
      break;

      case MusEPlugin::PluginBypassTypeEmulatedEnableFunction:
      break;

      case MusEPlugin::PluginBypassTypeEnableFunction:
      case MusEPlugin::PluginBypassTypeBypassFunction:
          connectToDummyAudioPorts = false;
      break;
    }
  }

  // See if the features require a fixed control period.
  // FIXME Better support for PluginPowerOf2BlockSize, by quantizing the control period times.
  //       For now we treat it like fixed control period.
  if(requiredFeatures() &
     (MusEPlugin::PluginFixedBlockSize |
      MusEPlugin::PluginPowerOf2BlockSize |
      MusEPlugin::PluginCoarseBlockSize))
    usefixedrate = true;

  // For now, the fixed size is clamped to the audio buffer size.
  // TODO: We could later add slower processing over several cycles -
  //  so that users can select a small audio period but a larger control period.
  const unsigned long min_per = (usefixedrate || MusEGlobal::config.minControlProcessPeriod > nframes) ? nframes : MusEGlobal::config.minControlProcessPeriod;
  const unsigned long min_per_mask = min_per-1;   // min_per must be power of 2

  AudioTrack* atrack = track();

  // This value is negative for correction.
  _transportLatencyCorr = 0.0f;
  if(atrack)
  {
    TransportSource& ts = atrack->transportSource();
    const TrackLatencyInfo& li = ts.getLatencyInfo(false);
    if(li._canCorrectOutputLatency)
      _transportLatencyCorr = li._sourceCorrectionValue;
  }

  const AutomationType at = atrack ? atrack->automationType() : AUTO_OFF;
  const bool no_auto = !MusEGlobal::automation || at == AUTO_OFF;
  const unsigned long in_ctrls = _synth->inControls();
  CtrlListList* cll = atrack->controller();
  ciCtrlList icl_first;
  const int plug_id = id();
  if(plug_id != -1)
    icl_first = cll->lower_bound(genACnum(plug_id, 0));

  // Inform the host callback we are in the audio thread.
  _inProcess = true;

  #ifdef VST_NATIVE_DEBUG_PROCESS
  fprintf(stderr, "VstNativeSynthIF::getData: Handling inputs...\n");
  #endif
  
  bool used_in_chan_array[in_ports]; // Don't bother initializing if not 'running'. 
  
  // Gather input data from connected input routes.
  // Don't bother if not 'running'.
  if(_curActiveState /*&& isOn*/)
  {
    // Initialize the array.
    for(unsigned long i = 0; i < in_ports; ++i)
      used_in_chan_array[i] = false;
    
    if(!atrack->noInRoute())
    {
      RouteList *irl = atrack->inRoutes();
      for(ciRoute i = irl->begin(); i != irl->end(); ++i)
      {
        if(i->track->isMidiTrack())
          continue;
        // Only this synth knows how many destination channels there are, 
        //  while only the track knows how many source channels there are.
        // So take care of the destination channels here, and let the track handle the source channels.
        const int dst_ch = i->channel <= -1 ? 0 : i->channel;
        if((unsigned long)dst_ch >= in_ports)
          continue;
        const int dst_chs = i->channels <= -1 ? in_ports : i->channels;
        //const int total_ins = atrack->totalRoutableInputs(Route::TRACK_ROUTE);
        const int src_ch = i->remoteChannel <= -1 ? 0 : i->remoteChannel;
        const int src_chs = i->channels;

        int fin_dst_chs = dst_chs;
        if((unsigned long)(dst_ch + fin_dst_chs) > in_ports)
          fin_dst_chs = in_ports - dst_ch;
            
        static_cast<AudioTrack*>(i->track)->copyData(pos, 
                                                     dst_ch, dst_chs, fin_dst_chs, 
                                                     src_ch, src_chs, 
                                                     nframes, &_audioInBuffers[0], 
                                                     false, used_in_chan_array);
        const int nxt_ch = dst_ch + fin_dst_chs;
        for(int ch = dst_ch; ch < nxt_ch; ++ch)
          used_in_chan_array[ch] = true;
      }
    }
  }

  // Diagnostics.
  //static long unsigned int prevcycle = 0;
  //static long unsigned int cycle = 0;
  //bool messprinted = false;

  // This is an attempt to synchronize what is shown on a midi track's patch display with the plugin's
  //  actual current program, especially when selecting a program through the plugin's native UI.
  // NOTE: TESTED:
  // This did not go so well. Various plugins behave differently.
  // Some don't seem to report a program at all or fix it at 0 even when it has programs.
  // So to avoid such plugins forcing the midi track patch display to 0,
  //  we'll leave this for another day.
#if 0
  if(mp)
  {
    bool midiprogsupported = false;

#ifndef VST_VESTIGE_SUPPORT
    const int chans = dispatch(effGetNumMidiInputChannels, 0, 0, nullptr, 0.0f);
    MidiProgramName pname;
    for(int i = 0; i < chans; ++i)
    {
      const int curprg = dispatch(effGetCurrentMidiProgram, i, 0, &pname, 0.0f);
      // Result is -1 if unsupported.
      if(curprg == -1)
        break;

      midiprogsupported = true;
      // Time value does not matter here.
      const MidiPlayEvent ev(0, mp->portno(), i, ME_CONTROLLER, CTRL_PROGRAM, curprg);

      // Does the CTRL_PROGRAM controller exist?
      iMidiCtrlValList imcvl = mp->controller()->find(i, CTRL_PROGRAM);
      if(imcvl != mp->controller()->end())
      {
        MidiCtrlValList *mpvl = imcvl->second;
        const int hwval = mpvl->hwVal();
        if(hwval != curprg)
          // Tell the gui to set the value.
          mp->handleGui2AudioEvent(ev, true);
      }
      else
      {
        // Tell the gui to create the controller and add the value.
        mp->handleGui2AudioEvent(ev, true);
      }
    }
#endif
    // Unsupported or plugin reported zero midi programs? Try the normal effGetProgram instead.
    if(!midiprogsupported)
    {
      const int curprg = dispatch(effGetProgram, 0, 0, nullptr, 0.0f);
      // Since the plugin does not seem to support per-channel programs, we have no choice
      //  but to set all channels' program controllers to the current program.
      for(int i = 0; i < MIDI_CHANNELS; ++i)
      {
        // Time value does not matter here.
        const MidiPlayEvent ev(0, mp->portno(), i, ME_CONTROLLER, CTRL_PROGRAM, curprg);

        // Does the CTRL_PROGRAM controller exist?
        iMidiCtrlValList imcvl = mp->controller()->find(i, CTRL_PROGRAM);
        if(imcvl != mp->controller()->end())
        {
          MidiCtrlValList *mpvl = imcvl->second;
          const int hwval = mpvl->hwVal();
          if(hwval != curprg)
            // Tell the gui to set the value.
            mp->handleGui2AudioEvent(ev, true);
        }
        else
        {
          // Tell the gui to create the controller and add the value.
          mp->handleGui2AudioEvent(ev, true);
        }
      }
    }
  }
#endif

  // Process events sent to us by the plugin's callback now before other controller stuff.
  if(_ipcCallbackEvents)
  {
    const unsigned sz = _ipcCallbackEvents->getSize();
    for(unsigned i = 0; i < sz; ++i)
    {
      PluginCallbackEventStruct ev;
      if(!_ipcCallbackEvents->get(ev))
        continue;
      switch(ev._type)
      {
        case PluginCallbackEventStruct::AutomationBeginType:
          // Update the automation controller's current value and tell the graphics to redraw etc.
          if(plug_id != -1)
            synti->setPluginCtrlVal(genACnum(plug_id, ev._id), ev._value);
          // Disable the automation controller stream while a control is being manipulated.
          enableController(ev._id, false);
          DEBUG_PARAMS(stderr, "VstNativeSynthIF::getData: Got AutomationBeginType: cycle dif:%lu\n", cycle - prevcycle);
          //messprinted = true;
        break;

        case PluginCallbackEventStruct::AutomationEditType:
          // Update the automation controller's current value and tell the graphics to redraw etc.
          if(plug_id != -1)
            synti->setPluginCtrlVal(genACnum(plug_id, ev._id), ev._value);
          // Disable the automation controller stream while a control is being manipulated.
          // No! We cannot disable controllers here.
          //enableController(ev._id, false);
          DEBUG_PARAMS(stderr, "VstNativeSynthIF::getData: Got AutomationEditType: cycle dif:%lu\n", cycle - prevcycle);
          //messprinted = true;
        break;

        case PluginCallbackEventStruct::AutomationEndType:
          // Update the automation controller's current value and tell the graphics to redraw etc.
          if(plug_id != -1)
            synti->setPluginCtrlVal(genACnum(plug_id, ev._id), ev._value);
          // Re-enable the automation controller stream now that a control is no longer being manipulated.
          enableController(ev._id, true);
          DEBUG_PARAMS(stderr, "VstNativeSynthIF::getData: Got AutomationEndType: cycle dif:%lu\n", cycle - prevcycle);
          //messprinted = true;
        break;

        case PluginCallbackEventStruct::UpdateDisplayType:
          DEBUG_PARAMS(stderr, "VstNativeSynthIF::getData: Got UpdateDisplayType: cycle dif:%lu\n", cycle - prevcycle);
          //messprinted = true;
          // If we receive an UpdateDisplay notification from the plugin, we use it as an indicator
          //  that the program changed. We disable all controller streams to be consistent because
          //  otherwise that job is left to the polling and automation handlers here, and they ONLY
          //  disable controller streams for which a control actually changed, which is imperfect.
          // For plugins that do not send UpdateDisplay, there is nothing more we can do than that.
          // No! We cannot disable controllers here.
          //for(unsigned long k = 0; k < in_ctrls; ++k)
          //  enableController(k, false);
        break;

        case PluginCallbackEventStruct::NoType:
        break;
      }
    }
  }

  // This section handles a few different scenarios:
  // 1) Catch parameters that change without notification, such as the plugin's own midi-to-control assignments.
  //    There seems to be a convention that such assignments do not notify the host on changes.
  // 2) Catch parameters that change upon program changes. Some plugins notify via audioMasterUpdateDisplay,
  //     some only notify via audioMasterAutomate, some do both, and some do nothing.
  //
  // FIXME NOTE: Unfixable?
  // Some plugins do not update their parameters immediately after setting them. It happens later.
  // So what happens is that this thinks there was a change and disables the controller stream.
  // A solution was to provide a timeout timer. We are expecting the parameter to change so we wait for some time,
  //  and if it changed, great, and if not we stop the timer and just carry on.
  // Unfortunately the continuous stream of automation controllers would constantly activate the timer,
  //  preventing anything else from activating this code, such as a plugin's own midi-to-control mapping which
  //  as mentioned does not inform us in any other way.
  // So, we CANNOT disable controller streams here.
  for(unsigned long k = 0; k < in_ctrls; ++k)
  {
    const float curval = getParameter(k);
    // Has the plugin's parameter changed, compared to our cached value?
    // NOTE:
    // Many plugins manipulate the parameters, for example Tonespace has a very long
    //  update time ~50 cycles! And most of them smooth parameter changes, ramping them up or down.
    // So the parameters are NOT changed to the requested value immediately, if at all.
    // Also, for things like switches the plugin may round requested values to 0 or 1.
    // This means the automation controllers, if enabled, in some cases will be constantly
    //  fighting with the cached value. It was observed that even static linear values returned
    //  from a plugin did not match what value our cache recorded was sent.
    // This suggests the plugin was applying granularity to the values.
    // The match even depended on the value itself. Some values sent matched, others did not.
    // So, during those cases this code will be CONSTANTLY called.
    // We have no choice but to just let it be called. Setting the cached value HERE or in the
    //  automation reading code below makes does not help and actually makes things worse.
    // Therefore make sure to check whether the value really needs changing below.
    // Then we should be OK.
    if(curval != _controls[k].val)
    {
      DEBUG_PARAMS(stderr, "VstNativeSynthIF::getData: Got param change k:%lu "
                   "plugin curval:%.15f controls[k].val:%.15f cycle dif:%lu\n",
                   k, curval, _controls[k].val, cycle - prevcycle);
      //messprinted = true;

      // Update the automation controller's current value and tell the graphics to redraw etc.
      if(plug_id != -1)
      {
        const long unsigned acnum = genACnum(plug_id, k);
        // Is the automation controller's static 'current value' different than the plugin's current value?
        const double accurval = cll->value(acnum, 0, true);
        if(accurval != curval)
        {
          DEBUG_PARAMS(stderr, "  accurval:%.15f != plugin curval. Updating.\n", accurval);

          // Update the automation controller's current value and tell the graphics to redraw etc.
          synti->setPluginCtrlVal(acnum, curval);
        }
      }
      // Whatever caused the change, we must treat this condition as a user-manipulated native control.
      // Disable the automation controller stream while a control is being manipulated.
      // No! We cannot disable controllers here.
      //enableController(k, false);
    }
  }

  // TODO: Should we implement emulated bypass for plugins without a bypass feature?
  //       That could be very difficult. How to determine any relationships between
  //        inputs and outputs? LV2 has some features for that. VST has speakers.
  //       And what about midi ports...

  #ifdef VST_NATIVE_DEBUG_PROCESS
  fprintf(stderr, "VstNativeSynthIF::getData: Processing automation control values...\n");
  #endif

  int cur_slice = 0;
  while(sample < fin_nsamp)
  {
    unsigned long slice_samps = fin_nsamp - sample;
    const unsigned long slice_frame = pos + sample;

    //
    // Process automation control values, while also determining the maximum acceptable
    //  size of this run. Further processing, from FIFOs for example, can lower the size
    //  from there, but this section determines where the next highest maximum frame
    //  absolutely needs to be for smooth playback of the controller value stream...
    //
    {
      ciCtrlList icl = icl_first;
      for(unsigned long k = 0; k < in_ctrls; ++k)
      {
        CtrlList* cl = (cll && plug_id != -1 && icl != cll->end()) ? icl->second : nullptr;
        CtrlInterpolate& ci = _controls[k].interp;
        // Always refresh the interpolate struct at first, since things may have changed.
        // Or if the frame is outside of the interpolate range - and eStop is not true.  // FIXME TODO: Be sure these comparisons are correct.
        if(cur_slice == 0 || (!ci.eStop && MusEGlobal::audio->isPlaying() &&
            (slice_frame < (unsigned long)ci.sFrame || (ci.eFrameValid && slice_frame >= (unsigned long)ci.eFrame)) ) )
        {
          if(cl && plug_id != -1 && (unsigned long)cl->id() == genACnum(plug_id, k))
          {
            cl->getInterpolation(slice_frame, no_auto || !_controls[k].enCtrl, &ci);
            if(icl != cll->end())
              ++icl;
          }
          else
          {
            // No matching controller, or end. Just copy the current value into the interpolator.
            // Keep the current icl iterator, because since they are sorted by frames,
            //  if the IDs didn't match it means we can just let k catch up with icl.
            ci.sFrame   = 0;
            ci.eFrame   = 0;
            ci.eFrameValid = false;
            ci.sVal     =  getParameter(k);
            ci.eVal     = ci.sVal;
            ci.doInterp = false;
            ci.eStop    = false;
          }
        }
        else
        {
          if(ci.eStop && ci.eFrameValid && slice_frame >= (unsigned long)ci.eFrame)  // FIXME TODO: Get that comparison right.
          {
            // Clear the stop condition and set up the interp struct appropriately as an endless value.
            ci.sFrame   = 0; //ci->eFrame;
            ci.eFrame   = 0;
            ci.eFrameValid = false;
            ci.sVal     = ci.eVal;
            ci.doInterp = false;
            ci.eStop    = false;
          }
          if(cl && cll && icl != cll->end())
            ++icl;
        }

        if(!usefixedrate && MusEGlobal::audio->isPlaying())
        {
          unsigned long samps = slice_samps;
          if(ci.eFrameValid)
            samps = (unsigned long)ci.eFrame - slice_frame;
          
          if(!ci.doInterp && samps > min_per)
          {
            samps &= ~min_per_mask;
            if((samps & min_per_mask) != 0)
              samps += min_per;
          }
          else
            samps = min_per;

          if(samps < slice_samps)
            slice_samps = samps;

        }

        float new_val;
        if(ci.doInterp && cl)
          new_val = cl->interpolate(MusEGlobal::audio->isPlaying() ? slice_frame : pos, ci);
        else
          new_val = ci.sVal;

        // Is the new desired value different than our cached value?
        if(_controls[k].val != new_val)
        {
          DEBUG_PARAMS(stderr, "VstNativeSynthIF::getData _controls[%lu].val:%.15f != new_val:%.15f "
            "cur_slice:%d sample:%lu fin_nsamp:%d slice_samps:%lu, slice_frame:%lu cycle dif:%lu\n",
            k, _controls[k].val, new_val, cur_slice, sample, fin_nsamp, slice_samps, slice_frame, cycle - prevcycle);
          //messprinted = true;

          _controls[k].val = new_val;

          // Hm, AMSynth said this was false and yet removing it works fine. I guess it doesn't support this call?
          //if(dispatch(effCanBeAutomated, k, 0, nullptr, 0.0f) == 1)
          {
            const float curval = _plugin->getParameter(_plugin, k);
            if(new_val != curval)
            {
              DEBUG_PARAMS(stderr, "  plugin curval:%.15f != new_val. Updating plugin.\n", curval);

              _plugin->setParameter(_plugin, k, new_val);
            }
          }

    #ifdef VST_NATIVE_DEBUG
          else
            fprintf(stderr, "VstNativeSynthIF::getData %s parameter:%lu cannot be automated\n",
                    name().toLocal8Bit().constData(), k);
    #endif
        }

#ifdef VST_NATIVE_DEBUG_PROCESS
        fprintf(stderr, "VstNativeSynthIF::getData k:%lu sample:%lu frame:%lu ci.eFrame:%d slice_samps:%lu \n",
                k, sample, frame, ci.eFrame, slice_samps);
#endif
        
      }
    }
    
#ifdef VST_NATIVE_DEBUG_PROCESS
      fprintf(stderr, "VstNativeSynthIF::getData sample:%lu slice_samps:%lu\n", sample, slice_samps);
#endif

    bool found = false;
    unsigned long frame = 0;
    unsigned long index = 0;
    unsigned long evframe;
    // Get all control ring buffer items valid for this time period...
    while(!_controlFifo.isEmpty())
    {
      const ControlEvent& v = _controlFifo.peek();
      // The events happened in the last period or even before that. Shift into this period with + n. This will sync with audio.
      // If the events happened even before current frame - n, make sure they are counted immediately as zero-frame.
      evframe = (syncFrame > v.frame + nframes) ? 0 : v.frame - syncFrame + nframes;

      #ifdef VST_NATIVE_DEBUG
      fprintf(stderr, "VstNativeSynthIF::getData found:%d evframe:%lu frame:%lu  event frame:%lu idx:%lu val:%f unique:%d\n",
          found, evframe, frame, v.frame, v.idx, v.value, v.unique);
      #endif

      // Protection. Observed this condition. Why? Supposed to be linear timestamps.
      if(found && evframe < frame)
      {
        fprintf(stderr, 
          "VstNativeSynthIF::getData *** Error: Event out of order: evframe:%lu < frame:%lu idx:%lu val:%f unique:%d syncFrame:%u nframes:%u v.frame:%lu\n",
          evframe, frame, v.idx, v.value, v.unique, syncFrame, nframes, v.frame);

        // No choice but to ignore it.
        _controlFifo.remove();               // Done with the ring buffer's item. Remove it.
        continue;
      }

      if(// Next events are for a later period.
         evframe >= nframes
         // Next events are for a later run in this period. (Autom took prio.)
         || (!usefixedrate && !found && !v.unique && (evframe - sample >= slice_samps))
         // Eat up events within minimum slice - they're too close.
         || (found && !v.unique && (evframe - sample >= min_per))
         // Fixed rate and must reply to all.
         || (usefixedrate && found && v.unique && v.idx == index))
        break;

      if(v.idx >= in_ctrls) // Sanity check.
      {
        _controlFifo.remove();               // Done with the ring buffer's item. Remove it.
        break;
      }

      DEBUG_PARAMS(stderr, "VstNativeSynthIF::getData got controlFifo item index:%lu value:%f\n", v.idx, v.value);

      found = true;
      frame = evframe;
      index = v.idx;

      {
        CtrlInterpolate* ci = &_controls[v.idx].interp;
        // Tell it to stop the current ramp at this frame, when it does stop, set this value:
        ci->eFrame = frame;
        ci->eFrameValid = true;
        ci->eVal   = v.value;
        ci->eStop  = true;
      }

      // Need to update the automation value, otherwise it overwrites later with the last automation value.
      if(plug_id != -1)
        synti->setPluginCtrlVal(genACnum(plug_id, v.idx), v.value);

      _controlFifo.remove();               // Done with the ring buffer's item. Remove it.
    }

    if(found && !usefixedrate)  // If a control FIFO item was found, takes priority over automation controller stream.
      slice_samps = frame - sample;

    if(sample + slice_samps > nframes)         // Safety check.
      slice_samps = nframes - sample;

    // TODO: Don't allow zero-length runs. This could/should be checked in the control loop instead.
    // Note this means it is still possible to get stuck in the top loop (at least for a while).
    if(slice_samps != 0)
    {
      {
        unsigned long nevents = 0;

        // Get the state of the stop flag.
        const bool do_stop = synti->stopFlag();
        // Get whether playback and user midi events can be written to this midi device.
        const bool we = synti->writeEnable();

        MidiPlayEvent buf_ev;

        // If stopping or not 'running' just purge ALL playback FIFO and container events.
        // But do not clear the user ones. We need to hold on to them until active,
        //  they may contain crucial events like loading a soundfont from a song file.
        if(do_stop || !_curActiveState || !we)
        {
          // Transfer the user lock-free buffer events to the user sorted multi-set.
          // To avoid too many events building up in the buffer while inactive, use the exclusive add.
          const unsigned int usr_buf_sz = synti->eventBuffers(MidiDevice::UserBuffer)->getSize();
          for(unsigned int i = 0; i < usr_buf_sz; ++i)
          {
            if(synti->eventBuffers(MidiDevice::UserBuffer)->get(buf_ev))
              synti->_outUserEvents.addExclusive(buf_ev);
          }

          synti->eventBuffers(MidiDevice::PlaybackBuffer)->clearRead();
          synti->_outPlaybackEvents.clear();
          // Reset the flag.
          synti->setStopFlag(false);
        }
        else
        {
          // Transfer the user lock-free buffer events to the user sorted multi-set.
          const unsigned int usr_buf_sz = synti->eventBuffers(MidiDevice::UserBuffer)->getSize();
          for(unsigned int i = 0; i < usr_buf_sz; ++i)
          {
            if(synti->eventBuffers(MidiDevice::UserBuffer)->get(buf_ev))
              synti->_outUserEvents.insert(buf_ev);
          }

          // Transfer the playback lock-free buffer events to the playback sorted multi-set.
          const unsigned int pb_buf_sz = synti->eventBuffers(MidiDevice::PlaybackBuffer)->getSize();
          for(unsigned int i = 0; i < pb_buf_sz; ++i)
          {
            if(synti->eventBuffers(MidiDevice::PlaybackBuffer)->get(buf_ev))
              synti->_outPlaybackEvents.insert(buf_ev);
          }
        }

        // Don't bother if not 'running'.
        if(_curActiveState && we)
        {
          // Count how many events we need.
          for(ciMPEvent impe = synti->_outPlaybackEvents.begin(); impe != synti->_outPlaybackEvents.end(); ++impe)
          {
            const MidiPlayEvent& e = *impe;
            if(e.time() >= (syncFrame + sample + slice_samps))
              break;
            ++nevents;
          }
          for(ciMPEvent impe = synti->_outUserEvents.begin(); impe != synti->_outUserEvents.end(); ++impe)
          {
            const MidiPlayEvent& e = *impe;
            if(e.time() >= (syncFrame + sample + slice_samps))
              break;
            ++nevents;
          }

          VstMidiEvent events[nevents];
          char evbuf[sizeof(VstMidiEvent*) * nevents + sizeof(VstEvents)];
          VstEvents *vst_events = (VstEvents*)evbuf;
          vst_events->numEvents = 0;
          vst_events->reserved  = 0;

          iMPEvent impe_pb = synti->_outPlaybackEvents.begin();
          iMPEvent impe_us = synti->_outUserEvents.begin();
          bool using_pb;

          unsigned long event_counter = 0;
          while(1)
          {
            if(impe_pb != synti->_outPlaybackEvents.end() && impe_us != synti->_outUserEvents.end())
              using_pb = *impe_pb < *impe_us;
            else if(impe_pb != synti->_outPlaybackEvents.end())
              using_pb = true;
            else if(impe_us != synti->_outUserEvents.end())
              using_pb = false;
            else break;

            const MidiPlayEvent& e = using_pb ? *impe_pb : *impe_us;

            #ifdef VST_NATIVE_DEBUG
            fprintf(stderr, "VstNativeSynthIF::getData eventFifos event time:%d\n", e.time());
            #endif

            // Event is for future?
            if(e.time() >= (sample + slice_samps + syncFrame))
              break;

            // Returns false if the event was not filled. It was handled, but some other way.
            if(processEvent(e, &events[event_counter]))
            {
              // Time-stamp the event.
              unsigned int ft = (e.time() < syncFrame) ? 0 : e.time() - syncFrame;
              ft = (ft < sample) ? 0 : ft - sample;

              if(ft >= slice_samps)
              {
                  fprintf(stderr, "VstNativeSynthIF::getData: eventFifos event time:%d "
                  "out of range. pos:%d syncFrame:%u ft:%u sample:%lu slice_samps:%lu\n",
                          e.time(), pos, syncFrame, ft, sample, slice_samps);
                  ft = slice_samps - 1;
              }
              vst_events->events[event_counter] = (VstEvent*)&events[event_counter];
              events[event_counter].deltaFrames = ft;

              ++event_counter;
            }

            // Done with buffer's event. Remove it.
            // C++11.
            if(using_pb)
              impe_pb = synti->_outPlaybackEvents.erase(impe_pb);
            else
              impe_us = synti->_outUserEvents.erase(impe_us);
          }

          if(event_counter < nevents)
            nevents = event_counter;

          // Set the events pointer.
          if(nevents > 0)
          {
            vst_events->numEvents = nevents;
            dispatch(effProcessEvents, 0, 0, vst_events, 0.0f);
          }
        }
      }

      #ifdef VST_NATIVE_DEBUG_PROCESS
      fprintf(stderr, "VstNativeSynthIF::getData: Connecting and running. sample:%lu nsamp:%lu nevents:%lu\n", sample, nsamp, nevents);
      #endif

      // Don't bother if not 'running'.
      if(_curActiveState)
      {
        float* in_bufs[in_ports];
        float* out_bufs[out_ports];
        for(unsigned long k = 0; k < out_ports; ++k)
        {
          if(!connectToDummyAudioPorts && k < nop)
            // Connect the given buffers directly to the ports, up to a max of synth ports.
            out_bufs[k] = buffer[k] + sample;
          else
            // Connect the remaining ports to some local buffers (not used yet).
            out_bufs[k] = _audioOutBuffers[k] + sample;
        }

        // Connect all inputs either to the input buffers, or a silence buffer.
        for(unsigned long k = 0; k < in_ports; ++k)
        {
          if(!connectToDummyAudioPorts && used_in_chan_array[k])
            in_bufs[k] = _audioInBuffers[k] + sample;
          else
            in_bufs[k] = _audioInSilenceBuf + sample;
        }

        // Run the synth for a period of time. This processes events and gets/fills our local buffers...
        if((_plugin->flags & effFlagsCanReplacing) && _plugin->processReplacing)
        {
          _plugin->processReplacing(_plugin, in_bufs, out_bufs, slice_samps);
        }
      }

      sample += slice_samps;
    }

    ++cur_slice; // Slice is done. Moving on to any next slice now...
  }
  
  // Diagnostics.
  //if(messprinted)
  //  prevcycle = cycle;
  //++cycle;

  // Inform the host callback we will be no longer in the audio thread.
  _inProcess = false;

  return true;
}

//--------------------------------
// Methods for PluginIBase:
//--------------------------------

unsigned long VstNativeSynthIF::pluginID() const                  { return (_plugin) ? _plugin->uniqueID : 0; }
void VstNativeSynthIF::enableController(unsigned long i, bool v)
{ if(_controls) _controls[i].enCtrl = v; }
bool VstNativeSynthIF::controllerEnabled(unsigned long i) const
{ return _controls ? _controls[i].enCtrl : true;}
void VstNativeSynthIF::enableAllControllers(bool v)
{
  if(!_synth || !_controls)
    return;
  const unsigned long sic = _synth->inControls();
  for(unsigned long i = 0; i < sic; ++i)
    _controls[i].enCtrl = v;
}

void VstNativeSynthIF::updateController(unsigned long i)
{
  // If plugin is not available just return.
  if(!_synth || !_controls || !synti || id() < 0)
    return;

  const float v = param(i);
  // Make sure to update this cached value.
  _controls[i].val = v;
  synti->setPluginCtrlVal(genACnum(id(), i), v);
}

void VstNativeSynthIF::updateControllers()
{
  const unsigned long sic = _synth->inControls();
  for(unsigned long i = 0; i < sic; ++i)
    updateController(i);
}

void VstNativeSynthIF::activate()
{
  if(_curActiveState)
    return;

  // Set some default properties
  dispatch(effSetSampleRate, 0, 0, nullptr, MusEGlobal::sampleRate);
  dispatch(effSetBlockSize, 0, MusEGlobal::segmentSize, nullptr, 0.0f);
  dispatch(effMainsChanged, 0, 1, nullptr, 0.0f);
  dispatch(effStartProcess, 0, 0, nullptr, 0.0f);

// REMOVE Tim. Or keep? From PluginI::activate().
//   if (initControlValues) {
//         for (unsigned long i = 0; i < controlPorts; ++i) {
//               controls[i].val = controls[i].tmpVal;
//               }
//         }
//   else {
//         // get initial control values from plugin
//         for (unsigned long i = 0; i < controlPorts; ++i) {
//               controls[i].tmpVal = controls[i].val;
//               }
//         }
  SynthIF::activate();
}
void VstNativeSynthIF::deactivate()
{
  if(!_curActiveState)
    return;
  SynthIF::deactivate();
  dispatch(effStopProcess, 0, 0, nullptr, 0.0f);
  dispatch(effMainsChanged, 0, 0, nullptr, 0.0f);
}

unsigned long VstNativeSynthIF::parameters() const                { return _synth ? _synth->inControls() : 0; }
unsigned long VstNativeSynthIF::parametersOut() const             { return 0; }
void VstNativeSynthIF::setParam(unsigned long i, double val)       { setParameter(i, val); }
double VstNativeSynthIF::param(unsigned long i) const              { return getParameter(i); }
double VstNativeSynthIF::paramOut(unsigned long) const            { return 0.0; }
const char* VstNativeSynthIF::paramName(unsigned long i) const
{
  if(!_plugin)
    return nullptr;
  static char buf[256];
  buf[0] = 0;
  dispatch(effGetParamName, i, 0, buf, 0);
  return buf;
}

const char* VstNativeSynthIF::paramOutName(unsigned long) const       { return 0; }
LADSPA_PortRangeHint VstNativeSynthIF::range(unsigned long /*i*/) const
{
  LADSPA_PortRangeHint h;
  // FIXME TODO:
  h.HintDescriptor = 0;
  h.LowerBound = 0.0;
  h.UpperBound = 1.0;
  return h;
}
LADSPA_PortRangeHint VstNativeSynthIF::rangeOut(unsigned long) const
{
  // There are no output controls.
  LADSPA_PortRangeHint h;
  h.HintDescriptor = 0;
  h.LowerBound = 0.0;
  h.UpperBound = 1.0;
  return h;
}
void VstNativeSynthIF::range(unsigned long /*i*/, float* min, float* max) const
{
  // FIXME TODO:
  *min = 0.0;
  *max = 1.0;
}
void VstNativeSynthIF::rangeOut(unsigned long /*i*/, float* min, float* max) const
{
  // There are no output controls.
  *min = 0.0;
  *max = 1.0;
}
// FIXME TODO:
CtrlValueType VstNativeSynthIF::ctrlValueType(unsigned long /*i*/) const { return VAL_LINEAR; }
CtrlList::Mode VstNativeSynthIF::ctrlMode(unsigned long /*i*/) const     { return CtrlList::INTERPOLATE; }
// There are no output controls.
CtrlValueType VstNativeSynthIF::ctrlOutValueType(unsigned long /*i*/) const { return VAL_LINEAR; }
CtrlList::Mode VstNativeSynthIF::ctrlOutMode(unsigned long /*i*/) const     { return CtrlList::INTERPOLATE; }

VstNativePluginWrapper::VstNativePluginWrapper(VstNativeSynth *s, MusEPlugin::PluginFeatures_t reqFeatures)
 : Plugin()
{
   _synth = s;

   _requiredFeatures = reqFeatures;
   
   _fakeLd.Label = strdup(_synth->label().toUtf8().constData());
   _fakeLd.Name = strdup(_synth->name().toUtf8().constData());
   _fakeLd.UniqueID = _synth->_id;
   _fakeLd.Maker = strdup(_synth->maker().toUtf8().constData());
   _fakeLd.Copyright = strdup(_synth->version().toUtf8().constData());

   _pluginType = MusEPlugin::PluginTypeLinuxVST;
   _pluginClass = s->pluginClass();

   int numPorts = _synth->inPorts()
                  + _synth->outPorts()
                  + _synth->inControls();
   _fakeLd.PortCount = numPorts;
   _fakePds = new LADSPA_PortDescriptor [numPorts];
   memset(_fakePds, 0, sizeof(int) * numPorts);

   for(size_t i = 0; i < _synth->inPorts(); i++)
   {
      _fakePds [i] = LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO;
   }

   for(size_t i = 0; i < _synth->outPorts(); i++)
   {
      _fakePds [i + _synth->inPorts()] = LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO;
   }

   for(size_t i = 0; i < _synth->inControls(); i++)
   {
      _fakePds [i + _synth->inPorts() + _synth->outPorts()] = LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
   }


   _fakeLd.PortNames = nullptr;
   _fakeLd.PortRangeHints = nullptr;
   _fakeLd.PortDescriptors = _fakePds;
   _fakeLd.Properties = 0;
   plugin = &_fakeLd;

   _fileInfo = _synth->_fileInfo;
   _uri = _synth->uri();
   _label = _synth->label();
   _name = _synth->name();
   _description = _synth->description();
   _uniqueID = plugin->UniqueID;
   _maker = _synth->maker();
   _copyright = _synth->version();

    _pluginFreewheelType = _synth->pluginFreewheelType();
    _freewheelPortIndex = _synth->freewheelPortIndex();
    _pluginLatencyReportingType = _synth->pluginLatencyReportingType();
    _latencyPortIndex = _synth->latencyPortIndex();
    _pluginBypassType = _synth->pluginBypassType();
    _enableOrBypassPortIndex = _synth->enableOrBypassPortIndex();

   _portCount = plugin->PortCount;

   for(unsigned long k = 0; k < _portCount; ++k)
   {
      LADSPA_PortDescriptor pd = plugin->PortDescriptors[k];

      if(pd & LADSPA_PORT_AUDIO)
      {
         if(pd & LADSPA_PORT_INPUT)
         {
            ++_inports;
         }
         else if(pd & LADSPA_PORT_OUTPUT)
         {
            ++_outports;
         }
      }
      else if(pd & LADSPA_PORT_CONTROL)
      {
         if(pd & LADSPA_PORT_INPUT)
         {
            ++_controlInPorts;
         }
         else if(pd & LADSPA_PORT_OUTPUT)
         {
            ++_controlOutPorts;
         }
      }
   }
}

VstNativePluginWrapper::~VstNativePluginWrapper()
{
   free((void*)_fakeLd.Label);
   free((void*)_fakeLd.Name);
   free((void*)_fakeLd.Maker);
   free((void*)_fakeLd.Copyright);
   delete [] _fakePds;
}

LADSPA_Handle VstNativePluginWrapper::instantiate(PluginI *pluginI)
{
  VstNativePluginWrapper_State *state = new VstNativePluginWrapper_State;
  if(!state)
  {
    abort();
  }
  state->plugin = _synth->instantiate(&state->userData);
  if(!state->plugin)
  {
    delete state;
    return 0;
  }

  if(!_synth->openPlugin(state->plugin))
  {
    delete state;
    return 0;
  }

  state->pluginI = pluginI;
  state->pluginWrapper = this;
  state->inPorts.resize(_inports);
  state->outPorts.resize(_outports);
  state->inControlPorts.resize(_controlInPorts);
  portNames.resize(_inports + _outports + _controlInPorts);
  memset(&state->inPorts [0], 0, _inports * sizeof(float *));
  memset(&state->outPorts [0], 0, _outports * sizeof(float *));
  memset(&state->inControlPorts [0], 0, _controlInPorts * sizeof(float *));

  for(size_t i = 0; i < portNames.size(); i++)
  {
     if(i < _inports)
     {
        std::stringstream ss;
        ss << "input" << i;
        portNames [i] = ss.str();
     }
     else if(i < _inports + _outports)
     {
        std::stringstream ss;
        ss << "output" << (i - _inports);
        portNames [i] = ss.str();
     }
     else if(i < _inports + _outports + _controlInPorts)
     {
        char buf[256];
        memset(buf, 0, sizeof(buf));
        dispatch(state, effGetParamName, i - _inports - _outports, 0, buf, 0);
        if(strlen(buf) > 0)
        {
           portNames [i] = buf;
        }
        else
        {
           std::stringstream ss;
           ss << "control" << (i - _inports - _outports);
           portNames [i] = ss.str();
        }
     }
  }

  QObject::connect(MusEGlobal::heartBeatTimer, SIGNAL(timeout()), state, SLOT(heartBeat()));

  return(LADSPA_Handle)state;

}

bool VstNativePluginWrapper::reference()
{
   return _synth->reference();
}

int VstNativePluginWrapper::release()
{
   return _synth->release();
}

void VstNativePluginWrapper::activate(LADSPA_Handle handle)
{
   VstNativePluginWrapper_State *state = (VstNativePluginWrapper_State *)handle;
   if(!state || state->active)
     return;
   // Set some default properties
   dispatch(state, effSetSampleRate, 0, 0, nullptr, MusEGlobal::sampleRate);
   dispatch(state, effSetBlockSize, 0, MusEGlobal::segmentSize, nullptr, 0.0f);
   dispatch(state, effMainsChanged, 0, 1, nullptr, 0.0f);
   dispatch(state, effStartProcess, 0, 0, nullptr, 0.0f);

   state->active = true;
}

void VstNativePluginWrapper::deactivate(LADSPA_Handle handle)
{
   VstNativePluginWrapper_State *state = (VstNativePluginWrapper_State *)handle;
   if(!state || !state->active)
      return;
   state->active = false;
   dispatch(state, effStopProcess, 0, 0, nullptr, 0.0f);
   dispatch(state, effMainsChanged, 0, 0, nullptr, 0.0f);
}

void VstNativePluginWrapper::cleanup(LADSPA_Handle handle)
{
   VstNativePluginWrapper_State *state = (VstNativePluginWrapper_State *)handle;
   if(!state)
   {
      return;
   }
   if(state->editor)
   {
     state->editor->close();
     state->editor = nullptr;
     state->guiVisible = false;
   }

   if (state->plugin)
   {
      dispatch(state, effClose, 0, 0, nullptr, 0);
      state->plugin = 0;
   }

   delete state;
}

void VstNativePluginWrapper::connectPort(LADSPA_Handle handle, unsigned long port, float *value)
{
   VstNativePluginWrapper_State *state = (VstNativePluginWrapper_State *)handle;
   if(port < _inports)
   {
      state->inPorts [port] = value;
   }
   else if(port < _inports + _outports)
   {
      state->outPorts [port - _inports] = value;
   }
   else if(port < _inports + _outports + _controlInPorts)
   {
      state->inControlPorts [port - _inports - _outports] = value;
   }

}

void VstNativePluginWrapper::apply(LADSPA_Handle handle, unsigned long n, float latency_corr)
{
   VstNativePluginWrapper_State *state = (VstNativePluginWrapper_State *)handle;
   state->inProcess = true;
   // Pass the value to the callback routine.
   state->_latency_corr = latency_corr;

  // If the plugin is active now, we can set the enable/bypass function.
  // Calling an enable/bypass function while the plugin is inactive
  //  might not be a good idea. How would we know it worked? Would it
  //  remember and take effect when reactivated, or would it take effect
  //  immediately, or would it be ignored?
  // So we defer until activated...
  if(state->active && _pluginBypassType == MusEPlugin::PluginBypassTypeBypassFunction)
  {
    // To avoid repeated settings, we keep a current state and compare with it.
    // The initial value is ON since it is assumed the plugin is created in
    //  the ON state. There is no reliable way to read the plugin's current state,
    //  for example VST2 has no such read function.
    const bool isOn = state->pluginI->on();
    if(state->curEnabledState != isOn)
    {
      _synth->setPluginEnabled(state->plugin, isOn);
      state->curEnabledState = isOn;
    }
  }

   if((state->plugin->flags & effFlagsCanReplacing) && state->plugin->processReplacing)
   {
     state->plugin->processReplacing(state->plugin, &state->inPorts [0], &state->outPorts [0], n);
   }
   state->inProcess = false;

}

LADSPA_PortDescriptor VstNativePluginWrapper::portd(unsigned long k) const
{
   return _fakeLd.PortDescriptors[k];
}

LADSPA_PortRangeHint VstNativePluginWrapper::range(unsigned long) const
{
   LADSPA_PortRangeHint hint;
   hint.HintDescriptor = 0;
   hint.LowerBound = 0.0f;
   hint.UpperBound = 1.0f;

   hint.HintDescriptor |= LADSPA_HINT_BOUNDED_BELOW;
   hint.HintDescriptor |= LADSPA_HINT_BOUNDED_ABOVE;

   return hint;
}

void VstNativePluginWrapper::range(unsigned long, float *min, float *max) const
{
   *min = 0.0f;
   *max = 1.0f;
}

// VST does not use port values and therefore does not need defaults.
double VstNativePluginWrapper::defaultValue(unsigned long ) const
{
   return 0.0;
}

const char *VstNativePluginWrapper::portName(unsigned long port) const
{
   return portNames [port].c_str();
}

CtrlValueType VstNativePluginWrapper::ctrlValueType(unsigned long) const
{
   return VAL_LINEAR;
}

CtrlList::Mode VstNativePluginWrapper::ctrlMode(unsigned long) const
{
   return CtrlList::INTERPOLATE;
}

bool VstNativePluginWrapper::hasNativeGui() const
{
   return _synth->_hasGui;
}

void VstNativePluginWrapper::showNativeGui(PluginI *p, bool bShow)
{
   assert(p->instances > 0);
   VstNativePluginWrapper_State *state = (VstNativePluginWrapper_State *)p->handle [0];
   if(!hasNativeGui())
      return;
   if(bShow)
   {
      if(state->editor)
      {
         if(!state->editor->isVisible())
            state->editor->show();
         state->editor->raise();
         state->editor->activateWindow();
      }
      else
      {
         Qt::WindowFlags wflags(Qt::Window
                                  | Qt::CustomizeWindowHint
                                  | Qt::WindowTitleHint
                                  | Qt::WindowSystemMenuHint
                                  | Qt::WindowMinMaxButtonsHint
                                  | Qt::WindowCloseButtonHint);
         state->editor = new MusEGui::VstNativeEditor(nullptr, wflags);
         state->editor->open(0, state);
      }
      if(state->pluginI)
      {
        updateNativeGuiWindowTitle(state->pluginI);
      }
   }
   else
   {
      if(state->editor)
      {
         state->editor->close();
         //_editor = nullptr;  // No - done in editorDeleted.
      }
   }
   state->guiVisible = bShow;
}

bool VstNativePluginWrapper::nativeGuiVisible(const PluginI *p) const
{
   assert(p->instances > 0);
   VstNativePluginWrapper_State *state = (VstNativePluginWrapper_State *)p->handle [0];
   return state->guiVisible;
}

void VstNativePluginWrapper::updateNativeGuiWindowTitle(const PluginI *p) const
{
  if(p->instances > 0)
  {
    VstNativePluginWrapper_State *state = (VstNativePluginWrapper_State *)p->handle [0];
    if(_synth)
      _synth->guiUpdateWindowTitle(&state->userData);
  }
}

QString VstNativePluginWrapper::getCustomConfiguration(LADSPA_Handle handle)
{
  VstNativePluginWrapper_State *state = (VstNativePluginWrapper_State *)handle;
  return _synth->vstconfGetCustomData(state->plugin);
}

bool VstNativePluginWrapper::setCustomData(LADSPA_Handle handle, const std::vector<QString> &customParams)
{
   VstNativePluginWrapper_State *state = (VstNativePluginWrapper_State *)handle;
  return _synth->vstconfSet(state->plugin, customParams);
}

void VstNativePluginWrapper_State::heartBeat()
{
   if(plugin && active)
   {
      if(guiVisible)
      {
        idleEditor();
      }
   }
}

void VstNativePluginWrapper_State::idleEditor()
{
#ifdef VST_NATIVE_DEBUG
  fprintf(stderr, "VstNativePluginWrapper_State::idleEditor %p\n", this);
#endif

  if(editor)
  {
    plugin->dispatcher(plugin, effEditIdle, 0, 0, nullptr, 0.0f);
    editor->update();
  }
}

} // namespace MusECore

#else  // VST_NATIVE_SUPPORT
namespace MusECore {
void initVST_Native() {}
} // namespace MusECore
#endif

