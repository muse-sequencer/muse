//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: jack.cpp,v 1.30.2.17 2009/12/20 05:00:35 terminator356 Exp $
//  (C) Copyright 2002 Werner Schweer (ws@seh.de)
//=========================================================

#include "config.h"
#include <stdio.h>
#include <errno.h>
#include <stdarg.h>
//#include <time.h> 
#include <unistd.h>

#include "audio.h"
#include "globals.h"
#include "song.h"
#include "jackaudio.h"
#include "track.h"
#include "pos.h"
#include "tempo.h"
#include "sync.h"
#include "utils.h"

#define JACK_DEBUG 0

//#include "errorhandler.h"

#ifndef RTCAP
extern void doSetuid();
extern void undoSetuid();
#endif

#ifdef VST_SUPPORT
#include <fst.h>
#endif

JackAudioDevice* jackAudio;

//---------------------------------------------------------
//   checkJackClient - make sure client is valid
//---------------------------------------------------------
inline bool checkJackClient(jack_client_t* _client)
      {
      if (_client == NULL) {
            printf("Panic! no _client!\n");
            return false;
            }
      return true;
      }
//---------------------------------------------------------
//   checkAudioDevice - make sure audioDevice exists
//---------------------------------------------------------
bool checkAudioDevice()
      {
      if (audioDevice == NULL) {
            printf("Muse:checkAudioDevice: no audioDevice\n");
            return false;
            }
      return true;
      }


//---------------------------------------------------------
//   jack_thread_init
//---------------------------------------------------------

static void jack_thread_init (void* )  // data
      {
      doSetuid();
      /*
      if (jackAudio->isRealtime()) {
            struct sched_param rt_param;
            int rv;
            memset(&rt_param, 0, sizeof(sched_param));
            int type;
            rv = pthread_getschedparam(pthread_self(), &type, &rt_param);
            if (rv != 0)
                  perror("get scheduler parameter");
            if (type != SCHED_FIFO) {
                  fprintf(stderr, "JACK thread not running SCHED_FIFO, try to set...\n");

                  memset(&rt_param, 0, sizeof(sched_param));
                  rt_param.sched_priority = 1;
                  rv = pthread_setschedparam(pthread_self(), SCHED_FIFO, &rt_param);
                  if (rv != 0)
                        perror("set realtime scheduler");
                  memset(&rt_param, 0, sizeof(sched_param));
                  rv = pthread_getschedparam(pthread_self(), &type, &rt_param);
                  if (rv != 0)
                        perror("get scheduler parameter");
                  if (type != SCHED_FIFO)
                        fprintf(stderr, "JACK still not running FIFO !?!\n"
                        "======reliable RT operation not possible!!======\n");
                  else
                        fprintf(stderr, "JACK thread succesfully set to SCHED_FIFO\n");
                  } 
            }
            */
#ifdef VST_SUPPORT
      if (loadVST)
            fst_adopt_thread();
#endif
      undoSetuid();
      }

//---------------------------------------------------------
//   processAudio
//    JACK callback
//---------------------------------------------------------

static int processAudio(jack_nframes_t frames, void*)
      {
//      if (JACK_DEBUG)
//            printf("processAudio - >>>>\n");
      segmentSize = frames;
      if (audio->isRunning())
            audio->process((unsigned long)frames);
      else {
            if (debugMsg)
                 puts("jack calling when audio is disconnected!\n");
            }
//      if (JACK_DEBUG)
//            printf("processAudio - <<<<\n");
      return 0;
      }

//---------------------------------------------------------
//   processSync
//    return TRUE (non-zero) when ready to roll.
//---------------------------------------------------------

