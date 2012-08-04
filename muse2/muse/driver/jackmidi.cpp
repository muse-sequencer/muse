//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: jackmidi.cpp,v 1.1.1.1 2010/01/27 09:06:43 terminator356 Exp $
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

#include <QString>

#include <stdio.h>
#include <string.h>

#include <jack/jack.h>
//#include <jack/midiport.h>

#include "jackmidi.h"
#include "jackaudio.h"
#include "song.h"
#include "globals.h"
#include "midi.h"
#include "mididev.h"
#include "../midiport.h"
#include "../midiseq.h"
#include "../midictrl.h"
#include "../audio.h"
#include "mpevent.h"
//#include "sync.h"
#include "audiodev.h"
#include "../mplugins/midiitransform.h"
#include "../mplugins/mitplugin.h"
#include "xml.h"
#include "gconfig.h"
#include "track.h"

// Turn on debug messages.
//#define JACK_MIDI_DEBUG

namespace MusECore {

//---------------------------------------------------------
//   MidiJackDevice
//   in_jack_port or out_jack_port can be null
//---------------------------------------------------------

MidiJackDevice::MidiJackDevice(const QString& n)
   : MidiDevice(n)
{
  _in_client_jackport  = NULL;
  _out_client_jackport = NULL;
  init();
}

MidiJackDevice::~MidiJackDevice()
{
  #ifdef JACK_MIDI_DEBUG
    printf("MidiJackDevice::~MidiJackDevice()\n");
  #endif  
  
  if(MusEGlobal::audioDevice)
  { 
    if(_in_client_jackport)
      MusEGlobal::audioDevice->unregisterPort(_in_client_jackport);
    if(_out_client_jackport)
      MusEGlobal::audioDevice->unregisterPort(_out_client_jackport);
  }  
    
    //close();
}
            
//---------------------------------------------------------
//   createJackMidiDevice
//   If name parameter is blank, creates a new (locally) unique one.
//---------------------------------------------------------

MidiDevice* MidiJackDevice::createJackMidiDevice(QString name, int rwflags) // 1:Writable 2: Readable 3: Writable + Readable
{
  int ni = 0;
  if(name.isEmpty())
  {
    for( ; ni < 65536; ++ni)
    {
      name.sprintf("jack-midi-%d", ni);
      if(!MusEGlobal::midiDevices.find(name))
        break;
    }
  }    
  if(ni >= 65536)
  {
    fprintf(stderr, "MusE: createJackMidiDevice failed! Can't find an unused midi device name 'jack-midi-[0-65535]'.\n");
    return 0;
  }
  
  MidiJackDevice* dev = new MidiJackDevice(name);  
  dev->setrwFlags(rwflags);
  MusEGlobal::midiDevices.add(dev);
  return dev;
}

//---------------------------------------------------------
//   setName
//---------------------------------------------------------

void MidiJackDevice::setName(const QString& s)
{ 
  #ifdef JACK_MIDI_DEBUG
  printf("MidiJackDevice::setName %s new name:%s\n", name().toLatin1().constData(), s.toLatin1().constData());
  #endif  
  _name = s; 
  
  if(inClientPort())  
    MusEGlobal::audioDevice->setPortName(inClientPort(), (s + QString(JACK_MIDI_IN_PORT_SUFFIX)).toLatin1().constData());
  if(outClientPort())  
    MusEGlobal::audioDevice->setPortName(outClientPort(), (s + QString(JACK_MIDI_OUT_PORT_SUFFIX)).toLatin1().constData());
}

//---------------------------------------------------------
//   open
//---------------------------------------------------------

QString MidiJackDevice::open()
{
  _openFlags &= _rwFlags; // restrict to available bits
  
  #ifdef JACK_MIDI_DEBUG
  printf("MidiJackDevice::open %s\n", name().toLatin1().constData());
  #endif  
  
  QString s;
  if(_openFlags & 1)
  {
    if(!_out_client_jackport)
    {
      if(MusEGlobal::audioDevice->deviceType() == AudioDevice::JACK_AUDIO)       
      {
        s = name() + QString(JACK_MIDI_OUT_PORT_SUFFIX);
        _out_client_jackport = (jack_port_t*)MusEGlobal::audioDevice->registerOutPort(s.toLatin1().constData(), true);   
        if(!_out_client_jackport)   
        {
          fprintf(stderr, "MusE: MidiJackDevice::open failed creating output port name %s\n", s.toLatin1().constData()); 
          _openFlags &= ~1; // Remove the flag, but continue on...
        }
      }  
    }  
  }
  else
  {
    if(_out_client_jackport)
    {
      // We want to unregister the port (which will also disconnect it), AND remove Routes, and then NULL-ify _out_client_jackport.
      // We could let our graph change callback (the gui thread one) remove the Routes (which it would anyway).
      // But that happens later (gui thread) and it needs a valid  _out_client_jackport, 
      //  so use of a registration callback would be required to finally NULL-ify _out_client_jackport, 
      //  and that would require some MidiDevice setter or re-scanner function.
      // So instead, manually remove the Routes (in the audio thread), then unregister the port, then immediately NULL-ify _out_client_jackport.
      // Our graph change callback (the gui thread one) will see a NULL  _out_client_jackport 
      //  so it cannot possibly remove the Routes, but that won't matter - we are removing them manually.
      // This is the same technique that is used for audio elsewhere in the code, like Audio::msgSetChannels()
      //  (but not Song::connectJackRoutes() which keeps the Routes for when undoing deletion of a track).
      //
      // NOTE: TESTED: Possibly a bug in QJackCtl, with Jack-1 (not Jack-2 !): 
      // After toggling the input/output green lights in the midi ports list (which gets us here), intermittently
      //  qjackctl refuses to draw connections. It allows them to be made (MusE responds) but blanks them out immediately
      //  and does not show 'disconnect', as if it is not properly aware of the connections.
      // But ALL else is OK - the connection is fine in MusE, verbose Jack messages show all went OK. 
      // Yes, there's no doubt the connections are being made.
      // When I toggle the lights again (which kills, then recreates the ports here), the problem can disappear or come back again.
      // Also once observed a weird double connection from the port to two different Jack ports but one of
      //  the connections should not have been there and kept toggling along with the other (like a 'ghost' connection).
      MusEGlobal::audio->msgRemoveRoutes(Route(this, 0), Route());   // New function msgRemoveRoutes simply uses Routes, for their pointers.
      MusEGlobal::audioDevice->unregisterPort(_out_client_jackport);
    }  
    _out_client_jackport = NULL;  
  }
  
  if(_openFlags & 2)
  {  
    if(!_in_client_jackport)
    {
      if(MusEGlobal::audioDevice->deviceType() == AudioDevice::JACK_AUDIO)       
      {
        s = name() + QString(JACK_MIDI_IN_PORT_SUFFIX);
        _in_client_jackport = (jack_port_t*)MusEGlobal::audioDevice->registerInPort(s.toLatin1().constData(), true);   
        if(!_in_client_jackport)    
        {
          fprintf(stderr, "MusE: MidiJackDevice::open failed creating input port name %s\n", s.toLatin1().constData());
          _openFlags &= ~2; // Remove the flag, but continue on...
        }  
      }
    }  
  }
  else
  {
    if(_in_client_jackport)
    {
      MusEGlobal::audio->msgRemoveRoutes(Route(), Route(this, 0));
      MusEGlobal::audioDevice->unregisterPort(_in_client_jackport);
    }  
    _in_client_jackport = NULL;  
  }
    
  _writeEnable = bool(_openFlags & 1);
  _readEnable = bool(_openFlags & 2);
  
  return QString("OK");
}

//---------------------------------------------------------
//   close
//---------------------------------------------------------

void MidiJackDevice::close()
{
  #ifdef JACK_MIDI_DEBUG
  printf("MidiJackDevice::close %s\n", name().toLatin1().constData());
  #endif  
  
  // TODO: I don't really want to unregister the
  //  Jack midi ports because then we lose the connections
  //  to Jack every time we click the read/write lights
  //  or change a port's device.   p3.3.55 
  
  /*
  if(_client_jackport)
  {
    int pf = jack_port_flags(_client_jackport);

    if(pf & JackPortIsOutput)
      _nextOutIdNum--;
    else
    if(pf & JackPortIsInput)
      _nextInIdNum--;
    MusEGlobal::audioDevice->unregisterPort(_client_jackport);
    _client_jackport = 0;
    _writeEnable = false;
    _readEnable = false;
    return;
  }  
  */
    
  _writeEnable = false;
  _readEnable = false;
}

//---------------------------------------------------------
//   writeRouting
//---------------------------------------------------------

void MidiJackDevice::writeRouting(int level, Xml& xml) const
{
      // If this device is not actually in use by the song, do not write any routes.
      // This prevents bogus routes from being saved and propagated in the med file.
      if(midiPort() == -1)
        return;
      
      QString s;
      if(rwFlags() & 2)  // Readable
      {
        for (ciRoute r = _inRoutes.begin(); r != _inRoutes.end(); ++r) 
        {
          if(!r->name().isEmpty())
          {
            xml.tag(level++, "Route");
            s = "source";
            if(r->type != Route::TRACK_ROUTE)
              s += QString(" type=\"%1\"").arg(r->type);
            s += QString(" name=\"%1\"/").arg(Xml::xmlString(r->name()));
            xml.tag(level, s.toLatin1().constData());
            xml.tag(level, "dest devtype=\"%d\" name=\"%s\"/", MidiDevice::JACK_MIDI, Xml::xmlString(name()).toLatin1().constData());
            xml.etag(level--, "Route");
          }
        }  
      } 
      
      for (ciRoute r = _outRoutes.begin(); r != _outRoutes.end(); ++r) 
      {
        if(!r->name().isEmpty())
        {
          s = "Route";
          if(r->channel != -1)
            s += QString(" channel=\"%1\"").arg(r->channel);
          xml.tag(level++, s.toLatin1().constData());
          xml.tag(level, "source devtype=\"%d\" name=\"%s\"/", MidiDevice::JACK_MIDI, Xml::xmlString(name()).toLatin1().constData());
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
//---------------------------------------------------------

/* FIX: if we fail to transmit the event,
 *      we return false (indicating OK). Otherwise
 *      it seems muse will retry forever
 */
bool MidiJackDevice::putMidiEvent(const MidiPlayEvent& /*event*/)
{
  return false;
}

//---------------------------------------------------------
//   recordEvent
//---------------------------------------------------------

void MidiJackDevice::recordEvent(MidiRecordEvent& event)
      {
      // Set the loop number which the event came in at.
      //if(MusEGlobal::audio->isRecording())
      if(MusEGlobal::audio->isPlaying())
        event.setLoopNum(MusEGlobal::audio->loopCount());
      
      if (MusEGlobal::midiInputTrace) {
            printf("MidiIn Jack: <%s>: ", name().toLatin1().constData());
            event.dump();
            }
            
      int typ = event.type();
      
      if(_port != -1)
      {
        int idin = MusEGlobal::midiPorts[_port].syncInfo().idIn();
        
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
                                MusEGlobal::midiSeq->mmcInput(_port, p, n);
                                return;
                                }
                          if (p[2] == 0x01) {
                                //mtcInputFull(p, n);
                                MusEGlobal::midiSeq->mtcInputFull(_port, p, n);
                                return;
                                }
                          }
                    else if (p[0] == 0x7e) {
                          //nonRealtimeSystemSysex(p, n);
                          MusEGlobal::midiSeq->nonRealtimeSystemSysex(_port, p, n);
                          return;
                          }
                    }
              }
          else    
            // Trigger general activity indicator detector. Sysex has no channel, don't trigger.
            MusEGlobal::midiPorts[_port].syncInfo().trigActDetect(event.channel());
      }
      
