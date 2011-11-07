//=========================================================
//  MusE
//  Linux Music Editor
//
//  Parts of this file taken from:
//      The analogue oscillator from Steve Harris plugin collection.
//      Werner Schweer's organ softsynth for MusE.
//	The music-dsp source archive.
//
//  (C) Copyright 2002 Jotsif Lindman Hï¿½nlund (jotsif@linux.nu)
//  (C) Copyright 2005 Robert Jonsson (rj@spamatica.se)
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
// 02111-1301, USA or point your web browser to http://www.gnu.org.
//=========================================================

#include <cmath>
#include <stdio.h>
#include <list>

#include "libsynti/mess.h"
#include "muse/midi.h"
#include "muse/midictrl.h"

#include "common_defs.h"
#include "vam.h"
#include "vamgui.h"
#include "libsynti/mono.h"

// Denormalise floats, only actually needed for PIII and very recent PowerPC
#define DENORMALISE(fv) (((*(unsigned int*)&(fv))&0x7f800000)==0)?0.0f:(fv)

// A fast, truncating towards 0 modulo function. ANSI C doesn't define
// which % will do, most truncate towards -inf
#define MOD(v,m) (v<0?v+m:(v>m?v-m:v))

// Limit values
#define LIMIT(v,l,u) (v<l?l:(v>u?u:v))

#define PI M_PI

//---------------------------------------------------------
//   Oscillator
//---------------------------------------------------------

struct Oscillator {
float phase;
float pitchmod;
float detune;
float freq;
float pwm;
float pw;
float fm;
int waveform;
bool on;
};

struct LPFilter {
float out[4];
float in[4];
};

//---------------------------------------------------------
//   Envelope
//---------------------------------------------------------

struct EnvelopeGenerator {
static const int onStates = 2;
static const int offStates = 1;

struct Segment {
		int ticks;
		double incr;
};
Segment segment[onStates + offStates];

int state;
double env;
int tick;

int attack;
int decay;
float sustain;
int release;

EnvelopeGenerator() {
		segment[0].ticks = 441;
		segment[0].incr = 1.0/441.0;
		segment[1].ticks = 0;
		segment[1].incr = 0.0;
		segment[2].ticks = 441;
		segment[2].incr = -(1.0/441.0);
}

void setSegment(int seg, int ticks, double incr) {
		segment[seg].ticks = ticks;
		segment[seg].incr = incr;
}
  
void keyOn() {
//		env = 0.0;
		state = 0;
		if(env) segment[state].incr = (1.0 - env) / segment[state].ticks;
		else env = 0.0;
		tick = segment[state].ticks;
}
void keyOff() {
		state = onStates;
		tick = segment[state].ticks;
}
bool isOff() {
		return state == (onStates+offStates);
}
bool step() {
		if(state >= onStates+offStates)
		return false;
		if (tick == 0)
		return true;
		env +=segment[state].incr;
		if(env < 0.0)
		    env = 0.0;
		--tick;
		while(tick == 0) {
		++state;
		if(state >= onStates+offStates)
				return false;
		if(state == onStates)
				return true;
		tick = segment[state].ticks;
		}
		return true;
}
};

//---------------------------------------------------------
//   VAM
//---------------------------------------------------------

class VAM : public MessMono {
      static int useCount;
      static const int CB_AMP_SIZE = 961;
      static const int LIN2EXP_SIZE = 256;

      static double cb2amp_tab[CB_AMP_SIZE];
      static double cb2amp(double cb);

      static float lin2exp[LIN2EXP_SIZE];

            /*	Synthvariables */
      static float *sin_tbl, *tri_tbl, *saw_tbl, *squ_tbl;
      bool isOn;
      int pitch, channel;
      float velocity;

      //int idata[NUM_CONTROLLER];  // buffer for init data
      //int *idata;
      unsigned char* idata;

      EnvelopeGenerator dco1_env;
      EnvelopeGenerator dco2_env;
      EnvelopeGenerator filt_env;

      LPFilter dco1_filter;
      LPFilter dco2_filter;

      Oscillator dco1;
      Oscillator dco2;
      Oscillator lfo;

      bool filt_invert, filt_keytrack;
      double filt_env_mod, filt_res, filt_cutoff, keytrack_cutoff;

      int controller[NUM_CONTROLLER];
      void noteoff(int channel, int pitch);
      void setController(int ctrl, int data);
      float *wave_tbl(int wave);
      double lowpass_filter(double cutoff, double resonance, double input, LPFilter *f);


      VAMGui* gui;

