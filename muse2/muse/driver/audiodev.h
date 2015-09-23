//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: audiodev.h,v 1.5.2.2 2009/12/20 05:00:35 terminator356 Exp $
//
//  (C) Copyright 1999/2000 Werner Schweer (ws@seh.de)
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

#ifndef __AUDIODEV_H__
#define __AUDIODEV_H__

#include <list>
#include <sys/resource.h>
#include <sys/time.h>

class QString;

namespace MusECore {

class Pos;

//---------------------------------------------------------
//   AudioDevice
//---------------------------------------------------------

class AudioDevice {

   public:
      enum { DUMMY_AUDIO=0, JACK_AUDIO=1 };  // p3.3.52
      timeval lastCpuTime;
      timeval lastSysTime;
      float fAvrCpuLoad;
      int avrCpuLoadCounter;
      float fCurCpuLoad;
      AudioDevice()
      {
          gettimeofday(&lastSysTime, NULL);
          lastCpuTime.tv_sec = 0;
          lastCpuTime.tv_usec = 0;
          fAvrCpuLoad = 0.0f;
          avrCpuLoadCounter = 0;
          fCurCpuLoad = 0.0f;
      }
      virtual ~AudioDevice() {}

      virtual int deviceType() const = 0;  // p3.3.52
      
      //virtual void start() = 0;
      virtual void start(int priority) = 0;
      
      virtual void stop () = 0;
      virtual int framePos() const = 0;
      virtual unsigned frameTime() const = 0;
      virtual double systemTime() const = 0;

      virtual float* getBuffer(void* port, unsigned long nframes) = 0;

      virtual std::list<QString> outputPorts(bool midi = false, int aliases = -1) = 0;
      virtual std::list<QString> inputPorts(bool midi = false, int aliases = -1) = 0;

      virtual void registerClient() = 0;

      virtual const char* clientName() = 0;
      
      //virtual void* registerOutPort(const char* name) = 0;
      //virtual void* registerInPort(const char* name) = 0;
      virtual void* registerOutPort(const char* /*name*/, bool /*midi*/) = 0;
      virtual void* registerInPort(const char* /*name*/, bool /*midi*/) = 0;
      
      virtual void unregisterPort(void*) = 0;
      virtual void connect(void*, void*) = 0;
      virtual void connect(const char*, const char*) = 0;
      virtual void disconnect(void*, void*) = 0;
      virtual void disconnect(const char*, const char*) = 0;
      virtual int connections(void* /*clientPort*/) = 0; 
      virtual bool portConnectedTo(void* our_port, const char* port) = 0;
      virtual void setPortName(void* p, const char* n) = 0;
      virtual void* findPort(const char* name) = 0;
      virtual char* portName(void* port, char* str, int str_size) = 0;
      virtual const char* canonicalPortName(void*) = 0;
      virtual unsigned int portLatency(void* port, bool capture) const = 0;
      virtual int getState() = 0;
      virtual unsigned getCurFrame() const = 0;
      virtual bool isRealtime() = 0;
      virtual int realtimePriority() const = 0; // return zero if not realtime
      virtual void startTransport() = 0;
      virtual void stopTransport() = 0;
      virtual void seekTransport(unsigned frame) = 0;
      virtual void seekTransport(const Pos &p) = 0;
      virtual void setFreewheel(bool f) = 0;
      virtual void graphChanged() {}
      virtual void registrationChanged() {}
      virtual void connectionsChanged() {}
      virtual int setMaster(bool f) = 0;
      virtual float getCPULoad()
      {
          struct rusage ru;
          struct timeval curSysTime;
          gettimeofday(&curSysTime, NULL);
          //float fLoad = 0.0f;
          if(getrusage(RUSAGE_SELF, &ru) != 0)
          {
              return 0.0f;
          }
          long msSysElapsed = (curSysTime.tv_usec / 1000L) + curSysTime.tv_sec * 1000L;
          msSysElapsed -= (lastSysTime.tv_usec / 1000L) + lastSysTime.tv_sec * 1000L;
          long msCpuElasped = (ru.ru_utime.tv_usec / 1000L) + ru.ru_utime.tv_sec * 1000L;
          msCpuElasped -= (lastCpuTime.tv_usec / 1000L) + lastCpuTime.tv_sec * 1000L;
          if(msSysElapsed > 0)
          {
              fAvrCpuLoad += (float)((double)msCpuElasped / (double)msSysElapsed);
              avrCpuLoadCounter++;
          }
          lastCpuTime = ru.ru_utime;
          lastSysTime = curSysTime;
          if(avrCpuLoadCounter > 10)
          {
              fCurCpuLoad = (fAvrCpuLoad / (float)avrCpuLoadCounter) * 100.0f;
              fAvrCpuLoad = 0.0f;
              avrCpuLoadCounter = 0;
          }

          return fCurCpuLoad;
      }

      };

} // namespace MusECore

#endif

