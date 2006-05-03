//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: organ.cpp,v 1.23 2005/12/16 15:36:51 wschweer Exp $
//
//  Parts of this file taken from:
//      Organ - Additive Organ Synthesizer Voice
//      Copyright (c) 1999, 2000 David A. Bartold
//
//  (C) Copyright 2001-2004 Werner Schweer (ws@seh.de)
//=========================================================

#include "muse/midi.h"
#include "libsynti/mpevent.h"

#include "organ.h"
#include "organgui.h"

SynthCtrl Organ::synthCtrl[] = {
      { "harm0",     HARM0,          0 },
      { "harm1",     HARM1,          0 },
      { "harm2",     HARM2,          0 },
      { "harm3",     HARM3,          0 },
      { "harm4",     HARM4,          0 },
      { "harm5",     HARM5,          0 },
      { "attackLo",  ATTACK_LO,     20 },
      { "decayLo",   DECAY_LO,      20 },
      { "sustainLo", SUSTAIN_LO,     0 },
      { "releaseLo", RELEASE_LO,    20 },
      { "attackHi",  ATTACK_HI,     10 },
      { "decayHi",   DECAY_HI,      10 },
      { "sustainHi", SUSTAIN_HI,     0 },
      { "releaseHi", RELEASE_HI,    10 },
      { "brass",     BRASS,          1 },
      { "flute",     FLUTE,          1 },
      { "reed",      REED,           1 },
      { "velocity",  VELO,           0 },
      // next controller not send as init data
      { "volume",    CTRL_VOLUME,  100 },
      };

static int NUM_CONTROLLER = sizeof(Organ::synthCtrl)/sizeof(*(Organ::synthCtrl));
static int NUM_INIT_CONTROLLER = NUM_CONTROLLER - 1;

float* Organ::sine_table;
float* Organ::g_triangle_table;
float* Organ::g_pulse_table;
int Organ::useCount = 0;
double Organ::cb2amp_tab[MAX_ATTENUATION];
unsigned Organ::freq256[128];

//---------------------------------------------------------
//   cb2amp
//    convert centibel to amplification (0 - 96dB)
//---------------------------------------------------------

double Organ::cb2amp(int cb)
      {
      if (cb < 0)
            return 1.0;
      if (cb >= MAX_ATTENUATION)
            return 0.0;
      return cb2amp_tab[cb];
      }

//---------------------------------------------------------
//   Organ
//---------------------------------------------------------

Organ::Organ(int sr)
   : Mess(1)
      {
      idata = new int[NUM_CONTROLLER];
      setSampleRate(sr);
      gui = 0;

      ++useCount;
      if (useCount > 1)
            return;

      // centibels to amplitude conversion
      for (int i = 0; i < MAX_ATTENUATION; i++)
            cb2amp_tab[i] = pow(10.0, double(i) / -200.0);

      for (int i = 0; i < 128; ++i) {
            double freq = 8.176 * exp(double(i)*log(2.0)/12.0);
            freq256[i]  = (int) (freq * ((double) RESOLUTION) / sr * 256.0);
            }
      int size  = RESOLUTION;
      int half  = size / 2;
      int slope = size / 10;
      int i;

      // Initialize sine table.
      sine_table = new float[size];
      for (i = 0; i < size; i++)
            sine_table[i] = sin ((i * 2.0 * M_PI) / size) / 6.0;

      // Initialize triangle table.
      g_triangle_table = new float[size];
      for (i = 0; i < half; i++)
            g_triangle_table[i] = (4.0 / size * i - 1.0) / 6.0;
      for (; i < size; i++)
            g_triangle_table[i] = (4.0 / size * (size - i) - 1.0) / 6.0;

      // Initialize pulse table.
      g_pulse_table = new float[size];
      for (i = 0; i < slope; i++)
            g_pulse_table[i] = (((double) -i) / slope) / 6.0;
      for (; i < half - slope; i++)
            g_pulse_table[i] = -1.0 / 6.0;
      for (; i < half + slope; i++)
            g_pulse_table[i] = (((double) i - half) / slope) / 6.0;
      for (; i < size - slope; i++)
            g_pulse_table[i] = 1.0 / 6.0;
      for (; i < size; i++)
            g_pulse_table[i] = (((double) size - i) / slope) / 6.0;
      }

