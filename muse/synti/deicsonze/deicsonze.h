//===========================================================================
//
//    DeicsOnze an emulator of the YAMAHA DX11 synthesizer
//
//    Version 0.4.1
//
//
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
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
// 02111-1307, USA or point your web browser to http://www.gnu.org.
//===========================================================================


#ifndef __DEICSONZE_H
#define __DEICSONZE_H


#include "deicsonzepreset.h"
#include "deicsonzegui.h"
#include "libsynti/mess.h"

#define DEICSONZESTR "deicsonze"

#define MAXPITCHBENDVALUE 8191

#define RESOLUTION   96000

#define NBRCTRLS 127

#define NBRPRESETS 128

#define LOWERNOTEFREQ 8.176

#define DB0LEVEL 90

#define LENGTHNAME 20
#define LENGTHCATEGORY 20
#define LENGTHSUBCATEGORY 20

#define MAXVELO 127
#define MAXVOLUME 100.0
#define MAXSTRLENGTHINITSETPATH 256
#define MAXSTRLENGTHBACKGROUNDPIXPATH 256

//coef determined by ear to sound like the YAMAHA DX11
#define COEFFEEDBACK 0.3
#define COEFPLFO(x) (x==0?0.0:(x==1?0.06:(x==2?0.12:(x==3?0.25:(x==4?0.5:(x==5?0.9:(x==6?3.9:7.9))))))) //return pitch amplitude with respect to sensitivity pitch
#define COEFALFO(x) (x==0?0.0:(x==1?0.4:(x==2?0.9:1.0)))
#define MAX(x,y) (x<y?y:x)
#define COEFLEVEL 1.0//19.0
#define COEFMAXATTACK 7.5
#define COEFERRDECSUS 0.01 //for the transition between DECAY and SUSTAIN
#define COEFERRSUSREL 0.001 //from SUSTAIN or RELEASE until no sound
//#define ERRPORTA 0.001 //dectection to stop portamento
#define COEFPORTA 0.01 //adjusted such that 10 second/octave with max porta
#define COEFDECAY 1.0
#define COEFSUSTAIN 0.2
#define COEFRELEASE 1.0
#define COEFDETUNE 0.008
#define COEFLEVELSCALE 0.0005
#define COEFGATTACK 0.9
#define COEFGRELEASE 0.9
#define LEVELSCALENOTE 24.0

#define NBRWAVES 8 //number wave forms, do not change
#define NBRBANKPRESETS 32
#define MAXNBRVOICES 64
#define NBRCHANNELS 16

#define SYSEX_INIT_DATA 1
#define SYSEX_INIT_DATA_VERSION 1
#define SAVEINITLENGTH 2

#define DEICSONZECONFIGURATIONSTR "deicsOnzeConfiguation"
#define SYSEX_MASTERVOL 4
#define MASTERVOLSTR "MasterVolume"
#define MAXMASTERVOLUME 255
#define INITMASTERVOL 96
#define SYSEX_QUALITY 5
#define QUALITYSTR "Quality"
#define HIGHSTR "High"
#define MIDDLESTR "Middle"
#define LOWSTR "Low"
#define SYSEX_FONTSIZE 6
#define FONTSIZESTR "fontSize"
#define SYSEX_SAVECONFIG 7
#define SAVECONFIGSTR "SaveConfig"
#define SYSEX_SAVEONLYUSED 8
#define SAVEONLYUSEDSTR "SaveOnlyUsed"
#define YESSTRDEI "yes"
#define NOSTRDEI "no"
#define SYSEX_LOADSET 10
#define SYSEX_ISINITSET 12
#define ISINITSETSTR "IsInitSet"
#define SYSEX_INITSETPATH 13
#define INITSETPATHSTR "InitSetPath"
#define SYSEX_ISBACKGROUNDPIX 14
#define ISBACKGROUNDPIXSTR "IsBackgroundPix"
#define SYSEX_BACKGROUNDPIXPATH 15
#define BACKGROUNDPIXPATHSTR "backgroundPixPath"
#define SYSEX_COLORGUI 20
#define TEXTCOLORSTR "TextColor"
#define BACKGROUNDCOLORSTR "BackgroundColor"
#define EDITTEXTCOLORSTR "EditTextColor"
#define EDITBACKGROUNDCOLORSTR "EditBackgroundColor"
#define COLORSYSEXLENGTH 12
#define SYSEX_UPDATESETGUI 25
#define SYSEX_PANIC 30

