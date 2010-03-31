//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: synth.cpp,v 1.43.2.23 2009/12/15 03:39:58 terminator356 Exp $
//  (C) Copyright 2000-2003 Werner Schweer (ws@seh.de)
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

#include <qdir.h>
#include <dlfcn.h>
#include <qpopupmenu.h>

#include "app.h"
#include "synth.h"
#include "xml.h"
#include "midi.h"
#include "midiport.h"
#include "mididev.h"
#include "libsynti/mess.h"
#include "song.h"
#include "audio.h"
#include "event.h"
#include "mpevent.h"
#include "audio.h"
#include "midiseq.h"
#include "midictrl.h"

std::vector<Synth*> synthis;  // array of available synthis

extern void connectNodes(AudioTrack*, AudioTrack*);

/*
//---------------------------------------------------------
//   description
//---------------------------------------------------------

const char* MessSynth::description() const
      {
      return _descr ? _descr->description : "";
      }

//---------------------------------------------------------
//   version
//---------------------------------------------------------

const char* MessSynth::version() const
      {
      return _descr ? _descr->version : "";
      }
*/

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

//---------------------------------------------------------
//   findSynth
//    search for synthesizer base class
//---------------------------------------------------------

//static Synth* findSynth(const QString& sclass)
static Synth* findSynth(const QString& sclass, const QString& label)
      {
      for (std::vector<Synth*>::iterator i = synthis.begin();
         i != synthis.end(); ++i) 
         {
            //if ((*i)->baseName() == sclass)
            //if ((*i)->name() == sclass)
            if ( ((*i)->baseName() == sclass) && (label.isEmpty() || ((*i)->name() == label)) )
                  
                  return *i;
         }
      printf("synthi class:%s label:%s not found\n", sclass.latin1(), label.latin1());
      return 0;
      }

//---------------------------------------------------------
//   createSynthInstance
//    create a synthesizer instance of class "label"
//---------------------------------------------------------

//static SynthI* createSynthI(const QString& sclass)
static SynthI* createSynthInstance(const QString& sclass, const QString& label)
      {
      //Synth* s = findSynth(sclass);
      Synth* s = findSynth(sclass, label);
      SynthI* si = 0;
      if (s) {
            si = new SynthI();
            QString n;
            n.setNum(s->instances());
            //QString instance_name = s->baseName() + "-" + n;
            QString instance_name = s->name() + "-" + n;
            
            if (si->initInstance(s, instance_name)) {
                  delete si;
                  return 0;
                  }
            }
      else
            printf("createSynthInstance: synthi class:%s label:%s not found\n", sclass.latin1(), label.latin1());
      return si;
      }

//---------------------------------------------------------
//   Synth
//---------------------------------------------------------

//Synth::Synth(const QFileInfo& fi)
//   : info(fi)
//Synth::Synth(const QFileInfo& fi, QString label)
//   : info(fi), _name(label)
Synth::Synth(const QFileInfo& fi, QString label, QString descr, QString maker, QString ver)
   : info(fi), _name(label), _description(descr), _maker(maker), _version(ver)
      {
      _instances = 0;
      }

//---------------------------------------------------------
//   instantiate
//---------------------------------------------------------

