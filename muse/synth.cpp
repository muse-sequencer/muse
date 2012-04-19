//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: synth.cpp,v 1.43.2.23 2009/12/15 03:39:58 terminator356 Exp $
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

#include "config.h"
#include <sys/wait.h>
#include <signal.h>
#include <pthread.h>
#include <unistd.h>
#include <errno.h>
#include <sys/mman.h>
#include <vector>
#include <fcntl.h>
#include <dlfcn.h>

#include <QDir>
#include <QString>

#include "app.h"
#include "arranger.h"
#include "synth.h"
#include "xml.h"
#include "midi.h"
#include "midiport.h"
#include "mididev.h"
#include "synti/libsynti/mess.h"  
#include "song.h"
#include "audio.h"
#include "event.h"
#include "mpevent.h"
#include "audio.h"
#include "midiseq.h"
#include "midictrl.h"
#include "popupmenu.h"
#include "globaldefs.h"

namespace MusEGlobal {
std::vector<MusECore::Synth*> synthis;  // array of available MusEGlobal::synthis
}

namespace MusECore {

extern void connectNodes(AudioTrack*, AudioTrack*);
bool SynthI::_isVisible=false;

const char* synthTypes[] = { "METRONOME", "MESS", "DSSI", "VST", "UNKNOWN" };
QString synthType2String(Synth::Type type) { return QString(synthTypes[type]); } 

Synth::Type string2SynthType(const QString& type) 
{ 
  for(int i = 0; i < Synth::SYNTH_TYPE_END; ++i)
  {  
    if(synthType2String((Synth::Type)i) == type)
      return (Synth::Type)i; 
  }  
  return Synth::SYNTH_TYPE_END;
} 

bool MessSynthIF::nativeGuiVisible() const
      {
      return _mess ? _mess->nativeGuiVisible() : false;
      }

void MessSynthIF::showNativeGui(bool v)
      {
      if (v == nativeGuiVisible())
            return;
      if (_mess)
            _mess->showNativeGui(v);
      }

bool MessSynthIF::hasNativeGui() const
      {
      if (_mess)
            return _mess->hasNativeGui();
      return false;
      }

MidiPlayEvent MessSynthIF::receiveEvent()
      {
      if (_mess)
            return _mess->receiveEvent();
      return MidiPlayEvent();
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

void MessSynthIF::getNativeGeometry(int* x, int* y, int* w, int* h) const
      {
      if (_mess)
            _mess->getNativeGeometry(x, y, w, h);
      }

void MessSynthIF::setNativeGeometry(int x, int y, int w, int h)
      {
      if (_mess)
            _mess->setNativeGeometry(x, y, w, h);
      }

//---------------------------------------------------------
//   findSynth
//    search for synthesizer base class
//---------------------------------------------------------

static Synth* findSynth(const QString& sclass, const QString& label, Synth::Type type = Synth::SYNTH_TYPE_END)
      {
      for (std::vector<Synth*>::iterator i = MusEGlobal::synthis.begin();
         i != MusEGlobal::synthis.end(); ++i) 
         {
            if( ((*i)->baseName() == sclass) && 
                (label.isEmpty() || ((*i)->name() == label)) &&
                (type == Synth::SYNTH_TYPE_END || type == (*i)->synthType()) )
              return *i;
         }
      printf("synthi type:%d class:%s label:%s not found\n", type, sclass.toLatin1().constData(), label.toLatin1().constData());
      return 0;
      }

//---------------------------------------------------------
//   createSynthInstance
//    create a synthesizer instance of class "label"
//---------------------------------------------------------

static SynthI* createSynthInstance(const QString& sclass, const QString& label, Synth::Type type = Synth::SYNTH_TYPE_END)
      {
      Synth* s = findSynth(sclass, label, type);
      SynthI* si = 0;
      if (s) {
            si = new SynthI();
            QString n;
            n.setNum(s->instances());
            QString instance_name = s->name() + "-" + n;
            if (si->initInstance(s, instance_name)) {
                  delete si;
                  return 0;
                  }
            }
      else
            printf("createSynthInstance: synthi class:%s label:%s not found\n", sclass.toLatin1().constData(), label.toLatin1().constData());
      return si;
      }

//---------------------------------------------------------
//   Synth
//---------------------------------------------------------

Synth::Synth(const QFileInfo& fi, QString label, QString descr, QString maker, QString ver)
   : info(fi), _name(label), _description(descr), _maker(maker), _version(ver)
      {
      _instances = 0;
      }

//---------------------------------------------------------
//   instantiate
//---------------------------------------------------------

void* MessSynth::instantiate(const QString& instanceName)
      {
      ++_instances;
      
      MusEGlobal::doSetuid();
      QByteArray ba = info.filePath().toLatin1();
      const char* path = ba.constData();

      // load Synti dll
      void* handle = dlopen(path, RTLD_NOW);
      if (handle == 0) {
            fprintf(stderr, "Synth::instantiate: dlopen(%s) failed: %s\n",
               path, dlerror());
            MusEGlobal::undoSetuid();
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
                     info.filePath().toAscii().constData(), txt);
                  MusEGlobal::undoSetuid();
                  return 0;
                  }
            }
      _descr = msynth();
      if (_descr == 0) {
            fprintf(stderr, "Synth::instantiate: no MESS descr found\n");
            MusEGlobal::undoSetuid();
            return 0;
            }
      Mess* mess = _descr->instantiate(MusEGlobal::sampleRate, MusEGlobal::muse, &MusEGlobal::museProject, instanceName.toLatin1().constData());
      MusEGlobal::undoSetuid();
      return mess;
      }

