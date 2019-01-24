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

// Whether or not to use QString instead of std::string.
// (And maybe QFileInfo and QProcess instead of strings and pthreads etc.)
#define PLUGIN_INFO_USE_QT 1


#include <vector>
#include <map>
// #include <list>
// #include <memory>

#ifdef PLUGIN_INFO_USE_QT
  #include <QString>
  typedef QString PluginInfoString_t;
#else
  #include <string>
  typedef std::string PluginInfoString_t;
#endif // PLUGIN_INFO_USE_QT

#include "config.h"
#include "globaldefs.h"

  
namespace MusEPlugin {

const char* const VST_OLD_PLUGIN_ENTRY_POINT = "main";
const char* const VST_NEW_PLUGIN_ENTRY_POINT = "VSTPluginMain";
  

//-----------------------------------------
// PluginPortEnumValue
//-----------------------------------------

struct PluginPortEnumValue
{
  float _value;
  PluginInfoString_t _label;
  PluginPortEnumValue() { _value = 0.0; }
  PluginPortEnumValue(float value, PluginInfoString_t label)
    : _value(value), _label(label) { }
};

typedef std::vector<PluginPortEnumValue> EnumValueList;
typedef EnumValueList::iterator iEnumValueList;
typedef EnumValueList::const_iterator ciEnumValueList;

typedef std::map<unsigned long /* port index */, EnumValueList,
                 std::less<unsigned long> > PortEnumValueMap;
typedef PortEnumValueMap::iterator iPortEnumValueMap;
typedef PortEnumValueMap::const_iterator ciPortEnumValueMap;
typedef std::pair<unsigned long /* port index */, EnumValueList> PortEnumValueMapPair;


//-----------------------------------------
// PluginPortInfo
//-----------------------------------------

struct PluginPortInfo
{
  enum PortType { UnknownPort = 0x00, AudioPort = 0x01, ControlPort = 0x02, MidiPort = 0x04, InputPort = 0x08, OutputPort = 0x10 };
  typedef int PortType_t;

  enum ValueFlags { NoValueFlags = 0x00,
    IntegerVal = 0x01,
    ToggledVal = 0x02,
    TriggerVal = 0x04,
    LogVal = 0x08,
    HasEnumerations = 0x10,
    HasMin = 0x20,
    HasMax = 0x40,
    HasDefault = 0x80,
    HasStep = 0x100 };
  typedef int PortValueFlags_t;

  enum PortFlags { NoPortFlags = 0x00,
    // Indicates min and max should be scaled by the current samplerate.
    ScaleBySamplerate = 0x01,
    
    // Indicates min is valid, and should be scaled by the current samplerate
    //  if ScaleBySamplerate is true. Otherwise min is set to zero.
    //BoundedBelow = 0x02,
    
    // Indicates max is valid, and should be scaled by the current samplerate
    //  if ScaleBySamplerate is true. Otherwise max is set to one.
    //BoundedAbove = 0x04
    
    // Whether the port is intended for latency reporting.
    IsLatency = 0x02,
    
    // Whether the port is an audio port which is used for high-speed control signals.
    IsCVPort = 0x04,
    
    SupportsTimePosition = 0x08,
    IsFreewheel = 0x10
  };
  typedef int PortFlags_t;

  static const float defaultPortValue;
  static const float defaultPortMin;
  static const float defaultPortMax;
  static const float defaultPortStep;
  static const float defaultPortSmallStep;
  static const float defaultPortLargeStep;
  
  PluginInfoString_t _name;
  PluginInfoString_t _symbol;
  unsigned long _index;
  PortType_t _type;
  PortValueFlags_t _valueFlags;
  PortFlags_t _flags;
  float _min;
  float _max;
  float _defaultVal;
  float _step;
  float _smallStep;
  float _largeStep;
  