//void* MessSynth::instantiate()
void* MessSynth::instantiate(const QString& instanceName)
      {
      ++_instances;
      
      //QString n;
      //n.setNum(_instances);
      //QString instanceName = baseName() + "-" + n;
      
      doSetuid();
      const char* path = info.filePath().latin1();

      // load Synti dll
      void* handle = dlopen(path, RTLD_NOW);
      if (handle == 0) {
            fprintf(stderr, "Synth::instantiate: dlopen(%s) failed: %s\n",
               path, dlerror());
            undoSetuid();
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
                     info.filePath().ascii(), txt);
                  undoSetuid();
                  return 0;
                  }
            }
      _descr = msynth();
      if (_descr == 0) {
            fprintf(stderr, "Synth::instantiate: no MESS descr found\n");
            undoSetuid();
            return 0;
            }
      Mess* mess = _descr->instantiate(sampleRate, muse, &museProject, instanceName.latin1());
      undoSetuid();
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
      setVolume(1.0);
      setPan(0.0);
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
//bool SynthI::putMidiEvent(const MidiPlayEvent& ev) 
{
  if(_writeEnable)
    return _sif->putEvent(ev);
  
  // Hmm, act as if the event went through? 
  //return true;  
  return false;  
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
//   init
//---------------------------------------------------------

//bool MessSynthIF::init(Synth* s)
bool MessSynthIF::init(Synth* s, SynthI* si)
      {
      //_mess = (Mess*)s->instantiate();
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

//SynthIF* MessSynth::createSIF() const
SynthIF* MessSynth::createSIF(SynthI* si)
      {
      //return new MessSynthIF(si);
      
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
      //sif         = s->createSIF();
      //_sif        = s->createSIF(this);
      
      //sif->init(s);

      setName(instanceName);    // set midi device name
      setIName(instanceName);   // set instrument name
      _sif        = s->createSIF(this);
      
      // p3.3.38
      //AudioTrack::setChannels(_sif->channels());
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
//            printf("looking for params\n");
            if (id == 0)
                  break;
//             printf("got parameter:: %s\n", name);
            
            
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

      EventList* iel = midiState();
      if (!iel->empty()) {
            for (iEvent i = iel->begin(); i != iel->end(); ++i) {
                  Event ev = i->second;
                  MidiPlayEvent pev(0, 0, 0, ev);
                  if (_sif->putEvent(pev))
                        break;   // try later
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
      midiDevices.remove(this);
      if (midiPort() != -1) {
            // synthi is attached
            midiPorts[midiPort()].setMidiDevice(0);
            }
      }
//---------------------------------------------------------
//   deactivate3
//---------------------------------------------------------

void SynthI::deactivate3()
      {
      _sif->deactivate3();
      // Moved below by Tim. p3.3.14
      //synthesizer->incInstances(-1);
      
      if(debugMsg)
        printf("SynthI::deactivate3 deleting _sif...\n");
      
      delete _sif;
      _sif = 0;
      
      if(debugMsg)
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
//    search for software synthis and advertise
//---------------------------------------------------------

void initMidiSynth()
      {
      QString s = museGlobalLib + "/synthi";

      QDir pluginDir(s, QString("*.so"), QDir::Files);
      if (debugMsg)
            printf("searching for software synthesizer in <%s>\n", s.latin1());
      if (pluginDir.exists()) {
            const QFileInfoList* list = pluginDir.entryInfoList();
            QFileInfoListIterator it(*list);
            QFileInfo* fi;
            while((fi = it.current())) {
            
            
                  //doSetuid();
                  const char* path = fi->filePath().latin1();
            
                  // load Synti dll
                  void* handle = dlopen(path, RTLD_NOW);
                  if (handle == 0) {
                        //fprintf(stderr, "initMidiSynth: dlopen(%s) failed: %s\n",
                        //  path, dlerror());
                        //undoSetuid();
                        //return 0;
                        ++it;
                        continue;
                        }
                  typedef const MESS* (*MESS_Function)();
                  MESS_Function msynth = (MESS_Function)dlsym(handle, "mess_descriptor");
            
                  if (!msynth) {
                        //const char *txt = dlerror();
                        //if (txt) {
                        //      fprintf(stderr,
                        //        "Unable to find msynth_descriptor() function in plugin "
                        //        "library file \"%s\": %s.\n"
                        //        "Are you sure this is a MESS plugin file?\n",
                        //        info.filePath().ascii(), txt);
                              //undoSetuid();
                              //return 0;
                        //      }
                          dlclose(handle);
                          ++it;
                          continue;
                        }
                  const MESS* descr = msynth();
                  if (descr == 0) {
                        //fprintf(stderr, "Synth::instantiate: no MESS descr found\n");
                        //undoSetuid();
                        //return 0;
                        dlclose(handle);
                        ++it;
                        continue;
                        }
                  //Mess* mess = descr->instantiate(sampleRate, muse, &museProject, instanceName.latin1());
                  //undoSetuid();
                  
            
            
            
                  //synthis.push_back(new MessSynth(*fi));
                  synthis.push_back(new MessSynth(*fi, QString(descr->name), QString(descr->description), QString(""), QString(descr->version)));
                  
                  dlclose(handle);
                  ++it;
                  }
            if (debugMsg)
                  printf("%zd soft synth found\n", synthis.size());
            }
      }

//---------------------------------------------------------
//   createSynthI
//    create a synthesizer instance of class "label"
//---------------------------------------------------------

//SynthI* Song::createSynthI(const QString& sclass)
SynthI* Song::createSynthI(const QString& sclass, const QString& label)
      {
      //printf("Song::createSynthI calling ::createSynthI class:%s\n", sclass.latin1());
      
      //SynthI* si = ::createSynthI(sclass);
      //SynthI* si = ::createSynthI(sclass, label);
      SynthI* si = createSynthInstance(sclass, label);
      if(!si)
        return 0;
      //printf("Song::createSynthI created SynthI. Before insertTrack1...\n");
      
      insertTrack1(si, -1);
      //printf("Song::createSynthI after insertTrack1. Before msgInsertTrack...\n");
      
      msgInsertTrack(si, -1, true);       // add to instance list
      //printf("Song::createSynthI after msgInsertTrack. Before insertTrack3...\n");
      
      insertTrack3(si, -1);

      //printf("Song::createSynthI after insertTrack3. Adding default routes...\n");
      
      OutputList* ol = song->outputs();
      // add default route to master (first audio output)
      if (!ol->empty()) {
            AudioOutput* ao = ol->front();
            // p3.3.38
            //audio->msgAddRoute(Route(si, -1), Route(ao, -1));
            //audio->msgAddRoute(Route((AudioTrack*)si, -1), Route(ao, -1));
            // Make sure the route channel and channels are valid.
            audio->msgAddRoute(Route((AudioTrack*)si, 0, ((AudioTrack*)si)->channels()), Route(ao, 0, ((AudioTrack*)si)->channels()));
            
            audio->msgUpdateSoloStates();
            }
      
      return si;
      }

//---------------------------------------------------------
//   write
//---------------------------------------------------------

void SynthI::write(int level, Xml& xml) const
      {
      xml.tag(level++, "SynthI");
      AudioTrack::writeProperties(level, xml);
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
            xml.tag(level++, "midistate");
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
//   SynthI::read
//---------------------------------------------------------

void SynthI::read(Xml& xml)
      {
      QString sclass;
      QString label;
      
      int port = -1;
      bool startGui = false;
      QRect r;

      for (;;) {
            Xml::Token token = xml.parse();
            const QString tag = xml.s1();
            switch (token) {
                  case Xml::Error:
                  case Xml::End:
                        return;
                  case Xml::TagStart:
                        if (tag == "class")
                              sclass = xml.parse1();
                        else if (tag == "label")
                              label  = xml.parse1();
                        else if (tag == "port")
                              port  = xml.parseInt();
                        else if (tag == "guiVisible")
                              startGui = xml.parseInt();
                        else if (tag == "midistate")
                              readMidiState(xml);
                        else if (tag == "param") {
                              float val = xml.parseFloat();
                              initParams.push_back(val);
                              }
                        else if (tag == "geometry")
                              r = readGeometry(xml, tag);
                        else if (AudioTrack::readProperties(xml, tag))
                              xml.unknown("softSynth");
                        break;
                  case Xml::TagEnd:
                        if (tag == "SynthI") {
                              //Synth* s = findSynth(sclass);
                              Synth* s = findSynth(sclass, label);
                              if (s == 0)
                                    return;
                              if (initInstance(s, name()))
                                    return;
                              song->insertTrack0(this, -1);
                              if (port != -1 && port < MIDI_PORTS)
                                    midiPorts[port].setMidiDevice(this);
                              showGui(startGui);
                              setGeometry(r.x(), r.y(), r.width(), r.height());
                              
                              mapRackPluginsToControllers();
                              
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
              //return _mess->getPatchName(channel, prog, type, drum);
              const char* s = _mess->getPatchName(channel, prog, type, drum);
              if(s)
                return s;
        }      
        return "";
      }

//---------------------------------------------------------
//   populatePatchPopup
//---------------------------------------------------------

void MessSynthIF::populatePatchPopup(QPopupMenu* menu, int ch, MType, bool)
      {
      menu->clear();
      const MidiPatch* mp = _mess->getPatchInfo(ch, 0);
      while (mp) {
            int id = ((mp->hbank & 0xff) << 16)
                      + ((mp->lbank & 0xff) << 8) + mp->prog;
            /*
            int pgid = ((mp->hbank & 0xff) << 8) | (mp->lbank & 0xff) | 0x40000000;          
            int itemnum = menu->indexOf(pgid);
            if(itemnum == -1)
            {
              QPopupMenu* submenu = new QPopupMenu(menu);
              itemnum = 
            }
            */  
            menu->insertItem(QString(mp->name), id);
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
      MidiPort* mp = (p != -1) ? &midiPorts[p] : 0;
      MPEventList* el = playEvents();
               
      iMPEvent ie = nextPlayEvent();
      
      ie = _sif->getData(mp, el, ie, pos, ports, n, buffer);
      
      setNextPlayEvent(ie);
      return true;
      }

iMPEvent MessSynthIF::getData(MidiPort* mp, MPEventList* el, iMPEvent i, unsigned pos, int /*ports*/, unsigned n, float** buffer)
{
      //prevent compiler warning: comparison of signed/unsigned
      int curPos      = pos;
      int endPos      = pos + n;
      int off         = pos;
      int frameOffset = audio->getFrameOffset();

      for (; i != el->end(); ++i) {
          int evTime = i->time(); 
          if (evTime == 0) {
          //      printf("MessSynthIF::getData - time is 0!\n");
          //      continue;
                evTime=frameOffset; // will cause frame to be zero, problem?
                }
          int frame = evTime - frameOffset;

//TODO           if (frame > 0) // robert: ugly fix, don't really know what is going on here
//                          // makes PPC work much better.

               if (frame >= endPos) {
                   printf("frame > endPos!! frame = %d >= endPos %d, i->time() %d, frameOffset %d curPos=%d\n", frame, endPos, i->time(), frameOffset,curPos);
                   continue;
                   }

            if (frame > curPos) {
                  if (frame < pos)
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
      if (midiOutputTrace)
            ev.dump();
      if (_mess)
            return _mess->processEvent(ev);
      return true;
      }
