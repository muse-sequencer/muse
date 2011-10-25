//
// C++ Implementation: testogui
//
// Description:
//
//
// Author: Mathias Lundgren <lunar_shuttle@users.sf.net>, (C) 2004
//  Contributer: (C) Copyright 2011 Tim E. Real (terminator356 at users.sourceforge.net)
//
//  This program is free software; you can redistribute it and/or
//  modify it under the terms of the GNU General Public License
//  as published by the Free Software Foundation; version 2 of
//  the License, or (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
//
//
//

#include <QButtonGroup>
#include <QLabel>
#include <QFileDialog>
#include <QSocketNotifier>
#include <QLayout>
#include <QToolTip>
#include <QLineEdit>
#include <QMessageBox>

#include "common_defs.h"
#include "simpledrumsgui.h"
//#include "libsynti/mpevent.h"
#include "muse/mpevent.h"   
#include "muse/midi.h"
#include "ssplugingui.h"

#define SS_VOLUME_MIN_VALUE                     0
#define SS_VOLUME_MAX_VALUE                   127
#define SS_VOLUME_DEFAULT_VALUE               100
#define SS_MASTERVOL_MAX_VALUE                127
#define SS_MASTERVOL_DEFAULT_VALUE    100.0/127.0
#define SS_SENDFX_MIN_VALUE                     0
#define SS_SENDFX_MAX_VALUE                   127

//Gui constants:
#define SS_BTNGRP_WIDTH                        50
#define SS_BTNGRP_HEIGHT                       80
#define SS_ONOFF_WIDTH                         16
#define SS_ONOFF_HEIGHT                        21
#define SS_VOLSLDR_WIDTH                       (SS_BTNGRP_WIDTH - 8)
#define SS_VOLSLDR_LENGTH                     120
#define SS_PANSLDR_WIDTH                       (SS_BTNGRP_WIDTH - 8)
#define SS_PANSLDR_LENGTH                      20
#define SS_PANSLDR_DEFAULT_VALUE               63
#define SS_NONOFF_LABEL_WIDTH                  30
#define SS_NONOFF_LABEL_HEIGHT                 16
#define SS_NONOFF_WIDTH                        SS_ONOFF_WIDTH
#define SS_NONOFF_HEIGHT                       SS_ONOFF_HEIGHT
#define SS_SENDFX_WIDTH                        ((SS_BTNGRP_WIDTH/2) - 4)
//#define SS_SENDFX_WIDTH                        28
#define SS_SENDFX_HEIGHT                       SS_SENDFX_WIDTH
#define SS_MASTERSLDR_WIDTH                    (SS_BTNGRP_WIDTH - 8)
#define SS_MASTERSLDR_HEIGHT                   (SS_BTNGRP_HEIGHT - 4)


// Sample groupbox

#define SS_SAMPLENAME_LABEL_WIDTH              30
#define SS_SAMPLENAME_LABEL_HEIGHT             21
#define SS_SAMPLENAME_LABEL_XOFF                4

#define SS_SAMPLE_LOAD_WIDTH                   15
#define SS_SAMPLE_LOAD_HEIGHT                  19

#define SS_SAMPLE_CLEAR_WIDTH                   SS_SAMPLE_LOAD_WIDTH
#define SS_SAMPLE_CLEAR_HEIGHT                  SS_SAMPLE_LOAD_HEIGHT

#define SS_SAMPLENAME_LINEEDIT_WIDTH           90
#define SS_SAMPLENAME_LINEEDIT_HEIGHT          21

#define SS_SAMPLE_INFO_LINE_HEIGHT             22
#define SS_SAMPLE_INFO_LINE_WIDTH               (SS_SAMPLENAME_LINEEDIT_XOFF + SS_SAMPLENAME_LINEEDIT_WIDTH)

#define SS_GUI_WINDOW_WIDTH                     ((SS_NR_OF_CHANNELS +1) * SS_BTNGRP_XOFF)
#define SS_MAIN_GROUPBOX_HEIGHT                 200
#define SS_GUI_WINDOW_HEIGHT                    (SS_BTNGRP_HEIGHT + SS_MAIN_GROUPBOX_HEIGHT)
#define SS_MAIN_GROUPBOX_WIDTH                  SS_GUI_WINDOW_WIDTH

SimpleSynthGui* simplesynthgui_ptr;


/*!
    \fn QChannelSlider::QChannelSlider(Qt::Orientation orientation, int ch, QWidget* parent, const char* name)
 */
QChannelSlider::QChannelSlider(Qt::Orientation orientation, int ch, QWidget* parent)
      : QSlider(orientation, parent)
{
    channel = ch;
}

void QChannelSlider::sliderChange(SliderChange change)
{
  QSlider::sliderChange(change);
  if(change == QAbstractSlider::SliderValueChange)
    emit valueChanged(channel, value());  
}

/*!
    \fn QChannelSlider::getChannel()
 */
int QChannelSlider::getChannel()
      {
      return channel;
      }


/*!
    \fn QChannelSlider::setChannel(int ch)
 */
void QChannelSlider::setChannel(int ch)
      {
      channel = ch;
      }

/*!
    \fn QChannelSlider::setValue(int val)
 */
/*
void QChannelSlider::setValue(int val)
      {
      val = (val > 127 ? 127 : val);
      val = (val < 0 ? 0 : val);
      QSlider::setValue(val);
      emit valueChanged(channel, val);
      }
*/