//---------------------------------------------------------
//   SynthI
//---------------------------------------------------------

SynthI::SynthI()
   : AudioTrack(AUDIO_SOFTSYNTH)
      {
      synthesizer = 0;
      _sif        = 0;
      _rwFlags    = 1;
      _openFlags  = 1;
      _readEnable = false;
      _writeEnable = false;
      
      _curBankH = 0;
      _curBankL = 0;
      _curProgram  = 0;

      setVolume(1.0);
      setPan(0.0);
      }


//---------------------------------------------------------
//   height in arranger
//---------------------------------------------------------
int SynthI::height() const
{
  if (_isVisible)
    return _height;
  return 0;
}


//---------------------------------------------------------
//   open
//---------------------------------------------------------

QString SynthI::open() 
{ 
  // Make it behave like a regular midi device.
  _readEnable = false;
  _writeEnable = (_openFlags & 0x01);
    
  return QString("OK");
}

//---------------------------------------------------------
//   close
//---------------------------------------------------------

void SynthI::close()
{
  _readEnable = false;
  _writeEnable = false;
}

//---------------------------------------------------------
//   putMidiEvent
//---------------------------------------------------------

bool SynthI::putEvent(const MidiPlayEvent& ev) 
{
  if(_writeEnable)
  {
    if (MusEGlobal::midiOutputTrace)
    {
          printf("MidiOut: Synth: <%s>: ", name().toLatin1().constData());
          ev.dump();
    }
    return _sif->putEvent(ev);
  }

  return false;  
}

//---------------------------------------------------------
//   processMidi
//---------------------------------------------------------

void SynthI::processMidi() 
{
    processStuckNotes(); 
}

//---------------------------------------------------------
//   setName
//---------------------------------------------------------

void SynthI::setName(const QString& s)
      {
      AudioTrack::setName(s);
      MidiDevice::setName(s);
      }

//---------------------------------------------------------
//   currentProg
//---------------------------------------------------------

void SynthI::currentProg(unsigned long *prog, unsigned long *bankL, unsigned long *bankH)
{
  if(prog)
    *prog  = _curProgram;
  if(bankL)
    *bankL = _curBankL;
  if(bankH)
    *bankH = _curBankH;
}

//---------------------------------------------------------
//   init
//---------------------------------------------------------

bool MessSynthIF::init(Synth* s, SynthI* si)
      {
      _mess = (Mess*)((MessSynth*)s)->instantiate(si->name());
      
      return (_mess == 0);
      }

int MessSynthIF::channels() const
      {
      return _mess->channels();
      }

int MessSynthIF::totalOutChannels() const
      {
      return _mess->channels();
      }

int MessSynthIF::totalInChannels() const
      {
      return 0;
      }

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

