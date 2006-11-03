//=============================================================================
//  MusE
//  Linux Music Editor
//  $Id:$
//
//  Copyright (C) 2002-2006 by Werner Schweer and others
//   This code is based on jack_fst:
//   Copyright (C) 2004 Paul Davis <paul@linuxaudiosystems.com>
//                      Torben Hohn <torbenh@informatik.uni-bremen.de>
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License version 2.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//=============================================================================

#include "config.h"

#ifdef VST_SUPPORT

#include "al/al.h"
#include <fst.h>
#include <vst/aeffectx.h>
#include <jack/jack.h>

#include "vst.h"
#include "globals.h"
#include "synth.h"
#include "driver/jackaudio.h"
#include "midi.h"
#include "al/xml.h"
#include "song.h"
#include "audio.h"
#include "widgets/utils.h"

extern "C" void fst_error(const char *fmt, ...);
extern JackAudio* jackAudio;

//---------------------------------------------------------
//   jfstReserveMem
//---------------------------------------------------------

static void jfstReserveMem(int size)
      {
      char buf[size];

      for (int i = 0; i < size; ++i)
            buf[i] = (char)(i % 256);
      }

//---------------------------------------------------------
//   vstHostCallback
//---------------------------------------------------------

static long vstHostCallback(AEffect* effect,
   long opcode, long index, long value, void* ptr, float opt)
      {
      static VstTimeInfo _timeInfo;

      VstPluginIF* pluginIF = effect ? (VstPluginIF*) effect->user : 0;

      jack_position_t jack_pos;
      jack_transport_state_t tstate;

      switch (opcode) {
            case audioMasterAutomate:
                  // index, value, returns 0
                  if (pluginIF) {
                        PluginI* pi = pluginIF->pluginInstance();
                        // pi->setParam(index, opt);
                        CVal cval;
                        cval.f = opt;
                        song->setControllerVal(pi->track(), pi->controller(index), cval);
                        }
                  return 0;

            case audioMasterVersion:
                  // vst version, currently 2 (0 for older)
                  return 2200;

            case audioMasterCurrentId:
                  // returns the unique id of a plug that's currently
                  // loading
                  return 0;

            case audioMasterIdle:
                  // call application idle routine (this will
                  // call effEditIdle for all open editors too)
                  effect->dispatcher(effect, effEditIdle, 0, 0, NULL, 0.0f);
                  return 0;

            case audioMasterPinConnected:
                  // inquire if an input or output is beeing connected;
                  // index enumerates input or output counting from zero:
                  // value is 0 for input and != 0 otherwise. note: the
                  // return value is 0 for <true> such that older versions
                  // will always return true.
                  return 1;

            case audioMasterWantMidi:
                  // <value> is a filter which is currently ignored
                  return 0;

            case audioMasterGetTime:
                  // returns const VstTimeInfo* (or 0 if not supported)
                  // <value> should contain a mask indicating which fields are required
                  // (see valid masks above), as some items may require extensive
                  // conversions

                  memset(&_timeInfo, 0, sizeof(_timeInfo));

                  if (effect) {
                        tstate = jackAudio->transportQuery(&jack_pos);

                        _timeInfo.samplePos  = jack_pos.frame;
                        _timeInfo.sampleRate = jack_pos.frame_rate;
                        _timeInfo.flags = 0;

                        if ((value & (kVstBarsValid|kVstTempoValid)) && (jack_pos.valid & JackPositionBBT)) {
                              _timeInfo.tempo = jack_pos.beats_per_minute;
                              _timeInfo.timeSigNumerator = (long) floor (jack_pos.beats_per_bar);
                              _timeInfo.timeSigDenominator = (long) floor (jack_pos.beat_type);
                              _timeInfo.flags |= (kVstBarsValid|kVstTempoValid);
                              }
                        if (tstate == JackTransportRolling) {
                              _timeInfo.flags |= kVstTransportPlaying;
                              }
                        }
                  else {
                        _timeInfo.samplePos  = 0;
                        _timeInfo.sampleRate = AL::sampleRate;
                        }
                  return (long)&_timeInfo;

            case audioMasterProcessEvents:
                  // VstEvents* in <ptr>
                  return 0;

            case audioMasterSetTime:
                  // VstTimenfo* in <ptr>, filter in <value>, not supported

            case audioMasterTempoAt:
                  // returns tempo (in bpm * 10000) at sample frame location passed in <value>
                  return 0;

            case audioMasterGetNumAutomatableParameters:
                  return 0;

            case audioMasterGetParameterQuantization:
                     // returns the integer value for +1.0 representation,
                   // or 1 if full single float precision is maintained
                     // in automation. parameter index in <value> (-1: all, any)
                  return 0;

            case audioMasterIOChanged:
                   // numInputs and/or numOutputs has changed
                  return 0;

            case audioMasterNeedIdle:
                   // plug needs idle calls (outside its editor window)
                  return 0;

            case audioMasterSizeWindow:
                  // index: width, value: height
                  return 0;

            case audioMasterGetSampleRate:
                  return 0;

            case audioMasterGetBlockSize:
                  return 0;

            case audioMasterGetInputLatency:
                  return 0;

            case audioMasterGetOutputLatency:
                  return 0;

            case audioMasterGetPreviousPlug:
                   // input pin in <value> (-1: first to come), returns cEffect*
                  return 0;

            case audioMasterGetNextPlug:
                   // output pin in <value> (-1: first to come), returns cEffect*

            case audioMasterWillReplaceOrAccumulate:
                   // returns: 0: not supported, 1: replace, 2: accumulate
                  return 0;

            case audioMasterGetCurrentProcessLevel:
                  // returns: 0: not supported,
                  // 1: currently in user thread (gui)
                  // 2: currently in audio thread (where process is called)
                  // 3: currently in 'sequencer' thread (midi, timer etc)
                  // 4: currently offline processing and thus in user thread
                  // other: not defined, but probably pre-empting user thread.
                  return 0;

            case audioMasterGetAutomationState:
                  // returns 0: not supported, 1: off, 2:read, 3:write, 4:read/write
                  // offline
                  return 0;

            case audioMasterOfflineStart:
            case audioMasterOfflineRead:
                   // ptr points to offline structure, see below. return 0: error, 1 ok
                  return 0;

            case audioMasterOfflineWrite:
                  // same as read
                  return 0;

            case audioMasterOfflineGetCurrentPass:
            case audioMasterOfflineGetCurrentMetaPass:
                  return 0;

            case audioMasterSetOutputSampleRate:
                  // for variable i/o, sample rate in <opt>
                  return 0;

            case audioMasterGetSpeakerArrangement:
                  // (long)input in <value>, output in <ptr>
                  return 0;

            case audioMasterGetVendorString:
                  // fills <ptr> with a string identifying the vendor (max 64 char)
                  strcpy ((char*) ptr, "LAD");
                  return 0;

            case audioMasterGetProductString:
                  // fills <ptr> with a string with product name (max 64 char)
                  strcpy ((char*) ptr, "FreeST");

            case audioMasterGetVendorVersion:
                  // returns vendor-specific version
                  return 1000;

            case audioMasterVendorSpecific:
                  // no definition, vendor specific handling
                  return 0;

            case audioMasterSetIcon:
                  // void* in <ptr>, format not defined yet
                  return 0;

            case audioMasterCanDo:
                  // string in ptr, see below
                  return 0;

            case audioMasterGetLanguage:
                  // see enum
                  return 0;

            case audioMasterOpenWindow:
                  // returns platform specific ptr
                  return 0;

            case audioMasterCloseWindow:
                  // close window, platform specific handle in <ptr>
                  return 0;

            case audioMasterGetDirectory:
                  // get plug directory, FSSpec on MAC, else char*
                  return 0;

            case audioMasterUpdateDisplay:
                  // something has changed, update 'multi-fx' display
                  effect->dispatcher(effect, effEditIdle, 0, 0, NULL, 0.0f);
                  return 0;

            case audioMasterBeginEdit:
                  // begin of automation session (when mouse down), parameter index in <index>
                  return 0;

            case audioMasterEndEdit:
                  // end of automation session (when mouse up),     parameter index in <index>
                  return 0;

            case audioMasterOpenFileSelector:
                  // open a fileselector window with VstFileSelect* in <ptr>
                  return 0;

            default:
                  break;
            }
      return 0;
      }


