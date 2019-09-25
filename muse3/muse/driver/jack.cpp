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
#include <sys/time.h>

#include <QString>
#include <QStringList>

#include <jack/thread.h>

#include "strntcpy.h"
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

#include "al/al.h"

#define JACK_DEBUG 0 

#define JACK_CALLBACK_FIFO_SIZE 512

// For debugging output: Uncomment the fprintf section.
#define DEBUG_PRST_ROUTES(dev, format, args...) // fprintf(dev, format, ##args);

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
              fprintf(stderr, "Muse:checkAudioDevice: no audioDevice\n");
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
jack_port_set_name_type           jack_port_set_name_fp = NULL;
jack_port_rename_type             jack_port_rename_fp = NULL;

// REMOVE Tim. latency. Added. TESTING.
// Jack BUG ? :
// Before latency compensation was added, we would deactivate and reactivate the Jack server
//  when loading a song. But tests revealed Jack does not like that, and ALL reported
//  port latencies are ZERO after reactivation. So we have no choice but to leave the
//  Jack server running all the time until program close.
bool jackStarted = false;

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
            fprintf(stderr, "Panic! no _client!\n");
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
            fprintf(stderr, "jack_thread_init()\n");
      MusEGlobal::doSetuid();
#ifdef VST_SUPPORT
      if (loadVST)
            fst_adopt_thread();
#endif
      MusEGlobal::undoSetuid();
      }

int JackAudioDevice::processAudio(jack_nframes_t frames, void*)
{
      jackAudio->_frameCounter += frames;
      MusEGlobal::segmentSize = frames;

      if (MusEGlobal::audio->isRunning())
      {
        // Are we using Jack transport?
        if(MusEGlobal::useJackTransport.value())
        {
          // Just call the audio process normally. Jack transport will take care of itself.
          // Don't process while we're syncing. ToDO: May need to deliver silence in process!
          //if(jackAudio->getState() != Audio::START_PLAY)
            MusEGlobal::audio->process((unsigned long)frames);
        }
        else
        {
          // Not using Jack transport. Use our built-in transport, which INCLUDES
          //  the necessary calls to Audio::sync() and ultimately Audio::process(),
          //  and increments the built-in play position.
          jackAudio->processTransport((unsigned long)frames);
        }
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
        fprintf(stderr, "processSync frame:%u\n", pos->frame);
      
        if(pos->valid & JackPositionBBT)
        {
          if(JACK_DEBUG)
          {
            fprintf(stderr, "processSync BBT:\n bar:%d beat:%d tick:%d\n bar_start_tick:%f beats_per_bar:%f beat_type:%f ticks_per_beat:%f beats_per_minute:%f\n",
                    pos->bar, pos->beat, pos->tick, pos->bar_start_tick, pos->beats_per_bar, pos->beat_type, pos->ticks_per_beat, pos->beats_per_minute);
            if(pos->valid & JackBBTFrameOffset)
              fprintf(stderr, "processSync BBTFrameOffset: %u\n", pos->bbt_offset);
          }
        }
      }
        
      if(!MusEGlobal::useJackTransport.value())
        return 1;
        
      int audioState = Audio::STOP;
      switch (int(state)) {
            case JackTransportStopped:   
              audioState = Audio::STOP;
            break;  
            case JackTransportLooping:
            case JackTransportRolling:   
              audioState = Audio::PLAY;
            break;  
            case JackTransportStarting:  
              //fprintf(stderr, "processSync JackTransportStarting\n");
              
              audioState = Audio::START_PLAY;
            break;  
            //case JackTransportNetStarting: -- only available in Jack-2!
            // FIXME: Quick and dirty hack to support both Jack-1 and Jack-2
            // Really need a config check of version...
            case 4:  
              //fprintf(stderr, "processSync JackTransportNetStarting\n");
              
              audioState = Audio::START_PLAY;
            break;  
            }
            
      unsigned frame = pos->frame;
      //return MusEGlobal::audio->sync(audioState, frame);
      int rv = MusEGlobal::audio->sync(audioState, frame);
      //fprintf(stderr, "Jack processSync() after MusEGlobal::audio->sync frame:%d\n", frame);
      return rv;      
      }

//---------------------------------------------------------
//   timebase_callback
//---------------------------------------------------------

static void timebase_callback(jack_transport_state_t /* state */,
   jack_nframes_t nframes,
   jack_position_t* pos,
   int new_pos,
   void* arg)
  {
    // REMOVE Tim. clip. Added.
    JackAudioDevice* jad = (JackAudioDevice*)arg;
    if(jad)
      jad->timebaseAck();

    if (JACK_DEBUG)
    {
      if(pos->valid & JackPositionBBT)
        fprintf(stderr, "timebase_callback BBT:\n bar:%d beat:%d tick:%d\n bar_start_tick:%f beats_per_bar:%f beat_type:%f ticks_per_beat:%f beats_per_minute:%f\n",
                pos->bar, pos->beat, pos->tick, pos->bar_start_tick, pos->beats_per_bar, pos->beat_type, pos->ticks_per_beat, pos->beats_per_minute);
      if(pos->valid & JackBBTFrameOffset)
        fprintf(stderr, "timebase_callback BBTFrameOffset: %u\n", pos->bbt_offset);
      if(pos->valid & JackPositionTimecode)
        fprintf(stderr, "timebase_callback JackPositionTimecode: frame_time:%f next_time:%f\n", pos->frame_time, pos->next_time);
      if(pos->valid & JackAudioVideoRatio)
        fprintf(stderr, "timebase_callback JackAudioVideoRatio: %f\n", pos->audio_frames_per_video_frame);
      if(pos->valid & JackVideoFrameOffset)
        fprintf(stderr, "timebase_callback JackVideoFrameOffset: %u\n", pos->video_offset);
    }
    
    //Pos p(pos->frame, false);
      Pos p(MusEGlobal::extSyncFlag.value() ? MusEGlobal::audio->tickPos() : pos->frame, MusEGlobal::extSyncFlag.value() ? true : false);
      // Can't use song pos - it is only updated every (slow) GUI heartbeat !
      //Pos p(MusEGlobal::extSyncFlag.value() ? MusEGlobal::song->cpos() : pos->frame, MusEGlobal::extSyncFlag.value() ? true : false);
      
      pos->valid = JackPositionBBT;
      int bar, beat, tick;
      p.mbt(&bar, &beat, &tick);
      pos->bar = bar;
      pos->beat = beat;
      pos->tick = tick;

      pos->bar_start_tick = Pos(pos->bar, 0, 0).tick();
      pos->bar++;
      pos->beat++;
      
      int z, n;
      MusEGlobal::sigmap.timesig(p.tick(), z, n);
      pos->beats_per_bar = z;
      pos->beat_type = n;
      pos->ticks_per_beat = MusEGlobal::config.division;
      //pos->ticks_per_beat = 24;
      
      double tempo = MusEGlobal::tempomap.tempo(p.tick());
      pos->beats_per_minute = ((double)MusEGlobal::tempomap.globalTempo() * 600000.0) / tempo;
      if (JACK_DEBUG)
      {
        fprintf(stderr, "timebase_callback is new_pos:%d nframes:%u frame:%u tickPos:%d cpos:%d\n", new_pos, nframes, pos->frame, MusEGlobal::audio->tickPos(), MusEGlobal::song->cpos());
        fprintf(stderr, " new: bar:%d beat:%d tick:%d\n bar_start_tick:%f beats_per_bar:%f beat_type:%f ticks_per_beat:%f beats_per_minute:%f\n",
               pos->bar, pos->beat, pos->tick, pos->bar_start_tick, pos->beats_per_bar, pos->beat_type, pos->ticks_per_beat, pos->beats_per_minute);
      }
      
      }

