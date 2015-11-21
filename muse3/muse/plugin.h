//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: plugin.h,v 1.9.2.13 2009/12/06 01:25:21 terminator356 Exp $
//
//  (C) Copyright 2000 Werner Schweer (ws@seh.de)
//  (C) Copyright 2011-2013 Tim E. Real (terminator356 on sourceforge)
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
//=========================================================

#ifndef __PLUGIN_H__
#define __PLUGIN_H__

#include <list>
#include <vector>
#include <QSet>
#include <QMap>
#include <QPair>

#include <QMouseEvent>
#include <QDialog>
#include <QTabBar>
#include <QFileInfo>
#include <QMainWindow>
#include <QUiLoader>


#include <ladspa.h>
#include "globals.h"
#include "globaldefs.h"
#include "ctrl.h"
#include "controlfifo.h"

#include "config.h"

#ifdef OSC_SUPPORT
#include "osc.h"
#endif

#ifdef DSSI_SUPPORT
#include <dssi.h>
#endif

class QAbstractButton;
class QComboBox;
class QRadioButton;
class QScrollArea;
class QToolButton;
class QToolButton;
class QTreeWidget;
class QRect;
class QByteArray;

namespace MusEGui {
class PluginGui;
}


namespace MusECore {
class AudioTrack;
class Xml;

class MidiController;


//---------------------------------------------------------
//   Plugin
//---------------------------------------------------------

class Plugin {

   protected:
   friend class PluginI;

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

      bool _isDssiSynth;
      bool _isDssi;
      bool _isLV2Synth;
      bool _isLV2Plugin;
      // Hack: Special flag required.
      bool _isDssiVst;

      #ifdef DSSI_SUPPORT
      const DSSI_Descriptor* dssi_descr;
      #endif

      unsigned long _portCount;
      unsigned long _inports;
      unsigned long _outports;
      unsigned long _controlInPorts;
      unsigned long _controlOutPorts;
      std::vector<unsigned long> rpIdx; // Port number to control input index. Item is -1 if it's not a control input.

      bool _inPlaceCapable;

   public:
      Plugin() {} //empty constructor for LV2PluginWrapper
      Plugin(QFileInfo* f, const LADSPA_Descriptor* d, bool isDssi = false, bool isDssiSynth = false);
      virtual ~Plugin();
      virtual QString label() const                        { return _label; }
      QString name() const                         { return _name; }
      unsigned long id() const                     { return _uniqueID; }
      QString maker() const                        { return _maker; }
      QString copyright() const                    { return _copyright; }
      QString lib(bool complete = true) const      { return complete ? fi.completeBaseName() : fi.baseName(); } // ddskrjo const
      QString dirPath(bool complete = true) const  { return complete ? fi.absolutePath() : fi.path(); }
      QString filePath() const                     { return fi.filePath(); }
      QString fileName() const                     { return fi.fileName(); }
      int references() const                       { return _references; }
      virtual int incReferences(int);
      int instNo()                                 { return _instNo++;        }

      bool isDssiPlugin() const { return _isDssi; }
      bool isDssiSynth() const  { return _isDssiSynth; }
      inline bool isLV2Plugin() const { return _isLV2Plugin; } //inline it to use in RT audio thread
      bool isLV2Synth() const { return _isLV2Synth; }

      virtual LADSPA_Handle instantiate(PluginI *);
      virtual void activate(LADSPA_Handle handle) {
            if (plugin && plugin->activate)
                  plugin->activate(handle);
            }
      virtual void deactivate(LADSPA_Handle handle) {
            if (plugin && plugin->deactivate)
                  plugin->deactivate(handle);
            }
      virtual void cleanup(LADSPA_Handle handle) {
            if (plugin && plugin->cleanup)
                  plugin->cleanup(handle);
            }
      virtual void connectPort(LADSPA_Handle handle, unsigned long port, float* value) {
            if(plugin)
              plugin->connect_port(handle, port, value);
            }
      virtual void apply(LADSPA_Handle handle, unsigned long n) {
            if(plugin)
              plugin->run(handle, n);
            }

      #ifdef OSC_SUPPORT
      int oscConfigure(LADSPA_Handle handle, const char* key, const char* value);
      #endif

      unsigned long ports() { return _portCount; }

      virtual LADSPA_PortDescriptor portd(unsigned long k) const {
            return plugin ? plugin->PortDescriptors[k] : 0;
            }

      virtual LADSPA_PortRangeHint range(unsigned long i) {
            // FIXME:
            //return plugin ? plugin->PortRangeHints[i] : 0; DELETETHIS
            return plugin->PortRangeHints[i];
            }

