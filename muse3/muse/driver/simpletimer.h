//=========================================================
//  MusE
//  Linux Music Editor
//
//  (C) Copyright 2015 Robert Jonsson (spamatica@gmail.com)
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

#ifndef __SIMPLETIMER_H__
#define __SIMPLETIMER_H__

#include "timerdev.h"

#include <QThread>

namespace MusECore {

//---------------------------------------------------------
//   AlsaTimer
//---------------------------------------------------------

class SimpleTimer : public Timer, public QThread {
    
    unsigned long tickCount;
    bool keepRunning;
    public:
       SimpleTimer();
       virtual ~SimpleTimer();
       
       virtual signed int initTimer(unsigned long init);
       virtual unsigned long setTimerResolution(unsigned long resolution);
       virtual unsigned long getTimerResolution();
       virtual unsigned long setTimerFreq(unsigned long freq);
       virtual unsigned long getTimerFreq();

       virtual bool startTimer();
       virtual bool stopTimer();
       virtual unsigned long getTimerTicks(bool printTicks=false);
       
       void setFindBestTimer(bool ) { }
    private:
       virtual void run();
};

} // namespace MusECore

#endif //__SIMPLETIMER_H__
