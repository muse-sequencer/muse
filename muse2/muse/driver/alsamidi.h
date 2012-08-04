//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: alsamidi.h,v 1.2 2004/01/14 09:06:43 wschweer Exp $
//  (C) Copyright 2001 Werner Schweer (ws@seh.de)
//  (C) Copyright 2011 Tim E. Real (terminator356 on sourceforge)
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

#ifndef __ALSAMIDI_H__
#define __ALSAMIDI_H__

#include <config.h>
#include <alsa/asoundlib.h>

#include "mpevent.h"
#include "mididev.h"

namespace MusECore {

class Xml;

//---------------------------------------------------------
//   MidiAlsaDevice
//---------------------------------------------------------

class MidiAlsaDevice : public MidiDevice {
   public:
      snd_seq_addr_t adr;

   private:
      // Special for ALSA midi device: Play event list is processed in the ALSA midi sequencer thread.
      // Need this FIFO, to decouple from audio thread which adds events to the list.       
      //MidiFifo playEventFifo;  
      //MidiFifo stuckNotesFifo;  
      
      virtual QString open();
      virtual void close();
      virtual void processInput()  {}
      virtual int selectRfd()      { return -1; }
      virtual int selectWfd();

      bool putEvent(snd_seq_event_t*);
      virtual bool putMidiEvent(const MidiPlayEvent&);

   public:
      MidiAlsaDevice(const snd_seq_addr_t&, const QString& name);
      virtual ~MidiAlsaDevice() {}
      
      virtual void* inClientPort() { return (void*)&adr; }     // For ALSA midi, in/out client ports are the same.
      virtual void* outClientPort() { return (void*)&adr; }    // That is, ALSA midi client ports can be both r/w.
      
      virtual void writeRouting(int, Xml&) const;
      virtual inline int deviceType() const { return ALSA_MIDI; } 
      // Schedule an event for playback. Returns false if event cannot be delivered.
      //virtual bool addScheduledEvent(const MidiPlayEvent& ev) { return !playEventFifo.put(ev); }
      // Add a stuck note. Returns false if event cannot be delivered.
      //virtual bool addStuckNote(const MidiPlayEvent& ev) { return !stuckNotesFifo.put(ev); }
      // Play all events up to current frame.
      virtual void processMidi();
      //virtual void handleStop();
      //virtual void handleSeek();
      };

extern bool initMidiAlsa();
extern void exitMidiAlsa();
extern int alsaSelectRfd();
extern int alsaSelectWfd();
extern void alsaProcessMidiInput();
extern void alsaScanMidiPorts();
extern void setAlsaClientName(const char*);

} // namespace MusECore

#endif


