//=============================================================================
//  MusE
//  Linux Music Editor
//
//  clap_host_lib.h
//  Reusable CLAP plugin-instance core, shared by the synth-track host
//  (clap_host.cpp -> ClapSynthIF) and the effect-rack host
//  (clap_host_effect.cpp -> ClapPluginWrapper). Implementations split as:
//    clap_host_lib_core.cpp - instantiation, params, events, process(), state
//    clap_host_lib_gui.cpp  - native GUI embedding, timer-support, posix-fd
//
//  ClapInstanceCore knows nothing about SynthIF or PluginI/Plugin. It is one
//  running clap_plugin_t instance plus everything needed to drive it: extension
//  pointers, audio/event buffers, the host vtable, and GUI/timer/fd plumbing.
//  Callers (ClapSynthIF, ClapPluginWrapper_State) own the translation to/from
//  MusE's MidiPlayEvent stream, LADSPA-style connectPort() pointers, etc.
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
#include <cstdint>
#include <memory>
#include <atomic>
#include <functional>

#include <QString>
#include <QHash>

#include <clap/clap.h>
#include <clap/ext/params.h>
#include <clap/ext/audio-ports.h>
#include <clap/ext/gui.h>
#include <clap/ext/state.h>
#include <clap/ext/timer-support.h>
#include <clap/ext/posix-fd-support.h>

class QWidget;
class QTimer;
class QSocketNotifier;

namespace MusECore {

class ClapSynth; // Existing shared descriptor: factory/entry/desc + paramIds/paramInfo.
                  // Reused as-is by both the synth and effect wrappers (same role
                  // as VstNativeSynth is reused by VstNativePluginWrapper).

//---------------------------------------------------------
//   ClapInstanceCore
//---------------------------------------------------------

class ClapInstanceCore {
public:
  ClapInstanceCore();
  ~ClapInstanceCore();

  //--- Lifecycle ---

  // Instantiates the plugin from synth's factory/desc, queries extensions,
  // counts audio ports/params, allocates buffers. Returns false on failure
  // (prints the reason; caller must not use this instance further).
  bool init(ClapSynth* synth, const QString& displayName);

  // Tears down GUI, timers/fds, deactivates + destroys the plugin, frees
  // buffers. Safe to call once; safe to call from the destructor.
  void shutdown();

  bool isValid() const { return _plugin != nullptr; }
  ClapSynth* synth() const { return _synth; }
  const clap_plugin_t* plugin() const { return _plugin; }
  const QString& displayName() const { return _displayName; }

  //--- Port introspection (mirrors what init() filled into ClapSynth) ---

  unsigned long inPorts()  const;
  unsigned long outPorts() const;
  unsigned long paramCount() const;
  const std::vector<uint32_t>& inPortChans()  const { return _inPortChans; }
  const std::vector<uint32_t>& outPortChans() const { return _outPortChans; }

  //--- Activation ---

  // Both are no-ops if already in the requested (or already-requested) state.
  // CLAP requires plugin->activate()/deactivate() on the MAIN thread, but
  // plugin->start_processing()/stop_processing() on the AUDIO thread (Diva
  // and others enforce this and abort otherwise). So activate()/deactivate()
  // here only call the main-thread plugin->activate()/deactivate() and flag
  // a start/stop-processing request; runProcess() (audio thread) services
  // that flag before calling plugin->process().
  void activate();
  void deactivate();
  bool isActive() const { return _curActiveState; }

  //--- Input event queue ---
  // Caller fills these before calling runProcess(); the queue is cleared
  // automatically at the start of each runProcess() call.

  bool pushNoteEvent(int16_t type, int32_t noteId, int16_t port, int16_t channel,
                     int16_t key, double velocity, uint32_t sampleOffset);
  bool pushMidiEvent(const uint8_t data[3], uint16_t port, uint32_t sampleOffset);
  bool pushParamValueEvent(clap_id paramId, double value, uint32_t sampleOffset);

  //--- Processing ---

  // ins/outs are flat per-channel float* arrays, already ordered/sized to match
  // inPortChans()/outPortChans() (silence/dummy-buffer substitution for
  // disconnected or bypassed ports is the caller's job, same as today's
  // ClapSynthIF::getData()). Internally: services any pending start_processing()/
  // stop_processing() request first (audio-thread-only per CLAP spec), sorts
  // queued input events, slices ins/outs into clap_audio_buffer_t per declared
  // port, calls _plugin->process(), then clears the input queue.
  clap_process_status runProcess(int64_t steadyTime, uint32_t nframes,
                                 float** ins, uint32_t numInChans,
                                 float** outs, uint32_t numOutChans);