QInvertedSlider::QInvertedSlider(Qt::Orientation o, QWidget* parent)
         : QSlider(o, parent) 
{
  setInvertedAppearance(true);    // p4.0.27 
}

void QInvertedSlider::sliderChange(SliderChange change)
{
  QSlider::sliderChange(change);
  if(change == QAbstractSlider::SliderValueChange)
    emit invertedValueChanged(value());  
}

QInvertedChannelSlider::QInvertedChannelSlider(Qt::Orientation o, int channel, QWidget* parent)
         : QChannelSlider(o, channel, parent)
{
  setInvertedAppearance(true);    // p4.0.27 
  //setInvertedControls(true);
}

/*!
    \fn QInvertedChannelSlider::setValue(int val)
 */
/*
void QInvertedChannelSlider::setValue(int val)
      {
      int inverted = this->maximum() - val;
      inverted = (inverted > 127 ? 127 : inverted);
      inverted = (inverted < 0 ? 0 : inverted);
      QSlider::setValue(val);
      emit valueChanged(channel, inverted);
      }
*/

/*!
    \fn QInvertedSlider::setValue(int val)
 */
/*
void QInvertedSlider::setValue(int val)
      {
      int inverted = this->maximum() - val;
      inverted = (inverted > 127 ? 127 : inverted);
      inverted = (inverted < 0 ? 0 : inverted);
      emit invertedValueChanged(inverted);
      QSlider::setValue(val);
      }
*/

/*!
    \fn QChannelCheckbox::QChannelCheckbox(QWidget* parent, int ch)
 */
QChannelCheckbox::QChannelCheckbox(QWidget* parent, int ch)
   : QCheckBox(parent)
      {
      channel = ch;
      connect(this, SIGNAL(clicked()), SLOT(isClicked()));
      }


/*!
    \fn QChannelCheckbox::isClicked()
 */
void QChannelCheckbox::isClicked()
      {
      emit channelState(channel, this->isChecked());
      }

/*!
    \fn QChannelButton::QChannelButton(QWidget* parent, const char* text, int ch, const char* name)
 */
QChannelButton::QChannelButton(QWidget* parent, const char* text, int ch)
      : QPushButton(parent), channel (ch)
      {
      connect(this, SIGNAL(clicked()), SLOT(isClicked()));
      setText(text);
      }

/*!
    \fn QChannelButton::isClicked()
 */
void QChannelButton::isClicked()
      {
      emit channelState(channel, this->isChecked());
      }

/*!
    \fn QChannelDial()
 */
QChannelDial::QChannelDial(QWidget* parent, int ch, int fxid)
   : QDial(parent)
      {
      setTracking(true);
      channel = ch;
      sendfxid = fxid;
      }

/*!
    \fn QChannelSlider::setValue(int val)
 */
/*
void QChannelDial::setValue(int val)
      {
      QDial::setValue(val);
      emit valueChanged(channel, sendfxid, val);
      }
*/

void QChannelDial::sliderChange(SliderChange change)
{
  QDial::sliderChange(change);
  if(change == QAbstractSlider::SliderValueChange)
    emit valueChanged(channel, sendfxid, value());  
}

/*!
    \fn SimpleSynthGui::SimpleSynthGui()
 */
