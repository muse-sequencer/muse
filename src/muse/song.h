//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: song.h,v 1.35.2.25 2009/12/15 03:39:58 terminator356 Exp $
//
//  (C) Copyright 1999/2000 Werner Schweer (ws@seh.de)
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

#ifndef __SONG_H__
#define __SONG_H__

#include <QObject>
#include <QStringList>
#include <QFont>

#include <map>
#include <set>
#include <list>

#include "type_defs.h"
#include "pos.h"
#include "globaldefs.h"
#include "tempo.h"
#include "sig.h"
#include "synth.h"
#include "operations.h"
#include "lock_free_buffer.h"
#include "mpevent.h"
#include "wave.h"


//#define IPC_EVENT_FIFO_SIZE ( std::min( std::max(size_t(256), size_t(MusEGlobal::segmentSize * 16)),  size_t(16384)) )

#include "time_stretch.h"
#include "audio_convert/audio_converter_settings_group.h"


// Forward declarations:
class QAction;
class QMenu;

namespace MusECore {

class Event;
class Xml;
class Track;
class Part;
class PartList;
class EventList;
class MarkerList;
class Marker;
class Route;
struct AudioMsg;
class MidiPart;
class Undo;
struct UndoOp;
class UndoList;

//---------------------------------------------------------
//    Song
//---------------------------------------------------------

class Song : public QObject {
      Q_OBJECT

   public:
      enum POSTYPE    { CPOS = 0, LPOS, RPOS };
      enum FollowMode { NO, JUMP, CONTINUOUS };
      enum            { REC_OVERDUP, REC_REPLACE };
      enum            { CYCLE_NORMAL, CYCLE_MIX, CYCLE_REPLACE };
      enum { MARKER_CUR };
      enum OperationType {
        // Execute the operation only, the operation is not un-doable. No song update.
        OperationExecute,
        // Execute the operation only, the operation is not un-doable. Song is updated.
        OperationExecuteUpdate,
        // Execute the operation, the operation is un-doable,
        //  and do not 'start' and 'end' the undo mode. No song update.
        OperationUndoable,
        // Execute the operation, the operation is un-doable,
        //  and do not 'start' and 'end' the undo mode. Song is updated.
        OperationUndoableUpdate,
        // Execute the operation, the operation is un-doable,
        //  and 'start' and 'end' the undo mode. Song is updated.
        OperationUndoMode
      };
      enum FastMove { //
          NORMAL_MOVEMENT,
          FAST_FORWARD,
          FAST_REWIND,
      };

   private:
      // fifos for feeding info events into the GUI
      LockFreeMPSCRingBuffer<MidiRecordEvent> *realtimeMidiEvents;
      LockFreeMPSCRingBuffer<MMC_Commands> *mmcEvents;

      TempoFifo _tempoFifo; // External tempo changes, processed in heartbeat.
      
      MusECore::SongChangedStruct_t updateFlags;

      TrackList _tracks;      // tracklist as seen by arranger
      MidiTrackList  _midis;
      WaveTrackList _waves;
      InputList _inputs;      // audio input ports
      OutputList _outputs;    // audio output ports
      GroupList _groups;      // mixer groups
      AuxList _auxs;          // aux sends
      SynthIList _synthIs;

      UndoList* undoList;
      UndoList* redoList;
      // New items created in GUI thread awaiting addition in audio thread.
      PendingOperationList pendingOperations;
      
      Pos pos[3];
      Pos _vcpos;               // virtual CPOS (locate in progress)
      MarkerList* _markerList;

      float _fCpuLoad;
      float _fDspLoad;
      long _xRunsCount;

      // Receives events from any threads. For now, specifically for creating new
      //  controllers in the gui thread and adding them safely to the controller lists.
      LockFreeMPSCRingBuffer<MidiPlayEvent> *_ipcInEventBuffers;
      LockFreeMPSCRingBuffer<MidiPlayEvent> *_ipcOutEventBuffers;
      
