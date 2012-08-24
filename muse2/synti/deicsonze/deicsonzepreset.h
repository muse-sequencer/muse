//===========================================================================
//
//    DeicsOnze an emulator of the YAMAHA DX11 synthesizer
//
//    Version 0.5.5
//
//    deicsonzepreset.h
//
//
//  Copyright (c) 2004-2006 Nil Geisweiller
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
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
// 02111-1301, USA or point your web browser to http://www.gnu.org.
//===========================================================================

#ifndef __DEICSONZEPRESET_H
#define __DEICSONZEPRESET_H

#include <vector>
#include <string>
#include "al/xml.h"

#define NBROP 4 //do not change
#define MAXCHARTAG 64

#define PROG_NBR 128
#define LBANK_NBR 128
#define HBANK_NBR 128

//---------------------------------------------------------
// define strings of the parameter names for load save and ctrl interface
// number of ctrl
// following the internal DX11 organization (c.f T81Z manual)
//---------------------------------------------------------
#define CTRLOFFSET 0x100
#define DECAPAR1 13
#define ARSTR "AR"
#define ARLONGSTR "AttackRate"
#define CTRL_AR 0+CTRLOFFSET
#define MAXAR 31
#define D1RSTR "D1R"
#define D1RLONGSTR "Decay1Rate"
#define CTRL_D1R 1+CTRLOFFSET
#define MAXD1R 31
#define D2RSTR "D2R"
#define D2RLONGSTR "Decay2Rate"
#define CTRL_D2R 2+CTRLOFFSET
#define MAXD2R 31
#define RRSTR "RR"
#define RRLONGSTR "ReleaseRate"
#define CTRL_RR 3+CTRLOFFSET
#define MAXRR 15
#define D1LSTR "D1L"
#define D1LLONGSTR "Decay1Level"
#define CTRL_D1L 4+CTRLOFFSET
#define MAXD1L 15
#define LSSTR "LS"
#define LSLONGSTR "LevelScaling"
#define CTRL_LS 5+CTRLOFFSET
#define MAXLS 99
#define RSSTR "RS"
#define RSLONGSTR "RateScaling"
#define CTRL_RS 6+CTRLOFFSET
#define MAXRS 3
#define EBSSTR "EBS"
#define EBSLONGSTR "EGBiasSensitivity"
#define CTRL_EBS 7+CTRLOFFSET
#define MAXEBS 7
#define AMESTR "AME"
#define AMELONGSTR "AmplitudeModulationEnable"
#define CTRL_AME 8+CTRLOFFSET
#define KVSSTR "KVS"
#define KVSLONGSTR "KeyVelocitySensitivity"
#define CTRL_KVS 9+CTRLOFFSET
#define MAXKVS 7
#define OUTSTR "OUT"
#define OUTLONGSTR "OperatorOutputLevel"
#define CTRL_OUT 10+CTRLOFFSET
#define MAXOUT 99
#define RATIOSTR "Ratio"
#define RATIOLONGSTR "Ratio"
#define CTRL_RATIO 11+CTRLOFFSET
#define MAXRATIO 64
#define DETSTR "DET"
#define DETLONGSTR "Detune"
#define CTRL_DET 12+CTRLOFFSET
#define MAXDET 3
#define ALGSTR "ALG"
#define ALGLONGSTR "Algorithm"
#define CTRL_ALG 52+CTRLOFFSET
#define MAXALG 7
#define FEEDBACKSTR "Feedback"
#define CTRL_FEEDBACK 53+CTRLOFFSET
#define MAXFEEDBACK 7
#define SPEEDSTR "Speed"
#define SPEEDLONGSTR "LFOSpeed"
#define CTRL_SPEED 54+CTRLOFFSET
#define MAXSPEED 99
#define DELAYSTR "Delay" //TODO LFOD
#define DELAYLONGSTR "LFODelay"
#define CTRL_DELAY 55+CTRLOFFSET
#define MAXDELAY 99
#define PMODDEPTHSTR "PModDepth"
#define PMODDEPTHLONGSTR "PitchModulationDepth"
#define CTRL_PMODDEPTH 56+CTRLOFFSET
#define MAXPMODDEPTH 99
#define AMODDEPTHSTR "AModDepth"
#define AMODDEPTHLONGSTR "AmplitudeModulationDepth"
#define CTRL_AMODDEPTH 57+CTRLOFFSET
#define MAXAMODDEPTH 99
#define SYNCSTR "Sync"
#define SYNCLONGSTR "LFOSync"
#define CTRL_SYNC 58+CTRLOFFSET
#define WAVESTR "Wave"
#define WAVELONGSTR "LFOWave"
#define CTRL_WAVE 59+CTRLOFFSET
#define MAXWAVE 3
#define PMODSENSSTR "PModSens"
#define PMODSENSLONGSTR "PitchModulationSensitivity"
#define CTRL_PMODSENS 60+CTRLOFFSET
#define MAXPMODSENS 7
#define AMSSTR "AMS"
#define AMSLONGSTR "AmplitudeModulationSensitivity"
#define CTRL_AMS 61+CTRLOFFSET
#define MAXAMS 3
#define TRANSPOSESTR "Transpose"
#define CTRL_TRANSPOSE 62+CTRLOFFSET
#define MAXTRANSPOSE 24
#define POLYMODESTR "PolyMode"
#define CTRL_POLYMODE 63+CTRLOFFSET
#define PBENDRANGESTR "PBendRange"
#define PBENDRANGELONGSTR "PitchBendRange"
#define CTRL_PBENDRANGE 64+CTRLOFFSET
#define MAXPBENDRANGE 12
#define PORTAMODESTR "PortaMode"
#define PORTAMODELONGSTR "PortamentoMode"
#define CTRL_PORTAMODE 65+CTRLOFFSET
#define PORTATIMESTR "PortaTime"
#define PORTATIMELONGSTR "PortamentoTime"
#define CTRL_PORTATIME 66+CTRLOFFSET
#define MAXPROTATIME 99
#define FCVOLUMESTR "FCVolume"
#define FCVOLUMELONGSTR "FootControllerVolume"
#define CTRL_FCVOLUME 67+CTRLOFFSET
#define MAXFCVOLUME 99
#define FSWSTR "FSW"
#define FSWLONGSTR "FootSwitch"
#define CTRL_FSW 68+CTRLOFFSET
#define MAXFSW 99
#define MWPITCHSTR "MWPitch"
#define MWPITCHLONGSTR "ModulationWheelPitch"
#define CTRL_MWPITCH 71+CTRLOFFSET
#define MAXMWPITCH 99
#define MWAMPLITUDESTR "MWAmplitude"
#define MWAMPLITUDELONGSTR "ModulationWheelAmplitude"
#define CTRL_MWAMPLITUDE 72+CTRLOFFSET
#define MAXMWAMPLITUDE 99
#define BCPITCHSTR "BCPitch"
#define BCPITCHLONGSTR "BreathControlPitch"
#define CTRL_BCPITCH 73+CTRLOFFSET
#define MAXBCPITCH 99
#define BCAMPLITUDESTR "BCAmplitude"
#define BCAMPLITUDELONGSTR "BreathControlAmplitude"
#define CTRL_BCAMPLITUDE 74+CTRLOFFSET
#define MAXBCAMPLITUDE 99
#define BCPITCHBIASSTR "BCPitchBias"
#define BCPITCHBIASLONGSTR "BreathControlPitchBias"
#define CTRL_BCPITCHBIAS 75+CTRLOFFSET
#define MAXBCPITCHBIAS 50
#define BCEGBIASSTR "BCEGBias"
#define BCEGBIASLONGSTR "BreathControlEGBias"
#define CTRL_BCEGBIAS 76+CTRLOFFSET
#define MAXBCEGBIAS 99
#define MIDATTACK 64
#define ATPITCHSTR "ATPitch"
#define ATPITCHLONGSTR "AfterTouchPitch"
#define CTRL_ATPITCH 77+CTRLOFFSET
#define MAXATPITCH 99
#define ATAMPLITUDESTR "ATAmplitude"
#define ATAMPLITUDELONGSTR "AfterTouchAmplitude"
#define CTRL_ATAMPLITUDE 78+CTRLOFFSET
#define MAXATAMPLITUDE 99
#define ATPITCHBIASSTR "ATPitchBias"
#define ATPITCHBIASLONGSTR "AfterTouchPitchBias"
#define CTRL_ATPITCHBIAS 79+CTRLOFFSET
#define MAXATPITCHBIAS 50
#define ATEGBIASSTR "ATEGBias"
#define ATEGBIASLONGSTR "AfterTouchEGBias"
#define CTRL_ATEGBIAS 80+CTRLOFFSET
#define MAXATEGBIAS 99
#define PR1STR "PR1"
#define PR1LONGSTR "PitchRateEG1"
#define CTRL_PR1 81+CTRLOFFSET
#define MAXPR 99
#define PR2STR "PR2"
#define PR2LONGSTR "PitchRateEG2"
#define CTRL_PR2 82+CTRLOFFSET
#define PR3STR "PR3"
#define PR3LONGSTR "PitchRateEG3"
#define CTRL_PR3 83+CTRLOFFSET
#define PL1STR "PL1"
#define PL1LONGSTR "PitchLevelEG1"
#define CTRL_PL1 84+CTRLOFFSET
#define MAXPL 99
#define PL2STR "PL2"
#define PL2LONGSTR "PitchLevelEG2"
#define CTRL_PL2 85+CTRLOFFSET
#define PL3STR "PL3"
#define PL3LONGSTR "PitchLevelEG3"
#define CTRL_PL3 86+CTRLOFFSET
#define DECAPAR2 5
#define FIXSTR "FIX"
#define FIXLONGSTR "FixedFrequency"
#define CTRL_FIX 100+CTRLOFFSET
#define FIXRANGESTR "FixRange"
#define FIXRANGELONGSTR "FixedFrequencyRange"
#define CTRL_FIXRANGE 101+CTRLOFFSET
#define MAXFIXRANGE 255
#define OSWSTR "OSW"
#define OSWLONGSTR "OperatorWaveform"
#define CTRL_OSW 103+CTRLOFFSET
#define MAXOSW 7
#define SHFTSTR "SHFT"
#define SHFTLONGSTR "EGShift"
#define CTRL_SHFT 104+CTRLOFFSET
#define MAXSHFT 3
#define REVERBRATESTR "ReverbRate"
#define CTRL_REVERBRATE 120+CTRLOFFSET
#define MAXREVERBRATE 7
#define FCPITCHSTR "FCPitch"
#define FCPITCHLONGSTR "FootControllerPitch"
#define CTRL_FCPITCH 121+CTRLOFFSET
#define MAXFCPITCH 99
#define FCAMPLITUDESTR "FCAmplitude"
#define FCAMPLITUDELONGSTR "FootControllerAmplitude"
#define CTRL_FCAMPLITUDE 122+CTRLOFFSET
#define MAXFCAMPLITUDE 99
#define CHANNELPANSTR "ChannelPan"
#define CTRL_CHANNELPAN 123+CTRLOFFSET
#define MAXCHANNELPAN 127
#define CHANNELDETUNESTR "ChannelDetune"
#define CTRL_CHANNELDETUNE 124+CTRLOFFSET
#define MAXCHANNELDETUNE 63
#define CHANNELVOLUMESTR "ChannelVolume"
#define CTRL_CHANNELVOLUME 125+CTRLOFFSET
#define MAXCHANNELVOLUME 255
#define FINEBRIGHTNESSSTR "FineBrightness"
#define CTRL_FINEBRIGHTNESS 126+CTRLOFFSET
#define MAXFINEBRIGHTNESS 4095
#define MIDFINEBRIGHTNESS (MAXFINEBRIGHTNESS+1)/2
#define BRIGHTNESSSTR "Brightness"
#define MAXBRIGHTNESS 127
#define MIDBRIGHTNESS 64
#define MAXMODULATION 127
#define MODULATIONSTR "Modulation"
#define ATTACKSTR "Attack"
#define MAXATTACK 127
#define MIDATTACK 64
#define RELEASESTR "Attack"
#define MAXRELEASE 127
#define MIDRELEASE 64
#define NBRVOICESSTR "NumberOfVoices"
#define MINNBRVOICES 1
#define CTRL_NBRVOICES 127+CTRLOFFSET
#define CHANNELENABLESTR "ChannelEnable"
#define MAXCHANNELENABLE 1
#define MINCHANNELENABLE 0
#define CTRL_CHANNELENABLE 128+CTRLOFFSET

