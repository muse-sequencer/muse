//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: track.h,v 1.39.2.17 2009/12/20 05:00:35 terminator356 Exp $
//
//  (C) Copyright 1999-2004 Werner Schweer (ws@seh.de)
//  (C) Copyright 2011-2013, 2016 Tim E. Real (terminator356 on sourceforge)
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

#ifndef __TRACK_H__
#define __TRACK_H__

#include <QString>

#include <vector>
#include <algorithm>

#include "wave.h" // for SndFileR
#include "part.h"
#include "mpevent.h"
#include "key.h"
#include "node.h"
#include "route.h"
#include "ctrl.h"
#include "globaldefs.h"
#include "cleftypes.h"
#include "controlfifo.h"

class QPixmap;
class QColor;

namespace MusECore {
class Pipeline;
class PluginI;
class SynthI;
class Xml;
struct DrumMap;
struct ControlEvent;
struct Port;
class PendingOperationList;
class Undo;
class WorkingDrumMapList;
class WorkingDrumMapPatchList;
struct MidiCtrlValRemapOperation;
class LatencyCompensator;

// During latency computations each process cycle,
//  this holds cached computed latency values.
struct TrackLatencyInfo
{
  // Whether this information is valid (has already
  //  been gathered in the current process cycle).
  // This is reset near the beginning of the process handler.
  bool _processed;
  // Whether this information is valid (has already been gathered
  //  in the forward latency scan in the current process cycle).
  // This is reset near the beginning of the process handler.
  bool _forwardProcessed;
  // Contributions to latency from rack plugins and/or Jack ports etc.
  // This value is the worst-case latency of all the channels in a track.
  // See AudioTrack::trackLatency().
  float _trackLatency;
  float _forwardTrackLatency;
  // The absolute latency of all signals leaving a track, relative to audio driver frame (transport, etc).
  // This value is the cumulative value of all series routes connected to this track, plus some
  //  adjustment for the track's own members' latency.
  // The goal is to have equal latency output on all channels.
  // Thus the value will be the WORST-CASE latency of any channel. All other channels are delayed to match it.
  // For example, a Wave Track can use this total value to appropriately shift recordings of the signals
  //  arriving at its inputs.
  float _outputLatency;
  float _forwardOutputLatency;
  // Maximum amount of latency that this track's input can correct.
  float _inputAvailableCorrection;
  float _forwardInputAvailableCorrection;
  // Maximum amount of latency that this track's output can correct.
  float _outputAvailableCorrection;
  float _forwardOutputAvailableCorrection;
};

// Default available wave track latency corrections. Just arbitrarily large values. 
// A track may supply something different (default for others is 0).
#define DEFAULT_WAVETRACK_IN_LATENCY_CORRECTION 4194304
#define DEFAULT_WAVETRACK_OUT_LATENCY_CORRECTION 4194304

typedef std::vector<double> AuxSendValueList;
typedef std::vector<double>::iterator iAuxSendValue;

//---------------------------------------------------------
//   Track
//---------------------------------------------------------

class Track {
   public:
      enum TrackType {
         MIDI=0, DRUM, NEW_DRUM, WAVE, AUDIO_OUTPUT, AUDIO_INPUT, AUDIO_GROUP,
         AUDIO_AUX, AUDIO_SOFTSYNTH
         };
      // NOTE: ASSIGN_DUPLICATE_PARTS ASSIGN_COPY_PARTS and ASSIGN_CLONE_PARTS are not allowed together - choose one. 
      // (Safe, but it will choose one action over the other.)
      enum AssignFlags {
         ASSIGN_PROPERTIES=1, 
         ASSIGN_DUPLICATE_PARTS=2, ASSIGN_COPY_PARTS=4, ASSIGN_CLONE_PARTS=8, 
         ASSIGN_PLUGINS=16, 
         ASSIGN_STD_CTRLS=32, ASSIGN_PLUGIN_CTRLS=64, 
         ASSIGN_ROUTES=128, ASSIGN_DEFAULT_ROUTES=256, 
         ASSIGN_DRUMLIST=512 };
         
   private:
      TrackType _type;
      QString _comment;

      PartList _parts;

      void init();
      void internal_assign(const Track&, int flags);

   protected:
      static unsigned int _soloRefCnt;
      static Track* _tmpSoloChainTrack;
      static bool _tmpSoloChainDoIns;
      static bool _tmpSoloChainNoDec;
      
      // Every time a track is selected, the track's _selectionOrder is set to this value,
      //  and then this value is incremented. The value is reset to zero occasionally, 
      //  for example whenever Song::selectAllTracks(false) is called.
      static int _selectionOrderCounter;
      
      RouteList _inRoutes;
      RouteList _outRoutes;
      bool _nodeTraversed;   // Internal anti circular route traversal flag.
      int _auxRouteCount;    // Number of aux paths feeding this track.
      
      QString _name;
      bool _recordFlag;
      bool _recMonitor;       // For midi and audio. Whether to pass the input through to output.
      bool _mute;
      bool _solo;
      unsigned int _internalSolo;
      bool _off;
      int _channels;          // 1 - mono, 2 - stereo

      int _activity;
      int _lastActivity;
      double _meter[MAX_CHANNELS];
      double _peak[MAX_CHANNELS];
      bool _isClipped[MAX_CHANNELS]; //used in audio mixer strip. Persistent.

      int _y;
      int _height;            // visual height in arranger

      bool _locked;
      bool _selected;
      // The selection order of this track, compared to other selected tracks.
      // The selected track with the highest selected order is the most recent selected.
      int _selectionOrder;

      // Holds latency computations each cycle.
      TrackLatencyInfo _latencyInfo;
      
