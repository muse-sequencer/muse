  //=========================================================
  //  MusE
  //  Linux Music Editor
  //  $Id: rtctimer.h,v 1.1.2.3 2005/08/21 18:11:28 spamatica Exp $
  //
  //  Most code moved from midiseq.cpp
  //
  //  (C) Copyright 2004 Robert Jonsson (rj@spamatica.se)
  //  (C) Copyright -2004 Werner Schweer (werner@seh.de)
  //=========================================================

#ifndef __RTCTIMER_H__
#define __RTCTIMER_H__

#include "timerdev.h"


//---------------------------------------------------------
//   AlsaTimer
//---------------------------------------------------------

class RtcTimer : public Timer{
    

    public:
       RtcTimer();
       virtual ~RtcTimer();
       
       virtual signed int initTimer();
       virtual unsigned int setTimerResolution(unsigned int resolution);
       virtual unsigned int getTimerResolution();
       virtual unsigned int setTimerFreq(unsigned int tick);
       virtual unsigned int getTimerFreq();

       virtual bool startTimer();
       virtual bool stopTimer();
       virtual unsigned int getTimerTicks(bool printTicks=false);
      
    private:
      int timerFd; 

};

#endif //__ALSATIMER_H__
