//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: jack.cpp,v 1.30.2.17 2009/12/20 05:00:35 terminator356 Exp $
//  (C) Copyright 2002 Werner Schweer (ws@seh.de)
//  (C) Copyright 2012 Tim E. Real (terminator356 on sourceforge.net)
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

#include "config.h"
#include <string>
#include <set>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <stdarg.h>
//#include <time.h> 
#include <unistd.h>
#include <jack/midiport.h>
#include <string.h>

#include "audio.h"
#include "globals.h"
#include "song.h"
#include "jackaudio.h"
#include "track.h"
#include "pos.h"
#include "tempo.h"
#include "sync.h"
#include "utils.h"
#include "gconfig.h"

#include "midi.h"
#include "mididev.h"
#include "mpevent.h"

#include "jackmidi.h"


#define JACK_DEBUG 0 

//#include "errorhandler.h"

#ifdef VST_SUPPORT
#include <fst.h>
#endif

namespace MusEGlobal {

#ifndef RTCAP
extern void doSetuid();
extern void undoSetuid();
#endif

//---------------------------------------------------------
//   checkAudioDevice - make sure audioDevice exists
//---------------------------------------------------------

bool checkAudioDevice()
      {
      if (audioDevice == NULL) {
            if(debugMsg)
              printf("Muse:checkAudioDevice: no audioDevice\n");
            return false;
            }
      return true;
      }

} // namespace MusEGlobal

namespace MusECore {

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
//   jack_thread_init
//---------------------------------------------------------

static void jack_thread_init (void* )  
      {
      if (JACK_DEBUG)
            printf("jack_thread_init()\n");
      MusEGlobal::doSetuid();
#ifdef VST_SUPPORT
      if (loadVST)
            fst_adopt_thread();
#endif
      MusEGlobal::undoSetuid();
      }

int JackAudioDevice::processAudio(jack_nframes_t frames, void*)
{
      int state_pending = jackAudio->_dummyStatePending;  // Snapshots.
      int pos_pending   = jackAudio->_dummyPosPending;    //
      jackAudio->_dummyStatePending = -1;                 // Reset.
      jackAudio->_dummyPosPending = -1;                   //
      
      jackAudio->_frameCounter += frames;
      MusEGlobal::segmentSize = frames;

      if (MusEGlobal::audio->isRunning())
      {
            // Are we not using Jack transport?
            if(!MusEGlobal::useJackTransport.value())
            {
              // STOP -> STOP, STOP -> START_PLAY, PLAY -> START_PLAY all count as 'syncing'.
              if(((jackAudio->dummyState == Audio::STOP || jackAudio->dummyState == Audio::PLAY) && state_pending == Audio::START_PLAY) 
                 || (jackAudio->dummyState == Audio::STOP && state_pending == Audio::STOP) )
              {
                jackAudio->_syncTimeout = (float)frames / (float)MusEGlobal::sampleRate;  // (Re)start the timeout counter...
                if(pos_pending != -1)
                  jackAudio->dummyPos = pos_pending; // Set the new dummy position.
                if((jackAudio->dummyState == Audio::STOP || jackAudio->dummyState == Audio::PLAY) && state_pending == Audio::START_PLAY)
                  jackAudio->dummyState = Audio::START_PLAY;
              }
              else // All other states such as START_PLAY -> STOP, PLAY -> STOP.
              if(state_pending != -1 && state_pending != jackAudio->dummyState)
              {
                jackAudio->_syncTimeout = 0.0;  // Reset.
                jackAudio->dummyState = state_pending;                
              }
              
              // Is the sync timeout counter running?
              if(jackAudio->_syncTimeout > 0.0)
              {
                //printf("Jack processAudio dummy sync: state:%d pending:%d\n", jackAudio->dummyState, state_pending);  
                // Is MusE audio ready to roll?
                if(MusEGlobal::audio->sync(jackAudio->dummyState, jackAudio->dummyPos))
                {
                  jackAudio->_syncTimeout = 0.0;  // Reset.
                  // We're ready. Switch to PLAY state.
                  if(jackAudio->dummyState == Audio::START_PLAY)
                    jackAudio->dummyState = Audio::PLAY;
                }
                else
                {  
                  jackAudio->_syncTimeout += (float)frames / (float)MusEGlobal::sampleRate;
                  if(jackAudio->_syncTimeout > 5.0)  // TODO: Make this timeout a 'settings' option so it can be applied both to Jack and here.
                  {
                    if (MusEGlobal::debugMsg)
                      puts("Jack dummy sync timeout! Starting anyway...\n");
                    jackAudio->_syncTimeout = 0.0;  // Reset.
                    // We're not ready, but no time left - gotta roll anyway. Switch to PLAY state, similar to how Jack is supposed to work.
                    if(jackAudio->dummyState == Audio::START_PLAY)
                    {
                      jackAudio->dummyState = Audio::PLAY;
                      // Docs say sync will be called with Rolling state when timeout expires.
                      MusEGlobal::audio->sync(jackAudio->dummyState, jackAudio->dummyPos);
                    }
                  }
                }
              }
            }
            
            //if(jackAudio->getState() != Audio::START_PLAY)  // Don't process while we're syncing. TODO: May need to deliver silence in process!
              MusEGlobal::audio->process((unsigned long)frames);
      }
      else {
            if (MusEGlobal::debugMsg)
                 puts("jack calling when audio is disconnected!\n");
            }
            
  return 0;
}

//---------------------------------------------------------
//   processSync
//    return TRUE (non-zero) when ready to roll.
//---------------------------------------------------------

static int processSync(jack_transport_state_t state, jack_position_t* pos, void*)
      {
      if (JACK_DEBUG)
      {
        printf("processSync frame:%u\n", pos->frame);
      
        if(pos->valid & JackPositionBBT)
        {
          if(JACK_DEBUG)
          {
            printf("processSync BBT:\n bar:%d beat:%d tick:%d\n bar_start_tick:%f beats_per_bar:%f beat_type:%f ticks_per_beat:%f beats_per_minute:%f\n",
                    pos->bar, pos->beat, pos->tick, pos->bar_start_tick, pos->beats_per_bar, pos->beat_type, pos->ticks_per_beat, pos->beats_per_minute);
            if(pos->valid & JackBBTFrameOffset)
              printf("processSync BBTFrameOffset: %u\n", pos->bbt_offset);
          }
        }
      }
        
      if(!MusEGlobal::useJackTransport.value())
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
              //printf("processSync JackTransportStarting\n");
              
              audioState = Audio::START_PLAY;
            break;  
            //case JackTransportNetStarting:  
            // FIXME: Quick and dirty hack to support both Jack-1 and Jack-2
            // Really need a config check of version...
            case 4:  
              //printf("processSync JackTransportNetStarting\n");
              
              audioState = Audio::START_PLAY;
            break;  
            }
            
