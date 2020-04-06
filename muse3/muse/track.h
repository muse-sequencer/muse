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
#include "audio_fifo.h"
#include "route.h"
#include "ctrl.h"
#include "globaldefs.h"
#include "cleftypes.h"
#include "controlfifo.h"
#include "latency_info.h"
#include "transport_obj.h"

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
         ASSIGN_NONE=0,
         ASSIGN_PROPERTIES=1, 
         ASSIGN_DUPLICATE_PARTS=2, ASSIGN_COPY_PARTS=4, ASSIGN_CLONE_PARTS=8, 
         ASSIGN_PLUGINS=16, 
         ASSIGN_STD_CTRLS=32, ASSIGN_PLUGIN_CTRLS=64, 
         ASSIGN_ROUTES=128, ASSIGN_DEFAULT_ROUTES=256, 
         ASSIGN_DRUMLIST=512 };
         
   private:
      TrackType _type;
      // Serial number generator.
      static int _snGen;
      QString _comment;
      PartList _parts;

      void init(int channels = 0);
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

      // Serial number.
      int _sn;
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
      double _meter[MusECore::MAX_CHANNELS];
      double _peak[MusECore::MAX_CHANNELS];
      bool _isClipped[MusECore::MAX_CHANNELS]; //used in audio mixer strip. Persistent.

      int _y;
      int _height;            // visual height in arranger

      bool _locked;
      bool _selected;
      // The selection order of this track, compared to other selected tracks.
      // The selected track with the highest selected order is the most recent selected.
      int _selectionOrder;

      // Holds latency computations each cycle.
      TrackLatencyInfo _latencyInfo;

      // Holds a special extra 'source': The transport stream.
      // Tracks or plugins that request/receive transport info use this.
      TransportSource _transportSource;

      int newSn() { return _snGen++; }
      
      bool readProperties(Xml& xml, const QString& tag);
      void writeProperties(int level, Xml& xml) const;

   public:
      Track(TrackType, int channels = 0);
      // Copy constructor.
      // The various AssignFlags (flags) determine what is copied.
      // For now, the new track's name is set to the original name.
      // The caller SHOULD set a proper unique name afterwards,
      //  otherwise identical track names will appear in the song.
      // It is possible for us to automatically to choose a unique name
      //  here, but that may not be wise in some cases (such as modifying a
      //  track copy and then requesting the operations system switch tracks -
      //  in that case we want the names to be the same).
      // Also, it is possible the name choosing routine(s) may fail for
      //  whatever reason, and we cannot abort creation here.
      // For that reason, it is best for the caller to try and pick a
      //  unique name successfully BEFORE calling this constructor.
      Track(const Track&, int flags);
      virtual ~Track();
      virtual void assign(const Track&, int flags);

      inline int serial() const { return _sn; }

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
      // Returns a name suitable for display like "1:Track 5" where the number is the track's index in the track list.
      // This is useful because in the case of importing a midi file we allow duplicate, often blank, names.
      // This display string will help identify them. Like "1:", "2:" etc.
      QString displayName() const;

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

      // Select or unselect a range of events. If t0 == t1, ALL events will be affected.
      // The t0 and t1 can be ticks or frames depending on the type of events. Unused for now.
      // Returns true if anything changed.
      bool selectEvents(bool select, unsigned long t0 = 0, unsigned long t1 = 0);
      
      virtual void write(int, Xml&) const = 0;

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

      virtual void preProcessAlways()    { }

      TransportSource& transportSource() { return _transportSource; }
