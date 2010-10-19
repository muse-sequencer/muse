//===========================================================================
//
//    DeicsOnze an emulator of the YAMAHA DX11 synthesizer
//
//    Version 0.2.2
//
//    deicsonzegui.cpp
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

#include <qvariant.h>
#include <q3groupbox.h>
#include <qslider.h>
#include <qcombobox.h>
#include <qcheckbox.h>
#include <qlineedit.h>
#include <qlcdnumber.h>
#include <q3listview.h>
#include <qspinbox.h>
#include <qlayout.h>
#include <qtooltip.h>
#include <q3whatsthis.h>
#include <qstring.h>
#include <q3filedialog.h>
#include <qmessagebox.h>
#include <qpushbutton.h>
#include <math.h>

#include "newpreset.h"
#include "deicsonzegui.h"
#include "deicsonze.h"

DeicsOnzeGui::DeicsOnzeGui(DeicsOnze* deicsOnze)
    : DeicsOnzeGuiBase(0, "deicsOnzeGui"),
      MessGui()
{
    this->setFixedWidth(470);
    this->setFixedHeight(358);
    _deicsOnze = deicsOnze;
    lastDir= "";
    connect(newPushButton, SIGNAL(clicked()),
	    this, SLOT(newPresetDialogue()));
    connect(deletePushButton, SIGNAL(clicked()),
	    this, SLOT(deletePresetDialogue()));
    connect(loadPushButton, SIGNAL(clicked()),
	    this, SLOT(loadPresetsDialogue()));
    connect(savePushButton, SIGNAL(clicked()),
	    this, SLOT(savePresetsDialogue()));
    //Preset and bank
    connect(nameLineEdit, SIGNAL(textChanged(const QString&)),
	    this, SLOT(setName(const QString&)));
    connect(subcategoryLineEdit, SIGNAL(textChanged(const QString&)),
	    this, SLOT(setSubcategory(const QString&)));
    connect(categoryLineEdit, SIGNAL(textChanged(const QString&)),
	    this, SLOT(setCategory(const QString&)));
    connect(bankSpinBox, SIGNAL(valueChanged(int)), this, SLOT(setBank(int)));
    connect(progSpinBox, SIGNAL(valueChanged(int)), this, SLOT(setProg(int)));
    //Global
    connect(masterVolSlider, SIGNAL(valueChanged(int)),
	    this, SLOT(setMasterVol(int)));
    connect(feedbackSlider, SIGNAL(valueChanged(int)),
	    this, SLOT(setFeedback(int)));
    connect(LFOWaveComboBox, SIGNAL(activated(const QString&)),
	    this, SLOT(setLfoWave(const QString&)));
    connect(LFOSpeedSlider, SIGNAL(valueChanged(int)),
	    this, SLOT(setLfoSpeed(int)));
    connect(LFODelaySlider, SIGNAL(valueChanged(int)),
	    this, SLOT(setLfoDelay(int)));
    connect(PModDepthSlider, SIGNAL(valueChanged(int)),
	    this, SLOT(setLfoPModDepth(int)));   
    connect(PModSensSlider, SIGNAL(valueChanged(int)),
	    this, SLOT(setLfoPitchSens(int)));   
    connect(AModDepthSlider, SIGNAL(valueChanged(int)),
	    this, SLOT(setLfoAModDepth(int)));   
    connect(AModSensSlider, SIGNAL(valueChanged(int)),
	    this, SLOT(setLfoAmpSens(int)));   
    connect(transposeSlider, SIGNAL(valueChanged(int)),
	    this, SLOT(setTranspose(int)));   
    connect(globalDetuneSlider, SIGNAL(valueChanged(int)),
	    this, SLOT(setGlobalDetune(int)));
    connect(algorithmComboBox, SIGNAL(activated(const QString&)),
	    this, SLOT(setAlgorithm(const QString&)));
    connect(PitchBendRangeSlider, SIGNAL(valueChanged(int)),
	    this, SLOT(setPitchBendRange(int)));    
    //envelope
    connect(AR1Slider, SIGNAL(valueChanged(int)), this, SLOT(setAR1(int)));
    connect(D1R1Slider, SIGNAL(valueChanged(int)), this, SLOT(setD1R1(int)));
    connect(D1L1Slider, SIGNAL(valueChanged(int)), this, SLOT(setD1L1(int)));
    connect(D2R1Slider, SIGNAL(valueChanged(int)), this, SLOT(setD2R1(int)));
    connect(RR1Slider, SIGNAL(valueChanged(int)), this, SLOT(setRR1(int)));
    connect(AR2Slider, SIGNAL(valueChanged(int)), this, SLOT(setAR2(int)));
    connect(D1R2Slider, SIGNAL(valueChanged(int)), this, SLOT(setD1R2(int)));
    connect(D1L2Slider, SIGNAL(valueChanged(int)), this, SLOT(setD1L2(int)));
    connect(D2R2Slider, SIGNAL(valueChanged(int)), this, SLOT(setD2R2(int)));
    connect(RR2Slider, SIGNAL(valueChanged(int)), this, SLOT(setRR2(int)));
    connect(AR3Slider, SIGNAL(valueChanged(int)), this, SLOT(setAR3(int)));
    connect(D1R3Slider, SIGNAL(valueChanged(int)), this, SLOT(setD1R3(int)));
    connect(D1L3Slider, SIGNAL(valueChanged(int)), this, SLOT(setD1L3(int)));
    connect(D2R3Slider, SIGNAL(valueChanged(int)), this, SLOT(setD2R3(int)));
    connect(RR3Slider, SIGNAL(valueChanged(int)), this, SLOT(setRR3(int)));
    connect(AR4Slider, SIGNAL(valueChanged(int)), this, SLOT(setAR4(int)));
    connect(D1R4Slider, SIGNAL(valueChanged(int)), this, SLOT(setD1R4(int)));
    connect(D1L4Slider, SIGNAL(valueChanged(int)), this, SLOT(setD1L4(int)));
    connect(D2R4Slider, SIGNAL(valueChanged(int)), this, SLOT(setD2R4(int)));
    connect(RR4Slider, SIGNAL(valueChanged(int)), this, SLOT(setRR4(int)));
    //Scaling
    connect(LS1Slider, SIGNAL(valueChanged(int)), this, SLOT(setLS1(int)));
    connect(RS1Slider, SIGNAL(valueChanged(int)), this, SLOT(setRS1(int)));
    connect(LS2Slider, SIGNAL(valueChanged(int)), this, SLOT(setLS2(int)));
    connect(RS2Slider, SIGNAL(valueChanged(int)), this, SLOT(setRS2(int)));
    connect(LS3Slider, SIGNAL(valueChanged(int)), this, SLOT(setLS3(int)));
    connect(RS3Slider, SIGNAL(valueChanged(int)), this, SLOT(setRS3(int)));
    connect(LS4Slider, SIGNAL(valueChanged(int)), this, SLOT(setLS4(int)));
    connect(RS4Slider, SIGNAL(valueChanged(int)), this, SLOT(setRS4(int)));
    //Volume
    connect(Vol1Slider, SIGNAL(valueChanged(int)), this, SLOT(setVol1(int)));
    connect(Vol2Slider, SIGNAL(valueChanged(int)), this, SLOT(setVol2(int)));
    connect(Vol3Slider, SIGNAL(valueChanged(int)), this, SLOT(setVol3(int)));
    connect(Vol4Slider, SIGNAL(valueChanged(int)), this, SLOT(setVol4(int)));
    //Ratio and Frequency
    connect(CoarseRatio1SpinBox, SIGNAL(valueChanged(int)), 
	    this, SLOT(setCoarseRatio1(int)));
    connect(FineRatio1SpinBox, SIGNAL(valueChanged(int)), 
	    this, SLOT(setFineRatio1(int)));
    connect(Freq1SpinBox, SIGNAL(valueChanged(int)), 
	    this, SLOT(setFreq1(int)));
    connect(Fix1CheckBox, SIGNAL(toggled(bool)), this, SLOT(setFix1(bool)));
    connect(CoarseRatio2SpinBox, SIGNAL(valueChanged(int)), 
	    this, SLOT(setCoarseRatio2(int)));
    connect(FineRatio2SpinBox, SIGNAL(valueChanged(int)), 
	    this, SLOT(setFineRatio2(int)));
    connect(Freq2SpinBox, SIGNAL(valueChanged(int)), 
	    this, SLOT(setFreq2(int)));
    connect(Fix2CheckBox, SIGNAL(toggled(bool)), this, SLOT(setFix2(bool)));
    connect(CoarseRatio3SpinBox, SIGNAL(valueChanged(int)), 
	    this, SLOT(setCoarseRatio3(int)));
    connect(FineRatio3SpinBox, SIGNAL(valueChanged(int)), 
	    this, SLOT(setFineRatio3(int)));
    connect(Freq3SpinBox, SIGNAL(valueChanged(int)), 
	    this, SLOT(setFreq3(int)));
    connect(Fix3CheckBox, SIGNAL(toggled(bool)), this, SLOT(setFix3(bool)));
    connect(CoarseRatio4SpinBox, SIGNAL(valueChanged(int)), 
	    this, SLOT(setCoarseRatio4(int)));
    connect(FineRatio4SpinBox, SIGNAL(valueChanged(int)), 
	    this, SLOT(setFineRatio4(int)));
    connect(Freq4SpinBox, SIGNAL(valueChanged(int)), 
	    this, SLOT(setFreq4(int)));
    connect(Fix4CheckBox, SIGNAL(toggled(bool)), this, SLOT(setFix4(bool)));
    //Sensitivity
    connect(AME1CheckBox, SIGNAL(toggled(bool)), this, SLOT(setAME1(bool)));
    connect(EBS1Slider, SIGNAL(valueChanged(int)), this, SLOT(setEBS1(int)));
    connect(KVS1Slider, SIGNAL(valueChanged(int)), this, SLOT(setKVS1(int)));
    connect(AME2CheckBox, SIGNAL(toggled(bool)), this, SLOT(setAME2(bool)));
    connect(EBS2Slider, SIGNAL(valueChanged(int)), this, SLOT(setEBS2(int)));
    connect(KVS2Slider, SIGNAL(valueChanged(int)), this, SLOT(setKVS2(int)));
    connect(AME3CheckBox, SIGNAL(toggled(bool)), this, SLOT(setAME3(bool)));
    connect(EBS3Slider, SIGNAL(valueChanged(int)), this, SLOT(setEBS3(int)));
    connect(KVS3Slider, SIGNAL(valueChanged(int)), this, SLOT(setKVS3(int)));
    connect(AME4CheckBox, SIGNAL(toggled(bool)), this, SLOT(setAME4(bool)));
    connect(EBS4Slider, SIGNAL(valueChanged(int)), this, SLOT(setEBS4(int)));
    connect(KVS4Slider, SIGNAL(valueChanged(int)), this, SLOT(setKVS4(int)));
    //detune
    connect(DET1Slider, SIGNAL(valueChanged(int)), this, SLOT(setDET1(int)));
    connect(DET2Slider, SIGNAL(valueChanged(int)), this, SLOT(setDET2(int)));
    connect(DET3Slider, SIGNAL(valueChanged(int)), this, SLOT(setDET3(int)));
    connect(DET4Slider, SIGNAL(valueChanged(int)), this, SLOT(setDET4(int)));
    //WaveForm
    connect(WaveForm1ComboBox, SIGNAL(activated(const QString&)),
	    this, SLOT(setWaveForm1(const QString&)));
    connect(WaveForm2ComboBox, SIGNAL(activated(const QString&)),
	    this, SLOT(setWaveForm2(const QString&)));
    connect(WaveForm3ComboBox, SIGNAL(activated(const QString&)),
	    this, SLOT(setWaveForm3(const QString&)));
    connect(WaveForm4ComboBox, SIGNAL(activated(const QString&)),
	    this, SLOT(setWaveForm4(const QString&)));
    //category subcategory preset
    connect(categoryListView, SIGNAL(currentChanged(Q3ListViewItem*)),
	    this, SLOT(setSubcategorySet(Q3ListViewItem*)));
    connect(categoryListView, SIGNAL(clicked(Q3ListViewItem*)),
	    this, SLOT(setSubcategorySet(Q3ListViewItem*)));
    connect(subcategoryListView, SIGNAL(currentChanged(Q3ListViewItem*)),
	    this, SLOT(setPresetSet(Q3ListViewItem*)));
    connect(subcategoryListView, SIGNAL(clicked(Q3ListViewItem*)),
	    this, SLOT(setPresetSet(Q3ListViewItem*)));
    connect(presetsListView, SIGNAL(currentChanged(Q3ListViewItem*)),
	    this, SLOT(setPreset(Q3ListViewItem*)));
    connect(presetsListView, SIGNAL(clicked(Q3ListViewItem*)),
	    this, SLOT(setPreset(Q3ListViewItem*)));


    for(unsigned int i=0; i<_deicsOnze->_categorySet->_categoryVector.size();
	i++) 
	(void) new QListViewItemCategory
	    (categoryListView, _deicsOnze->_categorySet->_categoryVector[i]
	     ->_categoryName.c_str(), _deicsOnze->_categorySet->_categoryVector[i]);

      // work around for probable QT/WM interaction bug.
      // for certain window managers, e.g xfce, this window is
      // is displayed although not specifically set to show();
      // bug: 2811156  	 Softsynth GUI unclosable with XFCE4 (and a few others)
      show();
      hide();
};


