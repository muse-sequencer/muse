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

#ifndef __MIDIPLUGIN_H__
#define __MIDIPLUGIN_H__

#include "globals.h"
#include "midiplugins/libmidiplugin/mempi.h"

namespace AL {
      class Xml;
      };
using AL::Xml;

class MidiPluginIF;
class MidiPluginGui;
class MidiPluginI;
class MidiTrackBase;
class Ctrl;
class MPEventList;
class MidiTrackBase;

typedef const MEMPI* (*MEMPI_Function)();

//---------------------------------------------------------
//   MidiPlugin
//---------------------------------------------------------

class MidiPlugin {
      MEMPI_Function mempi;
      const MEMPI* plugin;

   protected:
      int _instances;
      QFileInfo fi;

   public:
      MidiPlugin(const QFileInfo* f, const MEMPI_Function, const MEMPI* d);
      virtual ~MidiPlugin() {}

      int instances() const               { return _instances; }
      virtual void incInstances(int val)  { _instances += val; }

      QString lib() const                 { return fi.baseName();    }
      QString path() const                { return fi.absolutePath();     }
      QString name() const                { return QString(plugin->name); }
      QString description() const         { return QString(plugin->description); }
      QString version() const             { return QString(plugin->version); }
      MempiType type() const              { return plugin->type;   }
      MidiPluginI* instantiate(MidiTrackBase*);
      bool instantiate(MidiPluginI*);
      };

//---------------------------------------------------------
//   MidiPluginList
//---------------------------------------------------------

typedef std::list<MidiPlugin*>::iterator iMidiPlugin;

class MidiPluginList : public std::list<MidiPlugin*> {
   public:
      MidiPlugin* find(const QString&, const QString&);
      MidiPluginList() {}
      };

//---------------------------------------------------------
//   MidiPluginI
//    plugin instance
//---------------------------------------------------------

class MidiPluginI {
      MidiPlugin* _plugin;
      Mempi* mempi;

      MidiTrackBase* _track;
      bool _on;

   public:
      MidiPluginI(MidiPlugin*, MidiTrackBase*, Mempi*);
      MidiPluginI(MidiTrackBase*);
      ~MidiPluginI();

      void setMempi(Mempi* m)    { mempi = m; }

      MidiPlugin* plugin() const { return _plugin; }
      bool on() const            { return _on; }
      void setOn(bool val)       { _on = val; }
      void apply(unsigned, unsigned, MPEventList*, MPEventList*);

      QString name() const              { return QString(mempi->name()); }
      QString lib() const               { return _plugin->lib(); }
      MidiTrackBase* track() const      { return _track; }
      void getGeometry(int*x, int*y, int* w, int* h) const { mempi->getGeometry(x, y, w, h); }
      void setGeometry(int x, int y, int w, int h)   { mempi->setGeometry(x, y, w, h); }
      void writeConfiguration(Xml& xml);
      bool readConfiguration(QDomNode);

      void showGui();
      void showGui(bool);
      bool hasGui() const { return mempi->hasGui(); }
      void getInitData(int* len, const unsigned char** p) { mempi->getInitData(len, p); }
      void setInitData(int len, const unsigned char* p)   { mempi->setInitData(len, p); }
      bool guiVisible() const;
      };

//---------------------------------------------------------
//   Pipeline
//    chain of connected efx inserts
//---------------------------------------------------------

const int MidiPipelineDepth = 4;

class MidiPipeline : public std::vector<MidiPluginI*> {
   public:
      MidiPipeline();
      void insert(MidiPluginI* p, int index);
      bool isOn(int idx) const;
      void setOn(int, bool);
      QString name(int idx) const;
      void showGui(int, bool);
      bool guiVisible(int);
      void apply(unsigned, unsigned, MPEventList*, MPEventList*);
      void move(int idx, bool up);
      bool empty(int idx) const;
      MidiPluginI* plugin(int idx) { return (*this)[idx]; }
      };

typedef MidiPipeline::iterator iMidiPluginI;
typedef MidiPipeline::const_iterator ciMidiPluginI;

//---------------------------------------------------------
//   MidiPluginDialog
//---------------------------------------------------------

class MidiPluginDialog : public QDialog {
      QTreeWidget* pList;

      Q_OBJECT

   public:
      MidiPluginDialog(QWidget* parent=0);
      static MidiPlugin* getPlugin(QWidget* parent);
      MidiPlugin* value();

  public slots:
      void fillPlugs();
      };

extern void initMidiPlugins();
extern MidiPluginList midiPlugins;

#endif

