//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: plugin.h,v 1.9.2.13 2009/12/06 01:25:21 terminator356 Exp $
//
//  (C) Copyright 2000 Werner Schweer (ws@seh.de)
//=========================================================

#ifndef __PLUGIN_H__
#define __PLUGIN_H__

#include <list>
#include <vector>

#include <q3mainwindow.h>
#include <qstring.h>
#include <qwidget.h>
//#include <qwidgetfactory.h>
#include <qdialog.h>
#include <qfileinfo.h>
#include <qcombobox.h>

#include "ladspa.h"
#include "globals.h"
#include "globaldefs.h"
#include "ctrl.h"

//#include "stringparam.h"

#include "config.h"
 
#ifdef OSC_SUPPORT
//class OscIF;
#include "osc.h"
#endif

#ifdef DSSI_SUPPORT
#include <dssi.h>
#endif

class Xml;
class QWidget;
// class QLabel;
class Slider;
class Q3ListView;
class Q3ScrollView;
class QToolButton;
class DoubleLabel;
class AudioTrack;
class MidiController;

//---------------------------------------------------------
//   PluginWidgetFactory
//---------------------------------------------------------
#if 0
class PluginWidgetFactory : public QWidgetFactory
{
  public:
    virtual QWidget* createWidget(const QString& className, QWidget* parent, const char* name) const; 
};
#endif
//---------------------------------------------------------
//   Plugin
//---------------------------------------------------------

class Plugin {
   protected:
      void* _handle;
      int _references;
      int _instNo;
      QFileInfo fi;
      LADSPA_Descriptor_Function ladspa;
      const LADSPA_Descriptor *plugin;
      unsigned long _uniqueID;
      QString _label;
      QString _name;
      QString _maker;
      QString _copyright;
      
      bool _isDssi;
      #ifdef DSSI_SUPPORT
      const DSSI_Descriptor* dssi_descr;
      #endif
      
      //LADSPA_PortDescriptor* _portDescriptors;
      unsigned long _portCount;
      unsigned long _inports;
      unsigned long _outports;
      unsigned long _controlInPorts;
      unsigned long _controlOutPorts;
      std::vector<unsigned long> rpIdx; // Port number to control input index. Item is -1 if it's not a control input.
      
      bool _inPlaceCapable;
   
   public:
      Plugin(QFileInfo* f, const LADSPA_Descriptor* d, bool isDssi = false);
      ~Plugin();
      
      QString label() const                        { return _label; }
      QString name() const                         { return _name; }
      unsigned long id() const                     { return _uniqueID; }
      QString maker() const                        { return _maker; }
      QString copyright() const                    { return _copyright; }
      QString lib(bool complete = true) /*const*/      { return fi.baseName(complete); } // ddskrjo const
      QString dirPath(bool complete = true) const  { return fi.dirPath(complete); }
      QString filePath() const                     { return fi.filePath(); }
      int references() const                       { return _references; }
      int incReferences(int);
      int instNo()                                 { return _instNo++;        }

      bool isDssiPlugin() const { return _isDssi; }  
      
      LADSPA_Handle instantiate(); 
      void activate(LADSPA_Handle handle) {
            if (plugin && plugin->activate)
                  plugin->activate(handle);
            }
      void deactivate(LADSPA_Handle handle) {
            if (plugin && plugin->deactivate)
                  plugin->deactivate(handle);
            }
      void cleanup(LADSPA_Handle handle) {
            if (plugin && plugin->cleanup)
                  plugin->cleanup(handle);
            }
      void connectPort(LADSPA_Handle handle, int port, float* value) {
            if(plugin)
              plugin->connect_port(handle, port, value);
            }
      void apply(LADSPA_Handle handle, int n) {
            if(plugin)
              plugin->run(handle, n);
            }
      
      #ifdef OSC_SUPPORT
      int oscConfigure(LADSPA_Handle /*handle*/, const char* /*key*/, const char* /*value*/);
      #endif
      
      //int ports() { return plugin ? plugin->PortCount : 0; }
      unsigned long ports() { return _portCount; }
      
      LADSPA_PortDescriptor portd(unsigned long k) const {
            return plugin ? plugin->PortDescriptors[k] : 0;
            //return _portDescriptors[k];
            }
      