class Preset;
class Subcategory;
class Category;
class Set;

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
// Frequency
//---------------------------------------------------------
struct Frequency {
  double ratio;
  bool isFix; //if isFix no ratio but frequency
  double freq;
};

//---------------------------------------------------------
// OscWave
//---------------------------------------------------------
enum OscWave {
  W1, //sine wave
  W2, //sine� relative
  W3, //half sine
  W4, //half sine� relative
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
    Subcategory* _subcategory; //subcategory parent
    bool _isUsed; //false if the preset has never been used or modified,
                  //in this case the preset is not going to be
                  //save with the project
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
    //int globalDetune; //-31 to 31 //now to the channel
    std::string name;
    //unsigned char modulation; //0 to 127
    int prog; //0 to 127
    //Methods
    void printPreset();
    void initPreset();
    void readPreset(QDomNode qdn);
    void writePreset(AL::Xml* xml, bool onlyUsed);
    void linkSubcategory(Subcategory* sub);
    void merge(Preset* p); //copy the data of p in the preset
    void setIsUsed(bool b); //set flag _isUsed and transmit in the parents
    void getHBankLBankProg(int* h, int* l, int* p); //return the hbank, lbank and prog of the preset
    //Constructor destructor
    Preset();
    Preset(Subcategory* sub);
    Preset(Subcategory* sub, int prog);
    ~Preset();
};