bool SynthI::initInstance(Synth* s, const QString& instanceName)
      {
      synthesizer = s;

      setName(instanceName);    // set midi device name
      setIName(instanceName);   // set instrument name
      _sif        = s->createSIF(this);
      
      AudioTrack::setTotalOutChannels(_sif->totalOutChannels());
      AudioTrack::setTotalInChannels(_sif->totalInChannels());
      
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
            int initval = CTRL_VAL_UNKNOWN;
            id = _sif->getControllerInfo(id, &name, &ctrl, &min, &max, &initval);
            if (id == 0)
                  break;
            
            
            // Added by T356. Override existing program controller.
            iMidiController i = cl->end();
            if(ctrl == CTRL_PROGRAM)
            {
              for(i = cl->begin(); i != cl->end(); ++i) 
              {
                if(i->second->num() == CTRL_PROGRAM)
                {
                  delete i->second;
                  cl->erase(i);
                  
                  break;
                }
              }
            }  
            
            MidiController* c = new MidiController(QString(name), ctrl, min, max, initval);
            cl->add(c);
          }

      // Restore the midi state...
      EventList* iel = midiState();
      if (!iel->empty()) {
            for (iEvent i = iel->begin(); i != iel->end(); ++i) {
                  Event ev = i->second;
                  
                  // p4.0.27 A kludge to support old midistates by wrapping them in the proper header.
                  if(ev.type() == Sysex && _tmpMidiStateVersion < SYNTH_MIDI_STATE_SAVE_VERSION)
                  {
                    int len = ev.dataLen();
                    if(len > 0)
                    {
                      const unsigned char* data = ev.data();
                      const unsigned char* hdr;
                      // Get the unique header for the synth.
                      int hdrsz = _sif->oldMidiStateHeader(&hdr);
                      if(hdrsz > 0)
                      {
                        int newlen = hdrsz + len;
                        unsigned char* d = new unsigned char[newlen];
                        memcpy(d, hdr, hdrsz);
                        memcpy(d + hdrsz, data, len);
                        ev.setData(d, newlen);
                        delete[] d;
                      }  
                    }
                  }
                  
                  MidiPlayEvent pev(0, 0, 0, ev);
                  if (_sif->putEvent(pev))
                        break;   // try later
                  }
            iel->clear();
            }

      unsigned long idx = 0;
      for (std::vector<float>::iterator i = initParams.begin(); i != initParams.end(); ++i, ++idx)
            _sif->setParameter(idx, *i);
      
      // p3.3.40 Since we are done with the (sometimes huge) initial parameters list, clear it. 
      // TODO: Decide: Maybe keep them around for a 'reset to previously loaded values' (revert) command? ...
      initParams.clear();      
      
      return false;
      }

//---------------------------------------------------------
//   getControllerInfo
//---------------------------------------------------------

int MessSynthIF::getControllerInfo(int id, const char** name, int* ctrl, int* min, int* max, int* initval)
      {
      return _mess->getControllerInfo(id, name, ctrl, min, max, initval);
      }

//---------------------------------------------------------
//   SynthI::deactivate
//---------------------------------------------------------

void SynthI::deactivate2()
      {
      removeMidiInstrument(this);
      MusEGlobal::midiDevices.remove(this);
      if (midiPort() != -1) {
            // synthi is attached
            MusEGlobal::midiPorts[midiPort()].setMidiDevice(0);
            }
      }
//---------------------------------------------------------
//   deactivate3
//---------------------------------------------------------

void SynthI::deactivate3()
      {
      _sif->deactivate3();
      
      //synthesizer->incInstances(-1); // Moved below by Tim. p3.3.14
      
      if(MusEGlobal::debugMsg)
        printf("SynthI::deactivate3 deleting _sif...\n");
      
      delete _sif;
      _sif = 0;
      
      if(MusEGlobal::debugMsg)
        printf("SynthI::deactivate3 decrementing synth instances...\n");
      
      synthesizer->incInstances(-1);
      }

