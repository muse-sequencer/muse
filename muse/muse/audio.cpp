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

#include <fcntl.h>

#include "al/al.h"
#include "muse.h"
#include "globals.h"
#include "song.h"
#include "driver/audiodev.h"
#include "audioprefetch.h"
#include "audiowriteback.h"
#include "audio.h"
#include "midiseq.h"
#include "sync.h"
#include "midi.h"
#include "gconfig.h"
#include "al/sig.h"
#include "al/tempo.h"
#include "widgets/utils.h"
#include "synth.h"
#include "midioutport.h"
#include "midiinport.h"

extern double curTime();
extern bool initJackAudio();

Audio* audio;
AudioDriver* audioDriver;   // current audio device in use

const char* seqMsgList[] = {
      "SEQM_ADD_TRACK",
      "SEQM_REMOVE_TRACK",
      "SEQM_MOVE_TRACK",
      "SEQM_ADD_PART",
      "SEQM_REMOVE_PART",
      "SEQM_CHANGE_PART",
      "SEQM_ADD_EVENT",
      "SEQM_REMOVE_EVENT",
      "SEQM_CHANGE_EVENT",
      "SEQM_ADD_TEMPO",
/*10*/"SEQM_SET_TEMPO",
      "SEQM_REMOVE_TEMPO",
      "SEQM_ADD_SIG",
      "SEQM_REMOVE_SIG",
      "SEQM_SET_GLOBAL_TEMPO",
      "SEQM_UNDO",
      "SEQM_REDO",
      "SEQM_RESET_DEVICES",
      "SEQM_INIT_DEVICES",
      "AUDIO_ROUTEADD",
	"AUDIO_ROUTEREMOVE",
/*20*/"AUDIO_ADDPLUGIN",
      "AUDIO_ADDMIDIPLUGIN",
      "AUDIO_SET_SEG_SIZE",
      "AUDIO_SET_CHANNELS",
      "MS_PROCESS",
      "MS_START",
      "MS_STOP",
      "MS_SET_RTC",
      "SEQM_IDLE",
      "SEQM_ADD_CTRL",
      "SEQM_REMOVE_CTRL"
      };

const char* audioStates[] = {
      "STOP", "START_PLAY", "PLAY", "LOOP1", "LOOP2", "SYNC", "PRECOUNT"
      };


//---------------------------------------------------------
//   Audio
//---------------------------------------------------------

Audio::Audio()
      {
      recording     = false;
      idle          = false;
      _freewheel    = false;
      _bounce       = 0;
      loopPassed    = false;

      _pos.setType(AL::FRAMES);
      _pos.setFrame(~0);      // make sure first seek is not optimized away

      _curTickPos   = 0;
      _nextTickPos  = 0;

      midiClick     = 0;
      clickno       = 0;
      clicksMeasure = 0;
      ticksBeat     = 0;

      state         = STOP;
      lmark         = 0;      // left loop position
      rmark         = 0;      // right loop position
      msg           = 0;

      startRecordPos.setType(AL::TICKS);
      endRecordPos.setType(AL::TICKS);

      //---------------------------------------------------
      //  establish pipes/sockets
      //---------------------------------------------------

      int filedes[2];         // 0 - reading   1 - writing
      if (pipe(filedes) == -1) {
            perror("creating pipe0");
            fatalError("cannot create pipe0");
            }
      fromThreadFdw = filedes[1];   // blocking file descriptor
      fromThreadFdr = filedes[0];   // non blocking file descriptor

      int rv = fcntl(fromThreadFdw, F_SETFL, O_NONBLOCK);
      if (rv == -1)
            perror("set pipe O_NONBLOCK");

      if (pipe(filedes) == -1) {
            perror("creating pipe1");
            fatalError("cannot create pipe1");
            }
      sigFd = filedes[1];
      QSocketNotifier* ss = new QSocketNotifier(filedes[0], QSocketNotifier::Read);
      song->connect(ss, SIGNAL(activated(int)), song, SLOT(seqSignal(int)));
      }

//---------------------------------------------------------
//   start
//    start audio processing
//---------------------------------------------------------

