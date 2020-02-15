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

#include <QFile>

#include <cstdio>
#include <unistd.h>

#include <dlfcn.h>

#include "config.h"
#include "globaldefs.h"
#include "plugin_cache_writer.h"

#include <ladspa.h>
#include "synti/libsynti/mess.h"

#ifdef DSSI_SUPPORT
#include <dssi.h>
#endif


#ifdef VST_NATIVE_SUPPORT

#ifdef VST_SDK_QUIRK
#define __cdecl
#endif

#include "aeffectx.h"

//#include <semaphore.h>
//#include <QSemaphore>

#endif // VST_NATIVE_SUPPORT

// For debugging output: Uncomment the fprintf section.
#define DEBUG_PLUGIN_SCAN(dev, format, args...) // std::fprintf(dev, format, ##args);


// #include <semaphore.h>
// static sem_t _vstIdLock;
// #include <QSemaphore>
// static QSemaphore _vstIdLock;

namespace MusEPluginScan {

//---------------------------------------------------------
//   loadPluginLib
//    Returns true on success
//---------------------------------------------------------

static bool loadPluginLib(MusEPlugin::PluginScanInfoStruct::PluginType_t types,
                          const char* filename, const char* outfilename, bool do_ports)
{
  DEBUG_PLUGIN_SCAN(stderr, "loadPluginLib: filename:%s\n", filename);
  
//#ifdef VST_NATIVE_SUPPORT
  
// TODO: Copied from vst_native.cpp shell support code (below).
//       Since it would be executed all the time if VST_NATIVE_SUPPORT was enabled -
//        we don't yet know what type the lib is! - we'll just try this for all plugins...
//       Tested with QSemaphore (not sem_wait): This caused it to freeze. Conflicts with QProcess?
//       sem_wait(&_vstIdLock);
  //_vstIdLock.acquire();
//#endif // VST_NATIVE_SUPPORT

  bool found = false;

  // Open the output file...
  QFile outfile(outfilename);
  if(!outfile.exists())
  {
    std::fprintf(stderr, "muse_plugin_scan: the output file does not exist: %s for filename: %s\n", outfilename, filename);
    return false;
  }
  if(!outfile.open(QIODevice::WriteOnly /*| QIODevice::Text*/))
  {
    std::fprintf(stderr, "muse_plugin_scan: failed to open outfilename: %s for filename: %s\n", outfilename, filename);
    return false;
  }

  // Open the library...
  void* handle = dlopen(filename, RTLD_NOW);
  if (handle == 0)
  {
    std::fprintf(stderr, "muse_plugin_scan: dlopen(%s) failed: %s\n",
      filename, dlerror());
  }
  else
  {
    // Check if it's a DSSI plugin first...
#ifdef DSSI_SUPPORT
    if(!found)
    {
      DSSI_Descriptor_Function dssi = NULL;
      if(types & MusEPlugin::PluginScanInfoStruct::PluginTypeDSSI)
      {
        dssi = (DSSI_Descriptor_Function)dlsym(handle, "dssi_descriptor");
        if(dssi)
        {
          DEBUG_PLUGIN_SCAN(stderr, "loadPluginLib: Is a DSSI library\n");
          
          MusECore::Xml xml(&outfile);
          xml.header();
          int level = 0;
          level = xml.putFileVersion(level);
      
          MusEPlugin::writeDssiInfo(filename, dssi, do_ports, level, xml);

          xml.tag(1, "/muse");
          found = true;
        }
        else
        {
          DEBUG_PLUGIN_SCAN(stderr, "loadPluginLib: Not a DSSI library...\n");
        }
      }
    }

#endif

    // Check if it's a MESS plugin...
    if(!found)
    {
      MESS_Descriptor_Function msynth = NULL;
      if(types & MusEPlugin::PluginScanInfoStruct::PluginTypeMESS)
      {
        msynth = (MESS_Descriptor_Function)dlsym(handle, "mess_descriptor");
        if(msynth)
        {
          DEBUG_PLUGIN_SCAN(stderr, "loadPluginLib: Is a MESS library\n");
          MusECore::Xml xml(&outfile);
          xml.header();
          int level = 0;
          level = xml.putFileVersion(level);

          MusEPlugin::writeMessInfo(filename, msynth, do_ports, level, xml);

          xml.tag(1, "/muse");
          found = true;
        }
        else
        {
          DEBUG_PLUGIN_SCAN(stderr, "loadPluginLib: Not a MESS library...\n");
        }
      }
    }

    // Check if it's a LADSPA plugin...
    if(!found)
    {
      LADSPA_Descriptor_Function ladspa = NULL;
      if(types & MusEPlugin::PluginScanInfoStruct::PluginTypeLADSPA)
      {
        ladspa = (LADSPA_Descriptor_Function)dlsym(handle, "ladspa_descriptor");
        if(ladspa)
        {
          DEBUG_PLUGIN_SCAN(stderr, "loadPluginLib: Is a LADSPA library\n");
          MusECore::Xml xml(&outfile);
          xml.header();
          int level = 0;
          level = xml.putFileVersion(level);

          MusEPlugin::writeLadspaInfo(filename, ladspa, do_ports, level, xml);

          xml.tag(1, "/muse");
          found = true;
        }
        else
        {
          DEBUG_PLUGIN_SCAN(stderr, "loadPluginLib: Not a LADSPA library...\n");
        }
      }
    }

    // Check if it's a LinuxVST plugin...
#ifdef VST_NATIVE_SUPPORT
    if(!found)
    {
      LinuxVST_Instance_Function getInstance = NULL;
      if(types & MusEPlugin::PluginScanInfoStruct::PluginTypeLinuxVST)
      {
        getInstance = (LinuxVST_Instance_Function)dlsym(handle, MusEPlugin::VST_NEW_PLUGIN_ENTRY_POINT);
        if(getInstance)
        {
          DEBUG_PLUGIN_SCAN(stderr, "loadPluginLib: Is a LinuxVST library: New entrypoint found:%s\n",
                            MusEPlugin::VST_NEW_PLUGIN_ENTRY_POINT);
        }
        else  
        {
          getInstance = (LinuxVST_Instance_Function)dlsym(handle, MusEPlugin::VST_OLD_PLUGIN_ENTRY_POINT);
          
          if(getInstance)
          {
            DEBUG_PLUGIN_SCAN(stderr, "loadPluginLib: Is a LinuxVST library: Old entrypoint found:%s\n",
                              MusEPlugin::VST_OLD_PLUGIN_ENTRY_POINT);
          }
        }
        
        if(getInstance)
        {
//               sem_wait(&_vstIdLock);
//               _vstIdLock.acquire();
//               currentPluginId = 0;
//               bool bDontDlCLose = false;

          MusECore::Xml xml(&outfile);
          xml.header();
          int level = 0;
          level = xml.putFileVersion(level);

          bool success = MusEPlugin::writeLinuxVstInfo(filename, getInstance, do_ports, level, xml);

          xml.tag(1, "/muse");

          if (success) {
            found = true;
          } else {
            std::fprintf(stderr, "muse_plugin_scan: writeLinuxVstInfo(%s) failed\n", filename);
            found = false;
            outfile.seek(0);
          }
        }
        else
        {
          DEBUG_PLUGIN_SCAN(stderr, "loadPluginLib: Not a LinuxVST library: Entrypoints \"%s\" or \"%s\" not found...\n",
                            MusEPlugin::VST_NEW_PLUGIN_ENTRY_POINT, MusEPlugin::VST_OLD_PLUGIN_ENTRY_POINT);
        }
      }
    }
#endif // VST_NATIVE_SUPPORT
      
  }
  
  if(!found)
  {
    MusECore::Xml xml(&outfile);
    xml.header();
    int level = 0;
    level = xml.putFileVersion(level);

    MusEPlugin::writeUnknownPluginInfo(filename, level, xml);

    xml.tag(1, "/muse");
    found = true;
  }

  //_end:

  // Flush and close the output file.
  outfile.close();
    
  // Close the library for now. It will be opened 
  //  again when an instance is created.
  if(handle)
    dlclose(handle);
  
//#ifdef VST_NATIVE_SUPPORT
//       sem_post(&_vstIdLock);
  //_vstIdLock.release();
//#endif // VST_NATIVE_SUPPORT
  
  
  return found;
}

} // namespace MusEPluginScan


