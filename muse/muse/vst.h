//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: vst.h,v 1.11.2.3 2009/11/25 09:09:44 terminator356 Exp $
//  (C) Copyright 2004 Werner Schweer (ws@seh.de)
//=========================================================

#ifndef __VST_H__
#define __VST_H__

#include "synth.h"

struct _FSTHandle;
struct _FST;

//---------------------------------------------------------
//   VstSynth
//---------------------------------------------------------

class VstSynth : public Synth {
      _FSTHandle* fstHandle;

   public:
      //VstSynth(const QFileInfo& fi) : Synth(fi) { fstHandle = 0; }
      VstSynth(const QFileInfo& fi) : Synth(fi, fi->baseName()) {
            fstHandle = 0;
            }
      
      virtual ~VstSynth() {}
      virtual void incInstances(int val);
      virtual void* instantiate();
      //virtual SynthIF* createSIF() const;
      virtual SynthIF* createSIF(SynthI*) const;
      };

//---------------------------------------------------------
//   VstSynthIF
//    VSTi synthesizer instance
//---------------------------------------------------------

class VstSynthIF : public SynthIF
      {
      _FST* _fst;
      bool _guiVisible;

   public:
      //VstSynthIF() { _fst = 0; _guiVisible = false; }
      VstSynthIF(SynthI* s) : SynthIF(s) {
            _fst = 0;
            _guiVisible = false;
            }
            
      virtual bool guiVisible() const;
      virtual void showGui(bool v);
      virtual bool hasGui() const;
      virtual void getGeometry(int*, int*, int*, int*) const {}
      virtual void setGeometry(int, int, int, int) {}
      virtual iMPEvent getData(MidiPort*, MPEventList*, iMPEvent, unsigned pos, int ports, unsigned n, float** buffer) ;
      virtual bool putEvent(const MidiPlayEvent& ev);
      virtual MidiPlayEvent receiveEvent();
      virtual int eventsPending() const { return 0; }
      virtual bool init(Synth*);
      virtual int channels() const;
      virtual int totalOutChannels() const;
      virtual int totalInChannels() const;
      virtual void deactivate3();
      virtual const char* getPatchName(int, int, int, bool) const { return ""; }
      virtual const char* getPatchName(int, int, MType, bool) { return ""; }
      virtual void populatePatchPopup(QPopupMenu*, int, MType, bool) {};
      virtual void write(int level, Xml& xml) const;
      virtual void setParameter(int idx, float value);
      virtual int getControllerInfo(int, const char**, int*, int*, int*, int*) { return 0; }
      };


#endif

