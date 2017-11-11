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

// If sysex support is ever added, make sure this number is unique among all the
//  MESS synths (including ticksynth) and DSSI, VST, LV2 and other host synths.
// 127 is reserved for special MusE system messages.
//#define METRONOME_UNIQUE_ID      7

//#define METRONOME_DEBUG

// For debugging output: Uncomment the fprintf section.
// REMOVE Tim. autoconnect. Changed. Enabled. Disable when done.
//#define DEBUG_TICKSYNTH(dev, format, args...)  //fprintf(dev, format, ##args);
#define DEBUG_TICKSYNTH(dev, format, args...)  fprintf(dev, format, ##args);

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

// REMOVE Tim. autoconnect. Added.
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
      virtual bool initGui()     { return true; }
      virtual void guiHeartBeat()  {  }
      virtual bool guiVisible() const { return false; }
      virtual void showGui(bool) {}
      virtual bool hasGui() const { return false; }
      virtual bool nativeGuiVisible() const { return false; }
      virtual void showNativeGui(bool) { }
      virtual bool hasNativeGui() const { return false; }
      
      virtual void getGeometry(int*x, int*y, int*w, int*h) const { *x=0;*y=0;*w=0;*h=0; }
      virtual void setGeometry(int, int, int, int) {}
      virtual void getNativeGeometry(int*x, int*y, int*w, int*h) const { *x=0;*y=0;*w=0;*h=0; }
      virtual void setNativeGeometry(int, int, int, int) {}
      virtual void preProcessAlways() { }
// REMOVE Tim. autoconnect. Changed.
//       virtual iMPEvent getData(MidiPort*, MPEventList*, iMPEvent, unsigned pos, int ports, unsigned n, float** buffer);
      virtual bool getData(MidiPort*, unsigned pos, int ports, unsigned n, float** buffer);
// REMOVE Tim. autoconnect. Removed.
//       virtual bool putEvent(const MidiPlayEvent& ev);
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

      //-------------------------
      // Methods for PluginIBase:
      //-------------------------

      virtual bool addScheduledControlEvent(unsigned long /*i*/, double /*val*/, unsigned /*frame*/) { return true; }    // returns true if event cannot be delivered
      };

//---------------------------------------------------------
//   getData
//---------------------------------------------------------

// REMOVE Tim. autoconnect. Changed.
// iMPEvent MetronomeSynthIF::getData(MidiPort*, MPEventList* el, iMPEvent i, unsigned /*pos*/, int/*ports*/, unsigned n, float** buffer)
//       {
//       // Added by Tim. p3.3.18
//       #ifdef METRONOME_DEBUG
//       printf("MusE: MetronomeSynthIF::getData\n");
//       #endif
// 
//       if (((MidiPlayEvent&)*i).dataA() == MusECore::reloadClickSounds) {
//           initMetronome();
//       }
// 
//       //set type to unsigned , due to compiler warning: comparison signed/unsigned
// // REMOVE Tim. autoconnect. Changed.
// //       unsigned int curPos      = pos;             //prevent compiler warning: comparison signed/unsigned
// //       unsigned int endPos      = pos + n;         //prevent compiler warning: comparison signed/unsigned
// //       unsigned int off         = pos;             //prevent compiler warning: comparison signed/unsigned
//       unsigned int curPos      = 0;
//       unsigned int endPos      = n;
// // REMOVE Tim. autoconnect. Changed.
// //       int frameOffset = MusEGlobal::audio->getFrameOffset();
//       unsigned int syncFrame = MusEGlobal::audio->curSyncFrame();
// 
//       for (; i != el->end(); ++i) {
// // REMOVE Tim. autoconnect. Changed.
// //             unsigned int frame = i->time() - frameOffset; //prevent compiler warning: comparison signed /unsigned
// //             if (frame >= endPos)
//             unsigned int frame = (i->time() < syncFrame) ? 0 : i->time() - syncFrame;
//             if (frame >= n)
//                   break;
//             if (frame > curPos) {
// //                   if (frame < pos)
// //                         printf("should not happen: missed event %d\n", pos -frame);
//                   if(i->time() < syncFrame)
//                         fprintf(stderr, "MetronomeSynthIF: should not happen: missed event %u\n", i->time());
//                   else
// //                         process(buffer, curPos-pos, frame - curPos);
//                         process(buffer, curPos, frame - curPos);
//                   curPos = frame;
//                   }
//             putEvent(*i);
//             }
// //       if (endPos - curPos)
// //             process(buffer, curPos - off, endPos - curPos);
//       if (endPos > curPos)
//             process(buffer, curPos, endPos - curPos);
//       return el->end();
//       }

