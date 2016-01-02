//=========================================================
//  MusE
//  Linux Music Editor
//
//  vst_native.h
//  (C) Copyright 2012-2013 Tim E. Real (terminator356 on users dot sourceforge dot net)
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

#ifndef __VST_NATIVE_H__
#define __VST_NATIVE_H__

#include "config.h"

// Make sure this number is unique among all the MESS synths and DSSI and VST host synths.
#define VST_NATIVE_SYNTH_UNIQUE_ID 9
// Midistate sysex initialization command.
#define VST_NATIVE_INIT_DATA_CMD 1

#define VST_NATIVE_PARAMSAVE_VERSION_MAJOR  0
#define VST_NATIVE_PARAMSAVE_VERSION_MINOR  1

#define VST_NATIVE_CHUNK_FLAG_COMPRESSED 1

#ifdef VST_NATIVE_SUPPORT

class VstNativeSynthIF;

typedef class VstNativeSynthIF VSTPlugin;

#ifdef VST_SDK_QUIRK
#define __cdecl
#endif

#include "aeffectx.h"

#ifdef VST_VESTIGE_SUPPORT
#ifndef effGetProgramNameIndexed
#define effGetProgramNameIndexed 29
#endif
#endif

#ifndef VST_2_4_EXTENSIONS
#ifndef VST_VESTIGE_SUPPORT
typedef long     VstInt32;
typedef long     VstIntPtr;
#else
typedef int32_t  VstInt32;
typedef intptr_t VstIntPtr;
#define VSTCALLBACK
#endif
#endif

#include "vst_native_editor.h"
#include "synth.h"
#include "plugin.h"
#include "midictrl.h"

#include <semaphore.h>

#endif // VST_NATIVE_SUPPORT

namespace MusEGui {
class PopupMenu;
}

namespace MusECore {

#ifdef VST_NATIVE_SUPPORT

struct VstRect{
    short top;
    short left;
    short bottom;
    short right;
};

struct VST_Program {
    //unsigned long bank;
    unsigned long program;
    QString name;
};


//---------------------------------------------------------
//   VstNativeSynth
//---------------------------------------------------------

class VstNativeSynth : public Synth {
      enum VstPluginFlags
      {
        canSendVstEvents          = 1 << 0,
        canSendVstMidiEvents      = 1 << 1,
        canSendVstTimeInfo        = 1 << 2,
        canReceiveVstEvents       = 1 << 3,
        canReceiveVstMidiEvents   = 1 << 4,
        canReceiveVstTimeInfo     = 1 << 5,
        canProcessOffline         = 1 << 6,
        canUseAsInsert            = 1 << 7,
        canUseAsSend              = 1 << 8,
        canMixDryWet              = 1 << 9,
        canMidiProgramNames       = 1 << 10
      };

      void* _handle;
      int _vst_version;
      unsigned int _flags;
      VstIntPtr _id;
      
      unsigned long /*_portCount,*/ _inports, _outports, _controlInPorts; //, _controlOutPorts;
      std::vector<unsigned long> iIdx;  // Audio input index to port number.
      std::vector<unsigned long> oIdx;  // Audio output index to port number.
      std::vector<unsigned long> rpIdx; // Port number to control input index. Item is -1 if it's not a control input.
      MidiCtl2LadspaPortMap midiCtl2PortMap;   // Maps midi controller numbers to vst port numbers.
      MidiCtl2LadspaPortMap port2MidiCtlMap;   // Maps vst port numbers to midi controller numbers.
      bool _hasGui;
      bool _inPlaceCapable;
      bool _hasChunks;
      
   public:
      VstNativeSynth(const QFileInfo& fi, AEffect* plugin, const QString& label, const QString& desc, const QString& maker, const QString& ver, VstIntPtr id, void *dlHandle);

      virtual ~VstNativeSynth() {}
      virtual Type synthType() const { return VST_NATIVE_SYNTH; }
      virtual void incInstances(int val);
      virtual AEffect* instantiate(VstNativeSynthIF*);
      virtual SynthIF* createSIF(SynthI*);
      unsigned long inPorts()     const { return _inports; }
      unsigned long outPorts()    const { return _outports; }
      unsigned long inControls()  const { return _controlInPorts; }
      //unsigned long outControls() const { return _controlOutPorts; }

