//===========================================================================
//
//    DeicsOnze an emulator of the YAMAHA DX11 synthesizer
//
//    Version 0.5.5
//
//    deicsonzegui.cpp
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

#include <QDir>
#include <QDomDocument>
#include <QFileDialog>
#include <QMenu>
#include <QMessageBox>
#include <QMouseEvent>
#include <QPainter>
#include <QSocketNotifier>

#include "muse/midi.h"
#include "muse/midictrl.h"
#include "config.h"

#include "common_defs.h"
#include "deicsonzegui.h"

#include "plugin.h"
///#include "plugingui.h"
#include "plugins/pandelay/pandelaymodel.h"

//#include "deicsonzegui.h"

namespace MusEGui {
class PluginDialog;
}

DeicsOnzeGui::DeicsOnzeGui(DeicsOnze* deicsOnze)
    : QDialog(0),
      MessGui()
{
  setupUi(this);
  _deicsOnze = deicsOnze;
  lastDir = QDir::currentPath();

  _currentChannel = 0;

  //FX
  _chorusSuperWidget = NULL;
  _reverbSuperWidget = NULL;

  tColor = new TCOLOR;
  bColor = new BCOLOR;
  etColor = new ETCOLOR;
  ebColor = new EBCOLOR;
  curColor = new QColor(0, 0, 0);

  pitchEnvelopeGraph = new QFramePitchEnvelope(pitchEnvFrame, this);

  envelopeGraph[0] = new QFrameEnvelope(envelope1Frame, this, 0);
  envelopeGraph[1] = new QFrameEnvelope(envelope2Frame, this, 1);
  envelopeGraph[2] = new QFrameEnvelope(envelope3Frame, this, 2);
  envelopeGraph[3] = new QFrameEnvelope(envelope4Frame, this, 3);

  //change/enable channel
  connect(ChannelCheckBox, SIGNAL(toggled(bool)), this,
	  SLOT(setEnabledChannel(bool)));
  connect(ChannelNumSpinBox, SIGNAL(valueChanged(int)), this,
	  SLOT(setChangeChannel(int)));
  //MasterVolume
  connect(masterVolKnob, SIGNAL(valueChanged(double, int)),
	  this, SLOT(setMasterVolKnob(double)));
  //Panic
  connect(panicButton, SIGNAL(pressed()), this, SLOT(setPanic()));
  //reset Ctrls
  connect(resCtrlButton, SIGNAL(pressed()), this, SLOT(setResCtrl()));
  //FX
  connect(chorusActivCheckBox, SIGNAL(toggled(bool)),
	  this, SLOT(setChorusActiv(bool)));
  connect(chChorusSlider, SIGNAL(valueChanged(int)),
	  this, SLOT(setChannelChorus(int)));
  connect(returnChorusSlider, SIGNAL(valueChanged(int)),
	  this, SLOT(setChorusReturn(int)));
  connect(selectLadspaChorusPushButton, SIGNAL(pressed()),
	  this, SLOT(setSelectChorusPlugin()));
  /*connect(panChorus1Knob, SIGNAL(valueChanged(double, int)),
    this, SLOT(setPanChorus1(double)));
  connect(LFOFreqChorus1Knob, SIGNAL(valueChanged(double, int)),
	  this, SLOT(setLFOFreqChorus1(double)));
  connect(depthChorus1Knob, SIGNAL(valueChanged(double, int)),
	  this, SLOT(setDepthChorus1(double)));
  connect(panChorus2Knob, SIGNAL(valueChanged(double, int)),
	  this, SLOT(setPanChorus2(double)));
  connect(LFOFreqChorus2Knob, SIGNAL(valueChanged(double, int)),
	  this, SLOT(setLFOFreqChorus2(double)));
  connect(depthChorus2Knob, SIGNAL(valueChanged(double, int)),
  this, SLOT(setDepthChorus2(double)));*/
  connect(reverbActivCheckBox, SIGNAL(toggled(bool)),
	  this, SLOT(setReverbActiv(bool)));
  connect(chReverbSlider, SIGNAL(valueChanged(int)),
	  this, SLOT(setChannelReverb(int)));
  connect(returnReverbSlider, SIGNAL(valueChanged(int)),
	  this, SLOT(setReverbReturn(int)));
  connect(selectLadspaReverbPushButton, SIGNAL(pressed()),
	  this, SLOT(setSelectReverbPlugin()));
  //Quick edit
  connect(channelVolumeKnob, SIGNAL(valueChanged(double, int)),
	  this, SLOT(setChannelVolKnob(double)));
  connect(channelPanKnob, SIGNAL(valueChanged(double, int)),
	  this, SLOT(setChannelPan(double)));
  connect(brightnessKnob, SIGNAL(valueChanged(double, int)),
	  this, SLOT(setBrightnessKnob(double)));
  connect(modulationKnob, SIGNAL(valueChanged(double, int)),
	  this, SLOT(setModulationKnob(double)));
  connect(detuneKnob, SIGNAL(valueChanged(double, int)),
	  this, SLOT(setDetuneKnob(double)));
  connect(attackKnob, SIGNAL(valueChanged(double, int)),
	  this, SLOT(setAttackKnob(double)));
  connect(releaseKnob, SIGNAL(valueChanged(double, int)),
	  this, SLOT(setReleaseKnob(double)));
  //nbr of voices
  connect(nbrVoicesSpinBox, SIGNAL(valueChanged(int)), 
	  this, SLOT(setNbrVoices(int)));
  //quality
  connect(qualityComboBox, SIGNAL(activated(const QString&)),
	  this, SLOT(setQuality(const QString&)));
  connect(filterCheckBox, SIGNAL(toggled(bool)),
	  this, SLOT(setFilter(bool)));
  //change font size
  connect(fontSizeSpinBox, SIGNAL(valueChanged(int)), 
	  this, SLOT(setFontSize(int)));
  //load save configuration
  connect(saveConfPushButton, SIGNAL(pressed()),
	  this, SLOT(saveConfiguration()));
  connect(loadConfPushButton, SIGNAL(pressed()),
	  this, SLOT(loadConfiguration()));
  connect(saveDefaultPushButton, SIGNAL(pressed()),
	  this, SLOT(saveDefaultConfiguration()));
  //load init set
  connect(initSetCheckBox, SIGNAL(toggled(bool)),
	  this, SLOT(setIsInitSet(bool)));
  connect(initSetPathLineEdit, SIGNAL(textChanged(const QString&)),
	  this, SLOT(setInitSetPath(const QString&)));
  connect(initSetBrowsePushButton, SIGNAL(pressed()),
	  this, SLOT(setBrowseInitSetPath()));
  //load background pix
  connect(imageCheckBox, SIGNAL(toggled(bool)),
	  this, SLOT(setIsBackgroundPix(bool)));
  connect(imagePathLineEdit, SIGNAL(textChanged(const QString&)),
	  this, SLOT(setBackgroundPixPath(const QString&)));
  connect(imageBrowsePushButton, SIGNAL(pressed()),
	  this, SLOT(setBrowseBackgroundPixPath()));

  //Midi in channel
  //connect(MidiInChComboBox, SIGNAL(activated(int)),
  //  this, SLOT(setMidiInCh(int)));
  //Save mode ratio button
  connect(minSaveRadioButton, SIGNAL(toggled(bool)),
	  this, SLOT(setSaveOnlyUsed(bool)));
  connect(hugeSaveRadioButton, SIGNAL(toggled(bool)),
	  this, SLOT(setSaveOnlyUsedComp(bool)));
  connect(saveConfigCheckBox, SIGNAL(toggled(bool)),
	  this, SLOT(setSaveConfig(bool)));
  //Colors
  connect(redSlider, SIGNAL(valueChanged(int)),
	  this, SLOT(setRedColor(int)));
  connect(greenSlider, SIGNAL(valueChanged(int)),
	  this, SLOT(setGreenColor(int)));
  connect(blueSlider, SIGNAL(valueChanged(int)),
	  this, SLOT(setBlueColor(int)));
  connect(colorListBox,
	  SIGNAL(currentItemChanged(QListWidgetItem*,QListWidgetItem*)),
	  this, SLOT(setRGBSliders(QListWidgetItem*)));
  //PopupMenu Category Subcategory Preset
  connect(categoryListView,
	  SIGNAL(customContextMenuRequested(const QPoint&)),
	  this, SLOT(categoryPopupMenu(const QPoint&)));
  connect(subcategoryListView,
	  SIGNAL(customContextMenuRequested(const QPoint&)),
	  this, SLOT(subcategoryPopupMenu(const QPoint&)));
  connect(presetListView,
	  SIGNAL(customContextMenuRequested(const QPoint &)),
	  this, SLOT(presetPopupMenu(const QPoint &)));
  //Preset and bank
  connect(presetLineEdit, SIGNAL(textChanged(const QString&)),
	  this, SLOT(setPresetName(const QString&)));
  connect(subcategoryLineEdit, SIGNAL(textChanged(const QString&)),
	  this, SLOT(setSubcategoryName(const QString&)));
  connect(categoryLineEdit, SIGNAL(textChanged(const QString&)),
	  this, SLOT(setCategoryName(const QString&)));
  connect(hbankSpinBox, SIGNAL(valueChanged(int)),
	  this, SLOT(setHBank(int)));
  connect(lbankSpinBox, SIGNAL(valueChanged(int)),
	  this, SLOT(setLBank(int)));
  connect(progSpinBox, SIGNAL(valueChanged(int)), this, SLOT(setProg(int)));
  //Global
  //connect(channelPanSlider, SIGNAL(valueChanged(int)),
  //this, SLOT(setChannelPan(int)));
  connect(feedbackSlider, SIGNAL(valueChanged(int)),
	  this, SLOT(setFeedback(int)));
  connect(LFOWaveComboBox, SIGNAL(activated(int)),
	  this, SLOT(setLfoWave(int)));
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
  //connect(channelDetuneSlider, SIGNAL(valueChanged(int)),
  //  this, SLOT(setChannelDetune(int)));
  connect(algorithmComboBox, SIGNAL(activated(int)),
	  this, SLOT(setAlgorithm(int)));
  connect(pitchBendRangeSlider, SIGNAL(valueChanged(int)),
	  this, SLOT(setPitchBendRange(int)));
  //Pitch Envelope
  connect(PL1SpinBox, SIGNAL(valueChanged(int)), this, SLOT(setPL1(int)));
  connect(PL2SpinBox, SIGNAL(valueChanged(int)), this, SLOT(setPL2(int)));
  connect(PL3SpinBox, SIGNAL(valueChanged(int)), this, SLOT(setPL3(int)));
  connect(PR1SpinBox, SIGNAL(valueChanged(int)), this, SLOT(setPR1(int)));
  connect(PR2SpinBox, SIGNAL(valueChanged(int)), this, SLOT(setPR2(int)));
  connect(PR3SpinBox, SIGNAL(valueChanged(int)), this, SLOT(setPR3(int)));
  //Function
  connect(fcVolumeSpinBox, SIGNAL(valueChanged(int)),
	  this, SLOT(setFcVolume(int)));
  connect(fcPitchSpinBox, SIGNAL(valueChanged(int)),
	  this, SLOT(setFcPitch(int)));
  connect(fcAmplitudeSpinBox, SIGNAL(valueChanged(int)),
	  this, SLOT(setFcAmplitude(int)));
  connect(mwPitchSpinBox, SIGNAL(valueChanged(int)),
	  this, SLOT(setMwPitch(int)));
  connect(mwAmplitudeSpinBox, SIGNAL(valueChanged(int)),
	  this, SLOT(setMwAmplitude(int)));
  connect(bcPitchSpinBox, SIGNAL(valueChanged(int)),
	  this, SLOT(setBcPitch(int)));
  connect(bcAmplitudeSpinBox, SIGNAL(valueChanged(int)),
	  this, SLOT(setBcAmplitude(int)));
  connect(bcPitchBiasSpinBox, SIGNAL(valueChanged(int)),
	  this, SLOT(setBcPitchBias(int)));
  connect(bcEgBiasSpinBox, SIGNAL(valueChanged(int)),
	  this, SLOT(setBcEgBias(int)));
  connect(atPitchSpinBox, SIGNAL(valueChanged(int)),
	  this, SLOT(setAtPitch(int)));
  connect(atAmplitudeSpinBox, SIGNAL(valueChanged(int)),
	  this, SLOT(setAtAmplitude(int)));
  connect(atPitchBiasSpinBox, SIGNAL(valueChanged(int)),
	  this, SLOT(setAtPitchBias(int)));
  connect(atEgBiasSpinBox, SIGNAL(valueChanged(int)),
	  this, SLOT(setAtEgBias(int)));
  //connect(reverbSpinBox, SIGNAL(valueChanged(int)),
  //  this, SLOT(setReverbRate(int)));
  connect(polyMonoComboBox, SIGNAL(activated(int)),
	  this, SLOT(setPolyMode(int)));
  connect(PortFingerFullComboBox, SIGNAL(activated(int)),
	  this, SLOT(setPortFingerFull(int)));
  connect(PortamentoTimeSlider, SIGNAL(valueChanged(int)),
	  this, SLOT(setPortaTime(int)));
  //envelope
  connect(AR1SpinBox, SIGNAL(valueChanged(int)), this, SLOT(setAR1(int)));
  connect(D1R1SpinBox, SIGNAL(valueChanged(int)), this, SLOT(setD1R1(int)));
  connect(D1L1SpinBox, SIGNAL(valueChanged(int)), this, SLOT(setD1L1(int)));
  connect(D2R1SpinBox, SIGNAL(valueChanged(int)), this, SLOT(setD2R1(int)));
  connect(RR1SpinBox, SIGNAL(valueChanged(int)), this, SLOT(setRR1(int)));
  connect(AR2SpinBox, SIGNAL(valueChanged(int)), this, SLOT(setAR2(int)));
  connect(D1R2SpinBox, SIGNAL(valueChanged(int)), this, SLOT(setD1R2(int)));
  connect(D1L2SpinBox, SIGNAL(valueChanged(int)), this, SLOT(setD1L2(int)));
  connect(D2R2SpinBox, SIGNAL(valueChanged(int)), this, SLOT(setD2R2(int)));
  connect(RR2SpinBox, SIGNAL(valueChanged(int)), this, SLOT(setRR2(int)));
  connect(AR3SpinBox, SIGNAL(valueChanged(int)), this, SLOT(setAR3(int)));
  connect(D1R3SpinBox, SIGNAL(valueChanged(int)), this, SLOT(setD1R3(int)));
  connect(D1L3SpinBox, SIGNAL(valueChanged(int)), this, SLOT(setD1L3(int)));
  connect(D2R3SpinBox, SIGNAL(valueChanged(int)), this, SLOT(setD2R3(int)));
  connect(RR3SpinBox, SIGNAL(valueChanged(int)), this, SLOT(setRR3(int)));
  connect(AR4SpinBox, SIGNAL(valueChanged(int)), this, SLOT(setAR4(int)));
  connect(D1R4SpinBox, SIGNAL(valueChanged(int)), this, SLOT(setD1R4(int)));
  connect(D1L4SpinBox, SIGNAL(valueChanged(int)), this, SLOT(setD1L4(int)));
  connect(D2R4SpinBox, SIGNAL(valueChanged(int)), this, SLOT(setD2R4(int)));
  connect(RR4SpinBox, SIGNAL(valueChanged(int)), this, SLOT(setRR4(int)));
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
  connect(OUT1Slider, SIGNAL(valueChanged(int)), this, SLOT(setVol1(int)));
  connect(OUT2Slider, SIGNAL(valueChanged(int)), this, SLOT(setVol2(int)));
  connect(OUT3Slider, SIGNAL(valueChanged(int)), this, SLOT(setVol3(int)));
  connect(OUT4Slider, SIGNAL(valueChanged(int)), this, SLOT(setVol4(int)));
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
  connect(WaveForm1ComboBox, SIGNAL(activated(int)),
	  this, SLOT(setWaveForm1(int)));
  connect(WaveForm2ComboBox, SIGNAL(activated(int)),
	  this, SLOT(setWaveForm2(int)));
  connect(WaveForm3ComboBox, SIGNAL(activated(int)),
	  this, SLOT(setWaveForm3(int)));
  connect(WaveForm4ComboBox, SIGNAL(activated(int)),
	  this, SLOT(setWaveForm4(int)));
  //PanDelay
  connect(delayActivCheckBox, SIGNAL(toggled(bool)), this, 
	  SLOT(setActivDelay(bool)));
  connect(delayReturnSlider, SIGNAL(valueChanged(int)), this,
	  SLOT(setDelayReturn(int)));
  connect(chDelaySlider, SIGNAL(valueChanged(int)), this,
	  SLOT(setChannelDelay(int)));
  connect(delayBPMFloatentry, SIGNAL(valueChanged(double, int)), this,
	  SLOT(setDelayBPM(double)));
  connect(delayBPMKnob, SIGNAL(valueChanged(double, int)), this,
	  SLOT(setDelayBPM(double)));
  connect(delayBeatRatioFloatentry, SIGNAL(valueChanged(double, int)), this,
	  SLOT(setDelayBeatRatio(double)));
  connect(delayBeatRatioKnob, SIGNAL(valueChanged(double, int)), this,
	  SLOT(setDelayBeatRatio(double)));
  connect(delayFeedbackFloatentry, SIGNAL(valueChanged(double, int)), this,
	  SLOT(setDelayFeedback(double)));
  connect(delayFeedbackKnob, SIGNAL(valueChanged(double, int)), this,
	  SLOT(setDelayFeedback(double)));
  connect(delayPanLFOFreqFloatentry, SIGNAL(valueChanged(double, int)), this,
	  SLOT(setDelayPanLFOFreq(double)));
  connect(delayPanLFOFreqKnob, SIGNAL(valueChanged(double, int)), this,
	  SLOT(setDelayPanLFOFreq(double)));
  delayPanLFOFreqKnob->setMinLogValue(0.1);
  delayPanLFOFreqKnob->setMaxLogValue(10.0);
  connect(delayPanLFODepthFloatentry, SIGNAL(valueChanged(double, int)), this,
	  SLOT(setDelayPanLFODepth(double)));
  connect(delayPanLFODepthKnob, SIGNAL(valueChanged(double, int)), this,
	  SLOT(setDelayPanLFODepth(double)));
  //category subcategory preset
  connect(categoryListView,
	  SIGNAL(currentItemChanged(QTreeWidgetItem*,QTreeWidgetItem*)),
	  this, SLOT(setCategory(QTreeWidgetItem*)));
  connect(categoryListView, SIGNAL(itemClicked(QTreeWidgetItem*,int)),
	  this, SLOT(setCategory(QTreeWidgetItem*)));
  connect(subcategoryListView,
	  SIGNAL(currentItemChanged(QTreeWidgetItem*,QTreeWidgetItem*)),
	  this, SLOT(setSubcategory(QTreeWidgetItem*)));
  connect(subcategoryListView, SIGNAL(itemClicked(QTreeWidgetItem*,int)),
	  this, SLOT(setSubcategory(QTreeWidgetItem*)));
  connect(presetListView,
	  SIGNAL(currentItemChanged(QTreeWidgetItem*,QTreeWidgetItem*)),
	  this, SLOT(setPreset(QTreeWidgetItem*)));
  connect(presetListView, SIGNAL(itemClicked(QTreeWidgetItem*,int)),
	  this, SLOT(setPreset(QTreeWidgetItem*)));
  //Connect socketnotifier to fifo
  QSocketNotifier* s = new QSocketNotifier(readFd, QSocketNotifier::Read);
  connect(s, SIGNAL(activated(int)), SLOT(readMessage(int)));

  QString sharePath(MusEGlobal::museGlobalShare);
  // Tim.
  updateInitSetPath(sharePath + QString("/presets/deicsonze/SutulaBank.dei"));    // Tim.
  updateBackgroundPixPath(sharePath + QString("/wallpapers/paper2.jpg"));    // Tim.
  updateBackgroundPixCheckBox(false);

  setTextColor(reinterpret_cast<const QColor &>(*tColor));
  setBackgroundColor(reinterpret_cast<const QColor &>(*bColor));
  setEditTextColor(reinterpret_cast<const QColor &>(*etColor));
  setEditBackgroundColor(reinterpret_cast<const QColor &>(*ebColor));

  //select the first item in the color list
  colorListBox->setCurrentItem(colorListBox->item(0));

  //color the colorFrame with the color of the text
  QPalette p = colorFrame->palette();
  p.setColor(QPalette::Window, (reinterpret_cast<const QColor &>(*tColor)));
  colorFrame->setPalette(p);

  //update maaster volume
  //updateMasterVolume(INITMASTERVOL);
  //update Quick edit
  updateQuickEdit();

  //updatePreset();
  _enabledPreset = true;
  setEnabledPreset(false);
  
    
}

//-----------------------------------------------------------
// setEnabledChannel
//-----------------------------------------------------------
void DeicsOnzeGui::setEnabledChannel(bool e) {
  sendController(_currentChannel, CTRL_CHANNELENABLE, (int)e);
  updateEnabledChannel(e);
}
//-----------------------------------------------------------
// setUpdateEnabledChannelCheckBox
//-----------------------------------------------------------
void DeicsOnzeGui::updateChannelCheckBox(bool b) {
  ChannelCheckBox->blockSignals(true);
  ChannelCheckBox->setChecked(b);
  ChannelCheckBox->blockSignals(false);
}

//-----------------------------------------------------------
// setChangeChannel
//-----------------------------------------------------------
void DeicsOnzeGui::setChangeChannel(int c) {
  _currentChannel = c-1;
  updateChannelEnable(_deicsOnze->getChannelEnable(_currentChannel));
  updateNbrVoices(_deicsOnze->getNbrVoices(_currentChannel));
  //update quick edit
  updateQuickEdit();
  //update preset
  int p, l, h;
  _deicsOnze->_preset[_currentChannel]->getHBankLBankProg(&h, &l, &p);
  updateSelectPreset(h, l, p);
  updatePreset();
}
//-----------------------------------------------------------
// setPanic
//-----------------------------------------------------------
void DeicsOnzeGui::setPanic() {
  unsigned char message[3];
  message[0]=MUSE_SYNTH_SYSEX_MFG_ID;
  message[1]=DEICSONZE_UNIQUE_ID;
  message[2]=SYSEX_PANIC;
  sendSysex(message, 3); 
}

//-----------------------------------------------------------
// setResCtrl
//-----------------------------------------------------------
void DeicsOnzeGui::setResCtrl() {
  //Detune
  updateChannelDetune(0);
  sendController(_currentChannel, CTRL_CHANNELDETUNE, 0);
  //Brightness
  updateBrightness(MIDFINEBRIGHTNESS);
  sendController(_currentChannel, CTRL_FINEBRIGHTNESS, MIDFINEBRIGHTNESS);
  //Attack
  updateAttack(MIDATTACK);
  sendController(_currentChannel, MusECore::CTRL_ATTACK_TIME, MIDATTACK);
  //Release
  updateRelease(MIDRELEASE);
  sendController(_currentChannel, MusECore::CTRL_RELEASE_TIME, MIDRELEASE);
}

//-----------------------------------------------------------
// setNbrVoices
//-----------------------------------------------------------
void DeicsOnzeGui::setNbrVoices(int nv) {
  sendController(_currentChannel, CTRL_NBRVOICES, nv);
}

//----------------------------------------------------------
// setMidiInCh
//----------------------------------------------------------
//void DeicsOnzeGui::setMidiInCh(int m) {
//  unsigned char message[4];
//  message[0]=MUSE_SYNTH_SYSEX_MFG_ID;
//  message[1]=DEICSONZE_UNIQUE_ID;
//  message[2]=SYSEX_CHANNELNUM;
//  message[3]=(unsigned char)(m-1);
//  sendSysex(message, 4);
//}

//-----------------------------------------------------------
// saveConfiguration
//-----------------------------------------------------------
void DeicsOnzeGui::saveConfiguration() {
  QString filename =
    QFileDialog::getSaveFileName(
				 this,
				 tr("Save configuration"),
				 lastDir,
				 QString("*.dco"));
  if(!filename.isEmpty()) {
    QFileInfo fi(filename);
    lastDir = fi.path();
    if(!filename.endsWith(".dco")) filename+=".dco";
    QFile f(filename);
    f.open(QIODevice::WriteOnly);
    AL::Xml* xml = new AL::Xml(&f);
    xml->header();
    _deicsOnze->writeConfiguration(xml);
    f.close();
  }
}

//-----------------------------------------------------------
// saveDefaultConfiguration
//-----------------------------------------------------------
void DeicsOnzeGui::saveDefaultConfiguration() {
  QString filename = MusEGlobal::configPath + QString("/" DEICSONZESTR ".dco");
  if(!filename.isEmpty()) {
    QFile f(filename);
    f.open(QIODevice::WriteOnly);
   
    AL::Xml* xml = new AL::Xml(&f);
    xml->header();
    _deicsOnze->writeConfiguration(xml);
    f.close();
  }
}

