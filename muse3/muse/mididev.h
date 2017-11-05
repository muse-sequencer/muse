//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: mididev.h,v 1.3.2.4 2009/04/04 01:49:50 terminator356 Exp $
//
//  (C) Copyright 2000 Werner Schweer (ws@seh.de)
//  (C) Copyright 2011, 2016 Tim E. Real (terminator356 on users dot sourceforge dot net)
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
// REMOVE Tim. autoconnect. Added.
#include <vector>
#include <atomic>
#include "lock_free_buffer.h"
#include "sync.h"
#include "evdata.h"
//#include <boost/lockfree/queue.hpp>

#include <QString>


namespace MusECore {

class Xml;
class PendingOperationList;

struct MidiOutputParams {
      int BANKH;
      int BANKL;
      int PROG;
      int RPNL;
      int RPNH;
      int NRPNL;
      int NRPNH;
      int DATAH;
      int DATAL;
      
      MidiOutputParams() { reset(); }
      
      void reset() { BANKH = BANKL = PROG = 0xff; RPNL = RPNH = NRPNL = NRPNH = DATAH = DATAL = -1; }
      void resetParamNums() { RPNL = RPNH = NRPNL = NRPNH = DATAH = DATAL = -1; }
      void resetPatch() { BANKH = BANKL = PROG = 0xff; }
      void setRPNL(int a)  { RPNL = a;  NRPNL = NRPNH = -1; }
      void setRPNH(int a)  { RPNH = a;  NRPNL = NRPNH = -1; }
      void setNRPNL(int a) { NRPNL = a; RPNL  = RPNH = -1; }
      void setNRPNH(int a) { NRPNH = a; RPNL  = RPNH = -1; }
      void setDATAH(int a) { DATAH = a; }
      void setDATAL(int a) { DATAL = a; }
      void setBANKH(int a) { BANKH = a;}
      void setBANKL(int a) { BANKL = a;}
      void setPROG(int a)  { PROG = a;}
      void currentProg(int *prg, int *lbank, int *hbank)
           {  if(prg) *prg=PROG&0xff; if(lbank) *lbank=BANKL&0xff; if(hbank) *hbank=BANKH&0xff;  }
      void setCurrentProg(int prg, int lbank, int hbank)
           {  PROG=prg&0xff; BANKL=lbank&0xff; BANKH=hbank&0xff;  }
};

//---------------------------------------------------------
//   MidiDevice
//---------------------------------------------------------

class MidiDevice {
   public:
      // Types of MusE midi devices.
      enum MidiDeviceType { ALSA_MIDI=0, JACK_MIDI=1, SYNTH_MIDI=2 };
      
      // IDs for the various IPC FIFOs that are used.
      enum EventFifoIds
      {
        // Playback queued events put by the audio process thread.
        PlayFifo=0,
        // Gui events put by our gui thread.
        GuiFifo=1,
        // OSC events put by the OSC thread.
        OSCFifo=2,
        // Monitor input passthrough events put by Jack devices (audio process thread).
        JackFifo=3,
        // Monitor input passthrough events put by ALSA devices (midi seq thread).
        ALSAFifo=4
      };
      
      // Describes latency of events passed to putEvent().
      enum LatencyType
      {
        // The given event's time will be used 'as is' without modification.
        NotLate = 0, 
        // The given event's time has some latency. Automatically compensate
        //  by adding an appropriate number of frames, depending on device type.
        // For example, events sent from a GUI control to the GuiFifo are usually 
        //  time-stamped in the past. For Synths and Jack midi (buffer-based systems) 
        //  we add a forward offset (one segment size). For ALSA (a poll-based system), 
        //  no offset is required. Similarly, events sent from OSC handlers to the 
        //  OSCFifo are also usually time-stamped in the past. 
        // Another example, events sent by the play scheduler to the PlayFifo are 
        //  /already/ scheduled properly for the future and need /no/ further compensation
        //  for either Jack midi or ALSA devices.
        Late = 1
      };
      
   private:
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
      QString _state;
      std::atomic<bool> _stopFlag;
      
// REMOVE Tim. autoconnect. Removed.
//       bool _sysexReadingChunks;
      // For processing system exclusive input chunks.
      SysExInputProcessor _sysExInProcessor;
      // For processing system exclusive output chunks.
      SysExOutputProcessor _sysExOutProcessor;
      // Holds all non-realtime events while the sysex processor is in the Sending state.
      // The official midi specs say only realtime messages can be mingled in the middle of a sysex.
      std::vector<MidiPlayEvent> *_sysExOutDelayedEvents;
      
