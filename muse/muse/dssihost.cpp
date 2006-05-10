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

#include "config.h"

#ifdef DSSI_SUPPORT

#include <signal.h>
#include <dlfcn.h>
#include <dssi.h>
#include <alsa/asoundlib.h>

#include "dssihost.h"
#include "synth.h"
#include "driver/jackaudio.h"
#include "midi.h"
#include "al/al.h"
#include "al/xml.h"
#include "song.h"
#include "midictrl.h"

static lo_server_thread serverThread;
static char osc_path_tmp[1024];
static char* url;

//---------------------------------------------------------
//   oscError
//---------------------------------------------------------

static void oscError(int num, const char *msg, const char *path)
      {
      fprintf(stderr, "MusE: liblo server error %d in path %s: %s\n",
          num, path, msg);
      }

//---------------------------------------------------------
//   oscDebugHandler
//---------------------------------------------------------

static int oscDebugHandler(const char* path, const char* types, lo_arg** argv,
   int argc, void*, void*)
      {
      printf("MusE: got unhandled OSC message:\n   path: <%s>\n", path);
      for (int i = 0; i < argc; i++) {
            printf("   arg %d '%c' ", i, types[i]);
            lo_arg_pp(lo_type(types[i]), argv[i]);
            printf("\n");
            }
      return 1;
      }

//---------------------------------------------------------
//   oscUpdate
//---------------------------------------------------------

int DssiSynthIF::oscUpdate(lo_arg **argv)
      {
      const char *url = (char *)&argv[0]->s;

      if (uiTarget)
            lo_address_free(uiTarget);
      char* host = lo_url_get_hostname(url);
      char* port = lo_url_get_port(url);
      uiTarget   = lo_address_new(host, port);
      free(host);
      free(port);

      if (uiOscPath)
            free(uiOscPath);
      uiOscPath = lo_url_get_path(url);
      int pl = strlen(uiOscPath);

      if (uiOscControlPath)
            free(uiOscControlPath);
      uiOscControlPath = (char *)malloc(pl + 10);
      sprintf(uiOscControlPath, "%s/control", uiOscPath);

      if (uiOscConfigurePath)
            free(uiOscConfigurePath);
      uiOscConfigurePath = (char *)malloc(pl + 12);
      sprintf(uiOscConfigurePath, "%s/configure", uiOscPath);

      if (uiOscProgramPath)
            free(uiOscProgramPath);
      uiOscProgramPath = (char *)malloc(pl + 10);
      sprintf(uiOscProgramPath, "%s/program", uiOscPath);

      if (uiOscShowPath)
            free(uiOscShowPath);
      uiOscShowPath = (char *)malloc(pl + 10);
      sprintf(uiOscShowPath, "%s/show", uiOscPath);

      /* At this point a more substantial host might also call
      * configure() on the UI to set any state that it had remembered
      * for the plugin instance.  But we don't remember state for
      * plugin instances (see our own configure() implementation in
      * osc_configure_handler), and so we have nothing to send except
      * the optional project directory.
      */

      lo_send(uiTarget, uiOscConfigurePath, "ss",
         DSSI_PROJECT_DIRECTORY_KEY, song->projectDirectory().toLatin1().data());

#if 0
      /* Send current bank/program  (-FIX- another race...) */
      if (instance->pendingProgramChange < 0) {
            unsigned long bank = instance->currentBank;
            unsigned long program = instance->currentProgram;
            instance->uiNeedsProgramUpdate = 0;
            if (instance->uiTarget) {
                  lo_send(instance->uiTarget, instance->ui_osc_program_path, "ii", bank, program);
                  }
            }

      /* Send control ports */
      for (i = 0; i < instance->plugin->controlIns; i++) {
            int in = i + instance->firstControlIn;
            int port = pluginControlInPortNumbers[in];
            lo_send(instance->uiTarget, instance->ui_osc_control_path, "if", port,
               pluginControlIns[in]);
            /* Avoid overloading the GUI if there are lots and lots of ports */
            if ((i+1) % 50 == 0)
                  usleep(300000);
            }
#endif
      return 0;
      }

//---------------------------------------------------------
//   oscMessageHandler
//---------------------------------------------------------