//-----------------------------------------------------------
// newPresetsDialogue
//-----------------------------------------------------------
void DeicsOnzeGui::newPresetDialogue() {
    NewPreset* newpreset = new NewPreset(0, "newPreset");
    if(_currentQLVICategory) 
	newpreset->categoryNPLineEdit->setText(_currentQLVICategory
					       ->_c->_categoryName.c_str());
    if(_currentQLVISubcategory) 
	newpreset->subcategoryNPLineEdit->setText(_currentQLVISubcategory
						  ->_s->_subcategoryName.c_str());

    if(QDialog::Accepted==newpreset->exec()) {
	std::string std_s = newpreset->categoryNPLineEdit->text().ascii();
	subcategorySet* subcatS = _deicsOnze->_categorySet
	    ->findSubcategorySet(std_s);
	if(subcatS) {
	    std::string std_s_2 = newpreset->subcategoryNPLineEdit
		->text().ascii();
	    presetSet* preS = subcatS->findPresetSet(std_s_2);
	    if(preS) {
		preS->_presetVector.push_back(new Preset());
		Preset* presetTemp=preS->_presetVector.back();
		
		presetTemp->initPreset();
		presetTemp->name=newpreset->nameNPLineEdit->text().ascii();
		presetTemp->subcategory=
		    newpreset->subcategoryNPLineEdit->text().ascii();
		presetTemp->category=
		    newpreset->categoryNPLineEdit->text().ascii();
		presetTemp->bank=newpreset->bankNPSpinBox->value()-1;
		presetTemp->prog=newpreset->progNPSpinBox->value()-1;
	    }
	}
    }
}
//-----------------------------------------------------------
// deletePresetsDialogue
//-----------------------------------------------------------
void DeicsOnzeGui::deletePresetDialogue() {
    std::vector<Preset*>::iterator i=_currentQLVIPreset->i_p;
    if(!QMessageBox::question(
	   this,
	   tr("Delete preset"),
	   tr("Do you really want to delete %1 ?").arg((*i)->name.c_str()),
	   tr("&Yes"), tr("&No"),
	   QString::null, 0, 1 ))
    {
	//delete(_currentQLVIPreset->_p);
	delete(_currentQLVIPreset);
	_currentQLVISubcategory->_s->_presetVector.erase(i);
    }
}
//-----------------------------------------------------------
// loadPresetsDialogue
//-----------------------------------------------------------
void DeicsOnzeGui::loadPresetsDialogue() {
    QString filename =
	Q3FileDialog::getOpenFileName(lastDir,
				     QString("*.dei"),
				     this,
				     "Load presets dialog","Choose presets");
}
//-----------------------------------------------------------
// savePresetsDialogue
//-----------------------------------------------------------
void DeicsOnzeGui::savePresetsDialogue() {
    QString filename =
	Q3FileDialog::getSaveFileName(lastDir,
				     QString("*.dei"),
				     this,
				     "Save presets dialog","Choose file");
}
//-----------------------------------------------------------
// Preset and bank
//-----------------------------------------------------------
void DeicsOnzeGui::setName(const QString& n) {
    _deicsOnze->_preset->name=n.ascii();
}
void DeicsOnzeGui::setSubcategory(const QString& s) {
    _deicsOnze->_preset->subcategory=s.ascii();
}
void DeicsOnzeGui::setCategory(const QString& c) {
    _deicsOnze->_preset->category=c.ascii();
}
void DeicsOnzeGui::setBank(int b) {_deicsOnze->_preset->bank=b-1;}
void DeicsOnzeGui::setProg(int p) {_deicsOnze->_preset->prog=p-1;}