      MPEventList _stuckNotes; // Playback: Pending note-offs put directly to the device corresponding to currently playing notes
//       MPEventList _playEvents;
      
// REMOVE Tim. autoconnect. Removed.
//       // Fifo for midi events sent from gui direct to midi port:
//       MidiFifo eventFifo;  
      // REMOVE Tim. autoconnect. Added.
      // Fifo for midi events sent from OSC to audio (ex. sending to DSSI synth):
      //LockFreeBuffer<MidiPlayEvent> *_osc2AudioFifo;
      // Various IPC FIFOs.
//       LockFreeMultiBuffer<MidiPlayEvent> *_eventFifos;
      //boost::lockfree::queue<MEvent, boost::lockfree::fixed_sized<true>, boost::lockfree::capacity<1024> > q;
      // Playback IPC buffers. For playback events ONLY. Any thread can use this.
      LockFreeMPSCBuffer<MidiPlayEvent, 1024> _playbackEventBuffers;
      // Various IPC buffers. NOT for playback events. Any thread can use this.
      LockFreeMPSCBuffer<MidiPlayEvent, 1024> _userEventBuffers;
      
      // Recording fifos. To speed up processing, one per channel plus one special system 'channel' for channel-less events like sysex.
      MidiRecFifo _recordFifo[MIDI_CHANNELS + 1];   

      // To hold current output program, and RPN/NRPN parameter numbers and values.
      MidiOutputParams _curOutParamNums[MIDI_CHANNELS];
      
      volatile bool stopPending;         
      volatile bool seekPending;
      
      RouteList _inRoutes, _outRoutes;
      
// REMOVE Tim. autoconnect. Added.
      // Fifo holds brief history of incoming external clock messages.
      // Timestamped with both tick and frame so that pending play events can
      //  be scheduled by frame.
      // The audio thread processes this fifo and clears it.
      LockFreeBuffer<ExtMidiClock> *_extClockHistoryFifo;
      //ExtMidiClock::ExternState _playStateExt;   // used for keeping play state in sync functions
      // Returns the number of frames to shift forward output event scheduling times when putting events
      //  into the eventFifos. This is not quite the same as latency (requiring a backwards shift)
      //  although its requirement is a result of the latency.
      // For any driver running in the audio thread (Jack midi, synth, metro etc) this value typically 
      //  will equal one segment size.
      // For drivers running in their own thread (ALSA, OSC input) this will typically be near zero:
      //  1 ms for ALSA given a standard sequencer timer f = 1000Hz, or near zero for OSC input.
      //unsigned int _pbForwardShiftFrames;
      
      // Returns the number of frames to shift forward output event scheduling times when putting events
      //  into the eventFifos. This is not quite the same as latency (requiring a backwards shift)
      //  although its requirement is a result of the latency.
      // For any driver running in the audio thread (Jack midi, synth, metro etc) this value typically 
      //  will equal one segment size.
      // For drivers running in their own thread (ALSA, OSC input) this will typically be near zero:
      //  1 ms for ALSA given a standard sequencer timer f = 1000Hz, or near zero for OSC input.
      virtual unsigned int pbForwardShiftFrames() const { return 0; }

//       // Clears the device's output events list, unique to each device.
//       virtual void clearOutEvents() = 0;
      
      void init();
// REMOVE Tim. autoconnect. Removed. Made public.
//       virtual void processStuckNotes();
      
   public:
      MidiDevice();
      MidiDevice(const QString& name);
      virtual ~MidiDevice();

      SysExInputProcessor* sysExInProcessor() { return &_sysExInProcessor; }
      SysExOutputProcessor* sysExOutProcessor() { return &_sysExOutProcessor; }
      
      virtual MidiDeviceType deviceType() const = 0;
      virtual QString deviceTypeString() const;
      
      // The meaning of the returned pointer depends on the driver.
      // For Jack it returns the address of a Jack port, for ALSA it return the address of a snd_seq_addr_t.
      virtual void* inClientPort() { return 0; }
      virtual void* outClientPort() { return 0; }

      // These three are generally for ALSA.
      virtual void setAddressClient(int) { }
      virtual void setAddressPort(int) { }
      // We (ab)use the ALSA value SND_SEQ_ADDRESS_UNKNOWN to
      //  mean 'unavailable' if either client and port equal it.
      virtual bool isAddressUnknown() const { return true; }

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
      const QString& state() const     { return _state; }
      void setState(const QString& s)  { _state = s; }

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

      // Event time and tick must be set by caller beforehand.
      virtual void recordEvent(MidiRecordEvent&);

