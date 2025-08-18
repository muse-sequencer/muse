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

#include <vector>
#include <stdio.h>

#include <QDir>
#include <QString>

#include "app.h"
#include "synth.h"
#include "midi_consts.h"
#include "song.h"
#include "audio.h"
#include "event.h"
#include "mpevent.h"
#include "midictrl.h"
#include "midiitransform.h"
#include "mitplugin.h"
#include "helper.h"
#include "gconfig.h"
#include "globals.h"
#include "plugin_list.h"
#include "pluglist.h"
#include "ticksynth.h"
#include "undo.h"
#include "midiremote.h"
#include "hex_float.h"

// Forwards from header:
#include "midiport.h"
#include "synti/libsynti/mess.h"
#include "popupmenu.h"
#include "xml.h"
#include "xml_statistics.h"
#include "plugin_scan.h"
#include "ctrl.h"

// Undefine if and when multiple output routes are added to midi tracks.
#define _USE_MIDI_TRACK_SINGLE_OUT_PORT_CHAN_

// For debugging output: Uncomment the fprintf section.
#define DEBUG_SYNTH(dev, format, args...) // fprintf(dev, format, ##args);

namespace MusEGlobal {
  MusECore::SynthList synthis;  // array of available MusEGlobal::synthis
}

namespace MusECore {

extern void connectNodes(AudioTrack*, AudioTrack*);
bool SynthI::_isVisible=false;

//---------------------------------------------------------
//   find
//---------------------------------------------------------

Synth* SynthList::find(
  MusEPlugin::PluginTypes_t pluginTypes,
  const QString& fileCompleteBaseName,
  const QString& pluginUri,
  const QString& pluginLabel,
  bool useFileBaseName) const
      {
      const bool f_empty = fileCompleteBaseName.isEmpty();
      const bool u_empty = pluginUri.isEmpty();
      const bool l_empty = pluginLabel.isEmpty();
      for (ciSynthList i = cbegin(); i != cend(); ++i) {
            const Synth *sy = *i;
            if ((!u_empty || f_empty ||
                 fileCompleteBaseName == (useFileBaseName ? sy->baseName() : sy->completeBaseName())) &&
                (u_empty || pluginUri  == sy->uri()) &&
                (!u_empty || l_empty || pluginLabel == sy->label()) &&
                (pluginTypes & sy->pluginType()))
                  return *i;
            }

      return nullptr;
      }

//--------------------------------
//  SynthIF
//--------------------------------

SynthIF::SynthIF(SynthI* s) : PluginIBase()
{
  synti = s;
}

void SynthIF::write(int, Xml&) const { }

bool SynthIF::setupController(CtrlList *cl) const
{
  if(!cl || cl->id() < 0)
    return false;

  // Strip away any upper id bits.
  const int ctlnum = cl->id() & AC_PLUGIN_CTL_ID_MASK;

  if(ctlnum < 0 || (unsigned long int)ctlnum >= parameters())
    return false;

  float min, max;
  range(ctlnum, &min, &max);
  cl->setRange(min, max);
  cl->setName(QString(paramName(ctlnum)));
  cl->setValueType(ctrlValueType(ctlnum));
  cl->setMode(ctrlMode(ctlnum));
  cl->setCurVal(param(ctlnum));
  // Set the value units index.
  cl->setValueUnit(valueUnit(ctlnum));

  return true;
}

//--------------------------------
// Methods for PluginIBase:
//--------------------------------

MusEPlugin::PluginFeatures_t SynthIF::requiredFeatures() const { return MusEPlugin::PluginNoFeatures; }
bool SynthIF::hasActiveButton() const { return false; }
// Here we defer to the track's 'off' state.
bool SynthIF::active() const { return !synti->off(); }
// Here we defer to setting the track's 'off' state.
void SynthIF::setActive(bool val) { synti->setOff(!val); }
// Synth is not part of a rack plugin chain. It has no on/off (bypass) feature.
bool SynthIF::hasBypass() const                          { return false; }
// Synth is not part of a rack plugin chain. Always on.
bool SynthIF::on() const                                 { return true; }
void SynthIF::setOn(bool /*val*/)                        { }
unsigned long SynthIF::pluginID() const                  { return 0; }
int SynthIF::id() const                                  { return MusECore::MAX_PLUGINS; } // Set for special block reserved for synth.
QString SynthIF::name() const                            { return synti->name(); }
QString SynthIF::displayName() const
{ return titlePrefix() + pluginName(); }
QString SynthIF::pluginLabel() const          { return synti->synth() ? synti->synth()->label() : QString(); }
QString SynthIF::pluginName() const           { return synti->synth() ? synti->synth()->name() : QString(); }
QString SynthIF::lib() const          { return synti->synth() ? synti->synth()->completeBaseName() : QString(); }
QString SynthIF::uri() const                             { return synti->uri(); }
QString SynthIF::dirPath() const      { return synti->synth() ? synti->synth()->absolutePath() : QString(); }
QString SynthIF::fileName() const     { return synti->synth() ? synti->synth()->fileName() : QString(); }
QString SynthIF::titlePrefix() const                     { return synti->displayName() + QString(": "); }
MusECore::AudioTrack* SynthIF::track() const             { return static_cast < MusECore::AudioTrack* > (synti); }
void SynthIF::enableController(unsigned long, bool)  { }
bool SynthIF::controllerEnabled(unsigned long) const   { return true;}
void SynthIF::enableAllControllers(bool)               { }
void SynthIF::updateController(unsigned long /*i*/)      { }
void SynthIF::updateControllers()                        { }
void SynthIF::activate()
{
    _curActiveState = true;
}
void SynthIF::deactivate() { _curActiveState = false; }
void SynthIF::writeConfiguration(int /*level*/, Xml& /*xml*/, bool /*isCopy*/)        { }
bool SynthIF::readConfiguration(Xml& /*xml*/, bool /*readPreset*/, int /*channels*/) { return false; }
std::vector<QString> SynthIF::getCustomData() const { return std::vector<QString>(); }
unsigned long SynthIF::parameters() const                { return 0; }
unsigned long SynthIF::parametersOut() const             { return 0; }
void SynthIF::setParam(unsigned long, double)       { }
double SynthIF::param(unsigned long) const              { return 0.0; }
double SynthIF::paramOut(unsigned long) const          { return 0.0; }
const char* SynthIF::paramName(unsigned long) const    { return nullptr; }
const char* SynthIF::paramOutName(unsigned long) const { return nullptr; }
LADSPA_PortRangeHint SynthIF::range(unsigned long) const
{
  LADSPA_PortRangeHint h;
  h.HintDescriptor = 0;
  h.LowerBound = 0.0;
  h.UpperBound = 1.0;
  return h;
}
LADSPA_PortRangeHint SynthIF::rangeOut(unsigned long) const
{
  LADSPA_PortRangeHint h;
  h.HintDescriptor = 0;
  h.LowerBound = 0.0;
  h.UpperBound = 1.0;
  return h;
}
void SynthIF::range(unsigned long /*i*/, float* min, float* max) const
{
  *min = 0.0;
  *max = 1.0;
}
void SynthIF::rangeOut(unsigned long /*i*/, float* min, float* max) const
{
  *min = 0.0;
  *max = 1.0;
}
unsigned long SynthIF::latencyOutPortIndex() const { return synti->latencyOutPortIndex(); }
unsigned long SynthIF::freewheelPortIndex() const { return synti->freewheelPortIndex(); }
unsigned long SynthIF::enableOrBypassPortIndex() const
{ return synti->enableOrBypassPortIndex(); }

MusEPlugin::PluginLatencyReportingType SynthIF::pluginLatencyReportingType() const
{ return synti->pluginLatencyReportingType(); }

MusEPlugin::PluginBypassType SynthIF::pluginBypassType() const
{ return synti->pluginBypassType(); }

MusEPlugin::PluginFreewheelType SynthIF::pluginFreewheelType() const
{ return synti->pluginFreewheelType(); }

CtrlValueType SynthIF::ctrlValueType(unsigned long) const { return VAL_LINEAR; }
CtrlList::Mode SynthIF::ctrlMode(unsigned long) const     { return CtrlList::INTERPOLATE; }
CtrlValueType SynthIF::ctrlOutValueType(unsigned long) const { return VAL_LINEAR; }
CtrlList::Mode SynthIF::ctrlOutMode(unsigned long) const     { return CtrlList::INTERPOLATE; }

bool SynthIF::usesTransportSource() const { return false; }

float SynthIF::latency() const
{
  // Do not report any latency if the plugin is not active.
  if(!_curActiveState)
    return 0.0;

  switch(pluginBypassType())
  {
    // If the plugin has no enable/bypass feature and
    //  we are emulating it, if the plugin is bypassed
    //  do not report any latency since our emulated
    //  pass-through does not introduce any latency.
    case MusEPlugin::PluginBypassTypeEmulatedEnableFunction:
    case MusEPlugin::PluginBypassTypeEmulatedEnableController:
      if(!on())
        return 0.0;
    break;

    // Otherwise if the plugin has an enable/bypass feature,
    //  go ahead and allow the plugin to report its latency,
    //  regardless of whether it is bypassed or not.
    case MusEPlugin::PluginBypassTypeEnableFunction:
    case MusEPlugin::PluginBypassTypeEnablePort:
    case MusEPlugin::PluginBypassTypeBypassFunction:
    case MusEPlugin::PluginBypassTypeBypassPort:
    break;
  }

  if(cquirks()._overrideReportedLatency)
    return cquirks()._latencyOverrideValue;

  switch(pluginLatencyReportingType())
  {
    case MusEPlugin::PluginLatencyTypeNone:
    break;

    case MusEPlugin::PluginLatencyTypeFunction:
      if(synti)
        return synti->getPluginLatency(nullptr);
    break;

    case MusEPlugin::PluginLatencyTypePort:
      if(latencyOutPortIndex() < parametersOut())
        return paramOut(latencyOutPortIndex());
    break;
  }
  return 0.0;
}

//-------------------------------------------------------------------------



bool MessSynthIF::nativeGuiVisible() const
      {
      return _mess ? _mess->nativeGuiVisible() : false;
      }

void MessSynthIF::showNativeGui(bool v)
      {
      PluginIBase::showNativeGui(v);

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
void MessSynthIF::updateNativeGuiWindowTitle()
{
  if(_mess)
    _mess->setNativeGuiWindowTitle(displayName().toUtf8().constData());
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
//   createSynthInstance
//    create a synthesizer instance of class "label"
//---------------------------------------------------------

static SynthI* createSynthInstance(
  MusEPlugin::PluginType type,
  const QString& file, const QString& uri,
  const QString& label)
      {
      Synth* s = MusEGlobal::synthis.find(type, file, uri, label);
      SynthI* si = nullptr;
      if (s) {
            si = new SynthI();
            QString n;
            n.setNum(s->references());
            QString instance_name = s->label() + "-" + n;
            //Andrew Deryabin: check si->_sif for NULL as synth instance may not be created.
               if (si->initInstance(s, instance_name)) {
                  delete si;
                  fprintf(stderr, "createSynthInstance: synthi class:%s label:%s can not be created\n",
                          file.toLocal8Bit().constData(), label.toLocal8Bit().constData());
                  QMessageBox::warning(0,"Synth instantiation error!",
                              "Synth: " + label + " can not be created!");
                  return nullptr;
               }
            }
      else {
            fprintf(stderr, "createSynthInstance: synthi class:%s uri:%s label:%s not found\n",
                    file.toLocal8Bit().constData(), uri.toLocal8Bit().constData(), label.toLocal8Bit().constData());
            QMessageBox::warning(0,"Synth not found!",
                        "Synth: " + label + " not found, if the project is saved it will be removed from the project");
      }

      return si;
      }

//---------------------------------------------------------
//   Synth
//---------------------------------------------------------

Synth::Synth(const MusEPlugin::PluginScanInfoStruct& infoStruct) : PluginBase(infoStruct)
{
}

Synth::~Synth() {}

bool Synth::midiToAudioCtrlMapped(unsigned long int midiCtrl, unsigned long int* audioCtrl) const
{
  ciMidiCtl2LadspaPort ic = midiCtl2PortMap.find(midiCtrl);
  if(ic != midiCtl2PortMap.cend())
  {
    if(audioCtrl)
      *audioCtrl = ic->second;
    return true;
  }
  return false;
}

bool Synth::audioToMidiCtrlMapped(unsigned long int audioCtrl, unsigned long int* midiCtrl) const
{
  ciMidiCtl2LadspaPort ic = port2MidiCtlMap.find(audioCtrl);
  if(ic != port2MidiCtlMap.cend())
  {
    if(midiCtrl)
      *midiCtrl = ic->second;
    return true;
  }
  return false;
}

bool Synth::hasMappedMidiToAudioCtrls() const
{
  return !midiCtl2PortMap.empty();
}

MessSynth::MessSynth(const MusEPlugin::PluginScanInfoStruct& info)
 : Synth(info), _descr(nullptr)
{

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

      Synth* s = si.synth();
      if (s) {
            QString n;
            n.setNum(s->references());
            QString instance_name = s->label() + "-" + n;
            if(!initInstance(s, instance_name)) {  // false if success

                  if(((flags & ASSIGN_PROPERTIES) && !(flags & ASSIGN_STD_CTRLS)) || (flags & ASSIGN_STD_CTRLS))
                  {
                    int af = CtrlList::ASSIGN_PROPERTIES;
                    if(flags & ASSIGN_STD_CTRLS)
                      af |= CtrlList::ASSIGN_VALUES;

                    const AudioTrack& at = static_cast<const AudioTrack&>(si);
                    AudioTrack* at_this = static_cast<AudioTrack*>(this);
                    // The beginning of the special synth controller block.
                    const int synth_id = (int)genACnum(MusECore::MAX_PLUGINS, 0);
                    // The end of the special block.
                    const int synth_id_end = synth_id + AC_PLUGIN_CTL_BASE;
                    ciCtrlList icl           = at.controller()->lower_bound(synth_id);
                    ciCtrlList icl_this      = at_this->controller()->lower_bound(synth_id);
                    ciCtrlList icl_end       = at.controller()->lower_bound(synth_id_end);
                    ciCtrlList icl_this_end  = at_this->controller()->lower_bound(synth_id_end);
                    int id, id_this;
                    CtrlList* cl, *cl_this;

                    // Copy the special synth controller block...
                    while(icl != icl_end && icl_this != icl_this_end)
                    {
                      cl      = icl->second;
                      cl_this = icl_this->second;
                      id      = cl->id();
                      id_this = cl_this->id();
                      if(id < id_this)
                        ++icl;      // Let id catch up to this id.
                      else if(id > id_this)
                        ++icl_this; // Let this id catch up to id.
                      else
                      {
                        // Match found. Copy properties, and values if required.
                        cl_this->assign(*cl, af);
                        ++icl;
                        ++icl_this;
                      }
                    }
                  }
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
  _openFlags &= _rwFlags; // restrict to available bits

  // Make it behave like a regular midi device.
  _readEnable = (_openFlags & 0x02);
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
              const unsigned char* p = event.constData();
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
      // transfer also to gui for realtime playback and remote control
      {
        const bool nt = typ == ME_NOTEON || typ == ME_NOTEOFF;
        //const bool cc = typ == ME_CONTROLLER;
        //const bool pbpg = typ == ME_PITCHBEND || typ == ME_PROGRAM;
        const MidiRemote *curRem = MusEGlobal::midiRemoteUseSongSettings ? MusEGlobal::song->midiRemote() : &MusEGlobal::midiRemote;

        // Try to put only what we need to avoid overloading.
        if (((nt /*|| cc*/) &&
             (curRem->matches(event.port(), event.channel(), event.dataA(), nt, /*cc*/ false, nt) || MusEGlobal::midiRemoteIsLearning)) /*||
            ((cc || pbpg) &&
              MusEGlobal::midiToAudioAssignIsLearning)*/)
        {
          MusEGlobal::song->putEvent(event);
        }
      }

#endif

      // Do not bother recording if it is NOT actually being used by a port.
      // Because from this point on, process handles things, by selected port.
      if(_port == -1)
        return;

      // Split the events up into channel fifos. Special 'channel' number 17 for sysex events.
      unsigned int ch = (typ == ME_SYSEX)? MusECore::MUSE_MIDI_CHANNELS : event.channel();
      if(!_recordFifo[ch]->put(event))
        fprintf(stderr, "SynthI::recordEvent: fifo channel %d overflow\n", ch);
      }


RouteCapabilitiesStruct SynthI::routeCapabilities() const 
{ 
  RouteCapabilitiesStruct s = AudioTrack::routeCapabilities();
  s._trackChannels._inChannels = totalInChannels();
  s._trackChannels._inRoutable = (s._trackChannels._inChannels != 0);
  return s;
}

void SynthI::getMapItem(int channel, int patch, int index, DrumMap& dest_map, int
#ifdef _USE_INSTRUMENT_OVERRIDES_
  overrideType
#endif
) const
{
  bool has_note_name_list = false;
  QString note_name;
  if(_sif)
  {
    // true = Want percussion names, not melodic.
    has_note_name_list = _sif->getNoteSampleName(true, channel, patch, index, &note_name);
  }

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
        if(has_note_name_list)
          // It has a note name list. The note name can be blank meaning no note found.
          dest_map.name = note_name;
        return;
      }
    }
  }

  dest_map = iNewDrumMap[index];
  if(has_note_name_list)
    // It has a note name list. The note name can be blank meaning no note found.
    dest_map.name = note_name;
}

bool SynthI::midiToAudioCtrlMapped(unsigned long int midiCtrl, unsigned long int* audioCtrl) const
{
  if(synthesizer)
    return synthesizer->midiToAudioCtrlMapped(midiCtrl, audioCtrl);
  return false;
}

bool SynthI::audioToMidiCtrlMapped(unsigned long int audioCtrl, unsigned long int* midiCtrl) const
{
  if(synthesizer)
    return synthesizer->audioToMidiCtrlMapped(audioCtrl, midiCtrl);
  return false;
}

bool SynthI::hasMappedMidiToAudioCtrls() const
{
  if(synthesizer)
    return synthesizer->hasMappedMidiToAudioCtrls();
  return false;
}

void SynthI::guiHeartBeat()
{
  // Update the track's rack plugin generic or native UIs.
  AudioTrack::guiHeartBeat();
  // Update the synth's generic or native UI.
  if(_sif)
    _sif->guiHeartBeat();
}

unsigned long SynthI::latencyOutPortIndex() const { return synthesizer ? synthesizer->latencyPortIndex() : 0; }
unsigned long SynthI::freewheelPortIndex() const { return synthesizer ? synthesizer->freewheelPortIndex() : 0; }
unsigned long SynthI::enableOrBypassPortIndex() const { return synthesizer ? synthesizer->enableOrBypassPortIndex() : 0; }
MusEPlugin::PluginLatencyReportingType SynthI::pluginLatencyReportingType() const
{ return synthesizer ? synthesizer->pluginLatencyReportingType() : MusEPlugin::PluginLatencyTypeNone; }
MusEPlugin::PluginBypassType SynthI::pluginBypassType() const
{ return synthesizer ? synthesizer->pluginBypassType() : MusEPlugin::PluginBypassTypeEmulatedEnableFunction; }
MusEPlugin::PluginFreewheelType SynthI::pluginFreewheelType() const
{ return synthesizer ? synthesizer->pluginFreewheelType() : MusEPlugin::PluginFreewheelTypeNone; }
// Returns the plugin latency, if it has such as function.
// NOTE: If the plugin has a latency controller out, use that instead.
float SynthI::getPluginLatency(void* h) { return synthesizer ? synthesizer->getPluginLatency(h) : 0.0; }

//---------------------------------------------------------
//   init
//---------------------------------------------------------

bool MessSynthIF::init(MessSynth* s)
      {
      QByteArray configPathBA      = MusEGlobal::configPath.toUtf8();
      QByteArray cachePathBA       = MusEGlobal::cachePath.toUtf8();
      QByteArray museGlobalLibBA   = MusEGlobal::museGlobalLib.toUtf8();
      QByteArray museGlobalShareBA = MusEGlobal::museGlobalShare.toUtf8();
      QByteArray museUserBA        = MusEGlobal::museUser.toUtf8();
      QByteArray museProjectBA     = MusEGlobal::museProject.toUtf8();
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
      _mess = s->_descr->instantiate((unsigned long long)MusEGlobal::muse->winId(),
                                       displayName().toUtf8().constData(), &mcfg);

      return (_mess != nullptr);
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

bool MessSynth::reference()
{
    if (_references == 0)
    {
      _qlib.setFileName(filePath());
      // Same as dlopen RTLD_NOW.
      _qlib.setLoadHints(QLibrary::ResolveAllSymbolsHint);
      if(!_qlib.load())
      {
        fprintf(stderr, "MessSynth::reference(): load (%s) failed: %s\n",
          _qlib.fileName().toLocal8Bit().constData(), _qlib.errorString().toLocal8Bit().constData());
        return false;
      }

      MESS_Descriptor_Function msynth = (MESS_Descriptor_Function)_qlib.resolve("mess_descriptor");
      if(msynth)
        _descr = msynth();
    }

    ++_references;

    if(_descr == nullptr)
    {
      fprintf(stderr, "MessSynth::reference() Error: cannot find MESS synti %s\n", _label.toLocal8Bit().constData());
      release();
      return false;
    }

    return true;
}

int MessSynth::release()
{
  if (_references == 1)
  {
        // Attempt to unload the library.
        // It will remain loaded if the plugin has shell plugins still in use or there are other references.
        const bool ulres = _qlib.unload();
        // Dummy usage stops unused warnings.
        (void)ulres;
        DEBUG_SYNTH(stderr, "MessSynth::release(): No more instances. Result of unloading library %s: %d\n",
          _qlib.fileName().toLocal8Bit().constData(), ulres);

        _descr = nullptr;
        midiCtl2PortMap.clear();
        port2MidiCtlMap.clear();
  }
  if(_references > 0)
    --_references;
  return _references;
}

SynthIF* MessSynth::createSIF(SynthI* si)
{
    if(!reference())
      return nullptr;

    MessSynthIF* sif = new MessSynthIF(si);
    if(!sif->init(this))
    {
      fprintf(stderr, "DssiSynth::createSIF() Error: plugin:%s instantiate failed!\n", _label.toLocal8Bit().constData());
      delete sif;
      release();
      return nullptr;
    }
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

      // Persistent storage. Even if the synth is not found, allow the track to load.
      if(!s)
      {
        _sif = nullptr;
        return true;
      }

      _sif        = s->createSIF(this);

      //Andrew Deryabin: add check for NULL here to get rid of segfaults
      if(_sif == nullptr)
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

bool MessSynthIF::getNoteSampleName(
  bool drum, int channel, int patch, int note, QString* name) const
{
  if(!name)
    return false;

  const char* str;
  // Returns true if a note name list was found.
  // str is NULL if no note was found.
  // drum = Want percussion names, not melodic.
  if(_mess->getNoteSampleName(drum, channel, patch, note, &str))
  {
    // str could be null.
    *name = QString(str);
    // A note name list was found.
    return true;
  }

  // No note name list was found.
  return false;
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
         synthesizer->release();

      }

void MessSynthIF::deactivate3()
      {
      if (_mess) {
            delete _mess;
            _mess = nullptr;
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
      case MusEPlugin::PluginTypeMESS:
      {
        if(MusEGlobal::loadMESS)
        {
          const QString uri = PLUGIN_GET_QSTRING(info._uri);
          // Make sure it doesn't already exist.
          if(const Synth* sy = MusEGlobal::synthis.find(
              info._type,
              PLUGIN_GET_QSTRING(info._completeBaseName),
              uri,
              PLUGIN_GET_QSTRING(info._label)))
          {
            fprintf(stderr, "Ignoring MESS synth name:%s uri:%s path:%s duplicate of path:%s\n",
                    PLUGIN_GET_QSTRING(info._name).toLocal8Bit().constData(),
                    uri.toLocal8Bit().constData(),
                    PLUGIN_GET_QSTRING(info.filePath()).toLocal8Bit().constData(),
                    sy->filePath().toLocal8Bit().constData());
          }
          else
          {
            MusEGlobal::synthis.push_back(new MessSynth(info));
          }
        }
      }
      break;
      
      case MusEPlugin::PluginTypeLADSPA:
      case MusEPlugin::PluginTypeDSSI:
      case MusEPlugin::PluginTypeDSSIVST:
      case MusEPlugin::PluginTypeVST:
      case MusEPlugin::PluginTypeLV2:
      case MusEPlugin::PluginTypeLinuxVST:
      case MusEPlugin::PluginTypeMETRONOME:
      case MusEPlugin::PluginTypeUnknown:
      case MusEPlugin::PluginTypeNone:
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

SynthI* Song::createSynthI(MusEPlugin::PluginType type, const QString& file, const QString& uri,
                           const QString& label, Track* insertAt)
      {
      SynthI* si = createSynthInstance(type, file, uri, label);
      if(!si)
        return nullptr;

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

const PluginConfiguration& SynthI::initialConfiguration() const { return _initConfig; }

//---------------------------------------------------------
//   configure
//---------------------------------------------------------

void SynthI::configure(const PluginConfiguration& config, PluginIBase::ConfigureOptions_t opts)
{
  if(opts & PluginIBase::ConfigQuirks)
    _sif->setQuirks(config._quirks);

  //call SynthIF::setCustomData(...) with accumulated custom params
  bool hasCustomData = false;
  if(opts & PluginIBase::ConfigCustomData)
  {
    if(_sif->setCustomData(config._accumulatedCustomParams))
    {
      hasCustomData = true;
      const long unsigned int params = _sif->parameters();
      for(long unsigned int i = 0; i < params; ++i)
      {
        // Disable any automation controller streams so What You See Is What Was Last Stored by the plugin.
        // The process routine will update our own automation controllers with the new values from the plugin.
        _sif->enableController(i, false);
      }
    }
  }

  // Ignore parameters if there is custom data.
  // We only manually set controls if there was NO state data for that.
  // Otherwise a problem might be that the plugin thinks that the controls
  //  were manually altered, and flags its current patch as 'modified'.
  // The assumption is that control or parameter values would be included in the custom data.
  // See getCustomConfiguration() for more info.
  if(!hasCustomData && (opts & PluginIBase::ConfigParams))
  {
    const long unsigned int params = _sif->parameters();
    for (ciPluginControlList ipcl = _initConfig._initParams.cbegin();
         ipcl != _initConfig._initParams.cend(); ++ipcl)
    {
      const PluginControlConfig &cc = ipcl->second;
      if(cc._ctlnum >= 0 && (unsigned long)cc._ctlnum < params)
      {
        _sif->setParameter(cc._ctlnum, cc._val);
        // Disable any controller automation streams so What You See Is What Was Last Stored by us.
        // The process routine will update our own automation controllers with the new values from the plugin.
        _sif->enableController(cc._ctlnum, false);
      }
      else
      {
        fprintf(stderr, "SynthI::configure(%d %s, %f) controller number out of range\n",
          cc._ctlnum, cc._name.toLocal8Bit().constData(), cc._val);
        // Don't break. Let it continue.
        //break;
      }
    }
  }

  if(opts & PluginIBase::ConfigGeometry)
    setGeometry(_initConfig._geometry.x(), _initConfig._geometry.y(),
      _initConfig._geometry.width(), _initConfig._geometry.height());

  if(opts & PluginIBase::ConfigNativeGeometry)
    setNativeGeometry(_initConfig._nativeGeometry.x(), _initConfig._nativeGeometry.y(),
      _initConfig._nativeGeometry.width(), _initConfig._nativeGeometry.height());

  if(opts & PluginIBase::ConfigGui)
    showGuiPending(config._guiVisible);

  if(opts & PluginIBase::ConfigNativeGui)
    showNativeGuiPending(config._nativeGuiVisible);
}

void SynthI::configure(PluginIBase::ConfigureOptions_t opts)
{
  return configure(_initConfig, opts);
}

//---------------------------------------------------------
//   getConfiguration
//---------------------------------------------------------

PluginConfiguration SynthI::getConfiguration() const
{
  // If the plugin is not available, use the persistent values.
  if(!_sif || !synth())
    return _initConfig;

  Synth *s = synth();

  // Plugin is available. Ask it for the values...
  PluginConfiguration conf;

  //=============
  // Basic info
  //=============

  conf._pluginType = s->pluginType();
  conf._file = s->completeBaseName();
  conf._uri = s->uri();
  conf._pluginLabel = s->label();
  conf._name = name();
  conf._id = -1;
  conf._quirks = _sif->cquirks();
  conf._on = true;
  conf._active = true;
  conf._guiVisible = guiVisible();
  conf._nativeGuiVisible = nativeGuiVisible();
  // We won't need these. Reset them.
  conf._fileVerMaj = conf._fileVerMin = -1;

  int x, y, w, h;
  if (hasGui())
  {
    getGeometry(&x, &y, &w, &h);
    conf._geometry = QRect(x, y, w, h);
  }

  if (hasNativeGui())
  {
    getNativeGeometry(&x, &y, &w, &h);
    conf._nativeGeometry = QRect(x, y, w, h);
  }

  //=============
  // Custom data
  //=============

  conf._accumulatedCustomParams = _sif->getCustomData();

  //==============================
  // Plugin controls or parameters
  //==============================

  const unsigned long sz = _sif->parameters();
  for (unsigned long i = 0; i < sz; ++i)
  {
    float min, max;
    _sif->range(i, &min, &max);
    conf._initParams.insert(std::pair(i,
      PluginControlConfig(
        i,
        // Parameter name.
        QString(_sif->paramName(i)),
        // Parameter value.
        _sif->param(i),
        min, max, _sif->ctrlValueType(i), _sif->ctrlMode(i), _sif->valueUnit(i),
        PluginControlConfig::AllValid)));
  }

  return conf;
}

//---------------------------------------------------------
//   write
//---------------------------------------------------------

void SynthI::write(int level, Xml& xml, XmlWriteStatistics*) const
      {
      xml.tag(level++, "SynthI");

      AudioTrack::writeProperties(level, xml);

      // Get the plugin configuration. Include the automation controllers and midi mapping.
      PluginConfiguration pc = getConfiguration();
      // Write the plugin configuration.
      pc.writeProperties(level, xml, false, false);

      // Support a special block for synth controllers. (track ctrls -> effect rack plugin ctrls -> synth plugin ctrls).
      const int startId = genACnum(MusECore::MAX_PLUGINS, 0);
      const int   endId = genACnum(MusECore::MAX_PLUGINS + 1, 0);
      // Write the block of controllers.
      // Strip away the synth controller base id bits when writing.
      AudioTrack::controller()->write(level, xml, startId, endId, AC_PLUGIN_CTL_ID_MASK);
      // Write any midi assignments to this track's synth controllers.
      // Strip away the synth controller base id bits when writing.
      MidiAudioCtrlMap *macm = MusEGlobal::song->midiAssignments();
      if(macm)
        macm->write(level, xml, this, startId, endId, MidiAudioCtrlStruct::AudioControl, false, AC_PLUGIN_CTL_ID_MASK);

      if(openFlags() != 1)
        xml.intTag(level, "openFlags", openFlags());

      // If soft synth is attached to a midi port, write out port number.
      if(midiPort() != -1)
        xml.intTag(level, "port", midiPort());

      // If the plugin is available, write any special info such as midi state.
      if(_sif)
      {
        _sif->write(level, xml);
      }
      // If the plugin is not available, try to preserve existing midi state.
      else
      {
        const EventList* msl = midiState();
        if(msl && !msl->empty())
        {
          xml.tag(level++, "midistate version=\"%d\"", SYNTH_MIDI_STATE_SAVE_VERSION);
          for(ciEvent ie = msl->cbegin(); ie != msl->cend(); ++ie)
            ie->second.write(level, xml, 0);
          xml.etag(--level, "midistate");
        }
      }

      xml.etag(--level, "SynthI");
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
            xml.etag(--level, "event");
            xml.etag(--level, "midistate");
            }
      }

//---------------------------------------------------------
//   SynthI::read
//---------------------------------------------------------

void SynthI::read(Xml& xml, XmlReadStatistics*)
      {
      int port = -1;
      int oflags = 1;
      const bool preVer4 = xml.isVersionLessThan(4, 0);

      for (;;) {
            Xml::Token token = xml.parse();
            const QString tag = xml.s1();
            switch (token) {
                  case Xml::Error:
                  case Xml::End:
                        goto synth_read_end;
                  case Xml::TagStart:
                        // Is it a tag that the configuration structure recognizes?
                        if(!_initConfig.readProperties(xml, token))
                        {
                          // The configuration structure doesn't recognize it. Does the synth?
                          if (tag == "openFlags")
                            oflags = xml.parseInt();
                          else if (tag == "port")
                            port  = xml.parseInt();
                          else if (tag == "midistate")
                            readMidiState(xml);
                          // Added in song file version 4.
                          else if(tag == "AudioTrack")
                            AudioTrack::read(xml);
                          // Obsolete. Keep for compatibility.
                          else if (!preVer4 || AudioTrack::readProperties(xml, tag))
                            xml.unknown("softSynth");
                        }
                        break;
                  case Xml::TagEnd:
                        if (tag == "SynthI") {
                              fixOldColorScheme();

                              // NOTICE: This is a hack to quietly change songs to use the new 'fluid_synth' name
                              //          instead of 'fluidsynth'. Recent linker changes required the name change
                              //          in fluidsynth's cmakelists. Nov 8, 2011 By Tim.
                              if(_initConfig._file == QString("fluidsynth") &&
                                 (_initConfig._pluginType == MusEPlugin::PluginTypeMESS) &&
                                 (_initConfig._pluginLabel.isEmpty() ||
                                  _initConfig._pluginLabel == QString("FluidSynth")) )
                                _initConfig._file = QString("fluid_synth");

                              // For legacy song files earlier than version 4, which only stored the base name.
                              // Here we check both the song file version (in case first time load) and
                              //  the initial configuration (file was re-saved but plugin was missing).
                              const bool useFileBaseName =
                                preVer4 || (_initConfig._fileVerMaj >= 0 && _initConfig._fileVerMaj < 4);

                              Synth* s = MusEGlobal::synthis.find(
                                _initConfig._pluginType,
                                _initConfig._file,
                                _initConfig._uri,
                                _initConfig._pluginLabel,
                                useFileBaseName);

                              // Was a synth found?
                              if(s)
                              {
                                // Synth was found. There is no need for these anymore. Reset them.
                                _initConfig._fileVerMaj = _initConfig._fileVerMin = -1;
                              }
                              else
                              {
                                // Synth was not found. Remember the file version for later if re-saving,
                                //  if the version has not been stored yet.
                                if(_initConfig._fileVerMaj < 0 && _initConfig._fileVerMin < 0)
                                {
                                  _initConfig._fileVerMaj = xml.majorVersion();
                                  _initConfig._fileVerMin = xml.minorVersion();
                                }
                                // Add it to the missing plugins list.
                                MusEGlobal::missingPlugins.add(
                                  _initConfig._pluginType, _initConfig._file, _initConfig._uri, _initConfig._pluginLabel, true);
                              }

                              CtrlListList *cll = AudioTrack::controller();

                              // Persistent storage: If synth is not found allow the track to load.
                              // It's OK if s is NULL. initInstance needs to do a few things.
                              if(initInstance(s, _initConfig._name))
                              {
                                //---------------------------------------------------------
                                // The SIF could not be created or the synth could not
                                //  be found or created. Use the persistent information
                                //  to create and/or initialize controllers.
                                //---------------------------------------------------------
                                unsigned long int ctlnum;
                                for(ciPluginControlList ipcl = _initConfig._initParams.cbegin();
                                    ipcl != _initConfig._initParams.cend(); ++ipcl)
                                {
                                  const PluginControlConfig &cc = ipcl->second;
                                  iCtrlList icl;

                                  // Ignore controllers with IDs less than zero.
                                  // They can't be added to controller lists.
                                  if(cc._ctlnum < 0)
                                    continue;
                                  ctlnum = cc._ctlnum;

                                  CtrlList *cl;
                                  const unsigned long int ctlid = genACnum(MusECore::MAX_PLUGINS, ctlnum);
                                  icl = cll->find(ctlid);
                                  if(icl == cll->end())
                                  {
                                    cl = new MusECore::CtrlList();
                                    cl->setId(ctlid);
                                    cll->add(cl);
                                  }
                                  else
                                  {
                                    cl = icl->second;
                                  }
                                }
                                // We are not done with the initial parameters list
                                //  or custom config. Do not clear them so that if the
                                //  project is saved, they are used as persistent values.
                              }
                              else
                              {
                                // The SIF was created successfully.
                                // By now the automation controllers will have been
                                //  created and initialized with values obtained
                                //  from the SIF.

                                // Options for configuration.
                                PluginIBase::ConfigureOptions_t opts = PluginIBase::ConfigAll;
                                configure(opts);

                                // Done with the initial parameters list and custom config. Clear them.
                                // TODO: Decide: Maybe keep them around for a
                                //        'reset to previously loaded values' (revert) command?
                                _initConfig._initParams.clear();
                                _initConfig._accumulatedCustomParams.clear();
                              }

                              //---------------------------------------------------------
                              // If any automation controllers were included in the XML,
                              //  convert controller IDs and transfer to given list.
                              //---------------------------------------------------------
                              CtrlListList &conf_cll = _initConfig._ctrlListList;
                              for(ciCtrlList icl = conf_cll.cbegin(); icl != conf_cll.cend(); )
                              {
                                CtrlList *cl = icl->second;
                                int id = cl->id();

                                // If this is song file version 4 or greater, only synth controllers
                                //  should be appearing here and they should have no upper ID bits.
                                // If less than version 4, ALL controllers will appear here,
                                //  including track, rack plugins, and synth controllers, and
                                //  there will be upper ID bits already. Pass them on verbosely.
                                if(!preVer4)
                                {
                                  // Strip away any upper bits, just in case (shouldn't be there).
                                  const int m = id & AC_PLUGIN_CTL_ID_MASK;
                                  // Generate the new id.
                                  id = genACnum(MusECore::MAX_PLUGINS, m);
                                  cl->setId(id);
                                }

                                const int startId = genACnum(MusECore::MAX_PLUGINS, 0);
                                const int   endId = genACnum(MusECore::MAX_PLUGINS + 1, 0);
                                // If it's a synth controller, we can go ahead and set it up now.
                                // (Yes, this may be redundant if the synth was found since it
                                //   sets up already. But very old files may have set the controller's
                                //   current value to zero, so we need to grab it from the plugin info.)
                                // Meanwhile, rack plugin controllers (they will appear here in
                                //  song file version < 4) require us to handle it later
                                //  in (or after) mapRackPluginsToControllers() for example.
                                if(id >= startId && id < endId)
                                  setupController(cl);

                                // This takes ownership of the controller and will either
                                //  add/transfer it to the controller list, or delete it.
                                if(!addControllerFromXml(cl))
                                  fprintf(stderr, "SynthI::read: Error: Could not add controller #%d!\n", id);

                                // Done with the item. Erase it. Iterator will point to the next item.
                                icl = conf_cll.erase(icl);
                              }
                              // All of the items should be erased by now.

                              //---------------------------------------------------------
                              // If any midi controller mappings were included with in XML,
                              //  convert controller IDs and transfer to given list.
                              //---------------------------------------------------------
                              MidiAudioCtrlMap *macm = MusEGlobal::song->midiAssignments();
                              MidiAudioCtrlMap &conf_macm = _initConfig._midiAudioCtrlMap;
                              if(macm)
                              {
                                for(iMidiAudioCtrlMap imacm = conf_macm.begin(); imacm != conf_macm.end(); )
                                {
                                  MidiAudioCtrlStruct &macs = imacm->second;
                                  int id = macs.id();
                                  MidiAudioCtrlStruct::IdType idtype = macs.idType();

                                  // If this is song file version 4 or greater, only synth assignments should
                                  //  be appearing here and controller numbers should have no upper ID bits.
                                  // If less than version 4, ALL assignments will appear here,
                                  //  including track, rack plugins, and synth, and controller numbers will
                                  //  have upper ID bits already. Pass them on verbosely.
                                  if(idtype == MidiAudioCtrlStruct::AudioControl && !preVer4)
                                  {
                                    // Strip away any upper bits.
                                    const int m = id & AC_PLUGIN_CTL_ID_MASK;
                                    // Generate the new id.
                                    id = genACnum(MusECore::MAX_PLUGINS, m);
                                    macs.setId(id);
                                  }

                                  macs.setTrack(this);
                                  macm->add_ctrl_struct(imacm->first, macs);
                                  // Done with the item. Erase it. Iterator will point to the next item.
                                  imacm = conf_macm.erase(imacm);
                                }
                                // All of the items should be erased by now.
                              }
                              else
                              {
                                // Mappings were not transferred. Clear the list.
                                conf_macm.clear();
                              }

                              setOpenFlags(oflags);

                              MusEGlobal::song->insertTrack0(this, -1);

                              if (port != -1 && port < MusECore::MIDI_PORTS)
                                    MusEGlobal::midiPorts[port].setMidiDevice(this);

                              // Initializing OSC without actually showing the gui doesn't work,
                              //  at least for dssi-vst plugins - without showing the gui they
                              //  exit after ten seconds.
                              //initGui();

                              mapRackPluginsToControllers();

                              // Now that the track has been added to the lists in insertTrack2(),
                              //  the plugin guis can find the track and its rack effects plugins.
                              // Start the generic and native guis if required.
                              if(isShowGuiPending())
                                showGui(true);
                              if(isShowNativeGuiPending())
                                showNativeGui(true);
                              showPendingPluginGuis();
                              // This is mainly for MESS synths since unlike other plugins they
                              //  create their UIs when the synth is created (here via read for example),
                              //  so their titles can't be correct until insertTrack0 above. So do it now.
                              updateNativeGuiWindowTitle();

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
              const int hb = mp->hbank & 0xff;
              const int lb = mp->lbank & 0xff;
              const int pr = mp->prog & 0xff;
              const int id = (hb << 16) | (lb << 8) | pr;
              const bool vhb = hb != 0xff;
              const bool vlb = lb != 0xff;
              const bool vpr = pr != 0xff;
              QString astr;
              if(vhb || vlb || vpr) {
                if(vhb)
                  astr += QString::number(hb + 1) + QString(":");
                if(vlb)
                  astr += QString::number(lb + 1) + QString(":");
                else if(vhb)
                  astr += QString("--:");
                if(vpr)
                  astr += QString::number(pr + 1);
                else if(vhb && vlb)
                  astr += QString("--");
                astr += QString(" ");
              }
              astr += QString(mp->name);
              MusEGui::PopupMenu* m;
              if(lbank_menu)
                m = lbank_menu;
              else if(hbank_menu)
                m = hbank_menu;
              else
                m = menu;
              QAction *act = m->addAction(astr);
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
  float worst_lat = _sif ? _sif->latency() : 0.0f;
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

  if(!off() && (passthru || input))
  {
    // We want the AudioTrack in routes, not the MidiDevice in routes.
    RouteList* rl = AudioTrack::inRoutes();
    for (iRoute ir = rl->begin(); ir != rl->end(); ++ir)
    {
      if(ir->type != Route::TRACK_ROUTE || !ir->track || ir->track->isMidiTrack())
        continue;
      Track* track = ir->track;
      //if(!off() && !track->off() && (passthru || input))
      if(!track->off())
        track->setCorrectionLatencyInfo(false, finalWorstLatency, branch_lat);
    }

    if(writeEnable())
    {
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
          //if(!off() && !track->off() && (writeEnable()) && (passthru || input))
          if(!track->off())
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
                  //if(!off() && !track->off() && (writeEnable()) && (passthru || input))
                  if(!track->off())
                    track->setCorrectionLatencyInfo(false, finalWorstLatency, branch_lat);
                }
              break;

              default:
              break;
          }            
        }

    #endif

      }
    }

    // Special for the built-in metronome.
    //if(!off() && !MusECore::metronome->off() && (passthru || input) && sendMetronome())
    if(!MusECore::metronome->off() && sendMetronome())
    {
      MusECore::metronome->setCorrectionLatencyInfo(false, finalWorstLatency, branch_lat);
    }

    // Special for the transport source.
    //if(!off() && /*!_transportSource.off() &&*/ usesTransportSource() && (passthru || input))
    if(/*!_transportSource.off() &&*/ usesTransportSource())
    {
      _transportSource.setCorrectionLatencyInfo(
        false, finalWorstLatency, branch_lat, MusEGlobal::config.commonProjectLatency);
    }
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
    //        name().toLocal8Bit().constData(), finalWorstLatency, branch_lat, corr, _latencyInfo._sourceCorrectionValue);
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
  if((writeEnable()) && port >= 0 && port < MusECore::MIDI_PORTS)
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
  if((writeEnable()) && port >= 0 && port < MusECore::MIDI_PORTS)
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

  if(!off() && (passthru || input))
  {
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

            //if(!off() && !track->off() && (passthru || input))
            if(!track->off())
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

    if(writeEnable())
    {
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

          //if(!off() && !track->off() && (writeEnable()) && (passthru || input))
          if(!track->off())
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
                    
                  //if(!off() && !track->off() && (writeEnable()) && (passthru || input))
                  if(!track->off())
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

    #endif

      }
    }

    // Special for the built-in metronome.
    //if(!off() && !MusECore::metronome->off() && (passthru || input) && sendMetronome())
    if(!MusECore::metronome->off() && sendMetronome())
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

    // Special for the transport source.
    //if(!off() && /*!_transportSource.off() &&*/ usesTransportSource() && (passthru || input))
    if(/*!_transportSource.off() &&*/ usesTransportSource())
    {
      const TrackLatencyInfo& li = _transportSource.getDominanceInfo(false);
          
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
      
  if(!off() && (passthru || input))
  {
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

            //if(!off() && !track->off() && (passthru || input))
            if(!track->off())
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

    if(writeEnable())
    {
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

          //if(!off() && !track->off() && (writeEnable()) && (passthru || input))
          if(!track->off())
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
                    
                  //if(!off() && !track->off() && (writeEnable()) && (passthru || input))
                  if(!track->off())
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
    }

    // Special for the built-in metronome.
    //if(!off() && !MusECore::metronome->off() && (passthru || input) && sendMetronome())
    if(!MusECore::metronome->off() && sendMetronome())
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

    // Special for the transport source.
    //if(!off() && /*!_transportSource.off() &&*/ usesTransportSource() && (passthru || input))
    if(/*!_transportSource.off() &&*/ usesTransportSource())
    {
      const TrackLatencyInfo& li = _transportSource.getDominanceLatencyInfo(false);
          
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

  if(passthru || input)
  {
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

        // Default to zero.
        // TODO: FIXME: Where to store? We have no route to store it in.
        //ir->audioLatencyOut = 0.0f;
        //li._latencyOutMidiTrack = 0.0f;

        if(!off() && !track->off() && (writeEnable()))
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

                // Default to zero.
                ir->audioLatencyOut = 0.0f;

                if(!off() && !track->off() && (writeEnable()))
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
            break;

            default:
            break;
        }            
      }

  #endif

    }

    // Special for the built-in metronome.
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
        li._latencyOutMetronome = route_worst_latency - li._outputLatency;
        // Should not happen, but just in case.
        if((long int)li._latencyOutMetronome < 0)
          li._latencyOutMetronome = 0.0f;
      }
    }

    // Special for the transport source.
    // Default to zero.
    _transportSource.setTransportLatencyOut(0.0f);
    if(!off() && /*!_transportSource.off() &&*/ usesTransportSource())
    {
      TrackLatencyInfo& li = _transportSource.getLatencyInfo(false);

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
        _transportSource.setTransportLatencyOut(route_worst_latency - li._outputLatency);
        // Should not happen, but just in case.
        if((long int)_transportSource.transportLatencyOut() < 0)
          _transportSource.setTransportLatencyOut(0.0f);
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
  if(capture/*Tim*/ && ((/*capture ?*/ readEnable() /*: writeEnable()*/)) && port >= 0 && port < MusECore::MIDI_PORTS)
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
  if(capture/*Tim*/ && ((/*capture ?*/ readEnable() /*: writeEnable()*/)) && port >= 0 && port < MusECore::MIDI_PORTS)
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

      if(!off() && (passthru || input))
      {
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

                //if(!off() && !track->off() && (passthru || input))
                if(!track->off())
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
        if(!capture/*Tim*/ && port >= 0 && port < MusECore::MIDI_PORTS)
        {
          if(((/*capture ? readEnable() :*/ writeEnable())))
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

                //if(!off() && ((/*capture ? readEnable() :*/ writeEnable())) && !track->off() && (passthru || input))
                if(!track->off())
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
                          
                        //if(!off() && ((/*capture ? readEnable() :*/ writeEnable())) && !track->off() && (passthru || input))
                        if(!track->off())
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

    #endif          

            }
          }
          
          // Special for the built-in metronome.
          if(!capture)
          {
            MusECore::MetronomeSettings* metro_settings = 
              MusEGlobal::metroUseSongSettings ? &MusEGlobal::metroSongSettings : &MusEGlobal::metroGlobalSettings;

            //if(sendMetronome())
            if(metro_settings->midiClickFlag && metro_settings->clickPort == port)
            {
              //if(!off() && ((capture ? readEnable() : writeEnable())) && !MusECore::metronome->off() && (passthru || input))
              if(((capture ? readEnable() : writeEnable())) && !MusECore::metronome->off())
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
      }
      
      // Set the correction of all connected input branches,
      //  but ONLY if the track is not off.
      if(!off() && ((capture ? readEnable() : writeEnable())))
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
      if(!input && !off() && ((capture ? readEnable() : writeEnable())))
      {
        worst_self_latency = getWorstSelfLatencyAudio();
        const float worst_midi = getWorstSelfLatencyMidi(false /*playback*/);
        if(worst_midi > worst_self_latency)
          worst_self_latency = worst_midi;
      }
      
      if(!off() && (passthru || input))
      {
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

                //if(!off() && !track->off() && (passthru || input))
                if(!track->off())
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
        if(!capture/*Tim*/ && port >= 0 && port < MusECore::MIDI_PORTS)
        {
  //         bool used_chans[MusECore::MUSE_MIDI_CHANNELS];
  //         for(int i = 0; i < MusECore::MUSE_MIDI_CHANNELS; ++i)
  //           used_chans[i] = false;
  //         bool all_chans = false;

          
          if((/*capture ? readEnable() :*/ writeEnable()))
          {
            
    #ifdef _USE_MIDI_TRACK_SINGLE_OUT_PORT_CHAN_
            const MidiTrackList& tl = *MusEGlobal::song->midis();
            const MidiTrackList::size_type tl_sz = tl.size();
            for(MidiTrackList::size_type it = 0; it < tl_sz; ++it)
            {
              MidiTrack* track = static_cast<MidiTrack*>(tl[it]);
              if(track->outPort() != port)
                continue;

              //if(!off() && ((/*capture ? readEnable() :*/ writeEnable())) && !track->off() && (passthru || input))
              if(!track->off())
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
                        
                      //if(!off() && ((/*capture ? readEnable() :*/ writeEnable())) && !track->off() && (passthru || input))
                      if(!track->off())
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
          if(!capture)
          {
            MusECore::MetronomeSettings* metro_settings = 
              MusEGlobal::metroUseSongSettings ? &MusEGlobal::metroSongSettings : &MusEGlobal::metroGlobalSettings;

            //if(sendMetronome())
            if(metro_settings->midiClickFlag && metro_settings->clickPort == port)
            {
              //if(!off() && ((capture ? readEnable() : writeEnable())) && !MusECore::metronome->off() && (passthru || input))
              if(((capture ? readEnable() : writeEnable())) && !MusECore::metronome->off())
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
      }
      
      // Set the correction of all connected input branches,
      //  but ONLY if the track is not off.
      if(!off() && ((capture ? readEnable() : writeEnable())))
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
  if(!capture/*Tim*/ && !input && !off() && (writeEnable()))
  {
    worst_self_latency = getWorstSelfLatencyAudio();
    const float worst_midi = getWorstSelfLatencyMidi(false /*playback*/);
    if(worst_midi > worst_self_latency)
      worst_self_latency = worst_midi;
  }
      
  // The _trackLatency should already be calculated in the dominance scan.
  const float branch_lat = callerBranchLatency + worst_self_latency;

  if(!off() && (passthru || input))
  {
    // We want the AudioTrack in routes, not the MidiDevice in routes.
    RouteList* rl = AudioTrack::inRoutes();
    for (ciRoute ir = rl->cbegin(); ir != rl->cend(); ++ir)
    {
      if(ir->type != Route::TRACK_ROUTE || !ir->track || ir->track->isMidiTrack())
        continue;
      Track* track = ir->track;
      //if(!off() && !track->off() && (passthru || input))
      if(!track->off())
        track->setCorrectionLatencyInfo(false, finalWorstLatency, branch_lat);
    }

    const int port = midiPort();
    if(!capture/*Tim*/ && port >= 0 && port < MusECore::MIDI_PORTS)
    {
      if((writeEnable()))
      {
    #ifdef _USE_MIDI_TRACK_SINGLE_OUT_PORT_CHAN_
        const MidiTrackList& tl = *MusEGlobal::song->midis();
        const MidiTrackList::size_type tl_sz = tl.size();
        for(MidiTrackList::size_type it = 0; it < tl_sz; ++it)
        {
          MidiTrack* track = static_cast<MidiTrack*>(tl[it]);
          if(track->outPort() != port)
            continue;
          //if(!off() && (writeEnable()) && !track->off() && (passthru || input))
          if(!track->off())
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
                        
                      //if(!off() && (writeEnable()) && !track->off() && (passthru || input))
                      if(!track->off())
                        track->setCorrectionLatencyInfo(false, finalWorstLatency, branch_lat);
                    }
                  break;
                  
                  default:
                  break;
              }
        }

    #endif

      }
    }
    

    // Special for the built-in metronome.
    if(!capture)
    {
      MusECore::MetronomeSettings* metro_settings = 
        MusEGlobal::metroUseSongSettings ? &MusEGlobal::metroSongSettings : &MusEGlobal::metroGlobalSettings;

      //if(sendMetronome())
      if(metro_settings->midiClickFlag && metro_settings->clickPort == port)
      {
        //if(!off() && (writeEnable()) && !MusECore::metronome->off() && (passthru || input))
        if((writeEnable()) && !MusECore::metronome->off())
          MusECore::metronome->setCorrectionLatencyInfoMidi(capture, finalWorstLatency, branch_lat);
      }
    }
  }

  // Set the correction of all connected input branches,
  //  but ONLY if the track is not off.
  // Capture devices cannot be corrected. Only playback devices. Tim.
  if(!off() && (writeEnable()) && !capture/*Tim*/)
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

  if(passthru || input)
  {
    // Now that we know the worst-case latency of the connected branches,
    //  adjust each of the conveniently stored temporary latency values
    //  in the routes according to whether they can dominate...
    RouteList* rl = AudioTrack::inRoutes();
    for (iRoute ir = rl->begin(); ir != rl->end(); ++ir)
    {
      if(ir->type != Route::TRACK_ROUTE || !ir->track || ir->track->isMidiTrack())
        continue;

      Track* track = ir->track;

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

    const int port = midiPort();

    if(!capture/*Tim*/ && port >= 0 && port < MusECore::MIDI_PORTS)
    {
  #ifdef _USE_MIDI_TRACK_SINGLE_OUT_PORT_CHAN_
      const MidiTrackList& tl = *MusEGlobal::song->midis();
      const MidiTrackList::size_type tl_sz = tl.size();
      for(MidiTrackList::size_type it = 0; it < tl_sz; ++it)
      {
        MidiTrack* track = static_cast<MidiTrack*>(tl[it]);
        if(track->outPort() != port)
          continue;

        // TODO: FIXME: Where to store? We have no route to store it in.
        // Default to zero.
        //ir->audioLatencyOut = 0.0f;

        if(!off() && ((/*capture ? readEnable() :*/ writeEnable())) && !track->off())
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

              // Default to zero.
              ir->audioLatencyOut = 0.0f;

              if(!off() && ((/*capture ? readEnable() :*/ writeEnable())) && !track->off())
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
          break;

          default:
          break;
        }
      }

  #endif

      // Special for the built-in metronome.
      //if(!capture)
      //{
        // TODO: FIXME: Where to store? We have no route to store it in.
        // Default to zero.
        //ir->audioLatencyOut = 0.0f;

        // Special for the built-in metronome.
        // Default to zero.
        _latencyInfo._latencyOutMetronome = 0.0f;

        if((writeEnable()) && !MusECore::metronome->off() &&  // sendMetronome() &&
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
      //}

      // Special for the transport source.
      // Default to zero.
      _transportSource.setTransportLatencyOut(0.0f);
      if(!off() && /*!_transportSource.off() &&*/ usesTransportSource())
      {
        TrackLatencyInfo& li = _transportSource.getLatencyInfo(false);

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
          _transportSource.setTransportLatencyOut(route_worst_latency - li._outputLatency);
          // Should not happen, but just in case.
          if((long int)_transportSource.transportLatencyOut() < 0)
            _transportSource.setTransportLatencyOut(0.0f);
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

/*inline*/ unsigned long SynthI::latencyCompWriteOffsetMidi(bool capture) const
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
    //        name().toLocal8Bit().constData(), capture, worstCase, tli->_outputLatency, tli->_compensatorWriteOffset);
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
  //        name().toLocal8Bit().constData(), capture, worstCase, tli->_outputLatency, tli->_canDominateOutputLatency, tli->_compensatorWriteOffset);
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
      if(!off())
      {
        for (int k = 0; k < ports; ++k)
              memset(buffer[k], 0, n * sizeof(float));
      }

      if(!_sif)
      {
        // The synth doesn't exist. Flush ring buffers and event lists.
        eventBuffers(MidiDevice::PlaybackBuffer)->clearRead();
        eventBuffers(MidiDevice::UserBuffer)->clearRead();
        _outPlaybackEvents.clear();
        _outUserEvents.clear();
        // Reset the flag.
        setStopFlag(false);

        return false;
      }

      int p = midiPort();
      MidiPort* mp = (p != -1) ? &MusEGlobal::midiPorts[p] : 0;

      _sif->getData(mp, pos, ports, n, buffer);

      return true;
      }

bool MessSynthIF::getData(MidiPort* /*mp*/, unsigned pos, int ports, unsigned n, float** buffer)
{
      const unsigned int syncFrame = MusEGlobal::audio->curSyncFrame();
      unsigned int curPos = 0;
      unsigned int frame = 0;

      // Get the state of the stop flag.
      const bool do_stop = synti->stopFlag();
      // Get whether playback and user midi events can be written to this midi device.
      const bool we = synti->writeEnable();

      MidiPlayEvent buf_ev;

      // If stopping or not 'running' just purge ALL playback FIFO and container events.
      // But do not clear the user ones. We need to hold on to them until active,
      //  they may contain crucial events like loading a soundfont from a song file.
      if(do_stop || !_curActiveState || !we)
      {
        // Transfer the user lock-free buffer events to the user sorted multi-set.
        // To avoid too many events building up in the buffer while inactive, use the exclusive add.
        const unsigned int usr_buf_sz = synti->eventBuffers(MidiDevice::UserBuffer)->getSize();
        for(unsigned int i = 0; i < usr_buf_sz; ++i)
        {
          if(synti->eventBuffers(MidiDevice::UserBuffer)->get(buf_ev))
            synti->_outUserEvents.addExclusive(buf_ev);
        }

        synti->eventBuffers(MidiDevice::PlaybackBuffer)->clearRead();
        synti->_outPlaybackEvents.clear();
        // Reset the flag.
        synti->setStopFlag(false);
      }
      else
      {
        // Transfer the user lock-free buffer events to the user sorted multi-set.
        const unsigned int usr_buf_sz = synti->eventBuffers(MidiDevice::UserBuffer)->getSize();
        for(unsigned int i = 0; i < usr_buf_sz; ++i)
        {
          if(synti->eventBuffers(MidiDevice::UserBuffer)->get(buf_ev))
            synti->_outUserEvents.insert(buf_ev);
        }

        // Transfer the playback lock-free buffer events to the playback sorted multi-set.
        const unsigned int pb_buf_sz = synti->eventBuffers(MidiDevice::PlaybackBuffer)->getSize();
        for(unsigned int i = 0; i < pb_buf_sz; ++i)
        {
          if(synti->eventBuffers(MidiDevice::PlaybackBuffer)->get(buf_ev))
            synti->_outPlaybackEvents.insert(buf_ev);
        }
      }

      // Don't bother if not 'running'.
      //if(_curActiveState && we)
      // NOTE: Unlike the other synths, we must allow for some MESS events to be processed even when inactive.
      //       It is good that we can do that with MESS! The midi processing is SEPARATE from the audio processing.
      //       Other plugin architectures combine it all into the run function, and it must be run to make any change.
      if(we)
      {
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
            // Don't bother if not 'running'.
            if(_curActiveState)
            {

              if (!_mess)
                fprintf(stderr, "MessSynthIF::getData() should not happen - no _mess\n");
              else
                _mess->process(pos, buffer, ports, curPos, frame - curPos);
            }
            curPos = frame;
          }

          // If putEvent fails, although we would like to not miss events by keeping them
          //  until next cycle and trying again, that can lead to a large backup of events
          //  over a long time. So we'll just... miss them.
          //putEvent(ev);
          //synti->putEvent(ev);
          processEvent(ev);

          // Done with buffer event. Remove it.
          // C++11.
          if(using_pb)
            impe_pb = synti->_outPlaybackEvents.erase(impe_pb);
          else
            impe_us = synti->_outUserEvents.erase(impe_us);
        }
      }

      // Don't bother if not 'running'.
      if(_curActiveState && curPos < n)
      {
        if (!_mess)
          fprintf(stderr, "MessSynthIF::getData() should not happen - no _mess\n");
        else
          _mess->process(pos, buffer, ports, curPos, n - curPos);
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
           fprintf(stderr, "MidiOut: MESS: <%s>: ", synti->name().toLocal8Bit().constData());
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
            synti->currentProg(chn, nullptr, &lb, &hb);
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
              synti->currentProg(chn, &pr, &lb, nullptr);
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
              synti->currentProg(chn, &pr, nullptr, &hb);
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