      //
      //  process midi event input filtering and
      //    transformation
      //

      processMidiInputTransformPlugins(event);

      if (filterEvent(event, MusEGlobal::midiRecordType, false))
            return;
      
      if (!applyMidiInputTransformation(event)) {
            if (MusEGlobal::midiInputTrace)
                  printf("   midi input transformation: event filtered\n");
            return;
            }

      //
      // transfer noteOn and Off events to gui for step recording and keyboard
      // remote control (changed by flo93: added noteOff-events)
      //
      if (typ == ME_NOTEON) {
            int pv = ((event.dataA() & 0xff)<<8) + (event.dataB() & 0xff);
            MusEGlobal::song->putEvent(pv);
            }
      else if (typ == ME_NOTEOFF) {
            int pv = ((event.dataA() & 0xff)<<8) + (0x00); //send an event with velo=0
            MusEGlobal::song->putEvent(pv);
            }
      
      //if(_recordFifo.put(MidiPlayEvent(event)))
      //  printf("MidiJackDevice::recordEvent: fifo overflow\n");
      
      // Do not bother recording if it is NOT actually being used by a port.
      // Because from this point on, process handles things, by selected port.    p3.3.38
      if(_port == -1)
        return;
      
      // Split the events up into channel fifos. Special 'channel' number 17 for sysex events.
      unsigned int ch = (typ == ME_SYSEX)? MIDI_CHANNELS : event.channel();
      if(_recordFifo[ch].put(event))
        printf("MidiJackDevice::recordEvent: fifo channel %d overflow\n", ch);
      }

