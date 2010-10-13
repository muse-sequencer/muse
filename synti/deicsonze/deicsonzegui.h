//===========================================================================
//
//    DeicsOnze an emulator of the YAMAHA DX11 synthesizer
//
//    Version 0.2.2
//
//    deicsonzegui.h
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

#ifndef __DEICSONZEGUI_H
#define __DEICSONZEGUI_H

#include <q3listview.h>
#include <vector>

#include "deicsonzeguibase.h"
#include "libsynti/gui.h"

class DeicsOnze;
class categorySet;
class subcategorySet;
class presetSet;
class Preset;
class QListViewItemCategory;
class QListViewItemSubcategory;
class QListViewItemPreset;
//---------------------------------------------------------
//   DeicsOnzeGui
//---------------------------------------------------------

class DeicsOnzeGui : public DeicsOnzeGuiBase, public MessGui {
    DeicsOnze* _deicsOnze;
    QListViewItemCategory* _currentQLVICategory;
    QListViewItemSubcategory* _currentQLVISubcategory;
    QListViewItemPreset* _currentQLVIPreset;
    
    Q_OBJECT
    QString lastDir;
 private slots:
    void newPresetDialogue();
    void deletePresetDialogue();
    void loadPresetsDialogue();
    void savePresetsDialogue();
    //Preset and bank
    void setName(const QString&);
    void setSubcategory(const QString&);
    void setCategory(const QString&);
    void setBank(int);
    void setProg(int);
   //Global
    void setMasterVol(int);
    void setFeedback(int);
    void setLfoWave(const QString&);
    void setLfoSpeed(int);
    void setLfoDelay(int);
    void setLfoPModDepth(int);
    void setLfoPitchSens(int);
    void setLfoAModDepth(int);
    void setLfoAmpSens(int);
    void setTranspose(int);
    void setGlobalDetune(int);
    void setAlgorithm(const QString&);
    void setPitchBendRange(int);
    //envelope
    void setAR1(int val);
    void setD1R1(int val);
    void setD1L1(int val);
    void setD2R1(int val);
    void setRR1(int val);
    void setAR2(int val);
    void setD1R2(int val);
    void setD1L2(int val);
    void setD2R2(int val);
    void setRR2(int val);
    void setAR3(int val);
    void setD1R3(int val);
    void setD1L3(int val);
    void setD2R3(int val);
    void setRR3(int val);
    void setAR4(int val);
    void setD1R4(int val);
    void setD1L4(int val);
    void setD2R4(int val);
    void setRR4(int val);
    //scaling
    void setLS1(int val);
    void setRS1(int val);
    void setLS2(int val);
    void setRS2(int val);
    void setLS3(int val);
    void setRS3(int val);
    void setLS4(int val);
    void setRS4(int val);
    //vol
    void setVol1(int val);
    void setVol2(int val);
    void setVol3(int val);
    void setVol4(int val);
    //Ratio and Frequency
    void setCoarseRatio1(int val);
    void setFineRatio1(int val);
    void setFreq1(int val);
    void setFix1(bool f);
    void setCoarseRatio2(int val);
    void setFineRatio2(int val);
    void setFreq2(int val);
    void setFix2(bool f);
    void setCoarseRatio3(int val);
    void setFineRatio3(int val);
    void setFreq3(int val);
    void setFix3(bool f);
    void setCoarseRatio4(int val);
    void setFineRatio4(int val);
    void setFreq4(int val);
    void setFix4(bool f);
    //Sensitivity
    void setAME1(bool val);
    void setEBS1(int val);
    void setKVS1(int val);
    void setAME2(bool val);
    void setEBS2(int val);
    void setKVS2(int val);
    void setAME3(bool val);
    void setEBS3(int val);
    void setKVS3(int val);
    void setAME4(bool val);
    void setEBS4(int val);
    void setKVS4(int val);
    //detune
    void setDET1(int val);
    void setDET2(int val);
    void setDET3(int val);
    void setDET4(int val);
    //WaveForm
    void setWaveForm1(const QString&);
    void setWaveForm2(const QString&);
    void setWaveForm3(const QString&);
    void setWaveForm4(const QString&);
    //category subcategory preset
    void setSubcategorySet(Q3ListViewItem*);
    void setPresetSet(Q3ListViewItem*);
    void setPreset(Q3ListViewItem*);

 public:
    void updatePreset(void); //update gui following the current preset
    DeicsOnzeGui(DeicsOnze*);
};

class QListViewItemCategory:public Q3ListViewItem {
 public:
    subcategorySet* _c;
    QListViewItemCategory(Q3ListView* p, QString l, subcategorySet* c)
	:Q3ListViewItem(p, l) {_c=c;};
};

class QListViewItemSubcategory:public Q3ListViewItem {
 public:
    presetSet* _s;
    QListViewItemSubcategory(Q3ListView* p, QString l, presetSet* s)
	:Q3ListViewItem(p, l) {_s=s;};
};

class QListViewItemPreset:public Q3ListViewItem {
 public:
    std::vector<Preset*>::iterator i_p;//presetClass* _p;
    QListViewItemPreset(Q3ListView* pa, QString l, 
			std::vector<Preset*>::iterator ip)
	:Q3ListViewItem(pa, l) {i_p=ip;};
};

#endif /* __DEICSONZEGUI_H */
