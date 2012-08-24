//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: midiseq.cpp,v 1.30.2.21 2009/12/20 05:00:35 terminator356 Exp $
//
//    high priority task for scheduling midi events
//
//  (C) Copyright 2003 Werner Schweer (ws@seh.de)
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

#include <QMessageBox>
#include <QApplication>

#include <stdio.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <poll.h>
#include <math.h>

#include "app.h"
#include "globals.h"
#include "midi.h"
#include "midiseq.h"
#include "midiport.h"
#include "mididev.h"
#include "midictrl.h"
#include "audio.h"
#include "driver/alsamidi.h"
#include "driver/jackmidi.h"
#include "sync.h"
#include "synth.h"
#include "song.h"
#include "gconfig.h"

namespace MusEGlobal {
MusECore::MidiSeq* midiSeq;
volatile bool midiBusy=false;
}

namespace MusECore {

int MidiSeq::ticker = 0;

void initMidiSequencer()   
{
      MusEGlobal::midiSeq       = new MidiSeq("Midi");
}

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
      MusECore::AudioMsg* msg = (MusECore::AudioMsg*)m;
      switch(msg->id) {
            // This does not appear to be used anymore. Was sent in Audio::process1,  DELETETHIS 5 ??
            //  now Audio::processMidi is called directly. p4.0.15 Tim.
            //case MusECore::MS_PROCESS:
            //      audio->processMidi();
            //      break;
            
            case MusECore::SEQM_SEEK:
                  processSeek();
                  break;
            case MusECore::MS_STOP:
                  processStop();
                  break;
            
            case MusECore::MS_SET_RTC:
                  MusEGlobal::doSetuid();
                  setRtcTicks();
                  MusEGlobal::undoSetuid();
                  break;
            case MusECore::MS_UPDATE_POLL_FD:
                  updatePollFd();
                  break;
                  
                  
            // Moved into Song::processMsg p4.0.34  ...
            case MusECore::SEQM_ADD_TRACK:
                  MusEGlobal::song->insertTrack2(msg->track, msg->ival);
                  updatePollFd();
                  break;
            case MusECore::SEQM_REMOVE_TRACK:
                  MusEGlobal::song->cmdRemoveTrack(msg->track);
                  updatePollFd();
                  break;
            //case MusECore::SEQM_CHANGE_TRACK: DELETETHIS 4
            //      MusEGlobal::song->changeTrack((Track*)(msg->p1), (Track*)(msg->p2));
            //      updatePollFd();
            //      break;
            case MusECore::SEQM_ADD_PART:
                  MusEGlobal::song->cmdAddPart((Part*)msg->p1);
                  break;
            case MusECore::SEQM_REMOVE_PART:
                  MusEGlobal::song->cmdRemovePart((Part*)msg->p1);
                  break;
            case MusECore::SEQM_CHANGE_PART:
                  MusEGlobal::song->cmdChangePart((Part*)msg->p1, (Part*)msg->p2, msg->a, msg->b);
                  break;
                  
                  
            case MusECore::SEQM_SET_TRACK_OUT_CHAN:
                  {
                  MidiTrack* track = (MidiTrack*)(msg->p1);
                  track->setOutChanAndUpdate(msg->a);
                  }
                  break;
            case MusECore::SEQM_SET_TRACK_OUT_PORT:
                  {
                  MidiTrack* track = (MidiTrack*)(msg->p1);
                  track->setOutPortAndUpdate(msg->a);
                  }
                  break;
            case MusECore::SEQM_REMAP_PORT_DRUM_CTL_EVS:
                    MusEGlobal::song->remapPortDrumCtrlEvents(msg->ival, msg->a, msg->b, msg->c);
                  break;
            case MusECore::SEQM_CHANGE_ALL_PORT_DRUM_CTL_EVS:      
                    MusEGlobal::song->changeAllPortDrumCtrlEvents((bool)msg->a, (bool)msg->b);
                  break;
            case MusECore::SEQM_SET_MIDI_DEVICE:
                  ((MidiPort*)(msg->p1))->setMidiDevice((MidiDevice*)(msg->p2));
                  updatePollFd();
                  break;
            case MusECore::SEQM_IDLE:
                  idle = msg->a;
                  break;
            default:
                  printf("MidiSeq::processMsg() unknown id %d\n", msg->id);
                  break;
            }
      }

