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

#include "deicsonzeplugin.h"
#include "plugin.h"
///#include "plugingui.h"
#include "ctrl.h"
#include "fastlog.h"
#include "muse/midi.h"
#include "awl/floatentry.h"
#include "awl/slider.h"
#include "awl/checkbox.h"

using Awl::FloatEntry;
using Awl::Slider;
using Awl::CheckBox;

class PluginDialog;

void DeicsOnze::initPluginReverb(MusECore::Plugin* pluginReverb) {
  //init plugin
  if(_pluginIReverb) delete(_pluginIReverb);
  ///_pluginIReverb = new MusECore::PluginI(NULL);
  _pluginIReverb = new MusECore::PluginI();

  _pluginIReverb->initPluginInstance(pluginReverb, 2);

  //for(int i = 0; i < pluginReverb->parameter(); i++) {
  for(int i = 0; i < (int)pluginReverb->controlInPorts(); i++) {
    
// FIXME FIXME Tim
/*    Ctrl* c = new Ctrl();
    c->setCurVal((float)pluginReverb->defaultValue(i));
    _pluginIReverb->setControllerList(c);
*/
    
    //setReverbParam(i, pluginReverb->defaultValue(i));
  }

  //send build gui to the gui
  char data;
  data = SYSEX_BUILDGUIREVERB;
  MusECore::MidiPlayEvent evSysex(0, 0,MusECore::ME_SYSEX,(const unsigned char*)&data, 1);
  _gui->writeEvent(evSysex);
}

void DeicsOnze::initPluginChorus(MusECore::Plugin* pluginChorus) {
  if(_pluginIChorus) delete(_pluginIChorus);
  ///_pluginIChorus = new MusECore::PluginI(NULL);
  _pluginIChorus = new MusECore::PluginI();

  _pluginIChorus->initPluginInstance(pluginChorus, 2);

  //for(int i = 0; i < pluginChorus->parameter(); i++) {
  for(int i = 0; i < (int)pluginChorus->controlInPorts(); i++) {

// FIXME FIXME Tim
/*    
    Ctrl* c = new Ctrl();
    c->setCurVal((float)pluginChorus->defaultValue(i));
    _pluginIChorus->setControllerList(c);
*/    
    
    //setChorusParam(i, pluginChorus->defaultValue(i));
}

  //send build gui to the gui
  char data;
  data = SYSEX_BUILDGUICHORUS;
  MusECore::MidiPlayEvent evSysex(0, 0,MusECore::ME_SYSEX,(const unsigned char*)&data, 1);
  _gui->writeEvent(evSysex);
}

