//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: synth.h,v 1.22.2.12 2009/12/06 10:05:00 terminator356 Exp $
//  (C) Copyright 2000-2004 Werner Schweer (ws@seh.de)
//  (C) Copyright 2016 Tim E. Real (terminator356 on users dot sourceforge dot net)
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
//=========================================================

#ifndef __SYNTH_H__
#define __SYNTH_H__

#include <string>
#include <vector>
#include <map>

#include "globals.h"
#include "node.h"
#include "instruments/minstrument.h"
#include "mididev.h"
#include "midiport.h"
#include "track.h"
#include "stringparam.h"
#include "plugin.h"

#include <QFileInfo>

// Current version of saved midistate data.
#define SYNTH_MIDI_STATE_SAVE_VERSION 2

class Mess;
struct MESS;

namespace MusEGui {
class PopupMenu;
}

namespace MusECore {

class SynthI;
class SynthIF;

//---------------------------------------------------------
//   Synth
//    software synthesizer
//---------------------------------------------------------

class Synth {
   protected:
      QFileInfo info;
      int _instances;
      QString _name;
      QString _description;
      QString _maker;
      QString _version;
      Plugin::PluginFeatures _requiredFeatures;

   public:
      enum Type { METRO_SYNTH=0, MESS_SYNTH, DSSI_SYNTH, VST_SYNTH, VST_NATIVE_SYNTH, VST_NATIVE_EFFECT, LV2_SYNTH, LV2_EFFECT, SYNTH_TYPE_END };

      Synth(const QFileInfo& fi, 
            QString label, QString descr, QString maker, QString ver, 
            Plugin::PluginFeatures reqFeatures = Plugin::NoFeatures);

      virtual ~Synth() {}

      virtual Type synthType() const = 0;
      virtual Plugin::PluginFeatures requiredFeatures() const { return _requiredFeatures; }
      int instances() const                            { return _instances; }
      virtual void incInstances(int val)               { _instances += val; }
      QString completeBaseName()                       { return info.completeBaseName(); } // ddskrjo
      QString baseName()                               { return info.baseName(); } // ddskrjo
      QString name() const                             { return _name; }
      QString absolutePath() const                     { return info.absolutePath(); }
      QString path() const                             { return info.path(); }
      QString filePath() const                         { return info.filePath(); }
      QString fileName() const                         { return info.fileName(); }
      QString description() const                      { return _description; }
      QString version() const                          { return _version; }
      QString maker() const                            { return _maker; }

      virtual SynthIF* createSIF(SynthI*) = 0;
      };

//---------------------------------------------------------
//   MessSynth
//---------------------------------------------------------

class MessSynth : public Synth {
      const MESS* _descr;

   public:
      MessSynth(const QFileInfo& fi, QString label, QString descr, QString maker, QString ver) :
               Synth(fi, label, descr, maker, ver) { _descr = 0; }

      virtual ~MessSynth() {}
      virtual Type synthType() const { return MESS_SYNTH; }

      virtual void* instantiate(const QString&);

      virtual SynthIF* createSIF(SynthI*);
      };


//---------------------------------------------------------
//   SynthIF
//    synth instance interface
//   NOTICE: If implementing sysex support, be sure to make a unique ID and use
//    it to filter out unrecognized sysexes. Headers should be constructed as:
//      MUSE_SYNTH_SYSEX_MFG_ID        The MusE SoftSynth Manufacturer ID byte (0x7C) found in midi.h
//      0xNN                           The synth's unique ID byte
//---------------------------------------------------------

class SynthIF : public PluginIBase {

   protected:
      SynthI* synti;

   public:
      SynthIF(SynthI* s) { synti = s; }
      virtual ~SynthIF() {}

      // This is only a kludge required to support old songs' midistates. Do not use in any new synth.
      virtual int oldMidiStateHeader(const unsigned char** /*data*/) const { return 0; }

      virtual void guiHeartBeat() = 0;
      virtual void showGui(bool v) { if(synti && hasGui()) PluginIBase::showGui(v); } 
      virtual bool hasGui() const = 0;
      virtual bool hasNativeGui() const = 0;
      virtual void preProcessAlways() = 0;
      virtual bool getData(MidiPort*, unsigned pos, int ports, unsigned n, float** buffer) = 0;
      virtual MidiPlayEvent receiveEvent() = 0;
      virtual int eventsPending() const = 0;

      virtual int channels() const = 0;
      virtual int totalOutChannels() const = 0;
      virtual int totalInChannels() const = 0;
      virtual void deactivate3() = 0;
      virtual QString getPatchName(int, int, bool) const = 0;
      virtual void populatePatchPopup(MusEGui::PopupMenu*, int, bool) = 0;
      virtual void write(int level, Xml& xml) const = 0;
      virtual double getParameter(unsigned long idx) const = 0;
      virtual void setParameter(unsigned long idx, double value) = 0;
      virtual int getControllerInfo(int id, QString* name, int* ctrl, int* min, int* max, int* initval) = 0;      
      // Returns a map item with members filled from either the original or working map item,
      //  depending on which Field flags are set. The returned map includes any requested
      //  WorkingDrumMapEntry::OverrideType instrument overrides. Channel can be -1 meaning default.
      virtual void getMapItem(int /*channel*/, int /*patch*/, int /*index*/, DrumMap& /*dest_map*/, int /*overrideType*/ = WorkingDrumMapEntry::AllOverrides) const;

