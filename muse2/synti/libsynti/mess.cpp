//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: mess.cpp,v 1.2 2004/04/15 13:46:18 wschweer Exp $
//  (C) Copyright 2004 Werner Schweer (ws@seh.de)
//
//  This program is free software; you can redistribute it and/or
//  modify it under the terms of the GNU General Public License
//  as published by the Free Software Foundation; version 2 of
//  the License, or (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
//
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
      MusECore::MidiPlayEvent fifo[FIFO_SIZE];
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

void Mess::sendEvent(MusECore::MidiPlayEvent ev)
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

MusECore::MidiPlayEvent Mess::receiveEvent()
      {
      MusECore::MidiPlayEvent ev = d->fifo[d->fifoRindex];
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

bool Mess::processEvent(const MusECore::MidiPlayEvent& ev)
      {
      switch(ev.type()) {
            case MusECore::ME_NOTEON:
                  return playNote(ev.channel(), ev.dataA(), ev.dataB());
            case MusECore::ME_NOTEOFF:
                  return playNote(ev.channel(), ev.dataA(), 0);
            case MusECore::ME_SYSEX:
	            return sysex(ev.len(), ev.data());
            case MusECore::ME_CONTROLLER:
                  return setController(ev.channel(), ev.dataA(), ev.dataB());
            case MusECore::ME_PITCHBEND:       // Tim.
                  return setController(ev.channel(), MusECore::CTRL_PITCH, ev.dataA());
            }
      return false;
      }

