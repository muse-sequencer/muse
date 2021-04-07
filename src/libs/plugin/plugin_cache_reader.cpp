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
#include <QStandardPaths>

// For sorting port enum values.
#include <map>
  
#include <cstring>
#include <cstdlib>

#include "plugin_cache_reader.h"
#include "xml.h"

// For debugging output: Uncomment the fprintf section.
#define DEBUG_PLUGIN_SCAN(dev, format, args...)  // std::fprintf(dev, format, ##args);

//#define PORTS_ARE_SINGLE_LINE_TAGS 1

namespace MusEPlugin {

void setPluginScanFileInfo(const QString& filename, PluginScanInfoStruct* info)
{
  if(filename.isEmpty())
    return;

  const QFileInfo fi(filename);

  // AppImage: Adjust dynamic path prefix of internal plugins
  const QByteArray appDir = qgetenv("APPDIR");
  QString path = fi.path();
  QString absPath = fi.absolutePath();
  if (!appDir.isEmpty()) {
      const QString intLibPath = QString(LIBDIR);
      int idx;
      if ((idx = path.indexOf(intLibPath)) > 0) {
          path.remove(0, idx);
          path = appDir + path;
      }
      if ((idx = absPath.indexOf(intLibPath)) > 0) {
          absPath.remove(0, idx);
          absPath = appDir + absPath;
      }
  }

  info->_completeBaseName = PLUGIN_SET_QSTRING(fi.completeBaseName());
  info->_baseName         = PLUGIN_SET_QSTRING(fi.baseName());
  info->_suffix           = PLUGIN_SET_QSTRING(fi.suffix());
  info->_completeSuffix   = PLUGIN_SET_QSTRING(fi.completeSuffix());
  info->_absolutePath     = PLUGIN_SET_QSTRING(absPath);
  info->_path             = PLUGIN_SET_QSTRING(path);
//  info->_absolutePath     = PLUGIN_SET_QSTRING(fi.absolutePath());
//  info->_path             = PLUGIN_SET_QSTRING(fi.path());
  info->_fileTime         = fi.lastModified().toMSecsSinceEpoch();
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
                        else if (tag == "filetime")
                              info->_fileTime = xml.parseLongLong();
                        else if (tag == "fileIsBad")
                              info->_fileIsBad = xml.parseInt();
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
                                // We must include all plugins.
                                list->add(new PluginScanInfo(info));
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
#if (QT_VERSION >= QT_VERSION_CHECK(5, 10, 0))
  QString ladspaPath = qEnvironmentVariable("LADSPA_PATH");
#else
  QString ladspaPath = QString::fromLocal8Bit(qgetenv("LADSPA_PATH"));
#endif
  if(ladspaPath.isEmpty())
  {
    QString homePath = QStandardPaths::writableLocation(QStandardPaths::HomeLocation);
    if(!homePath.isEmpty())
      homePath += QString("/ladspa:") + homePath + QString("/.ladspa:");
    ladspaPath = homePath + QString("/usr/local/lib64/ladspa:/usr/lib64/ladspa:/usr/local/lib/ladspa:/usr/lib/ladspa");
  }
  if(!ladspaPath.isEmpty())
// QString::*EmptyParts is deprecated, use Qt::*EmptyParts, new as of 5.14.
#if QT_VERSION >= 0x050e00
    sl.append(ladspaPath.split(":", Qt::SkipEmptyParts, Qt::CaseSensitive));
#else
    sl.append(ladspaPath.split(":", QString::SkipEmptyParts, Qt::CaseSensitive));
#endif
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
#if (QT_VERSION >= QT_VERSION_CHECK(5, 10, 0))
  QString messPath = qEnvironmentVariable("MESS_PATH");
#else
  QString messPath = QString::fromLocal8Bit(qgetenv("MESS_PATH"));
#endif
  if(messPath.isEmpty())
  {
    QString homePath = QStandardPaths::writableLocation(QStandardPaths::HomeLocation);
    if(!homePath.isEmpty())
      homePath += QString("/MESS:");
    messPath = homePath + QString("/usr/local/lib64/MESS:/usr/lib64/MESS:/usr/local/lib/MESS:/usr/lib/MESS");
  }
  if(!messPath.isEmpty())
// QString::*EmptyParts is deprecated, use Qt::*EmptyParts, new as of 5.14.
#if QT_VERSION >= 0x050e00
    sl.append(messPath.split(":", Qt::SkipEmptyParts, Qt::CaseSensitive));
#else
    sl.append(messPath.split(":", QString::SkipEmptyParts, Qt::CaseSensitive));
#endif
  return sl;
}

//---------------------------------------------------------
//   pluginGetDssiDirectories
//---------------------------------------------------------

QStringList pluginGetDssiDirectories()
{
  QStringList sl;
#if (QT_VERSION >= QT_VERSION_CHECK(5, 10, 0))
  QString dssiPath = qEnvironmentVariable("DSSI_PATH");
#else
  QString dssiPath = QString::fromLocal8Bit(qgetenv("DSSI_PATH"));
#endif
  if(dssiPath.isEmpty())
  {
    QString homePath = QStandardPaths::writableLocation(QStandardPaths::HomeLocation);
    if(!homePath.isEmpty())
      homePath += QString("/dssi:") + homePath + QString("/.dssi:");
    dssiPath = homePath + QString("/usr/local/lib64/dssi:/usr/lib64/dssi:/usr/local/lib/dssi:/usr/lib/dssi");
  }
  if(!dssiPath.isEmpty())
// QString::*EmptyParts is deprecated, use Qt::*EmptyParts, new as of 5.14.
#if QT_VERSION >= 0x050e00
    sl.append(dssiPath.split(":", Qt::SkipEmptyParts, Qt::CaseSensitive));
#else
    sl.append(dssiPath.split(":", QString::SkipEmptyParts, Qt::CaseSensitive));
#endif
  return sl;
}

//---------------------------------------------------------
//   pluginGetLinuxVstDirectories Linux vst plugins (*.so)
//---------------------------------------------------------

QStringList pluginGetLinuxVstDirectories()
{
  QStringList sl;
#if (QT_VERSION >= QT_VERSION_CHECK(5, 10, 0))
  QString vstPath = qEnvironmentVariable("LXVST_PATH");
#else
  QString vstPath = QString::fromLocal8Bit(qgetenv("LXVST_PATH"));
#endif
  if(vstPath.isEmpty())
  {
#if (QT_VERSION >= QT_VERSION_CHECK(5, 10, 0))
    QString vstPath = qEnvironmentVariable("VST_PATH");
#else
    QString vstPath = QString::fromLocal8Bit(qgetenv("VST_PATH"));
#endif
    if(vstPath.isEmpty())
    {
      QString homePath = QStandardPaths::writableLocation(QStandardPaths::HomeLocation);
      QString fin_path;
      if(!homePath.isEmpty())
      {

// On win, vst is usually where *.dll files are found. We don't want that with Linux *.so vst files.
// Otherwise on Linux for example, vst is where Linux vst *.so files are found.
// On win, lxvst should be safe, likely where Linux vst *.so files might be found (if that's even a thing!).
#ifndef Q_OS_WIN
        fin_path += homePath + QString("/vst:");
#endif
        fin_path += homePath + QString("/lxvst:");

#ifndef Q_OS_WIN
        fin_path += homePath + QString("/.vst:");
#endif
        fin_path += homePath + QString("/.lxvst:");
      }

#ifndef Q_OS_WIN
      fin_path += QString("/usr/local/lib64/vst:");
#endif
      fin_path += QString("/usr/local/lib64/lxvst:");
      
#ifndef Q_OS_WIN
      fin_path += QString("/usr/local/lib/vst:");
#endif
      fin_path += QString("/usr/local/lib/lxvst:");
      
#ifndef Q_OS_WIN
      fin_path += QString("/usr/lib64/vst:");
#endif
      fin_path += QString("/usr/lib64/lxvst:");
      
#ifndef Q_OS_WIN
      fin_path += QString("/usr/lib/vst:");
#endif
      // No last colon.
      fin_path += QString("/usr/lib/lxvst");
                
      vstPath = fin_path;
    }
  }
  if(!vstPath.isEmpty())
// QString::*EmptyParts is deprecated, use Qt::*EmptyParts, new as of 5.14.
#if QT_VERSION >= 0x050e00
    sl.append(vstPath.split(":", Qt::SkipEmptyParts, Qt::CaseSensitive));
#else
    sl.append(vstPath.split(":", QString::SkipEmptyParts, Qt::CaseSensitive));
#endif
  return sl;
}

//---------------------------------------------------------
//   pluginGetVstDirectories Win vst plugins (*.dll)
//---------------------------------------------------------

QStringList pluginGetVstDirectories()
{
  QStringList sl;
#if (QT_VERSION >= QT_VERSION_CHECK(5, 10, 0))
  QString vstPath = qEnvironmentVariable("VST_PATH");
#else
  QString vstPath = QString::fromLocal8Bit(qgetenv("VST_PATH"));
#endif
  if(vstPath.isEmpty())
  {
    // TODO: Refine this, and below, for Q_OS_WIN. Where exactly do we look though?
    QString homePath = QStandardPaths::writableLocation(QStandardPaths::HomeLocation);
    if(!homePath.isEmpty())
    {
// On win, vst is usually where *.dll files are found. We don't want that with Linux *.so vst files.
// Otherwise on Linux for example, vst is where Linux vst *.so files are found.
#ifdef Q_OS_WIN
      homePath += QString("/vst:") + homePath + QString("/.vst");
#else
      homePath += QString("/vst win 32bit:") + homePath + QString("/.vst win 32bit");
#endif
    }
    vstPath = homePath;
  }
  if(!vstPath.isEmpty())
// QString::*EmptyParts is deprecated, use Qt::*EmptyParts, new as of 5.14.
#if QT_VERSION >= 0x050e00
    sl.append(vstPath.split(":", Qt::SkipEmptyParts, Qt::CaseSensitive));
#else
    sl.append(vstPath.split(":", QString::SkipEmptyParts, Qt::CaseSensitive));
#endif
  return sl;
}

//---------------------------------------------------------
//   pluginGetLv2Directories
//---------------------------------------------------------

QStringList pluginGetLv2Directories()
{
  QStringList sl;
#if (QT_VERSION >= QT_VERSION_CHECK(5, 10, 0))
  QString lv2Path = qEnvironmentVariable("LV2_PATH");
#else
  QString lv2Path = QString::fromLocal8Bit(qgetenv("LV2_PATH"));
#endif
  if(lv2Path.isEmpty())
  {
    QString homePath = QStandardPaths::writableLocation(QStandardPaths::HomeLocation);
    if(!homePath.isEmpty())
      homePath += QString("/lv2:") + homePath + QString("/.lv2:");
    lv2Path = homePath + QString("/usr/local/lib64/lv2:/usr/lib64/lv2:/usr/local/lib/lv2:/usr/lib/lv2");
  }
  if(!lv2Path.isEmpty())
// QString::*EmptyParts is deprecated, use Qt::*EmptyParts, new as of 5.14.
#if QT_VERSION >= 0x050e00
    sl.append(lv2Path.split(":", Qt::SkipEmptyParts, Qt::CaseSensitive));
#else
    sl.append(lv2Path.split(":", QString::SkipEmptyParts, Qt::CaseSensitive));
#endif
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

    case PluginScanInfoStruct::PluginTypeUnknown:
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

    // SPECIAL for LV2: Obsolete. Cache file not used any more.
    // Keep so we can delete old files.
    case PluginScanInfoStruct::PluginTypeLV2:
      return "lv2_plugins.scan";
    break;

    case PluginScanInfoStruct::PluginTypeVST:
      return "vst_plugins.scan";
    break;
    
    case PluginScanInfoStruct::PluginTypeUnknown:
      return "unknown_plugins.scan";
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

  if(types & PluginScanInfoStruct::PluginTypeUnknown)
    res |= pluginCacheFileExists(path, PluginScanInfoStruct::PluginTypeUnknown);

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

  // SPECIAL for LV2: No need for a cache file. Do not read one here.
  //if(types & PluginScanInfoStruct::PluginTypeLV2)
  //{
  //  if(!readPluginCacheFile(path, list, readPorts, readEnums, PluginScanInfoStruct::PluginTypeLV2))
  //    res = false;
  //}

  if(types & PluginScanInfoStruct::PluginTypeVST)
  {
    if(!readPluginCacheFile(path, list, readPorts, readEnums, PluginScanInfoStruct::PluginTypeVST))
      res = false;
  }

  if(types & PluginScanInfoStruct::PluginTypeUnknown)
  {
    if(!readPluginCacheFile(path, list, readPorts, readEnums, PluginScanInfoStruct::PluginTypeUnknown))
      res = false;
  }

  return res;
}

} // namespace MusEPlugin