      // Schedule an event for playback. Returns false if event cannot be delivered.
//       virtual bool addScheduledEvent(const MidiPlayEvent& ev) { _playEvents.add(ev); return true; }
//       virtual bool addScheduledEvent(const MidiPlayEvent& ev) { return _eventBuffers.put(ev); }
      // Add a stuck note. Returns false if event cannot be delivered.
      virtual bool addStuckNote(const MidiPlayEvent& ev) { _stuckNotes.add(ev); return true; }
      // Put an event for immediate playback. Returns true if event cannot be delivered.
//       virtual bool putEvent(const MidiPlayEvent&) = 0;
      // Put an event for playback. Returns true if event cannot be delivered.
      // The id parameter is of the desired FIFO. A separate FIFO is required for EACH
      //  DIFFERENT thread that sends events to the device. See the enum for possible ids.
      // In each FIFO, the event times should be frame-stamped and sorted. But if ASAP delivery 
      //  is desired, earlier event time frames (say 0) can be inter-mixed with sorted events.
      // NOTE: Avoid putting events with time >> current cycle start frame + segment size,
      //  because that will stall all processing of FIFOs until that frame has come -
      //  ie. don't accidentally put an event with frame time waaaay in the future !
      //virtual bool putEvent(const MidiPlayEvent& ev, EventFifoIds id/* = GuiFifo*/, LatencyType latencyType);
      virtual bool putPlaybackEvent(const MidiPlayEvent& ev, LatencyType latencyType);
      virtual bool putUserEvent(const MidiPlayEvent& ev, LatencyType latencyType);
// REMOVE Tim. autoconnect. Removed.
//       // This method will try to putEvent 'tries' times, waiting 'delayUs' microseconds between tries.
//       // Since it waits, it should not be used in RT or other time-sensitive threads.
//       bool putEventWithRetry(const MidiPlayEvent&, int tries = 2, long delayUs = 50000);  // 2 tries, 50 mS by default.
      MidiOutputParams* curOutParamNums(int chan) { return &_curOutParamNums[chan]; }
      void resetCurOutParamNums(int chan = -1); // Reset channel's current parameter numbers to -1. All channels if chan = -1.
      
      virtual void handleStop();  
      virtual void handleSeek();
      
// REMOVE Tim. autoconnect. Added.
      // This allows a device which processes in another thread (like ALSA) to 
      //  drain the playEvents list into a fifo that the other thread reads.
      // If the device processes in the audio thread, it is not required to use a fifo,
      //  the device can use the playEvents list directly as long as it drains the list.
      // To be called from audio thread only.
//       virtual void preparePlayEventFifo() { }
//       virtual void preparePlayEventFifo();
      // This clears the 'write' side of any fifo the device may have (like ALSA),
      //  by setting the size to zero and the write pointer equal to the read pointer.
//       virtual void clearPlayEventFifo() {}
      // Various IPC FIFOs.
//       LockFreeMultiBuffer<MidiPlayEvent> *eventFifos() { return _eventFifos; } 
      
      // Playback IPC buffers. For playback events ONLY. Any thread can use this.
      LockFreeMPSCBuffer<MidiPlayEvent, 1024> *playbackEventBuffers() { return &_playbackEventBuffers; } 
      // Various IPC buffers. NOT for playback events. Any thread can use this.
      LockFreeMPSCBuffer<MidiPlayEvent, 1024> *userEventBuffers() { return &_userEventBuffers; } 
      
// REMOVE Tim. autoconnect. Added. Moved from protected.
      virtual void processStuckNotes();
      
      virtual void collectMidiEvents() {}   
      // Process midi events. The frame is used by devices such as ALSA 
      //  that require grabbing a timestamp as early as possible and
      //  passing it along to all the devices. The other devices don't
      //  require the frame since they 'compose' a buffer based on the 
      //  frame at cycle start.
      virtual void processMidi(unsigned int /*curFrame*/ = 0) {}

// REMOVE Tim. autoconnect. Added.
//       // If playing, clears all notes and flushes out any
//       //  stuck notes which were put directly to the device
//       virtual void flushMidiEvents();
      
      void beforeProcess();
      void afterProcess();
      int tmpRecordCount(const unsigned int ch)     { return _tmpRecordCount[ch]; }
      MidiRecFifo& recordEvents(const unsigned int ch) { return _recordFifo[ch]; }
      bool sysexFIFOProcessed()                     { return _sysexFIFOProcessed; }
      void setSysexFIFOProcessed(bool v)            { _sysexFIFOProcessed = v; }
      // Informs the device to clear (flush) the outEvents and event buffers. 
      // To be called by audio thread only. Typically from the device's handleStop routine.
      void setStopFlag(bool flag) { _stopFlag.store(flag); }
      // Returns whether the device is flagged to clear the outEvents and event buffers.
      // To be called from the device's thread in the process routine.
      bool stopFlag() const { return _stopFlag.load(); }
// REMOVE Tim. autoconnect. Removed.
//       bool sysexReadingChunks() { return _sysexReadingChunks; }
//       void setSysexReadingChunks(bool v) { _sysexReadingChunks = v; }
      
// REMOVE Tim. autoconnect. Added.
      static const int extClockHistoryCapacity;
      LockFreeBuffer<ExtMidiClock> *extClockHistory() { return _extClockHistoryFifo; }
      void midiClockInput(unsigned int frame); // REMOVE Tim. autoconnect. Added.
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

