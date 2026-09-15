//=============================================================================
//  MusE
//  Linux Music Editor
//
//  clap_host_synth.cpp
//  CLAP (CLever Audio Plugin) host integration for MusE — synth-track side.
//  See clap_host_synth.h for the split rationale; ClapInstanceCore (clap_host_lib.h)
//  does the actual CLAP work, this file only translates to/from MusE's
//  SynthIF/track world.
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

#include <cstring>
#include <cstdlib>
#include <cstdio>

#include <QDir>
#include <QFileInfo>

#include <clap/clap.h>
#include <clap/factory/plugin-factory.h>

#include "clap_host_synth.h"
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
//   initCLAP
//   Registers instrument-class CLAP plugins into MusEGlobal::synthis.
//   Effect-class plugins are registered separately by initCLAPEffects()
//   (clap_host_effect.cpp) — see the header comment for the rationale.
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
    // Only instrument-class plugins go into synthis. A plugin marked as both
    // instrument and effect also gets registered by initCLAPEffects() into
    // MusEGlobal::plugins, independently.
    if(!(info._class & MusEPlugin::PluginClassInstrument))
      continue;

    const QString inf_uri   = PLUGIN_GET_QSTRING(info._uri);
    const QString inf_label = PLUGIN_GET_QSTRING(info._label);

    if(const Synth* sy = MusEGlobal::synthis.find(
         info._type,
         PLUGIN_GET_QSTRING(info._completeBaseName),
         inf_uri, inf_label))
    {
      if(MusEGlobal::debugMsg && !MusEGlobal::suppressPluginDuplicateWarnings)
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

    // Reset port counts — will be filled properly in ClapInstanceCore::init()
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

  // GUI teardown, timers/fds, deactivate+destroy, event buffers: all owned by
  // ClapInstanceCore. Explicit call (rather than relying on _core's own
  // destructor running after this body) keeps teardown order obvious.
  _core.shutdown();
  
  freeAudioBuffers();
  
  if(_controls)    { delete[] _controls;    _controls    = nullptr; }
  if(_controlsOut) { delete[] _controlsOut; _controlsOut = nullptr; }
}






void ClapSynthIF::freeAudioBuffers()
{
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
  _audioBufFrames = 0;
}

