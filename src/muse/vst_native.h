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

// Make sure this number is unique among all the MESS synths (including ticksynth) and DSSI, VST, LV2 and other host synths.
// 127 is reserved for special MusE system messages.
#define VST_NATIVE_SYNTH_UNIQUE_ID 9
// Midistate sysex initialization command.
#define VST_NATIVE_INIT_DATA_CMD 1

#define VST_NATIVE_PARAMSAVE_VERSION_MAJOR  0
#define VST_NATIVE_PARAMSAVE_VERSION_MINOR  1

#define VST_NATIVE_CHUNK_FLAG_COMPRESSED 1

#ifdef VST_NATIVE_SUPPORT

#include "plugin_list.h"
#include "globaldefs.h"

class VstNativeSynthIF;

typedef class VstNativeSynthIF VSTPlugin;

#ifdef VST_SDK_QUIRK_DEF
#ifndef __cdecl
#define __cdecl
#endif // __cdecl
#endif

#include "aeffectx.h"

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

#include <vector>
#include <QString>
#include <QFileInfo>

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

class VstNativePluginWrapper;
class VstNativePluginWrapper_State;
class VstNativeSynthIF;

struct VstNativeSynthOrPlugin
{
   VstNativeSynthIF *sif;
   VstNativePluginWrapper_State *pstate;
};

class VstNativeSynth : public Synth {
   friend class VstNativePluginWrapper;
      void* _handle;
      int _vst_version;
      VstPluginFlags_t _flags;
      VstIntPtr _id;
      bool _isSynth;
      bool _usesTransportSource;

      unsigned long /*_portCount,*/ _inports, _outports, _controlInPorts; //, _controlOutPorts;
      std::vector<unsigned long> iIdx;  // Audio input index to port number.
      std::vector<unsigned long> oIdx;  // Audio output index to port number.
      std::vector<unsigned long> rpIdx; // Port number to control input index. Item is -1 if it's not a control input.
      bool _hasGui;
      bool _hasChunks;

   public:
      VstNativeSynth(const MusEPlugin::PluginScanInfoStruct& info);
      virtual ~VstNativeSynth() {}

      virtual Type synthType() const { return _isSynth ? VST_NATIVE_SYNTH : VST_NATIVE_EFFECT; }
      virtual void incInstances(int val);
      virtual AEffect* instantiate(void *userData);
      // Opens a plugin instance, after instantiation.
      static bool openPlugin(AEffect* plugin);
      virtual SynthIF* createSIF(SynthI*);
      unsigned long inPorts()     const { return _inports; }
      unsigned long outPorts()    const { return _outports; }
      unsigned long inControls()  const { return _controlInPorts; }

      int vstVersion()  const { return _vst_version; }
      bool hasChunks()  const { return _hasChunks; }
      const std::vector<unsigned long>* getRpIdx() { return &rpIdx; }
      bool isSynth() { return _isSynth; }
      bool usesTransportSource() const { return _usesTransportSource; }

      static VstIntPtr pluginHostCallback(VstNativeSynthOrPlugin *userData, VstInt32 opcode, VstInt32 index, VstIntPtr value, void* ptr, float opt);
      static int guiControlChanged(VstNativeSynthOrPlugin *userData, unsigned long param_idx, float value);
      static void guiAutomationBegin(VstNativeSynthOrPlugin *userData, unsigned long param_idx);
      static void guiAutomationEnd(VstNativeSynthOrPlugin *userData, unsigned long param_idx);
      static bool resizeEditor(MusEGui::VstNativeEditor *editor, int w, int h);

// REMOVE Tim. tmp. Added.
      QString getCustomConfiguration(AEffect *plugin);
// REMOVE Tim. tmp. Changed.
//      void vstconfWrite(AEffect *plugin, const QString& name, int level, Xml &xml);
      void vstconfWrite(AEffect *plugin, const QString& label, int level, Xml &xml);
      void vstconfSet(AEffect *plugin, const std::vector<QString> & customParams);

      // Enables or disables the plugin, if it has such as function.
      void setPluginEnabled(AEffect *plugin, bool en);
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
      MusEGui::VstNativeEditor* _editor;
      bool _guiVisible;
      bool _inProcess; // To inform the callback of the 'process level' - are we in the audio thread?

