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
#include <unistd.h>
#include <fcntl.h>
#include "qttimer.h"

#ifndef TIMER_DEBUG
#define TIMER_DEBUG 1
#endif

#ifdef _WIN32
#define pipe(fds) _pipe(fds, 4096, _O_BINARY)
#endif

namespace MusECore {


  QtTimer::QtTimer()
     {
     if(TIMER_DEBUG)
       fprintf(stderr,"QtTimer::QtTimer(this=%p) called\n",this);
     innerTimer = nullptr;
     timeoutms = 10;
     readPipe=-1;
     writePipe=-1;
     }
     
  QtTimer::~QtTimer()
    {
    if(TIMER_DEBUG)
       fprintf(stderr,"QtTimer::~QtTimer(this=%p) called\n",this);
    exit(); // thread exit
    }
  
  signed int QtTimer::initTimer(unsigned long)
  {
    if(TIMER_DEBUG)
      printf("QtTimer::initTimer(this=%p)\n",this);

    int filedes[2];         // 0 - reading   1 - writing
    if (pipe(filedes) == -1) {
          perror("QtTimer - creating pipe failed");
          exit(-1);
          }
#ifndef _WIN32
    int rv = fcntl(filedes[1], F_SETFL, O_NONBLOCK);
    if (rv == -1)
          perror("set pipe O_NONBLOCK");
#endif
    if (pipe(filedes) == -1) {
          perror("QtTimer - creating pipe1");
          exit(-1);
          }
    writePipe = filedes[1];
    readPipe = filedes[0];

    return filedes[0];
  }
  
  long unsigned int QtTimer::setTimerResolution(unsigned long)
    {
    return 0;
    }
  
  long unsigned int QtTimer::setTimerFreq(unsigned long freq)
    {
    if (freq > 1000)
      freq = 1000;
    if (freq < 100)
      freq = 100;
    timeoutms = 1000/freq;
    return 1000/timeoutms;
    }
  
  long unsigned int QtTimer::getTimerResolution()
    {
    return  20;
    }

  long unsigned int QtTimer::getTimerFreq()
    {
    return 1000/timeoutms;
    }
        
  bool QtTimer::startTimer()
    {
    QThread::start();
    return true;
    }
  
  bool QtTimer::stopTimer()
    {
    QThread::quit();
    return true;
    }
        
  unsigned long int  QtTimer::getTimerTicks(bool /*printTicks*/)
    {

    if(TIMER_DEBUG)
      printf("getTimerTicks()\n");
    unsigned long int nn;
    if (readPipe==-1) {
        fprintf(stderr,"QtTimer::getTimerTicks(): no pipe open to read!\n");
        return 0;
    }
    if (read(readPipe, &nn, sizeof(char)) != sizeof(char)) {
        fprintf(stderr,"QtTimer::getTimerTicks(): error reading pipe\n");
        return 0;
        }
    //return nn;

    return innerTimer != 0 ? innerTimer->getTick() : 0;

    }

  void QtTimer::run()
  {

    //bool keepRunning = true;
    innerTimer = new InnerTimer();
    innerTimer->setupTimer(writePipe, timeoutms); // make sure it is running in the right thread

    exec();
  }

  void InnerTimer::setupTimer(int fd, int timeoutms)
  {
    tickCount=0;
    writePipe = fd;
    timer.start(timeoutms, this);
    printf("InnerTimer::setupTimer() started\n");

  }

  InnerTimer::~InnerTimer()
  {
    timer.stop();
  }

  void InnerTimer::timerEvent(QTimerEvent *event)
  {
    //if (tickCount%1000)
      //printf("InnerTimer::timerEvent %ld ++++++++++++++++++++++\n",tickCount);

    if (event->timerId() == timer.timerId()) {
      tickCount++;
      write(writePipe,"t",1);
    }

  }

  long int InnerTimer::getTick()
  {
    return tickCount;
  }

} // namespace MusECore