//---------------------------------------------------------
//   FstPlugin
//---------------------------------------------------------

FstPlugin::FstPlugin()
      {
      _fst = 0;
      }

//---------------------------------------------------------
//   ~FstPlugin
//---------------------------------------------------------

FstPlugin::~FstPlugin()
      {
      if (_fst) {
            destroyEditor();
            fst_close(_fst);
            _fst = 0;
            }
      }

//---------------------------------------------------------
//   instantiate
//---------------------------------------------------------

void FstPlugin::instantiate(FSTHandle* fstHandle, void* p)
      {
      _fst = fst_instantiate(fstHandle, vstHostCallback, p);
      if (_fst == 0) {
            printf("FstPlugin::instantiate:: cannot instantiate plugin\n");
            return;
            }
      AEffect* plugin = _fst->plugin;
      version = plugin->dispatcher (plugin, effGetVstVersion, 0, 0, 0, 0.0f);
      }

//---------------------------------------------------------
//   numInputs
//---------------------------------------------------------

int FstPlugin::numInputs() const
      {
      return _fst->plugin->numInputs;
      }

//---------------------------------------------------------
//   numOutputs
//---------------------------------------------------------

int FstPlugin::numOutputs() const
      {
      return _fst->plugin->numOutputs;
      }

//---------------------------------------------------------
//   numParameter
//---------------------------------------------------------

