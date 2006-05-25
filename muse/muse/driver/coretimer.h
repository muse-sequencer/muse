  //=========================================================
  //  MusE
  //  Linux Music Editor
  //  $Id: rtctimer.h,v 1.1.2.3 2005/08/21 18:11:28 spamatica Exp $
  //
  //  (C) Copyright 2004-2006 Robert Jonsson (rj@spamatica.se)
  //  (C) Copyright -2004 Werner Schweer (werner@seh.de)
  //=========================================================

#ifndef __CORETIMER_H__
#define __CORETIMER_H__

#include "timerdev.h"


//---------------------------------------------------------
//   AlsaTimer
//---------------------------------------------------------

class CoreTimer : public Timer {
    

    public:
       CoreTimer();
       virtual ~CoreTimer();
       
       virtual bool initTimer();
       virtual unsigned int setTimerResolution(unsigned int resolution);
       virtual int getTimerResolution();
       virtual bool setTimerFreq(unsigned int tick);
       virtual unsigned int getTimerFreq();

       virtual bool startTimer();
       virtual bool stopTimer();
       virtual unsigned long getTimerTicks();
      
    private:
      int timerFd; 

};

#endif //__CORETIMER_H__
