//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: s1.cpp,v 1.10 2005/01/13 21:16:27 wschweer Exp $
//
//    S1  - simple mono demo synthesizer
//        - plays only one note at a time
//        - has no gui nor any controller
//
//    Version 0.2: stop note on wave zero crossing to avoid
//                 clicks
//
//  (C) Copyright 2001-2004 Werner Schweer (ws@seh.de)
//=========================================================

#include <cmath>
#include <list>

#include "synti/libsynti/mono.h"

#define RESOLUTION   16384

//---------------------------------------------------------
//   S1 - simple mono demo synthesizer
//---------------------------------------------------------

class S1 : public MessMono {
      static int useCount;
      static float *wave_table;

      int gate;
      float freq;
      unsigned accu;
      float sample;

      virtual void note(int channel, int pitch, int velo);
      virtual void process(float** buffer, int offset, int n);

   public:
      S1();
      ~S1();
      };

float* S1::wave_table;
int S1::useCount = 0;

//---------------------------------------------------------
//   S1
//---------------------------------------------------------

S1::S1() : MessMono()
      {
      if (useCount++ == 0) {
            //
            // create sinus wave table
            //
            wave_table = new float[RESOLUTION];
            for (int i = 0; i < RESOLUTION; i++)
                  wave_table[i] = sin ((i * 2.0 * M_PI) / RESOLUTION) / 6.0;
            }
      gate = 0;
      }

//---------------------------------------------------------
//   ~S1
//---------------------------------------------------------

S1::~S1()
      {
      if (--useCount == 0)
            delete[] wave_table;
      }

//---------------------------------------------------------
//   noteon
//    process note on
//---------------------------------------------------------

void S1::note(int /*channel*/, int pitch, int velo)
      {
      if (velo == 0) {
            //
            // note off
            //
            if (sample == 0.0)
                  gate = 0;
            else if (sample > 0.0)
                  gate = 2;
            else if (sample < 0.0)
                  gate = 3;
            }
      else {
            //
            // note on
            //
            accu     = 0;
            gate     = 1;
            freq     = 8.176 * exp(float(pitch)*log(2.0)/12.0);
            }
      }

//---------------------------------------------------------
//   write
//    synthesize n samples into buffer+offset
//---------------------------------------------------------

void S1::process(float** buffer, int offset, int n)
      {
      if (gate == 0)
            return;
      float* p = buffer[0] + offset;
      unsigned freq_256 = (int) (freq * ((double) RESOLUTION) / sampleRate() * 256.0);
      for (int i = 0; i < n; i++) {
            accu += freq_256;
            while (accu >= RESOLUTION * 256)
                  accu -= RESOLUTION * 256;
            sample = wave_table[accu >> 8];
            //
            // stop on zero crossing
            // if in decay state
            //
            if (gate == 2 && sample <= 0.0) {
                  gate = 0;
                  break;
                  }
            else if (gate == 3 && sample >= 0.0) {
                  gate = 0;
                  break;
                  }
            p[i] += sample;
            }
      }

//---------------------------------------------------------
//   inst
//---------------------------------------------------------

static Mess* instantiate(int sr, const char*)
      {
      S1* s1 = new S1();
      s1->setSampleRate(sr);
      return s1;
      }

extern "C" {
      static MESS descriptor = {
            "S1",
            "S1 MusE Demo Software Synthesizer",
            "0.2",      // version string
            MESS_MAJOR_VERSION, MESS_MINOR_VERSION,
            instantiate
            };

      const MESS* mess_descriptor() { return &descriptor; }
      }

