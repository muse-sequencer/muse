//=============================================================================
//  MusE
//  Linux Music Editor
//  $Id:$
//
//  Copyright (C) 2002-2006 by Werner Schweer and others
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

#ifndef __SONG_H__
#define __SONG_H__

#include "undo.h"
#include "miditrack.h"
#include "wavetrack.h"
#include "audioinput.h"
#include "audiooutput.h"
#include "audiogroup.h"
#include "midisynti.h"
#include "synth.h"
#include "ctrl.h"
#include "midififo.h"

namespace AL {
      class Xml;
      class Marker;
      class MarkerList;
      class Pos;
      };

struct AudioMsg;
class Event;
class Track;
class Part;
class PartList;
class Marker;
class SettingsList;

#define SC_TRACK_INSERTED     1
#define SC_TRACK_REMOVED      2
#define SC_TRACK_MODIFIED     4
#define SC_PART_INSERTED      8
#define SC_PART_REMOVED       0x10
#define SC_PART_MODIFIED      0x20
#define SC_EVENT_INSERTED     0x40
#define SC_EVENT_REMOVED      0x80
#define SC_EVENT_MODIFIED     0x100
#define SC_SIG                0x200       // timing signature
#define SC_TEMPO              0x400       // tempo map changed
#define SC_MASTER             0x800       // master flag changed
#define SC_SELECTION          0x1000
#define SC_RECFLAG            0x10000
#define SC_ROUTE              0x20000
#define SC_CHANNELS           0x40000
#define SC_CONFIG             0x80000     // midiPort-midiDevice
#define SC_DRUMMAP            0x100000    // must update drumeditor
#define SC_AUTOMATION         0x800000
#define SC_RACK               0x1000000   // mixer rack changed
#define SC_CLIP_MODIFIED      0x2000000

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
      QString _projectPath;
      QString _comment;
      bool _backupWritten;    // set after first "save" operation from
                              // user; used to make sure there will
                              // be only one backup for a session
      bool _created;          // project was created in current session
      QDateTime _createDate;

      MidiFifo eventFifo;

      int updateFlags;

      TrackList _tracks;      // tracklist as seen by arranger
      MidiTrackList  _midis;
      WaveTrackList _waves;
      InputList _inputs;      // audio input ports
      OutputList _outputs;    // audio output ports
      GroupList _groups;      // mixer groups
      SynthIList _synthIs;
      MidiSyntiList _midiSyntis;
      MidiOutPortList _midiOutPorts;
      MidiInPortList _midiInPorts;
      MidiChannelList _midiChannel;
      Track* _selectedTrack;

      UndoList* undoList;
      UndoList* redoList;
      Pos pos[3];
      Pos _vcpos;               // virtual CPOS (locate in progress)
      AL::MarkerList* _markerList;
      bool seekInProgress;	// user initiated a seek

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
      unsigned _len;         // song len in ticks
      int _globalPitchShift;
      void readMarker(QDomNode);
      void restartJack();

   public slots:
      void beat();

      void undo();
      void redo();

      void setTempo(int t);
      void setSig(const AL::TimeSignature&);

      void setMasterFlag(bool flag);
      void setLoop(bool f);
      void setRecord(bool f);
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
      Track* addTrack(QAction*);
      void setMeasureLen(int l);
      void changePart(Part*, unsigned, unsigned);
      void createLRPart(Track* track);
      //void setTickPos(int, unsigned);

      void setPos(int, const AL::Pos&);
      void setPos(int, const AL::Pos&, bool sig, bool isSeek = true,
         bool adjustScrollbar = false);

   signals:
      void songChanged(int);
      void posChanged(int, const AL::Pos&, bool);
      void loopChanged(bool);
      void recordChanged(bool);
      void playChanged(bool);
      void punchinChanged(bool);
      void punchoutChanged(bool);
      void clickChanged(bool);
      void quantizeChanged(bool);
      void markerChanged(int);
      void midiPortsChanged();
      void midiEvent(const MidiEvent&);
      void trackAdded(Track*, int idx);
      void trackRemoved(Track*);
      void lenChanged(const AL::Pos&);
      void measureLenChanged(int);

      void recordChanged(Track*,bool);
      void muteChanged(Track*,bool);
      void soloChanged(Track*,bool);
      void offChanged(Track*,bool);
      void autoReadChanged(Track*,bool);
      void autoWriteChanged(Track*,bool);
      void trackSelectionChanged(Track*);
      void tempoChanged();

   public:
      Song();
      ~Song();

      void putEvent(const MidiEvent&);
      void endMsgCmd();
      void processMsg(AudioMsg* msg);

      bool dirty;
      bool backupWritten() const       { return _backupWritten; }
      void setBackupWritten(bool val)  { _backupWritten = val; }
      WaveTrack* bounceTrack;

      void updatePos();

      void read(QDomNode);
      void write(Xml&) const;

      void clear(bool signal);
      void update(int flags = -1);

      int globalPitchShift() const      { return _globalPitchShift; }
      void setGlobalPitchShift(int val) { _globalPitchShift = val; }

      //-----------------------------------------
      //   Marker
      //-----------------------------------------

      AL::MarkerList* marker() const { return _markerList; }
      AL::Marker* addMarker(const QString& s, const AL::Pos&);
      void removeMarker(AL::Marker*);
      AL::Marker* setMarkerName(AL::Marker*, const QString&);
      AL::Marker* setMarkerTick(AL::Marker*, int);
      AL::Marker* setMarkerLock(AL::Marker*, bool);
      void setMarkerCurrent(AL::Marker* m, bool f);

      //-----------------------------------------
      //   transport
      //-----------------------------------------

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

      //-----------------------------------------
      //    access tempomap/sigmap  (Mastertrack)
      //-----------------------------------------

      unsigned len() const { return _len; }
      int roundUpBar(int tick) const;
      int roundUpBeat(int tick) const;
      int roundDownBar(int tick) const;

      //-----------------------------------------
      //   event manipulations
      //-----------------------------------------

      bool addEvent(Event&, Part*);
      void changeEvent(Event&, Event&, Part*);
      void deleteEvent(Event&, Part*);
      void cmdChangeWave(QString original, QString tmpfile, unsigned sx, unsigned ex);

      //-----------------------------------------
      //   part manipulations
      //-----------------------------------------

      void cmdSplitPart(Part* p, const Pos&);
      void cmdGluePart(Part* p);

      void changePart(Part*, Part*);
      void addPart(Part* part);
      PartList* getSelectedMidiParts() const;
      PartList* getSelectedWaveParts() const;
      bool msgRemoveParts();

      void cmdChangePart(Part* oldPart, Part* newPart);
      void cmdRemovePart(Part* part);
      void cmdAddPart(Part* part);
      void movePart(Part*, unsigned, Track*);
      void linkPart(Part*, unsigned, Track*);
      void copyPart(Part*, unsigned, Track*);
      void selectPart(Part*, bool add=false);

