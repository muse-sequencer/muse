//=============================================================================
//  MusE
//  Linux Music Editor
//  $Id:$
//
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

#ifndef __POSIXTIMER_H__
#define __POSIXTIMER_H__

#include <time.h>
#include "timerdev.h"

//---------------------------------------------------------
//   PosixTimer
//---------------------------------------------------------

class PosixTimer : public Timer{
      timer_t timerId;
      struct itimerspec ts;
      clock_t clock;

    public:
      PosixTimer();
      virtual ~PosixTimer();

      virtual bool initTimer();
      virtual int  getTimerResolution();
      virtual bool setTimerTicks(int tick);

      virtual bool startTimer();
      virtual bool stopTimer();
      virtual unsigned long  getTimerTicks();
      virtual int getFd() const;
      };

#endif //__POSIXTIMER_H__

