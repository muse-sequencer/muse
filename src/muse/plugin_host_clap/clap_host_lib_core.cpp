//=============================================================================
//  MusE
//  Linux Music Editor
//
//  clap_host_lib_core.cpp
//  Reusable CLAP plugin-instance core (non-GUI half). See clap_host_lib.h for
//  the class contract and the GUI/timer/fd half in clap_host_lib_gui.cpp.
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

#include <stdio.h>
#include <stdint.h>
#include <cstring>
#include <algorithm>

#include <QCoreApplication>
#include <QMetaObject>
#include <QThread>
#include <QEventLoop>

#include <clap/clap.h>
#include <clap/ext/log.h>
#include <clap/ext/thread-check.h>

#include "clap_host_lib.h"
#include "clap_host_synth.h"    // ClapSynth : public Synth (factory/desc/paramIds/paramInfo)
#include "audio.h"        // MusEGlobal::audio->isAudioThread(), msgClapStopProcessing()
#include "globals.h"
#include "gconfig.h"      // MusEGlobal::config.useDenormalBias

namespace MusECore {

//---------------------------------------------------------
//   Event buffer slot
//   The host queues several CLAP event kinds into one buffer (notes, midi,
//   param values). They differ in size — clap_event_note_t and
//   clap_event_param_value_t are larger than clap_event_midi_t — so the slot
//   stride MUST be the size of the largest, or events overrun each other and
//   the plugin reads a corrupted stream (symptom: no/garbled audio).
//---------------------------------------------------------
union ClapEventSlot {
  clap_event_header_t      hdr;
  clap_event_note_t        note;
  clap_event_midi_t        midi;
  clap_event_param_value_t param;
};
static constexpr uint32_t kClapEventSlotSize = sizeof(ClapEventSlot);

//---------------------------------------------------------
//   clap - Thread Check - extension
//---------------------------------------------------------

static bool CLAP_ABI clapHostIsMainThread(const clap_host_t* host)
{ return coreFromClap(host)->hostIsMainThread(); }

static bool CLAP_ABI clapHostIsAudioThread(const clap_host_t* host)
{ return coreFromClap(host)->hostIsAudioThread(); }

static const clap_host_thread_check_t s_hostThreadCheckExt = {
  clapHostIsMainThread,
  clapHostIsAudioThread,
};

//---------------------------------------------------------
//   Static host vtable trampolines
//---------------------------------------------------------

static const void* CLAP_ABI clapHostGetExtension(const clap_host_t* host, const char* ext_id)
{ return coreFromClap(host)->hostGetExtension(ext_id); }

static void CLAP_ABI clapHostRequestRestart(const clap_host_t* host)
{ coreFromClap(host)->hostRequestRestart(); }

static void CLAP_ABI clapHostRequestProcess(const clap_host_t* /*host*/) {}

static void CLAP_ABI clapHostRequestCallback(const clap_host_t* host)
{ coreFromClap(host)->hostRequestCallback(); }

// clap_host_params
static void CLAP_ABI clapHostParamsRescan(const clap_host_t* host, clap_param_rescan_flags flags)
{ coreFromClap(host)->hostParamsRescan(flags); }

static void CLAP_ABI clapHostParamsClear(const clap_host_t* /*host*/,
                                         clap_id /*param_id*/,
                                         clap_param_clear_flags /*flags*/) {}

static void CLAP_ABI clapHostParamsRequestFlush(const clap_host_t* host)
{ coreFromClap(host)->hostParamsRequestFlush(); }

static const clap_host_params_t s_hostParamsExt = {
  clapHostParamsRescan,
  clapHostParamsClear,
  clapHostParamsRequestFlush,
};

// clap_host_log
static void CLAP_ABI clapHostLogLog(const clap_host_t* host,
                                    clap_log_severity severity, const char* msg)
{
  switch(severity)
  {
    case CLAP_LOG_DEBUG:
      #ifdef CLAP_DEBUG
      fprintf(stderr, "CLAP [debug]: %s\n", msg);
      #endif
      break;
    default:
      // CLAP discourages plugins from logging off the main thread, but
      // nothing enforces that — throttle so a chatty/misbehaving plugin
      // logging every block can't turn this into RT-thread fprintf spam.
      if(coreFromClap(host)->logPrintGateReady())
        fprintf(stderr, "CLAP [log]: %s\n", msg);
      break;
  }
}

static const clap_host_log_t s_hostLogExt = { clapHostLogLog };

//---------------------------------------------------------
//   ClapInstanceCore
//---------------------------------------------------------

ClapInstanceCore::ClapInstanceCore()
{
  #ifdef CLAP_DEBUG
  printf("ClapInstanceCore::ClapInstanceCore\n");
  #endif
}

ClapInstanceCore::~ClapInstanceCore()
{
  #ifdef CLAP_DEBUG
  printf("ClapInstanceCore::~ClapInstanceCore\n");
  #endif
  shutdown();
}

//---------------------------------------------------------
//   buildHostVtable
//---------------------------------------------------------

void ClapInstanceCore::buildHostVtable()
{
  _clapHost.clap_version    = CLAP_VERSION;
  _clapHost.host_data       = this;
  _clapHost.name            = "MusE";
  _clapHost.vendor          = "MusE Team";
  _clapHost.url             = "https://muse-sequencer.org";
  _clapHost.version         =  VERSION;
  _clapHost.get_extension   = clapHostGetExtension;
  _clapHost.request_restart  = clapHostRequestRestart;
  _clapHost.request_process  = clapHostRequestProcess;
  _clapHost.request_callback = clapHostRequestCallback;
}

//---------------------------------------------------------
//   init
//---------------------------------------------------------

bool ClapInstanceCore::init(ClapSynth* s, const QString& displayName)
{
  #ifdef CLAP_DEBUG
  printf("ClapInstanceCore::init\n");
  #endif

  if(!s)
  {
    fprintf(stderr, "ClapInstanceCore::init: synth is nullptr\n");
    return false;
  }

  _synth = s;
  _displayName = displayName;
  buildHostVtable();

  _plugin = _synth->_factory->create_plugin(
    _synth->_factory, &_clapHost, _synth->_desc->id);

  if(!_plugin)
  {
    fprintf(stderr, "ClapInstanceCore::init: create_plugin() returned nullptr for '%s'\n",
            _synth->_desc->id);
    return false;
  }

  if(!_plugin->init(_plugin))
  {
    fprintf(stderr, "ClapInstanceCore::init: plugin->init() failed for '%s'\n",
            _synth->_desc->id);
    _plugin->destroy(_plugin);
    _plugin = nullptr;
    return false;
  }

  // Query extensions
  _extParams     = static_cast<const clap_plugin_params_t*>(
                     _plugin->get_extension(_plugin, CLAP_EXT_PARAMS));
  _extAudioPorts = static_cast<const clap_plugin_audio_ports_t*>(
                     _plugin->get_extension(_plugin, CLAP_EXT_AUDIO_PORTS));
  _extGui        = static_cast<const clap_plugin_gui_t*>(
                     _plugin->get_extension(_plugin, CLAP_EXT_GUI));
  _extState      = static_cast<const clap_plugin_state_t*>(
                     _plugin->get_extension(_plugin, CLAP_EXT_STATE));
  _extTimer      = static_cast<const clap_plugin_timer_support_t*>(
                     _plugin->get_extension(_plugin, CLAP_EXT_TIMER_SUPPORT));
  _extPosixFd    = static_cast<const clap_plugin_posix_fd_support_t*>(
                     _plugin->get_extension(_plugin, CLAP_EXT_POSIX_FD_SUPPORT));

  // --- Count audio channels across all ports ---
  // Only the MAIN port in each direction is exposed here. Non-main ports
  // (sidechain/aux inputs, extra outputs) have no routing concept in MusE's
  // Pipeline/PluginI — they'd show up as extra fake LADSPA ports that never
  // get connectPort()'d, handing the plugin a null data32 array for that
  // port. Some plugins (e.g. ZamCompX2's sidechain input) read from it
  // unconditionally and crash. Diva-style single-port plugins are unaffected
  // since their one port already is the main one.
  unsigned long inports  = 0;
  unsigned long outports = 0;
  _inPortChans.clear();
  _outPortChans.clear();
  _procInPortChans.clear();
  _procOutPortChans.clear();
  _mainInPortIdx = _mainOutPortIdx = -1;
  if(_extAudioPorts)
  {
    const uint32_t inCount  = _extAudioPorts->count(_plugin, true);
    const uint32_t outCount = _extAudioPorts->count(_plugin, false);

    // Full declared layout (ALL ports) for plugin->process(); note which one is
    // the MAIN port that MusE actually connects audio to.
    for(uint32_t p = 0; p < inCount; ++p)
    {
      clap_audio_port_info_t pi{};
      if(_extAudioPorts->get(_plugin, p, true, &pi))
      {
        if((pi.flags & CLAP_AUDIO_PORT_IS_MAIN) && _mainInPortIdx < 0)
          _mainInPortIdx = static_cast<int>(_procInPortChans.size());
        _procInPortChans.push_back(pi.channel_count);
      }
    }
    for(uint32_t p = 0; p < outCount; ++p)
    {
      clap_audio_port_info_t pi{};
      if(_extAudioPorts->get(_plugin, p, false, &pi))
      {
        if((pi.flags & CLAP_AUDIO_PORT_IS_MAIN) && _mainOutPortIdx < 0)
          _mainOutPortIdx = static_cast<int>(_procOutPortChans.size());
        _procOutPortChans.push_back(pi.channel_count);
      }
    }
    // Defensive: if no port flagged main (spec violation, seen in the wild),
    // treat port 0 as main.
    if(_mainInPortIdx  < 0 && !_procInPortChans.empty())  _mainInPortIdx  = 0;
    if(_mainOutPortIdx < 0 && !_procOutPortChans.empty()) _mainOutPortIdx = 0;

    // MusE-visible (connected) channels = the MAIN port only.
    if(_mainInPortIdx >= 0)
    {
      inports += _procInPortChans[_mainInPortIdx];
      _inPortChans.push_back(_procInPortChans[_mainInPortIdx]);
    }
    if(_mainOutPortIdx >= 0)
    {
      outports += _procOutPortChans[_mainOutPortIdx];
      _outPortChans.push_back(_procOutPortChans[_mainOutPortIdx]);
    }
  }
  else
  {
    // Safe fallback: assume a single stereo in and out port.
    inports  = 2;
    outports = 2;
    _inPortChans.push_back(2);
    _outPortChans.push_back(2);
    _procInPortChans.push_back(2);
    _procOutPortChans.push_back(2);
    _mainInPortIdx = _mainOutPortIdx = 0;
  }

  _synth->_inports  = inports;
  _synth->_outports = outports;

  // Audio-port layout the plugin declared vs. what MusE connects. If a plugin
  // goes silent, check here: the MAIN out port is where MusE's audio is written;
  // every other declared port gets silence (in) / a throwaway buffer (out), so a
  // wrong _mainOutPortIdx means real output lands in the throwaway = silence.
  fprintf(stderr,
    "ClapInstanceCore::init '%s': in ports=%zu (main idx=%d, %lu ch), "
    "out ports=%zu (main idx=%d, %lu ch)\n",
    _displayName.toLocal8Bit().constData(),
    _procInPortChans.size(),  _mainInPortIdx,  inports,
    _procOutPortChans.size(), _mainOutPortIdx, outports);

  // --- Enumerate parameters ---
  unsigned long controlPorts = 0;
  if(_extParams)
  {
    const uint32_t paramCount = _extParams->count(_plugin);
    controlPorts = paramCount;
    _synth->paramIds.resize(paramCount);
    _synth->paramInfo.resize(paramCount);
    _synth->paramIdToIndex.clear();

    for(uint32_t i = 0; i < paramCount; ++i)
    {
      clap_param_info_t info{};
      if(!_extParams->get_info(_plugin, i, &info))
      {
        fprintf(stderr, "ClapInstanceCore::init: get_info failed for param index %u\n", i);
        continue;
      }
      _synth->paramIds[i]  = info.id;
      _synth->paramInfo[i] = info;
      _synth->paramIdToIndex[info.id] = i;
    }
  }

  _synth->_controlInPorts  = controlPorts;
  _synth->_controlOutPorts = 0; // CLAP has no explicit output-only params
  _synth->_portCount       = inports + outports + controlPorts;

  // --- Allocate event buffers ---
  _evBufCapacity = 256;
  _evInBuf  = new uint8_t[_evBufCapacity * kClapEventSlotSize];
  _evOutBuf = new uint8_t[_evBufCapacity * kClapEventSlotSize];
  _evInCount  = 0;
  _evOutCount = 0;

  // Build input events vtable
  _clapEvIn.ctx  = this;
  _clapEvIn.size = [](const clap_input_events_t* list) -> uint32_t {
    return static_cast<ClapInstanceCore*>(list->ctx)->_evInCount;
  };
  _clapEvIn.get = [](const clap_input_events_t* list, uint32_t index) -> const clap_event_header_t* {
    ClapInstanceCore* self = static_cast<ClapInstanceCore*>(list->ctx);
    if(index >= self->_evInCount) return nullptr;
    return reinterpret_cast<const clap_event_header_t*>(
      self->_evInBuf + index * kClapEventSlotSize);
  };

  // Build output events vtable
  _clapEvOut.ctx = this;
  _clapEvOut.try_push = [](const clap_output_events_t* list,
                           const clap_event_header_t* event) -> bool {
    ClapInstanceCore* self = static_cast<ClapInstanceCore*>(list->ctx);
    if(self->_evOutCount >= self->_evBufCapacity) return false;
    memcpy(self->_evOutBuf + self->_evOutCount * kClapEventSlotSize,
           event, event->size);
    ++self->_evOutCount;
    return true;
  };

  s_liveInstances.push_back(this);
  return true;
}

//---------------------------------------------------------
//   shutdown
//---------------------------------------------------------

void ClapInstanceCore::shutdown()
{
  const auto it = std::find(s_liveInstances.begin(), s_liveInstances.end(), this);
  if(it != s_liveInstances.end())
    s_liveInstances.erase(it);

  // Disarm any queued on_main_thread()/deferred-deactivate lambdas before we
  // tear anything down.
  *_instanceAlive = false;
  _instanceAlive = std::make_shared<bool>(false);

  clearGuiEventSources();
  destroyGui();

  if(_plugin)
  {
    // shutdown() runs on the MAIN thread (the ~ClapSynthIF /
    // ClapPluginWrapper_State destructor chain). Diva/u-he enforce that
    // plugin->stop_processing() runs ONLY on MusE's real audio thread and abort
    // otherwise — and by the time we get here that thread is gone (app quit) or
    // no longer processing this instance (track removed). So we must NOT call
    // stop_processing() here. The stop is expected to have already happened via
    // the live audio thread:
    //   - App quit: MusE::closeEvent() calls clapDeactivateAllBeforeAudioShutdown()
    //     BEFORE seqStop(), while the audio thread still ticks, so it drives
    //     stop_processing() on the correct thread and deactivate() on the main
    //     thread. Both _clapProcessing and _pluginActivated are false here.
    //   - Mid-session removal: SynthI::deactivate3() calls _core.deactivate()
    //     before delete _sif -> shutdown().
    //
    // ASSUMPTION: the instance was deactivated (and its stop_processing() driven
    // through the audio thread) before this destructor runs — i.e. via
    // deactivateAllBeforeAudioShutdown() at quit, or MusE's detach-first track
    // removal mid-session. If a future path destroys a still-processing CLAP
    // instance without that, we can't stop it safely here (wrong thread) and
    // destroy() may abort; such a path must stop+deactivate via the audio thread
    // first.
    if(!_clapProcessing && _pluginActivated.exchange(false))
    {
      // Processing already stopped; deactivate() is [main-thread] — finish it.
      _plugin->deactivate(_plugin);
    }
    else if(_clapProcessing)
    {
      fprintf(stderr,
        "ClapInstanceCore::shutdown: '%s' still processing at destroy time — "
        "cannot stop_processing() off the audio thread; destroying anyway\n",
        _displayName.toLocal8Bit().constData());
    }

    _clapProcessing     = false;
    _curActiveState     = false;
    _startProcessingReq = false;
    _stopProcessingReq  = false;
    _restartInFlight    = false;

    _plugin->destroy(_plugin);
    _plugin = nullptr;
  }

  delete[] _evInBuf;  _evInBuf  = nullptr;
  delete[] _evOutBuf; _evOutBuf = nullptr;
  _evInCount = _evOutCount = _evBufCapacity = 0;
}

//---------------------------------------------------------
//   Port / param introspection
//---------------------------------------------------------

unsigned long ClapInstanceCore::inPorts()  const { return _synth ? _synth->_inports  : 0; }
unsigned long ClapInstanceCore::outPorts() const { return _synth ? _synth->_outports : 0; }
unsigned long ClapInstanceCore::paramCount() const { return _synth ? _synth->_controlInPorts : 0; }

//---------------------------------------------------------
//   activate / deactivate
//   NOTE: plugin->activate()/deactivate() must run on the MAIN thread;
//   plugin->start_processing()/stop_processing() must run on the AUDIO
//   thread. See the _clapProcessing/_start.../_stopProcessingReq comment in
//   clap_host_lib.h. deactivate() therefore does NOT call plugin->deactivate()
//   itself — it only requests stop_processing(); runProcess() performs the
//   stop on the audio thread and marshals the actual plugin->deactivate()
//   call back onto the main thread once processing has stopped.
//---------------------------------------------------------

void ClapInstanceCore::activate()
{
  if(_teardown) return;               // shutting down — never (re)activate
  if(_curActiveState) return;
  if(!_plugin)
  {
    fprintf(stderr, "ClapInstanceCore::activate: _plugin is nullptr (%s)\n",
            _displayName.toLocal8Bit().constData());
    return;
  }

  if(hostIsMainThread())
  {
    if(!_plugin->activate(_plugin,
                          static_cast<double>(MusEGlobal::sampleRate),
                          1, MusEGlobal::segmentSize))
    {
      fprintf(stderr, "ClapInstanceCore::activate: plugin->activate() failed (%s)\n",
              _displayName.toLocal8Bit().constData());
      return;
    }
    _curActiveState     = true;
    _pluginActivated    = true;
    _stopProcessingReq  = false;
    _startProcessingReq = true;
    return;
  }

  // Called from a non-main thread — e.g. PluginI::apply()'s wantActive
  // bypass/enable toggling calls Plugin::activate() directly from the audio
  // thread, but CLAP requires plugin->activate() on the MAIN thread only
  // (Diva aborts otherwise, same as the start/stop_processing() rule).
  // Marshal it there asynchronously; _curActiveState stays false until the
  // marshaled call confirms, so the caller correctly sees "not yet active"
  // for a block or two rather than us violating the thread rule.
  if(_activateRequestPending.exchange(true))
    return; // one already queued, don't flood the event loop every block

  const clap_plugin_t* plug = _plugin;
  auto alive = _instanceAlive;
  ClapInstanceCore* self = this;
  const double sr  = static_cast<double>(MusEGlobal::sampleRate);
  const uint32_t seg = MusEGlobal::segmentSize;
  QMetaObject::invokeMethod(qApp,
    [plug, alive, self, sr, seg]()
    {
      // Skip if torn down (shutdown) or the plugin is already actually active;
      // either would otherwise cause Diva's "activated twice" abort.
      if(*alive && plug && !self->_teardown && !self->_curActiveState && !self->_pluginActivated)
      {
        if(plug->activate(plug, sr, 1, seg))
        {
          self->_curActiveState     = true;
          self->_pluginActivated    = true;
          self->_stopProcessingReq  = false;
          self->_startProcessingReq = true;
        }
        else
          fprintf(stderr, "ClapInstanceCore::activate (marshaled): plugin->activate() failed (%s)\n",
                  self->_displayName.toLocal8Bit().constData());
      }
      if(*alive)
        self->_activateRequestPending = false;
    },
    Qt::QueuedConnection);
}

void ClapInstanceCore::deactivate()
{
  if(!_curActiveState) return;
  if(!_plugin)
  {
    fprintf(stderr, "ClapInstanceCore::deactivate: _plugin is nullptr (%s)\n",
            _displayName.toLocal8Bit().constData());
    return;
  }
  _curActiveState     = false;
  _startProcessingReq = false;
  _stopProcessingReq  = true;
}

//---------------------------------------------------------
//   Input event queue
//---------------------------------------------------------

bool ClapInstanceCore::appendEvent(const void* evSlotSrc, uint32_t size, uint32_t /*sampleOffset*/)
{
  if(_evInCount >= _evBufCapacity)
  {
    fprintf(stderr, "ClapInstanceCore::appendEvent: event buffer overflow (%s)\n",
            _displayName.toLocal8Bit().constData());
    return false;
  }
  memcpy(_evInBuf + _evInCount * kClapEventSlotSize, evSlotSrc, size);
  ++_evInCount;
  return true;
}

bool ClapInstanceCore::pushNoteEvent(int16_t type, int32_t noteId, int16_t port, int16_t channel,
                                     int16_t key, double velocity, uint32_t sampleOffset)
{
  clap_event_note_t ev{};
  ev.header.size     = sizeof(ev);
  ev.header.time     = sampleOffset;
  ev.header.space_id = CLAP_CORE_EVENT_SPACE_ID;
  ev.header.type     = static_cast<uint16_t>(type);
  ev.header.flags    = 0;
  ev.note_id         = noteId;
  ev.port_index      = port;
  ev.channel         = channel;
  ev.key             = key;
  ev.velocity        = velocity;
  return appendEvent(&ev, sizeof(ev), sampleOffset);
}

bool ClapInstanceCore::pushMidiEvent(const uint8_t data[3], uint16_t port, uint32_t sampleOffset)
{
  clap_event_midi_t ev{};
  ev.header.size     = sizeof(ev);
  ev.header.time     = sampleOffset;
  ev.header.space_id = CLAP_CORE_EVENT_SPACE_ID;
  ev.header.type     = CLAP_EVENT_MIDI;
  ev.header.flags    = 0;
  ev.port_index      = port;
  ev.data[0] = data[0];
  ev.data[1] = data[1];
  ev.data[2] = data[2];
  return appendEvent(&ev, sizeof(ev), sampleOffset);
}

bool ClapInstanceCore::pushParamValueEvent(clap_id paramId, double value, uint32_t sampleOffset)
{
  clap_event_param_value_t ev{};
  ev.header.size     = sizeof(ev);
  ev.header.time     = sampleOffset;
  ev.header.space_id = CLAP_CORE_EVENT_SPACE_ID;
  ev.header.type     = CLAP_EVENT_PARAM_VALUE;
  ev.header.flags    = 0;
  ev.param_id        = paramId;
  ev.cookie          = nullptr;
  ev.note_id         = -1;
  ev.port_index      = -1;
  ev.channel         = -1;
  ev.key             = -1;
  ev.value           = value;

  if(_synth)
  {
    const auto it = _synth->paramIdToIndex.find(paramId);
    if(it != _synth->paramIdToIndex.end())
    {
      const clap_param_info_t& pi = _synth->paramInfo[it->second];
      ev.cookie = pi.cookie;
      if(value < pi.min_value || value > pi.max_value)
      {
        fprintf(stderr,
          "ClapInstanceCore::pushParamValueEvent: '%s' param %u value %.6f out of "
          "range [%.6f, %.6f] — clamping\n",
          _displayName.toLocal8Bit().constData(), paramId, value, pi.min_value, pi.max_value);
        ev.value = value < pi.min_value ? pi.min_value : pi.max_value;
      }
    }
  }
  return appendEvent(&ev, sizeof(ev), sampleOffset);
}

//---------------------------------------------------------
//   sortInEvents
//   Stable insertion sort of the queued input-event slots by header.time.
//   Counts are small (<= _evBufCapacity), and the buffer is nearly sorted
//   already, so insertion sort is cheap and realtime-safe (no allocation).
//---------------------------------------------------------

void ClapInstanceCore::sortInEvents()
{
  if(_evInCount < 2)
    return;

  uint8_t tmp[kClapEventSlotSize];
  for(uint32_t i = 1; i < _evInCount; ++i)
  {
    memcpy(tmp, _evInBuf + i * kClapEventSlotSize, kClapEventSlotSize);
    const uint32_t t = reinterpret_cast<const clap_event_header_t*>(tmp)->time;

    int64_t j = static_cast<int64_t>(i) - 1;
    while(j >= 0 &&
          reinterpret_cast<const clap_event_header_t*>(
            _evInBuf + j * kClapEventSlotSize)->time > t)
    {
      memcpy(_evInBuf + (j + 1) * kClapEventSlotSize,
             _evInBuf + j * kClapEventSlotSize, kClapEventSlotSize);
      --j;
    }
    memcpy(_evInBuf + (j + 1) * kClapEventSlotSize, tmp, kClapEventSlotSize);
  }
}

//---------------------------------------------------------
//   runProcess
//---------------------------------------------------------

clap_process_status ClapInstanceCore::runProcess(int64_t steadyTime, uint32_t nframes,
                                                  float** ins, uint32_t numInChans,
                                                  float** outs, uint32_t numOutChans)
{
  if(!_plugin)
  {
    fprintf(stderr, "ClapInstanceCore::runProcess: _plugin is nullptr (%s)\n",
            _displayName.toLocal8Bit().constData());
    return CLAP_PROCESS_ERROR;
  }

  // --- Verification only, not correctness logic ---
  // A genuine threading violation is a hard error and always worth printing
  // (the alternative — silent corruption — is worse); it should also never
  // be frequent enough to matter for RT budget.
  if(!hostIsAudioThread())
    fprintf(stderr, "ClapInstanceCore::runProcess: called OFF the audio thread (%s)\n",
            _displayName.toLocal8Bit().constData());
  if(_inRunProcess.exchange(true))
    fprintf(stderr,
      "ClapInstanceCore::runProcess: RE-ENTRANT call detected for '%s' — "
      "two threads are inside process() for this instance concurrently\n",
      _displayName.toLocal8Bit().constData());
  struct RunProcessGuard
  {
    std::atomic<bool>* flag;
    ~RunProcessGuard() { *flag = false; }
  } runProcessGuard{ &_inRunProcess };

  // Service a restart request (plugin called host->request_restart()): stop
  // here on the audio thread, then marshal deactivate()+reactivate() to the main
  // thread and re-arm start_processing(). Done before the stop/start servicing
  // below so a plain restart takes precedence.
  // _restartInFlight coalesces repeated request_restart() calls the same way
  // _activateRequestPending coalesces activate(): some plugins call
  // request_restart() once per GUI frame right after gui->create() (e.g. while
  // they settle on a port/latency configuration), which without this guard
  // queued a fresh deactivate()+activate() lambda on every single audio block
  // until the plugin stopped asking — each cycle "succeeded" (no error), but
  // the resulting stop/reactivate/start churn is what produced audible noise
  // right after opening the GUI.
  if(_restartReq.exchange(false) && !_restartInFlight.exchange(true))
  {
    if(_clapProcessing)
    {
      _plugin->stop_processing(_plugin);
      _clapProcessing = false;
    }
    const clap_plugin_t* plug = _plugin;
    auto alive = _instanceAlive;
    ClapInstanceCore* self = this;
    const double   sr  = static_cast<double>(MusEGlobal::sampleRate);
    const uint32_t seg = MusEGlobal::segmentSize;
    QMetaObject::invokeMethod(qApp,
      [plug, alive, self, sr, seg]()
      {
        if(!*alive || !self || self->_teardown)
        {
          if(*alive && self) self->_restartInFlight = false;
          return;
        }
        // deactivate + reactivate, both [main-thread], sequential in one lambda
        // (so no "activated twice" race). exchange() keeps deactivate exact-once.
        if(self->_pluginActivated.exchange(false))
          plug->deactivate(plug);
        // Only reactivate if still logically supposed to be active — a real
        // deactivate() (possibly from the audio thread) may have landed
        // while this restart was in flight; don't resurrect an instance the
        // user just turned off.
        if(self->_curActiveState)
        {
          if(plug->activate(plug, sr, 1, seg))
          {
            self->_pluginActivated    = true;
            self->_startProcessingReq = true; // audio thread restarts processing
          }
          else
          {
            self->_curActiveState = false;
            fprintf(stderr, "ClapInstanceCore: restart reactivate failed (%s)\n",
                    self->_displayName.toLocal8Bit().constData());
          }
        }
        self->_restartInFlight = false;
      },
      Qt::QueuedConnection);
  }

  // Service any pending stop_processing() request first (audio-thread-only).
  // The matching plugin->deactivate() (main-thread-only) is marshalled back
  // to the main thread here, guarded so it only fires if we're still
  // logically inactive by the time the main thread gets to it (a later
  // activate() may have already reversed the request).
  if(_stopProcessingReq.exchange(false))
  {
    if(_clapProcessing)
    {
      _plugin->stop_processing(_plugin);
      _clapProcessing = false;
    }
    const clap_plugin_t* plug = _plugin;
    auto alive = _instanceAlive;
    ClapInstanceCore* self = this;
    QMetaObject::invokeMethod(qApp,
      [plug, alive, self]()
      {
        // exchange() guarantees plugin->deactivate() runs exactly once even if
        // a shutdown-time direct deactivate races this queued lambda.
        if(*alive && self && !self->_curActiveState && self->_pluginActivated.exchange(false))
          plug->deactivate(plug);       // [main-thread] — legal here
      },
      Qt::QueuedConnection);
  }

  if(_startProcessingReq.exchange(false))
  {
    if(!_clapProcessing)
    {
      if(_plugin->start_processing(_plugin))
        _clapProcessing = true;
      else
        fprintf(stderr, "ClapInstanceCore::runProcess: start_processing() failed (%s)\n",
                _displayName.toLocal8Bit().constData());
    }
  }

  if(!_clapProcessing)
  {
    _evInCount  = 0;
    _evOutCount = 0;
    return CLAP_PROCESS_SLEEP;
  }

  // CLAP requires the in_events stream sorted ascending by time.
  sortInEvents();

  // Build buffers for the plugin's FULL declared port layout (not just the
  // MusE-connected main port). The main port gets the caller's connected
  // channels (silence-substituted per channel if a pointer is missing/null);
  // every other declared port (e.g. a vocoder's modulator input) gets silence
  // (inputs) or a throwaway buffer (outputs). Skipping the aux ports makes the
  // plugin read/write past audio_inputs[]/audio_outputs[] and crash.
  const size_t nInPorts  = _procInPortChans.size();
  const size_t nOutPorts = _procOutPortChans.size();

  size_t totalInChans = 0;
  for(uint32_t c : _procInPortChans)  totalInChans  += c;
  size_t totalOutChans = 0;
  for(uint32_t c : _procOutPortChans) totalOutChans += c;

  // Lazily (re)size the shared silence/throwaway buffers and the flat
  // channel-pointer scratch. Silence must be re-zeroed each call in case a
  // previous plugin wrote through a shared pointer (defensive; inputs shouldn't
  // be written, but cheap).
  if(_silenceBuf.size()  < nframes) _silenceBuf.assign(nframes, 0.0f);
  else std::fill(_silenceBuf.begin(), _silenceBuf.begin() + nframes, 0.0f);
  if(_dummyOutBuf.size() < nframes) _dummyOutBuf.resize(nframes);
  _inChanScratch.assign(totalInChans   > 0 ? totalInChans  : 1, nullptr);
  _outChanScratch.assign(totalOutChans > 0 ? totalOutChans : 1, nullptr);

  clap_audio_buffer_t inBufs[nInPorts > 0 ? nInPorts : 1];
  clap_audio_buffer_t outBufs[nOutPorts > 0 ? nOutPorts : 1];

  // Inputs: main port channels come from `ins`; all others are silence.
  {
    size_t scratchOff = 0;
    uint32_t connOff = 0; // index into the caller's flat `ins` (main port only)
    for(size_t p = 0; p < nInPorts; ++p)
    {
      const uint32_t chans = _procInPortChans[p];
      const bool isMain = (static_cast<int>(p) == _mainInPortIdx);
      for(uint32_t c = 0; c < chans; ++c)
      {
        float* ptr = _silenceBuf.data();
        if(isMain && connOff < numInChans && ins && ins[connOff])
          ptr = ins[connOff];
        if(isMain) ++connOff;
        _inChanScratch[scratchOff + c] = ptr;
      }
      inBufs[p] = clap_audio_buffer_t{};
      inBufs[p].data32        = &_inChanScratch[scratchOff];
      inBufs[p].channel_count = chans;
      scratchOff += chans;
    }
  }

  // Outputs: main port channels go to `outs`; all others to the throwaway buffer.
  {
    size_t scratchOff = 0;
    uint32_t connOff = 0; // index into the caller's flat `outs` (main port only)
    for(size_t p = 0; p < nOutPorts; ++p)
    {
      const uint32_t chans = _procOutPortChans[p];
      const bool isMain = (static_cast<int>(p) == _mainOutPortIdx);
      for(uint32_t c = 0; c < chans; ++c)
      {
        float* ptr = _dummyOutBuf.data();
        if(isMain && connOff < numOutChans && outs && outs[connOff])
          ptr = outs[connOff];
        if(isMain) ++connOff;
        _outChanScratch[scratchOff + c] = ptr;
      }
      outBufs[p] = clap_audio_buffer_t{};
      outBufs[p].data32        = &_outChanScratch[scratchOff];
      outBufs[p].channel_count = chans;
      scratchOff += chans;
    }
  }

  clap_process_t proc{};
  proc.steady_time         = steadyTime;
  proc.frames_count        = nframes;
  proc.transport           = nullptr; // TODO: fill transport info
  proc.audio_inputs        = (nInPorts > 0) ? inBufs : nullptr;
  proc.audio_inputs_count  = static_cast<uint32_t>(nInPorts);
  proc.audio_outputs       = (nOutPorts > 0) ? outBufs : nullptr;
  proc.audio_outputs_count = static_cast<uint32_t>(nOutPorts);
  proc.in_events           = &_clapEvIn;
  proc.out_events          = &_clapEvOut;

  #ifdef CLAP_DEBUG_PROCESS
  const auto t0 = std::chrono::steady_clock::now();
  #endif
  const clap_process_status status = _plugin->process(_plugin, &proc);
  #ifdef CLAP_DEBUG_PROCESS
  const auto t1 = std::chrono::steady_clock::now();
  const double elapsedMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
  const double budgetMs  = 1000.0 * static_cast<double>(nframes) /
                            static_cast<double>(MusEGlobal::sampleRate);
  if(elapsedMs > budgetMs * 0.8 && clapDebugGateReady(_lastOverrunPrintUs, 1000))
    fprintf(stderr,
      "ClapInstanceCore::runProcess: '%s' process() took %.3fms of a %.3fms budget "
      "(nframes=%u)\n",
      _displayName.toLocal8Bit().constData(), elapsedMs, budgetMs, nframes);
  #endif
  if(status == CLAP_PROCESS_ERROR)
    fprintf(stderr, "ClapInstanceCore::runProcess: plugin->process() returned error (%s)\n",
            _displayName.toLocal8Bit().constData());

  _evInCount = 0;
  return status;
}

const clap_event_header_t* ClapInstanceCore::outputEvent(uint32_t idx) const
{
  if(idx >= _evOutCount)
    return nullptr;
  return reinterpret_cast<const clap_event_header_t*>(_evOutBuf + idx * kClapEventSlotSize);
}

//---------------------------------------------------------
//   Parameters
//---------------------------------------------------------

double ClapInstanceCore::getParameter(clap_id paramId) const
{
  if(!_extParams || !_plugin)
    return 0.0;
  double val = 0.0;
  if(!_extParams->get_value(_plugin, paramId, &val))
    fprintf(stderr, "ClapInstanceCore::getParameter: get_value failed for id %u (%s)\n",
            paramId, _displayName.toLocal8Bit().constData());
  return val;
}

void ClapInstanceCore::setParameter(clap_id paramId, double value)
{
  pushParamValueEvent(paramId, value, 0);
}

//---------------------------------------------------------
//   State persistence (clap.state extension)
//---------------------------------------------------------

std::vector<QString> ClapInstanceCore::getCustomData() const
{
  std::vector<QString> out;
  if(!_extState || !_plugin)
    return out;

  QByteArray blob;
  clap_ostream_t ostream{};
  ostream.ctx = &blob;
  ostream.write = [](const clap_ostream_t* s, const void* buf, uint64_t size) -> int64_t {
    QByteArray* ba = static_cast<QByteArray*>(s->ctx);
    ba->append(static_cast<const char*>(buf), static_cast<int>(size));
    return static_cast<int64_t>(size);
  };

  if(_extState->save(_plugin, &ostream) && !blob.isEmpty())
  {
    fprintf(stderr, "ClapInstanceCore::getCustomData: saved %d bytes for '%s'\n",
            int(blob.size()), _displayName.toLocal8Bit().constData());
    out.push_back(QString::fromLatin1(blob.toBase64()));
  }
  else
    fprintf(stderr, "ClapInstanceCore::getCustomData: state save failed/empty for '%s'\n",
            _displayName.toLocal8Bit().constData());
  return out;
}

bool ClapInstanceCore::setCustomData(const std::vector<QString>& d)
{
  if(d.empty() || !_extState || !_plugin)
    return false;

  // We store exactly one base64 blob (the CLAP state stream).
  const QByteArray blob = QByteArray::fromBase64(d.front().toLatin1());
  if(blob.isEmpty())
  {
    fprintf(stderr, "ClapInstanceCore::setCustomData: empty/invalid blob for '%s'\n",
            _displayName.toLocal8Bit().constData());
    return false;
  }
  fprintf(stderr, "ClapInstanceCore::setCustomData: decoded %d bytes for '%s'\n",
          int(blob.size()), _displayName.toLocal8Bit().constData());

  struct ReadCtx { const char* p; int64_t rem; };
  ReadCtx rc{ blob.constData(), blob.size() };
  clap_istream_t istream{};
  istream.ctx = &rc;
  istream.read = [](const clap_istream_t* s, void* buf, uint64_t size) -> int64_t {
    ReadCtx* c = static_cast<ReadCtx*>(s->ctx);
    int64_t n = std::min((int64_t)size, c->rem);
    if(n <= 0) return 0;
    memcpy(buf, c->p, (size_t)n);
    c->p += n; c->rem -= n;
    return n;
  };

  // Plugin must be deactivated for state load per CLAP spec. We go through
  // the normal activate()/deactivate()/runProcess() request machinery so we
  // don't call plugin->deactivate() off the audio thread here.
  const bool wasActive = _curActiveState;
  if(wasActive)
  {
    deactivate();
    if(_clapProcessing)
    {
      _plugin->stop_processing(_plugin);
      _clapProcessing = false;
    }
    _stopProcessingReq = false;
    if(_pluginActivated.exchange(false))
      _plugin->deactivate(_plugin);
  }

  const bool ok = _extState->load(_plugin, &istream);
  fprintf(stderr, "ClapInstanceCore::setCustomData: state->load=%d rem=%lld\n",
          ok, (long long)rc.rem);
  if(ok)
    // May run on the main thread during song load; finish any deferred
    // patch-apply (e.g. Surge's preset name/category) so the displayed
    // preset matches the restored state.
    _plugin->on_main_thread(_plugin);

  if(wasActive)
    activate();

  return ok;
}

//---------------------------------------------------------
//   Host callback implementations
//---------------------------------------------------------

const void* ClapInstanceCore::hostGetExtension(const char* ext_id)
{
  if(strcmp(ext_id, CLAP_EXT_PARAMS) == 0) return &s_hostParamsExt;
  if(strcmp(ext_id, CLAP_EXT_LOG)    == 0) return &s_hostLogExt;
  if(strcmp(ext_id, CLAP_EXT_GUI)    == 0) return clapCoreGuiHostExt();
  if(strcmp(ext_id, CLAP_EXT_TIMER_SUPPORT)    == 0) return clapCoreTimerHostExt();
  if(strcmp(ext_id, CLAP_EXT_POSIX_FD_SUPPORT) == 0) return clapCorePosixFdHostExt();
  if(strcmp(ext_id, CLAP_EXT_THREAD_CHECK) == 0) return &s_hostThreadCheckExt;
  return nullptr;
}

void ClapInstanceCore::hostRequestRestart()
{
  // request_restart() may be called from any thread, including the audio
  // thread — throttle so a plugin calling it repeatedly can't turn this
  // into RT-thread fprintf spam.
  if(clapDebugGateReady(_lastRestartPrintUs, 1000))
    fprintf(stderr, "ClapInstanceCore::hostRequestRestart: plugin '%s' requested restart\n",
            _displayName.toLocal8Bit().constData());
  if(!_plugin || _teardown)
    return;
  // Serviced by runProcess() on the audio thread (stop), then a marshaled
  // main-thread deactivate()+activate(). request_restart may come from any
  // thread, so just set the atomic flag here.
  _restartReq = true;
}

void ClapInstanceCore::hostRequestCallback()
{
  #ifdef CLAP_DEBUG
  printf("ClapInstanceCore::hostRequestCallback\n");
  #endif
  // CLAP: request_callback may be called from any thread (incl. audio). Marshal
  // _plugin->on_main_thread() onto the Qt main thread. Surge XT defers the bulk
  // of its patch application (e.g. updating the displayed preset name) into this
  // callback, so skipping it leaves a stale preset label after state load.
  const clap_plugin_t* plug = _plugin;
  auto alive = _instanceAlive;
  QMetaObject::invokeMethod(qApp,
    [plug, alive]()
    {
      if(*alive && plug)
        plug->on_main_thread(plug);
    },
    Qt::QueuedConnection);
}

void ClapInstanceCore::hostParamsRescan(clap_param_rescan_flags flags)
{
  (void)flags;
  #ifdef CLAP_DEBUG
  printf("ClapInstanceCore::hostParamsRescan flags:%u\n", flags);
  #endif
}

void ClapInstanceCore::hostParamsRequestFlush()
{
  #ifdef CLAP_DEBUG
  printf("ClapInstanceCore::hostParamsRequestFlush\n");
  #endif
}

bool ClapInstanceCore::hostIsMainThread() const
{
  return QThread::currentThread() == qApp->thread();
}

bool ClapInstanceCore::hostIsAudioThread() const
{
  // True only on MusE's real RT audio thread, whichever backend owns it.
  return MusEGlobal::audio && MusEGlobal::audio->isAudioThread();
}

//---------------------------------------------------------
//   deactivateAllBeforeAudioShutdown
//---------------------------------------------------------

std::vector<ClapInstanceCore*> ClapInstanceCore::s_liveInstances;

void ClapInstanceCore::deactivateAllBeforeAudioShutdown(int perInstanceTimeoutMs)
{
  // MUST run while the audio engine is still live (BEFORE seqStop() /
  // Audio::stop()) — see the call site in MusE::closeEvent().
  //
  // Diva/u-he enforce a hard THREAD-IDENTITY check: plugin->stop_processing()
  // must run on MusE's real audio thread and aborts otherwise. We drive it
  // deterministically through the audio message system: Audio::processMsg()
  // runs on the audio thread at the TOP of Audio::process() (before its idle/
  // checkAudioDevice early-outs), so AUDIO_CLAP_STOP_PROCESSING reaches the
  // plugin even when the engine is idle at quit — unlike the passive
  // getData()->runProcess() path, which is skipped when idle and caused the
  // 3000ms timeouts. plugin->deactivate() is [main-thread] and is done here.
  //
  // perInstanceTimeoutMs is now unused??? — the message round-trip is
  // synchronous (sendMsg blocks until the audio thread confirms), so there's
  // nothing to poll. Kept for API/header compatibility.
  (void)perInstanceTimeoutMs;

  const std::vector<ClapInstanceCore*> instances = s_liveInstances;

  // Phase 1: suppress (re)activation on EVERY instance up front, so a queued
  // marshaled activate() lambda can't re-activate a plugin mid-teardown
  // ("Plugin was activated twice" abort).
  for(ClapInstanceCore* core : instances)
    if(core)
      core->_teardown = true;

  // Phase 2: stop_processing() for all instances, executed on the audio thread.
  // Only if the engine is running: if it isn't, sendMsg() would run processMsg()
  // inline on THIS (main) thread and Diva would abort stop_processing()'s
  // thread check. (If it isn't running, nothing is processing anyway.)
  bool anyProcessing = false;
  for(ClapInstanceCore* core : instances)
    if(core && core->_plugin && core->_clapProcessing) { anyProcessing = true; break; }

  if(anyProcessing)
  {
    if(MusEGlobal::audio && MusEGlobal::audio->isRunning())
      MusEGlobal::audio->msgClapStopProcessing(); // audio thread stops them all
    else
      fprintf(stderr,
        "ClapInstanceCore::deactivateAllBeforeAudioShutdown: audio engine not running; "
        "cannot stop_processing() on the audio thread — some plugins may warn on destroy\n");
  }

  // Phase 3: deactivate each instance on this (main) thread, exactly once.
  for(ClapInstanceCore* core : instances)
  {
    if(!core || !core->_plugin)
      continue;
    core->_curActiveState     = false;
    core->_startProcessingReq = false;
    core->_stopProcessingReq  = false;
    core->_restartInFlight    = false;
    if(core->_pluginActivated.exchange(false))
      core->_plugin->deactivate(core->_plugin);
  }
}

//---------------------------------------------------------
//   stopAllProcessingOnAudioThread
//   Runs inside Audio::processMsg (AUDIO_CLAP_STOP_PROCESSING), i.e. on the
//   audio thread. Stops every instance currently in the processing state.
//---------------------------------------------------------

void ClapInstanceCore::stopAllProcessingOnAudioThread()
{
  for(ClapInstanceCore* core : s_liveInstances)
  {
    if(!core || !core->_plugin)
      continue;
    if(core->_clapProcessing)
    {
      core->_plugin->stop_processing(core->_plugin);
      core->_clapProcessing     = false;
    }
    core->_startProcessingReq = false;
    core->_stopProcessingReq  = false;
  }
}

void clapDeactivateAllBeforeAudioShutdown()
{
  ClapInstanceCore::deactivateAllBeforeAudioShutdown();
}

} // namespace MusECore

#endif // CLAP_SUPPORT
