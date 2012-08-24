//===========================================================================
//
//    DeicsOnze an emulator of the YAMAHA DX11 synthesizer
//
//    Version 0.5.5
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
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
// 02111-1301, USA or point your web browser to http://www.gnu.org.
//===========================================================================


#ifndef __DEICSONZE_H
#define __DEICSONZE_H

#include <list>

#include "common_defs.h"
#include "deicsonzepreset.h"
#include "deicsonzegui.h"
#include "deicsonzeplugin.h"
#include "deicsonzefilter.h"
#include "libsynti/mess.h"
#include "plugin.h"

#define DEICSONZESTR "deicsonze"

#define MAXPITCHBENDVALUE 8191

#define RESOLUTION 96000

#define MAXFXBUFFERSIZE 48000
#define NBRFXINPUTS 2
#define NBRFXOUTPUTS 2

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
#define MAXSTRLENGTHFXLIB 256
#define MAXSTRLENGTHFXLABEL 256

#define DB_MIN 25.0

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
#define COEFPITCHENV 0.00000025 //adjust according to a real DX11....???
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
///#define SAVEINITLENGTH 2
#define SAVEINITLENGTH 4    // MFG ID, synth ID, init data command, init data version

#define DEICSONZECONFIGURATIONSTR "deicsOnzeConfiguation"
#define SYSEX_MASTERVOL 4
#define MASTERVOLSTR "MasterVolume"
#define MAXMASTERVOLUME 255
#define INITMASTERVOL 192
#define SYSEX_QUALITY 5
#define QUALITYSTR "Quality"
#define HIGHSTR "High"
#define MIDDLESTR "Middle"
#define LOWSTR "Low"
#define ULTRALOWSTR "UltraLow"
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
#define SYSEX_CHORUSACTIV 40
#define SYSEX_CHORUSPARAM 41
#define SYSEX_REVERBACTIV 60
#define SYSEX_REVERBPARAM 61
#define SYSEX_CHORUSRETURN 80
#define SYSEX_REVERBRETURN 81
#define MAXFXRETURN 255
#define INITFXRETURN 192
#define SYSEX_SELECTREVERB 82
#define SYSEX_SELECTCHORUS 83
#define SYSEX_BUILDGUIREVERB 84
#define SYSEX_BUILDGUICHORUS 85
#define SYSEX_FILTER 90
#define FILTERSTR "Filter"
#define SYSEX_DELAYACTIV 91
#define SYSEX_DELAYRETURN 92
#define SYSEX_DELAYBPM 93
#define SYSEX_DELAYBEATRATIO 94
#define SYSEX_DELAYFEEDBACK 95
#define SYSEX_DELAYLFOFREQ 96
#define SYSEX_DELAYLFODEPTH 97

//REVERB PARAMETERS

#define DEFAULTVOL 200

