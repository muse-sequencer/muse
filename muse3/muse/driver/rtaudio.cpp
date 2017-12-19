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

#include <QMessageBox>
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

//---------------------------------------------------------
//   RtAudioDevice
//---------------------------------------------------------

struct MuseRtAudioPort {
  QString name;
  float* buffer;
};

class RtAudioDevice : public AudioDevice {

      RtAudio *dac;

   public:
      Audio::State state;
      int _framePos;
      unsigned _framesAtCycleStart;
      double _timeAtCycleStart;
      int playPos;
      bool realtimeFlag;
      bool seekflag;

      QList<MuseRtAudioPort*> outputPortsList;
      QList<MuseRtAudioPort*> inputPortsList;

      RtAudioDevice();
      virtual ~RtAudioDevice()
      {

        while (outputPortsList.size() > 0) {
          MuseRtAudioPort *port = outputPortsList.takeFirst();
          free (port->buffer);
          free (port);
        }

        while (inputPortsList.size() > 0) {
          MuseRtAudioPort *port = inputPortsList.takeFirst();
          free (port->buffer);
          free (port);
        }

      }

      virtual inline int deviceType() const { return RTAUDIO_AUDIO; }
      
      virtual void start(int);
      
      virtual void stop ();
      virtual int framePos() const { 
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

        return ((MuseRtAudioPort*)port)->buffer;

      }

      virtual std::list<QString> outputPorts(bool, int) {
        std::list<QString> outlist;

        foreach (MuseRtAudioPort *port, outputPortsList) {

          outlist.push_back(port->name);
        }

        return outlist;
      }

      virtual std::list<QString> inputPorts(bool, int) {
        std::list<QString> inlist;

        foreach (MuseRtAudioPort *port, inputPortsList) {

          inlist.push_back(port->name);
        }

        return inlist;
      }

      virtual void registerClient() {}

      virtual const char* clientName() { return "RtAudio"; }
      
      virtual void* registerOutPort(const char* name, bool) {

        printf("register output port [%s] length %d char %c\n", name, int(strlen(name)), name[strlen(name)-1] );

        foreach (MuseRtAudioPort *port, outputPortsList) {
          if (port->name == name) {
            printf("RtAudioDevice::registerOutPort - port [%s] already exists, return existing.", name);
            return port;
          }
        }

        MuseRtAudioPort *port = new MuseRtAudioPort();
        port->name = name;
        port->buffer = new float[MusEGlobal::segmentSize];
        memset(port->buffer, 0, MusEGlobal::segmentSize * sizeof(float));

        outputPortsList.push_back(port);
        return port;
      }

      virtual void* registerInPort(const char* name, bool) {

        printf("register input port [%s] length %d char %c\n", name, int(strlen(name)), name[strlen(name)-1] );

        foreach (MuseRtAudioPort *port, inputPortsList) {
          if (port->name == name) {
            printf("RtAudioDevice::registerInPort - port [%s] already exists, return existing.", name);
            return port;
          }
        }

        MuseRtAudioPort *port = new MuseRtAudioPort();
        port->name = name;
        port->buffer = new float[MusEGlobal::segmentSize];
        memset(port->buffer, 0, MusEGlobal::segmentSize * sizeof(float));

        inputPortsList.push_back(port);
        return port;
      }

      float getDSP_Load() {
        return 0.0f;
      }