int oscMessageHandler(const char* path, const char* types, lo_arg** argv,
   int argc, void* data, void* user_data)
      {
      const char* p = path;
      if (strncmp(p, "/dssi/", 6))
            return oscDebugHandler(path, types, argv, argc, data, user_data);

      p += 6;

      SynthIList* sl = song->syntis();
      DssiSynthIF* instance = 0;
      SynthI* synti = 0;

      for (iSynthI si = sl->begin(); si != sl->end(); ++si) {
            int l = strlen((*si)->name().toLatin1().data());
            if (!strncmp(p, (*si)->name().toLatin1().data(), l)) {
                  synti = *si;
                  instance = (DssiSynthIF*)(synti->sif());
                  p += l;
                  break;
                  }
            }
      if (!instance)
            return oscDebugHandler(path, types, argv, argc, data, user_data);

      if (*p != '/' || *(p + 1) == 0)
            return oscDebugHandler(path, types, argv, argc, data, user_data);
      ++p;

      if (!strcmp(p, "configure") && argc == 2 && !strcmp(types, "ss"))
            return instance->oscConfigure(argv);
      else if (!strcmp(p, "control") && argc == 2 && !strcmp(types, "if"))
            return instance->oscControl(argv);
      else if (!strcmp(p, "midi") && argc == 1 && !strcmp(types, "m"))
            return instance->oscMidi(argv);
      else if (!strcmp(p, "program") && argc == 2 && !strcmp(types, "ii"))
            return instance->oscProgram(argv);
      else if (!strcmp(p, "update") && argc == 1 && !strcmp(types, "s"))
            return instance->oscUpdate(argv);
      else if (!strcmp(p, "exiting") && argc == 0)
            return instance->oscExiting(argv);
      return oscDebugHandler(path, types, argv, argc, data, user_data);
      }

//---------------------------------------------------------
//   scanDSSILib
//---------------------------------------------------------

static void scanDSSILib(const QFileInfo& fi)
      {
      void* handle = dlopen(fi.filePath().toLatin1().data(), RTLD_NOW);
      if (handle == 0) {
            fprintf(stderr, "dlopen(%s) failed: %s\n",
              fi.filePath().toAscii().data(), dlerror());
            return;
            }
      DSSI_Descriptor_Function dssi = (DSSI_Descriptor_Function)dlsym(handle, "dssi_descriptor");

      if (!dssi) {
            const char *txt = dlerror();
            if (txt) {
                  fprintf(stderr,
                     "Unable to find dssi_descriptor() function in plugin "
                     "library file \"%s\": %s.\n"
                     "Are you sure this is a DSSI plugin file?\n",
                     fi.filePath().toAscii().data(),
                     txt);
                  exit(1);
                  }
            }
      const DSSI_Descriptor* descr;
      for (int i = 0;; ++i) {
            descr = dssi(i);
            if (descr == 0)
                  break;
            QString label(descr->LADSPA_Plugin->Label);
            DssiSynth* s = new DssiSynth(&fi, label);
            synthis.push_back(s);
            }
      dlclose(handle);
      }

//---------------------------------------------------------
//   scanVstDir
//---------------------------------------------------------

static void scanDSSIDir(const QString& s)
      {
      if (debugMsg)
            printf("scan dssi plugin dir <%s>\n", s.toLatin1().data());

      QDir pluginDir(s, QString("*.so"), QDir::Unsorted, QDir::Files);
      if (!pluginDir.exists())
            return;

      const QFileInfoList list = pluginDir.entryInfoList();
      for (int i = 0; i < list.size(); ++i) {
      	QFileInfo fi = list.at(i);
            scanDSSILib(fi);
            }
      }

//---------------------------------------------------------
//   initDSSI
//---------------------------------------------------------

void initDSSI()
      {
      char* dssiPath = getenv("DSSI_PATH");
      if (dssiPath == 0)
            dssiPath = "/usr/lib/dssi:/usr/local/lib/dssi";

      char* p = dssiPath;
      while (*p != '\0') {
            char* pe = p;
            while (*pe != ':' && *pe != '\0')
                  pe++;

            int n = pe - p;
            if (n) {
                  char* buffer = new char[n + 1];
                  strncpy(buffer, p, n);
                  buffer[n] = '\0';
                  scanDSSIDir(QString(buffer));
                  delete[] buffer;
                  }
            p = pe;
            if (*p == ':')
                  p++;
            }
      // Create OSC thread

      serverThread = lo_server_thread_new(0, oscError);
      snprintf(osc_path_tmp, 31, "/dssi");
      char* tmp = lo_server_thread_get_url(serverThread);
      url = (char *)malloc(strlen(tmp) + strlen(osc_path_tmp));
      sprintf(url, "%s%s", tmp, osc_path_tmp + 1);
      free(tmp);

      lo_server_thread_add_method(serverThread, 0, 0, oscMessageHandler, 0);
      lo_server_thread_start(serverThread);
      }

