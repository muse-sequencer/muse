//=============================================================================
//=============================================================================
//  MusE
//  Linux Music Editor
//
//  clap_host_synth.h
//  CLAP (CLever Audio Plugin) host integration for MusE — synth-track side.
//  Thin SynthIF adapter over the shared ClapInstanceCore (clap_host_lib.h);
//  everything CLAP-mechanics-specific (instantiation, extensions, event
//  buffer, process(), state, GUI/timer/fd) lives in ClapInstanceCore. This
//  file only translates MusE's SynthIF/track world (MidiPlayEvent stream,
//  automation Port[] cache, song-file custom data) to/from it. See
//  clap_host_effect.h for the effect-rack equivalent, which shares the same
//  ClapSynth descriptor and ClapInstanceCore.
//
//  Copyright (C) 2024 MusE contributors
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; version 2 of the License.
//=============================================================================

#pragma once

#include "config.h"
#ifdef CLAP_SUPPORT

#include <vector>
#include <unordered_map>
#include <cstdint>
#include <atomic>

#include <QLibrary>

#include <clap/clap.h>
#include <clap/ext/params.h>

#include "synth.h"
#include "plugin.h"
#include "ctrl.h"
#include "clap_host_lib.h"

namespace MusEGui { class PopupMenu; }

namespace MusECore {

class ClapSynthIF;

//---------------------------------------------------------
//   ClapSynth
//   Represents a loaded CLAP plugin descriptor (factory metadata).
//   One per discovered plugin id in the scan list.
//   Multiple instances share one ClapSynth via reference counting.
//   Reused as-is by both the synth-track path (this file) and the
//   effect-rack path (clap_host_effect.cpp), same as VstNativeSynth is
//   shared by VstNativeSynthIF and VstNativePluginWrapper — the effect side
//   only touches ClapSynth's already-public members (inPorts()/outPorts()/
//   paramInfo/etc.), so no extra friendship is needed for it here.
//   Note: _qlib, _references, _label, _uri etc. come from PluginBase.
//         _portCount, _inports etc. are NOT in PluginBase/Synth (they live
//         in Plugin which is a sibling branch), so we declare them here.
//---------------------------------------------------------

class ClapSynth : public Synth {
  friend class ClapSynthIF;
  friend class ClapInstanceCore; // init() fills port/param metadata directly

public:
  ClapSynth(const MusEPlugin::PluginScanInfoStruct& info);
  ~ClapSynth() override;

  SynthIF* createSIF(SynthI*) override;

  // Reference-counted library load/unload (implements PluginBase pure virtuals)
  bool reference() override;
  int  release()   override;

  // Port counts — not in PluginBase/Synth, declared here like DssiSynth does
  unsigned long _portCount      = 0;
  unsigned long _inports        = 0;
  unsigned long _outports       = 0;
  unsigned long _controlInPorts = 0;
  unsigned long _controlOutPorts= 0;

  // Convenience accessors matching DssiSynth pattern
  unsigned long inPorts()    const { return _inports; }
  unsigned long outPorts()   const { return _outports; }
  unsigned long inControls() const { return _controlInPorts; }

  // Parameter metadata, filled in ClapInstanceCore::init() after instantiation
  std::vector<clap_id>           paramIds;
  std::vector<clap_param_info_t> paramInfo;
  std::unordered_map<clap_id, unsigned long> paramIdToIndex;

  // unused??? — kept for API consistency with DssiSynth port index vectors
  std::vector<unsigned long> iIdx;
  std::vector<unsigned long> oIdx;

private:
  const clap_plugin_entry_t*      _entry;   ///< resolved 'clap_entry' symbol
  const clap_plugin_factory_t*    _factory; ///< from _entry->get_factory()
  const clap_plugin_descriptor_t* _desc;    ///< descriptor matching our _label (id)
  // Note: _qlib inherited from PluginBase
};

//---------------------------------------------------------
//   ClapSynthIF
//   One running CLAP plugin instance inside MusE, wrapping a
//   ClapInstanceCore. One per active SynthI track using a ClapSynth.
//---------------------------------------------------------

class ClapSynthIF : public SynthIF {
public:
  explicit ClapSynthIF(SynthI* s);
  ~ClapSynthIF() override;

  //--- Initialisation ---
  bool init(ClapSynth* s);

  //--- SynthIF pure virtuals ---
  bool getData(MidiPort*, unsigned pos, int ports,
               unsigned nframes, float** buffer) override;
  MidiPlayEvent receiveEvent() override;
  int  eventsPending() const override { return 0; }
  int  channels()         const override;
  int  totalOutChannels() const override;
  int  totalInChannels()  const override;
  void deactivate3() override;
  QString getPatchName(int, int, bool) const override;
  void populatePatchPopup(MusEGui::PopupMenu*, int, bool) override;

