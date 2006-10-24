//=============================================================================
//  MusE
//  Linux Music Editor
//  $Id:$
//
//  Copyright (C) 2002-2006 by Werner Schweer and others
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License version 2.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//=============================================================================

#include "alsamidi.h"
#include "globals.h"
#include "midi.h"
#include "midiinport.h"
#include "midioutport.h"
#include "../midiseq.h"
#include "../song.h"
#include "al/pos.h"

static const unsigned int inCap  = SND_SEQ_PORT_CAP_SUBS_READ;
static const unsigned int outCap = SND_SEQ_PORT_CAP_SUBS_WRITE;

AlsaMidi alsaMidi;
AlsaMidi* midiDriver;

//---------------------------------------------------------
//   AlsaMidi
//---------------------------------------------------------

AlsaMidi::AlsaMidi()
      {
      alsaSeq = 0;
      }

//---------------------------------------------------------
//   init
//    return true on error
//---------------------------------------------------------

bool AlsaMidi::init()
      {
      if (debugMsg)
            printf("init AlsaMidi\n");
      int error = snd_seq_open(&alsaSeq, "hw", SND_SEQ_OPEN_DUPLEX, SND_SEQ_NONBLOCK);
      if (error < 0) {
            if (error == ENOENT)
            fprintf(stderr, "open ALSA sequencer failed: %s\n",
               snd_strerror(error));
            return true;
            }

      snd_seq_set_client_name(alsaSeq, "MusE Sequencer");

      //-----------------------------------------
      //    subscribe to "Announce"
      //    this enables callbacks for any
      //    alsa port changes
      //-----------------------------------------

      snd_seq_addr_t src, dst;
      int rv = snd_seq_create_simple_port(alsaSeq, "MusE Port 0",
         inCap | outCap | SND_SEQ_PORT_CAP_READ
         | SND_SEQ_PORT_CAP_WRITE
         | SND_SEQ_PORT_CAP_NO_EXPORT,
         SND_SEQ_PORT_TYPE_APPLICATION);
      if (rv < 0) {
            fprintf(stderr, "Alsa: create MusE port failed: %s\n", snd_strerror(error));
            exit(1);
            }
      dst.port   = rv;
      dst.client = snd_seq_client_id(alsaSeq);
      src.port   = SND_SEQ_PORT_SYSTEM_ANNOUNCE;
      src.client = SND_SEQ_CLIENT_SYSTEM;

      snd_seq_port_subscribe_t* subs;
      snd_seq_port_subscribe_alloca(&subs);
      snd_seq_port_subscribe_set_dest(subs, &dst);
      snd_seq_port_subscribe_set_sender(subs, &src);
      error = snd_seq_subscribe_port(alsaSeq, subs);
      if (error < 0) {
            fprintf(stderr, "Alsa: Subscribe System failed: %s\n", snd_strerror(error));
            return true;
            }

      return false;
      }

//---------------------------------------------------------
//   outputPorts
//---------------------------------------------------------

QList<PortName> AlsaMidi::outputPorts(bool)
      {
      QList<PortName> clientList;
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
                  if (((capability & outCap) == outCap)
                     && !(capability & SND_SEQ_PORT_CAP_NO_EXPORT)) {
                        int client = snd_seq_port_info_get_client(pinfo);
                        if (client != snd_seq_client_id(alsaSeq)) {
                              PortName pn;
                              pn.name = QString(snd_seq_port_info_get_name(pinfo));
                              pn.port = Port(client, snd_seq_port_info_get_port(pinfo));
                              clientList.append(pn);
                              }
                        }
                  }
            }
      return clientList;
      }

//---------------------------------------------------------
//   inputPorts
//---------------------------------------------------------

QList<PortName> AlsaMidi::inputPorts(bool)
      {
      QList<PortName> clientList;

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
                  if (((capability & inCap) == inCap)
                     && !(capability & SND_SEQ_PORT_CAP_NO_EXPORT)) {
                        int client = snd_seq_port_info_get_client(pinfo);
                        if (client != snd_seq_client_id(alsaSeq)) {
                              PortName pn;
                              pn.name = QString(snd_seq_port_info_get_name(pinfo));
                              pn.port = Port(client, snd_seq_port_info_get_port(pinfo));
                              clientList.append(pn);
                              }
                        }
                  }
            }
      return clientList;
      }

//---------------------------------------------------------
//   registerOutPort
//---------------------------------------------------------

Port AlsaMidi::registerOutPort(const QString& name, bool)
      {
      int alsaPort  = snd_seq_create_simple_port(alsaSeq, name.toLatin1().data(),
         outCap | SND_SEQ_PORT_CAP_WRITE, SND_SEQ_PORT_TYPE_APPLICATION);
      if (alsaPort < 0) {
            perror("cannot create alsa out port");
            return Port();
            }
      return Port(snd_seq_client_id(alsaSeq), alsaPort);
      }