void MessSynthIF::deactivate3()
      {
      if (_mess) {
            delete _mess;
            _mess = 0;
            }
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
//   initMidiSynth
//    search for software MusEGlobal::synthis and advertise
//---------------------------------------------------------

void initMidiSynth()
      {
      QString s = MusEGlobal::museGlobalLib + "/synthi";

      QDir pluginDir(s, QString("*.so")); // ddskrjo
      if (MusEGlobal::debugMsg)
            printf("searching for software synthesizer in <%s>\n", s.toLatin1().constData());
      if (pluginDir.exists()) {
            QFileInfoList list = pluginDir.entryInfoList();
	    QFileInfoList::iterator it=list.begin();
            QFileInfo* fi;
            while(it!=list.end()) {
                  fi = &*it;
            
                  QByteArray ba = fi->filePath().toLatin1();
                  const char* path = ba.constData();
            
                  // load Synti dll
                  void* handle = dlopen(path, RTLD_NOW);
                  if (handle == 0) {
                        fprintf(stderr, "initMidiSynth: MESS dlopen(%s) failed: %s\n", path, dlerror());
                        ++it;
                        continue;
                        }
                  typedef const MESS* (*MESS_Function)();
                  MESS_Function msynth = (MESS_Function)dlsym(handle, "mess_descriptor");
            
                  if (!msynth) {
                        #if 1
                        const char *txt = dlerror();
                        if (txt) {
                              fprintf(stderr,
                                "Unable to find msynth_descriptor() function in plugin "
                                "library file \"%s\": %s.\n"
                                "Are you sure this is a MESS plugin file?\n",
                                path, txt);
                              }
                        #endif      
                          dlclose(handle);
                          ++it;
                          continue;
                        }
                  const MESS* descr = msynth();
                  if (descr == 0) {
                        fprintf(stderr, "initMidiSynth: no MESS descr found in %s\n", path);
                        dlclose(handle);
                        ++it;
                        continue;
                        }
                  
                  MusEGlobal::synthis.push_back(new MessSynth(*fi, QString(descr->name), QString(descr->description), QString(""), QString(descr->version)));
                  
                  dlclose(handle);
                  ++it;
                  }
            if (MusEGlobal::debugMsg)
                  printf("%zd soft synth found\n", MusEGlobal::synthis.size());
            }
      }


//---------------------------------------------------------
//   createSynthI
//    create a synthesizer instance of class "label"
//    If insertAt is valid, inserts before insertAt. Else at the end after all tracks.
//---------------------------------------------------------

SynthI* Song::createSynthI(const QString& sclass, const QString& label, Synth::Type type, Track* insertAt)
      {
      SynthI* si = createSynthInstance(sclass, label, type);
      if(!si)
        return 0;
      
      int idx = insertAt ? _tracks.index(insertAt) : -1;
      
      insertTrack1(si, idx);
      
      msgInsertTrack(si, idx, true);       // add to instance list
      
      insertTrack3(si, idx);

      OutputList* ol = MusEGlobal::song->outputs();
      // add default route to master (first audio output)
      if (!ol->empty()) {
            AudioOutput* ao = ol->front();
            // Make sure the route channel and channels are valid.
            MusEGlobal::audio->msgAddRoute(Route((AudioTrack*)si, 0, ((AudioTrack*)si)->channels()), Route(ao, 0, ((AudioTrack*)si)->channels()));
            
            MusEGlobal::audio->msgUpdateSoloStates();
            }
      
      // DELETETHIS 5
      // Now that the track has been added to the lists in insertTrack2(),
      //  if it's a dssi synth, OSC can find the synth, and initialize (and show) its native gui.
      // No, initializing OSC without actually showing the gui doesn't work, at least for 
      //  dssi-vst plugins - without showing the gui they exit after ten seconds.
      //si->initGui();
      
      return si;
      }

//---------------------------------------------------------
//   write
//---------------------------------------------------------

void SynthI::write(int level, Xml& xml) const
      {
      xml.tag(level++, "SynthI");
      AudioTrack::writeProperties(level, xml);
      xml.strTag(level, "synthType", synthType2String(synth()->synthType()));

      xml.strTag(level, "class", synth()->baseName());
      
      // To support plugins like dssi-vst where all the baseNames are the same 'dssi-vst' and the label is the name of the dll file.
      // Added by Tim. p3.3.16
      xml.strTag(level, "label", synth()->name());

      //---------------------------------------------
      // if soft synth is attached to a midi port,
      // write out port number
      //---------------------------------------------

      if (midiPort() != -1)
            xml.intTag(level, "port", midiPort());

      if (hasGui()) {
            xml.intTag(level, "guiVisible", guiVisible());
            int x, y, w, h;
            w = 0;
            h = 0;
            getGeometry(&x, &y, &w, &h);
            if (h || w)
                  xml.qrectTag(level, "geometry", QRect(x, y, w, h));
            }

      if (hasNativeGui()) {
            xml.intTag(level, "nativeGuiVisible", nativeGuiVisible());
            int x, y, w, h;
            w = 0;
            h = 0;
            getNativeGeometry(&x, &y, &w, &h);
            if (h || w)
                  xml.qrectTag(level, "nativeGeometry", QRect(x, y, w, h));
            }

      _stringParamMap.write(level, xml, "stringParam");
      
      xml.tag(level, "curProgram bankH=\"%ld\" bankL=\"%ld\" prog=\"%ld\"/", _curBankH, _curBankL, _curProgram);
      
      _sif->write(level, xml);
      xml.etag(level, "SynthI");
      }

void MessSynthIF::write(int level, Xml& xml) const
      {
      //---------------------------------------------
      // dump current state of synth
      //---------------------------------------------

      int len = 0;
      const unsigned char* p;
      _mess->getInitData(&len, &p);
      if (len) {
            ///xml.tag(level++, "midistate");
            xml.tag(level++, "midistate version=\"%d\"", SYNTH_MIDI_STATE_SAVE_VERSION);
            xml.nput(level++, "<event type=\"%d\"", Sysex);
            xml.nput(" datalen=\"%d\">\n", len);
            xml.nput(level, "");
            for (int i = 0; i < len; ++i) {
                  if (i && ((i % 16) == 0)) {
                        xml.nput("\n");
                        xml.nput(level, "");
                        }
                  xml.nput("%02x ", p[i] & 0xff);
                  }
            xml.nput("\n");
            xml.tag(level--, "/event");
            xml.etag(level--, "midistate");
            }
      }

//---------------------------------------------------------
//   SynthI::readProgram
//---------------------------------------------------------

void SynthI::readProgram(Xml& xml, const QString& name)
{
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
                xml.unknown(name.toAscii().constData());
                break;
          case Xml::Attribut:
                if(tag == "bankH") 
                  _curBankH = xml.s2().toUInt();
                else
                if(tag == "bankL") 
                  _curBankL = xml.s2().toUInt();
                else
                if(tag == "prog") 
                  _curProgram = xml.s2().toUInt();
                else
                  xml.unknown(name.toAscii().constData());
                break;
          case Xml::TagEnd:
                if(tag == name) 
                  return;
          default:
                break;
    }
  }
}