//---------------------------------------------------------
//   midiReceived
//---------------------------------------------------------

void MidiJackDevice::eventReceived(jack_midi_event_t* ev)
      {
      MidiRecordEvent event;
      event.setB(0);
      event.setPort(_port);
      
      // NOTE: From muse_qt4_evolution. Not done here in Muse-2 (yet).
      // move all events 2*MusEGlobal::segmentSize into the future to get
      // jitterfree playback
      //
      //  cycle   n-1         n          n+1
      //          -+----------+----------+----------+-
      //               ^          ^          ^
      //               catch      process    play
      //
      
      // These Jack events arrived in the previous period, and it may not have been at the audio position before this one (after a seek).
      // This is how our ALSA driver works, events there are timestamped asynchronous of any process, referenced to the CURRENT audio 
      //  position, so that by the time of the NEXT process, THOSE events have also occured in the previous period.
      // So, technically this is correct. What MATTERS is how we adjust the times for storage, and/or simultaneous playback in THIS period,
      //  and TEST: we'll need to make sure any non-contiguous previous period is handled correctly by process - will it work OK as is?
      // If ALSA works OK than this should too...
#ifdef _AUDIO_USE_TRUE_FRAME_
      event.setTime(MusEGlobal::audio->previousPos().frame() + ev->time);
#else
      event.setTime(MusEGlobal::audio->pos().frame() + ev->time);
#endif
      event.setTick(MusEGlobal::lastExtMidiSyncTick);    

      event.setChannel(*(ev->buffer) & 0xf);
      int type = *(ev->buffer) & 0xf0;
      int a    = *(ev->buffer + 1) & 0x7f;
      int b    = *(ev->buffer + 2) & 0x7f;
      event.setType(type);

      switch(type) {
            case ME_NOTEON:
            case ME_NOTEOFF:
            case ME_CONTROLLER:
                  event.setA(*(ev->buffer + 1));
                  event.setB(*(ev->buffer + 2));
                  break;
            case ME_PROGRAM:
            case ME_AFTERTOUCH:
                  event.setA(*(ev->buffer + 1));
                  break;

            case ME_PITCHBEND:
                  event.setA(((b << 7) + a) - 8192);
                  break;

            case ME_SYSEX:
                  {
                    int type = *(ev->buffer) & 0xff;
                    switch(type) 
                    {
                          case ME_SYSEX:
                                
                                // TODO: Deal with large sysex, which are broken up into chunks!
                                // For now, do not accept if the last byte is not EOX, meaning it's a chunk with more chunks to follow.
                                if(*(((unsigned char*)ev->buffer) + ev->size - 1) != ME_SYSEX_END)
                                {
                                  if(MusEGlobal::debugMsg)
                                    printf("MidiJackDevice::eventReceived sysex chunks not supported!\n");
                                  return;
                                }
                                
                                //event.setTime(0);      // mark as used
                                event.setType(ME_SYSEX);
                                event.setData((unsigned char*)(ev->buffer + 1), ev->size - 2);
                                break;
                          case ME_MTC_QUARTER:
                                if(_port != -1)
                                  MusEGlobal::midiSeq->mtcInputQuarter(_port, *(ev->buffer + 1)); 
                                return;
                          case ME_SONGPOS:    
                                if(_port != -1)
                                  MusEGlobal::midiSeq->setSongPosition(_port, *(ev->buffer + 1) | (*(ev->buffer + 2) << 7 )); // LSB then MSB
                                return;
                          //case ME_SONGSEL:    
                          //case ME_TUNE_REQ:   
                          //case ME_SENSE:
                          case ME_CLOCK:      
                          case ME_TICK:       
                          case ME_START:      
                          case ME_CONTINUE:   
                          case ME_STOP:       
                          {
                                if(MusEGlobal::audioDevice && MusEGlobal::audioDevice->deviceType() == JACK_MIDI && _port != -1)
                                {
                                  MusECore::JackAudioDevice* jad = static_cast<MusECore::JackAudioDevice*>(MusEGlobal::audioDevice);
                                  jack_client_t* jc = jad->jackClient();
                                  if(jc)
                                  {
                                    jack_nframes_t abs_ft = jack_last_frame_time(jc)  + ev->time;
                                    double abs_ev_t = double(jack_frames_to_time(jc, abs_ft)) / 1000000.0;
                                    MusEGlobal::midiSeq->realtimeSystemInput(_port, type, abs_ev_t);
                                  }
                                }
                                return;
                          }
                          //case ME_SYSEX_END:  
                                //break;
                          //      return;
                          default:
                                if(MusEGlobal::debugMsg)
                                  printf("MidiJackDevice::eventReceived unsupported system event 0x%02x\n", type);
                                return;
                    }
                  }
                  //return;
                  break;
            default:
              if(MusEGlobal::debugMsg)
                printf("MidiJackDevice::eventReceived unknown event 0x%02x\n", type);
                //printf("MidiJackDevice::eventReceived unknown event 0x%02x size:%d buf:0x%02x 0x%02x 0x%02x ...0x%02x\n", type, ev->size, *(ev->buffer), *(ev->buffer + 1), *(ev->buffer + 2), *(ev->buffer + (ev->size - 1)));
              return;
            }

      #ifdef JACK_MIDI_DEBUG
      printf("MidiJackDevice::eventReceived time:%d type:%d ch:%d A:%d B:%d\n", event.time(), event.type(), event.channel(), event.dataA(), event.dataB());
      #endif  
      
      // Let recordEvent handle it from here, with timestamps, filtering, gui triggering etc.
      recordEvent(event);      
      }

