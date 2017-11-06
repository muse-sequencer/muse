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
#include <dlfcn.h>
#include <cmath>
#include <set>
#include <string>
#include <jack/jack.h>
#include <sstream>

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
#include "minstrument.h"

#include "vst_native.h"

#define OLD_PLUGIN_ENTRY_POINT "main"
#define NEW_PLUGIN_ENTRY_POINT "VSTPluginMain"

// Enable debugging messages
//#define VST_NATIVE_DEBUG
//#define VST_NATIVE_DEBUG_PROCESS

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

static bool scanSubPlugin(QFileInfo& fi, AEffect *plugin, int id, void *handle)
{
   char buffer[128];
   QString effectName;
   QString vendorString;
   QString productString;
   int vendorVersion;
   QString vendorVersionString;
   std::vector<Synth*>::iterator is;
   int vst_version = 0;
   VstNativeSynth* new_synth = NULL;

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
     {
        fprintf(stderr, "VST %s already exists!\n", (char *)effectName.toUtf8().constData());
       return false;
     }

   // "2 = VST2.x, older versions return 0". Observed 2400 on all the ones tested so far.
   vst_version = plugin->dispatcher(plugin, effGetVstVersion, 0, 0, NULL, 0.0f);
   bool isSynth = true;
   if(!((plugin->flags & effFlagsIsSynth) || (vst_version >= 2 && plugin->dispatcher(plugin, effCanDo, 0, 0,(void*) "receiveVstEvents", 0.0f) > 0)))
   {
     isSynth = false;
   }

   vendorVersionString = QString("%1.%2.%3").arg((vendorVersion >> 16) & 0xff).arg((vendorVersion >> 8) & 0xff).arg(vendorVersion & 0xff);
   
   Plugin::PluginFeatures reqfeat = Plugin::NoFeatures;
   
   new_synth = new VstNativeSynth(fi, plugin, 
                                  effectName, productString, vendorString, vendorVersionString, 
                                  id, handle, isSynth, reqfeat);

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

   if(new_synth->inPorts() > 0 && new_synth->outPorts() > 0)
   {
      MusEGlobal::plugins.push_back(new VstNativePluginWrapper(new_synth, reqfeat));
   }

   return true;

}

