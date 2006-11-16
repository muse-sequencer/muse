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

#include "miditrackbase.h"
#include "midiplugin.h"

//---------------------------------------------------------
//   MidiTrackBase
//---------------------------------------------------------

MidiTrackBase::MidiTrackBase()
   : Track()
      {
      _pipeline = new MidiPipeline();
      }

//---------------------------------------------------------
//   MidiTrackBase
//---------------------------------------------------------

MidiTrackBase::~MidiTrackBase()
      {
      foreach(MidiPluginI* plugin, *_pipeline)
            delete plugin;
      delete _pipeline;
      }

//---------------------------------------------------------
//   MidiTrackBase::writeProperties
//---------------------------------------------------------

void MidiTrackBase::writeProperties(Xml& xml) const
      {
      Track::writeProperties(xml);
      for (ciMidiPluginI ip = _pipeline->begin(); ip != _pipeline->end(); ++ip) {
            if (*ip)
                  (*ip)->writeConfiguration(xml);
            }
      }

//---------------------------------------------------------
//   MidiTrackBase::readProperties
//---------------------------------------------------------

bool MidiTrackBase::readProperties(QDomNode node)
      {
      QDomElement e = node.toElement();
      QString tag(e.tagName());
      if (tag == "midiPlugin") {
            MidiPluginI* pi = new MidiPluginI(this);
            if (pi->readConfiguration(node))
                  delete pi;
            else
                  addPlugin(pi, -1);
            }
      else
            return Track::readProperties(node);
      return false;
      }

//---------------------------------------------------------
//   plugin
//---------------------------------------------------------

MidiPluginI* MidiTrackBase::plugin(int idx) const
      {
      return _pipeline->value(idx);
      }

//---------------------------------------------------------
//   addPlugin
//    idx    = -1     append
//    plugin = 0   remove slot
//---------------------------------------------------------

void MidiTrackBase::addPlugin(MidiPluginI* plugin, int idx)
      {
      if (plugin == 0) {
#if 0
            MidiPluginI* oldPlugin = (*_pipeline)[idx];
            if (oldPlugin) {
                  int controller = oldPlugin->plugin()->parameter();
                  for (int i = 0; i < controller; ++i) {
                        int id = (idx + 1) * 0x1000 + i;
                        removeController(id);
                        }
                  }
#endif
            }
      if (idx == -1)
            idx = _pipeline->size();

      if (plugin) {
            _pipeline->insert(idx, plugin);
#if 0
            int ncontroller = plugin->plugin()->parameter();
            for (int i = 0; i < ncontroller; ++i) {
                  int id = (idx + 1) * 0x1000 + i;
                  QString name(plugin->getParameterName(i));
                  double min, max;
                  plugin->range(i, &min, &max);
                  Ctrl* cl = getController(id);
                  if (cl == 0) {
                        cl = new Ctrl(id, name);
                        cl->setRange(min, max);
                        double defaultValue = plugin->defaultValue(i);
                        cl->setDefault(defaultValue);
                        cl->setCurVal(defaultValue);
                        addController(cl);
                        }
                  plugin->setParam(i, cl->schedVal().f);
                  plugin->setControllerList(cl);
                  }
#endif
            }
      else {
            _pipeline->removeAt(idx);
            }
      }



