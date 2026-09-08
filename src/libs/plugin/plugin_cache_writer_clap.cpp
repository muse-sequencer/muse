// config.h must be included BEFORE testing CLAP_SUPPORT: it is what defines the
// macro (via #cmakedefine). Without this, when the build relies on config.h
// rather than a -DCLAP_SUPPORT compile flag, the guard below would be false and
// writeClapInfo() would never be compiled (undefined reference at link time).
#include "config.h"

#ifdef CLAP_SUPPORT

#include <cstring>
#include <cstdio>
#include <string>

#include <clap/clap.h>
#include <clap/factory/plugin-factory.h>
#include <clap/ext/audio-ports.h>
#include <clap/ext/params.h>

#include "plugin_cache_writer.h"  // writePluginScanInfo(), PluginScanInfoStruct
#include "plugin_cache_reader.h"  // setPluginScanFileInfo()
#include "plugin_scan.h"
#include "xml.h"

namespace MusEPlugin {

//---------------------------------------------------------
//   queryClapAudioPortCounts
//   Reads the channel count of the MAIN input/output audio port only —
//   mirrors ClapInstanceCore::init() (clap_host_lib_core.cpp), which is what
//   actually connects audio at runtime, so the cached counts match what a
//   live instance would report. Non-main ports (sidechains, extra outs)
//   have no routing concept in MusE's Pipeline/PluginI and are ignored here
//   too, same as at runtime.
//---------------------------------------------------------

static void queryClapAudioPortCounts(const clap_plugin_t* plugin,
                                     unsigned long* inports, unsigned long* outports,
                                     PluginPortList* portList)
{
  *inports = *outports = 0;

  const clap_plugin_audio_ports_t* ext_audio_ports =
    static_cast<const clap_plugin_audio_ports_t*>(
      plugin->get_extension(plugin, CLAP_EXT_AUDIO_PORTS));

  if(!ext_audio_ports)
  {
    // Safe fallback: assume a single stereo in and out port, matching
    // ClapInstanceCore::init()'s fallback for plugins without this extension.
    *inports  = 2;
    *outports = 2;
  }
  else
  {
    const uint32_t inCount  = ext_audio_ports->count(plugin, true);
    const uint32_t outCount = ext_audio_ports->count(plugin, false);

    int mainInIdx = -1, mainOutIdx = -1;
    for(uint32_t p = 0; p < inCount; ++p)
    {
      clap_audio_port_info_t pi{};
      if(ext_audio_ports->get(plugin, p, true, &pi) && (pi.flags & CLAP_AUDIO_PORT_IS_MAIN))
      {
        mainInIdx = static_cast<int>(p);
        *inports = pi.channel_count;
        break;
      }
    }
    // Defensive: if no port flagged main (spec violation, seen in the wild),
    // treat port 0 as main — same defensive fallback as ClapInstanceCore::init().
    if(mainInIdx < 0 && inCount > 0)
    {
      clap_audio_port_info_t pi{};
      if(ext_audio_ports->get(plugin, 0, true, &pi))
        *inports = pi.channel_count;
    }

    for(uint32_t p = 0; p < outCount; ++p)
    {
      clap_audio_port_info_t pi{};
      if(ext_audio_ports->get(plugin, p, false, &pi) && (pi.flags & CLAP_AUDIO_PORT_IS_MAIN))
      {
        mainOutIdx = static_cast<int>(p);
        *outports = pi.channel_count;
        break;
      }
    }
    if(mainOutIdx < 0 && outCount > 0)
    {
      clap_audio_port_info_t pi{};
      if(ext_audio_ports->get(plugin, 0, false, &pi))
        *outports = pi.channel_count;
    }
  }

  // One PluginPortInfo per channel — matches the per-channel fake LADSPA
  // ports ClapPluginWrapper::ClapPluginWrapper() builds at runtime, and the
  // "input"/"output" naming ClapPluginWrapper::instantiate() gives them.
  for(unsigned long i = 0; i < *inports; ++i)
  {
    PluginPortInfo port_info;
    port_info._index = static_cast<unsigned long>(portList->size());
    port_info._name  = PLUGIN_SET_STDSTRING("input" + std::to_string(i));
    port_info._type  = PluginPortInfo::AudioPort | PluginPortInfo::InputPort;
    portList->push_back(port_info);
  }
  for(unsigned long i = 0; i < *outports; ++i)
  {
    PluginPortInfo port_info;
    port_info._index = static_cast<unsigned long>(portList->size());
    port_info._name  = PLUGIN_SET_STDSTRING("output" + std::to_string(i));
    port_info._type  = PluginPortInfo::AudioPort | PluginPortInfo::OutputPort;
    portList->push_back(port_info);
  }
}

//---------------------------------------------------------
//   queryClapParams
//   Emits one PluginPortInfo per CLAP param (name/min/max/default) and
//   returns the param count.
//---------------------------------------------------------

static unsigned long queryClapParams(const clap_plugin_t* plugin, PluginPortList* portList)
{
  const clap_plugin_params_t* ext_params =
    static_cast<const clap_plugin_params_t*>(
      plugin->get_extension(plugin, CLAP_EXT_PARAMS));
  if(!ext_params)
    return 0;

  const uint32_t count = ext_params->count(plugin);
  for(uint32_t i = 0; i < count; ++i)
  {
    clap_param_info_t pi{};
    if(!ext_params->get_info(plugin, i, &pi))
      continue;

    PluginPortInfo port_info;
    port_info._index      = static_cast<unsigned long>(portList->size());
    port_info._name       = pi.name[0] ? PLUGIN_SET_CSTRING(pi.name) : PLUGIN_SET_STDSTRING("control" + std::to_string(i));
    port_info._type       = PluginPortInfo::ControlPort | PluginPortInfo::InputPort;
    port_info._min        = static_cast<float>(pi.min_value);
    port_info._max        = static_cast<float>(pi.max_value);
    port_info._defaultVal = static_cast<float>(pi.default_value);
    port_info._valueFlags = PluginPortInfo::HasMin | PluginPortInfo::HasMax | PluginPortInfo::HasDefault |
                            ((pi.flags & CLAP_PARAM_IS_STEPPED) ? PluginPortInfo::IntegerVal : PluginPortInfo::NoValueFlags);
    portList->push_back(port_info);
  }
  return count;
}

//---------------------------------------------------------
//   Minimal scan-only CLAP host vtable
//   Used only to instantiate a plugin long enough to query its port/param
//   counts, then destroy it again. This runs inside the sandboxed
//   muse_plugin_scan child process (see pluginScan() in
//   plugin_cache_writer.cpp) — a crash or hang here only takes down the
//   scan child, never the main application, unlike the old approach of
//   probing every plugin live inside MusE itself on every startup.
//---------------------------------------------------------

static const void* CLAP_ABI scanHostGetExtension(const clap_host_t*, const char*) { return nullptr; }
static void        CLAP_ABI scanHostRequestRestart(const clap_host_t*)  { }
static void        CLAP_ABI scanHostRequestProcess(const clap_host_t*)  { }
static void        CLAP_ABI scanHostRequestCallback(const clap_host_t*) { }

//---------------------------------------------------------
//   queryClapPortCounts
//   Instantiates one descriptor from an already-inited factory just long
//   enough to read its real port/param counts, then tears it down. Returns
//   false (leaving all counts at 0) if anything along the way fails; the
//   caller logs that rather than silently writing a bad cache entry.
//---------------------------------------------------------

static bool queryClapPortCounts(const clap_plugin_factory_t* factory,
                                const clap_plugin_descriptor_t* desc,
                                unsigned long* inports, unsigned long* outports,
                                unsigned long* controlInPorts, unsigned long* controlOutPorts,
                                PluginPortList* portList)
{
  *inports = *outports = *controlInPorts = *controlOutPorts = 0;
  portList->clear();

  if(!clap_version_is_compatible(desc->clap_version))
  {
    std::fprintf(stderr, "queryClapPortCounts: incompatible clap version for '%s'\n", desc->id);
    return false;
  }

  clap_host_t host{};
  host.clap_version     = CLAP_VERSION;
  host.host_data        = nullptr;
  host.name             = "MusE Plugin Scanner";
  host.vendor           = "MusE Team";
  host.url              = "https://muse-sequencer.org";
  host.version          = VERSION;
  host.get_extension    = scanHostGetExtension;
  host.request_restart  = scanHostRequestRestart;
  host.request_process  = scanHostRequestProcess;
  host.request_callback = scanHostRequestCallback;

  const clap_plugin_t* plugin = factory->create_plugin(factory, &host, desc->id);
  if(!plugin)
  {
    std::fprintf(stderr, "queryClapPortCounts: create_plugin() returned nullptr for '%s'\n", desc->id);
    return false;
  }

  if(!plugin->init(plugin))
  {
    std::fprintf(stderr, "queryClapPortCounts: plugin->init() failed for '%s'\n", desc->id);
    plugin->destroy(plugin);
    return false;
  }

  queryClapAudioPortCounts(plugin, inports, outports, portList);
  *controlInPorts  = queryClapParams(plugin, portList);
  *controlOutPorts = 0; // CLAP has no explicit output-only params.

  plugin->destroy(plugin);
  return true;
}

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
                   bool do_ports,
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