  // Output events produced by the last runProcess() call. Caller drains these
  // (e.g. to translate CLAP_EVENT_NOTE_END back into MidiPlayEvents, or to
  // notice CLAP_EVENT_PARAM_VALUE echoes) immediately after runProcess().
  uint32_t outputEventCount() const { return _evOutCount; }
  const clap_event_header_t* outputEvent(uint32_t idx) const;

  //--- Parameters ---

  double getParameter(clap_id paramId) const;
  // Queues an immediate (sampleOffset 0) param-value event for the next
  // runProcess() call. Does not call the plugin directly.
  void   setParameter(clap_id paramId, double value);

  //--- State persistence (clap.state extension) ---

  std::vector<QString> getCustomData() const;
  bool setCustomData(const std::vector<QString>&);

  //--- GUI (clap_host_lib_gui.cpp) ---

  bool hasGui()          const { return _extGui != nullptr; }
  bool nativeGuiVisible() const { return _isGuiVisible; }
  // v == true: create (if needed) + show. v == false: hide only, keeping the
  // GUI created (its render surface, GL context and event sources stay alive).
  // Destroying on every hide crashes GL plugins (Cardinal: glXMakeCurrent with a
  // dead drawable) and blackens others on re-show, so full teardown is deferred
  // to closeNativeGui()/destroyGui() at actual close/shutdown.
  // NOTE: ClapInstanceCore doesn't know about PluginIBase/Plugin, so callers
  //       are responsible for their own PluginIBase::showNativeGui(v)-style
  //       bookkeeping before/after calling this.
  void showNativeGui(bool v);
  void closeNativeGui();
  bool hostGuiRequestResize(uint32_t width, uint32_t height);
  void hostGuiClosed(bool wasDestroyed);

  // Invoked from hostGuiClosed() when the plugin/window-manager closed the
  // GUI without destroying it (was_destroyed == false) — the CLAP-side
  // equivalent of the owner's own "native gui pending" flag going false.
  // ClapSynthIF wires this to SynthIF::showNativeGuiPending(false); a future
  // ClapPluginWrapper_State wires it to PluginIBase's equivalent.
  using GuiClosedCallback = std::function<void()>;
  void setGuiClosedCallback(GuiClosedCallback cb) { _onGuiHiddenByPlugin = std::move(cb); }

  //--- Host callback implementations (clap_host_lib_core.cpp) ---

  const void* hostGetExtension(const char* extId);
  void hostRequestRestart();
  void hostRequestCallback();
  void hostParamsRescan(clap_param_rescan_flags flags);
  void hostParamsRequestFlush();
  bool hostIsMainThread() const;
  bool hostIsAudioThread() const;

  //--- Host callback implementations for GUI event sources (clap_host_lib_gui.cpp) ---

  bool hostTimerRegister(uint32_t periodMs, clap_id* timerId);
  bool hostTimerUnregister(clap_id timerId);
  bool hostFdRegister(int fd, clap_posix_fd_flags_t flags);
  bool hostFdModify(int fd, clap_posix_fd_flags_t flags);
  bool hostFdUnregister(int fd);

  //--- Pre-audio-exit shutdown ---
  // MusE tears down the audio engine (exitJackAudio() etc.) BEFORE deleting
  // tracks/synths, which is also before ~ClapSynthIF()/cleanup() run. That
  // leaves no live audio thread for shutdown() to safely call
  // stop_processing() on, and CLAP plugins (Diva included) reject
  // destroy() on a still-active instance even if we skip stop_processing()
  // to dodge the thread-check abort — so skipping isn't a workable fallback.
  // Call this ONCE, from the app's closeEvent()-equivalent, BEFORE the audio
  // engine is torn down: it deactivates every live ClapInstanceCore and
  // blocks (pumping the Qt event loop so the audio-thread-driven deferred
  // plugin->deactivate() marshal in runProcess() can actually execute)
  // until each one confirms it's fully deactivated, or a timeout elapses.
  static void deactivateAllBeforeAudioShutdown(int perInstanceTimeoutMs = 3000);

