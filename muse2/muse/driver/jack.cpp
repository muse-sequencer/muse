//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: jack.cpp,v 1.30.2.17 2009/12/20 05:00:35 terminator356 Exp $
//  (C) Copyright 2002 Werner Schweer (ws@seh.de)
//  (C) Copyright 2012-2015 Tim E. Real (terminator356 on sourceforge.net)
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
#include <list>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <stdarg.h>
#include <unistd.h>
#include <jack/midiport.h>
#include <string.h>
#include <dlfcn.h>

#include <QString>
#include <QStringList>

#include "libs/strntcpy.h"
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
#include "route.h"

#include "midi.h"
#include "mididev.h"
#include "mpevent.h"

#include "jackmidi.h"
#include "muse_atomic.h"

#define JACK_DEBUG 0 

#define JACK_CALLBACK_FIFO_SIZE 512

// REMOVE Tim. Persistent routes. Added.
#define DEBUG_PRST_ROUTES(dev, format, args...) //fprintf(dev, format, ##args);

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

int jack_ver_maj = 0, jack_ver_min = 0, jack_ver_micro = 0, jack_ver_proto = 0;
muse_atomic_t atomicGraphChangedPending;
bool jack1_port_by_name_workaround = false;
// Function pointers obtained with dlsym:
jack_get_version_type             jack_get_version_fp = NULL;  

//---------------------------------------------------------
//  JackCallbackFifo
//---------------------------------------------------------

class JackCallbackFifo
{
    JackCallbackEvent fifo[JACK_CALLBACK_FIFO_SIZE];
    volatile int size;
    int wIndex;
    int rIndex;

  public:
    JackCallbackFifo()  { clear(); }
    bool put(const JackCallbackEvent& event);   // returns true on fifo overflow
    const JackCallbackEvent& get();
    const JackCallbackEvent& peek(int n = 0);
    void remove();
    bool isEmpty() const { return size == 0; }
    void clear()         { size = 0, wIndex = 0, rIndex = 0; }
    int getSize() const  { return size; }
};

//---------------------------------------------------------
//   JackCallbackFifo
//    put
//    return true on fifo overflow
//---------------------------------------------------------

bool JackCallbackFifo::put(const JackCallbackEvent& event)
      {
      if (size < JACK_CALLBACK_FIFO_SIZE) {
            fifo[wIndex] = event;
            wIndex = (wIndex + 1) % JACK_CALLBACK_FIFO_SIZE;
            ++size;
            return false;
            }
      return true;
      }

//---------------------------------------------------------
//   get
//---------------------------------------------------------

const JackCallbackEvent& JackCallbackFifo::get()
      {
      const JackCallbackEvent& event = fifo[rIndex];
      rIndex = (rIndex + 1) % JACK_CALLBACK_FIFO_SIZE;
      --size;
      return event;
      }

//---------------------------------------------------------
//   peek
//---------------------------------------------------------

const JackCallbackEvent& JackCallbackFifo::peek(int n)
      {
      int idx = (rIndex + n) % JACK_CALLBACK_FIFO_SIZE;
      return fifo[idx];
      }

//---------------------------------------------------------
//   remove
//---------------------------------------------------------

void JackCallbackFifo::remove()
      {
      rIndex = (rIndex + 1) % JACK_CALLBACK_FIFO_SIZE;
      --size;
      }

