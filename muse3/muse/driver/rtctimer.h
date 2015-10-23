//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: rtctimer.h,v 1.1.2.3 2005/08/21 18:11:28 spamatica Exp $
//
//  Most code moved from midiseq.cpp
//
//  (C) Copyright -2004 Werner Schweer (werner@seh.de)
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

#ifndef __RTCTIMER_H__
#define __RTCTIMER_H__

#include "timerdev.h"

namespace MusECore {

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

} // namespace MusECore

#endif //__RTCTIMER_H__