//       // Returns true if the transport source is connected to any of the
//       //  track's midi input ports (ex. synth ports not muse midi ports).
//       // If midiport is -1, returns true if ANY port is connected.
//       virtual bool transportSourceConnected(int /*midiport*/ = -1) const { return false; }
      // Returns true if the transport source is connected to any of the
      //  track's midi input ports (ex. synth ports not muse midi ports).
      virtual bool usesTransportSource() const { return false; }
      virtual bool transportAffectsAudioLatency() const { return false; }

      // Initializes this track's latency information in preparation for a latency scan.
      virtual void prepareLatencyScan();
      // The contribution to latency from the track's own members (audio effect rack, etc).
      inline virtual float selfLatencyAudio(int /*channel*/) const { return 0.0f; }
      // The cached worst latency of all the channels in the track's effect rack plus any synthesizer latency if applicable.
      virtual float getWorstPluginLatencyAudio();
      // The cached worst contribution to latency by any ports (for ex. Jack ports of audio input/output tracks).
      virtual float getWorstPortLatencyAudio();
      // The cached worst latency of all the contributions from the track's own members (audio effect rack, etc)
      //  plus any port latency if applicable.
      virtual float getWorstSelfLatencyAudio();
      // Whether this track (and the branch it is in) can force other parallel branches to
      //  increase their latency compensation to match this one.
      // If false, this branch will NOT disturb other parallel branches' compensation,
      //  intead only allowing compensation UP TO the worst case in other branches.
      virtual bool canDominateOutputLatency() const;
      virtual bool canDominateInputLatency() const;
      // Whether this track (and the branch it is in) can force other parallel branches to
      //  increase their latency compensation to match this one - IF this track is an end-point
      //  and the branch allows domination.
      // If false, this branch will NOT disturb other parallel branches' compensation,
      //  intead only allowing compensation UP TO the worst case in other branches.
      inline virtual bool canDominateEndPointLatency() const { return false; }
      // Whether this track and its branch can correct for latency, not just compensate.
      inline virtual bool canCorrectOutputLatency() const { return false; }
      // Whether the track can pass latency values through, the SAME as if record monitor is
      //  supported and on BUT does not require record monitor support.
      // This is for example in the metronome MetronomeSynthI, since it is unique in that it
      //  can correct its own latency unlike other synths, but it does not 'pass through'
      //  the latency values to what drives it like other synths.
      virtual bool canPassThruLatency() const;
      // Whether any of the connected output routes are effectively connected.
      // That means track is not off, track is monitored where applicable, etc,
      //   ie. signal can actually flow.
      // For Wave Tracks for example, asks whether the track is an end-point from the view of the input side.
      virtual bool isLatencyInputTerminal() = 0;
      // Whether any of the connected output routes are effectively connected.
      // That means track is not off, track is monitored where applicable, etc,
      //   ie. signal can actually flow.
      // For Wave Tracks for example, asks whether the track is an end-point from the view of the playback side.
      virtual bool isLatencyOutputTerminal() = 0;

      virtual TrackLatencyInfo& getDominanceInfo(bool input) = 0;
      virtual TrackLatencyInfo& getDominanceLatencyInfo(bool input) = 0;
      // The finalWorstLatency is the grand final worst-case latency, of any output track or open branch,
      //  determined in the complete getDominanceLatencyInfo() scan.
      // The callerBranchLatency is the inherent branch latency of the calling track, or zero if calling from
      //  the very top outside of the branch heads (outside of output tracks or open branches).
      // The callerBranchLatency is accumulated as setCorrectionLatencyInfo() is called on each track
      //  in a branch of the graph.
      virtual TrackLatencyInfo& setCorrectionLatencyInfo(bool input, float finalWorstLatency, float callerBranchLatency = 0.0f) = 0;
      // Argument 'input': Whether we want the input side of the track. For example un-monitored wave tracks
      //  are considered two separate paths with a recording input side and a playback output side.
      virtual TrackLatencyInfo& getLatencyInfo(bool input) = 0;
      // Used during latency compensation processing. When analyzing in 'reverse' this mechansim is
      //  needed only to equalize the timing of all the AudioOutput tracks.
      // It is applied as a direct offset in the latency delay compensator in getData().
      virtual unsigned long latencyCompWriteOffset() const { return _latencyInfo._compensatorWriteOffset; }
      virtual void setLatencyCompWriteOffset(float /*worstCase*/) { }
      
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
      inline bool isClipped(int ch) const { if(ch >= MusECore::MAX_CHANNELS) return false; return _isClipped[ch]; }
      void resetClipper() { for(int ch = 0; ch < MusECore::MAX_CHANNELS; ++ch) _isClipped[ch] = false; }
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

      // FIXME This public assign() method doesn't really 'assign' routes -
      //        if routes are assigned in flags, it does not clear existing routes !
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
      
      virtual MidiTrack* clone(int flags) const { return new MidiTrack(*this, flags); }
      virtual Part* newPart(Part*p=0, bool clone=false);

      // Number of routable inputs/outputs for each Route::RouteType.
      virtual RouteCapabilitiesStruct routeCapabilities() const;

      virtual bool canDominateOutputLatency() const;
      virtual bool canCorrectOutputLatency() const;
      virtual bool isLatencyInputTerminal();
      virtual bool isLatencyOutputTerminal();

      virtual TrackLatencyInfo& getDominanceLatencyInfo(bool input);
      virtual TrackLatencyInfo& getDominanceInfo(bool input);
      // The finalWorstLatency is the grand final worst-case latency, of any output track or open branch,
      //  determined in the complete getDominanceLatencyInfo() scan.
      // The callerBranchLatency is the inherent branch latency of the calling track, or zero if calling from
      //  the very top outside of the branch heads (outside of output tracks or open branches).
      // The callerBranchLatency is accumulated as setCorrectionLatencyInfo() is called on each track
      //  in a branch of the graph.
      virtual TrackLatencyInfo& setCorrectionLatencyInfo(bool input, float finalWorstLatency, float callerBranchLatency = 0.0f);
      virtual TrackLatencyInfo& getLatencyInfo(bool input);
      // Used during latency compensation processing. When analyzing in 'reverse' this mechansim is
      //  needed only to equalize the timing of all the AudioOutput tracks.
      // It is applied as a direct offset in the latency delay compensator in getData().
      virtual void setLatencyCompWriteOffset(float worstCase);
      
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
      // Audio latency compensator.
      LatencyCompensator* _latencyComp;

      // These two are not the same as the number of track channels which is always either 1 (mono) or 2 (stereo):
      // Total number of output channels.
      int _totalOutChannels;
      // Total number of input channels.
      int _totalInChannels;
      
      Pipeline* _efxPipe;

      virtual bool getData(unsigned, int, unsigned, float**);

      SndFileR _recFile;
      // Exclusively for the recFile during bounce operations.
      long int _recFilePos;
      float _previousLatency;

      Fifo fifo;                    // fifo -> _recFile
      bool _processed;
      
   public:
      AudioTrack(TrackType t, int channels = 2);
      
      AudioTrack(const AudioTrack&, int flags);
      virtual ~AudioTrack();

      // FIXME This public assign() method doesn't really 'assign' routes -
      //        if routes are assigned in flags, it does not clear existing routes !
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

      virtual void preProcessAlways();

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

      // Whether to use latency correction/compensation at all.
      // Simply depends on _latencyComp existence AND global configuration enableLatencyCorrection flag.
      bool useLatencyCorrection() const;
      // The contribution to latency by the track's own members (audio effect rack, etc).
      virtual float selfLatencyAudio(int channel) const;
      // The cached worst latency of all the channels in the track's effect rack.
      virtual float getWorstPluginLatencyAudio();
      // The cached worst latency of all the contributions from the track's own members (audio effect rack, etc)
      //  plus any port latency if applicable.
      virtual float getWorstSelfLatencyAudio();
      virtual TrackLatencyInfo& getDominanceInfo(bool input);
      // Returns latency computations during each cycle. If the computations have already been done 
      //  this cycle, cached values are returned, otherwise they are computed, cached, then returned.
      virtual TrackLatencyInfo& getDominanceLatencyInfo(bool input);
      // The finalWorstLatency is the grand final worst-case latency, of any output track or open branch,
      //  determined in the complete getDominanceLatencyInfo() scan.
      // The callerBranchLatency is the inherent branch latency of the calling track, or zero if calling from
      //  the very top outside of the branch heads (outside of output tracks or open branches).
      // The callerBranchLatency is accumulated as setCorrectionLatencyInfo() is called on each track
      //  in a branch of the graph.
      virtual TrackLatencyInfo& setCorrectionLatencyInfo(bool input, float finalWorstLatency, float callerBranchLatency = 0.0f);
      virtual TrackLatencyInfo& getLatencyInfo(bool input);
      // Used during latency compensation processing. When analyzing in 'reverse' this mechansim is
      //  needed only to equalize the timing of all the AudioOutput tracks.
      // It is applied as a direct offset in the latency delay compensator in getData().
      virtual void setLatencyCompWriteOffset(float worstCase);
      virtual bool isLatencyInputTerminal();
      virtual bool isLatencyOutputTerminal();
      
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

      float selfLatencyAudio(int channel) const; 
      // The cached worst contribution to latency by any ports (for ex. Jack ports of audio input/output tracks).
      float getWorstPortLatencyAudio();
      // Audio Input tracks have no correction available. They ALWAYS dominate any parallel branches, if they are not 'off'.
      bool canDominateOutputLatency() const;
      
      // FIXME This public assign() method doesn't really 'assign' routes -
      //        if routes are assigned in flags, it does not clear existing routes !
      //       For input/output tracks we need to disconnect the routes from Jack.
      void assign(const Track&, int flags);
      AudioInput* clone(int flags) const { return new AudioInput(*this, flags); }
      void read(Xml&);
      void write(int, Xml&) const;
      // Register one or all input ports. If idx = -1 it registers all ports.
      // Returns true if ANY of the port(s) were successfully registered.
      bool registerPorts(int idx = -1);
      void setName(const QString& s);
      void* jackPort(int channel) { return jackPorts[channel]; }
      void setJackPort(int channel, void*p) { jackPorts[channel] = p; }
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
      void* jackPorts[MusECore::MAX_CHANNELS];
      float* buffer[MusECore::MAX_CHANNELS];
      float* buffer1[MusECore::MAX_CHANNELS];
      unsigned long _nframes;
      // Audio latency compensator, for compensating output signals
      //  according to any differing channel port latencies.
      LatencyCompensator* _outputLatencyComp;

      static bool _isVisible;
      void internal_assign(const Track& t, int flags);

   public:
      AudioOutput();
      AudioOutput(const AudioOutput&, int flags);
      virtual ~AudioOutput();

      float selfLatencyAudio(int channel) const;
      void setChannels(int n);
      // The cached worst contribution to latency by any ports (for ex. Jack ports of audio input/output tracks).
      float getWorstPortLatencyAudio();
      // Audio output tracks can allow a branch to dominate if they are an end-point and the branch can dominate.
      inline bool canDominateEndPointLatency() const { return true; }
      // Audio Output is considered a termination point.
      bool isLatencyInputTerminal();
      bool isLatencyOutputTerminal();
      void applyOutputLatencyComp(unsigned nframes);
      
      // FIXME This public assign() method doesn't really 'assign' routes -
      //        if routes are assigned in flags, it does not clear existing routes !
      //       For input/output tracks we need to disconnect the routes from Jack.
      void assign(const Track&, int flags);
      AudioOutput* clone(int flags) const { return new AudioOutput(*this, flags); }
      void read(Xml&);
      void write(int, Xml&) const;
      // Register one or all output ports. If idx = -1 it registers all ports.
      // Returns true if ANY of the port(s) were successfully registered.
      bool registerPorts(int idx = -1);
      void setName(const QString& s);
      void* jackPort(int channel) { return jackPorts[channel]; }
      void setJackPort(int channel, void*p) { jackPorts[channel] = p; }
      // Number of routable inputs/outputs for each Route::RouteType.
      RouteCapabilitiesStruct routeCapabilities() const;
      void processInit(unsigned);
      void process(unsigned pos, unsigned offset, unsigned);
      void processWrite();
      void silence(unsigned);
      bool canRecord() const { return true; }

      static void setVisible(bool t) { _isVisible = t; }
      static bool visible() { return _isVisible; }
      int height() const;
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
      float* buffer[MusECore::MAX_CHANNELS];
      static bool _isVisible;
      int _index;
   public:
      AudioAux();
      AudioAux(const AudioAux& t, int flags);
      
      AudioAux* clone(int flags) const { return new AudioAux(*this, flags); }
      ~AudioAux();
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
      virtual int index() { return _index; }
    };


