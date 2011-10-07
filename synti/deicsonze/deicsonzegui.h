//===========================================================================
//
//    DeicsOnze an emulator of the YAMAHA DX11 synthesizer
//
//    Version 0.5.5
//
//    deicsonzegui.h
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

#ifndef __DEICSONZEGUI_H
#define __DEICSONZEGUI_H

#include "awl/slider.h"
using Awl::Slider;

#include "deicsonze.h"

#include "deicsonzepreset.h"
#include "ui_deicsonzegui.h"
#include "libsynti/gui.h"
#include "awl/floatentry.h"
///#include "awl/slider.h"
#include "awl/checkbox.h"

using Awl::FloatEntry;
///using Awl::Slider;
using Awl::CheckBox;

//Envelope Gui constants
#define XOFFSET 2
#define YOFFSET 2
#define PENWIDTH 2
#define DOTWIDTH 3
#define DRAGWIDTH 6 //size of the mousetracking threshold
//pitch envelope constants
#define WALLWIDTH 6
#define PR1WIDTH (width()/3-9)
#define PR2WIDTH PR1WIDTH
#define PR3WIDTH PR1WIDTH
#define PL1HEIGHT (height()-4)
#define PL2HEIGHT PL1HEIGHT
#define PL3HEIGHT PL1HEIGHT
#define MAXPWIDTH PR1WIDTH+WALLWIDTH+PR2WIDTH+WALLWIDTH+PR3WIDTH+WALLWIDTH+PR1WIDTH
#define MAXPHEIGHT PL1HEIGHT
#define STEPVALUE 10
//amplitude envelope constants
#define ARWIDTH (width()/4-1)
#define D1RWIDTH ARWIDTH
#define D1LHEIGHT (height()-2)
#define D2RWIDTH ARWIDTH
#define RRWIDTH ARWIDTH
#define MAXWIDTH ARWIDTH+D1RWIDTH+D1RWIDTH+RRWIDTH
#define MAXHEIGHT D1LHEIGHT

//COLOR
#define TCOLOR QColor(0, 0, 0) //text color
#define BCOLOR QColor(210, 180, 90) //background color
#define ETCOLOR QColor(0, 150, 0) //edit text color
#define EBCOLOR QColor(255, 255, 30) //edit background color

class DeicsOnze;
class QFramePitchEnvelope;
class QFrameEnvelope;

class QTreeCategory:public QTreeWidgetItem {
 public:
    Category* _category;
    QTreeCategory(QTreeWidget* p, QString shbank, QString l, Category* c)
	:QTreeWidgetItem(p) {
      	setText(0, shbank);
      	setText(1, l);
      	_category=c;
      	};
};

class QTreeSubcategory:public QTreeWidgetItem {
 public:
  Subcategory* _subcategory;
  QTreeSubcategory(QTreeWidget* p, QString slbank,
		   QString l, Subcategory* s)
    :QTreeWidgetItem(p) {
    setText(0, slbank);
    setText(1, l);
    _subcategory=s;
  };
};

class QTreePreset:public QTreeWidgetItem {
 public:
    Preset* _preset;
    QTreePreset(QTreeWidget* pa, QString sprog, QString l, Preset* p)
	:QTreeWidgetItem(pa) {
            setText(0, sprog);
            setText(1, l);
           _preset=p;
            };
};

//---------------------------------------------------------
//   DeicsOnzeGui
//---------------------------------------------------------
class DeicsOnzeGui : public QDialog, public Ui::DeicsOnzeGuiBase, public MessGui {
    Q_OBJECT
    
    bool _enabledPreset;

    QFramePitchEnvelope* pitchEnvelopeGraph;
    QFrameEnvelope* envelopeGraph[NBROP];