      //-------------------------
      // Methods for PluginIBase:
      //-------------------------

      virtual Plugin::PluginFeatures requiredFeatures() const;
      virtual bool on() const;
      virtual void setOn(bool val);
      virtual unsigned long pluginID();
      virtual int id();
      virtual QString pluginLabel() const;
      virtual QString name() const;
      virtual QString lib() const;
      virtual QString dirPath() const;
      virtual QString fileName() const;
      virtual QString titlePrefix() const;
      virtual AudioTrack* track();
      virtual void enableController(unsigned long i, bool v = true);
      virtual bool controllerEnabled(unsigned long i) const;
      virtual void enableAllControllers(bool v = true);
      virtual void updateControllers();
      virtual void activate();
      virtual void deactivate();

      virtual void writeConfiguration(int level, Xml& xml);
      virtual bool readConfiguration(Xml& xml, bool readPreset=false);

      virtual unsigned long parameters() const;
      virtual unsigned long parametersOut() const;
      virtual void setParam(unsigned long i, double val);
      virtual double param(unsigned long i) const;
      virtual double paramOut(unsigned long i) const;
      virtual const char* paramName(unsigned long i);
      virtual const char* paramOutName(unsigned long i);
      // FIXME TODO: Either find a way to agnosticize these two ranges, or change them from ladspa ranges to a new MusE range class.
      virtual LADSPA_PortRangeHint range(unsigned long i);
      virtual LADSPA_PortRangeHint rangeOut(unsigned long i);
      virtual float latency() const;
      virtual CtrlValueType ctrlValueType(unsigned long i) const;
      virtual CtrlList::Mode ctrlMode(unsigned long i) const;
      };

//---------------------------------------------------------
//   SynthI
//    software synthesizer instance
//    Track
//    MidiDevice
//    MidiInstrument
//---------------------------------------------------------

class SynthI : public AudioTrack, public MidiDevice,
   public MidiInstrument
      {
      static bool _isVisible;
      SynthIF* _sif;

   protected:
      Synth* synthesizer;

      MPEventList _outPlaybackEvents;
      MPEventList _outUserEvents;
      
      // List of initial floating point parameters, for synths which use them.
      // Used once upon song reload, then discarded.
      std::vector<double> initParams;
      //custom params in xml song file , synth tag, that will be passed to new SynthIF:setCustomData(Xml &) method
      //now only lv2host uses them, others simply ignore
      std::vector<QString> accumulatedCustomParams;

      // Initial, and running, string parameters for synths which use them, like dssi.
      StringParamMap _stringParamMap;

// REMOVE Tim. latency. Removed. Moved into public.
//       void preProcessAlways();
      bool getData(unsigned a, int b, unsigned c, float** data);
      // Returns the number of frames to shift forward output event scheduling times when putting events
      //  into the eventFifos.
      virtual unsigned int pbForwardShiftFrames() const;

      virtual QString open();
      virtual void close();
      virtual Track* newTrack() const { return 0; }

   public:
      friend class SynthIF;
      friend class MessSynthIF;
      friend class DssiSynthIF;
	    friend class LV2SynthIF;
      friend class VstSynthIF;
      friend class VstNativeSynthIF;
      friend class MetronomeSynthIF;

      SynthI();
      SynthI(const SynthI& si, int flags);
      virtual ~SynthI();
      SynthI* clone(int flags) const { return new SynthI(*this, flags); }

      virtual inline MidiDeviceType deviceType() const { return SYNTH_MIDI; }
      // Virtual so that inheriters (synths etc) can return whatever they want.
      virtual inline NoteOffMode noteOffMode() const { return NoteOffAll; }

      SynthIF* sif() const { return _sif; }
      bool initInstance(Synth* s, const QString& instanceName);
      virtual float trackLatency(int channel) const { return _sif->latency() + AudioTrack::trackLatency(channel); }

      void read(Xml&);
      virtual void write(int, Xml&) const;

      void setName(const QString& s);
      QString name() const          { return AudioTrack::name(); }

      Synth* synth() const          { return synthesizer; }
      virtual bool isSynti() const  { return true; }

      // Event time and tick must be set by caller beforehand.
      // Overridden here because input from synths may need to be treated specially.
      virtual void recordEvent(MidiRecordEvent&);

      virtual Plugin::PluginFeatures pluginFeatures() const { return _sif->requiredFeatures(); }
      
      // Number of routable inputs/outputs for each Route::RouteType.
      virtual RouteCapabilitiesStruct routeCapabilities() const;
      
      virtual QString getPatchName(int ch, int prog, bool dr, bool /*includeDefault*/ = true) const {
            return _sif->getPatchName(ch, prog, dr);
            }

      // Returns a map item with members filled from either the original or working map item,
      //  depending on which Field flags are set. The returned map includes any requested
      //  WorkingDrumMapEntry::OverrideType instrument overrides. Channel can be -1 meaning default.
      virtual void getMapItem(int channel, int patch, int index, DrumMap& dest_map, int overrideType = WorkingDrumMapEntry::AllOverrides) const {
            return _sif->getMapItem(channel, patch, index, dest_map, overrideType);
            }

      virtual void populatePatchPopup(MusEGui::PopupMenu* m, int i, bool d) {
            _sif->populatePatchPopup(m, i, d);
            }

      void currentProg(int chan, int *prog, int *bankL, int *bankH)
           {  _curOutParamNums[chan].currentProg(prog, bankL, bankH);  }
      void setCurrentProg(int chan, int prog, int bankL, int bankH)
           {  _curOutParamNums[chan].setCurrentProg(prog, bankL, bankH);  }

      void guiHeartBeat()     { return _sif->guiHeartBeat(); }
      bool guiVisible() const { return _sif->guiVisible(); }
      void showGui(bool v)    { _sif->showGui(v); }
      bool hasGui() const     { return _sif->hasGui(); }
      bool nativeGuiVisible() const { return _sif->nativeGuiVisible(); }
      void showNativeGui(bool v)    { _sif->showNativeGui(v); }
      bool hasNativeGui() const     { return _sif->hasNativeGui(); }
      void getGeometry(int* x, int* y, int* w, int* h) const {
            _sif->getGeometry(x, y, w, h);
            }
      void setGeometry(int x, int y, int w, int h) {
            _sif->setGeometry(x, y, w, h);
            }
      void getNativeGeometry(int* x, int* y, int* w, int* h) const {
            _sif->getNativeGeometry(x, y, w, h);
            }
      void setNativeGeometry(int x, int y, int w, int h) {
            _sif->setNativeGeometry(x, y, w, h);
            }

      virtual void processMidi(unsigned int /*curFrame*/ = 0);
// REMOVE Tim. latency. Added. Moved here from protected.
      void preProcessAlways();

      MidiPlayEvent receiveEvent() { return _sif->receiveEvent(); }
      int eventsPending() const    { return _sif->eventsPending(); }
      void deactivate2();
      void deactivate3();
      bool isActivated() const         { return synthesizer && _sif; }
      virtual bool hasAuxSend() const  { return true; }
      static void setVisible(bool t) { _isVisible = t; }
      virtual int height() const;
      static bool visible() { return _isVisible; }

      };

//---------------------------------------------------------
//   MessSynthIF
//    mess synthesizer instance
//   NOTICE: If implementing sysex support, be sure to make a unique ID and use
//    it to filter out unrecognized sysexes. Headers should be constructed as:
//      MUSE_SYNTH_SYSEX_MFG_ID        The MusE SoftSynth Manufacturer ID byte (0x7C) found in midi.h
//      0xNN                           The synth's unique ID byte
//---------------------------------------------------------

class MessSynthIF : public SynthIF {
      Mess* _mess;