//---------------------------------------------------------
//   registerInPort
//---------------------------------------------------------

Port AlsaMidi::registerInPort(const QString& name, bool)
      {
      int alsaPort  = snd_seq_create_simple_port(alsaSeq, name.toLatin1().data(),
         inCap | SND_SEQ_PORT_CAP_READ, SND_SEQ_PORT_TYPE_APPLICATION);
      if (alsaPort < 0) {
            perror("cannot create alsa in port");
            return Port();
            }
      return Port(snd_seq_client_id(alsaSeq), alsaPort);
      }

//---------------------------------------------------------
//   unregisterPort
//---------------------------------------------------------

void AlsaMidi::unregisterPort(Port port)
      {
      snd_seq_delete_simple_port(alsaSeq, port.alsaPort());
      }

//---------------------------------------------------------
//   setPortName
//---------------------------------------------------------

void AlsaMidi::setPortName(Port, const QString& name)
      {
      printf("AlsaMidi::setPortName(%s): not impl.\n", name.toLatin1().data());
      }

//---------------------------------------------------------
//   portName
//---------------------------------------------------------

QString AlsaMidi::portName(Port p)
      {
      snd_seq_port_info_t* pinfo;
      snd_seq_port_info_alloca(&pinfo);
      snd_seq_get_any_port_info(alsaSeq, p.alsaClient(), p.alsaPort(), pinfo);
      return QString(snd_seq_port_info_get_name(pinfo));
      }

//---------------------------------------------------------
//   findPort
//---------------------------------------------------------

Port AlsaMidi::findPort(const QString& name)
      {
      snd_seq_client_info_t* cinfo;
      snd_seq_client_info_alloca(&cinfo);
      snd_seq_client_info_set_client(cinfo, 0);

      while (snd_seq_query_next_client(alsaSeq, cinfo) >= 0) {
            snd_seq_port_info_t *pinfo;
            snd_seq_port_info_alloca(&pinfo);
            snd_seq_port_info_set_client(pinfo, snd_seq_client_info_get_client(cinfo));
            snd_seq_port_info_set_port(pinfo, -1);
            while (snd_seq_query_next_port(alsaSeq, pinfo) >= 0) {
                  const char* pn = snd_seq_port_info_get_name(pinfo);
                  if (name == pn) {
                        return Port(snd_seq_port_info_get_client(pinfo),
                           snd_seq_port_info_get_port(pinfo));
                        }
                  }
            }
      printf("AlsaMidi: port <%s> not found\n", name.toLatin1().data());
      return Port();
      }

//---------------------------------------------------------
//   connect
//    return false if connect fails
//---------------------------------------------------------

bool AlsaMidi::connect(Port src, Port dst)
      {
      snd_seq_port_subscribe_t* sub;
      snd_seq_port_subscribe_alloca(&sub);

      snd_seq_addr_t s, d;
      s.port   = src.alsaPort();
      s.client = src.alsaClient();
      d.port   = dst.alsaPort();
      d.client = dst.alsaClient();
      snd_seq_port_subscribe_set_sender(sub, &s);
      snd_seq_port_subscribe_set_dest(sub, &d);

      int rv = snd_seq_subscribe_port(alsaSeq, sub);
      if (rv < 0) {
            printf("AlsaMidi::connect(%d:%d, %d:%d) failed: %s\n",
               src.alsaClient(), src.alsaPort(),
               dst.alsaClient(), dst.alsaPort(),
               snd_strerror(rv));
            return false;
            }
      return true;
      }

//---------------------------------------------------------
//   disconnect
//    return false if disconnect fails
//---------------------------------------------------------

bool AlsaMidi::disconnect(Port src, Port dst)
      {
      snd_seq_port_subscribe_t* sub;
      snd_seq_port_subscribe_alloca(&sub);
      snd_seq_addr_t s, d;
      s.port   = src.alsaPort();
      s.client = src.alsaClient();
      d.port   = dst.alsaPort();
      d.client = dst.alsaClient();
      snd_seq_port_subscribe_set_sender(sub, &s);
      snd_seq_port_subscribe_set_dest(sub, &d);
      int rv = snd_seq_unsubscribe_port(alsaSeq, sub);
      if (rv < 0)
            printf("AlsaMidi::disconnect() failed: %s\n",
               snd_strerror(rv));
      return rv >= 0;
      }

//---------------------------------------------------------
//   getInputPollFd
//---------------------------------------------------------

void AlsaMidi::getInputPollFd(struct pollfd** p, int* n)
      {
      int npfdi = snd_seq_poll_descriptors_count(alsaSeq, POLLIN);
      struct pollfd* pfdi  = new struct pollfd[npfdi];
      snd_seq_poll_descriptors(alsaSeq, pfdi, npfdi, POLLIN);
      *p = pfdi;
      *n = npfdi;
      }

