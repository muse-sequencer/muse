//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: minstrument.cpp,v 1.10.2.5 2009/03/28 01:46:10 terminator356 Exp $
//
//  (C) Copyright 2000-2003 Werner Schweer (ws@seh.de)
//  (C) Copyright 2016 Tim E. Real (terminator356 on users dot sourceforge dot net)
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
#include <QByteArray>

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

#ifdef _USE_INSTRUMENT_OVERRIDES_
namespace MusEGlobal {
  // This list holds instrument drum map overrides read from config.
  // Whenever an instrument has been loaded it will adopt any corresponding item in this list.
  // (Instruments are loaded long after config is loaded. So we need this 'holding' list.)
  MusECore::WorkingDrumMapInstrumentList workingDrumMapInstrumentList;
}
#endif

namespace MusECore {

MidiInstrumentList midiInstruments;
MidiInstrument* genericMidiInstrument;

//---------------------------------------------------------
//   string2sysex
//   Return -1 if cannot be converted.
//---------------------------------------------------------

int string2sysex(const QString& s, unsigned char** data)
      {
      QByteArray ba = s.toLatin1();
      const char* src = ba.constData();
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
          // Strip all f0 and f7 (whether accidental or on purpose enclosing etc).
          if(val == MusECore::ME_SYSEX || val == MusECore::ME_SYSEX_END)
            continue;
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
      for (int i = 0; i < len; ++i) {
            if ((i > 0) && ((i % 8)==0)) {
                  d += QString("\n");
                  }
            else if (i)
                  d += QString(" ");
            // Strip all f0 and f7 (whether accidental or on purpose enclosing etc).
            if(data[i] == MusECore::ME_SYSEX || data[i] == MusECore::ME_SYSEX_END)
              continue;
            d += QString("%1").arg(data[i], 2, 16, QLatin1Char('0'));
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
                              {

#ifdef _USE_INSTRUMENT_OVERRIDES_
                                // Add in the drum map overrides that were found in config.
                                // They can only be added now that the instrument has been loaded.
                                ciWorkingDrumMapInstrumentList_t iwdmil =
                                  MusEGlobal::workingDrumMapInstrumentList.find(i->iname().toStdString());
                                if(iwdmil != MusEGlobal::workingDrumMapInstrumentList.end())
                                {
                                  const WorkingDrumMapPatchList& wdmil = iwdmil->second;
                                  patch_drummap_mapping_list_t* pdml = i->get_patch_drummap_mapping();
                                  int patch;
                                  for(ciWorkingDrumMapPatchList_t iwdmpl = wdmil.begin(); iwdmpl != wdmil.end(); ++iwdmpl)
                                  {
                                    patch = iwdmpl->first;
                                    iPatchDrummapMapping_t ipdm = pdml->find(patch, false); // No default.
                                    if(ipdm != pdml->end())
                                    {
                                      patch_drummap_mapping_t& pdm = *ipdm;
                                      const WorkingDrumMapList& wdml = iwdmpl->second;
                                      pdm._workingDrumMapList = wdml;
                                    }
                                  }
                                  // TODO: Done with the config override, so erase it? Hm, maybe we might need it later...
                                  //MusEGlobal::workingDrumMapInstrumentList.erase(iwdmil);
                                }
#endif
                                midiInstruments.push_back(i);
                              }
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

      // Initialize with a default drum map on default channel. Patch is default 0xffffff. GM-1 does not specify a drum patch number.
      ChannelDrumMappingList* cdml = genericMidiInstrument->getChannelDrumMapping();
      cdml->add(-1, patch_drummap_mapping_list_t());

#ifdef _USE_INSTRUMENT_OVERRIDES_
      // Add in the drum map overrides that were found in config.
      // They can only be added now that the instrument has been created.
      ciWorkingDrumMapInstrumentList_t iwdmil =
        MusEGlobal::workingDrumMapInstrumentList.find(genericMidiInstrument->iname().toStdString());
      if(iwdmil != MusEGlobal::workingDrumMapInstrumentList.end())
      {
        const WorkingDrumMapPatchList& wdmil = iwdmil->second;
        int patch;
        for(ciWorkingDrumMapPatchList_t iwdmpl = wdmil.begin(); iwdmpl != wdmil.end(); ++iwdmpl)
        {
          patch = iwdmpl->first;
          iPatchDrummapMapping_t ipdm = pdml->find(patch, false); // No default.
          if(ipdm != pdml->end())
          {
            patch_drummap_mapping_t& pdm = *ipdm;
            const WorkingDrumMapList& wdml = iwdmpl->second;
            pdm._workingDrumMapList = wdml;
          }
        }
        // TODO: Done with the config override, so erase it? Hm, maybe we might need it later...
        //MusEGlobal::workingDrumMapInstrumentList.erase(iwdmil);
      }
#endif

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

#ifdef _USE_INSTRUMENT_OVERRIDES_
void MidiInstrumentList::writeDrummapOverrides(int level, Xml& xml) const
{
  MidiInstrument* mi;
  for(ciMidiInstrument imi = begin(); imi != end(); ++imi)
  {
    mi = *imi;
    mi->writeDrummapOverrides(level, xml);
  }
}
#endif

//---------------------------------------------------------
//   MidiInstrument
//---------------------------------------------------------

void MidiInstrument::init()
      {
      _noteOffMode = NoteOffAll; // By default, use note offs.
      _tmpMidiStateVersion = 1; // Assume old version. readMidiState will overwrite anyway.
      _initScript = 0;
      _waitForLSB = true;
      _midiInit  = new EventList();
      _midiReset = new EventList();
      _midiState = new EventList();
      _controller = new MidiControllerList;

      // add some default controller to controller list
      // this controllers are always available for all instruments
      //
      MidiController* prog = new MidiController("Program", CTRL_PROGRAM, 0, 0xffffff, 0, 0);
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

      _channelDrumMapping.clear();
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
  
  _controller->clr();
  _waitForLSB = ins._waitForLSB;
  _noteOffMode = ins._noteOffMode;

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
      np->program = pp->program;
      np->name = pp->name;
      np->drum = pp->drum;
      npg->patches.push_back(np);
    }
  }

  _name = ins._name;
  _filePath = ins._filePath;

  _channelDrumMapping = ins._channelDrumMapping;

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
//   To be called by audio thread only.
//---------------------------------------------------------

void MidiInstrument::reset(int portNo)
{
      MusECore::MidiPort* port = &MusEGlobal::midiPorts[portNo];
      if(port->device() == 0)
        return;

      MusECore::MidiPlayEvent ev;
      ev.setType(ME_NOTEOFF);
      ev.setPort(portNo);
      ev.setTime(0);  // Immediate processing. TODO: Use curFrame?
      ev.setB(64);

      for (int chan = 0; chan < MIDI_CHANNELS; ++chan)
      {
            ev.setChannel(chan);
            for (int pitch = 0; pitch < 128; ++pitch)
            {
                  ev.setA(pitch);
                  port->device()->putEvent(ev, MidiDevice::NotLate);
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
      hbank = -1;
      lbank = -1;
      program  = -1;
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
                              xml.s2().toInt();
                        }
                        else if (tag == "hbank")
                              hbank = xml.s2().toInt();
                        else if (tag == "lbank")
                              lbank = xml.s2().toInt();
                        else if (tag == "prog")
                              program = xml.s2().toInt();
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

            if(hbank != -1)
              xml.nput(" hbank=\"%d\"", hbank);

            if(lbank != -1)
              xml.nput(" lbank=\"%d\"", lbank);

            if(program != -1)
              xml.nput(" prog=\"%d\"", program);

            if(drum)
              xml.nput(" drum=\"%d\"", int(drum));
            xml.put(" />");
      }

iPatch PatchList::find(int patch, bool drum, bool includeDefault)
{
  int pnum;
  Patch* p;
  iPatch ip_default = end();
  for(iPatch ip = begin(); ip != end(); ++ip)
  {
    p = *ip;
    pnum = p->patch();
    // Look for an exact match above all else. The given patch must be valid.
    if(patch != CTRL_VAL_UNKNOWN && pnum == patch && p->drum == drum)
      return ip;
    // If no exact match is found we'll take a default if found (all three pr, hb, lb = don't care).
    if(includeDefault && p->dontCare() && p->drum == drum && ip_default == end())
      ip_default = ip;
  }
  return ip_default;
}

ciPatch PatchList::find(int patch, bool drum, bool includeDefault) const
{
  int pnum;
  const Patch* p;
  ciPatch ip_default = end();
  for(ciPatch ip = begin(); ip != end(); ++ip)
  {
    p = *ip;
    pnum = p->patch();
    // Look for an exact match above all else. The given patch must be valid.
    if(patch != CTRL_VAL_UNKNOWN && pnum == patch && p->drum == drum)
      return ip;
    // If no exact match is found we'll take a default if found (all three pr, hb, lb = don't care).
    if(includeDefault && p->dontCare() && p->drum == drum && ip_default == end())
      ip_default = ip;
  }
  return ip_default;
}

Patch* PatchGroupList::findPatch(int patch, bool drum, bool includeDefault)
{
  for(iPatchGroup ipg = begin(); ipg != end(); ++ipg)
  {
    PatchGroup* pg = *ipg;
    iPatch ip = pg->patches.find(patch, drum, includeDefault);
    if(ip != pg->patches.end())
      return *ip;
  }
  return 0;
}

Patch* PatchGroupList::findPatch(int patch, bool drum, bool includeDefault) const
{
  for(ciPatchGroup ipg = begin(); ipg != end(); ++ipg)
  {
    const PatchGroup* pg = *ipg;
    ciPatch ip = pg->patches.find(patch, drum, includeDefault);
    if(ip != pg->patches.end())
      return *ip;
  }
  return 0;
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
  //_channelDrumMapping.clear(); // ???
  const QString start_tag = xml.s1();
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
        if (tag == "drumMapChannel")
          _channelDrumMapping.read(xml);
        else if (tag == "entry")
        {
          patch_drummap_mapping_list_t pdml;
          pdml.read(xml);
          if(!pdml.empty())
            _channelDrumMapping.add(-1, pdml); // Add to the default channel.
        }
        else
          xml.unknown("MidiInstrument::readDrummaps");
        break;

      case Xml::TagEnd:
        if (tag == start_tag)
          return;

      default:
        break;
    }
  }
  printf("ERROR: THIS CANNOT HAPPEN: exited infinite loop in MidiInstrument::readDrummaps()!\n"
         "                           not returning anything. expect undefined behaviour or even crashes.\n");
}

void MidiInstrument::writeDrummaps(int level, Xml& xml) const
{
  xml.tag(level++, "Drummaps");

  _channelDrumMapping.write(level, xml);

  xml.etag(--level, "Drummaps");
}

//---------------------------------------------------------
//   read
//---------------------------------------------------------

void MidiInstrument::read(Xml& xml)
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
                                                _controller->del(i);
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
                        else if(tag == "nullparam") { } // Obsolete.
                        else if(tag == "NoteOffMode") 
                              _noteOffMode = (NoteOffMode)xml.s2().toInt(); // Default is NoteOffAll.
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

