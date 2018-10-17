//=========================================================
//  MusE
//  Linux Music Editor
//  
//  muse_plugin_scan.cpp
//  (C) Copyright 2018 Tim E. Real (terminator356 at users.sourceforge.net)
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
//=========================================================

#include <stdio.h>
// #include <stdlib.h>
#include <unistd.h>
// #include <string.h>

#include <dlfcn.h>
#include <ladspa.h>

#include <QDir>
#include <QStringList>
#include <sys/stat.h>

#include "config.h"
#include "globaldefs.h"
#include "plugin_scan.h"
#include "synti/libsynti/mess.h"

#ifdef DSSI_SUPPORT
#include <dssi.h>
#endif


#ifdef VST_NATIVE_SUPPORT

#define OLD_PLUGIN_ENTRY_POINT "main"
#define NEW_PLUGIN_ENTRY_POINT "VSTPluginMain"

#ifdef VST_SDK_QUIRK
#define __cdecl
#endif

#include "aeffectx.h"

#ifdef VST_VESTIGE_SUPPORT
#ifndef effGetProgramNameIndexed
#define effGetProgramNameIndexed 29
#endif
#endif

#ifndef VST_2_4_EXTENSIONS
#ifndef VST_VESTIGE_SUPPORT
typedef long     VstInt32;
typedef long     VstIntPtr;
#else
typedef int32_t  VstInt32;
typedef intptr_t VstIntPtr;
#define VSTCALLBACK
#endif
#endif

// #include "vst_native_editor.h"
// #include "synth.h"
// #include "plugin.h"
// #include "midictrl.h"
//#include <semaphore.h>
//#include <QSemaphore>

#endif // VST_NATIVE_SUPPORT

// For debugging output: Uncomment the fprintf section.
#define DEBUG_PLUGIN_SCAN(dev, format, args...) // fprintf(dev, format, ##args);


// #include <semaphore.h>
// static sem_t _vstIdLock;
// #include <QSemaphore>
// static QSemaphore _vstIdLock;


//-----------------------------------------------------------------------------------------
//   vstHostCallback
//   This must be a function, it cannot be a class method so we dispatch to various objects from here.
//-----------------------------------------------------------------------------------------

#ifdef VST_NATIVE_SUPPORT

static VstIntPtr currentPluginId = 0;
//static sem_t _vstIdLock;
//static QSemaphore _vstIdLock;

VstIntPtr VSTCALLBACK vstNativeHostCallback(AEffect* effect, VstInt32 opcode, VstInt32 /*index*/, VstIntPtr /*value*/, void* ptr, float /*opt*/)
{
      // Is this callback for an actual instance? Hand-off to the instance if so.
      //VSTPlugin* plugin;
      if(effect && effect->user)
      {
        /*
        VstNativeSynthOrPlugin *userData = (VstNativeSynthOrPlugin*)(effect->user);
        //return ((VstNativeSynthIF*)plugin)->hostCallback(opcode, index, value, ptr, opt);
        return VstNativeSynth::pluginHostCallback(userData, opcode, index, value, ptr, opt);
        */
        return 0;
      }

      // No instance found. So we are just scanning for plugins...
    
      DEBUG_PLUGIN_SCAN(stderr, "plugin_scan: vstNativeHostCallback eff:%p opcode:%ld\n", effect, opcode);
      
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
                  //return MusEGlobal::sampleRate;
                  return 44100;

            case audioMasterGetBlockSize:
                  //return MusEGlobal::segmentSize;
                  return 512;

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

        DEBUG_PLUGIN_SCAN(stderr, "  unknown opcode\n");

      return 0;
      }
      
//---------------------------------------------------------
//   loadPluginLib
//---------------------------------------------------------

