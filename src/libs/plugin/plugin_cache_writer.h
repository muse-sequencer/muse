//=========================================================
//  MusE
//  Linux Music Editor
//
//  plugin_cache_writer.h
//  (C) Copyright 2018 Tim E. Real (terminator356 on users dot sourceforge dot net)
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

#ifndef __PLUGIN_CACHE_WRITER_H__
#define __PLUGIN_CACHE_WRITER_H__

#include <QString>

#include "config.h"
#include "globaldefs.h"
#include "plugin_scan.h"
#include "plugin_list.h"

#include "synti/libsynti/mess.h"

#include <ladspa.h>

#ifdef DSSI_SUPPORT
#include <dssi.h>
#endif // DSSI_SUPPORT

#ifdef LV2_SUPPORT
#include "lilv/lilv.h"
#endif // LV2_SUPPORT


#ifdef VST_NATIVE_SUPPORT
#ifdef VST_SDK_QUIRK_DEF
#define __cdecl
#endif // VST_SDK_QUIRK_DEF

#include "aeffectx.h"

#ifdef VST_VESTIGE_SUPPORT
#ifndef effGetProgramNameIndexed
#define effGetProgramNameIndexed 29
#endif
#endif // VST_VESTIGE_SUPPORT

#ifdef VST_VESTIGE_SUPPORT
namespace MusEPlugin {
enum VstStringLengths
{
  VstNameLen          = VestigeMaxNameLen,
  VstLabelLen         = VestigeMaxLabelLen,
  VstShortLabelLen    = VestigeMaxShortLabelLen,
  VstCategoryLabelLen = VestigeMaxCategLabelLen,
  VstFileNameLen      = VestigeMaxFileNameLen
};
} // namespace MusEPlugin
#else // VST_VESTIGE_SUPPORT
namespace MusEPlugin {
enum VstStringLengths
{
  VstNameLen          = 64,
  VstLabelLen         = 64,
  VstShortLabelLen    = 8,
  VstCategoryLabelLen = 24,
  VstFileNameLen      = 100
};
} // namespace MusEPlugin
#endif // VST_VESTIGE_SUPPORT

#ifndef VST_2_4_EXTENSIONS
#ifndef VST_VESTIGE_SUPPORT
typedef long     VstInt32;
typedef long     VstIntPtr;
#else
typedef int32_t  VstInt32;
typedef intptr_t VstIntPtr;
#define VSTCALLBACK
#endif // VST_VESTIGE_SUPPORT
#endif // VST_2_4_EXTENSIONS

// A handy typedef consistent with that used for ladspa, dssi, mess etc.
typedef AEffect* (*LinuxVST_Instance_Function)(audioMasterCallback);

#endif // VST_NATIVE_SUPPORT


// Forward declarations:
namespace MusECore {
class Xml;
}

