//=============================================================================
//  MusE
//  Linux Music Editor
//
//  lv2host.h
//  Copyright (C) 2014 by Deryabin Andrew <andrewderyabin@gmail.com>
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

#ifndef __LV2HOST_H__
#define __LV2HOST_H__

#include "config.h"

// Make sure this number is unique among all the MESS synths (including ticksynth) and DSSI, VST, LV2 and other host synths.
// 127 is reserved for special MusE system messages.
#define VST_NATIVE_SYNTH_UNIQUE_ID 9

#ifdef LV2_SUPPORT

// Disable warnings for parentheses. Did not work!
// #if defined(__clang__)
// #    pragma clang diagnostic push
// #    pragma clang diagnostic ignored "-Wparentheses"
// #elif __GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 6)
// #    pragma GCC diagnostic push
// #    pragma GCC diagnostic warning "-Wparentheses"
// #endif

#include "lilv/lilv.h"

// #if defined(__clang__)
// #    pragma clang diagnostic pop
// #elif __GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 6)
// #    pragma GCC diagnostic pop
// #endif

#include "lv2/lv2plug.in/ns/ext/data-access/data-access.h"
#include "lv2/lv2plug.in/ns/ext/state/state.h"
#include "lv2/lv2plug.in/ns/ext/atom/atom.h"
#include "lv2/lv2plug.in/ns/ext/midi/midi.h"
#include "lv2/lv2plug.in/ns/ext/buf-size/buf-size.h"
#include "lv2/lv2plug.in/ns/ext/event/event.h"
#include "lv2/lv2plug.in/ns/ext/options/options.h"
#include "lv2/lv2plug.in/ns/ext/parameters/parameters.h"
#include "lv2/lv2plug.in/ns/ext/patch/patch.h"
#include "lv2/lv2plug.in/ns/ext/port-groups/port-groups.h"
#include "lv2/lv2plug.in/ns/ext/presets/presets.h"
#include "lv2/lv2plug.in/ns/ext/state/state.h"
#include "lv2/lv2plug.in/ns/ext/time/time.h"
#include "lv2/lv2plug.in/ns/ext/uri-map/uri-map.h"
#include "lv2/lv2plug.in/ns/ext/urid/urid.h"
#include "lv2/lv2plug.in/ns/ext/worker/worker.h"
#include "lv2/lv2plug.in/ns/ext/port-props/port-props.h"
#include "lv2/lv2plug.in/ns/ext/atom/forge.h"
#include "lv2/lv2plug.in/ns/ext/log/log.h"
#include "lv2/lv2plug.in/ns/extensions/ui/ui.h"
#include "lv2/lv2plug.in/ns/ext/dynmanifest/dynmanifest.h"
#include "lv2extui.h"
#include "lv2extprg.h"

#include <cstring>
#include <iostream>
#include <vector>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <QMutex>
#include <QSemaphore>
#include <QThread>
#include <QTimer>
#include <QWindow>

#include <assert.h>
#include <algorithm>

#include "globaldefs.h"
#include "midictrl.h"
#include "synth.h"
#include "stringparam.h"

#include "plugin.h"
#include "plugin_list.h"

#endif

// Define to use QWidget instead of QMainWindow for the plugin gui container.
// #define LV2_GUI_USE_QWIDGET ;

