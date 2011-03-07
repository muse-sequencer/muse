//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: organ.h,v 1.5 2004/04/15 13:46:18 wschweer Exp $
//
//  Parts of this file taken from:
//      Organ - Additive Organ Synthesizer Voice
//      Copyright (c) 1999, 2000 David A. Bartold
//
//  (C) Copyright 2001-2007 Werner Schweer (ws@seh.de)
//=========================================================

#ifndef __ORGAN_H__
#define __ORGAN_H__

#include "muse/midictrl.h"
#include "libsynti/mess2.h"

static const int NO_VOICES = 128;    // max polyphony
static const int NO_KEYS = 97 - 36;
class OrganGui;
class Reverb;

static const int MAX_ATTENUATION = 960;
static const int NO_BUSES        = 9;
static const int NO_WHEELS       = 91;
static const int NO_ELEMENTS     = 194;

enum {
      DRAWBAR0 = CTRL_RPN14_OFFSET, DRAWBAR1, DRAWBAR2,
         DRAWBAR3, DRAWBAR4, DRAWBAR5, DRAWBAR6, DRAWBAR7, DRAWBAR8,
      REVERB_ON, REVERB_ROOM_SIZE, REVERB_MIX,
      VIBRATO_ON, VIBRATO_FREQ, VIBRATO_DEPTH,
      PERC_ON, PERC_GAIN, PERC_DECAY, PERC_HARMONY,
      ROTARY_ON, ROT1_FREQ, ROT1_DEPTH, ROT2_FREQ, ROT2_DEPTH
      };

//---------------------------------------------------------
//   Wheel
//---------------------------------------------------------

struct Wheel {
      unsigned frameStep;
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
//   Elem
//---------------------------------------------------------

struct Elem {
      char wheel;
      char bus;
      float level;

      Elem() { bus = -1; }
      Elem(char w, char b, float l) : wheel(w), bus(b), level(l) {}
      };

//---------------------------------------------------------
//   Organ
//---------------------------------------------------------

class Organ : public Mess2 {
      static int useCount;

      static float* waveTable;
      static double cb2amp_tab[MAX_ATTENUATION];
      static double cb2amp(int cb);
      static Elem routing[NO_KEYS][NO_ELEMENTS];
      static float* attackEnv;
      static float* releaseEnv;
      static int envSize;
      static float keyCompression[NO_VOICES];

      OrganGui* gui;
      Reverb* reverb;
      bool reverbOn;
      double volume;

      unsigned vibratoStep;
      unsigned vibratoAccu;

      bool vibratoOn;
      double vibratoFreq;
      double vibratoDepth;

      // key compression
      float keyCompressionDelta;
      float keyCompressionValue;
      int keyCompressionCount;

      // percussion
      int percussionBus;      // usually drawbar 3 or drawbar 4
      bool percussionOn;
      double percDecay;
      double percussionEnvDecay;
      double percGain;
      double percGainInit;

      // rotary speaker emulation
      bool rotaryOn;
      double rot1Freq;        // horn: 0,67 - 6,7
      double rot1Depth;
      double rot2Freq;        // drum: 0,5  - 5,5
      double rot2Depth;
      unsigned rot1Step;
      unsigned rot1AccuL;
      unsigned rot1AccuR;
      unsigned rot2Step;
      unsigned rot2AccuL;
      unsigned rot2AccuR;

      float drawBarGain[NO_BUSES];
      Wheel wheels[NO_WHEELS];
      QList<char> pressedKeys;
      QList<Wheel*> activeWheels;

      void setController(int ctrl, int val);
      void changeKeyCompression();
      void percussionChanged();

      virtual void process(float**, int, int);
      virtual bool playNote(int channel, int pitch, int velo);
      virtual bool setController(int channel, int ctrl, int val);
	virtual bool sysex(int, const unsigned char*);

      virtual bool guiVisible() const;
      virtual void showGui(bool);
      virtual bool hasGui() const { return true; }
      virtual void getGeometry(int* x, int* y, int* w, int* h) const;
      virtual void setGeometry(int x, int y, int w, int h);

   public:
      friend class OrganGui;
      Organ(int sampleRate);
      ~Organ();
      bool init(const char* name);
      };

#endif