bool Audio::start()
      {
      TrackList* tl = song->tracks();

      _curTickPos   = 0;
      _nextTickPos  = 0;

      midiClick     = 0;
      clickno       = 0;
      clicksMeasure = 0;
      ticksBeat     = 0;

      msg           = 0;
      _pos.setFrame(~0);      // make sure seek is not optimized away

      //
      // init marker for synchronous loop processing
      //
      lmark = song->lPos().frame();
      rmark = song->rPos().frame();
      state = STOP;
      muse->setHeartBeat();

      if (audioDriver) {
          //
          // allocate ports
          //
          for (iTrack i = tl->begin(); i != tl->end(); ++i)
                (*i)->activate1();
          seek(song->cpos());
          process(segmentSize, STOP);   // warm up caches; audio must be stopped
          audioDriver->start(realTimePriority);
          }
      else {

          // if audio device has disappeared it probably
          // means jack has performed a shutdown
          // try to restart and reconnect everything

          if (false == initJackAudio()) {
                //
                // allocate ports, first resetting the old connection
                //
                InputList* itl = song->inputs();
                for (iAudioInput i = itl->begin(); i != itl->end(); ++i) {
//                      printf("reconnecting input %s\n", (*i)->name().toLatin1().data());
                      for (int x=0; x < (*i)->channels();x++) {
                          (*i)->setJackPort(Port(), x); // zero out the old connection
                          }
                      (*i)->activate1();
                      }

                OutputList* otl = song->outputs();
                for (iAudioOutput i = otl->begin(); i != otl->end(); ++i) {
//                      printf("reconnecting output %s\n", (*i)->name().toLatin1().data());
                      for (int x=0; x < (*i)->channels();x++)
                          (*i)->setJackPort(Port(), x);  // zero out the old connection
                      (*i)->activate1();
                      }
               audioDriver->start(realTimePriority);
               }
          else {
               printf("Failed to init audio!\n");
               return false;
               }
          }
      audioDriver->stopTransport();
      return true;
      }

//---------------------------------------------------------
//   stop
//    stop audio processing
//   non realtime
//---------------------------------------------------------

void Audio::stop()
      {
#if 0
      MidiOutPortList* opl = song->midiOutPorts();
      for (iMidiOutPort i = opl->begin(); i != opl->end(); ++i)
            (*i)->deactivate();
      MidiInPortList* ipl = song->midiInPorts();
      for (iMidiInPort i = ipl->begin(); i != ipl->end(); ++i)
            (*i)->deactivate();
#endif
      if (audioDriver)
          audioDriver->stop();
      }

//---------------------------------------------------------
//   sync
//    return true if sync is completed
//---------------------------------------------------------

bool Audio::sync(int jackState, unsigned frame)
      {
// printf("Audio::sync() state %s jackState %s frame 0x%x\n", audioStates[state], audioStates[jackState], frame);
      bool done = true;
      if (state == LOOP1)
            state = LOOP2;
      else {
            State s = State(jackState);
            //
            //  STOP -> START_PLAY	start rolling
            //  STOP -> STOP		seek in stop state
            //  PLAY -> START_PLAY  seek in play state

            if (state != START_PLAY) {
            	Pos p(frame, AL::FRAMES);
//                  printf("  sync seek\n");
	            seek(p);
      	      if (!_freewheel)
            	      done = audioPrefetch->seekDone();
              	if (s == START_PLAY)
              		state = START_PLAY;
              	}
            else {
            	if (frame != _pos.frame()) {
                  	// seek during seek
		            seek(Pos(frame, AL::FRAMES));
                  	}
            	done = audioPrefetch->seekDone();
                  }
            }
//      printf("  sync %s\n", done ? "done" : "busy");
      return done;
      }

//---------------------------------------------------------
//   setFreewheel
//---------------------------------------------------------

void Audio::setFreewheel(bool val)
      {
// printf("JACK: freewheel callback %d\n", val);
      _freewheel = val;
      }

//---------------------------------------------------------
//   shutdown
//---------------------------------------------------------

void Audio::shutdown()
      {
      audioState = AUDIO_STOP;
printf("JACK: shutdown callback\n");
      sendMsgToGui(MSG_JACK_SHUTDOWN);
      }

//---------------------------------------------------------
//   process
//    process one audio buffer at position "_pos "
//    of size "frames"
//---------------------------------------------------------

