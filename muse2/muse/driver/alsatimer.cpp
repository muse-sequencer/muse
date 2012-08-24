//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: alsatimer.cpp,v 1.1.2.9 2009/03/28 01:46:10 terminator356 Exp $
//
//  Plenty of code borrowed from timer.c example in 
//  alsalib 1.0.7
//
//  (C) Copyright 2004 Robert Jonsson (rj@spamatica.se)
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
        
#include "alsatimer.h"
#include <climits>

#define TIMER_DEBUG 0

namespace MusECore {
  
  AlsaTimer::AlsaTimer()
     {
     if(TIMER_DEBUG)
       fprintf(stderr,"AlsaTimer::AlsaTimer(this=%p) called\n",this);
     handle = NULL;
     id = NULL;
     info = NULL;
     params = NULL;
     findBest = true;
     }
     
  AlsaTimer::~AlsaTimer()
    {
    if(TIMER_DEBUG)
       fprintf(stderr,"AlsaTimer::~AlsaTimer(this=%p) called\n",this);
    if (handle)
      snd_timer_close(handle);
    if (id) snd_timer_id_free(id);
    if (info) snd_timer_info_free(info);
    if (params) snd_timer_params_free(params);
    }
  
  signed int AlsaTimer::initTimer()
    {
    if(TIMER_DEBUG)
      printf("AlsaTimer::initTimer(this=%p)\n",this);
  
    int err;
    int devclass = SND_TIMER_CLASS_GLOBAL;
    int sclass = SND_TIMER_CLASS_NONE;
    int card = 0;
    int device = SND_TIMER_GLOBAL_SYSTEM;
    int subdevice = 0;
    int test_ids[] = { SND_TIMER_GLOBAL_SYSTEM
                     , SND_TIMER_GLOBAL_RTC
#ifdef SND_TIMER_GLOBAL_HPET
                     , SND_TIMER_GLOBAL_HPET
#endif
                     };
    int max_ids = sizeof(test_ids) / sizeof(int);
    long best_res = LONG_MAX;
    //int best_dev = -1; // SND_TIMER_GLOBAL_SYSTEM;
    int best_dev = SND_TIMER_GLOBAL_SYSTEM;          
    int i;

    if (id || info || params) {
      fprintf(stderr,"AlsaTimer::initTimer(): called on initialised timer!\n");
      return fds->fd;
    }  
    snd_timer_id_malloc(&id);
    snd_timer_info_malloc(&info);
    snd_timer_params_malloc(&params);

    if (findBest) {
      for (i = 0; i < max_ids; ++i) {
        device = test_ids[i];
        sprintf(timername, "hw:CLASS=%i,SCLASS=%i,CARD=%i,DEV=%i,SUBDEV=%i", devclass, sclass, card, device, subdevice);
        if ((err = snd_timer_open(&handle, timername, SND_TIMER_OPEN_NONBLOCK)) < 0) {
          continue;
          }
        if ((err = snd_timer_info(handle, info)) < 0) {
          snd_timer_close(handle);
          continue;
          }
        // select a non slave timer with the lowest resolution value
        int is_slave = snd_timer_info_is_slave(info);
        long res = snd_timer_info_get_resolution(info);
        if ((is_slave == 0) && (best_res > res)) {
          best_res = res;
          best_dev = device;
          }
        snd_timer_close(handle);
        }
      device = best_dev;
      }

    //if(best_dev==-1)
    //  return -1; // no working timer found

    sprintf(timername, "hw:CLASS=%i,SCLASS=%i,CARD=%i,DEV=%i,SUBDEV=%i", devclass, sclass, card, device, subdevice);
    if ((err = snd_timer_open(&handle, timername, SND_TIMER_OPEN_NONBLOCK))<0) {
      fprintf(stderr, "AlsaTimer::initTimer(): timer open %i (%s)\n", err, snd_strerror(err));
      return -1;  
      }
    
    if ((err = snd_timer_info(handle, info)) < 0) {
      fprintf(stderr, "AlsaTimer::initTimer(): timer info %i (%s)\n", err, snd_strerror(err));
      return -1;
      }

    //if(debugMsg)
      fprintf(stderr, "AlsaTimer::initTimer(): best available ALSA timer: %s\n", snd_timer_info_get_name(info));

    snd_timer_params_set_auto_start(params, 1);
    snd_timer_params_set_ticks(params, 1);
      
    if ((err = snd_timer_params(handle, params)) < 0) {
      fprintf(stderr, "AlsaTimer::initTimer(): timer params %i (%s)\n", err, snd_strerror(err));
      return -1;
      }
    
    count = snd_timer_poll_descriptors_count(handle);
    fds = (pollfd *)calloc(count, sizeof(pollfd));
    if (fds == NULL) {
      fprintf(stderr, "AlsaTimer::initTimer(): malloc error\n");
      return -1;
      }
    if ((err = snd_timer_poll_descriptors(handle, fds, count)) < 0) {
      fprintf(stderr, "AlsaTimer::initTimer(): snd_timer_poll_descriptors error: %s\n", snd_strerror(err));
      return -1;
      }
    return fds->fd;
    }
  
