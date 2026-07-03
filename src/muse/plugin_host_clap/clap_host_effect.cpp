//=============================================================================
//  MusE
//  Linux Music Editor
//
//  clap_host_effect.cpp
//  CLAP host integration for MusE — effect-rack side. See clap_host_effect.h
//  for the class contract and the required plugin.h friend-declaration note.
//
//  (C) Copyright 2024 MusE contributors
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License
//  as published by the Free Software Foundation; version 2 of
//  the License, or (at your option) any later version.
//=============================================================================

#include "config.h"
#ifdef CLAP_SUPPORT

// Turn on debugging messages
//#define CLAP_DEBUG

#include <cassert>
#include <cstdio>
#include <cstring>
#include <sstream>

#include <clap/clap.h>
#include <clap/ext/params.h>

#include "clap_host_synth.h" // ClapSynth — actually dereferenced below (_synth->...)
#include "clap_host_effect.h"
#include "globals.h"
#include "gconfig.h"
#include "pluglist.h"

namespace MusECore {

//---------------------------------------------------------
//   initCLAPEffects
//---------------------------------------------------------

void initCLAPEffects()
{
  const MusEPlugin::PluginScanList& scan_list = MusEPlugin::pluginList;
  for(MusEPlugin::ciPluginScanList isl = scan_list.begin(); isl != scan_list.end(); ++isl)
  {
    const MusEPlugin::PluginScanInfoRef inforef = *isl;
    const MusEPlugin::PluginScanInfoStruct& info = inforef->info();
    if(info._type != MusEPlugin::PluginTypeCLAP)
      continue;
    if(!MusEGlobal::loadCLAP)
      continue;
    // Only effect-class plugins go into plugins. A plugin marked as both
    // instrument and effect also gets registered by initCLAP() into
    // MusEGlobal::synthis, independently.
    if(!(info._class & MusEPlugin::PluginClassEffect))
      continue;

    const QString inf_cbname = PLUGIN_GET_QSTRING(info._completeBaseName);
    const QString inf_name   = PLUGIN_GET_QSTRING(info._name);
    const QString inf_label  = PLUGIN_GET_QSTRING(info._label);
    const QString inf_uri    = PLUGIN_GET_QSTRING(info._uri);
    const QString inf_filepath = PLUGIN_GET_QSTRING(info.filePath());

    if(const Plugin* pl = MusEGlobal::plugins.find(info._type, inf_cbname, inf_uri, inf_label))
    {
      if(MusEGlobal::debugMsg && !MusEGlobal::suppressPluginDuplicateWarnings)
        fprintf(stderr,
          "Ignoring CLAP effect label:%s uri:%s path:%s duplicate of path:%s\n",
          inf_label.toLocal8Bit().constData(),
          inf_uri.toLocal8Bit().constData(),
          inf_filepath.toLocal8Bit().constData(),
          pl->filePath().toLocal8Bit().constData());
      continue;
    }

    if(MusEGlobal::debugMsg)
      fprintf(stderr, "initCLAPEffects: adding CLAP effect plugin:%s name:%s\n",
              inf_filepath.toLocal8Bit().constData(),
              inf_name.toLocal8Bit().constData());

    ClapSynth* s = new ClapSynth(info);
    MusEGlobal::plugins.push_back(new ClapPluginWrapper(s, info._requiredFeatures));
  }
}

//---------------------------------------------------------
//   ClapPluginWrapper
//---------------------------------------------------------

ClapPluginWrapper::ClapPluginWrapper(ClapSynth* s, MusEPlugin::PluginFeatures_t reqFeatures)
  : Plugin()
{
  _synth = s;
  _requiredFeatures = reqFeatures;

  // CLAP's real port/param counts are only knowable from a live plugin
  // instance (unlike LV2's static RDF metadata, or VST's lightweight AEffect
  // struct). plugin_cache_writer_clap.cpp's writeClapInfo() now instantiates
  // each descriptor once, offline, inside the sandboxed muse_plugin_scan
  // process, and writes the real counts into the scan cache. ClapSynth's
  // constructor already copies info._inports/_outports/_controlInPorts/
  // _controlOutPorts into its own members (clap_host_synth.cpp), so at this
  // point — before anything has called _synth->reference() — those values
  // already hold whatever the cache recorded, with zero extra dlopen/
  // instantiate/destroy work needed for the common case.
  //
  // This used to run a disposable probe instance (dlopen + create_plugin +
  // init + destroy + dlclose) for EVERY scanned CLAP plugin on EVERY MusE
  // startup, regardless of whether the current project even used it — the
  // dominant source of dlopen/init noise (and dlopen-time "definitely lost"
  // reports from third-party plugins' own static init) seen under valgrind.
  //
  // Fallback: if the cache has nothing for this plugin (stale cache from
  // before this fix, plugin file changed since the last scan, or the scan
  // tool's port query itself failed and legitimately left them at 0 — see
  // queryClapPortCounts() in plugin_cache_writer_clap.cpp), fall back to the
  // old disposable-probe behavior so _fakePds doesn't end up sized 0, which
  // previously corrupted port classification and crashed inside plugins
  // like ZamDelay/ZamCompX2 (see portd()/ports() below).
  if(_synth->inPorts() == 0 && _synth->outPorts() == 0 && _synth->inControls() == 0)
  {
    fprintf(stderr, "ClapPluginWrapper::ClapPluginWrapper: '%s' has no cached port/param "
            "counts (stale or missing scan cache?) — falling back to a live probe. "
            "Consider re-running the plugin scan.\n",
            _synth->name().toLocal8Bit().constData());

    ClapInstanceCore probe;
    if(_synth->reference())
    {
      if(!probe.init(_synth, _synth->name()))
        fprintf(stderr, "ClapPluginWrapper::ClapPluginWrapper: probe instantiation failed for '%s' "
                "— port/param counts will be 0\n", _synth->name().toLocal8Bit().constData());
      else
        probe.shutdown();
      _synth->release();
    }
    else
      fprintf(stderr, "ClapPluginWrapper::ClapPluginWrapper: _synth->reference() failed for '%s' "
              "— port/param counts will be 0\n", _synth->name().toLocal8Bit().constData());
  }

  _fakeLd.Label     = strdup(_synth->label().toUtf8().constData());
  _fakeLd.Name      = strdup(_synth->name().toUtf8().constData());
  _fakeLd.UniqueID  = _synth->id();
  _fakeLd.Maker     = strdup(_synth->maker().toUtf8().constData());
  _fakeLd.Copyright = strdup(_synth->version().toUtf8().constData());

  _pluginType  = MusEPlugin::PluginTypeCLAP;
  _pluginClass = s->pluginClass();

  const unsigned long numPorts =
    _synth->inPorts() + _synth->outPorts() + _synth->inControls();
  _fakeLd.PortCount = numPorts;
  _fakePds = new LADSPA_PortDescriptor[numPorts];
  memset(_fakePds, 0, sizeof(LADSPA_PortDescriptor) * numPorts);

  for(unsigned long i = 0; i < _synth->inPorts(); ++i)
    _fakePds[i] = LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO;
  for(unsigned long i = 0; i < _synth->outPorts(); ++i)
    _fakePds[i + _synth->inPorts()] = LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO;
  for(unsigned long i = 0; i < _synth->inControls(); ++i)
    _fakePds[i + _synth->inPorts() + _synth->outPorts()] = LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;

  _fakeLd.PortNames       = nullptr;
  _fakeLd.PortRangeHints  = nullptr;
  _fakeLd.PortDescriptors = _fakePds;
  _fakeLd.Properties      = 0;
  plugin = &_fakeLd;

  // Built from the public filePath() string rather than reading _synth's
  // protected _fileInfo directly — avoids needing friend access to ClapSynth
  // for something PluginBase already exposes.
  _fileInfo    = QFileInfo(_synth->filePath());
  _uri         = _synth->uri();
  _label       = _synth->label();
  _name        = _synth->name();
  _description = _synth->description();
  _uniqueID    = plugin->UniqueID;
  _maker       = _synth->maker();
  _copyright   = _synth->version();

  _pluginFreewheelType        = _synth->pluginFreewheelType();
  _freewheelPortIndex         = _synth->freewheelPortIndex();
  _pluginLatencyReportingType = _synth->pluginLatencyReportingType();
  _latencyPortIndex           = _synth->latencyPortIndex();
  _pluginBypassType           = _synth->pluginBypassType();
  _enableOrBypassPortIndex    = _synth->enableOrBypassPortIndex();

  _portCount = plugin->PortCount;
  _inports         = _synth->inPorts();
  _outports        = _synth->outPorts();
  _controlInPorts  = _synth->inControls();
  _controlOutPorts = 0; // CLAP has no explicit output-only params
}

ClapPluginWrapper::~ClapPluginWrapper()
{
  free((void*)_fakeLd.Label);
  free((void*)_fakeLd.Name);
  free((void*)_fakeLd.Maker);
  free((void*)_fakeLd.Copyright);
  delete[] _fakePds;

  // Plugin::plugin points at our own _fakeLd member (set in the ctor), not a
  // heap LADSPA_Descriptor. ~Plugin only warns/deletes when it's non-NULL and
  // the type isn't LV2/LinuxVST — CLAP isn't in that skip list — so clear it
  // here to avoid the false "plugin is not NULL" leak warning (and any future
  // delete of a non-heap pointer).
  plugin = nullptr;
}

//---------------------------------------------------------
//   reference / release
//   Delegate straight to the shared ClapSynth descriptor.
//---------------------------------------------------------

bool ClapPluginWrapper::reference() { return _synth->reference(); }
int  ClapPluginWrapper::release()   { return _synth->release();   }

//---------------------------------------------------------
//   instantiate
//---------------------------------------------------------

LADSPA_Handle ClapPluginWrapper::instantiate(PluginI* pluginI)
{
  // Ensures _synth->_factory/_desc are actually resolved (dlopen + clap_entry
  // lookup) before core.init() touches them. ClapSynth::createSIF() does the
  // same on the synth-track side — this call was missing here, which left
  // _factory null and crashed in ClapInstanceCore::init().
  if(!_synth->reference())
  {
    fprintf(stderr, "ClapPluginWrapper::instantiate: _synth->reference() failed for '%s'\n",
            name().toLocal8Bit().constData());
    return nullptr;
  }

  ClapPluginWrapper_State* state = nullptr;
  try
  {
    state = new ClapPluginWrapper_State;
  }
  catch(const std::bad_alloc& e)
  {
    fprintf(stderr, "ClapPluginWrapper::instantiate: out of memory (%s)\n", e.what());
    _synth->release();
    return nullptr;
  }

  if(!state->core.init(_synth, name()))
  {
    fprintf(stderr, "ClapPluginWrapper::instantiate: core.init() failed for '%s'\n",
            name().toLocal8Bit().constData());
    delete state;
    _synth->release();
    return nullptr;
  }

  state->pluginI       = pluginI;
  state->pluginWrapper = this;

  const unsigned long nIn  = _synth->inPorts();
  const unsigned long nOut = _synth->outPorts();
  const unsigned long nCtl = _synth->inControls();

  state->inPorts.assign(nIn,  nullptr);
  state->outPorts.assign(nOut, nullptr);
  state->inControlPorts.assign(nCtl, nullptr);
  state->lastControlVal.assign(nCtl, 0.0);
  for(unsigned long i = 0; i < nCtl && i < _synth->paramInfo.size(); ++i)
    state->lastControlVal[i] = _synth->paramInfo[i].default_value;

  portNames.resize(nIn + nOut + nCtl);
  for(size_t i = 0; i < portNames.size(); ++i)
  {
    if(i < nIn)
    {
      std::ostringstream ss;
      ss << "input" << i;
      portNames[i] = ss.str();
    }
    else if(i < nIn + nOut)
    {
      std::ostringstream ss;
      ss << "output" << (i - nIn);
      portNames[i] = ss.str();
    }
    else
    {
      const unsigned long ci = i - nIn - nOut;
      if(ci < _synth->paramInfo.size() && _synth->paramInfo[ci].name[0])
        portNames[i] = _synth->paramInfo[ci].name;
      else
      {
        std::ostringstream ss;
        ss << "control" << ci;
        portNames[i] = ss.str();
      }
    }
  }

  // Plugin/window-manager-initiated GUI close (not destroyed) — mirrors
  // ClapSynthIF's showNativeGuiPending(false) wiring. Defined inside this
  // member function so it runs with ClapPluginWrapper's access rights (the
  // friend declaration plugin.h needs — see clap_host_effect.h) rather than
  // ClapPluginWrapper_State's.
  state->core.setGuiClosedCallback(
    [pluginI]()
    {
      if(pluginI)
        pluginI->showNativeGuiPending(false);
    });

  return static_cast<LADSPA_Handle>(state);
}

//---------------------------------------------------------
//   activate / deactivate / cleanup
//---------------------------------------------------------

void ClapPluginWrapper::activate(LADSPA_Handle handle)
{
  ClapPluginWrapper_State* state = static_cast<ClapPluginWrapper_State*>(handle);
  if(!state)
  {
    fprintf(stderr, "ClapPluginWrapper::activate: handle is nullptr\n");
    return;
  }
  state->core.activate();
}

void ClapPluginWrapper::deactivate(LADSPA_Handle handle)
{
  ClapPluginWrapper_State* state = static_cast<ClapPluginWrapper_State*>(handle);
  if(!state)
  {
    fprintf(stderr, "ClapPluginWrapper::deactivate: handle is nullptr\n");
    return;
  }
  state->core.deactivate();
}

void ClapPluginWrapper::cleanup(LADSPA_Handle handle)
{
  ClapPluginWrapper_State* state = static_cast<ClapPluginWrapper_State*>(handle);
  if(!state)
    return;
  state->core.shutdown();
  delete state;
  // Balances the _synth->reference() taken in instantiate().
  _synth->release();
}

//---------------------------------------------------------
//   connectPort
//   LADSPA-style: just remember the pointer. PluginI writes automation
//   values into it between apply() calls; we read whatever's there when
//   apply() runs.
//---------------------------------------------------------

void ClapPluginWrapper::connectPort(LADSPA_Handle handle, unsigned long port, float* value)
{
  ClapPluginWrapper_State* state = static_cast<ClapPluginWrapper_State*>(handle);
  if(!state)
    return;

  const unsigned long nIn  = _synth->inPorts();
  const unsigned long nOut = _synth->outPorts();
  const unsigned long nCtl = _synth->inControls();

  if(port < nIn)
    state->inPorts[port] = value;
  else if(port < nIn + nOut)
    state->outPorts[port - nIn] = value;
  else if(port < nIn + nOut + nCtl)
    state->inControlPorts[port - nIn - nOut] = value;
}

//---------------------------------------------------------
//   apply
//   Diffs the connected control pointers against their last-sent value
//   (emitting a CLAP_EVENT_PARAM_VALUE at offset 0 for anything changed),
//   then runs the plugin for n frames using the connected audio pointers.
//---------------------------------------------------------

void ClapPluginWrapper::apply(LADSPA_Handle handle, unsigned long n, float /*latency_corr*/)
{
  ClapPluginWrapper_State* state = static_cast<ClapPluginWrapper_State*>(handle);
  if(!state)
    return;

  const unsigned long nCtl = _synth->inControls();
  for(unsigned long i = 0; i < nCtl; ++i)
  {
    float* p = state->inControlPorts[i];
    if(!p)
      continue;
    const double val = static_cast<double>(*p);
    if(val != state->lastControlVal[i])
    {
      state->core.pushParamValueEvent(_synth->paramIds[i], val, 0);
      state->lastControlVal[i] = val;
    }
  }

  const unsigned long nIn  = _synth->inPorts();
  const unsigned long nOut = _synth->outPorts();

  // steady_time is unknown at this level (Plugin::apply() isn't given an
  // absolute frame position — same limitation VstNativePluginWrapper::apply()
  // has); -1 is the CLAP-defined "unknown" sentinel.
  state->core.runProcess(-1, n,
                         nIn  > 0 ? state->inPorts.data()  : nullptr, static_cast<uint32_t>(nIn),
                         nOut > 0 ? state->outPorts.data() : nullptr, static_cast<uint32_t>(nOut));
}

//---------------------------------------------------------
//   Port introspection
//---------------------------------------------------------

LADSPA_PortDescriptor ClapPluginWrapper::portd(unsigned long k) const
{
  return _fakeLd.PortDescriptors[k];
}

LADSPA_PortRangeHint ClapPluginWrapper::range(unsigned long i) const
{
  LADSPA_PortRangeHint hint{};
  hint.HintDescriptor = LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE;

  const unsigned long nIn  = _synth->inPorts();
  const unsigned long nOut = _synth->outPorts();
  if(i >= nIn + nOut && i < nIn + nOut + _synth->inControls() &&
     (i - nIn - nOut) < _synth->paramInfo.size())
  {
    const clap_param_info_t& pi = _synth->paramInfo[i - nIn - nOut];
    hint.LowerBound = static_cast<float>(pi.min_value);
    hint.UpperBound = static_cast<float>(pi.max_value);
  }
  else
  {
    hint.LowerBound = 0.0f;
    hint.UpperBound = 1.0f;
  }
  return hint;
}

void ClapPluginWrapper::range(unsigned long i, float* min, float* max) const
{
  const unsigned long nIn  = _synth->inPorts();
  const unsigned long nOut = _synth->outPorts();
  if(i >= nIn + nOut && i < nIn + nOut + _synth->inControls() &&
     (i - nIn - nOut) < _synth->paramInfo.size())
  {
    const clap_param_info_t& pi = _synth->paramInfo[i - nIn - nOut];
    *min = static_cast<float>(pi.min_value);
    *max = static_cast<float>(pi.max_value);
  }
  else
  {
    *min = 0.0f;
    *max = 1.0f;
  }
}

double ClapPluginWrapper::defaultValue(unsigned long port) const
{
  const unsigned long nIn  = _synth->inPorts();
  const unsigned long nOut = _synth->outPorts();
  if(port >= nIn + nOut && port < nIn + nOut + _synth->inControls() &&
     (port - nIn - nOut) < _synth->paramInfo.size())
    return _synth->paramInfo[port - nIn - nOut].default_value;
  return 0.0;
}

const char* ClapPluginWrapper::portName(unsigned long port) const
{
  if(port >= portNames.size())
    return "";
  return portNames[port].c_str();
}

CtrlValueType ClapPluginWrapper::ctrlValueType(unsigned long i) const
{
  const unsigned long nIn  = _synth->inPorts();
  const unsigned long nOut = _synth->outPorts();
  if(i >= nIn + nOut && i < nIn + nOut + _synth->inControls() &&
     (i - nIn - nOut) < _synth->paramInfo.size())
    return (_synth->paramInfo[i - nIn - nOut].flags & CLAP_PARAM_IS_STEPPED) ? VAL_INT : VAL_LINEAR;
  return VAL_LINEAR;
}

CtrlList::Mode ClapPluginWrapper::ctrlMode(unsigned long /*i*/) const
{
  return CtrlList::INTERPOLATE;
}

//---------------------------------------------------------
//   GUI
//---------------------------------------------------------

bool ClapPluginWrapper::hasNativeGui() const
{
  // No pre-instantiation GUI flag from the CLAP scan cache yet (see the TODO
  // in plugin_cache_writer_clap.cpp — CLAP GUI support is only knowable after
  // instantiation). Conservative default: report true; if the plugin turns
  // out to have no clap.gui extension, ClapInstanceCore::showNativeGui() is
  // already a safe no-op.
  return true;
}

void ClapPluginWrapper::showNativeGui(PluginI* p, bool bShow)
{
  assert(p->instances > 0);
  ClapPluginWrapper_State* state = static_cast<ClapPluginWrapper_State*>(p->handle[0]);
  if(!state)
    return;
  state->core.showNativeGui(bShow);
  state->guiVisible = bShow;
}

bool ClapPluginWrapper::nativeGuiVisible(const PluginI* p) const
{
  assert(p->instances > 0);
  const ClapPluginWrapper_State* state = static_cast<const ClapPluginWrapper_State*>(p->handle[0]);
  return state ? state->core.nativeGuiVisible() : false;
}

//---------------------------------------------------------
//   State persistence
//---------------------------------------------------------

QString ClapPluginWrapper::getCustomConfiguration(LADSPA_Handle handle)
{
  ClapPluginWrapper_State* state = static_cast<ClapPluginWrapper_State*>(handle);
  if(!state)
    return QString();
  const std::vector<QString> d = state->core.getCustomData();
  return d.empty() ? QString() : d.front();
}

bool ClapPluginWrapper::setCustomData(LADSPA_Handle handle, const std::vector<QString>& customParams)
{
  ClapPluginWrapper_State* state = static_cast<ClapPluginWrapper_State*>(handle);
  if(!state)
    return false;

  const bool ok = state->core.setCustomData(customParams);
  if(ok)
  {
    // Resync the last-sent-value shadow so apply() doesn't immediately
    // re-push stale automation over the just-restored state.
    const unsigned long nCtl = _synth->inControls();
    for(unsigned long i = 0; i < nCtl && i < _synth->paramIds.size(); ++i)
      state->lastControlVal[i] = state->core.getParameter(_synth->paramIds[i]);
  }
  return ok;
}

} // namespace MusECore

#endif // CLAP_SUPPORT