      if(noteOffMode() != NoteOffAll) // Default is NoteOffAll.
        xml.nput(" NoteOffMode=\"%d\"", noteOffMode());
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

#ifdef _USE_INSTRUMENT_OVERRIDES_
void MidiInstrument::writeDrummapOverrides(int level, Xml& xml) const
{
  for(ciPatchDrummapMapping_t ipdm = patch_drummap_mapping.begin(); ipdm != patch_drummap_mapping.end(); ++ipdm)
  {
    if(!(*ipdm)._workingDrumMapList.empty())
    {
      xml.tag(level++, "drummapOverrides instrument=\"%s\"", Xml::xmlString(iname()).toLatin1().constData());
      patch_drummap_mapping.writeDrummapOverrides(level, xml);
      xml.etag(--level, "drummapOverrides");
      break;
    }
  }
}
#endif

patch_drummap_mapping_list_t* MidiInstrument::get_patch_drummap_mapping(int channel, bool includeDefault)
{
  patch_drummap_mapping_list_t* pdml = _channelDrumMapping.find(channel, includeDefault);
  if(!pdml)
    // Not found? Search the global mapping list.
    return genericMidiInstrument->getChannelDrumMapping()->find(channel, includeDefault);
  return pdml;
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
                                         + ((mp->lbank & 0xff) << 8) + (mp->program & 0xff);
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
                                 + ((mp->lbank & 0xff) << 8) + (mp->program & 0xff);
                        QAction* act = menu->addAction(mp->name);
                        act->setData(id);
                        //}
                  }
            }

    }

void MidiInstrument::getMapItem(int channel, int patch, int index, DrumMap& dest_map, int
#ifdef _USE_INSTRUMENT_OVERRIDES_
overrideType
#endif
) const
{
  const patch_drummap_mapping_list_t* pdml = _channelDrumMapping.find(channel, true); // Include default.
  if(!pdml)
  {
    fprintf(stderr, "MidiInstrument::getMapItem Error: No channel:%d mapping or default found. Using iNewDrumMap.\n", channel);
    dest_map = iNewDrumMap[index];
    return;
  }

  // Always search this instrument's mapping first.
  ciPatchDrummapMapping_t ipdm = pdml->find(patch, false); // Don't include defaults here.
  if(ipdm == pdml->end())
  {
    // Not found? Is there a default patch mapping?
#ifdef _USE_INSTRUMENT_OVERRIDES_
    if(overrideType & WorkingDrumMapEntry::InstrumentDefaultOverride)
#endif
      ipdm = pdml->find(CTRL_PROGRAM_VAL_DONT_CARE, false); // Don't include defaults here.

    if(ipdm == pdml->end())
    {
      // Not found? Search the global mapping list.
      patch_drummap_mapping_list_t* def_pdml = genericMidiInstrument->get_patch_drummap_mapping(channel, false);
      if(!def_pdml)
      {
        //fprintf(stderr, "MidiInstrument::getMapItem Error: No default patch mapping found in genericMidiInstrument. Using iNewDrumMap.\n");
        dest_map = iNewDrumMap[index];
        return;
      }
      ipdm = def_pdml->find(patch, false); // Don't include defaults here.
      if(ipdm == def_pdml->end())
      {
        // Not found? Is there a default patch mapping?
#ifdef _USE_INSTRUMENT_OVERRIDES_
        if(overrideType & WorkingDrumMapEntry::InstrumentDefaultOverride)
#endif
          ipdm = def_pdml->find(CTRL_PROGRAM_VAL_DONT_CARE, false); // Don't include defaults here.

        if(ipdm == def_pdml->end())
        {
          // Not found? Use the global drum map.
          // Update: This shouldn't really happen now, since we have added a default patch drum map to the genericMidiInstrument.
          fprintf(stderr, "MidiInstrument::getMapItem Error: No default patch mapping found in genericMidiInstrument. Using iNewDrumMap.\n");
          dest_map = iNewDrumMap[index];
          return;
        }
      }
    }
  }
  const patch_drummap_mapping_t& pdm = (*ipdm);

  dest_map = pdm.drummap[index];

#ifdef _USE_INSTRUMENT_OVERRIDES_
  // Did we request to include any instrument overrides?
  if(!(overrideType & WorkingDrumMapEntry::InstrumentOverride))
    return;

  // Get any instrument overrides.
  ciWorkingDrumMapPatch_t iwdp = pdm._workingDrumMapList.find(index);
  if(iwdp == pdm._workingDrumMapList.end())
    return;

  const WorkingDrumMapEntry& wdm = iwdp->second;

  if(wdm._fields & WorkingDrumMapEntry::NameField)
    dest_map.name = wdm._mapItem.name;

  if(wdm._fields & WorkingDrumMapEntry::VolField)
    dest_map.vol = wdm._mapItem.vol;

  if(wdm._fields & WorkingDrumMapEntry::QuantField)
    dest_map.quant = wdm._mapItem.quant;

  if(wdm._fields & WorkingDrumMapEntry::LenField)
    dest_map.len = wdm._mapItem.len;

  if(wdm._fields & WorkingDrumMapEntry::ChanField)
    dest_map.channel = wdm._mapItem.channel;

  if(wdm._fields & WorkingDrumMapEntry::PortField)
    dest_map.port = wdm._mapItem.port;

  if(wdm._fields & WorkingDrumMapEntry::Lv1Field)
    dest_map.lv1 = wdm._mapItem.lv1;

  if(wdm._fields & WorkingDrumMapEntry::Lv2Field)
    dest_map.lv2 = wdm._mapItem.lv2;

  if(wdm._fields & WorkingDrumMapEntry::Lv3Field)
    dest_map.lv3 = wdm._mapItem.lv3;

  if(wdm._fields & WorkingDrumMapEntry::Lv4Field)
    dest_map.lv4 = wdm._mapItem.lv4;

  if(wdm._fields & WorkingDrumMapEntry::ENoteField)
    dest_map.enote = wdm._mapItem.enote;

  if(wdm._fields & WorkingDrumMapEntry::ANoteField)
    dest_map.anote = wdm._mapItem.anote;

  if(wdm._fields & WorkingDrumMapEntry::MuteField)
    dest_map.mute = wdm._mapItem.mute;

  if(wdm._fields & WorkingDrumMapEntry::HideField)
    dest_map.hide = wdm._mapItem.hide;
#endif

}