#if 1  // DELETETHIS the #if and #endif?
//---------------------------------------------------------
//   processStop
//---------------------------------------------------------

void MidiSeq::processStop()
{
  // TODO Try to move this into Audio::stopRolling(). 
  playStateExt = false; // not playing
  
  //
  //    clear Alsa midi device notes and stop stuck notes
  //
  for(iMidiDevice id = MusEGlobal::midiDevices.begin(); id != MusEGlobal::midiDevices.end(); ++id) 
  {
    //MidiDevice* md = *id; DELETETHIS 3
    // Only ALSA devices are handled by this thread.
    //if((*id)->deviceType() == MidiDevice::ALSA_MIDI)      
      (*id)->handleStop();  
    /* DELETETHIS 14
    if (md->midiPort() == -1)
          continue;
    MPEventList* pel = md->playEvents();
    MPEventList* sel = md->stuckNotes();
    pel->clear();
    for(iMPEvent i = sel->begin(); i != sel->end(); ++i) 
    {
      MidiPlayEvent ev = *i;
      ev.setTime(0);
      pel->add(ev);
    }
    sel->clear();
    //md->setNextPlayEvent(pel->begin());  // Removed p4.0.15
    */
  }
}
#endif

#if 1   //DELETETHIS #if and #endif
//---------------------------------------------------------
//   processSeek
//---------------------------------------------------------

void MidiSeq::processSeek()
{
  int pos = MusEGlobal::audio->tickPos();
  // TODO Try to move this into MusEGlobal::audio::seek().   
  if (pos == 0 && !MusEGlobal::song->record())
        MusEGlobal::audio->initDevices();

  //---------------------------------------------------
  //    set all controller
  //---------------------------------------------------

  for (iMidiDevice i = MusEGlobal::midiDevices.begin(); i != MusEGlobal::midiDevices.end(); ++i) 
  {
    //MidiDevice* md = *i; DELETETHIS 3
    // Only ALSA devices are handled by this thread.
    //if((*i)->deviceType() == MidiDevice::ALSA_MIDI)      
      (*i)->handleSeek();  
    /* DELETETHIS 47
    int port = md->midiPort();
    if (port == -1)
          continue;
    MidiPort* mp = &MusEGlobal::midiPorts[port];
    MidiCtrlValListList* cll = mp->controller();

    MPEventList* el = md->playEvents();

    if (MusEGlobal::audio->isPlaying()) 
    {
      // stop all notes
      el->clear();
      MPEventList* sel = dev->stuckNotes();
      for (iMPEvent i = sel->begin(); i != sel->end(); ++i) 
      {
            MidiPlayEvent ev = *i;
            ev.setTime(0);
            el->add(ev);
      }
      sel->clear();
    }
    //else
      // Removed p4.0.15 Device now leaves beginning pointing at next event,
      //  immediately after playing some notes.  
      // NOTE: This removal needs testing. I'm not sure about this.
      //el->erase(el->begin(), dev->nextPlayEvent());  
    
    for (iMidiCtrlValList ivl = cll->begin(); ivl != cll->end(); ++ivl) 
    {
      MidiCtrlValList* vl = ivl->second;
      //int val = vl->value(pos);
      //if (val != CTRL_VAL_UNKNOWN) {
      //      int channel = ivl->first >> 24;
      //      el->add(MidiPlayEvent(0, port, channel, ME_CONTROLLER, vl->num(), val));
      //      }
      iMidiCtrlVal imcv = vl->iValue(pos);
      if(imcv != vl->end()) 
      {
        Part* p = imcv->second.part;
        unsigned t = (unsigned)imcv->first;
        // Do not add values that are outside of the part.
        if(p && t >= p->tick() && t < (p->tick() + p->lenTick()) )
          el->add(MidiPlayEvent(0, port, ivl->first >> 24, ME_CONTROLLER, vl->num(), imcv->second.val));
      }
    }
    //dev->setNextPlayEvent(el->begin());    // Removed p4.0.15
    */
  }
}
#endif