namespace MusECore
{

#ifdef LV2_SUPPORT

class LV2Synth;

#define LV2_RT_FIFO_SIZE 128
#define LV2_RT_FIFO_ITEM_SIZE (std::max(size_t(4096 * 16), size_t(MusEGlobal::segmentSize * 16)))
#define LV2_EVBUF_SIZE (2*LV2_RT_FIFO_ITEM_SIZE)

struct LV2MidiEvent
{
   int64_t frame;
   uint8_t midi [4];
};

class LV2EvBuf
{
   enum LV2_BUF_TYPE
   {
      LV2_BUF_EVENT,
      LV2_BUF_ATOM
   };
   std::vector<uint8_t> _buffer;
   size_t curWPointer;
   size_t curRPointer;
   bool _isInput;
   bool _oldApi;
   LV2_URID _uAtomTypeSequence;
   LV2_URID _uAtomTypeChunk;
   LV2_Atom_Sequence *_seqbuf;
   LV2_Event_Buffer *_evbuf;
public:
   LV2EvBuf(bool isInput, bool oldApi, LV2_URID atomTypeSequence, LV2_URID atomTypeChunk);
   inline size_t mkPadSize(size_t size);
   inline void resetPointers(bool r, bool w);
   inline void resetBuffer();
   bool write(uint32_t frames, uint32_t subframes, uint32_t type, uint32_t size, const uint8_t *data);
   bool read(uint32_t *frames, uint32_t  *subframes, uint32_t  *type, uint32_t  *size, uint8_t  **data );
   uint8_t *getRawBuffer();
   void dump();
};

class LV2SimpleRTFifo
{
public:
   typedef struct _lv2_uiControlEvent
   {
      uint32_t port_index;
      long buffer_size;
      char *data;
   } lv2_uiControlEvent;
private:
   //size_t numItems;
   std::vector<lv2_uiControlEvent> eventsBuffer;
   size_t readIndex;
   size_t writeIndex;
   size_t fifoSize;
   size_t itemSize;
public:
   LV2SimpleRTFifo(size_t size);
   ~LV2SimpleRTFifo();
   inline size_t getItemSize(){return itemSize; }
   bool put(uint32_t port_index, uint32_t size, const void *data);
   bool get(uint32_t *port_index, size_t *szOut, char *data_out);
};



struct LV2MidiPort
{
    LV2MidiPort (const LilvPort *_p, uint32_t _i, QString _n, bool _f, bool _supportsTimePos) :
        port ( _p ), index ( _i ), name ( _n ), old_api ( _f ), supportsTimePos(_supportsTimePos), buffer(0){}
    const LilvPort *port;
    uint32_t index; //plugin real port index
    QString name;
    bool old_api; //true for LV2_Event port    
    bool supportsTimePos;   
    LV2EvBuf *buffer;
};

enum LV2ControlPortType
{
   LV2_PORT_DISCRETE = 1,
   LV2_PORT_INTEGER,
   LV2_PORT_CONTINUOUS,
   LV2_PORT_LOGARITHMIC,
   LV2_PORT_TRIGGER
};

struct LV2ControlPort
{
   LV2ControlPort ( const LilvPort *_p, uint32_t _i, float _c, const char *_n, const char *_s, LV2ControlPortType _ctype, bool _isCVPort = false) :
      port ( _p ), index ( _i ), defVal ( _c ), minVal( _c ), maxVal ( _c ), cType(_ctype),
      isCVPort(_isCVPort)
   {
      cName = strdup ( _n );
      cSym = strdup(_s);
   }
   LV2ControlPort ( const LV2ControlPort &other ) :
      port ( other.port ), index ( other.index ), defVal ( other.defVal ),
      minVal(other.minVal), maxVal(other.maxVal), cType(other.cType),
      isCVPort(other.isCVPort)
   {
      cName = strdup ( other.cName );
      cSym = strdup(other.cSym);
   }
   ~LV2ControlPort()
   {
      free ( cName );      
      cName = NULL;
      free(cSym);
      cSym = NULL;
   }
   const LilvPort *port;
   uint32_t index; //plugin real port index
   float defVal; //default control value
   float minVal; //minimum control value
   float maxVal; //maximum control value
   char *cName; //cached value to share between function calls
   char *cSym; //cached port symbol
   LV2ControlPortType cType;
   bool isCVPort;
};

struct LV2AudioPort
{
    LV2AudioPort ( const LilvPort *_p, uint32_t _i, float *_b, QString _n ) :
        port ( _p ), index ( _i ), buffer ( _b ), name ( _n ) {}
    const LilvPort *port;
    uint32_t index; //plugin real port index
    float *buffer; //audio buffer
    QString name;
};

struct cmp_str
{
    bool operator() ( char const *a, char const *b ) const
    {
        return std::strcmp ( a, b ) < 0;
    }
};

typedef std::vector<LV2MidiPort> LV2_MIDI_PORTS;
typedef std::vector<LV2ControlPort> LV2_CONTROL_PORTS;
typedef std::vector<LV2AudioPort> LV2_AUDIO_PORTS;
typedef std::map<const char *, uint32_t, cmp_str> LV2_SYNTH_URID_MAP;
typedef std::map<uint32_t, const char *> LV2_SYNTH_URID_RMAP;

class LV2UridBiMap
{
private:
    LV2_SYNTH_URID_MAP _map;
    LV2_SYNTH_URID_RMAP _rmap;
    uint32_t nextId;
    QMutex idLock;
public:
    LV2UridBiMap();
    ~LV2UridBiMap();
    LV2_URID map ( const char *uri );
    const char *unmap ( uint32_t id );
};

class LV2SynthIF;
struct LV2PluginWrapper_State;

typedef std::map<const LilvUI *, std::pair<bool, const LilvNode *> > LV2_PLUGIN_UI_TYPES;

class LV2Synth : public Synth
{
private:
    const LilvPlugin *_handle;
    LV2UridBiMap uridBiMap;
    LV2_Feature *_features;
    LV2_Feature **_ppfeatures;
    LV2_Options_Option *_options;
    LV2_URID_Map _lv2_urid_map;
    LV2_URID_Unmap _lv2_urid_unmap;
    LV2_URI_Map_Feature _lv2_uri_map;
    LV2_Log_Log _lv2_log_log;    
    double _sampleRate;
    float _fSampleRate;
    bool _isSynth;
// LV2 does not use unique id numbers and frowns upon using anything but the uri.
//     int _uniqueID;
    uint32_t _midi_event_id;
    LilvUIs *_uis;
    std::map<uint32_t, uint32_t> _idxToControlMap;

