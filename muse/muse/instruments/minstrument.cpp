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

#include "minstrument.h"
#include "midioutport.h"
#include "globals.h"
#include "event.h"
#include "midievent.h"
#include "midictrl.h"
#include "gconfig.h"
#include "midiedit/drummap.h"

MidiInstrumentList midiInstruments;
MidiInstrument* genericMidiInstrument;

//---------------------------------------------------------
//   Patch
//---------------------------------------------------------

Patch::Patch()
      {
      drumMap = 0;
      }

Patch::~Patch()
      {
      if (drumMap)
            delete drumMap;
      }

//---------------------------------------------------------
//   loadIDF
//---------------------------------------------------------

static void loadIDF(QFileInfo* fi)
      {
      QFile qf(fi->filePath());
      if (!qf.open(QIODevice::ReadOnly)) {
            printf("cannot open file %s\n", fi->fileName().toLatin1().data());
            return;
            }
      QDomDocument doc;
      int line, column;
      QString err;
      if (!doc.setContent(&qf, false, &err, &line, &column)) {
            QString col, ln, error;
            col.setNum(column);
            ln.setNum(line);
            error = err + " at line: " + ln + " col: " + col;
            printf("error reading file <%s>:\n   %s\n",
               fi->filePath().toLatin1().data(), error.toLatin1().data());
            return;
            }
      QDomNode node = doc.documentElement();
      while (!node.isNull()) {
            QDomElement e = node.toElement();
            if (e.isNull())
                  continue;
            if (e.tagName() == "muse") {
                  QString version = e.attribute(QString("version"));
                  for (QDomNode n = node.firstChild(); !n.isNull(); n = n.nextSibling()) {
                        QDomElement e = n.toElement();
                        if (e.tagName() == "MidiInstrument") {
                              MidiInstrument* i = new MidiInstrument();
                              i->read(n);
                              i->setFilePath(fi->filePath());
                              midiInstruments.push_back(i);
                              }
                        }
                  }
            else
                  printf("MusE:laodIDF: %s not supported\n", e.tagName().toLatin1().data());
            node = node.nextSibling();
            }
      qf.close();
      }

//---------------------------------------------------------
//   initMidiInstruments
//---------------------------------------------------------

void initMidiInstruments()
      {
      genericMidiInstrument = new MidiInstrument(QWidget::tr("generic midi"));

      midiInstruments.push_back(genericMidiInstrument);
      QString museGlobalInstruments(museGlobalShare
         + QString("/instruments"));
      if (debugMsg)
            printf("load instrument definitions from <%s>\n", museGlobalInstruments.toLatin1().data());
      QDir instrumentsDir(museGlobalInstruments, QString("*.idf"),
         QDir::SortFlags(QDir::Name | QDir::IgnoreCase), QDir::Files);
      if (instrumentsDir.exists()) {
            QFileInfoList list = instrumentsDir.entryInfoList();
            int n = list.size();
            for (int i = 0; i < n; ++i) {
                  QFileInfo fi = list.at(i);
                  loadIDF(&fi);
                  }
            }
      }

//---------------------------------------------------------
//   registerMidiInstrument
//---------------------------------------------------------

MidiInstrument* registerMidiInstrument(const QString& name)
      {
      for (iMidiInstrument i = midiInstruments.begin();
         i != midiInstruments.end(); ++i) {
            if ((*i)->iname() == name)
                  return *i;
            }
      return genericMidiInstrument;
      }

//---------------------------------------------------------
//   removeMidiInstrument
//---------------------------------------------------------

void removeMidiInstrument(const QString& name)
      {
      for (iMidiInstrument i = midiInstruments.begin();
         i != midiInstruments.end(); ++i) {
            if ((*i)->iname() == name) {
                  midiInstruments.erase(i);
                  return;
                  }
            }
      }

void removeMidiInstrument(const MidiInstrument* instr)
      {
      for (iMidiInstrument i = midiInstruments.begin();
         i != midiInstruments.end(); ++i) {
            if (*i == instr) {
                  midiInstruments.erase(i);
                  return;
                  }
            }
      }

//---------------------------------------------------------
//   MidiInstrument
//---------------------------------------------------------

void MidiInstrument::init()
      {
      _initScript = 0;
      _midiInit  = new EventList();
      _midiReset = new EventList();
      _midiState = new EventList();
      _controller = new MidiControllerList;

      // add some default controller to controller list
      // this controllers are always available for all instruments
      //
      MidiController* prog = new MidiController("Program", CTRL_PROGRAM, 0, 0x7fffff, 0);
      _controller->push_back(prog);
      _dirty = false;
      _readonly = false;
      }

