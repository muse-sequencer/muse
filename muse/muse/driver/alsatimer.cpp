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

#include "alsatimer.h"


AlsaTimer::AlsaTimer()
      {
      handle = 0;
      id     = 0;
      info   = 0;
      params = 0;
      }

//---------------------------------------------------------
//   ~AlsaTimer
//---------------------------------------------------------

AlsaTimer::~AlsaTimer()
      {
      if (handle)
            snd_timer_close(handle);
      if (id)
            free(id);
      if (info)
            free(info);
      if (params)
            free(params);
      }

//---------------------------------------------------------
//   initTimer
//---------------------------------------------------------

bool AlsaTimer::initTimer()
      {
      int err;
      int devclass = SND_TIMER_CLASS_GLOBAL;
      int sclass = SND_TIMER_CLASS_NONE;
      int card = 0;
      int device = SND_TIMER_GLOBAL_SYSTEM;
      int subdevice = 0;

      if (id || info || params)
            return true;

      snd_timer_id_malloc(&id);
      snd_timer_info_malloc(&info);
      snd_timer_params_malloc(&params);


      sprintf(timername, "hw:CLASS=%i,SCLASS=%i,CARD=%i,DEV=%i,SUBDEV=%i", devclass, sclass, card, device, subdevice);
      if ((err = snd_timer_open(&handle, timername, SND_TIMER_OPEN_NONBLOCK))<0) {
            fprintf(stderr, "AlsaTimer::initTimer(): timer open %i (%s)\n", err, snd_strerror(err));
            }

      if ((err = snd_timer_info(handle, info)) < 0) {
            fprintf(stderr, "AlsaTimer::initTimer(): timer info %i (%s)\n", err, snd_strerror(err));
            return false;
            }

      snd_timer_params_set_auto_start(params, 1);
      snd_timer_params_set_ticks(params, 1);

      if ((err = snd_timer_params(handle, params)) < 0) {
            fprintf(stderr, "AlsaTimer::initTimer(): timer params %i (%s)\n", err, snd_strerror(err));
            return false;
            }

      count = snd_timer_poll_descriptors_count(handle);
      fds = (pollfd *)calloc(count, sizeof(pollfd));
      if (fds == 0) {
            fprintf(stderr, "mAlsaTimer::initTimer(): alloc error\n");
            return false;
            }
      if ((err = snd_timer_poll_descriptors(handle, fds, count)) < 0) {
            fprintf(stderr, "AlsaTimer::initTimer(): snd_timer_poll_descriptors error: %s\n", snd_strerror(err));
            return false;
            }
      return true;
      }


//---------------------------------------------------------
//   setTimerTicks
//---------------------------------------------------------

bool AlsaTimer::setTimerFreq(unsigned int freq)
    {
    signed int err;
    unsigned int setTick, actFreq;

    if(TIMER_DEBUG)
      printf("AlsaTimer::setTimerFreq(this=%p)\n",this);

    setTick = (1000000000 / snd_timer_info_get_resolution(info)) / freq;

    if (setTick == 0) {
      fprintf(stderr,"AlsaTimer::setTimerTicks(): requested freq %u Hz too high for timer (max is %g)\n",
        freq, 1000000000.0 / snd_timer_info_get_resolution(info));
      fprintf(stderr,"  freq stays at %ld Hz\n",(1000000000 / snd_timer_info_get_resolution(info)) / snd_timer_params_get_ticks(params));

      return true;
    }
    actFreq = (1000000000 / snd_timer_info_get_resolution(info)) / setTick;
    if (actFreq != freq) {
      fprintf(stderr,"AlsaTimer::setTimerTicks(): warning: requested %u Hz, actual freq is %u Hz\n",
        freq, actFreq);
    }
    if(TIMER_DEBUG)
      printf("AlsaTimer::setTimerFreq(): Setting ticks (period) to %d ticks\n", setTick);
    snd_timer_params_set_auto_start(params, 1);
    snd_timer_params_set_ticks(params, setTick);
    if ((err = snd_timer_params(handle, params)) < 0) {
      fprintf(stderr, "AlsaTimer::setTimerFreq(): timer params %i (%s)\n", err, snd_strerror(err));
      return false;
      }

    return true;
    }

//---------------------------------------------------------
//   getTimerResolution
//---------------------------------------------------------

int  AlsaTimer::getTimerResolution()
      {
      return  snd_timer_info_get_resolution(info);
      }

//---------------------------------------------------------
//   getTimerFreq
//---------------------------------------------------------

unsigned int AlsaTimer::getTimerFreq()
    {
    return (1000000000 / snd_timer_info_get_resolution(info)) / snd_timer_params_get_ticks(params);
    }

//---------------------------------------------------------
//   startTimer
//---------------------------------------------------------

bool AlsaTimer::startTimer()
      {
      int err;
      if ((err = snd_timer_start(handle)) < 0) {
            fprintf(stderr, "AlsaTimer::startTimer(): timer start %i (%s)\n", err, snd_strerror(err));
            return false;
            }
      return true;
      }

//---------------------------------------------------------
//   stopTimer
//---------------------------------------------------------

bool AlsaTimer::stopTimer()
    {
      int err;
      if ((err = snd_timer_stop(handle)) < 0) {
            fprintf(stderr, "AlsaTimer::startTimer(): timer stop %i (%s)\n", err, snd_strerror(err));
            return false;
            }
      snd_timer_close(handle);
      return true;
      }

//---------------------------------------------------------
//   getTimerTicks
//---------------------------------------------------------

unsigned long AlsaTimer::getTimerTicks()
      {
      snd_timer_read_t tr;
      if (snd_timer_read(handle, &tr, sizeof(tr)) == sizeof(tr)) {
            return tr.ticks; //(tr.ticks - 1) * 1024;  //??
            }
      return 0;
      }
