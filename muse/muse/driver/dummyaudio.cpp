//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: dummyaudio.cpp,v 1.3.2.16 2009/12/20 05:00:35 terminator356 Exp $
//  (C) Copyright 2002-2003 Werner Schweer (ws@seh.de)
//=========================================================

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <errno.h>
#include <stdarg.h>
#include <pthread.h>
#include <sys/poll.h>

#include "config.h"
#include "audio.h"
#include "audiodev.h"
#include "globals.h"
#include "song.h"
#include "driver/alsatimer.h"
#include "pos.h"
#include "gconfig.h"
#include "utils.h"

class MidiPlayEvent;

#define DEBUG_DUMMY 0
//---------------------------------------------------------
//   DummyAudioDevice
//---------------------------------------------------------

//static const unsigned dummyFrames = 1024;

enum Cmd {
trSeek,
trStart,
trStop
};

struct Msg {
  enum Cmd cmd;
  int arg;
};


class DummyAudioDevice : public AudioDevice {
      pthread_t dummyThread;
      // Changed by Tim. p3.3.15
      //float buffer[1024];
      float* buffer;
      int _realTimePriority;

   public:
      std::list<Msg> cmdQueue;
      Audio::State state;
      int _framePos;
      int playPos;
      bool realtimeFlag;
      
      DummyAudioDevice();
      virtual ~DummyAudioDevice()
      { 
        // Added by Tim. p3.3.15
        free(buffer); 
      }

      //virtual void start();
      virtual void start(int);
      
      virtual void stop ();
      virtual int framePos() const { 
            if(DEBUG_DUMMY)
                printf("DummyAudioDevice::framePos %d\n", _framePos);
            return _framePos; 
            }

      virtual float* getBuffer(void* /*port*/, unsigned long nframes)
            {
            // p3.3.30
            //if (nframes > dummyFrames) {
                  //printf("error: segment size > 1024\n");
            if (nframes > segmentSize) {
                  printf("DummyAudioDevice::getBuffer nframes > segment size\n");
                  
                  exit(-1);
                  }
            return buffer;
            }

      virtual std::list<QString> outputPorts();
      virtual std::list<QString> inputPorts();

      virtual void registerClient() {}

      virtual void* registerOutPort(const char*) {
            return (void*)1;
            }
      virtual void* registerInPort(const char*) {
            return (void*)2;
            }
      virtual void unregisterPort(void*) {}
      virtual void connect(void*, void*) {}
      virtual void disconnect(void*, void*) {}
      virtual void setPortName(void*, const char*) {}
      virtual void* findPort(const char*) { return 0;}
      virtual QString portName(void*) {
            return QString("mops");
            }
      virtual int getState() { 
//            if(DEBUG_DUMMY)
//                printf("DummyAudioDevice::getState %d\n", state);
      
      return state; }
      virtual unsigned getCurFrame() { 
            if(DEBUG_DUMMY)
                printf("DummyAudioDevice::getCurFrame %d\n", _framePos);
      
      return _framePos; }
      virtual unsigned frameTime() const {
            return lrint(curTime() * sampleRate);
            }
      virtual bool isRealtime() { return realtimeFlag; }
      //virtual int realtimePriority() const { return 40; }
      virtual int realtimePriority() const { return _realTimePriority; }
      virtual void startTransport() {
            if(DEBUG_DUMMY)
                printf("DummyAudioDevice::startTransport playPos=%d\n", playPos);
            Msg trcmd;
            trcmd.cmd = trStart;
            trcmd.arg = playPos;
            cmdQueue.push_front(trcmd);
/*            state = Audio::START_PLAY;
            audio->sync(state, playPos);            
            state = Audio::PLAY;*/
            }
      virtual void stopTransport() {
            if(DEBUG_DUMMY)
                printf("DummyAudioDevice::stopTransport, playPos=%d\n", playPos);
            state = Audio::STOP;
            }
      virtual int setMaster(bool) { return 1; }

      virtual void seekTransport(const Pos &p)
      {
            if(DEBUG_DUMMY)
                printf("DummyAudioDevice::seekTransport frame=%d topos=%d\n",playPos, p.frame());
            Msg trcmd;
            trcmd.cmd = trSeek;
            trcmd.arg = p.frame();
            cmdQueue.push_front(trcmd);
            playPos = p.frame();
      }
      virtual void seekTransport(unsigned pos) {
            if(DEBUG_DUMMY)
                printf("DummyAudioDevice::seekTransport frame=%d topos=%d\n",playPos,pos);
            Msg trcmd;
            trcmd.cmd = trSeek;
            trcmd.arg = pos;
            cmdQueue.push_front(trcmd);
            playPos = pos;
/*
            Audio::State tempState = state;
            state = Audio::START_PLAY;
            audio->sync(state, playPos);            
            state = tempState;*/
            }
      virtual void setFreewheel(bool) {}
      void setRealTime() { realtimeFlag = true; }
      };