  void read(Xml& xml);          // unused??? — superseded by getCustomData()/setCustomData()
  void write(int level, Xml& xml) const override; // unused??? — superseded, see .cpp

  // CLAP state persistence goes through MusE's custom-data framework (same as
  // LV2/VST): the blob is base64-encoded into <customData> and applied AFTER
  // initInstance() via configure()/setCustomData(). This is required because
  // _sif does not exist yet while SynthI::read() parses the section.
  std::vector<QString> getCustomData() const override;
  bool setCustomData(const std::vector<QString>&) override;

  double getParameter(unsigned long n) const override;
  void   setParameter(unsigned long n, double v) override;
  int    getControllerInfo(int id, QString* name,
                           int* ctrl, int* min, int* max, int* initval) override;

  bool hasGui()       const override { return _core.hasGui(); }
  bool hasNativeGui() const override { return _core.hasGui(); }

  //--- GUI ---
  bool nativeGuiVisible() const override { return _core.nativeGuiVisible(); }
  void showNativeGui(bool v) override;
  void closeNativeGui();

  //--- PluginIBase pure virtuals ---
  unsigned long pluginID() const override;
  void     enableController(unsigned long i, bool v) override;
  bool     controllerEnabled(unsigned long i) const override;
  void     enableAllControllers(bool v) override;
  void     activate() override;
  void     deactivate() override;
  unsigned long parameters()    const override;
  unsigned long parametersOut() const override;
  void     setParam(unsigned long i, double v) override;
  double   param(unsigned long i)    const override;
  double   paramOut(unsigned long i) const override;
  const char* paramName(unsigned long i)    const override;
  const char* paramOutName(unsigned long i) const override;
  LADSPA_PortRangeHint range(unsigned long i)    const override;
  LADSPA_PortRangeHint rangeOut(unsigned long i) const override;
  void range(unsigned long i, float* min, float* max) const override;
  void rangeOut(unsigned long i, float* min, float* max) const override;
  CtrlValueType  ctrlValueType(unsigned long i)    const override;
  CtrlList::Mode ctrlMode(unsigned long i)         const override;
  CtrlValueType  ctrlOutValueType(unsigned long i) const override;
  CtrlList::Mode ctrlOutMode(unsigned long i)      const override;

private:
  // Sort the queued input events ascending by header.time is handled inside
  // ClapInstanceCore::runProcess() now — nothing to do here.
  bool processEvent(const MidiPlayEvent& e, uint32_t sampleOffset);
  void handlePluginOutputEvents(int plug_id);
  void flushParamChanges(unsigned long syncFrame,
                         unsigned long nframes,
                         int plug_id);

  // (Re)allocate _audioIn/OutBuffers for `frames` samples; frees any
  // existing allocation first via freeAudioBuffers(). Shared by init() and
  // getData()'s defensive regrow when the host's block size changes
  // mid-session (PipeWire's JACK bridge can do this via adaptive quantum).
  bool allocAudioBuffers(unsigned long frames);
  void freeAudioBuffers();

  ClapSynth*       _synth = nullptr;
  ClapInstanceCore _core;

  // Audio I/O scratch buffers (posix_memalign, 16-byte aligned). Owned here
  // (not by ClapInstanceCore) since silence/dummy-port substitution is a
  // caller concern — see ClapInstanceCore::runProcess()'s doc comment.
  float** _audioInBuffers    = nullptr; ///< [_core.inPorts()]
  float*  _audioInSilenceBuf = nullptr; ///< silence for unconnected inputs
  float** _audioOutBuffers   = nullptr; ///< [_core.outPorts()] scratch buffers
  unsigned long _audioBufFrames = 0;    ///< frame capacity currently allocated above

  // Parameter value arrays (mirrors DSSI Port[] pattern). This is MusE's own
  // automation shadow cache — distinct from ClapInstanceCore's event queue.
  Port* _controls    = nullptr; ///< [_synth->_controlInPorts]
  Port* _controlsOut = nullptr; ///< [_synth->_controlOutPorts]

  // Debug-print throttles (see clapDebugGateReady() in clap_host_lib.h) —
  // not correctness state.
  std::atomic<int64_t> _lastBufRegrowPrintUs { 0 }; ///< getData() buffer-regrow message
  std::atomic<int64_t> _lastMidiWarnPrintUs  { 0 }; ///< processEvent() zero-vel-note warning
};

//---------------------------------------------------------
//   initCLAP
//   Called at startup to register CLAP instrument-class plugins from the
//   scan list into MusEGlobal::synthis. Effect-class plugins are registered
//   separately by initCLAPEffects() (clap_host_effect.h) into
//   MusEGlobal::plugins — a plugin marked as both classes gets registered by
//   both, independently, same as VstNativeSynth/VstNativePluginWrapper.
//---------------------------------------------------------
void initCLAP();

} // namespace MusECore

#endif // CLAP_SUPPORT
