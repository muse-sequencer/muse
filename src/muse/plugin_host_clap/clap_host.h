//=============================================================================
//  MusE
//  Linux Music Editor
//
//  clap_host.h
//  CLAP (CLever Audio Plugin) host integration for MusE
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
#include <memory>
#include <cstdint>

#include <QLibrary>
#include <QHash>

#include <clap/clap.h>
#include <clap/ext/params.h>
#include <clap/ext/audio-ports.h>
#include <clap/ext/gui.h>
#include <clap/ext/state.h>
#include <clap/ext/timer-support.h>     // host drives plugin GUI repaints
#include <clap/ext/posix-fd-support.h>  // host feeds plugin X11/toolkit fd events

#include "synth.h"
#include "plugin.h"
#include "ctrl.h"

// Forward declarations
class QWidget;
class QTimer;          // GUI event source for clap.timer-support
class QSocketNotifier; // GUI event source for clap.posix-fd-support
namespace MusEGui { class PopupMenu; }

namespace MusECore {

class ClapSynthIF;

//---------------------------------------------------------
//   ClapSynth
//   Represents a loaded CLAP plugin descriptor (factory metadata).
//   One per discovered plugin id in the scan list.
//   Multiple instances share one ClapSynth via reference counting.
//   Note: _qlib, _references, _label, _uri etc. come from PluginBase.
//         _portCount, _inports etc. are NOT in PluginBase/Synth (they live
//         in Plugin which is a sibling branch), so we declare them here.
//---------------------------------------------------------

class ClapSynth : public Synth {
  friend class ClapSynthIF;

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

  // Parameter metadata, filled in ClapSynthIF::init() after instantiation
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
//   One running CLAP plugin instance inside MusE.
//   One per active SynthI track using a ClapSynth.
//---------------------------------------------------------

class ClapSynthIF : public SynthIF {
  // Q_OBJECT

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

  void read(Xml& xml);          // restore plugin state from song file
  void write(int level, Xml& xml) const override;

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

  // hasGui: true if plugin has GUI extension and we successfully created it,
  //         or it supports GUI creation (we check _extGui != nullptr after init)
  bool hasGui()       const override { return _extGui != nullptr; }
  bool hasNativeGui() const override { return _extGui != nullptr; }

  //--- GUI ---
  bool nativeGuiVisible() const override;
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

  //--- Host callback implementations (called from static C trampolines) ---
  const void* hostGetExtension(const char* ext_id);
  void hostRequestRestart();
  void hostRequestCallback();
  void hostParamsRescan(clap_param_rescan_flags flags);
  void hostParamsRequestFlush();
  void hostGuiClosed(bool was_destroyed);
  // Plugin asked the host to resize its embedding window. Returns true if the
  // host honoured it. Floating GUIs size themselves, so this is a no-op there.
  bool hostGuiRequestResize(uint32_t width, uint32_t height);

  // clap.timer-support: the plugin registers a periodic timer; the host calls
  // back into _extTimer->on_timer() to drive GUI repaints/animation.
  bool hostTimerRegister(uint32_t period_ms, clap_id* timer_id);
  bool hostTimerUnregister(clap_id timer_id);

  // clap.posix-fd-support: the plugin registers a file descriptor (typically the
  // X11 connection socket); the host watches it and calls _extPosixFd->on_fd()
  // when it becomes ready, so the embedded GUI receives expose/draw events.
  bool hostFdRegister(int fd, clap_posix_fd_flags_t flags);
  bool hostFdModify(int fd, clap_posix_fd_flags_t flags);
  bool hostFdUnregister(int fd);


  // clap - thread check - extension
  bool hostIsMainThread() const;
  bool hostIsAudioThread() const;

private:
  void buildHostVtable();
  // Tear down the plugin GUI (if created) and the host container window,
  // in the correct order: plugin gui->destroy() first, then delete the window.
  void destroyGui();
  // Stop and delete all QTimers/QSocketNotifiers driving the plugin GUI.
  // Called from destroyGui() so we never call on_timer()/on_fd() into a plugin
  // whose GUI is being torn down.
  void clearGuiEventSources();
  // Sort the queued input events ascending by header.time. CLAP requires the
  // in_events stream to be time-ordered; param flushes and notes are appended
  // in separate passes, so the buffer can otherwise be out of order.
  void sortInEvents();
  bool processEvent(const MidiPlayEvent& e, uint32_t sampleOffset);
  void handlePluginOutputEvents(int plug_id);
  void flushParamChanges(unsigned long syncFrame,
                         unsigned long nframes,
                         int plug_id);

  ClapSynth*           _synth  = nullptr;
  const clap_plugin_t* _plugin = nullptr;