    public:
      virtual int getControllerInfo(int, const char**, int*, int*, int*, int*) const;
      //virtual void getInitData(int* n, const unsigned char**p) const;
      virtual void getInitData(int* n, const unsigned char**p);
      // This is only a kludge required to support old songs' midistates. Do not use in any new synth.
      virtual int oldMidiStateHeader(const unsigned char** data) const;
      //virtual bool guiVisible() const;
      //virtual void showGui(bool);
      //virtual bool hasGui() const { return true; }
      virtual bool nativeGuiVisible() const;
      virtual void showNativeGui(bool);
      virtual bool hasNativeGui() const { return true; }
      virtual void getNativeGeometry(int* x, int* y, int* w, int* h) const;
      virtual void setNativeGeometry(int x, int y, int w, int h);
      virtual void processMessages();
      virtual void process(float**, int, int);
      virtual void note(int channel, int pitch, int velo);
      virtual bool setController(int channel, int ctrl, int val);
      virtual bool sysex(int, const unsigned char*);
      VAM(int sr);
      virtual ~VAM();
      bool init(const char* name);
};

float* VAM::sin_tbl;
float* VAM::tri_tbl;
float* VAM::saw_tbl;
float* VAM::squ_tbl;
int VAM::useCount = 0;
double VAM::cb2amp_tab[VAM::CB_AMP_SIZE];
float VAM::lin2exp[VAM::LIN2EXP_SIZE];


//---------------------------------------------------------
//   VAM
//---------------------------------------------------------

VAM::VAM(int sr)
  : MessMono()
      {
      idata = new unsigned char[3 + NUM_CONTROLLER * sizeof(int)];   
      setSampleRate(sr);
      gui = 0;
      }

//---------------------------------------------------------
//   ~VAM
//---------------------------------------------------------

VAM::~VAM()
      {
      if (gui)
            delete gui;
      //delete idata;   
      delete [] idata;   // p4.0.27
      --useCount;
      if (useCount == 0) {
          delete[] sin_tbl;
          delete[] tri_tbl;
          delete[] saw_tbl;
          delete[] squ_tbl;
          }
      }

int VAM::oldMidiStateHeader(const unsigned char** data) const 
{
  static unsigned char const d[3] = {MUSE_SYNTH_SYSEX_MFG_ID, VAM_UNIQUE_ID, INIT_DATA_CMD};
  *data = &d[0];
  return 3; 
}
        
//---------------------------------------------------------
//   curTime
//---------------------------------------------------------

double VAM::cb2amp(double cb)
      {
      if(cb < 0.0)
          return 1.0;
      if(cb > 960.0)
          return 0.0;
      return cb2amp_tab[int(cb)];
      }

double VAM::lowpass_filter(double cutoff, double resonance, double input, LPFilter *f)
      {
      double output;
      cutoff *= 1.16;

      input -= f->out[3] * (resonance * 4.0) * (1.0 - 0.15 * cutoff * cutoff);
      input *= 0.35013 * cutoff * cutoff * cutoff * cutoff;

      f->out[0] = input + 0.3 * f->in[0] + (1.0 - cutoff) * f->out[0]; // Pole 1
      f->in[0]  = input;
      f->out[1] = f->out[0] + 0.3 * f->in[1] + (1.0 - cutoff) * f->out[1];  // Pole 2
      f->in[1]  = f->out[0];
      f->out[2] = f->out[1] + 0.3 * f->in[2] + (1.0 - cutoff) * f->out[2];  // Pole 3
      f->in[2]  = f->out[1];
      f->out[3] = f->out[2] + 0.3 * f->in[3] + (1.0 - cutoff) * f->out[3];  // Pole 4
      f->in[3]  = f->out[2];

      //	if(f.out[3] > 1.0) f.out[3] = 1.0;

      output = f->out[3];


      return output;
      }

float *VAM::wave_tbl(int wave)
      {
              if (wave == 0) {
                      return sin_tbl;
              }
      else if (wave == 1) {
                      return squ_tbl;
      }
      else if (wave == 2) {
          return saw_tbl;
      }
      else if (wave == 3) {
          return tri_tbl;
      }
      return sin_tbl;
      }

//---------------------------------------------------------
//   init
//---------------------------------------------------------