      bool readProperties(Xml& xml, const QString& tag);
      void writeProperties(int level, Xml& xml) const;

   public:
      Track(TrackType);
      Track(const Track&, int flags);
      virtual ~Track();
      virtual void assign(const Track&, int flags);
      
      static const char* _cname[];
      static QPixmap* trackTypeIcon(TrackType);
      static QColor trackTypeColor(TrackType);
      static QColor trackTypeLabelColor(TrackType);
      QPixmap* icon() const { return trackTypeIcon(type()); }
      QColor color() const { return trackTypeColor(type()); }
      QColor labelColor() const { return trackTypeLabelColor(type()); }

      QString comment() const         { return _comment; }
      void setComment(const QString& s) { _comment = s; }

      int y() const;
      void setY(int n)                { _y = n;         }
      virtual int height() const = 0;
      void setHeight(int n)           { _height = n;    }

      bool selected() const           { return _selected; }
      // Try to always call this instead of setting _selected, because it also sets _selectionOrder.
      void setSelected(bool f);
      // The order of selection of this track, compared to other selected tracks. 
      // The selected track with the highest selected order is the most recent selected.
      int selectionOrder() const       { return _selectionOrder; }
      // Resets the static selection counter. Optional. (Range is huge, unlikely would have to call). 
      // Called for example whenever Song::selectAllTracks(false) is called.
      static void clearSelectionOrderCounter(){ _selectionOrderCounter = 0; }

      bool locked() const             { return _locked; }
      void setLocked(bool b)          { _locked = b; }

      void clearRecAutomation(bool clearList);
      
      const QString& name() const     { return _name; }
      // setName can be overloaded to do other things like setting port names, while setNameText just sets the text.
      virtual void setName(const QString& s)  { _name = s; }
      // setNameText just sets the text, while setName can be overloaded to do other things like setting port names.
      void setNameText(const QString& s)  { _name = s; }

      TrackType type() const          { return _type; }
      void setType(TrackType t)       { _type = t; }
      QString cname() const           { int t = type(); return QString(_cname[t]); }

      // routing
      RouteList* inRoutes()    { return &_inRoutes; }
      RouteList* outRoutes()   { return &_outRoutes; }
      const RouteList* inRoutes() const { return &_inRoutes; }
      const RouteList* outRoutes() const { return &_outRoutes; }
      virtual bool noInRoute() const   { return _inRoutes.empty();  }
      virtual bool noOutRoute() const  { return _outRoutes.empty(); }
      void writeRouting(int, Xml&) const;
      bool isCircularRoute(Track* dst);   
      int auxRefCount() const { return _auxRouteCount; }  // Number of Aux Tracks with routing paths to this track. 
      void updateAuxRoute(int refInc, Track* dst);  // Internal use. 
      // Number of routable inputs/outputs for each Route::RouteType.
      virtual RouteCapabilitiesStruct routeCapabilities() const;
      
      PartList* parts()               { return &_parts; }
      const PartList* cparts() const  { return &_parts; }
      Part* findPart(unsigned tick);
      iPart addPart(Part* p);

      virtual void write(int, Xml&) const = 0;

      virtual Track* newTrack() const = 0;
      virtual Track* clone(int flags) const    = 0;
      // Returns true if any event in any part was opened. Does not operate on the part's clones, if any.
      virtual bool openAllParts() { return false; };
      // Returns true if any event in any part was closed. Does not operate on the part's clones, if any.
      virtual bool closeAllParts() { return false; };

      // Called from gui thread only.
      virtual bool setRecordFlag1(bool) = 0;
      // Called from audio thread only.
      virtual void setRecordFlag2(bool) = 0;
      // Same as setRecordFlag2 except it can automatically set the monitor flag
      //  depending on the global config.monitorOnRecord. Called from audio thread only.
      // Returns whether the monitor was changed.
      virtual bool setRecordFlag2AndCheckMonitor(bool r) = 0;

      virtual Part* newPart(Part*p=0, bool clone = false) = 0;
      void dump() const;

      virtual void setMute(bool val) { _mute = val; }
      virtual void setOff(bool val) { _off = val; }
      void setInternalSolo(unsigned int val);
      virtual void setSolo(bool val) = 0;

      // Returns true if playback ultimately is muted, depending on
      //  other factors such as soloing.
      virtual bool isMute() const {
        if(_solo || (_internalSolo && !_mute))
          return false;
        if(_soloRefCnt)
          return true;
        return _mute;
      }
      // Returns true if playback ultimately is monitored, depending on
      //  other factors such as soloing.
      virtual bool isRecMonitored() const {
        if(_off || !_recMonitor)
          return false;
        if(_solo || _internalSolo)
          return true;
        return _soloRefCnt == 0;
      }
      // Returns true (>= 1) if proxy-soloed.
      virtual unsigned int internalSolo() const  { return _internalSolo; }
      // Returns true if proxy-muted.
      virtual bool soloMode() const      { return _soloRefCnt; }
      // Returns true if soloed.
      virtual bool solo() const          { return _solo;         }
      // Returns true if muted.
      virtual bool mute() const          { return _mute;         }
      // Returns true if track is off.
      virtual bool off() const           { return _off;          }
      // Returns true if rec-armed.
      virtual bool recordFlag() const    { return _recordFlag;   }
      // Sets monitor.
      virtual void setRecMonitor(bool b) { if(canRecordMonitor()) _recMonitor = b; }
      // Returns true if monitored.
      virtual bool recMonitor() const    { return _recMonitor; }

      // The amount that this track type can CORRECT for input latency (not just COMPENSATE for it).
      virtual float inputLatencyCorrection() const { return 0.0f; }
      // The amount that this track type can CORRECT for output latency (not just COMPENSATE for it).
      virtual float outputLatencyCorrection() const { return 0.0f; }
      