//-----------------------------------------------------------
// Global controle
//-----------------------------------------------------------
void DeicsOnzeGui::setMasterVol(int mv) {
    _deicsOnze->setMasterVol(mv);
}

void DeicsOnzeGui::setFeedback(int f) {
    _deicsOnze->_preset->feedback=f;
    _deicsOnze->setFeedback();
}

void DeicsOnzeGui::setLfoWave(const QString& qs) {
    _deicsOnze->_preset->lfo.wave=
        //((operator==(qs,"Saw Up")?SAWUP:
	//  (operator==(qs,"Square")?SQUARE:
	//   (operator==(qs,"Triangl")?TRIANGL:SHOLD))));
        ((operator==(qs,QString("Saw Up"))?SAWUP:                   // p4.0.2
          (operator==(qs,QString("Square"))?SQUARE:
           (operator==(qs,QString("Triangl"))?TRIANGL:SHOLD))));
    _deicsOnze->setLfo();
}

void DeicsOnzeGui::setLfoSpeed(int ls) {
    _deicsOnze->_preset->lfo.speed=ls;
    _deicsOnze->setLfo();
}

void DeicsOnzeGui::setLfoDelay(int ld) {
    _deicsOnze->_preset->lfo.delay=ld;
    _deicsOnze->setLfo();
}