      int vstVersion()  const { return _vst_version; }
      bool hasChunks()  const { return _hasChunks; }
      const std::vector<unsigned long>* getRpIdx() { return &rpIdx; }
      };

//---------------------------------------------------------
//   VstNativeGuiWidgets
//---------------------------------------------------------

struct VstNativeGuiWidgets {
      // TODO: Remove Tim. Or keep.
      //enum {
      //      SLIDER, DOUBLE_LABEL, QCHECKBOX, QCOMBOBOX
      //      };
      //QWidget* widget;
      //int type;
      //unsigned long param;
      bool pressed;
      };

//---------------------------------------------------------
//   VstNativeSynthIF
//    VSTi synthesizer instance
//---------------------------------------------------------

class VstNativeSynthIF : public SynthIF
      {
      friend class VstNativeSynth;
      friend class MusEGui::VstNativeEditor;
      
      VstNativeSynth* _synth;
      AEffect* _plugin;
      bool _active;    // Whether it's safe to call effIdle or effEditIdle. 
      MusEGui::VstNativeEditor* _editor;
      bool _guiVisible;
      bool _inProcess; // To inform the callback of the 'process level' - are we in the audio thread?      

      // Struct array to keep track of pressed flags and so on. // TODO: Not used yet. REMOVE Tim. Or keep.
      VstNativeGuiWidgets* _gw;
      Port* _controls;
      float** _audioOutBuffers;
      float** _audioInBuffers;
      std::vector<unsigned long> _iUsedIdx;  // During process, tells whether an audio input port was used by any input routes.
      float*  _audioInSilenceBuf;            // Just all zeros all the time, so we don't have to clear for silence.

      std::vector<VST_Program> programs;
      void queryPrograms();
      void doSelectProgram(int bankH, int bankL, int prog);
      bool processEvent(const MidiPlayEvent&, VstMidiEvent*);
      void setVstEvent(VstMidiEvent* event, int a = 0, int b = 0, int c = 0, int d = 0);
      
      void editorDeleted();
      void editorOpened();
      void editorClosed();
      
   public:
      VstNativeSynthIF(SynthI* s);
      virtual ~VstNativeSynthIF();

      virtual bool init(Synth*);
      
      AEffect* plugin() const { return _plugin; }
      VstIntPtr hostCallback(VstInt32 opcode, VstInt32 index, VstIntPtr value, void* ptr, float opt);
      VstIntPtr dispatch(VstInt32 opcode, VstInt32 index, VstIntPtr value, void* ptr, float opt) const {
                  if(_plugin) return _plugin->dispatcher(_plugin, opcode, index, value, ptr, opt); return 0;  }
      void idleEditor();
      bool resizeEditor(int w, int h);
      
      virtual bool initGui()       { return true; };
      virtual void guiHeartBeat();
      virtual bool guiVisible() const;
      virtual void showGui(bool);
      virtual bool hasGui() const { return true; }
      virtual bool nativeGuiVisible() const;
      virtual void showNativeGui(bool v);
      virtual bool hasNativeGui() const;
      virtual void getGeometry(int*x, int*y, int*w, int*h) const;
      virtual void setGeometry(int, int, int, int);
      virtual void getNativeGeometry(int*x, int*y, int*w, int*h) const ;
      virtual void setNativeGeometry(int, int, int, int);
      virtual void preProcessAlways() { };
      virtual iMPEvent getData(MidiPort*, MPEventList*, iMPEvent, unsigned pos, int ports, unsigned nframes, float** buffer) ;
      virtual bool putEvent(const MidiPlayEvent& ev);
      virtual MidiPlayEvent receiveEvent();
      virtual int eventsPending() const { return 0; }
      virtual int channels() const;
      virtual int totalOutChannels() const;
      virtual int totalInChannels() const;
      virtual void deactivate3();
      virtual QString getPatchName(int chan, int prog, bool drum) const;
      virtual void populatePatchPopup(MusEGui::PopupMenu* menu, int chan, bool drum);
      virtual void write(int level, Xml& xml) const;
      virtual float getParameter(unsigned long idx) const;
      virtual void setParameter(unsigned long idx, float value);
      virtual int getControllerInfo(int, const char**, int*, int*, int*, int*) { return 0; }

      virtual void guiAutomationBegin(unsigned long param_idx);
      virtual void guiAutomationEnd(unsigned long param_idx);
      virtual int guiControlChanged(unsigned long param_idx, float value);

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
      void setParam(unsigned long i, float val);
      float param(unsigned long i) const;
      float paramOut(unsigned long i) const;
      const char* paramName(unsigned long i);
      const char* paramOutName(unsigned long i);
      LADSPA_PortRangeHint range(unsigned long i);
      LADSPA_PortRangeHint rangeOut(unsigned long i);
      CtrlValueType ctrlValueType(unsigned long i) const;
      CtrlList::Mode ctrlMode(unsigned long i) const;
      };

#endif // VST_NATIVE_SUPPORT

extern void initVST_Native();

} // namespace MusECore

#endif

