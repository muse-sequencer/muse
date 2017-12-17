//=========================================================
//  MusE
//  Linux Music Editor
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

#include <RtAudio.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <errno.h>
#include <stdarg.h>
#include <pthread.h>
#include <sys/poll.h>
#include <sys/time.h>
#include <unistd.h>

#include "config.h"
#include "audio.h"
#include "audiodev.h"
#include "globals.h"
#include "song.h"
#include "driver/alsatimer.h"
#include "pos.h"
#include "gconfig.h"
#include "utils.h"

#define DEBUG_RTAUDIO 0

#define MASTER_LEFT (void*)1
#define MASTER_RIGHT (void*)2

namespace MusECore {

class MidiPlayEvent;

//---------------------------------------------------------
//   RtAudioDevice
//---------------------------------------------------------

enum Cmd {
trSeek,
trStart,
trStop
};

struct Msg {
  enum Cmd cmd;
  int arg;
};


class RtAudioDevice : public AudioDevice {
//      float* buffer;


      int _realTimePriority;
      RtAudio *dac;

   public:
      std::list<Msg> cmdQueue;
      Audio::State state;
      int _framePos;
      unsigned _framesAtCycleStart;
      double _timeAtCycleStart;
      int playPos;
      bool realtimeFlag;
      bool seekflag;

      float* masterLeftBuffer;
      float* masterRightBuffer;

      RtAudioDevice();
      virtual ~RtAudioDevice()
      { 
//        free(buffer);
        free(masterLeftBuffer);
        free(masterRightBuffer);
      }

      virtual inline int deviceType() const { return RTAUDIO_AUDIO; }
      
      virtual void start(int);
      
      virtual void stop ();
      virtual int framePos() const { 
//            if(DEBUG_RTAUDIO)
//                printf("RtAudioDevice::framePos %d\n", _framePos);
            return _framePos; 
            }

      // These are meant to be called from inside process thread only.      
      virtual unsigned framesAtCycleStart() const { return _framesAtCycleStart; }
      virtual unsigned framesSinceCycleStart() const 
      { 
        unsigned f =  lrint((curTime() - _timeAtCycleStart) * MusEGlobal::sampleRate);
        // Safety due to inaccuracies. It cannot be after the segment, right?
        if(f >= MusEGlobal::segmentSize)
          f = MusEGlobal::segmentSize - 1;
        return f;
      }

      virtual float* getBuffer(void* port, unsigned long nframes)
      {
        if (nframes > MusEGlobal::segmentSize) {

          printf("RtAudioDevice::getBuffer nframes > segment size\n");

          exit(-1);
        }

        if (port == MASTER_LEFT) {

          return this->masterLeftBuffer;

        } else if (port == MASTER_RIGHT) {

          return this->masterRightBuffer;
        }

        return (float *)0;
      }

//      void setBuffer(float *bufferPtr) {
//        this->buffer = bufferPtr;
//      }

      virtual std::list<QString> outputPorts(bool midi = false, int aliases = -1);
      virtual std::list<QString> inputPorts(bool midi = false, int aliases = -1);

      virtual void registerClient() {}

      virtual const char* clientName() { return "MusE"; }
      
      // this method mocks creating ports at the moment
      // it should provide a dynamic approach with respect to what the hardware is capable of
      virtual void* registerOutPort(const char* name, bool) {

        printf("register output port [%s] length %d char %c\n", name, int(strlen(name)), name[strlen(name)-1] );

        if (name[strlen(name)-1] == '0') {

          return MASTER_LEFT;
        }
        if (name[strlen(name)-1] == '1') {

          return MASTER_RIGHT;
        }
        else {

          printf("RtAudioDevice::registerOutPort - ERROR illegal port being created\n");
          return 0;
        }
      }

      virtual void* registerInPort(const char*, bool) {
        return (void *)0;
      }

      float getDSP_Load() {
        return 0.0f;
      }

      virtual AudioDevice::PortType portType(void*) const { return AudioPort; }
      virtual AudioDevice::PortDirection portDirection(void*) const { return OutputPort; }
      virtual void unregisterPort(void*) {}
      virtual void connect(void* /*src*/, void* /*dst*/) {}
      virtual void connect(const char* /*src*/, const char* /*dst*/) {}
      virtual void disconnect(void* /*src*/, void* /*dst*/) {}
      virtual void disconnect(const char* /*src*/, const char* /*dst*/) {}
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

      virtual int getState() {
            return state; }
      virtual unsigned getCurFrame() const { 
            if(DEBUG_RTAUDIO)
                printf("RtAudioDevice::getCurFrame %d\n", _framePos);
      
      return _framePos; }
      virtual unsigned frameTime() const {
            return lrint(curTime() * MusEGlobal::sampleRate);
            }
      virtual double systemTime() const
      {
        struct timeval t;
        gettimeofday(&t, 0);
        //printf("%ld %ld\n", t.tv_sec, t.tv_usec);  // Note I observed values coming out of order! Causing some problems.
        return (double)((double)t.tv_sec + (t.tv_usec / 1000000.0));
      }
      virtual bool isRealtime() { return realtimeFlag; }
      //virtual int realtimePriority() const { return 40; }
      virtual int realtimePriority() const { return _realTimePriority; }
      virtual void startTransport() {
            if(DEBUG_RTAUDIO)
                printf("RtAudioDevice::startTransport playPos=%d\n", playPos);
            state = Audio::PLAY;
            }
      virtual void stopTransport() {
            if(DEBUG_RTAUDIO)
                printf("RtAudioDevice::stopTransport, playPos=%d\n", playPos);
            state = Audio::STOP;
            }
      virtual int setMaster(bool) { return 1; }

