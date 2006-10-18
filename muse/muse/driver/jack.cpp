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

#include "al/al.h"
#include "al/tempo.h"
#include "audio.h"
#include "globals.h"
#include "song.h"
#include "jackaudio.h"
#include "track.h"

#ifdef VST_SUPPORT
#include <fst.h>
#endif

JackAudio* jackAudio;

//---------------------------------------------------------
//   init
//---------------------------------------------------------

bool JackAudio::init()
      {
      return true;
      }

//---------------------------------------------------------
//   jack_thread_init
//---------------------------------------------------------

static void jack_thread_init (void* /*data*/)
      {
#ifdef VST_SUPPORT
      if (loadVST)
            fst_adopt_thread();
#endif
      }

//---------------------------------------------------------
//   timebase_callback
//---------------------------------------------------------

static void timebase_callback(jack_transport_state_t /* state */,
   jack_nframes_t /* nframes */,
   jack_position_t* pos,
   int /* new_pos */,
   void*)
	{
      AL::Pos p(pos->frame, AL::FRAMES);
      pos->valid = JackPositionBBT;
	p.mbt(&pos->bar, &pos->beat, &pos->tick);
      pos->bar++;
      pos->beat++;
      pos->bar_start_tick = Pos(pos->bar, 0, 0).tick();
      //
      //  dummy:
      //
      pos->beats_per_bar = 4;
      pos->beat_type = 4;
      pos->ticks_per_beat = 384;
      int tempo = AL::tempomap.tempo(p.tick());
      pos->beats_per_minute = (60000000.0 / tempo) * AL::tempomap.globalTempo()/100.0;
      }

//---------------------------------------------------------
//   processAudio
//    JACK callback
//---------------------------------------------------------

static int processAudio(jack_nframes_t frames, void*)
      {
      segmentSize = frames;
      if (audioState == AUDIO_RUNNING)
            audio->process((unsigned long)frames);
      else if (audioState == AUDIO_STOP) {
            if (debugMsg)
                 puts("jack calling when audio is stopped!\n");
            }
      else if (audioState == AUDIO_START1)
            audioState = AUDIO_START2;
      return 0;
      }

//---------------------------------------------------------
//   processSync
//    return TRUE (non-zero) when ready to roll.
//---------------------------------------------------------

static int processSync(jack_transport_state_t state, jack_position_t* pos, void*)
      {
      int audioState = Audio::STOP;
      switch (state) {
            case JackTransportStopped:
                  audioState = Audio::STOP;
                  break;
            case JackTransportLooping:
            case JackTransportRolling:
                  audioState = Audio::PLAY;
                  break;
            case JackTransportStarting:
                  audioState = Audio::START_PLAY;
                  break;
            }
      unsigned frame = pos->frame;
      return audio->sync(audioState, frame);
      }

//---------------------------------------------------------
//   processShutdown
//---------------------------------------------------------

static void processShutdown(void*)
      {
      audio->shutdown();

      for (int i = 0; i < 10; ++i) {
      	if (audioState == AUDIO_STOP)
            	break;
          	sleep(1);
      	}
	if (audioState == AUDIO_RUNNING)
		fprintf(stderr, "MusE: sequencer still running, something is very wrong.\n");
      jackAudio->zeroClientPtr(); // jack disconnect client no longer valid
      }

//---------------------------------------------------------
//   jackError
//---------------------------------------------------------

static void jackError(const char* s)
      {
      fprintf(stderr, "JACK ERROR: %s\n", s);
      }

//---------------------------------------------------------
//   noJackError
//---------------------------------------------------------

static void noJackError(const char* /* s */)
      {
      }

//---------------------------------------------------------
//   JackAudio
//---------------------------------------------------------

JackAudio::JackAudio(jack_client_t* cl, char * name)
   : AudioDriver()
      {
      strcpy(jackRegisteredName, name);
      _client = cl;
      }

//---------------------------------------------------------
//   ~JackAudio
//---------------------------------------------------------

JackAudio::~JackAudio()
      {
      if (_client) {
            if (jack_client_close(_client)) {
                  fprintf(stderr, "jack_client_close() failed: %s\n",
                    strerror(errno));
                  }
            }
            _client = 0;
      }
//---------------------------------------------------------
//   getJackName()
//---------------------------------------------------------