//---------------------------------------------------------
//   guiVisible
//---------------------------------------------------------

bool DssiSynthIF::guiVisible() const
      {
      return _guiVisible;
      }

//---------------------------------------------------------
//   showGui
//---------------------------------------------------------

void DssiSynthIF::showGui(bool v)
      {
      if (v == guiVisible())
            return;
      if (uiOscPath == 0) {
            printf("no uiOscPath\n");
            return;
            }
      char uiOscGuiPath[strlen(uiOscPath)+6];
      sprintf(uiOscGuiPath, "%s/%s", uiOscPath, v ? "show" : "hide");
      lo_send(uiTarget, uiOscGuiPath, "");
      _guiVisible = v;
      }

//---------------------------------------------------------
//   receiveEvent
//---------------------------------------------------------

MidiEvent DssiSynthIF::receiveEvent()
      {
      return MidiEvent();
      }

//---------------------------------------------------------
//   init
//---------------------------------------------------------

bool DssiSynthIF::init(DssiSynth* s)
      {
      synth = s;
      const DSSI_Descriptor* dssi = synth->dssi;
      const LADSPA_Descriptor* ld = dssi->LADSPA_Plugin;
      handle = ld->instantiate(ld, AL::sampleRate);
      if (ld->activate)
            ld->activate(handle);

      queryPrograms();
      int controlPorts = synth->_controller;
      controls = new LadspaPort[controlPorts];

      for (int k = 0; k < controlPorts; ++k) {
		int i = synth->pIdx[k];
		controls[k].val = ladspaDefaultValue(ld, i);
		ld->connect_port(handle, i, &controls[k].val);
            }
      if (dssi->configure) {
            char *rv = dssi->configure(handle, DSSI_PROJECT_DIRECTORY_KEY,
               museProject.toLatin1().data());
            if (rv)
                  fprintf(stderr, "MusE: Warning: plugin doesn't like project directory: \"%s\"\n", rv);
            }
      return true;
      }

//---------------------------------------------------------
//   ~DssiSynthIF
//---------------------------------------------------------

DssiSynthIF::~DssiSynthIF()
      {
      const DSSI_Descriptor* dssi = synth->dssi;
      const LADSPA_Descriptor* descr = dssi->LADSPA_Plugin;

      if (descr->cleanup)
            descr->cleanup(handle);
      if (guiPid != -1)
            kill(guiPid, SIGHUP);
      }

//---------------------------------------------------------
//   setParameter
//---------------------------------------------------------

void DssiSynthIF::setParameter(int, float)
      {
      }

//---------------------------------------------------------
//   write
//---------------------------------------------------------

void DssiSynthIF::write(Xml&) const
      {
      }

//---------------------------------------------------------
//   getData
//---------------------------------------------------------

iMPEvent DssiSynthIF::getData(MPEventList* el, iMPEvent i, unsigned, int ch, unsigned samples, float** data)
      {
      const DSSI_Descriptor* dssi = synth->dssi;
      const LADSPA_Descriptor* descr = dssi->LADSPA_Plugin;

      unsigned long nevents = 0;
      for (iMPEvent ii = i; ii != el->end(); ++ii, ++nevents)
            ;
      snd_seq_event_t events[nevents];
      memset(events, 0, sizeof(events));

      nevents = 0;
      for (; i != el->end(); ++i, ++nevents) {
            MidiEvent e = *i;
            int chn = e.channel();
            int a   = e.dataA();
            int b   = e.dataB();

            snd_seq_event_t* event = &events[nevents];
            event->queue = SND_SEQ_QUEUE_DIRECT;

            switch(e.type()) {
                  case ME_NOTEON:
                        if (b)
                              snd_seq_ev_set_noteon(event, chn, a, b);
                        else
                              snd_seq_ev_set_noteoff(event, chn, a, 0);
                        break;
                  case ME_NOTEOFF:
                        snd_seq_ev_set_noteoff(event, chn, a, 0);
                        break;
                  case ME_CONTROLLER:
                        if (a == CTRL_PROGRAM) {
                              int bank = b >> 8;
                              int prog = b & 0xff;
                              if (dssi->select_program)
                                    dssi->select_program(handle, bank, prog);
                              break;
                              }
//printf("ctrl %x %d = %d\n", a, a, b);
//                        snd_seq_ev_set_controller(event, chn, a, b);
                        break;
                  case ME_PITCHBEND:
                        snd_seq_ev_set_pitchbend(event, chn, a);
                        break;
                  case ME_AFTERTOUCH:
                        snd_seq_ev_set_chanpress(event, chn, a);
                        break;
                  default:
                        --nevents;
                        break;
                  }
            }

      for (int k = 0; k < ch; ++k)
            descr->connect_port(handle, synth->oIdx[k], data[k]);

      if (synth->dssi->run_synth)
            synth->dssi->run_synth(handle, samples, events, nevents);
      else if (synth->dssi->run_multiple_synths) {
            snd_seq_event_t* ev = events;
            synth->dssi->run_multiple_synths(1, &handle, samples, &ev, &nevents);
            }
      return i;
      }

