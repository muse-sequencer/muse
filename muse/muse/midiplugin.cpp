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
#include "al/xml.h"
#include "al/tempo.h"
#include "al/sig.h"
#include "midiplugin.h"
#include "ctrl.h"
#include "midiplugins/libmidiplugin/mempi.h"
#include "audio.h"
#include "gconfig.h"

MidiPluginList midiPlugins;

MempiHost mempiHost;

//---------------------------------------------------------
//   division
//---------------------------------------------------------

int MempiHost::division() const
      {
      return config.division;
      }

//---------------------------------------------------------
//   tempo
//---------------------------------------------------------

int MempiHost::tempo(unsigned tick) const
      {
      return AL::tempomap.tempo(tick);
      }

//---------------------------------------------------------
//   tick2frame
//---------------------------------------------------------

unsigned MempiHost::tick2frame(unsigned tick) const
      {
      return AL::tempomap.tick2frame(tick);
      }

//---------------------------------------------------------
//   frame2tick
//---------------------------------------------------------

unsigned MempiHost::frame2tick(unsigned frame) const
      {
      return AL::tempomap.frame2tick(frame);
      }

//---------------------------------------------------------
//   bar
//---------------------------------------------------------

void MempiHost::bar(int t, int* bar, int* beat, unsigned* tick) const
      {
      AL::sigmap.tickValues(t, bar, beat, tick);
      }

//---------------------------------------------------------
//   bar2tick
//---------------------------------------------------------

unsigned MempiHost::bar2tick(int bar, int beat, int tick) const
      {
      return AL::sigmap.bar2tick(bar, beat, tick);
      }

//---------------------------------------------------------
//   MidiPlugin
//---------------------------------------------------------

MidiPlugin::MidiPlugin(const QFileInfo* f, const MEMPI_Function mf,
   const MEMPI* d) : fi(*f)
      {
      mempi = mf;
      plugin = d;
      _instances = 0;
      }

//---------------------------------------------------------
//   instantiate
//---------------------------------------------------------

MidiPluginI* MidiPlugin::instantiate(MidiTrackBase* t)
      {
      if (plugin == 0) {
            printf("initMidiPluginInstance: zero plugin\n");
            return 0;
            }
      ++_instances;
      QString inst("-" + QString::number(_instances));
      QString s  = name() + inst;

      Mempi* m = plugin->instantiate(s.toLatin1().data(), &mempiHost);
      if (m->init()) {
            delete m;
            return 0;
            }
      MidiPluginI* mp = new MidiPluginI(this, t, m);
      return mp;
      }

bool MidiPlugin::instantiate(MidiPluginI* mp)
      {
      if (plugin == 0) {
            printf("initMidiPluginInstance: zero plugin\n");
            return 0;
            }
      ++_instances;
      QString inst("-" + QString::number(_instances));
      QString s  = name() + inst;

      Mempi* m = plugin->instantiate(s.toLatin1().data(), &mempiHost);
      if (m->init()) {
            delete m;
            return true;
            }
      mp->setMempi(m);
      return false;
      }

//---------------------------------------------------------
//   MidiPluginI
//---------------------------------------------------------

MidiPluginI::MidiPluginI(MidiPlugin* p, MidiTrackBase* t, Mempi* m)
      {
      _track            = t;
      _plugin           = p;
      mempi             = m;
      _on               = true;
      }

//---------------------------------------------------------
//   MidiPluginI
//---------------------------------------------------------

MidiPluginI::MidiPluginI(MidiTrackBase* t)
      {
      _track            = t;
      _plugin           = 0;
      mempi             = 0;
      _on               = true;
      }

//---------------------------------------------------------
//   MidiPluginI
//---------------------------------------------------------

MidiPluginI::~MidiPluginI()
      {
      if (mempi)
            delete mempi;
      }

//---------------------------------------------------------
//   loadMidiPlugin
//---------------------------------------------------------