SimpleSynthGui::SimpleSynthGui()
      {
      SS_TRACE_IN
      setupUi(this);
      simplesynthgui_ptr = this;
      pluginGui = new SS_PluginGui(this);
      pluginGui->hide();

      QVBoxLayout* mainLayout = new QVBoxLayout(this); //, 3);
      QHBoxLayout* channelLayout = new QHBoxLayout;
      mainLayout->addLayout(channelLayout);

      //this->setFixedWidth(SS_GUI_WINDOW_WIDTH);
      //this->setFixedHeight(SS_GUI_WINDOW_HEIGHT);
      for (int i=0; i<SS_NR_OF_CHANNELS; i++) {
            channelButtonGroups[i] = new QGroupBox(this);
//            channelButtonGroups[i]->setMinimumSize(SS_BTNGRP_WIDTH, SS_BTNGRP_HEIGHT);
            channelButtonGroups[i]->setTitle(QString::number(i + 1));

            QString name = QString("volumeSlider");
            name.append(i + 1);

            channelLayout->addWidget(channelButtonGroups[i]);

            QVBoxLayout* inchnlLayout = new QVBoxLayout(channelButtonGroups[i]); //, 2, 0, "channelinternallayout");
            inchnlLayout->setAlignment(Qt::AlignHCenter);

            onOff[i] = new QChannelCheckbox(channelButtonGroups[i], i);
//            onOff[i]->setMinimumSize(SS_ONOFF_WIDTH, SS_ONOFF_HEIGHT);
            onOff[i]->setToolTip("Channel " + QString::number(i + 1) + " on/off");
            inchnlLayout->addWidget(onOff[i]);
            connect(onOff[i], SIGNAL(channelState(int, bool)), SLOT(channelOnOff(int, bool)));

            ///volumeSliders[i] = new QInvertedChannelSlider(Qt::Vertical, i, channelButtonGroups[i]);
            // By Tim. p4.0.27 Inverted was not correct type. Maybe was work in progress, rest of code was not converted yet? 
            volumeSliders[i] = new QChannelSlider(Qt::Vertical, i, channelButtonGroups[i]);
            
            volumeSliders[i]->setMinimum(SS_VOLUME_MIN_VALUE);
            volumeSliders[i]->setMaximum(SS_VOLUME_MAX_VALUE);
            
            ///volumeSliders[i]->setValue(SS_VOLUME_MAX_VALUE - SS_VOLUME_DEFAULT_VALUE);
            volumeSliders[i]->setValue(SS_VOLUME_DEFAULT_VALUE);  // p4.0.27
            
//            volumeSliders[i]->setMinimumSize(SS_VOLSLDR_WIDTH, SS_VOLSLDR_LENGTH);
            volumeSliders[i]->setToolTip("Volume, channel " + QString::number(i + 1));
//            setMinimumSize(SS_VOLSLDR_WIDTH, SS_VOLSLDR_LENGTH);
            inchnlLayout->addWidget(volumeSliders[i]);
            connect(volumeSliders[i], SIGNAL(valueChanged(int, int)), SLOT(volumeChanged(int, int)));

            nOffLabel[i] = new QLabel(channelButtonGroups[i]);
//            nOffLabel[i]->setMinimumSize(SS_NONOFF_LABEL_WIDTH, SS_NONOFF_LABEL_HEIGHT);
            nOffLabel[i]->setText("nOff");
            inchnlLayout->addWidget(nOffLabel[i]);

            nOffIgnore[i] = new QChannelCheckbox(channelButtonGroups[i], i);
//            nOffIgnore[i]->setMinimumSize(SS_NONOFF_WIDTH, SS_NONOFF_HEIGHT);
            nOffIgnore[i]->setToolTip("Note off ignore, channel " + QString::number(i + 1));
            inchnlLayout->addWidget(nOffIgnore[i]);
            connect(nOffIgnore[i], SIGNAL(channelState(int, bool)),SLOT(channelNoteOffIgnore(int, bool)));

            panSliders[i] = new QChannelSlider(Qt::Horizontal, i, channelButtonGroups[i]);
            panSliders[i]->setRange(0, 127);
            panSliders[i]->setValue(SS_PANSLDR_DEFAULT_VALUE);
//            panSliders[i]->setMinimumSize(SS_PANSLDR_WIDTH, SS_PANSLDR_LENGTH);
            panSliders[i]->setToolTip("Pan, channel " + QString::number(i + 1));
            inchnlLayout->addWidget(panSliders[i]);
            connect(panSliders[i], SIGNAL(valueChanged(int, int)), SLOT(panChanged(int, int)));

            QGridLayout* dialGrid = new QGridLayout;
            inchnlLayout->addLayout(dialGrid);
            sendFxDial[i][0] = new QChannelDial(channelButtonGroups[i], i, 0);
            sendFxDial[i][0]->setRange(0, 127);
            sendFxDial[i][0]->setMaximumSize(SS_SENDFX_WIDTH, SS_SENDFX_HEIGHT);
            sendFxDial[i][0]->setToolTip("Fx 1 send amount");
            //inchnlLayout->addWidget(sendFxDial[i][0]);
            dialGrid->addWidget(sendFxDial[i][0], 0, 0, Qt::AlignCenter | Qt::AlignTop);

            connect(sendFxDial[i][0], SIGNAL(valueChanged(int, int, int)), SLOT(sendFxChanged(int, int, int)));

            sendFxDial[i][1] = new QChannelDial(channelButtonGroups[i], i, 1);
            sendFxDial[i][1]->setRange(0, 127);
            //inchnlLayout->add(sendFxDial[i][1]);
            dialGrid->addWidget(sendFxDial[i][1], 0, 1, Qt::AlignCenter | Qt::AlignTop);
            sendFxDial[i][1]->setMaximumSize(SS_SENDFX_WIDTH, SS_SENDFX_HEIGHT);
            sendFxDial[i][1]->setToolTip("Fx 2 send amount");

            connect(sendFxDial[i][1], SIGNAL(valueChanged(int, int, int)), SLOT(sendFxChanged(int, int, int)));

            sendFxDial[i][2] = new QChannelDial(channelButtonGroups[i], i, 2);
            sendFxDial[i][2]->setRange(0, 127);
            sendFxDial[i][2]->setMaximumSize(SS_SENDFX_WIDTH, SS_SENDFX_HEIGHT);
            //inchnlLayout->add(sendFxDial[i][2]);
            dialGrid->addWidget(sendFxDial[i][2], 1, 0, Qt::AlignCenter | Qt::AlignTop);
            sendFxDial[i][2]->setToolTip("Fx 3 send amount");
            connect(sendFxDial[i][2], SIGNAL(valueChanged(int, int, int)), SLOT(sendFxChanged(int, int, int)));

            sendFxDial[i][3] = new QChannelDial(channelButtonGroups[i], i, 3);
            sendFxDial[i][3]->setRange(0, 127);
            sendFxDial[i][3]->setMaximumSize(SS_SENDFX_WIDTH, SS_SENDFX_HEIGHT);
            sendFxDial[i][3]->setToolTip("Fx 4 send amount");

            dialGrid->addWidget(sendFxDial[i][3], 1, 1, Qt::AlignCenter | Qt::AlignTop);
            connect(sendFxDial[i][3], SIGNAL(valueChanged(int, int, int)), SLOT(sendFxChanged(int, int, int)));
            inchnlLayout->activate();
            //channelLayout->activate();
            }

      //Master buttongroup:
      masterButtonGroup = new QGroupBox(this);
      channelLayout->addWidget(masterButtonGroup);
      QVBoxLayout* mbgLayout = new QVBoxLayout(masterButtonGroup);
      mbgLayout->setAlignment(Qt::AlignCenter);
//      masterButtonGroup->setMinimumSize(SS_BTNGRP_WIDTH, SS_BTNGRP_HEIGHT);
      
      ///masterSlider = new QInvertedSlider(Qt::Vertical, masterButtonGroup);
      // By Tim. p4.0.27 Inverted was not correct type. Maybe was work in progress, rest of code was not converted yet? 
      masterSlider = new QSlider(Qt::Vertical, masterButtonGroup);
      
      masterSlider->setToolTip("Master volume");
      mbgLayout->addWidget(masterSlider);
      masterSlider->setRange(0, 127);
      
      ///masterSlider->setValue(SS_VOLUME_MAX_VALUE - (int)(SS_MASTERVOL_DEFAULT_VALUE*SS_VOLUME_MAX_VALUE));
      masterSlider->setValue((int)(SS_MASTERVOL_DEFAULT_VALUE*SS_VOLUME_MAX_VALUE)); // p4.0.27
      
//      masterSlider->setMinimumSize(SS_MASTERSLDR_WIDTH, SS_MASTERSLDR_HEIGHT);

      ///connect(masterSlider, SIGNAL(invertedValueChanged(int)), SLOT(masterVolChanged(int)));
      connect(masterSlider, SIGNAL(valueChanged(int)), SLOT(masterVolChanged(int)));  // p4.0.27

      //Main groupbox
      mainGroupBox = new QGroupBox(this);
      mainLayout->addWidget(mainGroupBox);

      QGridLayout* mgbLayout = new QGridLayout(mainGroupBox); // , 8, 3, 1);

      int i=0;

      for (int c=0; c<2; c++) {
            for (int r=0; r<SS_NR_OF_CHANNELS/2; r++) {
                  QHBoxLayout* strip = new QHBoxLayout;//(mgbLayout, 5);
                  mgbLayout->addLayout(strip, r, c);

                  QLabel* channelLabel = new QLabel(QString("Ch ") + QString::number(i + 1), mainGroupBox);
                  strip->addWidget(channelLabel);

                  sampleNameLineEdit[i] = new QLineEdit(mainGroupBox);
                  sampleNameLineEdit[i]->setReadOnly(true);
                  strip->addWidget(sampleNameLineEdit[i]);

                  loadSampleButton[i] = new QChannelButton(mainGroupBox, "L", i);
//                  loadSampleButton[i]->setMinimumSize(SS_SAMPLE_LOAD_WIDTH, SS_SAMPLE_LOAD_HEIGHT);
                  loadSampleButton[i]->setToolTip("Load sample on channel " + QString::number(i + 1));
                  strip->addWidget(loadSampleButton[i]);
                  connect(loadSampleButton[i], SIGNAL(channelState(int, bool)), SLOT(loadSampleDialogue(int)));

                  clearSampleButton[i] = new QChannelButton(mainGroupBox, "C", i);
//                  clearSampleButton[i]->setMinimumSize(SS_SAMPLE_CLEAR_WIDTH, SS_SAMPLE_CLEAR_HEIGHT);
                  clearSampleButton[i]->setToolTip("Clear sample on channel " + QString::number(i + 1));
                  strip->addWidget(clearSampleButton[i]);
                  connect(clearSampleButton[i], SIGNAL(channelState(int, bool)), SLOT(clearSample(int)));

                  i++;
                  }
            }

      // Right bottom panel:
      QGroupBox* rbPanel= new QGroupBox(mainGroupBox);
      mgbLayout->addWidget(rbPanel, 1, 3, 7, 1, Qt::AlignCenter);
      QGridLayout* rbLayout = new QGridLayout(rbPanel); // 6, 1, 8, 5);

      openPluginsButton = new QPushButton("&Send Effects");
      openPluginsButton->setToolTip("Configure LADSPA send effects");
      connect(openPluginsButton, SIGNAL(clicked()), SLOT(openPluginButtonClicked()));
      rbLayout->addWidget(openPluginsButton, 2, 1, Qt::AlignCenter | Qt::AlignVCenter);
      aboutButton = new QPushButton("About SimpleDrums");
      connect(aboutButton, SIGNAL(clicked()), SLOT(aboutButtonClicked()));
//TD      rbLayout->addRowSpacing(3, 20);
      rbLayout->addWidget(aboutButton, 4, 1, Qt::AlignLeft | Qt::AlignVCenter);


      loadButton = new QPushButton(tr("&Load setup"), rbPanel);
      connect(loadButton, SIGNAL(clicked()), SLOT(loadSetup()));
      saveButton = new QPushButton(tr("&Save setup"), rbPanel);
      connect(saveButton, SIGNAL(clicked()), SLOT(saveSetup()));
      //rbLayout->addWidget(openPluginsButton, 1, 1, Qt::AlignCenter | Qt::AlignVCenter);
//      rbLayout->addRowSpacing(2, 20);
      rbLayout->addWidget(loadButton,  3, 1, Qt::AlignCenter | Qt::AlignVCenter);
      rbLayout->addWidget(saveButton,  4, 1, Qt::AlignCenter | Qt::AlignVCenter);
//      rbLayout->addRowSpacing(5, 20);
      rbLayout->addWidget(aboutButton, 6, 1, Qt::AlignCenter | Qt::AlignVCenter);

      lastDir = "";
      //Connect socketnotifier to fifo
      QSocketNotifier* s = new QSocketNotifier(readFd, QSocketNotifier::Read);
      connect(s, SIGNAL(activated(int)), SLOT(readMessage(int)));
      SS_TRACE_OUT
      }