//---------------------------------------------------------
//   ~Organ
//---------------------------------------------------------

Organ::~Organ()
      {
      if (gui)
            delete gui;
      delete[] idata;
      --useCount;
      if (useCount == 0) {
            delete[] g_pulse_table;
            delete[] g_triangle_table;
            delete[] sine_table;
            }
      }

//---------------------------------------------------------
//   table_pos
//---------------------------------------------------------

static inline float table_pos (float* table, unsigned long freq_256, unsigned *accum)
      {
      *accum += freq_256;
      while (*accum >= RESOLUTION * 256)
            *accum -= RESOLUTION * 256;
      return table[*accum >> 8];
      }

//---------------------------------------------------------
//   init
//---------------------------------------------------------

bool Organ::init(const char* name)
      {
      gui = new OrganGui;
      gui->setWindowTitle(QString(name));
      gui->show();

      for (int i = 0; i < NUM_CONTROLLER; ++i)
            setController(0, synthCtrl[i].num, synthCtrl[i].val);

      for (int i = 0; i < VOICES; ++i)
            voices[i].isOn = false;
      return false;
      }

//---------------------------------------------------------
//   write
//---------------------------------------------------------

void Organ::process(float** ports, int offset, int sampleCount)
      {
      //
      //  get and process all pending events from the
      //  synthesizer GUI
      //
      while (gui->fifoSize()) {
            MidiEvent ev = gui->readEvent();
            if (ev.type() == ME_CONTROLLER) {
                  // process local?
                  setController(ev.dataA(), ev.dataB());
                  sendEvent(ev);
                  }
            else
                  printf("Organ::process(): unknown event\n");
            }

      float* buffer = *ports + offset;
      for (int i = 0; i < VOICES; ++i) {
            Voice* v = &voices[i];
            if (!v->isOn)
                  continue;
            double vol = velo ? v->velocity : 1.0;
            vol *= volume;

            unsigned freq_256 = freq256[v->pitch];
            unsigned* harm0_accum = &(v->harm0_accum);
            unsigned* harm1_accum = &(v->harm1_accum);
            unsigned* harm2_accum = &(v->harm2_accum);
            unsigned* harm3_accum = &(v->harm3_accum);
            unsigned* harm4_accum = &(v->harm4_accum);
            unsigned* harm5_accum = &(v->harm5_accum);

            unsigned long freq_256_harm2, freq_256_harm3;
            unsigned long freq_256_harm4, freq_256_harm5;

            float* reed_table  = reed  ? g_pulse_table    : sine_table;
            float* flute_table = flute ? g_triangle_table : sine_table;

            unsigned freq_256_harm0 = freq_256 / 2;
            unsigned freq_256_harm1 = freq_256;

            if (brass) {
                  freq_256_harm2 = freq_256       * 2;
                  freq_256_harm3 = freq_256_harm2 * 2;
                  freq_256_harm4 = freq_256_harm3 * 2;
                  freq_256_harm5 = freq_256_harm4 * 2;
                  for (int i = 0; i < sampleCount; i++) {
                        int a1, a2;
                        switch(v->state1) {
                              case ATTACK:
                                    if (v->envL1.step(&a1))
                                          break;
                                    v->state1 = DECAY;
                              case DECAY:
                                    if (v->envL2.step(&a1))
                                          break;
                                    v->state1 = SUSTAIN;
                              case SUSTAIN:
                                    a1 = sustain0;
                                    break;
                              case RELEASE:
                                    if (v->envL3.step(&a1))
                                          break;
                                    v->state1 = OFF;
                                    a1 = MAX_ATTENUATION;
                                    break;
                              }
                        switch(v->state2) {
                              case ATTACK:
                                    if (v->envH1.step(&a2))
                                          break;
                                    v->state2 = DECAY;
                              case DECAY:
                                    if (v->envH2.step(&a2))
                                          break;
                                    v->state2 = SUSTAIN;
                              case SUSTAIN:
                                    a2 = sustain1;
                                    break;
                              case RELEASE:
                                    if (v->envH3.step(&a2))
                                          break;
                                    v->state2 = OFF;
                                    a1 = MAX_ATTENUATION;
                                    break;
                              }
                        if (v->state1 == OFF && v->state2 == OFF) {
                              v->isOn = false;
                              break;
                              }
                        buffer[i] +=
                            (table_pos (sine_table, freq_256_harm0, harm0_accum) * harm0
                           + table_pos (sine_table, freq_256_harm1, harm1_accum) * harm1
                           + table_pos (reed_table, freq_256_harm2, harm2_accum) * harm2)
                              * cb2amp(a1) * vol
                           + (table_pos (sine_table,  freq_256_harm3, harm3_accum) * harm3
                           +  table_pos (flute_table, freq_256_harm4, harm4_accum) * harm4
                           +  table_pos (flute_table, freq_256_harm5, harm5_accum) * harm5)
                             * cb2amp(a2) * vol;
                        }
                  }
            else {
                  freq_256_harm2 = freq_256 * 3 / 2;
                  freq_256_harm3 = freq_256 * 2;
                  freq_256_harm4 = freq_256 * 3;
                  freq_256_harm5 = freq_256_harm3 * 2;
                  for (int i = 0; i < sampleCount; i++) {
                        int a1, a2;
                        switch(v->state1) {
                              case ATTACK:
                                    if (v->envL1.step(&a1))
                                          break;
                                    v->state1 = DECAY;
                              case DECAY:
                                    if (v->envL2.step(&a1))
                                          break;
                                    v->state1 = SUSTAIN;
                              case SUSTAIN:
                                    a1 = sustain0;
                                    break;
                              case RELEASE:
                                    if (v->envL3.step(&a1))
                                          break;
                                    v->state1 = OFF;
                                    a1 = MAX_ATTENUATION;
                                    break;
                              }
                        switch(v->state2) {
                              case ATTACK:
                                    if (v->envH1.step(&a2))
                                          break;
                                    v->state2 = DECAY;
                              case DECAY:
                                    if (v->envH2.step(&a2))
                                          break;
                                    v->state2 = SUSTAIN;
                              case SUSTAIN:
                                    a2 = sustain1;
                                    break;
                              case RELEASE:
                                    if (v->envH3.step(&a2))
                                          break;
                                    v->state2 = OFF;
                                    a1 = MAX_ATTENUATION;
                                    break;
                              }
                        if (v->state1 == OFF && v->state2 == OFF) {
                              v->isOn = false;
                              break;
                              }
                        buffer[i] +=
                           (table_pos (sine_table, freq_256_harm0, harm0_accum) * harm0
                           + table_pos (sine_table, freq_256_harm1, harm1_accum) * harm1
                           + table_pos (sine_table, freq_256_harm2, harm2_accum) * harm2)
                              * cb2amp(a1) * vol
                           + (table_pos (reed_table, freq_256_harm3, harm3_accum) * harm3
                           + table_pos (sine_table, freq_256_harm4,  harm4_accum) * harm4
                           + table_pos (flute_table, freq_256_harm5, harm5_accum) * harm5)
                             * cb2amp(a2) * vol;
                        }
                  }
            }
      }

