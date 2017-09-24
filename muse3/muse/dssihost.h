//=============================================================================
//  MusE
//  Linux Music Editor
//  $Id: dssihost.h,v 1.10.2.7 2009/12/06 10:05:00 terminator356 Exp $
//
//  Copyright (C) 1999-2011 by Werner Schweer and others
//  (C) Copyright 2011-2013 Tim E. Real (terminator356 on sourceforge)
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License
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
//=============================================================================

#ifndef __DSSIHOST_H__
#define __DSSIHOST_H__

#include "config.h"

// Make sure this number is unique among all the MESS synths (including ticksynth) and DSSI, VST, LV2 and other host synths.
// 127 is reserved for special MusE system messages.
#define DSSI_SYNTH_UNIQUE_ID 8
// Midistate sysex initialization command.
#define DSSI_INIT_DATA_CMD 1

#define DSSI_PARAMSAVE_VERSION_MAJOR  0
#define DSSI_PARAMSAVE_VERSION_MINOR  1


#ifdef DSSI_SUPPORT

#include <vector>
#include <map>
#include <string>

#ifdef OSC_SUPPORT
#include <lo/lo.h>
#include "osc.h"
#endif

#include <ladspa.h>
#include <dssi.h>

#include <alsa/asoundlib.h>

#include "midictrl.h"
#include "synth.h"
#include "stringparam.h"

#include "plugin.h"

//#include <QMenu>
#include "popupmenu.h"

#include "lock_free_buffer.h"

#endif // DSSI_SUPPORT

namespace MusECore {
 
#ifdef DSSI_SUPPORT

struct _DSSI;
class DssiPluginIF;

struct Port;

//---------------------------------------------------------
//   DssiSynth
//---------------------------------------------------------

class DssiSynth : public Synth {
   protected:
      void* handle;
      const DSSI_Descriptor* dssi;
      DSSI_Descriptor_Function df;
      unsigned long _portCount, _inports, _outports, _controlInPorts, _controlOutPorts;
      std::vector<unsigned long> iIdx;  // Audio input index to port number.
      std::vector<unsigned long> oIdx;  // Audio output index to port number.
      std::vector<unsigned long> rpIdx; // Port number to control input index. Item is -1 if it's not a control input.
      MidiCtl2LadspaPortMap midiCtl2PortMap;   // Maps midi controller numbers to DSSI port numbers.
      MidiCtl2LadspaPortMap port2MidiCtlMap;   // Maps DSSI port numbers to midi controller numbers.
      bool _hasGui;
      // Hack: Special flag required.
      bool _isDssiVst;

   public:
      DssiSynth(QFileInfo&, const DSSI_Descriptor*, bool isDssiVst = false, 
                Plugin::PluginFeatures reqFeatures = Plugin::NoFeatures); // removed const for QFileInfo
      virtual ~DssiSynth();
      virtual Type synthType() const { return DSSI_SYNTH; }

      virtual void incInstances(int);
      virtual SynthIF* createSIF(SynthI*);
      
      friend class DssiSynthIF;
      unsigned long inPorts()     const { return _inports; }
      unsigned long outPorts()    const { return _outports; }
      unsigned long inControls()  const { return _controlInPorts; }
      unsigned long outControls() const { return _controlOutPorts; }

      bool isDssiVst()            const { return _isDssiVst; }
      
      const std::vector<unsigned long>* getRpIdx() { return &rpIdx; }
      };

//---------------------------------------------------------
//   DssiSynthIF
//    VSTi synthesizer instance
//---------------------------------------------------------

class DssiSynthIF : public SynthIF
      {
      DssiSynth* _synth;
      LADSPA_Handle _handle;
      
      Port* _controls;
      Port* _controlsOut;
      bool          _hasLatencyOutPort;
      unsigned long _latencyOutPort;
      
      #ifdef OSC_SUPPORT
      OscDssiIF _oscif;
      #endif

      // Fifo for midi events sent from OSC to audio thread (ex. updating hardware knobs/sliders):
      LockFreeBuffer<MidiPlayEvent> *_osc2AudioFifo;
      
