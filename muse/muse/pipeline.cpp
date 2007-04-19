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

#include "pipeline.h"
#include "plugin.h"
#include "plugingui.h"
#include "al/dsp.h"

//---------------------------------------------------------
//   Pipeline
//---------------------------------------------------------

Pipeline::Pipeline()
      {
      for (int i = 0; i < MAX_CHANNELS; ++i)
            posix_memalign((void**)(buffer + i), 16, sizeof(float) * segmentSize);
      }

Pipeline::Pipeline(const Pipeline& p)
  : QList<PluginI*>(p)
      {
      for (int i = 0; i < MAX_CHANNELS; ++i)
            posix_memalign((void**)(buffer + i), 16, sizeof(float) * segmentSize);
      }

//---------------------------------------------------------
//   Pipeline
//---------------------------------------------------------

Pipeline::~Pipeline()
      {
      for (int i = 0; i < MAX_CHANNELS; ++i)
            ::free(buffer[i]);
      }

//---------------------------------------------------------
//   setChannels
//---------------------------------------------------------

void Pipeline::setChannels(int n)
      {
      foreach(PluginI* plugin, *this)
            plugin->setChannels(n);
      }

//---------------------------------------------------------
//   isOn
//---------------------------------------------------------

bool Pipeline::isOn(int idx) const
      {
      PluginI* p = value(idx);
      if (p)
            return p->on();
      return false;
      }

//---------------------------------------------------------
//   setOn
//---------------------------------------------------------

void Pipeline::setOn(int idx, bool flag)
      {
      PluginI* p = value(idx);
      if (p) {
            p->setOn(flag);
            if (p->gui())
                  p->gui()->setOn(flag);
            }
      }

//---------------------------------------------------------
//   label
//---------------------------------------------------------

QString Pipeline::label(int idx) const
      {
      PluginI* p = value(idx);
      if (p)
            return p->label();
      return QString("");
      }

//---------------------------------------------------------
//   name
//---------------------------------------------------------

QString Pipeline::name(int idx) const
      {
      PluginI* p = value(idx);
      if (p)
            return p->name();
      return QString("empty");
      }

//---------------------------------------------------------
//   hasNativeGui
//---------------------------------------------------------

bool Pipeline::hasNativeGui(int idx) const
      {
      PluginI* p = value(idx);
      if (p)
            return p->hasNativeGui();
      return false;
      }

//---------------------------------------------------------
//   move
//---------------------------------------------------------

void Pipeline::move(int idx, bool up)
      {
      PluginI* p1 = (*this)[idx];
      if (up) {
            (*this)[idx]   = (*this)[idx-1];
            (*this)[idx-1] = p1;
            }
      else {
            (*this)[idx]   = (*this)[idx+1];
            (*this)[idx+1] = p1;
            }
      }

//---------------------------------------------------------
//   showGui
//---------------------------------------------------------

void Pipeline::showGui(int idx, bool flag)
      {
      PluginI* p = (*this)[idx];
      if (p)
            p->showGui(flag);
      }

//---------------------------------------------------------
//   showNativeGui
//---------------------------------------------------------

void Pipeline::showNativeGui(int idx, bool flag)
      {
      PluginI* p = (*this)[idx];
      if (p)
            p->showNativeGui(flag);
      }

//---------------------------------------------------------
//   guiVisible
//---------------------------------------------------------

bool Pipeline::guiVisible(int idx)
      {
      PluginI* p = (*this)[idx];
      if (p)
            return p->guiVisible();
      return false;
      }

//---------------------------------------------------------
//   nativeGuiVisible
//---------------------------------------------------------

bool Pipeline::nativeGuiVisible(int idx)
      {
      PluginI* p = (*this)[idx];
      if (p)
            return p->nativeGuiVisible();
      return false;
      }

//---------------------------------------------------------
//   apply
//---------------------------------------------------------

void Pipeline::apply(int ports, unsigned long nframes, float** buffer1)
      {
      // prepare a second set of buffers in case a plugin is not
      // capable of inPlace processing

      bool swap = false;

      foreach (PluginI* p, *this) {
            if (p->on()) {
                  if (p->inPlaceCapable()) {
                        if (swap)
                              p->apply(nframes, ports, buffer, buffer);
                        else
                              p->apply(nframes, ports, buffer1, buffer1);
                        }
                  else {
                        if (swap)
                              p->apply(nframes, ports, buffer, buffer1);
                        else
                              p->apply(nframes, ports, buffer1, buffer);
                        swap = !swap;
                        }
                  }
            }
      if (swap) {
            for (int i = 0; i < ports; ++i)
                  AL::dsp->cpy(buffer1[i], buffer[i], nframes);
            }
      }

