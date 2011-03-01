//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: synth.h,v 1.22.2.12 2009/12/06 10:05:00 terminator356 Exp $
//  (C) Copyright 2000-2004 Werner Schweer (ws@seh.de)
//=========================================================

#ifndef __SYNTH_H__
#define __SYNTH_H__

#include <string>
#include <vector>
#include <map>

#include "globals.h"
#include "node.h"
#include "instruments/minstrument.h"
#include "mididev.h"
#include "midiport.h"
#include "track.h"
#include "stringparam.h"

#include <QFileInfo>

class QMenu;

//class MidiEvent;
class MidiPlayEvent;
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
      int _instances;
      QString _name;
      QString _description;
      QString _maker;
      QString _version;

   public:
      //Synth(const QFileInfo& fi);
      //Synth(const QFileInfo& fi, QString label);
      Synth(const QFileInfo& fi, QString label, QString descr, QString maker, QString ver);
      
      virtual ~Synth() {}
      //virtual const char* description() const { return ""; }
      //virtual const char* version() const { return ""; }

      int instances() const                            { return _instances; }
      virtual void incInstances(int val)               { _instances += val; }
      QString completeBaseName()             /*const*/ { return info.completeBaseName(); } // ddskrjo
      QString baseName()                     /*const*/ { return info.baseName(); } // ddskrjo
      QString name() const                             { return _name; }
      QString absolutePath() const                     { return info.absolutePath(); }
      QString path() const                             { return info.path(); }
      QString filePath() const                         { return info.filePath(); }
      QString description() const                      { return _description; }
      QString version() const                          { return _version; }
      //QString maker() const                            { return _version; } ??
      QString maker() const                            { return _maker; }
      
      //virtual void* instantiate() = 0;
      
      //virtual SynthIF* createSIF() const = 0;
      virtual SynthIF* createSIF(SynthI*) = 0;
      };

//---------------------------------------------------------
//   MessSynth
//---------------------------------------------------------

class MessSynth : public Synth {
      const MESS* _descr;

   public:
      //MessSynth(const QFileInfo& fi) : Synth(fi) { descr = 0; }
      //MessSynth(const QFileInfo& fi) : Synth(fi, fi.baseName()) { descr = 0; }
      MessSynth(const QFileInfo& fi, QString label, QString descr, QString maker, QString ver) : 
               Synth(fi, label, descr, maker, ver) { _descr = 0; }
      
      virtual ~MessSynth() {}
      //virtual const char* description() const;
      //virtual const char* version() const;
      
      //virtual void* instantiate();
      virtual void* instantiate(const QString&);
      
      //virtual SynthIF* createSIF() const;
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
      //SynthIF() {}
      SynthIF(SynthI* s) { synti = s; }
      virtual ~SynthIF() {}

      virtual bool initGui() = 0;
      virtual void guiHeartBeat() = 0;
      virtual bool guiVisible() const = 0;
      virtual void showGui(bool v) = 0;
      virtual bool hasGui() const = 0;
      virtual void getGeometry(int*, int*, int*, int*) const = 0;
      virtual void setGeometry(int, int, int, int) = 0;
      virtual void preProcessAlways() = 0;
      virtual iMPEvent getData(MidiPort*, MPEventList*, iMPEvent, unsigned pos, int ports, unsigned n, float** buffer) = 0;
      virtual bool putEvent(const MidiPlayEvent& ev) = 0;
      virtual MidiPlayEvent receiveEvent() = 0;
      virtual int eventsPending() const = 0;
      
      //virtual bool init(Synth* s) = 0;
      
      virtual int channels() const = 0;
      virtual int totalOutChannels() const = 0;
      virtual int totalInChannels() const = 0;
      virtual void deactivate3() = 0;
      virtual const char* getPatchName(int, int, int, bool) const = 0;
      virtual const char* getPatchName(int, int, MType, bool) = 0;
      virtual void populatePatchPopup(QMenu*, int, MType, bool) = 0;
      virtual void write(int level, Xml& xml) const = 0;
      virtual float getParameter(unsigned long idx) const = 0;
      virtual void setParameter(unsigned long idx, float value) = 0;
      virtual int getControllerInfo(int id, const char** name, int* ctrl, int* min, int* max, int* initval) = 0;
      };

//---------------------------------------------------------
//   SynthI
//    software synthesizer instance
//    Track
//    MidiDevice
//    MidiInstrument
//---------------------------------------------------------

