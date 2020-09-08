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
#include <vector>
#include <atomic>
#include "lock_free_buffer.h"
#include "sync.h"
#include "evdata.h"
#include "latency_info.h"

#include <QString>


namespace MusECore {


// Forward declarations:
class Xml;

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
      
      // The desired event buffer when putting an event.
      enum EventBufferType
      {
        // Playback queue for events that are scheduled by the playback engine.
        PlaybackBuffer=0,
        // User queue for non-playback events such as from GUI controls or external hw.
        UserBuffer=1
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
      int _tmpRecordCount[MusECore::MUSE_MIDI_CHANNELS + 1];
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
      
      // For processing system exclusive input chunks.
      SysExInputProcessor _sysExInProcessor;
      // For processing system exclusive output chunks.
      SysExOutputProcessor _sysExOutProcessor;
      // Holds all non-realtime events while the sysex processor is in the Sending state.
      // The official midi specs say only realtime messages can be mingled in the middle of a sysex.
      std::vector<MidiPlayEvent> *_sysExOutDelayedEvents;
      
      MPEventList _stuckNotes; // Playback: Pending note-offs put directly to the device corresponding to currently playing notes
      
      // Playback IPC buffers. For playback events ONLY. Any thread can use this.
      LockFreeMPSCRingBuffer<MidiPlayEvent> *_playbackEventBuffers;
      // Various IPC buffers. NOT for playback events. Any thread can use this.
      LockFreeMPSCRingBuffer<MidiPlayEvent> *_userEventBuffers;
      
      // Recording fifos. To speed up processing, one per channel plus one special system 'channel' for channel-less events like sysex.
      MidiRecFifo _recordFifo[MusECore::MUSE_MIDI_CHANNELS + 1];   

      // To hold current output program, and RPN/NRPN parameter numbers and values.
      MidiOutputParams _curOutParamNums[MusECore::MUSE_MIDI_CHANNELS];
      
      RouteList _inRoutes, _outRoutes;
      
      // Fifo holds brief history of incoming external clock messages.
      // Timestamped with both tick and frame so that pending play events can
      //  be scheduled by frame.
      // The audio thread processes this fifo and clears it.
      LockFreeBuffer<ExtMidiClock> *_extClockHistoryFifo;
      
      // Holds latency computations each cycle.
      TrackLatencyInfo _captureLatencyInfo;
      TrackLatencyInfo _playbackLatencyInfo;
      
      // Returns the number of frames to shift forward output event scheduling times when putting events
      //  into the eventFifos. This is not quite the same as latency (requiring a backwards shift)
      //  although its requirement is a result of the latency.
      // For any driver running in the audio thread (Jack midi, synth, metro etc) this value typically 
      //  will equal one segment size.
      // For drivers running in their own thread (ALSA, OSC input) this will typically be near zero:
      //  1 ms for ALSA given a standard sequencer timer f = 1000Hz, or near zero for OSC input.
      inline virtual unsigned int pbForwardShiftFrames() const { return 0; }

      // Various IPC buffers. Any thread can use this.
      inline LockFreeMPSCRingBuffer<MidiPlayEvent> *eventBuffers(EventBufferType bufferType) 
      { 
        switch(bufferType)
        {
          case PlaybackBuffer:
            return _playbackEventBuffers;
            
          case UserBuffer:
            return _userEventBuffers;
        }
        return _userEventBuffers;
      } 
      
      // Informs the device to clear (flush) the outEvents and event buffers. 
      // To be called by audio thread only. Typically from the device's handleStop routine.
      void setStopFlag(bool flag) { _stopFlag.store(flag); }
      // Returns whether the device is flagged to clear the outEvents and event buffers.
      // To be called from the device's thread in the process routine.
      bool stopFlag() const { return _stopFlag.load(); }
      
      void init();
      
   public:
      MidiDevice();
      MidiDevice(const QString& name);
      virtual ~MidiDevice();

      inline SysExInputProcessor* sysExInProcessor() { return &_sysExInProcessor; }
      inline SysExOutputProcessor* sysExOutProcessor() { return &_sysExOutProcessor; }
      