    QWidget* _chorusSuperWidget;
    QWidget* _reverbSuperWidget;
    std::vector<Slider*> _chorusSliderVector;
    std::vector<FloatEntry*> _chorusFloatEntryVector;
    std::vector<CheckBox*> _chorusCheckBoxVector;
    std::vector<Slider*> _reverbSliderVector;
    std::vector<FloatEntry*> _reverbFloatEntryVector;
    std::vector<CheckBox*> _reverbCheckBoxVector;

    
    QString lastDir;
  private slots:
    void readMessage(int);
    void setEnabledChannel(bool);
    void setChangeChannel(int);
    void setPanic();
    void setResCtrl();
    void setNbrVoices(int);
    void setSaveOnlyUsed(bool);
    void setSaveOnlyUsedComp(bool);
    void setSaveConfig(bool);
//    void setMidiInCh(int); //to change
    void setQuality(const QString&);
    void setFilter(bool);
    void setFontSize(int);
    void saveConfiguration();
    void saveDefaultConfiguration();
    void loadConfiguration();
    void loadConfiguration(QString s);
    //load init set
    void setIsInitSet(bool);
    void setInitSetPath(const QString&);
    void setBrowseInitSetPath();
    //load init set
    void setIsBackgroundPix(bool);
    void setBackgroundPixPath(const QString&);
    void setBrowseBackgroundPixPath();
    //FX
    void setChorusActiv(bool a); 
    void setChannelChorus(int c);
    void setChorusReturn(int al);
    void setSelectChorusPlugin();
    void setReverbCheckBox(double v, int i);
    void setChorusCheckBox(double v, int i);
    void setReverbActiv(bool a);
    void setChannelReverb(int r);
    void setReverbReturn(int val);
    void setSelectReverbPlugin();
    void setReverbFloatEntry(double v, int i);
    void setReverbSlider(double v, int i);
    void setChorusFloatEntry(double v, int i);
    void setChorusSlider(double v, int i);
    //quick edit
    void setChannelVolKnob(double val);
    void setChannelPan(double val);
    void setBrightnessKnob(double val);
    void setModulationKnob(double val);
    void setDetuneKnob(double val);
    void setAttackKnob(double val);
    void setReleaseKnob(double val);
    //Color
    void setRedColor(int);
    void setGreenColor(int);
    void setBlueColor(int);
    void setRGBSliders(QListWidgetItem*);
    void setTextColor(const QColor &);
    void setBackgroundColor(const QColor &);
    void setEditTextColor(const QColor &);
    void setEditBackgroundColor(const QColor &);
    //New Delete Load Save
    void deleteSetDialog();
    void loadSetDialog();
    void saveSetDialog();
    void deleteCategoryDialog();
    void newCategoryDialog();
    void loadCategoryDialog();
    void saveCategoryDialog();
    void deleteSubcategoryDialog();
    void newSubcategoryDialog();
    void loadSubcategoryDialog();
    void saveSubcategoryDialog();
    void newPresetDialog();
    void loadPresetDialog();
    void deletePresetDialog();
    void savePresetDialog();
    //popupMenu
    void categoryPopupMenu(const QPoint&);
    void subcategoryPopupMenu(const QPoint&);
    void presetPopupMenu(const QPoint&);
    //Preset and bank
    void setPresetName(const QString&);
    void setSubcategoryName(const QString&);
    void setCategoryName(const QString&);
    void setHBank(int);
    void setLBank(int);
    void setProg(int);
   //Global
    void setMasterVolKnob(double);
    void setMasterVol(int);
    void setFeedback(int);
    void setLfoWave(int);
    void setLfoSpeed(int);
    void setLfoDelay(int);
    void setLfoPModDepth(int);
    void setLfoPitchSens(int);
    void setLfoAModDepth(int);
    void setLfoAmpSens(int);
    void setTranspose(int);
    void setChannelDetune(int);
    void setAlgorithm(int);
    void setPitchBendRange(int);
    //Pitch Envelope
    void setPL1(int);
    void setPL2(int);
    void setPL3(int);
    void setPR1(int);
    void setPR2(int);
    void setPR3(int);
    //Function
    void setFcVolume(int);
    void setFcPitch(int);
    void setFcAmplitude(int);
    void setMwPitch(int);
    void setMwAmplitude(int);
    void setBcPitch(int);
    void setBcAmplitude(int);
    void setBcPitchBias(int);
    void setBcEgBias(int);
    void setAtPitch(int);
    void setAtAmplitude(int);
    void setAtPitchBias(int);
    void setAtEgBias(int);
    void setReverbRate(int);
    void setPolyMode(int);
    void setPortFingerFull(int);
    void setPortaTime(int);
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
    void setWaveForm1(int);
    void setWaveForm2(int);
    void setWaveForm3(int);
    void setWaveForm4(int);
    //Delay
    void setActivDelay(bool);
    void setDelayReturn(int);
    void setChannelDelay(int);
    //void setDelayTime(int);
    void setDelayBPM(double);
    void setDelayBeatRatio(double);
    //void setDelayFeedback(int);
    void setDelayFeedback(double);
    //void setDelayPanLFOFreq(int);
    void setDelayPanLFOFreq(double);
    //void setDelayPanLFODepth(int);
    void setDelayPanLFODepth(double);
    //category subcategory preset
    void setSet(void); //display the set, that is the category list
    void setCategory(Category*);
    void setCategory(QTreeWidgetItem*);
    void setSubcategory(Subcategory*);
    void setSubcategory(QTreeWidgetItem*);
    void setPreset(QTreeWidgetItem*);
 public:
    virtual void processEvent(const MusECore::MidiPlayEvent&);
    void updateSelectPreset(int hbank, int lbank, int prog);
    //update the gui
    void setEnabledPreset(bool b);
    void updateChannelCheckBox(bool b);
    void updateEnabledChannel(bool e);//put enabled the display concerning channel and preset
    void updateChannelEnable(bool e);//update channel enable
    void updateMasterVolume(int val);
    void updateNbrVoices(int val);
    //void updateMidiInCh(int val); //to change
    void updateQuality(int val);
    void updateFilter(bool f);
    void updateFontSize(int fs);
    void applyFontSize(int fs);
    void updateSaveOnlyUsed(bool);
    void updateSaveConfig(bool);
    //FX
    void updateChorusActiv(bool a);
    void updateChannelChorus(int c);
    void updateChorusReturn(int r);
    void updateReverbActiv(bool a);
    void updateChannelReverb(int r);
    void updateReverbReturn(int r);
    void updateLadspaReverbLineEdit(QString s);
    void updateLadspaChorusLineEdit(QString s);
    void updateDelayActiv(bool a);
    void updateChannelDelay(int r);
    void updateDelayReturn(int r);
    void updateDelayPanLFOFreq(float plf);
    void updateDelayBPM(float dt);
    void updateDelayBeatRatio(float dt);
    void updateDelayFeedback(float df);
    void updateDelayPanLFODepth(float dpd);
    void addPluginCheckBox(int index, QString text, bool toggled,
			   QWidget* parent, QGridLayout* grid, bool isReverb);
    void addPluginIntSlider(int index, QString text, double min, double max,
			    double val, QWidget* parent, QGridLayout* grid,
			    bool isReverb);
    void addPluginSlider(int index, QString text, bool isLog, double min,
			 double max, double val, QWidget* parent,
			 QGridLayout* grid, bool isReverb);
    void buildGuiReverb();
    void buildGuiChorus();
    void updateReverbSlider(double v, int i);
    void updateReverbFloatEntry(double v, int i);
    void updateChorusSlider(double v, int i);
    void updateChorusFloatEntry(double v, int i);
    //update load init set
    void updateInitSetCheckBox(bool);
    void updateInitSetPath(QString);
    //update background pix
    void updateBackgroundPixCheckBox(bool);
    void updateBackgroundPixPath(QString);
    void applyBackgroundPix();
    //update quick edit
    void updateChannelPan(int val);
    void updateBrightness(int val);
    void updateModulation(int val);
    void updateAttack(int val);
    void updateRelease(int val);
    void updateQuickEdit();
    //update pitch envelope
    void updatePL1(int val);
    void updatePL2(int val);
    void updatePL3(int val);
    void updatePR1(int val);
    void updatePR2(int val);
    void updatePR3(int val);
    //update function
    void updateFcVolume(int val);
    void updateFcPitch(int val);
    void updateFcAmplitude(int val);
    void updateMwPitch(int val);
    void updateMwAmplitude(int val);
    void updateBcPitch(int val);
    void updateBcAmplitude(int val);
    void updateBcPitchBias(int val);
    void updateBcEgBias(int val);
    void updateAtPitch(int val);
    void updateAtAmplitude(int val);
    void updateAtPitchBias(int val);
    void updateAtEgBias(int val);
    //void updateReverbRate(int val);
    //update envelope
    void updateAR(int op, int val);
    void updateD1R(int op, int val);
    void updateD2R(int op, int val);
    void updateRR(int op, int val);
    void updateD1L(int op, int val);
    //update scale
    void updateLS(int op, int val);
    void updateRS(int op, int val);
    void updateEBS(int op, int val);
    void updateAME(int op, bool val);
    void updateKVS(int op, int val);
    void updateOUT(int op, int val);
    void updateRATIO(int op, int val);
    void updateDET(int op, int val);
    //update global
    void updateALG(int val);
    void updateFEEDBACK(int val);
    void updateSPEED(int val);
    void updateDELAY(int val);
    void updatePMODDEPTH(int val);
    void updateAMODDEPTH(int val);
    void updateSYNC(bool val);
    void updateWAVE(int val);
    void updatePMODSENS(int val);
    void updateAMS(int val);
    void updateTRANSPOSE(int val);
    void updatePOLYMODE(int val);
    void updatePBENDRANGE(int val);
    void updatePORTAMODE(int val);
    void updatePORTATIME(int val);
    void updateFIX(int op, bool val);
    void updateFIXRANGE(int op, int val);
    void updateOSW(int op, int val);
    void updateSHFT(int op, int val);
    void updateChannelDetune(int val);
    void updateChannelDetuneKnob(int val);
    //void updateChannelDetuneSlider(int val);
    void updateChannelVolume(int val);
    void updateCategoryName(QString cn, bool enable);
    void updateSubcategoryName(QString sn, bool enable);
    void updatePresetName(QString pn, bool enable);
    void updatePresetName(QString pn);
    void updateHBank(int hbank, bool enable);
    void updateLBank(int lbank, bool enable);
    void updateProg(int prog, bool enable);
    void updatePreset(Preset* p);
    void updatePreset(void); //update gui following the current preset
    //void updateCurrentChannel(); //update gui channel attributes
    QString num3Digits(int);
    DeicsOnzeGui(DeicsOnze*);

