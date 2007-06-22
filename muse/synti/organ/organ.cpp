//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: organ.cpp,v 1.23 2005/12/16 15:36:51 wschweer Exp $
//
//  Parts of this file taken from:
//      Organ - Additive Organ Synthesizer Voice
//      Copyright (C) 1999, 2000 David A. Bartold
//  Some information was gathered form the "beatrix" organ
//      from Fredrik Kilander
//
//  (C) Copyright 2001-2007 Werner Schweer (ws@seh.de)
//=========================================================

#include "muse/midi.h"
#include "libsynti/midievent.h"

#include "organ.h"
#include "organgui.h"
#include "reverb.h"

float* Organ::attackEnv;
float* Organ::releaseEnv;
int    Organ::envSize;
float* Organ::waveTable;
int    Organ::useCount;
double Organ::cb2amp_tab[MAX_ATTENUATION];
float  Organ::keyCompression[NO_VOICES];

//---------------------------------------------------------
//   dBToGain
//---------------------------------------------------------

static double dBToGain(double dB)
      {
      return pow(10.0, (dB / 20.0));
      }

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

static const unsigned SHIFT      = 16;
static const double   RESO       = 256.0 * 256.0 * 256.0 * 256.0;
static const unsigned resolution = 256 * 256;   // 16 Bit

//---------------------------------------------------------
//   Organ
//---------------------------------------------------------

Organ::Organ(int sr)
   : Mess2(2)
      {
      setSampleRate(sr);
      gui    = 0;
      reverb = new Reverb();

      ++useCount;
      if (useCount > 1)
            return;

      // centibels to amplitude conversion
      for (int i = 0; i < MAX_ATTENUATION; i++)
            cb2amp_tab[i] = pow(10.0, double(i) / -200.0);

      // Initialize sine table.
      waveTable = new float[resolution];
      for (unsigned i = 0; i < resolution; i++)
            waveTable[i] = sin (double(i) * 2.0 * M_PI / double(resolution));

      // Initialize envelope tables

      envSize    = sr * 4 / 1000;     // 4 msec
      attackEnv  = new float[envSize];
      releaseEnv = new float[envSize];

      for (int i = 0; i < envSize; ++i) {
            attackEnv[i]  = float(i) / float(envSize);
            releaseEnv[i] = float(i) / float(envSize);
            }

      // Initialize key compression table

      keyCompression[ 0] = 1.0;
      keyCompression[ 1] = 1.0;
      keyCompression[ 2] = dBToGain(-1.1598);
      keyCompression[ 3] = dBToGain(-2.0291);
      keyCompression[ 4] = dBToGain(-2.4987);
      keyCompression[ 5] = dBToGain(-2.9952);
      keyCompression[ 6] = dBToGain(-3.5218);
      keyCompression[ 7] = dBToGain(-4.0823);
      keyCompression[ 8] = dBToGain(-4.6815);
      keyCompression[ 9] = dBToGain(-4.9975);
      keyCompression[10] = dBToGain(-4.9998);

      /* Linear interpolation from u to v. */

      static const float u = -5.0;
      static const float v = -9.0;
      static const float m = 1.0 / (NO_VOICES - 12);
      for (int i = 11; i < NO_VOICES; i++) {
            keyCompression[i] = dBToGain(u + ((v - u) * float(i - 11) * m));
            }

      // Initialize controller table

      addController("drawbar16",      DRAWBAR0,         0, 8,   8);
      addController("drawbar513",     DRAWBAR1,         0, 8,   8);
      addController("drawbar8",       DRAWBAR2,         0, 8,   8);
      addController("drawbar4",       DRAWBAR3,         0, 8,   0);
      addController("drawbar223",     DRAWBAR4,         0, 8,   0);
      addController("drawbar2",       DRAWBAR5,         0, 8,   0);
      addController("drawbar135",     DRAWBAR6,         0, 8,   0);
      addController("drawbar113",     DRAWBAR7,         0, 8,   0);
      addController("drawbar1",       DRAWBAR8,         0, 8,   0);
      addController("reverbOn",       REVERB_ON,        0, 1,   0);
      addController("reverbRoomSize", REVERB_ROOM_SIZE, 0, 127, 60);
      addController("reverbMix",      REVERB_MIX,       0, 127, 100);
      addController("vibratoOn",      VIBRATO_ON,       0, 1,   1);
      addController("vibratoFreq",    VIBRATO_FREQ,     0, 127, 100);
      addController("vibratoDepth",   VIBRATO_DEPTH,    0, 127, 50);
      addController("volume",         CTRL_VOLUME,      0, 127, 100);
      addController("percOn",         PERC_ON,          0, 1,   1);
      addController("percGain",       PERC_GAIN,        0, 127, 60);
      addController("percDecay",      PERC_DECAY,       0, 127, 60);
      addController("percHarmony",    PERC_HARMONY,     0, 8,   4);
      addController("rotaryOn",       ROTARY_ON,        0, 1,   0);
      addController("rot1Freq",       ROT1_FREQ,        0, 127, 100);
      addController("rot1Depth",      ROT1_DEPTH,       0, 127, 50);
      addController("rot2Freq",       ROT2_FREQ,        0, 127, 100);
      addController("rot2Depth",      ROT2_DEPTH,       0, 127, 50);
      }

