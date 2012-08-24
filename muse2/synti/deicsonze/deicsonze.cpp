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

// #include <cmath>
#include <list>

// #include <stdio.h>

#include <QDomDocument>
#include <QTemporaryFile>

#include "muse/midi.h"
#include "libsynti/mess.h"
//#include "common_defs.h"
#include "deicsonze.h"

#include "plugin.h"

#include "muse/midictrl.h"
//#include "deicsonze.h"
#include "config.h"

#define ABS(x) (x>=0?x:-x)


float DeicsOnze::waveTable[NBRWAVES][RESOLUTION];
int DeicsOnze::useCount = 0;

//---------------------------------------------------------
//   DeicsOnze
//---------------------------------------------------------

DeicsOnze::DeicsOnze() : Mess(2) {
  if (useCount++ == 0) {
    // create sinus wave table, W1
    for(int i = 0; i < RESOLUTION; i++)
      waveTable[W1][i] =
	(float)(sin((i * 2.0 * M_PI) / (double)RESOLUTION));
    // create sinus*abs(sinus) wave table, W2
    for(int i = 0; i < RESOLUTION; i++){
      double t = (i * 2.0 * M_PI) / (double)RESOLUTION;
      waveTable[W2][i] = (float)(ABS(sin(t))*sin(t));}
    // create halfsinus_ wave table, W3
    for(int i = 0; i < RESOLUTION; i++)
      waveTable[W3][i] = (float)
	(i<RESOLUTION/2?sin((i*2.0*M_PI)/(double)RESOLUTION):0.0);
    // create halfsinus*abs(sinus)_ wave table, W4
    for(int i = 0; i < RESOLUTION; i++){
      double t = (i * 2.0 * M_PI) / (double)RESOLUTION;
      waveTable[W4][i] = (float)(i<RESOLUTION/2?ABS(sin(t))*sin(t):0.0);}
    // create sinus_ wave table, W5
    for(int i = 0; i < RESOLUTION; i++)
      waveTable[W5][i] = (float)
	(i<RESOLUTION/2?sin((i*4.0*M_PI) / (double)RESOLUTION):0.0);
    // create sinus*abs(sinus)_ wave table, W6
    for(int i = 0; i < RESOLUTION; i++){
      double t = (i*4.0*M_PI) / (double)RESOLUTION;
      waveTable[W6][i] = (float)(i<RESOLUTION/2?ABS(sin(t))*sin(t):0.0);}
	// create 2halfsinus_ wave table, W7
    for(int i = 0; i < RESOLUTION; i++)
      waveTable[W7][i] = (float)
	(i<RESOLUTION/2?ABS(sin((i*4.0*M_PI)/(double)RESOLUTION)):0.0);
    // create 2halfsinus*abs(sinus)_ wave table, W8
    for(int i = 0; i < RESOLUTION; i++){
      double t = (i * 4.0 * M_PI) / (double)RESOLUTION;
      waveTable[W8][i] = (float)(i<RESOLUTION/2?sin(t)*sin(t):0.0);}
  }
  
  initBuffer  = 0;
  initLen     = 0;
      
  //alloc temp buffers chorus and reverb
  tempInputChorus = (float**) malloc(sizeof(float*)*NBRFXINPUTS);
  for(int i = 0; i < NBRFXINPUTS; i++)
    tempInputChorus[i] = (float*) malloc(sizeof(float*)*MAXFXBUFFERSIZE);
  tempOutputChorus = (float**) malloc(sizeof(float*)*NBRFXOUTPUTS);
  for(int i = 0; i < NBRFXOUTPUTS; i++)
    tempOutputChorus[i] = (float*) malloc(sizeof(float*)*MAXFXBUFFERSIZE);
  tempInputReverb = (float**) malloc(sizeof(float*)*NBRFXINPUTS);
  for(int i = 0; i < NBRFXINPUTS; i++)
    tempInputReverb[i] = (float*) malloc(sizeof(float*)*MAXFXBUFFERSIZE);
  tempOutputReverb = (float**) malloc(sizeof(float*)*NBRFXOUTPUTS);
  for(int i = 0; i < NBRFXOUTPUTS; i++)
    tempOutputReverb[i] = (float*) malloc(sizeof(float*)*MAXFXBUFFERSIZE);
  tempInputDelay = (float**) malloc(sizeof(float*)*NBRFXINPUTS);
  for(int i = 0; i < NBRFXINPUTS; i++)
    tempInputDelay[i] = (float*) malloc(sizeof(float*)*MAXFXBUFFERSIZE);
  tempOutputDelay = (float**) malloc(sizeof(float*)*NBRFXOUTPUTS);
  for(int i = 0; i < NBRFXOUTPUTS; i++)
    tempOutputDelay[i] = (float*) malloc(sizeof(float*)*MAXFXBUFFERSIZE);

  srand(time(0));   // initialize random number generator

  initCtrls();
  initGlobal();

  _numPatchProg = 0;
  _saveOnlyUsed = true;
  _saveConfig = true;
  _isInitSet = true; //false if an initial bank must be download
  
  QString sharePath(MusEGlobal::museGlobalShare);
  _initSetPath = sharePath + QString("/presets/deicsonze/SutulaBank.dei");
  
  
  //TODO
  //INSTPREFIX + "/share/" + PACKAGEVERSION + "/presets/deicsonze/ARCH_ALIN";
  _isBackgroundPix = true; //false if an initial bank must be download
  
  //"/usr/local/share/muse-1.0pre1/wallpapers/abstractdeicsonze1.jpg";
  _backgroundPixPath = sharePath + QString("/wallpapers/paper2.jpg");    // Tim.
  
  
  //initialization GUI
  _gui = new DeicsOnzeGui(this);
  _gui->hide();   // to avoid flicker during MusE startup
  _gui->setWindowTitle(QString("DeicsOnze"));

  //FX
  MusECore::Plugin* p;
  p = MusEGlobal::plugins.find("freeverb", "freeverb1");
  _pluginIReverb = NULL;
  if(p) initPluginReverb(p);
  _pluginIChorus = NULL;
  p = MusEGlobal::plugins.find("doublechorus", "doublechorus1");
  if(p) initPluginChorus(p);
  _pluginIDelay = NULL;
  p = MusEGlobal::plugins.find("pandelay", "pandelay");
  if(p) initPluginDelay(p);

  //Filter
  _dryFilter = new LowFilter();
  _chorusFilter = new LowFilter();
  _reverbFilter = new LowFilter();
  _delayFilter = new LowFilter();
  
  // Moved here from below due to crash - _preset not initialized when loadConfiguration called. Tim.
  _initialPreset = new 
    Preset(new Subcategory(new Category(NULL, "NONE", 0), "NONE", 0), 0);
  for(int c = 0; c < NBRCHANNELS; c++) {
    _preset[c]=_initialPreset;
    setPreset(c);
  }
  
  //Load configuration
  QString defaultConf = 
    (MusEGlobal::configPath + QString("/" DEICSONZESTR ".dco"));
  FILE* f;
  f = fopen(defaultConf.toAscii().data(), "r");
  if(f) {
    fclose(f);
    loadConfiguration(defaultConf);
  }
  
  //load Set
  _set=new Set("Initial Bank");
  if(_isInitSet) loadSet(_initSetPath);
  
  //loadSutulaPresets();
  
  // Moved above due to crash - _preset not initialized when loadConfiguration called. Tim.
  //_initialPreset = new 
  //  Preset(new Subcategory(new Category(NULL, "NONE", 0), "NONE", 0), 0);
  //for(int c = 0; c < NBRCHANNELS; c++) {
  //  _preset[c]=_initialPreset;
  //  setPreset(c);
  //}
  
  //update display gui
  //update mastervol
  unsigned char dataMasterVol[2];
  dataMasterVol[0]=SYSEX_MASTERVOL;
  dataMasterVol[1]=getMasterVol();
  MusECore::MidiPlayEvent evSysexMasterVol(0, 0, MusECore::ME_SYSEX, 
			     (const unsigned char*)dataMasterVol,
			     2);  
  _gui->writeEvent(evSysexMasterVol);
  //update return fx
  unsigned char *dataReverbRet = new unsigned char[2];
  dataReverbRet[0]=SYSEX_REVERBRETURN;
  dataReverbRet[1]=(unsigned char)getReverbReturn();
  MusECore::MidiPlayEvent evReverbRet(0, 0, MusECore::ME_SYSEX,(const unsigned char*)dataReverbRet, 2);
  _gui->writeEvent(evReverbRet);    
  unsigned char *dataChorusRet = new unsigned char[2];
  dataChorusRet[0]=SYSEX_CHORUSRETURN;
  dataChorusRet[1]=(unsigned char)getChorusReturn();
  MusECore::MidiPlayEvent evChorusRet(0, 0, MusECore::ME_SYSEX,(const unsigned char*)dataChorusRet, 2);
  _gui->writeEvent(evChorusRet);    
  unsigned char *dataDelayRet = new unsigned char[2];
  dataDelayRet[0]=SYSEX_DELAYRETURN;
  dataDelayRet[1]=(unsigned char)getDelayReturn();
  //printf("DELAY RET = %d, REVERB RET = %d\n",
  //getDelayReturn(), getReverbReturn());
  MusECore::MidiPlayEvent evDelayRet(0, 0, MusECore::ME_SYSEX,(const unsigned char*)dataDelayRet, 2);
  _gui->writeEvent(evDelayRet);    
  //update font size
  unsigned char *dataFontSize = new unsigned char[2];
  dataFontSize[0]=SYSEX_FONTSIZE;
  dataFontSize[1]=(unsigned char)_global.fontSize;
  MusECore::MidiPlayEvent evFontSize(0, 0, MusECore::ME_SYSEX, (const unsigned char*)dataFontSize, 2);
  _gui->writeEvent(evFontSize);
  //display load preset
  unsigned char dataUpdateGuiSet[1];
  dataUpdateGuiSet[0]=SYSEX_UPDATESETGUI;
  MusECore::MidiPlayEvent evSysexUpdateGuiSet(0, 0, MusECore::ME_SYSEX, 
				(const unsigned char*)dataUpdateGuiSet,
				1);
  _gui->writeEvent(evSysexUpdateGuiSet);
}

//---------------------------------------------------------
//   ~DeicsOnze
//---------------------------------------------------------

DeicsOnze::~DeicsOnze()
{
  if(_gui)        
    delete _gui;  // p4.0.27
    
  //if (--useCount == 0)
  //delete[] sine_table;
  //dealloc temp buffers chorus and reverb
  for(int i = 0; i < NBRFXINPUTS; i++) free(tempInputChorus[i]);
  free(tempInputChorus);
  for(int i = 0; i < NBRFXOUTPUTS; i++) free(tempOutputChorus[i]);
  free(tempOutputChorus);
  for(int i = 0; i < NBRFXINPUTS; i++) free(tempInputReverb[i]);
  free(tempInputReverb);
  for(int i = 0; i < NBRFXOUTPUTS; i++) free(tempOutputReverb[i]);
  free(tempOutputReverb);
  for(int i = 0; i < NBRFXINPUTS; i++) free(tempInputDelay[i]);
  free(tempInputDelay);
  for(int i = 0; i < NBRFXOUTPUTS; i++) free(tempOutputDelay[i]);
  free(tempOutputDelay);
  
  if (initBuffer)
        delete [] initBuffer;
}

int DeicsOnze::oldMidiStateHeader(const unsigned char** data) const 
{
  static unsigned char const d[2] = {MUSE_SYNTH_SYSEX_MFG_ID, DEICSONZE_UNIQUE_ID};
  *data = &d[0];
  return 2; 
}
        
//---------------------------------------------------------
// getSinusWaveTable
//---------------------------------------------------------
float* DeicsOnze::getSinusWaveTable() {
  return waveTable[W1];
}

//---------------------------------------------------------
//   guiVisible
//---------------------------------------------------------
bool DeicsOnze::nativeGuiVisible() const
{
    return _gui->isVisible();
}

//---------------------------------------------------------
// showGui
//---------------------------------------------------------
void DeicsOnze::showNativeGui(bool val)
{
    _gui->setVisible(val);
}

//---------------------------------------------------------
//   getNativeGeometry
//---------------------------------------------------------

void DeicsOnze::getNativeGeometry(int* x, int* y, int* w, int* h) const {
  QPoint pos(_gui->pos());
  QSize size(_gui->size());
  *x = pos.x();
  *y = pos.y();
  *w = size.width();
  *h = size.height();
}

void DeicsOnze::setSampleRate(int sr) {
  Mess::setSampleRate(sr);
  _dryFilter->setSamplerate(sr);
  _chorusFilter->setSamplerate(sr);
  _reverbFilter->setSamplerate(sr);
  _delayFilter->setSamplerate(sr);
  setQuality(_global.quality);
}

//---------------------------------------------------------
//   setNativeGeometry
//---------------------------------------------------------

void DeicsOnze::setNativeGeometry(int x, int y, int w, int h) {
    _gui->resize(QSize(w, h));
    _gui->move(QPoint(x, y));
}

//---------------------------------------------------------
// initCtrls
//---------------------------------------------------------
void DeicsOnze::initCtrls() {
    int i=0;
    for(int k=0; k<NBROP; k++) {
	_ctrl[i].name=(QString(ARSTR)+QString::number(k+1)).toAscii().data();
	_ctrl[i].num=CTRL_AR+k*DECAPAR1;
	_ctrl[i].min=0;
	_ctrl[i++].max=MAXAR;
	_ctrl[i].name=(QString(D1RSTR)+QString::number(k+1)).toAscii().data();
	_ctrl[i].num=CTRL_D1R+k*DECAPAR1;
	_ctrl[i].min=0;
	_ctrl[i++].max=MAXD1R;
	_ctrl[i].name=(QString(D2RSTR)+QString::number(k+1)).toAscii().data();
	_ctrl[i].num=CTRL_D2R+k*DECAPAR1;
	_ctrl[i].min=0;
	_ctrl[i++].max=MAXD2R;
	_ctrl[i].name=(QString(RRSTR)+QString::number(k+1)).toAscii().data();
	_ctrl[i].num=CTRL_RR+k*DECAPAR1;
	_ctrl[i].min=0;
	_ctrl[i++].max=MAXRR;
	_ctrl[i].name=(QString(D1LSTR)+QString::number(k+1)).toAscii().data();
	_ctrl[i].num=CTRL_D1L+k*DECAPAR1;
	_ctrl[i].min=0;
	_ctrl[i++].max=MAXD1L;
	_ctrl[i].name=(QString(LSSTR)+QString::number(k+1)).toAscii().data();
	_ctrl[i].num=CTRL_LS+k*DECAPAR1;
	_ctrl[i].min=0;
	_ctrl[i++].max=MAXLS;
	_ctrl[i].name=(QString(RSSTR)+QString::number(k+1)).toAscii().data();
	_ctrl[i].num=CTRL_RS+k*DECAPAR1;
	_ctrl[i].min=0;
	_ctrl[i++].max=MAXRS;
	_ctrl[i].name=(QString(EBSSTR)+QString::number(k+1)).toAscii().data();
	_ctrl[i].num=CTRL_EBS+k*DECAPAR1;
	_ctrl[i].min=0;
	_ctrl[i++].max=MAXEBS;
	_ctrl[i].name=(QString(AMESTR)+QString::number(k+1)).toAscii().data();
	_ctrl[i].num=CTRL_AME+k*DECAPAR1;
	_ctrl[i].min=0;
	_ctrl[i++].max=1;
	_ctrl[i].name=(QString(KVSSTR)+QString::number(k+1)).toAscii().data();
	_ctrl[i].num=CTRL_KVS+k*DECAPAR1;
	_ctrl[i].min=0;
	_ctrl[i++].max=MAXKVS;
	_ctrl[i].name=(QString(OUTSTR)+QString::number(k+1)).toAscii().data();
	_ctrl[i].num=CTRL_OUT+k*DECAPAR1;
	_ctrl[i].min=0;
	_ctrl[i++].max=MAXOUT;
	_ctrl[i].name=(QString("Centi")+QString(RATIOLONGSTR)+QString::number(k+1))
	    .toAscii().data();
	_ctrl[i].num=CTRL_RATIO+k*DECAPAR1;
	_ctrl[i].min=0;
	_ctrl[i++].max=MAXRATIO*100;
	_ctrl[i].name=(QString(DETSTR)+QString::number(k+1)).toAscii().data();
	_ctrl[i].num=CTRL_DET+k*DECAPAR1;
	_ctrl[i].min=-MAXDET;
	_ctrl[i++].max=MAXDET;	
    }
    _ctrl[i].name=ALGSTR;
    _ctrl[i].num=CTRL_ALG;
    _ctrl[i].min=0;
    _ctrl[i++].max=MAXALG;
    _ctrl[i].name=FEEDBACKSTR;
    _ctrl[i].num=CTRL_FEEDBACK;
    _ctrl[i].min=0;
    _ctrl[i++].max=MAXFEEDBACK;
    _ctrl[i].name=SPEEDSTR;
    _ctrl[i].num=CTRL_SPEED;
    _ctrl[i].min=0;
    _ctrl[i++].max=MAXSPEED;
    _ctrl[i].name=DELAYSTR;
    _ctrl[i].num=CTRL_DELAY;
    _ctrl[i].min=0;
    _ctrl[i++].max=MAXDELAY;
    _ctrl[i].name=PMODDEPTHSTR;
    _ctrl[i].num=CTRL_PMODDEPTH;
    _ctrl[i].min=0;
    _ctrl[i++].max=MAXPMODDEPTH;
    _ctrl[i].name=AMODDEPTHSTR;
    _ctrl[i].num=CTRL_AMODDEPTH;
    _ctrl[i].min=0;
    _ctrl[i++].max=MAXAMODDEPTH;
    _ctrl[i].name=SYNCSTR;
    _ctrl[i].num=CTRL_SYNC;
    _ctrl[i].min=0;
    _ctrl[i++].max=1;
    _ctrl[i].name=WAVESTR;
    _ctrl[i].num=CTRL_WAVE;
    _ctrl[i].min=0;
    _ctrl[i++].max=MAXWAVE;
    _ctrl[i].name=PMODSENSSTR;
    _ctrl[i].num=CTRL_PMODSENS;
    _ctrl[i].min=0;
    _ctrl[i++].max=MAXPMODSENS;
    _ctrl[i].name=AMSSTR;
    _ctrl[i].num=CTRL_AMS;
    _ctrl[i].min=0;
    _ctrl[i++].max=MAXAMS;
    _ctrl[i].name=TRANSPOSESTR;
    _ctrl[i].num=CTRL_TRANSPOSE;
    _ctrl[i].min=0;
    _ctrl[i++].max=MAXTRANSPOSE;
    _ctrl[i].name=POLYMODESTR;
    _ctrl[i].num=CTRL_POLYMODE;
    _ctrl[i].min=0;
    _ctrl[i++].max=1;
    _ctrl[i].name=PBENDRANGESTR;
    _ctrl[i].num=CTRL_PBENDRANGE;
    _ctrl[i].min=0;
    _ctrl[i++].max=MAXPBENDRANGE;
    _ctrl[i].name=PORTAMODESTR;
    _ctrl[i].num=CTRL_PORTAMODE;
    _ctrl[i].min=0;
    _ctrl[i++].max=1;
    _ctrl[i].name=PORTATIMESTR;
    _ctrl[i].num=CTRL_PORTATIME;
    _ctrl[i].min=0;
    _ctrl[i++].max=MAXPROTATIME;
    _ctrl[i].name=FCVOLUMESTR;
    _ctrl[i].num=CTRL_FCVOLUME;
    _ctrl[i].min=0;
    _ctrl[i++].max=MAXFCVOLUME;
    _ctrl[i].name=FSWSTR;
    _ctrl[i].num=CTRL_FSW;
    _ctrl[i].min=0;
    _ctrl[i++].max=MAXFSW;
    _ctrl[i].name=MWPITCHSTR;
    _ctrl[i].num=CTRL_MWPITCH;
    _ctrl[i].min=0;
    _ctrl[i++].max=MAXMWPITCH;
    _ctrl[i].name=MWAMPLITUDESTR;
    _ctrl[i].num=CTRL_MWAMPLITUDE;
    _ctrl[i].min=0;
    _ctrl[i++].max=MAXMWAMPLITUDE;
    _ctrl[i].name=BCPITCHSTR;
    _ctrl[i].num=CTRL_BCPITCH;
    _ctrl[i].min=0;
    _ctrl[i++].max=MAXBCPITCH;
    _ctrl[i].name=BCAMPLITUDESTR;
    _ctrl[i].num=CTRL_BCAMPLITUDE;
    _ctrl[i].min=0;
    _ctrl[i++].max=MAXBCAMPLITUDE;
    _ctrl[i].name=BCPITCHBIASSTR;
    _ctrl[i].num=CTRL_BCPITCHBIAS;
    _ctrl[i].min=-MAXBCPITCHBIAS;
    _ctrl[i++].max=MAXBCPITCHBIAS;
    _ctrl[i].name=BCEGBIASSTR;
    _ctrl[i].num=CTRL_BCEGBIAS;
    _ctrl[i].min=0;
    _ctrl[i++].max=MAXBCEGBIAS;
    _ctrl[i].name=ATPITCHSTR;
    _ctrl[i].num=CTRL_ATPITCH;
    _ctrl[i].min=0;
    _ctrl[i++].max=MAXATPITCH;
    _ctrl[i].name=ATAMPLITUDESTR;
    _ctrl[i].num=CTRL_ATAMPLITUDE;
    _ctrl[i].min=0;
    _ctrl[i++].max=MAXATAMPLITUDE;
    _ctrl[i].name=ATPITCHBIASSTR;
    _ctrl[i].num=CTRL_ATPITCHBIAS;
    _ctrl[i].min=-MAXATPITCHBIAS;
    _ctrl[i++].max=MAXATPITCHBIAS;
    _ctrl[i].name=ATEGBIASSTR;
    _ctrl[i].num=CTRL_ATEGBIAS;
    _ctrl[i].min=0;
    _ctrl[i++].max=MAXATEGBIAS;
    _ctrl[i].name=PR1STR;
    _ctrl[i].num=CTRL_PR1;
    _ctrl[i].min=0;
    _ctrl[i++].max=MAXPR;
    _ctrl[i].name=PR2STR;
    _ctrl[i].num=CTRL_PR2;
    _ctrl[i].min=0;
    _ctrl[i++].max=MAXPR;
    _ctrl[i].name=PR3STR;
    _ctrl[i].num=CTRL_PR3;
    _ctrl[i].min=0;
    _ctrl[i++].max=MAXPR;
    _ctrl[i].name=PL1STR;
    _ctrl[i].num=CTRL_PL1;
    _ctrl[i].min=0;
    _ctrl[i++].max=MAXPL;
    _ctrl[i].name=PL2STR;
    _ctrl[i].num=CTRL_PL2;
    _ctrl[i].min=0;
    _ctrl[i++].max=MAXPL;
    _ctrl[i].name=PL3STR;
    _ctrl[i].num=CTRL_PL3;
    _ctrl[i].min=0;
    _ctrl[i++].max=MAXPL;
    for(int k=0; k<NBROP; k++) {
	_ctrl[i].name=(QString(FIXSTR)+QString::number(k+1)).toAscii().data();
	_ctrl[i].num=CTRL_FIX+k*DECAPAR2;
	_ctrl[i].min=0;
	_ctrl[i++].max=1;
	_ctrl[i].name=(QString("Centi")+QString(FIXRANGESTR)
		       +QString::number(k+1)).toAscii().data();
	_ctrl[i].num=CTRL_FIXRANGE+k*DECAPAR2;
	_ctrl[i].min=0;
	_ctrl[i++].max=MAXFIXRANGE*100;
	_ctrl[i].name=(QString(OSWSTR)+QString::number(k+1)).toAscii().data();
	_ctrl[i].num=CTRL_OSW+k*DECAPAR2;
	_ctrl[i].min=0;
	_ctrl[i++].max=MAXOSW;
    	_ctrl[i].name=(QString(SHFTSTR)+QString::number(k+1)).toAscii().data();
	_ctrl[i].num=CTRL_SHFT+k*DECAPAR2;
	_ctrl[i].min=0;
	_ctrl[i++].max=MAXSHFT;
    }	
    _ctrl[i].name=REVERBRATESTR;
    _ctrl[i].num=CTRL_REVERBRATE;
    _ctrl[i].min=0;
    _ctrl[i++].max=7;
    _ctrl[i].name=FCPITCHSTR;
    _ctrl[i].num=CTRL_FCPITCH;
    _ctrl[i].min=0;
    _ctrl[i++].max=MAXFCPITCH;
    _ctrl[i].name=FCAMPLITUDESTR;
    _ctrl[i].num=CTRL_FCAMPLITUDE;
    _ctrl[i].min=0;
    _ctrl[i++].max=MAXFCAMPLITUDE;
    _ctrl[i].name=CHANNELPANSTR;
    _ctrl[i].num=CTRL_CHANNELPAN;
    _ctrl[i].min=-MAXCHANNELPAN;
    _ctrl[i++].max=MAXCHANNELPAN;
    _ctrl[i].name=CHANNELDETUNESTR;
    _ctrl[i].num=CTRL_CHANNELDETUNE;
    _ctrl[i].min=-MAXCHANNELDETUNE;
    _ctrl[i++].max=MAXCHANNELDETUNE;
    _ctrl[i].name=CHANNELVOLUMESTR;
    _ctrl[i].num=CTRL_CHANNELVOLUME;
    _ctrl[i].min=0;
    _ctrl[i++].max=MAXCHANNELVOLUME;
    _ctrl[i].name=FINEBRIGHTNESSSTR;
    _ctrl[i].num=CTRL_FINEBRIGHTNESS;
    _ctrl[i].min=0;
    _ctrl[i++].max=MAXFINEBRIGHTNESS;
    _ctrl[i].name=NBRVOICESSTR;
    _ctrl[i].num=CTRL_NBRVOICES;
    _ctrl[i].min=0;
    _ctrl[i++].max=MAXNBRVOICES;
    nbrCtrl=i;
}

