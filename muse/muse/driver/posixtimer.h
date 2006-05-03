//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: posixtimer.h,v 1.1 2005/11/14 20:05:36 wschweer Exp $
//
//  (C) Copyright 2005 Werner Schweer (werner@seh.de)
//=========================================================

#ifndef __POSIXTIMER_H__
#define __POSIXTIMER_H__

#include <time.h>
#include "timerdev.h"

//---------------------------------------------------------
//   PosixTimer
//---------------------------------------------------------

class PosixTimer : public Timer{
      timer_t timerId;
      struct itimerspec ts;
      clock_t clock;

    public:
      PosixTimer();
      virtual ~PosixTimer();

      virtual bool initTimer();
      virtual int  getTimerResolution();
      virtual bool setTimerTicks(int tick);

      virtual bool startTimer();
      virtual bool stopTimer();
      virtual unsigned long  getTimerTicks();
      virtual int getFd() const;
      };

#endif //__POSIXTIMER_H__

