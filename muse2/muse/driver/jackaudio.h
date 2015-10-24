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
class MidiPlayEvent;

enum JackCallbackEventType {PortRegister, PortUnregister, PortConnect, PortDisconnect, GraphChanged };
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
      //
      int checkDisconnectCallback(const jack_port_t* our_port, const jack_port_t* port);
      int checkPortRegisterCallback(const jack_port_t* port);
     

   public:
      JackAudioDevice(jack_client_t* cl, char * jack_id_string);
      virtual ~JackAudioDevice();
      virtual void nullify_client() { _client = 0; }
      
      virtual inline int deviceType() const { return JACK_AUDIO; }   
      
      virtual void start(int);
      virtual void stop ();
      
      virtual int framePos() const;
      virtual unsigned frameTime() const     { return _frameCounter; }  
      virtual double systemTime() const;

      virtual float* getBuffer(void* port, unsigned long nframes) {
            return (float*)jack_port_get_buffer((jack_port_t*)port, nframes);
            }

      virtual std::list<QString> outputPorts(bool midi = false, int aliases = -1);
      virtual std::list<QString> inputPorts(bool midi = false, int aliases = -1);

      jack_client_t* jackClient() const { return _client; }
      virtual void registerClient();
      virtual const char* clientName() { return jackRegisteredName; }

      virtual void* registerOutPort(const char* /*name*/, bool /*midi*/);
      virtual void* registerInPort(const char* /*name*/, bool /*midi*/);

      virtual void unregisterPort(void*);
      virtual void connect(void*, void*);
      virtual void connect(const char*, const char*);
      virtual void disconnect(void*, void*);
      virtual void disconnect(const char*, const char*);
      virtual int connections(void* clientPort) { return jack_port_connected((jack_port_t*)clientPort); }
      virtual void setPortName(void* p, const char* n) { jack_port_rename(_client,(jack_port_t*)p, n); }
      virtual void* findPort(const char* name);
      virtual char* portName(void* port, char* str, int str_size);
      virtual const char* canonicalPortName(void* port) { if(!port) return NULL; return jack_port_name((jack_port_t*)port); }
      virtual unsigned int portLatency(void* port, bool capture) const;
      virtual int getState();
      virtual unsigned int getCurFrame() const;
      virtual bool isRealtime()          { return jack_is_realtime(_client); }
      virtual int realtimePriority() const;
      virtual void startTransport();
      virtual void stopTransport();
      virtual void seekTransport(unsigned frame);
      virtual void seekTransport(const Pos &p);
      virtual void setFreewheel(bool f);
      jack_transport_state_t transportQuery(jack_position_t* pos);
      bool timebaseQuery(unsigned frames, unsigned* bar, unsigned* beat, unsigned* tick, unsigned* curr_abs_tick, unsigned* next_ticks);
      void graphChanged();
      virtual bool portConnectedTo(void* our_port, const char* port) { return jack_port_connected_to((jack_port_t*)our_port, port); }
      
      virtual int setMaster(bool f);

      virtual float getCPULoad() { return (_client != NULL) ? jack_cpu_load(_client) : 0.0f; }

      //static bool jackStarted;
      };

// Our own wrappers for functions we need to look up with dlsym:
typedef void(*jack_get_version_type)(int*, int*, int*, int*);
extern jack_get_version_type             jack_get_version_fp;

} // namespace MusECore

#endif

