//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: jackaudio.h,v 1.20.2.4 2009/12/20 05:00:35 terminator356 Exp $
//  (C) Copyright 2002 Werner Schweer (ws@seh.de)
//=========================================================

#ifndef __JACKAUDIO_H__
#define __JACKAUDIO_H__

#include <jack/jack.h>
#include "audiodev.h"

//---------------------------------------------------------
//   JackAudioDevice
//---------------------------------------------------------
bool checkAudioDevice();

class JackAudioDevice : public AudioDevice {

      jack_client_t* _client;
      double sampleTime;
      int samplePos;
      jack_transport_state_t transportState;
      jack_position_t pos;
      char jackRegisteredName[8];
      int dummyState;
      int dummyPos;

   public:
      JackAudioDevice(jack_client_t* cl, char * jack_id_string);
      virtual ~JackAudioDevice();
      virtual void nullify_client() { _client = 0; }
      
      //virtual void start();
      virtual void start(int);
      virtual void stop ();
      virtual bool dummySync(int state); // Artificial sync when not using Jack transport.
      
      virtual int framePos() const;

      virtual float* getBuffer(void* port, unsigned long nframes) {
            return (float*)jack_port_get_buffer((jack_port_t*)port, nframes);
            }

      virtual std::list<QString> outputPorts();
      virtual std::list<QString> inputPorts();

      virtual void registerClient();

      virtual void* registerOutPort(const char* name);
      virtual void* registerInPort(const char* name);

      virtual char* getJackName();

      virtual void unregisterPort(void*);
      virtual void connect(void*, void*);
      virtual void disconnect(void*, void*);
      virtual void setPortName(void* p, const char* n) { jack_port_set_name((jack_port_t*)p, n); }
      virtual void* findPort(const char* name);
      virtual QString portName(void* port);
      virtual int getState();
      virtual unsigned int getCurFrame();
      virtual bool isRealtime()          { return jack_is_realtime(_client); }
      virtual int realtimePriority() const;
      virtual void startTransport();
      virtual void stopTransport();
      virtual void seekTransport(unsigned frame);
      virtual void seekTransport(const Pos &p);
      virtual void setFreewheel(bool f);
      jack_transport_state_t transportQuery(jack_position_t* pos);
      void graphChanged();
      virtual int setMaster(bool f);

      //static bool jackStarted;
      };

#endif

