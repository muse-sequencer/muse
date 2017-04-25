//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: audio.h,v 1.25.2.13 2009/12/20 05:00:35 terminator356 Exp $
//
//  (C) Copyright 2001 Werner Schweer (ws@seh.de)
//  (C) Copyright 2011 Tim E. Real (terminator356 on users dot sourceforge dot net)
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

#ifndef __AUDIO_H__
#define __AUDIO_H__

#include "type_defs.h"
#include "thread.h"
#include "pos.h"
#include "mpevent.h"
#include "route.h"
#include "event.h"

// An experiment to use true frames for time-stamping all recorded input. 
// (All recorded data actually arrived in the previous period.)
// TODO: Some more work needs to be done in WaveTrack::getData() in order to
//  make everything line up and sync correctly. Cannot use this yet!
//#define _AUDIO_USE_TRUE_FRAME_

namespace MusECore {
class AudioDevice;
class AudioTrack;
class Event;
class Event;
class EventList;
class MidiDevice;
class MidiInstrument;
class MidiPlayEvent;
class MidiPort;
class MidiTrack;
class Part;
class PluginI;
class SynthI;
class Track;
class Undo;
class PendingOperationList;

//---------------------------------------------------------
//   AudioMsgId
//    this are the messages send from the GUI thread to
//    the midi thread
//---------------------------------------------------------

enum {
      SEQM_REVERT_OPERATION_GROUP, SEQM_EXECUTE_OPERATION_GROUP, 
      SEQM_EXECUTE_PENDING_OPERATIONS,
      SEQM_RESET_DEVICES, SEQM_INIT_DEVICES, SEQM_PANIC,
      SEQM_MIDI_LOCAL_OFF,
      SEQM_PLAY_MIDI_EVENT,
      SEQM_SET_HW_CTRL_STATE,
      SEQM_SET_HW_CTRL_STATES,
      SEQM_SET_TRACK_AUTO_TYPE,
      SEQM_SET_AUX,
      SEQM_UPDATE_SOLO_STATES,
      AUDIO_RECORD,
      AUDIO_RECORD_MONITOR,
      AUDIO_ROUTEADD, AUDIO_ROUTEREMOVE, AUDIO_REMOVEROUTES,
      AUDIO_ADDPLUGIN,
      AUDIO_SET_PREFADER, AUDIO_SET_CHANNELS,
      AUDIO_SWAP_CONTROLLER_IDX,
      AUDIO_CLEAR_CONTROLLER_EVENTS,
      AUDIO_SEEK_PREV_AC_EVENT,
      AUDIO_SEEK_NEXT_AC_EVENT,
      AUDIO_ERASE_AC_EVENT,
      AUDIO_ERASE_RANGE_AC_EVENTS,
      AUDIO_ADD_AC_EVENT,
      AUDIO_CHANGE_AC_EVENT,
      AUDIO_SET_SOLO, AUDIO_SET_MUTE, AUDIO_SET_TRACKOFF,
      AUDIO_SET_SEND_METRONOME,
      AUDIO_START_MIDI_LEARN,
      MS_PROCESS, MS_STOP, MS_SET_RTC, MS_UPDATE_POLL_FD,
      SEQM_IDLE, SEQM_SEEK,
      AUDIO_WAIT  // Do nothing. Just wait for an audio cycle to pass.
      };

extern const char* seqMsgList[];  // for debug

//---------------------------------------------------------
//   Msg
//---------------------------------------------------------

struct AudioMsg : public ThreadMsg {   // this should be an union
      int serialNo;
      //SndFile* downmix; // DELETETHIS this is unused and probably WRONG (all SndFiles have been replaced by SndFileRs)
      AudioTrack* snode;
      AudioTrack* dnode;
      Route sroute, droute;
      AudioDevice* device;
      int ival;
      int iival;
      double dval;
      PluginI* plugin;
      SynthI* synth;
      Part* spart;
      Part* dpart;
      Track* track;

      const void *p1, *p2, *p3;
      Event ev1, ev2;
      char port, channel, ctrl;
      int a, b, c;
      Pos pos;
      Undo* operations;
      PendingOperationList* pendingOps;
      };

//---------------------------------------------------------
//   Audio
//---------------------------------------------------------

class Audio {
   public:
      enum State {STOP, START_PLAY, PLAY, LOOP1, LOOP2, SYNC, PRECOUNT};