enum {
  NUM_MASTERVOL = SAVEINITLENGTH,
  NUM_CHANNEL_ENABLE,
  NUM_CHANNEL_VOL = NUM_CHANNEL_ENABLE + NBRCHANNELS + 1,
  NUM_CHANNEL_PAN = NUM_CHANNEL_VOL + NBRCHANNELS + 1,
  NUM_CHANNEL_BRIGHTNESS = NUM_CHANNEL_PAN + NBRCHANNELS + 1,
  NUM_CHANNEL_MODULATION = NUM_CHANNEL_BRIGHTNESS + 2*NBRCHANNELS +1,
  NUM_CHANNEL_DETUNE = NUM_CHANNEL_MODULATION + NBRCHANNELS + 1,
  NUM_CHANNEL_ATTACK = NUM_CHANNEL_DETUNE + NBRCHANNELS + 1,
  NUM_CHANNEL_RELEASE = NUM_CHANNEL_ATTACK + NBRCHANNELS + 1,
  NUM_CHANNEL_REVERB = NUM_CHANNEL_RELEASE + NBRCHANNELS + 1,
  NUM_CHANNEL_CHORUS = NUM_CHANNEL_REVERB + NBRCHANNELS + 1,  
  NUM_CHANNEL_DELAY = NUM_CHANNEL_CHORUS + NBRCHANNELS + 1,
  NUM_CURRENTPROG = NUM_CHANNEL_DELAY + NBRCHANNELS + 1,
  NUM_CURRENTLBANK = NUM_CURRENTPROG + NBRCHANNELS + 1,
  NUM_CURRENTHBANK = NUM_CURRENTLBANK + NBRCHANNELS + 1,
  NUM_NBRVOICES  = NUM_CURRENTHBANK + NBRCHANNELS + 1,
  NUM_SAVEONLYUSED  = NUM_NBRVOICES + NBRCHANNELS + 1,
  NUM_SAVECONFIG,
  NUM_RED_TEXT,
  NUM_GREEN_TEXT,
  NUM_BLUE_TEXT,
  NUM_RED_BACKGROUND,
  NUM_GREEN_BACKGROUND,
  NUM_BLUE_BACKGROUND,
  NUM_RED_EDITTEXT,
  NUM_GREEN_EDITTEXT,
  NUM_BLUE_EDITTEXT,
  NUM_RED_EDITBACKGROUND,
  NUM_GREEN_EDITBACKGROUND,
  NUM_BLUE_EDITBACKGROUND,
  NUM_QUALITY,
  NUM_FILTER,
  NUM_FONTSIZE,
  NUM_ISINITSET,
  NUM_INITSETPATH,
  NUM_ISBACKGROUNDPIX = NUM_INITSETPATH + MAXSTRLENGTHINITSETPATH + 1,
  NUM_BACKGROUNDPIXPATH,
  NUM_IS_REVERB_ON = NUM_BACKGROUNDPIXPATH + MAXSTRLENGTHBACKGROUNDPIXPATH + 1,
  NUM_REVERB_RETURN,
  NUM_REVERB_PARAM_NBR,
  NUM_REVERB_LIB,
  NUM_REVERB_LABEL = NUM_REVERB_LIB + MAXSTRLENGTHFXLIB + 1,
  NUM_IS_CHORUS_ON = NUM_REVERB_LABEL + MAXSTRLENGTHFXLABEL + 1,
  NUM_CHORUS_RETURN,
  NUM_CHORUS_PARAM_NBR,
  NUM_CHORUS_LIB,
  NUM_CHORUS_LABEL = NUM_CHORUS_LIB + MAXSTRLENGTHFXLIB + 1,
  NUM_IS_DELAY_ON = NUM_CHORUS_LABEL + MAXSTRLENGTHFXLABEL + 1,
  NUM_DELAY_RETURN,
  NUM_DELAY_BPM,
  NUM_DELAY_BEATRATIO = NUM_DELAY_BPM + sizeof(float),
  NUM_DELAY_FEEDBACK = NUM_DELAY_BEATRATIO + sizeof(float),
  NUM_DELAY_LFO_FREQ = NUM_DELAY_FEEDBACK + sizeof(float),
  NUM_DELAY_LFO_DEPTH = NUM_DELAY_LFO_FREQ + sizeof(float),
  NUM_CONFIGLENGTH = NUM_DELAY_LFO_DEPTH + sizeof(float)
};

class DeicsOnzeGui;
class DeicsOnzePlugin;

//---------------------------------------------------------
// outLevel2Amp, Amp for amplitude //between 0.0 and 2.0 or more
//  100->2.0, 90->1.0, 80->0.5 ...
//---------------------------------------------------------
inline double outLevel2Amp(int ol);

//---------------------------------------------------------
// level2amp, 
//  255->0dB->1.0, 0->-27dB->0
//---------------------------------------------------------
inline double level2amp(int l);

//---------------------------------------------------------
// amp2level
// 1.0->0dB->255, 0->-27dB->0
//---------------------------------------------------------
inline int amp2level(double amp);