      unsigned frame = pos->frame;
      //return MusEGlobal::audio->sync(audioState, frame);
      int rv = MusEGlobal::audio->sync(audioState, frame);
      //printf("Jack processSync() after MusEGlobal::audio->sync frame:%d\n", frame);
      return rv;      
      }

//---------------------------------------------------------
//   timebase_callback
//---------------------------------------------------------

static void timebase_callback(jack_transport_state_t /* state */,
   jack_nframes_t nframes,
   jack_position_t* pos,
   int new_pos,
   void*)
  {

    if (JACK_DEBUG)
    {
      if(pos->valid & JackPositionBBT)
        printf("timebase_callback BBT:\n bar:%d beat:%d tick:%d\n bar_start_tick:%f beats_per_bar:%f beat_type:%f ticks_per_beat:%f beats_per_minute:%f\n",
                pos->bar, pos->beat, pos->tick, pos->bar_start_tick, pos->beats_per_bar, pos->beat_type, pos->ticks_per_beat, pos->beats_per_minute);
      if(pos->valid & JackBBTFrameOffset)
        printf("timebase_callback BBTFrameOffset: %u\n", pos->bbt_offset);
      if(pos->valid & JackPositionTimecode)
        printf("timebase_callback JackPositionTimecode: frame_time:%f next_time:%f\n", pos->frame_time, pos->next_time);
      if(pos->valid & JackAudioVideoRatio)
        printf("timebase_callback JackAudioVideoRatio: %f\n", pos->audio_frames_per_video_frame);
      if(pos->valid & JackVideoFrameOffset)
        printf("timebase_callback JackVideoFrameOffset: %u\n", pos->video_offset);
    }
    
    //Pos p(pos->frame, false);
      Pos p(MusEGlobal::extSyncFlag.value() ? MusEGlobal::audio->tickPos() : pos->frame, MusEGlobal::extSyncFlag.value() ? true : false);
      // Can't use song pos - it is only updated every (slow) GUI heartbeat !
      //Pos p(MusEGlobal::extSyncFlag.value() ? MusEGlobal::song->cpos() : pos->frame, MusEGlobal::extSyncFlag.value() ? true : false);
      
      pos->valid = JackPositionBBT;
      p.mbt(&pos->bar, &pos->beat, &pos->tick);
      pos->bar_start_tick = Pos(pos->bar, 0, 0).tick();
      pos->bar++;
      pos->beat++;
      
      int z, n;
      AL::sigmap.timesig(p.tick(), z, n);
      pos->beats_per_bar = z;
      pos->beat_type = n;
      pos->ticks_per_beat = MusEGlobal::config.division;
      //pos->ticks_per_beat = 24;
      
      double tempo = MusEGlobal::tempomap.tempo(p.tick());
      pos->beats_per_minute = (60000000.0 / tempo) * double(MusEGlobal::tempomap.globalTempo())/100.0;
      if (JACK_DEBUG)
      {
        printf("timebase_callback is new_pos:%d nframes:%u frame:%u tickPos:%d cpos:%d\n", new_pos, nframes, pos->frame, MusEGlobal::audio->tickPos(), MusEGlobal::song->cpos());
        printf(" new: bar:%d beat:%d tick:%d\n bar_start_tick:%f beats_per_bar:%f beat_type:%f ticks_per_beat:%f beats_per_minute:%f\n",
               pos->bar, pos->beat, pos->tick, pos->bar_start_tick, pos->beats_per_bar, pos->beat_type, pos->ticks_per_beat, pos->beats_per_minute);
      }
      
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
      MusEGlobal::audio->shutdown();

      int c=0;
      while(MusEGlobal::midiSeqRunning == true) {
          if(c++ >10) {
              fprintf(stderr, "sequencer still running, something is very wrong.\n");
              break;
              }
          sleep(1);
          }
      delete jackAudio;
      jackAudio=0;
      MusEGlobal::audioDevice=0;
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
            //printf("noJackError()\n");
      }

//---------------------------------------------------------
//   JackAudioDevice
//---------------------------------------------------------

JackAudioDevice::JackAudioDevice(jack_client_t* cl, char* name)
   : AudioDevice()
      {
      _frameCounter = 0;
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
                  fprintf(stderr,"jack_client_close() failed: %s\n", strerror(errno));
                  }
            }
      if (JACK_DEBUG)
            printf("~JackAudioDevice() after jack_client_close()\n");
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
//   initJackAudio
//    return true if JACK not found
//---------------------------------------------------------

bool initJackAudio()
      {
      if (JACK_DEBUG)
            printf("initJackAudio()\n");
      if (MusEGlobal::debugMsg) {
            fprintf(stderr,"initJackAudio()\n");
            jack_set_error_function(jackError);
            }
      else
            jack_set_error_function(noJackError);
      MusEGlobal::doSetuid();

      //jack_client_t* client = 0;
      //int i = 0;
      //char jackIdString[8];
      //for (i = 0; i < 5; ++i) {
      //      sprintf(jackIdString, "MusE-%d", i+1);
            //client = jack_client_new(jackIdString);
      //      client = jack_client_open(jackIdString, JackNoStartServer, 0);
      //      if (client)
      //            break;
      //      }
      //if (i == 5)
      //      return true;
      jack_status_t status;
      jack_client_t* client = jack_client_open("MusE", JackNoStartServer, &status);
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
	    MusEGlobal::undoSetuid();   
            return true;
            }

      if (MusEGlobal::debugMsg)
            fprintf(stderr, "initJackAudio(): client %s opened.\n", jack_get_client_name(client));
      //jack_set_error_function(jackError);
      //jackAudio = new JackAudioDevice(client, jackIdString);
      jackAudio = new JackAudioDevice(client, jack_get_client_name(client));
      if (MusEGlobal::debugMsg)
            fprintf(stderr, "initJackAudio(): registering client...\n");
      
      MusEGlobal::undoSetuid();
      
      MusEGlobal::audioDevice = jackAudio;

      // WARNING Don't do this here. Do it after any MusE ALSA client is registered, otherwise random crashes can occur.
      //jackAudio->registerClient(); 

      MusEGlobal::sampleRate  = jack_get_sample_rate(client);
      MusEGlobal::segmentSize = jack_get_buffer_size(client);
      
      jackAudio->scanMidiPorts();
      
      return false;
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
      if (MusEGlobal::debugMsg || JACK_DEBUG)
            printf("JACK: freewheel_callback: starting%d\n", starting);
      MusEGlobal::audio->setFreewheel(starting);
      }