DummyAudioDevice* dummyAudio = 0;

DummyAudioDevice::DummyAudioDevice() 
      {
      // Added by Tim. p3.3.15
      // p3.3.30
      //posix_memalign((void**)&buffer, 16, sizeof(float) * dummyFrames);
      posix_memalign((void**)&buffer, 16, sizeof(float) * config.dummyAudioBufSize);
      
      realtimeFlag = false;
      state = Audio::STOP;
      _framePos = 0;
      playPos = 0;
      cmdQueue.clear();
      }

//---------------------------------------------------------
//   exitDummyAudio
//---------------------------------------------------------

void exitDummyAudio()
{
      if(dummyAudio)
        delete dummyAudio;
      dummyAudio = NULL;      
      audioDevice = NULL;      
}

//---------------------------------------------------------
//   initDummyAudio
//---------------------------------------------------------

bool initDummyAudio()
      {
      dummyAudio = new DummyAudioDevice();
      audioDevice = dummyAudio;
      return false;
      }

//---------------------------------------------------------
//   outputPorts
//---------------------------------------------------------

std::list<QString> DummyAudioDevice::outputPorts()
      {
      std::list<QString> clientList;
      clientList.push_back(QString("output1"));
      clientList.push_back(QString("output2"));
      return clientList;
      }

//---------------------------------------------------------
//   inputPorts
//---------------------------------------------------------

std::list<QString> DummyAudioDevice::inputPorts()
      {
      std::list<QString> clientList;
      clientList.push_back(QString("input1"));
      clientList.push_back(QString("input2"));
      return clientList;
      }

//---------------------------------------------------------
//   dummyLoop
//---------------------------------------------------------

