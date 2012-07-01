//=============================================================================
//  MusE
//  Linux Music Editor
//  $Id: dssihost.h,v 1.10.2.7 2009/12/06 10:05:00 terminator356 Exp $
//
//  Copyright (C) 1999-2011 by Werner Schweer and others
//  (C) Copyright 2011 Tim E. Real (terminator356 on sourceforge)
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

// Make sure this number is unique among all the MESS synths and DSSI host synth.
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

#include "ladspa.h"
#include <dssi.h>

#include <alsa/asoundlib.h>

#include "midictrl.h"
#include "synth.h"
#include "stringparam.h"

#include "plugin.h"

//#include <QMenu>
#include "popupmenu.h"

#endif // DSSI_SUPPORT

namespace MusECore {
 
#ifdef DSSI_SUPPORT

struct _DSSI;
class DssiPluginIF;

class Port;

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
      std::vector<int> iUsedIdx;       // During process, tells whether an audio input port was used by any input routes.
      std::vector<unsigned long> rpIdx; // Port number to control input index. Item is -1 if it's not a control input.
      MusECore::MidiCtl2LadspaPortMap midiCtl2PortMap;   // Maps midi controller numbers to DSSI port numbers.
      MusECore::MidiCtl2LadspaPortMap port2MidiCtlMap;   // Maps DSSI port numbers to midi controller numbers.
      bool _hasGui;
      bool _inPlaceCapable;
      // Hack: Special flag required.
      bool _isDssiVst;

   public:
      DssiSynth(QFileInfo&, const DSSI_Descriptor*); // removed const for QFileInfo
      virtual ~DssiSynth();
      virtual Type synthType() const { return DSSI_SYNTH; }

      virtual void incInstances(int);
      virtual SynthIF* createSIF(SynthI*);
      
      friend class DssiSynthIF;
      unsigned long inPorts()     const { return _inports; }
      unsigned long outPorts()    const { return _outports; }
      unsigned long inControls()  const { return _controlInPorts; }
      unsigned long outControls() const { return _controlOutPorts; }
      
      const std::vector<unsigned long>* getRpIdx() { return &rpIdx; }
      };

//---------------------------------------------------------
//   DssiSynthIF
//    VSTi synthesizer instance
//---------------------------------------------------------

class DssiSynthIF : public SynthIF, public PluginIBase
      {
      DssiSynth* synth;
      LADSPA_Handle handle;
      
      Port* controls;
      Port* controlsOut;
      
      #ifdef OSC_SUPPORT
      OscDssiIF _oscif;
      #endif

      std::vector<DSSI_Program_Descriptor> programs;
      void queryPrograms();
      void doSelectProgram(LADSPA_Handle handle, int bank, int prog); 
      bool processEvent(const MusECore::MidiPlayEvent&, snd_seq_event_t*);
      
      float** audioInBuffers;
      float** audioOutBuffers;
      float*  audioInSilenceBuf; // Just all zeros all the time, so we don't have to clear for silence.
      
   public:
      DssiSynthIF(SynthI* s);
      
      // This is only a kludge required to support old songs' midistates. Do not use in any new synth.
      virtual int oldMidiStateHeader(const unsigned char** data) const;
      
      virtual ~DssiSynthIF();

      virtual DssiSynth* dssiSynth() { return synth; }
      virtual SynthI* dssiSynthI()   { return synti; }
      
      virtual bool initGui();
      virtual void guiHeartBeat();
      virtual bool guiVisible() const;
      virtual void showGui(bool);
      virtual bool hasGui() const { return true; }
      virtual bool nativeGuiVisible() const;                                        
      virtual void showNativeGui(bool);                                              
      virtual bool hasNativeGui() const { return !dssi_ui_filename().isEmpty(); }    
      virtual void getGeometry(int*x, int*y, int*w, int*h) const { *x=0;*y=0;*w=0;*h=0; }
      virtual void setGeometry(int, int, int, int) {}
      virtual void getNativeGeometry(int*x, int*y, int*w, int*h) const { *x=0;*y=0;*w=0;*h=0; }
      virtual void setNativeGeometry(int, int, int, int) {}
      
      virtual void preProcessAlways();
      virtual MusECore::iMPEvent getData(MusECore::MidiPort*, MusECore::MPEventList*, MusECore::iMPEvent, unsigned pos, int ports, unsigned n, float** buffer);
      virtual bool putEvent(const MusECore::MidiPlayEvent& ev);
      virtual MusECore::MidiPlayEvent receiveEvent();
      virtual int eventsPending() const { return 0; }
      
      virtual int channels() const;
      virtual int totalOutChannels() const;
      virtual int totalInChannels() const;
      
      virtual void deactivate3() {}
      
      virtual const char* getPatchName(int, int, int, bool) const { return ""; }
      virtual const char* getPatchName(int, int, MType, bool);
      virtual void populatePatchPopup(MusEGui::PopupMenu*, int, MType, bool);
      
      virtual void write(int level, Xml& xml) const;
      
      virtual float getParameter(unsigned long /*idx*/) const;
      virtual float getParameterOut(unsigned long n) const;
      virtual void setParameter(unsigned long /*idx*/, float /*value*/);
      
      virtual int getControllerInfo(int, const char**, int*, int*, int*, int*);
      
      bool init(DssiSynth* s);

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
      bool on() const;       
      void setOn(bool val);   
      unsigned long pluginID();        
      int id();
      QString pluginLabel() const;  
      QString name() const;
      QString lib() const;            
      QString dirPath() const;
      QString fileName() const;
      QString titlePrefix() const;
      MusECore::AudioTrack* track();          
      void enableController(unsigned long i, bool v = true);      
      bool controllerEnabled(unsigned long i) const;          
      void enable2Controller(unsigned long i, bool v = true);      
      bool controllerEnabled2(unsigned long i) const;          
      void enableAllControllers(bool v = true);
      void enable2AllControllers(bool v = true);
      void updateControllers();
      void writeConfiguration(int level, Xml& xml);
      bool readConfiguration(Xml& xml, bool readPreset=false);

      unsigned long parameters() const;                            
      unsigned long parametersOut() const;
      void setParam(unsigned long i, float val); 
      float param(unsigned long i) const;        
      float paramOut(unsigned long i) const;        
      const char* paramName(unsigned long i);     
      const char* paramOutName(unsigned long i);
      LADSPA_PortRangeHint range(unsigned long i);
      LADSPA_PortRangeHint rangeOut(unsigned long i);
      CtrlValueType ctrlValueType(unsigned long i) const; 
      CtrlList::Mode ctrlMode(unsigned long i) const; 

      friend class DssiSynth;
      };

#endif // DSSI_SUPPORT
      
extern void initDSSI();

} // namespace MusECore

#endif

