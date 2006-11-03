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

#ifndef __DSSIHOST_H__
#define __DSSIHOST_H__

#include <lo/lo.h>
#include "synth.h"
#include "plugin.h"

struct _DSSI;
class DssiPluginIF;

//---------------------------------------------------------
//   DssiSynth
//---------------------------------------------------------

class DssiSynth : public Synth {
   protected:
      char* label;
      void* handle;
      const DSSI_Descriptor* dssi;
      DSSI_Descriptor_Function df;
      std::vector<int> pIdx;
      std::vector<int> iIdx;
      std::vector<int> oIdx;
      int _inports, _outports, _controller;
      bool _hasGui;

   public:
      DssiSynth(const QFileInfo* fi, QString l) : Synth(fi, l) {
            df = 0;
            handle = 0;
            dssi = 0;
            _hasGui = false;
            }
      virtual ~DssiSynth() {
            delete label;
            }
      virtual void incInstances(int val);
      virtual SynthIF* createSIF(SynthI*);
      friend class DssiSynthIF;
      float defaultValue(int);
      };

//---------------------------------------------------------
//   DssiSynthIF
//    VSTi synthesizer instance
//---------------------------------------------------------

class DssiSynthIF : public SynthIF
      {
      bool _guiVisible;
      DssiSynth* synth;
      LADSPA_Handle handle;
      LadspaPort* controls;
      void* uiTarget;
      char* uiOscShowPath;
      char* uiOscControlPath;
      char* uiOscConfigurePath;
      char* uiOscProgramPath;
      char* uiOscPath;

      std::vector<DSSI_Program_Descriptor> programs;
      void queryPrograms();

   protected:
      int guiPid;

   public:
      DssiSynthIF(SynthI* s) : SynthIF(s) {
            _guiVisible = false;
            uiTarget = 0;
            uiOscShowPath = 0;
            uiOscControlPath = 0;
            uiOscConfigurePath = 0;
            uiOscProgramPath = 0;
            uiOscPath = 0;
            guiPid = -1;
            }
      virtual ~DssiSynthIF();

      virtual bool guiVisible() const;
      virtual void showGui(bool v);
      virtual bool hasGui() const { return synth->_hasGui; }
      virtual void getGeometry(int*, int*, int*, int*) const {}
      virtual void setGeometry(int, int, int, int) {}
      virtual void getData(MidiEventList*, unsigned pos, int ports, unsigned n, float** buffer) ;
      virtual bool putEvent(const MidiEvent& ev);
      virtual MidiEvent receiveEvent();
      virtual int eventsPending() const { return 0; }
      virtual int channels() const { return synth->_outports; }
      virtual void deactivate3() {}
      virtual QString getPatchName(int, int);
      virtual void populatePatchPopup(QMenu*, int);
      virtual void write(Xml& xml) const;
      virtual void setParameter(int idx, float value);
      virtual int getControllerInfo(int, const char**, int*, int*, int*) { return 0; }
      bool init(DssiSynth* s);

      int oscUpdate(lo_arg**);
      int oscProgram(lo_arg**);
      int oscControl(lo_arg**);
      int oscExiting(lo_arg**);
      int oscMidi(lo_arg**);
      int oscConfigure(lo_arg**);

      friend class DssiSynth;
      };

#endif

