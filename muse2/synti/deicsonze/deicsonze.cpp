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

#include <cmath>
#include <list>

#include <stdio.h>

#include "config.h"

#include "libsynti/mess.h"
#include "muse/midictrl.h"
#include "deicsonze.h"

#include "deicsonzegui.h"

#include "muse/midi.h"
#define ABS(x) (x>=0?x:-x)


float DeicsOnze::waveTable[NBRWAVES][RESOLUTION];
int DeicsOnze::useCount = 0;

//---------------------------------------------------------
//   DeicsOnze
//---------------------------------------------------------

DeicsOnze::DeicsOnze() : Mess(1)
{
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



  srand(time(0));   // initialize random number generator

  loadSutulaPresets();

  initGlobal();
  initVoices();
  _preset = new Preset();
  _preset->initPreset();

  setPreset();

  _gui = new DeicsOnzeGui(this);
  _gui->setCaption(QString("DeicsOnze"));
  _gui->show();
}

//---------------------------------------------------------
//   ~DeicsOnze
//---------------------------------------------------------

DeicsOnze::~DeicsOnze()
{
    //if (--useCount == 0)
    //delete[] sine_table;
}

//---------------------------------------------------------
//   guiVisible
//---------------------------------------------------------
bool DeicsOnze::guiVisible() const
{
    return _gui->isVisible();
}

//---------------------------------------------------------
// showGui
//---------------------------------------------------------
void DeicsOnze::showGui(bool val)
{
    _gui->setShown(val);
}

//---------------------------------------------------------
//   getGeometry
//---------------------------------------------------------

void DeicsOnze::getGeometry(int* x, int* y, int* w, int* h) const {
    QPoint pos(_gui->pos());
    QSize size(_gui->size());
    *x = pos.x();
    *y = pos.y();
    *w = size.width();
    *h = size.height();
}

//---------------------------------------------------------
//   setGeometry
//---------------------------------------------------------

void DeicsOnze::setGeometry(int x, int y, int w, int h) {
    _gui->resize(QSize(w, h));
    _gui->move(QPoint(x, y));
}

//---------------------------------------------------------
// initGlobal
//---------------------------------------------------------
void DeicsOnze::initGlobal() {
    _global.amp=1.0/(double)NBRVOICES;
    _global.sustain=false;
    _global.pitchBendCoef=1.0;
    _global.lfoIndex=0;
    _numPatch=0;
}

//---------------------------------------------------------
// initVoices
//---------------------------------------------------------
void DeicsOnze::initVoices() {
    for(int i=0; i<NBRVOICES; i++) {
	_voices[i].isOn=false;
	_voices[i].isSustained=false;
    }
}

//---------------------------------------------------------
// initPreset
//   put the preset in the initial state as defined by YAMAHA
//---------------------------------------------------------
void Preset::initPreset() {
    //algorithm
    algorithm=FIRST;
    //feedeback
    feedback=0;
    //lfo
    lfo.wave=TRIANGL;
    lfo.speed=35;
    lfo.delay=0;
    lfo.pModDepth=0;
    lfo.aModDepth=0;
    lfo.sync=false;
  //sensitivity
    sensitivity.pitch=6;
    sensitivity.amplitude=0;
    sensitivity.ampOn[0]=false;
    sensitivity.ampOn[1]=false;
    sensitivity.ampOn[2]=false;
    sensitivity.ampOn[3]=false;
    sensitivity.egBias[0]=0;
    sensitivity.egBias[1]=0;
    sensitivity.egBias[2]=0;
    sensitivity.egBias[3]=0;
    sensitivity.keyVelocity[0]=0;
    sensitivity.keyVelocity[1]=0;
    sensitivity.keyVelocity[2]=0;
    sensitivity.keyVelocity[3]=0;
    //frequency
    frequency[0].ratio=1.0;
    frequency[1].ratio=1.0;
    frequency[2].ratio=1.0;
    frequency[3].ratio=1.0;
    frequency[0].isFix=false;
    frequency[1].isFix=false;
    frequency[2].isFix=false;
    frequency[3].isFix=false;
    frequency[0].freq=255.0;
    frequency[1].freq=255.0;
    frequency[2].freq=255.0;
    frequency[3].freq=255.0;
  //oscWave
    oscWave[0]=W1;
    oscWave[1]=W1;
    oscWave[2]=W1;
    oscWave[3]=W1;
  //detune
    detune[0]=0;
    detune[1]=0;
    detune[2]=0;
    detune[3]=0;
  //eg
    eg[0].ar=31;
    eg[1].ar=31;
    eg[2].ar=31;
    eg[3].ar=31;
    eg[0].d1r=31;
    eg[1].d1r=31;
    eg[2].d1r=31;
    eg[3].d1r=31;
    eg[0].d1l=15;
    eg[1].d1l=15;
    eg[2].d1l=15;
    eg[3].d1l=15;
    eg[0].d2r=0;
    eg[1].d2r=0;
    eg[2].d2r=0;
    eg[3].d2r=0;
    eg[0].rr=15;
    eg[1].rr=15;
    eg[2].rr=15;
    eg[3].rr=15;
    eg[0].egShift=VOF;
    eg[1].egShift=VOF;
    eg[2].egShift=VOF;
    eg[3].egShift=VOF;
    //pitchEg
    pitchEg.pr1=99;
    pitchEg.pr2=99;
    pitchEg.pr3=99;
    pitchEg.pl1=50;
    pitchEg.pl2=50;
    pitchEg.pl3=50;
    //outLevel
    outLevel[0]=90;
    outLevel[1]=0;
    outLevel[2]=0;
    outLevel[3]=0;
    //scaling
    scaling.rate[0]=0;
    scaling.rate[1]=0;
    scaling.rate[2]=0;
    scaling.rate[3]=0;
    scaling.level[0]=0;
    scaling.level[1]=0;
    scaling.level[2]=0;
    scaling.level[3]=0;
    //function
    function.transpose=0;
    function.mode=POLY;
    function.pBendRange=4;
    function.portamento=FULL;
    function.portamentoTime=0;
    function.fcVolume=40;
    function.fcPitch=0;
    function.fcAmplitude=0;
    function.mwPitch=50;
    function.mwAmplitude=0;
    function.bcPitch=0;
    function.bcAmplitude=0;
    function.bcPitchBias=0;
    function.bcEgBias=0;
    function.atPitch=0;
    function.atAmplitude=0;
    function.atPitchBias=0;
    function.atEgBias=0;
    function.reverbRate=0;
    globalDetune=0;
    //Name
    name="INIT VOICE";
    //Subcategory
    subcategory="NONE";
    //Category
    category="NONE";
}