//---------------------------------------------------------
//   processShutdown
//---------------------------------------------------------

static void processShutdown(void*)
      {
      if (JACK_DEBUG)
          fprintf(stderr, "processShutdown()\n");
      //fprintf(stderr, "processShutdown\n");
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
      fprintf(stderr,"JACK ERROR: %s\n", s);
      }

//---------------------------------------------------------
//   noJackError
//---------------------------------------------------------

static void noJackError(const char* /* s */)
      {
            //fprintf(stderr, "noJackError()\n");
      }
      
//---------------------------------------------------------
//   jackInfo
//---------------------------------------------------------

static void jackInfo(const char* s)
{
  fprintf(stderr, "JACK INFO: %s\n", s);
}
      
//---------------------------------------------------------
//   noJackInfo
//---------------------------------------------------------

static void noJackInfo(const char* /*s*/)
{
  //fprintf(stderr, "noJackInfo()\n");
}
      
//---------------------------------------------------------
//   JackAudioDevice
//---------------------------------------------------------

JackAudioDevice::JackAudioDevice(jack_client_t* cl, char* name)
   : AudioDevice()
{
      _frameCounter = 0;
      strcpy(jackRegisteredName, name);
      _client = cl;
      _timebaseAck = false;
}

//---------------------------------------------------------
//   ~JackAudioDevice
//---------------------------------------------------------

JackAudioDevice::~JackAudioDevice()
      {
      if (JACK_DEBUG)
            fprintf(stderr, "~JackAudioDevice()\n");
      if (_client) {
            if (jack_client_close(_client)) {
                  fprintf(stderr,"jack_client_close() failed: %s\n", strerror(errno));
                  }
            }
      if (JACK_DEBUG)
            fprintf(stderr, "~JackAudioDevice() after jack_client_close()\n");
      }

//---------------------------------------------------------
//   realtimePriority
//      return zero if not running realtime
//      can only be called if JACK client thread is already
//      running
//---------------------------------------------------------

int JackAudioDevice::realtimePriority() const
{
   if(!_client)
      return 0;

   pthread_t t = (pthread_t) jack_client_thread_id(_client);
   if(t == 0)
      return jack_client_real_time_priority(_client);

   int policy;
   struct sched_param param;
   memset(&param, 0, sizeof(param));
   int rv = pthread_getschedparam(t, &policy, &param);
   if (rv) {
      perror("MusE: JackAudioDevice::realtimePriority: Error: Get jack schedule parameter");
      return 0;
   }
   if (policy != SCHED_FIFO) {
      fprintf(stderr, "MusE: JackAudioDevice::realtimePriority: JACK is not running realtime\n");
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
            fprintf(stderr, "initJackAudio()\n");
      
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
        {
          fprintf(stderr, "MusE:initJackAudio: jack_get_version() returned zeros. Setting version major to 1.\n");
          jack_ver_maj = 1;
        }
      }

      jack_port_set_name_fp = reinterpret_cast<jack_port_set_name_type>(dlsym(RTLD_DEFAULT, "jack_port_set_name"));
      DEBUG_PRST_ROUTES(stderr, "initJackAudio jack_port_set_name() address:%p \n", jack_port_set_name_fp);
      if(jack_port_set_name_fp)
      {
      }
      
      jack_port_rename_fp = reinterpret_cast<jack_port_rename_type>(dlsym(RTLD_DEFAULT, "jack_port_rename"));
      DEBUG_PRST_ROUTES(stderr, "initJackAudio jack_port_rename() address:%p \n", jack_port_rename_fp);
      if(jack_port_rename_fp)
      {
      }
      
      if (MusEGlobal::debugMsg) {
            fprintf(stderr, "initJackAudio(): registering error and info callbacks...\n");
            jack_set_error_function(jackError);
            jack_set_info_function(jackInfo);
            }
      else {
            jack_set_error_function(noJackError);
            jack_set_info_function(noJackInfo);
      }
      
      MusEGlobal::doSetuid();

      int opts = JackNullOption;
      if(MusEGlobal::noAutoStartJack)
        opts |= JackNoStartServer;
      jack_status_t status;
      jack_client_t* client = jack_client_open("MusE", (jack_options_t)opts, &status);
      if (!client) {
            if (status & JackServerStarted)
                  fprintf(stderr, "jack server started...\n");
            if (status & JackServerFailed)
                  fprintf(stderr, "cannot connect to jack server\n");
            if (status & JackServerError)
                  fprintf(stderr, "communication with jack server failed\n");
            if (status & JackShmFailure)
                  fprintf(stderr, "jack cannot access shared memory\n");
            if (status & JackVersionError)
                  fprintf(stderr, "jack server has wrong version\n");
            fprintf(stderr, "cannot create jack client\n");
	    MusEGlobal::undoSetuid();   
            return true;
            }

      if (MusEGlobal::debugMsg)
            fprintf(stderr, "initJackAudio(): client %s opened.\n", jack_get_client_name(client));
      
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
      // Make sure the AL namespace variables mirror our variables.
      AL::sampleRate = MusEGlobal::sampleRate;
      MusEGlobal::segmentSize = jack_get_buffer_size(client);
      
      return false;
      }

