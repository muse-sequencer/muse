//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: alsamidi.cpp,v 1.8.2.7 2009/11/19 04:20:33 terminator356 Exp $
//  (C) Copyright 2000-2001 Werner Schweer (ws@seh.de)
//=========================================================

#include <stdio.h>

#include "alsamidi.h"
#include "globals.h"
#include "midi.h"
#include "mididev.h"
#include "../midiport.h"
#include "../midiseq.h"
#include "../midictrl.h"
#include "../audio.h"
#include "mpevent.h"
//#include "sync.h"
#include "utils.h"

static int alsaSeqFdi = -1;
static int alsaSeqFdo = -1;

snd_seq_t* alsaSeq;
static snd_seq_addr_t musePort;

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
      snd_seq_port_subscribe_t* subs;
      // Allocated on stack, no need to call snd_seq_port_subscribe_free() later.
      snd_seq_port_subscribe_alloca(&subs);

      QString estr;
      int wer = 0;
      int rer = 0;
      
      // subscribe for writing
      if (_openFlags & 1) 
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
            if(!wer)
              _writeEnable = true;      
      }

      // subscribe for reading
      if (_openFlags & 2) 
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
            if(!rer)
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
      snd_seq_port_subscribe_t* subs;
      // Allocated on stack, no need to call snd_seq_port_subscribe_free() later.
      snd_seq_port_subscribe_alloca(&subs);
      
      // Changed by T356. This function appears to be called only by MidiPort::setMidiDevice(), 
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
      //                no matter if jack midi is turned on or not.

      //if (_openFlags & 1) {
      //if (!(_openFlags & 1)) 
      {
            snd_seq_port_subscribe_set_sender(subs, &musePort);
            snd_seq_port_subscribe_set_dest(subs, &adr);
            
            // Already subscribed? Then unsubscribe.
            if(!snd_seq_get_port_subscription(alsaSeq, subs))
            {
              if(!snd_seq_unsubscribe_port(alsaSeq, subs))
                _writeEnable = false;      
              else
                printf("MidiAlsaDevice::close Error unsubscribing alsa midi port for writing\n");
            }   
            else
              _writeEnable = false;      
      }

      //if (_openFlags & 2) {
      //if (!(_openFlags & 2)) 
      {
            snd_seq_port_subscribe_set_dest(subs, &musePort);
            snd_seq_port_subscribe_set_sender(subs, &adr);
            
            // Already subscribed? Then unsubscribe.
            if(!snd_seq_get_port_subscription(alsaSeq, subs))
            {
              if(!snd_seq_unsubscribe_port(alsaSeq, subs))
                _readEnable = false;      
              else  
                printf("MidiAlsaDevice::close Error unsubscribing alsa midi port for reading\n");
            }  
            else
              _readEnable = false;      
      }
}

//---------------------------------------------------------
//   putEvent
//---------------------------------------------------------

