//===========================================================================
//
//    DeicsOnze an emulator of the YAMAHA DX11 synthesizer
//
//    Version 0.2.2
//
//
//
//
//  Copyright (c) 2004 Nil Geisweiller
//
//
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
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
// 02111-1307, USA or point your web browser to http://www.gnu.org.
//===========================================================================


#ifndef __DEICSONZE_H
#define __DEICSONZE_H

#include <iostream>
#include <vector>
#include <string>

#include "deicsonzegui.h"
#include "libsynti/mess.h"

// #define PRESETPATH "/home/a-lin/sources/muse-0.7.0/synti/deicsonze/ARCH_ALIN"

#define MAXPITCHBENDVALUE 8191

#define RESOLUTION   96000

#define NBRBANKS 19
#define NBRPRESETS 128

#define LOWERNOTEFREQ 8.176

#define DB0LEVEL 90

#define MAXFEEDBACK 7
#define MAXSPEED 99
#define MAXDELAY 99
#define MAXPMODDEPTH 99
#define MAXAMODDEPTH 99
#define MAXSENSPITCH 7
#define MAXKVS 7
#define MAXAR 31
#define MAXD1R 31
#define MAXD1L 15
#define MAXD2R 31
#define MAXOUTLEVEL 99
#define MAXRR 15
#define MAXRATE 3
#define MAXPBENDRANGE 12
#define MAXPROTATIME 99
#define MAXFCVOLUME 99
#define MAXFCPITCH 99
#define MAXFCAMPLITUDE 99
#define MAXMWPITCH 99
#define MAXMWAMPLITUDE 99
#define MAXBCPITCH 99
#define MAXBCAMPLITUDE 99
#define MAXBCEGBIAS 50
#define LENGTHNAME 20
#define LENGTHCATEGORY 20
#define LENGTHSUBCATEGORY 20

#define MAXGLOBALDETUNE 15
#define MAXVELO 127
#define MAXVOLUME 100.0

#define MAXMASTERVOL 255

//coef determined by ear to sound like the YAMAHA DX11
#define COEFFEEDBACK 0.3
#define COEFPLFO(x) (x==0?0.0:(x==1?0.06:(x==2?0.12:(x==3?0.25:(x==4?0.5:(x==5?0.9:(x==6?3.9:7.9))))))) //return pitch amplitude with respect to sensitivity pitch
#define COEFALFO(x) (x==0?0.0:(x==1?0.4:(x==2?0.9:1.0)))
#define COEFLEVEL 1.0//19.0
#define COEFMAXATTACK 7.5
#define COEFERRDECSUS 0.01 //for the transition between DECAY and SUSTAIN
#define COEFERRSUSREL 0.001 //from SUSTAIN or RELEASE until no sound
#define COEFDECAY 1.0
#define COEFSUSTAIN 0.2
#define COEFRELEASE 1.0
#define COEFDETUNE 0.008
#define COEFLEVELSCALE 0.0005
#define LEVELSCALENOTE 24.0

#define NBROP 4 //do not change
#define NBRWAVES 8 //number wave forms, do not change
#define NBRBANKPRESETS 32
#define NBRVOICES 8


//---------------------------------------------------------
// Algorithm
//---------------------------------------------------------

enum Algorithm {
  FIRST, // Op 0 modulated by Op 1 modulated by Op 2 modulated by Op3
  SECOND, // Op 0 modulated by Op 1 modulated by both Op 2 and Op 3
  THIRD, // Op 0 modulated by both Op 3 and Op 1 modulated by Op 2
  FOURTH, // Op 0 modulated by both Op 1 and Op 2 modulated by Op 3
  FIFTH, // (Op 0 modulated by Op 1) add to (Op 2 modulated by Op 3)
  SIXTH, // addition of the three Op 0, 1, 2 all modulated by Op 3
  SEVENTH, // addition of the three Op 0, 1, 2 with 2 modulated by Op3
  EIGHTH // addition of the four Op 0, 1, 2, 3
};

//---------------------------------------------------------
// Wave of the low frequency modulation
//---------------------------------------------------------
enum Wave {
  SAWUP,
  SQUARE,
  TRIANGL,
  SHOLD
};

//---------------------------------------------------------
// Lfo, low frequency modulation
//---------------------------------------------------------
struct Lfo {
  Wave wave;
  unsigned char speed; //0 to 99
  unsigned char delay; //0 to 99
  unsigned char pModDepth; //0 to 99
  unsigned char aModDepth; //0 to 99
  bool sync;
};

