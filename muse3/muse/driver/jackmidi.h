//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: jackmidi.h,v 1.1.1.1 2010/01/27 09:06:43 terminator356 Exp $
//  (C) Copyright 1999-2010 Werner Schweer (ws@seh.de)
//  (C) Copyright 2011 Tim E. Real (terminator356 on users dot sourceforge dot net)
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

#ifndef __JACKMIDI_H__
#define __JACKMIDI_H__

//#include <config.h>

#include <map>

#include <jack/jack.h>
#include <jack/midiport.h>

#include "mididev.h"
#include "route.h"
#include "mpevent.h"

class QString;
//class MidiFifo;
//class RouteList;

namespace MusECore {
class MidiRecordEvent;
class MidiPlayEvent;
class Xml;

// It appears one client port per remote port will be necessary.
// Jack doesn't seem to like manipulation of non-local ports buffers.
//#define JACK_MIDI_USE_MULTIPLE_CLIENT_PORTS

//---------------------------------------------------------
//   MidiJackDevice
//---------------------------------------------------------

class MidiJackDevice : public MidiDevice {
   private:
      
      jack_port_t* _in_client_jackport;
      jack_port_t* _out_client_jackport;
      
      MPEventList _outPlaybackEvents;
      MPEventList _outUserEvents;
      
      //RouteList _routes;
      
      virtual QString open();
      virtual void close();
      //bool putEvent(int*);
      
      // Return true if successful
      // evBuffer is the Jack buffer.
      bool processEvent(const MidiPlayEvent&, void* evBuffer);
      // Port is not midi port, it is the port(s) created for MusE.
      // evBuffer is the Jack buffer.
      bool queueEvent(const MidiPlayEvent&, void* evBuffer);
      
      //virtual bool putMidiEvent(const MidiPlayEvent&);  // REMOVE Tim.
      //bool sendEvent(const MidiPlayEvent&);
      
      void eventReceived(jack_midi_event_t*);

   protected:
      // Returns the number of frames to shift forward output event scheduling times when putting events
      //  into the eventFifos.
      virtual unsigned int pbForwardShiftFrames() const;
     
   public:
      MidiJackDevice(const QString& name); 
      virtual ~MidiJackDevice(); 
      
      static MidiDevice* createJackMidiDevice(QString name = "", int rwflags = 3); // 1:Writable 2: Readable 3: Writable + Readable
      virtual inline MidiDeviceType deviceType() const { return JACK_MIDI; } 
      virtual void setName(const QString&);
      
      //virtual void handleStop();  
      //virtual void handleSeek();
      virtual void processMidi(unsigned int curFrame = 0);
      
      virtual void recordEvent(MidiRecordEvent&);
      
      virtual void collectMidiEvents();
      
      // The meaning of the returned pointer depends on the driver.
      // For Jack it returns the address of a Jack port, for ALSA it return the address of a snd_seq_addr_t.
      virtual void* inClientPort()  { return (void*)  _in_client_jackport; }
      virtual void* outClientPort() { return (void*) _out_client_jackport; }
      
      virtual void writeRouting(int, Xml&) const;
      };

extern bool initMidiJack();

} // namespace MusECore

#endif


