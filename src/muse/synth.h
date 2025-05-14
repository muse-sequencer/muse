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

#include "globaldefs.h"
#include "instruments/minstrument.h"
#include "mididev.h"
#include "track.h"
#include "stringparam.h"
#include "plugin.h"
#include "midi_controller.h"

#include <QFileInfo>

// Current version of saved midistate data.
#define SYNTH_MIDI_STATE_SAVE_VERSION 2


// Forward declarations:
class Mess;
struct MESS;

namespace MusEGui {
class PopupMenu;
}

namespace MusEPlugin {
class PluginScanInfoStruct;
}

namespace MusECore {

class MidiPort;
class SynthI;
class SynthIF;
class Xml;
class CtrlListList;

//---------------------------------------------------------
//   Synth
//    software synthesizer
//---------------------------------------------------------

class Synth : public PluginBase {
   protected:
      MidiCtl2LadspaPortMap midiCtl2PortMap;   // Maps midi controller numbers to audio port numbers.
      MidiCtl2LadspaPortMap port2MidiCtlMap;   // Maps audio port numbers to midi controller numbers.

   public:
      Synth(const MusEPlugin::PluginScanInfoStruct&);
      virtual ~Synth();

      // Returns true if the midi controller number is mapped to an audio controller,
      //  and returns the audio controller number in audioCtrl (if valid). Returns false if not mapped.
      bool midiToAudioCtrlMapped(unsigned long int midiCtrl, unsigned long int* audioCtrl) const;
      // Returns true if the audio controller number is mapped to a midi controller,
      //  and returns the midi controller number in midiCtrl (if valid). Returns false if not mapped.
      bool audioToMidiCtrlMapped(unsigned long int audioCtrl, unsigned long int* midiCtrl) const;
      // Returns true if there are any mapped midi to audio controllers.
      bool hasMappedMidiToAudioCtrls() const;

      virtual SynthIF* createSIF(SynthI*) = 0;
      };

//---------------------------------------------------------
//   MessSynth
//---------------------------------------------------------

class MessSynth : public Synth {
      const MESS* _descr;

   public:
      MessSynth(const MusEPlugin::PluginScanInfoStruct&);
      virtual ~MessSynth() {}

      friend class MessSynthIF;

      SynthIF* createSIF(SynthI*);
      bool reference();
      int release();
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

      // Sets up an existing controller based on info gathered from either the plugin
      //  or the plugin's persistent info if the plugin is missing.
      // Sets various controller properties such as names, ranges, value types, and value units.
      bool setupController(CtrlList *cl) const;

   public:
      SynthIF(SynthI* s);
      virtual ~SynthIF() {}

      // Provide proper outside access instead of relying on being a 'friend' of SynthI.
      inline SynthI* synthI() { return synti; }
      inline const SynthI* synthI_const() const { return synti; }

      // This is only a kludge required to support old songs' midistates. Do not use in any new synth.
      inline virtual int oldMidiStateHeader(const unsigned char** /*data*/) const { return 0; }

      virtual void showGui(bool v) { if(synti && hasGui()) PluginIBase::showGui(v); } 
      virtual bool hasGui() const = 0;
      virtual bool hasNativeGui() const = 0;
      virtual void preProcessAlways() { }
      virtual bool getData(MidiPort*, unsigned pos, int ports, unsigned n, float** buffer) = 0;
      virtual MidiPlayEvent receiveEvent() = 0;
      virtual int eventsPending() const = 0;

      virtual int channels() const = 0;
      virtual int totalOutChannels() const = 0;
      virtual int totalInChannels() const = 0;
      virtual void deactivate3() = 0;
      virtual QString getPatchName(int, int, bool) const = 0;
      virtual void populatePatchPopup(MusEGui::PopupMenu*, int /*channel*/, bool /*drum*/) = 0;
      virtual void write(int level, Xml& xml) const;
      virtual double getParameter(unsigned long idx) const = 0;
      virtual void setParameter(unsigned long idx, double value) = 0;
      virtual int getControllerInfo(int id, QString* name, int* ctrl, int* min, int* max, int* initval) = 0;      
      // Returns true if a note name list is found for the given patch.
      // If true, name either contains the note name, or is blank if no note name was found.
      // drum = Want percussion names, not melodic.
      virtual bool getNoteSampleName(
        bool /*drum*/, int /*channel*/, 
        int /*patch*/, int /*note*/, QString* /*name*/) const { return false; }