bool VAM::init(const char* name)
      {
      gui = new VAMGui;
      gui->setWindowTitle(QString(name));
      gui->show();

      if (useCount == 0) {
          int i;
          float tmp;
          for(i = 0; i < CB_AMP_SIZE; i++) {
              cb2amp_tab[i] = pow(10.0, double(i) / -300.0);
              //cb2amp_tab[i] = 1.0 - i/(float)CB_AMP_SIZE;
              }
          for(i = 0; i < LIN2EXP_SIZE; i++) {
              tmp = i/255.0;
              lin2exp[i] = 1.5 * tmp * tmp * tmp - 0.69 * tmp * tmp + 0.16 * tmp;
              }
          int sr = sampleRate();
          /* Build up denormalised oscilator wavetables, these are sample_rate
             long, costs more RAM to create them but makes freqency calcs much
             cheaper, and means that interpolation isn't that neccesary, esp if
             you use integer frequncies */
          
          float *tmp_tbl = new float[sr];
          const int lag = sr/50;
          sin_tbl = new float[sr];
          for (i = 0; i < sr; i++) {
                tmp = sin(i * 2.0 * PI / sr);
                sin_tbl[i] = DENORMALISE(tmp);
                }
          tri_tbl = new float[sr];
          for (i = 0; i < sr; i++) {
                tmp = acos(cos(i * 2.0 * PI / sr)) / PI * 2.0 - 1.0;
                tri_tbl[i] = DENORMALISE(tmp);
                }
          squ_tbl = new float[sr];
          for (i = 0; i < sr/2; i++) {
                tmp_tbl[i] = -1.0f;
                }
          for (i = sr/2; i < sr; i++) {
                tmp_tbl[i] = +1.0f;
                }
          tmp = -1.0f;
          for (i = (sr/2)-lag; i < (sr/2)+lag; i++) {
                tmp_tbl[i] = tmp;
                tmp += 1.0/(lag * 2.0);
                }
          for (i = 0; i < sr; i++) {
                squ_tbl[i] = (tmp_tbl[MOD(i-lag, sr)] +
                tmp_tbl[MOD(i+lag, sr)]) * 0.5;
                }
          saw_tbl = new float[sr];
          for (i = 0; i < sr; i++) {
                tmp = ((2.0 * i) - (float)sr) / (float)sr;
                tmp_tbl[i] = DENORMALISE(tmp);
                }
          for (i = 0; i < sr; i++) {
                saw_tbl[i] = (tmp_tbl[MOD(i-lag, sr)] +
                tmp_tbl[MOD(i+lag, sr)]) * 0.5;
                }
          delete[] tmp_tbl;
          }
      
      dco1_filter.out[0] = dco1_filter.out[1] = dco1_filter.out[2] = dco1_filter.out[3] = 0.0;
      dco1_filter.in[0]  = dco1_filter.in[1] = dco1_filter.in[2] = dco1_filter.in[3] = 0.0;
      dco2_filter.out[0] = dco2_filter.out[1] = dco2_filter.out[2] = dco2_filter.out[3] = 0.0;
      dco2_filter.in[0]  = dco2_filter.in[1] = dco2_filter.in[2] = dco2_filter.in[3] = 0.0;
      
      ++useCount;
      dco1.phase = 0.0;
      dco2.phase = 0.0;
      lfo.phase = 0.0;
      
      memset(controller, 0, sizeof(controller));
      
      int maxval = 128*128-1;
      
      setController(0, DCO1_PITCHMOD, 8191);
      setController(0, DCO2_PITCHMOD, 8191);
      setController(0, DCO1_WAVEFORM, 1);
      setController(0, DCO2_WAVEFORM, 1);
      setController(0, DCO1_FM, 0);
      setController(0, DCO2_FM, 0);
      setController(0, DCO1_PWM, 0);
      setController(0, DCO2_PWM, 0);
      setController(0, DCO1_ATTACK, 0);
      setController(0, DCO2_ATTACK, 0);
      setController(0, DCO1_DECAY, 0);
      setController(0, DCO2_DECAY, 0);
      setController(0, DCO1_SUSTAIN, maxval - 255);
      setController(0, DCO2_SUSTAIN, maxval - 255);
      setController(0, DCO1_RELEASE, 0);
      setController(0, DCO2_RELEASE, 0);
      setController(0, LFO_FREQ, 0);
      setController(0, LFO_WAVEFORM, 0);
      setController(0, FILT_ENV_MOD, 0);
      setController(0, FILT_KEYTRACK, 0);
      setController(0, FILT_RES, 0);
      setController(0, FILT_ATTACK, 0);
      setController(0, FILT_DECAY, 0);
      setController(0, FILT_SUSTAIN, maxval);
      setController(0, FILT_RELEASE, 3);
      setController(0, DCO2ON, 0);
      setController(0, FILT_INVERT, 0);
      setController(0, FILT_CUTOFF, 15000);
      setController(0, DCO1_DETUNE, 8191);
      setController(0, DCO2_DETUNE, 8191);
      setController(0, DCO1_PW, 0);
      setController(0, DCO2_PW, 0);
      
      isOn = false;
      return false;
      }

//---------------------------------------------------------
//   processMessages
//   Called from host always, even if output path is unconnected.
//---------------------------------------------------------

void VAM::processMessages()
{
  //Process messages from the gui
  //
  //  get and process all pending events from the
  //  synthesizer GUI
  //
  while (gui->fifoSize()) 
  {
    MusECore::MidiPlayEvent ev = gui->readEvent();
    if (ev.type() == MusECore::ME_CONTROLLER) 
    {
      // process local?
      //setController(ev.dataA() & 0xfff, ev.dataB());
      setController(ev.dataA(), ev.dataB());
      sendEvent(ev);
    }
    else
    {
      #ifdef VAM_DEBUG
      printf("VAM::process(): unknown event\n");
      #endif
    }  
  }
}
  
//---------------------------------------------------------
//   process
//   Called from host, ONLY if output path is connected.
//---------------------------------------------------------

