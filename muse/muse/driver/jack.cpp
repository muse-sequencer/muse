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
      int policy;
      if ( (policy = sched_getscheduler (0)) < 0) {
            printf("cannot get current client scheduler for JACK thread: %s!\n", strerror(errno));
            }
      else {
            if (policy != SCHED_FIFO)
                  printf("JACK thread %d _NOT_ running SCHED_FIFO\n", getpid());
            else if (debugMsg) {
            	struct sched_param rt_param;
            	memset(&rt_param, 0, sizeof(sched_param));
            	int type;
            	int rv = pthread_getschedparam(pthread_self(), &type, &rt_param);
            	if (rv == -1)
                  	perror("get scheduler parameter");
                  printf("JACK thread running SCHED_FIFO priority %d\n",
                     rt_param.sched_priority);
                  }
            }

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
#if 0
      unsigned t1 = jackAudio->frameTime();
      unsigned t2 = jackAudio->lastFrameTime();
      unsigned t3 = jackAudio->framesSinceCyleStart();
      unsigned t4 = jackAudio->getCurFrame();

printf("cycle %8d %8d %8d %08d\n", t1, t2, t3, t4);
#endif

      int jackState = jackAudio->getTransportState();
      segmentSize = frames;
      if (audioState == AUDIO_RUNNING)
            audio->process((unsigned long)frames, jackState);
      else if (audioState == AUDIO_STOP) {
            if (debugMsg)
                 puts("jack calling when audio is stopped!\n");
            }
      else if (audioState == AUDIO_START1)
            audioState = AUDIO_START2;
      return 0;
      }

//---------------------------------------------------------
//    getTransportState
//---------------------------------------------------------