/*!
    \fn SimpleSynthGui::~SimpleSynthGui()
 */
SimpleSynthGui::~SimpleSynthGui()
      {
      SS_TRACE_IN
      simplesynthgui_ptr = 0;
      delete pluginGui;
      SS_TRACE_OUT
      }

/*!
    \fn SimpleSynthGui::readMessage(int)
 */
void SimpleSynthGui::readMessage(int)
      {
      MessGui::readMessage();
      }

/*!
    \fn SimpleSynthGui::processEvent(const MusECore::MidiPlayEvent& ev)
 */
void SimpleSynthGui::processEvent(const MusECore::MidiPlayEvent& ev)
      {
      SS_TRACE_IN
      if (SS_DEBUG_MIDI) {
            printf("GUI received midi event\n");
            }
      if (ev.type() == MusECore::ME_CONTROLLER) {
            int id  = ev.dataA();
            int val = ev.dataB();

            // Channel controllers:
            if (id >= SS_FIRST_CHANNEL_CONTROLLER && id <= SS_LAST_CHANNEL_CONTROLLER ) {
                  // Find out which channel we're dealing with:
                  id-= SS_FIRST_CHANNEL_CONTROLLER;
                  int ch = (id / SS_NR_OF_CHANNEL_CONTROLLERS);
                  id = (id % SS_NR_OF_CHANNEL_CONTROLLERS);

                  int fxid = -1;

                  if (SS_DEBUG_MIDI) {
                        printf("GUI received midi controller - id: %d val %d channel %d\n", id, val, ch);
                        }

                  switch(id) {
                        case SS_CHANNEL_CTRL_VOLUME:
                              volumeSliders[ch]->blockSignals(true);
                              
                              ///volumeSliders[ch]->setValue(SS_VOLUME_MAX_VALUE - val);
                              volumeSliders[ch]->setValue(val);   // p4.0.27
                              
                              volumeSliders[ch]->blockSignals(false);
                              break;

                        case SS_CHANNEL_CTRL_PAN:
                              panSliders[ch]->blockSignals(true);
                              panSliders[ch]->setValue(val);
                              panSliders[ch]->blockSignals(false);
                              break;

                        case SS_CHANNEL_CTRL_NOFF:
                              nOffIgnore[ch]->blockSignals(true);
                              nOffIgnore[ch]->setChecked(val);
                              nOffIgnore[ch]->blockSignals(false);
                              break;

                        case SS_CHANNEL_CTRL_ONOFF:
                              onOff[ch]->blockSignals(true);
                              onOff[ch]->setChecked(val);
                              onOff[ch]->blockSignals(false);
                              break;

                        case SS_CHANNEL_SENDFX1:
                        case SS_CHANNEL_SENDFX2:
                        case SS_CHANNEL_SENDFX3:
                        case SS_CHANNEL_SENDFX4:
                              fxid = id - SS_CHANNEL_SENDFX1;
                              if (SS_DEBUG_MIDI) {
                                    printf("SimpleSynthGui::processEvent - Channel sendfx, fxid: %d, val: %d\n", fxid, val);
                                    }
                              sendFxDial[ch][fxid]->blockSignals(true);
                              sendFxDial[ch][fxid]->setValue(val);
                              sendFxDial[ch][fxid]->blockSignals(false);
                              break;

                        default:
                              if (SS_DEBUG_MIDI)
                                    printf("SimpleSynthGui::processEvent - unknown controller received: %d\n", id);
                        }
                  }
            // Master controllers:
            else if (id >= SS_FIRST_MASTER_CONTROLLER && id <= SS_LAST_MASTER_CONTROLLER) {
                  if (id == SS_MASTER_CTRL_VOLUME) {
                        masterSlider->blockSignals(true);
                        
                        ///masterSlider->setValue(SS_MASTERVOL_MAX_VALUE - val);
                        masterSlider->setValue(val);   // p4.0.27
                        
                        masterSlider->blockSignals(false);
                        }
                  }
            else if (id>= SS_FIRST_PLUGIN_CONTROLLER && id <= SS_LAST_PLUGIN_CONTROLLER) {
                  int fxid = (id - SS_FIRST_PLUGIN_CONTROLLER) / SS_NR_OF_PLUGIN_CONTROLLERS;
                  int cmd = (id - SS_FIRST_PLUGIN_CONTROLLER) % SS_NR_OF_PLUGIN_CONTROLLERS;

                  // Plugin return-gain:
                  if (cmd == SS_PLUGIN_RETURN) {
                        if (SS_DEBUG_MIDI)
                              printf("SimpleSynthGui::processEvent - fx retgain received: fxid: %d val: %d\n", fxid, val);

                        SS_PluginFront* pf = pluginGui->getPluginFront((unsigned)fxid);
                        pf->setRetGain(val);
                        }
                  // Plugin on/off:
                  else if (cmd == SS_PLUGIN_ONOFF) {      // p4.0.27
                        if (SS_DEBUG_MIDI)
                              printf("SimpleSynthGui::processEvent - fx onoff received: fxid: %d val: %d\n", fxid, val);
                        SS_PluginFront* pf = pluginGui->getPluginFront((unsigned)fxid);
                        pf->setOnOff(val);
                        }
                  }
            }
            //
            // Sysexes:
            //
            else if (ev.type() == MusECore::ME_SYSEX) {
                  byte* data = ev.data();
                  //byte* data = d + 2;
                  int cmd = *data;
                  switch (cmd) {
                        case SS_SYSEX_LOAD_SAMPLE_OK: {
                              int ch = *(data+1);
                              QString filename = (const char*) (data+2);
                              sampleNameLineEdit[ch]->setText(filename.section('/',-1,-1));
                              if (SS_DEBUG_MIDI) {
                                    printf("SimpleSynthGui - sample %s loaded OK on channel: %d\n", filename.toLatin1().constData(), ch);
                                    }
                              if (!onOff[ch]->isChecked()) {
                                    onOff[ch]->blockSignals(true);
                                    onOff[ch]->setChecked(true);
                                    onOff[ch]->blockSignals(false);
                                    channelOnOff(ch, true);
                                    }
                              break;
                              }

                        case SS_SYSEX_LOAD_SAMPLE_ERROR: {
                              //int ch = *(data+1);
                              const char* filename = (const char*) (data+2);
                              /*QMessageBox* yn = new QMessageBox("Sample not found", "Failed to load sample: " + QString(filename) + "\n" +
                                                      "Do you want to open file browser and try to locate it elsewhere?",
                                                      QMessageBox::Warning,
                                                      QMessageBox::Yes,
                                                      QMessageBox::No,
                                                      QMessageBox::NoButton,
                                                      this);*/
                              /*int res = QMessageBox::warning(this,
                                                      "SimpleDrums","Failed to load sample: " + QString(filename) + "\n" +
                                                      "Do you want to open file browser and try to locate it elsewhere?",
                                                      "&Yes", "&No");
                                                      */
                              //int res = yn->exec();
                              printf("Error: Sample %s not found! TODO: Fix this\n", filename);
                              //if (res == 0) {
                              //      loadSampleDialogue(ch);
                              //      }
                              break;
                              }

                        case SS_SYSEX_LOAD_SENDEFFECT_OK: {
                              if (SS_DEBUG_MIDI) {
                                    printf("SimpleSynthGui - sysex load sendeffect OK on fxid: %d\n", *(data+1));
                                    }
                              int fxid = *(data+1);
                              SS_PluginFront* pf = pluginGui->getPluginFront((unsigned)fxid);
                              ///pf->updatePluginValue(*(data+2));
                              pf->updatePluginValue(  *((unsigned*)(data+2)) );     // p4.0.27
                              break;
                              }

                        case SS_SYSEX_CLEAR_SENDEFFECT_OK: {
                              if (SS_DEBUG_MIDI) {
                                    printf("SimpleSynthGui - sysex clear sendeffect OK on fxid: %d\n", *(data+1));
                                    }
                              SS_PluginFront* pf = pluginGui->getPluginFront((unsigned)*(data+1));
                              pf->clearPluginDisplay();
                              break;
                              }

                        case SS_SYSEX_CLEAR_SAMPLE_OK: {
                              if (SS_DEBUG_MIDI) {
                                    printf("SimpleSynthGui - sysex clear samle OK on channel: %d\n", *(data+1));
                                    }
                              byte ch = *(data+1);
                              sampleNameLineEdit[ch]->setText("");
                              break;
                              }

                        case SS_SYSEX_SET_PLUGIN_PARAMETER_OK: {
                              if (SS_DEBUG_MIDI) {
                                    printf("SimpleSynthGui - plugin parameter OK on fxid: %d\n", *(data+1));
                                    }
                              SS_PluginFront* pf = pluginGui->getPluginFront((unsigned)*(data+1));
                              int param = *(data+2);
                              int val   = *(data+3);
                              pf->blockSignals(true);
                              pf->setParameterValue(param, val);
                              pf->blockSignals(false);
                              break;
                              }

                        case SS_SYSEX_SEND_INIT_DATA: {
// FN: TODO
#if 1
                              const unsigned initdata_len = ev.len() - 1;
                              byte* init_data = (data + 1);
                              QFileInfo fileInfo = QFileInfo(lastSavedProject);

                              lastProjectDir = fileInfo.path();
                              if (fileInfo.suffix() != "sds" && fileInfo.suffix() != "SDS") {
                                    lastSavedProject += ".sds";
                                    fileInfo = QFileInfo(lastSavedProject);
                                    }
                              QFile theFile(fileInfo.filePath());

                              // Write data
                              if (theFile.open(QIODevice::WriteOnly)) {
                                    theFile.write((const char*)&initdata_len, sizeof(initdata_len)); // First write length
                                    if (theFile.write((const char*)init_data, initdata_len) == -1) {
                                          // Fatal error writing
                                          QMessageBox* msgBox = new QMessageBox(QMessageBox::Warning, "SimpleDrums error Dialog", "Fatal error when writing to file. Setup not saved.",
                                                QMessageBox::Ok, this);
                                          msgBox->exec();
                                          delete msgBox;
                                          }
                                    theFile.close();
                                    }
                              else {
                                    // An error occured when opening
                                    QMessageBox* msgBox = new QMessageBox(QMessageBox::Warning, "SimpleDrums error Dialog", "Error opening file. Setup was not saved.",
                                          QMessageBox::Ok, this);
                                    msgBox->exec();
                                    delete msgBox;
                                    }
#endif

                              break;
                              }


                        default:
                              if (SS_DEBUG_MIDI) {
                                    printf("SimpleSynthGui::processEvent - unknown sysex cmd received: %d\n", cmd);
                                    }
                              break;
                        }
                  }
      SS_TRACE_OUT
      }


