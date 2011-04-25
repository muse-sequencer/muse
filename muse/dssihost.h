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

#include "config.h"

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

#include <QMenu>

#define DSSI_PARAMSAVE_VERSION_MAJOR  0
#define DSSI_PARAMSAVE_VERSION_MINOR  1
 
struct _DSSI;
class DssiPluginIF;

//class LadspaPort;
class Port;

//---------------------------------------------------------
//   DssiSynth
//---------------------------------------------------------

class DssiSynth : public Synth {
   protected:
      //char* label;
      void* handle;
      const DSSI_Descriptor* dssi;
      DSSI_Descriptor_Function df;
      unsigned long _portCount, _inports, _outports, _controlInPorts, _controlOutPorts;
      //std::vector<unsigned long> pIdx;  // Control input index to port number. 
      //std::vector<unsigned long> opIdx; // Control output index to port number. This is sometimes a latency port and...?
      std::vector<unsigned long> iIdx;  // Audio input index to port number.
      std::vector<unsigned long> oIdx;  // Audio output index to port number.
      std::vector<bool> iUsedIdx;       // During process, tells whether an audio input port was used by any input routes.
      std::vector<unsigned long> rpIdx; // Port number to control input index. Item is -1 if it's not a control input.
      //unsigned long* rpIdx;           
      MidiCtl2LadspaPortMap midiCtl2PortMap;   // Maps midi controller numbers to DSSI port numbers.
      MidiCtl2LadspaPortMap port2MidiCtlMap;   // Maps DSSI port numbers to midi controller numbers.
      bool _hasGui;
      bool _inPlaceCapable;
      // Hack: Special flag required.
      bool _isDssiVst;

   public:
      //DssiSynth(const QFileInfo* fi, QString l) : Synth(fi, l) {
      //DssiSynth(const QFileInfo& fi, QString l) : Synth(fi, l) {
      //DssiSynth(const QFileInfo& fi, QString label, QString descr, QString maker, QString ver) : 
      //    Synth(fi, label, descr, maker, ver) {
      //      rpIdx = 0;
      //      df = 0;
      //      handle = 0;
      //      dssi = 0;
      //      _hasGui = false;
      //      }
      //DssiSynth(const QFileInfo& fi, QString label, QString descr, QString maker, QString ver);  
      DssiSynth(QFileInfo&, const DSSI_Descriptor*); // removed const for QFileInfo
      virtual ~DssiSynth();
      virtual void incInstances(int);
      
      //virtual void* instantiate();
      
      virtual SynthIF* createSIF(SynthI*);
      //virtual SynthIF* createSIF();
      
      friend class DssiSynthIF;
      //float defaultValue(int); // Not required
      unsigned long inPorts()     const { return _inports; }
      unsigned long outPorts()    const { return _outports; }
      unsigned long inControls()  const { return _controlInPorts; }
      unsigned long outControls() const { return _controlOutPorts; }
      
      //unsigned long inControlPortIdx(unsigned long i) { return pIdx[i]; }
      };

//---------------------------------------------------------
//   DssiSynthIF
//    VSTi synthesizer instance
//---------------------------------------------------------

