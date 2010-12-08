//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: minstrument.cpp,v 1.10.2.5 2009/03/28 01:46:10 terminator356 Exp $
//
//  (C) Copyright 2000-2003 Werner Schweer (ws@seh.de)
//=========================================================

#include <stdio.h>

#include <QDir>
#include <QFileInfo>
#include <QAction>

#include "minstrument.h"
#include "midiport.h"
#include "globals.h"
#include "xml.h"
#include "event.h"
#include "mpevent.h"
#include "midictrl.h"
#include "gconfig.h"

MidiInstrumentList midiInstruments;
MidiInstrument* genericMidiInstrument;

static const char* gmdrumname = "GM-drums";

//---------------------------------------------------------
//   string2sysex
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
/*      
      QFile qf(fi->filePath());
      if (!qf.open(IO_ReadOnly)) {
            printf("cannot open file %s\n", fi->fileName().toLatin1());
            return;
            }
      if (debugMsg)
            printf("   load instrument definition <%s>\n", fi->filePath().local8Bit().data());
      QDomDocument doc;
      int line, column;
      QString err;
      if (!doc.setContent(&qf, false, &err, &line, &column)) {
            QString col, ln, error;
            col.setNum(column);
            ln.setNum(line);
            error = err + " at line: " + ln + " col: " + col;
            printf("error reading file <%s>:\n   %s\n",
               fi->filePath().toLatin1(), error.toLatin1());
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
                  printf("MusE:laodIDF: %s not supported\n", e.tagName().toLatin1());
            node = node.nextSibling();
            }
      qf.close();
*/      
      
      FILE* f = fopen(fi->filePath().toAscii().constData(), "r");
      if (f == 0)
            return;
      if (debugMsg)
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
      if (debugMsg)
        printf("load user instrument definitions from <%s>\n", museUserInstruments.toLatin1().constData());
      QDir usrInstrumentsDir(museUserInstruments, QString("*.idf"));
      if (usrInstrumentsDir.exists()) {
            QFileInfoList list = usrInstrumentsDir.entryInfoList();
            QFileInfoList::iterator it=list.begin(); // ddskrjo
            while(it != list.end()) { // ddskrjo
                  loadIDF(&*it);
                  ++it;
                  }
            }
      //else
      //{
      //  if(usrInstrumentsDir.mkdir(museUserInstruments))
      //    printf("Created user instrument directory: %s\n", museUserInstruments.toLatin1());
      //  else
      //    printf("Unable to create user instrument directory: %s\n", museUserInstruments.toLatin1());
      //}
      
      if (debugMsg)
        printf("load instrument definitions from <%s>\n", museInstruments.toLatin1().constData());
      QDir instrumentsDir(museInstruments, QString("*.idf"));
      if (instrumentsDir.exists()) {
            QFileInfoList list = instrumentsDir.entryInfoList();
            QFileInfoList::iterator it=list.begin(); // ddskrjo
            while(it!=list.end()) {
                  loadIDF(&*it);
                  ++it;
                  }
            }
      else
        printf("Instrument directory not found: %s\n", museInstruments.toLatin1().constData());
        
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
      }

/*
//---------------------------------------------------------
//   uniqueCopy
//---------------------------------------------------------

MidiInstrument& MidiInstrument::uniqueCopy(const MidiInstrument& ins)
{
  _initScript = 0;
  _midiInit  = new EventList();
  _midiReset = new EventList();
  _midiState = new EventList();
  //---------------------------------------------------------
  // TODO: Copy the init script, and the lists. 
  //---------------------------------------------------------
  _controller = new MidiControllerList(*(ins._controller));
    
  // Assignment
  pg = ins.pg;
  
  _name = ins._name;
  _filePath = ins._filePath;
    
  // Hmm, dirty, yes? But init sets it to false...
  //_dirty = ins._dirty;
  //_dirty = false;
  _dirty = true;
  
  return *this;
}
*/

//---------------------------------------------------------
//   assign
//---------------------------------------------------------

