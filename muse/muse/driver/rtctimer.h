//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: rtctimer.h,v 1.5 2006/01/13 18:23:39 spamatica Exp $
//
//  Most code moved from midiseq.cpp
//
//  (C) Copyright 2004 Robert Jonsson (rj@spamatica.se)
//  (C) Copyright 2004 Werner Schweer (werner@seh.de)
//=========================================================

#ifndef __RTCTIMER_H__
#define __RTCTIMER_H__

#include "timerdev.h"

//---------------------------------------------------------
//   RtcTimer
//---------------------------------------------------------

class RtcTimer : public Timer{
      int timerFd;

    public:
      RtcTimer();
      virtual ~RtcTimer();

      virtual bool initTimer();
      virtual int getTimerResolution();
      virtual bool setTimerFreq(unsigned int tick);
      virtual unsigned int getTimerFreq();

      virtual bool startTimer();
      virtual bool stopTimer();
      virtual unsigned long  getTimerTicks();
      virtual int getFd() const { return timerFd; }
      };

#endif //__ALSATIMER_H__