char* JackAudio::getJackName()
      {
      return jackRegisteredName;
      }

//---------------------------------------------------------
//   initJackAudio
//    return true if JACK not found
//---------------------------------------------------------

bool initJackAudio()
      {
      if (debugMsg) {
            fprintf(stderr, "init Jack Audio\n");
            jack_set_error_function(jackError);
            }
      else
            jack_set_error_function(noJackError);

      jack_client_t* client = 0;
      int i = 0;
      char jackIdString[8];
      for (i = 0; i < 5; ++i) {
            sprintf(jackIdString, "MusE-%d", i+1);
            client = jack_client_new(jackIdString);
            if (client)
                  break;
            }

      if (i == 5)
            return true;

      if (debugMsg)
            fprintf(stderr, "init Jack Audio: register device\n");

      jack_set_error_function(jackError);
      if (debugMsg)
            fprintf(stderr, "init Jack Audio: register device\n");

      jackAudio = new JackAudio(client, jackIdString);
      if (debugMsg)
            fprintf(stderr, "init Jack Audio: register client\n");
      jackAudio->registerClient();
      AL::sampleRate  = jack_get_sample_rate(client);
      segmentSize     = jack_get_buffer_size(client);
      audioDriver     = jackAudio;
      return false;
      }

//---------------------------------------------------------
//   restart
//---------------------------------------------------------

void JackAudio::restart()
      {
      _client = jack_client_new(jackRegisteredName);
      registerClient();
      }

//---------------------------------------------------------
//   bufsize_callback
//---------------------------------------------------------

static int bufsize_callback(jack_nframes_t n, void*)
      {
      printf("JACK: buffersize changed %d\n", n);
      return 0;
      }

//---------------------------------------------------------
//   freewheel_callback
//---------------------------------------------------------

static void freewheel_callback(int starting, void*)
      {
      audio->setFreewheel(starting);
      }

//---------------------------------------------------------
//   srate_callback
//---------------------------------------------------------

static int srate_callback(jack_nframes_t n, void*)
      {
      if (debugMsg)
            printf("JACK: sample rate changed: %d\n", n);
      return 0;
      }

//---------------------------------------------------------
//   registration_callback
//---------------------------------------------------------

static void registration_callback(jack_port_id_t, int, void*)
      {
      if (debugMsg)
            printf("JACK: registration changed\n");
      }

//---------------------------------------------------------
//   graph_callback
//    this is called from jack when the connections
//    changed
//---------------------------------------------------------

static int graph_callback(void*)
      {
      // we cannot call JackAudio::graphChanged() from this
      // context, so we send a message to the gui thread which in turn
      // calls graphChanged()

      audio->sendMsgToGui(MSG_GRAPH_CHANGED);
      if (debugMsg)
            printf("JACK: graph changed!\n");
      return 0;
      }

//---------------------------------------------------------
//   JackAudio::graphChanged
//    this is called from song in gui context triggered
//    by graph_callback()
//---------------------------------------------------------

struct RouteRoute {
      Route src;
      Route dst;
      };