    LV2_PLUGIN_UI_TYPES _pluginUiTypes;

    //templates for LV2SynthIF and LV2PluginWrapper instantiation
    LV2_MIDI_PORTS _midiInPorts;
    LV2_MIDI_PORTS _midiOutPorts;
    LV2_CONTROL_PORTS _controlInPorts;
    LV2_CONTROL_PORTS _controlOutPorts;
    LV2_AUDIO_PORTS _audioInPorts;
    LV2_AUDIO_PORTS _audioOutPorts;

    MidiCtl2LadspaPortMap midiCtl2PortMap;   // Maps midi controller numbers to LV2 port numbers.
    MidiCtl2LadspaPortMap port2MidiCtlMap;   // Maps LV2 port numbers to midi controller numbers.
    uint32_t _fInstanceAccess;
    uint32_t _fUiParent;
    uint32_t _fExtUiHost;
    uint32_t _fExtUiHostD;
    uint32_t _fDataAccess;
    uint32_t _fWrkSchedule;
    uint32_t _fUiResize;
    uint32_t _fPrgHost;
    uint32_t _fMakePath;
    uint32_t _fMapPath;
    //const LilvNode *_pluginUIType = NULL;
    LV2_URID _uTime_Position;
    LV2_URID _uTime_frame;
    LV2_URID _uTime_speed;
    LV2_URID _uTime_beatsPerMinute;
    LV2_URID _uTime_barBeat;
    LV2_URID _uAtom_EventTransfer;
    LV2_URID _uAtom_Chunk;
    LV2_URID _uAtom_Sequence;
    LV2_URID _uAtom_StateChanged;
    LV2_URID _uAtom_Object;
    bool _hasFreeWheelPort;
    uint32_t _freeWheelPortIndex;
    bool _hasLatencyPort;
    uint32_t _latencyPortIndex;
    bool _isConstructed;
    float *_pluginControlsDefault;
    float *_pluginControlsMin;
    float *_pluginControlsMax;
    std::map<QString, LilvNode *> _presets;
public:
    virtual Type synthType() const {
        return _isSynth ? LV2_SYNTH : LV2_EFFECT;
    }
    LV2Synth ( const QFileInfo &fi, QString label, QString name, QString author, 
               const LilvPlugin *_plugin, PluginFeatures_t reqFeatures = PluginNoFeatures );
    virtual ~LV2Synth();
    virtual SynthIF *createSIF ( SynthI * );
    bool isSynth() {
        return _isSynth;
    }