/*!
    \fn SimpleSynthGui::volumeChanged(int val)
 */
void SimpleSynthGui::volumeChanged(int channel, int val)
      {
      setChannelVolume(channel, val);
      }

/*!
    \fn SimpleSynthGui::panChanged(int channel, int value)
 */
void SimpleSynthGui::panChanged(int channel, int value)
      {
      sendController(0, SS_CHANNEL_PAN_CONTROLLER(channel), value);
      }

/*!
    \fn SimpleSynthGui::channelOnOff(int channel, bool state)
 */
void SimpleSynthGui::channelOnOff(int channel, bool state)
      {
      sendController(0, SS_CHANNEL_ONOFF_CONTROLLER(channel), state);
      }

/*!
    \fn SimpleSynthGui::channelNoteOffIgnore(bool state)
 */
void SimpleSynthGui::channelNoteOffIgnore(int channel, bool state)
      {
      sendController(0, SS_CHANNEL_NOFF_CONTROLLER(channel), (int) state);
      }

/*!
    \fn SimpleSynthGui::sendFxChanged(int ch, int fxid, int val)
 */
void SimpleSynthGui::sendFxChanged(int ch, int fxid, int val)
      {
      sendController(0, SS_CHANNEL_SENDFX_CONTROLLER(ch, fxid), (int) val);
      }

