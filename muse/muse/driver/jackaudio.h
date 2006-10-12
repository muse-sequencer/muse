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

#ifndef __JACKAUDIO_H__
#define __JACKAUDIO_H__

#include <jack/jack.h>
#include "audiodev.h"

//---------------------------------------------------------
//   JackAudio
//---------------------------------------------------------

class JackAudio : public AudioDriver {
      jack_client_t* _client;
      jack_transport_state_t transportState;
      jack_position_t pos;
      char jackRegisteredName[8];

   public:
      JackAudio(jack_client_t* cl, char * jack_id_string);
      virtual ~JackAudio();
      virtual bool init();
      virtual void start(int);
      virtual void stop ();
      virtual void zeroClientPtr() { _client = 0; }
      virtual unsigned framePos() const;

      virtual float* getBuffer(void* port, unsigned long nframes) {
            return (float*)jack_port_get_buffer((jack_port_t*)port, nframes);
            }

      virtual QList<PortName> outputPorts();
      virtual QList<PortName> inputPorts();

      virtual void registerClient();

      virtual Port registerOutPort(const QString& name);
      virtual Port registerInPort(const QString& name);

      virtual char* getJackName();

      virtual void unregisterPort(Port);
      virtual bool connect(Port, Port);
      virtual bool disconnect(Port, Port);
      virtual void setPortName(Port p, const QString& n) {
            jack_port_set_name((jack_port_t*)p, n.toLatin1().data());
            }
      virtual Port findPort(const QString& name);
      virtual QString portName(Port);
      virtual int getState();
      virtual unsigned int getCurFrame() { return pos.frame; }
      virtual int realtimePriority() const;
      virtual void startTransport();
      virtual void stopTransport();
      virtual void seekTransport(unsigned frame);
      virtual void setFreewheel(bool f);
      jack_transport_state_t transportQuery(jack_position_t* pos) {
            return jack_transport_query(_client, pos);
            }
      void graphChanged();
      virtual bool equal(Port a, Port b) { return a == b; }
      };

#endif