//---------------------------------------------------------
//   playNote
//---------------------------------------------------------

bool Organ::playNote(int channel, int pitch, int velo)
      {
      if (velo == 0) {
            noteoff(channel, pitch);
            return false;
            }
      for (int i = 0; i < VOICES; ++i) {
            if (voices[i].isOn)
                  continue;
            voices[i].isOn     = true;
            voices[i].pitch    = pitch;
            voices[i].channel  = channel;
            // velo is never 0
            voices[i].velocity = cb2amp(int(200 * log10((127.0 * 127)/(velo*velo))));
            voices[i].state1 = ATTACK;
            voices[i].state2 = ATTACK;
            voices[i].envL1.set(attack0,  MAX_ATTENUATION, 0);
            voices[i].envL2.set(decay0,   MAX_ATTENUATION, sustain0);
            voices[i].envL3.set(release0, sustain0, MAX_ATTENUATION);

            voices[i].envH1.set(attack1,  MAX_ATTENUATION, 0);
            voices[i].envH2.set(decay1,   MAX_ATTENUATION, sustain1);
            voices[i].envH3.set(release1, sustain1, MAX_ATTENUATION);

            voices[i].harm0_accum = 0;
            voices[i].harm1_accum = 0;
            voices[i].harm2_accum = 0;
            voices[i].harm3_accum = 0;
            voices[i].harm4_accum = 0;
            voices[i].harm5_accum = 0;
            return false;
            }
      printf("organ: voices overflow!\n");
      return false;
      }