#ifdef _USE_INSTRUMENT_OVERRIDES_
int MidiInstrument::isWorkingMapItem(int patch, int index, int fields) const
{
  int ret = WorkingDrumMapEntry::NoOverride;

  // Is there a default patch override for this drum map item?
  bool def_ipdm_valid = true;
  ciPatchDrummapMapping_t def_ipdm = patch_drummap_mapping.find(CTRL_PROGRAM_VAL_DONT_CARE, false); // Don't include defaults here.
  if(def_ipdm == patch_drummap_mapping.end())
  {
    // Not found? Search the global mapping list.
    def_ipdm = genericMidiInstrument->get_patch_drummap_mapping()->find(CTRL_PROGRAM_VAL_DONT_CARE, false);
    if(def_ipdm == genericMidiInstrument->get_patch_drummap_mapping()->end())
      def_ipdm_valid = false;
  }
  if(def_ipdm_valid)
  {
    const patch_drummap_mapping_t& pdm = (*def_ipdm);
    ciWorkingDrumMapPatch_t iwdp = pdm._workingDrumMapList.find(index);
    if(iwdp != pdm._workingDrumMapList.end())
    {
      const WorkingDrumMapEntry& wdm = iwdp->second;
      if(wdm._fields & fields)
        ret |= WorkingDrumMapEntry::InstrumentDefaultOverride;
    }
  }

  // Is there a patch override for this drum map item?
  // Always search this instrument's mapping first.
  bool ipdm_valid = true;
  ciPatchDrummapMapping_t ipdm = patch_drummap_mapping.find(patch, false);
  if(ipdm == patch_drummap_mapping.end())
  {
    // Not found? Search the global mapping list.
    ipdm = MusECore::genericMidiInstrument->get_patch_drummap_mapping()->find(patch, false);
    if(ipdm == MusECore::genericMidiInstrument->get_patch_drummap_mapping()->end())
      ipdm_valid = false;
  }
  if(ipdm_valid)
  {
    const patch_drummap_mapping_t& pdm = (*ipdm);
    ciWorkingDrumMapPatch_t iwdp = pdm._workingDrumMapList.find(index);
    if(iwdp != pdm._workingDrumMapList.end())
    {
      const WorkingDrumMapEntry& wdm = iwdp->second;
      if(wdm._fields & fields)
        ret |= WorkingDrumMapEntry::InstrumentOverride;
    }
  }

  return ret;
}

void MidiInstrument::clearDrumMapOverrides()
{
  for(iPatchDrummapMapping_t ipdm = patch_drummap_mapping.begin(); ipdm != patch_drummap_mapping.end(); ++ipdm)
  {
    patch_drummap_mapping_t& pdm = *ipdm;
    pdm._workingDrumMapList.clear();
  }
}

bool MidiInstrument::setWorkingDrumMapItem(int patch, int index, const WorkingDrumMapEntry& item, bool isReset)
{
  // Special value. Save it from searching.
//   if(patch == CTRL_VAL_UNKNOWN)
//     return false;

//   iPatchDrummapMapping_t patch_ipm;
//   patch_ipm = patch_drummap_mapping.find(patch);
//   // You can't edit a drum map item in a collection that doesn't exist.
//   if(patch_ipm == patch_drummap_mapping.end())
//     return false;

  // Always search this instrument's mapping first.
  iPatchDrummapMapping_t ipdm = patch_drummap_mapping.find(patch, false); // Don't include defaults here.
  if(ipdm == patch_drummap_mapping.end())
  {
    // Not found? Search the global mapping list.
    ipdm = MusECore::genericMidiInstrument->get_patch_drummap_mapping()->find(patch, false); // Don't include defaults here.
    // Not found? You can't edit a drum map item in a collection that doesn't exist.
    if(ipdm == MusECore::genericMidiInstrument->get_patch_drummap_mapping()->end())
      return false;
  }

  patch_drummap_mapping_t& pdm = *ipdm;

  const int fields = item._fields;

  DrumMap cur_dm;
  getMapItem(patch, index, cur_dm, WorkingDrumMapEntry::InstrumentOverride | WorkingDrumMapEntry::InstrumentDefaultOverride);
  const int cur_enote = cur_dm.enote;

  if(isReset)
    pdm.removeWorkingDrumMapEntry(index, fields);
  else
    pdm.addWorkingDrumMapEntry(index, item);

  DrumMap new_dm;
  getMapItem(patch, index, new_dm, WorkingDrumMapEntry::InstrumentOverride | WorkingDrumMapEntry::InstrumentDefaultOverride);

  if(fields & WorkingDrumMapEntry::ENoteField)
  {
    int new_enote = new_dm.enote;
    int other_index = pdm.drum_in_map[new_enote];
    {
      DrumMap other_dm;
      if(isReset)
      {
        // Here we need to see the map item value just /before/ any override, so that we can tell
        //  whether this other_index brute-force 'reset' value is still technically an
        //  override, and either remove or add (modify) the list appropriately.
        getMapItem(patch, other_index, other_dm, WorkingDrumMapEntry::InstrumentDefaultOverride);
        if(other_dm.enote == cur_enote)
        {
          // The values are equal. This is technically no longer an override and we may remove it.
          //_workingDrumMapPatchList->remove(patch, other_index, WorkingDrumMapEntry::ENoteField);
          pdm.removeWorkingDrumMapEntry(other_index, WorkingDrumMapEntry::ENoteField);
        }
        else
        {
          // The values are not equal. This is technically still an override, so add (modify) it.
          other_dm.enote = cur_enote;
          WorkingDrumMapEntry other_wdme(other_dm, WorkingDrumMapEntry::ENoteField);
          //_workingDrumMapPatchList->add(patch, other_index, other_wdme);
          pdm.addWorkingDrumMapEntry(other_index, other_wdme);
        }
      }
      else
      {
        other_dm.enote = cur_enote;
        WorkingDrumMapEntry other_wdme(other_dm, WorkingDrumMapEntry::ENoteField);
        //_workingDrumMapPatchList->add(patch, other_index, other_wdme);
        pdm.addWorkingDrumMapEntry(other_index, other_wdme);
      }
      pdm.drum_in_map[cur_enote] = other_index;
      pdm.drum_in_map[new_enote] = index;
    }
  }




  return true;
}
#endif


