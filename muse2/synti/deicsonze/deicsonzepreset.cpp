//===========================================================================
//
//    DeicsOnze an emulator of the YAMAHA DX11 synthesizer
//
//    Version 0.5.5
//
//    deicsonzepreset.cpp
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

#include "deicsonzepreset.h"
#include <iostream>
#include <algorithm>

#include <QDomElement>

//-----------------------------------------------------------
// Constructor destructor
//-----------------------------------------------------------
Preset::Preset() {_subcategory=NULL;_isUsed=false;initPreset();}
Preset::Preset(Subcategory* sub) {
    _subcategory=sub;
    _isUsed=false;
    initPreset();
    if(sub) sub->_presetVector.push_back(this);
}
Preset::Preset(Subcategory* sub, int pr) {
    _subcategory=sub;
    _isUsed=false;
    initPreset();
    prog=pr;
    if(sub) sub->_presetVector.push_back(this);
}
Preset::~Preset() {
    if(_subcategory) {
	std::vector<Preset*>::iterator iB=_subcategory->_presetVector.begin();
	std::vector<Preset*>::iterator iE=_subcategory->_presetVector.end();
	std::vector<Preset*>::iterator iP=std::find(iB, iE, this);
	if(iP!=iE) _subcategory->_presetVector.erase(iP);
	else printf("Error : preset %s not found\n", name.c_str());
    }
}
//----------------------------------------------------------
// setIsUsed(bool b)
// set the flag _isUsed and transmit in the parents
//----------------------------------------------------------
void Preset::setIsUsed(bool b) {
  if(!_isUsed) {
    _isUsed=b;
    if(_subcategory) {
      _subcategory->_isUsed=b;
      if(_subcategory->_category) {
	_subcategory->_category->_isUsed=b;
      }
    }
  }
}
//----------------------------------------------------------
// getHBankLBankProg
// return the hbank, lbank and prog of the preset
// warning : if there is not subcategory of category
// the value l or h are let unchanged
//----------------------------------------------------------
void Preset::getHBankLBankProg(int* h, int* l, int* p) {
  *p = prog;
  if(_subcategory) {
    *l = _subcategory->_lbank;
    if(_subcategory->_category) *h = _subcategory->_category->_hbank;
  }
}
//----------------------------------------------------------
// linkSubcategory
// links the preset to a subcategory parent and erase itself
// from the last subcategory if not NULL
//----------------------------------------------------------
void Preset::linkSubcategory(Subcategory* sub) {
    if(_subcategory) {
	std::vector<Preset*> pv=_subcategory->_presetVector;
	std::vector<Preset*>::iterator iP=find(pv.begin(),pv.end(),this);
	if(iP!=pv.end()) pv.erase(iP);
	else printf("Error linkSubcategory: preset %s not found\n",
		    name.c_str());
    }
    _subcategory=sub;
    if(sub) sub->_presetVector.push_back(this);
}
//----------------------------------------------------------
// linkCategory
// links the subcategory to a category parent and erase itself
// from the last category if not NULL
//----------------------------------------------------------
void Subcategory::linkCategory(Category* cat) {
    if(_category) {
	std::vector<Subcategory*> sv=_category->_subcategoryVector;
	std::vector<Subcategory*>::iterator iS=find(sv.begin(),sv.end(),this);
	if(iS!=sv.end()) sv.erase(iS);
	else printf("Error linkCategory: preset %s not found\n",
		    _subcategoryName.c_str());
    }
    _category=cat;
    if(cat) cat->_subcategoryVector.push_back(this);
}

	
//----------------------------------------------------------
// linkSet
// links the category to a set parent (there is always only one set)
//----------------------------------------------------------
void Category::linkSet(Set* s) {
    _set=s;
    if(s) s->_categoryVector.push_back(this);
}

//----------------------------------------------------------
// Subcategory constructor and destruction
//----------------------------------------------------------
Subcategory::Subcategory() {_category=NULL;}
Subcategory::Subcategory(Category* cat) {
    _category=cat;
    _isUsed=false;
    if(cat) cat->_subcategoryVector.push_back(this);
}
Subcategory::Subcategory(const std::string name) {
    _category=NULL;
    _isUsed=false;
    _subcategoryName=name;
}
Subcategory::Subcategory(Category* cat, const std::string name, int lbank) {
    _category=cat;
    _isUsed=false;
    _subcategoryName=name;
    _lbank=lbank;
    if(cat) cat->_subcategoryVector.push_back(this);
}
Subcategory::~Subcategory() {
    while(!_presetVector.empty()) delete(*_presetVector.begin());
    if(_category) {
	std::vector<Subcategory*>::iterator
	    iB=_category->_subcategoryVector.begin();
	std::vector<Subcategory*>::iterator
	    iE=_category->_subcategoryVector.end();
	std::vector<Subcategory*>::iterator iS=std::find(iB, iE, this);
	if(iS!=iE) _category->_subcategoryVector.erase(iS);
	else printf("Error : subcategory %s not found\n",
		    _subcategoryName.c_str());
    }
}