void DeicsOnzeGui::setLfoPModDepth(int lpmd) {
    _deicsOnze->_preset->lfo.pModDepth=lpmd;
    _deicsOnze->setLfo();
}

void DeicsOnzeGui::setLfoPitchSens(int lps) {
    _deicsOnze->_preset->sensitivity.pitch=lps;
    _deicsOnze->setLfo();
}

void DeicsOnzeGui::setLfoAModDepth(int lamd) {
    _deicsOnze->_preset->lfo.aModDepth=lamd;
    _deicsOnze->setLfo();
}

void DeicsOnzeGui::setLfoAmpSens(int las) {
    _deicsOnze->_preset->sensitivity.amplitude=las;
    _deicsOnze->setLfo();
}

void DeicsOnzeGui::setTranspose(int t) {
    _deicsOnze->_preset->function.transpose=t;
}

void DeicsOnzeGui::setGlobalDetune(int gd) {
    _deicsOnze->_preset->globalDetune=gd;
}

void DeicsOnzeGui::setAlgorithm(const QString& a) {
    _deicsOnze->_preset->algorithm=
	//((operator==(a,"Algorithm 1")?FIRST:
	//  (operator==(a,"Algorithm 2")?SECOND:
	//   (operator==(a,"Algorithm 3")?THIRD:
	//    (operator==(a, "Algorithm 4")?FOURTH:
	//     (operator==(a, "Algorithm 5")?FIFTH:
	//      (operator==(a, "Algorithm 6")?SIXTH:
	//       (operator==(a, "Algorithm 7")?SEVENTH:EIGHTH))))))));
        ((operator==(a,QString("Algorithm 1"))?FIRST:                              // p4.0.2
          (operator==(a,QString("Algorithm 2"))?SECOND:
           (operator==(a,QString("Algorithm 3"))?THIRD:
            (operator==(a, QString("Algorithm 4"))?FOURTH:
             (operator==(a, QString("Algorithm 5"))?FIFTH:
              (operator==(a, QString("Algorithm 6"))?SIXTH:
               (operator==(a, QString("Algorithm 7"))?SEVENTH:EIGHTH))))))));
}

void DeicsOnzeGui::setPitchBendRange(int pbr) {
    _deicsOnze->_preset->function.pBendRange=pbr;
}

