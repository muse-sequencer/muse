//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: alsatimer.h,v 1.4 2006/01/13 18:23:39 spamatica Exp $
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

class AlsaTimer : public Timer {
      snd_timer_t *handle;
      snd_timer_id_t *id;
      snd_timer_info_t *info;
      snd_timer_params_t *params;
      struct pollfd *fds;
      char timername[64];
      int count;
      int ticks;

    public:
      AlsaTimer();
      virtual ~AlsaTimer();

      virtual bool initTimer();
      virtual int  getTimerResolution();
      virtual bool setTimerFreq(unsigned int tick);
      virtual unsigned int getTimerFreq();
      virtual bool startTimer();
      virtual bool stopTimer();
      virtual unsigned long getTimerTicks();
      virtual int getFd() const { return fds->fd; }
      };

#endif //__ALSATIMER_H__
