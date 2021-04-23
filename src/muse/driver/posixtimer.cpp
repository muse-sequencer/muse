//=========================================================
//  MusE
//  Linux Music Editor
//
//   posixtimer.cpp
//  (C) Copyright 2019 Tim E. Real (terminator356 on sourceforge)
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

//#include "posixtimer.h"

#ifdef POSIX_TIMER_SUPPORT

#include "globals.h"

#include <climits>
#include <stdio.h>

#include <sys/timerfd.h>
#include <unistd.h>
#include <stdlib.h>
//#include <fcntl.h>

#define POSIX_TIMER_DEBUG 0

namespace MusECore {
  
PosixTimer::PosixTimer()
{
  if(TIMER_DEBUG || POSIX_TIMER_DEBUG)
    fprintf(stderr,"PosixTimer::PosixTimer(this=%p) called\n",this);
  timerFd = -1;
}
     
PosixTimer::~PosixTimer()
{
  if(TIMER_DEBUG || POSIX_TIMER_DEBUG)
      fprintf(stderr,"PosixTimer::~PosixTimer(this=%p) called\n",this);
  if(timerFd != -1)
  {
    if(close(timerFd) != 0)
      fprintf(stderr, "PosixTimer::PosixTimer(): timer close(%d) failed!\n", timerFd);
  }
}

signed int PosixTimer::initTimer(unsigned long desiredFrequency)
{
  if(TIMER_DEBUG || POSIX_TIMER_DEBUG)
    fprintf(stderr, "PosixTimer::initTimer(this=%p)\n",this);

  if(timerFd != -1)
  {
    fprintf(stderr, "PosixTimer::initTimer(): called on initialised timer!\n");
    return timerFd;
  }

  timerFd = timerfd_create(CLOCK_REALTIME, 0);
  if(timerFd == -1)
  {
    fprintf(stderr, "PosixTimer::initTimer(): timerfd_create failed\n");
    return -1;
  }

//   if(fcntl(timerFd, F_SETFL, O_NONBLOCK) == -1)
//   {
//     fprintf(stderr, "PosixTimer::initTimer(): fcntl(O_NONBLOCK) failed\n");
//     if(close(timerFd) == -1)
//       fprintf(stderr, "PosixTimer::PosixTimer(): timer close(%d) failed!\n", timerFd);
//     else
//       timerFd = -1;
//     return -1;
//   }
  
  if(!setTimerFreq(desiredFrequency))
  {
    // unable to set timer frequency
    if(close(timerFd) == -1)
      fprintf(stderr, "PosixTimer::PosixTimer(): timer close(%d) failed!\n", timerFd);
    else
      timerFd = -1;
    return -1;
  }
  
  return timerFd;
}

unsigned long PosixTimer::setTimerResolution(unsigned long resolution)
{
  if(TIMER_DEBUG || POSIX_TIMER_DEBUG)
    fprintf(stderr, "PosixTimer::setTimerResolution(%lu)\n",resolution);
  /* Resolution of a PosixTimer is fixed - it cannot be set */
  return 0;
}
  
unsigned long PosixTimer::setTimerFreq(unsigned long freq)
{
  if(timerFd == -1 || freq == 0)
    return 0;
  
  if(TIMER_DEBUG || POSIX_TIMER_DEBUG)
    fprintf(stderr, "PosixTimer::setTimerFreq(this=%p, freq=%ld)\n", this, freq);

  struct itimerspec new_value;

  const long int bil = 1000L * 1000000L;
  long int sec = 1 / freq;
  long int nsec = (bil / freq) % bil;

  // Don't start the timer yet.
  new_value.it_value.tv_sec = 0;
  new_value.it_value.tv_nsec = 0;
  
  new_value.it_interval.tv_sec = sec;
  new_value.it_interval.tv_nsec = nsec;

  //if(timerfd_settime(fd, TFD_TIMER_ABSTIME, &new_value, nullptr) == -1)
  if(timerfd_settime(timerFd, 0 /* Relative */, &new_value, nullptr) == -1)
  {
    fprintf(stderr, "PosixTimer::setTimerFreq(): timerfd_settime failed. sec:%ld nsec:%ld\n", sec, nsec);
    return 0;
  }
  
  // Return actual frequency.
  return getTimerFreq();
}
  
unsigned long PosixTimer::getTimerResolution()
{
  /* The PosixTimer doesn't really work with a set resolution as such.
    * Not sure how this fits into things yet.
    */
  return 0;
}

unsigned long PosixTimer::getTimerFreq()
{
  if(timerFd == -1)
    return 0;

  struct itimerspec curr_value;
  if(timerfd_gettime(timerFd, &curr_value) == -1)
  {
    fprintf(stderr, "PosixTimer::getTimerFreq(): timerfd_gettime(fd:%d) failed\n", timerFd);
    return 0;
  }

  // Avoid divide by zero.
  if(curr_value.it_interval.tv_sec == 0 && curr_value.it_interval.tv_nsec == 0)
  {
    fprintf(stderr, "PosixTimer::getTimerFreq(): timerfd_gettime(fd:%d): zero sec and nsec!\n", timerFd);
    return 0;
  }
  
  const long int bil = 1000L * 1000000L;
  const long int nsec = bil * curr_value.it_interval.tv_sec +
                        curr_value.it_interval.tv_nsec;
  const unsigned long int freq = bil / nsec;
  return freq;
}
        
bool PosixTimer::startTimer()
{
  if(timerFd == -1)
    return false;

  struct itimerspec curr_value;
  if(timerfd_gettime(timerFd, &curr_value) == -1)
  {
    fprintf(stderr, "PosixTimer::startTimer(): timerfd_gettime(fd:%d) failed\n", timerFd);
    return false;
  }

  // Start the timer.
  curr_value.it_value.tv_sec = curr_value.it_interval.tv_sec;
  curr_value.it_value.tv_nsec = curr_value.it_interval.tv_nsec;
  if(timerfd_settime(timerFd, 0 /* Relative */, &curr_value, nullptr) == -1)
  {
    fprintf(stderr, "PosixTimer::startTimer(): timerfd_settime failed. cur interval sec:%ld nsec:%ld\n",
            curr_value.it_interval.tv_sec, curr_value.it_interval.tv_nsec);
    return false;
  }
  return true;
}

bool PosixTimer::stopTimer()
{
  if(timerFd == -1)
    return false;

  struct itimerspec curr_value;
  if(timerfd_gettime(timerFd, &curr_value) == -1)
  {
    fprintf(stderr, "PosixTimer::stopTimer(): timerfd_gettime(fd:%d) failed\n", timerFd);
    return false;
  }

  // Stop the timer.
  curr_value.it_value.tv_sec = 0;
  curr_value.it_value.tv_nsec = 0;
  if(timerfd_settime(timerFd, 0 /* Relative */, &curr_value, nullptr) == -1)
  {
    fprintf(stderr, "PosixTimer::stopTimer(): timerfd_settime failed. cur interval sec:%ld nsec:%ld\n",
            curr_value.it_interval.tv_sec, curr_value.it_interval.tv_nsec);
    return false;
  }
  return true;
}
        
unsigned long PosixTimer::getTimerTicks(bool printTicks)
{
  if(timerFd == -1)
    return 0;

  uint64_t nn;
  if (read(timerFd, &nn, sizeof(nn)) != sizeof(nn)) {
      fprintf(stderr,"PosixTimer::getTimerTicks(): error reading POSIX timer\n");
      return 0;
      }
  if (printTicks) {
      fprintf(stderr, "TIMER: ticks = %ld\n", (unsigned long)nn);
      }
  return nn;
}

} // namespace MusECore

#endif // POSIX_TIMER_SUPPORT
