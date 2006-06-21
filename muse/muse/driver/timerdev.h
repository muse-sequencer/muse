//=============================================================================
//  MusE
//  Linux Music Editor
//  $Id:$
//
//  Plenty of code borrowed from timer.c example in
//  alsalib 1.0.7
//
//  (C) Copyright 2004 Robert Jonsson (rj@spamatica.se)
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

#ifndef __TIMERDEV_H__
#define __TIMERDEV_H__

#include <pthread.h>

#define TIMER_DEBUG 0

//---------------------------------------------------------
//   Timer
//---------------------------------------------------------

class Timer {

     public:
      Timer() {};
      virtual ~Timer() {};

      virtual bool initTimer() = 0;
      virtual int  getTimerResolution() = 0;
      virtual bool setTimerFreq(unsigned int freq) = 0;

      virtual bool startTimer() = 0;
      virtual bool stopTimer() = 0;
      virtual unsigned long getTimerTicks() = 0;
      virtual int getFd() const { return -1; }
      };

#endif //__TIMERDEV_H__