//---------------------------------------------------------
//   MidiSeq
//---------------------------------------------------------

MidiSeq::MidiSeq(const char* name)
   : Thread(name)
      {
      prio = 0;
      
      idle = false;
      midiClock = 0;
      mclock1 = 0.0;
      mclock2 = 0.0;
      songtick1 = songtick2 = 0;
      lastTempo = 0;
      storedtimediffs = 0;
      playStateExt = false; // not playing

      _clockAveragerStages = new int[16]; // Max stages is 16!
      setSyncRecFilterPreset(MusEGlobal::syncRecFilterPreset);
      
      for(int i = 0; i < _clockAveragerPoles; ++i)
      {
        _avgClkDiffCounter[i] = 0;
        _averagerFull[i] = false;
      }
      _tempoQuantizeAmount = 1.0;
      _lastRealTempo      = 0.0;
      
      MusEGlobal::doSetuid();
      timerFd=selectTimer();
      MusEGlobal::undoSetuid();

      }

//---------------------------------------------------------
//   ~MidiSeq
//---------------------------------------------------------

MidiSeq::~MidiSeq()
    {
    delete timer;
    delete _clockAveragerStages;
    }

//---------------------------------------------------------
//   selectTimer()
//   select one of the supported timers to use during this run
//---------------------------------------------------------

signed int MidiSeq::selectTimer()
    {
    int tmrFd;
    
    printf("Trying RTC timer...\n");
    timer = new RtcTimer();
    tmrFd = timer->initTimer();
    if (tmrFd != -1) { // ok!
        printf("got timer = %d\n", tmrFd);
        return tmrFd;
    }
    delete timer;
    
    printf("Trying ALSA timer...\n");
    timer = new AlsaTimer();
    tmrFd = timer->initTimer();
    if ( tmrFd!= -1) { // ok!
        printf("got timer = %d\n", tmrFd);
        return tmrFd;
    }
    delete timer;
    timer=NULL;
    QMessageBox::critical( 0, /*tr*/(QString("Failed to start timer!")),
              /*tr*/(QString("No functional timer was available.\n"
                         "RTC timer not available, check if /dev/rtc is available and readable by current user\n"
                         "Alsa timer not available, check if module snd_timer is available and /dev/snd/timer is available")));
    printf("No functional timer available!!!\n");
    exit(1);
    }

//---------------------------------------------------------
//   threadStart
//    called from loop()
//---------------------------------------------------------

void MidiSeq::threadStart(void*)
      {
      // Removed by Tim. p3.3.17 DELETETHIS 13
      /*
      struct sched_param rt_param;
      memset(&rt_param, 0, sizeof(rt_param));
      int prio_min = sched_get_priority_min(SCHED_FIFO);
      int prio_max = sched_get_priority_max(SCHED_FIFO);

      if (prio < prio_min) prio = prio_min;
      else if (prio > prio_max) prio = prio_max;

      rt_param.sched_priority = prio;
      int rv = pthread_setschedparam(pthread_self(), SCHED_FIFO, &rt_param);
      if (rv != 0)
            perror("set realtime scheduler");
      */

      int policy;
      if ((policy = sched_getscheduler (0)) < 0) {
            printf("Cannot get current client scheduler: %s\n", strerror(errno));
            }
      if (policy != SCHED_FIFO)
            printf("midi thread %d _NOT_ running SCHED_FIFO\n", getpid());
      updatePollFd();
      }

//---------------------------------------------------------
//   alsaMidiRead
//---------------------------------------------------------

static void alsaMidiRead(void*, void*)
      {
      // calls itself midiDevice->recordEvent(MidiRecordEvent):
      alsaProcessMidiInput();
      }