bool ClapSynthIF::allocAudioBuffers(unsigned long frames)
{
  if(!_synth || frames == 0)
  {
    fprintf(stderr, "ClapSynthIF::allocAudioBuffers: no synth or frames==0\n");
    return false;
  }

  const unsigned long inports  = _synth->_inports;
  const unsigned long outports = _synth->_outports;

  if(inports != 0)
  {
    int rv = posix_memalign((void**)&_audioInSilenceBuf, 16, sizeof(float) * frames);
    if(rv != 0) { fprintf(stderr, "ClapSynthIF::allocAudioBuffers: posix_memalign error:%d\n", rv); abort(); }
    if(MusEGlobal::config.useDenormalBias)
      for(unsigned long q = 0; q < frames; ++q)
        _audioInSilenceBuf[q] = MusEGlobal::denormalBias;
    else
      memset(_audioInSilenceBuf, 0, sizeof(float) * frames);

    _audioInBuffers = new float*[inports];
    for(unsigned long k = 0; k < inports; ++k)
    {
      rv = posix_memalign((void**)&_audioInBuffers[k], 16, sizeof(float) * frames);
      if(rv != 0) { fprintf(stderr, "ClapSynthIF::allocAudioBuffers: posix_memalign error:%d\n", rv); abort(); }
      if(MusEGlobal::config.useDenormalBias)
        for(unsigned long q = 0; q < frames; ++q)
          _audioInBuffers[k][q] = MusEGlobal::denormalBias;
      else
        memset(_audioInBuffers[k], 0, sizeof(float) * frames);
    }
  }

  if(outports != 0)
  {
    _audioOutBuffers = new float*[outports];
    for(unsigned long k = 0; k < outports; ++k)
    {
      int rv = posix_memalign((void**)&_audioOutBuffers[k], 16, sizeof(float) * frames);
      if(rv != 0) { fprintf(stderr, "ClapSynthIF::allocAudioBuffers: posix_memalign error:%d\n", rv); abort(); }
      if(MusEGlobal::config.useDenormalBias)
        for(unsigned long q = 0; q < frames; ++q)
          _audioOutBuffers[k][q] = MusEGlobal::denormalBias;
      else
        memset(_audioOutBuffers[k], 0, sizeof(float) * frames);
    }
  }

  _audioBufFrames = frames;
  return true;
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

  if(!_core.init(s, s->name()))
    return false;

  // Owner-side reaction when the plugin/window-manager closes the GUI
  // without destroying it — see ClapInstanceCore::setGuiClosedCallback().
  _core.setGuiClosedCallback([this]{ showNativeGuiPending(false); });

  const unsigned long controlPorts = _core.paramCount();

  // Sized to the current block length; getData() regrows this if a later
  // callback's nframes ever exceeds it — PipeWire's JACK bridge can change
  // the block size at runtime (adaptive quantum), and a fixed one-time
  // allocation here silently overflowed on the audio thread whenever a
  // later callback's nframes exceeded it. See allocAudioBuffers().
  if(!allocAudioBuffers(MusEGlobal::segmentSize))
  {
    fprintf(stderr, "ClapSynthIF::init: allocAudioBuffers(%u) failed for '%s'\n",
            MusEGlobal::segmentSize, s->name().toLocal8Bit().constData());
    return false;
  }

  // --- Allocate parameter value arrays ---
  if(controlPorts != 0)
    _controls = new Port[controlPorts];

  // --- Initialise control values and hook up MusE automation ---
  if(_controls)
  {
    for(unsigned long cip = 0; cip < controlPorts; ++cip)
    {
      const clap_param_info_t& pi = _synth->paramInfo[cip];
      _controls[cip].idx    = cip;
      _controls[cip].val    = static_cast<float>(pi.default_value);
      _controls[cip].enCtrl = !(pi.flags & CLAP_PARAM_IS_READONLY);

      _controls[cip].val = static_cast<float>(_core.getParameter(pi.id));

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

  _core.activate();
  if(_core.isActive())
    SynthIF::activate();

  return true;
}


//---------------------------------------------------------
//   activate / deactivate
//---------------------------------------------------------

void ClapSynthIF::activate()
{
  _core.activate();
  if(_core.isActive())
    SynthIF::activate();
}

void ClapSynthIF::deactivate()
{
  _core.deactivate();
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
//   write / read — superseded by getCustomData()/setCustomData()
//---------------------------------------------------------

void ClapSynthIF::write(int /*level*/, Xml& /*xml*/) const   // empty, unused !!!
{
  // State now persists via getCustomData()/setCustomData() (the <customData>
  // framework), so this per-SIF write() emits nothing.
}

void ClapSynthIF::read( Xml& /*xml*/ ) // empty, unused !!!
{
  // superseded by getCustomData()/setCustomData()
}

//---------------------------------------------------------
//   getCustomData / setCustomData
//---------------------------------------------------------

std::vector<QString> ClapSynthIF::getCustomData() const
{
  return _core.getCustomData();
}

bool ClapSynthIF::setCustomData(const std::vector<QString>& d)
{
  const bool ok = _core.setCustomData(d);

  // Resync our _controls[] cache from the plugin after state load.
  if(ok && _controls && _synth)
  {
    for(unsigned long i = 0; i < _synth->_controlInPorts; ++i)
      _controls[i].val = static_cast<float>(_core.getParameter(_synth->paramInfo[i].id));
  }
  return ok;
}

//---------------------------------------------------------
//   processEvent
//   Translates a MusE MidiPlayEvent into a CLAP note/midi event queued on
//   the core for the next runProcess() call.
//---------------------------------------------------------

bool ClapSynthIF::processEvent(const MidiPlayEvent& e, uint32_t sampleOffset)
{
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
      int16_t evtype = CLAP_EVENT_NOTE_ON;
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
      return _core.pushNoteEvent(evtype, -1, 0, chn, a, velocity / 127.0, sampleOffset);
    }

    case ME_NOTEOFF:
    {
      if(nom == MidiInstrument::NoteOffNone) return false;
      int16_t evtype;
      double  velocity;
      if(nom == MidiInstrument::NoteOffConvertToZVNoteOn)
      {
        evtype   = CLAP_EVENT_NOTE_ON;
        velocity = 0.0;
      }
      else
      {
        evtype   = CLAP_EVENT_NOTE_OFF;
        velocity = b / 127.0;
      }
      return _core.pushNoteEvent(evtype, -1, 0, chn, a, velocity, sampleOffset);
    }

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
        const uint8_t data[3] = {
          static_cast<uint8_t>(0xB0 | (chn & 0x0f)),
          static_cast<uint8_t>(a & 0x7f),
          static_cast<uint8_t>(b & 0x7f)
        };
        return _core.pushMidiEvent(data, 0, sampleOffset);
      }
      return false;
    }

    case ME_PITCHBEND:
    {
      const int pb = a + 8192;
      const uint8_t data[3] = {
        static_cast<uint8_t>(0xE0 | (chn & 0x0f)),
        static_cast<uint8_t>( pb & 0x7f),
        static_cast<uint8_t>((pb >> 7) & 0x7f)
      };
      return _core.pushMidiEvent(data, 0, sampleOffset);
    }

    case ME_AFTERTOUCH:
    {
      const uint8_t data[3] = {
        static_cast<uint8_t>(0xD0 | (chn & 0x0f)),
        static_cast<uint8_t>(a & 0x7f),
        0
      };
      return _core.pushMidiEvent(data, 0, sampleOffset);
    }

    case ME_POLYAFTER:
    {
      const uint8_t data[3] = {
        static_cast<uint8_t>(0xA0 | (chn & 0x0f)),
        static_cast<uint8_t>(a & 0x7f),
        static_cast<uint8_t>(b & 0x7f)
      };
      return _core.pushMidiEvent(data, 0, sampleOffset);
    }

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
}