//---------------------------------------------------------
// initGlobal
//---------------------------------------------------------
void DeicsOnze::initGlobal() {
  setMasterVol(INITMASTERVOL);
  _global.quality = high;
  setFilter(false);
  _global.fontSize = 9;
  _global.isChorusActivated = false;
  _global.chorusReturn = level2amp(INITFXRETURN);
  _global.isReverbActivated = false;
  _global.reverbReturn = level2amp(INITFXRETURN);
  _global.isDelayActivated = false;
  _global.delayReturn = level2amp(INITFXRETURN);
  initChannels();
}

void DeicsOnze::initChannels() {
  for(int c=0; c<NBRCHANNELS; c++) initChannel(c);
  _global.channel[0].isEnable = true; //the first one is enable
}

void DeicsOnze::initChannel(int c) {
  _global.channel[c].isEnable = false;
  _global.channel[c].sustain = false;
  _global.channel[c].volume = DEFAULTVOL;
  _global.channel[c].pan = 0;
  _global.channel[c].modulation = 0;
  _global.channel[c].detune = 0;
  _global.channel[c].brightness = MIDFINEBRIGHTNESS;
  _global.channel[c].attack = MIDATTACK;
  _global.channel[c].release = MIDRELEASE;
  _global.channel[c].pitchBendCoef = 1.0;
  _global.channel[c].lfoIndex = 0;
  _global.channel[c].nbrVoices = 8;
  _global.channel[c].isLastNote = false;
  _global.channel[c].chorusAmount = 0.0;
  _global.channel[c].reverbAmount = 0.0;
  _global.channel[c].delayAmount = 0.0;
  applyChannelAmp(c);
  initVoices(c);
}

//---------------------------------------------------------
// resetVoices
//---------------------------------------------------------
void DeicsOnze::resetVoices() {
  for(int c = 0; c<NBRCHANNELS; c++) initVoices(c);
  //take care of this if initVoices() changes
}

//---------------------------------------------------------
// initVoice
//---------------------------------------------------------
void DeicsOnze::initVoice(int c /*channel*/, int v) {
  _global.channel[c].voices[v].hasAttractor = false;
  _global.channel[c].voices[v].isOn = false;
  _global.channel[c].voices[v].keyOn = false;
  _global.channel[c].voices[v].isSustained = false;
  _global.channel[c].voices[v].pitchEnvCoefInct = 1.0; 
  _global.channel[c].voices[v].pitchEnvCoefInctInct = 1.0;
  _global.channel[c].voices[v].pitchEnvState = OFF_PE;
 
}
//---------------------------------------------------------
// initVoices
//---------------------------------------------------------
void DeicsOnze::initVoices(int c) {
  for(int v=0; v<MAXNBRVOICES; v++) {
    initVoice(c, v);
    _global.channel[c].lastVoiceKeyOn.clear();
  }
}

//--------------------------------------------------------
// findPreset findSubcategory findCategory
//--------------------------------------------------------
Preset* DeicsOnze::findPreset(int hbank, int lbank, int prog) const {
  return _set->findPreset(hbank, lbank, prog);
}
Subcategory* DeicsOnze::findSubcategory(int hbank, int lbank) const {
  return _set->findSubcategory(hbank, lbank);
}
Category* DeicsOnze::findCategory(int hbank) const {
  return _set->findCategory(hbank);
}
//---------------------------------------------------------
// isPitchEnv
//  return true iff all levels are in the middle
//---------------------------------------------------------
inline bool isPitchEnv(PitchEg* pe) {
  return(pe->pl1 != 50 || pe->pl2 != 50 || pe->pl3 != 50);
}
//---------------------------------------------------------
// getPitchEnvCoefInct
//  returns the coefInct according to level pl
//---------------------------------------------------------
inline double getPitchEnvCoefInct(int pl) {
  /*
    pl = 0 <--> -4oct, pl = 50 <--> 0oct, pl = 100 <--> 4oct

    y = a * exp((pl - 50)/b)
    1.0 = a*exp(0) ==> a = 1.0
    8.0 = exp(50/b) ==> log 8.0 = 50/b ==> b = 50/log(8.0)
  */
  double b = 50.0/log(8.0);
  return exp((pl-50.0)/b);
}

//---------------------------------------------------------
// getPitchEnvCoefInctInct
//---------------------------------------------------------
inline double getPitchEnvCoefInctInct(int pl1, int pl2, int pr, double sr) {
  //TODO : depending on the sampleRate
  int a = pr;
  double c = 1.0 + COEFPITCHENV*((double)(a*a)+1.0);
  double inctInct = exp(log(c)*48000.0/sr);
  if(pl1<pl2) return(inctInct);
  else if(pl1>pl2)
    return(1.0/inctInct);
  else return 1.0;
}

//---------------------------------------------------------
// existsKeyOn
//---------------------------------------------------------
bool DeicsOnze::existsKeyOn(int ch) {
  return !_global.channel[ch].lastVoiceKeyOn.empty();
}

//---------------------------------------------------------
// note2Amp
//  return the Amp of a note depending on the level scaling
//---------------------------------------------------------
inline double note2Amp(double note, int ls)
{
    if(ls==0) return(1.0);
    else return((note<LEVELSCALENOTE?1.0:exp((double)ls*COEFLEVELSCALE*
					     ((double)LEVELSCALENOTE-note))));
}

//---------------------------------------------------------
// delay2Time
//  return the time in second corresponding to the LFO delay parameter
//---------------------------------------------------------
inline double delay2Time(int d) {
  double t;
  //fitting
  t=0.07617*(double)d-0.002695*(double)(d*d)+4.214e-05*(double)(d*d*d);
  //printf("delay2Time : %f\n", t);
  return(t);
}

//----------------------------------------------------------------
// setNbrVoices
//----------------------------------------------------------------
void DeicsOnze::setNbrVoices(int c, int nv) {
  nv=(nv>MAXNBRVOICES?MAXNBRVOICES:(nv<1?1:nv));
  //we assume that any voices
  //that is not included in the active voices is properly initialized
  for(int v=nv; v<_global.channel[c].nbrVoices; v++)
    initVoice(c, v); 
  _global.channel[c].nbrVoices=nv;
}

//----------------------------------------------------------------
// setMasterVol
//----------------------------------------------------------------
void DeicsOnze::setMasterVol(int mv) {
  _global.masterVolume=level2amp(mv); //watch out that MAXMASTERVOLUME==255
}
//----------------------------------------------------------------
// setChannelEnable
//----------------------------------------------------------------
void DeicsOnze::setChannelEnable(int c, bool e) {
  _global.channel[c].isEnable = e;
  setLfo(c);
}

//----------------------------------------------------------------
// setChannelVol
//----------------------------------------------------------------
void DeicsOnze::setChannelVol(int c, int v) {
  _global.channel[c].volume = v;
}

void DeicsOnze::applyChannelAmp(int c) {
  _global.channel[c].ampLeft = 
    level2amp(_global.channel[c].volume)
    * ((double)(MAXCHANNELPAN - _global.channel[c].pan)
       /(double)(2*MAXCHANNELPAN));
  _global.channel[c].ampRight =
    level2amp(_global.channel[c].volume)
    * ((double)(MAXCHANNELPAN + _global.channel[c].pan)
       /(double)(2*MAXCHANNELPAN));
}

//----------------------------------------------------------------
// setChannelPan
//----------------------------------------------------------------
void DeicsOnze::setChannelPan(int c, int p) {
  _global.channel[c].pan = p;
}
//----------------------------------------------------------------
// setChannelDetune
//----------------------------------------------------------------
void DeicsOnze::setChannelDetune(int c, int p) {
  _global.channel[c].detune = p;
}
//----------------------------------------------------------------
// setChannelBrightness
//----------------------------------------------------------------
void DeicsOnze::setChannelBrightness(int c, int b) {
  _global.channel[c].brightness = b;
}
//----------------------------------------------------------------
// setChannelModulation
//----------------------------------------------------------------
void DeicsOnze::setChannelModulation(int c, int m) {
  _global.channel[c].modulation = m;
}
//----------------------------------------------------------------
// setChannelAttack
//----------------------------------------------------------------
void DeicsOnze::setChannelAttack(int c, int a) {
  _global.channel[c].attack = a;
}
//----------------------------------------------------------------
// setChannelRelease
//----------------------------------------------------------------
void DeicsOnze::setChannelRelease(int c, int r) {
  _global.channel[c].release = r;
}
//----------------------------------------------------------------
// setChannelReverb
//----------------------------------------------------------------
void DeicsOnze::setChannelReverb(int c, int r) {
  _global.channel[c].reverbAmount = (float)lowlevel2amp(r);
}
//----------------------------------------------------------------
// setChannelChorus
//----------------------------------------------------------------
void DeicsOnze::setChannelChorus(int c, int val) {
  _global.channel[c].chorusAmount = (float)lowlevel2amp(val);
}
//----------------------------------------------------------------
// setChannelDelay
//----------------------------------------------------------------
void DeicsOnze::setChannelDelay(int c, int val) {
  _global.channel[c].delayAmount = (float)lowlevel2amp(val);
}

//----------------------------------------------------------------
// setChorusReturn
//----------------------------------------------------------------
void DeicsOnze::setChorusReturn(int val) {
  _global.chorusReturn = 2.0*(float)level2amp(val); //beware MAXFXRETURN==255
}

//----------------------------------------------------------------
// setReverbReturn
//----------------------------------------------------------------
void DeicsOnze::setReverbReturn(int val) {
  _global.reverbReturn = 2.0*(float)level2amp(val); //beware MAXFXRETURN==255
}

//----------------------------------------------------------------
// setDelayReturn
//----------------------------------------------------------------
void DeicsOnze::setDelayReturn(int val) {
  _global.delayReturn = 2.0*(float)level2amp(val); //beware MAXFXRETURN==255
}

//----------------------------------------------------------------
// getNbrVoices
//----------------------------------------------------------------
int DeicsOnze::getNbrVoices(int c) const {
  return(_global.channel[c].nbrVoices);
}
//----------------------------------------------------------------
// getMasterVol
//----------------------------------------------------------------
int DeicsOnze::getMasterVol(void) const {
  return(amp2level(_global.masterVolume));
}
//----------------------------------------------------------------
// getFilter
//----------------------------------------------------------------
bool DeicsOnze::getFilter(void) const {
  return _global.filter;
}
//----------------------------------------------------------------
// getChannelEnable
//----------------------------------------------------------------
bool DeicsOnze::getChannelEnable(int c) const {
  return _global.channel[c].isEnable;
}

//----------------------------------------------------------------
// getChannelVol
//----------------------------------------------------------------
int DeicsOnze::getChannelVol(int c) const { //TODO : to see if correct
  //return((int)(MAX(_global.channel[c].ampLeft, _global.channel[c].ampRight)
  //*(double)MAXCHANNELVOLUME));
  return(_global.channel[c].volume);
}
//----------------------------------------------------------------
// getChannelPan
//----------------------------------------------------------------
int DeicsOnze::getChannelPan(int c) const {
  return(_global.channel[c].pan);
}
//----------------------------------------------------------------
// setChannelDetune
//----------------------------------------------------------------
int DeicsOnze::getChannelDetune(int c) const {
    return _global.channel[c].detune;
}
//----------------------------------------------------------------
// getChannelBrightness
//----------------------------------------------------------------
int DeicsOnze::getChannelBrightness(int c) const {
  return(_global.channel[c].brightness);
}
//----------------------------------------------------------------
// getChannelModulation
//----------------------------------------------------------------
int DeicsOnze::getChannelModulation(int c) const {
  return(_global.channel[c].modulation);
}
//----------------------------------------------------------------
// getChannelAttack
//----------------------------------------------------------------
int DeicsOnze::getChannelAttack(int c) const {
  return(_global.channel[c].attack);
}
//----------------------------------------------------------------
// getChannelRelease
//----------------------------------------------------------------
int DeicsOnze::getChannelRelease(int c) const {
  return(_global.channel[c].release);
}
//----------------------------------------------------------------
// getChannelReverb
//----------------------------------------------------------------
int DeicsOnze::getChannelReverb(int c) const {
  return(amp2lowlevel(_global.channel[c].reverbAmount));
}
//----------------------------------------------------------------
// getChannelChorus
//----------------------------------------------------------------
int DeicsOnze::getChannelChorus(int c) const {
  return(amp2lowlevel(_global.channel[c].chorusAmount));
}
//----------------------------------------------------------------
// getChannelDelay
//----------------------------------------------------------------
int DeicsOnze::getChannelDelay(int c) const {
  return(amp2lowlevel(_global.channel[c].delayAmount));
}
//----------------------------------------------------------------
// getChorusReturn
//----------------------------------------------------------------
int DeicsOnze::getChorusReturn() const {
  return(amp2level(_global.chorusReturn/2.0));
}
//----------------------------------------------------------------
// getReverbReturn
//----------------------------------------------------------------
int DeicsOnze::getReverbReturn() const {
  return(amp2level(_global.reverbReturn/2.0));
}
//----------------------------------------------------------------
// getReverbReturn
//----------------------------------------------------------------
int DeicsOnze::getDelayReturn() const {
  return(amp2level(_global.delayReturn/2.0));
}

//----------------------------------------------------------------
// setLfo
//----------------------------------------------------------------
void DeicsOnze::setLfo(int c/*channel*/)
{
    double x;
    x=(double)_preset[c]->lfo.speed;
    // lfoSpeed to Hz, obtained by fitting the actual curve by a polynomial
    _global.channel[c].lfoFreq =
      -1.9389e-08*x*x*x*x*x+2.8826e-06*x*x*x*x-9.0316e-05*x*x*x
      +4.7453e-03*x*x-1.2295e-02*x+7.0347e-02;//a revoir
    //Pitch LFO
    _global.channel[c].lfoMaxIndex =
      (_global.channel[c].lfoFreq==0?0:(int)((1.0/_global.channel[c].lfoFreq)
				  *(double)_global.deiSampleRate));
    double totalpDepth = 
      ((double)_preset[c]->lfo.pModDepth +
       (((double)_global.channel[c].modulation)/127.0)
       * ((double)(MAXPMODDEPTH - _preset[c]->lfo.pModDepth))
       )/(double)MAXPMODDEPTH;
    _global.channel[c].lfoPitch =
      totalpDepth * (COEFPLFO(_preset[c]->sensitivity.pitch));
    //Amplitude LFO
    double totalaDepth = 
      ((double)_preset[c]->lfo.aModDepth +
       (((double)_global.channel[c].modulation)/127.0)
       * ((double)(MAXAMODDEPTH - _preset[c]->lfo.aModDepth))
       )/(double)MAXAMODDEPTH;
    _global.channel[c].lfoMaxAmp =
      totalaDepth * (COEFALFO(_preset[c]->sensitivity.amplitude));
    //index is concidered on the half of the frequency of the LFO
    _global.channel[c].lfoDelayMaxIndex = 
      delay2Time(_preset[c]->lfo.delay)*_global.channel[c].lfoFreq*2;
    _global.channel[c].lfoDelayInct = 
      (double)(RESOLUTION/4)/_global.channel[c].lfoDelayMaxIndex;
    
    //update the actuall values controlling the modulation now
    if(_global.channel[c].lfoDelayIndex<(double)(RESOLUTION/4)) {
      double delayCoef =
	(double)waveTable[W2][(int)_global.channel[c].lfoDelayIndex];
      _global.channel[c].lfoMaxCoefInct =
	exp((log(2.0)/12.0)*_global.channel[c].lfoPitch*delayCoef);
      _global.channel[c].lfoCoefInctInct =
	exp((log(2.0)/12.0)*((2*_global.channel[c].lfoPitch*delayCoef)
			     /_global.channel[c].lfoMaxIndex));
      _global.channel[c].lfoMaxDAmp = delayCoef*_global.channel[c].lfoMaxAmp;
    }
    else
      if(_global.channel[c].delayPassed) {
	_global.channel[c].lfoMaxCoefInct = 
	  exp((log(2.0)/12.0)*_global.channel[c].lfoPitch);
	_global.channel[c].lfoCoefInctInct=
	  exp((log(2.0)/12.0)*((2*_global.channel[c].lfoPitch)
			       /_global.channel[c].lfoMaxIndex));
	_global.channel[c].lfoMaxDAmp=_global.channel[c].lfoMaxAmp;
      }
}

//-----------------------------------------------------------------
// setOutLevel
//-----------------------------------------------------------------
void DeicsOnze::setOutLevel(int c, int k) {
  for(int v=0; v<_global.channel[c].nbrVoices; v++) {
    if(_global.channel[c].voices[v].op[k].envState!=OFF) {
      _global.channel[c].voices[v].op[k].amp =
	outLevel2Amp(_preset[c]->outLevel[k])
	* _global.channel[c].voices[v].op[k].ampVeloNote
	* brightness2Amp(c, k);
    }
  }
}
void DeicsOnze::setOutLevel(int c) {
  for(int k=0; k<NBROP; k++) {
    setOutLevel(c, k);
  }
}
//-----------------------------------------------------------------
// setEnvAttack
//-----------------------------------------------------------------
void DeicsOnze::setEnvAttack(int c, int v, int k) {
  if(_global.channel[c].voices[v].op[k].envState==ATTACK)
    _global.channel[c].voices[v].op[k].envInct=
      (_preset[c]->eg[k].ar==0?0:
       (double)(RESOLUTION/4)/(envAR2s(_preset[c]->eg[k].ar)
			       *_global.deiSampleRate))
      *coefAttack(_global.channel[c].attack);
}
void DeicsOnze::setEnvAttack(int c, int k) {
  for(int v=0; v<_global.channel[c].nbrVoices; v++) setEnvAttack(c, v, k);
}
void DeicsOnze::setEnvAttack(int c) {
  for(int k=0; k<NBROP; k++) setEnvAttack(c, k);
}
//-----------------------------------------------------------------
// setEnvRelease
//-----------------------------------------------------------------
void DeicsOnze::setEnvRelease(int c, int v, int k) {
  if(_global.channel[c].voices[v].op[k].envState==RELEASE)
    _global.channel[c].voices[v].op[k].coefVLevel =
      envRR2coef(_preset[c]->eg[k].rr, _global.deiSampleRate,
		 _global.channel[c].release);
}
void DeicsOnze::setEnvRelease(int c, int k) {
  for(int v=0; v<_global.channel[c].nbrVoices; v++) setEnvRelease(c, v, k);
}
void DeicsOnze::setEnvRelease(int c) {
  for(int k=0; k<NBROP; k++) setEnvRelease(c, k);
}  
//-----------------------------------------------------------------
// setPitchEnvRelease
//-----------------------------------------------------------------
void DeicsOnze::setPitchEnvRelease(int c, int v) {
  if(isPitchEnv(&_preset[c]->pitchEg)) {
    if(_global.channel[c].voices[v].pitchEnvCoefInct
       > _global.channel[c].voices[v].pitchEnvCoefInctPhase1) {
      _global.channel[c].voices[v].pitchEnvCoefInctInct = 
	getPitchEnvCoefInctInct(1, 0, _preset[c]->pitchEg.pr3,
				_global.deiSampleRate);
      _global.channel[c].voices[v].pitchEnvState = RELEASE_PE;
    }
    else if(_global.channel[c].voices[v].pitchEnvCoefInct
	    < _global.channel[c].voices[v].pitchEnvCoefInctPhase1) {
      _global.channel[c].voices[v].pitchEnvCoefInctInct = 
	getPitchEnvCoefInctInct(0, 1, _preset[c]->pitchEg.pr3,
				_global.deiSampleRate);
      _global.channel[c].voices[v].pitchEnvState = RELEASE_PE;
    }
    else {
      _global.channel[c].voices[v].pitchEnvCoefInctInct = 1.0;
      _global.channel[c].voices[v].pitchEnvState = OFF_PE;
    }
  }
}

//-----------------------------------------------------------------
// setQuality
//-----------------------------------------------------------------
void DeicsOnze::setQuality(Quality q) {
  _global.quality = q;
  switch(q) {
  case high :
    _global.qualityCounterTop = 1;
    break;
  case middle :
    _global.qualityCounterTop = 2;
    break;
  case low :
    _global.qualityCounterTop = 4;
    break;
  case ultralow :
    _global.qualityCounterTop = 6;
    break;
  default : printf("Error switch setQuality : out of value\n");
    break;
  }
  //calculate _global.deiSampleRate
  _global.deiSampleRate = (double)sampleRate()
    / (double)_global.qualityCounterTop;
  _global.qualityCounter = 0;
  //update lfo to consider the new samplerate
  for(int c = 0; c < 16; c++) if(_global.channel[c].isEnable) setLfo(c);
  //update the cutoffs of the filters
  _dryFilter->setCutoff(_global.deiSampleRate/4.0);
  _reverbFilter->setCutoff(_global.deiSampleRate/4.0);
  _chorusFilter->setCutoff(_global.deiSampleRate/4.0);
  _delayFilter->setCutoff(_global.deiSampleRate/4.0);
}

//-----------------------------------------------------------------
// setFilter
//-----------------------------------------------------------------
void DeicsOnze::setFilter(bool f) {
  _global.filter = f;
}
//-----------------------------------------------------------------
// brightness2Amp
//-----------------------------------------------------------------
double DeicsOnze::brightness2Amp(int c, int k) {
  if(
     (k==1 && (_preset[c]->algorithm!=SIXTH || _preset[c]->algorithm!=SEVENTH
	       || _preset[c]->algorithm!=EIGHTH))
     ||
     (k==2 && (_preset[c]->algorithm==FIRST || _preset[c]->algorithm==SECOND
	       || _preset[c]->algorithm==THIRD || _preset[c]->algorithm==FOURTH))
     ||
     (k==3 && (_preset[c]->algorithm!=EIGHTH))
     ) {
    double x = 2.0*(double)_global.channel[c].brightness
      / (double)MAXFINEBRIGHTNESS;
    double square_x = x*x;
    return(square_x*x);
  }
  else return(1.0);
}
//-----------------------------------------------------------------
// setFeedback
//-----------------------------------------------------------------
void DeicsOnze::setFeedback(int c) {
  _global.channel[c].feedbackAmp =
    COEFFEEDBACK*exp(log(2)*(double)(_preset[c]->feedback-MAXFEEDBACK));
}

//-----------------------------------------------------------------
// setPreset
//-----------------------------------------------------------------

void DeicsOnze::setPreset(int c) {
    setFeedback(c);
    setLfo(c);
    setEnvAttack(c);
    setEnvRelease(c);
    setOutLevel(c);
}


