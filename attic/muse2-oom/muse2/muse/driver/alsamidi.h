//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: alsamidi.h,v 1.2 2004/01/14 09:06:43 wschweer Exp $
//  (C) Copyright 2001 Werner Schweer (ws@seh.de)
//=========================================================

#ifndef __ALSAMIDI_H__
#define __ALSAMIDI_H__

#include <config.h>
#include <alsa/asoundlib.h>

#include "mididev.h"

class Xml;

//---------------------------------------------------------
//   MidiAlsaDevice
//---------------------------------------------------------

class MidiAlsaDevice : public MidiDevice {
   public:
      snd_seq_addr_t adr;

   private:
      virtual QString open();
      virtual void close();
      virtual void processInput()  {}
      virtual int selectRfd()      { return -1; }
      virtual int selectWfd();

      bool putEvent(snd_seq_event_t*);
      virtual bool putMidiEvent(const MidiPlayEvent&);

   public:
      //MidiAlsaDevice() {}  // p3.3.55 Removed
      MidiAlsaDevice(const snd_seq_addr_t&, const QString& name);
      virtual ~MidiAlsaDevice() {}
      
      //virtual void* clientPort() { return (void*)&adr; }
      // p3.3.55
      virtual void* inClientPort() { return (void*)&adr; }     // For ALSA midi, in/out client ports are the same.
      virtual void* outClientPort() { return (void*)&adr; }    // That is, ALSA midi client ports can be both r/w.
      
      virtual void writeRouting(int, Xml&) const;
      virtual inline int deviceType() { return ALSA_MIDI; } 
      };

extern bool initMidiAlsa();
extern int alsaSelectRfd();
extern int alsaSelectWfd();
extern void alsaProcessMidiInput();
extern void alsaScanMidiPorts();

#endif


