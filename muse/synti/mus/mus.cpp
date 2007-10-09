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

#include "all.h"
#include <stdio.h>
#include <getopt.h>
#include <dlfcn.h>

#include "config.h"
#include "libsynti/mess.h"
#include "driver/jackaudio.h"
#include <jack/midiport.h>

static const char* versionString = "1.0";
static bool debug = false;

static int sampleRate;
static int segmentSize;

static Mess* mess;
static const int MAX_OUTPORTS = 8;
static jack_port_t* inPort;
static jack_port_t* outPorts[MAX_OUTPORTS];
static float* outBuffer[MAX_OUTPORTS];

//---------------------------------------------------------
//   processAudio
//    JACK callback
//---------------------------------------------------------

static int processAudio(jack_nframes_t nFrames, void*)
      {
      int nch = mess->channels();
      for (int i = 0; i < nch; ++i) {
            outBuffer[i] = (float*)jack_port_get_buffer(outPorts[i], nFrames);
#ifdef JACK107
            jack_midi_clear_buffer(outBuffer[i]);
#endif
#ifdef JACK103
            jack_midi_clear_buffer(outBuffer[i], nFrames);
#endif
            // memset(outBuffer[i], 0, sizeof(float) * nFrames);
            }
      while(mess->eventsPending())
            mess->processEvent(mess->receiveEvent());

      void* midi = jack_port_get_buffer(inPort, nFrames);
#ifdef JACK107
      int n = jack_midi_get_event_count(midi);
#endif
#ifdef JACK103
      int n = jack_midi_get_event_count(midi, nFrames);
#endif
      unsigned offset = 0;

      for (int i = 0; i < n; ++i) {
            jack_midi_event_t event;

#ifdef JACK107
            jack_midi_event_get(&event, midi, i);
#endif
#ifdef JACK103
            jack_midi_event_get(&event, midi, i, nFrames);
#endif
            mess->process(outBuffer, offset, event.time - offset);
            offset = event.time;
            MidiEvent e;
            e.setType(event.buffer[0] & 0xf0);
            e.setChannel(event.buffer[0] & 0xf);
            e.setA(event.buffer[1]);
            e.setB(event.buffer[2]);
            mess->processEvent(e);
            }
      if (offset < nFrames)
            mess->process(outBuffer, offset, nFrames - offset);
      return 0;
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
//   printVersion
//---------------------------------------------------------

static void printVersion(const char* programName)
      {
      printf("%s: Version %s\n", programName, versionString);
      }

//---------------------------------------------------------
//   usage
//---------------------------------------------------------

static void usage(const char* programName)
      {
      fprintf(stderr, "%s: usage <options> MusE-synthesizer-plugin-name\n"
             "    options:   -v   print version\n"
             "               -d   debug mode on\n",
            programName
            );
      }

//---------------------------------------------------------
//   main
//---------------------------------------------------------

int main(int argc, char* argv[])
      {
      new QApplication(argc, argv);
      int c;
      while ((c = getopt(argc, argv, "vd")) != EOF) {
            switch (c) {
                  case 'v':
                        printVersion(argv[0]);
                        return 0;
                  case 'd':
                        debug = true;
                        break;
                  default:
                        usage(argv[0]);
                        return -1;
                  }
            }
      argc -= optind;
      if (argc != 1) {
            usage(argv[0]);
            return -1;
            }
      //
      //    load synthesizer plugin
      //
      QDir pluginDir(INSTPREFIX "/lib/" INSTALL_NAME "/synthi");
      if (!pluginDir.exists()) {
            fprintf(stderr, "plugin directory <%s> not found\n",
               pluginDir.path().toLocal8Bit().data());
            return -2;
            }
      QString pluginName(argv[1]);
      if (!pluginName.endsWith(".so", Qt::CaseInsensitive))
            pluginName += ".so";
      if (!pluginDir.exists(pluginName)) {
            fprintf(stderr, "plugin <%s> in directory <%s> not found\n",
               pluginName.toLocal8Bit().data(),
               pluginDir.path().toLocal8Bit().data());
            return -3;
            }
      QString path = pluginDir.filePath(pluginName);
      void* handle = dlopen(path.toLocal8Bit().data(), RTLD_LAZY);
      if (handle == 0) {
            fprintf(stderr, "%s: dlopen(%s) failed: %s\n",
               argv[0], path.toLocal8Bit().data(), dlerror());
            return -4;
            }
      typedef const MESS* (*MESS_Function)();
      MESS_Function msynth = (MESS_Function)dlsym(handle, "mess_descriptor");
      if (!msynth) {
            fprintf(stderr,
               "%s: Unable to find msynth_descriptor() function in plugin\n"
               "Are you sure this is a MESS plugin file?\n%s",
               argv[0], dlerror());
            return -5;
            }
      const MESS* descr = msynth();
      if (descr == 0) {
            fprintf(stderr, "%s: instantiate: no MESS descr found\n",
               argv[0]);
            return 6;
            }
      //
      //    initialize JACK
      //
      if (debug) {
            fprintf(stderr, "init JACK audio\n");
            jack_set_error_function(jackError);
            }
      else
            jack_set_error_function(noJackError);

      jack_client_t* client;
      int i = 0;
      QString instanceName;
      QString s(pluginName.left(pluginName.size()-3));
      s += "-%1";
      static const int MAX_INSTANCES = 100;
      for (i = 0; i < MAX_INSTANCES; ++i) {
            instanceName = s.arg(i);
            const char* jackIdString = strdup(instanceName.toLocal8Bit().data());
            client = jack_client_new(jackIdString);
            if (client)
                  break;
            }
      if (i == MAX_INSTANCES) {
            fprintf(stderr, "%s: too many instances of the synth! (>%d)\n",
               argv[0], MAX_INSTANCES);
            return -7;
            }
      jack_set_error_function(jackError);
      if (debug)
            fprintf(stderr, "init Jack Audio: register device\n");
      // jackAudio = new JackAudio(client, jackIdString);
      if (debug)
            fprintf(stderr, "init Jack Audio: register client\n");

      jack_set_process_callback(client, processAudio, 0);
//      jack_set_sync_callback(_client, processSync, 0);
//      jack_on_shutdown(_client, processShutdown, 0);
//      jack_set_buffer_size_callback(_client, bufsize_callback, 0);
//      jack_set_sample_rate_callback(_client, srate_callback, 0);
//      jack_set_port_registration_callback(_client, registration_callback, 0);
//      jack_set_graph_order_callback(_client, graph_callback, 0);
//      jack_set_freewheel_callback (_client, freewheel_callback, 0);
//      jack_set_timebase_callback(_client, 0, timebase_callback, 0);

      sampleRate  = jack_get_sample_rate(client);
      segmentSize = jack_get_buffer_size(client);
      //
      //    instantiate Synthesizer
      //
      mess = descr->instantiate(sampleRate, instanceName.toLocal8Bit().data());
      if (mess == 0) {
            fprintf(stderr, "%s: instantiate failed\n",
               argv[0]);
            }

      int channels = mess->channels();
      if (channels > MAX_OUTPORTS) {
            channels = MAX_OUTPORTS;
            fprintf(stderr, "%s: too many outports %d > %d\n",
                  argv[0], channels, MAX_OUTPORTS);
            }
      for (int i = 0; i < channels; ++i) {
            char portName[64];
            snprintf(portName, 64, "audioOut-%d", i);
            outPorts[i] = jack_port_register(client, portName,
               JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0);
            }
      inPort = jack_port_register(client, "midiIn",
         JACK_DEFAULT_MIDI_TYPE, JackPortIsInput, 0);
      if (mess->hasGui())
            mess->showGui(true);
      jack_activate(client);
      qApp->exec();
      }

