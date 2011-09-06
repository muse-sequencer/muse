//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: alsamidi.h,v 1.2 2004/01/14 09:06:43 wschweer Exp $
//  (C) Copyright 2001 Werner Schweer (ws@seh.de)
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
      virtual inline int deviceType() const { return ALSA_MIDI; } 
      };

extern bool initMidiAlsa();
extern int alsaSelectRfd();
extern int alsaSelectWfd();
extern void alsaProcessMidiInput();
extern void alsaScanMidiPorts();

#endif