//---------------------------------------------------------
//   noteoff
//---------------------------------------------------------

void Organ::noteoff(int channel, int pitch)
      {
      bool found = false;
      for (int i = 0; i < VOICES; ++i) {
            if (voices[i].isOn && (voices[i].pitch == pitch)
               && (voices[i].channel == channel)) {
                  found = true;
                  voices[i].state1 = RELEASE;
                  voices[i].state2 = RELEASE;
                  }
            }
      if (!found) {
            printf("Organ: noteoff %d:%d not found\n", channel, pitch);
            for (int i = 0; i < VOICES; ++i) {
                  if (voices[i].isOn)
                        printf("  %d\n", voices[i].pitch);
                  }
            }
      }

//---------------------------------------------------------
//   setController
//---------------------------------------------------------

void Organ::setController(int ctrl, int data)
      {
      int sr = sampleRate();
      switch (ctrl) {
            case HARM0:
                  harm0 = cb2amp(-data);
                  break;
            case HARM1:
                  harm1 = cb2amp(-data);
                  break;
            case HARM2:
                  harm2 = cb2amp(-data);
                  break;
            case HARM3:
                  harm3 = cb2amp(-data);
                  break;
            case HARM4:
                  harm4 = cb2amp(-data);
                  break;
            case HARM5:
                  harm5 = cb2amp(-data);
                  break;
            case ATTACK_LO:   // maxval -> 500msec
                  attack0 = (data * sr) / 1000;
                  break;
            case DECAY_LO:    // maxval -> 5000msec
                  decay0 = (data * sr) / 1000;
                  break;
            case SUSTAIN_LO:
                  sustain0 = -data;
                  break;
            case RELEASE_LO:
                  release0 = (data * sr) / 1000;
                  break;
            case ATTACK_HI:
                  attack1 = (data * sr) / 1000;
                  break;
            case DECAY_HI:
                  decay1 =  (data * sr) / 1000;
                  break;
            case SUSTAIN_HI:
                  sustain1 = -data;
                  break;
            case RELEASE_HI:
                  release1 = (data * sr) / 1000;
                  break;
            case BRASS:
                  brass = data;
                  break;
            case FLUTE:
                  flute = data;
                  break;
            case REED:
                  reed  = data;
                  break;
            case VELO:
                  velo = data;
                  break;
            case CTRL_VOLUME:
                  data &= 0x7f;
                  volume = data == 0 ? 0.0 : cb2amp(int(200 * log10((127.0 * 127)/(data*data))));
                  break;
            case CTRL_ALL_SOUNDS_OFF:
                  for (int i = 0; i < VOICES; ++i)
                        voices[i].isOn = false;
                  break;
            case CTRL_RESET_ALL_CTRL:
                  for (int i = 0; i < NUM_CONTROLLER; ++i)
                        setController(0, synthCtrl[i].num, synthCtrl[i].val);
                  break;
            default:
//                  fprintf(stderr, "Organ:set unknown Ctrl 0x%x to 0x%x\n", ctrl, data);
                  return;
            }
      for (int i = 0; i < NUM_CONTROLLER; ++i) {
            if (synthCtrl[i].num == ctrl) {
                  synthCtrl[i].val = data;
                  break;
                  }
            }
      }

