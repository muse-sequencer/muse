//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: midiseq.cpp,v 1.30.2.21 2009/12/20 05:00:35 terminator356 Exp $
//
//    high priority task for scheduling midi events
//
//  (C) Copyright 2003 Werner Schweer (ws@seh.de)
//  (C) Copyright 2016 Tim E. Real (terminator356 on sourceforge.net)
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
#ifndef _WIN32
#include <sys/ioctl.h>
#include <poll.h>
#endif
#include "muse_math.h"
#include <errno.h>

#include "config.h"
#include "app.h"
#include "globals.h"
#ifdef _WIN32
#include "driver/qttimer.h"
#else
#include "driver/alsatimer.h"
#include "driver/rtctimer.h"
#include "driver/posixtimer.h"
#endif
#include "midi_consts.h"
#include "midiseq.h"
#include "midiport.h"
#include "mididev.h"
#include "audio.h"
#include "audiodev.h"
#include "driver/alsamidi.h"
#include "sync.h"
#include "song.h"
#include "gconfig.h"
#include "warn_bad_timing.h"
#include "large_int.h"

namespace MusEGlobal {
MusECore::MidiSeq* midiSeq = nullptr;
}

namespace MusECore {

int MidiSeq::ticker = 0;

void initMidiSequencer()   
{
  if(!MusEGlobal::midiSeq)
    MusEGlobal::midiSeq = new MidiSeq("Midi");
}

void exitMidiSequencer()
{
  // Sequencer should be stopped before calling exitMidiSequencer(). Todo: maybe check that here.
  if(MusEGlobal::midiSeq)
  {
    //MusEGlobal::midiSeqRunning = false; // Done in stop.
    delete MusEGlobal::midiSeq;
    MusEGlobal::midiSeq = nullptr;
  }
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
                  fprintf(stderr, "MidiSeq::processMsg() unknown id %d\n", msg->id);
                  break;
            }
      }

//---------------------------------------------------------
//   processStop
//---------------------------------------------------------

void MidiSeq::processStop()
{
  // Clear Alsa midi device notes and stop stuck notes.
  for(iMidiDevice id = MusEGlobal::midiDevices.begin(); id != MusEGlobal::midiDevices.end(); ++id)
  {
    MidiDevice* md = *id;
    const MidiDevice::MidiDeviceType type = md->deviceType();
    // Only for ALSA devices.
    switch(type)
    {
      case MidiDevice::ALSA_MIDI:
        md->handleStop();
      break;

      case MidiDevice::JACK_MIDI:
      case MidiDevice::SYNTH_MIDI:
      break;
    }
  }
}

//---------------------------------------------------------
//   processSeek
//---------------------------------------------------------

void MidiSeq::processSeek()
{
  //---------------------------------------------------
  //    Set all controllers
  //---------------------------------------------------

  for (iMidiDevice i = MusEGlobal::midiDevices.begin(); i != MusEGlobal::midiDevices.end(); ++i)
  {
    MidiDevice* md = *i;
    const MidiDevice::MidiDeviceType type = md->deviceType();
    // Only for ALSA devices.
    switch(type)
    {
      case MidiDevice::ALSA_MIDI:
        md->handleSeek();
      break;

      case MidiDevice::JACK_MIDI:
      case MidiDevice::SYNTH_MIDI:
      break;
    }
  }
}


//---------------------------------------------------------
//   MidiSeq
//---------------------------------------------------------

MidiSeq::MidiSeq(const char* name)
   : Thread(name)
      {
      prio = 0;
      
      idle = false;
      
      MusEGlobal::doSetuid();
      timerFd=selectTimer();

      MusEGlobal::undoSetuid();
      }

//---------------------------------------------------------
//   ~MidiSeq
//---------------------------------------------------------

MidiSeq::~MidiSeq()
    {
    if(timer)
      delete timer;
    }

//---------------------------------------------------------
//   selectTimer()
//   select one of the supported timers to use during this run
//---------------------------------------------------------