//---------------------------------------------------------
//   collectMidiEvents
//---------------------------------------------------------

void MidiJackDevice::collectMidiEvents()
{
  if(!_readEnable)
    return;
  
  if(!_in_client_jackport)  
    return;
  
  void* port_buf = jack_port_get_buffer(_in_client_jackport, MusEGlobal::segmentSize);   
  jack_midi_event_t event;
  jack_nframes_t eventCount = jack_midi_get_event_count(port_buf);
  for (jack_nframes_t i = 0; i < eventCount; ++i) 
  {
    jack_midi_event_get(&event, port_buf, i);
    
    #ifdef JACK_MIDI_DEBUG
    printf("MidiJackDevice::collectMidiEvents number:%d time:%d\n", i, event.time);
    #endif  

    eventReceived(&event);
  }
}

//---------------------------------------------------------
//   putEvent
//    return true if event cannot be delivered
//---------------------------------------------------------

bool MidiJackDevice::putEvent(const MidiPlayEvent& ev)
{
  if(!_writeEnable || !_out_client_jackport)  
    return false;
    
  #ifdef JACK_MIDI_DEBUG
  printf("MidiJackDevice::putEvent time:%d type:%d ch:%d A:%d B:%d\n", ev.time(), ev.type(), ev.channel(), ev.dataA(), ev.dataB());
  #endif  
      
  bool rv = eventFifo.put(ev);
  if(rv)
    printf("MidiJackDevice::putEvent: port overflow\n");
  
  return rv;
}

//---------------------------------------------------------
//   queueEvent
//   return true if successful
//---------------------------------------------------------

bool MidiJackDevice::queueEvent(const MidiPlayEvent& e)
{
      // Perhaps we can find use for this value later, together with the Jack midi MusE port(s).
      // No big deal if not. Not used for now.
      //int port = e.port();
      
      //if(port >= JACK_MIDI_CHANNELS)
      //  return false;
        
      if(!_out_client_jackport)   
        return false;
      void* pb = jack_port_get_buffer(_out_client_jackport, MusEGlobal::segmentSize);  
    
      //unsigned frameCounter = ->frameTime();
      int frameOffset = MusEGlobal::audio->getFrameOffset();
      unsigned pos = MusEGlobal::audio->pos().frame();
      int ft = e.time() - frameOffset - pos;
      
      if (ft < 0)
            ft = 0;
      if (ft >= (int)MusEGlobal::segmentSize) {
            printf("MidiJackDevice::queueEvent: Event time:%d out of range. offset:%d ft:%d (seg=%d)\n", e.time(), frameOffset, ft, MusEGlobal::segmentSize);
            if (ft > (int)MusEGlobal::segmentSize)
                  ft = MusEGlobal::segmentSize - 1;
            }
      
      #ifdef JACK_MIDI_DEBUG
      printf("MidiJackDevice::queueEvent pos:%d fo:%d ft:%d time:%d type:%d ch:%d A:%d B:%d\n", pos, frameOffset, ft, e.time(), e.type(), e.channel(), e.dataA(), e.dataB());
      #endif  
      
      if (MusEGlobal::midiOutputTrace) {
            printf("MidiOut: Jack: <%s>: ", name().toLatin1().constData());
            e.dump();
            }
            
      switch(e.type()) {
            case ME_NOTEON:
            case ME_NOTEOFF:
            case ME_POLYAFTER:
            case ME_CONTROLLER:
            case ME_PITCHBEND:
                  {
                  #ifdef JACK_MIDI_DEBUG
                  printf("MidiJackDevice::queueEvent note on/off polyafter controller or pitch\n");
                  #endif  
                    
                  unsigned char* p = jack_midi_event_reserve(pb, ft, 3);
                  if (p == 0) {
                        #ifdef JACK_MIDI_DEBUG
                        fprintf(stderr, "MidiJackDevice::queueEvent NOTE CTL PAT or PB: buffer overflow, stopping until next cycle\n");  
                        #endif  
                        return false;
                        }
                  p[0] = e.type() | e.channel();
                  p[1] = e.dataA();
                  p[2] = e.dataB();
                  }
                  break;

            case ME_PROGRAM:
            case ME_AFTERTOUCH:
                  {
                  #ifdef JACK_MIDI_DEBUG
                  printf("MidiJackDevice::queueEvent program or aftertouch\n");
                  #endif  
                    
                  unsigned char* p = jack_midi_event_reserve(pb, ft, 2);
                  if (p == 0) {
                        #ifdef JACK_MIDI_DEBUG
                        fprintf(stderr, "MidiJackDevice::queueEvent PROG or AT: buffer overflow, stopping until next cycle\n");  
                        #endif  
                        return false;
                        }
                  p[0] = e.type() | e.channel();
                  p[1] = e.dataA();
                  }
                  break;
            case ME_SYSEX:
                  {
                  #ifdef JACK_MIDI_DEBUG
                  printf("MidiJackDevice::queueEvent sysex\n");
                  #endif  
                  
                  const unsigned char* data = e.data();
                  int len = e.len();
                  unsigned char* p = jack_midi_event_reserve(pb, ft, len+2);
                  if (p == 0) {
                        fprintf(stderr, "MidiJackDevice::queueEvent ME_SYSEX: buffer overflow, sysex too big, event lost\n");
                        
                        //return false;
                        // Changed to true. Absorb the sysex if it is too big, to avoid attempting 
                        //  to resend repeatedly. If the sysex is too big, it would just stay in the 
                        //  list and never be processed, because Jack could never reserve enough space.
                        // Other types of events should be OK since they are small and can be resent
                        //  next cycle.     p4.0.15 Tim.
                        // FIXME: We really need to chunk sysex events properly. It's tough. Investigating...
                        return true;
                        }
                  p[0] = 0xf0;
                  p[len+1] = 0xf7;
                  memcpy(p+1, data, len);
                  }
                  break;
            case ME_SONGPOS:
                  {
                  #ifdef JACK_MIDI_DEBUG
                  printf("MidiJackDevice::queueEvent songpos %d\n", e.dataA());
                  #endif  
                    
                  unsigned char* p = jack_midi_event_reserve(pb, ft, 3);
                  if (p == 0) {
                        #ifdef JACK_MIDI_DEBUG
                        fprintf(stderr, "MidiJackDevice::queueEvent songpos: buffer overflow, stopping until next cycle\n");  
                        #endif  
                        return false;
                        }
                  int pos = e.dataA();      
                  p[0] = e.type();
                  p[1] = pos & 0x7f;         // LSB
                  p[2] = (pos >> 7) & 0x7f;  // MSB
                  }
                  break;
            case ME_CLOCK:
            case ME_START:
            case ME_CONTINUE:
            case ME_STOP:
                  {
                  #ifdef JACK_MIDI_DEBUG
                  printf("MidiJackDevice::queueEvent realtime %x\n", e.type());
                  #endif  
                    
                  unsigned char* p = jack_midi_event_reserve(pb, ft, 1);
                  if (p == 0) {
                        #ifdef JACK_MIDI_DEBUG
                        fprintf(stderr, "MidiJackDevice::queueEvent realtime: buffer overflow, stopping until next cycle\n");  
                        #endif  
                        return false;
                        }
                  p[0] = e.type();
                  }
                  break;
            default:
                  if(MusEGlobal::debugMsg)
                    printf("MidiJackDevice::queueEvent: event type %x not supported\n", e.type());
                  return true;   // Absorb the event. Don't want it hanging around in the list. 
                  break;
            }
            
            return true;
}
      
