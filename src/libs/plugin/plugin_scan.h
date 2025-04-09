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
#include <cstdint>

#ifdef PLUGIN_INFO_USE_QT
  #include <QString>
  typedef QString PluginInfoString_t;
#else
  #include <string>
  typedef std::string PluginInfoString_t;
#endif // PLUGIN_INFO_USE_QT

#include "config.h"
// REMOVE Tim. tmp. Removed.
//#include "globaldefs.h"

//==============================================
// NOTICE:
// All strings here are UTF8.
// It may not be possible to know what encoding
//  was used when the plugin was made.
// We can only assume, and hope, that it was UTF8
//  which is the most sensible encoding.
// Even if a plugin's strings are known to be
//  some other encoding, for example UTF16, and
//  regardless of the string type QString or std::string,
//  the string MUST be stored as UTF8.
//==============================================

namespace MusEPlugin {

const char* const VST_OLD_PLUGIN_ENTRY_POINT = "main";
const char* const VST_NEW_PLUGIN_ENTRY_POINT = "VSTPluginMain";
  
//==============================================
// NOTE: Strings representing the following plugin
//  types and classes can be found in plugin.h
// Please ensure synchronization between them and
//  these enumerations.
//==============================================

// // Can be Or'd together.
// enum PluginType {
//   PluginTypeNone     = 0x00,
//   PluginTypeLADSPA   = 0x01,
//   PluginTypeDSSI     = 0x02,
//   PluginTypeVST      = 0x04,
//   PluginTypeDSSIVST  = 0x08,
//   PluginTypeLinuxVST = 0x10,
//   PluginTypeLV2      = 0x20,
//   PluginTypeMESS     = 0x40,
//   // Built-in metronome is not actually a loadable plugin, but is a synth.
//   PluginTypeMETRONOME = 0x80,
//   PluginTypeUnknown = 0x8000,
//   PluginTypeAll = PluginTypeLADSPA   | PluginTypeDSSI |
//                   PluginTypeVST      | PluginTypeDSSIVST |
//                   PluginTypeLinuxVST | PluginTypeLV2 |
//                   PluginTypeMESS     | PluginTypeUnknown |
//                   PluginTypeMETRONOME };
// typedef int PluginType_t;

// Can be Or'd together.
enum PluginType {
  PluginTypeNone      = 0x00,
  PluginTypeLADSPA    = 0x01,
  PluginTypeDSSI      = 0x02,
  PluginTypeVST       = 0x04,
  PluginTypeDSSIVST   = 0x08,
  PluginTypeLinuxVST  = 0x10,
  PluginTypeLV2       = 0x20,
  PluginTypeMESS      = 0x40,
  // Built-in metronome is not actually a loadable plugin, but is a synth.
  PluginTypeMETRONOME = 0x80,
  PluginTypeUnknown   = 0x8000 };

// Can be Or'd together, along with PluginType.
enum PluginTypes {
  PluginTypesAll = PluginTypeLADSPA   | PluginTypeDSSI |
                   PluginTypeVST      | PluginTypeDSSIVST |
                   PluginTypeLinuxVST | PluginTypeLV2 |
                   PluginTypeMESS     | PluginTypeUnknown |
                   PluginTypeMETRONOME };
typedef int PluginTypes_t;

// Can be Or'd together.
enum PluginClass {
  PluginClassNone = 0x00,
  PluginClassEffect = 0x01, PluginClassInstrument = 0x02,
  PluginClassAll = PluginClassEffect | PluginClassInstrument };
typedef int PluginClass_t;

// Can be Or'd together.
enum PluginFlags {
  PluginNoFlags = 0x00,
  PluginHasGui = 0x01,
  PluginHasChunks = 0x02,
  PluginIsRealtime = 0x04,
  PluginIsHardRealtimeCapable = 0x08,
  // Obsolete flag. Kept for backward compatibility.
  PluginHasFreewheelPort = 0x10,
  // Obsolete flag. Kept for backward compatibility.
  PluginHasLatencyPort = 0x20,
  PluginSupportsTimePosition = 0x40 };
typedef int PluginFlags_t;

// Can be Or'd together.
enum PluginFeature {
  PluginNoFeatures = 0x00,
  PluginFixedBlockSize = 0x01,
  PluginPowerOf2BlockSize = 0x02,
  PluginNoInPlaceProcessing = 0x04,
  PluginCoarseBlockSize = 0x08,
  PluginSupportStrictBounds = 0x10
};
typedef int PluginFeatures_t;

enum PluginBypassType {
  // The plugin has no bypass or enable feature.
  // We emulate an enable function with no controller graph.
  PluginBypassTypeEmulatedEnableFunction = 0,
  // The plugin has no bypass or enable feature.
  // We emulate an enable function with controller graph.
  PluginBypassTypeEmulatedEnableController,
  // The plugin has an enable function.
  // We provide an enable function with no controller graph.
  PluginBypassTypeEnableFunction,
  // The plugin has an enable controller port.
  // We provide an enable controller graph.
  PluginBypassTypeEnablePort,
  // The plugin has a bypass function.
  // We provide an (inverted) enable function with no controller graph.
  PluginBypassTypeBypassFunction,
  // The plugin has a bypass controller port.
  // We provide a bypass controller graph.
  PluginBypassTypeBypassPort
};

enum PluginLatencyReportingType {
  // Plugin has no latency reporting mechanism.
  PluginLatencyTypeNone = 0,
  // Plugin has a latency reporting function but no controller port.
  PluginLatencyTypeFunction,
  // Plugin has a latency reporting controller port.
  PluginLatencyTypePort
};

enum PluginFreewheelType {
  // Plugin has no freewheel mechanism.
  PluginFreewheelTypeNone = 0,
  // Plugin has a freewheel function but no controller port.
  PluginFreewheelTypeFunction,
  // Plugin has a freewheel controller port.
  PluginFreewheelTypePort
};

enum VstPluginFlags
{
  vstPluginNoFlags          = 0,
  canSendVstEvents          = 1 << 0,
  canSendVstMidiEvents      = 1 << 1,
  canReceiveVstEvents       = 1 << 3,
  canReceiveVstMidiEvents   = 1 << 4,
  canReceiveVstTimeInfo     = 1 << 5,
  canProcessVstOffline      = 1 << 6,
  canVstMidiProgramNames    = 1 << 10,
  canVstBypass              = 1 << 11
};
typedef int VstPluginFlags_t;

enum VstPluginToHostFlags
{
  vstHostNoFlags                          = 0,
  canHostSendVstEvents                    = 1 << 0,
  canHostSendVstMidiEvents                = 1 << 1,
  canHostSendVstTimeInfo                  = 1 << 2,
  canHostReceiveVstEvents                 = 1 << 3,
  canHostReceiveVstMidiEvents             = 1 << 4,
  canHostVstReportConnectionChanges       = 1 << 5,
  canHostVstAcceptIOChanges               = 1 << 6,
  canHostVstSizeWindow                    = 1 << 7,
  canHostVstOffline                       = 1 << 8,
  canHostVstOpenFileSelector              = 1 << 9,
  canHostVstCloseFileSelector             = 1 << 10,
  canHostVstStartStopProcess              = 1 << 11,
  canHostVstShellCategory                 = 1 << 12,
  canHostSendVstMidiEventFlagIsRealtime   = 1 << 13
};
typedef int VstPluginToHostFlags_t;


//-----------------------------------------
// PluginPortEnumValue
//-----------------------------------------

struct PluginPortEnumValue
{
  float _value;
  PluginInfoString_t _label;
  PluginPortEnumValue();
  PluginPortEnumValue(float value, PluginInfoString_t label);
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
    HasStep = 0x100,
    HasStrictBounds = 0x200 };
  typedef int PortValueFlags_t;

