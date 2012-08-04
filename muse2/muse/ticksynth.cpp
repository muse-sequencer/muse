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

#include "audio.h"
#include "ticksynth.h"
#include "default_click.h"

#include "popupmenu.h"

// If sysex support is ever added, make sure this number is unique among all the MESS synths.
//#define METRONOME_UNIQUE_ID      7

//#define METRONOME_DEBUG

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
      void process(float** buffer, int offset, int n);

   public:
      MetronomeSynthIF(SynthI* s) : SynthIF(s) {
            data = 0;
            }
      virtual bool initGui()     { return true; };
      virtual void guiHeartBeat()  {  }
      virtual bool guiVisible() const { return false; }
      virtual void showGui(bool) {}
      virtual bool hasGui() const { return false; }
      virtual bool nativeGuiVisible() const { return false; }
      virtual void showNativeGui(bool) { };
      virtual bool hasNativeGui() const { return false; }
      
      virtual void getGeometry(int*x, int*y, int*w, int*h) const { *x=0;*y=0;*w=0;*h=0; }
      virtual void setGeometry(int, int, int, int) {}
      virtual void getNativeGeometry(int*x, int*y, int*w, int*h) const { *x=0;*y=0;*w=0;*h=0; }
      virtual void setNativeGeometry(int, int, int, int) {}
      virtual void preProcessAlways() { };
      virtual iMPEvent getData(MidiPort*, MPEventList*, iMPEvent, unsigned pos, int ports, unsigned n, float** buffer);
      virtual bool putEvent(const MidiPlayEvent& ev);
      virtual MidiPlayEvent receiveEvent() { return MidiPlayEvent(); }
      virtual int eventsPending() const { return 0; }
      
      virtual int channels() const { return 1; }
      virtual int totalOutChannels() const { return 1; }
      virtual int totalInChannels() const { return 0; }
      virtual void deactivate3() {}
      virtual const char* getPatchName(int, int, int, bool) const { return ""; }
      virtual const char* getPatchName(int, int, MType, bool) { return ""; }
      virtual void populatePatchPopup(MusEGui::PopupMenu*, int, MType, bool) {};
      virtual void write(int, Xml&) const {}
      virtual float getParameter(unsigned long) const  { return 0.0; }
      virtual void setParameter(unsigned long, float) {}
      virtual int getControllerInfo(int, const char**, int*, int*, int*, int*) { return 0; }
      };

//---------------------------------------------------------
//   getData
//---------------------------------------------------------

iMPEvent MetronomeSynthIF::getData(MidiPort*, MPEventList* el, iMPEvent i, unsigned pos, int/*ports*/, unsigned n, float** buffer)
      {
      // Added by Tim. p3.3.18
      #ifdef METRONOME_DEBUG
      printf("MusE: MetronomeSynthIF::getData\n");
      #endif

      //set type to unsigned , due to compiler warning: comparison signed/unsigned
      unsigned int curPos      = pos;             //prevent compiler warning: comparison signed/unsigned
      unsigned int endPos      = pos + n;         //prevent compiler warning: comparison signed/unsigned
      unsigned int off         = pos;             //prevent compiler warning: comparison signed/unsigned
      int frameOffset = MusEGlobal::audio->getFrameOffset();

      for (; i != el->end(); ++i) {
            unsigned int frame = i->time() - frameOffset; //prevent compiler warning: comparison signed /unsigned
            if (frame >= endPos)
                  break;
            if (frame > curPos) {
                  if (frame < pos)
                        printf("should not happen: missed event %d\n", pos -frame);
                  else
                        process(buffer, curPos-pos, frame - curPos);
                  curPos = frame;
                  }
            putEvent(*i);
            }
      if (endPos - curPos)
            process(buffer, curPos - off, endPos - curPos);
      return el->end();
      }

//---------------------------------------------------------
//   putEvent
//---------------------------------------------------------

bool MetronomeSynthIF::putEvent(const MidiPlayEvent& ev)
      {
      if (ev.dataA() == 0) {
            data = defaultClickEmphasis;
            len  = defaultClickEmphasisLength;
            }
      else {
            data = defaultClick;
            len  = defaultClickLength;
            }
      pos = 0;
      return false;
      }

// DELETETHIS 9
//---------------------------------------------------------
//   createSIF
//---------------------------------------------------------

//SynthIF* MetronomeSynth::createSIF() const
//      {
//      return new MetronomeSynthIF();
//      }
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
            *d++ += *s++ * MusEGlobal::audioClickVolume;
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
