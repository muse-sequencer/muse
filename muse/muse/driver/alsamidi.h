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

#ifndef __ALSAMIDI_H__
#define __ALSAMIDI_H__

#include <alsa/asoundlib.h>
#include "driver.h"

class MidiSeq;
class MidiEvent;

//---------------------------------------------------------
//   AlsaMidi
//---------------------------------------------------------

class AlsaMidi : public Driver {
      snd_seq_t* alsaSeq;

      void removeConnection(snd_seq_connect_t* ev);
      void addConnection(snd_seq_connect_t* ev);
      bool putEvent(snd_seq_event_t* event);

   public:
      AlsaMidi();
      virtual bool init();

      virtual QList<PortName> outputPorts(bool midi);
      virtual QList<PortName> inputPorts(bool midi);

      virtual Port registerOutPort(const QString& name, bool midi);
      virtual Port registerInPort(const QString& name, bool midi);
      virtual void unregisterPort(Port);
      virtual void setPortName(Port p, const QString& n);
      virtual QString portName(Port);
      virtual Port findPort(const QString& name);

      virtual bool connect(Port, Port);
      virtual bool disconnect(Port, Port);
      virtual void putEvent(Port, const MidiEvent&);

      void getInputPollFd(struct pollfd**, int* n);
      void getOutputPollFd(struct pollfd**, int* n);

      void read(MidiSeq*);      // process events
      void write();
      };

extern AlsaMidi alsaMidi;
extern AlsaMidi* midiDriver;
#endif

