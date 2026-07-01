//=============================================================================
//  MusE
//  Linux Music Editor
//
//  clap_host.cpp
//  CLAP (CLever Audio Plugin) host integration for MusE
//
//  Copyright (C) 1999-2011 by Werner Schweer and others
//  (C) Copyright 2011-2016 Tim E. Real (terminator356 on sourceforge)
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
// Turn on constant flow of process debugging messages
//#define CLAP_DEBUG_PROCESS

#include <string.h>
#include <stdlib.h>
#include <stdio.h>


#include <QThread>
#include <QDir>
#include <QFileInfo>
#include <QCoreApplication>   // qApp, for marshalling on_main_thread() to GUI thread
#include <QMetaObject>

#include <clap/clap.h>
#include <clap/ext/params.h>
#include <clap/ext/audio-ports.h>
#include <clap/ext/gui.h>
#include <clap/ext/log.h>
#include <clap/ext/state.h>
#include <clap/ext/timer-support.h>
#include <clap/ext/posix-fd-support.h>
#include <clap/factory/plugin-factory.h>
#include <clap/ext/thread-check.h>


#include "clap_host.h"
#include "synth.h"
#include "audio.h"
#include "jackaudio.h"
#include "midi_consts.h"
#include "midiport.h"
#include "minstrument.h"
#include "plugin.h"
#include "controlfifo.h"
#include "xml.h"
#include "song.h"
#include "ctrl.h"
#include "app.h"
#include "globals.h"
#include "gconfig.h"
#include "popupmenu.h"
#include "lock_free_buffer.h"
#include "pluglist.h"

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
//   Clap - Thread Check - extension
//---------------------------------------------------------

static bool CLAP_ABI clapHostIsMainThread(const clap_host_t* host)
{ return hostFromClap(host)->hostIsMainThread(); }

static bool CLAP_ABI clapHostIsAudioThread(const clap_host_t* host)
{ return hostFromClap(host)->hostIsAudioThread(); }

static const clap_host_thread_check_t s_hostThreadCheckExt = {
  clapHostIsMainThread,
  clapHostIsAudioThread,
};


//---------------------------------------------------------
//   initCLAP
//---------------------------------------------------------

void initCLAP()
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
    if(!(info._class & MusEPlugin::PluginClassEffect ||
         info._class & MusEPlugin::PluginClassInstrument))
      continue;

    const QString inf_uri   = PLUGIN_GET_QSTRING(info._uri);
    const QString inf_label = PLUGIN_GET_QSTRING(info._label);

    if(const Synth* sy = MusEGlobal::synthis.find(
         info._type,
         PLUGIN_GET_QSTRING(info._completeBaseName),
         inf_uri, inf_label))
    {
      fprintf(stderr,
        "Ignoring CLAP synth label:%s uri:%s path:%s duplicate of path:%s\n",
        inf_label.toLocal8Bit().constData(),
        inf_uri.toLocal8Bit().constData(),
        PLUGIN_GET_QSTRING(info.filePath()).toLocal8Bit().constData(),
        sy->filePath().toLocal8Bit().constData());
    }
    else
    {
      ClapSynth* s = new ClapSynth(info);
      MusEGlobal::synthis.push_back(s);
    }
  }
}



//---------------------------------------------------------
//   ClapSynth
//---------------------------------------------------------

ClapSynth::ClapSynth(const MusEPlugin::PluginScanInfoStruct& info)
  : Synth(info),
    _entry(nullptr),
    _factory(nullptr),
    _desc(nullptr)
{
  // Port counts from scan info (may be 0; filled properly on first instantiation)
  _portCount       = info._portCount;
  _inports         = info._inports;
  _outports        = info._outports;
  _controlInPorts  = info._controlInPorts;
  _controlOutPorts = info._controlOutPorts;
}

ClapSynth::~ClapSynth()
{
  if(_entry)
    fprintf(stderr, "ClapSynth::~ClapSynth Warning: _entry not NULL — was release() called?\n");
}

//---------------------------------------------------------
//   ClapSynth::createSIF
//---------------------------------------------------------

SynthIF* ClapSynth::createSIF(SynthI* synti)
{
  if(!reference())
    return nullptr;

  ClapSynthIF* sif = new ClapSynthIF(synti);
  if(!sif->init(this))
  {
    fprintf(stderr, "ClapSynth::createSIF() Error: plugin:%s instantiation failed!\n",
            _label.toLocal8Bit().constData());
    delete sif;
    release();
    return nullptr;
  }
  return sif;
}

//---------------------------------------------------------
//   ClapSynth::reference
//---------------------------------------------------------

bool ClapSynth::reference()
{
  if(_references == 0)
  {
    _qlib.setFileName(filePath());
    _qlib.setLoadHints(QLibrary::ResolveAllSymbolsHint);
    if(!_qlib.load())
    {
      fprintf(stderr, "ClapSynth::reference(): load (%s) failed: %s\n",
              _qlib.fileName().toLocal8Bit().constData(),
              _qlib.errorString().toLocal8Bit().constData());
      return false;
    }

    _entry = reinterpret_cast<const clap_plugin_entry_t*>(
               _qlib.resolve("clap_entry"));
    if(!_entry)
    {
      fprintf(stderr, "ClapSynth::reference(): cannot resolve 'clap_entry' in %s\n",
              _qlib.fileName().toLocal8Bit().constData());
      _qlib.unload();
      return false;
    }

    _entry->init(_qlib.fileName().toStdString().c_str());

    _factory = static_cast<const clap_plugin_factory_t*>(
                 _entry->get_factory(CLAP_PLUGIN_FACTORY_ID));
    if(!_factory)
    {
      fprintf(stderr, "ClapSynth::reference(): no plugin factory in %s\n",
              _qlib.fileName().toLocal8Bit().constData());
      _entry->deinit();
      _entry = nullptr;
      _qlib.unload();
      return false;
    }

    _desc = nullptr;
    const uint32_t count = _factory->get_plugin_count(_factory);
    for(uint32_t i = 0; i < count; ++i)
    {
      const clap_plugin_descriptor_t* d = _factory->get_plugin_descriptor(_factory, i);
      if(!d) continue;
      if(QString(d->id) == _label)
      {
        _desc = d;
        break;
      }
    }

    if(!_desc)
    {
      fprintf(stderr, "ClapSynth::reference(): cannot find plugin id '%s' in %s\n",
              _label.toLocal8Bit().constData(),
              _qlib.fileName().toLocal8Bit().constData());
      _entry->deinit();
      _entry   = nullptr;
      _factory = nullptr;
      _qlib.unload();
      return false;
    }

    if(!clap_version_is_compatible(_desc->clap_version))
    {
      fprintf(stderr,
        "ClapSynth::reference(): incompatible clap version for '%s': "
        "plugin %u.%u.%u vs host %u.%u.%u\n",
        _label.toLocal8Bit().constData(),
        _desc->clap_version.major, _desc->clap_version.minor, _desc->clap_version.revision,
        CLAP_VERSION.major, CLAP_VERSION.minor, CLAP_VERSION.revision);
      _entry->deinit();
      _entry   = nullptr;
      _factory = nullptr;
      _desc    = nullptr;
      _qlib.unload();
      return false;
    }

    // Reset port counts — will be filled properly in ClapSynthIF::init()
    _inports         = 0;
    _outports        = 0;
    _controlInPorts  = 0;
    _controlOutPorts = 0;
    iIdx.clear();
    oIdx.clear();
    paramIds.clear();
    paramInfo.clear();
    paramIdToIndex.clear();
  }

  ++_references;

  if(!_desc)
  {
    fprintf(stderr, "ClapSynth::reference() Error: cannot find CLAP plugin %s\n",
            _label.toLocal8Bit().constData());
    release();
    return false;
  }
  return true;
}