    // Port/param counts: only knowable by instantiating the plugin. When
    // do_ports is requested, do that instantiation HERE — once, offline,
    // inside this sandboxed scan process — so the real MusE process can
    // read the counts straight out of the cache instead of instantiating
    // every scanned CLAP plugin itself on every startup (see
    // ClapPluginWrapper::ClapPluginWrapper() in clap_host_effect.cpp, which
    // now trusts these cached values and only falls back to a live probe
    // if they come back all zero).
    //
    // GUI capability is still left undetermined here (CLAP_EXT_GUI is only
    // checked once something actually instantiates for real) —
    // ClapPluginWrapper::hasNativeGui() conservatively assumes true and
    // ClapInstanceCore::showNativeGui() is already a safe no-op if there's
    // no GUI extension.
    if(do_ports)
    {
      unsigned long inports = 0, outports = 0, controlInPorts = 0, controlOutPorts = 0;
      if(queryClapPortCounts(factory, desc, &inports, &outports, &controlInPorts, &controlOutPorts, &info._portList))
      {
        info._inports         = inports;
        info._outports        = outports;
        info._controlInPorts  = controlInPorts;
        info._controlOutPorts = controlOutPorts;
        // Derive from the list we just filled rather than re-summing the
        // counts separately, so this can never mismatch info._portList.size()
        // (writePluginScanInfo() checks that and skips the per-port XML
        // block, with a stderr error, if they disagree).
        info._portCount = static_cast<unsigned long>(info._portList.size());
      }
      else
      {
        std::fprintf(stderr, "writeClapInfo: port query failed for '%s' in %s — leaving port counts at 0\n",
                     desc->id, filename);
      }
    }

    writePluginScanInfo(level, xml, info, do_ports);
  }

  entry->deinit();
  return true;
}

} // namespace MusEPlugin

#endif // CLAP_SUPPORT