static int processSync(jack_transport_state_t state, jack_position_t* pos, void*)
      {
      if (JACK_DEBUG)
            printf("processSync()\n");
      
      if(!useJackTransport)
        return 1;
        
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
              // Added by Tim. p3.3.6
              //printf("processSync JackTransportStarting\n");
              
              audioState = Audio::START_PLAY;
            break;  
            //case JackTransportNetStarting:  
            // FIXME: Quick and dirty hack to support both Jack-1 and Jack-2
            // Really need a config check of version...
            case 4:  
              // Added by Tim. p3.3.6
              //printf("processSync JackTransportNetStarting\n");
              
              audioState = Audio::START_PLAY;
            break;  
            }
            
      unsigned frame = pos->frame;
      // Added by Tim. p3.3.6
      //printf("processSync valid:%d frame:%d\n", pos->valid, frame);
      
            // p3.3.23
            //printf("Jack processSync() before audio->sync frame:%d\n", frame);
      //return audio->sync(audioState, frame);
      int rv = audio->sync(audioState, frame);
            // p3.3.23
            //printf("Jack processSync() after audio->sync frame:%d\n", frame);
      return rv;      
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
      // P3.3.27
      //Pos p(pos->frame, false);
      Pos p(extSyncFlag.value() ? audio->tickPos() : pos->frame, extSyncFlag.value() ? true : false);
      
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
      int tempo = tempomap.tempo(p.tick());
      pos->beats_per_minute = (60000000.0 / tempo) * tempomap.globalTempo()/100.0;
      }

//---------------------------------------------------------
//   processShutdown
//---------------------------------------------------------

static void processShutdown(void*)
      {
      if (JACK_DEBUG)
          printf("processShutdown()\n");
      //printf("processShutdown\n");
      jackAudio->nullify_client();
      audio->shutdown();

      int c=0;
      while(midiSeqRunning == true) {
          if(c++ >10) {
              fprintf(stderr, "sequencer still running, something is very wrong.\n");
              break;
              }
          sleep(1);
          }
      delete jackAudio;
      jackAudio=0;
      audioDevice=0;
      }

//---------------------------------------------------------
//   jackError
//---------------------------------------------------------

static void jackError(const char *s)
      {
      //error->logError( "JACK ERROR: %s\n", s);
      fprintf(stderr,"JACK ERROR: %s\n", s);
      }

//---------------------------------------------------------
//   noJackError
//---------------------------------------------------------

static void noJackError(const char* /* s */)
      {
      }

//---------------------------------------------------------
//   JackAudioDevice
//---------------------------------------------------------

JackAudioDevice::JackAudioDevice(jack_client_t* cl, char * name)
   : AudioDevice()
      {
      //JackAudioDevice::jackStarted=false;
      strcpy(jackRegisteredName, name);
      _client = cl;
      dummyState = Audio::STOP;
      dummyPos = 0;
      }

//---------------------------------------------------------
//   ~JackAudioDevice
//---------------------------------------------------------

JackAudioDevice::~JackAudioDevice()
      {
      if (JACK_DEBUG)
            printf("~JackAudioDevice()\n");
      if (_client) {
            if (jack_client_close(_client)) {
                  //error->logError("jack_client_close() failed: %s\n", strerror(errno));
                  fprintf(stderr,"jack_client_close() failed: %s\n", strerror(errno));
                  }
            }
      }

//---------------------------------------------------------
//   realtimePriority
//      return zero if not running realtime
//      can only be called if JACK client thread is already
//      running
//---------------------------------------------------------

int JackAudioDevice::realtimePriority() const
      {
      pthread_t t = jack_client_thread_id(_client);
      int policy;
      struct sched_param param;
      memset(&param, 0, sizeof(param));
        int rv = pthread_getschedparam(t, &policy, &param);
      if (rv) {
            perror("MusE: JackAudioDevice::realtimePriority: Error: Get jack schedule parameter");
            return 0;
            }
      if (policy != SCHED_FIFO) {
            printf("MusE: JackAudioDevice::realtimePriority: JACK is not running realtime\n");
            return 0;
            }
      return param.sched_priority;
      }

//---------------------------------------------------------
//   getJackName()
//---------------------------------------------------------

char* JackAudioDevice::getJackName()
      {
      return jackRegisteredName;
      }

//---------------------------------------------------------
//   initJackAudio
//    return true if JACK not found
//---------------------------------------------------------