void JackAudio::graphChanged()
      {
      QList<RouteRoute> rr;
      QList<RouteRoute> ra;

      InputList* il = song->inputs();
      for (iAudioInput ii = il->begin(); ii != il->end(); ++ii) {
            AudioInput* it = *ii;
            int channels = it->channels();
            for (int channel = 0; channel < channels; ++channel) {
                  jack_port_t* port = (jack_port_t*)(it->jackPort(channel));
                  if (port == 0)
                        continue;
                  const char** ports = jack_port_get_all_connections(_client, port);
                  RouteList* rl      = it->inRoutes();
                  //---------------------------------------
                  // check for disconnects
                  //---------------------------------------

                  for (iRoute irl = rl->begin(); irl != rl->end(); ++irl) {
                        if (irl->channel != channel)
                              continue;
                        Route r = *irl;
                        const char* name = jack_port_name((jack_port_t*)r.port);
                        bool found = false;
                        const char** pn = ports;
                        while (pn && *pn) {
                              if (strcmp(*pn, name) == 0) {
                                    found = true;
                                    break;
                                    }
                              ++pn;
                              }
                        if (!found) {
                              RouteRoute a;
                              a.src = Route(irl->port, channel, Route::AUDIOPORT);
                              a.dst = Route(it, channel);
                              rr.append(a);
                              }
                        }

                  //---------------------------------------
                  // check for connects
                  //---------------------------------------

                  if (ports) {
                        for (const char** pn = ports; *pn; ++pn) {
                              bool found = false;
                              for (iRoute irl = rl->begin(); irl != rl->end(); ++irl) {
                                    if (irl->channel != channel)
                                          continue;
                                    const char* name = jack_port_name((jack_port_t*)irl->port);
                                    if (strcmp(*pn, name) == 0) {
                                          found = true;
                                          break;
                                          }
                                    }
                              if (!found) {
                                    RouteRoute a;
                                    Port port = jack_port_by_name(_client, *pn);
                                    a.src = Route(port, channel, Route::AUDIOPORT);
                                    a.dst = Route(it, channel);
                                    ra.append(a);
                                    }
                              }

                        free(ports);
                        }
                  }
            }

      foreach(RouteRoute a, rr) {
            audio->msgRemoveRoute1(a.src, a.dst);
            }
      foreach(RouteRoute a, ra) {
            audio->msgAddRoute1(a.src, a.dst);
            }
      rr.clear();
      ra.clear();      

      OutputList* ol = song->outputs();
      for (iAudioOutput ii = ol->begin(); ii != ol->end(); ++ii) {
            AudioOutput* it = *ii;
            int channels = it->channels();
            for (int channel = 0; channel < channels; ++channel) {
                  jack_port_t* port = (jack_port_t*)(it->jackPort(channel));
                  if (port == 0)
                        continue;
                  const char** ports = jack_port_get_all_connections(_client, port);
                  RouteList* rl      = it->outRoutes();

                  //---------------------------------------
                  // check for disconnects
                  //---------------------------------------

                  for (iRoute irl = rl->begin(); irl != rl->end(); ++irl) {
                        if (irl->channel != channel)
                              continue;
                        const char* name = jack_port_name((jack_port_t*)irl->port);
                        bool found = false;
                        const char** pn = ports;
                        while (pn && *pn) {
                              if (strcmp(*pn, name) == 0) {
                                    found = true;
                                    break;
                                    }
                              ++pn;
                              }
                        if (!found) {
                              RouteRoute a;
                              a.src = Route(it, channel);
                              a.dst = Route(irl->port, channel, Route::AUDIOPORT);
                              rr.append(a);
                              }
                        }

                  //---------------------------------------
                  // check for connects
                  //---------------------------------------

                  if (ports) {
                        const char** pn = ports;
                        while (*pn) {
                              bool found = false;
                              Port port;
                              for (iRoute irl = rl->begin(); irl != rl->end(); ++irl) {
                                    if (irl->channel != channel)
                                          continue;
                                    port = irl->port;
                                    const char* name = jack_port_name((jack_port_t*)port);
                                    if (strcmp(*pn, name) == 0) {
                                          found = true;
                                          break;
                                          }
                                    }
                              if (!found) {
                                    RouteRoute a;
                                    a.src = Route(it, channel);
                                    a.dst = Route(port, channel, Route::AUDIOPORT);
                                    ra.append(a);
                                    }
                              ++pn;
                              }
                        free(ports);
                        }
                  }
            }
      foreach(RouteRoute a, rr)
            audio->msgRemoveRoute1(a.src, a.dst);
      foreach(RouteRoute a, ra)
            audio->msgAddRoute1(a.src, a.dst);
      }

//static int xrun_callback(void*)
//      {
//      printf("JACK: xrun\n");
//      return 0;
//      }

//---------------------------------------------------------
//   register
//---------------------------------------------------------

void JackAudio::registerClient()
      {
      jack_set_process_callback(_client, processAudio, 0);
      jack_set_sync_callback(_client, processSync, 0);
      jack_on_shutdown(_client, processShutdown, 0);
      jack_set_buffer_size_callback(_client, bufsize_callback, 0);
      jack_set_sample_rate_callback(_client, srate_callback, 0);
      jack_set_port_registration_callback(_client, registration_callback, 0);
      jack_set_graph_order_callback(_client, graph_callback, 0);
//      jack_set_xrun_callback(_client, xrun_callback, 0);
      jack_set_freewheel_callback (_client, freewheel_callback, 0);
	jack_set_thread_init_callback(_client, (JackThreadInitCallback) jack_thread_init, 0);
	jack_set_timebase_callback(_client, 0, timebase_callback, 0);
      }

