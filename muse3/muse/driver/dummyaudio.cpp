//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: dummyaudio.cpp,v 1.3.2.16 2009/12/20 05:00:35 terminator356 Exp $
//  (C) Copyright 2002-2003 Werner Schweer (ws@seh.de)
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

#include <stdio.h>
#include <stdlib.h>
#include "muse_math.h"
#include <errno.h>
#include <stdarg.h>
#include <pthread.h>
#ifndef _WIN32
#include <sys/poll.h>
#endif
#include <sys/time.h>
#include <unistd.h>

#include "config.h"
#include "audio.h"
#include "audiodev.h"
#include "globals.h"
#include "song.h"
// #include "driver/alsatimer.h"
#include "pos.h"
#include "gconfig.h"
#include "large_int.h"
#include "al/al.h"

// For debugging output: Uncomment the fprintf section.
#define DEBUG_DUMMY(dev, format, args...) // fprintf(dev, format, ##args);

namespace MusECore {

//---------------------------------------------------------
//   DummyAudioDevice
//---------------------------------------------------------

class DummyAudioDevice : public AudioDevice {
   private:
      pthread_t dummyThread;
      float* buffer;
      int _realTimePriority;

      // Critical variables that need to all update at once.
      // We employ a 'flipping' technique.
      unsigned _framesAtCycleStart[2];
      uint64_t _timeUSAtCycleStart[2];
      unsigned _frameCounter[2];
      unsigned _criticalVariablesIdx;
      
   public:
      // Time in microseconds at which the driver was created.
      uint64_t _start_timeUS;

      virtual const char* driverName() const { return "DummyAudioDevice"; }

      // For callback usage only.
      void setCriticalVariables(unsigned segmentSize)
      {
        static bool _firstTime = true;
        const unsigned idx = (_criticalVariablesIdx + 1) % 2;
        _timeUSAtCycleStart[idx] = systemTimeUS();
        // Let these start at zero and only increment on subsequent callbacks.
        if(!_firstTime)
        {
          _framesAtCycleStart[idx] = _framesAtCycleStart[_criticalVariablesIdx] + segmentSize;
          _frameCounter[idx] = _frameCounter[_criticalVariablesIdx] + segmentSize;
        }
        _firstTime = false;
        // Now 'flip' the variables all at once.
        _criticalVariablesIdx = idx;
      }

      DummyAudioDevice();
      virtual ~DummyAudioDevice()
      { 
        free(buffer); 
      }

      virtual inline int deviceType() const { return DUMMY_AUDIO; }
      
      // Returns true on success.
      virtual bool start(int);
      
      virtual void stop ();
      virtual unsigned framePos() const {
            // Not much choice but to do this:
            const unsigned int facs = framesAtCycleStart();
            const unsigned int fscs = framesSinceCycleStart();
            DEBUG_DUMMY(stderr, "DummyAudioDevice::framePos framesAtCycleStart:%u framesSinceCycleStart:%u\n", facs, fscs);
            return facs + fscs; 
            }

      // These are meant to be called from inside process thread only.      
      virtual unsigned framesAtCycleStart() const { return _framesAtCycleStart[_criticalVariablesIdx]; }
      virtual unsigned framesSinceCycleStart() const 
      { 
        const uint64_t ct = systemTimeUS();
        DEBUG_DUMMY(stderr, "DummyAudioDevice::framesSinceCycleStart systemTimeUS:%lu timeUSAtCycleStart:%lu\n", 
                ct, _timeUSAtCycleStart[_criticalVariablesIdx]);
        // Do not round up here since time resolution is higher than (audio) frame resolution.
        unsigned f = muse_multiply_64_div_64_to_64(ct - _timeUSAtCycleStart[_criticalVariablesIdx], MusEGlobal::sampleRate, 1000000UL);
        
        // Safety due to inaccuracies. It cannot be after the segment, right?
        if(f >= MusEGlobal::segmentSize)
          f = MusEGlobal::segmentSize - 1;
        return f;
      }

      virtual float* getBuffer(void* /*port*/, unsigned long nframes)
            {
            if (nframes > MusEGlobal::segmentSize) {
                  fprintf(stderr, "DummyAudioDevice::getBuffer nframes > segment size\n");
                  
                  exit(-1);
                  }
            return buffer;
            }

