//=============================================================================
//  MusE
//  Linux Music Editor
//  $Id:$
//
//  (C) Copyright 2004 Robert Jonsson (rj@spamatica.se)
//  Copyright (C) 2002-2006 by Werner Schweer and others
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