//---------------------------------------------------------
//   midiRead
//---------------------------------------------------------

static void midiRead(void*, void* d)
      {
      MidiDevice* dev = (MidiDevice*) d;
      dev->processInput();
      }

// DELETETHIS 12
//---------------------------------------------------------
//   synthIRead
//---------------------------------------------------------

#if 0
static void synthIRead(void*, void* d)
      {
      SynthI* syn = (SynthI*) d;
      syn->processInput();
      }
#endif

//---------------------------------------------------------
//   midiWrite
//---------------------------------------------------------

static void midiWrite(void*, void* d)
      {
      MidiDevice* dev = (MidiDevice*) d;
      dev->flush();
      }

//---------------------------------------------------------
//   updatePollFd
//---------------------------------------------------------

void MidiSeq::updatePollFd()
      {
      if (!isRunning())
            return;

      clearPollFd();
      addPollFd(timerFd, POLLIN, midiTick, this, 0);

      if (timerFd == -1) {
            fprintf(stderr, "updatePollFd: no timer fd\n");
            if (!MusEGlobal::debugMode)
                  exit(-1);
            }

      addPollFd(toThreadFdr, POLLIN, MusECore::readMsg, this, 0);

      //---------------------------------------------------
      //  midi ports
      //---------------------------------------------------

      for (iMidiDevice imd = MusEGlobal::midiDevices.begin(); imd != MusEGlobal::midiDevices.end(); ++imd) {
            MidiDevice* dev = *imd;
            int port = dev->midiPort();
            if (port == -1)
                  continue;
            if ((dev->rwFlags() & 0x2) || (MusEGlobal::extSyncFlag.value()
               && (MusEGlobal::midiPorts[port].syncInfo().MCIn())))
                  addPollFd(dev->selectRfd(), POLLIN, MusECore::midiRead, this, dev);
            if (dev->bytesToWrite())
                  addPollFd(dev->selectWfd(), POLLOUT, MusECore::midiWrite, this, dev);
            }
      // special handling for alsa midi:
      // (one fd for all devices)
      //    this allows for processing of some alsa events
      //    even if no alsa driver is active (assigned to a port)
      addPollFd(alsaSelectRfd(), POLLIN, MusECore::alsaMidiRead, this, 0);
      }

//---------------------------------------------------------
//   threadStop
//    called from loop()
//---------------------------------------------------------

void MidiSeq::threadStop()
      {
      timer->stopTimer();
      }

//---------------------------------------------------------
//   setRtcTicks
//    returns actual tick frequency
//---------------------------------------------------------

int MidiSeq::setRtcTicks()
      {
      int gotTicks = timer->setTimerFreq(MusEGlobal::config.rtcTicks);

      timer->startTimer();
      return gotTicks;
      }

//---------------------------------------------------------
//   start
//    return true on error
//---------------------------------------------------------

void MidiSeq::start(int priority)
      {
      prio = priority;
      
      MusEGlobal::doSetuid();
      setRtcTicks();
      MusEGlobal::undoSetuid();
      Thread::start(priority);
      }

//---------------------------------------------------------
//   checkAndReportTimingResolution
//---------------------------------------------------------
void MidiSeq::checkAndReportTimingResolution()
{
    int freq = timer->getTimerFreq();
    if (freq < 500) {
        QMessageBox::warning( MusEGlobal::muse, 
        qApp->translate("@default", QT_TRANSLATE_NOOP("@default", "Bad timing")), 
        qApp->translate("@default", QT_TRANSLATE_NOOP("@default", 
                             "Timing source frequency is %1hz, which is below the recommended minimum: 500hz!\n" \
                             "This could lead to audible timing problems for MIDI.\n" \
                             "Please see the FAQ on http://muse-sequencer.org for remedies.\n" \
                             "Also please check console output for any further error messages.\n ")).arg(freq) );
    }
}