      // Internal use...
      static void clearSoloRefCounts();
      void updateSoloState();
      virtual void updateSoloStates(bool noDec) = 0;  
      virtual void updateInternalSoloStates();        

      int activity()                     { return _activity;     }
      void setActivity(int v)            { _activity = v;        }
      int lastActivity()                 { return _lastActivity; }
      void setLastActivity(int v)        { _lastActivity = v;    }
      void addActivity(int v)            { _activity += v;       }
      void resetPeaks();
      static void resetAllMeter();
      double meter(int ch) const  { return _meter[ch]; }
      double peak(int ch) const   { return _peak[ch]; }
      void resetMeter();

      bool readProperty(Xml& xml, const QString& tag);
      void setDefaultName(QString base = QString());
      int channels() const                { return _channels; }
      virtual void setChannels(int n);
      bool isMidiTrack() const       { return type() == MIDI || type() == DRUM || type() == NEW_DRUM; }
      bool isDrumTrack() const       { return type() == DRUM || type() == NEW_DRUM; }
      bool isSynthTrack() const      { return type() == AUDIO_SOFTSYNTH; }
      virtual bool canRecord() const { return false; }
      virtual bool canRecordMonitor() const { return false; }
      virtual AutomationType automationType() const    = 0;
      virtual void setAutomationType(AutomationType t) = 0;
      static void setVisible(bool) { }
      bool isVisible();
      inline bool isClipped(int ch) const { if(ch >= MAX_CHANNELS) return false; return _isClipped[ch]; }
      void resetClipper() { for(int ch = 0; ch < MAX_CHANNELS; ++ch) _isClipped[ch] = false; }
      };

//---------------------------------------------------------
//   MidiTrack
//---------------------------------------------------------

class MidiTrack : public Track {
      int _outPort;
      int _outChannel;

   private:
      static bool _isVisible;
      clefTypes clefType;

      DrumMap* _drummap; // _drummap[foo].anote is always equal to foo

      // A list of user-altered drum map items.
      WorkingDrumMapPatchList* _workingDrumMapPatchList;

      bool _drummap_ordering_tied_to_patch; //if true, changing patch also changes drummap-ordering
      int drum_in_map[128];
      int _curDrumPatchNumber; // Can be CTRL_VAL_UNKNOWN.
      
      void init();
      void internal_assign(const Track&, int flags);
      void init_drummap(bool write_ordering); // function without argument in public
      void remove_ourselves_from_drum_ordering();
      
      void writeOurDrumSettings(int level, Xml& xml) const;
      void readOurDrumSettings(Xml& xml);

   public:
      EventList events;           // tmp Events during midi import
      MPEventList mpevents;       // tmp Events druring recording
      MPEventList stuckLiveNotes; // Live (rec): Currently sounding note-ons that we don't know the note-off time yet. Event times = 0.
      MPEventList stuckNotes;     // Playback: Currently sounding note-ons contributed by track - not sent directly to device
      
      MidiTrack();
      MidiTrack(const MidiTrack&, int flags);
      virtual ~MidiTrack();

      virtual void assign(const Track&, int flags);
      virtual void convertToType(TrackType trackType);

      virtual AutomationType automationType() const;
      virtual void setAutomationType(AutomationType);

      // play parameter
      int transposition;
      int velocity;
      int delay;
      int len;
      int compression;

      // Called from gui thread only.
      virtual bool setRecordFlag1(bool) { return canRecord(); }
      // Called from audio thread only.
      virtual void setRecordFlag2(bool f) { if(canRecord()) _recordFlag = f; }
      // Same as setRecordFlag2 except it can automatically set the monitor flag
      //  depending on the global config.monitorOnRecord. Called from audio thread only.
      // Returns whether the monitor was changed.
      virtual bool setRecordFlag2AndCheckMonitor(bool);

      virtual void read(Xml&);
      virtual void write(int, Xml&) const;

      virtual int height() const;
      
      virtual MidiTrack* newTrack() const { return new MidiTrack(); }
      virtual MidiTrack* clone(int flags) const { return new MidiTrack(*this, flags); }
      virtual Part* newPart(Part*p=0, bool clone=false);

      // Number of routable inputs/outputs for each Route::RouteType.
      virtual RouteCapabilitiesStruct routeCapabilities() const;
      
      // This enum describes what has changed in the following port/channel methods.
      enum ChangedType { NothingChanged = 0x0, PortChanged = 0x1, ChannelChanged = 0x2, DrumMapChanged = 0x4 };
      // OR'd ChangedType flags.
      typedef int ChangedType_t;
      // Sets the output port, and for a drum track updates any drum map. Returns true if anything changed.
      // If doSignal is true, automatically emits SC_DRUM_MAP or sends audio message if audio is running and not idle.
      ChangedType_t setOutPort(int i, bool doSignal = false);
      // Sets the output channel, and for a drum track updates any drum map. Returns true if anything changed.
      // If doSignal is true, automatically emits SC_DRUM_MAP or sends audio message if audio is running and not idle.
      ChangedType_t setOutChannel(int i, bool doSignal = false);
      // Same as setOutPort, but also transfers controller data to the new selected port.
      ChangedType_t setOutPortAndUpdate(int port, bool doSignal = false);
      // Same as setOutChannel, but also transfers controller data to the new selected channel.
      ChangedType_t setOutChanAndUpdate(int chan, bool doSignal = false);
      // Combines both setOutChannel and setOutPort operations, and transfers controller data to the new selected port/channel.
      ChangedType_t setOutPortAndChannelAndUpdate(int port, int chan, bool doSignal = false);
      