      // Used for fastforward and fastrewind states (currently only through MIDI remote control)
      FastMove _fastMove = NORMAL_MOVEMENT;
      bool loopFlag;
      bool punchinFlag;
      bool punchoutFlag;
      bool recordFlag;
      bool soloFlag;
      int _recMode;
      int _cycleMode;
      Pos _startPlayPosition;
      bool _click;
      bool _quantize;
      unsigned _songLenTicks;         // song len in ticks
      FollowMode _follow;
      int _globalPitchShift;
      void readMarker(Xml&);

      QString songInfoStr;  // contains user supplied song information, stored in song file.
      bool showSongInfo;

      // Private: Update the audio device's real transport position after a tempo or master change for ex.
      void updateTransportPos(const SongChangedStruct_t& flags);
      
      // These are called from non-RT thread operations execution stage 1.
      void insertTrackOperation(Track* track, int idx, PendingOperationList& ops);
      bool adjustMarkerListOperation(MarkerList* markerlist, unsigned int startPos, int diff, PendingOperationList& ops);
      void removeTrackOperation(Track* track, PendingOperationList& ops);
      bool addEventOperation(const Event&, Part*, bool do_port_ctrls = true, bool do_clone_port_ctrls = true);
      // Special: Returns the real actual event to be deleted found in the event lists.
      // This way even a modified event can be passed in, and as long as
      //  the ID AND position values match it will find and return the ORIGINAL event.
      // Useful for example for passing a pre-modified event to a DeleteEvent or ModyfyEvent operation
      //  in the Undo system, and it will automatically replace the Undo item's event with
      //  the real one returned here. (Otherwise when the user hits 'undo' it would restore
      //  that modified passed-in event sitting in the Undo item. That's not the right event!)
      Event changeEventOperation(const Event& oldEvent, const Event& newEvent,
                                Part*, bool do_port_ctrls = true, bool do_clone_port_ctrls = true);
      Event deleteEventOperation(const Event&, Part*, bool do_port_ctrls = true, bool do_clone_port_ctrls = true);

      void checkSongSampleRate();
      
      void normalizePart(MusECore::Part *part);

      // Fills operations if given, otherwise creates and executes its own operations list.
      void processTrackAutomationEvents(AudioTrack *atrack, Undo* operations = 0);

   public:
      Song(const char* name = 0);
      ~Song();

      /** It is not allowed nor checked(!) to AddPart a clone, and
       *  to AddEvent/DeleteEvent/ModifyEvent/SelectEvent events which
       *  would need to be replicated to the newly added clone part!
       */
      // group may be changed! prepareOperationGroup is called on group!
      // Sender can be set to the caller object and used by it
      //  to ignore self-generated songChanged signals.
      // The songChanged structure will contain this pointer.
      bool applyOperationGroup(Undo& group, OperationType type = OperationUndoMode, void* sender = 0);
      bool applyOperation(const UndoOp& op, OperationType type = OperationUndoMode, void* sender = 0);
      
      /** this sends emits a signal to each MidiEditor or whoever is interested.
       *  For each part which is 1) opened in this MidiEditor and 2) which is
       *  a key in this map, the Editors shall no more edit this part, but instead
       *  all parts in the_map[old_part] (which is a std::set<Part*>)
       */
      void informAboutNewParts(const std::map< const Part*, std::set<const Part*> >&);
      /** this sends emits a signal to each MidiEditor or whoever is interested.
       *  For each part which is 1) opened in this MidiEditor and 2) which is
       *  a key in this map, the Editors shall no more edit this part, but instead
       *  all parts in the_map[old_part] (which is a std::set<Part*>)
       *  this is a special case of the general function, which only replaces one part
       *  by up to nine different.
       */
      void informAboutNewParts(const Part* orig, const Part* p1, const Part* p2=nullptr, const Part* p3=nullptr, const Part* p4=nullptr, const Part* p5=nullptr, const Part* p6=nullptr, const Part* p7=nullptr, const Part* p8=nullptr, const Part* p9=nullptr);