//---------------------------------------------------------
// Frequency
//---------------------------------------------------------
struct Frequency {
  double ratio;
  bool isFix; //if isFix no ratio but frequency
  double freq;
};

//---------------------------------------------------------
// Sensitivity
//  of the frequency and amplitude of the lfo
//  and the key velocity
//---------------------------------------------------------
struct Sensitivity {
  unsigned char pitch; //0 to 7
  unsigned char amplitude; //0 to 3
  bool ampOn[NBROP];
  unsigned char egBias[NBROP]; //O to 7
  unsigned char keyVelocity[NBROP]; //0 to 7
};

//---------------------------------------------------------
// OscWave
//---------------------------------------------------------
enum OscWave {
  W1, //sine wave
  W2, //sineÃ¯Â¿Â½ relative
  W3, //half sine
  W4, //half sineÃ¯Â¿Â½ relative
  W5,
  W6,
  W7,
  W8
};

enum egShiftValue {VOF, V48, V24, V12};

//---------------------------------------------------------
// Eg
//  Envelope
//---------------------------------------------------------
struct Eg {
  unsigned char ar; //0 to 31 speed attack
  unsigned char d1r; //0 to 31 speed decay
  unsigned char d1l; //0 to 15 level sustain
  unsigned char d2r; //0 to 31 speed of sustain
  unsigned char rr; //1 to 15
  egShiftValue egShift;
};

//---------------------------------------------------------
// PitchEg
//---------------------------------------------------------
struct PitchEg {
  unsigned char pr1;//0 to 99
  unsigned char pr2;//0 to 99
  unsigned char pr3;//0 to 99
  unsigned char pl1;//0 to 99
  unsigned char pl2;//0 to 99
  unsigned char pl3;//0 to 99
};

//---------------------------------------------------------
// Scaling
//---------------------------------------------------------
struct Scaling {
  unsigned char rate[NBROP];//0 to 3
  unsigned char level[NBROP];//0 to 99
};

//---------------------------------------------------------
// Mode
//---------------------------------------------------------
enum Mode {
  POLY,
  MONO
};

//---------------------------------------------------------
// Portamento
//---------------------------------------------------------
enum Portamento {
  FINGER,
  FULL
};

//---------------------------------------------------------
// FootSw
//---------------------------------------------------------
enum FootSw {
  POR,
  SUS
};
//---------------------------------------------------------
// Function
//---------------------------------------------------------
struct Function {
  int transpose;
  Mode mode;
  unsigned char pBendRange;//0 to 12
  Portamento portamento;
  unsigned char portamentoTime;//0 to 99
  FootSw footSw;
  unsigned char fcVolume;//0 to 99
  unsigned char fcPitch;//0 to 99
  unsigned char fcAmplitude;//0 to 99
  unsigned char mwPitch;//0 to 99
  unsigned char mwAmplitude;//0 to 99
  unsigned char bcPitch;//0 to 99
  unsigned char bcAmplitude;//0 to 99
  signed char bcPitchBias;//-50 to 50
  unsigned char bcEgBias;//0 to 99
  unsigned char atPitch;//0 to 99
  unsigned char atAmplitude;//0 to 99
  signed char atPitchBias;//-50 to 50
  unsigned char atEgBias;//0 to 99
  signed char reverbRate;//O=off, 1 to 7
};

//---------------------------------------------------------
// Preset class
//---------------------------------------------------------

class Preset {
 public:
    //Attributes
    Algorithm algorithm;
    unsigned char feedback; //0 to 7
    Lfo lfo;
    Sensitivity sensitivity;
    Frequency frequency[NBROP];
    OscWave oscWave[NBROP];
    signed char detune[NBROP]; //-3 to 3
    Eg eg[NBROP];
    PitchEg pitchEg;
    unsigned char outLevel[NBROP]; //0 to 99
    Scaling scaling;
    Function function;
    int globalDetune; //-15 to 15
    std::string name;//char name[LENGTHNAME+1];
    std::string subcategory;//char subcategory[LENGTHSUBCATEGORY+1];
    std::string category;//char category[LENGTHCATEGORY+1];
    int bank; //0 to 127
    int prog; //0 to 127
    //Methods
    void initPreset();
    //constructor
    //Preset(Preset* p_preset) {_preset=*p_preset;}
    //~Preset();
};