  unsigned int AlsaTimer::setTimerResolution(unsigned int resolution)
    {
    if(TIMER_DEBUG)
      printf("AlsaTimer::setTimerResolution(%d)\n",resolution);
    /* Resolution of an AlsaTimer is fixed - it cannot be set */
    return 0;
    }
  
  unsigned int AlsaTimer::setTimerFreq(unsigned int freq)
    {
    signed int err;
    unsigned int setTick, actFreq;
    
    if(TIMER_DEBUG)
      printf("AlsaTimer::setTimerFreq(this=%p)\n",this);

    setTick = (1000000000 / snd_timer_info_get_resolution(info)) / freq;

    if (setTick == 0) {
      // return, print error if freq is below 500 (timing will suffer)
      if (((1000000000.0 / snd_timer_info_get_resolution(info)) / snd_timer_params_get_ticks(params)) < 500) {
        fprintf(stderr,"AlsaTimer::setTimerTicks(): requested freq %u Hz too high for timer (max is %g)\n",
          freq, 1000000000.0 / snd_timer_info_get_resolution(info));
        fprintf(stderr,"  freq stays at %ld Hz\n",
          (long int)((1000000000.0 / snd_timer_info_get_resolution(info)) / snd_timer_params_get_ticks(params)));
      }

      return (long int)((1000000000.0 / snd_timer_info_get_resolution(info)) / snd_timer_params_get_ticks(params));
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
      return 0;
      }

    return actFreq;
    }
  
  unsigned int AlsaTimer::getTimerResolution()
    {
    return  snd_timer_info_get_resolution(info);
    }

  unsigned int AlsaTimer::getTimerFreq()
    {
    return (1000000000 / snd_timer_info_get_resolution(info)) / snd_timer_params_get_ticks(params);
    }
        
  bool AlsaTimer::startTimer()
    {
    if(TIMER_DEBUG)
      printf("AlsaTimer::startTimer(this=%p): handle=%p\n",this,handle);
    int err;
    if ((err = snd_timer_start(handle)) < 0) {
      fprintf(stderr, "AlsaTimer::startTimer(): timer start %i (%s)\n", err, snd_strerror(err));
      return false;
      }
    return true;
    }
  
  bool AlsaTimer::stopTimer()
    {
    int err;
    if(TIMER_DEBUG)
      printf("AlsaTimer::stopTimer(this=%p): handle=%p\n",this,handle);
    if ((err = snd_timer_stop(handle)) < 0) {
      fprintf(stderr, "AlsaTimer::stopTimer(): timer stop %i (%s)\n", err, snd_strerror(err));
      return false;
      }
    return true;
    }
        
  unsigned int  AlsaTimer::getTimerTicks(bool printTicks)
    {
    //if(TIMER_DEBUG)
    //  printf("AlsaTimer::getTimerTicks\n");
    snd_timer_read_t tr;
    tr.ticks = 0;
    while (snd_timer_read(handle, &tr, sizeof(tr)) == sizeof(tr)) {
              if (printTicks) {
                  printf("TIMER: resolution = %uns, ticks = %u\n",
                    tr.resolution, tr.ticks);
                  }
      }
    return tr.ticks;
    }

} // namespace MusECore