void DeicsOnze::initPluginDelay(MusECore::Plugin* pluginDelay) {
  if(_pluginIDelay) delete(_pluginIDelay);
  ///_pluginIDelay = new MusECore::PluginI(NULL);
  _pluginIDelay = new MusECore::PluginI();

  _pluginIDelay->initPluginInstance(pluginDelay, 2);

  //for(int i = 0; i < pluginDelay->parameter(); i++) { 
  for(int i = 0; i < (int)pluginDelay->controlInPorts(); i++) { 

// FIXME FIXME Tim
/*    
    Ctrl* c = new Ctrl();
    c->setCurVal((float)pluginDelay->defaultValue(i));
    _pluginIDelay->setControllerList(c);
*/    

    //setChorusParam(i, pluginDelay->defaultValue(i));
  }
  setDelayDryWet(1);
  
  float f;
  char dataDelayBPM[sizeof(float)+1];
  dataDelayBPM[0] = SYSEX_DELAYBPM;
  f = getDelayBPM();
  memcpy(&dataDelayBPM[1], &f, sizeof(float));
  MusECore::MidiPlayEvent evSysexDelayBPM(0, 0,MusECore::ME_SYSEX,
			    (const unsigned char*)dataDelayBPM,
			    sizeof(float)+1);
  _gui->writeEvent(evSysexDelayBPM);
  char dataDelayBeatRatio[sizeof(float)+1];
  dataDelayBeatRatio[0] = SYSEX_DELAYBEATRATIO;
  f = getDelayBeatRatio();
  memcpy(&dataDelayBeatRatio[1], &f, sizeof(float));
  MusECore::MidiPlayEvent evSysexDelayBeatRatio(0, 0,MusECore::ME_SYSEX,
				  (const unsigned char*)dataDelayBeatRatio,
				  sizeof(float)+1);
  _gui->writeEvent(evSysexDelayBeatRatio);
  char dataDelayFeedback[sizeof(float)+1];
  dataDelayFeedback[0] = SYSEX_DELAYFEEDBACK;
  f = getDelayFeedback();
  memcpy(&dataDelayFeedback[1], &f, sizeof(float));
  MusECore::MidiPlayEvent evSysexDelayFeedback(0, 0,MusECore::ME_SYSEX,
				 (const unsigned char*)dataDelayFeedback,
				 sizeof(float)+1);
  _gui->writeEvent(evSysexDelayFeedback);
  char dataDelayLFOFreq[sizeof(float)+1];
  dataDelayLFOFreq[0] = SYSEX_DELAYLFOFREQ;
  f = getDelayLFOFreq();
  memcpy(&dataDelayLFOFreq[1], &f, sizeof(float));
  MusECore::MidiPlayEvent evSysexDelayLFOFreq(0, 0,MusECore::ME_SYSEX,
				(const unsigned char*)dataDelayLFOFreq,
				sizeof(float)+1);
  _gui->writeEvent(evSysexDelayLFOFreq);
  char dataDelayLFODepth[sizeof(float)+1];
  dataDelayLFODepth[0] = SYSEX_DELAYLFODEPTH;
  f = getDelayLFODepth();
  memcpy(&dataDelayLFODepth, &f, sizeof(float)+1);
  MusECore::MidiPlayEvent evSysexDelayLFODepth(0, 0,MusECore::ME_SYSEX,
				 (const unsigned char*)dataDelayLFODepth,
				 sizeof(float)+1);
  _gui->writeEvent(evSysexDelayLFODepth); 
}

void DeicsOnze::setReverbParam(int index, double val) {
  ///if(_pluginIReverb) _pluginIReverb->controller(index)->setCurVal((float)val);
  if(_pluginIReverb) _pluginIReverb->setParam(index, val);
  else printf("Warning : no DeicsOnze reverb loaded\n");
}
void DeicsOnze::setChorusParam(int index, double val) {
  ///if(_pluginIChorus) _pluginIChorus->controller(index)->setCurVal((float)val);
  if(_pluginIChorus) _pluginIChorus->setParam(index, val);
  else printf("Warning : no DeicsOnze chorus loaded\n");
}

double DeicsOnze::getReverbParam(int index) const {
  ///if(_pluginIReverb) return _pluginIReverb->controller(index)->curVal().f; 
  if(_pluginIReverb) return _pluginIReverb->param(index); 
  else {
    return 0.0;
    printf("Warning : no DeicsOnze reverb loaded\n");  
  }
}

double DeicsOnze::getChorusParam(int index) const {
  ///if(_pluginIChorus) return _pluginIChorus->controller(index)->curVal().f;
  if(_pluginIChorus) return _pluginIChorus->param(index);
  else {
    return 0.0;
    printf("Warning : no DeicsOnze chorus loaded\n");  
  }    
}

