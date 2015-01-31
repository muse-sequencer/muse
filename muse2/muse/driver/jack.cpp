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
#include <list>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <stdarg.h>
//#include <time.h> 
#include <unistd.h>
#include <jack/midiport.h>
#include <string.h>
// REMOVE Tim. Persistent routes. Added.
#include <dlfcn.h>

#include <QString>
#include <QStringList>

#include "libs/strntcpy.h"
#include "audio.h"
#include "globals.h"
//#include "globaldefs.h"
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

// REMOVE Tim. Persistent routes. Added.
#include "muse_atomic.h"

#define JACK_DEBUG 0 

// REMOVE Tim. Persistent routes. Added.
#define JACK_CALLBACK_FIFO_SIZE 512
//#define JACK_OPERATIONS_FIFO_SIZE 4096

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

// REMOVE Tim. Persistent routes. Added.
int jack_ver_maj = 0, jack_ver_min = 0, jack_ver_micro = 0, jack_ver_proto = 0;
muse_atomic_t atomicGraphChangedPending;
bool jack1_port_by_name_workaround = false;
// Function pointers obtained with dlsym:
jack_get_version_type             jack_get_version_fp = NULL;  
// jack1_internal_client_load_type   jack1_internal_client_load_fp = NULL;
// jack2_internal_client_load_type   jack2_internal_client_load_fp = NULL;
// jack1_internal_client_handle_type jack1_internal_client_handle_fp = NULL;
// jack2_internal_client_handle_type jack2_internal_client_handle_fp = NULL;

//---------------------------------------------------------
//  JackCallbackEvent
//  Item struct for JackCallbackFifo. 
//---------------------------------------------------------

// enum JackCallbackEventType {PortRegister, PortUnregister, PortConnect, PortDisconnect, GraphChanged };
// struct JackCallbackEvent
// {
//   JackCallbackEventType type;
//   jack_port_id_t port_id_A;
//   jack_port_id_t port_id_B;
//   jack_port_t* port_A;
//   jack_port_t* port_B;
//   ////QString name_A;
//   ////QString name_B;
//   //char name_A[ROUTE_PERSISTENT_NAME_SIZE];
//   //char name_B[ROUTE_PERSISTENT_NAME_SIZE];
//   //JackCallbackEvent() { name_A[0] = '\0'; name_B[0] = '\0'; }
// };


// enum CallbackRouteState { JCB_UNCHANGED, JCB_MODIFY, JCB_ADD, JCB_DELETE, JCB_CONNECT };
// struct RouteStruct
// {
//   // Link to the original route.
//   Route*         route;
//   // These items track changes to be committed to the route.
//   //jack_port_t*   port;
//   //jack_port_id_t port_id;
//   //QString        port_name;
//   Route          work_route;
//   Route          our_route;
//   bool           is_input;
//   //CallbackState  init_state;
//   CallbackRouteState  state;
// //   RouteStruct() { route = 
// //   RouteStruct(Route* route_, jack_port_t* port_, jack_port_id_t port_id_, const QString& port_name_, CallbackState init_state_, CallbackState state_)
// //   {
// //     route = route_; port = port_; port_id = port_id_; port_name = port_name_; init_state = init_state_; state = state_; 
// //   }
// };
// typedef std::vector<RouteStruct> RouteStructList;
// typedef std::vector<RouteStruct>::iterator iRouteStructList;

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

// //---------------------------------------------------------
// //  JackOperationsFifo
// //---------------------------------------------------------
// 
// class JackOperationsFifo
// {
//     JackOperation fifo[JACK_OPERATIONS_FIFO_SIZE];
//     volatile int size;
//     int wIndex;
//     int rIndex;
// 
//   public:
//     JackOperationsFifo()  { clear(); }
//     bool put(const JackOperation& event);   // returns true on fifo overflow
//     const JackOperation& get();
//     const JackOperation& peek(int n = 0);
//     void remove();
//     bool isEmpty() const { return size == 0; }
//     void clear()         { size = 0, wIndex = 0, rIndex = 0; }
//     int getSize() const  { return size; }
// };
// 
// //---------------------------------------------------------
// //   JackOperationsFifo
// //    put
// //    return true on fifo overflow
// //---------------------------------------------------------
// 
// bool JackOperationsFifo::put(const JackOperation& event)
//       {
//       if (size < JACK_OPERATIONS_FIFO_SIZE) {
//             fifo[wIndex] = event;
//             wIndex = (wIndex + 1) % JACK_OPERATIONS_FIFO_SIZE;
//             ++size;
//             return false;
//             }
//       return true;
//       }
// 
// //---------------------------------------------------------
// //   get
// //---------------------------------------------------------
// 
// const JackOperation& JackOperationsFifo::get()
//       {
//       const JackOperation& event = fifo[rIndex];
//       rIndex = (rIndex + 1) % JACK_OPERATIONS_FIFO_SIZE;
//       --size;
//       return event;
//       }
// 
// //---------------------------------------------------------
// //   peek
// //---------------------------------------------------------
// 
// const JackOperation& JackOperationsFifo::peek(int n)
//       {
//       int idx = (rIndex + n) % JACK_OPERATIONS_FIFO_SIZE;
//       return fifo[idx];
//       }
// 
// //---------------------------------------------------------
// //   remove
// //---------------------------------------------------------
// 
// void JackOperationsFifo::remove()
//       {
//       rIndex = (rIndex + 1) % JACK_OPERATIONS_FIFO_SIZE;
//       --size;
//       }

// REMOVE Tim. Persistent routes. Added.
//---------------------------------------------------------
//  JackCallbackEventList
//---------------------------------------------------------

//typedef std::list<JackCallbackEvent> JackCallbackEventList;
//typedef std::list<JackCallbackEvent>::iterator iJackCallbackEvent;

//JackCallbackEventList jackCallbackEventList;
JackCallbackFifo jackCallbackFifo;
//JackOperationsFifo jackOpsFifo;

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
      // REMOVE Tim. Persistent routes. Added.
//       _intClient = 0;
//       if(jack_ver_maj == 0)
//       {
//         if(jack1_internal_client_load_fp) 
//         {
//           jack1_internal_client_load_fp(_client, "muse_internal_client");
//         }
//       }
//       else
//       {
//         if(jack2_internal_client_load_fp) 
//         {
//           jack2_internal_client_load_fp();
//         }
//       }
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
      
      // REMOVE Tim. Persistent routes. Added.
      muse_atomic_init(&atomicGraphChangedPending);
      muse_atomic_set(&atomicGraphChangedPending, 0);
      
      // REMOVE Tim. Persistent routes. Added.
      jack_get_version_fp = reinterpret_cast<jack_get_version_type>(dlsym(RTLD_DEFAULT, "jack_get_version"));
      fprintf(stderr, "initJackAudio jack_get_version() address:%p \n", jack_get_version_fp);
      if(jack_get_version_fp) // ATM Only in Jack-2. Dlsym'd. Check for existence first.
      {
        jack_get_version_fp(&jack_ver_maj, &jack_ver_min, &jack_ver_micro, &jack_ver_proto);
        // REMOVE Tim. Persistent routes. Added.
        fprintf(stderr, "initJackAudio: jack_ver_maj:%d jack_ver_min:%d jack_ver_micro:%d jack_ver_proto:%d\n", 
                jack_ver_maj, jack_ver_min, jack_ver_micro, jack_ver_proto);
        // FIXME: ATM Jack-2 jack_get_version() returns all zeros. When it is fixed, do something with the values.
        if(jack_ver_maj == 0 && jack_ver_min == 0 && jack_ver_micro == 0 && jack_ver_proto == 0)
          jack_ver_maj = 1;
      }

      // REMOVE Tim. Persistent routes. Added.
//       if(jack_ver_maj == 0) // Assume only for Jack-1.
//       {
//         jack1_internal_client_load_fp   = reinterpret_cast<jack1_internal_client_load_type>(dlsym(RTLD_DEFAULT, "jack_internal_client_load"));
//         jack1_internal_client_handle_fp = reinterpret_cast<jack1_internal_client_handle_type>(dlsym(RTLD_DEFAULT, "jack_internal_client_handle"));
//       }
//       else
//       {
//         jack2_internal_client_load_fp   = reinterpret_cast<jack2_internal_client_load_type>(dlsym(RTLD_DEFAULT, "jack_internal_client_load"));
//         jack2_internal_client_handle_fp = reinterpret_cast<jack2_internal_client_handle_type>(dlsym(RTLD_DEFAULT, "jack_internal_client_handle"));
//       }
      
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
      
      // REMOVE Tim. Persistent routes. Added.
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
      
      // REMOVE Tim. Persistent routes.
      //jackAudio->scanMidiPorts();
      
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
  fprintf(stderr, "JACK: registration_callback: port_id:%d is_register:%d\n", port_id, is_register);

  // REMOVE Tim. Persistent routes. Removed.
  //MusEGlobal::audio->sendMsgToGui('R');
  

  // In Jack-1 do not use functions like jack_port_by_name and jack_port_by_id here. 
  // With registration the port has not been added yet, so they allocate a new 
  //  'external' port which is NOT the same as the port returned by jack_port_register !
  // Thereafter each call to those functions returns THAT allocated port NOT the jack_port_register one. 
  JackCallbackEvent ev;
  ev.type = is_register ? PortRegister : PortUnregister;
  ev.port_id_A = port_id;

//   ev.port_A = NULL;
//   
//   if(is_register)
//   {
//     JackAudioDevice* jad = (JackAudioDevice*)arg;
//     jack_client_t* client = jad->jackClient();
//     if(client)
//       // Caution: In Jack-1, under some conditions jack_port_by_id() might ALLOCATE a new port for use later if not found! 
//       // Should be safe and quick search here, we know that port_id is valid.
//       ev.port_A = jack_port_by_id(client, port_id);
//     
//     // FIXME: NOTE: Problems getting aliases! Some may not be available yet! May have to wait until next cyle has passed?
//     //bool res;
//     //ev.name_A = jad->portName(ev.port_A, &res);
//     //if(!res)
//     //  return;
//   }
//   else
//   {
//     // Find any previous disconnect, AFTER a GraphChanged event.
//     int cb_fifo_sz = callbackFifo.getSize();
//     for(int i = cb_fifo_sz - 1; i >= 0; --i)
//     {
//       const JackCallbackEvent& jce = callbackFifo.peek(i);
//       if(jce.type == GraphChanged)
//         break;
//       if(jce.type == PortDisconnect)
//       { 
//         if(jce.port_id_A == port_id)
//           { ev.port_A = jce.port_A; break; }
//         if(jce.port_id_B == port_id)
//           { ev.port_A = jce.port_B; break; }
//       }
//     }
//   }
  
  jackCallbackFifo.put(ev);
//   jackCallbackEventList.push_back(ev);

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
//   JackAudioDevice::registrationChanged
//    this is called from song in gui context triggered
//    by registration_callback()
//---------------------------------------------------------

// void JackAudioDevice::registrationChanged()
// {
//   if(JACK_DEBUG)
//     printf("JackAudioDevice::registrationChanged()\n");
// 
//   // REMOVE Tim. Persistent routes. Added.
//   fprintf(stderr, "JackAudioDevice::registrationChanged\n");
//   
//   // Rescan.
//   scanMidiPorts();
//   // Connect the Jack midi client ports to the device ports.
//   //connectJackMidiPorts();
//   //scanJackRoutes();
// }

//---------------------------------------------------------
//   client_registration_callback
//---------------------------------------------------------

static void client_registration_callback(const char *name, int isRegister, void*)
      {
      if (MusEGlobal::debugMsg || JACK_DEBUG)
            printf("JACK: client registration changed:%s register:%d\n", name, isRegister);
      // REMOVE Tim. Persistent routes. Added.
      printf("JACK: client registration changed:%s register:%d\n", name, isRegister);
      }

//---------------------------------------------------------
//   port_connect_callback
//---------------------------------------------------------