enum {
  NUMMASTERVOL = SAVEINITLENGTH,
  NUMCHANNELENABLE,
  NUMCHANNELVOL = NUMCHANNELENABLE + NBRCHANNELS + 1,
  NUMCHANNELPAN = NUMCHANNELVOL + NBRCHANNELS + 1,
  NUMCHANNELBRIGHTNESS = NUMCHANNELPAN + NBRCHANNELS + 1,
  NUMCHANNELMODULATION = NUMCHANNELBRIGHTNESS + 2*NBRCHANNELS +1,
  NUMCHANNELDETUNE = NUMCHANNELMODULATION + NBRCHANNELS + 1,
  NUMCHANNELATTACK = NUMCHANNELDETUNE + NBRCHANNELS + 1,
  NUMCHANNELRELEASE = NUMCHANNELATTACK + NBRCHANNELS + 1,
  NUMCURRENTPROG = NUMCHANNELRELEASE + NBRCHANNELS + 1,
  NUMCURRENTLBANK = NUMCURRENTPROG + NBRCHANNELS + 1,
  NUMCURRENTHBANK = NUMCURRENTLBANK + NBRCHANNELS + 1,
  NUMNBRVOICES  = NUMCURRENTHBANK + NBRCHANNELS + 1,
  NUMSAVEONLYUSED  = NUMNBRVOICES + NBRCHANNELS + 1,
  NUMSAVECONFIG,
  NUMREDTEXT,
  NUMGREENTEXT,
  NUMBLUETEXT,
  NUMREDBACKGROUND,
  NUMGREENBACKGROUND,
  NUMBLUEBACKGROUND,
  NUMREDEDITTEXT,
  NUMGREENEDITTEXT,
  NUMBLUEEDITTEXT,
  NUMREDEDITBACKGROUND,
  NUMGREENEDITBACKGROUND,
  NUMBLUEEDITBACKGROUND,
  NUMQUALITY,
  NUMFONTSIZE,
  NUMISINITSET,
  NUMINITSETPATH,
  NUMISBACKGROUNDPIX = NUMINITSETPATH + MAXSTRLENGTHINITSETPATH +1,
  NUMBACKGROUNDPIXPATH,
  NUMCONFIGLENGTH = NUMBACKGROUNDPIXPATH + MAXSTRLENGTHBACKGROUNDPIXPATH + 1
};

class DeicsOnzeGui;

//---------------------------------------------------------
// outLevel2Amp, Amp for amplitude //between 0.0 and 2.0 or more
//  100->2.0, 90->1.0, 80->0.5 ...
//---------------------------------------------------------
inline double outLevel2Amp(int ol);

//---------------------------------------------------------
// envAR2s
//  return the time in second of the ATTACK duration
//---------------------------------------------------------
inline double envAR2s(int ar);

//---------------------------------------------------------
// coefAttack
//  convert the attack value to a coef for envInct
//---------------------------------------------------------
inline double coefAttack(unsigned char attack);

//---------------------------------------------------------
// envRR2coef
//  return the coefficient for the exponential decrease
//  with respect to rr and sampleRate, sr
//---------------------------------------------------------
inline double envRR2coef(int rr, int sr, unsigned char release);

//--------------------------------------------------------
// DeicsOnzeCtrl
//--------------------------------------------------------
struct DeicsOnzeCtlr
{
  std::string name;
  int num;
  int min, max;
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
  double index;
  double inct;
  double targetInct; //used if portamento
  double amp; //between 0 and 1
  double ampVeloNote; //keeps the ratio amplitude from velo2AmpR and note2Amp
                      //in order to change independently the output level
                      //after pressing the note
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
  bool hasAttractor;//true iff the voice has an attractor (portamento occuring)
  double attractor; //contains some coeficent for portamento TODO
  bool isOn;
  bool keyOn;
  bool isSustained;
  int pitch; //number of the note
  double volume;
  OpVoice op[NBROP];
  float sampleFeedback;
};

//---------------------------------------------------------
// Channel
//---------------------------------------------------------
struct Channel {
  bool isEnable;
  float ampLeft;
  float ampRight;
  int volume; //0 to 255
  int pan; //TODO -63 +64 or -127 +128 
  int modulation;//0 to 127
  int detune;//-31 to 31
  int brightness; //0 to 4095
  int attack; //0 to 127
  int release; //0 to 127    
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
  unsigned char nbrVoices;
  Voice voices[MAXNBRVOICES];
  double lastInc[NBROP];
  bool isLastNote;
};

//---------------------------------------------------------
// Global
//---------------------------------------------------------
enum Quality {
  high,
  middle,
  low
};

