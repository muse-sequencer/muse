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

#include "globals.h"
#include "midi.h"
#include "midiseq.h"
#include "midictrl.h"
#include "audio.h"
#include "driver/mididev.h"
#include "driver/audiodev.h"
#include "driver/jackaudio.h"

#ifdef __APPLE__
#include "driver/coretimer.h"
#else
#include "driver/rtctimer.h"
#include "driver/posixtimer.h"
#include "driver/alsatimer.h"
#endif

#include "sync.h"
#include "song.h"
#include "gconfig.h"
#include "al/tempo.h"
#include "al/al.h"
#include "instruments/minstrument.h"
#include "midichannel.h"
#include "midiinport.h"
#include "midioutport.h"

MidiSeq* midiSeq;
static const unsigned char mmcStopMsg[] =  { 0x7f, 0x7f, 0x06, 0x01 };
static const unsigned char mmcDeferredPlayMsg[] = { 0x7f, 0x7f, 0x06, 0x03 };

volatile bool midiBusy;

//---------------------------------------------------------
//   readMsg
//---------------------------------------------------------

static void readMsg(void* p, void*)
      {
      MidiSeq* at = (MidiSeq*)p;
      at->readMsg();
      }

//---------------------------------------------------------
//   processMsg
//---------------------------------------------------------

void MidiSeq::processMsg(const ThreadMsg* m)
      {
      AudioMsg* msg = (AudioMsg*)m;
      switch (msg->id) {
            case MS_START:
                  processStart();
                  break;
            case MS_STOP:
                  processStop();
                  break;
            case MS_SET_RTC:
                  initRealtimeTimer();
                  break;
            case SEQM_RESET_DEVICES:
                  resetDevices();
                  break;
            case SEQM_ADD_TRACK:
                  song->insertTrack2(msg->track);
                  updatePollFd();
                  break;
            case SEQM_REMOVE_TRACK:
                  song->removeTrack2(msg->track);
                  updatePollFd();
                  break;
            case SEQM_ADD_PART:
                  song->cmdAddPart((Part*)msg->p1);
                  break;
            case SEQM_REMOVE_PART:
                  song->cmdRemovePart((Part*)msg->p1);
                  break;
            case SEQM_CHANGE_PART:
                  song->cmdChangePart((Part*)msg->p1, (Part*)msg->p2);
                  break;
            case SEQM_IDLE:
                  idle = msg->a;
                  break;
            case SEQM_MOVE_TRACK:
                  song->moveTrack((Track*)(msg->p1), (Track*)(msg->p2));
                  break;
            case AUDIO_ADDMIDIPLUGIN:
                  ((MidiTrackBase*)msg->track)->addPlugin(msg->mplugin, msg->ival);
                  break;
            default:
                  song->processMsg(msg);
                  break;
            }
      }

//---------------------------------------------------------
//   processStart
//---------------------------------------------------------

void MidiSeq::processStart()
      {
      if (genMMC || genMCSync) {
            MidiOutPortList* ol = song->midiOutPorts();
            for (iMidiOutPort i = ol->begin(); i != ol->end(); ++i) {
                  MidiOutPort* mp = *i;
                  if (!mp->sendSync())
                        continue;
                  if (genMMC)
                        mp->sendSysex(mmcDeferredPlayMsg, sizeof(mmcDeferredPlayMsg));
                  if (genMCSync) {
                        if (audio->curTickPos())
                              mp->sendContinue();
                        else
                              mp->sendStart();
                        }
                  }
            }
      }

//---------------------------------------------------------
//   processStop
//    synced with audio process()
//---------------------------------------------------------

