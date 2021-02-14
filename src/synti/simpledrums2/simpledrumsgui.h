//
// C++ Interface: testogui
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
#ifndef __MUSE_TESTOGUI_H__
#define __MUSE_TESTOGUI_H__

#include <QSlider>
#include <QCheckBox>
#include <QPushButton>
#include <QDial>
#include <QLabel>
//#include <QFileInfo>
#include <QGroupBox>
#include <QHeaderView>
#include <QComboBox>
#include <widgets/meter.h>

#include "libsynti/gui.h"
#include "ui_simpledrumsguibase.h"
#include "common.h"

class QButtonGroup;
class QLabel;
class SS_PluginGui;

//--------------------------------------
// QChannelSlider
//--------------------------------------
class QChannelSlider: public QSlider
   {
   Q_OBJECT

   public:
      QChannelSlider(Qt::Orientation, int ch, QWidget* parent = 0);
      int getChannel();
      void setChannel(int ch);

   public slots:
      void updateStatusField();

   signals:
      void valueChanged(int channel, int value);
      void updateInformationField(QString info);

   protected:
      int channel;
      virtual void sliderChange(SliderChange change);   
      virtual void mouseMoveEvent(QMouseEvent *e);
   };


//--------------------------------------
// QChannelOnOff
//--------------------------------------

class QChannelCheckbox : public QCheckBox
   {
   Q_OBJECT
   public:
      QChannelCheckbox(QWidget* parent, int channel);

   private:
      int channel;

    private slots:
      void isClicked();

   signals:
      void channelState(int channel, bool state);
   };

//--------------------------------------
// QChannelButton
//--------------------------------------
class QChannelButton : public QPushButton
   {
   Q_OBJECT

   private:
      int channel;

   public:
      QChannelButton(QWidget* parent, const char* text, int ch);

   private slots:
      void isClicked();

   signals:
      void channelState(int channel, bool state);

   };

//--------------------------------------
// QChannelDial
//--------------------------------------

class QChannelDial : public QDial
   {
   Q_OBJECT

   public:
      QChannelDial(QWidget* parent, int ch, int fxid);

   signals:
      void valueChanged(int channel, int fxid, int val);
      void sliderMoved(int channel, int val);
      void updateInformationField(QString info);

   public slots:
      void updateStatusField();

   protected:
      int channel;
      int sendfxid;
      virtual void sliderChange(SliderChange change);
      virtual void mouseMoveEvent(QMouseEvent *e);
   private slots:
      void forwardSliderMoved();
   };

//--------------------------------------
// SimpleSynthGui - the Gui
//--------------------------------------
class SimpleSynthGui : public QDialog, public Ui::SimpleDrumsGuiBase, public MessGui
   {
   Q_OBJECT
   private:
      // MESS interface:
      virtual void processEvent(const MusECore::MidiPlayEvent& ev);
      void setChannelVolume(int channel, int volume);
      void setChannelPitch(int channel, int volume);
      void setChannelRoute(int channel, int route);
      void displayPluginGui();
      QGroupBox* channelButtonGroups[SS_NR_OF_CHANNELS];
      
      ///QInvertedChannelSlider* volumeSliders[SS_NR_OF_CHANNELS];
      QChannelSlider*         volumeSliders[SS_NR_OF_CHANNELS];       // p4.0.27 Tim. Inverted not correct. Was WIP? 
      QChannelDial*           pitchKnobs[SS_NR_OF_CHANNELS];
      
      QChannelSlider*         panSliders[SS_NR_OF_CHANNELS];
      QChannelCheckbox*       onOff[SS_NR_OF_CHANNELS];
      QChannelCheckbox*       nOffIgnore[SS_NR_OF_CHANNELS];
      QChannelButton*         loadSampleButton[SS_NR_OF_CHANNELS];
      QChannelButton*         clearSampleButton[SS_NR_OF_CHANNELS];
      QLabel*                 nOffLabel[SS_NR_OF_CHANNELS];
      QLineEdit*              sampleNameLineEdit[SS_NR_OF_CHANNELS];
      
      ///QInvertedSlider*        masterSlider;
      QSlider*                masterSlider;                          // p4.0.27 Tim. Inverted not correct. Was WIP? 
      
      QChannelDial*           sendFxDial[SS_NR_OF_CHANNELS][SS_NR_OF_SENDEFFECTS];

      QPushButton*            openPluginsButton;
      QPushButton*            aboutButton;
      QPushButton*            loadButton;
      QPushButton*            saveButton;

      QComboBox*              chnRoutingCb[SS_NR_OF_CHANNELS];
      MusEGui::Meter*         chnMeter[SS_NR_OF_CHANNELS];
      double                  meterVal[SS_NR_OF_CHANNELS];
      double                  peakVal[SS_NR_OF_CHANNELS];


      QString lastDir;
      QString lastSavedProject;
      QString lastProjectDir;
      SS_PluginGui* pluginGui;

      int _sampleRate;
      
   public:
      SimpleSynthGui(int sampleRate);
      virtual ~SimpleSynthGui();

      int sampleRate() const { return _sampleRate; }
      // Returns true if the value was changed.
      bool setSampleRate(int sampleRate) { 
        bool r = _sampleRate != sampleRate; 
        _sampleRate = sampleRate; 
        return r;
      }
      
   public slots:
      void loadEffectInvoked(int fxid, QString lib, QString label);
      void returnLevelChanged(int fxid, int val);
      void toggleEffectOnOff(int fxid, int state);
      void clearPlugin(int fxid);
      void effectParameterChanged(int fxid, int parameter, int val);
      void heartBeat();

   private slots:
      void volumeChanged(int channel, int val);
      void pitchChanged(int channel, int, int val);
      void panChanged(int channel, int value);
      void channelOnOff(int channel, bool state);
      void channelNoteOffIgnore(int channel, bool state);
      void masterVolChanged(int val);
      void loadSampleDialogue(int channel);
      void readMessage();
      void clearSample(int ch);
      void sendFxChanged(int ch, int fxid, int val);
      void openPluginButtonClicked();
      void aboutButtonClicked();
      void loadSetup();
      void saveSetup();
      void routeChanged(int index);

      friend class SimpleSynth;
   };

#endif
