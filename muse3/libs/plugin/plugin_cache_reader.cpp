//=========================================================
//  MusE
//  Linux Music Editor
//
//  plugin_cache_reader.cpp
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

#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QDateTime>

// For sorting port enum values.
#include <map>
  
#include <cstring>
#include <cstdlib>

#include "plugin_cache_reader.h"

// For debugging output: Uncomment the fprintf section.
#define DEBUG_PLUGIN_SCAN(dev, format, args...)  // std::fprintf(dev, format, ##args);

//#define PORTS_ARE_SINGLE_LINE_TAGS 1

namespace MusEPlugin {

static void setPluginScanFileInfo(const QString& filename, PluginScanInfoStruct* info)
{
  if(filename.isEmpty())
    return;
  const QFileInfo fi(filename);
  info->_completeBaseName = PLUGIN_SET_QSTRING(fi.completeBaseName());
  info->_baseName         = PLUGIN_SET_QSTRING(fi.baseName());
  info->_suffix           = PLUGIN_SET_QSTRING(fi.suffix());
  info->_completeSuffix   = PLUGIN_SET_QSTRING(fi.completeSuffix());
  info->_absolutePath     = PLUGIN_SET_QSTRING(fi.absolutePath());
  info->_path             = PLUGIN_SET_QSTRING(fi.path());
}

//---------------------------------------------------------
//   readPluginScanInfoPortEnum
//    Returns true on error
//---------------------------------------------------------

bool readPluginScanInfoPortEnum(MusECore::Xml& xml, PluginPortEnumValue* enum_val)
      {
      for (;;) {
            MusECore::Xml::Token token = xml.parse();
            const QString& tag = xml.s1();

            switch (token) {
                  case MusECore::Xml::Error:
                  case MusECore::Xml::End:
                        return true;
                  case MusECore::Xml::TagStart:
                          xml.unknown("readPort");
                        break;
                  case MusECore::Xml::Attribut:
                        // As attributes on one line...
                        if (tag == "label")
                          enum_val->_label = PLUGIN_SET_QSTRING(xml.s2());
                        else if (tag == "val")
                          enum_val->_value = xml.s2().toFloat();
                        
                        break;
                  case MusECore::Xml::TagEnd:
                        if (tag == "enumVal") {
                          return false;
                          }
                        return true;
                  default:
                        break;
                  }
            }
      return true;
      }

//---------------------------------------------------------
//   readPluginPortEnumValMap
//    Returns true on error
//---------------------------------------------------------

bool readPluginPortEnumValMap(MusECore::Xml& xml, PortEnumValueMap* val_map)
      {
      unsigned int port_idx = 0;
      // Map for ensuring the enumerations are sorted by increasing value.
      std::map<float, PluginPortEnumValue, std::less<float> > sort_map;
      
      for (;;) {
            MusECore::Xml::Token token = xml.parse();
            const QString& tag = xml.s1();

            switch (token) {
                  case MusECore::Xml::Error:
                  case MusECore::Xml::End:
                        return true;
                  case MusECore::Xml::TagStart:
                          if (tag == "enumVal")
                          {
                            PluginPortEnumValue enum_val;
                            if(!readPluginScanInfoPortEnum(xml, &enum_val))
                              sort_map.insert(std::pair<float, PluginPortEnumValue>(enum_val._value, enum_val));
                          }
                          else
                          
                          xml.unknown("readPort");
                        break;
                  case MusECore::Xml::Attribut:
                        // As attributes on one line...
                        if (tag == "idx")
                          port_idx = xml.s2().toUInt();
                        
                        break;
                  case MusECore::Xml::TagEnd:
                        if (tag == "portEnumValMap")
                        {
                          // Copy the sorted enumeration values to a new vector enumeration list.
                          EnumValueList val_list;
                          for(std::map<float, PluginPortEnumValue, std::less<float>>::const_iterator iel = sort_map.begin();
                              iel != sort_map.end(); ++iel)
                          {
                            val_list.push_back(iel->second);
                          }

                          // Insert the new value list into the map.
                          // Ther can be only one value list per port index.
                          if(!val_list.empty())
                            val_map->insert(PortEnumValueMapPair(port_idx, val_list));

                          return false;
                        }
                        return true;
                  default:
                        break;
                  }
            }
      return true;
      }

//---------------------------------------------------------
//   readPluginScanInfoPort
//    Returns true on error
//---------------------------------------------------------

bool readPluginScanInfoPort(MusECore::Xml& xml, PluginScanInfoStruct* info)
      {
      PluginPortInfo port_info;
      for (;;) {
            MusECore::Xml::Token token = xml.parse();
            const QString& tag = xml.s1();

            switch (token) {
                  case MusECore::Xml::Error:
                  case MusECore::Xml::End:
                        return true;
                  case MusECore::Xml::TagStart:
                    
#ifndef PORTS_ARE_SINGLE_LINE_TAGS
                        // As a tag with attributes...
                        if (tag == "flags")
                          port_info._flags = PluginPortInfo::PortFlags_t(xml.parseInt());
                        else if (tag == "valFlags")
                          port_info._valueFlags = PluginPortInfo::PortValueFlags_t(xml.parseInt());
                        else if (tag == "min")
                          port_info._min = xml.parseFloat();
                        else if (tag == "max")
                          port_info._max = xml.parseFloat();
                        else if (tag == "def")
                          port_info._defaultVal = xml.parseFloat();
                        else if (tag == "step")
                          port_info._step = xml.parseFloat();
                        else if (tag == "smallStep")
                          port_info._smallStep = xml.parseFloat();
                        else if (tag == "largeStep")
                          port_info._largeStep = xml.parseFloat();
                        else
#endif
                    
                          xml.unknown("readPort");
                        break;
                  case MusECore::Xml::Attribut:
                        // As attributes on one line...
                        if (tag == "name")
                              port_info._name = PLUGIN_SET_QSTRING(xml.s2());
                        else if (tag == "idx")
                              port_info._index = xml.s2().toULong();
                        else if (tag == "type")
                              port_info._type = PluginPortInfo::PortType_t(xml.s2().toInt());
                        else if (tag == "symbol")
                              port_info._symbol = PLUGIN_SET_QSTRING(xml.s2());
                        
#ifdef PORTS_ARE_SINGLE_LINE_TAGS
                        else if (tag == "flags")
                          port_info._flags = PluginPortInfo::PortFlags_t(xml.s2().toInt());
                        else if (tag == "valFlags")
                          port_info._valueFlags = PluginPortInfo::PortValueFlags_t(xml.s2().toInt());
                        else if (tag == "min")
                          port_info._min = xml.s2().toFloat();
                        else if (tag == "max")
                          port_info._max = xml.s2().toFloat();
                        else if (tag == "def")
                          port_info._defaultVal = xml.s2().toFloat();
                        else if (tag == "step")
                          port_info._step = xml.s2().toFloat();
                        else if (tag == "smallStep")
                          port_info._smallStep = xml.s2().toFloat();
                        else if (tag == "largeStep")
                          port_info._largeStep = xml.s2().toFloat();
#endif
                        
                        break;
                  case MusECore::Xml::TagEnd:
                        if (tag == "port")
                        {
                          info->_portList.push_back(port_info);
                          return false;
                        }
                        return true;
                  default:
                        break;
                  }
            }
      return true;
      }

//---------------------------------------------------------
//   readPluginScanInfo
//    return true on error
//---------------------------------------------------------

bool readPluginScanInfo(MusECore::Xml& xml, PluginScanInfoStruct* info, bool readPorts, bool readEnums)
      {
      for (;;) {
            MusECore::Xml::Token token(xml.parse());
            const QString& tag(xml.s1());
            switch (token) {
                  case MusECore::Xml::Error:
                  case MusECore::Xml::End:
                        return true;
                  case MusECore::Xml::TagStart:
                        if (tag == "uri")
                              info->_uri = PLUGIN_SET_QSTRING(xml.parse1());
                        else if (tag == "type")
                              info->_type = PluginScanInfoStruct::PluginType(xml.parseInt());
                        else if (tag == "class")
                              info->_class = PluginScanInfoStruct::PluginClass(xml.parseInt());
                        else if (tag == "uniqueID")
                              info->_uniqueID = xml.parseUInt();
                        else if (tag == "subID")
                              info->_subID = xml.parseInt();
                        else if (tag == "name")
                              info->_name = PLUGIN_SET_QSTRING(xml.parse1());
                        else if (tag == "description")
                              info->_description = PLUGIN_SET_QSTRING(xml.parse1());
                        else if (tag == "version")
                              info->_version = PLUGIN_SET_QSTRING(xml.parse1());
                        else if (tag == "maker")
                              info->_maker = PLUGIN_SET_QSTRING(xml.parse1());
                        else if (tag == "copyright")
                              info->_copyright = PLUGIN_SET_QSTRING(xml.parse1());
                        else if (tag == "apiVersionMajor")
                              info->_apiVersionMajor = xml.parseInt();
                        else if (tag == "apiVersionMinor")
                              info->_apiVersionMinor = xml.parseInt();
                        else if (tag == "pluginVersionMajor")
                              info->_pluginVersionMajor = xml.parseInt();
                        else if (tag == "pluginVersionMinor")
                              info->_pluginVersionMinor = xml.parseInt();
                        else if (tag == "pluginFlags")
                              info->_pluginFlags = xml.parseInt();
                        else if (tag == "portCount")
                              info->_portCount = xml.parseUInt();
                        else if (tag == "inports")
                              info->_inports = xml.parseUInt();
                        else if (tag == "outports")
                              info->_outports = xml.parseUInt();
                        else if (tag == "ctlInports")
                              info->_controlInPorts = xml.parseUInt();
                        else if (tag == "ctlOutports")
                              info->_controlOutPorts = xml.parseUInt();
                        else if (tag == "evInports")
                              info->_eventInPorts = xml.parseUInt();
                        else if (tag == "evOutports")
                              info->_eventOutPorts = xml.parseUInt();
                        else if (tag == "freewheelPortIdx")
                              info->_freewheelPortIdx = xml.parseUInt();
                        else if (tag == "latencyPortIdx")
                              info->_latencyPortIdx = xml.parseUInt();
                        else if (tag == "requiredFeatures")
                              info->_requiredFeatures = xml.parseInt();
                        else if (tag == "vstPluginFlags")
                              info->_vstPluginFlags = xml.parseInt();
                        else if (tag == "uiFilename")
                              info->_uiFilename = PLUGIN_SET_QSTRING(xml.parse1());
                        else if (tag == "port")
                        {
                          if(readPorts)
                              readPluginScanInfoPort(xml, info);
                        }
                        else if (tag == "portEnumValMap")
                        {
                          if(readEnums)
                              readPluginPortEnumValMap(xml, &info->_portEnumValMap);
                        }
                        else
                              xml.unknown("PluginScanInfo");
                        break;
                  case MusECore::Xml::Attribut:
                        if (tag == "file") {
                                setPluginScanFileInfo(xml.s2(), info);
                              }
                        else if (tag == "label") {
                                info->_label = PLUGIN_SET_QSTRING(xml.s2());
                              }
                        break;
                  case MusECore::Xml::TagEnd:
                        if (tag == "plugin") {
                              return false;
                              }
                        return true;
                  default:
                        break;
                  }
            }
      return true;
      }

//---------------------------------------------------------
//   readPluginScan
//    return true on error
//---------------------------------------------------------

bool readPluginScan(MusECore::Xml& xml, PluginScanList* list, bool readPorts, bool readEnums)
      {
      for (;;) {
            MusECore::Xml::Token token(xml.parse());
            const QString& tag(xml.s1());
            switch (token) {
                  case MusECore::Xml::Error:
                  case MusECore::Xml::End:
                        return true;
                  case MusECore::Xml::TagStart:
                        if (tag == "muse")
                              break;
                        else if (tag == "plugin")
                        {
                              PluginScanInfoStruct info;
                              // read returns true on error.
                              if(!readPluginScanInfo(xml, &info, readPorts, readEnums))
                              {
                                // Make sure it doesn't already exist.
                                // Here we have a chance to include all plugins.
                                // There is nothing stopping us from doing that,
                                //  but for now let's just skip duplicates.
                                const MusEPlugin::PluginScanInfoRef found_inforef = list->find(info);
                                if(found_inforef)
                                {
                                  const MusEPlugin::PluginScanInfoStruct& found_infos = found_inforef->info();
                                  fprintf(stderr, "Ignoring plugin label:%s\n  path:%s duplicate of\n  path:%s\n",
                                          PLUGIN_GET_CSTRING(info._label),
                                          PLUGIN_GET_CSTRING(info.filePath()),
                                          PLUGIN_GET_CSTRING(found_infos.filePath())
                                        );
                                }
                                else
                                {
                                  list->add(new PluginScanInfo(info));
                                }
                              }
                              break;
                        }
                        else
                              xml.unknown("readPluginScan");
                        break;
                  case MusECore::Xml::Attribut:
                        if (tag == "version") {
                              int major = xml.s2().section('.', 0, 0).toInt();
                              int minor = xml.s2().section('.', 1, 1).toInt();
                              xml.setVersion(major, minor);
                              }
                        break;
                  case MusECore::Xml::TagEnd:
                        if (tag == "muse")
                        {
                              return false;
                        }
                        return true;
                  default:
                        break;
                  }
            }
      return true;
      }

//---------------------------------------------------------
//   pluginGetLadspaDirectories
//---------------------------------------------------------

QStringList pluginGetLadspaDirectories(const QString& museGlobalLib)
{
  QStringList sl;
  // Add our own LADSPA plugin directory...
  sl.append(museGlobalLib + QString("/plugins"));
  // Now add other directories...
  QString ladspaPath = std::getenv("LADSPA_PATH");
  if(ladspaPath.isEmpty())
  {
    QString homePath = std::getenv("HOME");
    if(!homePath.isEmpty())
      homePath += QString("/ladspa:");
    ladspaPath = homePath + QString("/usr/local/lib64/ladspa:/usr/lib64/ladspa:/usr/local/lib/ladspa:/usr/lib/ladspa");
  }
  if(!ladspaPath.isEmpty())
    sl.append(ladspaPath.split(":", QString::SkipEmptyParts, Qt::CaseSensitive));
  return sl;
}

//---------------------------------------------------------
//   pluginGetMessDirectories
//---------------------------------------------------------

QStringList pluginGetMessDirectories(const QString& museGlobalLib)
{
  QStringList sl;
  // Add our own MESS plugin directory...
  sl.append(museGlobalLib + QString("/synthi"));
  // Now add other directories...
  QString messPath = std::getenv("MESS_PATH");
  if(messPath.isEmpty())
  {
    QString homePath = std::getenv("HOME");
    if(!homePath.isEmpty())
      homePath += QString("/MESS:");
    messPath = homePath + QString("/usr/local/lib64/MESS:/usr/lib64/MESS:/usr/local/lib/MESS:/usr/lib/MESS");
  }
  if(!messPath.isEmpty())
    sl.append(messPath.split(":", QString::SkipEmptyParts, Qt::CaseSensitive));
  return sl;
}

//---------------------------------------------------------
//   pluginGetDssiDirectories
//---------------------------------------------------------

QStringList pluginGetDssiDirectories()
{
  QStringList sl;
  QString dssiPath = std::getenv("DSSI_PATH");
  if(dssiPath.isEmpty())
  {
    QString homePath = std::getenv("HOME");
    if(!homePath.isEmpty())
      homePath += QString("/dssi:");
    dssiPath = homePath + QString("/usr/local/lib64/dssi:/usr/lib64/dssi:/usr/local/lib/dssi:/usr/lib/dssi");
  }
  if(!dssiPath.isEmpty())
    sl.append(dssiPath.split(":", QString::SkipEmptyParts, Qt::CaseSensitive));
  return sl;
}

//---------------------------------------------------------
//   pluginGetLinuxVstDirectories
//---------------------------------------------------------

QStringList pluginGetLinuxVstDirectories()
{
  QStringList sl;
  QString vstPath = std::getenv("LINUX_VST_PATH");
  if(vstPath.isEmpty())
  {
    vstPath = std::getenv("VST_PATH");
    {
      if(vstPath.isEmpty())
      {
        QString homePath = std::getenv("HOME");
        if(!homePath.isEmpty())
          homePath += QString("/.vst:") + homePath + QString("/vst:");
        vstPath = homePath + QString("/usr/local/lib64/vst:/usr/local/lib/vst:/usr/lib64/vst:/usr/lib/vst");
      }
    }
  }
  if(!vstPath.isEmpty())
    sl.append(vstPath.split(":", QString::SkipEmptyParts, Qt::CaseSensitive));
  return sl;
}

//---------------------------------------------------------
//   pluginGetVstDirectories
//---------------------------------------------------------

QStringList pluginGetVstDirectories()
{
  QStringList sl;
  QString vstPath = std::getenv("VST_PATH");
  if(vstPath.isEmpty())
  {
    QString homePath = std::getenv("HOME");
    if(!homePath.isEmpty())
      homePath += QString("/.vst:") + homePath + QString("/vst:");
    vstPath = homePath + QString("/usr/local/lib64/vst:/usr/local/lib/vst:/usr/lib64/vst:/usr/lib/vst");
  }
  if(!vstPath.isEmpty())
    sl.append(vstPath.split(":", QString::SkipEmptyParts, Qt::CaseSensitive));
  return sl;
}

//---------------------------------------------------------
//   pluginGetLv2Directories
//---------------------------------------------------------

QStringList pluginGetLv2Directories()
{
  QStringList sl;
  QString lv2Path = std::getenv("LV2_PATH");
  if(lv2Path.isEmpty())
  {
    QString homePath = std::getenv("HOME");
    if(!homePath.isEmpty())
      homePath += QString("/.lv2:");
    lv2Path = homePath + QString("/usr/local/lib64/lv2:/usr/lib64/lv2:/usr/local/lib/lv2:/usr/lib/lv2");
  }
  if(!lv2Path.isEmpty())
    sl.append(lv2Path.split(":", QString::SkipEmptyParts, Qt::CaseSensitive));
  return sl;
}

//---------------------------------------------------------
//   pluginGetDirectories
//---------------------------------------------------------

QStringList pluginGetDirectories(const QString& museGlobalLib, PluginScanInfoStruct::PluginType type)
{
  switch(type)
  {
    case PluginScanInfoStruct::PluginTypeDSSI:
    case PluginScanInfoStruct::PluginTypeDSSIVST:
      return pluginGetDssiDirectories();
    break;

    case PluginScanInfoStruct::PluginTypeMESS:
      return pluginGetMessDirectories(museGlobalLib);
    break;

    case PluginScanInfoStruct::PluginTypeLADSPA:
      return pluginGetLadspaDirectories(museGlobalLib);
    break;

    case PluginScanInfoStruct::PluginTypeLinuxVST:
      return pluginGetLinuxVstDirectories();
    break;

    case PluginScanInfoStruct::PluginTypeLV2:
      return pluginGetLv2Directories();
    break;

    case PluginScanInfoStruct::PluginTypeVST:
      return pluginGetVstDirectories();
    break;

    case PluginScanInfoStruct::PluginTypeNone:
    case PluginScanInfoStruct::PluginTypeAll:
    break;
  }
  
  return QStringList();
}

//---------------------------------------------------------
//   pluginCacheFileExists
// Returns true if the cache file(s) for the given type(s) exist.
//---------------------------------------------------------

const char* pluginCacheFilename(PluginScanInfoStruct::PluginType type)
{
  switch(type)
  {
    case PluginScanInfoStruct::PluginTypeDSSI:
    case PluginScanInfoStruct::PluginTypeDSSIVST:
      return "dssi_plugins.scan";
    break;

    case PluginScanInfoStruct::PluginTypeMESS:
      return "mess_plugins.scan";
    break;

    case PluginScanInfoStruct::PluginTypeLADSPA:
      return "ladspa_plugins.scan";
    break;

    case PluginScanInfoStruct::PluginTypeLinuxVST:
      return "linux_vst_plugins.scan";
    break;

    case PluginScanInfoStruct::PluginTypeLV2:
      return "lv2_plugins.scan";
    break;

    case PluginScanInfoStruct::PluginTypeVST:
      return "vst_plugins.scan";
    break;
    
    case PluginScanInfoStruct::PluginTypeNone:
    case PluginScanInfoStruct::PluginTypeAll:
      return "";
    break;
  }
  
  return "";
}

//---------------------------------------------------------
//   pluginCacheFileExists
//---------------------------------------------------------

PluginScanInfoStruct::PluginType pluginCacheFileExists(
  const QString& path,
  PluginScanInfoStruct::PluginType type)
{
  const QFile targ_qfile(path + '/' + QString(pluginCacheFilename(type)));
  if(targ_qfile.exists())
    return type;
  return PluginScanInfoStruct::PluginTypeNone;
}

//---------------------------------------------------------
//   pluginCacheFilesExist
//---------------------------------------------------------

PluginScanInfoStruct::PluginType_t pluginCacheFilesExist(
  const QString& path,
  PluginScanInfoStruct::PluginType_t types)
{
  PluginScanInfoStruct::PluginType_t res = PluginScanInfoStruct::PluginTypeNone;
  
  if(types & (PluginScanInfoStruct::PluginTypeDSSI | PluginScanInfoStruct::PluginTypeDSSIVST))
  {
    if(pluginCacheFileExists(path, PluginScanInfoStruct::PluginTypeDSSI) == PluginScanInfoStruct::PluginTypeDSSI)
      res |= (PluginScanInfoStruct::PluginTypeDSSI | PluginScanInfoStruct::PluginTypeDSSIVST);
  }

  if(types & PluginScanInfoStruct::PluginTypeMESS)
    res |= pluginCacheFileExists(path, PluginScanInfoStruct::PluginTypeMESS);

  if(types & PluginScanInfoStruct::PluginTypeLADSPA)
    res |= pluginCacheFileExists(path, PluginScanInfoStruct::PluginTypeLADSPA);

  if(types & PluginScanInfoStruct::PluginTypeLinuxVST)
    res |= pluginCacheFileExists(path, PluginScanInfoStruct::PluginTypeLinuxVST);

  if(types & PluginScanInfoStruct::PluginTypeLV2)
    res |= pluginCacheFileExists(path, PluginScanInfoStruct::PluginTypeLV2);

  if(types & PluginScanInfoStruct::PluginTypeVST)
    res |= pluginCacheFileExists(path, PluginScanInfoStruct::PluginTypeVST);

  return res;
}

//---------------------------------------------------------
//   pluginGetCacheModified
//---------------------------------------------------------

static QDateTime pluginGetCacheModified(
  // Path to the cache files.
  const QString& path,
  PluginScanInfoStruct::PluginType type)
{
  const QString fn = pluginCacheFilename(type);
  if(fn.isEmpty())
    return QDateTime();
  const QFileInfo fi(path + "/" + fn);
  if(!fi.exists())
    return QDateTime();
  return fi.lastModified();
}

//---------------------------------------------------------
//   pluginGetDirectoryModified
//---------------------------------------------------------

static QDateTime pluginGetDirectoryModified(const QString& dirname, QDateTime date = QDateTime(), int recurseLevel = 0)
{
  const int max_levels = 10;
  if(recurseLevel >= max_levels)
  {
    std::fprintf(stderr, "pluginGetDirectoryModified: Ignoring too-deep directory level (max:%d) at:%s\n",
                 max_levels, dirname.toLocal8Bit().constData());
    return date;
  }

  DEBUG_PLUGIN_SCAN(stderr, "pluginGetDirectoryModified: <%s>\n", dirname.toLatin1().constData());

  QDir pluginDir(
    dirname,
    QString(),
    QDir::Name | QDir::IgnoreCase,
    QDir::Drives | QDir::AllDirs | QDir::NoDotAndDotDot);

  if(pluginDir.exists())
  {
    const QFileInfo dir_fi(dirname);
    const QDateTime dir_time = dir_fi.lastModified();
    
    if(!date.isValid() || dir_time > date)
      date = dir_time;
    
    QFileInfoList fi_list = pluginDir.entryInfoList();
    QFileInfoList::iterator it=fi_list.begin();
    while(it != fi_list.end())
    {
      const QFileInfo& fi = *it;
      if(fi.isDir())
      {
        // RECURSIVE!
        const QDateTime dt = pluginGetDirectoryModified(fi.filePath(), date, recurseLevel + 1);
        if(dt.isValid() && dt > date)
          date = dt;
      }
      
      ++it;
    }
  }

  return date;
}

//---------------------------------------------------------
//   pluginCacheIsDirty
//---------------------------------------------------------

bool pluginCacheIsDirty(
  const QString& path,
  const QString& museGlobalLib,
  PluginScanInfoStruct::PluginType type,
  bool debugStdErr)
{
  const QStringList sl = pluginGetDirectories(museGlobalLib, type);
  if(sl.isEmpty())
    return false;
  const QDateTime cm = pluginGetCacheModified(path, type);
  if(!cm.isValid())
    return true;
  const int sl_sz = sl.size();
  for(int i = 0; i < sl_sz; ++i)
  {
    const QString dn = sl.at(i);
    const QDateTime dm = pluginGetDirectoryModified(dn);
    if(!dm.isValid())
      continue;
    if(dm > cm)
    {
      if(debugStdErr)
        std::fprintf(stderr, "Plugin type: <%s> directory is dirty: <%s>\n",
                     pluginCacheFilename(type), dn.toLatin1().constData());
      return true;
    }
  }
  return false;
}

//---------------------------------------------------------
//   pluginCachesAreDirty
//---------------------------------------------------------

PluginScanInfoStruct::PluginType_t pluginCachesAreDirty(
  const QString& path, 
  const QString& museGlobalLib,
  PluginScanInfoStruct::PluginType_t types,
  bool debugStdErr
)
{
  PluginScanInfoStruct::PluginType_t res = PluginScanInfoStruct::PluginTypeNone;
  
  if((types & (PluginScanInfoStruct::PluginTypeDSSI | PluginScanInfoStruct::PluginTypeDSSIVST)))
  {
    if(pluginCacheIsDirty(path, museGlobalLib, PluginScanInfoStruct::PluginTypeDSSI, debugStdErr))
      res |= (PluginScanInfoStruct::PluginTypeDSSI | PluginScanInfoStruct::PluginTypeDSSIVST);
  }

  if((types & PluginScanInfoStruct::PluginTypeMESS) &&
     pluginCacheIsDirty(path, museGlobalLib, PluginScanInfoStruct::PluginTypeMESS, debugStdErr))
    res |= PluginScanInfoStruct::PluginTypeMESS;

  if((types & PluginScanInfoStruct::PluginTypeLADSPA) &&
     pluginCacheIsDirty(path, museGlobalLib, PluginScanInfoStruct::PluginTypeLADSPA, debugStdErr))
    res |= PluginScanInfoStruct::PluginTypeLADSPA;

  if((types & PluginScanInfoStruct::PluginTypeLinuxVST) &&
     pluginCacheIsDirty(path, museGlobalLib, PluginScanInfoStruct::PluginTypeLinuxVST, debugStdErr))
    res |= PluginScanInfoStruct::PluginTypeLinuxVST;

  if((types & PluginScanInfoStruct::PluginTypeLV2) &&
     pluginCacheIsDirty(path, museGlobalLib, PluginScanInfoStruct::PluginTypeLV2, debugStdErr))
    res |= PluginScanInfoStruct::PluginTypeLV2;

  if((types & PluginScanInfoStruct::PluginTypeVST) &&
     pluginCacheIsDirty(path, museGlobalLib, PluginScanInfoStruct::PluginTypeVST, debugStdErr))
    res |= PluginScanInfoStruct::PluginTypeVST;

  return res;
}

//---------------------------------------------------------
//   readPluginCacheFile
//---------------------------------------------------------

bool readPluginCacheFile(
  const QString& path,
  PluginScanList* list,
  bool readPorts,
  bool readEnums,
  PluginScanInfoStruct::PluginType type
)
{
  if(!pluginCacheFileExists(path, type))
    return false;
  
  bool res = false;
  const QString targ_filepath = path + "/" + QString(pluginCacheFilename(type));

  QFile targ_qfile(targ_filepath);
  
  // Cache file already existed. Open it for reading.
  if(!targ_qfile.open(QIODevice::ReadOnly | QIODevice::Text))
  {
    std::fprintf(stderr, "readPluginCacheFile: targ_qfile.open() failed: filename:%s\n",
                     targ_filepath.toLatin1().constData());
  }
  else
  {
      MusECore::Xml xml(&targ_qfile);

      // Returns true on error.
      if(readPluginScan(xml, list, readPorts, readEnums))
      {
        std::fprintf(stderr, "readPluginCacheFile: readPluginScan failed: filename:%s\n",
                             targ_filepath.toLatin1().constData());
      }

      DEBUG_PLUGIN_SCAN(stderr, "readPluginCacheFile: targ_qfile closing filename:%s\n",
                      filename.toLatin1().constData());
      targ_qfile.close();
      
      res = true;
  }

  return res;
}
  
//---------------------------------------------------------
//   readPluginCacheFiles
//---------------------------------------------------------

bool readPluginCacheFiles(
  const QString& path,
  PluginScanList* list,
  bool readPorts,
  bool readEnums,
  PluginScanInfoStruct::PluginType_t types)
{
  bool res = true;
  
  if(types & (PluginScanInfoStruct::PluginTypeDSSI | PluginScanInfoStruct::PluginTypeDSSIVST))
  {
    if(!readPluginCacheFile(path, list, readPorts, readEnums, PluginScanInfoStruct::PluginTypeDSSI))
      res = false;
  }

  if(types & PluginScanInfoStruct::PluginTypeMESS)
  {
    if(!readPluginCacheFile(path, list, readPorts, readEnums, PluginScanInfoStruct::PluginTypeMESS))
      res = false;
  }

  if(types & PluginScanInfoStruct::PluginTypeLADSPA)
  {
    if(!readPluginCacheFile(path, list, readPorts, readEnums, PluginScanInfoStruct::PluginTypeLADSPA))
      res = false;
  }

  if(types & PluginScanInfoStruct::PluginTypeLinuxVST)
  {
    if(!readPluginCacheFile(path, list, readPorts, readEnums, PluginScanInfoStruct::PluginTypeLinuxVST))
      res = false;
  }

  if(types & PluginScanInfoStruct::PluginTypeLV2)
  {
    if(!readPluginCacheFile(path, list, readPorts, readEnums, PluginScanInfoStruct::PluginTypeLV2))
      res = false;
  }

  if(types & PluginScanInfoStruct::PluginTypeVST)
  {
    if(!readPluginCacheFile(path, list, readPorts, readEnums, PluginScanInfoStruct::PluginTypeVST))
      res = false;
  }

  return res;
}

} // namespace MusEPlugin


