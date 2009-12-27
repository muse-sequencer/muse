//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: mess.cpp,v 1.3 2005/05/11 14:18:48 wschweer Exp $
//  (C) Copyright 2004 Werner Schweer (ws@seh.de)
//=========================================================

#include "mess.h"
#include "muse/midi.h"

static const int FIFO_SIZE = 32;

//---------------------------------------------------------
//   MessP
//    private data for class Mess
//---------------------------------------------------------

struct MessP {
      // Event Fifo  synti -> Host:
      MidiEvent fifo[FIFO_SIZE];
      volatile int fifoSize;
      int fifoWindex;
      int fifoRindex;
      };

//---------------------------------------------------------
//   Mess
//---------------------------------------------------------

Mess::Mess(int n)
      {
      _channels     = n;
      _sampleRate   = 44100;
      d             = new MessP;
      d->fifoSize   = 0;
      d->fifoWindex = 0;
      d->fifoRindex = 0;
      }

//---------------------------------------------------------
//   Mess
//---------------------------------------------------------

Mess::~Mess()
      {
      delete d;
      }

//---------------------------------------------------------
//   getGeometry
//    dummy
//---------------------------------------------------------

void Mess::getGeometry(int* x, int* y, int* w, int* h) const
      {
      x = 0;
      y = 0;
      w = 0;
      h = 0;
      }

//---------------------------------------------------------
//   sendEvent
//    send Event synti -> host
//---------------------------------------------------------

void Mess::sendEvent(MidiEvent ev)
      {
      if (d->fifoSize == FIFO_SIZE) {
            printf("event synti->host  fifo overflow\n");
            return;
            }
      d->fifo[d->fifoWindex] = ev;
      d->fifoWindex = (d->fifoWindex + 1) % FIFO_SIZE;
      ++(d->fifoSize);
      }

//---------------------------------------------------------
//   receiveEvent
//    called from host
//---------------------------------------------------------

MidiEvent Mess::receiveEvent()
      {
      MidiEvent ev = d->fifo[d->fifoRindex];
      d->fifoRindex = (d->fifoRindex + 1) % FIFO_SIZE;
      --(d->fifoSize);
      return ev;
      }

//---------------------------------------------------------
//   eventsPending
//    called from host:
//       while (eventsPending()) {
//             receiveEvent();
//             ...
//---------------------------------------------------------

int Mess::eventsPending() const
      {
      return d->fifoSize;
      }

//---------------------------------------------------------
//   processEvent
//    return true if synti is busy
//---------------------------------------------------------

bool Mess::processEvent(const MidiEvent& ev)
      {
      switch(ev.type()) {
            case ME_NOTEON:
                  return playNote(ev.channel(), ev.dataA(), ev.dataB());
            case ME_NOTEOFF:
                  return playNote(ev.channel(), ev.dataA(), 0);
            case ME_SYSEX:
	            return sysex(ev.len(), ev.data());
            case ME_CONTROLLER:
                  return setController(ev.channel(), ev.dataA(), ev.dataB());
            }
      return false;
      }