  enum PortFlags { NoPortFlags = 0x00,
    // Indicates min and max should be scaled by the current samplerate.
    ScaleBySamplerate = 0x01,
    // Whether the port is intended for latency reporting.
    IsLatency = 0x02,
    // Whether the port is an audio port which is used for high-speed control signals.
    IsCVPort = 0x04,
    SupportsTimePosition = 0x08,
    IsFreewheel = 0x10,
    // Whether the port is designated an enable port.
    IsEnable = 0x20,
    // Whether the port is designated a bypass port.
    IsBypass = 0x20
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
  
  PluginPortInfo();
  
  float min(float sampleRate) const;
  float max(float sampleRate) const;
};

typedef std::vector<PluginPortInfo> PluginPortList;
typedef PluginPortList::iterator iPluginPortList;
typedef PluginPortList::const_iterator ciPluginPortList;


//-----------------------------------------
// PluginScanInfoStruct
//-----------------------------------------

// REMOVE Tim. tmp. Changed.
// class PluginScanInfoStruct
// {
//    public:
//     enum PluginType { PluginTypeNone = 0x00,
//       PluginTypeLADSPA   = 0x01,  PluginTypeDSSI    = 0x02,
//       PluginTypeVST      = 0x04,  PluginTypeDSSIVST = 0x08,
//       PluginTypeLinuxVST = 0x10,  PluginTypeLV2     = 0x20,
//       PluginTypeMESS     = 0x40,  PluginTypeUnknown = 0x8000,
//       PluginTypeAll = PluginTypeLADSPA   | PluginTypeDSSI |
//                       PluginTypeVST      | PluginTypeDSSIVST |
//                       PluginTypeLinuxVST | PluginTypeLV2 |
//                       PluginTypeMESS     | PluginTypeUnknown};
//     typedef int PluginType_t;
//
//     enum PluginClass { PluginClassNone = 0x00,
//       PluginClassEffect = 0x01, PluginClassInstrument = 0x02,
//       PluginClassAll = PluginClassEffect | PluginClassInstrument };
//     typedef int PluginClass_t;
//
//     enum PluginFlags { NoPluginFlags = 0x00,
//       PluginHasGui = 0x01, PluginHasChunks = 0x02, Realtime = 0x04, PluginIsHardRealtimeCapable = 0x08,
//       // Obsolete flag. Kept for backward compatibility.
//       PluginHasFreewheelPort = 0x10,
//       // Obsolete flag. Kept for backward compatibility.
//       PluginHasLatencyPort = 0x20,
//       PluginSupportsTimePosition = 0x40 };
//     typedef int PluginFlags_t;
//
//   //private:
//   //protected:
//   public:
//
//     //QFileInfo _fi;
//     PluginInfoString_t _completeBaseName;
//     PluginInfoString_t _baseName;
//     PluginInfoString_t _suffix;
//     PluginInfoString_t _completeSuffix;
//     PluginInfoString_t _absolutePath;
//     PluginInfoString_t _path;
//
//     // Like "http://zynaddsubfx.sourceforge.net/fx#Phaser".
//     PluginInfoString_t _uri;
//
//     // The file's time stamp in milliseconds since epoch.
//     int64_t _fileTime;
//     // Whether the file failed scanning.
//     bool _fileIsBad;
//
//     PluginType _type;
//     PluginClass_t _class;
//     unsigned long _uniqueID;
//     long _subID; // vst shell ID etc.
//
//     PluginInfoString_t _label;
//     PluginInfoString_t _name;
//     PluginInfoString_t _description;
//     PluginInfoString_t _version;
//     PluginInfoString_t _maker;
//     PluginInfoString_t _copyright;
//
//     int _apiVersionMajor;
//     int _apiVersionMinor;
//
//     int _pluginVersionMajor;
//     int _pluginVersionMinor;
//
//     MusEPlugin::PluginFlags_t _pluginFlags;
//
//     unsigned long _portCount;
//     unsigned long _inports;
//     unsigned long _outports;
//     unsigned long _controlInPorts;
//     unsigned long _controlOutPorts;
//     unsigned long _eventInPorts;
//     unsigned long _eventOutPorts;
//
//     // Freewheel port index if PluginHasFreewheelPort is true.
//     unsigned long _freewheelPortIdx;
//     // Latency port index if PluginHasLatencyPort is true.
//     unsigned long _latencyPortIdx;
//     // Enable or bypass port index if UseEmulatedEnableController or HasEnablePort or HasBypassPort are true.
//     unsigned long _enableOrBypassPortIdx;
//     MusEPlugin::PluginLatencyReportingType _pluginLatencyReportingType;
//     MusEPlugin::PluginBypassType _pluginBypassType;
//     MusEPlugin::PluginFreewheelType _pluginFreewheelType;
//
//     // Port number to control input index. Item is -1 if it's not a control input.
//     // TODO: Not used yet.
//     //std::vector<unsigned long> rpIdx;
//     //std::vector<unsigned long> _pIdx; //control port numbers
//     //std::vector<unsigned long> _poIdx; //control out port numbers
//     //std::vector<unsigned long> _iIdx; //input port numbers
//     //std::vector<unsigned long> _oIdx; //output port numbers
//
//     MusEPlugin::PluginFeatures_t _requiredFeatures;
//     MusEPlugin::VstPluginFlags_t _vstPluginFlags;
//
//     PluginInfoString_t _uiFilename;
//
//     PluginPortList _portList;
//
//     PortEnumValueMap _portEnumValMap;
//
//   public:
//     PluginScanInfoStruct() :
//       _fileTime(0),
//       _fileIsBad(false),
//       _type(MusEPlugin::PluginTypeNone),
//       _class(MusEPlugin::PluginClassNone),
//       _uniqueID(0),
//       _subID(0),
//       _apiVersionMajor(0),
//       _apiVersionMinor(0),
//       _pluginVersionMajor(0),
//       _pluginVersionMinor(0),
//       _pluginFlags(MusEPlugin::NoPluginFlags),
//       _portCount(0),
//       _inports(0),
//       _outports(0),
//       _controlInPorts(0),
//       _controlOutPorts(0),
//       _eventInPorts(0),
//       _eventOutPorts(0),
//       _freewheelPortIdx(0),
//       _latencyPortIdx(0),
//       _enableOrBypassPortIdx(0),
//       _pluginLatencyReportingType(MusEPlugin::PluginLatencyTypeNone),
//       _pluginBypassType(MusEPlugin::PluginBypassTypeEmulatedEnableFunction),
//       _pluginFreewheelType(MusEPlugin::PluginFreewheelTypeNone),
//       _requiredFeatures(MusEPlugin::PluginNoFeatures),
//       _vstPluginFlags(MusEPlugin::vstPluginNoFlags)
//       { };
//
//     //~PluginScanInfoStruct();
//
// #ifdef PLUGIN_INFO_USE_QT
//
// #if defined(_WIN64) || defined(_WIN32)
//     PluginInfoString_t filePath() const
//       { const PluginInfoString_t fn = fileName(); return fn.isEmpty() ? _path : _path + '\\' + fn; }
// #else
//     PluginInfoString_t filePath() const
//       { const PluginInfoString_t fn = fileName(); return fn.isEmpty() ? _path : _path + '/' + fn; }
// #endif // defined(_WIN64) || defined(_WIN32)
//     PluginInfoString_t fileName() const
//       { return _completeSuffix.isEmpty() ? _baseName : _baseName + '.' + _completeSuffix; }
//     PluginInfoString_t lib(bool complete = true) const      { return complete ? _completeBaseName : _baseName; }
//     PluginInfoString_t dirPath(bool complete = true) const  { return complete ? _absolutePath : _path; }
//
// #else // PLUGIN_INFO_USE_QT
//
// #if defined(_WIN64) || defined(_WIN32)
//     std::string filePath() const
//       { const std::string fn = fileName(); return fn.empty() ? _path : _path + '\\' + fn; }
// #else
//     std::string filePath() const
//       { const std::string fn = fileName(); return fn.empty() ? _path : _path + '/' + fn; }
// #endif // defined(_WIN64) || defined(_WIN32)
//     std::string fileName() const
//       { return _completeSuffix.empty() ? _baseName : _baseName + '.' + _completeSuffix; }
//     std::string lib(bool complete = true) const      { return complete ? _completeBaseName : _baseName; }
//     std::string dirPath(bool complete = true) const  { return complete ? _absolutePath : _path; }
// #endif // PLUGIN_INFO_USE_QT
//
//     bool inPlaceCapable() const { return !(_requiredFeatures & MusEPlugin::PluginNoInPlaceProcessing); }
//
//     const char* typeString() const;
//     const char* classString() const;
//
//     void dump(const char* prefixMessage = 0) const;
// };
class PluginScanInfoStruct
{
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

