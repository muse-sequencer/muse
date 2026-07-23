//=========================================================
//  MusE
//  Linux Music Editor
//
//  Monotonic timer using timerfd(CLOCK_MONOTONIC)
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

#ifndef __TIMER_MONO_H__
#define __TIMER_MONO_H__

#include "config.h"

#ifndef _WIN32

#include "timerdev.h"

namespace MusECore {

//---------------------------------------------------------
//   MonoTimer
//---------------------------------------------------------

class MonoTimer : public Timer {

    public:
       MonoTimer();
       virtual ~MonoTimer();
       virtual const char * getTimerName() { return "MonoTimer"; }

       virtual signed int initTimer(unsigned long desiredFrequency);
       virtual unsigned long setTimerResolution(unsigned long resolution);
       virtual unsigned long getTimerResolution();
       virtual unsigned long setTimerFreq(unsigned long tick);
       virtual unsigned long getTimerFreq();

       virtual bool startTimer();
       virtual bool stopTimer();
       virtual unsigned long getTimerTicks(bool printTicks=false);

       bool set_real_time_prio();

    private:
      int timerFd;
      unsigned long timerResolution;
};

} // namespace MusECore

#endif // _WIN32

#endif // __TIMER_MONO_H__