//---------------------------------------------------------
// findPreset
//  return the first preset corresponding of lbank and prog
//---------------------------------------------------------
Preset* presetSet::findPreset(int lbank, int prog) {
    std::vector<Preset*>::iterator pvi;
    for(pvi=_presetVector.begin(); pvi!=_presetVector.end(); pvi++)
	if((*pvi)->bank==lbank && (*pvi)->prog==prog) {
	    return(*pvi);
	}
    return NULL;
}
Preset* subcategorySet::findPreset(int lbank, int prog) {
    Preset* p_preset;
    std::vector<presetSet*>::iterator psvi;
    for(psvi=_subcategoryVector.begin();psvi!=_subcategoryVector.end(); psvi++)
    {
	p_preset=(*psvi)->findPreset(lbank, prog);
	if(p_preset) return p_preset;
    }
    return NULL;
}
Preset* categorySet::findPreset(int lbank, int prog) {
    Preset* p_preset;
    std::vector<subcategorySet*>::iterator ssvi;
    for(ssvi=_categoryVector.begin(); ssvi!=_categoryVector.end(); ssvi++)
    {
	p_preset=(*ssvi)->findPreset(lbank, prog);
	if(p_preset) return p_preset;
    }
    return NULL;
}
Preset* DeicsOnze::findPreset(int lbank, int prog) {
    return _categorySet->findPreset(lbank, prog);
}

