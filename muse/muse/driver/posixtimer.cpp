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

//
// support posix high resolution timers
//    http://sourceforge.net/projects/high-res-timers
//
#include <signal.h>
#include <memory.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "posixtimer.h"

extern bool debugMsg;

static int fd[2];

//---------------------------------------------------------
//   getFd
//---------------------------------------------------------

int PosixTimer::getFd() const
      {
      return fd[0];
      }

//---------------------------------------------------------
//   sigalarm
//---------------------------------------------------------

static void sigalarm(int /*signo*/)
      {
      write(fd[1], ".", 1);
      }

//---------------------------------------------------------
//   PosixTimer
//---------------------------------------------------------

PosixTimer::PosixTimer()
      {
      timerId = 0;
      }

PosixTimer::~PosixTimer()
      {
      if (timerId != 0)
            timer_delete(timerId);
      }

//---------------------------------------------------------
//   initTimer
//    return -1 on error
//---------------------------------------------------------

bool PosixTimer::initTimer()
      {
      pthread_t tid = pthread_self();
printf("tid %p\n", (void*)tid);
      struct sigevent se;
      memset(&se, 0, sizeof(se));
      se.sigev_notify = SIGEV_THREAD_ID;
      se.sigev_signo  = SIGRTMAX;
//      se.sigev_value.sival_ptr = this;
      se._sigev_un._tid = tid;

      // allocate timer
      int rv;
      clock = CLOCK_REALTIME;
      rv = timer_create(clock, &se, &timerId);

      if (rv < 0) {
            perror("posix timer create");
            fprintf(stderr, "no posix timer available\n");
            return false;
            }
      else if (debugMsg) {
            if (clock == CLOCK_REALTIME)
                  printf("created CLOCK_REALTIME posix timer\n");
            }

      // Set up signal handler:

      struct sigaction act;
      sigfillset(&act.sa_mask);
      act.sa_flags = 0;
      act.sa_handler = sigalarm;
      sigaction(SIGRTMAX, &act, 0);

      if (pipe(fd) < 0) {
            perror("PosixTimer::initTimer: create pipe");
            return false;
            }
      return true;
      }

//---------------------------------------------------------
//   getTimerResolution
//    return timer resolution in microseconds
//---------------------------------------------------------

int PosixTimer::getTimerResolution()
      {
      struct timespec spec;
      int rv = clock_getres(clock, &spec);
      if (rv != -1)
            return spec.tv_nsec / 1000;
      return 0;
      }

//---------------------------------------------------------
//   setTimerTicks
//    return false on error
//---------------------------------------------------------

bool PosixTimer::setTimerTicks(int tick)
      {
      double nsec = (1000.0*1000.0*1000.0) / tick;
      ts.it_interval.tv_sec  = 0;
      ts.it_interval.tv_nsec = int(nsec);
      ts.it_value = ts.it_interval;
      return true;
      }

//---------------------------------------------------------
//   startTimer
//---------------------------------------------------------

bool PosixTimer::startTimer()
      {
      int rv = timer_settime(timerId, 0, &ts, 0);
      if (rv < 0) {
            perror("PosixTimer::startTimer: timer_settime");
            return false;
            }
      return true;
      }

//---------------------------------------------------------
//   stopTimer
//---------------------------------------------------------

bool PosixTimer::stopTimer()
      {
      struct itimerspec s;
      s.it_interval.tv_sec  = 0;
      s.it_interval.tv_nsec = 0;
      s.it_value.tv_sec = 0;
      s.it_value.tv_nsec = 0;

      int rv = timer_settime(timerId, 0, &s, 0);
      if (rv < 0) {
            perror("PosixTimer::stopTimer: timer_settime");
            return false;
            }
      return true;
      }

//---------------------------------------------------------
//   getTimerTicks
//---------------------------------------------------------

unsigned long PosixTimer::getTimerTicks()
      {
      char buffer[16];
      int n = read(fd[0], buffer, 16);
if (n != 1)
      printf("getTimerTicks %d\n", n);
      return n;
//      int n = timer_getoverrun(timerId);
//      return n;
      }

