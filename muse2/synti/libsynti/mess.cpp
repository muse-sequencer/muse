//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: mess.cpp,v 1.2 2004/04/15 13:46:18 wschweer Exp $
//  (C) Copyright 2004 Werner Schweer (ws@seh.de)
//=========================================================

#include "mess.h"
#include "muse/midi.h"
#include "muse/midictrl.h"

static const int FIFO_SIZE = 32;

//---------------------------------------------------------
//   MessP
//---------------------------------------------------------

struct MessP {
      // Event Fifo  synti -> Host:
      MidiPlayEvent fifo[FIFO_SIZE];
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
      *x = 0;
      *y = 0;
      *w = 0;
      *h = 0;
      }

//---------------------------------------------------------
//   getNativeGeometry
//    dummy
//---------------------------------------------------------

void Mess::getNativeGeometry(int* x, int* y, int* w, int* h) const
      {
      *x = 0;
      *y = 0;
      *w = 0;
      *h = 0;
      }

//---------------------------------------------------------
//   sendEvent
//    send Event synti -> host
//---------------------------------------------------------

void Mess::sendEvent(MidiPlayEvent ev)
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

MidiPlayEvent Mess::receiveEvent()
      {
      MidiPlayEvent ev = d->fifo[d->fifoRindex];
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

bool Mess::processEvent(const MidiPlayEvent& ev)
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
            case ME_PITCHBEND:       // Tim.
                  return setController(ev.channel(), CTRL_PITCH, ev.dataA());
            }
      return false;
      }

