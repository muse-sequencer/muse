//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: fluid.cpp,v 1.18.2.8 2009/12/06 10:05:00 terminator356 Exp $
//
//  This file is derived from fluid Synth and modified
//    for MusE.
//  Parts of fluid are derived from Smurf Sound Font Editor.
//  Parts of Smurf Sound Font Editor are derived from
//    awesfx utilities
//  Smurf:  Copyright (C) 1999-2000 Josh Green
//  fluid:  Copyright (C) 2001 Peter Hanappe
//  MusE:   Copyright (C) 2001 Werner Schweer
//  awesfx: Copyright (C) 1996-1999 Takashi Iwai
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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdarg.h>
#include <errno.h>
#include <pthread.h>
#include <cmath>
#include <sys/mman.h>
#include <sys/time.h>
#include <sched.h>

#include "muse/midictrl.h"

//#include "common_defs.h"
#include "muse/midi.h"
#include "fluid.h"
#include "fluidgui.h"

//---------------------------------------------------------
//   instantiate
//---------------------------------------------------------

class QWidget;

static Mess* instantiate(int sr, QWidget*, QString* /* projectPathPtr */, const char* name)
      {
      ISynth* synth = new ISynth();
      synth->setSampleRate(sr);
      if (synth->init(name)) {
            delete synth;
            synth = 0;
            }
      return synth;
      }

//---------------------------------------------------------
//    MESS
//---------------------------------------------------------

extern "C" {
      static MESS descriptor = {
            "fluid",
            "fluid soundfont loader by Werner Schweer",   
            "0.1",      // fluid version string
            MESS_MAJOR_VERSION, MESS_MINOR_VERSION,
            instantiate,
            };
      // We must compile with -fvisibility=hidden to avoid namespace
      // conflicts with global variables.
      // Only visible symbol is "mess_descriptor".
      // (TODO: all plugins should be compiled this way)
  
      __attribute__ ((visibility("default")))
      const MESS* mess_descriptor() { return &descriptor; }
      }

//---------------------------------------------------------
//   ISynth
//---------------------------------------------------------

ISynth::ISynth()
   : Mess(2)
      {
      _busy       = false;
      sfont       = 0;
      _gmMode     = false;     // General Midi Mode
      _fluidsynth = 0;
      initBuffer  = 0;
      initLen     = 0;
      fontId      = -1;
      }

//---------------------------------------------------------
//   playNote
//    return true if busy
//---------------------------------------------------------

bool ISynth::playNote(int channel, int pitch, int velo)
      {
      if (_busy) {
//            printf("fluid: playNote(): busy!\n");
            return true;
            }
      if (velo) {
            int err = fluid_synth_noteon(_fluidsynth, channel, pitch, velo);
            if (err) {
                  printf("ISynth: noteon error, channel %d pitch %d<%s>\n",
                     channel, pitch, fluid_synth_error(_fluidsynth));
			}
            }
      else
            fluid_synth_noteoff(_fluidsynth, channel, pitch);
      return false;
      }

//---------------------------------------------------------
//   setController
//    return true if busy
//---------------------------------------------------------

bool ISynth::setController(int ch, int ctrl, int val)
      {
      if (_busy) {
//            printf("fluid: setController(): busy!\n");
            return true;
            }
      switch(ctrl) {
            case MusECore::CTRL_PROGRAM:
                  {
                  int hbank = (val & 0xff0000) >> 16;
                  int lbank = (val & 0xff00) >> 8;
                  if (hbank > 127)  // map "dont care" to 0
                        hbank = 0;
                  if (lbank > 127)
                        lbank = 0;
                  if (lbank == 127 || ch == 9)       // drum HACK
                        lbank = 128;
                  int prog  = val & 0x7f;
                  fluid_synth_program_select(_fluidsynth, ch,
                     hbank, lbank, prog);
                  }
                  break;

            case MusECore::CTRL_PITCH:
                  // MusE's range is from -8192 to +8191, fluidsynth seems to be [0, 16384]
                  val +=8192;
                  fluid_synth_pitch_bend (_fluidsynth, ch, val);
                  break;

            default:
// printf("controller %x val %x\n", ctrl & 0x3fff, val);
                  fluid_synth_cc(_fluidsynth, ch, ctrl & 0x3fff, val);
                  break;
            }
      return false;
      }

//---------------------------------------------------------
//   sysex
//    7e 7f 09 01          GM on
//    7e 7f 09 02          GM off
//    7f 7f 04 01 ll hh    Master Volume (ll-low byte, hh-high byte)
//    7c 00 01 nn ...      replace Soundfont (nn-ascii char of path
//    7c 00 02 nn ...      add Soundfont
//    7c 00 03 nn ...      remove Soundfont
//
//    return true if busy
//---------------------------------------------------------