bool initJackAudio()
      {
      if (JACK_DEBUG)
            printf("initJackAudio()\n");
      if (debugMsg) {
            fprintf(stderr,"initJackAudio()\n");
            jack_set_error_function(jackError);
            }
      else
            jack_set_error_function(noJackError);
      doSetuid();

      jack_client_t* client = 0;
      int i = 0;
      char jackIdString[8];
      for (i = 0; i < 5; ++i) {
            sprintf(jackIdString, "MusE-%d", i+1);
            //client = jack_client_new(jackIdString);
            client = jack_client_open(jackIdString, JackNoStartServer, 0);
            if (client)
                  break;
            }

      if (i == 5)
            return true;

      if (debugMsg)
            fprintf(stderr, "initJackAudio(): client %s opened.\n", jackIdString);
      if (client) {
            jack_set_error_function(jackError);
            jackAudio = new JackAudioDevice(client, jackIdString);
            if (debugMsg)
                  fprintf(stderr, "initJackAudio(): registering client...\n");
            jackAudio->registerClient();
            sampleRate  = jack_get_sample_rate(client);
            segmentSize = jack_get_buffer_size(client);
            jack_set_thread_init_callback(client, (JackThreadInitCallback) jack_thread_init, 0);
            //jack_set_timebase_callback(client, 0, (JackTimebaseCallback) timebase_callback, 0);
            }
      undoSetuid();
      
      if (client) {
            audioDevice = jackAudio;
            return false;
            }
      return true;
      }

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
      if (debugMsg || JACK_DEBUG)
            printf("JACK: freewheel_callback: starting%d\n", starting);
      audio->setFreewheel(starting);
      }

static int srate_callback(jack_nframes_t n, void*)
      {
      if (debugMsg || JACK_DEBUG)
            printf("JACK: sample rate changed: %d\n", n);
      return 0;
      }

//---------------------------------------------------------
//   registration_callback
//---------------------------------------------------------

static void registration_callback(jack_port_id_t, int, void*)
      {
      if (debugMsg || JACK_DEBUG)
            printf("JACK: registration changed\n");
      }

//---------------------------------------------------------
//   graph_callback
//    this is called from jack when the connections
//    changed
//---------------------------------------------------------

static int graph_callback(void*)
      {
      if (JACK_DEBUG)
            printf("graph_callback()\n");
      // we cannot call JackAudioDevice::graphChanged() from this
      // context, so we send a message to the gui thread which in turn
      // calls graphChanged()
      audio->sendMsgToGui('C');
      if (debugMsg)
            printf("JACK: graph changed\n");
      return 0;
      }

//---------------------------------------------------------
//   JackAudioDevice::graphChanged
//    this is called from song in gui context triggered
//    by graph_callback()
//---------------------------------------------------------

void JackAudioDevice::graphChanged()
      {
      if (JACK_DEBUG)
            printf("graphChanged()\n");
      if(!checkJackClient(_client)) return;
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

                  bool erased;
                  // limit set to 20 iterations for disconnects, don't know how to make it go
                  // the "right" amount
                  for (int i = 0;i < 20;i++) {
                        erased = false;
                        for (iRoute irl = rl->begin(); irl != rl->end(); ++irl) {
                              if (irl->channel != channel)
                                    continue;
                              QString name = irl->name();
                              const char* portName = name.latin1();
                              //printf("portname=%s\n", portName);
                              bool found = false;
                              const char** pn = ports;
                              while (pn && *pn) {
                                    if (strcmp(*pn, portName) == 0) {
                                          found = true;
                                          break;
                                          }
                                    ++pn;
                                    }
                              if (!found) {
                                    audio->msgRemoveRoute1(
                                       Route(portName, false, channel),
                                       Route(it, channel)
                                       );
                                    erased = true;
                                    break;
                                    }
                              }
                        if (!erased)
                              break;
                        }

                  //---------------------------------------
                  // check for connects
                  //---------------------------------------

                  if (ports) {
                        const char** pn = ports;
                        while (*pn) {
                              bool found = false;
                              for (iRoute irl = rl->begin(); irl != rl->end(); ++irl) {
                                    if (irl->channel != channel)
                                          continue;
                                    QString name = irl->name();
                                    const char* portName = name.latin1();
                                    if (strcmp(*pn, portName) == 0) {
                                          found = true;
                                          break;
                                          }
                                    }
                              if (!found) {
                                    audio->msgAddRoute1(
                                       Route(*pn, false, channel),
                                       Route(it, channel)
                                       );
                                    }
                              ++pn;
                              }

                        delete ports;
                        ports = NULL;
                        }
                  }
            }
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

                  bool erased;
                  // limit set to 20 iterations for disconnects, don't know how to make it go
                  // the "right" amount
                  for (int i = 0; i < 20 ; i++) {
                        erased = false;
                        for (iRoute irl = rl->begin(); irl != rl->end(); ++irl) {
                              if (irl->channel != channel)
                                    continue;
                              QString name = irl->name();
                              const char* portName = name.latin1();
                              bool found = false;
                              const char** pn = ports;
                              while (pn && *pn) {
                                    if (strcmp(*pn, portName) == 0) {
                                          found = true;
                                          break;
                                          }
                                    ++pn;
                                    }
                              if (!found) {
                                    audio->msgRemoveRoute1(
                                       Route(it, channel),
                                       Route(portName, false, channel)
                                       );
                                    erased = true;
                                    break;
                                    }
                              }
                        if (!erased)
                              break;
                        }

                  //---------------------------------------
                  // check for connects
                  //---------------------------------------

                  if (ports) {
                        const char** pn = ports;
                        while (*pn) {
                              bool found = false;
                              for (iRoute irl = rl->begin(); irl != rl->end(); ++irl) {
                                    if (irl->channel != channel)
                                          continue;
                                    QString name = irl->name();
                                    const char* portName = name.latin1();
                                    if (strcmp(*pn, portName) == 0) {
                                          found = true;
                                          break;
                                          }
                                    }
                              if (!found) {
                                    audio->msgAddRoute1(
                                       Route(it, channel),
                                       Route(*pn, false, channel)
                                       );
                                    }
                              ++pn;
                              }

                        delete ports;
                        ports = NULL;
                        }
                  }
            }
      }