      virtual float defaultValue(unsigned long port) const;
      virtual void range(unsigned long i, float*, float*) const;
      virtual CtrlValueType ctrlValueType(unsigned long i) const;
      virtual CtrlList::Mode ctrlMode(unsigned long i) const;

      virtual const char* portName(unsigned long i) {
            return plugin ? plugin->PortNames[i] : 0;
            }

      unsigned long inports() const         { return _inports; }
      unsigned long outports() const        { return _outports; }
      unsigned long controlInPorts() const  { return _controlInPorts; }
      unsigned long controlOutPorts() const { return _controlOutPorts; }
      bool inPlaceCapable() const           { return _inPlaceCapable; }

      const std::vector<unsigned long>* getRpIdx() { return &rpIdx; }
      };

typedef std::list<Plugin *>::iterator iPlugin;


class PluginGroups : public QMap< QPair<QString, QString>, QSet<int> >
{
  public:
    QSet<int>& get(QString a, QString b) { return (*this)[(QPair<QString,QString>(a,b))]; }
    QSet<int>& get(const Plugin *p) { return (*this)[(QPair<QString,QString>(p->lib(),p->label()))]; }

    void shift_left(int first, int last);
    void shift_right(int first, int last);
    void erase(int index);

  private:
    void replace_group(int old, int now);
};


//---------------------------------------------------------
//   PluginList
//---------------------------------------------------------

class PluginList : public std::list<Plugin *> {
   public:
      void add(QFileInfo* fi, const LADSPA_Descriptor* d, bool isDssi = false, bool isDssiSynth = false)
      {
        push_back(new Plugin(fi, d, isDssi, isDssiSynth));
      }

      Plugin* find(const QString&, const QString&);
      PluginList() {}
      };

//---------------------------------------------------------
//   Port
//---------------------------------------------------------

struct Port {
      unsigned long idx;
      float val;
      float tmpVal;
      bool enCtrl;  // Enable controller stream.
      CtrlInterpolate interp;
      };

//---------------------------------------------------------
//   PluginIBase
//---------------------------------------------------------

class PluginIBase
{
   protected:
      ControlFifo _controlFifo;
      MusEGui::PluginGui* _gui;

      void makeGui();

   public:
      PluginIBase();
      virtual ~PluginIBase();
      virtual bool on() const = 0;
      virtual void setOn(bool val) = 0;
      virtual unsigned long pluginID() = 0;
      virtual int id() = 0;
      virtual QString pluginLabel() const = 0;
      virtual QString name() const = 0;
      virtual QString lib() const = 0;
      virtual QString dirPath() const = 0;
      virtual QString fileName() const = 0;
      virtual QString titlePrefix() const = 0;

      virtual AudioTrack* track() = 0;

      virtual void enableController(unsigned long i, bool v = true) = 0;
      virtual bool controllerEnabled(unsigned long i) const = 0;
      virtual void enableAllControllers(bool v = true) = 0;
      virtual void updateControllers() = 0;

      virtual void activate() = 0;
      virtual void deactivate() = 0;

      virtual void writeConfiguration(int level, Xml& xml) = 0;
      virtual bool readConfiguration(Xml& xml, bool readPreset=false) = 0;

      virtual bool addScheduledControlEvent(unsigned long i, float val, unsigned frame);    // returns true if event cannot be delivered
      virtual unsigned long parameters() const = 0;
      virtual unsigned long parametersOut() const = 0;
      virtual void setParam(unsigned long i, float val) = 0;
      virtual float param(unsigned long i) const = 0;
      virtual float paramOut(unsigned long i) const = 0;
      virtual const char* paramName(unsigned long i) = 0;
      virtual const char* paramOutName(unsigned long i) = 0;
      // FIXME TODO: Either find a way to agnosticize these two ranges, or change them from ladspa ranges to a new MusE range class.
      virtual LADSPA_PortRangeHint range(unsigned long i) = 0;
      virtual LADSPA_PortRangeHint rangeOut(unsigned long i) = 0;
      
// REMOVE Tim. Midi fixes. Added.
      virtual float latency() = 0;
      
      virtual void setCustomData(const std::vector<QString> &) {/* Do nothing by default */}
      virtual CtrlValueType ctrlValueType(unsigned long i) const = 0;
      virtual CtrlList::Mode ctrlMode(unsigned long i) const = 0;
      QString dssi_ui_filename() const;