//---------------------------------------------------------
//   getPatchName
//---------------------------------------------------------

QString MidiInstrument::getPatchName(int /*channel*/, int prog, bool drum, bool includeDefault) const
      {
  if(MusECore::Patch* p = pg.findPatch(prog, drum, includeDefault))
    return p->name;
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
                    int prog = mp->program;
                    int lbank = mp->lbank;
                    int hbank = mp->hbank;
                    tmp.push_back(dumb_patchlist_entry_t(prog,lbank,hbank));
                  }
            }
      }

      return tmp;
      }


//---------------------------------------------------------
//   patch_drummap_mapping_t
//---------------------------------------------------------

patch_drummap_mapping_t::patch_drummap_mapping_t()
{
  _patch = CTRL_PROGRAM_VAL_DONT_CARE;
  drummap=new DrumMap[128];
  for (int i=0;i<128;i++)
    drummap[i]=iNewDrumMap[i];
  update_drum_in_map();
}

patch_drummap_mapping_t::patch_drummap_mapping_t(const patch_drummap_mapping_t& that)
{
  drummap=NULL;
  if(that.drummap)
  {
    drummap=new DrumMap[128];
    for (int i=0;i<128;i++)
      drummap[i]=that.drummap[i];
  }

  _patch = that._patch;
  update_drum_in_map();
}

patch_drummap_mapping_t& patch_drummap_mapping_t::operator=(const patch_drummap_mapping_t& that)
{
  if (drummap)
    delete [] drummap;
  drummap=NULL;

  if(that.drummap)
  {
    drummap=new DrumMap[128];
    for (int i=0;i<128;i++)
      drummap[i]=that.drummap[i];
  }

  _patch = that._patch;

  update_drum_in_map();
 return *this;
}

bool patch_drummap_mapping_t::isValid() const
{
  return _patch != CTRL_VAL_UNKNOWN && drummap != NULL;
}

patch_drummap_mapping_t::~patch_drummap_mapping_t()
{
  if(drummap)
    delete [] drummap;
}

#ifdef _USE_INSTRUMENT_OVERRIDES_
void patch_drummap_mapping_t::addWorkingDrumMapEntry(int index,const WorkingDrumMapEntry& item)
{
  _workingDrumMapList.add(index, item);
}

void patch_drummap_mapping_t::removeWorkingDrumMapEntry(int index, const WorkingDrumMapEntry& item)
{
  _workingDrumMapList.remove(index, item);
}

void patch_drummap_mapping_t::removeWorkingDrumMapEntry(int index, int fields)
{
  _workingDrumMapList.remove(index, fields);
}
#endif

void patch_drummap_mapping_t::update_drum_in_map()
{
  if(drummap)
  {
    for(int i = 0; i < 128; ++i)
      drum_in_map[(int)drummap[i].enote] = i;
  }
  else
  {
    for(int i = 0; i < 128; ++i)
      drum_in_map[i] = i;
  }

#ifdef _USE_INSTRUMENT_OVERRIDES_
  int index;
  int enote;
  for(ciWorkingDrumMapPatch_t iwdmp = _workingDrumMapList.begin(); iwdmp != _workingDrumMapList.end(); ++iwdmp)
  {
    const WorkingDrumMapEntry& wde = iwdmp->second;
    if(wde._fields & WorkingDrumMapEntry::ENoteField)
    {
      index = iwdmp->first;
      const DrumMap& dm = wde._mapItem;
      enote = (int)dm.enote;
      drum_in_map[enote] = index;
    }
  }
#endif
}

bool patch_drummap_mapping_t::isPatchInRange(int patch, bool includeDefault) const
{
  // No exceptions: If all three prg, hb, and lb are don't care, then patch is always in range.
  if(dontCare())
    return includeDefault;

  // Special value. Unknown cannot be part of a collection (unless don't care).
  if(!isValid() || patch == CTRL_VAL_UNKNOWN)
    return false;

  const int hb = (patch >> 16) & 0xff;
  const int lb = (patch >> 8) & 0xff;
  const int pr = patch & 0xff;

  const bool hboff  = hb >= 128;
  const bool lboff  = lb >= 128;
  const bool prgoff = pr >= 128; // Shouldn't happen.

  return (programDontCare() || (!prgoff && pr == prog())) &&
         (hbankDontCare()   || (!hboff  && hb == hbank())) &&
         (lbankDontCare()   || (!lboff  && lb == lbank()));
}

QString patch_drummap_mapping_t::to_string()
{
  QString tmp;

  if (dontCare())
    tmp="default";
  else
  {
    if(hbankDontCare())
      tmp += "---";
    else
      tmp += QString::number(hbank() + 1);

    tmp+=" / ";

    if(lbankDontCare())
      tmp += "---";
    else
      tmp += QString::number(lbank() + 1);

    tmp+=" / ";

    if(programDontCare())
      tmp += "---";
    else
      tmp += QString::number(prog() + 1);
  }
  return tmp;
}

//---------------------------------------------------------
//   patch_drummap_mapping_t
//---------------------------------------------------------

void patch_drummap_mapping_list_t::add(const patch_drummap_mapping_list_t& other)
{
  for(ciPatchDrummapMapping_t ipdm = other.begin(); ipdm != other.end(); ++ipdm)
  {
    const patch_drummap_mapping_t& pdm = *ipdm;
    add(pdm);
  }
}

void patch_drummap_mapping_list_t::add(const patch_drummap_mapping_t& pdm)
{
  // No duplicates: If a mapping item by that patch already exists, replace it.
  iPatchDrummapMapping_t ipdm = find(pdm._patch, false); // No default.
  if(ipdm == end())
    push_back(pdm);
  else
    *ipdm = pdm;
}

iPatchDrummapMapping_t patch_drummap_mapping_list_t::find(int patch, bool includeDefault)
{
  iPatchDrummapMapping_t ipdm_default = end();
  for(iPatchDrummapMapping_t ipdm = begin(); ipdm != end(); ++ipdm)
  {
    // Look for an exact match above all else. The given patch must be valid.
    if(patch != CTRL_VAL_UNKNOWN && ipdm->_patch == patch)
      return ipdm;
    // If no exact match is found we'll take a default if found (all three pr, hb, lb = don't care).
    if(includeDefault && ipdm->dontCare() && ipdm_default == end())
      ipdm_default = ipdm;
  }
  return ipdm_default;
}