//---------------------------------------------------------
//   setController
//---------------------------------------------------------

bool Organ::setController(int channel, int ctrl, int data)
      {
      setController(ctrl, data);

      switch (ctrl) {
            case HARM0:
            case HARM1:
            case HARM2:
            case HARM3:
            case HARM4:
            case HARM5:
            case ATTACK_LO:
            case DECAY_LO:
            case SUSTAIN_LO:
            case RELEASE_LO:
            case ATTACK_HI:
            case DECAY_HI:
            case SUSTAIN_HI:
            case RELEASE_HI:
            case BRASS:
            case FLUTE:
            case REED:
            case VELO:
                  {
                  MidiEvent ev(0, channel, ME_CONTROLLER, ctrl, data);
                  gui->writeEvent(ev);
                  }
                  break;
            default:
                  break;
            }
      return false;
      }

//---------------------------------------------------------
//   sysex
//---------------------------------------------------------

bool Organ::sysex(int n, const unsigned char* data)
      {
      if (unsigned(n) != (NUM_INIT_CONTROLLER * sizeof(int))) {
            printf("Organ: unknown sysex\n");
            return false;
            }
      int* s = (int*) data;
      for (int i = 0; i < NUM_INIT_CONTROLLER; ++i) {
            int val = *s++;
            setController(0, synthCtrl[i].num, val);
            }
      return false;
      }

//---------------------------------------------------------
//   getInitData
//---------------------------------------------------------

void Organ::getInitData(int* n, const unsigned char**p)
      {
      int* d = idata;
      for (int i = 0; i < NUM_INIT_CONTROLLER; ++i)
            *d++ = synthCtrl[i].val;
      *n = NUM_INIT_CONTROLLER * sizeof(int); // sizeof(idata);
      *p = (unsigned char*)idata;
      }

//---------------------------------------------------------
//   MESS
//---------------------------------------------------------

//---------------------------------------------------------
//   getControllerInfo
//---------------------------------------------------------

int Organ::getControllerInfo(int id, const char** name, int* controller,
   int* min, int* max) const
      {
      if (id >= NUM_CONTROLLER)
            return 0;
      *controller = synthCtrl[id].num;
      *name       = synthCtrl[id].name;
      *min        = 0;
      *max        = 128*128-1;
      return ++id;
      }

//---------------------------------------------------------
//   guiVisible
//---------------------------------------------------------

bool Organ::guiVisible() const
      {
      return gui->isVisible();
      }

//---------------------------------------------------------
//   showGui
//---------------------------------------------------------

void Organ::showGui(bool val)
      {
      gui->setShown(val);
      }

//---------------------------------------------------------
//   getGeometry
//---------------------------------------------------------

void Organ::getGeometry(int* x, int* y, int* w, int* h) const
      {
      QPoint pos(gui->pos());
      QSize size(gui->size());
      *x = pos.x();
      *y = pos.y();
      *w = size.width();
      *h = size.height();
      }

//---------------------------------------------------------
//   setGeometry
//---------------------------------------------------------

void Organ::setGeometry(int x, int y, int w, int h)
      {
      gui->resize(QSize(w, h));
      gui->move(QPoint(x, y));
      }

//---------------------------------------------------------
//   instantiate
//    construct a new synthesizer instance
//---------------------------------------------------------

static Mess* instantiate(int sr, QWidget*, const char* name)
      {
      Organ* synth = new Organ(sr);
      if (synth->init(name)) {
            delete synth;
            synth = 0;
            }
      return synth;
      }

//---------------------------------------------------------
//   msynth_descriptor
//    Return a descriptor of the requested plugin type.
//---------------------------------------------------------

extern "C" {
      static MESS descriptor = {
            "Organ",
            "Organ; based on David A. Bartold's LADSPA plugin",
            "0.1",      // version string
            MESS_MAJOR_VERSION, MESS_MINOR_VERSION,
            instantiate,
            };

      const MESS* mess_descriptor() { return &descriptor; }
      }