/*!
    \fn SimpleSynthGui::masterVolChanged(int val)
 */
void SimpleSynthGui::masterVolChanged(int val)
      {
      sendController(0, SS_MASTER_CTRL_VOLUME, val);
      }

/*!
    \fn SimpleSynthGui::setChannelVolume(int channel, byte volume)
 */
void SimpleSynthGui::setChannelVolume(int channel, int volume)
      {
      //volumeSliders[channel]->setValue(SS_VOLUME_MAX_VALUE - volume);
      sendController(0, SS_CHANNEL_VOLUME_CONTROLLER(channel), (int)volume);
      }


/*!
    \fn SimpleSynthGui::loadSampleDialogue(int channel)
 */
void SimpleSynthGui::loadSampleDialogue(int channel)
      {
      QString filename =
            QFileDialog::getOpenFileName(
      					   this,
                                       tr("Load sample dialog"),
      					   lastDir,
                                       QString("*.wav *.WAV"));

      if (filename != QString::null) {
            QFileInfo fi(filename);
            lastDir = fi.path();

            if (SS_DEBUG)
                  printf("lastDir = %s\n", lastDir.toLatin1().constData());

            //int l = filename.length() + 4;
            int l = filename.length() + 6;
            byte d[l];

            //d[0] = SS_SYSEX_LOAD_SAMPLE;
            //d[1] = (byte) channel;
            //d[2] = (byte) filename.length();
            d[0] = MUSE_SYNTH_SYSEX_MFG_ID;
            d[1] = SIMPLEDRUMS_UNIQUE_ID;
            d[2] = SS_SYSEX_LOAD_SAMPLE;
            d[3] = (byte) channel;
            d[4] = (byte) filename.length();
            //memcpy(d+3, filename.toLatin1().constData(), filename.length()+1);
            memcpy(d+5, filename.toLatin1().constData(), filename.length()+1);
            sendSysex(d, l);
            }
      }