//---------------------------------------------------------
// findSubcategorySet
//  take string of a category and return the subcategorySet
//---------------------------------------------------------
subcategorySet* categorySet::findSubcategorySet(std::string s) {
    std::vector<subcategorySet*>::iterator ssvi=_categoryVector.begin();
    while(ssvi!=_categoryVector.end() && s!=(*ssvi)->_categoryName) ssvi++;
    return(*ssvi);
}
//---------------------------------------------------------
// findPresetSet
//  take string of a subcategory and return the presetSet
//---------------------------------------------------------
presetSet* subcategorySet::findPresetSet(std::string s) {
    std::vector<presetSet*>::iterator pvi=_subcategoryVector.begin();
    while(pvi!=_subcategoryVector.end() && s!=(*pvi)->_subcategoryName) pvi++;
    //if (pvi==NULL) printf("presetSet %s doesn't exist!", s);
    return(*pvi);
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
inline double delay2Time(int d)
{
    double t;
    //fitting
    t=0.07617*(double)d-0.002695*(double)(d*d)+4.214e-05*(double)(d*d*d);
    //printf("delay2Time : %f\n", t);
    return(t);
}


//----------------------------------------------------------------
// setMasterVol
//----------------------------------------------------------------
void DeicsOnze::setMasterVol(int mv)
{
    _global.amp=(double)mv/(double)MAXMASTERVOL;
}

//----------------------------------------------------------------
// getMasterVol
//----------------------------------------------------------------
int DeicsOnze::getMasterVol()
{
    return((int)(_global.amp*(double)MAXMASTERVOL));
}

//----------------------------------------------------------------
// setLfo
//----------------------------------------------------------------

void DeicsOnze::setLfo()
{
    double x;
    x=(double)_preset->lfo.speed;
    // lfoSpeed to Hz, obtained by fitting the actual curve by a polynomial
    _global.lfoFreq=-1.9389e-08*x*x*x*x*x+2.8826e-06*x*x*x*x-9.0316e-05*x*x*x
	+4.7453e-03*x*x-1.2295e-02*x+7.0347e-02;//a revoir
    //Pitch LFO
    _global.lfoMaxIndex=(_global.lfoFreq==0?0:(int)((1.0/_global.lfoFreq)
						    *(double)sampleRate()));
    _global.lfoPitch=(((double)_preset->lfo.pModDepth/(double)MAXPMODDEPTH)
		      *(COEFPLFO(_preset->sensitivity.pitch)));
    //Amplitude LFO
    _global.lfoMaxAmp=(((double)_preset->lfo.aModDepth/(double)MAXAMODDEPTH)
		    *(COEFALFO(_preset->sensitivity.amplitude)));
    //index is concidered on the frequency of the delay
    _global.lfoDelayMaxIndex=delay2Time(_preset->lfo.delay)*_global.lfoFreq;
    _global.lfoDelayInct=(double)(RESOLUTION/4)/_global.lfoDelayMaxIndex;
}

//-----------------------------------------------------------------
// setFeedback
//-----------------------------------------------------------------
void DeicsOnze::setFeedback() {
    _global.feedbackAmp=COEFFEEDBACK*exp(log(2)*(double)(_preset->feedback
							 -MAXFEEDBACK));
}

//-----------------------------------------------------------------
// setPreset
//-----------------------------------------------------------------

void DeicsOnze::setPreset() {
    setFeedback();
    setLfo();
}

//---------------------------------------------------------
// printPreset
//---------------------------------------------------------

inline void printPreset(Preset* p)
{
    printf("\n");
    printf("Algorithm : %d, Feedback : %d\n", p->algorithm, p->feedback);
    printf("LFO : ");
    switch(p->lfo.wave)
    {
	case(SAWUP) : printf("SAWUP ,"); break;
	case(SQUARE) : printf("SQUARE ,"); break;
	case(TRIANGL) : printf("TRIANGL ,"); break;
	case(SHOLD) : printf("SHOLD ,"); break;
	default : printf("No defined, "); break;
    }
    printf("Speed : %d, Delay : %d, PModD : %d, AModD : %d, ",
	   p->lfo.speed, p->lfo.delay, p->lfo.pModDepth, p->lfo.aModDepth);
    if(p->lfo.sync) printf("Sync\n"); else printf("Not Sync\n");
    printf("LFO Pitch Sensitivity : %d, LFO Amplitude Sensitivity : %d\n",
	   p->sensitivity.pitch, p->sensitivity.amplitude);
    for(int i=0; i<NBROP; i++)
    {
	printf("amp%d ",i+1);
	if(p->sensitivity.ampOn) printf("ON "); else printf("OFF ");
    }
    printf("\n");
    for(int i=0; i<NBROP; i++)
	printf("EgBias%d : %d ",i+1, p->sensitivity.egBias[i]);
    printf("\n");
    for(int i=0; i<NBROP; i++)
	printf("KVS%d : %d ",i+1, p->sensitivity.keyVelocity[i]);
    printf("\n");
    for(int i=0; i<NBROP; i++)
    {
	if(p->frequency[i].isFix)
	    printf("Freq%d : %f ",i+1, p->frequency[i].ratio);
	else printf("Ratio%d : %f ",i+1, p->frequency[i].ratio);
    }
    printf("\n");
    for(int i=0; i<NBROP; i++)
    {
	printf("OscWave%d ", i+1);
	switch(p->oscWave[i])
	{
	    case(W1) : printf("W1 "); break;
	    case(W2) : printf("W2 "); break;
	    case(W3) : printf("W3 "); break;
	    case(W4) : printf("W4 "); break;
	    case(W5) : printf("W5 "); break;
	    case(W6) : printf("W6 "); break;
	    case(W7) : printf("W7 "); break;
	    case(W8) : printf("W8 "); break;
	    default : printf("No defined "); break;
	}
    }
    printf("\n");
    for(int i=0; i<NBROP; i++)
	printf("Detune%d : %d ",i+1, p->detune[i]);
    printf("\n");
    for(int i=0; i<NBROP; i++)
    {
	printf("AR%d : %d, D1R%d : %d, D1L%d : %d, D2R%d : %d, RR%d : %d, EgShift%d : ",
	       i+1, p->eg[i].ar, i+1, p->eg[i].d1r,
	       i+1, p->eg[i].d1l, i+1, p->eg[i].d2r, i+1, p->eg[i].rr, i+1);
	switch(p->eg[i].egShift)
	{
	    case(VOF) : printf("VOF");
	    case(V48) : printf("48");
	    case(V24) : printf("24");
	    case(V12) : printf("12");
	}
	printf("\n");
    }
    printf("PitchEg pr1 : %d, pr2 : %d, pr3 : %d, pl1 : %d, pl2 : %d, pl3 : %d"
	   , p->pitchEg.pr1, p->pitchEg.pr2, p->pitchEg.pr3,
	   p->pitchEg.pl1, p->pitchEg.pl2, p->pitchEg.pl3);
    printf("\n");
    for(int i=0; i<NBROP; i++)
	printf("OutLevel%d : %d ",i+1, p->outLevel[i]);
    printf("\n");
    printf("Name : %s\n", p->name.c_str());
}

void presetSet::printSubcategory() {
    std::cout << "    " << _subcategoryName << "\n";
    for(unsigned int i=0; i<_presetVector.size(); i++)
	printPreset(_presetVector[i]);
}

void subcategorySet::printCategory() {
    std::cout << "  " << _categoryName << "\n";
    for(unsigned int i=0; i<_subcategoryVector.size(); i++)
	_subcategoryVector[i]->printSubcategory();
}

void categorySet::printBank() {
    std::cout << _bankName << "\n";
    for(unsigned int i=0; i<_categoryVector.size(); i++)
	_categoryVector[i]->printCategory();
}

inline double coarseFine2Ratio(int c,int f)
{
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
    int nBank, nPreset;

    _categorySet=new categorySet("Sutula Bank");
    Preset* presetTemp;

    nBank=0;
    nPreset=0;

    //QString presetPath(INSTPREFIX);
    //presetPath += "/share/muse/presets/deicsonze/ARCH_ALIN";
    //museGlobalShare
    QString presetPath(QString(INSTPREFIX) + QString("/") +                                             
                               QString(SHAREINSTPREFIX) + QString("/") +  // This has no prefix. Default is "share", set in top cmake script.
                               QString(INSTALL_NAME));   
    presetPath += "/presets/deicsonze/ARCH_ALIN";

    file = fopen (presetPath.latin1(), "rt");
    if (file == NULL) {
	printf("can't open ");
	printf("%s",presetPath.latin1());
	printf("\n");
        return;
    }
    else
    {
	while(fgets(s, 500, file) && !strstr(s, "** Source:"))
	{
	    if (strstr(s,"* CATEGORY"))
	    {
		sscanf(s, "* CATEGORY %s", scategory);
		_categorySet->_categoryVector
		    .push_back(new subcategorySet(scategory));
	    }
	    if (strstr(s,"* SUBCATEGORY"))
	    {
		sscanf(s, "* SUBCATEGORY %s", ssubcategory);
		_categorySet->_categoryVector.back()->_subcategoryVector
		    .push_back(new presetSet(ssubcategory));
	    }
	}
	while(!feof(file))
	{
	    _categorySet->_categoryVector.back()->_subcategoryVector.back()
		->_presetVector.push_back(new Preset);
	    presetTemp=_categorySet->_categoryVector.back()
		->_subcategoryVector.back()->_presetVector.back();
	    /************* Fill the preset *****************/
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
		presetTemp->frequency[k].freq=(v & 0x7);
		presetTemp->eg[k].egShift=
		    (((v & 0x30)>>4)==0?VOF:
		     (((v & 0x30)>>4)==1?V48:
		      (((v & 0x30)>>4)==2?V24:V12)));
		fscanf(file, "%x", &v);//74, 76, 78, 80
		fin[k]=v & 0xF;
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
	    presetTemp->globalDetune=0;

	    presetTemp->subcategory=ssubcategory;
	    presetTemp->category=scategory;	
	    presetTemp->bank=nBank;
	    presetTemp->prog=nPreset;
            /******************** End of filling the preset ********/

	    nPreset++;
	    while(fgets(s, 500, file) && !strstr(s, "** Source:"))
	    {
		if (strstr(s,"* CATEGORY"))
		{
		    sscanf(s, "* CATEGORY %s", scategory);
		    _categorySet->_categoryVector
			.push_back(new subcategorySet(scategory));
		}
		if (strstr(s,"* SUBCATEGORY"))
		{
		    sscanf(s, "* SUBCATEGORY %s", ssubcategory);	
		    _categorySet->_categoryVector.back()
			->_subcategoryVector
			.push_back(new presetSet(ssubcategory));
		}
		if(strstr(s, "--Bank"))
		{
		    nBank++;
		    nPreset=0;
		}
	    }
	}
    }
    //_categorySet->printBank();
    fclose(file);
}

//---------------------------------------------------------
// minVolu2Voice
//  return the number of the voice which is the least aloud
//  and is not is the ATTACK state
//---------------------------------------------------------
int DeicsOnze::minVolu2Voice()
{
  int minVoice=0;
  double min=MAXVOLUME;
  for(int i=0; i<NBRVOICES; i++)
    {
      min=((min>_voices[i].volume
	   && _voices[i].op[0].envState!=ATTACK
	   && _voices[i].op[1].envState!=ATTACK
	   && _voices[i].op[2].envState!=ATTACK
	   && _voices[i].op[3].envState!=ATTACK)?_voices[i].volume:min);
      minVoice=(min==_voices[i].volume?i:minVoice);
    }
  return minVoice;
}

//---------------------------------------------------------
// noteOff2Voice
//  return the number of the voice off, NBRVOICES otherwise
//---------------------------------------------------------
int DeicsOnze::noteOff2Voice()
{
  int offVoice=NBRVOICES;
  for(int i=0; i<NBRVOICES; i++) offVoice=(_voices[i].isOn?offVoice:i);
  return offVoice;
}

//---------------------------------------------------------
// pitchOn2Voice
//  return the number of the voice which has the input
//   pitch and is On and not release, NBRVOICES otherwise
//---------------------------------------------------------
int DeicsOnze::pitchOn2Voice(int pitch)
{
  int pitchVoice=NBRVOICES;
  for(int i=0; i<NBRVOICES; i++)
    pitchVoice=((_voices[i].pitch==pitch
		 && _voices[i].isOn
		 && (_voices[i].op[0].envState==ATTACK
		     || _voices[i].op[0].envState==DECAY
		     || _voices[i].op[0].envState==SUSTAIN)
		 && !_voices[i].isSustained)?
		i:pitchVoice);
  return pitchVoice;
}


//---------------------------------------------------------
// pitch2freq
//---------------------------------------------------------
inline double pitch2freq(double p)
{
    return(LOWERNOTEFREQ*exp(p*log(2.0)/12.0));
}

//---------------------------------------------------------
// lfoUpdate
//  update the coefficent which multiplies the current inct in order to
//  get the right current frequency with respect to the lfo
//  update the coefficent which multiplies the amplitude.
//---------------------------------------------------------
inline void lfoUpdate(Preset* p, Global* p_g, float* wt)
{
    double delayCoef;

    if(p_g->lfoIndex==0)
    {
	if(p_g->lfoDelayIndex<(double)(RESOLUTION/4))
	{
	    delayCoef=(double)wt[(int)p_g->lfoDelayIndex];
	    p_g->lfoMaxCoefInct=exp((log(2.0)/12.0)*p_g->lfoPitch*delayCoef);
	    p_g->lfoCoefInctInct=
		exp((log(2.0)/12.0)*((2*p_g->lfoPitch*delayCoef)
				     /p_g->lfoMaxIndex));
	    p_g->lfoDelayIndex+=p_g->lfoDelayInct;
	    p_g->lfoMaxDAmp=delayCoef*p_g->lfoMaxAmp;
	}
	else
	    if(!p_g->delayPassed)
	    {
		p_g->lfoMaxCoefInct=exp((log(2.0)/12.0)*p_g->lfoPitch);
		p_g->lfoCoefInctInct=
		    exp((log(2.0)/12.0)*((2*p_g->lfoPitch)/p_g->lfoMaxIndex));
		p_g->delayPassed=true;
		p_g->lfoMaxDAmp=p_g->lfoMaxDAmp;
	    }
    }

    switch(p->lfo.wave)
	{
	    case SAWUP :
		if(p_g->lfoIndex==0)
		{
		    p_g->lfoCoefInct=1.0/(p_g->lfoMaxCoefInct);
		    p_g->lfoCoefAmp=p_g->lfoMaxDAmp/(double)p_g->lfoMaxIndex;
		    p_g->lfoAmp=1.0;
		}
		else
		{
		    p_g->lfoCoefInct*=p_g->lfoCoefInctInct;
		    p_g->lfoAmp-=p_g->lfoCoefAmp;
		}
		break;
	    case SQUARE :
		if(p_g->lfoIndex==0)
		{
		    p_g->lfoCoefInct=p_g->lfoMaxCoefInct;
		    p_g->lfoAmp=1.0;
		}
		if(p_g->lfoIndex==(p_g->lfoMaxIndex/2))
		{
		    p_g->lfoCoefInct=1.0/p_g->lfoMaxCoefInct;
		    p_g->lfoAmp=1.0-p_g->lfoMaxDAmp;
		}
		break;
	    case TRIANGL :
		if(p_g->lfoIndex==0)
		{
		    p_g->lfoCoefInct=1.0;
		    p_g->lfoCoefAmp=p_g->lfoMaxDAmp
			/(double)(p_g->lfoMaxIndex/2);
		    p_g->lfoAmp=1.0-p_g->lfoMaxDAmp/2.0;
		}
		else if(p_g->lfoIndex<(p_g->lfoMaxIndex/4))
		{
		    p_g->lfoCoefInct*=p_g->lfoCoefInctInct;
		    p_g->lfoAmp-=p_g->lfoCoefAmp;
		}
		else if(p_g->lfoIndex<((3*p_g->lfoMaxIndex)/4))
		{
		    p_g->lfoCoefInct/=p_g->lfoCoefInctInct;
		    p_g->lfoAmp+=p_g->lfoCoefAmp;
		}
		else if(p_g->lfoIndex<p_g->lfoMaxIndex)
		{
		    p_g->lfoCoefInct*=p_g->lfoCoefInctInct;
		    p_g->lfoAmp-=p_g->lfoCoefAmp;
		}
		break;
	    case SHOLD :
		if(p_g->lfoIndex==0||p_g->lfoIndex==(p_g->lfoMaxIndex/2))
		{
		    double r;//uniform random between -1.0 and 1.0
		    r = (double)(2*rand()-RAND_MAX)/(double)RAND_MAX;
		    p_g->lfoCoefInct=(r>=0.0?1.0+r*(p_g->lfoMaxCoefInct-1.0)
				      :1.0/(1.0-r*(p_g->lfoMaxCoefInct-1.0)));
		    p_g->lfoAmp=1.0-(r/2.0+0.5)*p_g->lfoMaxDAmp;
		}
		break;
	    default : printf("Error : flo wave does not exist");
		break;
	}
    p_g->lfoIndex=(p_g->lfoIndex<p_g->lfoMaxIndex?p_g->lfoIndex+1:0);
    //printf("indexLfo : %d\n",p_g->indexLfo);
}

//---------------------------------------------------------
// outLevel2Amp, Amp for amplitude //between 0.0 and 2.0 or more
//  100->2.0, 90->1.0, 80->0.5 ...
//---------------------------------------------------------
inline double outLevel2Amp(int ol)
{
    double a;
    double b;
    a = log(2)/10.0;
    b = -a*DB0LEVEL;
    return exp(a*(double)ol+b);
}

//---------------------------------------------------------
// velo2RAmp, AmpR between 0.0 and 1.0
//  return an amplitude ratio with respect to _preset->sensitivity.keyVelocity
//---------------------------------------------------------
inline double velo2AmpR(int velo, int kvs)
{
  double lev;
  lev = exp(-log(2)*kvs);
  return (lev+(1.0-lev)*((double)velo/(double)MAXVELO));
}

//---------------------------------------------------------
// envAR2s
//  return the time in second of the ATTACK duration
//---------------------------------------------------------
inline double envAR2s(int ar)
{
    //determined using the fitting feature of gnuplot
    return 10.4423*exp(-0.353767*ar);
}

//---------------------------------------------------------
// envD1R2coef
//  return the coefficient for the exponential decrease
//  with respect to d1r and sampleRate, sr
//---------------------------------------------------------
inline double envD1R2coef(int d1r, int sr)
{
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
      return exp(alpha/(double)sr);
  }
}

