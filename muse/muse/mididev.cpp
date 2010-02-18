//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: mididev.cpp,v 1.10.2.6 2009/11/05 03:14:35 terminator356 Exp $
//
//  (C) Copyright 1999-2004 Werner Schweer (ws@seh.de)
//=========================================================

#include <config.h>

#include <qmessagebox.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>

#include "midictrl.h"
#include "song.h"
#include "midi.h"
#include "midiport.h"
#include "mididev.h"
#include "config.h"
#include "globals.h"
#include "audio.h"
#include "midiseq.h"
//#include "sync.h"
#include "midiitransform.h"

#ifdef MIDI_DRIVER_MIDI_SERIAL
extern void initMidiSerial();
#endif
extern bool initMidiAlsa();
extern bool initMidiJack();

MidiDeviceList midiDevices;
extern void processMidiInputTransformPlugins(MEvent&);

extern unsigned int volatile lastExtMidiSyncTick;

//---------------------------------------------------------
//   initMidiDevices
//---------------------------------------------------------

void initMidiDevices()
      {
#ifdef MIDI_DRIVER_MIDI_SERIAL
      initMidiSerial();
#endif
      if(initMidiAlsa())
          {
          QMessageBox::critical(NULL, "MusE fatal error.", "MusE failed to initialize the\n" 
                                                          "Alsa midi subsystem, check\n"
                                                          "your configuration.");
          exit(-1);
          }
      
      if(initMidiJack())
          {
          QMessageBox::critical(NULL, "MusE fatal error.", "MusE failed to initialize the\n" 
                                                          "Jack midi subsystem, check\n"
                                                          "your configuration.");
          exit(-1);
          }
      }

//---------------------------------------------------------
//   init
//---------------------------------------------------------

void MidiDevice::init()
      {
      _readEnable = false;
      _writeEnable = false;
      _rwFlags       = 3;
      _openFlags     = 3;
      _port          = -1;
      _nextPlayEvent = _playEvents.begin();
      }

//---------------------------------------------------------
//   MidiDevice
//---------------------------------------------------------

MidiDevice::MidiDevice()
      {
      ///_recBufFlipped = false;
      _tmpRecordCount = 0;
      init();
      }

MidiDevice::MidiDevice(const QString& n)
   : _name(n)
      {
      ///_recBufFlipped = false;
      _tmpRecordCount = 0;
      init();
      }

//---------------------------------------------------------
//   filterEvent
//    return true if event filtered
//---------------------------------------------------------

//static bool filterEvent(const MEvent& event, int type, bool thru)
bool filterEvent(const MEvent& event, int type, bool thru)
      {
      switch(event.type()) {
            case ME_NOTEON:
            case ME_NOTEOFF:
                  if (type & MIDI_FILTER_NOTEON)
                        return true;
                  break;
            case ME_POLYAFTER:
                  if (type & MIDI_FILTER_POLYP)
                        return true;
                  break;
            case ME_CONTROLLER:
                  if (type & MIDI_FILTER_CTRL)
                        return true;
                  if (!thru && (midiFilterCtrl1 == event.dataA()
                     || midiFilterCtrl2 == event.dataA()
                     || midiFilterCtrl3 == event.dataA()
                     || midiFilterCtrl4 == event.dataA())) {
                        return true;
                        }
                  break;
            case ME_PROGRAM:
                  if (type & MIDI_FILTER_PROGRAM)
                        return true;
                  break;
            case ME_AFTERTOUCH:
                  if (type & MIDI_FILTER_AT)
                        return true;
                  break;
            case ME_PITCHBEND:
                  if (type & MIDI_FILTER_PITCH)
                        return true;
                  break;
            case ME_SYSEX:
                  if (type & MIDI_FILTER_SYSEX)
                        return true;
                  break;
            default:
                  break;
            }
      return false;
      }

//---------------------------------------------------------
//   afterProcess
//    clear all recorded events after a process cycle
//---------------------------------------------------------

void MidiDevice::afterProcess()
      {
      while (_tmpRecordCount--)
            _recordFifo.remove();
      }

//---------------------------------------------------------
//   beforeProcess
//    "freeze" fifo for this process cycle
//---------------------------------------------------------

void MidiDevice::beforeProcess()
      {
      //if (!jackPort(0).isZero())
      //      audioDriver->collectMidiEvents(this, jackPort(0));
      _tmpRecordCount = _recordFifo.getSize();
      }