      MusEGui::PluginGui* gui() const { return _gui; }
      void deleteGui();
};

//---------------------------------------------------------
//   PluginI
//    plugin instance
//---------------------------------------------------------

#define AUDIO_IN (LADSPA_PORT_AUDIO  | LADSPA_PORT_INPUT)
#define AUDIO_OUT (LADSPA_PORT_AUDIO | LADSPA_PORT_OUTPUT)

class PluginI : public PluginIBase {
#ifdef LV2_SUPPORT
    friend class LV2PluginWrapper;
    friend class LV2Synth;
#endif
      Plugin* _plugin;
      int channel;
      int instances;
      AudioTrack* _track;
      int _id;

      LADSPA_Handle* handle;         // per instance
      Port* controls;
      Port* controlsOut;

      unsigned long controlPorts;      
      unsigned long controlOutPorts;    
      
// REMOVE Tim. Midi fixes. Added.
      bool          _hasLatencyOutPort;
      unsigned long _latencyOutPort;
      
      bool _on;
      bool initControlValues;
      QString _name;
      QString _label;

      #ifdef OSC_SUPPORT
      OscEffectIF _oscif;
      #endif
      bool _showNativeGuiPending;

      void init();

   public:
      PluginI();
      virtual ~PluginI();

      Plugin* plugin() const { return _plugin; }
      bool on() const        { return _on; }
      void setOn(bool val)   { _on = val; }

      void setTrack(AudioTrack* t)  { _track = t; }
      AudioTrack* track()           { return _track; }
      unsigned long pluginID()      { return _plugin->id(); }
      void setID(int i);
      int id()                      { return _id; }
      void updateControllers();

      bool initPluginInstance(Plugin*, int channels);
      void setChannels(int);
      void connect(unsigned long ports, unsigned long offset, float** src, float** dst);
      void apply(unsigned pos, unsigned long n, unsigned long ports, float** bufIn, float** bufOut);

      void enableController(unsigned long i, bool v = true)   { controls[i].enCtrl = v; }
      bool controllerEnabled(unsigned long i) const           { return controls[i].enCtrl; }
      void enableAllControllers(bool v = true);

      void activate();
      void deactivate();
      QString pluginLabel() const    { return _plugin->label(); }
      QString label() const          { return _label; }
      QString name() const           { return _name; }
      QString lib() const            { return _plugin->lib(); }
      QString dirPath() const        { return _plugin->dirPath(); }
      QString fileName() const       { return _plugin->fileName(); }
      QString titlePrefix() const;

      #ifdef OSC_SUPPORT
      OscEffectIF& oscIF() { return _oscif; }

      int oscControl(unsigned long dssiPort, float val);
      int oscConfigure(const char *key, const char *val);
      int oscUpdate();
      #endif

      void writeConfiguration(int level, Xml& xml);
      bool readConfiguration(Xml& xml, bool readPreset=false);
      bool loadControl(Xml& xml);
      bool setControl(const QString& s, float val);
      void showGui();
      void showGui(bool);
      bool isDssiPlugin() const { return _plugin->isDssiPlugin(); }
      bool isLV2Plugin() const { return _plugin->isLV2Plugin(); }
      void showNativeGui();
      void showNativeGui(bool);
      bool isShowNativeGuiPending() { return _showNativeGuiPending; }
      bool guiVisible();
      bool nativeGuiVisible();