//---------------------------------------------------------
//   ~Organ
//---------------------------------------------------------

Organ::~Organ()
      {
      if (gui)
            delete gui;
      delete reverb;

      --useCount;
      if (useCount == 0) {
            delete[] waveTable;
            delete[] attackEnv;
            delete[] releaseEnv;
            }
      }

//---------------------------------------------------------
//   init
//---------------------------------------------------------

bool Organ::init(const char* name)
      {
      gui = new OrganGui;
      gui->hide();
      gui->setWindowTitle(QString(name));

      //
      // Initialize controller
      //
      int idx = 0;
      foreach(SynthCtrl* c, ctrl) {
            setController(c->ctrl, c->init);    // init synti
            gui->setParamIdx(idx, c->init);     // init gui
            ++idx;
            }

      // see: http://www.dairiki.org/HammondWiki/GearRatio
      static const int gearA[12] = { 85, 71,67,35,69,12,37,49,48,11,67,54 };
      static const int gearB[12] = { 104,82,73,36,67,11,32,40,37, 8,46,35 };
      static const int teeth[]   = { 2, 4, 8, 16, 32, 64, 128, 192 };

      vibratoAccu = 0;
      rot1AccuL   = 0;
      rot1AccuR   = 0x80000000;
      rot2AccuL   = 0;
      rot2AccuR   = 0x80000000;

      for (int i = 0; i < NO_WHEELS; ++i) {
            int note     = i % 12;
            int octave   = i / 12;
            if (octave == 7)
                  note += 5;
            // in 60Hz organs, the motor turns at 1200 RPM (20 revolutions /sec)
            double freq         = 20.0 * teeth[octave] * gearA[note] / gearB[note];
            wheels[i].frameStep = lrint(freq * RESO / double(sampleRate()));
            wheels[i].accu      = 0;
            wheels[i].refCount  = 0;
            wheels[i].active    = false;
            for (int k = 0; k < NO_BUSES; ++k)
                  wheels[i].envCount[k] = 0;
            }
      keyCompressionValue = 1.0;
      keyCompressionCount = 0;
      percGain            = 0.0;
      return false;
      }

