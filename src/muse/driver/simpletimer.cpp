//=========================================================
//  MusE
//  Linux Music Editor
//
//  (C) Copyright 2015 Robert Jonsson (spamatica@gmail.com)
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

#include <cstdio>
#include "simpletimer.h"

#ifndef TIMER_DEBUG
#define TIMER_DEBUG 1
#endif

namespace MusECore {


  SimpleTimer::SimpleTimer()
     {
     if(TIMER_DEBUG)
       fprintf(stderr,"SimpleTimer::SimpleTimer(this=%p) called\n",this);
     }
     
  SimpleTimer::~SimpleTimer()
    {
    if(TIMER_DEBUG)
       fprintf(stderr,"SimpleTimer::~SimpleTimer(this=%p) called\n",this);
    }
  
  signed int SimpleTimer::initTimer(unsigned long)
    {
    if(TIMER_DEBUG)
      printf("SimpleTimer::initTimer(this=%p)\n",this);
  

    return 12345; // we return a dummy file descriptor for simplicity
    }             // current usage is only dummy timer which does not use the file descriptor
  
  unsigned long SimpleTimer::setTimerResolution(unsigned long)
    {
    return 0;
    }
  
  unsigned long SimpleTimer::setTimerFreq(unsigned long)
    {
    return 1000;
    }
  
  unsigned long SimpleTimer::getTimerResolution()
    {
    return  20;
    }

  unsigned long SimpleTimer::getTimerFreq()
    {
    return 1000;
    }
        
  bool SimpleTimer::startTimer()
    {
    QThread::start();
    return true;
    }
  
  bool SimpleTimer::stopTimer()
    {
    QThread::quit();
    return true;
    }
        
  unsigned long SimpleTimer::getTimerTicks(bool /*printTicks*/)
    {
    return tickCount;
    }

  void SimpleTimer::run()
  {
    bool keepRunning = true;
    while(keepRunning)
    {
      if (TIMER_DEBUG)
        printf("timer tick %ld\n",tickCount);
      usleep(1000);
      tickCount++;
    }
  }

} // namespace MusECore