    //own public functions
    LV2_URID mapUrid ( const char *uri );
    const char *unmapUrid ( LV2_URID id );
    size_t inPorts() {
        return _audioInPorts.size();
    }
    size_t outPorts() {
        return _audioOutPorts.size();
    }
    bool isConstructed() {return _isConstructed; }
    static void lv2ui_PostShow ( LV2PluginWrapper_State *state );
    static int lv2ui_Resize ( LV2UI_Feature_Handle handle, int width, int height );
    static void lv2ui_Gtk2AllocateCb(int width, int height, void *arg);
    static void lv2ui_Gtk2ResizeCb(int width, int height, void *arg);
    static void lv2ui_ShowNativeGui ( LV2PluginWrapper_State *state, bool bShow );
    static void lv2ui_PortWrite ( LV2UI_Controller controller, uint32_t port_index, uint32_t buffer_size, uint32_t protocol, void const *buffer );
    static void lv2ui_Touch (LV2UI_Controller controller, uint32_t port_index, bool grabbed);
    static void lv2ui_ExtUi_Closed ( LV2UI_Controller contr );
    static void lv2ui_SendChangedControls(LV2PluginWrapper_State *state);
    static void lv2state_FillFeatures ( LV2PluginWrapper_State *state );
    static void lv2state_PostInstantiate ( LV2PluginWrapper_State *state );
    static void lv2ui_FreeDescriptors(LV2PluginWrapper_State *state);
    static void lv2state_FreeState(LV2PluginWrapper_State *state);
    static void lv2audio_SendTransport(LV2PluginWrapper_State *state, LV2EvBuf *buffer, unsigned long nsamp);
    static void lv2state_InitMidiPorts ( LV2PluginWrapper_State *state );
    static void inline lv2audio_preProcessMidiPorts (LV2PluginWrapper_State *state, unsigned long nsamp);
    static void inline lv2audio_postProcessMidiPorts (LV2PluginWrapper_State *state, unsigned long nsamp);
    static const void *lv2state_stateRetreive ( LV2_State_Handle handle, uint32_t key, size_t *size, uint32_t *type, uint32_t *flags );
    static LV2_State_Status lv2state_stateStore ( LV2_State_Handle handle, uint32_t key, const void *value, size_t size, uint32_t type, uint32_t flags );
    static LV2_Worker_Status lv2wrk_scheduleWork(LV2_Worker_Schedule_Handle handle, uint32_t size, const void *data);
    static LV2_Worker_Status lv2wrk_respond(LV2_Worker_Respond_Handle handle, uint32_t size, const void* data);    
    static void lv2conf_write(LV2PluginWrapper_State *state, int level, Xml &xml);
    static void lv2conf_set(LV2PluginWrapper_State *state, const std::vector<QString> & customParams);
    static unsigned lv2ui_IsSupported (const char *, const char *ui_type_uri);
    static void lv2prg_updatePrograms(LV2PluginWrapper_State *state);
    static int lv2_printf(LV2_Log_Handle handle, LV2_URID type, const char *fmt, ...);
    static int lv2_vprintf(LV2_Log_Handle handle, LV2_URID type, const char *fmt, va_list ap);
    static char *lv2state_makePath(LV2_State_Make_Path_Handle handle, const char *path);
    static char *lv2state_abstractPath(LV2_State_Map_Path_Handle handle, const char *absolute_path);
    static char *lv2state_absolutePath(LV2_State_Map_Path_Handle handle, const char *abstract_path);
    static void lv2state_populatePresetsMenu(LV2PluginWrapper_State *state, MusEGui::PopupMenu *menu);
    static void lv2state_PortWrite ( LV2UI_Controller controller, uint32_t port_index, uint32_t buffer_size, uint32_t protocol, void const *buffer, bool fromUi);
    static void lv2state_setPortValue(const char *port_symbol, void *user_data, const void *value, uint32_t size, uint32_t type);
    static const void* lv2state_getPortValue(const char *port_symbol, void *user_data, uint32_t *size, uint32_t *type);
    static void lv2state_applyPreset(LV2PluginWrapper_State *state, LilvNode *preset);
    static void lv2state_UnloadLoadPresets(LV2Synth *synth, bool load = false, bool update = false);
    friend class LV2SynthIF;
    friend class LV2PluginWrapper;
    friend class LV2SynthIF_Timer;


};


class LV2SynthIF : public SynthIF
{
private:
    LV2Synth *_synth;
    LilvInstance *_handle;
    LV2_CONTROL_PORTS _controlInPorts;
    LV2_CONTROL_PORTS _controlOutPorts;
    LV2_AUDIO_PORTS _audioInPorts;
    LV2_AUDIO_PORTS _audioOutPorts;
    LV2_Feature *_ifeatures;
    LV2_Feature **_ppifeatures;
    Port *_controls;
    Port *_controlsOut;
    bool init ( LV2Synth *s );
    size_t _inports;
    size_t _outports;
    size_t _inportsControl;
    size_t _outportsControl;
    size_t _inportsMidi;
    size_t _outportsMidi;    
    float **_audioInBuffers;
    float **_audioOutBuffers;
    float  *_audioInSilenceBuf; // Just all zeros all the time, so we don't have to clear for silence.
    void doSelectProgram(unsigned char channel, int bankH, int bankL, int prog);
    inline void sendLv2MidiEvent(LV2EvBuf *evBuf, long frame, int paramCount, uint8_t a, uint8_t b = 0, uint8_t c = 0);
    bool processEvent (const MidiPlayEvent &, LV2EvBuf *evBuf, long frame);
    bool lv2MidiControlValues ( size_t port, int ctlnum, int *min, int *max, int *def );
    float midi2Lv2Value ( unsigned long port, int ctlnum, int val );
    LV2PluginWrapper_State *_state;
    
public:
    LV2SynthIF ( SynthI *s );
    virtual ~LV2SynthIF();