inline double coarseFine2Ratio(int c,int f) {
  double tab[64][16]=
    {
      {0.50,0.56,0.62,0.68,0.75,0.81,0.87,0.93,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0},
      {0.71,0.79,0.88,0.96,1.05,1.14,1.23,1.32,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0},
      {0.78,0.88,0.98,1.07,1.17,1.27,1.37,1.47,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0},
      {0.87,0.97,1.08,1.18,1.29,1.40,1.51,1.62,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0},
      {1.00,1.06,1.12,1.18,1.25,1.31,1.37,1.43,1.50,1.56,1.62,1.68,1.75,1.81,1.87,1.93},
      {1.41,1.49,1.58,1.67,1.76,1.85,1.93,2.02,2.11,2.20,2.29,2.37,2.46,2.55,2.64,2.73},
      {1.57,1.66,1.76,1.86,1.96,2.06,2.15,2.25,2.35,2.45,2.55,2.64,2.74,2.84,2.94,3.04},
      {1.73,1.83,1.94,2.05,2.16,2.27,2.37,2.48,2.59,2.70,2.81,2.91,3.02,3.13,3.24,3.35},
      {2.00,2.06,2.12,2.18,2.25,2.31,2.37,2.43,2.50,2.56,2.62,2.68,2.75,2.81,2.87,2.93},
      {2.82,2.90,2.99,3.08,3.17,3.26,3.34,3.43,3.52,3.61,3.70,3.78,3.87,3.96,4.05,3.14},
      {3.00,3.06,3.12,3.18,3.25,3.31,3.37,3.43,3.50,3.56,3.62,3.68,3.75,3.81,3.87,3.93} ,
      {3.14,3.23,3.33,3.43,3.53,3.63,3.72,3.82,3.92,4.02,4.12,4.21,4.31,4.41,4.51,4.61},
      {3.46,3.56,3.67,3.78,3.89,4.00,4.10,4.21,4.32,4.43,4.54,4.64,4.75,4.86,4.97,5.08},
      {4.00,4.06,4.12,4.18,4.25,4.31,4.37,4.43,4.50,4.56,4.62,4.68,4.75,4.81,4.87,4.93},
      {4.24,4.31,4.40,4.49,4.58,4.67,4.75,4.84,4.93,5.02,5.11,5.19,5.28,5.37,5.46,5.55},
      {4.71,4.80,4.90,5.00,5.10,5.20,5.29,5.39,5.49,5.59,5.69,5.78,5.88,5.98,6.08,6.18},
      {5.00,5.06,5.12,5.18,5.25,5.31,5.37,5.43,5.50,5.56,5.62,5.68,5.75,5.81,5.87,5.93},
      {5.19,5.29,5.40,5.51,5.62,5.73,5.83,5.94,6.05,6.16,6.27,6.37,6.48,6.59,6.70,6.81},
      {5.65,5.72,5.81,5.90,5.99,6.08,6.16,6.25,6.34,6.43,6.52,6.60,6.69,6.78,6.87,6.96},
      {6.00,6.06,6.12,6.18,6.25,6.31,6.37,6.43,6.50,6.56,6.62,6.68,6.75,6.81,6.87,6.93},
      {6.28,6.37,6.47,6.57,6.67,6.77,6.86,6.96,7.06,7.16,7.26,7.35,7.45,7.55,7.65,7.75},
      {6.92,7.02,7.13,7.24,7.35,7.46,7.56,7.67,7.78,7.89,8.00,8.10,8.21,8.32,8.43,8.54},
      {7.00,7.06,7.12,7.18,7.25,7.31,7.37,7.43,7.50,7.56,7.62,7.68,7.75,7.81,7.87,7.93},
      {7.07,7.13,7.22,7.31,7.40,7.49,7.57,7.66,7.75,7.84,7.93,8.01,8.10,8.19,8.28,8.37},
      {7.85,7.94,8.04,8.14,8.24,8.34,8.43,8.53,8.63,8.73,8.83,8.92,9.02,9.12,9.22,9.32},
      {8.00,8.06,8.12,8.18,8.25,8.31,8.37,8.43,8.50,8.56,8.62,8.68,8.75,8.81,8.87,8.93},
      {8.48,8.54,8.63,8.72,8.81,8.90,8.98,9.07,9.16,9.25,9.34,9.42,9.51,9.60,9.69,9.78},
      {8.65,8.75,8.86,8.97,9.08,9.19,9.29,9.40,9.51,9.62,9.73,9.83,9.94,10.05,10.16,10.27},
      {9.00,9.06,9.12,9.18,9.25,9.31,9.37,9.43,9.50,9.56,9.62,9.68,9.75,9.81,9.87,9.93},
      {9.42,9.51,9.61,9.71,9.81,9.91,10.00,10.10,10.20,10.30,10.40,10.49,10.59,10.69,10.79,10.89},
      {9.89,9.95,10.04,10.13,10.22,10.31,10.39,10.48,10.57,10.66,10.75,10.83,10.92,11.01,11.10,11.19},
      {10.00,10.06,10.12,10.18,10.25,10.31,10.37,10.43,10.50,10.56,10.62,10.68,10.75,10.81,10.87,10.93},
      {10.38,10.48,10.59,10.70,10.81,10.92,11.02,11.13,11.24,11.35,11.46,11.56,11.67,11.78,11.89,12.00},
      {10.99,11.08,11.18,11.28,11.38,11.48,11.57,11.67,11.77,11.87,11.97,12.06,12.16,12.26,12.36,12.46},
      {11.00,11.06,11.12,11.18,11.25,11.31,11.37,11.43,11.50,11.56,11.62,11.68,11.75,11.81,11.87,11.93},
      {11.30,11.36,11.45,11.54,11.63,11.72,11.80,11.89,11.98,12.07,12.16,12.24,12.33,12.42,12.51,12.60},
      {12.00,12.06,12.12,12.18,12.25,12.31,12.37,12.43,12.50,12.56,12.62,12.68,12.75,12.81,12.87,12.93},
      {12.11,12.21,12.32,12.43,12.54,12.65,12.75,12.86,12.97,13.08,13.19,13.29,13.40,13.51,13.62,13.73},
      {12.56,12.65,12.75,12.85,12.95,13.05,13.14,13.24,13.34,13.44,13.54,13.63,13.73,13.83,13.93,14.03},
      {12.72,12.77,12.86,12.95,13.04,13.13,13.21,13.30,13.39,13.48,13.57,13.65,13.74,13.83,13.92,14.01},
      {13.00,13.06,13.12,13.18,13.25,13.31,13.37,13.43,13.50,13.56,13.62,13.68,13.75,13.81,13.87,13.93},
      {13.84,13.94,14.05,14.16,14.27,14.38,14.48,14.59,14.70,14.81,14.92,15.02,15.13,15.24,15.35,15.46},
      {14.00,14.06,14.12,14.18,14.25,14.31,14.37,14.43,14.50,14.56,14.62,14.68,14.75,14.81,14.87,14.93},
      {14.10,14.18,14.27,14.36,14.45,14.54,14.62,14.71,14.80,14.89,14.98,15.06,15.15,15.24,15.33,15.42},
      {14.13,14.22,14.32,14.42,14.52,14.62,14.71,14.81,14.91,15.01,15.11,15.20,15.30,15.40,15.50,15.60},
      {15.00,15.06,15.12,15.18,15.25,15.31,15.37,15.43,15.50,15.56,15.62,15.68,15.75,15.81,15.87,15.93},
      {15.55,15.59,15.68,15.77,15.86,15.95,16.03,16.12,16.21,16.30,16.39,16.47,16.56,16.65,16.74,16.83},
      {15.57,15.67,15.78,15.89,16.00,16.11,16.21,16.32,16.43,16.54,16.65,16.75,16.86,16.97,17.08,17.19},
      {15.70,15.79,15.89,15.99,16.09,16.19,16.28,16.38,16.48,16.58,16.68,16.77,16.87,16.97,17.07,17.17},
      {16.96,17.00,17.09,17.18,17.27,17.36,17.44,17.53,17.62,17.71,17.80,17.88,17.97,18.06,18.15,18.24},
      {17.27,17.36,17.46,17.56,17.66,17.76,17.85,17.95,18.05,18.15,18.25,18.34,18.44,18.54,18.64,18.74},
      {17.30,17.40,17.51,17.62,17.73,17.84,17.94,18.05,18.16,18.27,18.38,18.48,18.59,18.70,18.81,18.92},
      {18.37,18.41,18.50,18.59,18.68,18.77,18.85,18.94,19.03,19.12,19.21,19.29,19.38,19.47,19.56,19.65},
      {18.84,18.93,19.03,19.13,19.23,19.33,19.42,19.52,19.62,19.72,19.82,19.91,20.01,20.11,20.21,20.31},
      {19.03,19.13,19.24,19.35,19.46,19.57,19.67,19.78,19.89,20.00,20.11,20.21,20.32,20.43,20.54,20.65},
      {19.78,19.82,19.91,20.00,20.09,20.18,20.26,20.35,20.44,20.53,20.62,20.70,20.79,20.88,20.97,21.06},
      {20.41,20.50,20.60,20.70,20.80,20.90,20.99,21.09,21.19,21.29,21.39,21.48,21.58,21.68,21.78,21.88},
      {20.76,20.86,20.97,21.08,21.19,21.30,21.40,21.51,21.62,21.73,21.84,21.94,22.05,22.16,22.27,22.38},
      {21.20,21.23,21.32,21.41,21.50,21.59,21.67,21.76,21.85,21.94,22.03,22.11,22.20,22.29,22.38,22.47},
      {21.98,22.07,22.17,22.17,22.37,22.47,22.56,22.66,22.76,22.86,22.96,23.05,23.15,23.25,23.35,23.45},
      {22.49,22.59,22.70,22.81,22.92,23.03,23.13,13.24,13.35,13.46,13.57,13.67,13.78,13.89,24.00,24.11},
      {23.55,23.64,23.74,23.84,23.94,24.04,24.13,24.23,24.33,24.43,24.53,24.62,24.72,24.82,24.92,25.02},
      {24.22,24.32,24.43,24.54,24.65,24.76,24.86,24.97,25.08,25.19,25.30,25.40,25.51,25.62,25.73,25.84},
      {25.95,26.05,26.16,26.27,26.38,26.49,26.59,26.70,26.81,26.92,27.03,27.13,27.24,27.35,27.46,27.57}
    };
  return(tab[c][f]);
}

//---------------------------------------------------------------
// loadSet
//---------------------------------------------------------------
void DeicsOnze::loadSet(QString fileName) {
  // read the XML file and create DOM tree
  if(!fileName.isEmpty()) {
    QFile deicsonzeFile(fileName);
    if(!deicsonzeFile.open(QIODevice::ReadOnly)) {
      printf("Critical Error Cannot open file %s\n", 
	     fileName.toAscii().data());
      return;
    }
    QDomDocument domTree;
    if (!domTree.setContent(&deicsonzeFile )) {
      printf("Critical Error Parsing error for file %s\n",
	     fileName.toAscii().data());
      deicsonzeFile.close();
      return;
    }
    deicsonzeFile.close();
    
    QDomNode node = domTree.documentElement();
    while (!node.isNull()) {
      QDomElement e = node.toElement();
      if (e.isNull())
	continue;
      if (e.tagName() == "deicsOnzeSet") {
	QString version = e.attribute(QString("version"));
	if (version == "1.0") {
	  for(int c = 0; c<NBRCHANNELS; c++) _preset[c]=_initialPreset;
	  while(!_set->_categoryVector.empty())
	    delete(*_set->_categoryVector.begin());
	  _set->readSet(node.firstChild());
	  //display load preset
	  unsigned char dataUpdateGuiSet[1];
	  dataUpdateGuiSet[0]=SYSEX_UPDATESETGUI;
	  MusECore::MidiPlayEvent evSysexUpdateGuiSet(0, 0, MusECore::ME_SYSEX, 
					(const unsigned char*)dataUpdateGuiSet,
					1);
	  _gui->writeEvent(evSysexUpdateGuiSet);
	}
	else printf("unsupported *.dei file version %s\n",
		    version.toLatin1().constData());
      }
      else printf("DeicsOnze: %s not supported\n",
		  e.tagName().toLatin1().constData());
      node = node.nextSibling();
    }
  }
}
//---------------------------------------------------------------
// loadSutulaPreset
//---------------------------------------------------------------

void DeicsOnze::loadSutulaPresets()
{
    FILE* file;
    int v;
    int crs[NBROP], fin[NBROP]; //coarse ratio, fine ratio
    char s[500];
    char sname[LENGTHNAME+1];
    char scategory[LENGTHCATEGORY+1];
    char ssubcategory[LENGTHSUBCATEGORY+1];
    int k;
    int nhBank, nlBank, nPreset;
    Preset* presetTemp;
    Subcategory* subcategoryTemp = NULL;
    Category* categoryTemp = NULL;

    if(!_set) _set=new Set("Sutula Bank");

    nhBank=0;
    nlBank=0;
    nPreset=0;

    //QString presetPath(INSTPREFIX);
    //presetPath += "/share/" PACKAGEVERSION "/presets/deicsonze/ARCH_ALIN";

    QString presetPath("/home/a-lin/sources/svnMusEDev/lmuse/muse/synti/deicsonze/ARCH_ALIN");

    file = fopen (presetPath.toLatin1().constData(), "rt");
    if (file == NULL) {
	printf("can't open ");
	printf("%s", presetPath.toLatin1().constData());
	printf("\n");
    }
    else
    {
	while(fgets(s, 500, file) && !strstr(s, "** Source:"))
	{
	    if (strstr(s,"* CATEGORY"))
	    {
		sscanf(s, "* CATEGORY %s", scategory);
		categoryTemp=new Category(_set, scategory,0);
	    }
	    if (strstr(s,"* SUBCATEGORY"))
	    {
		sscanf(s, "* SUBCATEGORY %s", ssubcategory);
		subcategoryTemp=new Subcategory(categoryTemp,ssubcategory,0);
		nlBank++;
	    }
	}
	while(!feof(file))
	{
	
	    presetTemp=new Preset(subcategoryTemp);
	    // Fill the preset
            //OP.4 to OP.1
	    for(int kaka=(NBROP-1); kaka>=0; kaka--)
	    {
		k=(kaka==2?1:(kaka==1?2:kaka));
		
		fscanf(file, "%x", &v);//0
		presetTemp->eg[k].ar=v;
		fscanf(file, "%x", &v);//1
		presetTemp->eg[k].d1r=v;
		fscanf(file, "%x", &v);//2
		presetTemp->eg[k].d2r=v;
		fscanf(file, "%x", &v);//3
		presetTemp->eg[k].rr=v;
		fscanf(file, "%x", &v);//4
		presetTemp->eg[k].d1l=v;
		fscanf(file, "%x", &v);//5
		presetTemp->scaling.level[k]=v;
		fscanf(file, "%x", &v);//6
		presetTemp->sensitivity.keyVelocity[k]=
		    v & 0x7;
		presetTemp->sensitivity.egBias[k]=
		    (v & 0x38)>>3;
		presetTemp->sensitivity.ampOn[k]=
		    (v & 0x40)>>6;
		fscanf(file, "%x", &v);//7
		presetTemp->outLevel[k]=v;
		fscanf(file, "%x", &v);//8
		crs[k]=v;
		fscanf(file, "%x", &v);//9
		presetTemp->detune[k]=(v & 0x7)-3;
		presetTemp->scaling.rate[k]=(v & 0x18)>>3;
	    }
	    fscanf(file, "%x", &v);//40
	    presetTemp->algorithm=
		((v & 0x7)==0?FIRST:
		 ((v & 0x7)==1?SECOND:
		  ((v & 0x7)==2?THIRD:
		   ((v & 0x7)==3?FOURTH:
		    ((v & 0x7)==4?FIFTH:
		     ((v & 0x7)==5?SIXTH:
		      ((v & 0x7)==6?SEVENTH:EIGHTH)))))));
	    presetTemp->feedback=(v & 0x38)>>3;
	    presetTemp->lfo.sync=(v & 0x40)>>6;	
	    fscanf(file, "%x", &v);//41
	    presetTemp->lfo.speed=v;
	    fscanf(file, "%x", &v);//42
	    presetTemp->lfo.delay=v;
	    fscanf(file, "%x", &v);//43
	    presetTemp->lfo.pModDepth=v;
	    fscanf(file, "%x", &v);//44
	    presetTemp->lfo.aModDepth=v;
	    fscanf(file, "%x", &v);//45
	    presetTemp->lfo.wave=
		((v & 0x3)==0?SAWUP:
		 ((v & 0x3)==1?SQUARE:
		  ((v & 0x3)==2?TRIANGL:SHOLD)));
	    presetTemp->sensitivity.amplitude=(v & 0xc)>>2;
	    presetTemp->sensitivity.pitch=(v & 0x70)>>4;
	    fscanf(file, "%x", &v);//46
	    presetTemp->function.transpose=v-24;
	    fscanf(file, "%x", &v);//47
	    presetTemp->function.pBendRange=v;
	    fscanf(file, "%x", &v);//48
	    presetTemp->function.portamento=
		((v & 0x1)==0?FULL:FINGER);
	    presetTemp->function.footSw=
		((v & 0x4)==0?SUS:POR);
	    presetTemp->function.mode=
		((v & 0x8)==0?POLY:MONO);
	    fscanf(file, "%x", &v);//49
	    presetTemp->function.portamentoTime=v;
	    fscanf(file, "%x", &v);//50
	    presetTemp->function.fcVolume=v;
	    fscanf(file, "%x", &v);//51
	    presetTemp->function.mwPitch=v;
	    fscanf(file, "%x", &v);//52
	    presetTemp->function.mwAmplitude=v;
	    fscanf(file, "%x", &v);//53
	    presetTemp->function.bcPitch=v;
	    fscanf(file, "%x", &v);//54
	    presetTemp->function.bcAmplitude=v;
	    fscanf(file, "%x", &v);//55
	    presetTemp->function.bcPitchBias=v;
	    fscanf(file, "%x", &v);//56
	    presetTemp->function.bcEgBias=v;
	    for(int l=0; l<10; l++)
	    {
		fscanf(file, "%x", &v);//57 to 66
		sname[l]=(char)v;
	    }
	    sname[10]='\0';
	    presetTemp->name=sname;
	    fscanf(file, "%x", &v);//67
	    presetTemp->pitchEg.pr1=v;
	    fscanf(file, "%x", &v);//68
	    presetTemp->pitchEg.pr2=v;
	    fscanf(file, "%x", &v);//69
	    presetTemp->pitchEg.pr3=v;
	    fscanf(file, "%x", &v);//70
	    presetTemp->pitchEg.pl1=v;
	    fscanf(file, "%x", &v);//71
	    presetTemp->pitchEg.pl1=v;
	    fscanf(file, "%x", &v);//72
	    presetTemp->pitchEg.pl1=v;
	    for(int kaka=(NBROP-1); kaka>=0; kaka--)
	    {
		k=(kaka==2?1:(kaka==1?2:kaka));

		fscanf(file, "%x", &v);//73, 75, 77, 79
		presetTemp->frequency[k].isFix=(v & 0x8)>>3;
		presetTemp->frequency[k].freq=((v & 0x7)==0?8:(v & 0x7)*16);
		presetTemp->eg[k].egShift=
		    (((v & 0x30)>>4)==0?VOF:
		     (((v & 0x30)>>4)==1?V48:
		      (((v & 0x30)>>4)==2?V24:V12)));
		fscanf(file, "%x", &v);//74, 76, 78, 80
		fin[k]=v & 0xF;
		presetTemp->frequency[k].freq+=fin[k];
		presetTemp->frequency[k].ratio=
		    coarseFine2Ratio(crs[k],fin[k]);
		presetTemp->oscWave[k]=
		    (((v & 0x70)>>4)==0?W1:
		     (((v & 0x70)>>4)==1?W2:
		      (((v & 0x70)>>4)==2?W3:
		       (((v & 0x70)>>4)==3?W4:
			(((v & 0x70)>>4)==4?W5:
			 (((v & 0x70)>>4)==5?W6:
			  (((v & 0x70)>>4)==6?W7:W8)))))));
	    }
	    fscanf(file, "%x", &v);//81
	    presetTemp->function.reverbRate=v;
	    fscanf(file, "%x", &v);//82
	    presetTemp->function.fcPitch=v;
	    fscanf(file, "%x", &v);//83
	    presetTemp->function.fcAmplitude=v;
	    //presetTemp->globalDetune=0;
	    presetTemp->prog=nPreset;
            // End of filling the preset

	    nPreset++;
	    while(fgets(s, 500, file) && !strstr(s, "** Source:"))
	    {
		if (strstr(s,"* CATEGORY"))
		{
		    sscanf(s, "* CATEGORY %s", scategory);
		    nhBank++;
		    categoryTemp=new Category(_set,scategory,nhBank);
		    nlBank=0;
		}
		if (strstr(s,"* SUBCATEGORY"))
		{
		    sscanf(s, "* SUBCATEGORY %s", ssubcategory);
		    subcategoryTemp=new
			Subcategory(categoryTemp,ssubcategory,nlBank);
		    nlBank++;
		    nPreset=0;
		}
	    }
	}
    }
    fclose(file);
}

//---------------------------------------------------------
// minVolu2Voice
//  return the number of the voice which is the least aloud
//  and is not is the ATTACK state
//---------------------------------------------------------
int DeicsOnze::minVolu2Voice(int c) {
  int minVoice=0;
  double min=MAXVOLUME;
  for(int i=0; i<_global.channel[c].nbrVoices; i++)
    {
      min=((min>_global.channel[c].voices[i].volume
	    && _global.channel[c].voices[i].op[0].envState!=ATTACK
	    && _global.channel[c].voices[i].op[1].envState!=ATTACK
	    && _global.channel[c].voices[i].op[2].envState!=ATTACK
	    && _global.channel[c].voices[i].op[3].envState!=ATTACK)?
	   _global.channel[c].voices[i].volume:min);
      minVoice=(min==_global.channel[c].voices[i].volume?i:minVoice);
    }
  return minVoice;
}

//---------------------------------------------------------
// noteOff2Voice
//  return the number of one off voice, MAXNBRVOICES otherwise
//---------------------------------------------------------
int DeicsOnze::noteOff2Voice(int c) {
  int offVoice=MAXNBRVOICES;
  for(int i=0; i<_global.channel[c].nbrVoices; i++)
    offVoice = (_global.channel[c].voices[i].isOn
		|| _global.channel[c].voices[i].keyOn?
		offVoice:i);
  return offVoice;
}

//---------------------------------------------------------
// pitchOn2Voice
//  return the number of the voice which has the input
//   pitch and is keyOn
//---------------------------------------------------------
int DeicsOnze::pitchOn2Voice(int c, int pitch) {
  int pitchVoice=MAXNBRVOICES;
  for(int i=0; i<_global.channel[c].nbrVoices; i++) {
    if(_global.channel[c].voices[i].pitch==
       pitch && _global.channel[c].voices[i].keyOn
       && !_global.channel[c].voices[i].isSustained) {
      pitchVoice = i;
      return pitchVoice;
    }
  }
  return pitchVoice;
}

//---------------------------------------------------------
// getAttractor
//---------------------------------------------------------
inline double getAttractor(int portamentoTime, double sr) {
  /* some explanations

     c(48000) = c > 1
     
     f_sr(0) = 1000, f_sr(t) = 2000
     
     f_sr*2(0) = 1000, f_sr*2(t*2) = 2000
     
     f_sr(t) = exp(t*ln(c(sr))) * 1000
     
     2000 = exp(t*ln(c(48000))) * 1000
     
     2000 = exp(t*2*ln(c(48000*2))) * 1000
     
     t*ln(c(48000)) = t*2*ln(c(48000*2))
     
     c(48000*m) = exp(ln(c)/m)
     
     sr = 48000*m
  */
  double c;
  c = 1.0 + COEFPORTA/(double)(portamentoTime*portamentoTime);
  return(exp(log(c)*48000.0/sr));
}

//---------------------------------------------------------
// pitch2freq
//---------------------------------------------------------
inline double pitch2freq(double p) {
  return(LOWERNOTEFREQ*exp(p*log(2.0)/12.0));
}

//---------------------------------------------------------
// lfoUpdate
//  update the coefficent which multiplies the current inct
//  in order to
//  get the right current frequency with respect to the lfo
//  update the coefficent which multiplies the amplitude.
//---------------------------------------------------------
inline void lfoUpdate(Preset* p, Channel* p_c, float* wt) {
  double delayCoef;

  //Manage LFO delay
  if(!p_c->delayPassed) {
    if(p_c->lfoIndex==0 || p_c->lfoIndex==p_c->lfoMaxIndex/2) {
      if(p_c->lfoDelayIndex<(double)(RESOLUTION/4)) {
	delayCoef=(double)wt[(int)p_c->lfoDelayIndex];
	p_c->lfoMaxCoefInct=exp((log(2.0)/12.0)*p_c->lfoPitch*delayCoef);
	p_c->lfoCoefInctInct=
	  exp((log(2.0)/12.0)*((2*p_c->lfoPitch*delayCoef)/p_c->lfoMaxIndex));
	p_c->lfoDelayIndex+=p_c->lfoDelayInct;
	p_c->lfoMaxDAmp=delayCoef*p_c->lfoMaxAmp;
      }
      else {
	p_c->lfoMaxCoefInct=exp((log(2.0)/12.0)*p_c->lfoPitch);
	p_c->lfoCoefInctInct=
	  exp((log(2.0)/12.0)*((2*p_c->lfoPitch)/p_c->lfoMaxIndex));
	p_c->delayPassed=true;
	p_c->lfoMaxDAmp=p_c->lfoMaxAmp;
      }
    }
  }
  switch(p->lfo.wave) {
  case SAWUP :
    if(p_c->lfoIndex==0) {
      p_c->lfoCoefInct=1.0/(p_c->lfoMaxCoefInct);
      p_c->lfoCoefAmp=p_c->lfoMaxDAmp/(double)p_c->lfoMaxIndex;
      p_c->lfoAmp=1.0;
    }
    else {
      p_c->lfoCoefInct*=p_c->lfoCoefInctInct;
      p_c->lfoAmp-=p_c->lfoCoefAmp;
    }
    break;
  case SQUARE :
    if(p_c->lfoIndex==0) {
      p_c->lfoCoefInct=p_c->lfoMaxCoefInct;
      p_c->lfoAmp=1.0;
    }
    if(p_c->lfoIndex==(p_c->lfoMaxIndex/2)) {
      p_c->lfoCoefInct=1.0/p_c->lfoMaxCoefInct;
      p_c->lfoAmp=1.0-p_c->lfoMaxDAmp;
    }
    break;
  case TRIANGL :
    if(p_c->lfoIndex==0) {
      p_c->lfoCoefInct=1.0;
      p_c->lfoCoefAmp=p_c->lfoMaxDAmp
	/(double)(p_c->lfoMaxIndex/2);
      p_c->lfoAmp=1.0-p_c->lfoMaxDAmp/2.0;
    }
    else if(p_c->lfoIndex<(p_c->lfoMaxIndex/4)) {
      p_c->lfoCoefInct*=p_c->lfoCoefInctInct;
      p_c->lfoAmp-=p_c->lfoCoefAmp;
    }
    else if(p_c->lfoIndex<((3*p_c->lfoMaxIndex)/4)) {
      p_c->lfoCoefInct/=p_c->lfoCoefInctInct;
      p_c->lfoAmp+=p_c->lfoCoefAmp;
    }
    else if(p_c->lfoIndex<p_c->lfoMaxIndex) {
      p_c->lfoCoefInct*=p_c->lfoCoefInctInct;
      p_c->lfoAmp-=p_c->lfoCoefAmp;
    }
    break;
  case SHOLD :
    if(p_c->lfoIndex==0||p_c->lfoIndex==(p_c->lfoMaxIndex/2)) {
      double r;//uniform random between -1.0 and 1.0
      r = (double)(2*rand()-RAND_MAX)/(double)RAND_MAX;
      p_c->lfoCoefInct=(r>=0.0?1.0+r*(p_c->lfoMaxCoefInct-1.0)
			:1.0/(1.0-r*(p_c->lfoMaxCoefInct-1.0)));
      p_c->lfoAmp=1.0-(r/2.0+0.5)*p_c->lfoMaxDAmp;
    }
    break;
  default : printf("Error : lfo wave does not exist\n");
    break;
  }
  p_c->lfoIndex=(p_c->lfoIndex<p_c->lfoMaxIndex?p_c->lfoIndex+1:0);
}

//---------------------------------------------------------
// portamento update
//---------------------------------------------------------
inline void portamentoUpdate(Channel* p_c, Voice* p_v) {
  double inctTemp;
  bool allTargetReached;
  if(p_v->hasAttractor) {
    allTargetReached = true;
    for(int k = 0; k<NBROP; k++) {
      if(p_v->op[k].inct < p_v->op[k].targetInct) {
	inctTemp = p_v->op[k].inct * p_v->attractor;
	if(inctTemp < p_v->op[k].targetInct) {
	  allTargetReached = false;
	  p_v->op[k].inct = inctTemp;
	}
	else p_v->op[k].inct = p_v->op[k].targetInct;
      }
      else if(p_v->op[k].inct > p_v->op[k].targetInct) {
	inctTemp = p_v->op[k].inct / p_v->attractor;
	if(inctTemp > p_v->op[k].targetInct) {
	  allTargetReached = false;
	  p_v->op[k].inct = inctTemp;
	}
	else p_v->op[k].inct = p_v->op[k].targetInct;
      }
      p_c->lastInc[k] = p_v->op[k].inct;
    }
    if(allTargetReached) p_v->hasAttractor = false;
  }
}