      void putEvent(MidiRecordEvent &inputMidiEvent);
      void putMMC_Command(MMC_Commands command);

      void endMsgCmd();
      void processMsg(AudioMsg* msg);

      void setFollow(FollowMode m)     { _follow = m; }
      FollowMode follow() const        { return _follow; }

      bool dirty;
      WaveTrack* bounceTrack;
      AudioOutput* bounceOutput;
      void updatePos();

      void read(Xml&, bool isTemplate=false);
      void write(int, Xml&) const;
//      void writeFont(int level, Xml& xml, const char* name,
//         const QFont& font) const;
//      QFont readFont(Xml& xml, const char* name);
      // After a song file is successfully loaded with read(), some items which
      //  reference other items in the file need to be resolved. (Mixer strip configs etc.)
      void resolveSongfileReferences();

      QString getSongInfo() { return songInfoStr; }
      void setSongInfo(QString info, bool show) { songInfoStr = info; showSongInfo = show; }
      bool showSongInfoOnStartup() { return showSongInfo; }

      // If clear_all is false, it will not touch things like midi ports.
      void clear(bool signal, bool clear_all = true);  
      void cleanupForQuit();

      int globalPitchShift() const      { return _globalPitchShift; }
      void setGlobalPitchShift(int val) { _globalPitchShift = val; }
      
      // REMOVE Tim. samplerate. Added. TODO
#if 0
      // Set the project's sample rate. This can be different than the current actual sample rate.
      void setProjectSampleRate(int rate);
      // Ratio of the project's sample rate to the current audio sample rate.
      double projectSampleRateRatio() const;
      // Whether the project's sample rate ratio is exactly 1.
      bool projectSampleRateDiffers() const;
      // This scales the sample rate, by scaling the frame values of all objects or properties that use or store 
      //  a value in 'frames', such as wave parts/events position and length, and audio automation graphs.
      // It does NOT resample audio files, that is another function.
      // Caution: Slight rounding errors can degrade timing accuracy, especially if repeated scalings are done.
      void convertProjectSampleRate(int newRate);
#endif

      //-----------------------------------------
      //   Marker
      //-----------------------------------------

      MarkerList* marker() const { return _markerList; }
      // For wholesale swapping of a new list. Takes ownership of the given list.
      void setMarkerList(MarkerList* ml) { _markerList = ml; }
      void addMarker(const QString& s, unsigned t, bool lck);
      void addMarker(const QString& s, const Pos& p);
      iMarker getMarkerAt(unsigned t);
      void removeMarker(const Marker&);
      void setMarkerName(const Marker&, const QString&);
      void setMarkerPos(const Marker&, const Pos& position);
      void setMarkerLock(const Marker&, bool);

      //-----------------------------------------
      //   transport
      //-----------------------------------------

      void setPos(POSTYPE posType, const Pos&, bool sig = true, bool isSeek = true,
         bool adjustScrollbar = false, bool force = false);
      const Pos& cPos() const       { return pos[0]; }
      const Pos& lPos() const       { return pos[1]; }
      const Pos& rPos() const       { return pos[2]; }
      unsigned cpos() const         { return pos[0].tick(); }
      unsigned vcpos() const        { return _vcpos.tick(); }
      const Pos& vcPos() const      { return _vcpos; }
      unsigned lpos() const         { return pos[1].tick(); }
      unsigned rpos() const         { return pos[2].tick(); }

      bool loop() const             { return loopFlag; }
      bool record() const           { return recordFlag; }
      bool punchin() const          { return punchinFlag; }
      bool punchout() const         { return punchoutFlag; }
      void setRecMode(int val);
      int  recMode() const          { return _recMode; }
      void setCycleMode(int val);
      int cycleMode() const         { return _cycleMode; }
      bool click() const            { return _click; }
      bool quantize() const         { return _quantize; }
      void setStopPlay(bool);
      // Fills operations if given, otherwise creates and executes its own operations list.
      void stopRolling(Undo* operations = 0);
      void abortRolling();