//---------------------------------------------------------
// amp2lowlevel
// 1.0->0dB->127, 0->-27dB->0
//---------------------------------------------------------
inline int amp2lowlevel(double amp);

//---------------------------------------------------------
// lowlevel2amp, 
//  127->0dB->1.0, 0->-27dB->0
//---------------------------------------------------------
inline double lowlevel2amp(int l);

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
inline double envRR2coef(int rr, double sr, unsigned char release);

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
// PitchEnvState
//---------------------------------------------------------
enum PitchEnvState{
  PHASE1,
  PHASE2,
  RELEASE_PE,
  OFF_PE
};

//---------------------------------------------------------
// Voice
//---------------------------------------------------------

struct Voice {
  bool hasAttractor;//true iff the voice has an attractor (portamento occuring)
  double attractor; //contains some coeficent for portamento TODO
  PitchEnvState pitchEnvState;
  double pitchEnvCoefInct;
  double pitchEnvCoefInctPhase1;
  double pitchEnvCoefInctPhase2;
  double pitchEnvCoefInctPhase3;
  double pitchEnvCoefInctRelease;
  double pitchEnvCoefInctInct;
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
  std::list<int> lastVoiceKeyOn; //stack of the voice number
  int lastVoiceKeyOff;
  bool isLastNote;
  //FX
  float chorusAmount; //between 0.0 and 1.0
  float reverbAmount; //between 0.0 and 1.0
  float delayAmount; //between 0.0 and 1.0
};

//---------------------------------------------------------
// Global
//---------------------------------------------------------
enum Quality {
  high,
  middle,
  low,
  ultralow
};

struct Global {
  float masterVolume;
  Quality quality; //high, middle, low
  int qualityCounter; //counter to skip some sample depending on quality
  int qualityCounterTop; //number of sample - 1 to skip
  double deiSampleRate; //depending on quality deicsOnze sample rate varies
  bool filter; //low passe filter used when the sampling is low
  int fontSize;
  float lastLeftSample;
  float lastRightSample;
  float lastInputLeftChorusSample;
  float lastInputRightChorusSample;
  float lastInputLeftReverbSample;
  float lastInputRightReverbSample;
  float lastInputLeftDelaySample;
  float lastInputRightDelaySample;
  Channel channel[NBRCHANNELS];
  bool isChorusActivated;
  float chorusReturn;
  bool isReverbActivated;
  float reverbReturn;
  bool isDelayActivated;
  float delayReturn;
};

//---------------------------------------------------------
//   DeicsOnze : DX11 emulator
//---------------------------------------------------------

class DeicsOnze : public Mess {
  DeicsOnzeGui* _gui;
      
  unsigned char* initBuffer;
  int initLen;

  static int useCount;
  static float waveTable[NBRWAVES][RESOLUTION];

 private:
  void parseInitData(int length, const unsigned char* data);
  void loadConfiguration(QString fileName);
  void setupInitBuffer(int len);

 public:
  float** tempInputChorus;
  float** tempOutputChorus;
  float** tempInputReverb;
  float** tempOutputReverb;
  float** tempInputDelay;
  float** tempOutputDelay;

  float* getSinusWaveTable();

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

  //FX
  MusECore::PluginI* _pluginIReverb;
  MusECore::PluginI* _pluginIChorus;
  MusECore::PluginI* _pluginIDelay;

  void initPluginReverb(MusECore::Plugin*);
  void initPluginChorus(MusECore::Plugin*);
  void initPluginDelay(MusECore::Plugin*);
  
  void setReverbParam(int i, double val);
  double getReverbParam(int i) const;
  void setChorusParam(int i, double val);
  double getChorusParam(int i) const;
  void setDelayBPM(float val);
  void setDelayBeatRatio(float val);
  void setDelayFeedback(float val);
  void setDelayLFOFreq(float val);
  void setDelayLFODepth(float val);
  void setDelayDryWet(float val);
  float getDelayBPM() const;
  float getDelayBeatRatio() const;
  float getDelayFeedback() const;
  float getDelayLFOFreq() const;
  float getDelayLFODepth() const;