//---------------------------------------------------------
// envRR2coef
//  return the coefficient for the exponential decrease
//  with respect to rr and sampleRate, sr
//---------------------------------------------------------
inline double envRR2coef(int rr, int sr)
{
  double dt;//such that amp(t+dt)=amp(t)/2
  double alpha;//such that amp(t)=exp(alpha*t)

//dt has been determined with the fitting function of gnuplot
  dt=7.06636*exp(-0.697606*(double)rr);

  //amp(0)=1
  //amp(t+dt)=amp(t)/2
  //amp(t)=exp(alpha*t)
  //amp(t+mt)
  //following the above equational system we found :
  alpha=-log(2)/dt;
  return exp(alpha/(double)sr);
}


//---------------------------------------------------------
// env2RAmp
//  return the amplitude ratio with respect to an envelope and an
//   envelope state, making evoluate the envelope
//  sr is the sample rate and st the sine_table
//---------------------------------------------------------
inline double env2AmpR(int sr, float* wt, Eg eg, OpVoice* p_opVoice)
{
  switch(p_opVoice->envState)
    {
    case ATTACK:
      p_opVoice->envIndex+=p_opVoice->envInct;
      if (p_opVoice->envIndex<(RESOLUTION/4))
	{
	  p_opVoice->envLevel=wt[(int)p_opVoice->envIndex];
	}
      else
	{
	  //printf("DECAY\n");
	  p_opVoice->envState=DECAY;
	  p_opVoice->envLevel=1.0;
	  p_opVoice->coefVLevel=envD1R2coef(eg.d1r, sr);
	}
      break;
    case DECAY:
      if (p_opVoice->envLevel>((double)eg.d1l/(double)MAXD1L)+COEFERRDECSUS)
	{
	  p_opVoice->envLevel*=p_opVoice->coefVLevel;
	}
      else
	{
	  //printf("SUSTAIN\n");
	  p_opVoice->envState=SUSTAIN;
	  p_opVoice->envLevel=((double)eg.d1l/(double)MAXD1L);
	  p_opVoice->coefVLevel=envD1R2coef(eg.d2r, sr);//probably the same
	}
      break;
    case SUSTAIN:
      if (p_opVoice->envLevel>COEFERRSUSREL)
	{
	  p_opVoice->envLevel*=p_opVoice->coefVLevel;
	}
      else
	{
	  //printf("OFF\n");
	  p_opVoice->envState=OFF;
	  p_opVoice->envLevel=0.0;
	}
      break;
    case RELEASE:
      if (p_opVoice->envLevel > COEFERRSUSREL)
	{
	  p_opVoice->envLevel*=p_opVoice->coefVLevel;
	}
      else
	{
	  p_opVoice->envState=OFF;
	  p_opVoice->envLevel=0.0;
	}
      break;
    default: 
       printf("Error case envelopeState");
       /* fall thru */
    case OFF:
       p_opVoice->envLevel = 0.0;
       break;
     }
    return p_opVoice->envLevel;


}

