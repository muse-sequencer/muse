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

#ifndef __SYNTH_H__
#define __SYNTH_H__

#include "globals.h"
// #include "instruments/minstrument.h"
#include "audiotrack.h"
#include "midiout.h"

class Mess;
struct MESS;

class SynthI;
class SynthIF;

//---------------------------------------------------------
//   Synth
//    software synthesizer
//---------------------------------------------------------

class Synth {
   protected:
      QFileInfo info;
      QString _name;
      int _instances;

   public:
      Synth(const QFileInfo* fi, QString label);
      virtual ~Synth() {}
      virtual const char* description() const { return ""; }
      virtual const char* version() const { return ""; }

      int instances() const       { return _instances; }
      virtual void incInstances(int val) { _instances += val; }

      QString name() const        { return _name; }
      QString path() const        { return info.absolutePath(); }
      virtual SynthIF* createSIF(SynthI*) = 0;
      };

//---------------------------------------------------------
//   MessSynth
//---------------------------------------------------------

class MessSynth : public Synth {
      const MESS* descr;

   public:
      MessSynth(const QFileInfo* fi) : Synth(fi, fi->baseName()) { descr = 0; }
      virtual ~MessSynth() {}
      virtual const char* description() const;
      virtual const char* version() const;
      virtual void* instantiate(const QString&);
      virtual SynthIF* createSIF(SynthI*);
      };

class Mess;

//---------------------------------------------------------
//   SynthIF
//    synth instance interface
//---------------------------------------------------------

class SynthIF {

   protected:
      SynthI* synti;

   public:
      SynthIF(SynthI* s) { synti = s; }
      virtual ~SynthIF() {}

      virtual bool guiVisible() const = 0;
      virtual void showGui(bool v) = 0;
      virtual bool hasGui() const = 0;
      virtual void getGeometry(int*, int*, int*, int*) const = 0;
      virtual void setGeometry(int, int, int, int) = 0;
      virtual iMPEvent getData(MPEventList*, iMPEvent, unsigned pos, int ports, unsigned n, float** buffer) = 0;
      virtual bool putEvent(const MidiEvent& ev) = 0;
      virtual MidiEvent receiveEvent() = 0;
      virtual int eventsPending() const = 0;
      virtual int channels() const = 0;
      virtual void deactivate3() = 0;
      virtual QString getPatchName(int, int) = 0;
      virtual void populatePatchPopup(QMenu*, int) = 0;
      virtual void write(Xml& xml) const = 0;
      virtual void setParameter(int idx, float value) = 0;
      virtual int getControllerInfo(int id, const char** name, int* ctrl, int* min, int* max) = 0;
      virtual bool hasAuxSend() const  { return true; }
      };

//---------------------------------------------------------
//   SynthI
//    software synthesizer instance
//    Track
//    MidiInstrument
//---------------------------------------------------------

class SynthI : public AudioTrack, public MidiOut
      {
      Q_OBJECT

      SynthIF* _sif;

   protected:
      Synth* synthesizer;
      std::vector<float> initParams;

      bool putMidiEvent(const MidiEvent& ev) {
            return _sif->putEvent(ev);
            }

      virtual QString open() { return QString("OK");}
      virtual void close()   {}
      virtual void collectInputData();

   public:
      SynthI();
      virtual ~SynthI();
      virtual TrackType type() const { return AUDIO_SOFTSYNTH; }

      virtual void setName(const QString& s);

      SynthIF* sif() const { return _sif; }
      bool initInstance(Synth* s);

      void read(QDomNode);
      virtual void write(Xml&) const;

      Synth* synth() const          { return synthesizer; }
      virtual bool isSynti() const  { return true; }

      virtual QString getPatchName(int ch, int prog) {
            return _sif->getPatchName(ch, prog);
            }
      virtual void populatePatchPopup(QMenu* m, int i) {
            _sif->populatePatchPopup(m, i);
            }

      void setParameter(const char* name, const char* value) const;

      bool guiVisible() const { return _sif->guiVisible(); }
      void showGui(bool v)    { _sif->showGui(v); }
      bool hasGui() const     { return _sif->hasGui(); }
      void getGeometry(int* x, int* y, int* w, int* h) const {
            _sif->getGeometry(x, y, w, h);
            }
      void setGeometry(int x, int y, int w, int h) {
            _sif->setGeometry(x, y, w, h);
            }
      MidiEvent receiveEvent() 	{ return _sif->receiveEvent(); }
      int eventsPending() const    	{ return _sif->eventsPending(); }
      void deactivate2();
      void deactivate3();
      bool isActivated() const         { return synthesizer && _sif; }
      virtual bool hasAuxSend() const  { return _sif->hasAuxSend(); }
      void processMidi(unsigned fromTick, unsigned toTick, unsigned fromFrame, unsigned toFrame);
      };

//---------------------------------------------------------
//   MessSynthIF
//    mess synthesizer instance
//---------------------------------------------------------

class MessSynthIF : public SynthIF {
      Mess* _mess;

   public:
      MessSynthIF(SynthI* s) : SynthIF(s) { _mess = 0; }
      virtual ~MessSynthIF();

      virtual bool guiVisible() const;
      virtual void showGui(bool v);
      virtual bool hasGui() const;
      virtual void getGeometry(int*, int*, int*, int*) const;
      virtual void setGeometry(int, int, int, int);
      virtual iMPEvent getData(MPEventList*, iMPEvent, unsigned pos, int ports, unsigned n, float** buffer);
      virtual bool putEvent(const MidiEvent& ev);
      virtual MidiEvent receiveEvent();
      virtual int eventsPending() const;
      virtual int channels() const;
      virtual void deactivate3();
      virtual QString getPatchName(int, int);
      virtual void populatePatchPopup(QMenu*, int);
      virtual void write(Xml& xml) const;
      virtual void setParameter(int, float) {}
      virtual int getControllerInfo(int id, const char** name, int* ctrl, int* min, int* max);
      bool init(Synth* s, SynthI* si);
      };

typedef tracklist<SynthI*>::iterator iSynthI;
typedef tracklist<SynthI*>::const_iterator ciSynthI;
typedef tracklist<SynthI*> SynthIList;

extern std::vector<Synth*> synthis;  // array of available synthis
extern Synth* findSynth(const QString& sclass);

#endif

