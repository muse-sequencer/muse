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

#include <map>
#include <set>
#include <list>

#include "type_defs.h"
#include "pos.h"
#include "globaldefs.h"
#include "tempo.h"
#include "al/sig.h"
#include "undo.h"
#include "track.h"
#include "synth.h"
#include "operations.h"

class QAction;
class QFont;
class QMenu;

namespace MusECore {

class SynthI;
struct MidiMsg;
class Event;
class Xml;
class Sequencer;
class Track;
class Part;
class PartList;
struct MPEventList;
class EventList;
class MarkerList;
class Marker;
class SNode;
struct RouteList;

struct AudioMsg;

class MidiPart;
class MidiPort;

class MidiDevice;
class AudioPort;
class AudioDevice;

// Song changed flags:
// These are flags, usually passed by connecting to the songChanged() signal,
//  which inform that various things have changed and appropriate action should 
//  be taken (redraw, refill lists etc.) upon the signal's reception.
// NOTE: Use the SongChangedFlags_t typedef in type_defs.h to support all the bits.

#define SC_TRACK_INSERTED      1
#define SC_TRACK_REMOVED       2
#define SC_TRACK_MODIFIED      4
#define SC_PART_INSERTED       8
#define SC_PART_REMOVED        0x10
#define SC_PART_MODIFIED       0x20
#define SC_EVENT_INSERTED      0x40
#define SC_EVENT_REMOVED       0x80
#define SC_EVENT_MODIFIED      0x100
#define SC_SIG                 0x200       // timing signature
#define SC_TEMPO               0x400       // tempo map changed
#define SC_MASTER              0x800       // master flag changed
#define SC_SELECTION           0x1000      // event selection. part and track selection have their own.
#define SC_MIDI_CONTROLLER     0x2000      // must update midi mixer
#define SC_MUTE                0x4000
#define SC_SOLO                0x8000
#define SC_RECFLAG             0x10000
#define SC_ROUTE               0x20000
#define SC_CHANNELS            0x40000
#define SC_CONFIG              0x80000     // midiPort-midiDevice
#define SC_DRUMMAP             0x100000    // must update drumeditor
#define SC_MIXER_VOLUME        0x200000
#define SC_MIXER_PAN           0x400000
#define SC_AUTOMATION          0x800000
#define SC_AUX                 0x1000000   // mixer aux changed
#define SC_RACK                0x2000000   // mixer rack changed
#define SC_CLIP_MODIFIED       0x4000000
#define SC_MIDI_CONTROLLER_ADD 0x8000000   // a hardware midi controller was added or deleted
#define SC_MIDI_TRACK_PROP     0x10000000   // a midi track's properties changed (channel, compression etc)
#define SC_PART_SELECTION      0x20000000   // part selection changed
#define SC_KEY                 0x40000000   // key map changed
#define SC_TRACK_SELECTION     0x80000000   // track selection changed
#define SC_PORT_ALIAS_PREFERENCE 0x100000000  // (Jack) port alias viewing preference has changed
#define SC_EVERYTHING          -1           // global update

#define REC_NOTE_FIFO_SIZE    16

//---------------------------------------------------------
//    Song
//---------------------------------------------------------

class Song : public QObject {
      Q_OBJECT

   public:
      enum POS        { CPOS = 0, LPOS, RPOS };
      enum FollowMode { NO, JUMP, CONTINUOUS };
      enum            { REC_OVERDUP, REC_REPLACE };
      enum            { CYCLE_NORMAL, CYCLE_MIX, CYCLE_REPLACE };
      enum { MARKER_CUR, MARKER_ADD, MARKER_REMOVE, MARKER_NAME,
         MARKER_TICK, MARKER_LOCK };

   private:
      // fifo for note-on events
      //    - this events are read by the heart beat interrupt
      //    - used for single step recording in midi editors

      int recNoteFifo[REC_NOTE_FIFO_SIZE];
      volatile int noteFifoSize;
      int noteFifoWindex;
      int noteFifoRindex;

      TempoFifo _tempoFifo; // External tempo changes, processed in heartbeat.
      
      MusECore::SongChangedFlags_t updateFlags;

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

      bool _masterFlag;
      bool loopFlag;
      bool punchinFlag;
      bool punchoutFlag;
      bool recordFlag;
      bool soloFlag;
      int _recMode;
      int _cycleMode;
      bool _click;
      bool _quantize;
      int _arrangerRaster;        // Used for audio rec new part snapping. Set by Arranger snap combo box.
      unsigned _len;         // song len in ticks
      FollowMode _follow;
      int _globalPitchShift;
      void readMarker(Xml&);

