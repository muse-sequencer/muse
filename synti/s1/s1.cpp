//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: s1.cpp,v 1.9.2.5 2009/11/19 04:20:33 terminator356 Exp $
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

#include <QMessageBox>

#include "libsynti/mono.h"

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
      bool _showGui;
      
      int param;

      virtual void note(int channel, int pitch, int velo);
      //virtual void processMessages();
      virtual void process(float** buffer, int offset, int n);
      virtual bool hasGui() const { return true; }
      virtual bool guiVisible() const { return _showGui; }
      virtual void showGui(bool);
      virtual bool setController(int channel, int ctrl, int val);
      virtual int getControllerInfo(int id, const char** name, int* ctrl, int* min, int* max, int* initval) const;

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
      
      param = 0;
      
      _showGui=false;
      showGui(true);
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
      float sample1, sample2;
      unsigned freq_256 = (int) (freq * ((double) RESOLUTION) / sampleRate() * 256.0);
      for (int i = 0; i < n; i++) {
            accu += freq_256;
            while (accu >= RESOLUTION * 256)
                  accu -= RESOLUTION * 256;
            
            sample1 = wave_table[accu >> 8]; // sinus component
            
            if (sample1< 0.0f) // square wave component
              sample2 = -0.4; 
            else
              sample2 = 0.4;
            
            sample = ((1.0-float(param)/127.0)*sample1 + (float(param)/127.0)*sample2) / 2.0;
            
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
      

void S1::showGui(bool show)
      {
      if (show)
        QMessageBox::information( NULL, "S1",
        "S1 is a demo synth mainly for\n"
        "developers wishing to learn\n"
        "how to make a M.E.S.S synth.\n"
        "\n"
        "One modulation parameter is available,\n"
        "it sweeps the signal between square and\n"
        "sinus wave.\n", 1 );
      }
     
bool S1::setController(int, int ctrl, int val)
      {
      if (ctrl == 1) {
             param = val;
             }
      return true;
      }

int S1::getControllerInfo(int id, const char** name, int* ctrl, int* min, int* max, int* initval) const
      {
        if (id == 0) {
            *name = "Modulation";
            *ctrl = 1;
            *min = 0;
            *max = 127;
            *initval = 0;
            return 1;
            }
        else
          return 0;
      }

//---------------------------------------------------------
//   inst
//---------------------------------------------------------
class QWidget;


static Mess* instantiate(int sr, QWidget*, QString* projectPathPtr, const char*)
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
      // We must compile with -fvisibility=hidden to avoid namespace
      // conflicts with global variables.
      // Only visible symbol is "mess_descriptor".
      // (TODO: all plugins should be compiled this way)
  
      __attribute__ ((visibility("default")))
      const MESS* mess_descriptor() { return &descriptor; }
      }