ciPatchDrummapMapping_t patch_drummap_mapping_list_t::find(int patch, bool includeDefault) const
{
  ciPatchDrummapMapping_t ipdm_default = end();
  for(ciPatchDrummapMapping_t ipdm = begin(); ipdm != end(); ++ipdm)
  {
    // Look for an exact match above all else. The given patch must be valid.
    if(patch != CTRL_VAL_UNKNOWN && ipdm->_patch == patch)
      return ipdm;
    // If no exact match is found we'll take a default if found (all three pr, hb, lb = don't care).
    if(includeDefault && ipdm->dontCare() && ipdm_default == end())
      ipdm_default = ipdm;
  }
  return ipdm_default;
}

void patch_drummap_mapping_list_t::read(Xml& xml)
{
  int patch = CTRL_PROGRAM_VAL_DONT_CARE;
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
        delete drummap;
        return;

      case Xml::TagStart:
        if (tag == "patch_collection")
          patch = readDrummapsEntryPatchCollection(xml);
        else if (tag == "drummap")
          read_new_style_drummap(xml, "drummap", drummap);
        else
          xml.unknown("patch_drummap_mapping_list_t::read");
        break;

      case Xml::TagEnd:
        if (tag == "entry")
        {
          push_back(patch_drummap_mapping_t(drummap, patch));
          return;
        }

      default:
        break;
    }
  }
  printf("ERROR: THIS CANNOT HAPPEN: exited infinite loop in patch_drummap_mapping_list_t::read()!\n"
         "                           not returning anything. expect undefined behaviour or even crashes.\n");
  delete drummap;
  return;
}

void patch_drummap_mapping_list_t::write(int level, Xml& xml) const
{
  for (ciPatchDrummapMapping_t it = begin();
       it != end(); it++)
  {
    xml.tag(level++, "entry");

    const patch_drummap_mapping_t& pdm = *it;

    if(!pdm.dontCare())
    {
      QString tmp="<patch_collection ";

      if(!pdm.programDontCare())
        tmp += "prog=\"" + QString::number(pdm.prog()) + QString("\" ");
      if(!pdm.lbankDontCare())
        tmp += "lbank=\"" + QString::number(pdm.lbank()) + QString("\" ");
      if(!pdm.hbankDontCare())
        tmp += "hbank=\"" + QString::number(pdm.hbank()) + QString("\" ");

      tmp+="/>\n";

      xml.nput(level, tmp.toLatin1().data());
    }

    write_new_style_drummap(level, xml, "drummap", it->drummap);
    //write_new_style_drummap(level, xml, "drummap", it->drummap, true); // true = Need to save all entries.

    xml.etag(--level, "entry");
  }
}

#ifdef _USE_INSTRUMENT_OVERRIDES_
void patch_drummap_mapping_list_t::writeDrummapOverrides(int level, Xml& xml) const
{
  for(ciPatchDrummapMapping_t ipdm = begin(); ipdm != end(); ++ipdm)
  {
    const patch_drummap_mapping_t& pdm = *ipdm;
    if(pdm._workingDrumMapList.empty())
      continue;
    xml.tag(level++, "drumMapPatch patch=\"%d\"", pdm._patch);
    pdm._workingDrumMapList.write(level, xml);
    xml.etag(--level, "drumMapPatch");
  }
}
#endif

//---------------------------------------------------------
//    WorkingDrumMapEntry
//---------------------------------------------------------

WorkingDrumMapEntry::WorkingDrumMapEntry()
{
  _fields = NoField;
}

WorkingDrumMapEntry::WorkingDrumMapEntry(const DrumMap& dm, fields_t fields)
{
  _fields = fields;
  _mapItem = dm;
}

WorkingDrumMapEntry::WorkingDrumMapEntry(const WorkingDrumMapEntry& other)
{
  _fields = other._fields;
  _mapItem = other._mapItem;
}

WorkingDrumMapEntry& WorkingDrumMapEntry::operator=(const WorkingDrumMapEntry& other)
{
  _fields = other._fields;
  _mapItem = other._mapItem;
  return *this;
}

//---------------------------------------------------------
//    WorkingDrumMapList
//---------------------------------------------------------

void WorkingDrumMapList::add(int index, const WorkingDrumMapEntry& item)
{
  WorkingDrumMapPatchInsertResult_t res = insert(WorkingDrumMapPatchInsertPair_t(index, item));
  if(res.second == false)
  {
    iWorkingDrumMapPatch_t& iwp = res.first;
    WorkingDrumMapEntry& wde = iwp->second;

    if(item._fields & WorkingDrumMapEntry::NameField)
      wde._mapItem.name = item._mapItem.name;

    if(item._fields & WorkingDrumMapEntry::VolField)
      wde._mapItem.vol = item._mapItem.vol;

    if(item._fields & WorkingDrumMapEntry::QuantField)
      wde._mapItem.quant = item._mapItem.quant;

    if(item._fields & WorkingDrumMapEntry::LenField)
      wde._mapItem.len = item._mapItem.len;

    if(item._fields & WorkingDrumMapEntry::ChanField)
      wde._mapItem.channel = item._mapItem.channel;

    if(item._fields & WorkingDrumMapEntry::PortField)
      wde._mapItem.port = item._mapItem.port;

    if(item._fields & WorkingDrumMapEntry::Lv1Field)
      wde._mapItem.lv1 = item._mapItem.lv1;

    if(item._fields & WorkingDrumMapEntry::Lv2Field)
      wde._mapItem.lv2 = item._mapItem.lv2;

    if(item._fields & WorkingDrumMapEntry::Lv3Field)
      wde._mapItem.lv3 = item._mapItem.lv3;

    if(item._fields & WorkingDrumMapEntry::Lv4Field)
      wde._mapItem.lv4 = item._mapItem.lv4;

    if(item._fields & WorkingDrumMapEntry::ENoteField)
      wde._mapItem.enote = item._mapItem.enote;

    if(item._fields & WorkingDrumMapEntry::ANoteField)
      wde._mapItem.anote = item._mapItem.anote;

    if(item._fields & WorkingDrumMapEntry::MuteField)
      wde._mapItem.mute = item._mapItem.mute;

    if(item._fields & WorkingDrumMapEntry::HideField)
      wde._mapItem.hide = item._mapItem.hide;

    wde._fields |= item._fields;
  }
}

int WorkingDrumMapList::remove(int index, const WorkingDrumMapEntry& item)
{
  return remove(index, item._fields);
}

int WorkingDrumMapList::remove(int index, int fields)
{
  iWorkingDrumMapPatch_t iwp = find(index);
  if(iwp == end())
    return fields;
  WorkingDrumMapEntry& wde = iwp->second;
  int ret = wde._fields ^ fields;
  wde._fields &= ~fields;
  ret ^= wde._fields;
  if(wde._fields == WorkingDrumMapEntry::NoField)
    erase(iwp);
  return ret;
}