  // Plugin extension pointers (owned by plugin, valid until destroy)
  const clap_plugin_params_t*      _extParams     = nullptr;
  const clap_plugin_audio_ports_t* _extAudioPorts = nullptr;
  const clap_plugin_gui_t*         _extGui        = nullptr;
  const clap_plugin_state_t*       _extState      = nullptr;
  // GUI event-source extensions, queried in init(). nullptr if the plugin does
  // not use them (then it drives its own loop and needs no host callbacks).
  const clap_plugin_timer_support_t*    _extTimer   = nullptr;
  const clap_plugin_posix_fd_support_t* _extPosixFd = nullptr;

  // Per-instance host vtable (host_data = this)
  clap_host_t _clapHost;

  // Audio I/O buffers (posix_memalign, 16-byte aligned)
  float** _audioInBuffers    = nullptr; ///< [_synth->_inports]
  float*  _audioInSilenceBuf = nullptr; ///< silence for unconnected inputs
  float** _audioOutBuffers   = nullptr; ///< [_synth->_outports] scratch buffers

  // Per-port channel layout as the plugin declares it (via CLAP_EXT_AUDIO_PORTS).
  // The flat _audioInBuffers/_audioOutBuffers above are sliced into these ports
  // when building the clap_process_t. Sizes are the plugin's audio port counts.
  std::vector<uint32_t> _inPortChans;  ///< channels of each input port
  std::vector<uint32_t> _outPortChans; ///< channels of each output port

  // Parameter value arrays (mirrors DSSI Port[] pattern)
  Port* _controls    = nullptr; ///< [_synth->_controlInPorts]
  Port* _controlsOut = nullptr; ///< [_synth->_controlOutPorts]

  // GUI state
  bool _isGuiCreated  = false;
  bool _isGuiVisible  = false;
  bool _isGuiFloating = false;

  // CLAP requires start_processing()/stop_processing() to run on the AUDIO
  // thread, while activate()/deactivate() run on the MAIN thread. So activate()
  // only flags a request; getData() (audio thread) performs the actual
  // start/stop. Diva (u-he) enforces this and aborts otherwise.
  std::atomic<bool> _clapProcessing        { false }; ///< plugin is in processing state
  std::atomic<bool> _startProcessingReq    { false }; ///< audio thread should start_processing
  std::atomic<bool> _stopProcessingReq     { false }; ///< audio thread should stop_processing
  
  // Host-owned native window the plugin draws into when embedded (X11/Win32/Cocoa).
  // Stays nullptr for floating GUIs, which own their own window.
  QWidget* _editorWindow = nullptr;


  // Alive-flag for queued hostRequestCallback() main-thread lambdas. Reset to
  // false in the destructor so any in-flight queued on_main_thread() call that
  // runs after this instance is gone becomes a safe no-op.
  std::shared_ptr<bool> _instanceAlive = std::make_shared<bool>(true);
  
  // GUI event sources, created on the main thread from the host callbacks above.
  // _timers: one QTimer per clap.timer-support registration.
  // _fd*:    one QSocketNotifier per (fd, direction) for clap.posix-fd-support.
  clap_id                      _nextTimerId = 0;
  QHash<clap_id, QTimer*>      _timers;
  QHash<clap_id, std::shared_ptr<bool>> _timerAlive; ///< alive-flags for in-flight timer lambdas
  QHash<int, QSocketNotifier*> _fdRead;   ///< CLAP_POSIX_FD_READ
  QHash<int, QSocketNotifier*> _fdWrite;  ///< CLAP_POSIX_FD_WRITE
  QHash<int, QSocketNotifier*> _fdError;  ///< CLAP_POSIX_FD_ERROR

  // CLAP event buffer (fixed-size pool; each slot is clap_event_midi_t-sized,
  // sufficient for all core event types we generate/receive)
  uint8_t* _evInBuf        = nullptr;
  uint8_t* _evOutBuf       = nullptr;
  uint32_t _evInCount      = 0;
  uint32_t _evOutCount     = 0;
  uint32_t _evBufCapacity  = 0;

  // Vtable wrappers for plugin to iterate our event buffers
  clap_input_events_t  _clapEvIn;
  clap_output_events_t _clapEvOut;
};

//---------------------------------------------------------
//   Shared host-vtable helpers
//   Defined/used by both clap_host.cpp (core) and
//   clap_host_gui.cpp (GUI). static inline => one copy per TU,
//   no duplicate-symbol issues.
//---------------------------------------------------------

static inline ClapSynthIF* hostFromClap(const clap_host_t* host)
{
  return static_cast<ClapSynthIF*>(host->host_data);
}

// The clap_host_gui vtable. Defined in clap_host_gui.cpp; core's
// hostGetExtension() returns it for CLAP_EXT_GUI.
const clap_host_gui_t* clapGuiHostExt();

//---------------------------------------------------------
//   initCLAP
//   Called at startup to register all CLAP plugins from the scan list.
//---------------------------------------------------------
void initCLAP();

} // namespace MusECore

#endif // CLAP_SUPPORT