JackCallbackFifo jackCallbackFifo;

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
            //case JackTransportNetStarting: -- only available in Jack-2!
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
      
      muse_atomic_init(&atomicGraphChangedPending);
      muse_atomic_set(&atomicGraphChangedPending, 0);
      
      jack_get_version_fp = reinterpret_cast<jack_get_version_type>(dlsym(RTLD_DEFAULT, "jack_get_version"));
      DEBUG_PRST_ROUTES(stderr, "initJackAudio jack_get_version() address:%p \n", jack_get_version_fp);
      if(jack_get_version_fp) // ATM Only in Jack-2. Dlsym'd. Check for existence first.
      {
        jack_get_version_fp(&jack_ver_maj, &jack_ver_min, &jack_ver_micro, &jack_ver_proto);
        DEBUG_PRST_ROUTES(stderr, "initJackAudio: jack_ver_maj:%d jack_ver_min:%d jack_ver_micro:%d jack_ver_proto:%d\n", 
                jack_ver_maj, jack_ver_min, jack_ver_micro, jack_ver_proto);
        // FIXME: ATM Jack-2 jack_get_version() returns all zeros. When it is fixed, do something with the values.
        if(jack_ver_maj == 0 && jack_ver_min == 0 && jack_ver_micro == 0 && jack_ver_proto == 0)
          jack_ver_maj = 1;
      }

      if (MusEGlobal::debugMsg) {
            fprintf(stderr,"initJackAudio()\n");
            jack_set_error_function(jackError);
            }
      else
            jack_set_error_function(noJackError);
      MusEGlobal::doSetuid();

      int opts = JackNullOption;
      if(MusEGlobal::noAutoStartJack)
        opts |= JackNoStartServer;
      jack_status_t status;
      jack_client_t* client = jack_client_open("MusE", (jack_options_t)opts, &status);
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
      
      // Check if Jack-1 jack_port_by_name() workaround is required:
      if(jack_ver_maj == 0)
      {
        sleep(1);
        jack_port_t* p = jack_port_register(client, "jack1_test_port", JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0);
        if(p)
        {
          sleep(1);
          int sz = jack_port_name_size();
          char s[sz];
          strcpy(s, jack_get_client_name(client));
          strcat(s, ":jack1_test_port");
          jack_port_t* sp = jack_port_by_name(client, s);
          if(sp)
          {
            if(p != sp)
            {
              fprintf(stderr, "initJackAudio(): Enabling Jack-1 jack_port_by_name() workaround\n");
              jack1_port_by_name_workaround = true;
            }
          }
          else
            fprintf(stderr, "initJackAudio(): Jack-1 jack_port_by_name() workaround: Error on jack_port_by_name(): port not found\n");
            
          if(jack_port_unregister(client, p))
            fprintf(stderr, "initJackAudio(): Jack-1 jack_port_by_name() workaround: Error on jack_port_unregister()\n");
          else
            sleep(1);
        }
        else
          fprintf(stderr, "initJackAudio(): Jack-1 jack_port_by_name() workaround: Error on jack_port_register()\n");
          
      }
      
      jackAudio = new JackAudioDevice(client, jack_get_client_name(client));
      if (MusEGlobal::debugMsg)
            fprintf(stderr, "initJackAudio(): registering client...\n");
      
      MusEGlobal::undoSetuid();
      
      MusEGlobal::audioDevice = jackAudio;

      // WARNING Don't do this here. Do it after any MusE ALSA client is registered, otherwise random crashes can occur.
      //jackAudio->registerClient(); 

      MusEGlobal::sampleRate  = jack_get_sample_rate(client);
      MusEGlobal::segmentSize = jack_get_buffer_size(client);
      
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