      LADSPA_PortRangeHint range(unsigned long i) {
            // FIXME:
            //return plugin ? plugin->PortRangeHints[i] : 0;
            return plugin->PortRangeHints[i];
            }

      double defaultValue(unsigned long port) const;
      void range(unsigned long i, float*, float*) const;
      
      const char* portName(unsigned long i) {
            return plugin ? plugin->PortNames[i] : 0;
            }
            
      // Returns (int)-1 if not an input control.   
      unsigned long port2InCtrl(unsigned long p) { return p >= rpIdx.size() ? (unsigned long)-1 : rpIdx[p]; }   
      
      unsigned long inports() const         { return _inports; }
      unsigned long outports() const        { return _outports; }
      unsigned long controlInPorts() const  { return _controlInPorts; }
      unsigned long controlOutPorts() const { return _controlOutPorts; }
      bool inPlaceCapable() const           { return _inPlaceCapable; }
      };

typedef std::list<Plugin>::iterator iPlugin;

//---------------------------------------------------------
//   PluginList
//---------------------------------------------------------

class PluginList : public std::list<Plugin> {
   public:
      void add(QFileInfo* fi, const LADSPA_Descriptor* d, bool isDssi = false) 
      {
        push_back(Plugin(fi, d, isDssi));
      }
      
      Plugin* find(const QString&, const QString&);
      PluginList() {}
      };

//---------------------------------------------------------
//   Port
//---------------------------------------------------------

struct Port {
      int idx;
      float val;
      float tmpVal;
      
      bool enCtrl;  // Enable controller stream.
      bool en2Ctrl; // Second enable controller stream (and'ed with enCtrl).
      };

//---------------------------------------------------------
//   GuiParam
//---------------------------------------------------------

struct GuiParam {
      enum {
            GUI_SLIDER, GUI_SWITCH
            };
      int type;
      int hint;
      
      DoubleLabel* label;
      QWidget* actuator;  // Slider or Toggle Button (SWITCH)
      };

//---------------------------------------------------------
//   GuiWidgets
//---------------------------------------------------------

struct GuiWidgets {
      enum {
            SLIDER, DOUBLE_LABEL, QCHECKBOX, QCOMBOBOX
            };
      QWidget* widget;
      int type;
      int param;
      };

class PluginI;

/*
class PluginBase 
{
   public:
      bool on() const        { return _on; }
      void setOn(bool val)   { _on = val; }
      int pluginID()                { return plugin()->id(); }
      int id()                      { return _id; }
      QString pluginLabel() const    { return _plugin->label(); }
      QString name() const           { return _name; }
      
      AudioTrack* track()           { return _track; }
      
      void enableController(int i, bool v = true)   { controls[i].enCtrl = v; }
      bool controllerEnabled(int i) const           { return controls[i].enCtrl; }
      bool controllerEnabled2(int i) const          { return controls[i].en2Ctrl; }
      void updateControllers();
      
      void writeConfiguration(int level, Xml& xml);
      bool readConfiguration(Xml& xml, bool readPreset=false);
      
      int parameters() const           { return controlPorts; }
      void setParam(int i, double val) { controls[i].tmpVal = val; }
      double param(int i) const        { return controls[i].val; }
      const char* paramName(int i)     { return _plugin->portName(controls[i].idx); }
      LADSPA_PortRangeHint range(int i) 
      {
            return _plugin->range(controls[i].idx);
      }
};
*/

//---------------------------------------------------------
//   PluginIBase 
//---------------------------------------------------------

class PluginIBase 
{
   public:
      virtual bool on() const = 0;       
      virtual void setOn(bool /*val*/) = 0;   
      virtual int pluginID() = 0;
      virtual int id() = 0;
      virtual QString pluginLabel() const = 0;  
      virtual QString name() const = 0;
      
      virtual AudioTrack* track() = 0;          
      
      virtual void enableController(int /*i*/, bool v = true) = 0; 
      virtual bool controllerEnabled(int /*i*/) const = 0;          
      virtual bool controllerEnabled2(int /*i*/) const = 0;          
      virtual void updateControllers() = 0;
      