//---------------------------------------------------------
// pitchEnvelopeUpdate
//---------------------------------------------------------
inline void pitchEnvelopeUpdate(Voice* v, PitchEg* pe, double sr) {
  if(v->pitchEnvState != OFF_PE) {
    switch(v->pitchEnvState) {
    case PHASE1 :
      if( //change to phase2
	 (v->pitchEnvCoefInctInct == 1.0)
	 || (v->pitchEnvCoefInctInct > 1.0 &&
	     v->pitchEnvCoefInct > v->pitchEnvCoefInctPhase2)
	 || (v->pitchEnvCoefInctInct < 1.0 &&
	     v->pitchEnvCoefInct < v->pitchEnvCoefInctPhase2)
	 ) {
	v->pitchEnvState = PHASE2;
	v->pitchEnvCoefInct = getPitchEnvCoefInct(pe->pl2);
	v->pitchEnvCoefInctInct =
	  getPitchEnvCoefInctInct(pe->pl2, pe->pl3, pe->pr2, sr);
      }
      else v->pitchEnvCoefInct *= v->pitchEnvCoefInctInct;
      break;
    case PHASE2 :
      if( //change to off (temporarely)
	 (v->pitchEnvCoefInctInct == 1.0)
	 || (v->pitchEnvCoefInctInct > 1.0 &&
	     v->pitchEnvCoefInct > v->pitchEnvCoefInctPhase3)
	 || (v->pitchEnvCoefInctInct < 1.0 &&
	     v->pitchEnvCoefInct < v->pitchEnvCoefInctPhase3)
	 ) {
	v->pitchEnvState = OFF_PE;
	v->pitchEnvCoefInct = getPitchEnvCoefInct(pe->pl3);
	v->pitchEnvCoefInctInct = 1.0;
      }
      else v->pitchEnvCoefInct *= v->pitchEnvCoefInctInct;
      break;
    case RELEASE_PE :
      if( //change to release2
	 (v->pitchEnvCoefInctInct == 1.0)
	 || (v->pitchEnvCoefInctInct > 1.0 &&
	     v->pitchEnvCoefInct > v->pitchEnvCoefInctPhase1)
	 || (v->pitchEnvCoefInctInct < 1.0 &&
	     v->pitchEnvCoefInct < v->pitchEnvCoefInctPhase1)
	 ) {
	v->pitchEnvState = OFF_PE;
	v->pitchEnvCoefInct = getPitchEnvCoefInct(pe->pl1);
	v->pitchEnvCoefInctInct = 1.0;
      }
      else v->pitchEnvCoefInct *= v->pitchEnvCoefInctInct;
      break;
    case OFF_PE :
      //do nothing, should not appear anyway
      break;
    default :
      printf("Error switch pitchEnvelopeUpdate, no such case\n");
      break;
    }
  }
}

//---------------------------------------------------------
// outLevel2Amp, Amp for amplitude //between 0.0 and 2.0 or more
//  100->2.0, 90->1.0, 80->0.5 ...
//---------------------------------------------------------
inline double outLevel2Amp(int ol) {
  double a;
  double b;
  a = log(2)/10.0;
  b = -a*DB0LEVEL;
  return exp(a*(double)ol+b);
}

//---------------------------------------------------------
// lowlevel2amp, 
//  127->0dB->1.0, 0->-25dB->0
//---------------------------------------------------------
inline double lowlevel2amp(int l) {
  double a, b, c, db;
  if(l==0) return 0.0;
  else {
    a = DB_MIN/127.0;
    b = -DB_MIN;
    db = a*l+b;
    c = -log(2)/3;
    return exp(-c*db);
  }
}

//---------------------------------------------------------
// level2amp, 
//  255->0dB->1.0, 0->-25dB->0
//---------------------------------------------------------
inline double level2amp(int l) {
  double a, b, c, db;
  if(l==0) return 0.0;
  else {
    a = DB_MIN/255.0;
    b = -DB_MIN;
    db = a*l+b;
    c = -log(2.0)/3.0;
    return exp(-c*db);
  }
}

//---------------------------------------------------------
// amp2level
// 1.0->0dB->255, 0->-25dB->0
//---------------------------------------------------------
inline int amp2level(double amp){
  double a, b, c;
  a = 255.0/DB_MIN;
  b = 255.0;
  c = log(2.0)/3.0;
  return (int)(a*(log(amp)/c)+b);
}

//---------------------------------------------------------
// amp2lowlevel
// 1.0->0dB->127, 0->-25dB->0
//---------------------------------------------------------
inline int amp2lowlevel(double amp){
  double a, b, c;
  a = 127.0/DB_MIN;
  b = 127.0;
  c = log(2.0)/3.0;
  return (int)(a*(log(amp)/c)+b);
}

//---------------------------------------------------------
// velo2RAmp, AmpR between 0.0 and 1.0
//  return an amplitude ratio with respect to _preset->sensitivity.keyVelocity
//---------------------------------------------------------
inline double velo2AmpR(int velo, int kvs) {
  double lev;
  lev = exp(-log(2)*kvs);
  return (lev+(1.0-lev)*((double)velo/(double)MAXVELO));
}

//---------------------------------------------------------
// envAR2s
//  return the time in second of the ATTACK duration
//---------------------------------------------------------
inline double envAR2s(int ar) {
  //determined using the fitting feature of gnuplot
  return 10.4423*exp(-0.353767*ar);
}

//---------------------------------------------------------
// envD1R2coef
//  return the coefficient for the exponential decrease
//  with respect to d1r and sampleRate, sr
//---------------------------------------------------------
inline double envD1R2coef(int d1r, double sr) {
  double dt;//such that amp(t+dt)=amp(t)/2
  double alpha;//such that amp(t)=exp(alpha*t)

  if(d1r==0) return 1.0;
  else
    {
      //dt has been determined with the fitting function of gnuplot
      dt=9.80715*exp(-0.356053*(double)d1r);

      //amp(0)=1
      //amp(t+dt)=amp(t)/2
      //amp(t)=exp(alpha*t)
      //amp(t+mt)
      //following the above equational system we found :
      alpha=-log(2)/dt;
      return exp(alpha/sr);
    }
}

//---------------------------------------------------------
// coefRelease
//  convert the release value to a coef for coefVLevel
//---------------------------------------------------------
inline double coefRelease(unsigned char release) {
  double x = COEFGRELEASE*(double)release/(double)MIDRELEASE+1.0-COEFGRELEASE;
  double square_x = x*x;
  return(1.0/(square_x*x));
}

//---------------------------------------------------------
// envRR2coef
//  return the coefficient for the exponential decrease
//  with respect to rr and sampleRate, sr
//---------------------------------------------------------
inline double envRR2coef(int rr, double sr, unsigned char release) {
  double dt;//such that amp(t+dt)=amp(t)/2
  double alpha;//such that amp(t)=exp(alpha*t)

  //dt has been determined with the fitting function of gnuplot
  dt=7.06636*exp(-0.697606*(double)rr);

  dt*=coefRelease(release);
  //printf("demi life = %e\n", dt);
  //amp(0)=1
  //amp(t+dt)=amp(t)/2
  //amp(t)=exp(alpha*t)
  //amp(t+mt)
  //following the above equational system we found :
  alpha=-log(2)/dt;
  return exp(alpha/sr);
}

//---------------------------------------------------------
// coefAttack
//  convert the attack value to a coef for envInct
//---------------------------------------------------------
inline double coefAttack(unsigned char attack) {
  double x = COEFGATTACK*(double)attack/(double)MIDATTACK + 1.0-COEFGATTACK;
  double square_x = x*x;
  return(square_x*square_x*x);
}

//---------------------------------------------------------
// env2RAmp
//  return the amplitude ratio with respect to an envelope and an
//   envelope state, making evoluate the envelope
//  sr is the sample rate and st the sine_table
//---------------------------------------------------------
inline double env2AmpR(double sr, float* wt, Eg eg, OpVoice* p_opVoice) {
  switch(p_opVoice->envState) {
  case ATTACK:
    p_opVoice->envIndex+=p_opVoice->envInct;
    if (p_opVoice->envIndex<(RESOLUTION/4)) {
      p_opVoice->envLevel=wt[(int)p_opVoice->envIndex];
    }
    else {
      p_opVoice->envState=DECAY;
      p_opVoice->envLevel=1.0;
      p_opVoice->coefVLevel=envD1R2coef(eg.d1r, sr);
    }
    return p_opVoice->envLevel;
    break;
  case DECAY:
    if (p_opVoice->envLevel>((double)eg.d1l/(double)MAXD1L)+COEFERRDECSUS) {
      p_opVoice->envLevel*=p_opVoice->coefVLevel;
    }
    else {
      p_opVoice->envState=SUSTAIN;
      p_opVoice->envLevel=((double)eg.d1l/(double)MAXD1L);
      p_opVoice->coefVLevel=envD1R2coef(eg.d2r, sr);//probably the same
    }
    return p_opVoice->envLevel;
    break;
  case SUSTAIN:
    if (p_opVoice->envLevel>COEFERRSUSREL) {
      p_opVoice->envLevel*=p_opVoice->coefVLevel;
    }
    else {
      p_opVoice->envState=OFF;
      p_opVoice->envLevel=0.0;
    }
    return p_opVoice->envLevel;
    break;
  case RELEASE:
    if (p_opVoice->envLevel > COEFERRSUSREL) {
	  p_opVoice->envLevel*=p_opVoice->coefVLevel;
    }
    else {
      p_opVoice->envState=OFF;
      p_opVoice->envLevel=0.0;
    }
    return p_opVoice->envLevel;
    break;
  case OFF: return 0.0;
    break;
  default: printf("Error case envelopeState");
    break;
  }
  return p_opVoice->envLevel;
}

//---------------------------------------------------------
// programSelect
//---------------------------------------------------------

void DeicsOnze::programSelect(int c, int hbank, int lbank, int prog) {
    Preset* foundPreset;
    foundPreset=findPreset(hbank, lbank, prog);
    if (foundPreset) _preset[c]=foundPreset;
    else {
	_preset[c]=_initialPreset;
	_preset[c]->prog=prog;
	_preset[c]->_subcategory->_lbank=lbank; //TODO : real link
	_preset[c]->_subcategory->_category->_hbank=hbank;
    }
    setPreset(c);
}

//---------------------------------------------------------
//   setModulation
//---------------------------------------------------------
void DeicsOnze::setModulation(int c, int val) {
  _global.channel[c].modulation = (unsigned char) val;
  setLfo(c);
}
//---------------------------------------------------------
//   setPitchBendCoef
//---------------------------------------------------------
void DeicsOnze::setPitchBendCoef(int c, int val) {
  _global.channel[c].pitchBendCoef =
    exp(log(2)*((double)_preset[c]->function.pBendRange
		/(double)MAXPBENDRANGE)
	*((double)val/(double)MAXPITCHBENDVALUE));
}

//---------------------------------------------------------
// setSustain
//---------------------------------------------------------
void DeicsOnze::setSustain(int c, int val) {
  _global.channel[c].sustain=(val>64);
  if(!_global.channel[c].sustain)
    for(int i=0; i<_global.channel[c].nbrVoices; i++)
      if(_global.channel[c].voices[i].isSustained) {
	for(int j=0; j<NBROP; j++) {
	  _global.channel[c].voices[i].op[j].envState = RELEASE;
	  setEnvRelease(c, i, j);
	}
	setPitchEnvRelease(c, i);
	_global.channel[c].voices[i].isSustained = false;
	_global.channel[c].voices[i].keyOn = false;
      }
}

//---------------------------------------------------------
//   readColor
//---------------------------------------------------------
QColor readColor(QDomNode node)
{
  QDomElement e = node.toElement();
  int r = e.attribute("r","0").toInt();
  int g = e.attribute("g","0").toInt();
  int b = e.attribute("b","0").toInt();
  return QColor(r, g, b);
}

//---------------------------------------------------------
// readConfiguration
//---------------------------------------------------------
void DeicsOnze::readConfiguration(QDomNode qdn) {
  QColor textColor, backgroundColor, editTextColor, editBackgroundColor;
  while(!qdn.isNull()) {
    QDomElement qdEl = qdn.toElement();
    if(qdEl.isNull())
      continue;
    //nbrVoices
    //question? does the configurqtion has to save the number of 
    //voices for each channel or not?
    //temporarly or definitly under comments
    /*
      if(qdEl.tagName()==NBRVOICESSTR) {
      setNbrVoices(qdEl.text().toInt());
      MusECore::MidiPlayEvent evNbrVoices(0, 0, 0, MusECore::ME_CONTROLLER,
			    CTRL_NBRVOICES, _global.nbrVoices);
      _gui->writeEvent(evNbrVoices);
      }*/
    //channelNum
    /*
      if(qdEl.tagName()==CHANNELNUMSTR) {
      _global.channelNum = (qdEl.text()==ALLSTR?-1:qdEl.text().toInt()-1);
      unsigned char *dataChannelNum = new unsigned char[2];
      dataChannelNum[0]=SYSEX_CHANNELNUM;
      dataChannelNum[1]=(unsigned char)_global.channelNum;
      MusECore::MidiPlayEvent 
	evChannelNum(0, 0, MusECore::ME_SYSEX, (const unsigned char*)dataChannelNum, 2);
      _gui->writeEvent(evChannelNum);    
      }*/
    //quality
    if(qdEl.tagName()==QUALITYSTR) {
      _global.quality = (qdEl.text()==HIGHSTR?high:
			 (qdEl.text()==MIDDLESTR?middle:
			  (qdEl.text()==LOWSTR?low:ultralow)));
      setQuality(_global.quality);
      unsigned char *dataQuality = new unsigned char[2];
      dataQuality[0]=SYSEX_QUALITY;
      dataQuality[1]=(unsigned char)_global.quality;
      MusECore::MidiPlayEvent evQuality(0, 0, MusECore::ME_SYSEX, (const unsigned char*)dataQuality, 2);
      _gui->writeEvent(evQuality);
    }
    //filter
    if(qdEl.tagName()==FILTERSTR) {
      setFilter(qdEl.text()==YESSTRDEI?true:false);
      unsigned char *dataFilter = new unsigned char[2];
      dataFilter[0]=SYSEX_FILTER;
      dataFilter[1]=(unsigned char)getFilter();
      MusECore::MidiPlayEvent evFilter(0, 0, MusECore::ME_SYSEX, (const unsigned char*)dataFilter, 2);
      _gui->writeEvent(evFilter);
    }
    //font size
    if(qdEl.tagName()==FONTSIZESTR) {
      _global.fontSize = qdEl.text().toInt();
      unsigned char *dataFontSize = new unsigned char[2];
      dataFontSize[0]=SYSEX_FONTSIZE;
      dataFontSize[1]=(unsigned char)_global.fontSize;
      MusECore::MidiPlayEvent evFontSize(0, 0, MusECore::ME_SYSEX, (const unsigned char*)dataFontSize, 2);
      _gui->writeEvent(evFontSize);
    }
    //saveConfig
    if(qdEl.tagName()==SAVECONFIGSTR) {
      _saveConfig = (qdEl.text()==YESSTRDEI?true:false);
      unsigned char *dataSaveConfig = new unsigned char[2];
      dataSaveConfig[0]=SYSEX_SAVECONFIG;
      dataSaveConfig[1]=(unsigned char)_saveConfig;
      MusECore::MidiPlayEvent
	evSaveConfig(0, 0, MusECore::ME_SYSEX, (const unsigned char*)dataSaveConfig, 2);
      _gui->writeEvent(evSaveConfig);
    }
    //saveOnlyUsed
    if(qdEl.tagName()==SAVEONLYUSEDSTR) {
      _saveOnlyUsed = (qdEl.text()==YESSTRDEI?true:false);
      unsigned char *dataSaveOnlyUsed = new unsigned char[2];
      dataSaveOnlyUsed[0]=SYSEX_SAVEONLYUSED;
      dataSaveOnlyUsed[1]=(unsigned char)_saveOnlyUsed;
      MusECore::MidiPlayEvent
	evSaveOnlyUsed(0, 0, MusECore::ME_SYSEX, (const unsigned char*)dataSaveOnlyUsed, 2);
      _gui->writeEvent(evSaveOnlyUsed);
    }
    //colors
    if(qdEl.tagName()==TEXTCOLORSTR) textColor = readColor(qdn);
    if(qdEl.tagName()==BACKGROUNDCOLORSTR) backgroundColor = readColor(qdn);
    if(qdEl.tagName()==EDITTEXTCOLORSTR) editTextColor = readColor(qdn);
    if(qdEl.tagName()==EDITBACKGROUNDCOLORSTR)
      editBackgroundColor = readColor(qdn);

    //must insert load image, later

    //load init set
    if(qdEl.tagName()==ISINITSETSTR) {
      _isInitSet = (qdEl.text()==YESSTRDEI?true:false);
      unsigned char *dataIsInitSet = new unsigned char[2];
      dataIsInitSet[0]=SYSEX_ISINITSET;
      dataIsInitSet[1]=(unsigned char)_isInitSet;
      MusECore::MidiPlayEvent
	evIsInitSet(0, 0, MusECore::ME_SYSEX, (const unsigned char*)dataIsInitSet, 2);
      _gui->writeEvent(evIsInitSet);
    }
    if(qdEl.tagName()==INITSETPATHSTR) {
      _initSetPath = qdEl.text();
      unsigned char *dataInitSetPath = 
	new unsigned char[1+MAXSTRLENGTHINITSETPATH];
      dataInitSetPath[0]=SYSEX_INITSETPATH;
      strncpy((char*)&dataInitSetPath[1], _initSetPath.toLatin1().constData(), 
	      MAXSTRLENGTHINITSETPATH);
      MusECore::MidiPlayEvent
	evInitSetPath(0, 0, MusECore::ME_SYSEX, (const unsigned char*)dataInitSetPath,
		      1+MAXSTRLENGTHINITSETPATH);
      _gui->writeEvent(evInitSetPath);
    }
    //load background pix
    if(qdEl.tagName()==ISBACKGROUNDPIXSTR) {
      _isBackgroundPix = (qdEl.text()==YESSTRDEI?true:false);
      unsigned char *dataIsBackgroundPix = new unsigned char[2];
      dataIsBackgroundPix[0]=SYSEX_ISBACKGROUNDPIX;
      dataIsBackgroundPix[1]=(unsigned char)_isBackgroundPix;
      MusECore::MidiPlayEvent
	evIsBackgroundPix(0, 0, MusECore::ME_SYSEX,
			  (const unsigned char*)dataIsBackgroundPix, 2);
      _gui->writeEvent(evIsBackgroundPix);
    }
    if(qdEl.tagName()==BACKGROUNDPIXPATHSTR) {
      _backgroundPixPath = qdEl.text();
      unsigned char *dataBackgroundPixPath = 
	new unsigned char[1+MAXSTRLENGTHBACKGROUNDPIXPATH];
      dataBackgroundPixPath[0]=SYSEX_BACKGROUNDPIXPATH;
      strncpy((char*)&dataBackgroundPixPath[1],
	      _backgroundPixPath.toLatin1().constData(), 
	      MAXSTRLENGTHBACKGROUNDPIXPATH);
      MusECore::MidiPlayEvent
	evBackgroundPixPath(0, 0, MusECore::ME_SYSEX,
			    (const unsigned char*)dataBackgroundPixPath,
			    1+MAXSTRLENGTHBACKGROUNDPIXPATH);
      _gui->writeEvent(evBackgroundPixPath);
    }
    qdn = qdn.nextSibling();
  }
  //send colors
  unsigned char dataColorGui[COLORSYSEXLENGTH+1];
  dataColorGui[0]=SYSEX_COLORGUI;
  dataColorGui[1]=(unsigned char)textColor.red();
  dataColorGui[2]=(unsigned char)textColor.green();
  dataColorGui[3]=(unsigned char)textColor.blue();
  dataColorGui[4]=(unsigned char)backgroundColor.red();
  dataColorGui[5]=(unsigned char)backgroundColor.green();
  dataColorGui[6]=(unsigned char)backgroundColor.blue();
  dataColorGui[7]=(unsigned char)editTextColor.red();
  dataColorGui[8]=(unsigned char)editTextColor.green();
  dataColorGui[9]=(unsigned char)editTextColor.blue();
  dataColorGui[10]=(unsigned char)editBackgroundColor.red();
  dataColorGui[11]=(unsigned char)editBackgroundColor.green();
  dataColorGui[12]=(unsigned char)editBackgroundColor.blue();
  MusECore::MidiPlayEvent evSysexColor(0, 0, MusECore::ME_SYSEX, (const unsigned char*)dataColorGui,
			 COLORSYSEXLENGTH+1);
  _gui->writeEvent(evSysexColor);
}

//-----------------------------------------------------------
// loadConfiguration
//-----------------------------------------------------------
void DeicsOnze::loadConfiguration(QString fileName) {
  // read the XML file and create DOM tree
  if(!fileName.isEmpty()) {
    QFile confFile(fileName);
    if(!confFile.open(QIODevice::ReadOnly)) {
      printf("Critical Error. Cannot open file %s\n",
	     fileName.toAscii().data());
      return;
    }
    QDomDocument domTree;
    if (!domTree.setContent(&confFile )) {
	printf("Critical Error. Parsing error for file %s\n",
	       fileName.toAscii().data());
      confFile.close();
      return;
    }

    confFile.close();

    QDomNode node = domTree.documentElement();
    while (!node.isNull()) {
      QDomElement e = node.toElement();
      if (e.isNull())
	continue;
      if (e.tagName() == DEICSONZECONFIGURATIONSTR) {
	QString version = e.attribute(QString("version"));
	if (version == "1.0") {
	  readConfiguration(node.firstChild());
	}
	else printf("unsupported *.dco file version %s\n",
		    version.toLatin1().constData());
      }
      else printf("DeicsOnze: %s not supported\n",
		  e.tagName().toLatin1().constData());
      node = node.nextSibling();
    }
  }
}

//---------------------------------------------------------
// writeConfiguration
//---------------------------------------------------------
void DeicsOnze::writeConfiguration(AL::Xml* xml) {
  QString str;
  xml->stag("deicsOnzeConfiguation version=\"1.0\"");
  //xml->intTag(NBRVOICESSTR, (int)_global.nbrVoices);
  //xml->strTag(CHANNELNUMSTR, (_global.channelNum==-1?ALLSTR:
  //                            str.setNum(_global.channelNum+1)));
  xml->tag(QUALITYSTR, QString((_global.quality==high?HIGHSTR:
			   (_global.quality==middle?MIDDLESTR:
			    (_global.quality==low?LOWSTR:ULTRALOWSTR)))));
  xml->tag(FILTERSTR, QString(getFilter()==true?YESSTRDEI:NOSTRDEI));
  xml->tag(FONTSIZESTR, _global.fontSize);
  xml->tag(SAVECONFIGSTR, QString((_saveConfig?YESSTRDEI:NOSTRDEI)));
  xml->tag(SAVEONLYUSEDSTR, QString((_saveOnlyUsed?YESSTRDEI:NOSTRDEI)));
  xml->tag(TEXTCOLORSTR,
		reinterpret_cast<const QColor &>(*_gui->tColor));
  xml->tag(BACKGROUNDCOLORSTR,
		reinterpret_cast<const QColor &>(*_gui->bColor));
  xml->tag(EDITTEXTCOLORSTR,
		reinterpret_cast<const QColor &>(*_gui->etColor));
  xml->tag(EDITBACKGROUNDCOLORSTR,
		reinterpret_cast<const QColor &>(*_gui->ebColor));
  xml->tag(ISINITSETSTR, QString((_isInitSet?YESSTRDEI:NOSTRDEI)));
  xml->tag(INITSETPATHSTR, QString(_initSetPath));
  xml->tag(ISBACKGROUNDPIXSTR, QString((_isBackgroundPix?YESSTRDEI:NOSTRDEI)));
  xml->tag(BACKGROUNDPIXPATHSTR, _backgroundPixPath);

  xml->etag(DEICSONZECONFIGURATIONSTR);
}

//---------------------------------------------------------
// getInitBuffer
//---------------------------------------------------------
void DeicsOnze::setupInitBuffer(int len)
{
  if (len > initLen) {
        if (initBuffer)
              delete [] initBuffer;
        initBuffer = new unsigned char[len];
        initLen = len;    
        }
}