void VAM::process(float** ports, int offset, int sampleCount)
      {
      /*
      //
      //  get and process all pending events from the
      //  synthesizer GUI
      //
      while (gui->fifoSize()) {
            MusECore::MidiPlayEvent ev = gui->readEvent();
            if (ev.type() == MusECore::ME_CONTROLLER) {
                  // process local?
                  setController(ev.dataA() & 0xfff, ev.dataB());
                  sendEvent(ev);
                  }
            else
                  printf("VAM::process(): unknown event\n");
            }
      */
            
      float* buffer = *ports + offset;
      if (!isOn)
            return;

      float sample, osc, lfol, *dco1_tbl, *dco2_tbl, *lfo_tbl, pw;
      float cutoff;
      int sr = sampleRate();
      
      dco1_tbl = wave_tbl(dco1.waveform);
      dco2_tbl = wave_tbl(dco2.waveform);
      lfo_tbl = wave_tbl(lfo.waveform);
      
      cutoff = filt_keytrack ? (dco1.freq /500.0 + filt_cutoff)/2 : filt_cutoff;
      cutoff = LIMIT(cutoff, 0.0, 1.0);
      
      for (int i = 0; i < sampleCount; i++) {
            if(!(dco1_env.step() + dco2_env.step())) {
                  isOn = false;
                  break;
                  }
            filt_env.step();

            /* DCO 1 */
            lfol = lfo_tbl[(int)lfo.phase];
            pw = dco1.pw + dco1.pwm * lfol * 0.5;
            pw = LIMIT(pw, 0.0, 1.0);
            if(dco1.phase < sr/2 * ( 1.0 - pw))
                  osc = dco1_tbl[int(dco1.phase / (1.0 - pw))];
            else
                  osc = dco1_tbl[int(dco1.phase / (1.0 + pw))];
            lfol = lfo_tbl[(int)lfo.phase];
            dco1.phase += dco1.freq + dco1.fm * lfol * 1500.0;
            lfo.phase += lfo.freq * 50.0;
            if(!filt_invert)
                  sample = lowpass_filter((cb2amp(960.0 * (1.0 - filt_env_mod * filt_env.env))
                        + 1.0 - filt_env_mod) * cutoff,
                        filt_res, osc, &dco1_filter) * cb2amp(960.0 * (1.0 - dco1_env.env));
            else 
                  sample = lowpass_filter((cb2amp(960.0 * (1.0 - filt_env_mod * (1.0 - filt_env.env)))
                        + 1.0 - filt_env_mod) * cutoff,
                        filt_res, osc, &dco1_filter) * cb2amp(960.0 * (1.0 - dco1_env.env));
            while(dco1.phase > sr) 
                  dco1.phase -= sr;
            while(dco1.phase < 0.0) 
                  dco1.phase += sr;
      
            /* DCO 2 */
            if(dco2.on) {
                  pw = dco2.pw + dco2.pwm * lfol * 0.5;
                  pw = LIMIT(pw, 0.0, 1.0);
                  if(dco2.phase < sr/2 * (1 - pw))
                      osc = dco2_tbl[int(dco2.phase / (1.0 - pw))];
                  else
                      osc = dco2_tbl[int(dco2.phase / (1.0 + pw))];
                  dco2.phase += dco2.freq + dco2.fm * lfol * 1500.0;
                  if(!filt_invert)
                  sample += lowpass_filter((cb2amp(960.0 * (1.0 - filt_env_mod * filt_env.env)) + 1.0 - filt_env_mod) * cutoff,
                      filt_res, osc, &dco2_filter) * cb2amp(960.0 * (1.0 - dco2_env.env));
                  else sample += lowpass_filter((cb2amp(960.0 * (1.0 - filt_env_mod * (1.0 - filt_env.env))) + 1.0 - filt_env_mod) 
                      * cutoff, filt_res, osc, &dco2_filter) * cb2amp(960.0 * (1.0 - dco2_env.env));
      
                  while (dco2.phase > sr)  dco2.phase -= sr;
                  while (dco2.phase < 0.0) dco2.phase += sr;
                  }
            while(lfo.phase > sr)
                  lfo.phase -= sr;
            while(lfo.phase < 0.0)
                  lfo.phase += sr;
            sample *= velocity * 0.5;
            sample = LIMIT(sample, -1.0, 1.0);
      
            //if(sample > 1.0) fprintf(stderr, "oooops %f\n", sample);
            buffer[i] = sample;
            }
      }

//---------------------------------------------------------
//   note
//---------------------------------------------------------