      //-------------------------
      // Methods for PluginIBase:
      //-------------------------

      virtual MusEPlugin::PluginFeatures_t requiredFeatures() const;
      virtual bool hasActiveButton() const;
      virtual bool active() const;
      virtual void setActive(bool);
      virtual bool hasBypass() const;
      virtual bool on() const;
      virtual void setOn(bool val);
      virtual unsigned long pluginID() const;
      virtual int id() const;
      // Here the name is the track name.
      virtual QString name() const;
      virtual QString displayName() const;
      // The short formal plugin name.
      virtual QString pluginLabel() const;
      // The full formal plugin name.
      virtual QString pluginName() const;
      virtual QString lib() const;
      virtual QString uri() const;
      virtual QString dirPath() const;
      virtual QString fileName() const;
      virtual QString titlePrefix() const;
      virtual AudioTrack* track() const;
      virtual void enableController(unsigned long i, bool v = true);
      virtual bool controllerEnabled(unsigned long i) const;
      virtual void enableAllControllers(bool v = true);
      virtual void updateControllers();
      virtual void activate();
      virtual void deactivate();

      // If isCopy is true, writes additional info including automation controllers and midi assignments.
      virtual void writeConfiguration(int level, Xml& xml, bool isCopy = false);
      virtual bool readConfiguration(Xml& xml, bool readPreset=false, int channels=0);
      // Returns a list of strings containing any custom configurations provided by the plugin.
      virtual std::vector<QString> getCustomData() const;

      virtual unsigned long parameters() const;
      virtual unsigned long parametersOut() const;
      virtual void setParam(unsigned long i, double val);
      virtual double param(unsigned long i) const;
      virtual double paramOut(unsigned long i) const;
      virtual const char* paramName(unsigned long i) const;
      virtual const char* paramOutName(unsigned long i) const;
      // FIXME TODO: Either find a way to agnosticize these two ranges, or change them from ladspa ranges to a new MusE range class.
      virtual LADSPA_PortRangeHint range(unsigned long i) const;
      virtual LADSPA_PortRangeHint rangeOut(unsigned long i) const;
      virtual void range(unsigned long i, float*, float*) const;
      virtual void rangeOut(unsigned long i, float*, float*) const;
      virtual unsigned long latencyOutPortIndex() const;
      virtual float latency() const;
      virtual unsigned long freewheelPortIndex() const;
      virtual unsigned long enableOrBypassPortIndex() const;
      virtual MusEPlugin::PluginLatencyReportingType pluginLatencyReportingType() const;
      virtual MusEPlugin::PluginBypassType pluginBypassType() const;
      virtual MusEPlugin::PluginFreewheelType pluginFreewheelType() const;
      virtual CtrlValueType ctrlValueType(unsigned long i) const;
      virtual CtrlList::Mode ctrlMode(unsigned long i) const;
      virtual CtrlValueType ctrlOutValueType(unsigned long i) const;
      virtual CtrlList::Mode ctrlOutMode(unsigned long i) const;

      // Returns true if the transport source is connected to any of the
      //  track's midi input ports (ex. synth ports not muse midi ports).
      virtual bool usesTransportSource() const;
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
  
      // Holds initial controller values, parameters, sysex, custom data etc. for synths which use them.
      PluginConfiguration _initConfig;

      bool getData(unsigned a, int b, unsigned c, float** data);

      // Returns the number of frames to shift forward output event scheduling times when putting events
      //  into the eventFifos.
      virtual unsigned int pbForwardShiftFrames() const;

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
      inline virtual float selfLatencyAudio(int channel) const
        { return (_sif ? _sif->latency() : 0) + AudioTrack::selfLatencyAudio(channel); }

