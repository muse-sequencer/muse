//=========================================================
//  MusE
//  Linux Music Editor
//    software synthesizer helper library
//    $Id: gui.h,v 1.4 2004/06/19 09:50:37 wschweer Exp $
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

#ifndef __SYNTH_GUI_H__
#define __SYNTH_GUI_H__

#include <QObject>
#include <QWidget>

#include "mpevent.h"

const int EVENT_FIFO_SIZE = 256;

class SignalGui : public QObject
{
  Q_OBJECT

  public:
    void sendSignal();

  // The magic of automatic connection types:
  // ----------------------------------------
  // Sometimes this will be a direct connection (when sending initialization
  //  data from GUI thread for example), and sometimes this will be a queued
  //  connection (when sending user-adjusted controller values from the realtime
  //  audio thread for example).
  // Tested OK so far in debugger. It decouples just fine by itself when needed !
  //
  // Quote from http://wiki.qt.io/Threads_Events_QObjects:
  // "...the current Qt documentation is simply wrong when it states:
  //  ' Auto Connection (default) The behavior is the same as the Direct Connection,
  //     if the emitter and receiver are in the same thread. The behavior is the same
  //     as the Queued Connection, if the emitter and receiver are in different threads.'
  //  because the emitter object's thread affinity does not matter."
  signals:
    void wakeup(int);
};

//---------------------------------------------------------
//   MessGui
//    manage IO from synti-GUI to Host
//---------------------------------------------------------

class MessGui {
      // Event Fifo  synti -> GUI
      MusECore::MidiPlayEvent rFifo[EVENT_FIFO_SIZE];
      volatile int rFifoSize;
      int rFifoWindex;
      int rFifoRindex;

      // Event Fifo  GUI -> synti
      MusECore::MidiPlayEvent wFifo[EVENT_FIFO_SIZE];
      volatile int wFifoSize;
      int wFifoWindex;
      int wFifoRindex;

   protected:
      SignalGui* guiSignal;
      virtual void readMessage();
      void sendEvent(const MusECore::MidiPlayEvent& ev);
      void sendController(int,int,int);
      void sendSysex(unsigned char*, int);

      virtual void processEvent(const MusECore::MidiPlayEvent&) {};

   public:
      MessGui();
      virtual ~MessGui();

      void writeEvent(const MusECore::MidiPlayEvent&);
      int fifoSize() const { return wFifoSize; }
      MusECore::MidiPlayEvent readEvent();
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

