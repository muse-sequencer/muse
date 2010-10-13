//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: ticksynth.cpp,v 1.8.2.7 2009/12/20 05:00:35 terminator356 Exp $
//  (C) Copyright 2004 Werner Schweer (ws@seh.de)
//=========================================================

#include "audio.h"
#include "ticksynth.h"
#include "default_click.h"
//Added by qt3to4:
#include <Q3PopupMenu>

// Added by Tim. p3.3.18
//#define METRONOME_DEBUG

MetronomeSynthI* metronome = 0;

class MetronomeSynth;
static MetronomeSynth* metronomeSynth = 0;

//---------------------------------------------------------
//   MetronomeSynth
//---------------------------------------------------------

class MetronomeSynth : public Synth {
   public:
      //MetronomeSynth(const QFileInfo& fi) : Synth(fi) {}
      //MetronomeSynth(const QFileInfo& fi) : Synth(fi, QString("Metronome")) {}
      MetronomeSynth(const QFileInfo& fi) : Synth(fi, QString("Metronome"), QString("Metronome"), QString(), QString()) {}
      virtual ~MetronomeSynth() {}
      virtual void incInstances(int) {}
      virtual void* instantiate();
      
      //virtual SynthIF* createSIF() const;
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
      //MetronomeSynthIF() {
      //      data = 0;
      //      }
      MetronomeSynthIF(SynthI* s) : SynthIF(s) {
            data = 0;
            }
      virtual bool initGui()     { return true; };
      virtual void guiHeartBeat()  {  }
      virtual bool guiVisible() const { return false; }
      virtual void showGui(bool) {}
      virtual bool hasGui() const { return false; }
      virtual void getGeometry(int*, int*, int*, int*) const {}
      virtual void setGeometry(int, int, int, int) {}
      virtual void preProcessAlways() { };
      virtual iMPEvent getData(MidiPort*, MPEventList*, iMPEvent, unsigned pos, int ports, unsigned n, float** buffer);
      virtual bool putEvent(const MidiPlayEvent& ev);
      virtual MidiPlayEvent receiveEvent() { return MidiPlayEvent(); }
      virtual int eventsPending() const { return 0; }
      
      //virtual bool init(Synth*) { return true; }
      
      virtual int channels() const { return 1; }
      virtual int totalOutChannels() const { return 1; }
      virtual int totalInChannels() const { return 0; }
      virtual void deactivate3() {}
      virtual const char* getPatchName(int, int, int, bool) const { return ""; }
      virtual const char* getPatchName(int, int, MType, bool) { return ""; }
      virtual void populatePatchPopup(Q3PopupMenu*, int, MType, bool) {};
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
      int frameOffset = audio->getFrameOffset();

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
            *d++ += *s++ * audioClickVolume;
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