//---------------------------------------------------------
// programSelect
//---------------------------------------------------------

void DeicsOnze::programSelect(int /*ch*/, int lbank, int prog) {
    Preset* p_preset;
    p_preset=findPreset(lbank, prog);
    if (p_preset) _preset=p_preset;
    else _preset->initPreset();
    setPreset();
}

//---------------------------------------------------------
//   setPitchBendCoef
//---------------------------------------------------------

void DeicsOnze::setPitchBendCoef(int /*ch*/, int val) {
    _global.pitchBendCoef=exp(log(2)
			      *((double)_preset->function.pBendRange
				      /(double)MAXPBENDRANGE)
			      *((double)val/(double)MAXPITCHBENDVALUE));
}

//---------------------------------------------------------
// setSustain
//---------------------------------------------------------
void DeicsOnze::setSustain(int /*ch*/, int val) {
    _global.sustain=(val>64);
    if(!_global.sustain) 
	for(int i=0; i<NBRVOICES; i++)
	    if(_voices[i].isSustained) {
		for(int j=0; j<NBROP; j++) {
		    _voices[i].op[j].envState=RELEASE;
		    _voices[i].op[j].coefVLevel=envRR2coef(_preset->eg[j].rr
							   ,sampleRate());
		}
		_voices[i].isSustained=false;
	    }
}

