//===================================================================
//  MusE
//  Linux Music Editor
//
//  vst_native.cpp
//  (C) Copyright 2012 Tim E. Real (terminator356 on users dot sourceforge dot net)
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
#include <stdio.h>
#include <dlfcn.h>
#include <cmath>
#include <set>
#include <string>
#include <jack/jack.h>

#include "globals.h"
#include "gconfig.h"
#include "audio.h"
#include "synth.h"
#include "jackaudio.h"
#include "midi.h"
#include "xml.h"
#include "plugin.h"
#include "popupmenu.h"
#include "pos.h"
#include "tempo.h"
#include "sync.h"
#include "al/sig.h"

#include "vst_native.h"

#define OLD_PLUGIN_ENTRY_POINT "main"
#define NEW_PLUGIN_ENTRY_POINT "VSTPluginMain"

// Enable debugging messages
//#define VST_NATIVE_DEBUG
//#define VST_NATIVE_DEBUG_PROCESS

namespace MusECore {

extern JackAudioDevice* jackAudio;

//-----------------------------------------------------------------------------------------
//   vstHostCallback
//   This must be a function, it cannot be a class method so we dispatch to various objects from here.
//-----------------------------------------------------------------------------------------

VstIntPtr VSTCALLBACK vstNativeHostCallback(AEffect* effect, VstInt32 opcode, VstInt32 index, VstIntPtr value, void* ptr, float opt)
{
      // Is this callback for an actual instance? Hand-off to the instance if so.
      VSTPlugin* plugin;
      if(effect && effect->user)
      {
        plugin = (VSTPlugin*)(effect->user);
        return ((VstNativeSynthIF*)plugin)->hostCallback(opcode, index, value, ptr, opt);
      }

      // No instance found. So we are just scanning for plugins...
    
#ifdef VST_NATIVE_DEBUG      
      fprintf(stderr, "vstNativeHostCallback eff:%p opcode:%ld\n", effect, opcode);
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
                  return 0;

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

            case audioMasterGetSpeakerArrangement:
                  // (long)input in <value>, output in <ptr>
                  return 0;

            case audioMasterPinConnected:
                  // inquire if an input or output is beeing connected;
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
                  
            default:
                  break;
            }

      if(MusEGlobal::debugMsg)
        fprintf(stderr, "  unknown opcode\n");

      return 0;
      }

//---------------------------------------------------------
//   loadPluginLib
//---------------------------------------------------------