      // Backward compatibility: For reading old songs.
      void setInPortAndChannelMask(unsigned int portmask, int chanmask); 
      
      // Overridden for special midi output behaviour.
      virtual bool noOutRoute() const;
      
      int outPort() const             { return _outPort;     }
      int outChannel() const          { return _outChannel;  }

      virtual void setSolo(bool val);
      virtual void updateSoloStates(bool noDec);
      virtual void updateInternalSoloStates();

      virtual bool addStuckNote(const MidiPlayEvent& ev);
      // These are only for 'live' (rec) notes for which we don't have a note-off time yet. Even times = 0.
      virtual bool addStuckLiveNote(int port, int chan, int note, int vel = 64);
      virtual bool removeStuckLiveNote(int port, int chan, int note);
      virtual bool stuckLiveNoteExists(int port, int chan, int note);

      virtual bool canRecord() const  { return true; }
      virtual bool canRecordMonitor() const { return true; }
      static void setVisible(bool t) { _isVisible = t; }
      static bool visible() { return _isVisible; }
      
      int getFirstControllerValue(int ctrl, int def=-1);
      int getControllerChangeAtTick(unsigned tick, int ctrl, int def=-1);
      unsigned getControllerValueLifetime(unsigned tick, int ctrl); // returns the tick where this CC gets overridden by a new one
                                                                    // returns UINT_MAX for "never"

      void setClef(clefTypes i) { clefType = i; }
      clefTypes getClef() { return clefType; }
      
      DrumMap* drummap() { return _drummap; }
      int map_drum_in(int enote) { return drum_in_map[enote]; }
      void update_drum_in_map();
      void init_drum_ordering();

      void init_drummap() { init_drummap(false); } // function with argument in private
      
      // For drum tracks, updates the drum map and returns true if anything changed.
      // If doSignal is true, automatically emits SC_DRUM_MAP or sends audio message if audio is running and not idle.
      bool updateDrummap(int doSignal);
      //void workingDrumMapOperation(int index, bool updateDruminmap, const WorkingDrumMapEntry& item, PendingOperationList& ops);
      WorkingDrumMapPatchList* workingDrumMap() const { return _workingDrumMapPatchList; }
      void setWorkingDrumMap(WorkingDrumMapPatchList* list, bool isInstrumentMod);
      void modifyWorkingDrumMap(WorkingDrumMapList& list, bool isReset, bool includeDefault, bool isInstrumentMod, bool doWholeMap);
      //void modifyWorkingDrumMap(WorkingDrumMapPatchList& list, bool clear, bool isReset, bool isInstrumentMod);
      // Returns a map item with members filled from either the original or working map item,
      //  depending on which Field flags are set. The returned map includes any requested
      //  WorkingDrumMapEntry::OverrideType track or instrument overrides.
      void getMapItem(int patch, int index, DrumMap& dest_map, int overrideType) const;
      // Returns a map item with members filled from either the original or working map item,
      //  depending on which Field flags are set. The returned map includes any requested
      //  WorkingDrumMapEntry::OverrideType track or instrument overrides.
      // Same as getMapItem(), but determines patch at supplied tick.
      void getMapItemAt(int tick, int index, DrumMap& dest_map, int overrideType) const;
      // Returns OR'd WorkingDrumMapEntry::OverrideType flags indicating whether a map item's members,
      //  given by 'fields' (OR'd WorkingDrumMapEntry::Fields), are either the original or working map item.
      // Here in MidiTrack the flags can be NoOverride, InstrumentOverride, and TrackOverride. See corresponding
      //  function in MidiInstrument. If patch is -1 it uses the track's current patch (midi controller hwCtrlVal).
      int isWorkingMapItem(int index, int fields, int patch = -1) const;

      // Ensures there are NO duplicate enote fields in the final drum map array.
      // Returns true if anything changed.
      bool normalizeDrumMap();
      // Ensures there are NO duplicate enote fields in the final drum map array for the given patch.
      // Returns true if anything changed.
      bool normalizeDrumMap(int patch);

      void set_drummap_ordering_tied_to_patch(bool);
      bool drummap_ordering_tied_to_patch() { return _drummap_ordering_tied_to_patch; }

      void MidiCtrlRemapOperation(int index, int newPort, int newChan, int newNote, MidiCtrlValRemapOperation* rmop);

      // Prints a handy debug table of drum map values and overrides etc.
      void dumpMap();
      };

//---------------------------------------------------------
//   AudioTrack
//    this track can hold audio automation data and can
//    hold tracktypes WAVE, AUDIO_GROUP, AUDIO_OUTPUT,
//    AUDIO_INPUT, AUDIO_SOFTSYNTH, AUDIO_AUX
//---------------------------------------------------------

class AudioTrack : public Track {
      bool _haveData; // Whether we have data from a previous process call during current cycle.
      
      CtrlListList _controller;   // Holds all controllers including internal, plugin and synth.
      ControlFifo _controlFifo;   // For internal controllers like volume and pan. Plugins/synths have their own.
      CtrlRecList _recEvents;     // recorded automation events

      unsigned long _controlPorts;
      Port* _controls;             // For internal controllers like volume and pan. Plugins/synths have their own.

      double _curVolume;
      double _curVol1;
      double _curVol2;
      
      bool _prefader;               // prefader metering
      AuxSendValueList _auxSend;
      void readAuxSend(Xml& xml);
      int recFileNumber;
      
      bool _sendMetronome;
      AutomationType _automationType;
      double _gain;

      void initBuffers();
      void internal_assign(const Track&, int flags);
      void processTrackCtrls(unsigned pos, int trackChans, unsigned nframes, float** buffer);

