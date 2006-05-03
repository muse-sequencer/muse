//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: audiodev.h,v 1.10 2006/01/17 17:18:07 wschweer Exp $
//
//  (C) Copyright 1999/2000 Werner Schweer (ws@seh.de)
//=========================================================

#ifndef __AUDIODEV_H__
#define __AUDIODEV_H__

#include "driver.h"

//---------------------------------------------------------
//   AudioDevice
//---------------------------------------------------------

class AudioDriver : public Driver {

   public:
      AudioDriver() {}
      virtual ~AudioDriver() {}

      virtual void start(int priority) = 0;
      virtual void stop () = 0;
      virtual unsigned framePos() const = 0;
      virtual float* getBuffer(void* port, unsigned long nframes) = 0;
      virtual void registerClient() = 0;
      virtual Port registerOutPort(const QString& name) = 0;
      virtual Port registerInPort(const QString& name) = 0;
      virtual int getState() = 0;
      virtual unsigned getCurFrame() = 0;
      virtual int realtimePriority() const = 0;	// return zero if not realtime
      virtual void startTransport() = 0;
      virtual void stopTransport() = 0;
      virtual void seekTransport(unsigned frame) = 0;
      virtual void setFreewheel(bool f) = 0;
      virtual void graphChanged() {}
      };

#endif