signed int MidiSeq::selectTimer()
    {
#ifdef _WIN32
    fprintf(stderr, "Trying Qt timer...\n");
    timer = new QtTimer();
    int qt_tmrFd = timer->initTimer(MusEGlobal::config.rtcTicks);
    if (qt_tmrFd != -1) { // ok!
        fprintf(stderr, "got timer = %d\n", qt_tmrFd);
        return qt_tmrFd;
    }
    delete timer;
#else
  #ifdef ALSA_SUPPORT
    fprintf(stderr, "Trying RTC timer...\n");
    timer = new RtcTimer();
    int rtc_tmrFd = timer->initTimer(MusEGlobal::config.rtcTicks);
    if (rtc_tmrFd != -1) { // ok!
        fprintf(stderr, "got timer = %d\n", rtc_tmrFd);
        return rtc_tmrFd;
    }
    delete timer;
  #endif
#endif
  
#ifdef POSIX_TIMER_SUPPORT
    fprintf(stderr, "Trying POSIX timer...\n");
    timer = new PosixTimer();
    int posix_tmrFd = timer->initTimer(MusEGlobal::config.rtcTicks);
    if (posix_tmrFd!= -1) { // ok!
        fprintf(stderr, "got timer = %d\n", posix_tmrFd);
        return posix_tmrFd;
    }
    delete timer;
#endif

#ifdef ALSA_SUPPORT
    fprintf(stderr, "Trying ALSA timer...\n");
    timer = new AlsaTimer();
    int alsa_tmrFd = timer->initTimer(MusEGlobal::config.rtcTicks);
    if (alsa_tmrFd!= -1) { // ok!
        fprintf(stderr, "got timer = %d\n", alsa_tmrFd);
        return alsa_tmrFd;
    }
    delete timer;
#endif

    timer=nullptr;
    QMessageBox::critical( 0, /*tr*/(QString("Failed to start timer!")),
              /*tr*/(QString("No functional timer was available.\n"
                         "RTC timer not available, check if /dev/rtc is available and readable by current user\n"
                         "Alsa timer not available, check if module snd_timer is available and /dev/snd/timer is available")));
    fprintf(stderr, "No functional timer available!!!\n");
    exit(1);
    }

//---------------------------------------------------------
//   deleteTimer()
//   Destroy timer if valid.
//---------------------------------------------------------