int FstPlugin::numParameter() const
      {
      return _fst->plugin->numParams;
      }

//---------------------------------------------------------
//   setSampleRate
//---------------------------------------------------------

void FstPlugin::setSampleRate(float sr)
      {
      AEffect* plugin = _fst->plugin;
      plugin->dispatcher(plugin, effSetSampleRate, 0, 0, 0, sr);
      }

//---------------------------------------------------------
//   setBlockSize
//---------------------------------------------------------

void FstPlugin::setBlockSize(int bs)
      {
      AEffect* plugin = _fst->plugin;
      plugin->dispatcher(plugin, effSetBlockSize,  0, bs, 0, 0.0f);
      }

//---------------------------------------------------------
//   mainsChanged
//---------------------------------------------------------

void FstPlugin::mainsChanged(bool on)
      {
      AEffect* plugin = _fst->plugin;
      plugin->dispatcher(plugin, effMainsChanged, 0, on, 0, 0.0f);
      }

//---------------------------------------------------------
//   setProgram
//---------------------------------------------------------

void FstPlugin::setProgram(int p)
      {
      AEffect* plugin = _fst->plugin;
      plugin->dispatcher(plugin, effSetProgram, 0, p, 0, 0.0f);
      }

//---------------------------------------------------------
//   hasGui
//---------------------------------------------------------

bool FstPlugin::hasGui() const
      {
      return _fst->plugin->flags & effFlagsHasEditor;
      }

//---------------------------------------------------------
//   canReplacing
//---------------------------------------------------------

bool FstPlugin::canReplacing()
      {
      return _fst->plugin->flags & effFlagsCanReplacing;
      }

//---------------------------------------------------------
//   runEditor
//---------------------------------------------------------

bool FstPlugin::runEditor()
      {
      return fst_run_editor(_fst);
      }

//---------------------------------------------------------
//   destroyEditor
//---------------------------------------------------------