//---------------------------------------------------------------
// envelope controle
//---------------------------------------------------------------
void DeicsOnzeGui::setAR1(int val){_deicsOnze->_preset->eg[0].ar=val;}
void DeicsOnzeGui::setD1R1(int val){_deicsOnze->_preset->eg[0].d1r=val;}
void DeicsOnzeGui::setD1L1(int val){_deicsOnze->_preset->eg[0].d1l=val;}
void DeicsOnzeGui::setD2R1(int val){_deicsOnze->_preset->eg[0].d2r=val;}
void DeicsOnzeGui::setRR1(int val){_deicsOnze->_preset->eg[0].rr=val;}
void DeicsOnzeGui::setAR2(int val){_deicsOnze->_preset->eg[1].ar=val;}
void DeicsOnzeGui::setD1R2(int val){_deicsOnze->_preset->eg[1].d1r=val;}
void DeicsOnzeGui::setD1L2(int val){_deicsOnze->_preset->eg[1].d1l=val;}
void DeicsOnzeGui::setD2R2(int val){_deicsOnze->_preset->eg[1].d2r=val;}
void DeicsOnzeGui::setRR2(int val){_deicsOnze->_preset->eg[1].rr=val;}
void DeicsOnzeGui::setAR3(int val){_deicsOnze->_preset->eg[2].ar=val;}
void DeicsOnzeGui::setD1R3(int val){_deicsOnze->_preset->eg[2].d1r=val;}
void DeicsOnzeGui::setD1L3(int val){_deicsOnze->_preset->eg[2].d1l=val;}
void DeicsOnzeGui::setD2R3(int val){_deicsOnze->_preset->eg[2].d2r=val;}
void DeicsOnzeGui::setRR3(int val){_deicsOnze->_preset->eg[2].rr=val;}
void DeicsOnzeGui::setAR4(int val){_deicsOnze->_preset->eg[3].ar=val;}
void DeicsOnzeGui::setD1R4(int val){_deicsOnze->_preset->eg[3].d1r=val;}
void DeicsOnzeGui::setD1L4(int val){_deicsOnze->_preset->eg[3].d1l=val;}
void DeicsOnzeGui::setD2R4(int val){_deicsOnze->_preset->eg[3].d2r=val;}
void DeicsOnzeGui::setRR4(int val){_deicsOnze->_preset->eg[3].rr=val;}

//--------------------------------------------------------------
// set Scaling
//--------------------------------------------------------------
void DeicsOnzeGui::setLS1(int val){_deicsOnze->_preset->scaling.level[0]=val;}
void DeicsOnzeGui::setRS1(int val){_deicsOnze->_preset->scaling.rate[0]=val;}
void DeicsOnzeGui::setLS2(int val){_deicsOnze->_preset->scaling.level[1]=val;}
void DeicsOnzeGui::setRS2(int val){_deicsOnze->_preset->scaling.rate[1]=val;}
void DeicsOnzeGui::setLS3(int val){_deicsOnze->_preset->scaling.level[2]=val;}
void DeicsOnzeGui::setRS3(int val){_deicsOnze->_preset->scaling.rate[2]=val;}
void DeicsOnzeGui::setLS4(int val){_deicsOnze->_preset->scaling.level[3]=val;}
void DeicsOnzeGui::setRS4(int val){_deicsOnze->_preset->scaling.rate[3]=val;}

//--------------------------------------------------------------
// set Volume
//--------------------------------------------------------------
void DeicsOnzeGui::setVol1(int val){_deicsOnze->_preset->outLevel[0]=val;}
void DeicsOnzeGui::setVol2(int val){_deicsOnze->_preset->outLevel[1]=val;}
void DeicsOnzeGui::setVol3(int val){_deicsOnze->_preset->outLevel[2]=val;}
void DeicsOnzeGui::setVol4(int val){_deicsOnze->_preset->outLevel[3]=val;}

//--------------------------------------------------------------
// set Ratio and Frequency
//--------------------------------------------------------------
void DeicsOnzeGui::setCoarseRatio1(int val) {
    double intf, decf;
    decf=modf(_deicsOnze->_preset->frequency[0].ratio, &intf);
    _deicsOnze->_preset->frequency[0].ratio=(double)val+decf;
}
void DeicsOnzeGui::setFineRatio1(int val) {
    double intf, decf;
    decf=modf(_deicsOnze->_preset->frequency[0].ratio, &intf);
    _deicsOnze->_preset->frequency[0].ratio=intf+0.01*(double)val;
}
void DeicsOnzeGui::setFreq1(int val) {
    _deicsOnze->_preset->frequency[0].freq=(double)val;}
void DeicsOnzeGui::setFix1(bool f) {
    _deicsOnze->_preset->frequency[0].isFix=f;}
void DeicsOnzeGui::setCoarseRatio2(int val) {
    double intf, decf;
    decf=modf(_deicsOnze->_preset->frequency[1].ratio, &intf);
    _deicsOnze->_preset->frequency[1].ratio=(double)val+decf;
}
void DeicsOnzeGui::setFineRatio2(int val) {
    double intf, decf;
    decf=modf(_deicsOnze->_preset->frequency[1].ratio, &intf);
    _deicsOnze->_preset->frequency[1].ratio=intf+0.01*(double)val;
}
void DeicsOnzeGui::setFreq2(int val) {
    _deicsOnze->_preset->frequency[1].freq=(double)val;}
void DeicsOnzeGui::setFix2(bool f) {
    _deicsOnze->_preset->frequency[1].isFix=f;}
void DeicsOnzeGui::setCoarseRatio3(int val) {
    double intf, decf;
    decf=modf(_deicsOnze->_preset->frequency[2].ratio, &intf);
    _deicsOnze->_preset->frequency[2].ratio=(double)val+decf;
}
void DeicsOnzeGui::setFineRatio3(int val) {
    double intf, decf;
    decf=modf(_deicsOnze->_preset->frequency[2].ratio, &intf);
    _deicsOnze->_preset->frequency[2].ratio=intf+0.01*(double)val;
}
void DeicsOnzeGui::setFreq3(int val) {
    _deicsOnze->_preset->frequency[2].freq=(double)val;}
void DeicsOnzeGui::setFix3(bool f) {
    _deicsOnze->_preset->frequency[2].isFix=f;}
void DeicsOnzeGui::setCoarseRatio4(int val) {
    double intf, decf;
    decf=modf(_deicsOnze->_preset->frequency[3].ratio, &intf);
    _deicsOnze->_preset->frequency[3].ratio=(double)val+decf;
}
void DeicsOnzeGui::setFineRatio4(int val) {
    double intf, decf;
    decf=modf(_deicsOnze->_preset->frequency[3].ratio, &intf);
    _deicsOnze->_preset->frequency[3].ratio=intf+0.01*(double)val;
}
void DeicsOnzeGui::setFreq4(int val) {
    _deicsOnze->_preset->frequency[3].freq=(double)val;}