static bool scanSubPlugin(int level, MusECore::Xml& xml,
                          const char* filename,
                          AEffect *plugin,
                          long int id,
                          void * /*handle*/)
{
  char buffer[128];
  QString effectName;
  QString vendorString;
  QString productString;
  int vendorVersion;
  QString vendorVersionString;
  int vst_version = 0;

  if(!(plugin->flags & effFlagsHasEditor))
  {
    DEBUG_PLUGIN_SCAN(stderr, "Plugin has no GUI\n");
  }
  else
  {
    DEBUG_PLUGIN_SCAN(stderr, "Plugin has a GUI\n");
  }

  if(!(plugin->flags & effFlagsCanReplacing))
  {
    DEBUG_PLUGIN_SCAN(stderr, "Plugin does not support processReplacing\n");
  }
  else
  {
    DEBUG_PLUGIN_SCAN(stderr, "Plugin supports processReplacing\n");
  }

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
  vendorVersionString = QString("%1.%2.%3").arg((vendorVersion >> 16) & 0xff).arg((vendorVersion >> 8) & 0xff).arg(vendorVersion & 0xff);

  QFileInfo fi(filename);
  
  // Some (older) plugins don't have any of these strings. We only have the filename to use.
  if(effectName.isEmpty())
    effectName = fi.completeBaseName();
  if(productString.isEmpty())
    //productString = fi.completeBaseName();
    productString = effectName;

//    // Make sure it doesn't already exist.
//    for(is = MusEGlobal::synthis.begin(); is != MusEGlobal::synthis.end(); ++is)
//      if((*is)->name() == effectName && (*is)->baseName() == fi.completeBaseName())
//      {
//         fprintf(stderr, "VST %s already exists!\n", (char *)effectName.toUtf8().constData());
//        return false;
//      }

  // "2 = VST2.x, older versions return 0". Observed 2400 on all the ones tested so far.
  vst_version = plugin->dispatcher(plugin, effGetVstVersion, 0, 0, NULL, 0.0f);
  
  MusECore::PluginScanInfo info;
  info._fi = fi;
  info._type = MusECore::PluginScanInfo::PluginTypeLinuxVST;
  info._class = MusECore::PluginScanInfo::PluginClassEffect;
  info._uniqueID = plugin->uniqueID;
  info._subID = id;
  info._label = effectName;
  info._name = effectName;
  info._maker = vendorString;
  info._description = productString;
  //info._copyright = descr->Copyright;
  info._version = vendorVersionString;

  info._apiVersionMajor = vst_version;
  info._apiVersionMinor = 0;
  
  info._portCount = plugin->numInputs + plugin->numOutputs + plugin->numParams;
  info._inports = plugin->numInputs;
  info._outports = plugin->numOutputs;
  info._controlInPorts = plugin->numParams;
  info._controlOutPorts = 0;
  

  unsigned long k = 0;
  for(int i = 0; i < plugin->numInputs; ++i)
  {
    MusECore::PluginPortInfo port_info;
    port_info._index = k;
    port_info._type = MusECore::PluginPortInfo::AudioPort | MusECore::PluginPortInfo::InputPort;
    info._portList.push_back(port_info);
    ++k;
  }
  
  for(int i = 0; i < plugin->numOutputs; ++i)
  {
    MusECore::PluginPortInfo port_info;
    port_info._index = k;
    port_info._type = MusECore::PluginPortInfo::AudioPort | MusECore::PluginPortInfo::OutputPort;
    info._portList.push_back(port_info);
    ++k;
  }
  
  for(int i = 0; i < plugin->numParams; ++i)
  {
    MusECore::PluginPortInfo port_info;
    char buf[256];
    buf[0] = 0;
    plugin->dispatcher(plugin, effGetParamName, i, 0, buf, 0);
    port_info._name = QString(buf);
    port_info._index = k;
    port_info._type = MusECore::PluginPortInfo::ControlPort | MusECore::PluginPortInfo::InputPort;
    info._portList.push_back(port_info);
    ++k;
  }

// TODO
  if((info._inports != info._outports)  /*|| LADSPA_IS_INPLACE_BROKEN(ladspa_descr->Properties)*/ )
    info._requiredFeatures |= MusECore::PluginNoInPlaceProcessing;

//   if()
//   {
//     info._requiredFeatures |= MusECore::PluginScanInfo::FixedBlockSize;
//   }

  // "2 = VST2.x, older versions return 0". Observed 2400 on all the ones tested so far.
  if(vst_version >= 2)
  {
    if(plugin->dispatcher(plugin, effCanDo, 0, 0, (void*)"receiveVstEvents", 0.0f) > 0)
      info._vstPluginFlags |= MusECore::canReceiveVstEvents;
    if(plugin->dispatcher(plugin, effCanDo, 0, 0, (void*)"sendVstEvents", 0.0f) > 0)
      info._vstPluginFlags |= MusECore::canSendVstEvents;
    if(plugin->dispatcher(plugin, effCanDo, 0, 0, (void*)"sendVstMidiEvent", 0.0f) > 0)
      info._vstPluginFlags |= MusECore::canSendVstMidiEvents;
    if(plugin->dispatcher(plugin, effCanDo, 0, 0, (void*)"sendVstTimeInfo", 0.0f) > 0)
      info._vstPluginFlags |= MusECore::canSendVstTimeInfo;
    if(plugin->dispatcher(plugin, effCanDo, 0, 0, (void*)"receiveVstMidiEvent", 0.0f) > 0)
      info._vstPluginFlags |= MusECore::canReceiveVstMidiEvents;
    if(plugin->dispatcher(plugin, effCanDo, 0, 0, (void*)"receiveVstTimeInfo", 0.0f) > 0)
      info._vstPluginFlags |= MusECore::canReceiveVstTimeInfo;
    if(plugin->dispatcher(plugin, effCanDo, 0, 0, (void*)"offline", 0.0f) > 0)
      info._vstPluginFlags |= MusECore::canProcessVstOffline;
    if(plugin->dispatcher(plugin, effCanDo, 0, 0, (void*)"plugAsChannelInsert", 0.0f) > 0)
      info._vstPluginFlags |= MusECore::canUseVstAsInsert;
    if(plugin->dispatcher(plugin, effCanDo, 0, 0, (void*)"plugAsSend", 0.0f) > 0)
      info._vstPluginFlags |= MusECore::canUseVstAsSend;
    if(plugin->dispatcher(plugin, effCanDo, 0, 0, (void*)"mixDryWet", 0.0f) > 0)
      info._vstPluginFlags |= MusECore::canMixVstDryWet;
    if(plugin->dispatcher(plugin, effCanDo, 0, 0, (void*)"midiProgramNames", 0.0f) > 0)
      info._vstPluginFlags |= MusECore::canVstMidiProgramNames;
  }

  if((plugin->flags & effFlagsIsSynth) || 
    (vst_version >= 2 && plugin->dispatcher(plugin, effCanDo, 0, 0,(void*) "receiveVstEvents", 0.0f) > 0))
  {
    info._class |= MusECore::PluginScanInfo::PluginClassInstrument;
  }

  info.write(level, xml);

  return true;
}
      