      // Struct array to keep track of pressed flags and so on. // TODO: Not used yet. REMOVE Tim. Or keep.
      VstNativeGuiWidgets* _gw;
      Port* _controls;
      float** _audioOutBuffers;
      float** _audioInBuffers;
      float*  _audioInSilenceBuf;            // Just all zeros all the time, so we don't have to clear for silence.

      VstNativeSynthOrPlugin userData;

      // Temporary variable holds value to be passed to the callback routine.
      float _transportLatencyCorr;

      std::vector<VST_Program> programs;
      void queryPrograms();
      void doSelectProgram(int bankH, int bankL, int prog);
      bool processEvent(const MidiPlayEvent&, VstMidiEvent*);
      void setVstEvent(VstMidiEvent* event, int a = 0, int b = 0, int c = 0, int d = 0);

      void editorDeleted();
      void editorOpened();
      void editorClosed();

      void eventReceived(VstMidiEvent*);

   protected:
      void activate() override;
      void deactivate() override;

   public:
      VstNativeSynthIF(SynthI* s);
      virtual ~VstNativeSynthIF();

      virtual bool init(Synth*);

      AEffect* plugin() const { return _plugin; }
      VstIntPtr dispatch(VstInt32 opcode, VstInt32 index, VstIntPtr value, void* ptr, float opt) const {
                  if(_plugin) {return _plugin->dispatcher(_plugin, opcode, index, value, ptr, opt); } return 0;  }
      void idleEditor();

      virtual void guiHeartBeat() override;
      virtual bool hasGui() const override { return true; }
      virtual bool nativeGuiVisible() const override;
      virtual void showNativeGui(bool v) override;
      virtual bool hasNativeGui() const override;
      virtual void getNativeGeometry(int*x, int*y, int*w, int*h) const override;
      virtual void setNativeGeometry(int, int, int, int) override;
      virtual bool getData(MidiPort*, unsigned pos, int ports, unsigned nframes, float** buffer) override;
      virtual MidiPlayEvent receiveEvent() override;
      virtual int eventsPending() const override { return 0; }
      virtual int channels() const override;
      virtual int totalOutChannels() const override;
      virtual int totalInChannels() const override;
      virtual void deactivate3() override;
      virtual QString getPatchName(int chan, int prog, bool drum) const override;
      virtual void populatePatchPopup(MusEGui::PopupMenu* menu, int chan, bool drum) override;
      virtual void write(int level, Xml& xml) const override;
      virtual double getParameter(unsigned long idx) const override;
      virtual void setParameter(unsigned long idx, double value) override;
      virtual int getControllerInfo(int, QString*, int*, int*, int*, int*) override { return 0; }

      //-------------------------
      // Methods for PluginIBase:
      //-------------------------
      unsigned long pluginID() const override;
      int id() const override;
// REMOVE Tim. tmp. Changed.
      QString pluginLabel() const override;
      QString pluginName() const override;
      QString lib() const override;
      QString uri() const override;
      QString dirPath() const override;
      QString fileName() const override;
      void enableController(unsigned long i, bool v = true) override;
      bool controllerEnabled(unsigned long i) const override;
      void enableAllControllers(bool v = true) override;
      void updateControllers() override;
      unsigned long parameters() const override;
      unsigned long parametersOut() const override;
      void setParam(unsigned long i, double val) override;
      double param(unsigned long i) const override;
      double paramOut(unsigned long i) const override;
      const char* paramName(unsigned long i) const override;
      const char* paramOutName(unsigned long i) const override;
      LADSPA_PortRangeHint range(unsigned long i) const override;
      LADSPA_PortRangeHint rangeOut(unsigned long i) const override;
      void range(unsigned long i, float*, float*) const override;
      void rangeOut(unsigned long i, float*, float*) const override;
      CtrlValueType ctrlValueType(unsigned long i) const override;
      CtrlList::Mode ctrlMode(unsigned long i) const override;
      CtrlValueType ctrlOutValueType(unsigned long i) const override;
      CtrlList::Mode ctrlOutMode(unsigned long i) const override;
      void setCustomData ( const std::vector<QString> & ) override;
      // Returns true if ANY of the midi input ports uses transport source.
      bool usesTransportSource() const override;
      // Temporary variable holds value to be passed to the callback routine.
      float transportLatencyCorr() const;
      };

