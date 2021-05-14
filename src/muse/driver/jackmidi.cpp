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

#include <QByteArray>

#include <stdio.h>
#include <string.h>

#include <jack/jack.h>

#include "jackmidi.h"
#include "jackaudio.h"
#include "song.h"
#include "globals.h"
#include "midi_consts.h"
#include "mididev.h"
#include "../midiport.h"
#include "../midiseq.h"
#include "../midictrl.h"
#include "../audio.h"
#include "minstrument.h"
#include "mpevent.h"
#include "sync.h"
#include "audiodev.h"
#include "../mplugins/midiitransform.h"
#include "../mplugins/mitplugin.h"
#include "gconfig.h"
#include "track.h"
#include "route.h"
#include "helper.h"

// Forwards from header:
#include "xml.h"

// Turn on debug messages.
//#define JACK_MIDI_DEBUG

// For debugging output: Uncomment the fprintf section.
#define DEBUG_PRST_ROUTES(dev, format, args...) // fprintf(dev, format, ##args);

namespace MusECore {

//---------------------------------------------------------
//   MidiJackDevice
//   in_jack_port or out_jack_port can be null
//---------------------------------------------------------

MidiJackDevice::MidiJackDevice(const QString& n)
   : MidiDevice(n)
{
  _in_client_jackport  = nullptr;
  _out_client_jackport = nullptr;
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
      name = QString("jack-midi-") + QString::number(ni);
      if(!MusEGlobal::midiDevices.find(name, JACK_MIDI))
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
  
  // Start by disabling for now.
  _writeEnable = _readEnable = false;
  if(!MusEGlobal::checkAudioDevice())
  {
    fprintf(stderr, "MusE: MidiJackDevice::open failed: No audio device\n"); 
    _state = QString("Not ready");
    return _state;
  }
   
  QString s;
  bool out_fail = false, in_fail = false;
  if(_openFlags & 1)
  {
    if(!_out_client_jackport)
    {
      if(MusEGlobal::audioDevice->deviceType() == AudioDevice::JACK_AUDIO)       
      {
        s = name() + QString(JACK_MIDI_OUT_PORT_SUFFIX);
        QByteArray ba = s.toLatin1();
        const char* cs = ba.constData();
        DEBUG_PRST_ROUTES(stderr, "MusE: MidiJackDevice::open creating output port name %s\n", cs);
        _out_client_jackport = (jack_port_t*)MusEGlobal::audioDevice->registerOutPort(cs, true);   
        if(!_out_client_jackport)   
        {
          fprintf(stderr, "MusE: MidiJackDevice::open failed creating output port name %s\n", cs); 
          _writeEnable = false;
          out_fail = true;
        }
        else
        {
          _writeEnable = true;
          const char* our_port_name = MusEGlobal::audioDevice->canonicalPortName(_out_client_jackport);
          if(our_port_name)
          {
            // (We just registered the port. At this point, any existing persistent routes' jackPort SHOULD be 0.)
            for(iRoute ir = _outRoutes.begin(); ir != _outRoutes.end(); ++ir)
            {
              if(ir->type != Route::JACK_ROUTE)
                continue;
              const char* route_name = ir->persistentJackPortName;
              if(!ir->jackPort)
                ir->jackPort = MusEGlobal::audioDevice->findPort(route_name);
              //if(!MusEGlobal::audioDevice->portConnectedTo(our_port, route_name))
              if(ir->jackPort)
                MusEGlobal::audioDevice->connect(our_port_name, route_name);
            }  
          }
        }
      }  
    }  
  }
  else
  {
    _writeEnable = false;
    if(_out_client_jackport)
    {
      DEBUG_PRST_ROUTES(stderr, "MusE: MidiJackDevice::open unregistering output port\n");
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
      for(iRoute ir = _outRoutes.begin(); ir != _outRoutes.end(); ++ir)
      {
        if(ir->type != Route::JACK_ROUTE)
          continue;
        if(ir->jackPort)
        {
          // Before we nullify the jackPort, grab the latest valid name of the port.
          MusEGlobal::audioDevice->portName(ir->jackPort, ir->persistentJackPortName, ROUTE_PERSISTENT_NAME_SIZE);
          ir->jackPort = 0;
        }
      }  
      
      MusEGlobal::audioDevice->unregisterPort(_out_client_jackport);
      _out_client_jackport = nullptr;  
    }  
  }
  
  if(_openFlags & 2)
  {  
    if(!_in_client_jackport)
    {
      if(MusEGlobal::audioDevice->deviceType() == AudioDevice::JACK_AUDIO)       
      {
        s = name() + QString(JACK_MIDI_IN_PORT_SUFFIX);
        QByteArray ba = s.toLatin1();
        const char* cs = ba.constData();
        DEBUG_PRST_ROUTES(stderr, "MusE: MidiJackDevice::open creating input port name %s\n", cs);
        _in_client_jackport = (jack_port_t*)MusEGlobal::audioDevice->registerInPort(cs, true);   
        if(!_in_client_jackport)    
        {
          fprintf(stderr, "MusE: MidiJackDevice::open failed creating input port name %s\n", cs);
          _readEnable = false;
          in_fail = true;
        }  
        else
        {
          _readEnable = true;
          const char* our_port_name = MusEGlobal::audioDevice->canonicalPortName(_in_client_jackport);
          if(our_port_name)
          {
            // (We just registered the port. At this point, any existing persistent routes' jackPort SHOULD be 0.)
            for(iRoute ir = _inRoutes.begin(); ir != _inRoutes.end(); ++ir) 
            {  
              if(ir->type != Route::JACK_ROUTE)  
                continue;
              const char* route_name = ir->persistentJackPortName;
              if(!ir->jackPort)
                ir->jackPort = MusEGlobal::audioDevice->findPort(route_name);
              //if(!MusEGlobal::audioDevice->portConnectedTo(our_port, route_name))
              if(ir->jackPort)
                MusEGlobal::audioDevice->connect(route_name, our_port_name);
            }
          }
        }
      }
    }  
  }
  else
  {
    _readEnable = false;
    if(_in_client_jackport)
    {
      DEBUG_PRST_ROUTES(stderr, "MusE: MidiJackDevice::open unregistering input port\n");
      for(iRoute ir = _inRoutes.begin(); ir != _inRoutes.end(); ++ir)
      {
        if(ir->type != Route::JACK_ROUTE)
          continue;
        if(ir->jackPort)
        {
          // Before we nullify the jackPort, grab the latest valid name of the port.
          MusEGlobal::audioDevice->portName(ir->jackPort, ir->persistentJackPortName, ROUTE_PERSISTENT_NAME_SIZE);
          ir->jackPort = 0;
        }
      }  
      
      MusEGlobal::audioDevice->unregisterPort(_in_client_jackport);
      _in_client_jackport = nullptr;  
    }  
  }
    
  if(out_fail && in_fail)
    _state = QString("R+W Open fail");
  else if(out_fail)
    _state = QString("Write open fail");
  else if(in_fail)
    _state = QString("Read open fail");
  else
    _state = QString("OK");
  
  return _state;
}

//---------------------------------------------------------
//   close
//---------------------------------------------------------

void MidiJackDevice::close()
{
  #ifdef JACK_MIDI_DEBUG
  printf("MidiJackDevice::close %s\n", name().toLatin1().constData());
  #endif  
  
  DEBUG_PRST_ROUTES(stderr, "MidiJackDevice::close %s\n", name().toLatin1().constData());
  // Disable immediately.
  _writeEnable = _readEnable = false;
  jack_port_t* i_jp = _in_client_jackport;
  jack_port_t* o_jp = _out_client_jackport;
  _in_client_jackport = 0;
  _out_client_jackport = 0;
  
  DEBUG_PRST_ROUTES(stderr, "MidiJackDevice::close nullifying route jackPorts...\n");
  
  for(iRoute ir = _outRoutes.begin(); ir != _outRoutes.end(); ++ir)
  {
    if(ir->type != Route::JACK_ROUTE)
      continue;
    if(ir->jackPort)
    {
      // Before we nullify the jackPort, grab the latest valid name of the port.
      if(MusEGlobal::checkAudioDevice())
        MusEGlobal::audioDevice->portName(ir->jackPort, ir->persistentJackPortName, ROUTE_PERSISTENT_NAME_SIZE);
      ir->jackPort = 0;
    }
  }  
  for(iRoute ir = _inRoutes.begin(); ir != _inRoutes.end(); ++ir)
  {
    if(ir->type != Route::JACK_ROUTE)
      continue;
    if(ir->jackPort)
    {
      // Before we nullify the jackPort, grab the latest valid name of the port.
      if(MusEGlobal::checkAudioDevice())
        MusEGlobal::audioDevice->portName(ir->jackPort, ir->persistentJackPortName, ROUTE_PERSISTENT_NAME_SIZE);
      ir->jackPort = 0;
    }
  }  

//   if(_in_client_jackport)
//   {
//     if(MusEGlobal::checkAudioDevice())
//       MusEGlobal::audioDevice->unregisterPort(_in_client_jackport);
//     _in_client_jackport = 0;
//   }
//   if(_out_client_jackport)
//   {
//     if(MusEGlobal::checkAudioDevice())
//       MusEGlobal::audioDevice->unregisterPort(_out_client_jackport);
//     _out_client_jackport = 0;
//   }
  
  DEBUG_PRST_ROUTES(stderr, "MidiJackDevice::close unregistering our ports...\n");
  
  if(i_jp)
  {
    if(MusEGlobal::checkAudioDevice())
      MusEGlobal::audioDevice->unregisterPort(i_jp);
  }
  if(o_jp)
  {
    if(MusEGlobal::checkAudioDevice())
      MusEGlobal::audioDevice->unregisterPort(o_jp);
  }
  _state = QString("Closed");
}

//---------------------------------------------------------
//   writeRouting
//---------------------------------------------------------

void MidiJackDevice::writeRouting(int level, Xml& xml) const
{
      // If this device is not actually in use by the song, do not write any routes.
      // This prevents bogus routes from being saved and propagated in the med file.
      // Removed. Need to let routes be saved.
      //if(midiPort() == -1)
      //  return;
      
      QString s;
      if(rwFlags() & 2)  // Readable
      {
        for (ciRoute r = _inRoutes.begin(); r != _inRoutes.end(); ++r) 
        {
          if((r->type == Route::TRACK_ROUTE && r->track) || (r->type != Route::TRACK_ROUTE && !r->name().isEmpty()))
          {
            xml.tag(level++, "Route");
            s = "source";
            if(r->type == Route::TRACK_ROUTE)
              s += QString(" track=\"%1\"/").arg(MusEGlobal::song->tracks()->index(r->track));
            else
              s += QString(" type=\"%1\" name=\"%2\"/").arg(r->type).arg(Xml::xmlString(r->name()));
            xml.tag(level, s.toLatin1().constData());
            xml.tag(level, "dest devtype=\"%d\" name=\"%s\"/", MidiDevice::JACK_MIDI, Xml::xmlString(name()).toLatin1().constData());
            xml.etag(level--, "Route");
          }
        }  
      } 
      
      for (ciRoute r = _outRoutes.begin(); r != _outRoutes.end(); ++r) 
      {
        if((r->type == Route::TRACK_ROUTE && r->track) || (r->type != Route::TRACK_ROUTE && !r->name().isEmpty()))
        {
          s = "Route";
          if(r->channel != -1)
            s += QString(" channel=\"%1\"").arg(r->channel);
          xml.tag(level++, s.toLatin1().constData());
          xml.tag(level, "source devtype=\"%d\" name=\"%s\"/", MidiDevice::JACK_MIDI, Xml::xmlString(name()).toLatin1().constData());
          s = "dest";
          if(r->type == Route::MIDI_DEVICE_ROUTE)
            s += QString(" devtype=\"%1\" name=\"%2\"/").arg(r->device->deviceType()).arg(Xml::xmlString(r->name()));
          else if(r->type == Route::TRACK_ROUTE)
            s += QString(" track=\"%1\"/").arg(MusEGlobal::song->tracks()->index(r->track));
          else
            s += QString(" type=\"%1\" name=\"%2\"/").arg(r->type).arg(Xml::xmlString(r->name()));
          xml.tag(level, s.toLatin1().constData());
          xml.etag(level--, "Route");
        }
      }
}

//---------------------------------------------------------
//   pbForwardShiftFrames
//---------------------------------------------------------

unsigned int MidiJackDevice::pbForwardShiftFrames() const
{
  return MusEGlobal::segmentSize;
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
            fprintf(stderr, "MidiIn Jack: <%s>: ", name().toLatin1().constData());
            dumpMPEvent(&event);
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
                                MusEGlobal::midiSyncContainer.mmcInput(_port, p, n);
                                return;
                                }
                          if (p[2] == 0x01) {
                                //mtcInputFull(p, n);
                                MusEGlobal::midiSyncContainer.mtcInputFull(_port, p, n);
                                return;
                                }
                          }
                    else if (p[0] == 0x7e) {
                          //nonRealtimeSystemSysex(p, n);
                          MusEGlobal::midiSyncContainer.nonRealtimeSystemSysex(_port, p, n);
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
      else if (MusEGlobal::rcEnableCC && typ == ME_CONTROLLER) {
          char cc = static_cast<char>(event.dataA() & 0xff);
          int value = static_cast<char>(event.dataB());
          printf("*** Input CC: %d Value: %d\n", cc, value);
          MusEGlobal::song->putEventCC(cc, value);
      }
      
      //if(_recordFifo.put(MidiPlayEvent(event)))
      //  printf("MidiJackDevice::recordEvent: fifo overflow\n");
      
      // Do not bother recording if it is NOT actually being used by a port.
      // Because from this point on, process handles things, by selected port.    p3.3.38
      if(_port == -1)
        return;
      
      // Split the events up into channel fifos. Special 'channel' number 17 for sysex events.
      unsigned int ch = (typ == ME_SYSEX)? MusECore::MUSE_MIDI_CHANNELS : event.channel();
      if(_recordFifo[ch].put(event))
        printf("MidiJackDevice::recordEvent: fifo channel %d overflow\n", ch);
      }