//class DssiSynthIF : public SynthIF 
class DssiSynthIF : public SynthIF, public PluginIBase
      {
      //bool _guiVisible;
      DssiSynth* synth;
      LADSPA_Handle handle;
      
      //LadspaPort* controls;
      Port* controls;
      Port* controlsOut;
      
      //unsigned long _curBank;
      //unsigned long _curProgram;
      
      #ifdef OSC_SUPPORT
      OscDssiIF _oscif;
      #endif

      //void* uiTarget;
      //char* uiOscShowPath;
      //char* uiOscControlPath;
      //char* uiOscConfigurePath;
      //char* uiOscProgramPath;
      //char* uiOscPath;

      std::vector<DSSI_Program_Descriptor> programs;
      void queryPrograms();
      bool processEvent(const MidiPlayEvent&, snd_seq_event_t*);
      
      float** audioInBuffers;
      float** audioOutBuffers;
      
   protected:
      //int guiPid;
      //QProcess* guiQProc;

   public:
      DssiSynthIF(SynthI* s);
      //DssiSynthIF();
      
      virtual ~DssiSynthIF();

      virtual DssiSynth* dssiSynth() { return synth; }
      virtual SynthI* dssiSynthI()   { return synti; }
      
      virtual bool initGui();
      virtual void guiHeartBeat();
      virtual bool guiVisible() const;
      virtual void showGui(bool);
      //virtual bool hasGui() const { return synth->_hasGui; }
      virtual bool hasGui() const { return true; }
      virtual bool nativeGuiVisible() const;                                        // p4.0.20
      virtual void showNativeGui(bool);                                             // 
      //virtual bool hasNativeGui() const { /*return synth->_hasGui; */}            // 
      virtual bool hasNativeGui() const { return !dssi_ui_filename().isEmpty(); }   // 
      virtual void getGeometry(int*x, int*y, int*w, int*h) const { *x=0;*y=0;*w=0;*h=0; }
      virtual void setGeometry(int, int, int, int) {}
      virtual void getNativeGeometry(int*x, int*y, int*w, int*h) const { *x=0;*y=0;*w=0;*h=0; }
      virtual void setNativeGeometry(int, int, int, int) {}
      
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
      virtual void populatePatchPopup(QMenu*, int, MType, bool);
      
      //virtual void write(Xml& xml) const;
      virtual void write(int level, Xml& xml) const;
      
      virtual float getParameter(unsigned long /*idx*/) const;
      virtual void setParameter(unsigned long /*idx*/, float /*value*/);
      
      //virtual int getControllerInfo(int, const char**, int*, int*, int*) { return 0; }
      virtual int getControllerInfo(int, const char**, int*, int*, int*, int*);
      
      bool init(DssiSynth* s);

      //StringParamMap& stringParameters() { return synti->stringParameters(); }

      #ifdef OSC_SUPPORT
      OscDssiIF& oscIF() { return _oscif; }
      /*
      int oscProgram(lo_arg**);
      int oscControl(lo_arg**);
      int oscMidi(lo_arg**);
      int oscConfigure(lo_arg**);
      int oscUpdate(lo_arg**);
      //int oscExiting(lo_arg**);
      */
      
      int oscProgram(unsigned long /*prog*/, unsigned long /*bank*/);
      int oscControl(unsigned long /*dssiPort*/, float /*val*/);
      int oscMidi(int /*a*/, int /*b*/, int /*c*/);
      int oscConfigure(const char */*key*/, const char */*val*/);
      int oscUpdate();
      //int oscExiting();
      #endif

      //-------------------------
      // Methods for PluginIBase:
      //-------------------------
      bool on() const;       
      void setOn(bool /*val*/);   
      //int pluginID();
      unsigned pluginID();        // p4.0.21
      int id();
      QString pluginLabel() const;  
      QString name() const;
      QString lib() const;            
      QString dirPath() const;
      QString fileName() const;
      AudioTrack* track();          
      //void enableController(int /*i*/, bool v = true); 
      //bool controllerEnabled(int /*i*/) const;          
      //bool controllerEnabled2(int /*i*/) const;          
      void enableController(unsigned /*i*/, bool v = true);      // p4.0.21
      bool controllerEnabled(unsigned /*i*/) const;          
      bool controllerEnabled2(unsigned /*i*/) const;          
      void updateControllers();
      void writeConfiguration(int /*level*/, Xml& /*xml*/);
      bool readConfiguration(Xml& /*xml*/, bool readPreset=false);
      //int parameters() const;          
      //void setParam(int /*i*/, double /*val*/); 
      //double param(int /*i*/) const;        
      //const char* paramName(int /*i*/);     
      //LADSPA_PortRangeHint range(int /*i*/); 
      unsigned parameters() const;                            // p4.0.21
      void setParam(unsigned /*i*/, float /*val*/); 
      float param(unsigned /*i*/) const;        
      const char* paramName(unsigned /*i*/);     
      LADSPA_PortRangeHint range(unsigned /*i*/); 

      friend class DssiSynth;
      };

extern void initDSSI();

#endif