//static int xrun_callback(void*)
//      {
//      printf("JACK: xrun\n");
//      return 0;
//      }

//---------------------------------------------------------
//   register
//---------------------------------------------------------

void JackAudioDevice::registerClient()
      {
      if (JACK_DEBUG)
            printf("registerClient()\n");
      if(!checkJackClient(_client)) return;
      jack_set_process_callback(_client, processAudio, 0);
      jack_set_sync_callback(_client, processSync, 0);
      // FIXME: FIXME:
      // Added by Tim. p3.3.20
      // Did not help. Seek during play: Jack keeps switching to STOP state after about 1-2 seconds timeout if sync is holding it up.
      // Nothing in MusE seems to be telling it to stop.
      jack_set_sync_timeout(_client, 5000000); // Change default 2 to 5 second sync timeout because prefetch may be very slow esp. with resampling !
      
      jack_on_shutdown(_client, processShutdown, 0);
      jack_set_buffer_size_callback(_client, bufsize_callback, 0);
      jack_set_sample_rate_callback(_client, srate_callback, 0);
      jack_set_port_registration_callback(_client, registration_callback, 0);
      jack_set_graph_order_callback(_client, graph_callback, 0);
//      jack_set_xrun_callback(client, xrun_callback, 0);
      jack_set_freewheel_callback (_client, freewheel_callback, 0);
      }

//---------------------------------------------------------
//   registerInPort
//---------------------------------------------------------

void* JackAudioDevice::registerInPort(const char* name)
      {
      if (JACK_DEBUG)
            printf("registerInPort()\n");
      if(!checkJackClient(_client)) return NULL;
      void* p = jack_port_register(_client, name, JACK_DEFAULT_AUDIO_TYPE, JackPortIsInput, 0);
// printf("JACK: registerInPort: <%s> %p\n", name, p);
      return p;
      }

//---------------------------------------------------------
//   registerOutPort
//---------------------------------------------------------

void* JackAudioDevice::registerOutPort(const char* name)
      {
      if (JACK_DEBUG)
            printf("registerOutPort()\n");
      if(!checkJackClient(_client)) return NULL;
      void* p = jack_port_register(_client, name, JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0);
// printf("JACK: registerOutPort: <%s> %p\n", name, p);
      return p;
      }

//---------------------------------------------------------
//   exitJackAudio
//---------------------------------------------------------

void exitJackAudio()
      {
      if (JACK_DEBUG)
            printf("exitJackAudio()\n");
      if (jackAudio)
            delete jackAudio;
            
      // Added by Tim. p3.3.14
      audioDevice = NULL;      
      
      }

//---------------------------------------------------------
//   connect
//---------------------------------------------------------