      QString songInfoStr;  // contains user supplied song information, stored in song file.
      bool showSongInfo;
      QStringList deliveredScriptNames;
      QStringList userScriptNames;

      // These are called from non-RT thread operations execution stage 1.
      void insertTrackOperation(Track* track, int idx, PendingOperationList& ops);
      void removeTrackOperation(Track* track, PendingOperationList& ops);
      bool addEventOperation(const Event&, Part*, bool do_port_ctrls = true, bool do_clone_port_ctrls = true);
      void changeEventOperation(const Event&, const Event&, Part*, bool do_port_ctrls = true, bool do_clone_port_ctrls = true);
      void deleteEventOperation(const Event&, Part*, bool do_port_ctrls = true, bool do_clone_port_ctrls = true);
      
   public:
      Song(const char* name = 0);
      ~Song();

      /** It is not allowed nor checked(!) to AddPart a clone, and
       *  to AddEvent/DeleteEvent/ModifyEvent/SelectEvent events which
       *  would need to be replicated to the newly added clone part!
       */
      bool applyOperationGroup(Undo& group, bool doUndo=true); // group may be changed! prepareOperationGroup is called on group!
      bool applyOperation(const UndoOp& op, bool doUndo=true);
      
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
      void informAboutNewParts(const Part* orig, const Part* p1, const Part* p2=NULL, const Part* p3=NULL, const Part* p4=NULL, const Part* p5=NULL, const Part* p6=NULL, const Part* p7=NULL, const Part* p8=NULL, const Part* p9=NULL);

      void putEvent(int pv);
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
      void writeFont(int level, Xml& xml, const char* name,
         const QFont& font) const;
      QFont readFont(Xml& xml, const char* name);
      QString getSongInfo() { return songInfoStr; }
      void setSongInfo(QString info, bool show) { songInfoStr = info; showSongInfo = show; }
      bool showSongInfoOnStartup() { return showSongInfo; }

      // If clear_all is false, it will not touch things like midi ports.
      void clear(bool signal, bool clear_all = true);  
      void cleanupForQuit();

      int globalPitchShift() const      { return _globalPitchShift; }
      void setGlobalPitchShift(int val) { _globalPitchShift = val; }

      //-----------------------------------------
      //   Marker
      //-----------------------------------------

      MarkerList* marker() const { return _markerList; }
      Marker* addMarker(const QString& s, int t, bool lck);
      Marker* getMarkerAt(int t);
      void removeMarker(Marker*);
      Marker* setMarkerName(Marker*, const QString&);
      Marker* setMarkerTick(Marker*, int);
      Marker* setMarkerLock(Marker*, bool);
      void setMarkerCurrent(Marker* m, bool f);

      //-----------------------------------------
      //   transport
      //-----------------------------------------

      void setPos(int, const Pos&, bool sig = true, bool isSeek = true,
         bool adjustScrollbar = false);
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
      bool masterFlag() const       { return _masterFlag; }
      void setRecMode(int val)      { _recMode = val; }
      int  recMode() const          { return _recMode; }
      void setCycleMode(int val)    { _cycleMode = val; }
      int cycleMode() const         { return _cycleMode; }
      bool click() const            { return _click; }
      bool quantize() const         { return _quantize; }
      void setStopPlay(bool);
      void stopRolling();
      void abortRolling();

      //-----------------------------------------
      //    access tempomap/sigmap  (Mastertrack)
      //-----------------------------------------

      unsigned len() const { return _len; }
      void setLen(unsigned l, bool do_update = true);     // set songlen in ticks
      int roundUpBar(int tick) const;
      int roundUpBeat(int tick) const;
      int roundDownBar(int tick) const;
      void initLen();

      //-----------------------------------------
      //   event manipulations
      //-----------------------------------------

      void cmdAddRecordedWave(WaveTrack* track, Pos, Pos);  
      void cmdAddRecordedEvents(MidiTrack*, const EventList&, unsigned);

      // May be called from GUI or audio thread. Also selects events in clone parts. Safe for now because audio/midi processing doesn't 
      //  depend on it, and all calls to part altering functions from GUI are synchronized with (wait for) audio thread.
      void selectEvent(Event&, Part*, bool select);   
      void selectAllEvents(Part*, bool select); // See selectEvent().  

      void cmdChangeWave(QString original, QString tmpfile, unsigned sx, unsigned ex); // FIXME TODO broken, fix that.
      void remapPortDrumCtrlEvents(int mapidx, int newnote, int newchan, int newport); // called from GUI thread
      void changeAllPortDrumCtrlEvents(bool add, bool drumonly = false); // called from GUI thread
      
      void addExternalTempo(const TempoRecEvent& e) { _tempoFifo.put(e); }
      