static int bufsize_callback(jack_nframes_t n, void*)
      {
      fprintf(stderr, "JACK: buffersize changed %d\n", n);
      return 0;
      }

//---------------------------------------------------------
//   freewheel_callback
//---------------------------------------------------------

static void freewheel_callback(int starting, void*)
      {
      if (MusEGlobal::debugMsg || JACK_DEBUG)
            fprintf(stderr, "JACK: freewheel_callback: starting%d\n", starting);
      MusEGlobal::audio->setFreewheel(starting);
      }

static int srate_callback(jack_nframes_t n, void*)
      {
      if (MusEGlobal::debugMsg || JACK_DEBUG)
            fprintf(stderr, "JACK: sample rate changed: %d\n", n);
      return 0;
      }

//---------------------------------------------------------
//   registration_callback
//---------------------------------------------------------

static void registration_callback(jack_port_id_t port_id, int is_register, void*)
{
  if(MusEGlobal::debugMsg || JACK_DEBUG)
    fprintf(stderr, "JACK: registration_callback\n");

  DEBUG_PRST_ROUTES(stderr, "JACK: registration_callback: port_id:%d is_register:%d\n", port_id, is_register);

  // With Jack-1 do not use functions like jack_port_by_name and jack_port_by_id here. 
  // With registration the port has not been added yet, so they allocate a new 
  //  'external' port which is NOT the same as the port returned by jack_port_register !
  // Thereafter each call to those functions returns THAT allocated port NOT the jack_port_register one. 
  // [ This was a bug in Jack1 due to a missing section. A fix by Tim was submitted late 2014 and was pending. ]
  JackCallbackEvent ev;
  ev.type = is_register ? PortRegister : PortUnregister;
  ev.port_id_A = port_id;

  jackCallbackFifo.put(ev);

  // NOTE: Jack-1 does not issue a graph order callback after a registration call. 
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
            fprintf(stderr, "JACK: client registration changed:%s register:%d\n", name, isRegister);
      DEBUG_PRST_ROUTES(stderr, "JACK: client registration changed:%s register:%d\n", name, isRegister);
      }

//---------------------------------------------------------
//   port_connect_callback
//---------------------------------------------------------

