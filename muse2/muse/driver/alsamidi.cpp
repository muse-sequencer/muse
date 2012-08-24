//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: alsamidi.cpp,v 1.8.2.7 2009/11/19 04:20:33 terminator356 Exp $
//  (C) Copyright 2000-2001 Werner Schweer (ws@seh.de)
//  (C) Copyright 2011 Tim E. Real (terminator356 on sourceforge)
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
//#include "mididev.h"
#include "../midiport.h"
#include "../midiseq.h"
#include "../midictrl.h"
#include "../audio.h"
//#include "mpevent.h"
//#include "sync.h"
#include "utils.h"
#include "audiodev.h"
#include "xml.h"
#include "part.h"
#include "gconfig.h"
#include "track.h"

#include <QApplication>

// Enable debugging:
//#define ALSA_DEBUG 1  


namespace MusECore {

static int alsaSeqFdi = -1;
static int alsaSeqFdo = -1;

snd_seq_t* alsaSeq = 0;
static snd_seq_addr_t musePort;
static snd_seq_addr_t announce_adr;

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

      snd_seq_port_info_t *pinfo;
      snd_seq_port_info_alloca(&pinfo);
      int rv = snd_seq_get_any_port_info(alsaSeq, adr.client, adr.port, pinfo);
      if(rv < 0)
      {  
        printf("MidiAlsaDevice::open Error getting port info: adr: %d:%d: %s\n", adr.client, adr.port, snd_strerror(rv));
        return QString(snd_strerror(rv));
      }
      
      snd_seq_port_subscribe_t* subs;
      // Allocated on stack, no need to call snd_seq_port_subscribe_free() later.
      snd_seq_port_subscribe_alloca(&subs);

      QString estr;
      int wer = 0;
      int rer = 0;

      int cap = snd_seq_port_info_get_capability(pinfo);

#ifdef ALSA_DEBUG
      printf("MidiAlsaDevice::open cap:%d\n", cap);  
#endif
      