/*
//---------------------------------------------------------
//   getEvents
//---------------------------------------------------------

void MidiDevice::getEvents(unsigned , unsigned , int ch, MPEventList* dst)  //from //to
{
  for (int i = 0; i < _tmpRecordCount; ++i) {
        const MidiPlayEvent& ev = _recordFifo.peek(i);
        if (ch == -1 || (ev.channel() == ch))
              dst->insert(ev);
        }
  
  //while(!recordFifo.isEmpty())
  //{
  //  MidiPlayEvent e(recordFifo.get());
  //  if (ch == -1 || (e.channel() == ch))
  //        dst->insert(e);
  //}  
}
*/

/*
//---------------------------------------------------------
//   recordEvent
//---------------------------------------------------------

MREventList* MidiDevice::recordEvents()        
{ 
  // Return which list is NOT currently being filled with incoming midi events. By T356.
  if(_recBufFlipped) 
    return &_recordEvents; 
  else  
    return &_recordEvents2; 
}
*/

//---------------------------------------------------------
//   recordEvent
//---------------------------------------------------------

void MidiDevice::recordEvent(MidiRecordEvent& event)
      {
      // p3.3.35
      // TODO: Tested, but record resolution not so good. Switch to wall clock based separate list in MidiDevice. And revert this line.
      //event.setTime(audio->timestamp());
      event.setTime(extSyncFlag.value() ? lastExtMidiSyncTick : audio->timestamp());
      
      //printf("MidiDevice::recordEvent event time:%d\n", event.time());
      
      // Added by Tim. p3.3.8
      
      // By T356. Set the loop number which the event came in at.
      //if(audio->isRecording())
      if(audio->isPlaying())
        event.setLoopNum(audio->loopCount());
      
      if (midiInputTrace) {
            printf("MidiInput: ");
            event.dump();
            }

      if(_port != -1)
      {
        int idin = midiPorts[_port].syncInfo().idIn();
        
// p3.3.26 1/23/10 Section was disabled, enabled by Tim.
//#if 0
        int typ = event.type();
  
        //---------------------------------------------------
        // filter some SYSEX events
        //---------------------------------------------------
  
        if (typ == ME_SYSEX) {
              const unsigned char* p = event.data();
              int n = event.len();
              if (n >= 4) {
                    if ((p[0] == 0x7f)
                      //&& ((p[1] == 0x7f) || (p[1] == rxDeviceId))) {
                      && ((p[1] == 0x7f) || (idin == 0x7f) || (p[1] == idin))) {
                          if (p[2] == 0x06) {
                                //mmcInput(p, n);
                                midiSeq->mmcInput(_port, p, n);
                                return;
                                }
                          if (p[2] == 0x01) {
                                //mtcInputFull(p, n);
                                midiSeq->mtcInputFull(_port, p, n);
                                return;
                                }
                          }
                    else if (p[0] == 0x7e) {
                          //nonRealtimeSystemSysex(p, n);
                          midiSeq->nonRealtimeSystemSysex(_port, p, n);
                          return;
                          }
                    }
              }
          else    
            // p3.3.26 1/23/10 Moved here from alsaProcessMidiInput(). Anticipating Jack midi support, so don't make it ALSA specific. Tim. 
            // Trigger general activity indicator detector. Sysex has no channel, don't trigger.
            midiPorts[_port].syncInfo().trigActDetect(event.channel());
              
//#endif

      }
      
      //
      //  process midi event input filtering and
      //    transformation
      //

      processMidiInputTransformPlugins(event);

      if (filterEvent(event, midiRecordType, false))
            return;
      
      if (!applyMidiInputTransformation(event)) {
            if (midiInputTrace)
                  printf("   midi input transformation: event filtered\n");
            return;
            }

      //
      // transfer noteOn events to gui for step recording and keyboard
      // remote control
      //
      if (event.type() == ME_NOTEON) {
            int pv = ((event.dataA() & 0xff)<<8) + (event.dataB() & 0xff);
            song->putEvent(pv);
            }
      
      ///if(_recBufFlipped)
      ///  _recordEvents2.add(event);     // add event to secondary list of recorded events
      ///else
      ///  _recordEvents.add(event);     // add event to primary list of recorded events
      if(_recordFifo.put(MidiPlayEvent(event)))
        printf("MidiDevice::recordEvent: fifo overflow\n");
      }

