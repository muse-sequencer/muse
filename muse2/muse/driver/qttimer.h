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

#ifndef __QTTIMER_H__
#define __QTTIMER_H__


#include <QThread>
#include <QBasicTimer>
#include <QTimerEvent>

#include "timerdev.h"

namespace MusECore {

//---------------------------------------------------------
//   QtTimer
//---------------------------------------------------------

class InnerTimer : public QObject {
  Q_OBJECT
  int writePipe;
  long int tickCount;
  QBasicTimer timer;
public:
   void setupTimer(int fd, int timeoutms);
   ~InnerTimer();
   long int getTick();
   bool isRunning() { return timer.isActive(); }

protected:
   void timerEvent(QTimerEvent *event);

};

class QtTimer : public BaseTimer, public QThread {
    
    int writePipe;
    int readPipe;
    bool keepRunning;
    InnerTimer *innerTimer;
    int timeoutms;
    public:
       QtTimer();
       virtual ~QtTimer();
       
       virtual signed int initTimer();
       virtual unsigned int setTimerResolution(unsigned int resolution);
       virtual unsigned int getTimerResolution();
       virtual unsigned int setTimerFreq(unsigned int freq);
       virtual unsigned int getTimerFreq();

       virtual bool startTimer();
       virtual bool stopTimer();
       virtual unsigned int getTimerTicks(bool printTicks=false);
       
       void setFindBestTimer(bool ) { }
    private:
       virtual void run();
};

} // namespace MusECore

#endif //__QTTIMER_H__
