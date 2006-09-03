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

#include "audio.h"
#include "ticksynth.h"
#include "default_click.h"

SynthI* metronome = 0;

class MetronomeSynth;
static MetronomeSynth* metronomeSynth;

//---------------------------------------------------------
//   MetronomeSynth
//---------------------------------------------------------

class MetronomeSynth : public Synth {
   public:
      MetronomeSynth(const QFileInfo* fi) : Synth(fi, QString("Metronome")) {}
      virtual ~MetronomeSynth() {}
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
      virtual bool guiVisible() const { return false; }
      virtual void showGui(bool) {}
      virtual bool hasGui() const { return false; }
      virtual void getGeometry(int*, int*, int*, int*) const {}
      virtual void setGeometry(int, int, int, int) {}
      virtual iMPEvent getData(MPEventList*, iMPEvent, unsigned pos, int ports, unsigned n, float** buffer);
      virtual bool putEvent(const MidiEvent& ev);
      virtual MidiEvent receiveEvent() { return MidiEvent(); }
      virtual int eventsPending() const { return 0; }
      virtual int channels() const { return 1; }
      virtual void deactivate2() {}
      virtual void deactivate3() {}
      virtual QString getPatchName(int, int, int) const { return ""; }
      virtual QString getPatchName(int, int) { return ""; }
      virtual void populatePatchPopup(QMenu*, int) {};
      virtual void write(Xml&) const {}
      virtual void setParameter(int, float) {}
      virtual int getControllerInfo(int, const char**, int*, int*, int*) { return 0; }
      virtual bool hasAuxSend() const  { return false; }
      };

//---------------------------------------------------------
//   getData
//---------------------------------------------------------

iMPEvent MetronomeSynthIF::getData(MPEventList* el, iMPEvent i, unsigned pos, int/*ports*/, unsigned n, float** buffer)
      {
      unsigned curPos      = pos;
      unsigned endPos      = pos + n;
      unsigned off         = pos;
      unsigned frameOffset = audio->getFrameOffset();

      for (; i != el->end(); ++i) {
            unsigned frame = i->time() - frameOffset;
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

bool MetronomeSynthIF::putEvent(const MidiEvent& ev)
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
      if (data == 0) {
            memset(buffer[0], 0, n * sizeof(float));
            return;
            }
      const float* s = data + pos;
      float* d       = *buffer + offset;
      int l          = std::min(n, len);
      int i;
      for (i = 0; i < l; ++i)
            *d++ = *s++;
      for (; i < n; ++i)
            *d++ = 0.0f;
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
      QFileInfo fi;     // dummy
      metronomeSynth = new MetronomeSynth(&fi);
      metronome = new SynthI();
      metronome->setName("metronome");
      metronome->initInstance(metronomeSynth);
      }