//---------------------------------------------------------
// getInitData
//---------------------------------------------------------
//void DeicsOnze::getInitData(int* length, const unsigned char** data) const {
void DeicsOnze::getInitData(int* length, const unsigned char** data) {
  //write the set in a temporary file and in a QByteArray
  QTemporaryFile file;
  file.open();
  AL::Xml* xml=new AL::Xml(&file);
  xml->header();
  _set->writeSet(xml, _saveOnlyUsed);
  file.reset(); //seek the start of the file
  QByteArray ba = file.readAll();
  file.close();

  //compress the QByteArray at default rate
  QByteArray baComp = qCompress(ba);

  //save the set
  *length = NUM_CONFIGLENGTH                       
  //*length = 2 + NUM_CONFIGLENGTH     // 2 for Header                  
    ///+ (_pluginIReverb?sizeof(float)*_pluginIReverb->plugin()->parameter():0) 
    + (_pluginIReverb?sizeof(float)*_pluginIReverb->plugin()->controlInPorts():0) 
    ///+ (_pluginIChorus?sizeof(float)*_pluginIChorus->plugin()->parameter():0)
    + (_pluginIChorus?sizeof(float)*_pluginIChorus->plugin()->controlInPorts():0)
    + baComp.size();

  ///unsigned char* buffer = new unsigned char[*length];
  setupInitBuffer(*length);  
  
  //save init data
  
  ///buffer[0]=SYSEX_INIT_DATA;
  initBuffer[0]=MUSE_SYNTH_SYSEX_MFG_ID;
  initBuffer[1]=DEICSONZE_UNIQUE_ID;
  initBuffer[2]=SYSEX_INIT_DATA;
  initBuffer[3]=SYSEX_INIT_DATA_VERSION;
  //save global data
  initBuffer[NUM_MASTERVOL] = (unsigned char) getMasterVol();
  for(int c = 0; c < NBRCHANNELS; c++) {
    initBuffer[NUM_CHANNEL_ENABLE + c] = (unsigned char) getChannelEnable(c);
    initBuffer[NUM_CHANNEL_VOL + c] = (unsigned char) getChannelVol(c);
    initBuffer[NUM_CHANNEL_PAN + c] = (unsigned char) getChannelPan(c);
    int b = getChannelBrightness(c);
    initBuffer[NUM_CHANNEL_BRIGHTNESS + 2*c] = (unsigned char) (b%256);
    initBuffer[NUM_CHANNEL_BRIGHTNESS + 2*c + 1] = (unsigned char) (b/256);
    initBuffer[NUM_CHANNEL_MODULATION + c] =
      (unsigned char) getChannelModulation(c);
    initBuffer[NUM_CHANNEL_DETUNE + c] =
      (unsigned char) getChannelDetune(c) + MAXCHANNELDETUNE;
    initBuffer[NUM_CHANNEL_ATTACK + c] = (unsigned char) getChannelAttack(c);
    initBuffer[NUM_CHANNEL_RELEASE + c] = (unsigned char) getChannelRelease(c);
    initBuffer[NUM_CHANNEL_REVERB + c] = (unsigned char) getChannelReverb(c);
    initBuffer[NUM_CHANNEL_CHORUS + c] = (unsigned char) getChannelChorus(c);    
    initBuffer[NUM_CHANNEL_DELAY + c] = (unsigned char) getChannelDelay(c);    
    initBuffer[NUM_CURRENTPROG + c] = (unsigned char) _preset[c]->prog;
    initBuffer[NUM_CURRENTLBANK + c] =
      (unsigned char) _preset[c]->_subcategory->_lbank;
    initBuffer[NUM_CURRENTHBANK + c] =
      (unsigned char) _preset[c]->_subcategory->_category->_hbank;
    initBuffer[NUM_NBRVOICES + c] = (unsigned char) getNbrVoices(c);
  }
  initBuffer[NUM_SAVEONLYUSED]=(unsigned char) _saveOnlyUsed;
  initBuffer[NUM_SAVECONFIG]=(unsigned char) _saveConfig;
  //save config data
  if(_saveConfig) {
    initBuffer[NUM_QUALITY]=(unsigned char)_global.quality;
    initBuffer[NUM_FILTER]=(unsigned char)getFilter();
    initBuffer[NUM_FONTSIZE]=(unsigned char)_global.fontSize;
    initBuffer[NUM_RED_TEXT]=(unsigned char)_gui->tColor->red();
    initBuffer[NUM_GREEN_TEXT]=(unsigned char)_gui->tColor->green();
    initBuffer[NUM_BLUE_TEXT]=(unsigned char)_gui->tColor->blue();
    initBuffer[NUM_RED_BACKGROUND]=(unsigned char)_gui->bColor->red();
    initBuffer[NUM_GREEN_BACKGROUND]=(unsigned char)_gui->bColor->green();
    initBuffer[NUM_BLUE_BACKGROUND]=(unsigned char)_gui->bColor->blue();
    initBuffer[NUM_RED_EDITTEXT]=(unsigned char)_gui->etColor->red();
    initBuffer[NUM_GREEN_EDITTEXT]=(unsigned char)_gui->etColor->green();
    initBuffer[NUM_BLUE_EDITTEXT]=(unsigned char)_gui->etColor->blue();
    initBuffer[NUM_RED_EDITBACKGROUND]=(unsigned char)_gui->ebColor->red();
    initBuffer[NUM_GREEN_EDITBACKGROUND]=(unsigned char)_gui->ebColor->green();
    initBuffer[NUM_BLUE_EDITBACKGROUND]=(unsigned char)_gui->ebColor->blue();
    initBuffer[NUM_ISINITSET]=(unsigned char)_isInitSet;
    strncpy((char*)&initBuffer[NUM_INITSETPATH],
	    _initSetPath.toLatin1().constData(), MAXSTRLENGTHINITSETPATH);
    initBuffer[NUM_ISBACKGROUNDPIX]=(unsigned char)_isBackgroundPix;
    strncpy((char*)&initBuffer[NUM_BACKGROUNDPIXPATH],
	    _backgroundPixPath.toLatin1().constData(),
	    MAXSTRLENGTHBACKGROUNDPIXPATH);
  }
  //FX
  //reverb
  initBuffer[NUM_IS_REVERB_ON]=(unsigned char)_global.isReverbActivated;
  initBuffer[NUM_REVERB_RETURN]=(unsigned char)getReverbReturn();
  initBuffer[NUM_REVERB_PARAM_NBR]=                                         
    ///(_pluginIReverb?(unsigned char)_pluginIReverb->plugin()->parameter() : 0);
    (_pluginIReverb?(unsigned char)_pluginIReverb->plugin()->controlInPorts() : 0);
  strncpy((char*)&initBuffer[NUM_REVERB_LIB],
	  (_pluginIReverb?
	   _pluginIReverb->plugin()->lib().toLatin1().constData() : "\0"),
	  MAXSTRLENGTHFXLIB);
  strncpy((char*)&initBuffer[NUM_REVERB_LABEL],
	  (_pluginIReverb?
	   _pluginIReverb->plugin()->label().toLatin1().constData() : "\0"),
	  MAXSTRLENGTHFXLABEL);
  //chorus
  initBuffer[NUM_IS_CHORUS_ON]=(unsigned char)_global.isChorusActivated;
  initBuffer[NUM_CHORUS_RETURN]=(unsigned char)getChorusReturn();
  initBuffer[NUM_CHORUS_PARAM_NBR]=                                         
    ///(_pluginIChorus?(unsigned char)_pluginIChorus->plugin()->parameter() : 0);
    (_pluginIChorus?(unsigned char)_pluginIChorus->plugin()->controlInPorts() : 0);
  strncpy((char*)&initBuffer[NUM_CHORUS_LIB],
	  (_pluginIChorus?
	   _pluginIChorus->plugin()->lib().toLatin1().constData() : "\0"),
	  MAXSTRLENGTHFXLIB);
  strncpy((char*)&initBuffer[NUM_CHORUS_LABEL],
	  (_pluginIChorus?
	   _pluginIChorus->plugin()->label().toLatin1().constData() : "\0"),
	  MAXSTRLENGTHFXLABEL);
  //delay
  initBuffer[NUM_IS_DELAY_ON]=(unsigned char)_global.isDelayActivated;
  initBuffer[NUM_DELAY_RETURN]=(unsigned char)getDelayReturn();
  //save FX parameters
  //reverb
  for(int i = 0; i < (int)initBuffer[NUM_REVERB_PARAM_NBR]; i++) {
    float val = (float)getReverbParam(i);
    memcpy(&initBuffer[NUM_CONFIGLENGTH + sizeof(float)*i], &val, sizeof(float));
  }
  //chorus
  for(int i = 0; i < (int)initBuffer[NUM_CHORUS_PARAM_NBR]; i++) {
    float val = (float)getChorusParam(i);
    memcpy(&initBuffer[NUM_CONFIGLENGTH
		   + sizeof(float)*(int)initBuffer[NUM_REVERB_PARAM_NBR]
		   + sizeof(float)*i], &val, sizeof(float));
  }
  //delay
  float delayfloat;
  delayfloat = getDelayBPM();
  memcpy(&initBuffer[NUM_DELAY_BPM], &delayfloat, 4);
  delayfloat = getDelayBeatRatio();
  memcpy(&initBuffer[NUM_DELAY_BEATRATIO], &delayfloat, sizeof(float));
  delayfloat = getDelayFeedback();
  memcpy(&initBuffer[NUM_DELAY_FEEDBACK], &delayfloat, sizeof(float));
  delayfloat = getDelayLFOFreq();
  memcpy(&initBuffer[NUM_DELAY_LFO_FREQ], &delayfloat, sizeof(float));
  delayfloat = getDelayLFODepth();
  memcpy(&initBuffer[NUM_DELAY_LFO_DEPTH], &delayfloat, sizeof(float));

  //save set data
  int offset =
    NUM_CONFIGLENGTH
    + sizeof(float)*(int)initBuffer[NUM_REVERB_PARAM_NBR]
    + sizeof(float)*(int)initBuffer[NUM_CHORUS_PARAM_NBR];
  for(int i = offset; i < *length; i++)
    initBuffer[i]=(unsigned char)baComp.at(i - offset);

  ///*data=buffer;
  *data=initBuffer;
}
//---------------------------------------------------------
// parseInitData
//---------------------------------------------------------
void DeicsOnze::parseInitData(int length, const unsigned char* data) {
  ///if(data[1]==SYSEX_INIT_DATA_VERSION) {
  if(data[3]==SYSEX_INIT_DATA_VERSION) {
    //load global parameters
    //master volume
    setMasterVol(data[NUM_MASTERVOL]);
    unsigned char *dataMasterVol = new unsigned char[2];
    dataMasterVol[0]=SYSEX_MASTERVOL;
    dataMasterVol[1]=(unsigned char) getMasterVol();
    MusECore::MidiPlayEvent 
      evMasterVol(0, 0, MusECore::ME_SYSEX, (const unsigned char*)dataMasterVol, 2);
    _gui->writeEvent(evMasterVol);
    //channel configuration
    for(int c = 0; c < NBRCHANNELS; c++) {
      //isEnable
      setChannelEnable(c, data[NUM_CHANNEL_ENABLE + c]);
      MusECore::MidiPlayEvent 
	evChEnable(0, 0, c, MusECore::ME_CONTROLLER,
		   CTRL_CHANNELENABLE, data[NUM_CHANNEL_ENABLE + c]);
      _gui->writeEvent(evChEnable);
      //nbrVoices
      setNbrVoices(c, data[NUM_NBRVOICES + c]);
      MusECore::MidiPlayEvent 
	evNbrVoices(0, 0, c,MusECore::ME_CONTROLLER,CTRL_NBRVOICES, data[NUM_NBRVOICES + c]);
      _gui->writeEvent(evNbrVoices);
      //channel volume
      setChannelVol(c, data[NUM_CHANNEL_VOL + c]);
      MusECore::MidiPlayEvent
	evChVol(0, 0, c, MusECore::ME_CONTROLLER,
		CTRL_CHANNELVOLUME, data[NUM_CHANNEL_VOL + c]);
      _gui->writeEvent(evChVol);
      //channel pan
      setChannelPan(c, data[NUM_CHANNEL_PAN + c]);
      MusECore::MidiPlayEvent
	evChPan(0, 0, c, MusECore::ME_CONTROLLER, CTRL_CHANNELPAN,
		data[NUM_CHANNEL_PAN + c]);
      _gui->writeEvent(evChPan);
      if(getChannelEnable(c)) applyChannelAmp(c);
      //channel detune
      setChannelDetune(c, data[NUM_CHANNEL_DETUNE + c]-MAXCHANNELDETUNE);
      MusECore::MidiPlayEvent
	evChDetune(0, 0, c, MusECore::ME_CONTROLLER, CTRL_CHANNELDETUNE,
		   data[NUM_CHANNEL_DETUNE + c]-MAXCHANNELDETUNE);
      _gui->writeEvent(evChDetune);
      //channel brightness
      setChannelBrightness(c,
			   data[NUM_CHANNEL_BRIGHTNESS + 2*c]
			   + data[NUM_CHANNEL_BRIGHTNESS + 2*c + 1] * 256);
      MusECore::MidiPlayEvent
	evChBrightness(0, 0, c, MusECore::ME_CONTROLLER,
		       CTRL_FINEBRIGHTNESS, getChannelBrightness(c));
      _gui->writeEvent(evChBrightness);
      //channel modulation
      setChannelModulation(c, data[NUM_CHANNEL_MODULATION + c]);
      MusECore::MidiPlayEvent 
	evChMod(0, 0, c, MusECore::ME_CONTROLLER,
		MusECore::CTRL_MODULATION, data[NUM_CHANNEL_MODULATION + c]);
      _gui->writeEvent(evChMod);
      //channel attack
      setChannelAttack(c, data[NUM_CHANNEL_ATTACK + c]);
      MusECore::MidiPlayEvent 
	evChAttack(0, 0, c, MusECore::ME_CONTROLLER,
		   MusECore::CTRL_ATTACK_TIME, data[NUM_CHANNEL_ATTACK + c]);
      _gui->writeEvent(evChAttack);
      //channel release
      setChannelRelease(c, data[NUM_CHANNEL_RELEASE + c]);
      MusECore::MidiPlayEvent 
	evChRelease(0, 0, c, MusECore::ME_CONTROLLER,
		    MusECore::CTRL_RELEASE_TIME, data[NUM_CHANNEL_RELEASE + c]);
      _gui->writeEvent(evChRelease);      
      //channel reverb
      setChannelReverb(c, data[NUM_CHANNEL_REVERB + c]);
      MusECore::MidiPlayEvent 
	evChReverb(0, 0, c, MusECore::ME_CONTROLLER,
		   MusECore::CTRL_REVERB_SEND, data[NUM_CHANNEL_REVERB + c]);
      _gui->writeEvent(evChReverb);      
      //channel chorus
      setChannelChorus(c, data[NUM_CHANNEL_CHORUS + c]);
      MusECore::MidiPlayEvent 
	evChChorus(0, 0, c, MusECore::ME_CONTROLLER,
		   MusECore::CTRL_CHORUS_SEND, data[NUM_CHANNEL_CHORUS + c]);
      _gui->writeEvent(evChChorus);      
      //channel delay
      setChannelDelay(c, data[NUM_CHANNEL_DELAY + c]);
      MusECore::MidiPlayEvent 
	evChDelay(0, 0, c, MusECore::ME_CONTROLLER,
		  MusECore::CTRL_VARIATION_SEND, data[NUM_CHANNEL_DELAY + c]);
      _gui->writeEvent(evChDelay);
    }
    //load configuration
    _saveConfig = (bool)data[NUM_SAVECONFIG];
    unsigned char *dataSaveConfig = new unsigned char[2];
    dataSaveConfig[0]=SYSEX_SAVECONFIG;
    dataSaveConfig[1]=(unsigned char)_saveConfig;
    MusECore::MidiPlayEvent 
      evSaveConfig(0, 0, MusECore::ME_SYSEX, (const unsigned char*)dataSaveConfig, 2);
    _gui->writeEvent(evSaveConfig);    
    if(_saveConfig) {
      //saveOnlyUsed
      _saveOnlyUsed = (bool)data[NUM_SAVEONLYUSED];
      unsigned char *dataSaveOnlyUsed = new unsigned char[2];
      dataSaveOnlyUsed[0]=SYSEX_SAVEONLYUSED;
      dataSaveOnlyUsed[1]=(unsigned char)_saveOnlyUsed;
      MusECore::MidiPlayEvent 
	evSaveOnlyUsed(0, 0, MusECore::ME_SYSEX, (const unsigned char*)dataSaveOnlyUsed, 2);
      _gui->writeEvent(evSaveOnlyUsed);    
      //colors
      unsigned char dataColorGui[COLORSYSEXLENGTH+1];
      dataColorGui[0]=SYSEX_COLORGUI;
      for (int i=0; i<COLORSYSEXLENGTH; i++)
	dataColorGui[i+1]=data[NUM_RED_TEXT+i];
      MusECore::MidiPlayEvent evSysexColor(0, 0, MusECore::ME_SYSEX, (const unsigned char*)dataColorGui,
			     COLORSYSEXLENGTH+1);
      _gui->writeEvent(evSysexColor);
      //quality
      unsigned char dataQuality[2];
      dataQuality[0]=SYSEX_QUALITY;
      dataQuality[1]=data[NUM_QUALITY];
      setQuality((Quality)data[NUM_QUALITY]);
      MusECore::MidiPlayEvent evQuality(0, 0, MusECore::ME_SYSEX, (const unsigned char*)dataQuality, 2);
      _gui->writeEvent(evQuality);
      //filter
      unsigned char dataFilter[2];
      dataFilter[0]=SYSEX_FILTER;
      dataFilter[1]=data[NUM_FILTER];
      setFilter((bool)data[NUM_FILTER]);
      MusECore::MidiPlayEvent evFilter(0, 0, MusECore::ME_SYSEX, (const unsigned char*)dataFilter, 2);
      _gui->writeEvent(evFilter);
      //font size
      unsigned char dataFontSize[2];
      dataFontSize[0]=SYSEX_FONTSIZE;
      dataFontSize[1]=data[NUM_FONTSIZE];
      MusECore::MidiPlayEvent evFontSize(0, 0, MusECore::ME_SYSEX, (const unsigned char*)dataFontSize, 2);
      _gui->writeEvent(evFontSize);
      //load init set
      unsigned char dataIsInitSet[2];
      dataIsInitSet[0]=SYSEX_ISINITSET;
      dataIsInitSet[1]=data[NUM_ISINITSET];
      MusECore::MidiPlayEvent evIsInitSet(0, 0, MusECore::ME_SYSEX,
			    (const unsigned char*)dataIsInitSet, 2);
      _gui->writeEvent(evIsInitSet);
      unsigned char dataInitSetPath[1+MAXSTRLENGTHINITSETPATH];
      dataInitSetPath[0]=SYSEX_INITSETPATH;
      for(int a = 0; a < MAXSTRLENGTHINITSETPATH; a++)
	dataInitSetPath[a+1] = data[a+NUM_INITSETPATH];
      MusECore::MidiPlayEvent evInitSetPath(0, 0, MusECore::ME_SYSEX,(const unsigned char*)dataInitSetPath,
			      1+MAXSTRLENGTHINITSETPATH);
      _gui->writeEvent(evInitSetPath);      
      //load background pix
      unsigned char dataIsBackgroundPix[2];
      dataIsBackgroundPix[0]=SYSEX_ISBACKGROUNDPIX;
      dataIsBackgroundPix[1]=data[NUM_ISBACKGROUNDPIX];
      MusECore::MidiPlayEvent evIsBackgroundPix(0, 0, MusECore::ME_SYSEX,
			    (const unsigned char*)dataIsBackgroundPix, 2);
      _gui->writeEvent(evIsBackgroundPix);
      unsigned char dataBackgroundPixPath[1+MAXSTRLENGTHBACKGROUNDPIXPATH];
      dataBackgroundPixPath[0]=SYSEX_BACKGROUNDPIXPATH;
      for(int a = 0; a < MAXSTRLENGTHBACKGROUNDPIXPATH; a++)
	dataBackgroundPixPath[a+1] = data[a+NUM_BACKGROUNDPIXPATH];
      MusECore::MidiPlayEvent evBackgroundPixPath(0, 0, MusECore::ME_SYSEX,
			      (const unsigned char*)dataBackgroundPixPath,
			      1+MAXSTRLENGTHBACKGROUNDPIXPATH);
      _gui->writeEvent(evBackgroundPixPath);      
    }
    else _gui->saveConfigCheckBox->setChecked(false);
    //load FX
    //reverb
    _global.isReverbActivated = (bool)data[NUM_IS_REVERB_ON];
    unsigned char *dataReverbAct = new unsigned char[2];
    dataReverbAct[0]=SYSEX_REVERBACTIV;
    dataReverbAct[1]=(unsigned char)_global.isReverbActivated;
    MusECore::MidiPlayEvent evReverbAct(0, 0, MusECore::ME_SYSEX,(const unsigned char*)dataReverbAct, 2);
    _gui->writeEvent(evReverbAct);    
    setReverbReturn((int)data[NUM_REVERB_RETURN]);
    unsigned char *dataReverbRet = new unsigned char[2];
    dataReverbRet[0]=SYSEX_REVERBRETURN;
    dataReverbRet[1]=(unsigned char)getReverbReturn();
    MusECore::MidiPlayEvent evReverbRet(0, 0, MusECore::ME_SYSEX,(const unsigned char*)dataReverbRet, 2);
    _gui->writeEvent(evReverbRet);
    MusECore::Plugin* p;
    p = MusEGlobal::plugins.find((const char*)&data[NUM_REVERB_LIB], 
		     (const char*)&data[NUM_REVERB_LABEL]);
    if(p) { 
      initPluginReverb(p);
      ///for(int i = 0; i < _pluginIReverb->plugin()->parameter(); i++) {
      for(int i = 0; i < (int)_pluginIReverb->plugin()->controlInPorts(); i++) {
	float val;
	memcpy(&val, &data[NUM_CONFIGLENGTH + sizeof(float)*i], sizeof(float));
	setReverbParam(i, (double)val);
      }
      char dataBuildRev;
      dataBuildRev = SYSEX_BUILDGUIREVERB;
      MusECore::MidiPlayEvent evSysexBuildRev(0, 0, MusECore::ME_SYSEX,
				(const unsigned char*)&dataBuildRev, 1);
      _gui->writeEvent(evSysexBuildRev);
    }
    else _pluginIReverb = NULL;
    //chorus
    _global.isChorusActivated = (bool)data[NUM_IS_CHORUS_ON];
    unsigned char *dataChorusAct = new unsigned char[2];
    dataChorusAct[0]=SYSEX_CHORUSACTIV;
    dataChorusAct[1]=(unsigned char)_global.isChorusActivated;
    MusECore::MidiPlayEvent evChorusAct(0, 0, MusECore::ME_SYSEX,(const unsigned char*)dataChorusAct, 2);
    _gui->writeEvent(evChorusAct);    
    setChorusReturn((int)data[NUM_CHORUS_RETURN]);
    unsigned char *dataChorusRet = new unsigned char[2];
    dataChorusRet[0]=SYSEX_CHORUSRETURN;
    dataChorusRet[1]=(unsigned char)getChorusReturn();
    MusECore::MidiPlayEvent evChorusRet(0, 0, MusECore::ME_SYSEX,(const unsigned char*)dataChorusRet, 2);
    _gui->writeEvent(evChorusRet);
    p = MusEGlobal::plugins.find((const char*)&data[NUM_CHORUS_LIB], 
		     (const char*)&data[NUM_CHORUS_LABEL]);
    if(p) {
      initPluginChorus(p);
      ///for(int i = 0; i < _pluginIChorus->plugin()->parameter(); i++) {
      for(int i = 0; i < (int)_pluginIChorus->plugin()->controlInPorts(); i++) {
	float val;
	memcpy(&val, &data[NUM_CONFIGLENGTH
			   + sizeof(float)*(int)data[NUM_REVERB_PARAM_NBR]
			   + sizeof(float)*i],
	       sizeof(float));
	setChorusParam(i, (double)val);
      }
      char dataBuildCho;
      dataBuildCho = SYSEX_BUILDGUICHORUS;
      MusECore::MidiPlayEvent evSysexBuildCho(0, 0, MusECore::ME_SYSEX,
				(const unsigned char*)&dataBuildCho, 1);
      _gui->writeEvent(evSysexBuildCho);
    }
    else _pluginIChorus = NULL;
    //delay
    _global.isDelayActivated = (bool)data[NUM_IS_DELAY_ON];
    unsigned char *dataDelayAct = new unsigned char[2];
    dataDelayAct[0]=SYSEX_DELAYACTIV;
    dataDelayAct[1]=(unsigned char)_global.isDelayActivated;
    MusECore::MidiPlayEvent evDelayAct(0, 0, MusECore::ME_SYSEX,(const unsigned char*)dataDelayAct, 2);
    _gui->writeEvent(evDelayAct);    
    setDelayReturn((int)data[NUM_DELAY_RETURN]);
    unsigned char *dataDelayRet = new unsigned char[2];
    dataDelayRet[0]=SYSEX_DELAYRETURN;
    dataDelayRet[1]=(unsigned char)getDelayReturn();
    MusECore::MidiPlayEvent evDelayRet(0, 0, MusECore::ME_SYSEX,(const unsigned char*)dataDelayRet, 2);
    _gui->writeEvent(evDelayRet);    
    //initPluginDelay(MusEGlobal::plugins.find("pandelay", "pandelay"));
    float delayfloat;
    memcpy(&delayfloat, &data[NUM_DELAY_BPM], sizeof(float));
    setDelayBPM(delayfloat);
    char dataDelayBPM[sizeof(float)+1];
    dataDelayBPM[0] = SYSEX_DELAYBPM;
    memcpy(&dataDelayBPM[1], &delayfloat, sizeof(float));
    MusECore::MidiPlayEvent evSysexDelayBPM(0, 0, MusECore::ME_SYSEX,
			      (const unsigned char*)dataDelayBPM,
			      sizeof(float)+1);
    _gui->writeEvent(evSysexDelayBPM);
    memcpy(&delayfloat, &data[NUM_DELAY_BEATRATIO], sizeof(float));
    setDelayBeatRatio(delayfloat);
    char dataDelayBeatRatio[sizeof(float)+1];
    dataDelayBeatRatio[0] = SYSEX_DELAYBEATRATIO;
    memcpy(&dataDelayBeatRatio[1], &delayfloat, sizeof(float));
    MusECore::MidiPlayEvent evSysexDelayBeatRatio(0, 0, MusECore::ME_SYSEX,
				    (const unsigned char*)dataDelayBeatRatio,
				    sizeof(float)+1);
    _gui->writeEvent(evSysexDelayBeatRatio);
    memcpy(&delayfloat, &data[NUM_DELAY_FEEDBACK], sizeof(float));
    setDelayFeedback(delayfloat);
    char dataDelayFeedback[sizeof(float)+1];
    dataDelayFeedback[0] = SYSEX_DELAYFEEDBACK;
    memcpy(&dataDelayFeedback[1], &delayfloat, sizeof(float));
    MusECore::MidiPlayEvent evSysexDelayFeedback(0, 0, MusECore::ME_SYSEX,
				   (const unsigned char*)dataDelayFeedback,
				   sizeof(float)+1);
    _gui->writeEvent(evSysexDelayFeedback);
    memcpy(&delayfloat, &data[NUM_DELAY_LFO_FREQ], sizeof(float));
    setDelayLFOFreq(delayfloat);
    char dataDelayLFOFreq[sizeof(float)+1];
    dataDelayLFOFreq[0] = SYSEX_DELAYLFOFREQ;
    memcpy(&dataDelayLFOFreq[1], &delayfloat, sizeof(float));
    MusECore::MidiPlayEvent evSysexDelayLFOFreq(0, 0, MusECore::ME_SYSEX,
				  (const unsigned char*)dataDelayLFOFreq,
				  sizeof(float)+1);
    _gui->writeEvent(evSysexDelayLFOFreq);
    memcpy(&delayfloat, &data[NUM_DELAY_LFO_DEPTH], sizeof(float));
    setDelayLFODepth(delayfloat);
    char dataDelayLFODepth[sizeof(float)+1];
    dataDelayLFODepth[0] = SYSEX_DELAYLFODEPTH;
    memcpy(&dataDelayLFODepth[1], &delayfloat, sizeof(float));
    MusECore::MidiPlayEvent evSysexDelayLFODepth(0, 0, MusECore::ME_SYSEX,
				   (const unsigned char*)dataDelayLFODepth,
				   sizeof(float)+1);
    _gui->writeEvent(evSysexDelayLFODepth);

    //load the set compressed
    int offset =
      NUM_CONFIGLENGTH 
      + sizeof(float)*(int)data[NUM_REVERB_PARAM_NBR]
      + sizeof(float)*(int)data[NUM_CHORUS_PARAM_NBR];
    QByteArray baComp = QByteArray((const char*)&data[offset], length-offset);
    
    //uncompress the set
    QByteArray baUncomp = qUncompress(baComp);

    //save the set in a temporary file and
    // read the XML file and create DOM tree
    QTemporaryFile file;
    file.open();
    file.write(baUncomp);
    QDomDocument domTree;
    file.reset(); //seek the start of the file
    domTree.setContent(&file);
    file.close();
    QDomNode node = domTree.documentElement();
    
    while (!node.isNull()) {
      QDomElement e = node.toElement();
      if (e.isNull())
	continue;
      if (e.tagName() == "deicsOnzeSet") {
	QString version = e.attribute(QString("version"));
	if (version == "1.0") {
	  for(int c = 0; c < NBRCHANNELS; c++) _preset[c]=_initialPreset;
	  //read the set
	  if((bool)data[NUM_SAVEONLYUSED]) {
	    //printf("Mini\n");
	    //updateSaveOnlyUsed(true);
	  }
	  else {
	    //printf("Huge\n");
	    while(!_set->_categoryVector.empty())
	      delete(*_set->_categoryVector.begin());
	    //updateSaveOnlyUsed(false);
	  }
	  _set->readSet(node.firstChild());
	  //display load preset
	  //setSet();
	}
	else printf("Wrong set version : %s\n",
		    version.toLatin1().constData());
      }
      node = node.nextSibling();
    }
    //send sysex to the gui to load the set (actually not because it doesn't
    //work -the code is just zapped in the middle???-, so it is done above
    //int dL=2+baUncomp.size();
    int dL = 2;
    char dataSend[dL];
    dataSend[0]=SYSEX_LOADSET;
    dataSend[1]=data[NUM_SAVEONLYUSED];
    //for(int i=2; i<dL; i++) dataSend[i]=baUncop.at(i-2);
    MusECore::MidiPlayEvent evSysex(0, 0, MusECore::ME_SYSEX,(const unsigned char*)dataSend, dL);
    _gui->writeEvent(evSysex);

    //select programs per channel
    for(int c = 0; c < NBRCHANNELS; c++) {
      int hbank=(int)data[NUM_CURRENTHBANK+c];
      int lbank=(int)data[NUM_CURRENTLBANK+c];
      int prog=(int)data[NUM_CURRENTPROG+c];
      programSelect(c, hbank, lbank, prog);
      int val=prog+(lbank<<8)+(hbank<<16);
      MusECore::MidiPlayEvent evProgSel(0, 0, c, MusECore::ME_CONTROLLER, MusECore::CTRL_PROGRAM, val);
      _gui->writeEvent(evProgSel);
    }

  }
}
//---------------------------------------------------------
// sysex
//---------------------------------------------------------
bool DeicsOnze::sysex(int length, const unsigned char* data) {
  sysex(length, data, false);
  return false;
}
bool DeicsOnze::sysex(int length, const unsigned char* data, bool fromGui) {
  
  if(length < 3 || data[0] != MUSE_SYNTH_SYSEX_MFG_ID 
      || data[1] != DEICSONZE_UNIQUE_ID) 
  {
    #ifdef DEICSONZE_DEBUG
    printf("MusE DeicsOnze: Unknown sysex header\n");
    #endif
    return false;
  }
  
  int l = length - 2;
  const unsigned char* d = data + 2;
  ///int cmd=data[0];
  int cmd=d[0];
  int index;
  float f;
  switch(cmd) {
  case SYSEX_INIT_DATA:
    parseInitData(length, data);
    //parseInitData(l, d);
    break;
  case SYSEX_MASTERVOL:
    ///setMasterVol((int)data[1]);
    setMasterVol((int)d[1]);
    if(!fromGui) {
      ///MusECore::MidiPlayEvent evSysex(0, 0, MusECore::ME_SYSEX, data, length);
      MusECore::MidiPlayEvent evSysex(0, 0, MusECore::ME_SYSEX, d, l);
      _gui->writeEvent(evSysex);
    }
    break;
    //case SYSEX_CHANNELNUM:
    ///_global.channelNum = (char)data[1];
    //_global.channelNum = (char)d[1];
    //if(!fromGui) {
    ///  MusECore::MidiPlayEvent evSysex(0, 0, MusECore::ME_SYSEX, data, length);
    //  MusECore::MidiPlayEvent evSysex(0, 0, MusECore::ME_SYSEX, d, l);
    //  _gui->writeEvent(evSysex);
    //}
    //break;
  case SYSEX_QUALITY:
    ///setQuality((Quality)data[1]);
    setQuality((Quality)d[1]);
    if(!fromGui) {
      ///MusECore::MidiPlayEvent evSysex(0, 0, MusECore::ME_SYSEX, data, length);
      MusECore::MidiPlayEvent evSysex(0, 0, MusECore::ME_SYSEX, d, l);
      _gui->writeEvent(evSysex);
    }
    break;
  case SYSEX_FILTER:
    ///setFilter((bool)data[1]);
    setFilter((bool)d[1]);
    if(!fromGui) {
      ///MusECore::MidiPlayEvent evSysex(0, 0, MusECore::ME_SYSEX, data, length);
      MusECore::MidiPlayEvent evSysex(0, 0, MusECore::ME_SYSEX, d, l);
      _gui->writeEvent(evSysex);
    }
    break;
  case SYSEX_FONTSIZE:
    ///_global.fontSize = (int)data[1];
    _global.fontSize = (int)d[1];
    if(!fromGui) {
      ///MusECore::MidiPlayEvent evSysex(0, 0, MusECore::ME_SYSEX, data, length);
      MusECore::MidiPlayEvent evSysex(0, 0, MusECore::ME_SYSEX, d, l);
      _gui->writeEvent(evSysex);
    }
    break;
  case SYSEX_SAVECONFIG:
    ///_saveConfig = (bool)data[1];
    _saveConfig = (bool)d[1];
    if(!fromGui) {
      ///MusECore::MidiPlayEvent evSysex(0, 0, MusECore::ME_SYSEX, data, length);
      MusECore::MidiPlayEvent evSysex(0, 0, MusECore::ME_SYSEX, d, l);
      _gui->writeEvent(evSysex);
    }
    break;
  case SYSEX_SAVEONLYUSED:
    ///_saveOnlyUsed = (bool)data[1];
    _saveOnlyUsed = (bool)d[1];
    if(!fromGui) {
      ///MusECore::MidiPlayEvent evSysex(0, 0, MusECore::ME_SYSEX, data, length);
      MusECore::MidiPlayEvent evSysex(0, 0, MusECore::ME_SYSEX, d, l);
      _gui->writeEvent(evSysex);
    }
    break;
  case SYSEX_ISINITSET:
    ///_isInitSet = (bool)data[1];
    _isInitSet = (bool)d[1];
    if(!fromGui) {
      ///MusECore::MidiPlayEvent evSysex(0, 0, MusECore::ME_SYSEX, data, length);
      MusECore::MidiPlayEvent evSysex(0, 0, MusECore::ME_SYSEX, d, l);
      _gui->writeEvent(evSysex);
    }
    break;
  case SYSEX_INITSETPATH:
    ///_initSetPath = (char*)&data[1];
    _initSetPath = (char*)&d[1];
    if(!fromGui) {
      ///MusECore::MidiPlayEvent evSysex(0, 0, MusECore::ME_SYSEX, data, length);
      MusECore::MidiPlayEvent evSysex(0, 0, MusECore::ME_SYSEX, d, l);
      _gui->writeEvent(evSysex);
    }
    break;
  case SYSEX_ISBACKGROUNDPIX:
    ///_isBackgroundPix = (bool)data[1];
    _isBackgroundPix = (bool)d[1];
    if(!fromGui) {
      ///MusECore::MidiPlayEvent evSysex(0, 0, MusECore::ME_SYSEX, data, length);
      MusECore::MidiPlayEvent evSysex(0, 0, MusECore::ME_SYSEX, d, l);
      _gui->writeEvent(evSysex);
    }
    break;
  case SYSEX_BACKGROUNDPIXPATH:
    ///_backgroundPixPath = (char*)&data[1];
    _backgroundPixPath = (char*)&d[1];
    if(!fromGui) {
      ///MusECore::MidiPlayEvent evSysex(0, 0, MusECore::ME_SYSEX, data, length);
      MusECore::MidiPlayEvent evSysex(0, 0, MusECore::ME_SYSEX, d, l);
      _gui->writeEvent(evSysex);
    }
    break;
  case SYSEX_PANIC:
    resetVoices();
    break;
  case SYSEX_CHORUSACTIV:
    ///_global.isChorusActivated = (bool)data[1];
    _global.isChorusActivated = (bool)d[1];
    if(!fromGui) {
      ///MusECore::MidiPlayEvent evSysex(0, 0, MusECore::ME_SYSEX, data, length);
      MusECore::MidiPlayEvent evSysex(0, 0, MusECore::ME_SYSEX, d, l);
      _gui->writeEvent(evSysex);
    }
    break;
  case SYSEX_CHORUSPARAM:
    ///index = (int)data[1];
    ///memcpy(&f, &data[2], sizeof(float));
    index = (int)d[1];
    memcpy(&f, &d[2], sizeof(float));
    setChorusParam(index, (double)f);
    if(!fromGui) {
      ///MusECore::MidiPlayEvent evSysex(0, 0, MusECore::ME_SYSEX, data, length);
      MusECore::MidiPlayEvent evSysex(0, 0, MusECore::ME_SYSEX, d, l);
      _gui->writeEvent(evSysex);
    }
    break;       
  case SYSEX_REVERBACTIV:
    ///_global.isReverbActivated = (bool)data[1];
    _global.isReverbActivated = (bool)d[1];
    if(!fromGui) {
      ///MusECore::MidiPlayEvent evSysex(0, 0, MusECore::ME_SYSEX, data, length);
      MusECore::MidiPlayEvent evSysex(0, 0, MusECore::ME_SYSEX, d, l);
      _gui->writeEvent(evSysex);
    }
    break;
  case SYSEX_REVERBPARAM:
    ///index = (int)data[1];
    ///memcpy(&f, &data[2], sizeof(float));
    index = (int)d[1];
    memcpy(&f, &d[2], sizeof(float));
    setReverbParam(index, (double)f);
    if(!fromGui) {
      ///MusECore::MidiPlayEvent evSysex(0, 0, MusECore::ME_SYSEX, data, length);
      MusECore::MidiPlayEvent evSysex(0, 0, MusECore::ME_SYSEX, d, l);
      _gui->writeEvent(evSysex);
    }
    break;       
  case SYSEX_DELAYACTIV:
    ///_global.isDelayActivated = (bool)data[1];
    _global.isDelayActivated = (bool)d[1];
    if(!fromGui) {
      ///MusECore::MidiPlayEvent evSysex(0, 0, MusECore::ME_SYSEX, data, length);
      MusECore::MidiPlayEvent evSysex(0, 0, MusECore::ME_SYSEX, d, l);
      _gui->writeEvent(evSysex);
    }
    break;
  case SYSEX_CHORUSRETURN:
    ///setChorusReturn((int)data[1]);
    setChorusReturn((int)d[1]);
    if(!fromGui) {
      ///MusECore::MidiPlayEvent evSysex(0, 0, MusECore::ME_SYSEX, data, length);
      MusECore::MidiPlayEvent evSysex(0, 0, MusECore::ME_SYSEX, d, l);
      _gui->writeEvent(evSysex);
    }
    break;
  case SYSEX_REVERBRETURN:
    ///setReverbReturn((int)data[1]);
    setReverbReturn((int)d[1]);
    if(!fromGui) {
      ///MusECore::MidiPlayEvent evSysex(0, 0, MusECore::ME_SYSEX, data, length);
      MusECore::MidiPlayEvent evSysex(0, 0, MusECore::ME_SYSEX, d, l);
      _gui->writeEvent(evSysex);
    }
    break;
  case SYSEX_DELAYRETURN:
    ///setDelayReturn((int)data[1]);
    setDelayReturn((int)d[1]);
    if(!fromGui) {
      ///MusECore::MidiPlayEvent evSysex(0, 0, MusECore::ME_SYSEX, data, length);
      MusECore::MidiPlayEvent evSysex(0, 0, MusECore::ME_SYSEX, d, l);
      _gui->writeEvent(evSysex);
    }
    break;
  case SYSEX_SELECTREVERB:
    MusECore::Plugin* pluginReverb;
    ///memcpy(&pluginReverb, &data[1], sizeof(MusECore::Plugin*));
    memcpy(&pluginReverb, &d[1], sizeof(MusECore::Plugin*));
    initPluginReverb(pluginReverb);
    break;
  case SYSEX_SELECTCHORUS:
    MusECore::Plugin* pluginChorus;
    ///memcpy(&pluginChorus, &data[1], sizeof(MusECore::Plugin*));
    memcpy(&pluginChorus, &d[1], sizeof(MusECore::Plugin*));
    initPluginChorus(pluginChorus);
    break;
  case SYSEX_DELAYBPM:
    ///memcpy(&f, &data[1], sizeof(float));
    memcpy(&f, &d[1], sizeof(float));
    setDelayBPM(f);
    if(!fromGui) {
      ///MusECore::MidiPlayEvent evSysex(0, 0, MusECore::ME_SYSEX, data, length);
      MusECore::MidiPlayEvent evSysex(0, 0, MusECore::ME_SYSEX, d, l);
      _gui->writeEvent(evSysex);
    }
    break;    
  case SYSEX_DELAYBEATRATIO:
    ///memcpy(&f, &data[1], sizeof(float));
    memcpy(&f, &d[1], sizeof(float));
    setDelayBeatRatio(f);
    if(!fromGui) {
      ///MusECore::MidiPlayEvent evSysex(0, 0, MusECore::ME_SYSEX, data, length);
      MusECore::MidiPlayEvent evSysex(0, 0, MusECore::ME_SYSEX, d, l);
      _gui->writeEvent(evSysex);
    }
    break;    
  case SYSEX_DELAYFEEDBACK:
    ///memcpy(&f, &data[1], sizeof(float));
    memcpy(&f, &d[1], sizeof(float));
    setDelayFeedback(f);
    if(!fromGui) {
      ///MusECore::MidiPlayEvent evSysex(0, 0, MusECore::ME_SYSEX, data, length);
      MusECore::MidiPlayEvent evSysex(0, 0, MusECore::ME_SYSEX, d, l);
      _gui->writeEvent(evSysex);
    }
    break;    
  case SYSEX_DELAYLFOFREQ:
    ///memcpy(&f, &data[1], sizeof(float));
    memcpy(&f, &d[1], sizeof(float));
    setDelayLFOFreq(f);
    if(!fromGui) {
      ///MusECore::MidiPlayEvent evSysex(0, 0, MusECore::ME_SYSEX, data, length);
      MusECore::MidiPlayEvent evSysex(0, 0, MusECore::ME_SYSEX, d, l);
      _gui->writeEvent(evSysex);
    }
    break;    
  case SYSEX_DELAYLFODEPTH:
    ///memcpy(&f, &data[1], sizeof(float));
    memcpy(&f, &d[1], sizeof(float));
    setDelayLFODepth(f);
    if(!fromGui) {
      ///MusECore::MidiPlayEvent evSysex(0, 0, MusECore::ME_SYSEX, data, length);
      MusECore::MidiPlayEvent evSysex(0, 0, MusECore::ME_SYSEX, d, l);
      _gui->writeEvent(evSysex);
    }
    break;    
  default:
    break;
  }
  return false;
}
//---------------------------------------------------------
//   setController
//---------------------------------------------------------
bool DeicsOnze::setController(int channel, int id, int val) {
    setController(channel, id, val, false);
    return false;
}
bool DeicsOnze::setController(int ch, int ctrl, int val, bool fromGui) {
  int deiPan, k=0;
  if(_global.channel[ch].isEnable || ctrl==CTRL_CHANNELENABLE) {
    if(ctrl>=CTRL_AR && ctrl<CTRL_ALG) {
      k=(ctrl-CTRLOFFSET)/DECAPAR1;
      ctrl=ctrl-DECAPAR1*k;
    }
    else if(ctrl>CTRL_PL3 && ctrl<CTRL_REVERBRATE) {
      k=(ctrl-CTRLOFFSET-100)/DECAPAR2;
      ctrl=ctrl-DECAPAR2*k;
    }
    switch(ctrl) {
    case CTRL_AR:
      _preset[ch]->setIsUsed(true);
      _preset[ch]->eg[k].ar=val;
      if(!fromGui) {
	MusECore::MidiPlayEvent ev(0, 0, ch,MusECore::ME_CONTROLLER,CTRL_AR+k*DECAPAR1,val);
	_gui->writeEvent(ev);
      }
      break;
    case CTRL_D1R:
      _preset[ch]->setIsUsed(true);
      _preset[ch]->eg[k].d1r=val;
      if(!fromGui) {
	MusECore::MidiPlayEvent ev(0, 0, ch,MusECore::ME_CONTROLLER,CTRL_D1R+k*DECAPAR1,val);
	_gui->writeEvent(ev);
      }
      break;
    case CTRL_D2R:
      _preset[ch]->setIsUsed(true);
      _preset[ch]->eg[k].d2r=val;
      if(!fromGui) {
	MusECore::MidiPlayEvent ev(0, 0, ch,MusECore::ME_CONTROLLER,CTRL_D2R+k*DECAPAR1,val);
	_gui->writeEvent(ev);
      }
      break;
    case CTRL_RR:
      _preset[ch]->setIsUsed(true);
      _preset[ch]->eg[k].rr=val;
      if(!fromGui) {
	MusECore::MidiPlayEvent ev(0, 0, ch,MusECore::ME_CONTROLLER,CTRL_RR+k*DECAPAR1,val);
	_gui->writeEvent(ev);
      }
      break;
    case CTRL_D1L:
      _preset[ch]->setIsUsed(true);
      _preset[ch]->eg[k].d1l=val;
      if(!fromGui) {
	MusECore::MidiPlayEvent ev(0, 0, ch,MusECore::ME_CONTROLLER,CTRL_D1L+k*DECAPAR1,val);
	_gui->writeEvent(ev);
      }
      break;
    case CTRL_LS:
      _preset[ch]->setIsUsed(true);
      _preset[ch]->scaling.level[k]=val;
      if(!fromGui) {
	MusECore::MidiPlayEvent ev(0, 0, ch,MusECore::ME_CONTROLLER,CTRL_LS+k*DECAPAR1,val);
	_gui->writeEvent(ev);
      }
      break;
    case CTRL_RS:
      _preset[ch]->setIsUsed(true);
      _preset[ch]->scaling.rate[k]=val;
      if(!fromGui) {
	MusECore::MidiPlayEvent ev(0, 0, ch,MusECore::ME_CONTROLLER,CTRL_RS+k*DECAPAR1,val);
	_gui->writeEvent(ev);
      }
      break;
    case CTRL_EBS:
      _preset[ch]->setIsUsed(true);
      _preset[ch]->sensitivity.egBias[k]=val;
      if(!fromGui) {
	MusECore::MidiPlayEvent ev(0, 0, ch,MusECore::ME_CONTROLLER,CTRL_EBS+k*DECAPAR1,val);
	_gui->writeEvent(ev);
      }
      break;
    case CTRL_AME:
      _preset[ch]->setIsUsed(true);
      _preset[ch]->sensitivity.ampOn[k]=val==1;
      if(!fromGui) {
	MusECore::MidiPlayEvent ev(0, 0, ch,MusECore::ME_CONTROLLER,CTRL_AME+k*DECAPAR1,val);
	_gui->writeEvent(ev);
      }
      break;
    case CTRL_KVS:
      _preset[ch]->setIsUsed(true);
      _preset[ch]->sensitivity.keyVelocity[k]=val;
      if(!fromGui) {
	MusECore::MidiPlayEvent ev(0, 0, ch,MusECore::ME_CONTROLLER,CTRL_KVS+k*DECAPAR1,val);
	_gui->writeEvent(ev);
      }
      break;
    case CTRL_OUT:
      _preset[ch]->setIsUsed(true);
      _preset[ch]->outLevel[k]=val;
      setOutLevel(k);
      if(!fromGui) {
	MusECore::MidiPlayEvent ev(0, 0, ch,MusECore::ME_CONTROLLER,CTRL_OUT+k*DECAPAR1,val);
	_gui->writeEvent(ev);
      }
      break;
    case CTRL_RATIO:
      _preset[ch]->setIsUsed(true);
      _preset[ch]->frequency[k].ratio=((double)val)/100.0;
      if(!fromGui) {
	MusECore::MidiPlayEvent ev(0, 0, ch,MusECore::ME_CONTROLLER,
		     CTRL_RATIO+k*DECAPAR1,val);
	_gui->writeEvent(ev);
      }
      break;
    case CTRL_DET:
      _preset[ch]->setIsUsed(true);
      _preset[ch]->detune[k]=val;
      if(!fromGui) {
	MusECore::MidiPlayEvent ev(0, 0, ch,MusECore::ME_CONTROLLER,CTRL_DET+k*DECAPAR1,val);
	_gui->writeEvent(ev);
      }
      break;
    case CTRL_ALG:
      _preset[ch]->setIsUsed(true);
      _preset[ch]->algorithm=(Algorithm)val;
      if(!fromGui) {
	MusECore::MidiPlayEvent ev(0, 0, ch,MusECore::ME_CONTROLLER,CTRL_ALG,val);
	_gui->writeEvent(ev);
      }
      break;
    case CTRL_FEEDBACK:
      _preset[ch]->setIsUsed(true);
      _preset[ch]->feedback=val;
      setFeedback(ch);
      if(!fromGui) {
	MusECore::MidiPlayEvent ev(0, 0, ch,MusECore::ME_CONTROLLER,CTRL_FEEDBACK,val);
	_gui->writeEvent(ev);
      }
      break;
    case CTRL_SPEED:
      _preset[ch]->setIsUsed(true);
      _preset[ch]->lfo.speed=val;
      setLfo(ch);
      if(!fromGui) {
	MusECore::MidiPlayEvent ev(0, 0, ch,MusECore::ME_CONTROLLER,CTRL_SPEED,val);
	_gui->writeEvent(ev);
      }
      break;
    case CTRL_DELAY:
      _preset[ch]->setIsUsed(true);
      _preset[ch]->lfo.delay=val;
      setLfo(ch);
      if(!fromGui) {
	MusECore::MidiPlayEvent ev(0, 0, ch,MusECore::ME_CONTROLLER,CTRL_DELAY,val);
	_gui->writeEvent(ev);
      }
      break;
    case CTRL_PMODDEPTH:
      _preset[ch]->setIsUsed(true);
      _preset[ch]->lfo.pModDepth=val;
      setLfo(ch);
      if(!fromGui) {
	MusECore::MidiPlayEvent ev(0, 0, ch,MusECore::ME_CONTROLLER,CTRL_PMODDEPTH,val);
	_gui->writeEvent(ev);
      }
      break;
    case CTRL_AMODDEPTH:
      _preset[ch]->setIsUsed(true);
      _preset[ch]->lfo.aModDepth=val;
      setLfo(ch);
      if(!fromGui) {
	MusECore::MidiPlayEvent ev(0, 0, ch,MusECore::ME_CONTROLLER,CTRL_AMODDEPTH,val);
	_gui->writeEvent(ev);
      }
      break;
    case CTRL_SYNC:
      _preset[ch]->setIsUsed(true);
      _preset[ch]->lfo.sync=val==1;
      setLfo(ch);
      if(!fromGui) {
	MusECore::MidiPlayEvent ev(0, 0, ch,MusECore::ME_CONTROLLER,CTRL_SYNC,val);
	_gui->writeEvent(ev);
      }
      break;
    case CTRL_WAVE:
      _preset[ch]->setIsUsed(true);
      _preset[ch]->lfo.wave=(Wave)val;
      setLfo(ch);
      if(!fromGui) {
	MusECore::MidiPlayEvent ev(0, 0, ch,MusECore::ME_CONTROLLER,CTRL_WAVE,val);
	_gui->writeEvent(ev);
      }
      break;
    case CTRL_PMODSENS:
      _preset[ch]->setIsUsed(true);
      _preset[ch]->sensitivity.pitch=val;
      setLfo(ch);
      if(!fromGui) {
	MusECore::MidiPlayEvent ev(0, 0, ch,MusECore::ME_CONTROLLER,CTRL_PMODSENS,val);
	_gui->writeEvent(ev);
      }
      break;
    case CTRL_AMS:
      _preset[ch]->setIsUsed(true);
      _preset[ch]->sensitivity.amplitude=val;
      setLfo(ch);
      if(!fromGui) {
	MusECore::MidiPlayEvent ev(0, 0, ch,MusECore::ME_CONTROLLER,CTRL_AMS,val);
	_gui->writeEvent(ev);
      }
      break;
    case CTRL_TRANSPOSE:
      _preset[ch]->setIsUsed(true);
      _preset[ch]->function.transpose=val;
      if(!fromGui) {
	MusECore::MidiPlayEvent ev(0, 0, ch,MusECore::ME_CONTROLLER,CTRL_TRANSPOSE,val);
	_gui->writeEvent(ev);
      }
      break;
    case CTRL_POLYMODE:
      _preset[ch]->setIsUsed(true);
      _preset[ch]->function.mode=(Mode)val;
      if(!fromGui) {
	MusECore::MidiPlayEvent ev(0, 0, ch,MusECore::ME_CONTROLLER,CTRL_POLYMODE,val);
	_gui->writeEvent(ev);
      }
      break;
    case CTRL_PBENDRANGE:
      _preset[ch]->setIsUsed(true);
      _preset[ch]->function.pBendRange=val;
      if(!fromGui) {
	MusECore::MidiPlayEvent ev(0, 0, ch,MusECore::ME_CONTROLLER,CTRL_PBENDRANGE,val);
	_gui->writeEvent(ev);
      }
      break;
    case CTRL_PORTAMODE:
      _preset[ch]->setIsUsed(true);
      _preset[ch]->function.portamento=(Portamento)val;
      if(!fromGui) {
	MusECore::MidiPlayEvent ev(0, 0, ch,MusECore::ME_CONTROLLER,CTRL_PORTAMODE,val);
	_gui->writeEvent(ev);
      }
      break;
    case CTRL_PORTATIME:
      _preset[ch]->setIsUsed(true);
      _preset[ch]->function.portamentoTime=val;
      if(!fromGui) {
	MusECore::MidiPlayEvent ev(0, 0, ch,MusECore::ME_CONTROLLER,CTRL_PORTATIME,val);
	_gui->writeEvent(ev);
      }
      break;
    case CTRL_FCVOLUME:
      _preset[ch]->setIsUsed(true);
      _preset[ch]->function.fcVolume=val;
      if(!fromGui) {
	MusECore::MidiPlayEvent ev(0, 0, ch,MusECore::ME_CONTROLLER,CTRL_FCVOLUME,val);
	_gui->writeEvent(ev);
      }
      break;
    case CTRL_FSW:
      _preset[ch]->setIsUsed(true);
      _preset[ch]->function.footSw=(FootSw)val;
      if(!fromGui) {
	MusECore::MidiPlayEvent ev(0, 0, ch,MusECore::ME_CONTROLLER,CTRL_FSW,val);
	_gui->writeEvent(ev);
      }
      break;
    case CTRL_MWPITCH:
      _preset[ch]->setIsUsed(true);
      _preset[ch]->function.mwPitch=val;
      if(!fromGui) {
	MusECore::MidiPlayEvent ev(0, 0, ch,MusECore::ME_CONTROLLER,CTRL_MWPITCH,val);
	_gui->writeEvent(ev);
      }
      break;
    case CTRL_MWAMPLITUDE:
      _preset[ch]->setIsUsed(true);
      _preset[ch]->function.mwAmplitude=val;
      if(!fromGui) {
	MusECore::MidiPlayEvent ev(0, 0, ch,MusECore::ME_CONTROLLER,CTRL_MWAMPLITUDE,val);
	_gui->writeEvent(ev);
      }
      break;
    case CTRL_BCPITCH:
      _preset[ch]->setIsUsed(true);
      _preset[ch]->function.bcPitch=val;
      if(!fromGui) {
	MusECore::MidiPlayEvent ev(0, 0, ch,MusECore::ME_CONTROLLER,CTRL_BCPITCH,val);
	_gui->writeEvent(ev);
      }
      break;
    case CTRL_BCAMPLITUDE:
      _preset[ch]->setIsUsed(true);
      _preset[ch]->function.bcAmplitude=val;
      if(!fromGui) {
	MusECore::MidiPlayEvent ev(0, 0, ch,MusECore::ME_CONTROLLER,CTRL_BCAMPLITUDE,val);
	_gui->writeEvent(ev);
      }
      break;
    case CTRL_BCPITCHBIAS:
      _preset[ch]->setIsUsed(true);
      _preset[ch]->function.bcPitchBias=val;
      if(!fromGui) {
	MusECore::MidiPlayEvent ev(0, 0, ch,MusECore::ME_CONTROLLER,CTRL_BCPITCHBIAS,val);
	_gui->writeEvent(ev);
      }
      break;
    case CTRL_BCEGBIAS:
      _preset[ch]->setIsUsed(true);
      _preset[ch]->function.bcEgBias=val;
      if(!fromGui) {
	MusECore::MidiPlayEvent ev(0, 0, ch,MusECore::ME_CONTROLLER,CTRL_BCEGBIAS,val);
	_gui->writeEvent(ev);
      }
      break;
    case CTRL_ATPITCH:
      _preset[ch]->setIsUsed(true);
      _preset[ch]->function.atPitch=val;
      if(!fromGui) {
	MusECore::MidiPlayEvent ev(0, 0, ch,MusECore::ME_CONTROLLER,CTRL_ATPITCH,val);
	_gui->writeEvent(ev);
      }
      break;
    case CTRL_ATAMPLITUDE:
      _preset[ch]->setIsUsed(true);
      _preset[ch]->function.atAmplitude=val;
      if(!fromGui) {
	MusECore::MidiPlayEvent ev(0, 0, ch,MusECore::ME_CONTROLLER,CTRL_ATAMPLITUDE,val);
	_gui->writeEvent(ev);
      }
      break;
    case CTRL_ATPITCHBIAS:
      _preset[ch]->setIsUsed(true);
      _preset[ch]->function.atPitchBias=val;
      if(!fromGui) {
	MusECore::MidiPlayEvent ev(0, 0, ch,MusECore::ME_CONTROLLER,CTRL_ATPITCHBIAS,val);
	_gui->writeEvent(ev);
      }
      break;
    case CTRL_ATEGBIAS:
      _preset[ch]->setIsUsed(true);
      _preset[ch]->function.atEgBias=val;
      if(!fromGui) {
	MusECore::MidiPlayEvent ev(0, 0, ch,MusECore::ME_CONTROLLER,CTRL_ATEGBIAS,val);
	_gui->writeEvent(ev);
      }
      break;
    case CTRL_PR1:
      _preset[ch]->setIsUsed(true);
      _preset[ch]->pitchEg.pr1=val;
      if(!fromGui) {
	MusECore::MidiPlayEvent ev(0, 0, ch,MusECore::ME_CONTROLLER,CTRL_PR1,val);
	_gui->writeEvent(ev);
      }
      break;
    case CTRL_PR2:
      _preset[ch]->setIsUsed(true);
      _preset[ch]->pitchEg.pr2=val;
      if(!fromGui) {
	MusECore::MidiPlayEvent ev(0, 0, ch,MusECore::ME_CONTROLLER,CTRL_PR2,val);
	_gui->writeEvent(ev);
      }
      break;
    case CTRL_PR3:
      _preset[ch]->setIsUsed(true);
      _preset[ch]->pitchEg.pr3=val;
      if(!fromGui) {
	MusECore::MidiPlayEvent ev(0, 0, ch,MusECore::ME_CONTROLLER,CTRL_PR3,val);
	_gui->writeEvent(ev);
      }
      break;
    case CTRL_PL1:
      _preset[ch]->setIsUsed(true);
      _preset[ch]->pitchEg.pl1=val;
      if(!fromGui) {
	MusECore::MidiPlayEvent ev(0, 0, ch,MusECore::ME_CONTROLLER,CTRL_PL1,val);
	_gui->writeEvent(ev);
      }
      break;
    case CTRL_PL2:
      _preset[ch]->setIsUsed(true);
      _preset[ch]->pitchEg.pl2=val;
      if(!fromGui) {
	MusECore::MidiPlayEvent ev(0, 0, ch,MusECore::ME_CONTROLLER,CTRL_PL2,val);
	_gui->writeEvent(ev);
      }
      break;
    case CTRL_PL3:
      _preset[ch]->setIsUsed(true);
      _preset[ch]->pitchEg.pl3=val;
      if(!fromGui) {
	MusECore::MidiPlayEvent ev(0, 0, ch,MusECore::ME_CONTROLLER,CTRL_PL3,val);
	_gui->writeEvent(ev);
      }
      break;
    case CTRL_FIX:
      _preset[ch]->setIsUsed(true);
      _preset[ch]->frequency[k].isFix=val==1;
      if(!fromGui) {
	MusECore::MidiPlayEvent ev(0, 0, ch,MusECore::ME_CONTROLLER,CTRL_FIX+k*DECAPAR2,val);
	_gui->writeEvent(ev);
      }	
      break;
    case CTRL_FIXRANGE:
      _preset[ch]->setIsUsed(true);
      _preset[ch]->frequency[k].freq=((double)val)/100.0;
      if(!fromGui) {
	MusECore::MidiPlayEvent ev(0, 0, ch,MusECore::ME_CONTROLLER,
		     CTRL_FIXRANGE+k*DECAPAR2,val);
	_gui->writeEvent(ev);
      }	
      break;
    case CTRL_OSW:
      _preset[ch]->setIsUsed(true);
      _preset[ch]->oscWave[k]=(OscWave)val;
      if(!fromGui) {
	MusECore::MidiPlayEvent ev(0, 0, ch,MusECore::ME_CONTROLLER,CTRL_OSW+k*DECAPAR2,val);
	_gui->writeEvent(ev);
      }	
      break;
    case CTRL_SHFT:
      _preset[ch]->setIsUsed(true);
      _preset[ch]->eg[k].egShift=(egShiftValue)val;
      if(!fromGui) {
	MusECore::MidiPlayEvent ev(0, 0, ch,MusECore::ME_CONTROLLER,CTRL_SHFT+k*DECAPAR2,val);
	_gui->writeEvent(ev);
      }	
      break;
    case CTRL_REVERBRATE:
      _preset[ch]->setIsUsed(true);
      _preset[ch]->function.reverbRate=val;
      if(!fromGui) {
	MusECore::MidiPlayEvent ev(0, 0, ch,MusECore::ME_CONTROLLER,CTRL_REVERBRATE,val);
	_gui->writeEvent(ev);
      }
      break;
    case CTRL_FCPITCH:
      _preset[ch]->setIsUsed(true);
      _preset[ch]->function.fcPitch=val;
      if(!fromGui) {
	MusECore::MidiPlayEvent ev(0, 0, ch,MusECore::ME_CONTROLLER,CTRL_FCPITCH,val);
	_gui->writeEvent(ev);
      }
      break;
    case CTRL_FCAMPLITUDE:
      _preset[ch]->setIsUsed(true);
      _preset[ch]->function.fcAmplitude=val;
      if(!fromGui) {
	MusECore::MidiPlayEvent ev(0, 0, ch,MusECore::ME_CONTROLLER,CTRL_FCAMPLITUDE,val);
	_gui->writeEvent(ev);
      }
    break;
    case CTRL_CHANNELENABLE:
      setChannelEnable(ch, (bool)val);
      if(!fromGui) {
	MusECore::MidiPlayEvent ev(0, 0, ch,MusECore::ME_CONTROLLER,CTRL_CHANNELENABLE,val);
	_gui->writeEvent(ev);
      }
      break;
    case CTRL_CHANNELDETUNE:
      _preset[ch]->setIsUsed(true);
      setChannelDetune(ch, val);
      if(!fromGui) {
	MusECore::MidiPlayEvent ev(0, 0, ch,MusECore::ME_CONTROLLER,CTRL_CHANNELDETUNE,val);
	_gui->writeEvent(ev);
      }
      break;
    case CTRL_CHANNELVOLUME:
      setChannelVol(ch, val);
      applyChannelAmp(ch);
      if(!fromGui) {
	MusECore::MidiPlayEvent ev(0, 0, ch,MusECore::ME_CONTROLLER,CTRL_CHANNELVOLUME,val);
	_gui->writeEvent(ev);
      }
      break;
    case CTRL_NBRVOICES:
      setNbrVoices(ch, val);
      if(!fromGui) {
	MusECore::MidiPlayEvent ev(0, 0, ch, MusECore::ME_CONTROLLER, CTRL_NBRVOICES, val);
	_gui->writeEvent(ev);
      }
    break;
    case MusECore::CTRL_PROGRAM: {
      int hbank = (val & 0xff0000) >> 16;
      int lbank = (val & 0xff00) >> 8;
      int prog  = val & 0x7f;
      if (hbank > 127)  // map "dont care" to 0
	hbank = 0;
      if (lbank > 127)
	lbank = 0;
      programSelect(ch, hbank, lbank, prog);
      _preset[ch]->setIsUsed(true);//TODO : not sure to put that
      if(!fromGui) {
	MusECore::MidiPlayEvent ev(0, 0, ch, MusECore::ME_CONTROLLER, MusECore::CTRL_PROGRAM, val);
	_gui->writeEvent(ev);
      }
    } break;
    case MusECore::CTRL_MODULATION:
      setModulation(ch, val);
      if(!fromGui) {
	MusECore::MidiPlayEvent ev(0, 0, ch, MusECore::ME_CONTROLLER, MusECore::CTRL_MODULATION, val);
	_gui->writeEvent(ev);
      }
      break;
    case MusECore::CTRL_PITCH:
      setPitchBendCoef(ch, val);
      break;
    case MusECore::CTRL_PANPOT:
      _preset[ch]->setIsUsed(true);
      deiPan = val*2*MAXCHANNELPAN/127-MAXCHANNELPAN;
      setChannelPan(ch, deiPan);
      applyChannelAmp(ch);
      if(!fromGui) {
	MusECore::MidiPlayEvent ev(0, 0, ch, MusECore::ME_CONTROLLER, CTRL_CHANNELPAN, deiPan);
	_gui->writeEvent(ev);
      }
      break;      
    case CTRL_CHANNELPAN:
      _preset[ch]->setIsUsed(true);
      setChannelPan(ch, val);
      applyChannelAmp(ch);
      if(!fromGui) {
	MusECore::MidiPlayEvent ev(0, 0, ch, MusECore::ME_CONTROLLER, CTRL_CHANNELPAN, val);
	_gui->writeEvent(ev);
      }
      break;      
    case CTRL_FINEBRIGHTNESS:
      _preset[ch]->setIsUsed(true);
      setChannelBrightness(ch, val);
      setOutLevel(ch);
      if(!fromGui) {
	MusECore::MidiPlayEvent ev(0, 0, ch, MusECore::ME_CONTROLLER, CTRL_FINEBRIGHTNESS, val);
	_gui->writeEvent(ev);
      }
      break;
    case MusECore::CTRL_BRIGHTNESS:
      _preset[ch]->setIsUsed(true);
      setChannelBrightness(ch, val*(MIDFINEBRIGHTNESS/MIDBRIGHTNESS));
      setOutLevel(ch);
      if(!fromGui) {
	MusECore::MidiPlayEvent
	  ev(0, 0, ch,MusECore::ME_CONTROLLER,CTRL_FINEBRIGHTNESS,getChannelBrightness(ch));
	_gui->writeEvent(ev);
      }
      break;
    case MusECore::CTRL_ATTACK_TIME:
      _preset[ch]->setIsUsed(true);
      setChannelAttack(ch, val);
      setEnvAttack(ch);
      if(!fromGui) {
	MusECore::MidiPlayEvent ev(0, 0, ch, MusECore::ME_CONTROLLER, MusECore::CTRL_ATTACK_TIME, val);
	_gui->writeEvent(ev);
      }
      break;
    case MusECore::CTRL_RELEASE_TIME:
      _preset[ch]->setIsUsed(true);
      setChannelRelease(ch, val);
      setEnvRelease(ch);
      if(!fromGui) {
	MusECore::MidiPlayEvent ev(0, 0, ch, MusECore::ME_CONTROLLER, MusECore::CTRL_RELEASE_TIME, val);
	_gui->writeEvent(ev);
      }
      break;
    case MusECore::CTRL_REVERB_SEND:
      setChannelReverb(ch, val);
      if(!fromGui) {
	MusECore::MidiPlayEvent ev(0, 0, ch, MusECore::ME_CONTROLLER, MusECore::CTRL_REVERB_SEND, val);
	_gui->writeEvent(ev);
      }
      break;
    case MusECore::CTRL_CHORUS_SEND:
      setChannelChorus(ch, val);
      if(!fromGui) {
	MusECore::MidiPlayEvent ev(0, 0, ch, MusECore::ME_CONTROLLER, MusECore::CTRL_CHORUS_SEND, val);
	_gui->writeEvent(ev);
      }
      break;
    case MusECore::CTRL_VARIATION_SEND:
      setChannelDelay(ch, val);
      if(!fromGui) {
	MusECore::MidiPlayEvent ev(0, 0, ch, MusECore::ME_CONTROLLER, MusECore::CTRL_VARIATION_SEND, val);
	_gui->writeEvent(ev);
      }
      break;
    case MusECore::CTRL_SUSTAIN:
      setSustain(ch, val);
      break;
    case MusECore::CTRL_VOLUME:
      setChannelVol(ch, val*(MAXCHANNELVOLUME/127));
      applyChannelAmp(ch);
      if(!fromGui) {
	MusECore::MidiPlayEvent
	  ev(0, 0, ch, MusECore::ME_CONTROLLER, CTRL_CHANNELVOLUME, getChannelVol(ch));
	_gui->writeEvent(ev);
      }
      break;      
    case MusECore::CTRL_ALL_SOUNDS_OFF:
      resetVoices();      
    default:
      break;
    }
  }
  return false;
}