//---------------------------------------------------------
//   setController
//---------------------------------------------------------

bool DeicsOnze::setController(int ch, int ctrl, int val)
{
    switch(ctrl) {
	case CTRL_PROGRAM:
	{
	    int hbank = (val & 0xff0000) >> 16;
	    int lbank = (val & 0xff00) >> 8;
	    if (hbank > 127)  // map "dont care" to 0
		hbank = 0;
	    if (lbank > 127)
		lbank = 0;
	    if (lbank == 127 || ch == 9)       // drum HACK
		lbank = 128;
	    int prog  = val & 0x7f;
	    programSelect(ch, lbank, prog);
	    _gui->updatePreset();
	}
	break;
	case CTRL_PITCH:
            setPitchBendCoef(ch, val);
	    break;
	case CTRL_SUSTAIN:
	    setSustain(ch, val);
	    break;
	default:
	    break;
    }
    return false;
}

//---------------------------------------------------------
//   getPatchName
//---------------------------------------------------------

const char* DeicsOnze::getPatchName(int /*ch*/, int val, int, bool /*drum*/) const
{
    int prog =   val & 0xff;
    if(val == CTRL_VAL_UNKNOWN || prog == 0xff)
          return "<unknown>";
    prog &= 0x7f;
    
    Preset* p_preset;
    int hbank = (val & 0xff0000) >> 16;
    int lbank = (val & 0xff00) >> 8;
    if (hbank > 127)
	hbank = 0;
    if (lbank > 127)
	lbank = 0;
    if (lbank == 127)       // drum HACK
	lbank = 128;
    const char* name="<unknown>";
    p_preset=_categorySet->findPreset(lbank, prog);
    if (p_preset) name=const_cast<char *>(p_preset->name.c_str());
    return name;
}


//---------------------------------------------------------
//   getPatchInfo
//---------------------------------------------------------

const MidiPatch* DeicsOnze::getPatchInfo(int /*ch*/, const MidiPatch* /*p*/) const
{
    /*if(_numPatch<NBRBANKPRESETS)
    {
	_patch.typ   = 0;
	_patch.name  = bankA[_numPatch].name;
	_patch.lbank = 1;
	_patch.prog  = _numPatch;
	return &_patch;
	}*/
    return 0;
}


//---------------------------------------------------------
//   playNote
//    process note on
//---------------------------------------------------------

bool DeicsOnze::playNote(int /*channel*/, int pitch, int velo)
{
  int newVoice;
  int nO2V;
  int p2V;
  
  p2V=pitchOn2Voice(pitch);
  
  if(velo==0) {//Note off
      if(p2V<NBRVOICES)
      {
	  if(_global.sustain) _voices[p2V].isSustained=true;
	  else
	      for(int i=0; i<NBROP; i++)
	      {
		  _voices[p2V].op[i].envState=RELEASE;
		  _voices[p2V].op[i].coefVLevel=envRR2coef(_preset->eg[i].rr
							   ,sampleRate());
	      }
	  return false;}
      //else printf("error over NBRVOICES\n");
  }
  else //Note on
    {
      nO2V=noteOff2Voice();
      newVoice=((nO2V==NBRVOICES)?minVolu2Voice():nO2V);

      _voices[newVoice].isOn=true;
      _voices[newVoice].sampleFeedback=0.0;
      _voices[newVoice].pitch=pitch;

      /*if(_preset->lfo.sync)*/ _global.lfoIndex=0;//a revoir
      _global.lfoDelayIndex=0.0;
      _global.delayPassed=false;

      for(int i=0; i<NBROP; i++)
	{
	  _voices[newVoice].op[i].amp=outLevel2Amp(_preset->outLevel[i])
	      *velo2AmpR(velo, _preset->sensitivity.keyVelocity[i])
	      *note2Amp((double)(pitch+_preset->function.transpose),
			_preset->scaling.level[i]);
	  _voices[newVoice].op[i].index=0.0;
	  _voices[newVoice].op[i].freq=
	      (pitch2freq((double)_preset->globalDetune
			  /(double)MAXGLOBALDETUNE)
	       /LOWERNOTEFREQ)*
	      (_preset->frequency[i].isFix?
	       _preset->frequency[i].freq:
	       (_preset->frequency[i].ratio
		*pitch2freq((double)(pitch+_preset->function.transpose)
			     +(double)_preset->detune[i]*COEFDETUNE)));
	  _voices[newVoice].op[i].inct=(double)RESOLUTION
	    /((double)sampleRate()/_voices[newVoice].op[i].freq);
	  _voices[newVoice].op[i].envState=ATTACK;
	  _voices[newVoice].op[i].envIndex=0.0;
	  _voices[newVoice].op[i].envInct=(_preset->eg[i].ar==0?0:
					   (double)(RESOLUTION/4)
					   /(envAR2s(_preset->eg[i].ar)
					     *(double)sampleRate()));
	}
      return false;
    }
  return false;
}