  //Filter
  LowFilter* _dryFilter;
  LowFilter* _chorusFilter;
  LowFilter* _reverbFilter;
  LowFilter* _delayFilter;

  mutable MidiPatch _patch;
  mutable int _numPatchProg; //used by getPatchInfo

  //preset tree 
  Set* _set;
  
  void setSampleRate(int sr);
  Preset* findPreset(int hbank, int lbank, int prog) const;
  Subcategory* findSubcategory(int hbank, int lbank) const;
  Category* findCategory(int hbank) const;
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
  void setPitchEnvRelease(int c, int v);
  void setQuality(Quality q);
  void setFilter(bool f);
  double brightness2Amp(int c, int k); //get the brightness of the operator k
  void loadSutulaPresets();
  void loadSet(QString s);
  int noteOff2Voice(int c); //return the first free voice
  int minVolu2Voice(int c);
  int pitchOn2Voice(int c, int pitch);
  void programSelect(int c, int hbank, int lbank, int prog);
  bool existsKeyOn(int ch);
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
  void setChannelReverb(int c, int r);
  void setChannelChorus(int c, int val);
  void setChannelDelay(int c, int val);
  void setChorusReturn(int val);
  void setReverbReturn(int val);
  void setDelayReturn(int val);
  bool getChannelEnable(int c) const;
  int getNbrVoices(int c) const;
  int getMasterVol(void) const;
  bool getFilter(void) const;
  int getChannelVol(int c) const;
  int getChannelPan(int c) const;
  int getChannelDetune(int c) const;
  int getChannelBrightness(int c) const;
  int getChannelModulation(int c) const;
  int getChannelAttack(int c) const;
  int getChannelRelease(int c) const;
  int getChannelReverb(int c) const;
  int getChannelChorus(int c) const;
  int getChannelDelay(int c) const;
  int getChorusReturn(void) const;
  int getReverbReturn(void) const;
  int getDelayReturn(void) const;
  void setPitchBendCoef(int c, int val);
  void setModulation(int c, int val); //TODO check between setChannelModulation
  void setSustain(int c, int val);

  void readConfiguration(QDomNode qdn);
  void writeConfiguration(AL::Xml* xml);

  bool setController(int ch, int ctrl, int val, bool fromGui);
  virtual bool setController(int ch, int ctrl, int val);
  bool sysex(int length, const unsigned char* data, bool fromGui); 
  virtual bool sysex(int l, const unsigned char* d);
  
  virtual const char* getPatchName(int ch, int number, int) const;
  virtual const MidiPatch* getPatchInfo(int, const MidiPatch *) const;

  virtual int getControllerInfo(int arg1, const char** arg2, 
				int* arg3, int* arg4, int* arg5, int* arg6) const;
  ///virtual void getInitData(int* length, const unsigned char** data) const;
  virtual void getInitData(int* length, const unsigned char** data);
  // This is only a kludge required to support old songs' midistates. Do not use in any new synth.
  virtual int oldMidiStateHeader(const unsigned char** data) const;

  virtual bool playNote(int channel, int pitch, int velo);
  virtual void processMessages();
  virtual void process(float** buffer, int offset, int n);
  
  // GUI interface routines
  //virtual bool hasGui() const { return true; }
  //virtual bool guiVisible() const;
  //virtual void showGui(bool);
  virtual bool hasNativeGui() const { return true; }
  virtual bool nativeGuiVisible() const;
  virtual void showNativeGui(bool);
  virtual void getNativeGeometry(int* x, int* y, int* w, int* h) const;
  virtual void setNativeGeometry(int, int, int, int);
  
  DeicsOnze();
  virtual ~DeicsOnze();
};


#endif /* __DEICSONZE_H */