    // The file's time stamp in milliseconds since epoch.
    int64_t _fileTime;
    // Whether the file failed scanning.
    bool _fileIsBad;

    MusEPlugin::PluginType _type;
    MusEPlugin::PluginClass_t _class;
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

    MusEPlugin::PluginFlags_t _pluginFlags;

    unsigned long _portCount;
    unsigned long _inports;
    unsigned long _outports;
    unsigned long _controlInPorts;
    unsigned long _controlOutPorts;
    unsigned long _eventInPorts;
    unsigned long _eventOutPorts;

    // Freewheel port index if PluginHasFreewheelPort is true.
    unsigned long _freewheelPortIdx;
    // Latency port index if PluginHasLatencyPort is true.
    unsigned long _latencyPortIdx;
    // Enable or bypass port index if UseEmulatedEnableController or HasEnablePort or HasBypassPort are true.
    unsigned long _enableOrBypassPortIdx;
    MusEPlugin::PluginLatencyReportingType _pluginLatencyReportingType;
    MusEPlugin::PluginBypassType _pluginBypassType;
    MusEPlugin::PluginFreewheelType _pluginFreewheelType;

    // Port number to control input index. Item is -1 if it's not a control input.
    // TODO: Not used yet.
    //std::vector<unsigned long> rpIdx;
    //std::vector<unsigned long> _pIdx; //control port numbers
    //std::vector<unsigned long> _poIdx; //control out port numbers
    //std::vector<unsigned long> _iIdx; //input port numbers
    //std::vector<unsigned long> _oIdx; //output port numbers