/*!
    \fn SimpleSynthGui::clearSample(int ch)
 */
void SimpleSynthGui::clearSample(int ch)
      {
      if (sampleNameLineEdit[ch]->text().length() > 0) { //OK, we've got a live one here
            //byte d[2];
            byte d[4];
            //d[0] = SS_SYSEX_CLEAR_SAMPLE;
            //d[1] = (byte) ch;
            d[0] = MUSE_SYNTH_SYSEX_MFG_ID;
            d[1] = SIMPLEDRUMS_UNIQUE_ID;
            d[2] = SS_SYSEX_CLEAR_SAMPLE;
            d[3] = (byte) ch;
            //sendSysex(d, 2);
            sendSysex(d, 4);
            sampleNameLineEdit[ch]->setText("");
            }
      }

/*!
    \fn SimpleSynthGui::displayPluginGui()
 */
void SimpleSynthGui::displayPluginGui()
      {
      pluginGui->show();
      }

/*!
    \fn SimpleSynthGui::loadEffectInvoked(int fxid, QString lib, QString label)
 */
void SimpleSynthGui::loadEffectInvoked(int fxid, QString lib, QString label)
      {
      //int l = 4 + lib.length() + label.length();
      int l = 6 + lib.length() + label.length();
      byte d[l];
      //d[0] = SS_SYSEX_LOAD_SENDEFFECT;
      //d[1] = (byte) fxid;
      d[0] = MUSE_SYNTH_SYSEX_MFG_ID;
      d[1] = SIMPLEDRUMS_UNIQUE_ID;
      d[2] = SS_SYSEX_LOAD_SENDEFFECT;
      d[3] = (byte) fxid;
      //memcpy (d+2, lib.toLatin1().constData(), lib.length()+1);
      //memcpy (d+3+lib.length(), label.toLatin1().constData(), label.length()+1);
      memcpy (d+4, lib.toLatin1().constData(), lib.length()+1);
      memcpy (d+5+lib.length(), label.toLatin1().constData(), label.length()+1);
      sendSysex(d, l);
      }


/*!
    \fn SimpleSynthGui::returnLevelChanged(int fxid, int val)
 */
void SimpleSynthGui::returnLevelChanged(int fxid, int val)
      {
      sendController(0, SS_PLUGIN_RETURNLEVEL_CONTROLLER(fxid), val);
      }


