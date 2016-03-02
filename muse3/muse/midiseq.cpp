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
#include "warn_bad_timing.h"

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
  
  // clear Alsa midi device notes and stop stuck notes
  for(iMidiDevice id = MusEGlobal::midiDevices.begin(); id != MusEGlobal::midiDevices.end(); ++id) 
      (*id)->handleStop();  
}
#endif

//---------------------------------------------------------
//   processSeek
//---------------------------------------------------------

void MidiSeq::processSeek()
{
  //---------------------------------------------------
  //    set all controller
  //---------------------------------------------------

  for (iMidiDevice i = MusEGlobal::midiDevices.begin(); i != MusEGlobal::midiDevices.end(); ++i) 
      (*i)->handleSeek();  
}

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

//---------------------------------------------------------
//   midiWrite
//---------------------------------------------------------

static void midiWrite(void*, void* d)
      {
      MidiDevice* dev = (MidiDevice*) d;
      dev->flush();
      }

void MidiSeq::addAlsaPollFd()
{
  // special handling for alsa midi:
  // (one fd for all devices)
  //    this allows for processing of some alsa events
  //    even if no alsa driver is active (assigned to a port)
  addPollFd(alsaSelectRfd(), POLLIN, MusECore::alsaMidiRead, this, 0);
}
      
void MidiSeq::removeAlsaPollFd()
{
  removePollFd(alsaSelectRfd(), POLLIN);
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
      addAlsaPollFd(); 
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
      if (MusEGlobal::config.rtcTicks-24 > gotTicks) {
          printf("INFO: Could not get the wanted frequency %d, got %d, still it should suffice.\n", MusEGlobal::config.rtcTicks, gotTicks);
      }
      timer->startTimer();
      return gotTicks;
      }

//---------------------------------------------------------
//   start
//    return true on error
//---------------------------------------------------------

void MidiSeq::start(int priority, void *)
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
    printf("Aquired timer frequency: %d\n", freq);
    if (freq < 500) {
        if(MusEGlobal::config.warnIfBadTiming)
        {
          MusEGui::WarnBadTimingDialog dlg;
          dlg.setLabelText(qApp->translate("@default", QT_TRANSLATE_NOOP("@default", 
                             "Timing source frequency is %1hz, which is below the recommended minimum: 500hz!\n" 
                             "This could lead to audible timing problems for MIDI.\n" 
                             "Please see the FAQ on http://muse-sequencer.org for remedies.\n" 
                             "Also please check console output for any further error messages.\n ")).arg(freq) );
          
          dlg.exec();
          bool warn = !dlg.dontAsk();
          if(warn != MusEGlobal::config.warnIfBadTiming)  
          {
            MusEGlobal::config.warnIfBadTiming = warn;
            //MusEGlobal::muse->changeConfig(true);  // Save settings? No, wait till close.
          }
        }
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
                    
                    if(MusEGlobal::debugMsg && used && perr > 1)
                      printf("Dropped %d midi out clock(s). curTick:%d midiClock:%d div:%d\n", perr, curTick, midiClock, div);

                  // Using equalization periods...
                  midiClock += (perr * div);
               }
            }

      // play all events upto curFrame
      for (iMidiDevice id = MusEGlobal::midiDevices.begin(); id != MusEGlobal::midiDevices.end(); ++id)
            if((*id)->deviceType() == MidiDevice::ALSA_MIDI)
              (*id)->processMidi();
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