//---------------------------------------------------------
//   registerInPort
//---------------------------------------------------------
                                    
Port JackAudio::registerInPort(const QString& name, bool midi)
      {
      const char* type = midi ? JACK_DEFAULT_MIDI_TYPE : JACK_DEFAULT_AUDIO_TYPE;
      void* p = jack_port_register(_client, name.toLatin1().data(), type, JackPortIsInput, 0);
// printf("JACK: registerInPort<%s>: <%s> %p\n", type, name.toLatin1().data(), p);
      return p;
      }

//---------------------------------------------------------
//   registerOutPort
//---------------------------------------------------------

Port JackAudio::registerOutPort(const QString& name, bool midi)
      {
      const char* type = midi ? JACK_DEFAULT_MIDI_TYPE : JACK_DEFAULT_AUDIO_TYPE;
      void* p = jack_port_register(_client, name.toLatin1().data(), type, JackPortIsOutput, 0);
// printf("JACK: registerOutPort<%s>: <%s> %p\n", type, name.toLatin1().data(), p);
      return p;
      }

//---------------------------------------------------------
//   exitJackAudio
//---------------------------------------------------------

void exitJackAudio()
      {
      if (jackAudio)
            delete jackAudio;
      }

//---------------------------------------------------------
//   connect
//    return false on error
//---------------------------------------------------------

bool JackAudio::connect(Port src, Port dst)
      {
      if (src == 0 || dst == 0) {
            fprintf(stderr, "JackAudio::connect(1): unknown jack ports\n");
            return false;
            }
      const char* sn = jack_port_name((jack_port_t*) src);
      const char* dn = jack_port_name((jack_port_t*) dst);

// printf("jack connect <%s>%p - <%s>%p\n", sn, src, dn, dst);

      if (sn == 0 || dn == 0) {
            fprintf(stderr, "JackAudio::connect(2): unknown jack ports\n");
            return false;
            }
      int rv = jack_connect(_client, sn, dn);
      if (rv) {
            fprintf(stderr, "%d: jack connect <%s> - <%s> failed\n",
               rv, sn, dn);
            if (rv == EEXIST)
                  fprintf(stderr, "  connection already made\n");
            else {
                  int pf = jack_port_flags((jack_port_t*)src);
                  if (!(pf & JackPortIsOutput))
                        fprintf(stderr, "  src is not an output port\n");
                  pf = jack_port_flags((jack_port_t*)dst);
                  if (!(pf & JackPortIsInput))
                        fprintf(stderr, "  dst is not an input port\n");
                  }
            return false;
            }
      return true;
      }

//---------------------------------------------------------
//   disconnect
//---------------------------------------------------------

bool JackAudio::disconnect(Port src, Port dst)
      {
      const char* sn = jack_port_name((jack_port_t*) src);
      const char* dn = jack_port_name((jack_port_t*) dst);

// printf("jack disconnect <%s>%p - <%s>%p\n", sn, src, dn, dst);

      if (sn == 0 || dn == 0) {
            fprintf(stderr, "JackAudio::disconnect: unknown jack ports\n");
            return false;
            }
      if (jack_disconnect(_client, sn, dn)) {
            fprintf(stderr, "jack disconnect <%s> - <%s> failed\n",
               sn, dn);
            return false;
            }
      return true;
      }

//---------------------------------------------------------
//   start
//---------------------------------------------------------

void JackAudio::start(int)
      {
      if (jack_activate(_client)) {
            fprintf (stderr, "JACK: cannot activate client\n");
            exit(-1);
            }
      }

//---------------------------------------------------------
//   stop
//---------------------------------------------------------

void JackAudio::stop()
      {
      if (_client == 0)
            return;
      if (jack_deactivate(_client))
            fprintf (stderr, "JACK: cannot deactivate client\n");
      }

//---------------------------------------------------------
//   framePos
//---------------------------------------------------------

unsigned JackAudio::framePos() const
      {
      return _client ? jack_frame_time(_client) : 0;
      }

//---------------------------------------------------------
//   outputPorts
//---------------------------------------------------------