      // subscribe for writing
      if (_openFlags & 1) 
      {
            if(cap & SND_SEQ_PORT_CAP_SUBS_WRITE)
            {  
              snd_seq_port_subscribe_set_sender(subs, &musePort);
              snd_seq_port_subscribe_set_dest(subs, &adr);
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
      

      if(wer < 0 || rer < 0)
        return estr;
        
      return QString("OK");
}

//---------------------------------------------------------
//   close
//---------------------------------------------------------

void MidiAlsaDevice::close()
{
      snd_seq_port_info_t *pinfo;
      snd_seq_port_info_alloca(&pinfo);
      int rv = snd_seq_get_any_port_info(alsaSeq, adr.client, adr.port, pinfo);
      if(rv < 0)
      {  
        printf("MidiAlsaDevice::close Error getting port info: adr: %d:%d: %s\n", adr.client, adr.port, snd_strerror(rv));
        return;
      }
      
      snd_seq_port_subscribe_t* subs;
      // Allocated on stack, no need to call snd_seq_port_subscribe_free() later.
      snd_seq_port_subscribe_alloca(&subs);
      
      int wer = 0;
      int rer = 0;

      int cap = snd_seq_port_info_get_capability(pinfo);

#ifdef ALSA_DEBUG
      printf("MidiAlsaDevice::close cap:%d\n", cap);  
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
                  printf("MidiAlsaDevice::close Error unsubscribing alsa midi port %d:%d for writing: %s\n", adr.client, adr.port, snd_strerror(wer));
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
                  printf("MidiAlsaDevice::close Error unsubscribing alsa midi port %d:%d for reading: %s\n", adr.client, adr.port, snd_strerror(rer));
              }  
              //else
              //  _readEnable = false;      
            }   
            _readEnable = false;      
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
//---------------------------------------------------------

bool MidiAlsaDevice::putMidiEvent(const MidiPlayEvent& e)
      {
      if (MusEGlobal::midiOutputTrace) {
            printf("MidiOut: Alsa: <%s>: ", name().toLatin1().constData());
            e.dump();
            }
      int chn = e.channel();
      int a   = e.dataA();
      int b   = e.dataB();

      snd_seq_event_t event;
      memset(&event, 0, sizeof(event));
      event.queue   = SND_SEQ_QUEUE_DIRECT;
      event.source  = musePort;
      event.dest    = adr;

      switch(e.type()) {
            case ME_NOTEON:
                  snd_seq_ev_set_noteon(&event, chn, a, b);
                  break;
            case ME_NOTEOFF:
                  snd_seq_ev_set_noteoff(&event, chn, a, 0);
                  break;
            case ME_PROGRAM:
                  snd_seq_ev_set_pgmchange(&event, chn, a);
                  break;
            case ME_CONTROLLER:
                  {
                    if(a == CTRL_PROGRAM)
                    {
                      snd_seq_ev_set_pgmchange(&event, chn, b);
                      break;
                    }
                    else if(a == CTRL_PITCH)
                    {
                      snd_seq_ev_set_pitchbend(&event, chn, b);
                      break;
                    }
                  }
                  
#if 1
                  snd_seq_ev_set_controller(&event, chn, a, b);
#else
                  {
                  int a   = e.dataA();
                  int b   = e.dataB();
                  int chn = e.channel();
                  if (a < CTRL_14_OFFSET) {          // 7 Bit Controller
                        snd_seq_ev_set_controller(&event, chn, a, b);
                        }
                  else if (a < CTRL_RPN_OFFSET) {     // 14 bit high resolution controller
                        int ctrlH = (a >> 8) & 0x7f;
                        int ctrlL = a & 0x7f;
                        a = (ctrlH << 7) + ctrlL;
                        snd_seq_ev_set_controller(&event, chn, a, b);
                        event.type = SND_SEQ_EVENT_CONTROL14;
                        }
                  else if (a < CTRL_NRPN_OFFSET) {     // RPN 7-Bit Controller
                        int ctrlH = (a >> 8) & 0x7f;
                        int ctrlL = a & 0x7f;
                        a = (ctrlH << 7) + ctrlL;
                        b <<= 7;
                        snd_seq_ev_set_controller(&event, chn, a, b);
                        event.type = SND_SEQ_EVENT_REGPARAM;
                        }
                  else if (a < CTRL_INTERNAL_OFFSET) {     // NRPN 7-Bit Controller
                        int ctrlH = (a >> 8) & 0x7f;
                        int ctrlL = a & 0x7f;
                        a = (ctrlH << 7) + ctrlL;
                        b <<= 7;
                        snd_seq_ev_set_controller(&event, chn, a, b);
                        event.type = SND_SEQ_EVENT_NONREGPARAM;
                        }
                  else if (a < CTRL_NRPN14_OFFSET) {     // RPN14 Controller
                        int ctrlH = (a >> 8) & 0x7f;
                        int ctrlL = a & 0x7f;
                        a = (ctrlH << 7) + ctrlL;
                        snd_seq_ev_set_controller(&event, chn, a, b);
                        event.type = SND_SEQ_EVENT_REGPARAM;
                        }
                  else if (a < CTRL_NONE_OFFSET) {     // NRPN14 Controller
                        int ctrlH = (a >> 8) & 0x7f;
                        int ctrlL = a & 0x7f;
                        a = (ctrlH << 7) + ctrlL;
                        snd_seq_ev_set_controller(&event, chn, a, b);
                        event.type = SND_SEQ_EVENT_NONREGPARAM;
                        }
                  else {
                        printf("putEvent: unknown controller type 0x%x\n", a);
                        }
                  }
#endif
                  break;
            case ME_PITCHBEND:
                  snd_seq_ev_set_pitchbend(&event, chn, a);
                  break;
            case ME_POLYAFTER:
                  // chnEvent2(chn, 0xa0, a, b);
                  break;
            case ME_AFTERTOUCH:
                  snd_seq_ev_set_chanpress(&event, chn, a);
                  break;
            case ME_SYSEX:
                  {
                  const unsigned char* p = e.data();
                  int n                  = e.len();
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
                  return putEvent(&event);
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
            default:
                  printf("MidiAlsaDevice::putEvent(): event type %d not implemented\n",
                     e.type());
                  return true;
            }
      return putEvent(&event);
      }

//---------------------------------------------------------
//   putEvent
//    return false if event is delivered
//---------------------------------------------------------

bool MidiAlsaDevice::putEvent(snd_seq_event_t* event)
      {
      int error;

#ifdef ALSA_DEBUG
      printf("MidiAlsaDevice::putEvent\n");  
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
                        fprintf(stderr, "MidiAlsaDevice::%p putEvent(): midi write error: %s\n",
                           this, snd_strerror(error));
                        fprintf(stderr, "  dst %d:%d\n", adr.client, adr.port);
                        //exit(-1);
                        }
                  }
            else
                  fprintf(stderr, "MidiAlsaDevice::putEvent(): midi write returns %d, expected %d: %s\n",
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
          if(putMidiEvent(*i))
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
      if (MusEGlobal::debugMsg)
            printf("initMidiAlsa\n");
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
            //printf( "ALSA client name: %s\n", cname);  
            
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
                  MidiAlsaDevice* dev = new MidiAlsaDevice(adr, QString(snd_seq_port_info_get_name(pinfo)));
                  int flags = 0;
                  if (capability & outCap)
                        flags |= 1;
                  if (capability & inCap)
                        flags |= 2;
                  dev->setrwFlags(flags);
                  if (MusEGlobal::debugMsg)
                        printf("ALSA port add: <%s>, %d:%d flags %d 0x%0x\n",
                           snd_seq_port_info_get_name(pinfo),
                           adr.client, adr.port,
                           flags, capability);
                  MusEGlobal::midiDevices.add(dev);
                  }
            }

      snd_seq_client_info_set_client(cinfo, -1);   // Reset
      while (snd_seq_query_next_client(alsaSeq, cinfo) >= 0) {
            const char* cname = snd_seq_client_info_get_name(cinfo);
            //printf( "ALSA client name: %s\n", cname);  
            
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
                  MidiAlsaDevice* dev = new MidiAlsaDevice(adr, QString(snd_seq_port_info_get_name(pinfo)));
                  int flags = 0;
                  if (capability & outCap)
                        flags |= 1;
                  if (capability & inCap)
                        flags |= 2;
                  dev->setrwFlags(flags);
                  if(is_thru)             // Don't auto-open Midi Through.
                    dev->setOpenFlags(0); 
                  if (MusEGlobal::debugMsg)
                        printf("ALSA port add: <%s>, %d:%d flags %d 0x%0x\n",
                           snd_seq_port_info_get_name(pinfo),
                           adr.client, adr.port,
                           flags, capability);
                  MusEGlobal::midiDevices.add(dev);
                  }
            }
            
            
      //snd_seq_set_client_name(alsaSeq, "MusE Sequencer");
      error = snd_seq_set_client_name(alsaSeq, MusEGlobal::audioDevice->clientName());
      if (error < 0) {
            printf("Alsa: Set client name failed: %s", snd_strerror(error));
            return true;
            }
      
      int ci = snd_seq_poll_descriptors_count(alsaSeq, POLLIN);
      int co = snd_seq_poll_descriptors_count(alsaSeq, POLLOUT);

      if (ci > 1 || co > 1) {
            printf("ALSA midi: cannot handle more than one poll fd\n");
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
            printf("Alsa: Subscribe System failed: %s", snd_strerror(error));
            return true;
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
        printf("MusE: exitMidiAlsa: Error unsubscribing alsa midi Announce port %d:%d for reading: %s\n", announce_adr.client, announce_adr.port, snd_strerror(error));
    }   
    
    error = snd_seq_delete_simple_port(alsaSeq, musePort.port);
    if(error < 0) 
      fprintf(stderr, "MusE: Could not delete ALSA simple port: %s\n", snd_strerror(error));
    
    error = snd_seq_close(alsaSeq);  
    if(error < 0) 
      fprintf(stderr, "MusE: Could not close ALSA sequencer: %s\n", snd_strerror(error));
  }  
}