      float cpuLoad() const { return _fCpuLoad; }
      float dspLoad() const { return _fDspLoad; }
      long xRunsCount() const { return _xRunsCount; }

      //-----------------------------------------
      //    access tempomap/MusEGlobal::sigmap  (Mastertrack)
      //-----------------------------------------

      unsigned len() const { return _songLenTicks; }
      void setLen(unsigned l, bool do_update = true);     // set songlen in ticks
      int roundUpBar(int tick) const;
      int roundUpBeat(int tick) const;
      int roundDownBar(int tick) const;
      void initLen();

      //-----------------------------------------
      //   event manipulations
      //-----------------------------------------

      void cmdAddRecordedWave(WaveTrack* track, Pos s, Pos e, Undo& operations);
      void cmdAddRecordedEvents(MidiTrack*, const EventList&, unsigned, Undo& operations);

      // May be called from GUI or audio thread. Also selects events in clone parts. Safe for now because audio/midi processing doesn't 
      //  depend on it, and all calls to part altering functions from GUI are synchronized with (wait for) audio thread.
      void selectEvent(Event&, Part*, bool select);   
      void selectAllEvents(Part*, bool select); // See selectEvent().  

      void cmdChangeWave(const Event& original, const QString& tmpfile, unsigned sx, unsigned ex);
      void remapPortDrumCtrlEvents(int mapidx, int newnote, int newchan, int newport); // called from GUI thread
      // Adds or removes midi event controller cache values.
      // Called from GUI thread
      // add true: add events. false: remove events
      // drum_tracks: Include drum tracks. midi_tracks: Include midi tracks.
      // drum_ctls: Do drum (per-note) controller events. non_drum_ctls: Do non-drum controller events.
      void changeMidiCtrlCacheEvents(
        bool add, bool drum_tracks = true, bool midi_tracks = true, bool drum_ctls = true, bool non_drum_ctls = true);
      
      void addExternalTempo(const TempoRecEvent& e) { _tempoFifo.put(e); }

      //--------------------------------------------
      //   Time-stretch / samplerate manipulations
      //--------------------------------------------

      void stretchModifyOperation(
         StretchList* stretch_list, StretchListItem::StretchEventType type, double value,
         PendingOperationList& ops) const;
      void stretchListAddOperation(
        StretchList* stretch_list, StretchListItem::StretchEventType type, MuseFrame_t frame,
        double value, PendingOperationList& ops) const;
      void stretchListDelOperation(
        StretchList* stretch_list, int types, MuseFrame_t frame, PendingOperationList& ops) const;
      void stretchListModifyOperation(
        StretchList* stretch_list, StretchListItem::StretchEventType type, MuseFrame_t frame,
        double value, PendingOperationList& ops) const;

      void setAudioConvertersOfflineOperation(
        bool isOffline
        );

      void modifyAudioConverterSettingsOperation(
        SndFileR sndfile,
        AudioConverterSettingsGroup* settings,
        AudioConverterSettingsGroup* defaultSettings,
        bool isLocalSettings,
        PendingOperationList& ops
        ) const;

      void modifyAudioConverterOperation(
        SndFileR sndfile,
        PendingOperationList& ops,
        bool doResample,
        bool doStretch
        ) const;

      void modifyStretchListOperation(
        SndFileR sndfile, int type, double value, PendingOperationList& ops) const;
      void addAtStretchListOperation(
        SndFileR sndfile, int type, MuseFrame_t frame, double value, PendingOperationList& ops) const;
      void delAtStretchListOperation(
        SndFileR sndfile, int types, MuseFrame_t frame, PendingOperationList& ops) const;
      void modifyAtStretchListOperation(
        SndFileR sndfile, int type, MuseFrame_t frame, double value, PendingOperationList& ops) const;
      void modifyDefaultAudioConverterSettingsOperation(
        AudioConverterSettingsGroup* settings, PendingOperationList& ops);