//---------------------------------------------------------------
// Bank, organized by a tree of category, subcategory, preset
//---------------------------------------------------------------
class Subcategory {
 public:
    Category* _category;//parent category
    bool _isUsed; //false if the subcategory has never been used or modified,
                  //in this case the subcategory is not going to be
                  //save with the project
    std::string _subcategoryName;
    int _lbank; //0 to 127
    std::vector<Preset*> _presetVector;
    Preset* findPreset(int prog);
    void readSubcategory(QDomNode subNode);
    void writeSubcategory(AL::Xml* xml, bool onlyUsed);
    void printSubcategory();
    void linkCategory(Category* cat);
    void unlink();
    bool isFreeProg(int prog);
    int firstFreeProg();
    void merge(Preset*);
    //Constructor destructor
    Subcategory();
    Subcategory(Category* cat);
    Subcategory(const std::string name);
    Subcategory(Category* cat, const std::string name, int lbank);
    ~Subcategory();
};

class Category {
 public:
    Set* _set;//parent set
    bool _isUsed; //false if the category has never been used or modified,
                  //in this case the category is not going to be
                  //save with the project
    std::string _categoryName;
    int _hbank; //0 to 127
    std::vector<Subcategory*> _subcategoryVector;
    Subcategory* findSubcategory(int lbank);
    Preset* findPreset(int lbank, int prog);
    void readCategory(QDomNode catNode);
    void writeCategory(AL::Xml* xml, bool onlyUsed);
    void printCategory();
    void linkSet(Set* s);
    void unlink();
    bool isFreeLBank(int lbank);
    int firstFreeLBank(); //return -1 if no free
    void merge(Subcategory*);
    //Constructor Destructor
    Category();
    Category(Set* s);
    Category(Set* s,const std::string name, int hbank);
    ~Category();
};

class Set {
 public:
    std::string _setName;
    std::vector<Category*> _categoryVector;
    Preset* findPreset(int hbank, int lbank, int prog);
    Subcategory* findSubcategory(int hbank, int lbank);
    Category* findCategory(int hbank);
    void readSet(QDomNode setNode);
    void writeSet(AL::Xml* xml, bool onlyUsed);
    void printSet();
    bool isFreeHBank(int hbank);
    int firstFreeHBank();
    void merge(Category*);
    //Constructor Destructor
    Set(const std::string name){_setName=name;}
    ~Set() {
	while(!_categoryVector.empty()) delete(*_categoryVector.begin());
    }
};

#endif /* __DEICSONZE_H */