MidiInstrument::MidiInstrument()
      {
      init();
      }

//---------------------------------------------------------
//   MidiInstrument
//---------------------------------------------------------

MidiInstrument::MidiInstrument(const QString& txt)
      {
      _name = txt;
      init();
      }

//---------------------------------------------------------
//   MidiInstrument
//---------------------------------------------------------

MidiInstrument::~MidiInstrument()
      {
      delete _midiInit;
      delete _midiReset;
      delete _midiState;
      delete _controller;
      if (_initScript)
            delete _initScript;
      }

//---------------------------------------------------------
//   readPatchGroup
//---------------------------------------------------------

void PatchGroup::read(QDomNode node)
      {
      for (; !node.isNull(); node = node.nextSibling()) {
            QDomElement e = node.toElement();
            QString tag(e.tagName());

            if (tag == "Patch") {
                  Patch* patch = new Patch;
                  patch->read(node, false);
                  patches.push_back(patch);
                  }
            else if (tag == "drummap") {
                  Patch* patch = new Patch;
                  patch->read(node, true);
                  patches.push_back(patch);
                  }
            else if (!tag.isEmpty())
                  printf("MusE:PatchGroup(): unknown tag %s in group %s\n",
                     e.tagName().toLatin1().data(), name.toLatin1().data());
            }
      }

//---------------------------------------------------------
//   read
//---------------------------------------------------------

void Patch::read(QDomNode node, bool dr)
      {
      QDomElement e = node.toElement();
      name  = e.attribute(QString("name"));
      typ   = e.attribute(QString("mode"), "-1").toInt();
      hbank = e.attribute(QString("hbank"), "-1").toInt();
      lbank = e.attribute(QString("lbank"), "-1").toInt();
      prog  = e.attribute(QString("prog"), "0").toInt();
      drumMap = 0;
      if (!dr)
            return;
      drumMap = new DrumMap(name);
      int idx = 0;
      for (node = node.firstChild(); !node.isNull(); node = node.nextSibling()) {
            e = node.toElement();
            QString tag(e.tagName());
            if (tag == "entry") {
                  DrumMapEntry* de = drumMap->entry(idx);
                  de->read(node);
                  ++idx;
                  }
            else if (!tag.isEmpty()) {
                  printf("Patch: read drummap: unknown tag %s\n", tag.toLatin1().data());
                  }
            }
      drumMap->init();
      }

//---------------------------------------------------------
//   readMidiState
//---------------------------------------------------------

void MidiInstrument::readMidiState(QDomNode node)
      {
      _midiState->read(node, true);
      }

//---------------------------------------------------------
//   read
//---------------------------------------------------------

void MidiInstrument::read(QDomNode node)
      {
      QDomElement e = node.toElement();
      _name = e.attribute("name");

      for (node = node.firstChild(); !node.isNull(); node = node.nextSibling()) {
            e = node.toElement();
            QString tag(e.tagName());
            if (tag == "Patch") {
                  Patch* patch = new Patch;
                  patch->read(node, false);
                  if (pg.empty()) {
                        PatchGroup p;
                        p.patches.push_back(patch);
                        pg.push_back(p);
                        }
                  else
                        pg[0].patches.push_back(patch);
                  }
            else if (tag == "Category")
                  ;     // TODO "Category"
            else if (tag == "drummap") {
                  Patch* patch = new Patch;
                  patch->read(node, true);
                  if (pg.empty()) {
                        PatchGroup p;
                        p.patches.push_back(patch);
                        pg.push_back(p);
                        }
                  else
                        pg[0].patches.push_back(patch);
                  }
            else if (tag == "PatchGroup") {
                  PatchGroup p;
                  p.name = e.attribute("name");
                  p.read(node.firstChild());
                  pg.push_back(p);
                  }
            else if (tag == "Controller") {
                  MidiController* mc = new MidiController();
                  mc->read(node);
                  _controller->push_back(mc);
                  }
            else if (tag == "Init")
                  _midiInit->read(node.firstChild(), true);
            else if (tag == "SysEx") {
                  SysEx se;
                  se.name = e.attribute("name");
                  for (QDomNode nnode = node.firstChild(); !nnode.isNull(); nnode = nnode.nextSibling()) {
                        e = nnode.toElement();
                        QString tag(e.tagName());
                        if (tag == "comment")
                              se.comment = e.text();
                        else if (tag == "data")
                              se.data = e.text();
                        else
                              printf("MidiInstrument::read():SysEx: unknown tag %s\n", tag.toLatin1().data());
                        }
                  sysex.push_back(se);
                  }
            else if (!tag.isEmpty()) {
                  printf("MidiInstrument::read(): unknown tag %s\n", tag.toLatin1().data());
                  }
            }
      }