void WorkingDrumMapList::read(Xml& xml, bool fillUnused, int defaultIndex)
{
  const QString start_tag = xml.s1();
  int index = defaultIndex;
  int index_read;
  bool enote_read = false;
  bool anote_read = false;
  bool ok;
  WorkingDrumMapEntry wdme;
  if(fillUnused)
  {
    // Must initialize the map item in case some fields aren't given.
    wdme._mapItem.init();
    // Technically we are overriding all fields even if some are not given.
    wdme._fields = WorkingDrumMapEntry::AllFields;
  }

  for (;;) {
        Xml::Token token = xml.parse();
        const QString& tag = xml.s1();
        switch (token) {
              case Xml::Error:
              case Xml::End:
                    return;
              case Xml::TagStart:
                    if (tag == "name")
                    {
                          wdme._mapItem.name = xml.parse1();
                          wdme._fields |= WorkingDrumMapEntry::NameField;
                    }
                    else if (tag == "vol")
                    {
                          wdme._mapItem.vol = xml.parseInt();
                          wdme._fields |= WorkingDrumMapEntry::VolField;
                    }
                    else if (tag == "quant")
                    {
                          wdme._mapItem.quant = xml.parseInt();
                          wdme._fields |= WorkingDrumMapEntry::QuantField;
                    }
                    else if (tag == "len")
                    {
                          wdme._mapItem.len = xml.parseInt();
                          wdme._fields |= WorkingDrumMapEntry::LenField;
                    }
                    else if (tag == "channel")
                    {
                          wdme._mapItem.channel = xml.parseInt();
                          wdme._fields |= WorkingDrumMapEntry::ChanField;
                    }
                    else if (tag == "port")
                    {
                          wdme._mapItem.port = xml.parseInt();
                          wdme._fields |= WorkingDrumMapEntry::PortField;
                    }
                    else if (tag == "lv1")
                    {
                          wdme._mapItem.lv1 = xml.parseInt();
                          wdme._fields |= WorkingDrumMapEntry::Lv1Field;
                    }
                    else if (tag == "lv2")
                    {
                          wdme._mapItem.lv2 = xml.parseInt();
                          wdme._fields |= WorkingDrumMapEntry::Lv2Field;
                    }
                    else if (tag == "lv3")
                    {
                          wdme._mapItem.lv3 = xml.parseInt();
                          wdme._fields |= WorkingDrumMapEntry::Lv3Field;
                    }
                    else if (tag == "lv4")
                    {
                          wdme._mapItem.lv4 = xml.parseInt();
                          wdme._fields |= WorkingDrumMapEntry::Lv4Field;
                    }
                    else if (tag == "enote")
                    {
                          wdme._mapItem.enote = xml.parseInt();
                          enote_read = true;
                          wdme._fields |= WorkingDrumMapEntry::ENoteField;
                    }
                    else if (tag == "anote")
                    {
                          wdme._mapItem.anote = xml.parseInt();
                          anote_read = true;
                          wdme._fields |= WorkingDrumMapEntry::ANoteField;
                    }
                    else if (tag == "mute")
                    {
                          wdme._mapItem.mute = xml.parseInt();
                          wdme._fields |= WorkingDrumMapEntry::MuteField;
                    }
                    else if (tag == "hide")
                    {
                          wdme._mapItem.hide = xml.parseInt();
                          wdme._fields |= WorkingDrumMapEntry::HideField;
                    }
                    else
                      xml.unknown(start_tag.toLatin1().constData());
                    break;
              case Xml::Attribut:
                    if (tag == "idx" || tag == "pitch")
                    {
                      index_read = xml.s2().toInt(&ok);
                      if(ok)
                        index = index_read;
                    }
                    break;
              case Xml::TagEnd:
                    if (tag == start_tag)
                    {
                      if(index >= 0 && index < 128)
                      {
                        // If no enote was given, set it to the index.
                        if(fillUnused)
                        {
                          if(!enote_read)
                            wdme._mapItem.enote = index;
                          // If no anote was given, set it to the enote.
                          if(!anote_read)
                            wdme._mapItem.anote = wdme._mapItem.enote;
                        }
                        insert(WorkingDrumMapPatchInsertPair_t(index, wdme));
                      }
                      return;
                    }
              default:
                    break;
              }
        }
}

void WorkingDrumMapList::write(int level, Xml& xml) const
{
  int index;
  for(ciWorkingDrumMapPatch_t iwdp = begin(); iwdp != end(); ++iwdp)
  {
    index = iwdp->first;
    xml.tag(level++, "entry idx=\"%d\"", index);

    const WorkingDrumMapEntry& wde = iwdp->second;

    if(wde._fields & WorkingDrumMapEntry::NameField)
      xml.strTag(level, "name", wde._mapItem.name);

    if(wde._fields & WorkingDrumMapEntry::VolField)
      xml.intTag(level, "vol", wde._mapItem.vol);

    if(wde._fields & WorkingDrumMapEntry::QuantField)
      xml.intTag(level, "quant", wde._mapItem.quant);

    if(wde._fields & WorkingDrumMapEntry::LenField)
      xml.intTag(level, "len", wde._mapItem.len);

    if(wde._fields & WorkingDrumMapEntry::ChanField)
      xml.intTag(level, "channel", wde._mapItem.channel);

    if(wde._fields & WorkingDrumMapEntry::PortField)
      xml.intTag(level, "port", wde._mapItem.port);

    if(wde._fields & WorkingDrumMapEntry::Lv1Field)
      xml.intTag(level, "lv1", wde._mapItem.lv1);

    if(wde._fields & WorkingDrumMapEntry::Lv2Field)
      xml.intTag(level, "lv2", wde._mapItem.lv2);

    if(wde._fields & WorkingDrumMapEntry::Lv3Field)
      xml.intTag(level, "lv3", wde._mapItem.lv3);

    if(wde._fields & WorkingDrumMapEntry::Lv4Field)
      xml.intTag(level, "lv4", wde._mapItem.lv4);

    if(wde._fields & WorkingDrumMapEntry::ENoteField)
      xml.intTag(level, "enote", wde._mapItem.enote);

    if(wde._fields & WorkingDrumMapEntry::ANoteField)
      xml.intTag(level, "anote", wde._mapItem.anote);

    if(wde._fields & WorkingDrumMapEntry::MuteField)
      xml.intTag(level, "mute", wde._mapItem.mute);

    if(wde._fields & WorkingDrumMapEntry::HideField)
      xml.intTag(level, "hide", wde._mapItem.hide);

    xml.tag(--level, "/entry");
  }
}


//---------------------------------------------------------
//    WorkingDrumMapInstrumentList
//---------------------------------------------------------

void WorkingDrumMapInstrumentList::read(Xml& xml)
{
  const QString start_tag = xml.s1();
  QString instr_name;
  WorkingDrumMapPatchList wdmpl;
  for (;;) {
        Xml::Token token = xml.parse();
        const QString& tag = xml.s1();
        switch (token) {
              case Xml::Error:
              case Xml::End:
                    return;
              case Xml::TagStart:
                    if (tag == "drumMapPatch")
                      // false = Do not fill in unused items.
                      wdmpl.read(xml, false);
                    else
                      xml.unknown(start_tag.toLatin1().constData());
                    break;
              case Xml::Attribut:
                    if (tag == "instrument")
                    {
                      instr_name = xml.s2();
                    }
                    break;
              case Xml::TagEnd:
                    if (tag == start_tag)
                    {
                      if(!instr_name.isEmpty() && !wdmpl.empty())
                        insert(WorkingDrumMapInstrumentListInsertPair_t(instr_name.toStdString(), wdmpl));
                      return;
                    }
              default:
                    break;
              }
        }
}


//---------------------------------------------------------
//    WorkingDrumMapPatchList
//---------------------------------------------------------

void WorkingDrumMapPatchList::add(const WorkingDrumMapPatchList& other)
{
  int patch;
  int index;
  for(ciWorkingDrumMapPatchList_t iwdmpl = other.begin(); iwdmpl != other.end(); ++iwdmpl)
  {
    patch = iwdmpl->first;
    const WorkingDrumMapList& wdml = iwdmpl->second;
    WorkingDrumMapPatchListInsertResult_t res = insert(WorkingDrumMapPatchListInsertPair_t(patch, wdml));
    iWorkingDrumMapPatchList_t res_iwdmpl = res.first;
    if(res_iwdmpl == end()) // Error.
      continue;
    WorkingDrumMapList& res_wdml = res_iwdmpl->second;
    for(iWorkingDrumMapPatch_t res_iwdp = res_wdml.begin(); res_iwdp != res_wdml.end(); ++res_iwdp)
    {
      index = res_iwdp->first;
      WorkingDrumMapEntry& wdme = res_iwdp->second;
      res_wdml.add(index, wdme);
    }
  }
}

