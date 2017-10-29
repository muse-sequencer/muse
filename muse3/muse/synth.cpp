//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: synth.cpp,v 1.43.2.23 2009/12/15 03:39:58 terminator356 Exp $
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

#include <QMessageBox>

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
#include <stdio.h>

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
#include "midiitransform.h"
#include "mitplugin.h"

// For debugging output: Uncomment the fprintf section.
// REMOVE Tim. autoconnect. Changed. Enabled. Disable when done.
//#define DEBUG_SYNTH(dev, format, args...)  //fprintf(dev, format, ##args);
#define DEBUG_SYNTH(dev, format, args...)  fprintf(dev, format, ##args);

namespace MusEGlobal {
std::vector<MusECore::Synth*> synthis;  // array of available MusEGlobal::synthis
}

namespace MusECore {

extern void connectNodes(AudioTrack*, AudioTrack*);
bool SynthI::_isVisible=false;

const char* synthTypes[] = { "METRONOME", "MESS", "DSSI", "Wine VST", "Native VST (synths)", "Native VST (effects)", "LV2 (synths)", "LV2 (effects)", "UNKNOWN" };
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

//--------------------------------
//  SynthIF
//--------------------------------

void SynthIF::getMapItem(int channel, int patch, int index, DrumMap& dest_map, int
#ifdef _USE_INSTRUMENT_OVERRIDES_
  overrideType
#endif
) const
{
  // Not found? Search the global mapping list.
  const patch_drummap_mapping_list_t* def_pdml = genericMidiInstrument->get_patch_drummap_mapping(channel, true); // Include default.
  if(def_pdml)
  {
    ciPatchDrummapMapping_t ipdm = def_pdml->find(patch, true); // Include default.
    if(ipdm == def_pdml->end())
    {
      // Not found? Is there a default patch mapping?
  #ifdef _USE_INSTRUMENT_OVERRIDES_
      if(overrideType & WorkingDrumMapEntry::InstrumentDefaultOverride)
  #endif
        ipdm = def_pdml->find(CTRL_PROGRAM_VAL_DONT_CARE, true); // Include default.

      if(ipdm != def_pdml->end())
      {
        dest_map = (*ipdm).drummap[index];
        return;
      }
    }
  }

  dest_map = iNewDrumMap[index];;
}

//--------------------------------
// Methods for PluginIBase:
//--------------------------------

Plugin::PluginFeatures SynthIF::requiredFeatures() const { return Plugin::NoFeatures; }
bool SynthIF::on() const                                 { return true; }  // Synth is not part of a rack plugin chain. Always on.
void SynthIF::setOn(bool /*val*/)                        { }
unsigned long SynthIF::pluginID()                        { return 0; }
int SynthIF::id()                                        { return MAX_PLUGINS; } // Set for special block reserved for synth.
QString SynthIF::pluginLabel() const                     { return QString(); }
QString SynthIF::name() const                            { return synti->name(); }
QString SynthIF::lib() const                             { return QString(); }
QString SynthIF::dirPath() const                         { return QString(); }
QString SynthIF::fileName() const                        { return QString(); }
QString SynthIF::titlePrefix() const                     { return QString(); }
MusECore::AudioTrack* SynthIF::track()                   { return static_cast < MusECore::AudioTrack* > (synti); }
void SynthIF::enableController(unsigned long, bool)  { }
bool SynthIF::controllerEnabled(unsigned long) const   { return true;}
void SynthIF::enableAllControllers(bool)               { }
void SynthIF::updateControllers()                        { }
void SynthIF::activate()                                 { }
void SynthIF::deactivate()                               { }
void SynthIF::writeConfiguration(int /*level*/, Xml& /*xml*/)        { }
bool SynthIF::readConfiguration(Xml& /*xml*/, bool /*readPreset*/) { return false; }
unsigned long SynthIF::parameters() const                { return 0; }
unsigned long SynthIF::parametersOut() const             { return 0; }
void SynthIF::setParam(unsigned long, double)       { }
double SynthIF::param(unsigned long) const              { return 0.0; }
double SynthIF::paramOut(unsigned long) const          { return 0.0; }
const char* SynthIF::paramName(unsigned long)          { return NULL; }
const char* SynthIF::paramOutName(unsigned long)       { return NULL; }
LADSPA_PortRangeHint SynthIF::range(unsigned long)
{
  LADSPA_PortRangeHint h;
  h.HintDescriptor = 0;
  h.LowerBound = 0.0;
  h.UpperBound = 1.0;
  return h;
}
LADSPA_PortRangeHint SynthIF::rangeOut(unsigned long)
{
  LADSPA_PortRangeHint h;
  h.HintDescriptor = 0;
  h.LowerBound = 0.0;
  h.UpperBound = 1.0;
  return h;
}
float SynthIF::latency() { return 0.0; }
CtrlValueType SynthIF::ctrlValueType(unsigned long) const { return VAL_LINEAR; }
CtrlList::Mode SynthIF::ctrlMode(unsigned long) const     { return CtrlList::INTERPOLATE; };

//-------------------------------------------------------------------------



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
                (type == Synth::SYNTH_TYPE_END || type == (*i)->synthType() || (type == Synth::LV2_SYNTH && (*i)->synthType() == Synth::LV2_EFFECT)) )
              return *i;
         }
      fprintf(stderr, "synthi type:%d class:%s label:%s not found\n", type, sclass.toLatin1().constData(), label.toLatin1().constData());
      QMessageBox::warning(0,"Synth not found!",
                  "Synth: " + label + " not found, if the project is saved it will be removed from the project");
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
            //Andrew Deryabin: check si->_sif for NULL as synth instance may not be created.
               if (si->initInstance(s, instance_name)) {
                  delete si;
                  fprintf(stderr, "createSynthInstance: synthi class:%s label:%s can not be created\n", sclass.toLatin1().constData(), label.toLatin1().constData());
                  QMessageBox::warning(0,"Synth instantiation error!",
                              "Synth: " + label + " can not be created!");
                  return 0;
               }
            }
      else {
            fprintf(stderr, "createSynthInstance: synthi class:%s label:%s not found\n", sclass.toLatin1().constData(), label.toLatin1().constData());
            QMessageBox::warning(0,"Synth not found!",
                        "Synth: " + label + " not found, if the project is saved it will be removed from the project");
      }

      return si;
      }