//---------------------------------------------------------
//   getPatchName
//---------------------------------------------------------

QString MidiInstrument::getPatchName(int /*channel*/, int prog)
      {
      int hbank = (prog >> 16) & 0xff;
      int lbank = (prog >> 8) & 0xff;
      prog &= 0xff;
      if (prog == 0xff)
            return "---";

      int tmask = 1;
      bool hb   = hbank == 0xff;
      bool lb   = lbank == 0xff;

      for (std::vector<PatchGroup>::iterator i = pg.begin(); i != pg.end(); ++i) {
            PatchList& pl = i->patches;
            for (PatchList::const_iterator ipl = pl.begin(); ipl != pl.end(); ++ipl) {
                  const Patch* mp = *ipl;
                  if ((mp->typ & tmask)
                    && (prog == mp->prog)
                    && (hbank == mp->hbank || hb || mp->hbank == -1)
                    && (lbank == mp->lbank || lb || mp->lbank == -1))
                        return mp->name;
                  }
            }
      return QString("---");
      }

//---------------------------------------------------------
//   getDrumMap
//---------------------------------------------------------

DrumMap* MidiInstrument::getDrumMap(int prog)
      {
      int hbank = (prog >> 16) & 0xff;
      int lbank = (prog >> 8) & 0xff;
      prog &= 0xff;
      if (prog == 0xff)
            return 0;

      int tmask = 1;
      bool hb   = hbank == 0xff;
      bool lb   = lbank == 0xff;

      for (std::vector<PatchGroup>::iterator i = pg.begin(); i != pg.end(); ++i) {
            PatchList& pl = i->patches;
            for (PatchList::const_iterator ipl = pl.begin(); ipl != pl.end(); ++ipl) {
                  const Patch* mp = *ipl;
                  if ((mp->typ & tmask)
                    && (prog == mp->prog)
                    && (hbank == mp->hbank || hb || mp->hbank == -1)
                    && (lbank == mp->lbank || lb || mp->lbank == -1)) {
                        return mp->drumMap;
                        }
                  }
            }
      return 0;
      }

//---------------------------------------------------------
//   populatePatchPopup
//---------------------------------------------------------

void MidiInstrument::populatePatchPopup(QMenu* menu, int)
      {
      menu->clear();
      int mask = 7;

      if (pg.size() > 1) {
            for (std::vector<PatchGroup>::iterator i = pg.begin(); i != pg.end(); ++i) {
                  QMenu* pm = menu->addMenu(i->name);
                  pm->setFont(config.fonts[0]);
                  PatchList& pl = i->patches;
                  for (PatchList::const_iterator ipl = pl.begin(); ipl != pl.end(); ++ipl) {
                        const Patch* mp = *ipl;
                        if (mp->typ & mask) {
                              int id = ((mp->hbank & 0xff) << 16)
                                         + ((mp->lbank & 0xff) << 8) + (mp->prog & 0xff);
                              QAction* a = pm->addAction(mp->name);
                              a->setData(id);
                              }
                        }
                  }
            }
      else if (pg.size() == 1 ){
            // no groups
            PatchList& pl = pg.front().patches;
            for (PatchList::const_iterator ipl = pl.begin(); ipl != pl.end(); ++ipl) {
                  const Patch* mp = *ipl;
                  if (mp->typ & mask) {
                        int id = ((mp->hbank & 0xff) << 16)
                                 + ((mp->lbank & 0xff) << 8) + (mp->prog & 0xff);
                        QAction* a = menu->addAction(mp->name);
                        a->setData(id);
                        }
                  }
            }
      }

//---------------------------------------------------------
//   getMidiController
//---------------------------------------------------------

MidiController* MidiInstrument::midiController(int num) const
      {
      for (iMidiController i = _controller->begin(); i != _controller->end(); ++i) {
            int cn = (*i)->num();
            if (cn == num)
                  return *i;
            // wildcard?
            if (((cn & 0xff) == 0xff) && ((cn & ~0xff) == (num & ~0xff)))
                  return *i;
            }
      for (iMidiController i = defaultMidiController.begin(); i != defaultMidiController.end(); ++i) {
            int cn = (*i)->num();
            if (cn == num)
                  return *i;
            // wildcard?
            if (((cn & 0xff) == 0xff) && ((cn & ~0xff) == (num & ~0xff)))
                  return *i;
            }
      QString name = midiCtrlName(num);
      int min = 0;
      int max = 127;
      MidiController* c = new MidiController(name, num, min, max, 0);
      defaultMidiController.push_back(c);
      return c;
      }