      virtual void seekTransport(const Pos &p)
      {
            if(DEBUG_RTAUDIO)
                printf("RtAudioDevice::seekTransport frame=%d topos=%d\n",playPos, p.frame());
            seekflag = true;
            //pos = n;
            playPos = p.frame();
            
      }
      virtual void seekTransport(unsigned pos) {
            if(DEBUG_RTAUDIO)
                printf("RtAudioDevice::seekTransport frame=%d topos=%d\n",playPos,pos);
            seekflag = true;
            //pos = n;
            playPos = pos;
            }
      virtual void setFreewheel(bool) {}
      void setRealTime() { realtimeFlag = true; }
};

RtAudioDevice* rtAudio = 0;

RtAudioDevice::RtAudioDevice() : AudioDevice()
      {
      printf("Init RtAudioDevice\n");
      MusEGlobal::sampleRate = MusEGlobal::config.rtAudioSampleRate;
      MusEGlobal::segmentSize = MusEGlobal::config.rtAudioBufSize;

      masterLeftBuffer = new float[MusEGlobal::segmentSize];
      masterRightBuffer = new float[MusEGlobal::segmentSize];
      memset(masterLeftBuffer, 0, MusEGlobal::segmentSize * sizeof(float));
      memset(masterRightBuffer, 0, MusEGlobal::segmentSize * sizeof(float));


      realtimeFlag = false;
      seekflag = false;
      state = Audio::STOP;
      //startTime = curTime();
      _framePos = 0;
      _framesAtCycleStart = 0;
      _timeAtCycleStart = 0.0;
      playPos = 0;
      cmdQueue.clear();


      dac = new RtAudio(RtAudio::LINUX_PULSE);
      if ( dac->getDeviceCount() < 1 ) {
        std::cout << "\nNo audio devices found!\n";
        //        exit( 0 );
      }
}


//---------------------------------------------------------
//   exitRtAudio
//---------------------------------------------------------

void exitRtAudio()
{
  if(rtAudio)
    delete rtAudio;
  rtAudio = NULL;
  MusEGlobal::audioDevice = NULL;
}


//---------------------------------------------------------
//   initRtAudio
//---------------------------------------------------------

bool initRtAudio()
{
  rtAudio = new RtAudioDevice();
  MusEGlobal::audioDevice = rtAudio;
  return false;
}

//---------------------------------------------------------
//   outputPorts
//---------------------------------------------------------

std::list<QString> RtAudioDevice::outputPorts(bool midi, int /*aliases*/)
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
std::list<QString> RtAudioDevice::inputPorts(bool midi, int /*aliases*/)
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
//   processAudio
//---------------------------------------------------------
int processAudio( void * outputBuffer, void * /* inputBuffer */, unsigned int nBufferFrames,
         double /* streamTime */, RtAudioStreamStatus /* status */, void * /* userData */ )
{
//  printf("RtAduioDevice::processAudio %d\n", nBufferFrames);
//  rtAudio->setBuffer((float *)outputBuffer);
  float *floatOutputBuffer = (float*)outputBuffer;

  rtAudio->_framePos += nBufferFrames;
  rtAudio->_framesAtCycleStart += nBufferFrames;

  if(rtAudio->seekflag)
  {
    MusEGlobal::audio->sync(Audio::STOP, rtAudio->playPos);

    rtAudio->seekflag = false;
  }

  if(rtAudio->state == Audio::PLAY) {

    rtAudio->playPos += nBufferFrames;
  }

  if (MusEGlobal::audio->isRunning()) {

    MusEGlobal::audio->process((unsigned long)nBufferFrames);
  }

  // copy buffers into output
  for (unsigned int i = 0; i < nBufferFrames; i++ ) {

    floatOutputBuffer[i*2] = rtAudio->masterLeftBuffer[i];
    floatOutputBuffer[i*2+1] = rtAudio->masterRightBuffer[i];
  }

  return 0;
}

//---------------------------------------------------------
//   start
//---------------------------------------------------------
void RtAudioDevice::start(int /* priority */)
{
  RtAudio::StreamParameters parameters;
  parameters.deviceId = dac->getDefaultOutputDevice();
  parameters.nChannels = 2;
  parameters.firstChannel = 0;
  double data[2];
  try {
    dac->openStream( &parameters, NULL, RTAUDIO_FLOAT32, MusEGlobal::sampleRate, &MusEGlobal::segmentSize, &processAudio, (void *)&data );
    dac->startStream();
  } catch ( RtAudioError& e ) {
    e.printMessage();
  }
}

void RtAudioDevice::stop ()
{
  try {
    dac->stopStream();
  } catch (RtAudioError& e) {
    e.printMessage();
  }
  if ( dac->isStreamOpen() ) {
    dac->closeStream();
  }

}


} // namespace MusECore
