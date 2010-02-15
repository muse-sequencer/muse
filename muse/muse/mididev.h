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

//---------------------------------------------------------
//   MidiDevice
//---------------------------------------------------------

class MidiDevice {
      MPEventList _stuckNotes;
      MPEventList _playEvents;
      iMPEvent _nextPlayEvent;
      ///MREventList _recordEvents;
      ///MREventList _recordEvents2;
      
      // Used for multiple reads of fifo during process.
      int _tmpRecordCount;
      
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
      // Recording fifo. 
      MidiFifo _recordFifo;
      void init();
      virtual bool putMidiEvent(const MidiPlayEvent&) = 0;

   public:
      MidiDevice();
      MidiDevice(const QString& name);
      virtual ~MidiDevice() {}

      virtual QString open() = 0;
      virtual void close() = 0;

      const QString& name() const      { return _name; }
      void setName(const QString& s)   { _name = s; }

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

      MPEventList* stuckNotes()          { return &_stuckNotes; }
      MPEventList* playEvents()          { return &_playEvents; }
      
      ///MREventList* recordEvents();
      ///void flipRecBuffer()               { _recBufFlipped = _recBufFlipped ? false : true; }
      ///bool recBufFlipped()               { return _recBufFlipped; }
      void beforeProcess();
      void afterProcess();
      int tmpRecordCount() { return _tmpRecordCount; }
      MidiFifo& recordEvents() { return _recordFifo; }
      //virtual void getEvents(unsigned /*from*/, unsigned /*to*/, int /*channel*/, MPEventList* /*dst*/);
      
      iMPEvent nextPlayEvent()           { return _nextPlayEvent; }
      void setNextPlayEvent(iMPEvent i)  { _nextPlayEvent = i; }
      bool sendNullRPNParams(int, bool);
      };

//---------------------------------------------------------
//   MidiDeviceList
//---------------------------------------------------------

typedef std::list<MidiDevice*>::iterator iMidiDevice;

class MidiDeviceList : public std::list<MidiDevice*> {
   public:
      void add(MidiDevice* dev);
      void remove(MidiDevice* dev);
      MidiDevice* find(const QString& name);
      iMidiDevice find(const MidiDevice* dev);
      };

extern MidiDeviceList midiDevices;
extern void initMidiDevices();
extern bool filterEvent(const MEvent& event, int type, bool thru);

#endif

