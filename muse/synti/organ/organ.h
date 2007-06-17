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
#include "libsynti/mess2.h"

static const int NO_VOICES = 128;    // max polyphony
static const int NO_KEYS = 97 - 36;
class OrganGui;

static const int MAX_ATTENUATION = 960;
static const int NO_BUSES        = 9;
static const int NO_WHEELS       = 91;
static const int NO_ELEMENTS     = 194;

enum {
      DRAWBAR0 = CTRL_RPN14_OFFSET, DRAWBAR1, DRAWBAR2,
         DRAWBAR3, DRAWBAR4, DRAWBAR5, DRAWBAR6, DRAWBAR7, DRAWBAR8,
      ATTACK_LO, DECAY_LO, SUSTAIN_LO, RELEASE_LO,
      ATTACK_HI, DECAY_HI, SUSTAIN_HI, RELEASE_HI,
      BRASS, FLUTE, REED, VELO
      };

//---------------------------------------------------------
//   Wheel
//---------------------------------------------------------

struct Wheel {
      unsigned freq256;
      unsigned accu;

      int refCount;
      bool active;
      float gain[NO_BUSES];

      // envelopes:
      float* env[NO_BUSES];
      int envCount[NO_BUSES];
      float deltaGain[NO_BUSES];
      };

//---------------------------------------------------------
//   Voice
//---------------------------------------------------------

struct Voice {
      bool isOn;
      int pitch;
      };

struct Elem {
      short wheel;
      short bus;
      float level;

      Elem() { bus = -1; }
      Elem(short w, short b, float l) : wheel(w), bus(b), level(l) {}
      };

//---------------------------------------------------------
//   Organ
//---------------------------------------------------------

class Organ : public Mess2 {
      static int useCount;

      static float* waveTable;
      static double cb2amp_tab[MAX_ATTENUATION];
      static unsigned freq256[128][NO_BUSES];
      static double cb2amp(int cb);
      static Elem routing[NO_KEYS][NO_ELEMENTS];
      static float* attackEnv;
      static float* releaseEnv;
      static int envSize;
      static int resolution;
      static int resolution256;

      double volume;

      float drawBarGain[NO_BUSES];
      Wheel wheels[NO_WHEELS];
      Voice voices[NO_VOICES];
      QList<Wheel*> activeWheels;

      void noteoff(int channel, int pitch);
      void setController(int ctrl, int val);

      virtual void process(float**, int, int);
      virtual bool playNote(int channel, int pitch, int velo);
      virtual bool setController(int channel, int ctrl, int val);
	virtual bool sysex(int, const unsigned char*);

      virtual bool guiVisible() const;
      virtual void showGui(bool);
      virtual bool hasGui() const { return true; }
      virtual void getGeometry(int* x, int* y, int* w, int* h) const;
      virtual void setGeometry(int x, int y, int w, int h);

      OrganGui* gui;

   public:
      friend class OrganGui;
      Organ(int sampleRate);
      ~Organ();
      bool init(const char* name);
      };

#endif