//---------------------------------------------------------
//   setAlsaClientName
//---------------------------------------------------------

void setAlsaClientName(const char* name)
{
#ifdef ALSA_DEBUG
  printf("setAlsaClientName: %s  seq:%p\n", name, alsaSeq);
#endif            
  
  if(!alsaSeq)
    return; 
    
  int error = snd_seq_set_client_name(alsaSeq, name);
  if (error < 0) 
    printf("setAlsaClientName: failed: %s", snd_strerror(error));
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
      };

static std::list<AlsaPort> portList;

//---------------------------------------------------------
//   alsaScanMidiPorts
//---------------------------------------------------------

void alsaScanMidiPorts()
      {
#ifdef ALSA_DEBUG
    printf("alsa scan midi ports\n");
#endif
      const int inCap  = SND_SEQ_PORT_CAP_SUBS_READ;
      const int outCap = SND_SEQ_PORT_CAP_SUBS_WRITE;

      portList.clear();

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
// printf("ALSA port add: <%s>, flags %d\n", name, flags);
                  portList.push_back(AlsaPort(adr, name, flags));
                  }
            }
      //
      //  check for devices to delete
      //
      for (iMidiDevice i = MusEGlobal::midiDevices.begin(); i != MusEGlobal::midiDevices.end();) {
            MidiAlsaDevice* d = dynamic_cast<MidiAlsaDevice*>(*i);
            if (d == 0) {
                  ++i;
                  continue;
                  }
            std::list<AlsaPort>::iterator k = portList.begin();
            for (; k != portList.end(); ++k) {
                  if (k->adr.client == d->adr.client
                     && k->adr.port == d->adr.port) {
                        break;
                        }
                  }
            if (k == portList.end()) {
                  if (d->midiPort() != -1)
                        MusEGlobal::midiPorts[d->midiPort()].setMidiDevice(0);
                  iMidiDevice k = i;
// printf("erase device\n");
                  ++i;
                  MusEGlobal::midiDevices.erase(k);
                  }
            else {
                  ++i;
                  }
            }
      //
      //  check for devices to add
      //
      // TODO: Possibly auto-add them to available midi ports.    p4.0.41            
      //
      for (std::list<AlsaPort>::iterator k = portList.begin(); k != portList.end(); ++k) {
            iMidiDevice i = MusEGlobal::midiDevices.begin();
// printf("ALSA port: <%s>\n", k->name);
            for (;i != MusEGlobal::midiDevices.end(); ++i) {
                  MidiAlsaDevice* d = dynamic_cast<MidiAlsaDevice*>(*i);
                  if (d == 0)
                        continue;
                  if ((k->adr.client == d->adr.client) && (k->adr.port == d->adr.port)) {
                        break;
                        }
                  }
            if (i == MusEGlobal::midiDevices.end()) {
                  // add device
                  MidiAlsaDevice* dev = new MidiAlsaDevice(k->adr,
                     QString(k->name));
                  dev->setrwFlags(k->flags);
                  MusEGlobal::midiDevices.add(dev);
// printf("add device\n");
                  }
            }
            
            
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
      MidiRecordEvent event;
      snd_seq_event_t* ev;
      
      for (;;) 
      {
            int rv = snd_seq_event_input(alsaSeq, &ev);
// printf("AlsaInput %d\n", rv);
            if (rv < 0) {
//                  printf("AlsaMidi: read error %s\n", snd_strerror(rv));
                  return;
                  }
            switch(ev->type) {
                  case SND_SEQ_EVENT_PORT_SUBSCRIBED:
                  case SND_SEQ_EVENT_PORT_UNSUBSCRIBED:
                        return;
                  case SND_SEQ_EVENT_CLIENT_START:
                  case SND_SEQ_EVENT_CLIENT_EXIT:
                        // return;
                        // on first start of a software synthesizer we only
                        // get CLIENT_START event and no PORT_START, why?

                  case SND_SEQ_EVENT_PORT_START:
                  case SND_SEQ_EVENT_PORT_EXIT:
                        alsaScanMidiPorts();
                        MusEGlobal::audio->midiPortsChanged();  // signal gui
                        snd_seq_free_event(ev);
                        return;
                  }

            int curPort = -1;
            MidiAlsaDevice* mdev = 0;
            //
            // find real source device
            //
            for (iMidiDevice i = MusEGlobal::midiDevices.begin(); i != MusEGlobal::midiDevices.end(); ++i) {
                  MidiAlsaDevice* d = dynamic_cast<MidiAlsaDevice*>(*i);
                  if (d  && d->adr.client == ev->source.client
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
                  return;
                  }
            
            event.setType(0);      // mark as unused
            event.setPort(curPort);
            event.setB(0);

            switch(ev->type) 
            {
                  case SND_SEQ_EVENT_NOTEON:
                  case SND_SEQ_EVENT_KEYPRESS:
                        event.setChannel(ev->data.note.channel);
                        event.setType(ME_NOTEON);
                        event.setA(ev->data.note.note);
                        event.setB(ev->data.note.velocity);
                        break;

                  case SND_SEQ_EVENT_NOTEOFF:
                        event.setChannel(ev->data.note.channel);
                        event.setType(ME_NOTEOFF);
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
                          printf("MusE: alsaProcessMidiInput sysex chunks not supported!\n");
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
                  // case SND_SEQ_EVENT_NONREGPARAM:
                  // case SND_SEQ_EVENT_REGPARAM:
                  default:
                        printf("ALSA Midi input: type %d not handled\n", ev->type);
                        break;
            }
            if(event.type())
              mdev->recordEvent(event);
                  
            snd_seq_free_event(ev);
            if (rv == 0)
                  break;
      }
}

} // namespace MusECore