      virtual MidiDeviceType deviceType() const = 0;
      virtual QString deviceTypeString() const;
      
      // The meaning of the returned pointer depends on the driver.
      // For Jack it returns the address of a Jack port, for ALSA it return the address of a snd_seq_addr_t.
      inline virtual void* inClientPort() { return 0; }
      inline virtual void* outClientPort() { return 0; }

      // These three are generally for ALSA.
      inline virtual void setAddressClient(int) { }
      inline virtual void setAddressPort(int) { }
      // We (ab)use the ALSA value SND_SEQ_ADDRESS_UNKNOWN to
      //  mean 'unavailable' if either client and port equal it.
      inline virtual bool isAddressUnknown() const { return true; }

      virtual QString open() = 0;
      virtual void close() = 0;
      virtual void writeRouting(int, Xml&) const {  };

      RouteList* inRoutes()   { return &_inRoutes; }
      RouteList* outRoutes()   { return &_outRoutes; }
      bool noInRoute() const   { return _inRoutes.empty();  }
      bool noOutRoute() const  { return _outRoutes.empty(); }
      
      inline const QString& name() const      { return _name; }
      // setName can be overloaded to do other things like setting port names, while setNameText just sets the text.
      inline virtual void setName(const QString& s)   { _name = s; }
      // setNameText just sets the text, while setName can be overloaded to do other things like setting port names.
      inline void setNameText(const QString& s)  { _name = s; }
      
      inline int midiPort() const             { return _port; }
      void setPort(int p);              

      inline int rwFlags() const              { return _rwFlags; }
      inline int openFlags() const            { return _openFlags; }
      inline void setOpenFlags(int val)       { _openFlags = val; }
      inline void setrwFlags(int val)         { _rwFlags = val; }
      inline const QString& state() const     { return _state; }
      inline void setState(const QString& s)  { _state = s; }

      inline virtual bool isSynti() const     { return false; }
      inline virtual int selectRfd()          { return -1; }
      inline virtual int selectWfd()          { return -1; }
      inline virtual int bytesToWrite()       { return 0; }
      inline virtual void flush()             {}
      inline virtual void processInput()      {}
      inline virtual void discardInput()      {}

      // Event time and tick must be set by caller beforehand.
      virtual void recordEvent(MidiRecordEvent&);

      // Add a stuck note. Returns false if event cannot be delivered.
      virtual bool addStuckNote(const MidiPlayEvent& ev) { _stuckNotes.add(ev); return true; }
      // Put either a playback or a user event. Returns true if event cannot be delivered.
      virtual bool putEvent(const MidiPlayEvent& ev, 
                                LatencyType latencyType, 
                                EventBufferType bufferType = UserBuffer);
      MidiOutputParams* curOutParamNums(int chan) { return &_curOutParamNums[chan]; }
      void resetCurOutParamNums(int chan = -1); // Reset channel's current parameter numbers to -1. All channels if chan = -1.
      
      virtual void handleStop();  
      virtual void handleSeek();
      
      virtual void processStuckNotes();
      
      virtual void collectMidiEvents() {}   
      // Process midi events. The frame is used by devices such as ALSA 
      //  that require grabbing a timestamp as early as possible and
      //  passing it along to all the devices. The other devices don't
      //  require the frame since they 'compose' a buffer based on the 
      //  frame at cycle start.
      virtual void processMidi(unsigned int /*curFrame*/ = 0) {}

      void beforeProcess();
      void afterProcess();
      int tmpRecordCount(const unsigned int ch)     { return _tmpRecordCount[ch]; }
      MidiRecFifo& recordEvents(const unsigned int ch) { return _recordFifo[ch]; }
      bool sysexFIFOProcessed()                     { return _sysexFIFOProcessed; }
      void setSysexFIFOProcessed(bool v)            { _sysexFIFOProcessed = v; }
      
      static const int extClockHistoryCapacity;
      LockFreeBuffer<ExtMidiClock> *extClockHistory() { return _extClockHistoryFifo; }
      void midiClockInput(unsigned int frame);


