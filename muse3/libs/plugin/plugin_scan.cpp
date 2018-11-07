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

// For debugging output: Uncomment the fprintf section.
#define DEBUG_PLUGIN_SCAN(dev, format, args...) // std::fprintf(dev, format, ##args);

namespace MusEPlugin {


const float PluginPortInfo::defaultPortValue = 0.0f;
const float PluginPortInfo::defaultPortMin = 0.0f;
const float PluginPortInfo::defaultPortMax = 1.0f;
const float PluginPortInfo::defaultPortStep = 0.0f;
const float PluginPortInfo::defaultPortSmallStep = 0.0f;
const float PluginPortInfo::defaultPortLargeStep = 0.0f;

// PluginScanInfoStruct::~PluginScanInfoStruct()
// {
//   std::fprintf(stderr, "PluginScanInfoStruct::~PluginScanInfoStruct: name:%s label:%s\n", getCString(_name), getCString(_label));
// }
//   
// PluginScanInfo::~PluginScanInfo()
// {
//   std::fprintf(stderr, "PluginScanInfo::~PluginScanInfo: name:%s label:%s\n", getCString(info()._name), getCString(info()._label));
// }
  
const char* PluginScanInfoStruct::typeString() const
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

    case PluginTypeNone:
    case PluginTypeAll:
    break;
  }
  return 0;
}

const char* PluginScanInfoStruct::classString() const
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

void PluginScanInfoStruct::dump(const char* prefixMessage) const
{
  fprintf(stderr, "%s plugin:%s type:%s class:%s name:%s label:%s required features:%d\n",
          prefixMessage,
          PLUGIN_GET_CSTRING(filePath()),
          typeString(),
          classString(),
          PLUGIN_GET_CSTRING(_name),
          PLUGIN_GET_CSTRING(_label),
          _requiredFeatures);
}

} // namespace MusEPlugin