static void scanVstNativeLib(QFileInfo& fi)
{
  sem_wait(&_vstIdLock);
  currentPluginId = 0;
  bool bDontDlCLose = false;
  AEffect *plugin = NULL;
  void* handle = dlopen(fi.filePath().toLatin1().constData(), RTLD_NOW);
  if (handle == NULL)
  {
    fprintf(stderr, "scanVstNativeLib: dlopen(%s) failed: %s\n", fi.filePath().toLatin1().constData(), dlerror());
    goto _end;
  }
  
  AEffect *(*getInstance)(audioMasterCallback);
  getInstance = (AEffect*(*)(audioMasterCallback))dlsym(handle, NEW_PLUGIN_ENTRY_POINT);
  if(!getInstance)
  {
    if(MusEGlobal::debugMsg)
    {
      fprintf(stderr, "VST 2.4 entrypoint \"" NEW_PLUGIN_ENTRY_POINT "\" not found in library %s, looking for \""
                      OLD_PLUGIN_ENTRY_POINT "\"\n", fi.filePath().toLatin1().constData());
    }

    getInstance = (AEffect*(*)(audioMasterCallback))dlsym(handle, OLD_PLUGIN_ENTRY_POINT);
    if(!getInstance)
    {
      fprintf(stderr, "ERROR: VST entrypoints \"" NEW_PLUGIN_ENTRY_POINT "\" or \""
                      OLD_PLUGIN_ENTRY_POINT "\" not found in library\n");
      goto _end;
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

  plugin = getInstance(vstNativeHostCallback);
  if(!plugin)
  {
    fprintf(stderr, "ERROR: Failed to instantiate plugin in VST library \"%s\"\n", fi.filePath().toLatin1().constData());
    goto _end;
  }
  else if(MusEGlobal::debugMsg)
    fprintf(stderr, "plugin instantiated\n");

  if(plugin->magic != kEffectMagic)
  {
    fprintf(stderr, "Not a VST plugin in library \"%s\"\n", fi.filePath().toLatin1().constData());
    goto _end;
  }
  else if(MusEGlobal::debugMsg)
    fprintf(stderr, "plugin is a VST\n");

  if(plugin->dispatcher(plugin, 24 + 11 /* effGetCategory */, 0, 0, 0, 0) == 10 /* kPlugCategShell */)
  {
     bDontDlCLose = true;
     std::map<VstIntPtr, std::string> shellPlugs;
     char cPlugName [128];
     do
     {
        memset(cPlugName, 0, sizeof(cPlugName));
        VstIntPtr id = plugin->dispatcher(plugin, 24 + 46 /* effShellGetNextPlugin */, 0, 0, cPlugName, 0);
        if(id != 0 && cPlugName [0] != 0)
        {
           shellPlugs.insert(std::make_pair<int, std::string>(id, std::string(cPlugName)));
        }
        else
           break;
     }
     while(true);

     for(std::map<VstIntPtr, std::string>::iterator it = shellPlugs.begin(); it != shellPlugs.end(); ++it)
     {
        if(plugin)
        {
            plugin->dispatcher(plugin, effClose, 0, 0, NULL, 0);
            plugin = NULL;
        }

        currentPluginId = it->first;
        getInstance = (AEffect*(*)(audioMasterCallback))dlsym(handle, NEW_PLUGIN_ENTRY_POINT);
        if(!getInstance)
          goto _end;

        AEffect *plugin = getInstance(vstNativeHostCallback);
        if(!plugin)
        {
          fprintf(stderr, "ERROR: Failed to instantiate plugin in VST library \"%s\", shell id=%ld\n", fi.filePath().toLatin1().constData(), (long)currentPluginId);
          goto _end;
        }
        scanSubPlugin(fi, plugin, currentPluginId, handle);
        currentPluginId = 0;
     }
  }
  else
  {
     scanSubPlugin(fi, plugin, 0, 0);
  }


  //plugin->dispatcher(plugin, effMainsChanged, 0, 0, NULL, 0);
  if(plugin)
      plugin->dispatcher(plugin, effClose, 0, 0, NULL, 0);

  _end:
  if(handle && !bDontDlCLose)
      dlclose(handle);

  sem_post(&_vstIdLock);
}

//---------------------------------------------------------
//   scanVstDir
//---------------------------------------------------------

static void scanVstNativeDir(const QString& s, int depth)
{
   if(++depth > 2){
      return;
   }
   if (MusEGlobal::debugMsg)
      fprintf(stderr, "scan vst native plugin dir <%s>\n", s.toLatin1().constData());
   QDir pluginDir(s, QString("*.so"), QDir::Unsorted, QDir::Files | QDir::AllDirs);
   if(!pluginDir.exists())
      return;
   QStringList list = pluginDir.entryList();
   int count = list.count();
   for(int i = 0; i < count; ++i)
   {
      QFileInfo fi(s + QString("/") + list[i]);
      if(fi.isDir())
      {
         if((list [i] != ".") && (list [i] != ".."))
         {
            scanVstNativeDir(fi.absoluteFilePath(), depth);
         }
         continue;
      }
      if(MusEGlobal::debugMsg)
         fprintf(stderr, "scanVstNativeDir: found %s\n", (s + QString("/") + list[i]).toLatin1().constData());


      scanVstNativeLib(fi);
   }
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
      std::string s;
      const char* vstPath = getenv("LINUX_VST_PATH");
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
          s = std::string(home) + std::string("/.vst:") + std::string(home) + std::string("/vst:/usr/local/lib64/vst:/usr/local/lib/vst:/usr/lib64/vst:/usr/lib/vst");
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
                  scanVstNativeDir(QString(buffer), 0);
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

VstNativeSynth::VstNativeSynth(const QFileInfo& fi, AEffect* plugin, 
                               const QString& label, const QString& desc, const QString& maker, const QString& ver, 
                               VstIntPtr id, void *dlHandle, bool isSynth, Plugin::PluginFeatures reqFeatures)
  : Synth(fi, label, desc, maker, ver, reqFeatures)
{
  _handle = dlHandle;
  _id = id;
  _hasGui = plugin->flags & effFlagsHasEditor;
  _inports = plugin->numInputs;
  _outports = plugin->numOutputs;
  _controlInPorts = plugin->numParams;
//#ifndef VST_VESTIGE_SUPPORT
  _hasChunks = plugin->flags & 32 /*effFlagsProgramChunks*/;
//#else
 // _hasChunks = false;
//#endif
  
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
  _isSynth = isSynth;
}

//---------------------------------------------------------
//   incInstances
//---------------------------------------------------------

void VstNativeSynth::incInstances(int val)
{
  _instances += val;
  if(_instances == 0)
  {
    if(_handle && _id == 0)
    {
      #ifdef VST_NATIVE_DEBUG
      fprintf(stderr, "VstNativeSynth::incInstances no more instances, closing library\n");
      #endif

      dlclose(_handle);
      _handle = NULL;
    }
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

AEffect* VstNativeSynth::instantiate(void* userData)
{
  int inst_num = _instances;
  inst_num++;
  QString n;
  n.setNum(inst_num);
  QString instanceName = baseName() + "-" + n;
  QByteArray ba = info.filePath().toLatin1();
  const char* path = ba.constData();
  void* hnd = _handle;
  //int vst_version;

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


  sem_wait(&_vstIdLock);

  currentPluginId = _id;

  AEffect *plugin = getInstance(vstNativeHostCallback);

  sem_post(&_vstIdLock);
  if(!plugin)
  {
    fprintf(stderr, "ERROR: Failed to instantiate plugin in VST library \"%s\"\n", path);
    if(_id == 0)
      dlclose(hnd);
    return NULL;
  }
  else if(MusEGlobal::debugMsg)
    fprintf(stderr, "plugin instantiated\n");

  if(plugin->magic != kEffectMagic)
  {
    fprintf(stderr, "Not a VST plugin in library \"%s\"\n", path);
    if(_id == 0)
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

  plugin->user = userData;
  plugin->dispatcher(plugin, effOpen, 0, 0, NULL, 0);

  // "2 = VST2.x, older versions return 0". Observed 2400 on all the ones tested so far.
  //vst_version = plugin->dispatcher(plugin, effGetVstVersion, 0, 0, NULL, 0.0f);
  /*if(!((plugin->flags & effFlagsIsSynth) || (vst_version >= 2 && plugin->dispatcher(plugin, effCanDo, 0, 0,(void*) "receiveVstEvents", 0.0f) > 0)))
  {
    if(MusEGlobal::debugMsg)
      fprintf(stderr, "Plugin is not a synth\n");
    goto _error;
  }*/


  ++_instances;
  _handle = hnd;

  // work around to get airwave to work (author contacted so maybe another solution will
  // reveal itself)
  plugin->dispatcher(plugin, effSetSampleRate, 0, 0, NULL, MusEGlobal::sampleRate);
  plugin->dispatcher(plugin, effSetBlockSize, 0, MusEGlobal::segmentSize, NULL, 0.0f);
  plugin->dispatcher(plugin, effMainsChanged, 0, 1, NULL, 0.0f);
  //

  //plugin->dispatcher(plugin, effSetProgram, 0, 0, NULL, 0.0f); // REMOVE Tim. Or keep?
  return plugin;
/*
_error:
  //plugin->dispatcher(plugin, effMainsChanged, 0, 0, NULL, 0);
  plugin->dispatcher(plugin, effClose, 0, 0, NULL, 0);
  if(_id == 0)
    dlclose(hnd);
  return NULL;
  */
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
      _gw = NULL;
      _synth = NULL;
      _plugin = NULL;
      _active = false;
      _editor = NULL;
      _inProcess = false;
       _controls = NULL;
//       controlsOut = 0;
      _audioInBuffers = NULL;
      _audioInSilenceBuf = NULL;
      _audioOutBuffers = NULL;
      userData.pstate = 0;
      userData.sif = this;
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
}

//---------------------------------------------------------
//   init
//---------------------------------------------------------

bool VstNativeSynthIF::init(Synth* s)
      {
      _synth = (VstNativeSynth*)s;
      _plugin = _synth->instantiate(&userData);
      if(!_plugin)
        return false;

      queryPrograms();
      
      unsigned long outports = _synth->outPorts();
      if(outports != 0)
      {
        _audioOutBuffers = new float*[outports];
        for(unsigned long k = 0; k < outports; ++k)
        {
          int rv = posix_memalign((void**)&_audioOutBuffers[k], 16, sizeof(float) * MusEGlobal::segmentSize);
          if(rv != 0)
          {
            fprintf(stderr, "ERROR: VstNativeSynthIF::init: posix_memalign returned error:%d. Aborting!\n", rv);
            abort();
          }
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
          int rv = posix_memalign((void**)&_audioInBuffers[k], 16, sizeof(float) * MusEGlobal::segmentSize);
          if(rv != 0)
          {
            fprintf(stderr, "ERROR: VstNativeSynthIF::init: posix_memalign returned error:%d. Aborting!\n", rv);
            abort();
          }
          if(MusEGlobal::config.useDenormalBias)
          {
            for(unsigned q = 0; q < MusEGlobal::segmentSize; ++q)
              _audioInBuffers[k][q] = MusEGlobal::denormalBias;
          }
          else
            memset(_audioInBuffers[k], 0, sizeof(float) * MusEGlobal::segmentSize);
        }
        
        int rv = posix_memalign((void**)&_audioInSilenceBuf, 16, sizeof(float) * MusEGlobal::segmentSize);
        if(rv != 0)
        {
          fprintf(stderr, "ERROR: VstNativeSynthIF::init: posix_memalign returned error:%d. Aborting!\n", rv);
          abort();
        }
        if(MusEGlobal::config.useDenormalBias)
        {
          for(unsigned q = 0; q < MusEGlobal::segmentSize; ++q)
            _audioInSilenceBuf[q] = MusEGlobal::denormalBias;
        }
        else
          memset(_audioInSilenceBuf, 0, sizeof(float) * MusEGlobal::segmentSize);
      }

      _controls = NULL;
      _gw = NULL;
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
        //float val;  // TODO
        //ladspaDefaultValue(ld, k, &val);   // FIXME TODO
        float val = _plugin->getParameter(_plugin, i);  // TODO
        _controls[i].val    = val;
        _controls[i].tmpVal = val;
        _controls[i].enCtrl  = true;

        // Support a special block for synth ladspa controllers.
        // Put the ID at a special block after plugins (far after).
        int id = genACnum(MAX_PLUGINS, i);
        const char* param_name = paramName(i);

        // TODO FIXME!
        ///float min, max;
        ///ladspaControlRange(ld, k, &min, &max);
        float min = 0.0, max = 1.0;

        CtrlList* cl;
        CtrlListList* cll = track()->controller();
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
          
          if(dispatch(26 /*effCanBeAutomated*/, i, 0, NULL, 0.0f) == 1)
          {
            double v = cl->curVal();
            if(v != _plugin->getParameter(_plugin, i))
              _plugin->setParameter(_plugin, i, v);
          }

  #ifdef VST_NATIVE_DEBUG
          else  
            fprintf(stderr, "VstNativeSynthIF::init %s parameter:%lu cannot be automated\n", name().toLatin1().constData(), i);
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
      return true;
      }

//---------------------------------------------------------
//   resizeEditor
//---------------------------------------------------------

bool VstNativeSynth::resizeEditor(MusEGui::VstNativeEditor *editor, int w, int h)
{
  if(!editor || w <= 0 || h <= 0)
    return false;
  editor->setFixedSize(w, h);
  return true;
}

//---------------------------------------------------------
//   hostCallback
//---------------------------------------------------------

VstIntPtr VstNativeSynth::pluginHostCallback(VstNativeSynthOrPlugin *userData, VstInt32 opcode, VstInt32 index, VstIntPtr value, void* ptr, float opt)
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
      // call application idle routine (this will
      // call effEditIdle for all open editors too)
      //_plugin->updateParamValues(false);
      //_plugin->dispatcher(_plugin, effEditIdle, 0, 0, NULL, 0.0f);
      ///idleEditor();  // REMOVE Tim. Or keep.
      return 0;

   case audioMasterGetTime:
   {
      // returns const VstTimeInfo* (or 0 if not supported)
      // <value> should contain a mask indicating which fields are required
      // (see valid masks above), as some items may require extensive
      // conversions

      // FIXME TODO: Optimizations: This may be called many times in one process call
      //              due to our multi-run slices. Some of the (costly) info will be redundant.
      //             So try to add some flag to try to only call some or all of this once per cycle.

#ifdef VST_NATIVE_DEBUG
      fprintf(stderr, "VstNativeSynthIF::hostCallback master time: valid: nanos:%d ppqpos:%d tempo:%d bars:%d cyclepos:%d sig:%d smpte:%d clock:%d\n",
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

      unsigned int curr_frame = MusEGlobal::audio->pos().frame();
      _timeInfo.samplePos = (double)curr_frame;
      _timeInfo.sampleRate = (double)MusEGlobal::sampleRate;
      _timeInfo.flags = 0;

      Pos p(MusEGlobal::extSyncFlag.value() ? MusEGlobal::audio->tickPos() : curr_frame, MusEGlobal::extSyncFlag.value() ? true : false);

      if(value & kVstBarsValid)
      {
         int p_bar, p_beat, p_tick;
         p.mbt(&p_bar, &p_beat, &p_tick);
         _timeInfo.barStartPos = (double)Pos(p_bar, 0, 0).tick() / (double)MusEGlobal::config.division;
         _timeInfo.flags |= kVstBarsValid;
      }

      if(value & kVstTimeSigValid)
      {
         int z, n;
         AL::sigmap.timesig(p.tick(), z, n);

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
         _timeInfo.ppqPos = (double)MusEGlobal::audio->tickPos() / (double)MusEGlobal::config.division;
         _timeInfo.flags |= kVstPpqPosValid;
      }

      if(value & kVstTempoValid)
      {
         double tempo = MusEGlobal::tempomap.tempo(p.tick());
         _timeInfo.tempo = (60000000.0 / tempo) * double(MusEGlobal::tempomap.globalTempo())/100.0;
         _timeInfo.flags |= kVstTempoValid;
      }

#ifdef VST_NATIVE_DEBUG
      fprintf(stderr, "VstNativeSynthIF::hostCallback master time: sample pos:%f samplerate:%f sig num:%ld den:%ld tempo:%f\n",
              _timeInfo.samplePos, _timeInfo.sampleRate, _timeInfo.timeSigNumerator, _timeInfo.timeSigDenominator, _timeInfo.tempo);
#endif

      if(MusEGlobal::audio->isPlaying())
         _timeInfo.flags |= (kVstTransportPlaying | kVstTransportChanged);
      // TODO
      //if(MusEGlobal::audio->isRecording())
      //  _timeInfo.flags |= (kVstTransportRecording | kVstTransportChanged);

      return (long)&_timeInfo;
   }

   case audioMasterProcessEvents:
   {
     // VstEvents* in <ptr>
     VstEvents* ve = (VstEvents*)ptr;
     int num_ev = ve->numEvents;
#ifdef VST_NATIVE_DEBUG
     fprintf(stderr, "VstNativeSynthIF::hostCallback audioMasterProcessEvents: numEvents:%d\n", num_ev);
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
   {
      // something has changed, update 'multi-fx' display

      //_plugin->updateParamValues(false);
      //QApplication::processEvents();     // REMOVE Tim. Or keep. Commented in QTractor.
      AEffect *vstPlug = 0;
      if(userData->sif)
         vstPlug = userData->sif->_plugin;
      else if(userData->pstate)
         vstPlug = userData->pstate->plugin;

      vstPlug->dispatcher(vstPlug, effEditIdle, 0, 0, NULL, 0.0f);  // ?

      return 0;
   }

   case audioMasterBeginEdit:
      // begin of automation session (when mouse down), parameter index in <index>
      VstNativeSynth::guiAutomationBegin(userData, index);
      return 1;

   case audioMasterEndEdit:
      // end of automation session (when mouse up),     parameter index in <index>
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

  // REMOVE Tim. Or keep.
  //_plugin->dispatcher(_plugin, effEditIdle, 0, 0, NULL, 0.0f);
  //if(_editor)
  //  _editor->update();
}

//---------------------------------------------------------
//   guiHeartBeat
//---------------------------------------------------------

void VstNativeSynthIF::guiHeartBeat()
{
#ifdef VST_NATIVE_DEBUG
  fprintf(stderr, "VstNativeSynthIF::guiHeartBeat %p\n", this);
#endif

  // REMOVE Tim. Or keep.
  if(_plugin && _active)
  {
//#ifdef VST_FORCE_DEPRECATED   // REMOVE Tim. Or keep
    //_plugin->dispatcher(_plugin, effIdle, 0, 0, NULL, 0.0f);
//#endif
     if(_guiVisible)
     {
       _plugin->dispatcher(_plugin, effEditIdle, 0, 0, NULL, 0.0f);
       if(_editor)
         _editor->update();
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
          _editor->open(this, 0);
        }
      }
      else
      {
        if(_editor)
        {
          _editor->close();
          //_editor = NULL;  // No - done in editorDeleted.
        }
      }
      _guiVisible = v;
}

//---------------------------------------------------------
//   getGeometry
//---------------------------------------------------------

void VstNativeSynthIF::getGeometry(int*x, int*y, int*w, int*h) const
{
  if(!_gui)
  {
    *x=0;*y=0;*w=0;*h=0;
    return;
  }

  *x = _gui->x();
  *y = _gui->y();
  *w = _gui->width();
  *h = _gui->height();
}

//---------------------------------------------------------
//   setGeometry
//---------------------------------------------------------

void VstNativeSynthIF::setGeometry(int x, int y, int w, int h)
{
  if(!_gui)
    return;

  _gui->setGeometry(x, y, w, h);
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
      //  position, so that by the time of the NEXT process, THOSE events have also occured in the previous period.
      // So, technically this is correct. What MATTERS is how we adjust the times for storage, and/or simultaneous playback in THIS period,
      //  and TEST: we'll need to make sure any non-contiguous previous period is handled correctly by process - will it work OK as is?
      // If ALSA works OK than this should too...
#ifdef _AUDIO_USE_TRUE_FRAME_
      event.setTime(MusEGlobal::audio->previousPos().frame() + ev->deltaFrames);
#else
      event.setTime(MusEGlobal::audio->pos().frame() + ev->deltaFrames);
#endif
      event.setTick(MusEGlobal::lastExtMidiSyncTick);

//       event.setChannel(*(ev->buffer) & 0xf);
//       int type = *(ev->buffer) & 0xf0;
//       int a    = *(ev->buffer + 1) & 0x7f;
//       int b    = *(ev->buffer + 2) & 0x7f;
      event.setChannel(ev->midiData[0] & 0xf);
      int type = ev->midiData[0] & 0xf0;
      int a    = ev->midiData[1] & 0x7f;
      int b    = ev->midiData[2] & 0x7f;
      event.setType(type);

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
// TODO: Sysex NOT suppported with Vestige !
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
  //if((int)vp < _plugin->numPrograms)
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

double VstNativeSynthIF::getParameter(unsigned long idx) const
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

void VstNativeSynthIF::setParameter(unsigned long idx, double value)
      {
      addScheduledControlEvent(idx, value, MusEGlobal::audio->curFrame());
      }

//---------------------------------------------------------
//   guiAutomationBegin
//---------------------------------------------------------

void VstNativeSynth::guiAutomationBegin(VstNativeSynthOrPlugin *userData, unsigned long param_idx)
{
   //_gw[param_idx].pressed = true; //not used
   AudioTrack* t = userData->sif ? userData->sif->track() : userData->pstate->pluginI->track();
   int plug_id = userData->sif ? userData->sif->id() : userData->pstate->pluginI->id();
   if(t && plug_id != -1)
   {
      plug_id = genACnum(plug_id, param_idx);

      //if(params[param].type == GuiParam::GUI_SLIDER)
      //{
      //double val = ((Slider*)params[param].actuator)->value();
      float val = userData->sif ? userData->sif->param(param_idx) : userData->pstate->pluginI->param(param_idx);
      // FIXME TODO:
      //if (LADSPA_IS_HINT_LOGARITHMIC(params[param].hint))
      //      val = pow(10.0, val/20.0);
      //else if (LADSPA_IS_HINT_INTEGER(params[param].hint))
      //      val = rint(val);
      //plugin->setParam(param, val);
      //((DoubleLabel*)params[param].label)->setValue(val);

      //if(t)
      //{
      t->startAutoRecord(plug_id, val);
      t->setPluginCtrlVal(plug_id, val);
      //}
      //}
      //   else if(params[param].type == GuiParam::GUI_SWITCH)
      //   {
      //     float val = (float)((CheckBox*)params[param].actuator)->isChecked();
      //     plugin->setParam(param, val);
      //
      //     //if(t)
      //     //{
      //       t->startAutoRecord(plug_id, val);
      //       t->setPluginCtrlVal(plug_id, val);
      //     //}
      //   }
   }
   if(userData->sif)
   {
      userData->sif->enableController(param_idx, false);
   }
   else
   {
      userData->pstate->pluginI->enableController(param_idx, false);
   }
}

//---------------------------------------------------------
//   guiAutomationEnd
//---------------------------------------------------------

void VstNativeSynth::guiAutomationEnd(VstNativeSynthOrPlugin *userData, unsigned long param_idx)
{
   AutomationType at = AUTO_OFF;
   AudioTrack* t = userData->sif ? userData->sif->track() : userData->pstate->pluginI->track();
   int plug_id = userData->sif ? userData->sif->id() : userData->pstate->pluginI->id();
   if(t)
      at = t->automationType();

   if(t && plug_id != -1)
   {
      plug_id = genACnum(plug_id, param_idx);

      //if(params[param].type == GuiParam::GUI_SLIDER)
      //{
      //double val = ((Slider*)params[param].actuator)->value();
      float val = userData->sif ? userData->sif->param(param_idx) : userData->pstate->pluginI->param(param_idx);
      // FIXME TODO:
      //if (LADSPA_IS_HINT_LOGARITHMIC(params[param].hint))
      //      val = pow(10.0, val/20.0);
      //else if (LADSPA_IS_HINT_INTEGER(params[param].hint))
      //      val = rint(val);
      t->stopAutoRecord(plug_id, val);
      //}
   }

   // Special for switch - don't enable controller until transport stopped.
   if ((at == AUTO_OFF) ||
       (at == AUTO_TOUCH)) // && (params[param].type != GuiParam::GUI_SWITCH ||  // FIXME TODO
      //   !MusEGlobal::audio->isPlaying()) ) )
   {
      if(userData->sif)
      {
         userData->sif->enableController(param_idx, true);
      }
      else
      {
         userData->pstate->pluginI->enableController(param_idx, true);
      }
   }

   //_gw[param_idx].pressed = false; //not used
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
         userData->pstate->pluginI->track()->recordAutomation(pid, value);
      }
   }

   // Schedules a timed control change:
   ControlEvent ce;
   ce.unique = false; // Not used for native vst.
   ce.fromGui = true; // It came from the plugin's own GUI.
   ce.idx = param_idx;
   ce.value = value;
   // Don't use timestamp(), because it's circular, which is making it impossible to deal
   // with 'modulo' events which slip in 'under the wire' before processing the ring buffers.
   ce.frame = MusEGlobal::audio->curFrame();

   ControlFifo &cfifo = userData->sif ? userData->sif->_controlFifo : userData->pstate->pluginI->_controlFifo;

   if(cfifo.put(ce))
      fprintf(stderr, "VstNativeSynthIF::guiControlChanged: fifo overflow: in control number:%lu\n", param_idx);

   if(userData->sif)
   {
      userData->sif->enableController(param_idx, false);
   }
   else
   {
      userData->pstate->pluginI->enableController(param_idx, false);
   }

   return 0;
}

//---------------------------------------------------------
//   write
//---------------------------------------------------------

void VstNativeSynthIF::write(int level, Xml& xml) const
{
//#ifndef VST_VESTIGE_SUPPORT
  if(_synth->hasChunks())
  {
    //---------------------------------------------
    // dump current state of synth
    //---------------------------------------------
    fprintf(stderr, "%s: commencing chunk data dump, plugin api version=%d\n", name().toLatin1().constData(), _synth->vstVersion());
    unsigned long len = 0;
    void* p = 0;
    len = dispatch(23 /* effGetChunk */, 0, 0, &p, 0.0); // index 0: is bank 1: is program
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
//#else
//  fprintf(stderr, "support for vst chunks not compiled in!\n");
//#endif

  //---------------------------------------------
  // dump current state of synth
  //---------------------------------------------

  int params = _plugin->numParams;
  for (int i = 0; i < params; ++i)
    xml.doubleTag(level, "param", _plugin->getParameter(_plugin, i));
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
      synti->currentProg(chn, NULL, &lb, &hb);
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
        synti->currentProg(chn, &pr, &lb, NULL);
        synti->setCurrentProg(chn, pr, lb, b & 0xff);
        doSelectProgram(b, lb, pr);
        // Event pointer not filled. Return false.
        return false;
      }
      
      if(a == CTRL_LBANK)
      {
        int hb, pr;
        synti->currentProg(chn, &pr, NULL, &hb);
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
//#ifndef VST_VESTIGE_SUPPORT
                    int chunk_flags = data[9];
                    if(chunk_flags & VST_NATIVE_CHUNK_FLAG_COMPRESSED)
                      fprintf(stderr, "chunk flags:%x compressed chunks not supported yet.\n", chunk_flags);
                    else
                    {
                      fprintf(stderr, "%s: loading chunk from sysex!\n", name().toLatin1().constData());
                      // 10 = 2 bytes header + "VSTSAVE" + 1 byte flags (compression etc)
                      dispatch(24 /* effSetChunk */, 0, e.len()-10, (void*)(data+10), 0.0); // index 0: is bank 1: is program
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

        // DELETETHIS, 50 clean it up or fix it?
        /*
        // p3.3.39 Read the state of current bank and program and all input control values.
        // TODO: Needs to be better. See write().
        //else
        if (QString((const char*)e.data()).startsWith("PARAMSAVE"))
        {
          #ifdef VST_NATIVE_DEBUG
          fprintf(stderr, "VstNativeSynthIF::processEvent midi event is ME_SYSEX PARAMSAVE\n");
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
          // "VstNativeSynthIF::processEvent midi event is ME_SYSEX"
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
//   If ports is 0, just process controllers only, not audio (do not 'run').
//---------------------------------------------------------

// REMOVE Tim. autoconnect. Changed.
// iMPEvent VstNativeSynthIF::getData(MidiPort* /*mp*/, MPEventList* /*el*/, iMPEvent start_event, unsigned pos, int ports, unsigned nframes, float** buffer)
bool VstNativeSynthIF::getData(MidiPort* /*mp*/, unsigned pos, int ports, unsigned nframes, float** buffer)
{
  // We may not be using ev_buf_sz all at once - this will be just the maximum.
// REMOVE Tim. autoconnect. Changed.
//   const unsigned long ev_buf_sz = el->size() + synti->eventFifo.getSize();
  // This also takes an internal snapshot of the size for use later...
  // False = don't use the size snapshot, but update it.
//   const unsigned long ev_buf_sz = synti->eventFifos()->getSize(false);
//   VstMidiEvent events[ev_buf_sz];
//   char evbuf[sizeof(VstMidiEvent*) * ev_buf_sz + sizeof(VstEvents)];
//   VstEvents *vst_events = (VstEvents*)evbuf;
//   vst_events->numEvents = 0;
//   vst_events->reserved  = 0;

// REMOVE Tim. autoconnect. Removed.
//   const unsigned long frameOffset = MusEGlobal::audio->getFrameOffset();
// REMOVE Tim. autoconnect. Changed.
  const unsigned int syncFrame = MusEGlobal::audio->curSyncFrame();

  #ifdef VST_NATIVE_DEBUG_PROCESS
//   fprintf(stderr, "VstNativeSynthIF::getData: pos:%u ports:%d nframes:%u syncFrame:%lu ev_buf_sz:%lu\n", pos, ports, nframes, syncFrame, ev_buf_sz);
  fprintf(stderr, "VstNativeSynthIF::getData: pos:%u ports:%d nframes:%u syncFrame:%lu\n", pos, ports, nframes, syncFrame);
  #endif

  // All ports must be connected to something!
  const unsigned long in_ports = _synth->inPorts();
  const unsigned long out_ports = _synth->outPorts();
  const unsigned long nop = ((unsigned long) ports) > out_ports ? out_ports : ((unsigned long) ports);

  unsigned long sample = 0;

  const bool usefixedrate = (requiredFeatures() & Plugin::FixedBlockSize);

  // For now, the fixed size is clamped to the audio buffer size.
  // TODO: We could later add slower processing over several cycles -
  //  so that users can select a small audio period but a larger control period.
  const unsigned long min_per = (usefixedrate || MusEGlobal::config.minControlProcessPeriod > nframes) ? nframes : MusEGlobal::config.minControlProcessPeriod;
  const unsigned long min_per_mask = min_per-1;   // min_per must be power of 2

  AudioTrack* atrack = track();
  const AutomationType at = atrack->automationType();
  const bool no_auto = !MusEGlobal::automation || at == AUTO_OFF;
  const unsigned long in_ctrls = _synth->inControls();
  CtrlListList* cll = atrack->controller();
  ciCtrlList icl_first;
  const int plug_id = id();
  if(plug_id != -1 && ports != 0)  // Don't bother if not 'running'.
    icl_first = cll->lower_bound(genACnum(plug_id, 0));

  // Inform the host callback we are in the audio thread.
  _inProcess = true;

  #ifdef VST_NATIVE_DEBUG_PROCESS
  fprintf(stderr, "VstNativeSynthIF::getData: Handling inputs...\n");
  #endif
  
  bool used_in_chan_array[in_ports]; // Don't bother initializing if not 'running'. 
  
  // Don't bother if not 'running'.
  if(ports != 0)
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

  #ifdef VST_NATIVE_DEBUG_PROCESS
  fprintf(stderr, "VstNativeSynthIF::getData: Processing automation control values...\n");
  #endif

  int cur_slice = 0;
  while(sample < nframes)
  {
    unsigned long nsamp = nframes - sample;
    const unsigned long slice_frame = pos + sample;

    //
    // Process automation control values, while also determining the maximum acceptable
    //  size of this run. Further processing, from FIFOs for example, can lower the size
    //  from there, but this section determines where the next highest maximum frame
    //  absolutely needs to be for smooth playback of the controller value stream...
    //
    if(ports != 0)    // Don't bother if not 'running'.
    {
      ciCtrlList icl = icl_first;
      for(unsigned long k = 0; k < in_ctrls; ++k)
      {
        CtrlList* cl = (cll && plug_id != -1 && icl != cll->end()) ? icl->second : NULL;
        CtrlInterpolate& ci = _controls[k].interp;
        // Always refresh the interpolate struct at first, since things may have changed.
        // Or if the frame is outside of the interpolate range - and eStop is not true.  // FIXME TODO: Be sure these comparisons are correct.
        if(cur_slice == 0 || (!ci.eStop && MusEGlobal::audio->isPlaying() &&
            (slice_frame < (unsigned long)ci.sFrame || (ci.eFrame != -1 && slice_frame >= (unsigned long)ci.eFrame)) ) )
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
            ci.eFrame   = -1;
            ci.sVal     = _controls[k].val;
            ci.eVal     = ci.sVal;
            ci.doInterp = false;
            ci.eStop    = false;
          }
        }
        else
        {
          if(ci.eStop && ci.eFrame != -1 && slice_frame >= (unsigned long)ci.eFrame)  // FIXME TODO: Get that comparison right.
          {
            // Clear the stop condition and set up the interp struct appropriately as an endless value.
            ci.sFrame   = 0; //ci->eFrame;
            ci.eFrame   = -1;
            ci.sVal     = ci.eVal;
            ci.doInterp = false;
            ci.eStop    = false;
          }
          if(cl && cll && icl != cll->end())
            ++icl;
        }

        if(!usefixedrate && MusEGlobal::audio->isPlaying())
        {
          unsigned long samps = nsamp;
          if(ci.eFrame != -1)
            samps = (unsigned long)ci.eFrame - slice_frame;
          
          if(!ci.doInterp && samps > min_per)
          {
            samps &= ~min_per_mask;
            if((samps & min_per_mask) != 0)
              samps += min_per;
          }
          else
            samps = min_per;

          if(samps < nsamp)
            nsamp = samps;

        }

        float new_val;
        if(ci.doInterp && cl)
          new_val = cl->interpolate(MusEGlobal::audio->isPlaying() ? slice_frame : pos, ci);
        else
          new_val = ci.sVal;
        if(_controls[k].val != new_val)
        {
          _controls[k].val = new_val;
          if(dispatch(26 /*effCanBeAutomated*/, k, 0, NULL, 0.0f) == 1)
          {
            if(_plugin->getParameter(_plugin, k) != new_val)
              _plugin->setParameter(_plugin, k, new_val);
          }
  #ifdef VST_NATIVE_DEBUG
          else
            fprintf(stderr, "VstNativeSynthIF::getData %s parameter:%lu cannot be automated\n", name().toLatin1().constData(), k);
  #endif
        }

#ifdef VST_NATIVE_DEBUG_PROCESS
        fprintf(stderr, "VstNativeSynthIF::getData k:%lu sample:%lu frame:%lu ci.eFrame:%d nsamp:%lu \n", k, sample, frame, ci.eFrame, nsamp);
#endif
        
      }
    }
    
#ifdef VST_NATIVE_DEBUG_PROCESS
      fprintf(stderr, "VstNativeSynthIF::getData sample:%lu nsamp:%lu\n", sample, nsamp);
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
        fprintf(stderr, "VstNativeSynthIF::getData *** Error: evframe:%lu < frame:%lu event: frame:%lu idx:%lu val:%f unique:%d\n",
          evframe, frame, v.frame, v.idx, v.value, v.unique);

        // No choice but to ignore it.
        _controlFifo.remove();               // Done with the ring buffer's item. Remove it.
        continue;
      }

      if(evframe >= nframes                                                         // Next events are for a later period.
          || (!usefixedrate && !found && !v.unique && (evframe - sample >= nsamp))  // Next events are for a later run in this period. (Autom took prio.)
          || (found && !v.unique && (evframe - sample >= min_per))                  // Eat up events within minimum slice - they're too close.
          || (usefixedrate && found && v.unique && v.idx == index))                 // Fixed rate and must reply to all.
        break;
//       _controlFifo.remove();               // Done with the ring buffer's item. Remove it.

      if(v.idx >= in_ctrls) // Sanity check.
      {
        _controlFifo.remove();               // Done with the ring buffer's item. Remove it.
        break;
      }

      found = true;
      frame = evframe;
      index = v.idx;

      if(ports == 0)                     // Don't bother if not 'running'.
      {
        _controls[v.idx].val = v.value;   // Might as well at least update these.
// #ifndef VST_VESTIGE_SUPPORT
//         if(dispatch(effCanBeAutomated, v.idx, 0, NULL, 0.0f) == 1)
//         {
// #endif
//           if(v.value != _plugin->getParameter(_plugin, v.idx))
//             _plugin->setParameter(_plugin, v.idx, v.value);
// #ifndef VST_VESTIGE_SUPPORT
//         }
//   #ifdef VST_NATIVE_DEBUG
//         else
//           fprintf(stderr, "VstNativeSynthIF::getData %s parameter:%lu cannot be automated\n", name().toLatin1().constData(), v.idx);
//   #endif
// #endif
      }
      else
      {
        CtrlInterpolate* ci = &_controls[v.idx].interp;
        // Tell it to stop the current ramp at this frame, when it does stop, set this value:
        ci->eFrame = frame;
        ci->eVal   = v.value;
        ci->eStop  = true;
      }

      // Need to update the automation value, otherwise it overwrites later with the last automation value.
      if(plug_id != -1)
        synti->setPluginCtrlVal(genACnum(plug_id, v.idx), v.value);
      
      _controlFifo.remove();               // Done with the ring buffer's item. Remove it.
    }

    if(found && !usefixedrate)  // If a control FIFO item was found, takes priority over automation controller stream.
      nsamp = frame - sample;

    if(sample + nsamp > nframes)         // Safety check.
      nsamp = nframes - sample;

    // TODO: Don't allow zero-length runs. This could/should be checked in the control loop instead.
    // Note this means it is still possible to get stuck in the top loop (at least for a while).
    if(nsamp != 0)
    {
      unsigned long nevents = 0;
// REMOVE Tim. autoconnect. Changed.
//       if(ports != 0)  // Don't bother if not 'running'.
//       {
//         // Process event list events...
//         for(; start_event != el->end(); ++start_event)
//         {
//           #ifdef VST_NATIVE_DEBUG
// // REMOVE Tim. autoconnect. Changed.
// //           fprintf(stderr, "VstNativeSynthIF::getData eventlist event time:%d pos:%u sample:%lu nsamp:%lu frameOffset:%d\n", 
// //                   start_event->time(), pos, sample, nsamp, frameOffset);
//           fprintf(stderr, "VstNativeSynthIF::getData eventlist event time:%d pos:%u sample:%lu nsamp:%lu syncFrame:%u\n", 
//                   start_event->time(), pos, sample, nsamp, syncFrame);
//           #endif
// 
// // REMOVE Tim. autoconnect. Changed.
// //           if(start_event->time() >= (pos + sample + nsamp + frameOffset))  // frameOffset? Test again...
//           if(start_event->time() >= (sample + nsamp + syncFrame))
//           {
//             #ifdef VST_NATIVE_DEBUG
// //             fprintf(stderr, " event is for future:%lu, breaking loop now\n", start_event->time() - frameOffset - pos - sample);
//             fprintf(stderr, " event is for future:%lu, breaking loop now\n", start_event->time() - syncFrame - sample);
//             #endif
//             break;
//           }
// 
// // REMOVE Tim. autoconnect. Removed.
// //           // Update hardware state so knobs and boxes are updated. Optimize to avoid re-setting existing values.
// //           // Same code as in MidiPort::sendEvent()
// //           if(mp && !mp->sendHwCtrlState(*start_event, false))
// //             continue;
// 
//           // Returns false if the event was not filled. It was handled, but some other way.
//           if(processEvent(*start_event, &events[nevents]))
//           {
//             // Time-stamp the event.
// // REMOVE Tim. autoconnect. Changed.
// //             int ft = start_event->time() - frameOffset - pos - sample;
// //             if(ft < 0)
// //               ft = 0;
//             unsigned int ft = (start_event->time() < syncFrame) ? 0 : start_event->time() - syncFrame;
//             ft = (ft < sample) ? 0 : ft - sample;
// 
// //             if (ft >= int(nsamp))
//             if (ft >= nsamp)
//             {
// //                 fprintf(stderr, "VstNativeSynthIF::getData: eventlist event time:%d out of range. pos:%d offset:%ld ft:%d sample:%lu nsamp:%lu\n", 
// //                         start_event->time(), pos, frameOffset, ft, sample, nsamp);
//                 fprintf(stderr, "VstNativeSynthIF::getData: eventlist event time:%d out of range. pos:%d syncFrame:%u ft:%u sample:%lu nsamp:%lu\n", 
//                         start_event->time(), pos, syncFrame, ft, sample, nsamp);
//                 ft = nsamp - 1;
//             }
// 
//             #ifdef VST_NATIVE_DEBUG
// //             fprintf(stderr, "VstNativeSynthIF::getData eventlist: ft:%d current nevents:%lu\n", ft, nevents);
//             fprintf(stderr, "VstNativeSynthIF::getData eventlist: ft:%u current nevents:%lu\n", ft, nevents);
//             #endif
// 
//             vst_events->events[nevents] = (VstEvent*)&events[nevents];
//             events[nevents].deltaFrames = ft;
//             ++nevents;
//           }
//         }
//       }
//       
//       // Now process putEvent events...
//       while(!synti->eventFifo.isEmpty())
//       {
//         MidiPlayEvent e = synti->eventFifo.peek();
// 
//         #ifdef VST_NATIVE_DEBUG
//         fprintf(stderr, "VstNativeSynthIF::getData eventFifo event time:%d\n", e.time());
//         #endif
// 
// // REMOVE Tim. autoconnect. Changed.
// //         if(e.time() >= (pos + sample + nsamp + frameOffset))
//         if(e.time() >= (sample + nsamp + syncFrame))
//           break;
// 
//         synti->eventFifo.remove();    // Done with ring buffer's event. Remove it.
//         if(ports != 0)  // Don't bother if not 'running'.
//         {
//           // Returns false if the event was not filled. It was handled, but some other way.
//           if(processEvent(e, &events[nevents]))
//           {
//             // Time-stamp the event.
// // REMOVE Tim. autoconnect. Changed.
// //             long ft = e.time() - frameOffset - pos  - sample;
// //             if(ft < 0)
// //               ft = 0;
//             unsigned int ft = (e.time() < syncFrame) ? 0 : e.time() - syncFrame;
//             ft = (ft < sample) ? 0 : ft - sample;
// 
// //             if (ft >= long(nsamp))
//             if (ft >= nsamp)
//             {
// //                 fprintf(stderr, "VstNativeSynthIF::getData: eventFifo event time:%d out of range. pos:%d offset:%ld ft:%ld sample:%lu nsamp:%lu\n", 
// //                         e.time(), pos, frameOffset, ft, sample, nsamp);
//                 fprintf(stderr, "VstNativeSynthIF::getData: eventFifo event time:%d out of range. pos:%d syncFrame:%u ft:%u sample:%lu nsamp:%lu\n", 
//                         e.time(), pos, syncFrame, ft, sample, nsamp);
//                 ft = nsamp - 1;
//             }
//             vst_events->events[nevents] = (VstEvent*)&events[nevents];
//             events[nevents].deltaFrames = ft;
// 
//             ++nevents;
//           }
//         }
//       }

      // Get the state of the stop flag.
      const bool do_stop = synti->stopFlag();

      MidiPlayEvent buf_ev;
      
      // Transfer the user lock-free buffer events to the user sorted multi-set.
      const unsigned int usr_buf_sz = synti->userEventBuffers()->bufferCapacity();
      for(unsigned int i = 0; i < usr_buf_sz; ++i)
      {
        if(synti->userEventBuffers()->get(buf_ev, i))
          synti->_outUserEvents.add(buf_ev);
      }
      
      // Transfer the playback lock-free buffer events to the playback sorted multi-set.
      // Don't bother adding to the set if we're stopping, but do get the buffer item.
      const unsigned int pb_buf_sz = synti->playbackEventBuffers()->bufferCapacity();
      for(unsigned int i = 0; i < pb_buf_sz; ++i)
        if(synti->playbackEventBuffers()->get(buf_ev, i) && !do_stop)
          synti->_outPlaybackEvents.add(buf_ev);

      // Are we stopping?
      if(do_stop)
      //{
        // Transport has stopped, purge ALL further scheduled playback events now.
        //synti->_outPlaybackEvents.clear();
        // Reset the flag.
        synti->setStopFlag(false);
      //}
      else
      //{
        // For convenience, simply transfer all playback events into the other user list. 
        //for(ciMPEvent impe = synti->_outPlaybackEvents.begin(); impe != synti->_outPlaybackEvents.end(); ++impe)
        //  synti->_outUserEvents.add(*impe);
        synti->_outUserEvents.insert(synti->_outPlaybackEvents.begin(), synti->_outPlaybackEvents.end());
      //}
      // Done with playback event list. Clear it.  
      synti->_outPlaybackEvents.clear();
      
      // Count how many events we need.
      for(ciMPEvent impe = synti->_outUserEvents.begin(); impe != synti->_outUserEvents.end(); )
      {
        const MidiPlayEvent& e = *impe;
        if(e.time() >= (syncFrame + sample + nsamp))
          break;
        ++nevents;
      }
      
      VstMidiEvent events[nevents];
      char evbuf[sizeof(VstMidiEvent*) * nevents + sizeof(VstEvents)];
      VstEvents *vst_events = (VstEvents*)evbuf;
      vst_events->numEvents = 0;
      vst_events->reserved  = 0;
  
//       // Now process putEvent events...
//       for(long unsigned int rb_idx = 0; rb_idx < ev_buf_sz; ++rb_idx)
      unsigned long event_counter = 0;
      for(iMPEvent impe = synti->_outUserEvents.begin(); impe != synti->_outUserEvents.end() && event_counter <= nevents; )
      {
//         // True = use the size snapshot.
//         const MidiPlayEvent& e = synti->eventFifos()->peek(true);
        const MidiPlayEvent& e = *impe;

        #ifdef VST_NATIVE_DEBUG
        fprintf(stderr, "VstNativeSynthIF::getData eventFifos event time:%d\n", e.time());
        #endif

        // Event is for future?
        if(e.time() >= (sample + nsamp + syncFrame))
          break;

//         // Done with ring buffer's event. Remove it.
//         // True = use the size snapshot.
//         synti->eventFifos()->remove(true);
        if(ports != 0)  // Don't bother if not 'running'.
        {
          // Returns false if the event was not filled. It was handled, but some other way.
//           if(processEvent(e, &events[nevents]))
          if(processEvent(e, &events[event_counter]))
          {
            // Time-stamp the event.
            unsigned int ft = (e.time() < syncFrame) ? 0 : e.time() - syncFrame;
            ft = (ft < sample) ? 0 : ft - sample;

            if(ft >= nsamp)
            {
                fprintf(stderr, "VstNativeSynthIF::getData: eventFifos event time:%d out of range. pos:%d syncFrame:%u ft:%u sample:%lu nsamp:%lu\n", 
                        e.time(), pos, syncFrame, ft, sample, nsamp);
                ft = nsamp - 1;
            }
//             vst_events->events[nevents] = (VstEvent*)&events[nevents];
            vst_events->events[event_counter] = (VstEvent*)&events[event_counter];
//             events[nevents].deltaFrames = ft;
            events[event_counter].deltaFrames = ft;

//             ++nevents;
            ++event_counter;
          }
        }
        // Done with ring buffer's event. Remove it.
        // True = use the size snapshot.
//         synti->eventFifos()->remove(true);
        
        // C++11.
        impe = synti->_outUserEvents.erase(impe);
      }
      
      if(event_counter < nevents)
        nevents = event_counter;
      
      #ifdef VST_NATIVE_DEBUG_PROCESS
      fprintf(stderr, "VstNativeSynthIF::getData: Connecting and running. sample:%lu nsamp:%lu nevents:%lu\n", sample, nsamp, nevents);
      #endif

      if(ports != 0)  // Don't bother if not 'running'.
      {
        // Set the events pointer.
        if(nevents > 0)
        {
          vst_events->numEvents = nevents;
          dispatch(effProcessEvents, 0, 0, vst_events, 0.0f);
        }

        float* in_bufs[in_ports];
        float* out_bufs[out_ports];

        // Connect the given buffers directly to the ports, up to a max of synth ports.
        for(unsigned long k = 0; k < nop; ++k)
          out_bufs[k] = buffer[k] + sample;
        // Connect the remaining ports to some local buffers (not used yet).
        for(unsigned long k = nop; k < out_ports; ++k)
          out_bufs[k] = _audioOutBuffers[k] + sample;
        // Connect all inputs either to some local buffers, or a silence buffer.
        for(unsigned long k = 0; k < in_ports; ++k)
        {
          if(used_in_chan_array[k])
            in_bufs[k] = _audioInBuffers[k] + sample;
          else
            in_bufs[k] = _audioInSilenceBuf + sample;
        }

        // Run the synth for a period of time. This processes events and gets/fills our local buffers...
        if((_plugin->flags & effFlagsCanReplacing) && _plugin->processReplacing)
        {
          _plugin->processReplacing(_plugin, in_bufs, out_bufs, nsamp);
        }
      }
      
      sample += nsamp;
    }

    ++cur_slice; // Slice is done. Moving on to any next slice now...
  }
  
  // Inform the host callback we will be no longer in the audio thread.
  _inProcess = false;

// REMOVE Tim. autoconnect. Changed.
//   return start_event;
  return true;
}

// REMOVE Tim. autoconnect. Removed.
// //---------------------------------------------------------
// //   putEvent
// //---------------------------------------------------------
// 
// bool VstNativeSynthIF::putEvent(const MidiPlayEvent& ev)
//       {
//       #ifdef VST_NATIVE_DEBUG
//       fprintf(stderr, "VstNativeSynthIF::putEvent midi event time:%d chn:%d a:%d b:%d\n", ev.time(), ev.channel(), ev.dataA(), ev.dataB());
//       #endif
//       
//       if (MusEGlobal::midiOutputTrace)
//             ev.dump();
//       return synti->eventFifo.put(ev);
//       }


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
void VstNativeSynthIF::enableAllControllers(bool v)
{
  if(!_synth)
    return;
  const unsigned long sic = _synth->inControls();
  for(unsigned long i = 0; i < sic; ++i)
    _controls[i].enCtrl = v;
}
void VstNativeSynthIF::updateControllers() { }
void VstNativeSynthIF::activate()
{
  // Set some default properties
  dispatch(effSetSampleRate, 0, 0, NULL, MusEGlobal::sampleRate);
  dispatch(effSetBlockSize, 0, MusEGlobal::segmentSize, NULL, 0.0f);
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
  _active = true;
}
void VstNativeSynthIF::deactivate()
{
  _active = false;
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
void VstNativeSynthIF::setParam(unsigned long i, double val)       { setParameter(i, val); }
double VstNativeSynthIF::param(unsigned long i) const              { return getParameter(i); }
double VstNativeSynthIF::paramOut(unsigned long) const            { return 0.0; }
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
CtrlList::Mode VstNativeSynthIF::ctrlMode(unsigned long /*i*/) const     { return CtrlList::INTERPOLATE; }

VstNativePluginWrapper::VstNativePluginWrapper(VstNativeSynth *s, PluginFeatures reqFeatures)
{
   _synth = s;

   _requiredFeatures = reqFeatures;
   
   _fakeLd.Label = strdup(_synth->name().toUtf8().constData());
   _fakeLd.Name = strdup(_synth->name().toUtf8().constData());
   _fakeLd.UniqueID = _synth->_id;
   _fakeLd.Maker = strdup(_synth->maker().toUtf8().constData());
   _fakeLd.Copyright = strdup(_synth->version().toUtf8().constData());
   _isVstNativePlugin = true;
   _isVstNativeSynth = s->isSynth();
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


   _fakeLd.PortNames = NULL;
   _fakeLd.PortRangeHints = NULL;
   _fakeLd.PortDescriptors = _fakePds;
   _fakeLd.Properties = 0;
   plugin = &_fakeLd;
   _isDssi = false;
   _isDssiSynth = false;
   _isLV2Plugin = false;
   _isLV2Synth = false;

#ifdef DSSI_SUPPORT
   dssi_descr = NULL;
#endif

   fi = _synth->info;
   ladspa = NULL;
   _handle = 0;
   _references = 0;
   _instNo     = 0;
   _label = _synth->name();
   _name = _synth->description();
   _uniqueID = plugin->UniqueID;
   _maker = _synth->maker();
   _copyright = _synth->version();

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

   state->pluginI = pluginI;
   state->pluginWrapper = this;
   state->inPorts.resize(_inports);
   state->outPorts.resize(_outports);
   state->inControlPorts.resize(_controlInPorts);
   state->inControlLastValues.resize(_controlInPorts);
   bool refillDefCtrls = false;
   if(inControlDefaults.size() == 0)
   {
      refillDefCtrls = true;
      inControlDefaults.resize(_controlInPorts);
      portNames.resize(_inports + _outports + _controlInPorts);
   }
   memset(&state->inPorts [0], 0, _inports * sizeof(float *));
   memset(&state->outPorts [0], 0, _outports * sizeof(float *));
   memset(&state->inControlPorts [0], 0, _controlInPorts * sizeof(float *));

   if(refillDefCtrls)
   {
      for(size_t i = 0; i < _controlInPorts; i++)
      {
         if(state->plugin->getParameter)
         {
            state->inControlLastValues [i] = inControlDefaults [i] = state->plugin->getParameter(state->plugin, i);
         }         
      }


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
   }

   QObject::connect(MusEGlobal::heartBeatTimer, SIGNAL(timeout()), state, SLOT(heartBeat()));

   return(LADSPA_Handle)state;

}

int VstNativePluginWrapper::incReferences(int ref)
{
   _synth->incInstances(ref);
   return _synth->instances();
}

void VstNativePluginWrapper::activate(LADSPA_Handle handle)
{
   VstNativePluginWrapper_State *state = (VstNativePluginWrapper_State *)handle;
   // Set some default properties
   dispatch(state, effSetSampleRate, 0, 0, NULL, MusEGlobal::sampleRate);
   dispatch(state, effSetBlockSize, 0, MusEGlobal::segmentSize, NULL, 0.0f);
   //for (unsigned short i = 0; i < instances(); ++i) {
   //        dispatch(i, effMainsChanged, 0, 1, NULL, 0.0f);
   dispatch(state, effMainsChanged, 0, 1, NULL, 0.0f);
   dispatch(state, 71 /*effStartProcess*/, 0, 0, NULL, 0.0f);

   if(state->plugin->getParameter)
   {
      for(size_t i = 0; i < _controlInPorts; i++)
      {
         state->pluginI->controls [i].val = state->pluginI->controls [i].tmpVal = inControlDefaults [i];
      }
   }
   state->active = true;
}

void VstNativePluginWrapper::deactivate(LADSPA_Handle handle)
{
   VstNativePluginWrapper_State *state = (VstNativePluginWrapper_State *)handle;
   if(!state)
   {
      return;
   }
   state->active = false;
   dispatch(state, 72 /*effStopProcess*/, 0, 0, NULL, 0.0f);
   dispatch(state, effMainsChanged, 0, 0, NULL, 0.0f);
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
     state->editor = NULL;
     state->guiVisible = false;
   }

   if (state->plugin)
   {
      dispatch(state, effClose, 0, 0, NULL, 0);
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

void VstNativePluginWrapper::apply(LADSPA_Handle handle, unsigned long n)
{
   VstNativePluginWrapper_State *state = (VstNativePluginWrapper_State *)handle;
   state->inProcess = true;
   if(state->pluginI->controls)
   {
      for(size_t i = 0; i < _controlInPorts; i++)
      {
         if(state->pluginI->controls [i].val == state->inControlLastValues [i])
         {
            continue;
         }
         state->inControlLastValues [i] = state->pluginI->controls [i].val;
         if(dispatch(state, 26 /*effCanBeAutomated*/, i, 0, NULL, 0.0f) == 1)
         {
            if(state->plugin->getParameter && state->plugin->setParameter)
            {
               if(state->plugin->getParameter(state->plugin, i) != state->inControlLastValues [i])
                  state->plugin->setParameter(state->plugin, i, state->inControlLastValues [i]);
            }
         }

      }
   }
   if((state->plugin->flags & effFlagsCanReplacing) && state->plugin->processReplacing)
   {
     state->plugin->processReplacing(state->plugin, &state->inPorts [0], &state->outPorts [0], n);
   }
   else if(state->plugin->process)
   {
      state->plugin->process(state->plugin, &state->inPorts [0], &state->outPorts [0], n);
   }
   state->inProcess = false;

}

LADSPA_PortDescriptor VstNativePluginWrapper::portd(unsigned long k) const
{
   return _fakeLd.PortDescriptors[k];
}

LADSPA_PortRangeHint VstNativePluginWrapper::range(unsigned long)
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

double VstNativePluginWrapper::defaultValue(unsigned long port) const
{
   return inControlDefaults [port];
}

const char *VstNativePluginWrapper::portName(unsigned long port)
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

bool VstNativePluginWrapper::hasNativeGui()
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
         Qt::WindowFlags wflags = Qt::Window
                                  | Qt::CustomizeWindowHint
                                  | Qt::WindowTitleHint
                                  | Qt::WindowSystemMenuHint
                                  | Qt::WindowMinMaxButtonsHint
                                  | Qt::WindowCloseButtonHint;
         state->editor = new MusEGui::VstNativeEditor(NULL, wflags);
         state->editor->open(0, state);
      }
   }
   else
   {
      if(state->editor)
      {
         state->editor->close();
         //_editor = NULL;  // No - done in editorDeleted.
      }
   }
   state->guiVisible = bShow;
}

bool VstNativePluginWrapper::nativeGuiVisible(PluginI *p)
{
   assert(p->instances > 0);
   VstNativePluginWrapper_State *state = (VstNativePluginWrapper_State *)p->handle [0];
   return state->guiVisible;
}

void VstNativePluginWrapper::writeConfiguration(LADSPA_Handle handle, int level, Xml &xml)
{
   VstNativePluginWrapper_State *state = (VstNativePluginWrapper_State *)handle;
   if(_synth->hasChunks())
   {
      //---------------------------------------------
      // dump current state of synth
      //---------------------------------------------
      fprintf(stderr, "%s: commencing chunk data dump, plugin api version=%d\n", name().toLatin1().constData(), _synth->vstVersion());
      unsigned long len = 0;
      void* p = 0;
      len = dispatch(state, 23 /* effGetChunk */, 0, 0, &p, 0.0); // index 0: is bank 1: is program
      if (len)
      {
         QByteArray arrOut = QByteArray::fromRawData((char *)p, len);

         QByteArray outEnc64 = arrOut.toBase64();
         QString customData(outEnc64);
         for (int pos=0; pos < customData.size(); pos+=150)
         {
            customData.insert(pos++,'\n'); // add newlines for readability
         }
         xml.strTag(level, "customData", customData);
      }

   }
}

void VstNativePluginWrapper::setCustomData(LADSPA_Handle handle, const std::vector<QString> &customParams)
{
   if(customParams.size() == 0)
      return;

   VstNativePluginWrapper_State *state = (VstNativePluginWrapper_State *)handle;
   if(!_synth->hasChunks())
   {
      return;
   }

   for(size_t i = 0; i < customParams.size(); i++)
   {
      QString param = customParams [i];
      param.remove('\n'); // remove all linebreaks that may have been added to prettyprint the songs file
      QByteArray paramIn;
      paramIn.append(param);
      QByteArray dec64 = QByteArray::fromBase64(paramIn);
      dispatch(state, 24 /* effSetChunk */, 0, dec64.size(), (void*)dec64.data(), 0.0); // index 0: is bank 1: is program
      break; //one customData tag includes all data in base64
   }
}

void VstNativePluginWrapper_State::heartBeat()
{
   if(plugin && active)
   {
      if(guiVisible)
      {
        plugin->dispatcher(plugin, effEditIdle, 0, 0, NULL, 0.0f);
        if(editor)
          editor->update();
      }
   }

}

} // namespace MusECore

#else  // VST_NATIVE_SUPPORT
namespace MusECore {
void initVST_Native() {}
} // namespace MusECore
#endif

