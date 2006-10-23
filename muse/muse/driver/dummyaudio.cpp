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

#include "al/al.h"
#include "widgets/utils.h"
#include "audio.h"
#include "audiodev.h"
#include "globals.h"
#include "song.h"

static const unsigned dummyFrames = 1024;
static double startTime;

//---------------------------------------------------------
//   DummyAudio
//---------------------------------------------------------

class DummyAudio : public AudioDriver {
      pthread_t dummyThread;
      float buffer[dummyFrames];
      std::vector<QString> oPorts;
      std::vector<QString> iPorts;
      int realTimePriority;

   public:
      int state;
      bool seekflag;
      unsigned pos;

      DummyAudio() {
            state = Audio::STOP;
            seekflag = false;
            startTime = curTime();
            }
      virtual ~DummyAudio() {}

      virtual bool init()   { return true; }
      virtual void start(int);
      virtual void stop ();
      virtual unsigned framePos() const {
            return lrint((curTime()-startTime) * AL::sampleRate);
            }

      virtual float* getBuffer(void* /*port*/, unsigned long nframes)
            {
            if (nframes > dummyFrames) {
                  fprintf(stderr, "error: segment size > %d\n", dummyFrames);
                  exit(-1);
                  }
            memset(buffer, 0, nframes * sizeof(float));
            return buffer;
            }

      virtual QList<PortName> outputPorts(bool midi = false);
      virtual QList<PortName> inputPorts(bool midi = false);

      virtual void registerClient() {}

      virtual void* registerOutPort(const QString& s, bool) {
            iPorts.push_back(QString(s));
            return (void*)(iPorts.size() + 3000);
            }
      virtual void* registerInPort(const QString& s, bool) {
            oPorts.push_back(QString(s));
            return (void*)(oPorts.size() + 4000);
            }
      virtual void unregisterPort(void*) {
/*            if (long(p) >= 100)
                  oPorts.erase(oPorts.begin() + (long(p)-4000));
            else
                  iPorts.erase(iPorts.begin() + long(p)-3000);
*/
            }
      virtual bool connect(void*, void*)           { return true; }
      virtual bool disconnect(void*, void*)        { return true; }
      virtual void setPortName(void*, const QString&) {}
      virtual void* findPort(const QString& s) {
            if (s == "input1")
                  return (void*)1000;
            if (s == "input2")
                  return (void*)1001;
            if (s == "output1")
                  return (void*)2000;
            if (s == "output2")
                  return (void*)2001;
            int k = 0;
            for (std::vector<QString>::const_iterator i = iPorts.begin(); i != iPorts.end(); ++i, ++k) {
                  if (s == *i)
                        return (void*)(3000+k);
                  }
            k = 0;
            for (std::vector<QString>::const_iterator i = oPorts.begin(); i != oPorts.end(); ++i, ++k) {
                  if (s == *i)
                        return (void*)(4000+k);
                  }
            return 0;
            }
      virtual QString portName(void* p) {
            if (long(p) == 1000)
                  return QString("input1");
            if (long(p) == 1001)
                  return QString("input2");
            if (long(p) == 2000)
                  return QString("output1");
            if (long(p) == 2001)
                  return QString("output2");
            if (long(p) >= 4000)
                  return QString(oPorts[long(p)-4000]);
            else
                  return QString(iPorts[long(p)-3000]);
            }
      virtual unsigned getCurFrame() { return pos; }
      virtual int realtimePriority() const { return 40; }
      virtual void startTransport() {
            state = Audio::PLAY;
            }
      virtual void stopTransport() {
            state = Audio::STOP;
            }
      virtual void seekTransport(unsigned n) {
            seekflag = true;
            pos = n;
            }
      virtual void setFreewheel(bool) {}
      virtual bool equal(Port a, Port b) {
            return a == b;
            }
      virtual void putEvent(Port, const MidiEvent&) {}
      };

DummyAudio* dummyAudio;

//---------------------------------------------------------
//   initDummyAudio
//---------------------------------------------------------

bool initDummyAudio()
      {
      dummyAudio = new DummyAudio();
      audioDriver = dummyAudio;
      return false;
      }

//---------------------------------------------------------
//   outputPorts
//---------------------------------------------------------

QList<PortName> DummyAudio::outputPorts(bool midi)
      {
      QList<PortName> clientList;
      if (!midi) {
            PortName p1;
            p1.name = QString("output1");
            p1.port = (void*)100;
            PortName p2;
            p2.name = QString("output2");
            p2.port = (void*)101;
            clientList.append(p1);
            clientList.append(p2);
            }
      return clientList;
      }

//---------------------------------------------------------
//   inputPorts
//---------------------------------------------------------

QList<PortName> DummyAudio::inputPorts(bool midi)
      {
      QList<PortName> clientList;
      if (!midi) {
            PortName p1;
            p1.name = QString("input1");
            p1.port = (void*)0;
            PortName p2;
            p2.name = QString("input2");
            p2.port = (void*)1;
            clientList.append(p1);
            clientList.append(p2);
            }
      return clientList;
      }

//---------------------------------------------------------
//   dummyLoop
//---------------------------------------------------------

static void* dummyLoop(void*)
      {
#ifndef __APPLE__
      if (realTimePriority) {
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
#endif

      for (;;) {
            if (audioState == AUDIO_RUNNING)
	            audio->process(segmentSize, dummyAudio->state);
            else if (audioState == AUDIO_START1)
                  audioState = AUDIO_START2;
            usleep(dummyFrames*1000000/AL::sampleRate);
            if (dummyAudio->seekflag) {
                  audio->sync(Audio::STOP, dummyAudio->pos);
                  dummyAudio->seekflag = false;
                  }
            if (dummyAudio->state == Audio::PLAY) {
                  dummyAudio->pos += dummyFrames;
                  }
            }
      pthread_exit(0);
      }

//---------------------------------------------------------
//   start
//---------------------------------------------------------

void DummyAudio::start(int priority)
      {
      realTimePriority = priority;
      pthread_attr_t* attributes = 0;

      if (priority) {
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
      if (pthread_create(&dummyThread, attributes, ::dummyLoop, this))
            perror("creating thread failed:");
      if (priority)
	      pthread_attr_destroy(attributes);
      }

void DummyAudio::stop ()
      {
      pthread_cancel(dummyThread);
      pthread_join(dummyThread, 0);
      }

