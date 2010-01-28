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

/* jack-midi channels */
#define JACK_MIDI_CHANNELS 32
/* jack-midi buffer size */
#define JACK_MIDI_BUFFER_SIZE 32

typedef struct {
  int  give;
  int  take;
  /* 32 parallel midi events, where each event contains three
   * midi-bytes and one busy-byte */
  char buffer[4 * JACK_MIDI_BUFFER_SIZE];
} muse_jack_midi_buffer;

//---------------------------------------------------------
//   MidiJackDevice
//---------------------------------------------------------

class MidiJackDevice : public MidiDevice {
   public:
      int adr;

   private:
      virtual QString open();
      virtual void close();
      bool putEvent(int*);
      virtual bool putMidiEvent(const MidiPlayEvent&);

   public:
      MidiJackDevice() {}
      MidiJackDevice(const int&, const QString& name);
      virtual ~MidiJackDevice() {}
      virtual int selectRfd();
      virtual int selectWfd();
      virtual void processInput();
      };

extern bool initMidiJack();
extern int jackSelectRfd();
extern int jackSelectWfd();
extern void jackProcessMidiInput();
extern void jackScanMidiPorts();

#endif