static int srate_callback(jack_nframes_t n, void*)
      {
      if (MusEGlobal::debugMsg || JACK_DEBUG)
            printf("JACK: sample rate changed: %d\n", n);
      return 0;
      }

//---------------------------------------------------------
//   registration_callback
//---------------------------------------------------------

static void registration_callback(jack_port_id_t, int, void*)
{
  if(MusEGlobal::debugMsg || JACK_DEBUG)
    printf("JACK: registration changed\n");
        
  MusEGlobal::audio->sendMsgToGui('R');
}

//---------------------------------------------------------
//   JackAudioDevice::registrationChanged
//    this is called from song in gui context triggered
//    by registration_callback()
//---------------------------------------------------------

void JackAudioDevice::registrationChanged()
{
  if(JACK_DEBUG)
    printf("JackAudioDevice::registrationChanged()\n");
  
  // Rescan.
  scanMidiPorts();
  // Connect the Jack midi client ports to the device ports.
  //connectJackMidiPorts();
}

//---------------------------------------------------------
//   JackAudioDevice::connectJackMidiPorts
//---------------------------------------------------------

void JackAudioDevice::connectJackMidiPorts()
{
  if(JACK_DEBUG)
    printf("JackAudioDevice::connectJackMidiPorts()\n");
  
  for (iMidiDevice i = MusEGlobal::midiDevices.begin(); i != MusEGlobal::midiDevices.end(); ++i) 
  {
    //MidiJackDevice* mjd = dynamic_cast<MidiJackDevice*>(*i);
    //if(!mjd)
    MidiDevice* md = *i;
    if(md->deviceType() != MidiDevice::JACK_MIDI)
      continue;
    
    //void* port = md->clientPort();
    if(md->rwFlags() & 1)
    {
      void* port = md->outClientPort(); 
      if(port)                           
      {
        RouteList* rl = md->outRoutes();
        for (ciRoute r = rl->begin(); r != rl->end(); ++r) 
        {  
          if(r->type != Route::JACK_ROUTE)  
            continue;
          connect(port, r->jackPort);
        }  
      }    
    }
    
    if(md->rwFlags() & 2)
    {  
      void* port = md->inClientPort();  
      if(port)                          
      {
        RouteList* rl = md->inRoutes();
        for (ciRoute r = rl->begin(); r != rl->end(); ++r) 
        {  
          if(r->type != Route::JACK_ROUTE)  
            continue;
          connect(r->jackPort, port);
        }
      }    
    }    
  }
}

//---------------------------------------------------------
//   client_registration_callback
//---------------------------------------------------------

static void client_registration_callback(const char *name, int isRegister, void*)
      {
      if (MusEGlobal::debugMsg || JACK_DEBUG)
            printf("JACK: client registration changed:%s register:%d\n", name, isRegister);
      }

//---------------------------------------------------------
//   port_connect_callback
//---------------------------------------------------------

