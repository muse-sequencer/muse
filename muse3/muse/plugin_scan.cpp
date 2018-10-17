//=========================================================
//  MusE
//  Linux Music Editor
//
//  plugin_scan.cpp
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
#include <QProcess>
#include <QByteArray>
#include <QStringList>
#include <sys/stat.h>
#include <stdio.h>

#include "plugin_scan.h"

namespace MusECore {

// REMOVE Tim. scan. Changed.
// QString PluginScanInfo::dssiUiFilename() const
// {
//   QString libr(lib());
//   if(dirPath().isEmpty() || libr.isEmpty())
//     return QString();
// 
//   QString guiPath(dirPath() + "/" + libr);
// 
//   QDir guiDir(guiPath, "*", QDir::Unsorted, QDir::Files);
//   if(!guiDir.exists())
//     return QString();
// 
//   QStringList list = guiDir.entryList();
// 
// //   QString plug(pluginLabel());
//   QString plug(_label);
//   QString lib_qt_ui;
//   QString lib_any_ui;
//   QString plug_qt_ui;
//   QString plug_any_ui;
// 
//   for(int i = 0; i < list.count(); ++i)
//   {
//     QFileInfo fi(guiPath + QString("/") + list[i]);
//     QString gui(fi.filePath());
//     struct stat buf;
//     if(stat(gui.toLatin1().constData(), &buf))
//       continue;
//     if(!((S_ISREG(buf.st_mode) || S_ISLNK(buf.st_mode)) &&
//         (buf.st_mode & (S_IXUSR | S_IXGRP | S_IXOTH))))
//       continue;
// 
//     // FIXME: Qt::CaseInsensitive - a quick and dirty way to accept any suffix. Should be case sensitive...
//     if(!libr.isEmpty())
//     {
//       if(lib_qt_ui.isEmpty() && list[i].contains(libr + QString("_qt"), Qt::CaseInsensitive))
//         lib_qt_ui = gui;
//       if(lib_any_ui.isEmpty() && list[i].contains(libr + QString('_') /*, Qt::CaseInsensitive*/))
//         lib_any_ui = gui;
//     }
//     if(!plug.isEmpty())
//     {
//       if(plug_qt_ui.isEmpty() && list[i].contains(plug + QString("_qt"), Qt::CaseInsensitive))
//         plug_qt_ui = gui;
//       if(plug_any_ui.isEmpty() && list[i].contains(plug + QString('_') /*, Qt::CaseInsensitive*/))
//         plug_any_ui = gui;
//     }
//   }
// 
//   // Prefer qt plugin ui
//   if(!plug_qt_ui.isEmpty())
//     return plug_qt_ui;
//   // Prefer any plugin ui
//   if(!plug_any_ui.isEmpty())
//     return plug_any_ui;
//   // Prefer qt lib ui
//   if(!lib_qt_ui.isEmpty())
//     return lib_qt_ui;
//   // Prefer any lib ui
//   if(!lib_any_ui.isEmpty())
//     return lib_any_ui;
// 
//   // No suitable UI file found
//   return QString();
// };

  
//---------------------------------------------------------
//   write
//---------------------------------------------------------

void PluginScanInfo::write(int level, Xml& xml)
      {
      xml.tag(level++, "plugin file=\"%s\" label=\"%s\"",
         Xml::xmlString(filePath()).toLatin1().constData(),
         Xml::xmlString(_label).toLatin1().constData());
      
      xml.intTag(level, "type", _type);
      xml.intTag(level, "class", _class);
      xml.uintTag(level, "uniqueID", _uniqueID);
      xml.intTag(level, "subID", _subID);
      if(!_name.isEmpty())
        xml.strTag(level, "name", _name);
      if(!_description.isEmpty())
        xml.strTag(level, "description", _description);
      if(!_version.isEmpty())
        xml.strTag(level, "version", _version);
      if(!_maker.isEmpty())
        xml.strTag(level, "maker", _maker);
      if(!_copyright.isEmpty())
        xml.strTag(level, "copyright", _copyright);
      xml.intTag(level, "apiVersionMajor", _apiVersionMajor);
      xml.intTag(level, "apiVersionMinor", _apiVersionMinor);
      xml.intTag(level, "pluginVersionMajor", _pluginVersionMajor);
      xml.intTag(level, "pluginVersionMinor", _pluginVersionMinor);
      xml.intTag(level, "portCount", _portCount);
      xml.intTag(level, "inports", _inports);
      xml.intTag(level, "outports", _outports);
      xml.intTag(level, "controlInPorts", _controlInPorts);
      xml.intTag(level, "controlOutPorts", _controlOutPorts);
      xml.intTag(level, "hasGui", _hasGui);
      xml.intTag(level, "hasChunks", _hasChunks);
      xml.intTag(level, "requiredFeatures", _requiredFeatures);
    #ifdef VST_NATIVE_SUPPORT
      if(_vstPluginFlags != vstPluginNoFlags)
        xml.intTag(level, "vstPluginFlags", _vstPluginFlags);
    #endif
      if(!_uiFilename.isEmpty())
        xml.strTag(level, "uiFilename", _uiFilename);

      for (unsigned long i = 0; i < _portCount; ++i) {
            const PluginPortInfo& port_info = _portList[i];
            QString s("port name=\"%1\" idx=\"%2\" type=\"%3\" /");
            xml.tag(level, s.arg(Xml::xmlString(port_info._name).toLatin1().constData()).arg(i).arg(port_info._type).toLatin1().constData()); }
            
      xml.tag(level--, "/plugin");
      }


//---------------------------------------------------------
//   readPort
//    Returns true on error
//---------------------------------------------------------

bool PluginScanInfo::readPort(Xml& xml)
      {
      PluginPortInfo port_info;
      for (;;) {
            Xml::Token token = xml.parse();
            const QString& tag = xml.s1();

            switch (token) {
                  case Xml::Error:
                  case Xml::End:
                        return true;
                  case Xml::TagStart:
                        xml.unknown("readPort");
                        break;
                  case Xml::Attribut:
                        if (tag == "name")
                              port_info._name = xml.s2();
                        else if (tag == "idx")
                              port_info._index = xml.s2().toULong();
                        else if (tag == "type")
                              port_info._type = xml.s2().toInt();
                        break;
                  case Xml::TagEnd:
                        if (tag == "port") {
                          _portList.push_back(port_info);
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
//   read
//    return true on error
//---------------------------------------------------------

bool PluginScanInfo::read(Xml& xml)
      {
      for (;;) {
            Xml::Token token(xml.parse());
            const QString& tag(xml.s1());
            switch (token) {
                  case Xml::Error:
                  case Xml::End:
                        return true;
                  case Xml::TagStart:
                        if (tag == "type")
                              _type = PluginScanInfo::PluginType(xml.parseInt());
                        else if (tag == "class")
                              _class = PluginScanInfo::PluginClass(xml.parseInt());
                        else if (tag == "uniqueID")
                              _uniqueID = xml.parseUInt();
                        else if (tag == "subID")
                              _subID = xml.parseInt();
                        else if (tag == "name")
                              _name = xml.parse1();
                        else if (tag == "description")
                              _description = xml.parse1();
                        else if (tag == "version")
                              _version = xml.parse1();
                        else if (tag == "maker")
                              _maker = xml.parse1();
                        else if (tag == "copyright")
                              _copyright = xml.parse1();
                        else if (tag == "apiVersionMajor")
                              _apiVersionMajor = xml.parseInt();
                        else if (tag == "apiVersionMinor")
                              _apiVersionMinor = xml.parseInt();
                        else if (tag == "pluginVersionMajor")
                              _pluginVersionMajor = xml.parseInt();
                        else if (tag == "pluginVersionMinor")
                              _pluginVersionMinor = xml.parseInt();
                        else if (tag == "portCount")
                              _portCount = xml.parseInt();
                        else if (tag == "inports")
                              _inports = xml.parseInt();
                        else if (tag == "outports")
                              _outports = xml.parseInt();
                        else if (tag == "controlInPorts")
                              _controlInPorts = xml.parseInt();
                        else if (tag == "controlOutPorts")
                              _controlOutPorts = xml.parseInt();
                        else if (tag == "hasGui")
                              _hasGui = xml.parseInt();
                        else if (tag == "hasChunks")
                              _hasChunks = xml.parseInt();
                        else if (tag == "requiredFeatures")
                              _requiredFeatures = xml.parseInt();
                      #ifdef VST_NATIVE_SUPPORT
                        else if (tag == "vstPluginFlags")
                              _vstPluginFlags = xml.parseInt();
                      #endif
                        else if (tag == "uiFilename")
                              _uiFilename = xml.parse1();
                        else if (tag == "port")
                              readPort(xml);
                        else
                              xml.unknown("PluginScanInfo");
                        break;
                  case Xml::Attribut:
                        if (tag == "file") {
                                _fi = QFileInfo(xml.s2());
                              }
                        else if (tag == "label") {
                                _label = xml.s2();
                              }
                        break;
                  case Xml::TagEnd:
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

const char* PluginScanInfo::typeString() const
{
  switch(_type)
  {
    case PluginTypeLADSPA:
      return "ladspa";
    break;

    case PluginTypeDSSI:
      return "dssi";
    break;

    case PluginTypeDSSIVST:
      return "dssi_vst";
    break;

    case PluginTypeVST:
      return "vst";
    break;

    case PluginTypeLinuxVST:
      return "linux_vst";
    break;

    case PluginTypeLV2:
      return "lv2";
    break;

    case PluginTypeMESS:
      return "mess";
    break;

    case PluginTypeAll:
    break;
  }
  return 0;
}

const char* PluginScanInfo::classString() const
{
  switch(_class)
  {
    case PluginClassEffect:
      return "effect";
    break;

    case PluginClassInstrument:
      return "instrument";
    break;

    case PluginClassAll:
    break;
  }
  return 0;
}

void PluginScanInfo::dump(const char* prefixMessage) const
{
  const char* type_name = typeString();
  const char* class_name = classString();
  
  fprintf(stderr, "%s plugin:%s type:%s class:%s name:%s label:%s required features:%d\n",
          prefixMessage,
          type_name,
          class_name,
          _fi.filePath().toLatin1().constData(),
          _name.toLatin1().constData(),
          _label.toLatin1().constData(),
          _requiredFeatures
  );
}


//---------------------------------------------------------
//   readPluginScan
//    return true on error
//---------------------------------------------------------

bool readPluginScan(Xml& xml, PluginScanList& plugin_scan_list)
      {
      for (;;) {
            Xml::Token token(xml.parse());
            const QString& tag(xml.s1());
            switch (token) {
                  case Xml::Error:
                  case Xml::End:
                        return true;
                  case Xml::TagStart:
                        if (tag == "muse")
                              break;
                        else if (tag == "plugin")
                        {
                              PluginScanInfo info;
                              // Returns false on success.
                              if(!info.read(xml))
                                plugin_scan_list.push_back(info);
                              break;
                        }
                        else
                              xml.unknown("readPluginScan");
                        break;
                  case Xml::Attribut:
                        if (tag == "version") {
                              int major = xml.s2().section('.', 0, 0).toInt();
                              int minor = xml.s2().section('.', 1, 1).toInt();
                              xml.setVersion(major, minor);
                              }
                        break;
                  case Xml::TagEnd:
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
//   readPluginScan
//     Returns true on success
//---------------------------------------------------------

bool pluginScan(const QString& filename, PluginScanList& scanList)
{
  QProcess process;
  
  const QString prog = QString(BINDIR) + QString("/muse_plugin_scan");
  
  QStringList args;
  args << "-f" << filename;
  
  process.start(prog, args);
  
//   if(!process.waitForStarted(4000))
//   {
//     fprintf(stderr, "pluginScan: waitForStarted failed\n");
//     return false;
//   }
  
//   if(!process.waitForReadyRead(4000))
//   {
//     fprintf(stderr, "pluginScan: waitForReadyRead failed\n");
//     return false;
//   }
  
  if(!process.waitForFinished(4000))
  {
    fprintf(stderr, "pluginScan: waitForFinished failed\n");
    return false;
  }
  
  if(process.exitStatus() != QProcess::NormalExit)
  {
    fprintf(stderr, "pluginScan: process not exited normally\n");
    return false;
  }
  
  if(process.exitCode() != 0)
  {
    fprintf(stderr, "pluginScan: process exit code not zero\n");
    return false;
  }
  
  QByteArray err_array = process.readAllStandardError();
  if(!err_array.isEmpty())
  {
    // Terminate just to be sure.
    err_array.append(char(0));
    fprintf(stderr, "pluginScan: stderr array:%s\n", err_array.constData());
  }
  
  QByteArray array = process.readAllStandardOutput();
  
  if(array.isEmpty())
  {
    fprintf(stderr, "pluginScan: stdout array is empty\n");
    return false;
  }
  
  // Terminate just to be sure.
  array.append(char(0));
  
  // Create an xml object based on the array.
  Xml xml(array.constData());

  // Read the list of plugins found in the xml.
  if(readPluginScan(xml, scanList))
  {
    fprintf(stderr, "pluginScan: readPluginScan failed\n");
  }
  //else
  //{
  //  fprintf(stderr, "pluginScan: readPluginScan success: scanList num plugins:%d\n", (int)scanList.size());
  //}
  
  fprintf(stderr, "pluginScan: success: stdout array:\n%s\n", array.constData());
  
  return true;
}

} // namespace MusECore
