//=========================================================
//  MusE
//  Linux Music Editor
//    software synthesizer helper library
//    $Id: gui.h,v 1.5 2005/05/11 14:18:48 wschweer Exp $
//
//  (C) Copyright 2004 Werner Schweer (ws@seh.de)
//=========================================================

#ifndef __SYNTH_GUI_H__
#define __SYNTH_GUI_H__

#include "midievent.h"

const int EVENT_FIFO_SIZE = 256;
class QWidget;

//---------------------------------------------------------
//   MessGui
//    manage IO from synti-GUI to Host
//---------------------------------------------------------

class MessGui {
      int writeFd;

      // Event Fifo  synti -> GUI
      MidiEvent rFifo[EVENT_FIFO_SIZE];
      volatile int rFifoSize;
      int rFifoWindex;
      int rFifoRindex;

      // Event Fifo  GUI -> synti
      MidiEvent wFifo[EVENT_FIFO_SIZE];
      volatile int wFifoSize;
      int wFifoWindex;
      int wFifoRindex;

   protected:
      int readFd;
      void readMessage();
      void sendEvent(const MidiEvent& ev);
      void sendController(int ch, int idx, int val);
      void sendSysex(unsigned char*, int);

      virtual void processEvent(const MidiEvent&) {};

   public:
      MessGui();
      virtual ~MessGui();

      void writeEvent(const MidiEvent&);
      int fifoSize() const { return wFifoSize; }
      MidiEvent readEvent();
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