//---------------------------------------------------------
//   find
//---------------------------------------------------------

MidiDevice* MidiDeviceList::find(const QString& s)
      {
      for (iMidiDevice i = begin(); i != end(); ++i)
            if ((*i)->name() == s)
                  return *i;
      return 0;
      }

iMidiDevice MidiDeviceList::find(const MidiDevice* dev)
      {
      for (iMidiDevice i = begin(); i != end(); ++i)
            if (*i == dev)
                  return i;
      return end();
      }

//---------------------------------------------------------
//   add
//---------------------------------------------------------

void MidiDeviceList::add(MidiDevice* dev)
      {
      bool gotUniqueName=false;
      int increment = 0;
      QString origname = dev->name();
      while (!gotUniqueName) {
            gotUniqueName = true;
            // check if the name's been taken
            for (iMidiDevice i = begin(); i != end(); ++i) {
                  const QString s = (*i)->name();
                  if (s == dev->name())
                        {
                        char incstr[4];
                        sprintf(incstr,"_%d",++increment);;
                        dev->setName(origname + incstr);
                        gotUniqueName = false;
                        }
                  }
            }
      
      push_back(dev);
      }

//---------------------------------------------------------
//   remove
//---------------------------------------------------------

void MidiDeviceList::remove(MidiDevice* dev)
      {
      for (iMidiDevice i = begin(); i != end(); ++i) {
            if (*i == dev) {
                  erase(i);
                  break;
                  }
            }
      }

//---------------------------------------------------------
//   sendNullRPNParams
//---------------------------------------------------------

bool MidiDevice::sendNullRPNParams(int chn, bool nrpn)
{
  if(_port == -1)
    return false;  
    
  int nv = midiPorts[_port].nullSendValue();
  if(nv == -1)
    return false;  
  
  int nvh = (nv >> 8) & 0xff;
  int nvl = nv & 0xff;
  if(nvh != 0xff)
  {
    if(nrpn)
      putMidiEvent(MidiPlayEvent(0, 0, chn, ME_CONTROLLER, CTRL_HNRPN, nvh & 0x7f));
    else
      putMidiEvent(MidiPlayEvent(0, 0, chn, ME_CONTROLLER, CTRL_HRPN, nvh & 0x7f));
  }
  if(nvl != 0xff)
  {
    if(nrpn)
      putMidiEvent(MidiPlayEvent(0, 0, chn, ME_CONTROLLER, CTRL_LNRPN, nvl & 0x7f));
    else  
      putMidiEvent(MidiPlayEvent(0, 0, chn, ME_CONTROLLER, CTRL_LRPN, nvl & 0x7f));
  }
  return true;  
}

//---------------------------------------------------------
//   putEvent
//    return true if event cannot be delivered
//    TODO: retry on controller putMidiEvent
//---------------------------------------------------------

