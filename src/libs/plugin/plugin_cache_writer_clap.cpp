// config.h must be included BEFORE testing CLAP_SUPPORT: it is what defines the
// macro (via #cmakedefine). Without this, when the build relies on config.h
// rather than a -DCLAP_SUPPORT compile flag, the guard below would be false and
// writeClapInfo() would never be compiled (undefined reference at link time).
#include "config.h"

#ifdef CLAP_SUPPORT

#include <cstring>

#include <clap/clap.h>
#include <clap/factory/plugin-factory.h>

#include "plugin_cache_writer.h"  // writePluginScanInfo(), PluginScanInfoStruct
#include "plugin_cache_reader.h"  // setPluginScanFileInfo()
#include "plugin_scan.h"
#include "xml.h"

namespace MusEPlugin {

//---------------------------------------------------------
//   writeClapInfo
//   Serialise a CLAP factory's descriptors into the MusE XML
//   plugin cache. Mirrors writeMessInfo/writeDssiInfo: we fill a
//   PluginScanInfoStruct per descriptor and hand it to
//   writePluginScanInfo(), so the output format stays in sync with
//   what readPluginScanInfo() expects.
//
//   NOTE: do NOT hand-roll the XML here. The reader parses <type>/<class>
//   as integers and reads file/label as attributes of <plugin>; emitting
//   them any other way makes _type come back as 0 and the entries get
//   filtered out of the cache file (empty clap_plugins.scan).
//---------------------------------------------------------

bool writeClapInfo(const char* filename,
                   const clap_plugin_entry_t* entry,
                   bool /*do_ports*/,   // CLAP ports need instantiation; skip for now
                   int level,
                   MusECore::Xml& xml)
{
  entry->init(filename);

  const clap_plugin_factory_t* factory =
    static_cast<const clap_plugin_factory_t*>(
      entry->get_factory(CLAP_PLUGIN_FACTORY_ID));
  if(!factory)
  {
    std::fprintf(stderr, "writeClapInfo: no plugin factory in %s\n", filename);
    entry->deinit();
    return false;
  }

  const uint32_t count = factory->get_plugin_count(factory);
  for(uint32_t i = 0; i < count; ++i)
  {
    const clap_plugin_descriptor_t* desc =
      factory->get_plugin_descriptor(factory, i);
    if(!desc || !desc->id || !desc->name)
      continue;

    // Determine class from features[].
    PluginClass_t cls = PluginClassNone;
    if(desc->features)
    {
      for(int f = 0; desc->features[f]; ++f)
      {
        if(strcmp(desc->features[f], CLAP_PLUGIN_FEATURE_INSTRUMENT) == 0)
          cls = PluginClass_t(cls | PluginClassInstrument);
        if(strcmp(desc->features[f], CLAP_PLUGIN_FEATURE_AUDIO_EFFECT) == 0)
          cls = PluginClass_t(cls | PluginClassEffect);
      }
    }
    if(cls == PluginClassNone)
      cls = PluginClassInstrument;

    // Fill a scan info struct and let the common writer emit it,
    // guaranteeing the exact format readPluginScanInfo() parses.
    PluginScanInfoStruct info;
    setPluginScanFileInfo(QString::fromUtf8(filename), &info);

    info._type  = MusEPlugin::PluginTypeCLAP;
    info._class = cls;

    // CLAP's stable string id serves as both URI and label.
    info._uri   = PLUGIN_SET_CSTRING(desc->id);
    info._label = PLUGIN_SET_CSTRING(desc->id);

    info._name        = PLUGIN_SET_CSTRING(desc->name);
    info._description = PLUGIN_SET_CSTRING(desc->description ? desc->description : "");
    info._maker       = PLUGIN_SET_CSTRING(desc->vendor      ? desc->vendor      : "");
    info._version     = PLUGIN_SET_CSTRING(desc->version     ? desc->version     : "");

    // Port counts and GUI capability are only known after instantiation
    // (CLAP exposes them via instance extensions), so leave them at their
    // defaults here. ClapSynthIF fills the real values at init time, and
    // hasGui() is decided from the live _extGui pointer.

    writePluginScanInfo(level, xml, info, false);
  }

  entry->deinit();
  return true;
}

} // namespace MusEPlugin

#endif // CLAP_SUPPORT
