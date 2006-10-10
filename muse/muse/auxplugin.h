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

#ifndef __AUXPLUGIN_H__
#define __AUXPLUGIN_H__

#include "plugin.h"

//---------------------------------------------------------
//   AuxPlugin
//---------------------------------------------------------

class AuxPlugin : public Plugin {

   public:
      AuxPlugin();

      virtual QString label() const     { return "Aux"; }
      virtual QString name() const      { return "Aux"; }
      virtual unsigned long id() const  { return 0; }
      virtual QString maker() const     { return ""; }
      virtual QString copyright() const { return ""; }

      void* instantiate()           { return 0; }
      virtual void range(int i, double*, double*) const;
      virtual int parameter() const { return 2;     }
      virtual int inports() const   { return 100; }
      virtual int outports() const  { return 100; }

      virtual bool inPlaceCapable() const { return true; }
      virtual PluginIF* createPIF(PluginI*);

      virtual bool isLog(int k) const;
      virtual bool isBool(int k) const;
      virtual bool isInt(int k) const;
      virtual double defaultValue(int) const;
      };

//---------------------------------------------------------
//   AuxPluginIF
//---------------------------------------------------------

class AuxPluginIF : public PluginIF {
      double volume;
      double pan;
      float** buffer;               // this buffer is filled by apply()
                                    // _volume and _pan is applied

   public:
      AuxPluginIF(PluginI* pi);
      virtual ~AuxPluginIF();

      virtual void apply(unsigned nframes, float** src, float** dst);
      virtual void activate() {}
      virtual void deactivate() {}
      virtual void cleanup() {}
      virtual const char* getParameterName(int i) const;
      virtual void setParam(int i, double val);
      virtual float param(int i) const;
      bool init(Plugin*);
      };

extern AuxPlugin* auxPlugin;
#endif