#endif // VST_NATIVE_SUPPORT


QString dssiUiFilename(const MusECore::PluginScanInfo& info)
{
  QString libr(info.lib());
  if(info.dirPath().isEmpty() || libr.isEmpty())
    return QString();

  QString guiPath(info.dirPath() + "/" + libr);

  QDir guiDir(guiPath, "*", QDir::Unsorted, QDir::Files);
  if(!guiDir.exists())
    return QString();

  QStringList list = guiDir.entryList();

//   QString plug(pluginLabel());
  QString plug(info._label);
  QString lib_qt_ui;
  QString lib_any_ui;
  QString plug_qt_ui;
  QString plug_any_ui;

  for(int i = 0; i < list.count(); ++i)
  {
    QFileInfo fi(guiPath + QString("/") + list[i]);
    QString gui(fi.filePath());
    struct stat buf;
    if(stat(gui.toLatin1().constData(), &buf))
      continue;
    if(!((S_ISREG(buf.st_mode) || S_ISLNK(buf.st_mode)) &&
        (buf.st_mode & (S_IXUSR | S_IXGRP | S_IXOTH))))
      continue;

    // FIXME: Qt::CaseInsensitive - a quick and dirty way to accept any suffix. Should be case sensitive...
    if(!libr.isEmpty())
    {
      if(lib_qt_ui.isEmpty() && list[i].contains(libr + QString("_qt"), Qt::CaseInsensitive))
        lib_qt_ui = gui;
      if(lib_any_ui.isEmpty() && list[i].contains(libr + QString('_') /*, Qt::CaseInsensitive*/))
        lib_any_ui = gui;
    }
    if(!plug.isEmpty())
    {
      if(plug_qt_ui.isEmpty() && list[i].contains(plug + QString("_qt"), Qt::CaseInsensitive))
        plug_qt_ui = gui;
      if(plug_any_ui.isEmpty() && list[i].contains(plug + QString('_') /*, Qt::CaseInsensitive*/))
        plug_any_ui = gui;
    }
  }

  // Prefer qt plugin ui
  if(!plug_qt_ui.isEmpty())
    return plug_qt_ui;
  // Prefer any plugin ui
  if(!plug_any_ui.isEmpty())
    return plug_any_ui;
  // Prefer qt lib ui
  if(!lib_qt_ui.isEmpty())
    return lib_qt_ui;
  // Prefer any lib ui
  if(!lib_any_ui.isEmpty())
    return lib_any_ui;

  // No suitable UI file found
  return QString();
};