class SynthI : public AudioTrack, public MidiDevice,
   public MidiInstrument
      {
      static bool _isVisible;
      SynthIF* _sif;

   protected:
      Synth* synthesizer;
      // MidiFifo putFifo;  // Moved into MidiDevice p4.0.15
      
      // List of initial floating point parameters, for synths which use them. 
      // Used once upon song reload, then discarded.
      std::vector<float> initParams;
      // List of gui controls to update upon heartbeat.
      std::vector<bool> _guiUpdateControls;  
      // Update gui program upon heartbeat.
      bool _guiUpdateProgram;
      // Initial, and running, string parameters for synths which use them, like dssi.
      StringParamMap _stringParamMap; 
      // Current bank and program for synths which use them, like dssi. 
      // In cases like dssi which have no 'hi' and 'lo' bank, just use _curBankL.
      unsigned long _curBankH;
      unsigned long _curBankL;
      unsigned long _curProgram;

      void preProcessAlways();
      bool getData(unsigned a, int b, unsigned c, float** data);
      
      //bool putEvent(const MidiPlayEvent& ev);

      virtual QString open();
      virtual void close();
      
      virtual bool putMidiEvent(const MidiPlayEvent&) {return true;}
      //bool putMidiEvent(const MidiEvent&); 
      
      virtual Track* newTrack() const { return 0; }

   public:
      friend class SynthIF;
      friend class MessSynthIF;
      friend class DssiSynthIF;
      friend class VstSynthIF;
      
      SynthI();
      virtual ~SynthI();
      //SynthI* clone() const { return new SynthI(*this); }
      SynthI* clone(bool /*cloneParts*/) const { return new SynthI(*this); }

      virtual inline int deviceType() { return SYNTH_MIDI; } 
      
      SynthIF* sif() const { return _sif; }
      bool initInstance(Synth* s, const QString& instanceName);

      void readProgram(Xml&, const QString&);
      void read(Xml&);
      virtual void write(int, Xml&) const;

      void setName(const QString& s);
      QString name() const          { return AudioTrack::name(); }

      Synth* synth() const          { return synthesizer; }
      virtual bool isSynti() const  { return true; }

      //virtual const char* getPatchName(int ch, int prog, MType t, bool dr) {
      virtual QString getPatchName(int ch, int prog, MType t, bool dr) {
            return _sif->getPatchName(ch, prog, t, dr);
            }
            
      virtual void populatePatchPopup(QMenu* m, int i, MType t, bool d) {
            _sif->populatePatchPopup(m, i, t, d);
            }
      
      // void setParameter(const char* name, const char* value) const;   // Not required
      //StringParamMap& stringParameters() { return _stringParamMap; }   // Not required
      void currentProg(unsigned long */*prog*/, unsigned long */*bankL*/, unsigned long */*bankH*/);

      void guiHeartBeat()     { return _sif->guiHeartBeat(); }
      bool initGui()    const { return _sif->initGui(); }
      bool guiVisible() const { return _sif->guiVisible(); }
      void showGui(bool v)    { _sif->showGui(v); }
      bool hasGui() const     { return _sif->hasGui(); }
      void getGeometry(int* x, int* y, int* w, int* h) const {
            _sif->getGeometry(x, y, w, h);
            }
      void setGeometry(int x, int y, int w, int h) {
            _sif->setGeometry(x, y, w, h);
            }

      bool putEvent(const MidiPlayEvent& ev);
      
      MidiPlayEvent receiveEvent() { return _sif->receiveEvent(); }
      int eventsPending() const    { return _sif->eventsPending(); }
      void deactivate2();
      void deactivate3();
      bool isActivated() const         { return synthesizer && _sif; }
      virtual bool hasAuxSend() const  { return true; }
      static void setVisible(bool t) { _isVisible = t; }
      virtual int height() const;
      };

//---------------------------------------------------------
//   MessSynthIF
//    mess synthesizer instance
//---------------------------------------------------------

class MessSynthIF : public SynthIF {
      Mess* _mess;

   public:
      //MessSynthIF() { _mess = 0; }
      MessSynthIF(SynthI* s) : SynthIF(s) { _mess = 0; }
      virtual ~MessSynthIF() { }

      virtual bool initGui()      { return true; };
      virtual void guiHeartBeat()  {  }
      virtual bool guiVisible() const;
      virtual void showGui(bool v);
      virtual bool hasGui() const;
      virtual void getGeometry(int*, int*, int*, int*) const;
      virtual void setGeometry(int, int, int, int);
      virtual void preProcessAlways();
      virtual iMPEvent getData(MidiPort*, MPEventList*, iMPEvent, unsigned pos, int ports, unsigned n, float** buffer);
      virtual bool putEvent(const MidiPlayEvent& ev);
      virtual MidiPlayEvent receiveEvent();
      virtual int eventsPending() const;
      //virtual bool init(Synth* s);
      bool init(Synth* s, SynthI* si);
      
      virtual int channels() const;
      virtual int totalOutChannels() const;
      virtual int totalInChannels() const;
      virtual void deactivate3();
      virtual const char* getPatchName(int, int, int, bool) const { return ""; }
      virtual const char* getPatchName(int, int, MType, bool);
      virtual void populatePatchPopup(QMenu*, int, MType, bool);
      virtual void write(int level, Xml& xml) const;
      virtual float getParameter(unsigned long) const { return 0.0; }
      virtual void setParameter(unsigned long, float) {}
      virtual int getControllerInfo(int id, const char** name, int* ctrl, int* min, int* max, int* initval);
      };

extern std::vector<Synth*> synthis;  // array of available synthis
#endif