    MusEPlugin::PluginFeatures_t _requiredFeatures;
    MusEPlugin::VstPluginFlags_t _vstPluginFlags;

    PluginInfoString_t _uiFilename;

    PluginPortList _portList;

    PortEnumValueMap _portEnumValMap;

  public:
    PluginScanInfoStruct();
    //~PluginScanInfoStruct();

#ifdef PLUGIN_INFO_USE_QT

#if defined(_WIN64) || defined(_WIN32)
    PluginInfoString_t filePath() const;
#else
    PluginInfoString_t filePath() const;
#endif // defined(_WIN64) || defined(_WIN32)

    PluginInfoString_t fileName() const;
    PluginInfoString_t lib(bool complete = true) const;
    PluginInfoString_t dirPath(bool complete = true) const;

#else // PLUGIN_INFO_USE_QT

#if defined(_WIN64) || defined(_WIN32)
    std::string filePath() const;
#else
    std::string filePath() const;
#endif // defined(_WIN64) || defined(_WIN32)

    std::string fileName() const;
    std::string lib(bool complete = true) const;
    std::string dirPath(bool complete = true) const;

#endif // PLUGIN_INFO_USE_QT

    bool inPlaceCapable() const;
// REMOVE Tim. tmp. Removed.
//     void dump(const char* prefixMessage = 0) const;
};

//-----------------------------------------
// PluginScanInfo
//-----------------------------------------

class PluginScanInfo
{
  protected:
    PluginScanInfoStruct _info;
    
