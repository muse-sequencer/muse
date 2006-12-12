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

#include "audiodev.h"

//---------------------------------------------------------
//   JackAudio
//---------------------------------------------------------

class JackAudio : public AudioDriver {
      jack_client_t* _client;
      jack_position_t pos;
      char jackRegisteredName[8];
      jack_transport_state_t transportState;

   public:
      JackAudio(jack_client_t* cl, char * jack_id_string);
      virtual ~JackAudio();

      int getTransportState();
      virtual bool init();
      virtual void start(int);
      virtual bool restart();
      virtual void stop ();
      virtual void zeroClientPtr() { _client = 0; }
      virtual float* getBuffer(Port port, unsigned long nframes) {
            return (float*)jack_port_get_buffer(port.jackPort(), nframes);
            }

      virtual QList<PortName> outputPorts(bool midi);
      virtual QList<PortName> inputPorts(bool midi);

      virtual void registerClient();

      virtual Port registerOutPort(const QString& name, bool midi);
      virtual Port registerInPort(const QString& name, bool midi);

      virtual char* getJackName();

      virtual void unregisterPort(Port);
      virtual bool connect(Port, Port);
      virtual bool disconnect(Port, Port);
      virtual void setPortName(Port p, const QString& n) {
            jack_port_set_name(p.jackPort(), n.toLatin1().data());
            }
      virtual Port findPort(const QString& name);
      virtual QString portName(Port);
      virtual int realtimePriority() const;
      virtual void startTransport();
      virtual void stopTransport();
      virtual void seekTransport(unsigned frame);
      virtual void setFreewheel(bool f);

      jack_transport_state_t transportQuery(jack_position_t* pos) {
            return jack_transport_query(_client, pos);
            }
      void graphChanged();
      virtual void putEvent(Port, const MidiEvent&);
      virtual void startMidiCycle(Port);

      virtual unsigned framePos() const {
            return pos.frame + jack_frames_since_cycle_start(_client);
            }
      virtual unsigned frameTime() const      { return jack_frame_time(_client); }
      };

#endif

