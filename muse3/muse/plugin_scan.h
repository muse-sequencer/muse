//=========================================================
//  MusE
//  Linux Music Editor
//
//  plugin_scan.h
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

#ifndef __PLUGIN_SCAN_H__
#define __PLUGIN_SCAN_H__

#include <QString>
#include <QFileInfo>
#include <vector>

#include "config.h"
#include "globaldefs.h"
#include "xml.h"

namespace MusECore {

class Xml;

struct PluginPortInfo
{
  enum PortType { UnknownPort = 0x00, AudioPort = 0x01, ControlPort = 0x02, InputPort = 0x04, OutputPort = 0x08 };
  typedef int PortType_t;

  QString _name;
  unsigned long _index;
  PortType_t _type;
  
  PluginPortInfo() { _index = 0; _type = UnknownPort; }
};

typedef std::vector<PluginPortInfo> PluginPortList;
typedef PluginPortList::iterator iPluginPortList;
typedef PluginPortList::const_iterator ciPluginPortList;

class PluginScanInfo
{
  public:
    enum PluginType { 
      PluginTypeLADSPA   = 0x01,  PluginTypeDSSI    = 0x02,
      PluginTypeVST      = 0x04,  PluginTypeDSSIVST = 0x08,
      PluginTypeLinuxVST = 0x10,  PluginTypeLV2     = 0x20,
      PluginTypeMESS     = 0x40,
      PluginTypeAll = PluginTypeLADSPA   | PluginTypeDSSI |
                      PluginTypeVST      | PluginTypeDSSIVST |
                      PluginTypeLinuxVST | PluginTypeLV2 |
                      PluginTypeMESS };
    typedef int PluginType_t;

    enum PluginClass { PluginClassNone = 0x00,
      PluginClassEffect = 0x01, PluginClassInstrument = 0x02,
      PluginClassAll = PluginClassEffect | PluginClassInstrument };
    typedef int PluginClass_t;

  //private:
  public:
    QFileInfo _fi;
    PluginType _type;
    PluginClass_t _class;
    unsigned long _uniqueID;
    long _subID; // vst shell ID etc.
    QString _label;
    QString _name;
    QString _description;
    QString _version;
    QString _maker;
    QString _copyright;
    
    int _apiVersionMajor;
    int _apiVersionMinor;

    int _pluginVersionMajor;
    int _pluginVersionMinor;
    
    unsigned long _portCount;
    unsigned long _inports;
    unsigned long _outports;
    unsigned long _controlInPorts;
    unsigned long _controlOutPorts;
    // Port number to control input index. Item is -1 if it's not a control input.
    //std::vector<unsigned long> rpIdx;
//     std::vector<unsigned long> _pIdx; //control port numbers
//     std::vector<unsigned long> _poIdx; //control out port numbers
//     std::vector<unsigned long> _iIdx; //input port numbers
//     std::vector<unsigned long> _oIdx; //output port numbers

    bool _hasGui;
    bool _hasChunks;
    
    PluginFeatures_t _requiredFeatures;
    
    #ifdef VST_NATIVE_SUPPORT
    VstPluginFlags_t _vstPluginFlags;
    #endif
  
    QString _uiFilename;
    
    PluginPortList _portList;
    
  public:
    PluginScanInfo() :
      _type(PluginTypeLADSPA),
      _class(PluginClassNone),
      _uniqueID(0),
      _subID(0),
      _apiVersionMajor(0),
      _apiVersionMinor(0),
      _portCount(0),
      _inports(0),
      _outports(0),
      _controlInPorts(0),
      _controlOutPorts(0),
      _hasGui(false),
      _hasChunks(false),
      _requiredFeatures(PluginNoFeatures)
      #ifdef VST_NATIVE_SUPPORT
      , _vstPluginFlags(vstPluginNoFlags)
      #endif
      { };

    PluginScanInfo(const QFileInfo& fi,
                   PluginType type,
                   PluginClass_t plugin_class,
                   unsigned long unique_id,
                   long sub_id,
                   const QString& label,
                   const QString& name,
                   const QString& description,
                   const QString& version,
                   const QString& maker,
                   const QString& copyright,
                   int api_version_major,
                   int api_version_minor,
                   unsigned long port_count,
                   unsigned long inports,
                   unsigned long outports,
                   unsigned long ctrl_inports,
                   unsigned long ctrl_outports,
                   bool has_gui,
                   bool has_chunks,
                   PluginFeatures_t required_features,
                 #ifdef VST_NATIVE_SUPPORT
                   VstPluginFlags_t vst_plugin_flags,
                 #endif
                   const QString& ui_filename
                  ) :
      _fi(fi),
      _type(type),
      _class(plugin_class),
      _uniqueID(unique_id),
      _subID(sub_id),
      _label(label),
      _name(name),
      _description(description),
      _version(version),
      _maker(maker),
      _copyright(copyright),
      _apiVersionMajor(api_version_major),
      _apiVersionMinor(api_version_minor),
      _portCount(port_count),
      _inports(inports),
      _outports(outports),
      _controlInPorts(ctrl_inports),
      _controlOutPorts(ctrl_outports),
      _hasGui(has_gui),
      _hasChunks(has_chunks),
      _requiredFeatures(required_features),
    #ifdef VST_NATIVE_SUPPORT
      _vstPluginFlags(vst_plugin_flags),
    #endif
      _uiFilename(ui_filename)
      { };
      
    QString lib(bool complete = true) const
      { return complete ? _fi.completeBaseName() : _fi.baseName(); }
    QString dirPath(bool complete = true) const
      { return complete ? _fi.absolutePath() : _fi.path(); }
    QString filePath() const
      { return _fi.filePath(); }
    QString fileName() const
      { return _fi.fileName(); }
    QString uiFilename() const { return _uiFilename; }
    bool readPort(Xml& xml);
    void write(int level, Xml& xml);
    bool read(Xml& xml);
    
    const char* typeString() const;
    const char* classString() const;
    
    void dump(const char* prefixMessage = 0) const;
};
  
typedef std::vector<PluginScanInfo> PluginScanList;
typedef PluginScanList::iterator iPluginScanList;
typedef PluginScanList::const_iterator ciPluginScanList;


bool pluginScan(const QString& filename, PluginScanList& scanList);

} // namespace MusECore

#endif

