//=============================================================================
//  MusE
//  Linux Music Editor
//  $Id:$
//
//  (C) Copyright 2004 Robert Jonsson (rj@spamatica.se)
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

#include "coretimer.h"
#include "globals.h"
#include "gconfig.h"


CoreTimer::CoreTimer()
    {
    timerFd = -1;
    }

CoreTimer::~CoreTimer()
    {
    if (timerFd != -1)
      close(timerFd);
    }

bool CoreTimer::initTimer()
    {
    if(TIMER_DEBUG)
          printf("CoreTimer::initTimer()\n");
/*    if (timerFd != -1) {
          fprintf(stderr,"CoreTimer::initTimer(): called on initialised timer!\n");
          return -1;
    }
    doSetuid();

    timerFd = ::open("/dev/rtc", O_RDONLY);
    if (timerFd == -1) {
          fprintf(stderr, "fatal error: open /dev/rtc failed: %s\n", strerror(errno));
          undoSetuid();
          return timerFd;
          }
    if (!setTimerFreq(config.rtcTicks)) {
          // unable to set timer frequency
          return -1;
          }
    // check if timer really works, start and stop it once.
    if (!startTimer()) {
          return -1;
          }
    if (!stopTimer()) {
          return -1;
          }
*/
    return true;
    }

unsigned int CoreTimer::setTimerResolution(unsigned int resolution)
    {
    if(TIMER_DEBUG)
      printf("CoreTimer::setTimerResolution(%d)\n",resolution);
    /* The RTC can take power-of-two frequencies from 2 to 8196 Hz.
     * It doesn't really have a resolution as such.
     */
    return 0;
    }

bool CoreTimer::setTimerFreq(unsigned int freq)
    {
/*    int rc = ioctl(timerFd, RTC_IRQP_SET, freq);
    if (rc == -1) {
            fprintf(stderr, "CoreTimer::setTimerFreq(): cannot set tick on /dev/rtc: %s\n",
               strerror(errno));
            fprintf(stderr, "  precise timer not available\n");
            return 0;
            }
*/
    return true ;
    }

int CoreTimer::getTimerResolution()
    {
    /* The RTC doesn't really work with a set resolution as such.
     * Not sure how this fits into things yet.
     */
    return 0;
    }

unsigned int CoreTimer::getTimerFreq()
    {
    unsigned int freq;
/*    int rv = ioctl(timerFd, RTC_IRQP_READ, &freq);
    if (rv < 1)
      return 0;
*/
    return freq;
    }

bool CoreTimer::startTimer()
    {
    if(TIMER_DEBUG)
      printf("CoreTimer::startTimer()\n");
/*    if (timerFd == -1) {
          fprintf(stderr, "CoreTimer::startTimer(): no timer open to start!\n");
          return false;
          }
    if (ioctl(timerFd, RTC_PIE_ON, 0) == -1) {
        perror("MidiThread: start: RTC_PIE_ON failed");
        undoSetuid();
        return false;
        }
*/    return true;
    }

bool CoreTimer::stopTimer()
    {
/*    if(TIMER_DEBUG)
      printf("CoreTimer::stopTimer\n");
    if (timerFd != -1) {
        ioctl(timerFd, RTC_PIE_OFF, 0);
        }
    else {
        fprintf(stderr,"CoreTimer::stopTimer(): no RTC to stop!\n");
        return false;
        }
*/    return true;
    }

unsigned long CoreTimer::getTimerTicks()
    {
    if(TIMER_DEBUG)
      printf("getTimerTicks()\n");
    unsigned long int nn;
/*    if (timerFd==-1) {
        fprintf(stderr,"CoreTimer::getTimerTicks(): no RTC open to read!\n");
        return 0;
    }
    if (read(timerFd, &nn, sizeof(unsigned long)) != sizeof(unsigned long)) {
        fprintf(stderr,"CoreTimer::getTimerTicks(): error reading RTC\n");
        return 0;
        }
*/
    return nn;
    }