//---------------------------------------------------------
//    processEvent
//    return true if successful
//---------------------------------------------------------

bool MidiJackDevice::processEvent(const MidiPlayEvent& event)
{    
  //int frameOffset = MusEGlobal::audio->getFrameOffset();
  //unsigned pos = MusEGlobal::audio->pos().frame();

  int chn    = event.channel();
  unsigned t = event.time();
  int a      = event.dataA();
  int b      = event.dataB();
  // Perhaps we can find use for this value later, together with the Jack midi MusE port(s).
  // No big deal if not. Not used for now.
  int port   = event.port();
  
  // TODO: No sub-tick playback resolution yet, with external sync.
  // Just do this 'standard midi 64T timing thing' for now until we figure out more precise external timings. 
  // Does require relatively short audio buffers, in order to catch the resolution, but buffer <= 256 should be OK... 
  // Tested OK so far with 128. 
  //if(MusEGlobal::extSyncFlag.value()) 
  // p4.0.15 Or, is the event marked to be played immediately?
  // Nothing to do but stamp the event to be queued for frame 0+.
  if(t == 0 || MusEGlobal::extSyncFlag.value())    
    t = MusEGlobal::audio->getFrameOffset() + MusEGlobal::audio->pos().frame();
    //t = frameOffset + pos;
      
  #ifdef JACK_MIDI_DEBUG
  //printf("MidiJackDevice::processEvent time:%d type:%d ch:%d A:%d B:%d\n", t, event.type(), chn, a, b);  
  #endif  
      
  if(event.type() == ME_PROGRAM) 
  {
    // don't output program changes for GM drum channel
    //if (!(MusEGlobal::song->mtype() == MT_GM && chn == 9)) {
          int hb = (a >> 16) & 0xff;
          int lb = (a >> 8) & 0xff;
          int pr = a & 0x7f;
          
          //printf("MidiJackDevice::processEvent ME_PROGRAM time:%d type:%d ch:%d A:%d B:%d hb:%d lb:%d pr:%d\n", 
          //       event.time(), event.type(), event.channel(), event.dataA(), event.dataB(), hb, lb, pr);
          
          if (hb != 0xff)
          {  
                if(!queueEvent(MidiPlayEvent(t, port, chn, ME_CONTROLLER, CTRL_HBANK, hb)))
                  return false;  // p4.0.15 Inform that processing the event in general failed. Ditto all below... 
///            t += 1;      
          }
          if (lb != 0xff)
          {
                if(!queueEvent(MidiPlayEvent(t, port, chn, ME_CONTROLLER, CTRL_LBANK, lb)))
                  return false;
///            t += 1;      
          }
          if(!queueEvent(MidiPlayEvent(t, port, chn, ME_PROGRAM, pr, 0)))
            return false;
            
    //      }
  }
  else
  if(event.type() == ME_PITCHBEND) 
  {
      int v = a + 8192;
      //printf("MidiJackDevice::processEvent ME_PITCHBEND v:%d time:%d type:%d ch:%d A:%d B:%d\n", v, event.time(), event.type(), event.channel(), event.dataA(), event.dataB());
      if(!queueEvent(MidiPlayEvent(t, port, chn, ME_PITCHBEND, v & 0x7f, (v >> 7) & 0x7f)))
        return false;
  }
  else
  if(event.type() == ME_CONTROLLER) 
  {
    // Perhaps we can find use for this value later, together with the Jack midi MusE port(s).
    // No big deal if not. Not used for now.
    //int port   = event.port();

    int nvh = 0xff;
    int nvl = 0xff;
    if(_port != -1)
    {
      int nv = MusEGlobal::midiPorts[_port].nullSendValue();
      if(nv != -1)
      {
        nvh = (nv >> 8) & 0xff;
        nvl = nv & 0xff;
      }
    }
      
    if(a == CTRL_PITCH) 
    {
      int v = b + 8192;
      //printf("MidiJackDevice::processEvent CTRL_PITCH v:%d time:%d type:%d ch:%d A:%d B:%d\n", v, event.time(), event.type(), event.channel(), event.dataA(), event.dataB());
      if(!queueEvent(MidiPlayEvent(t, port, chn, ME_PITCHBEND, v & 0x7f, (v >> 7) & 0x7f)))
        return false;
    }
    else if (a == CTRL_PROGRAM) 
    {
      // don't output program changes for GM drum channel
      //if (!(MusEGlobal::song->mtype() == MT_GM && chn == 9)) {
            int hb = (b >> 16) & 0xff;
            int lb = (b >> 8) & 0xff;
            int pr = b & 0x7f;
            //printf("MidiJackDevice::processEvent CTRL_PROGRAM time:%d type:%d ch:%d A:%d B:%d hb:%d lb:%d pr:%d\n", 
            //       event.time(), event.type(), event.channel(), event.dataA(), event.dataB(), hb, lb, pr);
          
            if (hb != 0xff)
            {
                  if(!queueEvent(MidiPlayEvent(t, port, chn, ME_CONTROLLER, CTRL_HBANK, hb)))
                    return false;
///                  t += 1;      
            }
            if (lb != 0xff)
            {
                  if(!queueEvent(MidiPlayEvent(t, port, chn, ME_CONTROLLER, CTRL_LBANK, lb)))
                    return false;
///                  t += 1;      
            }
            if(!queueEvent(MidiPlayEvent(t, port, chn, ME_PROGRAM, pr, 0)))
              return false;
              
      //      }
    }
    else if (a == CTRL_MASTER_VOLUME) 
    {
      unsigned char sysex[] = {
            0x7f, 0x7f, 0x04, 0x01, 0x00, 0x00
            };
      //sysex[1] = deviceId(); TODO FIXME p4.0.15 Grab the ID from midi port sync info.
      sysex[4] = b & 0x7f;
      sysex[5] = (b >> 7) & 0x7f;
      if(!queueEvent(MidiPlayEvent(t, port, ME_SYSEX, sysex, 6)))
        return false;
    }
    else if (a < CTRL_14_OFFSET) 
    {              // 7 Bit Controller
      //queueEvent(museport, MidiPlayEvent(t, port, chn, event));
      if(!queueEvent(event))
        return false;
    }
    else if (a < CTRL_RPN_OFFSET) 
    {     // 14 bit high resolution controller
      int ctrlH = (a >> 8) & 0x7f;
      int ctrlL = a & 0x7f;
      int dataH = (b >> 7) & 0x7f;
      int dataL = b & 0x7f;
      if(!queueEvent(MidiPlayEvent(t,   port, chn, ME_CONTROLLER, ctrlH, dataH)))
        return false;
///     t += 1;      
      if(!queueEvent(MidiPlayEvent(t, port, chn, ME_CONTROLLER, ctrlL, dataL)))
        return false;
    }
    else if (a < CTRL_NRPN_OFFSET) 
    {     // RPN 7-Bit Controller
      int ctrlH = (a >> 8) & 0x7f;
      int ctrlL = a & 0x7f;
      if(!queueEvent(MidiPlayEvent(t,   port, chn, ME_CONTROLLER, CTRL_HRPN, ctrlH)))
        return false;
///     t += 1;      
      if(!queueEvent(MidiPlayEvent(t, port, chn, ME_CONTROLLER, CTRL_LRPN, ctrlL)))
        return false;
///     t += 1;      
      if(!queueEvent(MidiPlayEvent(t, port, chn, ME_CONTROLLER, CTRL_HDATA, b)))
        return false;
///     t += 1;      
      
      //t += 3;  
      // Select null parameters so that subsequent data controller events do not upset the last *RPN controller.
      //sendNullRPNParams(chn, false);
      if(nvh != 0xff)
      {
        if(!queueEvent(MidiPlayEvent(t, port, chn, ME_CONTROLLER, CTRL_HRPN, nvh & 0x7f)))
          return false;
///        t += 1;  
      }
      if(nvl != 0xff)
      {
        if(!queueEvent(MidiPlayEvent(t, port, chn, ME_CONTROLLER, CTRL_LRPN, nvl & 0x7f)))
          return false;
      }    
    }
    //else if (a < CTRL_RPN14_OFFSET) 
    else if (a < CTRL_INTERNAL_OFFSET) 
    {     // NRPN 7-Bit Controller
      int ctrlH = (a >> 8) & 0x7f;
      int ctrlL = a & 0x7f;
      if(!queueEvent(MidiPlayEvent(t,   port, chn, ME_CONTROLLER, CTRL_HNRPN, ctrlH)))
        return false;
///     t += 1;      
      if(!queueEvent(MidiPlayEvent(t, port, chn, ME_CONTROLLER, CTRL_LNRPN, ctrlL)))
        return false;
///     t += 1;      
      if(!queueEvent(MidiPlayEvent(t, port, chn, ME_CONTROLLER, CTRL_HDATA, b)))
        return false;
///     t += 1;      
                  
      //t += 3;  
      //sendNullRPNParams(chn, true);
      if(nvh != 0xff)
      {
        if(!queueEvent(MidiPlayEvent(t, port, chn, ME_CONTROLLER, CTRL_HNRPN, nvh & 0x7f)))
          return false;
///        t += 1;  
      }
      if(nvl != 0xff)
      {
        if(!queueEvent(MidiPlayEvent(t, port, chn, ME_CONTROLLER, CTRL_LNRPN, nvl & 0x7f)))
          return false;
      }    
    }
    else if (a < CTRL_NRPN14_OFFSET) 
    {     // RPN14 Controller
      int ctrlH = (a >> 8) & 0x7f;
      int ctrlL = a & 0x7f;
      int dataH = (b >> 7) & 0x7f;
      int dataL = b & 0x7f;
      if(!queueEvent(MidiPlayEvent(t,   port, chn, ME_CONTROLLER, CTRL_HRPN, ctrlH)))
        return false;
///     t += 1;      
      if(!queueEvent(MidiPlayEvent(t, port, chn, ME_CONTROLLER, CTRL_LRPN, ctrlL)))
        return false;
///     t += 1;      
      if(!queueEvent(MidiPlayEvent(t, port, chn, ME_CONTROLLER, CTRL_HDATA, dataH)))
        return false;
///     t += 1;      
      if(!queueEvent(MidiPlayEvent(t, port, chn, ME_CONTROLLER, CTRL_LDATA, dataL)))
        return false;
///     t += 1;      
      
      //t += 4;  
      //sendNullRPNParams(chn, false);
      if(nvh != 0xff)
      {
        if(!queueEvent(MidiPlayEvent(t, port, chn, ME_CONTROLLER, CTRL_HRPN, nvh & 0x7f)))
          return false;
///        t += 1;  
      }
      if(nvl != 0xff)
      {
        if(!queueEvent(MidiPlayEvent(t, port, chn, ME_CONTROLLER, CTRL_LRPN, nvl & 0x7f)))
          return false;
      }    
    }
    else if (a < CTRL_NONE_OFFSET) 
    {     // NRPN14 Controller
      int ctrlH = (a >> 8) & 0x7f;
      int ctrlL = a & 0x7f;
      int dataH = (b >> 7) & 0x7f;
      int dataL = b & 0x7f;
      if(!queueEvent(MidiPlayEvent(t,   port, chn, ME_CONTROLLER, CTRL_HNRPN, ctrlH)))
        return false;
///     t += 1;      
      if(!queueEvent(MidiPlayEvent(t, port, chn, ME_CONTROLLER, CTRL_LNRPN, ctrlL)))
        return false;
///     t += 1;      
      if(!queueEvent(MidiPlayEvent(t, port, chn, ME_CONTROLLER, CTRL_HDATA, dataH)))
        return false;
///     t += 1;      
      if(!queueEvent(MidiPlayEvent(t, port, chn, ME_CONTROLLER, CTRL_LDATA, dataL)))
        return false;
///     t += 1;      
    
      //t += 4;  
      //sendNullRPNParams(chn, true);
      if(nvh != 0xff)
      {
        if(!queueEvent(MidiPlayEvent(t, port, chn, ME_CONTROLLER, CTRL_HNRPN, nvh & 0x7f)))
          return false;
///        t += 1;  
      }
      if(nvl != 0xff)
      {
        if(!queueEvent(MidiPlayEvent(t, port, chn, ME_CONTROLLER, CTRL_LNRPN, nvl & 0x7f)))
         return false;
      }   
    }
    else 
    {
      if(MusEGlobal::debugMsg)
        printf("MidiJackDevice::processEvent: unknown controller type 0x%x\n", a);
      //return false;  // Just ignore it.
    }
  }
  else 
  {
    //queueEvent(MidiPlayEvent(t, port, chn, event));
    if(!queueEvent(event))
      return false;
  }
  
  return true;
}
    
