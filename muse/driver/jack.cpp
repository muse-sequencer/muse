//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: jack.cpp,v 1.30.2.17 2009/12/20 05:00:35 terminator356 Exp $
//  (C) Copyright 2002 Werner Schweer (ws@seh.de)
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

//extern int jackmidi_pi[2];
//extern int jackmidi_po[2];

//jack_port_t *midi_port_in[JACK_MIDI_CHANNELS];
//jack_port_t *midi_port_out[JACK_MIDI_CHANNELS];

//muse_jack_midi_buffer jack_midi_out_data[JACK_MIDI_CHANNELS];
//muse_jack_midi_buffer jack_midi_in_data[JACK_MIDI_CHANNELS];

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

static void jack_thread_init (void* )  // data
      {
      MusEGlobal::doSetuid();
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
      MusEGlobal::undoSetuid();
      }

/*
//---------------------------------------------------------
//   processAudio + Midi
//    JACK callback
//---------------------------------------------------------
void
print_triplet(unsigned char *data)
{
  int a,b,c;
  a = b = c = 0;
  memcpy(&a, data, 1);
  memcpy(&b, data+1, 1);
  memcpy(&c, data+2, 1);
  fprintf(stderr, "%x,%x,%x", a, b, c);
}
*/

/*
void handle_jack_midi_in_events(jack_nframes_t frames)
{
  char buf = 0;
  int i,j;
  jack_midi_event_t midi_event;
  unsigned char t,n,v;

  for(j = 0; j < JACK_MIDI_CHANNELS; j++){
    void *midi_buffer_in = jack_port_get_buffer(midi_port_in[j], frames);
    int event_count = jack_midi_get_event_count(midi_buffer_in);

    for(i = 0; i < event_count; i++){
      jack_midi_event_get(&midi_event, midi_buffer_in, i);
      t = midi_event.buffer[0];
      n = midi_event.buffer[1];
      v = midi_event.buffer[2];
      if(((*(midi_event.buffer) & 0xf0)) == 0x90){
        fprintf(stderr, "jack-midi-in-event: ON_ time=%d %u ", midi_event.time,
                midi_event.size);
        print_triplet(midi_event.buffer);
        fprintf(stderr, "\n");
      }else if(((*(midi_event.buffer)) & 0xf0) == 0x80){
        fprintf(stderr, "jack-midi-in-event: OFF time=%d %u ", midi_event.time,
                midi_event.size);
        print_triplet(midi_event.buffer);
        fprintf(stderr, "\n");
      }else{
        fprintf(stderr, "jack-midi-in-event: ??? time=%d %u ", midi_event.time,
                midi_event.size);
        print_triplet(midi_event.buffer);
        fprintf(stderr, "\n");
      }
      jack_midi_in_data[j].buffer[0] = t;
      jack_midi_in_data[j].buffer[1] = n;
      jack_midi_in_data[j].buffer[2] = v;
      jack_midi_in_data[j].buffer[3] = 1;
      fprintf(stderr, "handle_jack_midi_in_events() w\n");
      write(jackmidi_pi[1], &buf, 1);
      fprintf(stderr, "handle_jack_midi_in_events() wd\n");
    }
  }
}

void handle_jack_midi_out_events(jack_nframes_t frames)
{
  unsigned char *data;
  void *port_buf;
  int i,j,n,x;

  //for(i = 0; i < JACK_MIDI_CHANNELS; i++){
  for(i = 0; i < JACK_MIDI_CHANNELS; ++i){
    // jack-midi-clear any old events 
    while(jack_midi_out_data[i].buffer[jack_midi_out_data[i].take*4+3] == 2){
      port_buf = jack_port_get_buffer(midi_port_out[i], frames);
      jack_midi_clear_buffer(port_buf);
      jack_midi_out_data[i].buffer[jack_midi_out_data[i].take*4+3] = 0;
      // point the take to the next slot 
      jack_midi_out_data[i].take++;
      if(jack_midi_out_data[i].take >= JACK_MIDI_BUFFER_SIZE){
        jack_midi_out_data[i].take = 0;
      }
    }
    // check if any incoming midi-events from muse 
    if(jack_midi_out_data[i].give != jack_midi_out_data[i].take){

      if(jack_midi_out_data[i].give > jack_midi_out_data[i].take){
        n = jack_midi_out_data[i].give - jack_midi_out_data[i].take;
      }else{
        n = jack_midi_out_data[i].give +
            (JACK_MIDI_BUFFER_SIZE - jack_midi_out_data[i].take);
      }
      port_buf = jack_port_get_buffer(midi_port_out[i], frames);
      jack_midi_clear_buffer(port_buf);
      // FIX: midi events has different sizes, compare note-on to
      //        program-change. We should first walk over the events
      //        counting the size. 
      //data = jack_midi_event_reserve(port_buf, 0, n*3);
      //x = jack_midi_out_data[i].take;
      //for(j = 0; j < n; j++){
      //  data[j*3+0] = jack_midi_out_data[i].buffer[x*4+0];
      //  data[j*3+1] = jack_midi_out_data[i].buffer[x*4+1];
      //  data[j*3+2] = jack_midi_out_data[i].buffer[x*4+2];
        // after having copied the buffer over to the jack-buffer, 
        // mark the muses midi-out buffer as 'need-cleaning' 
      //  jack_midi_out_data[i].buffer[x*4+3] = 2;
      //  x++;
      //  if(x >= JACK_MIDI_BUFFER_SIZE){
      //    x = 0;
      //  }
      //}
      
      x = jack_midi_out_data[i].take;
      for(j = 0; j < n; ++j)
      {
        data = jack_midi_event_reserve(port_buf, 0, 3);
        if(data == 0) 
        {
          fprintf(stderr, "handle_jack_midi_out_events: buffer overflow, event lost\n");
          // Can do no more processing. Just return.
          return;
        }
        data[0] = jack_midi_out_data[i].buffer[x*4+0];
        data[1] = jack_midi_out_data[i].buffer[x*4+1];
        data[2] = jack_midi_out_data[i].buffer[x*4+2];
        // after having copied the buffer over to the jack-buffer, 
        //  mark the muses midi-out buffer as 'need-cleaning' 
        jack_midi_out_data[i].buffer[x*4+3] = 2;
        x++;
        if(x >= JACK_MIDI_BUFFER_SIZE){
          x = 0;
        }
      }
      
    }
  }
}
*/