      bool processEvent(const MidiPlayEvent& ev);
      
   public:
      MessSynthIF(SynthI* s) : SynthIF(s) { _mess = 0; }
      virtual ~MessSynthIF() { }

      // This is only a kludge required to support old songs' midistates. Do not use in any new synth.
      virtual int oldMidiStateHeader(const unsigned char** data) const;

      virtual void guiHeartBeat();
      virtual bool guiVisible() const { return false; }
      virtual bool hasGui() const     { return false; }
      virtual bool nativeGuiVisible() const;
      virtual void showNativeGui(bool v);
      virtual bool hasNativeGui() const;
      virtual void getNativeGeometry(int*, int*, int*, int*) const;
      virtual void setNativeGeometry(int, int, int, int);
      virtual void preProcessAlways();
      virtual bool getData(MidiPort*, unsigned pos, int ports, unsigned n, float** buffer);
      virtual MidiPlayEvent receiveEvent();
      virtual int eventsPending() const;
      bool init(Synth* s, SynthI* si);

      virtual int channels() const;
      virtual int totalOutChannels() const;
      virtual int totalInChannels() const;
      virtual void deactivate3();
      virtual QString getPatchName(int, int, bool) const;
      virtual void populatePatchPopup(MusEGui::PopupMenu*, int, bool);
      virtual void write(int level, Xml& xml) const;
      virtual double getParameter(unsigned long) const { return 0.0; }
      virtual void setParameter(unsigned long, double) {}
      virtual int getControllerInfo(int id, QString* name, int* ctrl, int* min, int* max, int* initval);
      // Returns a map item with members filled from either the original or working map item,
      //  depending on which Field flags are set. The returned map includes any requested
      //  WorkingDrumMapEntry::OverrideType instrument overrides. Channel can be -1 meaning default.
      virtual void getMapItem(int /*channel*/, int /*patch*/, int /*index*/, DrumMap& /*dest_map*/, int /*overrideType*/ = WorkingDrumMapEntry::AllOverrides) const;
      };

extern QString synthType2String(Synth::Type);
extern Synth::Type string2SynthType(const QString&);

} // namespace MusECore

namespace MusEGlobal {
extern std::vector<MusECore::Synth*> synthis;  // array of available synthis
}

#endif