//---------------------------------------------------------
//    processMidi 
//    Called from audio thread only.
//---------------------------------------------------------

void MidiJackDevice::processMidi()
{
  //bool stop = stopPending;  // Snapshots
  //bool seek = seekPending;  //
  //seekPending = stopPending = false;

  processStuckNotes();       
  
  // Don't process if the device is not assigned to a port.
  //if(_port == -1)
  //  return;
    
  void* port_buf = 0;
  if(_out_client_jackport && _writeEnable)  
  {
    port_buf = jack_port_get_buffer(_out_client_jackport, MusEGlobal::segmentSize);
    jack_midi_clear_buffer(port_buf);
  }  
  
  int port = midiPort();
  MidiPort* mp = port == -1 ? 0 : &MusEGlobal::midiPorts[port];

  /*
  bool is_playing = MusEGlobal::audio->isPlaying();  // TODO Check this. It includes LOOP1 and LOOP2 besides PLAY.
  //bool is_playing = MusEGlobal::audio->isPlaying() || MusEGlobal::audio->isStarting(); 
  int pos = MusEGlobal::audio->tickPos();
  bool ext_sync = MusEGlobal::extSyncFlag.value();

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
          putEvent(MidiPlayEvent(0, 0, ME_SYSEX, msg, mmcStopMsgLen));
        }
        
        // Send midi stop...
        if(si.MRTOut()) 
        {
          putEvent(MidiPlayEvent(0, 0, 0, ME_STOP, 0, 0));
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
          putEvent(MidiPlayEvent(0, 0, 0, ME_STOP, 0, 0));
          // Hm, try scheduling these for after stuck notes scheduled below...
          //putEvent(MidiPlayEvent(0, 0, 0, ME_SONGPOS, beat, 0));
          //if(is_playing)
          //  putEvent(MidiPlayEvent(0, 0, 0, ME_CONTINUE, 0, 0));
        }    
      }
    }    
  }
  
  if(stop || (seek && is_playing))
  {
    // Clear all notes and handle stuck notes...
    _playEvents.clear();
    for(iMPEvent i = _stuckNotes.begin(); i != _stuckNotes.end(); ++i) 
    {
      MidiPlayEvent ev = *i;
      ev.setTime(0);  // Schedule immediately.
      putEvent(ev);
    }
    _stuckNotes.clear();
  }

  if(mp)
  {
    MidiSyncInfo& si = mp->syncInfo();
    // Try scheduling these now for after stuck notes scheduled above...
    if(stop || seek)
    {
      // Reset sustain.
      for(int ch = 0; ch < MIDI_CHANNELS; ++ch) 
        if(mp->hwCtrlState(ch, CTRL_SUSTAIN) == 127) 
          putEvent(MidiPlayEvent(0, _port, ch, ME_CONTROLLER, CTRL_SUSTAIN, 0));
    }
    if(seek)
    {
      // Send new song position.
      if(!ext_sync && si.MRTOut())
      {
        int beat = (pos * 4) / MusEGlobal::config.division;
        putEvent(MidiPlayEvent(0, 0, 0, ME_SONGPOS, beat, 0));
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
      //  putEvent(MidiPlayEvent(0, 0, 0, ME_CONTINUE, 0, 0));
    }
  }
  */
  
  while(!eventFifo.isEmpty())
  {
    MidiPlayEvent e(eventFifo.peek()); 
    //printf("MidiJackDevice::processMidi FIFO event time:%d type:%d ch:%d A:%d B:%d\n", e.time(), e.type(), e.channel(), e.dataA(), e.dataB()); 
    // Try to process only until full, keep rest for next cycle. If no out client port or no write enable, eat up events.  p4.0.15 
    if(port_buf && !processEvent(e))  
      return;            // Give up. The Jack buffer is full. Nothing left to do.  
    eventFifo.remove();  // Successfully processed event. Remove it from FIFO.
  }
  
  //if(!(stop || (seek && is_playing)))
  //  processStuckNotes();  
  
  if(_playEvents.empty())
    return;
  
  iMPEvent i = _playEvents.begin();     
  for(; i != _playEvents.end(); ++i) 
  {
    //printf("MidiJackDevice::processMidi playEvent time:%d type:%d ch:%d A:%d B:%d\n", i->time(), i->type(), i->channel(), i->dataA(), i->dataB()); 
    // Update hardware state so knobs and boxes are updated. Optimize to avoid re-setting existing values.   
    if(mp && !mp->sendHwCtrlState(*i, true)) // Force the event to be sent.
      continue;
  
    // Try to process only until full, keep rest for next cycle. If no out client port or no write enable, eat up events.  p4.0.15 
    if(port_buf && !processEvent(*i)) 
      break;
  }
  _playEvents.erase(_playEvents.begin(), i);
}