//---------------------------------------------------------
//   getOutputPollFd
//---------------------------------------------------------

void AlsaMidi::getOutputPollFd(struct pollfd** p, int* n)
      {
      int npfdo = snd_seq_poll_descriptors_count(alsaSeq, POLLOUT);
      struct pollfd* pfdo  = new struct pollfd[npfdo];
      snd_seq_poll_descriptors(alsaSeq, pfdo, npfdo, POLLOUT);
      *p = pfdo;
      *n = npfdo;
      }

//---------------------------------------------------------
//   addConnection
//    a new connection was added
//---------------------------------------------------------

void AlsaMidi::addConnection(snd_seq_connect_t* ev)
      {
      Port rs(ev->sender.client, ev->sender.port);
      Port rd(ev->dest.client, ev->dest.port);

      MidiOutPortList* opl = song->midiOutPorts();
      for (iMidiOutPort i = opl->begin(); i != opl->end(); ++i) {
            MidiOutPort* oport = *i;
            Port src = oport->alsaPort(0);

            if (src == rs) {
                  Route r(rd, Route::MIDIPORT);
                  if (oport->outRoutes()->indexOf(r) == -1) {
                        Port port(ev->dest.client, ev->dest.port);
                        oport->outRoutes()->push_back(Route(port, -1, Route::MIDIPORT));
                        }
                  break;
                  }
            }

      MidiInPortList* ipl = song->midiInPorts();
      for (iMidiInPort i = ipl->begin(); i != ipl->end(); ++i) {
            MidiInPort* iport = *i;
            Port dst = iport->alsaPort();

            if (dst == rd) {
                  Route r(rs, Route::MIDIPORT);
                  if (iport->inRoutes()->indexOf(r) == -1) {
                        Port port(ev->sender.client, ev->sender.port);
                        iport->inRoutes()->push_back(Route(port, -1, Route::MIDIPORT));
                        }
                  break;
                  }
            }
      }

//---------------------------------------------------------
//   removeConnection
//    a connection was removed
//---------------------------------------------------------

void AlsaMidi::removeConnection(snd_seq_connect_t* ev)
      {
      Port rs(ev->sender.client, ev->sender.port);
      Port rd(ev->dest.client, ev->dest.port);

      MidiInPortList* ipl = song->midiInPorts();
      for (iMidiInPort i = ipl->begin(); i != ipl->end(); ++i) {
            MidiInPort* iport = *i;
            Port dst = iport->alsaPort();

            if (dst == rd) {
                  RouteList* irl = iport->outRoutes();
                  for (iRoute r = irl->begin(); r != irl->end(); ++r) {
                        if (!r->disconnected && (r->port == rs)) {
                              iport->inRoutes()->erase(r);
                              break;
                              }
                        }
                  break;
                  }
            }

      MidiOutPortList* opl = song->midiOutPorts();
      for (iMidiOutPort i = opl->begin(); i != opl->end(); ++i) {
            MidiOutPort* oport = *i;
            Port src = oport->alsaPort();

            if (src == rs) {
                  RouteList* orl = oport->outRoutes();
                  for (iRoute r = orl->begin(); r != orl->end(); ++r) {
                        if (!r->disconnected && (r->port == rd)) {
                              orl->erase(r);
                              break;
                              }
                        }
                  break;
                  }
            }
      }

//---------------------------------------------------------
//   read
//    read ALSA midi events
//    This is called by the high priority RT MidiSeq
//    thread.
//---------------------------------------------------------