void DeicsOnzeGui::setFix4(bool f) {
    _deicsOnze->_preset->frequency[3].isFix=f;}

//--------------------------------------------------------------
// set Sensitivity
//--------------------------------------------------------------
void DeicsOnzeGui::setAME1(bool val) {
    _deicsOnze->_preset->sensitivity.ampOn[0]=val;}
void DeicsOnzeGui::setEBS1(int val) {
    _deicsOnze->_preset->sensitivity.egBias[0]=val;}
void DeicsOnzeGui::setKVS1(int val) {
    _deicsOnze->_preset->sensitivity.keyVelocity[0]=val;}
void DeicsOnzeGui::setAME2(bool val) {
    _deicsOnze->_preset->sensitivity.ampOn[1]=val;}
void DeicsOnzeGui::setEBS2(int val) {
    _deicsOnze->_preset->sensitivity.egBias[1]=val;}
void DeicsOnzeGui::setKVS2(int val) {
    _deicsOnze->_preset->sensitivity.keyVelocity[1]=val;}
void DeicsOnzeGui::setAME3(bool val) {
    _deicsOnze->_preset->sensitivity.ampOn[2]=val;}
void DeicsOnzeGui::setEBS3(int val) {
    _deicsOnze->_preset->sensitivity.egBias[2]=val;}
void DeicsOnzeGui::setKVS3(int val) {
    _deicsOnze->_preset->sensitivity.keyVelocity[2]=val;}
void DeicsOnzeGui::setAME4(bool val) {
    _deicsOnze->_preset->sensitivity.ampOn[3]=val;}
void DeicsOnzeGui::setEBS4(int val) {
    _deicsOnze->_preset->sensitivity.egBias[3]=val;}
void DeicsOnzeGui::setKVS4(int val) {
    _deicsOnze->_preset->sensitivity.keyVelocity[3]=val;}

//--------------------------------------------------------------
// set detune
//--------------------------------------------------------------
void DeicsOnzeGui::setDET1(int val){_deicsOnze->_preset->detune[0]=val;}
void DeicsOnzeGui::setDET2(int val){_deicsOnze->_preset->detune[1]=val;}
void DeicsOnzeGui::setDET3(int val){_deicsOnze->_preset->detune[2]=val;}
void DeicsOnzeGui::setDET4(int val){_deicsOnze->_preset->detune[3]=val;}

//--------------------------------------------------------------
// set WaveForm
//--------------------------------------------------------------
void DeicsOnzeGui::setWaveForm1(const QString& a) {
    _deicsOnze->_preset->oscWave[0]=
	//((operator==(a,"Wave1")?W1:
	//  (operator==(a,"Wave2")?W2:
	//   (operator==(a,"Wave3")?W3:
	//    (operator==(a, "Wave4")?W4:
	//     (operator==(a, "Wave5")?W5:
	//      (operator==(a, "Wave6")?W6:
	//       (operator==(a, "Wave7")?W7:W8))))))));
        ((operator==(a,QString("Wave1"))?W1:             // p4.0.2  
          (operator==(a,QString("Wave2"))?W2:
           (operator==(a,QString("Wave3"))?W3:
            (operator==(a, QString("Wave4"))?W4:
             (operator==(a, QString("Wave5"))?W5:
              (operator==(a, QString("Wave6"))?W6:
               (operator==(a, QString("Wave7"))?W7:W8))))))));
}
void DeicsOnzeGui::setWaveForm2(const QString& a) {
    _deicsOnze->_preset->oscWave[1]=
	//((operator==(a,"Wave1")?W1:
	//  (operator==(a,"Wave2")?W2:
	//   (operator==(a,"Wave3")?W3:
	//    (operator==(a, "Wave4")?W4:
	//     (operator==(a, "Wave5")?W5:
	//      (operator==(a, "Wave6")?W6:
	//       (operator==(a, "Wave7")?W7:W8))))))));
        ((operator==(a,QString("Wave1"))?W1:                     // p4.0.2
          (operator==(a,QString("Wave2"))?W2:
           (operator==(a,QString("Wave3"))?W3:
            (operator==(a, QString("Wave4"))?W4:
             (operator==(a, QString("Wave5"))?W5:
              (operator==(a, QString("Wave6"))?W6:
               (operator==(a, QString("Wave7"))?W7:W8))))))));
}
void DeicsOnzeGui::setWaveForm3(const QString& a) {
    _deicsOnze->_preset->oscWave[2]=
	//((operator==(a,"Wave1")?W1:
	//  (operator==(a,"Wave2")?W2:
	//   (operator==(a,"Wave3")?W3:
	//    (operator==(a, "Wave4")?W4:
	//     (operator==(a, "Wave5")?W5:
	//      (operator==(a, "Wave6")?W6:
	//       (operator==(a, "Wave7")?W7:W8))))))));
        ((operator==(a,QString("Wave1"))?W1:                         // p4.0.2
          (operator==(a,QString("Wave2"))?W2:
           (operator==(a,QString("Wave3"))?W3:
            (operator==(a, QString("Wave4"))?W4:
             (operator==(a, QString("Wave5"))?W5:
              (operator==(a, QString("Wave6"))?W6:
               (operator==(a, QString("Wave7"))?W7:W8))))))));
}
void DeicsOnzeGui::setWaveForm4(const QString& a) {
    _deicsOnze->_preset->oscWave[3]=
	//((operator==(a,"Wave1")?W1:
	//  (operator==(a,"Wave2")?W2:
	//   (operator==(a,"Wave3")?W3:
	//    (operator==(a, "Wave4")?W4:
	//     (operator==(a, "Wave5")?W5:
	//      (operator==(a, "Wave6")?W6:
	//       (operator==(a, "Wave7")?W7:W8))))))));
        ((operator==(a,QString("Wave1"))?W1:                        // p4.0.2
          (operator==(a,QString("Wave2"))?W2:
           (operator==(a,QString("Wave3"))?W3:
            (operator==(a, QString("Wave4"))?W4:
             (operator==(a, QString("Wave5"))?W5:
              (operator==(a, QString("Wave6"))?W6:
               (operator==(a, QString("Wave7"))?W7:W8))))))));
}