void WorkingDrumMapPatchList::add(int patch, const WorkingDrumMapList& list)
{
  insert(WorkingDrumMapPatchListInsertPair_t(patch, list));
}

void WorkingDrumMapPatchList::add(int patch, int index, const WorkingDrumMapEntry& item)
{
  WorkingDrumMapPatchListInsertResult_t res = insert(WorkingDrumMapPatchListInsertPair_t(patch, WorkingDrumMapList()));
  iWorkingDrumMapPatchList_t iwdmpl = res.first;
  if(iwdmpl == end())  // Error, should exist.
    return;
  WorkingDrumMapList& wdml = iwdmpl->second;
  wdml.add(index, item);
}

void WorkingDrumMapPatchList::remove(int patch, int index, const WorkingDrumMapEntry& item, bool includeDefault)
{
  remove(patch, index, item._fields, includeDefault);
}

void WorkingDrumMapPatchList::remove(int patch, int index, int fields, bool includeDefault)
{
  // Remove requested fields from the exact patch number first.
  iWorkingDrumMapPatchList_t iwdmpl = WorkingDrumMapPatchList_t::find(patch);
  if(iwdmpl != end())
  {
    WorkingDrumMapList& wdml = iwdmpl->second;
    // Consider defaults and real patch overrides as part of the same deal,
    //  remove all requested fields from BOTH.
    //fields = wdml.remove(index, fields);
    wdml.remove(index, fields);
    // No more items in the list? Remove this container list.
    if(wdml.empty())
      erase(iwdmpl);
  }

  // Consider defaults and real patch overrides as part of the same deal,
  //  remove all requested fields from BOTH.
  //if(includeDefault && fields != WorkingDrumMapEntry::NoField)
  if(includeDefault)
  {
    iwdmpl = WorkingDrumMapPatchList_t::find(CTRL_PROGRAM_VAL_DONT_CARE);
    if(iwdmpl != end())
    {
      WorkingDrumMapList& wdml = iwdmpl->second;
      wdml.remove(index, fields);
      // No more items in the list? Remove this container list.
      if(wdml.empty())
        erase(iwdmpl);
    }
  }
}

void WorkingDrumMapPatchList::remove(int patch, bool includeDefault)
{
  // Remove requested fields from the exact patch number first.
  iWorkingDrumMapPatchList_t iwdmpl = WorkingDrumMapPatchList_t::find(patch);
  if(iwdmpl != end())
    erase(iwdmpl);
  // Patch map not found? Look for a default patch number (all three pr, hb, lb = don't care).
  else if(includeDefault)
  {
    iwdmpl = WorkingDrumMapPatchList_t::find(CTRL_PROGRAM_VAL_DONT_CARE);
    if(iwdmpl != end())
      erase(iwdmpl);
  }
}

WorkingDrumMapList* WorkingDrumMapPatchList::find(int patch, bool includeDefault)
{
  // Look for an exact match above all else. The given patch must be valid.
  iWorkingDrumMapPatchList_t iwdmpl = WorkingDrumMapPatchList_t::find(patch);
  // If no exact match is found we'll take a default if found (all three pr, hb, lb = don't care).
  if(iwdmpl == end())
  {
    if(!includeDefault)
      return NULL;
    iwdmpl = WorkingDrumMapPatchList_t::find(CTRL_PROGRAM_VAL_DONT_CARE);
    if(iwdmpl == end())
      return NULL;
  }
  return &iwdmpl->second;
}

const WorkingDrumMapList* WorkingDrumMapPatchList::find(int patch, bool includeDefault) const
{
  // Look for an exact match above all else. The given patch must be valid.
  ciWorkingDrumMapPatchList_t iwdmpl = WorkingDrumMapPatchList_t::find(patch);
  // If no exact match is found we'll take a default if found (all three pr, hb, lb = don't care).
  if(iwdmpl == end())
  {
    if(!includeDefault)
      return NULL;
    iwdmpl = WorkingDrumMapPatchList_t::find(CTRL_PROGRAM_VAL_DONT_CARE);
    if(iwdmpl == end())
      return NULL;
  }
  return &iwdmpl->second;
}

WorkingDrumMapEntry* WorkingDrumMapPatchList::find(int patch, int index, bool includeDefault)
{
  WorkingDrumMapList* wdmpl = find(patch, includeDefault);
  if(!wdmpl)
    return NULL;
  iWorkingDrumMapPatch_t iwdmp = wdmpl->find(index);
  if(iwdmp == wdmpl->end())
    return NULL;
  return &iwdmp->second;
}

const WorkingDrumMapEntry* WorkingDrumMapPatchList::find(int patch, int index, bool includeDefault) const
{
  const WorkingDrumMapList* wdmpl = find(patch, includeDefault);
  if(!wdmpl)
    return NULL;
  ciWorkingDrumMapPatch_t iwdmp = wdmpl->find(index);
  if(iwdmp == wdmpl->end())
    return NULL;
  return &iwdmp->second;
}