static void port_connect_callback(jack_port_id_t a, jack_port_id_t b, int isConnect, void* arg)
{
  if (MusEGlobal::debugMsg || JACK_DEBUG)
      fprintf(stderr, "JACK: port connections changed: A:%d B:%d isConnect:%d\n", a, b, isConnect);
  
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
            fprintf(stderr, "graph_callback()\n");
  
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

// static void latency_callback(jack_latency_callback_mode_t /*mode*/, void* /*arg*/)
// {
//   //JackAudioDevice* jad = (JackAudioDevice*)arg;
// 
//   // TODO: Do all of our Input Track and Output Track ports.
// 
// //   jack_latency_range_t range;
// //   if (mode == JackCaptureLatency) {
// //     jack_port_get_latency_range (input_port, mode, &range);
// //     range.min += latency;
// //     range.max += latency;
// //     jack_port_set_latency_range (output_port, mode, &range);
// //   } else {
// //     jack_port_get_latency_range (output_port, mode, &range);
// //     range.min += latency;
// //     range.max += latency;
// //     jack_port_set_latency_range (input_port, mode, &range);
// //   }
// }


void JackAudioDevice::processJackCallbackEvents(const Route& our_node, jack_port_t* our_port, 
                                                RouteList* route_list, bool is_input)
{
  jack_client_t* client = jackClient();
  if(!client)
    return;
  
  jack_port_t* our_ext_port = our_port;
  const char* our_port_name = our_port ? jack_port_name(our_port) : 0;

  if(our_port && our_port_name && jack1_port_by_name_workaround)
  {
    jack_port_t* jp = jack_port_by_name(client, our_port_name);
    if(jp && jp != our_port)
    {
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
    // FIXME: TODO: Maybe switch to get_ports
    jack_port_t* jp = jack_port_by_name(client, route_jpname);
    if(jp)
    {
      // TODO: For Jack-2 maybe alter this? Before calling jack_port_connected_to(), maybe first check if the IDs 
      //        (hence jack ports) passed in the connect callback match here, to avoid calling jack_port_connected_to() ?
      if(our_port && jack_port_connected_to(our_port, route_jpname)) 
      {
        // The ports are connected. Keep the route node but update its jack port pointer if necessary.
        const char* s = NULL;
        if(jp != ir->jackPort)
        {
          DEBUG_PRST_ROUTES(stderr, "processJackCallbackEvents: Ports connected. Modifying route: our_port:%p old_route_jp:%p new_route_jp:%p route_persistent_name:%s\n", 
                  our_port, ir->jackPort, jp, route_jpname);
          s = route_jpname;
        }
        // Find a more appropriate name if necessary.
        char fin_name[ROUTE_PERSISTENT_NAME_SIZE];
        portName(jp, fin_name, ROUTE_PERSISTENT_NAME_SIZE);
        if(strcmp(ir->persistentJackPortName, fin_name) != 0)
        {
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
          // Support our port == null (midi device not assigned to a midi port or I/O disabled etc.):
          // If our port is null, treat this as an unregister...
          const int ret = our_ext_port ? checkDisconnectCallback(our_ext_port, jp) : 1;
          if(ret == 2)
          {
            DEBUG_PRST_ROUTES(stderr, "processJackCallbackEvents: Ports not connected, ret=DeleteRouteNode. Deleting route: our_port:%p route_jp:%p found_jp:%p route_persistent_name:%s\n", 
                      our_port, ir->jackPort, jp, route_jpname);
            // The port exists but is not connected to our port. Remove the route node.
            operations.add(PendingOperationItem(route_list, ir, PendingOperationItem::DeleteRouteNode));
          }
          else
          if(ret == 1)
          {
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
        DEBUG_PRST_ROUTES(stderr, "processJackCallbackEvents: Port non-existent. Modifying route: our_port:%p route_jp:%p route_persistent_name:%s\n", 
                our_port, ir->jackPort, route_jpname);
        operations.add(PendingOperationItem(Route(Route::JACK_ROUTE, 0, NULL, ir->channel, 0, 0, ir->persistentJackPortName), &(*ir), PendingOperationItem::ModifyRouteNode));
      }
    }
  }

  if(our_port)
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
        fprintf(stderr, "graphChanged()\n");
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
    MusEGlobal::audio->msgExecutePendingOperations(operations, true);
    operations.clear();
  }
}

void JackAudioDevice::processGraphChanges()
{
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
      // Support even if port == null.
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
      // Support even if port == null.
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
      // Support even if port == null.
      processJackCallbackEvents(Route(md, -1), port, md->outRoutes(), false);
    }  
          
    //------------------------
    // Midi inputs:
    //------------------------
    
    if(md->rwFlags() & 2) // Readable
    {
      jack_port_t* port = (jack_port_t*)md->inClientPort();
      // Support even if port == null.
      processJackCallbackEvents(Route(md, -1), port, md->inRoutes(), true);
    }  
  }
}

void JackAudioDevice::checkNewRouteConnections(jack_port_t* our_port, int channel, RouteList* route_list)
{
  DEBUG_PRST_ROUTES(stderr, "JackAudioDevice::checkNewRouteConnections(): client:%p our_port:%p channel:%d route_list:%p\n", 
          _client, our_port, channel, route_list);
  // Check for new connections...
  const char** ports = jack_port_get_all_connections(_client, our_port);
  if(ports) 
  {
    const char** pn = ports;
    while(*pn) 
    {
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
  DEBUG_PRST_ROUTES(stderr, "JackAudioDevice::checkPortRegisterCallback(): port:%p\n", port); 
      
  iJackCallbackEvent ijce = jackCallbackEvents.end();
  while(ijce != jackCallbackEvents.begin())
  {
    --ijce;
    if(ijce->type == PortRegister)
    {
      jack_port_id_t id = ijce->port_id_A;
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

int JackAudioDevice::static_JackXRunCallback(void *)
{
   MusEGlobal::audio->incXruns();
   return 0;
}

//static int xrun_callback(void*)
//      {
//      fprintf(stderr, "JACK: xrun\n");
//      return 0;
//      }

//---------------------------------------------------------
//   register
//---------------------------------------------------------

void JackAudioDevice::registerClient()
      {
      if (JACK_DEBUG)
            fprintf(stderr, "registerClient()\n");
      
//       if (MusEGlobal::debugMsg) {
//             fprintf(stderr, "JackAudioDevice::registerClient(): registering error and info callbacks...\n");
//             jack_set_error_function(jackError);
//             jack_set_info_function(jackInfo);
//             }
//       else {
//             fprintf(stderr, "JackAudioDevice::registerClient(): registering no error and no info callbacks...\n");
//             jack_set_error_function(noJackError);
//             jack_set_info_function(noJackInfo);
//       }
      
      if(!checkJackClient(_client)) return;

      jack_set_thread_init_callback(_client, (JackThreadInitCallback) jack_thread_init, 0);
      //jack_set_timebase_callback(client, 0, (JackTimebaseCallback) timebase_callback, 0);
      jack_set_process_callback(_client, processAudio, 0);
      
      // NOTE: We set the driver's sync timeout in the gui thread. See Song::seqSignal().
      jack_set_sync_callback(_client, processSync, 0);
      
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
      // Tell the JACK server to call `latency()' whenever the latency needs to be recalculated.
//       if(jack_set_latency_callback)
//         jack_set_latency_callback(_client, latency_callback, this);


      jack_set_xrun_callback(_client, static_JackXRunCallback, this);
      }

//---------------------------------------------------------
//   registerInPort
//---------------------------------------------------------

void* JackAudioDevice::registerInPort(const char* name, bool midi)
      {
      if(JACK_DEBUG)
        fprintf(stderr, "registerInPort()\n");
      if(!checkJackClient(_client) || !name || name[0] == '\0') 
        return NULL;
      const char* type = midi ? JACK_DEFAULT_MIDI_TYPE : JACK_DEFAULT_AUDIO_TYPE;
      void* p = jack_port_register(_client, name, type, JackPortIsInput, 0);
      DEBUG_PRST_ROUTES(stderr, "JACK: registerInPort: <%s> %p\n", name, p);
      return p;
      }

//---------------------------------------------------------
//   registerOutPort
//---------------------------------------------------------

void* JackAudioDevice::registerOutPort(const char* name, bool midi)
      {
      if(JACK_DEBUG)
        fprintf(stderr, "registerOutPort()\n");
      if(!checkJackClient(_client) || !name || name[0] == '\0') 
        return NULL;
      const char* type = midi ? JACK_DEFAULT_MIDI_TYPE : JACK_DEFAULT_AUDIO_TYPE;
      void* p = jack_port_register(_client, name, type, JackPortIsOutput, 0);
      DEBUG_PRST_ROUTES(stderr, "JACK: registerOutPort: <%s> %p\n", name, p);
      return p;
      }

//---------------------------------------------------------
//   connect
//---------------------------------------------------------

bool JackAudioDevice::connect(void* src, void* dst)
{
      if (JACK_DEBUG)
            fprintf(stderr, "JackAudioDevice::connect()\n");
      if(!checkJackClient(_client)) return false;
      const char* sn = jack_port_name((jack_port_t*) src);
      const char* dn = jack_port_name((jack_port_t*) dst);
      if (sn == 0 || dn == 0) {
            fprintf(stderr, "JackAudio::connect: unknown jack ports\n");
            return false;
            }
      int err = jack_connect(_client, sn, dn);
      //if (jack_connect(_client, sn, dn)) {
      if (err) {
            fprintf(stderr, "jack connect <%s>%p - <%s>%p failed with err:%d\n",
               sn, src, dn, dst, err);
            return false;
            }
      else
      if (JACK_DEBUG)
      {
        fprintf(stderr, "jack connect <%s>%p - <%s>%p succeeded\n",
           sn, src, dn, dst);
      }      
      return true;
}

bool JackAudioDevice::connect(const char* src, const char* dst)
{
  if(JACK_DEBUG)
    fprintf(stderr, "JackAudioDevice::connect()\n");
  if(!checkJackClient(_client) || !src || !dst || src[0] == '\0' || dst[0] == '\0') 
    return false;
  int err = jack_connect(_client, src, dst);
  if(err) 
  {
    fprintf(stderr, "jack connect <%s> - <%s> failed with err:%d\n", src, dst, err);
    return false;
  }
  return true;
}

//---------------------------------------------------------
//   disconnect
//---------------------------------------------------------

bool JackAudioDevice::disconnect(void* src, void* dst)
{
      if (JACK_DEBUG)
            fprintf(stderr, "JackAudioDevice::disconnect()\n");
      if(!checkJackClient(_client)) return false;
      if(!src || !dst)  
        return false;
      const char* sn = jack_port_name((jack_port_t*) src);
      const char* dn = jack_port_name((jack_port_t*) dst);
      if (sn == 0 || dn == 0) {
            fprintf(stderr, "JackAudio::disconnect: unknown jack ports\n");
            return false;
            }
      int err = jack_disconnect(_client, sn, dn);
      //if (jack_disconnect(_client, sn, dn)) {
      if (err) {
            fprintf(stderr, "jack disconnect <%s> - <%s> failed with err:%d\n",
               sn, dn, err);
            return false;
            }
      else
      if (JACK_DEBUG)
      {
            fprintf(stderr, "jack disconnect <%s> - <%s> succeeded\n",
               sn, dn);
      }      
      return true;
}

bool JackAudioDevice::disconnect(const char* src, const char* dst)
{
  if(JACK_DEBUG)
    fprintf(stderr, "JackAudioDevice::disconnect()\n");
  if(!checkJackClient(_client) || !src || !dst || src[0] == '\0' || dst[0] == '\0') 
    return false;
  int err = jack_disconnect(_client, src, dst);
  if(err)
  {
    fprintf(stderr, "jack disconnect <%s> - <%s> failed with err:%d\n", src, dst, err);
    return false;
  }
  return true;
}

//---------------------------------------------------------
//   portsCanDisconnect
//---------------------------------------------------------

bool JackAudioDevice::portsCanDisconnect(void* src, void* dst) const
{
  if(!_client)
    return false;
  if(!src || !dst)
    return false;
  
  const char** ports = jack_port_get_all_connections(_client, (jack_port_t*)src);
  if(!ports)
    return false;

  bool rv = false;  
  for(const char** p = ports; p && *p; ++p) 
  {
    jack_port_t* jp = jack_port_by_name(_client, *p);
    if(jp == dst)
    {
      rv = true;
      break;
    }
  }
  jack_free(ports);
  return rv;
}

bool JackAudioDevice::portsCanDisconnect(const char* src, const char* dst) const
{
  if(!_client)
    return false;
  return portsCanDisconnect(jack_port_by_name(_client, src), jack_port_by_name(_client, dst));
}

//---------------------------------------------------------
//   portsCanConnect
//---------------------------------------------------------

bool JackAudioDevice::portsCanConnect(void* src, void* dst) const
{ 
  if(!_client)
    return false;
  if(!src || !dst)
    return false;
  const char* src_type = jack_port_type((jack_port_t*)src);
  const char* dst_type = jack_port_type((jack_port_t*)dst);
  if(!src_type || !dst_type || (strcmp(src_type, dst_type) != 0))
    return false;

  if(!(jack_port_flags((jack_port_t*)src) & JackPortIsOutput) || !(jack_port_flags((jack_port_t*)dst) & JackPortIsInput))
    return false;
  
  const char** ports = jack_port_get_all_connections(_client, (jack_port_t*)src);
  if(!ports)
    return true;

  bool rv = true;  
  for(const char** p = ports; p && *p; ++p) 
  {
    jack_port_t* jp = jack_port_by_name(_client, *p);
    if(jp == dst)
    {
      rv = false;
      break;
    }
  }

  jack_free(ports);
  return rv;
}

bool JackAudioDevice::portsCanConnect(const char* src, const char* dst) const
{ 
  if(!_client)
    return false;
  return portsCanConnect(jack_port_by_name(_client, src), jack_port_by_name(_client, dst));
}

//---------------------------------------------------------
//   portsCompatible
//---------------------------------------------------------

bool JackAudioDevice::portsCompatible(void* src, void* dst) const
{
  if(!src || !dst)
    return false;
  const char* src_type = jack_port_type((jack_port_t*)src);
  const char* dst_type = jack_port_type((jack_port_t*)dst);
  if(!src_type || !dst_type || (strcmp(src_type, dst_type) != 0))
    return false;

  if(!(jack_port_flags((jack_port_t*)src) & JackPortIsOutput) || !(jack_port_flags((jack_port_t*)dst) & JackPortIsInput))
    return false;
  
  return true;
}

bool JackAudioDevice::portsCompatible(const char* src, const char* dst) const
{
  if(!_client)
    return false;
  return portsCompatible(jack_port_by_name(_client, src), jack_port_by_name(_client, dst));
}

//---------------------------------------------------------
//   start
//   Return true on success.
//---------------------------------------------------------

bool JackAudioDevice::start(int /*priority*/)
      {
      if (JACK_DEBUG)
            fprintf(stderr, "JackAudioDevice::start()\n");
      if(!checkJackClient(_client)) return false;

      MusEGlobal::doSetuid();

      DEBUG_PRST_ROUTES (stderr, "JackAudioDevice::start(): calling jack_activate()\n");

// REMOVE Tim. latency. Changed. TESTING.
//       if (jack_activate(_client)) {
//             MusEGlobal::undoSetuid();   
//             fprintf (stderr, "JACK: cannot activate client\n");
//             exit(-1);
//             }
      if(!jackStarted)
      {
        if (jack_activate(_client)) {
              MusEGlobal::undoSetuid();   
              fprintf (stderr, "JACK: cannot activate client\n");
              exit(-1);
              }
      }
      jackStarted = true;

      MusEGlobal::undoSetuid();
      
      /* connect the ports. Note: you can't do this before
         the client is activated, because we can't allow
         connections to be made to clients that aren't
         running.
       */
      MusEGlobal::song->connectAllPorts();

      fflush(stdin);
      
      return true;
      }

//---------------------------------------------------------
//   stop
//---------------------------------------------------------

void JackAudioDevice::stop()
      {
      if (JACK_DEBUG)
            fprintf(stderr, "JackAudioDevice::stop()\n");
      if(!checkJackClient(_client)) return;
      DEBUG_PRST_ROUTES (stderr, "JackAudioDevice::stop(): calling jack_deactivate()\n");
      
// REMOVE Tim. latency. Changed. TESTING.
//       if (jack_deactivate(_client)) {
//             fprintf (stderr, "cannot deactivate client\n");
//             }

//       //if(jackStarted)
//       {
// 
//         if (jack_deactivate(_client)) {
//               fprintf (stderr, "cannot deactivate client\n");
//               }
// 
//       }
//       jackStarted = false;
      }

//---------------------------------------------------------
//   transportQuery
//---------------------------------------------------------

jack_transport_state_t JackAudioDevice::transportQuery(jack_position_t* pos)
{ 
  if (JACK_DEBUG)
    fprintf(stderr, "JackAudioDevice::transportQuery pos:%d\n", (unsigned int)pos->frame);
  
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
    fprintf(stderr, "timebaseQuery frame:%u\n", jp.frame); 
  
  if(jp.valid & JackPositionBBT)
  {
    if(JACK_DEBUG)
    {
      fprintf(stderr, "timebaseQuery BBT:\n bar:%d beat:%d tick:%d\n bar_start_tick:%f beats_per_bar:%f beat_type:%f ticks_per_beat:%f beats_per_minute:%f\n",
              jp.bar, jp.beat, jp.tick, jp.bar_start_tick, jp.beats_per_bar, jp.beat_type, jp.ticks_per_beat, jp.beats_per_minute);
      if(jp.valid & JackBBTFrameOffset)
        fprintf(stderr, "timebaseQuery BBTFrameOffset: %u\n", jp.bbt_offset);
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
        fprintf(stderr, "timebaseQuery curr_tick:%u f_rate:%f ticks:%u\n", curr_tick, f_rate, ticks);  

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
      fprintf(stderr, "timebaseQuery JackPositionTimecode: frame_time:%f next_time:%f\n", jp.frame_time, jp.next_time);
    if(jp.valid & JackAudioVideoRatio)
      fprintf(stderr, "timebaseQuery JackAudioVideoRatio: %f\n", jp.audio_frames_per_video_frame);
    if(jp.valid & JackVideoFrameOffset)
      fprintf(stderr, "timebaseQuery JackVideoFrameOffset: %u\n", jp.video_offset);
  }
  
  return false;
}                

//---------------------------------------------------------
//   timebaseAck
//   This is called by the timebase callback.
//---------------------------------------------------------

void JackAudioDevice::timebaseAck()
{
  _timebaseAck = true;
}

//---------------------------------------------------------
//   systemTimeUS
//   Return system time in microseconds as a 64-bit integer.
//   Depends on selected clock source. 
//   With Jack, may be based upon wallclock time, the   
//    processor cycle counter or the HPET clock etc.
//---------------------------------------------------------

uint64_t JackAudioDevice::systemTimeUS() const
{
  // Client valid? According to sletz: For jack_get_time "There are some timing related 
  //  initialization that are done once when a first client is created."
  if(!checkJackClient(_client))
    return AudioDevice::systemTimeUS();
  return jack_get_time();
}

//---------------------------------------------------------
//   getCurFrame
//---------------------------------------------------------

unsigned int JackAudioDevice::getCurFrame() const
{ 
  if (JACK_DEBUG)
    fprintf(stderr, "JackAudioDevice::getCurFrame pos.frame:%d\n", pos.frame);
  
  if(!MusEGlobal::useJackTransport.value())
    return AudioDevice::getCurFrame();
    
  return pos.frame; 
}

//---------------------------------------------------------
//   framePos
//---------------------------------------------------------

unsigned JackAudioDevice::framePos() const
      {
      //if(!MusEGlobal::useJackTransport.value())
      //{
      //  if (JACK_DEBUG)
      //    fprintf(stderr, "JackAudioDevice::framePos dummyPos:%d\n", dummyPos);
      //  return dummyPos;
      //}
      
      if(!checkJackClient(_client)) return 0;
      jack_nframes_t n = jack_frame_time(_client);
      
      //if (JACK_DEBUG)
      //  fprintf(stderr, "JackAudioDevice::framePos jack frame:%d\n", (int)n);
      
      return n;
      }

//---------------------------------------------------------
//   framesAtCycleStart
//   Frame count at the start of current cycle. 
//   This is meant to be called from inside process thread only.      
//---------------------------------------------------------

unsigned JackAudioDevice::framesAtCycleStart() const 
{ 
      if(!checkJackClient(_client)) return 0;
      jack_nframes_t n = jack_last_frame_time(_client);
      //if (JACK_DEBUG)
      //  fprintf(stderr, "JackAudioDevice::framesAtCycleStart jack frame:%d\n", (unsigned)n);
      return (unsigned)n;
}

//---------------------------------------------------------
//   framesSinceCycleStart
//   Estimated frames since the last process cycle began
//   This is meant to be called from inside process thread only.      
//---------------------------------------------------------

unsigned JackAudioDevice::framesSinceCycleStart() const 
{ 
      if(!checkJackClient(_client)) return 0;
      jack_nframes_t n = jack_frames_since_cycle_start(_client);
      //if (JACK_DEBUG)
      //  fprintf(stderr, "JackAudioDevice::framesSinceCycleStart jack frame:%d\n", (unsigned)n);

      // Safety due to inaccuracies. It cannot be after the segment, right?
      if(n >= MusEGlobal::segmentSize)
        n = MusEGlobal::segmentSize - 1;
      
      return (unsigned)n;
}

#if 0
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

unsigned JackAudioDevice::curTransportFrame() const
{ 
  if(!checkJackClient(_client)) return 0;
  return jack_get_current_transport_frame(_client);
}

//---------------------------------------------------------
//   getJackPorts
//---------------------------------------------------------

void JackAudioDevice::getJackPorts(const char** ports, std::list<QString>& name_list, bool midi, bool physical, int aliases)
      {
      if (JACK_DEBUG)
            fprintf(stderr, "JackAudioDevice::getJackPorts()\n");
      QString qname;
      QString cname(jack_get_client_name(_client));
      
      for (const char** p = ports; p && *p; ++p) {
            // Should be safe and quick search here, we know that the port name is valid.
            jack_port_t* port = jack_port_by_name(_client, *p);
            int port_flags = jack_port_flags(port);

            // Ignore our own client ports.
            if(jack_port_is_mine(_client, port))
            {
              if(MusEGlobal::debugMsg)
                fprintf(stderr, "JackAudioDevice::getJackPorts ignoring own port: %s\n", *p);
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
                    //fprintf(stderr, "Checking port name for: %s\n", (QString("alsa_pcm:") + cname + QString("/")).toLatin1().constData());  
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
            fprintf(stderr, "JackAudioDevice::outputPorts()\n");
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
            fprintf(stderr, "JackAudioDevice::inputPorts()\n");
      
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
//   setPortName
//---------------------------------------------------------

void JackAudioDevice::setPortName(void* p, const char* n)
{ 
  // NOTE: jack_port_set_name() is deprecated as of Jack2 = 1.9.11, and Jack1 > 0.124.1
  if(jack_port_rename_fp)
  {
    if(!checkJackClient(_client))
      return;
    jack_port_rename_fp(_client, (jack_port_t*)p, n);
  }
  else if(jack_port_set_name_fp)
    jack_port_set_name_fp((jack_port_t*)p, n);
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
  if(na >= 1 && *al[0] != '\0')
  {
    if((strncmp(al[0], "system:", 7) != 0 && preferred_name_or_alias == -1) || preferred_name_or_alias == 1)
      return MusELib::strntcpy(str, al[0], str_size);
    B = true;
  }

  if(na >= 2 && *al[1] != '\0')
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
  if(!checkJackClient(_client) || !port)
    return 0;

  //QString s(jack_port_name((jack_port_t*)port));
  //fprintf(stderr, "Jack::portName %p %s\n", port, s.toLatin1().constData());  

  
  // NOTICE: For at least the ALSA driver (tested), the input latency is
  //          always 1 period while the output latency is always n periods
  //          (or n-1 periods for Jack1 or Jack2 Sync mode).
  //         (Also there is the user latency from command line or QJackCtl.)
  //         In other words, the Jack command line -p (number of periods) ONLY applies to audio output ports.
  
  jack_latency_range_t p_range;
  jack_port_get_latency_range((jack_port_t*)port, JackPlaybackLatency, &p_range);

  jack_latency_range_t c_range;
  jack_port_get_latency_range((jack_port_t*)port, JackCaptureLatency, &c_range);

  //if(MusEGlobal::audio->isPlaying())
  //  fprintf(stderr, "JackAudioDevice::portLatency port:%p capture:%d c_range.min:%d c_range.max:%d p_range.min:%d p_range.max:%d\n",
  //          port, capture, c_range.min, c_range.max, p_range.min, p_range.max);

  if(capture)
    return c_range.max;

  return p_range.max;

  // Hm... for speed, maybe cache the values?
}

//---------------------------------------------------------
//   unregisterPort
//---------------------------------------------------------

void JackAudioDevice::unregisterPort(void* p)
      {
      if(JACK_DEBUG)
        fprintf(stderr, "JackAudioDevice::unregisterPort(%p)\n", p);
      if(!checkJackClient(_client) || !p) 
        return;
//      fprintf(stderr, "JACK: unregister Port\n");
      jack_port_unregister(_client, (jack_port_t*)p);
      }

float JackAudioDevice::getDSP_Load()
{
  return jack_cpu_load(_client);
}

AudioDevice::PortType JackAudioDevice::portType(void* p) const
{ 
  if(!p)
    return UnknownType; 
  if(const char* type = jack_port_type((jack_port_t*)p))
  {
    if(strcmp(type, JACK_DEFAULT_AUDIO_TYPE) == 0)
      return AudioPort; 
    if(strcmp(type, JACK_DEFAULT_MIDI_TYPE) == 0)
      return MidiPort; 
  }
  return UnknownType; 
}

AudioDevice::PortDirection JackAudioDevice::portDirection(void* p) const
{ 
  if(!p)
    return UnknownDirection; 
  const int flags = jack_port_flags((jack_port_t*)p);
  if(flags & JackPortIsInput)
    return InputPort;
  if(flags & JackPortIsOutput)
    return OutputPort;
  return UnknownDirection; 
}
      
//---------------------------------------------------------
//   setSyncTimeout
//    Sets the amount of time to wait before sync times out, in microseconds.
//    Note that at least with the Jack driver, this function seems not realtime friendly.
//---------------------------------------------------------

void JackAudioDevice::setSyncTimeout(unsigned usec)
{ 
  // Make sure our built-in transport sync timeout is set as well.
  AudioDevice::setSyncTimeout(usec);
  
  if(!checkJackClient(_client)) return;
  // Note that docs say default is 2 sec, but that's wrong, 
  //  Jack1 does set 2 sec, but Jack2 sets a default of 10 secs !
  jack_set_sync_timeout(_client, usec);
}
      
//---------------------------------------------------------
//   transportSyncToPlayDelay
//   The number of frames which the driver waits to switch to PLAY
//    mode after the audio sync function says it is ready to roll.
//   For example Jack Transport waits one cycle while our own tranport does not.
//---------------------------------------------------------

unsigned JackAudioDevice::transportSyncToPlayDelay() const
{ 
  // If Jack transport is being used, it delays by one cycle.
  if(MusEGlobal::useJackTransport.value())
    return MusEGlobal::segmentSize;
  return 0;
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
        //  fprintf(stderr, "JackAudioDevice::getState dummyState:%d\n", dummyState);
        return AudioDevice::getState();
      }
      
      //if (JACK_DEBUG)
      //      fprintf(stderr, "JackAudioDevice::getState ()\n");
      if(!checkJackClient(_client)) return 0;
      transportState = jack_transport_query(_client, &pos);
      //if (JACK_DEBUG)
      //    fprintf(stderr, "JackAudioDevice::getState transportState:%d\n", transportState);
      
      switch (int(transportState)) {
            case JackTransportStopped:  
              return Audio::STOP;
            case JackTransportLooping:
            case JackTransportRolling:  
              return Audio::PLAY;
            case JackTransportStarting:  
              //fprintf(stderr, "JackAudioDevice::getState JackTransportStarting\n");
              
              return Audio::START_PLAY;
            //case JackTransportNetStarting: -- only available in Jack-2!
            // FIXME: Quick and dirty hack to support both Jack-1 and Jack-2
            // Really need a config check of version...
            case 4:  
              //fprintf(stderr, "JackAudioDevice::getState JackTransportNetStarting\n");
              
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
            fprintf(stderr, "JackAudioDevice::setFreewheel(\n");
      if(!checkJackClient(_client)) return;
      jack_set_freewheel(_client, f);
      }

//---------------------------------------------------------
//   startTransport
//---------------------------------------------------------

void JackAudioDevice::startTransport()
    {
      if (JACK_DEBUG)
            fprintf(stderr, "JackAudioDevice::startTransport()\n");
      
      // If we're not using Jack's transport, just pass PLAY and current frame along
      //  as if processSync was called. 
      if(!MusEGlobal::useJackTransport.value())
      {
        AudioDevice::startTransport();
        return;
      }
      
      if(!checkJackClient(_client)) return;
//      fprintf(stderr, "JACK: startTransport\n");
      jack_transport_start(_client);
    }

//---------------------------------------------------------
//   stopTransport
//---------------------------------------------------------

void JackAudioDevice::stopTransport()
    {
      if (JACK_DEBUG)
            fprintf(stderr, "JackAudioDevice::stopTransport()\n");
      
      if(!MusEGlobal::useJackTransport.value())
      {
        AudioDevice::stopTransport();
        return;
      }
      
      if(!checkJackClient(_client)) return;
      if (transportState != JackTransportStopped) {
        //      fprintf(stderr, "JACK: stopTransport\n");
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
            fprintf(stderr, "JackAudioDevice::seekTransport() frame:%d\n", frame);
      
      if(!MusEGlobal::useJackTransport.value())
      {
        // STOP -> STOP means seek in stop mode. PLAY -> START_PLAY means seek in play mode.
        AudioDevice::seekTransport(frame);
        return;
      }
      
      if(!checkJackClient(_client)) return;
//      fprintf(stderr, "JACK: seekTransport %d\n", frame);
      jack_transport_locate(_client, frame);
    }

//---------------------------------------------------------
//   seekTransport
//---------------------------------------------------------

void JackAudioDevice::seekTransport(const Pos &p)
      {
      if (JACK_DEBUG)
            fprintf(stderr, "JackAudioDevice::seekTransport(Pos) frame:%d\n", p.frame());
      
      if(!MusEGlobal::useJackTransport.value())
      {
        // STOP -> STOP means seek in stop mode. PLAY -> START_PLAY means seek in play mode.
        AudioDevice::seekTransport(p);
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
            fprintf(stderr, "JackAudioDevice::findPort(%s)\n", name);
      if(!checkJackClient(_client) || !name || name[0] == '\0') 
        return NULL;
      void* p = jack_port_by_name(_client, name);
      return p;
      }

//---------------------------------------------------------
//   setMaster
//---------------------------------------------------------

int JackAudioDevice::setMaster(bool f)
{
  if (JACK_DEBUG)
    fprintf(stderr, "JackAudioDevice::setMaster val:%d\n", f);
  if(!checkJackClient(_client)) 
    return 0;
  
  int r = 0;
  if(f)
  {
    if(MusEGlobal::useJackTransport.value())
    {
      // Make Muse the Jack timebase master. Do it unconditionally (second param = 0).
      r = jack_set_timebase_callback(_client, 0, (JackTimebaseCallback) timebase_callback, this);
      if(MusEGlobal::debugMsg || JACK_DEBUG)
      {
        if(r)
          fprintf(stderr, "JackAudioDevice::setMaster jack_set_timebase_callback failed: result:%d\n", r);
      }      
    }  
    else
    {
      r = 1;
      fprintf(stderr, "JackAudioDevice::setMaster cannot set master because useJackTransport is false\n");
    }
  }  
  else
  {
    r = jack_release_timebase(_client);
    if(MusEGlobal::debugMsg || JACK_DEBUG)
    {
      if(r)
        fprintf(stderr, "JackAudioDevice::setMaster jack_release_timebase failed: result:%d\n", r);
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
            fprintf(stderr, "exitJackAudio()\n");
      if (jackAudio)
            delete jackAudio;
            
      if (JACK_DEBUG)
            fprintf(stderr, "exitJackAudio() after delete jackAudio\n");
      
      MusEGlobal::audioDevice = NULL;      
      muse_atomic_destroy(&atomicGraphChangedPending);
      }
    
} // namespace MusECore