//---------------------------------------------------------
//   Synth
//---------------------------------------------------------

Synth::Synth(const QFileInfo& fi, QString label, QString descr, QString maker, QString ver, Plugin::PluginFeatures reqFeatures)
   : info(fi), _name(label), _description(descr), _maker(maker), _version(ver), _requiredFeatures(reqFeatures)
      {
      _requiredFeatures = Plugin::NoFeatures;
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
                     info.filePath().toLatin1().constData(), txt);
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

      // Allow synths to be readable, ie send midi back to the host.
      _rwFlags    = 3;
      _openFlags  = 3;

      _readEnable = false;
      _writeEnable = false;
      setVolume(1.0);
      setPan(0.0);
      }

SynthI::SynthI(const SynthI& si, int flags)
   : AudioTrack(si, flags)
      {
      synthesizer = 0;
      _sif        = 0;

      // Allow synths to be readable, ie send midi back to the host.
      _rwFlags    = 3;
      _openFlags  = 3;

      _readEnable = false;
      _writeEnable = false;
      setVolume(1.0);
      setPan(0.0);

      Synth* s = si.synth();
      if (s) {
            QString n;
            n.setNum(s->instances());
            QString instance_name = s->name() + "-" + n;
            if(!initInstance(s, instance_name)) {  // false if success
                  return;
                  }
            }
      fprintf(stderr, "SynthI copy ctor: error initializing synth s:%p\n", s);
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

  _state = QString("OK");
  return _state;
}

//---------------------------------------------------------
//   close
//---------------------------------------------------------

void SynthI::close()
{
  _readEnable = false;
  _writeEnable = false;
  _state = QString("Closed");
}

// REMOVE Tim. autoconnect. Removed.
// //---------------------------------------------------------
// //   putMidiEvent
// //---------------------------------------------------------
// 
// bool SynthI::putEvent(const MidiPlayEvent& ev)
// {
//   if(_writeEnable)
//   {
//     if (MusEGlobal::midiOutputTrace)
//     {
//           fprintf(stderr, "MidiOut: Synth: <%s>: ", name().toLatin1().constData());
//           ev.dump();
//     }
//     return _sif->putEvent(ev);
//   }
// 
//   return false;
// }

//---------------------------------------------------------
//   processMidi
//---------------------------------------------------------