//---------------------------------------------------------
//   WaveTrack
//---------------------------------------------------------

class WaveTrack : public AudioTrack {
      Fifo _prefetchFifo;  // prefetch Fifo
      // Each wavetrack has a separate prefetch position stamp
      //  so that consumers can retard or advance the stream and
      //  the prefetch can pump as much buffers as required while
      //  keeping track of the last buffer position stamp.
      unsigned _prefetchWritePos;
      static bool _isVisible;

      void internal_assign(const Track&, int flags);
      // Writes data from connected input routes to the track's latency compensator.
      // It uses buffer for temporary storage.
      bool getInputData(unsigned pos, int channels, unsigned nframes,
                        bool* usedInChannelArray, float** buffer);
      
      // Return false if error.
      bool getPrefetchData(bool have_data, sf_count_t framePos, int dstChannels, sf_count_t nframe, float** bp, bool do_overwrite);
      
   public:

      WaveTrack();
      WaveTrack(const WaveTrack& wt, int flags);

      // FIXME This public assign() method doesn't really 'assign' routes -
      //        if routes are assigned in flags, it does not clear existing routes !
      virtual void assign(const Track&, int flags);
      
      virtual WaveTrack* clone(int flags) const    { return new WaveTrack(*this, flags); }
      virtual Part* newPart(Part*p=0, bool clone=false);
      // Returns true if any event in any part was opened. Does not operate on the part's clones, if any.
      bool openAllParts();
      // Returns true if any event in any part was closed. Does not operate on the part's clones, if any.
      bool closeAllParts();

