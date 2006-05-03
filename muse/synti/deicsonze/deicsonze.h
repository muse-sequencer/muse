//===========================================================================
//
//    DeicsOnze an emulator of the YAMAHA DX11 synthesizer
//
//    Version 0.3
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

#define DEICSONZESTR "deicsonze-0.3"

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
#define MAXSTRLENGTHINITSETPATH 128

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
#define COEFGATTACK 0.9
#define COEFGRELEASE 0.9
#define LEVELSCALENOTE 24.0

#define NBRWAVES 8 //number wave forms, do not change
#define NBRBANKPRESETS 32

#define SYSEX_INIT_DATA 1
#define SYSEX_INIT_DATA_VERSION 1
#define SAVEINITLENGTH 2

#define DEICSONZECONFIGURATIONSTR "deicsOnzeConfiguation"
#define SYSEX_QUALITY 5
#define QUALITYSTR "Quality"
#define HIGHSTR "High"
#define MIDDLESTR "Middle"
#define LOWSTR "Low"
#define SYSEX_CHANNELNUM 6
#define CHANNELNUMSTR "ChannelNumber"
#define ALLSTR "All"
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
#define SYSEX_COLORGUI 20
#define TEXTCOLORSTR "TextColor"
#define BACKGROUNDCOLORSTR "BackgroundColor"
#define EDITTEXTCOLORSTR "EditTextColor"
#define EDITBACKGROUNDCOLORSTR "EditBackgroundColor"
#define COLORSYSEXLENGTH 12
#define SYSEX_UPDATESETGUI 25

#define NUMMASTERVOL SAVEINITLENGTH
#define NUMCURRENTPROG SAVEINITLENGTH+1
#define NUMCURRENTLBANK SAVEINITLENGTH+2
#define NUMCURRENTHBANK SAVEINITLENGTH+3
#define NUMSAVEONLYUSED SAVEINITLENGTH+4
#define NUMSAVECONFIG SAVEINITLENGTH+5
#define NUMNBRVOICES SAVEINITLENGTH+6
#define NUMCHANNELNUM SAVEINITLENGTH+7
#define SAVEGLOBALLENGTH 10

#define NUMREDTEXT SAVEINITLENGTH+SAVEGLOBALLENGTH
#define NUMGREENTEXT SAVEINITLENGTH+SAVEGLOBALLENGTH+1
#define NUMBLUETEXT SAVEINITLENGTH+SAVEGLOBALLENGTH+2
#define NUMREDBACKGROUND SAVEINITLENGTH+SAVEGLOBALLENGTH+3
#define NUMGREENBACKGROUND SAVEINITLENGTH+SAVEGLOBALLENGTH+4
#define NUMBLUEBACKGROUND SAVEINITLENGTH+SAVEGLOBALLENGTH+5
#define NUMREDEDITTEXT SAVEINITLENGTH+SAVEGLOBALLENGTH+6
#define NUMGREENEDITTEXT SAVEINITLENGTH+SAVEGLOBALLENGTH+7
#define NUMBLUEEDITTEXT SAVEINITLENGTH+SAVEGLOBALLENGTH+8
#define NUMREDEDITBACKGROUND SAVEINITLENGTH+SAVEGLOBALLENGTH+9
#define NUMGREENEDITBACKGROUND SAVEINITLENGTH+SAVEGLOBALLENGTH+10
#define NUMBLUEEDITBACKGROUND SAVEINITLENGTH+SAVEGLOBALLENGTH+11
#define NUMQUALITY SAVEINITLENGTH+SAVEGLOBALLENGTH+12
#define SAVECONFIGLENGTH 20

#define NUMISINITSET SAVEINITLENGTH+SAVEGLOBALLENGTH+SAVECONFIGLENGTH
#define NUMINITSETPATH SAVEINITLENGTH+SAVEGLOBALLENGTH+SAVECONFIGLENGTH+1

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
  double freq;
  double index;
  double inct;
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

enum Quality {
  high,
  middle,
  low
};

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
  Quality  quality; //0=high, 1=medium, 2=low
  unsigned char nbrVoices;
  char channelNum;//-1 to 15, -1 means all
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
  bool _saveOnlyUsed;
  bool _saveConfig;
  DeicsOnzeCtlr _ctrl[NBRCTRLS];
  Global _global;
  Voice _voices[MAXNBRVOICES];
  Preset* _preset;
  Preset* _initialPreset;
  
  mutable MidiPatch _patch;
  int _numPatch;
  
  //preset tree 
  Set* _set;
  
  Preset* findPreset(int hbank, int lbank, int prog);
  void initCtrls();
  void initGlobal();
  void initVoice(unsigned char v);
  void initVoices();
  void initPreset();
  void setPreset();
  void setFeedback();
  void setLfo();
  void setOutLevel(int k); //set the output level of the operator k
  void setOutLevel(); //do the same for all operators
  void setEnvAttack(int v, int k); //set envInct of voice v and operator k
  void setEnvAttack(int k); //do the same for all voices of operator k
  void setEnvAttack(); //do the same for all voices all operators
  void setEnvRelease(int v, int k); //set coefVLevel of voice v and operator k
  void setEnvRelease(int k); //do the same for all voices of operator k
  void setEnvRelease(); //do the same for all voices all operators  
  double brightness2Amp(int k); //get the brightness of the operator k
  //void loadSutulaPresets();
  void loadSet(QString s);
  int noteOff2Voice();
  int minVolu2Voice();
  int pitchOn2Voice(int pitch);
  void programSelect(int hbank, int lbank, int prog);
  
  void setNbrVoices(unsigned char nv);
  void setMasterVol(int mv);
  int getMasterVol(void);
  void setPitchBendCoef(int val);
  void setModulation(int val);
  void setSustain(int val);

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