//---------------------------------------------------------
//   setSyncRecFilterPreset
//   To be called in realtime thread only.
//---------------------------------------------------------
void MidiSeq::setSyncRecFilterPreset(MidiSyncInfo::SyncRecFilterPresetType type)
{
  _syncRecFilterPreset = type;
  alignAllTicks();
  
  switch(_syncRecFilterPreset)
  {
    // NOTE: Max _clockAveragerPoles is 16 and maximum stages is 48 per pole !
    case MidiSyncInfo::NONE:
      _clockAveragerPoles = 0;    
      _preDetect = false;
    break;  
    case MidiSyncInfo::TINY:
      _clockAveragerPoles = 2;    
      _clockAveragerStages[0] = 4; 
      _clockAveragerStages[1] = 4; 
      _preDetect = false;
    break;  
    case MidiSyncInfo::SMALL:
      _clockAveragerPoles = 3;    
      _clockAveragerStages[0] = 12; 
      _clockAveragerStages[1] = 8; 
      _clockAveragerStages[2] = 4; 
      _preDetect = false;
    break;  
    case MidiSyncInfo::MEDIUM:
      _clockAveragerPoles = 3;    
      _clockAveragerStages[0] = 28; 
      _clockAveragerStages[1] = 12; 
      _clockAveragerStages[2] = 8; 
      _preDetect = false;
    break;  
    case MidiSyncInfo::LARGE:
      _clockAveragerPoles = 4;    
      _clockAveragerStages[0] = 48; 
      _clockAveragerStages[1] = 48; 
      _clockAveragerStages[2] = 48; 
      _clockAveragerStages[3] = 48; 
      _preDetect = false;
    break;  
    case MidiSyncInfo::LARGE_WITH_PRE_DETECT:
      _clockAveragerPoles = 4;    
      _clockAveragerStages[0] = 8; 
      _clockAveragerStages[1] = 48; 
      _clockAveragerStages[2] = 48; 
      _clockAveragerStages[3] = 48; 
      _preDetect = true;
    break;  
    
    default:
      printf("MidiSeq::setSyncRecFilterPreset unknown preset type:%d\n", (int)type);
  }
}


//---------------------------------------------------------
//   processMidiClock
//---------------------------------------------------------

void MidiSeq::processMidiClock()
      {
      // DELETETHIS 30, maybe remove entire function?
//      if (genMCSync) {
//            MusEGlobal::midiPorts[txSyncPort].sendClock();
//      }

/*      if (state == START_PLAY) {
            // start play on sync
            state      = PLAY;
            _midiTick  = playTickPos;
            midiClock  = playTickPos;

            int bar, beat, tick;
            sigmap.tickValues(_midiTick, &bar, &beat, &tick);
            midiClick      = sigmap.bar2tick(bar, beat+1, 0);

            double cpos    = MusEGlobal::tempomap.tick2time(playTickPos);
            samplePosStart = samplePos - lrint(cpos * MusEGlobal::sampleRate);
            rtcTickStart   = rtcTick - lrint(cpos * realRtcTicks);

            endSlice       = playTickPos;
            recTick        = playTickPos;
            lastTickPos    = playTickPos;

            tempoSN = MusEGlobal::tempomap.tempoSN();

            startRecordPos.setPosTick(playTickPos);
            }
*/
//      midiClock += MusEGlobal::config.division/24;
      }

//---------------------------------------------------------
//   midiTick
//---------------------------------------------------------

void MidiSeq::midiTick(void* p, void*)
      {
      MidiSeq* at = (MidiSeq*)p;
      at->processTimerTick();
      if (TIMER_DEBUG)
      {
        if(MidiSeq::ticker++ > 100)
          {
          printf("tick!\n");
          MidiSeq::ticker=0;
          }
        }
      }

//---------------------------------------------------------
//   processTimerTick
//---------------------------------------------------------