static void* dummyLoop(void* ptr)
      {
      //unsigned int tickRate = 25;
      
      // p3.3.30
      //sampleRate = 25600;
      sampleRate = config.dummyAudioSampleRate;
      //segmentSize = dummyFrames;
      segmentSize = config.dummyAudioBufSize;
      //unsigned int tickRate = sampleRate / dummyFrames;
      unsigned int tickRate = sampleRate / segmentSize;
      
      AlsaTimer timer;
      fprintf(stderr, "Finding best alsa timer for dummy driver:\n");
      timer.setFindBestTimer(false);
      int fd = timer.initTimer();
      
      /* Depending on nature of the timer, the requested tickRate might not
       * be available.  The return value is the nearest available frequency,
       * so use this to reset our dummpy sampleRate to keep everything 
       * consistent.
       */
      tickRate = timer.setTimerFreq( /*250*/ tickRate );
      
      // p3.3.31
      // If it didn't work, get the actual rate.
      if(tickRate == 0)
        tickRate = timer.getTimerFreq();
        
      sampleRate = tickRate * segmentSize;
      timer.startTimer();

      DummyAudioDevice *drvPtr = (DummyAudioDevice *)ptr;

      pollfd myPollFd;

      myPollFd.fd = fd;
      myPollFd.events = POLLIN;

      /*
      doSetuid();
      struct sched_param rt_param;
      int rv;
      memset(&rt_param, 0, sizeof(sched_param));
      int type;
      rv = pthread_getschedparam(pthread_self(), &type, &rt_param);
      if (rv != 0)
            perror("get scheduler parameter");
      if (type != SCHED_FIFO) {
            fprintf(stderr, "Driver thread not running SCHED_FIFO, trying to set...\n");

            memset(&rt_param, 0, sizeof(sched_param));
            //rt_param.sched_priority = 1;
            rt_param.sched_priority = realtimePriority();
            rv = pthread_setschedparam(pthread_self(), SCHED_FIFO, &rt_param);
            if (rv != 0)
                  perror("set realtime scheduler");
            memset(&rt_param, 0, sizeof(sched_param));
            rv = pthread_getschedparam(pthread_self(), &type, &rt_param);
            if (rv != 0)
                  perror("get scheduler parameter");
            if (type == SCHED_FIFO) {
                  drvPtr->setRealTime();
                  fprintf(stderr, "Thread succesfully set to SCHED_FIFO\n");
                  }
                  else {
                  fprintf(stderr, "Unable to set thread to SCHED_FIFO\n");
                  }
            }
      undoSetuid();
      */
      
#ifndef __APPLE__
      doSetuid();
      //if (realTimePriority) {
      if (realTimeScheduling) {
            //
            // check if we really got realtime priviledges
            //
            int policy;
            if ((policy = sched_getscheduler (0)) < 0) {
                printf("cannot get current client scheduler for audio dummy thread: %s!\n", strerror(errno));
                }
            else
                {
                if (policy != SCHED_FIFO)
                          printf("audio dummy thread _NOT_ running SCHED_FIFO\n");
                else if (debugMsg) {
                        struct sched_param rt_param;
                    memset(&rt_param, 0, sizeof(sched_param));
                        int type;
                    int rv = pthread_getschedparam(pthread_self(), &type, &rt_param);
                        if (rv == -1)
                                perror("get scheduler parameter");
                    printf("audio dummy thread running SCHED_FIFO priority %d\n",
                             rt_param.sched_priority);
                    }
                }
            }
      undoSetuid();
#endif
      
      /* unsigned long tick = 0;*/    // prevent compiler warning: unused variable
      for (;;) {
            int _pollWait = 10;   // ms
            unsigned long count = 0;
            while (count < 1 /*250/tickRate*/) // will loop until the next tick occurs
                {
                /*int n = */  poll(&myPollFd, 1 /* npfd */, _pollWait);
                count += timer.getTimerTicks();
                while (drvPtr->cmdQueue.size())
                    {
                    Msg &msg = drvPtr->cmdQueue.back();
                    drvPtr->cmdQueue.pop_back();
                    switch(msg.cmd) {
                          case trSeek:
                            {
                            //printf("trSeek\n");
                            drvPtr->playPos = msg.arg;
                            Audio::State tempState = drvPtr->state;
                            drvPtr->state = Audio::START_PLAY;
                            audio->sync(drvPtr->state, msg.arg);
                            drvPtr->state = tempState;
                            }
                            break;
                          case trStart:
                            {
                            //printf("trStart\n");
                            drvPtr->state = Audio::START_PLAY;
                            audio->sync(drvPtr->state, msg.arg);
                            drvPtr->state = Audio::PLAY;
                            }
                            break;
                          case trStop:
                            break;
                          default:
                            printf("dummyLoop: Unknown command!\n");
                        }
                    }
                }
            audio->process(segmentSize);
            int increment = segmentSize; // 1 //tickRate / sampleRate * segmentSize;
            drvPtr->_framePos+=increment;
            if (drvPtr->state == Audio::PLAY) 
                  {
                  drvPtr->playPos+=increment;
                  }
            }
      timer.stopTimer();
      pthread_exit(0);
      }

//void DummyAudioDevice::start()
void DummyAudioDevice::start(int priority)
      {
      //realTimePriority = priority;
      _realTimePriority = priority;
      pthread_attr_t* attributes = 0;

      //if (priority) {
      if (realTimeScheduling && priority > 0) {
            attributes = (pthread_attr_t*) malloc(sizeof(pthread_attr_t));
            pthread_attr_init(attributes);

            if (pthread_attr_setschedpolicy(attributes, SCHED_FIFO)) {
                  printf("cannot set FIFO scheduling class for RT thread\n");
                  }
            if (pthread_attr_setscope (attributes, PTHREAD_SCOPE_SYSTEM)) {
                  printf("Cannot set scheduling scope for RT thread\n");
                  }
            struct sched_param rt_param;
            memset(&rt_param, 0, sizeof(rt_param));
            rt_param.sched_priority = priority;
            if (pthread_attr_setschedparam (attributes, &rt_param)) {
                  printf("Cannot set scheduling priority %d for RT thread (%s)\n",
                     priority, strerror(errno));
                  }
            }
      
      //pthread_attr_t* attributes = (pthread_attr_t*) malloc(sizeof(pthread_attr_t));
      //pthread_attr_init(attributes);
      if (pthread_create(&dummyThread, attributes, ::dummyLoop, this))
            perror("creating thread failed:");
      if (priority)
        pthread_attr_destroy(attributes);
      }

void DummyAudioDevice::stop ()
      {
      pthread_cancel(dummyThread);
      pthread_join(dummyThread, 0);
      dummyThread = 0;
      }

