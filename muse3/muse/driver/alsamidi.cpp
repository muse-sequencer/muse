//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: alsamidi.cpp,v 1.8.2.7 2009/11/19 04:20:33 terminator356 Exp $
//  (C) Copyright 2000-2001 Werner Schweer (ws@seh.de)
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

#include <stdio.h>

#include "alsamidi.h"
#include "globals.h"
#include "midi.h"
#include "../midiport.h"
#include "../midiseq.h"
#include "../midictrl.h"
#include "../audio.h"
#include "minstrument.h"
#include "utils.h"
#include "helper.h"
#include "audiodev.h"
#include "xml.h"
#include "part.h"
#include "gconfig.h"
#include "track.h"
#include "song.h"
#include "muse_atomic.h"

#include <QApplication>

// Enable debugging:
//#define ALSA_DEBUG 1  

// For debugging output: Uncomment the fprintf section.
#define DEBUG_PRST_ROUTES(dev, format, args...) // fprintf(dev, format, ##args);

namespace MusECore {
muse_atomic_t atomicAlsaMidiScanPending;

static int alsaSeqFdi = -1;
static int alsaSeqFdo = -1;

snd_seq_t* alsaSeq = 0;
static snd_seq_addr_t musePort;
static snd_seq_addr_t announce_adr;

//---------------------------------------------------------
//   createAlsaMidiDevice
//   If name parameter is blank, creates a new (locally) unique one.
//---------------------------------------------------------

MidiDevice* MidiAlsaDevice::createAlsaMidiDevice(QString name, int rwflags) // 1:Writable 2: Readable 3: Writable + Readable
{
  int ni = 0;
  if(name.isEmpty())
  {
    for( ; ni < 65536; ++ni)
    {
      name.sprintf("alsa-midi-%d", ni);
      if(!MusEGlobal::midiDevices.find(name))
        break;
    }
  }    
  if(ni >= 65536)
  {
    fprintf(stderr, "MusE: createAlsaMidiDevice failed! Can't find an unused midi device name 'alsa-midi-[0-65535]'.\n");
    return 0;
  }
  
  snd_seq_addr_t a;
  
  // From seq.h: "Special client (port) ids SND_SEQ_ADDRESS_UNKNOWN 253 = unknown source"
  // Hopefully we can use that as a 'valid' marker here. We can't use zero.
  a.client = SND_SEQ_ADDRESS_UNKNOWN;
  a.port = SND_SEQ_ADDRESS_UNKNOWN;
  
  MidiAlsaDevice* dev = new MidiAlsaDevice(a, name);

  dev->setrwFlags(rwflags);
  MusEGlobal::midiDevices.add(dev);
  return dev;
}


//---------------------------------------------------------
//   MidiAlsaDevice
//---------------------------------------------------------

MidiAlsaDevice::MidiAlsaDevice(const snd_seq_addr_t& a, const QString& n)
   : MidiDevice(n)
      {
      adr = a;
      init();
      }

//---------------------------------------------------------
//   selectWfd
//---------------------------------------------------------

int MidiAlsaDevice::selectWfd()
      {
      return alsaSeqFdo;
      }

//---------------------------------------------------------
//   open
//---------------------------------------------------------

QString MidiAlsaDevice::open()
{
      _openFlags &= _rwFlags; // restrict to available bits

      if(!alsaSeq)
      {
        _state = QString("Unavailable");
        return _state;
      }
      
      snd_seq_port_info_t *pinfo = NULL;
      snd_seq_port_subscribe_t* subs = NULL;

      DEBUG_PRST_ROUTES(stderr, "MidiAlsaDevice::open Getting port info: address: %d:%d\n", adr.client, adr.port);
      if(adr.client != SND_SEQ_ADDRESS_UNKNOWN && adr.port != SND_SEQ_ADDRESS_UNKNOWN)
      {
        snd_seq_port_info_alloca(&pinfo);
        int rv = snd_seq_get_any_port_info(alsaSeq, adr.client, adr.port, pinfo);
        if(rv < 0)
        {  
          fprintf(stderr, "MidiAlsaDevice::open Error getting port info: address: %d:%d: %s\n", adr.client, adr.port, snd_strerror(rv));
          _state = QString(snd_strerror(rv));
          return _state;
        }
        DEBUG_PRST_ROUTES(stderr, "MidiAlsaDevice::open: address: %d:%d\n", adr.client, adr.port);
        // Allocated on stack, no need to call snd_seq_port_subscribe_free() later.
        snd_seq_port_subscribe_alloca(&subs);
      }
      
      QString estr;
      int wer = 0;
      int rer = 0;

      if(adr.client != SND_SEQ_ADDRESS_UNKNOWN && adr.port != SND_SEQ_ADDRESS_UNKNOWN)
      {
        
        int cap = snd_seq_port_info_get_capability(pinfo);

#ifdef ALSA_DEBUG
        fprintf(stderr, "MidiAlsaDevice::open cap:%d\n", cap);  
#endif
        
        // subscribe for writing
        if (_openFlags & 1) 
        {
              if(cap & SND_SEQ_PORT_CAP_SUBS_WRITE)
              {  
                snd_seq_port_subscribe_set_sender(subs, &musePort);
                snd_seq_port_subscribe_set_dest(subs, &adr);
                DEBUG_PRST_ROUTES(stderr, "MidiAlsaDevice::open Checking write subscription: address: %d:%d\n", adr.client, adr.port);
                // Not already subscribed (or error)? Then try subscribing.
                if(snd_seq_get_port_subscription(alsaSeq, subs) < 0)
                {
                  //int error = snd_seq_subscribe_port(alsaSeq, subs);
                  wer = snd_seq_subscribe_port(alsaSeq, subs);
                  //if (error < 0)
                  if(wer < 0)
                        //return QString("Play: ")+QString(snd_strerror(error));
                        estr += (QString("Play: ") + QString(snd_strerror(wer)) + QString(" "));
                }        
              }  
              if(!wer && (cap & SND_SEQ_PORT_CAP_WRITE))
                _writeEnable = true;      
        }

        // subscribe for reading
        if (_openFlags & 2) 
        {
              if(cap & SND_SEQ_PORT_CAP_SUBS_READ)
              {  
                snd_seq_port_subscribe_set_dest(subs, &musePort);
                      snd_seq_port_subscribe_set_sender(subs, &adr);
                DEBUG_PRST_ROUTES(stderr, "MidiAlsaDevice::open Checking read subscription: address: %d:%d\n", adr.client, adr.port);
                // Not already subscribed (or error)? Then try subscribing.
                if(snd_seq_get_port_subscription(alsaSeq, subs) < 0)
                {
                  //int error = snd_seq_subscribe_port(alsaSeq, subs);
                  rer = snd_seq_subscribe_port(alsaSeq, subs);
                  //if (error < 0)
                  if(rer < 0)
                        //return QString("Rec: ") + QString(snd_strerror(error));
                        estr += (QString("Rec: ") + QString(snd_strerror(rer)));
                }
              }  
              if(!rer && (cap & SND_SEQ_PORT_CAP_READ))
                _readEnable = true;      
        }
      }
      else
      {
        _state = QString("Unavailable");
        return _state;
      }
      
      if(wer < 0 || rer < 0)
      {
        _state = estr;
        return _state;
      } 
      
      _state = QString("OK");
      return _state;
}

//---------------------------------------------------------
//   close
//---------------------------------------------------------

void MidiAlsaDevice::close()
{
      if(!alsaSeq)
      {
        _state = QString("Unavailable");
        return;
      }
      
      snd_seq_port_info_t *pinfo;
      snd_seq_port_subscribe_t* subs;
      if(adr.client != SND_SEQ_ADDRESS_UNKNOWN && adr.port != SND_SEQ_ADDRESS_UNKNOWN)
      {
        
        snd_seq_port_info_alloca(&pinfo);
        int rv = snd_seq_get_any_port_info(alsaSeq, adr.client, adr.port, pinfo);
        if(rv < 0)
        {  
          fprintf(stderr, "MidiAlsaDevice::close Error getting port info: adr: %d:%d: %s\n", adr.client, adr.port, snd_strerror(rv));
          _state = QString("Error on close");
          return;
        }
        // Allocated on stack, no need to call snd_seq_port_subscribe_free() later.
        snd_seq_port_subscribe_alloca(&subs);
      }

      if(adr.client == SND_SEQ_ADDRESS_UNKNOWN || adr.port == SND_SEQ_ADDRESS_UNKNOWN)
      {
        _readEnable = false;      
        _writeEnable = false;      
        _state = QString("Unavailable");
      }
      else
        
      {
        int wer = 0;
        int rer = 0;

        int cap = snd_seq_port_info_get_capability(pinfo);

#ifdef ALSA_DEBUG
        fprintf(stderr, "MidiAlsaDevice::close cap:%d\n", cap);  
#endif

        // This function appears to be called only by MidiPort::setMidiDevice(), 
        //  which closes then opens the device.
        // Because the open flags are set BEFORE setMidiDevice() is called, we must ignore the flags.
        //
        // NOTE: Tested: The read unsubscribe works ok but not the write.
        //               As viewed in say, qjackctl, the connection is clearly lost, 
        //                but strangely the events are still accepted, ie, playback notes 
        //                are still heard etc. Tried an alsa midi device AND external fluidsynth inst.
        //
        //               Also, jack running and with jack midi disabled, we get messages like
        //                MidiAlsaDevice::0x84512c0 putEvent(): midi write error: No such device
        //                 dst 16:0
        //                only sometimes (not when playing notes), but with jack midi turned on, 
        //                we don't get the messages. With jack stopped we get the messages
        //                no matter if jack midi is turned on or not.   Tim.

        //if (_openFlags & 1) {
        //if (!(_openFlags & 1)) 
        {
              if(cap & SND_SEQ_PORT_CAP_SUBS_WRITE)
              {  
                snd_seq_port_subscribe_set_sender(subs, &musePort);
                snd_seq_port_subscribe_set_dest(subs, &adr);
                
                // Already subscribed? Then unsubscribe.
                if(!snd_seq_get_port_subscription(alsaSeq, subs))
                {
                  wer = snd_seq_unsubscribe_port(alsaSeq, subs);
                  //if(!wer)
                  //  _writeEnable = false;      
                  //else
                  if(wer < 0)
                    fprintf(stderr, "MidiAlsaDevice::close Error unsubscribing alsa midi port %d:%d for writing: %s\n", adr.client, adr.port, snd_strerror(wer));
                }   
                //else
                  //_writeEnable = false;      
              }   
              _writeEnable = false;      
        }

        //if (_openFlags & 2) {
        //if (!(_openFlags & 2)) 
        {
              if(cap & SND_SEQ_PORT_CAP_SUBS_READ)
              {  
                snd_seq_port_subscribe_set_dest(subs, &musePort);
                snd_seq_port_subscribe_set_sender(subs, &adr);
                
                // Already subscribed? Then unsubscribe.
                if(!snd_seq_get_port_subscription(alsaSeq, subs))
                {
                  rer = snd_seq_unsubscribe_port(alsaSeq, subs);
                  //if(!rer)
                  //  _readEnable = false;      
                  //else  
                  if(rer < 0)
                    fprintf(stderr, "MidiAlsaDevice::close Error unsubscribing alsa midi port %d:%d for reading: %s\n", adr.client, adr.port, snd_strerror(rer));
                }  
                //else
                //  _readEnable = false;      
              }   
              _readEnable = false;      
        }
        _state = QString("Closed");
      }
}

//---------------------------------------------------------
//   writeRouting
//---------------------------------------------------------

void MidiAlsaDevice::writeRouting(int level, Xml& xml) const
{
      // If this device is not actually in use by the song, do not write any routes.
      // This prevents bogus routes from being saved and propagated in the med file.  Tim.
      if(midiPort() == -1)
        return;
     
      QString s;
      for (ciRoute r = _outRoutes.begin(); r != _outRoutes.end(); ++r) 
      {
        if(!r->name().isEmpty())
        {
          s = "Route";
          if(r->channel != -1)
            s += QString(" channel=\"%1\"").arg(r->channel);
          xml.tag(level++, s.toLatin1().constData());
          xml.tag(level, "source devtype=\"%d\" name=\"%s\"/", MidiDevice::ALSA_MIDI, Xml::xmlString(name()).toLatin1().constData());
          s = "dest";
          if(r->type == Route::MIDI_DEVICE_ROUTE)
            s += QString(" devtype=\"%1\"").arg(r->device->deviceType());
          else
          if(r->type != Route::TRACK_ROUTE)
            s += QString(" type=\"%1\"").arg(r->type);
          s += QString(" name=\"%1\"/").arg(Xml::xmlString(r->name()));
          xml.tag(level, s.toLatin1().constData());
          
          xml.etag(level--, "Route");
        }
      }
}
    
//---------------------------------------------------------
//   putEvent
//    return true if event cannot be delivered
//    TODO: retry on controller putMidiEvent
//    (Note: Since putEvent is virtual and there are different versions,
//     a retry facility is now found in putEventWithRetry. )
//---------------------------------------------------------

bool MidiAlsaDevice::putEvent(const MidiPlayEvent& ev)
      {
      if(!_writeEnable)
        //return true;
        return false;

      if (MusEGlobal::midiOutputTrace) {
            fprintf(stderr, "ALSA MidiOut pre-driver: <%s>: ", name().toLatin1().constData());
            ev.dump();
            }
            
      if(!alsaSeq || adr.client == SND_SEQ_ADDRESS_UNKNOWN || adr.port == SND_SEQ_ADDRESS_UNKNOWN)
        return true;
      
      int chn = ev.channel();
      int a   = ev.dataA();
      int b   = ev.dataB();

      snd_seq_event_t event;
      memset(&event, 0, sizeof(event));
      event.queue   = SND_SEQ_QUEUE_DIRECT;
      event.source  = musePort;
      event.dest    = adr;

      // REMOVE Tim. Noteoff. Added.
      MidiInstrument::NoteOffMode nom = MidiInstrument::NoteOffAll; // Default to NoteOffAll in case of no port.
      const int mport = midiPort();
      if(mport != -1)
      {
        if(MidiInstrument* mi = MusEGlobal::midiPorts[mport].instrument())
          nom = mi->noteOffMode();
      }
      
      switch(ev.type())
      {
        case ME_NOTEON:
          
              // REMOVE Tim. Noteoff. Added.
              if(b == 0)
              {
                // Handle zero-velocity note ons. Technically this is an error because internal midi paths
                //  are now all 'note-off' without zero-vel note ons - they're converted to note offs.
                // Nothing should be setting a Note type Event's on velocity to zero.
                // But just in case... If we get this warning, it means there is still code to change.
                fprintf(stderr, "MidiAlsaDevice::putEvent: Warning: Zero-vel note on: time:%d type:%d (ME_NOTEON) ch:%d A:%d B:%d\n", ev.time(), ev.type(), chn, a, b);  
                switch(nom)
                {
                  // Instrument uses note offs. Convert to zero-vel note off.
                  case MidiInstrument::NoteOffAll:
                    if(MusEGlobal::midiOutputTrace)
                      fprintf(stderr, "MidiOut: Alsa: Following event will be converted to zero-velocity note off:\n");
                    snd_seq_ev_set_noteoff(&event, chn, a, 0);
                  break;
                  
                  // Instrument uses no note offs at all. Send as-is.
                  case MidiInstrument::NoteOffNone:
                  // Instrument converts all note offs to zero-vel note ons. Send as-is.
                  case MidiInstrument::NoteOffConvertToZVNoteOn:
                    snd_seq_ev_set_noteon(&event, chn, a, b);
                  break;
                }
              }
              else
                
                snd_seq_ev_set_noteon(&event, chn, a, b);
              break;
        case ME_NOTEOFF:
          
              // REMOVE Tim. Noteoff. Added.
              switch(nom)
              {
                // Instrument uses note offs. Send as-is.
                case MidiInstrument::NoteOffAll:
                  snd_seq_ev_set_noteoff(&event, chn, a, b);
                break;
                
                // Instrument uses no note offs at all. Send nothing. Eat up the event - return false.
                case MidiInstrument::NoteOffNone:
                  return false;
                  
                // Instrument converts all note offs to zero-vel note ons. Convert to zero-vel note on.
                case MidiInstrument::NoteOffConvertToZVNoteOn:
                  if(MusEGlobal::midiOutputTrace)
                    fprintf(stderr, "MidiOut: Alsa: Following event will be converted to zero-velocity note on:\n");
                  snd_seq_ev_set_noteon(&event, chn, a, 0);
                break;
              }
              // REMOVE Tim. Noteoff. Removed.
//                   snd_seq_ev_set_noteoff(&event, chn, a, 0);
              break;
        case ME_PROGRAM:
              {
                _curOutParamNums[chn].resetParamNums();  // Probably best to reset.
                _curOutParamNums[chn].setPROG(a);
                snd_seq_ev_set_pgmchange(&event, chn, a);
              }
              break;
        case ME_PITCHBEND:
              snd_seq_ev_set_pitchbend(&event, chn, a);
              break;
        case ME_POLYAFTER:
              snd_seq_ev_set_keypress(&event, chn, a, b);
              break;
        case ME_AFTERTOUCH:
              snd_seq_ev_set_chanpress(&event, chn, a);
              break;
        case ME_SYSEX:
              {
              resetCurOutParamNums();  // Probably best to reset all.
              const unsigned char* p = ev.data();
              int n                  = ev.len();
              int len                = n + sizeof(event) + 2;
              char buf[len];
              event.type             = SND_SEQ_EVENT_SYSEX;
              event.flags            = SND_SEQ_EVENT_LENGTH_VARIABLE;
              event.data.ext.len     = n + 2;
              event.data.ext.ptr  = (void*)(buf + sizeof(event));
              memcpy(buf, &event, sizeof(event));
              char* pp = buf + sizeof(event);
              *pp++ = 0xf0;
              memcpy(pp, p, n);
              pp += n;
              *pp = 0xf7;
              // REMOVE Tim. Noteoff. Changed.
//               return putAlsaEvent(&event);
              break;
              }
        case ME_SONGPOS:
              event.data.control.value = a;
              event.type = SND_SEQ_EVENT_SONGPOS;
              break;
        case ME_CLOCK:
              event.type = SND_SEQ_EVENT_CLOCK;
              break;
        case ME_START:
              event.type = SND_SEQ_EVENT_START;
              break;
        case ME_CONTINUE:
              event.type = SND_SEQ_EVENT_CONTINUE;
              break;
        case ME_STOP:
              event.type = SND_SEQ_EVENT_STOP;
              break;
        case ME_CONTROLLER:
        {
            int a = ev.dataA();
            int b = ev.dataB();
            int chn = ev.channel();

            if(a == CTRL_PITCH)
              snd_seq_ev_set_pitchbend(&event, chn, b);
            else if((a | 0xff) == CTRL_POLYAFTER)
              snd_seq_ev_set_keypress(&event, chn, a & 0x7f, b & 0x7f);
            else if(a == CTRL_AFTERTOUCH)
              snd_seq_ev_set_chanpress(&event, chn, b);
            else if(a == CTRL_PROGRAM) {
                        _curOutParamNums[chn].resetParamNums();  // Probably best to reset.
                        int hb = (b >> 16) & 0xff;
                        int lb = (b >> 8) & 0xff;
                        int pr = b & 0xff;
                        _curOutParamNums[chn].setCurrentProg(pr, lb, hb);
                        if(hb != 0xff)
                        {
                          snd_seq_ev_set_controller(&event, chn, CTRL_HBANK, hb);
                          if(putAlsaEvent(&event))
                            return true;
                        }
                        if(lb != 0xff)
                        {
                          snd_seq_ev_set_controller(&event, chn, CTRL_LBANK, lb);
                          if(putAlsaEvent(&event))
                            return true;
                        }
                        if(pr != 0xff)
                        {
                          snd_seq_ev_set_pgmchange(&event, chn, pr);
                          if(putAlsaEvent(&event))
                            return true;
                        }
                        return false;
                  }

// Set this to 1 if ALSA cannot handle RPN NRPN etc.
// NOTE: Although ideally it should be 0, there are problems with
//        letting ALSA do the 'composition' of the messages in putMidiEvent() -
//        chiefly that ALSA does not handle 7-bit (N)RPN controllers.
//       This define is kept because it is important to understand, try, and see
//        the difference between the two techniques, and possibly make it work...
//       Also see the corresponding define in MidiAlsaDevice::putMidiEvent().
#if 0
                  snd_seq_ev_set_controller(&event, chn, a, b);
#else

            else if (a < CTRL_14_OFFSET) {          // 7 Bit Controller
                  if(a == CTRL_HRPN)
                    _curOutParamNums[chn].setRPNH(b);
                  else if(a == CTRL_LRPN)
                    _curOutParamNums[chn].setRPNL(b);
                  else if(a == CTRL_HNRPN)
                    _curOutParamNums[chn].setNRPNH(b);
                  else if(a == CTRL_LNRPN)
                    _curOutParamNums[chn].setNRPNL(b);
                  else if(a == CTRL_HBANK)
                  {
                    _curOutParamNums[chn].setBANKH(b);
                    _curOutParamNums[chn].resetParamNums();  // Probably best to reset.
                  }
                  else if(a == CTRL_LBANK)
                  {
                    _curOutParamNums[chn].setBANKH(b);
                    _curOutParamNums[chn].resetParamNums();  // Probably best to reset.
                  }
                  else if(a == CTRL_RESET_ALL_CTRL)
                    _curOutParamNums[chn].resetParamNums();  // Probably best to reset.
                    
                  snd_seq_ev_set_controller(&event, chn, a, b);
                  }
            else if (a < CTRL_RPN_OFFSET) {     // 14 bit high resolution controller
                  int ctrlH = (a >> 8) & 0x7f;
                  int ctrlL = a & 0x7f;
#if 0                  
                  int dataH = (b >> 7) & 0x7f;
                  int dataL = b & 0x7f;
                  snd_seq_ev_set_controller(&event, chn, ctrlH, dataH);
                  if(putAlsaEvent(&event))
                    return true;
                  snd_seq_ev_set_controller(&event, chn, ctrlL, dataL);
                  return putAlsaEvent(&event);
#else
                  snd_seq_event_t ev;
                  memset(&ev, 0, sizeof(ev));
                  ev.queue   = SND_SEQ_QUEUE_DIRECT;
                  ev.source  = musePort;
                  ev.dest    = adr;
                  int n = (ctrlH << 7) + ctrlL;
                  snd_seq_ev_set_controller(&ev, chn, n, b);
                  ev.type = SND_SEQ_EVENT_CONTROL14;
                  return putAlsaEvent(&ev);
#endif
                  
                  }
            else if (a < CTRL_NRPN_OFFSET) {     // RPN 7-Bit Controller
                  int ctrlH = (a >> 8) & 0x7f;
                  int ctrlL = a & 0x7f;
                  int data = b & 0x7f;
                  if(ctrlL != _curOutParamNums[chn].RPNL || !MusEGlobal::config.midiOptimizeControllers)
                  {
                    _curOutParamNums[chn].setRPNL(ctrlL);
                    snd_seq_ev_set_controller(&event, chn, CTRL_LRPN, ctrlL);
                    if(putAlsaEvent(&event))
                      return true;
                  }
                  if(ctrlH != _curOutParamNums[chn].RPNH || !MusEGlobal::config.midiOptimizeControllers)
                  {
                    _curOutParamNums[chn].setRPNH(ctrlH);
                    snd_seq_ev_set_controller(&event, chn, CTRL_HRPN, ctrlH);
                    if(putAlsaEvent(&event))
                      return true;
                  }
                  if(data != _curOutParamNums[chn].DATAH || !MusEGlobal::config.midiOptimizeControllers)
                  {
                    _curOutParamNums[chn].setDATAH(data);
                    snd_seq_ev_set_controller(&event, chn, CTRL_HDATA, data);
                    if(putAlsaEvent(&event))
                      return true;
                  }

                  // Select null parameters so that subsequent data controller
                  //  events do not upset the last *RPN controller.  Tim.
                  if(MusEGlobal::config.midiSendNullParameters)
                  {
                    _curOutParamNums[chn].setRPNH(0x7f);
                    snd_seq_ev_set_controller(&event, chn, CTRL_HRPN, 0x7f);
                    if(putAlsaEvent(&event))
                      return true;
                    
                    _curOutParamNums[chn].setRPNL(0x7f);
                    snd_seq_ev_set_controller(&event, chn, CTRL_LRPN, 0x7f);
                    if(putAlsaEvent(&event))
                      return true;
                  }
                  return false;
                }
            else if (a < CTRL_INTERNAL_OFFSET) {     // NRPN 7-Bit Controller
                  int ctrlH = (a >> 8) & 0x7f;
                  int ctrlL = a & 0x7f;
                  int data = b & 0x7f;
                  if(ctrlL != _curOutParamNums[chn].NRPNL || !MusEGlobal::config.midiOptimizeControllers)
                  {
                    _curOutParamNums[chn].setNRPNL(ctrlL);
                    snd_seq_ev_set_controller(&event, chn, CTRL_LNRPN, ctrlL);
                    if(putAlsaEvent(&event))
                      return true;
                  }
                  if(ctrlH != _curOutParamNums[chn].NRPNH || !MusEGlobal::config.midiOptimizeControllers)
                  {
                    _curOutParamNums[chn].setNRPNH(ctrlH);
                    snd_seq_ev_set_controller(&event, chn, CTRL_HNRPN, ctrlH);
                    if(putAlsaEvent(&event))
                      return true;
                  }
                  if(data != _curOutParamNums[chn].DATAH || !MusEGlobal::config.midiOptimizeControllers)
                  {
                    _curOutParamNums[chn].setDATAH(data);
                    snd_seq_ev_set_controller(&event, chn, CTRL_HDATA, data);
                    if(putAlsaEvent(&event))
                      return true;
                  }

                  if(MusEGlobal::config.midiSendNullParameters)
                  {
                    _curOutParamNums[chn].setNRPNH(0x7f);
                    snd_seq_ev_set_controller(&event, chn, CTRL_HNRPN, 0x7f);
                    if(putAlsaEvent(&event))
                      return true;
                    
                    _curOutParamNums[chn].setNRPNL(0x7f);
                    snd_seq_ev_set_controller(&event, chn, CTRL_LNRPN, 0x7f);
                    if(putAlsaEvent(&event))
                      return true;
                  }
                  return false;
                  }
            else if (a < CTRL_RPN14_OFFSET)      // Unaccounted for internal controller
                  return true;
            else if (a < CTRL_NRPN14_OFFSET) {     // RPN14 Controller
                  int ctrlH = (a >> 8) & 0x7f;
                  int ctrlL = a & 0x7f;
                  int dataH = (b >> 7) & 0x7f;
                  int dataL = b & 0x7f;
                  if(ctrlL != _curOutParamNums[chn].RPNL || !MusEGlobal::config.midiOptimizeControllers)
                  {
                    _curOutParamNums[chn].setRPNL(ctrlL);
                    snd_seq_ev_set_controller(&event, chn, CTRL_LRPN, ctrlL);
                    if(putAlsaEvent(&event))
                      return true;
                  }
                  if(ctrlH != _curOutParamNums[chn].RPNH || !MusEGlobal::config.midiOptimizeControllers)
                  {
                    _curOutParamNums[chn].setRPNH(ctrlH);
                    snd_seq_ev_set_controller(&event, chn, CTRL_HRPN, ctrlH);
                    if(putAlsaEvent(&event))
                      return true;
                  }
                  if(dataH != _curOutParamNums[chn].DATAH || !MusEGlobal::config.midiOptimizeControllers)
                  {
                    _curOutParamNums[chn].setDATAH(dataH);
                    snd_seq_ev_set_controller(&event, chn, CTRL_HDATA, dataH);
                    if(putAlsaEvent(&event))
                        return true;
                  }
                  if(dataL != _curOutParamNums[chn].DATAL || !MusEGlobal::config.midiOptimizeControllers)
                  {
                    _curOutParamNums[chn].setDATAL(dataL);
                    snd_seq_ev_set_controller(&event, chn, CTRL_LDATA, dataL);
                    if(putAlsaEvent(&event))
                        return true;
                  }

                  if(MusEGlobal::config.midiSendNullParameters)
                  {
                    _curOutParamNums[chn].setRPNH(0x7f);
                    snd_seq_ev_set_controller(&event, chn, CTRL_HRPN, 0x7f);
                    if(putAlsaEvent(&event))
                      return true;
                    
                    _curOutParamNums[chn].setRPNL(0x7f);
                    snd_seq_ev_set_controller(&event, chn, CTRL_LRPN, 0x7f);
                    if(putAlsaEvent(&event))
                      return true;
                  }
                  return false;
                  }
            else if (a < CTRL_NONE_OFFSET) {     // NRPN14 Controller
                  int ctrlH = (a >> 8) & 0x7f;
                  int ctrlL = a & 0x7f;
#if 0
                  int dataH = (b >> 7) & 0x7f;
                  int dataL = b & 0x7f;
                  if(ctrlL != _curOutParamNums[chn].NRPNL || !MusEGlobal::config.midiOptimizeControllers)
                  {
                    _curOutParamNums[chn].setNRPNL(ctrlL);
                    snd_seq_ev_set_controller(&event, chn, CTRL_LNRPN, ctrlL);
                    if(putAlsaEvent(&event))
                      return true;
                  }
                  if(ctrlH != _curOutParamNums[chn].NRPNH || !MusEGlobal::config.midiOptimizeControllers)
                  {
                    _curOutParamNums[chn].setNRPNH(ctrlH);
                    snd_seq_ev_set_controller(&event, chn, CTRL_HNRPN, ctrlH);
                    if(putAlsaEvent(&event))
                      return true;
                  }
                  if(dataH != _curOutParamNums[chn].DATAH || !MusEGlobal::config.midiOptimizeControllers)
                  {
                    _curOutParamNums[chn].setDATAH(dataH);
                    snd_seq_ev_set_controller(&event, chn, CTRL_HDATA, dataH);
                    if(putAlsaEvent(&event))
                      return true;
                  }
                  if(dataL != _curOutParamNums[chn].DATAL || !MusEGlobal::config.midiOptimizeControllers)
                  {
                    _curOutParamNums[chn].setDATAL(dataL);
                    snd_seq_ev_set_controller(&event, chn, CTRL_LDATA, dataL);
                    if(putAlsaEvent(&event))
                      return true;
                  }

#else                  
                  int n = (ctrlH << 7) + ctrlL;
                  snd_seq_ev_set_controller(&event, chn, n, b);
                  event.type = SND_SEQ_EVENT_NONREGPARAM;
                  if(putAlsaEvent(&event))
                    return true;
#endif
                  
                  if(MusEGlobal::config.midiSendNullParameters)
                  {
                    _curOutParamNums[chn].setNRPNH(0x7f);
                    snd_seq_ev_set_controller(&event, chn, CTRL_HNRPN, 0x7f);
                    if(putAlsaEvent(&event))
                      return true;
                    
                    _curOutParamNums[chn].setNRPNL(0x7f);
                    snd_seq_ev_set_controller(&event, chn, CTRL_LNRPN, 0x7f);
                    if(putAlsaEvent(&event))
                      return true;
                  }
                  return false;
                  }
            else {
                  fprintf(stderr, "putEvent: unknown controller type 0x%x\n", a);
                  return true;
                  }
#endif
        }
        break;  // ME_CONTROLLER
        
            default:
                  if(MusEGlobal::debugMsg)
                    fprintf(stderr, "MidiAlsaDevice::putEvent(): event type %d not implemented\n", ev.type());
                  return true;
            }
            
      return putAlsaEvent(&event);
      }

      
//---------------------------------------------------------
//   putAlsaEvent
//    return false if event is delivered
//---------------------------------------------------------

bool MidiAlsaDevice::putAlsaEvent(snd_seq_event_t* event)
      {
      if (MusEGlobal::midiOutputTrace) {
            fprintf(stderr, "ALSA MidiOut driver: <%s>: ", name().toLatin1().constData());
            dump(event);
            }
            
      if(!alsaSeq)
        return true;
        
      int error;

#ifdef ALSA_DEBUG
      fprintf(stderr, "MidiAlsaDevice::putAlsaEvent\n");  
#endif

      do {
            error   = snd_seq_event_output_direct(alsaSeq, event);
            int len = snd_seq_event_length(event);
            if (error == len) {
//                  printf(".");fflush(stdout);
                  return false;
                  }
            if (error < 0) {
                  if (error == -12) {
//                        printf("?");fflush(stdout);
                        return true;
                        }
                  else {
                        fprintf(stderr, "MidiAlsaDevice::%p putAlsaEvent(): midi write error: %s\n",
                           this, snd_strerror(error));
                        fprintf(stderr, "  dst %d:%d\n", adr.client, adr.port);
                        //exit(-1);
                        }
                  }
            else
                  fprintf(stderr, "MidiAlsaDevice::putAlsaEvent(): midi write returns %d, expected %d: %s\n",
                     error, len, snd_strerror(error));
            } while (error == -12);
      return true;
      }

//---------------------------------------------------------
//   processMidi
//   Called from ALSA midi sequencer thread only.
//---------------------------------------------------------

void MidiAlsaDevice::processMidi()
{
  //bool stop = stopPending;  // Snapshots
  //bool seek = seekPending;  //
  //seekPending = stopPending = false;
  // Transfer the stuck notes FIFO to the play events list.
  // FIXME It would be faster to have MidiAlsaDevice automatically add the stuck note so that only
  //  one FIFO would be needed. But that requires passing an extra 'tick' and 'off velocity' in
  //  addScheduledEvent, which felt too weird.
  //while(!stuckNotesFifo.isEmpty())
  //  _stuckNotes.add(stuckNotesFifo.get());
    
  //int frameOffset = getFrameOffset();
  //int nextTick = MusEGlobal::audio->nextTick();
  
  //bool is_playing = MusEGlobal::audio->isPlaying();  
  // We're in the ALSA midi thread. audio->isPlaying() might not be true during seek right now. Include START_PLAY state...
  //bool is_playing = MusEGlobal::audio->isPlaying() || MusEGlobal::audio->isStarting(); // TODO Check this. It includes LOOP1 and LOOP2 besides PLAY.
  int pos = MusEGlobal::audio->tickPos();
  int port = midiPort();
  MidiPort* mp = port == -1 ? 0 : &MusEGlobal::midiPorts[port];
  bool ext_sync = MusEGlobal::extSyncFlag.value();

  /*
  if(mp)
  {
    MidiSyncInfo& si = mp->syncInfo();
    if(stop)
    {
      // Don't send if external sync is on. The master, and our sync routing system will take care of that.   
      if(!ext_sync)
      {
        // Shall we check open flags?
        //if(!(dev->rwFlags() & 0x1) || !(dev->openFlags() & 1))
        //if(!(dev->openFlags() & 1))
        //  return;
              
        // Send MMC stop...
        if(si.MMCOut())
        {
          unsigned char msg[mmcStopMsgLen];
          memcpy(msg, mmcStopMsg, mmcStopMsgLen);
          msg[1] = si.idOut();
          putMidiEvent(MidiPlayEvent(0, 0, ME_SYSEX, msg, mmcStopMsgLen));
        }
        
        // Send midi stop...
        if(si.MRTOut()) 
        {
          putMidiEvent(MidiPlayEvent(0, 0, 0, ME_STOP, 0, 0));
          // Added check of option send continue not start.    p3.3.31
          // Hmm, is this required? Seems to make other devices unhappy.
          // (Could try now that this is in MidiDevice. p4.0.22 )
          //if(!si.sendContNotStart())
          //  mp->sendSongpos(MusEGlobal::audio->tickPos() * 4 / config.division);
        }
      }  
    }
    
    if(seek)
    {
      // Don't send if external sync is on. The master, and our sync routing system will take care of that.  
      if(!ext_sync)
      {
        // Send midi stop and song position pointer...
        if(si.MRTOut())
        {
          // Shall we check for device write open flag to see if it's ok to send?...
          //if(!(rwFlags() & 0x1) || !(openFlags() & 1))
          //if(!(openFlags() & 1))
          //  continue;
          putMidiEvent(MidiPlayEvent(0, 0, 0, ME_STOP, 0, 0));
          // Hm, try sending these after stuck notes below...
          //putMidiEvent(MidiPlayEvent(0, 0, 0, ME_SONGPOS, beat, 0));
          //if(is_playing)
          //  putMidiEvent(MidiPlayEvent(0, 0, 0, ME_CONTINUE, 0, 0));
        }    
      }
    }    
  }
  */
  
  /*
  if(stop || (seek && is_playing))  
  {
    // Clear all notes and handle stuck notes...
    //playEventFifo.clear();
    _playEvents.clear();
    for(iMPEvent i = _stuckNotes.begin(); i != _stuckNotes.end(); ++i) 
    {
      MidiPlayEvent ev = *i;
      ev.setTime(0);
      //_playEvents.add(ev);
      putMidiEvent(ev);  // Play immediately.
    }
    _stuckNotes.clear();
  }
  */
  
  /*
  if(mp)
  {
    MidiSyncInfo& si = mp->syncInfo();
    // Try sending these now after stuck notes above...
    if(stop || seek)
    {
      // Reset sustain.
      for(int ch = 0; ch < MIDI_CHANNELS; ++ch) 
        if(mp->hwCtrlState(ch, CTRL_SUSTAIN) == 127) 
          putMidiEvent(MidiPlayEvent(0, _port, ch, ME_CONTROLLER, CTRL_SUSTAIN, 0));
    }
    if(seek)
    {
      // Send new song position.
      if(!ext_sync && si.MRTOut())
      {
        int beat = (pos * 4) / MusEGlobal::config.division;
        putMidiEvent(MidiPlayEvent(0, 0, 0, ME_SONGPOS, beat, 0));
      }
      // Send new controller values.
      MidiCtrlValListList* cll = mp->controller();
      for(iMidiCtrlValList ivl = cll->begin(); ivl != cll->end(); ++ivl) 
      {
        MidiCtrlValList* vl = ivl->second;
        iMidiCtrlVal imcv = vl->iValue(pos);
        if(imcv != vl->end()) {
          Part* p = imcv->second.part;
          // Don't send if part or track is muted or off.
          if(!p || p->mute())
            continue;
          Track* track = p->track();
          if(track && (track->isMute() || track->off()))   
            continue;
          unsigned t = (unsigned)imcv->first;
          // Do not add values that are outside of the part.
          if(t >= p->tick() && t < (p->tick() + p->lenTick()))
            // Use sendEvent to get the optimizations and limiting. But force if there's a value at this exact position.
            mp->sendEvent(MidiPlayEvent(0, _port, ivl->first >> 24, ME_CONTROLLER, vl->num(), imcv->second.val), imcv->first == pos);
        }
      }
      
      // Send continue.
      // REMOVE Tim. This is redundant and too early - Audio::startRolling already properly sends it when sync ready.
      //if(is_playing && !ext_sync && si.MRTOut())
      //  putMidiEvent(MidiPlayEvent(0, 0, 0, ME_CONTINUE, 0, 0));
    }
  }
  */
  
  //if(!(stop || (seek && is_playing)))
  {
    // Transfer the play events FIFO to the play events list.
    //while(!playEventFifo.isEmpty())
    //  _playEvents.add(playEventFifo.get());
      
    /*  TODO Handle these more directly than putting them into play events list.
    //if(MusEGlobal::audio->isPlaying())  
    {
      iMPEvent k;
      for (k = _stuckNotes.begin(); k != _stuckNotes.end(); ++k) {
            if (k->time() >= nextTick)  
                  break;
            MidiPlayEvent ev(*k);
            if(extsync)              // p3.3.25
              ev.setTime(k->time());
            else 
              ev.setTime(tempomap.tick2frame(k->time()) + frameOffset);
            _playEvents.add(ev);
            }
      _stuckNotes.erase(_stuckNotes.begin(), k);
    }
    */
    processStuckNotes();  
  }
  
  if(_playEvents.empty())
    return;
  
  unsigned curFrame = MusEGlobal::audio->curFrame();
  
  // Play all events up to current frame.
  iMPEvent i = _playEvents.begin();            
  for (; i != _playEvents.end(); ++i) {
        if (i->time() > (ext_sync ? pos : curFrame))  // p3.3.25  Check: Should be nextTickPos? p4.0.34
          break; 
        if(mp){
          if (mp->sendEvent(*i, true))  // Force the event to be sent.
            break;
              }
        else
          // REMOVE Tim. This could not possibly work properly with given ALSA putEvent code
          //  unless the code was switched to the alternate ALSA specific calls.
          //if(putMidiEvent(*i))  
          if(putEvent(*i))
            break;
        }
  _playEvents.erase(_playEvents.begin(), i);
}

/*
//---------------------------------------------------------
//   handleStop
//---------------------------------------------------------

void MidiAlsaDevice::handleStop()
{
  // If the device is not in use by a port, don't bother it.
  if(_port == -1)
    return;
    
  stopPending = true;  // Trigger stop handling in processMidi.
  
  //---------------------------------------------------
  //    send midi stop
  //---------------------------------------------------
  
  // Don't send if external sync is on. The master, and our sync routing system will take care of that.   
  if(!MusEGlobal::extSyncFlag.value())
  {
    // Shall we check open flags?
    //if(!(dev->rwFlags() & 0x1) || !(dev->openFlags() & 1))
    //if(!(dev->openFlags() & 1))
    //  return;
          
    MidiSyncInfo& si = mp->syncInfo();
    if(si.MMCOut())
      mp->sendMMCStop();
    
    if(si.MRTOut()) 
    {
      mp->sendStop();
      // Added check of option send continue not start. Hmm, is this required? Seems to make other devices unhappy.
      // (Could try now that this is in MidiDevice.)
      //if(!si.sendContNotStart())
      //  mp->sendSongpos(MusEGlobal::audio->tickPos() * 4 / config.division);
    }
  }  

  //---------------------------------------------------
  //    reset sustain
  //---------------------------------------------------
  
  MidiPort* mp = &MusEGlobal::midiPorts[_port];
  for(int ch = 0; ch < MIDI_CHANNELS; ++ch) 
  {
    if(mp->hwCtrlState(ch, CTRL_SUSTAIN) == 127) 
    {
      MidiPlayEvent ev(0, _port, ch, ME_CONTROLLER, CTRL_SUSTAIN, 0);
      //putMidiEvent(ev);
      putEvent(ev);
      // Do sendEvent to get the optimizations - send only on a change of value.
      //mp->sendEvent(ev);
    }
  }
  
  //---------------------------------------------------
  //    send midi stop
  //---------------------------------------------------
  
//   // Don't send if external sync is on. The master, and our sync routing system will take care of that.   
//   if(!MusEGlobal::extSyncFlag.value())
//   {
//     // Shall we check open flags?
//     //if(!(dev->rwFlags() & 0x1) || !(dev->openFlags() & 1))
//     //if(!(dev->openFlags() & 1))
//     //  return;
//           
//     MidiSyncInfo& si = mp->syncInfo();
//     if(si.MMCOut())
//       mp->sendMMCStop();
//     
//     if(si.MRTOut()) 
//     {
//       // Send STOP 
//       mp->sendStop();
//       // Added check of option send continue not start. Hmm, is this required? Seems to make other devices unhappy.
//       // (Could try now that this is in MidiDevice.)
//       //if(!si.sendContNotStart())
//       //  mp->sendSongpos(MusEGlobal::audio->tickPos() * 4 / config.division);
//     }
//   }  
}
*/

/*
//---------------------------------------------------------
//   handleSeek
//---------------------------------------------------------

void MidiAlsaDevice::handleSeek()
{
  // If the device is not in use by a port, don't bother it.
  if(_port == -1)
    return;
  
  seekPending = true;  // Trigger seek handling in processMidi.
  
  MidiPort* mp = &MusEGlobal::midiPorts[_port];
  MidiCtrlValListList* cll = mp->controller();
  int pos = MusEGlobal::audio->tickPos();
  
  //---------------------------------------------------
  //    Send STOP 
  //---------------------------------------------------
    
  // Don't send if external sync is on. The master, and our sync routing system will take care of that.  
  if(!MusEGlobal::extSyncFlag.value())
  {
    if(mp->syncInfo().MRTOut())
    {
      // Shall we check for device write open flag to see if it's ok to send?...
      //if(!(rwFlags() & 0x1) || !(openFlags() & 1))
      //if(!(openFlags() & 1))
      //  continue;
      mp->sendStop();
    }    
  }
  
  //---------------------------------------------------
  //    reset sustain
  //---------------------------------------------------
  
  MidiPort* mp = &MusEGlobal::midiPorts[_port];
  for(int ch = 0; ch < MIDI_CHANNELS; ++ch) 
  {
    if(mp->hwCtrlState(ch, CTRL_SUSTAIN) == 127) 
    {
      MidiPlayEvent ev(0, _port, ch, ME_CONTROLLER, CTRL_SUSTAIN, 0);
      putEvent(ev);
      //putMidiEvent(ev);
      // Do sendEvent to get the optimizations - send only on a change of value.
      //mp->sendEvent(ev);
    }
  }
  
  //---------------------------------------------------
  //    Send new controller values
  //---------------------------------------------------
    
  for(iMidiCtrlValList ivl = cll->begin(); ivl != cll->end(); ++ivl) 
  {
    MidiCtrlValList* vl = ivl->second;
    iMidiCtrlVal imcv = vl->iValue(pos);
    if(imcv != vl->end()) 
    {
      Part* p = imcv->second.part;
      // Don't send if part or track is muted or off.
      if(!p || p->mute())
        continue;
      Track* track = p->track();
      if(track && (track->isMute() || track->off()))   
        continue;
      unsigned t = (unsigned)imcv->first;
      // Do not add values that are outside of the part.
      if(p && t >= p->tick() && t < (p->tick() + p->lenTick()) )
        //_playEvents.add(MidiPlayEvent(0, _port, ivl->first >> 24, ME_CONTROLLER, vl->num(), imcv->second.val));
        // Use sendEvent to get the optimizations and limiting. But force if there's a value at this exact position.
        mp->sendEvent(MidiPlayEvent(0, _port, ivl->first >> 24, ME_CONTROLLER, vl->num(), imcv->second.val), imcv->first == pos);
    }
  }
  
  //---------------------------------------------------
  //    Send STOP and "set song position pointer"
  //---------------------------------------------------
    
  // Don't send if external sync is on. The master, and our sync routing system will take care of that.  
  if(!MusEGlobal::extSyncFlag.value())
  {
    if(mp->syncInfo().MRTOut())
    {
      // Shall we check for device write open flag to see if it's ok to send?...
      //if(!(rwFlags() & 0x1) || !(openFlags() & 1))
      //if(!(openFlags() & 1))
      //  continue;
      
      //mp->sendStop(); // Moved above.
      int beat = (pos * 4) / MusEGlobal::config.division;
      mp->sendSongpos(beat);
    }    
  }
}
*/

//---------------------------------------------------------
//   initMidiAlsa
//    return true on error
//---------------------------------------------------------

bool initMidiAlsa()
      {
      if(alsaSeq)
      {
        DEBUG_PRST_ROUTES(stderr, "initMidiAlsa: alsaSeq already initialized, ignoring\n");
        return false; 
      }
      
      muse_atomic_init(&atomicAlsaMidiScanPending);
      muse_atomic_set(&atomicAlsaMidiScanPending, 0);
      
      if (MusEGlobal::debugMsg)
            fprintf(stderr, "initMidiAlsa\n");
      int error = snd_seq_open(&alsaSeq, "hw", SND_SEQ_OPEN_DUPLEX, SND_SEQ_NONBLOCK);
      if (error < 0) {
            fprintf(stderr, "Could not open ALSA sequencer: %s\n",
               snd_strerror(error));
            return true;
            }
            
      const int inCap  = SND_SEQ_PORT_CAP_SUBS_READ;
      const int outCap = SND_SEQ_PORT_CAP_SUBS_WRITE;
      
      snd_seq_client_info_t *cinfo;
      snd_seq_client_info_alloca(&cinfo);
      snd_seq_client_info_set_client(cinfo, -1);

      while (snd_seq_query_next_client(alsaSeq, cinfo) >= 0) {
            const char* cname = snd_seq_client_info_get_name(cinfo);
            //fprintf(stderr, "ALSA client name: %s\n", cname);  
            
            // Put Midi Through and user clients after others. Insert other unwanted clients here:          // p4.0.41
            if(snd_seq_client_info_get_type(cinfo) == SND_SEQ_USER_CLIENT || strcmp("Midi Through", cname) == 0)                   
              continue;
            
            snd_seq_port_info_t *pinfo;
            snd_seq_port_info_alloca(&pinfo);
            snd_seq_port_info_set_client(pinfo, snd_seq_client_info_get_client(cinfo));
            snd_seq_port_info_set_port(pinfo, -1);

            while (snd_seq_query_next_port(alsaSeq, pinfo) >= 0) {
                  unsigned int capability = snd_seq_port_info_get_capability(pinfo);
                  if (capability & SND_SEQ_PORT_CAP_NO_EXPORT)  // Ignore ports like "qjackctl" or "port".    p4.0.41
                    continue;
                  if ((capability & outCap) == 0) {
                          const char *name = snd_seq_port_info_get_name(pinfo);
                          if (strcmp("Timer", name) == 0 || 
                              strcmp("Announce", name) == 0 || 
                              strcmp("Receiver", name) == 0)
                                continue;
                          }
                  snd_seq_addr_t adr = *snd_seq_port_info_get_addr(pinfo);
                  
                  const QString dev_name(snd_seq_port_info_get_name(pinfo));
                  MidiDevice* dev = MusEGlobal::midiDevices.find(dev_name, MidiDevice::ALSA_MIDI);
                  const bool dev_found = dev;
                  if(dev_found)
                  {
                    DEBUG_PRST_ROUTES(stderr, "initMidiAlsa device found:%p %s\n", dev, snd_seq_port_info_get_name(pinfo));
                    // TODO: Hm, something more than this? Maybe ultimately will have to destroy/recreate the device?
                    dev->setAddressClient(adr.client);
                    dev->setAddressPort(adr.port);
                    // The state should be 'Unavailable', change it to 'Closed' (or 'Open' below).
                    //if(dev->midiPort() == -1)
                      dev->setState("Closed");
                  }
                  else 
                    dev = new MidiAlsaDevice(adr, QString(dev_name));
                  //MidiAlsaDevice* dev = new MidiAlsaDevice(adr, QString(snd_seq_port_info_get_name(pinfo)));
                  int flags = 0;
                  if (capability & outCap)
                        flags |= 1;
                  if (capability & inCap)
                        flags |= 2;
                  dev->setrwFlags(flags);
                  if (MusEGlobal::debugMsg) 
                        fprintf(stderr, "ALSA port add: <%s>, %d:%d flags %d 0x%0x\n",
                           snd_seq_port_info_get_name(pinfo),
                           adr.client, adr.port,
                           flags, capability);
                  DEBUG_PRST_ROUTES(stderr, "ALSA port add: <%s>, %d:%d flags %d 0x%0x\n",
                           snd_seq_port_info_get_name(pinfo),
                           adr.client, adr.port,
                           flags, capability);
                  if(dev_found)
                  {
//                     // The device should be closed right now. Open it if necessary,
//                     //  which will also change the state to 'Open'.
//                     if(dev->midiPort() != -1)
//                       dev->open();
                  }
                  else
                    MusEGlobal::midiDevices.add(dev);
                  }
            }

      snd_seq_client_info_set_client(cinfo, -1);   // Reset
      while (snd_seq_query_next_client(alsaSeq, cinfo) >= 0) {
            const char* cname = snd_seq_client_info_get_name(cinfo);
            //fprintf(stderr, "ALSA client name: %s\n", cname);  
            
            bool is_thru = (strcmp("Midi Through", cname) == 0);
            // Put Midi Through and user clients after others. Insert other unwanted clients here: // p4.0.41
            if( !(snd_seq_client_info_get_type(cinfo) == SND_SEQ_USER_CLIENT || is_thru) )
              continue;
            
            snd_seq_port_info_t *pinfo;
            snd_seq_port_info_alloca(&pinfo);
            snd_seq_port_info_set_client(pinfo, snd_seq_client_info_get_client(cinfo));
            snd_seq_port_info_set_port(pinfo, -1);

            while (snd_seq_query_next_port(alsaSeq, pinfo) >= 0) {
                  unsigned int capability = snd_seq_port_info_get_capability(pinfo);
                  if (capability & SND_SEQ_PORT_CAP_NO_EXPORT)  // Ignore ports like "qjackctl" or "port".    p4.0.41
                    continue;
                  if ((capability & outCap) == 0) {
                          const char *name = snd_seq_port_info_get_name(pinfo);
                          if (strcmp("Timer", name) == 0 || 
                              strcmp("Announce", name) == 0 || 
                              strcmp("Receiver", name) == 0)
                                continue;
                          }
                  snd_seq_addr_t adr = *snd_seq_port_info_get_addr(pinfo);
                  const QString dev_name(snd_seq_port_info_get_name(pinfo));
                  MidiDevice* dev = MusEGlobal::midiDevices.find(dev_name, MidiDevice::ALSA_MIDI);
                  const bool dev_found = dev;
                  if(dev_found)
                  {
                    DEBUG_PRST_ROUTES(stderr, "initMidiAlsa device found:%p %s\n", dev, snd_seq_port_info_get_name(pinfo));
                    // TODO: Hm, something more than this? Maybe ultimately will have to destroy/recreate the device?
                    dev->setAddressClient(adr.client);
                    dev->setAddressPort(adr.port);
                    // The state should be 'Unavailable', change it to 'Closed' (or 'Open' below).
                    //if(dev->midiPort() == -1)
                      dev->setState("Closed");
                  }
                  else
                    dev = new MidiAlsaDevice(adr, dev_name);
                  //MidiAlsaDevice* dev = new MidiAlsaDevice(adr, QString(snd_seq_port_info_get_name(pinfo)));
                  int flags = 0;
                  if (capability & outCap)
                        flags |= 1;
                  if (capability & inCap)
                        flags |= 2;
                  dev->setrwFlags(flags);
                  if(is_thru)             // Don't auto-open Midi Through.
                    dev->setOpenFlags(0); 
                  if (MusEGlobal::debugMsg)
                        fprintf(stderr, "ALSA port add: <%s>, %d:%d flags %d 0x%0x\n",
                           snd_seq_port_info_get_name(pinfo),
                           adr.client, adr.port,
                           flags, capability);
                  DEBUG_PRST_ROUTES(stderr, "ALSA port add: <%s>, %d:%d flags %d 0x%0x\n",
                           snd_seq_port_info_get_name(pinfo),
                           adr.client, adr.port,
                           flags, capability);
                  if(dev_found)
                  {
//                     // The device should be closed right now. Open it if necessary,
//                     //  which will also change the state to 'Open'.
//                     if(dev->midiPort() != -1)
//                       dev->open();
                  }
                  else
                    MusEGlobal::midiDevices.add(dev);
                  }
            }
            
            
      //snd_seq_set_client_name(alsaSeq, "MusE Sequencer");
      error = snd_seq_set_client_name(alsaSeq, MusEGlobal::audioDevice->clientName());
      if (error < 0) {
            fprintf(stderr, "Alsa: Set client name failed: %s", snd_strerror(error));
            return true;
            }
      
      int ci = snd_seq_poll_descriptors_count(alsaSeq, POLLIN);
      int co = snd_seq_poll_descriptors_count(alsaSeq, POLLOUT);

      if (ci > 1 || co > 1) {
            fprintf(stderr, "ALSA midi: cannot handle more than one poll fd\n");
            abort();
            }

      struct pollfd pfdi[ci];
      struct pollfd pfdo[co];
      snd_seq_poll_descriptors(alsaSeq, pfdi, ci, POLLIN);
      snd_seq_poll_descriptors(alsaSeq, pfdo, co, POLLOUT);
      alsaSeqFdo = pfdo[0].fd;
      alsaSeqFdi = pfdi[0].fd;

      int port  = snd_seq_create_simple_port(alsaSeq, "MusE Port 0",
         inCap | outCap | SND_SEQ_PORT_CAP_READ | SND_SEQ_PORT_CAP_WRITE,
         SND_SEQ_PORT_TYPE_APPLICATION);
      if (port < 0) {
            perror("create port");
            exit(1);
            }
      musePort.port   = port;
      musePort.client = snd_seq_client_id(alsaSeq);

      //-----------------------------------------
      //    subscribe to "Announce"
      //    this enables callbacks for any
      //    alsa port changes
      //-----------------------------------------

      //snd_seq_addr_t aadr;
      announce_adr.client = SND_SEQ_CLIENT_SYSTEM;
      announce_adr.port   = SND_SEQ_PORT_SYSTEM_ANNOUNCE;

      snd_seq_port_subscribe_t* subs;
      snd_seq_port_subscribe_alloca(&subs);
      snd_seq_port_subscribe_set_dest(subs, &musePort);
      snd_seq_port_subscribe_set_sender(subs, &announce_adr);
      error = snd_seq_subscribe_port(alsaSeq, subs);
      if (error < 0) {
            fprintf(stderr, "Alsa: Subscribe System failed: %s", snd_strerror(error));
            return true;
            }
            
            
      // The ALSA devices should be closed right now. Open them if necessary,
      //  which will also change their states to 'Open'.
      for(iMidiDevice i = MusEGlobal::midiDevices.begin(); i != MusEGlobal::midiDevices.end(); ++i) 
      {
        MidiDevice* d = *i;
        switch(d->deviceType())
        {
          case MidiDevice::ALSA_MIDI:
            if(d->midiPort() != -1)
              d->open();
          break;

          case MidiDevice::JACK_MIDI:
          case MidiDevice::SYNTH_MIDI:
          break;
        }
      }                
            
      return false;
      }


//---------------------------------------------------------
//   exitMidiAlsa
//---------------------------------------------------------

void exitMidiAlsa()
{
  if(alsaSeq)
  { 
    int error = 0;
    snd_seq_port_subscribe_t* subs;
    // Allocated on stack, no need to call snd_seq_port_subscribe_free() later.
    snd_seq_port_subscribe_alloca(&subs);
    
    //snd_seq_port_info_t *pinfo;
    //snd_seq_port_info_alloca(&pinfo);
    //snd_seq_port_info_set_client(pinfo, snd_seq_client_info_get_client(cinfo));
    //snd_seq_port_info_set_addr(pinfo, &announce_adr);

    snd_seq_port_subscribe_set_dest(subs, &musePort);
    snd_seq_port_subscribe_set_sender(subs, &announce_adr);
    
    // Already subscribed? Then unsubscribe.
    if(!snd_seq_get_port_subscription(alsaSeq, subs))
    {
      error = snd_seq_unsubscribe_port(alsaSeq, subs);
      if(error < 0)
        fprintf(stderr, "MusE: exitMidiAlsa: Error unsubscribing alsa midi Announce port %d:%d for reading: %s\n", announce_adr.client, announce_adr.port, snd_strerror(error));
    }   
    
    error = snd_seq_delete_simple_port(alsaSeq, musePort.port);
    if(error < 0) 
      fprintf(stderr, "MusE: Could not delete ALSA simple port: %s\n", snd_strerror(error));
    
    error = snd_seq_close(alsaSeq);  
    if(error < 0) 
      fprintf(stderr, "MusE: Could not close ALSA sequencer: %s\n", snd_strerror(error));
    
    muse_atomic_destroy(&atomicAlsaMidiScanPending);
  }
  else
    fprintf(stderr, "initMidiAlsa: alsaSeq already exited, ignoring\n");
  
  alsaSeq = 0;
  // Be sure to call MusEGlobal::midiSeq->msgUpdatePollFd() or midiSeq->updatePollFd()
  //  for this to take effect.
  alsaSeqFdi = -1;
  alsaSeqFdo = -1;
}


//---------------------------------------------------------
//   setAlsaClientName
//---------------------------------------------------------

void setAlsaClientName(const char* name)
{
#ifdef ALSA_DEBUG
  fprintf(stderr, "setAlsaClientName: %s  seq:%p\n", name, alsaSeq);
#endif            
  
  if(!alsaSeq)
    return; 
    
  int error = snd_seq_set_client_name(alsaSeq, name);
  if (error < 0) 
    fprintf(stderr, "setAlsaClientName: failed: %s", snd_strerror(error));
}

struct AlsaPort {
      snd_seq_addr_t adr;
      char* name;
      int flags;
      AlsaPort(snd_seq_addr_t a, const char* s, int f) {
            adr = a;
            name = strdup(s);
            flags = f;
            }
      //~AlsaPort() { if(name) free(name); }       
      };

static std::list<AlsaPort> portList;

//---------------------------------------------------------
//   alsaScanMidiPorts
//---------------------------------------------------------

void alsaScanMidiPorts()
      {
#ifdef ALSA_DEBUG
      fprintf(stderr, "alsa scan midi ports\n");
#endif
      DEBUG_PRST_ROUTES(stderr, "alsaScanMidiPorts\n");
      
      bool idling = false;
      portList.clear();
      
      if(!alsaSeq)
      {
        // Reset this now.
        muse_atomic_set(&atomicAlsaMidiScanPending, 0);
        
        // Check for devices to disable
        for(iMidiDevice i = MusEGlobal::midiDevices.begin(); i != MusEGlobal::midiDevices.end(); ++i) 
        {
          MidiAlsaDevice* d = dynamic_cast<MidiAlsaDevice*>(*i);
          if(d == 0) 
                continue;
                
          DEBUG_PRST_ROUTES(stderr, "alsaScanMidiPorts stopped: disabling device:%p %s\n", 
                  d, d->name().toLatin1().constData());
          if(!idling)
          {
            // Not much choice but to idle both the audio and midi threads since 
            //  midi does not idle while audio messages are being processed.
            MusEGlobal::audio->msgIdle(true);
            idling = true;
          }
          
          //operations.add(PendingOperationItem(d, SND_SEQ_ADDRESS_UNKNOWN, SND_SEQ_ADDRESS_UNKNOWN, PendingOperationItem::ModifyMidiDeviceAddress));
          d->adr.client = SND_SEQ_ADDRESS_UNKNOWN;
          d->adr.port = SND_SEQ_ADDRESS_UNKNOWN;
          // Close to reset some device members.
          d->close();
          // Update the port's state
          d->setState("Unavailable");
          if(d->midiPort() != -1)
            MusEGlobal::midiPorts[d->midiPort()].setState(d->state());
        }      
        
        if(idling)
        {
          MusEGlobal::audio->msgIdle(false);
          // Update the GUI.
          MusEGlobal::song->update(SC_CONFIG);
        }
        return;
      }
      
      QString state;
      const int inCap  = SND_SEQ_PORT_CAP_SUBS_READ;
      const int outCap = SND_SEQ_PORT_CAP_SUBS_WRITE;

      snd_seq_client_info_t* cinfo;
      snd_seq_client_info_alloca(&cinfo);
      snd_seq_client_info_set_client(cinfo, 0);

      while (snd_seq_query_next_client(alsaSeq, cinfo) >= 0) {
            snd_seq_port_info_t *pinfo;
            snd_seq_port_info_alloca(&pinfo);
            snd_seq_port_info_set_client(pinfo, snd_seq_client_info_get_client(cinfo));
            snd_seq_port_info_set_port(pinfo, -1);
            while (snd_seq_query_next_port(alsaSeq, pinfo) >= 0) {
                  unsigned int capability = snd_seq_port_info_get_capability(pinfo);
                  if (capability & SND_SEQ_PORT_CAP_NO_EXPORT)  // Ignore ports like "qjackctl" or "port".    p4.0.41
                    continue;
                  if (((capability & outCap) == 0)
                     && ((capability & inCap) == 0))
                        continue;
                  snd_seq_addr_t adr;
                  const char* name;
                  adr  = *snd_seq_port_info_get_addr(pinfo);
                  name = snd_seq_port_info_get_name(pinfo);
                  if (adr.client == musePort.client && adr.port == musePort.port)
                        continue;
                  int flags = 0;
                  if (capability & outCap)
                        flags |= 1;
                  if (capability & inCap)
                        flags |= 2;
// fprintf(stderr, "ALSA port add: <%s>, flags %d\n", name, flags);
                  portList.push_back(AlsaPort(adr, name, flags));
                  }
            }

      // Reset this now.
      muse_atomic_set(&atomicAlsaMidiScanPending, 0);
            
      //
      //  check for devices to delete
      //
      for (iMidiDevice i = MusEGlobal::midiDevices.begin(); i != MusEGlobal::midiDevices.end(); ++i) {
            MidiAlsaDevice* d = dynamic_cast<MidiAlsaDevice*>(*i);
            if (d == 0) 
                  continue;
                  
            std::list<AlsaPort>::iterator k = portList.begin();
            for (; k != portList.end(); ++k) {
                  if (k->adr.client == d->adr.client
                     && k->adr.port == d->adr.port) {
                        break;
                        }
                  // Search by name if either of the client or port are 0.
                  if(strcmp(k->name, d->name().toLatin1().constData()) == 0 &&
                     ((d->adr.client == SND_SEQ_ADDRESS_UNKNOWN && d->adr.port == SND_SEQ_ADDRESS_UNKNOWN) || 
                      (d->adr.client == SND_SEQ_ADDRESS_UNKNOWN && d->adr.port == k->adr.port) ||
                      (d->adr.port == SND_SEQ_ADDRESS_UNKNOWN && d->adr.client == k->adr.client)))
                    break;
                  }
            if (k == portList.end()) {
                  DEBUG_PRST_ROUTES(stderr, "alsaScanMidiPorts nulling adr op:ModifyMidiDeviceAddress device:%p %s\n", 
                          d, d->name().toLatin1().constData());
                  if(!idling)
                  {
                    // Not much choice but to idle both the audio and midi threads since 
                    //  midi does not idle while audio messages are being processed.
                    MusEGlobal::audio->msgIdle(true);
                    idling = true;
                  }
                  
                  //operations.add(PendingOperationItem(d, SND_SEQ_ADDRESS_UNKNOWN, SND_SEQ_ADDRESS_UNKNOWN, PendingOperationItem::ModifyMidiDeviceAddress));
                  d->adr.client = SND_SEQ_ADDRESS_UNKNOWN;
                  d->adr.port = SND_SEQ_ADDRESS_UNKNOWN;
                  // Close to reset some device members.
                  d->close();
                  // Update the port's state
                  d->setState("Unavailable");
                  if(d->midiPort() != -1)
                    MusEGlobal::midiPorts[d->midiPort()].setState(d->state());
                  }
            }
            
            
      //
      //  check for devices to add
      //
      // TODO: Possibly auto-add them to available midi ports.
      //
      for (std::list<AlsaPort>::iterator k = portList.begin(); k != portList.end(); ++k) {
            iMidiDevice i = MusEGlobal::midiDevices.begin();
            for (;i != MusEGlobal::midiDevices.end(); ++i) {
                  MidiAlsaDevice* d = dynamic_cast<MidiAlsaDevice*>(*i);
                  if (d == 0)
                        continue;
                  DEBUG_PRST_ROUTES(stderr, "alsaScanMidiPorts add: checking port:%s client:%d port:%d device:%p %s client:%d port:%d\n", 
                          k->name, k->adr.client, k->adr.port, d, d->name().toLatin1().constData(), d->adr.client, d->adr.port);
                  if (k->adr.client == d->adr.client && k->adr.port == d->adr.port)
                        break;
                  
                  if((d->adr.client == SND_SEQ_ADDRESS_UNKNOWN || d->adr.port == SND_SEQ_ADDRESS_UNKNOWN) && strcmp(k->name, d->name().toLatin1().constData()) == 0)
                  {
                    if(d->adr.client != SND_SEQ_ADDRESS_UNKNOWN && d->adr.client != k->adr.client)
                    {
                      DEBUG_PRST_ROUTES(stderr, "alsaScanMidiPorts: k->name:%s d->adr.client:%u != k->adr.client:%u", k->name, d->adr.client, k->adr.client);
                      //continue;
                    }
                    if(d->adr.port != SND_SEQ_ADDRESS_UNKNOWN && d->adr.port != k->adr.port)
                    {
                      DEBUG_PRST_ROUTES(stderr, "alsaScanMidiPorts: k->name:%s d->adr.port:%u != k->adr.port:%u", k->name, d->adr.port, k->adr.port);
                      //continue;
                    }
                    //if(d->adr.client == SND_SEQ_ADDRESS_UNKNOWN)
//                       d->adr.client = k->adr.client;
                    //if(d->adr.port == SND_SEQ_ADDRESS_UNKNOWN)
//                       d->adr.port = k->adr.port;
                    DEBUG_PRST_ROUTES(stderr, "alsaScanMidiPorts modifying adr op:ModifyMidiDeviceAddress device:%p %s client:%d port:%d\n", 
                            d, d->name().toLatin1().constData(), k->adr.client, k->adr.port);
                    
                    if(!idling)
                    {
                      MusEGlobal::audio->msgIdle(true);
                      idling = true;
                    }

                    //operations.add(PendingOperationItem(d, k->adr.client, k->adr.port, PendingOperationItem::ModifyMidiDeviceAddress));
                    // FIXME: Re-subscribe to any ports for now, need a time delay to implement unsubscribe-exit events
                    //operations.add(PendingOperationItem(d, k->flags, k->flags, PendingOperationItem::ModifyMidiDeviceFlags));
                    d->setAddressClient(k->adr.client);
                    d->setAddressPort(k->adr.port);
                    d->setrwFlags(k->flags);
                    // FIXME: Re-subscribe to any ports for now, need a time delay to implement unsubscribe-exit events
                    d->setOpenFlags(k->flags);
                    if(d->midiPort() < 0)
                      // Keep the device closed and update the state.
                      d->setState("Closed");
                    else
                      // Re-subscribe, open and update the port's state.
                      MusEGlobal::midiPorts[d->midiPort()].setState(d->open());
                    break;
                  }
                  
                  }
            if (i == MusEGlobal::midiDevices.end()) {

                  if(!idling)
                  {
                    MusEGlobal::audio->msgIdle(true);
                    idling = true;
                  }

                  // add device
                  
                  const QString dev_name(k->name);
                  MidiDevice* dev = MusEGlobal::midiDevices.find(dev_name, MidiDevice::ALSA_MIDI);
                  const bool dev_found = dev;
                  if(dev_found)
                  {
                    // TODO: Hm, something more than this? Maybe ultimately will have to destroy/recreate the device?
                    dev->setAddressClient(k->adr.client);
                    dev->setAddressPort(k->adr.port);
                  }
                  else  
                    dev = new MidiAlsaDevice(k->adr, dev_name);
                  //MidiAlsaDevice* dev = new MidiAlsaDevice(k->adr, QString(k->name));
                  dev->setrwFlags(k->flags);
                  DEBUG_PRST_ROUTES(stderr, "alsaScanMidiPorts op:AddMidiDevice adding:%p %s adr.client:%d adr.port:%d\n", 
                          dev, dev->name().toLatin1().constData(), k->adr.client, k->adr.port);

                  //operations.add(PendingOperationItem(MusEGlobal::midiDevices, dev, PendingOperationItem::AddMidiDevice));
                  //MusEGlobal::midiDevices.addOperation(dev, operations);
                  if(!dev_found)
                    MusEGlobal::midiDevices.add(dev);
                  // Subscribe
                  //dev->open();
                  }
            }
            
            //if(!operations.empty())
            //{
            //  if(!idling)
            //    MusEGlobal::audio->msgIdle(true);
            //  //MusEGlobal::audio->msgExecutePendingOperations(operations, true);
            //  // Execute directly both stages
            //  operations.executeNonRTStage();
            //  operations.executeRTStage();
              if(idling)
              {
                MusEGlobal::audio->msgIdle(false);
                // Update the GUI.
                MusEGlobal::song->update(SC_CONFIG);
              }
            //}
      }

//---------------------------------------------------------
//   alsaSelectRfd
//---------------------------------------------------------

int alsaSelectRfd()
      {
      return alsaSeqFdi;
      }

//---------------------------------------------------------
//   alsaSelectWfd
//---------------------------------------------------------

int alsaSelectWfd()
      {
      return alsaSeqFdo;
      }

//---------------------------------------------------------
//   processInput
//---------------------------------------------------------

void alsaProcessMidiInput()
{
      DEBUG_PRST_ROUTES(stderr, "alsaProcessMidiInput()\n");
              
      if(!alsaSeq)
        return;
      
      MidiRecordEvent event;
      snd_seq_event_t* ev;
      
      for (;;) 
      {
            int rv = snd_seq_event_input(alsaSeq, &ev);
// fprintf(stderr, "AlsaInput %d\n", rv);
            if (rv < 0) {
//                  fprintf(stderr, "AlsaMidi: read error %s\n", snd_strerror(rv));
                  return;
                  }
                  
            if (MusEGlobal::midiInputTrace) {
                  fprintf(stderr, "ALSA MidiIn driver: ");
                  MidiAlsaDevice::dump(ev);
                  }
                  
            switch(ev->type) {
                  case SND_SEQ_EVENT_PORT_SUBSCRIBED:
                        DEBUG_PRST_ROUTES(stderr, "alsaProcessMidiInput SND_SEQ_EVENT_PORT_SUBSCRIBED sender adr: %d:%d dest adr: %d:%d\n", 
                                ev->data.connect.sender.client, ev->data.connect.sender.port,
                                ev->data.connect.dest.client, ev->data.connect.dest.port);
//                         MusEGlobal::audio->midiPortsChanged();  // signal gui
//                         snd_seq_free_event(ev);
//                         return;
                        if(muse_atomic_read(&atomicAlsaMidiScanPending) == 0)
                        {
                          muse_atomic_set(&atomicAlsaMidiScanPending, 1);
                          MusEGlobal::audio->sendMsgToGui('P');
                        }
                        snd_seq_free_event(ev);
                        if(rv == 0)
                          return;
                        continue;
                        
                  case SND_SEQ_EVENT_PORT_UNSUBSCRIBED:
                        DEBUG_PRST_ROUTES(stderr, "alsaProcessMidiInput SND_SEQ_EVENT_PORT_UNSUBSCRIBED sender adr: %d:%d dest adr: %d:%d\n", 
                                ev->data.connect.sender.client, ev->data.connect.sender.port,
                                ev->data.connect.dest.client, ev->data.connect.dest.port);
//                         MusEGlobal::audio->midiPortsChanged();  // signal gui
//                         snd_seq_free_event(ev);
//                         return;
                        if(muse_atomic_read(&atomicAlsaMidiScanPending) == 0)
                        {
                          muse_atomic_set(&atomicAlsaMidiScanPending, 1);
                          MusEGlobal::audio->sendMsgToGui('P');
                        }
                        snd_seq_free_event(ev);
                        if(rv == 0)
                          return;
                        continue;
                        
                  case SND_SEQ_EVENT_CLIENT_START:
                        DEBUG_PRST_ROUTES(stderr, "alsaProcessMidiInput SND_SEQ_EVENT_CLIENT_START adr: %d:%d\n", 
                                ev->data.addr.client, ev->data.addr.port);
                        
                        //alsaScanMidiPorts();
//                         MusEGlobal::audio->midiPortsChanged();  // signal gui
//                         snd_seq_free_event(ev);
//                         return;
                        if(muse_atomic_read(&atomicAlsaMidiScanPending) == 0)
                        {
                          muse_atomic_set(&atomicAlsaMidiScanPending, 1);
                          MusEGlobal::audio->sendMsgToGui('P');
                        }
                        snd_seq_free_event(ev);
                        if(rv == 0)
                          return;
                        continue;
                        
                  case SND_SEQ_EVENT_CLIENT_EXIT:
                        DEBUG_PRST_ROUTES(stderr, "alsaProcessMidiInput SND_SEQ_EVENT_CLIENT_EXIT adr: %d:%d\n", 
                                ev->data.addr.client, ev->data.addr.port);
                        //snd_seq_free_event(ev);
                        // return;
                        // on first start of a software synthesizer we only
                        // get CLIENT_START event and no PORT_START, why?

                        //alsaScanMidiPorts();
//                         MusEGlobal::audio->midiPortsChanged();  // signal gui
//                         snd_seq_free_event(ev);
//                         return;
                        if(muse_atomic_read(&atomicAlsaMidiScanPending) == 0)
                        {
                          muse_atomic_set(&atomicAlsaMidiScanPending, 1);
                          MusEGlobal::audio->sendMsgToGui('P');
                        }
                        snd_seq_free_event(ev);
                        if(rv == 0)
                          return;
                        continue;

                  case SND_SEQ_EVENT_PORT_START:
                        DEBUG_PRST_ROUTES(stderr, "alsaProcessMidiInput SND_SEQ_EVENT_PORT_START adr: %d:%d\n", 
                                ev->data.addr.client, ev->data.addr.port);
                        
                        //alsaScanMidiPorts();
//                         MusEGlobal::audio->midiPortsChanged();  // signal gui
//                         snd_seq_free_event(ev);
//                         return;
                        if(muse_atomic_read(&atomicAlsaMidiScanPending) == 0)
                        {
                          muse_atomic_set(&atomicAlsaMidiScanPending, 1);
                          MusEGlobal::audio->sendMsgToGui('P');
                        }
                        snd_seq_free_event(ev);
                        if(rv == 0)
                          return;
                        continue;
                        
                  case SND_SEQ_EVENT_PORT_EXIT:
                        DEBUG_PRST_ROUTES(stderr, "alsaProcessMidiInput SND_SEQ_EVENT_PORT_EXIT adr: %d:%d\n", 
                                ev->data.addr.client, ev->data.addr.port);
                        //alsaScanMidiPorts();
//                         MusEGlobal::audio->midiPortsChanged();  // signal gui
//                         snd_seq_free_event(ev);
//                         return;
                        if(muse_atomic_read(&atomicAlsaMidiScanPending) == 0)
                        {
                          muse_atomic_set(&atomicAlsaMidiScanPending, 1);
                          MusEGlobal::audio->sendMsgToGui('P');
                        }
                        snd_seq_free_event(ev);
                        if(rv == 0)
                          return;
                        continue;
                        
//                   case SND_SEQ_EVENT_PORT_SUBSCRIBED:
//                   case SND_SEQ_EVENT_PORT_UNSUBSCRIBED:
//                   case SND_SEQ_EVENT_CLIENT_START:
//                   case SND_SEQ_EVENT_CLIENT_EXIT:
//                   case SND_SEQ_EVENT_PORT_START:
//                   case SND_SEQ_EVENT_PORT_EXIT:
//                         if(muse_atomic_read(&atomicAlsaMidiScanPending) == 0)
//                         {
//                           muse_atomic_set(&atomicAlsaMidiScanPending, 1);
//                           MusEGlobal::audio->sendMsgToGui('P');
//                         }
//                         snd_seq_free_event(ev);
//                         if(rv == 0)
//                           return;
//                         continue;
                  }

            int curPort = -1;
            MidiAlsaDevice* mdev = 0;
            //
            // find real source device
            //
            for (iMidiDevice i = MusEGlobal::midiDevices.begin(); i != MusEGlobal::midiDevices.end(); ++i) {
                  if((*i)->deviceType() != MidiDevice::ALSA_MIDI)
                    continue;
                  MidiAlsaDevice* d = static_cast<MidiAlsaDevice*>(*i);
                  if(d->adr.client == ev->source.client
                    && d->adr.port == ev->source.port) {
                        curPort = d->midiPort();
                        mdev = d;
                        }
                  }
            
            if (mdev == 0 || curPort == -1) {
                  if (MusEGlobal::debugMsg) {
                        fprintf(stderr, "no port %d:%d found for received alsa event\n",
                           ev->source.client, ev->source.port);
                        }
                  snd_seq_free_event(ev);
                  //return;
                  if(rv == 0)
                    return;
                  continue;
                  }

            event.setType(0);      // mark as unused
            event.setPort(curPort);
            event.setB(0);

            //MidiInstrument* instr = MusEGlobal::midiPorts[curPort].inputInstrument();
            
            switch(ev->type) 
            {
                  case SND_SEQ_EVENT_NOTEON:
                        if(ev->data.note.velocity == 0)
                        {
                          // Convert zero-velocity note ons to note offs as per midi spec.
                          event.setChannel(ev->data.note.channel);
                          event.setType(ME_NOTEOFF);
                          event.setA(ev->data.note.note);
                          event.setB(ev->data.note.velocity);
                        }
                        else
                        {
                          event.setChannel(ev->data.note.channel);
                          event.setType(ME_NOTEON);
                          event.setA(ev->data.note.note);
                          event.setB(ev->data.note.velocity);
                        }
                        break;

                  case SND_SEQ_EVENT_NOTEOFF:
                        event.setChannel(ev->data.note.channel);
                        event.setType(ME_NOTEOFF);
                        event.setA(ev->data.note.note);
                        event.setB(ev->data.note.velocity);
                        break;

                  case SND_SEQ_EVENT_KEYPRESS:
                        event.setChannel(ev->data.note.channel);
                        event.setType(ME_POLYAFTER);
                        event.setA(ev->data.note.note);
                        event.setB(ev->data.note.velocity);
                        break;

                  case SND_SEQ_EVENT_CHANPRESS:
                        event.setChannel(ev->data.control.channel);
                        event.setType(ME_AFTERTOUCH);
                        event.setA(ev->data.control.value);
                        break;

                  case SND_SEQ_EVENT_PGMCHANGE:
                        event.setChannel(ev->data.control.channel);
                        event.setType(ME_PROGRAM);
                        event.setA(ev->data.control.value);
                        break;

                  case SND_SEQ_EVENT_PITCHBEND:
                        event.setChannel(ev->data.control.channel);
                        event.setType(ME_PITCHBEND);
                        event.setA(ev->data.control.value);
                        break;

                  case SND_SEQ_EVENT_CONTROLLER:
                        event.setChannel(ev->data.control.channel);
                        event.setType(ME_CONTROLLER);
                        event.setA(ev->data.control.param);
                        event.setB(ev->data.control.value);
                        break;

                  case SND_SEQ_EVENT_CLOCK:
                        MusEGlobal::midiSeq->realtimeSystemInput(curPort, ME_CLOCK, curTime());
                        //mdev->syncInfo().trigMCSyncDetect();
                        break;

                  case SND_SEQ_EVENT_START:
                        MusEGlobal::midiSeq->realtimeSystemInput(curPort, ME_START, curTime());
                        break;

                  case SND_SEQ_EVENT_CONTINUE:
                        MusEGlobal::midiSeq->realtimeSystemInput(curPort, ME_CONTINUE, curTime());
                        break;

                  case SND_SEQ_EVENT_STOP:
                        MusEGlobal::midiSeq->realtimeSystemInput(curPort, ME_STOP, curTime());
                        break;

                  case SND_SEQ_EVENT_TICK:
                        MusEGlobal::midiSeq->realtimeSystemInput(curPort, ME_TICK, curTime());
                        //mdev->syncInfo().trigTickDetect();
                        break;

                  case SND_SEQ_EVENT_SYSEX:
                        // TODO: Deal with large sysex, which are broken up into chunks!
                        // For now, do not accept if the first byte is not SYSEX or the last byte is not EOX, 
                        //  meaning it's a chunk, possibly with more chunks to follow.
                        if((*((unsigned char*)ev->data.ext.ptr) != ME_SYSEX) ||
                           (*(((unsigned char*)ev->data.ext.ptr) + ev->data.ext.len - 1) != ME_SYSEX_END))
                        {
                          fprintf(stderr, "MusE: alsaProcessMidiInput sysex chunks not supported!\n");
                          break;
                        }
                        
                        event.setTime(0);      // mark as used
                        event.setType(ME_SYSEX);
                        event.setData((unsigned char*)(ev->data.ext.ptr)+1,
                           ev->data.ext.len-2);
                        break;
                  case SND_SEQ_EVENT_PORT_SUBSCRIBED:
                  case SND_SEQ_EVENT_PORT_UNSUBSCRIBED:  // write port is released
                        break;
                  case SND_SEQ_EVENT_SONGPOS:
                        MusEGlobal::midiSeq->setSongPosition(curPort, ev->data.control.value);
                        break;
                  case SND_SEQ_EVENT_SENSING:
                        break;
                  case SND_SEQ_EVENT_QFRAME:
                        MusEGlobal::midiSeq->mtcInputQuarter(curPort, ev->data.control.value);
                        break;
                  // case SND_SEQ_EVENT_CLIENT_START:
                  // case SND_SEQ_EVENT_CLIENT_EXIT:
                  // case SND_SEQ_EVENT_CLIENT_CHANGE:
                  // case SND_SEQ_EVENT_PORT_CHANGE:
                  // case SND_SEQ_EVENT_SONGSEL:
                  // case SND_SEQ_EVENT_TIMESIGN:
                  // case SND_SEQ_EVENT_KEYSIGN:
                  // case SND_SEQ_EVENT_SETPOS_TICK:
                  // case SND_SEQ_EVENT_SETPOS_TIME:
                  // case SND_SEQ_EVENT_TEMPO:
                  // case SND_SEQ_EVENT_TUNE_REQUEST:
                  // case SND_SEQ_EVENT_RESET:

                  // case SND_SEQ_EVENT_NOTE:
                  // case SND_SEQ_EVENT_CONTROL14:
//                   case SND_SEQ_EVENT_NONREGPARAM:
//                         fprintf(stderr, "ALSA Midi input: NONREGPARAM ch:%u param:%u value:%d\n",
//                                         ev->data.control.channel,
//                                         ev->data.control.param,
//                                         ev->data.control.value);
//                         event.setChannel(ev->data.control.channel);
//                         event.setType(ME_CONTROLLER);
//                         event.setA(ev->data.control.param);
//                         event.setB(ev->data.control.value);
//                     break;
                  // case SND_SEQ_EVENT_REGPARAM:
                  default:
                        fprintf(stderr, "ALSA Midi input: type %d not handled\n", ev->type);
                        break;
            }
            if(event.type())
              mdev->recordEvent(event);
                  
            snd_seq_free_event(ev);
            if (rv == 0)
                  break;
      }
}

//---------------------------------------------------------
//   dump
//   static
//---------------------------------------------------------

void MidiAlsaDevice::dump(const snd_seq_event_t* ev)
{
  switch(ev->type) 
  {
    case SND_SEQ_EVENT_PORT_SUBSCRIBED:
      fprintf(stderr, "SND_SEQ_EVENT_PORT_SUBSCRIBED sender adr: %d:%d dest adr: %d:%d\n", 
              ev->data.connect.sender.client, ev->data.connect.sender.port,
              ev->data.connect.dest.client, ev->data.connect.dest.port);
    break;
    
    case SND_SEQ_EVENT_PORT_UNSUBSCRIBED:
      fprintf(stderr, "SND_SEQ_EVENT_PORT_UNSUBSCRIBED sender adr: %d:%d dest adr: %d:%d\n", 
              ev->data.connect.sender.client, ev->data.connect.sender.port,
              ev->data.connect.dest.client, ev->data.connect.dest.port);
    break;
          
    case SND_SEQ_EVENT_CLIENT_START:
      fprintf(stderr, "SND_SEQ_EVENT_CLIENT_START adr: %d:%d\n", 
              ev->data.addr.client, ev->data.addr.port);
    break;
          
    case SND_SEQ_EVENT_CLIENT_EXIT:
      fprintf(stderr, "SND_SEQ_EVENT_CLIENT_EXIT adr: %d:%d\n", 
              ev->data.addr.client, ev->data.addr.port);
    break;

    case SND_SEQ_EVENT_PORT_START:
      fprintf(stderr, "SND_SEQ_EVENT_PORT_START adr: %d:%d\n", 
              ev->data.addr.client, ev->data.addr.port);
    break;
          
    case SND_SEQ_EVENT_PORT_EXIT:
      fprintf(stderr, "SND_SEQ_EVENT_PORT_EXIT adr: %d:%d\n", 
              ev->data.addr.client, ev->data.addr.port);
    break;

    case SND_SEQ_EVENT_CONTROLLER:
      fprintf(stderr, "SND_SEQ_EVENT_CONTROLLER chan:%u param:%u value:%d\n", 
              ev->data.control.channel, ev->data.control.param, ev->data.control.value);
    break;

    case SND_SEQ_EVENT_NOTE:
      fprintf(stderr, "SND_SEQ_EVENT_NOTE chan:%u note:%u velocity:%u off_velocity:%u duration:%u\n", 
              ev->data.note.channel, ev->data.note.note, ev->data.note.velocity, ev->data.note.off_velocity, ev->data.note.duration);
    break;

    case SND_SEQ_EVENT_NOTEON:
      fprintf(stderr, "SND_SEQ_EVENT_NOTEON chan:%u note:%u velocity:%u\n", 
              ev->data.note.channel, ev->data.note.note, ev->data.note.velocity);
    break;

    case SND_SEQ_EVENT_NOTEOFF:
      fprintf(stderr, "SND_SEQ_EVENT_NOTEOFF chan:%u note:%u velocity:%u\n", 
              ev->data.note.channel, ev->data.note.note, ev->data.note.velocity);
    break;

    case SND_SEQ_EVENT_KEYPRESS:
      fprintf(stderr, "SND_SEQ_EVENT_KEYPRESS chan:%u note:%u velocity:%u\n", 
              ev->data.note.channel, ev->data.note.note, ev->data.note.velocity);
    break;

    case SND_SEQ_EVENT_CHANPRESS:
      fprintf(stderr, "SND_SEQ_EVENT_CHANPRESS chan:%u value:%d\n", 
              ev->data.control.channel, ev->data.control.value);
    break;

    case SND_SEQ_EVENT_PGMCHANGE:
      fprintf(stderr, "SND_SEQ_EVENT_PGMCHANGE chan:%u value:%d\n", 
              ev->data.control.channel, ev->data.control.value);
    break;

    case SND_SEQ_EVENT_PITCHBEND:
      fprintf(stderr, "SND_SEQ_EVENT_PITCHBEND chan:%u value:%d\n", 
              ev->data.control.channel, ev->data.control.value);
    break;

    case SND_SEQ_EVENT_CLOCK:
      fprintf(stderr, "SND_SEQ_EVENT_CLOCK\n");
    break;

    case SND_SEQ_EVENT_START:
      fprintf(stderr, "SND_SEQ_EVENT_START\n");
    break;

    case SND_SEQ_EVENT_CONTINUE:
      fprintf(stderr, "SND_SEQ_EVENT_CONTINUE\n");
    break;

    case SND_SEQ_EVENT_STOP:
      fprintf(stderr, "SND_SEQ_EVENT_STOP\n");
    break;

    case SND_SEQ_EVENT_TICK:
      fprintf(stderr, "SND_SEQ_EVENT_TICK\n");
    break;

    case SND_SEQ_EVENT_SYSEX:
      if(ev->data.ext.len >= 2)
      fprintf(stderr, "SND_SEQ_EVENT_SYSEX len:%u hex: f0 %0x ...\n", 
              ev->data.ext.len, ((unsigned char*)ev->data.ext.ptr)[1]);
      else
      fprintf(stderr, "SND_SEQ_EVENT_SYSEX len:%u\n", 
              ev->data.ext.len);
      
    break;
    
    case SND_SEQ_EVENT_SONGPOS:
      fprintf(stderr, "SND_SEQ_EVENT_SONGPOS value:%d\n", 
              ev->data.control.value);
    break;
    
    case SND_SEQ_EVENT_SENSING:
      fprintf(stderr, "SND_SEQ_EVENT_SENSING\n");
    break;
          
    case SND_SEQ_EVENT_QFRAME:
      fprintf(stderr, "SND_SEQ_EVENT_QFRAME value:%d\n", 
              ev->data.control.value);
    break;
    case SND_SEQ_EVENT_CONTROL14:
      fprintf(stderr, "SND_SEQ_EVENT_CONTROL14 ch:%u param:%u value:%d\n",
              ev->data.control.channel,
              ev->data.control.param,
              ev->data.control.value);
    break;
    
    case SND_SEQ_EVENT_NONREGPARAM:
      fprintf(stderr, "SND_SEQ_EVENT_NONREGPARAM ch:%u param:%u value:%d\n",
              ev->data.control.channel,
              ev->data.control.param,
              ev->data.control.value);
    break;
    
    case SND_SEQ_EVENT_REGPARAM:
      fprintf(stderr, "SND_SEQ_EVENT_REGPARAM ch:%u param:%u value:%d\n",
              ev->data.control.channel,
              ev->data.control.param,
              ev->data.control.value);
    break;
    
    // case SND_SEQ_EVENT_CLIENT_CHANGE:
    // case SND_SEQ_EVENT_PORT_CHANGE:
    // case SND_SEQ_EVENT_SONGSEL:
    // case SND_SEQ_EVENT_TIMESIGN:
    // case SND_SEQ_EVENT_KEYSIGN:
    // case SND_SEQ_EVENT_SETPOS_TICK:
    // case SND_SEQ_EVENT_SETPOS_TIME:
    // case SND_SEQ_EVENT_TEMPO:
    // case SND_SEQ_EVENT_TUNE_REQUEST:
    // case SND_SEQ_EVENT_RESET:

    default:
      fprintf(stderr, "ALSA dump event: unknown type:%u\n", ev->type);
    break;
  }
}

} // namespace MusECore