void FstPlugin::destroyEditor()
      {
      fst_destroy_editor(_fst);
      }

//---------------------------------------------------------
//   getVstVersion
//---------------------------------------------------------

int FstPlugin::getVstVersion()
      {
      return version;
      }

//---------------------------------------------------------
//   setParameter
//---------------------------------------------------------

void FstPlugin::setParameter(int idx, float value)
      {
      AEffect* plugin = _fst->plugin;
      plugin->setParameter(plugin, idx, value);
      }

//---------------------------------------------------------
//   getParameter
//---------------------------------------------------------

float FstPlugin::getParameter(int idx)
      {
      AEffect* plugin = _fst->plugin;
      return plugin->getParameter(plugin, idx);
      }

//---------------------------------------------------------
//   processReplacing
//---------------------------------------------------------

void FstPlugin::processReplacing(float** ins, float** outs, int n)
      {
      AEffect* plugin = _fst->plugin;
      plugin->processReplacing(plugin, ins, outs, n);
      }

//---------------------------------------------------------
//   process
//---------------------------------------------------------

void FstPlugin::process(float** ins, float** outs, int n)
      {
      AEffect* plugin = _fst->plugin;
      plugin->process(plugin, ins, outs, n);
      }

//---------------------------------------------------------
//   putEvent
//---------------------------------------------------------

void FstPlugin::putEvent(const MidiEvent& ev)
      {
      AEffect* plugin = _fst->plugin;
      static struct VstEvents events;
      static struct VstMidiEvent event;
      events.numEvents = 1;
      events.reserved  = 0;
      events.events[0] = (VstEvent*)(&event);

      event.type         = kVstMidiType;
      event.byteSize     = 24;
      event.deltaFrames  = 0;
      event.flags        = 0;
      event.detune       = 0;
      event.noteLength   = 0;
      event.noteOffset   = 0;
      event.reserved1    = 0;
      event.reserved2    = 0;
      event.noteOffVelocity = 0;
      switch (ev.type()) {
            case ME_PITCHBEND:
                  {
                  int a = ev.dataA() + 8192;
                  int b = a >> 7;
                  event.midiData[0]  = (ev.type() | ev.channel()) & 0xff;
                  event.midiData[1]  = a & 0x7f;
                  event.midiData[2]  = b & 0x7f;
                  event.midiData[3]  = 0;
                  }
                  break;

            case ME_CONTROLLER:
            case ME_NOTEON:
            default:
                  event.midiData[0]  = (ev.type() | ev.channel()) & 0xff;
                  event.midiData[1]  = ev.dataA() & 0xff;
                  event.midiData[2]  = ev.dataB() & 0xff;
                  event.midiData[3]  = 0;
                  break;
            }
      plugin->dispatcher(plugin, effProcessEvents, 0, 0, &events, 0.0f);
      }

//---------------------------------------------------------
//   getParameterName
//---------------------------------------------------------

const char* FstPlugin::getParameterName(int idx) const
      {
      static char dsp[8];
      dsp[0] = 0;
      AEffect* effect = _fst->plugin;
      effect->dispatcher(effect, effGetParamName, idx, 0, &dsp, 0.0f);
      return dsp;
      }

//---------------------------------------------------------
//   getParameterLabel
//---------------------------------------------------------

const char* FstPlugin::getParameterLabel(int idx) const
      {
      static char dsp[8];
      dsp[0] = 0;
      AEffect* effect = _fst->plugin;
      effect->dispatcher(effect, effGetParamLabel, idx, 0, &dsp, 0.0f);
      return dsp;
      }

//---------------------------------------------------------
//   getParameterDisplay
//---------------------------------------------------------

const char* FstPlugin::getParameterDisplay(int idx, float val) const
      {
      static char dsp[8];
      dsp[0] = 0;
      AEffect* effect = _fst->plugin;
      effect->dispatcher(effect, effGetParamDisplay, idx, 0, &dsp, val);
      return dsp;
      }

