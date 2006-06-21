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

#ifndef __COREMIDI_H__
#define __COREMIDI_H__

#include "driver.h"

class MidiSeq;
class MidiEvent;

//typedef snd_seq_addr_t* AlsaPort;

//---------------------------------------------------------
//   CoreMidi
//---------------------------------------------------------

class CoreMidi : public Driver {
      //snd_seq_t* alsaSeq;

//       void removeConnection(snd_seq_connect_t* ev);
//       void addConnection(snd_seq_connect_t* ev);

   public:
      CoreMidi();
      virtual bool init();

      virtual std::list<PortName>* outputPorts();
      virtual std::list<PortName>* inputPorts();

      virtual Port registerOutPort(const QString& name);
      virtual Port registerInPort(const QString& name);
      virtual void unregisterPort(Port);
      virtual void setPortName(Port p, const QString& n);
      virtual QString portName(Port);
      virtual Port findPort(const QString& name);
      virtual bool equal(Port, Port);

      virtual bool connect(Port, Port);
      virtual bool disconnect(Port, Port);

      void getInputPollFd(struct pollfd**, int* n);
      void getOutputPollFd(struct pollfd**, int* n);

      void read(MidiSeq*);      // process events
      void write();

      void putEvent(Port, const MidiEvent&);
      //bool putEvent(snd_seq_event_t* event);
      };

extern CoreMidi coreMidi;
extern CoreMidi* midiDriver;
#endif

