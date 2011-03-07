//
// C++ Interface: plugin
//
// Description:
//
//
//  (C) Copyright 2000 Werner Schweer (ws@seh.de)
// Additions/modifications: Mathias Lundgren <lunar_shuttle@users.sf.net>, (C) 2004
// Copyright: See COPYING file that comes with this distribution
//
//

#ifndef __PLUGIN_H__
#define __PLUGIN_H__

//#include <ladspa.h>
#include "muse/ladspa.h"
#include "muse/fastlog.h"
#include <math.h>

//---------------------------------------------------------
//   Port
//---------------------------------------------------------

struct Port {
      float val;
      };

//---------------------------------------------------------
//   Plugin
//---------------------------------------------------------

class Plugin
   {
   protected:
      QFileInfo fi;

   public:
      Plugin(const QFileInfo* f);
      virtual ~Plugin() {}
      virtual QString label() const     { return QString(); }
      virtual QString name() const      { return QString(); }
      virtual unsigned long id() const  { return 0;         }
      virtual QString maker() const     { return QString(); }
      virtual QString copyright() const { return QString(); }
      virtual int parameter() const       { return 0;     }
      virtual int inports() const         { return 0;     }
      virtual int outports() const        { return 0;     }
      virtual bool inPlaceCapable() const { return false; }

      virtual bool isLog(int) const       { return false; }
      virtual bool isBool(int) const      { return false; }
      virtual bool isInt(int) const       { return false; }
      virtual float defaultValue(int) const { return 0.0f;  }
      virtual void range(int, float* min, float* max) const {
            *min = 0.0f;
            *max = 1.0f;
            }
      virtual const char* getParameterName(int /*param*/) const  { return ""; } //prevnt unused parameter
      QString lib() const               { return fi.baseName();    }
      QString path() const              { return fi.dirPath();     }
   };

//---------------------------------------------------------
//   LadspaPlugin
//---------------------------------------------------------

class LadspaPlugin : public Plugin
   {
      LADSPA_Descriptor_Function ladspa;
      const LADSPA_Descriptor* plugin;
      LADSPA_Handle handle;
      bool active;

      Port* controls;
      Port* inputs;
      Port* outputs;

   protected:
      int _parameter;
      std::vector<int> pIdx; //control port numbers

      int _inports;
      std::vector<int> iIdx; //input port numbers

      int _outports;
      std::vector<int> oIdx; //output port numbers

      bool _inPlaceCapable;

   public:
      LadspaPlugin(const QFileInfo* f, const LADSPA_Descriptor_Function, const LADSPA_Descriptor* d);
      virtual ~LadspaPlugin();
      virtual QString label() const     { return QString(plugin->Label); }
      virtual QString name() const      { return QString(plugin->Name); }
      virtual unsigned long id() const  { return plugin->UniqueID; }
      virtual QString maker() const     { return QString(plugin->Maker); }
      virtual QString copyright() const { return QString(plugin->Copyright); }
      virtual int parameter() const { return _parameter;     }
      virtual int inports() const   { return _inports; }
      virtual int outports() const  { return _outports; }
      virtual bool inPlaceCapable() const { return _inPlaceCapable; }
      const LADSPA_Descriptor* ladspaDescriptor() const { return plugin; }
      virtual bool isLog(int k) const {
            LADSPA_PortRangeHint r = plugin->PortRangeHints[pIdx[k]];
            return LADSPA_IS_HINT_LOGARITHMIC(r.HintDescriptor);
            }
      virtual bool isBool(int k) const {
            return LADSPA_IS_HINT_TOGGLED(plugin->PortRangeHints[pIdx[k]].HintDescriptor);
            }
      virtual bool isInt(int k) const {
            LADSPA_PortRangeHint r = plugin->PortRangeHints[pIdx[k]];
            return LADSPA_IS_HINT_INTEGER(r.HintDescriptor);
            }
      virtual void range(int i, float*, float*) const;
      virtual const char* getParameterName(int i) const {
            return plugin->PortNames[pIdx[i]];
            }
      virtual float defaultValue(int) const;
      virtual float getControlValue(int k) const {
            return controls[k].val;
            }

      int   getGuiControlValue(int parameter) const;
      float convertGuiControlValue(int parameter, int val) const;

      bool instantiate();
      bool start();
      void stop();
      void connectInport(int k, LADSPA_Data* datalocation);
      void connectOutport(int k, LADSPA_Data* datalocation);
      void process(unsigned long);
      void setParam(int i, float val);

   };

//---------------------------------------------------------
//   PluginList
//---------------------------------------------------------

typedef std::list<Plugin*>::iterator iPlugin;

class PluginList : public std::list<Plugin*> {
   public:
      Plugin* find(const QString& file, const QString& name);
      PluginList() {}
      };

extern void SS_initPlugins();
extern PluginList plugins;

#endif