//---------------------------------------------------------
//   scanVstDir
//---------------------------------------------------------

static void scanVstDir(const QString& s)
      {
      if (debugMsg)
            printf("scan vst plugin dir <%s>\n", s.toLatin1().data());
      QDir pluginDir(s, QString("*.dll"), QDir::Unsorted, QDir::Files);
      if (pluginDir.exists()) {
            const QFileInfoList list = pluginDir.entryInfoList();
            for (int i = 0; i < list.size(); ++i) {
                  QFileInfo fi = list.at(i);
                  char* path = strdup(fi.filePath().toLatin1().data());
                  FSTInfo* info = fst_get_info(path);
                  if (info) {
                        //
                        // simple hack:
                        // plugins with no inputs are treated
                        // as software synthesizer
                        //
                        // if (info->wantEvents && info->numOutputs) {
                        if (info->numInputs == 0 && info->numOutputs) {
                              if (debugMsg)
                                    printf("  add vsti synti <%s>\n", fi.fileName().toLatin1().data());
                              synthis.push_back(new VstSynth(&fi));
                              fst_free_info(info);
                              }
                        else if (info->numInputs && info->numOutputs) {
                              if (debugMsg)
                                    printf("  add vst plugin <%s>\n", fi.fileName().toLatin1().data());
                              plugins.push_back(new VstPlugin(&fi, info));
                              }
                        }
                  free(path);
                  }
            }
      }

//---------------------------------------------------------
//   fstSignalHandler
//---------------------------------------------------------

static void fstSignalHandler(int sig, siginfo_t* /*info*/, void* /*context*/)
      {
      fst_error("fst signal handler %d, thread = 0x%x", sig, pthread_self ());
      //if (sig == SIGSEGV || sig == SIGABRT) {
            char*p = 0;
            *p = 0;
//            }
      fatalError("fst signal");
      }

//---------------------------------------------------------
//   initVST
//---------------------------------------------------------

void initVST()
      {
      jfstReserveMem(1000000);

      if (fst_init(fstSignalHandler)) {
            printf("initVST failed\n");
            return;
            }
      char* vstPath = getenv("VST_PATH");
      if (vstPath == 0)
            vstPath = "/usr/lib/vst:/usr/local/lib/vst";

      char* p = vstPath;
      while (*p != '\0') {
            char* pe = p;
            while (*pe != ':' && *pe != '\0')
                  pe++;

            int n = pe - p;
            if (n) {
                  char* buffer = new char[n + 1];
                  strncpy(buffer, p, n);
                  buffer[n] = '\0';
                  scanVstDir(QString(buffer));
                  delete[] buffer;
                  }
            p = pe;
            if (*p == ':')
                  p++;
            }
      }

//---------------------------------------------------------
//   guiVisible
//---------------------------------------------------------

bool VstSynthIF::guiVisible() const
      {
      return _guiVisible;
      }

//---------------------------------------------------------
//   showGui
//---------------------------------------------------------

void VstSynthIF::showGui(bool v)
      {
      if (v == guiVisible())
            return;
      if (v)
            _fst->runEditor();
      else
            _fst->destroyEditor();
      _guiVisible = v;
      }

//---------------------------------------------------------
//   receiveEvent
//---------------------------------------------------------

MidiEvent VstSynthIF::receiveEvent()
      {
      return MidiEvent();
      }

//---------------------------------------------------------
//   hasGui
//---------------------------------------------------------

bool VstSynthIF::hasGui() const
      {
      return _fst->hasGui();
      }

//---------------------------------------------------------
//   incInstances
//---------------------------------------------------------

void VstSynth::incInstances(int val)
      {
      _instances += val;
      if (_instances == 0 && fstHandle) {
            fst_unload(fstHandle);
            fstHandle = 0;
            }
      }

//---------------------------------------------------------
//   init
//---------------------------------------------------------