static void port_connect_callback(jack_port_id_t a, jack_port_id_t b, int isConnect, void* arg)
{
  if (MusEGlobal::debugMsg || JACK_DEBUG)
      printf("JACK: port connections changed: A:%d B:%d isConnect:%d\n", a, b, isConnect);
  
  // REMOVE Tim. Persistent routes. Added.
  fprintf(stderr, "JACK: port_connect_callback id a:%d id b:%d isConnect:%d\n", a, b, isConnect);
  
  // REMOVE Tim. Persistent routes. Added.
  //
  // A port connect callback will come before any port unregistration. 
  // We need to grab the port names NOW before the unregistration has a chance to happen.
  // Here we have the opportunity to grab two port names at once.
//        if(!isConnect)
//        {
//           jack_client_t* client = (jack_client_t*)arg;
//           JackCallbackEvent ev;
//           ev.type = PortConnect;
//           ev.port = jack_port_by_id(client, a);
//           const char* name = jack_port_name(ev.port);
//           if(name)
//           {
//             ev.name = QString(name);
//             callbackFifo.put(ev);
//           }
//           ev.port = jack_port_by_id(client, b);
//           name = jack_port_name(ev.port);
//           if(name)
//           {
//             ev.name = QString(name);
//             callbackFifo.put(ev);
//           }
    
//           // REMOVE Tim. Persistent routes. Added.
//           //MusEGlobal::audio->sendMsgToGui('J');
//   

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
    
    // REMOVE Tim. Persistent routes. Added.
//     fprintf(stderr, "JACK: port_connect_callback id a:%d port A:%p name:%s id b:%d port B:%p name:%s isConnect:%d\n", 
//             a, ev.port_A, ev.port_A ? jack_port_name(ev.port_A) : NULL, b, ev.port_B, ev.port_B ? jack_port_name(ev.port_B) : NULL, isConnect);
    
//           // A port (dis)connect callback will come before any port unregistration. 
//           // We need to grab the port names NOW before the unregistration has a chance to happen.
//           // Here we have the opportunity to grab two port names at once.
//           //ev.name_A = QString(jack_port_name(ev.port_A));
//           //ev.name_B = QString(jack_port_name(ev.port_B));
// // FIXME: NOTE: Problems getting aliases! Some may not be available yet! May have to wait until next cyle has passed?
// //           bool res1, res2;
// //           ev.name_A = jad->portName(ev.port_A, &res1);
// //           ev.name_B = jad->portName(ev.port_B, &res2);
// //           if(!res1 || !res2)
// //             return;

    jackCallbackFifo.put(ev);

//           jackCallbackEventList.push_back(ev);
//         
// //        }
    
    
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
      fprintf(stderr, "JACK: graph_callback\n");
      
//       JackAudioDevice* jad = (JackAudioDevice*)arg;
//       
//       for(iJackCallbackEvent ijce = jackCallbackEventList.begin(); ijce != jackCallbackEventList.end(); ++ijce)
//       {
//         // Need to do some processing here because alias names sometimes aren't available yet in the other callbacks...
//         switch(ijce->type)
//         {
//           case PortConnect:
//           case PortDisconnect:
//           {
//             bool res1, res2;
//             ijce->name_A = jad->portName(ijce->port_A, &res1);
//             ijce->name_B = jad->portName(ijce->port_B, &res2);
//             // FIXME: Try a few more times?   // REMOVE Tim. Persistent routes. 
//             ijce->name_A = jad->portName(ijce->port_A, &res1);
//             ijce->name_B = jad->portName(ijce->port_B, &res2);
//             ijce->name_A = jad->portName(ijce->port_A, &res1);
//             ijce->name_B = jad->portName(ijce->port_B, &res2);
//             if(!res1 || !res2)
//               continue;
//           }
//           break;
//           
//           case PortRegister:
//           {
//             bool res;
//             ijce->name_A = jad->portName(ijce->port_A, &res);
//             if(!res)
//               continue;
//           }
//           break;
//           
//           default:
//           break;
//         }
//         // Add the event to the FIFO.
//         callbackFifo.put(*ijce);
//       }
//       // Done with the list. Clear it.
//       jackCallbackEventList.clear(); 
//       
      // Optimization: No multiple GraphChanged events.
      // FIXME: NOTE: Can't do this! Writer can't peek ring buffer!
//       int cb_fifo_sz = callbackFifo.getSize();
//       if(cb_fifo_sz && callbackFifo.peek(cb_fifo_sz - 1).type != GraphChanged)
//       {
//         // Add a GraphChanged event.
//         JackCallbackEvent ev;
//         ev.type = GraphChanged;
//         jackCallbackFifo.put(ev);
//         // we cannot call JackAudioDevice::graphChanged() from this
//         // context, so we send a message to the gui thread which in turn
//         // calls graphChanged()
//         MusEGlobal::audio->sendMsgToGui('C');
//       }
//       if(jack_ver_maj == 1)
//       {
//         // Jack-2: Call graphChanged() directly.
//         //((JackAudioDevice*)arg)->graphChanged();
//         //((JackAudioDevice*)arg)->processGraphChanges();
//         ((JackAudioDevice*)arg)->processGraphChangeCallback();
//       }
//       else
      {
        // Jack-1: Call graphChanged() via message to GUI.
        // Add a GraphChanged event.
        JackCallbackEvent ev;
        ev.type = GraphChanged;
        jackCallbackFifo.put(ev);
//       }
      }
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

//---------------------------------------------------------
//   JackAudioDevice::connectionsChanged
//    this is called from song in gui context triggered
//    by port_connect_callback()
//---------------------------------------------------------

// void JackAudioDevice::connectionsChanged()
// {
//   if(JACK_DEBUG)
//     printf("JackAudioDevice::connectionsChanged()\n");
// 
//   // REMOVE Tim. Persistent routes. Added.
//   fprintf(stderr, "JackAudioDevice::connectionsChanged()\n");
//   
//   // connectionsChanged() is called just before ::registrationChanged() when a port is unregistered.
//   // Update all the names, from the ring buffers filled by port_connect_callback().
//   //updateAllJackRouteNames();
// }

// REMOVE Tim. Persistent routes. Removed.
// //---------------------------------------------------------
// //   JackAudioDevice::graphChanged
// //    this is called from song in gui context triggered
// //    by graph_callback()
// //---------------------------------------------------------
// 
// void JackAudioDevice::graphChanged()
// {
//       if (JACK_DEBUG)
//             printf("graphChanged()\n");
//       if(!checkJackClient(_client)) return;
//       
//       PendingOperationList operations;
//       
//       InputList* il = MusEGlobal::song->inputs();
//       for (iAudioInput ii = il->begin(); ii != il->end(); ++ii) {
//             AudioInput* it = *ii;
//             int channels = it->channels();
//             for (int channel = 0; channel < channels; ++channel) {
//                   jack_port_t* port = (jack_port_t*)(it->jackPort(channel));
//                   if (port == 0)
//                         continue;
//                   const char** ports = jack_port_get_all_connections(_client, port);
//                   RouteList* rl      = it->inRoutes();
// 
//                   //---------------------------------------
//                   // check for disconnects
//                   //---------------------------------------
// 
//                   //bool erased;
//                   // limit set to 20 iterations for disconnects, don't know how to make it go
//                   // the "right" amount
//                   // REMOVE Tim. Persistent routes. Removed.
//                   //for (int i = 0;i < 20;i++) 
//                   {
//                         //erased = false;
//                         for (ciRoute irl = rl->begin(); irl != rl->end(); ++irl) {
//                               if(irl->type != Route::JACK_ROUTE)  
//                                 continue;
//                               // Persistent Jack routes: Don't remove the route if marked as 'unavailable'.
//                               if(!irl->jackPort)  
//                                 continue;
//                               if (irl->channel != channel)
//                                     continue;
//                               QString name = irl->name();
//                               QByteArray ba = name.toLatin1();
//                               const char* portName = ba.constData();
//                               //printf("portname=%s\n", portName);
//                               bool found = false;
//                               const char** pn = ports;
//                               while (pn && *pn) {
//                                     if (strcmp(*pn, portName) == 0) {
//                                           found = true;
//                                           break;
//                                           }
//                                     ++pn;
//                                     }
//                               if (!found) {
//                                     // REMOVE Tim. Persistent routes. Added.
//                                     Route src(portName, false, channel, Route::JACK_ROUTE);
//                                     Route dst(it, channel);
//                                     operations.add(PendingOperationItem(src, dst, PendingOperationItem::DeleteRoute));
// // REMOVE Tim. Persistent routes. Removed.
// //                                     MusEGlobal::audio->msgRemoveRoute1(
// //                                        //Route(portName, false, channel),
// //                                        Route(portName, false, channel, Route::JACK_ROUTE),
// //                                        Route(it, channel)
// //                                        );
// //                                     erased = true;
// //                                     break;
//                                     }
//                               }
// //                         if (!erased)
// //                               break;
//                         }
// 
//                   //---------------------------------------
//                   // check for connects
//                   //---------------------------------------
// 
//                   if (ports) {
//                         const char** pn = ports;
//                         while (*pn) {
//                               bool found = false;
//                               for (ciRoute irl = rl->begin(); irl != rl->end(); ++irl) {
//                                     if(irl->type != Route::JACK_ROUTE)  
//                                       continue;
//                                     if (irl->channel != channel)
//                                           continue;
//                                     QString name = irl->name();
// 				    QByteArray ba = name.toLatin1();
//                                     const char* portName = ba.constData();
//                                     if (strcmp(*pn, portName) == 0) {
//                                           found = true;
//                                           break;
//                                           }
//                                     }
//                               if (!found) {
//                                     // REMOVE Tim. Persistent routes. Added.
//                                     Route src(*pn, false, channel, Route::JACK_ROUTE);
//                                     Route dst(it, channel);
//                                     operations.add(PendingOperationItem(src, dst, PendingOperationItem::AddRoute));
// // REMOVE Tim. Persistent routes. Removed.
// //                                     MusEGlobal::audio->msgAddRoute1(
// //                                        //Route(*pn, false, channel),
// //                                        Route(*pn, false, channel, Route::JACK_ROUTE),
// //                                        Route(it, channel)
// //                                        );
//                                     }
//                               ++pn;
//                               }
// 
//                         jack_free(ports);  // p4.0.29
//                         
//                         ports = NULL;
//                         }
//                   }
//             }
//             
//             
//       OutputList* ol = MusEGlobal::song->outputs();
//       for (iAudioOutput ii = ol->begin(); ii != ol->end(); ++ii) {
//             AudioOutput* it = *ii;
//             int channels = it->channels();
//             for (int channel = 0; channel < channels; ++channel) {
//                   jack_port_t* port = (jack_port_t*)(it->jackPort(channel));
//                   if (port == 0)
//                         continue;
//                   const char** ports = jack_port_get_all_connections(_client, port);
//                   RouteList* rl      = it->outRoutes();
// 
//                   //---------------------------------------
//                   // check for disconnects
//                   //---------------------------------------
// 
//                   //bool erased;
//                   // limit set to 20 iterations for disconnects, don't know how to make it go
//                   // the "right" amount
//                   // REMOVE Tim. Persistent routes. Removed.
//                   //for (int i = 0; i < 20 ; i++) 
//                   {
//                         //erased = false;
//                         for (ciRoute irl = rl->begin(); irl != rl->end(); ++irl) {
//                               if(irl->type != Route::JACK_ROUTE)  
//                                 continue;
//                               // Persistent Jack routes: Don't remove the route if marked as 'unavailable'.
//                               if(!irl->jackPort)  
//                                 continue;
//                               if (irl->channel != channel)
//                                     continue;
//                               QString name = irl->name();
//                               QByteArray ba = name.toLatin1();
//                               const char* portName = ba.constData();
//                               bool found = false;
//                               const char** pn = ports;
//                               while (pn && *pn) {
//                                     if (strcmp(*pn, portName) == 0) {
//                                           found = true;
//                                           break;
//                                           }
//                                     ++pn;
//                                     }
//                               if (!found) {
//                                     // REMOVE Tim. Persistent routes. Added.
//                                     Route src(it, channel);
//                                     Route dst(portName, false, channel, Route::JACK_ROUTE);
//                                     operations.add(PendingOperationItem(src, dst, PendingOperationItem::DeleteRoute));
// // REMOVE Tim. Persistent routes. Removed.
// //                                     MusEGlobal::audio->msgRemoveRoute1(
// //                                        Route(it, channel),
// //                                        //Route(portName, false, channel)
// //                                        Route(portName, false, channel, Route::JACK_ROUTE)
// //                                        );
// //                                     erased = true;
// //                                     break;
//                                     }
//                               }
// //                         if (!erased)
// //                               break;
//                         }
// 
//                   //---------------------------------------
//                   // check for connects
//                   //---------------------------------------
// 
//                   if (ports) {
//                         const char** pn = ports;
//                         while (*pn) {
//                               bool found = false;
//                               for (ciRoute irl = rl->begin(); irl != rl->end(); ++irl) {
//                                     if(irl->type != Route::JACK_ROUTE)  
//                                       continue;
//                                     if (irl->channel != channel)
//                                           continue;
//                                     QString name = irl->name();
//                                     QByteArray ba = name.toLatin1();
//                                     const char* portName = ba.constData();
//                                     if (strcmp(*pn, portName) == 0) {
//                                           found = true;
//                                           break;
//                                           }
//                                     }
//                               if (!found) {
//                                     // REMOVE Tim. Persistent routes. Added.
//                                     Route src(it, channel);
//                                     Route dst(*pn, false, channel, Route::JACK_ROUTE);
//                                     operations.add(PendingOperationItem(src, dst, PendingOperationItem::AddRoute));
// // REMOVE Tim. Persistent routes. Removed.
// //                                     MusEGlobal::audio->msgAddRoute1(
// //                                        Route(it, channel),
// //                                        //Route(*pn, false, channel)
// //                                        Route(*pn, false, channel, Route::JACK_ROUTE)
// //                                        );
//                                     }
//                               ++pn;
//                               }
// 
//                         jack_free(ports);  // p4.0.29
//                         
//                         ports = NULL;
//                         }
//                   }
//             }
//             
//       for (iMidiDevice ii = MusEGlobal::midiDevices.begin(); ii != MusEGlobal::midiDevices.end(); ++ii) 
//       {
//             MidiDevice* md = *ii;
//             if(md->deviceType() != MidiDevice::JACK_MIDI)
//               continue;
//             
//             //MidiJackDevice* mjd = dynamic_cast<MidiJackDevice*>(*ii);
//             //if(!mjd)
//             //  continue;
//             //for (int channel = 0; channel < channels; ++channel) 
//             //{
//                   
//                   // p3.3.55 Removed
//                   //jack_port_t* port = (jack_port_t*)md->clientPort();
//                   //if (port == 0)
//                   //      continue;
//                   //const char** ports = jack_port_get_all_connections(_client, port);
//                   
//                   //---------------------------------------
//                   // outputs
//                   //---------------------------------------
//                   
//                   if(md->rwFlags() & 1) // Writable
//                   {
//                     jack_port_t* port = (jack_port_t*)md->outClientPort();
//                     if(port != 0)
//                     {
//                       const char** ports = jack_port_get_all_connections(_client, port);
//                       
//                       RouteList* rl      = md->outRoutes();
//     
//                       //---------------------------------------
//                       // check for disconnects
//                       //---------------------------------------
//     
//                       // REMOVE Tim. Persistent routes. Removed.
//                       //bool erased;
//                       // limit set to 20 iterations for disconnects, don't know how to make it go
//                       // the "right" amount
//                       //for (int i = 0; i < 20 ; i++) // REMOVE Tim. Persistent routes. Removed.
//                       //{
//                             //erased = false;
//                             for (ciRoute irl = rl->begin(); irl != rl->end(); ++irl) {
//                                   if(irl->type != Route::JACK_ROUTE)  
//                                     continue;
//                                   // Persistent Jack routes: Don't remove the route if marked as 'unavailable'.
//                                   if(!irl->jackPort)  
//                                     continue;
//                                   //if (irl->channel != channel)
//                                   //      continue;
//                                   QString name = irl->name();
//                                   //name += QString(JACK_MIDI_OUT_PORT_SUFFIX);    
//                                   QByteArray ba = name.toLatin1();
//                                   const char* portName = ba.constData();
//                                   bool found = false;
//                                   const char** pn = ports;
//                                   while (pn && *pn) {
//                                         if (strcmp(*pn, portName) == 0) {
//                                               found = true;
//                                               break;
//                                               }
//                                         ++pn;
//                                         }
//                                   if (!found) {
//                                         // REMOVE Tim. Persistent routes. Added.
//                                         Route src(md, -1);
//                                         Route dst(portName, false, -1, Route::JACK_ROUTE);
//                                         operations.add(PendingOperationItem(src, dst, PendingOperationItem::DeleteRoute));
// // REMOVE Tim. Persistent routes. Removed.
// //                                         MusEGlobal::audio->msgRemoveRoute1(
// //                                           //Route(it, channel),
// //                                           //Route(mjd),
// //                                           Route(md, -1),
// //                                           //Route(portName, false, channel)
// //                                           //Route(portName, false, -1)
// //                                           Route(portName, false, -1, Route::JACK_ROUTE)
// //                                           );
// //                                         erased = true;
// //                                         break;
//                                         }
//                                   }
// //                             if (!erased)
// //                                   break;
//                       //}
// 
//                       
//                       
//                       //---------------------------------------
//                       // check for connects
//                       //---------------------------------------
//     
//                       if (ports) 
//                       {
//                             const char** pn = ports;
//                             while (*pn) {
//                                   bool found = false;
//                                   for (ciRoute irl = rl->begin(); irl != rl->end(); ++irl) {
//                                         if(irl->type != Route::JACK_ROUTE)  
//                                           continue;
//                                         //if (irl->channel != channel)
//                                         //      continue;
//                                         QString name = irl->name();
//                                         QByteArray ba = name.toLatin1();
//                                         const char* portName = ba.constData();
//                                         if (strcmp(*pn, portName) == 0) {
//                                               found = true;
//                                               break;
//                                               }
//                                         }
//                                   if (!found)
//                                   {
//                                         // REMOVE Tim. Persistent routes. Added.
//                                         Route src(md, -1);
//                                         Route dst(*pn, false, -1, Route::JACK_ROUTE);
//                                         operations.add(PendingOperationItem(src, dst, PendingOperationItem::AddRoute));
// // REMOVE Tim. Persistent routes. Removed.
// //                                         MusEGlobal::audio->msgAddRoute1(
// //                                           //Route(it, channel),
// //                                           //Route(mjd),
// //                                           Route(md, -1),
// //                                           //Route(*pn, false, channel)
// //                                           //Route(*pn, false, -1)
// //                                           Route(*pn, false, -1, Route::JACK_ROUTE)
// //                                           );
//                                         }
//                                   ++pn;
//                                   }
//     
//                             jack_free(ports);
//                       }
//                       
//                       
//                      }  
//                    }  
//                   
//                   
//                   //------------------------
//                   // Inputs
//                   //------------------------
//                   
//                   if(md->rwFlags() & 2) // Readable
//                   {
//                     jack_port_t* port = (jack_port_t*)md->inClientPort();
//                     if(port != 0)
//                     {
//                       const char** ports = jack_port_get_all_connections(_client, port);
//                       
//                       RouteList* rl = md->inRoutes();
//     
//                       //---------------------------------------
//                       // check for disconnects
//                       //---------------------------------------
//     
// // REMOVE Tim. Persistent routes. Removed.
//                       //bool erased;
//                       // limit set to 20 iterations for disconnects, don't know how to make it go
//                       // the "right" amount
//                       //for (int i = 0; i < 20 ; i++) // REMOVE Tim. Persistent routes. Removed.
//                       //{
//                             //erased = false;
//                             for (ciRoute irl = rl->begin(); irl != rl->end(); ++irl) {
//                                   if(irl->type != Route::JACK_ROUTE)  
//                                     continue;
//                                   // Persistent Jack routes: Don't remove the route if marked as 'unavailable'.
//                                   if(!irl->jackPort)  
//                                     continue;
//                                   //if (irl->channel != channel)
//                                   //      continue;
//                                   QString name = irl->name();
//                                   QByteArray ba = name.toLatin1();
//                                   const char* portName = ba.constData();
//                                   bool found = false;
//                                   const char** pn = ports;
//                                   while (pn && *pn) {
//                                         if (strcmp(*pn, portName) == 0) {
//                                               found = true;
//                                               break;
//                                               }
//                                         ++pn;
//                                         }
//                                   if (!found) {
//                                         // REMOVE Tim. Persistent routes. Added.
//                                         Route src(portName, false, -1, Route::JACK_ROUTE);
//                                         Route dst(md, -1);
//                                         operations.add(PendingOperationItem(src, dst, PendingOperationItem::DeleteRoute));
// // REMOVE Tim. Persistent routes. Removed.
// //                                         MusEGlobal::audio->msgRemoveRoute1(
// //                                           //Route(portName, false, channel),
// //                                           //Route(portName, false, -1),
// //                                           Route(portName, false, -1, Route::JACK_ROUTE),
// //                                           //Route(it, channel)
// //                                           //Route(mjd)
// //                                           Route(md, -1)
// //                                           );
// //                                         erased = true;
// //                                         break;
//                                         }
//                                   }
// //                             if (!erased)
// //                                   break;
//                       //}
// 
//                       
//                       
//                       //---------------------------------------
//                       // check for connects
//                       //---------------------------------------
//     
//                       if (ports) 
//                       {
//                             const char** pn = ports;
//                             while (*pn) {
//                                   bool found = false;
//                                   for (ciRoute irl = rl->begin(); irl != rl->end(); ++irl) {
//                                         if(irl->type != Route::JACK_ROUTE)  
//                                           continue;
//                                         //if (irl->channel != channel)
//                                         //      continue;
//                                         QString name = irl->name();
//                                         QByteArray ba = name.toLatin1();
//                                         const char* portName = ba.constData();
//                                         if (strcmp(*pn, portName) == 0) {
//                                               found = true;
//                                               break;
//                                               }
//                                         }
//                                   if (!found) {
//                                         // REMOVE Tim. Persistent routes. Added.
//                                         Route src(*pn, false, -1, Route::JACK_ROUTE);
//                                         Route dst(md, -1);
//                                         operations.add(PendingOperationItem(src, dst, PendingOperationItem::AddRoute));
// // REMOVE Tim. Persistent routes. Removed.
// //                                         MusEGlobal::audio->msgAddRoute1(
// //                                           //Route(*pn, false, channel),
// //                                           //Route(*pn, false, -1),
// //                                           Route(*pn, false, -1, Route::JACK_ROUTE),
// //                                           //Route(it, channel)
// //                                           //Route(mjd)
// //                                           Route(md, -1)
// //                                           );
//                                         }
//                                   ++pn;
//                                   }
//                                   
//                             jack_free(ports);
//                       }
//                     }  
//                   }  
//       }
//       
//       if(!operations.empty())
//         MusEGlobal::audio->msgExecutePendingOperations(operations);
// }



// //---------------------------------------------------------
// //   JackAudioDevice::graphChanged
// //    this is called from song in gui context triggered
// //    by graph_callback()
// //---------------------------------------------------------
// 
// void JackAudioDevice::graphChanged()
// {
//   if (JACK_DEBUG)
//         printf("graphChanged()\n");
//   if(!checkJackClient(_client))
//   {
//     callbackFifo.clear(); // TODO: Want this?
//     return;
//   }
// 
//   // Find the last GraphChanged event, if any.
//   int last_gc_idx = -1;
//   int cb_fifo_sz = callbackFifo.getSize();
//   for(int i = 0; i < cb_fifo_sz; ++i)
//   {
//     if(callbackFifo.peek(i).type == GraphChanged)
//       last_gc_idx = i;
//   }
//   if(last_gc_idx == -1)
//     return;
// 
//   // Move the events into a list for processing.
//   // Leave any 'still in progress' ending events (without closing GraphChanged event) in the ring buffer.
//   std::list<JackCallbackEvent> jce_list;
//   typedef std::list<JackCallbackEvent>::iterator iJackCallbackEvent;
//   for(int i = 0; i < last_gc_idx; ++i)
//     jce_list.push_back(callbackFifo.get());
//   
//   PendingOperationList operations;
//   
//   for(iJackCallbackEvent ijce = jce_list.begin(); ijce != jce_list.end(); ++ijce)
//   {
//     // Ignore the actual GraphChanged events.
//     if(ijce->type == GraphChanged)
//       continue;
//     
//     // See if PortDisconnect is followed by PortUnregister, before the next GraphChanged event.
//     if(ijce->type == PortDisconnect)
//     {
//       bool found = false;
//       iJackCallbackEvent ifl = ijce;
//       ++ifl;
//       for( ; ifl != jce_list.end(); ++ifl)
//       {
//         // Stop looking on the next GraphChanged event.
//         if(ijce->type == GraphChanged)
//           break;
//         // If we find a PortDisconnect followed by a PortUnregister, copy the port name to the
//         //  PortUnregister event, to make it easier to deal with during list enumeration below.
//         if(ifl->type == PortUnregister && (ifl->port_id_A == ijce->port_id_A || ifl->port_id_A == ijce->port_id_B))
//         {
//           if(ifl->port_id_A == ijce->port_id_A)
//             ifl->name_A = ijce->name_A;
//           if(ifl->port_id_A == ijce->port_id_B)
//             ifl->name_A = ijce->name_B;
//           found = true;
//         }
//       }
//       // If we found the corresponding PortUnregister, IGNORE the PortDisconnect.
//       if(found)
//         continue;  
//     }
// 
//     // See if PortConnect is followed by PortDisconnect anywhere.
//     if(ijce->type == PortConnect)
//     {
//       bool found = false;
//       iJackCallbackEvent ifl = ijce;
//       ++ifl;
//       for( ; ifl != jce_list.end(); ++ifl)
//       {
//         // If we find a PortConnect followed by a PortDisconnect, copy the port name to the
//         //  PortUnregister event, to make it easier to deal with during list enumeration below.
//         if(ifl->type == PortDisconnect && (ifl->port_id_A == ijce->port_id_A && ifl->port_id_B == ijce->port_id_B))
//         {
//           if(ifl->port_id_A == ijce->port_id_A)
//             ifl->name_A = ijce->name_A;
//           if(ifl->port_id_A == ijce->port_id_B)
//             ifl->name_A = ijce->name_B;
//           found = true;
//         }
//       }
//       // If we found the corresponding PortUnregister, IGNORE the PortDisconnect.
//       if(found)
//         continue;  
//     }
//     
//     //---------------------------------------
//     // Audio inputs:
//     //---------------------------------------
//     
//     InputList* il = MusEGlobal::song->inputs();
//     for (iAudioInput ii = il->begin(); ii != il->end(); ++ii) {
//           AudioInput* it = *ii;
//           int channels = it->channels();
//           for (int channel = 0; channel < channels; ++channel) {
//                 jack_port_t* port = (jack_port_t*)(it->jackPort(channel));
//                 if (port == 0)
//                       continue;
//                 const char** ports = jack_port_get_all_connections(_client, port);
//                 RouteList* rl      = it->inRoutes();
// 
//                 //---------------------------------------
//                 // check for disconnects
//                 //---------------------------------------
// 
//                 //bool erased;
//                 // limit set to 20 iterations for disconnects, don't know how to make it go
//                 // the "right" amount
//                 // REMOVE Tim. Persistent routes. Removed.
//                 //for (int i = 0;i < 20;i++) 
//                 //{
//                       //erased = false;
//                       bool found = false;
//                       for (ciRoute irl = rl->begin(); irl != rl->end(); ++irl) 
//                       {
//                             if(irl->type != Route::JACK_ROUTE)  
//                               continue;
//                             // Persistent Jack routes: Don't remove the route if marked as 'unavailable'.
//                             if(!irl->jackPort)  
//                               continue;
//                             if (irl->channel != channel)
//                                   continue;
// 
// 
//                             // See if PortDisconnect is followed by PortUnregister, before the next GraphChanged event.
//                             // REMOVE Tim. Persistent routes. Added.
//                             switch(ijce->type)
//                             {
//                               case PortConnect:
//                               {
//                                 jack_port_t* j_port_A = jack_port_by_id(ijce->port_id_A);
//                                 jack_port_t* j_port_B = jack_port_by_id(ijce->port_id_B);
//                                 if((irl->jackPort == j_port_A && port == j_port_B) || (irl->jackPort == j_port_B && port == j_port_A))
//                                 {
//                                   found = true;
//                                   // TODO: Break out of irl loop
//                                   // break;
//                                 }
//                               }
//                               break;
// 
//                               case PortDisconnect:
//                               {
//                                 jack_port_t* j_port_A = jack_port_by_id(ijce->port_id_A);
//                                 jack_port_t* j_port_B = jack_port_by_id(ijce->port_id_B);
//                                 jack_port_id_t j_port_id = 0;
//                                 QString p_name;
//                                 if(irl->jackPort == j_port_A && port == j_port_B)
//                                 {
//                                   j_port_id = ijce->port_id_A;
//                                   p_name = ijce->name_A;
//                                 }
//                                 else if(irl->jackPort == j_port_B && port == j_port_A)
//                                 {
//                                   j_port_id = ijce->port_id_B;
//                                   p_name = ijce->name_B;
//                                 }
//                                 if(j_port_id != 0)
//                                 {
//                                   // If BOTH ports (port and irl->jackPort) are owned by us, don't do 
//                                   //  any persistent stuff. We don't want to autoconnect to our own ports.
//                                   // TODO: Maybe ONLY autoconnect to hardware ports, not other apps' ports?
//                                   if(!jack_port_is_mine(_client, irl->jackPort))
//                                   {
//                                     bool pu_found = false;
//                                     iJackCallbackEvent ifl = ijce;
//                                     ++ifl;
//                                     for( ; ifl != jce_list.end(); ++ifl)
//                                     {
//                                       // Stop looking on the next GraphChanged event.
//                                       if(ijce->type == GraphChanged)
//                                         break;
//                                       // If we find a PortDisconnect followed by a PortUnregister, copy the port name to the
//                                       //  PortUnregister event, to make it easier to deal with during list enumeration.
//                                       if(ifl->type == PortUnregister && ifl->port_id_A == j_port_id)
//                                       {
//                                         ifl->name_A = p_name;
//                                         pu_found = true;
//                                       }
//                                     }
//                                     // If we found a corresponding PortUnregister, IGNORE the PortDisconnect.
//                                     if(pu_found)
//                                       continue; // Next irl.
//                                   }
//                                   Route dst(it, channel);
//                                   operations.add(PendingOperationItem(*irl, dst, PendingOperationItem::DeleteRoute));
//                                 }
//                               }
//                               break;
//                               
//                               case PortRegister:
//                                 if(irl->jackPort == ijce->port_id_A)
//                                 {
//                                   found = true;
//                                 }
//                               break;
// 
//                               case PortUnregister:
//                                 if(irl->jackPort == ijce->port_id_A)
//                                 {
//                                   found = true;
//                                 }
//                               break;
// 
//                               case GraphChanged:
//                               break;
//                             }
// 
// 
// 
// 
// 
// 
//                             
//                             // REMOVE Tim. Persistent routes. Added.
//                             switch(ijce->type)
//                             {
//                               case PortConnect:
//                                 if(irl->jackPort == ijce->port_id_A || irl->jackPort == ijce->port_id_B)
//                                   found = true;
//                               break;
// 
//                               case PortDisconnect:
//                                 if(irl->jackPort == ijce->port_id_A || irl->jackPort == ijce->port_id_B)
//                                 {
//                                   Route dst(it, channel);
//                                   found = true;
//                                   operations.add(PendingOperationItem(*irl, dst, PendingOperationItem::DeleteRoute));
//                                 }
//                               break;
// 
//                               case PortRegister:
//                                 if(irl->jackPort == ijce->port_id_A)
//                                 {
//                                   found = true;
//                                 }
//                               break;
// 
//                               case PortUnregister:
//                                 if(irl->jackPort == ijce->port_id_A)
//                                 {
//                                   found = true;
//                                 }
//                               break;
// 
//                               case GraphChanged:
//                               break;
//                             }
//                       }      
//                             
//                             // REMOVE Tim. Persistent routes. Removed.
// //                       for (ciRoute irl = rl->begin(); irl != rl->end(); ++irl) 
// //                       {
// //                             if(irl->type != Route::JACK_ROUTE)  
// //                               continue;
// //                             // Persistent Jack routes: Don't remove the route if marked as 'unavailable'.
// //                             if(!irl->jackPort)  
// //                               continue;
// //                             if (irl->channel != channel)
// //                                   continue;
// //                             QString name = irl->name();
// //                             QByteArray ba = name.toLatin1();
// //                             const char* portName = ba.constData();
// //                             //printf("portname=%s\n", portName);
// //                             bool found = false;
// //                             const char** pn = ports;
// //                             while (pn && *pn) {
// //                                   if (strcmp(*pn, portName) == 0) {
// //                                         found = true;
// //                                         break;
// //                                         }
// //                                   ++pn;
// //                                   }
// //                             if (!found) {
// //                                   // REMOVE Tim. Persistent routes. Added.
// //                                   Route src(portName, false, channel, Route::JACK_ROUTE);
// //                                   Route dst(it, channel);
// //                                   operations.add(PendingOperationItem(src, dst, PendingOperationItem::DeleteRoute));
// // // REMOVE Tim. Persistent routes. Removed.
// // //                                     MusEGlobal::audio->msgRemoveRoute1(
// // //                                        //Route(portName, false, channel),
// // //                                        Route(portName, false, channel, Route::JACK_ROUTE),
// // //                                        Route(it, channel)
// // //                                        );
// // //                                     erased = true;
// // //                                     break;
// //                                   }
// //                             }
// // //                         if (!erased)
// // //                               break;
// //                      }
// 
//                 //---------------------------------------
//                 // check for connects
//                 //---------------------------------------
// 
//                 
//                 
//                 
//                 // REMOVE Tim. Persistent routes. Removed.
// //                 if (ports) {
// //                       const char** pn = ports;
// //                       while (*pn) {
// //                             bool found = false;
// //                             for (ciRoute irl = rl->begin(); irl != rl->end(); ++irl) {
// //                                   if(irl->type != Route::JACK_ROUTE)  
// //                                     continue;
// //                                   if (irl->channel != channel)
// //                                         continue;
// //                                   QString name = irl->name();
// //                                   QByteArray ba = name.toLatin1();
// //                                   const char* portName = ba.constData();
// //                                   if (strcmp(*pn, portName) == 0) {
// //                                         found = true;
// //                                         break;
// //                                         }
// //                                   }
// //                             if (!found) {
// //                                   // REMOVE Tim. Persistent routes. Added.
// //                                   Route src(*pn, false, channel, Route::JACK_ROUTE);
// //                                   Route dst(it, channel);
// //                                   operations.add(PendingOperationItem(src, dst, PendingOperationItem::AddRoute));
// // // REMOVE Tim. Persistent routes. Removed.
// // //                                     MusEGlobal::audio->msgAddRoute1(
// // //                                        //Route(*pn, false, channel),
// // //                                        Route(*pn, false, channel, Route::JACK_ROUTE),
// // //                                        Route(it, channel)
// // //                                        );
// //                                   }
// //                             ++pn;
// //                             }
// // 
// //                       jack_free(ports);  // p4.0.29
// //                       
// //                       ports = NULL;
// //                       
// //                       }
//                       
//                       
// 
//                       // REMOVE Tim. Persistent routes. Added.
//                       switch(ijce->type)
//                       {
//                         case PortConnect:
//                           if(!found) 
//                           {
//                             // REMOVE Tim. Persistent routes. Added.
//                             Route src(*pn, false, channel, Route::JACK_ROUTE);
//                             Route dst(it, channel);
//                             operations.add(PendingOperationItem(src, dst, PendingOperationItem::AddRoute));
//                           }
//                         break;
//                         
//                         case PortDisconnect:
//                         break;
//                         
//                         case PortRegister:
//                         break;
//                         
//                         case PortUnregister:
//                         break;
//                         
//                         case GraphChanged:
//                         break;
//                       }
//                             
//                       
//                       
//                       
//                 
//                       if(!found) 
//                       {
//                         // REMOVE Tim. Persistent routes. Added.
//                         Route src(*pn, false, channel, Route::JACK_ROUTE);
//                         Route dst(it, channel);
//                         operations.add(PendingOperationItem(src, dst, PendingOperationItem::AddRoute));
//                       }
//                 
//                       
//                       
//                       
//                       
//                 }
//           }
//           
//           
//     //---------------------------------------
//     // Audio outputs:
//     //---------------------------------------
//     
//     OutputList* ol = MusEGlobal::song->outputs();
//     for (iAudioOutput ii = ol->begin(); ii != ol->end(); ++ii) {
//           AudioOutput* it = *ii;
//           int channels = it->channels();
//           for (int channel = 0; channel < channels; ++channel) {
//                 jack_port_t* port = (jack_port_t*)(it->jackPort(channel));
//                 if (port == 0)
//                       continue;
//                 const char** ports = jack_port_get_all_connections(_client, port);
//                 RouteList* rl      = it->outRoutes();
// 
//                 //---------------------------------------
//                 // check for disconnects
//                 //---------------------------------------
// 
//                 //bool erased;
//                 // limit set to 20 iterations for disconnects, don't know how to make it go
//                 // the "right" amount
//                 // REMOVE Tim. Persistent routes. Removed.
//                 //for (int i = 0; i < 20 ; i++) 
//                 {
//                       //erased = false;
//                       for (ciRoute irl = rl->begin(); irl != rl->end(); ++irl) {
//                             if(irl->type != Route::JACK_ROUTE)  
//                               continue;
//                             // Persistent Jack routes: Don't remove the route if marked as 'unavailable'.
//                             if(!irl->jackPort)  
//                               continue;
//                             if (irl->channel != channel)
//                                   continue;
//                             QString name = irl->name();
//                             QByteArray ba = name.toLatin1();
//                             const char* portName = ba.constData();
//                             bool found = false;
//                             const char** pn = ports;
//                             while (pn && *pn) {
//                                   if (strcmp(*pn, portName) == 0) {
//                                         found = true;
//                                         break;
//                                         }
//                                   ++pn;
//                                   }
//                             if (!found) {
//                                   // REMOVE Tim. Persistent routes. Added.
//                                   Route src(it, channel);
//                                   Route dst(portName, false, channel, Route::JACK_ROUTE);
//                                   operations.add(PendingOperationItem(src, dst, PendingOperationItem::DeleteRoute));
// // REMOVE Tim. Persistent routes. Removed.
// //                                     MusEGlobal::audio->msgRemoveRoute1(
// //                                        Route(it, channel),
// //                                        //Route(portName, false, channel)
// //                                        Route(portName, false, channel, Route::JACK_ROUTE)
// //                                        );
// //                                     erased = true;
// //                                     break;
//                                   }
//                             }
// //                         if (!erased)
// //                               break;
//                       }
// 
//                 //---------------------------------------
//                 // check for connects
//                 //---------------------------------------
// 
//                 if (ports) {
//                       const char** pn = ports;
//                       while (*pn) {
//                             bool found = false;
//                             for (ciRoute irl = rl->begin(); irl != rl->end(); ++irl) {
//                                   if(irl->type != Route::JACK_ROUTE)  
//                                     continue;
//                                   if (irl->channel != channel)
//                                         continue;
//                                   QString name = irl->name();
//                                   QByteArray ba = name.toLatin1();
//                                   const char* portName = ba.constData();
//                                   if (strcmp(*pn, portName) == 0) {
//                                         found = true;
//                                         break;
//                                         }
//                                   }
//                             if (!found) {
//                                   // REMOVE Tim. Persistent routes. Added.
//                                   Route src(it, channel);
//                                   Route dst(*pn, false, channel, Route::JACK_ROUTE);
//                                   operations.add(PendingOperationItem(src, dst, PendingOperationItem::AddRoute));
// // REMOVE Tim. Persistent routes. Removed.
// //                                     MusEGlobal::audio->msgAddRoute1(
// //                                        Route(it, channel),
// //                                        //Route(*pn, false, channel)
// //                                        Route(*pn, false, channel, Route::JACK_ROUTE)
// //                                        );
//                                   }
//                             ++pn;
//                             }
// 
//                       jack_free(ports);  // p4.0.29
//                       
//                       ports = NULL;
//                       }
//                 }
//           }
//           
//     //---------------------------------------
//     // Midi devices:
//     //---------------------------------------
//     
//     for (iMidiDevice ii = MusEGlobal::midiDevices.begin(); ii != MusEGlobal::midiDevices.end(); ++ii) 
//     {
//           MidiDevice* md = *ii;
//           if(md->deviceType() != MidiDevice::JACK_MIDI)
//             continue;
//           
//           //MidiJackDevice* mjd = dynamic_cast<MidiJackDevice*>(*ii);
//           //if(!mjd)
//           //  continue;
//           //for (int channel = 0; channel < channels; ++channel) 
//           //{
//                 
//                 // p3.3.55 Removed
//                 //jack_port_t* port = (jack_port_t*)md->clientPort();
//                 //if (port == 0)
//                 //      continue;
//                 //const char** ports = jack_port_get_all_connections(_client, port);
//                 
//                 //---------------------------------------
//                 // Midi outputs:
//                 //---------------------------------------
//                 
//                 if(md->rwFlags() & 1) // Writable
//                 {
//                   jack_port_t* port = (jack_port_t*)md->outClientPort();
//                   if(port != 0)
//                   {
//                     const char** ports = jack_port_get_all_connections(_client, port);
//                     
//                     RouteList* rl      = md->outRoutes();
//   
//                     //---------------------------------------
//                     // check for disconnects
//                     //---------------------------------------
//   
//                     // REMOVE Tim. Persistent routes. Removed.
//                     //bool erased;
//                     // limit set to 20 iterations for disconnects, don't know how to make it go
//                     // the "right" amount
//                     //for (int i = 0; i < 20 ; i++) // REMOVE Tim. Persistent routes. Removed.
//                     //{
//                           //erased = false;
//                           for (ciRoute irl = rl->begin(); irl != rl->end(); ++irl) {
//                                 if(irl->type != Route::JACK_ROUTE)  
//                                   continue;
//                                 // Persistent Jack routes: Don't remove the route if marked as 'unavailable'.
//                                 if(!irl->jackPort)  
//                                   continue;
//                                 //if (irl->channel != channel)
//                                 //      continue;
//                                 QString name = irl->name();
//                                 //name += QString(JACK_MIDI_OUT_PORT_SUFFIX);    
//                                 QByteArray ba = name.toLatin1();
//                                 const char* portName = ba.constData();
//                                 bool found = false;
//                                 const char** pn = ports;
//                                 while (pn && *pn) {
//                                       if (strcmp(*pn, portName) == 0) {
//                                             found = true;
//                                             break;
//                                             }
//                                       ++pn;
//                                       }
//                                 if (!found) {
//                                       // REMOVE Tim. Persistent routes. Added.
//                                       Route src(md, -1);
//                                       Route dst(portName, false, -1, Route::JACK_ROUTE);
//                                       operations.add(PendingOperationItem(src, dst, PendingOperationItem::DeleteRoute));
// // REMOVE Tim. Persistent routes. Removed.
// //                                         MusEGlobal::audio->msgRemoveRoute1(
// //                                           //Route(it, channel),
// //                                           //Route(mjd),
// //                                           Route(md, -1),
// //                                           //Route(portName, false, channel)
// //                                           //Route(portName, false, -1)
// //                                           Route(portName, false, -1, Route::JACK_ROUTE)
// //                                           );
// //                                         erased = true;
// //                                         break;
//                                       }
//                                 }
// //                             if (!erased)
// //                                   break;
//                     //}
// 
//                     
//                     
//                     //---------------------------------------
//                     // check for connects
//                     //---------------------------------------
//   
//                     if (ports) 
//                     {
//                           const char** pn = ports;
//                           while (*pn) {
//                                 bool found = false;
//                                 for (ciRoute irl = rl->begin(); irl != rl->end(); ++irl) {
//                                       if(irl->type != Route::JACK_ROUTE)  
//                                         continue;
//                                       //if (irl->channel != channel)
//                                       //      continue;
//                                       QString name = irl->name();
//                                       QByteArray ba = name.toLatin1();
//                                       const char* portName = ba.constData();
//                                       if (strcmp(*pn, portName) == 0) {
//                                             found = true;
//                                             break;
//                                             }
//                                       }
//                                 if (!found)
//                                 {
//                                       // REMOVE Tim. Persistent routes. Added.
//                                       Route src(md, -1);
//                                       Route dst(*pn, false, -1, Route::JACK_ROUTE);
//                                       operations.add(PendingOperationItem(src, dst, PendingOperationItem::AddRoute));
// // REMOVE Tim. Persistent routes. Removed.
// //                                         MusEGlobal::audio->msgAddRoute1(
// //                                           //Route(it, channel),
// //                                           //Route(mjd),
// //                                           Route(md, -1),
// //                                           //Route(*pn, false, channel)
// //                                           //Route(*pn, false, -1)
// //                                           Route(*pn, false, -1, Route::JACK_ROUTE)
// //                                           );
//                                       }
//                                 ++pn;
//                                 }
//   
//                           jack_free(ports);
//                     }
//                     
//                     
//                     }  
//                   }  
//                 
//                 
//                 //------------------------
//                 // Midi inputs:
//                 //------------------------
//                 
//                 if(md->rwFlags() & 2) // Readable
//                 {
//                   jack_port_t* port = (jack_port_t*)md->inClientPort();
//                   if(port != 0)
//                   {
//                     const char** ports = jack_port_get_all_connections(_client, port);
//                     
//                     RouteList* rl = md->inRoutes();
//   
//                     //---------------------------------------
//                     // check for disconnects
//                     //---------------------------------------
//   
// // REMOVE Tim. Persistent routes. Removed.
//                     //bool erased;
//                     // limit set to 20 iterations for disconnects, don't know how to make it go
//                     // the "right" amount
//                     //for (int i = 0; i < 20 ; i++) // REMOVE Tim. Persistent routes. Removed.
//                     //{
//                           //erased = false;
//                           for (ciRoute irl = rl->begin(); irl != rl->end(); ++irl) {
//                                 if(irl->type != Route::JACK_ROUTE)  
//                                   continue;
//                                 // Persistent Jack routes: Don't remove the route if marked as 'unavailable'.
//                                 if(!irl->jackPort)  
//                                   continue;
//                                 //if (irl->channel != channel)
//                                 //      continue;
//                                 QString name = irl->name();
//                                 QByteArray ba = name.toLatin1();
//                                 const char* portName = ba.constData();
//                                 bool found = false;
//                                 const char** pn = ports;
//                                 while (pn && *pn) {
//                                       if (strcmp(*pn, portName) == 0) {
//                                             found = true;
//                                             break;
//                                             }
//                                       ++pn;
//                                       }
//                                 if (!found) {
//                                       // REMOVE Tim. Persistent routes. Added.
//                                       Route src(portName, false, -1, Route::JACK_ROUTE);
//                                       Route dst(md, -1);
//                                       operations.add(PendingOperationItem(src, dst, PendingOperationItem::DeleteRoute));
// // REMOVE Tim. Persistent routes. Removed.
// //                                         MusEGlobal::audio->msgRemoveRoute1(
// //                                           //Route(portName, false, channel),
// //                                           //Route(portName, false, -1),
// //                                           Route(portName, false, -1, Route::JACK_ROUTE),
// //                                           //Route(it, channel)
// //                                           //Route(mjd)
// //                                           Route(md, -1)
// //                                           );
// //                                         erased = true;
// //                                         break;
//                                       }
//                                 }
// //                             if (!erased)
// //                                   break;
//                     //}
// 
//                     
//                     
//                     //---------------------------------------
//                     // check for connects
//                     //---------------------------------------
//   
//                     if (ports) 
//                     {
//                           const char** pn = ports;
//                           while (*pn) {
//                                 bool found = false;
//                                 for (ciRoute irl = rl->begin(); irl != rl->end(); ++irl) {
//                                       if(irl->type != Route::JACK_ROUTE)  
//                                         continue;
//                                       //if (irl->channel != channel)
//                                       //      continue;
//                                       QString name = irl->name();
//                                       QByteArray ba = name.toLatin1();
//                                       const char* portName = ba.constData();
//                                       if (strcmp(*pn, portName) == 0) {
//                                             found = true;
//                                             break;
//                                             }
//                                       }
//                                 if (!found) {
//                                       // REMOVE Tim. Persistent routes. Added.
//                                       Route src(*pn, false, -1, Route::JACK_ROUTE);
//                                       Route dst(md, -1);
//                                       operations.add(PendingOperationItem(src, dst, PendingOperationItem::AddRoute));
// // REMOVE Tim. Persistent routes. Removed.
// //                                         MusEGlobal::audio->msgAddRoute1(
// //                                           //Route(*pn, false, channel),
// //                                           //Route(*pn, false, -1),
// //                                           Route(*pn, false, -1, Route::JACK_ROUTE),
// //                                           //Route(it, channel)
// //                                           //Route(mjd)
// //                                           Route(md, -1)
// //                                           );
//                                       }
//                                 ++pn;
//                                 }
//                                 
//                           jack_free(ports);
//                     }
//                   }  
//                 }  
//     }
//       
//       
//   }
//       
//   if(!operations.empty())
//     MusEGlobal::audio->msgExecutePendingOperations(operations);
// }
// 

//                     // REMOVE Tim. Persistent routes. Removed.
//                     //bool erased;
//                     // limit set to 20 iterations for disconnects, don't know how to make it go
//                     // the "right" amount
//                     //for (int i = 0; i < 20 ; i++) // REMOVE Tim. Persistent routes. Removed.
//                     //{
//                           //erased = false;
//                           for (ciRoute irl = rl->begin(); irl != rl->end(); ++irl) {
//                                 if(irl->type != Route::JACK_ROUTE)  
//                                   continue;
//                                 // Persistent Jack routes: Don't remove the route if marked as 'unavailable'.
//                                 //if(!irl->jackPort)  
//                                 //  continue;
//                                 //if (irl->channel != channel)
//                                 //      continue;
//                   processJackCallbackEvents(_client, jce_list, *irl, port, Route(md, -1), true, operations);
//                                 QString name = irl->name();
//                                 //name += QString(JACK_MIDI_OUT_PORT_SUFFIX);    
//                                 QByteArray ba = name.toLatin1();
//                                 const char* portName = ba.constData();
//                                 bool found = false;
//                                 const char** pn = ports;
//                                 while (pn && *pn) {
//                                       if (strcmp(*pn, portName) == 0) {
//                                             found = true;
//                                             break;
//                                             }
//                                       ++pn;
//                                       }
//                                 if (!found) {
//                                       // REMOVE Tim. Persistent routes. Added.
//                                       Route src(md, -1);
//                                       Route dst(portName, false, -1, Route::JACK_ROUTE);
//                                       operations.add(PendingOperationItem(src, dst, PendingOperationItem::DeleteRoute));
// // REMOVE Tim. Persistent routes. Removed.
// //                                         MusEGlobal::audio->msgRemoveRoute1(
// //                                           //Route(it, channel),
// //                                           //Route(mjd),
// //                                           Route(md, -1),
// //                                           //Route(portName, false, channel)
// //                                           //Route(portName, false, -1)
// //                                           Route(portName, false, -1, Route::JACK_ROUTE)
// //                                           );
// //                                         erased = true;
// //                                         break;
//                                       }
//                                 }
// //                             if (!erased)
// //                                   break;
//                     //}
// 
//                     








                  
                  
                  
//                   jack_port_id_t jpid = 0;
//                   jack_port_t* jp = irl->jackPort;
//                   QString jpname = irl->persistentJackPortName;
//                   CallbackState init_state = irl->jackPort ? CONNECTED : PERSISTENT_UNREGISTERED;
//                   CallbackState state = init_state;
//                   
//                   for(iJackCallbackEvent ijce = jce_list.begin(); ijce != jce_list.end(); ++ijce)
//                   {
//                     switch(state)
//                     {
//                       case DISCONNECTED:
//                         switch(ijce->type)
//                         {
//                           case PortConnect:
//                             if(ijce->port_A == jp && ijce->port_B == port)
//                             {
//                               jpid = ijce->port_id_A;
//                               jpname = ijce->name_A;
//                               state = CONNECTED;
//                             }
//                             else if(ijce->port_B == jp && ijce->port_A == port)
//                             {
//                               jpid = ijce->port_id_B;
//                               jpname = ijce->name_B;
//                               state = CONNECTED;
//                             }
//                           break;
//                           
//                           case PortUnregister:
//                             if(ijce->port_id_A == jpid)
//                             {
//                               jpid = 0;
//                               jp = 0;
//                               state = UNREGISTERED;
//                             }
//                           break;
// 
//                           case PortDisconnect:
//                             if((ijce->port_A == jp && ijce->port_B == port) || (ijce->port_B == jp && ijce->port_A == port))
//                             {
//                               fprintf(stderr, "ERROR: JackAudioDevice::graphChanged state:DISCONNECTED command:PortDisconnect\n");
//                             }
//                           break;
// 
//                           case PortRegister:
//                             if(ijce->name_A == jpname)
//                             {
//                               fprintf(stderr, "ERROR: JackAudioDevice::graphChanged state:DISCONNECTED command:PortRegister\n");
//                             }
//                           break;
// 
//                           default:
//                           break;
//                         }
//                       break;
//                       
//                       case DISCONNECTED_PENDING:
//                         switch(ijce->type)
//                         {
//                           case GraphChanged:
//                             state = DISCONNECTED;
//                           break;
// 
//                           case PortConnect:
//                             if(ijce->port_A == jp && ijce->port_B == port)
//                             {
//                               jpid = ijce->port_id_A;
//                               jpname = ijce->name_A;
//                               state = CONNECTED;
//                             }
//                             else if(ijce->port_B == jp && ijce->port_A == port)
//                             {
//                               jpid = ijce->port_id_B;
//                               jpname = ijce->name_B;
//                               state = CONNECTED;
//                             }
//                           break;
// 
//                           case PortUnregister:
//                             if(ijce->port_id_A == jpid)
//                             {
//                               jpid = 0;
//                               jp = 0;
//                               state = PERSISTENT_UNREGISTERED;
//                             }
//                           break;
// 
//                           case PortDisconnect:
//                             if((ijce->port_A == jp && ijce->port_B == port) || (ijce->port_B == jp && ijce->port_A == port))
//                             {
//                               fprintf(stderr, "ERROR: JackAudioDevice::graphChanged state:DISCONNECTED_PENDING command:PortDisconnect\n");
//                             }
//                           break;
// 
//                           case PortRegister:
//                             if(ijce->name_A == jpname)
//                             {
//                               fprintf(stderr, "ERROR: JackAudioDevice::graphChanged state:DISCONNECTED_PENDING command:PortRegister\n");
//                             }
//                           break;
// 
//                           default:
//                           break;
//                         }
//                       break;
//                       
//                       case CONNECTED:
//                         switch(ijce->type)
//                         {
//                           case PortDisconnect:
//                             if(ijce->port_A == jp && ijce->port_B == port)
//                             {
//                               jpid = ijce->port_id_A;
//                               jpname = ijce->name_A;
//                               state = DISCONNECTED_PENDING;
//                             }
//                             else if(ijce->port_B == jp && ijce->port_A == port)
//                             {
//                               jpid = ijce->port_id_B;
//                               jpname = ijce->name_B;
//                               state = DISCONNECTED_PENDING;
//                             }
//                           break;
// 
//                           case PortConnect:
//                             if((ijce->port_A == jp && ijce->port_B == port) || (ijce->port_B == jp && ijce->port_A == port))
//                             {
//                               fprintf(stderr, "ERROR: JackAudioDevice::graphChanged state:CONNECTED command:PortConnect\n");
//                             }
//                           break;
//                           
//                           //case PortUnregister:
//                           //  if(ijce->name_A == jpname)
//                           //  {
//                           //    fprintf(stderr, "ERROR: JackAudioDevice::graphChanged state:CONNECTED command:PortUnregister\n");
//                           //  }
//                           //break;
//                           
//                           case PortRegister:
//                             if(ijce->port_A == jp)
//                             {
//                               fprintf(stderr, "ERROR: JackAudioDevice::graphChanged state:CONNECTED command:PortRegister\n");
//                             }
//                           break;
//                           
//                           default:
//                           break;
//                         }
//                       break;
//                       
//                       case PERSISTENT_UNREGISTERED:
//                         switch(ijce->type)
//                         {
//                           case PortRegister:
//                             if(ijce->name_A == jpname)
//                             {
//                               jpid = ijce->port_id_A;
//                               jp = jack_port_by_id(_client, jpid);
//                               state = REGISTERED;
//                             }
//                           break;
//                           
//                           default:
//                           break;
//                         }
//                       break;
//                       
//                       case UNREGISTERED:
//                         switch(ijce->type)
//                         {
//                           case PortRegister:
//                             if(ijce->name_A == jpname)
//                             {
//                               jpid = ijce->port_id_A;
//                               jp = jack_port_by_id(_client, jpid);
//                               state = REGISTERED;
//                             }
//                           break;
// 
//                           default:
//                           break;
//                         }
//                       break;
//                       
//                       case REGISTERED:
//                         switch(ijce->type)
//                         {
//                           case PortConnect:
//                             if(ijce->port_A == jp && ijce->port_B == port)
//                             {
//                               jpid = ijce->port_id_A;
//                               jpname = ijce->name_A;
//                               state = CONNECTED;
//                             }
//                             else if(ijce->port_B == jp && ijce->port_A == port)
//                             {
//                               jpid = ijce->port_id_B;
//                               jpname = ijce->name_B;
//                               state = CONNECTED;
//                             }
//                           break;
// 
//                           case PortUnregister:
//                             if(ijce->port_id_A == jpid)
//                             {
//                               jpid = 0;
//                               jp = 0;
//                               state = UNREGISTERED;
//                             }
//                           break;
// 
//                           default:
//                           break;
//                         }
//                       break;
//                     }
//                   }
//                   
//                   if((state != init_state) || (jp != irl->jackPort) || (jpname != irl->persistentJackPortName)) // || (jpid != 0))
//                   {
//                     switch(state)
//                     {
//                       case UNREGISTERED:
//                       case REGISTERED:
//                       case DISCONNECTED:
//                       case DISCONNECTED_PENDING:
//                       {
//                         Route dst(it, channel);
//                         operations.add(PendingOperationItem(*irl, dst, PendingOperationItem::DeleteRoute));
//                       }
//                       break;
//                       
//                       case PERSISTENT_UNREGISTERED:
//                       case CONNECTED:
//                       {
//                         Route src(*irl);
//                         src.jackPort = jp;
//                         src.persistentJackPortName = jpname;
//                         operations.add(PendingOperationItem(src, &(*irl), PendingOperationItem::ModifyRouteNode));
//                       }
//                       break;
//                     }
//                   }
// 
//                   
//                   
//                   
//                   iJackCallbackEvent ijce_unreg;
//                   if(irl->jackPort)
//                   {
//                     jack_port_t* jp = 0;
//                     QString jpname;
//                     // Find the first port disconnect and port unregister pair before a graph change...
//                     for(iJackCallbackEvent ijce = jce_list.begin(); ijce != jce_list.end(); ++ijce)
//                     {
//                       // See if PortDisconnect is followed by PortUnregister, before the next GraphChanged event.
//                       if(ijce->type == PortDisconnect && (irl->jackPort == ijce->port_A || irl->jackPort == ijce->port_B))
//                       {
//                         iJackCallbackEvent ifl = ijce;
//                         ++ifl;
//                         for( ; ifl != jce_list.end(); ++ifl)
//                         {
//                           // Stop looking on the next GraphChanged event.
//                           if(ijce->type == GraphChanged)
//                             break;
//                           if(ifl->type == PortUnregister)
//                           {
//                             if(ifl->port_id_A == ijce->port_id_A)
//                             {
//                               jp = ijce->port_A;
//                               jpname = ijce->name_A;
//                               ijce_unreg = ifl;
//                               break;
//                             }
//                             else if(ifl->port_id_A == ijce->port_id_B)
//                             {
//                               jp = ijce->port_B;
//                               jpname = ijce->name_B;
//                               ijce_unreg = ifl;
//                               break;
//                             }
//                           }
//                         }
//                         if(jp)
//                           break; // We found a disconnect/unregister pair.
//                       }
//                     }
// 
//                     // If a port disconnect and port unregister pair were found...
//                     if(jp)
//                     {
//                       Route src(*irl);
//                       src.jackPort = 0;
//                       src.persistentJackPortName = jpname;
//                       operations.add(PendingOperationItem(src, *irl, PendingOperationItem::ModifyRouteNode));
//                     }
//                     else
//                     {
//                       const char* portName = irl->name().toLatin1().constData();
//                       bool found = false;
//                       const char** pn = ports;
//                       while(pn && *pn) 
//                       {
//                         if(strcmp(*pn, portName) == 0) 
//                         {
//                           found = true;
//                           break;
//                         }
//                         ++pn;
//                       }
//                       if(!found) 
//                       {
//                         Route dst(it, channel);
//                         operations.add(PendingOperationItem(*irl, dst, PendingOperationItem::DeleteRoute));
//                       }
//                     }
//                   }
//                   else
//                   {
//                     // Check for auto-reconnects...
//                     const char* portName = irl->persistentJackPortName.toLatin1().constData();
//                     const char** pn = aud_inports;
//                     while(pn && *pn) 
//                     {
//                       if(strcmp(*pn, portName) == 0) 
//                       {
//                         jack_port_t* p = jack_port_by_name(_client, portName);
//                         if(p)
//                         {
//                           Route src(*irl);
//                           src.jackPort = p;
//                           operations.add(PendingOperationItem(src, &(*irl), PendingOperationItem::ModifyRouteNode));
//                           break;
//                         }
//                       }
//                       ++pn;
//                     }
//                   }

//                             QString name = irl->name();
//                             QByteArray ba = name.toLatin1();
//                             const char* portName = ba.constData();
//                             bool found = false;
//                             const char** pn = ports;
//                             while (pn && *pn) {
//                                   if (strcmp(*pn, portName) == 0) {
//                                         found = true;
//                                         break;
//                                         }
//                                   ++pn;
//                                   }
//                             if (!found) {
//                                   // REMOVE Tim. Persistent routes. Added.
//                                   Route src(it, channel);
//                                   Route dst(portName, false, channel, Route::JACK_ROUTE);
//                                   operations.add(PendingOperationItem(src, dst, PendingOperationItem::DeleteRoute));
// // REMOVE Tim. Persistent routes. Removed.
// //                                     MusEGlobal::audio->msgRemoveRoute1(
// //                                        Route(it, channel),
// //                                        //Route(portName, false, channel)
// //                                        Route(portName, false, channel, Route::JACK_ROUTE)
// //                                        );
// //                                     erased = true;
// //                                     break;
//                                   }
//                             }
// //                         if (!erased)
// //                               break;


//   // Ignore the actual GraphChanged events.
//   if(ijce->type == GraphChanged)
//     continue;
//   
//   // See if PortDisconnect is followed by PortUnregister, before the next GraphChanged event.
//   if(ijce->type == PortDisconnect)
//   {
//     bool found = false;
//     iJackCallbackEvent ifl = ijce;
//     ++ifl;
//     for( ; ifl != jce_list.end(); ++ifl)
//     {
//       // Stop looking on the next GraphChanged event.
//       if(ijce->type == GraphChanged)
//         break;
//       // If we find a PortDisconnect followed by a PortUnregister, copy the port name to the
//       //  PortUnregister event, to make it easier to deal with during list enumeration below.
//       if(ifl->type == PortUnregister && (ifl->port_id_A == ijce->port_id_A || ifl->port_id_A == ijce->port_id_B))
//       {
//         if(ifl->port_id_A == ijce->port_id_A)
//           ifl->name_A = ijce->name_A;
//         if(ifl->port_id_A == ijce->port_id_B)
//           ifl->name_A = ijce->name_B;
//         found = true;
//       }
//     }
//     // If we found the corresponding PortUnregister, IGNORE the PortDisconnect.
//     if(found)
//       continue;  
//   }
// 
//   // See if PortConnect is followed by PortDisconnect anywhere.
//   if(ijce->type == PortConnect)
//   {
//     bool found = false;
//     iJackCallbackEvent ifl = ijce;
//     ++ifl;
//     for( ; ifl != jce_list.end(); ++ifl)
//     {
//       // If we find a PortConnect followed by a PortDisconnect, copy the port name to the
//       //  PortUnregister event, to make it easier to deal with during list enumeration below.
//       if(ifl->type == PortDisconnect && (ifl->port_id_A == ijce->port_id_A && ifl->port_id_B == ijce->port_id_B))
//       {
//         if(ifl->port_id_A == ijce->port_id_A)
//           ifl->name_A = ijce->name_A;
//         if(ifl->port_id_A == ijce->port_id_B)
//           ifl->name_A = ijce->name_B;
//         found = true;
//       }
//     }
//     // If we found the corresponding PortUnregister, IGNORE the PortDisconnect.
//     if(found)
//       continue;  
//   }
//   


// REMOVE Tim. Persistent routes. Added. First incarnation...
// //---------------------------------------------------------
// //   processJackCallbackEvents
// //---------------------------------------------------------
// 
// static int processJackCallbackEvents(jack_client_t* client, JackCallbackEventList& jce_list, Route& ext_node, 
//                                      jack_port_t* our_port, const Route& our_node, bool is_input, PendingOperationList& operations)
// {
//   enum CallbackState { DISCONNECTED, DISCONNECTED_PENDING, CONNECTED, PERSISTENT_UNREGISTERED, UNREGISTERED, REGISTERED }; 
// 
//   jack_port_id_t jpid = 0;
//   jack_port_t* jp = (jack_port_t*)ext_node.jackPort;
//   QString jpname = ext_node.persistentJackPortName;
//   CallbackState init_state = jp ? CONNECTED : PERSISTENT_UNREGISTERED;
//   CallbackState state = init_state;
//   int err_cnt = 0;
//   
//   for(iJackCallbackEvent ijce = jce_list.begin(); ijce != jce_list.end(); ++ijce)
//   {
//     switch(state)
//     {
//       case DISCONNECTED:
//         switch(ijce->type)
//         {
//           case PortConnect:
//             if(ijce->port_A == jp && ijce->port_B == our_port)
//             {
//               jpid = ijce->port_id_A;
//               jpname = ijce->name_A;
//               state = CONNECTED;
//             }
//             else if(ijce->port_B == jp && ijce->port_A == our_port)
//             {
//               jpid = ijce->port_id_B;
//               jpname = ijce->name_B;
//               state = CONNECTED;
//             }
//           break;
//           
//           case PortUnregister:
//             if(ijce->port_id_A == jpid)
//             {
//               jpid = 0;
//               jp = 0;
//               state = UNREGISTERED;
//             }
//           break;
// 
//           case PortDisconnect:
//             if((ijce->port_A == jp && ijce->port_B == our_port) || (ijce->port_B == jp && ijce->port_A == our_port))
//             {
//               ++err_cnt;
//               fprintf(stderr, "ERROR: processJackCallbackEvents state:DISCONNECTED command:PortDisconnect\n");
//             }
//           break;
// 
//           case PortRegister:
//             if(ijce->name_A == jpname)
//             {
//               ++err_cnt;
//               fprintf(stderr, "ERROR: processJackCallbackEvents state:DISCONNECTED command:PortRegister\n");
//             }
//           break;
// 
//           default:
//           break;
//         }
//       break;
//       
//       case DISCONNECTED_PENDING:
//         switch(ijce->type)
//         {
//           case GraphChanged:
//             state = DISCONNECTED;
//           break;
// 
//           case PortConnect:
//             if(ijce->port_A == jp && ijce->port_B == our_port)
//             {
//               jpid = ijce->port_id_A;
//               jpname = ijce->name_A;
//               state = CONNECTED;
//             }
//             else if(ijce->port_B == jp && ijce->port_A == our_port)
//             {
//               jpid = ijce->port_id_B;
//               jpname = ijce->name_B;
//               state = CONNECTED;
//             }
//           break;
// 
//           case PortUnregister:
//             if(ijce->port_id_A == jpid)
//             {
//               jpid = 0;
//               jp = 0;
//               state = PERSISTENT_UNREGISTERED;
//             }
//           break;
// 
//           case PortDisconnect:
//             if((ijce->port_A == jp && ijce->port_B == our_port) || (ijce->port_B == jp && ijce->port_A == our_port))
//             {
//               ++err_cnt;
//               fprintf(stderr, "ERROR: processJackCallbackEvents state:DISCONNECTED_PENDING command:PortDisconnect\n");
//             }
//           break;
// 
//           case PortRegister:
//             if(ijce->name_A == jpname)
//             {
//               ++err_cnt;
//               fprintf(stderr, "ERROR: processJackCallbackEvents state:DISCONNECTED_PENDING command:PortRegister\n");
//             }
//           break;
// 
//           default:
//           break;
//         }
//       break;
//       
//       case CONNECTED:
//         switch(ijce->type)
//         {
//           case PortDisconnect:
//             if(ijce->port_A == jp && ijce->port_B == our_port)
//             {
//               jpid = ijce->port_id_A;
//               jpname = ijce->name_A;
//               state = DISCONNECTED_PENDING;
//             }
//             else if(ijce->port_B == jp && ijce->port_A == our_port)
//             {
//               jpid = ijce->port_id_B;
//               jpname = ijce->name_B;
//               state = DISCONNECTED_PENDING;
//             }
//           break;
// 
//           case PortConnect:
//             if((ijce->port_A == jp && ijce->port_B == our_port) || (ijce->port_B == jp && ijce->port_A == our_port))
//             {
//               ++err_cnt;
//               fprintf(stderr, "ERROR: processJackCallbackEvents state:CONNECTED command:PortConnect\n");
//             }
//           break;
//           
//           //case PortUnregister:
//           //  if(ijce->name_A == jpname)
//           //  {
//           //    fprintf(stderr, "ERROR: processJackCallbackEvents state:CONNECTED command:PortUnregister\n");
//           //  }
//           //break;
//           
//           case PortRegister:
//             if(ijce->port_A == jp)
//             {
//               ++err_cnt;
//               fprintf(stderr, "ERROR: processJackCallbackEvents state:CONNECTED command:PortRegister\n");
//             }
//           break;
//           
//           default:
//           break;
//         }
//       break;
//       
//       case PERSISTENT_UNREGISTERED:
//         switch(ijce->type)
//         {
//           case PortRegister:
//             if(ijce->name_A == jpname)
//             {
//               jpid = ijce->port_id_A;
//               jp = jack_port_by_id(client, jpid);
//               state = REGISTERED;
//             }
//           break;
//           
//           default:
//           break;
//         }
//       break;
//       
//       case UNREGISTERED:
//         switch(ijce->type)
//         {
//           case PortRegister:
//             if(ijce->name_A == jpname)
//             {
//               jpid = ijce->port_id_A;
//               jp = jack_port_by_id(client, jpid);
//               state = REGISTERED;
//             }
//           break;
// 
//           default:
//           break;
//         }
//       break;
//       
//       case REGISTERED:
//         switch(ijce->type)
//         {
//           case PortConnect:
//             if(ijce->port_A == jp && ijce->port_B == our_port)
//             {
//               jpid = ijce->port_id_A;
//               jpname = ijce->name_A;
//               state = CONNECTED;
//             }
//             else if(ijce->port_B == jp && ijce->port_A == our_port)
//             {
//               jpid = ijce->port_id_B;
//               jpname = ijce->name_B;
//               state = CONNECTED;
//             }
//           break;
// 
//           case PortUnregister:
//             if(ijce->port_id_A == jpid)
//             {
//               jpid = 0;
//               jp = 0;
//               state = UNREGISTERED;
//             }
//           break;
// 
//           default:
//           break;
//         }
//       break;
//     }
//   }
// 
//   if((state != init_state) || (jp != ext_node.jackPort) || (jpname != ext_node.persistentJackPortName)) // || (jpid != 0))
//   {
//     switch(state)
//     {
//       case UNREGISTERED:
//       case REGISTERED:
//       case DISCONNECTED:
//       case DISCONNECTED_PENDING:
//       {
//         // REMOVE Tim. Persistent routes. Added.
//         fprintf(stderr, "processJackCallbackEvents Deleting route: state:%d init_state:%d jp:%p ext_jp:%p jpname:%s ext_persistent_name:%s\n", 
//                 state, init_state, jp, ext_node.jackPort, jpname.toLatin1().constData(), ext_node.persistentJackPortName.toLatin1().constData());
//         if(is_input)
//           operations.add(PendingOperationItem(ext_node, our_node, PendingOperationItem::DeleteRoute));
//         else
//           operations.add(PendingOperationItem(our_node, ext_node, PendingOperationItem::DeleteRoute));
//       }
//       break;
//       
//       case PERSISTENT_UNREGISTERED:
//       case CONNECTED:
//       {
//         // REMOVE Tim. Persistent routes. Added.
//         fprintf(stderr, "processJackCallbackEvents Modifying route: state:%d init_state:%d jp:%p ext_jp:%p jpname:%s ext_persistent_name:%s\n", 
//                 state, init_state, jp, ext_node.jackPort, jpname.toLatin1().constData(), ext_node.persistentJackPortName.toLatin1().constData());
//         Route src(ext_node);
//         src.jackPort = jp;
//         src.persistentJackPortName = jpname;
//         operations.add(PendingOperationItem(src, &ext_node, PendingOperationItem::ModifyRouteNode));
//       }
//       break;
//     }
//   }
//   
//   return err_cnt;
// }


//---------------------------------------------------------
//   processJackCallbackEvents
//---------------------------------------------------------

// enum CallbackRouteState { JCB_UNCHANGED, JCB_MODIFY, JCB_ADD, JCB_DELETE }; 
// struct RouteStruct
// {
//   // Link to the original route.
//   Route*         route;    
//   // These items track changes to be committed to the route.
//   //jack_port_t*   port;
//   //jack_port_id_t port_id;
//   //QString        port_name;
//   Route          work_route;    
//   //CallbackState  init_state;
//   CallbackRouteState  state;
// //   RouteStruct() { route = 
// //   RouteStruct(Route* route_, jack_port_t* port_, jack_port_id_t port_id_, const QString& port_name_, CallbackState init_state_, CallbackState state_)
// //   {
// //     route = route_; port = port_; port_id = port_id_; port_name = port_name_; init_state = init_state_; state = state_; 
// //   }
// };
// typedef std::vector<RouteStruct> RouteStructList;
// typedef std::vector<RouteStruct>::iterator iRouteStructList;

// static int processJackCallbackEvents(JackAudioDevice* jad, JackCallbackEventList& jce_list, const Route& our_node, jack_port_t* our_port, PendingOperationList& operations, RouteList* route_list, bool is_input)
// {
//   if(route_list->empty())
//     return 0;
//   
//   int err_cnt = 0;
// 
//   RouteStructList working_rl;
//   for(iRoute ir = route_list->begin(); ir != route_list->end(); ++ir)
//   {
//     if(ir->type != Route::JACK_ROUTE)  
//       continue;
//     if(our_node.channel != -1 && ir->channel != our_node.channel)
//       continue;
//     RouteStruct rs;
//     rs.route = &(*ir);
//     rs.port = (jack_port_t*)ir->jackPort;
//     rs.port_id = 0;
//     rs.port_name = ir->persistentJackPortName;
//     rs.init_state = rs.port ? CONNECTED : PERSISTENT_UNREGISTERED;
//     rs.state = rs.init_state;
//     working_rl.push_back(rs);
//   }
// 
//   for(iJackCallbackEvent ijce = jce_list.begin(); ijce != jce_list.end(); ++ijce)
//   {
//     bool port_connect_handled = false;
//     for(iRouteStructList irs = working_rl.begin(); irs != working_rl.end(); ++irs)
//     {
//       switch(irs->state)
//       {
//         case DISCONNECTED:
//           switch(ijce->type)
//           {
//             case PortConnect:
//               if(ijce->port_A == irs->port && ijce->port_B == our_port)
//               {
//                 irs->port_id = ijce->port_id_A;
//                 irs->port_name = ijce->name_A;
//                 irs->state = CONNECTED;
//                 port_connect_handled = true;
//               }
//               else if(ijce->port_B == irs->port && ijce->port_A == our_port)
//               {
//                 irs->port_id = ijce->port_id_B;
//                 irs->port_name = ijce->name_B;
//                 irs->state = CONNECTED;
//                 port_connect_handled = true;
//               }
//             break;
//             
//             case PortUnregister:
//               if(ijce->port_id_A == irs->port_id)
//               {
//                 irs->port_id = 0;
//                 irs->port = 0;
//                 irs->state = UNREGISTERED;
//               }
//             break;
// 
//             case PortDisconnect:
//               if((ijce->port_A == irs->port && ijce->port_B == our_port) || (ijce->port_B == irs->port && ijce->port_A == our_port))
//               {
//                 ++err_cnt;
//                 fprintf(stderr, "ERROR: processJackCallbackEvents state:DISCONNECTED command:PortDisconnect\n");
//               }
//             break;
// 
// //             case PortRegister:
// //               if(ijce->name_A == irs->port_name)
// //               {
// //                 ++err_cnt;
// //                 fprintf(stderr, "ERROR: processJackCallbackEvents state:DISCONNECTED command:PortRegister\n");
// //               }
// //             break;
// 
//             default:
//             break;
//           }
//         break;
// 
//         case DISCONNECTED_PENDING:
//           switch(ijce->type)
//           {
//             case GraphChanged:
//               irs->state = DISCONNECTED;
//             break;
// 
//             case PortConnect:
//               if(ijce->port_A == irs->port && ijce->port_B == our_port)
//               {
//                 irs->port_id = ijce->port_id_A;
//                 irs->port_name = ijce->name_A;
//                 irs->state = CONNECTED;
//                 port_connect_handled = true;
//               }
//               else if(ijce->port_B == irs->port && ijce->port_A == our_port)
//               {
//                 irs->port_id = ijce->port_id_B;
//                 irs->port_name = ijce->name_B;
//                 irs->state = CONNECTED;
//                 port_connect_handled = true;
//               }
//             break;
// 
//             case PortUnregister:
//               if(ijce->port_id_A == irs->port_id)
//               {
//                 irs->port_id = 0;
//                 irs->port = 0;
//                 irs->state = PERSISTENT_UNREGISTERED;
//               }
//             break;
// 
//             case PortDisconnect:
//               if((ijce->port_A == irs->port && ijce->port_B == our_port) || (ijce->port_B == irs->port && ijce->port_A == our_port))
//               {
//                 ++err_cnt;
//                 fprintf(stderr, "ERROR: processJackCallbackEvents irs->state:DISCONNECTED_PENDING command:PortDisconnect\n");
//               }
//             break;
// 
// //             case PortRegister:
// //               if(ijce->name_A == irs->port_name)
// //               {
// //                 ++err_cnt;
// //                 fprintf(stderr, "ERROR: processJackCallbackEvents irs->state:DISCONNECTED_PENDING command:PortRegister\n");
// //               }
// //             break;
// 
//             default:
//             break;
//           }
//         break;
//         
//         case CONNECTED:
//           switch(ijce->type)
//           {
//             case PortDisconnect:
//               if(ijce->port_A == irs->port && ijce->port_B == our_port)
//               {
//                 irs->port_id = ijce->port_id_A;
//                 irs->port_name = ijce->name_A;
//                 irs->state = DISCONNECTED_PENDING;
//               }
//               else if(ijce->port_B == irs->port && ijce->port_A == our_port)
//               {
//                 irs->port_id = ijce->port_id_B;
//                 irs->port_name = ijce->name_B;
//                 irs->state = DISCONNECTED_PENDING;
//               }
//             break;
// 
//             case PortConnect:
//               if((ijce->port_A == irs->port && ijce->port_B == our_port) || (ijce->port_B == irs->port && ijce->port_A == our_port))
//               {
//                 ++err_cnt;
//                 port_connect_handled = true;
//                 fprintf(stderr, "ERROR: processJackCallbackEvents irs->state:CONNECTED command:PortConnect\n");
//               }
//             break;
//             
//             //case PortUnregister:
//             //  if(ijce->name_A == irs->port_name)
//             //  {
//             //    fprintf(stderr, "ERROR: processJackCallbackEvents irs->state:CONNECTED command:PortUnregister\n");
//             //  }
//             //break;
//             
// //             case PortRegister:
// //               if(ijce->port_A == irs->port)
// //               {
// //                 ++err_cnt;
// //                 fprintf(stderr, "ERROR: processJackCallbackEvents irs->state:CONNECTED command:PortRegister\n");
// //               }
// //             break;
//             
//             default:
//             break;
//           }
//         break;
//         
//         case PERSISTENT_UNREGISTERED:
//           switch(ijce->type)
//           {
//             case PortRegister:
//             {
//               bool res;
//               QString n = jad->portName(ijce->port_A, &res);
//               if(res && n == irs->port_name)
//               {
//                 irs->port_id = ijce->port_id_A;
//                 //irs->port = jack_port_by_id(client, irs->port_id);
//                 irs->port = ijce->port_A;
//                 irs->state = REGISTERED;
//               }
//             }
//             break;
//             
//             case PortConnect:
//               if((ijce->name_A == irs->port_name && ijce->port_B == our_port) || (ijce->name_B == irs->port_name && ijce->port_A == our_port))
//               {
//                 ++err_cnt;
//                 port_connect_handled = true;
//                 fprintf(stderr, "ERROR: processJackCallbackEvents irs->state:PERSISTENT_UNREGISTERED command:PortConnect\n");
//               }
//             break;
// 
//             default:
//             break;
//           }
//         break;
//         
//         case UNREGISTERED:
//           switch(ijce->type)
//           {
//             case PortRegister:
//             {
//               bool res;
//               QString n = jad->portName(ijce->port_A, &res);
//               if(res && n == irs->port_name)
//               {
//                 irs->port_id = ijce->port_id_A;
//                 //irs->port = jack_port_by_id(client, irs->port_id);
//                 irs->port = ijce->port_A;
//                 irs->state = REGISTERED;
//               }
//             }
//             break;
// 
//             case PortConnect:
//               if((ijce->name_A == irs->port_name && ijce->port_B == our_port) || (ijce->name_B == irs->port_name && ijce->port_A == our_port))
//               {
//                 ++err_cnt;
//                 port_connect_handled = true;
//                 fprintf(stderr, "ERROR: processJackCallbackEvents irs->state:UNREGISTERED command:PortConnect\n");
//               }
//             break;
// 
//             default:
//             break;
//           }
//         break;
//         
//         case REGISTERED:
//           switch(ijce->type)
//           {
//             case PortConnect:
//               if(ijce->port_A == irs->port && ijce->port_B == our_port)
//               {
//                 irs->port_id = ijce->port_id_A;
//                 irs->port_name = ijce->name_A;
//                 irs->state = CONNECTED;
//                 port_connect_handled = true;
//               }
//               else if(ijce->port_B == irs->port && ijce->port_A == our_port)
//               {
//                 irs->port_id = ijce->port_id_B;
//                 irs->port_name = ijce->name_B;
//                 irs->state = CONNECTED;
//                 port_connect_handled = true;
//               }
//             break;
// 
//             case PortUnregister:
//               if(ijce->port_id_A == irs->port_id)
//               {
//                 irs->port_id = 0;
//                 irs->port = 0;
//                 irs->state = UNREGISTERED;
//               }
//             break;
// 
//             default:
//             break;
//           }
//         break;
//       }
//     }  
// 
//     // Check for unhandled port connects...
//     if(ijce->type == PortConnect && !port_connect_handled)
//     {
//       if(ijce->port_A == our_port)
//       {
//         if(is_input)
//         {
//           Route src(Route::JACK_ROUTE, 0, ijce->port_B, our_node.channel, 0, 0, ijce->name_B);
//           operations.add(PendingOperationItem(src, our_node, PendingOperationItem::AddRoute));
//         }
//         else
//         {
//           Route dst(Route::JACK_ROUTE, 0, ijce->port_B, our_node.channel, 0, 0, ijce->name_B);
//           operations.add(PendingOperationItem(our_node, dst, PendingOperationItem::AddRoute));
//         }
//       }
//       else if(ijce->port_B == our_port)
//       {
//         if(is_input)
//         {
//           Route src(Route::JACK_ROUTE, 0, ijce->port_A, our_node.channel, 0, 0, ijce->name_A);
//           operations.add(PendingOperationItem(src, our_node, PendingOperationItem::AddRoute));
//         }
//         else
//         {
//           Route dst(Route::JACK_ROUTE, 0, ijce->port_A, our_node.channel, 0, 0, ijce->name_A);
//           operations.add(PendingOperationItem(our_node, dst, PendingOperationItem::AddRoute));
//         }
//       }
//     }
//   }  
//   
//   // Commit any routing list changes...
//   for(iRouteStructList irs = working_rl.begin(); irs != working_rl.end(); ++irs)
//   {
//     switch(irs->state)
//     {
//       case UNREGISTERED:
//       case REGISTERED:
//       case DISCONNECTED:
//       case DISCONNECTED_PENDING:
//       {
//         if(irs->state != irs->init_state)
//         {
//           // REMOVE Tim. Persistent routes. Added.
//           fprintf(stderr, "processJackCallbackEvents Deleting route: state:%d init_state:%d jp:%p route_jp:%p jpname:%s route_persistent_name:%s\n", 
//                   irs->state, irs->init_state, irs->port, irs->route->jackPort, irs->port_name.toLatin1().constData(), irs->route->persistentJackPortName.toLatin1().constData());
//           if(is_input)
//             operations.add(PendingOperationItem(*irs->route, our_node, PendingOperationItem::DeleteRoute));
//           else
//             operations.add(PendingOperationItem(our_node, *irs->route, PendingOperationItem::DeleteRoute));
//         }
//       }
//       break;
//       
//       case PERSISTENT_UNREGISTERED:
//       case CONNECTED:
//       {
//         if((irs->port != irs->route->jackPort) || (irs->port_name != irs->route->persistentJackPortName)) // || (irs->port_id != 0))
//         {
//           // REMOVE Tim. Persistent routes. Added.
//           fprintf(stderr, "processJackCallbackEvents Modifying route: state:%d init_state:%d jp:%p route_jp:%p jpname:%s route_persistent_name:%s\n", 
//                   irs->state, irs->init_state, irs->port, irs->route->jackPort, irs->port_name.toLatin1().constData(), irs->route->persistentJackPortName.toLatin1().constData());
//           Route src(*irs->route);
//           src.jackPort = irs->port;
//           src.persistentJackPortName = irs->port_name;
//           operations.add(PendingOperationItem(src, irs->route, PendingOperationItem::ModifyRouteNode));
//         }
//       }
//       break;
//     }
//   }
//   return err_cnt;
// }  
//   


//static void processJackCallbackEvents(JackAudioDevice* jad, const Route& our_node, jack_port_t* our_port, PendingOperationList& operations, RouteList* route_list, bool is_input)
//void JackAudioDevice::processJackCallbackEvents(const Route& our_node, jack_port_t* our_port, 
//                                                PendingOperationList& operations, RouteList* route_list, bool is_input)
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
      fprintf(stderr, "JackAudioDevice::processJackCallbackEvents: changing audio input port!: channel:%d our_port:%p new port:%p\n", 
              our_node.channel, our_port, jp);
      //operations.add(PendingOperationItem(route_list, r, PendingOperationItem::AddRouteNode)); // TODO
      our_ext_port = jp;
    }
  }
  