static void loadMidiPlugin(QFileInfo* fi)
      {
      if (debugMsg)
            printf("  load midi plugin <%s>\n", fi->filePath().toAscii().data());
      void* handle = dlopen(fi->filePath().toLocal8Bit().data(), RTLD_NOW);
      if (handle == 0) {
            fprintf(stderr, "loadMidiPlugin::dlopen(%s) failed: %s\n",
              fi->filePath().toLatin1().data(), dlerror());
            return;
            }
      MEMPI_Function mempi = (MEMPI_Function)dlsym(handle, "mempi_descriptor");

      if (!mempi) {
            const char *txt = dlerror();
            if (txt) {
                  fprintf(stderr,
                        "Unable to find mempi_descriptor() function in plugin "
                        "library file \"%s\": %s.\n"
                        "Are you sure this is a MEMPI plugin file?\n",
                        fi->filePath().toLatin1().data(),
                        txt);
                  return;
                  }
            }
      const MEMPI* descr = mempi();

      if (descr == 0) {
            fprintf(stderr, "Mempi::instantiate: no MEMPI descr found\n");
            return;
            }
      if (descr->majorMempiVersion != MEMPI_MAJOR_VERSION) {
            fprintf(stderr, "Mempi::instantiate: bad MEMPI version %d, expected %d\n",
               descr->majorMempiVersion, MEMPI_MAJOR_VERSION);
            return;
            }
      midiPlugins.push_back(new MidiPlugin(fi, mempi, descr));
      }

//---------------------------------------------------------
//   loadMidiPluginDir
//---------------------------------------------------------

static void loadMidiPluginDir(const QString& s)
      {
      if (debugMsg)
            printf("scan midi plugin dir <%s>\n", s.toLatin1().data());
#ifdef __APPLE__
      QDir pluginDir(s, QString("*.dylib"), 0, QDir::Files);
#else
      QDir pluginDir(s, QString("*.so"), 0, QDir::Files);
#endif
      if (pluginDir.exists()) {
            QFileInfoList list = pluginDir.entryInfoList();
            for (int i = 0; i < list.size(); ++i) {
                  QFileInfo fi = list.at(i);
                  loadMidiPlugin(&fi);
                  }
            }
      }

//---------------------------------------------------------
//   initMidiPlugins
//    search for midi MEPI plugins
//---------------------------------------------------------

void initMidiPlugins()
      {
      loadMidiPluginDir(museGlobalLib + QString("/midiplugins"));
      }

//---------------------------------------------------------
//   find
//---------------------------------------------------------

MidiPlugin* MidiPluginList::find(const QString& /*file*/, const QString& name)
      {
      for (iMidiPlugin i = begin(); i != end(); ++i) {
            if (name == (*i)->name())
                  return *i;
            }
      printf("MidiPlugin <%s> not found\n", name.toLatin1().data());
      return 0;
      }

//---------------------------------------------------------
//   Pipeline
//---------------------------------------------------------

MidiPipeline::MidiPipeline()
   : QList<MidiPluginI*>()
      {
      }

//---------------------------------------------------------
//   isOn
//---------------------------------------------------------

bool MidiPipeline::isOn(int idx) const
      {
      MidiPluginI* p = (*this)[idx];
      if (p)
            return p->on();
      return false;
      }

//---------------------------------------------------------
//   setOn
//---------------------------------------------------------

void MidiPipeline::setOn(int idx, bool flag)
      {
      MidiPluginI* p = (*this)[idx];
      if (p) {
            p->setOn(flag);
            }
      }

//---------------------------------------------------------
//   name
//---------------------------------------------------------

QString MidiPipeline::name(int idx) const
      {
      MidiPluginI* p = (*this)[idx];
      if (p)
            return p->name();
      return QString("empty");
      }

#if 0
//---------------------------------------------------------
//   empty
//---------------------------------------------------------

bool MidiPipeline::empty(int idx) const
      {
      MidiPluginI* p = (*this)[idx];
      return p == 0;
      }
#endif

//---------------------------------------------------------
//   move
//---------------------------------------------------------