int JackAudio::getTransportState()
      {
      int jackState;
      transportState = jack_transport_query(_client, &pos);
      switch (jackAudio->transportState) {
            case JackTransportStopped:
                  jackState = Audio::STOP;
                  break;
            case JackTransportLooping:
            case JackTransportRolling:
                  jackState = Audio::PLAY;
                  break;
            case JackTransportStarting:
                  jackState = Audio::START_PLAY;
                  break;
            default:
                  jackState = Audio::STOP;
                  break;
            }
      return jackState;
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

JackAudio::JackAudio(jack_client_t* cl, char* name)
   : AudioDriver()
      {
printf("JackAudio::JackAudio(%p,%s)\n", cl, name);
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
printf("JackAudio::~JackAudio\n");
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
//   restart
//---------------------------------------------------------

bool JackAudio::restart()
      {
printf("JackAudio::restart\n");
      _client = jack_client_new(jackRegisteredName);
      if (!_client)
            return true;
      registerClient();
      return false;
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

void JackAudio::graphChanged()
      {
      RouteList rr, ra;

      InputList* il = song->inputs();
      for (iAudioInput ii = il->begin(); ii != il->end(); ++ii) {
            AudioInput* it = *ii;
            int channels   = it->channels();
            RouteList* irl = it->inRoutes();

            for (int channel = 0; channel < channels; ++channel) {
                  jack_port_t* port = it->jackPort(channel).jackPort();
                  if (port == 0)
                        continue;
                  const char** ports = jack_port_get_all_connections(_client, port);

                  //---------------------------------------
                  // check for disconnects
                  //---------------------------------------

                  foreach (const Route& r, *irl) {
                        if (r.dst.channel != channel)
                              continue;
                        const char* name = jack_port_name(r.src.port.jackPort());
                        bool found      = false;
                        for (const char** pn = ports; pn && *pn; ++pn) {
                              if (strcmp(*pn, name) == 0) {
                                    found = true;
                                    break;
                                    }
                              }
                        if (!found)
                              rr.append(r);
                        }

                  //---------------------------------------
                  // check for connects
                  //---------------------------------------

                  if (ports) {
                        for (const char** pn = ports; *pn; ++pn) {
                              bool found = false;
                              foreach(const Route& r, *irl) {
                                    if (r.dst.channel != channel)
                                          continue;
                                    const char* name = jack_port_name(r.src.port.jackPort());
                                    if (strcmp(*pn, name) == 0) {
                                          found = true;
                                          break;
                                          }
                                    }
                              if (!found) {
                                    Route a;
                                    Port port(jack_port_by_name(_client, *pn));
                                    a.src = RouteNode(port, -1, RouteNode::AUDIOPORT);
                                    a.dst = RouteNode(it, channel);
                                    ra.append(a);
                                    }
                              }
                        free(ports);
                        }
                  }
            }

// printf("  input: remove %d add %d routes\n", rr.size(), ra.size());
      foreach(Route r, rr)
            audio->msgRemoveRoute1(r);
      foreach(Route r, ra)
            audio->msgAddRoute1(r);
      rr.clear();
      ra.clear();

      OutputList* ol = song->outputs();
      for (iAudioOutput ii = ol->begin(); ii != ol->end(); ++ii) {
            AudioOutput* it = *ii;
            int channels = it->channels();
            for (int channel = 0; channel < channels; ++channel) {
                  jack_port_t* port = it->jackPort(channel).jackPort();
                  if (port == 0)
                        continue;
                  const char** ports = jack_port_get_all_connections(_client, port);
                  RouteList* rl      = it->outRoutes();

                  //---------------------------------------
                  // check for disconnects
                  //---------------------------------------

                  foreach(const Route& r, *rl) {
                        if (r.src.channel != channel)
                              continue;
                        const char* name = jack_port_name(r.dst.port.jackPort());
                        bool found = false;
                        const char** pn = ports;
                        while (pn && *pn) {
                              if (strcmp(*pn, name) == 0) {
                                    found = true;
                                    break;
                                    }
                              ++pn;
                              }
                        if (!found)
                              rr.append(r);
                        }

                  //---------------------------------------
                  // check for connects
                  //---------------------------------------

                  if (ports) {
                        const char** pn = ports;
                        while (*pn) {
                              bool found = false;
                              foreach (const Route& r, *rl) {
                                    if (r.src.channel != channel)
                                          continue;
                                    const char* name = jack_port_name(r.dst.port.jackPort());
                                    if (strcmp(*pn, name) == 0) {
                                          found = true;
                                          break;
                                          }
                                    }
                              if (!found) {
                                    Route a;
                                    Port port(jack_port_by_name(_client, *pn));
                                    a.src = RouteNode(it, channel, RouteNode::TRACK);
                                    a.dst = RouteNode(port, -1, RouteNode::AUDIOPORT);
                                    ra.append(a);
                                    }
                              ++pn;
                              }
                        free(ports);
                        }
                  }
            }
// printf("  output: remove %d add %d routes\n", rr.size(), ra.size());
      foreach(Route r, rr)
            audio->msgRemoveRoute1(r);
      foreach(Route r, ra)
            audio->msgAddRoute1(r);
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
      Port p(jack_port_register(_client, name.toLatin1().data(), type, JackPortIsInput, 0));
// printf("JACK: registerInPort<%s>: <%s> %p\n", type, name.toLatin1().data(), p.jackPort());
      if (!p.jackPort())
            p.setZero();
      return p;
      }

//---------------------------------------------------------
//   registerOutPort
//---------------------------------------------------------

Port JackAudio::registerOutPort(const QString& name, bool midi)
      {
      const char* type = midi ? JACK_DEFAULT_MIDI_TYPE : JACK_DEFAULT_AUDIO_TYPE;
      Port p(jack_port_register(_client, name.toLatin1().data(), type, JackPortIsOutput, 0));
// printf("JACK: registerOutPort<%s>: <%s> %p\n", type, name.toLatin1().data(), p.jackPort());
#if 0
      if (midi) {
            jack_nframes_t nframes = jack_get_buffer_size(_client);
            jack_midi_reset_new_port(jack_port_get_buffer(p.jackPort(), nframes), nframes);
            }
#endif
      if (!p.jackPort())
            p.setZero();
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
      if (src.isZero() || dst.isZero()) {
            fprintf(stderr, "JackAudio::connect(1): unknown jack ports (%d-%d)\n",
               src.isZero(), dst.isZero());
            return false;
            }
      const char* sn = jack_port_name(src.jackPort());
      const char* dn = jack_port_name(dst.jackPort());

      if (debugMsg)
            printf("jack connect <%s>%p - <%s>%p\n", sn, src.jackPort(), dn, dst.jackPort());

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
                  int pf = jack_port_flags(src.jackPort());
                  if (!(pf & JackPortIsOutput))
                        fprintf(stderr, "  src is not an output port\n");
                  pf = jack_port_flags(dst.jackPort());
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
      const char* sn = jack_port_name(src.jackPort());
      const char* dn = jack_port_name(dst.jackPort());

      if (debugMsg)
            printf("jack disconnect <%s>%p - <%s>%p\n", sn, src.jackPort(), dn, dst.jackPort());

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
            pn.port = Port(port);
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
            pn.port = Port(port);
            clientList.append(pn);
            }
      return clientList;
      }

//---------------------------------------------------------
//   portName
//---------------------------------------------------------

QString JackAudio::portName(Port port)
      {
      const char* nameStrPtr = jack_port_name(port.jackPort());
      QString s(nameStrPtr);
      return s;
      }

//---------------------------------------------------------
//   unregisterPort
//---------------------------------------------------------

void JackAudio::unregisterPort(Port p)
      {
      if (_client) {
// printf("JACK: unregister Port %p\n", p);
            if (jack_port_unregister(_client, p.jackPort()))
                  fprintf(stderr, "jack unregister port %p failed\n", p.jackPort());
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
      if (_client == 0) {
            printf("JackAudio(%p)::findPort(%s): _client==0\n", this, qPrintable(name));
            return Port();
            }
      jack_port_t* port = jack_port_by_name(_client, name.toLatin1().data());
// printf("Jack::findPort <%s>, %p\n", name.toLatin1().data(), port);
      return (port == 0) ? Port() : Port(port);
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
      jack_status_t status;
      jack_options_t options = JackNullOption;
      client = jack_client_open("MusE", options, &status);
      if (!client) {
            if (status & JackServerStarted)
                  printf("jack server started...\n");
            if (status & JackServerFailed)
                  printf("cannot connect to jack server\n");
            if (status & JackServerError)
                  printf("communication with jack server failed\n");
            if (status & JackShmFailure)
                  printf("jack cannot access shared memory\n");
            if (status & JackVersionError)
                  printf("jack server has wrong version\n");
            printf("cannot create jack client\n");
            return true;
            }

      if (debugMsg)
            fprintf(stderr, "init Jack Audio: register device\n");

      jack_set_error_function(jackError);
      if (debugMsg)
            fprintf(stderr, "init Jack Audio: register device\n");

printf("create jack, %p\n", client);
      jackAudio = new JackAudio(client, jack_get_client_name(client));
printf("create jack, client=%p jack=%p\n", client, jackAudio);
      if (debugMsg)
            fprintf(stderr, "init Jack Audio: register client\n");
      jackAudio->registerClient();
      AL::sampleRate = jack_get_sample_rate(client);
      segmentSize    = jack_get_buffer_size(client);
      audioDriver    = jackAudio;
      return false;
      }

//---------------------------------------------------------
//   putEvent
//---------------------------------------------------------

void JackAudio::putEvent(Port port, const MidiEvent& e)
      {
      if (midiOutputTrace) {
            printf("MidiOut<%s>: jackMidi: ", portName(port).toLatin1().data());
            e.dump();
            }
      void* pb = jack_port_get_buffer(port.jackPort(), segmentSize);
      int ft = e.time() - lastFrameTime();
      if (ft < 0 || ft >= (int)segmentSize) {
            printf("JackAudio::putEvent: time out of range %d\n", ft);
            if (ft < 0)
                  ft = 0;
            if (ft > (int)segmentSize)
                  ft = segmentSize - 1;
            }
      switch(e.type()) {
            case ME_NOTEON:
            case ME_NOTEOFF:
            case ME_POLYAFTER:
            case ME_CONTROLLER:
            case ME_PITCHBEND:
                  {
                  unsigned char* p = jack_midi_event_reserve(pb, ft, 3);
                  if (p == 0) {
                        fprintf(stderr, "JackMidi: buffer overflow, event lost\n");
                        return;
                        }
                  p[0] = e.type() | e.channel();
                  p[1] = e.dataA();
                  p[2] = e.dataB();
                  }
                  break;

            case ME_PROGRAM:
            case ME_AFTERTOUCH:
                  {
                  unsigned char* p = jack_midi_event_reserve(pb, ft, 2);
                  if (p == 0) {
                        fprintf(stderr, "JackMidi: buffer overflow, event lost\n");
                        return;
                        }
                  p[0] = e.type() | e.channel();
                  p[1] = e.dataA();
                  }
                  break;
            case ME_SYSEX:
                  {
                  const unsigned char* data = e.data();
                  int len = e.len();
                  unsigned char* p = jack_midi_event_reserve(pb, ft, len+2);
                  if (p == 0) {
                        fprintf(stderr, "JackMidi: buffer overflow, event lost\n");
                        return;
                        }
                  p[0] = 0xf0;
                  p[len+1] = 0xf7;
                  memcpy(p+1, data, len);
                  }
                  break;
            case ME_SONGPOS:
            case ME_CLOCK:
            case ME_START:
            case ME_CONTINUE:
            case ME_STOP:
                  printf("JackMidi: event type %x not supported\n", e.type());
                  break;
            }
      }

//---------------------------------------------------------
//    startMidiCycle
//---------------------------------------------------------

void JackAudio::startMidiCycle(Port port)
      {
      void* port_buf = jack_port_get_buffer(port.jackPort(), segmentSize);
      jack_midi_clear_buffer(port_buf);
      }

