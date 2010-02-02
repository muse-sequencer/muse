//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: audiodev.h,v 1.5.2.2 2009/12/20 05:00:35 terminator356 Exp $
//
//  (C) Copyright 1999/2000 Werner Schweer (ws@seh.de)
//=========================================================

#ifndef __AUDIODEV_H__
#define __AUDIODEV_H__

#include <qstring.h>
#include <list>

class MidiPlayEvent;

//---------------------------------------------------------
//   AudioDevice
//---------------------------------------------------------

class AudioDevice {

   public:
      AudioDevice() {}
      virtual ~AudioDevice() {}

      //virtual void start() = 0;
      virtual void start(int priority) = 0;
      
      virtual void stop () = 0;
      virtual int framePos() const = 0;
      virtual unsigned frameTime() const = 0;

      virtual float* getBuffer(void* port, unsigned long nframes) = 0;

      virtual std::list<QString> outputPorts() = 0;
      virtual std::list<QString> inputPorts() = 0;

      virtual void registerClient() = 0;

      virtual void* registerOutPort(const char* name) = 0;
      virtual void* registerInPort(const char* name) = 0;
      virtual void unregisterPort(void*) = 0;
      virtual void connect(void*, void*) = 0;
      virtual void disconnect(void*, void*) = 0;
      virtual void setPortName(void* p, const char* n) = 0;
      virtual void* findPort(const char* name) = 0;
      virtual QString portName(void* port) = 0;
      virtual int getState() = 0;
      virtual unsigned getCurFrame() = 0;
      virtual bool isRealtime() = 0;
      virtual int realtimePriority() const = 0; // return zero if not realtime
      virtual void startTransport() = 0;
      virtual void stopTransport() = 0;
      virtual void seekTransport(unsigned frame) = 0;
      virtual void seekTransport(const Pos &p) = 0;
      virtual void setFreewheel(bool f) = 0;
      virtual void graphChanged() {}
      virtual int setMaster(bool f) = 0;
      
      virtual bool putEvent(int port, const MidiPlayEvent&) = 0;
      };

#endif

