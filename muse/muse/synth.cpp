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
#include "muse.h"
#include "synth.h"
#include "midi.h"
#include "synti/libsynti/mess.h"
#include "song.h"
#include "audio.h"
#include "event.h"
#include "midievent.h"
#include "audio.h"
#include "midiseq.h"
#include "midictrl.h"
#include "instruments/minstrument.h"

std::vector<Synth*> synthis;  // array of available synthis

extern void connectNodes(AudioTrack*, AudioTrack*);

//---------------------------------------------------------
//   description
//---------------------------------------------------------

const char* MessSynth::description() const
      {
      return descr ? descr->description : "";
      }

//---------------------------------------------------------
//   version
//---------------------------------------------------------

const char* MessSynth::version() const
      {
      return descr ? descr->version : "";
      }

bool MessSynthIF::guiVisible() const
      {
      return _mess ? _mess->guiVisible() : false;
      }

void MessSynthIF::showGui(bool v)
      {
      if (v == guiVisible())
            return;
      if (_mess)
            _mess->showGui(v);
      }

bool MessSynthIF::hasGui() const
      {
      if (_mess)
            return _mess->hasGui();
      return false;
      }

MidiEvent MessSynthIF::receiveEvent()
      {
      if (_mess)
            return _mess->receiveEvent();
      return MidiEvent();
      }

int MessSynthIF::eventsPending() const
      {
      if (_mess)
            return _mess->eventsPending();
      return 0;
      }

void MessSynthIF::getGeometry(int* x, int* y, int* w, int* h) const
      {
      if (_mess)
            _mess->getGeometry(x, y, w, h);
      }

void MessSynthIF::setGeometry(int x, int y, int w, int h)
      {
      if (_mess)
            _mess->setGeometry(x, y, w, h);
      }

//---------------------------------------------------------
//   findSynth
//    search for synthesizer base class
//---------------------------------------------------------

Synth* findSynth(const QString& sclass)
      {
      for (std::vector<Synth*>::iterator i = synthis.begin();
         i != synthis.end(); ++i) {
            if ((*i)->name() == sclass)
                  return *i;
            }
      printf("synthi class <%s> not found\n", sclass.toLatin1().data());
      return 0;
      }

//---------------------------------------------------------
//   Synth
//---------------------------------------------------------

Synth::Synth(const QFileInfo* fi, QString s)
   : info(*fi), _name(s)
      {
      _instances = 0;
      }

//---------------------------------------------------------
//   instantiate
//---------------------------------------------------------

void* MessSynth::instantiate(const QString& instanceName)
      {
      ++_instances;
      const char* path = strdup(info.filePath().toAscii().data());

      // load Synti dll
      if (debugMsg)
            printf("  load synti <%s>\n", path);
      void* handle = dlopen(path, RTLD_NOW);
      // void* handle = dlopen(path, RTLD_LAZY);
      if (handle == 0) {
            fprintf(stderr, "Synth::instantiate: dlopen(%s) failed: %s\n",
               path, dlerror());
            delete path;
            return 0;
            }
      typedef const MESS* (*MESS_Function)();
      MESS_Function msynth = (MESS_Function)dlsym(handle, "mess_descriptor");

      if (!msynth) {
            const char *txt = dlerror();
            if (txt) {
                  fprintf(stderr,
                     "Unable to find msynth_descriptor() function in plugin "
                     "library file \"%s\": %s.\n"
                     "Are you sure this is a MESS plugin file?\n",
                     path, txt);
                  delete path;
                  return 0;
                  }
            }
      delete path;
      descr = msynth();
      if (descr == 0) {
            fprintf(stderr, "Synth::instantiate: no MESS descr found\n");
            return 0;
            }
      Mess* mess = descr->instantiate(AL::sampleRate, instanceName.toLatin1().data());
      return mess;
      }

//---------------------------------------------------------
//   SynthI
//---------------------------------------------------------

SynthI::SynthI()
   : AudioTrack()
      {
      track      = this;
      synthesizer = 0;
      _sif        = 0;
      // setVolume(1.0);
      // setPan(0.0);
      setReadonly(true);      // midi instrument cannot be edited
      }

//---------------------------------------------------------
//   ~SynthI
//---------------------------------------------------------

SynthI::~SynthI()
      {
      deactivate2();
      deactivate3();
      }

//---------------------------------------------------------
//   setName
//---------------------------------------------------------

void SynthI::setName(const QString& s)
      {
      Track::setName(s);
      }

//---------------------------------------------------------
//   init
//---------------------------------------------------------

bool MessSynthIF::init(Synth* s, SynthI* si)
      {
      _mess = (Mess*)((MessSynth*)s)->instantiate(si->name());
      return (_mess == 0);
      }

//---------------------------------------------------------
//   channels
//---------------------------------------------------------

int MessSynthIF::channels() const
      {
      return _mess->channels();
      }

//---------------------------------------------------------
//   createSIF
//---------------------------------------------------------

SynthIF* MessSynth::createSIF(SynthI* si)
      {
      MessSynthIF* sif = new MessSynthIF(si);
      sif->init(this, si);
      return sif;
      }

