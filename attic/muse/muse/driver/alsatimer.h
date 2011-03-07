//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: alsatimer.h,v 1.1.2.4 2009/03/09 02:05:18 terminator356 Exp $
//
//  Plenty of code borrowed from timer.c example in 
//  alsalib 1.0.7
//
//  (C) Copyright 2004 Robert Jonsson (rj@spamatica.se)
//=========================================================

#ifndef __ALSATIMER_H__
#define __ALSATIMER_H__

#include "alsa/asoundlib.h"
#include "timerdev.h"


//---------------------------------------------------------
//   AlsaTimer
//---------------------------------------------------------

class AlsaTimer : public Timer{
    
    snd_timer_t *handle;
    snd_timer_id_t *id;
    snd_timer_info_t *info;
    snd_timer_params_t *params;
    struct pollfd *fds;
    char timername[64];
    signed int count;
    unsigned int ticks;
    bool findBest;

    public:
       AlsaTimer();
       virtual ~AlsaTimer();
       
       virtual signed int initTimer();
       virtual unsigned int setTimerResolution(unsigned int resolution);
       virtual unsigned int getTimerResolution();
       virtual unsigned int setTimerFreq(unsigned int freq);
       virtual unsigned int getTimerFreq();

       virtual bool startTimer();
       virtual bool stopTimer();
       virtual unsigned int getTimerTicks(bool printTicks=false);
       
       void setFindBestTimer(bool b) { findBest = b; }
};

#endif //__ALSATIMER_H__
