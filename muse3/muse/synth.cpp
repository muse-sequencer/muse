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
#ifndef _WIN32
#include <sys/wait.h>
#include <sys/mman.h>
#endif
#include <signal.h>
#include <pthread.h>
#include <unistd.h>
#include <errno.h>
#include <vector>
#include <fcntl.h>
#include <dlfcn.h>
#include <stdio.h>
#include <string>

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
#include "midiitransform.h"
#include "mitplugin.h"
#include "helper.h"
#include "gconfig.h"
#include "globals.h"
#include "plugin_list.h"
#include "pluglist.h"
#include "ticksynth.h"

// Undefine if and when multiple output routes are added to midi tracks.
#define _USE_MIDI_TRACK_SINGLE_OUT_PORT_CHAN_

// For debugging output: Uncomment the fprintf section.
#define DEBUG_SYNTH(dev, format, args...)  //fprintf(dev, format, ##args);

namespace MusEGlobal {
  MusECore::SynthList synthis;  // array of available MusEGlobal::synthis
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


//---------------------------------------------------------
//   find
//---------------------------------------------------------

Synth* SynthList::find(const QString& fileCompleteBaseName, const QString& pluginName) const
      {
      for (ciSynthList i = begin(); i != end(); ++i) {
            if ((fileCompleteBaseName == (*i)->completeBaseName()) && (pluginName == (*i)->name()))
                  return *i;
            }

      return 0;
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

inline PluginFeatures_t SynthIF::requiredFeatures() const       { return PluginNoFeatures; }
inline bool SynthIF::on() const                                 { return true; }  // Synth is not part of a rack plugin chain. Always on.
inline void SynthIF::setOn(bool /*val*/)                        { }
inline unsigned long SynthIF::pluginID()                        { return 0; }
inline int SynthIF::id()                                        { return MusECore::MAX_PLUGINS; } // Set for special block reserved for synth.
inline QString SynthIF::pluginLabel() const                     { return QString(); }
inline QString SynthIF::name() const                            { return synti->name(); }
inline QString SynthIF::lib() const                             { return QString(); }
inline QString SynthIF::dirPath() const                         { return QString(); }
inline QString SynthIF::fileName() const                        { return QString(); }
inline QString SynthIF::titlePrefix() const                     { return QString(); }
inline MusECore::AudioTrack* SynthIF::track()                   { return static_cast < MusECore::AudioTrack* > (synti); }
inline void SynthIF::enableController(unsigned long, bool)  { }
inline bool SynthIF::controllerEnabled(unsigned long) const   { return true;}
inline void SynthIF::enableAllControllers(bool)               { }
inline void SynthIF::updateControllers()                        { }
inline void SynthIF::activate()                                 { }
inline void SynthIF::deactivate()                               { }
inline void SynthIF::writeConfiguration(int /*level*/, Xml& /*xml*/)        { }
inline bool SynthIF::readConfiguration(Xml& /*xml*/, bool /*readPreset*/) { return false; }
inline unsigned long SynthIF::parameters() const                { return 0; }
inline unsigned long SynthIF::parametersOut() const             { return 0; }
inline void SynthIF::setParam(unsigned long, double)       { }
inline double SynthIF::param(unsigned long) const              { return 0.0; }
inline double SynthIF::paramOut(unsigned long) const          { return 0.0; }
inline const char* SynthIF::paramName(unsigned long)          { return NULL; }
inline const char* SynthIF::paramOutName(unsigned long)       { return NULL; }
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
inline bool SynthIF::hasLatencyOutPort() const { return false; }
inline unsigned long SynthIF::latencyOutPortIndex() const { return 0; }
inline float SynthIF::latency() const { return 0.0; }
inline CtrlValueType SynthIF::ctrlValueType(unsigned long) const { return VAL_LINEAR; }
inline CtrlList::Mode SynthIF::ctrlMode(unsigned long) const     { return CtrlList::INTERPOLATE; };

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

void MessSynthIF::guiHeartBeat()
{
  if(_mess)
    _mess->guiHeartBeat();
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

Synth::Synth(const QFileInfo& fi, QString label, QString descr, QString maker, QString ver, PluginFeatures_t reqFeatures)
   : info(fi), _name(label), _description(descr), _maker(maker), _version(ver), _requiredFeatures(reqFeatures)
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
            
      MESS_Descriptor_Function msynth = (MESS_Descriptor_Function)dlsym(handle, "mess_descriptor");
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
      QByteArray configPathBA      = MusEGlobal::configPath.toLatin1();
      QByteArray cachePathBA       = MusEGlobal::cachePath.toLatin1();
      QByteArray museGlobalLibBA   = MusEGlobal::museGlobalLib.toLatin1();
      QByteArray museGlobalShareBA = MusEGlobal::museGlobalShare.toLatin1();
      QByteArray museUserBA        = MusEGlobal::museUser.toLatin1();
      QByteArray museProjectBA     = MusEGlobal::museProject.toLatin1();
      MessConfig mcfg(MusEGlobal::segmentSize,
                      MusEGlobal::sampleRate,
                      MusEGlobal::config.minMeter,
                      MusEGlobal::config.useDenormalBias,
                      MusEGlobal::denormalBias,
                      MusEGlobal::config.leftMouseButtonCanDecrease,
                      configPathBA.constData(),
                      cachePathBA.constData(),
                      museGlobalLibBA.constData(),
                      museGlobalShareBA.constData(),
                      museUserBA.constData(),
                      museProjectBA.constData());
      Mess* mess = _descr->instantiate((unsigned long long)MusEGlobal::muse->winId(),
                                       instanceName.toLatin1().constData(), &mcfg);
      
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

//---------------------------------------------------------
//   processMidi
//---------------------------------------------------------

void SynthI::processMidi(unsigned int /*curFrame*/)
{
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
            dumpMPEvent(&event);
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
      unsigned int ch = (typ == ME_SYSEX)? MusECore::MUSE_MIDI_CHANNELS : event.channel();
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

                  MidiPlayEvent pev = ev.asMidiPlayEvent(0, 0, 0);
                  _userEventBuffers->put(pev);
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
      int i_ctrl;
      int i_min;
      int i_max;
      int i_initval;
      const char* s_name;
      
      int ret = _mess->getControllerInfo(id, &s_name, &i_ctrl, &i_min, &i_max, &i_initval);
      
      if(ctrl)
        *ctrl = i_ctrl;
      if(min)
        *min = i_min;
      if(max)
        *max = i_max;
      if(initval)
        *initval = i_initval;
      if(name)
        *name = QString(s_name);
      
      return ret;
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

  const char* str;
  // true = Want percussion names, not melodic.
  if(_mess->getNoteSampleName(true, channel, patch, index, &str))
    dest_map.name = QString(str);
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
  const MusEPlugin::PluginScanList& scan_list = MusEPlugin::pluginList;
  for(MusEPlugin::ciPluginScanList isl = scan_list.begin(); isl != scan_list.end(); ++isl)
  {
    const MusEPlugin::PluginScanInfoRef inforef = *isl;
    const MusEPlugin::PluginScanInfoStruct& info = inforef->info();
    switch(info._type)
    {
      case MusEPlugin::PluginScanInfoStruct::PluginTypeMESS:
      {
        if(MusEGlobal::loadMESS)
        {
          // Make sure it doesn't already exist.
          if(const Synth* sy = MusEGlobal::synthis.find(PLUGIN_GET_QSTRING(info._completeBaseName),
              PLUGIN_GET_QSTRING(info._name)))
          {
            fprintf(stderr, "Ignoring MESS synth name:%s path:%s duplicate of path:%s\n",
                    PLUGIN_GET_CSTRING(info._name),
                    PLUGIN_GET_CSTRING(info.filePath()),
                    sy->filePath().toLatin1().constData());
          }
          else
          {
            MusEGlobal::synthis.push_back(
              new MessSynth(PLUGIN_GET_QSTRING(info.filePath()),
                            PLUGIN_GET_QSTRING(info._name),
                            PLUGIN_GET_QSTRING(info._description),
                            QString(""),
                            PLUGIN_GET_QSTRING(info._version)));
          }
        }
      }
      break;
      
      case MusEPlugin::PluginScanInfoStruct::PluginTypeLADSPA:
      case MusEPlugin::PluginScanInfoStruct::PluginTypeDSSI:
      case MusEPlugin::PluginScanInfoStruct::PluginTypeDSSIVST:
      case MusEPlugin::PluginScanInfoStruct::PluginTypeVST:
      case MusEPlugin::PluginScanInfoStruct::PluginTypeLV2:
      case MusEPlugin::PluginScanInfoStruct::PluginTypeLinuxVST:
      case MusEPlugin::PluginScanInfoStruct::PluginTypeUnknown:
      case MusEPlugin::PluginScanInfoStruct::PluginTypeNone:
      case MusEPlugin::PluginScanInfoStruct::PluginTypeAll:
      break;
    }
  }
  if(MusEGlobal::debugMsg)
    fprintf(stderr, "%zd soft synth found\n", MusEGlobal::synthis.size());
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
                        goto synth_read_end;
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

                              if (port != -1 && port < MusECore::MIDI_PORTS)
                                    MusEGlobal::midiPorts[port].setMidiDevice(this);

                              // DELETETHIS 5
                              // Now that the track has been added to the lists in insertTrack2(),
                              //  if it's a dssi synth, OSC can find the synth, and initialize (and show) its native gui.
                              // No, initializing OSC without actually showing the gui doesn't work, at least for
                              //  dssi-vst plugins - without showing the gui they exit after ten seconds.
                              //initGui();
                              setNativeGeometry(nr.x(), nr.y(), nr.width(), nr.height());
                              showNativeGui(startngui);

                              mapRackPluginsToControllers();

                              setGeometry(r.x(), r.y(), r.width(), r.height());
                              showGui(startgui);

                              // Now that the track has been added to the lists in insertTrack2(), if it's a dssi synth
                              //  OSC can find the track and its plugins, and start their native guis if required...
                              showPendingPluginNativeGuis();

                              return;
                              }
                  default:
                        break;
                  }
            }

synth_read_end:
      AudioTrack::mapRackPluginsToControllers();
      }

//---------------------------------------------------------
//   getPatchName
//---------------------------------------------------------