//   // TODO: Maybe allocate this instead of stack, if required. 
//   JackOperationList working_rl;
//   for(iRoute ir = route_list->begin(); ir != route_list->end(); ++ir)
//   {
//     if(ir->type != Route::JACK_ROUTE)  
//       continue;
//     if(our_node.channel != -1 && ir->channel != our_node.channel)
//       continue;
//     JackOperation rs;
//     rs.route = &(*ir);
// //     rs.port = (jack_port_t*)ir->jackPort;
// //     rs.port_id = 0;
// //     rs.port_name = ir->persistentJackPortName;
// //     rs.init_state = rs.port ? CONNECTED : PERSISTENT_UNREGISTERED;
// //     rs.state = rs.init_state;
// //     rs.work_route.jackPort = ir->jackPort;
// //     rs.work_route.persistentJackPortName = ir->persistentJackPortName;
// //     rs.work_route.channel = ir->channel;
//     rs.work_route = *ir;
//     rs.type = JCB_UNCHANGED;
//     working_rl.push_back(rs);
//     //working_rl.push_back(*ir);
//   }
  
  for(iRoute ir = route_list->begin(); ir != route_list->end(); ++ir)
  //for(iJackOperationList ir = working_rl.begin(); ir != working_rl.end(); ++ir)
  {
    // Check correct route type, and channel if required.
    if((ir->type != Route::JACK_ROUTE) || (our_node.channel != -1 && ir->channel != our_node.channel))
      continue;
    //const char* route_jpname = ir->persistentJackPortName.toLatin1().constData();
    //const char* route_jpname = ir->work_route.persistentJackPortName.toLatin1().constData();
    const char* route_jpname = ir->persistentJackPortName;
    //const char* route_jpname = ir->work_route.persistentJackPortName;
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
        //JackOperation jo;
        //jo.type = JCB_UNCHANGED;
        const char* s = NULL;
        if(jp != ir->jackPort)
        //if(jp != ir->work_route.jackPort)
        {
          // REMOVE Tim. Persistent routes. Added.
          fprintf(stderr, "processJackCallbackEvents: Ports connected. Modifying route: our_port:%p old_route_jp:%p new_route_jp:%p route_persistent_name:%s\n", 
                  our_port, ir->jackPort, jp, route_jpname);
                  //our_port, ir->work_route.jackPort, jp, route_jpname);
          //jo.route = ir;
          //jo.work_route = Route(Route::JACK_ROUTE, 0, jp, our_node.channel, 0, 0, route_jpname);
          s = route_jpname;
          //jo.type = JCB_MODIFY;
          //ir->work_route.jackPort = jp;
          //ir->type = JCB_MODIFY;
        }
        // Find a more appropriate name if necessary.
        //bool res = false;
        char fin_name[ROUTE_PERSISTENT_NAME_SIZE];
        //QString fin_name = portName(jp, &res);
        portName(jp, fin_name, ROUTE_PERSISTENT_NAME_SIZE);
        //Route src(*ir);
        //src.jackPort = jp;
        //ir->work_route.jackPort = jp;
        // Replace the persistentJackPortName with the more appropriate one if necessary.
          //if(res)
            //ir->persistentJackPortName = fin_name; 
            //ir->work_route.persistentJackPortName = fin_name; 
        if(strcmp(ir->persistentJackPortName, fin_name) != 0)
        //if(strcmp(ir->work_route.persistentJackPortName, fin_name) != 0)
        {
          // REMOVE Tim. Persistent routes. Added.
          fprintf(stderr, "processJackCallbackEvents: Ports connected. Modifying route name: route_persistent_name:%s new name:%s\n", route_jpname, fin_name);
          //MusELib::strntcpy(ir->work_route.persistentJackPortName, fin_name, ROUTE_PERSISTENT_NAME_SIZE); 
          //strcpy(ir->persistentJackPortName, fin_name); 
          //strcpy(ir->work_route.persistentJackPortName, fin_name); 
          s = fin_name;
          //jo.type = JCB_MODIFY;
          //ir->state = JCB_MODIFY;
        }
        
        //if(jo.type == JCB_MODIFY)
        if(s)
        {
          //jo.route = &(*ir);
          //jo.work_route = Route(Route::JACK_ROUTE, 0, jp, our_node.channel, 0, 0, NULL);
          //strcpy(jo.work_route.persistentJackPortName, s);
          //jackOpsFifo.put(jo);
          
          
          operations.add(PendingOperationItem(Route(Route::JACK_ROUTE, 0, jp, ir->channel, 0, 0, s), &(*ir), PendingOperationItem::ModifyRouteNode));
        }
        
      }
      else 
      {
        if(ir->jackPort)
        //if(ir->work_route.jackPort)
        {
          // Check whether the disconnect happened BEFORE this graphChanged() was called, 
          //  or just now during it, or was followed by an unregister. 
          //int ret = checkDisconnectCallback(our_port, jp);
          int ret = checkDisconnectCallback(our_ext_port, jp);
          if(ret == 2)
          {
            // REMOVE Tim. Persistent routes. Added.
            fprintf(stderr, "processJackCallbackEvents: Ports not connected, ret=DeleteRouteNode. Deleting route: our_port:%p route_jp:%p found_jp:%p route_persistent_name:%s\n", 
                      our_port, ir->jackPort, jp, route_jpname);
                      //our_port, ir->route->jackPort, jp, route_jpname);
            // The port exists but is not connected to our port. Remove the route node.
  //           JackOperation jo;
  //           //jo.is_input = is_input;
  //           //jo.our_route = our_node;
  //           //jo.work_route = *ir;
  //           jo.route_list = route_list;
  //           jo.iroute = ir;
  //           jo.type = JCB_DELETE;
  //           jackOpsFifo.put(jo);
  //           //ir->state = JCB_DELETE;
            
            operations.add(PendingOperationItem(route_list, ir, PendingOperationItem::DeleteRouteNode));
          }
          else
          if(ret == 1)
          {
            // REMOVE Tim. Persistent routes. Added.
            fprintf(stderr, "processJackCallbackEvents: Ports not connected, ret=ModifyRouteNode. Modifying route: our_port:%p route_jp:%p found_jp:%p route_persistent_name:%s\n", 
                      our_port, ir->jackPort, jp, route_jpname);
            operations.add(PendingOperationItem(Route(Route::JACK_ROUTE, 0, NULL, ir->channel, 0, 0, ir->persistentJackPortName), &(*ir), PendingOperationItem::ModifyRouteNode));
          }
        }
        else 
        if(MusEGlobal::audio && MusEGlobal::audio->isRunning()) // Don't try to connect if not running.
        {
          //std::vector<jack_port_t*>::iterator icf;
          //for(icf = failedConnects.begin(); icf != failedConnects.end(); ++icf)
          //  if(*icf == jp)
          //    break;
          //if(icf == failedConnects.end())
          {
            //for(icf = failedConnectsTemp.begin(); icf != failedConnectsTemp.end(); ++icf)
            //  if(*icf == jp)
            //    break;
            //if(icf == failedConnectsTemp.end())
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
                    
//                   const char* our_port_name = jack_port_name(our_port);
                  // REMOVE Tim. Persistent routes. Added.
                  fprintf(stderr, "processJackCallbackEvents: Ports not connected. Reconnecting route: our_port:%p route_jp:%p found_jp:%p our_port_name:%s route_persistent_name:%s\n", 
                            our_port, ir->jackPort, jp, our_port_name, route_jpname);
                            //our_port, ir->route->jackPort, jp, jpn, route_jpname);
                  if(our_port_name)
                  {
  //                   JackOperation jo;
  //                   jo.is_input = is_input;
  //                   //jo.our_route = our_node;
  //                   jo.route = &(*ir);
  //                   //jo.work_route = *ir;
  //                   jo.work_route.jackPort = jp;
  //                   jo.work_route.channel = ir->channel;
  //                   jo.work_route.type = ir->type;
  //                   jo.our_port = our_port;
  //                   //jo.our_route = our_node;
  //                   // (Ab)use the name for storing our port's name, not the external port name.
  //                   //MusELib::strntcpy(jo.work_route.persistentJackPortName, jpn, ROUTE_PERSISTENT_NAME_SIZE);
                    // Find a more appropriate name if necessary.
                    //char fin_name[ROUTE_PERSISTENT_NAME_SIZE];
                    //portName(jp, fin_name, ROUTE_PERSISTENT_NAME_SIZE);
  //                   portName(jp, jo.work_route.persistentJackPortName, ROUTE_PERSISTENT_NAME_SIZE);
                  
  //                   if(strcmp(ir->persistentJackPortName, fin_name) != 0)
  //                   {
  //                     // REMOVE Tim. Persistent routes. Added.
  //                     fprintf(stderr, "processJackCallbackEvents: Ports connected. Modifying route name: route_persistent_name:%s new name:%s\n", route_jpname, fin_name);
  //                     //MusELib::strntcpy(ir->work_route.persistentJackPortName, fin_name, ROUTE_PERSISTENT_NAME_SIZE); 
  //                     strcpy(jo.work_route.persistentJackPortName, fin_name); 
  //                     //ir->state = JCB_MODIFY;
  //                   }
  //                   else
  //                     strcpy(jo.work_route.persistentJackPortName, ir->persistentJackPortName); 
                    
  //                   jo.type = JCB_CONNECT;
  //                   jackOpsFifo.put(jo);
                    
                    int err;
                    if(is_input)
                      err = jack_connect(client, route_jpname, our_port_name);
                    else
                      err = jack_connect(client, our_port_name, route_jpname);
                    if(err)
                    {
                      // REMOVE Tim. Persistent routes. Added.
                      fprintf(stderr, "processJackCallbackEvents: Ports not connected. Reconnecting route: ERROR:%d our_port:%p route_jp:%p found_jp:%p our_port_name:%s route_persistent_name:%s\n", 
                              //err, our_port, ir->route->jackPort, jp, jpn, route_jpname);
                              err, our_port, ir->jackPort, jp, our_port_name, route_jpname);
                      //failedConnectsTemp.push_back(jp);
                    }
                    else
                    {
                      // We have our jack port, on a supposedly active client. Update the route node's jack port pointer.
                      //ir->work_route.jackPort = jp;
                      //ir->state = JCB_MODIFY;
                      
                      // Find a more appropriate name if necessary.
                      //char fin_name[ROUTE_PERSISTENT_NAME_SIZE];
                      //portName(jp, fin_name, ROUTE_PERSISTENT_NAME_SIZE);
                      //if(strcmp(ir->work_route.persistentJackPortName, fin_name) != 0)
                      //{
                      //  fprintf(stderr, "processJackCallbackEvents: Ports connected. Modifying route name: route_persistent_name:%s new name:%s\n", route_jpname, fin_name);
                      //  //MusELib::strntcpy(ir->work_route.persistentJackPortName, fin_name, ROUTE_PERSISTENT_NAME_SIZE); 
                      //  strcpy(ir->work_route.persistentJackPortName, fin_name); 
                      //  ir->state = JCB_MODIFY;
                      //}
                      
                      // We have our jack port, on a supposedly active client. Update the route node's jack port pointer.
                      // Find a more appropriate name if necessary.
                      const char* s = ir->persistentJackPortName;
                      char fin_name[ROUTE_PERSISTENT_NAME_SIZE];
                      portName(jp, fin_name, ROUTE_PERSISTENT_NAME_SIZE);
                      if(strcmp(ir->persistentJackPortName, fin_name) != 0)
                      {
                        fprintf(stderr, "processJackCallbackEvents: Ports connected. Modifying route name: route_persistent_name:%s new name:%s\n", route_jpname, fin_name);
                        s = fin_name;
                      }
                      operations.add(PendingOperationItem(Route(Route::JACK_ROUTE, 0, jp, ir->channel, 0, 0, s), &(*ir), PendingOperationItem::ModifyRouteNode));
                    }
                  }
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
      //if(ir->work_route.jackPort)
      {
        // REMOVE Tim. Persistent routes. Added.
        fprintf(stderr, "processJackCallbackEvents: Port non-existent. Modifying route: our_port:%p route_jp:%p route_persistent_name:%s\n", 
                our_port, ir->jackPort, route_jpname);
                //our_port, ir->work_route.jackPort, route_jpname);
        //JackOperation jo;
        //jo.route = &(*ir);
        //jo.work_route = *ir;
        //jo.work_route.jackPort = NULL;
        //jo.type = JCB_MODIFY;
        //jackOpsFifo.put(jo);
        //
        //ir->work_route.jackPort = 0;
        //ir->state = JCB_MODIFY;
        
        operations.add(PendingOperationItem(Route(Route::JACK_ROUTE, 0, NULL, ir->channel, 0, 0, ir->persistentJackPortName), &(*ir), PendingOperationItem::ModifyRouteNode));
      }
    }
  }

  checkNewRouteConnections(our_port, our_node.channel, route_list);
  
//   if(jack_ver_maj != 1) // Not Jack "2"
//   {
//     // Check for new connections...
//     const char** ports = jack_port_get_all_connections(client, our_port);
//     if(ports) 
//     {
//       const char** pn = ports;
//       while(*pn) 
//       {
//         // Caution: In Jack-1, under some conditions jack_port_by_name() might ALLOCATE a new port for use later if not found! 
//         // Should be safe and quick search here, we know that the port name is valid.
//         jack_port_t* jp = jack_port_by_name(client, *pn);
//         if(jp)
//         {
//   //         bool res = false;
//   //         QString good_name = jad->portName(jp, &res);
//   //         const char* fin_name = res ? good_name.toLatin1().constData() : *pn;
//   //         // REMOVE Tim. Persistent routes. Added.
//   //         fprintf(stderr, "processJackCallbackEvents: New connection test: res:%d port_name:%s good_name:%s fin_name:%s\n", 
//   //                 res, *pn, good_name.toLatin1().constData(), fin_name);
//           bool found = false;
//           //for(ciRoute ir = route_list->begin(); ir != route_list->end(); ++ir) 
//           for(iJackOperationList ir = working_rl.begin(); ir != working_rl.end(); ++ir) 
//           {
//             // Check correct route type, and channel if required.
//             //if((ir->type != Route::JACK_ROUTE) || (our_node.channel != -1 && ir->channel != our_node.channel))
//             //  continue;
//             //QString route_good_name;
//             //if(ir->jackPort)
//             //if(ir->work_route.jackPort)
//             if(ir->work_route.jackPort)
//             {
//               if(ir->work_route.jackPort != jp)
//                 continue;
//               found = true;
//               break;
//             }
//   //             // REMOVE Tim. Persistent routes. Added.
//   //             fprintf(stderr, "  testing A: fin_name:%s work_route.jackPort:%p\n", fin_name, ir->work_route.jackPort);
//   //             //route_good_name = ir->name(); 
//   //             route_good_name = ir->work_route.name(); 
//   //             // REMOVE Tim. Persistent routes. Added.
//   //             fprintf(stderr, "  testing B: fin_name:%s route_good_name:%s\n", fin_name, route_good_name.toLatin1().constData());
//   //           }
//             else
//             {
//               //jack_port_t* route_jp = jack_port_by_name(client, ir->name().toLatin1().constData());
//               //jack_port_t* route_jp = jack_port_by_name(client, ir->work_route.name().toLatin1().constData());
//               //jack_port_t* route_jp = jack_port_by_name(client, ir->work_route.persistentJackPortName.toLatin1().constData());
//               
//               // Caution: In Jack-1, under some conditions jack_port_by_name() might ALLOCATE a new port for use later if not found! 
//               // FIXME: TODO: switch to get_ports
//               jack_port_t* route_jp = jack_port_by_name(client, ir->work_route.persistentJackPortName);
//               if(route_jp == jp)
//               {
//                 found = true;
//                 break;
//               }
//   //             bool route_res = false;
//   //             QString n = jad->portName(route_jp, &route_res);
//   //             if(route_res)
//   //               route_good_name = n;  
//   // //             }
//   //             const char* portName = route_good_name.toLatin1().constData();
//   //             if(strcmp(fin_name, portName) == 0) 
//   //             {
//   //               found = true;
//   //               break;
//   //             }
//             }
//           }
//           if(!found) 
//           {
//             //bool res = false;
//             char good_name[ROUTE_PERSISTENT_NAME_SIZE];
//             //QString good_name = portName(jp, &res);
//             portName(jp, good_name, ROUTE_PERSISTENT_NAME_SIZE);
//             // REMOVE Tim. Persistent routes. Added.
//             fprintf(stderr, "processJackCallbackEvents: New connection. Adding route: our_port:%p route_jp:%p portname:%s route_persistent_name:%s\n", 
//                     //our_port, jp, *pn, good_name.toLatin1().constData());
//                     our_port, jp, *pn, good_name);
//             //Route r(Route::JACK_ROUTE, 0, jp, our_node.channel, 0, 0, res ? good_name : QString(*pn));
//             Route r(Route::JACK_ROUTE, 0, jp, our_node.channel, 0, 0, good_name);
//             //if(is_input)
//             //  operations.add(PendingOperationItem(r, our_node, PendingOperationItem::AddRoute));
//             //else
//             //  operations.add(PendingOperationItem(our_node, r, PendingOperationItem::AddRoute));
//             JackOperation rs;
//             rs.route = 0;
//             rs.work_route = r;
//             rs.type = JCB_ADD;
//             working_rl.push_back(rs);
//           }
//         }
//         ++pn;
//       }
// 
//       jack_free(ports);
//     }
//   }
//   
//   for(iJackOperationList ir = working_rl.begin(); ir != working_rl.end(); ++ir)
//   {
//     switch(ir->state)
//     {
//       case JCB_MODIFY:
//         // REMOVE Tim. Persistent routes. Added.
//         fprintf(stderr, "processJackCallbackEvents: JCB_MODIFY\n"); 
//         operations.add(PendingOperationItem(ir->work_route, ir->route, PendingOperationItem::ModifyRouteNode));
//       break;  
//       case JCB_ADD:
//         // REMOVE Tim. Persistent routes. Added.
//         fprintf(stderr, "processJackCallbackEvents: JCB_ADD\n"); 
//         if(is_input)
//           operations.add(PendingOperationItem(ir->work_route, our_node, PendingOperationItem::AddRoute));
//         else
//           operations.add(PendingOperationItem(our_node, ir->work_route, PendingOperationItem::AddRoute));
//       break;  
//       case JCB_DELETE:
//         // REMOVE Tim. Persistent routes. Added.
//         fprintf(stderr, "processJackCallbackEvents: JCB_DELETE\n"); 
//         if(is_input)
//           operations.add(PendingOperationItem(*ir->route, our_node, PendingOperationItem::DeleteRoute));
//         else
//           operations.add(PendingOperationItem(our_node, *ir->route, PendingOperationItem::DeleteRoute));
//       break;  
//       case JCB_UNCHANGED:
//       break;  
//     }
//   }  
}  
  