  public:
    PluginScanInfo();
    PluginScanInfo(const PluginScanInfoStruct& info);
    //~PluginScanInfo();
      
    const PluginScanInfoStruct& info() const;
};


//-----------------------------------------
// functions
//-----------------------------------------

#ifdef PLUGIN_INFO_USE_QT

  #define PLUGIN_STRING_EMPTY(x) (x).isEmpty()

// REMOVE Tim. tmp. Removed.
//   #define PLUGIN_GET_CSTRING(x) (x).toUtf8().constData()
  #define PLUGIN_GET_STDSTRING(x) (x).toStdString()
  #define PLUGIN_GET_QSTRING(x) (x)

// REMOVE Tim. tmp. Changed.
//   #define PLUGIN_SET_CSTRING(x) QString(x)
  #define PLUGIN_SET_CSTRING(x) QString::fromUtf8(x)
  #define PLUGIN_SET_STDSTRING(x) QString::fromStdString(x)
  #define PLUGIN_SET_QSTRING(x) (x)

#else // PLUGIN_INFO_USE_QT

  #define PLUGIN_STRING_EMPTY(x) (x).empty()

// REMOVE Tim. tmp. Removed.
//   #define PLUGIN_GET_CSTRING(x) (x).c_str()
  #define PLUGIN_GET_STDSTRING(x) (x)
  #define PLUGIN_GET_QSTRING(x) QString::fromStdString(x)

  #define PLUGIN_SET_CSTRING(x) ((x) ? std::string(x) : std::string())
  #define PLUGIN_SET_STDSTRING(x) (x)
  #define PLUGIN_SET_QSTRING(x) (x).toStdString()

#endif // PLUGIN_INFO_USE_QT

//==============================================
// These are the permanent strings representing
//  the plugin types and classes.
// They are NOT to be translated since they are
//  used for storage in song files.
// Translated versions are available in plugin.h
//==============================================
extern const std::map<int, const char*> PluginTypeStringMap;
extern const char* pluginTypeToString(const MusEPlugin::PluginType type);
extern MusEPlugin::PluginType pluginStringToType(const char *s);

extern const std::map<int, const char*> PluginClassStringMap;
extern const char* pluginClassToString(const MusEPlugin::PluginClass_t c);
extern MusEPlugin::PluginClass_t pluginStringToClass(const char *s);

} // namespace MusEPlugin

#endif

