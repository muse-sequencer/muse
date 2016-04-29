//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: jackaudio.h,v 1.20.2.4 2009/12/20 05:00:35 terminator356 Exp $
//  (C) Copyright 2002 Werner Schweer (ws@seh.de)
//  (C) Copyright 2015 Tim E. Real (terminator356 on sourceforge.net)
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

#ifndef __JACKAUDIO_H__
#define __JACKAUDIO_H__

#include <jack/jack.h>
#include <list> 
#include "audiodev.h"
#include "operations.h" 

class QString;

namespace MusEGlobal {
bool checkAudioDevice();
}

namespace MusECore {

enum JackCallbackEventType {PortRegister, PortUnregister, PortConnect, PortDisconnect, GraphChanged};
struct JackCallbackEvent
{
  JackCallbackEventType type;
  jack_port_id_t port_id_A;
  jack_port_id_t port_id_B;
  jack_port_t* port_A;
  jack_port_t* port_B;
};
typedef std::list<JackCallbackEvent> JackCallbackEventList;
typedef std::list<JackCallbackEvent>::iterator iJackCallbackEvent;

//---------------------------------------------------------
//   JackAudioDevice
//---------------------------------------------------------

class JackAudioDevice : public AudioDevice {
      jack_client_t* _client;
      float _syncTimeout;
      jack_transport_state_t transportState;
      jack_position_t pos;
      char jackRegisteredName[16];
      int dummyState;
      int dummyPos;
      volatile int _dummyStatePending;
      volatile int _dummyPosPending;
      // Free-running frame counter incremented always in process.
      jack_nframes_t _frameCounter; 

      PendingOperationList operations;
      // Temporary, for processing callback event FIFO.
      JackCallbackEventList jackCallbackEvents; 
      
      void getJackPorts(const char** ports, std::list<QString>& name_list, bool midi, bool physical, int aliases);
      static int processAudio(jack_nframes_t frames, void*);
      
      void processGraphChanges();
      void processJackCallbackEvents(const Route& our_node, jack_port_t* our_port, RouteList* route_list, bool is_input);
      void checkNewRouteConnections(jack_port_t* our_port, int channel, RouteList* route_list);
      // Return 0: Neither disconnect nor unregister found
      //        1: Disconnect found followed later by unregister
      //        2: Disconnect found (with no unregister later)
      int checkDisconnectCallback(const jack_port_t* our_port, const jack_port_t* port);
      // Return 0: No port register found (or it was cancelled by a later unregister)
      //        1: Port register was found.
      int checkPortRegisterCallback(const jack_port_t* port);

      static int static_JackXRunCallback(void *);
     

   public:
      JackAudioDevice(jack_client_t* cl, char * jack_id_string);
      virtual ~JackAudioDevice();
      virtual inline int deviceType() const { return JACK_AUDIO; }   
      
      virtual void start(int);
      virtual void stop ();
      
      // These are meant to be called from inside process thread only.      
      virtual unsigned framesAtCycleStart() const;
      virtual unsigned framesSinceCycleStart() const;

      jack_client_t* jackClient() const { return _client; }
      virtual void registerClient();
      virtual const char* clientName() { return jackRegisteredName; }
      virtual void nullify_client() { _client = 0; }

      float getDSP_Load();
      virtual std::list<QString> outputPorts(bool midi = false, int aliases = -1);
      virtual std::list<QString> inputPorts(bool midi = false, int aliases = -1);
      virtual void* registerOutPort(const char* /*name*/, bool /*midi*/);
      virtual void* registerInPort(const char* /*name*/, bool /*midi*/);
      virtual void unregisterPort(void*);
      virtual AudioDevice::PortType portType(void*) const;
      virtual AudioDevice::PortDirection portDirection(void*) const;
      virtual void connect(void* src, void* dst);
      virtual void connect(const char* src, const char* dst);
      virtual void disconnect(void* src, void* dst);
      virtual void disconnect(const char* src, const char* dst);
      virtual int connections(void* clientPort) { return jack_port_connected((jack_port_t*)clientPort); }
      virtual bool portConnectedTo(void* our_port, const char* port) { return jack_port_connected_to((jack_port_t*)our_port, port); }
      // Returns true if the ports are connected.
      virtual bool portsCanDisconnect(void* src, void* dst) const;
      // Returns true if the ports are found and they are connected.
      virtual bool portsCanDisconnect(const char* src, const char* dst) const;
      // Returns true if the ports are not connected and CAN be connected.
      virtual bool portsCanConnect(void* src, void* dst) const;
      // Returns true if the ports are found and they are not connected and CAN be connected.
      virtual bool portsCanConnect(const char* src, const char* dst) const;
      // Returns true if the ports CAN be connected.
      virtual bool portsCompatible(void* src, void* dst) const;
      // Returns true if the ports are found and they CAN be connected.
      virtual bool portsCompatible(const char* src, const char* dst) const;
      virtual void setPortName(void* p, const char* n);
      // preferred_name_or_alias: -1: No preference 0: Prefer canonical name 1: Prefer 1st alias 2: Prefer 2nd alias.
      virtual char* portName(void* port, char* str, int str_size, int preferred_name_or_alias = -1);
      virtual const char* canonicalPortName(void* port) { if(!port) return NULL; return jack_port_name((jack_port_t*)port); }
      virtual void* findPort(const char* name);
      virtual unsigned int portLatency(void* port, bool capture) const;
      virtual float* getBuffer(void* port, unsigned long nframes) {
            return (float*)jack_port_get_buffer((jack_port_t*)port, nframes);
            }

      virtual int getState();
      virtual unsigned int getCurFrame() const;
      virtual int framePos() const;
      virtual unsigned frameTime() const     { return _frameCounter; }  
      virtual double systemTime() const;
      virtual bool isRealtime()          { return jack_is_realtime(_client); }
      virtual int realtimePriority() const;
      
      virtual void startTransport();
      virtual void stopTransport();
      virtual void seekTransport(unsigned frame);
      virtual void seekTransport(const Pos &p);
      virtual void setFreewheel(bool f);
      virtual int setMaster(bool f);
      jack_transport_state_t transportQuery(jack_position_t* pos);
      bool timebaseQuery(unsigned frames, unsigned* bar, unsigned* beat, unsigned* tick, unsigned* curr_abs_tick, unsigned* next_ticks);

      void graphChanged();
      };

// Our own wrappers for functions we need to look up with dlsym:
typedef void(*jack_get_version_type)(int*, int*, int*, int*);
extern jack_get_version_type             jack_get_version_fp;

typedef int(*jack_port_set_name_type)(jack_port_t*, const char*);
extern jack_port_set_name_type           jack_port_set_name_fp;

typedef int(*jack_port_rename_type)(jack_client_t*, jack_port_t*, const char*);
extern jack_port_rename_type             jack_port_rename_fp;


} // namespace MusECore

#endif

