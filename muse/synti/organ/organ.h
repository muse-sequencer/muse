//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: organ.h,v 1.5 2004/04/15 13:46:18 wschweer Exp $
//
//  Parts of this file taken from:
//      Organ - Additive Organ Synthesizer Voice
//      Copyright (c) 1999, 2000 David A. Bartold
//
//  (C) Copyright 2001-2004 Werner Schweer (ws@seh.de)
//=========================================================

#ifndef __ORGAN_H__
#define __ORGAN_H__

#include "muse/midictrl.h"
#include "libsynti/mess.h"

#define RESOLUTION   (16384*2)
#define VOICES          128    // max polyphony

class OrganGui;

static const int MAX_ATTENUATION = 960;

enum EnvelopeState {
      ATTACK,
      DECAY,
      SUSTAIN,
      RELEASE,
      OFF
      };

//---------------------------------------------------------
//   Envelope
//---------------------------------------------------------

struct Envelope {
      int ticks;        // len of segment
      int error, delta, schritt;
      int y, yinc;

      void set(int t, int y1, int y2) {
            ticks   = t;
            y       = y1;
            int dy  = y2 - y1;
            int dx  = t;
            error   = -dx;
            schritt = 2*dx;
            if (dy < 0) {
                  yinc = -1;
                  delta = -2 * dy;
                  }
            else {
                  yinc = 1;
                  delta   = 2 * dy;
                  }
            }

      // return false on envelope end
      bool step(int* a) {
            *a = y;
            if (ticks == 0)
                  return false;
            error += delta;
            while (error > 0) {
                  y += yinc;
                  error -= schritt;
                  }
            --ticks;
            return true;
            }
      };

static const int HARM0      =  0 + CTRL_RPN14_OFFSET;
static const int HARM1      =  1 + CTRL_RPN14_OFFSET;
static const int HARM2      =  2 + CTRL_RPN14_OFFSET;
static const int HARM3      =  3 + CTRL_RPN14_OFFSET;
static const int HARM4      =  4 + CTRL_RPN14_OFFSET;
static const int HARM5      =  5 + CTRL_RPN14_OFFSET;
static const int ATTACK_LO  =  6 + CTRL_RPN14_OFFSET;
static const int DECAY_LO   =  7 + CTRL_RPN14_OFFSET;
static const int SUSTAIN_LO =  8 + CTRL_RPN14_OFFSET;
static const int RELEASE_LO =  9 + CTRL_RPN14_OFFSET;
static const int ATTACK_HI  = 10 + CTRL_RPN14_OFFSET;
static const int DECAY_HI   = 11 + CTRL_RPN14_OFFSET;
static const int SUSTAIN_HI = 12 + CTRL_RPN14_OFFSET;
static const int RELEASE_HI = 13 + CTRL_RPN14_OFFSET;
static const int BRASS      = 14 + CTRL_RPN14_OFFSET;
static const int FLUTE      = 15 + CTRL_RPN14_OFFSET;
static const int REED       = 16 + CTRL_RPN14_OFFSET;
static const int VELO       = 17 + CTRL_RPN14_OFFSET;

//---------------------------------------------------------
//   SynthCtrl
//---------------------------------------------------------

struct SynthCtrl {
      const char* name;
      int num;
      int val;
      };

//---------------------------------------------------------
//   Voice
//---------------------------------------------------------

struct Voice {
      bool isOn;
      int pitch;
      int channel;

      double velocity;

      int state1, state2;
      Envelope envL1, envL2, envL3;
      Envelope envH1, envH2, envH3;

      unsigned harm0_accum;
      unsigned harm1_accum;
      unsigned harm2_accum;
      unsigned harm3_accum;
      unsigned harm4_accum;
      unsigned harm5_accum;
      };

//---------------------------------------------------------
//   Preset
//---------------------------------------------------------

struct Preset {
      char* name;
      bool brass, flute, reed;
      int attack0, attack1;
      int release0, release1;
      int decay0, decay1;
      double harm0, harm1, harm2, harm3, harm4, harm5;
      bool velo;
      };

//---------------------------------------------------------
//   Organ
//---------------------------------------------------------

class Organ : public Mess {
      static int useCount;

      static double cb2amp_tab[MAX_ATTENUATION];
      static unsigned freq256[128];
      static double cb2amp(int cb);

      int* idata;  // buffer for init data

      bool brass, flute, reed;
      int attack0, attack1;
      int release0, release1;
      int decay0, decay1;        // ticks
      int sustain0, sustain1;    // centibel
      bool velo;
      double volume;

      double harm0, harm1, harm2, harm3, harm4, harm5;

      Voice voices[VOICES];

      static float* sine_table;
      static float* g_triangle_table;
      static float* g_pulse_table;

      void noteoff(int channel, int pitch);
      void setController(int ctrl, int val);

      virtual void process(float**, int, int);
      virtual bool playNote(int channel, int pitch, int velo);
      virtual bool setController(int channel, int ctrl, int val);
	virtual bool sysex(int, const unsigned char*);

      virtual int getControllerInfo(int, const char**, int*, int*, int*);
      virtual void getInitData(int*, const unsigned char**);

      virtual bool guiVisible() const;
      virtual void showGui(bool);
      virtual bool hasGui() const { return true; }
      virtual void getGeometry(int* x, int* y, int* w, int* h) const;
      virtual void setGeometry(int x, int y, int w, int h);

      OrganGui* gui;

   public:
      static SynthCtrl synthCtrl[];
      Organ(int sampleRate);
      ~Organ();
      bool init(const char* name);
      };

#endif