QString MessSynthIF::getPatchName(int channel, int prog, bool drum) const
      {
        if (_mess)
          return QString(_mess->getPatchName(channel, prog, drum));
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

//================================================
// BEGIN Latency correction/compensation routines.
//================================================

//---------------------------------------------------------
//   getWorstPluginLatencyAudio
//---------------------------------------------------------

float SynthI::getWorstPluginLatencyAudio()
{ 
  // Have we been here before during this scan?
  // Just return the cached value.
  if(_latencyInfo._worstPluginLatencyProcessed)
    return _latencyInfo._worstPluginLatency;

  // Include the synth's own latency.
  float worst_lat = _sif->latency();
  // Include the effects rack latency.
  if(_efxPipe)
    worst_lat += _efxPipe->latency();
  
  _latencyInfo._worstPluginLatency = worst_lat;
  _latencyInfo._worstPluginLatencyProcessed = true;
  return _latencyInfo._worstPluginLatency;
}

//---------------------------------------------------------
//   setCorrectionLatencyInfo
//---------------------------------------------------------

TrackLatencyInfo& SynthI::setCorrectionLatencyInfo(bool input, float finalWorstLatency, float callerBranchLatency)
{
  const bool passthru = canPassThruLatency();

  float worst_self_latency = 0.0f;
  if(!input && !off())
  {
    //worst_self_latency = getWorstSelfLatency();
    
    worst_self_latency = getWorstSelfLatencyAudio();
    const float worst_midi = getWorstSelfLatencyMidi(false /*playback*/);
    if(worst_midi > worst_self_latency)
      worst_self_latency = worst_midi;
  }
      
  // The _trackLatency should already be calculated in the dominance scan.
  const float branch_lat = callerBranchLatency + worst_self_latency;

  // We want the AudioTrack in routes, not the MidiDevice in routes.
  RouteList* rl = AudioTrack::inRoutes();
  for (iRoute ir = rl->begin(); ir != rl->end(); ++ir)
  {
    if(ir->type != Route::TRACK_ROUTE || !ir->track || ir->track->isMidiTrack())
      continue;
    Track* track = ir->track;
    if(!off() && !track->off() && (passthru || input))
      track->setCorrectionLatencyInfo(false, finalWorstLatency, branch_lat);
  }

  const int port = midiPort();
  if(port >= 0 && port < MusECore::MIDI_PORTS)
  {
#ifdef _USE_MIDI_TRACK_SINGLE_OUT_PORT_CHAN_
    const MidiTrackList& tl = *MusEGlobal::song->midis();
    const MidiTrackList::size_type tl_sz = tl.size();
    for(MidiTrackList::size_type it = 0; it < tl_sz; ++it)
    {
      MidiTrack* track = static_cast<MidiTrack*>(tl[it]);
      if(track->outPort() != port)
        continue;
      if(!off() && !track->off() && (openFlags() & 1 /*write*/) && (passthru || input))
        track->setCorrectionLatencyInfo(false, finalWorstLatency, branch_lat);
    }

#else

    MidiPort* mp = &MusEGlobal::midiPorts[port];
    RouteList* mrl = mp->inRoutes();
    for (iRoute ir = mrl->begin(); ir != mrl->end(); ++ir)
    {
      switch(ir->type)
      {
          case Route::TRACK_ROUTE:
            if(!ir->track)
              continue;
            
            if(ir->track->isMidiTrack())
            {
              if(ir->channel < -1 || ir->channel >= MusECore::MUSE_MIDI_CHANNELS)
                continue;
              Track* track = ir->track;
              if(!off() && !track->off() && (openFlags() & 1 /*write*/) && (passthru || input))
                track->setCorrectionLatencyInfo(false, finalWorstLatency, branch_lat);
            }
          break;

          default:
          break;
      }            
    }

#endif

  }

  // Special for the built-in metronome.
  if(!off() && !MusECore::metronome->off() && (passthru || input) && sendMetronome())
  {
    MusECore::metronome->setCorrectionLatencyInfo(false, finalWorstLatency, branch_lat);
  }

  // Set the correction of all connected input branches,
  //  but ONLY if the track is not off.
  if(!off())
  {
    if(input)
    {
    }
    else
    {
      if(canCorrectOutputLatency() && _latencyInfo._canCorrectOutputLatency)
      {
        float corr = 0.0f;
        if(MusEGlobal::config.commonProjectLatency)
          corr -= finalWorstLatency;

        corr -= branch_lat;
        // The _sourceCorrectionValue is initialized to zero.
        // Whichever calling branch needs the most correction gets it.
        if(corr < _latencyInfo._sourceCorrectionValue)
          _latencyInfo._sourceCorrectionValue = corr;
      }
    }

    //fprintf(stderr, "AudioTrack::setCorrectionLatencyInfo() name:%s finalWorstLatency:%f branch_lat:%f corr:%f _sourceCorrectionValue:%f\n",
    //        name().toLatin1().constData(), finalWorstLatency, branch_lat, corr, _latencyInfo._sourceCorrectionValue);
  }

  return _latencyInfo;
}

bool SynthI::isLatencyInputTerminal()
{
  // Have we been here before during this scan?
  // Just return the cached value.
  if(_latencyInfo._isLatencyInputTerminalProcessed)
    return _latencyInfo._isLatencyInputTerminal;

  // Ultimately if the track is off there is no audio or midi processing, so it's a terminal.
  if(off())
  {
    _latencyInfo._isLatencyInputTerminal = true;
    _latencyInfo._isLatencyInputTerminalProcessed = true;
    return true;
  }
  
  if(!canRecordMonitor() || (MusEGlobal::config.monitoringAffectsLatency && isRecMonitored()))
  {
    const RouteList* rl = AudioTrack::outRoutes();
    for (ciRoute ir = rl->begin(); ir != rl->end(); ++ir) {
      switch(ir->type)
      {
        case Route::TRACK_ROUTE:
          if(!ir->track)
            continue;
          if(ir->track->isMidiTrack())
          {
            // TODO
          }
          else
          {
            Track* track = ir->track;
            if(track->off()) // || 
              //(track->canRecordMonitor() && (MusEGlobal::config.monitoringAffectsLatency || !track->isRecMonitored())))
               //&& track->canRecord() && !track->recordFlag()))
              continue;
            
            _latencyInfo._isLatencyInputTerminal = false;
            _latencyInfo._isLatencyInputTerminalProcessed = true;
            return false;
          }
        break;

        default:
        break;
      }
    }
  }

  const int port = midiPort();
  if((openFlags() & 1 /*write*/) && port >= 0 && port < MusECore::MIDI_PORTS)
  {
    MidiPort* mp = &MusEGlobal::midiPorts[port];
    const RouteList* mrl = mp->outRoutes();
    for (ciRoute ir = mrl->begin(); ir != mrl->end(); ++ir) {
      switch(ir->type)
      {
        case Route::TRACK_ROUTE:
          if(!ir->track)
            continue;
          if(ir->track->isMidiTrack())
          {
            Track* track = ir->track;
            if(track->off()) // || 
              //(track->canRecordMonitor() && (MusEGlobal::config.monitoringAffectsLatency || !track->isRecMonitored())))
              //&& atrack->canRecord() && !atrack->recordFlag()))
              continue;
            
            _latencyInfo._isLatencyInputTerminal = false;
            _latencyInfo._isLatencyInputTerminalProcessed = true;
            return false;
          }
          //else
          //{
          //  // TODO ?
          //}
        break;

        default:
        break;
      }
    }
  }

  _latencyInfo._isLatencyInputTerminal = true;
  _latencyInfo._isLatencyInputTerminalProcessed = true;
  return true;
}

bool SynthI::isLatencyOutputTerminal()
{
  // Have we been here before during this scan?
  // Just return the cached value.
  if(_latencyInfo._isLatencyOutputTerminalProcessed)
    return _latencyInfo._isLatencyOutputTerminal;

  const RouteList* rl = AudioTrack::outRoutes();
  for (ciRoute ir = rl->begin(); ir != rl->end(); ++ir) {
    switch(ir->type)
    {
      case Route::TRACK_ROUTE:
        if(!ir->track)
          continue;
        if(ir->track->isMidiTrack())
        {
          // TODO ?
        }
        else
        {
          Track* track = ir->track;
          if(track->off()) // || 
            //(track->canRecordMonitor() && (MusEGlobal::config.monitoringAffectsLatency || !track->isRecMonitored())))
              //&& track->canRecord() && !track->recordFlag()))
            continue;
          
          _latencyInfo._isLatencyOutputTerminal = false;
          _latencyInfo._isLatencyOutputTerminalProcessed = true;
          return false;
        }
      break;

      default:
      break;
    }
  }

  const int port = midiPort();
  if((openFlags() & 1 /*write*/) && port >= 0 && port < MusECore::MIDI_PORTS)
  {
    MidiPort* mp = &MusEGlobal::midiPorts[port];
    const RouteList* mrl = mp->outRoutes();
    for (ciRoute ir = mrl->begin(); ir != mrl->end(); ++ir) {
      switch(ir->type)
      {
        case Route::TRACK_ROUTE:
          if(!ir->track)
            continue;
          if(ir->track->isMidiTrack())
          {
            Track* track = ir->track;
            if(track->off()) // || 
              //(track->canRecordMonitor() && (MusEGlobal::config.monitoringAffectsLatency || !track->isRecMonitored())))
              //&& atrack->canRecord() && !atrack->recordFlag()))
              continue;
            
            _latencyInfo._isLatencyOutputTerminal = false;
            _latencyInfo._isLatencyOutputTerminalProcessed = true;
            return false;
          }
          //else
          //{
          //  // TODO ?
          //}
        break;

        default:
        break;
      }
    }
  }
    
  _latencyInfo._isLatencyOutputTerminal = true;
  _latencyInfo._isLatencyOutputTerminalProcessed = true;
  return true;
}

//---------------------------------------------------------
//   getDominanceInfo
//---------------------------------------------------------

TrackLatencyInfo& SynthI::getDominanceInfo(bool input)
{
  // Have we been here before during this scan?
  // Just return the cached value.
  if((input && _latencyInfo._canDominateInputProcessed) ||
     (!input && _latencyInfo._canDominateProcessed))
    return _latencyInfo;

  // Get the default domination for this track type.
  bool can_dominate_lat = input ? canDominateInputLatency() : canDominateOutputLatency();
  bool can_correct_lat = canCorrectOutputLatency();

  const bool passthru = canPassThruLatency();

  bool item_found = false;

  // We want the AudioTrack in routes, not the MidiDevice in routes.
  RouteList* rl = AudioTrack::inRoutes();
  for (iRoute ir = rl->begin(); ir != rl->end(); ++ir)
  {
    switch(ir->type)
    {
      case Route::TRACK_ROUTE:
        if(!ir->track)
          continue;
        if(ir->track->isMidiTrack())
        {
          // TODO ?
        }
        else
        {
          Track* track = ir->track;

          if(!off() && !track->off() && (passthru || input))
          {
            const TrackLatencyInfo& li = track->getDominanceInfo(false);

            // Whether the branch can dominate or correct latency or if we
            //  want to allow unterminated input branches to
            //  participate in worst branch latency calculations.
            const bool participate = 
              (li._canCorrectOutputLatency ||
              li._canDominateOutputLatency ||
              MusEGlobal::config.correctUnterminatedInBranchLatency);

            if(participate)
            {
              // Is it the first found item?
              if(item_found)
              {
                // If any one of the branches can dominate the latency,
                //  that overrides any which cannot.
                if(li._canDominateOutputLatency)
                  can_dominate_lat = true;
                if(li._canCorrectOutputLatency)
                  can_correct_lat = true;
              }
              else
              {
                item_found = true;
                // Override the defaults with this first item's values.
                can_dominate_lat = li._canDominateOutputLatency;
                can_correct_lat = li._canCorrectOutputLatency;
              }
            }
          }
        }
      break;

      default:
      break;
    }
  }

  const int port = midiPort();
  if(port >= 0 && port < MusECore::MIDI_PORTS)
  {
#ifdef _USE_MIDI_TRACK_SINGLE_OUT_PORT_CHAN_
    const MidiTrackList& tl = *MusEGlobal::song->midis();
    const MidiTrackList::size_type tl_sz = tl.size();
    for(MidiTrackList::size_type it = 0; it < tl_sz; ++it)
    {
      MidiTrack* track = static_cast<MidiTrack*>(tl[it]);
      if(track->outPort() != port)
        continue;

      if(!off() && !track->off() && (openFlags() & 1 /*write*/) && (passthru || input))
      {
        const TrackLatencyInfo& li = track->getDominanceInfo(false);

        // Whether the branch can dominate or correct latency or if we
        //  want to allow unterminated input branches to
        //  participate in worst branch latency calculations.
        const bool participate = 
          (li._canCorrectOutputLatency ||
          li._canDominateOutputLatency ||
          MusEGlobal::config.correctUnterminatedInBranchLatency);

        if(participate)
        {
          // Is it the first found item?
          if(item_found)
          {
            // If any one of the branches can dominate the latency,
            //  that overrides any which cannot.
            if(li._canDominateOutputLatency)
              can_dominate_lat = true;
            if(li._canCorrectOutputLatency)
              can_correct_lat = true;
          }
          else
          {
            item_found = true;
            // Override the defaults with this first item's values.
            can_dominate_lat = li._canDominateOutputLatency;
            can_correct_lat = li._canCorrectOutputLatency;
          }
        }
      }
    }

#else
    MidiPort* mp = &MusEGlobal::midiPorts[port];
    RouteList* mrl = mp->inRoutes();
    for (iRoute ir = mrl->begin(); ir != mrl->end(); ++ir)
    {
      switch(ir->type)
      {
          case Route::TRACK_ROUTE:
            if(!ir->track)
              continue;
            
            if(ir->track->isMidiTrack())
            {
              if(ir->channel < -1 || ir->channel >= MusECore::MUSE_MIDI_CHANNELS)
                continue;

              Track* track = ir->track;
//                     if(ir->channel < 0)
//                       all_chans = true;
//                     else
//                       used_chans[ir->channel] = true;
                
              if(!off() && !track->off() && (openFlags() & 1 /*write*/) && (passthru || input))
              {
                const TrackLatencyInfo& li = track->getDominanceInfo(false);

                // Whether the branch can dominate or correct latency or if we
                //  want to allow unterminated input branches to
                //  participate in worst branch latency calculations.
                const bool participate = 
                  (li._canCorrectOutputLatency ||
                  li._canDominateOutputLatency ||
                  MusEGlobal::config.correctUnterminatedInBranchLatency);

                if(participate)
                {
                  // Is it the first found item?
                  if(item_found)
                  {
                    // If any one of the branches can dominate the latency,
                    //  that overrides any which cannot.
                    if(li._canDominateOutputLatency)
                      can_dominate_out_lat = true;
                    if(li._canCorrectOutputLatency)
                      can_correct_lat = true;
                  }
                  else
                  {
                    item_found = true;
                    // Override the defaults with this first item's values.
                    can_dominate_out_lat = li._canDominateOutputLatency;
                    can_correct_lat = li._canCorrectOutputLatency;
                  }
                }
              }
            }
          break;

          default:
          break;
      }            
    }

#endif

  }

  // Special for the built-in metronome.
  if(!off() && !MusECore::metronome->off() && (passthru || input) && sendMetronome())
  {
    const TrackLatencyInfo& li = MusECore::metronome->getDominanceInfo(false);
        
    // Whether the branch can dominate or correct latency or if we
    //  want to allow unterminated input branches to
    //  participate in worst branch latency calculations.
    const bool participate = 
      (li._canCorrectOutputLatency ||
      li._canDominateOutputLatency ||
      MusEGlobal::config.correctUnterminatedInBranchLatency);

    if(participate)
    {
      // Is it the first found item?
      if(item_found)
      {
        // If any one of the branches can dominate the latency,
        //  that overrides any which cannot.
        if(li._canDominateOutputLatency)
          can_dominate_lat = true;
        if(li._canCorrectOutputLatency)
          can_correct_lat = true;
      }
      else
      {
        item_found = true;
        can_dominate_lat = li._canDominateOutputLatency;
        can_correct_lat = li._canCorrectOutputLatency;
      }
    }
  }

  // Set the correction of all connected input branches,
  //  but ONLY if the track is not off.
  if(!off())
  {
    if(input)
    {
      _latencyInfo._canDominateInputLatency = can_dominate_lat;
    }
    else
    {
      _latencyInfo._canDominateOutputLatency = can_dominate_lat;
      _latencyInfo._canCorrectOutputLatency = canCorrectOutputLatency();
      // If any of the branches can dominate, then this node cannot correct.
      _latencyInfo._canCorrectOutputLatency = can_correct_lat && !can_dominate_lat;
    }
  }

  if(input)
    _latencyInfo._canDominateInputProcessed = true;
  else
    _latencyInfo._canDominateProcessed = true;

  return _latencyInfo;
}

//---------------------------------------------------------
//   getDominanceLatencyInfo
//---------------------------------------------------------

TrackLatencyInfo& SynthI::getDominanceLatencyInfo(bool input)
{
  // Have we been here before during this scan?
  // Just return the cached value.
  if((input && _latencyInfo._dominanceInputProcessed) ||
     (!input && _latencyInfo._dominanceProcessed))
    return _latencyInfo;

  float route_worst_latency = 0.0f;

  const bool passthru = canPassThruLatency();

  bool item_found = false;

  float worst_self_latency = 0.0f;
  if(!input && !off())
  {
    worst_self_latency = getWorstSelfLatencyAudio();
    const float worst_midi = getWorstSelfLatencyMidi(false /*playback*/);
    if(worst_midi > worst_self_latency)
      worst_self_latency = worst_midi;
  }
      
  // We want the AudioTrack in routes, not the MidiDevice in routes.
  RouteList* rl = AudioTrack::inRoutes();
  for (iRoute ir = rl->begin(); ir != rl->end(); ++ir)
  {
    switch(ir->type)
    {
      case Route::TRACK_ROUTE:
        if(!ir->track)
          continue;
        if(ir->track->isMidiTrack())
        {
          // TODO ?
        }
        else
        {
          Track* track = ir->track;

          if(!off() && !track->off() && (passthru || input))
          {
            const TrackLatencyInfo& li = track->getDominanceLatencyInfo(false);

            // Whether the branch can dominate or correct latency or if we
            //  want to allow unterminated input branches to
            //  participate in worst branch latency calculations.
            const bool participate = 
              (li._canCorrectOutputLatency ||
              li._canDominateOutputLatency ||
              MusEGlobal::config.correctUnterminatedInBranchLatency);

            if(participate)
            {
              // Is it the first found item?
              if(item_found)
              {
                // If any one of the branches can dominate the latency,
                //  that overrides any which cannot.
                if(li._canDominateOutputLatency)
                {
                  // Override the current worst value if the latency is greater,
                  //  but ONLY if the branch can dominate.
                  //if(li._outputLatency > route_worst_latency)
                  //  route_worst_latency = li._outputLatency;
                }
                // Override the current worst value if the latency is greater,
                //  but ONLY if the branch can dominate.
                if(li._outputLatency > route_worst_latency)
                  route_worst_latency = li._outputLatency;
              }
              else
              {
                item_found = true;
                // Override the default worst value, but ONLY if the branch can dominate.
                //if(li._canDominateOutputLatency)
                  route_worst_latency = li._outputLatency;
              }
            }
          }
        }
      break;

      default:
      break;
    }
  }

  const int port = midiPort();
  if(port >= 0 && port < MusECore::MIDI_PORTS)
  {
#ifdef _USE_MIDI_TRACK_SINGLE_OUT_PORT_CHAN_
    const MidiTrackList& tl = *MusEGlobal::song->midis();
    const MidiTrackList::size_type tl_sz = tl.size();
    for(MidiTrackList::size_type it = 0; it < tl_sz; ++it)
    {
      MidiTrack* track = static_cast<MidiTrack*>(tl[it]);
      if(track->outPort() != port)
        continue;

      if(!off() && !track->off() && (openFlags() & 1 /*write*/) && (passthru || input))
      {
        const TrackLatencyInfo& li = track->getDominanceLatencyInfo(false);

        // Whether the branch can dominate or correct latency or if we
        //  want to allow unterminated input branches to
        //  participate in worst branch latency calculations.
        const bool participate = 
          (li._canCorrectOutputLatency ||
          li._canDominateOutputLatency ||
          MusEGlobal::config.correctUnterminatedInBranchLatency);

        if(participate)
        {
          // Is it the first found item?
          if(item_found)
          {
            // If any one of the branches can dominate the latency,
            //  that overrides any which cannot.
            if(li._canDominateOutputLatency)
            {
              // Override the current worst value if the latency is greater,
              //  but ONLY if the branch can dominate.
              //if(li._outputLatency > route_worst_latency)
              //  route_worst_latency = li._outputLatency;
            }
            // Override the current worst value if the latency is greater,
            //  but ONLY if the branch can dominate.
            if(li._outputLatency > route_worst_latency)
              route_worst_latency = li._outputLatency;
          }
          else
          {
            item_found = true;
            // Override the default worst value, but ONLY if the branch can dominate.
            //if(li._canDominateOutputLatency)
              route_worst_latency = li._outputLatency;
          }
        }
      }
    }

#else
    MidiPort* mp = &MusEGlobal::midiPorts[port];
    RouteList* mrl = mp->inRoutes();
    for (iRoute ir = mrl->begin(); ir != mrl->end(); ++ir)
    {
      switch(ir->type)
      {
          case Route::TRACK_ROUTE:
            if(!ir->track)
              continue;
            
            if(ir->track->isMidiTrack())
            {
              if(ir->channel < -1 || ir->channel >= MusECore::MUSE_MIDI_CHANNELS)
                continue;

              Track* track = ir->track;
//                     if(ir->channel < 0)
//                       all_chans = true;
//                     else
//                       used_chans[ir->channel] = true;
                
              if(!off() && !track->off() && (openFlags() & 1 /*write*/) && (passthru || input))
              {
                const TrackLatencyInfo& li = track->getDominanceLatencyInfo(false);

                // Whether the branch can dominate or correct latency or if we
                //  want to allow unterminated input branches to
                //  participate in worst branch latency calculations.
                const bool participate = 
                  (li._canCorrectOutputLatency ||
                  li._canDominateOutputLatency ||
                  MusEGlobal::config.correctUnterminatedInBranchLatency);

                if(participate)
                {
                  // Is it the first found item?
                  if(item_found)
                  {
                    // If any one of the branches can dominate the latency,
                    //  that overrides any which cannot.
                    if(li._canDominateOutputLatency)
                    {
                      // Override the current worst value if the latency is greater,
                      //  but ONLY if the branch can dominate.
                      //if(li._outputLatency > route_worst_latency)
                      //  route_worst_latency = li._outputLatency;
                    }
                    // Override the current worst value if the latency is greater,
                    //  but ONLY if the branch can dominate.
                    if(li._outputLatency > route_worst_latency)
                      route_worst_latency = li._outputLatency;
                  }
                  else
                  {
                    item_found = true;
                    // Override the default worst value, but ONLY if the branch can dominate.
                    //if(li._canDominateOutputLatency)
                      route_worst_latency = li._outputLatency;
                  }
                }
              }
            }
          break;

          default:
          break;
      }            
    }

#endif

  }

  // Special for the built-in metronome.
  if(!off() && !MusECore::metronome->off() && (passthru || input) && sendMetronome())
  {
    const TrackLatencyInfo& li = MusECore::metronome->getDominanceLatencyInfo(false);
        
    // Whether the branch can dominate or correct latency or if we
    //  want to allow unterminated input branches to
    //  participate in worst branch latency calculations.
    const bool participate = 
      (li._canCorrectOutputLatency ||
      li._canDominateOutputLatency ||
      MusEGlobal::config.correctUnterminatedInBranchLatency);

    if(participate)
    {
      // Is it the first found item?
      if(item_found)
      {
        // If any one of the branches can dominate the latency,
        //  that overrides any which cannot.
        if(li._canDominateOutputLatency)
        {
          // Override the current worst value if the latency is greater,
          //  but ONLY if the branch can dominate.
          //if(li._outputLatency > route_worst_latency)
          //  route_worst_latency = li._outputLatency;
        }
        // Override the current worst value if the latency is greater,
        //  but ONLY if the branch can dominate.
        if(li._outputLatency > route_worst_latency)
          route_worst_latency = li._outputLatency;
      }
      else
      {
        item_found = true;
        // Override the default worst value, but ONLY if the branch can dominate.
        //if(li._canDominateOutputLatency)
          route_worst_latency = li._outputLatency;
      }
    }
  }

  // Set the correction of all connected input branches,
  //  but ONLY if the track is not off.
  if(!off())
  {
    if(input)
    {
      _latencyInfo._inputLatency = route_worst_latency;
    }
    else
    {
      if(passthru)
      {
        _latencyInfo._outputLatency = worst_self_latency + route_worst_latency;
        _latencyInfo._inputLatency = route_worst_latency;
      }
      else
      {
        _latencyInfo._outputLatency = worst_self_latency + _latencyInfo._sourceCorrectionValue;
      }
    }
  }

  if(input)
    _latencyInfo._dominanceInputProcessed = true;
  else
    _latencyInfo._dominanceProcessed = true;

  return _latencyInfo;
}

//---------------------------------------------------------
//   getLatencyInfo
//---------------------------------------------------------

TrackLatencyInfo& SynthI::getLatencyInfo(bool input)
{
  // Have we been here before during this scan?
  // Just return the cached value.
  if((input && _latencyInfo._inputProcessed) ||
    (!input && _latencyInfo._processed))
    return _latencyInfo;

  float route_worst_latency = _latencyInfo._inputLatency;

  const bool passthru = canPassThruLatency();

  // We want the AudioTrack in routes, not the MidiDevice in routes.
  RouteList* rl = AudioTrack::inRoutes();

  // Now that we know the worst-case latency of the connected branches,
  //  adjust each of the conveniently stored temporary latency values
  //  in the routes according to whether they can dominate...
  for (iRoute ir = rl->begin(); ir != rl->end(); ++ir)
  {
    if(ir->type != Route::TRACK_ROUTE || !ir->track || ir->track->isMidiTrack())
      continue;

    Track* track = ir->track;

    if(passthru || input)
    {
      // Default to zero.
      ir->audioLatencyOut = 0.0f;

      if(!off() && !track->off())
      {
        const TrackLatencyInfo& li = track->getLatencyInfo(false);
        const bool participate =
          (li._canCorrectOutputLatency ||
          li._canDominateOutputLatency ||
          MusEGlobal::config.correctUnterminatedInBranchLatency);

        if(participate)
        {
          // Prepare the latency value to be passed to the compensator's writer,
          //  by adjusting each route latency value. ie. the route with the worst-case
          //  latency will get ZERO delay, while routes having smaller latency will get
          //  MORE delay, to match all the signal timings together.
          // The route's audioLatencyOut should have already been calculated and
          //  conveniently stored in the route.
          ir->audioLatencyOut = route_worst_latency - li._outputLatency;
          // Should not happen, but just in case.
          if((long int)ir->audioLatencyOut < 0)
            ir->audioLatencyOut = 0.0f;
        }
      }
    }
  }

  const int port = midiPort();
  if(port >= 0 && port < MusECore::MIDI_PORTS)
  {
#ifdef _USE_MIDI_TRACK_SINGLE_OUT_PORT_CHAN_
    const MidiTrackList& tl = *MusEGlobal::song->midis();
    const MidiTrackList::size_type tl_sz = tl.size();
    for(MidiTrackList::size_type it = 0; it < tl_sz; ++it)
    {
      MidiTrack* track = static_cast<MidiTrack*>(tl[it]);
      if(track->outPort() != port)
        continue;

      if(passthru || input)
      {
        // Default to zero.
        // TODO: FIXME: Where to store? We have no route to store it in.
        //ir->audioLatencyOut = 0.0f;
        //li._latencyOutMidiTrack = 0.0f;

        if(!off() && !track->off() && (openFlags() & 1 /*write*/))
        {
          TrackLatencyInfo& li = track->getLatencyInfo(false);
          // Whether the branch can dominate or correct latency or if we
          //  want to allow unterminated input branches to
          //  participate in worst branch latency calculations.
          const bool participate =
            li._canCorrectOutputLatency ||
            li._canDominateOutputLatency ||
            MusEGlobal::config.correctUnterminatedInBranchLatency;

          if(participate)
          {
            // TODO: FIXME: Where to store? We have no route to store it in.
            // Prepare the latency value to be passed to the compensator's writer,
            //  by adjusting each route latency value. ie. the route with the worst-case
            //  latency will get ZERO delay, while routes having smaller latency will get
            //  MORE delay, to match all the signal timings together.
            // The route's audioLatencyOut should have already been calculated and
            //  conveniently stored in the route.
//               ir->audioLatencyOut = route_worst_latency - ir->audioLatencyOut;
//               // Should not happen, but just in case.
//               if((long int)ir->audioLatencyOut < 0)
//                 ir->audioLatencyOut = 0.0f;

            // Special for Midi Tracks: We don't have Midi Track to Midi Port routes yet
            //  because we don't have multiple Midi Track outputs yet, only a single output port.
            // So we must store this information here just for Midi Tracks.
            li._latencyOutMidiTrack = route_worst_latency - li._outputLatency;
            // Should not happen, but just in case.
            if((long int)li._latencyOutMidiTrack < 0)
              li._latencyOutMidiTrack = 0.0f;
          }
        }
      }
    }

#else

    MidiPort* mp = &MusEGlobal::midiPorts[port];
    RouteList* mrl = mp->inRoutes();
    for (iRoute ir = mrl->begin(); ir != mrl->end(); ++ir)
    {
      switch(ir->type)
      {
          case Route::TRACK_ROUTE:
            if(!ir->track)
              continue;

            if(ir->track->isMidiTrack())
            {
              if(ir->channel < -1 || ir->channel >= MusECore::MUSE_MIDI_CHANNELS)
                continue;

              Track* track = ir->track;

//                     if(ir->channel < 0)
//                       all_chans = true;
//                     else
//                       used_chans[ir->channel] = true;

              if(passthru || input)
              {
                // Default to zero.
                ir->audioLatencyOut = 0.0f;

                if(!off() && !track->off() && (openFlags() & 1 /*write*/)
                {
                  TrackLatencyInfo& li = track->getLatencyInfo(false);
                  // Whether the branch can dominate or correct latency or if we
                  //  want to allow unterminated input branches to
                  //  participate in worst branch latency calculations.
                  const bool participate =
                    li._canCorrectOutputLatency ||
                    li._canDominateOutputLatency ||
                    MusEGlobal::config.correctUnterminatedInBranchLatency;

                  if(participate)
                  {
                    // Prepare the latency value to be passed to the compensator's writer,
                    //  by adjusting each route latency value. ie. the route with the worst-case
                    //  latency will get ZERO delay, while routes having smaller latency will get
                    //  MORE delay, to match all the signal timings together.
                    // The route's audioLatencyOut should have already been calculated and
                    //  conveniently stored in the route.
                    ir->audioLatencyOut = route_worst_latency - li._outputLatency;
                    // Should not happen, but just in case.
                    if((long int)ir->audioLatencyOut < 0)
                      ir->audioLatencyOut = 0.0f;
                  }
                }
              }
            }
          break;

          default:
          break;
      }            
    }

#endif

  }

  // Special for the built-in metronome.
  if(passthru || input)
  {
    // Default to zero.
    _latencyInfo._latencyOutMetronome = 0.0f;

    if(!off() && !MusECore::metronome->off() && sendMetronome())
    {
      TrackLatencyInfo& li = MusECore::metronome->getLatencyInfo(false);

      const bool participate =
        li._canCorrectOutputLatency ||
        li._canDominateOutputLatency ||
        MusEGlobal::config.correctUnterminatedInBranchLatency;

      if(participate)
      {
        // TODO: FIXME: Where to store? We have no route to store it in.
        // Prepare the latency value to be passed to the compensator's writer,
        //  by adjusting each route latency value. ie. the route with the worst-case
        //  latency will get ZERO delay, while routes having smaller latency will get
        //  MORE delay, to match all the signal timings together.
        // The route's audioLatencyOut should have already been calculated and
        //  conveniently stored in the route.

//             ir->audioLatencyOut = route_worst_latency - ir->audioLatencyOut;
//             // Should not happen, but just in case.
//             if((long int)ir->audioLatencyOut < 0)
//               ir->audioLatencyOut = 0.0f;

        // Special for Midi Tracks: We don't have Midi Track to Midi Port routes yet
        //  because we don't have multiple Midi Track outputs yet, only a single output port.
        // So we must store this information here just for Midi Tracks.
        li._latencyOutMetronome = route_worst_latency - li._latencyOutMetronome;
        // Should not happen, but just in case.
        if((long int)li._latencyOutMetronome < 0)
          li._latencyOutMetronome = 0.0f;
      }
    }
  }

  if(input)
    _latencyInfo._inputProcessed = true;
  else
    _latencyInfo._processed = true;

  return _latencyInfo;
}

bool SynthI::isLatencyInputTerminalMidi(bool capture)
{
  TrackLatencyInfo* tli = capture ? &_captureLatencyInfo : &_playbackLatencyInfo;

  // Have we been here before during this scan?
  // Just return the cached value.
  if(tli->_isLatencyInputTerminalProcessed)
    return tli->_isLatencyInputTerminal;

  // Ultimately if the track is off there is no audio or midi processing, so it's a terminal.
  if(off())
  {
    tli->_isLatencyInputTerminal = true;
    tli->_isLatencyInputTerminalProcessed = true;
    return true;
  }
  
  if(!canRecordMonitor() || (MusEGlobal::config.monitoringAffectsLatency && isRecMonitored()))
  {
    const RouteList* rl = AudioTrack::outRoutes();
    for (ciRoute ir = rl->begin(); ir != rl->end(); ++ir) {
      switch(ir->type)
      {
        case Route::TRACK_ROUTE:
          if(!ir->track)
            continue;
          if(ir->track->isMidiTrack())
          {
            // TODO
          }
          else
          {
            Track* track = ir->track;
            if(track->off()) // || 
              //(track->canRecordMonitor() && (MusEGlobal::config.monitoringAffectsLatency || !track->isRecMonitored())))
               //&& track->canRecord() && !track->recordFlag()))
              continue;
            
            tli->_isLatencyInputTerminal = false;
            tli->_isLatencyInputTerminalProcessed = true;
            return false;
          }
        break;

        default:
        break;
      }
    }
  }
  
  const int port = midiPort();
  if((openFlags() & (capture ? 2 : 1)) && port >= 0 && port < MusECore::MIDI_PORTS)
  {
    MidiPort* mp = &MusEGlobal::midiPorts[port];
    const RouteList* mrl = mp->outRoutes();
    for (ciRoute ir = mrl->begin(); ir != mrl->end(); ++ir) {
      switch(ir->type)
      {
        case Route::TRACK_ROUTE:
          if(!ir->track)
            continue;
          if(ir->track->isMidiTrack())
          {
            Track* track = ir->track;
            if(track->off()) // || 
              //(atrack->canRecordMonitor() && (MusEGlobal::config.monitoringAffectsLatency || !atrack->isRecMonitored())))
               //&& atrack->canRecord() && !atrack->recordFlag()))
              continue;
            
            tli->_isLatencyInputTerminal = false;
            tli->_isLatencyInputTerminalProcessed = true;
            return false;
          }
        break;

        default:
        break;
      }
    }
  }

  tli->_isLatencyInputTerminal = true;
  tli->_isLatencyInputTerminalProcessed = true;
  return true;
}

bool SynthI::isLatencyOutputTerminalMidi(bool capture)
{
  TrackLatencyInfo* tli = capture ? &_captureLatencyInfo : &_playbackLatencyInfo;

  // Have we been here before during this scan?
  // Just return the cached value.
  if(tli->_isLatencyOutputTerminalProcessed)
    return tli->_isLatencyOutputTerminal;

  if(!canRecordMonitor() || (MusEGlobal::config.monitoringAffectsLatency && isRecMonitored()))
  {
    const RouteList* rl = AudioTrack::outRoutes();
    for (ciRoute ir = rl->begin(); ir != rl->end(); ++ir) {
      switch(ir->type)
      {
        case Route::TRACK_ROUTE:
          if(!ir->track)
            continue;
          if(ir->track->isMidiTrack())
          {
            // TODO
          }
          else
          {
            Track* track = ir->track;
            if(track->off()) // || 
              //(track->canRecordMonitor() && (MusEGlobal::config.monitoringAffectsLatency || !track->isRecMonitored())))
               //&& track->canRecord() && !track->recordFlag()))
              continue;
            
            tli->_isLatencyOutputTerminal = false;
            tli->_isLatencyOutputTerminalProcessed = true;
            return false;
          }
        break;

        default:
        break;
      }
    }
  }
  
  const int port = midiPort();
  if((openFlags() & (capture ? 2 : 1)) && port >= 0 && port < MusECore::MIDI_PORTS)
  {
    MidiPort* mp = &MusEGlobal::midiPorts[port];
    const RouteList* mrl = mp->outRoutes();
    for (ciRoute ir = mrl->begin(); ir != mrl->end(); ++ir) {
      switch(ir->type)
      {
        case Route::TRACK_ROUTE:
          if(!ir->track)
            continue;
          if(ir->track->isMidiTrack())
          {
            Track* track = ir->track;
            if(track->off()) // || 
              //(atrack->canRecordMonitor() && (MusEGlobal::config.monitoringAffectsLatency || !atrack->isRecMonitored())))
              //&& atrack->canRecord() && !atrack->recordFlag()))
              continue;
            
            tli->_isLatencyOutputTerminal = false;
            tli->_isLatencyOutputTerminalProcessed = true;
            return false;
          }
        break;

        default:
        break;
      }
    }
  }

  tli->_isLatencyOutputTerminal = true;
  tli->_isLatencyOutputTerminalProcessed = true;
  return true;
}

//---------------------------------------------------------
//   getDominanceInfoMidi
//---------------------------------------------------------

TrackLatencyInfo& SynthI::getDominanceInfoMidi(bool capture, bool input)
{
      TrackLatencyInfo* tli = capture ? &_captureLatencyInfo : &_playbackLatencyInfo;

      // Have we been here before during this scan?
      // Just return the cached value.
      if((input && tli->_canDominateInputProcessed) ||
        (!input && tli->_canDominateProcessed))
        return *tli;

      // Get the default domination for this track type.
      bool can_dominate_lat = input ? canDominateInputLatencyMidi(capture) : canDominateOutputLatencyMidi(capture);
      bool can_correct_lat = canCorrectOutputLatencyMidi();

      const bool passthru = canPassThruLatencyMidi(capture);

      bool item_found = false;

      // Gather latency info from all connected input branches,
      //  but ONLY if the track is not off.
      RouteList* rl = AudioTrack::inRoutes();
      for (iRoute ir = rl->begin(); ir != rl->end(); ++ir)
      {
        switch(ir->type)
        {
          case Route::TRACK_ROUTE:
            if(!ir->track)
              continue;
            if(ir->track->isMidiTrack())
            {
              // TODO ?
            }
            else
            {
              Track* track = ir->track;

              if(!off() && !track->off() && (passthru || input))
              {
                const TrackLatencyInfo& li = track->getDominanceInfo(false);

                // Whether the branch can dominate or correct latency or if we
                //  want to allow unterminated input branches to
                //  participate in worst branch latency calculations.
                const bool participate = 
                  (li._canCorrectOutputLatency ||
                  li._canDominateOutputLatency ||
                  MusEGlobal::config.correctUnterminatedInBranchLatency);

                if(participate)
                {
                  // Is it the first found item?
                  if(item_found)
                  {
                    // If any one of the branches can dominate the latency,
                    //  that overrides any which cannot.
                    if(li._canDominateOutputLatency)
                      can_dominate_lat = true;
                    if(li._canCorrectOutputLatency)
                      can_correct_lat = true;
                  }
                  else
                  {
                    item_found = true;
                    // Override the defaults with this first item's values.
                    can_dominate_lat = li._canDominateOutputLatency;
                    can_correct_lat = li._canCorrectOutputLatency;
                  }
                }
              }
            }
          break;

          default:
          break;
        }
      }

      const int port = midiPort();
      if(port >= 0 && port < MusECore::MIDI_PORTS)
      {
//         bool used_chans[MusECore::MUSE_MIDI_CHANNELS];
//         for(int i = 0; i < MusECore::MUSE_MIDI_CHANNELS; ++i)
//           used_chans[i] = false;
//         bool all_chans = false;

        {
          
#ifdef _USE_MIDI_TRACK_SINGLE_OUT_PORT_CHAN_
          const MidiTrackList& tl = *MusEGlobal::song->midis();
          const MidiTrackList::size_type tl_sz = tl.size();
          for(MidiTrackList::size_type it = 0; it < tl_sz; ++it)
          {
            MidiTrack* track = static_cast<MidiTrack*>(tl[it]);
            if(track->outPort() != port)
              continue;

            if(!off() && (openFlags() & (capture ? 2 : 1)) && !track->off() && (passthru || input))
            {
              const TrackLatencyInfo& li = track->getDominanceInfo(false);

              // Whether the branch can dominate or correct latency or if we
              //  want to allow unterminated input branches to
              //  participate in worst branch latency calculations.
              const bool participate = 
                (li._canCorrectOutputLatency ||
                li._canDominateOutputLatency ||
                MusEGlobal::config.correctUnterminatedInBranchLatency);

              if(participate)
              {
                // Is it the first found item?
                if(item_found)
                {
                  // If any one of the branches can dominate the latency,
                  //  that overrides any which cannot.
                  if(li._canDominateOutputLatency)
                    can_dominate_lat = true;
                  if(li._canCorrectOutputLatency)
                    can_correct_lat = true;
                }
                else
                {
                  item_found = true;
                  // Override the defaults with this first item's values.
                  can_dominate_lat = li._canDominateOutputLatency;
                  can_correct_lat = li._canCorrectOutputLatency;
                }
              }
            }
          }

#else

          MidiPort* mp = &MusEGlobal::midiPorts[port];
          RouteList* mrl = mp->inRoutes();
          for (iRoute ir = mrl->begin(); ir != mrl->end(); ++ir)
          {
            switch(ir->type)
            {
                case Route::TRACK_ROUTE:
                  if(!ir->track)
                    continue;
                  
                  if(ir->track->isMidiTrack())
                  {
                    if(ir->channel < -1 || ir->channel >= MusECore::MUSE_MIDI_CHANNELS)
                      continue;

                    Track* track = ir->track;
//                     if(ir->channel < 0)
//                       all_chans = true;
//                     else
//                       used_chans[ir->channel] = true;
                      
                    if(!off() && (openFlags() & (capture ? 2 : 1)) && !track->off() && (passthru || input))
                    {
                      const TrackLatencyInfo& li = track->getDominanceInfo(false);

                      // Whether the branch can dominate or correct latency or if we
                      //  want to allow unterminated input branches to
                      //  participate in worst branch latency calculations.
                      const bool participate = 
                        (li._canCorrectOutputLatency ||
                        li._canDominateOutputLatency ||
                        MusEGlobal::config.correctUnterminatedInBranchLatency);

                      if(participate)
                      {
                        // Is it the first found item?
                        if(item_found)
                        {
                          // If any one of the branches can dominate the latency,
                          //  that overrides any which cannot.
                          if(li._canDominateOutputLatency)
                            can_dominate_out_lat = true;
                          if(li._canCorrectOutputLatency)
                            can_correct_lat = true;
                        }
                        else
                        {
                          item_found = true;
                          // Override the defaults with this first item's values.
                          can_dominate_out_lat = li._canDominateOutputLatency;
                          can_correct_lat = li._canCorrectOutputLatency;
                        }
                      }
                    }
                  }
                break;

                default:
                break;
            }            
          }

#endif          

        }
        
        // Special for the built-in metronome.
        if(!capture)
        {
          MusECore::MetronomeSettings* metro_settings = 
            MusEGlobal::metroUseSongSettings ? &MusEGlobal::metroSongSettings : &MusEGlobal::metroGlobalSettings;

          //if(sendMetronome())
          if(metro_settings->midiClickFlag && metro_settings->clickPort == port)
          {
            if(!off() && (openFlags() & (capture ? 2 : 1)) && !MusECore::metronome->off() && (passthru || input))
            {
              const TrackLatencyInfo& li = MusECore::metronome->getDominanceInfoMidi(capture, false);

              // Whether the branch can dominate or correct latency or if we
              //  want to allow unterminated input branches to
              //  participate in worst branch latency calculations.
              const bool participate = 
                (li._canCorrectOutputLatency ||
                li._canDominateOutputLatency ||
                MusEGlobal::config.correctUnterminatedInBranchLatency);

              if(participate)
              {
                // Is it the first found item?
                if(item_found)
                {
                  // If any one of the branches can dominate the latency,
                  //  that overrides any which cannot.
                  if(li._canDominateOutputLatency)
                    can_dominate_lat = true;
                  if(li._canCorrectOutputLatency)
                    can_correct_lat = true;
                }
                else
                {
                  item_found = true;
                  // Override the defaults with this first item's values.
                  //route_worst_out_corr = li._outputAvailableCorrection;
                  can_dominate_lat = li._canDominateOutputLatency;
                  can_correct_lat = li._canCorrectOutputLatency;
                }
              }
            }
          }
        }
      }
      
      // Set the correction of all connected input branches,
      //  but ONLY if the track is not off.
      if(!off() && (openFlags() & (capture ? 2 : 1)))
      {
        if(input)
        {
          tli->_canDominateInputLatency = can_dominate_lat;
        }
        else
        {
          tli->_canDominateOutputLatency = can_dominate_lat;
          // If any of the branches can dominate, then this node cannot correct.
          tli->_canCorrectOutputLatency = can_correct_lat && !can_dominate_lat;
      }
      }

      if(input)
        tli->_canDominateInputProcessed = true;
      else
        tli->_canDominateProcessed = true;

      return *tli;
}

//---------------------------------------------------------
//   getDominanceLatencyInfoMidi
//---------------------------------------------------------

TrackLatencyInfo& SynthI::getDominanceLatencyInfoMidi(bool capture, bool input)
{
      TrackLatencyInfo* tli = capture ? &_captureLatencyInfo : &_playbackLatencyInfo;

      // Have we been here before during this scan?
      // Just return the cached value.
      if((input && tli->_dominanceInputProcessed) ||
        (!input && tli->_dominanceProcessed))
        return *tli;

      float route_worst_latency = 0.0f;

      const bool passthru = canPassThruLatencyMidi(capture);

      bool item_found = false;

      float worst_self_latency = 0.0f;
      if(!input && !off() && (openFlags() & (capture ? 2 : 1)))
      {
        worst_self_latency = getWorstSelfLatencyAudio();
        const float worst_midi = getWorstSelfLatencyMidi(false /*playback*/);
        if(worst_midi > worst_self_latency)
          worst_self_latency = worst_midi;
      }
      
      // Gather latency info from all connected input branches,
      //  but ONLY if the track is not off.
      RouteList* rl = AudioTrack::inRoutes();
      for (iRoute ir = rl->begin(); ir != rl->end(); ++ir)
      {
        switch(ir->type)
        {
          case Route::TRACK_ROUTE:
            if(!ir->track)
              continue;
            if(ir->track->isMidiTrack())
            {
              // TODO ?
            }
            else
            {
              Track* track = ir->track;

              if(!off() && !track->off() && (passthru || input))
              {
                const TrackLatencyInfo& li = track->getDominanceLatencyInfo(false);

                // Whether the branch can dominate or correct latency or if we
                //  want to allow unterminated input branches to
                //  participate in worst branch latency calculations.
                const bool participate = 
                  (li._canCorrectOutputLatency ||
                  li._canDominateOutputLatency ||
                  MusEGlobal::config.correctUnterminatedInBranchLatency);

                if(participate)
                {
                  // Is it the first found item?
                  if(item_found)
                  {
                    // If any one of the branches can dominate the latency,
                    //  that overrides any which cannot.
                    if(li._canDominateOutputLatency)
                    {
                      // Override the current worst value if the latency is greater,
                      //  but ONLY if the branch can dominate.
                      //if(li._outputLatency > route_worst_latency)
                      //  route_worst_latency = li._outputLatency;
                    }
                    // Override the current worst value if the latency is greater,
                    //  but ONLY if the branch can dominate.
                    if(li._outputLatency > route_worst_latency)
                      route_worst_latency = li._outputLatency;
                  }
                  else
                  {
                    item_found = true;
                    // Override the default worst value, but ONLY if the branch can dominate.
                    //if(li._canDominateOutputLatency)
                      route_worst_latency = li._outputLatency;
                  }
                }
              }
            }
          break;

          default:
          break;
        }
      }

      const int port = midiPort();
      if(port >= 0 && port < MusECore::MIDI_PORTS)
      {
//         bool used_chans[MusECore::MUSE_MIDI_CHANNELS];
//         for(int i = 0; i < MusECore::MUSE_MIDI_CHANNELS; ++i)
//           used_chans[i] = false;
//         bool all_chans = false;

        
#ifdef _USE_MIDI_TRACK_SINGLE_OUT_PORT_CHAN_
        const MidiTrackList& tl = *MusEGlobal::song->midis();
        const MidiTrackList::size_type tl_sz = tl.size();
        for(MidiTrackList::size_type it = 0; it < tl_sz; ++it)
        {
          MidiTrack* track = static_cast<MidiTrack*>(tl[it]);
          if(track->outPort() != port)
            continue;

          if(!off() && (openFlags() & (capture ? 2 : 1)) && !track->off() && (passthru || input))
          {
            const TrackLatencyInfo& li = track->getDominanceLatencyInfo(false);

            // Whether the branch can dominate or correct latency or if we
            //  want to allow unterminated input branches to
            //  participate in worst branch latency calculations.
            const bool participate = 
              (li._canCorrectOutputLatency ||
              li._canDominateOutputLatency ||
              MusEGlobal::config.correctUnterminatedInBranchLatency);

            if(participate)
            {
              // Is it the first found item?
              if(item_found)
              {
                // If any one of the branches can dominate the latency,
                //  that overrides any which cannot.
                if(li._canDominateOutputLatency)
                {
                  // Override the current worst value if the latency is greater,
                  //  but ONLY if the branch can dominate.
                  //if(li._outputLatency > route_worst_latency)
                  //  route_worst_latency = li._outputLatency;
                }
                // Override the current worst value if the latency is greater,
                //  but ONLY if the branch can dominate.
                if(li._outputLatency > route_worst_latency)
                  route_worst_latency = li._outputLatency;
              }
              else
              {
                item_found = true;
                // Override the default worst value, but ONLY if the branch can dominate.
                //if(li._canDominateOutputLatency)
                  route_worst_latency = li._outputLatency;
              }
            }
          }
        }

#else

        MidiPort* mp = &MusEGlobal::midiPorts[port];
        RouteList* mrl = mp->inRoutes();
        for (iRoute ir = mrl->begin(); ir != mrl->end(); ++ir)
        {
          switch(ir->type)
          {
              case Route::TRACK_ROUTE:
                if(!ir->track)
                  continue;
                
                if(ir->track->isMidiTrack())
                {
                  if(ir->channel < -1 || ir->channel >= MusECore::MUSE_MIDI_CHANNELS)
                    continue;

                  Track* track = ir->track;
//                     if(ir->channel < 0)
//                       all_chans = true;
//                     else
//                       used_chans[ir->channel] = true;
                    
                  if(!off() && (openFlags() & (capture ? 2 : 1)) && !track->off() && (passthru || input))
                  {
                    const TrackLatencyInfo& li = track->getDominanceLatencyInfo(false);

                    // Whether the branch can dominate or correct latency or if we
                    //  want to allow unterminated input branches to
                    //  participate in worst branch latency calculations.
                    const bool participate = 
                      (li._canCorrectOutputLatency ||
                      li._canDominateOutputLatency ||
                      MusEGlobal::config.correctUnterminatedInBranchLatency);

                    if(participate)
                    {
                      // Is it the first found item?
                      if(item_found)
                      {
                        // If any one of the branches can dominate the latency,
                        //  that overrides any which cannot.
                        if(li._canDominateOutputLatency)
                        {
                          // Override the current worst value if the latency is greater,
                          //  but ONLY if the branch can dominate.
                          //if(li._outputLatency > route_worst_latency)
                          //  route_worst_latency = li._outputLatency;
                        }
                        // Override the current worst value if the latency is greater,
                        //  but ONLY if the branch can dominate.
                        if(li._outputLatency > route_worst_latency)
                          route_worst_latency = li._outputLatency;
                      }
                      else
                      {
                        item_found = true;
                        // Override the default worst value, but ONLY if the branch can dominate.
                        //if(li._canDominateOutputLatency)
                          route_worst_latency = li._outputLatency;
                      }
                    }
                  }
                }
              break;

              default:
              break;
          }            
        }

#endif          
        
        // Special for the built-in metronome.
        if(!capture)
        {
          MusECore::MetronomeSettings* metro_settings = 
            MusEGlobal::metroUseSongSettings ? &MusEGlobal::metroSongSettings : &MusEGlobal::metroGlobalSettings;

          //if(sendMetronome())
          if(metro_settings->midiClickFlag && metro_settings->clickPort == port)
          {
            if(!off() && (openFlags() & (capture ? 2 : 1)) && !MusECore::metronome->off() && (passthru || input))
            {
              const TrackLatencyInfo& li = MusECore::metronome->getDominanceLatencyInfoMidi(capture, false);

              // Whether the branch can dominate or correct latency or if we
              //  want to allow unterminated input branches to
              //  participate in worst branch latency calculations.
              const bool participate = 
                (li._canCorrectOutputLatency ||
                li._canDominateOutputLatency ||
                MusEGlobal::config.correctUnterminatedInBranchLatency);

              if(participate)
              {
                // Is it the first found item?
                if(item_found)
                {
                  // If any one of the branches can dominate the latency,
                  //  that overrides any which cannot.
                  if(li._canDominateOutputLatency)
                  {
                    // Override the current worst value if the latency is greater,
                    //  but ONLY if the branch can dominate.
                    //if(li._outputLatency > route_worst_latency)
                    //  route_worst_latency = li._outputLatency;
                  }
                  // Override the current worst value if the latency is greater,
                  //  but ONLY if the branch can dominate.
                  if(li._outputLatency > route_worst_latency)
                    route_worst_latency = li._outputLatency;
                }
                else
                {
                  item_found = true;
                  // Override the default worst value, but ONLY if the branch can dominate.
                  //if(li._canDominateOutputLatency)
                    route_worst_latency = li._outputLatency;
                }
              }
            }
          }
        }
      }
      
      // Set the correction of all connected input branches,
      //  but ONLY if the track is not off.
      if(!off() && (openFlags() & (capture ? 2 : 1)))
      {
        if(input)
        {
          tli->_inputLatency = route_worst_latency;
        }
        else
        {
          if(passthru)
          {
            tli->_outputLatency = worst_self_latency + route_worst_latency;
            tli->_inputLatency = route_worst_latency;
          }
          else
          {
            tli->_outputLatency = worst_self_latency + tli->_sourceCorrectionValue;
          }
        }
      }

      if(input)
        tli->_dominanceInputProcessed = true;
      else
        tli->_dominanceProcessed = true;

      return *tli;
}

//---------------------------------------------------------
//   setCorrectionLatencyInfoMidi
//---------------------------------------------------------

TrackLatencyInfo& SynthI::setCorrectionLatencyInfoMidi(bool capture, bool input, float finalWorstLatency, float callerBranchLatency)
{
  TrackLatencyInfo* tli = capture ? &_captureLatencyInfo : &_playbackLatencyInfo;

  const bool passthru = canPassThruLatencyMidi(capture);

  float worst_self_latency = 0.0f;
  if(!input && !off() && (openFlags() & 1 /*write*/))
  {
    worst_self_latency = getWorstSelfLatencyAudio();
    const float worst_midi = getWorstSelfLatencyMidi(false /*playback*/);
    if(worst_midi > worst_self_latency)
      worst_self_latency = worst_midi;
  }
      
  // The _trackLatency should already be calculated in the dominance scan.
  const float branch_lat = callerBranchLatency + worst_self_latency;

  // We want the AudioTrack in routes, not the MidiDevice in routes.
  RouteList* rl = AudioTrack::inRoutes();
  for (ciRoute ir = rl->cbegin(); ir != rl->cend(); ++ir)
  {
    if(ir->type != Route::TRACK_ROUTE || !ir->track || ir->track->isMidiTrack())
      continue;
    Track* track = ir->track;
    if(!off() && !track->off() && (passthru || input))
      track->setCorrectionLatencyInfo(false, finalWorstLatency, branch_lat);
  }

  const int port = midiPort();
  if(port >= 0 && port < MusECore::MIDI_PORTS)
  {
#ifdef _USE_MIDI_TRACK_SINGLE_OUT_PORT_CHAN_
    const MidiTrackList& tl = *MusEGlobal::song->midis();
    const MidiTrackList::size_type tl_sz = tl.size();
    for(MidiTrackList::size_type it = 0; it < tl_sz; ++it)
    {
      MidiTrack* track = static_cast<MidiTrack*>(tl[it]);
      if(track->outPort() != port)
        continue;
      if(!off() && (openFlags() & 1 /*write*/) && !track->off() && (passthru || input))
        track->setCorrectionLatencyInfo(false, finalWorstLatency, branch_lat);
    }

#else

    MidiPort* mp = &MusEGlobal::midiPorts[port];
    RouteList* mrl = mp->inRoutes();
    for (iRoute ir = mrl->begin(); ir != mrl->end(); ++ir) {
          switch(ir->type)
          {
              case Route::TRACK_ROUTE:
                if(!ir->track)
                  continue;
                
                if(ir->track->isMidiTrack())
                {
                  if(ir->channel < -1 || ir->channel >= MusECore::MUSE_MIDI_CHANNELS)
                    continue;

                  Track* track = ir->track;
//                     if(ir->channel < 0)
//                       all_chans = true;
//                     else
//                       used_chans[ir->channel] = true;
                    
                  if(!off() && (openFlags() & 1 /*write*/) && !track->off() && (passthru || input))
                    track->setCorrectionLatencyInfo(false, finalWorstLatency, branch_lat);
                }
              break;
              
              default:
              break;
          }
    }

#endif

  }

  // Special for the built-in metronome.
  if(!capture)
  {
    MusECore::MetronomeSettings* metro_settings = 
      MusEGlobal::metroUseSongSettings ? &MusEGlobal::metroSongSettings : &MusEGlobal::metroGlobalSettings;

    //if(sendMetronome())
    if(metro_settings->midiClickFlag && metro_settings->clickPort == port)
    {
      if(!off() && (openFlags() & 1 /*write*/) && !MusECore::metronome->off() && (passthru || input))
        MusECore::metronome->setCorrectionLatencyInfoMidi(capture, finalWorstLatency, branch_lat);
    }
  }

  // Set the correction of all connected input branches,
  //  but ONLY if the track is not off.
  if(!off() && (openFlags() & 1 /*write*/))
  {
    if(input)
    {
    }
    else
    {
      if(canCorrectOutputLatencyMidi() &&tli->_canCorrectOutputLatency)
      {
        float corr = 0.0f;
        if(MusEGlobal::config.commonProjectLatency)
          corr -= finalWorstLatency;

        corr -= branch_lat;
        // The _sourceCorrectionValue is initialized to zero.
        // Whichever calling branch needs the most correction gets it.
        if(corr < tli->_sourceCorrectionValue)
          tli->_sourceCorrectionValue = corr;
      }
    }
  }

  return *tli;
}

//---------------------------------------------------------
//   getLatencyInfoMidi
//---------------------------------------------------------

TrackLatencyInfo& SynthI::getLatencyInfoMidi(bool capture, bool input)
{
  TrackLatencyInfo* tli = capture ? &_captureLatencyInfo : &_playbackLatencyInfo;

  // Have we been here before during this scan?
  // Just return the cached value.
  if((input && tli->_inputProcessed) ||
    (!input && tli->_processed))
    return *tli;

  float route_worst_latency = tli->_inputLatency;

  MusECore::MetronomeSettings* metro_settings = 
    MusEGlobal::metroUseSongSettings ? &MusEGlobal::metroSongSettings : &MusEGlobal::metroGlobalSettings;

  const bool passthru = canPassThruLatencyMidi(capture);

  // Now that we know the worst-case latency of the connected branches,
  //  adjust each of the conveniently stored temporary latency values
  //  in the routes according to whether they can dominate...
  RouteList* rl = AudioTrack::inRoutes();
  for (iRoute ir = rl->begin(); ir != rl->end(); ++ir)
  {
    if(ir->type != Route::TRACK_ROUTE || !ir->track || ir->track->isMidiTrack())
      continue;

    Track* track = ir->track;

    if(passthru || input)
    {
      // Default to zero.
      ir->audioLatencyOut = 0.0f;

      if(!off() && !track->off())
      {
        const TrackLatencyInfo& li = track->getLatencyInfo(false);
        const bool participate =
          li._canCorrectOutputLatency ||
          li._canDominateOutputLatency ||
          MusEGlobal::config.correctUnterminatedInBranchLatency;

        if(participate)
        {
          // Prepare the latency value to be passed to the compensator's writer,
          //  by adjusting each route latency value. ie. the route with the worst-case
          //  latency will get ZERO delay, while routes having smaller latency will get
          //  MORE delay, to match all the signal timings together.
          // The route's audioLatencyOut should have already been calculated and
          //  conveniently stored in the route.
          ir->audioLatencyOut = route_worst_latency - li._outputLatency;
          // Should not happen, but just in case.
          if((long int)ir->audioLatencyOut < 0)
            ir->audioLatencyOut = 0.0f;
        }
      }
    }
  }

  const int port = midiPort();

  if(port >= 0 && port < MusECore::MIDI_PORTS)
  {
#ifdef _USE_MIDI_TRACK_SINGLE_OUT_PORT_CHAN_
    const MidiTrackList& tl = *MusEGlobal::song->midis();
    const MidiTrackList::size_type tl_sz = tl.size();
    for(MidiTrackList::size_type it = 0; it < tl_sz; ++it)
    {
      MidiTrack* track = static_cast<MidiTrack*>(tl[it]);
      if(track->outPort() != port)
        continue;

      if(passthru || input)
      {
        // TODO: FIXME: Where to store? We have no route to store it in.
        // Default to zero.
        //ir->audioLatencyOut = 0.0f;

        if(!off() && (openFlags() & (capture ? 2 : 1)) && !track->off())
        {
          TrackLatencyInfo& li = track->getLatencyInfo(false);
          const bool participate =
            li._canCorrectOutputLatency ||
            li._canDominateOutputLatency ||
            MusEGlobal::config.correctUnterminatedInBranchLatency;

          if(participate)
          {
            // TODO: FIXME: Where to store? We have no route to store it in.
            // Prepare the latency value to be passed to the compensator's writer,
            //  by adjusting each route latency value. ie. the route with the worst-case
            //  latency will get ZERO delay, while routes having smaller latency will get
            //  MORE delay, to match all the signal timings together.
            // The route's audioLatencyOut should have already been calculated and
            //  conveniently stored in the route.
  //           ir->audioLatencyOut = route_worst_latency - ir->audioLatencyOut;
  //           // Should not happen, but just in case.
  //           if((long int)ir->audioLatencyOut < 0)
  //             ir->audioLatencyOut = 0.0f;

            // Special for Midi Tracks: We don't have Midi Track to Midi Port routes yet
            //  because we don't have multiple Midi Track outputs yet, only a single output port.
            // So we must store this information here just for Midi Tracks.
            li._latencyOutMidiTrack = route_worst_latency - li._outputLatency;
            // Should not happen, but just in case.
            if((long int)li._latencyOutMidiTrack < 0)
              li._latencyOutMidiTrack = 0.0f;
          }
        }
      }
    }

#else

    for (iRoute ir = rl->begin(); ir != rl->end(); ++ir)
    {
      switch(ir->type)
      {
        case Route::TRACK_ROUTE:
          if(!ir->track)
            continue;

          if(ir->track->isMidiTrack())
          {
            if(ir->channel < -1 || ir->channel >= MusECore::MUSE_MIDI_CHANNELS)
              continue;

            Track* track = ir->track;

            if(passthru || input)
            {
              // Default to zero.
              ir->audioLatencyOut = 0.0f;

              if(!off() && (openFlags() & (capture ? 2 : 1)) && !track->off())
              {
                TrackLatencyInfo& li = track->getLatencyInfo(false);
                const bool participate =
                  li._canCorrectOutputLatency ||
                  li._canDominateOutputLatency ||
                  MusEGlobal::config.correctUnterminatedInBranchLatency;

                if(participate)
                {
                  // Prepare the latency value to be passed to the compensator's writer,
                  //  by adjusting each route latency value. ie. the route with the worst-case
                  //  latency will get ZERO delay, while routes having smaller latency will get
                  //  MORE delay, to match all the signal timings together.
                  // The route's audioLatencyOut should have already been calculated and
                  //  conveniently stored in the route.
                  ir->audioLatencyOut = route_worst_latency - li._outputLatency;
                  // Should not happen, but just in case.
                  if((long int)ir->audioLatencyOut < 0)
                    ir->audioLatencyOut = 0.0f;
                }
              }
            }
          }
        break;

        default:
        break;
      }
    }

#endif

    // Special for the built-in metronome.
    if(!capture)
    {
      if(passthru || input)
      {
        // TODO: FIXME: Where to store? We have no route to store it in.
        // Default to zero.
        //ir->audioLatencyOut = 0.0f;

        if((openFlags() & 1 /*write*/) && !MusECore::metronome->off() &&  // sendMetronome() &&
            metro_settings->midiClickFlag && metro_settings->clickPort == port)
        {
          TrackLatencyInfo& li = MusECore::metronome->getLatencyInfoMidi(capture, false);
          const bool participate =
            (li._canCorrectOutputLatency ||
            li._canDominateOutputLatency ||
            MusEGlobal::config.correctUnterminatedInBranchLatency);

          if(participate)
          {
            // TODO: FIXME: Where to store? We have no route to store it in.
            // Prepare the latency value to be passed to the compensator's writer,
            //  by adjusting each route latency value. ie. the route with the worst-case
            //  latency will get ZERO delay, while routes having smaller latency will get
            //  MORE delay, to match all the signal timings together.
            // The route's audioLatencyOut should have already been calculated and
            //  conveniently stored in the route.

//             ir->audioLatencyOut = route_worst_latency - ir->audioLatencyOut;
//             // Should not happen, but just in case.
//             if((long int)ir->audioLatencyOut < 0)
//               ir->audioLatencyOut = 0.0f;

//                 // Special for Midi Tracks: We don't have Midi Track to Midi Port routes yet
//                 //  because we don't have multiple Midi Track outputs yet, only a single output port.
//                 // So we must store this information here just for Midi Tracks.
//                 li._latencyOutMidiTrack = route_worst_latency - li._outputLatency;
//                 // Should not happen, but just in case.
//                 if((long int)li._latencyOutMidiTrack < 0)
//                   li._latencyOutMidiTrack = 0.0f;

            // Special for metronome: We don't have metronome routes yet.
            // So we must store this information here just for the metronome.
            li._latencyOutMetronome = route_worst_latency - li._outputLatency;
            // Should not happen, but just in case.
            if((long int)li._latencyOutMetronome < 0)
              li._latencyOutMetronome = 0.0f;
          }
        }
      }
    }
  }

  if(input)
    tli->_inputProcessed = true;
  else
    tli->_processed = true;

  return *tli;
}

inline unsigned long SynthI::latencyCompWriteOffsetMidi(bool capture) const
{
  const TrackLatencyInfo* tli = capture ? &_captureLatencyInfo : &_playbackLatencyInfo;
  return tli->_compensatorWriteOffset;
}

void SynthI::setLatencyCompWriteOffsetMidi(float worstCase, bool capture)
{
  TrackLatencyInfo* tli = capture ? &_captureLatencyInfo : &_playbackLatencyInfo;
  
  // If independent branches are NOT to affect project latency,
  //  then there should be no need for any extra delay in the branch.
  if(!MusEGlobal::config.commonProjectLatency)
  {
    tli->_compensatorWriteOffset = 0;
    //fprintf(stderr, "SynthI::setLatencyCompWriteOffset() name:%s capture:%d worstCase:%f _outputLatency:%f _compensatorWriteOffset:%lu\n",
    //        name().toLatin1().constData(), capture, worstCase, tli->_outputLatency, tli->_compensatorWriteOffset);
    return;
  }
    
  if(tli->_canDominateOutputLatency)
  {
    const long unsigned int wc = worstCase;
    const long unsigned int ol = tli->_outputLatency;
    if(ol > wc)
      tli->_compensatorWriteOffset = 0;
    else
      tli->_compensatorWriteOffset = wc - ol;
  }
  else
  {
//     if(tli->_outputLatency < 0)
      tli->_compensatorWriteOffset = 0;
//     else
//       tli->_compensatorWriteOffset = tli->_outputLatency;
  }

  //fprintf(stderr, "SynthI::setLatencyCompWriteOffset() name:%s capture:%d worstCase:%f"
  //    " _outputLatency:%f _canDominateOutputLatency:%d _compensatorWriteOffset:%lu\n",
  //        name().toLatin1().constData(), capture, worstCase, tli->_outputLatency, tli->_canDominateOutputLatency, tli->_compensatorWriteOffset);
}

//================================================
// END Latency correction/compensation routines.
//================================================


//---------------------------------------------------------
//   preProcessAlways
//---------------------------------------------------------

void SynthI::preProcessAlways()
{
  AudioTrack::preProcessAlways();
  if(_sif)
    _sif->preProcessAlways();

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
    // Eat up any buffer events.
    _playbackEventBuffers->clearRead();
    //_userEventBuffers->clearRead();
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

      _sif->getData(mp, pos, ports, n, buffer);

      return true;
      }

bool MessSynthIF::getData(MidiPort* /*mp*/, unsigned pos, int /*ports*/, unsigned n, float** buffer)
{
      const unsigned int syncFrame = MusEGlobal::audio->curSyncFrame();
      unsigned int curPos = 0;
      unsigned int frame = 0;

      // Get the state of the stop flag.
      const bool do_stop = synti->stopFlag();

      MidiPlayEvent buf_ev;
      
      // Transfer the user lock-free buffer events to the user sorted multi-set.
      // False = don't use the size snapshot, but update it.
      const unsigned int usr_buf_sz = synti->eventBuffers(MidiDevice::UserBuffer)->getSize(false);
      for(unsigned int i = 0; i < usr_buf_sz; ++i)
      {
        if(synti->eventBuffers(MidiDevice::UserBuffer)->get(buf_ev))
          synti->_outUserEvents.insert(buf_ev);
      }
      
      // Transfer the playback lock-free buffer events to the playback sorted multi-set.
      const unsigned int pb_buf_sz = synti->eventBuffers(MidiDevice::PlaybackBuffer)->getSize(false);
      for(unsigned int i = 0; i < pb_buf_sz; ++i)
      {
        // Are we stopping? Just remove the item.
        if(do_stop)
          synti->eventBuffers(MidiDevice::PlaybackBuffer)->remove();
        // Otherwise get the item.
        else if(synti->eventBuffers(MidiDevice::PlaybackBuffer)->get(buf_ev))
          synti->_outPlaybackEvents.insert(buf_ev);
      }
  
      // Are we stopping?
      if(do_stop)
      {
        // Transport has stopped, purge ALL further scheduled playback events now.
        synti->_outPlaybackEvents.clear();
        // Reset the flag.
        synti->setStopFlag(false);
      }
      
      iMPEvent impe_pb = synti->_outPlaybackEvents.begin();
      iMPEvent impe_us = synti->_outUserEvents.begin();
      bool using_pb;
  
      while(1)
      {  
        if(impe_pb != synti->_outPlaybackEvents.end() && impe_us != synti->_outUserEvents.end())
          using_pb = *impe_pb < *impe_us;
        else if(impe_pb != synti->_outPlaybackEvents.end())
          using_pb = true;
        else if(impe_us != synti->_outUserEvents.end())
          using_pb = false;
        else break;
        
        const MidiPlayEvent& ev = using_pb ? *impe_pb : *impe_us;
        
        const unsigned int evTime = ev.time();
        if(evTime < syncFrame)
        {
          if(evTime != 0)
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
        // C++11.
        if(using_pb)
          impe_pb = synti->_outPlaybackEvents.erase(impe_pb);
        else
          impe_us = synti->_outUserEvents.erase(impe_us);
      }

      if(curPos < n)
      {
        if (!_mess)
          fprintf(stderr, "MessSynthIF::getData() should not happen - no _mess\n");
        else
          _mess->process(pos, buffer, curPos, n - curPos);
      }
      
      return true;
}

//---------------------------------------------------------
//   putEvent
//    return true on error (busy)
//---------------------------------------------------------

bool MessSynthIF::processEvent(const MidiPlayEvent& ev)
{
      if (!_mess)
        return true;
      
      if (MusEGlobal::midiOutputTrace)
      {
           fprintf(stderr, "MidiOut: MESS: <%s>: ", synti->name().toLatin1().constData());
           dumpMPEvent(&ev);
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

int MessSynthIF::oldMidiStateHeader(const unsigned char** data) const
{
  return _mess ? _mess->oldMidiStateHeader(data) : 0;
}

} // namespace MusECore
