//=========================================================
//  MusE
//  Linux Music Editor
//    software synthesizer helper library
//    $Id: gui.h,v 1.4 2004/06/19 09:50:37 wschweer Exp $
//
//  (C) Copyright 2004 Werner Schweer (ws@seh.de)
//=========================================================

#ifndef __SYNTH_GUI_H__
#define __SYNTH_GUI_H__

#include "mpevent.h"

const int EVENT_FIFO_SIZE = 256;
class QWidget;

//---------------------------------------------------------
//   MessGui
//    manage IO from synti-GUI to Host
//---------------------------------------------------------

class MessGui {
      int writeFd;

      // Event Fifo  synti -> GUI
      MidiPlayEvent rFifo[EVENT_FIFO_SIZE];
      volatile int rFifoSize;
      int rFifoWindex;
      int rFifoRindex;

      // Event Fifo  GUI -> synti
      MidiPlayEvent wFifo[EVENT_FIFO_SIZE];
      volatile int wFifoSize;
      int wFifoWindex;
      int wFifoRindex;

   protected:
      int readFd;
      void readMessage();
      void sendEvent(const MidiPlayEvent& ev);
      void sendController(int,int,int);
      void sendSysex(unsigned char*, int);

      virtual void processEvent(const MidiPlayEvent&) {};

   public:
      MessGui();
      virtual ~MessGui();

      void writeEvent(const MidiPlayEvent&);
      int fifoSize() const { return wFifoSize; }
      MidiPlayEvent readEvent();
      };

//---------------------------------------------------------
//   SynthGuiCtrl
//---------------------------------------------------------

struct SynthGuiCtrl  {
      enum EditorType { SLIDER, SWITCH, COMBOBOX };
      QWidget* editor;
      QWidget* label;
      EditorType type;

      SynthGuiCtrl() {}
      SynthGuiCtrl(QWidget* w, QWidget* l, const EditorType t)
         : editor(w), label(l), type(t) {}
      };

#endif

