//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: synth.h,v 1.22.2.12 2009/12/06 10:05:00 terminator356 Exp $
//  (C) Copyright 2000-2004 Werner Schweer (ws@seh.de)
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

// Current version of saved midistate data.
#define SYNTH_MIDI_STATE_SAVE_VERSION 2

class Mess;
struct MESS;

namespace MusEGui {
class PopupMenu;
}

namespace MusECore {

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
      enum Type { METRO_SYNTH=0, MESS_SYNTH, DSSI_SYNTH, VST_SYNTH, SYNTH_TYPE_END };

      Synth(const QFileInfo& fi, QString label, QString descr, QString maker, QString ver);
      
      virtual ~Synth() {}

      virtual Type synthType() const = 0;
      int instances() const                            { return _instances; }
      virtual void incInstances(int val)               { _instances += val; }
      QString completeBaseName()                       { return info.completeBaseName(); } // ddskrjo
      QString baseName()                               { return info.baseName(); } // ddskrjo
      QString name() const                             { return _name; }
      QString absolutePath() const                     { return info.absolutePath(); }
      QString path() const                             { return info.path(); }
      QString filePath() const                         { return info.filePath(); }
      QString fileName() const                         { return info.fileName(); }
      QString description() const                      { return _description; }
      QString version() const                          { return _version; }
      QString maker() const                            { return _maker; }
      
      virtual SynthIF* createSIF(SynthI*) = 0;
      };

//---------------------------------------------------------
//   MessSynth
//---------------------------------------------------------

class MessSynth : public Synth {
      const MESS* _descr;

   public:
      MessSynth(const QFileInfo& fi, QString label, QString descr, QString maker, QString ver) : 
               Synth(fi, label, descr, maker, ver) { _descr = 0; }
      
      virtual ~MessSynth() {}
      virtual Type synthType() const { return MESS_SYNTH; }

      virtual void* instantiate(const QString&);
      
      virtual SynthIF* createSIF(SynthI*);
      };


//---------------------------------------------------------
//   SynthIF
//    synth instance interface
//   NOTICE: If implementing sysex support, be sure to make a unique ID and use   
//    it to filter out unrecognized sysexes. Headers should be constructed as:
//      MUSE_SYNTH_SYSEX_MFG_ID        The MusE SoftSynth Manufacturer ID byte (0x7C) found in midi.h 
//      0xNN                           The synth's unique ID byte
//---------------------------------------------------------

class SynthIF {
      
   protected:
      SynthI* synti;
      
   public:
      SynthIF(SynthI* s) { synti = s; }
      virtual ~SynthIF() {}

      // This is only a kludge required to support old songs' midistates. Do not use in any new synth.
      virtual int oldMidiStateHeader(const unsigned char** /*data*/) const { return 0; } 

      virtual bool initGui() = 0;
      virtual void guiHeartBeat() = 0;
      virtual bool guiVisible() const = 0;
      virtual void showGui(bool v) = 0;
      virtual bool hasGui() const = 0;
      virtual bool nativeGuiVisible() const = 0;
      virtual void showNativeGui(bool v) = 0;
      virtual bool hasNativeGui() const = 0;
      virtual void getGeometry(int*, int*, int*, int*) const = 0;
      virtual void setGeometry(int, int, int, int) = 0;
      virtual void getNativeGeometry(int*, int*, int*, int*) const = 0;
      virtual void setNativeGeometry(int, int, int, int) = 0;
      virtual void preProcessAlways() = 0;
      virtual iMPEvent getData(MidiPort*, MPEventList*, iMPEvent, unsigned pos, int ports, unsigned n, float** buffer) = 0;
      virtual bool putEvent(const MidiPlayEvent& ev) = 0;
      virtual MidiPlayEvent receiveEvent() = 0;
      virtual int eventsPending() const = 0;
      
      virtual int channels() const = 0;
      virtual int totalOutChannels() const = 0;
      virtual int totalInChannels() const = 0;
      virtual void deactivate3() = 0;
      virtual const char* getPatchName(int, int, int, bool) const = 0;
      virtual const char* getPatchName(int, int, MType, bool) = 0;
      virtual void populatePatchPopup(MusEGui::PopupMenu*, int, MType, bool) = 0;
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

      // Initial, and running, string parameters for synths which use them, like dssi.
      StringParamMap _stringParamMap; 

      // Current bank and program for synths which use them, like dssi. 
      // In cases like dssi which have no 'hi' and 'lo' bank, just use _curBankL.
      unsigned long _curBankH;
      unsigned long _curBankL;
      unsigned long _curProgram;

      void preProcessAlways();
      bool getData(unsigned a, int b, unsigned c, float** data);
      
      virtual QString open();
      virtual void close();
      
      virtual bool putMidiEvent(const MidiPlayEvent&) {return true;}
      
      virtual Track* newTrack() const { return 0; }

   public:
      friend class SynthIF;
      friend class MessSynthIF;
      friend class DssiSynthIF;
      friend class VstSynthIF;
      
      SynthI();
      virtual ~SynthI();
      SynthI* clone(int /*flags*/) const { return new SynthI(*this); }