void Audio::process(unsigned frames, int jackState)
      {
// printf("process %d\n", frames);
      extern int watchAudio;
      ++watchAudio;           // make a simple watchdog happy

      if (msg) {
            processMsg();
            msg    = 0;    // dont process again
            int rv = write(fromThreadFdw, "x", 1);
            if (rv != 1) {
                  fprintf(stderr, "audio: write(%d) pipe failed: %s\n",
                     fromThreadFdw, strerror(errno));
                  }
            }

      if (jackState != state) {
            if (state == START_PLAY && jackState == PLAY) {
                  startRolling();
                  if (_bounce)
                        sendMsgToGui(MSG_START_BOUNCE);
                  }
            else if (state == LOOP2 && jackState == PLAY) {
                  Pos newPos(loopFrame, AL::FRAMES);
                  seek(newPos);
                  startRolling();
                  }
            else if (isPlaying() && jackState == STOP)
                  stopRolling();
            else if (state == START_PLAY && jackState == STOP) {
                  state = STOP;
                  updateController = true;
                  if (_bounce)
                        audioDriver->startTransport();
                  else
                        sendMsgToGui(MSG_STOP);
                  }
            else if (state == STOP && jackState == PLAY)
                  startRolling();
            }

      OutputList* ol = song->outputs();
      if (idle || state == START_PLAY) {
            // deliver silence
            for (iAudioOutput i = ol->begin(); i != ol->end(); ++i)
                  (*i)->silence(frames);
            return;
            }
      unsigned framePos = _pos.frame();

      if (state == PLAY) {
            //
            // clear prefetch FIFO if left/right locators
            // have changed
            //
            unsigned llmark = song->lPos().frame();
            unsigned rrmark = song->rPos().frame();

            if (lmark != llmark || rmark != rrmark) {
                  //
                  // invalidate audio prefetch buffer
                  //
                  audioPrefetch->getFifo()->clear();
                  audioPrefetch->msgSeek(framePos);
                  lmark = llmark;
                  rmark = rrmark;
                  }
            }

      if (isPlaying()) {
            if (_bounce == 1 && _pos >= song->rPos()) {
                  _bounce = 2;
                  sendMsgToGui(MSG_STOP_BOUNCE);
                  }
            //
            //  check for end of song
            //
            if ((_curTickPos >= song->len())
               && !(song->record() || _bounce || song->loop())) {
                  audioDriver->stopTransport();
                  return;
                  }

            //
            //  check for loop end
            //
            if (state == PLAY && song->loop() && !_bounce && !extSyncFlag) {
                  unsigned n = rmark - framePos - (3 * frames);
                  if (n < frames) {
                        // loop end in current cycle
                        // adjust loop start so we get exact loop len
                        if (n > lmark)
                              n = 0;
                        state = LOOP1;
                        loopFrame = lmark - n;
                        audioDriver->seekTransport(loopFrame);
                        }
                  }

            Pos ppp(_pos);
            ppp += frames;
            _nextTickPos = ppp.tick();
            }

      //
      //  compute current controller values
      //  (automation)
      //
      TrackList* tl = song->tracks();
      if (state == PLAY || updateController) {
            updateController = false;
            for (iTrack it = tl->begin(); it != tl->end(); ++it) {
                  if ((*it)->isMidiTrack())
                        continue;
                  AudioTrack* track = (AudioTrack*)(*it);
                  if (!track->autoRead())
                        continue;
                  CtrlList* cl = track->controller();
                  for (iCtrl i = cl->begin(); i != cl->end(); ++i) {
                        Ctrl* c = i->second;
                        float val = c->value(framePos).f;
                        if (val != c->curVal().f) {
                              c->setCurVal(val);
                              c->setChanged(true);
                              }
                        }
                  }
            }

      MidiOutPortList* mol = song->midiOutPorts();
      for (iMidiOutPort i = mol->begin(); i != mol->end(); ++i)
            audioDriver->startMidiCycle((*i)->jackPort(0));

      processMidi(frames);

      GroupList* gl     = song->groups();
      SynthIList* sl    = song->syntis();
      InputList* il     = song->inputs();
      WaveTrackList* wl = song->waves();

      for (iAudioInput i = il->begin(); i != il->end(); ++i)
            (*i)->process();
      for (iSynthI i = sl->begin(); i != sl->end(); ++i)
            (*i)->process();

      _curReadIndex = -1;
      if (isPlaying() && !wl->empty()) {
   	      Fifo1* fifo = audioPrefetch->getFifo();
      	if (fifo->count() == 0) {
            	printf("MusE::Audio: fifo underflow at 0x%x\n", _curTickPos);
                  audioPrefetch->msgTick();
                  }
            else {
                  bool msg = true;
                  do {
	                  unsigned fifoPos = fifo->readPos();
                  	if (fifoPos == framePos) {
                  		_curReadIndex = fifo->readIndex();
                              break;
                              }
                        else {
                              if (msg) {
	                              printf("Muse::Audio: wrong prefetch data 0x%x, expected 0x%x\n",
      	                     	   fifoPos, framePos);
                              	msg = false;
                                    }
                              if (fifoPos > framePos) {
                                    // discard whole prefetch buffer
                                    seek(_pos + frames);
                                    break;
                                    }
                              fifo->pop();      // discard buffer
                              }
              		} while (fifo->count());
                  }
            }
      for (iWaveTrack i = wl->begin(); i != wl->end(); ++i) {
      	if (song->bounceTrack != *i)
            	(*i)->process();
            }
      for (iAudioGroup i = gl->begin(); i != gl->end(); ++i)
            (*i)->process();
      for (iAudioOutput i = ol->begin(); i != ol->end(); ++i)
            (*i)->process();

      if (_bounce == 1 && song->bounceTrack && song->bounceTrack->type() == Track::WAVE)
            song->bounceTrack->process();

      if (isPlaying()) {
      	if (!freewheel()) {
                  //
                  // consume prefetch buffer
                  //
                  if (_curReadIndex != -1) {
                        audioPrefetch->getFifo()->pop();
			      audioPrefetch->msgTick();  // wakeup prefetch thread
                        }
                  }
            if (recording && (_bounce == 0 || _bounce == 1))
                  audioWriteback->trigger();
            _pos       += frames;
            _curTickPos = _nextTickPos;
            }
      }