//---------------------------------------------------------
//   main
//---------------------------------------------------------

int main(int argc, char* argv[])
      {
      bool do_ports = false;
      const char* filename = 0;
      const char* outfilename = 0;
      MusEPlugin::PluginScanInfoStruct::PluginType_t types = MusEPlugin::PluginScanInfoStruct::PluginTypeAll;
      int c;
      while ((c = getopt(argc, argv, "f:t:o:p")) != EOF) {
            switch (c) {
                  case 'f': filename = optarg; break;
                  case 'o': outfilename = optarg; break;
                  case 't': types = MusEPlugin::PluginScanInfoStruct::PluginType_t(atoi(optarg)); break;
                  case 'p': do_ports = true; break;
                  default:  std::fprintf(stderr, "%s: -t <types flags> -f <filename> -o <output filename> -p (scan plugin ports)\n",
                              argv[0]);  return -1;
                  }
            }

      if(!filename || filename[0] == 0)
      {
        std::fprintf(stderr, "Error: No filename given\n");
        return -1;
      }
      
      if(!outfilename || outfilename[0] == 0)
      {
        std::fprintf(stderr, "Error: No output filename given\n");
        return -1;
      }
   
      const bool res = MusEPluginScan::loadPluginLib(types, filename, outfilename, do_ports);
      
      if(!res)
      {
        std::fprintf(stderr, "Error loading plugin: <%s>\n", filename);
        return 1;
      }

      return 0;
      }