    int _currentChannel;

    QColor* tColor; //text color
    QColor* bColor; //background color
    QColor* etColor;//edit text color
    QColor* ebColor;//edit background color
    QColor* curColor;//current color

    DeicsOnze* _deicsOnze;
};

class QFramePitchEnvelope:private QFrame {
  QPoint startlinkP1, //first point
    P1linkP2, //point linking P1 to P2
    P2linkP3, //point linking P2 to P3
    P3linkEnd; //point linking P3 to End
  bool isStartlinkP1Edit;
  bool isP1linkP2Edit;
  bool isP2linkP3Edit;
  bool isP3linkEndEdit;
 public:
  DeicsOnzeGui* _deicsOnzeGui;
  QFramePitchEnvelope(QWidget* parent, DeicsOnzeGui* dog):QFrame(parent){
    _deicsOnzeGui = dog;
    isStartlinkP1Edit=false;
    isP1linkP2Edit=false;
    isP2linkP3Edit=false;
    isP3linkEndEdit=false;
  };
  void env2Points(int pl1, int pl2, int pl3, int pr1, int pr2, int pr3);
  void updateEnv(void) {update();};
 protected:
  virtual void paintEvent(QPaintEvent* e);
  virtual void mouseMoveEvent(QMouseEvent* e);
  virtual void mousePressEvent(QMouseEvent * e);
  virtual void mouseReleaseEvent(QMouseEvent * e);
};

