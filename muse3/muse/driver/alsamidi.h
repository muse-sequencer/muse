//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: alsamidi.h,v 1.2 2004/01/14 09:06:43 wschweer Exp $
//  (C) Copyright 2001 Werner Schweer (ws@seh.de)
//  (C) Copyright 2011, 2015 Tim E. Real (terminator356 on sourceforge)
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

#include "config.h"

#ifdef ALSA_SUPPORT

#include <alsa/asoundlib.h>

#include "mpevent.h"
#include "mididev.h"

#endif // ALSA_SUPPORT

namespace MusECore {

#ifdef ALSA_SUPPORT

class Xml;

//---------------------------------------------------------
//   MidiAlsaDevice
//---------------------------------------------------------

class MidiAlsaDevice : public MidiDevice {
   public:
      snd_seq_addr_t adr;

   private:
      // Fifo for midi playback events sent from audio thread:
      // The audio thread will gather the events in _playEvents for the 
      //  convenience of its sorting, then dump them to this FIFO so that 
      //  a driver or device may read it, possibly from another thread (ALSA driver).
      SeqMPEventList _outPlaybackEvents;
      SeqMPEventList _outUserEvents;
     
      // Return false if event is delivered.
      bool processEvent(const MidiPlayEvent& ev);
      
      virtual void processInput()  {}
      virtual int selectRfd()      { return -1; }
      virtual int selectWfd();

      bool putAlsaEvent(snd_seq_event_t*);
      
   public:
      MidiAlsaDevice(const snd_seq_addr_t&, const QString& name);
      virtual ~MidiAlsaDevice();
      
      static MidiDevice* createAlsaMidiDevice(QString name = "", int rwflags = 3); // 1:Writable 2: Readable 3: Writable + Readable 
      static void dump(const snd_seq_event_t* ev);
      
      virtual QString open();
      virtual void close();
      
      // The meaning of the returned pointer depends on the driver.
      // For Jack it returns the address of a Jack port, for ALSA it return the address of a snd_seq_addr_t.
      // For ALSA midi, in/out client ports are the same. That is, ALSA midi client ports can be both r/w.
      virtual void* inClientPort() { return (void*)&adr; }
      virtual void* outClientPort() { return (void*)&adr; }
      
      virtual void writeRouting(int, Xml&) const;
      virtual inline MidiDeviceType deviceType() const { return ALSA_MIDI; } 
      
      // Play all events up to current frame.
      virtual void processMidi(unsigned int curFrame = 0);

      virtual void setAddressClient(int client) { adr.client = client; }
      virtual void setAddressPort(int port) { adr.port = port; }
      // We (ab)use the ALSA value SND_SEQ_ADDRESS_UNKNOWN to mean 'unavailable' - if BOTH client and port equal it.
      virtual bool isAddressUnknown() const { return adr.client == SND_SEQ_ADDRESS_UNKNOWN || adr.port == SND_SEQ_ADDRESS_UNKNOWN; }
      };

#endif // ALSA_SUPPORT

extern bool initMidiAlsa();
extern void exitMidiAlsa();
extern int alsaSelectRfd();
extern int alsaSelectWfd();
extern void alsaProcessMidiInput();
extern void alsaScanMidiPorts();
extern void setAlsaClientName(const char*);


} // namespace MusECore

#endif


