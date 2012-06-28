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

#include "pos.h"
#include "globaldefs.h"
#include "tempo.h"
#include "al/sig.h"
#include "undo.h"
#include "track.h"
#include "synth.h"

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
class MPEventList;
class EventList;
class MarkerList;
class Marker;
class SNode;

struct AudioMsg;

class MidiPart;
class MidiPort;

class MidiDevice;
class AudioPort;
class AudioDevice;

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
#define SC_SELECTION           0x1000
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
#define SC_SONG_TYPE           0x20000000   // the midi song type (mtype) changed
#define SC_KEY                 0x40000000   // key map changed

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
      
      int updateFlags;

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
      Pos pos[3];
      Pos _vcpos;               // virtual CPOS (locate in progress)
      MarkerList* _markerList;

      bool _masterFlag;
      bool loopFlag;
      bool punchinFlag;
      bool punchoutFlag;
      bool recordFlag;
      bool soloFlag;
      enum MType _mtype;
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

   public:
      Song(const char* name = 0);
      ~Song();

      bool applyOperationGroup(Undo& group, bool doUndo=true);
      void informAboutNewParts(const std::map< Part*, std::set<Part*> >&);
      void informAboutNewParts(Part* orig, Part* p1, Part* p2=NULL, Part* p3=NULL, Part* p4=NULL, Part* p5=NULL, Part* p6=NULL, Part* p7=NULL, Part* p8=NULL, Part* p9=NULL);

      void putEvent(int pv);
      void endMsgCmd();
      void processMsg(AudioMsg* msg);

      void setMType(MType t);
      MType mtype() const              { return _mtype; }

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
      void setLen(unsigned l);     // set songlen in ticks
      int roundUpBar(int tick) const;
      int roundUpBeat(int tick) const;
      int roundDownBar(int tick) const;
      void initLen();
      void tempoChanged();

      //-----------------------------------------
      //   event manipulations
      //-----------------------------------------

      void cmdAddRecordedWave(WaveTrack* track, Pos, Pos);  
      void cmdAddRecordedEvents(MidiTrack*, EventList*, unsigned);
      bool addEvent(Event&, Part*);
      void changeEvent(Event&, Event&, Part*);
      void deleteEvent(Event&, Part*);
      void cmdChangeWave(QString original, QString tmpfile, unsigned sx, unsigned ex);
      void remapPortDrumCtrlEvents(int mapidx, int newnote, int newchan, int newport);
      void changeAllPortDrumCtrlEvents(bool add, bool drumonly = false);
      
      void addACEvent(AudioTrack* t, int acid, int frame, double val);
      void changeACEvent(AudioTrack* t, int acid, int frame, int newFrame, double val);
      void addExternalTempo(const TempoRecEvent& e) { _tempoFifo.put(e); }
      
      //-----------------------------------------
      //   part manipulations
      //-----------------------------------------

      void cmdResizePart(Track* t, Part* p, unsigned int size, bool doClones=false);
      void cmdSplitPart(Track* t, Part* p, int tick);
      void cmdGluePart(Track* t, Part* p);

      void addPart(Part* part);
      void removePart(Part* part);
      void changePart(Part*, Part*);
      PartList* getSelectedMidiParts() const;
      PartList* getSelectedWaveParts() const;
      bool msgRemoveParts();

      void cmdChangePart(Part* oldPart, Part* newPart, bool doCtrls, bool doClones);
      void cmdRemovePart(Part* part);
      void cmdAddPart(Part* part);
      int arrangerRaster() { return _arrangerRaster; }        // Used by Song::cmdAddRecordedWave to snap new wave parts
      void setArrangerRaster(int r) { _arrangerRaster = r; }  // Used by Arranger snap combo box

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
      
      void cmdRemoveTrack(Track* track);
      void removeTrack0(Track* track);
      void removeTrack1(Track* track);
      void removeTrack2(Track* track);
      void removeTrack3(Track* track);
      void removeMarkedTracks();
      //void changeTrack(Track* oldTrack, Track* newTrack); DELETETHIS
      MidiTrack* findTrack(const Part* part) const;
      Track* findTrack(const QString& name) const;
      void swapTracks(int i1, int i2);
      void setChannelMute(int channel, bool flag);
      void setRecordFlag(Track*, bool);
      void insertTrack0(Track*, int idx);
      void insertTrack1(Track*, int idx);
      void insertTrack2(Track*, int idx);
      void insertTrack3(Track*, int idx);
      void deselectTracks();
      void readRoute(Xml& xml);
      void recordEvent(MidiTrack*, Event&);
      void msgInsertTrack(Track* track, int idx, bool u = true);
      void clearRecAutomation(bool clearList);
      void processAutomationEvents();
      void processMasterRec();
      int execAutomationCtlPopup(AudioTrack*, const QPoint&, int);
      int execMidiAutomationCtlPopup(MidiTrack*, MidiPart*, const QPoint&, int);
      void connectJackRoutes(AudioTrack* track, bool disconnect);
      void updateSoloStates();

      //-----------------------------------------
      //   undo, redo
      //-----------------------------------------

      void startUndo();
      void endUndo(int);

      void undoOp(UndoOp::UndoType type, const char* changedFile, const char* changeData, int startframe, int endframe);

      bool doUndo1();
      void doUndo2();
      void doUndo3();
      bool doRedo1();
      void doRedo2();
      void doRedo3();

      void addUndo(UndoOp i);

      //-----------------------------------------
      //   Configuration
      //-----------------------------------------

      SynthI* createSynthI(const QString& sclass, const QString& label = QString(), Synth::Type type = Synth::SYNTH_TYPE_END, Track* insertAt = 0);
      
      void rescanAlsaPorts();

      //-----------------------------------------
      //   Debug
      //-----------------------------------------

      void dumpMaster();
      void addUpdateFlags(int f)  { updateFlags |= f; }

      //-----------------------------------------
      //   Python bridge related
      //-----------------------------------------
#ifdef ENABLE_PYTHON
      virtual bool event (QEvent* e );
#endif
      void executeScript(const char* scriptfile, PartList* parts, int quant, bool onlyIfSelected);

   public slots:
      void seekTo(int tick);
      void update(int flags = -1);
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
      Track* addTrack(Undo& operations, Track::TrackType type, Track* insertAt = 0);
      Track* addNewTrack(QAction* action, Track* insertAt = 0);
      void duplicateTracks();
      QString getScriptPath(int id, bool delivered);
      void populateScriptMenu(QMenu* menuPlugins, QObject* receiver);

   signals:
      void songChanged(int);
      void posChanged(int, unsigned, bool);
      void loopChanged(bool);
      void recordChanged(bool);
      void playChanged(bool);
      void punchinChanged(bool);
      void punchoutChanged(bool);
      void clickChanged(bool);
      void quantizeChanged(bool);
      void markerChanged(int);
      void midiPortsChanged();
      void midiNote(int pitch, int velo);  
      void controllerChanged(MusECore::Track*, int); 
      void newPartsCreated(const std::map< MusECore::Part*, std::set<MusECore::Part*> >&);
      };

} // namespace MusECore

namespace MusEGlobal {
extern MusECore::Song* song;
}

#endif