//---------------------------------------------------------
//   eventReceived
//---------------------------------------------------------

void MidiJackDevice::eventReceived(jack_midi_event_t* ev)
      {
      if(ev->size == 0)
        return;
      
      MidiRecordEvent event;
      event.setB(0);
      event.setPort(_port);
      jack_nframes_t abs_ft = 0;

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
      //  position, so that by the time of the NEXT process, THOSE events have also occurred in the previous period.
      // So, technically this is correct. What MATTERS is how we adjust the times for storage, and/or simultaneous playback in THIS period,
      //  and TEST: we'll need to make sure any non-contiguous previous period is handled correctly by process - will it work OK as is?
      // If ALSA works OK than this should too...
      // The events arrived in the previous cycle, not this one. Adjust.
      abs_ft = MusEGlobal::audio->curSyncFrame() + ev->time;
      if(abs_ft >= MusEGlobal::segmentSize)
        abs_ft -= MusEGlobal::segmentSize;
      event.setTime(abs_ft);
      event.setTick(MusEGlobal::lastExtMidiSyncTick);    

      event.setChannel(*(ev->buffer) & 0xf);
      const int type = *(ev->buffer) & 0xf0;
      event.setType(type);

      switch(type) {
            case ME_NOTEON:
            {
                 if(ev->size < 3)
                   return;
                 // Convert zero-velocity note ons to note offs as per midi spec.
                 if(*(ev->buffer + 2) == 0)
                   event.setType(ME_NOTEOFF);
            }
            // Fall through.
                
            case ME_NOTEOFF:
            case ME_CONTROLLER:
            case ME_POLYAFTER:
                  if(ev->size < 3)
                    return;
                  event.setA(*(ev->buffer + 1) & 0x7f);
                  event.setB(*(ev->buffer + 2) & 0x7f);
                  break;
            case ME_PROGRAM:
            case ME_AFTERTOUCH:
                  if(ev->size < 2)
                    return;
                  event.setA(*(ev->buffer + 1) & 0x7f);
                  break;

            case ME_PITCHBEND:
                  if(ev->size < 3)
                    return;
                  event.setA(( ((*(ev->buffer + 2) & 0x7f) << 7) + 
                                (*(ev->buffer + 1) & 0x7f) )
                              - 8192);
                  break;

            case ME_SYSEX:
                  {
                    const int type = *(ev->buffer) & 0xff;
                    switch(type) 
                    {
                          case ME_SYSEX:
                              #ifdef JACK_MIDI_DEBUG
                                // ---Diagnostics---:
                                fprintf(stderr, "MidiJackDevice::eventReceived SYSEX len:%u data: ", (unsigned int)ev->size);
                                for(unsigned int i = 0; i < ev->size && i < 16; ++i)
                                  fprintf(stderr, "%0x ", ((unsigned char*)ev->buffer)[i]);
                                if(ev->size >= 16) 
                                  fprintf(stderr, "..."); 
                                fprintf(stderr, "\n"); 
                              #endif
      
                                // TODO: Deal with large sysex, which are broken up into chunks!
                                // For now, do not accept if the last byte is not EOX, meaning it's a chunk with more chunks to follow.
                                if(*(((unsigned char*)ev->buffer) + ev->size - 1) != ME_SYSEX_END)
                                {
                                  fprintf(stderr, "MidiJackDevice::eventReceived sysex chunks not supported!\n");
                                  return;
                                }
                                
                                //event.setTime(0);      // mark as used
                                event.setType(ME_SYSEX);
                                event.setData((unsigned char*)(ev->buffer + 1), ev->size - 2);
                                break;
                          case ME_MTC_QUARTER:
                                if(_port != -1)
                                {
                                  MusEGlobal::midiSyncContainer.mtcInputQuarter(_port, *(ev->buffer + 1));
                                }
                                return;
                          case ME_SONGPOS:    
                                if(_port != -1)
                                {
                                  MusEGlobal::midiSyncContainer.setSongPosition(_port, *(ev->buffer + 1) | (*(ev->buffer + 2) << 7 )); // LSB then MSB
                                }
                                return;
                          //case ME_SONGSEL:    
                          //case ME_TUNE_REQ:

                          // We don't use sensing. But suppress warning about this one since it is repetitive.
                          case ME_SENSE:
                                return;

                          case ME_CLOCK:      
                          {
                                midiClockInput(abs_ft);
                                return;
                          }
                          case ME_START:
                          {
                            #ifdef JACK_MIDI_DEBUG
                              fprintf(stderr, "MidiJackDevice::eventReceived: START port:%d time:%u\n", _port, abs_ft);
                            #endif
                          }
                            // FALLTHROUGH

                          case ME_TICK:       
                          case ME_CONTINUE:   
                          case ME_STOP:       
                          {
                                MusEGlobal::midiSyncContainer.realtimeSystemInput(_port, type);
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
                  break;
            default:
              if(MusEGlobal::debugMsg)
                printf("MidiJackDevice::eventReceived unknown event 0x%02x\n", type);
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
//   queueEvent
//   return true if successful
//---------------------------------------------------------

bool MidiJackDevice::queueEvent(const MidiPlayEvent& e, void* evBuffer)
{
      // Perhaps we can find use for this value later, together with the Jack midi MusE port(s).
      // No big deal if not. Not used for now.
      //int port = e.port();
      
      if(!_writeEnable || !evBuffer)   
        return false;

      const unsigned int syncFrame = MusEGlobal::audio->curSyncFrame();
      if(e.time() != 0 && e.time() < syncFrame)
        fprintf(stderr, "MidiJackDevice::queueEvent() evTime:%u < syncFrame:%u!!\n", e.time(), syncFrame);
      unsigned int ft = (e.time() < syncFrame) ? 0 : e.time() - syncFrame;
      if (ft >= MusEGlobal::segmentSize) {
            fprintf(stderr, "MidiJackDevice::queueEvent: Event time:%d out of range. syncFrame:%d ft:%d (seg=%d)\n", 
                    e.time(), syncFrame, ft, MusEGlobal::segmentSize);
            ft = MusEGlobal::segmentSize - 1;
            }
      
      #ifdef JACK_MIDI_DEBUG
      fprintf(stderr, "MidiJackDevice::queueEvent pos:%d syncFrame:%d ft:%d time:%d type:%d ch:%d A:%d B:%d\n", 
              pos, syncFrame, ft, e.time(), e.type(), e.channel(), e.dataA(), e.dataB());
      #endif  
      
      if (MusEGlobal::midiOutputTrace) {
            fprintf(stderr, "MidiOut: Jack: <%s>: ", name().toLatin1().constData());
            dumpMPEvent(&e);
            }
            
      switch(e.type()) {


            case ME_CONTROLLER:
            case ME_NOTEON:
            case ME_NOTEOFF:
            case ME_POLYAFTER:
            case ME_PITCHBEND:
                  {
                  #ifdef JACK_MIDI_DEBUG
                  printf("MidiJackDevice::queueEvent note on/off polyafter controller or pitch\n");
                  #endif  
                    
                  unsigned char* p = jack_midi_event_reserve(evBuffer, ft, 3);
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
                    
                  unsigned char* p = jack_midi_event_reserve(evBuffer, ft, 2);
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
                  
                  const unsigned char* data = e.constData();
                  int len = e.len();
                  unsigned char* p = jack_midi_event_reserve(evBuffer, ft, len+2);
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
                  memcpy(p+1, data, len);
                  p[len+1] = 0xf7;
                  }
                  break;
            case ME_SONGPOS:
                  {
                  #ifdef JACK_MIDI_DEBUG
                  printf("MidiJackDevice::queueEvent songpos %d\n", e.dataA());
                  #endif  
                    
                  unsigned char* p = jack_midi_event_reserve(evBuffer, ft, 3);
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
                    
                  unsigned char* p = jack_midi_event_reserve(evBuffer, ft, 1);
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
            }
            
            return true;
}
      
//---------------------------------------------------------
//    processEvent
//    return true if successful
//---------------------------------------------------------

bool MidiJackDevice::processEvent(const MidiPlayEvent& event, void* evBuffer)
{    
  int chn    = event.channel();
  unsigned t = event.time();
  int a      = event.dataA();
  int b      = event.dataB();
  // Perhaps we can find use for this value later, together with the Jack midi MusE port(s).
  // No big deal if not. Not used for now.
  int port   = event.port();

  #ifdef JACK_MIDI_DEBUG
  //printf("MidiJackDevice::processEvent time:%d type:%d ch:%d A:%d B:%d\n", t, event.type(), chn, a, b);  
  #endif  
      
  MidiInstrument::NoteOffMode nom = MidiInstrument::NoteOffAll; // Default to NoteOffAll in case of no port.
  const int mport = midiPort();
  if(mport != -1)
  {
    if(MidiInstrument* mi = MusEGlobal::midiPorts[mport].instrument())
      nom = mi->noteOffMode();
  }
  
  // REMOVE Tim. Noteoff. Added.
  if(event.type() == ME_NOTEON)
  {
    if(b == 0)
    {
      // Handle zero-velocity note ons. Technically this is an error because internal midi paths
      //  are now all 'note-off' without zero-vel note ons - they're converted to note offs.
      // Nothing should be setting a Note type Event's on velocity to zero.
      // But just in case... If we get this warning, it means there is still code to change.
      fprintf(stderr, "MidiJackDevice::processEvent: Warning: Zero-vel note on: time:%d type:%d (ME_NOTEON) ch:%d A:%d B:%d\n", t, event.type(), chn, a, b);  
      switch(nom)
      {
        // Instrument uses note offs. Convert to zero-vel note off.
        case MidiInstrument::NoteOffAll:
          return queueEvent(MidiPlayEvent(t, port, chn, ME_NOTEOFF, a, 0), evBuffer);
        break;
        
        // Instrument uses no note offs at all. Send as-is.
        case MidiInstrument::NoteOffNone:
        // Instrument converts all note offs to zero-vel note ons. Send as-is.
        case MidiInstrument::NoteOffConvertToZVNoteOn:
          return queueEvent(event, evBuffer);
        break;
      }
    }
    return queueEvent(event, evBuffer);
  }
  else if(event.type() == ME_NOTEOFF)
  {
    switch(nom)
    {
      // Instrument uses note offs. Send as-is.
      case MidiInstrument::NoteOffAll:
        return queueEvent(event, evBuffer);
      break;
      
      // Instrument uses no note offs at all. Send nothing. Eat up the event - return true.
      case MidiInstrument::NoteOffNone:
        return true;
        
      // Instrument converts all note offs to zero-vel note ons. Convert to zero-vel note on.
      case MidiInstrument::NoteOffConvertToZVNoteOn:
        return queueEvent(MidiPlayEvent(t, port, chn, ME_NOTEON, a, 0), evBuffer);
      break;
    }
    return queueEvent(event, evBuffer);
  }
  
  else if(event.type() == ME_PROGRAM) 
  {
    //_curOutParamNums[chn].resetParamNums();  // Probably best to reset.
    // don't output program changes for GM drum channel
    //if (!(MusEGlobal::song->mtype() == MT_GM && chn == 9)) {
          _curOutParamNums[chn].resetParamNums();  // Probably best to reset.
          _curOutParamNums[chn].setPROG(a);
          if(!queueEvent(MidiPlayEvent(t, port, chn, ME_PROGRAM, a, 0), evBuffer))
            return false;
          
    //      }
  }
  else if(event.type() == ME_PITCHBEND)
  {
      int v = a + 8192;
      //printf("MidiJackDevice::processEvent ME_PITCHBEND v:%d time:%d type:%d ch:%d A:%d B:%d\n", v, event.time(), event.type(), event.channel(), event.dataA(), event.dataB());
      if(!queueEvent(MidiPlayEvent(t, port, chn, ME_PITCHBEND, v & 0x7f, (v >> 7) & 0x7f), evBuffer))
        return false;
  }
  else if(event.type() == ME_SYSEX)
  {
    resetCurOutParamNums();  // Probably best to reset all.
    if(!queueEvent(event, evBuffer))
      return false;
  }
  else if(event.type() == ME_CONTROLLER)
  {
    // Perhaps we can find use for this value later, together with the Jack midi MusE port(s).
    // No big deal if not. Not used for now.
    //int port   = event.port();

    if((a | 0xff) == CTRL_POLYAFTER) 
    {
      //printf("MidiJackDevice::processEvent CTRL_POLYAFTER v:%d time:%d type:%d ch:%d A:%d B:%d\n", v, event.time(), event.type(), event.channel(), event.dataA(), event.dataB());
      if(!queueEvent(MidiPlayEvent(t, port, chn, ME_POLYAFTER, a & 0x7f, b & 0x7f), evBuffer))
        return false;
    }
    else if(a == CTRL_AFTERTOUCH) 
    {
      //printf("MidiJackDevice::processEvent CTRL_AFTERTOUCH v:%d time:%d type:%d ch:%d A:%d B:%d\n", v, event.time(), event.type(), event.channel(), event.dataA(), event.dataB());
      if(!queueEvent(MidiPlayEvent(t, port, chn, ME_AFTERTOUCH, b & 0x7f, 0), evBuffer))
        return false;
    }
    else if(a == CTRL_PITCH) 
    {
      int v = b + 8192;
      //printf("MidiJackDevice::processEvent CTRL_PITCH v:%d time:%d type:%d ch:%d A:%d B:%d\n", v, event.time(), event.type(), event.channel(), event.dataA(), event.dataB());
      if(!queueEvent(MidiPlayEvent(t, port, chn, ME_PITCHBEND, v & 0x7f, (v >> 7) & 0x7f), evBuffer))
        return false;
    }
    else if (a == CTRL_PROGRAM) 
    {
      _curOutParamNums[chn].resetParamNums();  // Probably best to reset.
      // don't output program changes for GM drum channel
      //if (!(MusEGlobal::song->mtype() == MT_GM && chn == 9)) {
            int hb = (b >> 16) & 0xff;
            int lb = (b >> 8) & 0xff;
            int pr = b & 0xff;
            //printf("MidiJackDevice::processEvent CTRL_PROGRAM time:%d type:%d ch:%d A:%d B:%d hb:%d lb:%d pr:%d\n", 
            //       event.time(), event.type(), event.channel(), event.dataA(), event.dataB(), hb, lb, pr);

            _curOutParamNums[chn].setCurrentProg(pr, lb, hb);
            if (hb != 0xff)
            {
                  if(!queueEvent(MidiPlayEvent(t, port, chn, ME_CONTROLLER, CTRL_HBANK, hb), evBuffer))
                    return false;
            }
            if (lb != 0xff)
            {
                  if(!queueEvent(MidiPlayEvent(t, port, chn, ME_CONTROLLER, CTRL_LBANK, lb), evBuffer))
                    return false;
            }
            if (pr != 0xff)
            {
                  if(!queueEvent(MidiPlayEvent(t, port, chn, ME_PROGRAM, pr, 0), evBuffer))
                    return false;
            }
              
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
      if(!queueEvent(MidiPlayEvent(t, port, ME_SYSEX, sysex, 6), evBuffer))
        return false;
    }
    else if (a < CTRL_14_OFFSET) 
    {              // 7 Bit Controller
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
        _curOutParamNums[chn].setBANKL(b);
        _curOutParamNums[chn].resetParamNums();  // Probably best to reset.
      }
      else if(a == CTRL_RESET_ALL_CTRL) 
        _curOutParamNums[chn].resetParamNums();  // Probably best to reset.

      //queueEvent(museport, MidiPlayEvent(t, port, chn, event));
      if(!queueEvent(event, evBuffer))
        return false;
    }
    else if (a < CTRL_RPN_OFFSET) 
    {     // 14 bit high resolution controller
      int ctrlH = (a >> 8) & 0x7f;
      int ctrlL = a & 0x7f;
      int dataH = (b >> 7) & 0x7f;
      int dataL = b & 0x7f;
      if(!queueEvent(MidiPlayEvent(t,   port, chn, ME_CONTROLLER, ctrlH, dataH), evBuffer))
        return false;
      if(!queueEvent(MidiPlayEvent(t, port, chn, ME_CONTROLLER, ctrlL, dataL), evBuffer))
        return false;
    }
    else if (a < CTRL_NRPN_OFFSET) 
    {     // RPN 7-Bit Controller
      int ctrlH = (a >> 8) & 0x7f;
      int ctrlL = a & 0x7f;
      int data = b & 0x7f;
      if(ctrlH != _curOutParamNums[chn].RPNH || !MusEGlobal::config.midiOptimizeControllers)
      {
        _curOutParamNums[chn].setRPNH(ctrlH);
        if(!queueEvent(MidiPlayEvent(t,   port, chn, ME_CONTROLLER, CTRL_HRPN, ctrlH), evBuffer))
          return false;
      }
      if(ctrlL != _curOutParamNums[chn].RPNL || !MusEGlobal::config.midiOptimizeControllers)
      {
        _curOutParamNums[chn].setRPNL(ctrlL);
        if(!queueEvent(MidiPlayEvent(t, port, chn, ME_CONTROLLER, CTRL_LRPN, ctrlL), evBuffer))
          return false;
      }
      if(data != _curOutParamNums[chn].DATAH || !MusEGlobal::config.midiOptimizeControllers)
      {
        _curOutParamNums[chn].setDATAH(data);
        if(!queueEvent(MidiPlayEvent(t, port, chn, ME_CONTROLLER, CTRL_HDATA, data), evBuffer))
          return false;
      }
      
      // Select null parameters so that subsequent data controller events do not upset the last *RPN controller.
      if(MusEGlobal::config.midiSendNullParameters)
      {
        _curOutParamNums[chn].setRPNH(0x7f);
        if(!queueEvent(MidiPlayEvent(t, port, chn, ME_CONTROLLER, CTRL_HRPN, 0x7f), evBuffer))
          return false;
        
        _curOutParamNums[chn].setRPNL(0x7f);
        if(!queueEvent(MidiPlayEvent(t, port, chn, ME_CONTROLLER, CTRL_LRPN, 0x7f), evBuffer))
          return false;
      }
    }
    else if (a < CTRL_INTERNAL_OFFSET) 
    {     // NRPN 7-Bit Controller
      int ctrlH = (a >> 8) & 0x7f;
      int ctrlL = a & 0x7f;
      int data = b & 0x7f;
      if(ctrlH != _curOutParamNums[chn].NRPNH || !MusEGlobal::config.midiOptimizeControllers)
      {
        _curOutParamNums[chn].setNRPNH(ctrlH);
        if(!queueEvent(MidiPlayEvent(t,   port, chn, ME_CONTROLLER, CTRL_HNRPN, ctrlH), evBuffer))
          return false;
      }
      if(ctrlL != _curOutParamNums[chn].NRPNL || !MusEGlobal::config.midiOptimizeControllers)
      {
        _curOutParamNums[chn].setNRPNL(ctrlL);
        if(!queueEvent(MidiPlayEvent(t, port, chn, ME_CONTROLLER, CTRL_LNRPN, ctrlL), evBuffer))
          return false;
      }
      if(data != _curOutParamNums[chn].DATAH || !MusEGlobal::config.midiOptimizeControllers)
      {
        _curOutParamNums[chn].setDATAH(data);
        if(!queueEvent(MidiPlayEvent(t, port, chn, ME_CONTROLLER, CTRL_HDATA, data), evBuffer))
          return false;
      }
                  
      if(MusEGlobal::config.midiSendNullParameters)
      {
        _curOutParamNums[chn].setNRPNH(0x7f);
        if(!queueEvent(MidiPlayEvent(t, port, chn, ME_CONTROLLER, CTRL_HNRPN, 0x7f), evBuffer))
          return false;
        
        _curOutParamNums[chn].setNRPNL(0x7f);
        if(!queueEvent(MidiPlayEvent(t, port, chn, ME_CONTROLLER, CTRL_LNRPN, 0x7f), evBuffer))
          return false;
      }
    }
    else if (a < CTRL_RPN14_OFFSET)      // Unaccounted for internal controller
      return false;
    else if (a < CTRL_NRPN14_OFFSET) 
    {     // RPN14 Controller
      int ctrlH = (a >> 8) & 0x7f;
      int ctrlL = a & 0x7f;
      int dataH = (b >> 7) & 0x7f;
      int dataL = b & 0x7f;
      if(ctrlH != _curOutParamNums[chn].RPNH || !MusEGlobal::config.midiOptimizeControllers)
      {
        _curOutParamNums[chn].setRPNH(ctrlH);
        if(!queueEvent(MidiPlayEvent(t,   port, chn, ME_CONTROLLER, CTRL_HRPN, ctrlH), evBuffer))
          return false;
      }
      if(ctrlL != _curOutParamNums[chn].RPNL || !MusEGlobal::config.midiOptimizeControllers)
      {
        _curOutParamNums[chn].setRPNL(ctrlL);
        if(!queueEvent(MidiPlayEvent(t, port, chn, ME_CONTROLLER, CTRL_LRPN, ctrlL), evBuffer))
          return false;
      }
      if(dataH != _curOutParamNums[chn].DATAH || !MusEGlobal::config.midiOptimizeControllers)
      {
        _curOutParamNums[chn].setDATAH(dataH);
        if(!queueEvent(MidiPlayEvent(t, port, chn, ME_CONTROLLER, CTRL_HDATA, dataH), evBuffer))
          return false;
      }
      if(dataL != _curOutParamNums[chn].DATAL || !MusEGlobal::config.midiOptimizeControllers)
      {
        _curOutParamNums[chn].setDATAL(dataL);
        if(!queueEvent(MidiPlayEvent(t, port, chn, ME_CONTROLLER, CTRL_LDATA, dataL), evBuffer))
          return false;
      }
      
      if(MusEGlobal::config.midiSendNullParameters)
      {
        _curOutParamNums[chn].setRPNH(0x7f);
        if(!queueEvent(MidiPlayEvent(t, port, chn, ME_CONTROLLER, CTRL_HRPN, 0x7f), evBuffer))
          return false;
        
        _curOutParamNums[chn].setRPNL(0x7f);
        if(!queueEvent(MidiPlayEvent(t, port, chn, ME_CONTROLLER, CTRL_LRPN, 0x7f), evBuffer))
          return false;
      }
    }
    else if (a < CTRL_NONE_OFFSET) 
    {     // NRPN14 Controller
      int ctrlH = (a >> 8) & 0x7f;
      int ctrlL = a & 0x7f;
      int dataH = (b >> 7) & 0x7f;
      int dataL = b & 0x7f;
      if(ctrlH != _curOutParamNums[chn].NRPNH || !MusEGlobal::config.midiOptimizeControllers)
      {
        _curOutParamNums[chn].setNRPNH(ctrlH);
        if(!queueEvent(MidiPlayEvent(t,   port, chn, ME_CONTROLLER, CTRL_HNRPN, ctrlH), evBuffer))
          return false;
      }
      if(ctrlL != _curOutParamNums[chn].NRPNL || !MusEGlobal::config.midiOptimizeControllers)
      {
        _curOutParamNums[chn].setNRPNL(ctrlL);
        if(!queueEvent(MidiPlayEvent(t, port, chn, ME_CONTROLLER, CTRL_LNRPN, ctrlL), evBuffer))
          return false;
      }
      if(dataH != _curOutParamNums[chn].DATAH || !MusEGlobal::config.midiOptimizeControllers)
      {
        _curOutParamNums[chn].setDATAH(dataH);
        if(!queueEvent(MidiPlayEvent(t, port, chn, ME_CONTROLLER, CTRL_HDATA, dataH), evBuffer))
          return false;
      }
      if(dataL != _curOutParamNums[chn].DATAL || !MusEGlobal::config.midiOptimizeControllers)
      {
        _curOutParamNums[chn].setDATAL(dataL);
        if(!queueEvent(MidiPlayEvent(t, port, chn, ME_CONTROLLER, CTRL_LDATA, dataL), evBuffer))
          return false;
      }
    
      if(MusEGlobal::config.midiSendNullParameters)
      {
        _curOutParamNums[chn].setNRPNH(0x7f);
        if(!queueEvent(MidiPlayEvent(t, port, chn, ME_CONTROLLER, CTRL_HNRPN, 0x7f), evBuffer))
          return false;
        
        _curOutParamNums[chn].setNRPNL(0x7f);
        if(!queueEvent(MidiPlayEvent(t, port, chn, ME_CONTROLLER, CTRL_LNRPN, 0x7f), evBuffer))
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
    if(!queueEvent(event, evBuffer))
      return false;
  }
  
  return true;
}
    
//---------------------------------------------------------
//    processMidi 
//    Called from audio thread only.
//---------------------------------------------------------

void MidiJackDevice::processMidi(unsigned int curFrame)
{
  void* port_buf = 0;
  if(_out_client_jackport && _writeEnable)  
  {
    port_buf = jack_port_get_buffer(_out_client_jackport, MusEGlobal::segmentSize);
    jack_midi_clear_buffer(port_buf);
  }  
  
  // Get the state of the stop flag.
  const bool do_stop = stopFlag();

  MidiPlayEvent buf_ev;
  
  // Transfer the user lock-free buffer events to the user sorted multi-set.
  // False = don't use the size snapshot, but update it.
  const unsigned int usr_buf_sz = eventBuffers(UserBuffer)->getSize(false);
  for(unsigned int i = 0; i < usr_buf_sz; ++i)
  {
    if(eventBuffers(UserBuffer)->get(buf_ev))
      _outUserEvents.insert(buf_ev);
  }
  
  // Transfer the playback lock-free buffer events to the playback sorted multi-set.
  const unsigned int pb_buf_sz = eventBuffers(PlaybackBuffer)->getSize(false);
  for(unsigned int i = 0; i < pb_buf_sz; ++i)
  {
    // Are we stopping? Just remove the item.
    if(do_stop)
      eventBuffers(PlaybackBuffer)->remove();
    // Otherwise get the item.
    else if(eventBuffers(PlaybackBuffer)->get(buf_ev))
      _outPlaybackEvents.insert(buf_ev);
  }
  
  // Are we stopping?
  if(do_stop)
  {
    // Transport has stopped, purge ALL further scheduled playback events now.
    _outPlaybackEvents.clear();
    // Reset the flag.
    setStopFlag(false);
  }
  
  iMPEvent impe_pb = _outPlaybackEvents.begin();
  iMPEvent impe_us = _outUserEvents.begin();
  bool using_pb;
  
  while(1)
  {  
    if(impe_pb != _outPlaybackEvents.end() && impe_us != _outUserEvents.end())
      using_pb = *impe_pb < *impe_us;
    else if(impe_pb != _outPlaybackEvents.end())
      using_pb = true;
    else if(impe_us != _outUserEvents.end())
      using_pb = false;
    else break;
    
    const MidiPlayEvent& ev = using_pb ? *impe_pb : *impe_us;
    
    if(ev.time() >= (curFrame + MusEGlobal::segmentSize))
    {
      #ifdef JACK_MIDI_DEBUG
      fprintf(stderr, "MusE: Jack midi: putted event is for future:%lu, breaking loop now\n", ev.time() - curFrame);
      #endif
      break;
    }

    // If processEvent fails, although we would like to not miss events by keeping them
    //  until next cycle and trying again, that can lead to a large backup of events
    //  over a long time. So we'll just... miss them.
    processEvent(ev, port_buf);
    
    // Successfully processed event. Remove it from FIFO.
    // C++11.
    if(using_pb)
      impe_pb = _outPlaybackEvents.erase(impe_pb);
    else
      impe_us = _outUserEvents.erase(impe_us);
  }
}

//---------------------------------------------------------
//   portLatency
//   If capture is true get the capture latency,
//    otherwise get the playback latency.
//---------------------------------------------------------

unsigned int MidiJackDevice::portLatency(void* /*port*/, bool capture) const
{
//   jack_latency_range_t c_range;
//   jack_port_get_latency_range((jack_port_t*)port, JackCaptureLatency, &c_range);
//   jack_latency_range_t p_range;
//   jack_port_get_latency_range((jack_port_t*)port, JackPlaybackLatency, &p_range);

  // TODO FIXME: Tests on both Jack-1 Midi and Jack-2 Midi show the returned values are always zero.
  //             Spent a few days trying to diagnose, it appears Jack does not initialize any ALSA
  //              midi port latency values as it does with the audio ports. Thus right from the start,
  //              right from the backend physical port, the values passed throughout the system and
  //              to the app are always zero!
  
  // NOTICE: For at least the ALSA seq driver (tested), the input latency is
  //          always 1 period while the output latency is always 2 periods
  //          regardless of Jack command line -p (period size).
  //         (Also there is the user latency from command line or QJackCtl.)
  //         In other words, the Jack command line -p (number of periods) ONLY applies to audio output ports.
  
  //fprintf(stderr, "MidiJackDevice::portLatency port:%p capture:%d c_range.min:%d c_range.max:%d p_range.min:%d p_range.max:%d\n",
  //        port, capture, c_range.min, c_range.max, p_range.min, p_range.max);

  if(capture)
  {
//     jack_latency_range_t c_range;
//     jack_port_get_latency_range((jack_port_t*)port, JackCaptureLatency, &c_range);

// REMOVE Tim. latency. TESTING Reinstate. For simulating non-functional jack midi latency. This seems to work well.
//     return c_range.max;
    return MusEGlobal::segmentSize;
  }
  else
  {
//     jack_latency_range_t p_range;
//     jack_port_get_latency_range((jack_port_t*)port, JackPlaybackLatency, &p_range);

// REMOVE Tim. latency. TESTING Reinstate. For simulating non-functional jack midi latency. This seems to work well.
//     return p_range.max;
    return MusEGlobal::segmentSize * 2;
  }
}

//---------------------------------------------------------
//   selfLatencyMidi
//---------------------------------------------------------

float MidiJackDevice::selfLatencyMidi(int channel, bool capture) const
{
  float l = MidiDevice::selfLatencyMidi(channel, capture);

  //if(!MusEGlobal::checkAudioDevice())
  //  return l;

  if(capture)
  {
    if(_in_client_jackport)
      l += portLatency(_in_client_jackport, capture);
  }
  else
  {
    if(_out_client_jackport)
      l += portLatency(_out_client_jackport, capture);
  }
  return l;
}

//---------------------------------------------------------
//   initMidiJack
//    return true on error
//---------------------------------------------------------

bool initMidiJack()
{
  return false;
}

} // namespace MusECore