// REMOVE Tim. Persistent routes. Added.
//   jack_port_id_t jpid = 0;
//   jack_port_t* jp = (jack_port_t*)ext_node.jackPort;
//   QString jpname = ext_node.persistentJackPortName;
//   CallbackState init_state = jp ? CONNECTED : PERSISTENT_UNREGISTERED;
//   CallbackState state = init_state;
//   int err_cnt = 0;
//   
//   for(iJackCallbackEvent ijce = jce_list.begin(); ijce != jce_list.end(); ++ijce)
//   {
//     switch(state)
//     {
//       case DISCONNECTED:
//         switch(ijce->type)
//         {
//           case PortConnect:
//             if(ijce->port_A == jp && ijce->port_B == our_port)
//             {
//               jpid = ijce->port_id_A;
//               jpname = ijce->name_A;
//               state = CONNECTED;
//             }
//             else if(ijce->port_B == jp && ijce->port_A == our_port)
//             {
//               jpid = ijce->port_id_B;
//               jpname = ijce->name_B;
//               state = CONNECTED;
//             }
//           break;
//           
//           case PortUnregister:
//             if(ijce->port_id_A == jpid)
//             {
//               jpid = 0;
//               jp = 0;
//               state = UNREGISTERED;
//             }
//           break;
// 
//           case PortDisconnect:
//             if((ijce->port_A == jp && ijce->port_B == our_port) || (ijce->port_B == jp && ijce->port_A == our_port))
//             {
//               ++err_cnt;
//               fprintf(stderr, "ERROR: processJackCallbackEvents state:DISCONNECTED command:PortDisconnect\n");
//             }
//           break;
// 
//           case PortRegister:
//             if(ijce->name_A == jpname)
//             {
//               ++err_cnt;
//               fprintf(stderr, "ERROR: processJackCallbackEvents state:DISCONNECTED command:PortRegister\n");
//             }
//           break;
// 
//           default:
//           break;
//         }
//       break;
//       
//       case DISCONNECTED_PENDING:
//         switch(ijce->type)
//         {
//           case GraphChanged:
//             state = DISCONNECTED;
//           break;
// 
//           case PortConnect:
//             if(ijce->port_A == jp && ijce->port_B == our_port)
//             {
//               jpid = ijce->port_id_A;
//               jpname = ijce->name_A;
//               state = CONNECTED;
//             }
//             else if(ijce->port_B == jp && ijce->port_A == our_port)
//             {
//               jpid = ijce->port_id_B;
//               jpname = ijce->name_B;
//               state = CONNECTED;
//             }
//           break;
// 
//           case PortUnregister:
//             if(ijce->port_id_A == jpid)
//             {
//               jpid = 0;
//               jp = 0;
//               state = PERSISTENT_UNREGISTERED;
//             }
//           break;
// 
//           case PortDisconnect:
//             if((ijce->port_A == jp && ijce->port_B == our_port) || (ijce->port_B == jp && ijce->port_A == our_port))
//             {
//               ++err_cnt;
//               fprintf(stderr, "ERROR: processJackCallbackEvents state:DISCONNECTED_PENDING command:PortDisconnect\n");
//             }
//           break;
// 
//           case PortRegister:
//             if(ijce->name_A == jpname)
//             {
//               ++err_cnt;
//               fprintf(stderr, "ERROR: processJackCallbackEvents state:DISCONNECTED_PENDING command:PortRegister\n");
//             }
//           break;
// 
//           default:
//           break;
//         }
//       break;
//       
//       case CONNECTED:
//         switch(ijce->type)
//         {
//           case PortDisconnect:
//             if(ijce->port_A == jp && ijce->port_B == our_port)
//             {
//               jpid = ijce->port_id_A;
//               jpname = ijce->name_A;
//               state = DISCONNECTED_PENDING;
//             }
//             else if(ijce->port_B == jp && ijce->port_A == our_port)
//             {
//               jpid = ijce->port_id_B;
//               jpname = ijce->name_B;
//               state = DISCONNECTED_PENDING;
//             }
//           break;
// 
//           case PortConnect:
//             if((ijce->port_A == jp && ijce->port_B == our_port) || (ijce->port_B == jp && ijce->port_A == our_port))
//             {
//               ++err_cnt;
//               fprintf(stderr, "ERROR: processJackCallbackEvents state:CONNECTED command:PortConnect\n");
//             }
//           break;
//           
//           //case PortUnregister:
//           //  if(ijce->name_A == jpname)
//           //  {
//           //    fprintf(stderr, "ERROR: processJackCallbackEvents state:CONNECTED command:PortUnregister\n");
//           //  }
//           //break;
//           
//           case PortRegister:
//             if(ijce->port_A == jp)
//             {
//               ++err_cnt;
//               fprintf(stderr, "ERROR: processJackCallbackEvents state:CONNECTED command:PortRegister\n");
//             }
//           break;
//           
//           default:
//           break;
//         }
//       break;
//       
//       case PERSISTENT_UNREGISTERED:
//         switch(ijce->type)
//         {
//           case PortRegister:
//             if(ijce->name_A == jpname)
//             {
//               jpid = ijce->port_id_A;
//               jp = jack_port_by_id(client, jpid);
//               state = REGISTERED;
//             }
//           break;
//           
//           default:
//           break;
//         }
//       break;
//       
//       case UNREGISTERED:
//         switch(ijce->type)
//         {
//           case PortRegister:
//             if(ijce->name_A == jpname)
//             {
//               jpid = ijce->port_id_A;
//               jp = jack_port_by_id(client, jpid);
//               state = REGISTERED;
//             }
//           break;
// 
//           default:
//           break;
//         }
//       break;
//       
//       case REGISTERED:
//         switch(ijce->type)
//         {
//           case PortConnect:
//             if(ijce->port_A == jp && ijce->port_B == our_port)
//             {
//               jpid = ijce->port_id_A;
//               jpname = ijce->name_A;
//               state = CONNECTED;
//             }
//             else if(ijce->port_B == jp && ijce->port_A == our_port)
//             {
//               jpid = ijce->port_id_B;
//               jpname = ijce->name_B;
//               state = CONNECTED;
//             }
//           break;
// 
//           case PortUnregister:
//             if(ijce->port_id_A == jpid)
//             {
//               jpid = 0;
//               jp = 0;
//               state = UNREGISTERED;
//             }
//           break;
// 
//           default:
//           break;
//         }
//       break;
//     }
//   }

