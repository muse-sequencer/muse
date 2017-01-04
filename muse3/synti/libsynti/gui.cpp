//=========================================================
//  MusE
//  Linux Music Editor
//    software synthesizer helper library
//    $Id: gui.cpp,v 1.5 2004/04/11 10:46:14 wschweer Exp $
//
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

#include "gui.h"
#include "muse/midi.h"

#include <unistd.h>

//---------------------------------------------------------
//   MessGui
//---------------------------------------------------------

MessGui::MessGui()
      {
      //
      // prepare for interprocess communication:
      //
      int filedes[2];         // 0 - reading   1 - writing
      if (pipe(filedes) == -1) {
            perror("thread:creating pipe4");
            exit(-1);
            }
      readFd      = filedes[0];
      writeFd     = filedes[1];
      wFifoSize   = 0;
      wFifoWindex = 0;
      wFifoRindex = 0;
      rFifoSize   = 0;
      rFifoWindex = 0;
      rFifoRindex = 0;
      }

//---------------------------------------------------------
//   MessGui
//---------------------------------------------------------

MessGui::~MessGui()
      {
      }

//---------------------------------------------------------
//   readMessage
//---------------------------------------------------------

void MessGui::readMessage()
      {
      char c;
      while (rFifoSize) {
            ::read(readFd, &c, 1);
            processEvent(rFifo[rFifoRindex]);
            rFifoRindex = (rFifoRindex + 1) % EVENT_FIFO_SIZE;
            --rFifoSize;
            }
      }

//---------------------------------------------------------
//   sendEvent
//---------------------------------------------------------

void MessGui::sendEvent(const MusECore::MidiPlayEvent& ev)
      {
      if (wFifoSize == EVENT_FIFO_SIZE) {
            printf("event gui->synti  fifo overflow\n");
            return;
            }
      wFifo[wFifoWindex] = ev;
      wFifoWindex = (wFifoWindex + 1) % EVENT_FIFO_SIZE;
      ++wFifoSize;
      }

//---------------------------------------------------------
//   sendController
//---------------------------------------------------------

void MessGui::sendController(int ch, int idx, int val)
      {
//      MusECore::MidiPlayEvent pe(0, 0, ch, MusECore::ME_CONTROLLER, idx, val);
//      sendEvent(pe);

      sendEvent(MusECore::MidiPlayEvent(0, 0, ch, MusECore::ME_CONTROLLER, idx, val));
      }

//---------------------------------------------------------
//   sendSysex
//---------------------------------------------------------

void MessGui::sendSysex(unsigned char* p, int n)
      {
//      MusECore::MidiPlayEvent pe(0, 0, MusECore::ME_SYSEX, p, n);
//      sendEvent(pe);
        
        sendEvent(MusECore::MidiPlayEvent(0, 0, MusECore::ME_SYSEX, p, n));
      }

//---------------------------------------------------------
//   writeEvent
//    send an event to synti gui
//---------------------------------------------------------

void MessGui::writeEvent(const MusECore::MidiPlayEvent& ev)
      {
      if (rFifoSize == EVENT_FIFO_SIZE) {
            printf("event synti->gui  fifo overflow\n");
            return;
            }
      rFifo[rFifoWindex] = ev;
      rFifoWindex = (rFifoWindex + 1) % EVENT_FIFO_SIZE;
      ++rFifoSize;
      write(writeFd, "x", 1);  // wakeup GUI
      }

//---------------------------------------------------------
//   readEvent
//    read event from synti gui
//---------------------------------------------------------

MusECore::MidiPlayEvent MessGui::readEvent()
      {
      MusECore::MidiPlayEvent ev = wFifo[wFifoRindex];
      wFifoRindex = (wFifoRindex + 1) % EVENT_FIFO_SIZE;
      --wFifoSize;
      return ev;
      }

