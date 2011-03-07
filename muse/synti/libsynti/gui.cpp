//=========================================================
//  MusE
//  Linux Music Editor
//    software synthesizer helper library
//    $Id: gui.cpp,v 1.5 2004/04/11 10:46:14 wschweer Exp $
//
//  (C) Copyright 2004 Werner Schweer (ws@seh.de)
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

void MessGui::sendEvent(const MidiPlayEvent& ev)
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
//      MidiPlayEvent pe(0, 0, ch, ME_CONTROLLER, idx, val);
//      sendEvent(pe);

      sendEvent(MidiPlayEvent(0, 0, ch, ME_CONTROLLER, idx, val));
      }

//---------------------------------------------------------
//   sendSysex
//---------------------------------------------------------

void MessGui::sendSysex(unsigned char* p, int n)
      {
//      MidiPlayEvent pe(0, 0, ME_SYSEX, p, n);
//      sendEvent(pe);

      sendEvent(MidiPlayEvent(0, 0, ME_SYSEX, p, n));
      }

//---------------------------------------------------------
//   writeEvent
//    send an event to synti gui
//---------------------------------------------------------

void MessGui::writeEvent(const MidiPlayEvent& ev)
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

MidiPlayEvent MessGui::readEvent()
      {
      MidiPlayEvent ev = wFifo[wFifoRindex];
      wFifoRindex = (wFifoRindex + 1) % EVENT_FIFO_SIZE;
      --wFifoSize;
      return ev;
      }