      virtual QString open();
      virtual void close();

      void read(Xml&, XmlReadStatistics* stats = nullptr);
      virtual void write(int, Xml&, XmlWriteStatistics* stats = nullptr) const;

      void setName(const QString& s);
      inline QString name() const          { return AudioTrack::name(); }

      inline QString uri() const           { return synthesizer ? synthesizer->uri() : QString(); }

      const PluginConfiguration& initialConfiguration() const;
      // This version uses the supplied configuration.
      // NOTE: The track must already have been added to the track lists,
      //        because for DSSI, OSC needs to find the plugin in the track lists.
      // NOTE: If ConfigCustomData and ConfigParams options are given, and there is
      //        plugin custom data, ConfigParams will be ignored since that information
      //        is supposed to be inside the custom data.
      void configure(const PluginConfiguration&, PluginIBase::ConfigureOptions_t);
      // This version uses the built-in initial configuration member, which must already be filled.
      // NOTE: The track must already have been added to the track lists,
      //        because for DSSI, OSC needs to find the plugin in the track lists.
      // NOTE: If ConfigCustomData and ConfigParams options are given, and there is
      //        plugin custom data, ConfigParams will be ignored since that information
      //        is supposed to be inside the custom data.
      void configure(PluginIBase::ConfigureOptions_t);
      // Returns a synth configuration structure filled with the current state of the synth.
      // Note this does NOT include the controllers or midi assignments members. They should
      //  be obtained separately by the caller.
      PluginConfiguration getConfiguration() const;

      inline Synth* synth() const          { return synthesizer; }
      inline virtual bool isSynti() const  { return true; }

      // Event time and tick must be set by caller beforehand.
      // Overridden here because input from synths may need to be treated specially.
      virtual void recordEvent(MidiRecordEvent&);

      inline virtual MusEPlugin::PluginFeatures_t pluginFeatures() const { return _sif->requiredFeatures(); }
      
      // Number of routable inputs/outputs for each Route::RouteType.
      virtual RouteCapabilitiesStruct routeCapabilities() const;
      
      virtual QString getPatchName(int ch, int prog, bool dr, bool /*includeDefault*/ = true) const {
            return _sif ? _sif->getPatchName(ch, prog, dr) : QString();
            }

      // Returns a map item with members filled from either the original or working map item,
      //  depending on which Field flags are set. The returned map includes any requested
      //  WorkingDrumMapEntry::OverrideType instrument overrides. Channel can be -1 meaning default.
      virtual void getMapItem(int channel, int patch, int index, DrumMap& dest_map,
                              int overrideType = WorkingDrumMapEntry::AllOverrides) const;

      virtual void populatePatchPopup(MusEGui::PopupMenu* m, int i, bool d) {
            if(!_sif) return; else _sif->populatePatchPopup(m, i, d);
            }

      void currentProg(int chan, int *prog, int *bankL, int *bankH)
           {  _curOutParamNums[chan].currentProg(prog, bankL, bankH);  }
      void setCurrentProg(int chan, int prog, int bankL, int bankH)
           {  _curOutParamNums[chan].setCurrentProg(prog, bankL, bankH);  }

      // Returns true if the midi controller number is mapped to an audio controller,
      //  and returns the audio controller number in audioCtrl (if valid). Returns false if not mapped.
      bool midiToAudioCtrlMapped(unsigned long int midiCtrl, unsigned long int* audioCtrl) const;
      // Returns true if the audio controller number is mapped to a midi controller,
      //  and returns the midi controller number in midiCtrl (if valid). Returns false if not mapped.
      bool audioToMidiCtrlMapped(unsigned long int audioCtrl, unsigned long int* midiCtrl) const;
      // Returns true if there are any mapped midi to audio controllers.
      bool hasMappedMidiToAudioCtrls() const;