      virtual std::list<QString> outputPorts(bool midi = false, int aliases = -1);
      virtual std::list<QString> inputPorts(bool midi = false, int aliases = -1);

      virtual void registerClient() {}

      virtual const char* clientName() { return "MusE"; }
      
      virtual void* registerOutPort(const char*, bool) {
            return (void*)1;
            }
      virtual void* registerInPort(const char*, bool) {
            return (void*)2;
            }
// REMOVE Tim. clip. Removed.
//       float getDSP_Load()
//       {
//         return 0.0f;
//       }
      virtual AudioDevice::PortType portType(void*) const { return UnknownType; }
      virtual AudioDevice::PortDirection portDirection(void*) const { return UnknownDirection; }
      virtual void unregisterPort(void*) {}
      virtual bool connect(void* /*src*/, void* /*dst*/) { return false; }
      virtual bool connect(const char* /*src*/, const char* /*dst*/) { return false; }
      virtual bool disconnect(void* /*src*/, void* /*dst*/) { return false; }
      virtual bool disconnect(const char* /*src*/, const char* /*dst*/) { return false; }
      virtual int connections(void* /*clientPort*/) { return 0; }
      virtual bool portConnectedTo(void*, const char*) { return false; }
      virtual bool portsCanDisconnect(void* /*src*/, void* /*dst*/) const { return false; };
      virtual bool portsCanDisconnect(const char* /*src*/, const char* /*dst*/) const { return false; }
      virtual bool portsCanConnect(void* /*src*/, void* /*dst*/) const { return false; }
      virtual bool portsCanConnect(const char* /*src*/, const char* /*dst*/) const { return false; }
      virtual bool portsCompatible(void* /*src*/, void* /*dst*/) const { return false; }
      virtual bool portsCompatible(const char* /*src*/, const char* /*dst*/) const { return false; }
      virtual void setPortName(void*, const char*) {}
      virtual void* findPort(const char*) { return 0;}
      // preferred_name_or_alias: -1: No preference 0: Prefer canonical name 1: Prefer 1st alias 2: Prefer 2nd alias.
      virtual char*  portName(void*, char* str, int str_size, int /*preferred_name_or_alias*/ = -1) { if(str_size == 0) return 0; str[0] = '\0'; return str; }
      virtual const char* canonicalPortName(void*) { return 0; }
      virtual unsigned int portLatency(void* /*port*/, bool /*capture*/) const { return 0; }
      virtual unsigned frameTime() const {
            return _frameCounter[_criticalVariablesIdx];
            }

      virtual bool isRealtime() { return MusEGlobal::realTimeScheduling; }
      //virtual int realtimePriority() const { return 40; }
      virtual int realtimePriority() const { return _realTimePriority; }