//---------------------------------------------------------
//   loadPluginLib
//    Returns true on success
//---------------------------------------------------------

// static void loadPluginLib(QFileInfo* fi)
static bool loadPluginLib(const char* filename)
{
      DEBUG_PLUGIN_SCAN(stderr, "loadPluginLib: %s\n", filename);
      
//#ifdef VST_NATIVE_SUPPORT
//       sem_wait(&_vstIdLock);
      //_vstIdLock.acquire();
      currentPluginId = 0;
//#endif // VST_NATIVE_SUPPORT
              
      void* handle = dlopen(filename, RTLD_NOW);
      if (handle == 0)
      {
        fprintf(stderr, "loadPluginLib: dlopen(%s) failed: %s\n",
          filename, dlerror());
        return false;
      }

      bool ret = false;
      
      // Check if it's a DSSI plugin first...
      #ifdef DSSI_SUPPORT
      DSSI_Descriptor_Function dssi = (DSSI_Descriptor_Function)dlsym(handle, "dssi_descriptor");
      if(dssi)
      {
        // Open the standard out channel to communicate back to the main app.
        MusECore::Xml xml(stdout);
        xml.header();
        int level = 0;
        xml.nput(level++, "<muse version=\"%d.%d\">\n", xml.latestMajorVersion(), xml.latestMinorVersion());
        
        const DSSI_Descriptor* descr;
        for (unsigned long i = 0;; ++i)
        {
          descr = dssi(i);
          if (descr == 0)
                break;

          const LADSPA_Descriptor* ladspa_descr = descr->LADSPA_Plugin;
          
          MusECore::PluginScanInfo info;
          info._fi = QFileInfo(filename);
          info._type = MusECore::PluginScanInfo::PluginTypeDSSI;
          info._class = MusECore::PluginScanInfo::PluginClassEffect;
          info._uniqueID = ladspa_descr->UniqueID;
          info._label = ladspa_descr->Label;
          info._name = ladspa_descr->Name;
          //info._description = ladspa_descr->Name;
          info._maker = ladspa_descr->Maker;
          info._copyright = ladspa_descr->Copyright;

          info._apiVersionMajor = descr->DSSI_API_Version;
          info._apiVersionMinor = 0;
          
          info._portCount = ladspa_descr->PortCount;
          info._inports = 0;
          info._outports = 0;
          info._controlInPorts = 0;
          info._controlOutPorts = 0;
          for(unsigned long k = 0; k < ladspa_descr->PortCount; ++k)
          {
            MusECore::PluginPortInfo port_info;
            port_info._name = QString(ladspa_descr->PortNames[k]);
            port_info._index = k;

            LADSPA_PortDescriptor pd = ladspa_descr->PortDescriptors[k];
            if(pd & LADSPA_PORT_AUDIO)
            {
              port_info._type = MusECore::PluginPortInfo::AudioPort;
              if(pd & LADSPA_PORT_INPUT)
              {
                port_info._type |= MusECore::PluginPortInfo::InputPort;
                ++info._inports;
              }
              else
              if(pd & LADSPA_PORT_OUTPUT)
              {
                port_info._type |= MusECore::PluginPortInfo::OutputPort;
                ++info._outports;
              }
              //info.rpIdx.push_back((unsigned long)-1);
            }
            else
            if(pd & LADSPA_PORT_CONTROL)
            {
              port_info._type = MusECore::PluginPortInfo::ControlPort;
              if(pd & LADSPA_PORT_INPUT)
              {
                port_info._type |= MusECore::PluginPortInfo::InputPort;
                //info.rpIdx.push_back(info._controlInPorts);
                ++info._controlInPorts;
              }
              else
              if(pd & LADSPA_PORT_OUTPUT)
              {
                port_info._type |= MusECore::PluginPortInfo::OutputPort;
                //info.rpIdx.push_back((unsigned long)-1);
                ++info._controlOutPorts;
              }
            }
            info._portList.push_back(port_info);
          }
          
          if((info._inports != info._outports) || LADSPA_IS_INPLACE_BROKEN(ladspa_descr->Properties))
            info._requiredFeatures |= MusECore::PluginNoInPlaceProcessing;

          if(info._fi.completeBaseName() == QString("dssi-vst"))
          {
            info._type = MusECore::PluginScanInfo::PluginTypeDSSIVST;
            info._requiredFeatures |= MusECore::PluginFixedBlockSize;
          }

          if(descr->run_synth || descr->run_synth_adding ||
             descr->run_multiple_synths || descr->run_multiple_synths_adding)
            info._class |= MusECore::PluginScanInfo::PluginClassInstrument;
          
          info._uiFilename = dssiUiFilename(info);
          info._hasGui = !info._uiFilename.isEmpty();
          
          info.write(level, xml);
        }
        
        xml.tag(1, "/muse");
        ret = true;
      }
      else
      #endif
      // Check if it's a MESS plugin...
      {
        typedef const MESS* (*MESS_Function)();
        MESS_Function msynth = (MESS_Function)dlsym(handle, "mess_descriptor");
        if(msynth)
        {
          const MESS* descr = msynth();
          if (descr == 0)
          {
            fprintf(stderr, "loadPluginLib: no MESS descr found in %s\n", filename);
          }
          else
          {
            // Open the standard out channel to communicate back to the main app.
            MusECore::Xml xml(stdout);
            xml.header();
            int level = 0;
            xml.nput(level++, "<muse version=\"%d.%d\">\n", xml.latestMajorVersion(), xml.latestMinorVersion());
            
            MusECore::PluginScanInfo info;
            info._fi = QFileInfo(filename);
            info._type = MusECore::PluginScanInfo::PluginTypeMESS;
            info._class = MusECore::PluginScanInfo::PluginClassInstrument;
            info._uniqueID = 0;
            //info._label = descr->Label;
            info._name = descr->name;
            //info._maker = descr->Maker;
            //info._copyright = descr->Copyright;
            info._description = descr->description;
            info._version = descr->version;

            info._apiVersionMajor = descr->majorMessVersion;
            info._apiVersionMinor = descr->minorMessVersion;
          
            info._portCount = 0;
            info._inports = 0;
            info._outports = 0;
            info._controlInPorts = 0;
            info._controlOutPorts = 0;
            
            info._hasGui = true;
            
            info.write(level, xml);
            
            xml.tag(1, "/muse");
            ret = true;
          }
        }
        else
        // Check if it's a LADSPA plugin...
        {
          LADSPA_Descriptor_Function ladspa = (LADSPA_Descriptor_Function)dlsym(handle, "ladspa_descriptor");
          if(ladspa)
          {
            // Open the standard out channel to communicate back to the main app.
            MusECore::Xml xml(stdout);
            xml.header();
            int level = 0;
            xml.nput(level++, "<muse version=\"%d.%d\">\n", xml.latestMajorVersion(), xml.latestMinorVersion());
            
            const LADSPA_Descriptor* descr;
            for (unsigned long i = 0;; ++i)
            {
              descr = ladspa(i);
              if (descr == NULL)
                    break;

              MusECore::PluginScanInfo info;
              info._fi = QFileInfo(filename);
              info._type = MusECore::PluginScanInfo::PluginTypeLADSPA;
              info._class = MusECore::PluginScanInfo::PluginClassEffect;
              info._uniqueID = descr->UniqueID;
              info._label = descr->Label;
              info._name = descr->Name;
              info._maker = descr->Maker;
              info._copyright = descr->Copyright;

              info._portCount = descr->PortCount;
              info._inports = 0;
              info._outports = 0;
              info._controlInPorts = 0;
              info._controlOutPorts = 0;
              for(unsigned long k = 0; k < descr->PortCount; ++k)
              {
                MusECore::PluginPortInfo port_info;
                port_info._name = QString(descr->PortNames[k]);
                port_info._index = k;

                LADSPA_PortDescriptor pd = descr->PortDescriptors[k];
                if(pd & LADSPA_PORT_AUDIO)
                {
                  port_info._type = MusECore::PluginPortInfo::AudioPort;
                  if(pd & LADSPA_PORT_INPUT)
                  {
                    port_info._type |= MusECore::PluginPortInfo::InputPort;
                    ++info._inports;
                  }
                  else
                  if(pd & LADSPA_PORT_OUTPUT)
                  {
                    port_info._type |= MusECore::PluginPortInfo::OutputPort;
                    ++info._outports;
                  }
                  //info.rpIdx.push_back((unsigned long)-1);
                }
                else
                if(pd & LADSPA_PORT_CONTROL)
                {
                  port_info._type = MusECore::PluginPortInfo::ControlPort;
                  if(pd & LADSPA_PORT_INPUT)
                  {
                    port_info._type |= MusECore::PluginPortInfo::InputPort;
                    //info.rpIdx.push_back(info._controlInPorts);
                    ++info._controlInPorts;
                  }
                  else
                  if(pd & LADSPA_PORT_OUTPUT)
                  {
                    port_info._type |= MusECore::PluginPortInfo::OutputPort;
                    //info.rpIdx.push_back((unsigned long)-1);
                    ++info._controlOutPorts;
                  }
                }
                info._portList.push_back(port_info);
              }
              
              if((info._inports != info._outports) || LADSPA_IS_INPLACE_BROKEN(descr->Properties))
                info._requiredFeatures |= MusECore::PluginNoInPlaceProcessing;
              
              info.write(level, xml);
            }
            xml.tag(1, "/muse");
            ret = true;
          }
          else
          {
#ifdef VST_NATIVE_SUPPORT
            // Check if it's a LinuxVST plugin...
            AEffect *(*getInstance)(audioMasterCallback);
            getInstance = (AEffect*(*)(audioMasterCallback))dlsym(handle, NEW_PLUGIN_ENTRY_POINT);
            if(getInstance)
            {
              DEBUG_PLUGIN_SCAN(stderr, "VST entrypoint \"" NEW_PLUGIN_ENTRY_POINT "\" found\n");
            }
            else
            {
              DEBUG_PLUGIN_SCAN(stderr, "VST 2.4 entrypoint \"" NEW_PLUGIN_ENTRY_POINT "\" not found in library %s, looking for \""
                              OLD_PLUGIN_ENTRY_POINT "\"\n", filename);

              getInstance = (AEffect*(*)(audioMasterCallback))dlsym(handle, OLD_PLUGIN_ENTRY_POINT);
              if(getInstance)
              {
                DEBUG_PLUGIN_SCAN(stderr, "VST entrypoint \"" OLD_PLUGIN_ENTRY_POINT "\" found\n");
              }
              else
              {
                DEBUG_PLUGIN_SCAN(stderr, "ERROR: VST entrypoints \"" NEW_PLUGIN_ENTRY_POINT "\" or \""
                                OLD_PLUGIN_ENTRY_POINT "\" not found in library\n");
              }
            }
            
            if(getInstance)
            {
//               sem_wait(&_vstIdLock);
//               _vstIdLock.acquire();
//               currentPluginId = 0;
//               bool bDontDlCLose = false;
              AEffect *plugin = NULL;
              
              
              plugin = getInstance(vstNativeHostCallback);
              if(!plugin)
              {
                fprintf(stderr, "ERROR: Failed to instantiate plugin in VST library \"%s\"\n", filename);
                goto _end;
              }
              else
              {
                DEBUG_PLUGIN_SCAN(stderr, "plugin instantiated\n");
              }

              if(plugin->magic != kEffectMagic)
              {
                fprintf(stderr, "Not a VST plugin in library \"%s\"\n", filename);
                goto _end;
              }
              else
              {
                DEBUG_PLUGIN_SCAN(stderr, "plugin is a VST\n");
              }

              // Open the standard out channel to communicate back to the main app.
              MusECore::Xml xml(stdout);
              xml.header();
              int level = 0;
              xml.nput(level++, "<muse version=\"%d.%d\">\n", xml.latestMajorVersion(), xml.latestMinorVersion());
              
              if(plugin->dispatcher(plugin, 24 + 11 /* effGetCategory */, 0, 0, 0, 0) == 10 /* kPlugCategShell */)
              {
//                 bDontDlCLose = true;
                std::map<VstIntPtr, std::string> shellPlugs;
                char cPlugName [128];
                do
                {
                    memset(cPlugName, 0, sizeof(cPlugName));
                    VstIntPtr id = plugin->dispatcher(plugin, 24 + 46 /* effShellGetNextPlugin */, 0, 0, cPlugName, 0);
                    if(id != 0 && cPlugName [0] != 0)
                    {
                      shellPlugs.insert(std::make_pair(id, std::string(cPlugName)));
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
                      fprintf(stderr, "ERROR: Failed to instantiate plugin in VST library \"%s\", shell id=%ld\n",
                              filename, (long)currentPluginId);
                      goto _end;
                    }
                    scanSubPlugin(level, xml, filename, plugin, currentPluginId, handle);
                    currentPluginId = 0;
                }
              }
              else
              {
                scanSubPlugin(level, xml, filename, plugin, 0, 0);
              }


              //plugin->dispatcher(plugin, effMainsChanged, 0, 0, NULL, 0);
              if(plugin)
                  plugin->dispatcher(plugin, effClose, 0, 0, NULL, 0);

              xml.tag(1, "/muse");
              ret = true;
              
//               _end:
//               if(handle && !bDontDlCLose)
//                   dlclose(handle);
// 
//               sem_post(&_vstIdLock);
//               _vstIdLock.release();
              
              
              
              
              
              ret = true;
            }
            else
#endif // VST_NATIVE_SUPPORT
            {
              DEBUG_PLUGIN_SCAN(stderr, "loadPluginLib: unknown library:%s\n", filename);
            }
          }
        }
      }
       
      _end:
      
      // Close the library for now. It will be opened 
      //  again when an instance is created.
      if(handle)
        dlclose(handle);
      
//#ifdef VST_NATIVE_SUPPORT
//       sem_post(&_vstIdLock);
      //_vstIdLock.release();
//#endif // VST_NATIVE_SUPPORT
      
      return ret;
}

