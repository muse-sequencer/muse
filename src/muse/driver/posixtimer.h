//=========================================================
//  MusE
//  Linux Music Editor
//
//   posixtimer.h
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

#ifndef __POSIXTIMER_H__
#define __POSIXTIMER_H__

#include "config.h"

#ifdef POSIX_TIMER_SUPPORT

#include "timerdev.h"

namespace MusECore {

//---------------------------------------------------------
//   PosixTimer
//---------------------------------------------------------

class PosixTimer : public Timer{
    int timerFd;

    public:
       PosixTimer();
       virtual ~PosixTimer();
       virtual const char * getTimerName() { return "PosixTimer"; }

       virtual signed int initTimer(unsigned long desiredFrequency);
       virtual unsigned long setTimerResolution(unsigned long resolution);
       virtual unsigned long getTimerResolution();
       virtual unsigned long setTimerFreq(unsigned long freq);
       virtual unsigned long getTimerFreq();

       virtual bool startTimer();
       virtual bool stopTimer();
       virtual unsigned long getTimerTicks(bool printTicks=false);
};

} // namespace MusECore

#endif // POSIX_TIMER_SUPPORT

#endif //__POSIXTIMER_H__