//---------------------------------------------------------
// EnvState
//---------------------------------------------------------

enum EnvState{
  ATTACK,
  DECAY,
  SUSTAIN,
  RELEASE,
  OFF
};

//---------------------------------------------------------
// OpVoice
//---------------------------------------------------------

struct OpVoice {
  double freq;
  double index;
  double inct;
  double amp; //between 0 and 1
  EnvState envState;
  double envIndex;
  double envInct;
  double envLevel;
  double coefVLevel;
};

//---------------------------------------------------------
// Voice
//---------------------------------------------------------

struct Voice {
    bool isOn;
    bool isSustained;
    int pitch;
    double volume;
    OpVoice op[NBROP];
    float sampleFeedback;
};

//---------------------------------------------------------
// Global
//---------------------------------------------------------

struct Global {
    float amp;
    float feedbackAmp;
    float lfoFreq;
    float lfoPitch;
    float lfoMaxCoefInct;
    float lfoCoefInct;
    float lfoCoefInctInct;
    unsigned int lfoIndex;
    unsigned int lfoMaxIndex;
    float lfoMaxAmp;
    float lfoMaxDAmp;
    float lfoAmp;
    float lfoCoefAmp;
    double lfoDelayIndex;
    double lfoDelayInct;
    double lfoDelayMaxIndex;
    bool delayPassed;
    bool sustain;
    double pitchBendCoef;//speed coef to read the sample
};

//---------------------------------------------------------------
// Bank, organized by a tree of category, subcategory, preset
//---------------------------------------------------------------
class presetSet {
 public:
    std::string _subcategoryName;
    std::vector<Preset*> _presetVector;
    Preset* findPreset(int lbank, int prog);
    void printSubcategory();
    presetSet(std::string name){_subcategoryName=name;}
    ~presetSet(){};
};

class subcategorySet {
 public:
    std::string _categoryName;
    std::vector<presetSet*> _subcategoryVector;
    presetSet* findPresetSet(std::string s);
    Preset* findPreset(int lbank, int prog);
    void printCategory();
    subcategorySet(const std::string name){_categoryName=name;}
    ~subcategorySet(){};
};

class categorySet {
 public:
    std::string _bankName;
    std::vector<subcategorySet*> _categoryVector;
    Preset* findPreset(int lbank, int prog);
    subcategorySet* findSubcategorySet(std::string s);
    void printBank();
    categorySet(const std::string name){_bankName=name;}
    ~categorySet(){};
};

//---------------------------------------------------------
//   DeicsOnze : DX11 emulator
//---------------------------------------------------------

class DeicsOnze : public Mess {
    DeicsOnzeGui* _gui;

    static int useCount;
    static float waveTable[NBRWAVES][RESOLUTION];

 public:

    Global _global;
    Voice _voices[NBRVOICES];
    Preset* _preset;

    mutable MidiPatch _patch;
    int _numPatch;

    //preset tree
    categorySet* _categorySet;

    /*subcategorySet* findSubcategorySet(std::string s);*/
    Preset* findPreset(int lbank, int prog);
    void initGlobal();
    void initVoices();
    void initPreset();
    void setPreset();
    void setFeedback();
    void setLfo();
    void loadSutulaPresets();
    int noteOff2Voice();
    int minVolu2Voice();
    int pitchOn2Voice(int pitch);
    void programSelect(int ch, int lbank, int prog);

    void setPitchBendCoef(int ch, int val);
    void setSustain(int ch, int val);
    virtual bool setController(int ch, int ctrl, int val);
    virtual const char* getPatchName(int ch, int number, int, bool) const;
    virtual const MidiPatch* getPatchInfo(int, const MidiPatch *) const;
    virtual bool playNote(int channel, int pitch, int velo);
    //virtual void processMessages();
    virtual void process(float** buffer, int offset, int n);

    virtual bool processEvent(const MidiPlayEvent&);
    // GUI interface routines
    virtual bool hasGui() const { return true; }
    virtual bool guiVisible() const;
    virtual void showGui(bool);
    virtual void getGeometry(int* x, int* y, int* w, int* h) const;
    virtual void setGeometry(int, int, int, int);

    void setMasterVol(int mv);
    int getMasterVol();

    DeicsOnze();
    ~DeicsOnze();
};


#endif /* __DEICSONZE_H */
