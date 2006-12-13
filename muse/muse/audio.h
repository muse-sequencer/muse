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

#ifndef __AUDIO_H__
#define __AUDIO_H__

#include "thread.h"
#include "midievent.h"
#include "route.h"
#include "al/tempo.h"
#include "al/pos.h"
#include "event.h"
#include "ctrl.h"

namespace AL {
      class TimeSignature;
      };

using AL::Xml;
class SndFile;
class PluginI;
class MidiPluginI;
class SynthI;
class AudioDriver;
class Track;
class AudioTrack;
class Part;
class Event;
class MidiEvent;
class Event;
class MidiTrack;
class MidiSeq;
class MidiTrackBase;

//---------------------------------------------------------
//   GuiMessages
//    messages from sequencer to GUI
//    used in Audio::sendMsgToGui(char c)
//---------------------------------------------------------

#define MSG_STOP              '0'
#define MSG_PLAY              '1'
#define MSG_RECORD            '2'
#define MSG_SEEK              'G'
#define MSG_JACK_SHUTDOWN     'S'
#define MSG_START_BOUNCE      'f'
#define MSG_STOP_BOUNCE       'F'
#define MSG_GRAPH_CHANGED     'C'
#define MSG_ALSA_CHANGED      'P'

//---------------------------------------------------------
//   AudioMsgId
//    this are the messages send from the GUI thread to
//    the midi thread
//---------------------------------------------------------

enum {
      SEQM_ADD_TRACK,
      SEQM_REMOVE_TRACK,
      SEQM_MOVE_TRACK,
      SEQM_ADD_PART,
      SEQM_REMOVE_PART,
      SEQM_CHANGE_PART,
      SEQM_ADD_EVENT,
      SEQM_REMOVE_EVENT,
      SEQM_CHANGE_EVENT,
      SEQM_ADD_TEMPO,
      SEQM_SET_TEMPO,
      SEQM_REMOVE_TEMPO,
      SEQM_ADD_SIG,
      SEQM_REMOVE_SIG,
      SEQM_SET_GLOBAL_TEMPO,
      SEQM_UNDO,
      SEQM_REDO,
      SEQM_RESET_DEVICES,
      SEQM_INIT_DEVICES,
      //
      AUDIO_ROUTEADD,
      AUDIO_ROUTEREMOVE,
      AUDIO_ADDPLUGIN,
      AUDIO_ADDMIDIPLUGIN,
      AUDIO_SET_SEG_SIZE,
      AUDIO_SET_CHANNELS,

      MS_PROCESS,
      MS_START,
      MS_STOP,
      MS_SET_RTC,

      SEQM_IDLE,
      SEQM_ADD_CTRL,
      SEQM_REMOVE_CTRL
      };

extern const char* seqMsgList[];  // for debug

//---------------------------------------------------------
//   Msg
//---------------------------------------------------------

struct AudioMsg : public ThreadMsg {   // this should be an union
      int serialNo;
      SndFile* downmix;
      Route route;
      int ival;
      int iival;
      CVal cval1, cval2;
      PluginI* plugin;
      MidiPluginI* mplugin;
      SynthI* synth;
      Part* spart;
      Part* dpart;
      Track* track;

      const void *p1, *p2, *p3;
      Event ev1, ev2;
      char port, channel, ctrl;
      int a, b, c;
      Pos pos;
      unsigned time;
      };

class AudioOutput;


//---------------------------------------------------------
//   SeqTime
//    timing information for sequencer cycle
//---------------------------------------------------------

extern unsigned int segmentSize;

struct SeqTime {
      unsigned lastFrameTime; // free running counter

      // transport values for current cycle:
      Pos pos;                // current play position
      unsigned curTickPos;    // pos at start of frame during play/record
      unsigned nextTickPos;   // pos at start of next frame during play/record

      unsigned startFrame() const { return pos.frame(); }
      unsigned endFrame() const   { return startFrame() + segmentSize; }

      //---------------------------------------------------------
      //   tick2frame
      //    translate from tick to frameTime, this event has to
      //    be scheduled
      //---------------------------------------------------------

      unsigned tick2frame(unsigned tick) {
            return AL::tempomap.tick2frame(tick) - pos.frame() + lastFrameTime + segmentSize;
            }
      unsigned frame2tick(unsigned frame) {
            return AL::tempomap.frame2tick(frame - lastFrameTime + pos.frame());
            }
      };

