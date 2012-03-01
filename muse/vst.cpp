//===================================================================
//  MusE
//  Linux Music Editor
//  $Id: vst.cpp,v 1.5.2.6 2009/12/06 10:05:00 terminator356 Exp $
//
//   This code is based on jack_fst:
//   Copyright (C) 2004 Paul Davis <paul@linuxaudiosystems.com>
//                      Torben Hohn <torbenh@informatik.uni-bremen.de>
//
//  (C) Copyright 2004 Werner Schweer (ws@seh.de)
//
//  This program is free software; you can redistribute it and/or
//  modify it under the terms of the GNU General Public License
//  as published by the Free Software Foundation; version 2 of
//  the License, or (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
//
//===================================================================

#include "config.h"

#ifdef VST_SUPPORT

#include <QDir>
#include <QMenu>

#include <cmath>
#include <fst.h>
#include <vst/aeffectx.h>
#include <jack/jack.h>

#include "vst.h"
#include "globals.h"
#include "synth.h"
#include "jackaudio.h"
#include "midi.h"
#include "xml.h"

namespace MusECore {

extern "C" void fst_error(const char *fmt, ...);
extern long vstHostCallback (AEffect*, long, long, long, void*, float);

extern JackAudioDevice* jackAudio;

//---------------------------------------------------------
//   vstHostCallback
//---------------------------------------------------------

long vstHostCallback(AEffect* effect,
   long opcode, long index, long value, void* ptr, float opt)
      {
      static VstTimeInfo _timeInfo;

      jack_position_t jack_pos;
      jack_transport_state_t tstate;

      switch (opcode) {
            case audioMasterAutomate:
                  // index, value, returns 0
                  effect->setParameter (effect, index, opt);
                  return 0;

            case audioMasterVersion:
                  // vst version, currently 2 (0 for older)
                  return 2;

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
                        _timeInfo.sampleRate = sampleRate;
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
//   scanVstDir
//---------------------------------------------------------

static void scanVstDir(const QString& s)
      {
      if (debugMsg)
            printf("scan vst plugin dir <%s>\n", s.toLatin1());
      QDir pluginDir(s, QString("*.dll"), QDir::Files);
      if (pluginDir.exists()) {
            const QFileInfoList* list = pluginDir.entryInfoList();
            QFileInfoListIterator it(*list);
            QFileInfo* fi;
            while((fi = it.current())) {
                  char* path = strdup(fi->filePath().toLatin1());
                  FSTInfo* info = fst_get_info(path);
                  if (info) {
                        if (info->numInputs == 0 && info->numOutputs)
                              synthis.push_back(new VstSynth(*fi, fi->baseName(), QString(), QString(), QString()));
                        fst_free_info(info);
                        }
                  free(path);
                  ++it;
                  }
            }
      }

//---------------------------------------------------------
//   fstSignalHandler
//---------------------------------------------------------

static void fstSignalHandler(int sig, siginfo_t* /*info*/, void* /*context*/)
      {
      fst_error("fst signal handler %d, thread = 0x%x", sig, pthread_self ());
      if (sig == SIGSEGV || sig == SIGABRT) {
            char*p = 0;
            *p = 0;
            }
      exit(-1);
      }

void jfst_reserve_mem (int bufsize)
{
        char buf [bufsize];
        int i;

        fprintf (stderr, "Reserving memory: base=%p, size=%d, end=%p\n",
                buf, sizeof(buf), buf+sizeof(buf));
        for (i=0; i<bufsize; i++)
        {
                buf[i] = (char) (i % 256);
        }
}

//---------------------------------------------------------
//   initVST
//---------------------------------------------------------

void initVST()
      {
      jfst_reserve_mem(1000000);
      
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

bool VstSynthIF::nativeGuiVisible() const
      {
      return _guiVisible;
      }


//---------------------------------------------------------
//   showGui
//---------------------------------------------------------

void VstSynthIF::showNativeGui(bool v)
      {
      if (v == nativeGuiVisible())
            return;
      if (v)
            fst_run_editor(_fst);
      else
            fst_destroy_editor(_fst);
      _guiVisible = v;
      }

//---------------------------------------------------------
//   receiveEvent
//---------------------------------------------------------

MidiPlayEvent VstSynthIF::receiveEvent()
      {
      return MidiPlayEvent();
      }

//---------------------------------------------------------
//   hasGui
//---------------------------------------------------------

bool VstSynthIF::hasNativeGui() const
      {
      return _fst->plugin->flags & effFlagsHasEditor;
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
//   instantiate
//---------------------------------------------------------

void* VstSynth::instantiate()
      {
      ++_instances;
      QString n;
      n.setNum(_instances);
      QString instanceName = baseName() + "-" + n;
      doSetuid();
      QByteArray ba = info.filePath().toLatin1();
      const char* path = ba.constData();

      fstHandle = fst_load(path);
      if (fstHandle == 0) {
            printf("Synth::instantiate: cannot load vst plugin %s\n", path);
            undoSetuid();
            return 0;
            }
      FST* fst = fst_instantiate(fstHandle, vstHostCallback, 0);
      if (fst == 0) {
            printf("Synth::instantiate:: cannot instantiate plugin %s\n", path);
            undoSetuid();
            return 0;
            }
      AEffect* plugin = fst->plugin;
      plugin->dispatcher (plugin, effMainsChanged, 0, 1, 0, 0.0f);

      /* set program to zero */

      plugin->dispatcher (plugin, effSetProgram, 0, 0, NULL, 0.0f);

      if (fst_run_editor(fst)) {
            printf("Synth::instantiate: cannot create gui");
            undoSetuid();
            return 0;
            }

      undoSetuid();
      return fst;
      }

//---------------------------------------------------------
//   init
//---------------------------------------------------------

bool VstSynthIF::init(Synth* s)
      {
      _fst = (FST*)((VstSynth*)s)->instantiate();
      return (_fst == 0);
      }

//---------------------------------------------------------
//   channels
//---------------------------------------------------------

int VstSynthIF::channels() const
      {
      AEffect* plugin = _fst->plugin;
      return plugin->numOutputs;
      }

int VstSynthIF::totalOutChannels() const
      {
      AEffect* plugin = _fst->plugin;
      return plugin->numOutputs;
      }

int VstSynthIF::totalInChannels() const
      {
      AEffect* plugin = _fst->plugin;
      return plugin->numInputs;
      }

//---------------------------------------------------------
//   createSIF
//---------------------------------------------------------

SynthIF* VstSynth::createSIF(SynthI* s)
      {
      VstSynthIF* sif = new VstSynthIF(s);
      sif->init(this, s);
      return sif;
      }

//---------------------------------------------------------
//   deactivate3
//---------------------------------------------------------

void VstSynthIF::deactivate3()
      {
      if (_fst) {
            if (_guiVisible)
                  fst_destroy_editor(_fst);
            fst_close(_fst);
            _fst = 0;
            }
      }

//---------------------------------------------------------
//   getParameter
//---------------------------------------------------------

float VstSynthIF::getParameter(unsigned long idx) const
      {
      return _fst->plugin->getParameter(_fst->plugin, idx);
      }

//---------------------------------------------------------
//   setParameter
//---------------------------------------------------------

void VstSynthIF::setParameter(unsigned long idx, float value)
      {
      _fst->plugin->setParameter(_fst->plugin, idx, value);
      }

//---------------------------------------------------------
//   write
//---------------------------------------------------------

void VstSynthIF::write(int level, Xml& xml) const
      {
      //---------------------------------------------
      // dump current state of synth
      //---------------------------------------------

      int len = 0;
      const unsigned char* p;
      AEffect* plugin = _fst->plugin;
      int params = plugin->numParams;
      for (int i = 0; i < params; ++i) {
            float f = plugin->getParameter(plugin, i);
            xml.floatTag(level, "param", f);
            }
      }

//---------------------------------------------------------
//   getData
//---------------------------------------------------------

iMPEvent VstSynthIF::getData(MidiPort* mp, MPEventList* el, iMPEvent i, unsigned pos, int ports, unsigned n, float** buffer)
      {
      AEffect* plugin = _fst->plugin;
      for (; i != el->end(); ++i) {
          if (mp)
                  mp->sendEvent(*i);
            else {
                  if (putEvent(*i))
                        break;
                  }
            }
      if (plugin->flags & effFlagsCanReplacing) {
            plugin->processReplacing(plugin, 0, buffer, n);
            }
      else {
            plugin->process(plugin, 0, buffer, n);
            }
      return el->end();
      }

//---------------------------------------------------------
//   putEvent
//---------------------------------------------------------

bool VstSynthIF::putEvent(const MidiPlayEvent& ev)
      {
      if (midiOutputTrace)
            ev.dump();
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
      int rv = plugin->dispatcher(plugin, effProcessEvents, 0, 0, &events, 0.0f);
      return false;
      }

} // namespace MusECore

#else
namespace MusECore {
void initVST() {}
} // namespace MusECore
#endif

