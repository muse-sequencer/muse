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
#include "plugingui.h"
#include "al/xml.h"
#include "fastlog.h"
#include "ctrl.h"

PluginList plugins;

//---------------------------------------------------------
//   defaultValue
//---------------------------------------------------------

float ladspaDefaultValue(const LADSPA_Descriptor* plugin, int k)
      {
      LADSPA_PortRangeHint range = plugin->PortRangeHints[k];
      LADSPA_PortRangeHintDescriptor rh = range.HintDescriptor;
//      bool isLog = LADSPA_IS_HINT_LOGARITHMIC(rh);
      double val = 1.0;
      float m = (rh & LADSPA_HINT_SAMPLE_RATE) ? float(AL::sampleRate) : 1.0f;
      if (LADSPA_IS_HINT_DEFAULT_MINIMUM(rh)) {
            val = range.LowerBound * m;
            }
      else if (LADSPA_IS_HINT_DEFAULT_LOW(rh)) {
            if (LADSPA_IS_HINT_LOGARITHMIC(rh))
                  val = exp(fast_log10(range.LowerBound * m) * .75 +
                     log(range.UpperBound * m) * .25);
            else
                  val = range.LowerBound*.75*m + range.UpperBound*.25*m;
            }
      else if (LADSPA_IS_HINT_DEFAULT_MIDDLE(rh)) {
            if (LADSPA_IS_HINT_LOGARITHMIC(rh))
                  val = exp(log(range.LowerBound * m) * .5 +
                     log10(range.UpperBound * m) * .5);
            else
                  val = range.LowerBound*.5*m + range.UpperBound*.5*m;
            }
      else if (LADSPA_IS_HINT_DEFAULT_HIGH(rh)) {
            if (LADSPA_IS_HINT_LOGARITHMIC(rh))
                  val = exp(log(range.LowerBound * m) * .25 +
                     log(range.UpperBound * m) * .75);
            else
                  val = range.LowerBound*.25*m + range.UpperBound*.75*m;
            }
      else if (LADSPA_IS_HINT_DEFAULT_MAXIMUM(rh)) {
            val = range.UpperBound*m;
            }
      else if (LADSPA_IS_HINT_DEFAULT_0(rh))
            val = 0.0;
      else if (LADSPA_IS_HINT_DEFAULT_1(rh))
            val = 1.0;
      else if (LADSPA_IS_HINT_DEFAULT_100(rh))
            val = 100.0;
      else if (LADSPA_IS_HINT_DEFAULT_440(rh))
            val = 440.0;
      return val;
      }

//---------------------------------------------------------
//   Plugin
//---------------------------------------------------------

Plugin::Plugin(const QFileInfo* f)
   : fi(*f)
      {
      _instances = 0;
      }

//---------------------------------------------------------
//   LadpsaPlugin
//---------------------------------------------------------

LadspaPlugin::LadspaPlugin(const QFileInfo* f,
   const LADSPA_Descriptor_Function ldf,
   const LADSPA_Descriptor* d)
   : Plugin(f), ladspa(ldf), plugin(d)
      {
      _inports        = 0;
      _outports       = 0;
      _parameter      = 0;
      for (unsigned k = 0; k < plugin->PortCount; ++k) {
            LADSPA_PortDescriptor pd = d->PortDescriptors[k];
            static const int CI = LADSPA_PORT_CONTROL | LADSPA_PORT_INPUT;
            if ((pd &  CI) == CI) {
                  ++_parameter;
                  pIdx.push_back(k);
                  }
            else if (pd &  LADSPA_PORT_INPUT) {
                  ++_inports;
                  iIdx.push_back(k);
                  }
            else if (pd &  LADSPA_PORT_OUTPUT) {
                  ++_outports;
                  oIdx.push_back(k);
                  }
            }
      LADSPA_Properties properties = plugin->Properties;
      _inPlaceCapable = !LADSPA_IS_INPLACE_BROKEN(properties);
      if (_inports != _outports)
            _inPlaceCapable = false;
      }

//---------------------------------------------------------
//   instantiate
//---------------------------------------------------------

