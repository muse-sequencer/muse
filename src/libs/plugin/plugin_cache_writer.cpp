//=========================================================
//  MusE
//  Linux Music Editor
//
//  plugin_cache_writer.cpp
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

#include <QMessageBox>
#include <QDir>
#include <QTemporaryFile>
#include <QDateTime>
#include <QFile>
#include <QFileInfo>
//#include <QFileInfoList>
#include <QFileDevice>
#include <QProcess>
#include <QByteArray>
//#include <QByteArrayList>
#include <QStringList>
#include <sys/stat.h>

#include <map>

#include <cstdio>
#include <cstring>
//#include <cstdint>
#include "muse_math.h"

//#include "plugin_rdf.h"
#include "plugin_cache_writer.h"
#include "plugin_cache_reader.h"

#ifdef HAVE_LRDF
  #include <lrdf.h>
#endif // HAVE_LRDF


#ifdef LV2_SUPPORT

#include <set>
#include <vector>

// Disable warnings for parentheses. Did not work!
// #if defined(__clang__)
// #    pragma clang diagnostic push
// #    pragma clang diagnostic ignored "-Wparentheses"
// #elif __GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 6)
// #    pragma GCC diagnostic push
// #    pragma GCC diagnostic warning "-Wparentheses"
// #endif

// #include "lilv/lilv.h"

// #if defined(__clang__)
// #    pragma clang diagnostic pop
// #elif __GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 6)
// #    pragma GCC diagnostic pop
// #endif

#include "lv2/lv2plug.in/ns/ext/data-access/data-access.h"
#include "lv2/lv2plug.in/ns/ext/state/state.h"
//#include "lv2/lv2plug.in/ns/ext/atom/atom.h"
//#include "lv2/lv2plug.in/ns/ext/midi/midi.h"
#include "lv2/lv2plug.in/ns/ext/buf-size/buf-size.h"
#include "lv2/lv2plug.in/ns/ext/event/event.h"
#include "lv2/lv2plug.in/ns/ext/options/options.h"
#include "lv2/lv2plug.in/ns/ext/parameters/parameters.h"
//#include "lv2/lv2plug.in/ns/ext/patch/patch.h"
//#include "lv2/lv2plug.in/ns/ext/port-groups/port-groups.h"
#include "lv2/lv2plug.in/ns/ext/presets/presets.h"
//#include "lv2/lv2plug.in/ns/ext/state/state.h"
#include "lv2/lv2plug.in/ns/ext/time/time.h"
#include "lv2/lv2plug.in/ns/ext/uri-map/uri-map.h"
#include "lv2/lv2plug.in/ns/ext/urid/urid.h"
#include "lv2/lv2plug.in/ns/ext/worker/worker.h"
#include "lv2/lv2plug.in/ns/ext/port-props/port-props.h"
#include "lv2/lv2plug.in/ns/ext/atom/forge.h"
#include "lv2/lv2plug.in/ns/ext/log/log.h"
#include "lv2/lv2plug.in/ns/extensions/ui/ui.h"
//#include "lv2/lv2plug.in/ns/ext/dynmanifest/dynmanifest.h"
#include "lv2extui.h"
#include "lv2extprg.h"

//#include <sord/sord.h>

#endif // LV2_SUPPORT

// Forwards from header:
#include "xml.h"

// For debugging output: Uncomment the fprintf section.
#define DEBUG_PLUGIN_SCAN(dev, format, args...)  // std::fprintf(dev, format, ##args);

//#define PORTS_ARE_SINGLE_LINE_TAGS 1