/*
//---------------------------------------------------------
//   handleStop
//---------------------------------------------------------

void MidiJackDevice::handleStop()
{
  // If the device is not in use by a port, don't bother it.
  if(_port == -1)
    return;
    
  stopPending = true;  // Trigger stop handling in processMidi.
  
//   //---------------------------------------------------
//   //    reset sustain
//   //---------------------------------------------------
//   
//   MidiPort* mp = &MusEGlobal::midiPorts[_port];
//   for(int ch = 0; ch < MIDI_CHANNELS; ++ch) 
//   {
//     if(mp->hwCtrlState(ch, CTRL_SUSTAIN) == 127) 
//     {
//       //printf("send clear sustain!!!!!!!! port %d ch %d\n", i,ch);
//       MidiPlayEvent ev(0, _port, ch, ME_CONTROLLER, CTRL_SUSTAIN, 0);
//       putEvent(ev);
//       // Do sendEvent to get the optimizations - send only on a change of value.
//       //mp->sendEvent(ev);
//     }
//   }
  
//   //---------------------------------------------------
//   //    send midi stop
//   //---------------------------------------------------
//   
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
//       
//       // Added check of option send continue not start.    p3.3.31
//       // Hmm, is this required? Seems to make other devices unhappy.
//       // (Could try now that this is in MidiDevice. p4.0.22 )
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

void MidiJackDevice::handleSeek()
{
  // If the device is not in use by a port, don't bother it.
  if(_port == -1)
    return;
  
  seekPending = true;  // Trigger seek handling in processMidi.
  
  //MidiPort* mp = &MusEGlobal::midiPorts[_port];
  //MidiCtrlValListList* cll = mp->controller();
  //int pos = MusEGlobal::audio->tickPos();
  
  //---------------------------------------------------
  //    Send new contoller values
  //---------------------------------------------------
    
//   for(iMidiCtrlValList ivl = cll->begin(); ivl != cll->end(); ++ivl) 
//   {
//     MidiCtrlValList* vl = ivl->second;
//     iMidiCtrlVal imcv = vl->iValue(pos);
//     if(imcv != vl->end()) 
//     {
//       Part* p = imcv->second.part;
//       //printf("MidiAlsaDevice::handleSeek _port:%d ctl:%d num:%d val:%d\n", _port, ivl->first >> 24, vl->num(), imcv->second.val); 
//       unsigned t = (unsigned)imcv->first;
//       // Do not add values that are outside of the part.
//       if(p && t >= p->tick() && t < (p->tick() + p->lenTick()) )
//         // Keep this and the section in processMidi() just in case we need to revert...
//         //_playEvents.add(MidiPlayEvent(0, _port, ivl->first >> 24, ME_CONTROLLER, vl->num(), imcv->second.val));
//         // Hmm, play event list for immediate playback? Try putEvent, putMidiEvent, or sendEvent (for the optimizations) instead. 
//         mp->sendEvent(MidiPlayEvent(0, _port, ivl->first >> 24, ME_CONTROLLER, vl->num(), imcv->second.val));
//     }
//   }
  
  //---------------------------------------------------
  //    Send STOP and "set song position pointer"
  //---------------------------------------------------
    
//   // Don't send if external sync is on. The master, and our sync routing system will take care of that.  p3.3.31
//   if(!MusEGlobal::extSyncFlag.value())
//   {
//     if(mp->syncInfo().MRTOut())
//     {
//       // Shall we check for device write open flag to see if it's ok to send?...
//       // This means obey what the user has chosen for read/write in the midi port config dialog,
//       //  which already takes into account whether the device is writable or not.
//       //if(!(rwFlags() & 0x1) || !(openFlags() & 1))
//       //if(!(openFlags() & 1))
//       //  continue;
//       
//       int beat = (pos * 4) / MusEGlobal::config.division;
//         
//       //bool isPlaying = (state == PLAY);
//       bool isPlaying = MusEGlobal::audio->isPlaying();  // TODO Check this it includes LOOP1 and LOOP2 besides PLAY.  p4.0.22
//         
//       mp->sendStop();
//       mp->sendSongpos(beat);
//       // REMOVE Tim. This is redundant and too early - Audio::startRolling already properly sends it when sync ready.
//       //if(isPlaying)
//       //  mp->sendContinue();
//     }    
//   }
}
*/

//---------------------------------------------------------
//   initMidiJack
//    return true on error
//---------------------------------------------------------

bool initMidiJack()
{
  return false;
}

} // namespace MusECore
