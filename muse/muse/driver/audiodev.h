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
      virtual bool restart() { return false; }           // return true on error
      virtual void stop () = 0;
      virtual unsigned framePos() const = 0;
      virtual float* getBuffer(Port, unsigned long nframes) = 0;
      virtual void registerClient() = 0;
      virtual Port registerOutPort(const QString& name, bool midi) = 0;
      virtual Port registerInPort(const QString& name, bool midi) = 0;
      virtual unsigned getCurFrame() = 0;
      virtual int realtimePriority() const = 0;	// return zero if not realtime
      virtual void startTransport() = 0;
      virtual void stopTransport() = 0;
      virtual void seekTransport(unsigned frame) = 0;
      virtual void setFreewheel(bool f) = 0;
      virtual void graphChanged() {}
      virtual void startMidiCycle(Port) {}
      };

#endif