//---------------------------------------------------------
//   getPatchName
//---------------------------------------------------------

const char* DeicsOnze::getPatchName(int ch, int val, int) const {
  if(_global.channel[ch].isEnable) {
    Preset* p_preset;
    int hbank = (val & 0xff0000) >> 16;
    int lbank = (val & 0xff00) >> 8;
    if (hbank > 127)
      hbank = 0;
    if (lbank > 127)
      lbank = 0;
    if (lbank == 127)       // drum HACK
      lbank = 128;
    int prog =   val & 0x7f;
    const char* tempName="INITVOICE";
    p_preset=_set->findPreset(hbank, lbank, prog);
    if (p_preset) tempName=const_cast<char *>(p_preset->name.c_str());
    return tempName;
  }
  return " ";
}

//---------------------------------------------------------
//   getPatchInfo
//---------------------------------------------------------
const MidiPatch* DeicsOnze::getPatchInfo(int /*ch*/, const MidiPatch* p) const {
  Preset* preset = NULL;
  Subcategory* sub = NULL;
  Category* cat = NULL;
  if(p) {
    _patch.hbank = p->hbank;
    _patch.lbank = p->lbank;
    _patch.prog = p->prog;
    switch(p->typ) {
    case MP_TYPE_HBANK :
      sub = findSubcategory(_patch.hbank, _patch.lbank);
      if(sub) {
	_patch.name = sub->_subcategoryName.c_str();
	_patch.typ = MP_TYPE_LBANK;
	return &_patch;
      }
      else {
	if(_patch.lbank + 1 < LBANK_NBR) {
	  _patch.lbank++;
	  return getPatchInfo(0, &_patch);
	}
	else {
	  _patch.prog = PROG_NBR - 1; //hack to go faster
	  _patch.typ = 0;
	  return getPatchInfo(0, &_patch);
	}
      }
      break;
    case MP_TYPE_LBANK :
      preset = findPreset(_patch.hbank, _patch.lbank, _patch.prog);
      _patch.typ = 0;
      if(preset) {
	_patch.name = preset->name.c_str();
	return &_patch;
      }
      else return getPatchInfo(0, &_patch);
      break;
    default :
      if(_patch.prog + 1 < PROG_NBR) {
	_patch.prog++;
	preset = findPreset(_patch.hbank, _patch.lbank, _patch.prog);
	if(preset) {
	  _patch.name = preset->name.c_str();
	  return &_patch;
	}
	else return getPatchInfo(0, &_patch);
      }
      else {
	_patch.prog = 0;
	if(_patch.lbank + 1 < LBANK_NBR) {
	  _patch.lbank++;
	  _patch.typ = MP_TYPE_HBANK;
	   return getPatchInfo(0, &_patch);
	}
	else {
	  _patch.lbank = 0;
	  if(_patch.hbank + 1 < HBANK_NBR) {
	    _patch.hbank++;
	    _patch.typ = MP_TYPE_HBANK;
	    cat = findCategory(_patch.hbank);
	    if(cat) {
	      _patch.name = cat->_categoryName.c_str();
	      return &_patch;
	    }
	    return getPatchInfo(0, &_patch);
	  }
	  else return NULL;
	}	  
      }
    }
  }
  else {
    _patch.typ = MP_TYPE_HBANK;
    _patch.hbank = 0;
    _patch.lbank = 0;
    _patch.prog = 0;
    cat = findCategory(_patch.hbank);
    if(cat) {
      _patch.name = cat->_categoryName.c_str();
      return &_patch;
    }
    else {
      _patch.hbank++;
      return getPatchInfo(0, &_patch);
    }
  } 
}

