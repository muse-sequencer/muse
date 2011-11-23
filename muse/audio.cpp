//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: audio.cpp,v 1.59.2.30 2009/12/20 05:00:35 terminator356 Exp $
//
//  (C) Copyright 2001-2004 Werner Schweer (ws@seh.de)
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

#include <cmath>
#include <errno.h>

#include <QSocketNotifier>

#include "app.h"
#include "song.h"
#include "node.h"
#include "audiodev.h"
#include "mididev.h"
#include "alsamidi.h"
#include "synth.h"
#include "audioprefetch.h"
#include "plugin.h"
#include "audio.h"
#include "wave.h"
#include "midictrl.h"
#include "midiseq.h"
#include "sync.h"
#include "midi.h"
#include "event.h"
#include "gconfig.h"
#include "pos.h"
#include "ticksynth.h"


namespace MusEGlobal {
MusECore::Audio* audio;
MusECore::AudioDevice* audioDevice;   // current audio device in use
extern unsigned int volatile midiExtSyncTicks;   // p3.3.25
}

namespace MusECore {
extern double curTime();

//static const unsigned char mmcDeferredPlayMsg[] = { 0x7f, 0x7f, 0x06, 0x03 };
//static const unsigned char mmcStopMsg[] =         { 0x7f, 0x7f, 0x06, 0x01 };

const char* seqMsgList[] = {
      "SEQM_ADD_TRACK", "SEQM_REMOVE_TRACK", "SEQM_CHANGE_TRACK", "SEQM_MOVE_TRACK",
      "SEQM_ADD_PART", "SEQM_REMOVE_PART", "SEQM_CHANGE_PART",
      "SEQM_ADD_EVENT", "SEQM_REMOVE_EVENT", "SEQM_CHANGE_EVENT",
      "SEQM_ADD_TEMPO", "SEQM_SET_TEMPO", "SEQM_REMOVE_TEMPO", "SEQM_ADD_SIG", "SEQM_REMOVE_SIG",
      "SEQM_SET_GLOBAL_TEMPO",
      "SEQM_UNDO", "SEQM_REDO",
      "SEQM_RESET_DEVICES", "SEQM_INIT_DEVICES", "SEQM_PANIC",
      "SEQM_MIDI_LOCAL_OFF",
      "SEQM_SET_MIDI_DEVICE",
      "SEQM_PLAY_MIDI_EVENT",
      "SEQM_SET_HW_CTRL_STATE",
      "SEQM_SET_HW_CTRL_STATES",
      "SEQM_SET_TRACK_OUT_PORT",
      "SEQM_SET_TRACK_OUT_CHAN",
      "SEQM_REMAP_PORT_DRUM_CTL_EVS",
      "SEQM_CHANGE_ALL_PORT_DRUM_CTL_EVS",
      "SEQM_SCAN_ALSA_MIDI_PORTS",
      "SEQM_SET_AUX",
      "SEQM_UPDATE_SOLO_STATES",
      //"MIDI_SHOW_INSTR_GUI",
      //"MIDI_SHOW_INSTR_NATIVE_GUI",
      "AUDIO_RECORD",
      "AUDIO_ROUTEADD", "AUDIO_ROUTEREMOVE", "AUDIO_REMOVEROUTES",
      //"AUDIO_VOL", "AUDIO_PAN",
      "AUDIO_ADDPLUGIN",
      "AUDIO_SET_SEG_SIZE",
      "AUDIO_SET_PREFADER", "AUDIO_SET_CHANNELS",
      //"AUDIO_SET_PLUGIN_CTRL_VAL",
      "AUDIO_SWAP_CONTROLLER_IDX",
      "AUDIO_CLEAR_CONTROLLER_EVENTS",
      "AUDIO_SEEK_PREV_AC_EVENT",
      "AUDIO_SEEK_NEXT_AC_EVENT",
      "AUDIO_ERASE_AC_EVENT",
      "AUDIO_ERASE_RANGE_AC_EVENTS",
      "AUDIO_ADD_AC_EVENT",
      "AUDIO_CHANGE_AC_EVENT",
      "AUDIO_SET_SOLO", "AUDIO_SET_SEND_METRONOME", 
      "MS_PROCESS", "MS_STOP", "MS_SET_RTC", "MS_UPDATE_POLL_FD",
      "SEQM_IDLE", "SEQM_SEEK"
      };

const char* audioStates[] = {
      "STOP", "START_PLAY", "PLAY", "LOOP1", "LOOP2", "SYNC", "PRECOUNT"
      };


//---------------------------------------------------------
//   Audio
//---------------------------------------------------------

Audio::Audio()
      {
      _running      = false;
      recording     = false;
      idle          = false;
      _freewheel    = false;
      _bounce       = false;
      //loopPassed    = false;
      _loopFrame    = 0;
      _loopCount    = 0;

      _pos.setType(Pos::FRAMES);
      _pos.setFrame(0);
      nextTickPos = curTickPos = 0;

      midiClick     = 0;
      clickno       = 0;
      clicksMeasure = 0;
      ticksBeat     = 0;

      syncTime      = 0.0;
      syncFrame     = 0;
      frameOffset   = 0;

      state         = STOP;
      msg           = 0;

      //startRecordPos.setType(Pos::TICKS);
      //endRecordPos.setType(Pos::TICKS);
      startRecordPos.setType(Pos::FRAMES);  // Tim
      endRecordPos.setType(Pos::FRAMES);
      
      _audioMonitor = 0;
      _audioMaster  = 0;

      //---------------------------------------------------
      //  establish pipes/sockets
      //---------------------------------------------------

      int filedes[2];         // 0 - reading   1 - writing
      if (pipe(filedes) == -1) {
            perror("creating pipe0");
            exit(-1);
            }
      fromThreadFdw = filedes[1];
      fromThreadFdr = filedes[0];
      int rv = fcntl(fromThreadFdw, F_SETFL, O_NONBLOCK);
      if (rv == -1)
            perror("set pipe O_NONBLOCK");

      if (pipe(filedes) == -1) {
            perror("creating pipe1");
            exit(-1);
            }
      sigFd = filedes[1];
      QSocketNotifier* ss = new QSocketNotifier(filedes[0], QSocketNotifier::Read);
      MusEGlobal::song->connect(ss, SIGNAL(activated(int)), MusEGlobal::song, SLOT(seqSignal(int)));
      }

//---------------------------------------------------------
//   start
//    start audio processing
//---------------------------------------------------------

extern bool initJackAudio();

bool Audio::start()
      {
      //process(MusEGlobal::segmentSize);   // warm up caches
      state = STOP;
      _loopCount = 0;
      MusEGlobal::muse->setHeartBeat();
      if (MusEGlobal::audioDevice) {
          //_running = true;
          //MusEGlobal::audioDevice->start();
          }
      else {
          if(false == initJackAudio()) {
                //_running = true;
                InputList* itl = MusEGlobal::song->inputs();
                for (iAudioInput i = itl->begin(); i != itl->end(); ++i) {
                      //printf("reconnecting input %s\n", (*i)->name().ascii());
                      for (int x=0; x < (*i)->channels();x++)
                          (*i)->setJackPort(x,0);
                      (*i)->setName((*i)->name()); // restore jack connection
                      }

                OutputList* otl = MusEGlobal::song->outputs();
                for (iAudioOutput i = otl->begin(); i != otl->end(); ++i) {
                      //printf("reconnecting output %s\n", (*i)->name().ascii());
                      for (int x=0; x < (*i)->channels();x++)
                          (*i)->setJackPort(x,0);
                      //printf("name=%s\n",(*i)->name().toLatin1());
                      (*i)->setName((*i)->name()); // restore jack connection
                      }
               //MusEGlobal::audioDevice->start();
               }
          else {
               printf("Failed to init audio!\n");
               return false;
               }
          }

      MusEGlobal::audioDevice->start(MusEGlobal::realTimePriority);
      
      _running = true;

      // shall we really stop JACK transport and locate to
      // saved position?

      MusEGlobal::audioDevice->stopTransport();
      //MusEGlobal::audioDevice->seekTransport(MusEGlobal::song->cPos().frame());
      MusEGlobal::audioDevice->seekTransport(MusEGlobal::song->cPos());
      return true;
      }

//---------------------------------------------------------
//   stop
//    stop audio processing
//---------------------------------------------------------

void Audio::stop(bool)
      {
      if (MusEGlobal::audioDevice)
            MusEGlobal::audioDevice->stop();
      _running = false;
      }

//---------------------------------------------------------
//   sync
//    return true if sync is completed
//---------------------------------------------------------

bool Audio::sync(int jackState, unsigned frame)
      {
      bool done = true;
      if (state == LOOP1)
            state = LOOP2;
      else {
            State s = State(jackState);
            //
            //  STOP -> START_PLAY      start rolling
            //  STOP -> STOP            seek in stop state
            //  PLAY -> START_PLAY  seek in play state

            if (state != START_PLAY) {
                Pos p(frame, false);
                seek(p);
              if (!_freewheel)
                      done = MusEGlobal::audioPrefetch->seekDone();
                if (s == START_PLAY)
                        state = START_PLAY;
                }
            else {
                if (frame != _pos.frame()) {
                        // seek during seek
                            seek(Pos(frame, false));
                        }
                done = MusEGlobal::audioPrefetch->seekDone();
                  }
            }
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
      _running = false;
      printf("Audio::shutdown()\n");
      write(sigFd, "S", 1);
      }

//---------------------------------------------------------
//   process
//    process one audio buffer at position "_pos "
//    of size "frames"
//---------------------------------------------------------

void Audio::process(unsigned frames)
      {
//      extern int watchAudio;
//      ++watchAudio;           // make a simple watchdog happy. Disabled. 
      
      if (!MusEGlobal::checkAudioDevice()) return;
      if (msg) {
            processMsg(msg);
            int sn = msg->serialNo;
            msg    = 0;    // dont process again
            int rv = write(fromThreadFdw, &sn, sizeof(int));
            if (rv != sizeof(int)) {
                  fprintf(stderr, "audio: write(%d) pipe failed: %s\n",
                     fromThreadFdw, strerror(errno));
                  }
            }

      OutputList* ol = MusEGlobal::song->outputs();
      if (idle) {
            // deliver no audio
            for (iAudioOutput i = ol->begin(); i != ol->end(); ++i)
                  (*i)->silence(frames);
            return;
            }

      int jackState = MusEGlobal::audioDevice->getState();

      //if(MusEGlobal::debugMsg)
      //  printf("Audio::process Current state:%s jackState:%s\n", audioStates[state], audioStates[jackState]);
      
      if (state == START_PLAY && jackState == PLAY) {
            _loopCount = 0;
            startRolling();
            if (_bounce)
                  write(sigFd, "f", 1);
            }
      else if (state == LOOP2 && jackState == PLAY) {
            ++_loopCount;                  // Number of times we have looped so far
            Pos newPos(_loopFrame, false);
            seek(newPos);
            startRolling();
            }
      else if (isPlaying() && jackState == STOP) {
            // Make sure to stop bounce and freewheel mode, for example if user presses stop 
            //  in QJackCtl before right-hand marker is reached (which is handled below). p3.3.43 
            //printf("Audio::process isPlaying() && jackState == STOP\n");
            //if (_bounce) 
            //{
              //printf("  stopping bounce...\n");
            //  _bounce = false;
            //  write(sigFd, "F", 1);
            //}
            
            stopRolling();
            }
      else if (state == START_PLAY && jackState == STOP) {
            state = STOP;
            if (_bounce) {
                  MusEGlobal::audioDevice->startTransport();
                  }
            else
                  write(sigFd, "3", 1);   // abort rolling
            }
      else if (state == STOP && jackState == PLAY) {
            _loopCount = 0;
            startRolling();
            }
      else if (state == LOOP1 && jackState == PLAY)
            ;     // treat as play
      else if (state == LOOP2 && jackState == START_PLAY) {
            ;     // sync cycle
            }
      else if (state != jackState)
            printf("JACK: state transition %s -> %s ?\n",
               audioStates[state], audioStates[jackState]);

// printf("p %s %s %d\n", audioStates[jackState], audioStates[state], _pos.frame());

      //
      // clear aux send buffers
      //
      AuxList* al = MusEGlobal::song->auxs();
      for (unsigned i = 0; i < al->size(); ++i) {
            AudioAux* a = (AudioAux*)((*al)[i]);
            float** dst = a->sendBuffer();
            for (int ch = 0; ch < a->channels(); ++ch)
                  memset(dst[ch], 0, sizeof(float) * MusEGlobal::segmentSize);
            }

      for (iAudioOutput i = ol->begin(); i != ol->end(); ++i)
            (*i)->processInit(frames);
      int samplePos = _pos.frame();
      int offset    = 0;      // buffer offset in audio buffers

      if (isPlaying()) {
            if (!freewheel())
                  MusEGlobal::audioPrefetch->msgTick();

            if (_bounce && _pos >= MusEGlobal::song->rPos()) {
                  _bounce = false;
                  write(sigFd, "F", 1);
                  return;
                  }

            //
            //  check for end of song
            //
            if ((curTickPos >= MusEGlobal::song->len())
               && !(MusEGlobal::song->record()
                || _bounce
                || MusEGlobal::song->loop())) {
                  //if(MusEGlobal::debugMsg)
                  //  printf("Audio::process curTickPos >= MusEGlobal::song->len\n");
                  
                  MusEGlobal::audioDevice->stopTransport();
                  return;
                  }

            //
            //  check for loop end
            //
            if (state == PLAY && MusEGlobal::song->loop() && !_bounce && !MusEGlobal::extSyncFlag.value()) {
                  const Pos& loop = MusEGlobal::song->rPos();
                  unsigned n = loop.frame() - samplePos - (3 * frames);
                  if (n < frames) {
                        // loop end in current cycle
                        unsigned lpos = MusEGlobal::song->lPos().frame();
                        // adjust loop start so we get exact loop len
                        if (n > lpos)
                              n = 0;
                        state = LOOP1;
                        _loopFrame = lpos - n;

                        // clear sustain
                        for (int i = 0; i < MIDI_PORTS; ++i) {
                            MidiPort* mp = &MusEGlobal::midiPorts[i];
                            for (int ch = 0; ch < MIDI_CHANNELS; ++ch) {
                                if (mp->hwCtrlState(ch, CTRL_SUSTAIN) == 127) {
                                    if (mp->device()!=NULL) {
                                        //printf("send clear sustain!!!!!!!! port %d ch %d\n", i,ch);
                                        MidiPlayEvent ev(0, i, ch, ME_CONTROLLER, CTRL_SUSTAIN, 0);
                                        // may cause problems, called from audio thread
                                        mp->device()->putEvent(ev);
                                        }
                                    }
                                }
                            }

                        //MusEGlobal::audioDevice->seekTransport(_loopFrame);
                        Pos lp(_loopFrame, false);
                        MusEGlobal::audioDevice->seekTransport(lp);
// printf("  process: seek to %d, end %d\n", _loopFrame, loop.frame());
                        }
                  }
            
            if(MusEGlobal::extSyncFlag.value())        // p3.3.25
            {
              nextTickPos = curTickPos + MusEGlobal::midiExtSyncTicks;
              // Probably not good - interfere with midi thread.
              MusEGlobal::midiExtSyncTicks = 0;
            }
            else
            {
              
              Pos ppp(_pos);
              ppp += frames;
              nextTickPos = ppp.tick();
            }
          }
      //
      // resync with audio interface
      //
      syncFrame   = MusEGlobal::audioDevice->framePos();
      syncTime    = curTime();
      frameOffset = syncFrame - samplePos;

      //printf("Audio::process calling process1:\n");
      
      process1(samplePos, offset, frames);
      for (iAudioOutput i = ol->begin(); i != ol->end(); ++i)
            (*i)->processWrite();
      if (isPlaying()) {
            _pos += frames;
            curTickPos = nextTickPos;
            }
      }

//---------------------------------------------------------
//   process1
//---------------------------------------------------------

void Audio::process1(unsigned samplePos, unsigned offset, unsigned frames)
      {
      if (MusEGlobal::midiSeqRunning) {
            processMidi();
            }
            //MusEGlobal::midiSeq->msgProcess();
      
      //
      // process not connected tracks
      // to animate meter display
      //
      TrackList* tl = MusEGlobal::song->tracks();
      AudioTrack* track; 
      int channels;
      for(ciTrack it = tl->begin(); it != tl->end(); ++it) 
      {
        if((*it)->isMidiTrack())
          continue;
        track = (AudioTrack*)(*it);
        
        // For audio track types, synths etc. which need some kind of non-audio 
        //  (but possibly audio-affecting) processing always, even if their output path
        //  is ultimately unconnected.
        // Example: A fluidsynth instance whose output path ultimately led to nowhere 
        //  would not allow us to load a font. Since process() was driven by audio output,
        //  in this case there was nothing driving the process() function which responds to
        //  such gui commands. So I separated the events processing from process(), into this.
        // It should be used for things like midi events, gui events etc. - things which need to
        //  be done BEFORE all the AudioOutput::process() are called below. That does NOT include 
        //  audio processing, because THAT is done at the very end of this routine.
        // This will also reset the track's processed flag.         Tim.
        track->preProcessAlways();
      }
      
      // Pre-process the metronome.
      ((AudioTrack*)metronome)->preProcessAlways();
      
      // Process Aux tracks first.
      for(ciTrack it = tl->begin(); it != tl->end(); ++it) 
      {
        if((*it)->isMidiTrack())
          continue;
        track = (AudioTrack*)(*it);
        if(!track->processed() && track->type() == Track::AUDIO_AUX)
        {
          //printf("Audio::process1 Do aux: track:%s\n", track->name().toLatin1().constData());  
          channels = track->channels();
          // Just a dummy buffer.
          float* buffer[channels];
          float data[frames * channels];
          for (int i = 0; i < channels; ++i)
                buffer[i] = data + i * frames;
          //printf("Audio::process1 calling track->copyData for track:%s\n", track->name().toLatin1());
          track->copyData(samplePos, channels, -1, -1, frames, buffer);
        }
      }      
      
      OutputList* ol = MusEGlobal::song->outputs();
      for (ciAudioOutput i = ol->begin(); i != ol->end(); ++i) 
        (*i)->process(samplePos, offset, frames);
            
      // Were ANY tracks unprocessed as a result of processing all the AudioOutputs, above? 
      // Not just unconnected ones, as previously done, but ones whose output path ultimately leads nowhere.
      // Those tracks were missed, until this fix.
      // Do them now. This will animate meters, and 'quietly' process some audio which needs to be done -
      //  for example synths really need to be processed, 'quietly' or not, otherwise the next time processing 
      //  is 'turned on', if there was a backlog of events while it was off, then they all happen at once.  Tim.
      for(ciTrack it = tl->begin(); it != tl->end(); ++it) 
      {
        if((*it)->isMidiTrack())
          continue;
        track = (AudioTrack*)(*it);
        // Ignore unprocessed tracks which have an output route, because they will be processed by 
        //  whatever track(s) they are routed to.
        //if(!track->processed() && track->noOutRoute() && (track->type() != Track::AUDIO_OUTPUT))
        // No, do all.
        if(!track->processed() && (track->type() != Track::AUDIO_OUTPUT))
        {
          //printf("Audio::process1 track:%s\n", track->name().toLatin1().constData()); 
          channels = track->channels();
          // Just a dummy buffer.
          float* buffer[channels];
          float data[frames * channels];
          for (int i = 0; i < channels; ++i)
                buffer[i] = data + i * frames;
          //printf("Audio::process1 calling track->copyData for track:%s\n", track->name().toLatin1());
          track->copyData(samplePos, channels, -1, -1, frames, buffer);
        }
      }      
    }

//---------------------------------------------------------
//   processMsg
//---------------------------------------------------------

void Audio::processMsg(AudioMsg* msg)
      {
      switch(msg->id) {
            case AUDIO_RECORD:
                  msg->snode->setRecordFlag2(msg->ival);
                  break;
            case AUDIO_ROUTEADD:
                  addRoute(msg->sroute, msg->droute);
                  break;
            case AUDIO_ROUTEREMOVE:
                  removeRoute(msg->sroute, msg->droute);
                  break;
            case AUDIO_REMOVEROUTES:      
                  removeAllRoutes(msg->sroute, msg->droute);
                  break;
            //case AUDIO_VOL:
            //      msg->snode->setVolume(msg->dval);
            //      break;
            //case AUDIO_PAN:
            //      msg->snode->setPan(msg->dval);
            //      break;
            case SEQM_SET_AUX:
                  msg->snode->setAuxSend(msg->ival, msg->dval);
                  break;
            case AUDIO_SET_PREFADER:
                  msg->snode->setPrefader(msg->ival);
                  break;
            case AUDIO_SET_CHANNELS:
                  msg->snode->setChannels(msg->ival);
                  break;
            case AUDIO_ADDPLUGIN:
                  msg->snode->addPlugin(msg->plugin, msg->ival);
                  break;
            //case AUDIO_SET_PLUGIN_CTRL_VAL:
                  //msg->plugin->track()->setPluginCtrlVal(msg->ival, msg->dval);
            //      msg->snode->setPluginCtrlVal(msg->ival, msg->dval);
            //      break;
            case AUDIO_SWAP_CONTROLLER_IDX:
                  msg->snode->swapControllerIDX(msg->a, msg->b);
                  break;
            case AUDIO_CLEAR_CONTROLLER_EVENTS:
                  msg->snode->clearControllerEvents(msg->ival);
                  break;
            case AUDIO_SEEK_PREV_AC_EVENT:
                  msg->snode->seekPrevACEvent(msg->ival);
                  break;
            case AUDIO_SEEK_NEXT_AC_EVENT:
                  msg->snode->seekNextACEvent(msg->ival);
                  break;
            case AUDIO_ERASE_AC_EVENT:
                  msg->snode->eraseACEvent(msg->ival, msg->a);
                  break;
            case AUDIO_ERASE_RANGE_AC_EVENTS:
                  msg->snode->eraseRangeACEvents(msg->ival, msg->a, msg->b);
                  break;
            case AUDIO_ADD_AC_EVENT:
                  msg->snode->addACEvent(msg->ival, msg->a, msg->dval);
                  break;
            case AUDIO_CHANGE_AC_EVENT:
                  msg->snode->changeACEvent(msg->ival, msg->a, msg->b, msg->dval);
                  break;
            case AUDIO_SET_SOLO:
                  msg->track->setSolo((bool)msg->ival);
                  break;

            case AUDIO_SET_SEND_METRONOME:
                  msg->snode->setSendMetronome((bool)msg->ival);
                  break;
            
            case AUDIO_SET_SEG_SIZE:
                  MusEGlobal::segmentSize = msg->ival;
                  MusEGlobal::sampleRate  = msg->iival;
#if 0 //TODO
                  audioOutput.MusEGlobal::segmentSizeChanged();
                  for (int i = 0; i < mixerGroups; ++i)
                        audioGroups[i].MusEGlobal::segmentSizeChanged();
                  for (iSynthI ii = synthiInstances.begin(); ii != synthiInstances.end();++ii)
                        (*ii)->MusEGlobal::segmentSizeChanged();
#endif
                  break;

            case SEQM_RESET_DEVICES:
                  //printf("Audio::processMsg SEQM_RESET_DEVICES\n");  
                  for (int i = 0; i < MIDI_PORTS; ++i)                         
                  {      
                    if(MusEGlobal::midiPorts[i].device())                       
                      MusEGlobal::midiPorts[i].instrument()->reset(i, MusEGlobal::song->mtype());
                  }      
                  break;
            case SEQM_INIT_DEVICES:
                  initDevices();
                  break;
            case SEQM_MIDI_LOCAL_OFF:
                  sendLocalOff();
                  break;
            case SEQM_PANIC:
                  panic();
                  break;
            case SEQM_PLAY_MIDI_EVENT:
                  {
                  MidiPlayEvent* ev = (MidiPlayEvent*)(msg->p1);
                  MusEGlobal::midiPorts[ev->port()].sendEvent(*ev);
                  // Record??
                  }
                  break;
            case SEQM_SET_HW_CTRL_STATE:
                  {
                  MidiPort* port = (MidiPort*)(msg->p1);
                  port->setHwCtrlState(msg->a, msg->b, msg->c);
                  }
                  break;
            case SEQM_SET_HW_CTRL_STATES:
                  {
                  MidiPort* port = (MidiPort*)(msg->p1);
                  port->setHwCtrlStates(msg->a, msg->b, msg->c, msg->ival);
                  }
                  break;
            case SEQM_SCAN_ALSA_MIDI_PORTS:
                  alsaScanMidiPorts();
                  break;
            //case MIDI_SHOW_INSTR_GUI:
            //      MusEGlobal::midiSeq->msgUpdatePollFd();
            //      break;
            //case MIDI_SHOW_INSTR_NATIVE_GUI:   
            //      MusEGlobal::midiSeq->msgUpdatePollFd();
            //      break;
            case SEQM_ADD_TEMPO:
            case SEQM_REMOVE_TEMPO:
            case SEQM_SET_GLOBAL_TEMPO:
            case SEQM_SET_TEMPO:
                  MusEGlobal::song->processMsg(msg);
                  if (isPlaying()) {
                        if (!MusEGlobal::checkAudioDevice()) return;
                        _pos.setTick(curTickPos);
                        int samplePos = _pos.frame();
                        syncFrame     = MusEGlobal::audioDevice->framePos();
                        syncTime      = curTime();
                        frameOffset   = syncFrame - samplePos;
                        }
                  break;
            //case SEQM_ADD_TRACK:
            //case SEQM_REMOVE_TRACK:
            //case SEQM_CHANGE_TRACK:
            //case SEQM_ADD_PART:
            //case SEQM_REMOVE_PART:
            //case SEQM_CHANGE_PART:
            case SEQM_SET_TRACK_OUT_CHAN:
            case SEQM_SET_TRACK_OUT_PORT:
            case SEQM_REMAP_PORT_DRUM_CTL_EVS:
            case SEQM_CHANGE_ALL_PORT_DRUM_CTL_EVS:
                  MusEGlobal::midiSeq->sendMsg(msg);
                  break;

            case SEQM_IDLE:
                  idle = msg->a;
                  MusEGlobal::midiSeq->sendMsg(msg);
                  break;

            default:
                  MusEGlobal::song->processMsg(msg);
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
      if (_pos == p) {
            if(MusEGlobal::debugMsg)
              printf("Audio::seek already there\n");
            return;        
            }
      //printf("Audio::seek frame:%d\n", p.frame());
      _pos        = p;
      if (!MusEGlobal::checkAudioDevice()) return;
      syncFrame   = MusEGlobal::audioDevice->framePos();
      frameOffset = syncFrame - _pos.frame();
      curTickPos  = _pos.tick();

      if (curTickPos == 0 && !MusEGlobal::song->record())     
            MusEGlobal::audio->initDevices();

      for(iMidiDevice i = MusEGlobal::midiDevices.begin(); i != MusEGlobal::midiDevices.end(); ++i) 
          (*i)->handleSeek();  
      
      //loopPassed = true;   // for record loop mode
      if (state != LOOP2 && !freewheel())
      {
            //MusEGlobal::audioPrefetch->msgSeek(_pos.frame());
            // We need to force prefetch to update, to ensure the most recent data. 
            // Things can happen to a part before play is pressed - such as part muting, 
            //  part moving etc. Without a force, the wrong data was being played.  Tim 08/17/08
            MusEGlobal::audioPrefetch->msgSeek(_pos.frame(), true);
      }
            
      write(sigFd, "G", 1);   // signal seek to gui
      }

//---------------------------------------------------------
//   writeTick
//    called from audio prefetch thread context
//    write another buffer to soundfile
//---------------------------------------------------------

void Audio::writeTick()
      {
      AudioOutput* ao = MusEGlobal::song->bounceOutput;
      if(ao && MusEGlobal::song->outputs()->find(ao) != MusEGlobal::song->outputs()->end())
      {
        if(ao->recordFlag())
          ao->record();
      }
      WaveTrackList* tl = MusEGlobal::song->waves();
      for (iWaveTrack t = tl->begin(); t != tl->end(); ++t) {
            WaveTrack* track = *t;
            if (track->recordFlag())
                  track->record();
            }
      }

//---------------------------------------------------------
//   startRolling
//---------------------------------------------------------

void Audio::startRolling()
      {
      if (MusEGlobal::debugMsg)
        printf("startRolling - loopCount=%d, _pos=%d\n", _loopCount, _pos.tick());

      if(_loopCount == 0) {
        startRecordPos = _pos;
      }
      if (MusEGlobal::song->record()) {
            recording      = true;
            TrackList* tracks = MusEGlobal::song->tracks();
            for (iTrack i = tracks->begin(); i != tracks->end(); ++i) {
                  if ((*i)->isMidiTrack())
                        continue;
                  if ((*i)->type() == Track::WAVE)
                        ((WaveTrack*)(*i))->resetMeter();
                  }
            }
      state = PLAY;
      write(sigFd, "1", 1);   // Play

      // Don't send if external sync is on. The master, and our sync routing system will take care of that.
      if(!MusEGlobal::extSyncFlag.value())
      {
        for(int port = 0; port < MIDI_PORTS; ++port) 
        {
          MidiPort* mp = &MusEGlobal::midiPorts[port];
          MidiDevice* dev = mp->device();
          if(!dev)
            continue;
              
          // Shall we check open flags?
          //if(!(dev->rwFlags() & 0x1) || !(dev->openFlags() & 1))
          //if(!(dev->openFlags() & 1))
          //  continue;
          
          MidiSyncInfo& si = mp->syncInfo();
            
          if(si.MMCOut())
            mp->sendMMCDeferredPlay();
          
          if(si.MRTOut())
          {
            if(curTickPos)
              mp->sendContinue();
            else
              mp->sendStart();
          }  
        }
      }  
      
      if (MusEGlobal::precountEnableFlag
         && MusEGlobal::song->click()
         && !MusEGlobal::extSyncFlag.value()
         && MusEGlobal::song->record()) {
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
            AL::sigmap.tickValues(curTickPos, &bar, &beat, &tick);
            if (tick)
                  beat += 1;
            midiClick = AL::sigmap.bar2tick(bar, beat, 0);
            }

      // reenable sustain 
      for (int i = 0; i < MIDI_PORTS; ++i) {
          MidiPort* mp = &MusEGlobal::midiPorts[i];
          for (int ch = 0; ch < MIDI_CHANNELS; ++ch) {
              if (mp->hwCtrlState(ch, CTRL_SUSTAIN) == 127) {
                  if(mp->device() != NULL) {
                        //printf("send enable sustain!!!!!!!! port %d ch %d\n", i,ch);
                        MidiPlayEvent ev(0, i, ch, ME_CONTROLLER, CTRL_SUSTAIN, 127);
                        mp->device()->addScheduledEvent(ev);    // TODO: Not working? Try putEvent
                        }
                  }
              }
          }
     
     //tempomap.clearExtTempoList();     
     }

//---------------------------------------------------------
//   stopRolling
//---------------------------------------------------------

void Audio::stopRolling()
{
      //if(MusEGlobal::debugMsg)
      //  printf("Audio::stopRolling state %s\n", audioStates[state]);
      
      state = STOP;
      
      MusEGlobal::midiSeq->setExternalPlayState(false); // not playing   Moved here from MidiSeq::processStop()   p4.0.34
      
      for(iMidiDevice id = MusEGlobal::midiDevices.begin(); id != MusEGlobal::midiDevices.end(); ++id) 
      {
        MidiDevice* md = *id;
        md->handleStop();
      }

      WaveTrackList* tracks = MusEGlobal::song->waves();
      for (iWaveTrack i = tracks->begin(); i != tracks->end(); ++i) {
            WaveTrack* track = *i;
            track->resetMeter();
            }
      recording    = false;
      endRecordPos = _pos;
      write(sigFd, "0", 1);   // STOP
      }

//---------------------------------------------------------
//   recordStop
//    execution environment: gui thread
//---------------------------------------------------------

void Audio::recordStop()
      {
      if (MusEGlobal::debugMsg)
        printf("recordStop - startRecordPos=%d\n", startRecordPos.tick());

      MusEGlobal::audio->msgIdle(true); // gain access to all data structures

      MusEGlobal::song->startUndo();
      WaveTrackList* wl = MusEGlobal::song->waves();

      for (iWaveTrack it = wl->begin(); it != wl->end(); ++it) {
            WaveTrack* track = *it;
            if (track->recordFlag() || MusEGlobal::song->bounceTrack == track) {
                  MusEGlobal::song->cmdAddRecordedWave(track, startRecordPos, endRecordPos);
                  // The track's _recFile pointer may have been kept and turned
                  //  into a SndFileR and added to a new part.
                  // Or _recFile may have been discarded (no new recorded part created).
                  // Regardless, we are done with the pointer itself. Set to zero so
                  //  MusEGlobal::song->setRecordFlag knows about it...

                  track->setRecFile(0);              // flush out the old file
                  MusEGlobal::song->setRecordFlag(track, false); //
                  //track->setRecordFlag1(true);       // and re-arm the track here
                  //MusEGlobal::song->setRecordFlag(track, true);  // here
                  }
            }
      MidiTrackList* ml = MusEGlobal::song->midis();
      for (iMidiTrack it = ml->begin(); it != ml->end(); ++it) {
            MidiTrack* mt     = *it;
            MPEventList* mpel = mt->mpevents();
            EventList* el     = mt->events();

            //---------------------------------------------------
            //    resolve NoteOff events, Controller etc.
            //---------------------------------------------------

            //buildMidiEventList(el, mpel, mt, MusEGlobal::config.division, true);
            // Do SysexMeta. Do loops.
            buildMidiEventList(el, mpel, mt, MusEGlobal::config.division, true, true);
            MusEGlobal::song->cmdAddRecordedEvents(mt, el, startRecordPos.tick());
            el->clear();
            mpel->clear();
            }
      
      //
      // bounce to file operates on the only
      // selected output port
      //
      
      AudioOutput* ao = MusEGlobal::song->bounceOutput;
      if(ao && MusEGlobal::song->outputs()->find(ao) != MusEGlobal::song->outputs()->end())
      {
        if(ao->recordFlag())
        {            
          MusEGlobal::song->bounceOutput = 0;
          SndFile* sf = ao->recFile();
          if (sf)
                delete sf;              // close
          ao->setRecFile(0);
          ao->setRecordFlag1(false);
          msgSetRecord(ao, false);
        }
      }  
      MusEGlobal::audio->msgIdle(false);
      MusEGlobal::song->endUndo(0);
      MusEGlobal::song->setRecord(false);
      }

//---------------------------------------------------------
//   curFrame
//    extrapolates current play frame on syncTime/syncFrame
//---------------------------------------------------------

unsigned int Audio::curFrame() const
      {
      return lrint((curTime() - syncTime) * MusEGlobal::sampleRate) + syncFrame;
      }

//---------------------------------------------------------
//   timestamp
//---------------------------------------------------------

int Audio::timestamp() const
      {
      int t = curFrame() - frameOffset;
      return t;
      }

//---------------------------------------------------------
//   sendMsgToGui
//---------------------------------------------------------

void Audio::sendMsgToGui(char c)
      {
      write(sigFd, &c, 1);
      }

} // namespace MusECore
