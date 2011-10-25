//
// C++ Interface: plugin
//
// Description:
//
//
//  (C) Copyright 2000 Werner Schweer (ws@seh.de)
// Additions/modifications: Mathias Lundgren <lunar_shuttle@users.sf.net>, (C) 2004
//                          (C) Copyright 2011 Tim E. Real (terminator356 at users.sourceforge.net)
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

#ifndef __PLUGIN_H__
#define __PLUGIN_H__

#include <QFileInfo>

#include <ladspa.h>
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
      virtual const char* getParameterName(int /*param*/) const           { return ""; }
      QString lib() const               { return fi.baseName();    }
      QString path() const              { return fi.absolutePath();     }
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


static inline float fast_log2 (float val)
      {
      /* don't use reinterpret_cast<> because that prevents this
         from being used by pure C code (for example, GnomeCanvasItems)
      */
      int* const exp_ptr = (int *)(&val);
      int x              = *exp_ptr;
      const int log_2    = ((x >> 23) & 255) - 128;
      x &= ~(255 << 23);
      x += 127 << 23;
      *exp_ptr = x;
      val = ((-1.0f/3) * val + 2) * val - 2.0f/3;   // (1)
      return (val + log_2);
      }

static inline float fast_log10 (const float val)
      {
      return fast_log2(val) / 3.312500f;
      }

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