bool ISynth::sysex(int len, const unsigned char* data)
      {
      if (_busy) {
//            printf("fluid: sysex(): busy!\n");
            return true;
            }
      if (len >= 4) {
            //---------------------------------------------
            //  Universal Non Realtime
            //---------------------------------------------

            if (data[0] == 0x7e) {
                  if (data[1] == 0x7f) {  // device Id
                        if (data[2] == 0x9) {   // GM
                              if (data[3] == 0x1) {
                                    gmOn(true);
                                    return false;
                                    }
                              else if (data[3] == 0x2) {
                                    gmOn(false);
                                    return false;
                                    }
                              }
                        }
                  }
            //---------------------------------------------
            //  Universal Realtime
            //---------------------------------------------

            else if (data[0] == 0x7f) {
                  if (data[1] == 0x7f) {  // device Id
                        if ((data[2] == 0x4) && (data[3] == 0x1)) {
                           if(len == 6)
                           {
                              float v = (data[5]*128 + data[4])/32767.0;
                              fluid_synth_set_gain(_fluidsynth, v);
                              return false;
                              }
                           }   
                        }
                  }
            //---------------------------------------------
            //  MusE Soft Synth
            //---------------------------------------------

            else if (data[0] == MUSE_SYNTH_SYSEX_MFG_ID) {
                  int n = len - 3;
                  if (n < 1) {
                        #ifdef FS_DEBUG
                        printf("fluid: bad sysEx:\n");
                        #endif
                        return false;
                        }
                  char buffer[n+1];
                  memcpy(buffer, (char*)data+3, n);
                  buffer[n] = 0;
                  if (data[1] == FLUID_UNIQUE_ID) {     // fluid
                        //if (data[2] == 1) {  // load sound font
                        if (data[2] == SF_REPLACE) {  // load sound font
                              sysexSoundFont(SF_REPLACE, buffer);
                              return false;
                              }
                        //else if (data[2] == 2) {  // load sound font
                        else if (data[2] == SF_ADD) {  // load sound font
                              sysexSoundFont(SF_ADD, buffer);
                              return false;
                              }
                        //else if (data[2] == 3) {  // load sound font
                        else if (data[2] == SF_REMOVE) {  // load sound font
                              sysexSoundFont(SF_REMOVE, buffer);
                              return false;
                              }
                        }
                  }
            
            else if (data[0] == 0x41) {   // roland
                  if (data[1] == 0x10 && data[2] == 0x42 && data[3] == 0x12)
                  {
                     if (len == 8) {
                       if(data[4] == 0x40 && data[5] == 00 && data[6] == 0x7f && data[7] == 0x41) {
                          // gs on
                          gmOn(true);
                          return false;
                       }    
                     }
                  }      
                }
            }      
      #ifdef FS_DEBUG
      printf("fluid: unknown sysex received, len %d:\n", len);
      for (int i = 0; i < len; ++i)
            printf("%02x ", data[i]);
      printf("\n");
      #endif
      return false;
      }

//---------------------------------------------------------
//   gmOn
//---------------------------------------------------------

void ISynth::gmOn(bool flag)
      {
      _gmMode = flag;
      allNotesOff();
      }

//---------------------------------------------------------
//   allNotesOff
//    stop all notes
//---------------------------------------------------------

void ISynth::allNotesOff()
      {
      for (int ch = 0; ch < 16; ++ch) {
            fluid_synth_cc(_fluidsynth, ch, 0x7b, 0);  // all notes off
            }
      }

//---------------------------------------------------------
//   guiVisible
//---------------------------------------------------------

bool ISynth::nativeGuiVisible() const
      {
      return gui->isVisible();
      }

//---------------------------------------------------------
//   showGui
//---------------------------------------------------------

void ISynth::showNativeGui(bool flag)
      {
      gui->setVisible(flag);
      }

//---------------------------------------------------------
//   ~ISynth
//---------------------------------------------------------

ISynth::~ISynth()
      {
      if(gui)        
        delete gui;  // p4.0.27
      // TODO delete settings
      if (_fluidsynth)
            delete_fluid_synth(_fluidsynth);
      if (initBuffer)
            delete [] initBuffer;
      }

//---------------------------------------------------------
//   processMessages
//   Called from host always, even if output path is unconnected.
//---------------------------------------------------------

void ISynth::processMessages()
{
  //Process messages from the gui
  if (!_busy) 
  {
    //
    //  get and process all pending events from the
    //  synthesizer GUI
    //
    while (gui->fifoSize())
          processEvent(gui->readEvent());
  }
}
  
