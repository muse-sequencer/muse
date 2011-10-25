//
// C++ Interface: ssplugingui
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
// Copyright: See COPYING file that comes with this distribution
//
//

#ifndef __SS_PLUGINGUI_H__
#define __SS_PLUGINGUI_H__
#include <QDialog>
#include <QSlider>
#include <QButtonGroup>
#include <QtGui>
//#include <QHBoxLayout>
//#include <QVBoxLayout>

#include "ui_sspluginchooserbase.h"
#include "common.h"
#include "ssplugin.h"

class SS_ParameterWidget
      {
   protected:
      int fxid;
      int parameter;

      LadspaPlugin* plugin;

   public:
      SS_ParameterWidget() { }
      virtual ~SS_ParameterWidget() { }
      int getFxId() { SS_TRACE_IN SS_TRACE_OUT return fxid; }
      bool isBool() { SS_TRACE_IN SS_TRACE_OUT return plugin->isBool(parameter); }
      bool isLog()  { SS_TRACE_IN SS_TRACE_OUT return plugin->isLog(parameter); }
      bool isInt()  { SS_TRACE_IN SS_TRACE_OUT return plugin->isInt(parameter); }
      virtual void setParamValue(int) { printf("Virtual function - should not be called!"); };
      };

class SS_ParameterCheckBox : public QCheckBox, public SS_ParameterWidget
   {
   Q_OBJECT

   public:
      SS_ParameterCheckBox(QWidget* parent, LadspaPlugin* in_plugin, int in_id, int in_parameter)
         : QCheckBox(parent) , SS_ParameterWidget()
         {
         SS_TRACE_IN
         plugin = in_plugin;
         fxid = in_id;
         parameter = in_parameter;
         connect(this, SIGNAL(clicked()), SLOT(isClicked()));
         SS_TRACE_OUT
         }

      virtual void setParamValue(int val) { SS_TRACE_IN setChecked(val); SS_TRACE_OUT}

   private slots:
      void isClicked() { SS_TRACE_IN emit valueChanged(fxid, parameter, (int)this->isChecked()); SS_TRACE_OUT}

   signals:
      void valueChanged(int id, int param, int val);
   };

class SS_ParameterSlider : public QSlider, public SS_ParameterWidget
   {
   Q_OBJECT

   public:
      SS_ParameterSlider(QWidget* parent, LadspaPlugin* in_plugin, int in_id, int in_parameter)
         : QSlider(Qt::Horizontal, parent), SS_ParameterWidget()
         {
         SS_TRACE_IN
         plugin = in_plugin;
         fxid = in_id;
         parameter = in_parameter;
         SS_TRACE_OUT
         }

      virtual void setParamValue(int val) { SS_TRACE_IN setValue(val); SS_TRACE_OUT}

   ///public slots:
   ///   virtual void setValue(int val) { SS_TRACE_IN QSlider::setValue(val); emit valueChanged(fxid, parameter, val); SS_TRACE_OUT }

   signals:
      void valueChanged(int id, int param, int val);
   
   protected:
      virtual void sliderChange(SliderChange change)   // p4.0.27 Tim
      { 
        SS_TRACE_IN 
        QSlider::sliderChange(change);
        if(change == QAbstractSlider::SliderValueChange)
          emit valueChanged(fxid, parameter, value());  
        SS_TRACE_OUT
      }    
   };

typedef std::list<SS_ParameterWidget*>           SS_ParameterWidgetList;
typedef std::list<SS_ParameterWidget*>::iterator SS_iParameterWidgetList ;

//-------------------------------
// SS_PluginChooser
//-------------------------------
class SS_PluginChooser : public QDialog, Ui::SS_PluginChooserBase
{
   Q_OBJECT
   private:
         LadspaPlugin* selectedPlugin;
   protected:

   public:
         SS_PluginChooser(QWidget* parent);
         LadspaPlugin* getSelectedPlugin() { SS_TRACE_IN SS_TRACE_OUT return selectedPlugin; }

   private slots:
      void okPressed();
      void cancelPressed();
      ///void selectionChanged(QTreeWidgetItem* item);
      void selectionChanged();
      void doubleClicked(QTreeWidgetItem* item);

   private:
      QTreeWidgetItem* selectedItem;
      LadspaPlugin* findSelectedPlugin();

};

//-------------------------------
// SS_PluginGuiFront
//-------------------------------
class SS_PluginFront : public QGroupBox
   {
   Q_OBJECT
   private:
      QHBoxLayout*      layout;
      QVBoxLayout*      expLayout;
      QLineEdit*        pluginName;
      QCheckBox*        onOff;
      QPushButton*      loadFxButton;
      QPushButton*      clearFxButton;
      QPushButton*      expandButton;
      QSlider*          outGainSlider;
      SS_PluginChooser* pluginChooser;
      LadspaPlugin*     plugin;
      QGroupBox*     expGroup;

      int               fxid;
      bool              expanded;

      //For effect parameters:
      SS_ParameterWidgetList  paramWidgets;

   protected:

   public:
      SS_PluginFront(QWidget* parent, int id);
      void setPluginName(QString name);
      ~SS_PluginFront();
      void updatePluginValue(unsigned i);
      void clearPluginDisplay();
      void setParameterValue(int param, int val);
      void setRetGain(int val);
      void setOnOff(bool val);

   protected:
      virtual QSize sizeHint() const;
      virtual QSizePolicy sizePolicy() const;

   private slots:
      void loadButton();
      void returnSliderMoved(int val);
      void onOffToggled(bool state);
      void clearButtonPressed();
      void expandButtonPressed();
      void parameterValueChanged(int fxid, int parameter, int val);

   signals:
      void loadPlugin(int fxid, QString lib, QString name);
      void returnLevelChanged(int fxid, int val);
      void fxToggled(int fxid, int state);
      void clearPlugin(int fxid);
      void sizeChanged(int fxid, int val);
      void effectParameterChanged(int fxid, int param, int val);

   private:
      void createPluginParameters();
   };


//-------------------------------
// SS_PluginGui
// Main plugin class, dialog
//-------------------------------
class SS_PluginGui : public QDialog
   {
   Q_OBJECT
   private:
      QVBoxLayout* layout;
      SS_PluginFront* pluginFronts[4];

   public:
      SS_PluginGui(QWidget* parent);
      SS_PluginFront* getPluginFront(unsigned i);
      ~SS_PluginGui() {}
private slots:
    void pluginFrontSizeChanged(int fxid, int val);
   };


#endif

