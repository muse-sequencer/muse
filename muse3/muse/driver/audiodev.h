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
#ifdef _WIN32
#include <stdlib.h>
#else
#include <sys/resource.h>
#include <sys/time.h>
#endif
#include <stdint.h>

class QString;

namespace MusECore {

class Pos;

//---------------------------------------------------------
//   AudioDevice
//---------------------------------------------------------

class AudioDevice {

   private:
      //
      // The following are for the built-in transport:
      //
     
      // The amount of time to wait before sync times out, in seconds.
      float _syncTimeout;
      float _syncTimeoutCounter; // In seconds.
      int _dummyState;
      unsigned int _dummyPos;
      volatile int _dummyStatePending;
      volatile unsigned int _dummyPosPending;
  
   public:
      enum { DUMMY_AUDIO=0, JACK_AUDIO=1, RTAUDIO_AUDIO=2 };
      enum PortType { UnknownType=0, AudioPort=1, MidiPort=2 };
      enum PortDirection { UnknownDirection=0, InputPort=1, OutputPort=2 };
      
      AudioDevice();
      virtual ~AudioDevice() {}

      virtual int deviceType() const = 0;
      virtual const char* driverName() const = 0;
      virtual bool isRealtime() = 0;
      virtual int realtimePriority() const = 0; // return zero if not realtime
      
      // Returns true on success.
      virtual bool start(int priority) = 0;
      
      virtual void stop () = 0;
      // Estimated current frame.
      // This is meant to be called from threads other than the process thread.
      virtual unsigned framePos() const = 0;
      // A constantly increasing counter, incremented by segment size at cycle start.
      virtual unsigned frameTime() const = 0;
      virtual uint64_t systemTimeUS() const;
      virtual unsigned curTransportFrame() const { return 0; }

      // These are meant to be called from inside process thread only.      
      virtual unsigned framesAtCycleStart() const = 0;
      virtual unsigned framesSinceCycleStart() const = 0;

      virtual float* getBuffer(void* port, unsigned long nframes) = 0;

      virtual std::list<QString> outputPorts(bool midi = false, int aliases = -1) = 0;
      virtual std::list<QString> inputPorts(bool midi = false, int aliases = -1) = 0;

      virtual void registerClient() = 0;

      virtual const char* clientName() = 0;
      
      virtual void* registerOutPort(const char* /*name*/, bool /*midi*/) = 0;
      virtual void* registerInPort(const char* /*name*/, bool /*midi*/) = 0;

      virtual float getDSP_Load() = 0;

      virtual PortType portType(void*) const = 0;
      virtual PortDirection portDirection(void*) const = 0;
      virtual void unregisterPort(void*) = 0;
      virtual bool connect(void* src, void* dst) = 0;
      virtual bool connect(const char* src, const char* dst) = 0;
      virtual bool disconnect(void* src, void* dst) = 0;
      virtual bool disconnect(const char* src, const char* dst) = 0;
      virtual int connections(void* /*clientPort*/) = 0; 
      virtual bool portConnectedTo(void* our_port, const char* port) = 0;
      // Returns true if the ports are connected.
      virtual bool portsCanDisconnect(void* src, void* dst) const = 0;
      // Returns true if the ports are found and they are connected.
      virtual bool portsCanDisconnect(const char* src, const char* dst) const = 0;
      // Returns true if the ports are not connected and CAN be connected.
      virtual bool portsCanConnect(void* src, void* dst) const = 0;
      // Returns true if the ports are found and they are not connected and CAN be connected.
      virtual bool portsCanConnect(const char* src, const char* dst) const = 0;
      // Returns true if the ports CAN be connected.
      virtual bool portsCompatible(void* src, void* dst) const = 0;
      // Returns true if the ports are found and they CAN be connected.
      virtual bool portsCompatible(const char* src, const char* dst) const = 0;
      virtual void setPortName(void* p, const char* n) = 0;
      virtual void* findPort(const char* name) = 0;
      // preferred_name_or_alias: -1: No preference 0: Prefer canonical name 1: Prefer 1st alias 2: Prefer 2nd alias.
      virtual char* portName(void* port, char* str, int str_size, int preferred_name_or_alias = -1) = 0;
      virtual const char* canonicalPortName(void*) = 0;
      virtual unsigned int portLatency(void* port, bool capture) const = 0;
      
      virtual void setFreewheel(bool f) = 0;
      virtual void graphChanged() {}
      virtual void registrationChanged() {}
      virtual void connectionsChanged() {}
      virtual int setMaster(bool f) = 0;

      //----------------------------------------------
      //   Functions for built-in transport.
      //   Audio devices are free to ignore, defer to, 
      //    override etc. the built-in transport.
      //   For example with the Jack driver, we can use
      //    either Jack's transport or this built-in one.
      //----------------------------------------------
      
      // Sets the amount of time to wait before sync times out, in microseconds.
      // Note that at least with the Jack driver, this function seems not realtime friendly.
      virtual void setSyncTimeout(unsigned usec) { _syncTimeout = (float)usec / 1000000.0; }
      // The number of frames that the driver waits to switch to PLAY
      //  mode after the audio sync function says it is ready to roll.
      // For example Jack Transport waits one cycle while our own tranport does not.
      virtual unsigned transportSyncToPlayDelay() const { return 0; }
      virtual int getState() { return _dummyState; }
      virtual unsigned getCurFrame() const { return _dummyPos; }
      virtual void startTransport();
      virtual void stopTransport();
      virtual void seekTransport(unsigned frame);
      virtual void seekTransport(const Pos &p);
      // This includes the necessary calls to Audio::sync() and ultimately 
      //  Audio::process(). There's no need to call them from the device code.
      // Returns true on success.
      virtual bool processTransport(unsigned int frames);
      };

} // namespace MusECore

#endif

