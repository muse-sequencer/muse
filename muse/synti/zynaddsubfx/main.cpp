//=============================================================================
//  MusE
//  Linux Music Editor
//  $Id:$
//
//  Copyright (C) 2002-2006 by Werner Schweer and others
//
//  ZynAddSubFX - a software synthesizer
//  Copyright (C) 2002-2005 Nasca Octavian Paul
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

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>

#include "Misc/Master.h"
#include "Misc/Util.h"
#include "MasterUI.h"

//=========================================================
//    MESS interface
//=========================================================

#include "synti/libsynti/mess.h"
int instances = -1;

//---------------------------------------------------------
//   Zynadd
//---------------------------------------------------------

class Zynadd : public Mess, public Master 
      {
      virtual void process(float** buffer, int offset, int n);
      virtual bool processEvent(const MidiEvent&);
      virtual void getInitData(int*, const unsigned char**);
      virtual int getControllerInfo(int, const char**, int*, int*, int*);
      virtual const char* getPatchName(int, int, int) const;
      virtual const char* getBankName(int) const;
      virtual const MidiPatch* getPatchInfo(int, const MidiPatch*) const;
      virtual bool hasGui() const { return true; }
      virtual bool guiVisible() const { return _guiVisible; }
      virtual void showGui(bool val);

      mutable MidiPatch patch;
      mutable int currentBank;

      bool _guiVisible;
      bool loadBank(int);
      char* messPatch[MAX_NUM_BANKS][128];

   public:
      int Pexitprogram;
	MasterUI* ui;
	pthread_t thr;

      Zynadd();
      ~Zynadd();

      enum {
            GUI_NO_CMD, GUI_REFRESH, GUI_HIDE, GUI_SHOW
            };
      int guiCmd;
      };

//---------------------------------------------------------
//    guiThread
//---------------------------------------------------------

void* guiThread(void *arg)
      {
      Zynadd* z = (Zynadd *) arg;
      z->ui = new MasterUI(z, &z->Pexitprogram);
      z->ui->showUI();
      while (z->Pexitprogram == 0) {
            switch(z->guiCmd) {
                  case Zynadd::GUI_REFRESH:
                        z->ui->refresh_master_ui();
                        break;
                  case Zynadd::GUI_HIDE:
                        switch (config.cfg.UserInterfaceMode) {
	                        case 0:   
                                    z->ui->selectuiwindow->hide();
	                              break;
	                        case 1:
                                    z->ui->masterwindow->hide();
	                              break;
	                        case 2:
                                    z->ui->simplemasterwindow->hide();
	                              break;
                              }
                        break;
                  case Zynadd::GUI_SHOW:
                        z->ui->showUI();
                        break;
                  }
            z->guiCmd = Zynadd::GUI_NO_CMD;
            Fl::wait(0.01);
            }
      delete(z->ui);
      Fl::wait(0.01);
      pthread_exit(0);
      return 0;
      }

//---------------------------------------------------------
//   Zynadd
//---------------------------------------------------------

Zynadd::Zynadd() : Mess(2), Master()
      {
      instances++;
      swaplr        = config.cfg.SwapStereo;
      Pexitprogram  = 0;
      currentBank   = -1;
      guiCmd        = 0;

      swaplr = 0;   //1 for left-right swapping
      memset(messPatch, 0, sizeof(messPatch));
      bank.rescanforbanks();
      defaults();

      for (int i = 1; i < MAX_NUM_BANKS; ++i) {
            if (bank.banks[i].dir == 0)
                  break;
            loadBank(i);
            for (unsigned int k = 0; k < 128; ++k) {
            	if (!bank.emptyslot(k)) {
                        messPatch[i][k] = strdup(bank.getname(k));
                        }
                  }
            }

      pthread_create(&thr, NULL, guiThread, this);
      _guiVisible = true;
      }

//---------------------------------------------------------
//   ~Zynadd
//---------------------------------------------------------

Zynadd::~Zynadd()
      {
      Pexitprogram = 1;
      sleep(2);               //wait the thread to finish
      instances--;
      }

//---------------------------------------------------------
//    showGui
//---------------------------------------------------------

void Zynadd::showGui(bool val)
      {
      if (val != _guiVisible)
            guiCmd = val ? GUI_SHOW : GUI_HIDE;
      _guiVisible = val;
      }

