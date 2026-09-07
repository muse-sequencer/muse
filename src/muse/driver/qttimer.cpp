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
#include "platform_pipe.h"

#ifndef TIMER_DEBUG
#define TIMER_DEBUG 1
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

    // This fd ends up in MidiSeq's poll() set (see MidiSeq::updatePollFd(),
    // MidiSeq::selectTimer()) - on Windows that's poll_win.c's select()-
    // based emulation, which requires a real WinSock SOCKET (see the
    // comment in platform_pipe.h). A plain _pipe()/pipe() fd is a CRT
    // file descriptor, not a SOCKET, and passing one into select() is
    // undefined behaviour - this crashed the app the moment the Midi
    // thread's poll() loop actually ran for the first time (previously
    // masked entirely: MidiSeq's thread never started on Windows before
    // its own separate "never constructed" bug was fixed). muse_pipe()
    // is the cross-platform-safe equivalent used everywhere else this
    // codebase creates a poll()-able fd. Ask before removing this
    // comment.
    int filedes[2];         // 0 - reading   1 - writing
    if (muse_pipe(filedes) == -1) {
          perror("QtTimer - creating pipe failed");
          exit(-1);
          }
    muse_pipe_set_nonblock(filedes[1]);

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
    if (muse_pipe_read(readPipe, &nn, sizeof(char)) != sizeof(char)) {
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
      muse_pipe_write(writePipe,"t",1);
    }

  }

  long int InnerTimer::getTick()
  {
    return tickCount;
  }

} // namespace MusECore
