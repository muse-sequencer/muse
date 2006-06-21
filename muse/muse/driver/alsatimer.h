//=============================================================================
//  MusE
//  Linux Music Editor
//  $Id:$
//
//  Plenty of code borrowed from timer.c example in
//  alsalib 1.0.7
//
//  (C) Copyright 2004 Robert Jonsson (rj@spamatica.se)
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License version 2.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//=============================================================================

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
