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

#include <dlfcn.h>

#include "al/al.h"
#include "plugin.h"
#include "ladspaplugin.h"
#include "auxplugin.h"
#include "plugingui.h"
#include "al/xml.h"
#include "fastlog.h"
#include "ctrl.h"

PluginList plugins;

//---------------------------------------------------------
//   Plugin
//---------------------------------------------------------

Plugin::Plugin(const QFileInfo* f)
   : fi(*f)
      {
      _instances = 0;
      }

//---------------------------------------------------------
//   loadPluginLib
//---------------------------------------------------------

static void loadPluginLib(QFileInfo* fi)
      {
      void* handle = dlopen(fi->filePath().toLatin1().data(), RTLD_NOW);
      if (handle == 0) {
            fprintf(stderr, "dlopen(%s) failed: %s\n",
              fi->filePath().toLatin1().data(), dlerror());
            return;
            }
      LADSPA_Descriptor_Function ladspa = (LADSPA_Descriptor_Function)dlsym(handle, "ladspa_descriptor");

      if (!ladspa) {
            const char *txt = dlerror();
            if (txt) {
                  fprintf(stderr,
                        "Unable to find ladspa_descriptor() function in plugin "
                        "library file \"%s\": %s.\n"
                        "Are you sure this is a LADSPA plugin file?\n",
                        fi->filePath().toLatin1().data(),
                        txt);
                  return;
                  }
            }
      const LADSPA_Descriptor* descr;
      for (int i = 0;; ++i) {
            descr = ladspa(i);
            if (descr == NULL)
                  break;
            plugins.push_back(new LadspaPlugin(fi, ladspa, descr));
            }
      }

//---------------------------------------------------------
//   loadPluginDir
//---------------------------------------------------------

static void loadPluginDir(const QString& s)
      {
      if (debugMsg)
            printf("scan ladspa plugin dir <%s>\n", s.toLatin1().data());
#ifdef __APPLE__      
      QDir pluginDir(s, QString("*.dylib"), 0, QDir::Files);
#else
      QDir pluginDir(s, QString("*.so"), 0, QDir::Files);
#endif
      if (pluginDir.exists()) {
            QFileInfoList list = pluginDir.entryInfoList();
            for (int i = 0; i < list.size(); ++i) {
                  QFileInfo fi = list.at(i);
                  loadPluginLib(&fi);
                  }
            }
      }

//---------------------------------------------------------
//   initPlugins
//    search for LADSPA plugins
//---------------------------------------------------------

void initPlugins()
      {
      loadPluginDir(museGlobalLib + QString("/plugins"));

      char* ladspaPath = getenv("LADSPA_PATH");
      if (ladspaPath == 0)
            ladspaPath = "/usr/lib/ladspa:/usr/local/lib/ladspa";

      char* p = ladspaPath;
      while (*p != '\0') {
            char* pe = p;
            while (*pe != ':' && *pe != '\0')
                  pe++;

            int n = pe - p;
            if (n) {
                  char* buffer = new char[n + 1];
                  strncpy(buffer, p, n);
                  buffer[n] = '\0';
                  loadPluginDir(QString(buffer));
                  delete[] buffer;
                  }
            p = pe;
            if (*p == ':')
                  p++;
            }
      auxPlugin = new AuxPlugin;
      }

//---------------------------------------------------------
//   find
//---------------------------------------------------------

Plugin* PluginList::find(const QString& file, const QString& name)
      {
      for (iPlugin i = begin(); i != end(); ++i) {
            if ((file == (*i)->lib()) && (name == (*i)->label()))
                  return *i;
            }
      printf("MusE: Plugin <%s> not found\n", name.toAscii().data());
      return 0;
      }

//---------------------------------------------------------
//   PluginI
//---------------------------------------------------------

PluginI::PluginI(AudioTrack* t)
      {
      _track            = t;
      _plugin           = 0;
      instances         = 0;
      _gui              = 0;
      _on               = true;
      pif               = 0;
      initControlValues = false;
      }

//---------------------------------------------------------
//   PluginI
//---------------------------------------------------------

PluginI::~PluginI()
      {
      if (_plugin) {
            deactivate();
            _plugin->incInstances(-1);
            }
      if (_gui)
            delete _gui;
      if (pif) {
            for (int i = 0; i < instances; ++i) {
                  delete pif[i];
                  }
            delete[] pif;
            }
      }

//---------------------------------------------------------
//   apply
//---------------------------------------------------------

void PluginI::apply(unsigned nframes, int ports, float** src, float** dst)
      {
      int oports = _plugin->outports();
      int iports = _plugin->inports();

      float* sp[iports * instances];
      float* dp[oports * instances];

      for (int i = 0; i < iports * instances; ++i)
            sp[i] = src[i % ports];
      for (int i = 0; i < oports * instances; ++i)
            dp[i] = dst[i % ports];

      float** spp = sp;
      float** dpp = dp;
      for (int i = 0; i < instances; ++i) {
            pif[i]->apply(nframes, spp, dpp);
            spp += iports;
            dpp += oports;
            }
      }

//---------------------------------------------------------
//   setChannel
//---------------------------------------------------------

void PluginI::setChannels(int c)
      {
      if (_channel == c)
            return;
      int ni = c / _plugin->outports();
      if (ni == 0)
            ni = 1;
      _channel = c;
      if (ni == instances)
            return;
      _channel = c;

      // remove old instances:
      deactivate();
      for (int i = 0; i < instances; ++i)
            delete pif[i];
      delete pif;

      instances = ni;
      pif = new PluginIF*[instances];
      for (int i = 0; i < instances; ++i) {
            pif[i] = _plugin->createPIF(this);
            if (pif[i] == 0)
                  return;
            }
      activate();
      }

