//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: jackmidi.h,v 1.1.1.1 2010/01/27 09:06:43 terminator356 Exp $
//  (C) Copyright 1999-2010 Werner Schweer (ws@seh.de)
//=========================================================

#ifndef __JACKMIDI_H__
#define __JACKMIDI_H__

#include <config.h>

#include "mididev.h"
class MidiFifo;

// Turn on to show multiple devices, work in progress, 
//  not working fully yet, can't seem to connect...
/// #define JACK_MIDI_SHOW_MULTIPLE_DEVICES

/* jack-midi channels */
// Sorry, only one MusE Jack midi port for now.
//#define JACK_MIDI_CHANNELS 32
#define JACK_MIDI_CHANNELS 1

/* jack-midi buffer size */
//#define JACK_MIDI_BUFFER_SIZE 32

/*
typedef struct {
  int  give;
  int  take;
  // 32 parallel midi events, where each event contains three
  //  midi-bytes and one busy-byte 
  char buffer[4 * JACK_MIDI_BUFFER_SIZE];
} muse_jack_midi_buffer;
*/

//---------------------------------------------------------
//   MidiJackDevice
//---------------------------------------------------------

class MidiJackDevice : public MidiDevice {
   public:
      int adr;

   private:
      // fifo for midi events sent from gui
      // direct to midi port:
      MidiFifo eventFifo;

      virtual QString open();
      virtual void close();
      //bool putEvent(int*);
      
      void processEvent(int /*port*/, const MidiPlayEvent&);
      // Port is not midi port, it is the port(s) created for MusE.
      bool queueEvent(int /*port*/, const MidiPlayEvent&);
      //bool queueEvent(const MidiPlayEvent&);
      
      virtual bool putMidiEvent(const MidiPlayEvent&);
      //bool sendEvent(const MidiPlayEvent&);

   public:
      MidiJackDevice() {}
      MidiJackDevice(const int&, const QString& name);
      void processMidi();
      virtual ~MidiJackDevice() {}
      //virtual int selectRfd();
      //virtual int selectWfd();
      //virtual void processInput();
      
      virtual bool putEvent(const MidiPlayEvent&);
      };

extern bool initMidiJack();
//extern int jackSelectRfd();
//extern int jackSelectWfd();
//extern void jackProcessMidiInput();
//extern void jackScanMidiPorts();

#endif