//      SettingsList* settingsList() { return esettingsList; }

      //-----------------------------------------
      //   track manipulations
      //-----------------------------------------

      TrackList* tracks()             { return &_tracks;       }
      MidiTrackList* midis()          { return &_midis;        }
      WaveTrackList* waves()          { return &_waves;        }
      InputList* inputs()             { return &_inputs;       }
      OutputList* outputs()           { return &_outputs;      }
      GroupList* groups()             { return &_groups;       }
      SynthIList* syntis()            { return &_synthIs;      }
      MidiOutPortList* midiOutPorts()   { return &_midiOutPorts; }
      MidiOutPort* midiOutPort(int idx) { return _midiOutPorts.index(idx); }
      MidiSyntiList* midiSyntis()       { return &_midiSyntis;   }
      MidiInPortList* midiInPorts()     { return &_midiInPorts;  }
      MidiChannelList* midiChannel()    { return &_midiChannel;  }

      bool trackExists(Track*) const;

      void removeTrack(Track* track);
      void removeTrack1(Track* track);
      void removeTrack2(Track* track);
      void removeTrack3(Track* track);
      void removeMarkedTracks();
      void changeTrackName(Track* track, const QString&);

      MidiTrack* findTrack(const Part* part) const;
      Track* findTrack(const QString& name) const;
      void swapTracks(int i1, int i2);
      void moveTrack(Track*, Track*);
      void insertTrack(Track*, int idx);
      void insertTrack0(Track*, int idx);
      void insertTrack1(Track*, int idx);
      void insertTrack2(Track*);
      void readRoute(QDomNode);
      void recordEvent(MidiTrack*, Event&);
      std::vector<QString>* synthesizer() const;

      void deselectTracks();
      void selectTrack(Track*);

      Track* selectedTrack() const { return _selectedTrack; }
      void updateSelectedTrack();

      //-----------------------------------------
      //   undo, redo
      //-----------------------------------------

      void startUndo();
      void endUndo(int);
      void undoOp(UndoOp::UndoType, int, Track*);
      void undoOp(UndoOp::UndoType, int, int, int = 0);
      void undoOp(UndoOp::UndoType, Part*);
      void undoOp(UndoOp::UndoType, Event& oevent, Event& nevent, Part*);
      void undoOp(UndoOp::UndoType, SigEvent* oevent, SigEvent* nevent);
      void undoOp(UndoOp::UndoType, int channel, int ctrl, int oval, int nval);
      void undoOp(UndoOp::UndoType, Part* oPart, Part* nPart);
      void undoOp(UndoOp::UndoType, Track*, int, unsigned, CVal, CVal);
      void undoOp(UndoOp::UndoType, Track*, const QString&, const QString&);
      void undoOp(UndoOp::UndoType type, const char* changedFile, const char* changeData, int startframe, int endframe);
      bool doUndo1();
      void doUndo2();
      void doUndo3();
      bool doRedo1();
      void doRedo2();
      void doRedo3();

      void addUndo(UndoOp& i);

      //-----------------------------------------
      //   Configuration
      //-----------------------------------------

//      void rescanAlsaPorts();

      //-----------------------------------------
      //   Controller
      //-----------------------------------------

      void addControllerVal(Track*, Ctrl*, const Pos&, CVal);
      void addControllerVal(Track*, int, const Pos&, CVal);
      void setControllerVal(Track*, Ctrl*, CVal);
      void setControllerVal(Track*, int, CVal);

      void removeControllerVal(Track*,int,unsigned);
      void setAutoRead(Track*,bool);
      void setAutoWrite(Track*,bool);

      //-----------------------------------------
      //   Misc/Debug
      //-----------------------------------------

      void setLen(int);
      void dumpMaster();
      void addUpdateFlags(int f)  { updateFlags |= f; }
      bool solo() const           { return soloFlag; }
      void setRecordFlag(Track*, bool);
      void setMute(Track*,bool);
      void setSolo(Track*,bool);
      void setOff(Track*,bool);

      QString projectPath() const;
      QString absoluteProjectPath() const;
      QString projectName() const;
      void setProjectPath(const QString&);
      QString comment() const           { return _comment; }
      void setComment(const QString& s) { _comment = s; }
      void setCreated(bool val)         { _created = val; }
      bool created() const              { return _created; }
      QDateTime createDate() const      { return _createDate; }

      bool read(QFile* qf);
      void read20(QDomNode node);
      void read10(QDomNode);
      void updateCurrentMarker();
      };

extern Song* song;

#endif