//---------------------------------------------------------
//   initPluginInstance
//    return true on error
//---------------------------------------------------------

bool PluginI::initPluginInstance(Plugin* plug, int c)
      {
      if (plug == 0) {
            printf("initPluginInstance: zero plugin\n");
            return true;
            }
      _channel = c;
      _plugin = plug;
      _plugin->incInstances(1);
      QString inst("-" + QString::number(_plugin->instances()));
      _name  = _plugin->name() + inst;
      _label = _plugin->label() + inst;

      instances = _channel / plug->outports();
      if (instances < 1)
            instances = 1;
      pif = new PluginIF*[instances];
      for (int i = 0; i < instances; ++i) {
            pif[i] = _plugin->createPIF(this);
            if (pif[i] == 0)
                  return true;
            }
      activate();
      return false;
      }

//---------------------------------------------------------
//   setParameter
//    set plugin instance controller value by name
//    return true on error
//---------------------------------------------------------

bool PluginI::setParameter(const QString& s, double val)
      {
      if (_plugin == 0)
            return true;
      int n = _plugin->parameter();
      for (int i = 0; i < n; ++i) {
            if (getParameterName(i) == s) {
                  setParam(i, val);
                  return false;
                  }
            }
      printf("PluginI:setControl(%s, %f) controller not found\n",
         s.toLatin1().data(), val);
      return true;
      }

//---------------------------------------------------------
//   saveConfiguration
//---------------------------------------------------------

void PluginI::writeConfiguration(Xml& xml, bool prefader)
      {
      writeConfiguration1(xml, prefader);
      xml.etag("plugin"); // append endtag
      }

//---------------------------------------------------------
//   saveConfiguration
//---------------------------------------------------------

void PluginI::writeConfiguration1(Xml& xml, bool prefader)
      {
      xml.stag(QString("plugin pre=\"%1\" file=\"%2\" label=\"%3\" channel=\"%4\"")
         .arg(prefader)
         .arg(_plugin->lib())
         .arg(_plugin->label())
         .arg(_channel));
//         instances * _plugin->inports());
      if (_on == false)
            xml.tag("on", _on);
      if (guiVisible()) {
            xml.tag("gui", 1);
            xml.tag("geometry", _gui);
            }
      if (hasNativeGui() && nativeGuiVisible())
            xml.tag("nativeGui", 1);
      }

//---------------------------------------------------------
//   readConfiguration
//    return true on error
//---------------------------------------------------------

bool PluginI::readConfiguration(QDomNode node, bool* prefader)
      {
      QDomElement e = node.toElement();
      QString file  = e.attribute("file");
      QString label = e.attribute("label");
      _channel      = e.attribute("channel").toInt();
      *prefader     = e.attribute("pre", "1").toInt();

      if (_plugin == 0) {
            // special case: internal plugin Aux
            if (file.isEmpty() && label == "Aux")
                  _plugin = auxPlugin;
            else
                  _plugin = plugins.find(file, label);
            if (_plugin == 0)
                  return true;
            if (initPluginInstance(_plugin, _channel))
                  return true;
            }
      node = node.firstChild();
      while (!node.isNull()) {
            e = node.toElement();
            int i = e.text().toInt();
            QString tag(e.tagName());
            if (tag == "on") {
                  bool flag = i;
                  _on = flag;
                  }
            else if (tag == "gui") {
                  bool flag = i;
                  showGui(flag);
                  }
            else if (tag == "nativeGui") {
                  bool flag = i;
                  showNativeGui(flag);
                  }
            else if (tag == "geometry") {
                  QRect r(AL::readGeometry(node));
                  if (_gui) {
                        _gui->resize(r.size());
                        _gui->move(r.topLeft());
                        }
                  }
            else
                  printf("MusE:PluginI: unknown tag %s\n", e.tagName().toLatin1().data());
            node = node.nextSibling();
            }
      if (_gui)
            _gui->updateValues();
      return false;
      }

//---------------------------------------------------------
//   showGui
//---------------------------------------------------------

void PluginI::showGui()
      {
      if (_plugin) {
          if (_gui == 0)
                makeGui();
          if (_gui->isVisible())
                _gui->hide();
          else
                _gui->show();
          }
      }

void PluginI::showGui(bool flag)
      {
      if (_plugin) {
          if (flag) {
                if (_gui == 0)
                      makeGui();
                _gui->show();
                }
          else {
                if (_gui)
                      _gui->hide();
                }
          }
      }

//---------------------------------------------------------
//   guiVisible
//---------------------------------------------------------

bool PluginI::guiVisible() const
      {
      return _gui && _gui->isVisible();
      }

//---------------------------------------------------------
//   makeGui
//---------------------------------------------------------

void PluginI::makeGui()
      {
      _gui = new PluginGui(this);
      }

//---------------------------------------------------------
//   deactivate
//---------------------------------------------------------

void PluginI::deactivate()
      {
      for (int i = 0; i < instances; ++i) {
            pif[i]->deactivate();
            pif[i]->cleanup();
            }
      }

//---------------------------------------------------------
//   setParam
//---------------------------------------------------------

void PluginI::setParam(int idx, double val)
      {
      if (_gui)
            _gui->updateValue(idx, val);
      for (int i = 0; i < instances; ++i)
            pif[i]->setParam(idx, val);
      }

//---------------------------------------------------------
//   activate
//---------------------------------------------------------

void PluginI::activate()
      {
      for (int i = 0; i < instances; ++i)
            pif[i]->activate();
      }