//---------------------------------------------------------
//   SynthI::read
//---------------------------------------------------------

void SynthI::read(Xml& xml)
      {
      QString sclass;
      QString label;
      Synth::Type type = Synth::SYNTH_TYPE_END;

      int port = -1;
      bool startgui = false;
      bool startngui = false;
      QRect r, nr;

      for (;;) {
            Xml::Token token = xml.parse();
            const QString tag = xml.s1();
            switch (token) {
                  case Xml::Error:
                  case Xml::End:
                        return;
                  case Xml::TagStart:
                        if (tag == "synthType")
                              type = string2SynthType(xml.parse1());
                        else if (tag == "class")
                              sclass = xml.parse1();
                        else if (tag == "label")
                              label  = xml.parse1();
                        else if (tag == "port")
                              port  = xml.parseInt();
                        else if (tag == "guiVisible")
                              startgui = xml.parseInt();
                        else if (tag == "nativeGuiVisible")
                              startngui = xml.parseInt();
                        else if (tag == "midistate")
                              readMidiState(xml);
                        else if (tag == "param") {
                              float val = xml.parseFloat();
                              initParams.push_back(val);
                              }
                        else if (tag == "stringParam") 
                              _stringParamMap.read(xml, tag);
                        else if (tag == "curProgram") 
                              readProgram(xml, tag);
                        else if (tag == "geometry")
                              r = readGeometry(xml, tag);
                        else if (tag == "nativeGeometry")
                              nr = readGeometry(xml, tag);
                        else if (AudioTrack::readProperties(xml, tag))
                              xml.unknown("softSynth");
                        break;
                  case Xml::TagEnd:
                        if (tag == "SynthI") {
                              
                              // NOTICE: This is a hack to quietly change songs to use the new 'fluid_synth' name instead of 'fluidsynth'.
                              //         Recent linker changes required the name change in fluidsynth's cmakelists. Nov 8, 2011 By Tim.
                              if(sclass == QString("fluidsynth") && 
                                 (type == Synth::SYNTH_TYPE_END || type == Synth::MESS_SYNTH) &&
                                 (label.isEmpty() || label == QString("FluidSynth")) )
                                sclass = QString("fluid_synth");
                              
                              Synth* s = findSynth(sclass, label, type);
                              if (s == 0)
                                    return;
                              if (initInstance(s, name()))
                                    return;
                              MusEGlobal::song->insertTrack0(this, -1);
                              
                              if (port != -1 && port < MIDI_PORTS)
                                    MusEGlobal::midiPorts[port].setMidiDevice(this);
                              
                              // DELETETHIS 5
                              // Now that the track has been added to the lists in insertTrack2(),
                              //  if it's a dssi synth, OSC can find the synth, and initialize (and show) its native gui.
                              // No, initializing OSC without actually showing the gui doesn't work, at least for 
                              //  dssi-vst plugins - without showing the gui they exit after ten seconds.
                              //initGui();
                              showNativeGui(startngui);
                              setNativeGeometry(nr.x(), nr.y(), nr.width(), nr.height());
                              
                              mapRackPluginsToControllers();
                              
                              showGui(startgui);
                              setGeometry(r.x(), r.y(), r.width(), r.height());
                              
                              // Now that the track has been added to the lists in insertTrack2(), if it's a dssi synth 
                              //  OSC can find the track and its plugins, and start their native guis if required...
                              showPendingPluginNativeGuis();
                              
                              return;
                              }
                  default:
                        break;
                  }
            }
      AudioTrack::mapRackPluginsToControllers();
      }

