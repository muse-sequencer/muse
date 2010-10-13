//
// C++ Interface: ssplugingui
//
// Description:
//
//
// Author: Mathias Lundgren <lunar_shuttle@users.sf.net>, (C) 2004
//
// Copyright: See COPYING file that comes with this distribution
//
//

#ifndef __SS_PLUGINGUI_H__
#define __SS_PLUGINGUI_H__
#include <qdialog.h>
#include <qslider.h>
#include <Q3ButtonGroup>
#include <QtGui>
//Added by qt3to4:
#include <Q3HBoxLayout>
#include <Q3VBoxLayout>
#include "sspluginchooserbase.h"
#include "common.h"
#include "ssplugin.h"

class SS_ParameterWidget
      {
   protected:
      int fxid;
      int parameter;

      LadspaPlugin* plugin;

   public:
      SS_ParameterWidget() { };
      virtual ~SS_ParameterWidget() { };  

      int getFxId() { SS_TRACE_IN SS_TRACE_OUT return fxid; }
      bool isBool() { SS_TRACE_IN SS_TRACE_OUT return plugin->isBool(parameter); }
      bool isLog()  { SS_TRACE_IN SS_TRACE_OUT return plugin->isLog(parameter); }
      bool isInt()  { SS_TRACE_IN SS_TRACE_OUT return plugin->isInt(parameter); }
      virtual void setParamValue(int /*val*/) {  //prevent compiler  warning unused parameter
               printf("Virtual function - should not be called!"); };
      };

class SS_ParameterCheckBox : public QCheckBox, public SS_ParameterWidget
   {
   Q_OBJECT

   public:
      SS_ParameterCheckBox(QWidget* parent, LadspaPlugin* in_plugin, int in_id, int in_parameter, const char* name = 0)
         : QCheckBox(parent, name) , SS_ParameterWidget()
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
      void isClicked() { SS_TRACE_IN emit valueChanged(fxid, parameter, (int)this->isOn()); SS_TRACE_OUT}

   signals:
      void valueChanged(int id, int param, int val);
   };

class SS_ParameterSlider : public QSlider, public SS_ParameterWidget
   {
   Q_OBJECT

   public:
      SS_ParameterSlider(QWidget* parent, LadspaPlugin* in_plugin, int in_id, int in_parameter, const char* name = 0)
         : QSlider(Qt::Horizontal, parent, name), SS_ParameterWidget()
         {
         SS_TRACE_IN
         plugin = in_plugin;
         fxid = in_id;
         parameter = in_parameter;
         SS_TRACE_OUT
         }

      virtual void setParamValue(int val) { SS_TRACE_IN setValue(val); SS_TRACE_OUT}

   public slots:
      virtual void setValue(int val) { SS_TRACE_IN QSlider::setValue(val); emit valueChanged(fxid, parameter, val); SS_TRACE_OUT }

   signals:
      void valueChanged(int id, int param, int val);
   };

typedef std::list<SS_ParameterWidget*>           SS_ParameterWidgetList;
typedef std::list<SS_ParameterWidget*>::iterator SS_iParameterWidgetList ;

//-------------------------------
// SS_PluginChooser
//-------------------------------
class SS_PluginChooser : public SS_PluginChooserBase
{
   Q_OBJECT
   private:
         LadspaPlugin* selectedPlugin;
   protected:

   public:
         SS_PluginChooser(QWidget* parent, const char* name=0);
         LadspaPlugin* getSelectedPlugin() { SS_TRACE_IN SS_TRACE_OUT return selectedPlugin; }

   private slots:
      void okPressed();
      void cancelPressed();
      void selectionChanged(Q3ListViewItem* item);
      void doubleClicked(Q3ListViewItem* item);

   private:
      Q3ListViewItem* selectedItem;
      LadspaPlugin* findSelectedPlugin();

};

//-------------------------------
// SS_PluginGuiFront
//-------------------------------
class SS_PluginFront : public Q3GroupBox
   {
   Q_OBJECT
   private:
      Q3HBoxLayout*      layout;
      Q3VBoxLayout*      expLayout;
      QLineEdit*        pluginName;
      QCheckBox*        onOff;
      QPushButton*      loadFxButton;
      QPushButton*      clearFxButton;
      QPushButton*      expandButton;
      QSlider*          outGainSlider;
      SS_PluginChooser* pluginChooser;
      LadspaPlugin*     plugin;
      Q3ButtonGroup*     expGroup;

      int               fxid;
      bool              expanded;

      //For effect parameters:
      SS_ParameterWidgetList  paramWidgets;

   protected:

   public:
      SS_PluginFront(QWidget* parent, int id, const char* name = 0);
      void setPluginName(QString name);
      ~SS_PluginFront();
      void updatePluginValue(unsigned i);
      void clearPluginDisplay();
      void setParameterValue(int param, int val);
      void setRetGain(int val);

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
      Q3VBoxLayout* layout;
      SS_PluginFront* pluginFronts[4];

   public:
      SS_PluginGui(QWidget* parent, const char* name = 0);
      SS_PluginFront* getPluginFront(unsigned i);
      ~SS_PluginGui() {}
private slots:
    void pluginFrontSizeChanged(int fxid, int val);
   };


#endif