      virtual void guiHeartBeat();
      bool guiVisible() const { if(!_sif) return false; else return _sif->guiVisible(); }
      void showGui(bool v)    { if(!_sif) return; else _sif->showGui(v); }
      bool hasGui() const     { if(!_sif) return false; else return _sif->hasGui(); }
      void showGuiPending(bool v) { if(!_sif) return; else _sif->showGuiPending(v); }
      bool isShowGuiPending() const { if(!_sif) return false; else return _sif->isShowGuiPending(); }
      void updateGuiWindowTitle() const { if(!_sif) return; else _sif->updateGuiWindowTitle(); }
      bool nativeGuiVisible() const { if(!_sif) return false; else return _sif->nativeGuiVisible(); }
      void showNativeGui(bool v)    { if(!_sif) return; else _sif->showNativeGui(v); }
      void showNativeGuiPending(bool v) { if(!_sif) return; else _sif->showNativeGuiPending(v); }
      bool isShowNativeGuiPending() const { if(!_sif) return false; else return _sif->isShowNativeGuiPending(); }
      void updateNativeGuiWindowTitle() const { if(!_sif) return; else _sif->updateNativeGuiWindowTitle(); }
      bool hasNativeGui() const     { if(!_sif) return false; else return _sif->hasNativeGui(); }
      void nativeGuiTitleAboutToChange() const { if(!_sif) return; else _sif->nativeGuiTitleAboutToChange(); }
      void getGeometry(int* x, int* y, int* w, int* h) const {
            if(_sif)
              _sif->getGeometry(x, y, w, h);
            else { if(x) *x = 0; if(y) *y = 0; if(w) *w = 0; if(h) *h = 0; }
            }
      void setGeometry(int x, int y, int w, int h) {
            if(!_sif) return; else _sif->setGeometry(x, y, w, h);
            }
      void getNativeGeometry(int* x, int* y, int* w, int* h) const {
            if(_sif)
              _sif->getNativeGeometry(x, y, w, h);
            else { if(x) *x = 0; if(y) *y = 0; if(w) *w = 0; if(h) *h = 0; }
            }
      void setNativeGeometry(int x, int y, int w, int h) {
            if(!_sif) return; else _sif->setNativeGeometry(x, y, w, h);
            }

      virtual void processMidi(unsigned int /*curFrame*/ = 0);
      void preProcessAlways();

      // Returns true if the transport source is connected to any of the
      //  track's midi input ports (ex. synth ports not muse midi ports).
      virtual bool usesTransportSource() const
        { if(_sif) return _sif->usesTransportSource(); return false; }
      virtual bool transportAffectsAudioLatency() const
        { if(_sif) return usesTransportSource() && _sif->cquirks()._transportAffectsAudioLatency; return false; }

      // Synth devices can never dominate latency, only physical/hardware midi devices can.
      inline virtual bool canDominateOutputLatencyMidi(bool /*capture*/) const { return false; }
      inline virtual bool canDominateEndPointLatencyMidi(bool /*capture*/) const { return false; }
      inline virtual bool canCorrectOutputLatencyMidi() const { return false; }
      virtual bool isLatencyInputTerminalMidi(bool capture);
      virtual bool isLatencyOutputTerminalMidi(bool capture);
      virtual TrackLatencyInfo& getDominanceInfoMidi(bool capture, bool input);
      virtual TrackLatencyInfo& getDominanceLatencyInfoMidi(bool capture, bool input);
      virtual TrackLatencyInfo& setCorrectionLatencyInfoMidi(bool capture, bool input, float finalWorstLatency, float callerBranchLatency = 0.0f);
      virtual TrackLatencyInfo& getLatencyInfoMidi(bool capture, bool input);
      virtual unsigned long latencyCompWriteOffsetMidi(bool capture) const;
      virtual void setLatencyCompWriteOffsetMidi(float worstCase, bool capture);