void VAM::note(int chan, int newpitch, int velo)
      {
      if (velo == 0) {
          noteoff(chan, newpitch);
          return;
          }
      isOn = true;
      channel = chan;
      pitch = newpitch;
      velocity = velo / 127.0;
      dco1.freq = 8.176 * exp(float(pitch + dco1.pitchmod + dco1.detune)*log(2.0)/12.0);
      dco2.freq = 8.176 * exp(float(pitch + dco2.pitchmod + dco2.detune)*log(2.0)/12.0);
      keytrack_cutoff = 16.0 * dco1.freq / sampleRate();
      if(keytrack_cutoff > 1.0) keytrack_cutoff = 1.0;
              dco1_env.setSegment(0, dco1_env.attack,    1.0/dco1_env.attack);
              dco1_env.setSegment(1, dco1_env.decay, -((1.0-dco1_env.sustain)/dco1_env.decay));
              dco2_env.setSegment(0, dco2_env.attack,    1.0/dco2_env.attack);
              dco2_env.setSegment(1, dco2_env.decay, -((1.0-dco2_env.sustain)/dco2_env.decay));
              filt_env.setSegment(0, filt_env.attack,    1.0/filt_env.attack);
              filt_env.setSegment(1, filt_env.decay, -((1.0-filt_env.sustain)/filt_env.decay));
      dco1_env.keyOn();
      dco2_env.keyOn();
      filt_env.env = 0.0;
      filt_env.keyOn();
      //	dco1.phase = 0.0;
      //	dco2.phase = 0.0;
      //	lfo.phase = 0.0;
      }

//---------------------------------------------------------
//   noteoff
//---------------------------------------------------------

void VAM::noteoff(int chan, int offpitch)
      {
      if(isOn && (pitch == offpitch) && (channel == chan)) {
            dco1_env.keyOff();
            dco2_env.keyOff();
            filt_env.keyOff();
            }
      }

int VAM::getControllerInfo(int id, const char** name, int* controller,
   int* min, int* max, int* initval) const
      {
      return gui->getControllerInfo(id, name, controller, min, max, initval);
      }

//---------------------------------------------------------
//   setController
//---------------------------------------------------------

bool VAM::setController(int /*channel*/, int ctrl, int data)
      {
      //setController(ctrl & 0xfff, data);
      // p4.0.27
      if(ctrl < VAM_FIRST_CTRL || ctrl > VAM_LAST_CTRL)
      {
        #ifdef VAM_DEBUG
        printf("VAM::setController Invalid controller number 0x%x\n", ctrl);
        #endif
        return false;
      }
      setController(ctrl, data);
      
      MusECore::MidiPlayEvent ev(0, 0, channel, MusECore::ME_CONTROLLER, ctrl, data);
      gui->writeEvent(ev);
      return false;
      }

void VAM::setController(int ctrl, int data)
      {
      // p4.0.27
      if(ctrl < VAM_FIRST_CTRL || ctrl > VAM_LAST_CTRL)
      {
        #ifdef VAM_DEBUG
        printf("VAM: Invalid controller number 0x%x\n", ctrl);
        #endif
        return;
      }
      
      //	fprintf(stderr, "ctrl: %d data: %d\n", ctrl, data);
      int maxval = 128*128-1;
      double normval = double(data) / double(maxval);
      switch (ctrl) {
          case DCO1_PITCHMOD:
              dco1.pitchmod = (data  - 8191) / 341.333;
              break;
          case DCO1_WAVEFORM:
              dco1.waveform = data;
              break;
          case DCO1_FM:
              dco1.fm = lin2exp[int(normval * 255.0)];
              break;
          case DCO1_PWM:
              dco1.pwm = normval;
              break;
          case DCO1_ATTACK:
              dco1_env.attack = int(lin2exp[int(normval * 255.0)] * 5.0 * sampleRate()) + 1;
              break;
          case DCO1_DECAY:
              dco1_env.decay = (data * sampleRate() * 5) / maxval + 1;
              break;
          case DCO1_SUSTAIN:
              dco1_env.sustain = normval;
              break;
          case DCO1_RELEASE:
              dco1_env.release = int(lin2exp[int(normval * 255.0)] * 10.0 * sampleRate()) + 1;
              dco1_env.setSegment(2, dco1_env.release, -(1.0/dco1_env.release));
              break;

          case DCO2_PITCHMOD:
              dco2.pitchmod = (data - 8191) / 341.333;
              break;
          case DCO2_WAVEFORM:
              dco2.waveform = data;
              break;
          case DCO2_FM:
              dco2.fm = normval;
              break;
          case DCO2_PWM:
              dco2.pwm = normval;
              break;
          case DCO2_ATTACK:
              dco2_env.attack = int(lin2exp[int(normval * 255.0)] * 5.0 * sampleRate()) + 1;
              break;
          case DCO2_DECAY:
              dco2_env.decay = (data * sampleRate() * 5) / maxval + 1;
              break;
          case DCO2_SUSTAIN:
              dco2_env.sustain = normval;
              break;
          case DCO2_RELEASE:
              dco2_env.release = int(lin2exp[int(normval * 255.0)] * 10.0 * sampleRate()) + 1;
              dco2_env.setSegment(2, dco2_env.release, -(1.0/dco2_env.release));
              break;
          case LFO_FREQ:
              lfo.freq = lin2exp[int(normval * 255.0)];
              //fprintf(stderr, "%f\n", lfo.freq);
              break;
          case LFO_WAVEFORM:
              lfo.waveform = data;
              break;
          case FILT_ENV_MOD:
              filt_env_mod = 1.0 - lin2exp[int(255.0 - normval * 255.0)];
              break;
          case FILT_KEYTRACK:
              filt_keytrack = data;
              break;
          case FILT_RES:
              filt_res = normval;
              break;
          case FILT_ATTACK:
              //filt_env.attack = int(lin2exp[int(normval * 255.0)] * 5.0 * sampleRate());
              filt_env.attack = int(lin2exp[int(normval * 255.0)] * 5.0 * sampleRate()) + 1;
              break;
          case FILT_DECAY:
              filt_env.decay = (data * sampleRate() * 5) / maxval + 1;
              break;
          case FILT_SUSTAIN:
              filt_env.sustain = normval;
              break;
          case FILT_RELEASE:
              filt_env.release = int(lin2exp[int(normval * 255.0)] * 10.0 * sampleRate()) + 1;
              filt_env.setSegment(2, filt_env.release, -(1.0/filt_env.release));
              break;
          case DCO2ON:
              dco2.on = data;
              break;
          case FILT_INVERT:
              filt_invert = data;
              break;
          case FILT_CUTOFF:
              filt_cutoff = normval;
              //fprintf(stderr, "%f\n", filt_cutoff);
              break;
          case DCO1_DETUNE:
              dco1.detune = (data - 8191) / 16384.0;
              break;
          case DCO2_DETUNE:
              dco2.detune = (data - 8191) / 16384.0;
              break;
          case DCO1_PW:
              dco1.pw = normval;
              if(dco1.pw == 1.0) 
                  dco1.pw = 0.99;
              break;
          case DCO2_PW:
              dco2.pw = normval;
              if(dco2.pw == 1.0) dco2.pw = 0.99;
              break;
          default:
              //#ifdef VAM_DEBUG
              //printf("VAM: set unknown Ctrl 0x%x to 0x%x\n", ctrl, data);
              //#endif
              //break;
              return;   // p4.0.27
          }
      //controller[ctrl] = data;
      controller[ctrl - VAM_FIRST_CTRL] = data;        // p4.0.27
      }

