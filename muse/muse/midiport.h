//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: midiport.h,v 1.9.2.6 2009/11/17 22:08:22 terminator356 Exp $
//
//  (C) Copyright 1999-2004 Werner Schweer (ws@seh.de)
//=========================================================

#ifndef __MIDIPORT_H__
#define __MIDIPORT_H__

#include "globaldefs.h"
#include "sync.h"

class MidiDevice;
class MidiInstrument;
class MidiCtrlValListList;
class MidiPlayEvent;
class MidiController;
class MidiCtrlValList;
class Part;
//class MidiSyncInfo;

//---------------------------------------------------------
//   MidiPort
//---------------------------------------------------------

class MidiPort {
      MidiCtrlValListList* _controller;
      MidiDevice* _device;
      QString _state;               // result of device open
      MidiInstrument* _instrument;
      AutomationType _automationType[MIDI_CHANNELS];
      // Holds sync settings and detection monitors.
      MidiSyncInfo _syncInfo;

      void clearDevice();

   public:
      MidiPort();
      ~MidiPort();

      //
      // manipulate active midi controller
      //
      MidiCtrlValListList* controller() { return _controller; }
      int getCtrl(int ch, int tick, int ctrl) const;
      int getCtrl(int ch, int tick, int ctrl, Part* part) const;
      // Removed by T356.
      //bool setCtrl(int ch, int tick, int ctrl, int val);
      bool setControllerVal(int ch, int tick, int ctrl, int val, Part* part);
      // Can be CTRL_VAL_UNKNOWN until a valid state is set
      int lastValidHWCtrlState(int ch, int ctrl) const;
      int hwCtrlState(int ch, int ctrl) const;
      bool setHwCtrlState(int ch, int ctrl, int val);
      bool setHwCtrlStates(int ch, int ctrl, int val, int lastval);
      void deleteController(int ch, int tick, int ctrl, Part* part);

      bool guiVisible() const;
      bool hasGui() const;

      int portno() const;

      MidiDevice* device() const                { return _device; }
      const QString& state() const              { return _state; }
      void setState(const QString& s)           { _state = s; }
      void setMidiDevice(MidiDevice* dev);
      const QString& portname() const;
      MidiInstrument* instrument() const        { return _instrument; }
      void setInstrument(MidiInstrument* i)     { _instrument = i; }
      MidiController* midiController(int num) const;
      MidiCtrlValList* addManagedController(int channel, int ctrl);
      void tryCtrlInitVal(int chan, int ctl, int val);
      int limitValToInstrCtlRange(int ctl, int val);
      int limitValToInstrCtlRange(MidiController* mc, int val);
      MidiController* drumController(int ctl);
      int nullSendValue();
      void setNullSendValue(int v);

      // send events to midi device and keep track of
      // device state:
      void sendGmOn();
      void sendGsOn();
      void sendXgOn();
      void sendGmInitValues();
      void sendGsInitValues();
      void sendXgInitValues();
      void sendStart();
      void sendStop();
      void sendContinue();
      void sendSongpos(int);
      void sendClock();
      void sendSysex(const unsigned char* p, int n);
      bool sendEvent(const MidiPlayEvent&);
      AutomationType automationType(int channel) { return _automationType[channel]; }
      void setAutomationType(int channel, AutomationType t) {
            _automationType[channel] = t;
            }
      MidiSyncInfo& syncInfo() { return _syncInfo; }
      };

extern MidiPort midiPorts[MIDI_PORTS];
extern void initMidiPorts();

class QPopupMenu;
class QWidget;
//extern QPopupMenu* midiPortsPopup(QWidget*);
extern QPopupMenu* midiPortsPopup(QWidget*, int checkPort = -1);
#endif