      //-----------------------------------------
      //   part manipulations
      //-----------------------------------------

      void addPart(Part* part);
      void removePart(Part* part);
      void changePart(Part*, Part*);

      void normalizeWaveParts(Part *partCursor = nullptr);

      //-----------------------------------------
      //   track manipulations
      //-----------------------------------------

      TrackList* tracks()       { return &_tracks;  }
      MidiTrackList* midis()    { return &_midis;   }
      WaveTrackList* waves()    { return &_waves;   }
      InputList* inputs()       { return &_inputs;  }
      OutputList* outputs()     { return &_outputs; }
      GroupList* groups()       { return &_groups;  }
      AuxList* auxs()           { return &_auxs;    }
      SynthIList* syntis()      { return &_synthIs; }
      
      Track* findTrack(const Part* part) const;
      Track* findTrack(const QString& name) const;
      bool trackExists(Track* t) const { return _tracks.find(t) != _tracks.cend(); }

      void setRecordFlag(Track*, bool val, Undo* operations = 0);
      // This is a non- realtime safe operation mainly used when loading files, while the engine is paused or idle.
      void insertTrack0(Track*, int idx);

      // The currently selected track (in a multi-selection the last one selected), or null.
      Track* selectedTrack() const { return _tracks.currentSelection(); }
      // Total number of selected tracks.
      int countSelectedTracks() const { return _tracks.countSelected(); }
      // Selects or deselects all tracks.
      void selectAllTracks(bool select)
      {
        _tracks.selectAll(select);
        if(!select) // Not essential, but if unselecting ALL tracks, clear the static counter.
          Track::clearSelectionOrderCounter();
      }

      void readRoute(Xml& xml);
      void recordEvent(MidiTrack*, Event&);
      // Enable all track and plugin controllers, and synth controllers if applicable, which are NOT in AUTO_WRITE mode.
      void reenableTouchedControllers();  
      void clearRecAutomation(bool clearList);
      // Fills operations if given, otherwise creates and executes its own operations list.
      void processAutomationEvents(Undo* operations = 0);
      void processMasterRec();
      int execAutomationCtlPopup(AudioTrack*, const QPoint&, int);
      int execMidiAutomationCtlPopup(MidiTrack*, MidiPart*, const QPoint&, int);
      bool connectJackRoutes(const MusECore::Route& src, const MusECore::Route& dst, bool disconnect = false);
      void connectAudioPorts();
      void connectMidiPorts();
      void connectAllPorts() { connectAudioPorts(); connectMidiPorts(); }
      void updateSoloStates();
      // Put an event into the IPC event ring buffer for the gui thread to process. Returns true on success.
      // NOTE: Although the ring buffer is multi-writer, call this from audio thread only for now, unless
      //  you know what you are doing because the thread needs to ask whether the controller exists before
      //  calling, and that may not be safe from threads other than gui or audio.
      bool putIpcInEvent(const MidiPlayEvent& ev);
      // Put an event into the IPC event ring buffer for the audio thread to process.
      // Called by gui thread only. Returns true on success.
      bool putIpcOutEvent(const MidiPlayEvent& ev);
      // Process any special IPC audio thread - to - gui thread messages.
      // Called by gui thread only. Returns true on success.
      bool processIpcInEventBuffers();
      // Process any special gui thread - to - IPC audio thread messages.
      // Called by audio thread only. Returns true on success.
      bool processIpcOutEventBuffers();

      //-----------------------------------------
      //   undo, redo, operation groups
      //-----------------------------------------

