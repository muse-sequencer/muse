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
      float* buffer;
      pthread_t dummyThread;
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
            posix_memalign((void**)&buffer, 16, sizeof(float) * dummyFrames);
            }
      virtual ~DummyAudio() {
            free(buffer);
            }

      virtual bool init()   { return true; }
      virtual void start(int);
      virtual void stop ();
      virtual unsigned frameTime() const {
            return lrint(curTime() * AL::sampleRate);
            }
      virtual unsigned lastFrameTime() const {
            return lrint(startTime * AL::sampleRate);
            }
      virtual unsigned curFrame() const { return pos; }

      virtual float* getBuffer(Port /*port*/, unsigned long nframes)
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

      virtual Port registerOutPort(const QString& s, bool) {
            iPorts.push_back(QString(s));
            Port port(0, iPorts.size() + 3000);
            return port;
            }
      virtual Port registerInPort(const QString& s, bool) {
            oPorts.push_back(QString(s));
            Port port(0, oPorts.size() + 40);
            return port;
            }
      virtual void unregisterPort(Port) {
/*            if (long(p) >= 100)
                  oPorts.erase(oPorts.begin() + (long(p)-40));
            else
                  iPorts.erase(iPorts.begin() + long(p)-30);
*/
            }
      virtual bool connect(Port, Port)           { return true; }
      virtual bool disconnect(Port, Port)        { return true; }
      virtual void setPortName(Port, const QString&) {}
      virtual Port findPort(const QString& s) {
            if (s == "input1")
                  return Port(0, 10);
            if (s == "input2")
                  return Port(0, 11);
            if (s == "output1")
                  return Port(0, 20);
            if (s == "output2")
                  return Port(0, 21);
            int k = 0;
            for (std::vector<QString>::const_iterator i = iPorts.begin(); i != iPorts.end(); ++i, ++k) {
                  if (s == *i)
                        return Port(0, 30+k);
                  }
            k = 0;
            for (std::vector<QString>::const_iterator i = oPorts.begin(); i != oPorts.end(); ++i, ++k) {
                  if (s == *i)
                        return Port(0, 40);
                  }
            return Port();
            }
      virtual QString portName(Port port) {
            if (port.alsaPort() == 10)
                  return QString("input1");
            if (port.alsaPort() == 11)
                  return QString("input2");
            if (port.alsaPort() == 20)
                  return QString("output1");
            if (port.alsaPort() == 21)
                  return QString("output2");
            if (port.alsaPort() >= 40)
                  return QString(oPorts[port.alsaPort() - 40]);
            else
                  return QString(iPorts[port.alsaPort() - 30]);
            }
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
            p1.port = Port(0, 100);
            PortName p2;
            p2.name = QString("output2");
            p2.port = Port(0, 101);
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
            p1.port = Port(0, 0);
            PortName p2;
            p2.name = QString("input2");
            p2.port = Port(0, 1);
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