//---------------------------------------------------------
//   process
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
                  setController(ev.dataA(), ev.dataB());
                  sendEvent(ev);
                  }
            else
                  printf("Organ::process(): unknown event\n");
            }

      float* buffer1 = ports[0] + offset;
      float* buffer2 = ports[1] + offset;
      memset(buffer1, 0, sizeof(float) * sampleCount);
      memset(buffer2, 0, sizeof(float) * sampleCount);

      float vibrato[sampleCount];

      if (vibratoOn) {
            //
            // compute partial vibrato sinus
            //
            for (int i = 0; i < sampleCount; ++i) {
                  vibratoAccu += vibratoStep;
                  vibrato[i]  = waveTable[vibratoAccu >> SHIFT] * vibratoDepth;
                  }
            }

      foreach (Wheel* w, activeWheels) {
            for (int i = 0; i < sampleCount; ++i) {

                  unsigned step = w->frameStep;
                  if (vibratoOn)
                        step += unsigned(step * vibrato[i]);

                  w->accu  += step;

                  int idx    = w->accu >> SHIFT;
                  float val1 = waveTable[idx];
                  idx = (idx + 1) & 0xffff;
                  float val2 = waveTable[idx];
                  float val  = val1 + (val2 - val1) * double(w->accu & 0xffff)/double(0x10000);

                  for (int k = 0; k < NO_BUSES; ++k) {
                        int* envCnt = &(w->envCount[k]);
                        float v;
                        if (*envCnt > 0) {
                              (*envCnt)--;
                              float gain = w->gain[k] - w->deltaGain[k] * w->env[k][*envCnt];
                              v          = val * gain;
                              if ((*envCnt == 0) && (w->refCount == 0)) {
                                    int idx = activeWheels.indexOf(w);
                                    if (idx != -1) {
                                          activeWheels.removeAt(idx);
                                          w->active = false;
                                          }
                                    }
                              }
                        else {
                              v = val * w->gain[k];
                              }
                        buffer1[i] += v * drawBarGain[k];
                        if (k == percussionBus)
                              buffer2[i] += v;
                        }
                  }
            }
      if (percussionOn) {
            for (int i = 0; i < sampleCount; ++i) {
                  buffer1[i] = buffer1[i] * volume * keyCompressionValue
                              + buffer2[i] * percGain;
                  percGain *= percussionEnvDecay;
                  if (keyCompressionCount) {
                        keyCompressionValue += keyCompressionDelta;
                        --keyCompressionCount;
                        }
                  }
            }
      else {
            for (int i = 0; i < sampleCount; ++i) {
                  buffer1[i] *= volume * keyCompressionValue;
                  if (keyCompressionCount) {
                        keyCompressionValue += keyCompressionDelta;
                        --keyCompressionCount;
                        }
                  }
            }
      memcpy(buffer2, buffer1, sizeof(float) * sampleCount);
      if (reverbOn)
            reverb->process(buffer1, buffer2, sampleCount);
      }

//---------------------------------------------------------
//   changeKeyCompression
//---------------------------------------------------------

void Organ::changeKeyCompression()
      {
      float kc            = keyCompression[pressedKeys.size()];
      keyCompressionCount = int(sampleRate() * .005);      // 5 msec envelope
      if (keyCompressionCount < 2)
            keyCompressionCount = 2;
      keyCompressionDelta = (kc - keyCompressionValue) / keyCompressionCount;
      }

//---------------------------------------------------------
//   playNote
//---------------------------------------------------------

bool Organ::playNote(int /*channel*/, int pitch, int velo)
      {
      if (pitch < 36 || pitch > 97)
            return false;
      if (velo == 0) {
            int idx = pressedKeys.indexOf(pitch);
            if (idx == -1) {
                  printf("Organ: noteoff %d not found\n", pitch);
                  return false;
                  }
            pressedKeys.removeAt(idx);
            }
      else {
            if (pressedKeys.isEmpty())
                  percGain = percGainInit;
            pressedKeys.append(pitch);
            }
      changeKeyCompression();

      for (int k = 0; k < NO_ELEMENTS; ++k) {
            const Elem* e = &routing[pitch - 36][k];
            if (e->bus == -1)
                  break;
            Wheel* w    = &wheels[int(e->wheel)];
            int bus     = e->bus;
            float level = e->level;

            if (velo) {
                  if (!w->active) {
                        // activate wheel
                        for (int k = 0; k < NO_BUSES; ++k) {
                              w->gain[k]     = 0.0;
                              w->envCount[k] = 0;
                              }
                        activeWheels.append(w);
                        w->active = true;
                        }
                  float deltaGain = level;

                  if (w->envCount[bus]) {
                        deltaGain += w->deltaGain[bus] * w->env[bus][w->envCount[bus]];
                        }
                  w->env[bus]       = attackEnv;
                  w->deltaGain[bus] = deltaGain;
                  w->gain[bus]      += level;
                  w->refCount++;
                  }
            else {
                  float deltaGain = -level;

                  if (w->envCount[bus]) {
                        deltaGain += w->deltaGain[bus] * w->env[bus][w->envCount[bus]];
                        }
                  w->env[bus]       = releaseEnv;
                  w->deltaGain[bus] = deltaGain;
                  w->gain[bus]     -= level;
                  if (w->refCount)
                        w->refCount--;
                  }
            w->envCount[bus] = envSize;
            }
      return false;
      }

//---------------------------------------------------------
//   percussionChanged
//---------------------------------------------------------

void Organ::percussionChanged()
      {
      percussionEnvDecay = exp(log(0.001/percGainInit) / (percDecay * double(sampleRate())));
      }

//---------------------------------------------------------
//   setController
//---------------------------------------------------------