MidiInstrument& MidiInstrument::assign(const MidiInstrument& ins)
{
  //---------------------------------------------------------
  // TODO: Copy the _initScript, and _midiInit, _midiReset, and _midiState lists. 
  //---------------------------------------------------------
  
  for(iMidiController i = _controller->begin(); i != _controller->end(); ++i) 
      delete i->second;
  _controller->clear();
  
  _nullvalue = ins._nullvalue;
  
  // Assignment
  // *_controller = *(ins._controller);
  for(ciMidiController i = ins._controller->begin(); i != ins._controller->end(); ++i)
  {
    MidiController* mc = i->second;
    _controller->add(new MidiController(*mc));
  }  
  
//  pg.clear();
//  for(iPatchGroup ipg = pg.begin(); ipg != pg.end(); ++ipg)
//  {
    //ipg->patches.clear();
    
    //const PatchGroup& g = *ipg;
    //for(ciPatch ip = ipg->begin(); ip != ipg->end(); ++ipg)
    //{
    
    //}
//  }
  
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
//  pg = ins.pg;
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
      np->typ = pp->typ;  
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
    
  // Hmm, dirty, yes? But init sets it to false...
  //_dirty = ins._dirty;
  //_dirty = false;
  //_dirty = true;
  
  return *this;
}

//---------------------------------------------------------
//   reset
//    send note off to all channels
//---------------------------------------------------------