class VstNativePluginWrapper_State : public QObject
{
   Q_OBJECT
public:
   AEffect* plugin;
   VstNativePluginWrapper *pluginWrapper;
   PluginI *pluginI;
   std::vector<float *> inPorts;
   std::vector<float *> outPorts;
   std::vector<float *> inControlPorts;
   std::vector<float> inControlLastValues;
   MusEGui::VstNativeEditor* editor;
   VstNativeSynthOrPlugin userData;
   bool guiVisible;
   bool inProcess;
   bool active;
   bool curEnabledState;
   // Temporary variable holds value to be passed to the callback routine.
   float _latency_corr;
   VstNativePluginWrapper_State()
   {
      plugin = 0;
      pluginWrapper = 0;
      pluginI = 0;
      editor = 0;
      guiVisible = false;
      userData.sif = 0;
      userData.pstate = this;
      inProcess = false;
      active = false;
      curEnabledState = true;
      _latency_corr = 0.0f;
   }
   virtual ~VstNativePluginWrapper_State() {}
   void editorDeleted()
   {
      editor = 0;
   }
   void editorOpened()
   {
      guiVisible = true;
   }

   void editorClosed()
   {
      guiVisible = false;
   }
   void idleEditor();
protected slots:
   virtual void heartBeat();
};

class VstNativePluginWrapper: public Plugin
{
   friend class MusEGui::VstNativeEditor;
   friend class VstNativeSynth;
private:
    VstNativeSynth *_synth;
    LADSPA_Descriptor _fakeLd;
    LADSPA_PortDescriptor *_fakePds;
    std::vector<float> inControlDefaults;
    std::vector<std::string> portNames;
public:
    VstNativePluginWrapper ( VstNativeSynth *s, PluginFeatures_t reqFeatures = PluginNoFeatures );
    VstNativeSynth *synth() {
        return _synth;
    }
    virtual ~VstNativePluginWrapper();
    virtual LADSPA_Handle instantiate ( PluginI * );
    virtual int incReferences ( int ref );
    virtual void activate ( LADSPA_Handle handle );
    virtual void deactivate ( LADSPA_Handle handle );
    virtual void cleanup ( LADSPA_Handle handle );
    virtual void connectPort ( LADSPA_Handle handle, unsigned long port, float *value );
    virtual void apply ( LADSPA_Handle handle, unsigned long n, float latency_corr = 0.0f );
    virtual LADSPA_PortDescriptor portd ( unsigned long k ) const;

    virtual LADSPA_PortRangeHint range ( unsigned long i ) const;
    virtual void range (unsigned long, float *min, float *max ) const;

    virtual double defaultValue ( unsigned long port ) const;
    virtual const char *portName (unsigned long port ) const;
    virtual CtrlValueType ctrlValueType ( unsigned long ) const;
    virtual CtrlList::Mode ctrlMode ( unsigned long ) const;
    virtual bool hasNativeGui() const;
    virtual void showNativeGui ( PluginI *p, bool bShow );
    virtual bool nativeGuiVisible (const PluginI *p ) const;
// REMOVE Tim. tmp. Added.
    virtual QString getCustomConfiguration(LADSPA_Handle handle);
    virtual void writeConfiguration(LADSPA_Handle handle, int level, Xml& xml);
    virtual void setCustomData (LADSPA_Handle handle, const std::vector<QString> & customParams);

    VstIntPtr dispatch(VstNativePluginWrapper_State *state, VstInt32 opcode, VstInt32 index, VstIntPtr value, void* ptr, float opt) const {
                if(state->plugin) return state->plugin->dispatcher(state->plugin, opcode, index, value, ptr, opt); else return 0;  }
};


#endif // VST_NATIVE_SUPPORT

extern void initVST_Native();

} // namespace MusECore

#endif