struct Global {
  float masterVolume;
  Quality quality;
  int fontSize;
  Channel channel[NBRCHANNELS];
};

//---------------------------------------------------------
//   DeicsOnze : DX11 emulator
//---------------------------------------------------------

class DeicsOnze : public Mess {
  DeicsOnzeGui* _gui;
  
  static int useCount;
  static float waveTable[NBRWAVES][RESOLUTION];
  
 private:
  void parseInitData(int length, const unsigned char* data);
  void loadConfiguration(QString fileName);
  
 public:
  int nbrCtrl;
  
  QString _initSetPath;
  bool _isInitSet;
  QString _backgroundPixPath;
  bool _isBackgroundPix;
  bool _saveOnlyUsed;
  bool _saveConfig;
  DeicsOnzeCtlr _ctrl[NBRCTRLS];
  Global _global;
  Preset* _preset[NBRCHANNELS];
  Preset* _initialPreset;
  
  mutable MidiPatch _patch;
  int _numPatch; //what is this? TODO
  
  //preset tree 
  Set* _set;
  
  Preset* findPreset(int hbank, int lbank, int prog);
  void initCtrls();
  void initGlobal();
  void initChannels();
  void initChannel(int c);
  void resetVoices(); //when panic is pressed
  void initVoice(int c, int v);
  void initVoices(int c);
  void setPreset(int c);
  void setFeedback(int c);
  void setLfo(int c);
  void setOutLevel(int c, int k); //set the output level of the op k
  void setOutLevel(int c); //do the same for all operators
  void setEnvAttack(int c, int v, int k); //set envInct of voice v and op k
  void setEnvAttack(int c, int k); //do the same for all voices of operator k
  void setEnvAttack(int c); //do the same for all voices all operators
  void setEnvRelease(int c, int v, int k); //set coefVLevel of voice v and op k
  void setEnvRelease(int c, int k); //do the same for all voices of operator k
  void setEnvRelease(int c); //do the same for all voices all operators  
  double brightness2Amp(int c, int k); //get the brightness of the operator k
  void loadSutulaPresets();
  void loadSet(QString s);
  int noteOff2Voice(int c); //return the first free voice
  int minVolu2Voice(int c);
  int pitchOn2Voice(int c, int pitch);
  void programSelect(int c, int hbank, int lbank, int prog);
  
  void setNbrVoices(int c, int nv);
  void setMasterVol(int v);
  void setChannelEnable(int c, bool e);
  void setChannelVol(int c, int v);
  void setChannelPan(int c, int v);
  void applyChannelAmp(int c);
  void setChannelDetune(int c, int d);
  void setChannelBrightness(int c, int b);
  void setChannelModulation(int c, int m);
  void setChannelAttack(int c, int a);
  void setChannelRelease(int c, int r);
  bool getChannelEnable(int c) const;
  int getNbrVoices(int c) const;
  int getMasterVol(void) const;
  int getChannelVol(int c) const;
  int getChannelPan(int c) const;
  int getChannelDetune(int c) const;
  int getChannelBrightness(int c) const;
  int getChannelModulation(int c) const;
  int getChannelAttack(int c) const;
  int getChannelRelease(int c) const;
  void setPitchBendCoef(int c, int val);
  void setModulation(int c, int val); //TODO check between setChannelModulation
  void setSustain(int c, int val);

  void readConfiguration(QDomNode qdn);
  void writeConfiguration(AL::Xml* xml);

  bool setController(int ch, int ctrl, int val, bool fromGui);
  virtual bool setController(int ch, int ctrl, int val);
  bool sysex(int length, const unsigned char* data, bool fromGui); 
  virtual bool sysex(int length, const unsigned char* data);
  
  virtual const char* getPatchName(int ch, int number, int) const;
  virtual const MidiPatch* getPatchInfo(int, const MidiPatch *) const;
  virtual int getControllerInfo(int arg1, const char** arg2, 
				int* arg3, int* arg4, int* arg5) const;
  virtual void getInitData(int* length, const unsigned char** data) const;
  virtual bool playNote(int channel, int pitch, int velo);
  virtual void process(float** buffer, int offset, int n);
  
  // GUI interface routines
  virtual bool hasGui() const { return true; }
  virtual bool guiVisible() const;
  virtual void showGui(bool);
  virtual void getGeometry(int* x, int* y, int* w, int* h) const;
  virtual void setGeometry(int, int, int, int);
  
  DeicsOnze();
  ~DeicsOnze();
};


#endif /* __DEICSONZE_H */