// REMOVE Tim. autoconnect. Changed.
// iMPEvent MetronomeSynthIF::getData(MidiPort*, MPEventList* /*el*/, iMPEvent i, unsigned /*pos*/, int/*ports*/, unsigned n, float** buffer)
bool MetronomeSynthIF::getData(MidiPort*, unsigned /*pos*/, int/*ports*/, unsigned n, float** buffer)
      {
      #ifdef METRONOME_DEBUG
      fprintf(stderr, "MusE: MetronomeSynthIF::getData\n");
      #endif

// REMOVE Tim. autoconnect. Removed.
// FIXME Not realtime safe
//       if (((MidiPlayEvent&)*i).dataA() == MusECore::reloadClickSounds) {
//           initMetronome();
//       }

      const unsigned int syncFrame = MusEGlobal::audio->curSyncFrame();
      unsigned int curPos = 0;
      unsigned int frame = 0;

      // Get the state of the stop flag.
      const bool do_stop = synti->stopFlag();

      MidiPlayEvent buf_ev;
      
      //const unsigned int usr_buf_sz = synti->eventBuffers(MidiDevice::UserBuffer)->bufferCapacity();
      // Transfer the user lock-free buffer events to the user sorted multi-set.
      // False = don't use the size snapshot, but update it.
      const unsigned int usr_buf_sz = synti->eventBuffers(MidiDevice::UserBuffer)->getSize(false);
      for(unsigned int i = 0; i < usr_buf_sz; ++i)
      {
        //if(synti->eventBuffers(MidiDevice::UserBuffer)->get(buf_ev, i))
        if(synti->eventBuffers(MidiDevice::UserBuffer)->get(buf_ev))
          //synti->_outUserEvents.add(buf_ev);
          synti->_outUserEvents.insert(buf_ev);
      }
      
      // Transfer the playback lock-free buffer events to the playback sorted multi-set.
      //const unsigned int pb_buf_sz = synti->eventBuffers(MidiDevice::PlaybackBuffer)->bufferCapacity();
      const unsigned int pb_buf_sz = synti->eventBuffers(MidiDevice::PlaybackBuffer)->getSize(false);
      for(unsigned int i = 0; i < pb_buf_sz; ++i)
      {
        // Are we stopping? Just remove the item.
        if(do_stop)
          //synti->eventBuffers(PlaybackBuffer)->remove(i);
          synti->eventBuffers(MidiDevice::PlaybackBuffer)->remove();
        // Otherwise get the item.
        //else if(synti->eventBuffers(MidiDevice::PlaybackBuffer)->get(buf_ev, i))
        else if(synti->eventBuffers(MidiDevice::PlaybackBuffer)->get(buf_ev))
          //synti->_outPlaybackEvents.add(buf_ev);
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
//       else
//       //{
//         // For convenience, simply transfer all playback events into the other user list. 
//         //for(ciMPEvent impe = synti->_outPlaybackEvents.begin(); impe != synti->_outPlaybackEvents.end(); ++impe)
//         //  synti->_outUserEvents.add(*impe);
//         synti->_outUserEvents.insert(synti->_outPlaybackEvents.begin(), synti->_outPlaybackEvents.end());
//       //}
//       // Done with playback event list. Clear it.  
//       //synti->_outPlaybackEvents.clear();
        
      iMPEvent impe_pb = synti->_outPlaybackEvents.begin();
      iMPEvent impe_us = synti->_outUserEvents.begin();
      bool using_pb;
  
//       // This also takes an internal snapshot of the size for use later...
//       // False = don't use the size snapshot, but update it.
//       const int sz = synti->eventFifos()->getSize(false);
//       for(int i = 0; i < sz; ++i)
//       for(iMPEvent impe = synti->_outUserEvents.begin(); impe != synti->_outUserEvents.end(); )
      while(1)
      {  
        if(impe_pb != synti->_outPlaybackEvents.end() && impe_us != synti->_outUserEvents.end())
          using_pb = *impe_pb < *impe_us;
        else if(impe_pb != synti->_outPlaybackEvents.end())
          using_pb = true;
        else if(impe_us != synti->_outUserEvents.end())
          using_pb = false;
        else break;
        
//         // True = use the size snapshot.
//         const MidiPlayEvent& ev(synti->eventFifos()->peek(true)); 
//         const MidiPlayEvent& ev = *impe;
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

//         // Done with ring buffer event. Remove it from FIFO.
//         // True = use the size snapshot.
//         synti->eventFifos()->remove(true);
        
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
        // True = use the size snapshot.
//         synti->eventFifos()->remove(true);
        
        // C++11.
        //impe = synti->_outUserEvents.erase(impe);
        if(using_pb)
          impe_pb = synti->_outPlaybackEvents.erase(impe_pb);
        else
          impe_us = synti->_outUserEvents.erase(impe_us);
      }

      if(curPos < n)
        process(buffer, curPos, n - curPos);
      
// REMOVE Tim. autoconnect. Changed.
//       return i;
      return true;
      //return el->end();
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
//   putEvent
//---------------------------------------------------------

// REMOVE Tim. autoconnect. Changed.
//bool MetronomeSynthIF::putEvent(const MidiPlayEvent& ev)
bool MetronomeSynthIF::processEvent(const MidiPlayEvent& ev)
{
    if(ev.type() != MusECore::ME_NOTEON)
      return false;
    if (ev.dataA() == MusECore::measureSound) {
        if (MusEGlobal::clickSamples == MusEGlobal::origSamples) {
            data = defaultClickEmphasis;
            len  = defaultClickEmphasisLength;
        }
        else {
              data = measSamples;
              len  = measLen;
        }
        volume = MusEGlobal::measClickVolume;
    }
    else if (ev.dataA() == MusECore::beatSound) {
        if (MusEGlobal::clickSamples == MusEGlobal::origSamples) {
            data = defaultClick;
            len  = defaultClickLength;
        } else {
            data = beatSamples;
            len  = beatLen;
        }
        volume = MusEGlobal::beatClickVolume;
    }
    else if (ev.dataA() == MusECore::accent1Sound) {
             data = accent1Samples;
             len  = accent1Len;
             volume = MusEGlobal::accent1ClickVolume;
             if (MusEGlobal::clickSamples == MusEGlobal::origSamples) {
                 volume=0.0;
             }
    }
    else if (ev.dataA() == MusECore::accent2Sound) {
             data = accent2Samples;
             len  = accent2Len;
             volume = MusEGlobal::accent2ClickVolume;
             if (MusEGlobal::clickSamples == MusEGlobal::origSamples) {
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
      // Added by Tim. p3.3.18
      #ifdef METRONOME_DEBUG
      printf("MusE: MetronomeSynthIF::process data:%p offset:%d n:%d\n", data, offset, n);
      #endif

      if (data == 0)
        return;

      const float* s = data + pos;
      float* d       = *buffer + offset;
      int l          = std::min(n, len);

      for (int i = 0; i < l; ++i)
            *d++ += *s++ * MusEGlobal::audioClickVolume * volume;
      pos += l;
      len -= l;
      if (len <= 0)
            data = 0;
      }

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