      //-----------------------------------------
      //   part manipulations
      //-----------------------------------------

      void cmdResizePart(Track* t, Part* p, unsigned int size, bool doClones=false); // called from GUI thread, calls applyOperationGroup. FIXME TODO: better move that into functions.cpp or whatever.

      void addPart(Part* part);
      void removePart(Part* part);
      void changePart(Part*, Part*);

      
      PartList* getSelectedMidiParts() const; // FIXME TODO move functionality into function.cpp
      PartList* getSelectedWaveParts() const;

      int arrangerRaster() { return _arrangerRaster; }        // Used by Song::cmdAddRecordedWave to snap new wave parts
      void setArrangerRaster(int r) { _arrangerRaster = r; }  // Used by Arranger snap combo box

private:
      void normalizePart(MusECore::Part *part);
public:
      void normalizeWaveParts(Part *partCursor = NULL);

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
      
      MidiTrack* findTrack(const Part* part) const;
      Track* findTrack(const QString& name) const;
      void setRecordFlag(Track*, bool);
      void insertTrack0(Track*, int idx);
      void insertTrack1(Track*, int idx);
      void insertTrack2(Track*, int idx);
      void insertTrack3(Track*, int idx);
      void deselectTracks();
      void readRoute(Xml& xml);
      void recordEvent(MidiTrack*, Event&);
      // Enable all track and plugin controllers, and synth controllers if applicable, which are NOT in AUTO_WRITE mode.
      void reenableTouchedControllers();  
      void clearRecAutomation(bool clearList);
      void processAutomationEvents();
      void processMasterRec();
      int execAutomationCtlPopup(AudioTrack*, const QPoint&, int);
      int execMidiAutomationCtlPopup(MidiTrack*, MidiPart*, const QPoint&, int);
      void connectJackRoutes(const MusECore::Route& src, const MusECore::Route& dst, bool disconnect = false);
      void connectAudioPorts();
      void connectMidiPorts();
      void connectAllPorts() { connectAudioPorts(); connectMidiPorts(); }
      void updateSoloStates();

      //-----------------------------------------
      //   undo, redo, operation groups
      //-----------------------------------------

      void startUndo();
      void endUndo(MusECore::SongChangedFlags_t);

    void undoOp(UndoOp::UndoType type, const QString& changedFile, const QString& changeData, int startframe, int endframe); // FIXME FINDMICHJETZT what's that?! remove it!

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

      SynthI* createSynthI(const QString& sclass, const QString& label = QString(), Synth::Type type = Synth::SYNTH_TYPE_END, Track* insertAt = 0);
      
      //-----------------------------------------
      //   Debug
      //-----------------------------------------

      void dumpMaster();
      void addUpdateFlags(MusECore::SongChangedFlags_t f)  { updateFlags |= f; }

      //-----------------------------------------
      //   Python bridge related
      //-----------------------------------------
#ifdef ENABLE_PYTHON
      virtual bool event (QEvent* e );
#endif
      void executeScript(const char* scriptfile, PartList* parts, int quant, bool onlyIfSelected);

   public slots:
      void seekTo(int tick);
      // use allowRecursion with care! this could lock up muse if you 
      //  aren't sure that your recursion will be finite!
      void update(MusECore::SongChangedFlags_t flags = -1, bool allowRecursion=false); 
      void beat();

      void undo();
      void redo();

      void setTempo(int t);
      void setSig(int a, int b);
      void setSig(const AL::TimeSignature&);
      void setTempo(double tempo)  { setTempo(int(60000000.0/tempo)); }

      void setMasterFlag(bool flag);
      bool getLoop() { return loopFlag; }
      void setLoop(bool f);
      void setRecord(bool f, bool autoRecEnable = true);
      void clearTrackRec();
      void setPlay(bool f);
      void setStop(bool);
      void forward();
      void rewindStart();
      void rewind();
      void setPunchin(bool f);
      void setPunchout(bool f);
      void setClick(bool val);
      void setQuantize(bool val);
      void panic();
      void seqSignal(int fd);
      Track* addTrack(Track::TrackType type, Track* insertAt = 0);
      Track* addNewTrack(QAction* action, Track* insertAt = 0);
      void duplicateTracks();
      QString getScriptPath(int id, bool delivered);
      void populateScriptMenu(QMenu* menuPlugins, QObject* receiver);
      void setDirty() { emit sigDirty(); }

   signals:
      void songChanged(MusECore::SongChangedFlags_t); 
      void posChanged(int, unsigned, bool);
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
      };

} // namespace MusECore

namespace MusEGlobal {
extern MusECore::Song* song;
}

#endif