   private:
      bool _running;          // audio is active
      bool recording;         // recording is active
      bool idle;              // do nothing in idle mode
      bool _freewheel;
      bool _bounce;
      unsigned _loopFrame;     // Startframe of loop if in LOOP mode. Not quite the same as left marker !
      int _loopCount;         // Number of times we have looped so far

      Pos _pos;               // current play position
      
#ifdef _AUDIO_USE_TRUE_FRAME_
      Pos _previousPos;       // previous play position
#endif
      
      unsigned curTickPos;   // pos at start of frame during play/record
      unsigned nextTickPos;  // pos at start of next frame during play/record

      //metronome values
      unsigned midiClick;
      int clickno;      // precount values
      int clicksMeasure;
      int ticksBeat;

      double syncTime;  // wall clock at last sync point
      unsigned syncFrame;    // corresponding frame no. to syncTime
      unsigned long frameOffset;  // offset to free running hw frame counter

      State state;

      AudioMsg* msg;
      int fromThreadFdw, fromThreadFdr;  // message pipe

      int sigFd;              // pipe fd for messages to gui
      int sigFdr;
      
      // record values:
      Pos startRecordPos;
      Pos endRecordPos;
      unsigned startExternalRecTick;
      unsigned endExternalRecTick;

      long m_Xruns;
      
      void sendLocalOff();
      bool filterEvent(const MidiPlayEvent* event, int type, bool thru);

      void startRolling();
      void stopRolling();

      void panic();
      void processMsg(AudioMsg* msg);
      void process1(unsigned samplePos, unsigned offset, unsigned samples);

      void collectEvents(MidiTrack*, unsigned int startTick, unsigned int endTick);

   public:
      Audio();
      virtual ~Audio() { } 

      // Access to message pipe (like from gui namespace), otherwise audio would need to depend on gui.
      int getFromThreadFdw() { return sigFd; } 
      int getFromThreadFdr() { return sigFdr; }  
      
      void process(unsigned frames);
      bool sync(int state, unsigned frame);
      // Called whenever the audio needs to re-sync, such as after any tempo changes.
      void reSyncAudio();
      void shutdown();
      void writeTick();

      // transport:
      bool start();
      void stop(bool);
      void seek(const Pos& pos);

      bool isStarting() const   { return state == START_PLAY; }
      bool isPlaying() const    { return state == PLAY || state == LOOP1 || state == LOOP2; }
      bool isRecording() const  { return state == PLAY && recording; }
      void setRunning(bool val) { _running = val; }
      bool isRunning() const    { return _running; }
      bool isIdle() const { return idle; }

      //-----------------------------------------
      //   message interface
      //-----------------------------------------

      void msgSeek(const Pos&);
      void msgPlay(bool val);

      void msgExecuteOperationGroup(Undo&); // calls exe1, then calls exe2 in audio context, then calls exe3
      void msgRevertOperationGroup(Undo&); // similar.
      // Bypass the Undo system and directly execute the pending operations.
      // Do a song update with accumulated flags and extra_flags, if doUpdate is true.
      void msgExecutePendingOperations(PendingOperationList& operations, bool doUpdate = false, SongChangedFlags_t extraFlags = 0);