void DeicsOnzeGui::addPluginCheckBox(int index, QString text, bool toggled,
				     QWidget* parent, QGridLayout* grid,
				     bool isReverb) {
  CheckBox* cb = new CheckBox(parent);
  cb->setId(index);
  cb->setText(text);
  cb->setChecked(toggled);
  grid->addWidget(cb, index, 0);
  //push on vectors
  if(isReverb) {
    _reverbSliderVector.push_back(NULL);
    _reverbFloatEntryVector.push_back(NULL);
    _reverbCheckBoxVector.push_back(cb);
  }
  else {
    _chorusSliderVector.push_back(NULL);
    _chorusFloatEntryVector.push_back(NULL);
    _chorusCheckBoxVector.push_back(cb);
  }
  //connect slots
  if(isReverb) {
    connect(cb, SIGNAL(valueChanged(double, int)),
	    this, SLOT(setReverbCheckBox(double, int)));
  }
  else {
    connect(cb, SIGNAL(valueChanged(double, int)),
	    this, SLOT(setChorusCheckBox(double, int)));
  }
}

void DeicsOnzeGui::addPluginIntSlider(int index, QString text, double min,
				      double max, double val, QWidget* parent,
				      QGridLayout* grid, bool isReverb) {
  addPluginSlider(index, text, false, min, max, val, parent, grid, isReverb);
}

void DeicsOnzeGui::addPluginSlider(int index, QString text, bool isLog,
				   double min, double max, double val,
				   QWidget* parent, QGridLayout* grid,
				   bool isReverb) {
  QLabel* l = new QLabel(text, parent);
  grid->addWidget(l, index, 0);
  FloatEntry* f = new FloatEntry(parent);
  f->setValue(val);
  f->setMinValue(min);
  f->setMaxValue(max);
  f->setMaximumWidth(72);
  grid->addWidget(f, index, 1);
  Slider* s = new Slider(parent);
  s->setId(index);
  s->setLog(isLog);
  s->setLogRange(min, max);
  s->setValue(val);
  s->setOrientation(Qt::Horizontal);
  //s->setFixedHeight(h);
  s->setLineStep((min-max)/100.0);
  s->setPageStep((min-max)/10.0);
  grid->addWidget(s, index, 2);
  //push on vectors
  if(isReverb) {
    _reverbSliderVector.push_back(s);
    _reverbFloatEntryVector.push_back(f);
    _reverbCheckBoxVector.push_back(NULL);
  }
  else {
    _chorusSliderVector.push_back(s);
    _chorusFloatEntryVector.push_back(f);
    _chorusCheckBoxVector.push_back(NULL);
  }
  //connect slots
  if(isReverb) {
    connect(f, SIGNAL(valueChanged(double, int)),
	    this, SLOT(setReverbFloatEntry(double, int)));
    connect(s, SIGNAL(valueChanged(double, int)),
	    this, SLOT(setReverbSlider(double, int)));
  }
  else {
    connect(f, SIGNAL(valueChanged(double, int)),
	    this, SLOT(setChorusFloatEntry(double, int)));
    connect(s, SIGNAL(valueChanged(double, int)),
	    this, SLOT(setChorusSlider(double, int)));
  }
}

void DeicsOnzeGui::buildGuiReverb() {
  MusECore::PluginI* plugI = _deicsOnze->_pluginIReverb;
  QString name = plugI->name();
  name.resize(name.size()-2);
  updateLadspaReverbLineEdit(name);
  //build super layout
  if(parametersReverbGroupBox->layout())
    delete(parametersReverbGroupBox->layout());
  QGridLayout* superLayout = new QGridLayout(parametersReverbGroupBox);
  parametersReverbGroupBox->setLayout(superLayout);
  //build super widget
  if(_reverbSuperWidget) delete(_reverbSuperWidget);
  _reverbSuperWidget = new QWidget(parametersReverbGroupBox);
  superLayout->addWidget(_reverbSuperWidget);
  //build grid
  QGridLayout* grid = new QGridLayout(_reverbSuperWidget);
  _reverbSuperWidget->setLayout(grid);
  grid->setSpacing(0);
  //init vectors
  if(!_reverbSliderVector.empty()) _reverbSliderVector.clear();
  if(!_reverbFloatEntryVector.empty()) _reverbFloatEntryVector.clear();  
  if(!_reverbCheckBoxVector.empty()) _reverbCheckBoxVector.clear();  
  //build sliders                                         
  //for(int i = 0; i < plugI->plugin()->parameter(); i++) { 
  for(int i = 0; i < (int)plugI->plugin()->controlInPorts(); i++) { 
    ///double min, max, val;
    float min, max; //, val;
    plugI->range(i, &min, &max);

// FIXME FIXME Tim
/*     
    val = _deicsOnze->getReverbParam(i);
     if(plugI->isBool(i))
      addPluginCheckBox(i, plugI->getParameterName(i), val > 0.0,
			_reverbSuperWidget, grid, true);
    else if(plugI->isInt(i)) {
      addPluginIntSlider(i, plugI->getParameterName(i), rint(min), rint(max),
			 rint(val), _reverbSuperWidget, grid, true);
    }
    else {
      addPluginSlider(i, plugI->getParameterName(i), plugI->isLog(i),
		      min, max, val, _reverbSuperWidget, grid, true);
    }
*/    
    
  }
  //update colors of the new sliders (and the whole gui actually)
  setEditTextColor(reinterpret_cast<const QColor &>(*etColor));
  setEditBackgroundColor(reinterpret_cast<const QColor &>(*ebColor));
}