    //virtual methods from SynthIF
    virtual void guiHeartBeat();
    virtual bool hasGui() const;
    virtual bool nativeGuiVisible() const;
    virtual void showNativeGui ( bool v );
    virtual bool hasNativeGui() const;
    virtual void getNativeGeometry ( int *, int *, int *, int * ) const;
    virtual void setNativeGeometry (int x, int y, int w, int h);
    virtual bool getData ( MidiPort *, unsigned pos, int ports, unsigned n, float **buffer );
    virtual MidiPlayEvent receiveEvent();
    virtual int eventsPending() const;

    virtual int channels() const;
    virtual int totalOutChannels() const;
    virtual int totalInChannels() const;
    void activate();
    virtual void deactivate();
    virtual void deactivate3();
    virtual QString getPatchName (int, int, bool ) const;
    virtual void populatePatchPopup ( MusEGui::PopupMenu *, int, bool );
    virtual void write ( int level, Xml &xml ) const;
    virtual double getParameter ( unsigned long idx ) const;
    virtual double getParameterOut ( unsigned long n ) const;
    virtual void setParameter ( unsigned long idx, double value );
    virtual int getControllerInfo ( int id, QString* name, int *ctrl, int *min, int *max, int *initval );

    virtual void writeConfiguration ( int level, Xml &xml );
    virtual bool readConfiguration ( Xml &xml, bool readPreset=false );

    virtual void setCustomData ( const std::vector<QString> & );


    unsigned long parameters() const;
    unsigned long parametersOut() const;
    void setParam ( unsigned long i, double val );
    double param ( unsigned long i ) const;
    double paramOut ( unsigned long i ) const;
    const char *paramName ( unsigned long i );
    const char *paramOutName ( unsigned long i );
    virtual CtrlValueType ctrlValueType ( unsigned long ) const;
    virtual CtrlList::Mode ctrlMode ( unsigned long ) const;
    virtual LADSPA_PortRangeHint range(unsigned long i);
    virtual LADSPA_PortRangeHint rangeOut(unsigned long i);
    bool hasLatencyOutPort() const;
    unsigned long latencyOutPortIndex() const;
    float latency() const;

    virtual void enableController(unsigned long i, bool v = true);
    virtual bool controllerEnabled(unsigned long i) const;
    virtual void enableAllControllers(bool v = true);
    virtual void updateControllers();

    void populatePresetsMenu(MusEGui::PopupMenu *menu);
    void applyPreset(void *preset);


    int id() {
        return MusECore::MAX_PLUGINS;
    }

    static void lv2prg_Changed(LV2_Programs_Handle handle, int32_t index);

    friend class LV2Synth;
};


class LV2PluginWrapper;
class LV2PluginWrapper_Worker;
class LV2PluginWrapper_Window;

typedef struct _lv2ExtProgram
{
   uint32_t index;
   uint32_t bank;
   uint32_t prog;
   QString name;
   bool useIndex;
   bool operator<(const _lv2ExtProgram& other) const
   {
      if(useIndex == other.useIndex && useIndex == true)
         return index < other.index;

      if(bank < other.bank)
         return true;
      else if(bank == other.bank && prog < other.prog)
         return true;
      return false;
   }

   bool operator==(const _lv2ExtProgram& other) const
   {
      if(useIndex == other.useIndex && useIndex == true)
         return index == other.index;

      return (bank == other.bank && prog == other.prog);
   }


} lv2ExtProgram;

struct LV2PluginWrapper_State {
   LV2PluginWrapper_State():
      _ifeatures(NULL),
      _ppifeatures(NULL),
      widget(NULL),      
      handle(NULL),
      uiDlHandle(NULL),
      uiDesc(NULL),
      uiInst(NULL),
      inst(NULL),
      lastControls(NULL),
      controlsMask(NULL),
      lastControlsOut(NULL),
      plugInst(NULL),
      sif(NULL),
      synth(NULL),
      human_id(NULL),
      iState(NULL),
      tmpValues(NULL),
      numStateValues(0),
      wrkDataSize(0),
      wrkDataBuffer(0),
      wrkThread(NULL),
      wrkEndWork(false),
      controlTimers(NULL),
      deleteLater(false),
      hasGui(false),
      hasExternalGui(false),
      uiIdleIface(NULL),
      uiCurrent(NULL),
      uiX11Size(0, 0),
      pluginWindow(NULL),
      pluginQWindow(NULL),
      