      virtual void read(Xml&);
      virtual void write(int, Xml&) const;

      // Called from prefetch thread:
      // If overwrite is true, copies the data. If false, adds the data.
      virtual void fetchData(unsigned pos, unsigned frames, float** bp, bool doSeek, bool overwrite, int latency_correction = 0);
      
      virtual void seekData(sf_count_t pos);
      
      virtual bool getData(unsigned, int ch, unsigned, float** bp);

      // Depending on the Monitor setting, Wave Tracks can have available correction.
      // If unmonitored, they will never dominate parallel branches.
      bool canDominateOutputLatency() const;
      bool canCorrectOutputLatency() const;
      
      void clearPrefetchFifo();
      Fifo* prefetchFifo()          { return &_prefetchFifo; }
      virtual void prefetchAudio(sf_count_t writePos, sf_count_t frames);

      // For prefetch thread use only.
      inline unsigned prefetchWritePos() const { return _prefetchWritePos; }
      inline void setPrefetchWritePos(unsigned p) { _prefetchWritePos = p; }

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
      const_iterator begin() const    { return vlist::cbegin(); }
      const_iterator end() const      { return vlist::cend(); }
      const_iterator cbegin() const   { return vlist::cbegin(); }
      const_iterator cend() const     { return vlist::cend(); }
      reverse_iterator rbegin()       { return vlist::rbegin(); }
      reverse_iterator rend()         { return vlist::rend(); }
      T& back() const                 { return (T&)(vlist::back()); }
      T& front() const                { return (T&)(vlist::front()); }
      iterator find(const Track* t)       {
            return std::find(begin(), end(), t);
            }
      const_iterator find(const Track* t) const {
            return std::find(cbegin(), cend(), t);
            }
      bool contains(const Track* t) const {
            return std::find(cbegin(), cend(), t) != cend();
            }
      int index(const Track* t) const {
            int n = 0;
            for (vlist::const_iterator i = cbegin(); i != cend(); ++i, ++n) {
                  if (*i == t)
                        return n;
                  }
            return -1;
            }
      T index(int k) const {
            if (k < 0 || k >= (int)size())
                  return nullptr;
            return (*this)[k];
            }
      T findSerial(int sn) const {
            if (sn < 0)
                  return nullptr;
            for (vlist::const_iterator i = cbegin(); i != cend(); ++i) {
                  if ((*i)->serial() == sn) {
                        return *i;
                        }
                  }
            return nullptr;
            }
      int indexOfSerial(int sn) const {
            if (sn < 0)
                  return -1;
            int n = 0;
            for (vlist::const_iterator i = cbegin(); i != cend(); ++i, ++n) {
                  if ((*i)->serial() == sn) {
                        return n;
                        }
                  }
            return -1;
            }
      iterator index2iterator(int k) {
            if (k < 0 || k >= (int)size())
                  return end();
            return begin() + k;
            }
      void erase(Track* t)           { vlist::erase(find(t)); }

      void clearDelete() {
            for (vlist::iterator i = begin(); i != end(); ++i)
                  delete *i;
            vlist::clear();
            }
      void erase(vlist::const_iterator i) { vlist::erase(i); }
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
            for (vlist::const_iterator i = cbegin(); i != cend(); ++i) {
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
            for (vlist::const_iterator i = cbegin(); i != cend(); ++i) {
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

