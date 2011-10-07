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

   ///public slots:
   ///   virtual void setValue(int val);

   signals:
      void valueChanged(int channel, int value);
   
   protected:
      int channel;
      virtual void sliderChange(SliderChange change);   
   };

//--------------------------------------
// QInvertedSlider
//--------------------------------------
class QInvertedSlider : public QSlider
   {
   Q_OBJECT
   public:
      QInvertedSlider(Qt::Orientation o, QWidget* parent = 0);
         ///: QSlider(o, parent) {}

   ///public slots:
   ///   virtual void setValue(int val);

   signals:
      void invertedValueChanged(int value);
   
   protected:
      virtual void sliderChange(SliderChange change);    
   };

//--------------------------------------
// QInvertedChannelSlider
//--------------------------------------
class QInvertedChannelSlider : public QChannelSlider
   {
   Q_OBJECT
   public:
      QInvertedChannelSlider(Qt::Orientation o, int channel, QWidget* parent = 0);

   ///public slots:
   ///   virtual void setValue(int val);
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

   ///public slots:
   ///   virtual void setValue(int val);

   protected:
      int channel;
      int sendfxid;
      virtual void sliderChange(SliderChange change);    
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
      void displayPluginGui();
      QGroupBox* channelButtonGroups[SS_NR_OF_CHANNELS];
      QGroupBox*           masterButtonGroup;
      QGroupBox*              mainGroupBox;
      
      ///QInvertedChannelSlider* volumeSliders[SS_NR_OF_CHANNELS];
      QChannelSlider*         volumeSliders[SS_NR_OF_CHANNELS];       // p4.0.27 Tim. Inverted not correct. Was WIP? 
      
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


      QString lastDir;
      QString lastSavedProject;
      QString lastProjectDir;
      SS_PluginGui* pluginGui;

   public:
      SimpleSynthGui();
      virtual ~SimpleSynthGui();

   public slots:
      void loadEffectInvoked(int fxid, QString lib, QString label);
      void returnLevelChanged(int fxid, int val);
      void toggleEffectOnOff(int fxid, int state);
      void clearPlugin(int fxid);
      void effectParameterChanged(int fxid, int parameter, int val);

   private slots:
      void volumeChanged(int channel, int val);
      void panChanged(int channel, int value);
      void channelOnOff(int channel, bool state);
      void channelNoteOffIgnore(int channel, bool state);
      void masterVolChanged(int val);
      void loadSampleDialogue(int channel);
      void readMessage(int);
      void clearSample(int ch);
      void sendFxChanged(int ch, int fxid, int val);
      void openPluginButtonClicked();
      void aboutButtonClicked();
      void loadSetup();
      void saveSetup();

   };

extern SimpleSynthGui* simplesynthgui_ptr;

#endif