bool VstSynthIF::init(FSTHandle* h)
      {
      _fst = new FstPlugin();
      _fst->instantiate(h, 0);
      _fst->setSampleRate(AL::sampleRate);
      _fst->setBlockSize(segmentSize);
      _fst->mainsChanged(true);
      _fst->setProgram(0);
      return true;
      }

//---------------------------------------------------------
//   channels
//---------------------------------------------------------

int VstSynthIF::channels() const
      {
      return _fst->numOutputs();
      }

//---------------------------------------------------------
//   createSIF
//---------------------------------------------------------

SynthIF* VstSynth::createSIF(SynthI* s)
      {
      VstSynthIF* sif = new VstSynthIF(s);
      ++_instances;
      const char* path = info.filePath().toLatin1().data();

      if (fstHandle == 0) {
            fstHandle = fst_load(path);
            if (fstHandle == 0) {
                  printf("SynthIF:: cannot load vst plugin %s\n", path);
                  return 0;
                  }
            }
      sif->init(fstHandle);
      return sif;
      }

//---------------------------------------------------------
//   deactivate3
//---------------------------------------------------------

void VstSynthIF::deactivate3()
      {
      if (_fst) {
            delete _fst;
            _fst = 0;
            }
      }

//---------------------------------------------------------
//   ~VstSynthIF
//---------------------------------------------------------

VstSynthIF::~VstSynthIF()
      {
      deactivate3();
      }

//---------------------------------------------------------
//   setParameter
//---------------------------------------------------------

void VstSynthIF::setParameter(int idx, float value)
      {
      _fst->setParameter(idx, value);
      }

//---------------------------------------------------------
//   write
//---------------------------------------------------------

void VstSynthIF::write(Xml& xml) const
      {
      //---------------------------------------------
      // dump current state of synth
      //---------------------------------------------

      int params = _fst->numParameter();
      for (int i = 0; i < params; ++i) {
            float f = _fst->getParameter(i);
            xml.floatTag("param", f);
            }
      }

//---------------------------------------------------------
//   getData
//---------------------------------------------------------

void VstSynthIF::getData(MidiEventList* el, unsigned pos, int ports, unsigned n, float** buffer)
      {
      int endPos = pos + n;
      iMidiEvent i = el->begin();
      for (; i != el->end(); ++i) {
            if (i->time() >= endPos)
                  break;
            putEvent(*i);
            }
      el->erase(el->begin(), i);

      int outputs = _fst->numOutputs();
      if (ports < outputs) {
            float* ob[outputs];
            float fp[n * (outputs-ports)];   // dummy output buffer
            for (int i = 0; i < outputs; ++i) {
                  if (i < ports)
                        ob[i] = buffer[i];
                  else
                        ob[i] = fp + n * (i-ports);
                  }
            if (_fst->canReplacing())
                  _fst->processReplacing(0, ob, n);
            else {
                  for (int i = 0; i < outputs; ++i)
                        memset(ob[i], 0, n * sizeof(float));
                  _fst->process(0, ob, n);
                  }
            }
      else {
            if (_fst->canReplacing())
                  _fst->processReplacing(0, buffer, n);
            else {
                  for (int i = 0; i < outputs; ++i)
                        memset(buffer[i], 0, n * sizeof(float));
                  _fst->process(0, buffer, n);
                  }
            }
      }

//---------------------------------------------------------
//   putEvent
//---------------------------------------------------------

bool VstSynthIF::putEvent(const MidiEvent& ev)
      {
      if (midiOutputTrace)
            ev.dump();
      _fst->putEvent(ev);
      return false;
      }

//---------------------------------------------------------
//   VstPlugin
//---------------------------------------------------------

VstPlugin::VstPlugin(const QFileInfo* fi, FSTInfo* i)
   : Plugin(fi)
      {
      info = i;
      fstHandle = 0;
      }

//---------------------------------------------------------
//   VstPlugin
//---------------------------------------------------------