//--------------------------------------------------------
// Category constructor destructor
//--------------------------------------------------------
Category::Category() {_set=NULL;_isUsed=false;}
Category::Category(Set* s) {
    _set=s;
    _isUsed=false;
    if(s) s->_categoryVector.push_back(this);
}
Category::Category(Set* s,const std::string name, int hbank) {
    _set=s;
    _isUsed=false;
    _categoryName=name;
    _hbank=hbank;
    if(s) s->_categoryVector.push_back(this);
}
Category::~Category() {
    while(!_subcategoryVector.empty()) delete(*_subcategoryVector.begin());
    if(_set) {
	std::vector<Category*>::iterator iB=_set->_categoryVector.begin();
	std::vector<Category*>::iterator iE=_set->_categoryVector.end();
	std::vector<Category*>::iterator iC=std::find(iB, iE, this);
	if(iC!=iE) _set->_categoryVector.erase(iC);
	else printf("Error : category %s not found\n", _categoryName.c_str());
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
    //globalDetune=0;
    //Name
    name="INITVOICE";
}
//---------------------------------------------------------
// Preset::merge
//   copy the preset
//---------------------------------------------------------
void Preset::merge(Preset* p) {
    if(p) {
	//algorithm
	algorithm=p->algorithm;
	//feedeback
	feedback=p->feedback;
	//lfo
	lfo.wave=p->lfo.wave;
	lfo.speed=p->lfo.speed;
	lfo.delay=p->lfo.delay;
	lfo.pModDepth=p->lfo.pModDepth;
	lfo.aModDepth=p->lfo.aModDepth;
	lfo.sync=p->lfo.sync;
	//sensitivity
	sensitivity.pitch=p->sensitivity.pitch;
	sensitivity.amplitude=p->sensitivity.amplitude;
	for(int k=0; k<NBROP; k++) {
	    sensitivity.ampOn[k]=p->sensitivity.ampOn[k];
	    sensitivity.egBias[k]=p->sensitivity.egBias[k];
	    sensitivity.keyVelocity[k]=p->sensitivity.keyVelocity[k];
	    //frequency
	    frequency[k].ratio=p->frequency[k].ratio;
	    frequency[k].isFix=p->frequency[k].isFix;
	    frequency[k].freq=p->frequency[k].freq;
	    //oscWave
	    oscWave[k]=p->oscWave[k];
	    //detune
	    detune[k]=p->detune[k];
	    //eg
	    eg[k].ar=p->eg[k].ar;
	    eg[k].d1r=p->eg[k].d1r;
	    eg[k].d1l=p->eg[k].d1l;
	    eg[k].d2r=p->eg[k].d2r;
	    eg[k].rr=p->eg[k].rr;
	    eg[k].egShift=p->eg[k].egShift;
	    //outLevel
	    outLevel[k]=p->outLevel[k];
	    //scaling
	    scaling.rate[k]=p->scaling.rate[k];
	    scaling.level[k]=p->scaling.level[k];
	}
	//pitchEg
	pitchEg.pr1=p->pitchEg.pr1;
	pitchEg.pr2=p->pitchEg.pr2;
	pitchEg.pr3=p->pitchEg.pr3;
	pitchEg.pl1=p->pitchEg.pl1;
	pitchEg.pl2=p->pitchEg.pl2;
	pitchEg.pl3=p->pitchEg.pl3;
	//function
	function.transpose=p->function.transpose;
	function.mode=p->function.mode;
	function.pBendRange=p->function.pBendRange;
	function.portamento=p->function.portamento;
	function.portamentoTime=p->function.portamentoTime;
	function.fcVolume=p->function.fcVolume;
	function.fcPitch=p->function.fcPitch;
	function.fcAmplitude=p->function.fcAmplitude;
	function.mwPitch=p->function.mwPitch;
	function.mwAmplitude=p->function.mwAmplitude;
	function.bcPitch=p->function.bcPitch;
	function.bcAmplitude=p->function.bcAmplitude;
	function.bcPitchBias=p->function.bcPitchBias;
	function.bcEgBias=p->function.bcEgBias;
	function.atPitch=p->function.atPitch;
	function.atAmplitude=p->function.atAmplitude;
	function.atPitchBias=p->function.atPitchBias;
	function.atEgBias=p->function.atEgBias;
	function.reverbRate=p->function.reverbRate;
	//globalDetune=p->globalDetune;
	//Name
	name=p->name;
    }
}
//---------------------------------------------------------
// findPreset
//  return the first preset corresponding of hbank, lbank, prog
//---------------------------------------------------------
Preset* Subcategory::findPreset(int prog) {
    std::vector<Preset*>::iterator pvi;
    for(pvi=_presetVector.begin(); pvi!=_presetVector.end(); pvi++) {
	if((*pvi)->prog==prog) return(*pvi);
    }
    return NULL;
}
Preset* Category::findPreset(int lbank, int prog) {
    Subcategory* s=findSubcategory(lbank);
    if(s) {
	Preset* p=s->findPreset(prog);
	if(p) return(p);
    }
    return(NULL);
}
Preset* Set::findPreset(int hbank, int lbank, int prog) {
    Category* c=findCategory(hbank);
    if(c) {
	Preset* p=c->findPreset(lbank, prog);
	if(p) return(p);
    }
    return NULL;
}

//---------------------------------------------------------
// isFreeHBank, firstFreeHBank -1 otherwise
//---------------------------------------------------------
bool Set::isFreeHBank(int hbank) {
    if(findCategory(hbank)) return(false);
    else return(true);
}
int Set::firstFreeHBank() {
    for(int hbank=0; hbank<128; hbank++) if(isFreeHBank(hbank)) return(hbank);
    return(-1);
}
//--------------------------------------------------------------
// Set::merge
//--------------------------------------------------------------
void Set::merge(Category* c) {
    if(isFreeHBank(c->_hbank)) c->linkSet(this);
    else {
	Category* cFromSet=findCategory(c->_hbank);
	cFromSet->_categoryName=c->_categoryName;
	for(std::vector<Subcategory*>::iterator
		i=c->_subcategoryVector.begin();
	    i!=c->_subcategoryVector.end(); i++) cFromSet->merge(*i);
	//delete(c);
    }
}

bool Category::isFreeLBank(int lbank) {
    if(findSubcategory(lbank)) return(false);
    else return(true);
}
int Category::firstFreeLBank() {
    for(int lbank=0; lbank<128; lbank++) if(isFreeLBank(lbank)) return(lbank);
    return(-1);
}
//--------------------------------------------------------------
// Category::merge
//--------------------------------------------------------------
void Category::merge(Subcategory* s) {
    if(isFreeLBank(s->_lbank)) s->linkCategory(this);
    else {
	Subcategory* sFromCat=findSubcategory(s->_lbank);
	sFromCat->_subcategoryName=s->_subcategoryName;
	for(std::vector<Preset*>::iterator
		i=s->_presetVector.begin();
	    i!=s->_presetVector.end(); i++) sFromCat->merge(*i);
	//delete(s);
    }
}
//---------------------------------------------------------
// Category::unlink
//  unlink the subcategories, so don't delete them when delete
//---------------------------------------------------------
void Category::unlink() {
    while(!_subcategoryVector.empty())
	_subcategoryVector.erase(_subcategoryVector.begin());
}

bool Subcategory::isFreeProg(int pr) {
    if(findPreset(pr)) return(false);
    else return(true);
}
int Subcategory::firstFreeProg() {
    for(int pr=0; pr<128; pr++) if(isFreeProg(pr)) return(pr);
    return(-1);
}
//---------------------------------------------------------
// Subcategory::unlink
//  unlink the presets, so don't delete them when delete
//---------------------------------------------------------
void Subcategory::unlink() {
    while(!_presetVector.empty()) _presetVector.erase(_presetVector.begin());
}

//---------------------------------------------------------
// findSubcategory
//  take hbank and lbank and return the subcategory corresponding,
//  NULL if doesn't exist
//---------------------------------------------------------
Subcategory* Set::findSubcategory(int hbank, int lbank) {
  Category* c = findCategory(hbank);
  Subcategory* s;
  if(c) {
    s = c->findSubcategory(lbank);
    return s;
  }
  else return NULL;
}

//---------------------------------------------------------
// findCategory
//  takes hbank a category and return the first category,
//  NULL if doesn't exist
//---------------------------------------------------------
Category* Set::findCategory(int hbank) {
    std::vector<Category*>::iterator cvi;
    for(cvi=_categoryVector.begin(); cvi!=_categoryVector.end(); cvi++)
	if((*cvi)->_hbank==hbank) return(*cvi);
    return(NULL);
}
//---------------------------------------------------------
// findSubcategory
//  takes lbank a subcategory and return the subcategory
//  NULL if doesn't exist
//---------------------------------------------------------
Subcategory* Category::findSubcategory(int lbank) {
    std::vector<Subcategory*>::iterator svi;
    for(svi=_subcategoryVector.begin(); svi!=_subcategoryVector.end(); svi++)
	if((*svi)->_lbank==lbank) return(*svi);
    return(NULL);
}
//--------------------------------------------------------------
// Subcategory::merge
//--------------------------------------------------------------
void Subcategory::merge(Preset* p) {
    if(isFreeProg(p->prog)) p->linkSubcategory(this);
    else {
	Preset* pFromSub=findPreset(p->prog);
	pFromSub->merge(p);
    }
}

//---------------------------------------------------------
// readSet
//---------------------------------------------------------
void Set::readSet(QDomNode setNode) {
    while(!setNode.isNull()) {
	QDomElement setEl = setNode.toElement();
	if (setEl.isNull())
	    continue;
	if (setEl.tagName() == "setName")
	    _setName=setEl.text().toAscii().data();
	if (setEl.tagName() == "deicsOnzeCategory") {
	    //load category
	    QString version = setEl.attribute(QString("version"));
	    if (version == "1.0") {
		Category* lCategory = new Category();
		lCategory->readCategory(setNode.firstChild());
		//printf("Ready to merge!\n");
		merge(lCategory);
	    }
	}
	setNode = setNode.nextSibling();
    }
}

//---------------------------------------------------------
// writeSet
//---------------------------------------------------------
void Set::writeSet(AL::Xml* xml, bool onlyUsed) {
    xml->stag("deicsOnzeSet version=\"1.0\"");
    xml->tag("setName", QString(_setName.c_str()));
    for(std::vector<Category*>::iterator i=_categoryVector.begin();
	i!=_categoryVector.end(); i++) (*i)->writeCategory(xml, onlyUsed);
    xml->etag("deicsOnzeSet");
}

//---------------------------------------------------------
// readCategory
//---------------------------------------------------------
void Category::readCategory(QDomNode catNode) {
    while(!catNode.isNull()) {
	QDomElement catEl = catNode.toElement();
	if (catEl.isNull())
	    continue;
	if (catEl.tagName() == "categoryName")
	    _categoryName=catEl.text().toAscii().data();
	if (catEl.tagName() == "hbank")
	    _hbank=catEl.text().toInt();
	if (catEl.tagName() == "deicsOnzeSubcategory") {
	    //load Subcategory
	    QString version = catEl.attribute(QString("version"));
	    if (version == "1.0") {
		Subcategory* lSubcategory = new Subcategory(this);
		lSubcategory->readSubcategory(catNode.firstChild());
	    }
	}
	catNode = catNode.nextSibling();	
    }
}

//---------------------------------------------------------
// writeCategory
//---------------------------------------------------------
void Category::writeCategory(AL::Xml* xml, bool onlyUsed) {
    if((!onlyUsed || _isUsed)) {
	xml->stag("deicsOnzeCategory version=\"1.0\"");
	xml->tag("categoryName", QString(_categoryName.c_str()));
	xml->tag("hbank", _hbank);
	for(std::vector<Subcategory*>::iterator i=_subcategoryVector.begin();
	    i!=_subcategoryVector.end(); i++)
	    (*i)->writeSubcategory(xml, onlyUsed);
	xml->etag("deicsOnzeCategory");
    }
}

//---------------------------------------------------------
// readSubcategory
//---------------------------------------------------------
void Subcategory::readSubcategory(QDomNode subNode) {
    while(!subNode.isNull()) {
	QDomElement subEl = subNode.toElement();
	if (subEl.isNull())
	    continue;
	if (subEl.tagName() == "subcategoryName")
	    _subcategoryName=subEl.text().toAscii().data();
	if (subEl.tagName() == "lbank")
	    _lbank=subEl.text().toInt();
	if (subEl.tagName() == "deicsOnzePreset") {
	    //load preset
	    QString version = subEl.attribute(QString("version"));
	    if (version == "1.0") {
		Preset* lPreset = new Preset(this);
		lPreset->readPreset(subNode.firstChild());
	    }
	}
	subNode = subNode.nextSibling();	
    }
}

//---------------------------------------------------------
// writeSubcategory
//---------------------------------------------------------
void Subcategory::writeSubcategory(AL::Xml* xml, bool onlyUsed) {
    if((!onlyUsed || _isUsed)) {
	xml->stag("deicsOnzeSubcategory version=\"1.0\"");
	xml->tag("subcategoryName", QString(_subcategoryName.c_str()));
	xml->tag("lbank", _lbank);
	for(std::vector<Preset*>::iterator i=_presetVector.begin();
	    i!=_presetVector.end(); i++) (*i)->writePreset(xml, onlyUsed);
	xml->etag("deicsOnzeSubcategory");
    }
}

//---------------------------------------------------------
// readPreset
//---------------------------------------------------------
void Preset::readPreset(QDomNode presetNode) {
  while(!presetNode.isNull()) {
    QDomElement presetEl = presetNode.toElement();
    if (presetEl.isNull())
      continue;
    //algorithm
    if(presetEl.tagName()==ALGSTR)
      algorithm=(presetEl.text()=="FIRST"? FIRST:
		 (presetEl.text()=="SECOND"? SECOND:
		  (presetEl.text()=="THIRD"? THIRD:
		   (presetEl.text()=="FOURTH"? FOURTH:
		    (presetEl.text()=="FIFTH"? FIFTH:
		     (presetEl.text()=="SIXTH"? SIXTH:
		      (presetEl.text()=="SEVENTH"? SEVENTH:EIGHTH)))))));
    //feedback
    else if(presetEl.tagName()==FEEDBACKSTR)
      feedback=presetEl.text().toInt();
    //quick edit
    //else if(presetEl.tagName()==FINEBRIGHTNESSSTR)
    //  brightness=presetEl.text().toInt();
    //else if(presetEl.tagName()==MODULATIONSTR)
    //  modulation=(unsigned char)presetEl.text().toInt();
    //else if(presetEl.tagName()==GLOBALDETUNESTR)
    //  globalDetune=presetEl.text().toInt();
    //else if(presetEl.tagName()==ATTACKSTR)
    //  attack=presetEl.text().toInt();
    //else if(presetEl.tagName()==RELEASESTR)
    //  release=presetEl.text().toInt();
    //lfo
    else if(presetEl.tagName()=="lfo") {
      QDomNode lfoNode = presetNode.firstChild();
      while(!lfoNode.isNull()) {
	QDomElement lfoEl = lfoNode.toElement();
	if (lfoEl.isNull())
	  continue;
	if(lfoEl.tagName()==WAVESTR)
	  lfo.wave=(lfoEl.text()=="SAWUP"? SAWUP:
		    (lfoEl.text()=="SQUARE"? SQUARE:
		     (lfoEl.text()=="TRIANGL"? TRIANGL:SHOLD)));
	else if(lfoEl.tagName()==SPEEDSTR)
	  lfo.speed=lfoEl.text().toInt();
	else if(lfoEl.tagName()==DELAYSTR)
	  lfo.delay=lfoEl.text().toInt();
	else if(lfoEl.tagName()==PMODDEPTHSTR)
	  lfo.pModDepth=lfoEl.text().toInt();
	else if(lfoEl.tagName()==AMODDEPTHSTR)
	  lfo.aModDepth=lfoEl.text().toInt();
	else if(lfoEl.tagName()==SYNCSTR)
	  lfo.sync=(lfoEl.text()=="on"? true:false);
	lfoNode = lfoNode.nextSibling();
      }
    }
    //sensitivity
    else if(presetEl.tagName()=="sensitivity") {
      QDomNode sensitivityNode = presetNode.firstChild();
      while(!sensitivityNode.isNull()) {
	QDomElement sensitivityEl = sensitivityNode.toElement();
	if (sensitivityEl.isNull())
	  continue;
	QString st=sensitivityEl.tagName();
	if(st==PMODSENSSTR)
	  sensitivity.pitch=sensitivityEl.text().toInt();
	else if(st==AMSSTR)
	  sensitivity.amplitude=sensitivityEl.text().toInt();
	else if(st.contains(AMESTR, Qt::CaseSensitive)) {
	  int op = (st.remove(AMESTR, Qt::CaseSensitive)).toInt()-1;
	  sensitivity.ampOn[op]=(sensitivityEl.text()=="on"? true:false);
	}
	else if(st.contains(EBSSTR, Qt::CaseSensitive)) {
	  int op = (st.remove(EBSSTR, Qt::CaseSensitive)).toInt()-1;
	  sensitivity.egBias[op]=sensitivityEl.text().toInt();
	}
	else if(st.contains(KVSSTR, Qt::CaseSensitive)) {
	  int op = (st.remove(KVSSTR, Qt::CaseSensitive)).toInt()-1;
	  sensitivity.keyVelocity[op]=sensitivityEl.text().toInt();
	}
	sensitivityNode =sensitivityNode.nextSibling();
      }
    }
    //frequency
    else if(presetEl.tagName()=="frequency") {
      QDomNode frequencyNode = presetNode.firstChild();
      while(!frequencyNode.isNull()) {
	QDomElement frequencyEl = frequencyNode.toElement();
	if (frequencyEl.isNull())
	  continue;
	QString ft = frequencyEl.tagName();
	if(ft.contains(RATIOSTR, Qt::CaseSensitive)) {
	  int op = (ft.remove(RATIOSTR, Qt::CaseSensitive)).toInt()-1;
	  frequency[op].ratio=frequencyEl.text().toDouble();
	}
	else if(ft.contains(FIXSTR, Qt::CaseSensitive)) {
	  int op = (ft.remove(FIXSTR, Qt::CaseSensitive)).toInt()-1;
	  frequency[op].isFix=(frequencyEl.text()=="yes"?true:false);
	}
	else if(ft.contains(FIXRANGESTR, Qt::CaseSensitive)) {
	  int op= (ft.remove(FIXRANGESTR, Qt::CaseSensitive)).toInt()-1;
	  frequency[op].freq=frequencyEl.text().toDouble();
	}
	frequencyNode =frequencyNode.nextSibling();
      }
    }
    //oscWave
    else if(presetEl.tagName().contains(OSWSTR, Qt::CaseSensitive)) {
      int op=(presetEl.tagName().remove(OSWSTR, Qt::CaseSensitive)).toInt()-1;
      oscWave[op]=(presetEl.text()=="W1"? W1:
		   (presetEl.text()=="W2"?W2:
		    (presetEl.text()=="W3"?W3:
		     (presetEl.text()=="W4"?W4:
		      (presetEl.text()=="W5"?W5:
		       (presetEl.text()=="W6"?W6:
			(presetEl.text()=="W7"?W7:W8)))))));
      
    }
    //detune
    else if(presetEl.tagName().contains(DETSTR, Qt::CaseSensitive)) {
      int op=(presetEl.tagName().remove(DETSTR, Qt::CaseSensitive)).toInt()-1;
      detune[op]=presetEl.text().toInt();
    }
    //eg
    else if(presetEl.tagName()=="eg") {
      QDomNode egNode = presetNode.firstChild();
      while(!egNode.isNull()) {
	QDomElement egEl = egNode.toElement();
	if (egEl.isNull())
	  continue;
	QString et=egEl.tagName();
	if(et.contains(ARSTR, Qt::CaseSensitive)) {
	  int op=(et.remove(ARSTR, Qt::CaseSensitive)).toInt()-1;
	  eg[op].ar=egEl.text().toInt();
	}
	else if(et.contains(D1RSTR, Qt::CaseSensitive)) {
	  int op=(et.remove(D1RSTR, Qt::CaseSensitive)).toInt()-1;
	  eg[op].d1r=egEl.text().toInt();
	}
	else if(et.contains(D1LSTR, Qt::CaseSensitive)) {
	  int op = (et.remove(D1LSTR, Qt::CaseSensitive)).toInt()-1;
	  eg[op].d1l=egEl.text().toInt();
	}
	else if(et.contains(D2RSTR, Qt::CaseSensitive)) {
	  int op=(et.remove(D2RSTR, Qt::CaseSensitive)).toInt()-1;
	  eg[op].d2r=egEl.text().toInt();
	}
	else if(et.contains(RRSTR, Qt::CaseSensitive)) {
	  int op=(et.remove(RRSTR, Qt::CaseSensitive)).toInt()-1;
	  eg[op].rr=egEl.text().toInt();
	}
	else if(et.contains(SHFTSTR, Qt::CaseSensitive)) {
	  int op=(et.remove(SHFTSTR, Qt::CaseSensitive)).toInt()-1;
	  eg[op].egShift=(egEl.text()=="VOF"?VOF:
			  (egEl.text()=="V48"?V48:
			   (egEl.text()=="V24"?V24:V12)));
	}
	egNode =egNode.nextSibling();
      }
    }	
    //pitchEg
    else if(presetEl.tagName()=="pitchEg") {
      QDomNode pitchEgNode = presetNode.firstChild();
      while(!pitchEgNode.isNull()) {
	QDomElement pitchEgEl = pitchEgNode.toElement();
	if (pitchEgEl.isNull())
	  continue;
	QString pt=pitchEgEl.tagName();
	if(pt==PR1STR) pitchEg.pr1=pitchEgEl.text().toInt();
	else if(pt==PR2STR) pitchEg.pr2=pitchEgEl.text().toInt();
	else if(pt==PR3STR) pitchEg.pr3=pitchEgEl.text().toInt();
	else if(pt==PL1STR) pitchEg.pl1=pitchEgEl.text().toInt();
	else if(pt==PL2STR) pitchEg.pl2=pitchEgEl.text().toInt();
	else if(pt==PL3STR) pitchEg.pl3=pitchEgEl.text().toInt();
	pitchEgNode=pitchEgNode.nextSibling();
      }
    }
    //outLevel
    else if(presetEl.tagName().contains(OUTSTR, Qt::CaseSensitive)) {
      int op=(presetEl.tagName().remove(OUTSTR, Qt::CaseSensitive)).toInt()-1;
      outLevel[op]=presetEl.text().toInt();
    }
    //scaling
    else if(presetEl.tagName()=="scaling") {
      QDomNode scalingNode = presetNode.firstChild();
      while(!scalingNode.isNull()) {
	QDomElement scalingEl = scalingNode.toElement();
	if (scalingEl.isNull())
	  continue;
	QString st=scalingEl.tagName();
	if(st.contains(RSSTR, Qt::CaseSensitive)) {
	  int op=(st.remove(RSSTR, Qt::CaseSensitive)).toInt()-1;
	  scaling.rate[op]=scalingEl.text().toInt();
	}
	else if(st.contains(LSSTR, Qt::CaseSensitive)) {
	  int op=(st.remove(LSSTR, Qt::CaseSensitive)).toInt()-1;
	  scaling.level[op]=scalingEl.text().toInt();
	}
	scalingNode =scalingNode.nextSibling();
      }
    }	
    //function
    else if(presetEl.tagName()=="function") {
      QDomNode functionNode = presetNode.firstChild();
      while(!functionNode.isNull()) {
	QDomElement functionEl = functionNode.toElement();
	if (functionEl.isNull())
	  continue;
	QString ft=functionEl.tagName();
	if(ft==TRANSPOSESTR)
	  function.transpose=functionEl.text().toInt();
	else if(ft==POLYMODESTR)
	  function.mode=(functionEl.text()=="POLY"?POLY:MONO);
	else if(ft==PBENDRANGESTR)
	  function.pBendRange=functionEl.text().toInt();
	else if(ft==PORTAMODESTR)
	  function.portamento=
	    functionEl.text()=="FINGER"?FINGER:FULL;
	else if(ft==PORTATIMESTR)
	  function.portamentoTime=functionEl.text().toInt();
	else if(ft==FSWSTR)
	  function.footSw=(functionEl.text()=="POR"?POR:SUS);
	else if(ft==FCVOLUMESTR)
	  function.fcVolume=functionEl.text().toInt();
	else if(ft==FCPITCHSTR)
	  function.fcPitch=functionEl.text().toInt();
	else if(ft==FCAMPLITUDESTR)
	  function.fcAmplitude=functionEl.text().toInt();
	else if(ft==MWPITCHSTR)
	  function.mwPitch=functionEl.text().toInt();
	else if(ft==MWAMPLITUDESTR)
	  function.mwAmplitude=functionEl.text().toInt();
	else if(ft==BCPITCHSTR)
	  function.bcPitch=functionEl.text().toInt();
	else if(ft==BCAMPLITUDESTR)
	  function.bcAmplitude=functionEl.text().toInt();
	else if(ft==BCPITCHBIASSTR)
	  function.bcPitchBias=functionEl.text().toInt();
	else if(ft==BCEGBIASSTR)
	  function.bcEgBias=functionEl.text().toInt();
	else if(ft==ATPITCHSTR)
	  function.atPitch=functionEl.text().toInt();
	else if(ft==ATAMPLITUDESTR)
	  function.atAmplitude=functionEl.text().toInt();
	else if(ft==ATPITCHBIASSTR)
	  function.atPitchBias=functionEl.text().toInt();
	else if(ft==ATEGBIASSTR)
	  function.atEgBias=functionEl.text().toInt();
	else if(ft==REVERBRATESTR)
	  function.reverbRate=functionEl.text().toInt();
	functionNode=functionNode.nextSibling();
      }
    }
    //globalDetune
    //else if(presetEl.tagName()=="globalDetune")
    //  globalDetune=presetEl.text().toInt();
    //Names
    else if(presetEl.tagName()=="name")
      name=presetEl.text().toAscii().data();
    //prog
    else if(presetEl.tagName()=="prog")
      prog=presetEl.text().toInt();
    presetNode = presetNode.nextSibling();
  }
}

//---------------------------------------------------------
// witePreset
//---------------------------------------------------------
void Preset::writePreset(AL::Xml* xml, bool onlyUsed) {
    char s[MAXCHARTAG];
    if((!onlyUsed || _isUsed)) {
	xml->stag("deicsOnzePreset version=\"1.0\"");
	
	//algorithm
	xml->tag(ALGSTR, QString((algorithm==FIRST? "FIRST":
			     (algorithm==SECOND? "SECOND":
			      (algorithm==THIRD? "THIRD":
			       (algorithm==FOURTH? "FOURTH":
				(algorithm==FIFTH? "FIFTH":
				 (algorithm==SIXTH? "SIXTH":
				  (algorithm==SEVENTH? "SEVENTH":
				   "EIGHTH")))))))));
	//feedback
	xml->tag(FEEDBACKSTR, feedback);
	//quick edit
	//xml->tag(FINEBRIGHTNESSSTR, brightness);
	//xml->tag(MODULATIONSTR, (int)modulation);
	//xml->tag(GLOBALDETUNESTR, globalDetune);	
	//xml->tag(ATTACKSTR, attack);	
	//xml->tag(RELEASESTR, release);
	//lfo
	xml->stag("lfo");
	xml->tag(WAVESTR, QString((lfo.wave==SAWUP? "SAWUP":
			      (lfo.wave==SQUARE? "SQUARE":
			       (lfo.wave==TRIANGL? "TRIANGL":"SHOLD")))));
	xml->tag(SPEEDSTR, lfo.speed);
	xml->tag(DELAYSTR, lfo.delay);
	xml->tag(PMODDEPTHSTR, lfo.pModDepth);
	xml->tag(AMODDEPTHSTR, lfo.aModDepth);
	xml->tag(SYNCSTR, QString((lfo.sync==true? "on":"off")));
	xml->etag("lfo");
	//sensitivity
	xml->stag("sensitivity");
	xml->tag(PMODSENSSTR, sensitivity.pitch);
	xml->tag(AMSSTR, sensitivity.amplitude);
	for(int i=0; i<NBROP; i++) {
	    sprintf(s, AMESTR "%d",i+1);
	    xml->tag(s, QString((sensitivity.ampOn[i]==true? "on":"off")));
	}
	for(int i=0; i<NBROP; i++) {
	    sprintf(s,EBSSTR "%d",i+1);
	    xml->tag(s, sensitivity.egBias[i]);
	}
	for(int i=0; i<NBROP; i++) {
	    sprintf(s, KVSSTR "%d",i+1);
	    xml->tag(s, sensitivity.keyVelocity[i]);
	}
	xml->etag("sensitivity");
	//frequency
	xml->stag("frequency");
	for(int i=0; i<NBROP; i++) {
	  sprintf(s, RATIOSTR "%d",i+1);
	    xml->tag(s, frequency[i].ratio);
	}
	for(int i=0; i<NBROP; i++) {
	    sprintf(s, FIXSTR "%d",i+1);
	    xml->tag(s, QString((frequency[i].isFix==true? "yes":"no")));
	}
	for(int i=0; i<NBROP; i++) {
	    sprintf(s, FIXRANGESTR "%d",i+1);
	    xml->tag(s, frequency[i].freq);
	}
	xml->etag("frequency");
	//oscWave
	for(int i=0; i<NBROP; i++) {
	    sprintf(s, OSWSTR "%d",i+1);
	    xml->tag(s, QString((oscWave[i]==W1?"W1":
			    (oscWave[i]==W2?"W2":
			     (oscWave[i]==W3?"W3":
			      (oscWave[i]==W4?"W4":
			       (oscWave[i]==W5?"W5":
				(oscWave[i]==W6?"W6":
				 (oscWave[i]==W7?"W7":"W8")))))))));
	}
	//detune
	for(int i=0; i<NBROP; i++) {
	    sprintf(s, DETSTR "%d",i+1);
	    xml->tag(s, detune[i]);
	}
	//eg
	xml->stag("eg");
	for(int i=0; i<NBROP; i++) {
	    sprintf(s, ARSTR "%d",i+1);
	    xml->tag(s, eg[i].ar);
	}
	for(int i=0; i<NBROP; i++) {
	    sprintf(s, D1RSTR "%d",i+1);
	    xml->tag(s, eg[i].d1r);
	}
	for(int i=0; i<NBROP; i++) {
	    sprintf(s, D1LSTR "%d",i+1);
	    xml->tag(s, eg[i].d1l);
	}
	for(int i=0; i<NBROP; i++) {
	    sprintf(s, D2RSTR "%d",i+1);
	    xml->tag(s, eg[i].d2r);
	}
	for(int i=0; i<NBROP; i++) {
	    sprintf(s, RRSTR "%d",i+1);
	    xml->tag(s, eg[i].rr);
	}
	for(int i=0; i<NBROP; i++) {
	    sprintf(s, SHFTSTR "%d",i+1);
	    xml->tag(s, QString((eg[i].egShift==VOF?"VOF":
			    (eg[i].egShift==V48?"V48":
			     (eg[i].egShift==V24?"V24":"V12")))));
	}
	xml->etag("eg");
	//pitchEg
	xml->stag("pitchEg");
	xml->tag(PR1STR,pitchEg.pr1);
	xml->tag(PR2STR,pitchEg.pr2);
	xml->tag(PR3STR,pitchEg.pr3);
	xml->tag(PL1STR,pitchEg.pl1);
	xml->tag(PL2STR,pitchEg.pl2);
	xml->tag(PL3STR,pitchEg.pl3);
	xml->etag("pitchEg");
	//outLevel
	for(int i=0; i<NBROP; i++) {
	    sprintf(s, OUTSTR "%d",i+1);
	    xml->tag(s, outLevel[i]);
	}
	//scaling
	xml->stag("scaling");
	for(int i=0; i<NBROP; i++) {
	    sprintf(s, RSSTR "%d",i+1);
	    xml->tag(s, scaling.rate[i]);
	}
	for(int i=0; i<NBROP; i++) {
	    sprintf(s, LSSTR "%d",i+1);
	    xml->tag(s, scaling.level[i]);
	}
	xml->etag("scaling");
	//function
	xml->stag("function");
	xml->tag(TRANSPOSESTR, function.transpose);
	xml->tag(POLYMODESTR, QString((function.mode==POLY? "POLY":"MONO")));
	xml->tag(PBENDRANGESTR, function.pBendRange);
	xml->tag(PORTAMODESTR, QString((function.portamento==FINGER?
	       "FINGER":"FULL")));
	xml->tag(PORTATIMESTR, function.portamentoTime);
	xml->tag(FSWSTR, QString((function.footSw==POR? "POR":"SUS")));
	xml->tag(FCVOLUMESTR, function.fcVolume);
	xml->tag(FCPITCHSTR, function.fcPitch);
	xml->tag(FCAMPLITUDESTR, function.fcAmplitude);
	xml->tag(MWPITCHSTR, function.mwPitch);
	xml->tag(MWAMPLITUDESTR, function.mwAmplitude);
	xml->tag(BCPITCHSTR, function.bcPitch);
	xml->tag(BCAMPLITUDESTR, function.bcAmplitude);
	xml->tag(BCPITCHBIASSTR, function.bcPitchBias);
	xml->tag(BCEGBIASSTR, function.bcEgBias);
	xml->tag(ATPITCHSTR, function.atPitch);
	xml->tag(ATAMPLITUDESTR, function.atAmplitude);
	xml->tag(ATPITCHBIASSTR, function.atPitchBias);
	xml->tag(ATEGBIASSTR, function.atEgBias);
	xml->tag(REVERBRATESTR, function.reverbRate);
	xml->etag("function");
	//globalDetune
	//xml->tag("globalDetune", globalDetune);
	//preset name
	xml->tag("name", QString(name.c_str()));
	//bank prog
	xml->tag("prog",prog);
	
	xml->etag("deicsOnzePreset");
    }
}

//---------------------------------------------------------
// printPreset
//---------------------------------------------------------

void Preset::printPreset()
{
    printf("\n");
    printf("Algorithm : %d, Feedback : %d\n", algorithm, feedback);
    printf("LFO : ");
    switch(lfo.wave)
    {
	case(SAWUP) : printf("SAWUP ,"); break;
	case(SQUARE) : printf("SQUARE ,"); break;
	case(TRIANGL) : printf("TRIANGL ,"); break;
	case(SHOLD) : printf("SHOLD ,"); break;
	default : printf("No defined, "); break;
    }
    printf("Speed : %d, Delay : %d, PModD : %d, AModD : %d, ",
	   lfo.speed, lfo.delay, lfo.pModDepth, lfo.aModDepth);
    if(lfo.sync) printf("Sync\n"); else printf("Not Sync\n");
    printf("LFO Pitch Sensitivity : %d, LFO Amplitude Sensitivity : %d\n",
	   sensitivity.pitch, sensitivity.amplitude);
    for(int i=0; i<NBROP; i++)
    {
	printf("amp%d ",i+1);
	if(sensitivity.ampOn) printf("ON "); else printf("OFF ");
    }
    printf("\n");
    for(int i=0; i<NBROP; i++)
	printf("EgBias%d : %d ",i+1, sensitivity.egBias[i]);
    printf("\n");
    for(int i=0; i<NBROP; i++)
	printf("KVS%d : %d ",i+1, sensitivity.keyVelocity[i]);
    printf("\n");
    for(int i=0; i<NBROP; i++)
    {
	if(frequency[i].isFix)
	    printf("Freq%d : %f ",i+1, frequency[i].ratio);
	else printf("Ratio%d : %f ",i+1, frequency[i].ratio);
    }
    printf("\n");
    for(int i=0; i<NBROP; i++)
    {
	printf("OscWave%d ", i+1);
	switch(oscWave[i])
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
	printf("Detune%d : %d ",i+1, detune[i]);
    printf("\n");
    for(int i=0; i<NBROP; i++)
    {
	printf("AR%d : %d, D1R%d : %d, D1L%d : %d, D2R%d : %d, RR%d : %d, EgShift%d : ",
	       i+1, eg[i].ar, i+1, eg[i].d1r,
	       i+1, eg[i].d1l, i+1, eg[i].d2r, i+1, eg[i].rr, i+1);
	switch(eg[i].egShift)
	{
	    case(VOF) : printf("VOF");
	    case(V48) : printf("48");
	    case(V24) : printf("24");
	    case(V12) : printf("12");
	}
	printf("\n");
    }
    printf("PitchEg pr1 : %d, pr2 : %d, pr3 : %d, pl1 : %d, pl2 : %d, pl3 : %d"
	   , pitchEg.pr1, pitchEg.pr2, pitchEg.pr3,
	   pitchEg.pl1, pitchEg.pl2, pitchEg.pl3);
    printf("\n");
    for(int i=0; i<NBROP; i++)
	printf("OutLevel%d : %d ",i+1, outLevel[i]);
    printf("\n");
    printf("Name : %s\n", name.c_str());
}

void Subcategory::printSubcategory() {
    std::cout << "    " << _subcategoryName << "\n";
    for(std::vector<Preset*>::iterator i=_presetVector.begin();
	i!=_presetVector.end(); i++) (*i)->printPreset();
}

void Category::printCategory() {
    std::cout << "  " << _categoryName << "\n";
    for(unsigned int i=0; i<_subcategoryVector.size(); i++)
	_subcategoryVector[i]->printSubcategory();
}

void Set::printSet() {
    std::cout << _setName << "\n";
    for(unsigned int i=0; i<_categoryVector.size(); i++)
	_categoryVector[i]->printCategory();
}
