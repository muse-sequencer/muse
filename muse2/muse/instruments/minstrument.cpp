//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: minstrument.cpp,v 1.10.2.5 2009/03/28 01:46:10 terminator356 Exp $
//
//  (C) Copyright 2000-2003 Werner Schweer (ws@seh.de)
//
//  This program is free software; you can redistribute it and/or
//  modify it under the terms of the GNU General Public License
//  as published by the Free Software Foundation; version 2 of
//  the License, or (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
//
//=========================================================

#include <stdio.h>
#include <string.h>

#include <QAction>
#include <QDir>
#include <QFileInfo>
#include <QString>

#include "minstrument.h"
#include "midiport.h"
#include "mididev.h"
#include "audio.h"
#include "midi.h"
#include "globals.h"
#include "xml.h"
#include "event.h"
#include "mpevent.h"
#include "midictrl.h"
#include "gconfig.h"
#include "popupmenu.h"
#include "drummap.h"
#include "helper.h"

namespace MusECore {

MidiInstrumentList midiInstruments;
MidiInstrument* genericMidiInstrument;

//---------------------------------------------------------
//   string2sysex
//   Return -1 if cannot be converted.
//---------------------------------------------------------

int string2sysex(const QString& s, unsigned char** data)
      {
      const char* src = s.toLatin1().constData();
      char buffer[2048];
      char* dst = buffer;

      if(src) {
        while (*src) {
          while (*src == ' ' || *src == '\n') {
            ++src;
          }
          if(!(*src))
            break;
          char* ep;
          long val = strtol(src, &ep, 16);
          if (ep == src) {
            printf("string2sysex: Cannot convert string to sysex %s\n", src);
            return -1;
          }
          src    = ep;
          *dst++ = val;
          if (dst - buffer >= 2048) {
            printf("string2sysex: Hex String too long (2048 bytes limit)\n");
            return -1;
          }
        }
      }
      int len = dst - buffer;
      if(len > 0)
      {
        unsigned char* b = new unsigned char[len];
        memcpy(b, buffer, len);
        *data = b;
      }
      else
        *data = 0;

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
//   readEventList
//---------------------------------------------------------

static void readEventList(Xml& xml, EventList* el, const char* name)
      {
      for (;;) {
            Xml::Token token = xml.parse();
            const QString& tag = xml.s1();
            switch (token) {
                  case Xml::Error:
                  case Xml::End:
                        return;
                  case Xml::TagStart:
                        if (tag == "event") {
                              Event e(Note);
                              e.read(xml);
                              el->add(e);
                              }
                        else
                              xml.unknown("readEventList");
                        break;
                  case Xml::TagEnd:
                        if (tag == name)
                              return;
                  default:
                        break;
                  }
            }
      }

//---------------------------------------------------------
//   read
//---------------------------------------------------------

//---------------------------------------------------------
//   loadIDF
//---------------------------------------------------------

static void loadIDF(QFileInfo* fi)
      {
      FILE* f = fopen(fi->filePath().toLatin1().constData(), "r");
      if (f == 0)
            return;
      if (MusEGlobal::debugMsg)
            printf("READ IDF %s\n", fi->filePath().toLatin1().constData());
      Xml xml(f);

      bool skipmode = true;
      for (;;) {
            Xml::Token token = xml.parse();
            const QString& tag = xml.s1();
            switch (token) {
                  case Xml::Error:
                  case Xml::End:
                        return;
                  case Xml::TagStart:
                        if (skipmode && tag == "muse")
                              skipmode = false;
                        else if (skipmode)
                              break;
                        else if (tag == "MidiInstrument") {
                              MidiInstrument* i = new MidiInstrument();
                              i->setFilePath(fi->filePath());
                              i->read(xml);
                              // Ignore duplicate named instruments.
                              iMidiInstrument ii = midiInstruments.begin();
                              for(; ii != midiInstruments.end(); ++ii)
                              {
                                if((*ii)->iname() == i->iname())
                                  break;
                              }
                              if(ii == midiInstruments.end())
                                midiInstruments.push_back(i);
                              else
                                delete i;
                            }
                        else
                              xml.unknown("muse");
                        break;
                  case Xml::Attribut:
                        break;
                  case Xml::TagEnd:
                        if (!skipmode && tag == "muse") {
                              return;
                              }
                  default:
                        break;
                  }
            }
      fclose(f);


      }

//---------------------------------------------------------
//   initMidiInstruments
//---------------------------------------------------------

void initMidiInstruments()
      {
      genericMidiInstrument = new MidiInstrument(QWidget::tr("generic midi"));
      midiInstruments.push_back(genericMidiInstrument);
      if (MusEGlobal::debugMsg)
        printf("load user instrument definitions from <%s>\n", MusEGlobal::museUserInstruments.toLatin1().constData());
      QDir usrInstrumentsDir(MusEGlobal::museUserInstruments, QString("*.idf"));
      if (usrInstrumentsDir.exists()) {
            QFileInfoList list = usrInstrumentsDir.entryInfoList();
            QFileInfoList::iterator it=list.begin(); // ddskrjo
            while(it != list.end()) { // ddskrjo
                  loadIDF(&*it);
                  ++it;
                  }
            }

      if (MusEGlobal::debugMsg)
        printf("load instrument definitions from <%s>\n", MusEGlobal::museInstruments.toLatin1().constData());
      QDir instrumentsDir(MusEGlobal::museInstruments, QString("*.idf"));
      if (instrumentsDir.exists()) {
            QFileInfoList list = instrumentsDir.entryInfoList();
            QFileInfoList::iterator it=list.begin(); // ddskrjo
            while(it!=list.end()) {
                  loadIDF(&*it);
                  ++it;
                  }
            }
      else
        printf("Instrument directory not found: %s\n", MusEGlobal::museInstruments.toLatin1().constData());

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
//   findMidiInstrument
//---------------------------------------------------------

iMidiInstrument MidiInstrumentList::find(const MidiInstrument* instr)
      {
      for (iMidiInstrument i = begin();
         i != end(); ++i) {
            if (*i == instr) {
                  return i;
                  }
            }
      return end();
      }

//---------------------------------------------------------
//   MidiInstrument
//---------------------------------------------------------

void MidiInstrument::init()
      {
      _tmpMidiStateVersion = 1; // Assume old version. readMidiState will overwrite anyway.
      _nullvalue = -1;
      _initScript = 0;
      _midiInit  = new EventList();
      _midiReset = new EventList();
      _midiState = new EventList();
      _controller = new MidiControllerList;

      // add some default controller to controller list
      // this controllers are always available for all instruments
      //
      MidiController* prog = new MidiController("Program", CTRL_PROGRAM, 0, 0xffffff, 0);
      _controller->add(prog);
      _dirty = false;

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
      for (ciPatchGroup g = pg.begin(); g != pg.end(); ++g)
      {
        PatchGroup* pgp = *g;
        const PatchList& pl = pgp->patches;
        for (ciPatch p = pl.begin(); p != pl.end(); ++p)
        {
          delete *p;
        }
        delete pgp;
      }


      delete _midiInit;
      delete _midiReset;
      delete _midiState;
      for(iMidiController i = _controller->begin(); i != _controller->end(); ++i)
          delete i->second;
      delete _controller;

      if (_initScript)
            delete _initScript;

      if(!_sysex.isEmpty())
      {
        int j = _sysex.size();
        for(int i = 0; i < j; ++i)
          delete _sysex.at(i);
      }

      patch_drummap_mapping.clear();
      }

//---------------------------------------------------------
//   assign
//---------------------------------------------------------

MidiInstrument& MidiInstrument::assign(const MidiInstrument& ins)
{
  //---------------------------------------------------------
  // TODO: Copy the _initScript (if and when it is ever used)
  //---------------------------------------------------------

  for(iMidiController i = _controller->begin(); i != _controller->end(); ++i)
      delete i->second;
  _controller->clear();

  _nullvalue = ins._nullvalue;

  // Assignment
  for(ciMidiController i = ins._controller->begin(); i != ins._controller->end(); ++i)
  {
    MidiController* mc = i->second;
    _controller->add(new MidiController(*mc));
  }

  if(!_sysex.isEmpty())
  {
    int j = _sysex.size();
    for(int i = 0; i < j; ++i)
      delete _sysex.at(i);
    _sysex.clear();
  }
  if(!ins.sysex().isEmpty())
  {
    int j = ins.sysex().size();
    for(int i = 0; i < j; ++i)
      _sysex.append(new MusECore::SysEx(*(ins.sysex().at(i))));
  }

  *(_midiInit) = *(ins._midiInit);
  *(_midiReset) = *(ins._midiReset);
  *(_midiState) = *(ins._midiState);

  for (ciPatchGroup g = pg.begin(); g != pg.end(); ++g)
  {
    PatchGroup* pgp = *g;
    const PatchList& pl = pgp->patches;
    for (ciPatch p = pl.begin(); p != pl.end(); ++p)
    {
      delete *p;
    }

    delete pgp;
  }
  pg.clear();

  // Assignment
  for(ciPatchGroup g = ins.pg.begin(); g != ins.pg.end(); ++g)
  {
    PatchGroup* pgp = *g;
    const PatchList& pl = pgp->patches;
    PatchGroup* npg = new PatchGroup;
    npg->name = pgp->name;
    pg.push_back(npg);
    for (ciPatch p = pl.begin(); p != pl.end(); ++p)
    {
      Patch* pp = *p;
      Patch* np = new Patch;
      //np->typ = pp->typ;
      np->hbank = pp->hbank;
      np->lbank = pp->lbank;
      np->prog = pp->prog;
      np->name = pp->name;
      np->drum = pp->drum;
      npg->patches.push_back(np);
    }
  }

  _name = ins._name;
  _filePath = ins._filePath;

  patch_drummap_mapping=ins.patch_drummap_mapping;

  // Hmm, dirty, yes? But init sets it to false... DELETETHIS
  //_dirty = ins._dirty;
  //_dirty = false;
  //_dirty = true;

  return *this;
}

//---------------------------------------------------------
//   midiType
//---------------------------------------------------------

MType MidiInstrument::midiType() const
{
  if(_name == "GM")
    return MT_GM;
  if(_name == "GM2")
    return MT_GM2;
  if(_name == "GS")
    return MT_GS;
  if(_name == "XG")
    return MT_XG;
  return MT_UNKNOWN;
}

//---------------------------------------------------------
//   reset
//    send note off to all channels
//---------------------------------------------------------

void MidiInstrument::reset(int portNo)
{
      MusECore::MidiPort* port = &MusEGlobal::midiPorts[portNo];
      if(port->device() == 0)
        return;

      MusECore::MidiPlayEvent ev;
      ev.setType(0x90);
      ev.setPort(portNo);
      ev.setTime(0);

      for (int chan = 0; chan < MIDI_CHANNELS; ++chan)
      {
            ev.setChannel(chan);
            for (int pitch = 0; pitch < 128; ++pitch)
            {
                  ev.setA(pitch);
                  ev.setB(0);

                  port->sendEvent(ev);
            }
      }
}

//---------------------------------------------------------
//   readPatchGroup
//---------------------------------------------------------

void PatchGroup::read(Xml& xml)
      {
      for (;;) {
            Xml::Token token = xml.parse();
            const QString& tag = xml.s1();
            switch (token) {
                  case Xml::Error:
                  case Xml::End:
                        return;
                  case Xml::TagStart:
                        if (tag == "Patch") {
                              Patch* patch = new Patch;
                              patch->read(xml);
                              patches.push_back(patch);
                              }
                        else
                              xml.unknown("PatchGroup");
                        break;
                  case Xml::Attribut:
                        if (tag == "name")
                              name = xml.s2();
                        break;
                  case Xml::TagEnd:
                        if (tag == "PatchGroup")
                              return;
                  default:
                        break;
                  }
            }
      }

//---------------------------------------------------------
//   read
//---------------------------------------------------------

void Patch::read(Xml& xml)
      {
      //typ   = -1;
      hbank = -1;
      lbank = -1;
      prog  = 0;
      drum  = false;
      for (;;) {
            Xml::Token token = xml.parse();
            const QString& tag = xml.s1();
            switch (token) {
                  case Xml::Error:
                  case Xml::End:
                        return;
                  case Xml::TagStart:
                        xml.unknown("Patch");
                        break;
                  case Xml::Attribut:
                        if (tag == "name")
                              name = xml.s2();
                        else if (tag == "mode")  // Obsolete
                        {
                              //typ = xml.s2().toInt();
                              xml.s2().toInt();
                        }
                        else if (tag == "hbank")
                              hbank = xml.s2().toInt();
                        else if (tag == "lbank")
                              lbank = xml.s2().toInt();
                        else if (tag == "prog")
                              prog = xml.s2().toInt();
                        else if (tag == "drum")
                              drum = xml.s2().toInt();
                        break;
                  case Xml::TagEnd:
                        if (tag == "Patch")
                              return;
                  default:
                        break;
                  }
            }
      }

//---------------------------------------------------------
//   write
//---------------------------------------------------------

void Patch::write(int level, Xml& xml)
      {
            xml.nput(level, "<Patch name=\"%s\"", Xml::xmlString(name).toLatin1().constData());
            //if(typ != -1)
            //  xml.nput(" mode=\"%d\"", typ);  // Obsolete

            if(hbank != -1)
              xml.nput(" hbank=\"%d\"", hbank);

            if(lbank != -1)
              xml.nput(" lbank=\"%d\"", lbank);

            xml.nput(" prog=\"%d\"", prog);

            if(drum)
              xml.nput(" drum=\"%d\"", int(drum));
            xml.put(" />");
      }

//---------------------------------------------------------
//   SysEx
//---------------------------------------------------------

SysEx::SysEx()
{
  dataLen = 0;
  data = 0;
}

SysEx::SysEx(const SysEx& src)
{
  name    = src.name;
  comment = src.comment;
  dataLen = src.dataLen;
  data = 0;
  if(dataLen != 0 && src.data)
  {
    data = new unsigned char[dataLen];
    memcpy(data, src.data, dataLen);
  }
}

SysEx::~SysEx()
{
  if(dataLen != 0 && data)
    delete[] data;
}

bool SysEx::read(Xml& xml)
      {
      for (;;) {
            Xml::Token token = xml.parse();
            const QString& tag = xml.s1();
            switch (token) {
                  case Xml::Error:
                  case Xml::End:
                        return false;
                  case Xml::TagStart:
                        if (tag == "comment")
                              comment = xml.parse1();
                        else if (tag == "data")
                        {
                              unsigned char*d;
                              int len = string2sysex(xml.parse1(), &d);
                              // Was the conversion succesful, even if empty?
                              if(len != -1)
                              {
                                // Delete existing.
                                if(dataLen != 0 && data)
                                  delete[] data;
                                dataLen = len;
                                data = d;
                              }
                        }
                        else
                              xml.unknown("SysEx");
                        break;
                  case Xml::Attribut:
                        if (tag == "name")
                              name = xml.s2();
                        break;
                  case Xml::TagEnd:
                        if (tag == "SysEx")
                        {
                          return !name.isEmpty();
                        }
                  default:
                        break;
                  }
            }

      return false;
      }

void SysEx::write(int level, Xml& xml)
      {
            xml.nput(level, "<SysEx name=\"%s\">\n", Xml::xmlString(name).toLatin1().constData());

            level++;
            if(!comment.isEmpty())
              xml.strTag(level, "comment", Xml::xmlString(comment).toLatin1().constData());
            if(dataLen > 0 && data)
              xml.strTag(level, "data", sysex2string(dataLen, data));

            xml.etag(level, "SysEx");
      }

//---------------------------------------------------------
//   readMidiState
//---------------------------------------------------------

void MidiInstrument::readMidiState(Xml& xml)
{
  // A kludge to support old midistates by wrapping them in the proper header.
  _tmpMidiStateVersion = 1;    // Assume old (unmarked) first version 1.
  for (;;)
  {
    Xml::Token token = xml.parse();
    const QString tag = xml.s1();
    switch (token)
    {
          case Xml::Error:
          case Xml::End:
                return;
          case Xml::TagStart:
                if (tag == "event")
                {
                  Event e(Note);
                  e.read(xml);
                  _midiState->add(e);
                }
                else
                xml.unknown("midistate");
                break;
          case Xml::Attribut:
                if(tag == "version")
                  _tmpMidiStateVersion = xml.s2().toInt();
                else
                  xml.unknown("MidiInstrument");
                break;
          case Xml::TagEnd:
                if(tag == "midistate")
                  return;
          default:
                break;
    }
  }
}

void MidiInstrument::readDrummaps(Xml& xml)
{
  patch_drummap_mapping.clear();

  for (;;)
  {
    Xml::Token token = xml.parse();
    const QString& tag = xml.s1();
    switch (token)
    {
      case Xml::Error:
      case Xml::End:
        return;

      case Xml::TagStart:
        if (tag == "entry")
          patch_drummap_mapping.push_back(readDrummapsEntry(xml));
        else
          xml.unknown("MidiInstrument::readDrummaps");
        break;

      case Xml::TagEnd:
        if (tag == "Drummaps")
          return;

      default:
        break;
    }
  }
  printf("ERROR: THIS CANNOT HAPPEN: exited infinite loop in MidiInstrument::readDrummaps()!\n"
         "                           not returning anything. expect undefined behaviour or even crashes.\n");
}

patch_drummap_mapping_t MidiInstrument::readDrummapsEntry(Xml& xml)
{
  using std::list;

  patch_collection_t collection;
  DrumMap* drummap=new DrumMap[128];
  for (int i=0;i<128;i++)
    drummap[i]=iNewDrumMap[i];

  for (;;)
  {
    Xml::Token token = xml.parse();
    const QString& tag = xml.s1();
    switch (token)
    {
      case Xml::Error:
      case Xml::End:
        return patch_drummap_mapping_t(collection, drummap);

      case Xml::TagStart:
        if (tag == "patch_collection")
          collection=readDrummapsEntryPatchCollection(xml);
        else if (tag == "drummap")
          read_new_style_drummap(xml, "drummap", drummap);
        else
          xml.unknown("MidiInstrument::readDrummapsEntry");
        break;

      case Xml::TagEnd:
        if (tag == "entry")
          return patch_drummap_mapping_t(collection, drummap);

      default:
        break;
    }
  }
  printf("ERROR: THIS CANNOT HAPPEN: exited infinite loop in MidiInstrument::readDrummapsEntry()!\n"
         "                           not returning anything. expect undefined behaviour or even crashes.\n");
  return patch_drummap_mapping_t();
}

patch_collection_t MidiInstrument::readDrummapsEntryPatchCollection(Xml& xml)
{
  int first_prog=0, last_prog=256;   // this means:
  int first_lbank=0, last_lbank=256; // "does not matter"
  int first_hbank=0, last_hbank=256;

  for (;;)
  {
    Xml::Token token = xml.parse();
    const QString& tag = xml.s1();
    switch (token)
    {
      case Xml::Error:
      case Xml::End:
        return patch_collection_t(-1,-1,-1,-1,-1,-1); // an invalid collection

      case Xml::TagStart:
        xml.unknown("MidiInstrument::readDrummapsEntryPatchCollection");
        break;

      case Xml::Attribut:
        if (tag == "prog")
          parse_range(xml.s2(), &first_prog, &last_prog);
        else if (tag == "lbank")
          parse_range(xml.s2(), &first_lbank, &last_lbank);
        else if (tag == "hbank")
          parse_range(xml.s2(), &first_hbank, &last_hbank);
        break;

      case Xml::TagEnd:
        if (tag == "patch_collection")
          return patch_collection_t(first_prog, last_prog, first_lbank, last_lbank, first_hbank, last_hbank);

      default:
        break;
    }
  }

  printf("ERROR: THIS CANNOT HAPPEN: exited infinite loop in MidiInstrument::readDrummapsEntryPatchCollection()!\n"
         "                           not returning anything. expect undefined behaviour or even crashes.\n");
}

void MidiInstrument::writeDrummaps(int level, Xml& xml) const
{
  xml.tag(level++, "Drummaps");

  for (std::list<patch_drummap_mapping_t>::const_iterator it=patch_drummap_mapping.begin();
       it!=patch_drummap_mapping.end(); it++)
  {
    xml.tag(level++, "entry");

    const patch_collection_t* ap = &it->affected_patches;
    QString tmp="<patch_collection ";
    if (ap->first_program==ap->last_program)
      tmp+="prog=\""+QString::number(ap->first_program)+"\" ";
    else if (! (ap->first_program==0 && ap->last_program>=127))
      tmp+="prog=\""+QString::number(ap->first_program)+"-"+QString::number(ap->last_program)+"\" ";

    if (ap->first_lbank==ap->last_lbank)
      tmp+="lbank=\""+QString::number(ap->first_lbank)+"\" ";
    else if (! (ap->first_lbank==0 && ap->last_lbank>=127))
      tmp+="lbank=\""+QString::number(ap->first_lbank)+"-"+QString::number(ap->last_lbank)+"\" ";

    if (ap->first_hbank==ap->last_hbank)
      tmp+="hbank=\""+QString::number(ap->first_hbank)+"\" ";
    else if (! (ap->first_hbank==0 && ap->last_hbank>=127))
      tmp+="hbank=\""+QString::number(ap->first_hbank)+"-"+QString::number(ap->last_hbank)+"\" ";

    tmp+="/>\n";

    xml.nput(level, tmp.toLatin1().data());

    write_new_style_drummap(level, xml, "drummap", it->drummap);

    xml.etag(--level, "entry");
  }

  xml.etag(--level, "Drummaps");
}

//---------------------------------------------------------
//   read
//---------------------------------------------------------

void MidiInstrument::read(Xml& xml)
      {
      bool ok;
      int base = 10;
      _nullvalue = -1;
      for (;;) {
            Xml::Token token = xml.parse();
            const QString& tag = xml.s1();
            switch (token) {
                  case Xml::Error:
                  case Xml::End:
                        return;
                  case Xml::TagStart:
                        if (tag == "Patch") {
                              Patch* patch = new Patch;
                              patch->read(xml);
                              if (pg.empty()) {
                                    PatchGroup* p = new PatchGroup;
                                    p->patches.push_back(patch);
                                    pg.push_back(p);
                                    }
                              else
                                    pg[0]->patches.push_back(patch);
                              }
                        else if (tag == "PatchGroup") {
                              PatchGroup* p = new PatchGroup;
                              p->read(xml);
                              pg.push_back(p);
                              }
                        else if (tag == "Controller") {
                              MidiController* mc = new MidiController();
                              mc->read(xml);
                              //
                              // HACK: make predefined "Program" controller overloadable
                              //
                              if (mc->name() == "Program") {
                                    for (iMidiController i = _controller->begin(); i != _controller->end(); ++i) {
                                          if (i->second->name() == mc->name()) {
                                                delete i->second;
                                                _controller->erase(i);
                                                break;
                                                }
                                          }
                                    }

                              _controller->add(mc);
                              }
                        else if (tag == "Drummaps") {
                              readDrummaps(xml);
                              }
                        else if (tag == "Init")
                              readEventList(xml, _midiInit, "Init");
                        else if (tag == "Reset")
                              readEventList(xml, _midiReset, "Reset");
                        else if (tag == "State")
                              readEventList(xml, _midiState, "State");
                        else if (tag == "InitScript") {
                              if (_initScript)
                                    delete _initScript;
                              QByteArray ba = xml.parse1().toLatin1();
                              const char* istr = ba.constData();
                              int len = ba.length() +1;
                              if (len > 1) {
                                    _initScript = new char[len];
                                    memcpy(_initScript, istr, len);
                                    }
                              }
                        else if (tag == "SysEx") {
                              SysEx* se = new SysEx;
                              if(!se->read(xml))
                              {
                                delete se;
                                printf("MidiInstrument::read():SysEx: reading sysex failed\n");
                              }
                              else
                                _sysex.append(se);
                              }
                        else
                              xml.unknown("MidiInstrument");
                        break;
                  case Xml::Attribut:
                        if (tag == "name")
                              setIName(xml.s2());
                        else if(tag == "nullparam") {
                              _nullvalue = xml.s2().toInt(&ok, base);
                        }
                        break;
                  case Xml::TagEnd:
                        if (tag == "MidiInstrument")
                              return;
                  default:
                        break;
                  }
            }
      }

//---------------------------------------------------------
//   write
//---------------------------------------------------------

void MidiInstrument::write(int level, Xml& xml)
      {
      xml.header();
      xml.tag(level, "muse version=\"1.0\"");
      level++;
      xml.nput(level, "<MidiInstrument name=\"%s\"", Xml::xmlString(iname()).toLatin1().constData());

      if(_nullvalue != -1)
      {
        QString nv;
        nv.setNum(_nullvalue);
        xml.nput(" nullparam=\"%s\"", nv.toLatin1().constData());
      }
      xml.put(">");

      level++;
      for (ciPatchGroup g = pg.begin(); g != pg.end(); ++g) {
            PatchGroup* pgp = *g;
            const PatchList& pl = pgp->patches;
            xml.tag(level, "PatchGroup name=\"%s\"", Xml::xmlString(pgp->name).toLatin1().constData());
            level++;
            for (ciPatch p = pl.begin(); p != pl.end(); ++p)
                  (*p)->write(level, xml);
            level--;
            xml.etag(level, "PatchGroup");
            }
      for (iMidiController ic = _controller->begin(); ic != _controller->end(); ++ic)
            ic->second->write(level, xml);
      if(!_sysex.isEmpty())
      {
        int j = _sysex.size();
        for(int i = 0; i < j; ++i)
          _sysex.at(i)->write(level, xml);
      }

      xml.tag(level++, "Init");
      for(ciEvent ev=_midiInit->begin(); ev != _midiInit->end(); ++ev)
        ev->second.write(level, xml, MusECore::Pos(0, true));
      xml.etag(--level, "Init");

      // -------------
      // TODO: What about _midiReset, _midiState, and _initScript ?
      // -------------

      writeDrummaps(level, xml);

      level--;
      xml.etag(level, "MidiInstrument");
      level--;
      xml.etag(level, "muse");
      }


//---------------------------------------------------------
//   populateInstrPopup  (static)
//---------------------------------------------------------

void MidiInstrument::populateInstrPopup(MusEGui::PopupMenu* menu, MidiInstrument* /*current*/, bool show_synths)
      {
      menu->clear();
      for (MusECore::iMidiInstrument i = MusECore::midiInstruments.begin(); i
          != MusECore::midiInstruments.end(); ++i)
          {
            // Do not list synths. Although it is possible to assign a synth
            //  as an instrument to a non-synth device, we should not allow this.
            // (One reason is that the 'show gui' column is then enabled, which
            //  makes no sense for a non-synth device).
            if(show_synths || !(*i)->isSynti())
              menu->addAction((*i)->iname());
          }
    }

//---------------------------------------------------------
//   populatePatchPopup
//---------------------------------------------------------

void MidiInstrument::populatePatchPopup(MusEGui::PopupMenu* menu, int /*chan*/, bool drum)
      {
      menu->clear();
      //int mask = 7;

      if (pg.size() > 1) {
            for (ciPatchGroup i = pg.begin(); i != pg.end(); ++i) {
                  PatchGroup* pgp = *i;
                  MusEGui::PopupMenu* pm = 0;
                  const PatchList& pl = pgp->patches;
                  for (ciPatch ipl = pl.begin(); ipl != pl.end(); ++ipl) {
                        const Patch* mp = *ipl;
                        if (//(mp->typ & mask) &&
                            (mp->drum == drum)) {
                              if(!pm) {
                                pm = new MusEGui::PopupMenu(pgp->name, menu, menu->stayOpen());  // Use the parent stayOpen here.
                                menu->addMenu(pm);
                                pm->setFont(MusEGlobal::config.fonts[0]);
                              }
                              int id = ((mp->hbank & 0xff) << 16)
                                         + ((mp->lbank & 0xff) << 8) + (mp->prog & 0xff);
                              QAction* act = pm->addAction(mp->name);
                              act->setData(id);
                            }
                        }
                  }
            }
      else if (pg.size() == 1 ){
            // no groups
            const PatchList& pl = pg.front()->patches;
            for (ciPatch ipl = pl.begin(); ipl != pl.end(); ++ipl) {
                  const Patch* mp = *ipl;
                  //if (mp->typ & mask) {
                        int id = ((mp->hbank & 0xff) << 16)
                                 + ((mp->lbank & 0xff) << 8) + (mp->prog & 0xff);
                        QAction* act = menu->addAction(mp->name);
                        act->setData(id);
                        //}
                  }
            }

    }



//---------------------------------------------------------
//   getPatchName
//---------------------------------------------------------

QString MidiInstrument::getPatchName(int /*channel*/, int prog, bool drum) const
      {
      int pr = prog & 0xff;
      if(prog == CTRL_VAL_UNKNOWN || pr == 0xff)
            return "<unknown>";

      int hbank = (prog >> 16) & 0xff;
      int lbank = (prog >> 8) & 0xff;
      //int tmask = 1;

      bool hb = hbank != 0xff;
      bool lb = lbank != 0xff;
      for (ciPatchGroup i = pg.begin(); i != pg.end(); ++i) {
            const PatchList& pl = (*i)->patches;
            for (ciPatch ipl = pl.begin(); ipl != pl.end(); ++ipl) {
                  const Patch* mp = *ipl;
                  if (//(mp->typ & tmask) &&
                      (pr == mp->prog)
                    && (mp->drum == drum)

                    && (hbank == mp->hbank || !hb || mp->hbank == -1)
                    && (lbank == mp->lbank || !lb || mp->lbank == -1))
                        return mp->name;
                  }
            }
      return "<unknown>";
      }

unsigned MidiInstrument::getNextPatch(int channel, unsigned patch, bool drum)
{
  QList<dumb_patchlist_entry_t> haystack=getPatches(channel,drum);
  if (haystack.empty()) return MusECore::CTRL_VAL_UNKNOWN;

  int prog=patch&0xFF;
  int lbank=(patch>>8)&0xFF;
  int hbank=(patch>>16)&0xFF;

  dumb_patchlist_entry_t needle=dumb_patchlist_entry_t(prog, (lbank!=0xFF)?lbank:-1, (hbank!=0xFF)?hbank:-1);

  QList<dumb_patchlist_entry_t>::iterator it;
  for (it=haystack.begin(); it!=haystack.end(); it++)
    if ((*it) == needle)
      break;

  if (it==haystack.end()) //not found? use first entry
    it=haystack.begin();
  else
  {
    for (;it!=haystack.end(); it++)
      if ((*it)!=needle)
        break;
    if (it==haystack.end()) it=haystack.begin(); //wrap-over
  }

  return (it->prog&0xFF)  |
         ((((it->lbank==-1)?0xFF:it->lbank)<<8)&0xFF00)  |
         ((((it->hbank==-1)?0xFF:it->hbank)<<16)&0xFF0000);
}

unsigned MidiInstrument::getPrevPatch(int channel, unsigned patch, bool drum)
{
  QList<dumb_patchlist_entry_t> haystack=getPatches(channel,drum);
  if (haystack.empty()) return MusECore::CTRL_VAL_UNKNOWN;

  int prog=patch&0xFF;
  int lbank=(patch>>8)&0xFF;
  int hbank=(patch>>16)&0xFF;

  dumb_patchlist_entry_t needle=dumb_patchlist_entry_t(prog, (lbank!=0xFF)?lbank:-1, (hbank!=0xFF)?hbank:-1);

  QList<dumb_patchlist_entry_t>::iterator it;
  for (it=haystack.begin(); it!=haystack.end(); it++)
    if ((*it) == needle)
      break;

  if (it==haystack.end()) //not found? use first entry
    it=haystack.begin();
  else
  {
    if (it==haystack.begin()) it=haystack.end(); //wrap-over
    it--;
  }

  return (it->prog&0xFF)  |
         ((((it->lbank==-1)?0xFF:it->lbank)<<8)&0xFF00)  |
         ((((it->hbank==-1)?0xFF:it->hbank)<<16)&0xFF0000);
}

QList<dumb_patchlist_entry_t> MidiInstrument::getPatches(int /*channel*/, bool drum)
      {
      //int tmask = 1;
      QList<dumb_patchlist_entry_t> tmp;

      for (ciPatchGroup i = pg.begin(); i != pg.end(); ++i) {
            const PatchList& pl = (*i)->patches;
            for (ciPatch ipl = pl.begin(); ipl != pl.end(); ++ipl) {
                  const Patch* mp = *ipl;
                  if (//(mp->typ & tmask) &&
                      (mp->drum == drum))
                  {
                    int prog = mp->prog;
                    int lbank = mp->lbank;
                    int hbank = mp->hbank;
                    tmp.push_back(dumb_patchlist_entry_t(prog,lbank,hbank));
                  }
            }
      }

      return tmp;
      }


const DrumMap* MidiInstrument::drummap_for_patch(int patch) const
{
  using std::list;

  int program = (patch & 0x0000FF);
  int lbank =   (patch & 0x00FF00) >> 8;
  int hbank =   (patch & 0xFF0000) >> 16;

  for (list<patch_drummap_mapping_t>::const_iterator it=patch_drummap_mapping.begin();
       it!=patch_drummap_mapping.end(); it++)
  {
    const patch_collection_t* ap = &it->affected_patches;
    // if the entry matches our patch
    if ( (program >= ap->first_program && program <= ap->last_program) &&
         (hbank >= ap->first_hbank && hbank <= ap->last_hbank) &&
         (lbank >= ap->first_lbank && lbank <= ap->last_lbank) )
    {
      return it->drummap;
    }
  }

  // if nothing was found
  return iNewDrumMap;
}

patch_drummap_mapping_t::patch_drummap_mapping_t()
{
  drummap=new DrumMap[128];
  for (int i=0;i<128;i++)
    drummap[i]=iNewDrumMap[i];
}

patch_drummap_mapping_t::patch_drummap_mapping_t(const patch_drummap_mapping_t& that)
{
  drummap=new DrumMap[128];
  for (int i=0;i<128;i++)
    drummap[i]=that.drummap[i];

  affected_patches=that.affected_patches;
}

patch_drummap_mapping_t& patch_drummap_mapping_t::operator=(const patch_drummap_mapping_t& that)
{
  if (drummap)
    delete [] drummap;

  drummap=new DrumMap[128];
  for (int i=0;i<128;i++)
    drummap[i]=that.drummap[i];

  affected_patches=that.affected_patches;

  return *this;
}

patch_drummap_mapping_t::~patch_drummap_mapping_t()
{
  delete [] drummap;
}

QString patch_collection_t::to_string()
{
  QString tmp;

  if (first_program==0 && last_program>=127 &&
      first_lbank==0 && last_lbank>=127 &&
      first_hbank==0 && last_hbank>=127)
    tmp="default";
  else
  {
    tmp+="prog: ";
    if (first_program==last_program)
      tmp+=QString::number(first_program+1);
    else if (! (first_program==0 && last_program>=127))
      tmp+=QString::number(first_program+1)+"-"+QString::number(last_program+1);
    else
      tmp+="*";

    tmp+=" bank=";
    if (first_lbank==last_lbank)
      tmp+=QString::number(first_lbank+1);
    else if (! (first_lbank==0 && last_lbank>=127))
      tmp+=QString::number(first_lbank+1)+"-"+QString::number(last_lbank+1);
    else
      tmp+="*";

    tmp+="/";
    if (first_hbank==last_hbank)
      tmp+=QString::number(first_hbank+1);
    else if (! (first_hbank==0 && last_hbank>=127))
      tmp+=QString::number(first_hbank+1)+"-"+QString::number(last_hbank+1);
    else
      tmp+="*";

  }
  return tmp;
}

} // namespace MusECore