  PluginPortInfo() {
    _index = 0;
    _type = UnknownPort;
    _valueFlags = NoValueFlags;
    _flags = NoPortFlags;
    _min = defaultPortMin;
    _max = defaultPortMax;
    _defaultVal = defaultPortValue;
    _step = 0.0;
    _smallStep = 0.0;
    _largeStep = 0.0;
  }
  
  float min(float sampleRate) const { return _flags & ScaleBySamplerate ? _min * sampleRate : _min; }
  float max(float sampleRate) const { return _flags & ScaleBySamplerate ? _max * sampleRate : _max;  }
};

typedef std::vector<PluginPortInfo> PluginPortList;
typedef PluginPortList::iterator iPluginPortList;
typedef PluginPortList::const_iterator ciPluginPortList;


//-----------------------------------------
// PluginScanInfoStruct
//-----------------------------------------

class PluginScanInfoStruct
{
   public:
    enum PluginType { PluginTypeNone = 0x00,
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

    enum PluginFlags { NoPluginFlags = 0x00,
      HasGui = 0x01, HasChunks = 0x02, Realtime = 0x04, HardRealtimeCapable = 0x08, HasFreewheelPort = 0x10,
      HasLatencyPort = 0x20 };
    typedef int PluginFlags_t;
    
  //private:
  //protected:
  public:
    
    //QFileInfo _fi;
    PluginInfoString_t _completeBaseName;
    PluginInfoString_t _baseName;
    PluginInfoString_t _suffix;
    PluginInfoString_t _completeSuffix;
    PluginInfoString_t _absolutePath;
    PluginInfoString_t _path;
    
    // Like "http://zynaddsubfx.sourceforge.net/fx#Phaser".
    PluginInfoString_t _uri;
    
    PluginType _type;
    PluginClass_t _class;
    unsigned long _uniqueID;
    long _subID; // vst shell ID etc.

    PluginInfoString_t _label;
    PluginInfoString_t _name;
    PluginInfoString_t _description;
    PluginInfoString_t _version;
    PluginInfoString_t _maker;
    PluginInfoString_t _copyright;

    int _apiVersionMajor;
    int _apiVersionMinor;

    int _pluginVersionMajor;
    int _pluginVersionMinor;

    PluginFlags_t _pluginFlags;
    
    unsigned long _portCount;
    unsigned long _inports;
    unsigned long _outports;
    unsigned long _controlInPorts;
    unsigned long _controlOutPorts;
    unsigned long _eventInPorts;
    unsigned long _eventOutPorts;

    // Freewheel port index ff HasFreewheelPort is true.
    unsigned long _freewheelPortIdx;
    // Latency port index if HasLatencyPort is true.
    unsigned long _latencyPortIdx;
    
    // Port number to control input index. Item is -1 if it's not a control input.
    // TODO: Not used yet.
    //std::vector<unsigned long> rpIdx;
    //std::vector<unsigned long> _pIdx; //control port numbers
    //std::vector<unsigned long> _poIdx; //control out port numbers
    //std::vector<unsigned long> _iIdx; //input port numbers
    //std::vector<unsigned long> _oIdx; //output port numbers

    MusECore::PluginFeatures_t _requiredFeatures;

    #ifdef VST_NATIVE_SUPPORT
    MusECore::VstPluginFlags_t _vstPluginFlags;
    #endif

    PluginInfoString_t _uiFilename;

    PluginPortList _portList;
    
    PortEnumValueMap _portEnumValMap;

  public:
    PluginScanInfoStruct() :
      _type(PluginTypeNone),
      _class(PluginClassNone),
      _uniqueID(0),
      _subID(0),
      _apiVersionMajor(0),
      _apiVersionMinor(0),
      _pluginVersionMajor(0),
      _pluginVersionMinor(0),
      _pluginFlags(NoPluginFlags),
      _portCount(0),
      _inports(0),
      _outports(0),
      _controlInPorts(0),
      _controlOutPorts(0),
      _eventInPorts(0),
      _eventOutPorts(0),
      _freewheelPortIdx(0),
      _latencyPortIdx(0),
      _requiredFeatures(MusECore::PluginNoFeatures)
      #ifdef VST_NATIVE_SUPPORT
      , _vstPluginFlags(MusECore::vstPluginNoFlags)
      #endif
      { };