void WorkingDrumMapPatchList::read(Xml& xml, bool fillUnused)
{
  const QString start_tag = xml.s1();
  // Default "don't care" patch number, in case no patch number found.
  int patch = 0xffffff;
  int patch_read;
  bool ok;
  int index = 0;
  WorkingDrumMapList wdml;

// TODO Need to move this stuff up into the caller, because a default patch map may not have been loaded yet!
// For now, we rely on the loaded map being trustworthy with no duplicate enotes. Still the situation IS compensated
//  for at the lowest level in MidiTrack::normalizeDrumMap(), so it IS tolerant of mistakes in the loaded map.
//
// REMOVE Tim. newdrums. Removed.
//   WorkingDrumMapList* def_wdml = 0;
//   //if(patch != CTRL_PROGRAM_VAL_DONT_CARE)
//     def_wdml = find(CTRL_PROGRAM_VAL_DONT_CARE, false);
//   WorkingDrumMapEntry new_wdme;
//   // We can init these outside of the loop.
//   new_wdme._fields = WorkingDrumMapEntry::AllFields;
//   new_wdme._mapItem.init();
//
//   bool used_index[128];
//   int used_enotes[128];
//   for(int i = 0; i < 128; ++i)
//   {
//     used_index[i] = false;
//     used_enotes[i] = 0;
//   }
//   char unused_enotes[128];
//   int unused_enotes_sz = 0;
//   char unused_index[128];
//   int unused_index_sz = 0;

  for (;;) {
        Xml::Token token = xml.parse();
        const QString& tag = xml.s1();
        switch (token) {
              case Xml::Error:
              case Xml::End:
                    return;
              case Xml::TagStart:
                    if (tag == "entry")
                    {
                      // In case there are no index attributes in this drum map,
                      //  we use a running index.
                      wdml.read(xml, fillUnused, index);
                      ++index;
                    }
                    else if (tag == "comment")
                      xml.parse();
                    else
                      xml.unknown(start_tag.toLatin1().constData());
                    break;
              case Xml::Attribut:
                    if (tag == "patch")
                    {
                      patch_read = xml.s2().toInt(&ok);
                      if(ok)
                        patch = patch_read;
                    }
                    break;
              case Xml::TagEnd:
                    if (tag == start_tag)
                    {

                      if(!wdml.empty())
                      {
// //                         // We can only deal with duplicate enotes here if requesting to
// //                         //  fill up all items, because in the context of further overrides
// //                         //  masking any unused ones here, we cannot fully know which enotes are used.
//                         //if(fillUnused)
//                         //{
//                           // Find all the used enotes and indexes.
//                           for(iWorkingDrumMapPatch_t iwdml = wdml.begin(); iwdml != wdml.end(); ++iwdml)
//                           {
//                             used_index[iwdml->first] = true;
//                             ++used_enotes[(unsigned char)iwdml->second._mapItem.enote];
//                           }
//
//                           // Find all the unused enotes and indexes.
//                           for(int i = 0; i < 128; ++i)
//                           {
//                             if(!used_index[i])
//                               unused_index[unused_index_sz++] = i;
//                             if(used_enotes[i] == 0)
//                               unused_enotes[unused_enotes_sz++] = i;
//                           }
//
//                           // Ensure there are NO duplicate enotes in the existing map items so far.
//                           int unused_enotes_cnt = 0;
//                           for(iWorkingDrumMapPatch_t iwdml = wdml.begin(); iwdml != wdml.end(); ++iwdml)
//                           {
//                             // More than 1 (this) usage?
//                             if(used_enotes[(unsigned char)iwdml->second._mapItem.enote] > 1)
//                             {
//                               if(unused_enotes_cnt >= unused_enotes_sz)
//                               {
//                                 fprintf(stderr, "WorkingDrumMapPatchList::read: Error: unused_enotes_cnt >= unused_enotes_sz:%d\n",
//                                         unused_enotes_sz);
//                                 break;
//                               }
//                               --used_enotes[(unsigned char)iwdml->second._mapItem.enote];
//                               iwdml->second._mapItem.enote = unused_enotes[unused_enotes_cnt++];
//                             }
//                           }
//
//                           // Technically we are overriding the entire map, even if some map items weren't given.
//                           // In case of loading a partial or incomplete map, ensure that all 128 map items are filled.
//                           for(int i = 0; i < unused_index_sz; ++i)
//                           {
//                             if(unused_enotes_cnt >= unused_enotes_sz)
//                             {
//                               fprintf(stderr, "WorkingDrumMapPatchList::read: Error filling unused items: unused_enotes_cnt >= unused_enotes_sz:%d\n",
//                                       unused_enotes_sz);
//                               break;
//                             }
//                             // Set the enote.
//                             new_wdme._mapItem.enote = unused_enotes[unused_enotes_cnt++];
//                             // Might as well set the anote to enote.
//                             new_wdme._mapItem.anote = new_wdme._mapItem.enote;
//                             // Insert the item at the unused index.
//                             wdml.insert(WorkingDrumMapPatchInsertPair_t(unused_index[i], new_wdme));
//                           }
//                         //}

                        // Insert the working drum map list at the patch.
                        insert(WorkingDrumMapPatchListInsertPair_t(patch, wdml));
                      }
                      return;
                    }
              default:
                    break;
              }
        }
}

void WorkingDrumMapPatchList::write(int level, Xml& xml) const
{
  int patch;
  for(ciWorkingDrumMapPatchList_t iwdpl = begin(); iwdpl != end(); ++iwdpl)
  {
    const WorkingDrumMapList& wdml = iwdpl->second;
    if(wdml.empty())
      continue;
    patch = iwdpl->first;
    xml.tag(level++, "drumMapPatch patch=\"%d\"", patch);
    wdml.write(level, xml);
    xml.etag(--level, "drumMapPatch");
  }
}


//---------------------------------------------------------
//    ChannelDrumMappingList
//---------------------------------------------------------

ChannelDrumMappingList::ChannelDrumMappingList()
{
  // Ensure there is always a default channel.
  // Initialize with a default drum map on default channel. Patch is default 0xffffff. GM-1 does not specify a drum patch number.
  add(-1, patch_drummap_mapping_list_t());
}

void ChannelDrumMappingList::add(const ChannelDrumMappingList& other)
{
  int channel;

  for(ciChannelDrumMappingList_t icdml = other.begin(); icdml != other.end(); ++icdml)
  {
    channel = icdml->first;
    const patch_drummap_mapping_list_t& pdml = icdml->second;
    add(channel, pdml);
  }
}

void ChannelDrumMappingList::add(int channel, const patch_drummap_mapping_list_t& list)
{
  ChannelDrumMappingListInsertResult_t res = insert(ChannelDrumMappingListInsertPair_t(channel, list));
  if(res.second == false)
  {
    iChannelDrumMappingList_t res_icdml = res.first;
    patch_drummap_mapping_list_t& res_pdml = res_icdml->second;
    res_pdml.add(list);
  }
}

patch_drummap_mapping_list_t* ChannelDrumMappingList::find(int channel, bool includeDefault)
{
  // Look for an exact match above all else. The given channel must be valid.
  iChannelDrumMappingList_t icdml = ChannelDrumMappingList_t::find(channel);
  // If no exact match is found we'll take a default if found.
  if(icdml == end())
  {
    if(!includeDefault)
      return NULL;
    icdml = ChannelDrumMappingList_t::find(-1);
    if(icdml == end())
      return NULL;
  }
  return &icdml->second;
}

const patch_drummap_mapping_list_t* ChannelDrumMappingList::find(int channel, bool includeDefault) const
{
  // Look for an exact match above all else. The given channel must be valid.
  ciChannelDrumMappingList_t icdml = ChannelDrumMappingList_t::find(channel);
  // If no exact match is found we'll take a default if found.
  if(icdml == end())
  {
    if(!includeDefault)
      return NULL;
    icdml = ChannelDrumMappingList_t::find(-1);
    if(icdml == end())
      return NULL;
  }
  return &icdml->second;
}

void ChannelDrumMappingList::read(Xml& xml)
{
  const QString start_tag = xml.s1();
  // Default "don't care" channel number, in case no channel number found.
  int channel = -1; // Default.
  int channel_read;
  bool ok;

  for (;;) {
        Xml::Token token = xml.parse();
        const QString& tag = xml.s1();
        switch (token) {
              case Xml::Error:
              case Xml::End:
                    return;
              case Xml::TagStart:
                    if (tag == "entry")
                    {
                      patch_drummap_mapping_list_t pdml;
                      pdml.read(xml);
                      if(!pdml.empty())
                        add(channel, pdml);
                    }
                    else if (tag == "comment")
                      xml.parse();
                    else
                      xml.unknown(start_tag.toLatin1().constData());
                    break;
              case Xml::Attribut:
                    if (tag == "channel")
                    {
                      channel_read = xml.s2().toInt(&ok);
                      if(ok)
                        channel = channel_read;
                    }
                    break;
              case Xml::TagEnd:
                    if (tag == start_tag)
                      return;
              default:
                    break;
              }
        }
}

void ChannelDrumMappingList::write(int level, Xml& xml) const
{
  int channel;

  // Count the items used.
  int sz = 0;
  for(ciChannelDrumMappingList_t icdml = begin(); icdml != end(); ++icdml)
  {
    const patch_drummap_mapping_list_t& pdml = icdml->second;
    if(pdml.empty())
      continue;
    ++sz;
  }

  for(ciChannelDrumMappingList_t icdml = begin(); icdml != end(); ++icdml)
  {
    const patch_drummap_mapping_list_t& pdml = icdml->second;
    if(pdml.empty())
      continue;
    channel = icdml->first;

    // Don't bother with the drumMapChannel tag if not required.
    if(sz >= 2 || channel != -1) // -1 is default.
      xml.tag(level++, "drumMapChannel channel=\"%d\"", channel);

    pdml.write(level, xml);

    if(sz >= 2 || channel != -1) // -1 is default.
      xml.etag(--level, "drumMapChannel");
  }
}

} // namespace MusECore
