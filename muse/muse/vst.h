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

#ifndef __VST_H__
#define __VST_H__

#include "synth.h"
#include "plugin.h"

struct _FSTHandle;
struct _FST;
class VstPluginIF;

//---------------------------------------------------------
//   FstPlugin
//---------------------------------------------------------

class FstPlugin {
      _FST* _fst;
      int version;

   public:
      FstPlugin();
      ~FstPlugin();
      void instantiate(FSTHandle* s, void*);
      int numInputs() const;
      int numOutputs() const;
      int numParameter() const;
      void setProgram(int p);
      void mainsChanged(bool on);
      void setBlockSize(int bs);
      void setSampleRate(float sr);
      bool runEditor();
      void destroyEditor();
      int getVstVersion();
      bool hasGui() const;
      bool canReplacing();
      void setParameter(int idx, float value);
      float getParameter(int idx);
      void processReplacing(float**, float**, int);
      void process(float**, float**, int);
      void putEvent(const MidiEvent& ev);
      const char* getParameterName(int idx) const;
      const char* getParameterLabel(int idx) const;
      const char* getParameterDisplay(int idx, float val) const;
      };

//---------------------------------------------------------
//   VstSynth
//---------------------------------------------------------

class VstSynth : public Synth {
      _FSTHandle* fstHandle;
      int vstVersion;

   public:
      VstSynth(const QFileInfo* fi) : Synth(fi, fi->baseName()) {
            fstHandle = 0;
            }
      virtual ~VstSynth() {}
      virtual void incInstances(int val);
      virtual SynthIF* createSIF(SynthI*);
      };

//---------------------------------------------------------
//   VstSynthIF
//    VSTi synthesizer instance
//---------------------------------------------------------

class VstSynthIF : public SynthIF
      {
      FstPlugin* _fst;
      bool _guiVisible;

   public:
      VstSynthIF(SynthI* s) : SynthIF(s) {
            _fst = 0;
            _guiVisible = false;
            }
      virtual ~VstSynthIF();

      virtual bool guiVisible() const;
      virtual void showGui(bool v);
      virtual bool hasGui() const;
      virtual void getGeometry(int*, int*, int*, int*) const {}
      virtual void setGeometry(int, int, int, int) {}
      virtual void getData(MidiEventList*, unsigned pos, int ports, unsigned n, float** buffer) ;
      virtual bool putEvent(const MidiEvent& ev);
      virtual MidiEvent receiveEvent();
      virtual int eventsPending() const { return 0; }
      virtual int channels() const;
      virtual void deactivate3();
      virtual QString getPatchName(int, int) { return QString(""); }
      virtual void populatePatchPopup(QMenu*, int) {};
      virtual void write(Xml& xml) const;
      virtual void setParameter(int idx, float value);
      virtual int getControllerInfo(int, const char**, int*, int*, int*) { return 0; }
      bool init(FSTHandle*);
      };

//---------------------------------------------------------
//   VstPlugin
//---------------------------------------------------------

class VstPlugin : public Plugin {
      _FSTHandle* fstHandle;
      FSTInfo* info;

   protected:
      int vstVersion;
      friend class VstPluginIF;

   public:
      VstPlugin(const QFileInfo* fi, FSTInfo* i);
      virtual ~VstPlugin();
      virtual PluginIF* createPIF(PluginI*);
      virtual QString name() const      { return QString(info->name); }
      virtual QString label() const     { return QString(info->name); }

      virtual int parameter() const;
      virtual int inports() const;
      virtual int outports() const;
      virtual unsigned long id() const;
      };

//---------------------------------------------------------
//   VstPluginIF
//    Vst plugin instance interface
//---------------------------------------------------------

class VstPluginIF : public PluginIF {
      FstPlugin* _fst;
      bool _guiVisible;

   public:
      VstPluginIF(PluginI*);
      virtual ~VstPluginIF();

      virtual void apply(unsigned nframes, float** src, float** dst);
      virtual void activate() {}
      virtual void deactivate() {}
      virtual void cleanup() {}
      virtual void setParam(int i, double val);
      virtual float param(int i) const;
      virtual const char* getParameterName(int k) const;
      virtual const char* getParameterLabel(int) const;
      virtual const char* getParameterDisplay(int, float) const;
      virtual bool hasGui() const;
      virtual bool guiVisible() const { return _guiVisible; }
      virtual void showGui(bool f);
      bool init(FSTHandle*);
      };

#endif