//---------------------------------------------------------
//   initInstance
//    returns false on success
//---------------------------------------------------------

bool SynthI::initInstance(Synth* s)
      {
      synthesizer = s;
      _sif        = s->createSIF(this);

      setIName(name());   // set instrument name
      AudioTrack::setChannels(_sif->channels());

      //---------------------------------------------------
      //  read available controller from synti
      //---------------------------------------------------

      int id = 0;
      MidiControllerList* cl = MidiInstrument::controller();
      for (;;) {
            const char* name;
            int ctrl;
            int min;
            int max;
            id = _sif->getControllerInfo(id, &name, &ctrl, &min, &max);
            if (id == 0)
                  break;
            MidiController* c = new MidiController(QString(name), ctrl, min, max, 0);
            cl->push_back(c);
            }

      EventList* iel = midiState();
      if (!iel->empty()) {
            for (iEvent i = iel->begin(); i != iel->end(); ++i) {
                  Event ev = i->second;
                  MidiEvent pev(0, 0, ev);
                  if (_sif->putEvent(pev))
                        putFifo.put(pev);       // save for later retry
                  }
            iel->clear();
            }

      int idx = 0;
      for (std::vector<float>::iterator i = initParams.begin(); i != initParams.end(); ++i, ++idx)
            _sif->setParameter(idx, *i);
      return false;
      }

//---------------------------------------------------------
//   getControllerInfo
//---------------------------------------------------------

int MessSynthIF::getControllerInfo(int id, const char** name, int* ctrl, int* min, int* max)
      {
      return _mess->getControllerInfo(id, name, ctrl, min, max);
      }

//---------------------------------------------------------
//   SynthI::deactivate
//---------------------------------------------------------

void SynthI::deactivate2()
      {
      removeMidiInstrument(this);
      }

//---------------------------------------------------------
//   deactivate3
//---------------------------------------------------------

void SynthI::deactivate3()
      {
      delete _sif;
      _sif = 0;
      synthesizer->incInstances(-1);
      }

void MessSynthIF::deactivate3()
      {
      if (_mess) {
            delete _mess;
            _mess = 0;
            }
      }

MessSynthIF::~MessSynthIF()
      {
      deactivate3();
      }

//---------------------------------------------------------
//   initMidiSynth
//    search for software synthis and advertise
//---------------------------------------------------------

void initMidiSynth()
      {
      QString s = museGlobalLib + "/synthi";

#ifdef __APPLE__
      QDir pluginDir(s, QString("*.dylib"), 0, QDir::Files);
#else
      QDir pluginDir(s, QString("*.so"), 0, QDir::Files);
#endif
      if (debugMsg)
            printf("searching for software synthesizer in <%s>\n", s.toLatin1().data());
      if (pluginDir.exists()) {
            QFileInfoList list = pluginDir.entryInfoList();
            for (int i = 0; i < list.size(); ++i) {
                  QFileInfo fi = list.at(i);
                  synthis.push_back(new MessSynth(&fi));
                  }
            if (debugMsg)
                  printf("%zd soft synth found\n", synthis.size());
            }
      }

//---------------------------------------------------------
//   write
//---------------------------------------------------------

void SynthI::write(Xml& xml) const
      {
      xml.stag("SynthI");
      AudioTrack::writeProperties(xml);
      xml.tag("class", synth()->name());

//      for (int i = 0; i < MIDI_CHANNELS; ++i) {
//            if (!_channel[i]->noInRoute())
//                  _channel[i]->write(xml);
//            }

      //---------------------------------------------
      // if soft synth is attached to a midi port,
      // write out port number
      //---------------------------------------------

      if (hasGui()) {
            xml.tag("guiVisible", guiVisible());
            int x, y, w, h;
            w = 0;
            h = 0;
            getGeometry(&x, &y, &w, &h);
            if (h || w)
                  xml.tag("geometry", QRect(x, y, w, h));
            }
      _sif->write(xml);
      xml.etag("SynthI");

      }

void MessSynthIF::write(Xml& xml) const
      {
      //---------------------------------------------
      // dump current state of synth
      //---------------------------------------------

      int len = 0;
      const unsigned char* p;
      _mess->getInitData(&len, &p);
      if (len) {
            xml.stag("midistate");
            xml.stag("event type=\"%d\" datalen=\"%d\"", Sysex, len);
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
            xml.etag("event");
            xml.etag("midistate");
            }
      }

//---------------------------------------------------------
//   SynthI::read
//---------------------------------------------------------