//---------------------------------------------------------
//   putEvent
//---------------------------------------------------------

bool DssiSynthIF::putEvent(const MidiEvent& ev)
      {
      if (midiOutputTrace)
            ev.dump();
      return false;
      }

//---------------------------------------------------------
//   incInstances
//---------------------------------------------------------

void DssiSynth::incInstances(int val)
      {
      _instances += val;
      if (_instances == 0) {
            if (handle)
                  dlclose(handle);
            dssi = 0;
            df   = 0;
            }
      }

//---------------------------------------------------------
//   createSIF
//---------------------------------------------------------

SynthIF* DssiSynth::createSIF(SynthI* synti)
      {
      if (_instances == 0) {
            handle = dlopen(info.filePath().toAscii().data(), RTLD_NOW);
            if (handle == 0) {
                  fprintf(stderr, "dlopen(%s) failed: %s\n",
                    info.filePath().toAscii().data(), dlerror());
                  return 0;
                  }
            df = (DSSI_Descriptor_Function)dlsym(handle, "dssi_descriptor");

            if (!df) {
                  const char *txt = dlerror();
                  fprintf(stderr,
                     "Unable to find dssi_descriptor() function in plugin "
                     "library file \"%s\": %s.\n"
                     "Are you sure this is a DSSI plugin file?\n",
                     info.filePath().toAscii().data(),
                     txt ? txt : "?");
                  dlclose(handle);
                  handle = 0;
                  return 0;
                  }
            for (int i = 0;; ++i) {
                  dssi = df(i);
                  if (dssi == 0)
                        break;
                  QString label(dssi->LADSPA_Plugin->Label);
                  if (label == _name)
                        break;
                  }

            _inports    = 0;
            _outports   = 0;
            _controller = 0;
            const LADSPA_Descriptor* d = dssi->LADSPA_Plugin;
            for (unsigned k = 0; k < d->PortCount; ++k) {
                  LADSPA_PortDescriptor pd = d->PortDescriptors[k];
                  static const int CI = LADSPA_PORT_CONTROL | LADSPA_PORT_INPUT;
                  if ((pd &  CI) == CI) {
                        ++_controller;
                        pIdx.push_back(k);
                        }
                  else if (pd &  LADSPA_PORT_INPUT) {
                        ++_inports;
                        iIdx.push_back(k);
                        }
                  else if (pd &  LADSPA_PORT_OUTPUT) {
                        ++_outports;
                        oIdx.push_back(k);
                        }
                  }
            }
      if (dssi == 0) {
            fprintf(stderr, "cannot found DSSI synti %s\n", _name.toLatin1().data());
            dlclose(handle);
            handle = 0;
            df     = 0;
            return 0;
            }
      DssiSynthIF* sif = new DssiSynthIF(synti);
      ++_instances;
      sif->init(this);

      //
      //  start gui
      //
      static char oscUrl[1024];
      snprintf(oscUrl, 1024, "%s/%s", url, synti->name().toLatin1().data());

      QString guiPath(info.path() + "/" + info.baseName());
      QDir guiDir(guiPath, "*", QDir::Unsorted, QDir::Files);
      if (guiDir.exists()) {
            const QFileInfoList list = guiDir.entryInfoList();
            for (int i = 0; i < list.size(); ++i) {
            	QFileInfo fi = list.at(i);
                  QString gui(fi.filePath());
                  if (gui.contains('_') == 0)
                        continue;
                  struct stat buf;
                  if (stat(gui.toLatin1().data(), &buf)) {
                        perror("stat failed");
                        continue;
                        }

                  if ((S_ISREG(buf.st_mode) || S_ISLNK(buf.st_mode)) &&
                     (buf.st_mode & (S_IXUSR | S_IXGRP | S_IXOTH))) {
                        if ((sif->guiPid = fork()) == 0) {
                              execlp(
                                 fi.filePath().toLatin1().data(),
                                 fi.fileName().toLatin1().data(),
                                 oscUrl,
                                 info.filePath().toLatin1().data(),
                                 name().toLatin1().data(),
                                 "channel 1", (void*)0);
                              fprintf(stderr, "exec %s %s %s %s failed: %s\n",
                                 fi.filePath().toLatin1().data(),
                                 fi.fileName().toLatin1().data(),
                                 oscUrl,
                                 name().toLatin1().data(),
                                 strerror(errno));
                              exit(1);
                              }
                        }
                  }
            _hasGui = true;
            }
      else {
            printf("%s: no dir for dssi gui found: %s\n",
               name().toLatin1().data(), guiPath.toLatin1().data());
            _hasGui = false;
            }
      return sif;
      }

