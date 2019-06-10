//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: ticksynth.cpp,v 1.8.2.7 2009/12/20 05:00:35 terminator356 Exp $
//  (C) Copyright 2004 Werner Schweer (ws@seh.de)
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

#include "audio.h"
#include "ticksynth.h"
#include "default_click.h"
#include "midi.h"
#include "popupmenu.h"
#include "gconfig.h"
#include "wave.h"
#include "operations.h"
#include "metronome_class.h"
#include "globals.h"
#include "song.h"
#include "track.h"

// If sysex support is ever added, make sure this number is unique among all the
//  MESS synths (including ticksynth) and DSSI, VST, LV2 and other host synths.
// 127 is reserved for special MusE system messages.
//#define METRONOME_UNIQUE_ID      7

// // Undefine if and when multiple output routes are added to midi tracks.
// #define _USE_MIDI_TRACK_SINGLE_OUT_PORT_CHAN_

//#define METRONOME_DEBUG

// For debugging output: Uncomment the fprintf section.
#define DEBUG_TICKSYNTH(dev, format, args...)  //fprintf(dev, format, ##args);

namespace MusECore {

MetronomeSynthI* metronome = 0;

class MetronomeSynth;
static MetronomeSynth* metronomeSynth = 0;

//---------------------------------------------------------
//   MetronomeSynth
//---------------------------------------------------------

class MetronomeSynth : public Synth {
   public:
      MetronomeSynth(const QFileInfo& fi) : Synth(fi, QString("Metronome"), QString("Metronome"), QString(), QString()) {}
      virtual ~MetronomeSynth() {}
      virtual Type synthType() const { return METRO_SYNTH; }
      virtual void incInstances(int) {}
      virtual void* instantiate();
      
      virtual SynthIF* createSIF(SynthI*);
      };

//---------------------------------------------------------
//   instantiate
//---------------------------------------------------------

void* MetronomeSynth::instantiate()
      {
      return 0;
      }

//---------------------------------------------------------
//   MetronomeSynthIF
//---------------------------------------------------------

class MetronomeSynthIF : public SynthIF
      {
      const float* data;
      int pos;
      int len;
      float volume;
      void process(float** buffer, int offset, int n);
      void initSamples();

      float *measSamples;
      int    measLen;
      float *beatSamples;
      int    beatLen;
      float *accent1Samples;
      int    accent1Len;
      float *accent2Samples;
      int    accent2Len;

      bool processEvent(const MidiPlayEvent& ev);
      
   public:
      MetronomeSynthIF(SynthI* s) : SynthIF(s) {
            data = 0;
            beatLen = 0;
            measLen = 0;
            accent1Len = 0;
            accent2Len = 0;
            initSamples();
            }
      virtual void guiHeartBeat()  {  }
      virtual bool guiVisible() const { return false; }
      virtual bool hasGui() const { return false; }
      virtual bool nativeGuiVisible() const { return false; }
      virtual void showNativeGui(bool) { }
      virtual bool hasNativeGui() const { return false; }
      
      virtual void getNativeGeometry(int*x, int*y, int*w, int*h) const { *x=0;*y=0;*w=0;*h=0; }
      virtual void setNativeGeometry(int, int, int, int) {}
      virtual bool getData(MidiPort*, unsigned pos, int ports, unsigned n, float** buffer);
      virtual MidiPlayEvent receiveEvent() { return MidiPlayEvent(); }
      virtual int eventsPending() const { return 0; }
      
      virtual int channels() const { return 1; }
      virtual int totalOutChannels() const { return 1; }
      virtual int totalInChannels() const { return 0; }
      virtual void deactivate3() {}
      virtual QString getPatchName(int, int, bool) const { return ""; }
      virtual void populatePatchPopup(MusEGui::PopupMenu*, int, bool) {}
      virtual void write(int, Xml&) const {}
      virtual double getParameter(unsigned long) const  { return 0.0; }
      virtual void setParameter(unsigned long, double) {}
      virtual int getControllerInfo(int, QString*, int*, int*, int*, int*) { return 0; }

      void initSamplesOperation(MusECore::PendingOperationList&);
       
      //-------------------------
      // Methods for PluginIBase:
      //-------------------------