      virtual void writeConfiguration(int /*level*/, Xml& /*xml*/) = 0;
      virtual bool readConfiguration(Xml& /*xml*/, bool readPreset=false) = 0;
      
      virtual int parameters() const = 0;          
      virtual void setParam(int /*i*/, double /*val*/) = 0; 
      virtual double param(int /*i*/) const = 0;        
      virtual const char* paramName(int /*i*/) = 0;     
      virtual LADSPA_PortRangeHint range(int /*i*/) = 0; 
};

//---------------------------------------------------------
//   PluginGui
//---------------------------------------------------------

class PluginGui : public Q3MainWindow {
      Q_OBJECT

      //PluginI* plugin;        // plugin instance
      PluginIBase* plugin;        // plugin instance
      
      GuiParam* params;
      int nobj;               // number of widgets in gw
      GuiWidgets* gw;

      QToolButton* onOff;
      QWidget* mw;            // main widget
      Q3ScrollView* view;

      void updateControls();

   private slots:
      void load();
      void save();
      void bypassToggled(bool);
      void sliderChanged(double, int);
      void labelChanged(double, int);
      void guiParamChanged(int);
      void ctrlPressed(int);
      void ctrlReleased(int);
      void guiParamPressed(int);
      void guiParamReleased(int);
      void guiSliderPressed(int);
      void guiSliderReleased(int);
      void ctrlRightClicked(const QPoint &, int);
      void guiSliderRightClicked(const QPoint &, int);

   protected slots:
      void heartBeat();

   public:
      //PluginGui(PluginI*);
      PluginGui(PluginIBase*);
      
      ~PluginGui();
      void setOn(bool);
      void updateValues();
      };

//---------------------------------------------------------
//   PluginI
//    plugin instance
//---------------------------------------------------------

#define AUDIO_IN (LADSPA_PORT_AUDIO  | LADSPA_PORT_INPUT)
#define AUDIO_OUT (LADSPA_PORT_AUDIO | LADSPA_PORT_OUTPUT)

//class PluginI {
class PluginI : public PluginIBase {
      Plugin* _plugin;
      int channel;
      int instances;
      AudioTrack* _track;
      int _id;

      LADSPA_Handle* handle;         // per instance
      Port* controls;
      Port* controlsOut;

      int controlPorts;
      int controlOutPorts;
      PluginGui* _gui;
      bool _on;
      bool initControlValues;
      QString _name;
      QString _label;

      //#ifdef DSSI_SUPPORT
      //StringParamMap _stringParamMap;
      //#endif
      
      #ifdef OSC_SUPPORT
      OscEffectIF _oscif;
      #endif
      bool _showNativeGuiPending;

      void init();
      void makeGui();
      
   public:
      PluginI();
      ~PluginI();

      Plugin* plugin() const { return _plugin; }
      bool on() const        { return _on; }
      void setOn(bool val)   { _on = val; }
      PluginGui* gui() const { return _gui; }
      void deleteGui();
      
      void setTrack(AudioTrack* t)  { _track = t; }
      AudioTrack* track()           { return _track; }
      int pluginID()                { return _plugin->id(); }
      void setID(int i);
      int id()                      { return _id; }
      void updateControllers();
      
      bool initPluginInstance(Plugin*, int channels);
      void setChannels(int);
      void connect(int ports, float** src, float** dst);
      void apply(int n);

      void enableController(int i, bool v = true)   { controls[i].enCtrl = v; }
      bool controllerEnabled(int i) const           { return controls[i].enCtrl; }
      void enable2Controller(int i, bool v = true)  { controls[i].en2Ctrl = v; }
      bool controllerEnabled2(int i) const          { return controls[i].en2Ctrl; }
      void enableAllControllers(bool v = true);
      void enable2AllControllers(bool v = true);
      
      void activate();
      void deactivate();
      QString pluginLabel() const    { return _plugin->label(); }
      QString label() const          { return _label; }
      QString name() const           { return _name; }
      CtrlValueType valueType() const;
      QString lib() const            { return _plugin->lib(); }

      #ifdef OSC_SUPPORT
      OscEffectIF& oscIF() { return _oscif; }
      /*
      int oscConfigure(lo_arg**);
      int oscControl(lo_arg**);
      //int oscUpdate(lo_arg**);
      //int oscExiting(lo_arg**);
      */
      