//   if((state != init_state) || (jp != ext_node.jackPort) || (jpname != ext_node.persistentJackPortName)) // || (jpid != 0))
//   {
//     switch(state)
//     {
//       case UNREGISTERED:
//       case REGISTERED:
//       case DISCONNECTED:
//       case DISCONNECTED_PENDING:
//       {
//         // REMOVE Tim. Persistent routes. Added.
//         fprintf(stderr, "processJackCallbackEvents Deleting route: state:%d init_state:%d jp:%p ext_jp:%p jpname:%s ext_persistent_name:%s\n", 
//                 state, init_state, jp, ext_node.jackPort, jpname.toLatin1().constData(), ext_node.persistentJackPortName.toLatin1().constData());
//         if(is_input)
//           operations.add(PendingOperationItem(ext_node, our_node, PendingOperationItem::DeleteRoute));
//         else
//           operations.add(PendingOperationItem(our_node, ext_node, PendingOperationItem::DeleteRoute));
//       }
//       break;
//       
//       case PERSISTENT_UNREGISTERED:
//       case CONNECTED:
//       {
//         // REMOVE Tim. Persistent routes. Added.
//         fprintf(stderr, "processJackCallbackEvents Modifying route: state:%d init_state:%d jp:%p ext_jp:%p jpname:%s ext_persistent_name:%s\n", 
//                 state, init_state, jp, ext_node.jackPort, jpname.toLatin1().constData(), ext_node.persistentJackPortName.toLatin1().constData());
//         Route src(ext_node);
//         src.jackPort = jp;
//         src.persistentJackPortName = jpname;
//         operations.add(PendingOperationItem(src, &ext_node, PendingOperationItem::ModifyRouteNode));
//       }
//       break;
//     }
//   }
  