//---------------------------------------------------------
//   process
//   Called from host, ONLY if output path is connected.
//---------------------------------------------------------

void ISynth::process(float** ports, int offset, int n)
      {
      if (!_busy) {
            /*
            //
            //  get and process all pending events from the
            //  synthesizer GUI
            //
            while (gui->fifoSize())
                  processEvent(gui->readEvent());
            */      
            fluid_synth_write_float(_fluidsynth, n, ports[0],
               offset, 1, ports[1], offset, 1);
            }
      // printf("%f %f\n", *ports[0], *(ports[0]+1));
      }

//---------------------------------------------------------
//   processEvent
//    All events from the sequencer go here
//---------------------------------------------------------
bool ISynth::processEvent(const MusECore::MidiPlayEvent& ev)
      {
      switch(ev.type()) {
            case MusECore::ME_CONTROLLER:
                setController(ev.channel(), ev.dataA(), ev.dataB());
                //return true;  // ??
                break;            
            case MusECore::ME_NOTEON:
                  return playNote(ev.channel(), ev.dataA(), ev.dataB());
            case MusECore::ME_NOTEOFF:
                  return playNote(ev.channel(), ev.dataA(), 0);
            case MusECore::ME_SYSEX:
                  return sysex(ev.len(), ev.data());
            case MusECore::ME_PITCHBEND:
                setController(ev.channel(), MusECore::CTRL_PITCH, ev.dataA());
                break;            
            case MusECore::ME_PROGRAM:
                setController(ev.channel(), MusECore::CTRL_PROGRAM, ev.dataA());
                break;   
            default:
                break;
            }
      return false;
      }

//---------------------------------------------------------
//   getPatchName
//---------------------------------------------------------

const char* ISynth::getPatchName(int /*ch*/, int val, int, bool /*drum*/) const
      {
      int prog =   val & 0xff;
      if(val == MusECore::CTRL_VAL_UNKNOWN || prog == 0xff)
            return "<unknown>";
      prog &= 0x7f;
      
      int hbank = (val & 0xff0000) >> 16;
      int lbank = (val & 0xff00) >> 8;
      if (hbank > 127)
            hbank = 0;
      if (lbank > 127)
            lbank = 0;
      if (lbank == 127)       // drum HACK
            lbank = 128;
      const char* name = "<unknown>";

      if (_busy) {
            //#ifdef FS_DEBUG
            printf("fluid: getPatchName(): busy!\n");
            //#endif
            return name;
            }
      fluid_font = fluid_synth_get_sfont_by_id(_fluidsynth, hbank);
      if (fluid_font) {
            fluid_preset_t* preset = (*fluid_font->get_preset)(fluid_font, lbank, prog);
            if (preset)
                  name = (*preset->get_name)(preset);
            else
            {
                  //#ifdef FS_DEBUG
                  fprintf(stderr, "no fluid preset for bank %d prog %d\n",
                     lbank, prog);
                  //#endif   
            }         
            }
      else
            fprintf(stderr, "ISynth::getPatchName(): no fluid font id=%d found\n", hbank);
      return name;
      }

//---------------------------------------------------------
//   getNextPatch
//---------------------------------------------------------

const MidiPatch* ISynth::getPatchInfo(int ch, const MidiPatch* p) const
      {
      if (_busy) {
            //#ifdef FS_DEBUG
            printf("fluid: getPatchInfo(): busy!\n");
            //#endif
            return 0;
            }
      if (p == 0) {
            // get font at font stack index 0
            fluid_font = fluid_synth_get_sfont(_fluidsynth, 0);
            if (fluid_font == 0)
                  return 0;
            (*fluid_font->iteration_start)(fluid_font);
            }
      fluid_preset_t preset;

      while ((*fluid_font->iteration_next)(fluid_font, &preset)) {
            patch.hbank = fluid_sfont_get_id(fluid_font);
            int bank = (*preset.get_banknum)(&preset);
            if (ch == 9 && bank != 128) // show only drums for channel 10
                  continue;
            if (bank == 128)
                  bank = 127;
            patch.typ   = 0;
            patch.name  = (*preset.get_name)(&preset);
            patch.lbank = bank;
            patch.prog  = (*preset.get_num)(&preset);
            return &patch;
            }
      return 0;
      }

//---------------------------------------------------------
//   getInitData
//    construct an initialization string which can be used
//    as a sysex to restore current state
//---------------------------------------------------------