QList<PortName> JackAudio::outputPorts(bool midi)
      {
      const char* type = midi ? JACK_DEFAULT_MIDI_TYPE : JACK_DEFAULT_AUDIO_TYPE;
      const char** ports = jack_get_ports(_client, 0, type, JackPortIsOutput);
      QList<PortName> clientList;
      for (const char** p = ports; p && *p; ++p) {
            jack_port_t* port = jack_port_by_name(_client, *p);
            char buffer[128];
            strncpy(buffer, *p, 128);
            if (strncmp(buffer, "MusE", 4) == 0)
                  continue;
            PortName pn;
            pn.name = QString(buffer);
            pn.port = port;
            clientList.append(pn);
            }
      return clientList;
      }

//---------------------------------------------------------
//   inputPorts
//---------------------------------------------------------

QList<PortName> JackAudio::inputPorts(bool midi)
      {
      const char* type = midi ? JACK_DEFAULT_MIDI_TYPE : JACK_DEFAULT_AUDIO_TYPE;
      const char** ports = jack_get_ports(_client, 0, type, JackPortIsInput);
      QList<PortName> clientList;
      for (const char** p = ports; p && *p; ++p) {
            jack_port_t* port = jack_port_by_name(_client, *p);
            char buffer[128];
            strncpy(buffer, *p, 128);
            if (strncmp(buffer, "MusE", 4) == 0)
                  continue;
            PortName pn;
            pn.name = QString(buffer);
            pn.port = port;
            clientList.append(pn);
            }
      return clientList;
      }

//---------------------------------------------------------
//   portName
//---------------------------------------------------------

QString JackAudio::portName(void* port)
      {
      const char *nameStrPtr = jack_port_name((jack_port_t*)port);
      QString s(nameStrPtr);
      return s;
      }

//---------------------------------------------------------
//   unregisterPort
//---------------------------------------------------------

void JackAudio::unregisterPort(Port p)
      {
 //printf("JACK: unregister Port %p\n", p);
      jack_port_unregister(_client, (jack_port_t*)p);
      }

//---------------------------------------------------------
//   getState
//---------------------------------------------------------

int JackAudio::getState()
      {
      transportState = jack_transport_query(_client, &pos);
      switch (transportState) {
            case JackTransportStopped:  return Audio::STOP;
            case JackTransportLooping:
            case JackTransportRolling:  return Audio::PLAY;
            case JackTransportStarting:  return Audio::START_PLAY;
            default:
                  return Audio::STOP;
            }
      }

//---------------------------------------------------------
//   setFreewheel
//---------------------------------------------------------

void JackAudio::setFreewheel(bool f)
      {
//      printf("JACK: setFreewheel %d\n", f);
      jack_set_freewheel(_client, f);
      }

//---------------------------------------------------------
//   startTransport
//---------------------------------------------------------

void JackAudio::startTransport()
      {
//      printf("JACK: startTransport\n");
      jack_transport_start(_client);
      }

//---------------------------------------------------------
//   stopTransport
//---------------------------------------------------------

void JackAudio::stopTransport()
      {
//      printf("JACK: stopTransport\n");
      if (_client)
            jack_transport_stop(_client);
      }

//---------------------------------------------------------
//   seekTransport
//---------------------------------------------------------

void JackAudio::seekTransport(unsigned frame)
      {
//      printf("JACK: seekTransport %d\n", frame);
      jack_transport_locate(_client, frame);
      }

//---------------------------------------------------------
//   findPort
//---------------------------------------------------------

Port JackAudio::findPort(const QString& name)
      {
      void* p = jack_port_by_name(_client, name.toLatin1().data());
// printf("Jack::findPort <%s>, %p\n", name.toLatin1().data(), p);
      return p;
      }

//---------------------------------------------------------
//   realtimePriority
//	return zero if not running realtime
//	can only be called if JACK client thread is already
//	running
//---------------------------------------------------------

int JackAudio::realtimePriority() const
      {
      pthread_t t = jack_client_thread_id(_client);
      int policy;
      struct sched_param param;
      memset(&param, 0, sizeof(param));
	int rv = pthread_getschedparam(t, &policy, &param);
      if (rv) {
            perror("MusE: get jack schedule parameter");
            return 0;
            }
      if (policy != SCHED_FIFO) {
            printf("JACK is not running realtime\n");
            return 0;
            }
      return param.sched_priority;
      }