      // Sender can be set to the caller object and used by it
      //  to ignore self-generated songChanged signals.
      // The songChanged structure will contain this pointer.
      void startUndo(void* sender = 0);
      void endUndo(MusECore::SongChangedStruct_t);

      void executeOperationGroup1(Undo& operations);
      void executeOperationGroup2(Undo& operations);
      void executeOperationGroup3(Undo& operations);
      void revertOperationGroup1(Undo& operations);
      void revertOperationGroup2(Undo& operations);
      void revertOperationGroup3(Undo& operations);

      void addUndo(UndoOp i);
      void setUndoRedoText();

      //-----------------------------------------
      //   Configuration
      //-----------------------------------------

      SynthI* createSynthI(const QString& sclass, const QString& uri, const QString& label = QString(),
                           Synth::Type type = Synth::SYNTH_TYPE_END, Track* insertAt = 0);
      
      //-----------------------------------------
      //   Debug
      //-----------------------------------------

      void dumpMaster();
      void addUpdateFlags(MusECore::SongChangedStruct_t f)  { updateFlags |= f; }

      //-----------------------------------------
      //   Python bridge related
      //-----------------------------------------
#ifdef PYTHON_SUPPORT
      virtual bool event (QEvent* e );
#endif

   public slots:
      void seekTo(int tick);
      // use allowRecursion with care! this could lock up muse if you 
      //  aren't sure that your recursion will be finite!
      void update(SongChangedStruct_t flags = SongChangedStruct_t(SC_EVERYTHING),
                  bool allowRecursion=false); 
      void beat();

      void undo();
      void redo();

      void setTempo(int t);
      void setSig(int a, int b);
      void setSig(const MusECore::TimeSignature&);
      void setTempo(double tempo)  { setTempo(int(60000000.0/tempo)); }

      void setMasterFlag(bool flag);
      bool getLoop() { return loopFlag; }
      void setLoop(bool f);
      void setRecord(bool f, bool autoRecEnable = true);
      void clearTrackRec();
      void setPlay(bool f);
      void setStop(bool);
      void forwardStep();
      void rewindStart();
      void rewindStep();
      void setPunchin(bool f);
      void setPunchout(bool f);
      void setClick(bool val);
      void setQuantize(bool val);
      void panic();
      void seqSignal(int fd);

      void resetFastMove() { _fastMove = NORMAL_MOVEMENT; }

      // Creates a track but does not add it to the track list, the caller must do that.
      // The track name is not set and must be set by the caller.
      // If setDefaults is true, adds default in/out routes/channels to the track.
      Track* createTrack(Track::TrackType type, bool setDefaults = true);
      Track* addTrack(Track::TrackType type, Track* insertAt = 0);
      Track* addNewTrack(QAction* action, Track* insertAt = 0);
      void duplicateTracks(Track* t = nullptr);
      void setDirty() { emit sigDirty(); }

      /* restarts recording from last start position
       * if discard is true (default) then
       * recording will start on existing tracks,
       * else new copies of armed tracks will be created
       * and current armed tracks will be muted and unarmed
       * Called from gui thread only. */
      void restartRecording(bool discard = true);

   signals:
      void songChanged(MusECore::SongChangedStruct_t); 
      void posChanged(int, unsigned, bool);
      void posChanged(int, const MusECore::Pos, bool);
      void loopChanged(bool);
      void recordChanged(bool);
      void playChanged(bool);
      void punchinChanged(bool);
      void punchoutChanged(bool);
      void clickChanged(bool);
      void quantizeChanged(bool);
      void markerChanged(int);
      void midiNote(int pitch, int velo);  
      void controllerChanged(MusECore::Track*, int); 
      void newPartsCreated(const std::map< const MusECore::Part*, std::set<const MusECore::Part*> >&);
      void sigDirty();
      void recModeChanged(int);
      void cycleModeChanged(int);
      };

} // namespace MusECore

namespace MusEGlobal {
extern MusECore::Song* song;
}

#endif