void SynthI::processMidi(unsigned int /*curFrame*/)
{
// REMOVE Tim. autoconnect. Removed.
//     processStuckNotes();
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
//   recordEvent
//---------------------------------------------------------

void SynthI::recordEvent(MidiRecordEvent& event)
      {
      if(MusEGlobal::audio->isPlaying())
        event.setLoopNum(MusEGlobal::audio->loopCount());

      if (MusEGlobal::midiInputTrace) {
            fprintf(stderr, "MidiInput from synth: ");
            event.dump();
            }

      int typ = event.type();

      if(_port != -1)
      {
        int idin = MusEGlobal::midiPorts[_port].syncInfo().idIn();

        //---------------------------------------------------
        // filter some SYSEX events
        //---------------------------------------------------

        if (typ == ME_SYSEX) {
              const unsigned char* p = event.data();
              int n = event.len();
              if (n >= 4) {
                    if ((p[0] == 0x7f)
                      && ((p[1] == 0x7f) || (idin == 0x7f) || (p[1] == idin))) {
                          if (p[2] == 0x06) {
                                MusEGlobal::midiSyncContainer.mmcInput(_port, p, n);
                                return;
                                }
                          if (p[2] == 0x01) {
                                MusEGlobal::midiSyncContainer.mtcInputFull(_port, p, n);
                                return;
                                }
                          }
                    else if (p[0] == 0x7e) {
                          MusEGlobal::midiSyncContainer.nonRealtimeSystemSysex(_port, p, n);
                          return;
                          }
                    }
          }
          else
            // Trigger general activity indicator detector. Sysex has no channel, don't trigger.
            MusEGlobal::midiPorts[_port].syncInfo().trigActDetect(event.channel());
      }

      //
      //  process midi event input filtering and
      //    transformation
      //

      processMidiInputTransformPlugins(event);

      if (filterEvent(event, MusEGlobal::midiRecordType, false))
            return;

      if (!applyMidiInputTransformation(event)) {
            if (MusEGlobal::midiInputTrace)
                  fprintf(stderr, "   midi input transformation: event filtered\n");
            return;
            }

// TODO Maybe support this later, but for now it's not a good idea to control from the synths.
//      Especially since buggy ones may repeat events multiple times.
#if 1
      //
      // transfer noteOn and Off events to gui for step recording and keyboard
      // remote control (changed by flo93: added noteOff-events)
      //
      if (typ == ME_NOTEON) {
            int pv = ((event.dataA() & 0xff)<<8) + (event.dataB() & 0xff);
            MusEGlobal::song->putEvent(pv);
            }
      else if (typ == ME_NOTEOFF) {
            int pv = ((event.dataA() & 0xff)<<8) + (0x00); //send an event with velo=0
            MusEGlobal::song->putEvent(pv);
            }
#endif

      // Do not bother recording if it is NOT actually being used by a port.
      // Because from this point on, process handles things, by selected port.
      if(_port == -1)
        return;

      // Split the events up into channel fifos. Special 'channel' number 17 for sysex events.
      unsigned int ch = (typ == ME_SYSEX)? MIDI_CHANNELS : event.channel();
      if(_recordFifo[ch].put(event))
        fprintf(stderr, "SynthI::recordEvent: fifo channel %d overflow\n", ch);
      }


RouteCapabilitiesStruct SynthI::routeCapabilities() const 
{ 
  RouteCapabilitiesStruct s = AudioTrack::routeCapabilities();
  s._trackChannels._inChannels = totalInChannels();
  s._trackChannels._inRoutable = (s._trackChannels._inChannels != 0);
  return s;
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

      //Andrew Deryabin: add check for NULL here to get rid of segfaults
      if(_sif == NULL)
      {
         return true; //true if error (?)
      }

      AudioTrack::setTotalOutChannels(_sif->totalOutChannels());
      AudioTrack::setTotalInChannels(_sif->totalInChannels());

      //---------------------------------------------------
      //  read available controller from synti
      //---------------------------------------------------

      int id = 0;
      MidiControllerList* cl = MidiInstrument::controller();
      for (;;) {
            QString name;
            int ctrl;
            int min;
            int max;
            int initval = CTRL_VAL_UNKNOWN;
            id = _sif->getControllerInfo(id, &name, &ctrl, &min, &max, &initval);
            if (id == 0)
                  break;
            // Override existing program controller.
            iMidiController i = cl->end();
            if(ctrl == CTRL_PROGRAM)
            {
              for(i = cl->begin(); i != cl->end(); ++i)
              {
                if(i->second->num() == CTRL_PROGRAM)
                {
                  delete i->second;
                  cl->del(i);
                  break;
                }
              }
            }

            MidiController* c = new MidiController(name, ctrl, min, max, initval, initval);
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
// REMOVE Tim. autoconnect. Changed.
//                   if (_sif->putEvent(pev))
                  //if(!addScheduledEvent(pev))
                  if(!_eventFifos->put(PlayFifo, pev))
                        break;   // try later
                  }
            iel->clear();
            }

      unsigned long idx = 0;
      for (std::vector<double>::iterator i = initParams.begin(); i != initParams.end(); ++i, ++idx)
            _sif->setParameter(idx, *i);

      // p3.3.40 Since we are done with the (sometimes huge) initial parameters list, clear it.
      // TODO: Decide: Maybe keep them around for a 'reset to previously loaded values' (revert) command? ...
      initParams.clear();

      //call SynthIF::setCustomData(...) with accumulated custom params
      _sif->setCustomData(accumulatedCustomParams);

      accumulatedCustomParams.clear();

      return false;
      }