      void msgRemoveTracks();
      void msgRemoveTrack(Track*, bool u = true); // only does applyOperation
      void msgMoveTrack(int idx1, int dx2, bool u = true); // only does applyOperation
      void msgAddPart(Part*, bool u = true); // only does applyOperation
      void msgRemovePart(Part*, bool u = true); // only does applyOperation
      void msgAddEvent(Event&, Part*, bool u = true, bool doCtrls = true, bool doClones = false); // only does applyOperation
      void msgDeleteEvent(Event&, Part*, bool u = true, bool doCtrls = true, bool doClones = false); // only does applyOperation
      void msgChangeEvent(Event&, Event&, Part*, bool u = true, bool doCtrls = true, bool doClones = false); // only does applyOperation
      void msgAddTempo(int tick, int tempo, bool doUndoFlag = true); // only does applyOperation
      void msgSetTempo(int tick, int tempo, bool doUndoFlag = true); // FIXME FINDMICHJETZT TODO!
      void msgDeleteTempo(int tick, int tempo, bool doUndoFlag = true); // only does applyOperation
      void msgUpdateSoloStates(); // TODO and below
      void msgSetAux(AudioTrack*, int, double);
      void msgSetGlobalTempo(int val, bool doUndoFlag = true);
      void msgAddSig(int tick, int z, int n, bool doUndoFlag = true);
      void msgRemoveSig(int tick, int z, int n, bool doUndoFlag = true);
      void msgAddKey(int tick, int key, bool doUndoFlag = true);
      void msgRemoveKey(int tick, int key, bool doUndoFlag = true);
      void msgPanic();
      void sendMsg(AudioMsg*);
      bool sendMessage(AudioMsg* m, bool doUndo);
      void msgRemoveRoute(Route, Route);
      void msgRemoveRoute1(Route, Route); 
      void msgAddRoute(Route, Route);
      void msgAddRoute1(Route, Route);
      void msgAddPlugin(AudioTrack*, int idx, PluginI* plugin);
      void msgSetPrefader(AudioTrack*, int);
      void msgSetChannels(AudioTrack*, int);
      void msgSetRecord(Track*, bool);
      void msgSetRecMonitor(Track*, bool);
      void msgLocalOff();
      void msgInitMidiDevices(bool force = true);
      void msgResetMidiDevices();
      void msgIdle(bool);
      void msgAudioWait();
      void msgBounce();
      void msgSwapControllerIDX(AudioTrack*, int, int);
      void msgClearControllerEvents(AudioTrack*, int);
      void msgSeekPrevACEvent(AudioTrack*, int);
      void msgSeekNextACEvent(AudioTrack*, int);
      void msgEraseACEvent(AudioTrack*, int, int);
      void msgEraseRangeACEvents(AudioTrack*, int, int, int);
      void msgAddACEvent(AudioTrack*, int, int, double);
      void msgChangeACEvent(AudioTrack* node, int acid, int frame, int newFrame, double val);
      void msgSetSolo(Track*, bool);
      void msgSetTrackMute(Track*, bool);
      void msgSetTrackOff(Track*, bool);
      void msgSetHwCtrlState(MidiPort*, int, int, int);
      void msgSetHwCtrlStates(MidiPort*, int, int, int, int);
      void msgSetTrackAutomationType(Track*, int);
      void msgSetSendMetronome(AudioTrack*, bool);
      void msgStartMidiLearn();
      void msgPlayMidiEvent(const MidiPlayEvent* event);
      void msgSetMidiDevice(MidiPort* port, MidiDevice* device);

      void midiPortsChanged();

      const Pos& pos() const { return _pos; }
#ifdef _AUDIO_USE_TRUE_FRAME_
      const Pos& previousPos() const { return _previousPos; }
#endif
      const Pos& getStartRecordPos() const { return startRecordPos; }
      const Pos& getEndRecordPos() const { return endRecordPos; }
      unsigned getStartExternalRecTick() const { return startExternalRecTick; }
      unsigned getEndExternalRecTick() const { return endExternalRecTick; }
      int loopCount() { return _loopCount; }         // Number of times we have looped so far
      unsigned loopFrame() { return _loopFrame; }          

      unsigned tickPos() const    { return curTickPos; }
      unsigned nextTick() const   { return nextTickPos; }
      unsigned timestamp() const;
      void processMidi();
      unsigned curFrame() const;
      unsigned curSyncFrame() const { return syncFrame; }
      unsigned curFramePos() const;
      // This is meant to be called from inside process thread only.      
      unsigned framesAtCycleStart() const;
      // This can be called from outside process thread. 
      unsigned framesSinceCycleStart() const;   
      
      void recordStop(bool restart = false, Undo* operations = NULL);
      bool freewheel() const       { return _freewheel; }
      void setFreewheel(bool val);
      unsigned long getFrameOffset() const   { return frameOffset; }
      void initDevices(bool force = true);

      void sendMsgToGui(char c);
      bool bounce() const { return _bounce; }

      long getXruns() { return m_Xruns; }
      void resetXruns() { m_Xruns = 0; }
      void incXruns() { m_Xruns++; }

      };

extern int processAudio(unsigned long, void*);
extern void processAudio1(void*, void*);

} // namespace MusECore

namespace MusEGlobal {
extern MusECore::Audio* audio;
extern MusECore::AudioDevice* audioDevice;   // current audio device in use
}

#endif