      // The cached worst latency of all the channels in the track's effect rack plus any synthesizer latency if applicable.
      virtual float getWorstPluginLatencyAudio();
      // Synth devices can never dominate latency, only physical/hardware midi devices can.
      inline virtual bool canDominateOutputLatency() const { return false; }
      inline virtual bool canDominateEndPointLatency() const { return false; }
      inline virtual bool canCorrectOutputLatency() const { return false; }
      virtual bool isLatencyInputTerminal();
      virtual bool isLatencyOutputTerminal();
      virtual TrackLatencyInfo& getDominanceInfo(bool input);
      virtual TrackLatencyInfo& getDominanceLatencyInfo(bool input);
      virtual TrackLatencyInfo& setCorrectionLatencyInfo(bool input, float finalWorstLatency, float callerBranchLatency = 0.0f);
      virtual TrackLatencyInfo& getLatencyInfo(bool input);
      
      inline MidiPlayEvent receiveEvent() { if(!_sif) return MidiPlayEvent(); else return _sif->receiveEvent(); }
      inline int eventsPending() const    { if(!_sif) return 0; else return _sif->eventsPending(); }
      void deactivate2();
      void deactivate3();
      inline virtual bool hasAuxSend() const  { return true; }
      static void setVisible(bool t) { _isVisible = t; }
      virtual int height() const;
      static bool visible() { return _isVisible; }

      unsigned long latencyOutPortIndex() const;
      unsigned long freewheelPortIndex() const;
      unsigned long enableOrBypassPortIndex() const;
      MusEPlugin::PluginLatencyReportingType pluginLatencyReportingType() const;
      MusEPlugin::PluginBypassType pluginBypassType() const;
      MusEPlugin::PluginFreewheelType pluginFreewheelType() const;
      // Returns the plugin latency, if it has such as function.
      // NOTE: If the plugin has a latency controller out, use that instead.
      virtual float getPluginLatency(void* /*handle*/);
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
      int oldMidiStateHeader(const unsigned char** data) const;

      void guiHeartBeat();
      inline bool guiVisible() const { return false; }
      inline bool hasGui() const     { return false; }
      bool nativeGuiVisible() const;
      void showNativeGui(bool v);
      bool hasNativeGui() const;
      void getNativeGeometry(int*, int*, int*, int*) const;
      void setNativeGeometry(int, int, int, int);
      void updateNativeGuiWindowTitle();
      void preProcessAlways();
      bool getData(MidiPort*, unsigned pos, int ports, unsigned n, float** buffer);
      MidiPlayEvent receiveEvent();
      int eventsPending() const;
      bool init(MessSynth* s);

      int channels() const;
      int totalOutChannels() const;
      int totalInChannels() const;
      void deactivate3();
      QString getPatchName(int, int, bool) const;
      void populatePatchPopup(MusEGui::PopupMenu*, int, bool);
      void write(int level, Xml& xml) const;
      inline double getParameter(unsigned long) const { return 0.0; }
      void setParameter(unsigned long, double) {}
      int getControllerInfo(int id, QString* name, int* ctrl, int* min, int* max, int* initval);
      // Returns true if a note name list is found for the given patch.
      // If true, name either contains the note name, or is blank if no note name was found.
      // drum = Want percussion names, not melodic.
      bool getNoteSampleName(
        bool drum, int channel, int patch, int note, QString* name) const;
      };

// NOTE: Moved here from track.h to avoid circular dependency.
typedef tracklist<SynthI*>::iterator iSynthI;
typedef tracklist<SynthI*>::const_iterator ciSynthI;
typedef tracklist<SynthI*> SynthIList;

typedef std::vector<MusECore::Synth*>::iterator iSynthList;
typedef std::vector<MusECore::Synth*>::const_iterator ciSynthList;
class SynthList : public std::vector<MusECore::Synth*>
{
  public:
    SynthList() { }
    // Each argument optional, can be empty.
    // If uri is not empty, file and label are ignored.
    // If useFileBaseName is true, fileCompleteBaseName will be compared with the synth's base name instead of
    //  the complete base name. (For legacy song files earlier than version 4, which only stored the base name.)
    Synth* find(
      MusEPlugin::PluginTypes_t pluginTypes,
      const QString& fileCompleteBaseName,
      const QString& pluginUri,
      const QString& pluginLabel,
      bool useFileBaseName = false) const;
};

} // namespace MusECore

namespace MusEGlobal {
extern MusECore::SynthList synthis;  // array of available synthis
}

#endif