void MidiSeq::processStop()
      {
      //---------------------------------------------------
      //    stop stuck notes
      //---------------------------------------------------

      MidiOutPortList* ol = song->midiOutPorts();
      for (iMidiOutPort id = ol->begin(); id != ol->end(); ++id) {
            MidiOutPort* md = *id;
            if (md->noOutRoute())
                  continue;
            MPEventList* pel = md->playEvents();
            for (iMPEvent i = pel->begin(); i != pel->end(); ++i) {
                  MidiEvent ev = *i;
                  if (ev.isNoteOff()) {
                        ev.setTime(0);
                        md->putEvent(ev);
                        }
                  }
            pel->clear();
            }

      //---------------------------------------------------
      //    reset sustain
      //---------------------------------------------------

      MidiChannelList* mcl = song->midiChannel();
      for (iMidiChannel i = mcl->begin(); i != mcl->end(); ++i) {
            MidiChannel* mc = *i;
            if (mc->noInRoute())
                  continue;
            MidiOutPort* mp = mc->port();
            if (mc->hwCtrlState(CTRL_SUSTAIN) != CTRL_VAL_UNKNOWN) {
                  MidiEvent ev(0, 0, ME_CONTROLLER, CTRL_SUSTAIN, 0);
                  ev.setChannel(mc->channelNo());
                  mp->putEvent(ev);
                  }
            }

      if (genMMC || genMCSync) {
            MidiOutPortList* ol = song->midiOutPorts();
            for (iMidiOutPort i = ol->begin(); i != ol->end(); ++i) {
                  MidiOutPort* mp = *i;
                  if (!mp->sendSync())
                        continue;
                  if (genMMC) {
                        unsigned char mmcPos[] = {
                              0x7f, 0x7f, 0x06, 0x44, 0x06, 0x01,
                              0, 0, 0, 0, 0
                              };
                        int frame = AL::tempomap.tick2frame(audio->curTickPos());
                        MTC mtc(double(frame) / double(AL::sampleRate));
                        mmcPos[6] = mtc.h() | (AL::mtcType << 5);
                        mmcPos[7] = mtc.m();
                        mmcPos[8] = mtc.s();
                        mmcPos[9] = mtc.f();
                        mmcPos[10] = mtc.sf();
                        mp->sendSysex(mmcStopMsg, sizeof(mmcStopMsg));
                        mp->sendSysex(mmcPos, sizeof(mmcPos));
                        }
                  if (genMCSync) {         // Midi Clock
                        // send STOP and
                        // "set song position pointer"
                        mp->sendStop();
                        mp->sendSongpos(audio->curTickPos() * 4 / config.division);
                        }
                  }
            }
      }

//---------------------------------------------------------
//   resetDevices
//---------------------------------------------------------

void MidiSeq::resetDevices()
      {
      MidiOutPortList* ol = song->midiOutPorts();
      for (iMidiOutPort id = ol->begin(); id != ol->end(); ++id) {
            MidiOutPort* md = *id;
            MidiInstrument* instr = md->instrument();
            instr->reset(md);
            }
      }

//---------------------------------------------------------
//   processSeek
//    synced with audio process()
//---------------------------------------------------------

void MidiSeq::processSeek()
      {
      int pos = audio->curTickPos();
      if (genMCSync) {
            MidiOutPortList* ol = song->midiOutPorts();
            for (iMidiOutPort i = ol->begin(); i != ol->end(); ++i) {
                  MidiOutPort* mp = *i;
                  if (!mp->sendSync())
                        continue;
                  int beat = (pos * 4) / config.division;
                  mp->sendStop();
                  mp->sendSongpos(beat);
                  mp->sendContinue();
                  }
            }

//      if (pos == 0 && !song->record())
//            audio->initDevices();

      //---------------------------------------------------
      //    set all controller
      //---------------------------------------------------

      MidiOutPortList* opl = song->midiOutPorts();

      for (iMidiOutPort i = opl->begin(); i != opl->end(); ++i) {
            MidiOutPort* op = *i;
            if (op->mute())
                  continue;
            MPEventList* el = op->playEvents();
            if (audio->isPlaying()) {
                  // stop all notes
                  for (iMPEvent i = el->begin(); i != el->end(); ++i) {
                        MidiEvent ev = *i;
                        if (ev.isNoteOff()) {
                              ev.setTime(0);
                              op->putEvent(ev);
                              // el->add(ev);
                              }
                        }
                  el->clear();
                  }
//            else
//                  el->erase(el->begin(), op->nextPlayEvent());
            for (int ch = 0; ch < MIDI_CHANNELS; ++ch) {
                  MidiChannel* mc = op->channel(ch);
                  if (mc->mute() || mc->noInRoute() || !mc->autoRead())
                        continue;
                  CtrlList* cll = mc->controller();
                  for (iCtrl ivl = cll->begin(); ivl != cll->end(); ++ivl) {
                        Ctrl* c   = ivl->second;
                        int val = c->value(pos).i;
                        if (val != CTRL_VAL_UNKNOWN && val != c->curVal().i) {
                              op->putEvent(MidiEvent(0, ch, ME_CONTROLLER, c->id(), val));
                              }
                        }
                  }
//            op->setNextPlayEvent(op->playEvents()->begin());
            }
      }