//--------------------------------------------------------------
// setSubcategorySet
//--------------------------------------------------------------
void DeicsOnzeGui::setSubcategorySet(Q3ListViewItem* cat) {
    if(cat) {
	_currentQLVICategory=(QListViewItemCategory*)cat;
	subcategoryListView->clear();
	for(unsigned int i=0;
	    i<((QListViewItemCategory*)cat)->_c->_subcategoryVector.size();i++)
	    (void) new QListViewItemSubcategory(subcategoryListView,
						_currentQLVICategory->_c
						->_subcategoryVector[i]
						->_subcategoryName.c_str(),
						_currentQLVICategory->_c
						->_subcategoryVector[i]);
    }
}

///--------------------------------------------------------------
// setPresetSet
//--------------------------------------------------------------
void DeicsOnzeGui::setPresetSet(Q3ListViewItem* subcat) {
    if(subcat) {
	_currentQLVISubcategory=(QListViewItemSubcategory*)subcat;
	presetsListView->clear();
	for(std::vector<Preset*>::iterator 
		i=_currentQLVISubcategory->_s->_presetVector.begin();
	    i!=_currentQLVISubcategory->_s->_presetVector.end(); i++)
	    (void) new QListViewItemPreset(presetsListView,
					   (*i)->name.c_str(), i);
    }
}

