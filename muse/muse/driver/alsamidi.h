//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: alsamidi.h,v 1.10 2005/12/19 16:16:27 wschweer Exp $
//  (C) Copyright 2001 Werner Schweer (ws@seh.de)
//=========================================================

#ifndef __ALSAMIDI_H__
#define __ALSAMIDI_H__

// #include <config.h>
#include <alsa/asoundlib.h>

#include "driver.h"

class MidiSeq;
class MidiEvent;

typedef snd_seq_addr_t* AlsaPort;

//---------------------------------------------------------
//   AlsaMidi
//---------------------------------------------------------

class AlsaMidi : public Driver {
      snd_seq_t* alsaSeq;

      void removeConnection(snd_seq_connect_t* ev);
      void addConnection(snd_seq_connect_t* ev);

   public:
      AlsaMidi();
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
      bool putEvent(snd_seq_event_t* event);
      };

extern AlsaMidi alsaMidi;
extern AlsaMidi* midiDriver;
#endif