//---------------------------------------------------------
//   getPatchName
//---------------------------------------------------------

const char* MessSynthIF::getPatchName(int channel, int prog, MType type, bool drum)
      {
        if (_mess)
        {
              const char* s = _mess->getPatchName(channel, prog, type, drum);
              if(s)
                return s;
        }      
        return "";
      }

//---------------------------------------------------------
//   populatePatchPopup
//---------------------------------------------------------

void MessSynthIF::populatePatchPopup(MusEGui::PopupMenu* menu, int ch, MType, bool)
      {
      menu->clear();
      const MidiPatch* mp = _mess->getPatchInfo(ch, 0);
      while (mp) {
            int id = ((mp->hbank & 0xff) << 16)
                      + ((mp->lbank & 0xff) << 8) + mp->prog;
            /* DELETETHIS 9
            int pgid = ((mp->hbank & 0xff) << 8) | (mp->lbank & 0xff) | 0x40000000;          
            int itemnum = menu->indexOf(pgid);
            if(itemnum == -1)
            {
              QPopupMenu* submenu = new QPopupMenu(menu);
              itemnum = 
            }
            */  
            QAction *act = menu->addAction(QString(mp->name));
            act->setData(id);
            mp = _mess->getPatchInfo(ch, mp);
            }
      }

//---------------------------------------------------------
//   preProcessAlways
//---------------------------------------------------------

void SynthI::preProcessAlways()
{
  if(_sif)
    _sif->preProcessAlways();
  _processed = false;  
  
  // TODO: p4.0.15 Tim. Erasure of already-played events was moved from Audio::processMidi() 
  //  to each of the midi devices - ALSA, Jack, or Synth in SynthI::getData() below. 
  // If a synth track is 'off', AudioTrack::copyData() does not call our getData().
  // So there is no processing of midi play events, or putEvent FIFOs.
  // Hence the play events list and putEvent FIFOs will then accumulate events, sometimes
  //  thousands. Only when the Synth track is turned on again, are all these events 
  //  processed. Whether or not we want this is a question.
  //
  // If we DON'T want the events to accumulate, we NEED this following piece of code.
  // Without this code: When a song is loaded, if a Synth track is off, various controller init events
  //  can remain queued up so that when the Synth track is turned on, those initializations 
  //  will be processed. Otherwise we, or the user, will have to init every time the track is turned on.
  // Con: Thousands of events can accumulate. For example selecting "midi -> Reset Instr." sends a flood 
  //  of 2048 note-off events, one for each note in each channel! Each time, the 2048, 4096, 8192 etc. 
  //  events remain in the list.
  // Variation: Maybe allow certain types, or groups, of events through, especially bulk init or note offs.
  if(off())
  {
    // Clear any accumulated play events.
    //playEvents()->clear(); DELETETHIS
    _playEvents.clear();
    // Eat up any fifo events.
    //while(!eventFifo.isEmpty())  DELETETHIS
    //  eventFifo.get();  
    eventFifo.clear();  // Clear is the same but faster AND safer, right?
  }
}

