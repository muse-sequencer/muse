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

#include <qmainwindow.h>
#include <qstring.h>
#include <qwidget.h>
#include <qdialog.h>
#include <qfileinfo.h>
#include <qcombobox.h>

#include "ladspa.h"
#include "globals.h"
#include "globaldefs.h"
#include "ctrl.h"

class Xml;
class QWidget;
// class QLabel;
class Slider;
class QListView;
class QToolButton;
class DoubleLabel;
class AudioTrack;
class MidiController;

//---------------------------------------------------------
//   Plugin
//---------------------------------------------------------

class Plugin {
      int _references;
      int _instNo;
      QFileInfo fi;
      LADSPA_Descriptor_Function ladspa;
      const LADSPA_Descriptor *plugin;
      int _inports;
      int _outports;
      bool _inPlaceCapable;
   
   public:
      Plugin(QFileInfo* f,
         LADSPA_Descriptor_Function df, const LADSPA_Descriptor* d, bool inPlace);

      QString label() const    { return QString(plugin->Label); }
      QString name() const     { return QString(plugin->Name); }
      unsigned long id() const { return plugin->UniqueID; }
      QString maker() const    { return QString(plugin->Maker); }
      QString copyright() const { return QString(plugin->Copyright); }
      QString lib() const      { return fi.baseName(true); }
      QString path() const     { return fi.dirPath(); }
      int references() const   { return _references; }
      int incReferences(int n) { return _references += n; }
      int instNo()             { return _instNo++;        }

      LADSPA_Handle instantiate() {
            return plugin->instantiate(plugin, sampleRate);
            }
      void activate(LADSPA_Handle handle) {
            if (plugin->activate)
                  plugin->activate(handle);
            }
      void deactivate(LADSPA_Handle handle) {
            if (plugin->deactivate)
                  plugin->deactivate(handle);
            }
      void cleanup(LADSPA_Handle handle) {
            if (plugin->cleanup)
                  plugin->cleanup(handle);
            }
      void connectPort(LADSPA_Handle handle, int port, float* value) {
            plugin->connect_port(handle, port, value);
            }
      void apply(LADSPA_Handle handle, int n) {
            plugin->run(handle, n);
            }
      int ports() { return plugin->PortCount; }
      double defaultValue(unsigned int port) const;
      LADSPA_PortDescriptor portd(int k) const {
            return plugin->PortDescriptors[k];
            }
      void range(int i, float*, float*) const;
      LADSPA_PortRangeHint range(int i) {
            return plugin->PortRangeHints[i];
            }

      const char* portName(int i) {
            return plugin->PortNames[i];
            }
      int inports() const  { return _inports; }
      int outports() const { return _outports; }
      bool inPlaceCapable() const { return _inPlaceCapable; }
      };

typedef std::list<Plugin>::iterator iPlugin;

//---------------------------------------------------------
//   PluginList
//---------------------------------------------------------

class PluginList : public std::list<Plugin> {
   public:
      void add(QFileInfo* fi, LADSPA_Descriptor_Function df,
         const LADSPA_Descriptor* d, bool inPlaceOk) {
            push_back(Plugin(fi, df, d, inPlaceOk));
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

//---------------------------------------------------------
//   PluginGui
//---------------------------------------------------------

class PluginGui : public QMainWindow {
      Q_OBJECT

      PluginI* plugin;        // plugin instance
      GuiParam* params;
      int nobj;               // number of widgets in gw
      GuiWidgets* gw;

      QToolButton* onOff;
      QWidget* mw;            // main widget

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
      PluginGui(PluginI*);
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

class PluginI {
      Plugin* _plugin;
      int channel;
      int instances;
      AudioTrack* _track;
      int _id;

      LADSPA_Handle* handle;         // per instance
      Port* controls;

      int controlPorts;
      PluginGui* _gui;
      bool _on;
      bool initControlValues;
      QString _name;
      QString _label;

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
      QString label() const          { return _label; }
      QString name() const           { return _name; }
      CtrlValueType valueType() const;
      QString lib() const            { return _plugin->lib(); }

      void writeConfiguration(int level, Xml& xml);
      bool readConfiguration(Xml& xml, bool readPreset=false);
      bool loadControl(Xml& xml);
      bool setControl(const QString& s, double val);
      void showGui();
      void showGui(bool);
      bool guiVisible();
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
      void deleteGui(int idx);
      void deleteAllGuis();
      bool guiVisible(int);
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

class PluginDialog : public QDialog {
      QListView* pList;

      Q_OBJECT

   public:
      PluginDialog(QWidget* parent=0, const char* name=0, bool modal=true);
      static Plugin* getPlugin(QWidget* parent);
      Plugin* value();
      void accept();
public slots:
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