//---------------------------------------------------------
//   ClapSynth::release
//---------------------------------------------------------

int ClapSynth::release()
{
  if(_references == 1)
  {
    if(_entry)
    {
      _entry->deinit();
      _entry   = nullptr;
      _factory = nullptr;
      _desc    = nullptr;
    }
    const bool ulres = _qlib.unload();
    (void)ulres;
    #ifdef CLAP_DEBUG
    fprintf(stderr, "ClapSynth::release(): no more instances. Unload result:%d\n", ulres);
    #endif
    iIdx.clear();
    oIdx.clear();
    paramIds.clear();
    paramInfo.clear();
    paramIdToIndex.clear();
  }
  if(_references > 0)
    --_references;
  return _references;
}

//=============================================================================
//  ClapSynthIF
//=============================================================================

ClapSynthIF::ClapSynthIF(SynthI* s)
  : SynthIF(s)
{
  #ifdef CLAP_DEBUG
  printf("ClapSynthIF::ClapSynthIF\n");
  #endif
}

ClapSynthIF::~ClapSynthIF()
{
  #ifdef CLAP_DEBUG
  printf("ClapSynthIF::~ClapSynthIF\n");
  #endif

  // Disarm any queued on_main_thread() lambdas before we tear anything down.
  *_instanceAlive = false;
  
  // Clear events explicitly here before GUI destruction to avoid memory leaks.
  clearGuiEventSources();

  // GUI teardown (plugin gui->destroy() then host window) lives in
  // clap_host_gui.cpp; keep all widget handling out of the core file.
  destroyGui();

  if(_plugin)
  {
    deactivate();
    _plugin->destroy(_plugin);
    _plugin = nullptr;
  }

  if(_synth && _audioInBuffers)
  {
    for(unsigned long i = 0; i < _synth->_inports; ++i)
      if(_audioInBuffers[i]) free(_audioInBuffers[i]);
    delete[] _audioInBuffers;
    _audioInBuffers = nullptr;
  }

  if(_audioInSilenceBuf) { free(_audioInSilenceBuf); _audioInSilenceBuf = nullptr; }

  if(_synth && _audioOutBuffers)
  {
    for(unsigned long i = 0; i < _synth->_outports; ++i)
      if(_audioOutBuffers[i]) free(_audioOutBuffers[i]);
    delete[] _audioOutBuffers;
    _audioOutBuffers = nullptr;
  }

  if(_controls)    { delete[] _controls;    _controls    = nullptr; }
  if(_controlsOut) { delete[] _controlsOut; _controlsOut = nullptr; }

  delete[] _evInBuf;  _evInBuf  = nullptr;
  delete[] _evOutBuf; _evOutBuf = nullptr;
}

//---------------------------------------------------------
//   Static host vtable trampolines
//---------------------------------------------------------

static const void* CLAP_ABI clapHostGetExtension(const clap_host_t* host, const char* ext_id)
{ return hostFromClap(host)->hostGetExtension(ext_id); }

static void CLAP_ABI clapHostRequestRestart(const clap_host_t* host)
{ hostFromClap(host)->hostRequestRestart(); }

static void CLAP_ABI clapHostRequestProcess(const clap_host_t* /*host*/) {}

static void CLAP_ABI clapHostRequestCallback(const clap_host_t* host)
{ hostFromClap(host)->hostRequestCallback(); }

// clap_host_params
static void CLAP_ABI clapHostParamsRescan(const clap_host_t* host, clap_param_rescan_flags flags)
{ hostFromClap(host)->hostParamsRescan(flags); }

static void CLAP_ABI clapHostParamsClear(const clap_host_t* /*host*/,
                                         clap_id /*param_id*/,
                                         clap_param_clear_flags /*flags*/) {}

static void CLAP_ABI clapHostParamsRequestFlush(const clap_host_t* host)
{ hostFromClap(host)->hostParamsRequestFlush(); }

static const clap_host_params_t s_hostParamsExt = {
  clapHostParamsRescan,
  clapHostParamsClear,
  clapHostParamsRequestFlush,
};

// clap_host_log
static void CLAP_ABI clapHostLogLog(const clap_host_t* /*host*/,
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
      fprintf(stderr, "CLAP [log]: %s\n", msg);
      break;
  }
}

static const clap_host_log_t s_hostLogExt = { clapHostLogLog };



// clap_host_timer_support
static bool CLAP_ABI clapHostTimerRegister(const clap_host_t* host,
                                           uint32_t period_ms, clap_id* timer_id)
{ return hostFromClap(host)->hostTimerRegister(period_ms, timer_id); }

static bool CLAP_ABI clapHostTimerUnregister(const clap_host_t* host, clap_id timer_id)
{ return hostFromClap(host)->hostTimerUnregister(timer_id); }

static const clap_host_timer_support_t s_hostTimerExt = {
  clapHostTimerRegister,
  clapHostTimerUnregister,
};

// clap_host_posix_fd_support
static bool CLAP_ABI clapHostFdRegister(const clap_host_t* host,
                                        int fd, clap_posix_fd_flags_t flags)
{ return hostFromClap(host)->hostFdRegister(fd, flags); }

static bool CLAP_ABI clapHostFdModify(const clap_host_t* host,
                                      int fd, clap_posix_fd_flags_t flags)
{ return hostFromClap(host)->hostFdModify(fd, flags); }

static bool CLAP_ABI clapHostFdUnregister(const clap_host_t* host, int fd)
{ return hostFromClap(host)->hostFdUnregister(fd); }

