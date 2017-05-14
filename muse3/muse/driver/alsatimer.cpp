//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: alsatimer.cpp,v 1.1.2.9 2009/03/28 01:46:10 terminator356 Exp $
//
//  Plenty of code borrowed from timer.c example in 
//  alsalib 1.0.7
//
//  (C) Copyright 2004 Robert Jonsson (rj@spamatica.se)
//  (C) Copyright 2016 Tim E. Real (terminator356 on users dot sourceforge dot net)
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

#ifdef ALSA_SUPPORT

#include <climits>
#include <stdio.h>

#define ALSA_TIMER_DEBUG 0

namespace MusECore {
  
  AlsaTimer::AlsaTimer()
     {
     if(TIMER_DEBUG || ALSA_TIMER_DEBUG)
       fprintf(stderr,"AlsaTimer::AlsaTimer(this=%p) called\n",this);
     handle = NULL;
     id = NULL;
     info = NULL;
     params = NULL;
     }
     
  AlsaTimer::~AlsaTimer()
    {
    if(TIMER_DEBUG || ALSA_TIMER_DEBUG)
       fprintf(stderr,"AlsaTimer::~AlsaTimer(this=%p) called\n",this);
    if (handle)
      snd_timer_close(handle);
    if (id) snd_timer_id_free(id);
    if (info) snd_timer_info_free(info);
    if (params) snd_timer_params_free(params);
    }

signed int AlsaTimer::initTimer()
{
  if(TIMER_DEBUG || ALSA_TIMER_DEBUG)
    fprintf(stderr, "AlsaTimer::initTimer(this=%p)\n",this);

  if(id || info || params)
  {
    fprintf(stderr, "AlsaTimer::initTimer(): called on initialised timer!\n");
    return fds->fd;
  }

  snd_timer_id_malloc(&id);
  snd_timer_id_set_class(id, SND_TIMER_CLASS_NONE);
  snd_timer_info_malloc(&info);
  snd_timer_params_malloc(&params);

  int best_dev = SND_TIMER_GLOBAL_SYSTEM;
  int best_devclass = SND_TIMER_CLASS_GLOBAL;
  int best_sclass = SND_TIMER_CLASS_NONE;
  int best_card = 0;
  int best_subdevice = 0;
  long best_res = LONG_MAX;
  int err;

  snd_timer_query_t *timer_query = NULL;
  if(snd_timer_query_open(&timer_query, "hw", 0) >= 0)
  {
    int is_slave;
    int device = SND_TIMER_GLOBAL_SYSTEM;
    int devclass = SND_TIMER_CLASS_GLOBAL;
    int sclass = SND_TIMER_CLASS_NONE;
    int card = 0;
    int subdevice = 0;

    while(snd_timer_query_next_device(timer_query, id) >= 0)
    {
      devclass = snd_timer_id_get_class(id);
      if(devclass < 0)
        break;
      sclass = snd_timer_id_get_sclass(id);
      if(sclass < 0)
        sclass = 0;
      card = snd_timer_id_get_card(id);
      if(card < 0)
        card = 0;
      device = snd_timer_id_get_device(id);
      if(device < 0)
        device = 0;
      subdevice = snd_timer_id_get_subdevice(id);
      if(subdevice < 0)
        subdevice = 0;
      snprintf(timername, sizeof(timername) - 1, "hw:CLASS=%i,SCLASS=%i,CARD=%i,DEV=%i,SUBDEV=%i", devclass, sclass, card, device, subdevice);
      if(snd_timer_open(&handle, timername, SND_TIMER_OPEN_NONBLOCK) >= 0)
      {
        if(snd_timer_info(handle, info) >= 0)
        {
          // Select a non slave timer with the lowest resolution value
          is_slave = snd_timer_info_is_slave(info);
          long res = snd_timer_info_get_resolution(info);
          if((is_slave == 0) && (best_res > res))
          {
            best_res = res;
            best_dev = device;
            best_devclass = devclass;
            best_sclass = sclass;
            best_card = card;
            best_subdevice = subdevice;
          }
        }
        snd_timer_close(handle);
      }
    }
    snd_timer_query_close(timer_query);
  }

  sprintf(timername, "hw:CLASS=%i,SCLASS=%i,CARD=%i,DEV=%i,SUBDEV=%i", best_devclass, best_sclass, best_card, best_dev, best_subdevice);
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

//   snd_timer_params_set_auto_start(params, 1);
//   
//   if(!snd_timer_info_is_slave(info))
//   {
//     const long int resolution = snd_timer_info_get_resolution(info);
//     fprintf(stderr, "   Average resolution:%li\n", resolution);
//     snd_timer_params_set_ticks(params, (1000000000L / resolution) / 10000000L); // 100Hz
//     if(snd_timer_params_get_ticks(params) < 1)
//       snd_timer_params_set_ticks(params, 1);
// //     fprintf(stderr, "   Using %li tick(s)\n", snd_timer_params_get_ticks(params));
//   }
//   else
//   {
//     snd_timer_params_set_ticks(params, 1);
//   }
//   
//   if ((err = snd_timer_params(handle, params)) < 0) 
//   {
//     snd_timer_params_set_ticks(params, 1);
//     
//     fprintf(stderr, "AlsaTimer::initTimer(): timer params %i (%s)\n"
//       " Unable to cȟange timer settings. Your system may need adjustment.\n"
//       " Timer frequency remains at %liHz\n", 
//       err, snd_strerror(err),
//       1000000000L / snd_timer_info_get_resolution(info) / snd_timer_params_get_ticks(params));
//     //fprintf(stderr, "   Timer frequency remains at:%liHz\n", 
//     //  1000000000L / snd_timer_info_get_resolution(info) / snd_timer_params_get_ticks(params));
//     // REMOVE Tim. autoconnect. Removed. On openSuse you must jump through some hoops
//     //  before you can access the HPET and RTC timers. And their default freq is a lousy 64Hz !
//     // Allow it to fall through and at least run.
// //     return -1;
//   }
// 
//   fprintf(stderr, "AlsaTimer::initTimer() Using %li tick(s)\n", snd_timer_params_get_ticks(params));
  
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
    if(TIMER_DEBUG || ALSA_TIMER_DEBUG)
      fprintf(stderr, "AlsaTimer::setTimerResolution(%d)\n",resolution);
    /* Resolution of an AlsaTimer is fixed - it cannot be set */
    return 0;
    }
  
unsigned int AlsaTimer::setTimerFreq(unsigned int freq)
{
  signed int err;

  if(TIMER_DEBUG || ALSA_TIMER_DEBUG)
    fprintf(stderr, "AlsaTimer::setTimerFreq(this=%p)\n",this);

//   const long int ticks = snd_timer_params_get_ticks(params);
  const long int res = snd_timer_info_get_resolution(info);
  const long int adj_res = 1000000000L / res;
  const long int setTick = adj_res / freq;
//   const long int cur_freq = adj_res / ticks;

  if(TIMER_DEBUG || ALSA_TIMER_DEBUG)
//     fprintf(stderr, "AlsaTimer::setTimerFreq res:%ld ticks:%ld\n", res, ticks);
    fprintf(stderr, "AlsaTimer::setTimerFreq res:%ld adj_res:%ld setTick:%ld\n", res, adj_res, setTick);

//   if(setTick == 0)
//   {
//     // Return, print error if freq is below 500 (timing will suffer).
//     if(cur_freq < 500)
//     {
//       fprintf(stderr,"AlsaTimer::setTimerFreq(): requested freq %u Hz too high for timer (max is %ld)\n", freq, adj_res);
//       fprintf(stderr,"  freq stays at %ld Hz\n", cur_freq);
//     }
//     return cur_freq;
//   }

  const long int actFreq = adj_res / setTick;
  if(actFreq != freq)
  {
    fprintf(stderr,"AlsaTimer::setTimerFreq(): warning: requested %u Hz, actual freq is %ld Hz\n", freq, actFreq);
  }
  if(TIMER_DEBUG || ALSA_TIMER_DEBUG)
    fprintf(stderr, "AlsaTimer::setTimerFreq(): Setting ticks (period) to %ld ticks\n", setTick);
  snd_timer_params_set_auto_start(params, 1);
  if(!snd_timer_info_is_slave(info))
  {
    snd_timer_params_set_ticks(params, setTick);
    if(snd_timer_params_get_ticks(params) < 1)
      snd_timer_params_set_ticks(params, 1);
//     if ((err = snd_timer_params(handle, params)) < 0)
//     {
//         fprintf(stderr, "AlsaTimer::setTimerFreq(): timer params %i (%s)\n", err, snd_strerror(err));
//         fprintf(stderr, "MusE Error: Unable to cȟange timer settings. Your system may need adjustment.\n");
//         fprintf(stderr, "   Timer frequency remains at:%liHz\n", 1000000000L / snd_timer_info_get_resolution(info));
//         // REMOVE Tim. autoconnect. Changed.
//     //     return 0;
//         return 1000000000L / snd_timer_info_get_resolution(info);
//     }
    
    //const long int resolution = snd_timer_info_get_resolution(info);
    //fprintf(stderr, "   Average resolution:%li\n", resolution);
    //snd_timer_params_set_ticks(params, (1000000000L / resolution) / 10000000L); // 100Hz
    //if(snd_timer_params_get_ticks(params) < 1)
    //  snd_timer_params_set_ticks(params, 1);
    //fprintf(stderr, "   Using %li tick(s)\n", snd_timer_params_get_ticks(params));
    
  }
  else
    snd_timer_params_set_ticks(params, 1);

  if((err = snd_timer_params(handle, params)) < 0)
  {
    snd_timer_params_set_ticks(params, 1);
      
    fprintf(stderr, "AlsaTimer::setTimerFreq(%u): timer params %i (%s)\n"
      " Unable to cȟange timer settings. Your system may need adjustment.\n"
      " Timer frequency remains at %liHz\n", 
      freq, err, snd_strerror(err),
      1000000000L / snd_timer_info_get_resolution(info) / snd_timer_params_get_ticks(params));
      // REMOVE Tim. autoconnect. Changed.
//       return 0;
      //return 1000000000L / snd_timer_info_get_resolution(info);
  }
    
  const long int ticks = snd_timer_params_get_ticks(params);
  const long int cur_freq = adj_res / ticks;
  if(setTick <= 0)
  {
    // Return, print error if freq is below 500 (timing will suffer).
    if(cur_freq < 500)
    {
      fprintf(stderr,"AlsaTimer::setTimerFreq(): requested freq %u Hz too high for timer (max is %ld)\n", freq, adj_res);
      fprintf(stderr,"  freq stays at %ld Hz\n", cur_freq);
    }
    //return cur_freq;
  }

  fprintf(stderr, "AlsaTimer::setTimerFreq(%u): Using %li tick(s)\n", freq, ticks);
  
  // REMOVE Tim. autoconnect. Changed.
//   return actFreq;
  // Return the actual frequency.
  //return 1000000000L / snd_timer_info_get_resolution(info) / snd_timer_params_get_ticks(params);
  //return adj_res / ticks;
  return cur_freq;
}
  