//---------------------------------------------------------
//   MidiSeq
//---------------------------------------------------------

MidiSeq::MidiSeq(const char* name)
   : Thread(name)
      {
      idle      = false;
      midiClock = 0;
      mclock1 = 0.0;
      mclock2 = 0.0;
      songtick1 = songtick2 = 0;
      lastTempo = 0;
      storedtimediffs = 0;
      timer = 0;
      }

//---------------------------------------------------------
//   threadStart
//    called from loop()
//---------------------------------------------------------

void MidiSeq::threadStart(void*)
      {
      struct sched_param rt_param;
      memset(&rt_param, 0, sizeof(rt_param));

      int prio_min = sched_get_priority_min(SCHED_FIFO);
      int prio_max = sched_get_priority_max(SCHED_FIFO);
	int prio     = realTimePriority;
      if (prio < prio_min)
            prio = prio_min;
      else if (prio > prio_max)
            prio = prio_max;
      rt_param.sched_priority = prio;

      pthread_t tid = pthread_self();
      int rv = pthread_setschedparam(tid, SCHED_FIFO, &rt_param);
      if (rv != 0)
            perror("set realtime scheduler");

      int policy;
#ifdef __APPLE__
      if (0) {
#else
      if ( (policy = sched_getscheduler (0)) < 0) {
#endif
            printf("cannot get current client scheduler for midi thread: %s!\n", strerror(errno));
            }
      else {
            if (policy != SCHED_FIFO)
                  printf("midi thread %d _NOT_ running SCHED_FIFO\n", getpid());
            else if (debugMsg) {
            	struct sched_param rt_param;
            	int rv;
            	memset(&rt_param, 0, sizeof(sched_param));
            	int type;
            	rv = pthread_getschedparam(pthread_self(), &type, &rt_param);
            	if (rv == -1)
                  	perror("get scheduler parameter");
                  printf("midiseq thread running SCHED_FIFO priority %d\n",
                     rt_param.sched_priority);
                  }
            }

#ifdef __APPLE__
            timer = new CoreTimer;
#else
//      timer = new PosixTimer;
//      if (!timer->initTimer()) {
//            delete timer;
            timer = new RtcTimer;
            if (!timer->initTimer()) {
                  delete timer;
                  timer = new AlsaTimer;
                  if (!timer->initTimer()) {
                        fprintf(stderr, "no midi timer available\n");
                        abort();
                        }
                  }
//            }
#endif
      initRealtimeTimer();
      updatePollFd();
      }

//---------------------------------------------------------
//   midiRead
//---------------------------------------------------------


static void midiRead(void*, void*)
      {
      midiDriver->read(midiSeq);
      }

//---------------------------------------------------------
//   updatePollFd
//---------------------------------------------------------

void MidiSeq::updatePollFd()
      {
      if (!isRunning())
            return;
      clearPollFd();

      if (timer) {
            int timerFd = timer->getFd();
            if (timerFd != -1)
                  addPollFd(timerFd, POLLIN, midiTick, this, 0);
            }
      addPollFd(toThreadFdr, POLLIN, ::readMsg, this, 0);

      struct pollfd* pfd;
      int n;
      midiDriver->getInputPollFd(&pfd, &n);
      for (int i = 0; i < n; ++i)
            addPollFd(pfd[i].fd, POLLIN, ::midiRead, this, 0);
//      midiDriver->getOutputPollFd(&pfd, &n);
//      for (int i = 0; i < n; ++i)
//            addPollFd(pfdi[i].fd, POLLOUT, ::midiWrite, this, 0);
      }

//---------------------------------------------------------
//   threadStop
//    called from loop()
//---------------------------------------------------------

void MidiSeq::threadStop()
      {
      if (timer)
	      timer->stopTimer();
      }

//---------------------------------------------------------
//   setRtcTicks
//    return true on success
//---------------------------------------------------------

bool MidiSeq::initRealtimeTimer()
      {
      if (!timer)
            return false;
      if (!timer->setTimerFreq(config.rtcTicks))
            return false;
      if (!timer->startTimer())
            return false;
      realRtcTicks = config.rtcTicks;
      return true;
      }

//---------------------------------------------------------
//   start
//    return true on error
//---------------------------------------------------------

bool MidiSeq::start(int prio)
      {
      Thread::start(prio);
      return false;
      }

//---------------------------------------------------------
//   processMidiClock
//---------------------------------------------------------

void MidiSeq::processMidiClock()
      {
/*      if (genMCSync)
            midiPorts[txSyncPort].sendClock();
      if (state == START_PLAY) {
            // start play on sync
            state      = PLAY;
            _midiTick  = playTickPos;
            midiClock  = playTickPos;

            int bar, beat, tick;
            sigmap.tickValues(_midiTick, &bar, &beat, &tick);
            midiClick      = sigmap.bar2tick(bar, beat+1, 0);

            double cpos    = tempomap.tick2time(playTickPos);
            samplePosStart = samplePos - lrint(cpos * sampleRate);
            rtcTickStart   = rtcTick - lrint(cpos * realRtcTicks);

            endSlice       = playTickPos;
            recTick        = playTickPos;
            lastTickPos    = playTickPos;

            tempoSN = tempomap.tempoSN();

            startRecordPos.setPosTick(playTickPos);
            }
*/
      midiClock += config.division/24;
      }

//---------------------------------------------------------
//   midiTick
//---------------------------------------------------------

void MidiSeq::midiTick(void* p, void*)
      {
      MidiSeq* at = (MidiSeq*)p;
      at->processTimerTick();
      }

//---------------------------------------------------------
//   processTimerTick
//---------------------------------------------------------

void MidiSeq::processTimerTick()
      {
      extern int watchMidi;
      ++watchMidi;            // make a simple watchdog happy

      timer->getTimerTicks();  // read elapsed rtc timer ticks

      if (idle)
            return;

      if (midiBusy) {
            // we hit audio: midiSeq->msgProcess
            // miss this timer tick
            return;
            }
      unsigned curFrame = audioDriver->framePos();

/*      if (!extSyncFlag.value()) {
            int curTick = AL::tempomap.frame2tick(curFrame);
            if (curTick >= midiClock)
                  processMidiClock();
            }
  */

      //
      // play all events upto curFrame
      //
      MidiOutPortList* ol = song->midiOutPorts();
      for (iMidiOutPort id = ol->begin(); id != ol->end(); ++id) {
            MidiOutPort* mp = *id;
            MPEventList* el = mp->playEvents();

            iMPEvent i = el->begin();
            for (; i != el->end(); ++i) {
                  if (i->time() > curFrame)
                        break;
                  mp->putEvent(*i);
                  }
            el->erase(el->begin(), i);
            }
      }

//---------------------------------------------------------
//   msgMsg
//---------------------------------------------------------

void MidiSeq::msgMsg(int id)
      {
      AudioMsg msg;
      msg.id = id;
      Thread::sendMsg(&msg);
      }

void MidiSeq::msgStop()         { msgMsg(MS_STOP); }
void MidiSeq::msgStart()        { msgMsg(MS_START); }
void MidiSeq::msgSetRtc()       { msgMsg(MS_SET_RTC); }

