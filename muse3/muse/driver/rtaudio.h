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

#include "audiodev.h"
#include "large_int.h"
#include <rtaudio/RtAudio.h>


// For debugging output: Uncomment the fprintf section.
#define DEBUG_RTAUDIO(dev, format, args...) // fprintf(dev, format, ##args);

namespace MusECore {

//---------------------------------------------------------
//   RtAudioDevice
//---------------------------------------------------------

struct MuseRtAudioPort {
  QString name;
  float* buffer;
};

class RtAudioDevice : public AudioDevice {
  private:
      RtAudio *dac;

      // Critical variables that need to all update at once.
      // We employ a 'flipping' technique.
      unsigned _framesAtCycleStart[2];
      uint64_t _timeUSAtCycleStart[2];
      unsigned _frameCounter[2];
      unsigned _criticalVariablesIdx;

   public:
      // Time in microseconds at which the driver was created.
      uint64_t _start_timeUS;

      QList<MuseRtAudioPort*> outputPortsList;
      QList<MuseRtAudioPort*> inputPortsList;

      virtual const char* driverName() const { return "RtAudioDevice"; }

      virtual QString driverBackendName() {

        auto api = dac->getCurrentApi();
        switch (api) {
          case RtAudio::RTAUDIO_DUMMY:
            return "RTAUDIO_DUMMY";
            break;
          case RtAudio::UNSPECIFIED:
              return "UNSPECIFIED";
              break;
          case RtAudio::LINUX_ALSA:
              return "LINUX_ALSA";
              break;
          case RtAudio::LINUX_PULSE:
              return "LINUX_PULSE";
              break;
          case RtAudio::LINUX_OSS:
              return "LINUX_OSS";
              break;
          case RtAudio::UNIX_JACK:
              return "UNIX_JACK";
              break;
          case RtAudio::MACOSX_CORE:
              return "MACOSX_CORE";
              break;
          case RtAudio::WINDOWS_WASAPI:
              return "WINDOWS_WASAPI";
              break;
          case RtAudio::WINDOWS_ASIO:
              return "WINDOWS_ASIO";
              break;
          case RtAudio::WINDOWS_DS:
              return "WINDOWS_DS";
              break;
          default:
              return "UNKNOWN";
        }

      }

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

      RtAudioDevice(bool forceDefault);
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

      virtual bool start(int);

      virtual void stop ();
      virtual unsigned  framePos() const {
            // Not much choice but to do this:
            const unsigned int facs = framesAtCycleStart();
            const unsigned int fscs = framesSinceCycleStart();
            DEBUG_RTAUDIO(stderr, "RtAudioDevice::framePos framesAtCycleStart:%u framesSinceCycleStart:%u\n", facs, fscs);
            return facs + fscs;
            }

      // These are meant to be called from inside process thread only.
      virtual unsigned framesAtCycleStart() const { return _framesAtCycleStart[_criticalVariablesIdx]; }
      virtual unsigned framesSinceCycleStart() const
      {
        const uint64_t ct = systemTimeUS();
        DEBUG_RTAUDIO(stderr, "RtAudioDevice::framesSinceCycleStart systemTimeUS:%lu timeUSAtCycleStart:%lu\n",
                      ct, _timeUSAtCycleStart[_criticalVariablesIdx]);
        // Do not round up here since time resolution is higher than (audio) frame resolution.
        unsigned f = muse_multiply_64_div_64_to_64(ct - _timeUSAtCycleStart[_criticalVariablesIdx], MusEGlobal::sampleRate, 1000000UL);

        // Safety due to inaccuracies. It cannot be after the segment, right?
        if(f >= MusEGlobal::segmentSize)
          f = MusEGlobal::segmentSize - 1;
        return f;
      }