void Organ::setController(int ctrlId, int data)
      {
      int ctrlIdx = controllerIdx(ctrlId);
      if (ctrlIdx != -1)
            ctrl[ctrlIdx]->val = data;
      switch (ctrlId) {
            case DRAWBAR0 ... DRAWBAR8:
                  {
                  int db = ctrlId - DRAWBAR0;
                  drawBarGain[db] = float(data) / 8.0;
                  }
                  break;
            case REVERB_ROOM_SIZE:
                  reverb->setRoomSize(float(data) / 127.0);
                  break;

            case REVERB_MIX:
                  reverb->setMix(float(data) / 127.0);
                  break;

            case REVERB_ON:
                  reverbOn = data != 0;
                  break;

            case VIBRATO_ON:
                  vibratoOn = data != 0;
                  break;

            case VIBRATO_FREQ:
                  vibratoFreq = float(data) * 6.0 / 127.0 + 4;
                  vibratoStep = lrint(vibratoFreq * RESO / double(sampleRate()));
                  break;

            case VIBRATO_DEPTH:
                  vibratoDepth = float(data) / 127.0 * .01;
                  break;

            case PERC_ON:
                  percussionOn = data != 0;
                  break;

            case PERC_GAIN:         //   0.01 - 0.4
                  percGainInit = float(data) * .39 / 127.0 + 0.01;
                  percussionChanged();
                  break;

            case PERC_DECAY:        // 0.5 - 4.5 sec
                  percDecay = float(data) * 4.0 / 127.0 + 0.5;
                  percussionChanged();
                  break;

            case PERC_HARMONY:
                  percussionBus = data;
                  break;

            case ROTARY_ON:
                  rotaryOn = data != 0;
                  break;

            case ROT1_FREQ:
                  rot1Freq = float(data) * 6.0 / 127.0 + 0.67;
                  rot1Step = lrint(rot1Freq * RESO / double(sampleRate()));
                  break;

            case ROT1_DEPTH:
                  rot1Depth = float(data) / 127.0 * 1.0;
                  break;

            case ROT2_FREQ:
                  rot1Freq = float(data) * 5.0 / 127.0 + 0.5;
                  rot1Step = lrint(rot1Freq * RESO / double(sampleRate()));
                  break;

            case ROT2_DEPTH:
                  rot2Depth = float(data) / 127.0 * 1.0;
                  break;

            case CTRL_VOLUME:
                  data &= 0x7f;
                  volume = data == 0 ? 0.0 : cb2amp(int(200 * log10((127.0 * 127)/(data*data))));
                  volume *= .04;
                  break;

            case CTRL_ALL_SOUNDS_OFF:
                  foreach(Wheel* w, activeWheels) {
                        for (int k = 0; k < NO_ELEMENTS; ++k) {
                              w->gain[k] = 0.0;
                              w->envCount[k] = 0;
                              }
                        w->refCount       = 0;
                        }
                  pressedKeys.clear();
                  break;

            case CTRL_RESET_ALL_CTRL:
//                  for (int i = 0; i < NUM_CONTROLLER; ++i)
//                        setController(0, synthCtrl[i].num, synthCtrl[i].val);
                  break;
            default:
                  fprintf(stderr, "Organ:set unknown Ctrl 0x%x to 0x%x\n", ctrlId, data);
                  return;
            }
      }

//---------------------------------------------------------
//   setController
//---------------------------------------------------------

bool Organ::setController(int channel, int ctrlId, int data)
      {
      MidiEvent ev(0, channel, ME_CONTROLLER, ctrlId, data);
      gui->writeEvent(ev);
      setController(ctrlId, data);
      return false;
      }

//---------------------------------------------------------
//   sysex
//---------------------------------------------------------

bool Organ::sysex(int n, const unsigned char* data)
      {
      int nn = ctrl.size() * sizeof(int);
      if (nn != n) {
            printf("unknown sysex %d %02x %02x\n", n, data[0], data[1]);
            return false;
            }
      const int* s = (const int*) data;
      for (int i = 0; i < ctrl.size(); ++i) {
            setController(0, ctrl[i]->ctrl, *s);
            setController(ctrl[i]->ctrl, *s);
            s++;
            }
      return false;
      }

//---------------------------------------------------------
//   MESS
//---------------------------------------------------------

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

static Mess* instantiate(int sr, const char* name)
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