//---------------------------------------------------------
//   getControllerInfo
/*!
  \fn SimpleSynth::getControllerInfo
  \brief Called from host to collect info about which controllers
  the synth supports
  \param index current controller number
  \param name pointer where name is stored
  \param controller int pointer where muse controller number is stored
  \param min int pointer where controller min value is stored
  \param max int pointer where controller max value is stored
  \return 0 when done, otherwise return next desired controller index
*/
//---------------------------------------------------------
int DeicsOnze::getControllerInfo(int index, const char** name,
				 int* controller, int* min, int* max, int* initval) const
{
    if (index >= nbrCtrl) {
	return 0;
    }

    *name = _ctrl[index].name.c_str();
    *controller = _ctrl[index].num;
    *min = _ctrl[index].min;
    *max = _ctrl[index].max;
    *initval = 0;                // p4.0.27 FIXME NOTE TODO    
    return (index +1);
}

//---------------------------------------------------------
//   playNote
//    process note on
//---------------------------------------------------------
bool DeicsOnze::playNote(int ch, int pitch, int velo) {
  int newVoice;
  int nO2V;
  int p2V;
  double tempTargetFreq;
  if(_global.channel[ch].isEnable) {    
    if(velo==0) {//Note off
      p2V=pitchOn2Voice(ch, pitch);
      //printf("Note Off : pitchOn2Voice = %d\n", p2V);
      if(p2V<_global.channel[ch].nbrVoices) {
	if(_global.channel[ch].sustain)
	  _global.channel[ch].voices[p2V].isSustained = true;
	else {
	  _global.channel[ch].voices[p2V].keyOn = false;
	  _global.channel[ch].lastVoiceKeyOff = p2V;
	  _global.channel[ch].lastVoiceKeyOn.remove(p2V);
	  if(_preset[ch]->function.mode == MONO && existsKeyOn(ch)
	     && _global.channel[ch].voices[p2V].isOn) {
	    newVoice = _global.channel[ch].lastVoiceKeyOn.back();
	    //portamento
	    if(_preset[ch]->function.portamentoTime!=0) {
	      _global.channel[ch].voices[newVoice].hasAttractor = true;
	      _global.channel[ch].voices[newVoice].attractor =
		getAttractor(_preset[ch]->function.portamentoTime, 
			     _global.deiSampleRate);
	    }
	    else _global.channel[ch].voices[newVoice].hasAttractor = false;
	    //feedback
	    _global.channel[ch].voices[newVoice].sampleFeedback =
	      _global.channel[ch].voices[p2V].sampleFeedback;
	    //on/off
	    _global.channel[ch].voices[p2V].isOn = false;
	    _global.channel[ch].voices[newVoice].isOn = true;
	    //per op
	    for(int i = 0; i < NBROP; i++) {
	      _global.channel[ch].voices[newVoice].op[i].index =
		_global.channel[ch].voices[p2V].op[i].index;
	      _global.channel[ch].voices[newVoice].op[i].envState = 
		_global.channel[ch].voices[p2V].op[i].envState;
	      _global.channel[ch].voices[newVoice].op[i].envIndex = 
		_global.channel[ch].voices[p2V].op[i].envIndex;
	      _global.channel[ch].voices[newVoice].op[i].envInct = 
		_global.channel[ch].voices[p2V].op[i].envInct;
	      _global.channel[ch].voices[newVoice].op[i].envLevel = 
		_global.channel[ch].voices[p2V].op[i].envLevel;
	      _global.channel[ch].voices[newVoice].op[i].coefVLevel = 
		_global.channel[ch].voices[p2V].op[i].coefVLevel;
	      if(_global.channel[ch].voices[newVoice].hasAttractor)
		_global.channel[ch].voices[newVoice].op[i].inct =
		  _global.channel[ch].voices[p2V].op[i].inct;
	    }
	  }
	  else {
	    setPitchEnvRelease(ch, p2V);
	    for(int i=0; i<NBROP; i++) {
	      _global.channel[ch].voices[p2V].op[i].envState = RELEASE;
	      setEnvRelease(ch, p2V, i);
	    }
	  }
	}
	return false;
      }
      //else printf("error over NBRVOICES\n");
    }
    else //Note on
      {
	nO2V=noteOff2Voice(ch);
	newVoice=((nO2V==MAXNBRVOICES)?minVolu2Voice(ch):nO2V);
	//printf("Note On : ch = %d, v = %d, p = %d\n", ch, newVoice, pitch);
	
	//----------
	//portamento
	//----------
	//if there is no previous note there is no portamento
	if(_preset[ch]->function.portamentoTime!=0
	   && _global.channel[ch].isLastNote &&
	   ((_preset[ch]->function.portamento==FULL) ||
	   (_preset[ch]->function.portamento==FINGER && existsKeyOn(ch)))) {
	  _global.channel[ch].voices[newVoice].hasAttractor = true;
	  _global.channel[ch].voices[newVoice].attractor =
	    getAttractor(_preset[ch]->function.portamentoTime,
			 _global.deiSampleRate);
	}
	else _global.channel[ch].voices[newVoice].hasAttractor = false;
	
	if(_preset[ch]->lfo.sync) _global.channel[ch].lfoIndex=0;

	_global.channel[ch].lfoDelayIndex = 
	  (_preset[ch]->lfo.delay==0?(double)(RESOLUTION/4):0.0);
	_global.channel[ch].delayPassed = false;
	
	//--------------
	//PITCH ENVELOPE
	//--------------
	if(isPitchEnv(&_preset[ch]->pitchEg)) {
	  _global.channel[ch].voices[newVoice].pitchEnvState = PHASE1;
	  _global.channel[ch].voices[newVoice].pitchEnvCoefInctPhase1 =
	    getPitchEnvCoefInct(_preset[ch]->pitchEg.pl1);
	  _global.channel[ch].voices[newVoice].pitchEnvCoefInctPhase2 =
	    getPitchEnvCoefInct(_preset[ch]->pitchEg.pl2);
	  _global.channel[ch].voices[newVoice].pitchEnvCoefInctPhase3 =
	    getPitchEnvCoefInct(_preset[ch]->pitchEg.pl3);
	  _global.channel[ch].voices[newVoice].pitchEnvCoefInct =
	    _global.channel[ch].voices[newVoice].pitchEnvCoefInctPhase1;
	  _global.channel[ch].voices[newVoice].pitchEnvCoefInctInct =
	    getPitchEnvCoefInctInct(_preset[ch]->pitchEg.pl1,
				    _preset[ch]->pitchEg.pl2,
				    _preset[ch]->pitchEg.pr1,
				    _global.deiSampleRate);
	}
	else {
	  _global.channel[ch].voices[newVoice].pitchEnvState = OFF_PE;
	  _global.channel[ch].voices[newVoice].pitchEnvCoefInct = 1.0;
	}
	//per operator
	for(int i=0; i<NBROP; i++) {
	  //------
	  //VOLUME
	  //------
	  _global.channel[ch].voices[newVoice].op[i].ampVeloNote =
	    velo2AmpR(velo, _preset[ch]->sensitivity.keyVelocity[i])
	    *note2Amp((double) (pitch+_preset[ch]->function.transpose),
		      _preset[ch]->scaling.level[i]);
	  _global.channel[ch].voices[newVoice].op[i].amp =
	    outLevel2Amp(_preset[ch]->outLevel[i])
	    *_global.channel[ch].voices[newVoice].op[i].ampVeloNote
	    * brightness2Amp(ch, i);
	  //----------------
	  //INDEX & ENVELOPE
	  //----------------
	  //if index get 0.0, it means that the offset is 0
	  if(existsKeyOn(ch)) {
	    int lastVoice = _global.channel[ch].lastVoiceKeyOn.back();
	    if(_preset[ch]->function.mode == MONO) {
	      _global.channel[ch].voices[newVoice].op[i].index =
		_global.channel[ch].voices[lastVoice].op[i].index;
	      _global.channel[ch].voices[newVoice].sampleFeedback =
		_global.channel[ch].voices[lastVoice].sampleFeedback;
	      _global.channel[ch].voices[newVoice].op[i].envState = 
		_global.channel[ch].voices[lastVoice].op[i].envState;
	      _global.channel[ch].voices[newVoice].op[i].envIndex = 
		_global.channel[ch].voices[lastVoice].op[i].envIndex;
	      _global.channel[ch].voices[newVoice].op[i].envInct = 
		_global.channel[ch].voices[lastVoice].op[i].envInct;
	      _global.channel[ch].voices[newVoice].op[i].envLevel = 
		_global.channel[ch].voices[lastVoice].op[i].envLevel;
	      _global.channel[ch].voices[newVoice].op[i].coefVLevel = 
		_global.channel[ch].voices[lastVoice].op[i].coefVLevel;
	      _global.channel[ch].voices[lastVoice].isOn = false;
	    }
	    else {
	      _global.channel[ch].voices[newVoice].op[i].index = 0.0;
	      _global.channel[ch].voices[newVoice].sampleFeedback = 0.0;
	      _global.channel[ch].voices[newVoice].op[i].envState = ATTACK;
	      _global.channel[ch].voices[newVoice].op[i].envIndex = 0.0;
	      setEnvAttack(ch, newVoice, i);
	    }
	  }
	  else {
	    _global.channel[ch].voices[newVoice].op[i].index = 0.0;
	    _global.channel[ch].voices[newVoice].sampleFeedback = 0.0;
	    _global.channel[ch].voices[newVoice].op[i].envState = ATTACK;
	    _global.channel[ch].voices[newVoice].op[i].envIndex = 0.0;
	    setEnvAttack(ch, newVoice, i);
	    if(_preset[ch]->function.mode == MONO &&
	       _global.channel[ch].isLastNote) {
	      _global.channel[ch].voices[_global.channel[ch].lastVoiceKeyOff]
		.isOn = false;
	    }
	  }
	  
	  //----
	  //FREQ
	  //----
	  //the frequence for each operator is calculated
	  //and is used later to calculate inct
	  tempTargetFreq = 
	    (pitch2freq((double)getChannelDetune(ch)
			/(double)MAXCHANNELDETUNE)
	     /LOWERNOTEFREQ)*
	    (_preset[ch]->frequency[i].isFix?
	     _preset[ch]->frequency[i].freq:
	     (_preset[ch]->frequency[i].ratio
	      *pitch2freq((double)(pitch+_preset[ch]->function.transpose)
			  +(double)_preset[ch]->detune[i]*COEFDETUNE)));
	  //----
	  //INCT
	  //----
	  //compute inct
	  _global.channel[ch].voices[newVoice].op[i].targetInct =
	    (double)RESOLUTION / ( _global.deiSampleRate / tempTargetFreq );
	  if(_global.channel[ch].voices[newVoice].hasAttractor &&
	     !_preset[ch]->frequency[i].isFix)
	    _global.channel[ch].voices[newVoice].op[i].inct =
	      _global.channel[ch].lastInc[i];
	  else _global.channel[ch].voices[newVoice].op[i].inct =
	    _global.channel[ch].voices[newVoice].op[i].targetInct;
	}
	//--------------------
	//some initializations
	//--------------------
	_global.channel[ch].voices[newVoice].keyOn = true;
	_global.channel[ch].voices[newVoice].isSustained = false;
	_global.channel[ch].voices[newVoice].isOn = true;
	_global.channel[ch].voices[newVoice].pitch = pitch;
	_global.channel[ch].isLastNote = true;
	_global.channel[ch].lastVoiceKeyOn.push_back(newVoice);
	for(int k = 0; k < NBROP; k++)
	  _global.channel[ch].lastInc[k] =
	    _global.channel[ch].voices[newVoice].op[k].inct;
	return false;
      }
  }
  return false;
}