//   return err_cnt;
// }

// Last good
// //---------------------------------------------------------
// //   JackAudioDevice::graphChanged
// //    this is called from song in gui context triggered
// //    by graph_callback()
// //---------------------------------------------------------
// 
// void JackAudioDevice::graphChanged()
// {
//   if (JACK_DEBUG)
//         printf("graphChanged()\n");
//   // REMOVE Tim. Persistent routes. Added.
//   fprintf(stderr, "JackAudioDevice::graphChanged()\n"); 
// 
//   if(!checkJackClient(_client))
//   {
//     jackCallbackFifo.clear(); // Want this?
//     //jackOpsFifo.clear();      //?
//     return;
//   }
// 
//   //PendingOperationList operations;
// 
//   if(jack_ver_maj == 1)
//   {
//     while(!jackOpsFifo.isEmpty())
//       operations.push_back(jackOpsFifo.get());
//     if(!operations.empty())
//     {
//       MusEGlobal::audio->msgExecutePendingOperations(operations);
//       operations.clear();
//     }
//     return;
//   }
//   
//   
//   // For Jack-1 only: See if we need to wait, for example for a port unregister event...
//   if(MusEGlobal::audio)
//   {
//     if(jack_ver_maj != 1)  // Jack "2" does not require waiting, Jack "1" does, assume any other version requires the wait (no harm).
//     {
//       // TODO: It may be desirable to always wait so that bunches of commands can be processed easier.
//       bool do_audio_wait = false;
//       // This is safe because the writer only increases the size.
//       int cb_fifo_sz = jackCallbackFifo.getSize();
//       for(int i = 0; i < cb_fifo_sz; ++i)
//       {
//         const JackCallbackEvent& jcb = jackCallbackFifo.peek(i);
//         if(jcb.type == PortDisconnect)
//         {
//           InputList* il = MusEGlobal::song->inputs();
//           for(iAudioInput ii = il->begin(); ii != il->end(); ++ii) 
//           {
//             AudioInput* it = *ii;
//             int channels = it->channels();
//             for(int channel = 0; channel < channels; ++channel) 
//             {
//               jack_port_t* port = (jack_port_t*)(it->jackPort(channel));
//               if(port == jcb.port_A || port == jcb.port_B)
//               {
//                 do_audio_wait = true;
//                 goto out_of_loop;
//               }
//             }
//           }
//           
//           OutputList* ol = MusEGlobal::song->outputs();
//           for(iAudioOutput ii = ol->begin(); ii != ol->end(); ++ii) 
//           {
//             AudioOutput* it = *ii;
//             int channels = it->channels();
//             for(int channel = 0; channel < channels; ++channel) 
//             {
//               jack_port_t* port = (jack_port_t*)(it->jackPort(channel));
//               if(port == jcb.port_A || port == jcb.port_B)
//               {
//                 do_audio_wait = true;
//                 goto out_of_loop;
//               }
//             }
//           }
//           
//           for(iMidiDevice ii = MusEGlobal::midiDevices.begin(); ii != MusEGlobal::midiDevices.end(); ++ii) 
//           {
//             MidiDevice* md = *ii;
//             if(md->deviceType() != MidiDevice::JACK_MIDI)
//               continue;
//             // Midi outputs:
//             if(md->rwFlags() & 1) // Writable
//             {
//               jack_port_t* port = (jack_port_t*)md->outClientPort();
//               if(port == jcb.port_A || port == jcb.port_B)
//               {
//                 do_audio_wait = true;
//                 goto out_of_loop;
//               }
//             }  
//             // Midi inputs:
//             if(md->rwFlags() & 2) // Readable
//             {
//               jack_port_t* port = (jack_port_t*)md->inClientPort();
//               if(port == jcb.port_A || port == jcb.port_B)
//               {
//                 do_audio_wait = true;
//                 goto out_of_loop;
//               }
//             }  
//           }
//         }
//       }
//     
//       out_of_loop:
//       if(do_audio_wait)
//       {
//         // REMOVE Tim. Persistent routes. Added.
//         fprintf(stderr, "JackAudioDevice::graphChanged: *** calling msgAudioWait()\n");
//         MusEGlobal::audio->msgAudioWait(); // Wait until upcoming process call has finished...
//       }
//     }
//   }
// 
//   // Find the last GraphChanged event, if any.
//   // This is safe because the writer only increases the size.
//   int last_gc_idx = -1;
//   int cb_fifo_sz = jackCallbackFifo.getSize();
//   for(int i = 0; i < cb_fifo_sz; ++i)
//   {
//     if(jackCallbackFifo.peek(i).type == GraphChanged)
//       last_gc_idx = i;
//   }
//   if(last_gc_idx == -1)
//     return;
// 
//   // Move the events into a list for processing, including the final GraphChanged event.
//   // Leave any 'still in progress' ending events (without closing GraphChanged event) in the ring buffer.
//   //JackCallbackEventList jce_list;
//   jackCallbackEventList.clear();
//   for(int i = 0; i <= last_gc_idx; ++i)
//     jackCallbackEventList.push_back(jackCallbackFifo.get());
//   
//   //---------------------------------------
//   // Audio inputs:
//   //---------------------------------------
//   
//   InputList* il = MusEGlobal::song->inputs();
//   for(iAudioInput ii = il->begin(); ii != il->end(); ++ii) 
//   {
//     AudioInput* it = *ii;
//     int channels = it->channels();
//     for(int channel = 0; channel < channels; ++channel) 
//     {
//       jack_port_t* port = (jack_port_t*)(it->jackPort(channel));
//       if(port == 0)
//         continue;
// //       const char** ports = jack_port_get_all_connections(_client, port);
// //       RouteList* rl      = it->inRoutes();
// 
//       //---------------------------------------
//       // check for disconnects
//       //---------------------------------------
// 
// //       for(iRoute irl = rl->begin(); irl != rl->end(); ++irl) 
// //       {
// //         if(irl->type != Route::JACK_ROUTE)  
// //           continue;
// //         if (irl->channel != channel)
// //           continue;
// //         processJackCallbackEvents(_client, jce_list, *irl, port, Route(it, channel), true, operations);
// //       }      
//       //processJackCallbackEvents(this, jce_list, Route(it, channel), port, operations, it->inRoutes(), true);
//       //processJackCallbackEvents(this, Route(it, channel), port, operations, it->inRoutes(), true);
//       processJackCallbackEvents(Route(it, channel), port, operations, it->inRoutes(), true);
// 
//       //---------------------------------------
//       // check for connects
//       //---------------------------------------
//       
// //       if(ports) 
// //       {
// //         const char** pn = ports;
// //         while(*pn) 
// //         {
// //           bool found = false;
// //           for(ciRoute irl = rl->begin(); irl != rl->end(); ++irl) 
// //           {
// //             if(irl->type != Route::JACK_ROUTE)  
// //               continue;
// //             if(irl->channel != channel)
// //               continue;
// //             const char* portName = irl->name().toLatin1().constData();
// //             if(strcmp(*pn, portName) == 0) 
// //             {
// //               found = true;
// //               break;
// //             }
// //           }
// //           if(!found) 
// //           {
// //             Route src(*pn, false, channel, Route::JACK_ROUTE);
// //             Route dst(it, channel);
// //             operations.add(PendingOperationItem(src, dst, PendingOperationItem::AddRoute));
// //           }
// //           ++pn;
// //         }
// // 
// //         jack_free(ports);
// //         ports = NULL;
// //       }
//     }
//   }
//         
//         
//   //---------------------------------------
//   // Audio outputs:
//   //---------------------------------------
//   
//   OutputList* ol = MusEGlobal::song->outputs();
//   for(iAudioOutput ii = ol->begin(); ii != ol->end(); ++ii) 
//   {
//     AudioOutput* it = *ii;
//     int channels = it->channels();
//     for(int channel = 0; channel < channels; ++channel) 
//     {
//       jack_port_t* port = (jack_port_t*)(it->jackPort(channel));
//       if(port == 0)
//         continue;
// //       const char** ports = jack_port_get_all_connections(_client, port);
// //       RouteList* rl      = it->outRoutes();
// 
//       //---------------------------------------
//       // check for disconnects
//       //---------------------------------------
// 
// //       for(iRoute irl = rl->begin(); irl != rl->end(); ++irl) 
// //       {
// //         if(irl->type != Route::JACK_ROUTE)  
// //           continue;
// //         if (irl->channel != channel)
// //               continue;
// //         processJackCallbackEvents(_client, jce_list, *irl, port, Route(it, channel), false, operations);
// //       }
//       //processJackCallbackEvents(this, jce_list, Route(it, channel), port, operations, it->outRoutes(), false);
//       //processJackCallbackEvents(this, Route(it, channel), port, operations, it->outRoutes(), false);
//       processJackCallbackEvents(Route(it, channel), port, operations, it->outRoutes(), false);
// 
//       //---------------------------------------
//       // check for connects
//       //---------------------------------------
// 
// //       if(ports) 
// //       {
// //         const char** pn = ports;
// //         while(*pn) 
// //         {
// //           bool found = false;
// //           for(ciRoute irl = rl->begin(); irl != rl->end(); ++irl) 
// //           {
// //             if(irl->type != Route::JACK_ROUTE)  
// //               continue;
// //             if(irl->channel != channel)
// //               continue;
// //             const char* portName = irl->name().toLatin1().constData();
// //             if(strcmp(*pn, portName) == 0) 
// //             {
// //               found = true;
// //               break;
// //             }
// //           }
// //           if(!found) 
// //           {
// //             Route src(it, channel);
// //             Route dst(*pn, true, channel, Route::JACK_ROUTE);
// //             operations.add(PendingOperationItem(src, dst, PendingOperationItem::AddRoute));
// //           }
// //           ++pn;
// //         }
// // 
// //         jack_free(ports);
// //         ports = NULL;
// //       }
//     }
//   }
//         
//   //---------------------------------------
//   // Midi devices:
//   //---------------------------------------
//   
//   for(iMidiDevice ii = MusEGlobal::midiDevices.begin(); ii != MusEGlobal::midiDevices.end(); ++ii) 
//   {
//     MidiDevice* md = *ii;
//     if(md->deviceType() != MidiDevice::JACK_MIDI)
//       continue;
//     
//     //---------------------------------------
//     // Midi outputs:
//     //---------------------------------------
//     
//     if(md->rwFlags() & 1) // Writable
//     {
//       jack_port_t* port = (jack_port_t*)md->outClientPort();
//       if(port != 0)
//       {
// //         const char** ports = jack_port_get_all_connections(_client, port);
// //         RouteList* rl      = md->outRoutes();
// 
//         //---------------------------------------
//         // check for disconnects
//         //---------------------------------------
// 
// //         for(iRoute irl = rl->begin(); irl != rl->end(); ++irl) 
// //         {
// //           if(irl->type != Route::JACK_ROUTE)  
// //             continue;
// //           //if (irl->channel != channel)
// //           //      continue;
// //           processJackCallbackEvents(_client, jce_list, *irl, port, Route(md, -1), true, operations);
// //         }
//         //processJackCallbackEvents(this, jce_list, Route(md, -1), port, operations, md->outRoutes(), false);
//         //processJackCallbackEvents(this, Route(md, -1), port, operations, md->outRoutes(), false);
//         processJackCallbackEvents(Route(md, -1), port, operations, md->outRoutes(), false);
//         
//         //---------------------------------------
//         // check for connects
//         //---------------------------------------
// 
// //         if(ports) 
// //         {
// //           const char** pn = ports;
// //           while(*pn)
// //           {
// //             bool found = false;
// //             for(ciRoute irl = rl->begin(); irl != rl->end(); ++irl) 
// //             {
// //               if(irl->type != Route::JACK_ROUTE)  
// //                 continue;
// //               //if (irl->channel != channel)
// //               //      continue;
// //               QString name = irl->name();
// //               QByteArray ba = name.toLatin1();
// //               const char* portName = ba.constData();
// //               if(strcmp(*pn, portName) == 0) 
// //               {
// //                 found = true;
// //                 break;
// //               }
// //             }
// //             if(!found)
// //             {
// //               Route src(md, -1);
// //               Route dst(*pn, true, -1, Route::JACK_ROUTE);
// //               operations.add(PendingOperationItem(src, dst, PendingOperationItem::AddRoute));
// //             }
// //             ++pn;
// //           }
// // 
// //           jack_free(ports);
// //           ports = NULL;
// //         }
//       }  
//     }  
//           
//           
//     //------------------------
//     // Midi inputs:
//     //------------------------
//     
//     if(md->rwFlags() & 2) // Readable
//     {
//       jack_port_t* port = (jack_port_t*)md->inClientPort();
//       if(port != 0)
//       {
// //         const char** ports = jack_port_get_all_connections(_client, port);
// //         RouteList* rl = md->inRoutes();
// 
//         //---------------------------------------
//         // check for disconnects
//         //---------------------------------------
// 
// //         for(iRoute irl = rl->begin(); irl != rl->end(); ++irl) 
// //         {
// //           if(irl->type != Route::JACK_ROUTE)  
// //             continue;
// //           //if (irl->channel != channel)
// //           //      continue;
// //           processJackCallbackEvents(_client, jce_list, *irl, port, Route(md, -1), false, operations);
// //         }
//         //processJackCallbackEvents(this, jce_list, Route(md, -1), port, operations, md->inRoutes(), true);
//         //processJackCallbackEvents(this, Route(md, -1), port, operations, md->inRoutes(), true);
//         processJackCallbackEvents(Route(md, -1), port, operations, md->inRoutes(), true);
// 
//         //---------------------------------------
//         // check for connects
//         //---------------------------------------
// 
// //         if(ports) 
// //         {
// //           const char** pn = ports;
// //           while(*pn) 
// //           {
// //             bool found = false;
// //             for(ciRoute irl = rl->begin(); irl != rl->end(); ++irl) 
// //             {
// //               if(irl->type != Route::JACK_ROUTE)  
// //                 continue;
// //               //if (irl->channel != channel)
// //               //      continue;
// //               QString name = irl->name();
// //               QByteArray ba = name.toLatin1();
// //               const char* portName = ba.constData();
// //               if(strcmp(*pn, portName) == 0) 
// //               {
// //                 found = true;
// //                 break;
// //               }
// //             }
// //             if(!found) 
// //             {
// //               Route src(*pn, false, -1, Route::JACK_ROUTE);
// //               Route dst(md, -1);
// //               operations.add(PendingOperationItem(src, dst, PendingOperationItem::AddRoute));
// //             }
// //             ++pn;
// //           }
// //                 
// //           jack_free(ports);
// //           ports = NULL;
// //         }
//       }  
//     }  
//   }
// 
//   // Copy all the failed connections during this graphChanged over to the list for next graphChanged.
//   failedConnects = failedConnectsTemp;
//   // Clear the temporary list.
//   failedConnectsTemp.clear();
//   
//   if(!operations.empty())
//   {
//     MusEGlobal::audio->msgExecutePendingOperations(operations);
//     operations.clear();
//   }
// }

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
  fprintf(stderr, "JackAudioDevice::graphChanged()\n"); 

  if(!checkJackClient(_client))
  {
    jackCallbackFifo.clear(); // Want this?
    //jackOpsFifo.clear();      // Want this?
    // Reset this now.
    muse_atomic_set(&atomicGraphChangedPending, 0);
    return;
  }

  //PendingOperationList operations;

//   if(jack_ver_maj == 1) // Jack "2"
//   {
//     while(!jackOpsFifo.isEmpty())
//     {
//       const JackOperation& rs = jackOpsFifo.get();
//       switch(rs.type)
//       {
//         case JCB_CONNECT:
//         {
//           const char* our_name = jack_port_name(rs.our_port);
//           if(our_name)
//           {
//             // REMOVE Tim. Persistent routes. Added.
//             fprintf(stderr, " operation: JCB_CONNECT our_port_name:%s route_persistent_name:%s\n", 
//                       our_name, rs.route->persistentJackPortName);
//             int err;
//             if(rs.is_input)
//               err = jack_connect(_client, rs.route->persistentJackPortName, our_name);
//             else
//               err = jack_connect(_client, our_name, rs.route->persistentJackPortName);
//             if(err)
//             {
//               // REMOVE Tim. Persistent routes. Added.
//               fprintf(stderr, " ERROR:%d on jack_connect()\n", 
//                       err, our_name, rs.route->persistentJackPortName);
//               //failedConnectsTemp.push_back(jp);
//             }
//             else
//               // We have our jack port, on a supposedly active client. Update the route node's jack port pointer.
//               operations.add(PendingOperationItem(rs.work_route, rs.route, PendingOperationItem::ModifyRouteNode));
//           }
//         }
//         break;
//         
//         case JCB_MODIFY:
//           // REMOVE Tim. Persistent routes. Added.
//           fprintf(stderr, " operation: JCB_MODIFY\n"); 
//           operations.add(PendingOperationItem(rs.work_route, rs.route, PendingOperationItem::ModifyRouteNode));
//         break;  
//         
//         case JCB_DELETE:
//           // REMOVE Tim. Persistent routes. Added.
//           fprintf(stderr, " operation: JCB_DELETE\n"); 
//           operations.add(PendingOperationItem(rs.route_list, rs.iroute, PendingOperationItem::DeleteRouteNode));
//         break;  
//         
//         case JCB_ADD: // Done later
//         case JCB_UNCHANGED:
//         break;  
//       }
//     }
//   }
//   else
  // For Jack-1 only: See if we need to wait, for example for a port unregister event.
  // Jack "2" does not require waiting, Jack "1" does, assume any other version requires the wait (no harm).
  if(MusEGlobal::audio && jack_ver_maj != 1) 
  {
    //if(jack_ver_maj != 1)  // Jack "2" does not require waiting, Jack "1" does, assume any other version requires the wait (no harm).
    //if(MusEGlobal::audio)
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
          fprintf(stderr, "JackAudioDevice::graphChanged: *** calling msgAudioWait()\n");
          MusEGlobal::audio->msgAudioWait(); // Wait until upcoming process call has finished...
          break;
        }
      } 
    }
  }
        
//         if(jcb.type == PortDisconnect)
//         {
//           InputList* il = MusEGlobal::song->inputs();
//           for(iAudioInput ii = il->begin(); ii != il->end(); ++ii) 
//           {
//             AudioInput* it = *ii;
//             int channels = it->channels();
//             for(int channel = 0; channel < channels; ++channel) 
//             {
//               jack_port_t* port = (jack_port_t*)(it->jackPort(channel));
//               if(port == jcb.port_A || port == jcb.port_B)
//               {
//                 do_audio_wait = true;
//                 goto out_of_loop;
//               }
//             }
//           }
//           
//           OutputList* ol = MusEGlobal::song->outputs();
//           for(iAudioOutput ii = ol->begin(); ii != ol->end(); ++ii) 
//           {
//             AudioOutput* it = *ii;
//             int channels = it->channels();
//             for(int channel = 0; channel < channels; ++channel) 
//             {
//               jack_port_t* port = (jack_port_t*)(it->jackPort(channel));
//               if(port == jcb.port_A || port == jcb.port_B)
//               {
//                 do_audio_wait = true;
//                 goto out_of_loop;
//               }
//             }
//           }
//           
//           for(iMidiDevice ii = MusEGlobal::midiDevices.begin(); ii != MusEGlobal::midiDevices.end(); ++ii) 
//           {
//             MidiDevice* md = *ii;
//             if(md->deviceType() != MidiDevice::JACK_MIDI)
//               continue;
//             // Midi outputs:
//             if(md->rwFlags() & 1) // Writable
//             {
//               jack_port_t* port = (jack_port_t*)md->outClientPort();
//               if(port == jcb.port_A || port == jcb.port_B)
//               {
//                 do_audio_wait = true;
//                 goto out_of_loop;
//               }
//             }  
//             // Midi inputs:
//             if(md->rwFlags() & 2) // Readable
//             {
//               jack_port_t* port = (jack_port_t*)md->inClientPort();
//               if(port == jcb.port_A || port == jcb.port_B)
//               {
//                 do_audio_wait = true;
//                 goto out_of_loop;
//               }
//             }  
//           }
//         }
//       }
//     
//       out_of_loop:
//       if(do_audio_wait)
//       {
//         // REMOVE Tim. Persistent routes. Added.
//         fprintf(stderr, "JackAudioDevice::graphChanged: *** calling msgAudioWait()\n");
//         MusEGlobal::audio->msgAudioWait(); // Wait until upcoming process call has finished...
//       }
//     }
//   }

  // Reset this now.
  muse_atomic_set(&atomicGraphChangedPending, 0);
  
  // Find the last GraphChanged event, if any.
  // This is safe because the writer only increases the size.
  //int last_gc_idx = -1;
  int cb_fifo_sz = jackCallbackFifo.getSize();
  if(cb_fifo_sz)
  {
    int last_gc_idx = cb_fifo_sz - 1;
    if(jack_ver_maj == 1)
      for(int i = 0; i < cb_fifo_sz; ++i)
        if(jackCallbackFifo.peek(i).type == GraphChanged)
          last_gc_idx = i;
      
    //if(last_gc_idx != -1)
    //{
      // Move the events into a list for processing, including the final GraphChanged event.
      // Leave any 'still in progress' ending events (without closing GraphChanged event) in the ring buffer.
      //JackCallbackEventList jce_list;
      jackCallbackEvents.clear();
      for(int i = 0; i <= last_gc_idx; ++i)
        jackCallbackEvents.push_back(jackCallbackFifo.get());
    //}      
  }
  processGraphChanges();
  
  //checkNewConnections();  
  