class QFrameEnvelope:private QFrame {
  unsigned char op; //operator number, between 0 and 3
  QPoint startlinkAR, //first point
    ARlinkD1, //point linking AR to D1
    D1linkD2, //point linking D1 to D2
    D2linkRR, //point linking D2 to RR
    RRlinkEnd; //last point
  bool isARlinkD1Edit;
  bool isD1linkD2Edit;
  bool isD2linkRREdit;
  bool isRRlinkEndEdit;
 public:
  DeicsOnzeGui* _deicsOnzeGui;
  QFrameEnvelope(QWidget* parent, DeicsOnzeGui* dog, unsigned char k):QFrame(parent){
    _deicsOnzeGui = dog;
    isARlinkD1Edit=false;
    isD1linkD2Edit=false;
    isD2linkRREdit=false;
    isRRlinkEndEdit=false;
    op = k;
    //setGeometry(XOFFSET, YOFFSET, XOFFSET+MAXWIDTH, MAXHEIGHT);
    //setMouseTracking(true);
  };
  void env2Points(int ar, int d1r, int d1l, int d2r, int rr);
  void updateEnv(void) {update();};
 protected:
  void paintEvent(QPaintEvent* e);
  void mouseMoveEvent(QMouseEvent* e);
  void mousePressEvent(QMouseEvent * e);
  void mouseReleaseEvent(QMouseEvent * e);
};

#endif /* __DEICSONZEGUI_H */
