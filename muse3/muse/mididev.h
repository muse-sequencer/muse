//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: mididev.h,v 1.3.2.4 2009/04/04 01:49:50 terminator356 Exp $
//
//  (C) Copyright 2000 Werner Schweer (ws@seh.de)
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

#ifndef __MIDIDEV_H__
#define __MIDIDEV_H__

#include <list>

#include "mpevent.h"
#include "route.h"
#include "globaldefs.h"

#include <QString>


namespace MusECore {

class Xml;
class PendingOperationList;

//---------------------------------------------------------
//   MidiDevice
//---------------------------------------------------------

class MidiDevice {
      // Used for multiple reads of fifos during process.
      int _tmpRecordCount[MIDI_CHANNELS + 1];
      bool _sysexFIFOProcessed;

   protected:
      QString _name;
      int _port;         // connected to midi port; -1 - not connected
      int _rwFlags;      // possible open flags, 1 write, 2 read, 3 rw
      int _openFlags;    // configured open flags
      bool _readEnable;  // set when opened/closed.
      bool _writeEnable; //
      
      bool _sysexReadingChunks;
      
      MPEventList _stuckNotes;
      MPEventList _playEvents;
      
      // Fifo for midi events sent from gui direct to midi port:
      MidiFifo eventFifo;  
      // Recording fifos. To speed up processing, one per channel plus one special system 'channel' for channel-less events like sysex.
      MidiRecFifo _recordFifo[MIDI_CHANNELS + 1];   

      volatile bool stopPending;         
      volatile bool seekPending;
      
      RouteList _inRoutes, _outRoutes;
      
      void init();
      virtual bool putMidiEvent(const MidiPlayEvent&) = 0;
      virtual void processStuckNotes();

   public:
      enum MidiDeviceType { ALSA_MIDI=0, JACK_MIDI=1, SYNTH_MIDI=2 };
      
      MidiDevice();
      MidiDevice(const QString& name);
      virtual ~MidiDevice() {}

      virtual MidiDeviceType deviceType() const = 0;
      virtual QString deviceTypeString();
      
      virtual void* inClientPort() { return 0; }
      virtual void* outClientPort() { return 0; }
      // These two are generally for ALSA.
      virtual void setAddressClient(int) { }
      virtual void setAddressPort(int) { }
      
      virtual QString open() = 0;
      virtual void close() = 0;
      virtual void writeRouting(int, Xml&) const {  };

      RouteList* inRoutes()   { return &_inRoutes; }
      RouteList* outRoutes()   { return &_outRoutes; }
      bool noInRoute() const   { return _inRoutes.empty();  }
      bool noOutRoute() const  { return _outRoutes.empty(); }
      
      const QString& name() const      { return _name; }
      // setName can be overloaded to do other things like setting port names, while setNameText just sets the text.
      virtual void setName(const QString& s)   { _name = s; }
      // setNameText just sets the text, while setName can be overloaded to do other things like setting port names.
      void setNameText(const QString& s)  { _name = s; }
      
      int midiPort() const             { return _port; }
      void setPort(int p);              

      int rwFlags() const              { return _rwFlags; }
      int openFlags() const            { return _openFlags; }
      void setOpenFlags(int val)       { _openFlags = val; }
      void setrwFlags(int val)         { _rwFlags = val; }

      virtual bool guiVisible() const { return false; }
      virtual void showGui(bool)    { }
      virtual bool hasGui() const     { return false; }
      virtual bool nativeGuiVisible() const { return false; }
      virtual void showNativeGui(bool)    { }
      virtual bool hasNativeGui() const     { return false; }
      virtual void getGeometry(int* x, int* y, int* w, int* h) const { *x = 0; *y = 0; *w = 0; *h = 0; }
      virtual void setGeometry(int /*x*/, int /*y*/, int /*w*/, int /*h*/) { }
      virtual void getNativeGeometry(int* x, int* y, int* w, int* h) const { *x = 0; *y = 0; *w = 0; *h = 0; }
      virtual void setNativeGeometry(int /*x*/, int /*y*/, int /*w*/, int /*h*/) { }

      
      virtual bool isSynti() const     { return false; }
      virtual int selectRfd()          { return -1; }
      virtual int selectWfd()          { return -1; }
      virtual int bytesToWrite()       { return 0; }
      virtual void flush()             {}
      virtual void processInput()      {}
      virtual void discardInput()      {}

      virtual void recordEvent(MidiRecordEvent&);

      // Schedule an event for playback. Returns false if event cannot be delivered.
      virtual bool addScheduledEvent(const MidiPlayEvent& ev) { _playEvents.add(ev); return true; }
      // Add a stuck note. Returns false if event cannot be delivered.
      virtual bool addStuckNote(const MidiPlayEvent& ev) { _stuckNotes.add(ev); return true; }
      // Put an event for immediate playback.
      virtual bool putEvent(const MidiPlayEvent&);
      // This method will try to putEvent 'tries' times, waiting 'delayUs' microseconds between tries.
      // Since it waits, it should not be used in RT or other time-sensitive threads. p4.0.15
      bool putEventWithRetry(const MidiPlayEvent&, int tries = 2, long delayUs = 50000);  // 2 tries, 50 mS by default.
      
      virtual void handleStop();  
      virtual void handleSeek();
      
      virtual void collectMidiEvents() {}   
      virtual void processMidi() {}

      void beforeProcess();
      void afterProcess();
      int tmpRecordCount(const unsigned int ch)     { return _tmpRecordCount[ch]; }
      MidiRecFifo& recordEvents(const unsigned int ch) { return _recordFifo[ch]; }
      bool sysexFIFOProcessed()                     { return _sysexFIFOProcessed; }
      void setSysexFIFOProcessed(bool v)            { _sysexFIFOProcessed = v; }
      bool sysexReadingChunks() { return _sysexReadingChunks; }
      void setSysexReadingChunks(bool v) { _sysexReadingChunks = v; }
      bool sendNullRPNParams(unsigned time, int port, int chan, bool);
      };

//---------------------------------------------------------
//   MidiDeviceList
//---------------------------------------------------------

typedef std::list<MidiDevice*>::iterator iMidiDevice;
typedef std::list<MidiDevice*>::const_iterator ciMidiDevice;

class MidiDeviceList : public std::list<MidiDevice*> 
{
   public:
      void add(MidiDevice* dev);
      void remove(MidiDevice* dev);
      MidiDevice* find(const QString& name, int typeHint = -1);

      iterator find(const MidiDevice* dev)
      {
        for(iterator i = begin(); i != end(); ++i)
          if(*i == dev)
            return i;
        return end();
      }

      const_iterator find(const MidiDevice* dev) const
      {
        for(const_iterator i = begin(); i != end(); ++i)
          if(*i == dev)
            return i;
        return end();
      }
      
      bool contains(const MidiDevice* dev) const
      {
        for(const_iterator i = begin(); i != end(); ++i)
          if(*i == dev)
            return true;
        return false;
      }
      
      void addOperation(MidiDevice* dev, PendingOperationList& ops);
};

extern void initMidiDevices();
extern bool filterEvent(const MEvent& event, int type, bool thru);

} // namespace MusECore

namespace MusEGlobal {
extern MusECore::MidiDeviceList midiDevices;
}

#endif