  unsigned int AlsaTimer::getTimerResolution()
    {
    return  snd_timer_info_get_resolution(info);
    }

  unsigned int AlsaTimer::getTimerFreq()
    {
    return (1000000000L / snd_timer_info_get_resolution(info)) / snd_timer_params_get_ticks(params);
    }
        
  bool AlsaTimer::startTimer()
    {
    if(TIMER_DEBUG || ALSA_TIMER_DEBUG)
      fprintf(stderr, "AlsaTimer::startTimer(this=%p): handle=%p\n",this,handle);
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
    if(TIMER_DEBUG || ALSA_TIMER_DEBUG)
      fprintf(stderr, "AlsaTimer::stopTimer(this=%p): handle=%p\n",this,handle);
    if ((err = snd_timer_stop(handle)) < 0) {
      fprintf(stderr, "AlsaTimer::stopTimer(): timer stop %i (%s)\n", err, snd_strerror(err));
      return false;
      }
    return true;
    }
        
  unsigned int  AlsaTimer::getTimerTicks(bool printTicks)
    {
    //if(TIMER_DEBUG || ALSA_TIMER_DEBUG)
    //  printf("AlsaTimer::getTimerTicks\n");
    snd_timer_read_t tr;
    tr.ticks = 0;
    while (snd_timer_read(handle, &tr, sizeof(tr)) == sizeof(tr)) {
              if (printTicks) {
                  fprintf(stderr, "TIMER: resolution = %uns, ticks = %u\n",
                    tr.resolution, tr.ticks);
                  }
      }
    return tr.ticks;
    }

} // namespace MusECore

#endif // ALSA_SUPPORT