   protected:
      // Cached audio data for all channels. If prefader is not on, the first two channels
      //  have volume and pan applied if track is stereo, or the first channel has just
      //  volume applied if track is mono.
      float** outBuffers;
      // Extra cached audio data.
      float** outBuffersExtraMix;
      // Just all zeros all the time, so we don't have to clear for silence.
      float*  audioInSilenceBuf;
      // Just a place to connect all unused audio outputs.
      float*  audioOutDummyBuf;
      // Internal temporary buffers for getData().
      float** _dataBuffers;
      // REMOVE Tim. latency. Added.
      // Audio latency compensator.
      LatencyCompensator* _latencyComp;
      // Used during latency compensation processing.
      unsigned long _latCompWriteOffset;

      // These two are not the same as the number of track channels which is always either 1 (mono) or 2 (stereo):
      // Total number of output channels.
      int _totalOutChannels;
      // Total number of input channels.
      int _totalInChannels;
      
      Pipeline* _efxPipe;

      virtual bool getData(unsigned, int, unsigned, float**);

      SndFileR _recFile;
      Fifo fifo;                    // fifo -> _recFile
      bool _processed;
      
   public:
      AudioTrack(TrackType t);
      
      AudioTrack(const AudioTrack&, int flags);
      virtual ~AudioTrack();

      virtual void assign(const Track&, int flags);
      
      virtual AudioTrack* clone(int flags) const = 0;
      virtual Part* newPart(Part*p=0, bool clone=false);

      // Called from gui thread only.
      virtual bool setRecordFlag1(bool);
      // Called from audio thread only.
      virtual void setRecordFlag2(bool);
      bool prepareRecording();
      // Same as setRecordFlag2 except it can automatically set the monitor flag
      //  depending on the global config.monitorOnRecord. Called from audio thread only.
      // Returns whether the monitor was changed.
      virtual bool setRecordFlag2AndCheckMonitor(bool);

      bool processed() { return _processed; }

      void addController(CtrlList*);
      void removeController(int id);
      void swapControllerIDX(int idx1, int idx2);

      bool readProperties(Xml&, const QString&);
      void writeProperties(int, Xml&) const;

      void mapRackPluginsToControllers();
      void showPendingPluginNativeGuis();

      SndFileR recFile() const           { return _recFile; }
      void setRecFile(SndFileR sf)       { _recFile = sf; }

      CtrlListList* controller()         { return &_controller; }
      // For setting/getting the _controls 'port' values.
      unsigned long parameters() const { return _controlPorts; }
      
      void setParam(unsigned long i, double val); 
      double param(unsigned long i) const;

      virtual void setChannels(int n);
      virtual void setTotalOutChannels(int num);
      virtual int totalOutChannels() const { return _totalOutChannels; }
      virtual void setTotalInChannels(int num);
      virtual int totalInChannels() const { return _totalInChannels; }
      // Number of routable inputs/outputs for each Route::RouteType.
      virtual RouteCapabilitiesStruct routeCapabilities() const;
      // Number of required processing buffers.
      virtual int totalProcessBuffers() const { return (channels() == 1) ? 1 : totalOutChannels(); }

      virtual void setSolo(bool val);
      virtual void updateSoloStates(bool noDec);
      virtual void updateInternalSoloStates();
      
      // Puts to the recording fifo.
// REMOVE Tim. latency. Changed.
//       void putFifo(int channels, unsigned long n, float** bp);
      // This also performs adjustments for latency compensation before putting to the fifo.
      // Returns true on success.
      bool putFifo(int channels, unsigned long n, float** bp);
      // Transfers the recording fifo to _recFile.
      void record();
      // Returns the recording fifo current count.
      int recordFifoCount() { return fifo.getCount(); }

      virtual void setMute(bool val);
      virtual void setOff(bool val);

      void setSendMetronome(bool val) { _sendMetronome = val; }
      bool sendMetronome() const { return _sendMetronome; }
      
      double volume() const;
      void setVolume(double val);
      double pan() const;
      void setPan(double val);
      double gain() const;
      void setGain(double val);

      bool prefader() const              { return _prefader; }
      double auxSend(int idx) const;
      void setAuxSend(int idx, double v);
      void addAuxSend(int n);
      void addAuxSendOperation(int n, PendingOperationList& ops);

      void setPrefader(bool val);
      Pipeline* efxPipe()                { return _efxPipe;  }
      void deleteAllEfxGuis();
      void clearEfxList();
      // Removes any existing plugin and inserts plugin into effects rack, and calls setupPlugin.
      void addPlugin(PluginI* plugin, int idx);
      // Assigns valid ID and track to plugin, and creates controllers for plugin.
      void setupPlugin(PluginI* plugin, int idx);

      double pluginCtrlVal(int ctlID) const;
      void setPluginCtrlVal(int param, double val);
      
      void readVolume(Xml& xml);

// REMOVE Tim. latency. Changed.
//       virtual void preProcessAlways() { _processed = false; }
      virtual void preProcessAlways() { 
        _processed = false; 
        _latencyInfo._processed = false;
        _latencyInfo._forwardProcessed = false;
        _latCompWriteOffset = 0;
      }
      // Gathers this track's audio data and either copies or adds it to a supplied destination buffer.
      // If the per-channel 'addArray' is supplied, whether to copy or add each channel is given in the array,
      //  otherwise it is given by the bulk 'add' flag.
      // The range of buffers in 'dstBuffer' given by 'dstStartChan' and 'availDstChannels' are filled with data.
      // If 'availDstChannels' is greater than 'requestedDstChannels', the excess buffers are filled with silence.
      // If 'requestedDstChannels' is greater than 'availDstChannels', copyData() acts AS IF 'requestedDstChannels'
      //  was the real availDstChannels except it only fills up to 'availDstChannels'. This is to allow copyData()
      //  to mix properly even when some routes are not available (ex. switching a track from stereo to mono, 
      //  which has an existing one-channel route on the right channel).
      // The 'srcStartChan' and 'srcChannels' give the range of channels to copy or add from this track.
      // If 'srcStartChan' is -1 it will be set to zero. If 'srcChannels' is -1`it will be set to this track's output channels. 
      // The 'dstStartChan' can also be -1, but 'requestedDstChannels' and availDstChannels cannot.
      virtual void copyData(unsigned samplePos, 
                            int dstStartChan, int requestedDstChannels, int availDstChannels,
                            int srcStartChan, int srcChannels, 
                            unsigned frames, float** dstBuffer, 
                            bool add = false,
                            const bool* addArray = 0);
      