/*!
    \fn SimpleSynthGui::toggleEffectOnOff(int fxid, int state)
 */
void SimpleSynthGui::toggleEffectOnOff(int fxid, int state)
      {
      sendController(0, SS_PLUGIN_ONOFF_CONTROLLER(fxid), state);
      }


/*!
    \fn SimpleSynthGui::clearPlugin(int fxid)
 */
void SimpleSynthGui::clearPlugin(int fxid)
      {
      //byte d[2];
      byte d[4];
      //d[0] = SS_SYSEX_CLEAR_SENDEFFECT;
      //d[1] = fxid;
      d[0] = MUSE_SYNTH_SYSEX_MFG_ID;
      d[1] = SIMPLEDRUMS_UNIQUE_ID;
      d[2] = SS_SYSEX_CLEAR_SENDEFFECT;
      d[3] = fxid;
      //sendSysex(d, 2);
      sendSysex(d, 4);
      }


/*!
    \fn SimpleSynthGui::effectParameterChanged(int fxid, int parameter, int val)
 */
void SimpleSynthGui::effectParameterChanged(int fxid, int parameter, int val)
      {
      //int len = 4;
      int len = 6;
      byte d[len];
      //d[0] = SS_SYSEX_SET_PLUGIN_PARAMETER;
      //d[1] = (byte) fxid;
      //d[2] = (byte) parameter;
      //d[3] = (byte) val;
      d[0] = MUSE_SYNTH_SYSEX_MFG_ID;
      d[1] = SIMPLEDRUMS_UNIQUE_ID;
      d[2] = SS_SYSEX_SET_PLUGIN_PARAMETER;
      d[3] = (byte) fxid;
      d[4] = (byte) parameter;
      d[5] = (byte) val;
      sendSysex(d, len);
      }


/*!
    \fn SimpleSynthGui::openPluginButtonClicked()
 */
void SimpleSynthGui::openPluginButtonClicked()
      {
      if (pluginGui->isVisible())
            pluginGui->raise();
      else
            displayPluginGui();
      }


/*!
    \fn SimpleSynthGui::aboutButtonClicked()
 */
void SimpleSynthGui::aboutButtonClicked()
      {
      QString caption = "SimpleDrums ver";
      caption+= SS_VERSIONSTRING;
      ///QString text = caption + "\n\n(C) Copyright 2000-2004 Mathias Lundgren (lunar_shuttle@users.sf.net), Werner Schweer\nPublished under the GNU Public License";
      QString text = caption + "\n\n(C) Copyright 2000-2004 Mathias Lundgren (lunar_shuttle@users.sf.net), Werner Schweer\n"
                               "Fixes/mods: (C) Copyright 2011 Tim E. Real (terminator356@users.sf.net)\nPublished under the GNU Public License";
      QMessageBox* msgBox = new QMessageBox(caption, text, QMessageBox::NoIcon,
            QMessageBox::Ok, Qt::NoButton, Qt::NoButton, this);
      msgBox->exec();
      }


/*!
    \fn SimpleSynthGui::loadSetup()
    \brief Load setup from file
 */
void SimpleSynthGui::loadSetup()
      {
      bool success = true;
      QString filename =
            QFileDialog::getOpenFileName(this, "Load setup dialog", lastProjectDir,
                                       QString("*.sds *.SDS"));

      if (filename != QString::null) {
            QFile theFile(filename);
            if (theFile.open(QIODevice::ReadOnly)) {
                  unsigned initdata_len = 0;
                  if (theFile.read((char*)&initdata_len, sizeof(initdata_len)) == -1)
                     success = false;

                  ///byte* init_data = new byte[initdata_len];
                  byte* init_data = new byte[initdata_len + 2];   // 2 for MFG ID and synth ID. 
                  init_data[0] = MUSE_SYNTH_SYSEX_MFG_ID;
                  init_data[1] = SIMPLEDRUMS_UNIQUE_ID;
                  //if (theFile.read((char*)(init_data), initdata_len) == -1)
                  if (theFile.read((char*)(init_data + 2), initdata_len) == -1)
                     success = false;

                  if (!success) {
                        QMessageBox* msgBox = new QMessageBox(QMessageBox::Warning, "SimpleDrums Error Dialog", "Error opening/reading from file. Setup not loaded.",
                              QMessageBox::Ok, this);
                        msgBox->exec();
                        delete msgBox;
                        }
                  else {
                        ///sendSysex(init_data, initdata_len);
                        sendSysex(init_data, initdata_len + 2);
                        }

                  delete[] init_data;
                  }
            }
      }


/*!
    \fn SimpleSynthGui::saveSetup()
    \brief Save setup to file
 */
void SimpleSynthGui::saveSetup()
      {
      QString filename =
            QFileDialog::getSaveFileName(this, "Save setup dialog", lastProjectDir,
                                       QString("*.sds *.SDS"));

      if (filename != QString::null) {
            lastSavedProject = filename;
            //byte d[1];
            byte d[3];
            //d[0] = SS_SYSEX_GET_INIT_DATA;
            d[0] = MUSE_SYNTH_SYSEX_MFG_ID;
            d[1] = SIMPLEDRUMS_UNIQUE_ID;
            d[2] = SS_SYSEX_GET_INIT_DATA;
            //sendSysex(d, 1); // Makes synth send gui initdata, where rest of the saving takes place
            sendSysex(d, 3); // Makes synth send gui initdata, where rest of the saving takes place
            }
      }