  // Calls stop_processing() on every live instance that is processing. MUST run
  // on the audio thread (Diva/u-he enforce this). Invoked from
  // Audio::processMsg (AUDIO_CLAP_STOP_PROCESSING) via
  // deactivateAllBeforeAudioShutdown().
  static void stopAllProcessingOnAudioThread();

private:
  void buildHostVtable();
  bool appendEvent(const void* evSlotSrc, uint32_t size, uint32_t sampleOffset);
  void sortInEvents();
  // Tear down the plugin GUI (if created) and the host container window.
  void destroyGui();
  // Stop/delete all QTimers/QSocketNotifiers driving the plugin GUI. Called
  // from destroyGui()/shutdown() so we never call on_timer()/on_fd() into a
  // plugin whose GUI is being torn down.
  void clearGuiEventSources();

  ClapSynth*            _synth  = nullptr;
  const clap_plugin_t*  _plugin = nullptr;
  QString               _displayName;

  const clap_plugin_params_t*      _extParams     = nullptr;
  const clap_plugin_audio_ports_t* _extAudioPorts = nullptr;
  const clap_plugin_gui_t*         _extGui        = nullptr;
  const clap_plugin_state_t*       _extState      = nullptr;
  const clap_plugin_timer_support_t*    _extTimer   = nullptr;
  const clap_plugin_posix_fd_support_t* _extPosixFd = nullptr;

  clap_host_t _clapHost;
  std::atomic<bool> _curActiveState { false };
  
  // CLAP requires start_processing()/stop_processing() to run on the AUDIO
  // thread, while activate()/deactivate() run on the MAIN thread. So
  // activate()/deactivate() only flag a request; runProcess() (audio thread)
  // performs the actual start/stop. Diva (u-he) enforces this and aborts
  // otherwise.
  std::atomic<bool> _clapProcessing     { false }; ///< plugin is in processing state
  std::atomic<bool> _startProcessingReq { false }; ///< audio thread should start_processing
  std::atomic<bool> _stopProcessingReq  { false }; ///< audio thread should stop_processing

  // Set by hostRequestRestart() (plugin asked to be deactivated+reactivated,
  // e.g. after a latency/port change). Serviced in runProcess(): stop on the
  // audio thread, then deactivate()+activate() marshaled to the main thread,
  // then start_processing() again. Ignoring it makes some plugins go silent.
  std::atomic<bool> _restartReq { false };

  // Coalesces repeated request_restart() calls the same way
  // _activateRequestPending coalesces activate(): without this, a plugin
  // calling request_restart() several times in a row (e.g. right after
  // gui->create()) queues a fresh deactivate()+activate() cycle on every
  // audio block until it stops asking — the resulting stop/restart churn
  // is audible as noise right after the GUI opens, then silence once the
  // plugin ends up mid-cycle.
  std::atomic<bool> _restartInFlight { false };

  // Guards against flooding the Qt event queue: set while a marshaled
  // plugin->activate() call (queued from activate() when called off the main
  // thread) is in flight, so repeated activate() calls before it lands don't
  // queue duplicate lambdas.
  std::atomic<bool> _activateRequestPending { false };

  // Tracks the plugin's ACTUAL activation state: true after a successful
  // plugin->activate(), false after plugin->deactivate() has really run.
  // Distinct from _curActiveState, which is the LOGICAL "should be active"
  // state and flips to false the instant deactivate() is requested — while
  // the real plugin->deactivate() is still deferred/marshaled. Shutdown-time
  // waiting keys off this (not _curActiveState) so we never destroy() a plugin
  // whose deactivate() hasn't landed yet.
  std::atomic<bool> _pluginActivated { false };

  // Set once shutdown teardown begins (deactivateAllBeforeAudioShutdown). While
  // we pump the Qt event loop waiting for deactivation, a still-queued marshaled
  // activate() lambda could otherwise run and re-activate a plugin we're tearing
  // down — Diva aborts with "Plugin was activated twice". Every activate path
  // checks this and becomes a no-op once it's set.
  std::atomic<bool> _teardown { false };

  // Debug/verification only: catches real threading violations. Set at
  // entry / cleared at exit of runProcess(); if it's already true on entry,
  // two threads are inside process() for this instance at once — should
  // never happen and points straight at a thread-order bug rather than a
  // buffer or CPU-load issue.
  std::atomic<bool> _inRunProcess { false };

  // Rolling worst-case process() time, for the periodic budget-overrun log
  // in runProcess() — lets you see whether a plugin's process() calls are
  // trending slower over a session rather than just spiking once.
  std::atomic<double> _maxProcessMs { 0.0 };


  std::vector<uint32_t> _inPortChans;
  std::vector<uint32_t> _outPortChans;