static void port_connect_callback(jack_port_id_t a, jack_port_id_t b, int isConnect, void*)
      {
        if (MusEGlobal::debugMsg || JACK_DEBUG)
        {
            //jack_port_t* ap = jack_port_by_id(_client, a);
            //jack_port_t* bp = jack_port_by_id(_client, b);
            //printf("JACK: port connections changed: A:%d:%s B:%d:%s isConnect:%d\n", a, jack_port_name(ap), b, jack_port_name(bp), isConnect);
            printf("JACK: port connections changed: A:%d B:%d isConnect:%d\n", a, b, isConnect);
        }    
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
      MusEGlobal::audio->sendMsgToGui('C');
      if (MusEGlobal::debugMsg)
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
      InputList* il = MusEGlobal::song->inputs();
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
                        for (ciRoute irl = rl->begin(); irl != rl->end(); ++irl) {
                              if(irl->type != Route::JACK_ROUTE)  
                                continue;
                              if (irl->channel != channel)
                                    continue;
                              QString name = irl->name();
                              QByteArray ba = name.toLatin1();
                              const char* portName = ba.constData();
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
                                    MusEGlobal::audio->msgRemoveRoute1(
                                       //Route(portName, false, channel),
                                       Route(portName, false, channel, Route::JACK_ROUTE),
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
                              for (ciRoute irl = rl->begin(); irl != rl->end(); ++irl) {
                                    if(irl->type != Route::JACK_ROUTE)  
                                      continue;
                                    if (irl->channel != channel)
                                          continue;
                                    QString name = irl->name();
				    QByteArray ba = name.toLatin1();
                                    const char* portName = ba.constData();
                                    if (strcmp(*pn, portName) == 0) {
                                          found = true;
                                          break;
                                          }
                                    }
                              if (!found) {
                                    MusEGlobal::audio->msgAddRoute1(
                                       //Route(*pn, false, channel),
                                       Route(*pn, false, channel, Route::JACK_ROUTE),
                                       Route(it, channel)
                                       );
                                    }
                              ++pn;
                              }

                        jack_free(ports);  // p4.0.29
                        
                        ports = NULL;
                        }
                  }
            }
      OutputList* ol = MusEGlobal::song->outputs();
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
                        for (ciRoute irl = rl->begin(); irl != rl->end(); ++irl) {
                              if(irl->type != Route::JACK_ROUTE)  
                                continue;
                              if (irl->channel != channel)
                                    continue;
                              QString name = irl->name();
                              QByteArray ba = name.toLatin1();
                              const char* portName = ba.constData();
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
                                    MusEGlobal::audio->msgRemoveRoute1(
                                       Route(it, channel),
                                       //Route(portName, false, channel)
                                       Route(portName, false, channel, Route::JACK_ROUTE)
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
                              for (ciRoute irl = rl->begin(); irl != rl->end(); ++irl) {
                                    if(irl->type != Route::JACK_ROUTE)  
                                      continue;
                                    if (irl->channel != channel)
                                          continue;
                                    QString name = irl->name();
                                    QByteArray ba = name.toLatin1();
                                    const char* portName = ba.constData();
                                    if (strcmp(*pn, portName) == 0) {
                                          found = true;
                                          break;
                                          }
                                    }
                              if (!found) {
                                    MusEGlobal::audio->msgAddRoute1(
                                       Route(it, channel),
                                       //Route(*pn, false, channel)
                                       Route(*pn, false, channel, Route::JACK_ROUTE)
                                       );
                                    }
                              ++pn;
                              }

                        jack_free(ports);  // p4.0.29
                        
                        ports = NULL;
                        }
                  }
            }
            
      for (iMidiDevice ii = MusEGlobal::midiDevices.begin(); ii != MusEGlobal::midiDevices.end(); ++ii) 
      {
            MidiDevice* md = *ii;
            if(md->deviceType() != MidiDevice::JACK_MIDI)
              continue;
            
            //MidiJackDevice* mjd = dynamic_cast<MidiJackDevice*>(*ii);
            //if(!mjd)
            //  continue;
            //for (int channel = 0; channel < channels; ++channel) 
            //{
                  
                  // p3.3.55 Removed
                  //jack_port_t* port = (jack_port_t*)md->clientPort();
                  //if (port == 0)
                  //      continue;
                  //const char** ports = jack_port_get_all_connections(_client, port);
                  
                  //---------------------------------------
                  // outputs
                  //---------------------------------------
                  
                  if(md->rwFlags() & 1) // Writable
                  {
                    jack_port_t* port = (jack_port_t*)md->outClientPort();
                    if(port != 0)
                    {
                      const char** ports = jack_port_get_all_connections(_client, port);
                      
                      RouteList* rl      = md->outRoutes();
    
                      //---------------------------------------
                      // check for disconnects
                      //---------------------------------------
    
                      bool erased;
                      // limit set to 20 iterations for disconnects, don't know how to make it go
                      // the "right" amount
                      for (int i = 0; i < 20 ; i++) 
                      {
                            erased = false;
                            for (ciRoute irl = rl->begin(); irl != rl->end(); ++irl) {
                                  if(irl->type != Route::JACK_ROUTE)  
                                    continue;
                                  //if (irl->channel != channel)
                                  //      continue;
                                  QString name = irl->name();
                                  //name += QString(JACK_MIDI_OUT_PORT_SUFFIX);    
                                  QByteArray ba = name.toLatin1();
                                  const char* portName = ba.constData();
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
                                        MusEGlobal::audio->msgRemoveRoute1(
                                          //Route(it, channel),
                                          //Route(mjd),
                                          Route(md, -1),
                                          //Route(portName, false, channel)
                                          //Route(portName, false, -1)
                                          Route(portName, false, -1, Route::JACK_ROUTE)
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
    
                      if (ports) 
                      {
                            const char** pn = ports;
                            while (*pn) {
                                  bool found = false;
                                  for (ciRoute irl = rl->begin(); irl != rl->end(); ++irl) {
                                        if(irl->type != Route::JACK_ROUTE)  
                                          continue;
                                        //if (irl->channel != channel)
                                        //      continue;
                                        QString name = irl->name();
                                        QByteArray ba = name.toLatin1();
                                        const char* portName = ba.constData();
                                        if (strcmp(*pn, portName) == 0) {
                                              found = true;
                                              break;
                                              }
                                        }
                                  if (!found) {
                                        MusEGlobal::audio->msgAddRoute1(
                                          //Route(it, channel),
                                          //Route(mjd),
                                          Route(md, -1),
                                          //Route(*pn, false, channel)
                                          //Route(*pn, false, -1)
                                          Route(*pn, false, -1, Route::JACK_ROUTE)
                                          );
                                        }
                                  ++pn;
                                  }
    
                            jack_free(ports);  // p4.0.29
                      }
                    }  
                  }  
                  
                  
                  //------------------------
                  // Inputs
                  //------------------------
                  
                  if(md->rwFlags() & 2) // Readable
                  {
                    jack_port_t* port = (jack_port_t*)md->inClientPort();
                    if(port != 0)
                    {
                      const char** ports = jack_port_get_all_connections(_client, port);
                      
                      RouteList* rl = md->inRoutes();
    
                      //---------------------------------------
                      // check for disconnects
                      //---------------------------------------
    
                      bool erased;
                      // limit set to 20 iterations for disconnects, don't know how to make it go
                      // the "right" amount
                      for (int i = 0; i < 20 ; i++) 
                      {
                            erased = false;
                            for (ciRoute irl = rl->begin(); irl != rl->end(); ++irl) {
                                  if(irl->type != Route::JACK_ROUTE)  
                                    continue;
                                  //if (irl->channel != channel)
                                  //      continue;
                                  QString name = irl->name();
                                  QByteArray ba = name.toLatin1();
                                  const char* portName = ba.constData();
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
                                        MusEGlobal::audio->msgRemoveRoute1(
                                          //Route(portName, false, channel),
                                          //Route(portName, false, -1),
                                          Route(portName, false, -1, Route::JACK_ROUTE),
                                          //Route(it, channel)
                                          //Route(mjd)
                                          Route(md, -1)
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
    
                      if (ports) 
                      {
                            const char** pn = ports;
                            while (*pn) {
                                  bool found = false;
                                  for (ciRoute irl = rl->begin(); irl != rl->end(); ++irl) {
                                        if(irl->type != Route::JACK_ROUTE)  
                                          continue;
                                        //if (irl->channel != channel)
                                        //      continue;
                                        QString name = irl->name();
                                        QByteArray ba = name.toLatin1();
                                        const char* portName = ba.constData();
                                        if (strcmp(*pn, portName) == 0) {
                                              found = true;
                                              break;
                                              }
                                        }
                                  if (!found) {
                                        MusEGlobal::audio->msgAddRoute1(
                                          //Route(*pn, false, channel),
                                          //Route(*pn, false, -1),
                                          Route(*pn, false, -1, Route::JACK_ROUTE),
                                          //Route(it, channel)
                                          //Route(mjd)
                                          Route(md, -1)
                                          );
                                        }
                                  ++pn;
                                  }
                                  
                            jack_free(ports);  // p4.0.29
                      }
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

      jack_set_thread_init_callback(_client, (JackThreadInitCallback) jack_thread_init, 0);
      //jack_set_timebase_callback(client, 0, (JackTimebaseCallback) timebase_callback, 0);
      jack_set_process_callback(_client, processAudio, 0);
      jack_set_sync_callback(_client, processSync, 0);
      //jack_set_sync_timeout(_client, 5000000); // Change default 2 to 5 second sync timeout because prefetch may be very slow esp. with resampling !
      
      jack_on_shutdown(_client, processShutdown, 0);
      jack_set_buffer_size_callback(_client, bufsize_callback, 0);
      jack_set_sample_rate_callback(_client, srate_callback, 0);
      jack_set_port_registration_callback(_client, registration_callback, 0);
      jack_set_client_registration_callback(_client, client_registration_callback, 0);
      jack_set_port_connect_callback(_client, port_connect_callback, 0);
      
      jack_set_graph_order_callback(_client, graph_callback, 0);
//      jack_set_xrun_callback(client, xrun_callback, 0);
      jack_set_freewheel_callback (_client, freewheel_callback, 0);
      }

//---------------------------------------------------------
//   registerInPort
//---------------------------------------------------------

void* JackAudioDevice::registerInPort(const char* name, bool midi)
      {
      if (JACK_DEBUG)
            printf("registerInPort()\n");
      if(!checkJackClient(_client)) return NULL;
      const char* type = midi ? JACK_DEFAULT_MIDI_TYPE : JACK_DEFAULT_AUDIO_TYPE;
      //void* p = jack_port_register(_client, name, JACK_DEFAULT_AUDIO_TYPE, JackPortIsInput, 0);
      void* p = jack_port_register(_client, name, type, JackPortIsInput, 0);
// printf("JACK: registerInPort: <%s> %p\n", name, p);
      return p;
      }

//---------------------------------------------------------
//   registerOutPort
//---------------------------------------------------------

void* JackAudioDevice::registerOutPort(const char* name, bool midi)
      {
      if (JACK_DEBUG)
            printf("registerOutPort()\n");
      if(!checkJackClient(_client)) return NULL;
      const char* type = midi ? JACK_DEFAULT_MIDI_TYPE : JACK_DEFAULT_AUDIO_TYPE;
      //void* p = jack_port_register(_client, name, JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0);
      void* p = jack_port_register(_client, name, type, JackPortIsOutput, 0);
// printf("JACK: registerOutPort: <%s> %p\n", name, p);
      return p;
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
      int err = jack_connect(_client, sn, dn);
      //if (jack_connect(_client, sn, dn)) {
      if (err) {
            fprintf(stderr, "jack connect <%s>%p - <%s>%p failed with err:%d\n",
               sn, src, dn, dst, err);
            }
      else
      if (JACK_DEBUG)
      {
        fprintf(stderr, "jack connect <%s>%p - <%s>%p succeeded\n",
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
      if(!src || !dst)  
        return;
      const char* sn = jack_port_name((jack_port_t*) src);
      const char* dn = jack_port_name((jack_port_t*) dst);
      if (sn == 0 || dn == 0) {
            fprintf(stderr, "JackAudio::disconnect: unknown jack ports\n");
            return;
            }
      int err = jack_disconnect(_client, sn, dn);
      //if (jack_disconnect(_client, sn, dn)) {
      if (err) {
            fprintf(stderr, "jack disconnect <%s> - <%s> failed with err:%d\n",
               sn, dn, err);
            }
      else
      if (JACK_DEBUG)
      {
            fprintf(stderr, "jack disconnect <%s> - <%s> succeeded\n",
               sn, dn);
      }      
}

//---------------------------------------------------------
//   start
//---------------------------------------------------------

void JackAudioDevice::start(int /*priority*/)
      {
      if (JACK_DEBUG)
            printf("JackAudioDevice::start()\n");
      if(!checkJackClient(_client)) return;

      MusEGlobal::doSetuid();

      if (jack_activate(_client)) {
            MusEGlobal::undoSetuid();   
            fprintf (stderr, "JACK: cannot activate client\n");
            exit(-1);
            }
      /* connect the ports. Note: you can't do this before
         the client is activated, because we can't allow
         connections to be made to clients that aren't
         running.
       */

      InputList* il = MusEGlobal::song->inputs();
      for (iAudioInput i = il->begin(); i != il->end(); ++i) {
            AudioInput* ai = *i;
            int channel = ai->channels();
            for (int ch = 0; ch < channel; ++ch) {
                  RouteList* rl = ai->inRoutes();
                  void* port = ai->jackPort(ch);
                  for (ciRoute ir = rl->begin(); ir != rl->end(); ++ir) {
                        if(ir->type != Route::JACK_ROUTE)  
                          continue;
                        if (ir->channel == ch)
                              connect(ir->jackPort, port);
                        }
                  }
            }
      OutputList* ol = MusEGlobal::song->outputs();
      for (iAudioOutput i = ol->begin(); i != ol->end(); ++i) {
            AudioOutput* ai = *i;
            int channel = ai->channels();
            for (int ch = 0; ch < channel; ++ch) {
                  RouteList* rl = ai->outRoutes();
                  void* port = ai->jackPort(ch);
                  for (ciRoute r = rl->begin(); r != rl->end(); ++r) {
                        if(r->type != Route::JACK_ROUTE)  
                          continue;
                        if (r->channel == ch) {
                              connect(port, r->jackPort);
                              }
                        }
                  }
            }
      
      // Connect the Jack midi client ports to device ports.
      connectJackMidiPorts();
      
      MusEGlobal::undoSetuid();
      
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
//   timebaseQuery
//   Given the number of frames in this period, get the bar, beat, tick, 
//    and current absolute tick, and number of ticks in this period.
//   Return false if information could not be obtained.
//---------------------------------------------------------

bool JackAudioDevice::timebaseQuery(unsigned frames, unsigned* bar, unsigned* beat, unsigned* tick, unsigned* curr_abs_tick, unsigned* next_ticks) 
{
  jack_position_t jp;
  jack_transport_query(_client, &jp); 

  if(JACK_DEBUG)
    printf("timebaseQuery frame:%u\n", jp.frame); 
  
  if(jp.valid & JackPositionBBT)
  {
    if(JACK_DEBUG)
    {
      printf("timebaseQuery BBT:\n bar:%d beat:%d tick:%d\n bar_start_tick:%f beats_per_bar:%f beat_type:%f ticks_per_beat:%f beats_per_minute:%f\n",
              jp.bar, jp.beat, jp.tick, jp.bar_start_tick, jp.beats_per_bar, jp.beat_type, jp.ticks_per_beat, jp.beats_per_minute);
      if(jp.valid & JackBBTFrameOffset)
        printf("timebaseQuery BBTFrameOffset: %u\n", jp.bbt_offset);
    }
    
    if(jp.ticks_per_beat > 0.0)
    {
      unsigned muse_tick = unsigned((double(jp.tick) / jp.ticks_per_beat) * double(MusEGlobal::config.division));
      unsigned curr_tick = ((jp.bar - 1) * jp.beats_per_bar + (jp.beat - 1)) * double(MusEGlobal::config.division) + muse_tick;
      // Prefer the reported frame rate over the app's rate if possible.  
      double f_rate = jp.frame_rate != 0 ? jp.frame_rate : MusEGlobal::sampleRate;
      // beats_per_minute is "supposed" to be quantized to period size - that is, computed
      //  so that mid-period changes are averaged out to produce a single tempo which 
      //  produces the same tick in the end. If we can rely on that, we should be good accuracy.
      unsigned ticks  = double(MusEGlobal::config.division) * (jp.beats_per_minute / 60.0) * double(frames) / f_rate;   

      if(JACK_DEBUG)
        printf("timebaseQuery curr_tick:%u f_rate:%f ticks:%u\n", curr_tick, f_rate, ticks);  

      if(bar) *bar = jp.bar;
      if(beat) *beat = jp.beat;
      if(tick) *tick = muse_tick;
      
      if(curr_abs_tick) *curr_abs_tick = curr_tick;
      if(next_ticks) *next_ticks = ticks;
        
      return true;
    }
  }

  if(JACK_DEBUG)
  {
    if(jp.valid & JackPositionTimecode)
      printf("timebaseQuery JackPositionTimecode: frame_time:%f next_time:%f\n", jp.frame_time, jp.next_time);
    if(jp.valid & JackAudioVideoRatio)
      printf("timebaseQuery JackAudioVideoRatio: %f\n", jp.audio_frames_per_video_frame);
    if(jp.valid & JackVideoFrameOffset)
      printf("timebaseQuery JackVideoFrameOffset: %u\n", jp.video_offset);
  }
  
  return false;
}                

//---------------------------------------------------------
//   systemTime
//   Return system time. Depends on selected clock source. 
//   With Jack, may be based upon wallclock time, the   
//    processor cycle counter or the HPET clock etc.
//---------------------------------------------------------

double JackAudioDevice::systemTime() const
{
  // Client valid? According to sletz: For jack_get_time "There are some timing related 
  //  initialization that are done once when a first client is created."
  if(!checkJackClient(_client))
  {
    struct timeval t;
    gettimeofday(&t, 0);
    //printf("%ld %ld\n", t.tv_sec, t.tv_usec);  // Note I observed values coming out of order! Causing some problems.
    return (double)((double)t.tv_sec + (t.tv_usec / 1000000.0));
  }
  
  jack_time_t t = jack_get_time();
  return double(t) / 1000000.0;
}

//---------------------------------------------------------
//   getCurFrame
//---------------------------------------------------------

unsigned int JackAudioDevice::getCurFrame() const
{ 
  if (JACK_DEBUG)
    printf("JackAudioDevice::getCurFrame pos.frame:%d\n", pos.frame);
  
  if(!MusEGlobal::useJackTransport.value())
    return (unsigned int)dummyPos;
    
  return pos.frame; 
}

//---------------------------------------------------------
//   framePos
//---------------------------------------------------------

int JackAudioDevice::framePos() const
      {
      //if(!MusEGlobal::useJackTransport.value())
      //{
      //  if (JACK_DEBUG)
      //    printf("JackAudioDevice::framePos dummyPos:%d\n", dummyPos);
      //  return dummyPos;
      //}
      
      if(!checkJackClient(_client)) return 0;
      jack_nframes_t n = jack_frame_time(_client);
      
      //if (JACK_DEBUG)
      //  printf("JackAudioDevice::framePos jack frame:%d\n", (int)n);
      
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
      jack_nframes_t n = (MusEGlobal::segmentSize * (segmentCount-1)) - jack_frames_since_cycle_start(client);
      return (int)n;
      }
#endif

//---------------------------------------------------------
//   getJackPorts
//---------------------------------------------------------

void JackAudioDevice::getJackPorts(const char** ports, std::list<QString>& name_list, bool midi, bool physical, int aliases)
      {
      if (JACK_DEBUG)
            printf("JackAudioDevice::getJackPorts()\n");
      //std::list<QString> clientList;
      //if(!checkJackClient(_client)) return clientList;
      //if(!checkJackClient(_client)) return;
      QString qname;
      //const char* type = midi ? JACK_DEFAULT_MIDI_TYPE : JACK_DEFAULT_AUDIO_TYPE;
      //const char** ports = jack_get_ports(_client, 0, type, JackPortIsInput);
      //const char** ports = jack_get_ports(_client, 0, type, jflags);
      
      QString cname(jack_get_client_name(_client));
      
      for (const char** p = ports; p && *p; ++p) {
            jack_port_t* port = jack_port_by_name(_client, *p);
            //int flags = jack_port_flags(port);
            //if (!(flags & JackPortIsInput))
            //      continue;
            //char buffer[128];
            
            int port_flags = jack_port_flags(port);
            //printf("JackAudioDevice::getJackPorts port: %s flags: %d\n", *p, port_flags); 

            // Ignore our own client ports.
            if(jack_port_is_mine(_client, port))
            {
              if(MusEGlobal::debugMsg)
                printf("JackAudioDevice::getJackPorts ignoring own port: %s\n", *p);
              continue;         
            }
            
            int nsz = jack_port_name_size();
            char buffer[nsz];

            bool mthrough = false;
            
            if(midi)
            {  
              strncpy(buffer, *p, nsz);
              char a2[nsz]; 
              char* al[2];
              al[0] = buffer;
              al[1] = a2;
              int na = jack_port_get_aliases(port, al);
              if(na >= 1)
              {
                qname = QString(al[0]);
                    //printf("Checking port name for: %s\n", (QString("alsa_pcm:") + cname + QString("/")).toLatin1().constData());  
                // Ignore our own ALSA client!
                if(qname.startsWith(QString("alsa_pcm:") + cname + QString("/")))
                  continue;
                // Put Midi Through after all others.
                mthrough = qname.startsWith(QString("alsa_pcm:Midi-Through/"));  
                //if((physical && mthrough) || (!physical && !mthrough))
                //if(physical && mthrough)
                //  continue;
              }    
            }  
            // Put physical/terminal ports before others.
            bool is_phys = (port_flags & (JackPortIsTerminal | JackPortIsPhysical)) && !mthrough;
            if((physical && !is_phys) || (!physical && is_phys))
              continue;
            

            strncpy(buffer, *p, nsz);
            if((aliases == 0) || (aliases == 1)) 
            {
              char a2[nsz]; 
              char* al[2];
              al[0] = buffer;
              al[1] = a2;
              int na = jack_port_get_aliases(port, al);  
              int a = aliases;
              if(a >= na)
              {
                a = na;
                if(a > 0)
                  a--;
              }    
              qname = QString(al[a]);
            }
            else
              qname = QString(buffer);
            
            //clientList.push_back(QString(buffer));
            name_list.push_back(qname);
            }
            
      //  jack_free(ports);  // p4.0.29
      
      //return clientList;
      }
      
//---------------------------------------------------------
//   outputPorts
//---------------------------------------------------------

std::list<QString> JackAudioDevice::outputPorts(bool midi, int aliases)
      {
      if (JACK_DEBUG)
            printf("JackAudioDevice::outputPorts()\n");
      std::list<QString> clientList;
      if(!checkJackClient(_client)) return clientList;
      const char* type = midi ? JACK_DEFAULT_MIDI_TYPE : JACK_DEFAULT_AUDIO_TYPE;
      const char** ports = jack_get_ports(_client, 0, type, JackPortIsOutput);
      
      if(ports)
      {
        getJackPorts(ports, clientList, midi, true, aliases);   // Get physical ports first.
        getJackPorts(ports, clientList, midi, false, aliases);  // Get non-physical ports last.
        jack_free(ports);  
      }
        
      return clientList;
      }

//---------------------------------------------------------
//   inputPorts
//---------------------------------------------------------

std::list<QString> JackAudioDevice::inputPorts(bool midi, int aliases)
      {
      if (JACK_DEBUG)
            printf("JackAudioDevice::inputPorts()\n");
      
      std::list<QString> clientList;
      if(!checkJackClient(_client)) return clientList;
      const char* type = midi ? JACK_DEFAULT_MIDI_TYPE : JACK_DEFAULT_AUDIO_TYPE;
      const char** ports = jack_get_ports(_client, 0, type, JackPortIsInput);
      
      if(ports)
      {
        getJackPorts(ports, clientList, midi, true, aliases);   // Get physical ports first.
        getJackPorts(ports, clientList, midi, false, aliases);  // Get non-physical ports last.
        jack_free(ports);  
      }
        
      return clientList;
      }

//---------------------------------------------------------
//   portName
//---------------------------------------------------------

QString JackAudioDevice::portName(void* port)
      {
      //if (JACK_DEBUG)
      //      printf("JackAudioDevice::portName\n");
      if(!checkJackClient(_client)) return "";
      if (!port) 
            return "";
      
      QString s(jack_port_name((jack_port_t*)port));
 //printf("Jack::portName %p %s\n", port, s.toLatin1());
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
      if(!MusEGlobal::useJackTransport.value())
      {
        //pos.valid = jack_position_bits_t(0);
        //pos.frame = MusEGlobal::audio->pos().frame();
        //return MusEGlobal::audio->getState();
        //if (JACK_DEBUG)
        //  printf("JackAudioDevice::getState dummyState:%d\n", dummyState);
        return dummyState;
      }
      
      //if (JACK_DEBUG)
      //      printf("JackAudioDevice::getState ()\n");
      if(!checkJackClient(_client)) return 0;
      transportState = jack_transport_query(_client, &pos);
      //if (JACK_DEBUG)
      //    printf("JackAudioDevice::getState transportState:%d\n", transportState);
      
      switch (transportState) {
            case JackTransportStopped:  
              return Audio::STOP;
            case JackTransportLooping:
            case JackTransportRolling:  
              return Audio::PLAY;
            case JackTransportStarting:  
              //printf("JackAudioDevice::getState JackTransportStarting\n");
              
              return Audio::START_PLAY;
            //case JackTransportNetStarting:  
            // FIXME: Quick and dirty hack to support both Jack-1 and Jack-2
            // Really need a config check of version...
            case 4:  
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
//   startTransport
//---------------------------------------------------------

void JackAudioDevice::startTransport()
    {
      if (JACK_DEBUG)
            printf("JackAudioDevice::startTransport()\n");
      
      // If we're not using Jack's transport, just pass PLAY and current frame along
      //  as if processSync was called. 
      if(!MusEGlobal::useJackTransport.value())
      {
        _dummyStatePending = Audio::START_PLAY;
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
      
      if(!MusEGlobal::useJackTransport.value())
      {
        _dummyStatePending = Audio::STOP;
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
      
      if(!MusEGlobal::useJackTransport.value())
      {
        _dummyPosPending   = frame;
        // STOP -> STOP means seek in stop mode. PLAY -> START_PLAY means seek in play mode.
        _dummyStatePending = (dummyState == Audio::STOP ? Audio::STOP : Audio::START_PLAY);
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
            printf("JackAudioDevice::seekTransport(Pos) frame:%d\n", p.frame());
      
      if(!MusEGlobal::useJackTransport.value())
      {
        _dummyPosPending   = p.frame();
        // STOP -> STOP means seek in stop mode. PLAY -> START_PLAY means seek in play mode.
        _dummyStatePending = (dummyState == Audio::STOP ? Audio::STOP : Audio::START_PLAY);
        return;
      }
      
      if(!checkJackClient(_client)) return;

// TODO: Be friendly to other apps... Sadly not many of us use jack_transport_reposition.
//       This is actually required IF we want the extra position info to show up
//        in the sync callback, otherwise we get just the frame only.
//       This information is shared on the server, it is directly passed around. 
//       jack_transport_locate blanks the info from sync until the timebase callback reads 
//        it again right after, from some timebase master. See process in audio.cpp     

//       jack_position_t jp;
//       jp.frame = p.frame();
//       
//       jp.valid = JackPositionBBT;
//       p.mbt(&jp.bar, &jp.beat, &jp.tick);
//       jp.bar_start_tick = Pos(jp.bar, 0, 0).tick();
//       jp.bar++;
//       jp.beat++;
//       jp.beats_per_bar = 5;  // TODO Make this correct !
//       jp.beat_type = 8;      //
//       jp.ticks_per_beat = MusEGlobal::config.division;
//       int tempo = MusEGlobal::tempomap.tempo(p.tick());
//       jp.beats_per_minute = (60000000.0 / tempo) * MusEGlobal::tempomap.globalTempo()/100.0;
//       jack_transport_reposition(_client, &jp);
      
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

//---------------------------------------------------------
//   setMaster
//---------------------------------------------------------

int JackAudioDevice::setMaster(bool f)
{
  if (JACK_DEBUG)
    printf("JackAudioDevice::setMaster val:%d\n", f);
  if(!checkJackClient(_client)) 
    return 0;
  
  int r = 0;
  if(f)
  {
    if(MusEGlobal::useJackTransport.value())
    {
      // Make Muse the Jack timebase master. Do it unconditionally (second param = 0).
      r = jack_set_timebase_callback(_client, 0, (JackTimebaseCallback) timebase_callback, 0);
      if(MusEGlobal::debugMsg || JACK_DEBUG)
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
    if(MusEGlobal::debugMsg || JACK_DEBUG)
    {
      if(r)
        printf("JackAudioDevice::setMaster jack_release_timebase failed: result:%d\n", r);
    }      
  }
  return r;  
}

//---------------------------------------------------------
//   scanMidiPorts
//---------------------------------------------------------

void JackAudioDevice::scanMidiPorts()
{
  //if(MusEGlobal::debugMsg)
  //  printf("JackAudioDevice::scanMidiPorts:\n");
  
/*  
  const char* type = JACK_DEFAULT_MIDI_TYPE;
  const char** ports = jack_get_ports(_client, 0, type, 0);
  
  std::set<std::string> names;
  for (const char** p = ports; p && *p; ++p) 
  {
    jack_port_t* port = jack_port_by_name(_client, *p);
    if(!port)
      continue;
    // Ignore our own client ports.
    if(jack_port_is_mine(_client, port))
    {
      if(MusEGlobal::debugMsg)
        printf(" ignoring own port: %s\n", *p);
      continue;         
    }
    
    int nsz = jack_port_name_size();
    char buffer[nsz];
    strncpy(buffer, *p, nsz);
    // Ignore the MusE Jack port.
    //if(strncmp(buffer, "MusE", 4) == 0)
    //  continue;
    
    if(MusEGlobal::debugMsg)
      printf(" found port: %s  ", buffer);
    
    // If there are aliases for this port, use the first one - much better for identifying. 
    //char a1[nsz]; 
    char a2[nsz]; 
    char* aliases[2];
    //aliases[0] = a1;
    aliases[0] = buffer;
    aliases[1] = a2;
    // To disable aliases, just rem this line.
    jack_port_get_aliases(port, aliases);
    //int na = jack_port_get_aliases(port, aliases);
    //char* namep = (na >= 1) ? aliases[0] : buffer;
    //char* namep = aliases[0];
    //names.insert(std::string(*p));
    if(MusEGlobal::debugMsg)
      printf("alias: %s\n", aliases[0]);
    
    names.insert(std::string(aliases[0]));
  }
  if(ports)
    free(ports);      
  
  std::list<MidiDevice*> to_del;
  for(iMidiDevice imd = MusEGlobal::midiDevices.begin(); imd != MusEGlobal::midiDevices.end(); ++imd)
  {
    // Only Jack midi devices.
    if(dynamic_cast<MidiJackDevice*>(*imd) == 0)
      continue;
    if(names.find(std::string((*imd)->name().toLatin1())) == names.end())
      to_del.push_back(*imd);
  }
  
  for(std::list<MidiDevice*>::iterator imd = to_del.begin(); imd != to_del.end(); ++imd)
  {
    if(MusEGlobal::debugMsg)
      printf(" removing port device:%s\n", (*imd)->name().toLatin1());
    MusEGlobal::midiDevices.remove(*imd);
    // This will close (and unregister) the client port.
    delete (*imd);
  }
  
  //for (const char** p = ports; p && *p; ++p) 
  for(std::set<std::string>::iterator is = names.begin(); is != names.end(); ++is)
  {
    //jack_port_t* port = jack_port_by_name(_client, *p);
    jack_port_t* port = jack_port_by_name(_client, is->c_str());
    if(!port)
      continue;
*/    
    
    /*
    int nsz = jack_port_name_size();
    char buffer[nsz];
    //strncpy(buffer, *p, nsz);
    strncpy(buffer, is->c_str(), nsz);
    // Ignore the MusE Jack port.
    //if(strncmp(buffer, "MusE", 4) == 0)
    //  continue;
    
    // If there are aliases for this port, use the first one - much better for identifying. 
    //char a1[nsz]; 
    char a2[nsz]; 
    char* aliases[2];
    //aliases[0] = a1;
    aliases[0] = buffer;
    aliases[1] = a2;
    // To disable aliases, just rem this line.
    jack_port_get_aliases(port, aliases);
    //int na = jack_port_get_aliases(port, aliases);
    //char* namep = (na >= 1) ? aliases[0] : buffer;
    char* namep = aliases[0];
    QString qname(namep);
    */
    
/*
    QString qname(is->c_str());
    
    // Port already exists?
    if(MusEGlobal::midiDevices.find(qname))
      continue;
    
    int flags = 0;
    int pf = jack_port_flags(port);
    // If Jack port can send data to us...
    if(pf & JackPortIsOutput)
      // Mark as input capable.
      flags |= 2;
    // If Jack port can receive data from us...
    if(pf & JackPortIsInput)
      // Mark as output capable.
      flags |= 1;
    
    //JackPort jp(0, QString(buffer), flags);
    //portList.append(jp);
    
    if(MusEGlobal::debugMsg)
      printf(" adding port device:%s\n", qname.toLatin1());
    
    MidiJackDevice* dev = new MidiJackDevice(0, qname);
    dev->setrwFlags(flags);
    MusEGlobal::midiDevices.add(dev);
  }
*/
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
            
      if (JACK_DEBUG)
            printf("exitJackAudio() after delete jackAudio\n");
      
      MusEGlobal::audioDevice = NULL;      
      
      }
} // namespace MusECore