//static int processAudio(jack_nframes_t frames, void*)
int JackAudioDevice::processAudio(jack_nframes_t frames, void*)
{
  jackAudio->_frameCounter += frames;
  
///  handle_jack_midi_in_events(frames);
///  handle_jack_midi_out_events(frames);
  
//      if (JACK_DEBUG)
//            printf("processAudio - >>>>\n");
      MusEGlobal::segmentSize = frames;
      if (MusEGlobal::audio->isRunning())
            MusEGlobal::audio->process((unsigned long)frames);
      else {
            if (MusEGlobal::debugMsg)
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
      //printf("processSync valid:%d frame:%d\n", pos->valid, frame);
      
            // p3.3.23
            //printf("Jack processSync() before MusEGlobal::audio->sync frame:%d\n", frame);
      //return MusEGlobal::audio->sync(audioState, frame);
      int rv = MusEGlobal::audio->sync(audioState, frame);
            //printf("Jack processSync() after MusEGlobal::audio->sync frame:%d\n", frame);
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
      //printf("Jack timebase_callback pos->frame:%u MusEGlobal::audio->tickPos:%d MusEGlobal::song->cpos:%d\n", pos->frame, MusEGlobal::audio->tickPos(), MusEGlobal::song->cpos());
      
      // p3.3.27
      //Pos p(pos->frame, false);
      Pos p(MusEGlobal::extSyncFlag.value() ? MusEGlobal::audio->tickPos() : pos->frame, MusEGlobal::extSyncFlag.value() ? true : false);
      // Can't use song pos - it is only updated every (slow) GUI heartbeat !
      //Pos p(MusEGlobal::extSyncFlag.value() ? MusEGlobal::song->cpos() : pos->frame, MusEGlobal::extSyncFlag.value() ? true : false);
      
      pos->valid = JackPositionBBT;
      p.mbt(&pos->bar, &pos->beat, &pos->tick);
      pos->bar++;
      pos->beat++;
      pos->bar_start_tick = Pos(pos->bar, 0, 0).tick();
      
      //
      //  dummy:
      //
      
      // p3.3.26
      //pos->beats_per_bar = 4;
      //pos->beat_type = 4;
      //pos->ticks_per_beat = 384;
      //
      /* // From example client transport.c :
      float time_beats_per_bar = 4.0;
      float time_beat_type = 0.25;            // Huh? Inverted? From docs: "Time signature 'denominator'" 
      double time_ticks_per_beat = 1920.0;    // Huh? Ticks per beat should be 24 etc. not 384 or 1920 etc. Otherwise it would be called 'frames_per_beat'.
      double time_beats_per_minute = 120.0;
      */
      //
      int z, n;
      AL::sigmap.timesig(p.tick(), z, n);
      pos->beats_per_bar = z;
      pos->beat_type = n;
      //pos->ticks_per_beat = config.division;
      pos->ticks_per_beat = 24;
      
      int tempo = MusEGlobal::tempomap.tempo(p.tick());
      pos->beats_per_minute = (60000000.0 / tempo) * MusEGlobal::tempomap.globalTempo()/100.0;
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
            
            /*
            // p3.3.35
            for(int i = 0; i < JACK_MIDI_CHANNELS; i++)
            {
              if(midi_port_in[i])
                jack_port_unregister(_client, midi_port_in[i]);
              if(midi_port_out[i])
                jack_port_unregister(_client, midi_port_out[i]);
            }
            */
            
            if (jack_client_close(_client)) {
                  //error->logError("jack_client_close() failed: %s\n", strerror(errno));
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

/*
//---------------------------------------------------------
//   getJackName()
//---------------------------------------------------------

char* JackAudioDevice::getJackName()
      {
      return jackRegisteredName;
      }
*/

/*
//---------------------------------------------------------
//   clientName()
//---------------------------------------------------------

const char* JackAudioDevice::clientName()
{
  //if(_client)
  //  return jack_get_client_name(_client);
  //else
  //  return "MusE";  
  return jackRegisteredName;
}
*/

//---------------------------------------------------------
//   initJackAudio
//    return true if JACK not found
//---------------------------------------------------------

bool initJackAudio()
      {
      /*
      // p3.3.35
      for(int i = 0; i < JACK_MIDI_CHANNELS; i++)
      {
        midi_port_in[i] = 0;
        midi_port_out[i] = 0;
      }
      */
      
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
	    MusEGlobal::undoSetuid();   // p3.3.51
            return true;
            }

      if (MusEGlobal::debugMsg)
            fprintf(stderr, "initJackAudio(): client %s opened.\n", jack_get_client_name(client));
      if (client) {
            jack_set_error_function(jackError);
            //jackAudio = new JackAudioDevice(client, jackIdString);
            jackAudio = new JackAudioDevice(client, jack_get_client_name(client));
            if (MusEGlobal::debugMsg)
                  fprintf(stderr, "initJackAudio(): registering client...\n");
            jackAudio->registerClient();
            MusEGlobal::sampleRate  = jack_get_sample_rate(client);
            MusEGlobal::segmentSize = jack_get_buffer_size(client);
            jack_set_thread_init_callback(client, (JackThreadInitCallback) jack_thread_init, 0);
            //jack_set_timebase_callback(client, 0, (JackTimebaseCallback) timebase_callback, 0);
            }
      MusEGlobal::undoSetuid();
      
      /*
      // setup midi input/output 
      //memset(jack_midi_out_data, 0, JACK_MIDI_CHANNELS * sizeof(muse_jack_midi_buffer));
      //memset(jack_midi_in_data, 0, JACK_MIDI_CHANNELS * sizeof(muse_jack_midi_buffer));
      if(client){
        for(i = 0; i < JACK_MIDI_CHANNELS; i++)
        {
          char buf[80];
          snprintf(buf, 80, "muse-jack-midi-in-%d", i+1);
          midi_port_in[i] = jack_port_register(client, buf,
                                              JACK_DEFAULT_MIDI_TYPE,
                                              JackPortIsInput, 0);
          if(midi_port_in[i] == NULL){
            fprintf(stderr, "failed to register jack-midi-in\n");
            exit(-1);
          }
          snprintf(buf, 80, "muse-jack-midi-out-%d", i+1);
          midi_port_out[i] = jack_port_register(client, buf,
                                                JACK_DEFAULT_MIDI_TYPE,
                                                JackPortIsOutput, 0);
          if(midi_port_out == NULL)
          {
            fprintf(stderr, "failed to register jack-midi-out\n");
            exit(-1);
          }
        }
      }
      else
      {
        fprintf(stderr, "WARNING NO muse-jack midi connection\n");
      }
      */    
          
      if (client) {
            MusEGlobal::audioDevice = jackAudio;
            jackAudio->scanMidiPorts();
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
      void* port = md->outClientPort(); // p3.3.55
      if(port)                          // 
      {
        RouteList* rl = md->outRoutes();
        for (ciRoute r = rl->begin(); r != rl->end(); ++r) 
          connect(port, r->jackPort);
      }    
    }
    
    // else  // p3.3.55 Removed
    
    if(md->rwFlags() & 2)
    {  
      void* port = md->inClientPort();  // p3.3.55
      if(port)                          //
      {
        RouteList* rl = md->inRoutes();
        for (ciRoute r = rl->begin(); r != rl->end(); ++r) 
          connect(r->jackPort, port);
      }    
    }    
  }
  
  
  /*
  const char* type = JACK_DEFAULT_MIDI_TYPE;
  const char** ports = jack_get_ports(_client, 0, type, 0);
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
    char* namep = aliases[0];
  
    if(MusEGlobal::debugMsg)
      printf("alias: %s\n", aliases[0]);
    
    //int flags = 0;
    int pf = jack_port_flags(port);
    // If Jack port can send data to us...
    //if(pf & JackPortIsOutput)
      // Mark as input capable.
    //  flags |= 2;
    // If Jack port can receive data from us...
    //if(pf & JackPortIsInput)
      // Mark as output capable.
    //  flags |= 1;
    
    //JackPort jp(0, QString(buffer), flags);
    //portList.append(jp);
    
    QString name(namep);
    
    if(JACK_DEBUG)
      printf("JackAudioDevice::graphChanged %s\n", name.toLatin1());
      
    for(iMidiDevice imd = MusEGlobal::midiDevices.begin(); imd != MusEGlobal::midiDevices.end(); ++imd)
    {
      // Is it a Jack midi device? 
      MidiJackDevice* mjd = dynamic_cast<MidiJackDevice*>(*imd);
      if(!mjd)
        continue;
        
      //if(dev->name() != name)
      //  continue;
      
      // Is this port the one created for the Jack midi device?
      if(!mjd->clientJackPort() || (mjd->clientJackPort() != port))
        continue;
      
      jack_port_t* devport = jack_port_by_name(_client, mjd->name().toLatin1());
      if(!devport)
        continue;
      
      int ofl = mjd->openFlags();
    
      if(JACK_DEBUG)
        printf("JackAudioDevice::graphChanged found MidiJackDevice:%s\n", mjd->name().toLatin1());
      
      // Note docs say it can't be both input and output. src, dest
      // If Jack port can receive data from us and we actually want to...
      if((pf & JackPortIsOutput) && (ofl & 1))
      {
        if(JACK_DEBUG)
          printf("JackAudioDevice::graphChanged connecting MusE output\n");
        MusEGlobal::audioDevice->connect(port, devport);
      }
      else 
      // If Jack port can send data to us and we actually want it...
      if((pf & JackPortIsInput) && (ofl & 2))
      {
        if(JACK_DEBUG)
          printf("JackAudioDevice::graphChanged connecting MusE input\n");
        MusEGlobal::audioDevice->connect(devport, port);
      }
      
      break;  
    }
  }
  
  if(ports)
    free(ports);      
    
  */  
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

                        // p3.3.37
                        //delete ports;
                        //free(ports);
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

                        // p3.3.37
                        //delete ports;
                        //free(ports);
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
                    // p3.3.55
                    jack_port_t* port = (jack_port_t*)md->outClientPort();
                    if(port != 0)
                    {
                      //printf("graphChanged() valid out client port\n"); // p3.3.55
                      
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
                                  //if (irl->channel != channel)
                                  //      continue;
                                  QString name = irl->name();
                                  //name += QString(JACK_MIDI_OUT_PORT_SUFFIX);    // p3.3.55
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
    
                            // p3.3.55
                            // Done with ports. Free them.
                            //free(ports);
                            jack_free(ports);  // p4.0.29
                      }
                    }  
                  }  
                  
                  
                  //------------------------
                  // Inputs
                  //------------------------
                  
                  if(md->rwFlags() & 2) // Readable
                  {
                    // p3.3.55
                    jack_port_t* port = (jack_port_t*)md->inClientPort();
                    if(port != 0)
                    {
                      //printf("graphChanged() valid in client port\n"); // p3.3.55
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
                            // p3.3.55
                            // Done with ports. Free them.
                            //free(ports);
                            jack_free(ports);  // p4.0.29
                      }
                    }  
                  }  
                  
                  // p3.3.55 Removed.
                  //if(ports) 
                    // Done with ports. Free them.
                    //delete ports;
                  //  free(ports);
                  //ports = NULL;
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
      // NOTE: Update: It was a bug in QJackCtl. Fixed now.
      //jack_set_sync_timeout(_client, 5000000); // Change default 2 to 5 second sync timeout because prefetch may be very slow esp. with resampling !
      
      jack_on_shutdown(_client, processShutdown, 0);
      jack_set_buffer_size_callback(_client, bufsize_callback, 0);
      jack_set_sample_rate_callback(_client, srate_callback, 0);
      jack_set_port_registration_callback(_client, registration_callback, 0);
      // p3.3.37
      jack_set_client_registration_callback(_client, client_registration_callback, 0);
      jack_set_port_connect_callback(_client, port_connect_callback, 0);
      
      jack_set_graph_order_callback(_client, graph_callback, 0);