      virtual bool hasAuxSend() const { return false; }

      // The contribution to latency by the track's own members (audio effect rack, etc).
      virtual float trackLatency(int channel) const;
// REMOVE Tim. latency. Added.
      // The absolute latency of all signals leaving this track, relative to audio driver frame (transport, etc).
      // This value is the cumulative value of all series routes connected to this track, plus some
      //  adjustment for the track's own members' latency.
      // The goal is to have equal latency output on all channels. Thus there is no 'channel' parameter.
      // The value will be the WORST-CASE latency of any channel. All other channels are delayed to match it.
      // For example, a Wave Track can use this total value to appropriately shift recordings of the signals
      //  arriving at its inputs.
      //virtual float outputLatency(); // const; 
      
// REMOVE Tim. latency. Added.
      // Returns latency computations during each cycle. If the computations have already been done 
      //  this cycle, cached values are returned, otherwise they are computed, cached, then returned.
      virtual TrackLatencyInfo getLatencyInfo();
      // Returns forward latency computations (from wavetracks outward) during each cycle.
      // If the computations have already been done this cycle, cached values are returned,
      //  otherwise they are computed, cached, then returned.
      virtual TrackLatencyInfo getForwardLatencyInfo();
      //
      // Used during latency compensation processing. When analyzing in 'reverse' this mechansim is
      //  needed only to equalize the timing of all the AudioOutput tracks.
      // It is applied as a direct offset in the latency delay compensator in getData().
      virtual unsigned long latencyCompWriteOffset() const { return _latCompWriteOffset; }
      virtual void setLatencyCompWriteOffset(unsigned long off) { _latCompWriteOffset = off; }
      
      // automation
      virtual AutomationType automationType() const    { return _automationType; }
      virtual void setAutomationType(AutomationType t);
      // Fills operations if given, otherwise creates and executes its own operations list.
      void processAutomationEvents(Undo* operations = 0);
      CtrlRecList* recEvents()                         { return &_recEvents; }
      bool addScheduledControlEvent(int track_ctrl_id, double val, unsigned frame); // return true if event cannot be delivered
      void enableController(int track_ctrl_id, bool en);
      bool controllerEnabled(int track_ctrl_id) const;
      // Enable all track and plugin controllers, and synth controllers if applicable.
      void enableAllControllers();
      void recordAutomation(int n, double v);
      void startAutoRecord(int, double);
      void stopAutoRecord(int, double);
      void setControllerMode(int, CtrlList::Mode m); 
      void clearControllerEvents(int);
      void seekPrevACEvent(int);
      void seekNextACEvent(int);
      void eraseACEvent(int, int);
      void eraseRangeACEvents(int, int, int);
      void addACEvent(int, int, double);
      void changeACEvent(int id, int frame, int newframe, double newval);
      const AuxSendValueList &getAuxSendValueList() { return _auxSend; }      
      };

//---------------------------------------------------------
//   AudioInput
//---------------------------------------------------------

class AudioInput : public AudioTrack {
      void* jackPorts[MAX_CHANNELS];
      bool getData(unsigned, int, unsigned, float**);
      static bool _isVisible;
      void internal_assign(const Track& t, int flags);
      
   public:
      AudioInput();
      AudioInput(const AudioInput&, int flags);
      virtual ~AudioInput();

      float trackLatency(int channel) const; 
// REMOVE Tim. latency. Added.
//       float outputLatency() const; 
      void assign(const Track&, int flags);
      AudioInput* clone(int flags) const { return new AudioInput(*this, flags); }
      AudioInput* newTrack() const { return new AudioInput(); }
      void read(Xml&);
      void write(int, Xml&) const;
      void setName(const QString& s);
      void* jackPort(int channel) { return jackPorts[channel]; }
      void setJackPort(int channel, void*p) { jackPorts[channel] = p; }
      void setChannels(int n);
      bool hasAuxSend() const { return true; }
      // Number of routable inputs/outputs for each Route::RouteType.
      RouteCapabilitiesStruct routeCapabilities() const;
      static void setVisible(bool t) { _isVisible = t; }
      int height() const;
      static bool visible() { return _isVisible; }
    };

//---------------------------------------------------------
//   AudioOutput
//---------------------------------------------------------

class AudioOutput : public AudioTrack {
      void* jackPorts[MAX_CHANNELS];
      float* buffer[MAX_CHANNELS];
      float* buffer1[MAX_CHANNELS];
      unsigned long _nframes;
      static bool _isVisible;
      void internal_assign(const Track& t, int flags);

   public:
      AudioOutput();
      AudioOutput(const AudioOutput&, int flags);
      virtual ~AudioOutput();