//---------------------------------------------------------
// plusMod
//  add two doubles modulo SINRESOLUTION
//---------------------------------------------------------
inline double plusMod(double x, double y)
{
  double res;
  res=x+y;
  if (res>=0) while (res >= (double)RESOLUTION) res-=(double)RESOLUTION;
  else while (res < 0) res+=(double)RESOLUTION;
  return res;
}


//---------------------------------------------------------
//   write
//    synthesize n samples into buffer+offset
//---------------------------------------------------------

void DeicsOnze::process(float** buffer, int offset, int n)
{
  float* p = buffer[0] + offset;
  float sample[NBRVOICES];
  float resSample;
  float sampleOp[NBROP];
  float ampOp[NBROP];
  for(int i = 0; i < n; i++)
    {
      resSample = 0;
      //stepProcess return the result to resSample

      //Global
      lfoUpdate(_preset, &_global, waveTable[W2]);

      //per voice
      for(int j=0; j<NBRVOICES; j++)
      {
	  if (_voices[j].isOn) 
	  {
	      for(int k=0; k<NBROP; k++) 
	      {
		  _voices[j].op[k].index=
		      plusMod(_voices[j].op[k].index,
			      _global.lfoCoefInct*_voices[j].op[k].inct
			      *_global.pitchBendCoef);
		  
		  ampOp[k]=_voices[j].op[k].amp*COEFLEVEL
		      *(_preset->sensitivity.ampOn[k]?_global.lfoAmp:1.0)
		      *env2AmpR(sampleRate(), waveTable[W2],
				_preset->eg[k], &_voices[j].op[k]);
	      }
	      
	      switch(_preset->algorithm)
	      {
		  case FIRST :
		      sampleOp[3]=ampOp[3]
			  *waveTable[_preset->oscWave[3]]
			  [(int)plusMod(_voices[j].op[3].index,
					(float)RESOLUTION
					*_voices[j].sampleFeedback)];
		      sampleOp[2]=ampOp[2]
			  *waveTable[_preset->oscWave[2]]
			  [(int)plusMod(_voices[j].op[2].index,
					(float)RESOLUTION*sampleOp[3])];
		      sampleOp[1]=ampOp[1]
			  *waveTable[_preset->oscWave[1]]
			  [(int)plusMod(_voices[j].op[1].index,
					(float)RESOLUTION*sampleOp[2])];
		      sampleOp[0]=ampOp[0]
			  *waveTable[_preset->oscWave[0]]
			  [(int)plusMod(_voices[j].op[0].index,
					(float)RESOLUTION*sampleOp[1])];
		
		      sample[j]=sampleOp[0];///COEFLEVEL;
		
		      _voices[j].isOn=(_voices[j].op[0].envState!=OFF);
		      break;
		  case SECOND :
		      sampleOp[3]=ampOp[3]
			  *waveTable[_preset->oscWave[3]]
			  [(int)plusMod(_voices[j].op[3].index,
					(float)RESOLUTION
					*_voices[j].sampleFeedback)];
		      sampleOp[2]=ampOp[2]
			  *waveTable[_preset->oscWave[2]]
			  [(int)_voices[j].op[2].index];
		      sampleOp[1]=ampOp[1]
			  *waveTable[_preset->oscWave[1]]
			  [(int)plusMod(_voices[j].op[1].index,
					(float)RESOLUTION
					*(sampleOp[2]+sampleOp[3])/2.0)];
		      sampleOp[0]=ampOp[0]
			  *waveTable[_preset->oscWave[0]]
			  [(int)plusMod(_voices[j].op[0].index,
					(float)RESOLUTION
					*sampleOp[1])];
		
		      sample[j]=sampleOp[0];///COEFLEVEL;
		
		      _voices[j].isOn=(_voices[j].op[0].envState!=OFF);
			  break;
		  case THIRD :
		      sampleOp[3]=ampOp[3]
			  *waveTable[_preset->oscWave[3]]
			  [(int)plusMod(_voices[j].op[3].index,
					(float)RESOLUTION
					*_voices[j].sampleFeedback)];
		      sampleOp[2]=ampOp[2]
			  *waveTable[_preset->oscWave[2]]
			  [(int)_voices[j].op[2].index];
		      sampleOp[1]=ampOp[1]
			  *waveTable[_preset->oscWave[1]]
			  [(int)plusMod(_voices[j].op[1].index,
					(float)RESOLUTION*sampleOp[2])];
		      sampleOp[0]=ampOp[0]
			  *waveTable[_preset->oscWave[0]]
			  [(int)plusMod(_voices[j].op[0].index,
					(float)RESOLUTION
					*(sampleOp[3]+sampleOp[1])/2.0)];
		
		      sample[j]=sampleOp[0];///COEFLEVEL;
		
		      _voices[j].isOn=(_voices[j].op[0].envState!=OFF);
		      break;
		  case FOURTH :
		      sampleOp[3]=ampOp[3]
			  *waveTable[_preset->oscWave[3]]
			  [(int)plusMod(_voices[j].op[3].index,
					(float)RESOLUTION
					*_voices[j].sampleFeedback)];
		      sampleOp[2]=ampOp[2]
			  *waveTable[_preset->oscWave[2]]
			  [(int)plusMod(_voices[j].op[2].index,
					(float)RESOLUTION
					*sampleOp[3])];
		      sampleOp[1]=ampOp[1]
			  *waveTable[_preset->oscWave[1]]
			  [(int)_voices[j].op[1].index];
		      sampleOp[0]=ampOp[0]
			  *waveTable[_preset->oscWave[0]]
			  [(int)plusMod(_voices[j].op[0].index,
					(float)RESOLUTION
					*(sampleOp[1]+sampleOp[2])/2.0)];
		
		      sample[j]=sampleOp[0];///COEFLEVEL;
		
		      _voices[j].isOn=(_voices[j].op[0].envState!=OFF);
		      break;
		  case FIFTH :
		      sampleOp[3]=ampOp[3]
			  *waveTable[_preset->oscWave[3]]
			  [(int)plusMod(_voices[j].op[3].index,
					(float)RESOLUTION
					*_voices[j].sampleFeedback)];
		      sampleOp[2]=ampOp[2]
			  *waveTable[_preset->oscWave[2]]
			  [(int)plusMod(_voices[j].op[2].index,
					(float)RESOLUTION*sampleOp[3])];
		      sampleOp[1]=ampOp[1]
			  *waveTable[_preset->oscWave[1]]
			  [(int)_voices[j].op[1].index];
		      sampleOp[0]=ampOp[0]
			  *waveTable[_preset->oscWave[0]]
			  [(int)plusMod(_voices[j].op[0].index,
					(float)RESOLUTION*sampleOp[1])];
		
		      sample[j]=(sampleOp[0]+sampleOp[2])/2.0;///COEFLEVEL;
		
		      _voices[j].isOn=(_voices[j].op[0].envState!=OFF
				       ||_voices[j].op[2].envState!=OFF);
		      break;
		  case SIXTH :
		      sampleOp[3]=ampOp[3]
			  *waveTable[_preset->oscWave[3]]
			  [(int)plusMod(_voices[j].op[3].index,
					(float)RESOLUTION
					*_voices[j].sampleFeedback)];
		      sampleOp[2]=ampOp[2]
			  *waveTable[_preset->oscWave[2]]
			  [(int)plusMod(_voices[j].op[2].index,
					(float)RESOLUTION*sampleOp[3])];
		      sampleOp[1]=ampOp[1]
			  *waveTable[_preset->oscWave[1]]
			  [(int)plusMod(_voices[j].op[1].index,
					(float)RESOLUTION*sampleOp[3])];
		      sampleOp[0]=ampOp[0]
			  *waveTable[_preset->oscWave[0]]
			  [(int)plusMod(_voices[j].op[0].index,
					(float)RESOLUTION*sampleOp[3])];
		
		      sample[j]=(sampleOp[0]+sampleOp[1]+sampleOp[2])/3.0;
		
		      _voices[j].isOn=(_voices[j].op[0].envState!=OFF);
		      break;
		  case SEVENTH :
		      sampleOp[3]=ampOp[3]
			  *waveTable[_preset->oscWave[3]]
			  [(int)plusMod(_voices[j].op[3].index,
					(float)RESOLUTION
					*_voices[j].sampleFeedback)];
		      sampleOp[2]=ampOp[2]
			  *waveTable[_preset->oscWave[2]]
			  [(int)plusMod(_voices[j].op[2].index,
					(float)RESOLUTION*sampleOp[3])];
		      sampleOp[1]=ampOp[1]
			  *waveTable[_preset->oscWave[1]]
			  [(int)_voices[j].op[1].index];
		      sampleOp[0]=ampOp[0]*waveTable[_preset->oscWave[3]]
			  [(int)_voices[j].op[0].index];
		
		      sample[j]=(sampleOp[0]+sampleOp[1]+sampleOp[2])/3.0;
		
		      _voices[j].isOn=(_voices[j].op[0].envState!=OFF);
		      break;		
		  case EIGHTH :
		      sampleOp[3]=ampOp[3]
			  *waveTable[_preset->oscWave[3]]
			  [(int)plusMod(_voices[j].op[3].index,
					(float)RESOLUTION
					*_voices[j].sampleFeedback)];
		      sampleOp[2]=ampOp[2]
			  *waveTable[_preset->oscWave[3]]
			  [(int)_voices[j].op[2].index];
		      sampleOp[1]=ampOp[1]
			  *waveTable[_preset->oscWave[3]]
			  [(int)_voices[j].op[1].index];
		      sampleOp[0]=ampOp[0]
			  *waveTable[_preset->oscWave[3]]
			  [(int)_voices[j].op[0].index];
		
		      sample[j]=
			  (sampleOp[0]+sampleOp[1]+sampleOp[2]+sampleOp[3])
			  /4.0;
		
		      _voices[j].isOn=(_voices[j].op[0].envState!=OFF
				       || _voices[j].op[1].envState!=OFF
				       || _voices[j].op[2].envState!=OFF
				       || _voices[j].op[3].envState!=OFF);
		      break;
		  default : printf("Error : No algorithm");
			
			sampleOp[3]= sampleOp[2]= sampleOp[1]= sampleOp[0]=0; //prevent unitialiazed use of variable
		      break;
	      }
	
	      _voices[j].volume=
		  ampOp[0]+ampOp[1]+ampOp[2]+ampOp[3];
	
	      _voices[j].sampleFeedback=sampleOp[3]*_global.feedbackAmp;
	
	      resSample += sample[j];
	    }
	}
      p[i] += resSample*_global.amp;
    }
}