void MidiSeq::processTimerTick()
      {
      //---------------------------------------------------
      //    read elapsed rtc timer ticks
      //---------------------------------------------------

      // This is required otherwise it freezes.
      unsigned long nn;
      if (timerFd != -1) {
            nn = timer->getTimerTicks();
            nn >>= 8;
            }

      if (idle)
            return;

      if (MusEGlobal::midiBusy) {
            // we hit MusEGlobal::audio: MusEGlobal::midiSeq->msgProcess (actually this has been MusEGlobal::audio->processMidi for some time now - Tim)
            // miss this timer tick
            return;
            }

      unsigned curFrame = MusEGlobal::audio->curFrame();
      
      if (!MusEGlobal::extSyncFlag.value()) {
            int curTick = lrint((double(curFrame)/double(MusEGlobal::sampleRate)) * double(MusEGlobal::tempomap.globalTempo()) * double(MusEGlobal::config.division) * 10000.0 / double(MusEGlobal::tempomap.tempo(MusEGlobal::song->cpos())));
              
            if(midiClock > curTick)
              midiClock = curTick;
            
            int div = MusEGlobal::config.division/24;
            if(curTick >= midiClock + div)  {
                  int perr = (curTick - midiClock) / div;
                  
                  bool used = false;
                  
                    for(int port = 0; port < MIDI_PORTS; ++port)
                    {
                      MidiPort* mp = &MusEGlobal::midiPorts[port];
                      
                      // No device? Clock out not turned on? DELETETHIS 3
                      if(!mp->device() || !mp->syncInfo().MCOut())
                        continue;
                        
                      used = true;
                      
                      mp->sendClock();
                    }
                    
                    // DELETETHIS 35 ??
                    /*
                    for(iMidiDevice imd = MusEGlobal::midiDevices.begin(); imd != MusEGlobal::midiDevices.end(); ++imd) 
                    {
                      MidiDevice* dev = *imd;
                      
                      if(!dev->syncInfo().MCOut())
                        continue;
                        
                      // Shall we check open flags?
                      //if(!(dev->rwFlags() & 0x1) || !(dev->openFlags() & 1))
                      //if(!(dev->openFlags() & 1))
                      //  continue;
        
                      int port = dev->midiPort();
                      // Without this -1 check, interesting sync things can be done by the user without ever
                      //  assigning any devices to ports ! 
                      //if(port < 0 || port > MIDI_PORTS)
                      if(port < -1 || port > MIDI_PORTS)
                        continue;
                        
                      if(port == -1)
                      // Send straight to the device... Copied from MidiPort.
                      {
                        MidiPlayEvent event(0, 0, 0, ME_CLOCK, 0, 0);
                        dev->putEvent(event);
                      }  
                      else
                        // Go through the port...
                        MusEGlobal::midiPorts[port].sendClock();
                    }
                    */
                    
                    if(MusEGlobal::debugMsg && used && perr > 1)
                      printf("Dropped %d midi out clock(s). curTick:%d midiClock:%d div:%d\n", perr, curTick, midiClock, div);
                  //} DELETETHIS and maybe the below ???
                    
                  // Increment as if we had caught the timer exactly on the mark, even if the timer
                  //  has passed beyond the mark, or even beyond 2 * div.
                  // If we missed some chances to send clock, resume the count where it would have been, 
                  //  had we not missed chances.
                  // We can't do anything about missed chances except send right away, and make up
                  //  for gained time by losing time in the next count...
                  // In other words, use equalization periods to counter gained/lost time, so that
                  //  ultimately, over time, the receiver remains in phase, despite any short dropouts / phase glitches.
                  // (midiClock only increments by div units).
                  //
                  // Tested: With midi thread set to high priority, very few clock dropouts ocurred (P4 1.6Ghz).
                  // But target device tick drifts out of phase with muse tick slowly over time, say 20 bars or so.
                  // May need more tweaking, possibly use round with/instead of lrint (above), and/or
                  //  do not use equalization periods - set midiClock to fractions of div.
                  // Tested: With RTC resolution at 1024, stability was actually better than with 8192!
                  // It stayed in sync more than 64 bars...
                  //
                  //
                  // Using equalization periods...
                  midiClock += (perr * div);
                  //midiClock += perr; DELETETHIS
                  //
                  // No equalization periods... TODO: or DELETETHIS?
                  //midiClock += (perr * div);
               }
            }

      // play all events upto curFrame
      for (iMidiDevice id = MusEGlobal::midiDevices.begin(); id != MusEGlobal::midiDevices.end(); ++id) {
            //MidiDevice* md = *id; DELETETHIS 10
            // Is it a Jack midi device? They are iterated in Audio::processMidi. p3.3.36 
            //MidiJackDevice* mjd = dynamic_cast<MidiJackDevice*>(md);
            //if(mjd)
            //if(md->deviceType() == MidiDevice::JACK_MIDI)
            //  continue;
            //if(md->isSynti())      // syntis are handled by audio thread
            //      continue;
            // Only ALSA midi devices are handled by this thread.
            if((*id)->deviceType() == MidiDevice::ALSA_MIDI)
              (*id)->processMidi();
            
            // Moved into MidiAlsaDevice.      p4.0.34 DELETETHIS 40
            /*
            int port = md->midiPort();
            MidiPort* mp = port != -1 ? &MusEGlobal::midiPorts[port] : 0;
            MPEventList* el = md->playEvents();
            if (el->empty())
                  continue;
            
            ///iMPEvent i = md->nextPlayEvent();
            iMPEvent i = el->begin();            // p4.0.15 Tim.
            
            for (; i != el->end(); ++i) {
                  // p3.3.25
                  // If syncing to external midi sync, we cannot use the tempo map.
                  // Therefore we cannot get sub-tick resolution. Just use ticks instead of frames.
                  //if (i->time() > curFrame) {
                  if (i->time() > (extsync ? tickpos : curFrame)) {
                        //printf("  curT %d  frame %d\n", i->time(), curFrame);
                        break; // skip this event
                        }

                  if (mp) {
                        if (mp->sendEvent(*i))
                              break;
                        }
                  else {
                        if (md->putEvent(*i))
                              break;
                        }
                  }
            ///md->setNextPlayEvent(i);
            // p4.0.15 We are done with these events. Let us erase them here instead of Audio::processMidi.
            // That way we can simply set the next play event to the beginning.
            // This also allows other events to be inserted without the problems caused by the next play event 
            //  being at the 'end' iterator and not being *easily* set to some new place beginning of the newer insertions. 
            // The way that MPEventList sorts made it difficult to predict where the iterator of the first newly inserted items was.
            // The erasure in Audio::processMidi was missing some events because of that.
            el->erase(el->begin(), i);
            //md->setNextPlayEvent(el->begin());  // Removed p4.0.15
            */
            
            }
      }