void JackAudioDevice::connect(void* src, void* dst)
      {
      if (JACK_DEBUG)
            printf("JackAudioDevice::connect()\n");
      if(!checkJackClient(_client)) return;
      const char* sn = jack_port_name((jack_port_t*) src);
      const char* dn = jack_port_name((jack_port_t*) dst);
      if (sn == 0 || dn == 0) {
            fprintf(stderr, "JackAudio::connect: unknown jack ports\n");
            return;
            }
      if (jack_connect(_client, sn, dn)) {
            fprintf(stderr, "jack connect <%s>%p - <%s>%p failed\n",
               sn, src, dn, dst);
            }
      }

//---------------------------------------------------------
//   disconnect
//---------------------------------------------------------

void JackAudioDevice::disconnect(void* src, void* dst)
      {
      if (JACK_DEBUG)
            printf("JackAudioDevice::disconnect()\n");
      if(!checkJackClient(_client)) return;
      const char* sn = jack_port_name((jack_port_t*) src);
      const char* dn = jack_port_name((jack_port_t*) dst);
      if (sn == 0 || dn == 0) {
            fprintf(stderr, "JackAudio::disconnect: unknown jack ports\n");
            return;
            }
      if (jack_disconnect(_client, sn, dn)) {
            fprintf(stderr, "jack disconnect <%s> - <%s> failed\n",
               sn, dn);
            }
      }

//---------------------------------------------------------
//   start
//---------------------------------------------------------

//void JackAudioDevice::start()
void JackAudioDevice::start(int /*priority*/)
      {
      if (JACK_DEBUG)
            printf("JackAudioDevice::start()\n");
      if(!checkJackClient(_client)) return;

      doSetuid();

      if (jack_activate(_client)) {
            fprintf (stderr, "JACK: cannot activate client\n");
            exit(-1);
            }
      /* connect the ports. Note: you can't do this before
         the client is activated, because we can't allow
         connections to be made to clients that aren't
         running.
       */

      InputList* il = song->inputs();
      for (iAudioInput i = il->begin(); i != il->end(); ++i) {
            AudioInput* ai = *i;
            int channel = ai->channels();
            for (int ch = 0; ch < channel; ++ch) {
                  RouteList* rl = ai->inRoutes();
                  void* port = ai->jackPort(ch);
                  for (iRoute ir = rl->begin(); ir != rl->end(); ++ir) {
                        if (ir->channel == ch)
                              connect(ir->jackPort, port);
                        }
                  }
            }
      OutputList* ol = song->outputs();
      for (iAudioOutput i = ol->begin(); i != ol->end(); ++i) {
            AudioOutput* ai = *i;
            int channel = ai->channels();
            for (int ch = 0; ch < channel; ++ch) {
                  RouteList* rl = ai->outRoutes();
                  void* port = ai->jackPort(ch);
                  for (iRoute r = rl->begin(); r != rl->end(); ++r) {
                        if (r->channel == ch) {
                              connect(port, r->jackPort);
                              }
                        }
                  }
            }
      
      undoSetuid();
      
      //MUSE_DEBUG("JackAudioDevice::start()\n");
      fflush(stdin);
      //JackAudioDevice::jackStarted=true;
      }

//---------------------------------------------------------
//   stop
//---------------------------------------------------------

void JackAudioDevice::stop()
      {
      if (JACK_DEBUG)
            printf("JackAudioDevice::stop()\n");
      if(!checkJackClient(_client)) return;
      if (jack_deactivate(_client)) {
            fprintf (stderr, "cannot deactivate client\n");
            }
      //JackAudioDevice::jackStarted=false;
      }

//---------------------------------------------------------
//   transportQuery
//---------------------------------------------------------

jack_transport_state_t JackAudioDevice::transportQuery(jack_position_t* pos)
{ 
  if (JACK_DEBUG)
    printf("JackAudioDevice::transportQuery pos:%d\n", (unsigned int)pos->frame);
  
  // TODO: Compose and return a state if MusE is disengaged from Jack transport.
  
  return jack_transport_query(_client, pos); 
}

//---------------------------------------------------------
//   getCurFrame
//---------------------------------------------------------

unsigned int JackAudioDevice::getCurFrame()
{ 
  if (JACK_DEBUG)
    printf("JackAudioDevice::getCurFrame pos.frame:%d\n", pos.frame);
  
  if(!useJackTransport)
    return (unsigned int)dummyPos;
    
  return pos.frame; 
}