static const clap_host_posix_fd_support_t s_hostPosixFdExt = {
  clapHostFdRegister,
  clapHostFdModify,
  clapHostFdUnregister,
};




//---------------------------------------------------------
//   clap - Thread Check - extension 
//---------------------------------------------------------

bool ClapSynthIF::hostIsMainThread() const
{
  return QThread::currentThread() == qApp->thread();
}

bool ClapSynthIF::hostIsAudioThread() const
{
  // True only on MusE's real RT audio thread, whichever backend owns it.
  return MusEGlobal::audio && MusEGlobal::audio->isAudioThread();
}

//---------------------------------------------------------
//   Host callback implementations
//---------------------------------------------------------

const void* ClapSynthIF::hostGetExtension(const char* ext_id)
{
  if(strcmp(ext_id, CLAP_EXT_PARAMS) == 0) return &s_hostParamsExt;
  if(strcmp(ext_id, CLAP_EXT_LOG)    == 0) return &s_hostLogExt;
  if(strcmp(ext_id, CLAP_EXT_GUI)    == 0) return clapGuiHostExt();
  if(strcmp(ext_id, CLAP_EXT_TIMER_SUPPORT)    == 0) return &s_hostTimerExt;
  if(strcmp(ext_id, CLAP_EXT_POSIX_FD_SUPPORT) == 0) return &s_hostPosixFdExt;
  if(strcmp(ext_id, CLAP_EXT_THREAD_CHECK) == 0) return &s_hostThreadCheckExt;
  return nullptr;
}

void ClapSynthIF::hostRequestRestart()
{
  fprintf(stderr, "ClapSynthIF::hostRequestRestart: plugin '%s' requested restart\n",
          _synth ? _synth->name().toLocal8Bit().constData() : "(unknown)");
  // TODO: post restart request to MusE main event queue
}