//---------------------------------------------------------
//   getInitData
//---------------------------------------------------------

//void VAM::getInitData(int* n, const unsigned char**p) const
void VAM::getInitData(int* n, const unsigned char**p) 
{
      // p4.0.27
      *n = 3 + NUM_CONTROLLER * sizeof(int);
      idata[0] = MUSE_SYNTH_SYSEX_MFG_ID;         // Global MusE Soft Synth Manufacturer ID
      idata[1] = VAM_UNIQUE_ID;                   // VAM
      idata[2] = INIT_DATA_CMD;                   // Initialization command
      int* d = (int*)&idata[3];
      
      //int i;//prevent of compiler warning: unused variable
      //int* d = idata;
      //int maxval = 128*128-1;	//prevent of compiler warning: unused variable
      // *n = NUM_CONTROLLER * sizeof(int);

// //       setController(0, DCO1_PITCHMOD, p++);
//       *d++ = int(dco1.pitchmod+8191*341.333);
      *d++ = gui->getController(DCO1_PITCHMOD);

// //       setController(0, DCO2_PITCHMOD, p++);
//       *d++ = int(dco2.pitchmod+8191*341.333);
      *d++ = gui->getController(DCO2_PITCHMOD);

// //       setController(0, DCO1_WAVEFORM, p++);
//       *d++ = dco1.waveform;
      *d++ = gui->getController(DCO1_WAVEFORM);

// //       setController(0, DCO2_WAVEFORM, p++);
//       *d++ = dco2.waveform;
      *d++ = gui->getController(DCO2_WAVEFORM);

// //       setController(0, DCO1_FM, p++);
//       for (i = 0;i<LIN2EXP_SIZE;i++) {
//           if (fabs((lin2exp[i] == dco1.fm)) < 0.1)
//             break;
//           }
//       *d++ = int(i/255.0*double(maxval));
      *d++ = gui->getController(DCO1_FM);

// 
// 
// //       setController(0, DCO2_FM, p++);
//       for (i = 0;i<LIN2EXP_SIZE;i++) {
//           if (fabs((lin2exp[i] - dco2.fm)) < 0.1)
//             break;
//           }
//       *d++ = int(i/255.0*double(maxval));
      *d++ = gui->getController(DCO2_FM);

// 
// //       setController(0, DCO1_PWM, p++);
//       *d++ = int(dco1.pwm*double(maxval));
      *d++ = gui->getController(DCO1_PWM);

// 
// //       setController(0, DCO2_PWM, p++);
//       *d++ = int(dco2.pwm*double(maxval));
      *d++ = gui->getController(DCO2_PWM);

// 
// //       setController(0, DCO1_ATTACK, p++);
//       for (i = 0;i<LIN2EXP_SIZE;i++) {
//           if (fabs(lin2exp[i] -( (dco1_env.attack-1)/5.0/sampleRate())) < 0.1)
//             break;
//           }
//       *d++ = int(i/255.0*double(maxval));
      *d++ = gui->getController(DCO1_ATTACK);
// 
// //       setController(0, DCO2_ATTACK, p++);
//       for (i = 0;i<LIN2EXP_SIZE;i++) {
//           if (fabs(lin2exp[i] -( (dco2_env.attack-1)/5.0/sampleRate())) < 0.1)
//             break;
//           }
//       *d++ = int(i/255.0*double(maxval));
      *d++ = gui->getController(DCO2_ATTACK);

// 
// //       setController(0, DCO1_DECAY, p++);
//       *d++ = int((dco1_env.decay-1)/sampleRate()/5 * maxval);
      *d++ = gui->getController(DCO1_DECAY);

// 
// //       setController(0, DCO2_DECAY, p++);
//       *d++ = int((dco2_env.decay-1)/sampleRate()/5 * maxval);
      *d++ = gui->getController(DCO2_DECAY);

// 
// //       setController(0, DCO1_SUSTAIN, p++ );
//       *d++ = int(dco1_env.sustain*double(maxval));
      *d++ = gui->getController(DCO1_SUSTAIN);

// 
// //       setController(0, DCO2_SUSTAIN, p++ );
//       *d++ = int(dco2_env.sustain*double(maxval));
      *d++ = gui->getController(DCO2_SUSTAIN);
// 
// //       setController(0, DCO1_RELEASE, p++);
//       for (i = 0;i<LIN2EXP_SIZE;i++) {
//           if (fabs(lin2exp[i] -( (dco1_env.release-1)/10.0/sampleRate())) < 0.1)
//             break;
//           }
//       *d++ = int(i/255.0*double(maxval));
      *d++ = gui->getController(DCO1_RELEASE);

// 
// //       setController(0, DCO2_RELEASE, p++);
//       for (i = 0;i<LIN2EXP_SIZE;i++) {
//           if (fabs(lin2exp[i] -( (dco2_env.release-1)/10.0/sampleRate())) < 0.1)
//             break;
//           }
//       *d++ = int(i/255.0*double(maxval));
      *d++ = gui->getController(DCO2_RELEASE);

// 
// //       setController(0, LFO_FREQ, p++);
//       for (i = 0;i<LIN2EXP_SIZE;i++) {
//           if (fabs((lin2exp[i] - lfo.freq)) < 0.1)
//             break;
//           }
//       *d++ = int(i/255.0*double(maxval));
      *d++ = gui->getController(LFO_FREQ);

// 
// //       setController(0, LFO_WAVEFORM, p++);
//       *d++ = lfo.waveform;
      *d++ = gui->getController(LFO_WAVEFORM);

// 
// //       setController(0, FILT_ENV_MOD, p++);
//       for (i = 0;i<LIN2EXP_SIZE;i++) {
//           if (fabs((lin2exp[i] - (1 - filt_env_mod))) < 0.1)
//             break;
//           }
//       *d++ = int((255-i)/255.0*double(maxval));
      *d++ = gui->getController(FILT_ENV_MOD);

// 
// //       setController(0, FILT_KEYTRACK, p++);
//       *d++ = filt_keytrack;
      *d++ = gui->getController(FILT_KEYTRACK);

// 
// //       setController(0, FILT_RES, p++);
//       *d++ = int(filt_res*double(maxval));
      *d++ = gui->getController(FILT_RES);

// 
// //       setController(0, FILT_ATTACK, p++);
//       for (i = 0;i<LIN2EXP_SIZE;i++) {
//           if (fabs(lin2exp[i] -( (filt_env.attack-1)/5.0/sampleRate())) < 0.1)
//             break;
//           }
//       *d++ = int(i/255.0*double(maxval));
      *d++ = gui->getController(FILT_ATTACK);

// 
// //       setController(0, FILT_DECAY, p++);
//       *d++ = int((filt_env.decay-1)/sampleRate()*double(maxval)/5);
      *d++ = gui->getController(FILT_DECAY);

// 
// //       setController(0, FILT_SUSTAIN, p++);
//       *d++ = int(filt_env.sustain*double(maxval));
      *d++ = gui->getController(FILT_SUSTAIN);

// 
// //       setController(0, FILT_RELEASE, p++);
//       for (i = 0;i<LIN2EXP_SIZE;i++) {
//           if (fabs(lin2exp[i] -( (filt_env.release-1)/10.0/sampleRate())) < 0.1)
//             break;
//           }
//       *d++ = int(i/255.0*double(maxval));
      *d++ = gui->getController(FILT_RELEASE);

// 
// //       setController(0, DCO2ON, p++);
//       *d++ = dco2.on;
      *d++ = gui->getController(DCO2ON);

// 
// //       setController(0, FILT_INVERT, p++);
//       *d++ = filt_invert;
      *d++ = gui->getController(FILT_INVERT);

// 
// //       setController(0, FILT_CUTOFF, p++);
//       *d++ = int(filt_cutoff*double(maxval));
      *d++ = gui->getController(FILT_CUTOFF);

// 
// //       setController(0, DCO1_DETUNE, p++);
//       *d++ = int(dco1.detune *16834 + 8191);
      *d++ = gui->getController(DCO1_DETUNE);

// 
// //       setController(0, DCO2_DETUNE, p++);
//       *d++ = int(dco2.detune *16834 + 8191);
      *d++ = gui->getController(DCO2_DETUNE);

// 
// //       setController(0, DCO1_PW, p++);
//       *d++ = int(dco1.pw*double(maxval));
      *d++ = gui->getController(DCO1_PW);

// 
// //       setController(0, DCO2_PW, p++);
//       *d++ = int(dco2.pw*double(maxval));
      *d++ = gui->getController(DCO2_PW);

      *p = (unsigned char*)idata;
}

