//=============================================================================
//  MusE
//  Linux Music Editor
//  $Id: dssihost.h,v 1.10.2.7 2009/12/06 10:05:00 terminator356 Exp $
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

#include <vector>

#include <lo/lo.h>
#include <dssi.h>
#include <alsa/asoundlib.h>

#include "midictrl.h"
#include "synth.h"
#include "plugin.h"

struct _DSSI;
class DssiPluginIF;

//class LadspaPort;
class Port;
class QProcess;

//---------------------------------------------------------
//   DssiSynth
//---------------------------------------------------------

class DssiSynth : public Synth {
   protected:
      //char* label;
      void* handle;
      const DSSI_Descriptor* dssi;
      DSSI_Descriptor_Function df;
      std::vector<int> pIdx;  // Control input index to port number. 
      std::vector<int> opIdx; // Control output index to port number. This is sometimes a latency port and...?
      std::vector<int> iIdx;  // Audio input index to port number.
      std::vector<int> oIdx;  // Audio output index to port number.
      std::vector<bool> iUsedIdx; // This is for audio input ports during process to tell whether an audio input port was used by any input routes.
      int _inports, _outports, _controller, _controllerOut;
      std::vector<int> rpIdx;  // Port number to control input index. Item is -1 if it's not a control input.
      MidiCtl2LadspaPortMap midiCtl2PortMap;   // Maps midi controller numbers to DSSI port numbers.
      MidiCtl2LadspaPortMap port2MidiCtlMap;   // Maps DSSI port numbers to midi controller numbers.
      bool _hasGui;

   public:
      //DssiSynth(const QFileInfo* fi, QString l) : Synth(fi, l) {
      //DssiSynth(const QFileInfo& fi, QString l) : Synth(fi, l) {
      DssiSynth(const QFileInfo& fi, QString label, QString descr, QString maker, QString ver) : 
          Synth(fi, label, descr, maker, ver) {
            df = 0;
            handle = 0;
            dssi = 0;
            _hasGui = false;
            }
      virtual ~DssiSynth() {
            //delete label;
            }
      virtual void incInstances(int val);
      
      //virtual void* instantiate();
      
      virtual SynthIF* createSIF(SynthI*);
      //virtual SynthIF* createSIF();
      
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
      
      //LadspaPort* controls;
      Port* controls;
      Port* controlsOut;
      
      void* uiTarget;
      char* uiOscShowPath;
      char* uiOscControlPath;
      char* uiOscConfigurePath;
      char* uiOscProgramPath;
      char* uiOscPath;

      std::vector<DSSI_Program_Descriptor> programs;
      void queryPrograms();
      bool processEvent(const MidiPlayEvent&, snd_seq_event_t*);
      
      float** audioInBuffers;
      float** audioOutBuffers;
      
   protected:
      //int guiPid;
      QProcess* guiQProc;

   public:
      DssiSynthIF(SynthI* s);
      //DssiSynthIF();
      
      virtual ~DssiSynthIF();

      virtual bool startGui();
      virtual bool guiVisible() const;
      virtual void showGui(bool v);
      virtual bool hasGui() const { return synth->_hasGui; }
      virtual void getGeometry(int*, int*, int*, int*) const {}
      virtual void setGeometry(int, int, int, int) {}
      
      virtual void preProcessAlways();
      
      //virtual void getData(MidiEventList*, unsigned pos, int ports, unsigned n, float** buffer) ;
      virtual iMPEvent getData(MidiPort*, MPEventList*, iMPEvent, unsigned pos, int ports, unsigned n, float** buffer);
      
      //virtual bool putEvent(const MidiEvent& ev);
      virtual bool putEvent(const MidiPlayEvent& ev);
      
      //virtual MidiEvent receiveEvent();
      virtual MidiPlayEvent receiveEvent();
      
      virtual int eventsPending() const { return 0; }
      
      //virtual int channels() const { return synth->_outports; }
      virtual int channels() const;
      virtual int totalOutChannels() const;
      virtual int totalInChannels() const;
      
      virtual void deactivate3() {}
      
      //virtual QString getPatchName(int, int);
      virtual const char* getPatchName(int, int, int, bool) const { return ""; }
      virtual const char* getPatchName(int, int, MType, bool);
      
      //virtual void populatePatchPopup(QMenu*, int);
      virtual void populatePatchPopup(QPopupMenu*, int, MType, bool);
      
      //virtual void write(Xml& xml) const;
      virtual void write(int level, Xml& xml) const;
      
      virtual void setParameter(int idx, float value);
      
      //virtual int getControllerInfo(int, const char**, int*, int*, int*) { return 0; }
      virtual int getControllerInfo(int, const char**, int*, int*, int*, int*);
      
      bool init(DssiSynth* s);

      int oscUpdate(lo_arg**);
      int oscProgram(lo_arg**);
      int oscControl(lo_arg**);
      int oscExiting(lo_arg**);
      int oscMidi(lo_arg**);
      int oscConfigure(lo_arg**);

      friend class DssiSynth;
      };

extern void initDSSI();

#endif

