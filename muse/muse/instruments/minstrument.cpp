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
//   string2sysex
//---------------------------------------------------------

int string2sysex(const QString& s, unsigned char** data)
      {
      const char* src = s.toLatin1().data();
      char buffer[2048];
      char* dst = buffer;

      while (*src) {
            while (*src == ' ' || *src == '\n')
                  ++src;
            char* ep;
            long val = strtol(src, &ep, 16);
            if (ep == src) {
                  QMessageBox::information(0,
                     QString("MusE"),
                     QWidget::tr("Cannot convert sysex string"));
                  return 0;
                  }
            src    = ep;
            *dst++ = val;
            if (dst - buffer >= 2048) {
                  QMessageBox::information(0,
                     QString("MusE"),
                     QWidget::tr("Hex String too long (2048 bytes limit)"));
                  return 0;
                  }
            }
      int len = dst - buffer;
      unsigned char* b = new unsigned char[len+1];
      memcpy(b, buffer, len);
      b[len] = 0;
      *data = b;
      return len;
      }

//---------------------------------------------------------
//   sysex2string
//---------------------------------------------------------

QString sysex2string(int len, unsigned char* data)
      {
      QString d;
      QString s;
      for (int i = 0; i < len; ++i) {
            if ((i > 0) && ((i % 8)==0)) {
                  d += "\n";
                  }
            else if (i)
                  d += " ";
            d += s.sprintf("%02x", data[i]);
            }
      return d;
      }

//---------------------------------------------------------
//   Patch
//---------------------------------------------------------

Patch::Patch()
      {
      drumMap = 0;
      categorie = -1;
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
      if (debugMsg)
            printf("   load instrument definition <%s>\n", fi->filePath().toLocal8Bit().data());
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
                              bool replaced = false;
                              for (int idx = 0; idx < midiInstruments.size(); ++idx) {
                                    if (midiInstruments[idx]->iname() == i->iname()) {
                                          midiInstruments.replace(idx, i);
                                          replaced = true;
                                          if (debugMsg)
                                                printf("Midi Instrument Definition <%s> overwritten\n", 
                                                   i->iname().toLocal8Bit().data());
                                          break;
                                          }
                                    }
                              if (!replaced)
                                    midiInstruments += i;
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
      QString path2 = QDir::homePath() + "/" + config.instrumentPath;
      if (debugMsg)
            printf("load instrument definitions from <%s>\n", path2.toLatin1().data());
      QDir instrumentsDir2(path2, QString("*.idf"),
         QDir::SortFlags(QDir::Name | QDir::IgnoreCase), QDir::Files);
      if (instrumentsDir2.exists()) {
            QFileInfoList list = instrumentsDir2.entryInfoList();
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

void PatchGroup::read(QDomNode node, MidiInstrument* instrument)
      {
      for (; !node.isNull(); node = node.nextSibling()) {
            QDomElement e = node.toElement();
            QString tag(e.tagName());

            if (tag == "Patch") {
                  Patch* patch = new Patch;
                  patch->read(node, false, instrument);
                  patches.push_back(patch);
                  }
            else if (tag == "drummap") {
                  Patch* patch = new Patch;
                  patch->read(node, true, instrument);
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

void Patch::read(QDomNode node, bool dr, MidiInstrument* instrument)
      {
      QDomElement e = node.toElement();
      name  = e.attribute("name");
      typ   = e.attribute("mode", "-1").toInt();
      hbank = e.attribute("hbank", "-1").toInt();
      lbank = e.attribute("lbank", "-1").toInt();
      prog  = e.attribute("prog", "0").toInt();
      QString cat = e.attribute("cat");
      categorie = instrument->categories().indexOf(cat);
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
//   write
//---------------------------------------------------------

void Patch::write(Xml& xml)
      {
      if (drumMap == 0) {
            QString s = QString("Patch name=\"%1\"").arg(Xml::xmlString(name));
            if (typ != -1)
                  s += QString(" mode=\"%d\"").arg(typ);
            s += QString(" hbank=\"%1\" lbank=\"%2\" prog=\"%3\"").arg(hbank).arg(lbank).arg(prog);
            xml.tagE(s);
            return;
            }
      QString s = QString("drummap name=\"%1\"").arg(Xml::xmlString(name));
      s += QString(" hbank=\"%1\" lbank=\"%2\" prog=\"%3\"").arg(hbank).arg(lbank).arg(prog);
      xml.stag(s);
      for (int i = 0; i < DRUM_MAPSIZE; ++i) {
            DrumMapEntry* dm = drumMap->entry(i);
            dm->write(xml);
            }
      xml.etag("drummap");
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
                  patch->read(node, false, this);
                  if (pg.empty()) {
                        PatchGroup p;
                        p.patches.push_back(patch);
                        pg.push_back(p);
                        }
                  else
                        pg[0].patches.push_back(patch);
                  }
            else if (tag == "Category") {
                  QString name = e.attribute(QString("name"));
                  _categories.append(name);
                  }
            else if (tag == "drummap") {
                  Patch* patch = new Patch;
                  patch->read(node, true, this);
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
                  p.read(node.firstChild(), this);
                  pg.push_back(p);
                  }
            else if (tag == "Controller") {
                  MidiController* mc = new MidiController();
                  mc->read(node);
                  //
                  // HACK: make predefined "Program" controller overloadable
                  //
                  if (mc->name() == "Program") {
                        for (iMidiController i = _controller->begin(); i != _controller->end(); ++i) {
                              if ((*i)->name() == mc->name()) {
                                    _controller->erase(i);
                                    break;
                                    }
                              }
                        }
                  _controller->push_back(mc);
                  }
            else if (tag == "Init")
                  _midiInit->read(node.firstChild(), true);
            else if (tag == "SysEx") {
                  SysEx* se = new SysEx;
                  se->name = e.attribute("name");
                  for (QDomNode nnode = node.firstChild(); !nnode.isNull(); nnode = nnode.nextSibling()) {
                        e = nnode.toElement();
                        QString tag(e.tagName());
                        if (tag == "comment")
                              se->comment = e.text();
                        else if (tag == "data") {
                              se->dataLen = string2sysex(e.text(), &(se->data));
                              }
                        else
                              printf("MidiInstrument::read():SysEx: unknown tag %s\n", tag.toLatin1().data());
                        }
                  _sysex.append(se);
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

//---------------------------------------------------------
//   write
//---------------------------------------------------------

void MidiInstrument::write(Xml& xml)
      {
      xml.header();
      xml.stag("muse version=\"2.1\"");
      xml.stag(QString("MidiInstrument name=\"%1\"").arg(Xml::xmlString(iname())));

      foreach(const QString& s, _categories)
            xml.tagE(QString("Category name=\"%1\"").arg(Xml::xmlString(s)));

      std::vector<PatchGroup>* pg = groups();
      for (std::vector<PatchGroup>::iterator g = pg->begin(); g != pg->end(); ++g) {
            xml.stag(QString("PatchGroup name=\"%1\"").arg(Xml::xmlString(g->name)));
            for (iPatch p = g->patches.begin(); p != g->patches.end(); ++p)
                  (*p)->write(xml);
            xml.etag("PatchGroup");
            }
      for (iMidiController ic = _controller->begin(); ic != _controller->end(); ++ic)
            (*ic)->write(xml);
      xml.etag("MidiInstrument");
      xml.etag("muse");
      }