//-----------------------------------------------------------
// loadConfiguration
//-----------------------------------------------------------
void DeicsOnzeGui::loadConfiguration(QString fileName) {
  // read the XML file and create DOM tree
  if(!fileName.isEmpty()) {
    QFile confFile(fileName);
    if(!confFile.open(QIODevice::ReadOnly)) {
      QMessageBox::critical(0,
			    tr("Critical Error"),
			    tr("Cannot open file %1").arg(fileName));
      return;
    }
    QDomDocument domTree;
    if (!domTree.setContent(&confFile )) {
      QMessageBox::critical
	(0, tr("Critical Error"),
	 tr("Parsing error for file %1").arg(fileName));
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
	  _deicsOnze->readConfiguration(node.firstChild());
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

//-----------------------------------------------------------
// loadConfiguration
//-----------------------------------------------------------
void DeicsOnzeGui::loadConfiguration() {
  QString fileName =
    QFileDialog::getOpenFileName(
				 this,
				 tr("Load category dialog"),
				 lastDir,
				 QString("*.dco"));
  QFileInfo fi(fileName);
  lastDir = fi.path();
  loadConfiguration(fileName);
}

//-----------------------------------------------------------
// setQuality
//-----------------------------------------------------------
void DeicsOnzeGui::setQuality(const QString& q) {
  unsigned char message[4];
  message[0]=MUSE_SYNTH_SYSEX_MFG_ID;
  message[1]=DEICSONZE_UNIQUE_ID;
  message[2]=SYSEX_QUALITY;
  message[3]=(unsigned char)(q=="High"?
			     high:(q=="Middle"?
				   middle:(q=="Low"?low:ultralow)));
  sendSysex(message, 4);
}
//-----------------------------------------------------------
// setFilter
//-----------------------------------------------------------
void DeicsOnzeGui::setFilter(bool f) {
  unsigned char message[4];
  message[0]=MUSE_SYNTH_SYSEX_MFG_ID;
  message[1]=DEICSONZE_UNIQUE_ID;
  message[2]=SYSEX_FILTER;
  message[3]=(unsigned char)f;
  sendSysex(message, 4);
}  
//-----------------------------------------------------------
// setFontSize
//-----------------------------------------------------------
void DeicsOnzeGui::setFontSize(int fs) {
  applyFontSize(fs);
  unsigned char message[4];
  message[0]=MUSE_SYNTH_SYSEX_MFG_ID;
  message[1]=DEICSONZE_UNIQUE_ID;
  message[2]=SYSEX_FONTSIZE;
  message[3]=(unsigned char)fs;
  sendSysex(message, 4);
}
//-----------------------------------------------------------
// setSaveOnlyUsed
//-----------------------------------------------------------
void DeicsOnzeGui::setSaveOnlyUsed(bool sou) {
  unsigned char message[4];
  message[0]=MUSE_SYNTH_SYSEX_MFG_ID;
  message[1]=DEICSONZE_UNIQUE_ID;
  message[2]=SYSEX_SAVEONLYUSED;
  message[3]=(unsigned char)sou;
  sendSysex(message, 4);
  updateSaveOnlyUsed(sou);  
}
void DeicsOnzeGui::setSaveOnlyUsedComp(bool souc) {
  unsigned char message[4];
  message[0]=MUSE_SYNTH_SYSEX_MFG_ID;
  message[1]=DEICSONZE_UNIQUE_ID;
  message[2]=SYSEX_SAVEONLYUSED;
  message[3]=(unsigned char)!souc;
  sendSysex(message, 4);
  updateSaveOnlyUsed(!souc);
}
//-----------------------------------------------------------
// setSaveConfig
//-----------------------------------------------------------
void DeicsOnzeGui::setSaveConfig(bool ssc) {
  unsigned char message[4];
  message[0]=MUSE_SYNTH_SYSEX_MFG_ID;
  message[1]=DEICSONZE_UNIQUE_ID;
  message[2]=SYSEX_SAVECONFIG;
  message[3]=(unsigned char)ssc;
  sendSysex(message, 4);
}
//-----------------------------------------------------------
// setColor
//-----------------------------------------------------------
void DeicsOnzeGui::setRedColor(int r) {
  QListWidgetItem* i=colorListBox->selectedItems().at(0);
  if(i){
    curColor->setRgb(r, curColor->green(), curColor->blue());
    if(i->text()==QString("Text")) {
      tColor->setRgb(curColor->rgb());
      setTextColor(reinterpret_cast<const QColor &>(*curColor));
    }
    else if(i->text()==QString("Background")) {
      bColor->setRgb(curColor->rgb());
      setBackgroundColor(reinterpret_cast<const QColor &>(*curColor));
    }
    else if(i->text()==QString("Edit Text")) {
      etColor->setRgb(curColor->rgb());
      setEditTextColor(reinterpret_cast<const QColor &>(*curColor));
    }
    else if(i->text()==QString("Edit Background")) {
      ebColor->setRgb(curColor->rgb());
      setEditBackgroundColor(reinterpret_cast<const QColor &>(*curColor));
    }
    QPalette p = colorFrame->palette();
    p.setColor(QPalette::Window,
	       (reinterpret_cast<const QColor &>(*curColor)));
    colorFrame->setPalette(p);
  }
}
void DeicsOnzeGui::setGreenColor(int g) {
    QListWidgetItem* i=colorListBox->selectedItems().at(0);
    if(i) {
	curColor->setRgb(curColor->red(), g, curColor->blue());
	if(i->text()==QString("Text")) {
	    tColor->setRgb(curColor->rgb());
	    setTextColor(reinterpret_cast<const QColor &>(*curColor));
	}
	else if(i->text()==QString("Background")) {
	    bColor->setRgb(curColor->rgb());
	    setBackgroundColor(reinterpret_cast<const QColor &>(*curColor));
	}
	else if(i->text()==QString("Edit Text")) {
	    etColor->setRgb(curColor->rgb());
	    setEditTextColor(reinterpret_cast<const QColor &>(*curColor));
	}
	else if(i->text()==QString("Edit Background")) {
	    ebColor->setRgb(curColor->rgb());
	    setEditBackgroundColor(reinterpret_cast<const QColor &>(*curColor));
	}
	QPalette p = colorFrame->palette();
	p.setColor(QPalette::Window, (reinterpret_cast<const QColor &>(*curColor)));
	colorFrame->setPalette(p);
    }
}
void DeicsOnzeGui::setBlueColor(int b) {
    QListWidgetItem* i=colorListBox->selectedItems().at(0);
    if(i) {
	curColor->setRgb(curColor->red(), curColor->green(), b);
	if(i->text()==QString("Text")) {
	    tColor->setRgb(curColor->rgb());
	    setTextColor(reinterpret_cast<const QColor &>(*curColor));
	}
	else if(i->text()==QString("Background")) {
	    bColor->setRgb(curColor->rgb());
	    setBackgroundColor(reinterpret_cast<const QColor &>(*curColor));
	}
	else if(i->text()==QString("Edit Text")) {
	    etColor->setRgb(curColor->rgb());
	    setEditTextColor(reinterpret_cast<const QColor &>(*curColor));
	}
	else if(i->text()==QString("Edit Background")) {
	    ebColor->setRgb(curColor->rgb());
	    setEditBackgroundColor(reinterpret_cast<const QColor &>(*curColor));
	}
	QPalette p = colorFrame->palette();
	p.setColor(QPalette::Window, (reinterpret_cast<const QColor &>(*curColor)));
	colorFrame->setPalette(p);
    }
}
void DeicsOnzeGui::setRGBSliders(QListWidgetItem* i) {
  if(i->text()==QString("Text"))
    curColor->setRgb(tColor->red(), tColor->green(), tColor->blue());
  else if(i->text()==QString("Background"))
    curColor->setRgb(bColor->red(), bColor->green(), bColor->blue());
  else if(i->text()==QString("Edit Text"))
    curColor->setRgb(etColor->red(), etColor->green(), etColor->blue());
  else if(i->text()==QString("Edit Background"))
    curColor->setRgb(ebColor->red(), ebColor->green(), ebColor->blue());

  redSlider->blockSignals(true);
  redSlider->setValue(curColor->red());
  redSlider->blockSignals(false);
  redSpinBox->blockSignals(true);
  redSpinBox->setValue(curColor->red());
  redSpinBox->blockSignals(false);
  greenSlider->blockSignals(true);
  greenSlider->setValue(curColor->green());
  greenSlider->blockSignals(false);
  greenSpinBox->blockSignals(true);
  greenSpinBox->setValue(curColor->green());
  greenSpinBox->blockSignals(false);
  blueSlider->blockSignals(true);
  blueSlider->setValue(curColor->blue());
  blueSlider->blockSignals(false);
  blueSpinBox->blockSignals(true);
  blueSpinBox->setValue(curColor->blue());
  blueSpinBox->blockSignals(false);
  QPalette p = colorFrame->palette();
  p.setColor(QPalette::Window, (reinterpret_cast<const QColor &>(*curColor)));
  colorFrame->setPalette(p);
}
void DeicsOnzeGui::setTextColor(const QColor & c) {
  QPalette p = this->palette();
  p.setColor(QPalette::WindowText, c);
  this->setPalette(p);
  channelCtrlGroupBox->setPalette(p);
  //channelPanGroupBox->setPalette(p);
  FeedbackGroupBox->setPalette(p);
  LFOGroupBox->setPalette(p);
  ModulationMatrixGroupBox->setPalette(p);
  FeedbackGroupBox->setPalette(p);
  pitchEnvGroupBox->setPalette(p);
  Frequency1groupBox->setPalette(p);
  OUT1groupBox->setPalette(p);
  Env1GroupBox->setPalette(p);
  Scaling1GroupBox->setPalette(p);
  DetWaveEGS1GroupBox->setPalette(p);
  sensitivity1groupBox->setPalette(p);
  Frequency2groupBox->setPalette(p);
  OUT2groupBox->setPalette(p);
  Env2GroupBox->setPalette(p);
  Scaling2GroupBox->setPalette(p);
  DetWaveEGS2GroupBox->setPalette(p);
  sensitivity2groupBox->setPalette(p);
  Frequency3groupBox->setPalette(p);
  OUT3groupBox->setPalette(p);
  Env3GroupBox->setPalette(p);
  Scaling3GroupBox->setPalette(p);
  DetWaveEGS3GroupBox->setPalette(p);
  sensitivity3groupBox->setPalette(p);
  Frequency4groupBox->setPalette(p);
  OUT4groupBox->setPalette(p);
  Env4GroupBox->setPalette(p);
  Scaling4GroupBox->setPalette(p);
  DetWaveEGS4GroupBox->setPalette(p);
  sensitivity4groupBox->setPalette(p);
  transposeGroupBox->setPalette(p);
  //detuneGroupBox->setPalette(p);
  //footSWGroupBox->setPalette(p);
  pitchBendRangeGroupBox->setPalette(p);
  //reverbGroupBox->setPalette(p);
  modeGroupBox->setPalette(p);
  portamentoGroupBox->setPalette(p);
  colorGroupBox->setPalette(p);
  pathGroupBox->setPalette(p);
  qualityGroupBox->setPalette(p);
  saveModeButtonGroup->setPalette(p);
  fileGroupBox->setPalette(p);
  onReverbGroupBox->setPalette(p);
  selectLadspaReverbGroupBox->setPalette(p);
  channelReverbGroupBox->setPalette(p);
  parametersReverbGroupBox->setPalette(p);
  onChorusGroupBox->setPalette(p);
  selectLadspaChorusGroupBox->setPalette(p);
  channelChorusGroupBox->setPalette(p);
  parametersChorusGroupBox->setPalette(p);
  fontSizeGroupBox->setPalette(p);
  delayBPMGroupBox->setPalette(p);
  delayBeatRatioGroupBox->setPalette(p);
  delayFeedbackGroupBox->setPalette(p);
  delayPanLFOGroupBox->setPalette(p);
  delayPanDepthGroupBox->setPalette(p);
  delayReturnGroupBox->setPalette(p);
  channelDelayGroupBox->setPalette(p);
}

void DeicsOnzeGui::setBackgroundColor(const QColor & c) {
  if(imageCheckBox->checkState()==Qt::Unchecked) {
    QPalette p = this->palette();
    p.setColor(QPalette::Window, c);
    this->setPalette(p);
  }
}
void DeicsOnzeGui::setEditTextColor(const QColor & c) {
  QPalette p = this->palette();
  p.setColor(QPalette::Text, c);
  this->setPalette(p);
  channelCtrlGroupBox->setPalette(p);
  //channelPanGroupBox->setPalette(p);
  FeedbackGroupBox->setPalette(p);
  LFOGroupBox->setPalette(p);
  ModulationMatrixGroupBox->setPalette(p);
  FeedbackGroupBox->setPalette(p);
  pitchEnvGroupBox->setPalette(p);
  Frequency1groupBox->setPalette(p);
  OUT1groupBox->setPalette(p);
  Env1GroupBox->setPalette(p);
  Scaling1GroupBox->setPalette(p);
  DetWaveEGS1GroupBox->setPalette(p);
  sensitivity1groupBox->setPalette(p);
  Frequency2groupBox->setPalette(p);
  OUT2groupBox->setPalette(p);
  Env2GroupBox->setPalette(p);
  Scaling2GroupBox->setPalette(p);
  DetWaveEGS2GroupBox->setPalette(p);
  sensitivity2groupBox->setPalette(p);
  Frequency3groupBox->setPalette(p);
  OUT3groupBox->setPalette(p);
  Env3GroupBox->setPalette(p);
  Scaling3GroupBox->setPalette(p);
  DetWaveEGS3GroupBox->setPalette(p);
  sensitivity3groupBox->setPalette(p);
  Frequency4groupBox->setPalette(p);
  OUT4groupBox->setPalette(p);
  Env4GroupBox->setPalette(p);
  Scaling4GroupBox->setPalette(p);
  DetWaveEGS4GroupBox->setPalette(p);
  sensitivity4groupBox->setPalette(p);
  transposeGroupBox->setPalette(p);
  //detuneGroupBox->setPalette(p);
  //footSWGroupBox->setPalette(p);
  pitchBendRangeGroupBox->setPalette(p);
  //reverbGroupBox->setPalette(p);
  modeGroupBox->setPalette(p);
  portamentoGroupBox->setPalette(p);
  colorGroupBox->setPalette(p);
  pathGroupBox->setPalette(p);
  qualityGroupBox->setPalette(p);
  saveModeButtonGroup->setPalette(p);
  fileGroupBox->setPalette(p);
  masterVolKnob->setScaleValueColor(c);
  channelVolumeKnob->setScaleValueColor(c);
  channelPanKnob->setScaleValueColor(c);
  brightnessKnob->setScaleValueColor(c);
  modulationKnob->setScaleValueColor(c);
  detuneKnob->setScaleValueColor(c);
  attackKnob->setScaleValueColor(c);
  releaseKnob->setScaleValueColor(c);
  channelChorusGroupBox->setPalette(p);
  selectLadspaChorusGroupBox->setPalette(p);
  parametersChorusGroupBox->setPalette(p);
  for(int i=0; i < (int)_chorusSliderVector.size(); i++)
    if(_chorusSliderVector[i]) _chorusSliderVector[i]->setScaleValueColor(c);
  channelReverbGroupBox->setPalette(p);
  selectLadspaReverbGroupBox->setPalette(p);
  parametersReverbGroupBox->setPalette(p);
  for(int i=0; i < (int)_reverbSliderVector.size(); i++)
    if(_reverbSliderVector[i]) _reverbSliderVector[i]->setScaleValueColor(c);
  channelDelayGroupBox->setPalette(p);
  delayBPMKnob->setScaleValueColor(c);
  delayBPMGroupBox->setPalette(p);
  delayBeatRatioKnob->setScaleValueColor(c);
  delayBeatRatioGroupBox->setPalette(p);
  delayFeedbackKnob->setScaleValueColor(c);
  delayFeedbackGroupBox->setPalette(p);
  delayPanLFOFreqKnob->setScaleValueColor(c);
  delayPanLFOGroupBox->setPalette(p);
  delayPanLFODepthKnob->setScaleValueColor(c);
  delayPanDepthGroupBox->setPalette(p);
  fontSizeGroupBox->setPalette(p);
}
void DeicsOnzeGui::setEditBackgroundColor(const QColor & c) {
  QPalette p = this->palette();
  p.setColor(QPalette::Base, c);
  this->setPalette(p);
  channelCtrlGroupBox->setPalette(p);
  //channelPanGroupBox->setPalette(p);
  FeedbackGroupBox->setPalette(p);
  LFOGroupBox->setPalette(p);
  ModulationMatrixGroupBox->setPalette(p);
  FeedbackGroupBox->setPalette(p);
  pitchEnvGroupBox->setPalette(p);
  Frequency1groupBox->setPalette(p);
  OUT1groupBox->setPalette(p);
  Env1GroupBox->setPalette(p);
  Scaling1GroupBox->setPalette(p);
  DetWaveEGS1GroupBox->setPalette(p);
  sensitivity1groupBox->setPalette(p);
  Frequency2groupBox->setPalette(p);
  OUT2groupBox->setPalette(p);
  Env2GroupBox->setPalette(p);
  Scaling2GroupBox->setPalette(p);
  DetWaveEGS2GroupBox->setPalette(p);
  sensitivity2groupBox->setPalette(p);
  Frequency3groupBox->setPalette(p);
  OUT3groupBox->setPalette(p);
  Env3GroupBox->setPalette(p);
  Scaling3GroupBox->setPalette(p);
  DetWaveEGS3GroupBox->setPalette(p);
  sensitivity3groupBox->setPalette(p);
  Frequency4groupBox->setPalette(p);
  OUT4groupBox->setPalette(p);
  Env4GroupBox->setPalette(p);
  Scaling4GroupBox->setPalette(p);
  DetWaveEGS4GroupBox->setPalette(p);
  sensitivity4groupBox->setPalette(p);
  transposeGroupBox->setPalette(p);
  //detuneGroupBox->setPalette(p);
  //footSWGroupBox->setPalette(p);
  pitchBendRangeGroupBox->setPalette(p);
  //reverbGroupBox->setPalette(p);
  modeGroupBox->setPalette(p);
  portamentoGroupBox->setPalette(p);
  colorGroupBox->setPalette(p);
  pathGroupBox->setPalette(p);
  qualityGroupBox->setPalette(p);
  saveModeButtonGroup->setPalette(p);
  fileGroupBox->setPalette(p);
  p = pitchEnvFrame->palette();
  p.setColor(QPalette::Window, c);
  pitchEnvFrame->setPalette(p);
  p = envelope1Frame->palette();
  p.setColor(QPalette::Window, c);
  envelope1Frame->setPalette(p);
  p = envelope2Frame->palette();
  p.setColor(QPalette::Window, c);
  envelope2Frame->setPalette(p);
  p = envelope3Frame->palette();
  p.setColor(QPalette::Window, c);
  envelope3Frame->setPalette(p);
  p = envelope4Frame->palette();
  p.setColor(QPalette::Window, c);
  envelope4Frame->setPalette(p);
  masterVolKnob->setScaleColor(c);
  channelVolumeKnob->setScaleColor(c);
  channelPanKnob->setScaleColor(c);
  brightnessKnob->setScaleColor(c);
  modulationKnob->setScaleColor(c);
  detuneKnob->setScaleColor(c);
  attackKnob->setScaleColor(c);
  releaseKnob->setScaleColor(c);
  channelChorusGroupBox->setPalette(p);
  selectLadspaChorusGroupBox->setPalette(p);
  parametersChorusGroupBox->setPalette(p);
  for(int i=0; i < (int)_chorusSliderVector.size(); i++)
    if(_chorusSliderVector[i]) _chorusSliderVector[i]->setScaleColor(c);
  channelReverbGroupBox->setPalette(p);
  selectLadspaReverbGroupBox->setPalette(p);
  parametersReverbGroupBox->setPalette(p);
  for(int i=0; i < (int)_reverbSliderVector.size(); i++)
    if(_reverbSliderVector[i]) _reverbSliderVector[i]->setScaleColor(c);
  channelDelayGroupBox->setPalette(p);
  delayBPMKnob->setScaleColor(c);
  delayBPMGroupBox->setPalette(p);
  delayBeatRatioKnob->setScaleColor(c);
  delayBeatRatioGroupBox->setPalette(p);
  delayFeedbackKnob->setScaleColor(c);
  delayFeedbackGroupBox->setPalette(p);
  delayPanLFOFreqKnob->setScaleColor(c);
  delayPanLFOGroupBox->setPalette(p);
  delayPanLFODepthKnob->setScaleColor(c);
  delayPanDepthGroupBox->setPalette(p);
  fontSizeGroupBox->setPalette(p);
}

//-----------------------------------------------------------
// QFramePitchEnvelope
//-----------------------------------------------------------
void QFramePitchEnvelope::paintEvent(QPaintEvent* /*e*/) {
  QPainter paint(this);
  QPen pen;
  pen.setColor(*(_deicsOnzeGui->etColor));
  //if the size of pitchEnvFrame is different than QFramePitchEnvelope resize
  //and update the envelope
  if(_deicsOnzeGui->pitchEnvFrame->width()!=width() ||
     _deicsOnzeGui->pitchEnvFrame->height()!=height()) {
    resize(_deicsOnzeGui->pitchEnvFrame->width(),
	   _deicsOnzeGui->pitchEnvFrame->height());
    //update the positions of the envelope
    PitchEg* pe = &_deicsOnzeGui->_deicsOnze
      ->_preset[_deicsOnzeGui->_currentChannel]->pitchEg;
    env2Points(pe->pl1, pe->pl2, pe->pl3, pe->pr1, pe->pr2, pe->pr3);
  }
  //Draw the verticale line on the release time
  pen.setWidth(1);
  pen.setStyle(Qt::DotLine);
  paint.setPen(pen);
  paint.drawLine(P2linkP3.x(), height(), P2linkP3.x(), 0);
  //Draw the horisontal line for the center of the pitch
  pen.setStyle(Qt::DashDotLine);
  paint.setPen(pen);
  paint.drawLine(0, height()/2-DOTWIDTH/2, width(), height()/2-DOTWIDTH/2);
  //Draw the pitch envelope
  pen.setWidth(PENWIDTH);
  pen.setStyle(Qt::SolidLine);
  paint.setPen(pen);
  paint.drawRect(startlinkP1.x()-DOTWIDTH/2, startlinkP1.y()-DOTWIDTH/2,
		 DOTWIDTH, DOTWIDTH);
  paint.drawRect(P1linkP2.x()-DOTWIDTH/2, P1linkP2.y()-DOTWIDTH/2,
		 DOTWIDTH, DOTWIDTH);
  paint.drawRect(P2linkP3.x()-DOTWIDTH/2, P2linkP3.y()-DOTWIDTH/2,
		 DOTWIDTH, DOTWIDTH);
  paint.drawRect(P3linkEnd.x()-DOTWIDTH/2, P3linkEnd.y()-DOTWIDTH/2,
		 DOTWIDTH, DOTWIDTH);
  paint.drawLine(startlinkP1,P1linkP2);
  paint.drawLine(P1linkP2, P2linkP3);
  paint.drawLine(P2linkP3, P3linkEnd);
}
void QFramePitchEnvelope::mousePressEvent(QMouseEvent * e) {
    //startlinkP1
    if(e->x()<startlinkP1.x()+DRAGWIDTH && e->x()>startlinkP1.x()-DRAGWIDTH
       && e->y()<startlinkP1.y()+DRAGWIDTH && e->y()>startlinkP1.y()-DRAGWIDTH)
	isStartlinkP1Edit=true;
    //P1linkP2
    if(e->x()<P1linkP2.x()+DRAGWIDTH && e->x()>P1linkP2.x()-DRAGWIDTH
       && e->y()<P1linkP2.y()+DRAGWIDTH && e->y()>P1linkP2.y()-DRAGWIDTH)
	isP1linkP2Edit=true;
    //P2linkP3
    if(e->x()<P2linkP3.x()+DRAGWIDTH && e->x()>P2linkP3.x()-DRAGWIDTH
       && e->y()<P2linkP3.y()+DRAGWIDTH && e->y()>P2linkP3.y()-DRAGWIDTH)
	isP2linkP3Edit=true;
    //P3linkEnd
    if(e->x()<P3linkEnd.x()+DRAGWIDTH && e->x()>P3linkEnd.x()-DRAGWIDTH
       && e->y()<P3linkEnd.y()+DRAGWIDTH && e->y()>P3linkEnd.y()-DRAGWIDTH)
	isP3linkEndEdit=true;
}
void QFramePitchEnvelope::mouseReleaseEvent(QMouseEvent* /*e*/) {
    isStartlinkP1Edit=false;
    isP1linkP2Edit=false;
    isP2linkP3Edit=false;
    isP3linkEndEdit=false;
}
void QFramePitchEnvelope::mouseMoveEvent(QMouseEvent* e) {
  if(isStartlinkP1Edit) {
    if(e->y()>startlinkP1.y()) _deicsOnzeGui->PL1SpinBox->stepDown();
    if(e->y()<startlinkP1.y()) _deicsOnzeGui->PL1SpinBox->stepUp();
  }
  if(isP1linkP2Edit) {
    if(e->x()>P1linkP2.x()) _deicsOnzeGui->PR1SpinBox->stepDown();
    if(e->x()<P1linkP2.x()) _deicsOnzeGui->PR1SpinBox->stepUp();
    if(e->y()>P1linkP2.y()) _deicsOnzeGui->PL2SpinBox->stepDown();
    if(e->y()<P1linkP2.y()) _deicsOnzeGui->PL2SpinBox->stepUp();
  }
  if(isP2linkP3Edit) {
    if(e->x()>P2linkP3.x()) _deicsOnzeGui->PR2SpinBox->stepDown();
    if(e->x()<P2linkP3.x()) _deicsOnzeGui->PR2SpinBox->stepUp();
    if(e->y()>P2linkP3.y()) _deicsOnzeGui->PL3SpinBox->stepDown();
    if(e->y()<P2linkP3.y()) _deicsOnzeGui->PL3SpinBox->stepUp();
  }
  if(isP3linkEndEdit) {
    if(e->x()>P3linkEnd.x()) _deicsOnzeGui->PR3SpinBox->stepDown();
    if(e->x()<P3linkEnd.x()) _deicsOnzeGui->PR3SpinBox->stepUp();
    if(e->y()>P3linkEnd.y()) _deicsOnzeGui->PL1SpinBox->stepDown();
    if(e->y()<P3linkEnd.y()) _deicsOnzeGui->PL1SpinBox->stepUp();
  }
}
//-----------------------------------------------------------
// env2Points
//   assigns the right coordinates to the points
//   to draw the pitch envelope
//-----------------------------------------------------------
void QFramePitchEnvelope::env2Points(int pl1, int pl2, int pl3,
				     int pr1, int pr2, int pr3) {
  startlinkP1.setY(YOFFSET+MAXPHEIGHT
		   -PENWIDTH-((PL1HEIGHT-PENWIDTH)*pl1)/MAXPL);
  startlinkP1.setX(XOFFSET);
  P1linkP2.setY(YOFFSET+MAXPHEIGHT-PENWIDTH-((PL2HEIGHT-PENWIDTH)*pl2)/MAXPL);
  P1linkP2.setX(XOFFSET+WALLWIDTH+PR1WIDTH-(PR1WIDTH*pr1)/MAXPR);
  P2linkP3.setY(YOFFSET+MAXPHEIGHT-PENWIDTH-((PL3HEIGHT-PENWIDTH)*pl3)/MAXPL);
  P2linkP3.setX(P1linkP2.x()+WALLWIDTH+PR2WIDTH-(PR2WIDTH*pr2)/MAXPR);
  P3linkEnd.setY(YOFFSET+MAXPHEIGHT-PENWIDTH-((PL1HEIGHT-PENWIDTH)*pl1)/MAXPL);
  P3linkEnd.setX(P2linkP3.x()+WALLWIDTH+PR3WIDTH-(PR3WIDTH*pr3)/MAXPR);
}
//-----------------------------------------------------------
// QFrameEnvelope
//-----------------------------------------------------------
void QFrameEnvelope::paintEvent(QPaintEvent* /*e*/) {
  QPainter paint(this);
  QPen pen;
  pen.setColor(*(_deicsOnzeGui->etColor));
  //if the size of pitchEnvFrame is different than QFramePitchEnvelope resize
  //and update the envelope
  int op = _deicsOnzeGui->deicsOnzeTabWidget->currentIndex()-2;
                              //-2 because of the presetsTab and globalTab
  Eg* eg=&(_deicsOnzeGui->_deicsOnze->
	   _preset[_deicsOnzeGui->_currentChannel]->eg[op]);
  switch(op) {
  case 0 :
    if(_deicsOnzeGui->envelope1Frame->width()!=width() ||
       _deicsOnzeGui->envelope1Frame->height()!=height()) {
      resize(_deicsOnzeGui->envelope1Frame->width(),
	     _deicsOnzeGui->envelope1Frame->height());
      //update the positions of the envelope
      env2Points(eg->ar, eg->d1r, eg->d1l, eg->d2r, eg->rr);
    }
    break;
  case 1 :
    if(_deicsOnzeGui->envelope2Frame->width()!=width() ||
       _deicsOnzeGui->envelope2Frame->height()!=height()) {
      resize(_deicsOnzeGui->envelope2Frame->width(),
	     _deicsOnzeGui->envelope2Frame->height());
      //update the positions of the envelope
      env2Points(eg->ar, eg->d1r, eg->d1l, eg->d2r, eg->rr);
    }
    break;
  case 2 :
    if(_deicsOnzeGui->envelope3Frame->width()!=width() ||
       _deicsOnzeGui->envelope3Frame->height()!=height()) {
      resize(_deicsOnzeGui->envelope3Frame->width(),
	     _deicsOnzeGui->envelope3Frame->height());
      //update the positions of the envelope
      env2Points(eg->ar, eg->d1r, eg->d1l, eg->d2r, eg->rr);
    }
    break;
  case 3 :
    if(_deicsOnzeGui->envelope4Frame->width()!=width() ||
       _deicsOnzeGui->envelope4Frame->height()!=height()) {
      resize(_deicsOnzeGui->envelope4Frame->width(),
	     _deicsOnzeGui->envelope4Frame->height());
      //update the positions of the envelope
      env2Points(eg->ar, eg->d1r, eg->d1l, eg->d2r, eg->rr);
    }
    break;
  default :
    printf("QFrameEnvelope::paintEvent switch case error\n");
    break;
  }
  //Draw the vertical line of the release note
  pen.setWidth(1);
  pen.setStyle(Qt::DotLine);
  paint.setPen(pen);
  paint.drawLine(D2linkRR.x(), MAXHEIGHT, D2linkRR.x(), YOFFSET);
  //Draw the volume envelope
  pen.setWidth(PENWIDTH);
  pen.setStyle(Qt::SolidLine);
  paint.setPen(pen);
  paint.drawRect(startlinkAR.x()-DOTWIDTH/2, startlinkAR.y()-DOTWIDTH/2,
		 DOTWIDTH, DOTWIDTH);
  paint.drawRect(ARlinkD1.x()-DOTWIDTH/2, ARlinkD1.y()-DOTWIDTH/2,
		 DOTWIDTH, DOTWIDTH);
  paint.drawRect(D1linkD2.x()-DOTWIDTH/2, D1linkD2.y()-DOTWIDTH/2,
		 DOTWIDTH, DOTWIDTH);
  paint.drawRect(D2linkRR.x()-DOTWIDTH/2, D2linkRR.y()-DOTWIDTH/2,
		 DOTWIDTH, DOTWIDTH);
  paint.drawRect(RRlinkEnd.x()-DOTWIDTH/2, RRlinkEnd.y()-DOTWIDTH/2,
		 DOTWIDTH, DOTWIDTH);
  paint.drawLine(startlinkAR,ARlinkD1);
  paint.drawLine(ARlinkD1, D1linkD2);
  paint.drawLine(D1linkD2, D2linkRR);
  paint.drawLine(D2linkRR, RRlinkEnd);
}
void QFrameEnvelope::mousePressEvent(QMouseEvent * e) {
    //ARlinkD1
    if(e->x()<ARlinkD1.x()+DRAGWIDTH && e->x()>ARlinkD1.x()-DRAGWIDTH
       && e->y()<ARlinkD1.y()+DRAGWIDTH && e->y()>ARlinkD1.y()-DRAGWIDTH)
	isARlinkD1Edit=true;
    //D1linkD2
    if(e->x()<D1linkD2.x()+DRAGWIDTH && e->x()>D1linkD2.x()-DRAGWIDTH
       && e->y()<D1linkD2.y()+DRAGWIDTH && e->y()>D1linkD2.y()-DRAGWIDTH)
	isD1linkD2Edit=true;
    //D2linkRR
    if(e->x()<D2linkRR.x()+DRAGWIDTH && e->x()>D2linkRR.x()-DRAGWIDTH
       && e->y()<D2linkRR.y()+DRAGWIDTH && e->y()>D2linkRR.y()-DRAGWIDTH)
	isD2linkRREdit=true;
    //RRlinkEnd
    if(e->x()<RRlinkEnd.x()+DRAGWIDTH && e->x()>RRlinkEnd.x()-DRAGWIDTH
       && e->y()<RRlinkEnd.y()+DRAGWIDTH && e->y()>RRlinkEnd.y()-DRAGWIDTH)
	isRRlinkEndEdit=true;
}
void QFrameEnvelope::mouseReleaseEvent(QMouseEvent* /*e*/) {
    isARlinkD1Edit=false;
    isD1linkD2Edit=false;
    isD2linkRREdit=false;
    isRRlinkEndEdit=false;
}
void QFrameEnvelope::mouseMoveEvent(QMouseEvent* e) {
    if(isARlinkD1Edit)
    {
	switch(op) {
	    case 0 :
		if(e->x()>ARlinkD1.x()) _deicsOnzeGui->AR1SpinBox->stepDown();
		if(e->x()<ARlinkD1.x()) _deicsOnzeGui->AR1SpinBox->stepUp();
		break;
	    case 1 :
		if(e->x()>ARlinkD1.x()) _deicsOnzeGui->AR2SpinBox->stepDown();
		if(e->x()<ARlinkD1.x()) _deicsOnzeGui->AR2SpinBox->stepUp();
		break;
	    case 2 :
		if(e->x()>ARlinkD1.x()) _deicsOnzeGui->AR3SpinBox->stepDown();
		if(e->x()<ARlinkD1.x()) _deicsOnzeGui->AR3SpinBox->stepUp();
		break;
	    case 3 :
		if(e->x()>ARlinkD1.x()) _deicsOnzeGui->AR4SpinBox->stepDown();
		if(e->x()<ARlinkD1.x()) _deicsOnzeGui->AR4SpinBox->stepUp();
		break;
	    default :
		break;
	}
    }
    if(isD1linkD2Edit)
    {
	switch(op) {
	    case 0 :
		if(e->x()>D1linkD2.x()) _deicsOnzeGui->D1R1SpinBox->stepDown();
		if(e->x()<D1linkD2.x()) _deicsOnzeGui->D1R1SpinBox->stepUp();
		if(e->y()>D1linkD2.y()) _deicsOnzeGui->D1L1SpinBox->stepDown();
		if(e->y()<D1linkD2.y()) _deicsOnzeGui->D1L1SpinBox->stepUp();
		break;
	    case 1 :
		if(e->x()>D1linkD2.x()) _deicsOnzeGui->D1R2SpinBox->stepDown();
		if(e->x()<D1linkD2.x()) _deicsOnzeGui->D1R2SpinBox->stepUp();
		if(e->y()>D1linkD2.y()) _deicsOnzeGui->D1L2SpinBox->stepDown();
		if(e->y()<D1linkD2.y()) _deicsOnzeGui->D1L2SpinBox->stepUp();
		break;
	    case 2 :
		if(e->x()>D1linkD2.x()) _deicsOnzeGui->D1R3SpinBox->stepDown();
		if(e->x()<D1linkD2.x()) _deicsOnzeGui->D1R3SpinBox->stepUp();
		if(e->y()>D1linkD2.y()) _deicsOnzeGui->D1L3SpinBox->stepDown();
		if(e->y()<D1linkD2.y()) _deicsOnzeGui->D1L3SpinBox->stepUp();
		break;
	    case 3 :
		if(e->x()>D1linkD2.x()) _deicsOnzeGui->D1R4SpinBox->stepDown();
		if(e->x()<D1linkD2.x()) _deicsOnzeGui->D1R4SpinBox->stepUp();
		if(e->y()>D1linkD2.y()) _deicsOnzeGui->D1L4SpinBox->stepDown();
		if(e->y()<D1linkD2.y()) _deicsOnzeGui->D1L4SpinBox->stepUp();
		break;
	    default :
		break;
	}
    }
    if(isD2linkRREdit)
    {
	switch(op) {
	    case 0 :
		if(e->x()>D2linkRR.x() /*&& e->y()<D2linkRR.y()*/)
		    _deicsOnzeGui->D2R1SpinBox->stepDown();
		if(e->x()<D2linkRR.x() /*&& e->y()>D2linkRR.y()*/)
		    _deicsOnzeGui->D2R1SpinBox->stepUp();
		break;
	    case 1 :
		if(e->x()>D2linkRR.x() /*&& e->y()<D2linkRR.y()*/)
		    _deicsOnzeGui->D2R2SpinBox->stepDown();
		if(e->x()<D2linkRR.x() /*&& e->y()>D2linkRR.y()*/)
		    _deicsOnzeGui->D2R2SpinBox->stepUp();
		break;
	    case 2 :
		if(e->x()>D2linkRR.x() /*&& e->y()<D2linkRR.y()*/)
		    _deicsOnzeGui->D2R3SpinBox->stepDown();
		if(e->x()<D2linkRR.x() /*&& e->y()>D2linkRR.y()*/)
		    _deicsOnzeGui->D2R3SpinBox->stepUp();
		break;
	    case 3 :
		if(e->x()>D2linkRR.x() /*&& e->y()<D2linkRR.y()*/)
		    _deicsOnzeGui->D2R4SpinBox->stepDown();
		if(e->x()<D2linkRR.x() /*&& e->y()>D2linkRR.y()*/)
		    _deicsOnzeGui->D2R4SpinBox->stepUp();
		break;
	    default :
		break;
	}
    }
    if(isRRlinkEndEdit)
    {
	switch(op) {
	    case 0 :
		if(e->x()>RRlinkEnd.x()) _deicsOnzeGui->RR1SpinBox->stepDown();
		if(e->x()<RRlinkEnd.x()) _deicsOnzeGui->RR1SpinBox->stepUp();
		break;
	    case 1 :
		if(e->x()>RRlinkEnd.x()) _deicsOnzeGui->RR2SpinBox->stepDown();
		if(e->x()<RRlinkEnd.x()) _deicsOnzeGui->RR2SpinBox->stepUp();
		break;
	    case 2 :
		if(e->x()>RRlinkEnd.x()) _deicsOnzeGui->RR3SpinBox->stepDown();
		if(e->x()<RRlinkEnd.x()) _deicsOnzeGui->RR3SpinBox->stepUp();
		break;
	    case 3 :
		if(e->x()>RRlinkEnd.x()) _deicsOnzeGui->RR4SpinBox->stepDown();
		if(e->x()<RRlinkEnd.x()) _deicsOnzeGui->RR4SpinBox->stepUp();
		break;
	    default :
		break;
	}
    }
}
//-----------------------------------------------------------
// env2Points
//   assigns the right coordinates to the points
//   to draw the envelope
//-----------------------------------------------------------
void QFrameEnvelope::env2Points(int ar, int d1r, int d1l, int d2r, int rr) {
    startlinkAR.setY(MAXHEIGHT-PENWIDTH);
    startlinkAR.setX(PENWIDTH);
    ARlinkD1.setY(PENWIDTH);
    ARlinkD1.setX(PENWIDTH+ARWIDTH-(ARWIDTH*ar)/MAXAR);
    D1linkD2.setY(PENWIDTH+
		  (D1LHEIGHT-2*PENWIDTH-((D1LHEIGHT-2*PENWIDTH)*d1l)/MAXD1L));
    D1linkD2.setX(ARlinkD1.x()+D1RWIDTH-(D1RWIDTH*d1r)/MAXD1R);
    D2linkRR.setY(D1linkD2.y()
		  +((D1LHEIGHT-2*PENWIDTH-D1linkD2.y())*d2r)/MAXD2R);
    D2linkRR.setX(D1linkD2.x()+D2RWIDTH-(D2RWIDTH*d2r)/MAXD2R);
    RRlinkEnd.setY(MAXHEIGHT-PENWIDTH);
    RRlinkEnd.setX(D2linkRR.x()
		   +(RRWIDTH-PENWIDTH-((RRWIDTH-PENWIDTH)*rr)/MAXRR));
}

//-----------------------------------------------------------
// processEvent(const MidiEvent&);
//-----------------------------------------------------------
void DeicsOnzeGui::processEvent(const MusECore::MidiPlayEvent& ev) {
  //Controler
  if (ev.type() == MusECore::ME_CONTROLLER) {
    //printf("MusECore::ME_CONTROLLER\n");
    int id=ev.dataA();
    int ch=ev.channel();
    int val=ev.dataB();
    if(ch == _currentChannel || id == CTRL_CHANNELENABLE) {
      switch(id) {
      case CTRL_AR: updateAR(0, val); break;
      case CTRL_D1R: updateD1R(0, val); break;
      case CTRL_D2R: updateD2R(0, val); break;
      case CTRL_RR: updateRR(0, val); break;
      case CTRL_D1L: updateD1L(0, val); break;
      case CTRL_LS: updateLS(0, val); break;
      case CTRL_RS: updateRS(0, val); break;
      case CTRL_EBS: updateEBS(0, val); break;
      case CTRL_AME: updateAME(0, val==1); break;
      case CTRL_KVS: updateKVS(0, val); break;
      case CTRL_OUT: updateOUT(0, val); break;
      case CTRL_RATIO: updateRATIO(0, val); break;
      case CTRL_DET: updateDET(0, val); break;
      case CTRL_AR+DECAPAR1: updateAR(1, val); break;
      case CTRL_D1R+DECAPAR1: updateD1R(1, val); break;
      case CTRL_D2R+DECAPAR1: updateD2R(1, val); break;
      case CTRL_RR+DECAPAR1: updateRR(1, val); break;
      case CTRL_D1L+DECAPAR1: updateD1L(1, val); break;
      case CTRL_LS+DECAPAR1: updateLS(1, val); break;
      case CTRL_RS+DECAPAR1: updateRS(1, val); break;
      case CTRL_EBS+DECAPAR1: updateEBS(1, val); break;
      case CTRL_AME+DECAPAR1: updateAME(1, val==1); break;
      case CTRL_KVS+DECAPAR1: updateKVS(1, val); break;
      case CTRL_OUT+DECAPAR1: updateOUT(1, val); break;
      case CTRL_RATIO+DECAPAR1: updateRATIO(1, val); break;
      case CTRL_DET+DECAPAR1: updateDET(1, val); break;
      case CTRL_AR+2*DECAPAR1: updateAR(2, val); break;
      case CTRL_D1R+2*DECAPAR1: updateD1R(2, val); break;
      case CTRL_D2R+2*DECAPAR1: updateD2R(2, val); break;
      case CTRL_RR+2*DECAPAR1: updateRR(2, val); break;
      case CTRL_D1L+2*DECAPAR1: updateD1L(2, val); break;
      case CTRL_LS+2*DECAPAR1: updateLS(2, val); break;
      case CTRL_RS+2*DECAPAR1: updateRS(2, val); break;
      case CTRL_EBS+2*DECAPAR1: updateEBS(2, val); break;
      case CTRL_AME+2*DECAPAR1: updateAME(2, val==1); break;
      case CTRL_KVS+2*DECAPAR1: updateKVS(2, val); break;
      case CTRL_OUT+2*DECAPAR1: updateOUT(2, val); break;
      case CTRL_RATIO+2*DECAPAR1: updateRATIO(2, val); break;
      case CTRL_DET+2*DECAPAR1: updateDET(2, val); break;
      case CTRL_AR+3*DECAPAR1: updateAR(3, val); break;
      case CTRL_D1R+3*DECAPAR1: updateD1R(3, val); break;
      case CTRL_D2R+3*DECAPAR1: updateD2R(3, val); break;
      case CTRL_RR+3*DECAPAR1: updateRR(3, val); break;
      case CTRL_D1L+3*DECAPAR1: updateD1L(3, val); break;
      case CTRL_LS+3*DECAPAR1: updateLS(3, val); break;
      case CTRL_RS+3*DECAPAR1: updateRS(3, val); break;
      case CTRL_EBS+3*DECAPAR1: updateEBS(3, val); break;
      case CTRL_AME+3*DECAPAR1: updateAME(3, val==1); break;
      case CTRL_KVS+3*DECAPAR1: updateKVS(3, val); break;
      case CTRL_OUT+3*DECAPAR1: updateOUT(3, val); break;
      case CTRL_RATIO+3*DECAPAR1: updateRATIO(3, val); break;
      case CTRL_DET+3*DECAPAR1: updateDET(3, val); break;
      case CTRL_ALG: updateALG(val); break;
      case CTRL_FEEDBACK: updateFEEDBACK(val); break;
      case CTRL_SPEED: updateSPEED(val); break;
      case CTRL_DELAY: updateDELAY(val); break;
      case CTRL_PMODDEPTH: updatePMODDEPTH(val); break;
      case CTRL_AMODDEPTH: updateAMODDEPTH(val); break;
      case CTRL_SYNC: updateSYNC(val==1); break;
      case CTRL_WAVE: updateWAVE(val); break;
      case CTRL_PMODSENS: updatePMODSENS(val); break;
      case CTRL_AMS: updateAMS(val); break;
      case CTRL_TRANSPOSE: updateTRANSPOSE(val); break;
      case CTRL_POLYMODE: updatePOLYMODE(val); break;
      case CTRL_PBENDRANGE: updatePBENDRANGE(val); break;
      case CTRL_PORTAMODE: updatePORTAMODE(val); break;
      case CTRL_PORTATIME: updatePORTATIME(val); break;
      case CTRL_FCVOLUME: updateFcVolume(val); break;
      case CTRL_FSW:
	break;
      case CTRL_MWPITCH: updateMwPitch(val); break;
      case CTRL_MWAMPLITUDE: updateMwAmplitude(val); break;
      case CTRL_BCPITCH: updateBcPitch(val); break;
      case CTRL_BCAMPLITUDE: updateBcAmplitude(val); break;
      case CTRL_BCPITCHBIAS: updateBcPitchBias(val); break;
      case CTRL_BCEGBIAS: updateBcEgBias(val); break;
      case CTRL_PR1: updatePR1(val); break;
      case CTRL_PR2: updatePR2(val); break;
      case CTRL_PR3: updatePR3(val); break;
      case CTRL_PL1: updatePL1(val); break;
      case CTRL_PL2: updatePL2(val); break;
      case CTRL_PL3: updatePL3(val); break;
      case CTRL_FIX: updateFIX(0, val==1); break;
      case CTRL_FIXRANGE: updateFIXRANGE(0, val); break;
      case CTRL_OSW: updateOSW(0, val); break;
      case CTRL_SHFT: updateSHFT(0, val); break;
      case CTRL_FIX+DECAPAR2: updateFIX(1, val==1); break;
      case CTRL_FIXRANGE+DECAPAR2: updateFIXRANGE(1, val); break;
      case CTRL_OSW+DECAPAR2: updateOSW(1, val); break;
      case CTRL_SHFT+DECAPAR2: updateSHFT(1, val); break;
      case CTRL_FIX+2*DECAPAR2: updateFIX(2, val==1); break;
      case CTRL_FIXRANGE+2*DECAPAR2: updateFIXRANGE(2, val); break;
      case CTRL_OSW+2*DECAPAR2: updateOSW(2, val); break;
      case CTRL_SHFT+2*DECAPAR2: updateSHFT(2, val); break;
      case CTRL_FIX+3*DECAPAR2: updateFIX(3, val==1); break;
      case CTRL_FIXRANGE+3*DECAPAR2: updateFIXRANGE(3, val); break;
      case CTRL_OSW+3*DECAPAR2: updateOSW(3, val); break;
      case CTRL_SHFT+3*DECAPAR2: updateSHFT(3, val); break;
      case CTRL_REVERBRATE: /*updateReverbRate(val);*/ break;
      case CTRL_FCPITCH: updateFcPitch(val); break;
      case CTRL_FCAMPLITUDE: updateFcAmplitude(val); break;
      case CTRL_CHANNELENABLE:
	if(ch == _currentChannel) updateChannelEnable(val);
	break;
      case CTRL_CHANNELDETUNE: updateChannelDetune(val); break;
      case CTRL_CHANNELPAN: updateChannelPan(val); break;
      case CTRL_CHANNELVOLUME: updateChannelVolume(val); break;
      case CTRL_NBRVOICES: updateNbrVoices(val); break;
      case CTRL_FINEBRIGHTNESS: updateBrightness(val); break;
      case MusECore::CTRL_ATTACK_TIME: updateAttack(val); break;
      case MusECore::CTRL_RELEASE_TIME: updateRelease(val); break;
      case MusECore::CTRL_CHORUS_SEND: updateChannelChorus(val); break;
      case MusECore::CTRL_REVERB_SEND: updateChannelReverb(val); break;
      case MusECore::CTRL_VARIATION_SEND: updateChannelDelay(val); break;
      case MusECore::CTRL_MODULATION: updateModulation(val); break;
      case MusECore::CTRL_PROGRAM :	
	int hbank = (val & 0xff0000) >> 16;
	int lbank = (val & 0xff00) >> 8;
	if (hbank > 127)  // map "dont care" to 0
	  hbank = 0;
	if (lbank > 127)
	  lbank = 0;
	int prog  = val & 0x7f;
	//printf("GUI program : ch = %d, hbank = %d, lbank = %d, prog = %d\n", 
	//       ch, hbank, lbank, prog);
	//change the _deicsonze preset
	//to update the right preset
	_deicsOnze->programSelect(ch, hbank, lbank, prog);
	//only display _deicsonze preset
	updateSelectPreset(hbank, lbank, prog);
	updatePreset();
	break;
      }
    }
  }
  // Sysexes
  else if (ev.type() == MusECore::ME_SYSEX) {
    //printf("MusECore::ME_SYSEX\n");
    unsigned char* data = ev.data();
    
    int cmd = *data;
    float f;
    switch (cmd) {
    case SYSEX_CHORUSACTIV :
      updateChorusActiv((bool)data[1]);
      break;
    case SYSEX_CHORUSRETURN :
      updateChorusReturn((int)data[1]);
      break;
    case SYSEX_REVERBACTIV :
      updateReverbActiv((bool)data[1]);
      break;
    case SYSEX_REVERBRETURN :
      updateReverbReturn((int)data[1]);
      break;
      /*case SYSEX_CHORUS1PAN :
      updatePanChorus1((int)data[1]);
      break;
    case SYSEX_CHORUS1LFOFREQ :
      updateLFOFreqChorus1((int)data[1]);
      break;
    case SYSEX_CHORUS1DEPTH :
      updateDepthChorus1((int)data[1]);
      break;
    case SYSEX_CHORUS2PAN :
      updatePanChorus2((int)data[1]);
      break;
    case SYSEX_CHORUS2LFOFREQ :
      updateLFOFreqChorus2((int)data[1]);
      break;
    case SYSEX_CHORUS2DEPTH :
      updateDepthChorus2((int)data[1]);
      break;*/
    case SYSEX_DELAYACTIV :
      updateDelayActiv((bool)data[1]);
      break;
    case SYSEX_DELAYRETURN :
      updateDelayReturn((int)data[1]);
      break;
    case SYSEX_DELAYBPM :
      memcpy(&f, &data[1], sizeof(float));
      updateDelayBPM(f);
      break;
    case SYSEX_DELAYBEATRATIO :
      memcpy(&f, &data[1], sizeof(float));
      updateDelayBeatRatio(f);
      break;
    case SYSEX_DELAYFEEDBACK :
      memcpy(&f, &data[1], sizeof(float));
      updateDelayFeedback(f);
      break;
    case SYSEX_DELAYLFOFREQ :
      memcpy(&f, &data[1], sizeof(float));
      updateDelayPanLFOFreq(f);
      break;
    case SYSEX_DELAYLFODEPTH :
      memcpy(&f, &data[1], sizeof(float));
      updateDelayPanLFODepth(f);
      break;
    case SYSEX_QUALITY :
      updateQuality((int)data[1]);
      break;
    case SYSEX_FILTER :
      updateFilter((bool)data[1]);
      break;
    case SYSEX_FONTSIZE :
      updateFontSize((int)data[1]);
      applyFontSize((int)data[1]);
      break;
    case SYSEX_MASTERVOL :
      updateMasterVolume((int)data[1]);
      break;
    case SYSEX_SAVECONFIG :
      updateSaveConfig((bool)data[1]);
      break;
    case SYSEX_SAVEONLYUSED :
      updateSaveOnlyUsed((bool)data[1]);
      break;
    case SYSEX_COLORGUI :
      tColor->setRgb(data[1], data[2], data[3]);
      bColor->setRgb(data[4], data[5], data[6]);
      etColor->setRgb(data[7], data[8], data[9]);
      ebColor->setRgb(data[10], data[11], data[12]);
      setTextColor(reinterpret_cast<const QColor &>(*tColor));
      setBackgroundColor(reinterpret_cast<const QColor &>(*bColor));
      setEditTextColor(reinterpret_cast<const QColor &>(*etColor));
      setEditBackgroundColor(reinterpret_cast<const QColor &>(*ebColor));
      setRGBSliders(colorListBox->currentItem());
      break;
    case SYSEX_ISINITSET :
      updateInitSetCheckBox((bool)data[1]);
      break;
    case SYSEX_INITSETPATH :
      updateInitSetPath(QString((char*)&data[1]));
      break;
    case SYSEX_ISBACKGROUNDPIX :
      updateBackgroundPixCheckBox((bool)data[1]);
      if((bool)data[1]) applyBackgroundPix();
      break;
    case SYSEX_BACKGROUNDPIXPATH :
      updateBackgroundPixPath(QString((char*)&data[1]));
      break;
    case SYSEX_UPDATESETGUI :
      setSet();
      subcategoryListView->clear();
      presetListView->clear();
      updateCategoryName("NONE", false);
      hbankSpinBox->setEnabled(false);
      updatePreset();
      updateSubcategoryName("NONE", false);
      progSpinBox->setEnabled(false);
      updatePresetName("INITVOICE", false);
      break;
    case SYSEX_BUILDGUIREVERB :
      buildGuiReverb();
      break;
    case SYSEX_BUILDGUICHORUS :
      buildGuiChorus();
      break;
    case SYSEX_LOADSET :
      //printf("LoadSet\n");
      // read the XML file and create DOM tree
      /*QString filename = (const char*) (data+2);
      QFile deicsonzeFile(filename);
      deicsonzeFile.open(IO_ReadOnly);
      QDomDocument domTree;
      domTree.setContent(&deicsonzeFile);
      deicsonzeFile.close();
      QDomNode node = domTree.documentElement();

      printf("After XML\n");
      while (!node.isNull()) {
	QDomElement e = node.toElement();
	if (e.isNull())
	  continue;
	if (e.tagName() == "deicsOnzeSet") {
	  QString version = e.attribute(QString("version"));
	  if (version == "1.0") {
	  _deicsOnze->_preset=_deicsOnze->_initialPreset;*/
      //read the set
      if((bool)data[1]) {
	//printf("Mini\n");
	updateSaveOnlyUsed(true);
      }
      else {
	//printf("Huge\n");
	//while(!_deicsOnze->_set->_categoryVector.empty())
	//  delete(*_deicsOnze->_set->_categoryVector.begin());
	updateSaveOnlyUsed(false);
      }
      //_deicsOnze->_set->readSet(node.firstChild());
      //display load preset
      setSet();
      /*}
	  else printf("Wrong set version : %s\n",
		      version.toLatin1().constData());
	}
	node = node.nextSibling();
      }
      break;
      // delete the temporary file created
      QString rmfile;
      rmfile="rm ";
      rmfile+=filename;
      system(rmfile);
      printf("Finit\n");*/
    }
  }
}

/*!
    \fn SimpleSynthGui::readMessage(int)
 */
void DeicsOnzeGui::readMessage(int)
{
    MessGui::readMessage();
}

//-----------------------------------------------------------
// num3Digits(int n)
//-----------------------------------------------------------
QString DeicsOnzeGui::num3Digits(int n) {
    QString sn=QString::number(n);
    return(sn.length()==1?"00"+sn:(sn.length()==2?"0"+sn:sn));
}

//-----------------------------------------------------------
// deleteSet
//-----------------------------------------------------------
void DeicsOnzeGui::deleteSetDialog() {
  //TODO : maybe to put this in sysex to deicsonze.cpp
  for(int c = 0; c < NBRCHANNELS; c++)
    _deicsOnze->_preset[c]=_deicsOnze->_initialPreset;
  while(!_deicsOnze->_set->_categoryVector.empty())
    delete(*_deicsOnze->_set->_categoryVector.begin());
  setSet();
  //_currentQLVCategory = NULL;
  presetListView->clear();
  subcategoryListView->clear();
  updateCategoryName("NONE", false);
  hbankSpinBox->setEnabled(false);
  updateSubcategoryName("NONE", false);
  lbankSpinBox->setEnabled(false);
  updatePresetName("INITVOICE", false);
  progSpinBox->setEnabled(false);
  updatePreset();
}
//-----------------------------------------------------------
// loadSetDialog
//-----------------------------------------------------------
void DeicsOnzeGui::loadSetDialog() {
  QString fileName =
    QFileDialog::getOpenFileName(
				 this,
				 tr("Load set dialog"),
				 lastDir,
				 QString("*.dei")
				 );
  
  // read the XML file and create DOM tree
  if(!fileName.isEmpty()) {
    QFileInfo fi(fileName);
    lastDir = fi.path();
    QFile deicsonzeFile(fileName);
    if(!deicsonzeFile.open(QIODevice::ReadOnly)) {
      QMessageBox::critical(0,
			    tr("Critical Error"),
			    tr("Cannot open file %1").arg(fileName));
      return;
    }
    QDomDocument domTree;
    if (!domTree.setContent(&deicsonzeFile )) {
      QMessageBox::critical
	(0, tr("Critical Error"),
	 tr("Parsing error for file %1").arg(fileName));
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
	  for(int c = 0; c < NBRCHANNELS; c++)
	    _deicsOnze->_preset[c]=_deicsOnze->_initialPreset;
	  while(!_deicsOnze->_set->_categoryVector.empty())
	    delete(*_deicsOnze->_set->_categoryVector.begin());
	  _deicsOnze->_set->readSet(node.firstChild());
	  //display load preset
	  setSet();
	  subcategoryListView->clear();
	  presetListView->clear();
	  updateCategoryName("NONE", false);
	  hbankSpinBox->setEnabled(false);
	  updatePreset();
	  updateSubcategoryName("NONE", false);
	  progSpinBox->setEnabled(false);
	  updatePresetName("INITVOICE", false);
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

//-----------------------------------------------------------
// saveSetDialog
//-----------------------------------------------------------
void DeicsOnzeGui::saveSetDialog() {
  QString filename =
    QFileDialog::getSaveFileName(
				 this,
				 tr("Save set dialog"),
				 lastDir,
				 QString("*.dei"));
  if(!filename.isEmpty()) {
    QFileInfo fi(filename);
    lastDir = fi.path();
    if(!filename.endsWith(".dei")) filename+=".dei";
    QFile f(filename);
    f.open(QIODevice::WriteOnly);
    
    AL::Xml* xml = new AL::Xml(&f);
    xml->header();
    _deicsOnze->_set->writeSet(xml, false);
    
    f.close();
  }
}

//-----------------------------------------------------------
// popupMenuCategory
//-----------------------------------------------------------
void DeicsOnzeGui::categoryPopupMenu(const QPoint&) {
  QTreeWidgetItem* cat = categoryListView->currentItem();
  QMenu *categoryMenu = new QMenu;
  categoryMenu->addAction(tr("New category"),
			  this, SLOT(newCategoryDialog()));
  QAction* deleteItem = categoryMenu->addAction(tr("Delete category"), this,
						SLOT(deleteCategoryDialog()));
  categoryMenu->addAction(tr("Load category"),
			  this, SLOT(loadCategoryDialog()));
  QAction* saveItem = categoryMenu->addAction(tr("Save category"), this,
					      SLOT(saveCategoryDialog()));
  if(!cat || !categoryListView->isItemSelected(cat)) {
    deleteItem->setEnabled(false);
    saveItem->setEnabled(false);	
  }
  categoryMenu->addSeparator();
  categoryMenu->addAction(tr("Load set"),
			  this, SLOT(loadSetDialog()));;
  categoryMenu->addAction(tr("Save set"),
			  this, SLOT(saveSetDialog()));;
  categoryMenu->addAction(tr("Delete set"),
			  this, SLOT(deleteSetDialog()));;
  categoryMenu->exec(QCursor::pos());
  delete categoryMenu; // Tim.
}
void DeicsOnzeGui::subcategoryPopupMenu(const QPoint&) {
  QTreeWidgetItem* sub = subcategoryListView->currentItem();
  QMenu *subcategoryMenu = new QMenu;
  QAction* newItem =
    subcategoryMenu->addAction(tr("New subcategory"), this,
			       SLOT(newSubcategoryDialog()));
  QAction* deleteItem =
    subcategoryMenu->addAction(tr("Delete subcategory"),
			       this, SLOT(deleteSubcategoryDialog()));
  QAction* loadItem =
    subcategoryMenu->addAction(tr("Load subcategory"), this,
			       SLOT(loadSubcategoryDialog()));
  QAction* saveItem =
    subcategoryMenu->addAction(tr("Save subcategory"), this,
			       SLOT(saveSubcategoryDialog()));
  if(!sub || !subcategoryListView->isItemSelected(sub)) {
    deleteItem->setEnabled(false);
    saveItem->setEnabled(false);	
  }
  if(!categoryListView->currentItem()
     || !categoryListView->isItemSelected(categoryListView->currentItem())) {
    newItem->setEnabled(false);
    loadItem->setEnabled(false);	
  }	
  subcategoryMenu->exec(QCursor::pos());
  delete subcategoryMenu; // Tim.
}
void DeicsOnzeGui::presetPopupMenu(const QPoint&) {
  QTreeWidgetItem* pre = presetListView->currentItem();
  QMenu *presetMenu = new QMenu;
  QAction* newItem = presetMenu->addAction(tr("New preset"), this,
					   SLOT(newPresetDialog()));
  QAction* deleteItem = presetMenu->addAction(tr("Delete preset"), this,
					      SLOT(deletePresetDialog()));
  QAction* loadItem = presetMenu->addAction(tr("Load preset"), this,
					    SLOT(loadPresetDialog()));
  QAction* saveItem = presetMenu->addAction(tr("Save preset"), this,
					    SLOT(savePresetDialog()));
  if(!pre || !presetListView->isItemSelected(pre)) {
    deleteItem->setEnabled(false);
    saveItem->setEnabled(false);
  }
  if(!subcategoryListView->currentItem() ||
     !subcategoryListView->isItemSelected(subcategoryListView->currentItem())){
    newItem->setEnabled(false);
    loadItem->setEnabled(false);	
  }	
  presetMenu->exec(QCursor::pos());
  delete presetMenu;   // Tim.
}

//-----------------------------------------------------------
// newCategoryDialog
//-----------------------------------------------------------
void DeicsOnzeGui::newCategoryDialog() {
    int nhbank=_deicsOnze->_set->firstFreeHBank();
    if(nhbank==-1)
	QMessageBox::information(this,
				 tr("No more category supported"),
				 tr("You can not add more categories"),
				 QMessageBox::Ok);
    else {
	new Category(_deicsOnze->_set, "NEWCAT", nhbank);
	setSet();
	QTreeWidgetItem* ci=
	    categoryListView->findItems(num3Digits(nhbank+1), Qt::MatchExactly).at(0);
	categoryListView->setItemSelected(ci, true);
	categoryListView->setCurrentItem(ci);
	setCategory(ci);
	categoryListView->scrollToItem(ci);
    }
}

//-----------------------------------------------------------
// deleteCategoryDialog
//-----------------------------------------------------------
void DeicsOnzeGui::deleteCategoryDialog() {
  QTreeCategory* cat = (QTreeCategory*) categoryListView->currentItem();
  if(cat && categoryListView->isItemSelected(cat)) {
    if(!QMessageBox::question(
			      this,
			      tr("Delete category"),
			      tr("Do you really want to delete %1 ?")
			      .arg(cat->_category->_categoryName.c_str()),
			      tr("&Yes"), tr("&No"),
			      QString::null, 0, 1 ))
      {
	for(int c = 0; c < NBRCHANNELS; c++)
	  _deicsOnze->_preset[c]=_deicsOnze->_initialPreset;
	delete(cat->_category);
	delete(cat);
	subcategoryListView->clear();
	presetListView->clear();
	updateCategoryName("NONE", false);
	hbankSpinBox->setEnabled(false);
	updatePreset();
	updateSubcategoryName("NONE", false);
	progSpinBox->setEnabled(false);
	updatePresetName("INITVOICE", false);
      }
  }
  else QMessageBox::warning(this, tr("No category selected"),
			    tr("You must first select a category."));
}

//-----------------------------------------------------------
// loadCategoryDialog
//-----------------------------------------------------------
void DeicsOnzeGui::loadCategoryDialog() {
  QString buffstr;
  QString fileName =
    QFileDialog::getOpenFileName(
				 this,
				 tr("Load category dialog"),
				 lastDir,
				 QString("*.dec"));
  
  // read the XML file and create DOM tree
  if(!fileName.isEmpty()) {
    QFileInfo fi(fileName);
    lastDir = fi.path();
    QFile deicsonzeFile(fileName);
    if(!deicsonzeFile.open(QIODevice::ReadOnly)) {
      QMessageBox::critical(0,
			    tr("Critical Error"),
			    tr("Cannot open file %1").arg(fileName));
      return;
    }
    QDomDocument domTree;
    if (!domTree.setContent(&deicsonzeFile )) {
      QMessageBox::critical
	(0, tr("Critical Error"),
	 tr("Parsing error for file %1").arg(fileName));
      deicsonzeFile.close();
      return;
    }
    deicsonzeFile.close();
    
    QDomNode node = domTree.documentElement();
    while (!node.isNull()) {
      QDomElement e = node.toElement();
      if (e.isNull())
	continue;
      if (e.tagName() == "deicsOnzeCategory") {
	QString version = e.attribute(QString("version"));
	if (version == "1.0") {
	  Category* lCategory = new Category();
	  lCategory->readCategory(node.firstChild());
	  if (!_deicsOnze->_set->isFreeHBank(lCategory->_hbank)) {
	    if(!QMessageBox::question(
				      this,
				      tr("Replace or add"),
				      tr("%1 is supposed to be affected to the hbank number %2, but there is already one on this slot.\n Do you want to replace it or to add it in the next free slot ?")
				      .arg((lCategory->_categoryName).c_str())
				      .arg(buffstr.setNum(lCategory->_hbank+1)),
				      tr("&Replace"), tr("&Add"),
				      QString::null, 0, 1 )) {
	      delete(_deicsOnze->_set
		     ->findCategory(lCategory->_hbank));
	      lCategory->linkSet(_deicsOnze->_set);
	    }
	    else {
	      int ffhb=_deicsOnze->_set->firstFreeHBank();
	      if(ffhb==-1)
		QMessageBox::warning
		  (this, tr("Download error"),
		   tr("There is no more free category slot."));
	      else lCategory->_hbank=ffhb;
	      lCategory->linkSet(_deicsOnze->_set);
	    }
	  }
	  else lCategory->linkSet(_deicsOnze->_set);
	  //display category
	  setSet();
	}
	else printf("unsupported *.dec file version %s\n",
		    version.toLatin1().constData());
      }
      else printf("DeicsOnze: %s not supported\n",
		  e.tagName().toLatin1().constData());
      node = node.nextSibling();
    }
  }
}

//-----------------------------------------------------------
// saveCategoryDialog
//-----------------------------------------------------------
void DeicsOnzeGui::saveCategoryDialog() {
  QTreeCategory* cat = (QTreeCategory*) categoryListView->currentItem();
  if(cat) {
    QString filename =
      QFileDialog::getSaveFileName(
				   this,
				   tr("Save category dialog"),
				   lastDir,
				   QString("*.dec"));

    if(!filename.isEmpty()) {
      QFileInfo fi(filename);
      lastDir = fi.path();
      if(!filename.endsWith(".dec")) filename+=".dec";
      QFile f(filename);
      f.open(QIODevice::WriteOnly);
      AL::Xml* xml = new AL::Xml(&f);
      xml->header();
      cat->_category->writeCategory(xml, false);

      f.close();
    }
  }
  else QMessageBox::warning(this, tr("No category selected"),
			    tr("You must first select a category."));
}

//-----------------------------------------------------------
// newSubcategoryDialog
//-----------------------------------------------------------
void DeicsOnzeGui::newSubcategoryDialog() {
  QTreeCategory* cat = (QTreeCategory*) categoryListView->currentItem();
  if(cat && categoryListView->isItemSelected(cat)) {
    int nlbank=cat->_category->firstFreeLBank();
    if(nlbank==-1)
      QMessageBox::information(this,
			       tr("No more subcategory supported"),
			       tr("You can not add more subcategories"),
			       QMessageBox::Ok);
    else {
      new Subcategory(cat->_category, "NEWSUB", nlbank);
      setCategory(cat);
      QTreeWidgetItem* si=
	subcategoryListView->findItems(num3Digits(nlbank+1),
				       Qt::MatchExactly).at(0);
      subcategoryListView->setItemSelected(si, true);
      subcategoryListView->setCurrentItem(si);
      setSubcategory(si);
      subcategoryListView->scrollToItem(si);
    }
  }
}

//-----------------------------------------------------------
// deleteSubcategoryDialog
//-----------------------------------------------------------
void DeicsOnzeGui::deleteSubcategoryDialog() {
  QTreeSubcategory* sub =
    (QTreeSubcategory*) subcategoryListView->currentItem();
  if(sub && subcategoryListView->isItemSelected(sub)) {
    if(!QMessageBox::question(
			      this,
			      tr("Delete subcategory"),
			      tr("Do you really want to delete %1 ?")
			      .arg(sub->_subcategory
				   ->_subcategoryName.c_str()),
			      tr("&Yes"), tr("&No"),
			      QString::null, 0, 1 )) {
      	for(int c = 0; c < NBRCHANNELS; c++)
	  _deicsOnze->_preset[c]=_deicsOnze->_initialPreset;
      delete(sub->_subcategory);
      delete(sub);
      presetListView->clear();
      //subcategoryLineEdit->setEnabled(false);
      updateSubcategoryName("NONE", false);
      lbankSpinBox->setEnabled(false);
      updatePreset();
    }
  }
  else QMessageBox::warning(this, tr("No subcategory selected"),
			    tr("You must first select a subcategory."));
}

//-----------------------------------------------------------
// loadSubcategoryDialog
//-----------------------------------------------------------
void DeicsOnzeGui::loadSubcategoryDialog() {
  QTreeCategory* cat = (QTreeCategory*) categoryListView->currentItem();
  QString buffstr;
  QString fileName =
    QFileDialog::getOpenFileName(
				 this,
				 tr("Load subcategory dialog"),
				 lastDir,
				 QString("*.des"));

  // read the XML file and create DOM tree
  if(!fileName.isEmpty()) {
    QFileInfo fi(fileName);
    lastDir = fi.path();
    QFile deicsonzeFile(fileName);
    if(!deicsonzeFile.open(QIODevice::ReadOnly)) {
      QMessageBox::critical(0,
			    tr("Critical Error"),
			    tr("Cannot open file %1").arg(fileName));
      return;
    }
    QDomDocument domTree;
    if (!domTree.setContent(&deicsonzeFile )) {
      QMessageBox::critical
	(0, tr("Critical Error"),
	 tr("Parsing error for file %1").arg(fileName));
      deicsonzeFile.close();
      return;
    }
    deicsonzeFile.close();

    QDomNode node = domTree.documentElement();
    while (!node.isNull()) {
      QDomElement e = node.toElement();
      if (e.isNull())
	continue;
      if (e.tagName() == "deicsOnzeSubcategory") {
	QString version = e.attribute(QString("version"));
	if (version == "1.0") {
	  Subcategory* lSubcategory = new Subcategory();
	  lSubcategory->readSubcategory(node.firstChild());
	  if (!cat->_category->isFreeLBank(lSubcategory->_lbank)) {
	    if(!QMessageBox::question(
				      this,
				      tr("Replace or add"),
				      tr("%1 is supposed to be affected to the lbank number %2, but there is already one on this slot.\n Do you want to replace it or to add it in the next free slot ?")
				      .arg((lSubcategory->_subcategoryName)
					   .c_str())
				      .arg(buffstr.setNum(lSubcategory->_lbank+1)),
				      tr("&Replace"), tr("&Add"),
				      QString::null, 0, 1 )) {
	      delete(cat->_category->findSubcategory(lSubcategory->_lbank));
	      lSubcategory->linkCategory(cat->_category);
	    }
	    else {
	      int fflb=cat->_category->firstFreeLBank();
	      if(fflb==-1)
		QMessageBox::warning
		  (this, tr("Download error"),
		   tr("There is no more free subcategory slot."));
	      else lSubcategory->_lbank=fflb;
	      lSubcategory->linkCategory(cat->_category);
	    }
	  }
	  else lSubcategory->linkCategory(cat->_category);
	  //display subcategory
	  setCategory(cat);
	}
	else printf("unsupported *.des file version %s\n",
		    version.toLatin1().constData());
      }
      else printf("DeicsOnze: %s not supported\n",
		  e.tagName().toLatin1().constData());
      node = node.nextSibling();
    }
  }
}

//-----------------------------------------------------------
// saveSubcategoryDialog
//-----------------------------------------------------------
void DeicsOnzeGui::saveSubcategoryDialog() {
  QTreeSubcategory* sub =
    (QTreeSubcategory*) subcategoryListView->currentItem();
  if(sub) {
    QString filename =
      QFileDialog::getSaveFileName(
				   this,
				   tr("Save subcategory dialog"),
				   lastDir,
				   QString("*.des"));

    if(!filename.isEmpty()) {
      QFileInfo fi(filename);
      lastDir = fi.path();
      if(!filename.endsWith(".des")) filename+=".des";
      QFile f(filename);
      f.open(QIODevice::WriteOnly);

      AL::Xml* xml = new AL::Xml(&f);
      xml->header();
      sub->_subcategory->writeSubcategory(xml, false);

      f.close();
    }
  }
  else QMessageBox::warning(this, tr("No subcategory selected"),
			    tr("You must first select a subcategory."));
}


//-----------------------------------------------------------
// newPresetDialog
//-----------------------------------------------------------
void DeicsOnzeGui::newPresetDialog() {
  QTreeSubcategory* sub =
    (QTreeSubcategory*) subcategoryListView->currentItem();
  if(sub && subcategoryListView->isItemSelected(sub)) {
    int nprog=sub->_subcategory->firstFreeProg();
    if(nprog==-1)
      QMessageBox::information(this,
			       tr("No more preset supported"),
			       tr("You can not add more presets"),
			       QMessageBox::Ok);
    else {
      new Preset(sub->_subcategory, nprog);
      setSubcategory(sub);
      QTreeWidgetItem* pi=
	presetListView->findItems(num3Digits(nprog+1),
				  Qt::MatchExactly).at(0);
      presetListView->setItemSelected(pi, true);
      presetListView->setCurrentItem(pi);
      setPreset(pi);
      presetListView->scrollToItem(pi);
    }
  }
}

//-----------------------------------------------------------
// deletePresetDialog
//-----------------------------------------------------------
void DeicsOnzeGui::deletePresetDialog() {
  QTreePreset* pre = (QTreePreset*) presetListView->currentItem();
  if(pre) {
    if(presetListView->isItemSelected(pre)) {
      if(!QMessageBox::question(
				this,
				tr("Delete preset"),
				tr("Do you really want to delete %1 ?")
				.arg(pre->_preset->name.c_str()),
				tr("&Yes"), tr("&No"),
				QString::null, 0, 1 )) {
	for(int c = 0; c < NBRCHANNELS; c++)
	  _deicsOnze->_preset[c]=_deicsOnze->_initialPreset;
	delete(pre->_preset);
	delete(pre);
	presetLineEdit->setEnabled(false);
	progSpinBox->setEnabled(false);
	updatePreset();
      }
    }
    else QMessageBox::warning(this, tr("No preset selected"),
			      tr("You must first select a preset."));
  }
  else QMessageBox::warning(this, tr("No preset selected"),
			    tr("You must first select a preset."));
}

//-----------------------------------------------------------
// loadPresetDialog
//-----------------------------------------------------------
void DeicsOnzeGui::loadPresetDialog() {
  QTreeSubcategory* sub =
    (QTreeSubcategory*) subcategoryListView->currentItem();
  QString buffstr;
  QString fileName =
    QFileDialog::getOpenFileName(
				 this,
				 tr("Load preset dialog"),
				 lastDir,
				 QString("*.dep"));

  // read the XML file and create DOM tree
  if(!fileName.isEmpty()) {
    QFileInfo fi(fileName);
    lastDir = fi.path();
    QFile deicsonzeFile(fileName);
    if(!deicsonzeFile.open(QIODevice::ReadOnly)) {
      QMessageBox::critical(0,
			    tr("Critical Error"),
			    tr("Cannot open file %1").arg(fileName));
      return;
    }
    QDomDocument domTree;
    if (!domTree.setContent(&deicsonzeFile )) {
      QMessageBox::critical
	(0, tr("Critical Error"),
	 tr("Parsing error for file %1").arg(fileName));
      deicsonzeFile.close();
      return;
    }
    deicsonzeFile.close();

    QDomNode node = domTree.documentElement();
    while (!node.isNull()) {
      QDomElement e = node.toElement();
      if(e.isNull())
	continue;
      if(e.tagName() == "deicsOnzePreset") {
	QString version = e.attribute(QString("version"));
	if (version == "1.0") {
	  Preset* lPreset = new Preset();
	  lPreset->readPreset(node.firstChild());
	  if(!sub->_subcategory->isFreeProg(lPreset->prog)) {
	    if(!QMessageBox::question(
				      this,
				      tr("Replace or add"),
				      tr("%1 is supposed to be affected to the prog number %2, but there is already one on this slot.\n Do you want to replace it or to add it in the next free slot ?")
				      .arg((lPreset->name).c_str())
				      .arg(buffstr.setNum(lPreset->prog+1)),
				      tr("&Replace"), tr("&Add"),
				      QString::null, 0, 1 )) {
	      delete(sub->_subcategory->findPreset(lPreset->prog));
	      lPreset->linkSubcategory(sub->_subcategory);
	    }
	    else {
	      int ffp=sub->_subcategory->firstFreeProg();
	      if(ffp==-1)
		QMessageBox::warning
		  (this, tr("Download error"),
		   tr("There is no more free preset slot."));
	      else lPreset->prog=ffp;
	      lPreset->linkSubcategory(sub->_subcategory);
	    }
	  }
	  else lPreset->linkSubcategory(sub->_subcategory);
	  //display preset
	  setSubcategory(sub->_subcategory);
	}
	else printf("unsupported *.dep file version %s\n",
		    version.toLatin1().constData());
      }
      else printf("DeicsOnze: %s not supported\n",
		  e.tagName().toLatin1().constData());
      node = node.nextSibling();
    }
  }
}


//-----------------------------------------------------------
// savePresetDialog
//-----------------------------------------------------------
void DeicsOnzeGui::savePresetDialog() {
  QTreePreset* pre = (QTreePreset*) presetListView->currentItem();
  if(pre) {
    QString filename =
      QFileDialog::getSaveFileName(
				   this,
				   tr("Save preset dialog"),
				   lastDir,
				   QString("*.dep"));
    if(!filename.isEmpty()) {
      QFileInfo fi(filename);
      lastDir = fi.path();
      if(!filename.endsWith(".dep")) filename+=".dep";
      QFile f(filename);
      f.open(QIODevice::WriteOnly);
      AL::Xml* xml = new AL::Xml(&f);

      xml->header();
      pre->_preset->writePreset(xml, false);
      f.close();
    }
  }
  else QMessageBox::warning(this, tr("No preset selected"),
			    tr("You must first select a preset."));
}

//-----------------------------------------------------------
// Preset and bank
//-----------------------------------------------------------
void DeicsOnzeGui::setPresetName(const QString& n) {
  QTreeWidgetItem* pre = presetListView->currentItem();
  if(pre) {
    //TODO : must be changed with SysEx
    _deicsOnze->_preset[_currentChannel]->name = n.toAscii().data();
    pre->setText(1,n.toAscii().data());
  }
}
void DeicsOnzeGui::setSubcategoryName(const QString& s) {
  QTreeSubcategory* sub =
    (QTreeSubcategory*) subcategoryListView->currentItem();
  if(sub) {//must be changed with SysEx
    sub->_subcategory->_subcategoryName=s.toAscii().data();
    sub->setText(1, s.toAscii().data());
  }
}
void DeicsOnzeGui::setCategoryName(const QString& c) {
  QTreeCategory* cat = (QTreeCategory*) categoryListView->currentItem();
  if(cat) {//must be changed with SysEx
    cat->_category->_categoryName=c.toAscii().data();
    cat->setText(1, c.toAscii().data());
  }
}
void DeicsOnzeGui::setHBank(int hb) {
  QTreeCategory* cat = (QTreeCategory*) categoryListView->currentItem();
  if(cat) {//must be changed with SysEx
    if(!_deicsOnze->_set->isFreeHBank(hb-1)) {
      QTreeWidgetItem* qcat;
      qcat=categoryListView->findItems(num3Digits(hb), Qt::MatchExactly).at(0);
      ((QTreeCategory*)qcat)->_category->_hbank=
	cat->_category->_hbank;
      qcat->setText(0, num3Digits(((QTreeCategory*)qcat)
				  ->_category->_hbank+1));
    }
    cat->_category->_hbank=hb-1;
    cat->setText(0,num3Digits(hb));
    categoryListView->sortItems(0, Qt::AscendingOrder);
    categoryListView->scrollToItem(cat);
  }
}
void DeicsOnzeGui::setLBank(int lb) {//must be changed with SysEx
  QTreeSubcategory* sub =
    (QTreeSubcategory*) subcategoryListView->currentItem();
  if(sub) {
    Category* currentCat=sub->_subcategory->_category;
    if(!currentCat->isFreeLBank(lb-1)) {
      QTreeWidgetItem* qsub;
      qsub=subcategoryListView->findItems(num3Digits(lb),
					  Qt::MatchExactly).at(0);
      ((QTreeSubcategory*)qsub)->_subcategory->_lbank=
	sub->_subcategory->_lbank;
      qsub->setText(0, num3Digits(((QTreeSubcategory*)qsub)
				  ->_subcategory->_lbank+1));
    }
    sub->_subcategory->_lbank=lb-1;
    sub->setText(0,num3Digits(lb));
    subcategoryListView->sortItems(0, Qt::AscendingOrder);
    subcategoryListView->scrollToItem(sub);
  }
}
void DeicsOnzeGui::setProg(int pr) {//must be changed with SysEx
  QTreePreset* pre = (QTreePreset*) presetListView->currentItem();
  if(pre) {
    Subcategory* currentSub=pre->_preset->_subcategory;
    if(!currentSub->isFreeProg(pr-1)) {
      QTreeWidgetItem* qpre;
      qpre=presetListView->findItems(num3Digits(pr), Qt::MatchExactly).at(0);
      ((QTreePreset*)qpre)->_preset->prog=pre->_preset->prog;
      qpre->setText(0, num3Digits(((QTreePreset*)qpre)->_preset->prog+1));
    }
    pre->_preset->prog=pr-1;
    pre->setText(0,num3Digits(pr));
    presetListView->sortItems(0,Qt::AscendingOrder);
    presetListView->scrollToItem(pre);
  }
}
//-----------------------------------------------------------
// load init set
//-----------------------------------------------------------
void DeicsOnzeGui::setIsInitSet(bool b) {
  initSetPathLineEdit->setEnabled(b);
  initSetBrowsePushButton->setEnabled(b);
  unsigned char message[4];
  message[0]=MUSE_SYNTH_SYSEX_MFG_ID;
  message[1]=DEICSONZE_UNIQUE_ID;
  message[2]=SYSEX_ISINITSET;
  message[3]=(unsigned char)b;
  sendSysex(message, 4);
}
void DeicsOnzeGui::setInitSetPath(const QString& s) {
  unsigned char message[3+MAXSTRLENGTHINITSETPATH];
  message[0]=MUSE_SYNTH_SYSEX_MFG_ID;
  message[1]=DEICSONZE_UNIQUE_ID;
  message[2]=SYSEX_INITSETPATH;
  strncpy((char*)&message[3], s.toAscii().data(), MAXSTRLENGTHINITSETPATH);
  sendSysex(message, 3+MAXSTRLENGTHINITSETPATH);
}
void DeicsOnzeGui::setBrowseInitSetPath() {
  QString fileName =
    QFileDialog::getOpenFileName(
				 this,
				 tr("Browse set dialog"),
				 lastDir,
				 QString("*.dei"));
  if(!fileName.isEmpty()) {
    QFileInfo fi(fileName);
    lastDir = fi.path();
    updateInitSetPath(fileName);
    setInitSetPath(fileName);
  }
}
//-----------------------------------------------------------
// load background pix
//-----------------------------------------------------------
void DeicsOnzeGui::setIsBackgroundPix(bool b) {
  if(b && !imagePathLineEdit->text().isEmpty()) applyBackgroundPix();
  else setBackgroundColor(reinterpret_cast<const QColor &>(*bColor));
  imagePathLineEdit->setEnabled(b);
  imageBrowsePushButton->setEnabled(b);
  unsigned char message[4];
  message[0]=MUSE_SYNTH_SYSEX_MFG_ID;
  message[1]=DEICSONZE_UNIQUE_ID;
  message[2]=SYSEX_ISBACKGROUNDPIX;
  message[3]=(unsigned char)b;
  sendSysex(message, 4);
}
void DeicsOnzeGui::setBackgroundPixPath(const QString& s) {
  applyBackgroundPix();
  unsigned char message[3+MAXSTRLENGTHBACKGROUNDPIXPATH];
  message[0]=MUSE_SYNTH_SYSEX_MFG_ID;
  message[1]=DEICSONZE_UNIQUE_ID;
  message[2]=SYSEX_BACKGROUNDPIXPATH;
  strncpy((char*)&message[3], s.toAscii().data(),
	  MAXSTRLENGTHBACKGROUNDPIXPATH);
  sendSysex(message, 3+MAXSTRLENGTHBACKGROUNDPIXPATH);
}
void DeicsOnzeGui::setBrowseBackgroundPixPath() {
  QString fileName =
    QFileDialog::getOpenFileName(
				 this,
				 tr("Browse image dialog"),
				 lastDir,
				 QString("*.jpg *.png"));
  if(!fileName.isEmpty()) {
    QFileInfo fi(fileName);
    lastDir = fi.path();
    updateBackgroundPixPath(fileName);
    setBackgroundPixPath(fileName);
  }
}
//-----------------------------------------------------------
// FX
//-----------------------------------------------------------
void DeicsOnzeGui::setChorusActiv(bool a) {
  unsigned char message[4];
  message[0]=MUSE_SYNTH_SYSEX_MFG_ID;
  message[1]=DEICSONZE_UNIQUE_ID;
  message[2]=SYSEX_CHORUSACTIV;
  message[3]=(unsigned char)a;
  sendSysex(message, 4);  
}
void DeicsOnzeGui::setChannelChorus(int c) {
  sendController(_currentChannel, MusECore::CTRL_CHORUS_SEND, c);
}
void DeicsOnzeGui::setChorusReturn(int val) {
  unsigned char message[4];
  message[0]=MUSE_SYNTH_SYSEX_MFG_ID;
  message[1]=DEICSONZE_UNIQUE_ID;
  message[2]=SYSEX_CHORUSRETURN;
  message[3]=(unsigned char)val;
  sendSysex(message, 4);
}
void DeicsOnzeGui::setSelectChorusPlugin() {
  MusECore::Plugin* pluginChorus = MusEGui::PluginDialog::getPlugin(this);
  if(pluginChorus) {
    unsigned char message[3+sizeof(MusECore::Plugin*)];
    message[0]=MUSE_SYNTH_SYSEX_MFG_ID;
    message[1]=DEICSONZE_UNIQUE_ID;
    message[2]=SYSEX_SELECTCHORUS;
    memcpy(&message[3], &pluginChorus, sizeof(MusECore::Plugin*));
    sendSysex(message, 3+sizeof(MusECore::Plugin*));
  }
}
/*void DeicsOnzeGui::setPanChorus1(double i) {
  unsigned char message[4];
  message[0]=MUSE_SYNTH_SYSEX_MFG_ID;
  message[1]=DEICSONZE_UNIQUE_ID;
  message[2]=SYSEX_CHORUS1PAN;
  message[3]=(unsigned char)(i*(double)MAXCHORUSPARAM);
  sendSysex(message, 4);  
}
void DeicsOnzeGui::setLFOFreqChorus1(double i) {
  unsigned char message[4];
  message[0]=MUSE_SYNTH_SYSEX_MFG_ID;
  message[1]=DEICSONZE_UNIQUE_ID;
  message[2]=SYSEX_CHORUS1LFOFREQ;
  message[3]=(unsigned char)(i*(double)MAXCHORUSPARAM);
  sendSysex(message, 4);  
}
void DeicsOnzeGui::setDepthChorus1(double i) {
  unsigned char message[4];
  message[0]=MUSE_SYNTH_SYSEX_MFG_ID;
  message[1]=DEICSONZE_UNIQUE_ID;
  message[2]=SYSEX_CHORUS1DEPTH;
  message[3]=(unsigned char)(i*(double)MAXCHORUSPARAM);
  sendSysex(message, 4);  
}
void DeicsOnzeGui::setPanChorus2(double i) {
  unsigned char message[4];
  message[0]=MUSE_SYNTH_SYSEX_MFG_ID;
  message[1]=DEICSONZE_UNIQUE_ID;
  message[2]=SYSEX_CHORUS2PAN;
  message[3]=(unsigned char)(i*(double)MAXCHORUSPARAM);
  sendSysex(message, 4);  
}
void DeicsOnzeGui::setLFOFreqChorus2(double i) {
  unsigned char message[4];
  message[0]=MUSE_SYNTH_SYSEX_MFG_ID;
  message[1]=DEICSONZE_UNIQUE_ID;
  message[2]=SYSEX_CHORUS2LFOFREQ;
  message[3]=(unsigned char)(i*(double)MAXCHORUSPARAM);
  sendSysex(message, 4);  
}
void DeicsOnzeGui::setDepthChorus2(double i) {
  unsigned char message[4];
  message[0]=MUSE_SYNTH_SYSEX_MFG_ID;
  message[1]=DEICSONZE_UNIQUE_ID;
  message[2]=SYSEX_CHORUS2DEPTH;
  message[3]=(unsigned char)(i*(double)MAXCHORUSPARAM);
  sendSysex(message, 4);  
  }*/
void DeicsOnzeGui::setReverbActiv(bool a) {
  unsigned char message[4];
  message[0]=MUSE_SYNTH_SYSEX_MFG_ID;
  message[1]=DEICSONZE_UNIQUE_ID;
  message[2]=SYSEX_REVERBACTIV;
  message[3]=(unsigned char)a;
  sendSysex(message, 4);
}
void DeicsOnzeGui::setChannelReverb(int r) {
  sendController(_currentChannel, MusECore::CTRL_REVERB_SEND, r);
}
void DeicsOnzeGui::setReverbReturn(int val) {
  unsigned char message[4];
  message[0]=MUSE_SYNTH_SYSEX_MFG_ID;
  message[1]=DEICSONZE_UNIQUE_ID;
  message[2]=SYSEX_REVERBRETURN;
  message[3]=(unsigned char)val;
  sendSysex(message, 4);
}
void DeicsOnzeGui::setSelectReverbPlugin() {
  MusECore::Plugin* pluginReverb = MusEGui::PluginDialog::getPlugin(this);
  if(pluginReverb) {
    unsigned char message[3+sizeof(MusECore::Plugin*)];
    message[0]=MUSE_SYNTH_SYSEX_MFG_ID;
    message[1]=DEICSONZE_UNIQUE_ID;
    message[2]=SYSEX_SELECTREVERB;
    memcpy(&message[3], &pluginReverb, sizeof(MusECore::Plugin*));
    sendSysex(message, 3+sizeof(MusECore::Plugin*));
  }
}

//-----------------------------------------------------------
// Quick Edit
//-----------------------------------------------------------
void DeicsOnzeGui::setChannelVolKnob(double val) {
  sendController(_currentChannel, CTRL_CHANNELVOLUME,
		 (int)(val*(double)MAXCHANNELVOLUME));
}
void DeicsOnzeGui::setBrightnessKnob(double val) {
  sendController(_currentChannel, CTRL_FINEBRIGHTNESS,
		 (int)(val*(double)MAXFINEBRIGHTNESS));
}
void DeicsOnzeGui::setModulationKnob(double val) {
  sendController(_currentChannel, MusECore::CTRL_MODULATION,
		 (int)(val*(double)MAXMODULATION));
}
void DeicsOnzeGui::setDetuneKnob(double val) {
  //channelDetuneSlider->setValue((int)((2.0*val-1.0)*(double)MAXCHANNELDETUNE));
  setChannelDetune((int)((2.0*val-1.0)*(double)MAXCHANNELDETUNE));
}
void DeicsOnzeGui::setAttackKnob(double val) {
  sendController(_currentChannel, MusECore::CTRL_ATTACK_TIME,
		 (int)(val*(double)MAXATTACK));
}
void DeicsOnzeGui::setReleaseKnob(double val) {
  sendController(_currentChannel, MusECore::CTRL_RELEASE_TIME, (int)(val*(double)MAXRELEASE));
}
//-----------------------------------------------------------
// Global control
//-----------------------------------------------------------
void DeicsOnzeGui::setChannelPan(double mv) {
  sendController(_currentChannel, CTRL_CHANNELPAN,
		 (int)((mv-0.5)*2*(double)MAXCHANNELPAN));
}
void DeicsOnzeGui::setMasterVolKnob(double mv) {
  setMasterVol((int)(mv*(double)MAXMASTERVOLUME));
}
void DeicsOnzeGui::setMasterVol(int mv) {
  unsigned char message[4];
  message[0]=MUSE_SYNTH_SYSEX_MFG_ID;
  message[1]=DEICSONZE_UNIQUE_ID;
  message[2]=SYSEX_MASTERVOL;
  message[3]=(unsigned char)mv;
  sendSysex(message, 4);
}

void DeicsOnzeGui::setFeedback(int f) {sendController(_currentChannel, CTRL_FEEDBACK, f);}

void DeicsOnzeGui::setLfoWave(int lw) {sendController(_currentChannel, CTRL_WAVE, lw);}

void DeicsOnzeGui::setLfoSpeed(int ls) {sendController(_currentChannel, CTRL_SPEED, ls);}

void DeicsOnzeGui::setLfoDelay(int ld) {sendController(_currentChannel, CTRL_DELAY, ld);}

void DeicsOnzeGui::setLfoPModDepth(int lpmd) {
  sendController(_currentChannel, CTRL_PMODDEPTH, lpmd);
}

void DeicsOnzeGui::setLfoPitchSens(int lps) {
  sendController(_currentChannel, CTRL_PMODSENS, lps);
}

void DeicsOnzeGui::setLfoAModDepth(int lamd) {
  sendController(_currentChannel, CTRL_AMODDEPTH, lamd);
}
void DeicsOnzeGui::setLfoAmpSens(int las) {sendController(_currentChannel, CTRL_AMS, las);}

void DeicsOnzeGui::setTranspose(int t) {sendController(_currentChannel, CTRL_TRANSPOSE, t);}

void DeicsOnzeGui::setChannelDetune(int d) {
  sendController(_currentChannel, CTRL_CHANNELDETUNE, d);
  updateChannelDetuneKnob(d);
}

void DeicsOnzeGui::setAlgorithm(int a) {
    sendController(_currentChannel, CTRL_ALG, (int) (a==0?FIRST:
				       (a==1?SECOND:
					(a==2?THIRD:
					 (a==3?FOURTH:
					  (a==4?FIFTH:
					   (a==5?SIXTH:
					    (a==6?SEVENTH:EIGHTH))))))));
}

void DeicsOnzeGui::setPitchBendRange(int pbr) {
    sendController(_currentChannel, CTRL_PBENDRANGE, pbr);
}
	
//---------------------------------------------------------------
// Pitch Envelope
//---------------------------------------------------------------
void DeicsOnzeGui::setPL1(int val) {
  PitchEg* pe=&_deicsOnze->_preset[_currentChannel]->pitchEg;
  pitchEnvelopeGraph->env2Points(val, pe->pl2, pe->pl3,
				 pe->pr1, pe->pr2, pe->pr3);
  pitchEnvelopeGraph->updateEnv();
  sendController(_currentChannel, CTRL_PL1, val);
}
void DeicsOnzeGui::setPL2(int val) {
  PitchEg* pe=&_deicsOnze->_preset[_currentChannel]->pitchEg;
  pitchEnvelopeGraph->env2Points(pe->pl1, val, pe->pl3,
				 pe->pr1, pe->pr2, pe->pr3);
  pitchEnvelopeGraph->updateEnv();
  sendController(_currentChannel, CTRL_PL2, val);
}
void DeicsOnzeGui::setPL3(int val) {
  PitchEg* pe=&_deicsOnze->_preset[_currentChannel]->pitchEg;
  pitchEnvelopeGraph->env2Points(pe->pl1, pe->pl2, val,
				 pe->pr1, pe->pr2, pe->pr3);
  pitchEnvelopeGraph->updateEnv();
  sendController(_currentChannel, CTRL_PL3, val);
}
void DeicsOnzeGui::setPR1(int val) {
  PitchEg* pe=&_deicsOnze->_preset[_currentChannel]->pitchEg;
  pitchEnvelopeGraph->env2Points(pe->pl1, pe->pl2, pe->pl3,
				 val, pe->pr2, pe->pr3);
  pitchEnvelopeGraph->updateEnv();
  sendController(_currentChannel, CTRL_PR1, val);
}
void DeicsOnzeGui::setPR2(int val) {
  PitchEg* pe=&_deicsOnze->_preset[_currentChannel]->pitchEg;
  pitchEnvelopeGraph->env2Points(pe->pl1, pe->pl2, pe->pl3,
				 pe->pr1, val, pe->pr3);
  pitchEnvelopeGraph->updateEnv();
  sendController(_currentChannel, CTRL_PR2, val);
}
void DeicsOnzeGui::setPR3(int val) {
  PitchEg* pe=&_deicsOnze->_preset[_currentChannel]->pitchEg;
  pitchEnvelopeGraph->env2Points(pe->pl1, pe->pl2, pe->pl3,
				 pe->pr1, pe->pr2, val);
  pitchEnvelopeGraph->updateEnv();
  sendController(_currentChannel, CTRL_PR3, val);
}
//---------------------------------------------------------------
// Function
//---------------------------------------------------------------
void DeicsOnzeGui::setFcVolume(int val){sendController(_currentChannel, CTRL_FCVOLUME, val);}
void DeicsOnzeGui::setFcPitch(int val){sendController(_currentChannel, CTRL_FCPITCH, val);}
void DeicsOnzeGui::setFcAmplitude(int val) {
  sendController(_currentChannel, CTRL_FCAMPLITUDE, val);
}
void DeicsOnzeGui::setMwPitch(int val){sendController(_currentChannel, CTRL_MWPITCH, val);}
void DeicsOnzeGui::setMwAmplitude(int val) {
  sendController(_currentChannel, CTRL_MWAMPLITUDE, val);
}
void DeicsOnzeGui::setBcPitch(int val){sendController(_currentChannel, CTRL_BCPITCH, val);}
void DeicsOnzeGui::setBcAmplitude(int val) {
  sendController(_currentChannel, CTRL_BCAMPLITUDE, val);
}
void DeicsOnzeGui::setBcPitchBias(int val) {
  sendController(_currentChannel, CTRL_BCPITCHBIAS, val);}
void DeicsOnzeGui::setBcEgBias(int val) {
  sendController(_currentChannel, CTRL_BCEGBIAS, val);
}
void DeicsOnzeGui::setAtPitch(int val){sendController(_currentChannel, CTRL_ATPITCH, val);}
void DeicsOnzeGui::setAtAmplitude(int val) {
  sendController(_currentChannel, CTRL_ATAMPLITUDE, val);
}
void DeicsOnzeGui::setAtPitchBias(int val) {
  sendController(_currentChannel, CTRL_ATPITCHBIAS, val);}
void DeicsOnzeGui::setAtEgBias(int val) {
  sendController(_currentChannel, CTRL_ATEGBIAS, val);
}
void DeicsOnzeGui::setReverbRate(int val) {
  sendController(_currentChannel, CTRL_REVERBRATE, val);
}
void DeicsOnzeGui::setPolyMode(int val) {
  sendController(_currentChannel, CTRL_POLYMODE, val);
}
void DeicsOnzeGui::setPortFingerFull(int val) {
  sendController(_currentChannel, CTRL_PORTAMODE, val);
}
void DeicsOnzeGui::setPortaTime(int val) {
  sendController(_currentChannel, CTRL_PORTATIME, val);
}

//---------------------------------------------------------------
// envelope controle
//---------------------------------------------------------------
void DeicsOnzeGui::setAR1(int val) {
  Eg* _eg=&(_deicsOnze->_preset[_currentChannel]->eg[0]);
  //printf("ar : %d, d1r : %d, d1l : %d, d2r : %d, rr : %d\n", 
  // val, _eg->d1r, _eg->d1l, _eg->d2r, _eg->rr);
  envelopeGraph[0]->env2Points(val, _eg->d1r, _eg->d1l, _eg->d2r, _eg->rr);
  envelopeGraph[0]->updateEnv();
  sendController(_currentChannel, CTRL_AR, val);
}
void DeicsOnzeGui::setD1R1(int val) {
    Eg* _eg=&(_deicsOnze->_preset[_currentChannel]->eg[0]);
    envelopeGraph[0]->env2Points(_eg->ar, val, _eg->d1l, _eg->d2r, _eg->rr);
    envelopeGraph[0]->updateEnv();
    sendController(_currentChannel, CTRL_D1R, val);
}
void DeicsOnzeGui::setD1L1(int val) {
    Eg* _eg=&(_deicsOnze->_preset[_currentChannel]->eg[0]);
    envelopeGraph[0]->env2Points(_eg->ar, _eg->d1r, val, _eg->d2r, _eg->rr);
    envelopeGraph[0]->updateEnv();
    sendController(_currentChannel, CTRL_D1L, val);
}
void DeicsOnzeGui::setD2R1(int val) {
    Eg* _eg=&(_deicsOnze->_preset[_currentChannel]->eg[0]);
    envelopeGraph[0]->env2Points(_eg->ar, _eg->d1r, _eg->d1l, val, _eg->rr);
    envelopeGraph[0]->updateEnv();
    sendController(_currentChannel, CTRL_D2R, val);
}
void DeicsOnzeGui::setRR1(int val){
    Eg* _eg=&(_deicsOnze->_preset[_currentChannel]->eg[0]);
    envelopeGraph[0]->env2Points(_eg->ar, _eg->d1r, _eg->d1l, _eg->d2r, val);
    envelopeGraph[0]->updateEnv();
    sendController(_currentChannel, CTRL_RR, val);
}
void DeicsOnzeGui::setAR2(int val) {
    Eg* _eg=&(_deicsOnze->_preset[_currentChannel]->eg[1]);
    envelopeGraph[1]->env2Points(val, _eg->d1r, _eg->d1l, _eg->d2r, _eg->rr);
    envelopeGraph[1]->updateEnv();
    sendController(_currentChannel, CTRL_AR+DECAPAR1, val);
}
void DeicsOnzeGui::setD1R2(int val) {
    Eg* _eg=&(_deicsOnze->_preset[_currentChannel]->eg[1]);
    envelopeGraph[1]->env2Points(_eg->ar, val, _eg->d1l, _eg->d2r, _eg->rr);
    envelopeGraph[1]->updateEnv();
    sendController(_currentChannel, CTRL_D1R+DECAPAR1, val);
}
void DeicsOnzeGui::setD1L2(int val) {
    Eg* _eg=&(_deicsOnze->_preset[_currentChannel]->eg[1]);
    envelopeGraph[1]->env2Points(_eg->ar, _eg->d1r, val, _eg->d2r, _eg->rr);
    envelopeGraph[1]->updateEnv();
    sendController(_currentChannel, CTRL_D1L+DECAPAR1, val);
}
void DeicsOnzeGui::setD2R2(int val) {
    Eg* _eg=&(_deicsOnze->_preset[_currentChannel]->eg[1]);
    envelopeGraph[1]->env2Points(_eg->ar, _eg->d1r, _eg->d1l, val, _eg->rr);
    envelopeGraph[1]->updateEnv();
    sendController(_currentChannel, CTRL_D2R+DECAPAR1, val);
}
void DeicsOnzeGui::setRR2(int val){
    Eg* _eg=&(_deicsOnze->_preset[_currentChannel]->eg[1]);
    envelopeGraph[1]->env2Points(_eg->ar, _eg->d1r, _eg->d1l, _eg->d2r, val);
    envelopeGraph[1]->updateEnv();
    sendController(_currentChannel, CTRL_RR+DECAPAR1, val);
}
void DeicsOnzeGui::setAR3(int val) {
    Eg* _eg=&(_deicsOnze->_preset[_currentChannel]->eg[2]);
    envelopeGraph[2]->env2Points(val, _eg->d1r, _eg->d1l, _eg->d2r, _eg->rr);
    envelopeGraph[2]->updateEnv();
    sendController(_currentChannel, CTRL_AR+2*DECAPAR1, val);
}
void DeicsOnzeGui::setD1R3(int val) {
    Eg* _eg=&(_deicsOnze->_preset[_currentChannel]->eg[2]);
    envelopeGraph[2]->env2Points(_eg->ar, val, _eg->d1l, _eg->d2r, _eg->rr);
    envelopeGraph[2]->updateEnv();
    sendController(_currentChannel, CTRL_D1R+2*DECAPAR1, val);
}
void DeicsOnzeGui::setD1L3(int val) {
    Eg* _eg=&(_deicsOnze->_preset[_currentChannel]->eg[2]);
    envelopeGraph[2]->env2Points(_eg->ar, _eg->d1r, val, _eg->d2r, _eg->rr);
    envelopeGraph[2]->updateEnv();
    sendController(_currentChannel, CTRL_D1L+2*DECAPAR1, val);
}
void DeicsOnzeGui::setD2R3(int val) {
    Eg* _eg=&(_deicsOnze->_preset[_currentChannel]->eg[2]);
    envelopeGraph[2]->env2Points(_eg->ar, _eg->d1r, _eg->d1l, val, _eg->rr);
    envelopeGraph[2]->updateEnv();
    sendController(_currentChannel, CTRL_D2R+2*DECAPAR1, val);
}
void DeicsOnzeGui::setRR3(int val){
    Eg* _eg=&(_deicsOnze->_preset[_currentChannel]->eg[2]);
    envelopeGraph[2]->env2Points(_eg->ar, _eg->d1r, _eg->d1l, _eg->d2r, val);
    envelopeGraph[2]->updateEnv();
    sendController(_currentChannel, CTRL_RR+2*DECAPAR1, val);
}
void DeicsOnzeGui::setAR4(int val) {
    Eg* _eg=&(_deicsOnze->_preset[_currentChannel]->eg[3]);
    envelopeGraph[3]->env2Points(val, _eg->d1r, _eg->d1l, _eg->d2r, _eg->rr);
    envelopeGraph[3]->updateEnv();
    sendController(_currentChannel, CTRL_AR+3*DECAPAR1, val);
}
void DeicsOnzeGui::setD1R4(int val) {
    Eg* _eg=&(_deicsOnze->_preset[_currentChannel]->eg[3]);
    envelopeGraph[3]->env2Points(_eg->ar, val, _eg->d1l, _eg->d2r, _eg->rr);
    envelopeGraph[3]->updateEnv();
    sendController(_currentChannel, CTRL_D1R+3*DECAPAR1, val);
}
void DeicsOnzeGui::setD1L4(int val) {
    Eg* _eg=&(_deicsOnze->_preset[_currentChannel]->eg[3]);
    envelopeGraph[3]->env2Points(_eg->ar, _eg->d1r, val, _eg->d2r, _eg->rr);
    envelopeGraph[3]->updateEnv();
    sendController(_currentChannel, CTRL_D1L+3*DECAPAR1, val);
}
void DeicsOnzeGui::setD2R4(int val) {
    Eg* _eg=&(_deicsOnze->_preset[_currentChannel]->eg[3]);
    envelopeGraph[3]->env2Points(_eg->ar, _eg->d1r, _eg->d1l, val, _eg->rr);
    envelopeGraph[3]->updateEnv();
    sendController(_currentChannel, CTRL_D2R+3*DECAPAR1, val);
}
void DeicsOnzeGui::setRR4(int val){
    Eg* _eg=&(_deicsOnze->_preset[_currentChannel]->eg[3]);
    envelopeGraph[3]->env2Points(_eg->ar, _eg->d1r, _eg->d1l, _eg->d2r, val);
    envelopeGraph[3]->updateEnv();
    sendController(_currentChannel, CTRL_RR+3*DECAPAR1, val);
}

//--------------------------------------------------------------
// set Scaling
//--------------------------------------------------------------
void DeicsOnzeGui::setLS1(int val){sendController(_currentChannel, CTRL_LS, val);}
void DeicsOnzeGui::setRS1(int val){sendController(_currentChannel, CTRL_RS, val);}
void DeicsOnzeGui::setLS2(int val){sendController(_currentChannel, CTRL_LS+DECAPAR1, val);}
void DeicsOnzeGui::setRS2(int val){sendController(_currentChannel, CTRL_RS+DECAPAR1, val);}
void DeicsOnzeGui::setLS3(int val){sendController(_currentChannel, CTRL_LS+2*DECAPAR1, val);}
void DeicsOnzeGui::setRS3(int val){sendController(_currentChannel, CTRL_RS+2*DECAPAR1, val);}
void DeicsOnzeGui::setLS4(int val){sendController(_currentChannel, CTRL_LS+3*DECAPAR1, val);}
void DeicsOnzeGui::setRS4(int val){sendController(_currentChannel, CTRL_RS+3*DECAPAR1, val);}

//--------------------------------------------------------------
// set Volume
//--------------------------------------------------------------
void DeicsOnzeGui::setVol1(int val){sendController(_currentChannel, CTRL_OUT, val);}
void DeicsOnzeGui::setVol2(int val){sendController(_currentChannel, CTRL_OUT+DECAPAR1, val);}
void DeicsOnzeGui::setVol3(int val){sendController(_currentChannel,CTRL_OUT+2*DECAPAR1,val);}
void DeicsOnzeGui::setVol4(int val){sendController(_currentChannel,CTRL_OUT+3*DECAPAR1,val);}

//--------------------------------------------------------------
// set Ratio and Frequency
//--------------------------------------------------------------
void DeicsOnzeGui::setCoarseRatio1(int val) {
    sendController(_currentChannel, CTRL_RATIO, val*100+FineRatio1SpinBox->value());
}
void DeicsOnzeGui::setFineRatio1(int val) {
    sendController(_currentChannel, CTRL_RATIO, val+CoarseRatio1SpinBox->value()*100);
}
void DeicsOnzeGui::setFreq1(int val) {
    sendController(_currentChannel,CTRL_FIXRANGE,val*100);}
void DeicsOnzeGui::setFix1(bool f)  {
    sendController(_currentChannel, CTRL_FIX, (f==false?0:1));}
void DeicsOnzeGui::setCoarseRatio2(int val) {
    sendController(_currentChannel, CTRL_RATIO+DECAPAR1, val*100+FineRatio2SpinBox->value());
}
void DeicsOnzeGui::setFineRatio2(int val) {
    sendController(_currentChannel,CTRL_RATIO+DECAPAR1,val+CoarseRatio2SpinBox->value()*100);
}
void DeicsOnzeGui::setFreq2(int val) {
    sendController(_currentChannel,CTRL_FIXRANGE+DECAPAR2,val*100);}
void DeicsOnzeGui::setFix2(bool f)  {
    sendController(_currentChannel, CTRL_FIX+DECAPAR2, (f==false?0:1));}
void DeicsOnzeGui::setCoarseRatio3(int val) {
    sendController(_currentChannel,CTRL_RATIO+2*DECAPAR1,val*100+FineRatio3SpinBox->value());
}
void DeicsOnzeGui::setFineRatio3(int val) {
    sendController(_currentChannel,CTRL_RATIO+2*DECAPAR1,
		   val+CoarseRatio3SpinBox->value()*100);
}
void DeicsOnzeGui::setFreq3(int val) {
    sendController(_currentChannel,CTRL_FIXRANGE+2*DECAPAR2,val*100);}
void DeicsOnzeGui::setFix3(bool f)  {
    sendController(_currentChannel, CTRL_FIX+2*DECAPAR2, (f==false?0:1));}
void DeicsOnzeGui::setCoarseRatio4(int val) {
    sendController(_currentChannel,CTRL_RATIO+3*DECAPAR1,val*100+FineRatio4SpinBox->value());
}
void DeicsOnzeGui::setFineRatio4(int val) {
    sendController(_currentChannel,CTRL_RATIO+3*DECAPAR1,
		   val+CoarseRatio4SpinBox->value()*100);
}
void DeicsOnzeGui::setFreq4(int val) {
    sendController(_currentChannel,CTRL_FIXRANGE+3*DECAPAR2,val*100);}
void DeicsOnzeGui::setFix4(bool f)  {
    sendController(_currentChannel, CTRL_FIX+3*DECAPAR2, (f==false?0:1));}

//--------------------------------------------------------------
// set Sensitivity
//--------------------------------------------------------------
void DeicsOnzeGui::setAME1(bool val) {sendController(_currentChannel, CTRL_AME, val);}
void DeicsOnzeGui::setEBS1(int val) {sendController(_currentChannel, CTRL_EBS, val);}
void DeicsOnzeGui::setKVS1(int val) {sendController(_currentChannel, CTRL_KVS, val);}
void DeicsOnzeGui::setAME2(bool val) {sendController(_currentChannel,CTRL_AME+DECAPAR1,val);}
void DeicsOnzeGui::setEBS2(int val) {sendController(_currentChannel,CTRL_EBS+DECAPAR1,val);}
void DeicsOnzeGui::setKVS2(int val) {sendController(_currentChannel,CTRL_KVS+DECAPAR1,val);}
void DeicsOnzeGui::setAME3(bool val) {
    sendController(_currentChannel,CTRL_AME+2*DECAPAR1,val);}
void DeicsOnzeGui::setEBS3(int val) {
    sendController(_currentChannel, CTRL_EBS+2*DECAPAR1, val);}
void DeicsOnzeGui::setKVS3(int val) {
    sendController(_currentChannel, CTRL_KVS+2*DECAPAR1, val);}
void DeicsOnzeGui::setAME4(bool val) {
    sendController(_currentChannel, CTRL_AME+3*DECAPAR1, val);}
void DeicsOnzeGui::setEBS4(int val) {
    sendController(_currentChannel, CTRL_EBS+3*DECAPAR1, val);}
void DeicsOnzeGui::setKVS4(int val) {
    sendController(_currentChannel, CTRL_KVS+3*DECAPAR1, val);}

//--------------------------------------------------------------
// set detune
//--------------------------------------------------------------
void DeicsOnzeGui::setDET1(int val){sendController(_currentChannel, CTRL_DET, val);}
void DeicsOnzeGui::setDET2(int val){sendController(_currentChannel, CTRL_DET+DECAPAR1, val);}
void DeicsOnzeGui::setDET3(int val){sendController(_currentChannel,CTRL_DET+2*DECAPAR1,val);}
void DeicsOnzeGui::setDET4(int val){sendController(_currentChannel,CTRL_DET+3*DECAPAR1,val);}

//--------------------------------------------------------------
// set WaveForm
//--------------------------------------------------------------
void DeicsOnzeGui::setWaveForm1(int w) {
    sendController(_currentChannel, CTRL_OSW, w);
}
void DeicsOnzeGui::setWaveForm2(int w) {
    sendController(_currentChannel, CTRL_OSW+DECAPAR2, w);
}
void DeicsOnzeGui::setWaveForm3(int w) {
    sendController(_currentChannel, CTRL_OSW+2*DECAPAR2, w);
}
void DeicsOnzeGui::setWaveForm4(int w) {
    sendController(_currentChannel, CTRL_OSW+3*DECAPAR2, w);
}
//--------------------------------------------------------------
// set delay
//--------------------------------------------------------------
void DeicsOnzeGui::setActivDelay(bool a) {
  unsigned char message[4];
  message[0]=MUSE_SYNTH_SYSEX_MFG_ID;
  message[1]=DEICSONZE_UNIQUE_ID;
  message[2]=SYSEX_DELAYACTIV;
  message[3]=(unsigned char)a;
  sendSysex(message, 4);
}
void DeicsOnzeGui::setDelayReturn(int r) {
  unsigned char message[4];
  message[0]=MUSE_SYNTH_SYSEX_MFG_ID;
  message[1]=DEICSONZE_UNIQUE_ID;
  message[2]=SYSEX_DELAYRETURN;
  message[3]=(unsigned char)r;
  sendSysex(message, 4);
}
void DeicsOnzeGui::setChannelDelay(int d) {
  sendController(_currentChannel, MusECore::CTRL_VARIATION_SEND, (unsigned char)d);
}
//void DeicsOnzeGui::setDelayTime(int t) {
//  unsigned char message[4];
//  message[0]=MUSE_SYNTH_SYSEX_MFG_ID;
//  message[1]=DEICSONZE_UNIQUE_ID;
//  message[2]=SYSEX_DELAYTIME;
//  message[3]=(unsigned char)t;
//  sendSysex(message, 4);
//  updateDelayTime(t);
//}
void DeicsOnzeGui::setDelayBPM(double t) {
  //int it = (int)(((t - MINDELAYTIME) / (MAXDELAYTIME - MINDELAYTIME))*255.0);
  unsigned char message[sizeof(float)+3];
  message[0]=MUSE_SYNTH_SYSEX_MFG_ID;
  message[1]=DEICSONZE_UNIQUE_ID;
  message[2]=SYSEX_DELAYBPM;
  float f = (float)t;
  ///memcpy(&message[1], &f, sizeof(float));
  memcpy(&message[3], &f, sizeof(float));
  ///message[1]=(unsigned char)f;
  message[3]=(unsigned char)f;
  sendSysex(message, sizeof(float)+3);
  //updateDelayTime(it);
} 
void DeicsOnzeGui::setDelayBeatRatio(double t) {
  unsigned char message[sizeof(float)+3];
  message[0]=MUSE_SYNTH_SYSEX_MFG_ID;
  message[1]=DEICSONZE_UNIQUE_ID;
  message[2]=SYSEX_DELAYBEATRATIO;
  float f = (float)t;
  ///memcpy(&message[1], &f, sizeof(float));
  memcpy(&message[3], &f, sizeof(float));
  ///message[1]=(unsigned char)f;
  message[3]=(unsigned char)f;
  sendSysex(message, sizeof(float)+3);
} 
//void DeicsOnzeGui::setDelayFeedback(int f) {
//  unsigned char message[4];
//  message[0]=MUSE_SYNTH_SYSEX_MFG_ID;
//  message[1]=DEICSONZE_UNIQUE_ID;
//  message[2]=SYSEX_DELAYFEEDBACK;
//  message[3]=(unsigned char)f;
//  sendSysex(message, 4);
//  updateDelayFeedback(f);
//}
void DeicsOnzeGui::setDelayFeedback(double t) {
  //int idf = (int)(f*128.0+128.0);
  unsigned char message[sizeof(float)+3];
  message[0]=MUSE_SYNTH_SYSEX_MFG_ID;
  message[1]=DEICSONZE_UNIQUE_ID;
  message[2]=SYSEX_DELAYFEEDBACK;
  float f = (float)t;
  memcpy(&message[3], &f, sizeof(float));
  sendSysex(message, sizeof(float)+3);
  //updateDelayFeedback(idf);
}
//void DeicsOnzeGui::setDelayPanLFOFreq(int pf) {
//  unsigned char message[4];
//  message[0]=MUSE_SYNTH_SYSEX_MFG_ID;
//  message[1]=DEICSONZE_UNIQUE_ID;
//  message[2]=SYSEX_DELAYLFOFREQ;
//  message[3]=(unsigned char)pf;
//  sendSysex(message, 4);
//  updateDelayPanLFOFreq(pf);
//}
void DeicsOnzeGui::setDelayPanLFOFreq(double pf) {
  //int ipf = (int)(((pf - MINFREQ) / (MAXFREQ - MINFREQ))*255.0);
  unsigned char message[sizeof(float)+3];
  message[0]=MUSE_SYNTH_SYSEX_MFG_ID;
  message[1]=DEICSONZE_UNIQUE_ID;
  message[2]=SYSEX_DELAYLFOFREQ;
  float f = (float)pf;
  memcpy(&message[3], &f, sizeof(float));
  sendSysex(message, sizeof(float)+3);
  //updateDelayPanLFOFreq(ipf);
}
//void DeicsOnzeGui::setDelayPanLFODepth(int pd) {
//  unsigned char message[4];
//  message[0]=MUSE_SYNTH_SYSEX_MFG_ID;
//  message[1]=DEICSONZE_UNIQUE_ID;
//  message[2]=SYSEX_DELAYLFODEPTH;
//  message[3]=(unsigned char)pd;
//  sendSysex(message, 4);
//  updateDelayPanLFODepth(pd);
//}
void DeicsOnzeGui::setDelayPanLFODepth(double pd) {
  //int ipd = (int)(pd*255.0);
  unsigned char message[sizeof(float)+3];
  message[0]=MUSE_SYNTH_SYSEX_MFG_ID;
  message[1]=DEICSONZE_UNIQUE_ID;
  message[2]=SYSEX_DELAYLFODEPTH;
  float f = (float)pd;
  memcpy(&message[3], &f, sizeof(float));
  sendSysex(message, sizeof(float)+3);
  //updateDelayPanLFODepth(ipd);
}


//--------------------------------------------------------------
// setSet
//  Display the set, that is the category list
//--------------------------------------------------------------
void DeicsOnzeGui::setSet() {
  categoryListView->clear();
  hbankSpinBox->setEnabled(false);
  categoryLineEdit->setEnabled(false);
  for(std::vector<Category*>::iterator
	i=_deicsOnze->_set->_categoryVector.begin();
      i!=_deicsOnze->_set->_categoryVector.end(); i++)
    (void) new QTreeCategory
      (categoryListView, num3Digits((*i)->_hbank+1),
       (*i)->_categoryName.c_str(), *i);
  categoryListView->resizeColumnToContents(0);
  categoryListView->sortItems(0,Qt::AscendingOrder);
}

//--------------------------------------------------------------
// setCategory
//--------------------------------------------------------------
void DeicsOnzeGui::setCategory(Category* cat) {
    subcategoryListView->clear();
    lbankSpinBox->setEnabled(false);
    subcategoryLineEdit->setEnabled(false);
    for(std::vector<Subcategory*>::iterator i=cat->_subcategoryVector.begin();
	i!=cat->_subcategoryVector.end(); i++)
	(void) new QTreeSubcategory(subcategoryListView,
					    num3Digits((*i)->_lbank+1),
					    (*i)->_subcategoryName.c_str(),*i);
    subcategoryListView->resizeColumnToContents(0);
    subcategoryListView->sortItems(0,Qt::AscendingOrder);
}
void DeicsOnzeGui::setCategory(QTreeWidgetItem* cat) {
  QTreeCategory* ccat = (QTreeCategory*) categoryListView->currentItem();
  if(cat) {
    categoryLineEdit->setEnabled(true);
    hbankSpinBox->setEnabled(true);	
    categoryLineEdit
      ->setText(((QTreeCategory*)cat)->_category->_categoryName.c_str());
    hbankSpinBox->setValue(((QTreeCategory*)cat)->_category->_hbank+1);
    ccat=(QTreeCategory*)cat;
    setCategory(ccat->_category);
  }
}

//--------------------------------------------------------------
// setSubcategory
//--------------------------------------------------------------
void DeicsOnzeGui::setSubcategory(Subcategory* sub) {
  presetListView->clear();
  progSpinBox->setEnabled(false);
  presetLineEdit->setEnabled(false);
  for(std::vector<Preset*>::iterator i=sub->_presetVector.begin();
      i!=sub->_presetVector.end(); i++)
    (void) new QTreePreset(presetListView,
			       num3Digits((*i)->prog+1),
			       (*i)->name.c_str(),*i);
  presetListView->resizeColumnToContents(0);
  presetListView->sortItems(0,Qt::AscendingOrder);
}
void DeicsOnzeGui::setSubcategory(QTreeWidgetItem* sub) {
  QTreeSubcategory* csub =
    (QTreeSubcategory*) subcategoryListView->currentItem();
  if(sub) {
    subcategoryLineEdit->setEnabled(true);
    lbankSpinBox->setEnabled(true);	
    subcategoryLineEdit->setText(((QTreeSubcategory*)sub)
				 ->_subcategory->_subcategoryName.c_str());
    lbankSpinBox->setValue(((QTreeSubcategory*)sub)
			   ->_subcategory->_lbank+1);
    csub=(QTreeSubcategory*)sub;
    setSubcategory(csub->_subcategory);
  }
}
//--------------------------------------------------------------
// setPreset
//--------------------------------------------------------------
void DeicsOnzeGui::setPreset(QTreeWidgetItem* pre) {
  if(pre) {
    QTreePreset* cpre=(QTreePreset*)pre;
    updatePresetName(cpre->_preset->name.c_str(), true);
    updateProg(cpre->_preset->prog, true);
    int prog = cpre->_preset->prog;
    int lbank = cpre->_preset->_subcategory->_lbank;
    int hbank = cpre->_preset->_subcategory->_category->_hbank;
    setEnabledPreset(true);
    updatePreset(cpre->_preset);
    sendController(_currentChannel, MusECore::CTRL_PROGRAM, (hbank<<16)+(lbank<<8)+prog);
  }
}
//--------------------------------------------------------------
// update the gui
//--------------------------------------------------------------
void DeicsOnzeGui::setEnabledPreset(bool b) {
  if(_enabledPreset!=b) {
    FeedbackGroupBox->setEnabled(b);
    LFOGroupBox->setEnabled(b);
    pitchEnvGroupBox->setEnabled(b);
    ModulationMatrixGroupBox->setEnabled(b);
    Op1Tab->setEnabled(b);
    Op2Tab->setEnabled(b);
    Op3Tab->setEnabled(b);
    Op4Tab->setEnabled(b);
    FunctionsTab->setEnabled(b);
    chorusTab->setEnabled(b);
    reverbTab->setEnabled(b);
    
    _enabledPreset=b;
  }
}
void DeicsOnzeGui::updateChannelEnable(bool e) {
  updateChannelCheckBox(e);
  updateEnabledChannel(e);
}
void DeicsOnzeGui::updateEnabledChannel(bool e) {
  numberVoicesLabel->setEnabled(e);
  nbrVoicesSpinBox->setEnabled(e);
  channelCtrlGroupBox->setEnabled(e);
  deicsOnzeTabWidget->setEnabled(e);
}
void DeicsOnzeGui::updateNbrVoices(int val) {
  nbrVoicesSpinBox->blockSignals(true);
  nbrVoicesSpinBox->setValue(val);
  nbrVoicesSpinBox->blockSignals(false);  
}
void DeicsOnzeGui::updateMasterVolume(int val) {
  double d = (double)val/(double)MAXMASTERVOLUME;
  //printf("D = %d, %f\n", val, d);
  masterVolKnob->blockSignals(true);
  masterVolKnob->setValue(d);
  masterVolKnob->blockSignals(false);
}
//void DeicsOnzeGui::updateMidiInCh(int val) {
  //MidiInChComboBox->blockSignals(true);
  //MidiInChComboBox->setCurrentIndex(val);		
  //MidiInChComboBox->blockSignals(false);
//}
void DeicsOnzeGui::updateQuality(int val) {
  qualityComboBox->blockSignals(true);
  qualityComboBox->setCurrentIndex(val);		
  qualityComboBox->blockSignals(false);
}
void DeicsOnzeGui::updateFilter(bool f) {
  filterCheckBox->blockSignals(true);
  filterCheckBox->setChecked(f);		
  filterCheckBox->blockSignals(false);
}
void DeicsOnzeGui::updateFontSize(int val) {
  fontSizeSpinBox->blockSignals(true);
  fontSizeSpinBox->setValue(val);
  fontSizeSpinBox->blockSignals(false);
}
//FX
void DeicsOnzeGui::updateChorusActiv(bool a) {
  chorusActivCheckBox->blockSignals(true);
  chorusActivCheckBox->setChecked(a);		
  chorusActivCheckBox->blockSignals(false);
}
void DeicsOnzeGui::updateChannelChorus(int c) {
  chChorusSlider->blockSignals(true);
  chChorusSlider->setValue(c);
  chChorusSlider->blockSignals(false);
  chChorusSpinBox->blockSignals(true);
  chChorusSpinBox->setValue(c);
  chChorusSpinBox->blockSignals(false);
}
void DeicsOnzeGui::updateChorusReturn(int r) {
  returnChorusSlider->blockSignals(true);
  returnChorusSlider->setValue(r);
  returnChorusSlider->blockSignals(false);
}
/*void DeicsOnzeGui::updatePanChorus1(int c) {
  panChorus1Knob->blockSignals(true);
  panChorus1Knob->setValue((double)c/(double)MAXCHORUSPARAM);
  panChorus1Knob->blockSignals(false);
}
void DeicsOnzeGui::updateLFOFreqChorus1(int c) {
  LFOFreqChorus1Knob->blockSignals(true);
  LFOFreqChorus1Knob->setValue((double)c/(double)MAXCHORUSPARAM);
  LFOFreqChorus1Knob->blockSignals(false);
}
void DeicsOnzeGui::updateDepthChorus1(int c) {
  depthChorus1Knob->blockSignals(true);
  depthChorus1Knob->setValue((double)c/(double)MAXCHORUSPARAM);
  depthChorus1Knob->blockSignals(false);
}
void DeicsOnzeGui::updatePanChorus2(int c) {
  panChorus2Knob->blockSignals(true);
  panChorus2Knob->setValue((double)c/(double)MAXCHORUSPARAM);
  panChorus2Knob->blockSignals(false);
}
void DeicsOnzeGui::updateLFOFreqChorus2(int c) {
  LFOFreqChorus2Knob->blockSignals(true);
  LFOFreqChorus2Knob->setValue((double)c/(double)MAXCHORUSPARAM);
  LFOFreqChorus2Knob->blockSignals(false);
}
void DeicsOnzeGui::updateDepthChorus2(int c) {
  depthChorus2Knob->blockSignals(true);
  depthChorus2Knob->setValue((double)c/(double)MAXCHORUSPARAM);
  depthChorus2Knob->blockSignals(false);
  }*/
void DeicsOnzeGui::updateReverbActiv(bool a) {
  reverbActivCheckBox->blockSignals(true);
  reverbActivCheckBox->setChecked(a);		
  reverbActivCheckBox->blockSignals(false);
}
void DeicsOnzeGui::updateChannelReverb(int r) {
  chReverbSlider->blockSignals(true);
  chReverbSlider->setValue(r);
  chReverbSlider->blockSignals(false);
  chReverbSpinBox->blockSignals(true);
  chReverbSpinBox->setValue(r);
  chReverbSpinBox->blockSignals(false);
}
void DeicsOnzeGui::updateReverbReturn(int r) {
  returnReverbSlider->blockSignals(true);
  returnReverbSlider->setValue(r);
  returnReverbSlider->blockSignals(false);
}
void DeicsOnzeGui::updateLadspaReverbLineEdit(QString s) {
  selectLadspaReverbLineEdit->blockSignals(true);
  selectLadspaReverbLineEdit->setText(s);
  selectLadspaReverbLineEdit->blockSignals(false);
}
void DeicsOnzeGui::updateLadspaChorusLineEdit(QString s) {
  selectLadspaChorusLineEdit->blockSignals(true);
  selectLadspaChorusLineEdit->setText(s);
  selectLadspaChorusLineEdit->blockSignals(false);
}

void DeicsOnzeGui::updateDelayActiv(bool a) {
  delayActivCheckBox->blockSignals(true);
  delayActivCheckBox->setChecked(a);		
  delayActivCheckBox->blockSignals(false);
}
void DeicsOnzeGui::updateChannelDelay(int r) {
  chDelaySlider->blockSignals(true);
  chDelaySlider->setValue(r);
  chDelaySlider->blockSignals(false);
  chDelaySpinBox->blockSignals(true);
  chDelaySpinBox->setValue(r);
  chDelaySpinBox->blockSignals(false);
}
void DeicsOnzeGui::updateDelayReturn(int r) {
  delayReturnSlider->blockSignals(true);
  delayReturnSlider->setValue(r);
  delayReturnSlider->blockSignals(false);
}
void DeicsOnzeGui::updateDelayPanLFOFreq(float plf) {
  delayPanLFOFreqKnob->blockSignals(true);
  delayPanLFOFreqKnob->setValue((double)plf);
  delayPanLFOFreqKnob->blockSignals(false);
  delayPanLFOFreqFloatentry->blockSignals(true);
  delayPanLFOFreqFloatentry->setValue((double)plf);
  delayPanLFOFreqFloatentry->blockSignals(false);
}
void DeicsOnzeGui::updateDelayBPM(float dt) {
  delayBPMKnob->blockSignals(true);
  delayBPMKnob->setValue((double)dt);
  delayBPMKnob->blockSignals(false);
  delayBPMFloatentry->blockSignals(true);
  delayBPMFloatentry->setValue((double)dt);
  delayBPMFloatentry->blockSignals(false);
}
void DeicsOnzeGui::updateDelayBeatRatio(float dt) {
  delayBeatRatioKnob->blockSignals(true);
  delayBeatRatioKnob->setValue((double)dt);
  delayBeatRatioKnob->blockSignals(false);
  delayBeatRatioFloatentry->blockSignals(true);
  delayBeatRatioFloatentry->setValue((double)dt);
  delayBeatRatioFloatentry->blockSignals(false);
}
void DeicsOnzeGui::updateDelayFeedback(float df) {
  delayFeedbackKnob->blockSignals(true);
  delayFeedbackKnob->setValue((double)df);
  delayFeedbackKnob->blockSignals(false);
  delayFeedbackFloatentry->blockSignals(true);
  delayFeedbackFloatentry->setValue((double)df);
  delayFeedbackFloatentry->blockSignals(false);
}
void DeicsOnzeGui::updateDelayPanLFODepth(float dpd) {
  delayPanLFODepthKnob->blockSignals(true);
  delayPanLFODepthKnob->setValue((double)dpd);
  delayPanLFODepthKnob->blockSignals(false);
  delayPanLFODepthFloatentry->blockSignals(true);
  delayPanLFODepthFloatentry->setValue((double)dpd);
  delayPanLFODepthFloatentry->blockSignals(false);
}

void DeicsOnzeGui::applyFontSize(int fs) {
  QFont f = font();
  f.setPointSize(fs);
  setFont(f);
}
void DeicsOnzeGui::updateSaveConfig(bool usc) {
  saveConfigCheckBox->blockSignals(true);
  saveConfigCheckBox->setChecked(usc);		
  saveConfigCheckBox->blockSignals(false);
}
void DeicsOnzeGui::updateSaveOnlyUsed(bool usou) {
  hugeSaveRadioButton->blockSignals(true);
  hugeSaveRadioButton->setChecked(!usou);		
  hugeSaveRadioButton->blockSignals(false);
  minSaveRadioButton->blockSignals(true);
  minSaveRadioButton->setChecked(usou);		
  minSaveRadioButton->blockSignals(false);
}
//Pitch Envelope
void DeicsOnzeGui::updatePL1(int val) {
  PL1SpinBox->blockSignals(true);
  PL1SpinBox->setValue(val);
  PL1SpinBox->blockSignals(false);
}
void DeicsOnzeGui::updatePL2(int val) {
  PL2SpinBox->blockSignals(true);
  PL2SpinBox->setValue(val);
  PL2SpinBox->blockSignals(false);
}
void DeicsOnzeGui::updatePL3(int val) {
  PL3SpinBox->blockSignals(true);
  PL3SpinBox->setValue(val);
  PL3SpinBox->blockSignals(false);
}
void DeicsOnzeGui::updatePR1(int val) {
  PR1SpinBox->blockSignals(true);
  PR1SpinBox->setValue(val);
  PR1SpinBox->blockSignals(false);
}
void DeicsOnzeGui::updatePR2(int val) {
  PR2SpinBox->blockSignals(true);
  PR2SpinBox->setValue(val);
  PR2SpinBox->blockSignals(false);
}
void DeicsOnzeGui::updatePR3(int val) {
  PR3SpinBox->blockSignals(true);
  PR3SpinBox->setValue(val);
  PR3SpinBox->blockSignals(false);
}
//Function
void DeicsOnzeGui::updateFcVolume(int val) {
  fcVolumeSpinBox->blockSignals(true);
  fcVolumeSpinBox->setValue(val);
  fcVolumeSpinBox->blockSignals(false);
  fcVolumeSlider->blockSignals(true);
  fcVolumeSlider->setValue(val);
  fcVolumeSlider->blockSignals(false);
}
void DeicsOnzeGui::updateFcPitch(int val) {
  fcPitchSpinBox->blockSignals(true);
  fcPitchSpinBox->setValue(val);
  fcPitchSpinBox->blockSignals(false);
  fcPitchSlider->blockSignals(true);
  fcPitchSlider->setValue(val);
  fcPitchSlider->blockSignals(false);
}
void DeicsOnzeGui::updateFcAmplitude(int val) {
  fcAmplitudeSpinBox->blockSignals(true);
  fcAmplitudeSpinBox->setValue(val);
  fcAmplitudeSpinBox->blockSignals(false);
  fcAmplitudeSlider->blockSignals(true);
  fcAmplitudeSlider->setValue(val);
  fcAmplitudeSlider->blockSignals(false);
}
void DeicsOnzeGui::updateMwPitch(int val) {
  mwPitchSpinBox->blockSignals(true);
  mwPitchSpinBox->setValue(val);
  mwPitchSpinBox->blockSignals(false);
  mwPitchSlider->blockSignals(true);
  mwPitchSlider->setValue(val);
  mwPitchSlider->blockSignals(false);
}
void DeicsOnzeGui::updateMwAmplitude(int val) {
  mwAmplitudeSpinBox->blockSignals(true);
  mwAmplitudeSpinBox->setValue(val);
  mwAmplitudeSpinBox->blockSignals(false);
  mwAmplitudeSlider->blockSignals(true);
  mwAmplitudeSlider->setValue(val);
  mwAmplitudeSlider->blockSignals(false);
}
void DeicsOnzeGui::updateBcPitch(int val) {
  bcPitchSpinBox->blockSignals(true);
  bcPitchSpinBox->setValue(val);
  bcPitchSpinBox->blockSignals(false);
  bcPitchSlider->blockSignals(true);
  bcPitchSlider->setValue(val);
  bcPitchSlider->blockSignals(false);
}
void DeicsOnzeGui::updateBcAmplitude(int val) {
  bcAmplitudeSpinBox->blockSignals(true);
  bcAmplitudeSpinBox->setValue(val);
  bcAmplitudeSpinBox->blockSignals(false);
  bcAmplitudeSlider->blockSignals(true);
  bcAmplitudeSlider->setValue(val);
  bcAmplitudeSlider->blockSignals(false);
}
void DeicsOnzeGui::updateBcPitchBias(int val) {
  bcPitchBiasSpinBox->blockSignals(true);
  bcPitchBiasSpinBox->setValue(val);
  bcPitchBiasSpinBox->blockSignals(false);
  bcPitchBiasSlider->blockSignals(true);
  bcPitchBiasSlider->setValue(val);
  bcPitchBiasSlider->blockSignals(false);
}
void DeicsOnzeGui::updateBcEgBias(int val) {
  bcEgBiasSpinBox->blockSignals(true);
  bcEgBiasSpinBox->setValue(val);
  bcEgBiasSpinBox->blockSignals(false);
  bcEgBiasSlider->blockSignals(true);
  bcEgBiasSlider->setValue(val);
  bcEgBiasSlider->blockSignals(false);
}
void DeicsOnzeGui::updateAtPitch(int val) {
  atPitchSpinBox->blockSignals(true);
  atPitchSpinBox->setValue(val);
  atPitchSpinBox->blockSignals(false);
  atPitchSlider->blockSignals(true);
  atPitchSlider->setValue(val);
  atPitchSlider->blockSignals(false);
}
void DeicsOnzeGui::updateAtAmplitude(int val) {
  atAmplitudeSpinBox->blockSignals(true);
  atAmplitudeSpinBox->setValue(val);
  atAmplitudeSpinBox->blockSignals(false);
  atAmplitudeSlider->blockSignals(true);
  atAmplitudeSlider->setValue(val);
  atAmplitudeSlider->blockSignals(false);
}
void DeicsOnzeGui::updateAtPitchBias(int val) {
  atPitchBiasSpinBox->blockSignals(true);
  atPitchBiasSpinBox->setValue(val);
  atPitchBiasSpinBox->blockSignals(false);
  atPitchBiasSlider->blockSignals(true);
  atPitchBiasSlider->setValue(val);
  atPitchBiasSlider->blockSignals(false);
}
void DeicsOnzeGui::updateAtEgBias(int val) {
  atEgBiasSpinBox->blockSignals(true);
  atEgBiasSpinBox->setValue(val);
  atEgBiasSpinBox->blockSignals(false);
  atEgBiasSlider->blockSignals(true);
  atEgBiasSlider->setValue(val);
  atEgBiasSlider->blockSignals(false);
}
//void DeicsOnzeGui::updateReverbRate(int val) {
  //reverbSpinBox->blockSignals(true);
  //reverbSpinBox->setValue(val);
  //reverbSpinBox->blockSignals(false);
  //reverbSlider->blockSignals(true);
  //reverbSlider->setValue(val);
  //reverbSlider->blockSignals(false);
//}
//Envelope
void DeicsOnzeGui::updateAR(int op, int val) {
    Eg* _eg=&(_deicsOnze->_preset[_currentChannel]->eg[op]);
    envelopeGraph[op]->env2Points(val, _eg->d1r, _eg->d1l, _eg->d2r, _eg->rr);
    envelopeGraph[op]->updateEnv();
    switch(op) {
	case 0:
	    AR1SpinBox->blockSignals(true);
	    AR1SpinBox->setValue(val);
	    AR1SpinBox->blockSignals(false);
	    break;
	case 1:
	    AR2SpinBox->blockSignals(true);
	    AR2SpinBox->setValue(val);
	    AR2SpinBox->blockSignals(false);
	    break;
	case 2:
	    AR3SpinBox->blockSignals(true);
	    AR3SpinBox->setValue(val);
	    AR3SpinBox->blockSignals(false);
	    break;
	case 3:
	    AR4SpinBox->blockSignals(true);
	    AR4SpinBox->setValue(val);
	    AR4SpinBox->blockSignals(false);
	    break;
	default: printf("DeicsOnzeGui::updateAR : Error switch\n");
    }
}
	
void DeicsOnzeGui::updateD1R(int op, int val) {
    Eg* _eg=&(_deicsOnze->_preset[_currentChannel]->eg[op]);
    envelopeGraph[op]->env2Points(_eg->ar, val, _eg->d1l, _eg->d2r, _eg->rr);
    envelopeGraph[op]->updateEnv();
    switch(op) {
	case 0:
	    D1R1SpinBox->blockSignals(true);
	    D1R1SpinBox->setValue(val);
	    D1R1SpinBox->blockSignals(false);
	    break;
	case 1:
	    D1R2SpinBox->blockSignals(true);
	    D1R2SpinBox->setValue(val);
	    D1R2SpinBox->blockSignals(false);
	    break;
	case 2:
	    D1R3SpinBox->blockSignals(true);
	    D1R3SpinBox->setValue(val);
	    D1R3SpinBox->blockSignals(false);
	    break;
	case 3:
	    D1R4SpinBox->blockSignals(true);
	    D1R4SpinBox->setValue(val);
	    D1R4SpinBox->blockSignals(false);
	    break;
	default: printf("DeicsOnzeGui::updateD1R : Error switch\n");
    }
}
void DeicsOnzeGui::updateD2R(int op, int val) {
    Eg* _eg=&(_deicsOnze->_preset[_currentChannel]->eg[op]);
    envelopeGraph[op]->env2Points(_eg->ar, _eg->d1r, _eg->d1l, val, _eg->rr);
    envelopeGraph[op]->updateEnv();
    switch(op) {
	case 0:
	    D2R1SpinBox->blockSignals(true);
	    D2R1SpinBox->setValue(val);
	    D2R1SpinBox->blockSignals(false);
	    break;
	case 1:
	    D2R2SpinBox->blockSignals(true);
	    D2R2SpinBox->setValue(val);
	    D2R2SpinBox->blockSignals(false);
	    break;
	case 2:
	    D2R3SpinBox->blockSignals(true);
	    D2R3SpinBox->setValue(val);
	    D2R3SpinBox->blockSignals(false);
	    break;
	case 3:
	    D2R4SpinBox->blockSignals(true);
	    D2R4SpinBox->setValue(val);
	    D2R4SpinBox->blockSignals(false);
	    break;
	default: printf("DeicsOnzeGui::updateD2R : Error switch\n");
    }
}
void DeicsOnzeGui::updateRR(int op, int val) {
    Eg* _eg=&(_deicsOnze->_preset[_currentChannel]->eg[op]);
    envelopeGraph[op]->env2Points(_eg->ar, _eg->d1r, _eg->d1l, _eg->d2r, val);
    envelopeGraph[op]->updateEnv();
    switch(op) {
	case 0:
	    RR1SpinBox->blockSignals(true);
	    RR1SpinBox->setValue(val);
	    RR1SpinBox->blockSignals(false);
	    break;
	case 1:
	    RR2SpinBox->blockSignals(true);
	    RR2SpinBox->setValue(val);
	    RR2SpinBox->blockSignals(false);
	    break;
	case 2:
	    RR3SpinBox->blockSignals(true);
	    RR3SpinBox->setValue(val);
	    RR3SpinBox->blockSignals(false);
	    break;
	case 3:
	    RR4SpinBox->blockSignals(true);
	    RR4SpinBox->setValue(val);
	    RR4SpinBox->blockSignals(false);
	    break;
	default: printf("DeicsOnzeGui::updateRR : Error switch\n");
    }
}
void DeicsOnzeGui::updateD1L(int op, int val) {
    Eg* _eg=&(_deicsOnze->_preset[_currentChannel]->eg[op]);
    envelopeGraph[op]->env2Points(_eg->ar, _eg->d1r, val, _eg->d2r, _eg->rr);
    envelopeGraph[op]->updateEnv();
    switch(op) {
	case 0:
	    D1L1SpinBox->blockSignals(true);
	    D1L1SpinBox->setValue(val);
	    D1L1SpinBox->blockSignals(false);
	    break;
	case 1:
	    D1L2SpinBox->blockSignals(true);
	    D1L2SpinBox->setValue(val);
	    D1L2SpinBox->blockSignals(false);
	    break;
	case 2:
	    D1L3SpinBox->blockSignals(true);
	    D1L3SpinBox->setValue(val);
	    D1L3SpinBox->blockSignals(false);
	    break;
	case 3:
	    D1L4SpinBox->blockSignals(true);
	    D1L4SpinBox->setValue(val);
	    D1L4SpinBox->blockSignals(false);
	    break;
	default: printf("DeicsOnzeGui::updateD1L : Error switch\n");
    }
}
void DeicsOnzeGui::updateLS(int op, int val) {
    switch(op) {
	case 0:
	    LS1Slider->blockSignals(true);
	    LS1Slider->setValue(val);
	    LS1Slider->blockSignals(false);
	    LS1SpinBox->blockSignals(true);
	    LS1SpinBox->setValue(val);
	    LS1SpinBox->blockSignals(false);
	    break;
	case 1:
	    LS2Slider->blockSignals(true);
	    LS2Slider->setValue(val);
	    LS2Slider->blockSignals(false);
	    LS2SpinBox->blockSignals(true);
	    LS2SpinBox->setValue(val);
	    LS2SpinBox->blockSignals(false);
	    break;
	case 2:
	    LS3Slider->blockSignals(true);
	    LS3Slider->setValue(val);
	    LS3Slider->blockSignals(false);
	    LS3SpinBox->blockSignals(true);
	    LS3SpinBox->setValue(val);
	    LS3SpinBox->blockSignals(false);
	    break;
	case 3:
	    LS4Slider->blockSignals(true);
	    LS4Slider->setValue(val);
	    LS4Slider->blockSignals(false);
	    LS4SpinBox->blockSignals(true);
	    LS4SpinBox->setValue(val);
	    LS4SpinBox->blockSignals(false);
	    break;
	default: printf("DeicsOnzeGui::updateLS : Error switch\n");
    }
}
void DeicsOnzeGui::updateRS(int op, int val) {
    switch(op) {
	case 0:
	    RS1Slider->blockSignals(true);
	    RS1Slider->setValue(val);
	    RS1Slider->blockSignals(false);
	    RS1SpinBox->blockSignals(true);
	    RS1SpinBox->setValue(val);
	    RS1SpinBox->blockSignals(false);
	    break;
	case 1:
	    RS2Slider->blockSignals(true);
	    RS2Slider->setValue(val);
	    RS2Slider->blockSignals(false);
	    RS2SpinBox->blockSignals(true);
	    RS2SpinBox->setValue(val);
	    RS2SpinBox->blockSignals(false);
	    break;
	case 2:
	    RS3Slider->blockSignals(true);
	    RS3Slider->setValue(val);
	    RS3Slider->blockSignals(false);
	    RS3SpinBox->blockSignals(true);
	    RS3SpinBox->setValue(val);
	    RS3SpinBox->blockSignals(false);
	    break;
	case 3:
	    RS4Slider->blockSignals(true);
	    RS4Slider->setValue(val);
	    RS4Slider->blockSignals(false);
	    RS4SpinBox->blockSignals(true);
	    RS4SpinBox->setValue(val);
	    RS4SpinBox->blockSignals(false);
	    break;
	default: printf("DeicsOnzeGui::updateRS : Error switch\n");
    }
}
void DeicsOnzeGui::updateEBS(int op, int val) {
    switch(op) {
	case 0:
	    EBS1Slider->blockSignals(true);
	    EBS1Slider->setValue(val);
	    EBS1Slider->blockSignals(false);
	    EBS1SpinBox->blockSignals(true);
	    EBS1SpinBox->setValue(val);
	    EBS1SpinBox->blockSignals(false);
	    break;
	case 1:
	    EBS2Slider->blockSignals(true);
	    EBS2Slider->setValue(val);
	    EBS2Slider->blockSignals(false);
	    EBS2SpinBox->blockSignals(true);
	    EBS2SpinBox->setValue(val);
	    EBS2SpinBox->blockSignals(false);
	    break;
	case 2:
	    EBS3Slider->blockSignals(true);
	    EBS3Slider->setValue(val);
	    EBS3Slider->blockSignals(false);
	    EBS3SpinBox->blockSignals(true);
	    EBS3SpinBox->setValue(val);
	    EBS3SpinBox->blockSignals(false);
	    break;
	case 3:
	    EBS4Slider->blockSignals(true);
	    EBS4Slider->setValue(val);
	    EBS4Slider->blockSignals(false);
	    EBS4SpinBox->blockSignals(true);
	    EBS4SpinBox->setValue(val);
	    EBS4SpinBox->blockSignals(false);
	    break;
	default: printf("DeicsOnzeGui::updateEBS : Error switch\n");
    }
}
void DeicsOnzeGui::updateAME(int op, bool val) {
    switch(op) {
	case 0:
	    AME1CheckBox->blockSignals(true);
	    AME1CheckBox->setChecked(val);
	    AME1CheckBox->blockSignals(false);
	    break;
	case 1:
	    AME2CheckBox->blockSignals(true);
	    AME2CheckBox->setChecked(val);
	    AME2CheckBox->blockSignals(false);
	    break;
	case 2:
	    AME3CheckBox->blockSignals(true);
	    AME3CheckBox->setChecked(val);
	    AME3CheckBox->blockSignals(false);
	    break;
	case 3:
	    AME4CheckBox->blockSignals(true);
	    AME4CheckBox->setChecked(val);
	    AME4CheckBox->blockSignals(false);
	    break;
	default: printf("DeicsOnzeGui::updateAME : Error switch\n");
    }
}
void DeicsOnzeGui::updateKVS(int op, int val) {
    switch(op) {
	case 0:
	    KVS1Slider->blockSignals(true);
	    KVS1Slider->setValue(val);
	    KVS1Slider->blockSignals(false);
	    KVS1SpinBox->blockSignals(true);
	    KVS1SpinBox->setValue(val);
	    KVS1SpinBox->blockSignals(false);
	    break;
	case 1:
	    KVS2Slider->blockSignals(true);
	    KVS2Slider->setValue(val);
	    KVS2Slider->blockSignals(false);
	    KVS2SpinBox->blockSignals(true);
	    KVS2SpinBox->setValue(val);
	    KVS2SpinBox->blockSignals(false);
	    break;
	case 2:
	    KVS3Slider->blockSignals(true);
	    KVS3Slider->setValue(val);
	    KVS3Slider->blockSignals(false);
	    KVS3SpinBox->blockSignals(true);
	    KVS3SpinBox->setValue(val);
	    KVS3SpinBox->blockSignals(false);
	    break;
	case 3:
	    KVS4Slider->blockSignals(true);
	    KVS4Slider->setValue(val);
	    KVS4Slider->blockSignals(false);
	    KVS4SpinBox->blockSignals(true);
	    KVS4SpinBox->setValue(val);
	    KVS4SpinBox->blockSignals(false);
	    break;
	default: printf("DeicsOnzeGui::updateKVS : Error switch\n");
    }
}
void DeicsOnzeGui::updateOUT(int op, int val) {
    switch(op) {
	case 0:
	    OUT1Slider->blockSignals(true);
	    OUT1Slider->setValue(val);
	    OUT1Slider->blockSignals(false);
	    OUT1SpinBox->blockSignals(true);
	    OUT1SpinBox->setValue(val);
	    OUT1SpinBox->blockSignals(false);
	    break;
	case 1:
	    OUT2Slider->blockSignals(true);
	    OUT2Slider->setValue(val);
	    OUT2Slider->blockSignals(false);
	    OUT2SpinBox->blockSignals(true);
	    OUT2SpinBox->setValue(val);
	    OUT2SpinBox->blockSignals(false);
	    break;
	case 2:
	    OUT3Slider->blockSignals(true);
	    OUT3Slider->setValue(val);
	    OUT3Slider->blockSignals(false);
	    OUT3SpinBox->blockSignals(true);
	    OUT3SpinBox->setValue(val);
	    OUT3SpinBox->blockSignals(false);
	    break;
	case 3:
	    OUT4Slider->blockSignals(true);
	    OUT4Slider->setValue(val);
	    OUT4Slider->blockSignals(false);
	    OUT4SpinBox->blockSignals(true);
	    OUT4SpinBox->setValue(val);
	    OUT4SpinBox->blockSignals(false);
	    break;
	default: printf("DeicsOnzeGui::updateOUT : Error switch\n");
    }
}
void DeicsOnzeGui::updateRATIO(int op, int val) {
    switch(op) {
	case 0:
	    CoarseRatio1SpinBox->blockSignals(true);
	    CoarseRatio1SpinBox->setValue(val/100);
	    CoarseRatio1SpinBox->blockSignals(false);
	    FineRatio1SpinBox->blockSignals(true);
	    FineRatio1SpinBox->setValue(val%100);
	    FineRatio1SpinBox->blockSignals(false);
	    break;
	case 1:
	    CoarseRatio2SpinBox->blockSignals(true);
	    CoarseRatio2SpinBox->setValue(val/100);
	    CoarseRatio2SpinBox->blockSignals(false);
	    FineRatio2SpinBox->blockSignals(true);
	    FineRatio2SpinBox->setValue(val%100);
	    FineRatio2SpinBox->blockSignals(false);
	    break;
	case 2:
	    CoarseRatio3SpinBox->blockSignals(true);
	    CoarseRatio3SpinBox->setValue(val/100);
	    CoarseRatio3SpinBox->blockSignals(false);
	    FineRatio3SpinBox->blockSignals(true);
	    FineRatio3SpinBox->setValue(val%100);
	    FineRatio3SpinBox->blockSignals(false);
	    break;
	case 3:
	    CoarseRatio4SpinBox->blockSignals(true);
	    CoarseRatio4SpinBox->setValue(val/100);
	    CoarseRatio4SpinBox->blockSignals(false);
	    FineRatio4SpinBox->blockSignals(true);
	    FineRatio4SpinBox->setValue(val%100);
	    FineRatio4SpinBox->blockSignals(false);
	    break;
	default: printf("DeicsOnzeGui::updateRATIO : Error switch\n");
    }
}
void DeicsOnzeGui::updateDET(int op, int val) {
    switch(op) {
	case 0:
	    DET1Slider->blockSignals(true);
	    DET1Slider->setValue(val);
	    DET1Slider->blockSignals(false);
	    DET1SpinBox->blockSignals(true);
	    DET1SpinBox->setValue(val);
	    DET1SpinBox->blockSignals(false);
	    break;
	case 1:
	    DET2Slider->blockSignals(true);
	    DET2Slider->setValue(val);
	    DET2Slider->blockSignals(false);
	    DET2SpinBox->blockSignals(true);
	    DET2SpinBox->setValue(val);
	    DET2SpinBox->blockSignals(false);
	    break;
	case 2:
	    DET3Slider->blockSignals(true);
	    DET3Slider->setValue(val);
	    DET3Slider->blockSignals(false);
	    DET3SpinBox->blockSignals(true);
	    DET3SpinBox->setValue(val);
	    DET3SpinBox->blockSignals(false);
	    break;
	case 3:
	    DET4Slider->blockSignals(true);
	    DET4Slider->setValue(val);
	    DET4Slider->blockSignals(false);
	    DET4SpinBox->blockSignals(true);
	    DET4SpinBox->setValue(val);
	    DET4SpinBox->blockSignals(false);
	    break;
	default: printf("DeicsOnzeGui::updateDET : Error switch\n");
    }
}
void DeicsOnzeGui::updateALG(int val) {
    algorithmComboBox->blockSignals(true);
    algorithmComboBox->setCurrentIndex(val);
    algorithmComboBox->blockSignals(false);
}
void DeicsOnzeGui::updateFEEDBACK(int val) {
    feedbackSlider->blockSignals(true);
    feedbackSlider->setValue(val);
    feedbackSlider->blockSignals(false);
    feedbackSpinBox->blockSignals(true);
    feedbackSpinBox->setValue(val);
    feedbackSpinBox->blockSignals(false);
}
void DeicsOnzeGui::updateSPEED(int val) {
    LFOSpeedSlider->blockSignals(true);
    LFOSpeedSlider->setValue(val);
    LFOSpeedSlider->blockSignals(false);
    LFOSpeedSpinBox->blockSignals(true);
    LFOSpeedSpinBox->setValue(val);
    LFOSpeedSpinBox->blockSignals(false);
}
void DeicsOnzeGui::updateDELAY(int val) {
    LFODelaySlider->blockSignals(true);
    LFODelaySlider->setValue(val);
    LFODelaySlider->blockSignals(false);
    LFODelaySpinBox->blockSignals(true);
    LFODelaySpinBox->setValue(val);
    LFODelaySpinBox->blockSignals(false);
}
void DeicsOnzeGui::updatePMODDEPTH(int val) {
    PModDepthSlider->blockSignals(true);
    PModDepthSlider->setValue(val);
    PModDepthSlider->blockSignals(false);
    PModDepthSpinBox->blockSignals(true);
    PModDepthSpinBox->setValue(val);
    PModDepthSpinBox->blockSignals(false);
}
void DeicsOnzeGui::updateAMODDEPTH(int val) {
    AModDepthSlider->blockSignals(true);
    AModDepthSlider->setValue(val);
    AModDepthSlider->blockSignals(false);
    AModDepthSpinBox->blockSignals(true);
    AModDepthSpinBox->setValue(val);
    AModDepthSpinBox->blockSignals(false);
}
void DeicsOnzeGui::updateSYNC(bool val) {
    LFOSyncCheckBox->blockSignals(true);
    LFOSyncCheckBox->setChecked(val);
    LFOSyncCheckBox->blockSignals(false);
}
void DeicsOnzeGui::updateWAVE(int val) {
    LFOWaveComboBox->blockSignals(true);
    LFOWaveComboBox->setCurrentIndex(val);
    LFOWaveComboBox->blockSignals(false);
}
void DeicsOnzeGui::updatePMODSENS(int val) {
    PModSensSlider->blockSignals(true);
    PModSensSlider->setValue(val);
    PModSensSlider->blockSignals(false);
    PModSensSpinBox->blockSignals(true);
    PModSensSpinBox->setValue(val);
    PModSensSpinBox->blockSignals(false);
}
void DeicsOnzeGui::updateAMS(int val) {
    AModSensSlider->blockSignals(true);
    AModSensSlider->setValue(val);
    AModSensSlider->blockSignals(false);
    AModSensSpinBox->blockSignals(true);
    AModSensSpinBox->setValue(val);
    AModSensSpinBox->blockSignals(false);
}
void DeicsOnzeGui::updateTRANSPOSE(int val) {
    transposeSlider->blockSignals(true);
    transposeSlider->setValue(val);
    transposeSlider->blockSignals(false);
    transposeSpinBox->blockSignals(true);
    transposeSpinBox->setValue(val);
    transposeSpinBox->blockSignals(false);
}
void DeicsOnzeGui::updatePOLYMODE(int val) {
    polyMonoComboBox->blockSignals(true);
    polyMonoComboBox->setCurrentIndex(val);
    polyMonoComboBox->blockSignals(false);
}
void DeicsOnzeGui::updatePBENDRANGE(int val) {
    pitchBendRangeSlider->blockSignals(true);
    pitchBendRangeSlider->setValue(val);
    pitchBendRangeSlider->blockSignals(false);
    pitchBendRangeSpinBox->blockSignals(true);
    pitchBendRangeSpinBox->setValue(val);
    pitchBendRangeSpinBox->blockSignals(false);
}
void DeicsOnzeGui::updatePORTAMODE(int val) {
    PortFingerFullComboBox->blockSignals(true);
    PortFingerFullComboBox->setCurrentIndex(val);
    PortFingerFullComboBox->blockSignals(false);
}
void DeicsOnzeGui::updatePORTATIME(int val) {
    PortamentoTimeSlider->blockSignals(true);
    PortamentoTimeSlider->setValue(val);
    PortamentoTimeSlider->blockSignals(false);
    PortamentoTimeSpinBox->blockSignals(true);
    PortamentoTimeSpinBox->setValue(val);
    PortamentoTimeSpinBox->blockSignals(false);
}
void DeicsOnzeGui::updateFIX(int op, bool val) {
    switch(op) {
	case 0:
	    Fix1CheckBox->blockSignals(true);
	    Fix1CheckBox->setChecked(val);
	    Fix1CheckBox->blockSignals(false);
	    FineRatio1SpinBox->blockSignals(true);
	    FineRatio1SpinBox->setEnabled(!val);
	    FineRatio1SpinBox->blockSignals(false);
	    CoarseRatio1SpinBox->blockSignals(true);
	    CoarseRatio1SpinBox->setEnabled(!val);
	    CoarseRatio1SpinBox->blockSignals(false);
	    break;
	case 1:
	    Fix2CheckBox->blockSignals(true);
	    Fix2CheckBox->setChecked(val);
	    Fix2CheckBox->blockSignals(false);
	    FineRatio2SpinBox->blockSignals(true);
	    FineRatio2SpinBox->setEnabled(!val);
	    FineRatio2SpinBox->blockSignals(false);
	    CoarseRatio2SpinBox->blockSignals(true);
	    CoarseRatio2SpinBox->setEnabled(!val);
	    CoarseRatio2SpinBox->blockSignals(false);
	    break;
	case 2:
	    Fix3CheckBox->blockSignals(true);
	    Fix3CheckBox->setChecked(val);
	    Fix3CheckBox->blockSignals(false);
	    FineRatio3SpinBox->blockSignals(true);
	    FineRatio3SpinBox->setEnabled(!val);
	    FineRatio3SpinBox->blockSignals(false);
	    CoarseRatio3SpinBox->blockSignals(true);
	    CoarseRatio3SpinBox->setEnabled(!val);
	    CoarseRatio3SpinBox->blockSignals(false);
	    break;
	case 3:
	    Fix4CheckBox->blockSignals(true);
	    Fix4CheckBox->setChecked(val);
	    Fix4CheckBox->blockSignals(false);
	    FineRatio4SpinBox->blockSignals(true);
	    FineRatio4SpinBox->setEnabled(!val);
	    FineRatio4SpinBox->blockSignals(false);
	    CoarseRatio4SpinBox->blockSignals(true);
	    CoarseRatio4SpinBox->setEnabled(!val);
	    CoarseRatio4SpinBox->blockSignals(false);
	    break;
	default: printf("DeicsOnzeGui::updateFIX : error switch\n");
    }
}
void DeicsOnzeGui::updateFIXRANGE(int op, int val) {
    switch(op) {
	case 0:
	    Freq1SpinBox->blockSignals(true);
	    Freq1SpinBox->setValue(val/100);
	    //val/100 because it is still a coarse display
	    Freq1SpinBox->blockSignals(false);
	    break;
	case 1:
	    Freq2SpinBox->blockSignals(true);
	    Freq2SpinBox->setValue(val/100);
	    Freq2SpinBox->blockSignals(false);
	    break;
	case 2:
	    Freq3SpinBox->blockSignals(true);
	    Freq3SpinBox->setValue(val/100);
	    Freq3SpinBox->blockSignals(false);
	    break;
	case 3:
	    Freq4SpinBox->blockSignals(true);
	    Freq4SpinBox->setValue(val/100);
	    Freq4SpinBox->blockSignals(false);
	    break;
	default: printf("DeicsOnzeGui::updateFIXRANGE : error switch\n");
    }
}
void DeicsOnzeGui::updateOSW(int op, int val) {
  switch(op) {
  case 0:
    WaveForm1ComboBox->blockSignals(true);
    WaveForm1ComboBox->setCurrentIndex(val);
    WaveForm1ComboBox->blockSignals(false);
    break;
  case 1:
    WaveForm2ComboBox->blockSignals(true);
    WaveForm2ComboBox->setCurrentIndex(val);
    WaveForm2ComboBox->blockSignals(false);
    break;
  case 2:
    WaveForm3ComboBox->blockSignals(true);
    WaveForm3ComboBox->setCurrentIndex(val);
    WaveForm3ComboBox->blockSignals(false);
    break;
  case 3:
    WaveForm4ComboBox->blockSignals(true);
    WaveForm4ComboBox->setCurrentIndex(val);
    WaveForm4ComboBox->blockSignals(false);
    break;
  default: printf("DeicsOnzeGui::updateOSW : Error switch\n");
  }
}
void DeicsOnzeGui::updateSHFT(int op, int val) {
  switch(op) {
  case 0:
    EGS1ComboBox->blockSignals(true);
    EGS1ComboBox->setCurrentIndex(val);
    EGS1ComboBox->blockSignals(false);
    break;
  case 1:
    EGS2ComboBox->blockSignals(true);
    EGS2ComboBox->setCurrentIndex(val);
    EGS2ComboBox->blockSignals(false);
    break;
  case 2:
    EGS3ComboBox->blockSignals(true);
    EGS3ComboBox->setCurrentIndex(val);
    EGS3ComboBox->blockSignals(false);
    break;
  case 3:
    EGS4ComboBox->blockSignals(true);
    EGS4ComboBox->setCurrentIndex(val);
    EGS4ComboBox->blockSignals(false);
    break;
  default: printf("DeicsOnzeGui::updateSHFT : Error switch\n");
  }
}
void DeicsOnzeGui::updateChannelDetune(int val) {
  updateChannelDetuneKnob(val);
  //updateChannelDetuneSlider(val);
}
void DeicsOnzeGui::updateChannelDetuneKnob(int val) {
  detuneKnob->blockSignals(true);
  detuneKnob->setValue((((double)val)/((double)MAXCHANNELDETUNE))/2.0+0.5);
  detuneKnob->blockSignals(false);
}
//void DeicsOnzeGui::updateChannelDetuneSlider(int val) {
  //channelDetuneSlider->blockSignals(true);
  //channelDetuneSlider->setValue(val);
  //channelDetuneSlider->blockSignals(false);
  //channelDetuneSpinBox->blockSignals(true);
  //channelDetuneSpinBox->setValue(val);
  //channelDetuneSpinBox->blockSignals(false);
//}
void DeicsOnzeGui::updateChannelVolume(int val) {
  channelVolumeKnob->blockSignals(true);
  channelVolumeKnob->setValue(((double)val)/(double)MAXCHANNELVOLUME);
  channelVolumeKnob->blockSignals(false);
}
void DeicsOnzeGui::updateCategoryName(QString cn, bool enable) {
  categoryLineEdit->setEnabled(enable);
  categoryLineEdit->blockSignals(true);
  categoryLineEdit->setText(cn);
  categoryLineEdit->blockSignals(false);
}
void DeicsOnzeGui::updateSubcategoryName(QString sn, bool enable) {
  subcategoryLineEdit->setEnabled(enable);
  subcategoryLineEdit->blockSignals(true);
  subcategoryLineEdit->setText(sn);
  subcategoryLineEdit->blockSignals(false);
}
void DeicsOnzeGui::updatePresetName(QString pn) {
  //presetNameLineEdit->blockSignals(true);
  //presetNameLineEdit->setText(pn);
  //presetNameLineEdit->blockSignals(false);
  //presetNameLabel->setText(pn);
  presetLineEdit->blockSignals(true);
  presetLineEdit->setText(pn);
  presetLineEdit->blockSignals(false);
}
void DeicsOnzeGui::updatePresetName(QString pn, bool enable) {
  presetLineEdit->setEnabled(enable);
  //presetNameLineEdit->setEnabled(enable);
  updatePresetName(pn);
}
void DeicsOnzeGui::updateHBank(int n, bool enable) {
  hbankSpinBox->setEnabled(enable);
  hbankSpinBox->blockSignals(true);
  hbankSpinBox->setValue(n+1);
  hbankSpinBox->blockSignals(false);
}
void DeicsOnzeGui::updateLBank(int n, bool enable) {
  lbankSpinBox->setEnabled(enable);
  lbankSpinBox->blockSignals(true);
  lbankSpinBox->setValue(n+1);
  lbankSpinBox->blockSignals(false);
}
void DeicsOnzeGui::updateProg(int n, bool enable) {
  progSpinBox->setEnabled(enable);
  progSpinBox->blockSignals(true);
  progSpinBox->setValue(n+1);
  progSpinBox->blockSignals(false);
}
void DeicsOnzeGui::updateInitSetCheckBox(bool b) {
  initSetCheckBox->blockSignals(true);
  initSetCheckBox->setChecked(b);
  initSetCheckBox->blockSignals(false);
  initSetPathLineEdit->setEnabled(b);
  initSetBrowsePushButton-> setEnabled(b);
}
void DeicsOnzeGui::updateInitSetPath(QString s) {
  initSetPathLineEdit->blockSignals(true);
  initSetPathLineEdit->setText(s);
  initSetPathLineEdit->blockSignals(false);
}
void DeicsOnzeGui::updateBackgroundPixCheckBox(bool b) {
  imageCheckBox->blockSignals(true);
  imageCheckBox->setChecked(b);
  imageCheckBox->blockSignals(false);
  imagePathLineEdit->setEnabled(b);
  imageBrowsePushButton-> setEnabled(b);
}
void DeicsOnzeGui::updateBackgroundPixPath(QString s) {
  imagePathLineEdit->blockSignals(true);
  imagePathLineEdit->setText(s);
  imagePathLineEdit->blockSignals(false);
}
void DeicsOnzeGui::applyBackgroundPix() {
  #ifdef DEICSONZE_DEBUG
  printf("applyBackgroundPix\n");
  #endif
  QPalette p = this->palette();
  QPixmap pixmap = QPixmap(imagePathLineEdit->text());
  p.setBrush((this)->backgroundRole(), QBrush(pixmap));
  (this)->setPalette(p);
}
void DeicsOnzeGui::updateChannelPan(int val) {
  channelPanKnob->blockSignals(true);
  channelPanKnob->setValue((((double)val/(double)MAXCHANNELPAN)+1.0)/2.0);
  channelPanKnob->blockSignals(false);
  //channelPanSpinBox->blockSignals(true);
  //channelPanSpinBox->setValue(val);
  //channelPanSpinBox->blockSignals(false);
}
void DeicsOnzeGui::updateBrightness(int val) {
  brightnessKnob->blockSignals(true);
  brightnessKnob->setValue((double)val/((double)MAXFINEBRIGHTNESS));
  brightnessKnob->blockSignals(false);
}
void DeicsOnzeGui::updateModulation(int val) {
  modulationKnob->blockSignals(true);
  modulationKnob->setValue((double)val/((double)MAXMODULATION));
  modulationKnob->blockSignals(false);
}
void DeicsOnzeGui::updateAttack(int val) {
  attackKnob->blockSignals(true);
  attackKnob->setValue((double)val/((double)MAXATTACK));
  attackKnob->blockSignals(false);
}
void DeicsOnzeGui::updateRelease(int val) {
  releaseKnob->blockSignals(true);
  releaseKnob->setValue((double)val/((double)MAXRELEASE));
  releaseKnob->blockSignals(false);
}
void DeicsOnzeGui::updateQuickEdit() {
  updateChannelVolume(_deicsOnze->getChannelVol(_currentChannel));
  updateChannelPan(_deicsOnze->getChannelPan(_currentChannel));
  updateBrightness(_deicsOnze->getChannelBrightness(_currentChannel));
  updateModulation(_deicsOnze->getChannelModulation(_currentChannel));
  updateChannelDetune(_deicsOnze->getChannelDetune(_currentChannel));
  updateAttack(_deicsOnze->getChannelAttack(_currentChannel));
  updateRelease(_deicsOnze->getChannelRelease(_currentChannel));
  updateChannelReverb(_deicsOnze->getChannelReverb(_currentChannel));
  updateChannelChorus(_deicsOnze->getChannelChorus(_currentChannel));
  updateChannelDelay(_deicsOnze->getChannelDelay(_currentChannel));
}
//--------------------------------------------------------------
// updatePreset
//--------------------------------------------------------------
void DeicsOnzeGui::updatePreset(Preset* p) {
    //TODO : why updateMasterVolume
    //updateMasterVolume(_deicsOnze->getMasterVol()); //to change
    updatePresetName(p->name.c_str());
    updateFEEDBACK(p->feedback);
    updateWAVE((int)p->lfo.wave);
    updateSPEED(p->lfo.speed);
    updateDELAY(p->lfo.delay);
    updatePMODDEPTH(p->lfo.pModDepth);
    updatePMODSENS(p->sensitivity.pitch);
    updateAMODDEPTH(p->lfo.aModDepth);
    updateAMS(p->sensitivity.amplitude);
    updateTRANSPOSE(p->function.transpose);
    updateALG((int)p->algorithm);
    updatePBENDRANGE(p->function.pBendRange);
    //pitch envelope
    PitchEg* pe=&(p->pitchEg);
    pitchEnvelopeGraph
      ->env2Points(pe->pl1, pe->pl2, pe->pl3, pe->pr1, pe->pr2, pe->pr3);
    pitchEnvelopeGraph->updateEnv();
    updatePL1(pe->pl1);
    updatePL2(pe->pl2);
    updatePL3(pe->pl3);
    updatePR1(pe->pr1);
    updatePR2(pe->pr2);
    updatePR3(pe->pr3);
    //function
    updateFcVolume(p->function.fcVolume);
    updateFcPitch(p->function.fcPitch);
    updateFcAmplitude(p->function.fcAmplitude);
    updateMwPitch(p->function.mwPitch);
    updateMwAmplitude(p->function.mwAmplitude);
    updateBcPitch(p->function.bcPitch);
    updateBcAmplitude(p->function.bcAmplitude);
    updateBcPitchBias(p->function.bcPitchBias);
    updateBcEgBias(p->function.bcEgBias);
    updateAtPitch(p->function.atPitch);
    updateAtAmplitude(p->function.atAmplitude);
    updateAtPitchBias(p->function.atPitchBias);
    updateAtEgBias(p->function.atEgBias);
    //updateReverbRate(p->function.reverbRate);
    updatePOLYMODE((int)p->function.mode);
    updatePORTAMODE((int)p->function.portamento);
    updatePORTATIME((int)p->function.portamentoTime);
    for(int k=0; k<NBROP; k++) {
	//envelope
	Eg* _eg=&(p->eg[k]);
	envelopeGraph[k]
	    ->env2Points(_eg->ar, _eg->d1r, _eg->d1l, _eg->d2r, _eg->rr);
	envelopeGraph[k]->updateEnv();
	updateAR(k, _eg->ar);
	updateD1R(k, _eg->d1r);
	updateD1L(k, _eg->d1l);
	updateD2R(k, _eg->d2r);
	updateRR(k, _eg->rr);
	//scaling
	updateLS(k, p->scaling.level[k]);
	updateRS(k, p->scaling.rate[k]);
	//Volume
	updateOUT(k, p->outLevel[k]);
	//Ratio and Frequency
	updateRATIO(k, (int)(100*p->frequency[k].ratio));
	updateFIXRANGE(k, (int)(100*p->frequency[k].freq));
	updateFIX(k, p->frequency[k].isFix);
	//Sensitivity
	updateAME(k, p->sensitivity.ampOn[k]);
	updateEBS(k, p->sensitivity.egBias[k]);
	updateKVS(k, p->sensitivity.keyVelocity[k]);
	//detune
	updateDET(k, p->detune[k]);
	//Waveform
	updateOSW(k, (int)p->oscWave[0]);
    }
}
/*void DeicsOnzeGui::updateCurrentChannel() {
  updateBrightness(_deicsOnze->_global.channel[_currentChannel].brightness);
  updateModulation(_deicsOnze->_global.channel[_currentChannel].modulation);
  updateChannelDetune(_deicsOnze->_global.channel[_currentChannel].detune);
  updateAttack(_deicsOnze->_global.channel[_currentChannel].attack);
  updateRelease(_deicsOnze->_global.channel[_currentChannel].release);
  }*/
void DeicsOnzeGui::updatePreset() {
  updatePreset(_deicsOnze->_preset[_currentChannel]);
}

void DeicsOnzeGui::updateSelectPreset(int hbank, int lbank, int prog) {
  //QTreeWidgetItem* cat = categoryListView->currentItem();
  //QTreeWidgetItem* sub = subcategoryListView->currentItem();
  //QTreeWidgetItem* pre = presetListView->currentItem();
  //select category, subcategory, preset
  //category
  QList<QTreeWidgetItem *> qlcat =
    categoryListView->findItems(num3Digits(hbank+1), Qt::MatchExactly);
  QTreeWidgetItem* qcat = qlcat.empty()? NULL:qlcat.at(0);
  //if the category is different than the last one then select the new one
  //if(!cat || !qcat || qcat!= cat) {
  if(qcat) {
    categoryListView->setItemSelected(qcat, true);
    categoryListView->setCurrentItem(qcat);
    categoryListView->scrollToItem(qcat);
    setEnabledPreset(true);
  }
  else {
    updateCategoryName(QString("NONE"), false);
    updateHBank(hbank, false);
    categoryListView->clearSelection();
    subcategoryListView->clear();
    setEnabledPreset(false);
  }
  //}
  //subcategory
  //if(cat) {
  QList<QTreeWidgetItem *> qlsub =
    subcategoryListView->findItems(num3Digits(lbank+1), Qt::MatchExactly);
  QTreeWidgetItem* qsub = qlsub.empty()? NULL:qlsub.at(0);
  //  if(!sub || qsub!=sub) {
  if(qsub) {
    subcategoryListView->setItemSelected(qsub, true);
    subcategoryListView->setCurrentItem(qsub);
    subcategoryListView->scrollToItem(qsub);
    setEnabledPreset(true);
  }
  else {
    updateSubcategoryName(QString("NONE"), false);
    updateLBank(lbank, false);
    subcategoryListView->clearSelection();
    presetListView->clear();
    setEnabledPreset(false);
  }
  //  }
  //}
  //else {
  //  updateSubcategoryName(QString("NONE"), false);
  //  updateLBank(lbank, false);
  //  subcategoryListView->clearSelection();
  //  presetListView->clear();
  //  setEnabledPreset(false);
  //}
  //preset
  //if(sub) {
  QList<QTreeWidgetItem *> qlpre =
    presetListView->findItems(num3Digits(prog+1), Qt::MatchExactly);
  QTreeWidgetItem* qpre = qlpre.empty()? NULL:qlpre.at(0);
  if(qpre) {
    presetListView->blockSignals(true);
    presetListView->setItemSelected(qpre, true);
    presetListView->setCurrentItem(qpre);
    presetListView->blockSignals(false);
    presetListView->scrollToItem(qpre);
    updatePresetName(qpre->text(1), true);
    updateProg(prog, true);
    //pre=(QTreePreset*) qpre;
    setEnabledPreset(true);
  }
  else {
    updatePresetName(QString("INITVOICE"), false);
    updateProg(prog, false);
    presetListView->clearSelection();
    setEnabledPreset(false);
  }
  //}
  //else {
  //  updatePresetName(QString("INITVOICE"), false);
  //  updateProg(prog, false);
  //  presetListView->clearSelection();
  //  setEnabledPreset(false);
  //}
}