namespace MusEPlugin {

//-----------------------------------------
// functions
//-----------------------------------------

// TODO Depending on our needs later, make some of these private static in the cpp file.

bool scanLadspaPorts(const LADSPA_Descriptor* ladspa_descr, PluginScanInfoStruct* info, bool do_rdf);
bool scanLadspaDescriptor(const char* filename, const LADSPA_Descriptor* ladspa_descr,
                          PluginScanInfoStruct* info, bool do_ports, bool do_rdf);
bool writeLadspaInfo(const char* filename, LADSPA_Descriptor_Function ladspa, bool do_ports, int level, MusECore::Xml& xml);

bool scanMessDescriptor(const char* filename, const MESS* mess_descr, PluginScanInfoStruct* info);
bool writeMessInfo(const char* filename, MESS_Descriptor_Function mess, bool do_ports, int level, MusECore::Xml& xml);

#ifdef DSSI_SUPPORT
QString getDssiUiFilename(PluginScanInfoStruct* info);
bool scanDssiDescriptor(const char* filename, const DSSI_Descriptor* dssi_descr,
                        PluginScanInfoStruct* info, bool do_ports, bool do_rdf);
bool writeDssiInfo(const char* filename, DSSI_Descriptor_Function dssi, bool do_ports, int level, MusECore::Xml& xml);
#endif

#ifdef VST_NATIVE_SUPPORT
bool scanLinuxVstPorts(AEffect *plugin, PluginScanInfoStruct* info);
bool scanLinuxVstDescriptor(const char* filename, AEffect *plugin, long int id, PluginScanInfoStruct* info, bool do_ports);
bool writeLinuxVstInfo(const char* filename, LinuxVST_Instance_Function lvst, bool do_ports, int level, MusECore::Xml& xml);
#endif

#ifdef LV2_SUPPORT
void scanLv2Ports(const LilvPlugin *plugin, PluginScanInfoStruct* info, bool debugStdErr);
#endif

bool writeUnknownPluginInfo(const char* filename, int level, MusECore::Xml& xml);

void writePluginScanInfo(int level, MusECore::Xml& xml, const PluginScanInfoStruct& info, bool writePorts);

// The museGlobalLib is where to find the application's installed libraries.
void scanLadspaPlugins(const QString& museGlobalLib, PluginScanList* list, bool scanPorts, bool debugStdErr);
void scanMessPlugins(const QString& museGlobalLib, PluginScanList* list, bool scanPorts, bool debugStdErr);
void scanDssiPlugins(PluginScanList* list, bool scanPorts, bool debugStdErr);
void scanLinuxVSTPlugins(PluginScanList* list, bool scanPorts, bool debugStdErr);
void scanLv2Plugins(PluginScanList* list, bool scanPorts, bool debugStdErr);

void scanAllPlugins(const QString& museGlobalLib,
                    PluginScanList* list,
                    bool scanPorts,
                    bool debugStdErr,
                    PluginScanInfoStruct::PluginType_t types = PluginScanInfoStruct::PluginTypeAll);

//-----------------------------------------
// Public cache writer functions
//-----------------------------------------

// Create (or overwrite) a cache file.
bool createPluginCacheFile(
  // Path to the cache file directory (eg. config path + /scanner).
  const QString& path,
  // The type of plugin cache file to write.
  PluginScanInfoStruct::PluginType type,
  // List to read into and write from.
  PluginScanList* list,
  // Whether to write port information.
  bool writePorts,
  // Where to find the application's installed libraries.
  const QString& museGlobalLib = QString(),
  // The types of plugins to write into this one file.
  PluginScanInfoStruct::PluginType_t types = PluginScanInfoStruct::PluginTypeAll,
  // Print some stderr text
  bool debugStdErr = false
);

bool createPluginCacheFiles(
  // Path to the cache file directory (eg. config path + /scanner).
  const QString& path,
  // List to read into and write from.
  PluginScanList* list,
  // Whether to write port information.
  bool writePorts,
  // Where to find the application's installed libraries.
  const QString& museGlobalLib,
  // The types of plugin cache files to create.
  PluginScanInfoStruct::PluginType_t types = PluginScanInfoStruct::PluginTypeAll,
  // Print some stderr text
  bool debugStdErr = false
);

// Checks existence of given cache file types.
// Writes ALL the given types of cache files if ANY are not found.
// Returns true on success.
bool checkPluginCacheFiles(
  // Path to the cache file directory (eg. config path + /scanner).
  const QString& path,
  // List to read into and write from.
  PluginScanList* list,
  // Whether to write port information.
  bool writePorts,
  // Force it to create every time.
  bool alwaysRecreate = false,
  // Do not re-create.
  bool dontRecreate = false,
  // Where to find the application's installed libraries.
  const QString& museGlobalLib = QString(),
  // The types of plugin cache files to write.
  PluginScanInfoStruct::PluginType_t types = PluginScanInfoStruct::PluginTypeAll,
  // Print some stderr text
  bool debugStdErr = false
);

// Write the list of plugins to a plugin cache text file.
bool writePluginCacheFile(
  // Path to the cache file directory (eg. config path + /scanner).
  const QString& scanOutPath,
  // Cache file name.
  const QString& filename,
  // List to write.
  const PluginScanList& list,
  // Whether to write port information.
  bool writePorts,
  // The types of plugins to write.
  PluginScanInfoStruct::PluginType_t types = PluginScanInfoStruct::PluginTypeAll);


} // namespace MusEPlugin

#endif