//---------------------------------------------------------
//   processMsg
//---------------------------------------------------------

void Audio::processMsg()
      {
// printf("---msg %d\n", msg->id);
      switch(msg->id) {
            case AUDIO_ROUTEADD:
                  addRoute(msg->sroute, msg->droute);
                  break;
            case AUDIO_ROUTEREMOVE:
                  removeRoute(msg->sroute, msg->droute);
                  break;
            case AUDIO_SET_CHANNELS:
                  msg->track->setChannels(msg->ival);
                  break;
            case AUDIO_ADDPLUGIN:
                  ((AudioTrack*)msg->track)->addPlugin(msg->plugin, msg->ival,
                     msg->iival);
                  break;

            case AUDIO_SET_SEG_SIZE:
                  segmentSize = msg->ival;
                  AL::sampleRate  = msg->iival;
#if 0 //TODO
                  audioOutput.segmentSizeChanged();
                  for (int i = 0; i < mixerGroups; ++i)
                        audioGroups[i].segmentSizeChanged();
                  for (iSynthI ii = synthiInstances.begin(); ii != synthiInstances.end();++ii)
                        (*ii)->segmentSizeChanged();
#endif
                  break;

            case SEQM_INIT_DEVICES:
                  initDevices();
                  break;
            case SEQM_ADD_TEMPO:
            case SEQM_REMOVE_TEMPO:
            case SEQM_SET_GLOBAL_TEMPO:
            case SEQM_SET_TEMPO:
                  song->processMsg(msg);
                  if (isPlaying())
                        _pos.setTick(_curTickPos);
                  break;

            case SEQM_IDLE:
                  idle = msg->a;
                  // fall through
            default:
                  midiSeq->sendMsg(msg);
                  break;
            }
      }

//---------------------------------------------------------
//   seek
//    - called before start play
//    - initiated from gui
//---------------------------------------------------------

void Audio::seek(const Pos& p)
      {
      _pos.setFrame(p.frame());
      _curTickPos      = _pos.tick();
      _nextTickPos     = _curTickPos;
      updateController = true;

      loopPassed = true;      // for record loop mode
      if (state != LOOP2 && !freewheel())
            audioPrefetch->msgSeek(_pos.frame());

      midiBusy = true;
      midiSeq->processSeek(); // handle stuck notes and set
                              // controller for new position
      midiBusy = false;

      sendMsgToGui(MSG_SEEK);
      }

//---------------------------------------------------------
//   startRolling
//---------------------------------------------------------

void Audio::startRolling()
      {
      startRecordPos = _pos;
      if (song->record()) {
            startRecordPos = _pos;
            recording      = true;
            TrackList* tracks = song->tracks();
            for (iTrack i = tracks->begin(); i != tracks->end(); ++i) {
                  if ((*i)->isMidiTrack())
                        continue;
                  if ((*i)->type() == Track::WAVE)
                        ((WaveTrack*)(*i))->resetMeter();
                  ((AudioTrack*)(*i))->recEvents()->clear();
                  }
            }
      state = PLAY;
      sendMsgToGui(MSG_PLAY);

      midiSeq->msgStart();

      if (precountEnableFlag
         && song->click()
         && !extSyncFlag
         && song->record()) {
#if 0
            state = PRECOUNT;
            int z, n;
            if (precountFromMastertrackFlag)
                  AL::sigmap.timesig(playTickPos, z, n);
            else {
                  z = precountSigZ;
                  n = precountSigN;
                  }
            clickno       = z * preMeasures;
            clicksMeasure = z;
            ticksBeat     = (division * 4)/n;
#endif
            }
      else {
            //
            // compute next midi metronome click position
            //
            int bar, beat;
            unsigned tick;
            AL::sigmap.tickValues(_curTickPos, &bar, &beat, &tick);
            if (tick)
                  beat += 1;
            midiClick = AL::sigmap.bar2tick(bar, beat, 0);
            }
      }

//---------------------------------------------------------
//   stopRolling
//	execution environment: realtime thread
//---------------------------------------------------------

void Audio::stopRolling()
      {
      state = STOP;
      midiSeq->msgStop();
	recording    = false;
      endRecordPos = _pos;
      _bounce = 0;
      sendMsgToGui(MSG_STOP);
      seek(_pos);
      }

//---------------------------------------------------------
//   sendMsgToGui
//---------------------------------------------------------

void Audio::sendMsgToGui(char c)
      {
      write(sigFd, &c, 1);
      }