      unsigned long parameters() const           { return controlPorts; }
      unsigned long parametersOut() const           { return controlOutPorts; }
      void setParam(unsigned long i, float val);
      void putParam(unsigned long i, float val) { controls[i].val = controls[i].tmpVal = val; }
      float param(unsigned long i) const        { return controls[i].val; }
      float paramOut(unsigned long i) const        { return controlsOut[i].val; }
      float defaultValue(unsigned long param) const;
      const char* paramName(unsigned long i)     { return _plugin->portName(controls[i].idx); }
      const char* paramOutName(unsigned long i)     { return _plugin->portName(controlsOut[i].idx); }
      LADSPA_PortDescriptor portd(unsigned long i) const { return _plugin->portd(controls[i].idx); }
      void range(unsigned long i, float* min, float* max) const { _plugin->range(controls[i].idx, min, max); }
      bool isAudioIn(unsigned long k) { return (_plugin->portd(k) & AUDIO_IN) == AUDIO_IN; }
      bool isAudioOut(unsigned long k) { return (_plugin->portd(k) & AUDIO_OUT) == AUDIO_OUT; }
      LADSPA_PortRangeHint range(unsigned long i) { return _plugin->range(controls[i].idx); }
      LADSPA_PortRangeHint rangeOut(unsigned long i) { return _plugin->range(controlsOut[i].idx); }
// REMOVE Tim. Midi fixes. Added.
      float latency();
// REMOVE Tim. Midi fixes. Changed.
//       inline bool inPlaceCapable() const { return _plugin->inPlaceCapable(); }
      bool inPlaceCapable() const { return _plugin->inPlaceCapable(); }
      CtrlValueType ctrlValueType(unsigned long i) const { return _plugin->ctrlValueType(controls[i].idx); }
      CtrlList::Mode ctrlMode(unsigned long i) const { return _plugin->ctrlMode(controls[i].idx); }
      virtual void setCustomData(const std::vector<QString> &customParams);
      };

//---------------------------------------------------------
//   Pipeline
//    chain of connected efx inserts
//---------------------------------------------------------

class Pipeline : public std::vector<PluginI*> {
   private:
      float* buffer[MAX_CHANNELS];
      void initBuffers();
   public:
      Pipeline();
      Pipeline(const Pipeline&, AudioTrack*);
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
      bool isLV2Plugin(int idx) const;
      bool has_dssi_ui(int idx) const;
      void showNativeGui(int, bool);
      void deleteGui(int idx);
      void deleteAllGuis();
      bool guiVisible(int);
      bool nativeGuiVisible(int);
      void apply(unsigned pos, unsigned long ports, unsigned long nframes, float** buffer);
      void move(int idx, bool up);
      bool empty(int idx) const;
      void setChannels(int);
      bool addScheduledControlEvent(int track_ctrl_id, float val, unsigned frame); // returns true if event cannot be delivered
      void enableController(int track_ctrl_id, bool en);
      bool controllerEnabled(int track_ctrl_id);
      float latency();
      };

typedef Pipeline::iterator iPluginI;
typedef Pipeline::const_iterator ciPluginI;

extern void initPlugins();

extern bool ladspaDefaultValue(const LADSPA_Descriptor* plugin, unsigned long port, float* val);
extern void ladspaControlRange(const LADSPA_Descriptor* plugin, unsigned long port, float* min, float* max);
extern bool ladspa2MidiControlValues(const LADSPA_Descriptor* plugin, unsigned long port, int ctlnum, int* min, int* max, int* def);
extern float midi2LadspaValue(const LADSPA_Descriptor* plugin, unsigned long port, int ctlnum, int val);
extern CtrlValueType ladspaCtrlValueType(const LADSPA_Descriptor* plugin, int port);
extern CtrlList::Mode ladspaCtrlMode(const LADSPA_Descriptor* plugin, int port);

} // namespace MusECore


namespace MusEGui {
class DoubleLabel;

//   PluginLoader
//---------------------------------------------------------

class PluginLoader : public QUiLoader
{
   public:
      virtual QWidget* createWidget(const QString & className, QWidget * parent = 0, const QString & name = QString());
      PluginLoader(QObject * parent = 0) : QUiLoader(parent) {}
};

//---------------------------------------------------------
//   GuiParam
//---------------------------------------------------------

struct GuiParam {
      enum {
            GUI_SLIDER, GUI_SWITCH, GUI_METER
            };
      int type;
      int hint;
      bool pressed;

      MusEGui::DoubleLabel* label;
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
      unsigned long param;
      bool pressed;
      };

//---------------------------------------------------------
//   PluginGui
//---------------------------------------------------------

class PluginGui : public QMainWindow {
      Q_OBJECT

      MusECore::PluginIBase* plugin;        // plugin instance

      GuiParam* params;
      GuiParam* paramsOut;
      unsigned long nobj;             // number of widgets in gw
      GuiWidgets* gw;

      QAction* onOff;
      QWidget* mw;            // main widget
      QScrollArea* view;

      void updateControls();
      void getPluginConvertedValues(LADSPA_PortRangeHint range,
                     double &lower, double &upper, double &dlower, double &dupper, double &dval);
   private slots:
      void load();
      void save();
      void bypassToggled(bool);
      void sliderChanged(double, int, bool);
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
      void guiContextMenuReq(int idx);

   protected slots:
      void heartBeat();

   public:
      PluginGui(MusECore::PluginIBase*);

      ~PluginGui();
      void setOn(bool);
      void updateValues();
      };


} // namespace MusEGui

namespace MusEGlobal {
extern MusECore::PluginList plugins;
extern MusECore::PluginGroups plugin_groups;
extern QList<QString> plugin_group_names;

void writePluginGroupConfiguration(int level, MusECore::Xml& xml);
void readPluginGroupConfiguration(MusECore::Xml& xml);
}
#endif