//---------------------------------------------------------
//   pbForwardShiftFrames
//---------------------------------------------------------

unsigned int SynthI::pbForwardShiftFrames() const
{
  return MusEGlobal::segmentSize;
}
      
//---------------------------------------------------------
//   getControllerInfo
//---------------------------------------------------------

int MessSynthIF::getControllerInfo(int id, QString* name, int* ctrl, int* min, int* max, int* initval)
      {
      return _mess->getControllerInfo(id, name, ctrl, min, max, initval);
      }

void MessSynthIF::getMapItem(int channel, int patch, int index, DrumMap& dest_map, int
#ifdef _USE_INSTRUMENT_OVERRIDES_
  overrideType
#endif
) const
{
  // Could just call the ancestor, but we can save the double string copy by optimizing below...
  // SynthIF::getMapItem(channel, patch, index, dest_map);

  DrumMap* dm = NULL;
  // Not found? Search the global mapping list.
  patch_drummap_mapping_list_t* def_pdml = genericMidiInstrument->get_patch_drummap_mapping(channel, true); // Include default.
  if(def_pdml)
  {
    ciPatchDrummapMapping_t ipdm = def_pdml->find(patch, true); // Include default.
    if(ipdm == def_pdml->end())
    {
      // Not found? Is there a default patch mapping?
      #ifdef _USE_INSTRUMENT_OVERRIDES_
      if(overrideType & WorkingDrumMapEntry::InstrumentDefaultOverride)
      #endif
        ipdm = def_pdml->find(CTRL_PROGRAM_VAL_DONT_CARE, true); // Include default.

      if(ipdm != def_pdml->end())
        dm = &(*ipdm).drummap[index];
    }
  }
  if(!dm)
    dm = &iNewDrumMap[index];
  DrumMap& base_dm = *dm;
  dest_map.vol = base_dm.vol;
  dest_map.quant = base_dm.quant;
  dest_map.len = base_dm.len;
  dest_map.anote = base_dm.anote;
  dest_map.enote = base_dm.enote;
  dest_map.channel = base_dm.channel;
  dest_map.port = base_dm.port;
  dest_map.lv1 = base_dm.lv1;
  dest_map.lv2 = base_dm.lv2;
  dest_map.lv3 = base_dm.lv3;
  dest_map.lv4 = base_dm.lv4;
  dest_map.hide = base_dm.hide;
  dest_map.mute = base_dm.mute;

  QString str;
  // true = Want percussion names, not melodic.
  if(_mess->getNoteSampleName(true, channel, patch, index, &str))
    dest_map.name = str;
  else
    dest_map.name = base_dm.name;
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

      //Andrew Deryabin: add checks for NULLness of _sif and syntheeizer instances
      if(_sif)
      {
         _sif->deactivate3();
      }

      //synthesizer->incInstances(-1); // Moved below by Tim. p3.3.14

      if(MusEGlobal::debugMsg)
        fprintf(stderr, "SynthI::deactivate3 deleting _sif...\n");

      if(_sif)
      {
         delete _sif;
         _sif = 0;
      }

      if(MusEGlobal::debugMsg)
        fprintf(stderr, "SynthI::deactivate3 decrementing synth instances...\n");

      if(synthesizer)
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
//   initMidiSynth
//    search for software MusEGlobal::synthis and advertise
//---------------------------------------------------------

void initMidiSynth()
      {
      QString s = MusEGlobal::museGlobalLib + "/synthi";

      QDir pluginDir(s, QString("*.so")); // ddskrjo
      if (MusEGlobal::debugMsg)
            fprintf(stderr, "searching for software synthesizer in <%s>\n", s.toLatin1().constData());
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
                  fprintf(stderr, "%zd soft synth found\n", MusEGlobal::synthis.size());
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

      OutputList* ol = MusEGlobal::song->outputs();
      // Add an omnibus default route to master (first audio output)
      if (!ol->empty()) {
            AudioOutput* ao = ol->front();
            // AddTrack operation 'mirrors' the route.
            static_cast<Track*>(si)->outRoutes()->push_back(Route(ao));
            }
      
      MusEGlobal::song->applyOperation(UndoOp(UndoOp::AddTrack, idx, si));

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
      xml.strTag(level, "label", synth()->name());

      if(openFlags() != 1)
        xml.intTag(level, "openFlags", openFlags());
            
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
      int oflags = 1;

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
                        else if (tag == "openFlags")
                              oflags = xml.parseInt();
                        
                        else if (tag == "port")
                              port  = xml.parseInt();
                        else if (tag == "guiVisible")
                              startgui = xml.parseInt();
                        else if (tag == "nativeGuiVisible")
                              startngui = xml.parseInt();
                        else if (tag == "midistate")
                              readMidiState(xml);
                        else if (tag == "param") {
                              double val = xml.parseDouble();
                              initParams.push_back(val);
                              }
                        else if (tag == "stringParam")
                              _stringParamMap.read(xml, tag);
                        else if (tag == "geometry")
                              r = readGeometry(xml, tag);
                        else if (tag == "nativeGeometry")
                              nr = readGeometry(xml, tag);
                        else if (tag == "customData") { //just place tag contents in accumulatedCustomParams
                              QString customData = xml.parse1();
                              if(!customData.isEmpty()){
                                 accumulatedCustomParams.push_back(customData);
                              }
                        }
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
                              setOpenFlags(oflags);
                              
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

QString MessSynthIF::getPatchName(int channel, int prog, bool drum) const
      {
        if (_mess)
          return _mess->getPatchName(channel, prog, drum);
        return "";
      }

//---------------------------------------------------------
//   populatePatchPopup
//---------------------------------------------------------

void MessSynthIF::populatePatchPopup(MusEGui::PopupMenu* menu, int ch, bool)
      {
      MusEGui::PopupMenu* hbank_menu = 0;
      MusEGui::PopupMenu* lbank_menu = 0;
      menu->clear();
      const MidiPatch* mp = _mess->getPatchInfo(ch, 0);
      while (mp) {
            if(mp->typ == MP_TYPE_HBANK)
            {
              lbank_menu = 0;
              hbank_menu = new MusEGui::PopupMenu(QString(mp->name),  menu, true);
              menu->addMenu(hbank_menu);
            }
            else
            if(mp->typ == MP_TYPE_LBANK)
            {
              lbank_menu = new MusEGui::PopupMenu(QString(mp->name),  menu, true);
              hbank_menu->addMenu(lbank_menu);
            }
            else
            {
              int id = ((mp->hbank & 0xff) << 16)
                        + ((mp->lbank & 0xff) << 8) + mp->prog;
              MusEGui::PopupMenu* m;
              if(lbank_menu)
                m = lbank_menu;
              else if(hbank_menu)
                m = hbank_menu;
              else
                m = menu;
              QAction *act = m->addAction(QString(mp->name));
              act->setData(id);
            }
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
// REMOVE Tim. autoconnect. Changed.
//   if(off())
//   {
//     // Clear any accumulated play events.
//     //playEvents()->clear(); DELETETHIS
//     _playEvents.clear();
//     // Eat up any fifo events.
//     //while(!eventFifo.isEmpty())  DELETETHIS
//     //  eventFifo.get();
//     eventFifo.clear();  // Clear is the same but faster AND safer, right?
//   }
  if(off() || midiPort() < 0 || midiPort() >= MIDI_PORTS)
  {
//     // Clear any accumulated play events.
//     _playEvents.clear();
//     // Eat up any fifo events.
//     eventFifo.clearRead();
// //     _osc2AudioFifo->clearRead();
    // Eat up any fifo events.
    _eventFifos->clearRead();
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

// REMOVE Tim. autoconnect. Changed.
//       iMPEvent ie = _playEvents.begin();
//       ie = _sif->getData(mp, &_playEvents, ie, pos, ports, n, buffer);
      //MPEventList dummy_mpel;
      //iMPEvent ie = dummy_mpel.begin();
      //ie = _sif->getData(mp, &dummy_mpel, ie, pos, ports, n, buffer);
      _sif->getData(mp, pos, ports, n, buffer);

      // p4.0.15 We are done with these events. Let us erase them here instead of Audio::processMidi.
      // That way we can simply set the next play event to the beginning.
      // This also allows other events to be inserted without the problems caused by the next play event
      //  being at the 'end' iterator and not being *easily* set to some new place beginning of the newer insertions.
      // The way that MPEventList sorts made it difficult to predict where the iterator of the first newly inserted items was.
      // The erasure in Audio::processMidi was missing some events because of that.
// REMOVE Tim. autoconnect. Removed.
//       _playEvents.erase(_playEvents.begin(), ie);

      return true;
      }

// REMOVE Tim. autoconnect. Changed.
// iMPEvent MessSynthIF::getData(MidiPort* /*mp*/, MPEventList* el, iMPEvent i, unsigned pos, int /*ports*/, unsigned n, float** buffer)
// {
//       //prevent compiler warning: comparison of signed/unsigned
// // REMOVE Tim. autoconnect. Changed.
// //       unsigned curPos      = pos;
// //       unsigned endPos      = pos + n;
// //       unsigned off         = pos;
// //       unsigned long frameOffset = MusEGlobal::audio->getFrameOffset();
//       unsigned int curPos      = 0;
//       unsigned int endPos      = n;
//       unsigned int syncFrame = MusEGlobal::audio->curSyncFrame();
// 
//       for (; i != el->end(); ++i) {
// //           unsigned evTime = i->time();
//           const MidiPlayEvent ev = *i;
//           unsigned evTime = ev.time();
// //           if (evTime == 0)
// //                 evTime=frameOffset; // will cause frame to be zero, problem?
// 
// //           unsigned frame = evTime - frameOffset;
//           //unsigned frame = (evTime < syncFrame) ? 0 : evTime - syncFrame;
//           unsigned frame;
//           if(evTime < syncFrame)
//           {
//                 fprintf(stderr, "evTime:%u < syncFrame:%u!! curPos=%d\n", 
//                         evTime, syncFrame, curPos);
//                 frame = 0;
//           }
//           else
//           {
//             frame = evTime - syncFrame;
//           }
// 
//             if (frame >= endPos) {
// //                 fprintf(stderr, "frame > endPos!! frame = %d >= endPos %d, i->time() %u, frameOffset %lu curPos=%d\n", 
// //                         frame, endPos, i->time(), frameOffset,curPos);
//                 fprintf(stderr, "frame > endPos!! frame = %u >= endPos %d, evTime %u, syncFrame %u curPos=%d\n", 
//                         frame, endPos, evTime, syncFrame, curPos);
//                 continue;
//                 }
// 
//             if (frame > curPos) {
// //                   if (frame < pos)
// //                   if (evTime < syncFrame)
// //                         fprintf(stderr, "should not happen: missed event %d\n", evTime);
// //                   else
// //                   {
//                         if (!_mess)
//                               fprintf(stderr, "should not happen - no _mess\n");
//                         else
//                         {
// //                                 _mess->process(pos, buffer, curPos-pos, frame - curPos);
//                                 _mess->process(pos, buffer, curPos, frame - curPos);
//                         }
// //                   }
//                   curPos = frame;
//             }
// 
// // //             if (mp)
// // //                   mp->sendEvent(*i);
// // //             else {
// // //                   if (putEvent(*i))
// // //                         break;
// //             //ev.setTime(frame);
// //             if (mp)
// //                   mp->sendEvent(ev);
// //             else {
// //                   if (putEvent(ev))
// //                         break;
// //             }
//             putEvent(ev);
//       }
// 
// //       if (endPos - curPos)
//       if (endPos > curPos)
//       {
//             if (!_mess)
//                   fprintf(stderr, "should not happen - no _mess\n");
//             else
//             {
// //                     _mess->process(pos, buffer, curPos - off, endPos - curPos);
//                     _mess->process(pos, buffer, curPos, endPos - curPos);
//             }
//       }
//       return i;
// }

// REMOVE Tim. autoconnect. Changed.
// iMPEvent MessSynthIF::getData(MidiPort* /*mp*/, MPEventList* /*el*/, iMPEvent i, unsigned pos, int /*ports*/, unsigned n, float** buffer)
bool MessSynthIF::getData(MidiPort* /*mp*/, unsigned pos, int /*ports*/, unsigned n, float** buffer)
{
      const unsigned int syncFrame = MusEGlobal::audio->curSyncFrame();
      unsigned int curPos = 0;
      unsigned int frame = 0;

      // This also takes an internal snapshot of the size for use later...
      // False = don't use the size snapshot, but update it.
      const int sz = synti->eventFifos()->getSize(false);
      for(int i = 0; i < sz; ++i)
      {  
        // True = use the size snapshot.
        const MidiPlayEvent& ev(synti->eventFifos()->peek(true)); 
        const unsigned int evTime = ev.time();
        if(evTime < syncFrame)
        {
          fprintf(stderr, "MessSynthIF::getData() evTime:%u < syncFrame:%u!! curPos=%d\n", 
                  evTime, syncFrame, curPos);
          frame = 0;
        }
        else
          frame = evTime - syncFrame;

        // Event is for future?
        if(frame >= n) 
        {
          DEBUG_SYNTH(stderr, "MessSynthIF::getData(): Event for future, breaking loop: frame:%u n:%d evTime:%u syncFrame:%u curPos:%d\n", 
                  frame, n, evTime, syncFrame, curPos);
          //continue;
          break;
        }

//         // Done with ring buffer event. Remove it from FIFO.
//         // True = use the size snapshot.
//         synti->eventFifos()->remove(true);
        
        if(frame > curPos)
        {
          if (!_mess)
            fprintf(stderr, "MessSynthIF::getData() should not happen - no _mess\n");
          else
            _mess->process(pos, buffer, curPos, frame - curPos);
          curPos = frame;
        }
        
        // If putEvent fails, although we would like to not miss events by keeping them
        //  until next cycle and trying again, that can lead to a large backup of events
        //  over a long time. So we'll just... miss them.
        //putEvent(ev);
        //synti->putEvent(ev);
        processEvent(ev);
        
        // Done with ring buffer event. Remove it from FIFO.
        // True = use the size snapshot.
        synti->eventFifos()->remove(true);
      }

      if(curPos < n)
      {
        if (!_mess)
          fprintf(stderr, "MessSynthIF::getData() should not happen - no _mess\n");
        else
          _mess->process(pos, buffer, curPos, n - curPos);
      }
      
// REMOVE Tim. autoconnect. Changed.
//       return i;
      return true;
      


//       for (; i != el->end(); ++i) {
// //           unsigned evTime = i->time();
//           const MidiPlayEvent ev = *i;
//           unsigned evTime = ev.time();
// //           if (evTime == 0)
// //                 evTime=frameOffset; // will cause frame to be zero, problem?
// 
// //           unsigned frame = evTime - frameOffset;
//           //unsigned frame = (evTime < syncFrame) ? 0 : evTime - syncFrame;
//           unsigned frame;
//           if(evTime < syncFrame)
//           {
//                 fprintf(stderr, "evTime:%u < syncFrame:%u!! curPos=%d\n", 
//                         evTime, syncFrame, curPos);
//                 frame = 0;
//           }
//           else
//           {
//             frame = evTime - syncFrame;
//           }
// 
//             if (frame >= endPos) {
// //                 fprintf(stderr, "frame > endPos!! frame = %d >= endPos %d, i->time() %u, frameOffset %lu curPos=%d\n", 
// //                         frame, endPos, i->time(), frameOffset,curPos);
//                 fprintf(stderr, "frame > endPos!! frame = %u >= endPos %d, evTime %u, syncFrame %u curPos=%d\n", 
//                         frame, endPos, evTime, syncFrame, curPos);
//                 continue;
//                 }
// 
//             if (frame > curPos) {
// //                   if (frame < pos)
// //                   if (evTime < syncFrame)
// //                         fprintf(stderr, "should not happen: missed event %d\n", evTime);
// //                   else
// //                   {
//                         if (!_mess)
//                               fprintf(stderr, "should not happen - no _mess\n");
//                         else
//                         {
// //                                 _mess->process(pos, buffer, curPos-pos, frame - curPos);
//                                 _mess->process(pos, buffer, curPos, frame - curPos);
//                         }
// //                   }
//                   curPos = frame;
//             }
// 
// // //             if (mp)
// // //                   mp->sendEvent(*i);
// // //             else {
// // //                   if (putEvent(*i))
// // //                         break;
// //             //ev.setTime(frame);
// //             if (mp)
// //                   mp->sendEvent(ev);
// //             else {
// //                   if (putEvent(ev))
// //                         break;
// //             }
//             putEvent(ev);
//       }
// 
// //       if (endPos - curPos)
//       if (endPos > curPos)
//       {
//             if (!_mess)
//                   fprintf(stderr, "should not happen - no _mess\n");
//             else
//             {
// //                     _mess->process(pos, buffer, curPos - off, endPos - curPos);
//                     _mess->process(pos, buffer, curPos, endPos - curPos);
//             }
//       }
//       return i;
}

//---------------------------------------------------------
//   putEvent
//    return true on error (busy)
//---------------------------------------------------------

// REMOVE Tim. autoconnect. Changed.
//bool MessSynthIF::putEvent(const MidiPlayEvent& ev)
bool MessSynthIF::processEvent(const MidiPlayEvent& ev)
{
      if (!_mess)
        return true;
      
      if (MusEGlobal::midiOutputTrace)
      {
           fprintf(stderr, "MidiOut: MESS: <%s>: ", synti->name().toLatin1().constData());
           ev.dump();
      }
      
      int chn = ev.channel();
      int a = ev.dataA();
      int b = ev.dataB();
      
      switch(ev.type())
      {
        // Special for program, hi bank, and lo bank: Virtually all synths encapsulate banks and program together
        //  call rather than breaking them out into three separate controllers. Therefore we need to 'compose' a
        //  CTRL_PROGRAM which supports the full complement of hi/lo bank and program. The synths should therefore NEVER
        //  be allowed to receive ME_PROGRAM or CTRL_HBANK or CTRL_LBANK alone (it also saves them the management trouble)...
        // TODO: Try to move this into the individual synths and since we must not talk directly to them, rely on feedback
        //        from them (in midi.cpp) to update our HOST current program absolutely when they change their own program !
        case ME_PROGRAM:
          {
            int hb;
            int lb;
            synti->currentProg(chn, NULL, &lb, &hb);
            synti->setCurrentProg(chn, a & 0xff, lb, hb);
            // Only if there's something to change...
            //if(hb < 128 || lb < 128 || a < 128)
            //{
              if(hb > 127) // Map "dont care" to 0
                hb = 0;
              if(lb > 127)
                lb = 0;
              if(a > 127)
                a = 0;
              // REMOVE Tim. autoconnect. Changed. Oops! Wrong direction!
              //int full_prog = (hb >> 16) | (lb >> 8) | pr;
              const int full_prog = (hb << 16) | (lb << 8) | a;
              return _mess->processEvent(MidiPlayEvent(ev.time(), ev.port(), chn, ME_CONTROLLER, CTRL_PROGRAM, full_prog));
            //}
            //return false;
          }
          break;
        case ME_CONTROLLER:
          {
            // Our internal hwCtrl controllers support the 'unknown' value.
            // Don't send 'unknown' values to the driver. Ignore and return no error.
            if(b == CTRL_VAL_UNKNOWN)
              return false;
            
            if(a == CTRL_PROGRAM)
            {
              int hb = (b >> 16) & 0xff;
              int lb = (b >> 8)  & 0xff;
              int pr = b & 0xff;
              synti->setCurrentProg(chn, pr, lb, hb);
              // Only if there's something to change...
              //if(hb < 128 || lb < 128 || pr < 128)
              //{
                if(hb > 127)
                  hb = 0;
                if(lb > 127)
                  lb = 0;
                if(pr > 127)
                  pr = 0;
                const int full_prog = (hb << 16) | (lb << 8) | pr;
                return _mess->processEvent(MidiPlayEvent(ev.time(), ev.port(), chn, ME_CONTROLLER, CTRL_PROGRAM, full_prog));
              //}
              //return false;
            }
            
            if(a == CTRL_HBANK)
            {
              int lb;
              int pr;
              synti->currentProg(chn, &pr, &lb, NULL);
              synti->setCurrentProg(chn, pr, lb, b & 0xff);
              // Only if there's something to change...
              //if(b < 128 || lb < 128 || pr < 128)
              //{
                if(b > 127)
                  b = 0;
                if(lb > 127)
                  lb = 0;
                if(pr > 127)
                  pr = 0;
                // REMOVE Tim. autoconnect. Changed. Oops! Wrong direction!
                //int full_prog = (hb << 16) | (lb >> 8) | pr;
                const int full_prog = (b << 16) | (lb << 8) | pr;
                return _mess->processEvent(MidiPlayEvent(ev.time(), ev.port(), chn, ME_CONTROLLER, CTRL_PROGRAM, full_prog));
              //}
              //return false;
            }
            
            if(a == CTRL_LBANK)
            {
              int hb;
              int pr;
              synti->currentProg(chn, &pr, NULL, &hb);
              synti->setCurrentProg(chn, pr, b & 0xff, hb);
              // Only if there's something to change...
              //if(hb < 128 || b < 128 || pr < 128)
              //{
                if(hb > 127)
                  hb = 0;
                if(b > 127)
                  b = 0;
                if(pr > 127)
                  pr = 0;
                // REMOVE Tim. autoconnect. Changed. Oops! Wrong direction!
                //int full_prog = (hb >> 16) | (lb << 8) | pr;
                const int full_prog = (hb << 16) | (b << 8) | pr;
                return _mess->processEvent(MidiPlayEvent(ev.time(), ev.port(), chn, ME_CONTROLLER, CTRL_PROGRAM, full_prog));
              //}
              //return false;
            }
          }
          break;

        default:
          break;
      }
      return _mess->processEvent(ev);
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