///--------------------------------------------------------------
// setPreset
//--------------------------------------------------------------
void DeicsOnzeGui::setPreset(Q3ListViewItem* pre) {
    if(pre) {
	_currentQLVIPreset=(QListViewItemPreset*)pre;
	std::vector<Preset*>::iterator i=_currentQLVIPreset->i_p;
	_deicsOnze->programSelect(1, (*i)->bank, (*i)->prog);
	updatePreset();
    }
}
//--------------------------------------------------------------
// updatePreset
//  update gui following the current preset
//--------------------------------------------------------------
void DeicsOnzeGui::updatePreset(void) {
    //global
    masterVolSlider->setValue(_deicsOnze->getMasterVol());
    feedbackSlider->setValue(_deicsOnze->_preset->feedback);
    LFOWaveComboBox->setCurrentItem((int)_deicsOnze->_preset->lfo.wave);
    LFOSpeedSlider->setValue(_deicsOnze->_preset->lfo.speed);
    LFODelaySlider->setValue(_deicsOnze->_preset->lfo.delay);
    PModDepthSlider->setValue(_deicsOnze->_preset->lfo.pModDepth);
    PModSensSlider->setValue(_deicsOnze->_preset->sensitivity.pitch);
    AModDepthSlider->setValue(_deicsOnze->_preset->lfo.aModDepth);
    AModSensSlider->setValue(_deicsOnze->_preset->sensitivity.amplitude);
    transposeSlider->setValue(_deicsOnze->_preset->function.transpose);
    algorithmComboBox->setCurrentItem((int)_deicsOnze->_preset->algorithm);
    PitchBendRangeSlider->setValue(_deicsOnze->_preset->function.pBendRange);
    //envelope
    AR1Slider->setValue(_deicsOnze->_preset->eg[0].ar);
    D1R1Slider->setValue(_deicsOnze->_preset->eg[0].d1r);
    D1L1Slider->setValue(_deicsOnze->_preset->eg[0].d1l);
    D2R1Slider->setValue(_deicsOnze->_preset->eg[0].d2r);
    RR1Slider->setValue(_deicsOnze->_preset->eg[0].rr);
    AR2Slider->setValue(_deicsOnze->_preset->eg[1].ar);
    D1R2Slider->setValue(_deicsOnze->_preset->eg[1].d1r);
    D1L2Slider->setValue(_deicsOnze->_preset->eg[1].d1l);
    D2R2Slider->setValue(_deicsOnze->_preset->eg[1].d2r);
    RR2Slider->setValue(_deicsOnze->_preset->eg[1].rr);
    AR3Slider->setValue(_deicsOnze->_preset->eg[2].ar);
    D1R3Slider->setValue(_deicsOnze->_preset->eg[2].d1r);
    D1L3Slider->setValue(_deicsOnze->_preset->eg[2].d1l);
    D2R3Slider->setValue(_deicsOnze->_preset->eg[2].d2r);
    RR3Slider->setValue(_deicsOnze->_preset->eg[2].rr);
    AR4Slider->setValue(_deicsOnze->_preset->eg[3].ar);
    D1R4Slider->setValue(_deicsOnze->_preset->eg[3].d1r);
    D1L4Slider->setValue(_deicsOnze->_preset->eg[3].d1l);
    D2R4Slider->setValue(_deicsOnze->_preset->eg[3].d2r);
    RR4Slider->setValue(_deicsOnze->_preset->eg[3].rr);
    //scaling
    LS1Slider->setValue(_deicsOnze->_preset->scaling.level[0]);
    RS1Slider->setValue(_deicsOnze->_preset->scaling.rate[0]);
    LS2Slider->setValue(_deicsOnze->_preset->scaling.level[1]);
    RS2Slider->setValue(_deicsOnze->_preset->scaling.rate[1]);
    LS3Slider->setValue(_deicsOnze->_preset->scaling.level[2]);
    RS3Slider->setValue(_deicsOnze->_preset->scaling.rate[2]);
    LS4Slider->setValue(_deicsOnze->_preset->scaling.level[3]);
    RS4Slider->setValue(_deicsOnze->_preset->scaling.rate[3]);
    //Volume
    Vol1Slider->setValue(_deicsOnze->_preset->outLevel[0]);
    Vol2Slider->setValue(_deicsOnze->_preset->outLevel[1]);
    Vol3Slider->setValue(_deicsOnze->_preset->outLevel[2]);
    Vol4Slider->setValue(_deicsOnze->_preset->outLevel[3]);
    //Ratio and Frequency
    double intf, decf;
    decf=modf(_deicsOnze->_preset->frequency[0].ratio, &intf);
    CoarseRatio1SpinBox->setValue((int)intf);
    FineRatio1SpinBox->setValue((int)(decf*100.0));
    Freq1SpinBox->setValue((int)_deicsOnze->_preset->frequency[0].freq);
    Fix1CheckBox->setChecked(_deicsOnze->_preset->frequency[0].isFix);
    decf=modf(_deicsOnze->_preset->frequency[1].ratio, &intf);
    CoarseRatio2SpinBox->setValue((int)intf);
    FineRatio2SpinBox->setValue((int)(decf*100.0));
    Freq2SpinBox->setValue((int)_deicsOnze->_preset->frequency[1].freq);
    Fix2CheckBox->setChecked(_deicsOnze->_preset->frequency[1].isFix);
    decf=modf(_deicsOnze->_preset->frequency[2].ratio, &intf);
    CoarseRatio3SpinBox->setValue((int)intf);
    FineRatio3SpinBox->setValue((int)(decf*100.0));
    Freq3SpinBox->setValue((int)_deicsOnze->_preset->frequency[2].freq);
    Fix3CheckBox->setChecked(_deicsOnze->_preset->frequency[2].isFix);
    decf=modf(_deicsOnze->_preset->frequency[3].ratio, &intf);
    CoarseRatio4SpinBox->setValue((int)intf);
    FineRatio4SpinBox->setValue((int)(decf*100.0));
    Freq4SpinBox->setValue((int)_deicsOnze->_preset->frequency[3].freq);
    Fix4CheckBox->setChecked(_deicsOnze->_preset->frequency[3].isFix);
    //Sensitivity
    AME1CheckBox->setChecked(_deicsOnze->_preset->sensitivity.ampOn[0]);
    EBS1Slider->setValue(_deicsOnze->_preset->sensitivity.egBias[0]);
    KVS1Slider->setValue(_deicsOnze->_preset->sensitivity.keyVelocity[0]);
    AME2CheckBox->setChecked(_deicsOnze->_preset->sensitivity.ampOn[1]);
    EBS2Slider->setValue(_deicsOnze->_preset->sensitivity.egBias[1]);
    KVS2Slider->setValue(_deicsOnze->_preset->sensitivity.keyVelocity[1]);
    AME3CheckBox->setChecked(_deicsOnze->_preset->sensitivity.ampOn[2]);
    EBS3Slider->setValue(_deicsOnze->_preset->sensitivity.egBias[2]);
    KVS3Slider->setValue(_deicsOnze->_preset->sensitivity.keyVelocity[2]);
    AME4CheckBox->setChecked(_deicsOnze->_preset->sensitivity.ampOn[3]);
    EBS4Slider->setValue(_deicsOnze->_preset->sensitivity.egBias[3]);
    KVS4Slider->setValue(_deicsOnze->_preset->sensitivity.keyVelocity[3]);
    //detune
    DET1Slider->setValue(_deicsOnze->_preset->detune[0]);
    DET2Slider->setValue(_deicsOnze->_preset->detune[1]);
    DET3Slider->setValue(_deicsOnze->_preset->detune[2]);
    DET4Slider->setValue(_deicsOnze->_preset->detune[3]);
    //Waveform
    WaveForm1ComboBox->setCurrentItem((int)_deicsOnze->_preset->oscWave[0]);
    WaveForm2ComboBox->setCurrentItem((int)_deicsOnze->_preset->oscWave[1]);
    WaveForm3ComboBox->setCurrentItem((int)_deicsOnze->_preset->oscWave[2]);
    WaveForm4ComboBox->setCurrentItem((int)_deicsOnze->_preset->oscWave[3]);
    //name, subcategory, category
    nameLineEdit->setText(QString(_deicsOnze->_preset->name.c_str()));
    subcategoryLineEdit->setText(QString(_deicsOnze->_preset->subcategory.c_str()));
    categoryLineEdit->setText(QString(_deicsOnze->_preset->category.c_str()));
    //bank n prog
    bankSpinBox->setValue(_deicsOnze->_preset->bank+1);
    progSpinBox->setValue(_deicsOnze->_preset->prog+1);
}