//---------------------------------------------------------
//   sysex
//---------------------------------------------------------

bool VAM::sysex(int n, const unsigned char* data)
{
      // p4.0.27
      if(unsigned(n) == (3 + NUM_CONTROLLER * sizeof(int))) 
      {
        if (data[0] == MUSE_SYNTH_SYSEX_MFG_ID)              //  Global MusE Soft Synth Manufacturer ID
        {
          if (data[1] == VAM_UNIQUE_ID)                      // VAM
          {
            if (data[2] == INIT_DATA_CMD)                  // Initialization command
            {  
              int *p= (int*)(data + 3);
              setController(0, DCO1_PITCHMOD, *p++);
              setController(0, DCO2_PITCHMOD, *p++);
              setController(0, DCO1_WAVEFORM, *p++);
              setController(0, DCO2_WAVEFORM, *p++);
              setController(0, DCO1_FM, *p++);
              setController(0, DCO2_FM, *p++);
              setController(0, DCO1_PWM, *p++);
              setController(0, DCO2_PWM, *p++);
              setController(0, DCO1_ATTACK, *p++);
              setController(0, DCO2_ATTACK, *p++);
              setController(0, DCO1_DECAY, *p++);
              setController(0, DCO2_DECAY, *p++);
              setController(0, DCO1_SUSTAIN, *p++ );
              setController(0, DCO2_SUSTAIN, *p++ );
              setController(0, DCO1_RELEASE, *p++);
              setController(0, DCO2_RELEASE, *p++);
              setController(0, LFO_FREQ, *p++);
              setController(0, LFO_WAVEFORM, *p++);
              setController(0, FILT_ENV_MOD, *p++);
              setController(0, FILT_KEYTRACK, *p++);
              setController(0, FILT_RES, *p++);
              setController(0, FILT_ATTACK, *p++);
              setController(0, FILT_DECAY, *p++);
              setController(0, FILT_SUSTAIN, *p++);
              setController(0, FILT_RELEASE, *p++);
              setController(0, DCO2ON, *p++);
              setController(0, FILT_INVERT, *p++);
              setController(0, FILT_CUTOFF, *p++);
              setController(0, DCO1_DETUNE, *p++);
              setController(0, DCO2_DETUNE, *p++);
              setController(0, DCO1_PW, *p++);
              setController(0, DCO2_PW, *p++);
              return false;
            }
          }  
        }
      }
      
      #ifdef VAM_DEBUG
      printf("VAM: unknown sysex\n");
      #endif
      
      return false;
}