      virtual AudioDevice::PortType portType(void*) const { return AudioPort; }
      virtual AudioDevice::PortDirection portDirection(void*) const { return OutputPort; }
      virtual void unregisterPort(void*) {}
      virtual bool connect(void* /*src*/, void* /*dst*/)  { return false; }
      virtual bool connect(const char* /*src*/, const char* /*dst*/)  { return false; }
      virtual bool disconnect(void* /*src*/, void* /*dst*/)  { return false; }
      virtual bool disconnect(const char* /*src*/, const char* /*dst*/)  { return false; }
      virtual int connections(void* /*clientPort*/) { return 0; }
      virtual bool portConnectedTo(void*, const char*) { return false; }
      virtual bool portsCanDisconnect(void* /*src*/, void* /*dst*/) const { return false; }
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
      virtual int realtimePriority() const { return 40; }
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

RtAudioDevice* rtAudioDevice = 0;

RtAudioDevice::RtAudioDevice() : AudioDevice()
      {
      printf("Init RtAudioDevice\n");
      MusEGlobal::sampleRate = MusEGlobal::config.deviceAudioSampleRate;
      MusEGlobal::segmentSize = MusEGlobal::config.deviceAudioBufSize;

      realtimeFlag = false;
      seekflag = false;
      state = Audio::STOP;
      //startTime = curTime();
      _framePos = 0;
      _framesAtCycleStart = 0;
      _timeAtCycleStart = 0.0;
      playPos = 0;

      RtAudio::Api api = RtAudio::UNSPECIFIED;

      switch (MusEGlobal::config.deviceRtAudioBackend) {
        case 0:
          api = RtAudio::UNSPECIFIED;
          break;
        case 1:
          api = RtAudio::LINUX_ALSA;
        break;
        case 2:
          api = RtAudio::LINUX_PULSE;
        break;
        case 3:
          api = RtAudio::LINUX_OSS;
        break;
        case 4:
          api = RtAudio::UNIX_JACK;
        break;
      }

      dac = new RtAudio(api);
      if ( dac->getDeviceCount() < 1 ) {
        printf ("\nNo audio devices found!\n");
        QMessageBox::warning(NULL,"No sound device.","RtAudio did not find any audio device - start with dummy audio if this is what you want.", QMessageBox::Ok);
      }
}


//---------------------------------------------------------
//   exitRtAudio
//---------------------------------------------------------

void exitRtAudio()
{
  if(rtAudioDevice)
    delete rtAudioDevice;
  rtAudioDevice = NULL;
  MusEGlobal::audioDevice = NULL;
}


//---------------------------------------------------------
//   initRtAudio
//---------------------------------------------------------

bool initRtAudio()
{
  rtAudioDevice = new RtAudioDevice();
  MusEGlobal::audioDevice = rtAudioDevice;
  return false;
}

//---------------------------------------------------------
//   processAudio
//---------------------------------------------------------
int processAudio( void * outputBuffer, void *inputBuffer, unsigned int nBufferFrames,
         double /* streamTime */, RtAudioStreamStatus /* status */, void * /* userData */ )
{
//  printf("RtAduioDevice::processAudio %d\n", nBufferFrames);

  float *floatOutputBuffer = (float*)outputBuffer;
  float *floatInputBuffer = (float*)inputBuffer;

  rtAudioDevice->_framePos += nBufferFrames;
  rtAudioDevice->_framesAtCycleStart += nBufferFrames;

  if(rtAudioDevice->seekflag)
  {
    MusEGlobal::audio->sync(Audio::STOP, rtAudioDevice->playPos);

    rtAudioDevice->seekflag = false;
  }

  if(rtAudioDevice->state == Audio::PLAY) {

    rtAudioDevice->playPos += nBufferFrames;
  }

  if (MusEGlobal::audio->isRunning()) {

    MusEGlobal::audio->process((unsigned long)nBufferFrames);
  }

  if (rtAudioDevice->outputPortsList.size() >= 2) {

    MuseRtAudioPort *left = rtAudioDevice->outputPortsList.at(0);
    MuseRtAudioPort *right= rtAudioDevice->outputPortsList.at(1);

    // copy buffers into output
    for (unsigned int i = 0; i < nBufferFrames; i++ ) {

      floatOutputBuffer[i*2] = left->buffer[i];
      floatOutputBuffer[i*2+1] = right->buffer[i];
    }
  } else {

    //printf("Too few ports in list, won't copy any data\n");
  }

  if (rtAudioDevice->inputPortsList.size() >= 2) {

    MuseRtAudioPort *left = rtAudioDevice->inputPortsList.at(0);
    MuseRtAudioPort *right= rtAudioDevice->inputPortsList.at(1);

    // copy buffers into input
    for (unsigned int i = 0; i < nBufferFrames; i++ ) {

      left->buffer[i] = floatInputBuffer[i*2];
      right->buffer[i] = floatInputBuffer[i*2+1];
    }

  } else {

    //printf("Too few ports in list, won't copy any data\n");
  }


  return 0;
}

//---------------------------------------------------------
//   start
//---------------------------------------------------------
void RtAudioDevice::start(int /* priority */)
{
  RtAudio::StreamParameters outParameters;
  outParameters.deviceId = dac->getDefaultOutputDevice();
  outParameters.nChannels = 2;
  outParameters.firstChannel = 0;

  RtAudio::StreamParameters inParameters;
  inParameters.deviceId = dac->getDefaultInputDevice();
  inParameters.nChannels = 2;
  inParameters.firstChannel = 0;

  double data[2];

  try {

    dac->openStream( &outParameters, &inParameters, RTAUDIO_FLOAT32, MusEGlobal::sampleRate, &MusEGlobal::segmentSize, &processAudio, (void *)&data );
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