  // Full audio-port layout as the plugin declares it (ALL ports, declared
  // order), used only to build the clap_process_t buffers. MusE itself only
  // connects the MAIN port (_inPortChans/_outPortChans, _synth->_inports/
  // _outports); the other ports (e.g. a vocoder's modulator/sidechain input)
  // must still be handed to plugin->process() with valid buffers or the plugin
  // reads past audio_inputs[]/audio_outputs[] and crashes. Non-main ports get
  // silence (in) / a throwaway buffer (out). _mainInPortIdx/_mainOutPortIdx are
  // the declared-order index of the MusE-connected main port (-1 = none).
  std::vector<uint32_t> _procInPortChans;
  std::vector<uint32_t> _procOutPortChans;
  int _mainInPortIdx  = -1;
  int _mainOutPortIdx = -1;
  std::vector<float>  _silenceBuf;      // zeroed, shared by all silent input chans
  std::vector<float>  _dummyOutBuf;     // throwaway, shared by all discarded output chans
  std::vector<float*> _inChanScratch;   // per-process flat channel-pointer array (in)
  std::vector<float*> _outChanScratch;  // per-process flat channel-pointer array (out)

  // Alive-flag for queued hostRequestCallback() main-thread lambdas and
  // in-flight timer/fd lambdas. Reset to false in shutdown() so anything
  // still queued after teardown becomes a safe no-op.
  std::shared_ptr<bool> _instanceAlive = std::make_shared<bool>(true);

  // GUI state (clap_host_lib_gui.cpp)
  bool _isGuiCreated  = false;
  bool _isGuiVisible  = false;
  bool _isGuiFloating = false;
  QWidget* _editorWindow = nullptr;
  GuiClosedCallback _onGuiHiddenByPlugin; ///< see setGuiClosedCallback()

  clap_id                                _nextTimerId = 0;
  QHash<clap_id, QTimer*>                _timers;
  QHash<clap_id, std::shared_ptr<bool>>  _timerAlive;
  QHash<int, QSocketNotifier*> _fdRead;
  QHash<int, QSocketNotifier*> _fdWrite;
  QHash<int, QSocketNotifier*> _fdError;

  // Fixed-size event-slot pool. Slot stride = sizeof(largest clap_event_*),
  // see kClapEventSlotSize in the .cpp - large enough for note/midi/param
  // events, which is everything either wrapper currently generates.
  uint8_t* _evInBuf  = nullptr;
  uint8_t* _evOutBuf = nullptr;
  uint32_t _evInCount     = 0;
  uint32_t _evOutCount    = 0;
  uint32_t _evBufCapacity = 0;

  clap_input_events_t  _clapEvIn;
  clap_output_events_t _clapEvOut;

  // Registry of all live instances, used only by deactivateAllBeforeAudioShutdown().
  // init()/shutdown() always run on the main thread (instantiate()/createSIF()
  // are UI-triggered), same thread deactivateAllBeforeAudioShutdown() runs on
  // — no locking needed.
  static std::vector<ClapInstanceCore*> s_liveInstances;
};

// Shared host-vtable helper. Used by clap_host_lib_core.cpp and
// clap_host_lib_gui.cpp trampolines to recover the owning core instance from
// the clap_host_t* CLAP hands back into host callbacks.
static inline ClapInstanceCore* coreFromClap(const clap_host_t* host)
{
  return static_cast<ClapInstanceCore*>(host->host_data);
}

// clap_host_lib_gui.cpp defines these three extension tables (GUI embedding,
// timer-support, posix-fd-support). clap_host_lib_core.cpp's
// ClapInstanceCore::hostGetExtension() returns them for the matching
// CLAP_EXT_* ids, keeping all GUI/timer/fd vtable state in the gui TU.
const clap_host_gui_t*                clapCoreGuiHostExt();
const clap_host_timer_support_t*      clapCoreTimerHostExt();
const clap_host_posix_fd_support_t*   clapCorePosixFdHostExt();

//---------------------------------------------------------
//   clapDeactivateAllBeforeAudioShutdown
//   Free-function wrapper for ClapInstanceCore::deactivateAllBeforeAudioShutdown(),
//   for callers outside this module (e.g. MusE::closeEvent()) that don't need
//   the rest of ClapInstanceCore's interface. MUST be called before the audio
//   engine (exitJackAudio() etc.) is torn down — see the method's doc comment.
//---------------------------------------------------------
void clapDeactivateAllBeforeAudioShutdown();

} // namespace MusECore

#endif // CLAP_SUPPORT