void MidiPipeline::move(int idx, bool up)
      {
      MidiPluginI* p1 = (*this)[idx];
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

void MidiPipeline::showGui(int idx, bool flag)
      {
      MidiPluginI* p = (*this)[idx];
      if (p)
            p->showGui(flag);
      }

//---------------------------------------------------------
//   guiVisible
//---------------------------------------------------------

bool MidiPipeline::guiVisible(int idx) const
      {
      MidiPluginI* p = (*this)[idx];
      if (p)
            return p->guiVisible();
      return false;
      }

//---------------------------------------------------------
//   hasGui
//---------------------------------------------------------

bool MidiPipeline::hasGui(int idx) const
      {
      MidiPluginI* p = (*this)[idx];
      if (p)
            return p->hasGui();
      return false;
      }

//---------------------------------------------------------
//   apply
//---------------------------------------------------------

void MidiPipeline::apply(unsigned from, unsigned to, MidiEventList* il, MidiEventList* ool)
      {
      MidiEventList oList;
      MidiEventList* ol = &oList;

      bool swap = true;
      for (iMidiPluginI i = begin(); i != end(); ++i) {
            MidiPluginI* p = *i;
            if (p == 0 || !p->on())
                  continue;
            if (swap) {
                  ol->clear();
                  p->apply(from, to, il, ol);
                  }
            else {
                  il->clear();
                  p->apply(from, to, ol, il);
                  }
            swap = !swap;
            }
      MidiEventList* l = swap ? il : ol;
      for (iMidiEvent i = l->begin(); i != l->end(); ++i)
            ool->insert(*i);
      }

//---------------------------------------------------------
//   apply
//---------------------------------------------------------

void MidiPluginI::apply(unsigned from, unsigned to, MidiEventList* il, MidiEventList* ol)
      {
      mempi->process(from, to, il, ol);
      }

//---------------------------------------------------------
//   saveConfiguration
//---------------------------------------------------------

void MidiPluginI::writeConfiguration(Xml& xml)
      {
      xml.stag("midiPlugin file=\"%s\" name=\"%s\"",
         _plugin->lib().toLatin1().data(), _plugin->name().toLatin1().data());
      if (_on == false)
            xml.tag("on", _on);
      if (mempi->hasGui()) {
            xml.tag("guiVisible", mempi->guiVisible());
            int x, y, w, h;
            w = 0;
            h = 0;
            mempi->getGeometry(&x, &y, &w, &h);
            if (h || w)
                  xml.tag("geometry", QRect(x, y, w, h));
            }

      //---------------------------------------------
      // dump current state of plugin
      //---------------------------------------------

      int len = 0;
      const unsigned char* p;
      mempi->getInitData(&len, &p);
      if (len) {
            xml.stag("init len=\"%d\"", len);
            int col = 0;
            xml.putLevel();
            for (int i = 0; i < len; ++i, ++col) {
                  if (col >= 16) {
                        xml.put("");
                        col = 0;
                        xml.putLevel();
                        }
                  xml.nput("%02x ", p[i] & 0xff);
                  }
            if (col)
                  xml.put("");
            xml.etag("init");
            }
      xml.etag("midiPlugin");
      }

//---------------------------------------------------------
//   readConfiguration
//    return true on error
//---------------------------------------------------------

bool MidiPluginI::readConfiguration(QDomNode node)
      {
      QDomElement e = node.toElement();
      QString file  = e.attribute("file");
      QString label = e.attribute("name");

      if (_plugin == 0) {
            _plugin = midiPlugins.find(file, label);
            if (_plugin == 0)
                  return true;
            if (_plugin->instantiate(this))
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
            else if (tag == "guiVisible") {
                  showGui(i);
                  }
            else if (tag == "geometry") {
                  QRect r(AL::readGeometry(node));
                  mempi->setGeometry(r.x(), r.y(), r.width(), r.height());
                  }
            else if (tag == "init") {
                  int len = e.attribute("len","0").toInt();
                  if (len) {
                        const char* s = e.text().toLatin1().data();
                        unsigned char data[len];
                        unsigned char* d = data;
                        int numberBase = 16;
                        for (int i = 0; i < len; ++i) {
                              char* endp;
                              *d++ = strtol(s, &endp, numberBase);
                              s = endp;
                              if (s == 0)
                                    break;
                              }
                        mempi->setInitData(len, data);
                        }
                  }
            else
                  printf("MusE:MidiPluginI: unknown tag %s\n", e.tagName().toLatin1().data());
            node = node.nextSibling();
            }
      return false;
      }

//---------------------------------------------------------
//   showGui
//---------------------------------------------------------

void MidiPluginI::showGui()
      {
      mempi->showGui(!mempi->guiVisible());
      }

void MidiPluginI::showGui(bool flag)
      {
      mempi->showGui(flag);
      }

//---------------------------------------------------------
//   guiVisible
//---------------------------------------------------------

bool MidiPluginI::guiVisible() const
      {
      return mempi->guiVisible();
      }

//---------------------------------------------------------
//   MidiPluginDialog
//---------------------------------------------------------

MidiPluginDialog::MidiPluginDialog(QWidget* parent)
   : QDialog(parent)
      {
      resize(QSize(550, 300));
      setModal(true);
      setWindowTitle(tr("MusE: select midi plugin"));
      QVBoxLayout* layout = new QVBoxLayout(this);

      pList  = new QTreeWidget(this);
      pList->setIndentation(0);
      pList->setColumnCount(4);
      pList->setSortingEnabled(true);
      pList->setSelectionBehavior(QAbstractItemView::SelectRows);
      pList->setSelectionMode(QAbstractItemView::SingleSelection);
      pList->setAlternatingRowColors(true);

      QStringList headerLabels;

      headerLabels << tr("File") << tr("Name") << tr("Version") << tr("Description");
      pList->header()->resizeSection(0, 100);
      pList->header()->resizeSection(1, 120);
      pList->header()->resizeSection(2, 70);
      pList->header()->setResizeMode(3, QHeaderView::Stretch);
      pList->setHeaderLabels(headerLabels);

      fillPlugs();

      layout->addWidget(pList);

      //---------------------------------------------------
      //  Ok/Cancel Buttons
      //---------------------------------------------------

      QBoxLayout* w5 = new QHBoxLayout;
      layout->addLayout(w5);

      QPushButton* okB     = new QPushButton(tr("Ok"));
      QPushButton* cancelB = new QPushButton(tr("Cancel"));
      okB->setFixedWidth(80);
      cancelB->setFixedWidth(80);
      okB->setDefault(true);
      w5->addStretch(100);
      w5->addWidget(okB);
      w5->addWidget(cancelB);

      connect(pList,   SIGNAL(itemDoubleClicked(QTreeWidgetItem*,int)), SLOT(accept()));
      connect(cancelB, SIGNAL(clicked()), SLOT(reject()));
      connect(okB,     SIGNAL(clicked()), SLOT(accept()));
      }

//---------------------------------------------------------
//   getPlugin
//---------------------------------------------------------

MidiPlugin* MidiPluginDialog::getPlugin(QWidget* parent)
      {
      MidiPluginDialog* dialog = new MidiPluginDialog(parent);
      if (dialog->exec())
            return dialog->value();
      return 0;
      }

//---------------------------------------------------------
//   value
//---------------------------------------------------------

MidiPlugin* MidiPluginDialog::value()
      {
      QTreeWidgetItem* item = pList->selectedItems().at(0);
      if (item)
            return midiPlugins.find(item->text(0), item->text(1));
      return 0;
      }

//---------------------------------------------------------
//   fillPlugs
//---------------------------------------------------------

void MidiPluginDialog::fillPlugs()
      {
      pList->clear();
      for (iMidiPlugin i = midiPlugins.begin(); i != midiPlugins.end(); ++i) {
            if ((*i)->type() != MEMPI_FILTER)
                  continue;
            QTreeWidgetItem* item = new QTreeWidgetItem;
            item->setText(0, (*i)->lib());
            item->setText(1, (*i)->name());
            item->setText(2, (*i)->version());
            item->setText(3, (*i)->description());
            pList->addTopLevelItem(item);
            }
      }