      virtual void setFreewheel(bool) {}
      virtual int setMaster(bool, bool /*unconditional*/ = false) { return 1; }
      };

DummyAudioDevice* dummyAudio = 0;

DummyAudioDevice::DummyAudioDevice() : AudioDevice()
      {
      MusEGlobal::sampleRate = MusEGlobal::config.deviceAudioSampleRate;
      // Make sure the AL namespace variables mirror our variables.
      AL::sampleRate = MusEGlobal::sampleRate;
      MusEGlobal::segmentSize = MusEGlobal::config.deviceAudioBufSize;
#ifdef _WIN32
  buffer = (float *) _aligned_malloc(16, sizeof(float) * MusEGlobal::segmentSize);
  if(buffer == NULL)
  {
      fprintf(stderr, "ERROR: DummyAudioDevice ctor: _aligned_malloc returned error: NULL. Aborting!\n");
      abort();
  }
#else
      int rv = posix_memalign((void**)&buffer, 16, sizeof(float) * MusEGlobal::segmentSize);
      if(rv != 0)
      {
        fprintf(stderr, "ERROR: DummyAudioDevice ctor: posix_memalign returned error:%d. Aborting!\n", rv);
        abort();
      }
#endif
      if(MusEGlobal::config.useDenormalBias)
      {
        for(unsigned q = 0; q < MusEGlobal::segmentSize; ++q)
          buffer[q] = MusEGlobal::denormalBias;
      }
      else
        memset(buffer, 0, sizeof(float) * MusEGlobal::segmentSize);

      dummyThread = 0;
      _start_timeUS = systemTimeUS();
      _criticalVariablesIdx = 0;
      for(unsigned x = 0; x < 2; ++x)
      {
        _timeUSAtCycleStart[x] = 0;
        _framesAtCycleStart[x] = 0;
        _frameCounter[x] = 0;
      }
      }


//---------------------------------------------------------
//   exitDummyAudio
//---------------------------------------------------------

void exitDummyAudio()
{
      if(dummyAudio)
        delete dummyAudio;
      dummyAudio = NULL;      
      MusEGlobal::audioDevice = NULL;      
}


//---------------------------------------------------------
//   initDummyAudio
//---------------------------------------------------------

bool initDummyAudio()
      {
      dummyAudio = new DummyAudioDevice();
      MusEGlobal::audioDevice = dummyAudio;
      return false;
      }

//---------------------------------------------------------
//   outputPorts
//---------------------------------------------------------

std::list<QString> DummyAudioDevice::outputPorts(bool midi, int /*aliases*/)
      {
      std::list<QString> clientList;
      if(!midi)
      {
        clientList.push_back(QString("output1"));
        clientList.push_back(QString("output2"));
      }  
      return clientList;
      }

//---------------------------------------------------------
//   inputPorts
//---------------------------------------------------------

std::list<QString> DummyAudioDevice::inputPorts(bool midi, int /*aliases*/)
      {
      std::list<QString> clientList;
      if(!midi)
      {
        clientList.push_back(QString("input1"));
        clientList.push_back(QString("input2"));
      }  
      return clientList;
      }

//---------------------------------------------------------
//   dummyLoop
//---------------------------------------------------------

static void* dummyLoop(void* ptr)
      {

      DummyAudioDevice *drvPtr = (DummyAudioDevice *)ptr;
      
      for(;;) 
      {
        drvPtr->setCriticalVariables(MusEGlobal::segmentSize);
  
        if(MusEGlobal::audio->isRunning()) {
          // Use our built-in transport, which INCLUDES the necessary
          //  calls to Audio::sync() and ultimately Audio::process(),
          //  and increments the built-in play position.
          drvPtr->processTransport(MusEGlobal::segmentSize);
        }

        usleep(MusEGlobal::segmentSize*1000000/MusEGlobal::sampleRate);
      }
      pthread_exit(0);
      }

//---------------------------------------------------------
//   dummyLoop
//   Returns true on success.
//---------------------------------------------------------

bool DummyAudioDevice::start(int priority)
{
      _realTimePriority = priority;
      pthread_attr_t* attributes = 0;

      if (MusEGlobal::realTimeScheduling && _realTimePriority > 0) {
            attributes = (pthread_attr_t*) malloc(sizeof(pthread_attr_t));
            pthread_attr_init(attributes);

            if (pthread_attr_setschedpolicy(attributes, SCHED_FIFO)) {
                  fprintf(stderr, "cannot set FIFO scheduling class for dummy RT thread\n");
                  }
            if (pthread_attr_setscope (attributes, PTHREAD_SCOPE_SYSTEM)) {
                  fprintf(stderr, "Cannot set scheduling scope for dummy RT thread\n");
                  }
            // p4.0.16 Dummy was not running FIFO because this is needed.
            if (pthread_attr_setinheritsched(attributes, PTHREAD_EXPLICIT_SCHED)) {
                  fprintf(stderr, "Cannot set setinheritsched for dummy RT thread\n");
                  }
                  
            struct sched_param rt_param;
            memset(&rt_param, 0, sizeof(rt_param));
            rt_param.sched_priority = priority;
            if (pthread_attr_setschedparam (attributes, &rt_param)) {
                  fprintf(stderr, "Cannot set scheduling priority %d for dummy RT thread (%s)\n",
                     priority, strerror(errno));
                  }
            }
      
      int rv = pthread_create(&dummyThread, attributes, dummyLoop, this); 
      if(rv)
      {  
        if (MusEGlobal::realTimeScheduling && _realTimePriority > 0) 
          rv = pthread_create(&dummyThread, NULL, dummyLoop, this); 
      }
      
      if(rv)
          fprintf(stderr, "creating dummy audio thread failed: %s\n", strerror(rv));

      if (attributes)
      {
        pthread_attr_destroy(attributes);
        free(attributes);
      }
      
      return true;
}

void DummyAudioDevice::stop ()
      {
      pthread_cancel(dummyThread);
      pthread_join(dummyThread, 0);
      dummyThread = 0;
      }


} // namespace MusECore