//---------------------------------------------------------
//   handlePluginOutputEvents
//---------------------------------------------------------

void ClapSynthIF::handlePluginOutputEvents(int plug_id)
{
  const uint32_t n = _core.outputEventCount();
  for(uint32_t i = 0; i < n; ++i)
  {
    const clap_event_header_t* h = _core.outputEvent(i);
    if(!h || h->space_id != CLAP_CORE_EVENT_SPACE_ID) continue;

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

    if(_controls &&
       _core.pushParamValueEvent(_synth->paramIds[v.idx], v.value,
                                 static_cast<uint32_t>(evframe)))
    {
      _controls[v.idx].val = static_cast<float>(v.value);
      if(plug_id != -1)
        synti->setPluginCtrlVal(genACnum(plug_id, v.idx), v.value);
    }
    _controlFifo.remove();
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


  // Defensive regrow: PipeWire's JACK bridge can change the block size
  // (adaptive quantum) between callbacks. Buffers are sized once in
  // init(); without this check a later nframes exceeding that silently
  // overflows _audioOutBuffers[k]/_audioInBuffers[k] on the audio thread —
  // no crash on the spot, just heap corruption that surfaces a second or
  // two later as noise, slowdown, then a hard crash.
  if(nframes > _audioBufFrames)
  {
    fprintf(stderr,
      "ClapSynthIF::getData: block size grew (%lu -> %u frames) for '%s' — "
      "reallocating CLAP audio buffers\n",
      _audioBufFrames, nframes,
      _synth ? _synth->name().toLocal8Bit().constData() : "?");
    freeAudioBuffers();
    if(!allocAudioBuffers(nframes))
    {
      fprintf(stderr, "ClapSynthIF::getData: allocAudioBuffers(%u) failed for '%s'\n",
              nframes, _synth ? _synth->name().toLocal8Bit().constData() : "?");
      return false;
    }
  }

  const unsigned long out_ports = _core.outPorts();
  const unsigned long in_ports  = _core.inPorts();
  const unsigned long nop = ((unsigned long)ports > out_ports)
                             ? out_ports : (unsigned long)ports;
  const bool isOn         = on();
  const bool isActive     = _core.isActive();
  const bool connectDummy = !isActive || !isOn;

  // --- Gather audio inputs ---
  bool used_in_chan_array[in_ports > 0 ? in_ports : 1];
  if(isActive && in_ports > 0)
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

  if(isActive && _controls)
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

  // --- Flush scheduled control changes ---
  if(isActive)
    flushParamChanges(syncFrame, nframes, plug_id);

  // --- Convert MIDI events ---
  if(isActive)
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
  if(isActive && out_ports > 0)
  {
    // Flat channel pointers, in port order — ClapInstanceCore::runProcess()
    // slices these into per-port clap_audio_buffer_t internally.
    float* ins[in_ports > 0 ? in_ports : 1];
    float* outs[out_ports];

    for(unsigned long k = 0; k < in_ports; ++k)
      ins[k] = (!connectDummy && used_in_chan_array[k])
                ? _audioInBuffers[k] : _audioInSilenceBuf;

    for(unsigned long k = 0; k < out_ports; ++k)
      outs[k] = (!connectDummy && k < nop)
                ? buffer[k] : _audioOutBuffers[k];

    _core.runProcess(static_cast<int64_t>(pos), nframes,
                     ins, static_cast<uint32_t>(in_ports),
                     outs, static_cast<uint32_t>(out_ports));

    handlePluginOutputEvents(plug_id);
  }
  else if(!isActive || !isOn)
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
  const int outports = (int)_core.outPorts();
  return outports > MusECore::MAX_CHANNELS ? MusECore::MAX_CHANNELS : outports;
}

int ClapSynthIF::totalOutChannels() const { return (int)_core.outPorts(); }
int ClapSynthIF::totalInChannels()  const { return (int)_core.inPorts();  }

//---------------------------------------------------------
//   GUI
//---------------------------------------------------------

void ClapSynthIF::showNativeGui(bool v)
{
  PluginIBase::showNativeGui(v);
  _core.showNativeGui(v);
}

void ClapSynthIF::closeNativeGui()
{
  _core.closeNativeGui();
}

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