//   if(jack_ver_maj == 1) // Jack "2"
//   {
    // Copy all the failed connections during this graphChanged over to the list for next graphChanged.
    failedConnects = failedConnectsTemp;
    // Clear the temporary list.
    failedConnectsTemp.clear();
//   }
    
  if(!operations.empty())
  {
    MusEGlobal::audio->msgExecutePendingOperations(operations);
    operations.clear();
  }
}

void JackAudioDevice::processGraphChanges()
{
  // REMOVE Tim. Persistent routes. Added.
  fprintf(stderr, "JackAudioDevice::processGraphChanges()\n");
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
//       // See if Jack-1 changed the port pointer by adding the port to Jack-1's external ports list!
//       if(jack_ver_maj == 0)
//       {
//         jack_port_t* jp = jack_port_by_name(_client, jack_port_name(port));
//         if(jp && jp != port)
//         {
//           // REMOVE Tim. Persistent routes. Added.
//           fprintf(stderr, " changing audio input port!: channel:%d our_port:%p new port:%p\n", channel, port, jp);
//           //operations.add(PendingOperationItem(route_list, r, PendingOperationItem::AddRouteNode)); // TODO
//           port = jp;
//         }
//       }
      //processJackCallbackEvents(Route(it, channel), port, operations, it->inRoutes(), true);
      processJackCallbackEvents(Route(it, channel), port, it->inRoutes(), true);
      //checkNewRouteConnections(port, channel, it->inRoutes());
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
//       // See if Jack-1 changed the port pointer by adding the port to Jack-1's external ports list!
//       if(jack_ver_maj == 0)
//       {
//         jack_port_t* jp = jack_port_by_name(_client, jack_port_name(port));
//         if(jp && jp != port)
//         {
//           // REMOVE Tim. Persistent routes. Added.
//           fprintf(stderr, " changing audio output port!: channel:%d our_port:%p new port:%p\n", channel, port, jp);
//           //operations.add(PendingOperationItem(route_list, r, PendingOperationItem::AddRouteNode)); // TODO
//           port = jp;
//         }
//       }
      //processJackCallbackEvents(Route(it, channel), port, operations, it->outRoutes(), false);
      processJackCallbackEvents(Route(it, channel), port, it->outRoutes(), false);
      //checkNewRouteConnections(port, channel, it->outRoutes());
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
      {
//         // See if Jack-1 changed the port pointer by adding the port to Jack-1's external ports list!
//         if(jack_ver_maj == 0)
//         {
//           jack_port_t* jp = jack_port_by_name(_client, jack_port_name(port));
//           if(jp && jp != port)
//           {
//             // REMOVE Tim. Persistent routes. Added.
//             fprintf(stderr, " changing midi output port!: our_port:%p new port:%p\n", port, jp);
//             //operations.add(PendingOperationItem(route_list, r, PendingOperationItem::AddRouteNode)); // TODO
//             port = jp;
//           }
//         }
        //processJackCallbackEvents(Route(md, -1), port, operations, md->outRoutes(), false);
        processJackCallbackEvents(Route(md, -1), port, md->outRoutes(), false);
        //checkNewRouteConnections(port, -1, md->outRoutes());
      }  
    }  
          
          
    //------------------------
    // Midi inputs:
    //------------------------
    
    if(md->rwFlags() & 2) // Readable
    {
      jack_port_t* port = (jack_port_t*)md->inClientPort();
      if(port != 0)
      {
//         // See if Jack-1 changed the port pointer by adding the port to Jack-1's external ports list!
//         if(jack_ver_maj == 0)
//         {
//           jack_port_t* jp = jack_port_by_name(_client, jack_port_name(port));
//           if(jp && jp != port)
//           {
//             // REMOVE Tim. Persistent routes. Added.
//             fprintf(stderr, " changing midi input port!: our_port:%p new port:%p\n", port, jp);
//             //operations.add(PendingOperationItem(route_list, r, PendingOperationItem::AddRouteNode)); // TODO
//             port = jp;
//           }
//         }
        //processJackCallbackEvents(Route(md, -1), port, operations, md->inRoutes(), true);
        processJackCallbackEvents(Route(md, -1), port, md->inRoutes(), true);
        //checkNewRouteConnections(port, -1, md->inRoutes());
      }  
    }  
  }

//   // Copy all the failed connections during this graphChanged over to the list for next graphChanged.
//   failedConnects = failedConnectsTemp;
//   // Clear the temporary list.
//   failedConnectsTemp.clear();
//   
//   if(!operations.empty())
//   {
//     MusEGlobal::audio->msgExecutePendingOperations(operations);
//     operations.clear();
//   }
}

void JackAudioDevice::checkNewRouteConnections(jack_port_t* our_port, int channel, RouteList* route_list)
{
  // REMOVE Tim. Persistent routes. Added.
  fprintf(stderr, "JackAudioDevice::checkNewRouteConnections(): our_port:%p channel:%d route_list:%p\n", 
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
                //if(ipo->_route_list == route_list && *ipo->_iRoute == *ir)
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
          fprintf(stderr, " adding route: route_jp:%p portname:%s route_persistent_name:%s\n", 
                  jp, *pn, r.persistentJackPortName);
          operations.add(PendingOperationItem(route_list, r, PendingOperationItem::AddRouteNode));
        }
      }
      ++pn;
    }
    jack_free(ports);
  }
}

// void JackAudioDevice::checkNewConnections()
// {
//   // REMOVE Tim. Persistent routes. Added.
//   fprintf(stderr, "JackAudioDevice::checkNewConnections()\n");
//   //---------------------------------------
//   // Audio inputs:
//   //---------------------------------------
//   
//   InputList* il = MusEGlobal::song->inputs();
//   for(iAudioInput ii = il->begin(); ii != il->end(); ++ii) 
//   {
//     AudioInput* it = *ii;
//     int channels = it->channels();
//     for(int channel = 0; channel < channels; ++channel) 
//     {
//       jack_port_t* port = (jack_port_t*)(it->jackPort(channel));
//       if(port)
//         checkNewRouteConnections(port, channel, it->inRoutes());
//     }
//   }
//         
//   //---------------------------------------
//   // Audio outputs:
//   //---------------------------------------
//   
//   OutputList* ol = MusEGlobal::song->outputs();
//   for(iAudioOutput ii = ol->begin(); ii != ol->end(); ++ii) 
//   {
//     AudioOutput* it = *ii;
//     int channels = it->channels();
//     for(int channel = 0; channel < channels; ++channel) 
//     {
//       jack_port_t* port = (jack_port_t*)(it->jackPort(channel));
//       if(port)
//         checkNewRouteConnections(port, channel, it->outRoutes());
//     }
//   }
//         
//   //---------------------------------------
//   // Midi devices:
//   //---------------------------------------
//   
//   for(iMidiDevice ii = MusEGlobal::midiDevices.begin(); ii != MusEGlobal::midiDevices.end(); ++ii) 
//   {
//     MidiDevice* md = *ii;
//     if(md->deviceType() != MidiDevice::JACK_MIDI)
//       continue;
//     
//     //---------------------------------------
//     // Midi outputs:
//     //---------------------------------------
//     
//     if(md->rwFlags() & 1) // Writable
//     {
//       jack_port_t* port = (jack_port_t*)md->outClientPort();
//       if(port)
//         checkNewRouteConnections(port, -1, md->outRoutes());
//     }  
//           
//     //------------------------
//     // Midi inputs:
//     //------------------------
//     
//     if(md->rwFlags() & 2) // Readable
//     {
//       jack_port_t* port = (jack_port_t*)md->inClientPort();
//       if(port)
//         checkNewRouteConnections(port, -1, md->inRoutes());
//     }  
//   }
// }