bool MidiDevice::putEvent(const MidiPlayEvent& ev)
      {
      if(!_writeEnable)
        //return true;
        return false;
        
      if (ev.type() == ME_CONTROLLER) {
            int a = ev.dataA();
            int b = ev.dataB();
            int chn = ev.channel();
            if (a == CTRL_PITCH) {
                  return putMidiEvent(MidiPlayEvent(0, 0, chn, ME_PITCHBEND, b, 0));
                  }
            if (a == CTRL_PROGRAM) {
                  // don't output program changes for GM drum channel
                  if (!(song->mtype() == MT_GM && chn == 9)) {
                        int hb = (b >> 16) & 0xff;
                        int lb = (b >> 8) & 0xff;
                        int pr = b & 0x7f;
                        if (hb != 0xff)
                              putMidiEvent(MidiPlayEvent(0, 0, chn, ME_CONTROLLER, CTRL_HBANK, hb));
                        if (lb != 0xff)
                              putMidiEvent(MidiPlayEvent(0, 0, chn, ME_CONTROLLER, CTRL_LBANK, lb));
                        return putMidiEvent(MidiPlayEvent(0, 0, chn, ME_PROGRAM, pr, 0));
                        }
                  }
#if 1 // if ALSA cannot handle RPN NRPN etc.
            
            // p3.3.37
            //if (a < 0x1000) {          // 7 Bit Controller
            if (a < CTRL_14_OFFSET) {          // 7 Bit Controller
                  //putMidiEvent(MidiPlayEvent(0, 0, chn, ME_CONTROLLER, a, b));
                  putMidiEvent(ev);
                  }
            //else if (a < 0x20000) {     // 14 bit high resolution controller
            else if (a < CTRL_RPN_OFFSET) {     // 14 bit high resolution controller
                  int ctrlH = (a >> 8) & 0x7f;
                  int ctrlL = a & 0x7f;
                  int dataH = (b >> 7) & 0x7f;
                  int dataL = b & 0x7f;
                  putMidiEvent(MidiPlayEvent(0, 0, chn, ME_CONTROLLER, ctrlH, dataH));
                  putMidiEvent(MidiPlayEvent(0, 0, chn, ME_CONTROLLER, ctrlL, dataL));
                  }
            //else if (a < 0x30000) {     // RPN 7-Bit Controller
            else if (a < CTRL_NRPN_OFFSET) {     // RPN 7-Bit Controller
                  int ctrlH = (a >> 8) & 0x7f;
                  int ctrlL = a & 0x7f;
                  putMidiEvent(MidiPlayEvent(0, 0, chn, ME_CONTROLLER, CTRL_HRPN, ctrlH));
                  putMidiEvent(MidiPlayEvent(0, 0, chn, ME_CONTROLLER, CTRL_LRPN, ctrlL));
                  putMidiEvent(MidiPlayEvent(0, 0, chn, ME_CONTROLLER, CTRL_HDATA, b));
                  
                  // Added by T356. Select null parameters so that subsequent data controller
                  //  events do not upset the last *RPN controller.
                  sendNullRPNParams(chn, false);
                }
            //else if (a < 0x40000) {     // NRPN 7-Bit Controller
            else if (a < CTRL_INTERNAL_OFFSET) {     // NRPN 7-Bit Controller
                  int ctrlH = (a >> 8) & 0x7f;
                  int ctrlL = a & 0x7f;
                  putMidiEvent(MidiPlayEvent(0, 0, chn, ME_CONTROLLER, CTRL_HNRPN, ctrlH));
                  putMidiEvent(MidiPlayEvent(0, 0, chn, ME_CONTROLLER, CTRL_LNRPN, ctrlL));
                  putMidiEvent(MidiPlayEvent(0, 0, chn, ME_CONTROLLER, CTRL_HDATA, b));
                  
                  sendNullRPNParams(chn, true);
                  }
            //else if (a < 0x60000) {     // RPN14 Controller
            else if (a < CTRL_NRPN14_OFFSET) {     // RPN14 Controller
                  int ctrlH = (a >> 8) & 0x7f;
                  int ctrlL = a & 0x7f;
                  int dataH = (b >> 7) & 0x7f;
                  int dataL = b & 0x7f;
                  putMidiEvent(MidiPlayEvent(0, 0, chn, ME_CONTROLLER, CTRL_HRPN, ctrlH));
                  putMidiEvent(MidiPlayEvent(0, 0, chn, ME_CONTROLLER, CTRL_LRPN, ctrlL));
                  putMidiEvent(MidiPlayEvent(0, 0, chn, ME_CONTROLLER, CTRL_HDATA, dataH));
                  putMidiEvent(MidiPlayEvent(0, 0, chn, ME_CONTROLLER, CTRL_LDATA, dataL));
                  
                  sendNullRPNParams(chn, false);
                  }
            //else if (a < 0x70000) {     // NRPN14 Controller
            else if (a < CTRL_NONE_OFFSET) {     // NRPN14 Controller
                  int ctrlH = (a >> 8) & 0x7f;
                  int ctrlL = a & 0x7f;
                  int dataH = (b >> 7) & 0x7f;
                  int dataL = b & 0x7f;
                  putMidiEvent(MidiPlayEvent(0, 0, chn, ME_CONTROLLER, CTRL_HNRPN, ctrlH));
                  putMidiEvent(MidiPlayEvent(0, 0, chn, ME_CONTROLLER, CTRL_LNRPN, ctrlL));
                  putMidiEvent(MidiPlayEvent(0, 0, chn, ME_CONTROLLER, CTRL_HDATA, dataH));
                  putMidiEvent(MidiPlayEvent(0, 0, chn, ME_CONTROLLER, CTRL_LDATA, dataL));
                  
                  sendNullRPNParams(chn, true);
                  }
            else {
                  printf("putEvent: unknown controller type 0x%x\n", a);
                  }
            return false;
#endif
            }
      return putMidiEvent(ev);
      }