      prgIface(NULL),
      uiPrgIface(NULL),
      uiDoSelectPrg(false),
      newPrgIface(false),
      uiChannel(0),
      uiBank(0),
      uiProg(0),
      gtk2Plug(NULL),
      pluginCVPorts(NULL),
      uiControlEvt(LV2_RT_FIFO_SIZE),
      plugControlEvt(LV2_RT_FIFO_SIZE),
      gtk2ResizeCompleted(false),
      gtk2AllocateCompleted(false),
      songDirtyPending(false),
      uiIsOpening(false)
   {
      extHost.plugin_human_id = NULL;
      extHost.ui_closed = NULL;
      uiResize.handle = (LV2UI_Feature_Handle)this;
      uiResize.ui_resize = LV2Synth::lv2ui_Resize;
      prgHost.handle = (LV2_Programs_Handle)this;
      prgHost.program_changed = LV2SynthIF::lv2prg_Changed;
      makePath.handle = (LV2_State_Make_Path_Handle)this;
      makePath.path = LV2Synth::lv2state_makePath;
      mapPath.handle = (LV2_State_Map_Path_Handle)this;
      mapPath.absolute_path = LV2Synth::lv2state_absolutePath;
      mapPath.abstract_path = LV2Synth::lv2state_abstractPath;

      midiInPorts.clear();
      midiOutPorts.clear();
      idx2EvtPorts.clear();
      inPortsMidi = outPortsMidi = 0;
   }

    LV2_Feature *_ifeatures;
    LV2_Feature **_ppifeatures;
    void *widget;
    LV2_External_UI_Host extHost;
    LV2_Extension_Data_Feature extData;
    LV2_Worker_Schedule wrkSched;
    LV2_State_Make_Path makePath;
    LV2_State_Map_Path mapPath;
    LilvInstance *handle;
    void *uiDlHandle;
    const LV2UI_Descriptor *uiDesc;
    LV2UI_Handle uiInst;
    LV2PluginWrapper *inst;
    float *lastControls;
    bool *controlsMask;
    float *lastControlsOut;
    PluginI *plugInst;
    LV2SynthIF *sif;
    LV2Synth *synth;
    char *human_id;
    LV2_State_Interface *iState;
    QMap<QString, QPair<QString, QVariant> > iStateValues;
    char **tmpValues;
    size_t numStateValues;
    uint32_t wrkDataSize;
    const void *wrkDataBuffer;
    LV2PluginWrapper_Worker *wrkThread;
    LV2_Worker_Interface *wrkIface;
    bool wrkEndWork;
    int *controlTimers;
    bool deleteLater;
    LV2_Atom_Forge atomForge;
    float curBpm;
    bool curIsPlaying;
    unsigned int curFrame;
    bool hasGui;
    bool hasExternalGui;
    LV2UI_Idle_Interface *uiIdleIface;
    const LilvUI *uiCurrent;    
    LV2UI_Resize uiResize;
    QSize uiX11Size;
    LV2PluginWrapper_Window *pluginWindow;
    QWindow *pluginQWindow;
    LV2_MIDI_PORTS midiInPorts;
    LV2_MIDI_PORTS midiOutPorts;
    size_t inPortsMidi;
    size_t outPortsMidi;
    LV2_Programs_Interface *prgIface;
    LV2_Programs_UI_Interface *uiPrgIface;
    bool uiDoSelectPrg;
    bool newPrgIface;
    std::map<uint32_t, lv2ExtProgram> index2prg;
    std::map<uint32_t, uint32_t> prg2index;
    LV2_Programs_Host prgHost;
    unsigned char uiChannel;
    int uiBank;
    int uiProg;
    void *gtk2Plug;
    std::map<QString, size_t> controlsNameMap;
    std::map<QString, size_t> controlsSymMap;
    float **pluginCVPorts;
    LV2SimpleRTFifo uiControlEvt;
    LV2SimpleRTFifo plugControlEvt;
    std::map<uint32_t, LV2EvBuf *> idx2EvtPorts;
    bool gtk2ResizeCompleted;
    bool gtk2AllocateCompleted;
    bool songDirtyPending;
    bool uiIsOpening;
};


class LV2PluginWrapper_Worker :public QThread
{
private:
    LV2PluginWrapper_State *_state;
    QSemaphore _mSem;
    bool _closing;
public:
    explicit LV2PluginWrapper_Worker ( LV2PluginWrapper_State *s ) : QThread(),
       _state ( s ),
       _mSem(0),
       _closing(false)
    {}