void DeicsOnzeGui::buildGuiChorus() {
  MusECore::PluginI* plugI = _deicsOnze->_pluginIChorus;
  QString name = plugI->name();
  name.resize(name.size()-2);
  updateLadspaChorusLineEdit(name);
  //build super layout
  if(parametersChorusGroupBox->layout())
    delete(parametersChorusGroupBox->layout());
  QGridLayout* superLayout = new QGridLayout(parametersChorusGroupBox);
  parametersChorusGroupBox->setLayout(superLayout);
  //build super widget
  if(_chorusSuperWidget) delete(_chorusSuperWidget);
  _chorusSuperWidget = new QWidget(parametersChorusGroupBox);
  superLayout->addWidget(_chorusSuperWidget);
  //build grid
  QGridLayout* grid = new QGridLayout(_chorusSuperWidget);
  _chorusSuperWidget->setLayout(grid);
  grid->setSpacing(2);
  //init vectors
  if(!_chorusSliderVector.empty()) _chorusSliderVector.clear();
  if(!_chorusFloatEntryVector.empty()) _chorusFloatEntryVector.clear();
  if(!_chorusCheckBoxVector.empty()) _chorusCheckBoxVector.clear();  
  //build sliders                              
  //for(int i = 0; i < plugI->plugin()->parameter(); i++) {
  for(int i = 0; i < (int)plugI->plugin()->controlInPorts(); i++) {
    ///double min, max, val;
    float min, max; //, val;
    plugI->range(i, &min, &max);
    
// FIXME FIXME Tim
/*    
    val = _deicsOnze->getChorusParam(i);
    if(plugI->isBool(i))
      addPluginCheckBox(i, plugI->getParameterName(i), val > 0.0,
			_chorusSuperWidget, grid, false);
    else if(plugI->isInt(i)) {
      addPluginIntSlider(i, plugI->getParameterName(i), rint(min), rint(max),
			 rint(val), _chorusSuperWidget, grid, false);
    }
    else {
      addPluginSlider(i, plugI->getParameterName(i), plugI->isLog(i),
		      min, max, val, _chorusSuperWidget, grid, false);
    }
*/    
    
  }
  //update colors of the new sliders (and the whole gui actually)
  setEditTextColor(reinterpret_cast<const QColor &>(*etColor));
  setEditBackgroundColor(reinterpret_cast<const QColor &>(*ebColor));
}

//setReverbCheckBox is used, by the way, to send the value
//of the parameter because it sends a double and does not
//change any thing
void DeicsOnzeGui::setReverbCheckBox(double v, int i) {
  float f = (float)v;
  unsigned char* message = new unsigned char[2+sizeof(float)];
  message[0]=SYSEX_REVERBPARAM;
  if(i<256) {
    message[1]=(unsigned char)i;
    memcpy(&message[2], &f, sizeof(float));
    sendSysex(message, 2+sizeof(float));
  }
  else printf("setReverbCheckBox Error : cannot send controller upper than 225\n");
}