static void scanVstNativeLib(QFileInfo& fi)
{
  void* handle = dlopen(fi.filePath().toAscii().constData(), RTLD_NOW);
  if (handle == NULL)
  {
    fprintf(stderr, "scanVstNativeLib: dlopen(%s) failed: %s\n", fi.filePath().toAscii().constData(), dlerror());
    return;
  }

  char buffer[128];
  QString effectName;
  QString vendorString;
  QString productString;
  int vendorVersion;
  std::vector<Synth*>::iterator is;
  int vst_version = 0;
  VstNativeSynth* new_synth = NULL;
  
  AEffect *(*getInstance)(audioMasterCallback);
  getInstance = (AEffect*(*)(audioMasterCallback))dlsym(handle, NEW_PLUGIN_ENTRY_POINT);
  if(!getInstance)
  {
    if(MusEGlobal::debugMsg)
    {
      fprintf(stderr, "VST 2.4 entrypoint \"" NEW_PLUGIN_ENTRY_POINT "\" not found in library %s, looking for \""
                      OLD_PLUGIN_ENTRY_POINT "\"\n", fi.filePath().toAscii().constData());
    }

    getInstance = (AEffect*(*)(audioMasterCallback))dlsym(handle, OLD_PLUGIN_ENTRY_POINT);
    if(!getInstance)
    {
      fprintf(stderr, "ERROR: VST entrypoints \"" NEW_PLUGIN_ENTRY_POINT "\" or \""
                      OLD_PLUGIN_ENTRY_POINT "\" not found in library\n");
      dlclose(handle);
      return;
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

  AEffect *plugin = getInstance(vstNativeHostCallback);
  if(!plugin)
  {
    fprintf(stderr, "ERROR: Failed to instantiate plugin in VST library \"%s\"\n", fi.filePath().toAscii().constData());
    dlclose(handle);
    return;
  }
  else if(MusEGlobal::debugMsg)
    fprintf(stderr, "plugin instantiated\n");

  if(plugin->magic != kEffectMagic)
  {
    fprintf(stderr, "Not a VST plugin in library \"%s\"\n", fi.filePath().toAscii().constData());
    dlclose(handle);
    return;
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

  plugin->dispatcher(plugin, effOpen, 0, 0, NULL, 0);
  
  buffer[0] = 0;
  plugin->dispatcher(plugin, effGetEffectName, 0, 0, buffer, 0);
  if(buffer[0])
    effectName = QString(buffer);

  buffer[0] = 0;
  plugin->dispatcher(plugin, effGetVendorString, 0, 0, buffer, 0);
  if (buffer[0])
    vendorString = QString(buffer);

  buffer[0] = 0;
  plugin->dispatcher(plugin, effGetProductString, 0, 0, buffer, 0);
  if (buffer[0])
    productString = QString(buffer);

  vendorVersion = plugin->dispatcher(plugin, effGetVendorVersion, 0, 0, NULL, 0);

  // Some (older) plugins don't have any of these strings. We only have the filename to use.
  if(effectName.isEmpty())
    effectName = fi.completeBaseName();
  if(productString.isEmpty())
    //productString = fi.completeBaseName();
    productString = effectName;
  
  // Make sure it doesn't already exist.
  for(is = MusEGlobal::synthis.begin(); is != MusEGlobal::synthis.end(); ++is)
    if((*is)->name() == effectName && (*is)->baseName() == fi.completeBaseName())
      goto _ending;
  
  // "2 = VST2.x, older versions return 0". Observed 2400 on all the ones tested so far.
  vst_version = plugin->dispatcher(plugin, effGetVstVersion, 0, 0, NULL, 0.0f);
  if(!((plugin->flags & effFlagsIsSynth) || (vst_version >= 2 && plugin->dispatcher(plugin, effCanDo, 0, 0,(void*) "receiveVstEvents", 0.0f) > 0)))
  {
    if(MusEGlobal::debugMsg)
      fprintf(stderr, "Plugin is not a synth\n");
    goto _ending;  
  }

  new_synth = new VstNativeSynth(fi, plugin, effectName, productString, vendorString, QString::number(vendorVersion)); 
  
  if(MusEGlobal::debugMsg)
    fprintf(stderr, "scanVstNativeLib: adding vst synth plugin:%s name:%s effectName:%s vendorString:%s productString:%s vstver:%d\n",
            fi.filePath().toLatin1().constData(),
            fi.completeBaseName().toLatin1().constData(),
            effectName.toLatin1().constData(),
            vendorString.toLatin1().constData(),
            productString.toLatin1().constData(),
            vst_version
            );

  MusEGlobal::synthis.push_back(new_synth);

_ending: ;
  
  //plugin->dispatcher(plugin, effMainsChanged, 0, 0, NULL, 0);
  plugin->dispatcher(plugin, effClose, 0, 0, NULL, 0);
  dlclose(handle);
}

//---------------------------------------------------------
//   scanVstDir
//---------------------------------------------------------

static void scanVstNativeDir(const QString& s)
{
      if (MusEGlobal::debugMsg)
            fprintf(stderr, "scan vst native plugin dir <%s>\n", s.toLatin1().constData());
      QDir pluginDir(s, QString("*.so"), QDir::Unsorted, QDir::Files);
      if(!pluginDir.exists())
        return;
      QStringList list = pluginDir.entryList();
      int count = list.count();
      for(int i = 0; i < count; ++i)
      {
        if(MusEGlobal::debugMsg)
          fprintf(stderr, "scanVstNativeDir: found %s\n", (s + QString("/") + list[i]).toLatin1().constData());

        QFileInfo fi(s + QString("/") + list[i]);
        scanVstNativeLib(fi);
      }
}

//---------------------------------------------------------
//   initVST_Native
//---------------------------------------------------------

void initVST_Native()
      {
      std::string s;
      const char* vstPath = getenv("VST_NATIVE_PATH");
      if (vstPath)
      {
        if (MusEGlobal::debugMsg)
            fprintf(stderr, "scan native vst: VST_NATIVE_PATH is: %s\n", vstPath);
      }
      else
      {
        if (MusEGlobal::debugMsg)
            fprintf(stderr, "scan native vst: VST_NATIVE_PATH not set\n");
      }
      
      if(!vstPath)
      {
        vstPath = getenv("VST_PATH");
        if (vstPath)
        {
          if (MusEGlobal::debugMsg)
              fprintf(stderr, "scan native vst: VST_PATH is: %s\n", vstPath);
        }
        else
        {
          if (MusEGlobal::debugMsg)
              fprintf(stderr, "scan native vst: VST_PATH not set\n");
          const char* home = getenv("HOME");
          s = std::string(home) + std::string("/vst:/usr/local/lib64/vst:/usr/local/lib/vst:/usr/lib64/vst:/usr/lib/vst");
          vstPath = s.c_str();
          if (MusEGlobal::debugMsg)
              fprintf(stderr, "scan native vst: defaulting to path: %s\n", vstPath);
        }
      }
      
      const char* p = vstPath;
      while (*p != '\0') {
            const char* pe = p;
            while (*pe != ':' && *pe != '\0')
                  pe++;

            int n = pe - p;
            if (n) {
                  char* buffer = new char[n + 1];
                  strncpy(buffer, p, n);
                  buffer[n] = '\0';
                  scanVstNativeDir(QString(buffer));
                  delete[] buffer;
                  }
            p = pe;
            if (*p == ':')
                  p++;
            }
      }

//---------------------------------------------------------
//   VstNativeSynth
//---------------------------------------------------------

VstNativeSynth::VstNativeSynth(const QFileInfo& fi, AEffect* plugin, const QString& label, const QString& desc, const QString& maker, const QString& ver)
  : Synth(fi, label, desc, maker, ver)
{
  _handle = NULL;
  _hasGui = plugin->flags & effFlagsHasEditor;
  _inports = plugin->numInputs;
  _outports = plugin->numOutputs;
  _controlInPorts = plugin->numParams;
  _inPlaceCapable = false; //(plugin->flags & effFlagsCanReplacing) && (_inports == _outports) && MusEGlobal::config.vstInPlace;
#ifndef VST_VESTIGE_SUPPORT
  _hasChunks = plugin->flags & effFlagsProgramChunks;
#else
  _hasChunks = false;
#endif
  
  _flags = 0;
  _vst_version = 0;
  _vst_version = plugin->dispatcher(plugin, effGetVstVersion, 0, 0, NULL, 0.0f);
  // "2 = VST2.x, older versions return 0". Observed 2400 on all the ones tested so far.
  if(_vst_version >= 2)
  {
    if(plugin->dispatcher(plugin, effCanDo, 0, 0, (void*)"receiveVstEvents", 0.0f) > 0)
      _flags |= canReceiveVstEvents;
    if(plugin->dispatcher(plugin, effCanDo, 0, 0, (void*)"sendVstEvents", 0.0f) > 0)
      _flags |= canSendVstEvents;
    if(plugin->dispatcher(plugin, effCanDo, 0, 0, (void*)"sendVstMidiEvent", 0.0f) > 0)
      _flags |= canSendVstMidiEvents;
    if(plugin->dispatcher(plugin, effCanDo, 0, 0, (void*)"sendVstTimeInfo", 0.0f) > 0)
      _flags |= canSendVstTimeInfo;
    if(plugin->dispatcher(plugin, effCanDo, 0, 0, (void*)"receiveVstMidiEvent", 0.0f) > 0)
      _flags |= canReceiveVstMidiEvents;
    if(plugin->dispatcher(plugin, effCanDo, 0, 0, (void*)"receiveVstTimeInfo", 0.0f) > 0)
      _flags |= canReceiveVstTimeInfo;
    if(plugin->dispatcher(plugin, effCanDo, 0, 0, (void*)"offline", 0.0f) > 0)
      _flags |=canProcessOffline;
    if(plugin->dispatcher(plugin, effCanDo, 0, 0, (void*)"plugAsChannelInsert", 0.0f) > 0)
      _flags |= canUseAsInsert;
    if(plugin->dispatcher(plugin, effCanDo, 0, 0, (void*)"plugAsSend", 0.0f) > 0)
      _flags |= canUseAsSend;
    if(plugin->dispatcher(plugin, effCanDo, 0, 0, (void*)"mixDryWet", 0.0f) > 0)
      _flags |= canMixDryWet;
    if(plugin->dispatcher(plugin, effCanDo, 0, 0, (void*)"midiProgramNames", 0.0f) > 0)
      _flags |= canMidiProgramNames;
  }
}

//---------------------------------------------------------
//   incInstances
//---------------------------------------------------------

void VstNativeSynth::incInstances(int val)
{
  _instances += val;
  if(_instances == 0)
  {
    if(_handle)
    {
      #ifdef VST_NATIVE_DEBUG
      fprintf(stderr, "VstNativeSynth::incInstances no more instances, closing library\n");
      #endif

      dlclose(_handle);
    }
    _handle = NULL;
    iIdx.clear();
    oIdx.clear();
    rpIdx.clear();
    midiCtl2PortMap.clear();
    port2MidiCtlMap.clear();
  }
}

//---------------------------------------------------------
//   instantiate
//---------------------------------------------------------

AEffect* VstNativeSynth::instantiate()
{
  int inst_num = _instances;
  inst_num++;
  QString n;
  n.setNum(inst_num);
  QString instanceName = baseName() + "-" + n;
  QByteArray ba = info.filePath().toLatin1();
  const char* path = ba.constData();
  void* hnd = _handle;
  int vst_version;

  if(hnd == NULL)
  {
    hnd = dlopen(path, RTLD_NOW);
    if(hnd == NULL)
    {
      fprintf(stderr, "dlopen(%s) failed: %s\n", path, dlerror());
      return NULL;
    }
  }

  AEffect *(*getInstance)(audioMasterCallback);
  getInstance = (AEffect*(*)(audioMasterCallback))dlsym(hnd, NEW_PLUGIN_ENTRY_POINT);
  if(!getInstance)
  {
    if(MusEGlobal::debugMsg)
    {
      fprintf(stderr, "VST 2.4 entrypoint \"" NEW_PLUGIN_ENTRY_POINT "\" not found in library %s, looking for \""
                      OLD_PLUGIN_ENTRY_POINT "\"\n", path);
    }

    getInstance = (AEffect*(*)(audioMasterCallback))dlsym(hnd, OLD_PLUGIN_ENTRY_POINT);
    if(!getInstance)
    {
      fprintf(stderr, "ERROR: VST entrypoints \"" NEW_PLUGIN_ENTRY_POINT "\" or \""
                      OLD_PLUGIN_ENTRY_POINT "\" not found in library\n");
      dlclose(hnd);
      return NULL;
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

  AEffect *plugin = getInstance(vstNativeHostCallback);
  if(!plugin)
  {
    fprintf(stderr, "ERROR: Failed to instantiate plugin in VST library \"%s\"\n", path);
    dlclose(hnd);
    return NULL;
  }
  else if(MusEGlobal::debugMsg)
    fprintf(stderr, "plugin instantiated\n");

  if(plugin->magic != kEffectMagic)
  {
    fprintf(stderr, "Not a VST plugin in library \"%s\"\n", path);
    dlclose(hnd);
    return NULL;
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

  plugin->dispatcher(plugin, effOpen, 0, 0, NULL, 0);

  // "2 = VST2.x, older versions return 0". Observed 2400 on all the ones tested so far.
  vst_version = plugin->dispatcher(plugin, effGetVstVersion, 0, 0, NULL, 0.0f);
  if(!((plugin->flags & effFlagsIsSynth) || (vst_version >= 2 && plugin->dispatcher(plugin, effCanDo, 0, 0,(void*) "receiveVstEvents", 0.0f) > 0)))
  {
    if(MusEGlobal::debugMsg)
      fprintf(stderr, "Plugin is not a synth\n");
    goto _error;
  }

  ++_instances;
  _handle = hnd;
  
  plugin->dispatcher(plugin, effOpen, 0, 0, NULL, 0);
  //plugin->dispatcher(plugin, effSetProgram, 0, 0, NULL, 0.0f); // REMOVE Tim. Or keep?
  return plugin;

_error:
  //plugin->dispatcher(plugin, effMainsChanged, 0, 0, NULL, 0);
  plugin->dispatcher(plugin, effClose, 0, 0, NULL, 0);
  dlclose(hnd);
  return NULL;
}

//---------------------------------------------------------
//   createSIF
//---------------------------------------------------------

SynthIF* VstNativeSynth::createSIF(SynthI* s)
      {
      VstNativeSynthIF* sif = new VstNativeSynthIF(s);
      sif->init(this);
      return sif;
      }
  
//---------------------------------------------------------
//   VstNativeSynthIF
//---------------------------------------------------------

VstNativeSynthIF::VstNativeSynthIF(SynthI* s) : SynthIF(s)
{
      _guiVisible = false;
      _synth = NULL;
      _plugin = NULL;
      _editor = NULL;
      _inProcess = false;
       _controls = NULL;
//       controlsOut = 0;
      _audioInBuffers = NULL;
      _audioInSilenceBuf = NULL;
      _audioOutBuffers = NULL;
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
}

//---------------------------------------------------------
//   init
//---------------------------------------------------------

bool VstNativeSynthIF::init(Synth* s)
      {
      _synth = (VstNativeSynth*)s;
      _plugin = _synth->instantiate();
      if(!_plugin)
        return false;
      _plugin->user = this;

      queryPrograms();
      
      unsigned long outports = _synth->outPorts();
      if(outports != 0)
      {
        _audioOutBuffers = new float*[outports];
        for(unsigned long k = 0; k < outports; ++k)
        {
          posix_memalign((void**)&_audioOutBuffers[k], 16, sizeof(float) * MusEGlobal::segmentSize);
          memset(_audioOutBuffers[k], 0, sizeof(float) * MusEGlobal::segmentSize);
        }
      }

      unsigned long inports = _synth->inPorts();
      if(inports != 0)
      {
        _audioInBuffers = new float*[inports];
        for(unsigned long k = 0; k < inports; ++k)
        {
          posix_memalign((void**)&_audioInBuffers[k], 16, sizeof(float) * MusEGlobal::segmentSize);
          memset(_audioInBuffers[k], 0, sizeof(float) * MusEGlobal::segmentSize);
          _iUsedIdx.push_back(false); // Start out with all false.
        }
        
        posix_memalign((void**)&_audioInSilenceBuf, 16, sizeof(float) * MusEGlobal::segmentSize);
        memset(_audioInSilenceBuf, 0, sizeof(float) * MusEGlobal::segmentSize);
      }

      unsigned long controlPorts = _synth->inControls();
      if(controlPorts != 0)
        _controls = new Port[controlPorts];
      else
        _controls = NULL;

      //_synth->midiCtl2PortMap.clear();
      //_synth->port2MidiCtlMap.clear();

      for(unsigned long i = 0; i < controlPorts; ++i)
      {
        _controls[i].idx = i;
        //float val;  // TODO
        //ladspaDefaultValue(ld, k, &val);   // FIXME TODO
        float val = _plugin->getParameter(_plugin, i);  // TODO
        _controls[i].val    = val;
        _controls[i].tmpVal = val;
        _controls[i].enCtrl  = true;
        _controls[i].en2Ctrl = true;

        // Support a special block for synth ladspa controllers.
        // Put the ID at a special block after plugins (far after).
        int id = genACnum(MAX_PLUGINS, i);
        const char* param_name = paramName(i);

        // TODO FIXME!
        ///float min, max;
        ///ladspaControlRange(ld, k, &min, &max);
        float min = 0.0, max = 1.0;

        CtrlList* cl;
        CtrlListList* cll = ((MusECore::AudioTrack*)synti)->controller();
        iCtrlList icl = cll->find(id);
        if (icl == cll->end())
        {
          cl = new CtrlList(id);
          cll->add(cl);
          cl->setCurVal(_controls[i].val);
          //cl->setCurVal(_plugin->getParameter(_plugin, i));
        }
        else
        {
          cl = icl->second;
          _controls[i].val = cl->curVal();
          
#ifndef VST_VESTIGE_SUPPORT
          if(dispatch(effCanBeAutomated, i, 0, NULL, 0.0f) == 1)
          {
#endif            
            double v = cl->curVal();
            if(v != _plugin->getParameter(_plugin, i))
              _plugin->setParameter(_plugin, i, v);
#ifndef VST_VESTIGE_SUPPORT
          }

  #ifdef VST_NATIVE_DEBUG
          else  
            fprintf(stderr, "VstNativeSynthIF::init %s parameter:%lu cannot be automated\n", name().toLatin1().constData(), i);
  #endif

#endif
        }
        
        cl->setRange(min, max);
        cl->setName(QString(param_name));
        //cl->setValueType(ladspaCtrlValueType(ld, k));
        cl->setValueType(ctrlValueType(i));
        //cl->setMode(ladspaCtrlMode(ld, k));
        cl->setMode(ctrlMode(i));
      }

      activate();     
      doSelectProgram(synti->_curBankH, synti->_curBankL, synti->_curProgram);
      //doSelectProgram(synti->_curProgram);
      
      return true;
      }

//---------------------------------------------------------
//   resizeEditor
//---------------------------------------------------------

bool VstNativeSynthIF::resizeEditor(int w, int h)
{
  if(!_editor || w <= 0 || h <= 0)
    return false;
  _editor->resize(w, h);
  return true;
}

//---------------------------------------------------------
//   hostCallback
//---------------------------------------------------------

VstIntPtr VstNativeSynthIF::hostCallback(VstInt32 opcode, VstInt32 index, VstIntPtr value, void* ptr, float opt)
      {
      static VstTimeInfo _timeInfo;

#ifdef VST_NATIVE_DEBUG
      if(opcode != audioMasterGetTime)
        fprintf(stderr, "VstNativeSynthIF::hostCallback %s opcode:%ld\n", name().toLatin1().constData(), opcode);
#endif

      switch (opcode) {
            case audioMasterAutomate:
                  // index, value, returns 0
                  ///_plugin->setParameter (_plugin, index, opt);
                  guiControlChanged(index, opt);
                  return 0;

            case audioMasterVersion:
                  // vst version, currently 2 (0 for older)
                  return 2300;

            case audioMasterCurrentId:
                  // returns the unique id of a plug that's currently
                  // loading
                  return 0;

            case audioMasterIdle:
                  // call application idle routine (this will
                  // call effEditIdle for all open editors too)
                  //_plugin->updateParamValues(false);
                  //_plugin->dispatcher(_plugin, effEditIdle, 0, 0, NULL, 0.0f);
                  idleEditor();
                  return 0;

            case audioMasterGetTime:
            {
                  // returns const VstTimeInfo* (or 0 if not supported)
                  // <value> should contain a mask indicating which fields are required
                  // (see valid masks above), as some items may require extensive
                  // conversions

                  memset(&_timeInfo, 0, sizeof(_timeInfo));

                  unsigned int curr_frame = MusEGlobal::audio->pos().frame();
                  _timeInfo.samplePos = (double)curr_frame;
                  _timeInfo.sampleRate = (double)MusEGlobal::sampleRate;
                  _timeInfo.flags = 0;

                  if(value & (kVstBarsValid | kVstTimeSigValid | kVstTempoValid | kVstPpqPosValid))
                  {
                    Pos p(MusEGlobal::extSyncFlag.value() ? MusEGlobal::audio->tickPos() : curr_frame, MusEGlobal::extSyncFlag.value() ? true : false);
                    // Can't use song pos - it is only updated every (slow) GUI heartbeat !
                    //Pos p(MusEGlobal::extSyncFlag.value() ? MusEGlobal::song->cpos() : pos->frame, MusEGlobal::extSyncFlag.value() ? true : false);

                    // TODO
                    int p_bar, p_beat, p_tick;
                    p.mbt(&p_bar, &p_beat, &p_tick);
                    
#ifndef VST_VESTIGE_SUPPORT
                    _timeInfo.barStartPos = Pos(p_bar, 0, 0).tick();
                    _timeInfo.ppqPos = MusEGlobal::audio->tickPos();
#else
                    *((double*)&_timeInfo.empty2[0]) = (double)Pos(p_bar, 0, 0).tick() / 120.0;
                    *((double*)&_timeInfo.empty1[8]) = (double)MusEGlobal::audio->tickPos() / 120.0;  
#endif
                    //pos->bar++;
                    //pos->beat++;

                    int z, n;
                    AL::sigmap.timesig(p.tick(), z, n);

#ifndef VST_VESTIGE_SUPPORT
                    _timeInfo.timeSigNumerator = (long)z;
                    _timeInfo.timeSigDenominator = (long)n;
#else
                    _timeInfo.timeSigNumerator = z;
                    _timeInfo.timeSigDenominator = n;
#endif

                    // TODO
                    ////pos->ticks_per_beat = 24;
                    //pos->ticks_per_beat = MusEGlobal::config.division;

                    double tempo = MusEGlobal::tempomap.tempo(p.tick());
                    _timeInfo.tempo = (60000000.0 / tempo) * double(MusEGlobal::tempomap.globalTempo())/100.0;
                    _timeInfo.flags |= (kVstBarsValid | kVstTimeSigValid | kVstTempoValid | kVstPpqPosValid);

#ifdef VST_NATIVE_DEBUG
                    fprintf(stderr, "VstNativeSynthIF::hostCallback master time: sample pos:%f samplerate:%f sig num:%ld den:%ld tempo:%f\n",
                      _timeInfo.samplePos, _timeInfo.sampleRate, _timeInfo.timeSigNumerator, _timeInfo.timeSigDenominator, _timeInfo.tempo);
#endif
                  }

                  if(MusEGlobal::audio->isPlaying())
                    _timeInfo.flags |= (kVstTransportPlaying | kVstTransportChanged);
                  return (long)&_timeInfo;
            }
            
            case audioMasterProcessEvents:
                  // VstEvents* in <ptr>
                  return 0;  // TODO:

            case audioMasterIOChanged:
                   // numInputs and/or numOutputs has changed
                  return 0;

            case audioMasterSizeWindow:
                  // index: width, value: height
                  if(resizeEditor(int(index), int(value)))
                    return 1; // supported.
                  return 0;

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
                  // returns: 0: not supported,
                  // 1: currently in user thread (gui)
                  // 2: currently in audio thread (where process is called)
                  // 3: currently in 'sequencer' thread (midi, timer etc)
                  // 4: currently offline processing and thus in user thread
                  // other: not defined, but probably pre-empting user thread.
                  if(_inProcess)
                    return 2;
                  else
                    return 1;

            case audioMasterGetAutomationState:
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
                     !strcmp((char*)ptr, "supplyIdle"))               
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
                  // something has changed, update 'multi-fx' display

                  //_plugin->updateParamValues(false);   
                  //QApplication::processEvents();     // REMOVE Tim. Or keep. Commented in QTractor.

                  _plugin->dispatcher(_plugin, effEditIdle, 0, 0, NULL, 0.0f);  // ?
                  
                  return 0;

            case audioMasterBeginEdit:
                  // begin of automation session (when mouse down), parameter index in <index>
                  guiAutomationBegin(index);
                  return 1;  

            case audioMasterEndEdit:
                  // end of automation session (when mouse up),     parameter index in <index>
                  guiAutomationEnd(index);
                  return 1;  

#if 0 //ifndef VST_VESTIGE_SUPPORT
            case audioMasterOpenFileSelector:
                  // open a fileselector window with VstFileSelect* in <ptr>
                  return 0;
                  
            case audioMasterCloseFileSelector:
                  return 0;
#endif

#ifdef VST_FORCE_DEPRECATED
                  
            case audioMasterGetSpeakerArrangement:
                  // (long)input in <value>, output in <ptr>
                  return 0;

            case audioMasterPinConnected:
                  // inquire if an input or output is beeing connected;
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

                  
            default:
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

  _plugin->dispatcher(_plugin, effEditIdle, 0, 0, NULL, 0.0f);
  if(_editor)
    _editor->update();
}

//---------------------------------------------------------
//   nativeGuiVisible
//---------------------------------------------------------

bool VstNativeSynthIF::nativeGuiVisible() const
      {
      return _guiVisible;
      }

//---------------------------------------------------------
//   guiVisible
//---------------------------------------------------------

bool VstNativeSynthIF::guiVisible() const
      {
      return _gui && _gui->isVisible();
      }

//---------------------------------------------------------
//   showGui
//---------------------------------------------------------

void VstNativeSynthIF::showGui(bool v)
{
    if (v) {
            if (_gui == 0)
                makeGui();
            _gui->show();
            }
    else {
            if (_gui)
                _gui->hide();
            }
}

//---------------------------------------------------------
//   showGui
//---------------------------------------------------------

void VstNativeSynthIF::showNativeGui(bool v)
      {
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
          Qt::WindowFlags wflags = Qt::Window
                  | Qt::CustomizeWindowHint
                  | Qt::WindowTitleHint
                  | Qt::WindowSystemMenuHint
                  | Qt::WindowMinMaxButtonsHint
                  | Qt::WindowCloseButtonHint;
          _editor = new MusEGui::VstNativeEditor(NULL, wflags);
          _editor->open(this);
        }
      }
      else
      {
        if(_editor)
        {
          delete _editor;
          //_editor = NULL;  // No - done in editorDeleted.
        }
      }
      _guiVisible = v;
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
  _editor = NULL;
}

//---------------------------------------------------------
//   receiveEvent
//---------------------------------------------------------

MidiPlayEvent VstNativeSynthIF::receiveEvent()
      {
      return MidiPlayEvent();
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
      //return _plugin->numOutputs;
      return _plugin->numOutputs > MAX_CHANNELS ? MAX_CHANNELS : _plugin->numOutputs ;
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
        delete _editor;
        _editor = NULL;
        _guiVisible = false;
      }

      deactivate();
      if (_plugin) {
            _plugin->dispatcher (_plugin, effClose, 0, 0, NULL, 0);
            _plugin = NULL;
            }
      }

//---------------------------------------------------------
//   queryPrograms
//---------------------------------------------------------

void VstNativeSynthIF::queryPrograms()
{
      char buf[256];
      programs.clear();
      int num_progs = _plugin->numPrograms;
      int iOldIndex = dispatch(effGetProgram, 0, 0, NULL, 0.0f);
      bool need_restore = false;
      for(int prog = 0; prog < num_progs; ++prog)
      {
        buf[0] = 0;

//#ifndef VST_VESTIGE_SUPPORT
        // value = category. -1 = regular linear.
        if(dispatch(effGetProgramNameIndexed, prog, -1, buf, 0.0f) == 0)  
        {
//#endif
          dispatch(effSetProgram, 0, prog, NULL, 0.0f);
          dispatch(effGetProgramName, 0, 0, buf, 0.0f);
          need_restore = true;
//#ifndef VST_VESTIGE_SUPPORT
        }
//#endif

        int bankH = (prog >> 14) & 0x7f;
        int bankL = (prog >> 7) & 0x7f;
        int patch = prog & 0x7f;
        VST_Program p;
        p.name    = QString(buf);
        //p.program = prog & 0x7f;
        //p.bank    = prog << 7;
        p.program = (bankH << 16) | (bankL << 8) | patch;
        programs.push_back(p);
      }

      // Restore current program.
      if(need_restore) // && num_progs > 0)
      { 
        dispatch(effSetProgram, 0, iOldIndex, NULL, 0.0f);
        fprintf(stderr, "FIXME: VstNativeSynthIF::queryPrograms(): effGetProgramNameIndexed returned 0. Used ugly effSetProgram/effGetProgramName instead\n");
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

  if(bankH == 0xff)
    bankH = 0;
  if(bankL == 0xff)
    bankL = 0;
  if(prog == 0xff)
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
    //if(dispatch(effBeginSetProgram, 0, 0, NULL, 0.0f) == 1)  // TESTED: Usually it did not acknowledge. So IGNORE it.
    dispatch(effBeginSetProgram, 0, 0, NULL, 0.0f);
    //{
#endif      
      dispatch(effSetProgram, 0, p, NULL, 0.0f);
      //dispatch(effSetProgram, 0, prog, NULL, 0.0f);
      // "host calls this after the new program (effSetProgram) has been loaded"
#ifndef VST_VESTIGE_SUPPORT
      dispatch(effEndSetProgram, 0, 0, NULL, 0.0f);
    //}
    //else
    //  fprintf(stderr, "VstNativeSynthIF::doSelectProgram bankH:%d bankL:%d prog:%d Effect did not acknowledge effBeginSetProgram\n", bankH, bankL, prog);
#endif
  //}
    
  // TODO: Is this true of VSTs? See the similar section in dssihost.cpp  // REMOVE Tim.
  //   "A plugin is permitted to re-write the values of its input control ports when select_program is called.
  //    The host should re-read the input control port values and update its own records appropriately.
  //    (This is the only circumstance in which a DSSI plugin is allowed to modify its own input ports.)"   From dssi.h
  // Need to update the automation value, otherwise it overwrites later with the last automation value.
  if(id() != -1)
  {
    const unsigned long sic = _synth->inControls();
    for(unsigned long k = 0; k < sic; ++k)
    {
      // We're in the audio thread context: no need to send a message, just modify directly.
      //synti->setPluginCtrlVal(genACnum(id(), k), _controls[k].val);
      //synti->setPluginCtrlVal(genACnum(id(), k), _plugin->getParameter(_plugin, k));
      const float v = _plugin->getParameter(_plugin, k);
      _controls[k].val = v;
      synti->setPluginCtrlVal(genACnum(id(), k), v);
    }
  }

//   // Reset parameters default value...   // TODO ? 
//   AEffect *pVstEffect = vst_effect(0);
//   if (pVstEffect) {
//           const qtractorPlugin::Params& params = qtractorPlugin::params();
//           qtractorPlugin::Params::ConstIterator param = params.constBegin();
//           for ( ; param != params.constEnd(); ++param) {
//                   qtractorPluginParam *pParam = param.value();
//                   float *pfValue = pParam->subject()->data();
//                   *pfValue = pVstEffect->getParameter(pVstEffect, pParam->index());
//                   pParam->setDefaultValue(*pfValue);
//           }
//   }
  
}

//---------------------------------------------------------
//   getPatchName
//---------------------------------------------------------

const char* VstNativeSynthIF::getPatchName(int /*chan*/, int prog, bool /*drum*/)
{
  unsigned long  program = prog & 0x7f;
  unsigned long  lbank   = (prog >> 8) & 0xff;
  unsigned long  hbank   = (prog >> 16) & 0xff;
  if (lbank == 0xff)
        lbank = 0;
  if (hbank == 0xff)
        hbank = 0;
  unsigned long p = (hbank << 16) | (lbank << 8) | program;
  int vp          = (hbank << 14) | (lbank << 7) | program;
  if((int)vp < _plugin->numPrograms)
  {
    for(std::vector<VST_Program>::const_iterator i = programs.begin(); i != programs.end(); ++i)
    {
      if(i->program == p)
        return i->name.toLatin1().constData();
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
  // So make sure we're up to date by calling queryPrograms.
  queryPrograms();
  
  menu->clear();

  for (std::vector<VST_Program>::const_iterator i = programs.begin(); i != programs.end(); ++i)
       {
        //int bank = i->bank;
        int prog = i->program;
        //int id   = (bank << 7) + prog;

        QAction *act = menu->addAction(i->name);
        //act->setData(id);
        act->setData(prog);
        }

}

//---------------------------------------------------------
//   getParameter
//---------------------------------------------------------

float VstNativeSynthIF::getParameter(unsigned long idx) const
      {
      if(idx >= _synth->inControls())
      {
        fprintf(stderr, "VstNativeSynthIF::getParameter param number %lu out of range of ports:%lu\n", idx, _synth->inControls());
        return 0.0;
      }

      return _plugin->getParameter(_plugin, idx);
      }

//---------------------------------------------------------
//   setParameter
//---------------------------------------------------------

void VstNativeSynthIF::setParameter(unsigned long idx, float value)
      {
      //_plugin->setParameter(_plugin, idx, value);
      addScheduledControlEvent(idx, value, MusEGlobal::audio->curFrame());
      }

//---------------------------------------------------------
//   guiAutomationBegin
//---------------------------------------------------------

void VstNativeSynthIF::guiAutomationBegin(unsigned long param_idx)
{
  AutomationType at = AUTO_OFF;
  MusECore::AudioTrack* t = track();
  if(t)
    at = t->automationType();

  // FIXME TODO: This stuff should probably be sent as control FIFO events.
  //             And probably not safe - accessing from gui and audio threads...
    
  if (at == AUTO_READ || at == AUTO_TOUCH || at == AUTO_WRITE)
    enableController(param_idx, false);

  int plug_id = id();

  if(plug_id == -1)
    return;

  plug_id = MusECore::genACnum(plug_id, param_idx);

  //if(params[param].type == GuiParam::GUI_SLIDER)
  //{
    //double val = ((Slider*)params[param].actuator)->value();
    float val = param(param_idx);
    // FIXME TODO:
    //if (LADSPA_IS_HINT_LOGARITHMIC(params[param].hint))
    //      val = pow(10.0, val/20.0);
    //else if (LADSPA_IS_HINT_INTEGER(params[param].hint))
    //      val = rint(val);
    //plugin->setParam(param, val);
    //((DoubleLabel*)params[param].label)->setValue(val);

    if(t)
    {
      t->setPluginCtrlVal(plug_id, val);
      t->startAutoRecord(plug_id, val);
    }
  //}
//   else if(params[param].type == GuiParam::GUI_SWITCH)
//   {
//     float val = (float)((CheckBox*)params[param].actuator)->isChecked();
//     plugin->setParam(param, val);
// 
//     if(t)
//     {
//       t->setPluginCtrlVal(plug_id, val);
//       t->startAutoRecord(plug_id, val);
//     }
//   }
}

//---------------------------------------------------------
//   guiAutomationEnd
//---------------------------------------------------------

void VstNativeSynthIF::guiAutomationEnd(unsigned long param_idx)
{
  AutomationType at = AUTO_OFF;
  MusECore::AudioTrack* t = track();
  if(t)
    at = t->automationType();

  // FIXME TODO: This stuff should probably be sent as control FIFO events.
  //             And probably not safe - accessing from gui and audio threads...
    
  // Special for switch - don't enable controller until transport stopped.
  if ((at == AUTO_OFF) ||
      (at == AUTO_READ) ||
      (at == AUTO_TOUCH)) // && (params[param].type != GuiParam::GUI_SWITCH ||  // FIXME TODO
                         //   !MusEGlobal::audio->isPlaying()) ) )
    enableController(param_idx, true);

  int plug_id = id();
  if(!t || plug_id == -1)
    return;
  plug_id = MusECore::genACnum(plug_id, param_idx);

  //if(params[param].type == GuiParam::GUI_SLIDER)
  //{
    //double val = ((Slider*)params[param].actuator)->value();
    float val = param(param_idx);
    // FIXME TODO:
    //if (LADSPA_IS_HINT_LOGARITHMIC(params[param].hint))
    //      val = pow(10.0, val/20.0);
    //else if (LADSPA_IS_HINT_INTEGER(params[param].hint))
    //      val = rint(val);
    t->stopAutoRecord(plug_id, val);
  //}
}
      
//---------------------------------------------------------
//   guiControlEventHandler
//---------------------------------------------------------

int VstNativeSynthIF::guiControlChanged(unsigned long param_idx, float value)
{
  #ifdef VST_NATIVE_DEBUG
  fprintf(stderr, "VstNativeSynthIF::guiControlChanged received oscControl port:%lu val:%f\n", param_idx, value);
  #endif

  if(param_idx >= _synth->inControls())
  {
    fprintf(stderr, "VstNativeSynthIF::guiControlChanged: port number:%lu is out of range of index list size:%lu\n", param_idx, _synth->inControls());
    return 0;
  }

  ControlEvent ce;
  ce.unique = false; // Not used for native vst.     
  ce.fromGui = true; // It came form the plugin's own GUI.
  ce.idx = param_idx;
  ce.value = value;
  // don't use timestamp(), because it's circular, which is making it impossible to deal
  // with 'modulo' events which slip in 'under the wire' before processing the ring buffers.
  ce.frame = MusEGlobal::audio->curFrame();

  if(_controlFifo.put(ce))
  {
    fprintf(stderr, "VstNativeSynthIF::guiControlChanged: fifo overflow: in control number:%lu\n", param_idx);
  }

  // FIXME TODO: This stuff should probably be sent as control FIFO events.
  //             And probably not safe - accessing from gui and audio threads...
  
  // Record automation:
  // Take care of this immediately rather than in the fifo processing.
  if(id() != -1)
  {
    unsigned long pid = genACnum(id(), param_idx);
    AutomationType at = synti->automationType();

    if ((at == AUTO_WRITE) ||
        (at == AUTO_TOUCH && MusEGlobal::audio->isPlaying()))
      enableController(param_idx, false);

    synti->recordAutomation(pid, value);
  }

  return 0;
}

//---------------------------------------------------------
//   write
//---------------------------------------------------------

void VstNativeSynthIF::write(int level, Xml& xml) const
{
#ifndef VST_VESTIGE_SUPPORT
  if(_synth->hasChunks())
  {
    //---------------------------------------------
    // dump current state of synth
    //---------------------------------------------
    fprintf(stderr, "%s: commencing chunk data dump, plugin api version=%d\n", name().toLatin1().constData(), _synth->vstVersion());
    unsigned long len = 0;
    void* p = 0;
    len = dispatch(effGetChunk, 0, 0, &p, 0.0); // index 0: is bank 1: is program
    if (len)
    {
      xml.tag(level++, "midistate version=\"%d\"", SYNTH_MIDI_STATE_SAVE_VERSION);
      xml.nput(level++, "<event type=\"%d\"", Sysex);
      // 10 = 2 bytes header + "VSTSAVE" + 1 byte flags (compression etc)
      xml.nput(" datalen=\"%d\">\n", len+10);
      xml.nput(level, "");
      xml.nput("%02x %02x ", (char)MUSE_SYNTH_SYSEX_MFG_ID, (char)VST_NATIVE_SYNTH_UNIQUE_ID); // Wrap in a proper header
      xml.nput("56 53 54 53 41 56 45 "); // embed a save marker "string 'VSTSAVE'
      xml.nput("%02x ", (char)0); // No flags yet, only uncompressed supported for now. TODO
      for (unsigned long int i = 0; i < len; ++i)
      {
        if (i && (((i+10) % 16) == 0))
        {
          xml.nput("\n");
          xml.nput(level, "");
        }
        xml.nput("%02x ", ((char*)(p))[i] & 0xff);
      }
      xml.nput("\n");
      xml.tag(level--, "/event");
      xml.etag(level--, "midistate");
    }
  }
#else
  fprintf(stderr, "support for vst chunks not compiled in!\n");
#endif

  //---------------------------------------------
  // dump current state of synth
  //---------------------------------------------

  int params = _plugin->numParams;
  for (int i = 0; i < params; ++i)
  {
    float f = _plugin->getParameter(_plugin, i);
    xml.floatTag(level, "param", f);
  }
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

bool VstNativeSynthIF::processEvent(const MusECore::MidiPlayEvent& e, VstMidiEvent* event)
{
  int type = e.type();
  int chn = e.channel();
  int a   = e.dataA();
  int b   = e.dataB();

  #ifdef VST_NATIVE_DEBUG
  fprintf(stderr, "VstNativeSynthIF::processEvent midi event type:%d chn:%d a:%d b:%d\n", type, chn, a, b);
  #endif

  switch(type)
  {
    case MusECore::ME_NOTEON:
      #ifdef VST_NATIVE_DEBUG
      fprintf(stderr, "VstNativeSynthIF::processEvent midi event is MusECore::ME_NOTEON\n");
      #endif
      setVstEvent(event, (type | chn) & 0xff, a & 0x7f, b & 0x7f);
    break;
    case MusECore::ME_NOTEOFF:
      #ifdef VST_NATIVE_DEBUG
      fprintf(stderr, "VstNativeSynthIF::processEvent midi event is MusECore::ME_NOTEOFF\n");
      #endif
      setVstEvent(event, (type | chn) & 0xff, a & 0x7f, 0);
    break;
    case MusECore::ME_PROGRAM:
    {
      #ifdef VST_NATIVE_DEBUG
      fprintf(stderr, "VstNativeSynthIF::processEvent midi event is MusECore::ME_PROGRAM\n");
      #endif

      int bankH = (a >> 16) & 0xff;
      int bankL = (a >> 8) & 0xff;
      int prog = a & 0xff;
      synti->_curBankH = bankH;
      synti->_curBankL = bankL;
      synti->_curProgram = prog;
      doSelectProgram(bankH, bankL, prog);
      return false;  // Event pointer not filled. Return false.
    }
    break;
    case MusECore::ME_CONTROLLER:
    {
      #ifdef VST_NATIVE_DEBUG
      fprintf(stderr, "VstNativeSynthIF::processEvent midi event is MusECore::ME_CONTROLLER\n");
      #endif

      if((a == 0) || (a == 32))
        return false;

      if(a == MusECore::CTRL_PROGRAM)
      {
        #ifdef VST_NATIVE_DEBUG
        fprintf(stderr, "VstNativeSynthIF::processEvent midi event is MusECore::ME_CONTROLLER, dataA is MusECore::CTRL_PROGRAM\n");
        #endif

        int bankH = (b >> 16) & 0xff;
        int bankL = (b >> 8) & 0xff;
        int prog = b & 0xff;

        synti->_curBankH = bankH;
        synti->_curBankL = bankL;
        synti->_curProgram = prog;
        doSelectProgram(bankH, bankL, prog);
        return false; // Event pointer not filled. Return false.
      }

      if(a == MusECore::CTRL_PITCH)
      {
        #ifdef VST_NATIVE_DEBUG
        fprintf(stderr, "VstNativeSynthIF::processEvent midi event is MusECore::ME_CONTROLLER, dataA is MusECore::CTRL_PITCH\n");
        #endif
        int v = b + 8192;
        setVstEvent(event, (type | chn) & 0xff, v & 0x7f, (v >> 7) & 0x7f);
        return true;
      }

      if(a == MusECore::CTRL_AFTERTOUCH)
      {
        #ifdef VST_NATIVE_DEBUG
        fprintf(stderr, "VstNativeSynthIF::processEvent midi event is MusECore::ME_CONTROLLER, dataA is MusECore::CTRL_AFTERTOUCH\n");
        #endif
        setVstEvent(event, (type | chn) & 0xff, b & 0x7f);
        return true;
      }

      if((a | 0xff)  == MusECore::CTRL_POLYAFTER)
      {
        #ifdef VST_NATIVE_DEBUG
        fprintf(stderr, "VstNativeSynthIF::processEvent midi event is MusECore::ME_CONTROLLER, dataA is MusECore::CTRL_POLYAFTER\n");
        #endif
        setVstEvent(event, (type | chn) & 0xff, a & 0x7f, b & 0x7f);
        return true;
      }

      #ifdef VST_NATIVE_DEBUG
      fprintf(stderr, "VstNativeSynthIF::processEvent midi event is MusECore::ME_CONTROLLER, dataA is:%d\n", a);
      #endif
      
      // Regular controller. Pass it on.
      setVstEvent(event, (type | chn) & 0xff, a & 0x7f, b & 0x7f);
      
      return true; 

// REMOVE Tim. Or keep. TODO For native vsts? Or not...
//      
//       const LADSPA_Descriptor* ld = dssi->LADSPA_Plugin;
// 
//       MusECore::ciMidiCtl2LadspaPort ip = synth->midiCtl2PortMap.find(a);
//       // Is it just a regular midi controller, not mapped to a LADSPA port (either by the plugin or by us)?
//       // NOTE: There's no way to tell which of these controllers is supported by the plugin.
//       // For example sustain footpedal or pitch bend may be supported, but not mapped to any LADSPA port.
//       if(ip == synth->midiCtl2PortMap.end())
//       {
//         int ctlnum = a;
//         if(MusECore::midiControllerType(a) != MusECore::MidiController::Controller7)
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
//         ctlnum = k + (MusECore::CTRL_NRPN14_OFFSET + 0x2000);
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
//           ctlnum = DSSI_NRPN_NUMBER(c) + MusECore::CTRL_NRPN14_OFFSET;
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
    case MusECore::ME_PITCHBEND:
    {
      int v = a + 8192;
      setVstEvent(event, (type | chn) & 0xff, v & 0x7f, (v >> 7) & 0x7f);
    }
    break;
    case MusECore::ME_AFTERTOUCH:
      setVstEvent(event, (type | chn) & 0xff, a & 0x7f);
    break;
    case MusECore::ME_POLYAFTER:
      setVstEvent(event, (type | chn) & 0xff, a & 0x7f, b & 0x7f);
    break;
    case MusECore::ME_SYSEX:
      {
        #ifdef VST_NATIVE_DEBUG
        fprintf(stderr, "VstNativeSynthIF::processEvent midi event is MusECore::ME_SYSEX\n");
        #endif

        const unsigned char* data = e.data();
        if(e.len() >= 2)
        {
          if(data[0] == MUSE_SYNTH_SYSEX_MFG_ID)
          {
            if(data[1] == VST_NATIVE_SYNTH_UNIQUE_ID)
            {
              //if(e.len() >= 9)
              if(e.len() >= 10)
              {
                if (QString((const char*)(data + 2)).startsWith("VSTSAVE"))
                {
                  if(_synth->hasChunks())
                  {
#ifndef VST_VESTIGE_SUPPORT
                    int chunk_flags = data[9];
                    if(chunk_flags & VST_NATIVE_CHUNK_FLAG_COMPRESSED)
                      fprintf(stderr, "chunk flags:%x compressed chunks not supported yet.\n", chunk_flags);
                    else
                    {
                      fprintf(stderr, "%s: loading chunk from sysex!\n", name().toLatin1().constData());
                      // 10 = 2 bytes header + "VSTSAVE" + 1 byte flags (compression etc)
                      dispatch(effSetChunk, 0, e.len()-10, (void*)(data+10), 0.0); // index 0: is bank 1: is program
                    }
#else
                    fprintf(stderr, "support for vst chunks not compiled in!\n");
#endif
                  }
                  // Event not filled.
                  return false;
                }
              }
            }
          }
        }

        // DELETETHIS, 50 clean it up or fix it?
        /*
        // p3.3.39 Read the state of current bank and program and all input control values.
        // TODO: Needs to be better. See write().
        //else
        if (QString((const char*)e.data()).startsWith("PARAMSAVE"))
        {
          #ifdef VST_NATIVE_DEBUG
          fprintf(stderr, "VstNativeSynthIF::processEvent midi event is MusECore::ME_SYSEX PARAMSAVE\n");
          #endif

          unsigned long dlen = e.len() - 9; // Minus "PARAMSAVE"
          if(dlen > 0)
          {
            //if(dlen < 2 * sizeof(unsigned long))
            if(dlen < (2 + 2 * sizeof(unsigned long))) // Version major and minor bytes, bank and program.
              fprintf(stderr, "VstNativeSynthIF::processEvent Error: PARAMSAVE data length does not include at least version major and minor, bank and program!\n");
            else
            {
              // Not required, yet.
              //char vmaj = *((char*)(e.data() + 9));  // After "PARAMSAVE"
              //char vmin = *((char*)(e.data() + 10));

              unsigned long* const ulp = (unsigned long*)(e.data() + 11);  // After "PARAMSAVE" + version major and minor.
              // TODO: TODO: Set plugin bank and program.
              _curBank = ulp[0];
              _curProgram = ulp[1];

              dlen -= (2 + 2 * sizeof(unsigned long)); // After the version major and minor, bank and program.

              if(dlen > 0)
              {
                if((dlen % sizeof(float)) != 0)
                  fprintf(stderr, "VstNativeSynthIF::processEvent Error: PARAMSAVE float data length not integral multiple of float size!\n");
                else
                {
                  const unsigned long n = dlen / sizeof(float);
                  if(n != synth->_controlInPorts)
                    fprintf(stderr, "VstNativeSynthIF::processEvent Warning: PARAMSAVE number of floats:%lu != number of controls:%lu\n", n, synth->_controlInPorts);

                  // Point to location after "PARAMSAVE", version major and minor, bank and progam.
                  float* const fp = (float*)(e.data() + 9 + 2 + 2 * sizeof(unsigned long));

                  for(unsigned long i = 0; i < synth->_controlInPorts && i < n; ++i)
                  {
                    const float v = fp[i];
                    controls[i].val = v;
                  }
                }
              }
            }
          }
          // Event not filled.
          return false;
        }
        */
        //else
        {
          // FIXME TODO: Sysex support.
          return false;

          //int len = e.len();
          //char ca[len + 2];
          //ca[0] = 0xF0;      // FIXME Stack overflow with big dumps. Use std::vector<char> or something else.
          //memcpy(ca + 1, (const char*)e.data(), len);
          //ca[len + 1] = 0xF7;
          //len += 2;

          // NOTE: There is a limit on the size of a sysex. Got this:
          // "VstNativeSynthIF::processEvent midi event is MusECore::ME_SYSEX"
          // "WARNING: MIDI event of type ? decoded to 367 bytes, discarding"
          // That might be ALSA doing that.
//           snd_seq_ev_clear(event);
//           event->queue = SND_SEQ_QUEUE_DIRECT;
//           snd_seq_ev_set_sysex(event, len,
//             (unsigned char*)ca);
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
//---------------------------------------------------------

iMPEvent VstNativeSynthIF::getData(MidiPort* /*mp*/, MPEventList* el, iMPEvent start_event, unsigned pos, int ports, unsigned nframes, float** buffer)
{
  // We may not be using nevents all at once - this will be just the maximum.
  unsigned long nevents = el->size() + synti->eventFifo.getSize();
  VstMidiEvent events[nevents];
  char evbuf[sizeof(VstMidiEvent*) * nevents + sizeof(VstEvents)];
  VstEvents *vst_events = (VstEvents*)evbuf;
  vst_events->numEvents = 0;
  vst_events->reserved  = 0;

  int frameOffset = MusEGlobal::audio->getFrameOffset();
  unsigned long syncFrame = MusEGlobal::audio->curSyncFrame();

  #ifdef VST_NATIVE_DEBUG_PROCESS
  fprintf(stderr, "VstNativeSynthIF::getData: pos:%u ports:%d nframes:%u syncFrame:%lu nevents:%lu\n", pos, ports, nframes, syncFrame, nevents);
  #endif

  unsigned long nop, k;
  nop = ((unsigned long) ports) > _synth->outPorts() ? _synth->outPorts() : ((unsigned long) ports);
  unsigned long sample = 0;

  // I read that some plugins do not like changing sample run length (compressors etc). Some are OK with it.
  // TODO: In order to support this effectively, must be user selectable, per-plugin. ENABLED for now.
  const bool usefixedrate = false;  
  unsigned long fixedsize = nframes; 

  // For now, the fixed size is clamped to the audio buffer size.
  // TODO: We could later add slower processing over several cycles -
  //  so that users can select a small audio period but a larger control period.
  if(fixedsize > nframes)
    fixedsize = nframes;

  unsigned long min_per = MusEGlobal::config.minControlProcessPeriod;  // Must be power of 2 !
  if(min_per > nframes)
    min_per = nframes;

  // Inform the host callback we are in the audio thread.
  _inProcess = true;

  #ifdef VST_NATIVE_DEBUG_PROCESS
  fprintf(stderr, "VstNativeSynthIF::getData: Handling inputs...\n");
  #endif

  // Handle inputs...
  if(!((MusECore::AudioTrack*)synti)->noInRoute())
  {
    RouteList* irl = ((MusECore::AudioTrack*)synti)->inRoutes();
    iRoute i = irl->begin();
    if(!i->track->isMidiTrack())
    {
      int ch     = i->channel       == -1 ? 0 : i->channel;
      int remch  = i->remoteChannel == -1 ? 0 : i->remoteChannel;
      int chs    = i->channels      == -1 ? 0 : i->channels;

      if((unsigned)ch < _synth->inPorts() && (unsigned)(ch + chs) <= _synth->inPorts())
      {
        int h = remch + chs;
        for(int j = remch; j < h; ++j)
          _iUsedIdx[j] = true;

        ((MusECore::AudioTrack*)i->track)->copyData(pos, chs, ch, -1, nframes, &_audioInBuffers[remch]);
      }
    }

    ++i;
    for(; i != irl->end(); ++i)
    {
      if(i->track->isMidiTrack())
        continue;

      int ch     = i->channel       == -1 ? 0 : i->channel;
      int remch  = i->remoteChannel == -1 ? 0 : i->remoteChannel;
      int chs    = i->channels      == -1 ? 0 : i->channels;

      if((unsigned)ch < _synth->inPorts() && (unsigned)(ch + chs) <= _synth->inPorts())
      {
        bool u1 = _iUsedIdx[remch];
        if(chs >= 2)
        {
          bool u2 = _iUsedIdx[remch + 1];
          if(u1 && u2)
            ((MusECore::AudioTrack*)i->track)->addData(pos, chs, ch, -1, nframes, &_audioInBuffers[remch]);
          else
          if(!u1 && !u2)
            ((MusECore::AudioTrack*)i->track)->copyData(pos, chs, ch, -1, nframes, &_audioInBuffers[remch]);
          else
          {
            if(u1)
              ((MusECore::AudioTrack*)i->track)->addData(pos, 1, ch, 1, nframes, &_audioInBuffers[remch]);
            else
              ((MusECore::AudioTrack*)i->track)->copyData(pos, 1, ch, 1, nframes, &_audioInBuffers[remch]);

            if(u2)
              ((MusECore::AudioTrack*)i->track)->addData(pos, 1, ch + 1, 1, nframes, &_audioInBuffers[remch + 1]);
            else
              ((MusECore::AudioTrack*)i->track)->copyData(pos, 1, ch + 1, 1, nframes, &_audioInBuffers[remch + 1]);
          }
        }
        else
        {
            if(u1)
              ((MusECore::AudioTrack*)i->track)->addData(pos, 1, ch, -1, nframes, &_audioInBuffers[remch]);
            else
              ((MusECore::AudioTrack*)i->track)->copyData(pos, 1, ch, -1, nframes, &_audioInBuffers[remch]);
        }

        int h = remch + chs;
        for(int j = remch; j < h; ++j)
          _iUsedIdx[j] = true;
      }
    }
  }

  #ifdef VST_NATIVE_DEBUG_PROCESS
  fprintf(stderr, "VstNativeSynthIF::getData: Processing automation control values...\n");
  #endif

  while(sample < nframes)
  {
    unsigned long nsamp = usefixedrate ? fixedsize : nframes - sample;

    //
    // Process automation control values, while also determining the maximum acceptable
    //  size of this run. Further processing, from FIFOs for example, can lower the size
    //  from there, but this section determines where the next highest maximum frame
    //  absolutely needs to be for smooth playback of the controller value stream...
    //
    if(id() != -1)
    {
      unsigned long frame = pos + sample;
      AutomationType at = AUTO_OFF;
      at = synti->automationType();
      bool no_auto = !MusEGlobal::automation || at == AUTO_OFF;
      AudioTrack* track = (static_cast<AudioTrack*>(synti));
      int nextFrame;
      const unsigned long in_ctrls = _synth->inControls();
      for(unsigned long k = 0; k < in_ctrls; ++k)
      {
        const float v = track->controller()->value(genACnum(id(), k), frame,
                                   no_auto || !_controls[k].enCtrl || !_controls[k].en2Ctrl,
                                   &nextFrame);
        if(_controls[k].val != v)
        {
          _controls[k].val = v;
#ifndef VST_VESTIGE_SUPPORT
          if(dispatch(effCanBeAutomated, k, 0, NULL, 0.0f) == 1)
          {
#endif
            if(_plugin->getParameter(_plugin, k) != v)
              _plugin->setParameter(_plugin, k, v);
#ifndef VST_VESTIGE_SUPPORT
          }
  #ifdef VST_NATIVE_DEBUG
          else
            fprintf(stderr, "VstNativeSynthIF::getData %s parameter:%lu cannot be automated\n", name().toLatin1().constData(), k);
  #endif
#endif
        }
            
#ifdef VST_NATIVE_DEBUG_PROCESS
        fprintf(stderr, "VstNativeSynthIF::getData k:%lu sample:%lu frame:%lu nextFrame:%d nsamp:%lu \n", k, sample, frame, nextFrame, nsamp);
#endif
        if(MusEGlobal::audio->isPlaying() && !usefixedrate && nextFrame != -1)
        {
          // Returned value of nextFrame can be zero meaning caller replaces with some (constant) value.
          unsigned long samps = (unsigned long)nextFrame;
          if(samps > frame + min_per)
          {
            unsigned long diff = samps - frame;
            unsigned long mask = min_per-1;   // min_per must be power of 2
            samps = diff & ~mask;
            if((diff & mask) != 0)
              samps += min_per;
          }
          else
            samps = min_per;

          if(samps < nsamp)
            nsamp = samps;
        }
      }
#ifdef VST_NATIVE_DEBUG_PROCESS
      fprintf(stderr, "VstNativeSynthIF::getData sample:%lu nsamp:%lu\n", sample, nsamp);
#endif
    }

    bool found = false;
    unsigned long frame = 0;
    unsigned long index = 0;
    unsigned long evframe;
    // Get all control ring buffer items valid for this time period...
    while(!_controlFifo.isEmpty())
    {
      ControlEvent v = _controlFifo.peek();
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
        fprintf(stderr, "VstNativeSynthIF::getData *** Error: evframe:%lu < frame:%lu event: frame:%lu idx:%lu val:%f unique:%d\n",
          evframe, frame, v.frame, v.idx, v.value, v.unique);

        // No choice but to ignore it.
        _controlFifo.remove();               // Done with the ring buffer's item. Remove it.
        continue;
      }

      if(evframe >= nframes                                                        // Next events are for a later period.
         || (!usefixedrate && !found && !v.unique && (evframe - sample >= nsamp))  // Next events are for a later run in this period. (Autom took prio.)
         || (found && !v.unique && (evframe - sample >= min_per))                  // Eat up events within minimum slice - they're too close.
         || (usefixedrate && found && v.unique && v.idx == index))                 // Special for dssi-vst: Fixed rate and must reply to all.
        break;
      _controlFifo.remove();               // Done with the ring buffer's item. Remove it.

      if(v.idx >= _synth->inControls()) // Sanity check.
        break;
      found = true;
      frame = evframe;
      index = v.idx;

      //if(_controls[v.idx].val != v.value)  // Mm nah, force it always. REMOVE Tim.
      //{
        _controls[v.idx].val = v.value;
#ifndef VST_VESTIGE_SUPPORT
        if(dispatch(effCanBeAutomated, v.idx, 0, NULL, 0.0f) == 1)
        {
#endif        
          if(v.value != _plugin->getParameter(_plugin, v.idx))
            _plugin->setParameter(_plugin, v.idx, v.value);
#ifndef VST_VESTIGE_SUPPORT
        }
  #ifdef VST_NATIVE_DEBUG
        else
          fprintf(stderr, "VstNativeSynthIF::getData %s parameter:%lu cannot be automated\n", name().toLatin1().constData(), v.idx);
  #endif
#endif
        // Need to update the automation value, otherwise it overwrites later with the last automation value.
        if(id() != -1)
          synti->setPluginCtrlVal(genACnum(id(), v.idx), v.value);
      //}
    }

    if(found && !usefixedrate)  // If a control FIFO item was found, takes priority over automation controller stream.
      nsamp = frame - sample;

    if(sample + nsamp >= nframes)         // Safety check.
      nsamp = nframes - sample;

    // TODO: Don't allow zero-length runs. This could/should be checked in the control loop instead.
    // Note this means it is still possible to get stuck in the top loop (at least for a while).
    if(nsamp == 0)
      continue;

    nevents = 0;
    // Process event list events...
    for(; start_event != el->end(); ++start_event)
    {
      #ifdef VST_NATIVE_DEBUG
      fprintf(stderr, "VstNativeSynthIF::getData eventlist event time:%d pos:%u sample:%lu nsamp:%lu frameOffset:%d\n", start_event->time(), pos, sample, nsamp, frameOffset);
      #endif

      if(start_event->time() >= (pos + sample + nsamp + frameOffset))  // frameOffset? Test again...
      {
        #ifdef VST_NATIVE_DEBUG
        fprintf(stderr, " event is for future:%lu, breaking loop now\n", start_event->time() - frameOffset - pos - sample);
        #endif
        break;
      }

      // Update hardware state so knobs and boxes are updated. Optimize to avoid re-setting existing values.
      // Same code as in MidiPort::sendEvent()
      if(synti->midiPort() != -1)
      {
        MusECore::MidiPort* mp = &MusEGlobal::midiPorts[synti->midiPort()];
        if(start_event->type() == MusECore::ME_CONTROLLER)
        {
          int da = start_event->dataA();
          int db = start_event->dataB();
          db = mp->limitValToInstrCtlRange(da, db);
          if(!mp->setHwCtrlState(start_event->channel(), da, db))
            continue;
        }
        else if(start_event->type() == MusECore::ME_PITCHBEND)
        {
          int da = mp->limitValToInstrCtlRange(MusECore::CTRL_PITCH, start_event->dataA());
          if(!mp->setHwCtrlState(start_event->channel(), MusECore::CTRL_PITCH, da))
            continue;
        }
        else if(start_event->type() == MusECore::ME_AFTERTOUCH)
        {
          int da = mp->limitValToInstrCtlRange(MusECore::CTRL_AFTERTOUCH, start_event->dataA());
          if(!mp->setHwCtrlState(start_event->channel(), MusECore::CTRL_AFTERTOUCH, da))
            continue;
        }
        else if(start_event->type() == MusECore::ME_POLYAFTER)
        {
          int ctl = (MusECore::CTRL_POLYAFTER & ~0xff) | (start_event->dataA() & 0x7f);
          int db = mp->limitValToInstrCtlRange(ctl, start_event->dataB());
          if(!mp->setHwCtrlState(start_event->channel(), ctl , db))
            continue;
        }
        else if(start_event->type() == MusECore::ME_PROGRAM)
        {
          if(!mp->setHwCtrlState(start_event->channel(), MusECore::CTRL_PROGRAM, start_event->dataA()))
            continue;
        }
      }

      // Returns false if the event was not filled. It was handled, but some other way.
      if(processEvent(*start_event, &events[nevents]))
      {
        // Time-stamp the event.
        int ft = start_event->time() - frameOffset - pos - sample;
        if(ft < 0)
          ft = 0;

        if (ft >= int(nsamp))
        {
            fprintf(stderr, "VstNativeSynthIF::getData: eventlist event time:%d out of range. pos:%d offset:%d ft:%d sample:%lu nsamp:%lu\n", start_event->time(), pos, frameOffset, ft, sample, nsamp);
            ft = nsamp - 1;
        }

        #ifdef VST_NATIVE_DEBUG
        fprintf(stderr, "VstNativeSynthIF::getData eventlist: ft:%d current nevents:%lu\n", ft, nevents);
        #endif

        vst_events->events[nevents] = (VstEvent*)&events[nevents];
        events[nevents].deltaFrames = ft;
        ++nevents;
      }
    }

    // Now process putEvent events...
    while(!synti->eventFifo.isEmpty())
    {
      MusECore::MidiPlayEvent e = synti->eventFifo.peek();

      #ifdef VST_NATIVE_DEBUG
      fprintf(stderr, "VstNativeSynthIF::getData eventFifo event time:%d\n", e.time());
      #endif

      if(e.time() >= (pos + sample + nsamp + frameOffset))
        break;

      synti->eventFifo.remove();    // Done with ring buffer's event. Remove it.
      // Returns false if the event was not filled. It was handled, but some other way.
      if(processEvent(e, &events[nevents]))
      {
        // Time-stamp the event.
        int ft = e.time() - frameOffset - pos  - sample;
        if(ft < 0)
          ft = 0;
        if (ft >= int(nsamp))
        {
            fprintf(stderr, "VstNativeSynthIF::getData: eventFifo event time:%d out of range. pos:%d offset:%d ft:%d sample:%lu nsamp:%lu\n", e.time(), pos, frameOffset, ft, sample, nsamp);
            ft = nsamp - 1;
        }
        vst_events->events[nevents] = (VstEvent*)&events[nevents];
        events[nevents].deltaFrames = ft;

        ++nevents;
      }
    }

    #ifdef VST_NATIVE_DEBUG_PROCESS
    fprintf(stderr, "VstNativeSynthIF::getData: Connecting and running. sample:%lu nsamp:%lu nevents:%lu\n", sample, nsamp, nevents);
    #endif

    // Set the events pointer.
    if(nevents > 0)
    {
      vst_events->numEvents = nevents;
      dispatch(effProcessEvents, 0, 0, vst_events, 0.0f);
    }

    float* in_bufs[_synth->inPorts()];
    float* out_bufs[_synth->outPorts()];
    
    k = 0;
    // Connect the given buffers directly to the ports, up to a max of synth ports.
    for(; k < nop; ++k)
      out_bufs[k] = buffer[k] + sample;
    // Connect the remaining ports to some local buffers (not used yet).
    const unsigned long op =_synth->outPorts();  
    for(; k < op; ++k)
      out_bufs[k] = _audioOutBuffers[k] + sample;
    // Connect all inputs either to some local buffers, or a silence buffer.
    const unsigned long ip =_synth->inPorts();
    for(k = 0; k < ip; ++k)
    {
      if(_iUsedIdx[k])
      {
        _iUsedIdx[k] = false; // Reset
        in_bufs[k] = _audioInBuffers[k] + sample;
      }
      else
        in_bufs[k] = _audioInSilenceBuf + sample;
    }

    // Run the synth for a period of time. This processes events and gets/fills our local buffers...
    if((_plugin->flags & effFlagsCanReplacing) && _plugin->processReplacing)
    {
      _plugin->processReplacing(_plugin, in_bufs, out_bufs, nsamp);
    }
#if 0 //ifdef VST_FORCE_DEPRECATED
    else
    {
      // TODO: This is not right, we don't accumulate data here. Depricated now, and frowned upon anyway...
      if(_plugin->process)
        _plugin->process(_plugin, in_bufs, out_bufs, nsamp);
    }
#endif
    
    sample += nsamp;
  }

  // Inform the host callback we will be no longer in the audio thread.
  _inProcess = true;
  
  return start_event;
}

//---------------------------------------------------------
//   putEvent
//---------------------------------------------------------

bool VstNativeSynthIF::putEvent(const MidiPlayEvent& ev)
      {
      #ifdef VST_NATIVE_DEBUG
      fprintf(stderr, "VstNativeSynthIF::putEvent midi event time:%d chn:%d a:%d b:%d\n", ev.time(), ev.channel(), ev.dataA(), ev.dataB());
      #endif
      
      if (MusEGlobal::midiOutputTrace)
            ev.dump();
      return synti->eventFifo.put(ev);
      }


//--------------------------------
// Methods for PluginIBase:
//--------------------------------

unsigned long VstNativeSynthIF::pluginID()                        { return (_plugin) ? _plugin->uniqueID : 0; }
int VstNativeSynthIF::id()                                        { return MAX_PLUGINS; } // Set for special block reserved for synth. 
QString VstNativeSynthIF::pluginLabel() const                     { return _synth ? QString(_synth->name()) : QString(); } // FIXME Maybe wrong
QString VstNativeSynthIF::lib() const                             { return _synth ? _synth->completeBaseName() : QString(); }
QString VstNativeSynthIF::dirPath() const                         { return _synth ? _synth->absolutePath() : QString(); }
QString VstNativeSynthIF::fileName() const                        { return _synth ? _synth->fileName() : QString(); }
void VstNativeSynthIF::enableController(unsigned long i, bool v)  { _controls[i].enCtrl = v; }
bool VstNativeSynthIF::controllerEnabled(unsigned long i) const   { return _controls[i].enCtrl;}
void VstNativeSynthIF::enable2Controller(unsigned long i, bool v) { _controls[i].en2Ctrl = v; }
bool VstNativeSynthIF::controllerEnabled2(unsigned long i) const  { return _controls[i].en2Ctrl; }
void VstNativeSynthIF::enableAllControllers(bool v)
{
  if(!_synth)
    return;
  const unsigned long sic = _synth->inControls();
  for(unsigned long i = 0; i < sic; ++i)
    _controls[i].enCtrl = v;
}
void VstNativeSynthIF::enable2AllControllers(bool v)
{
  if(!_synth)
    return;
  const unsigned long sic = _synth->inControls();
  for(unsigned long i = 0; i < sic; ++i)
    _controls[i].en2Ctrl = v;
}
void VstNativeSynthIF::updateControllers() { }
void VstNativeSynthIF::activate()
{
  //for (unsigned short i = 0; i < instances(); ++i) {
  //        dispatch(i, effMainsChanged, 0, 1, NULL, 0.0f);
  dispatch(effMainsChanged, 0, 1, NULL, 0.0f);
#ifndef VST_VESTIGE_SUPPORT
  //dispatch(i, effStartProcess, 0, 0, NULL, 0.0f);
  dispatch(effStartProcess, 0, 0, NULL, 0.0f);
#endif
  //}

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
}
void VstNativeSynthIF::deactivate()
{
  //for (unsigned short i = 0; i < instances(); ++i) {
#ifndef VST_VESTIGE_SUPPORT
  //dispatch(i, effStopProcess, 0, 0, NULL, 0.0f);
  dispatch(effStopProcess, 0, 0, NULL, 0.0f);
#endif
  //dispatch(i, effMainsChanged, 0, 0, NULL, 0.0f);
  dispatch(effMainsChanged, 0, 0, NULL, 0.0f);
  //}
}

unsigned long VstNativeSynthIF::parameters() const                { return _synth ? _synth->inControls() : 0; }
unsigned long VstNativeSynthIF::parametersOut() const             { return 0; }
void VstNativeSynthIF::setParam(unsigned long i, float val)       { setParameter(i, val); }
float VstNativeSynthIF::param(unsigned long i) const              { return getParameter(i); }
float VstNativeSynthIF::paramOut(unsigned long) const           { return 0.0; }
const char* VstNativeSynthIF::paramName(unsigned long i)          
{
  if(!_plugin)
    return 0;
  static char buf[256];
  buf[0] = 0;
  dispatch(effGetParamName, i, 0, buf, 0);
  return buf;
}

const char* VstNativeSynthIF::paramOutName(unsigned long)       { return 0; }
LADSPA_PortRangeHint VstNativeSynthIF::range(unsigned long /*i*/)     
{
  LADSPA_PortRangeHint h;
  // FIXME TODO:
  h.HintDescriptor = 0;
  h.LowerBound = 0.0;
  h.UpperBound = 1.0;
  return h;
}
LADSPA_PortRangeHint VstNativeSynthIF::rangeOut(unsigned long)  
{
  // There are no output controls.
  LADSPA_PortRangeHint h;
  h.HintDescriptor = 0;
  h.LowerBound = 0.0;
  h.UpperBound = 1.0;
  return h;
}
// FIXME TODO:
CtrlValueType VstNativeSynthIF::ctrlValueType(unsigned long /*i*/) const { return VAL_LINEAR; }
CtrlList::Mode VstNativeSynthIF::ctrlMode(unsigned long /*i*/) const     { return CtrlList::INTERPOLATE; };

} // namespace MusECore

#else  // VST_NATIVE_SUPPORT
namespace MusECore {
void initVST_Native() {}
} // namespace MusECore
#endif

