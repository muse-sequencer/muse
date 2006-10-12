//=============================================================================
//  MusE
//  Linux Music Editor
//  $Id:$
//
//  Copyright (C) 2002-2006 by Werner Schweer and others
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License version 2.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//=============================================================================

#ifndef __PLUGIN_H__
#define __PLUGIN_H__

#include "ladspa.h"
#include "globals.h"

namespace AL {
      class Xml;
      };
using AL::Xml;

class PluginIF;
class PluginGui;
class PluginI;
class LadspaPluginIF;
class AudioTrack;
class Ctrl;

#define AUDIO_IN (LADSPA_PORT_AUDIO  | LADSPA_PORT_INPUT)
#define AUDIO_OUT (LADSPA_PORT_AUDIO | LADSPA_PORT_OUTPUT)

//---------------------------------------------------------
//   Plugin
//---------------------------------------------------------

class Plugin {

   protected:
      int _instances;
      QFileInfo fi;

   public:
      Plugin(const QFileInfo* f);
      virtual ~Plugin() {}

      int instances() const              { return _instances; }
      virtual void incInstances(int val) { _instances += val; }

      QString lib() const               { return fi.baseName();    }
      QString path() const              { return fi.absolutePath();     }

      virtual QString label() const     { return QString(); }
      virtual QString name() const      { return QString(); }
      virtual unsigned long id() const  { return 0;         }
      virtual QString maker() const     { return QString(); }
      virtual QString copyright() const { return QString(); }

      virtual PluginIF* createPIF(PluginI*) = 0;

      virtual void range(int, double* min, double* max) const {
            *min = 0.0f;
            *max = 1.0f;
            }

      virtual int parameter() const       { return 0;     }
      virtual int inports() const         { return 0;     }
      virtual int outports() const        { return 0;     }

      virtual bool inPlaceCapable() const { return false; }

      virtual bool isLog(int) const       { return false; }
      virtual bool isBool(int) const      { return false; }
      virtual bool isInt(int) const       { return false; }
      virtual double defaultValue(int) const { return 0.0f;  }
      };

//---------------------------------------------------------
//   PluginList
//---------------------------------------------------------

typedef std::list<Plugin*>::iterator iPlugin;

class PluginList : public std::list<Plugin*> {
   public:
      Plugin* find(const QString&, const QString&);
      PluginList() {}
      };

//---------------------------------------------------------
//   PluginIF
//    plugin instance interface
//---------------------------------------------------------

class PluginIF {
   protected:
      PluginI* pluginI;

   public:
      PluginIF(PluginI* pi) { pluginI = pi; }
      virtual ~PluginIF() {}

      PluginI* pluginInstance() { return pluginI; }
      const PluginI* pluginInstance() const { return pluginI; }

      virtual void apply(unsigned nframes, float** src, float** dst) = 0;
      virtual void activate() = 0;
      virtual void deactivate() = 0;
      virtual void cleanup() = 0;
      virtual void setParam(int i, double val) = 0;
      virtual float param(int i) const = 0;
      virtual const char* getParameterName(int) const           { return ""; }
      virtual const char* getParameterLabel(int) const          { return 0; }
      virtual const char* getParameterDisplay(int, float) const { return 0; }
      virtual bool hasGui() const { return false; }
      virtual bool guiVisible() const { return false; }
      virtual void showGui(bool) {}
      };

//---------------------------------------------------------
//   PluginI
//    plugin instance
//---------------------------------------------------------

class PluginI {
      Plugin* _plugin;
      AudioTrack* _track;

      int instances;
      PluginIF** pif;

      int _channel;

      PluginGui* _gui;
      bool _on;

      QString _name;
      QString _label;

      std::vector<Ctrl*> controllerList;

      void makeGui();

   protected:
      bool initControlValues;
      friend class LadspaPluginIF;

   public:
      PluginI(AudioTrack*);
      ~PluginI();

      Plugin* plugin() const { return _plugin; }
      bool on() const        { return _on; }
      void setOn(bool val)   { _on = val; }
      PluginGui* gui() const { return _gui; }

      bool initPluginInstance(Plugin*, int channels);
      void setChannels(int);
      int channel() const    { return _channel; }
      void apply(unsigned nframes, int ports, float** b1, float** b2);

      void activate();
      void deactivate();
      QString label() const          { return _label; }
      QString name() const           { return _name; }
      QString lib() const            { return _plugin->lib(); }

      AudioTrack* track() const      { return _track; }

      void writeConfiguration(Xml&, bool);
      void writeConfiguration1(Xml&, bool); // without end tag!
      bool readConfiguration(QDomNode, bool*);

      void showGui();
      void showGui(bool);
      bool guiVisible() const;

      bool hasNativeGui() const                  { return pif[0]->hasGui(); }
      void showNativeGui(bool f)                 { return pif[0]->showGui(f); }
      bool nativeGuiVisible() const              { return pif[0]->guiVisible(); }

      void setControllerList(Ctrl* cl)           { controllerList.push_back(cl); }
      Ctrl* controller(int idx) const            { return controllerList[idx]; }
      bool setParameter(const QString& s, double val);
      void setParam(int i, double val);
      double param(int i) const                   { return pif[0]->param(i); }

      const char* getParameterName(int i) const  { return pif[0]->getParameterName(i); }
      const char* getParameterLabel(int i) const { return pif[0]->getParameterLabel(i); }
      const char* getParameterDisplay(int i, float v) const { return pif[0]->getParameterDisplay(i, v); }

      void range(int i, double* min, double* max) const {
            _plugin->range(i, min, max);
            }
      double defaultValue(int i) const { return _plugin->defaultValue(i);   }
      bool inPlaceCapable() const      { return _plugin->inPlaceCapable(); }

      bool isLog(int k) const          { return _plugin->isLog(k);  }
      bool isBool(int k) const         { return _plugin->isBool(k); }
      bool isInt(int k) const          { return _plugin->isInt(k);  }

      PluginIF* pluginIF(int idx)      { return pif[idx]; }
      };

extern void initPlugins();
extern PluginList plugins;

#endif