//---------------------------------------------------------
//   framePos
//---------------------------------------------------------

int JackAudioDevice::framePos() const
      {
      //if(!useJackTransport)
      //{
      //  if (JACK_DEBUG)
      //    printf("JackAudioDevice::framePos dummyPos:%d\n", dummyPos);
      //  return dummyPos;
      //}
      
      if(!checkJackClient(_client)) return 0;
      jack_nframes_t n = jack_frame_time(_client);
      
      if (JACK_DEBUG)
        printf("JackAudioDevice::framePos jack frame:%d\n", (int)n);
      
      return (int)n;
      }

#if 0
//---------------------------------------------------------
//   framesSinceCycleStart
//---------------------------------------------------------

int JackAudioDevice::framesSinceCycleStart() const
      {
      jack_nframes_t n = jack_frames_since_cycle_start(client);
      return (int)n;
      }

//---------------------------------------------------------
//   framesDelay
//    TODO
//---------------------------------------------------------

int JackAudioDevice::frameDelay() const
      {
      jack_nframes_t n = (segmentSize * (segmentCount-1)) - jack_frames_since_cycle_start(client);
      return (int)n;
      }
#endif

//---------------------------------------------------------
//   outputPorts
//---------------------------------------------------------

std::list<QString> JackAudioDevice::outputPorts()
      {
      if (JACK_DEBUG)
            printf("JackAudioDevice::outputPorts()\n");
      std::list<QString> clientList;
      if(!checkJackClient(_client)) return clientList;
      const char** ports = jack_get_ports(_client, 0, JACK_DEFAULT_AUDIO_TYPE, 0);
      for (const char** p = ports; p && *p; ++p) {
            jack_port_t* port = jack_port_by_name(_client, *p);
            int flags = jack_port_flags(port);
            if (!(flags & JackPortIsOutput))
                  continue;
            char buffer[128];
            strncpy(buffer, *p, 128);
            if (strncmp(buffer, "MusE", 4) == 0)
                  continue;
            clientList.push_back(QString(buffer));
            }
      return clientList;
      }

//---------------------------------------------------------
//   inputPorts
//---------------------------------------------------------

std::list<QString> JackAudioDevice::inputPorts()
      {
      if (JACK_DEBUG)
            printf("JackAudioDevice::inputPorts()\n");
      std::list<QString> clientList;
      if(!checkJackClient(_client)) return clientList;
      const char** ports = jack_get_ports(_client, 0, JACK_DEFAULT_AUDIO_TYPE, 0);
      for (const char** p = ports; p && *p; ++p) {
            jack_port_t* port = jack_port_by_name(_client, *p);
            int flags = jack_port_flags(port);
            if (!(flags & JackPortIsInput))
                  continue;
            char buffer[128];
            strncpy(buffer, *p, 128);
            if (strncmp(buffer, "MusE", 4) == 0)
                  continue;
            clientList.push_back(QString(buffer));
            }
      return clientList;
      }

//---------------------------------------------------------
//   portName
//---------------------------------------------------------

QString JackAudioDevice::portName(void* port)
      {
      if (JACK_DEBUG)
            printf("JackAudioDevice::portName(\n");
      if(!checkJackClient(_client)) return "";
      if (!port) 
            return "";
      
      QString s(jack_port_name((jack_port_t*)port));
 //printf("Jack::portName %p %s\n", port, s.latin1());
      return s;
      }

//---------------------------------------------------------
//   unregisterPort
//---------------------------------------------------------

void JackAudioDevice::unregisterPort(void* p)
      {
      if (JACK_DEBUG)
            printf("JackAudioDevice::unregisterPort(\n");
      if(!checkJackClient(_client)) return;
//      printf("JACK: unregister Port\n");
      jack_port_unregister(_client, (jack_port_t*)p);
      }

//---------------------------------------------------------
//   getState
//---------------------------------------------------------