//---------------------------------------------------------
//   usage
//---------------------------------------------------------

void usage(const char* fname, const char* txt)
      {
      fprintf(stderr, "%s: %s\n", fname, txt);
//      fprintf(stderr, "usage:\n");
      }

//---------------------------------------------------------
//   main
//---------------------------------------------------------

int main(int argc, char* argv[])
      {
      //const char* type = 0;
      const char* filename = 0;
      int c;
//       while ((c = getopt(argc, argv, "f")) != EOF) {
      //while ((c = getopt(argc, argv, "t:f:")) != EOF) {
      while ((c = getopt(argc, argv, "f:")) != EOF) {
            switch (c) {
//                   case 'f': printName = true; break;
                  //case 't': type = optarg; break;
                  case 'f': filename = optarg; break;
//                   default:  usage(argv[0], "bad argument"); return -1;
                  //default:  usage(argv[0], "-t <type=ladspa, dssi, linuxvst> -f <filename>"); return -1;
                  default:  usage(argv[0], "-f <filename>"); return -1;
                  }
            }
//       argc -= optind;
//       ++argc;
//       const char* p = 0;
//       for (int i = 1; i < argc; ++i) {
//             switch (grepMidi(argv[i])) {
//                   case 0:     break;
//                   case -1:    p = "not found"; break;
//                   case -2:    p = "no 'MThd': not a midi file"; break;
//                   case -3:    p = "file too short"; break;
//                   case -4:    p = "bad file type"; break;
//                   case -5:    p = "no 'MTrk': not a midi file"; break;
//                   case -6:    p = "no running state"; break;
//                   default:
//                         printf("was??\n");
//                         return -1;
//                   }
//             }
//       if (p)
//             printf("Error: <%s>\n", p);

//       if (optind >= argc)
//       { 
//         fprintf(stderr, "Expected argument after options\n"); 
//         return -1;
//       }


//       if(!type)
//       {
//         fprintf(stderr, "Error: No type given\n");
//         return -1;
//       }
      
      if(!filename)
      {
        fprintf(stderr, "Error: No filename given\n");
        return -1;
      }
      
      const bool res = loadPluginLib(filename);
      
      if(!res)
      {
        //fprintf(stderr, "Error loading %s plugin: <%s>\n", type, filename);
        fprintf(stderr, "Error loading plugin: <%s>\n", filename);
        return -1;
      }

      return 0;
      }