//---------------------------------------------------------
//   processEvent
//    All events from the sequencer go here
//---------------------------------------------------------
bool DeicsOnze::processEvent(const MidiPlayEvent& ev)
{
  switch(ev.type()) {
    case ME_CONTROLLER:
      setController(ev.channel(), ev.dataA(), ev.dataB());
      return true;
    case ME_NOTEON:
      return playNote(ev.channel(), ev.dataA(), ev.dataB());
    case ME_NOTEOFF:
      return playNote(ev.channel(), ev.dataA(), 0);
    case ME_SYSEX:
      return sysex(ev.len(), ev.data());
    case ME_PITCHBEND:
      setController(ev.channel(), CTRL_PITCH, ev.dataA());
      break;            
    case ME_PROGRAM:
      setController(ev.channel(), CTRL_PROGRAM, ev.dataA());
      break;   
    default:
      break;
  }
  return false;
}

//---------------------------------------------------------
//   inst
//---------------------------------------------------------

class QWidget;

static Mess* instantiate(int sr, QWidget*, QString* projectPathPtr, const char*)
{
    DeicsOnze* deicsonze = new DeicsOnze();
    deicsonze->setSampleRate(sr);
    return deicsonze;
}

extern "C" {
    static MESS descriptor = {
	"DeicsOnze",
	"DeicsOnze FM DX11 emulator",
	"0.2.2",      // version string
	MESS_MAJOR_VERSION, MESS_MINOR_VERSION,
	instantiate
    };

    const MESS* mess_descriptor() { return &descriptor; }
}