//setChorusCheckBox is used, by the way, to send the value
//of the parameter because it sends a double and does not
//change any thing
void DeicsOnzeGui::setChorusCheckBox(double v, int i) {
  float f = (float)v;
  unsigned char* message = new unsigned char[2+sizeof(float)];
  message[0]=SYSEX_CHORUSPARAM;
  if(i<256) {
    message[1]=(unsigned char)i;
    memcpy(&message[2], &f, sizeof(float));
    sendSysex(message, 2+sizeof(float));
  }
  else printf("setChorusCheckBox Error : cannot send controller upper than 225\n");
}

void DeicsOnzeGui::setReverbFloatEntry(double /*v*/, int /*i*/) {
  if(_deicsOnze->_pluginIReverb) {
    
// FIXME FIXME Tim
/*    
    if(_deicsOnze->_pluginIReverb->isInt(i)) v = rint(v);
    updateReverbFloatEntry(v, i);  
    updateReverbSlider(v, i);
    setReverbCheckBox(v, i); //because this send the SYSEX
*/    
    
  }
  else printf("Warning : no DeicsOnze reverb loaded\n");
}
void DeicsOnzeGui::setReverbSlider(double /*v*/, int /*i*/) {
  if(_deicsOnze->_pluginIReverb) {
    
// FIXME FIXME Tim
/*
    if(_deicsOnze->_pluginIReverb->isInt(i)) v = rint(v);
    updateReverbFloatEntry(v, i);
    updateReverbSlider(v, i);
    setReverbCheckBox(v, i); //because this send the SYSEX
*/
    
  }
  else printf("Warning : no DeicsOnze reverb loaded\n");
}
void DeicsOnzeGui::setChorusFloatEntry(double /*v*/, int /*i*/) {
  if(_deicsOnze->_pluginIReverb) {
    
// FIXME FIXME Tim
/*
    if(_deicsOnze->_pluginIChorus->isInt(i)) v = rint(v);
    updateChorusFloatEntry(v, i);
    updateChorusSlider(v, i);
    setChorusCheckBox(v, i); //because this send the SYSEX
*/
    
  }
  else printf("Warning : no DeicsOnze chorus loaded\n");
}
void DeicsOnzeGui::setChorusSlider(double /*v*/, int /*i*/) {
  if(_deicsOnze->_pluginIReverb) {  
    
// FIXME FIXME Tim
/*
    if(_deicsOnze->_pluginIChorus->isInt(i)) v = rint(v);
    updateChorusSlider(v, i);
    updateChorusFloatEntry(v, i);
    setChorusCheckBox(v, i); //because this send the SYSEX
*/    
    
  }
  else printf("Warning : no DeicsOnze chorus loaded\n");
}

//updates
void DeicsOnzeGui::updateReverbSlider(double v, int i) {
  if(i < (int)_reverbSliderVector.size() && _reverbSliderVector[i]) {
    _reverbSliderVector[i]->blockSignals(true);
    _reverbSliderVector[i]->setValue(v);
    _reverbSliderVector[i]->blockSignals(false);
  }
}
void DeicsOnzeGui::updateReverbFloatEntry(double v, int i) {
  if(i < (int)_reverbFloatEntryVector.size() && _reverbFloatEntryVector[i]) {
    _reverbFloatEntryVector[i]->blockSignals(true);
    _reverbFloatEntryVector[i]->setValue(v);
    _reverbFloatEntryVector[i]->blockSignals(false);
  }
}
void DeicsOnzeGui::updateChorusSlider(double v, int i) {
  if(i < (int)_reverbSliderVector.size() && _reverbSliderVector[i]) {
    _chorusSliderVector[i]->blockSignals(true);
    _chorusSliderVector[i]->setValue(v);
    _chorusSliderVector[i]->blockSignals(false);
  }
}
void DeicsOnzeGui::updateChorusFloatEntry(double v, int i) {
  if(i < (int)_chorusFloatEntryVector.size() && _chorusFloatEntryVector[i]) {
    _chorusFloatEntryVector[i]->blockSignals(true);
    _chorusFloatEntryVector[i]->setValue(v);
    _chorusFloatEntryVector[i]->blockSignals(false);
  }
}