//---------------------------------------------------------
//   oscProgram
//---------------------------------------------------------

int DssiSynthIF::oscProgram(lo_arg** argv)
      {
      int bank    = argv[0]->i;
      int program = argv[1]->i;
      CVal cval;
      cval.i = (bank << 8) + program;

printf("received oscProgram\n");
#if 0 //TD
      MidiTrackList* tl = song->midis();
      for (iMidiTrack i = tl->begin(); i != tl->end(); ++i) {
            MidiTrack* t = *i;
            MidiPort* port = &midiPorts[t->outPort()];
            MidiDevice* dev = port->device();
            if (dev == synti) {
                  song->setControllerVal(t, CTRL_PROGRAM, cval);
                  break;
                  }
            }
#endif
      return 0;
      }

//---------------------------------------------------------
//   oscControl
//---------------------------------------------------------

int DssiSynthIF::oscControl(lo_arg**)
      {
printf("received oscControl\n");
#if 0
      int port = argv[0]->i;
      LADSPA_Data value = argv[1]->f;

      if (port < 0 || port > instance->plugin->descriptor->LADSPA_Plugin->PortCount) {
            fprintf(stderr, "MusE: OSC: %s port number (%d) is out of range\n",
               instance->friendly_name, port);
            return 0;
            }
      if (instance->pluginPortControlInNumbers[port] == -1) {
            fprintf(stderr, "MusE: OSC: %s port %d is not a control in\n",
               instance->friendly_name, port);
            return 0;
            }
      pluginControlIns[instance->pluginPortControlInNumbers[port]] = value;
      if (verbose) {
            printf("MusE: OSC: %s port %d = %f\n",
               instance->friendly_name, port, value);
            }
#endif
      return 0;
      }

//---------------------------------------------------------
//   oscExiting
//---------------------------------------------------------

int DssiSynthIF::oscExiting(lo_arg**)
      {
      printf("not impl.: oscExiting\n");
#if 0
      int i;

      if (verbose) {
            printf("MusE: OSC: got exiting notification for instance %d\n",
               instance->number);
            }

      if (instance->plugin) {

            /*!!! No, this isn't safe -- plugins deactivated in this way
              would still be included in a run_multiple_synths call unless
              we re-jigged the instance array at the same time -- leave it
              for now
            if (instance->plugin->descriptor->LADSPA_Plugin->deactivate) {
                  instance->plugin->descriptor->LADSPA_Plugin->deactivate
                     (instanceHandles[instance->number]);
                  }
            */
            /* Leave this flag though, as we need it to determine when to exit */
            instance->inactive = 1;
            }

      /* Do we have any plugins left running? */

      for (i = 0; i < instance_count; ++i) {
            if (!instances[i].inactive)
                  return 0;
            }

      if (verbose) {
            printf("MusE: That was the last remaining plugin, exiting...\n");
            }
      exiting = 1;
#endif
      return 0;
      }

//---------------------------------------------------------
//   oscMidi
//---------------------------------------------------------