int JackAudioDevice::getState()
      {
      // If we're not using Jack's transport, just return current state.
      if(!useJackTransport)
      {
        //pos.valid = jack_position_bits_t(0);
        //pos.frame = audio->pos().frame();
        //return audio->getState();
        if (JACK_DEBUG)
          printf("JackAudioDevice::getState dummyState:%d\n", dummyState);
        return dummyState;
      }
      
      //if (JACK_DEBUG)
      //      printf("JackAudioDevice::getState ()\n");
      if(!checkJackClient(_client)) return 0;
      transportState = jack_transport_query(_client, &pos);
      if (JACK_DEBUG)
          printf("JackAudioDevice::getState transportState:%d\n", transportState);
      
      switch (transportState) {
            case JackTransportStopped:  
              return Audio::STOP;
            case JackTransportLooping:
            case JackTransportRolling:  
              return Audio::PLAY;
            case JackTransportStarting:  
              // Added by Tim. p3.3.6
              //printf("JackAudioDevice::getState JackTransportStarting\n");
              
              return Audio::START_PLAY;
            //case JackTransportNetStarting:  
            // FIXME: Quick and dirty hack to support both Jack-1 and Jack-2
            // Really need a config check of version...
            case 4:  
              // Added by Tim. p3.3.6
              //printf("JackAudioDevice::getState JackTransportNetStarting\n");
              
              return Audio::START_PLAY;
            break;  
            default:
              return Audio::STOP;
            }
      }

//---------------------------------------------------------
//   setFreewheel
//---------------------------------------------------------

void JackAudioDevice::setFreewheel(bool f)
      {
      if (JACK_DEBUG)
            printf("JackAudioDevice::setFreewheel(\n");
      if(!checkJackClient(_client)) return;
//      printf("JACK: setFreewheel %d\n", f);
      jack_set_freewheel(_client, f);
      }

//---------------------------------------------------------
//   dummySync
//---------------------------------------------------------

bool JackAudioDevice::dummySync(int state)
{
  // Roughly segment time length.
  //timespec ts = { 0, (1000000000 * segmentSize) / sampleRate };     // In nanoseconds.
  unsigned int sl = (1000000 * segmentSize) / sampleRate;            // In microseconds.
  
  double ct = curTime();
  // Wait for a default maximum of 5 seconds. 
  // Similar to how Jack is supposed to wait a default of 2 seconds for slow clients.
  // TODO: Make this timeout a 'settings' option so it can be applied both to Jack and here.
  while((curTime() - ct) < 5.0)  
  {
    // Is MusE audio ready to roll?
    if(audio->sync(state, dummyPos))
      return true;
    
    // Not ready. Wait a 'segment', try again...
    //nanosleep(&ts, NULL);  
    usleep(sl);              // usleep is supposed to be obsolete!
  }
    
  //if(JACK_DEBUG)
    printf("JackAudioDevice::dummySync Sync timeout - audio not ready!\n");
  
  return false;
}

//---------------------------------------------------------
//   startTransport
//---------------------------------------------------------

void JackAudioDevice::startTransport()
    {
      if (JACK_DEBUG)
            printf("JackAudioDevice::startTransport()\n");
      
      // If we're not using Jack's transport, just pass PLAY and current frame along
      //  as if processSync was called. 
      if(!useJackTransport)
      {
        //dummyState = Audio::START_PLAY;
        
        // Is MusE audio ready to roll?
        //if(dummySync(dummyState))
        if(dummySync(Audio::START_PLAY))
        {
          // MusE audio is ready to roll. Let's play.
          dummyState = Audio::PLAY;
          return;
        }
          
        // Ready or not, we gotta roll. Similar to how Jack is supposed to roll anyway.
        dummyState = Audio::PLAY;
        return;
      }
      
      if(!checkJackClient(_client)) return;
//      printf("JACK: startTransport\n");
      jack_transport_start(_client);
    }

//---------------------------------------------------------
//   stopTransport
//---------------------------------------------------------

void JackAudioDevice::stopTransport()
    {
      if (JACK_DEBUG)
            printf("JackAudioDevice::stopTransport()\n");
      
      dummyState = Audio::STOP;
      
      if(!useJackTransport)
      {
        //dummyState = Audio::STOP;
        return;
      }
      
      if(!checkJackClient(_client)) return;
      if (transportState != JackTransportStopped) {
        //      printf("JACK: stopTransport\n");
            jack_transport_stop(_client);
            transportState=JackTransportStopped;
            }
    }

//---------------------------------------------------------
//   seekTransport
//---------------------------------------------------------