//---------------------------------------------------------
//    loadBank
//---------------------------------------------------------

bool Zynadd::loadBank(int n)
      {
      if (n != currentBank) {
            if (bank.banks[n].dir == 0) {
                  printf("Zynaddsubfx: empty bank %d\n", n);
                        return false;
                  }
            bank.loadbank(bank.banks[n].dir);
            currentBank = n;
            }
      return true;
      }

//---------------------------------------------------------
//   getBankName
//---------------------------------------------------------

const char* Zynadd::getBankName(int n) const
      {
      n += 1;     // bank 0 is always empty ?!
//    printf("Zyn: getBankName %d <%s>\n", n, bank.banks[n].name);
      return bank.banks[n].name;
      }

//---------------------------------------------------------
//   getControllerInfo
//---------------------------------------------------------

struct ZynCtrl {
      const char* name;
      int num;
      int min;
      int max;
      };

int Zynadd::getControllerInfo(int i, const char** name, int* num, int* min, int* max)
      {
      static const ZynCtrl ctrl[] = {
            { "Pitch",          0x40000, -8191, 8190 },
            { "ProgramChange",  0x40001,  0, 0xffffff },
            { "Modulation",           1,  0, 127     },
            { "MainVolume",           7,  0, 127     },
            { "Pan",                 10,  0, 127     },
            { "Expression",          11,  0, 127     },
            { "Sustain",             64,  0, 127     },
            { "Portamento",          65,  0, 127     },
            { "FilterQ",	       71,  0, 127     },
            { "FilterCutoff",	       74,  0, 127     },
            { "Bandwidth",           75,  0, 127     },
            { "ModulationAmp",       76,  0, 127     },
            { "ResonanceCenter",     77,  0, 127     },
            { "ResonanceBandwidth",  78,  0, 127     },
            { "AllSoundsOff",       120,  0, 127     },
            { "ResetAllController", 121,  0, 127     },
            { "AllNotesOff",        123,  0, 127     },
            };
      if ((unsigned)i >= sizeof(ctrl)/sizeof(*ctrl))
            return 0;
      *name = ctrl[i].name;
      *num  = ctrl[i].num;
      *min  = ctrl[i].min;
      *max  = ctrl[i].max;
      return i+1;;
      }

//---------------------------------------------------------
//   getPatchName
//---------------------------------------------------------

const char* Zynadd::getPatchName(int, int val, int) const
      {
      int bankNo = (val >> 8) + 1;
      int program = val & 0x7f;
      return messPatch[bankNo][program];
      }

//---------------------------------------------------------
//   getPatchInfo
//---------------------------------------------------------

const MidiPatch* Zynadd::getPatchInfo(int, const MidiPatch* p) const
      {
      if (!p)
            return 0;
      int bn = ((p->hbank << 8) & 0xff) + ((p->lbank) & 0xff) + 1;
      for (unsigned int i = p->prog + 1; i < 128; ++i) {
            if (messPatch[bn][i]) {
                  patch.name  = messPatch[bn][i];
                  patch.typ   = 0xff;
                  patch.prog  = i;
                  patch.hbank = p->hbank;
                  patch.lbank = p->lbank;
                  return &patch;
                  }
            }
      return 0;
      }

//---------------------------------------------------------
//    getInitData
//---------------------------------------------------------

void Zynadd::getInitData(int* n, const unsigned char** data)
      {
      *n = getalldata((char **)data);
      }

//---------------------------------------------------------
//   process
//    synthesize n samples into buffer+offset
//---------------------------------------------------------

void Zynadd::process(float** outputs, int offset, int n)
      {
      float* outl = outputs[0] + offset;
      float* outr = outputs[1] + offset;
      if (busy) {
            memset(outl, 0, sizeof(float) * n);
            memset(outr, 0, sizeof(float) * n);
            return;
            }
      GetAudioOutSamples(n, outl, outr);
      }

//---------------------------------------------------------
//    processEvent
//---------------------------------------------------------