void ClapSynthIF::hostRequestCallback()
{
  #ifdef CLAP_DEBUG
  printf("ClapSynthIF::hostRequestCallback\n");
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

void ClapSynthIF::hostParamsRescan(clap_param_rescan_flags flags)
{
  (void)flags;
  #ifdef CLAP_DEBUG
  printf("ClapSynthIF::hostParamsRescan flags:%u\n", flags);
  #endif
}

void ClapSynthIF::hostParamsRequestFlush()
{
  #ifdef CLAP_DEBUG
  printf("ClapSynthIF::hostParamsRequestFlush\n");
  #endif
}

//---------------------------------------------------------
//   buildHostVtable
//---------------------------------------------------------

void ClapSynthIF::buildHostVtable()
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

bool ClapSynthIF::init(ClapSynth* s)
{
  #ifdef CLAP_DEBUG
  printf("ClapSynthIF::init\n");
  #endif

  _synth = s;
  buildHostVtable();

  _plugin = _synth->_factory->create_plugin(
    _synth->_factory, &_clapHost, _synth->_desc->id);

  if(!_plugin)
  {
    fprintf(stderr, "ClapSynthIF::init: create_plugin() returned nullptr for '%s'\n",
            _synth->_desc->id);
    return false;
  }

  if(!_plugin->init(_plugin))
  {
    fprintf(stderr, "ClapSynthIF::init: plugin->init() failed for '%s'\n",
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
  unsigned long inports  = 0;
  unsigned long outports = 0;
  _inPortChans.clear();
  _outPortChans.clear();
  if(_extAudioPorts)
  {
    const uint32_t inCount  = _extAudioPorts->count(_plugin, true);
    const uint32_t outCount = _extAudioPorts->count(_plugin, false);
    for(uint32_t p = 0; p < inCount; ++p)
    {
      clap_audio_port_info_t pi{};
      if(_extAudioPorts->get(_plugin, p, true, &pi))
      {
        inports += pi.channel_count;
        _inPortChans.push_back(pi.channel_count);
      }
    }
    for(uint32_t p = 0; p < outCount; ++p)
    {
      clap_audio_port_info_t pi{};
      if(_extAudioPorts->get(_plugin, p, false, &pi))
      {
        outports += pi.channel_count;
        _outPortChans.push_back(pi.channel_count);
      }
    }
  }
  else
  {
    // Safe fallback: assume a single stereo in and out port.
    inports  = 2;
    outports = 2;
    _inPortChans.push_back(2);
    _outPortChans.push_back(2);
  }

  _synth->_inports  = inports;
  _synth->_outports = outports;

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
        fprintf(stderr, "ClapSynthIF::init: get_info failed for param index %u\n", i);
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

  // --- Allocate audio input buffers ---
  if(inports != 0)
  {
    int rv = posix_memalign((void**)&_audioInSilenceBuf, 16,
                             sizeof(float) * MusEGlobal::segmentSize);
    if(rv != 0) { fprintf(stderr, "ClapSynthIF::init: posix_memalign error:%d\n", rv); abort(); }

    if(MusEGlobal::config.useDenormalBias)
      for(unsigned q = 0; q < MusEGlobal::segmentSize; ++q)
        _audioInSilenceBuf[q] = MusEGlobal::denormalBias;
    else
      memset(_audioInSilenceBuf, 0, sizeof(float) * MusEGlobal::segmentSize);

    _audioInBuffers = new float*[inports];
    for(unsigned long k = 0; k < inports; ++k)
    {
      rv = posix_memalign((void**)&_audioInBuffers[k], 16,
                           sizeof(float) * MusEGlobal::segmentSize);
      if(rv != 0) { fprintf(stderr, "ClapSynthIF::init: posix_memalign error:%d\n", rv); abort(); }
      if(MusEGlobal::config.useDenormalBias)
        for(unsigned q = 0; q < MusEGlobal::segmentSize; ++q)
          _audioInBuffers[k][q] = MusEGlobal::denormalBias;
      else
        memset(_audioInBuffers[k], 0, sizeof(float) * MusEGlobal::segmentSize);
    }
  }

  // --- Allocate audio output buffers ---
  if(outports != 0)
  {
    _audioOutBuffers = new float*[outports];
    for(unsigned long k = 0; k < outports; ++k)
    {
      int rv = posix_memalign((void**)&_audioOutBuffers[k], 16,
                               sizeof(float) * MusEGlobal::segmentSize);
      if(rv != 0) { fprintf(stderr, "ClapSynthIF::init: posix_memalign error:%d\n", rv); abort(); }
      if(MusEGlobal::config.useDenormalBias)
        for(unsigned q = 0; q < MusEGlobal::segmentSize; ++q)
          _audioOutBuffers[k][q] = MusEGlobal::denormalBias;
      else
        memset(_audioOutBuffers[k], 0, sizeof(float) * MusEGlobal::segmentSize);
    }
  }

  // --- Allocate parameter value arrays ---
  if(controlPorts != 0)
    _controls = new Port[controlPorts];

  // --- Initialise control values and hook up MusE automation ---
  if(_extParams && _controls)
  {
    for(unsigned long cip = 0; cip < controlPorts; ++cip)
    {
      const clap_param_info_t& pi = _synth->paramInfo[cip];
      _controls[cip].idx    = cip;
      _controls[cip].val    = static_cast<float>(pi.default_value);
      _controls[cip].enCtrl = !(pi.flags & CLAP_PARAM_IS_READONLY);

      double val = pi.default_value;
      if(_extParams->get_value(_plugin, pi.id, &val))
        _controls[cip].val = static_cast<float>(val);

      const int id = genACnum(MusECore::MAX_PLUGINS, cip);
      CtrlList* cl;
      CtrlListList* cll = track()->controller();
      iCtrlList icl = cll->find(id);
      if(icl == cll->end())
      {
        cl = new CtrlList(id);
        cll->add(cl);
        cl->setCurVal(_controls[cip].val);
      }
      else
      {
        cl = icl->second;
        _controls[cip].val = static_cast<float>(cl->curVal());
      }
      setupController(cl);
    }
  }

  // --- Allocate event buffers ---
  _evBufCapacity = 256;
  _evInBuf  = new uint8_t[_evBufCapacity * kClapEventSlotSize];
  _evOutBuf = new uint8_t[_evBufCapacity * kClapEventSlotSize];
  _evInCount  = 0;
  _evOutCount = 0;

  // Build input events vtable
  _clapEvIn.ctx  = this;
  _clapEvIn.size = [](const clap_input_events_t* list) -> uint32_t {
    return static_cast<ClapSynthIF*>(list->ctx)->_evInCount;
  };
  _clapEvIn.get = [](const clap_input_events_t* list, uint32_t index) -> const clap_event_header_t* {
    ClapSynthIF* self = static_cast<ClapSynthIF*>(list->ctx);
    if(index >= self->_evInCount) return nullptr;
    return reinterpret_cast<const clap_event_header_t*>(
      self->_evInBuf + index * kClapEventSlotSize);
  };

  // Build output events vtable
  _clapEvOut.ctx = this;
  _clapEvOut.try_push = [](const clap_output_events_t* list,
                           const clap_event_header_t* event) -> bool {
    ClapSynthIF* self = static_cast<ClapSynthIF*>(list->ctx);
    if(self->_evOutCount >= self->_evBufCapacity) return false;
    memcpy(self->_evOutBuf + self->_evOutCount * kClapEventSlotSize,
           event, event->size);
    ++self->_evOutCount;
    return true;
  };

  activate();
  return true;
}

//---------------------------------------------------------
//   activate / deactivate
//---------------------------------------------------------

void ClapSynthIF::activate()
{
  if(_curActiveState) return;
  if(!_plugin)
  {
    printf("ClapSynthIF::activate: _plugin is nullptr\n");
    return;
  }
  if(!_plugin->activate(_plugin,
                        static_cast<double>(MusEGlobal::sampleRate),
                        1, MusEGlobal::segmentSize))
  {
    fprintf(stderr, "ClapSynthIF::activate: plugin->activate() failed\n");
    return;
  }
  if(!_plugin->start_processing(_plugin))
  {
    fprintf(stderr, "ClapSynthIF::activate: start_processing() failed\n");
    _plugin->deactivate(_plugin);
    return;
  }
  SynthIF::activate();
}

void ClapSynthIF::deactivate()
{
  if(!_curActiveState) return;
  if(!_plugin)
  {
    printf("ClapSynthIF::deactivate: _plugin is nullptr\n");
    return;
  }
  _plugin->stop_processing(_plugin);
  _plugin->deactivate(_plugin);
  SynthIF::deactivate();
}

void ClapSynthIF::deactivate3() { deactivate(); }

//---------------------------------------------------------
//   getParameter / setParameter
//---------------------------------------------------------

double ClapSynthIF::getParameter(unsigned long n) const
{
  if(!_synth || n >= _synth->_controlInPorts)
  {
    printf("ClapSynthIF::getParameter: index %lu out of range\n", n);
    return 0.0;
  }
  if(!_controls) return 0.0;
  return _controls[n].val;
}

void ClapSynthIF::setParameter(unsigned long n, double v)
{
  addScheduledControlEvent(n, v, MusEGlobal::audio->curFrame());
}

//---------------------------------------------------------
//   receiveEvent
//---------------------------------------------------------

MidiPlayEvent ClapSynthIF::receiveEvent() { return MidiPlayEvent(); }

//---------------------------------------------------------
//   write — save plugin state 
//---------------------------------------------------------

void ClapSynthIF::write(int /*level*/, Xml& /*xml*/) const   // empty, unused !!!
{
  // superseded by setCustomData()
  //
  // State now persists via getCustomData()/setCustomData() (the <customData>
  // framework), so this per-SIF write() emits nothing. The old <clapstate>/
  // <clapparams> path is superseded — the loader never dispatched into it.
  //
  // unused??? — kept for reference, do not re-enable (produced unreadable tags):
  //   if(_extState && _plugin) { ... <clapstate><data>base64</data> ... }
  //   else { ... <clapparams> per-param fallback ... }
}



//---------------------------------------------------------
//   read — load plugin state 
//---------------------------------------------------------

void ClapSynthIF::read( Xml& /*xml*/ ) // empty, unused !!!
{
  // superseded by getCustomData()
}







//---------------------------------------------------------
//   getCustomData — save CLAP state blob (base64) for <customData>
//---------------------------------------------------------

std::vector<QString> ClapSynthIF::getCustomData() const
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
    fprintf(stderr, "ClapSynthIF::getCustomData: saved %d bytes for '%s'\n",
            int(blob.size()), _synth ? _synth->name().toLocal8Bit().constData() : "?");
    out.push_back(QString::fromLatin1(blob.toBase64()));
  }
  else
    fprintf(stderr, "ClapSynthIF::getCustomData: state save failed/empty for '%s'\n",
            _synth ? _synth->name().toLocal8Bit().constData() : "?");
  return out;
}

//---------------------------------------------------------
//   setCustomData — restore CLAP state blob, applied after initInstance()
//---------------------------------------------------------

bool ClapSynthIF::setCustomData(const std::vector<QString>& d)
{
  if(d.empty() || !_extState || !_plugin)
    return false;

  // We store exactly one base64 blob (the CLAP state stream).
  const QByteArray blob = QByteArray::fromBase64(d.front().toLatin1());
  if(blob.isEmpty())
  {
    fprintf(stderr, "ClapSynthIF::setCustomData: empty/invalid blob for '%s'\n",
            _synth ? _synth->name().toLocal8Bit().constData() : "?");
    return false;
  }
  fprintf(stderr, "ClapSynthIF::setCustomData: decoded %d bytes for '%s'\n",
          int(blob.size()), _synth ? _synth->name().toLocal8Bit().constData() : "?");

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

  // Plugin must be deactivated for state load per CLAP spec.
  const bool wasActive = _curActiveState;
  if(wasActive) deactivate();

  const bool ok = _extState->load(_plugin, &istream);
  fprintf(stderr, "ClapSynthIF::setCustomData: state->load=%d rem=%lld\n",
          ok, (long long)rc.rem);
  if(ok)
    // Song load runs on the main thread; finish Surge's deferred patch apply
    // (preset name/category) so the displayed preset matches the restored state.
    _plugin->on_main_thread(_plugin);

  if(wasActive) activate();

  // Resync our _controls[] cache from the plugin after state load.
  if(ok && _extParams && _controls && _synth)
  {
    for(unsigned long i = 0; i < _synth->_controlInPorts; ++i)
    {
      double val = _controls[i].val;
      if(_extParams->get_value(_plugin, _synth->paramInfo[i].id, &val))
        _controls[i].val = static_cast<float>(val);
    }
  }
  return ok;
}





//---------------------------------------------------------
//   processEvent
//---------------------------------------------------------

bool ClapSynthIF::processEvent(const MidiPlayEvent& e, uint32_t sampleOffset)
{
  if(_evInCount >= _evBufCapacity)
  {
    fprintf(stderr, "ClapSynthIF::processEvent: event buffer overflow\n");
    return false;
  }

  int chn = e.channel();
  int a   = e.dataA();
  int b   = e.dataB();

  #ifdef CLAP_DEBUG
  fprintf(stderr, "ClapSynthIF::processEvent type:%d chn:%d a:%d b:%d\n",
          e.type(), chn, a, b);
  #endif

  const MidiInstrument::NoteOffMode nom = synti->noteOffMode();

  switch(e.type())
  {
    case ME_NOTEON:
    {
      int velocity = b;
      uint16_t evtype = CLAP_EVENT_NOTE_ON;
      if(velocity == 0)
      {
        fprintf(stderr, "ClapSynthIF::processEvent: Warning: zero-vel note on ch:%d key:%d\n", chn, a);
        switch(nom)
        {
          case MidiInstrument::NoteOffAll:
            evtype   = CLAP_EVENT_NOTE_OFF;
            velocity = 0;
            break;
          default: break;
        }
      }
      clap_event_note_t ev{};
      ev.header.size     = sizeof(ev);
      ev.header.time     = sampleOffset;
      ev.header.space_id = CLAP_CORE_EVENT_SPACE_ID;
      ev.header.type     = evtype;
      ev.header.flags    = 0;
      ev.note_id         = -1;
      ev.port_index      = 0;
      ev.channel         = chn;
      ev.key             = a;
      ev.velocity        = velocity / 127.0;
      memcpy(_evInBuf + _evInCount * kClapEventSlotSize, &ev, sizeof(ev));
      ++_evInCount;
    }
    break;

    case ME_NOTEOFF:
    {
      if(nom == MidiInstrument::NoteOffNone) return false;
      clap_event_note_t ev{};
      ev.header.size     = sizeof(ev);
      ev.header.time     = sampleOffset;
      ev.header.space_id = CLAP_CORE_EVENT_SPACE_ID;
      ev.header.flags    = 0;
      ev.note_id         = -1;
      ev.port_index      = 0;
      ev.channel         = chn;
      ev.key             = a;
      if(nom == MidiInstrument::NoteOffConvertToZVNoteOn)
      {
        ev.header.type = CLAP_EVENT_NOTE_ON;
        ev.velocity    = 0.0;
      }
      else
      {
        ev.header.type = CLAP_EVENT_NOTE_OFF;
        ev.velocity    = b / 127.0;
      }
      memcpy(_evInBuf + _evInCount * kClapEventSlotSize, &ev, sizeof(ev));
      ++_evInCount;
    }
    break;

    case ME_PROGRAM:
    {
      int hb, lb;
      synti->currentProg(chn, nullptr, &lb, &hb);
      synti->setCurrentProg(chn, a & 0xff, lb, hb);
      return false;
    }

    case ME_CONTROLLER:
    {
      if(b == CTRL_VAL_UNKNOWN) return false;
      if(a == CTRL_PROGRAM)
      {
        synti->setCurrentProg(chn, b & 0xff, (b>>8) & 0xff, (b>>16) & 0xff);
        return false;
      }
      if(a == CTRL_HBANK || a == CTRL_LBANK) return false;

      // Encode as raw MIDI CC
      if(midiControllerType(a) == MidiController::Controller7)
      {
        clap_event_midi_t ev{};
        ev.header.size     = sizeof(ev);
        ev.header.time     = sampleOffset;
        ev.header.space_id = CLAP_CORE_EVENT_SPACE_ID;
        ev.header.type     = CLAP_EVENT_MIDI;
        ev.header.flags    = 0;
        ev.port_index      = 0;
        ev.data[0] = static_cast<uint8_t>(0xB0 | (chn & 0x0f));
        ev.data[1] = static_cast<uint8_t>(a & 0x7f);
        ev.data[2] = static_cast<uint8_t>(b & 0x7f);
        memcpy(_evInBuf + _evInCount * kClapEventSlotSize, &ev, sizeof(ev));
        ++_evInCount;
        return true;
      }
      return false;
    }

    case ME_PITCHBEND:
    {
      clap_event_midi_t ev{};
      ev.header.size     = sizeof(ev);
      ev.header.time     = sampleOffset;
      ev.header.space_id = CLAP_CORE_EVENT_SPACE_ID;
      ev.header.type     = CLAP_EVENT_MIDI;
      ev.header.flags    = 0;
      ev.port_index      = 0;
      int pb = a + 8192;
      ev.data[0] = static_cast<uint8_t>(0xE0 | (chn & 0x0f));
      ev.data[1] = static_cast<uint8_t>( pb & 0x7f);
      ev.data[2] = static_cast<uint8_t>((pb >> 7) & 0x7f);
      memcpy(_evInBuf + _evInCount * kClapEventSlotSize, &ev, sizeof(ev));
      ++_evInCount;
    }
    break;

    case ME_AFTERTOUCH:
    {
      clap_event_midi_t ev{};
      ev.header.size     = sizeof(ev);
      ev.header.time     = sampleOffset;
      ev.header.space_id = CLAP_CORE_EVENT_SPACE_ID;
      ev.header.type     = CLAP_EVENT_MIDI;
      ev.header.flags    = 0;
      ev.port_index      = 0;
      ev.data[0] = static_cast<uint8_t>(0xD0 | (chn & 0x0f));
      ev.data[1] = static_cast<uint8_t>(a & 0x7f);
      ev.data[2] = 0;
      memcpy(_evInBuf + _evInCount * kClapEventSlotSize, &ev, sizeof(ev));
      ++_evInCount;
    }
    break;

    case ME_POLYAFTER:
    {
      clap_event_midi_t ev{};
      ev.header.size     = sizeof(ev);
      ev.header.time     = sampleOffset;
      ev.header.space_id = CLAP_CORE_EVENT_SPACE_ID;
      ev.header.type     = CLAP_EVENT_MIDI;
      ev.header.flags    = 0;
      ev.port_index      = 0;
      ev.data[0] = static_cast<uint8_t>(0xA0 | (chn & 0x0f));
      ev.data[1] = static_cast<uint8_t>(a & 0x7f);
      ev.data[2] = static_cast<uint8_t>(b & 0x7f);
      memcpy(_evInBuf + _evInCount * kClapEventSlotSize, &ev, sizeof(ev));
      ++_evInCount;
    }
    break;

    case ME_SYSEX:
      #ifdef CLAP_DEBUG
      fprintf(stderr, "ClapSynthIF::processEvent: ME_SYSEX dropped (no sysex in CLAP core)\n");
      #endif
      return false;

    default:
      if(MusEGlobal::debugMsg)
        fprintf(stderr, "ClapSynthIF::processEvent: unknown midi type:%d\n", e.type());
      return false;
  }

  return true;
}

//---------------------------------------------------------
//   handlePluginOutputEvents
//---------------------------------------------------------

void ClapSynthIF::handlePluginOutputEvents(int plug_id)
{
  for(uint32_t i = 0; i < _evOutCount; ++i)
  {
    const clap_event_header_t* h = reinterpret_cast<const clap_event_header_t*>(
      _evOutBuf + i * kClapEventSlotSize);
    if(h->space_id != CLAP_CORE_EVENT_SPACE_ID) continue;

    if(h->type == CLAP_EVENT_PARAM_VALUE)
    {
      const clap_event_param_value_t* pv =
        reinterpret_cast<const clap_event_param_value_t*>(h);
      auto it = _synth->paramIdToIndex.find(pv->param_id);
      if(it == _synth->paramIdToIndex.end()) continue;
      const unsigned long cip = it->second;
      if(cip < _synth->_controlInPorts && _controls)
      {
        _controls[cip].val = static_cast<float>(pv->value);
        if(plug_id != -1)
          synti->setPluginCtrlVal(genACnum(plug_id, cip),
                                  static_cast<double>(pv->value));
      }
    }
  }
}

//---------------------------------------------------------
//   flushParamChanges
//---------------------------------------------------------

void ClapSynthIF::flushParamChanges(unsigned long syncFrame,
                                    unsigned long nframes,
                                    int plug_id)
{
  while(!_controlFifo.isEmpty())
  {
    const ControlEvent& v = _controlFifo.peek();
    unsigned long evframe = (syncFrame > v.frame + nframes)
                            ? 0 : v.frame - syncFrame + nframes;
    if(evframe >= nframes) break;

    if(v.idx >= _synth->_controlInPorts) { _controlFifo.remove(); break; }

    if(_evInCount < _evBufCapacity && _extParams && _controls)
    {
      clap_event_param_value_t ev{};
      ev.header.size     = sizeof(ev);
      ev.header.time     = static_cast<uint32_t>(evframe);
      ev.header.space_id = CLAP_CORE_EVENT_SPACE_ID;
      ev.header.type     = CLAP_EVENT_PARAM_VALUE;
      ev.header.flags    = 0;
      ev.param_id        = _synth->paramIds[v.idx];
      ev.cookie          = _synth->paramInfo[v.idx].cookie;
      ev.note_id         = -1;
      ev.port_index      = -1;
      ev.channel         = -1;
      ev.key             = -1;
      ev.value           = v.value;
      memcpy(_evInBuf + _evInCount * kClapEventSlotSize, &ev, sizeof(ev));
      ++_evInCount;

      _controls[v.idx].val = static_cast<float>(v.value);
      if(plug_id != -1)
        synti->setPluginCtrlVal(genACnum(plug_id, v.idx), v.value);
    }
    _controlFifo.remove();
  }
}

//---------------------------------------------------------
//   sortInEvents
//   Stable insertion sort of the queued input-event slots by header.time.
//   Counts are small (<= _evBufCapacity), and the buffer is nearly sorted
//   already, so insertion sort is cheap and realtime-safe (no allocation).
//---------------------------------------------------------

void ClapSynthIF::sortInEvents()
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
//   getData
//---------------------------------------------------------

bool ClapSynthIF::getData(MidiPort* /*mp*/, unsigned pos, int ports,
                          unsigned nframes, float** buffer)
{
  const unsigned long syncFrame = MusEGlobal::audio->curSyncFrame();

  #ifdef CLAP_DEBUG_PROCESS
  fprintf(stderr, "ClapSynthIF::getData: pos:%u ports:%d nframes:%u\n", pos, ports, nframes);
  #endif

  const unsigned long out_ports = _synth->outPorts();
  const unsigned long in_ports  = _synth->inPorts();
  const unsigned long nop = ((unsigned long)ports > out_ports)
                             ? out_ports : (unsigned long)ports;
  const bool isOn         = on();
  const bool connectDummy = !_curActiveState || !isOn;

  // --- Gather audio inputs ---
  bool used_in_chan_array[in_ports > 0 ? in_ports : 1];
  if(_curActiveState && in_ports > 0)
  {
    for(unsigned long i = 0; i < in_ports; ++i)
      used_in_chan_array[i] = false;

    if(!track()->noInRoute())
    {
      RouteList* irl = track()->inRoutes();
      for(ciRoute i = irl->begin(); i != irl->end(); ++i)
      {
        if(i->track->isMidiTrack()) continue;
        const int dst_ch  = i->channel       <= -1 ? 0 : i->channel;
        const int dst_chs = i->channels      <= -1 ? (int)in_ports : i->channels;
        const int src_ch  = i->remoteChannel <= -1 ? 0 : i->remoteChannel;
        const int src_chs = i->channels;
        if((unsigned long)dst_ch >= in_ports) continue;
        int fin_dst_chs = dst_chs;
        if((unsigned long)(dst_ch + fin_dst_chs) > in_ports)
          fin_dst_chs = (int)in_ports - dst_ch;
        static_cast<AudioTrack*>(i->track)->copyData(
          pos, dst_ch, dst_chs, fin_dst_chs,
          src_ch, src_chs, nframes, &_audioInBuffers[0],
          false, used_in_chan_array);
        for(int ch = dst_ch; ch < dst_ch + fin_dst_chs; ++ch)
          used_in_chan_array[ch] = true;
      }
    }
  }

  // --- Apply automation ---
  AudioTrack* atrack  = track();
  const AutomationType at = atrack->automationType();
  const bool no_auto  = !MusEGlobal::automation || at == AUTO_OFF;
  const unsigned long in_ctrls = _synth->inControls();
  CtrlListList* cll   = atrack->controller();
  const int plug_id   = id();
  ciCtrlList icl_first;
  if(plug_id != -1)
    icl_first = cll->lower_bound(genACnum(plug_id, 0));

  if(_curActiveState && _controls)
  {
    ciCtrlList icl = icl_first;
    for(unsigned long k = 0; k < in_ctrls; ++k)
    {
      CtrlList* cl = (cll && plug_id != -1 && icl != cll->end()) ? icl->second : nullptr;
      if(cl && plug_id != -1 && (unsigned long)cl->id() == genACnum(plug_id, k))
      {
        if(!no_auto && _controls[k].enCtrl)
          _controls[k].val = static_cast<float>(cl->curVal());
        if(icl != cll->end()) ++icl;
      }
    }
  }

  // --- Clear event buffers ---
  _evInCount  = 0;
  _evOutCount = 0;

  // --- Flush scheduled control changes ---
  if(_curActiveState)
    flushParamChanges(syncFrame, nframes, plug_id);

  // --- Convert MIDI events ---
  if(_curActiveState)
  {
    const bool do_stop = synti->stopFlag();
    const bool we      = synti->writeEnable();
    MidiPlayEvent buf_ev;

    if(do_stop || !we)
    {
      const unsigned int sz = synti->eventBuffers(MidiDevice::UserBuffer)->getSize();
      for(unsigned int i = 0; i < sz; ++i)
        if(synti->eventBuffers(MidiDevice::UserBuffer)->get(buf_ev))
          synti->_outUserEvents.addExclusive(buf_ev);
      synti->eventBuffers(MidiDevice::PlaybackBuffer)->clearRead();
      synti->_outPlaybackEvents.clear();
      synti->setStopFlag(false);
    }
    else
    {
      unsigned int sz = synti->eventBuffers(MidiDevice::UserBuffer)->getSize();
      for(unsigned int i = 0; i < sz; ++i)
        if(synti->eventBuffers(MidiDevice::UserBuffer)->get(buf_ev))
          synti->_outUserEvents.insert(buf_ev);
      sz = synti->eventBuffers(MidiDevice::PlaybackBuffer)->getSize();
      for(unsigned int i = 0; i < sz; ++i)
        if(synti->eventBuffers(MidiDevice::PlaybackBuffer)->get(buf_ev))
          synti->_outPlaybackEvents.insert(buf_ev);
    }

    if(we)
    {
      iMPEvent impe_pb = synti->_outPlaybackEvents.begin();
      iMPEvent impe_us = synti->_outUserEvents.begin();
      while(true)
      {
        bool using_pb = false;
        if(impe_pb != synti->_outPlaybackEvents.end() &&
           impe_us != synti->_outUserEvents.end())
          using_pb = (*impe_pb < *impe_us);
        else if(impe_pb != synti->_outPlaybackEvents.end())
          using_pb = true;
        else if(impe_us != synti->_outUserEvents.end())
          using_pb = false;
        else break;

        const MidiPlayEvent& mev = using_pb ? *impe_pb : *impe_us;
        if(mev.time() >= (syncFrame + nframes)) break;

        uint32_t ft = (mev.time() < syncFrame) ? 0 : (uint32_t)(mev.time() - syncFrame);
        if(ft >= nframes) ft = nframes - 1;
        processEvent(mev, ft);

        if(using_pb) impe_pb = synti->_outPlaybackEvents.erase(impe_pb);
        else         impe_us = synti->_outUserEvents.erase(impe_us);
      }
    }
  }

  // --- Run plugin ---
  if(_curActiveState && _plugin && out_ports > 0)
  {
    // Flat channel pointers, in port order (matches how _in/_outPortChans
    // were filled at init).
    float* ins[in_ports > 0 ? in_ports : 1];
    float* outs[out_ports];

    for(unsigned long k = 0; k < in_ports; ++k)
      ins[k] = (!connectDummy && used_in_chan_array[k])
                ? _audioInBuffers[k] : _audioInSilenceBuf;

    for(unsigned long k = 0; k < out_ports; ++k)
      outs[k] = (!connectDummy && k < nop)
                ? buffer[k] : _audioOutBuffers[k];

    // Slice the flat channel arrays into one clap_audio_buffer_t per declared
    // port. CLAP is port-based: audio_*_count must equal the plugin's port
    // count, each buffer carrying that port's channels. Collapsing everything
    // into a single buffer makes multi-port plugins (e.g. Surge XT) reject the
    // process call with CLAP_PROCESS_ERROR.
    const size_t nInPorts  = _inPortChans.size();
    const size_t nOutPorts = _outPortChans.size();

    clap_audio_buffer_t inBufs[nInPorts > 0 ? nInPorts : 1];
    clap_audio_buffer_t outBufs[nOutPorts > 0 ? nOutPorts : 1];

    unsigned long off = 0;
    for(size_t p = 0; p < nInPorts; ++p)
    {
      inBufs[p] = clap_audio_buffer_t{};
      inBufs[p].data32        = ins + off;
      inBufs[p].channel_count = _inPortChans[p];
      off += _inPortChans[p];
    }

    off = 0;
    for(size_t p = 0; p < nOutPorts; ++p)
    {
      outBufs[p] = clap_audio_buffer_t{};
      outBufs[p].data32        = outs + off;
      outBufs[p].channel_count = _outPortChans[p];
      off += _outPortChans[p];
    }

    // CLAP requires the input event stream sorted ascending by time.
    sortInEvents();

    clap_process_t proc{};
    proc.steady_time         = static_cast<int64_t>(pos);
    proc.frames_count        = nframes;
    proc.transport           = nullptr; // TODO: fill transport info
    proc.audio_inputs        = (nInPorts > 0) ? inBufs : nullptr;
    proc.audio_inputs_count  = static_cast<uint32_t>(nInPorts);
    proc.audio_outputs       = outBufs;
    proc.audio_outputs_count = static_cast<uint32_t>(nOutPorts);
    proc.in_events           = &_clapEvIn;
    proc.out_events          = &_clapEvOut;

    const clap_process_status status = _plugin->process(_plugin, &proc);
    if(status == CLAP_PROCESS_ERROR)
      fprintf(stderr, "ClapSynthIF::getData: plugin->process() returned error\n");

    handlePluginOutputEvents(plug_id);
  }
  else if(!_curActiveState || !isOn)
  {
    for(unsigned long k = 0; k < nop; ++k)
      memset(buffer[k], 0, sizeof(float) * nframes);
  }

  return true;
}

//---------------------------------------------------------
//   getPatchName / populatePatchPopup
//---------------------------------------------------------

QString ClapSynthIF::getPatchName(int /*chan*/, int /*prog*/, bool /*drum*/) const
{
  return QString("?"); // TODO: CLAP_EXT_PRESET_LOAD
}

void ClapSynthIF::populatePatchPopup(MusEGui::PopupMenu* menu, int /*ch*/, bool /*drum*/)
{
  menu->clear(); // TODO: CLAP_EXT_PRESET_LOAD
}

//---------------------------------------------------------
//   getControllerInfo
//---------------------------------------------------------

int ClapSynthIF::getControllerInfo(int idx, QString* name,
                                   int* ctrl, int* min, int* max, int* initval)
{
  if(!_synth || !_controls) return 0;
  const int controlPorts = (int)_synth->_controlInPorts;

  if(idx == controlPorts)
  {
    *ctrl = CTRL_POLYAFTER; *min = 0; *max = 127;
    *initval = CTRL_VAL_UNKNOWN; *name = midiCtrlName(*ctrl);
    return ++idx;
  }
  if(idx == controlPorts + 1)
  {
    *ctrl = CTRL_AFTERTOUCH; *min = 0; *max = 127;
    *initval = CTRL_VAL_UNKNOWN; *name = midiCtrlName(*ctrl);
    return ++idx;
  }
  if(idx >= controlPorts + 2) return 0;

  const clap_param_info_t& pi = _synth->paramInfo[idx];
  *name    = QString::fromUtf8(pi.name);
  *min     = (int)(pi.min_value);
  *max     = (int)(pi.max_value);
  *initval = (int)(pi.default_value);
  *ctrl    = CTRL_NRPN14_OFFSET + 0x2000 + idx;
  return ++idx;
}

//---------------------------------------------------------
//   Channel counts
//---------------------------------------------------------

int ClapSynthIF::channels() const
{
  if(!_synth) return 0;
  return ((int)_synth->_outports) > MusECore::MAX_CHANNELS
           ? MusECore::MAX_CHANNELS : (int)_synth->_outports;
}

int ClapSynthIF::totalOutChannels() const { return _synth ? (int)_synth->_outports : 0; }
int ClapSynthIF::totalInChannels()  const { return _synth ? (int)_synth->_inports  : 0; }

//---------------------------------------------------------
//   PluginIBase
//---------------------------------------------------------

unsigned long ClapSynthIF::pluginID() const
{
  if(!_synth || !_synth->_desc) return 0;
  return qHash(QString(_synth->_desc->id));
}

void ClapSynthIF::enableController(unsigned long i, bool v)
{
  if(_controls && _synth && i < _synth->_controlInPorts)
    _controls[i].enCtrl = v;
}

bool ClapSynthIF::controllerEnabled(unsigned long i) const
{
  return (_controls && _synth && i < _synth->_controlInPorts)
          ? _controls[i].enCtrl : true;
}

void ClapSynthIF::enableAllControllers(bool v)
{
  if(!_synth || !_controls) return;
  for(unsigned long i = 0; i < _synth->_controlInPorts; ++i)
    _controls[i].enCtrl = v;
}

unsigned long ClapSynthIF::parameters()    const { return _synth ? _synth->_controlInPorts  : 0; }
unsigned long ClapSynthIF::parametersOut() const { return _synth ? _synth->_controlOutPorts : 0; }
void   ClapSynthIF::setParam(unsigned long i, double v) { setParameter(i, v); }
double ClapSynthIF::param(unsigned long i)    const { return getParameter(i); }
double ClapSynthIF::paramOut(unsigned long /*i*/) const { return 0.0; }

const char* ClapSynthIF::paramName(unsigned long i) const
{
  if(!_synth || i >= _synth->_controlInPorts) return nullptr;
  return _synth->paramInfo[i].name;
}

const char* ClapSynthIF::paramOutName(unsigned long /*i*/) const { return nullptr; }

LADSPA_PortRangeHint ClapSynthIF::range(unsigned long i) const
{
  if(!_synth || i >= _synth->_controlInPorts)
    return LADSPA_PortRangeHint{0, 0.0f, 0.0f};
  const clap_param_info_t& pi = _synth->paramInfo[i];
  LADSPA_PortRangeHint h;
  h.HintDescriptor = LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE;
  h.LowerBound     = static_cast<float>(pi.min_value);
  h.UpperBound     = static_cast<float>(pi.max_value);
  return h;
}

LADSPA_PortRangeHint ClapSynthIF::rangeOut(unsigned long /*i*/) const
{
  return LADSPA_PortRangeHint{0, 0.0f, 0.0f};
}

void ClapSynthIF::range(unsigned long i, float* mn, float* mx) const
{
  if(!_synth || i >= _synth->_controlInPorts) { *mn = *mx = 0.0f; return; }
  *mn = static_cast<float>(_synth->paramInfo[i].min_value);
  *mx = static_cast<float>(_synth->paramInfo[i].max_value);
}

void ClapSynthIF::rangeOut(unsigned long /*i*/, float* mn, float* mx) const
{ *mn = *mx = 0.0f; }

CtrlValueType ClapSynthIF::ctrlValueType(unsigned long i) const
{
  if(!_synth || i >= _synth->_controlInPorts) return VAL_LINEAR;
  return (_synth->paramInfo[i].flags & CLAP_PARAM_IS_STEPPED) ? VAL_INT : VAL_LINEAR;
}

CtrlList::Mode ClapSynthIF::ctrlMode(unsigned long /*i*/) const
{ return CtrlList::INTERPOLATE; }

CtrlValueType  ClapSynthIF::ctrlOutValueType(unsigned long /*i*/) const { return VAL_LINEAR; }
CtrlList::Mode ClapSynthIF::ctrlOutMode(unsigned long /*i*/)      const { return CtrlList::INTERPOLATE; }

} // namespace MusECore

#endif // CLAP_SUPPORT
