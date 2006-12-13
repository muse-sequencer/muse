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

#include "midiseq.h"
#include "audio.h"
#include "driver/mididev.h"
#include "driver/audiodev.h"

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

MidiSeq* midiSeq;

//---------------------------------------------------------
//   MidiSeq
//---------------------------------------------------------

MidiSeq::MidiSeq(const char* name)
   : Thread(name)
      {
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
            	memset(&rt_param, 0, sizeof(sched_param));
            	int type;
            	int rv = pthread_getschedparam(pthread_self(), &type, &rt_param);
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
//      addPollFd(toThreadFdr, POLLIN, ::readMsg, this, 0);

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
//   midiTick
//    schedule events in playEvents
//    "now" is the free running audioDriver->frameTime()
//    frame counter
//---------------------------------------------------------

void MidiSeq::midiTick(void* p, void*)
      {
      extern int watchMidi;
      ++watchMidi;            // make a simple watchdog happy

      MidiSeq* at = (MidiSeq*)p;
      at->getTimerTicks();    // read elapsed rtc timer ticks

      while (!at->fifo.isEmpty())
            at->playEvents.insert(at->fifo.get());

      //
      // schedule all events upto curFrame()
      //
      unsigned curFrame = audioDriver->frameTime();
      iMidiOutEvent i = at->playEvents.begin();
      for (; i != at->playEvents.end(); ++i) {
            if (i->event.time() > curFrame)
                  break;
            midiDriver->putEvent(i->port, i->event);
            }
      at->playEvents.erase(at->playEvents.begin(), i);
      }

