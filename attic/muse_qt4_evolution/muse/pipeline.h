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

#ifndef __PIPELINE_H__
#define __PIPELINE_H__

#include "globaldefs.h"

class PluginI;

//---------------------------------------------------------
//   Pipeline
//    chain of connected efx inserts
//---------------------------------------------------------

class Pipeline : public QList<PluginI*> {
      float* buffer[MAX_CHANNELS];

   public:
      Pipeline();
      ~Pipeline();

      Pipeline(const Pipeline&);
      Pipeline& operator=(const Pipeline&);     // disable copies

      bool isOn(int idx) const;
      void setOn(int, bool);
      QString label(int idx) const;
      QString name(int idx) const;
      bool hasNativeGui(int idx) const;
      void showGui(int, bool);
      bool guiVisible(int);
      bool nativeGuiVisible(int);
      void showNativeGui(int, bool);
      void apply(int ports, unsigned long nframes, float** buffer);
      void move(int idx, bool up);
      void setChannels(int);
      PluginI* plugin(int idx) { return value(idx); }
      };

#endif