      virtual float* getBuffer(void* port, unsigned long nframes)
      {
        if (nframes > MusEGlobal::segmentSize) {

          fprintf(stderr, "RtAudioDevice::getBuffer nframes > segment size\n");

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

        fprintf(stderr, "register output port [%s] length %d char %c\n", name, int(strlen(name)), name[strlen(name)-1] );

        foreach (MuseRtAudioPort *port, outputPortsList) {
          if (port->name == name) {
            fprintf(stderr, "RtAudioDevice::registerOutPort - port [%s] already exists, return existing.", name);
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

        fprintf(stderr, "register input port [%s] length %d char %c\n", name, int(strlen(name)), name[strlen(name)-1] );

        foreach (MuseRtAudioPort *port, inputPortsList) {
          if (port->name == name) {
            fprintf(stderr, "RtAudioDevice::registerInPort - port [%s] already exists, return existing.", name);
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

      virtual AudioDevice::PortType portType(void*) const { return AudioPort; }
      virtual AudioDevice::PortDirection portDirection(void*) const { return OutputPort; }
      virtual void unregisterPort(void*) {}
      virtual bool connect(void* /*src*/, void* /*dst*/)  {
        DEBUG_RTAUDIO(stderr, "RtAudio::connect\n");
        return false;
      }
      virtual bool connect(const char* /*src*/, const char* /*dst*/)  {
        DEBUG_RTAUDIO(stderr, "RtAudio::connect2\n");
        return false;
      }
      virtual bool disconnect(void* /*src*/, void* /*dst*/)  { return false; }
      virtual bool disconnect(const char* /*src*/, const char* /*dst*/)  { return false; }
      virtual int connections(void* /* clientPort */) {
        DEBUG_RTAUDIO(stderr, "RtAudio::connections\n");
        return 1; // always return nonzero, for now
      }
      virtual bool portConnectedTo(void*, const char*) {
        DEBUG_RTAUDIO(stderr, "RtAudio::portConnectedTo\n");
        return false;
      }
      virtual bool portsCanDisconnect(void* /*src*/, void* /*dst*/) const {
        DEBUG_RTAUDIO(stderr, "RtAudio::portCanDisconnect\n");
        return false;
      }
      virtual bool portsCanDisconnect(const char* /*src*/, const char* /*dst*/) const {
        DEBUG_RTAUDIO(stderr, "RtAudio::portCanDisconnect2\n");
        return false;
      }
      virtual bool portsCanConnect(void* /*src*/, void* /*dst*/) const {
        DEBUG_RTAUDIO(stderr, "RtAudio::portCanConnect\n");
        return false;
      }
      virtual bool portsCanConnect(const char* /*src*/, const char* /*dst*/) const {
        DEBUG_RTAUDIO(stderr, "RtAudio::portCanConnect\n");
        return false;
      }
      virtual bool portsCompatible(void* /*src*/, void* /*dst*/) const {
        DEBUG_RTAUDIO(stderr, "RtAudio::portCompatible\n");
        return false;
      }
      virtual bool portsCompatible(const char* /*src*/, const char* /*dst*/) const {
        DEBUG_RTAUDIO(stderr, "RtAudio::portCompatible2\n");
        return false;
      }
      virtual void setPortName(void*, const char*) {
        DEBUG_RTAUDIO(stderr, "RtAudio::setPortName\n");
      }
      virtual void* findPort(const char*) {
        DEBUG_RTAUDIO(stderr, "RtAudio::findPort\n");
        return 0;
      }
      // preferred_name_or_alias: -1: No preference 0: Prefer canonical name 1: Prefer 1st alias 2: Prefer 2nd alias.
      virtual char*  portName(void*, char* str, int str_size, int /*preferred_name_or_alias*/ = -1) {
        DEBUG_RTAUDIO(stderr, "RtAudio::portName %s\n", str);
        if(str_size == 0) {
          return 0;
        }
        str[0] = '\0';
        return str;
      }
      virtual const char* canonicalPortName(void*) {
        DEBUG_RTAUDIO(stderr, "RtAudio::canonicalPortName\n");
        return 0;
      }
      virtual unsigned int portLatency(void* /*port*/, bool /*capture*/) const {
        DEBUG_RTAUDIO(stderr, "RtAudio::portLatency\n");
        return 0;
      }

      virtual unsigned frameTime() const {
        return _frameCounter[_criticalVariablesIdx];
      }

      virtual bool isRealtime() { return MusEGlobal::realTimeScheduling; }
      virtual int realtimePriority() const { return 40; }

      virtual void setFreewheel(bool) {}
      virtual int setMaster(bool, bool /*unconditional*/ = false) { return 1; }
};

} // namespace MusECore