void SynthI::read(QDomNode node)
      {
      QString sclass;
      int port = -1;
      bool startGui = false;
      QRect r;

      for (; !node.isNull(); node = node.nextSibling()) {
            QDomElement e = node.toElement();
            QString tag(e.tagName());
            if (tag == "class")
                  sclass = e.text();
            else if (tag == "port")
                  port  = e.text().toInt();
            else if (tag == "guiVisible")
                  startGui = e.text().toInt();
            else if (tag == "midistate")
                  readMidiState(node.firstChild());
            else if (tag == "param") {
                  float val = e.text().toFloat();
                  initParams.push_back(val);
                  }
            else if (tag == "geometry")
                  r = AL::readGeometry(node);
//            else if (tag == "MidiChannel") {
//                  int idx = e.attribute("idx", "0").toInt();
//                  _channel[idx]->read(node.firstChild());
//                  }
            else if (AudioTrack::readProperties(node)) {
                  printf("MusE:SynthI: unknown tag %s\n", e.tagName().toLatin1().data());
                  }
            }
      Synth* s = findSynth(sclass);
      if (s == 0)
            return;
      if (initInstance(s))
            return;
      song->insertTrack0(this, -1);
      setGeometry(r.x(), r.y(), r.width(), r.height());
      showGui(startGui);
      }

//---------------------------------------------------------
//   getPatchName
//---------------------------------------------------------

QString MessSynthIF::getPatchName(int channel, int prog)
      {
      if (_mess)
            return _mess->getPatchName(channel, prog, 0);
      return "";
      }

//---------------------------------------------------------
//   populatePatchPopup
//---------------------------------------------------------

void MessSynthIF::populatePatchPopup(QMenu* menu, int ch)
      {
      menu->clear();
      const char* bank = _mess->getBankName(0);
      int idx = 0;
      if (bank) {
            while (bank) {
                  // synthesizer has banks
                  QMenu* a = menu->addMenu(QString(bank));

                  MidiPatch patch;
                  patch.typ   = 0;
                  patch.hbank = idx << 8;
                  patch.lbank = idx;
                  patch.prog  = 0;
                  const MidiPatch* mp = _mess->getPatchInfo(ch, &patch);
                  while (mp) {
                        int id = ((mp->hbank & 0xff) << 16)
                           + ((mp->lbank & 0xff) << 8) + mp->prog;
                        QAction* aa = a->addAction(QString(mp->name));
                        aa->setData(id);
                        mp = _mess->getPatchInfo(ch, mp);
                        }
                  ++idx;
                  bank = _mess->getBankName(idx);
                  }
            }
      else {
            const MidiPatch* mp = _mess->getPatchInfo(ch, 0);
            while (mp) {
                  int id = ((mp->hbank & 0xff) << 16)
                            + ((mp->lbank & 0xff) << 8) + mp->prog;
                  QAction* a = menu->addAction(QString(mp->name));
                  a->setData(id);
                  mp = _mess->getPatchInfo(ch, mp);
                  }
            }
      }

//---------------------------------------------------------
//   getData
//---------------------------------------------------------

void MessSynthIF::getData(MidiEventList* el, unsigned pos, int ports, unsigned n, float** buffer)
      {
      // Reset buffers first
      for (int port = 0; port < ports; ++port)
            memset(buffer[port], 0, n * sizeof(float));

      // seems to be negative when sequencer not running, still 
      // working when entering notes from editor though. weird :-) (ml)

      int curPos = pos;
      int endPos = pos + n;

      while (!synti->putFifo.isEmpty())
            putEvent(synti->putFifo.get());

      if (ports >= channels()) {
            iMidiEvent i = el->begin();
            for (; i != el->end(); ++i) {
                  int frame = i->time();
                  if (frame >= endPos)
                        break;
                  if (frame > curPos) { 
                        // Several following notes during same segmentsize?
                        _mess->process(buffer, curPos-pos, frame - curPos);
                        curPos = frame; // don't process this piece again
                        }
                  if (putEvent(*i))
                        synti->putFifo.put(*i);
                  }
            if (endPos - curPos > 0)
                  _mess->process(buffer, curPos-pos, endPos - curPos);
            el->erase(el->begin(), i);
            }
      else {
            // this happens if the synth has stereo and we switch the
            // channel to mono

            printf("MessSynthIF::getData - ports %d < channels %d\n", 
               ports, channels());
            }
      }

//---------------------------------------------------------
//   putEvent
//    return true on error (busy), event will later be
//    resend
//---------------------------------------------------------

bool MessSynthIF::putEvent(const MidiEvent& ev)
      {
      bool rv = true;
      if (_mess) {
            rv = _mess->processEvent(ev);
            if (midiOutputTrace && !rv) {
                  printf("<%s>", synti->name().toLatin1().data());
                  ev.dump();
                  }
            }
      return rv;
      }

//---------------------------------------------------------
//   collectInputData
//---------------------------------------------------------

void SynthI::collectInputData()
      {
      bufferEmpty = false;
      unsigned pos = audio->pos().frame();
      _sif->getData(&_schedEvents, pos, channels(), segmentSize, buffer);
      }

//-------------------------------------------------------------------
//   process
//    Collect all midi events for the current process cycle and put
//    into _schedEvents queue. For note on events create the proper
//    note off events. The note off events maybe played after the
//    current process cycle.
//-------------------------------------------------------------------

void SynthI::processMidi(unsigned fromTick, unsigned toTick, unsigned fromFrame, unsigned toFrame)
      {
      if (mute())
            return;
      MidiOut::processMidi(_schedEvents, fromTick, toTick, fromFrame, toFrame);
      }

