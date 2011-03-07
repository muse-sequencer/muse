//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: mididev.h,v 1.3.2.4 2009/04/04 01:49:50 terminator356 Exp $
//
//  (C) Copyright 2000 Werner Schweer (ws@seh.de)
//=========================================================

#ifndef __MIDIDEV_H__
#define __MIDIDEV_H__

#include <qstring.h>
#include <list>

#include "mpevent.h"
//#include "sync.h"
#include "route.h"
#include "globaldefs.h"

//class RouteList;
class Xml;

//---------------------------------------------------------
//   MidiDevice
//---------------------------------------------------------

class MidiDevice {
      MPEventList _stuckNotes;
      MPEventList _playEvents;
      iMPEvent _nextPlayEvent;
      ///MREventList _recordEvents;
      ///MREventList _recordEvents2;
      
      // Used for multiple reads of fifos during process.
      //int _tmpRecordCount;
      int _tmpRecordCount[MIDI_CHANNELS + 1];
      bool _sysexFIFOProcessed;
      
      ///bool _recBufFlipped;
      // Holds sync settings and detection monitors.
      //MidiSyncInfo _syncInfo;

   protected:
      QString _name;
      int _port;         // connected to midi port; -1 - not connected
      int _rwFlags;      // possible open flags, 1 write, 2 read, 3 rw
      int _openFlags;    // configured open flags
      bool _readEnable;  // set when opened/closed.
      bool _writeEnable; //
      //int _sysexWriteChunk;
      //int _sysexReadChunk;
      //bool _sysexWritingChunks;
      bool _sysexReadingChunks;
      
      // Recording fifo. 
      //MidiFifo _recordFifo;
      // Recording fifos. To speed up processing, one per channel plus one special system 'channel' for channel-less events like sysex.
      MidiFifo _recordFifo[MIDI_CHANNELS + 1];
      
      RouteList _inRoutes, _outRoutes;
      
      void init();
      virtual bool putMidiEvent(const MidiPlayEvent&) = 0;

   public:
      enum { ALSA_MIDI=0, JACK_MIDI=1, SYNTH_MIDI=2 };
      
      MidiDevice();
      MidiDevice(const QString& name);
      virtual ~MidiDevice() {}

      virtual int deviceType() = 0;
      
      //virtual void* clientPort() { return 0; }
      // p3.3.55
      virtual void* inClientPort() { return 0; }
      virtual void* outClientPort() { return 0; }
      
      virtual QString open() = 0;
      virtual void close() = 0;
      virtual void writeRouting(int, Xml&) const {  };

      RouteList* inRoutes()   { return &_inRoutes; }
      RouteList* outRoutes()   { return &_outRoutes; }
      bool noInRoute() const   { return _inRoutes.empty();  }
      bool noOutRoute() const  { return _outRoutes.empty(); }
      
      const QString& name() const      { return _name; }
      virtual void setName(const QString& s)   { _name = s; }
      
      int midiPort() const             { return _port; }
      void setPort(int p)              { _port = p; }

      int rwFlags() const              { return _rwFlags; }
      int openFlags() const            { return _openFlags; }
      void setOpenFlags(int val)       { _openFlags = val; }
      void setrwFlags(int val)         { _rwFlags = val; }
      //MidiSyncInfo& syncInfo()         { return _syncInfo; }

      virtual bool isSynti() const     { return false; }
      virtual int selectRfd()          { return -1; }
      virtual int selectWfd()          { return -1; }
      virtual int bytesToWrite()       { return 0; }
      virtual void flush()             {}
      virtual void processInput()      {}
      virtual void discardInput()      {}

      virtual void recordEvent(MidiRecordEvent&);

      virtual bool putEvent(const MidiPlayEvent&);
      
      // For Jack-based devices - called in Jack audio process callback
      virtual void collectMidiEvents() {}   
      virtual void processMidi() {}

      MPEventList* stuckNotes()          { return &_stuckNotes; }
      MPEventList* playEvents()          { return &_playEvents; }
      
      ///MREventList* recordEvents();
      ///void flipRecBuffer()               { _recBufFlipped = _recBufFlipped ? false : true; }
      ///bool recBufFlipped()               { return _recBufFlipped; }
      void beforeProcess();
      void afterProcess();
      //int tmpRecordCount() { return _tmpRecordCount; }
      int tmpRecordCount(const unsigned int ch)     { return _tmpRecordCount[ch]; }
      //MidiFifo& recordEvents() { return _recordFifo; }
      MidiFifo& recordEvents(const unsigned int ch) { return _recordFifo[ch]; }
      bool sysexFIFOProcessed()                     { return _sysexFIFOProcessed; }
      void setSysexFIFOProcessed(bool v)            { _sysexFIFOProcessed = v; }
      //bool sysexWritingChunks() { return _sysexWritingChunks; }
      //void setSysexWritingChunks(bool v) { _sysexWritingChunks = v; }
      bool sysexReadingChunks() { return _sysexReadingChunks; }
      void setSysexReadingChunks(bool v) { _sysexReadingChunks = v; }
      //virtual void getEvents(unsigned /*from*/, unsigned /*to*/, int /*channel*/, MPEventList* /*dst*/);
      
      iMPEvent nextPlayEvent()           { return _nextPlayEvent; }
      void setNextPlayEvent(iMPEvent i)  { _nextPlayEvent = i; }
      bool sendNullRPNParams(int, bool);
      };

//---------------------------------------------------------
//   MidiDeviceList
//---------------------------------------------------------

typedef std::list<MidiDevice*>::iterator iMidiDevice;

class MidiDeviceList : public std::list<MidiDevice*> 
{
   public:
      void add(MidiDevice* dev);
      void remove(MidiDevice* dev);
      MidiDevice* find(const QString& name, int typeHint = -1);
      iMidiDevice find(const MidiDevice* dev);
};

extern MidiDeviceList midiDevices;
extern void initMidiDevices();
extern bool filterEvent(const MEvent& event, int type, bool thru);

#endif