      virtual float trackLatency(int channel) const;
      virtual void assign(const Track&, int flags);
      AudioOutput* clone(int flags) const { return new AudioOutput(*this, flags); }
      virtual AudioOutput* newTrack() const { return new AudioOutput(); }
      virtual void read(Xml&);
      virtual void write(int, Xml&) const;
      virtual void setName(const QString& s);
      void* jackPort(int channel) { return jackPorts[channel]; }
      void setJackPort(int channel, void*p) { jackPorts[channel] = p; }
      virtual void setChannels(int n);
      // Number of routable inputs/outputs for each Route::RouteType.
      virtual RouteCapabilitiesStruct routeCapabilities() const;
      void processInit(unsigned);
      void process(unsigned pos, unsigned offset, unsigned);
      void processWrite();
      void silence(unsigned);
      virtual bool canRecord() const { return true; }

      static void setVisible(bool t) { _isVisible = t; }
      static bool visible() { return _isVisible; }
      virtual int height() const;
    };

//---------------------------------------------------------
//   AudioGroup
//---------------------------------------------------------

class AudioGroup : public AudioTrack {
      static bool _isVisible;
   public:
      AudioGroup() : AudioTrack(AUDIO_GROUP) {  }
      AudioGroup(const AudioGroup& t, int flags) : AudioTrack(t, flags) { }
      
      AudioGroup* clone(int flags) const { return new AudioGroup(*this, flags); }
      virtual AudioGroup* newTrack() const { return new AudioGroup(); }
      virtual void read(Xml&);
      virtual void write(int, Xml&) const;
      virtual bool hasAuxSend() const { return true; }
      static  void setVisible(bool t) { _isVisible = t; }
      virtual int height() const;
      static bool visible() { return _isVisible; }
    };

//---------------------------------------------------------
//   AudioAux
//---------------------------------------------------------

class AudioAux : public AudioTrack {
      float* buffer[MAX_CHANNELS];
      static bool _isVisible;
      int _index;
   public:
      AudioAux();
      AudioAux(const AudioAux& t, int flags);
      
      AudioAux* clone(int flags) const { return new AudioAux(*this, flags); }
      ~AudioAux();
      virtual AudioAux* newTrack() const { return new AudioAux(); }
      virtual void read(Xml&);
      virtual void write(int, Xml&) const;
      virtual bool getData(unsigned, int, unsigned, float**);
      virtual void setChannels(int n);
      // Number of routable inputs/outputs for each Route::RouteType.
      virtual RouteCapabilitiesStruct routeCapabilities() const {
                RouteCapabilitiesStruct s = AudioTrack::routeCapabilities();
                s._trackChannels._inRoutable = false;
                s._trackChannels._inChannels = 0;
                return s; }
      float** sendBuffer() { return buffer; }
      static  void setVisible(bool t) { _isVisible = t; }
      virtual int height() const;
      static bool visible() { return _isVisible; }
      virtual QString auxName();
      virtual int index() { return _index; }
    };


//---------------------------------------------------------
//   WaveTrack
//---------------------------------------------------------

class WaveTrack : public AudioTrack {
      Fifo _prefetchFifo;  // prefetch Fifo
      static bool _isVisible;

      void internal_assign(const Track&, int flags);
      bool getDataPrivate(unsigned, int, unsigned, float**);
      
   public:

      WaveTrack();
      WaveTrack(const WaveTrack& wt, int flags);

      virtual void assign(const Track&, int flags);
      
      virtual WaveTrack* clone(int flags) const    { return new WaveTrack(*this, flags); }
      virtual WaveTrack* newTrack() const { return new WaveTrack(); }
      virtual Part* newPart(Part*p=0, bool clone=false);
      // Returns true if any event in any part was opened. Does not operate on the part's clones, if any.
      bool openAllParts();
      // Returns true if any event in any part was closed. Does not operate on the part's clones, if any.
      bool closeAllParts();

      virtual void read(Xml&);
      virtual void write(int, Xml&) const;

      // If overwrite is true, copies the data. If false, adds the data.
      virtual void fetchData(unsigned pos, unsigned frames, float** bp, bool doSeek, bool overwrite);
      
      virtual bool getData(unsigned, int ch, unsigned, float** bp);

// REMOVE Tim. latency. Added.
      // Returns latency computations during each cycle. If the computations have already been done 
      //  this cycle, cached values are returned, otherwise they are computed, cached, then returned.
      TrackLatencyInfo getLatencyInfo();
      // Returns forward latency computations (from wavetracks outward) during each cycle.
      // If the computations have already been done this cycle, cached values are returned,
      //  otherwise they are computed, cached, then returned.
      TrackLatencyInfo getForwardLatencyInfo();
      // The amount that this track type can CORRECT for input latency (not just COMPENSATE for it).
      float inputLatencyCorrection() const { return DEFAULT_WAVETRACK_IN_LATENCY_CORRECTION; }
      // The amount that this track type can CORRECT for output latency (not just COMPENSATE for it).
      float outputLatencyCorrection() const { return DEFAULT_WAVETRACK_OUT_LATENCY_CORRECTION; }
      