void ISynth::getInitData(int* len, const unsigned char** data)
      {
      if (sfont == 0) {
            *len = 0;
            return;
            }
      int n = 4 + strlen(sfont);
      if (n > initLen) {
            if (initBuffer)
                  delete [] initBuffer;
            initBuffer = new unsigned char[n];
            initLen = n;    // p4.0.27 Tim.
            }
      initBuffer[0] = MUSE_SYNTH_SYSEX_MFG_ID;
      initBuffer[1] = FLUID_UNIQUE_ID;
      initBuffer[2] = SF_REPLACE;
      strcpy((char*)(initBuffer+3), sfont);
      *len = n;
      *data = (unsigned char*)initBuffer;
      }

//---------------------------------------------------------
//   sysexSoftfont
//---------------------------------------------------------

void ISynth::sysexSoundFont(SfOp op, const char* data)
      {
      char c = 'x';
      allNotesOff();
      switch(op) {
            case SF_REMOVE:
                  break;
            case SF_REPLACE:
            case SF_ADD:
                  if (sfont && (strcmp(sfont, data) == 0)) {
                        #ifdef FS_DEBUG
                        fprintf(stderr, "fluid: font already loaded\n");
                        #endif
                        break;
                        }
                  if (_busy) {
                        //#ifdef FS_DEBUG
                        fprintf(stderr, "fluid: busy!\n");
                        //#endif
                        break;
                        }
                  _busy = true;
                  if (sfont)
                        delete[] sfont;
                  sfont = new char[strlen(data)+1];
                  strcpy(sfont, data);
                  _busy = true;
                  write(writeFd, &c, 1);
                  break;
            }
      }

//---------------------------------------------------------
//   fontLoad
//    helper thread to load soundfont in the
//    background
//---------------------------------------------------------

static void* helper(void* t)
      {
      ISynth* is = (ISynth*) t;
      is->noRTHelper();
      pthread_exit(0);
      }

//------------------------------------
//   noRTHelper
//---------------------------------------------------------

void ISynth::noRTHelper()
      {
      for (;;) {
            char c;
            int n = read(readFd, &c, 1);
            if (n != 1) {
                  perror("ISynth::read ipc failed\n");
                  continue;
                  }
            int id = getFontId();
            if (id != -1) {
                  #ifdef FS_DEBUG
                  fprintf(stderr, "ISynth: unload old font\n");
                  #endif
                  fluid_synth_sfunload(synth(), (unsigned)id, true);
                  }
            const char* fontname = getFont();
            int rv = fluid_synth_sfload(synth(), fontname, true);
            if (rv == -1) {
                  fprintf(stderr, "ISynth: sfload %s failed\n",
                     fluid_synth_error(synth()));
                  }
            else {
                  setFontId(rv);
                  
                  // Inform the gui.      p4.0.27 Tim
                  int slen = strlen(fontname);
                  int n = slen + 2;
                  unsigned char d[n];
                  d[0] = FS_SEND_SOUNDFONT_NAME;
                  if(slen != 0)
                    memcpy(d + 1, fontname, slen);
                  d[1 + slen] = 0;
                  MusECore::MidiPlayEvent ev(0,0, MusECore::ME_SYSEX, d, n);
                  gui->writeEvent(ev);
                  
                  #ifdef FS_DEBUG
                  fprintf(stderr, "ISynth: sfont %s loaded as %d\n ",
                     getFont(), rv);
                  #endif   
                  }
            fluid_synth_set_gain(synth(), 1.0);  //?
            _busy = false;
            }
      }

//---------------------------------------------------------
//   init
//    return true on error
//---------------------------------------------------------

bool ISynth::init(const char* name)
      {
      fluid_settings_t* settings;
      settings = new_fluid_settings();
      fluid_settings_setnum(settings, (char*) "synth.sample-rate", float(sampleRate()));

      _fluidsynth = new_fluid_synth(settings);

      //---------------------------------------
      //    create non realtime helper thread
      //    create message channels
      //
      int filedes[2];         // 0 - reading   1 - writing
      if (pipe(filedes) == -1) {
            perror("ISynth::thread:creating pipe");
            return true;
            }
      readFd  = filedes[0];
      writeFd = filedes[1];

      pthread_attr_t* attributes = (pthread_attr_t*) malloc(sizeof(pthread_attr_t));
      pthread_attr_init(attributes);
      if (pthread_create(&helperThread, attributes, ::helper, this))
            perror("creating thread failed:");
      pthread_attr_destroy(attributes);

      char* p = getenv("DEFAULT_SOUNDFONT");
      if (p) {
            sfont = new char[strlen(p)+1];
            strcpy(sfont, p);
            char c = 'x';
            _busy = true;
            write(writeFd, &c, 1);
            }

      gui = new FLUIDGui;
      gui->setWindowTitle(QString(name));
      gui->show();
      return false;
      }