void* LadspaPlugin::instantiate()
      {
      return plugin->instantiate(plugin, AL::sampleRate);
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

      float* buffer2[ports];
      float data[nframes * ports];
      for (int i = 0; i < ports; ++i)
            buffer2[i] = data + i * nframes;

      bool swap = false;

      for (iPluginI ip = begin(); ip != end(); ++ip) {
            PluginI* p = *ip;
            if (p && p->on()) {
                  if (p->inPlaceCapable()) {
                        if (swap)
                              p->apply(nframes, ports, buffer2, buffer2);
                        else
                              p->apply(nframes, ports, buffer1, buffer1);
                        }
                  else {
                        if (swap)
                              p->apply(nframes, ports, buffer2, buffer1);
                        else
                              p->apply(nframes, ports, buffer1, buffer2);
                        swap = !swap;
                        }
                  }
            }
      if (swap) {
            for (int i = 0; i < ports; ++i)
                  memcpy(buffer1[i], buffer2[i], sizeof(float) * nframes);
            }
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
      if (_plugin)
            deactivate();
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
//   range
//---------------------------------------------------------

void LadspaPlugin::range(int i, float* min, float* max) const
      {
      i = pIdx[i];
      LADSPA_PortRangeHint range = plugin->PortRangeHints[i];
      LADSPA_PortRangeHintDescriptor desc = range.HintDescriptor;
      if (desc & LADSPA_HINT_TOGGLED) {
            *min = 0.0;
            *max = 1.0;
            return;
            }
      float m = (desc & LADSPA_HINT_SAMPLE_RATE) ? float(AL::sampleRate) : 1.0f;

      if (desc & LADSPA_HINT_BOUNDED_BELOW)
            *min =  range.LowerBound * m;
      else
            *min = 0.0;
      if (desc & LADSPA_HINT_BOUNDED_ABOVE)
            *max =  range.UpperBound * m;
      else
            *max = 1.0;
      }

//---------------------------------------------------------
//   createPIF
//---------------------------------------------------------

PluginIF* LadspaPlugin::createPIF(PluginI* pi)
      {
      LadspaPluginIF* pif = new LadspaPluginIF(pi);
      pif->init(pi->plugin());
      return pif;
      }

//---------------------------------------------------------
//   LadspaPluginIF
//---------------------------------------------------------

LadspaPluginIF::LadspaPluginIF(PluginI* pi)
   : PluginIF(pi)
      {
      descr  = 0;
      plugin = (LadspaPlugin*)(pi->plugin());
      }

//---------------------------------------------------------
//   init
//    return true on error
//---------------------------------------------------------

bool LadspaPluginIF::init(Plugin* pl)
      {
      handle = (LADSPA_Descriptor*) ((LadspaPlugin*)pl)->instantiate();
      plugin = (LadspaPlugin*)pl;
      descr  = plugin->ladspaDescriptor();

      int controlPorts = plugin->parameter();
      controls = new LadspaPort[controlPorts];

      for (int k = 0; k < controlPorts; ++k) {
            controls[k].val = plugin->defaultValue(k);
            descr->connect_port(handle, plugin->pIdx[k], &controls[k].val);
            }
      return handle == 0;
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
//   apply
//---------------------------------------------------------

void LadspaPluginIF::apply(unsigned nframes, float** src, float** dst)
      {
      int iports = plugin->inports();
      int oports = plugin->outports();
      int cports = plugin->parameter();

      //
      // update parameter
      //
      for (int i = 0; i < cports; ++i)
            controls[i].val = pluginI->controllerList[i]->curVal().f;
      //
      // set connections
      //
      for (int k = 0; k < iports; ++k)
            descr->connect_port(handle, plugin->iIdx[k], src[k]);
      for (int k = 0; k < oports; ++k)
            descr->connect_port(handle, plugin->oIdx[k], dst[k]);

      descr->run(handle, nframes);
      }

//---------------------------------------------------------
//   setChannel
//---------------------------------------------------------

void PluginI::setChannels(int c)
      {
      if (channel == c)
            return;
      int ni = c / _plugin->outports();
      if (ni == 0)
            ni = 1;
      channel = c;
      if (ni == instances)
            return;
      channel = c;

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
//   defaultValue
//---------------------------------------------------------

float LadspaPlugin::defaultValue(int k) const
      {
      k = pIdx[k];
      return ladspaDefaultValue(plugin, k);
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
      channel = c;
      _plugin = plug;
      _plugin->incInstances(1);
      QString inst("-" + QString::number(_plugin->instances()));
      _name  = _plugin->name() + inst;
      _label = _plugin->label() + inst;

      instances = channel / plug->outports();
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

void PluginI::writeConfiguration(Xml& xml)
      {
      writeConfiguration1(xml);
      xml.etag("plugin"); // append endtag
      }

//---------------------------------------------------------
//   saveConfiguration
//---------------------------------------------------------

void PluginI::writeConfiguration1(Xml& xml)
      {
      xml.tag("plugin file=\"%s\" label=\"%s\" channel=\"%d\"",
         _plugin->lib().toLatin1().data(), _plugin->label().toLatin1().data(), instances * _plugin->inports());
      if (_on == false)
            xml.intTag("on", _on);
      if (guiVisible()) {
            xml.intTag("gui", 1);
            xml.geometryTag("geometry", _gui);
            }
      if (hasNativeGui() && nativeGuiVisible())
            xml.intTag("nativeGui", 1);
      }

//---------------------------------------------------------
//   readConfiguration
//    return true on error
//---------------------------------------------------------

bool PluginI::readConfiguration(QDomNode node)
      {
      QDomElement e = node.toElement();
      QString file  = e.attribute("file");
      QString label = e.attribute("label");
      channel       = e.attribute("channel").toInt();

      if (_plugin == 0) {
            _plugin = plugins.find(file, label);
            if (_plugin == 0)
                  return true;
            if (initPluginInstance(_plugin, channel))
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

void PluginI::setParam(int idx, float val)
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

//---------------------------------------------------------
//   activate
//---------------------------------------------------------

void LadspaPluginIF::activate()
      {
      //
      // TODO: init values?
      //
      if (descr->activate)
            descr->activate(handle);
      }
#if 0
//---------------------------------------------------------
//   controllerId
//---------------------------------------------------------

int PluginI::controllerId(int idx) const
      {
      return controllerList[idx]->id();
      }
#endif