void JackAudioDevice::seekTransport(unsigned frame)
    {
      if (JACK_DEBUG)
            printf("JackAudioDevice::seekTransport() frame:%d\n", frame);
      
      dummyPos = frame;
      if(!useJackTransport)
      {
        // If we're not using Jack's transport, just pass the current state and new frame along
        //  as if processSync was called. 
        //dummyPos = frame;
        int tempState = dummyState;
        //dummyState = Audio::START_PLAY;
        
        // Is MusE audio ready yet?
        //audio->sync(dummyState, dummyPos);
        //if(dummySync(dummyState))
        if(dummySync(Audio::START_PLAY))
        {
          dummyState = tempState;
          return;
        }
        
        // Not ready, resume previous state anyway.
        // FIXME: Observed: Seek during play: Jack transport STOPs on timeout. 
        // Docs say when starting play, transport will roll anyway, ready or not (observed),
        //  but don't mention what should happen on seek during play. 
        // And setting the slow-sync timeout doesn't seem to do anything!
        //dummyState = tempState;
        dummyState = Audio::STOP;
        return;
      }
      
      if(!checkJackClient(_client)) return;
//      printf("JACK: seekTransport %d\n", frame);
      jack_transport_locate(_client, frame);
    }

//---------------------------------------------------------
//   seekTransport
//---------------------------------------------------------

void JackAudioDevice::seekTransport(const Pos &p)
      {
      if (JACK_DEBUG)
            printf("JackAudioDevice::seekTransport() frame:%d\n", p.frame());
      
      dummyPos = p.frame();
      if(!useJackTransport)
      {
        // If we're not using Jack's transport, just pass the current state and new frame along
        //  as if processSync was called. 
        //dummyPos = p.frame();
        int tempState = dummyState;
        //dummyState = Audio::START_PLAY;
        
        // Is MusE audio ready yet?
        //audio->sync(dummyState, dummyPos);
        //if(dummySync(dummyState))
        if(dummySync(Audio::START_PLAY))
        {
          dummyState = tempState;
          return;
        }
        
        // Not ready, resume previous state anyway.
        // FIXME: See fixme in other seekTransport...
        //dummyState = tempState;
        dummyState = Audio::STOP;
        return;
      }
      
      if(!checkJackClient(_client)) return;
      
      /*
      jack_position_t jp;
      jp.valid = JackPositionBBT;
      p.mbt(&jp.bar, &jp.beat, &jp.tick);
      jp.bar++;
      jp.beat++;
      jp.bar_start_tick = Pos(jp.bar, 0, 0).tick();
      //
      //  dummy:
      //
      jp.beats_per_bar = 4;
      jp.beat_type = 4;
      jp.ticks_per_beat = 384;
      int tempo = tempomap.tempo(p.tick());
      jp.beats_per_minute = (60000000.0 / tempo) * tempomap.globalTempo()/100.0;
      
      jack_transport_reposition(_client, &jp);
      */
      jack_transport_locate(_client, p.frame());
      }

//---------------------------------------------------------
//   findPort
//---------------------------------------------------------

void* JackAudioDevice::findPort(const char* name)
      {
      if (JACK_DEBUG)
            printf("JackAudioDevice::findPort(\n");
      if(!checkJackClient(_client)) return NULL;
      void* p = jack_port_by_name(_client, name);
// printf("Jack::findPort <%s>, %p\n", name, p);
      return p;
      }

int JackAudioDevice::setMaster(bool f)
{
  if (JACK_DEBUG)
    printf("JackAudioDevice::setMaster val:%d\n", f);
  if(!checkJackClient(_client)) 
    return 0;
  
  int r = 0;
  if(f)
  {
    if(useJackTransport)
    {
      // Make Muse the Jack timebase master. Do it unconditionally (second param = 0).
      r = jack_set_timebase_callback(_client, 0, (JackTimebaseCallback) timebase_callback, 0);
      if(debugMsg || JACK_DEBUG)
      {
        if(r)
          printf("JackAudioDevice::setMaster jack_set_timebase_callback failed: result:%d\n", r);
      }      
    }  
    else
    {
      r = 1;
      printf("JackAudioDevice::setMaster cannot set master because useJackTransport is false\n");
    }
  }  
  else
  {
    r = jack_release_timebase(_client);
    if(debugMsg || JACK_DEBUG)
    {
      if(r)
        printf("JackAudioDevice::setMaster jack_release_timebase failed: result:%d\n", r);
    }      
  }
  return r;  
}
