//=========================================================
//  MusE
//  Linux Music Editor
//
//  plugin_cache_reader.h
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

#ifndef __PLUGIN_CACHE_READER_H__
#define __PLUGIN_CACHE_READER_H__

#include <QString>
#include <QStringList>

#include "config.h"
#include "xml.h"
#include "plugin_scan.h"
#include "plugin_list.h"

namespace MusECore {
class Xml;
}

namespace MusEPlugin {

//-----------------------------------------
// Public plugin cache functions
//-----------------------------------------

// TODO Depending on our needs later, make some of these private static in the cpp file.

// The museGlobalLib is where to find the application's installed libraries.
QStringList pluginGetLadspaDirectories(const QString& museGlobalLib);
QStringList pluginGetMessDirectories(const QString& museGlobalLib);
QStringList pluginGetDssiDirectories();
QStringList pluginGetLinuxVstDirectories();
QStringList pluginGetLv2Directories();
QStringList pluginGetVstDirectories();
QStringList pluginGetDirectories(const QString& museGlobalLib,
                                 PluginScanInfoStruct::PluginType type = PluginScanInfoStruct::PluginTypeNone);

// Returns the name of a cache file, without path.
const char* pluginCacheFilename(
  // The type of plugin cache to check.
  PluginScanInfoStruct::PluginType type = PluginScanInfoStruct::PluginTypeNone
);

// Returns the given type parameter if the cache file for the given type exists.
PluginScanInfoStruct::PluginType pluginCacheFileExists(
  // Path to the cache file directory (eg. config path + /scanner).
  const QString& path,
  // The type of plugin cache to check.
  PluginScanInfoStruct::PluginType type = PluginScanInfoStruct::PluginTypeNone
);

// Returns plugin type true if the cache file(s) for the given type(s) exist.
PluginScanInfoStruct::PluginType_t pluginCacheFilesExist(
  // Path to the cache file directory (eg. config path + /scanner).
  const QString& path,
  // The types of plugin caches to check.
  PluginScanInfoStruct::PluginType_t types = PluginScanInfoStruct::PluginTypeAll
);

// Returns true if any of the directories or subdirectories of the given plugin cache type
//  have modification date/times GREATER than the existing plugin cache file of the given type.
bool pluginCacheIsDirty(
  // Path to the cache file directory (eg. config path + /scanner).
  const QString& path,
  // Where to find the application's installed libraries.
  const QString& museGlobalLib,
  // The type of plugin cache to check.
  PluginScanInfoStruct::PluginType type,
  // Print some stderr text
  bool debugStdErr = false
);

// Returns plugin type true if any of the directories or subdirectories of the given plugin cache types
//  have modification date/times GREATER than the existing plugin cache file of the given types.
PluginScanInfoStruct::PluginType_t pluginCachesAreDirty(
  // Path to the cache file directory (eg. config path + /scanner).
  const QString& path,
  // Where to find the application's installed libraries.
  const QString& museGlobalLib,
  // The types of plugin caches to check.
  PluginScanInfoStruct::PluginType_t types = PluginScanInfoStruct::PluginTypeAll,
  // Print some stderr text
  bool debugStdErr = false
);

// Return true on error
bool readPluginScanInfoPortEnum(MusECore::Xml& xml, PluginPortInfo* port_info);
// Return true on error
bool readPluginScanInfoPort(MusECore::Xml& xml, PluginScanInfoStruct* info);
// Return true on error
bool readPluginScanInfo(MusECore::Xml& xml, PluginScanInfoStruct* info, bool readPorts = false, bool readEnums = false);
// Return true on error
bool readPluginScan(MusECore::Xml& xml, PluginScanList* list, bool readPorts = false, bool readEnums = false);

// Read the plugin cache text file to a plugin list.
bool readPluginCacheFile(
  // Path to the cache file directory (eg. config path + /scanner).
  const QString& path,
  // List to read into.
  PluginScanList* list,
  // Whether to read port information.
  bool readPorts = false,
  // Whether to read port value enumeration information.
  bool readEnums = false,
  // The type of plugin cache file to read.
  PluginScanInfoStruct::PluginType type = PluginScanInfoStruct::PluginTypeNone
);

// Read all plugin cache text files to a plugin list.
bool readPluginCacheFiles(
  // Path to the cache file directory (eg. config path + /scanner).
  const QString& path,
  // List to read into.
  PluginScanList* list,
  // Whether to read port information.
  bool readPorts = false,
  // Whether to read port value enumeration information.
  bool readEnums = false,
  // The types of plugin cache files to read.
  PluginScanInfoStruct::PluginType_t types = PluginScanInfoStruct::PluginTypeAll);


} // namespace MusEPlugin

#endif