//-------------------------------------------------------------
// set Delay
//-------------------------------------------------------------
void DeicsOnze::setDelayBPM(float val) {
  ///if(_pluginIDelay) _pluginIDelay->controller(0)->setCurVal(val);
  if(_pluginIDelay) _pluginIDelay->setParam(0, val);
  else printf("Warning : no DeicsOnze delay loaded\n");
}
void DeicsOnze::setDelayBeatRatio(float val) {
  ///if(_pluginIDelay) _pluginIDelay->controller(1)->setCurVal(val);
  if(_pluginIDelay) _pluginIDelay->setParam(1, val);
  else printf("Warning : no DeicsOnze delay loaded\n");
}
float DeicsOnze::getDelayBPM() const {
  ///if(_pluginIDelay) return _pluginIDelay->controller(0)->curVal().f;
  if(_pluginIDelay) return _pluginIDelay->param(0);
  else {
    printf("Warning : no DeicsOnze delay loaded\n");
    return 0.0;
  }
}
float DeicsOnze::getDelayBeatRatio() const {
  ///if(_pluginIDelay) return _pluginIDelay->controller(1)->curVal().f;
  if(_pluginIDelay) return _pluginIDelay->param(1);
  else {
    printf("Warning : no DeicsOnze delay loaded\n");
    return 0.0;
  }
}
void DeicsOnze::setDelayFeedback(float val) {
  ///if(_pluginIDelay) return _pluginIDelay->controller(2)->setCurVal(val);
  if(_pluginIDelay) return _pluginIDelay->setParam(2, val);
  else printf("Warning : no DeicsOnze delay loaded\n");
}
float DeicsOnze::getDelayFeedback() const {
  ///if(_pluginIDelay) return _pluginIDelay->controller(2)->curVal().f;
  if(_pluginIDelay) return _pluginIDelay->param(2);
  else {
    printf("Warning : no DeicsOnze delay loaded\n");
    return 0.0;
  }
}
void DeicsOnze::setDelayLFOFreq(float val) {
  ///if(_pluginIDelay) _pluginIDelay->controller(3)->setCurVal(val);
  if(_pluginIDelay) _pluginIDelay->setParam(3, val);
  else printf("Warning : no DeicsOnze delay loaded\n");
}
float DeicsOnze::getDelayLFOFreq() const {
  ///if(_pluginIDelay) return _pluginIDelay->controller(3)->curVal().f;
  if(_pluginIDelay) return _pluginIDelay->param(3);
  else {
    printf("Warning : no DeicsOnze delay loaded\n");
    return 0.0;
  }
}
void DeicsOnze::setDelayLFODepth(float val) {
  ///if(_pluginIDelay) _pluginIDelay->controller(4)->setCurVal(val);
  if(_pluginIDelay) _pluginIDelay->setParam(4, val);
  else printf("Warning : no DeicsOnze delay loaded\n");
}
float DeicsOnze::getDelayLFODepth() const {
  ///if(_pluginIDelay) return _pluginIDelay->controller(4)->curVal().f;
  if(_pluginIDelay) return _pluginIDelay->param(4);
  else {
    printf("Warning : no DeicsOnze delay loaded\n");
    return 0.0;
  }
}
void DeicsOnze::setDelayDryWet(float val) {
  ///if(_pluginIDelay) _pluginIDelay->controller(5)->setCurVal(val);
  if(_pluginIDelay) _pluginIDelay->setParam(5, val);
  else printf("Warning : no DeicsOnze delay loaded\n");
}