void MidiInstrument::reset(int portNo, MType)
      {
      MidiPlayEvent ev;
      ev.setType(0x90);
      MidiPort* port = &midiPorts[portNo];
      if (port == 0)
            return;
      ev.setPort(portNo);
      for (int chan = 0; chan < MIDI_CHANNELS; ++chan) {
            ev.setChannel(chan);
            for (int pitch = 0; pitch < 128; ++pitch) {
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
      typ   = -1;
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
                        else if (tag == "mode")
                              typ = xml.s2().toInt();
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
      //if (drumMap == 0) 
      //{
            //QString s = QString("Patch name=\"%1\"").arg(Xml::xmlString(name));
            //if (typ != -1)
            //      s += QString(" mode=\"%d\"").arg(typ);
            //s += QString(" hbank=\"%1\" lbank=\"%2\" prog=\"%3\"").arg(hbank).arg(lbank).arg(prog);
            //xml.tagE(s);
            xml.nput(level, "<Patch name=\"%s\"", Xml::xmlString(name).toLatin1().constData());
            if(typ != -1)
              xml.nput(" mode=\"%d\"", typ);
            
            if(hbank != -1)
              xml.nput(" hbank=\"%d\"", hbank);

            if(lbank != -1)
              xml.nput(" lbank=\"%d\"", lbank);
            
            xml.nput(" prog=\"%d\"", prog);
            
            //xml.nput(level, " hbank=\"%d\" lbank=\"%d\" prog=\"%d\"", hbank, lbank, prog);
            if(drum)
              //xml.nput(level, " drum=\"%d\"", int(drum));
              xml.nput(" drum=\"%d\"", int(drum));
            //xml.put(level, " />");
            xml.put(" />");
            
            //return;
      //}
      
      //QString s = QString("drummap name=\"%1\"").arg(Xml::xmlString(name));
      //s += QString(" hbank=\"%1\" lbank=\"%2\" prog=\"%3\"").arg(hbank).arg(lbank).arg(prog);
      //xml.stag(s);
      //for (int i = 0; i < DRUM_MAPSIZE; ++i) {
      //      DrumMapEntry* dm = drumMap->entry(i);
      //      dm->write(xml);
      //      }
      //xml.etag("drummap");
      }

//---------------------------------------------------------
//   readMidiState
//---------------------------------------------------------

void MidiInstrument::readMidiState(Xml& xml)
      {
      _midiState->read(xml, "midistate", true);
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
                              // Added by Tim. Copied from muse 2.
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
                              int len = strlen(istr) +1;
                              if (len > 1) {
                                    _initScript = new char[len];
                                    memcpy(_initScript, istr, len);
                                    }
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
      //xml.stag("muse version=\"2.1\"");
      xml.tag(level, "muse version=\"1.0\"");
      //xml.stag(QString("MidiInstrument name=\"%1\"").arg(Xml::xmlString(iname())));
      level++;
      //xml.tag(level, "MidiInstrument name=\"%s\"", Xml::xmlString(iname()).toLatin1().constData());
      xml.nput(level, "<MidiInstrument name=\"%s\"", Xml::xmlString(iname()).toLatin1().constData());
      
      if(_nullvalue != -1)
      {
        QString nv; 
        nv.setNum(_nullvalue);
        xml.nput(" nullparam=\"%s\"", nv.toLatin1().constData());
      }  
      xml.put(">");

      // -------------
      // What about Init, Reset, State, and InitScript ?
      // -------------
      
      //std::vector<PatchGroup>* pg = groups();
      //for (std::vector<PatchGroup>::iterator g = pg->begin(); g != pg->end(); ++g) {
      level++;
      //for (std::vector<PatchGroup>::iterator g = pg.begin(); g != pg.end(); ++g) {
      for (ciPatchGroup g = pg.begin(); g != pg.end(); ++g) {
            PatchGroup* pgp = *g;
            const PatchList& pl = pgp->patches;
            //xml.stag(QString("PatchGroup name=\"%1\"").arg(Xml::xmlString(g->name)));
            //xml.tag(level, "PatchGroup name=\"%s\"", Xml::xmlString(g->name).toLatin1().constData());
            xml.tag(level, "PatchGroup name=\"%s\"", Xml::xmlString(pgp->name).toLatin1().constData());
            level++;
            //for (iPatch p = g->patches.begin(); p != g->patches.end(); ++p)
            for (ciPatch p = pl.begin(); p != pl.end(); ++p)
                  //(*p)->write(xml);
                  //p->write(level, xml);
                  (*p)->write(level, xml);
            level--;
            //xml.etag("PatchGroup");
            xml.etag(level, "PatchGroup");
            }
      for (iMidiController ic = _controller->begin(); ic != _controller->end(); ++ic)
            //(*ic)->write(xml);
            ic->second->write(level, xml);
      //xml.etag("MidiInstrument");
      level--;
      xml.etag(level, "MidiInstrument");
      //xml.etag("muse");
      level--;
      xml.etag(level, "muse");
      }

//---------------------------------------------------------
//   getPatchName
//---------------------------------------------------------

const char* MidiInstrument::getPatchName(int channel, int prog, MType mode, bool drum)
      {
      int pr = prog & 0xff;
      if(prog == CTRL_VAL_UNKNOWN || pr == 0xff)
            return "<unknown>";

      int hbank = (prog >> 16) & 0xff;
      int lbank = (prog >> 8) & 0xff;
      int tmask = 1;
      bool drumchan = channel == 9;
      bool hb = false;
      bool lb = false;
      switch (mode) {
            case MT_GS:
                  tmask = 2;
                  hb    = true;
                  break;
            case MT_XG:
                  hb    = true;
                  lb    = true;
                  tmask = 4;
                  break;
            case MT_GM:
                  if(drumchan)
                        return gmdrumname;
                  tmask = 1;
                  break;
            default:
                  hb    = true;     // MSB bank matters
                  lb    = true;     // LSB bank matters
                  break;
            }
      for (ciPatchGroup i = pg.begin(); i != pg.end(); ++i) {
            const PatchList& pl = (*i)->patches;
            for (ciPatch ipl = pl.begin(); ipl != pl.end(); ++ipl) {
                  const Patch* mp = *ipl;
                  if ((mp->typ & tmask)
                    && (pr == mp->prog)
                    && ((drum && mode != MT_GM) || 
                       (mp->drum == drumchan))   
                    
                    && (hbank == mp->hbank || !hb || mp->hbank == -1)
                    && (lbank == mp->lbank || !lb || mp->lbank == -1))
                        return mp->name.toLatin1().constData();
                  }
            }
      return "<unknown>";
      }

//---------------------------------------------------------
//   populatePatchPopup
//---------------------------------------------------------

void MidiInstrument::populatePatchPopup(QMenu* menu, int chan, MType songType, bool drum)
      {
      menu->clear();
      int mask = 0;
      bool drumchan = chan == 9;
      switch (songType) {
            case MT_XG: mask = 4; break;
            case MT_GS: mask = 2; break;
            case MT_GM: 
              if(drumchan)
                return;
              mask = 1; 
              break;
            case MT_UNKNOWN:  mask = 7; break;
            }
      if (pg.size() > 1) {
            for (ciPatchGroup i = pg.begin(); i != pg.end(); ++i) {
                  PatchGroup* pgp = *i;
                  QMenu* pm = menu->addMenu(pgp->name);
                  pm->setFont(config.fonts[0]);
                  const PatchList& pl = pgp->patches;
                  for (ciPatch ipl = pl.begin(); ipl != pl.end(); ++ipl) {
                        const Patch* mp = *ipl;
                        if ((mp->typ & mask) && 
                            ((drum && songType != MT_GM) || 
                            (mp->drum == drumchan)) )  
                            {
                              int id = ((mp->hbank & 0xff) << 16)
                                         + ((mp->lbank & 0xff) << 8) + (mp->prog & 0xff);
                              QAction* act = pm->addAction(mp->name);
                              //act->setCheckable(true);
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
                  if (mp->typ & mask) {
                        int id = ((mp->hbank & 0xff) << 16)
                                 + ((mp->lbank & 0xff) << 8) + (mp->prog & 0xff);
                        QAction* act = menu->addAction(mp->name);
                        //act->setCheckable(true);
                        act->setData(id);
                        }
                  }
            }
      }