bool MidiSeq::deleteTimer()
{
  if(timer)
  {
    delete timer;
    timer = nullptr;
    return true;
  }
  return false;
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
            if ((dev->rwFlags() & 0x2) || (MusEGlobal::extSyncFlag
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
      if(gotTicks == 0)
        return 0;
      if (MusEGlobal::config.rtcTicks-24 > gotTicks) {
          fprintf(stderr, "INFO: Could not get the wanted frequency %d, got %d, still it should suffice.\n", MusEGlobal::config.rtcTicks, gotTicks);
      }
      else
        fprintf(stderr, "INFO: Requested timer frequency:%d actual:%d\n", MusEGlobal::config.rtcTicks, gotTicks);

      timer->startTimer();
      return gotTicks;
      }

//---------------------------------------------------------
//   start
//    return true on error
//---------------------------------------------------------

void MidiSeq::start(int /*priority*/, void*)
{
  // Already running?
  if(isRunning())
    return;

  if(!MusEGlobal::audioDevice)
  {
    fprintf(stderr, "MusE::seqStartMidi: audioDevice is NULL\n");
    return;
  }

  if(!MusEGlobal::audio->isRunning())
  {
    fprintf(stderr, "MusE::seqStartMidi: audio is not running\n");
    return;
  }

  int midiprio = 0;

  // NOTE: MusEGlobal::realTimeScheduling can be true (gotten using jack_is_realtime()),
  //  while the determined MusEGlobal::realTimePriority can be 0.
  // MusEGlobal::realTimePriority is gotten using pthread_getschedparam() on the client thread
  //  in JackAudioDevice::realtimePriority() which is a bit flawed - it reports there's no RT...
  if(MusEGlobal::realTimeScheduling)
  {
    if(MusEGlobal::realTimePriority - 1 >= 0)
      midiprio = MusEGlobal::realTimePriority - 1;
  }

  if(MusEGlobal::midiRTPrioOverride > 0)
    midiprio = MusEGlobal::midiRTPrioOverride;

  // FIXME: The MusEGlobal::realTimePriority of the Jack thread seems to always be 5 less than the value passed to jackd command.

  prio = midiprio;

  MusEGlobal::doSetuid();
  int freq = setRtcTicks();
  MusEGlobal::undoSetuid();
  
  if(freq == 0)
  {
    fprintf(stderr, "Error setting timer frequency! Midi playback will not work!\n");
  }
  
  Thread::start(prio);

  int counter=0;
  while (++counter) {
    if (counter > 1000) {
        fprintf(stderr,"midi sequencer thread does not start!? Exiting...\n");
//         exit(33);
        break;
    }
    MusEGlobal::midiSeqRunning = MusEGlobal::midiSeq->isRunning();
    if (MusEGlobal::midiSeqRunning)
      break;
    usleep(1000);
    if(MusEGlobal::debugMsg)
      printf("looping waiting for sequencer thread to start\n");
  }
  if(!MusEGlobal::midiSeqRunning)
  {
//       fprintf(stderr, "midiSeq is not running! Exiting...\n");
//       exit(33);
    fprintf(stderr, "midiSeq is still not running!\n");
  }
}


//---------------------------------------------------------
//   checkAndReportTimingResolution
//---------------------------------------------------------
void MidiSeq::checkAndReportTimingResolution()
{
    int freq = timer->getTimerFreq();
    fprintf(stderr, "Acquired timer frequency: %d\n", freq);
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
            // Save settings. Use simple version - do NOT set style or stylesheet, this has nothing to do with that.
            //MusEGlobal::muse->changeConfig(true);  // Save settings? No, wait till close.
          }
        }
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
          fprintf(stderr, "ticker:%i\n", MidiSeq::ticker);
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

      unsigned curFrame = MusEGlobal::audio->curFrame();
      
// REMOVE Tim. clock. Removed. Done with scheduling now in audio thread process.
      if (!MusEGlobal::extSyncFlag) {
            // Do not round up here since (audio) frame resolution is higher than tick resolution.
            const unsigned int curTick = muse_multiply_64_div_64_to_64(
              (uint64_t)MusEGlobal::config.division * (uint64_t)MusEGlobal::tempomap.globalTempo() * 10000UL, curFrame,
              (uint64_t)MusEGlobal::sampleRate * (uint64_t)MusEGlobal::tempomap.tempo(MusEGlobal::song->cpos()));
            
            unsigned int mclock = MusEGlobal::midiSyncContainer.midiClock();
            if(mclock > curTick)
            {
              mclock = curTick;
              MusEGlobal::midiSyncContainer.setMidiClock(mclock);
            }

            const unsigned int div = MusEGlobal::config.division/24;
            if(curTick >= mclock + div)  {
                  const unsigned int perr = (curTick - mclock) / div;
                  
                  bool used = false;
                  
                    for(int port = 0; port < MusECore::MIDI_PORTS; ++port)
                    {
                      MidiPort* mp = &MusEGlobal::midiPorts[port];
                      
                      // No device? Clock out not turned on?
                      if(!mp->device() || !mp->syncInfo().MCOut())
                        continue;
                        
                      used = true;
                      
                      mp->sendClock();
                    }
                    
                    if(MusEGlobal::debugMsg && used && perr > 1)
                      printf("Dropped %u midi out clock(s). curTick:%u midiClock:%u div:%u\n", perr, curTick, MusEGlobal::midiSyncContainer.midiClock(), div);

                  // Using equalization periods...
                  MusEGlobal::midiSyncContainer.setMidiClock(mclock + (perr * div));
               }
            }

      // Play all events up to curFrame.
      for (iMidiDevice id = MusEGlobal::midiDevices.begin(); id != MusEGlobal::midiDevices.end(); ++id)
      {
        MidiDevice* md = *id;
        const MidiDevice::MidiDeviceType type = md->deviceType();
        // Only for ALSA devices.
        switch(type)
        {
          case MidiDevice::ALSA_MIDI:
              md->processMidi(curFrame);
          break;

          case MidiDevice::JACK_MIDI:
          case MidiDevice::SYNTH_MIDI:
          break;
        }
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

void MidiSeq::msgSeek()         { msgMsg(MusECore::SEQM_SEEK); }   
void MidiSeq::msgStop()         { msgMsg(MusECore::MS_STOP); }     
void MidiSeq::msgSetRtc()       { msgMsg(MusECore::MS_SET_RTC); }
void MidiSeq::msgUpdatePollFd() { msgMsg(MusECore::MS_UPDATE_POLL_FD); }

} // namespace MusECore