bool MidiAlsaDevice::putMidiEvent(const MidiPlayEvent& e)
      {
      if (midiOutputTrace) {
            printf("MidiOut: midiAlsa: ");
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
#if 1
                  snd_seq_ev_set_controller(&event, chn, a, b);
#else
                  {
                  int a   = e.dataA();
                  int b   = e.dataB();
                  int chn = e.channel();
                  // p3.3.37
                  //if (a < 0x1000) {          // 7 Bit Controller
                  if (a < CTRL_14_OFFSET) {          // 7 Bit Controller
                        snd_seq_ev_set_controller(&event, chn, a, b);
                        }
                  //else if (a < 0x20000) {     // 14 bit high resolution controller
                  else if (a < CTRL_RPN_OFFSET) {     // 14 bit high resolution controller
                        int ctrlH = (a >> 8) & 0x7f;
                        int ctrlL = a & 0x7f;
                        a = (ctrlH << 7) + ctrlL;
                        snd_seq_ev_set_controller(&event, chn, a, b);
                        event.type = SND_SEQ_EVENT_CONTROL14;
                        }
                  //else if (a < 0x30000) {     // RPN 7-Bit Controller
                  else if (a < CTRL_NRPN_OFFSET) {     // RPN 7-Bit Controller
                        int ctrlH = (a >> 8) & 0x7f;
                        int ctrlL = a & 0x7f;
                        a = (ctrlH << 7) + ctrlL;
                        b <<= 7;
                        snd_seq_ev_set_controller(&event, chn, a, b);
                        event.type = SND_SEQ_EVENT_REGPARAM;
                        }
                  //else if (a < 0x40000) {     // NRPN 7-Bit Controller
                  else if (a < CTRL_INTERNAL_OFFSET) {     // NRPN 7-Bit Controller
                        int ctrlH = (a >> 8) & 0x7f;
                        int ctrlL = a & 0x7f;
                        a = (ctrlH << 7) + ctrlL;
                        b <<= 7;
                        snd_seq_ev_set_controller(&event, chn, a, b);
                        event.type = SND_SEQ_EVENT_NONREGPARAM;
                        }
                  //else if (a < 0x60000) {     // RPN14 Controller
                  else if (a < CTRL_NRPN14_OFFSET) {     // RPN14 Controller
                        int ctrlH = (a >> 8) & 0x7f;
                        int ctrlL = a & 0x7f;
                        a = (ctrlH << 7) + ctrlL;
                        snd_seq_ev_set_controller(&event, chn, a, b);
                        event.type = SND_SEQ_EVENT_REGPARAM;
                        }
                  //else if (a < 0x70000) {     // NRPN14 Controller
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
//   initMidiAlsa
//    return true on error
//---------------------------------------------------------

bool initMidiAlsa()
      {
      if (debugMsg)
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
            snd_seq_port_info_t *pinfo;
            snd_seq_port_info_alloca(&pinfo);
            snd_seq_port_info_set_client(pinfo, snd_seq_client_info_get_client(cinfo));
            snd_seq_port_info_set_port(pinfo, -1);

            while (snd_seq_query_next_port(alsaSeq, pinfo) >= 0) {
                  unsigned int capability = snd_seq_port_info_get_capability(pinfo);
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
                  if (debugMsg)
                        printf("ALSA port add: <%s>, %d:%d flags %d 0x%0x\n",
                           snd_seq_port_info_get_name(pinfo),
                           adr.client, adr.port,
                           flags, capability);
                  midiDevices.add(dev);
                  
                  /*
                  // Experimental... Need to list 'sensible' devices first and ignore unwanted ones...
                  // Add instance last in midi device list.
                  for(int i = 0; i < MIDI_PORTS; ++i) 
                  {
                    MidiPort* mp  = &midiPorts[i];
                    if(mp->device() == 0) 
                    {
                      // midiSeq might not be initialzed yet!
                      //midiSeq->msgSetMidiDevice(mp, dev);
                      mp->setMidiDevice(dev);
                      
                      //muse->changeConfig(true);     // save configuration file
                      //update();
                      break;
                    }
                  }
                  */
                  
                  }
            }
      snd_seq_set_client_name(alsaSeq, "MusE Sequencer");
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

      snd_seq_addr_t aadr;
      aadr.client = SND_SEQ_CLIENT_SYSTEM;
      aadr.port   = SND_SEQ_PORT_SYSTEM_ANNOUNCE;

      snd_seq_port_subscribe_t* subs;
      snd_seq_port_subscribe_alloca(&subs);
      snd_seq_port_subscribe_set_dest(subs, &musePort);
      snd_seq_port_subscribe_set_sender(subs, &aadr);
      error = snd_seq_subscribe_port(alsaSeq, subs);
      if (error < 0) {
            printf("Alsa: Subscribe System failed: %s", snd_strerror(error));
            return true;
            }
      return false;
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
//    printf("alsa scan midi ports\n");
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
      for (iMidiDevice i = midiDevices.begin(); i != midiDevices.end();) {
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
                        midiPorts[d->midiPort()].setMidiDevice(0);
                  iMidiDevice k = i;
// printf("erase device\n");
                  ++i;
                  midiDevices.erase(k);
                  }
            else {
                  ++i;
                  }
            }
      //
      //  check for devices to add
      //
      for (std::list<AlsaPort>::iterator k = portList.begin(); k != portList.end(); ++k) {
            iMidiDevice i = midiDevices.begin();
// printf("ALSA port: <%s>\n", k->name);
            for (;i != midiDevices.end(); ++i) {
                  MidiAlsaDevice* d = dynamic_cast<MidiAlsaDevice*>(*i);
                  if (d == 0)
                        continue;
                  if ((k->adr.client == d->adr.client) && (k->adr.port == d->adr.port)) {
                        break;
                        }
                  }
            if (i == midiDevices.end()) {
                  // add device
                  MidiAlsaDevice* dev = new MidiAlsaDevice(k->adr,
                     QString(k->name));
                  dev->setrwFlags(k->flags);
                  midiDevices.add(dev);
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
                        audio->midiPortsChanged();  // signal gui
                        snd_seq_free_event(ev);
                        return;
                  }

            int curPort = -1;
            MidiAlsaDevice* mdev = 0;
            //
            // find real source device
            //
            for (iMidiDevice i = midiDevices.begin(); i != midiDevices.end(); ++i) {
                  MidiAlsaDevice* d = dynamic_cast<MidiAlsaDevice*>(*i);
                  if (d  && d->adr.client == ev->source.client
                     && d->adr.port == ev->source.port) {
                        curPort = d->midiPort();
                        mdev = d;
                        }
                  }
            
            if (mdev == 0 || curPort == -1) {
                  if (debugMsg) {
                        fprintf(stderr, "no port %d:%d found for received alsa event\n",
                           ev->source.client, ev->source.port);
                        }
                  snd_seq_free_event(ev);
                  return;
                  }
            
            /*
            if(curPort == -1) 
            {
                if(mdev == 0)
                {  
                  if (debugMsg) 
                  {
                    fprintf(stderr, "no port %d:%d found for received alsa event\n",
                      ev->source.client, ev->source.port);
                  }
                }
                else
                {
                  // Allow the sync detect mechanisms to work, even if device is not assigned to a port.
                  if(ev->type == SND_SEQ_EVENT_CLOCK)
                    mdev->syncInfo().trigMCSyncDetect();
                  else  
                  if(ev->type == SND_SEQ_EVENT_TICK)
                    mdev->syncInfo().trigTickDetect();
                }
                snd_seq_free_event(ev);
                return;
            }
            */      
                  
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
                        midiSeq->realtimeSystemInput(curPort, 0xf8);
                        //mdev->syncInfo().trigMCSyncDetect();
                        break;

                  case SND_SEQ_EVENT_START:
                        midiSeq->realtimeSystemInput(curPort, 0xfa);
                        break;

                  case SND_SEQ_EVENT_CONTINUE:
                        midiSeq->realtimeSystemInput(curPort, 0xfb);
                        break;

                  case SND_SEQ_EVENT_STOP:
                        midiSeq->realtimeSystemInput(curPort, 0xfc);
                        break;

                  case SND_SEQ_EVENT_TICK:
                        midiSeq->realtimeSystemInput(curPort, 0xf9);
                        //mdev->syncInfo().trigTickDetect();
                        break;

                  case SND_SEQ_EVENT_SYSEX:
                        event.setTime(0);      // mark as used
                        event.setType(ME_SYSEX);
                        event.setData((unsigned char*)(ev->data.ext.ptr)+1,
                           ev->data.ext.len-2);
                        break;
                  case SND_SEQ_EVENT_PORT_SUBSCRIBED:
                  case SND_SEQ_EVENT_PORT_UNSUBSCRIBED:  // write port is released
                        break;
                  case SND_SEQ_EVENT_SONGPOS:
                        midiSeq->setSongPosition(curPort, ev->data.control.value);
                        break;
                  case SND_SEQ_EVENT_SENSING:
                        break;
                  case SND_SEQ_EVENT_QFRAME:
                        midiSeq->mtcInputQuarter(curPort, ev->data.control.value);
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
            {
              mdev->recordEvent(event);
              // p3.3.26 1/23/10 Moved to MidiDevice now. Anticipating Jack midi support, so don't make it ALSA specific. Tim.
              //if(ev->type != SND_SEQ_EVENT_SYSEX)
                // Trigger general activity indicator detector. Sysex has no channel, don't trigger.
              //  midiPorts[curPort].syncInfo().trigActDetect(event.channel());
            }
                  
            snd_seq_free_event(ev);
            if (rv == 0)
                  break;
      }
}