int DssiSynthIF::oscMidi(lo_arg** /*argv*/)
      {
printf("received oscMidi\n");
      MidiTrackList* tl = song->midis();
      for (iMidiTrack i = tl->begin(); i != tl->end(); ++i) {
#if 0 //TD
            MidiTrack* t = *i;
            MidiChannel* mc = t->channel();
            MidiPort* port = &midiPorts[t->outPort()];
            MidiDevice* dev = port->device();
            if (dev == synti) {
                  int a = argv[0]->m[1];
                  int b = argv[0]->m[2];
                  int c = argv[0]->m[3];
                  if (a == ME_NOTEOFF) {
                        a = ME_NOTEON;
                        c = 0;
                        }
                  MidiEvent event(0, t->outPort(), t->outChannel(), a, b, c);
                  // dev->recordEvent(event);
                  audio->msgPlayMidiEvent(&event);
                  break;
                  }
#endif
            }
      return 0;
      }

//---------------------------------------------------------
//   oscConfigure
//---------------------------------------------------------

int DssiSynthIF::oscConfigure(lo_arg** argv)
      {
      if (!synth->dssi->configure)
            return 0;

      const char *key = (const char *)&argv[0]->s;
      const char *value = (const char *)&argv[1]->s;

      /* This is pretty much the simplest legal implementation of
       * configure in a DSSI host. */

      /* The host has the option to remember the set of (key,value)
       * pairs associated with a particular instance, so that if it
       * wants to restore the "same" instance on another occasion it can
       * just call configure() on it for each of those pairs and so
       * restore state without any input from a GUI.  Any real-world GUI
       * host will probably want to do that.  This host doesn't have any
       * concept of restoring an instance from one run to the next, so
       * we don't bother remembering these at all. */

      if (!strncmp(key, DSSI_RESERVED_CONFIGURE_PREFIX,
         strlen(DSSI_RESERVED_CONFIGURE_PREFIX))) {
            fprintf(stderr, "MusE: OSC: UI for plugin '%s' attempted to use reserved configure key \"%s\", ignoring\n",
               synti->name().toLatin1().data(), key);
            return 0;
            }

      char* message = synth->dssi->configure(handle, key, value);
      if (message) {
            printf("MusE: on configure '%s' '%s', plugin '%s' returned error '%s'\n",
               key, value, synti->name().toLatin1().data(), message);
            free(message);
            }

      // also call back on UIs for plugins other than the one
      // that requested this:
      // if (n != instance->number && instances[n].uiTarget) {
      //      lo_send(instances[n].uiTarget,
      //      instances[n].ui_osc_configure_path, "ss", key, value);
      //      }

      /* configure invalidates bank and program information, so
        we should do this again now: */
      queryPrograms();
      return 0;
      }

//---------------------------------------------------------
//   queryPrograms
//---------------------------------------------------------

void DssiSynthIF::queryPrograms()
      {
      for (std::vector<DSSI_Program_Descriptor>::const_iterator i = programs.begin();
         i != programs.end(); ++i) {
            free((void*)(i->Name));
            }
      programs.clear();

      if (!(synth->dssi->get_program && synth->dssi->select_program))
            return;

      for (int i = 0;; ++i) {
            const DSSI_Program_Descriptor* pd = synth->dssi->get_program(handle, i);
            if (pd == 0)
                  break;
            DSSI_Program_Descriptor d;
            d.Name    = strdup(pd->Name);
            d.Program = pd->Program;
            d.Bank    = pd->Bank;
            programs.push_back(d);
            }
      }

//---------------------------------------------------------
//   getPatchName
//---------------------------------------------------------

QString DssiSynthIF::getPatchName(int, int prog)
      {
      unsigned program = prog & 0x7f;
      int lbank   = (prog >> 8) & 0xff;
      int hbank   = (prog >> 16) & 0xff;

      if (lbank == 0xff)
            lbank = 0;
      if (hbank == 0xff)
            hbank = 0;
      unsigned bank = (hbank << 8) + lbank;

      for (std::vector<DSSI_Program_Descriptor>::const_iterator i = programs.begin();
         i != programs.end(); ++i) {
            if (i->Bank == bank && i->Program ==program)
                  return i->Name;
            }
      return "?";
      }

//---------------------------------------------------------
//   populatePatchPopup
//---------------------------------------------------------

void DssiSynthIF::populatePatchPopup(QMenu* menu, int)
      {
      menu->clear();

      for (std::vector<DSSI_Program_Descriptor>::const_iterator i = programs.begin();
         i != programs.end(); ++i) {
            int bank = i->Bank;
            int prog = i->Program;
            int id   = (bank << 16) + prog;
            QAction* a = menu->addAction(QString(i->Name));
            a->setData(id);
            }
      }

#else
void initDSSI() {}
#endif