bool Zynadd::processEvent(const MidiEvent& e)
      {
      if (busy)
            return true;
      int ch = e.channel();
      switch(e.type()) {
            case 0x80:  // note off
	            NoteOff(ch, e.dataA());
                  break;
            case 0x90:  // note on
                  NoteOn(ch, e.dataA(), e.dataB());
		      break;
	      case 0xb0:  // controller
                  switch(e.dataA()) {
                        case 0x40000:     // pitch
	                        SetController(ch, C_pitchwheel, e.dataB());
                              break;
                        case 0x40001:     // program change
                              {
                              int bankNo = (e.dataB() >> 8) + 1;
                              if (!loadBank(bankNo))
                                    return false;
                              int program = e.dataB() & 0x7f;
                              if (bank.emptyslot(program)) {
                                    printf("Zynaddsubfx: programslot %d is empty!\n", program);
                                    break;
                                    }
                              for (int npart = 0; npart < NUM_MIDI_PARTS; npart++) {
                                    Part* p = part[npart];
	                              if ((ch == p->Prcvchn) && (p->Penabled != 0))
	                                    bank.loadfromslot(program, p);
                                    }
                              guiCmd = GUI_REFRESH;
                              }
                              break;
                        default:
                              {
                              int ctl;
                              switch (e.dataA()) {
	                              case 1:    ctl = C_modwheel; break;
                           	      case 7:    ctl = C_volume; break;
                           	      case 10:   ctl = C_panning; break;
                           	      case 11:   ctl = C_expression; break;
                           	      case 64:   ctl = C_sustain; break;
                           	      case 65:   ctl = C_portamento; break;
                           	      case 71:   ctl = C_filterq; break;
                           	      case 74:   ctl = C_filtercutoff; break;
                           	      case 75:   ctl = C_bandwidth; break;
                           	      case 76:   ctl = C_fmamp; break;
                           	      case 77:   ctl = C_resonance_center; break;
                           	      case 78:   ctl = C_resonance_bandwidth; break;
                           	      case 120:  ctl = C_allsoundsoff; break;
                           	      case 121:  ctl = C_resetallcontrollers; break;
                           	      case 123:  ctl = C_allnotesoff; break;
                           	      case 0x06: ctl = C_dataentryhi; break;
                           	      case 0x26: ctl = C_dataentrylo; break;
                           	      case 99:   ctl = C_nrpnhi; break;
                           	      case 98:   ctl = C_nrpnlo; break;
                           	      default:   ctl = C_NULL; break;
                           	      }
	      	            SetController(ch, ctl, e.dataB());
                              }
                              break;
                        }
                  break;

            case 0xf0:
                  putalldata((char*)e.data(), e.len());
                  break;
            }
      return false;
      }

//---------------------------------------------------------
//   instantiate
//---------------------------------------------------------

static Mess* instantiate(int sr, const char*)
      {
      if (instances == -1) {
            config.init();
	      instances = 0;
            srand(time(0));
            // SOUND_BUFFER_SIZE restricts midi resolution
            SOUND_BUFFER_SIZE = 64;
            OSCIL_SIZE        = 256;      // config.cfg.OscilSize;
            SAMPLE_RATE       = sr;
            denormalkillbuf = new REALTYPE [SOUND_BUFFER_SIZE];
            for (int i = 0; i < SOUND_BUFFER_SIZE; i++) 
                  denormalkillbuf[i] = (RND - 0.5) * 1e-16;
    
            OscilGen::tmpsmps = new REALTYPE[OSCIL_SIZE];
                  newFFTFREQS(&OscilGen::outoscilFFTfreqs,OSCIL_SIZE/2);
            }
      if (instances != 0) 
            return 0;   //don't allow multiple instances

      Zynadd* sintetizator = new Zynadd();
      sintetizator->setSampleRate(sr);
      return sintetizator;
      }

extern "C" {
      static MESS descriptor = {
            "Zynaddsubfx",
            "Zynaddsubfx Software Synthesizer",
            "0.1",      // version string
            MESS_MAJOR_VERSION, MESS_MINOR_VERSION,
            instantiate
            };

      // We must compile with -fvisibility=hidden to avoid namespace
      // conflicts with global variables.
      // Only visible symbol is "mess_descriptor".
      // (TODO: all plugins should be compiled this way)

      __attribute__ ((visibility("default")))
         const MESS* mess_descriptor() { return &descriptor; }
      }

