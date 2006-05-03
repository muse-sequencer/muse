//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: timerdev.h,v 1.6 2006/01/13 18:23:39 spamatica Exp $
//
//  Plenty of code borrowed from timer.c example in
//  alsalib 1.0.7
//
//  (C) Copyright 2004 Robert Jonsson (rj@spamatica.se)
//=========================================================

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