VstPlugin::~VstPlugin()
      {
      fst_free_info(info);
      }

//---------------------------------------------------------
//   createPIF
//---------------------------------------------------------

PluginIF* VstPlugin::createPIF(PluginI* pi)
      {
      VstPluginIF* pif = new VstPluginIF(pi);
      ++_instances;
      const char* path = fi.filePath().toLatin1().data();

      if (fstHandle == 0) {
            fstHandle = fst_load(path);
            if (fstHandle == 0) {
                  printf("SynthIF:: cannot load vst plugin %s\n", path);
                  return 0;
                  }
            }
      pif->init(fstHandle);
      return pif;
      }

//---------------------------------------------------------
//   parameter
//---------------------------------------------------------

int VstPlugin::parameter() const
      {
      return info->numParams;
      }

//---------------------------------------------------------
//   inports
//---------------------------------------------------------

int VstPlugin::inports() const
      {
      return info->numInputs;
      }

//---------------------------------------------------------
//   outports
//---------------------------------------------------------

int VstPlugin::outports() const
      {
      return info->numOutputs;
      }

//---------------------------------------------------------
//   id
//---------------------------------------------------------

unsigned long VstPlugin::id() const
      {
      return info->UniqueID;
      }

//---------------------------------------------------------
//   VstPluginIF
//---------------------------------------------------------

VstPluginIF::VstPluginIF(PluginI* pi)
   : PluginIF(pi)
      {
      _fst = 0;
      _guiVisible = false;
      }

VstPluginIF::~VstPluginIF()
      {
      if (_fst == 0)
            return;
      delete _fst;
      }

//---------------------------------------------------------
//   init
//---------------------------------------------------------

bool VstPluginIF::init(FSTHandle* h)
      {
      _fst = new FstPlugin();
      _fst->instantiate(h, this);
      _fst->setSampleRate(AL::sampleRate);
      _fst->setBlockSize(segmentSize);
      _fst->mainsChanged(true);
      _fst->setProgram(0);
      return true;
      }

//---------------------------------------------------------
//   hasGui
//---------------------------------------------------------

bool VstPluginIF::hasGui() const
      {
      return _fst->hasGui();
      }

//---------------------------------------------------------
//   showGui
//---------------------------------------------------------

void VstPluginIF::showGui(bool v)
      {
      if (v == guiVisible())
            return;
      if (v)
            _fst->runEditor();
      else
            _fst->destroyEditor();
      _guiVisible = v;
      }

//---------------------------------------------------------
//   getParameterName
//---------------------------------------------------------

const char* VstPluginIF::getParameterName(int idx) const
      {
      return _fst->getParameterName(idx);
      }

//---------------------------------------------------------
//   getParameterLabel
//---------------------------------------------------------

const char* VstPluginIF::getParameterLabel(int idx) const
      {
      return _fst->getParameterLabel(idx);
      }

//---------------------------------------------------------
//   getParameterDisplay
//---------------------------------------------------------

const char* VstPluginIF::getParameterDisplay(int idx, float val) const
      {
      return _fst->getParameterDisplay(idx, val);
      }

//---------------------------------------------------------
//   apply
//---------------------------------------------------------

void VstPluginIF::apply(unsigned nframes, float** src, float** dst)
      {
      if (_fst->canReplacing())
            _fst->processReplacing(src, dst, nframes);
      else {
            int n = _fst->numOutputs();

            for (int i = 0; i < n; ++i)
                  memset(dst[i], 0, sizeof(float) * nframes);
            _fst->process(src, dst, nframes);
            }
      }

//---------------------------------------------------------
//   setParam
//---------------------------------------------------------

void VstPluginIF::setParam(int i, float val)
      {
      _fst->setParameter(i, val);
      }

//---------------------------------------------------------
//   param
//---------------------------------------------------------

float VstPluginIF::param(int i) const
      {
      return _fst->getParameter(i);
      }

#else
void initVST() {}
#endif

