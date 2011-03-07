//=============================================================================
//  MusE
//  Linux Music Editor
//  $Id:$
//
//  Copyright (C) 2006 by Werner Schweer and others
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

#ifndef __LADSPAPLUGIN_H__
#define __LADSPAPLUGIN_H__

#include "ladspa.h"
#include "plugin.h"

//---------------------------------------------------------
//   LadspaPlugin
//---------------------------------------------------------

class LadspaPlugin : public Plugin {
      LADSPA_Descriptor_Function ladspa;
      const LADSPA_Descriptor* plugin;

   protected:
      int _parameter;
      std::vector<int> pIdx;

      int _inports;
      std::vector<int> iIdx;

      int _outports;
      std::vector<int> oIdx;

      bool _inPlaceCapable;
      friend class LadspaPluginIF;

   public:
      LadspaPlugin(const QFileInfo* f,
         const LADSPA_Descriptor_Function,
         const LADSPA_Descriptor* d);

      virtual QString label() const     { return QString(plugin->Label); }
      virtual QString name() const      { return QString(plugin->Name); }
      virtual unsigned long id() const  { return plugin->UniqueID; }
      virtual QString maker() const     { return QString(plugin->Maker); }
      virtual QString copyright() const { return QString(plugin->Copyright); }

      void* instantiate();
      virtual void range(int i, double*, double*) const;
      virtual int parameter() const { return _parameter;     }
      virtual int inports() const   { return _inports; }
      virtual int outports() const  { return _outports; }

      virtual bool inPlaceCapable() const { return _inPlaceCapable; }
      virtual PluginIF* createPIF(PluginI*);
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
      virtual double defaultValue(int) const;
      };

//---------------------------------------------------------
//   LadspaPort
//---------------------------------------------------------

struct LadspaPort {
      float val;
      };

//---------------------------------------------------------
//   LadspaPluginIF
//---------------------------------------------------------

class LadspaPluginIF : public PluginIF {
      const LADSPA_Descriptor* descr;
      LADSPA_Handle handle;         // per instance
      LadspaPlugin* plugin;

      LadspaPort* controls;

   public:
      LadspaPluginIF(PluginI* pi);

      virtual void apply(unsigned nframes, float** src, float** dst);
      virtual void activate();
      virtual void deactivate() {
            if (descr->deactivate)
                  descr->deactivate(handle);
            }
      virtual void cleanup() {
            if (descr->cleanup)
                  descr->cleanup(handle);
            }
      virtual const char* getParameterName(int i) const {
            return plugin->plugin->PortNames[plugin->pIdx[i]];
            }
      virtual void setParam(int i, double val) { controls[i].val = val; }
      virtual float param(int i) const        { return controls[i].val; }
      bool init(Plugin*);
      };

extern float ladspaDefaultValue(const LADSPA_Descriptor* plugin, int k);

#endif