int JackAudioDevice::checkDisconnectCallback(const jack_port_t* our_port, const jack_port_t* port)
{
  // REMOVE Tim. Persistent routes. Added.
  fprintf(stderr, "JackAudioDevice::checkDisconnectCallback(): our_port:%p port:%p\n", our_port, port); 
  
  iJackCallbackEvent ijce = jackCallbackEvents.end();
  while(ijce != jackCallbackEvents.begin())
  {
    --ijce;
    
//     if(ijce->type == PortConnect)
//     {
//       jack_port_t* p_a = jack_port_by_id(_client, ijce->port_id_A);
//       jack_port_t* p_b = jack_port_by_id(_client, ijce->port_id_B);
//       // REMOVE Tim. Persistent routes. Added.
//       fprintf(stderr, "JackAudioDevice::checkDisconnectCallback(): PortConnect: port a:%p port b:%p\n", p_a, p_b); 
//     }
//     else if(ijce->type == PortDisconnect)
//     {
//       jack_port_t* p_a = jack_port_by_id(_client, ijce->port_id_A);
//       jack_port_t* p_b = jack_port_by_id(_client, ijce->port_id_B);
//       // REMOVE Tim. Persistent routes. Added.
//       fprintf(stderr, "JackAudioDevice::checkDisconnectCallback(): PortDisconnect: port a:%p port b:%p\n", p_a, p_b); 
//     }


    
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
  fprintf(stderr, "JackAudioDevice::checkPortRegisterCallback(): port:%p\n", port); 
      
  iJackCallbackEvent ijce = jackCallbackEvents.end();
  while(ijce != jackCallbackEvents.begin())
  {
    --ijce;
    if(ijce->type == PortRegister)
    {
      jack_port_id_t id = ijce->port_id_A;
      
//       jack_port_t* p = jack_port_by_id(_client, id);
//       // REMOVE Tim. Persistent routes. Added.
//       fprintf(stderr, "JackAudioDevice::checkPortRegisterCallback(): PortRegister: port_by_id: port id:%d port:%p\n", id, p); 
      
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
      // REMOVE Tim. Persistent routes. Added. Can't use this! Incompatible in jack 1/2 and unimplemented in jack1.
      //jack_set_port_rename_callback(_client, port_rename_callback, 0);
      // Jack1:
      //typedef void (*JackPortRenameCallback)(jack_port_id_t port, const char* old_name, const char* new_name, void* arg);
      // Jack2:
      //typedef int (*JackPortRenameCallback)(jack_port_id_t port, const char* old_name, const char* new_name, void *arg);
      
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
      // Force Jack-1 to add the port to Jack-1's external port list NOW instead of 
      //  doing it behind our backs later. Otherwise, the port returned by jack_port_by_name()
      //  later will NOT match the port returned by jack_port_register() here !!!
      //if(p && jack_ver_maj == 0)
      //  p = jack_port_by_name(_client, name);
      printf("JACK: registerInPort: <%s> %p\n", name, p);
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
      // Force Jack-1 to add the port to Jack-1's external port list NOW instead of 
      //  doing it behind our backs later. Otherwise, the port returned by jack_port_by_name()
      //  later will NOT match the port returned by jack_port_register() here !!!
      //if(p && jack_ver_maj == 0)
      //  p = jack_port_by_name(_client, name);
      printf("JACK: registerOutPort: <%s> %p\n", name, p);
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

// REMOVE Tim. Persistent routes. Added.
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

// REMOVE Tim. Persistent routes. Added.
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

// REMOVE Tim. Persistent routes. Removed.
// //---------------------------------------------------------
// //   JackAudioDevice::connectJackMidiPorts
// //---------------------------------------------------------
// 
// void JackAudioDevice::connectJackMidiPorts()
// {
//   if(JACK_DEBUG)
//     printf("JackAudioDevice::connectJackMidiPorts()\n");
//   
//   for (iMidiDevice i = MusEGlobal::midiDevices.begin(); i != MusEGlobal::midiDevices.end(); ++i) 
//   {
//     //MidiJackDevice* mjd = dynamic_cast<MidiJackDevice*>(*i);
//     //if(!mjd)
//     MidiDevice* md = *i;
//     if(md->deviceType() != MidiDevice::JACK_MIDI)
//       continue;
//     
//     //void* port = md->clientPort();
//     if(md->rwFlags() & 1)
//     {
//       void* port = md->outClientPort(); 
//       if(port)                           
//       {
//         RouteList* rl = md->outRoutes();
//         for (ciRoute r = rl->begin(); r != rl->end(); ++r) 
//         {  
//           if(r->type != Route::JACK_ROUTE)  
//             continue;
//           connect(port, r->jackPort);
//         }  
//       }    
//     }
//     
//     if(md->rwFlags() & 2)
//     {  
//       void* port = md->inClientPort();  
//       if(port)                          
//       {
//         RouteList* rl = md->inRoutes();
//         for (ciRoute r = rl->begin(); r != rl->end(); ++r) 
//         {  
//           if(r->type != Route::JACK_ROUTE)  
//             continue;
//           connect(r->jackPort, port);
//         }
//       }    
//     }    
//   }
// }

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
      fprintf (stderr, "JackAudioDevice::start(): calling jack_activate()\n");
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

// REMOVE Tim. Persistent routes. Removed.      
//       InputList* il = MusEGlobal::song->inputs();
//       for (iAudioInput i = il->begin(); i != il->end(); ++i) {
//             AudioInput* ai = *i;
//             int channel = ai->channels();
//             for (int ch = 0; ch < channel; ++ch) {
//                   RouteList* rl = ai->inRoutes();
//                   void* port = ai->jackPort(ch);
//                   for (ciRoute ir = rl->begin(); ir != rl->end(); ++ir) {
//                         if(ir->type != Route::JACK_ROUTE)  
//                           continue;
//                         if (ir->channel == ch)
//                               connect(ir->jackPort, port);
//                         }
//                   }
//             }
//       OutputList* ol = MusEGlobal::song->outputs();
//       for (iAudioOutput i = ol->begin(); i != ol->end(); ++i) {
//             AudioOutput* ai = *i;
//             int channel = ai->channels();
//             for (int ch = 0; ch < channel; ++ch) {
//                   RouteList* rl = ai->outRoutes();
//                   void* port = ai->jackPort(ch);
//                   for (ciRoute r = rl->begin(); r != rl->end(); ++r) {
//                         if(r->type != Route::JACK_ROUTE)  
//                           continue;
//                         if (r->channel == ch) {
//                               connect(port, r->jackPort);
//                               }
//                         }
//                   }
//             }
//       
//       // Connect the Jack midi client ports to device ports.
//       connectJackMidiPorts();
      
      MusEGlobal::undoSetuid();
      
      // REMOVE Tim. Persistent routes. Added.
      MusEGlobal::song->connectAllPorts();
      // Call the graphChanged handler, it will take care of all new and restoreable routes.
      //graphChanged();
      
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
      // REMOVE Tim. Persistent routes. Added.
      fprintf (stderr, "JackAudioDevice::stop(): calling jack_deactivate()\n");
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
            // Caution: In Jack-1, under some conditions jack_port_by_name() might ALLOCATE a new port for use later if not found! 
            // Should be safe and quick search here, we know that the port name is valid.
            jack_port_t* port = jack_port_by_name(_client, *p);
            //int flags = jack_port_flags(port);
            //if (!(flags & JackPortIsInput))
            //      continue;
            //char buffer[128];
            
            // REMOVE Tim. Persistent routes. Added.
            //jack_uuid_t j_uuid = jack_port_uuid(_client, port_id); // Crud! Not on my Jack2 yet. Function added to Jack2 in May/14.
            //fprintf(stderr, "JACK: registration changed: port_id:%d is_register:%d\n", port_id, is_register);
  
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

// REMOVE Tim. Persistent routes. Removed.
// //      
// //---------------------------------------------------------
// //   portName
// //   Returns name of port and sets success.
// // REMOVE Tim. Persistent routes. Added.
// //   This method consults a blacklist of client names, 
// //    such as "system:", whether to pick the name or 
// //    one of the aliases, whichever does NOT contain 
// //    the blacklist names.
// //---------------------------------------------------------
// 
// QString JackAudioDevice::portName(void* port, bool* success)
// {
//       //if (JACK_DEBUG)
//       //      printf("JackAudioDevice::portName\n");
//       
// // REMOVE Tim. Persistent routes. Removed.
// //       if(!checkJackClient(_client)) return "";
// //       if (!port) 
// //             return "";
// //       
// //       QString s(jack_port_name((jack_port_t*)port));
// //       //printf("Jack::portName %p %s\n", port, s.toLatin1());
// //       return s;
// 
// 
// // REMOVE Tim. Persistent routes. Removed.
// //       bool res = false;  
// //       QString s;
// //       
// //       if(port && checkJackClient(_client))
// //       {
// //         const char* name = jack_port_name((jack_port_t*)port);
// //         if(name)
// //         {
// //           s = QString(name);
// //           res = true;
// //         }
// //       }
// //       
// //       if(success)
// //         *success = res;
// //       
// //       //fprintf(stderr, "Jack::portName %p %s\n", port, s.toLatin1());
// //       return s;
//       
//       
// // REMOVE Tim. Persistent routes. Added.
// //   if(!port || !checkJackClient(_client))
// //   {
// //     if(success)
// //       *success = false;
// //     return QString();
// //   }
// //   int nsz = jack_port_name_size();
// //   QString fin_name;
// //   const char* p_name = jack_port_name((jack_port_t*)port);
// //   if(p_name)
// //   {
// //     if(strncmp(p_name, "system:", 7) == 0)   // TODO: Make this a user editable blacklist of client names!
// //     {
// //       char a1[nsz]; 
// //       char a2[nsz]; 
// //       char* al[2]; 
// //       al[0] = a1;
// //       al[1] = a2;
// //       int na = jack_port_get_aliases((jack_port_t*)port, al);
// //       if(na >= 2 && strncmp(al[0], "system:", 7) == 0)  // TODO
// //         fin_name = QString(al[1]);
// //       else 
// //       if(na >= 1)
// //         fin_name = QString(al[0]);
// //       else
// //         fin_name = QString(p_name);
// //     }
// //     else
// //       fin_name = QString(p_name);
// //   }
// //   
// //   if(success)
// //     *success = (bool)p_name;
// //   return fin_name;
// // 
//   
//   int nsz = jack_port_name_size();
//   char s[nsz];
//   return QString(portName(port, s, nsz));
// }

// REMOVE Tim. Persistent routes. Added.
char* JackAudioDevice::portName(void* port, char* str, int str_size)
{
  //if(!port || !checkJackClient(_client))
  //  return 0;
//   int nsz = jack_port_name_size();
//   const char* p_name = jack_port_name((jack_port_t*)port);
//   if(p_name)
//   {
//     if(strncmp(p_name, "system:", 7) == 0)   // TODO: Make this a user editable blacklist of client names!
//     {
//       char a1[nsz]; 
//       char a2[nsz]; 
//       char* al[2]; 
//       al[0] = &a1[0];
//       al[1] = &a2[0];
//       int na = jack_port_get_aliases((jack_port_t*)port, al);
//       if(na >= 2 && strncmp(al[0], "system:", 7) == 0)  // TODO ''
//         return MusELib::strntcpy(str, al[1], str_size);
//       if(na >= 1)
//         return MusELib::strntcpy(str, al[0], str_size);
//       return MusELib::strntcpy(str, p_name, str_size);
//     }
//   }
//   return MusELib::strntcpy(str, p_name, str_size); // strntcpy accepts NULL source
  
  bool A = false, B = false, C = false;
  const char* p_name = jack_port_name((jack_port_t*)port);
  if(p_name && p_name[0] != '\0')
  {
    if(strncmp(p_name, "system:", 7) != 0)              // TODO: Make this a user editable blacklist of client names!
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
    if(strncmp(al[0], "system:", 7) != 0)               //
      return MusELib::strntcpy(str, al[0], str_size);
    B = true;
  }

  if(na >= 2 && al[1] != '\0')
  {
    if(strncmp(al[1], "system:", 7) != 0)               //
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

// //---------------------------------------------------------
// //   scanJackRoutes
// //---------------------------------------------------------
// 
// void JackAudioDevice::scanJackRoutes()
// {
//   if(JACK_DEBUG)
//         printf("scanJackRoutes()\n");
//   //if(!checkJackClient(_client)) return;
//   
//   PendingOperationList operations;
// 
//   //---------------------------------------
//   // audio inputs
//   //---------------------------------------
//   
//   InputList* il = MusEGlobal::song->inputs();
//   for(iAudioInput ii = il->begin(); ii != il->end(); ++ii) 
//   {
//     AudioInput* it = *ii;
//     int channels = it->channels();
//     for(int channel = 0; channel < channels; ++channel) 
//     {
//       jack_port_t* port = (jack_port_t*)(it->jackPort(channel));
//       RouteList* rl = it->inRoutes();
//       for(iRoute irl = rl->begin(); irl != rl->end(); ++irl) 
//       {
//         if(irl->type != Route::JACK_ROUTE)  
//           continue;
//         if(irl->channel != channel)
//           continue;
//         QString name = irl->name();
//         QByteArray ba = name.toLatin1();
//         const char* portName = ba.constData();
//         jack_port_t* j_port = jack_port_by_name(_client, portName);
//         if(port && j_port && !jack_port_connected_to(port, portName))
//           jack_connect(_client, portName, jack_port_name(port));
//         if(j_port != irl->jackPort)
//         {
//           Route route_change(*irl);
//           route_change.jackPort = j_port;
//           operations.add(PendingOperationItem(route_change, &(*irl), PendingOperationItem::ModifyRouteNode));
//         }
//       }
//     }
//   }
//   
//   //---------------------------------------
//   // audio outputs
//   //---------------------------------------
//   
//   OutputList* ol = MusEGlobal::song->outputs();
//   for(iAudioOutput ii = ol->begin(); ii != ol->end(); ++ii) 
//   {
//     AudioOutput* it = *ii;
//     int channels = it->channels();
//     for (int channel = 0; channel < channels; ++channel) 
//     {
//       jack_port_t* port = (jack_port_t*)(it->jackPort(channel));
//       RouteList* rl = it->outRoutes();
//       for(iRoute irl = rl->begin(); irl != rl->end(); ++irl) 
//       {
//         if(irl->type != Route::JACK_ROUTE)  
//           continue;
//         if(irl->channel != channel)
//           continue;
//         QString name = irl->name();
//         QByteArray ba = name.toLatin1();
//         const char* portName = ba.constData();
//         jack_port_t* j_port = jack_port_by_name(_client, portName);
//         if(port && j_port && !jack_port_connected_to(port, portName))
//           jack_connect(_client, jack_port_name(port), portName);
//         if(j_port != irl->jackPort)
//         {
//           Route route_change(*irl);
//           route_change.jackPort = j_port;
//           operations.add(PendingOperationItem(route_change, &(*irl), PendingOperationItem::ModifyRouteNode));
//         }
//       }
//     }
//   }
//               
//   //------------------------
//   // midi devices
//   //------------------------
//   
//   for(iMidiDevice ii = MusEGlobal::midiDevices.begin(); ii != MusEGlobal::midiDevices.end(); ++ii) 
//   {
//     MidiDevice* md = *ii;
//     if(md->deviceType() != MidiDevice::JACK_MIDI)
//       continue;
//     
//     //------------------------
//     // midi inputs
//     //------------------------
//     
//     if(md->rwFlags() & 2) // Readable
//     {
//       jack_port_t* port = (jack_port_t*)md->inClientPort();
//       RouteList* rl = md->inRoutes();
//       for (iRoute irl = rl->begin(); irl != rl->end(); ++irl) 
//       {
//         if(irl->type != Route::JACK_ROUTE)  
//           continue;
//         QString name = irl->name();
//         QByteArray ba = name.toLatin1();
//         const char* portName = ba.constData();
//         jack_port_t* j_port = jack_port_by_name(_client, portName);
//         if(port && j_port && !jack_port_connected_to(port, portName))
//           jack_connect(_client, portName, jack_port_name(port));
//         if(j_port != irl->jackPort)
//         {
//           Route route_change(*irl);
//           route_change.jackPort = j_port;
//           operations.add(PendingOperationItem(route_change, &(*irl), PendingOperationItem::ModifyRouteNode));
//         }
//       }
//     }  
//     
//     //---------------------------------------
//     // midi outputs
//     //---------------------------------------
//     
//     if(md->rwFlags() & 1) // Writable
//     {
//       jack_port_t* port = (jack_port_t*)md->outClientPort();
//       RouteList* rl      = md->outRoutes();
//       for(iRoute irl = rl->begin(); irl != rl->end(); ++irl) 
//       {
//         if(irl->type != Route::JACK_ROUTE)  
//           continue;
//         QString name = irl->name();
//         QByteArray ba = name.toLatin1();
//         const char* portName = ba.constData();
//         
//         jack_port_t* j_port = jack_port_by_name(_client, portName);
//         if(port && j_port && !jack_port_connected_to(port, portName))
//           jack_connect(_client, jack_port_name(port), portName);
//         
//         if(j_port != irl->jackPort)
//         {
//           Route route_change(*irl);
//           route_change.jackPort = j_port;
//           operations.add(PendingOperationItem(route_change, &(*irl), PendingOperationItem::ModifyRouteNode));
//         }
//       }
//     }
//   }
//   
//   if(!operations.empty())
//     MusEGlobal::audio->msgExecutePendingOperations(operations);
// }
// 
// //---------------------------------------------------------
// //   collectPortRenames
// //---------------------------------------------------------
// 
// void JackAudioDevice::collectPortRenames(jack_port_id_t id)
// {
//   if (JACK_DEBUG)
//         printf("JackAudioDevice::updateAllJackRouteNames\n");
//   if(!checkJackClient())
//     return;
//   
//   jack_port_t* port = jack_port_by_id(id);
//   if(!port)
//     return;
//   const char* port_name = jack_port_name(port);
//   if(!port_name)
//     return;
//   QString name(port_name);
//      
//   //---------------------------------------
//   // audio inputs
//   //---------------------------------------
//   
//   InputList* il = MusEGlobal::song->inputs();
//   for(iAudioInput ii = il->begin(); ii != il->end(); ++ii) 
//   {
//     AudioInput* it = *ii;
//     int channels = it->channels();
//     for(int channel = 0; channel < channels; ++channel) 
//     {
//       RouteList* rl = it->inRoutes();
//       for(iRoute irl = rl->begin(); irl != rl->end(); ++irl) 
//       {
//         if(irl->type != Route::JACK_ROUTE)  
//           continue;
//         if(irl->channel != channel)
//           continue;
//         if(irl->jackPort == port && irl->persistentJackPortName != name)
//         {
//           JackCallbackEvent ev;
//           ev.type = PortRename;
//           ev.port = port;
//           ev.name = name;
//           callbackFifo.put(ev);
//         }
//       }
//     }
//   }
//   
//   //---------------------------------------
//   // audio outputs
//   //---------------------------------------
//   
//   OutputList* ol = MusEGlobal::song->outputs();
//   for(iAudioOutput ii = ol->begin(); ii != ol->end(); ++ii) 
//   {
//     AudioOutput* it = *ii;
//     int channels = it->channels();
//     for (int channel = 0; channel < channels; ++channel) 
//     {
//       RouteList* rl = it->outRoutes();
//       for(iRoute irl = rl->begin(); irl != rl->end(); ++irl) 
//       {
//         if(irl->type != Route::JACK_ROUTE)  
//           continue;
//         if(irl->channel != channel)
//           continue;
//         if(irl->jackPort == port && irl->persistentJackPortName != name)
//         {
//           JackCallbackEvent ev;
//           ev.type = PortRename;
//           ev.port = port;
//           ev.name = name;
//           callbackFifo.put(ev);
//         }
//       }
//     }
//   }
//               
//   //------------------------
//   // midi devices
//   //------------------------
//   
//   for(iMidiDevice ii = MusEGlobal::midiDevices.begin(); ii != MusEGlobal::midiDevices.end(); ++ii) 
//   {
//     MidiDevice* md = *ii;
//     if(md->deviceType() != MidiDevice::JACK_MIDI)
//       continue;
//     
//     //------------------------
//     // midi inputs
//     //------------------------
//     
//     if(md->rwFlags() & 2) // Readable
//     {
//       RouteList* rl = md->inRoutes();
//       for (iRoute irl = rl->begin(); irl != rl->end(); ++irl) 
//       {
//         if(irl->type != Route::JACK_ROUTE)  
//           continue;
//         if(irl->jackPort == port && irl->persistentJackPortName != name)
//         {
//           JackCallbackEvent ev;
//           ev.type = PortRename;
//           ev.port = port;
//           ev.name = name;
//           callbackFifo.put(ev);
//         }
//       }
//     }  
//     
//     //---------------------------------------
//     // midi outputs
//     //---------------------------------------
//     
//     if(md->rwFlags() & 1) // Writable
//     {
//       RouteList* rl      = md->outRoutes();
//       for(iRoute irl = rl->begin(); irl != rl->end(); ++irl) 
//       {
//         if(irl->type != Route::JACK_ROUTE)  
//           continue;
//         if(irl->jackPort == port && irl->persistentJackPortName != name)
//         {
//           JackCallbackEvent ev;
//           ev.type = PortRename;
//           ev.port = port;
//           ev.name = name;
//           callbackFifo.put(ev);
//         }
//       }
//     }
//   }
// }
// 
// //---------------------------------------------------------
// //   updateAllJackRouteNames
// //---------------------------------------------------------
// 
// void JackAudioDevice::updateAllJackRouteNames()
// {
//   if (JACK_DEBUG)
//         printf("JackAudioDevice::updateAllJackRouteNames\n");
// 
//   PendingOperationList operations;
// 
//   QString name;
//   jack_port_t* port = 0;
//   while(!callbackFifo.isEmpty())
//   {
//     JackCallbackEvent v = callbackFifo.peek();
//     port = v.port;
//     name = v.name;
//     callbackFifo.remove(); // Done with the ring buffer's item. Remove it.
//     
//   
//     //---------------------------------------
//     // audio inputs
//     //---------------------------------------
//     
//     InputList* il = MusEGlobal::song->inputs();
//     for(iAudioInput ii = il->begin(); ii != il->end(); ++ii) 
//     {
//       AudioInput* it = *ii;
//       int channels = it->channels();
//       for(int channel = 0; channel < channels; ++channel) 
//       {
//         RouteList* rl = it->inRoutes();
//         for(iRoute irl = rl->begin(); irl != rl->end(); ++irl) 
//         {
//           if(irl->type != Route::JACK_ROUTE)  
//             continue;
//           if(irl->channel != channel)
//             continue;
//           if(irl->jackPort == port && irl->persistentJackPortName != name)
//           {
//             Route route_change(*irl);
//             route_change.persistentJackPortName = name;
//             operations.add(PendingOperationItem(route_change, &(*irl), PendingOperationItem::ModifyRouteNode));
//           }
//         }
//       }
//     }
//     
//     //---------------------------------------
//     // audio outputs
//     //---------------------------------------
//     
//     OutputList* ol = MusEGlobal::song->outputs();
//     for(iAudioOutput ii = ol->begin(); ii != ol->end(); ++ii) 
//     {
//       AudioOutput* it = *ii;
//       int channels = it->channels();
//       for (int channel = 0; channel < channels; ++channel) 
//       {
//         RouteList* rl = it->outRoutes();
//         for(iRoute irl = rl->begin(); irl != rl->end(); ++irl) 
//         {
//           if(irl->type != Route::JACK_ROUTE)  
//             continue;
//           if(irl->channel != channel)
//             continue;
//           if(irl->jackPort == port && irl->persistentJackPortName != name)
//           {
//             Route route_change(*irl);
//             route_change.persistentJackPortName = name;
//             operations.add(PendingOperationItem(route_change, &(*irl), PendingOperationItem::ModifyRouteNode));
//           }
//         }
//       }
//     }
//                 
//     //------------------------
//     // midi devices
//     //------------------------
//     
//     for(iMidiDevice ii = MusEGlobal::midiDevices.begin(); ii != MusEGlobal::midiDevices.end(); ++ii) 
//     {
//       MidiDevice* md = *ii;
//       if(md->deviceType() != MidiDevice::JACK_MIDI)
//         continue;
//       
//       //------------------------
//       // midi inputs
//       //------------------------
//       
//       if(md->rwFlags() & 2) // Readable
//       {
//         RouteList* rl = md->inRoutes();
//         for (iRoute irl = rl->begin(); irl != rl->end(); ++irl) 
//         {
//           if(irl->type != Route::JACK_ROUTE)  
//             continue;
//           if(irl->jackPort == port && irl->persistentJackPortName != name)
//           {
//             Route route_change(*irl);
//             route_change.persistentJackPortName = name;
//             operations.add(PendingOperationItem(route_change, &(*irl), PendingOperationItem::ModifyRouteNode));
//           }
//         }
//       }  
//       
//       //---------------------------------------
//       // midi outputs
//       //---------------------------------------
//       
//       if(md->rwFlags() & 1) // Writable
//       {
//         RouteList* rl      = md->outRoutes();
//         for(iRoute irl = rl->begin(); irl != rl->end(); ++irl) 
//         {
//           if(irl->type != Route::JACK_ROUTE)  
//             continue;
//           if(irl->jackPort == port && irl->persistentJackPortName != name)
//           {
//             Route route_change(*irl);
//             route_change.persistentJackPortName = name;
//             operations.add(PendingOperationItem(route_change, &(*irl), PendingOperationItem::ModifyRouteNode));
//           }
//         }
//       }
//     }
//   }
//   
//   if(!operations.empty())
//     MusEGlobal::audio->msgExecutePendingOperations(operations);
// 
// 
// 
// 
// /*
// 
// 
// 
//       
//       PendingOperationList operations;
// 
//       
//       std::list<jack_port_t*> port_list;
// 
// //       std::list<QString> clientList;
// //       //const char* type = midi ? JACK_DEFAULT_MIDI_TYPE : JACK_DEFAULT_AUDIO_TYPE;
// //       const char** ports = jack_get_ports(_client, 0, 
// //                                           JACK_DEFAULT_MIDI_TYPE | JACK_DEFAULT_AUDIO_TYPE, 
// //                                           JackPortIsOutput | JackPortIsInput);
// // 
// 
// 
// 
// 
// 
// 
// 
// 
//       
// //       if(ports)
// //       {
// //         getJackPorts(ports, clientList, midi, true, aliases);   // Get physical ports first.
// //         getJackPorts(ports, clientList, midi, false, aliases);  // Get non-physical ports last.
// //         jack_free(ports);  
// //       }
// 
//       
//       
//       
//       
//       
//   //---------------------------------------
//   // audio inputs
//   //---------------------------------------
// 
//   port_list.clear();    
//   const char** ports = jack_get_ports(_client, 0, JACK_DEFAULT_AUDIO_TYPE, JackPortIsInput);
//   for(const char** p = ports; p && *p; ++p) 
//   {
//     jack_port_t* port = jack_port_by_name(_client, *p);
//     if(port)
//       port_list.push_back(port);
//   }
//   if(ports)
//     jack_free(ports);  
// 
//   InputList* il = MusEGlobal::song->inputs();
//   for(iAudioInput ii = il->begin(); ii != il->end(); ++ii) 
//   {
//     AudioInput* it = *ii;
//     int channels = it->channels();
//     for(int channel = 0; channel < channels; ++channel) 
//     {
//       jack_port_t* port = (jack_port_t*)(it->jackPort(channel));
//       RouteList* rl = it->inRoutes();
//       for(iRoute irl = rl->begin(); irl != rl->end(); ++irl) 
//       {
//         if(irl->type != Route::JACK_ROUTE)  
//           continue;
//         if(irl->channel != channel)
//           continue;
//         
//         if(ports)
//         {
//           for (const char** p = ports; p && *p; ++p) 
//           {
//               jack_port_t* port = jack_port_by_name(_client, *p);
//               int port_flags = jack_port_flags(port);
//               // Ignore our own client ports.
//               if(jack_port_is_mine(_client, port))
//                 continue;         
//               int nsz = jack_port_name_size();
//               char buffer[nsz];
//               bool mthrough = false;
//               
//               strncpy(buffer, *p, nsz);
//               char a2[nsz]; 
//               char* al[2];
//               al[0] = buffer;
//               al[1] = a2;
//               int na = jack_port_get_aliases(port, al);
//               if(na >= 1)
//               {
//                 qname = QString(al[0]);
//                     //printf("Checking port name for: %s\n", (QString("alsa_pcm:") + cname + QString("/")).toLatin1().constData());  
//                 // Ignore our own ALSA client!
//                 if(qname.startsWith(QString("alsa_pcm:") + cname + QString("/")))
//                   continue;
//                 // Put Midi Through after all others.
//                 mthrough = qname.startsWith(QString("alsa_pcm:Midi-Through/"));  
//                 //if((physical && mthrough) || (!physical && !mthrough))
//                 //if(physical && mthrough)
//                 //  continue;
//               }    
//               // Put physical/terminal ports before others.
//               bool is_phys = (port_flags & (JackPortIsTerminal | JackPortIsPhysical)) && !mthrough;
//               if((physical && !is_phys) || (!physical && is_phys))
//                 continue;
//               
// 
//               strncpy(buffer, *p, nsz);
//               if((aliases == 0) || (aliases == 1)) 
//               {
//                 char a2[nsz]; 
//                 char* al[2];
//                 al[0] = buffer;
//                 al[1] = a2;
//                 int na = jack_port_get_aliases(port, al);  
//                 int a = aliases;
//                 if(a >= na)
//                 {
//                   a = na;
//                   if(a > 0)
//                     a--;
//                 }    
//                 qname = QString(al[a]);
//               }
//               else
//                 qname = QString(buffer);
//               
//               //clientList.push_back(QString(buffer));
//               name_list.push_back(qname);
//           }
// 
//         }
//           
//         
//         
//         
//         QString name = irl->name();
//         QByteArray ba = name.toLatin1();
//         const char* portName = ba.constData();
//         jack_port_t* j_port = jack_port_by_name(_client, portName);
//         if(port && j_port && !jack_port_connected_to(port, portName))
//           jack_connect(_client, portName, jack_port_name(port));
//         if(j_port != irl->jackPort)
//         {
//           Route route_change(*irl);
//           route_change.jackPort = j_port;
//           operations.add(PendingOperationItem(route_change, &(*irl), PendingOperationItem::ModifyRouteNode));
//         }
//       }
//     }
//   }
//   
//   //---------------------------------------
//   // audio outputs
//   //---------------------------------------
//   
//   OutputList* ol = MusEGlobal::song->outputs();
//   for(iAudioOutput ii = ol->begin(); ii != ol->end(); ++ii) 
//   {
//     AudioOutput* it = *ii;
//     int channels = it->channels();
//     for (int channel = 0; channel < channels; ++channel) 
//     {
//       jack_port_t* port = (jack_port_t*)(it->jackPort(channel));
//       RouteList* rl = it->outRoutes();
//       for(iRoute irl = rl->begin(); irl != rl->end(); ++irl) 
//       {
//         if(irl->type != Route::JACK_ROUTE)  
//           continue;
//         if(irl->channel != channel)
//           continue;
//         QString name = irl->name();
//         QByteArray ba = name.toLatin1();
//         const char* portName = ba.constData();
//         jack_port_t* j_port = jack_port_by_name(_client, portName);
//         if(port && j_port && !jack_port_connected_to(port, portName))
//           jack_connect(_client, jack_port_name(port), portName);
//         if(j_port != irl->jackPort)
//         {
//           Route route_change(*irl);
//           route_change.jackPort = j_port;
//           operations.add(PendingOperationItem(route_change, &(*irl), PendingOperationItem::ModifyRouteNode));
//         }
//       }
//     }
//   }
//               
//   //------------------------
//   // midi devices
//   //------------------------
//   
//   for(iMidiDevice ii = MusEGlobal::midiDevices.begin(); ii != MusEGlobal::midiDevices.end(); ++ii) 
//   {
//     MidiDevice* md = *ii;
//     if(md->deviceType() != MidiDevice::JACK_MIDI)
//       continue;
//     
//     //------------------------
//     // midi inputs
//     //------------------------
//     
//     if(md->rwFlags() & 2) // Readable
//     {
//       jack_port_t* port = (jack_port_t*)md->inClientPort();
//       RouteList* rl = md->inRoutes();
//       for (iRoute irl = rl->begin(); irl != rl->end(); ++irl) 
//       {
//         if(irl->type != Route::JACK_ROUTE)  
//           continue;
//         QString name = irl->name();
//         QByteArray ba = name.toLatin1();
//         const char* portName = ba.constData();
//         jack_port_t* j_port = jack_port_by_name(_client, portName);
//         if(port && j_port && !jack_port_connected_to(port, portName))
//           jack_connect(_client, portName, jack_port_name(port));
//         if(j_port != irl->jackPort)
//         {
//           Route route_change(*irl);
//           route_change.jackPort = j_port;
//           operations.add(PendingOperationItem(route_change, &(*irl), PendingOperationItem::ModifyRouteNode));
//         }
//       }
//     }  
//     
//     //---------------------------------------
//     // midi outputs
//     //---------------------------------------
//     
//     if(md->rwFlags() & 1) // Writable
//     {
//       jack_port_t* port = (jack_port_t*)md->outClientPort();
//       RouteList* rl      = md->outRoutes();
//       for(iRoute irl = rl->begin(); irl != rl->end(); ++irl) 
//       {
//         if(irl->type != Route::JACK_ROUTE)  
//           continue;
//         QString name = irl->name();
//         QByteArray ba = name.toLatin1();
//         const char* portName = ba.constData();
//         
//         jack_port_t* j_port = jack_port_by_name(_client, portName);
//         if(port && j_port && !jack_port_connected_to(port, portName))
//           jack_connect(_client, jack_port_name(port), portName);
//         
//         if(j_port != irl->jackPort)
//         {
//           Route route_change(*irl);
//           route_change.jackPort = j_port;
//           operations.add(PendingOperationItem(route_change, &(*irl), PendingOperationItem::ModifyRouteNode));
//         }
//       }
//     }
//   }
//   
//   if(!operations.empty())
//     MusEGlobal::audio->msgExecutePendingOperations(operations);
//       */
//       
// }
      
//---------------------------------------------------------
//   scanMidiPorts
//---------------------------------------------------------

// void JackAudioDevice::scanMidiPorts()
// {
//   //if(MusEGlobal::debugMsg)
//   //  printf("JackAudioDevice::scanMidiPorts:\n");
//   
// /*  
//   const char* type = JACK_DEFAULT_MIDI_TYPE;
//   const char** ports = jack_get_ports(_client, 0, type, 0);
//   
//   std::set<std::string> names;
//   for (const char** p = ports; p && *p; ++p) 
//   {
//     jack_port_t* port = jack_port_by_name(_client, *p);
//     if(!port)
//       continue;
//     // Ignore our own client ports.
//     if(jack_port_is_mine(_client, port))
//     {
//       if(MusEGlobal::debugMsg)
//         printf(" ignoring own port: %s\n", *p);
//       continue;         
//     }
//     
//     int nsz = jack_port_name_size();
//     char buffer[nsz];
//     strncpy(buffer, *p, nsz);
//     // Ignore the MusE Jack port.
//     //if(strncmp(buffer, "MusE", 4) == 0)
//     //  continue;
//     
//     if(MusEGlobal::debugMsg)
//       printf(" found port: %s  ", buffer);
//     
//     // If there are aliases for this port, use the first one - much better for identifying. 
//     //char a1[nsz]; 
//     char a2[nsz]; 
//     char* aliases[2];
//     //aliases[0] = a1;
//     aliases[0] = buffer;
//     aliases[1] = a2;
//     // To disable aliases, just rem this line.
//     jack_port_get_aliases(port, aliases);
//     //int na = jack_port_get_aliases(port, aliases);
//     //char* namep = (na >= 1) ? aliases[0] : buffer;
//     //char* namep = aliases[0];
//     //names.insert(std::string(*p));
//     if(MusEGlobal::debugMsg)
//       printf("alias: %s\n", aliases[0]);
//     
//     names.insert(std::string(aliases[0]));
//   }
//   if(ports)
//     free(ports);      
//   
//   std::list<MidiDevice*> to_del;
//   for(iMidiDevice imd = MusEGlobal::midiDevices.begin(); imd != MusEGlobal::midiDevices.end(); ++imd)
//   {
//     // Only Jack midi devices.
//     if(dynamic_cast<MidiJackDevice*>(*imd) == 0)
//       continue;
//     if(names.find(std::string((*imd)->name().toLatin1())) == names.end())
//       to_del.push_back(*imd);
//   }
//   
//   for(std::list<MidiDevice*>::iterator imd = to_del.begin(); imd != to_del.end(); ++imd)
//   {
//     if(MusEGlobal::debugMsg)
//       printf(" removing port device:%s\n", (*imd)->name().toLatin1());
//     MusEGlobal::midiDevices.remove(*imd);
//     // This will close (and unregister) the client port.
//     delete (*imd);
//   }
//   
//   //for (const char** p = ports; p && *p; ++p) 
//   for(std::set<std::string>::iterator is = names.begin(); is != names.end(); ++is)
//   {
//     //jack_port_t* port = jack_port_by_name(_client, *p);
//     jack_port_t* port = jack_port_by_name(_client, is->c_str());
//     if(!port)
//       continue;
// */    
//     
//     /*
//     int nsz = jack_port_name_size();
//     char buffer[nsz];
//     //strncpy(buffer, *p, nsz);
//     strncpy(buffer, is->c_str(), nsz);
//     // Ignore the MusE Jack port.
//     //if(strncmp(buffer, "MusE", 4) == 0)
//     //  continue;
//     
//     // If there are aliases for this port, use the first one - much better for identifying. 
//     //char a1[nsz]; 
//     char a2[nsz]; 
//     char* aliases[2];
//     //aliases[0] = a1;
//     aliases[0] = buffer;
//     aliases[1] = a2;
//     // To disable aliases, just rem this line.
//     jack_port_get_aliases(port, aliases);
//     //int na = jack_port_get_aliases(port, aliases);
//     //char* namep = (na >= 1) ? aliases[0] : buffer;
//     char* namep = aliases[0];
//     QString qname(namep);
//     */
//     
// /*
//     QString qname(is->c_str());
//     
//     // Port already exists?
//     if(MusEGlobal::midiDevices.find(qname))
//       continue;
//     
//     int flags = 0;
//     int pf = jack_port_flags(port);
//     // If Jack port can send data to us...
//     if(pf & JackPortIsOutput)
//       // Mark as input capable.
//       flags |= 2;
//     // If Jack port can receive data from us...
//     if(pf & JackPortIsInput)
//       // Mark as output capable.
//       flags |= 1;
//     
//     //JackPort jp(0, QString(buffer), flags);
//     //portList.append(jp);
//     
//     if(MusEGlobal::debugMsg)
//       printf(" adding port device:%s\n", qname.toLatin1());
//     
//     MidiJackDevice* dev = new MidiJackDevice(0, qname);
//     dev->setrwFlags(flags);
//     MusEGlobal::midiDevices.add(dev);
//   }
// */
// }


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
      
      // REMOVE Tim. Persistent routes. Added.
      muse_atomic_destroy(&atomicGraphChangedPending);
      }
    
      
// //---------------------------------------------------------
// //   JackCallbackEventList
// //---------------------------------------------------------
// 
// bool JackCallbackEventList::add(const JackCallbackEvent& ev)
// {
//   switch(ev.type)
//   {
//     case GraphChanged: 
//       break;
// 
//     case PortRegister: 
//       break;
// 
//     case PortUnregister: 
//       for(riJackCallbackEvent ijce = rbegin(); ijce != rend(); ++ijce)
//       {
//         // Stop looking on any previous GraphChanged.
//         if(ijce->type == GraphChanged)
//           break;
//         if(ijce->type == PortDisconnect && (ijce->port_id_A == ev.port_id_A || ijce->port_id_B == ev.port_id_A))
//         {
//           // Copy the names over to the PortUnregister event, to make it easier when iterating the list later.
//           ev.name_A = ijce->name_A;
//           ev.name_B = ijce->name_B;
//           // Erase the PortDisconnect event.
//           erase((++ijce).base());
//           break;
//         }
//       }
//       break;
// 
//     case PortConnect: 
//       break;
// 
//     case PortDisconnect: 
//       for(riJackCallbackEvent ijce = rbegin(); ijce != rend(); ++ijce)
//         if(ijce->type == PortConnect && ijce->port_id_A == ev.port_id_A && ijce->port_id_B == ev.port_id_B)
//         {
//           erase((++ijce).base());
//           return false;
//         }
//       break;
//   }
//   push_back(ev);
//   return true;
// }
//      
      
} // namespace MusECore