void MessSynthIF::preProcessAlways()
{
  if(_mess)
    _mess->processMessages();
}

//---------------------------------------------------------
//   getData
//---------------------------------------------------------

bool SynthI::getData(unsigned pos, int ports, unsigned n, float** buffer)
      {
      for (int k = 0; k < ports; ++k)
            memset(buffer[k], 0, n * sizeof(float));

      int p = midiPort();
      MidiPort* mp = (p != -1) ? &MusEGlobal::midiPorts[p] : 0;
               
      iMPEvent ie = _playEvents.begin();      
      
      ie = _sif->getData(mp, &_playEvents, ie, pos, ports, n, buffer);
      
      // p4.0.15 We are done with these events. Let us erase them here instead of Audio::processMidi.
      // That way we can simply set the next play event to the beginning.
      // This also allows other events to be inserted without the problems caused by the next play event 
      //  being at the 'end' iterator and not being *easily* set to some new place beginning of the newer insertions. 
      // The way that MPEventList sorts made it difficult to predict where the iterator of the first newly inserted items was.
      // The erasure in Audio::processMidi was missing some events because of that.
      _playEvents.erase(_playEvents.begin(), ie);
      
      return true;
      }

iMPEvent MessSynthIF::getData(MidiPort* mp, MPEventList* el, iMPEvent i, unsigned pos, int /*ports*/, unsigned n, float** buffer)
{
      //prevent compiler warning: comparison of signed/unsigned
      int curPos      = pos;
      int endPos      = pos + n;
      int off         = pos;
      int frameOffset = MusEGlobal::audio->getFrameOffset();

      for (; i != el->end(); ++i) {
          int evTime = i->time(); 
          if (evTime == 0)
                evTime=abs(frameOffset); // will cause frame to be zero, problem?

          int frame = evTime - abs(frameOffset);

            if (frame >= endPos) {
                printf("frame > endPos!! frame = %d >= endPos %d, i->time() %d, frameOffset %d curPos=%d\n", frame, endPos, i->time(), frameOffset,curPos);
                continue;
                }

            if (frame > curPos) {
                  if (frame < (int) pos)
                        printf("should not happen: missed event %d\n", pos -frame);
                  else 
                  {
                        if (!_mess)
                              printf("should not happen - no _mess\n");
                        else
                        {
                                _mess->process(buffer, curPos-pos, frame - curPos);
                        }      
                  }
                  curPos = frame;
            }
            
            if (mp)
                  mp->sendEvent(*i);
            else {
                  if (putEvent(*i))
                        break;
            }
      }
      
      if (endPos - curPos) 
      {
            if (!_mess)
                  printf("should not happen - no _mess\n");
            else
            {
                    _mess->process(buffer, curPos - off, endPos - curPos);
            }      
      }
      return i;
}

//---------------------------------------------------------
//   putEvent
//    return true on error (busy)
//---------------------------------------------------------

bool MessSynthIF::putEvent(const MidiPlayEvent& ev)
      {
      //if (MusEGlobal::midiOutputTrace) DELETETHIS or re-enable?
      //{
      //      printf("MidiOut: MESS: <%s>: ", synti->name().toLatin1().constData());
      //      ev.dump();
      //}
      if (_mess)
            return _mess->processEvent(ev);
      return true;
      }

//unsigned long MessSynthIF::uniqueID() const  DELETETHIS
//{ 
//  return _mess ? _mess->uniqueID() : 0; 
//}

//MidiPlayEvent& MessSynthIF::wrapOldMidiStateVersion(MidiPlayEvent& e) const
//{
//  return _mess ? _mess->wrapOldMidiStateVersion(e) : e; 
//}
 
int MessSynthIF::oldMidiStateHeader(const unsigned char** data) const
{
  return _mess ? _mess->oldMidiStateHeader(data) : 0;
}

} // namespace MusECore