      virtual bool addScheduledControlEvent(unsigned long /*i*/, double /*val*/, unsigned /*frame*/) { return true; }    // returns true if event cannot be delivered
      };

//---------------------------------------------------------
//   getData
//---------------------------------------------------------

bool MetronomeSynthIF::getData(MidiPort*, unsigned /*pos*/, int/*ports*/, unsigned n, float** buffer)
      {
      #ifdef METRONOME_DEBUG
      fprintf(stderr, "MusE: MetronomeSynthIF::getData\n");
      #endif

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
          fprintf(stderr, "MetronomeSynthIF::getData() evTime:%u < syncFrame:%u!! curPos=%d\n", 
                  evTime, syncFrame, curPos);
          frame = 0;
        }
        else
          frame = evTime - syncFrame;

        // Event is for future?
        if(frame >= n) 
        {
          DEBUG_TICKSYNTH(stderr, "MetronomeSynthIF::getData(): Event for future, breaking loop: frame:%u n:%d evTime:%u syncFrame:%u curPos:%d\n", 
                  frame, n, evTime, syncFrame, curPos);
          //continue;
          break;
        }

        if(frame > curPos)
        {
          process(buffer, curPos, frame - curPos);
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
        process(buffer, curPos, n - curPos);
      
      return true;
      }
      
//---------------------------------------------------------
//   initSamples
//---------------------------------------------------------

void MetronomeSynthIF::initSamples()
{
    if (beatLen)
      delete beatSamples;
    if (measLen)
      delete measSamples;
    if (accent1Len)
      delete accent1Samples;
    if (accent2Len)
      delete accent2Samples;
    beatLen = 0;
    measLen = 0;
    accent1Len = 0;
    accent2Len = 0;

    SndFile beat(MusEGlobal::museGlobalShare + "/metronome/" + MusEGlobal::config.beatSample);
    if (!beat.openRead(false)) {
      beatLen = beat.samples();
      beatSamples = new float[beatLen];
      beat.read(1, &beatSamples, beatLen);
    }

    SndFile meas(MusEGlobal::museGlobalShare  + "/metronome/" + MusEGlobal::config.measSample);
    if (!meas.openRead(false)) {
      measLen = meas.samples();
      measSamples = new float[measLen];
      meas.read(1, &measSamples, measLen);
    }

    SndFile accent1(MusEGlobal::museGlobalShare +  "/metronome/" + MusEGlobal::config.accent1Sample);
    if (!accent1.openRead(false)) {
      accent1Len = accent1.samples();
      accent1Samples = new float[accent1Len];
      accent1.read(1, &accent1Samples, accent1Len);
    }

    SndFile accent2(MusEGlobal::museGlobalShare +  "/metronome/" + MusEGlobal::config.accent2Sample);
    if (!accent2.openRead(false)) {
      accent2Len = accent2.samples();
      accent2Samples = new float[accent2Len];
      accent2.read(1, &accent2Samples, accent2Len);
    }

}

//---------------------------------------------------------
//   initSamplesOperation
//---------------------------------------------------------

void MetronomeSynthIF::initSamplesOperation(MusECore::PendingOperationList& operations)
{
  SndFile beat(MusEGlobal::museGlobalShare + "/metronome/" + MusEGlobal::config.beatSample);
  if (!beat.openRead(false)) {
    const sf_count_t newBeatLen = beat.samples();
    if(newBeatLen != 0)
    {
      float* newBeatSamples = new float[newBeatLen];
      beat.read(1, &newBeatSamples, newBeatLen);
      operations.add(PendingOperationItem(&beatSamples, newBeatSamples, 
                                          &beatLen, newBeatLen, 
                                          PendingOperationItem::ModifyAudioSamples));
    }
  }
  
  SndFile meas(MusEGlobal::museGlobalShare  + "/metronome/" + MusEGlobal::config.measSample);
  if (!meas.openRead(false)) {
    const sf_count_t newMeasLen = meas.samples();
    if(newMeasLen != 0)
    {
      float* newMeasSamples = new float[newMeasLen];
      meas.read(1, &newMeasSamples, newMeasLen);
      operations.add(PendingOperationItem(&measSamples, newMeasSamples, 
                                          &measLen, newMeasLen, 
                                          PendingOperationItem::ModifyAudioSamples));
    }
  }

  SndFile accent1(MusEGlobal::museGlobalShare +  "/metronome/" + MusEGlobal::config.accent1Sample);
  if (!accent1.openRead(false)) {
    const sf_count_t newAccent1Len = accent1.samples();
    if(newAccent1Len != 0)
    {
      float* newAccent1Samples = new float[newAccent1Len];
      accent1.read(1, &newAccent1Samples, newAccent1Len);
      operations.add(PendingOperationItem(&accent1Samples, newAccent1Samples, 
                                          &accent1Len, newAccent1Len, 
                                          PendingOperationItem::ModifyAudioSamples));
    }
  }

  SndFile accent2(MusEGlobal::museGlobalShare +  "/metronome/" + MusEGlobal::config.accent2Sample);
  if (!accent2.openRead(false)) {
    const sf_count_t newAccent2Len = accent2.samples();
    if(newAccent2Len != 0)
    {
      float* newAccent2Samples = new float[newAccent2Len];
      accent2.read(1, &newAccent2Samples, newAccent2Len);
      operations.add(PendingOperationItem(&accent2Samples, newAccent2Samples, 
                                          &accent2Len, newAccent2Len, 
                                          PendingOperationItem::ModifyAudioSamples));
    }
  }
}

//---------------------------------------------------------
//   putEvent
//---------------------------------------------------------

bool MetronomeSynthIF::processEvent(const MidiPlayEvent& ev)
{
    if(ev.type() != MusECore::ME_NOTEON)
      return false;

    MusECore::MetronomeSettings* metro_settings = 
      MusEGlobal::metroUseSongSettings ? &MusEGlobal::metroSongSettings : &MusEGlobal::metroGlobalSettings;

    if (ev.dataA() == MusECore::measureSound) {
        if (metro_settings->clickSamples == MetronomeSettings::origSamples) {
            data = defaultClickEmphasis;
            len  = defaultClickEmphasisLength;
        }
        else {
              data = measSamples;
              len  = measLen;
        }
        volume = metro_settings->measClickVolume;
    }
    else if (ev.dataA() == MusECore::beatSound) {
        if (metro_settings->clickSamples == MetronomeSettings::origSamples) {
            data = defaultClick;
            len  = defaultClickLength;
        } else {
            data = beatSamples;
            len  = beatLen;
        }
        volume = metro_settings->beatClickVolume;
    }
    else if (ev.dataA() == MusECore::accent1Sound) {
             data = accent1Samples;
             len  = accent1Len;
             volume = metro_settings->accent1ClickVolume;
             if (metro_settings->clickSamples == MetronomeSettings::origSamples) {
                 volume=0.0;
             }
    }
    else if (ev.dataA() == MusECore::accent2Sound) {
             data = accent2Samples;
             len  = accent2Len;
             volume = metro_settings->accent2ClickVolume;
             if (metro_settings->clickSamples == MetronomeSettings::origSamples) {
                 volume=0.0;
             }
    }


    pos = 0;
    return false;
}

//---------------------------------------------------------
//   createSIF
//---------------------------------------------------------

SynthIF* MetronomeSynth::createSIF(SynthI* s)
      {
      return new MetronomeSynthIF(s);
      }

//---------------------------------------------------------
//   process
//    synthesize n samples into buffer+offset
//---------------------------------------------------------

void MetronomeSynthIF::process(float** buffer, int offset, int n)
      {
      #ifdef METRONOME_DEBUG
      printf("MusE: MetronomeSynthIF::process data:%p offset:%d n:%d\n", data, offset, n);
      #endif

      if (data == 0)
        return;

      MusECore::MetronomeSettings* metro_settings = 
        MusEGlobal::metroUseSongSettings ? &MusEGlobal::metroSongSettings : &MusEGlobal::metroGlobalSettings;

      const float* s = data + pos;
      float* d       = *buffer + offset;
      int l          = std::min(n, len);

      for (int i = 0; i < l; ++i)
            *d++ += *s++ * metro_settings->audioClickVolume * volume;
      pos += l;
      len -= l;
      if (len <= 0)
            data = 0;
      }

//---------------------------------------------------------
//   MetronomeSynthI
//---------------------------------------------------------

void MetronomeSynthI::initSamplesOperation(PendingOperationList& operations)
{ 
  if(sif()) 
    dynamic_cast<MetronomeSynthIF*>(sif())->initSamplesOperation(operations);
}
   
//================================================
// BEGIN Latency correction/compensation routines.
//================================================

//---------------------------------------------------------
//   isLatencyInputTerminal
//---------------------------------------------------------

bool MetronomeSynthI::isLatencyInputTerminal()
{
  // Have we been here before during this scan?
  // Just return the cached value.
  if(_latencyInfo._isLatencyInputTerminalProcessed)
    return _latencyInfo._isLatencyInputTerminal;

  // Ultimately if the track is off there is no audio or midi processing, so it's a terminal.
  if(off() /*|| !MusEGlobal::song->click()*/)
  {
    _latencyInfo._isLatencyInputTerminal = true;
    _latencyInfo._isLatencyInputTerminalProcessed = true;
    return true;
  }
  
  MusECore::MetronomeSettings* metro_settings = 
    MusEGlobal::metroUseSongSettings ? &MusEGlobal::metroSongSettings : &MusEGlobal::metroGlobalSettings;

  if(metro_settings->audioClickFlag)
  {
    const ciAudioOutput tl_end = MusEGlobal::song->outputs()->cend();
    for(ciAudioOutput it = MusEGlobal::song->outputs()->cbegin(); it != tl_end; ++it)
    {
      const AudioOutput* ao = *it;
      if(ao->off())
        continue;
      if(ao->sendMetronome())
      {
        _latencyInfo._isLatencyInputTerminal = false;
        _latencyInfo._isLatencyInputTerminalProcessed = true;
        return false;
      }
    }
  }    

  if(metro_settings->midiClickFlag /*&& !precount_mute_metronome*/)
  {
      const int port = metro_settings->clickPort;
      if((openFlags() & 2 /*read*/) && port >= 0 && port < MusECore::MIDI_PORTS)
      {
        const MidiPort* mp = &MusEGlobal::midiPorts[port];
        const MidiDevice* md = mp->device();
        // TODO Make a new SynthI::isReadable and isWritable to replace this and MANY others in the app, and add to some places
        //       where it's not even used. Prevent sending to the synth, maybe finally get rid of the flushing in SynthI::preProcessAlways()
        if(md && ((md->openFlags() & 1 /*write*/) && (!md->isSynti() || !static_cast<const SynthI*>(md)->off())))
        {
          _latencyInfo._isLatencyInputTerminal = false;
          _latencyInfo._isLatencyInputTerminalProcessed = true;
          return false;
        }
      }
  }

  _latencyInfo._isLatencyInputTerminal = true;
  _latencyInfo._isLatencyInputTerminalProcessed = true;
  return true;
}

//---------------------------------------------------------
//   isLatencyOutputTerminal
//---------------------------------------------------------

bool MetronomeSynthI::isLatencyOutputTerminal()
{
  // Have we been here before during this scan?
  // Just return the cached value.
  if(_latencyInfo._isLatencyOutputTerminalProcessed)
    return _latencyInfo._isLatencyOutputTerminal;

  MusECore::MetronomeSettings* metro_settings = 
    MusEGlobal::metroUseSongSettings ? &MusEGlobal::metroSongSettings : &MusEGlobal::metroGlobalSettings;

  if(metro_settings->audioClickFlag)
  {
    const ciAudioOutput tl_end = MusEGlobal::song->outputs()->cend();
    for(ciAudioOutput it = MusEGlobal::song->outputs()->cbegin(); it != tl_end; ++it)
    {
      const AudioOutput* ao = *it;
      if(ao->off())
        continue;
      if(ao->sendMetronome())
      {
        _latencyInfo._isLatencyOutputTerminal = false;
        _latencyInfo._isLatencyOutputTerminalProcessed = true;
        return false;
      }
    }
  }    

  if(metro_settings->midiClickFlag /*&& !precount_mute_metronome*/)
  {
      const int port = metro_settings->clickPort;
      if((openFlags() & 2 /*read*/) && port >= 0 && port < MusECore::MIDI_PORTS)
      {
        const MidiPort* mp = &MusEGlobal::midiPorts[port];
        const MidiDevice* md = mp->device();
        // TODO Make a new SynthI::isReadable and isWritable to replace this and MANY others in the app, and add to some places
        //       where it's not even used. Prevent sending to the synth, maybe finally get rid of the flushing in SynthI::preProcessAlways()
        if(md && ((md->openFlags() & 1 /*write*/) && (!md->isSynti() || !static_cast<const SynthI*>(md)->off())))
        {
          _latencyInfo._isLatencyOutputTerminal = false;
          _latencyInfo._isLatencyOutputTerminalProcessed = true;
          return false;
        }
      }
  }
    
  _latencyInfo._isLatencyOutputTerminal = true;
  _latencyInfo._isLatencyOutputTerminalProcessed = true;
  return true;
}

bool MetronomeSynthI::isLatencyInputTerminalMidi(bool capture)
{
  TrackLatencyInfo* tli = capture ? &_captureLatencyInfo : &_playbackLatencyInfo;

  // Have we been here before during this scan?
  // Just return the cached value.
  if(tli->_isLatencyInputTerminalProcessed)
    return tli->_isLatencyInputTerminal;

  // Ultimately if the track is off there is no audio or midi processing, so it's a terminal.
  if(off() /*|| !MusEGlobal::song->click()*/)
  {
    tli->_isLatencyInputTerminal = true;
    tli->_isLatencyInputTerminalProcessed = true;
    return true;
  }
  
  MusECore::MetronomeSettings* metro_settings = 
    MusEGlobal::metroUseSongSettings ? &MusEGlobal::metroSongSettings : &MusEGlobal::metroGlobalSettings;

  if(metro_settings->audioClickFlag)
  {
    const ciAudioOutput tl_end = MusEGlobal::song->outputs()->cend();
    for(ciAudioOutput it = MusEGlobal::song->outputs()->cbegin(); it != tl_end; ++it)
    {
      const AudioOutput* ao = *it;
      if(ao->off())
        continue;
      if(ao->sendMetronome())
      {
        tli->_isLatencyInputTerminal = false;
        tli->_isLatencyInputTerminalProcessed = true;
        return false;
      }
    }
  }    

  if(metro_settings->midiClickFlag /*&& !precount_mute_metronome*/)
  {
      const int port = metro_settings->clickPort;
      if((openFlags() & 2 /*read*/) && port >= 0 && port < MusECore::MIDI_PORTS)
      {
        const MidiPort* mp = &MusEGlobal::midiPorts[port];
        const MidiDevice* md = mp->device();
        // TODO Make a new SynthI::isReadable and isWritable to replace this and MANY others in the app, and add to some places
        //       where it's not even used. Prevent sending to the synth, maybe finally get rid of the flushing in SynthI::preProcessAlways()
        if(md && ((md->openFlags() & 1 /*write*/) && (!md->isSynti() || !static_cast<const SynthI*>(md)->off())))
        {
          tli->_isLatencyInputTerminal = false;
          tli->_isLatencyInputTerminalProcessed = true;
          return false;
        }
      }
  }

  tli->_isLatencyInputTerminal = true;
  tli->_isLatencyInputTerminalProcessed = true;
  return true;
}

bool MetronomeSynthI::isLatencyOutputTerminalMidi(bool capture)
{
  TrackLatencyInfo* tli = capture ? &_captureLatencyInfo : &_playbackLatencyInfo;

  // Have we been here before during this scan?
  // Just return the cached value.
  if(tli->_isLatencyOutputTerminalProcessed)
    return tli->_isLatencyOutputTerminal;

  MusECore::MetronomeSettings* metro_settings = 
    MusEGlobal::metroUseSongSettings ? &MusEGlobal::metroSongSettings : &MusEGlobal::metroGlobalSettings;

  if(metro_settings->audioClickFlag)
  {
    const ciAudioOutput tl_end = MusEGlobal::song->outputs()->cend();
    for(ciAudioOutput it = MusEGlobal::song->outputs()->cbegin(); it != tl_end; ++it)
    {
      const AudioOutput* ao = *it;
      if(ao->off())
        continue;
      if(ao->sendMetronome())
      {
        tli->_isLatencyOutputTerminal = false;
        tli->_isLatencyOutputTerminalProcessed = true;
        return false;
      }
    }
  }    

  if(metro_settings->midiClickFlag /*&& !precount_mute_metronome*/)
  {
      const int port = metro_settings->clickPort;
      if((openFlags() & 2 /*read*/) && port >= 0 && port < MusECore::MIDI_PORTS)
      {
        const MidiPort* mp = &MusEGlobal::midiPorts[port];
        const MidiDevice* md = mp->device();
        // TODO Make a new SynthI::isReadable and isWritable to replace this and MANY others in the app, and add to some places
        //       where it's not even used. Prevent sending to the synth, maybe finally get rid of the flushing in SynthI::preProcessAlways()
        if(md && ((md->openFlags() & 1 /*write*/) && (!md->isSynti() || !static_cast<const SynthI*>(md)->off())))
        {
          tli->_isLatencyOutputTerminal = false;
          tli->_isLatencyOutputTerminalProcessed = true;
          return false;
        }
      }
  }

  tli->_isLatencyOutputTerminal = true;
  tli->_isLatencyOutputTerminalProcessed = true;
  return true;
}

//================================================
// END Latency correction/compensation routines.
//================================================


//---------------------------------------------------------
//   initMetronome
//---------------------------------------------------------

void initMetronome()
      {
      QFileInfo fi;
      metronomeSynth = new MetronomeSynth(fi);
      metronome = new MetronomeSynthI();
      
      QString name("metronome");
      metronome->initInstance(metronomeSynth, name);
      }

//---------------------------------------------------------
//   exitMetronome
//---------------------------------------------------------

void exitMetronome()
{
      if(metronome)
        delete metronome;
      metronome = 0;  
      
      if(metronomeSynth)
        delete metronomeSynth;
      metronomeSynth = 0;
}

} // namespace MusECore