      std::vector<DSSI_Program_Descriptor> programs;
      void queryPrograms();
      void doSelectProgram(LADSPA_Handle handle, int bank, int prog); 
      bool processEvent(const MidiPlayEvent&, snd_seq_event_t*);
      
      float** _audioInBuffers;
      float** _audioOutBuffers;
      float*  _audioInSilenceBuf; // Just all zeros all the time, so we don't have to clear for silence.
      
   public:
      DssiSynthIF(SynthI* s);
      
      // This is only a kludge required to support old songs' midistates. Do not use in any new synth.
      virtual int oldMidiStateHeader(const unsigned char** data) const;
      
      virtual ~DssiSynthIF();

      bool init(DssiSynth* s);

      virtual DssiSynth* dssiSynth() { return _synth; }
      virtual SynthI* dssiSynthI()   { return synti; }
      
      virtual bool initGui();
      virtual void guiHeartBeat();
      virtual bool guiVisible() const;
      virtual void showGui(bool);
      virtual bool hasGui() const { return true; }
      virtual bool nativeGuiVisible() const;                                        
      virtual void showNativeGui(bool);                                              
      virtual bool hasNativeGui() const { return !dssi_ui_filename().isEmpty(); }    
      virtual void getGeometry(int*x, int*y, int*w, int*h) const;
      virtual void setGeometry(int, int, int, int);
      virtual void getNativeGeometry(int*x, int*y, int*w, int*h) const { *x=0;*y=0;*w=0;*h=0; }
      virtual void setNativeGeometry(int, int, int, int) {}
      
      virtual void preProcessAlways();
      virtual iMPEvent getData(MidiPort*, MPEventList*, iMPEvent, unsigned pos, int ports, unsigned n, float** buffer);
      virtual bool putEvent(const MidiPlayEvent& ev);
      virtual MidiPlayEvent receiveEvent();
      virtual int eventsPending() const { return 0; }
      
      virtual int channels() const;
      virtual int totalOutChannels() const;
      virtual int totalInChannels() const;
      
      virtual void deactivate3();
      
      virtual QString getPatchName(int, int, bool) const;
      virtual void populatePatchPopup(MusEGui::PopupMenu*, int, bool);
      
      virtual void write(int level, Xml& xml) const;
      
      virtual double getParameter(unsigned long /*idx*/) const;
      virtual double getParameterOut(unsigned long n) const;
      virtual void setParameter(unsigned long /*idx*/, double /*value*/);
      virtual int getControllerInfo(int, QString*, int*, int*, int*, int*);
      
      #ifdef OSC_SUPPORT
      OscDssiIF& oscIF() { return _oscif; }
      int oscProgram(unsigned long prog, unsigned long bank);
      int oscControl(unsigned long dssiPort, float val);
      int oscMidi(int a, int b, int c);
      int oscConfigure(const char *key, const char *val);
      int oscUpdate();
      #endif

      //-------------------------
      // Methods for PluginIBase:
      //-------------------------
      
      unsigned long pluginID();        
      int id();
      QString pluginLabel() const;  
      QString lib() const;            
      QString dirPath() const;
      QString fileName() const;
      void enableController(unsigned long i, bool v = true);      
      bool controllerEnabled(unsigned long i) const;          
      void enableAllControllers(bool v = true);
      void updateControllers();
      void activate();
      void deactivate();

      unsigned long parameters() const;                            
      unsigned long parametersOut() const;
      void setParam(unsigned long i, double val); 
      double param(unsigned long i) const;        
      double paramOut(unsigned long i) const;        
      const char* paramName(unsigned long i);     
      const char* paramOutName(unsigned long i);
      LADSPA_PortRangeHint range(unsigned long i);
      LADSPA_PortRangeHint rangeOut(unsigned long i);
      float latency();
      CtrlValueType ctrlValueType(unsigned long i) const; 
      CtrlList::Mode ctrlMode(unsigned long i) const; 

      friend class DssiSynth;
      };

#endif // DSSI_SUPPORT
      
extern void initDSSI();

} // namespace MusECore

#endif