      virtual inline int deviceType() const { return SYNTH_MIDI; } 
      
      SynthIF* sif() const { return _sif; }
      bool initInstance(Synth* s, const QString& instanceName);

      void readProgram(Xml&, const QString&);
      void read(Xml&);
      virtual void write(int, Xml&) const;

      void setName(const QString& s);
      QString name() const          { return AudioTrack::name(); }

      Synth* synth() const          { return synthesizer; }
      virtual bool isSynti() const  { return true; }

      virtual QString getPatchName(int ch, int prog, MType t, bool dr) {
            return _sif->getPatchName(ch, prog, t, dr);
            }
            
      virtual void populatePatchPopup(MusEGui::PopupMenu* m, int i, MType t, bool d) {
            _sif->populatePatchPopup(m, i, t, d);
            }
      
      void currentProg(unsigned long *prog, unsigned long *bankL, unsigned long *bankH);

      void guiHeartBeat()     { return _sif->guiHeartBeat(); }
      bool initGui()    const { return _sif->initGui(); }
      bool guiVisible() const { return _sif->guiVisible(); }
      void showGui(bool v)    { _sif->showGui(v); }
      bool hasGui() const     { return _sif->hasGui(); }
      bool nativeGuiVisible() const { return _sif->nativeGuiVisible(); }
      void showNativeGui(bool v)    { _sif->showNativeGui(v); }
      bool hasNativeGui() const     { return _sif->hasNativeGui(); }
      void getGeometry(int* x, int* y, int* w, int* h) const {
            _sif->getGeometry(x, y, w, h);
            }
      void setGeometry(int x, int y, int w, int h) {
            _sif->setGeometry(x, y, w, h);
            }
      void getNativeGeometry(int* x, int* y, int* w, int* h) const {
            _sif->getNativeGeometry(x, y, w, h);
            }
      void setNativeGeometry(int x, int y, int w, int h) {
            _sif->setNativeGeometry(x, y, w, h);
            }

      bool putEvent(const MidiPlayEvent& ev);
      virtual void processMidi();
      
      MidiPlayEvent receiveEvent() { return _sif->receiveEvent(); }
      int eventsPending() const    { return _sif->eventsPending(); }
      void deactivate2();
      void deactivate3();
      bool isActivated() const         { return synthesizer && _sif; }
      virtual bool hasAuxSend() const  { return true; }
      static void setVisible(bool t) { _isVisible = t; }
      virtual int height() const;
      static bool visible() { return _isVisible; }

      };

//---------------------------------------------------------
//   MessSynthIF
//    mess synthesizer instance
//   NOTICE: If implementing sysex support, be sure to make a unique ID and use   
//    it to filter out unrecognized sysexes. Headers should be constructed as:
//      MUSE_SYNTH_SYSEX_MFG_ID        The MusE SoftSynth Manufacturer ID byte (0x7C) found in midi.h 
//      0xNN                           The synth's unique ID byte
//---------------------------------------------------------

class MessSynthIF : public SynthIF {
      Mess* _mess;

   public:
      MessSynthIF(SynthI* s) : SynthIF(s) { _mess = 0; }
      virtual ~MessSynthIF() { }

      // This is only a kludge required to support old songs' midistates. Do not use in any new synth.
      virtual int oldMidiStateHeader(const unsigned char** data) const;  

      virtual bool initGui()          { return true; }
      virtual void guiHeartBeat()     { }
      virtual bool guiVisible() const { return false; }
      virtual void showGui(bool)    { };
      virtual bool hasGui() const     { return false; }
      virtual bool nativeGuiVisible() const;
      virtual void showNativeGui(bool v);
      virtual bool hasNativeGui() const;
      virtual void getGeometry(int*, int*, int*, int*) const;
      virtual void setGeometry(int, int, int, int);
      virtual void getNativeGeometry(int*, int*, int*, int*) const;
      virtual void setNativeGeometry(int, int, int, int);
      virtual void preProcessAlways();
      virtual iMPEvent getData(MidiPort*, MPEventList*, iMPEvent, unsigned pos, int ports, unsigned n, float** buffer);
      virtual bool putEvent(const MidiPlayEvent& ev);
      virtual MidiPlayEvent receiveEvent();
      virtual int eventsPending() const;
      bool init(Synth* s, SynthI* si);
      
      virtual int channels() const;
      virtual int totalOutChannels() const;
      virtual int totalInChannels() const;
      virtual void deactivate3();
      virtual const char* getPatchName(int, int, int, bool) const { return ""; }
      virtual const char* getPatchName(int, int, MType, bool);
      virtual void populatePatchPopup(MusEGui::PopupMenu*, int, MType, bool);
      virtual void write(int level, Xml& xml) const;
      virtual float getParameter(unsigned long) const { return 0.0; }
      virtual void setParameter(unsigned long, float) {}
      virtual int getControllerInfo(int id, const char** name, int* ctrl, int* min, int* max, int* initval);
      };

extern QString synthType2String(Synth::Type);
extern Synth::Type string2SynthType(const QString&); 
      
} // namespace MusECore

namespace MusEGlobal {
extern std::vector<MusECore::Synth*> synthis;  // array of available synthis
}

#endif