//---------------------------------------------------------
// plusMod
//  add two doubles modulo RESOLUTION
//---------------------------------------------------------
inline double plusMod(double x, double y) {
  double res;
  res=x+y;
  if (res>=0) while (res >= (double)RESOLUTION) res-=(double)RESOLUTION;
  else while (res < 0) res+=(double)RESOLUTION;
  return res;
}


//---------------------------------------------------------
//   processMessages
//   Called from host always, even if output path is unconnected.
//---------------------------------------------------------

void DeicsOnze::processMessages()
{
  //Process messages from the gui
  while (_gui->fifoSize()) {
    MusECore::MidiPlayEvent ev = _gui->readEvent();
    if (ev.type() == MusECore::ME_SYSEX) {
      sysex(ev.len(), ev.data(), true);
      sendEvent(ev);
    }
    else if (ev.type() == MusECore::ME_CONTROLLER) {
      setController(ev.channel(), ev.dataA(), ev.dataB(), true);
      sendEvent(ev);
    }
  }
}

//---------------------------------------------------------
//   write
//    synthesize n samples into buffer+offset
//---------------------------------------------------------
void DeicsOnze::process(float** buffer, int offset, int n) {
  /*
  //Process messages from the gui
  while (_gui->fifoSize()) {
    MusECore::MidiPlayEvent ev = _gui->readEvent();
    if (ev.type() == MusECore::ME_SYSEX) {
      sysex(ev.len(), ev.data(), true);
      sendEvent(ev);
    }
    else if (ev.type() == MusECore::ME_CONTROLLER) {
      setController(ev.channel(), ev.dataA(), ev.dataB(), true);
      sendEvent(ev);
    }
  }
  */
  
  float* leftOutput = buffer[0] + offset;
  float* rightOutput = buffer[1] + offset; 

  float sample[MAXNBRVOICES];
  float tempLeftOutput;
  float tempRightOutput;
  float tempChannelOutput;
  float tempChannelLeftOutput;
  float tempChannelRightOutput;
  float tempIncChannel; //for optimization
  float sampleOp[NBROP];
  for(int i = 0; i < NBROP; i++) sampleOp[i] = 0.0;
  float ampOp[NBROP];
  for(int i = 0; i < n; i++) {
    if(_global.qualityCounter == 0) {
      tempLeftOutput = 0.0;
      tempRightOutput = 0.0;
      _global.lastInputLeftChorusSample = 0.0;
      _global.lastInputRightChorusSample = 0.0;
      _global.lastInputLeftReverbSample = 0.0;
      _global.lastInputRightReverbSample = 0.0;
      _global.lastInputLeftDelaySample = 0.0;
      _global.lastInputRightDelaySample = 0.0;
      //per channel
      for(int c = 0; c < NBRCHANNELS; c++) {
	tempChannelOutput = 0.0;
	if(_global.channel[c].isEnable) {
	  //lfo, trick : we use the first quater of the wave W2
	  lfoUpdate(_preset[c], &_global.channel[c], waveTable[W2]);
	  
	  //optimization
	  tempIncChannel =
	    _global.channel[c].lfoCoefInct * _global.channel[c].pitchBendCoef;
	  
	  //per voice
	  for(int j=0; j<_global.channel[c].nbrVoices; j++) {
	    if (_global.channel[c].voices[j].isOn) {
	      //portamento
	      portamentoUpdate(&_global.channel[c],
			       &_global.channel[c].voices[j]);
	      //pitch envelope
	      pitchEnvelopeUpdate(&_global.channel[c].voices[j],
				  &_preset[c]->pitchEg, _global.deiSampleRate);
	      //per op
	      for(int k=0; k<NBROP; k++) {
		//compute the next index on the wavetable,
		//without taking account of the feedback and FM modulation
		_global.channel[c].voices[j].op[k].index=
		  plusMod(_global.channel[c].voices[j].op[k].index,
			  _global.channel[c].voices[j].op[k].inct
			  * tempIncChannel
			  * _global.channel[c].voices[j].pitchEnvCoefInct);
		
		ampOp[k]=_global.channel[c].voices[j].op[k].amp*COEFLEVEL
		  *(_preset[c]->sensitivity.ampOn[k]?
		    _global.channel[c].lfoAmp:1.0)
		  *env2AmpR(_global.deiSampleRate, waveTable[W2],
			    _preset[c]->eg[k],
			    &_global.channel[c].voices[j].op[k]);
	      }
	      switch(_preset[c]->algorithm) {
	      case FIRST :
		sampleOp[3]=ampOp[3]
		  *waveTable[_preset[c]->oscWave[3]]
		  [(int)plusMod(_global.channel[c].voices[j].op[3].index,
				(float)RESOLUTION
				*_global.channel[c].voices[j].sampleFeedback)];
		sampleOp[2]=ampOp[2]
		  *waveTable[_preset[c]->oscWave[2]]
		  [(int)plusMod(_global.channel[c].voices[j].op[2].index,
				(float)RESOLUTION*sampleOp[3])];
		sampleOp[1]=ampOp[1]
		  *waveTable[_preset[c]->oscWave[1]]
		  [(int)plusMod(_global.channel[c].voices[j].op[1].index,
				(float)RESOLUTION*sampleOp[2])];
		sampleOp[0]=ampOp[0]
		  *waveTable[_preset[c]->oscWave[0]]
		  [(int)plusMod(_global.channel[c].voices[j].op[0].index,
				(float)RESOLUTION*sampleOp[1])];
		
		sample[j]=sampleOp[0];///COEFLEVEL;
		
		_global.channel[c].voices[j].isOn =
		  (_global.channel[c].voices[j].op[0].envState!=OFF);
		break;
	      case SECOND :
		sampleOp[3]=ampOp[3]
		  *waveTable[_preset[c]->oscWave[3]]
		  [(int)plusMod(_global.channel[c].voices[j].op[3].index,
				(float)RESOLUTION
				*_global.channel[c].voices[j].sampleFeedback)];
		sampleOp[2]=ampOp[2]
		  *waveTable[_preset[c]->oscWave[2]]
		  [(int)_global.channel[c].voices[j].op[2].index];
		sampleOp[1]=ampOp[1]
		  *waveTable[_preset[c]->oscWave[1]]
		  [(int)plusMod(_global.channel[c].voices[j].op[1].index,
				(float)RESOLUTION
				*(sampleOp[2]+sampleOp[3])/2.0)];
		sampleOp[0]=ampOp[0]
		  *waveTable[_preset[c]->oscWave[0]]
		  [(int)plusMod(_global.channel[c].voices[j].op[0].index,
				(float)RESOLUTION
				*sampleOp[1])];
		
		sample[j]=sampleOp[0];///COEFLEVEL;
		
		_global.channel[c].voices[j].isOn =
		  (_global.channel[c].voices[j].op[0].envState!=OFF);
		break;
	      case THIRD :
		sampleOp[3]=ampOp[3]
		  *waveTable[_preset[c]->oscWave[3]]
		  [(int)plusMod(_global.channel[c].voices[j].op[3].index,
				(float)RESOLUTION
				*_global.channel[c].voices[j].sampleFeedback)];
		sampleOp[2]=ampOp[2]
		  *waveTable[_preset[c]->oscWave[2]]
		  [(int)_global.channel[c].voices[j].op[2].index];
		sampleOp[1]=ampOp[1]
		  *waveTable[_preset[c]->oscWave[1]]
		  [(int)plusMod(_global.channel[c].voices[j].op[1].index,
				(float)RESOLUTION*sampleOp[2])];
		sampleOp[0]=ampOp[0]
		  *waveTable[_preset[c]->oscWave[0]]
		  [(int)plusMod(_global.channel[c].voices[j].op[0].index,
				(float)RESOLUTION
				*(sampleOp[3]+sampleOp[1])/2.0)];
		
		sample[j]=sampleOp[0];///COEFLEVEL;
		
		_global.channel[c].voices[j].isOn = 
		  (_global.channel[c].voices[j].op[0].envState!=OFF);
		break;
	      case FOURTH :
		sampleOp[3]=ampOp[3]
		  *waveTable[_preset[c]->oscWave[3]]
		  [(int)plusMod(_global.channel[c].voices[j].op[3].index,
				(float)RESOLUTION
				*_global.channel[c].voices[j].sampleFeedback)];
		sampleOp[2]=ampOp[2]
		  *waveTable[_preset[c]->oscWave[2]]
		  [(int)plusMod(_global.channel[c].voices[j].op[2].index,
				(float)RESOLUTION
				*sampleOp[3])];
		sampleOp[1]=ampOp[1]
		  *waveTable[_preset[c]->oscWave[1]]
		  [(int)_global.channel[c].voices[j].op[1].index];
		sampleOp[0]=ampOp[0]
		  *waveTable[_preset[c]->oscWave[0]]
		  [(int)plusMod(_global.channel[c].voices[j].op[0].index,
				(float)RESOLUTION
				*(sampleOp[1]+sampleOp[2])/2.0)];
		
		sample[j]=sampleOp[0];///COEFLEVEL;
		
		_global.channel[c].voices[j].isOn =
		  (_global.channel[c].voices[j].op[0].envState!=OFF);
		break;
	      case FIFTH :
		sampleOp[3]=ampOp[3]
		  *waveTable[_preset[c]->oscWave[3]]
		  [(int)plusMod(_global.channel[c].voices[j].op[3].index,
				(float)RESOLUTION
				*_global.channel[c].voices[j].sampleFeedback)];
		sampleOp[2]=ampOp[2]
		  *waveTable[_preset[c]->oscWave[2]]
		  [(int)plusMod(_global.channel[c].voices[j].op[2].index,
				(float)RESOLUTION*sampleOp[3])];
		sampleOp[1]=ampOp[1]
		  *waveTable[_preset[c]->oscWave[1]]
		  [(int)_global.channel[c].voices[j].op[1].index];
		sampleOp[0]=ampOp[0]
		  *waveTable[_preset[c]->oscWave[0]]
		  [(int)plusMod(_global.channel[c].voices[j].op[0].index,
				(float)RESOLUTION*sampleOp[1])];
		
		sample[j]=(sampleOp[0]+sampleOp[2])/2.0;///COEFLEVEL;
		
		_global.channel[c].voices[j].isOn = 
		  (_global.channel[c].voices[j].op[0].envState!=OFF
		   ||_global.channel[c].voices[j].op[2].envState!=OFF);
		break;
	      case SIXTH :
		sampleOp[3]=ampOp[3]
		  *waveTable[_preset[c]->oscWave[3]]
		  [(int)plusMod(_global.channel[c].voices[j].op[3].index,
				(float)RESOLUTION
				*_global.channel[c].voices[j].sampleFeedback)];
		sampleOp[2]=ampOp[2]
		  *waveTable[_preset[c]->oscWave[2]]
		  [(int)plusMod(_global.channel[c].voices[j].op[2].index,
				(float)RESOLUTION*sampleOp[3])];
		sampleOp[1]=ampOp[1]
		  *waveTable[_preset[c]->oscWave[1]]
		  [(int)plusMod(_global.channel[c].voices[j].op[1].index,
				(float)RESOLUTION*sampleOp[3])];
		sampleOp[0]=ampOp[0]
		  *waveTable[_preset[c]->oscWave[0]]
		  [(int)plusMod(_global.channel[c].voices[j].op[0].index,
				(float)RESOLUTION*sampleOp[3])];
		
		sample[j]=(sampleOp[0]+sampleOp[1]+sampleOp[2])/3.0;
		
		_global.channel[c].voices[j].isOn = 
		  (_global.channel[c].voices[j].op[0].envState!=OFF);
		break;
	      case SEVENTH :
		sampleOp[3]=ampOp[3]
		  *waveTable[_preset[c]->oscWave[3]]
		  [(int)plusMod(_global.channel[c].voices[j].op[3].index,
				(float)RESOLUTION
				*_global.channel[c].voices[j].sampleFeedback)];
		sampleOp[2]=ampOp[2]
		  *waveTable[_preset[c]->oscWave[2]]
		  [(int)plusMod(_global.channel[c].voices[j].op[2].index,
				(float)RESOLUTION*sampleOp[3])];
		sampleOp[1]=ampOp[1]
		  *waveTable[_preset[c]->oscWave[1]]
		  [(int)_global.channel[c].voices[j].op[1].index];
		sampleOp[0]=ampOp[0]*waveTable[_preset[c]->oscWave[0]]
		  [(int)_global.channel[c].voices[j].op[0].index];
		
		sample[j]=(sampleOp[0]+sampleOp[1]+sampleOp[2])/3.0;
		
		_global.channel[c].voices[j].isOn =
		  (_global.channel[c].voices[j].op[0].envState!=OFF);
		break;		
	      case EIGHTH :
		sampleOp[3]=ampOp[3]
		  *waveTable[_preset[c]->oscWave[3]]
		  [(int)plusMod(_global.channel[c].voices[j].op[3].index,
				(float)RESOLUTION
				*_global.channel[c].voices[j].sampleFeedback)];
		sampleOp[2]=ampOp[2]
		  *waveTable[_preset[c]->oscWave[2]]
		  [(int)_global.channel[c].voices[j].op[2].index];
		sampleOp[1]=ampOp[1]
		  *waveTable[_preset[c]->oscWave[1]]
		  [(int)_global.channel[c].voices[j].op[1].index];
		sampleOp[0]=ampOp[0]
		  *waveTable[_preset[c]->oscWave[0]]
		  [(int)_global.channel[c].voices[j].op[0].index];
		
		sample[j]=
		  (sampleOp[0]+sampleOp[1]+sampleOp[2]+sampleOp[3])
		  /4.0;
		
		_global.channel[c].voices[j].isOn =
		  (_global.channel[c].voices[j].op[0].envState!=OFF
		   || _global.channel[c].voices[j].op[1].envState!=OFF
		   || _global.channel[c].voices[j].op[2].envState!=OFF
		   || _global.channel[c].voices[j].op[3].envState!=OFF);
		break;
	      default : printf("Error : No algorithm");
		break;
	      }
	      
	      _global.channel[c].voices[j].volume=
		ampOp[0]+ampOp[1]+ampOp[2]+ampOp[3];
	      
	      _global.channel[c].voices[j].sampleFeedback =
		sampleOp[3]*_global.channel[c].feedbackAmp;
	      
	      tempChannelOutput += sample[j];
	    }
	  }
	  //printf("left out = %f, temp out = %f, left amp = %f\n",
	  //tempLeftOutput, tempChannelOutput, _global.channel[c].ampLeft);
 
	  tempChannelLeftOutput = tempChannelOutput*_global.channel[c].ampLeft;
	  tempChannelRightOutput=tempChannelOutput*_global.channel[c].ampRight;
	  
	  if(_global.isChorusActivated) {
	    _global.lastInputLeftChorusSample += tempChannelLeftOutput *
	      _global.channel[c].chorusAmount;
	    _global.lastInputRightChorusSample += tempChannelRightOutput *
	      _global.channel[c].chorusAmount;
	  }
	  if(_global.isReverbActivated) {
	    _global.lastInputLeftReverbSample += tempChannelLeftOutput *
	      _global.channel[c].reverbAmount;
	    _global.lastInputRightReverbSample += tempChannelRightOutput *
	      _global.channel[c].reverbAmount;
	  }
	  if(_global.isDelayActivated) {
	    _global.lastInputLeftDelaySample += tempChannelLeftOutput *
	      _global.channel[c].delayAmount;
	    _global.lastInputRightDelaySample += tempChannelRightOutput *
	      _global.channel[c].delayAmount;
	  }
	  tempLeftOutput += tempChannelLeftOutput;
	  tempRightOutput += tempChannelRightOutput;
	}
      }
      _global.lastLeftSample = tempLeftOutput * _global.masterVolume;
      _global.lastRightSample = tempRightOutput * _global.masterVolume;
    }
    leftOutput[i] += _global.lastLeftSample;
    rightOutput[i] += _global.lastRightSample;
	  
    if(_global.isChorusActivated) {
      tempInputChorus[0][i] = _global.lastInputLeftChorusSample;
      tempInputChorus[1][i] = _global.lastInputRightChorusSample;
    }
    if(_global.isReverbActivated) {
      tempInputReverb[0][i] = _global.lastInputLeftReverbSample;
      tempInputReverb[1][i] = _global.lastInputRightReverbSample;
    }    
    if(_global.isDelayActivated) {
      tempInputDelay[0][i] = _global.lastInputLeftDelaySample;
      tempInputDelay[1][i] = _global.lastInputRightDelaySample;
    }    
    
    _global.qualityCounter++;
    _global.qualityCounter %= _global.qualityCounterTop;
  }
  //apply Filter
  if(_global.filter) _dryFilter->process(leftOutput, rightOutput, n);
  //Chorus
  if(_pluginIChorus && _global.isChorusActivated) {
    //apply Filter
    if(_global.filter) _chorusFilter->process(tempOutputChorus[0],
					      tempOutputChorus[1], n);
    //apply Chorus
    _pluginIChorus->apply(n, 2, tempInputChorus, tempOutputChorus);
    for(int i = 0; i < n; i++) {
      leftOutput[i] += 
	tempOutputChorus[0][i] * _global.chorusReturn * _global.masterVolume;
      rightOutput[i] +=
	tempOutputChorus[1][i] * _global.chorusReturn * _global.masterVolume;
    }
  }
  //Reverb
  if(_pluginIReverb && _global.isReverbActivated) {
    //apply Filter
    if(_global.filter) _reverbFilter->process(tempOutputReverb[0],
					      tempOutputReverb[1], n);
    //apply Reverb
    _pluginIReverb->apply(n, 2, tempInputReverb, tempOutputReverb);
    for(int i = 0; i < n; i++) {
      leftOutput[i] +=
	tempOutputReverb[0][i] * _global.reverbReturn * _global.masterVolume;
      rightOutput[i] +=
	tempOutputReverb[1][i] * _global.reverbReturn * _global.masterVolume;
    }
  }
  //Delay
  if(_pluginIDelay && _global.isDelayActivated) {
    //apply Filter
    if(_global.filter) _delayFilter->process(tempOutputDelay[0],
					     tempOutputDelay[1], n);
    //apply Delay
    _pluginIDelay->apply(n, 2, tempInputDelay, tempOutputDelay);
    for(int i = 0; i < n; i++) {
      leftOutput[i] +=
	tempOutputDelay[0][i] * _global.delayReturn * _global.masterVolume;
      rightOutput[i] +=
	tempOutputDelay[1][i] * _global.delayReturn * _global.masterVolume;
    }
  }
}


//---------------------------------------------------------
//   inst
//---------------------------------------------------------

class QWidget;

///static Mess* instantiate(int sr, const char*)
static Mess* instantiate(int sr, QWidget*, QString* /* projectPathPtr */, const char*)
{
    DeicsOnze* deicsonze = new DeicsOnze();
    deicsonze->setSampleRate(sr);
    return deicsonze;
}

extern "C" {
    static MESS descriptor = {
	"DeicsOnze",
	"DeicsOnze FM DX11/TX81Z emulator",
	"0.5.5",      // version string
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

