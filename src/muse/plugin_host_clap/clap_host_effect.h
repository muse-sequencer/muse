//=============================================================================
//  MusE
//  Linux Music Editor
//
//  clap_host_effect.h
//  CLAP (CLever Audio Plugin) host integration for MusE — effect-rack side.
//  Presents a ClapSynth-backed CLAP plugin as a LADSPA-shaped Plugin so it
//  can sit in MusE's effect rack (Pipeline/PluginI), same role as
//  VstNativePluginWrapper (vst_native.h) plays for native VST effects.
//  Reuses the same ClapSynth descriptor and ClapInstanceCore as the
//  synth-track side (clap_host_synth.h / clap_host_lib.h) — only the LADSPA-facing
//  adapter is new here.
//
//  IMPORTANT — companion edit required in plugin.h:
//  PluginI declares "friend class VstNativePluginWrapper;" (and the LV2
//  equivalent) so those wrapper classes can reach PluginI's private
//  instances/handle[] array for GUI dispatch. ClapPluginWrapper needs the
//  same: add
//      friend class ClapPluginWrapper;
//  to the PluginI class definition in plugin.h, alongside the existing
//  VstNativePluginWrapper/LV2PluginWrapper friend lines. Not done here since
//  plugin.h wasn't part of this pass — showNativeGui()/nativeGuiVisible()
//  below won't compile without it.
//
//  (C) Copyright 2024 MusE contributors
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License
//  as published by the Free Software Foundation; version 2 of
//  the License, or (at your option) any later version.
//=============================================================================

#pragma once

#include "config.h"
#ifdef CLAP_SUPPORT

#include <vector>
#include <string>

#include <QObject>
#include <QString>

#include <clap/clap.h>

class ClapSynth; // forward decl only — only ever used as a pointer here; the
                  // full definition (clap_host_synth.h) is only needed where
                  // it's actually dereferenced, in clap_host_effect.cpp.
                  

#include "clap_host_lib.h" // ClapInstanceCore
#include "plugin.h"        // Plugin, PluginI

namespace MusECore {

class ClapPluginWrapper;

//---------------------------------------------------------
//   ClapPluginWrapper_State
//   Per-instance handle returned as the opaque LADSPA_Handle from
//   ClapPluginWrapper::instantiate(). Mirrors VstNativePluginWrapper_State:
//   holds the live ClapInstanceCore plus the raw float* pointers connected
//   via Plugin::connectPort(), in the synthetic LADSPA port order
//   (audio-in, audio-out, control-in).
//---------------------------------------------------------

class ClapPluginWrapper_State
{
public:
  ClapInstanceCore     core;
  ClapPluginWrapper*   pluginWrapper = nullptr;
  PluginI*             pluginI       = nullptr;

  std::vector<float*> inPorts;
  std::vector<float*> outPorts;
  std::vector<float*> inControlPorts;

  // Last value pushed to the plugin for each control port, so apply() only
  // emits a CLAP_EVENT_PARAM_VALUE when the connected pointer's value has
  // actually changed since the previous apply() call — connectPort() only
  // ever hands us a persistent pointer, PluginI writes new automation values
  // into it between calls, we never see the write itself.
  std::vector<double> lastControlVal;

  bool guiVisible = false;

  ClapPluginWrapper_State() = default;
  ~ClapPluginWrapper_State() = default;
};

//---------------------------------------------------------
//   ClapPluginWrapper
//---------------------------------------------------------

class ClapPluginWrapper : public Plugin
{
  friend class ClapPluginWrapper_State;

public:
  explicit ClapPluginWrapper(ClapSynth* s,
                             MusEPlugin::PluginFeatures_t reqFeatures = MusEPlugin::PluginNoFeatures);
  ~ClapPluginWrapper() override;

  ClapSynth* synth() const { return _synth; }

  LADSPA_Handle instantiate(PluginI*) override;
  bool reference() override;
  int  release()   override;
  void activate(LADSPA_Handle handle)   override;
  void deactivate(LADSPA_Handle handle) override;
  void cleanup(LADSPA_Handle handle)    override;
  void connectPort(LADSPA_Handle handle, unsigned long port, float* value) override;
  void apply(LADSPA_Handle handle, unsigned long n, float latency_corr = 0.0f) override;

  LADSPA_PortDescriptor portd(unsigned long k) const override;
  LADSPA_PortRangeHint  range(unsigned long i) const override;
  void   range(unsigned long i, float* min, float* max) const override;
  double defaultValue(unsigned long port) const override;
  const char* portName(unsigned long port) const override;
  CtrlValueType  ctrlValueType(unsigned long i) const override;
  CtrlList::Mode ctrlMode(unsigned long i) const override;

  // Not Plugin virtuals (Plugin's base interface has no GUI concept — LADSPA
  // has none). Called by PluginI via a pluginType()-dispatched downcast, same
  // as VstNativePluginWrapper's equivalents; see the plugin.h note above.
  bool hasNativeGui() const;
  void showNativeGui(PluginI* p, bool bShow);
  bool nativeGuiVisible(const PluginI* p) const;

  QString getCustomConfiguration(LADSPA_Handle handle);
  bool setCustomData(LADSPA_Handle handle, const std::vector<QString>& customParams);

private:
  ClapSynth* _synth;
  LADSPA_Descriptor       _fakeLd{};
  LADSPA_PortDescriptor*  _fakePds = nullptr;
  std::vector<std::string> portNames;
};

//---------------------------------------------------------
//   initCLAPEffects
//   Registers effect-class CLAP plugins from the scan list into
//   MusEGlobal::plugins, mirroring initVST_Native()'s add_plug logic.
//   Independent of initCLAP() (clap_host_synth.h), which registers
//   instrument-class plugins into MusEGlobal::synthis — a plugin marked as
//   both classes gets registered by both, each with its own ClapSynth
//   instance (own QLibrary/reference count; see the .cpp for the tradeoff).
//---------------------------------------------------------
void initCLAPEffects();

} // namespace MusECore

#endif // CLAP_SUPPORT