namespace MusEPlugin {

bool scanLadspaPorts(
  const LADSPA_Descriptor* ladspa_descr,
  PluginScanInfoStruct* info,
  bool
#ifdef HAVE_LRDF
  do_rdf
#endif
)
{
  info->_portCount = ladspa_descr->PortCount;
  unsigned long ip = 0, op = 0, cip = 0, cop = 0;
  for(unsigned long k = 0; k < ladspa_descr->PortCount; ++k)
  {
    PluginPortInfo port_info;
    port_info._name = PLUGIN_SET_CSTRING(ladspa_descr->PortNames[k]);
    port_info._index = k;

    const LADSPA_PortRangeHint range_hint = ladspa_descr->PortRangeHints[k];
    LADSPA_PortRangeHintDescriptor range_hint_desc = range_hint.HintDescriptor;

//     if(LADSPA_IS_HINT_LOGARITHMIC(range_hint_desc))
//       port_info._valueFlags = PluginPortInfo::LogVal;
//     else if(LADSPA_IS_HINT_TOGGLED(range_hint_desc))
//       port_info._valueFlags = PluginPortInfo::ToggledVal;
//     else if(LADSPA_IS_HINT_INTEGER(range_hint_desc))
//       port_info._valueFlags = PluginPortInfo::IntegerVal;
//     else
//       //if(!LADSPA_IS_HINT_INTEGER(range_hint_desc) &&
//       //        !LADSPA_IS_HINT_LOGARITHMIC(range_hint_desc) &&
//       //        !LADSPA_IS_HINT_TOGGLED(range_hint_desc))
//       port_info._valueType = PluginPortInfo::LinearVal;


    if(LADSPA_IS_HINT_LOGARITHMIC(range_hint_desc))
      port_info._valueFlags |= PluginPortInfo::LogVal;

    if(LADSPA_IS_HINT_TOGGLED(range_hint_desc))
      port_info._valueFlags |= PluginPortInfo::ToggledVal;

    if(LADSPA_IS_HINT_INTEGER(range_hint_desc))
      port_info._valueFlags |= PluginPortInfo::IntegerVal;

//     if(LADSPA_IS_HINT_TOGGLED(range_hint_desc))
//     {
//       port_info._min = 0.0f;
//       port_info._max = 1.0f;
//       port_info._valueFlags |= PluginPortInfo::HasMin;
//       port_info._valueFlags |= PluginPortInfo::HasMax;
//     }
//     else
    {
      if(LADSPA_IS_HINT_SAMPLE_RATE(range_hint_desc))
        port_info._flags |= PluginPortInfo::ScaleBySamplerate;

      if(LADSPA_IS_HINT_BOUNDED_BELOW(range_hint_desc))
      {
        port_info._min =  range_hint.LowerBound; // * m;
        port_info._valueFlags |= PluginPortInfo::HasMin;
      }
      else
      {
        //port_info._min = 0.0f;
      }

      if(LADSPA_IS_HINT_BOUNDED_ABOVE(range_hint_desc))
      {
        port_info._max =  range_hint.UpperBound; // * m;
        //port_info._flags |= PluginPortInfo::BoundedAbove;
        port_info._valueFlags |= PluginPortInfo::HasMax;
      }
      else
      {
        //port_info._max = 1.0f;
      }
    }

    if(LADSPA_IS_HINT_HAS_DEFAULT(range_hint_desc))
      port_info._valueFlags |= PluginPortInfo::HasDefault;

    LADSPA_Data default_val = PluginPortInfo::defaultPortValue;
    if(LADSPA_IS_HINT_DEFAULT_MINIMUM(range_hint_desc))
      default_val = range_hint.LowerBound;
    else if(LADSPA_IS_HINT_DEFAULT_MAXIMUM(range_hint_desc))
      default_val = range_hint.UpperBound;
    else if(LADSPA_IS_HINT_DEFAULT_LOW(range_hint_desc))
    {
      if(LADSPA_IS_HINT_LOGARITHMIC(range_hint_desc))
        default_val = std::exp(std::log(range_hint.LowerBound) * .75f +
                               std::log(range_hint.UpperBound) * .25f);
      else
        default_val = range_hint.LowerBound*.75f + range_hint.UpperBound*.25f;
    }
    else if(LADSPA_IS_HINT_DEFAULT_MIDDLE(range_hint_desc))
    {
      if(LADSPA_IS_HINT_LOGARITHMIC(range_hint_desc))
        default_val = std::exp(std::log(range_hint.LowerBound) * .5f +
                               std::log(range_hint.UpperBound) * .5f);
      else
        default_val = range_hint.LowerBound*.5f + range_hint.UpperBound*.5f;
    }
    else if(LADSPA_IS_HINT_DEFAULT_HIGH(range_hint_desc))
    {
      if(LADSPA_IS_HINT_LOGARITHMIC(range_hint_desc))
        default_val = std::exp(std::log(range_hint.LowerBound) * .25f +
                               std::log(range_hint.UpperBound) * .75f);
      else
        default_val = range_hint.LowerBound*.25f + range_hint.UpperBound*.75f;
    }
    else if(LADSPA_IS_HINT_DEFAULT_0(range_hint_desc))
      default_val = 0.0f;
    else if(LADSPA_IS_HINT_DEFAULT_1(range_hint_desc))
      default_val = 1.0f;
    else if(LADSPA_IS_HINT_DEFAULT_100(range_hint_desc))
      default_val = 100.0f;
    else if(LADSPA_IS_HINT_DEFAULT_440(range_hint_desc))
      default_val = 440.0f;
      // No default found. Make one up...
    else if(LADSPA_IS_HINT_BOUNDED_BELOW(range_hint_desc) && LADSPA_IS_HINT_BOUNDED_ABOVE(range_hint_desc))
    {
      if(LADSPA_IS_HINT_LOGARITHMIC(range_hint_desc))
        default_val = std::exp(std::log(range_hint.LowerBound) * .5f +
                               std::log(range_hint.UpperBound) * .5f);
      else
        default_val = range_hint.LowerBound*.5f + range_hint.UpperBound*.5f;
    }
    else if(LADSPA_IS_HINT_BOUNDED_BELOW(range_hint_desc))
      default_val = range_hint.LowerBound;
    else if(LADSPA_IS_HINT_BOUNDED_ABOVE(range_hint_desc))
    {
      // Hm. What to do here... Just try 0.0 or the upper bound if less than zero.
      //if(range.UpperBound > 0.0f)
      //  default_val = 0.0f;
      //else
      //  default_val = range.UpperBound;
      // Instead try this: Adopt an 'attenuator-like' policy, where upper is the default.
      default_val = range_hint.UpperBound;
    }
    port_info._defaultVal = default_val;

    const LADSPA_PortDescriptor pd = ladspa_descr->PortDescriptors[k];
    if(pd & LADSPA_PORT_AUDIO)
    {
      port_info._type = PluginPortInfo::AudioPort;
      if(pd & LADSPA_PORT_INPUT)
      {
        port_info._type |= PluginPortInfo::InputPort;
      }
      else
      if(pd & LADSPA_PORT_OUTPUT)
      {
        port_info._type |= PluginPortInfo::OutputPort;
        ++op;
      }
    }
    else
    if(pd & LADSPA_PORT_CONTROL)
    {
      port_info._type = PluginPortInfo::ControlPort;
      if(pd & LADSPA_PORT_INPUT)
      {
        port_info._type |= PluginPortInfo::InputPort;
        ++cip;
      }
      else
      if(pd & LADSPA_PORT_OUTPUT)
      {
        port_info._type |= PluginPortInfo::OutputPort;
        ++cop;
        const QString pname = PLUGIN_SET_CSTRING(ladspa_descr->PortNames[k]);
        if(pname == QString("latency") || pname == QString("_latency"))
        {
          info->_pluginFlags |= PluginScanInfoStruct::HasLatencyPort;
          info->_latencyPortIdx = k;
        }
      }
    }

#ifdef HAVE_LRDF

    if(do_rdf)
    {
      lrdf_defaults *defs;
      defs = lrdf_get_scale_values(info->_uniqueID, k);
      if(defs)
      {
        // Map for ensuring the enumerations are sorted by increasing value.
        std::map<float, PluginPortEnumValue, std::less<float> > enum_list;
        for(unsigned int i = 0; defs && i < defs->count; i++)
        {
          DEBUG_PLUGIN_SCAN(stderr, "%f = '%s'\n", defs->items[i].value, defs->items[i].label);
          PluginPortEnumValue enum_val(defs->items[i].value, PLUGIN_SET_CSTRING(defs->items[i].label));
          enum_list.insert(std::pair<float, PluginPortEnumValue>(enum_val._value, enum_val));
        }
        lrdf_free_setting_values(defs);

        // Copy the sorted enumeration values to the vector enumeration list.
        EnumValueList dst_list;
        for(std::map<float, PluginPortEnumValue, std::less<float>>::const_iterator iel = enum_list.begin();
            iel != enum_list.end(); ++iel)
        {
          dst_list.push_back(iel->second);
        }
        if(!dst_list.empty())
        {
          info->_portEnumValMap.insert(PortEnumValueMapPair(k, dst_list));
          port_info._valueFlags |= PluginPortInfo::HasEnumerations;
        }
      }

// Yikes. A bit too much information to store. Maybe let's skip this and
//  only read presets when the user actually wants to load one?
//
//       lrdf_uris * set_uris;
//       set_uris = lrdf_get_setting_uris(info->_uniqueID);
//       if(set_uris)
//       {
//         for(unsigned int i = 0; i < set_uris->count; ++i)
//         {
//           defs = lrdf_get_setting_values(set_uris->items[i]);
//
//
//           lrdf_free_setting_values(defs);
//         }
//       }

    }

#endif // HAVE_LRDF

    info->_portList.push_back(port_info);
  }

  info->_inports = ip;
  info->_outports = op;
  info->_controlInPorts = cip;
  info->_controlOutPorts = cop;

  if((info->_inports != info->_outports) || LADSPA_IS_INPLACE_BROKEN(ladspa_descr->Properties))
    info->_requiredFeatures |= MusECore::PluginNoInPlaceProcessing;

  return true;
}

bool scanLadspaDescriptor(
  const char* filename,
  const LADSPA_Descriptor* ladspa_descr,
  PluginScanInfoStruct* info,
  bool do_ports,
  bool do_rdf)
{
  setPluginScanFileInfo(filename, info);
  info->_type = PluginScanInfoStruct::PluginTypeLADSPA;
  info->_class = PluginScanInfoStruct::PluginClassEffect;
  info->_uniqueID = ladspa_descr->UniqueID;
  info->_label = PLUGIN_SET_CSTRING(ladspa_descr->Label);
  info->_name = PLUGIN_SET_CSTRING(ladspa_descr->Name);
  info->_maker = PLUGIN_SET_CSTRING(ladspa_descr->Maker);
  info->_copyright = PLUGIN_SET_CSTRING(ladspa_descr->Copyright);

  if(LADSPA_IS_REALTIME(ladspa_descr->Properties))
    info->_pluginFlags |= PluginScanInfoStruct::Realtime;

  if(LADSPA_IS_HARD_RT_CAPABLE(ladspa_descr->Properties))
    info->_pluginFlags |= PluginScanInfoStruct::HardRealtimeCapable;

//   DEBUG_PLUGIN_SCAN(stderr, "scanLadspaDescriptor: name:%s info->_name:%s cstring:%s info->_label:%s cstring:%s\n",
//                     ladspa_descr->Name,
//                     info->_name.toLatin1().constData(),
//                     MusEPlugin::getCString(info->_name),
//                     info->_label.toLatin1().constData(),
//                     MusEPlugin::getCString(info->_label)
//                    );

  if(do_ports)
  {
    scanLadspaPorts(ladspa_descr, info, do_rdf);
  }
  else
  {
    info->_portCount = ladspa_descr->PortCount;
    unsigned long ip = 0, op = 0, cip = 0, cop = 0;
    for(unsigned long k = 0; k < ladspa_descr->PortCount; ++k)
    {
      const LADSPA_PortDescriptor pd = ladspa_descr->PortDescriptors[k];
      if(pd & LADSPA_PORT_AUDIO)
      {
        if(pd & LADSPA_PORT_INPUT)
        {
          ++ip;
        }
        else
        if(pd & LADSPA_PORT_OUTPUT)
        {
          ++op;
        }
      }
      else
      if(pd & LADSPA_PORT_CONTROL)
      {
        if(pd & LADSPA_PORT_INPUT)
        {
          ++cip;
        }
        else
        if(pd & LADSPA_PORT_OUTPUT)
        {
          ++cop;
          const QString pname(PLUGIN_SET_CSTRING(ladspa_descr->PortNames[k]));
          if(pname == QString("latency") || pname == QString("_latency"))
          {
            info->_pluginFlags |= PluginScanInfoStruct::HasLatencyPort;
            info->_latencyPortIdx = k;
          }
        }
      }
    }

    info->_inports = ip;
    info->_outports = op;
    info->_controlInPorts = cip;
    info->_controlOutPorts = cop;

    if((info->_inports != info->_outports) || LADSPA_IS_INPLACE_BROKEN(ladspa_descr->Properties))
      info->_requiredFeatures |= MusECore::PluginNoInPlaceProcessing;
  }

  return true;
}

bool writeLadspaInfo (
  const char* filename,
  LADSPA_Descriptor_Function ladspa,
  bool do_ports,
  int level,
  MusECore::Xml& xml)
{
  bool do_rdf = false;

#ifdef HAVE_LRDF

  if(do_ports)
  {
    const QString sch("file:///");
    QStringList rdfs;
    lrdf_init();
    scanLrdfPlugins(&rdfs, false);
    const int rdfs_sz = rdfs.size();
    QByteArrayList ba_rdfs_uris;
    const char* uris[rdfs_sz + 1];
    for(int i = 0; i < rdfs_sz; ++i)
    {
      ba_rdfs_uris.append((sch + rdfs.at(i)).toLocal8Bit());
      uris[i] = ba_rdfs_uris.at(i).constData();
    }
    uris[rdfs_sz] = nullptr;
    if(lrdf_read_files(uris))
      std::fprintf(stderr, "writeLadspaInfo: lrdf_read_files() Failed\n");
    else
      do_rdf = true;
  }

#endif // HAVE_LRDF

  const LADSPA_Descriptor* descr;
  for(unsigned long i = 0;; ++i)
  {
    descr = ladspa(i);
    if(descr == NULL)
      break;
    PluginScanInfoStruct info;
    if(!scanLadspaDescriptor(filename, descr, &info, do_ports, do_rdf))
      continue;
    writePluginScanInfo(level, xml, info, do_ports);
  }

#ifdef HAVE_LRDF
  if(do_ports)
    lrdf_cleanup();
#endif // HAVE_LRDF

  return true;
}

bool scanMessDescriptor(const char* filename, const MESS* mess_descr, PluginScanInfoStruct* info)
{
  setPluginScanFileInfo(filename, info);
  info->_type = PluginScanInfoStruct::PluginTypeMESS;
  info->_class = PluginScanInfoStruct::PluginClassInstrument;
  info->_uniqueID = 0;

  //info->_label = MusEPlugin::setString(mess_descr->Label);
  info->_label = PLUGIN_SET_CSTRING(mess_descr->name);
  info->_name = PLUGIN_SET_CSTRING(mess_descr->name);
  //info->_maker = MusEPlugin::setString(mess_descr->Maker);
  //info->_copyright = MusEPlugin::setString(mess_descr->Copyright);
  info->_description = PLUGIN_SET_CSTRING(mess_descr->description);
  info->_version = PLUGIN_SET_CSTRING(mess_descr->version);

  info->_apiVersionMajor = mess_descr->majorMessVersion;
  info->_apiVersionMinor = mess_descr->minorMessVersion;

  info->_portCount = 0;
  info->_inports = 0;
  info->_outports = 0;
  info->_controlInPorts = 0;
  info->_controlOutPorts = 0;

  info->_pluginFlags |= PluginScanInfoStruct::HasGui;

  return true;
}

bool writeMessInfo(const char* filename, MESS_Descriptor_Function mess, bool do_ports, int level, MusECore::Xml& xml)
{
  const MESS* mess_descr = mess();
  if(mess_descr)
  {
    PluginScanInfoStruct info;
    if(scanMessDescriptor(filename, mess_descr, &info))
    {
      writePluginScanInfo(level, xml, info, do_ports);
      return true;
    }
  }
  return false;
}


#ifdef DSSI_SUPPORT

QString getDssiUiFilename(PluginScanInfoStruct* info)
{
  if(PLUGIN_STRING_EMPTY(info->_absolutePath) || PLUGIN_STRING_EMPTY(info->lib()))
    return QString();

  const QString libr = PLUGIN_GET_QSTRING(info->lib());
  const QString guiPath(PLUGIN_GET_QSTRING(info->dirPath()) + "/" + libr);

  const QDir guiDir(guiPath, "*", QDir::Unsorted, QDir::Files);
  if(!guiDir.exists())
    return QString();

  const QStringList list = guiDir.entryList();

  const QString plug = PLUGIN_GET_QSTRING(info->_label);
  QString lib_qt_ui;
  QString lib_any_ui;
  QString plug_qt_ui;
  QString plug_any_ui;

  for(int i = 0; i < list.count(); ++i)
  {
    QFileInfo fi(guiPath + QString("/") + list[i]);
    QString gui(fi.filePath());
    struct stat buf;
    if(stat(gui.toLatin1().constData(), &buf))
      continue;
    if(!((S_ISREG(buf.st_mode) || S_ISLNK(buf.st_mode)) &&
        (buf.st_mode & (S_IXUSR | S_IXGRP | S_IXOTH))))
      continue;

    // FIXME: Qt::CaseInsensitive - a quick and dirty way to accept any suffix. Should be case sensitive...
    if(!libr.isEmpty())
    {
      if(lib_qt_ui.isEmpty() && list[i].contains(libr + QString("_qt"), Qt::CaseInsensitive))
        lib_qt_ui = gui;
      if(lib_any_ui.isEmpty() && list[i].contains(libr + QString('_') /*, Qt::CaseInsensitive*/))
        lib_any_ui = gui;
    }
    if(!plug.isEmpty())
    {
      if(plug_qt_ui.isEmpty() && list[i].contains(plug + QString("_qt"), Qt::CaseInsensitive))
        plug_qt_ui = gui;
      if(plug_any_ui.isEmpty() && list[i].contains(plug + QString('_') /*, Qt::CaseInsensitive*/))
        plug_any_ui = gui;
    }
  }

  // Prefer qt plugin ui
  if(!plug_qt_ui.isEmpty())
    return plug_qt_ui;
  // Prefer any plugin ui
  if(!plug_any_ui.isEmpty())
    return plug_any_ui;
  // Prefer qt lib ui
  if(!lib_qt_ui.isEmpty())
    return lib_qt_ui;
  // Prefer any lib ui
  if(!lib_any_ui.isEmpty())
    return lib_any_ui;

  // No suitable UI file found
  return QString();
};

bool scanDssiDescriptor(
  const char* filename,
  const DSSI_Descriptor* dssi_descr,
  PluginScanInfoStruct* info,
  bool do_ports,
  bool do_rdf)
{
  const LADSPA_Descriptor* ladspa_descr = dssi_descr->LADSPA_Plugin;
  if(!ladspa_descr)
    return false;

  if(!scanLadspaDescriptor(filename, ladspa_descr, info, do_ports, do_rdf))
    return false;

  info->_type = PluginScanInfoStruct::PluginTypeDSSI;
  info->_apiVersionMajor = dssi_descr->DSSI_API_Version;
  info->_apiVersionMinor = 0;

  if(PLUGIN_GET_QSTRING(info->_completeBaseName) == PLUGIN_GET_QSTRING("dssi-vst"))
  {
    info->_type = PluginScanInfoStruct::PluginTypeDSSIVST;
    info->_requiredFeatures |= MusECore::PluginFixedBlockSize;
    info->_requiredFeatures |= MusECore::PluginCoarseBlockSize;
  }

  if(dssi_descr->run_synth || dssi_descr->run_synth_adding ||
      dssi_descr->run_multiple_synths || dssi_descr->run_multiple_synths_adding)
    info->_class |= PluginScanInfoStruct::PluginClassInstrument;

  info->_uiFilename = PLUGIN_SET_QSTRING(getDssiUiFilename(info));
  if(!PLUGIN_STRING_EMPTY(info->_uiFilename))
    info->_pluginFlags |= PluginScanInfoStruct::HasGui;

  return true;
}

bool writeDssiInfo(const char* filename, DSSI_Descriptor_Function dssi, bool do_ports, int level, MusECore::Xml& xml)
{
  bool do_rdf = false;

#ifdef HAVE_LRDF

  if(do_ports)
  {
    const QString sch("file:///");
    QStringList rdfs;
    lrdf_init();
    scanLrdfPlugins(&rdfs, false);
    const int rdfs_sz = rdfs.size();
    QByteArrayList ba_rdfs_uris;
    const char* uris[rdfs_sz + 1];
    for(int i = 0; i < rdfs_sz; ++i)
    {
      ba_rdfs_uris.append((sch + rdfs.at(i)).toLocal8Bit());
      uris[i] = ba_rdfs_uris.at(i).constData();
    }
    uris[rdfs_sz] = nullptr;
    if(lrdf_read_files(uris))
      std::fprintf(stderr, "writeDssiInfo: lrdf_read_files() Failed\n");
    else
      do_rdf = true;
  }

#endif // HAVE_LRDF

  const DSSI_Descriptor* dssi_descr;
  for(unsigned long i = 0;; ++i)
  {
    dssi_descr = dssi(i);
    if(dssi_descr == 0)
      break;
    PluginScanInfoStruct info;
    if(!scanDssiDescriptor(filename, dssi_descr, &info, do_ports, do_rdf))
      continue;
    writePluginScanInfo(level, xml, info, do_ports);
  }

#ifdef HAVE_LRDF
  if(do_ports)
    lrdf_cleanup();
#endif // HAVE_LRDF

  return true;
}

#endif // DSSI_SUPPORT


#ifdef VST_NATIVE_SUPPORT

static VstIntPtr currentPluginId = 0;
//static sem_t _vstIdLock;
//static QSemaphore _vstIdLock;

//-----------------------------------------------------------------------------------------
//   vstHostCallback
//   This must be a function, it cannot be a class method so we dispatch to various objects from here.
//-----------------------------------------------------------------------------------------

VstIntPtr VSTCALLBACK vstNativeHostCallback(AEffect* effect, VstInt32 opcode, VstInt32 /*index*/, VstIntPtr /*value*/, void* ptr, float /*opt*/)
{
      // Is this callback for an actual instance? Hand-off to the instance if so.
      //VSTPlugin* plugin;
      if(effect && effect->user)
      {
        /*
        VstNativeSynthOrPlugin *userData = (VstNativeSynthOrPlugin*)(effect->user);
        //return ((VstNativeSynthIF*)plugin)->hostCallback(opcode, index, value, ptr, opt);
        return VstNativeSynth::pluginHostCallback(userData, opcode, index, value, ptr, opt);
        */
        return 0;
      }

      // No instance found. So we are just scanning for plugins...

      DEBUG_PLUGIN_SCAN(stderr, "plugin_scan: vstNativeHostCallback eff:%p opcode:%ld\n", effect, (unsigned long)opcode);

      switch (opcode) {
            case audioMasterAutomate:
                  // index, value, returns 0
                  return 0;

            case audioMasterVersion:
                  // vst version, currently 2 (0 for older)
                  return 2300;

            case audioMasterCurrentId:
                  // returns the unique id of a plug that's currently
                  // loading
                  return currentPluginId;

            case audioMasterIdle:
                  // call application idle routine (this will
                  // call effEditIdle for all open editors too)
                  return 0;

            case audioMasterGetTime:
                  // returns const VstTimeInfo* (or 0 if not supported)
                  // <value> should contain a mask indicating which fields are required
                  // (see valid masks above), as some items may require extensive
                  // conversions
                  return 0;

            case audioMasterProcessEvents:
                  // VstEvents* in <ptr>
                  return 0;

            case audioMasterIOChanged:
                   // numInputs and/or numOutputs has changed
                  return 0;

            case audioMasterSizeWindow:
                  // index: width, value: height
                  return 0;

            case audioMasterGetSampleRate:
                  //return MusEGlobal::sampleRate;
                  return 44100;

            case audioMasterGetBlockSize:
                  //return MusEGlobal::segmentSize;
                  return 512;

            case audioMasterGetInputLatency:
                  return 0;

            case audioMasterGetOutputLatency:
                  return 0;

            case audioMasterGetCurrentProcessLevel:
                  // returns: 0: not supported,
                  // 1: currently in user thread (gui)
                  // 2: currently in audio thread (where process is called)
                  // 3: currently in 'sequencer' thread (midi, timer etc)
                  // 4: currently offline processing and thus in user thread
                  // other: not defined, but probably pre-empting user thread.
                  return 0;

            case audioMasterGetAutomationState:
                  // returns 0: not supported, 1: off, 2:read, 3:write, 4:read/write
                  // offline
                  return 0;

            case audioMasterOfflineStart:
            case audioMasterOfflineRead:
                   // ptr points to offline structure, see below. return 0: error, 1 ok
                  return 0;

            case audioMasterOfflineWrite:
                  // same as read
                  return 0;

            case audioMasterOfflineGetCurrentPass:
            case audioMasterOfflineGetCurrentMetaPass:
                  return 0;

            case audioMasterGetVendorString:
                  // fills <ptr> with a string identifying the vendor (max 64 char)
                  strcpy ((char*) ptr, "MusE");
                  return 1;

            case audioMasterGetProductString:
                  // fills <ptr> with a string with product name (max 64 char)
                  strcpy ((char*) ptr, "NativeVST");
                  return 1;

            case audioMasterGetVendorVersion:
                  // returns vendor-specific version
                  return 2000;

            case audioMasterVendorSpecific:
                  // no definition, vendor specific handling
                  return 0;

            case audioMasterCanDo:
                  // string in ptr, see below
                  return 0;

            case audioMasterGetLanguage:
                  // see enum
                  return kVstLangEnglish;

            case audioMasterGetDirectory:
                  // get plug directory, FSSpec on MAC, else char*
                  return 0;

            case audioMasterUpdateDisplay:
                  // something has changed, update 'multi-fx' display
                  return 0;

            case audioMasterBeginEdit:
                  // begin of automation session (when mouse down), parameter index in <index>
                  return 0;

            case audioMasterEndEdit:
                  // end of automation session (when mouse up),     parameter index in <index>
                  return 0;

            case audioMasterOpenFileSelector:
                  // open a fileselector window with VstFileSelect* in <ptr>
                  return 0;

            case audioMasterCloseFileSelector:
                  return 0;

#ifdef VST_FORCE_DEPRECATED
#ifndef VST_2_4_EXTENSIONS // deprecated in 2.4

            case audioMasterGetSpeakerArrangement:
                  // (long)input in <value>, output in <ptr>
                  return 0;

            case audioMasterPinConnected:
                  // inquire if an input or output is being connected;
                  // index enumerates input or output counting from zero:
                  // value is 0 for input and != 0 otherwise. note: the
                  // return value is 0 for <true> such that older versions
                  // will always return true.
                  //return 1;
                  return 0;

            // VST 2.0 opcodes...
            case audioMasterWantMidi:
                  // <value> is a filter which is currently ignored
                  return 0;

            case audioMasterSetTime:
                  // VstTimenfo* in <ptr>, filter in <value>, not supported
                  return 0;

            case audioMasterTempoAt:
                  // returns tempo (in bpm * 10000) at sample frame location passed in <value>
                  return 0;  // TODO:

            case audioMasterGetNumAutomatableParameters:
                  return 0;

            case audioMasterGetParameterQuantization:
                     // returns the integer value for +1.0 representation,
                   // or 1 if full single float precision is maintained
                     // in automation. parameter index in <value> (-1: all, any)
                  //return 0;
                  return 1;

            case audioMasterNeedIdle:
                   // plug needs idle calls (outside its editor window)
                  return 0;

            case audioMasterGetPreviousPlug:
                   // input pin in <value> (-1: first to come), returns cEffect*
                  return 0;

            case audioMasterGetNextPlug:
                   // output pin in <value> (-1: first to come), returns cEffect*
                  return 0;

            case audioMasterWillReplaceOrAccumulate:
                   // returns: 0: not supported, 1: replace, 2: accumulate
                  //return 0;
                  return 1;

            case audioMasterSetOutputSampleRate:
                  // for variable i/o, sample rate in <opt>
                  return 0;

            case audioMasterSetIcon:
                  // void* in <ptr>, format not defined yet
                  return 0;

            case audioMasterOpenWindow:
                  // returns platform specific ptr
                  return 0;

            case audioMasterCloseWindow:
                  // close window, platform specific handle in <ptr>
                  return 0;
#endif
#endif

            default:
                  break;
            }

        DEBUG_PLUGIN_SCAN(stderr, "  unknown opcode\n");

      return 0;
      }


static void setVstParameterInfo(
  AEffect *plugin,
  unsigned long i,
  PluginPortInfo* port_info
  )
{
  VstParameterProperties props;
  if(plugin->dispatcher(plugin, effGetParameterProperties, i, 0, &props, 0))
  {
    if(props.flags & kVstParameterIsSwitch)
      port_info->_valueFlags |= PluginPortInfo::ToggledVal;

    if(props.flags & kVstParameterUsesIntegerMinMax)
    {
      port_info->_min = props.minInteger;
      port_info->_max = props.maxInteger;
      port_info->_valueFlags |= (PluginPortInfo::HasMin | PluginPortInfo::HasMax);
    }

    if(props.flags & kVstParameterUsesFloatStep)
    {
      port_info->_step = props.stepFloat;
      port_info->_smallStep = props.smallStepFloat;
      port_info->_largeStep = props.largeStepFloat;
      port_info->_valueFlags |= (PluginPortInfo::HasMin | PluginPortInfo::HasStep);
    }

    if(props.flags & kVstParameterUsesIntStep)
    {
      port_info->_step = props.stepInteger;
      // No such member. Makes sense I suppose.
      //port_info->_smallStep = props.smallStepInteger;
      port_info->_largeStep = props.largeStepInteger;
      port_info->_valueFlags |= (PluginPortInfo::HasMin | PluginPortInfo::HasStep);
    }

    char label_buf[VstLabelLen + 1];
        label_buf[0] = 0;
    std::strncpy(label_buf, props.label, VstLabelLen);
        label_buf[VstLabelLen] = 0;
    port_info->_name = PLUGIN_SET_CSTRING(label_buf);

    char shlabel_buf[VstShortLabelLen + 1];
        shlabel_buf[0] = 0;
    std::strncpy(shlabel_buf, props.label, VstShortLabelLen);
        shlabel_buf[VstShortLabelLen] = 0;
    port_info->_symbol = PLUGIN_SET_CSTRING(shlabel_buf);
  }
  else
  {
    char buf[256];
    buf[0] = 0;
    plugin->dispatcher(plugin, effGetParamName, i, 0, buf, 0);
    port_info->_name = PLUGIN_SET_CSTRING(buf);
  }
}

//---------------------------------------------------------
//   scanLinuxVstDescriptor
//---------------------------------------------------------

bool scanLinuxVstPorts(AEffect *plugin, PluginScanInfoStruct* info)
{
  info->_portCount = plugin->numInputs + plugin->numOutputs + plugin->numParams;
  info->_inports = plugin->numInputs;
  info->_outports = plugin->numOutputs;
  info->_controlInPorts = plugin->numParams;
  info->_controlOutPorts = 0;

  if((info->_inports != info->_outports) || !(plugin->flags & effFlagsCanReplacing))
    info->_requiredFeatures |= MusECore::PluginNoInPlaceProcessing;

  unsigned long k = 0;
  for(int i = 0; i < plugin->numInputs; ++i)
  {
    PluginPortInfo port_info;
    port_info._index = k;
    port_info._type = PluginPortInfo::AudioPort | PluginPortInfo::InputPort;
    info->_portList.push_back(port_info);
    ++k;
  }

  for(int i = 0; i < plugin->numOutputs; ++i)
  {
    PluginPortInfo port_info;
    port_info._index = k;
    port_info._type = PluginPortInfo::AudioPort | PluginPortInfo::OutputPort;
    info->_portList.push_back(port_info);
    ++k;
  }

  for(int i = 0; i < plugin->numParams; ++i)
  {
    PluginPortInfo port_info;
    port_info._index = k;
    port_info._type = PluginPortInfo::ControlPort | PluginPortInfo::InputPort;
    // Notice the 'i' not 'k'.
    setVstParameterInfo(plugin, i, &port_info);

    info->_portList.push_back(port_info);
    ++k;
  }

  return true;
}

//---------------------------------------------------------
//   scanLinuxVstDescriptor
//---------------------------------------------------------

bool scanLinuxVstDescriptor(const char* filename, AEffect *plugin, long int id, PluginScanInfoStruct* info, bool do_ports)
{
  char buffer[256];
  int vendorVersion;
  int vst_version = 0;

  if(plugin->flags & effFlagsHasEditor)
  {
    info->_pluginFlags |= PluginScanInfoStruct::HasGui;
    DEBUG_PLUGIN_SCAN(stderr, "Plugin has a GUI\n");
  }
  else
  {
    DEBUG_PLUGIN_SCAN(stderr, "Plugin has no GUI\n");
  }

  if(plugin->flags & effFlagsCanReplacing)
  {
    DEBUG_PLUGIN_SCAN(stderr, "Plugin supports processReplacing\n");
  }
  else
  {
    DEBUG_PLUGIN_SCAN(stderr, "Plugin does not support processReplacing\n");
  }

  plugin->dispatcher(plugin, effOpen, 0, 0, nullptr, 0);

  buffer[0] = 0;
  plugin->dispatcher(plugin, effGetEffectName, 0, 0, buffer, 0);
  if(buffer[0])
    info->_label = PLUGIN_SET_CSTRING(buffer);

  buffer[0] = 0;
  plugin->dispatcher(plugin, effGetVendorString, 0, 0, buffer, 0);
  if (buffer[0])
    info->_maker = PLUGIN_SET_CSTRING(buffer);

  buffer[0] = 0;
  plugin->dispatcher(plugin, effGetProductString, 0, 0, buffer, 0);
  if (buffer[0])
    info->_description = PLUGIN_SET_CSTRING(buffer);

  vendorVersion = plugin->dispatcher(plugin, effGetVendorVersion, 0, 0, nullptr, 0);

  buffer[0] = 0;
  sprintf(buffer, "%d.%d.%d", vendorVersion >> 16, vendorVersion >> 8, vendorVersion & 0xff);
  if(buffer[0])
    info->_version = PLUGIN_SET_CSTRING(buffer);

  setPluginScanFileInfo(filename, info);

  // Some (older) plugins don't have any of these strings. We only have the filename to use.
  if(PLUGIN_STRING_EMPTY(info->_label))
    info->_label = info->_completeBaseName;
  if(PLUGIN_STRING_EMPTY(info->_description))
    //info->_description = info._completeBaseName;
    info->_description = info->_label;

  info->_name = info->_label;

  // "2 = VST2.x, older versions return 0". Observed 2400 on all the ones tested so far.
  vst_version = plugin->dispatcher(plugin, effGetVstVersion, 0, 0, nullptr, 0.0f);

  info->_type = PluginScanInfoStruct::PluginTypeLinuxVST;
  info->_class = PluginScanInfoStruct::PluginClassEffect;
  info->_uniqueID = plugin->uniqueID;
  info->_subID = id;

  info->_apiVersionMajor = vst_version;
  info->_apiVersionMinor = 0;
  info->_pluginVersionMajor = (vendorVersion >> 16) & 0xff;
  //info->_pluginVersionMinor = (vendorVersion >> 8) & 0xff;
  info->_pluginVersionMinor = vendorVersion & 0xffff;
  //info->_pluginVersionMicro = vendorVersion & 0xff;

  if(plugin->flags & 32 /*effFlagsProgramChunks*/)
    info->_pluginFlags |= PluginScanInfoStruct::HasChunks;

  if(do_ports)
  {
    scanLinuxVstPorts(plugin, info);
  }
  else
  {
    info->_portCount = plugin->numInputs + plugin->numOutputs + plugin->numParams;
    info->_inports = plugin->numInputs;
    info->_outports = plugin->numOutputs;
    info->_controlInPorts = plugin->numParams;
    info->_controlOutPorts = 0;

    if((info->_inports != info->_outports) || !(plugin->flags & effFlagsCanReplacing))
      info->_requiredFeatures |= MusECore::PluginNoInPlaceProcessing;
  }

// TODO
//   if()
//   {
//     info._requiredFeatures |= MusECore::PluginScanInfo::FixedBlockSize;
//   }

  // "2 = VST2.x, older versions return 0". Observed 2400 on all the ones tested so far.
  if(vst_version >= 2)
  {
    if(plugin->dispatcher(plugin, effCanDo, 0, 0, (void*)"receiveVstEvents", 0.0f) > 0)
      info->_vstPluginFlags |= MusECore::canReceiveVstEvents;
    if(plugin->dispatcher(plugin, effCanDo, 0, 0, (void*)"sendVstEvents", 0.0f) > 0)
      info->_vstPluginFlags |= MusECore::canSendVstEvents;
    if(plugin->dispatcher(plugin, effCanDo, 0, 0, (void*)"sendVstMidiEvent", 0.0f) > 0)
      info->_vstPluginFlags |= MusECore::canSendVstMidiEvents;
    if(plugin->dispatcher(plugin, effCanDo, 0, 0, (void*)"sendVstTimeInfo", 0.0f) > 0)
      info->_vstPluginFlags |= MusECore::canSendVstTimeInfo;
    if(plugin->dispatcher(plugin, effCanDo, 0, 0, (void*)"receiveVstMidiEvent", 0.0f) > 0)
      info->_vstPluginFlags |= MusECore::canReceiveVstMidiEvents;
    if(plugin->dispatcher(plugin, effCanDo, 0, 0, (void*)"receiveVstTimeInfo", 0.0f) > 0)
      info->_vstPluginFlags |= MusECore::canReceiveVstTimeInfo;
    if(plugin->dispatcher(plugin, effCanDo, 0, 0, (void*)"offline", 0.0f) > 0)
      info->_vstPluginFlags |= MusECore::canProcessVstOffline;
    if(plugin->dispatcher(plugin, effCanDo, 0, 0, (void*)"plugAsChannelInsert", 0.0f) > 0)
      info->_vstPluginFlags |= MusECore::canUseVstAsInsert;
    if(plugin->dispatcher(plugin, effCanDo, 0, 0, (void*)"plugAsSend", 0.0f) > 0)
      info->_vstPluginFlags |= MusECore::canUseVstAsSend;
    if(plugin->dispatcher(plugin, effCanDo, 0, 0, (void*)"mixDryWet", 0.0f) > 0)
      info->_vstPluginFlags |= MusECore::canMixVstDryWet;
    if(plugin->dispatcher(plugin, effCanDo, 0, 0, (void*)"midiProgramNames", 0.0f) > 0)
      info->_vstPluginFlags |= MusECore::canVstMidiProgramNames;
  }

  if((plugin->flags & effFlagsIsSynth) ||
    (vst_version >= 2 && plugin->dispatcher(plugin, effCanDo, 0, 0,(void*) "receiveVstEvents", 0.0f) > 0))
  {
    info->_class |= PluginScanInfoStruct::PluginClassInstrument;
  }

  plugin->dispatcher(plugin, effClose, 0, 0, nullptr, 0);

  return true;
}

bool writeLinuxVstInfo(
  const char* filename,
  LinuxVST_Instance_Function lvst,
  bool do_ports,
  int level,
  MusECore::Xml& xml)
{
// sem_wait(&_vstIdLock);
// _vstIdLock.acquire();
  currentPluginId = 0;
// bool bDontDlCLose = false;

  AEffect *plugin = nullptr;

  plugin = lvst(vstNativeHostCallback);
  if(!plugin)
  {
    std::fprintf(stderr, "ERROR: Failed to instantiate plugin in VST library \"%s\"\n", filename);
    //goto _end;
    return false;
  }
  else
  {
    DEBUG_PLUGIN_SCAN(stderr, "plugin instantiated\n");
  }

  if(plugin->magic != kEffectMagic)
  {
    std::fprintf(stderr, "Not a VST plugin in library \"%s\"\n", filename);
    //goto _end;
    return false;
  }
  else
  {
    DEBUG_PLUGIN_SCAN(stderr, "plugin is a VST\n");
  }

  if(plugin->dispatcher(plugin, 24 + 11 /* effGetCategory */, 0, 0, 0, 0) == 10 /* kPlugCategShell */)
  {
//     bDontDlCLose = true;
    std::map<VstIntPtr, std::string> shellPlugs;
    char cPlugName [256];
    do
    {
        memset(cPlugName, 0, sizeof(cPlugName));
        VstIntPtr id = plugin->dispatcher(plugin, 24 + 46 /* effShellGetNextPlugin */, 0, 0, cPlugName, 0);
        if(id != 0 && cPlugName [0] != 0)
        {
          shellPlugs.insert(std::make_pair(id, std::string(cPlugName)));
        }
        else
          break;
    }
    while(true);

    for(std::map<VstIntPtr, std::string>::iterator it = shellPlugs.begin(); it != shellPlugs.end(); ++it)
    {
        if(plugin)
        {
          // TODO: Is effClose necessary? Try removing.
          //plugin->dispatcher(plugin, effClose, 0, 0, nullptr, 0);
          plugin = nullptr;
        }

        currentPluginId = it->first;

        plugin = lvst(vstNativeHostCallback);
        if(!plugin)
        {
          std::fprintf(stderr, "ERROR: Failed to instantiate plugin in VST library \"%s\", shell id=%ld\n",
                  filename, (long)currentPluginId);
          //goto _end;
        }
        else
        {
          PluginScanInfoStruct info;
          if(scanLinuxVstDescriptor(filename, plugin, currentPluginId, &info, do_ports))
            writePluginScanInfo(level, xml, info, do_ports);
        }
        currentPluginId = 0;
    }
  }
  else
  {
    PluginScanInfoStruct info;
    if(scanLinuxVstDescriptor(filename, plugin, 0, &info, do_ports))
      writePluginScanInfo(level, xml, info, do_ports);
  }


  //plugin->dispatcher(plugin, effMainsChanged, 0, 0, nullptr, 0);
//   if(plugin)
//       plugin->dispatcher(plugin, effClose, 0, 0, nullptr, 0);

//               _end:
//               if(handle && !bDontDlCLose)
//                   dlclose(handle);
//
//               sem_post(&_vstIdLock);
//               _vstIdLock.release();

  return true;
}

#endif // VST_NATIVE_SUPPORT

//---------------------------------------------------------
//   writeUnknownPluginInfo
//---------------------------------------------------------

bool writeUnknownPluginInfo (
  const char* filename,
  int level,
  MusECore::Xml& xml)
{
  PluginScanInfoStruct info;
  setPluginScanFileInfo(filename, &info);
  info._type = PluginScanInfoStruct::PluginTypeUnknown;
  //info._fileIsBad = true;
  writePluginScanInfo(level, xml, info, false);

  return true;
}

//---------------------------------------------------------
//   writePluginScanInfo
//---------------------------------------------------------

void writePluginScanInfo(int level, MusECore::Xml& xml, const PluginScanInfoStruct& info, bool writePorts)
      {
      xml.tag(level++, "plugin file=\"%s\" label=\"%s\"",
         MusECore::Xml::xmlString(PLUGIN_GET_QSTRING(info.filePath())).toLatin1().constData(),
         MusECore::Xml::xmlString(PLUGIN_GET_QSTRING(info._label)).toLatin1().constData());

      if(!PLUGIN_STRING_EMPTY(info._uri))
        xml.strTag(level, "uri", PLUGIN_GET_CSTRING(info._uri));

      if(info._fileTime != 0)
        xml.longLongTag(level, "filetime", info._fileTime);
      if(info._fileIsBad)
        xml.intTag(level, "fileIsBad", info._fileIsBad);

      xml.intTag(level, "type", info._type);
      xml.intTag(level, "class", info._class);
      if(info._uniqueID != 0)
        xml.uintTag(level, "uniqueID", info._uniqueID);
      if(info._subID != 0)
        xml.intTag(level, "subID", info._subID);
      if(!PLUGIN_STRING_EMPTY(info._name))
        xml.strTag(level, "name", PLUGIN_GET_CSTRING(info._name));
      if(!PLUGIN_STRING_EMPTY(info._description))
        xml.strTag(level, "description", PLUGIN_GET_CSTRING(info._description));
      if(!PLUGIN_STRING_EMPTY(info._version))
        xml.strTag(level, "version", PLUGIN_GET_CSTRING(info._version));
      if(!PLUGIN_STRING_EMPTY(info._maker))
        xml.strTag(level, "maker", PLUGIN_GET_CSTRING(info._maker));
      if(!PLUGIN_STRING_EMPTY(info._copyright))
        xml.strTag(level, "copyright", PLUGIN_GET_CSTRING(info._copyright));

      // Optimize out all numeric values if they are zero.
      // PluginScanInfoStruct initializes them to zero anyway.
      if(info._apiVersionMajor != 0)
        xml.intTag(level, "apiVersionMajor", info._apiVersionMajor);
      if(info._apiVersionMinor != 0)
        xml.intTag(level, "apiVersionMinor", info._apiVersionMinor);
      if(info._pluginVersionMajor != 0)
        xml.intTag(level, "pluginVersionMajor", info._pluginVersionMajor);
      if(info._pluginVersionMinor != 0)
        xml.intTag(level, "pluginVersionMinor", info._pluginVersionMinor);
      if(info._pluginFlags != 0)
        xml.intTag(level, "pluginFlags", info._pluginFlags);
      if(info._portCount != 0)
        xml.uintTag(level, "portCount", info._portCount);
      if(info._inports != 0)
        xml.uintTag(level, "inports", info._inports);
      if(info._outports != 0)
        xml.uintTag(level, "outports", info._outports);
      if(info._controlInPorts != 0)
        xml.uintTag(level, "ctlInports", info._controlInPorts);
      if(info._controlOutPorts != 0)
        xml.uintTag(level, "ctlOutports", info._controlOutPorts);
      if(info._eventInPorts != 0)
        xml.uintTag(level, "evInports", info._eventInPorts);
      if(info._eventOutPorts != 0)
        xml.uintTag(level, "evOutports", info._eventOutPorts);
      if((info._pluginFlags & PluginScanInfoStruct::HasFreewheelPort) || info._freewheelPortIdx != 0)
        xml.uintTag(level, "freewheelPortIdx", info._freewheelPortIdx);
      if((info._pluginFlags & PluginScanInfoStruct::HasLatencyPort) || info._latencyPortIdx != 0)
        xml.uintTag(level, "latencyPortIdx", info._latencyPortIdx);
      if(info._requiredFeatures != 0)
        xml.intTag(level, "requiredFeatures", info._requiredFeatures);
      if(info._vstPluginFlags != MusECore::vstPluginNoFlags)
        xml.intTag(level, "vstPluginFlags", info._vstPluginFlags);
      if(!PLUGIN_STRING_EMPTY(info._uiFilename))
        xml.strTag(level, "uiFilename", PLUGIN_GET_CSTRING(info._uiFilename));

      // Make sure the actual list has that many ports.
      if(writePorts)
      {
        if(info._portList.size() != info._portCount)
        {
          std::fprintf(stderr, "writePluginScanInfo: Error: port count:%u != port list size:%u\n",
                       (unsigned)info._portCount, (unsigned)info._portList.size());
        }
        else
        {
          for (unsigned long i = 0; i < info._portCount; ++i)
          {
            const PluginPortInfo& port_info = info._portList[i];

            // Optimize out all values if they are default.
            // PluginScanInfoStruct initializes them to defaults anyway.

    #ifdef PORTS_ARE_SINGLE_LINE_TAGS
            // As attributes on one line...

            QString s = QString("port");

            if(!PLUGIN_STRING_EMPTY(port_info._name))
              s += QString(" name=\"%1\"").arg(MusECore::Xml::xmlString(PLUGIN_GET_QSTRING(port_info._name)));

            if(!PLUGIN_STRING_EMPTY(port_info._symbol))
              s += QString(" symbol=\"%1\"").arg(MusECore::Xml::xmlString(PLUGIN_GET_QSTRING(port_info._symbol)));

            s += QString(" idx=\"%1\"").arg(i /*port_info._index*/);
            s += QString(" type=\"%1\"").arg(port_info._type);

            if(port_info._flags != PluginPortInfo::NoPortFlags)
              s += QString(" flags=\"%1\"").arg(port_info._flags);

            if(port_info._valueFlags != PluginPortInfo::NoValueFlags)
              s += QString(" valFlags=\"%1\"").arg(port_info._valueFlags);

            if((port_info._valueFlags & PluginPortInfo::HasMin) || port_info._min != PluginPortInfo::defaultPortMin)
              s += QString(" min=\"%1\"").arg(port_info._min);

            if((port_info._valueFlags & PluginPortInfo::HasMax) || port_info._max != PluginPortInfo::defaultPortMax)
              s += QString(" max=\"%1\"").arg(port_info._max);

            if((port_info._valueFlags & PluginPortInfo::HasDefault) || port_info._defaultVal != PluginPortInfo::defaultPortValue)
              s += QString(" def=\"%1\"").arg(port_info._defaultVal);

            if((port_info._valueFlags & PluginPortInfo::HasStep) || port_info._step != PluginPortInfo::defaultPortStep)
              s += QString(" step=\"%1\"").arg(port_info._step);

            if((port_info._valueFlags & PluginPortInfo::HasStep) || port_info._smallStep != PluginPortInfo::defaultPortSmallStep)
              s += QString(" smallStep=\"%1\"").arg(port_info._smallStep);

            if((port_info._valueFlags & PluginPortInfo::HasStep) || port_info._largeStep != PluginPortInfo::defaultPortLargeStep)
              s += QString(" largeStep=\"%1\"").arg(port_info._largeStep);

            s += QString(" /");

            xml.tag(level, s.toLatin1().constData());


    #else
            // As a tag with attributes...

            xml.tag(level++, "port name=\"%s\" symbol=\"%s\" idx=\"%s\" type=\"%s\"",
              MusECore::Xml::xmlString(PLUGIN_GET_QSTRING(port_info._name)).toLatin1().constData(),
              MusECore::Xml::xmlString(PLUGIN_GET_QSTRING(port_info._symbol)).toLatin1().constData(),
              QString::number(i).toLatin1().constData(),
              QString::number(port_info._type).toLatin1().constData()
            );

            if(port_info._flags != PluginPortInfo::NoPortFlags)
              xml.intTag(level, "flags", port_info._flags);

            if(port_info._valueFlags != PluginPortInfo::NoValueFlags)
              xml.intTag(level, "valFlags", port_info._valueFlags);

            if((port_info._valueFlags & PluginPortInfo::HasMin) || port_info._min != PluginPortInfo::defaultPortMin)
              xml.floatTag(level, "min", port_info._min);

            if((port_info._valueFlags & PluginPortInfo::HasMax) || port_info._max != PluginPortInfo::defaultPortMax)
              xml.floatTag(level, "max", port_info._max);

            if((port_info._valueFlags & PluginPortInfo::HasDefault) || port_info._defaultVal != PluginPortInfo::defaultPortValue)
              xml.floatTag(level, "def", port_info._defaultVal);

            if((port_info._valueFlags & PluginPortInfo::HasStep) || port_info._step != PluginPortInfo::defaultPortStep)
              xml.floatTag(level, "step", port_info._step);

            if((port_info._valueFlags & PluginPortInfo::HasStep) || port_info._smallStep != PluginPortInfo::defaultPortSmallStep)
              xml.floatTag(level, "smallStep", port_info._smallStep);

            if((port_info._valueFlags & PluginPortInfo::HasStep) || port_info._largeStep != PluginPortInfo::defaultPortLargeStep)
              xml.floatTag(level, "largeStep", port_info._largeStep);

            xml.tag(level--, "/port");
    #endif

          }

          if(!info._portEnumValMap.empty())
          {
            for(ciPortEnumValueMap ipev = info._portEnumValMap.begin(); ipev != info._portEnumValMap.end(); ++ipev)
            {
              // As a tag with attributes...

              const unsigned long idx = ipev->first;

              xml.tag(level++, "portEnumValMap idx=\"%s\"", QString::number(idx).toLatin1().constData());

              const EnumValueList& evl = ipev->second;
              for(ciEnumValueList ivl = evl.begin(); ivl != evl.end(); ++ivl)
              {
                const PluginPortEnumValue& pev = *ivl;

                QString s = QString("enumVal");
                s += QString(" val=\"%1\"").arg(pev._value);
                if(!PLUGIN_STRING_EMPTY(pev._label))
                  s += QString(" label=\"%1\"").arg(MusECore::Xml::xmlString(PLUGIN_GET_QSTRING(pev._label)));
                s += QString(" /");

                xml.tag(level, s.toLatin1().constData());
              }

              xml.tag(level--, "/portEnumValMap");
            }
          }
        }
      }

      xml.tag(level--, "/plugin");
      }

//---------------------------------------------------------
//   pluginScan
//   If debugStdErr is true, any stderr content received
//    from the scan program will be printed.
//   Returns true on success
//---------------------------------------------------------

static bool pluginScan(
  const QString& filename,
  PluginScanInfoStruct::PluginType_t types,
  PluginScanList* list,
  bool scanPorts,
  bool debugStdErr)
{
  const QByteArray filename_ba = filename.toLocal8Bit();
  QTemporaryFile tmpfile;
  // Must open the temp file to get its name.
  if(!tmpfile.open())
  {
    std::fprintf(stderr, "\npluginScan FAILED: Could not create temporary output file for input file: %s\n\n", filename_ba.constData());
    return false;
  }
  // Get the unique temp file name.
  const QString tmpfilename = tmpfile.fileName();
  const QByteArray tmpfilename_ba = tmpfilename.toLocal8Bit();
  // Close the temp file. It exists until tmpfile goes out of scope.
  tmpfile.close();

  if(debugStdErr)
    std::fprintf(stderr, "\nChecking file: <%s>\n", filename_ba.constData());

  QProcess process;

  QString prog;
  const QByteArray appDir = qgetenv("APPDIR");
  if (!appDir.isEmpty())
      prog = appDir + QString(BINDIR) + QString("/muse_plugin_scan");
  else
      prog = QString(BINDIR) + QString("/muse_plugin_scan");

  QStringList args;
  args << QString("-t") + QString::number(types) << QString("-f") + filename << QString("-o") + tmpfilename;
  if(scanPorts)
    args << QString("-p");

  process.start(prog, args);

  bool fail = false;

//   if(!process.waitForStarted(4000))
//   {
//     std::fprintf(stderr, "pluginScan: waitForStarted failed\n");
// //     return false;
//     fail = true;
//   }

//   if(!process.waitForReadyRead(4000))
//   {
//     std::fprintf(stderr, "pluginScan: waitForReadyRead failed\n");
// //     return false;
//     fail = true;
//   }

  if(!process.waitForFinished(6000))
  {
    std::fprintf(stderr, "\npluginScan FAILED: waitForFinished: file: %s\n\n", filename_ba.constData());
    while (1) {
      QMessageBox::StandardButton btn = QMessageBox::warning(
          nullptr, QMessageBox::tr("Plugin Scanner"),
          QMessageBox::tr("Checking Plugin %1 is taking a very long time, do you want to keep waiting for it to finish, or skip this plugin?").arg(filename),
          QMessageBox::Retry|QMessageBox::Abort, QMessageBox::Retry);

      if (btn == QMessageBox::Retry) {
        if (process.state() == QProcess::NotRunning) {
          break; // success
        }
        if(process.waitForFinished(6000)) {
          break; // success
        }
      } else {
        fail = true;
        break; // failure
      }
    }
  }

  if(!fail && debugStdErr)
  {
    QByteArray out_array = process.readAllStandardOutput();
    if(!out_array.isEmpty() && out_array.at(0) != 0)
    {
      // Terminate just to be sure.
      out_array.append(char(0));
      std::fprintf(stderr, "\npluginScan: Standard output from scan:\n%s\n", out_array.constData());
    }
    QByteArray err_array = process.readAllStandardError();
    if(!err_array.isEmpty() && err_array.at(0) != 0)
    {
      // Terminate just to be sure.
      err_array.append(char(0));
      std::fprintf(stderr, "\npluginScan: Standard error output from scan:\n%s\n", err_array.constData());
    }
  }

  if(process.exitStatus() != QProcess::NormalExit)
  {
    std::fprintf(stderr, "\npluginScan FAILED: Scan not exited normally: file: %s\n\n", filename_ba.constData());
    fail = true;
  }

  if(process.exitCode() != 0)
  {
    std::fprintf(stderr, "\npluginScan FAILED: Scan exit code not 0: file: %s\n\n", filename_ba.constData());
    fail = true;
  }

  if(!fail)
  {
    // Open the temp file again...
    QFile infile(tmpfilename);
    if(!infile.exists())
    {
      std::fprintf(stderr, "\npluginScan FAILED: Temporary file does not exist: %s\n\n", tmpfilename_ba.constData());
      fail = true;
    }
    if(!fail && !infile.open(QIODevice::ReadOnly /*| QIODevice::Text*/))
    {
      std::fprintf(stderr, "\npluginScan FAILED: Could not re-open temporary output file: %s\n\n",
                  tmpfilename_ba.constData());
      fail = true;
    }

    if(!fail)
    {
      // Create an xml object based on the file.
      MusECore::Xml xml(&infile);

      // Read the list of plugins found in the xml.
      // For now we don't supply a separate scanEnums flag in pluginScan(), so just use scanPorts instead.
      if(readPluginScan(xml, list, scanPorts, scanPorts))
      {
        std::fprintf(stderr, "\npluginScan FAILED: On readPluginScan(): file: %s\n\n", filename_ba.constData());
        fail = true;
      }

      // Close the temp file.
      infile.close();

      // Now going out of scope destroys the temporary file...
      if(!fail)
        return true;
    }
  }

  //---------------------------------------------------------------
  // Plugin failed scanning. Add it to the list but mark it as bad!
  //---------------------------------------------------------------

  PluginScanInfoStruct info;
  setPluginScanFileInfo(filename, &info);
  info._type = PluginScanInfoStruct::PluginTypeUnknown;
  info._fileIsBad = true;
  // We must include all plugins.
  list->add(new PluginScanInfo(info));

  return false;
}

//---------------------------------------------------------
//   scanPluginDir
//   This might be called recursively!
//---------------------------------------------------------

static void scanPluginDir(
  const QString& dirname,
  PluginScanInfoStruct::PluginType_t types,
  PluginScanList* list,
  bool scanPorts,
  bool debugStdErr,
  // Only for recursions, original top caller should not touch!
  int recurseLevel = 0
)
{
  const int max_levels = 10;
  if(recurseLevel >= max_levels)
  {
    std::fprintf(stderr, "scanPluginDir: Ignoring too-deep directory level (max:%d) at:%s\n",
                 max_levels, dirname.toLocal8Bit().constData());
    return;
  }

  DEBUG_PLUGIN_SCAN(stderr, "scan plugin dir <%s>\n", dirname.toLatin1().constData());

  QDir pluginDir(
    dirname,
    QString("*.so"),
    QDir::Name | QDir::IgnoreCase,
    QDir::Drives | QDir::Files | QDir::AllDirs | QDir::NoDotAndDotDot);

  if(pluginDir.exists())
  {
    QFileInfoList fi_list = pluginDir.entryInfoList();
    QFileInfoList::iterator it=fi_list.begin();
    while(it != fi_list.end())
    {
      const QFileInfo& fi = *it;
      if(fi.isDir())
        // RECURSIVE!
        scanPluginDir(fi.filePath(), types, list, scanPorts, debugStdErr, recurseLevel + 1);
      else
        pluginScan(fi.filePath(), types, list, scanPorts, debugStdErr);

      ++it;
    }
  }
}

//---------------------------------------------------------
//   scanLadspaPlugins
//---------------------------------------------------------

void scanLadspaPlugins(const QString& museGlobalLib, PluginScanList* list, bool scanPorts, bool debugStdErr)
{
  QStringList sl = pluginGetLadspaDirectories(museGlobalLib);
  for(QStringList::const_iterator it = sl.cbegin(); it != sl.cend(); ++it)
    scanPluginDir(*it, PluginScanInfoStruct::PluginTypeAll, list, scanPorts, debugStdErr);
}

//---------------------------------------------------------
//   scanMessPlugins
//---------------------------------------------------------

void scanMessPlugins(const QString& museGlobalLib, PluginScanList* list, bool scanPorts, bool debugStdErr)
{
  QStringList sl = pluginGetMessDirectories(museGlobalLib);
  for(QStringList::const_iterator it = sl.cbegin(); it != sl.cend(); ++it)
    scanPluginDir(*it, PluginScanInfoStruct::PluginTypeAll, list, scanPorts, debugStdErr);
}

//---------------------------------------------------------
//   scanDssiPlugins
//---------------------------------------------------------

#ifdef DSSI_SUPPORT
void scanDssiPlugins(PluginScanList* list, bool scanPorts, bool debugStdErr)
{
  QStringList sl = pluginGetDssiDirectories();
  for(QStringList::const_iterator it = sl.cbegin(); it != sl.cend(); ++it)
    scanPluginDir(*it, PluginScanInfoStruct::PluginTypeAll, list, scanPorts, debugStdErr);
}
#else // No DSSI_SUPPORT
void scanDssiPlugins(PluginScanList* /*list*/, bool /*scanPorts*/, bool /*debugStdErr*/)
{
}
#endif // DSSI_SUPPORT

//---------------------------------------------------------
//   scanLinuxVSTPlugins
//---------------------------------------------------------

#ifdef VST_NATIVE_SUPPORT
void scanLinuxVSTPlugins(PluginScanList* list, bool scanPorts, bool debugStdErr)
{
  #ifdef VST_VESTIGE_SUPPORT
    std::fprintf(stderr, "Initializing Native VST support. Using VESTIGE compatibility implementation.\n");
  #else
    std::fprintf(stderr, "Initializing Native VST support. Using Steinberg VSTSDK.\n");
  #endif

//   sem_init(&_vstIdLock, 0, 1);

  QStringList sl = pluginGetLinuxVstDirectories();
  for(QStringList::const_iterator it = sl.cbegin(); it != sl.cend(); ++it)
    scanPluginDir(*it, PluginScanInfoStruct::PluginTypeAll, list, scanPorts, debugStdErr);
}
#else
void scanLinuxVSTPlugins(PluginScanList* /*list*/, bool /*scanPorts*/, bool /*debugStdErr*/)
{
}
#endif // VST_NATIVE_SUPPORT

#ifdef LV2_SUPPORT

#define NS_EXT "http://lv2plug.in/ns/ext/"
#define NS_LV2CORE "http://lv2plug.in/ns/lv2core"

#define LV2_INSTRUMENT_CLASS NS_LV2CORE "#InstrumentPlugin"
#define LV2_F_BOUNDED_BLOCK_LENGTH LV2_BUF_SIZE__boundedBlockLength
#define LV2_F_FIXED_BLOCK_LENGTH LV2_BUF_SIZE__fixedBlockLength
#define LV2_F_POWER_OF_2_BLOCK_LENGTH LV2_BUF_SIZE__powerOf2BlockLength
// BUG FIXME: 'coarseBlockLength' is NOT in the lv2 buf-size.h header file!
// #define LV2_F_COARSE_BLOCK_LENGTH LV2_BUF_SIZE__coarseBlockLength
#define LV2_F_COARSE_BLOCK_LENGTH LV2_BUF_SIZE_PREFIX "coarseBlockLength"
#define LV2_P_SEQ_SIZE LV2_BUF_SIZE__sequenceSize
#define LV2_P_MAX_BLKLEN LV2_BUF_SIZE__maxBlockLength
#define LV2_P_MIN_BLKLEN LV2_BUF_SIZE__minBlockLength
#define LV2_P_NOM_BLKLEN LV2_BUF_SIZE__nominalBlockLength
#define LV2_P_SAMPLE_RATE LV2_PARAMETERS__sampleRate
#define LV2_F_OPTIONS LV2_OPTIONS__options
#define LV2_F_URID_MAP LV2_URID__map
#define LV2_F_URID_UNMAP LV2_URID__unmap
#define LV2_F_URI_MAP LV2_URI_MAP_URI
#define LV2_F_UI_PARENT LV2_UI__parent
#define LV2_F_INSTANCE_ACCESS NS_EXT "instance-access"
#define LV2_F_DATA_ACCESS LV2_DATA_ACCESS_URI
#define LV2_F_UI_EXTERNAL_HOST LV2_EXTERNAL_UI__Host
#define LV2_F_WORKER_SCHEDULE LV2_WORKER__schedule
#define LV2_F_WORKER_INTERFACE LV2_WORKER__interface
#define LV2_F_UI_IDLE LV2_UI__idleInterface
#define LV2_F_UI_Qt5_UI LV2_UI_PREFIX "Qt5UI"
#define LV2_UI_HOST_URI LV2_F_UI_Qt5_UI
#define LV2_UI_EXTERNAL LV2_EXTERNAL_UI__Widget
#define LV2_UI_EXTERNAL_DEPRECATED LV2_EXTERNAL_UI_DEPRECATED_URI
//#define LV2_F_DEFAULT_STATE LV2_STATE_PREFIX "loadDefaultState"
#define LV2_F_STATE_CHANGED LV2_STATE_PREFIX "StateChanged"

//uri cache structure.
typedef struct
{
   LilvNode *atom_AtomPort;
   LilvNode *ev_EventPort;
   LilvNode *lv2_AudioPort;
   LilvNode *lv2_ControlPort;
   LilvNode *lv2_InputPort;
   LilvNode *lv2_OutputPort;
   LilvNode *lv2_connectionOptional;
   LilvNode *host_uiType;
   LilvNode *ext_uiType;
   LilvNode *ext_d_uiType;
   LilvNode *lv2_portDiscrete;
   LilvNode *lv2_portEnumeration;
   LilvNode *lv2_portContinuous;
   LilvNode *lv2_portLogarithmic;
   LilvNode *lv2_portInteger;
   LilvNode *lv2_portTrigger;
   LilvNode *lv2_portToggled;
   LilvNode *lv2_TimePosition;
   LilvNode *lv2_FreeWheelPort;
   LilvNode *lv2_isLive;
   LilvNode *lv2_HardRealtimeCapable;
   LilvNode *lv2_InPlaceBroken;
   LilvNode *lv2_SampleRate;
   LilvNode *lv2_CVPort;
   LilvNode *lv2_psetPreset;
   LilvNode *lv2_rdfsLabel;
   LilvNode *lv2_actionSavePreset;
   LilvNode *lv2_actionUpdatePresets;
   LilvNode *end;  ///< NULL terminator for easy freeing of entire structure
} CacheNodes;

static CacheNodes lv2CacheNodes;

LV2_Feature lv2Features [] =
{
   {LV2_F_URID_MAP, NULL},
   {LV2_F_URID_UNMAP, NULL},
   {LV2_F_URI_MAP, NULL},
   {LV2_F_BOUNDED_BLOCK_LENGTH, NULL},
   {LV2_F_FIXED_BLOCK_LENGTH, NULL},
   {LV2_F_POWER_OF_2_BLOCK_LENGTH, NULL},
   {LV2_F_COARSE_BLOCK_LENGTH, NULL},
   {LV2_F_UI_PARENT, NULL},
   {LV2_F_INSTANCE_ACCESS, NULL},
   {LV2_F_UI_EXTERNAL_HOST, NULL},
   {LV2_UI_EXTERNAL_DEPRECATED, NULL},
   {LV2_F_WORKER_SCHEDULE, NULL},
   {LV2_F_UI_IDLE, NULL},
   {LV2_F_OPTIONS, NULL},
   {LV2_UI__resize, NULL},
   {LV2_PROGRAMS__Host, NULL},
   {LV2_LOG__log, NULL},
#ifdef LV2_MAKE_PATH_SUPPORT
   {LV2_STATE__makePath, NULL},
#endif
   {LV2_STATE__mapPath, NULL},
   {LV2_F_STATE_CHANGED, NULL},
   {LV2_F_DATA_ACCESS, NULL} //must be the last always!
};

#define SIZEOF_ARRAY(x) sizeof(x)/sizeof(x[0])

void scanLv2Ports(const LilvPlugin *plugin,
                  PluginScanInfoStruct* info,
                  bool /*debugStdErr*/)
{
  // Does this plugin have a 'freewheel' port?
  if(const LilvPort *lilvFreeWheelPort =
    lilv_plugin_get_port_by_designation(
      plugin, lv2CacheNodes.lv2_InputPort, lv2CacheNodes.lv2_FreeWheelPort))
  {
    info->_pluginFlags |= PluginScanInfoStruct::HasFreewheelPort;
    info->_freewheelPortIdx = lilv_port_get_index(plugin, lilvFreeWheelPort);
  }

  if(lilv_plugin_has_latency(plugin))
  {
    info->_pluginFlags |= PluginScanInfoStruct::HasLatencyPort;
    info->_latencyPortIdx = lilv_plugin_get_latency_port_index(plugin);
  }

  info->_portCount = lilv_plugin_get_num_ports(plugin);

  float pluginControlsDefault[info->_portCount];
  float pluginControlsMin[info->_portCount];
  float pluginControlsMax[info->_portCount];
  lilv_plugin_get_port_ranges_float(plugin, pluginControlsMin, pluginControlsMax, pluginControlsDefault);

  unsigned int aip = 0;
  unsigned int aop = 0;
  unsigned int cip = 0;
  unsigned int cop = 0;
  unsigned int eip = 0;
  unsigned int eop = 0;
  for(unsigned long k = 0; k < info->_portCount; ++k)
  {
    PluginPortInfo port_info;
    port_info._index = k;

    const LilvPort *lilvPort = lilv_plugin_get_port_by_index(plugin, k);
    LilvNode *nPname = lilv_port_get_name(plugin, lilvPort);
    const LilvNode *nPsym = lilv_port_get_symbol(plugin, lilvPort);

    char cAutoGenPortName [1024];
    char cAutoGenPortSym [1024];
    memset(cAutoGenPortName, 0, sizeof(cAutoGenPortName));
    memset(cAutoGenPortSym, 0, sizeof(cAutoGenPortSym));
    snprintf(cAutoGenPortName, sizeof(cAutoGenPortName) - 1, "autoport #%u", (unsigned)k);
    snprintf(cAutoGenPortSym, sizeof(cAutoGenPortSym) - 1, "autoport#%u", (unsigned)k);
    const char *_portName = cAutoGenPortName;
    const char *_portSym = cAutoGenPortSym;

    if(nPname)
      _portName = lilv_node_as_string(nPname);
    if(nPsym)
      _portSym = lilv_node_as_string(nPsym);

    if(_portName && _portName[0])
      port_info._name = PLUGIN_SET_CSTRING(_portName);
    if(_portSym && _portSym[0])
      port_info._symbol = PLUGIN_SET_CSTRING(_portSym);

    const bool optional = lilv_port_has_property(plugin, lilvPort, lv2CacheNodes.lv2_connectionOptional);

    if(lilv_port_is_a(plugin, lilvPort, lv2CacheNodes.lv2_InputPort))
    {
      port_info._type = PluginPortInfo::InputPort;
    }
    else if(lilv_port_is_a(plugin, lilvPort, lv2CacheNodes.lv2_OutputPort))
    {
      port_info._type = PluginPortInfo::OutputPort;
    }
    else
    {
      DEBUG_PLUGIN_SCAN(stderr, "plugin has port with unknown direction - ignoring\n");
      if(nPname)
        lilv_node_free(nPname);
      continue;
    }

    bool isCVPort = lilv_port_is_a(plugin, lilvPort, lv2CacheNodes.lv2_CVPort);

    if(isCVPort)
      port_info._flags |= PluginPortInfo::IsCVPort;

    if(lilv_port_is_a(plugin, lilvPort, lv2CacheNodes.lv2_ControlPort) || isCVPort)
    {
      port_info._type |= PluginPortInfo::ControlPort;

//       port_info._valueType = PluginPortInfo::LinearVal;
//       if(lilv_port_has_property(plugin, lilvPort, lv2CacheNodes.lv2_portDiscrete))
//         port_info._valueFlags = PluginPortInfo::IntegerVal;
//       else if(lilv_port_has_property(plugin, lilvPort, lv2CacheNodes.lv2_portInteger))
//         port_info._valueFlags = PluginPortInfo::IntegerVal;
//       else if(lilv_port_has_property(plugin, lilvPort, lv2CacheNodes.lv2_portTrigger)
//               || lilv_port_has_property(plugin, lilvPort, lv2CacheNodes.lv2_portToggled))
//         port_info._valueFlags = PluginPortInfo::ToggledVal;
//       else if(lilv_port_has_property(plugin, lilvPort, lv2CacheNodes.lv2_portLogarithmic))
//         port_info._valueFlags = PluginPortInfo::LogVal;

      if(lilv_port_has_property(plugin, lilvPort, lv2CacheNodes.lv2_portDiscrete))
        port_info._valueFlags |= PluginPortInfo::IntegerVal;

      if(lilv_port_has_property(plugin, lilvPort, lv2CacheNodes.lv2_portInteger))
        port_info._valueFlags |= PluginPortInfo::IntegerVal;

      if(lilv_port_has_property(plugin, lilvPort, lv2CacheNodes.lv2_portToggled))
        port_info._valueFlags |= PluginPortInfo::ToggledVal;

      if(lilv_port_has_property(plugin, lilvPort, lv2CacheNodes.lv2_portTrigger))
        port_info._valueFlags |= PluginPortInfo::TriggerVal;

      if(lilv_port_has_property(plugin, lilvPort, lv2CacheNodes.lv2_portLogarithmic))
        port_info._valueFlags |= PluginPortInfo::LogVal;

      // "If a port doesn't have a minimum, maximum or default value,
      //  or the port's type is not float, the corresponding array element
      //  will be set to NAN."
      if(std::isnan(pluginControlsDefault[k]))
        pluginControlsDefault[k] = 0;
      else
        port_info._valueFlags |= PluginPortInfo::HasDefault;

      if(std::isnan(pluginControlsMin[k]))
        pluginControlsMin[k] = 0;
      else
        port_info._valueFlags |= PluginPortInfo::HasMin;

      if(std::isnan(pluginControlsMax[k]))
        pluginControlsMax[k] = 1;
      else
        port_info._valueFlags |= PluginPortInfo::HasMax;

      // Is it a specialized audio control port?
      if(isCVPort)
      {
        // Audio ports don't usually have range information, so make up a range.
        pluginControlsDefault[k] = 1;
        pluginControlsMin[k] = 0;
        pluginControlsMax[k] = 1;
        port_info._valueFlags |=
          (PluginPortInfo::HasDefault | PluginPortInfo::HasMin | PluginPortInfo::HasMax);
      }
      // It's a normal control port. Do the values depend on the current sample rate?
      else if (lilv_port_has_property (plugin, lilvPort, lv2CacheNodes.lv2_SampleRate))
      {
        port_info._flags |= PluginPortInfo::ScaleBySamplerate;
      }

      port_info._min = pluginControlsMin[k];
      port_info._max = pluginControlsMax[k];
      port_info._defaultVal = pluginControlsDefault[k];

      // Read any value enumerations...
      LilvScalePoints* scale_points;
      scale_points = lilv_port_get_scale_points(plugin, lilvPort);
      if(scale_points)
      {
        // Map for ensuring the enumerations are sorted by increasing value.
        std::map<float, PluginPortEnumValue, std::less<float> > enum_list;

        LilvIter* sp_it = lilv_scale_points_begin(scale_points);
        while(!lilv_scale_points_is_end(scale_points, sp_it))
        {
          const LilvScalePoint* sp = lilv_scale_points_get(scale_points, sp_it);
          const LilvNode* vn = lilv_scale_point_get_value(sp);
          const LilvNode* ln = lilv_scale_point_get_label(sp);
          if(lilv_node_is_float(vn) && lilv_node_is_string(ln))
          {
            const float v = lilv_node_as_float(vn);
            const PluginPortEnumValue ev(v, PLUGIN_SET_CSTRING(lilv_node_as_string(ln)));
            enum_list.insert(std::pair<float, PluginPortEnumValue>(v, ev));
          }
          sp_it = lilv_scale_points_next(scale_points, sp_it);
        }
        lilv_scale_points_free(scale_points);

        // Copy the sorted enumeration values to the vector enumeration list.
        EnumValueList dst_list;
        for(std::map<float, PluginPortEnumValue, std::less<float>>::const_iterator iel = enum_list.begin();
            iel != enum_list.end(); ++iel)
        {
          dst_list.push_back(iel->second);
        }
        if(!dst_list.empty())
        {
          info->_portEnumValMap.insert(PortEnumValueMapPair(k, dst_list));
          port_info._valueFlags |= PluginPortInfo::HasEnumerations;
        }
      }
    }
    else if(lilv_port_is_a(plugin, lilvPort, lv2CacheNodes.lv2_AudioPort))
    {
      port_info._type |= PluginPortInfo::AudioPort;
    }
    else if(lilv_port_is_a(plugin, lilvPort, lv2CacheNodes.ev_EventPort))
    {
      if(lilv_port_supports_event(plugin, lilvPort, lv2CacheNodes.lv2_TimePosition))
      {
        port_info._flags |= PluginPortInfo::SupportsTimePosition;
        info->_pluginFlags |= PluginScanInfoStruct::SupportsTimePosition;
      }
      port_info._type |= PluginPortInfo::MidiPort;
    }
    else if(lilv_port_is_a(plugin, lilvPort, lv2CacheNodes.atom_AtomPort))
    {
      if(lilv_port_supports_event(plugin, lilvPort, lv2CacheNodes.lv2_TimePosition))
      {
        port_info._flags |= PluginPortInfo::SupportsTimePosition;
        info->_pluginFlags |= PluginScanInfoStruct::SupportsTimePosition;
      }
      port_info._type |= PluginPortInfo::MidiPort;
    }
    else if(!optional)
    {
      DEBUG_PLUGIN_SCAN(stderr, "plugin has port with unknown type - ignoring plugin:%s\n",
                        name.toLatin1().constData());
      if(nPname != 0)
        lilv_node_free(nPname);
      return;
    }

    if(nPname != 0)
      lilv_node_free(nPname);

    if(port_info._type & PluginPortInfo::AudioPort)
    {
      if(port_info._type & PluginPortInfo::InputPort)
        ++aip;
      else if(port_info._type & PluginPortInfo::OutputPort)
        ++aop;
    }
    if(port_info._type & PluginPortInfo::ControlPort)
    {
      if(port_info._type & PluginPortInfo::InputPort)
        ++cip;
      else if(port_info._type & PluginPortInfo::OutputPort)
        ++cop;
    }
    if(port_info._type & PluginPortInfo::MidiPort)
    {
      if(port_info._type & PluginPortInfo::InputPort)
        ++eip;
      else if(port_info._type & PluginPortInfo::OutputPort)
        ++eop;
    }

    if((info->_pluginFlags & PluginScanInfoStruct::HasFreewheelPort) &&
       k == info->_freewheelPortIdx)
    {
      port_info._flags |= PluginPortInfo::IsFreewheel;
    }

    if((info->_pluginFlags & PluginScanInfoStruct::HasLatencyPort) &&
       k == info->_latencyPortIdx)
    {
      port_info._flags |= PluginPortInfo::IsLatency;
    }

    info->_portList.push_back(port_info);
  }

  info->_inports = aip;
  info->_outports = aop;
  info->_controlInPorts = cip;
  info->_controlOutPorts = cop;
  info->_eventInPorts = eip;
  info->_eventOutPorts = eop;

  if((info->_inports != info->_outports) || lilv_plugin_has_feature(plugin, lv2CacheNodes.lv2_InPlaceBroken))
    info->_requiredFeatures |= MusECore::PluginNoInPlaceProcessing;
}

static void scanLv2Plugin(const LilvPlugin *plugin,
                          PluginScanList* list,
                          const std::set<std::string>& supportedFeatures,
                          bool do_ports,
                          bool debugStdErr)
{
// LV2 does not use unique id numbers and frowns upon using anything but the uri.
//   static unsigned long FakeLv2UniqueID = 1;


  LilvNode *nameNode = lilv_plugin_get_name(plugin);
  const LilvNode *uriNode = lilv_plugin_get_uri(plugin);

  if(!nameNode)
    return;

  if(!lilv_node_is_string(nameNode))
  {
    lilv_node_free(nameNode);
    return;
  }

  const char *pluginName = lilv_node_as_string(nameNode);
  const char *lfp = lilv_file_uri_parse(lilv_node_as_string(lilv_plugin_get_library_uri(plugin)), NULL);
  LilvNodes *fts = lilv_plugin_get_required_features(plugin);
  LilvIter *nit = lilv_nodes_begin(fts);
  bool shouldLoad = true;
  MusECore::PluginFeatures_t reqfeat = MusECore::PluginNoFeatures;
  while(true)
  {
    if(lilv_nodes_is_end(fts, nit))
      break;

    const LilvNode *fnode = lilv_nodes_get(fts, nit);
    const char *uri = lilv_node_as_uri(fnode);
    bool isSupported = (supportedFeatures.find(uri) != supportedFeatures.end());

    if(isSupported)
    {
      if(std::strcmp(uri, LV2_F_FIXED_BLOCK_LENGTH) == 0)
        reqfeat |= MusECore::PluginFixedBlockSize;
      else if(std::strcmp(uri, LV2_F_POWER_OF_2_BLOCK_LENGTH) == 0)
        reqfeat |= MusECore::PluginPowerOf2BlockSize;
      else if(std::strcmp(uri, LV2_F_COARSE_BLOCK_LENGTH) == 0)
        reqfeat |= MusECore::PluginCoarseBlockSize;
    }
    else
    {
      shouldLoad = false;
    }

    nit = lilv_nodes_next(fts, nit);
  }
  lilv_nodes_free(fts);

  //if (!shouldLoad || !isSynth)
  if(!shouldLoad)   //load all plugins for now, not only synths
  {
    lilv_free((void*)lfp); // Must free.
    lilv_node_free(nameNode);
    return;
  }

  const QString name = QString(pluginName) + QString(" LV2");

  MusEPlugin::PluginScanInfoStruct info;
  setPluginScanFileInfo(lfp, &info);
  if(uriNode)
    info._uri = PLUGIN_SET_CSTRING(lilv_node_as_string(uriNode));
  info._type = PluginScanInfoStruct::PluginTypeLV2;
  info._class = PluginScanInfoStruct::PluginClassEffect;
  // Fake id for LV2PluginWrapper functionality.
  // LV2 does not use unique id numbers and frowns upon using anything but the uri.
  // info._uniqueID =  FakeLv2UniqueID++;
  //info._label = MusEPlugin::setString(label);
  info._label = PLUGIN_SET_QSTRING(name);
  info._name = PLUGIN_SET_QSTRING(name);
  //info._maker = MusEPlugin::setString(ladspa_descr->Maker);
  //info._copyright = MusEPlugin::setString(ladspa_descr->Copyright);

  info._requiredFeatures |= reqfeat;

  const LilvPluginClass *cls = lilv_plugin_get_class(plugin);
  const LilvNode *ncuri = lilv_plugin_class_get_uri(cls);
  const char *clsname = lilv_node_as_uri(ncuri);
  if(strcmp(clsname, LV2_INSTRUMENT_CLASS) == 0)
  {
    info._class |= PluginScanInfoStruct::PluginClassEffect;
  }

  if(LilvNode *nAuthor = lilv_plugin_get_author_name(plugin))
  {
    info._maker = PLUGIN_SET_CSTRING(lilv_node_as_string(nAuthor));
    lilv_node_free(nAuthor);
  }

  if(do_ports)
  {
    scanLv2Ports(plugin, &info, debugStdErr);
  }
  else
  {
    // Does this plugin have a 'freewheel' port?
    if(const LilvPort *lilvFreeWheelPort =
      lilv_plugin_get_port_by_designation(
        plugin, lv2CacheNodes.lv2_InputPort, lv2CacheNodes.lv2_FreeWheelPort))
    {
      info._pluginFlags |= PluginScanInfoStruct::HasFreewheelPort;
      info._freewheelPortIdx = lilv_port_get_index(plugin, lilvFreeWheelPort);
    }

    if(lilv_plugin_has_latency(plugin))
    {
      info._pluginFlags |= PluginScanInfoStruct::HasLatencyPort;
      info._latencyPortIdx = lilv_plugin_get_latency_port_index(plugin);
    }

    info._portCount = lilv_plugin_get_num_ports(plugin);
    unsigned int aip = 0;
    unsigned int aop = 0;
    unsigned int cip = 0;
    unsigned int cop = 0;
    unsigned int eip = 0;
    unsigned int eop = 0;
    for(unsigned long k = 0; k < info._portCount; ++k)
    {
      const LilvPort *lilvPort = lilv_plugin_get_port_by_index(plugin, k);
      const bool optional = lilv_port_has_property(plugin, lilvPort, lv2CacheNodes.lv2_connectionOptional);

      bool is_output = false;
      if(lilv_port_is_a(plugin, lilvPort, lv2CacheNodes.lv2_OutputPort))
      {
        is_output = true;
      }
      else if(!lilv_port_is_a(plugin, lilvPort, lv2CacheNodes.lv2_InputPort))
      {
        DEBUG_PLUGIN_SCAN(stderr, "plugin has port with unknown direction - ignoring\n");
        continue;
      }

      bool isCVPort = lilv_port_is_a(plugin, lilvPort, lv2CacheNodes.lv2_CVPort);

      if(lilv_port_is_a(plugin, lilvPort, lv2CacheNodes.lv2_ControlPort) || isCVPort)
      {
        if(is_output)
          ++cop;
        else
          ++cip;
      }
      else if(lilv_port_is_a(plugin, lilvPort, lv2CacheNodes.lv2_AudioPort))
      {
        if(is_output)
          ++aop;
        else
          ++aip;
      }
      else if(lilv_port_is_a(plugin, lilvPort, lv2CacheNodes.ev_EventPort))
      {
        if(is_output)
          ++eop;
        else
          ++eip;
        if(lilv_port_supports_event(plugin, lilvPort, lv2CacheNodes.lv2_TimePosition))
          info._pluginFlags |= PluginScanInfoStruct::SupportsTimePosition;
      }
      else if(lilv_port_is_a(plugin, lilvPort, lv2CacheNodes.atom_AtomPort))
      {
        if(is_output)
          ++eop;
        else
          ++eip;
        if(lilv_port_supports_event(plugin, lilvPort, lv2CacheNodes.lv2_TimePosition))
          info._pluginFlags |= PluginScanInfoStruct::SupportsTimePosition;
      }
      else if(!optional)
      {
        DEBUG_PLUGIN_SCAN(stderr, "plugin has port with unknown type - ignoring plugin:%s\n",
                          name.toLatin1().constData());
        return;
      }
    }

    info._inports = aip;
    info._outports = aop;
    info._controlInPorts = cip;
    info._controlOutPorts = cop;
    info._eventInPorts = eip;
    info._eventOutPorts = eop;
  }

  // Look for optional features we support (now or later)...
  if(lilv_plugin_has_feature(plugin, lv2CacheNodes.lv2_isLive))
    info._pluginFlags |= PluginScanInfoStruct::Realtime;

  if(lilv_plugin_has_feature(plugin, lv2CacheNodes.lv2_HardRealtimeCapable))
    info._pluginFlags |= PluginScanInfoStruct::HardRealtimeCapable;

  if((info._inports != info._outports) || lilv_plugin_has_feature(plugin, lv2CacheNodes.lv2_InPlaceBroken))
    info._requiredFeatures |= MusECore::PluginNoInPlaceProcessing;

  // Make sure it doesn't already exist.
  if(list->find(info))
  {
    lilv_free((void*)lfp); // Must free.
    lilv_node_free(nameNode);
    return;
  }

  list->add(new PluginScanInfo(info));

  lilv_free((void*)lfp); // Must free.
  lilv_node_free(nameNode);
}

#endif // LV2_SUPPORT

//---------------------------------------------------------
//   scanLv2Plugins
//---------------------------------------------------------

#ifdef LV2_SUPPORT
void scanLv2Plugins(PluginScanList* list, bool scanPorts, bool debugStdErr)
{
  std::set<std::string> supportedFeatures;

  // REMOVE Tim. lv2. Added.
  //LV2_Feature* feats[SIZEOF_ARRAY(lv2Features) + 1];

  unsigned long int feat = 0;
  for(; feat < SIZEOF_ARRAY(lv2Features); feat++)
  {
    // REMOVE Tim. lv2. Added.
    //feats[feat] = &lv2Features[feat];

    supportedFeatures.insert(lv2Features [feat].URI);
  }
  // REMOVE Tim. lv2. Added.
  //feats[feat] = nullptr;

  LilvWorld *lilvWorld = 0;
  lilvWorld = lilv_world_new();
  if(!lilvWorld)
    return;

  lv2CacheNodes.atom_AtomPort          = lilv_new_uri(lilvWorld, LV2_ATOM__AtomPort);
  lv2CacheNodes.ev_EventPort           = lilv_new_uri(lilvWorld, LV2_EVENT__EventPort);
  lv2CacheNodes.lv2_AudioPort          = lilv_new_uri(lilvWorld, LV2_CORE__AudioPort);
  lv2CacheNodes.lv2_ControlPort        = lilv_new_uri(lilvWorld, LV2_CORE__ControlPort);
  lv2CacheNodes.lv2_InputPort          = lilv_new_uri(lilvWorld, LV2_CORE__InputPort);
  lv2CacheNodes.lv2_OutputPort         = lilv_new_uri(lilvWorld, LV2_CORE__OutputPort);
  lv2CacheNodes.lv2_connectionOptional = lilv_new_uri(lilvWorld, LV2_CORE__connectionOptional);
  lv2CacheNodes.host_uiType            = lilv_new_uri(lilvWorld, LV2_UI_HOST_URI);
  lv2CacheNodes.ext_uiType             = lilv_new_uri(lilvWorld, LV2_UI_EXTERNAL);
  lv2CacheNodes.ext_d_uiType           = lilv_new_uri(lilvWorld, LV2_UI_EXTERNAL_DEPRECATED);
  lv2CacheNodes.lv2_portContinuous     = lilv_new_uri(lilvWorld, LV2_PORT_PROPS__continuousCV);
  lv2CacheNodes.lv2_portDiscrete       = lilv_new_uri(lilvWorld, LV2_PORT_PROPS__discreteCV);
  lv2CacheNodes.lv2_portEnumeration    = lilv_new_uri(lilvWorld, LV2_CORE__enumeration);
  lv2CacheNodes.lv2_portLogarithmic    = lilv_new_uri(lilvWorld, LV2_PORT_PROPS__logarithmic);
  lv2CacheNodes.lv2_portInteger        = lilv_new_uri(lilvWorld, LV2_CORE__integer);
  lv2CacheNodes.lv2_portTrigger        = lilv_new_uri(lilvWorld, LV2_PORT_PROPS__trigger);
  lv2CacheNodes.lv2_portToggled        = lilv_new_uri(lilvWorld, LV2_CORE__toggled);
  lv2CacheNodes.lv2_TimePosition       = lilv_new_uri(lilvWorld, LV2_TIME__Position);
  lv2CacheNodes.lv2_FreeWheelPort      = lilv_new_uri(lilvWorld, LV2_CORE__freeWheeling);
  lv2CacheNodes.lv2_isLive             = lilv_new_uri(lilvWorld, LV2_CORE__isLive);
  lv2CacheNodes.lv2_HardRealtimeCapable= lilv_new_uri(lilvWorld, LV2_CORE__hardRTCapable);
  lv2CacheNodes.lv2_InPlaceBroken      = lilv_new_uri(lilvWorld, LV2_CORE__inPlaceBroken);
  lv2CacheNodes.lv2_SampleRate         = lilv_new_uri(lilvWorld, LV2_CORE__sampleRate);
  lv2CacheNodes.lv2_CVPort             = lilv_new_uri(lilvWorld, LV2_CORE__CVPort);
  lv2CacheNodes.lv2_psetPreset         = lilv_new_uri(lilvWorld, LV2_PRESETS__Preset);
  lv2CacheNodes.lv2_rdfsLabel          = lilv_new_uri(lilvWorld, "http://www.w3.org/2000/01/rdf-schema#label");
  lv2CacheNodes.lv2_actionSavePreset   = lilv_new_uri(lilvWorld, "http://www.muse-sequencer.org/lv2host#lv2_actionSavePreset");
  lv2CacheNodes.lv2_actionUpdatePresets= lilv_new_uri(lilvWorld, "http://www.muse-sequencer.org/lv2host#lv2_actionUpdatePresets");
  lv2CacheNodes.end                    = nullptr;

  lilv_world_load_all(lilvWorld);
  const LilvPlugins *plugins = lilv_world_get_all_plugins(lilvWorld);
  LilvIter *pit = lilv_plugins_begin(plugins);

  while(true)
  {
    if(lilv_plugins_is_end(plugins, pit))
    {
        break;
    }

    const LilvPlugin *plugin = lilv_plugins_get(plugins, pit);

    if(lilv_plugin_is_replaced(plugin))
    {
        pit = lilv_plugins_next(plugins, pit);
        continue;
    }

    // REMOVE Tim. lv2. Added.
//     // Test instantiating the plugin. TODO: Maybe try to pass the correct sample rate.
//     LilvInstance* handle = lilv_plugin_instantiate(plugin, 44100, feats);
//     if(!handle)
//     {
//         pit = lilv_plugins_next(plugins, pit);
//         continue;
//     }
//     //
//     // TODO: Maybe do some stuff with the plugin.
//     //
//     // No crash so far. Done with instance. Close it - free it.
//     lilv_instance_free(handle);

    // Now go ahead and scan the textual descriptions of the plugin.
    scanLv2Plugin(plugin, list, supportedFeatures, scanPorts, debugStdErr);

    pit = lilv_plugins_next(plugins, pit);
  }

   for(LilvNode **n = (LilvNode **)&lv2CacheNodes; *n; ++n)
     lilv_node_free(*n);


  lilv_world_free(lilvWorld);
  lilvWorld = nullptr;
}
#else
void scanLv2Plugins(PluginScanList* /*list*/, bool /*scanPorts*/, bool /*debugStdErr*/)
{
}
#endif // LV2_SUPPORT

//---------------------------------------------------------
//   scanAllPlugins
//---------------------------------------------------------

void scanAllPlugins(
  const QString& museGlobalLib,
  PluginScanList* list,
  bool scanPorts,
  bool debugStdErr,
  PluginScanInfoStruct::PluginType_t types)
{
  if(types & (PluginScanInfoStruct::PluginTypeDSSI | PluginScanInfoStruct::PluginTypeDSSIVST))
    // Take care of DSSI plugins first...
    scanDssiPlugins(list, scanPorts, debugStdErr);

  if(types & (PluginScanInfoStruct::PluginTypeLADSPA))
    // Now do LADSPA plugins...
    scanLadspaPlugins(museGlobalLib, list, scanPorts, debugStdErr);

  if(types & (PluginScanInfoStruct::PluginTypeMESS))
    // Now do MESS plugins...
    scanMessPlugins(museGlobalLib, list, scanPorts, debugStdErr);

  if(types & (PluginScanInfoStruct::PluginTypeLinuxVST))
    // Now do LinuxVST plugins...
    scanLinuxVSTPlugins(list, scanPorts, debugStdErr);

  if(types & (PluginScanInfoStruct::PluginTypeLV2))
    // Now do LV2 plugins...
    scanLv2Plugins(list, scanPorts, debugStdErr);
}

typedef std::map<QString, std::int64_t, std::less<QString> > filepath_set;
typedef std::pair<QString, std::int64_t> filepath_set_pair;

//---------------------------------------------------------
//   findPluginFilesDir
//   This might be called recursively!
//---------------------------------------------------------

static QString findPluginFilesDir(
  const QString& dirname,
  PluginScanInfoStruct::PluginType_t types,
  filepath_set& fplist,
  bool debugStdErr,
  // Only for recursions, original top caller should not touch!
  int recurseLevel = 0
)
{
  const int max_levels = 10;
  if(recurseLevel >= max_levels)
  {
    std::fprintf(stderr, "findPluginFilesDir: Ignoring too-deep directory level (max:%d) at:%s\n",
                 max_levels, dirname.toLocal8Bit().constData());
    return QString();
  }

  DEBUG_PLUGIN_SCAN(stderr, "find plugin dir <%s>\n", dirname.toLatin1().constData());

  QDir pluginDir(
    dirname,
    QString("*.so"),
    QDir::Name | QDir::IgnoreCase,
    QDir::Drives | QDir::Files | QDir::AllDirs | QDir::NoDotAndDotDot);

  if(pluginDir.exists())
  {
    QFileInfoList fi_list = pluginDir.entryInfoList();
    QFileInfoList::iterator it=fi_list.begin();
    while(it != fi_list.end())
    {
      const QFileInfo& fi = *it;
      if(fi.isDir())
      {
        // RECURSIVE!
        findPluginFilesDir(fi.filePath(), types, fplist, debugStdErr, recurseLevel + 1);
      }
      else
      {
        fplist.insert(filepath_set_pair(fi.filePath(), fi.lastModified().toMSecsSinceEpoch()));
      }

      ++it;
    }
  }
  return QString();
}

//---------------------------------------------------------
//   findLadspaPluginFiles
//---------------------------------------------------------

static void findLadspaPluginFiles(const QString& museGlobalLib, filepath_set& fplist, bool debugStdErr)
{
  const QStringList sl = pluginGetLadspaDirectories(museGlobalLib);
  for(QStringList::const_iterator it = sl.cbegin(); it != sl.cend(); ++it)
    findPluginFilesDir(*it, PluginScanInfoStruct::PluginTypeAll, fplist, debugStdErr);
}

//---------------------------------------------------------
//   findMessPluginFiles
//---------------------------------------------------------

static void findMessPluginFiles(const QString& museGlobalLib, filepath_set& fplist, bool debugStdErr)
{
  const QStringList sl = pluginGetMessDirectories(museGlobalLib);
  for(QStringList::const_iterator it = sl.cbegin(); it != sl.cend(); ++it)
    findPluginFilesDir(*it, PluginScanInfoStruct::PluginTypeAll, fplist, debugStdErr);
}

//---------------------------------------------------------
//   findDssiPluginFiles
//---------------------------------------------------------

#ifdef DSSI_SUPPORT
static void findDssiPluginFiles(filepath_set& fplist, bool debugStdErr)
{
  const QStringList sl = pluginGetDssiDirectories();
  for(QStringList::const_iterator it = sl.cbegin(); it != sl.cend(); ++it)
    findPluginFilesDir(*it, PluginScanInfoStruct::PluginTypeAll, fplist, debugStdErr);
}
#else // No DSSI_SUPPORT
static void findDssiPluginFiles(filepath_set& /*fplist*/, bool /*debugStdErr*/)
{
}
#endif // DSSI_SUPPORT

//---------------------------------------------------------
//   findLinuxVSTPluginFiles
//---------------------------------------------------------

#ifdef VST_NATIVE_SUPPORT
static void findLinuxVSTPluginFiles(filepath_set& fplist, bool debugStdErr)
{
  const QStringList sl = pluginGetLinuxVstDirectories();
  for(QStringList::const_iterator it = sl.cbegin(); it != sl.cend(); ++it)
    findPluginFilesDir(*it, PluginScanInfoStruct::PluginTypeAll, fplist, debugStdErr);
}
#else // Not VST_NATIVE_SUPPORT
static void findLinuxVSTPluginFiles(filepath_set& /*fplist*/, bool /*debugStdErr*/)
{
}
#endif // VST_NATIVE_SUPPORT


// SPECIAL for LV2: No need for a cache file.
// Do not find and compare LV2 library files here.
// This caused problems with identically named plugins
//  being excluded, and triggering rescans every time.
// LV2 frowns upon using anything but the URI !
#if 0

//---------------------------------------------------------
//   findLv2PluginFile
//---------------------------------------------------------

#ifdef LV2_SUPPORT
static void findLv2PluginFile(const LilvPlugin *plugin,
                          filepath_set& fplist,
                          const std::set<std::string>& supportedFeatures,
                          bool /*debugStdErr*/)
{
// LV2 does not use unique id numbers and frowns upon using anything but the uri.
//   static unsigned long FakeLv2UniqueID = 1;

  LilvNode *nameNode = lilv_plugin_get_name(plugin);
  if(!nameNode)
    return;

  if(!lilv_node_is_string(nameNode))
  {
    lilv_node_free(nameNode);
    return;
  }

  const char *lfp = lilv_file_uri_parse(lilv_node_as_string(lilv_plugin_get_library_uri(plugin)), NULL);
  LilvNodes *fts = lilv_plugin_get_required_features(plugin);
  LilvIter *nit = lilv_nodes_begin(fts);
  bool shouldLoad = true;
  while(true)
  {
    if(lilv_nodes_is_end(fts, nit))
      break;
    const LilvNode *fnode = lilv_nodes_get(fts, nit);
    const char *uri = lilv_node_as_uri(fnode);
    const bool isSupported = (supportedFeatures.find(uri) != supportedFeatures.end());
    if(!isSupported)
      shouldLoad = false;
    nit = lilv_nodes_next(fts, nit);
  }
  lilv_nodes_free(fts);

  //if (!shouldLoad || !isSynth)
  if(!shouldLoad)   //load all plugins for now, not only synths
  {
    lilv_free((void*)lfp); // Must free.
    lilv_node_free(nameNode);
    return;
  }

  if(lfp && lfp[0])
  {
    QFileInfo fi(lfp);
    if(fi.exists())
      fplist.insert(filepath_set_pair(fi.filePath(), fi.lastModified().toMSecsSinceEpoch()));
  }

  lilv_free((void*)lfp); // Must free.
  lilv_node_free(nameNode);
}
#endif

//---------------------------------------------------------
//   findLv2PluginFiles
//---------------------------------------------------------

#ifdef LV2_SUPPORT
static void findLv2PluginFiles(filepath_set& fplist, bool debugStdErr)
{
  std::set<std::string> supportedFeatures;

  unsigned long int feat = 0;
  for(; feat < SIZEOF_ARRAY(lv2Features); feat++)
  {
    supportedFeatures.insert(lv2Features [feat].URI);
  }

  LilvWorld *lilvWorld = 0;
  lilvWorld = lilv_world_new();
  if(!lilvWorld)
    return;

  lilv_world_load_all(lilvWorld);
  const LilvPlugins *plugins = lilv_world_get_all_plugins(lilvWorld);
  LilvIter *pit = lilv_plugins_begin(plugins);

  while(true)
  {
    if(lilv_plugins_is_end(plugins, pit))
      break;

    const LilvPlugin *plugin = lilv_plugins_get(plugins, pit);
    if(lilv_plugin_is_replaced(plugin))
    {
      pit = lilv_plugins_next(plugins, pit);
      continue;
    }

    findLv2PluginFile(plugin, fplist, supportedFeatures, debugStdErr);

    pit = lilv_plugins_next(plugins, pit);
  }

  lilv_world_free(lilvWorld);
  lilvWorld = nullptr;
}
#else
static void findLv2PluginFiles(filepath_set& /*fplist*/, bool /*debugStdErr*/)
{
}
#endif

#endif  // 0


//---------------------------------------------------------
//  findPluginFiles
//---------------------------------------------------------

static void findPluginFiles(const QString& museGlobalLib,
                    filepath_set& fplist,
                    bool debugStdErr,
                    PluginScanInfoStruct::PluginType_t types)
{
  if(types & (PluginScanInfoStruct::PluginTypeDSSI | PluginScanInfoStruct::PluginTypeDSSIVST))
  {
    // Take care of DSSI plugins first...
    findDssiPluginFiles(fplist, debugStdErr);
  }

  if(types & (PluginScanInfoStruct::PluginTypeLADSPA))
  {
    // Now do LADSPA plugins...
    findLadspaPluginFiles(museGlobalLib, fplist, debugStdErr);
  }

  if(types & (PluginScanInfoStruct::PluginTypeMESS))
  {
    // Now do MESS plugins...
    findMessPluginFiles(museGlobalLib, fplist, debugStdErr);
  }

  if(types & (PluginScanInfoStruct::PluginTypeLinuxVST))
  {
    // Now do LinuxVST plugins...
    findLinuxVSTPluginFiles(fplist, debugStdErr);
  }

  // SPECIAL for LV2: No need for a cache file.
  // Do not find and compare LV2 library files here.
  // This caused problems with identically named plugins
  //  being excluded, and triggering rescans every time.
  // LV2 frowns upon using anything but the URI !
  //if(types & (PluginScanInfoStruct::PluginTypeLV2))
  //{
  //  // Now do LV2 plugins...
  //  findLv2PluginFiles(fplist, debugStdErr);
  //}
}

//---------------------------------------------------------
//   writePluginCacheFile
//---------------------------------------------------------

bool writePluginCacheFile(
  const QString& scanOutPath,
  const QString& filename,
  const PluginScanList& list,
  bool writePorts,
  PluginScanInfoStruct::PluginType_t types)
{
  bool res = false;
  const QString targ_filepath = scanOutPath + "/" + filename;

  const QDir scanOutDir(scanOutPath);
  if(!scanOutDir.exists())
  {
     std::fprintf(stderr, "Creating plugin cache directory:%s\n",
                 scanOutPath.toLatin1().constData());
     scanOutDir.mkpath(".");
  }

  QFile targ_qfile(targ_filepath);
  if(!targ_qfile.open(QIODevice::WriteOnly | QIODevice::Text))
  {
    std::fprintf(stderr, "writePluginCacheFile: targ_qfile.open() failed: filename:%s\n",
                     filename.toLatin1().constData());
  }
  else
  {
      MusECore::Xml xml(&targ_qfile);
      int level = 0;
      xml.header();
      level = xml.putFileVersion(level);

      for(ciPluginScanList ips = list.begin(); ips != list.end(); ++ips)
      {
        PluginScanInfoRef inforef = *ips;
        const PluginScanInfoStruct& infos = inforef->info();

        // Look only for the specified type(s).
        if(infos._type & types)
          writePluginScanInfo(level, xml, infos, writePorts);
      }

      xml.tag(1, "/muse");

      DEBUG_PLUGIN_SCAN(stderr, "writePluginCacheFile: targ_qfile closing filename:%s\n",
                      filename.toLatin1().constData());
      targ_qfile.close();

      res = true;
  }

  return res;
}

//---------------------------------------------------------
//   createPluginCacheFile
//---------------------------------------------------------

bool createPluginCacheFile(
  const QString& path,
  PluginScanInfoStruct::PluginType type,
  PluginScanList* list,
  bool writePorts,
  const QString& museGlobalLib,
  PluginScanInfoStruct::PluginType_t types,
  bool debugStdErr)
{
  // Scan all plugins into the list.
  scanAllPlugins(museGlobalLib, list, writePorts, debugStdErr, type);

  // Write the list's cache file.
  if(!writePluginCacheFile(path, QString(pluginCacheFilename(type)), *list, writePorts, types))
  {
    std::fprintf(stderr, "createCacheFile: writePluginCacheFile() failed: filename:%s\n", pluginCacheFilename(type));
    return false;
  }

  return true;
}

//---------------------------------------------------------
//   createPluginCacheFiles
//---------------------------------------------------------

bool createPluginCacheFiles(
  const QString& path,
  PluginScanList* list,
  bool writePorts,
  const QString& museGlobalLib,
  PluginScanInfoStruct::PluginType_t types,
  bool debugStdErr)
{
  if(types & (PluginScanInfoStruct::PluginTypeDSSI | PluginScanInfoStruct::PluginTypeDSSIVST))
    createPluginCacheFile(path, PluginScanInfoStruct::PluginTypeDSSI, list, writePorts,
      museGlobalLib, PluginScanInfoStruct::PluginTypeDSSI | PluginScanInfoStruct::PluginTypeDSSIVST, debugStdErr);

  // NOTE: Because the dss-vst library installs itself in both the dssi AND ladspa folders,
  //        we must include dssi-vst types in the search here.
  //       The result is that the ladspa cache file will contain BOTH the ladspa folder dssi-vst file scan
  //        and the dssi folder dss-vst file scan.
  if(types & PluginScanInfoStruct::PluginTypeLADSPA)
    createPluginCacheFile(path, PluginScanInfoStruct::PluginTypeLADSPA, list, writePorts,
      museGlobalLib, PluginScanInfoStruct::PluginTypeLADSPA | PluginScanInfoStruct::PluginTypeDSSIVST, debugStdErr);

  if(types & PluginScanInfoStruct::PluginTypeLinuxVST)
    createPluginCacheFile(path, PluginScanInfoStruct::PluginTypeLinuxVST, list, writePorts,
      museGlobalLib, PluginScanInfoStruct::PluginTypeLinuxVST, debugStdErr);

  if(types & PluginScanInfoStruct::PluginTypeMESS)
    createPluginCacheFile(path, PluginScanInfoStruct::PluginTypeMESS, list, writePorts,
      museGlobalLib, PluginScanInfoStruct::PluginTypeMESS, debugStdErr);

  // SPECIAL for LV2: No need for a cache file. Do not create one here. Read directly into the list later.
  //if(types & PluginScanInfoStruct::PluginTypeLV2)
  //  createPluginCacheFile(path, PluginScanInfoStruct::PluginTypeLV2, list, writePorts,
  //    museGlobalLib, PluginScanInfoStruct::PluginTypeLV2, debugStdErr);

  if(types & PluginScanInfoStruct::PluginTypeVST)
    createPluginCacheFile(path, PluginScanInfoStruct::PluginTypeVST, list, writePorts,
      museGlobalLib, PluginScanInfoStruct::PluginTypeVST, debugStdErr);

  if(types & PluginScanInfoStruct::PluginTypeUnknown)
    createPluginCacheFile(path, PluginScanInfoStruct::PluginTypeUnknown, list, writePorts,
      museGlobalLib, PluginScanInfoStruct::PluginTypeUnknown, debugStdErr);

  return true;
}

//---------------------------------------------------------
//   checkPluginCacheFiles
//---------------------------------------------------------

bool checkPluginCacheFiles(
  const QString& path,
  PluginScanList* list,
  bool writePorts,
  bool alwaysRecreate,
  bool dontRecreate,
  const QString& museGlobalLib,
  PluginScanInfoStruct::PluginType_t types,
  bool debugStdErr
)
{
  filepath_set cache_fpset;
  bool res = true;
  bool cache_dirty = false;

  //-----------------------------------------------------
  // Read whatever we've got in our current cache files.
  //-----------------------------------------------------

  if(!readPluginCacheFiles(path, list, false, false, types))
  {
    cache_dirty = true;
    std::fprintf(stderr, "checkPluginCacheFiles: readAllPluginCacheFiles() failed\n");
  }

  // Check if cache is dirty. Don't bother if we already know it is dirty.
  if(!dontRecreate && !cache_dirty)
  {
    //-----------------------------------------------------
    // Gather the current plugin files.
    //-----------------------------------------------------

    filepath_set fpset;
    findPluginFiles(museGlobalLib, fpset, debugStdErr, types);

    //-------------------------------------------------------------------------
    // Gather the unique (non-duplicate) plugin file paths found in our cache.
    //-------------------------------------------------------------------------

    for(iPluginScanList ips = list->begin(); ips != list->end(); ++ips)
    {
      PluginScanInfoRef inforef = *ips;
      const PluginScanInfoStruct& infos = inforef->info();
      cache_fpset.insert(filepath_set_pair(PLUGIN_GET_QSTRING(infos.filePath()), infos._fileTime));
    }

    //---------------------------------------
    // Check for missing or altered plugins.
    //---------------------------------------

    for(filepath_set::iterator icfps = cache_fpset.begin(); icfps != cache_fpset.end(); ++icfps)
    {
      filepath_set::iterator ifpset = fpset.find(icfps->first);
      if(ifpset == fpset.end() || ifpset->second != icfps->second)
      {
        cache_dirty = true;

        if (debugStdErr) {
            std::fprintf(stderr, "Setting cache to dirty due to missing or modified plugins:\n");
            if(ifpset == fpset.end())
                std::fprintf(stderr, "Missing plugin: %s:\n", icfps->first.toLatin1().data());
            else
                std::fprintf(stderr, "Modified plugin: %s (Cache ts: %ld / File ts: %ld)\n",
                             icfps->first.toLatin1().data(),
                             icfps->second,
                             ifpset->second);
        }

        break;
      }
      // Done with the current plugin file path. Erase it.
      fpset.erase(ifpset);
    }

    //-------------------------
    // Check for new plugins.
    //-------------------------

    // Any remaining items in fpset must be 'new' plugins.
    if(!cache_dirty && !fpset.empty()) {
      if(debugStdErr)
      {
        std::fprintf(stderr, "Setting cache to dirty due to NEW plugins:\n");
        for (auto &plug: fpset) {
            std::fprintf(stderr, "New plugin %s:\n", plug.first.toLatin1().data());
        }
      }
      cache_dirty = true;
    }
  }

  // If ANY of the cache files do not exist we will recreate all of them.
  // The reason is that it might be good to show the user plugins found
  //  in folders which might not really belong there but ended up there
  //  perhaps by accident, like mixing your vst and linux vst plugins.
  // For that we must rescan everything even if only one cache file is missing.
  // If ANY of the caches are dirty or we are forcing recreation, create them now.
  if(!dontRecreate && (alwaysRecreate || cache_dirty))
  {
    if(debugStdErr)
      std::fprintf(stderr, "Re-scanning and creating plugin cache files...\n");

    list->clear();
    if(!createPluginCacheFiles(path, list, writePorts, museGlobalLib, types, debugStdErr))
    {
      res = false;
      std::fprintf(stderr, "checkPluginCacheFiles: createPluginCacheFiles() failed\n");
    }
  }

  // SPECIAL for LV2: Get rid of old cache file. Not used any more.
  const QString targ_filepath = path + "/" + QString(pluginCacheFilename(PluginScanInfoStruct::PluginTypeLV2));
  QFile targ_qfile(targ_filepath);
  if(targ_qfile.exists())
  {
    std::fprintf(stderr, "Deleting obsolete LV2 plugin cache file:%s\n",
                 targ_filepath.toLatin1().constData());
    if(!targ_qfile.remove())
      std::fprintf(stderr, "Error: Deleting obsolete LV2 plugin cache file failed!\n");
  }

  // SPECIAL for LV2: No need for a cache file.
  // Bring LV2 plugins directly into the list, after all the cache scanning and creation.
  if(types & PluginScanInfoStruct::PluginTypeLV2)
    scanLv2Plugins(list, writePorts, debugStdErr);

  return res;
}


} // namespace MusEPlugin