    //~PluginScanInfoStruct();
      
#ifdef PLUGIN_INFO_USE_QT
      
#if defined(_WIN64) || defined(_WIN32)
    PluginInfoString_t filePath() const                     
      { const PluginInfoString_t fn = fileName(); return fn.isEmpty() ? _path : _path + '\\' + fn; }
#else
    PluginInfoString_t filePath() const                     
      { const PluginInfoString_t fn = fileName(); return fn.isEmpty() ? _path : _path + '/' + fn; }
#endif // defined(_WIN64) || defined(_WIN32)
    PluginInfoString_t fileName() const
      { return _completeSuffix.isEmpty() ? _baseName : _baseName + '.' + _completeSuffix; }
    PluginInfoString_t lib(bool complete = true) const      { return complete ? _completeBaseName : _baseName; }
    PluginInfoString_t dirPath(bool complete = true) const  { return complete ? _absolutePath : _path; }
    
#else // PLUGIN_INFO_USE_QT

#if defined(_WIN64) || defined(_WIN32)
    std::string filePath() const                     
      { const std::string fn = fileName(); return fn.empty() ? _path : _path + '\\' + fn; }
#else
    std::string filePath() const                     
      { const std::string fn = fileName(); return fn.empty() ? _path : _path + '/' + fn; }
#endif // defined(_WIN64) || defined(_WIN32)
    std::string fileName() const
      { return _completeSuffix.empty() ? _baseName : _baseName + '.' + _completeSuffix; }
    std::string lib(bool complete = true) const      { return complete ? _completeBaseName : _baseName; }
    std::string dirPath(bool complete = true) const  { return complete ? _absolutePath : _path; }
#endif // PLUGIN_INFO_USE_QT

    bool inPlaceCapable() const { return !(_requiredFeatures & MusECore::PluginNoInPlaceProcessing); }
      
    const char* typeString() const;
    const char* classString() const;
    
    void dump(const char* prefixMessage = 0) const;
};


//-----------------------------------------
// PluginScanInfo
//-----------------------------------------

class PluginScanInfo
{
  protected:
    PluginScanInfoStruct _info;
    
  public:
    PluginScanInfo() { };
    PluginScanInfo(const PluginScanInfoStruct& info) : _info(info) { };
    //~PluginScanInfo();
      
    const PluginScanInfoStruct& info() const { return _info; }
};


//-----------------------------------------
// functions
//-----------------------------------------

#ifdef PLUGIN_INFO_USE_QT

  #define PLUGIN_STRING_EMPTY(x) (x).isEmpty()

  #define PLUGIN_GET_CSTRING(x) (x).toLatin1().constData()
  #define PLUGIN_GET_STDSTRING(x) (x).toStdString()
  #define PLUGIN_GET_QSTRING(x) (x)

  #define PLUGIN_SET_CSTRING(x) QString(x)
  #define PLUGIN_SET_STDSTRING(x) QString::fromStdString(x)
  #define PLUGIN_SET_QSTRING(x) (x)

#else // PLUGIN_INFO_USE_QT

  #define PLUGIN_STRING_EMPTY(x) (x).empty()

  #define PLUGIN_GET_CSTRING(x) (x).c_str()
  #define PLUGIN_GET_STDSTRING(x) (x)
  #define PLUGIN_GET_QSTRING(x) QString::fromStdString(x)

  #define PLUGIN_SET_CSTRING(x) ((x) ? std::string(x) : std::string())
  #define PLUGIN_SET_STDSTRING(x) (x)
  #define PLUGIN_SET_QSTRING(x) (x).toStdString()

#endif // PLUGIN_INFO_USE_QT



} // namespace MusEPlugin

#endif