      int oscControl(unsigned long /*dssiPort*/, float /*val*/);
      int oscConfigure(const char */*key*/, const char */*val*/);
      int oscUpdate();
      //int oscExiting();
      #endif
      
      void writeConfiguration(int level, Xml& xml);
      bool readConfiguration(Xml& xml, bool readPreset=false);
      bool loadControl(Xml& xml);
      bool setControl(const QString& s, double val);
      void showGui();
      void showGui(bool);
      bool isDssiPlugin() const { return _plugin->isDssiPlugin(); }  
      void showNativeGui();
      void showNativeGui(bool);
      bool isShowNativeGuiPending() { return _showNativeGuiPending; }
      bool guiVisible();
      bool nativeGuiVisible();
      int parameters() const           { return controlPorts; }
      void setParam(int i, double val) { controls[i].tmpVal = val; }
      double param(int i) const        { return controls[i].val; }
      double defaultValue(unsigned int param) const;
      const char* paramName(int i)     { return _plugin->portName(controls[i].idx); }
      LADSPA_PortDescriptor portd(int i) const { return _plugin->portd(controls[i].idx); }
      void range(int i, float* min, float* max) const {
            _plugin->range(controls[i].idx, min, max);
            }
      bool isAudioIn(int k) {
            return (_plugin->portd(k) & AUDIO_IN) == AUDIO_IN;
            }
      bool isAudioOut(int k) {
            return (_plugin->portd(k) & AUDIO_OUT) == AUDIO_OUT;
            }
      bool inPlaceCapable() const { return _plugin->inPlaceCapable(); }
      LADSPA_PortRangeHint range(int i) {
            return _plugin->range(controls[i].idx);
            }
      };

//---------------------------------------------------------
//   Pipeline
//    chain of connected efx inserts
//---------------------------------------------------------

const int PipelineDepth = 4;

class Pipeline : public std::vector<PluginI*> {
      float* buffer[MAX_CHANNELS];
   
   public:
      Pipeline();
      ~Pipeline();
      
      void insert(PluginI* p, int index);
      void remove(int index);
      void removeAll();
      bool isOn(int idx) const;
      void setOn(int, bool);
      QString label(int idx) const;
      QString name(int idx) const;
      void showGui(int, bool);
      bool isDssiPlugin(int) const; 
      void showNativeGui(int, bool);
      void deleteGui(int idx);
      void deleteAllGuis();
      bool guiVisible(int);
      bool nativeGuiVisible(int);
      void apply(int ports, unsigned long nframes, float** buffer);
      void move(int idx, bool up);
      bool empty(int idx) const;
      void setChannels(int);
      };

typedef Pipeline::iterator iPluginI;
typedef Pipeline::const_iterator ciPluginI;

//---------------------------------------------------------
//   PluginDialog
//---------------------------------------------------------

enum { SEL_SM, SEL_S, SEL_M, SEL_ALL };

class PluginDialog : public QDialog {
      QTreeWidget* pList;
      QRadioButton* allPlug;
      QRadioButton* onlyM;
      QRadioButton* onlyS;
      QRadioButton* onlySM;

      Q_OBJECT

   public:
      PluginDialog(QWidget* parent=0);
      static Plugin* getPlugin(QWidget* parent);
      Plugin* value();
      void accept();
public slots:
    void fillPlugs(QAbstractButton*);
    void fillPlugs(int i);
    void fillPlugs(const QString& sortValue);
  private:
    QComboBox *sortBox;
    static int selectedPlugType;
    static QStringList sortItems;
      };

extern void initPlugins();
extern PluginList plugins;

extern bool ladspaDefaultValue(const LADSPA_Descriptor* plugin, int port, float* val);
extern void ladspaControlRange(const LADSPA_Descriptor* plugin, int i, float* min, float* max);
extern bool ladspa2MidiControlValues(const LADSPA_Descriptor* plugin, int port, int ctlnum, int* min, int* max, int* def);
//extern MidiController* ladspa2MidiController(const LADSPA_Descriptor* plugin, int port, int ctlnum);
extern float midi2LadspaValue(const LADSPA_Descriptor* plugin, int port, int ctlnum, int val);

#endif