    void run();
    LV2_Worker_Status scheduleWork();
    void makeWork();
    void setClosing() {_closing = true; _mSem.release();}

};


#ifdef LV2_GUI_USE_QWIDGET
class LV2PluginWrapper_Window : public QWidget
#else
class LV2PluginWrapper_Window : public QMainWindow
#endif
{
   Q_OBJECT
protected:
   void closeEvent ( QCloseEvent *event );
   void showEvent(QShowEvent *e);
   void hideEvent(QHideEvent *e);
private:
   LV2PluginWrapper_State *_state;
   bool _closing;
   QTimer updateTimer;
   void stopUpdateTimer();
public:
   explicit LV2PluginWrapper_Window ( LV2PluginWrapper_State *state, 
                                      QWidget *parent = Q_NULLPTR, 
                                      Qt::WindowFlags flags = Qt::WindowFlags());
   ~LV2PluginWrapper_Window();
   void startNextTime();
   void stopNextTime();
   void doChangeControls();
   void setClosing(bool closing) {_closing = closing; }
signals:
   void makeStopFromGuiThread();
   void makeStartFromGuiThread();
public slots:
   void updateGui();
   void stopFromGuiThread();
   void startFromGuiThread();
};


class LV2PluginWrapper: public Plugin
{
private:
    LV2Synth *_synth;
    LADSPA_Descriptor _fakeLd;
    LADSPA_PortDescriptor *_fakePds;       
public:
    LV2PluginWrapper ( LV2Synth *s, PluginFeatures_t reqFeatures = PluginNoFeatures );
    LV2Synth *synth() {
        return _synth;
    }    
    virtual ~LV2PluginWrapper();
    virtual LADSPA_Handle instantiate ( PluginI * );
    virtual int incReferences ( int ref );
    virtual void activate ( LADSPA_Handle handle );
    virtual void deactivate ( LADSPA_Handle handle );
    virtual void cleanup ( LADSPA_Handle handle );
    virtual void connectPort ( LADSPA_Handle handle, unsigned long port, float *value );
    virtual void apply ( LADSPA_Handle handle, unsigned long n );
    virtual LADSPA_PortDescriptor portd ( unsigned long k ) const;

    virtual LADSPA_PortRangeHint range ( unsigned long i );
    virtual void range ( unsigned long i, float *min, float *max ) const;

    virtual double defaultValue ( unsigned long port ) const;
    virtual const char *portName ( unsigned long i );
    virtual CtrlValueType ctrlValueType ( unsigned long ) const;
    virtual CtrlList::Mode ctrlMode ( unsigned long ) const;
    virtual bool hasNativeGui() const;
    virtual void showNativeGui ( PluginI *p, bool bShow );
    virtual bool nativeGuiVisible (const PluginI *p ) const;
    virtual void setLastStateControls(LADSPA_Handle handle, size_t index, bool bSetMask, bool bSetVal, bool bMask, float fVal);
    virtual void writeConfiguration(LADSPA_Handle handle, int level, Xml& xml);
    virtual void setCustomData (LADSPA_Handle handle, const std::vector<QString> & customParams);

    void populatePresetsMenu(PluginI *p, MusEGui::PopupMenu *menu);
    void applyPreset(PluginI *p, void *preset);
};

#endif // LV2_SUPPORT

extern void initLV2();

} // namespace MusECore

#endif