//---------------------------------------------------------
//   Audio
//---------------------------------------------------------

class Audio {
   public:
      enum State {STOP, START_PLAY, PLAY, LOOP1, LOOP2, SYNC, PRECOUNT};

   private:
      bool recording;         // recording is active
      bool idle;              // do nothing in idle mode
      bool _freewheel;
      int _bounce;
      bool loopPassed;
      unsigned loopFrame;     // startframe of loop if in LOOP mode
      unsigned lmark;         // left loop position
      unsigned rmark;         // right loop position

      SeqTime _seqTime;
      Pos startRecordPos;
      Pos endRecordPos;

      int _curReadIndex;

      State state;
      bool updateController;

      AudioMsg* volatile msg;
      int fromThreadFdw, fromThreadFdr;  // message pipe

      int sigFd;              // pipe fd for messages to gui

      bool filterEvent(const MidiEvent* event, int type, bool thru);

      void startRolling();
      void stopRolling();

      void collectEvents(MidiTrack*, unsigned startTick, unsigned endTick);
      void processMsg();
      void initMidiDevices();
      void resetMidiDevices();

   public:
      Audio();
      virtual ~Audio() {}

      void process(unsigned frames, int jackState);
      bool sync(int state, unsigned frame);
      void shutdown();

      // transport:
      bool start();
      void stop();
      void seek(const Pos& pos);

      bool isPlaying() const    { return state == PLAY || state == LOOP1 || state == LOOP2; }
      bool isRecording() const  { return state == PLAY && recording; }

      //-----------------------------------------
      //   message interface
      //-----------------------------------------

      void msgSeek(const Pos&);
      void msgPlay(bool val);

      void msgRemoveTrack(Track*);
      void msgRemoveTracks();
      void msgMoveTrack(Track*, Track*);

      void msgAddEvent(const Event&, Part*, bool u = true);
      void msgDeleteEvent(const Event&, Part*, bool u = true);
      void msgChangeEvent(const Event&, const Event&, Part*, bool u = true);

      void msgAddTempo(int tick, int tempo, bool doUndoFlag = true);
      void msgSetTempo(int tick, int tempo, bool doUndoFlag = true);
      void msgSetGlobalTempo(int val);
      void msgDeleteTempo(int tick, int tempo, bool doUndoFlag = true);
      void msgAddSig(int tick, const AL::TimeSignature&, bool doUndoFlag = true);
      void msgRemoveSig(int tick, int z, int n, bool doUndoFlag = true);
      void msgPanic();
      void sendMsg(AudioMsg*);
      bool sendMessage(AudioMsg* m, bool doUndo);
      void msgRoute(bool add, Route);
      void msgRemoveRoute(Route);
      void msgRemoveRoute1(Route);
      void msgAddRoute(Route);
      void msgAddRoute1(Route);
      void msgAddPlugin(AudioTrack*, int idx, PluginI* plugin, bool prefader);
      void msgAddMidiPlugin(MidiTrackBase*, int idx, MidiPluginI* plugin);
      void msgSetMute(AudioTrack*, bool val);
      void msgAddSynthI(SynthI* synth);
      void msgRemoveSynthI(SynthI* synth);
      void msgSetSegSize(int, int);
      void msgSetChannels(AudioTrack*, int);
      void msgSetOff(AudioTrack*, bool);
      void msgUndo();
      void msgRedo();
      void msgLocalOff();
      void msgInitMidiDevices();
      void msgResetMidiDevices();
      void msgIdle(bool);
      void msgBounce();
      void msgAddController(Track*, int id, unsigned time, CVal);
      void msgRemoveController(Track*, int id, unsigned time);
      void msgSetRtc();

      const Pos& getStartRecordPos() const { return startRecordPos; }
      const Pos& getEndRecordPos() const { return endRecordPos; }

      bool freewheel() const       { return _freewheel; }
      void setFreewheel(bool val);

      void sendMsgToGui(char c);
      bool bounce() const { return _bounce != 0; }
      MidiEvent* getMidiEvent();
      void popMidiEvent();
      int curReadIndex() const       { return _curReadIndex;  }

      const SeqTime* seqTime() const { return &_seqTime;      }
      };

extern int processAudio(unsigned long, void*);
extern void processAudio1(void*, void*);

extern Audio* audio;
class AudioDriver;
extern AudioDriver* audioDriver;   // current audio device in use
#endif