void AlsaMidi::read(MidiSeq* seq)
      {
      snd_seq_event_t* ev;
      for (int i = 0;; ++i) {
            int rv = snd_seq_event_input(alsaSeq, &ev);
            if (rv <= 0) {
                  if (rv < 0 && rv != -11)   // Resource temporarily unavailable
                        printf("AlsaMidi: read error 0x%x %s\n", rv, snd_strerror(rv));
                  break;
                  }
// printf("Alsa midi event %d %d\n", rv, ev->type);

            switch(ev->type) {
                  case SND_SEQ_EVENT_PORT_SUBSCRIBED:
                        // printf("subscribe\n");
                        addConnection((snd_seq_connect_t*)(&ev->data));
                        break;
                  case SND_SEQ_EVENT_PORT_UNSUBSCRIBED:
                        // printf("unsubscribe\n");
                        removeConnection((snd_seq_connect_t*)(&ev->data));
                        break;
                  case SND_SEQ_EVENT_CLIENT_START:
                        // printf("client start\n");
                        break;
                  case SND_SEQ_EVENT_CLIENT_EXIT:
                        // printf("client exit\n");
                        break;
                  case SND_SEQ_EVENT_PORT_START:
                        // printf("port start\n");
                        break;
                  case SND_SEQ_EVENT_PORT_EXIT:
                        // printf("port exit\n");
                        break;
                  case SND_SEQ_EVENT_SYSEX:
                  	{
                        //
                        // look for Midi Machine Control events (MMC)
                        //
                        unsigned char* data = ((unsigned char*)ev->data.ext.ptr) + 1;
                        int len = ev->data.ext.len - 2;
                        if ((len == 4) && (data[0] == 0x7f) && (data[2] == 0x06)) {
                              seq->mmcInput(data[1], data[3], 0);
                              break;
                              }
                        if ((len == 11) && (data[0] == 0x7f)
                           && (data[2] == 0x06)
                           && (data[3] == 0x44) && (data[4] == 0x06)
                           && (data[5] == 0x1)) {
                              int h = data[6];
                              int m = data[7];
                              int s = data[8];
                              int f = data[9];
                              int sf = data[10];
                              AL::Pos pos(h * 60 + m, s, f, sf);
                              seq->mmcInput(data[1], data[3], pos);
                              break;
                              }
                        }
                  	
                  case SND_SEQ_EVENT_KEYPRESS:
                  case SND_SEQ_EVENT_CHANPRESS:
                  case SND_SEQ_EVENT_NOTEON:
                  case SND_SEQ_EVENT_NOTEOFF:
                  case SND_SEQ_EVENT_PGMCHANGE:
                  case SND_SEQ_EVENT_PITCHBEND:
                  case SND_SEQ_EVENT_CONTROLLER:
                        {
                        Port port(ev->dest.client, ev->dest.port);

                        MidiInPortList* mpl = song->midiInPorts();
                        for (iMidiInPort i = mpl->begin(); i != mpl->end(); ++i) {
                              MidiInPort* inPort = *i;
                              if (port == inPort->alsaPort()) {
                                    inPort->eventReceived(ev);
                                    }
                              }
                        }
                        break;

                  case SND_SEQ_EVENT_CLOCK:
                        seq->realtimeSystemInput(0, 0xf8);
                        break;
                  case SND_SEQ_EVENT_START:
                        seq->realtimeSystemInput(0, 0xfa);
                        break;
                  case SND_SEQ_EVENT_CONTINUE:
                        seq->realtimeSystemInput(0, 0xfb);
                        break;
                  case SND_SEQ_EVENT_STOP:
                        seq->realtimeSystemInput(0, 0xfc);
                        break;
                  case SND_SEQ_EVENT_TICK:
                        seq->realtimeSystemInput(0, 0xf9);
                        break;
                  case SND_SEQ_EVENT_SONGPOS:
                        seq->setSongPosition(0, ev->data.control.value);
                        break;
                  case SND_SEQ_EVENT_SENSING:
                        break;
                  case SND_SEQ_EVENT_QFRAME:
                        seq->mtcInputQuarter(0, ev->data.control.value);
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
            snd_seq_free_event(ev);
            }
      }

//---------------------------------------------------------
//   write
//---------------------------------------------------------

void AlsaMidi::write()
      {
      }

//---------------------------------------------------------
//   putEvent
//---------------------------------------------------------

void AlsaMidi::putEvent(Port p, const MidiEvent& e)
      {
      if (midiOutputTrace) {
            printf("MidiOut<%s>: midiAlsa: ", portName(p).toLatin1().data());
            e.dump();
            }
      int chn = e.channel();
      int a   = e.dataA();
      int b   = e.dataB();

      snd_seq_event_t event;
      memset(&event, 0, sizeof(event));
      snd_seq_ev_set_direct(&event);
      snd_seq_ev_set_source(&event, p.alsaPort());
      snd_seq_ev_set_dest(&event, SND_SEQ_ADDRESS_SUBSCRIBERS, 0);

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
                  snd_seq_ev_set_controller(&event, chn, a, b);
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
                  putEvent(&event);
                  return;
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
                  return;
            }
      putEvent(&event);
      }

//---------------------------------------------------------
//   putEvent
//    return false if event is delivered
//---------------------------------------------------------

bool AlsaMidi::putEvent(snd_seq_event_t* event)
      {
      int error;

      do {
            error   = snd_seq_event_output_direct(alsaSeq, event);
            int len = snd_seq_event_length(event);
            if (error == len) {
                  return false;
                  }
            if (error < 0) {
                  if (error == -12) {
                        return true;
                        }
                  else {
                        fprintf(stderr, "MidiAlsaDevice::%p putEvent(): midi write error: %s\n",
                           this, snd_strerror(error));
                        //exit(-1);
                        }
                  }
            else
                  fprintf(stderr, "MidiAlsaDevice::putEvent(): midi write returns %d, expected %d: %s\n",
                     error, len, snd_strerror(error));
            } while (error == -12);
      return true;
      }