//---------------------------------------------------------
//   msgMsg
//---------------------------------------------------------

void MidiSeq::msgMsg(int id)
      {
      MusECore::AudioMsg msg;
      msg.id = id;
      Thread::sendMsg(&msg);
      }

//---------------------------------------------------------
//   msgSetMidiDevice
//    to avoid timeouts in the RT-thread, setMidiDevice
//    is done in GUI context after setting the midi thread
//    into idle mode
//---------------------------------------------------------

void MidiSeq::msgSetMidiDevice(MidiPort* port, MidiDevice* device)
      {
        MusECore::AudioMsg msg;
        msg.id = MusECore::SEQM_IDLE;
        msg.a  = true;
        Thread::sendMsg(&msg);
        
        port->setMidiDevice(device);

        msg.id = MusECore::SEQM_IDLE;
        msg.a  = false;
        Thread::sendMsg(&msg);
      }

void MidiSeq::msgSeek()         { msgMsg(MusECore::SEQM_SEEK); }   
void MidiSeq::msgStop()         { msgMsg(MusECore::MS_STOP); }     
void MidiSeq::msgSetRtc()       { msgMsg(MusECore::MS_SET_RTC); }
void MidiSeq::msgUpdatePollFd() { msgMsg(MusECore::MS_UPDATE_POLL_FD); }

} // namespace MusECore