static void registration_callback(jack_port_id_t port_id, int is_register, void*)
{
  if(MusEGlobal::debugMsg || JACK_DEBUG)
    printf("JACK: registration_callback\n");

  // REMOVE Tim. Persistent routes. Added.
  DEBUG_PRST_ROUTES(stderr, "JACK: registration_callback: port_id:%d is_register:%d\n", port_id, is_register);

  // In Jack-1 do not use functions like jack_port_by_name and jack_port_by_id here. 
  // With registration the port has not been added yet, so they allocate a new 
  //  'external' port which is NOT the same as the port returned by jack_port_register !
  // Thereafter each call to those functions returns THAT allocated port NOT the jack_port_register one. 
  JackCallbackEvent ev;
  ev.type = is_register ? PortRegister : PortUnregister;
  ev.port_id_A = port_id;

  jackCallbackFifo.put(ev);

  // Jack-1 does not issue a graph order callback after a registration call. 
  // Jack-1 callbacks: [ port connect -> graph order -> registration ] {...}
  // Jack-2 callbacks: [ port connect {...} -> registration {...} -> graph order ] {...} 
  if(jack_ver_maj != 1)
  {
    // Add a GraphChanged event.
    JackCallbackEvent ev;
    ev.type = GraphChanged;
    jackCallbackFifo.put(ev);
    // we cannot call JackAudioDevice::graphChanged() from this
    // context, so we send a message to the gui thread which in turn
    // calls graphChanged()
    if(muse_atomic_read(&atomicGraphChangedPending) == 0)
    {
      muse_atomic_set(&atomicGraphChangedPending, 1);
      MusEGlobal::audio->sendMsgToGui('C');
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
      // REMOVE Tim. Persistent routes. Added.
      DEBUG_PRST_ROUTES(stderr, "JACK: client registration changed:%s register:%d\n", name, isRegister);
      }

//---------------------------------------------------------
//   port_connect_callback
//---------------------------------------------------------

static void port_connect_callback(jack_port_id_t a, jack_port_id_t b, int isConnect, void* arg)
{
  if (MusEGlobal::debugMsg || JACK_DEBUG)
      printf("JACK: port connections changed: A:%d B:%d isConnect:%d\n", a, b, isConnect);
  
    // REMOVE Tim. Persistent routes. Added.
    DEBUG_PRST_ROUTES(stderr, "JACK: port_connect_callback id a:%d id b:%d isConnect:%d\n", a, b, isConnect);

    JackCallbackEvent ev;
    ev.type = isConnect ? PortConnect : PortDisconnect;
    ev.port_id_A = a;
    ev.port_id_B = b;
    JackAudioDevice* jad = (JackAudioDevice*)arg;
    jack_client_t* client = jad->jackClient();
    if(client)
    {
      ev.port_A = jack_port_by_id(client, a);
      ev.port_B = jack_port_by_id(client, b);
    }
    else
    {
      ev.port_A = NULL;
      ev.port_B = NULL;
    }
    
    jackCallbackFifo.put(ev);
}

//---------------------------------------------------------
//   graph_callback
//    this is called from jack when the connections
//    changed
//---------------------------------------------------------

static int graph_callback(void*)
      {
      if (MusEGlobal::debugMsg || JACK_DEBUG)
            printf("graph_callback()\n");
  
      // REMOVE Tim. Persistent routes. Added.
      DEBUG_PRST_ROUTES(stderr, "JACK: graph_callback\n");
      
      // Add a GraphChanged event.
      JackCallbackEvent ev;
      ev.type = GraphChanged;
      jackCallbackFifo.put(ev);
      
      // we cannot call JackAudioDevice::graphChanged() from this
      // context, so we send a message to the gui thread which in turn
      // calls graphChanged()
      if(muse_atomic_read(&atomicGraphChangedPending) == 0)
      {
        muse_atomic_set(&atomicGraphChangedPending, 1);
        MusEGlobal::audio->sendMsgToGui('C');
      }
      return 0;
      }

void JackAudioDevice::processJackCallbackEvents(const Route& our_node, jack_port_t* our_port, 
                                                RouteList* route_list, bool is_input)
{
  jack_client_t* client = jackClient();
  if(!client)
    return;
  
  jack_port_t* our_ext_port = our_port;
  const char* our_port_name = jack_port_name(our_port);

  if(jack1_port_by_name_workaround)
  {
    jack_port_t* jp = jack_port_by_name(_client, our_port_name);
    if(jp && jp != our_port)
    {
      // REMOVE Tim. Persistent routes. Added.
      DEBUG_PRST_ROUTES(stderr, "JackAudioDevice::processJackCallbackEvents: changing audio input port!: channel:%d our_port:%p new port:%p\n", 
              our_node.channel, our_port, jp);
      our_ext_port = jp;
    }
  }
  
  for(iRoute ir = route_list->begin(); ir != route_list->end(); ++ir)
  {
    // Check correct route type, and channel if required.
    if((ir->type != Route::JACK_ROUTE) || (our_node.channel != -1 && ir->channel != our_node.channel))
      continue;
    const char* route_jpname = ir->persistentJackPortName;
    // Caution: In Jack-1, under some conditions jack_port_by_name() might ALLOCATE a new port for use later if not found! 
    // FIXME: TODO: Maybe switch to get_ports
    jack_port_t* jp = jack_port_by_name(client, route_jpname);
    if(jp)
    {
      // TODO: For Jack-2 maybe alter this? Before calling jack_port_connected_to(), maybe first check if the IDs 
      //        (hence jack ports) passed in the connect callback match here, to avoid calling jack_port_connected_to() ?
      if(jack_port_connected_to(our_port, route_jpname)) 
      {
        // The ports are connected. Keep the route node but update its jack port pointer if necessary.
        const char* s = NULL;
        if(jp != ir->jackPort)
        {
          // REMOVE Tim. Persistent routes. Added.
          DEBUG_PRST_ROUTES(stderr, "processJackCallbackEvents: Ports connected. Modifying route: our_port:%p old_route_jp:%p new_route_jp:%p route_persistent_name:%s\n", 
                  our_port, ir->jackPort, jp, route_jpname);
          s = route_jpname;
        }
        // Find a more appropriate name if necessary.
        char fin_name[ROUTE_PERSISTENT_NAME_SIZE];
        portName(jp, fin_name, ROUTE_PERSISTENT_NAME_SIZE);
        if(strcmp(ir->persistentJackPortName, fin_name) != 0)
        {
          // REMOVE Tim. Persistent routes. Added.
          DEBUG_PRST_ROUTES(stderr, "processJackCallbackEvents: Ports connected. Modifying route name: route_persistent_name:%s new name:%s\n", route_jpname, fin_name);
          s = fin_name;
        }
        
        if(s)
          operations.add(PendingOperationItem(Route(Route::JACK_ROUTE, 0, jp, ir->channel, 0, 0, s), &(*ir), PendingOperationItem::ModifyRouteNode));
      }
      else 
      {
        if(ir->jackPort)
        {
          // Check whether the disconnect happened BEFORE this graphChanged() was called, 
          //  or just now during it, or was followed by an unregister. 
          int ret = checkDisconnectCallback(our_ext_port, jp);
          if(ret == 2)
          {
            // REMOVE Tim. Persistent routes. Added.
            DEBUG_PRST_ROUTES(stderr, "processJackCallbackEvents: Ports not connected, ret=DeleteRouteNode. Deleting route: our_port:%p route_jp:%p found_jp:%p route_persistent_name:%s\n", 
                      our_port, ir->jackPort, jp, route_jpname);
            // The port exists but is not connected to our port. Remove the route node.
            operations.add(PendingOperationItem(route_list, ir, PendingOperationItem::DeleteRouteNode));
          }
          else
          if(ret == 1)
          {
            // REMOVE Tim. Persistent routes. Added.
            DEBUG_PRST_ROUTES(stderr, "processJackCallbackEvents: Ports not connected, ret=ModifyRouteNode. Modifying route: our_port:%p route_jp:%p found_jp:%p route_persistent_name:%s\n", 
                      our_port, ir->jackPort, jp, route_jpname);
            operations.add(PendingOperationItem(Route(Route::JACK_ROUTE, 0, NULL, ir->channel, 0, 0, ir->persistentJackPortName), &(*ir), PendingOperationItem::ModifyRouteNode));
          }
        }
        else 
        if(MusEGlobal::audio && MusEGlobal::audio->isRunning()) // Don't try to connect if not running.
        {
          // Check whether a port registration happened BEFORE this graphChanged() was called, 
          //  or just now during it, or was followed by an unregister. 
          int ret = checkPortRegisterCallback(jp);
          if(ret == 1)
          {  
            // No failed attempts at connection to the Jack port now or in the previous call to graphChanged().
            if(our_port)
            {
              // The port exists but is not connected to our port. Reconnect the route. 
              // NOTE: Jack2: graph changed callback will be called again regardless if jack_connect succeeds or fails...
              // REMOVE Tim. Persistent routes. Added.
              DEBUG_PRST_ROUTES(stderr, "processJackCallbackEvents: Ports not connected. Reconnecting route: our_port:%p route_jp:%p found_jp:%p our_port_name:%s route_persistent_name:%s\n", 
                        our_port, ir->jackPort, jp, our_port_name, route_jpname);
              if(our_port_name)
              {
                int err;
                if(is_input)
                  err = jack_connect(client, route_jpname, our_port_name);
                else
                  err = jack_connect(client, our_port_name, route_jpname);
                if(err)
                {
                  // REMOVE Tim. Persistent routes. Added.
                  DEBUG_PRST_ROUTES(stderr, "processJackCallbackEvents: Ports not connected. Reconnecting route: ERROR:%d our_port:%p route_jp:%p found_jp:%p our_port_name:%s route_persistent_name:%s\n", 
                          err, our_port, ir->jackPort, jp, our_port_name, route_jpname);
                }
                else
                {
                  // We have our jack port, on a supposedly active client. Update the route node's jack port pointer.
                  // Find a more appropriate name if necessary.
                  const char* s = ir->persistentJackPortName;
                  char fin_name[ROUTE_PERSISTENT_NAME_SIZE];
                  portName(jp, fin_name, ROUTE_PERSISTENT_NAME_SIZE);
                  if(strcmp(ir->persistentJackPortName, fin_name) != 0)
                  {
                    // REMOVE Tim. Persistent routes. Added.
                    DEBUG_PRST_ROUTES(stderr, "processJackCallbackEvents: Ports connected. Modifying route name: route_persistent_name:%s new name:%s\n", route_jpname, fin_name);
                    s = fin_name;
                  }
                  operations.add(PendingOperationItem(Route(Route::JACK_ROUTE, 0, jp, ir->channel, 0, 0, s), &(*ir), PendingOperationItem::ModifyRouteNode));
                }
              }
            }
          }
          // State unchanged.
        }
      }
    }
    else
    {
      // Port does not exist. Keep the route node but clear its jack port pointer if necessary.
      if(ir->jackPort)
      {
        // REMOVE Tim. Persistent routes. Added.
        DEBUG_PRST_ROUTES(stderr, "processJackCallbackEvents: Port non-existent. Modifying route: our_port:%p route_jp:%p route_persistent_name:%s\n", 
                our_port, ir->jackPort, route_jpname);
        operations.add(PendingOperationItem(Route(Route::JACK_ROUTE, 0, NULL, ir->channel, 0, 0, ir->persistentJackPortName), &(*ir), PendingOperationItem::ModifyRouteNode));
      }
    }
  }

  checkNewRouteConnections(our_port, our_node.channel, route_list);
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
  // REMOVE Tim. Persistent routes. Added.
  DEBUG_PRST_ROUTES(stderr, "JackAudioDevice::graphChanged()\n"); 

  if(!checkJackClient(_client))
  {
    jackCallbackFifo.clear(); // Want this?
    // Reset this now.
    muse_atomic_set(&atomicGraphChangedPending, 0);
    return;
  }

  // For Jack-1 only: See if we need to wait, for example for a port unregister event.
  // Jack "2" does not require waiting, Jack "1" does, assume any other version requires the wait (no harm).
  if(MusEGlobal::audio && jack_ver_maj != 1) 
  {
    // TODO: It may be desirable to always wait so that bunches of commands can be processed easier.
    //bool do_audio_wait = false;
    // This is safe because the writer only increases the size.
    int cb_fifo_sz = jackCallbackFifo.getSize();
    for(int i = 0; i < cb_fifo_sz; ++i)
    {
      const JackCallbackEvent& jcb = jackCallbackFifo.peek(i);
      if(jcb.type == PortDisconnect && (jack_port_is_mine(_client, jcb.port_A) || jack_port_is_mine(_client, jcb.port_B)))
      {
        // REMOVE Tim. Persistent routes. Added.
        DEBUG_PRST_ROUTES(stderr, "JackAudioDevice::graphChanged: *** calling msgAudioWait()\n");
        MusEGlobal::audio->msgAudioWait(); // Wait until upcoming process call has finished...
        break;
      }
    } 
  }

  // Reset this now.
  muse_atomic_set(&atomicGraphChangedPending, 0);
  
  jackCallbackEvents.clear();
  // Find the last GraphChanged event, if any.
  // This is safe because the writer only increases the size.
  int cb_fifo_sz = jackCallbackFifo.getSize();
  if(cb_fifo_sz)
  {
    int last_gc_idx = cb_fifo_sz - 1;
    if(jack_ver_maj == 1)
      for(int i = 0; i < cb_fifo_sz; ++i)
        if(jackCallbackFifo.peek(i).type == GraphChanged)
          last_gc_idx = i;
    // Move the events into a list for processing, including the final GraphChanged event.
    // Leave any 'still in progress' ending events (without closing GraphChanged event) in the ring buffer.
    //jackCallbackEvents.clear();
    for(int i = 0; i <= last_gc_idx; ++i)
      jackCallbackEvents.push_back(jackCallbackFifo.get());
  }
  processGraphChanges();
  
  if(!operations.empty())
  {
    MusEGlobal::audio->msgExecutePendingOperations(operations);
    operations.clear();
  }
}

void JackAudioDevice::processGraphChanges()
{
  // REMOVE Tim. Persistent routes. Added.
  DEBUG_PRST_ROUTES(stderr, "JackAudioDevice::processGraphChanges()\n");
  //---------------------------------------
  // Audio inputs:
  //---------------------------------------
  
  InputList* il = MusEGlobal::song->inputs();
  for(iAudioInput ii = il->begin(); ii != il->end(); ++ii) 
  {
    AudioInput* it = *ii;
    int channels = it->channels();
    for(int channel = 0; channel < channels; ++channel) 
    {
      jack_port_t* port = (jack_port_t*)(it->jackPort(channel));
      if(port == 0)
        continue;
      processJackCallbackEvents(Route(it, channel), port, it->inRoutes(), true);
    }
  }
        
  //---------------------------------------
  // Audio outputs:
  //---------------------------------------
  
  OutputList* ol = MusEGlobal::song->outputs();
  for(iAudioOutput ii = ol->begin(); ii != ol->end(); ++ii) 
  {
    AudioOutput* it = *ii;
    int channels = it->channels();
    for(int channel = 0; channel < channels; ++channel) 
    {
      jack_port_t* port = (jack_port_t*)(it->jackPort(channel));
      if(port == 0)
        continue;
      processJackCallbackEvents(Route(it, channel), port, it->outRoutes(), false);
    }
  }
        
  //---------------------------------------
  // Midi devices:
  //---------------------------------------
  
  for(iMidiDevice ii = MusEGlobal::midiDevices.begin(); ii != MusEGlobal::midiDevices.end(); ++ii) 
  {
    MidiDevice* md = *ii;
    if(md->deviceType() != MidiDevice::JACK_MIDI)
      continue;
    
    //---------------------------------------
    // Midi outputs:
    //---------------------------------------
    
    if(md->rwFlags() & 1) // Writable
    {
      jack_port_t* port = (jack_port_t*)md->outClientPort();
      if(port != 0)
        processJackCallbackEvents(Route(md, -1), port, md->outRoutes(), false);
    }  
          
    //------------------------
    // Midi inputs:
    //------------------------
    
    if(md->rwFlags() & 2) // Readable
    {
      jack_port_t* port = (jack_port_t*)md->inClientPort();
      if(port != 0)
        processJackCallbackEvents(Route(md, -1), port, md->inRoutes(), true);
    }  
  }
}

void JackAudioDevice::checkNewRouteConnections(jack_port_t* our_port, int channel, RouteList* route_list)
{
  // REMOVE Tim. Persistent routes. Added.
  DEBUG_PRST_ROUTES(stderr, "JackAudioDevice::checkNewRouteConnections(): our_port:%p channel:%d route_list:%p\n", 
          our_port, channel, route_list);
  // Check for new connections...
  const char** ports = jack_port_get_all_connections(_client, our_port);
  if(ports) 
  {
    const char** pn = ports;
    while(*pn) 
    {
      // Caution: In Jack-1, under some conditions jack_port_by_name() might ALLOCATE a new port for use later if not found! 
      // Should be safe and quick search here, we know that the port name is valid.
      jack_port_t* jp = jack_port_by_name(_client, *pn);
      if(jp)
      {
        bool found = false;
        for(ciRoute ir = route_list->begin(); ir != route_list->end(); ++ir) 
        {
          if(ir->type != Route::JACK_ROUTE || (channel != -1 && ir->channel != channel))
            continue;
          
          // See if any changes are pending for the route node and take them into account.
          jack_port_t* op_jp = (jack_port_t*)ir->jackPort;
          const char* op_ppname = ir->persistentJackPortName;
          iPendingOperation ipo = operations.end();
          while(ipo != operations.begin())
          {
            --ipo;
            switch(ipo->_type)
            {
              case PendingOperationItem::DeleteRouteNode:
                if(ipo->_route_list == route_list && &(*ipo->_iRoute) == &(*ir))
                {
                  found = true;
                  ipo = operations.begin();  // Breakout
                }
              break;

              case PendingOperationItem::ModifyRouteNode:
                if(ipo->_dst_route_pointer == &(*ir))
                {
                  op_jp = (jack_port_t*)ipo->_src_route.jackPort;
                  op_ppname = ipo->_src_route.persistentJackPortName;
                  ipo = operations.begin();  // Breakout
                }
              break;
              
              default:
              break;
            }
          }
          if(found)
          {
            found = false;
            continue; // Ignore the route node - it has been scheduled for deletion.
          }
          
          // Caution: In Jack-1, under some conditions jack_port_by_name() might ALLOCATE a new port for use later if not found! 
          // TODO: Maybe switch to get_ports
          if(op_jp == jp || jack_port_by_name(_client, op_ppname) == jp)
          {
            found = true;
            break;
          }
        }
        if(!found) 
        {
          Route r(Route::JACK_ROUTE, 0, jp, channel, 0, 0, NULL);
          // Find a better name.
          portName(jp, r.persistentJackPortName, ROUTE_PERSISTENT_NAME_SIZE);
          // REMOVE Tim. Persistent routes. Added.
          DEBUG_PRST_ROUTES(stderr, " adding route: route_jp:%p portname:%s route_persistent_name:%s\n", 
                  jp, *pn, r.persistentJackPortName);
          operations.add(PendingOperationItem(route_list, r, PendingOperationItem::AddRouteNode));
        }
      }
      ++pn;
    }
    jack_free(ports);
  }
}

int JackAudioDevice::checkDisconnectCallback(const jack_port_t* our_port, const jack_port_t* port)
{
  // REMOVE Tim. Persistent routes. Added.
  DEBUG_PRST_ROUTES(stderr, "JackAudioDevice::checkDisconnectCallback(): our_port:%p port:%p\n", our_port, port); 
  
  iJackCallbackEvent ijce = jackCallbackEvents.end();
  while(ijce != jackCallbackEvents.begin())
  {
    --ijce;
    if(ijce->type == PortConnect && ((ijce->port_A == our_port && ijce->port_B == port) || (ijce->port_B == our_port && ijce->port_A == port)))
      return 0;
    if(ijce->type == PortDisconnect)
    {
      jack_port_id_t id;
      if(ijce->port_A == our_port && ijce->port_B == port)
        id = ijce->port_id_B;
      else if(ijce->port_B == our_port && ijce->port_A == port)
        id = ijce->port_id_A;
      else continue;
      
      for( ++ijce ; ijce != jackCallbackEvents.end(); ++ijce)
        if(ijce->type == PortUnregister && ijce->port_id_A == id)
          return 1;
      return 2;
    }
  }
  return 0;
}

int JackAudioDevice::checkPortRegisterCallback(const jack_port_t* port)
{
  // REMOVE Tim. Persistent routes. Added.
  DEBUG_PRST_ROUTES(stderr, "JackAudioDevice::checkPortRegisterCallback(): port:%p\n", port); 
      
  iJackCallbackEvent ijce = jackCallbackEvents.end();
  while(ijce != jackCallbackEvents.begin())
  {
    --ijce;
    if(ijce->type == PortRegister)
    {
      jack_port_id_t id = ijce->port_id_A;
      // Caution: In Jack-1, under some conditions jack_port_by_id() might ALLOCATE a new port for use later if not found! 
      if(jack_port_by_id(_client, id) == port)
      {
        for( ++ijce ; ijce != jackCallbackEvents.end(); ++ijce)
          if(ijce->type == PortUnregister && ijce->port_id_A == id)
            return 0;
        return 1;
      }
    }
  }
  return 0;
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
      jack_set_port_registration_callback(_client, registration_callback, this);
      jack_set_client_registration_callback(_client, client_registration_callback, 0);
      jack_set_port_connect_callback(_client, port_connect_callback, this);
      // Can't use this! Incompatible in jack 1/2 and unimplemented in jack1.
      //jack_set_port_rename_callback(_client, port_rename_callback, 0);
      jack_set_graph_order_callback(_client, graph_callback, this);
//      jack_set_xrun_callback(client, xrun_callback, 0);
      jack_set_freewheel_callback (_client, freewheel_callback, 0);
      }

//---------------------------------------------------------
//   registerInPort
//---------------------------------------------------------

void* JackAudioDevice::registerInPort(const char* name, bool midi)
      {
      if(JACK_DEBUG)
        printf("registerInPort()\n");
      if(!checkJackClient(_client) || !name || name[0] == '\0') 
        return NULL;
      const char* type = midi ? JACK_DEFAULT_MIDI_TYPE : JACK_DEFAULT_AUDIO_TYPE;
      void* p = jack_port_register(_client, name, type, JackPortIsInput, 0);
      // REMOVE Tim. Persistent routes. Added.
      DEBUG_PRST_ROUTES(stderr, "JACK: registerInPort: <%s> %p\n", name, p);
      return p;
      }

//---------------------------------------------------------
//   registerOutPort
//---------------------------------------------------------

void* JackAudioDevice::registerOutPort(const char* name, bool midi)
      {
      if(JACK_DEBUG)
        printf("registerOutPort()\n");
      if(!checkJackClient(_client) || !name || name[0] == '\0') 
        return NULL;
      const char* type = midi ? JACK_DEFAULT_MIDI_TYPE : JACK_DEFAULT_AUDIO_TYPE;
      void* p = jack_port_register(_client, name, type, JackPortIsOutput, 0);
      // REMOVE Tim. Persistent routes. Added.
      DEBUG_PRST_ROUTES(stderr, "JACK: registerOutPort: <%s> %p\n", name, p);
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

void JackAudioDevice::connect(const char* src, const char* dst)
{
  if(JACK_DEBUG)
    printf("JackAudioDevice::connect()\n");
  if(!checkJackClient(_client) || !src || !dst || src[0] == '\0' || dst[0] == '\0') 
    return;
  int err = jack_connect(_client, src, dst);
  if(err) 
    fprintf(stderr, "jack connect <%s> - <%s> failed with err:%d\n", src, dst, err);
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

void JackAudioDevice::disconnect(const char* src, const char* dst)
{
  if(JACK_DEBUG)
    printf("JackAudioDevice::disconnect()\n");
  if(!checkJackClient(_client) || !src || !dst || src[0] == '\0' || dst[0] == '\0') 
    return;
  int err = jack_disconnect(_client, src, dst);
  if(err) 
    fprintf(stderr, "jack disconnect <%s> - <%s> failed with err:%d\n", src, dst, err);
}

//---------------------------------------------------------
//   portsConnected
//---------------------------------------------------------

bool JackAudioDevice::portsConnected(const char* port1, const char* port2)
{
  if(!_client)
    return false;
  jack_port_t* jp1 = jack_port_by_name(_client, port1);
  if(!jp1)
    return false;
  jack_port_t* jp2 = jack_port_by_name(_client, port2);
  if(!jp2)
    return false;
  
  const char** ports = jack_port_get_all_connections(_client, jp1);
  if(!ports)
    return false;

  bool rv = false;  
  for(const char** p = ports; p && *p; ++p) 
  {
    jack_port_t* jp = jack_port_by_name(_client, *p);
    if(jp == jp2)
    {
      rv = true;
      break;
    }
  }
  
  jack_free(ports);
  return rv;
}

//---------------------------------------------------------
//   portsCanConnect
//---------------------------------------------------------

bool JackAudioDevice::portsCanConnect(const char* port1, const char* port2)
{ 
  if(!_client)
    return false;
  jack_port_t* jp1 = jack_port_by_name(_client, port1);
  if(!jp1)
    return false;
  jack_port_t* jp2 = jack_port_by_name(_client, port2);
  if(!jp2)
    return false;
  
  const char** ports = jack_port_get_all_connections(_client, jp1);
  if(!ports)
    return true;

  bool rv = true;  
  for(const char** p = ports; p && *p; ++p) 
  {
    jack_port_t* jp = jack_port_by_name(_client, *p);
    if(jp == jp2)
    {
      rv = false;
      break;
    }
  }
  
  jack_free(ports);
  return rv;
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

      // REMOVE Tim. Persistent routes. Added.
      DEBUG_PRST_ROUTES (stderr, "JackAudioDevice::start(): calling jack_activate()\n");

      if (jack_activate(_client)) {
            MusEGlobal::undoSetuid();   
            fprintf (stderr, "JACK: cannot activate client\n");
            exit(-1);
            }
            
      MusEGlobal::undoSetuid();
      
      /* connect the ports. Note: you can't do this before
         the client is activated, because we can't allow
         connections to be made to clients that aren't
         running.
       */
      MusEGlobal::song->connectAllPorts();

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
      // REMOVE Tim. Persistent routes. Added.
      DEBUG_PRST_ROUTES (stderr, "JackAudioDevice::stop(): calling jack_deactivate()\n");
      
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
      QString qname;
      QString cname(jack_get_client_name(_client));
      
      for (const char** p = ports; p && *p; ++p) {
            // Caution: In Jack-1, under some conditions jack_port_by_name() might ALLOCATE a new port for use later if not found! 
            // Should be safe and quick search here, we know that the port name is valid.
            jack_port_t* port = jack_port_by_name(_client, *p);
            int port_flags = jack_port_flags(port);

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
            
            name_list.push_back(qname);
            }
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
//   Returns name of port and sets success.
//   This method consults a blacklist of client names, 
//    such as "system:", whether to pick the name or 
//    one of the aliases, whichever does NOT contain 
//    the blacklist names.
//   preferred_name_or_alias: -1: No preference 0: Prefer canonical name 1: Prefer 1st alias 2: Prefer 2nd alias.
//---------------------------------------------------------

char* JackAudioDevice::portName(void* port, char* str, int str_size, int preferred_name_or_alias)
{
  bool A = false, B = false, C = false;
  const char* p_name = jack_port_name((jack_port_t*)port);
  if(p_name && p_name[0] != '\0')
  {
    // TODO: Make this a user editable blacklist of client names!
    if((strncmp(p_name, "system:", 7) != 0 && preferred_name_or_alias == -1) || preferred_name_or_alias == 0)
      return MusELib::strntcpy(str, p_name, str_size);
    A = true;
  }
  
  int nsz = jack_port_name_size();
  char a1[nsz];
  char a2[nsz];
  char* al[2];
  al[0] = &a1[0];
  al[1] = &a2[0];
  
  int na = jack_port_get_aliases((jack_port_t*)port, al);
  if(na >= 1 && al[0] != '\0')
  {
    if((strncmp(al[0], "system:", 7) != 0 && preferred_name_or_alias == -1) || preferred_name_or_alias == 1)
      return MusELib::strntcpy(str, al[0], str_size);
    B = true;
  }

  if(na >= 2 && al[1] != '\0')
  {
    if((strncmp(al[1], "system:", 7) != 0 && preferred_name_or_alias == -1) || preferred_name_or_alias == 2)
      return MusELib::strntcpy(str, al[1], str_size);
    C = true;
  }

  if(A)
    return MusELib::strntcpy(str, p_name, str_size);
  if(B)
    return MusELib::strntcpy(str, al[0], str_size);
  if(C)
    return MusELib::strntcpy(str, al[1], str_size);
  
  return MusELib::strntcpy(str, p_name, str_size); // strntcpy accepts NULL source
  
}

//---------------------------------------------------------
//   portLatency
//   If capture is true get the capture latency,
//    otherwise get the playback latency.
//---------------------------------------------------------

unsigned int JackAudioDevice::portLatency(void* port, bool capture) const
{
  // TODO: Experimental code... Finish it...
  
  if(!checkJackClient(_client) || !port)
    return 0;

  //QString s(jack_port_name((jack_port_t*)port));
  //fprintf(stderr, "Jack::portName %p %s\n", port, s.toLatin1().constData());  
  
  jack_latency_range_t p_range;
  jack_port_get_latency_range((jack_port_t*)port, JackPlaybackLatency, &p_range);
  //fprintf(stderr, "JackAudioDevice::portLatency playback min:%u max:%u\n", p_range.min, p_range.max);  
  if(p_range.max != p_range.min)
  {
    //fprintf(stderr, "JackAudioDevice::portLatency min:%u != max:%u\n", p_range.min, p_range.max);  
    //return (p_range.max - p_range.min) / 2;
  }
  
  jack_latency_range_t c_range;
  jack_port_get_latency_range((jack_port_t*)port, JackCaptureLatency, &c_range);
  //fprintf(stderr, "JackAudioDevice::portLatency capture min:%u max:%u\n", c_range.min, c_range.max);  
  if(c_range.max != c_range.min)
  {
    //fprintf(stderr, "JackAudioDevice::portLatency capture min:%u != max:%u\n", c_range.min, c_range.max);  

    // TODO TEST Decide which method to use. Although, it is arbitrary.
    //return (c_range.max - c_range.min) / 2;  
    //return c_range.max;
  }

  if(capture)
    //return (c_range.max - c_range.min) / 2;
    return c_range.max;
  
  //return (p_range.max - p_range.min) / 2;
  return p_range.max;
}

//---------------------------------------------------------
//   unregisterPort
//---------------------------------------------------------

void JackAudioDevice::unregisterPort(void* p)
      {
      if(JACK_DEBUG)
        printf("JackAudioDevice::unregisterPort(%p)\n", p);
      if(!checkJackClient(_client) || !p) 
        return;
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
            //case JackTransportNetStarting: -- only available in Jack-2!
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
            printf("JackAudioDevice::findPort(%s)\n", name);
      if(!checkJackClient(_client) || !name || name[0] == '\0') 
        return NULL;
      // Caution: In Jack-1, under some conditions jack_port_by_name() might ALLOCATE a new port for use later if not found! 
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
      muse_atomic_destroy(&atomicGraphChangedPending);
      }
    
} // namespace MusECore