      // Initializes this track's latency information in preparation for a latency scan.
      virtual void prepareLatencyScan();
      // Returns the latency of a given capture or playback port of the device.
      inline virtual unsigned int portLatency(void* /*port*/, bool /*capture*/) const { return 0; }
      // The contribution to latency by the device's own members (midi effect rack, Jack ports etc).
      // A midi device can contain both an input and an output. The 'capture' parameter determines which one.
      inline virtual float selfLatencyMidi(int /*channel*/, bool /*capture*/) const { return 0.0f; }
      // The cached worst latency of all the contributions from the track's own members (audio effect rack, etc)
      //  plus any port latency if applicable.
      virtual float getWorstSelfLatencyMidi(bool capture);
      // Whether this track (and the branch it is in) can force other parallel branches to
      //  increase their latency compensation to match this one.
      // If false, this branch will NOT disturb other parallel branches' compensation,
      //  intead only allowing compensation UP TO the worst case in other branches.
      virtual bool canDominateOutputLatencyMidi(bool capture) const;
      virtual bool canDominateInputLatencyMidi(bool capture) const;
      // Whether this track (and the branch it is in) can force other parallel branches to
      //  increase their latency compensation to match this one - IF this track is an end-point
      //  and the branch allows domination.
      // If false, this branch will NOT disturb other parallel branches' compensation,
      //  intead only allowing compensation UP TO the worst case in other branches.
      virtual bool canDominateEndPointLatencyMidi(bool capture) const;
      // Whether this track and its branch can correct for latency, not just compensate.
      inline virtual bool canCorrectOutputLatencyMidi() const { return false; }
      // Whether the track can pass latency values through, the SAME as if record monitor is
      //  supported and on BUT does not require record monitor support.
      // This is for example in the metronome MetronomeSynthI, since it is unique in that it
      //  can correct its own latency unlike other synths, but it does not 'pass through'
      //  the latency values to what drives it like other synths.
      virtual bool canPassThruLatencyMidi(bool capture) const;
      // Whether any of the connected output routes are effectively connected.
      // That means track is not off, track is monitored where applicable, etc,
      //   ie. signal can actually flow.
      // For Wave Tracks for example, asks whether the track is an end-point from the view of the input side.
      virtual bool isLatencyInputTerminalMidi(bool capture);
      // Whether any of the connected output routes are effectively connected.
      // That means track is not off, track is monitored where applicable, etc,
      //   ie. signal can actually flow.
      // For Wave Tracks for example, asks whether the track is an end-point from the view of the playback side.
      virtual bool isLatencyOutputTerminalMidi(bool capture);

      virtual TrackLatencyInfo& getDominanceInfoMidi(bool capture, bool input);
      virtual TrackLatencyInfo& getDominanceLatencyInfoMidi(bool capture, bool input);
      // The finalWorstLatency is the grand final worst-case latency, of any output track or open branch,
      //  determined in the complete getDominanceLatencyInfo() scan.
      // The callerBranchLatency is the inherent branch latency of the calling track, or zero if calling from
      //  the very top outside of the branch heads (outside of output tracks or open branches).
      // The callerBranchLatency is accumulated as setCorrectionLatencyInfo() is called on each track
      //  in a branch of the graph.
      virtual TrackLatencyInfo& setCorrectionLatencyInfoMidi(bool capture, bool input, float finalWorstLatency, float callerBranchLatency = 0.0f);
      virtual TrackLatencyInfo& getLatencyInfoMidi(bool capture, bool input);
      // Used during latency compensation processing. When analyzing in 'reverse' this mechansim is
      //  needed only to equalize the timing of all the AudioOutput tracks.
      // It is applied as a direct offset in the latency delay compensator in getData().
      virtual unsigned long latencyCompWriteOffsetMidi(bool capture) const;
      virtual void setLatencyCompWriteOffsetMidi(float worstCase, bool capture);
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
};

extern void initMidiDevices();
extern bool filterEvent(const MEvent& event, int type, bool thru);

} // namespace MusECore

namespace MusEGlobal {
extern MusECore::MidiDeviceList midiDevices;
}

#endif