//---------------------------------------------------------
//   nativeGuiVisible
//---------------------------------------------------------

bool VAM::nativeGuiVisible() const
      {
      return gui->isVisible();
      }

//---------------------------------------------------------
//   showNativeGui
//---------------------------------------------------------

void VAM::showNativeGui(bool val)
      {
      gui->setVisible(val);
      }

//---------------------------------------------------------
//   getNativeGeometry
//---------------------------------------------------------

void VAM::getNativeGeometry(int* x, int* y, int* w, int* h) const
      {
      QPoint pos(gui->pos());
      QSize size(gui->size());
      *x = pos.x();
      *y = pos.y();
      *w = size.width();
      *h = size.height();
      }

//---------------------------------------------------------
//   setNativeGeometry
//---------------------------------------------------------

void VAM::setNativeGeometry(int x, int y, int w, int h)
      {
      gui->resize(QSize(w, h));
      gui->move(QPoint(x, y));
      }

//---------------------------------------------------------
//   inst
//---------------------------------------------------------

class QWidget;

static Mess* instantiate(int sr, QWidget*, QString*, const char* name)
      {
      VAM* vam = new VAM(sr);
      if (vam->init(name)) {
            delete vam;
            return 0;
            }
      return vam;
      }

extern "C" {
      static MESS descriptor = {
            "vam",
            "vam soft synth",
            "0.1",      // version string
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

