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

#include "plugin_scan.h"
#include <cstring>

// For debugging output: Uncomment the fprintf section.
#define DEBUG_PLUGIN_SCAN(dev, format, args...) // std::fprintf(dev, format, ##args);

#ifdef PLUGIN_INFO_USE_QT
  #include <QtGlobal>
#else
  #ifndef QT_TRANSLATE_NOOP
    #define QT_TRANSLATE_NOOP(scope, x) x
  #endif
#endif // PLUGIN_INFO_USE_QT

namespace MusEPlugin {

//=======================================================

// NOTE: I tried to make a macro for the TRANSLATE_NOOP
//        which could be switched on or off depending
//        on PLUGIN_INFO_USE_QT, but it didn't work.
//       The Qt translator wants to see the macro right here.
//       So instead a different method was used, above.
const std::map<int, const char*> PluginTypeStringMap
{{PluginTypeNone, QT_TRANSLATE_NOOP("MusEPlugin", "<No Type>")},
 {PluginTypeLADSPA, QT_TRANSLATE_NOOP("MusEPlugin", "LADSPA")},
 {PluginTypeDSSI, QT_TRANSLATE_NOOP("MusEPlugin", "DSSI")},
 {PluginTypeVST, QT_TRANSLATE_NOOP("MusEPlugin", "VST")},
 {PluginTypeDSSIVST, QT_TRANSLATE_NOOP("MusEPlugin", "DSSI VST")},
 {PluginTypeLinuxVST, QT_TRANSLATE_NOOP("MusEPlugin", "Linux VST")},
 {PluginTypeLV2, QT_TRANSLATE_NOOP("MusEPlugin", "LV2")},
 {PluginTypeMESS, QT_TRANSLATE_NOOP("MusEPlugin", "MESS")},
 {PluginTypeMETRONOME, QT_TRANSLATE_NOOP("MusEPlugin", "Metronome")},
 {PluginTypeUnknown, QT_TRANSLATE_NOOP("MusEPlugin", "Unknown")}};

const char* pluginTypeToString(const MusEPlugin::PluginType type)
{
  std::map<int, const char*>::const_iterator i = PluginTypeStringMap.find(type);
  if(i == PluginTypeStringMap.cend())
    return PluginTypeStringMap.at(PluginTypeUnknown);

  return i->second;
}

MusEPlugin::PluginType pluginStringToType(const char *s)
{
  for(std::map<int, const char*>::const_iterator i = PluginTypeStringMap.cbegin();
      i != PluginTypeStringMap.cend(); ++i)
  {
    if(strcmp(i->second, s) == 0)
      return MusEPlugin::PluginType(i->first);
  }
  return MusEPlugin::PluginTypeUnknown;
}

const std::map<int, const char*> PluginClassStringMap
{{PluginClassNone, QT_TRANSLATE_NOOP("MusEPlugin", "<No Class>")},
 {PluginClassEffect, QT_TRANSLATE_NOOP("MusEPlugin", "Effect")},
 {PluginClassInstrument, QT_TRANSLATE_NOOP("MusEPlugin", "Synth")},
 {PluginClassEffect | PluginClassInstrument, QT_TRANSLATE_NOOP("MusEPlugin", "Effect/Synth")}};

const char* pluginClassToString(const MusEPlugin::PluginClass_t c)
{
  std::map<int, const char*>::const_iterator i = PluginClassStringMap.find(c);
  if(i == PluginClassStringMap.cend())
    return "Unknown";

  return i->second;
}

MusEPlugin::PluginClass_t pluginStringToClass(const char *s)
{
  for(std::map<int, const char*>::const_iterator i = PluginClassStringMap.cbegin();
      i != PluginClassStringMap.cend(); ++i)
  {
    if(strcmp(i->second, s) == 0)
      return MusEPlugin::PluginClass_t(i->first);
  }
  return MusEPlugin::PluginClassNone;
}

//=======================================================
PluginPortEnumValue::PluginPortEnumValue() { _value = 0.0; }
PluginPortEnumValue::PluginPortEnumValue(float value, PluginInfoString_t label)
    : _value(value), _label(label) { }


//=======================================================
const float PluginPortInfo::defaultPortValue = 0.0f;
const float PluginPortInfo::defaultPortMin = 0.0f;
const float PluginPortInfo::defaultPortMax = 1.0f;
const float PluginPortInfo::defaultPortStep = 0.0f;
const float PluginPortInfo::defaultPortSmallStep = 0.0f;
const float PluginPortInfo::defaultPortLargeStep = 0.0f;

PluginPortInfo::PluginPortInfo()
{
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

float PluginPortInfo::min(float sampleRate) const
{ return _flags & ScaleBySamplerate ? _min * sampleRate : _min; }
float PluginPortInfo::max(float sampleRate) const
{ return _flags & ScaleBySamplerate ? _max * sampleRate : _max;  }


//=======================================================
PluginScanInfoStruct::PluginScanInfoStruct() :
  _fileTime(0),
  _fileIsBad(false),
  _type(MusEPlugin::PluginTypeNone),
  _class(MusEPlugin::PluginClassNone),
  _uniqueID(0),
  _subID(0),
  _apiVersionMajor(0),
  _apiVersionMinor(0),
  _pluginVersionMajor(0),
  _pluginVersionMinor(0),
  _pluginFlags(MusEPlugin::PluginNoFlags),
  _portCount(0),
  _inports(0),
  _outports(0),
  _controlInPorts(0),
  _controlOutPorts(0),
  _eventInPorts(0),
  _eventOutPorts(0),
  _freewheelPortIdx(0),
  _latencyPortIdx(0),
  _enableOrBypassPortIdx(0),
  _pluginLatencyReportingType(MusEPlugin::PluginLatencyTypeNone),
  _pluginBypassType(MusEPlugin::PluginBypassTypeEmulatedEnableFunction),
  _pluginFreewheelType(MusEPlugin::PluginFreewheelTypeNone),
  _requiredFeatures(MusEPlugin::PluginNoFeatures),
  _vstPluginFlags(MusEPlugin::vstPluginNoFlags)
{ };

// PluginScanInfoStruct::~PluginScanInfoStruct()
// {
//   std::fprintf(stderr,
//     "PluginScanInfoStruct::~PluginScanInfoStruct: name:%s label:%s\n",
//     getCString(_name), getCString(_label));
// }

#ifdef PLUGIN_INFO_USE_QT

#if defined(_WIN64) || defined(_WIN32)
PluginInfoString_t PluginScanInfoStruct::filePath() const
  { const PluginInfoString_t fn = fileName(); return fn.isEmpty() ? _path : _path + '\\' + fn; }
#else
PluginInfoString_t PluginScanInfoStruct::filePath() const
{ const PluginInfoString_t fn = fileName(); return fn.isEmpty() ? _path : _path + '/' + fn; }
#endif // defined(_WIN64) || defined(_WIN32)

PluginInfoString_t PluginScanInfoStruct::fileName() const
{ return _completeSuffix.isEmpty() ? _baseName : _baseName + '.' + _completeSuffix; }
PluginInfoString_t PluginScanInfoStruct::lib(bool complete) const
{ return complete ? _completeBaseName : _baseName; }
PluginInfoString_t PluginScanInfoStruct::dirPath(bool complete) const
{ return complete ? _absolutePath : _path; }

#else // PLUGIN_INFO_USE_QT

#if defined(_WIN64) || defined(_WIN32)
std::string PluginScanInfoStruct::filePath() const
{ const std::string fn = fileName(); return fn.empty() ? _path : _path + '\\' + fn; }
#else
std::string PluginScanInfoStruct::filePath() const
{ const std::string fn = fileName(); return fn.empty() ? _path : _path + '/' + fn; }
#endif // defined(_WIN64) || defined(_WIN32)

std::string PluginScanInfoStruct::fileName() const
{ return _completeSuffix.empty() ? _baseName : _baseName + '.' + _completeSuffix; }
std::string PluginScanInfoStruct::lib(bool complete) const
{ return complete ? _completeBaseName : _baseName; }
std::string PluginScanInfoStruct::dirPath(bool complete) const
{ return complete ? _absolutePath : _path; }

#endif // PLUGIN_INFO_USE_QT

bool PluginScanInfoStruct::inPlaceCapable() const
{ return !(_requiredFeatures & MusEPlugin::PluginNoInPlaceProcessing); }

//=======================================================
PluginScanInfo::PluginScanInfo() { };
PluginScanInfo::PluginScanInfo(const PluginScanInfoStruct& info) : _info(info) { };

// PluginScanInfo::~PluginScanInfo()
// {
//   std::fprintf(stderr,
//     "PluginScanInfo::~PluginScanInfo: name:%s label:%s\n",
//     getCString(info()._name), getCString(info()._label));
// }

const PluginScanInfoStruct& PluginScanInfo::info() const { return _info; }

//=======================================================

} // namespace MusEPlugin
