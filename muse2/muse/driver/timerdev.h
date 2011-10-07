//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: timerdev.h,v 1.1.2.3 2005/08/21 18:11:28 spamatica Exp $
//
//  Plenty of code borrowed from timer.c example in 
//  alsalib 1.0.7
//
//  (C) Copyright 2004 Robert Jonsson (rj@spamatica.se)
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

#ifndef __TIMERDEV_H__
#define __TIMERDEV_H__

#include "alsa/asoundlib.h"

#define TIMER_DEBUG 0

namespace MusECore {

//---------------------------------------------------------
//   AlsaTimer
//---------------------------------------------------------

class Timer {
    
     public:
       Timer() {};
       virtual ~Timer() {};
       
       virtual signed int initTimer() = 0;
       virtual unsigned int setTimerResolution(unsigned int resolution) = 0;
       virtual unsigned int getTimerResolution() = 0;
       virtual unsigned int setTimerFreq(unsigned int freq) = 0;
       virtual unsigned int getTimerFreq() = 0;
       
       virtual bool startTimer() = 0;
       virtual bool stopTimer() = 0;
       virtual unsigned int getTimerTicks(bool printTicks = false) = 0;
        
};

} // namespace MusECore

#endif //__ALSATIMER_H__