      void clearPrefetchFifo()      { _prefetchFifo.clear(); }
      Fifo* prefetchFifo()          { return &_prefetchFifo; }
      virtual void setChannels(int n);
      virtual bool hasAuxSend() const { return true; }
      bool canEnableRecord() const;
      virtual bool canRecord() const { return true; }
      virtual bool canRecordMonitor() const { return true; }
      static void setVisible(bool t) { _isVisible = t; }
      virtual int height() const;
      static bool visible() { return _isVisible; }
    };

//---------------------------------------------------------
//   TrackList
//---------------------------------------------------------

template<class T> class tracklist : public std::vector<Track*> {
      typedef std::vector<Track*> vlist;

   public:
      class iterator : public vlist::iterator {
         public:
            iterator() : vlist::iterator() {}
            iterator(vlist::iterator i) : vlist::iterator(i) {}

            T operator*() {
                  return (T)(**((vlist::iterator*)this));
                  }
            iterator operator++(int) {
                  return iterator ((*(vlist::iterator*)this).operator++(0));
                  }
            iterator& operator++() {
                  return (iterator&) ((*(vlist::iterator*)this).operator++());
                  }
            };

      class const_iterator : public vlist::const_iterator {
         public:
            const_iterator() : vlist::const_iterator() {}
            const_iterator(vlist::const_iterator i) : vlist::const_iterator(i) {}
            const_iterator(vlist::iterator i) : vlist::const_iterator(i) {}

            const T operator*() const {
                  return (T)(**((vlist::const_iterator*)this));
                  }
            };

      class reverse_iterator : public vlist::reverse_iterator {
         public:
            reverse_iterator() : vlist::reverse_iterator() {}
            reverse_iterator(vlist::reverse_iterator i) : vlist::reverse_iterator(i) {}

            T operator*() {
                  return (T)(**((vlist::reverse_iterator*)this));
                  }
            };

      tracklist() : vlist() {}
      virtual ~tracklist() {}

      void push_back(T v)             { vlist::push_back(v); }
      iterator begin()                { return vlist::begin(); }
      iterator end()                  { return vlist::end(); }
      const_iterator begin() const    { return vlist::begin(); }
      const_iterator end() const      { return vlist::end(); }
      reverse_iterator rbegin()       { return vlist::rbegin(); }
      reverse_iterator rend()         { return vlist::rend(); }
      T& back() const                 { return (T&)(vlist::back()); }
      T& front() const                { return (T&)(vlist::front()); }
      iterator find(const Track* t)       {
            return std::find(begin(), end(), t);
            }
      const_iterator find(const Track* t) const {
            return std::find(begin(), end(), t);
            }
      bool contains(const Track* t) const {
            return std::find(begin(), end(), t) != end();
            }
      unsigned index(const Track* t) const {
            unsigned n = 0;
            for (vlist::const_iterator i = begin(); i != end(); ++i, ++n) {
                  if (*i == t)
                        return n;
                  }
            return -1;
            }
      T index(int k) const           { return (*this)[k]; }
      iterator index2iterator(int k) {
            if ((unsigned)k >= size())
                  return end();
            return begin() + k;
            }
      void erase(Track* t)           { vlist::erase(find(t)); }

      void clearDelete() {
            for (vlist::iterator i = begin(); i != end(); ++i)
                  delete *i;
            vlist::clear();
            }
      void erase(vlist::iterator i) { vlist::erase(i); }
      void replace(Track* ot, Track* nt) {
            for (vlist::iterator i = begin(); i != end(); ++i) {
                  if (*i == ot) {
                        *i = nt;
                        return;
                        }
                  }
            }
      // Returns the number of selected tracks in this list.
      int countSelected() const {
            int c = 0;
            for (vlist::const_iterator i = begin(); i != end(); ++i) {
                  if ((*i)->selected()) {
                        ++c;
                        }
                  }
            return c;
            }
      // Returns the current (most recent) selected track, or null if none.
      // It returns the track with the highest _selectionOrder.
      // This helps with multi-selection common-property editing.
      T currentSelection() const {
            T cur = 0;
            int c = 0;
            int so;
            for (vlist::const_iterator i = begin(); i != end(); ++i) {
                  T t = *i;
                  so = t->selectionOrder();
                  if (t->selected() && so >= c) {
                        cur = t;
                        c = so;
                        }
                  }
            return cur;
            }
      // Selects or unselects all tracks in this list.
      void selectAll(bool select) {
            for (vlist::iterator i = begin(); i != end(); ++i) {
                  (*i)->setSelected(select);
                  }
            }
      };

typedef tracklist<Track*> TrackList;
typedef TrackList::iterator iTrack;
typedef TrackList::const_iterator ciTrack;

typedef tracklist<MidiTrack*>::iterator iMidiTrack;
typedef tracklist<MidiTrack*>::const_iterator ciMidiTrack;
typedef tracklist<MidiTrack*> MidiTrackList;

typedef tracklist<WaveTrack*>::iterator iWaveTrack;
typedef tracklist<WaveTrack*>::const_iterator ciWaveTrack;
typedef tracklist<WaveTrack*> WaveTrackList;

typedef tracklist<AudioInput*>::iterator iAudioInput;
typedef tracklist<AudioInput*>::const_iterator ciAudioInput;
typedef tracklist<AudioInput*> InputList;

typedef tracklist<AudioOutput*>::iterator iAudioOutput;
typedef tracklist<AudioOutput*>::const_iterator ciAudioOutput;
typedef tracklist<AudioOutput*> OutputList;

typedef tracklist<AudioGroup*>::iterator iAudioGroup;
typedef tracklist<AudioGroup*>::const_iterator ciAudioGroup;
typedef tracklist<AudioGroup*> GroupList;

typedef tracklist<AudioAux*>::iterator iAudioAux;
typedef tracklist<AudioAux*>::const_iterator ciAudioAux;
typedef tracklist<AudioAux*> AuxList;

typedef tracklist<SynthI*>::iterator iSynthI;
typedef tracklist<SynthI*>::const_iterator ciSynthI;
typedef tracklist<SynthI*> SynthIList;

extern void addPortCtrlEvents(MidiTrack* t);
extern void removePortCtrlEvents(MidiTrack* t);
extern void addPortCtrlEvents(Track* track, PendingOperationList& ops);
extern void removePortCtrlEvents(Track* track, PendingOperationList& ops);
} // namespace MusECore

#endif