//      jack_set_xrun_callback(client, xrun_callback, 0);
      jack_set_freewheel_callback (_client, freewheel_callback, 0);
      }

//---------------------------------------------------------
//   registerInPort
//---------------------------------------------------------

//void* JackAudioDevice::registerInPort(const char* name)
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

//void* JackAudioDevice::registerOutPort(const char* name)
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
      if(!src || !dst)  // p3.3.55
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

//void JackAudioDevice::start()
void JackAudioDevice::start(int /*priority*/)
      {
      if (JACK_DEBUG)
            printf("JackAudioDevice::start()\n");
      if(!checkJackClient(_client)) return;

      MusEGlobal::doSetuid();

      if (jack_activate(_client)) {
            MusEGlobal::undoSetuid();   // p3.3.51
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
                        if (r->channel == ch) {
                              connect(port, r->jackPort);
                              }
                        }
                  }
            }
      
      // p3.3.37
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
//   getCurFrame
//---------------------------------------------------------

unsigned int JackAudioDevice::getCurFrame()
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
//   outputPorts
//---------------------------------------------------------

std::list<QString> JackAudioDevice::outputPorts(bool midi, int aliases)
      {
      if (JACK_DEBUG)
            printf("JackAudioDevice::outputPorts()\n");
      std::list<QString> clientList;
      if(!checkJackClient(_client)) return clientList;
      QString qname;
      const char* type = midi ? JACK_DEFAULT_MIDI_TYPE : JACK_DEFAULT_AUDIO_TYPE;
      const char** ports = jack_get_ports(_client, 0, type, JackPortIsOutput);
      for (const char** p = ports; p && *p; ++p) {
            jack_port_t* port = jack_port_by_name(_client, *p);
            //int flags = jack_port_flags(port);
            //if (!(flags & JackPortIsOutput))
            //      continue;
            //char buffer[128];
            
            int nsz = jack_port_name_size();
            char buffer[nsz];
            
            strncpy(buffer, *p, nsz);
            //if (strncmp(buffer, "MusE", 4) == 0)
            //{
            //  if(MusEGlobal::debugMsg)
            //    printf("JackAudioDevice::outputPorts ignoring own MusE port: %s\n", *p);
            //  continue;         
            //}
            
            // Ignore our own client ports.
            if(jack_port_is_mine(_client, port))
            {
              if(MusEGlobal::debugMsg)
                printf("JackAudioDevice::outputPorts ignoring own port: %s\n", *p);
              continue;         
            }
            
            // p3.3.38
            if((aliases == 0) || (aliases == 1)) 
            {
              //char a1[nsz]; 
              char a2[nsz]; 
              char* al[2];
              //aliases[0] = a1;
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
            clientList.push_back(qname);
            }
            
      // p3.3.37
      if(ports)
        //free(ports);      
        jack_free(ports);  // p4.0.29
      
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
      QString qname;
      const char* type = midi ? JACK_DEFAULT_MIDI_TYPE : JACK_DEFAULT_AUDIO_TYPE;
      const char** ports = jack_get_ports(_client, 0, type, JackPortIsInput);
      for (const char** p = ports; p && *p; ++p) {
            jack_port_t* port = jack_port_by_name(_client, *p);
            //int flags = jack_port_flags(port);
            //if (!(flags & JackPortIsInput))
            //      continue;
            //char buffer[128];
            
            int nsz = jack_port_name_size();
            char buffer[nsz];
            
            strncpy(buffer, *p, nsz);
            //if (strncmp(buffer, "MusE", 4) == 0)
            //{
            //  if(MusEGlobal::debugMsg)
            //    printf("JackAudioDevice::inputPorts ignoring own MusE port: %s\n", *p);
            //  continue;         
            //}
            
            // Ignore our own client ports.
            if(jack_port_is_mine(_client, port))
            {
              if(MusEGlobal::debugMsg)
                printf("JackAudioDevice::inputPorts ignoring own port: %s\n", *p);
              continue;         
            }
            
            // p3.3.38
            if((aliases == 0) || (aliases == 1)) 
            {
              //char a1[nsz]; 
              char a2[nsz]; 
              char* al[2];
              //aliases[0] = a1;
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
            clientList.push_back(qname);
            }
            
      // p3.3.37
      if(ports)
        //free(ports);      
        jack_free(ports);  // p4.0.29
      
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
//   dummySync
//---------------------------------------------------------

bool JackAudioDevice::dummySync(int state)
{
  // Roughly segment time length.
  //timespec ts = { 0, (1000000000 * MusEGlobal::segmentSize) / MusEGlobal::sampleRate };     // In nanoseconds.
  unsigned int sl = (1000000 * MusEGlobal::segmentSize) / MusEGlobal::sampleRate;            // In microseconds.
  
  double ct = curTime();
  // Wait for a default maximum of 5 seconds. 
  // Similar to how Jack is supposed to wait a default of 2 seconds for slow clients.
  // TODO: Make this timeout a 'settings' option so it can be applied both to Jack and here.
  while((curTime() - ct) < 5.0)  
  {
    // Is MusE audio ready to roll?
    if(MusEGlobal::audio->sync(state, dummyPos))
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
      if(!MusEGlobal::useJackTransport.value())
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
      
      if(!MusEGlobal::useJackTransport.value())
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
      if(!MusEGlobal::useJackTransport.value())
      {
        // If we're not using Jack's transport, just pass the current state and new frame along
        //  as if processSync was called. 
        //dummyPos = frame;
        int tempState = dummyState;
        //dummyState = Audio::START_PLAY;
        
        // Is MusE audio ready yet?
        //MusEGlobal::audio->sync(dummyState, dummyPos);
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
        // NOTE: Update: It was a bug with QJackCtl. Fixed now.
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
      if(!MusEGlobal::useJackTransport.value())
      {
        // If we're not using Jack's transport, just pass the current state and new frame along
        //  as if processSync was called. 
        //dummyPos = p.frame();
        int tempState = dummyState;
        //dummyState = Audio::START_PLAY;
        
        // Is MusE audio ready yet?
        //MusEGlobal::audio->sync(dummyState, dummyPos);
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
      int tempo = MusEGlobal::tempomap.tempo(p.tick());
      jp.beats_per_minute = (60000000.0 / tempo) * MusEGlobal::tempomap.globalTempo()/100.0;
      
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
  if(MusEGlobal::debugMsg)
    printf("JackAudioDevice::scanMidiPorts:\n");
  
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
      if (MusECore::jackAudio)
            delete MusECore::jackAudio;
            
      if (JACK_DEBUG)
            printf("exitJackAudio() after delete jackAudio\n");
      
      // Added by Tim. p3.3.14
      MusEGlobal::audioDevice = NULL;      
      
      }
} // namespace MusECore


