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
// We don't use internal sysex with LV2 so this is not used, but if it was it would be this:
#define LV2_SYNTH_UNIQUE_ID 10

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

#include "lv2/data-access/data-access.h"
#include "lv2/state/state.h"
#include "lv2/atom/atom.h"
#include "lv2/midi/midi.h"
#include "lv2/buf-size/buf-size.h"
#ifdef LV2_EVENT_BUFFER_SUPPORT
#include "lv2/event/event.h"
#endif
#include "lv2/options/options.h"
#include "lv2/parameters/parameters.h"
#include "lv2/patch/patch.h"
#include "lv2/port-groups/port-groups.h"
#include "lv2/presets/presets.h"
#include "lv2/time/time.h"
#ifdef LV2_URI_MAP_SUPPORT
#include "lv2/uri-map/uri-map.h"
#endif
#include "lv2/urid/urid.h"
#include "lv2/worker/worker.h"
#include "lv2/port-props/port-props.h"
#include "lv2/atom/forge.h"
#include "lv2/log/log.h"
#include "lv2/ui/ui.h"
#include "lv2/dynmanifest/dynmanifest.h"
#include "lv2/resize-port/resize-port.h"
#include "lv2extui.h"
#include "lv2extprg.h"
#ifdef MIDNAM_SUPPORT
#include "midnam_lv2.h"
#endif
#include "lv2/units/units.h"

#include <vector>
#include <map>
#include <QString>
#include <QMutex>
#include <QSemaphore>
#include <QThread>
#include <QTimer>
#include <QWindow>
// REMOVE Tim. tmp. Added.
//#include <QMetaObject>

#include "type_defs.h"
#include "globaldefs.h"
#include "midictrl.h"
#include "synth.h"
#include "stringparam.h"

#include "plugin.h"
#include "plugin_list.h"

#include "lock_free_buffer.h"
#include "lock_free_data_buffer.h"

#endif

// Define to use QWidget instead of QMainWindow for the plugin gui container.
// #define LV2_GUI_USE_QWIDGET ;

namespace MusEPlugin {
class PluginScanInfoStruct;
}

namespace MusECore
{

#ifdef LV2_SUPPORT

class LV2Synth;

struct LV2MidiEvent
{
   int64_t frame;
   uint8_t midi [4];
};

class LV2EvBuf
{
#ifdef LV2_EVENT_BUFFER_SUPPORT
   enum LV2_BUF_TYPE
   {
      LV2_BUF_EVENT,
      LV2_BUF_ATOM
   };
#endif
   std::vector<uint8_t> _buffer;
   size_t curWPointer;
   size_t curRPointer;
   bool _isInput;
#ifdef LV2_EVENT_BUFFER_SUPPORT
   bool _oldApi;
#endif
   LV2_URID _uAtomTypeSequence;
   LV2_URID _uAtomTypeChunk;
   LV2_Atom_Sequence *_seqbuf;
#ifdef LV2_EVENT_BUFFER_SUPPORT
   LV2_Event_Buffer *_evbuf;
#endif
public:
#ifdef LV2_EVENT_BUFFER_SUPPORT
   LV2EvBuf(bool isInput, bool oldApi, LV2_URID atomTypeSequence, LV2_URID atomTypeChunk, size_t size);
#else
   LV2EvBuf(bool isInput, LV2_URID atomTypeSequence, LV2_URID atomTypeChunk, size_t size);
#endif
   inline size_t mkPadSize(size_t size) const;
   inline void resetPointers(bool r, bool w);
   inline void resetBuffer();
#ifdef LV2_EVENT_BUFFER_SUPPORT
   bool write(uint32_t frames, uint32_t subframes, uint32_t type, uint32_t size, const uint8_t *data);
   bool read(uint32_t *frames, uint32_t  *subframes, uint32_t  *type, uint32_t  *size, uint8_t  **data );
#else
   bool write(uint32_t frames, uint32_t type, uint32_t size, const uint8_t *data);
   bool read(uint32_t *frames, uint32_t  *type, uint32_t  *size, uint8_t  **data );
#endif
   uint8_t *getRawBuffer();
   void dump();
   // Returns true if there is something to read.
   bool canRead() const;
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
   inline size_t getItemSize();
   bool put(uint32_t port_index, uint32_t size, const void *data);
   bool get(uint32_t *port_index, size_t *szOut, char *data_out);
};

typedef struct _lv2ExtProgram
{
   uint32_t index;
   uint32_t bank;
   uint32_t prog;
   QString name;
   bool useIndex;
   bool operator<(const _lv2ExtProgram& other) const;
   bool operator==(const _lv2ExtProgram& other) const;

} lv2ExtProgram;


struct LV2MidiPort
{
#ifdef LV2_EVENT_BUFFER_SUPPORT
    LV2MidiPort (const LilvPort *_p, uint32_t _i, QString _n, bool _f, bool _supportsTimePos);
#else
    LV2MidiPort (const LilvPort *_p, uint32_t _i, QString _n, bool _supportsTimePos);
#endif
    const LilvPort *port;
    uint32_t index; //plugin real port index
    QString name;
#ifdef LV2_EVENT_BUFFER_SUPPORT
    bool old_api; //true for LV2_Event port    
#endif
    bool supportsTimePos;   
    LV2EvBuf *buffer;
};

// Can be OR'd together.
enum LV2ControlPortType
{
    LV2_PORT_NO_FLAGS = 0x0,
    LV2_PORT_INTEGER = 0x1,
    LV2_PORT_LOGARITHMIC = 0x2,
    LV2_PORT_TOGGLE = 0x4,
    LV2_PORT_ENUMERATION = 0x8,
    LV2_PORT_NON_CONTINUOUS = LV2_PORT_INTEGER | LV2_PORT_TOGGLE | LV2_PORT_ENUMERATION
};
// A combination of LV2ControlPortType flags.
typedef int LV2ControlPortType_t;

struct LV2ControlPort
{
   LV2ControlPort ( const LilvPort *_p, uint32_t _i, float _defVal, float _minVal, float _maxVal,
                    const char *_name, const char *_symbol, int _unitTextIdx,
                    LV2ControlPortType_t _ctype, bool _isCVPort = false, CtrlVal::CtrlEnumValues* scalePoints = nullptr,
                    QString group = QString(), bool isTrigger = false, bool notOnGui = false,
                    bool isDiscrete = false, bool hasStrictBounds = false, bool isSampleRate = false);
   LV2ControlPort ( const LV2ControlPort &other );
   ~LV2ControlPort();

   const LilvPort *port;
   uint32_t index; //plugin real port index
   float defVal; //default control value
   float minVal; //minimum control value
   float maxVal; //maximum control value
   bool hasStrictBounds;
   bool isSampleRate; // Any specified bounds should be interpreted as multiples of the sample rate
   char *cName; //cached value to share between function calls
   char *cSym; //cached port symbol
   LV2ControlPortType_t cType;
   bool isCVPort;
   const CtrlVal::CtrlEnumValues* scalePoints;
   QString group;
   bool isTrigger;
   bool notOnGui;
   bool isDiscrete;
   // Index into global unit text string list. Can be -1 meaning no text.
   int unitTextIdx;
};

struct LV2AudioPort
{
    LV2AudioPort ( const LilvPort *_p, uint32_t _i, float *_b, QString _n );
    const LilvPort *port;
    uint32_t index; //plugin real port index
    float *buffer; //audio buffer
    QString name;
};

struct cmp_str
{
    bool operator() ( char const *a, char const *b ) const;
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

class LV2OperationMessage
{
  public:
    enum Type { 
      // For the programs extension.
      ProgramChanged=0,
      // For the midnam extension.
      MidnamUpdate
    };

    Type _type;
    // For the programs extension. Index of program to update.
    // Can be -1 for update all programs.
    int _index;
    
    LV2OperationMessage();
    LV2OperationMessage(Type type, int index = 0);
};


typedef struct {
    QString symbol;
    QString label;
    PropType type;
    float min;             ///< Minimum value
    float max;             ///< Maximum value
    float def;             ///< Default value
} LV2Property;


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
#ifdef LV2_URI_MAP_SUPPORT
    LV2_URI_Map_Feature _lv2_uri_map;
#endif
    LV2_Log_Log _lv2_log_log;    
    double _sampleRate;
    float _fSampleRate;
    float _scaleFactor;
    // Just so we can point to a zero.
    static const unsigned minBlockSize;
    bool _isSynth;
// LV2 does not use unique id numbers and frowns upon using anything but the uri.
//     int _uniqueID;
    uint32_t _midi_event_id;
    LilvUIs *_uis;
    std::map<uint32_t, uint32_t> _idxToControlMap;
    std::map<uint32_t, uint32_t> _idxToControlOutMap;

    LV2_PLUGIN_UI_TYPES _pluginUiTypes;

    //templates for LV2SynthIF and LV2PluginWrapper instantiation
    LV2_MIDI_PORTS _midiInPorts;
    LV2_MIDI_PORTS _midiOutPorts;
    LV2_CONTROL_PORTS _controlInPorts;
    LV2_CONTROL_PORTS _controlOutPorts;
    LV2_AUDIO_PORTS _audioInPorts;
    LV2_AUDIO_PORTS _audioOutPorts;

    uint32_t _fInstanceAccess;
    uint32_t _fUiParent;
    uint32_t _fExtUiHost;
    uint32_t _fExtUiHostD;
    uint32_t _fDataAccess;
    uint32_t _fWrkSchedule;
    uint32_t _fUiResize;
    uint32_t _fUiRequestValue;
    uint32_t _fPrgHost;
#ifdef MIDNAM_SUPPORT
    uint32_t _fMidNamUpdate;
#endif
#ifdef LV2_MAKE_PATH_SUPPORT
    uint32_t _fMakePath;
#endif
    uint32_t _fMapPath;
    uint32_t _fLoadDefaultState;

    //const LilvNode *_pluginUIType = nullptr;

    LV2_URID _uTime_Position;
    LV2_URID _uTime_frame;
    LV2_URID _uTime_framesPerSecond;
    LV2_URID _uTime_speed;
    LV2_URID _uTime_beatsPerMinute;
    LV2_URID _uTime_beatsPerBar;
    LV2_URID _uTime_beat;
    LV2_URID _uTime_bar;
    LV2_URID _uTime_barBeat;
    LV2_URID _uTime_beatUnit;

    LV2_URID _uAtom_EventTransfer;
    LV2_URID _uAtom_Chunk;
    LV2_URID _uAtom_Sequence;
    LV2_URID _uAtom_StateChanged;
    LV2_URID _uAtom_Object;
    bool _usesTimePosition;
    bool _isConstructed;
    float *_pluginControlsDefault;
    float *_pluginControlsMin;
    float *_pluginControlsMax;
    std::map<QString, LilvNode *> _presets;

public:
    virtual Type synthType() const;
    LV2Synth (const MusEPlugin::PluginScanInfoStruct&, const LilvPlugin*);
    virtual ~LV2Synth();

    virtual SynthIF *createSIF ( SynthI * );
    bool isSynth();

    //own public functions
    LV2_URID mapUrid ( const char *uri );
    const char *unmapUrid ( LV2_URID id );
    size_t inPorts();
    size_t outPorts();
    bool isConstructed();
    // Returns true if ANY of the midi input ports uses time position (transport).
    bool usesTimePosition() const;
    static void lv2ui_PostShow ( LV2PluginWrapper_State *state );
    static int lv2ui_Resize ( LV2UI_Feature_Handle handle, int width, int height );
    static LV2UI_Request_Value_Status lv2ui_Request_Value (
      LV2UI_Feature_Handle handle, LV2_URID key, LV2_URID type, const LV2_Feature *const *features );
    static void lv2ui_Gtk2AllocateCb(int width, int height, void *arg);
    static void lv2ui_Gtk2ResizeCb(int width, int height, void *arg);
    static void lv2ui_ShowNativeGui (LV2PluginWrapper_State *state, bool bShow , bool fixScaling);
    static void lv2ui_PortWrite ( LV2UI_Controller controller, uint32_t port_index, uint32_t buffer_size, uint32_t protocol, void const *buffer );
    static void lv2ui_Touch (LV2UI_Controller controller, uint32_t port_index, bool grabbed);
    static void lv2ui_ExtUi_Closed ( LV2UI_Controller contr );
    static void lv2ui_SendChangedControls(LV2PluginWrapper_State *state);
    static void lv2state_FillFeatures ( LV2PluginWrapper_State *state );
    static void lv2state_PostInstantiate ( LV2PluginWrapper_State *state );
    static void lv2ui_FreeDescriptors(LV2PluginWrapper_State *state);
    static void lv2state_FreeState(LV2PluginWrapper_State *state);
    static void lv2audio_SendTransport(LV2PluginWrapper_State *state,
                                       unsigned long sample, unsigned long nsamp,
                                       float latency_corr = 0.0f);
    static void lv2state_InitMidiPorts ( LV2PluginWrapper_State *state );
    static void inline lv2audio_preProcessMidiPorts (LV2PluginWrapper_State *state, unsigned long sample, unsigned long nsamp);
    static void inline lv2audio_postProcessMidiPorts (LV2PluginWrapper_State *state, unsigned long sample, unsigned long nsamp);
    static const void *lv2state_stateRetreive ( LV2_State_Handle handle, uint32_t key, size_t *size, uint32_t *type, uint32_t *flags );
    static LV2_State_Status lv2state_stateStore ( LV2_State_Handle handle, uint32_t key, const void *value, size_t size, uint32_t type, uint32_t flags );
    static LV2_Worker_Status lv2wrk_scheduleWork(LV2_Worker_Schedule_Handle handle, uint32_t size, const void *data);
    static LV2_Worker_Status lv2wrk_respond(LV2_Worker_Respond_Handle handle, uint32_t size, const void* data);    
// REMOVE Tim. tmp. Added.
    static QString getCustomConfiguration(LV2PluginWrapper_State *state);
    static void lv2conf_write(LV2PluginWrapper_State *state, int level, Xml &xml);
    static void lv2conf_set(LV2PluginWrapper_State *state, const std::vector<QString> & customParams);
    static unsigned lv2ui_IsSupported (const char *, const char *ui_type_uri);
    static void lv2prg_updateProgram(LV2PluginWrapper_State *state, int idx);
    static void lv2prg_updatePrograms(LV2PluginWrapper_State *state);
#ifdef MIDNAM_SUPPORT
    static void lv2midnam_updateMidnam(LV2PluginWrapper_State *state);
#endif    
    static int lv2_printf(LV2_Log_Handle handle, LV2_URID type, const char *fmt, ...);
    static int lv2_vprintf(LV2_Log_Handle handle, LV2_URID type, const char *fmt, va_list ap);
#ifdef LV2_MAKE_PATH_SUPPORT
    static char *lv2state_makePath(LV2_State_Make_Path_Handle handle, const char *path);
#endif
    static char *lv2state_abstractPath(LV2_State_Map_Path_Handle handle, const char *absolute_path);
    static char *lv2state_absolutePath(LV2_State_Map_Path_Handle handle, const char *abstract_path);
    static void lv2state_populatePresetsMenu(LV2PluginWrapper_State *state, MusEGui::PopupMenu *menu);
    static void lv2state_PortWrite ( LV2UI_Controller controller, uint32_t port_index, uint32_t buffer_size, uint32_t protocol, void const *buffer, bool fromUi);
    static void lv2state_setPortValue(const char *port_symbol, void *user_data, const void *value, uint32_t size, uint32_t type);
    static const void* lv2state_getPortValue(const char *port_symbol, void *user_data, uint32_t *size, uint32_t *type);
    static void lv2state_applyPreset(LV2PluginWrapper_State *state, LilvNode *preset);
    static void lv2state_UnloadLoadPresets(LV2Synth *synth, bool load = false, bool update = false);
// REMOVE Tim. tmp. Added.
    static void lv2ui_UpdateWindowTitle(LV2PluginWrapper_State *state);

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
    // Just a place to connect all unused audio outputs.
    float *_audioOutDummyBuf;
    // For plugins that DO support the programs extension. Returns true if the selection succeeded
    //  (ie the programs interface and functions exist).
    bool doSelectProgram(unsigned char channel, int bankH, int bankL, int prog);
    // For plugins that DO NOT support the programs extension. Sends as bankH/bankL/prog midi events.
    bool doSendProgram(unsigned char channel, int bankH, int bankL, int prog, LV2EvBuf *evBuf, long frame);
    inline void sendLv2MidiEvent(LV2EvBuf *evBuf, long frame, int paramCount, uint8_t a, uint8_t b = 0, uint8_t c = 0);
    void eventReceived(uint32_t frames, uint32_t size, uint8_t* data);
    bool processEvent (const MidiPlayEvent &, LV2EvBuf *evBuf, long frame);
    bool lv2MidiControlValues ( size_t port, int ctlnum, int *min, int *max, int *def );
    float midi2Lv2Value ( unsigned long port, int ctlnum, int val );
#ifdef MIDNAM_SUPPORT
    QString getPatchNameMidNam (int, int, bool ) const;
    void populatePatchPopupMidNam (MusEGui::PopupMenu*, int channel, bool drum);
#endif

    LV2PluginWrapper_State *_state;

protected:
    void activate() override;
    void deactivate() override;

public:
    LV2SynthIF ( SynthI *s );
    virtual ~LV2SynthIF();

    //virtual methods from SynthIF
    virtual void guiHeartBeat() override;
    virtual bool hasGui() const override;
    virtual bool nativeGuiVisible() const override;
    virtual void showNativeGui ( bool v ) override;
    virtual bool hasNativeGui() const override;
    virtual void getNativeGeometry ( int *, int *, int *, int * ) const override;
    virtual void setNativeGeometry (int x, int y, int w, int h) override;
// REMOVE Tim. tmp. Added.
    void updateNativeGuiWindowTitle() override;
    virtual bool getData ( MidiPort *, unsigned pos, int ports, unsigned n, float **buffer ) override;
    virtual MidiPlayEvent receiveEvent() override;
    virtual int eventsPending() const override;

    virtual int channels() const override;
    virtual int totalOutChannels() const override;
    virtual int totalInChannels() const override;
    virtual void deactivate3() override;
    virtual QString getPatchName (int, int, bool ) const override;
    virtual void populatePatchPopup ( MusEGui::PopupMenu *, int, bool ) override;
    virtual void write ( int level, Xml &xml ) const override;
    virtual double getParameter ( unsigned long idx ) const override;
    virtual double getParameterOut ( unsigned long n ) const;
    virtual void setParameter ( unsigned long idx, double value ) override;
    virtual int getControllerInfo ( int id, QString* name, int *ctrl, int *min, int *max, int *initval ) override;
    // Returns true if a note name list is found for the given patch.
    // If true, name either contains the note name, or is blank if no note name was found.
    // drum = Want percussion names, not melodic.
    virtual bool getNoteSampleName(
      bool drum, int channel, int patch, int note, QString* name) const override;

// REMOVE Tim. tmp. Removed. Not required.
    // virtual void writeConfiguration ( int level, Xml &xml ) override;
    // virtual bool readConfiguration ( Xml &xml, bool readPreset=false ) override;

    virtual void setCustomData ( const std::vector<QString> & ) override;


    unsigned long parameters() const override;
    unsigned long parametersOut() const override;
    void setParam ( unsigned long i, double val ) override;
    double param ( unsigned long i ) const override;
    double paramOut ( unsigned long i ) const override;
    const char *paramName ( unsigned long i ) const override;
    const char *paramOutName ( unsigned long i ) const override;
    CtrlValueType ctrlValueType ( unsigned long ) const override;
    CtrlList::Mode ctrlMode ( unsigned long ) const override;
    const CtrlVal::CtrlEnumValues *ctrlEnumValues(unsigned long i) const override;
    QString portGroup(long unsigned int i) const override;
    bool ctrlIsTrigger(long unsigned int i) const override;
    bool ctrlNotOnGui(long unsigned int i) const override;
    CtrlValueType ctrlOutValueType ( unsigned long ) const override;
    CtrlList::Mode ctrlOutMode ( unsigned long ) const override;
    const CtrlVal::CtrlEnumValues *ctrlOutEnumValues(unsigned long i) const override;
    QString portGroupOut(long unsigned int i) const override;
    bool ctrlOutIsTrigger(long unsigned int i) const override;
    bool ctrlOutNotOnGui(long unsigned int i) const override;
    // Returns a value unit string for displaying unit symbols.
    QString unitSymbol(unsigned long i) const override;
    QString unitSymbolOut(unsigned long i) const override;
    // Returns index into the global value units for displaying unit symbols.
    // Can be -1 meaning no units.
    int valueUnit(unsigned long i) const override;
    int valueUnitOut(unsigned long i) const override;

    LADSPA_PortRangeHint range(unsigned long i) const override;
    LADSPA_PortRangeHint rangeOut(unsigned long i) const override;
    void range(unsigned long i, float*, float*) const override;
    void rangeOut(unsigned long i, float*, float*) const override;

    // Returns true if ANY of the midi input ports uses time position (transport).
    bool usesTransportSource() const override;

    virtual void enableController(unsigned long i, bool v = true) override;
    virtual bool controllerEnabled(unsigned long i) const override;
    virtual void enableAllControllers(bool v = true) override;
    virtual void updateControllers() override;

    void populatePresetsMenu(MusEGui::PopupMenu *menu);
    void applyPreset(void *preset);

// REMOVE Tim. tmp. Removed.
//     int id() const override;

    static void lv2prg_Changed(LV2_Programs_Handle handle, int32_t index);
#ifdef MIDNAM_SUPPORT
    static void lv2midnam_Changed(LV2_Midnam_Handle handle);
#endif

    friend class LV2Synth;
};


class LV2PluginWrapper;
class LV2PluginWrapper_Worker;
class LV2PluginWrapper_Window;

struct LV2PluginWrapper_State {
    LV2PluginWrapper_State();

    LV2_Feature *_ifeatures;
    LV2_Feature **_ppifeatures;
    void *widget;
    LV2_External_UI_Host extHost;
    LV2_Extension_Data_Feature extData;
    LV2_Worker_Schedule wrkSched;
#ifdef LV2_MAKE_PATH_SUPPORT
    LV2_State_Make_Path makePath;
#endif
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
    LockFreeDataRingBuffer *wrkDataBuffer;
    LockFreeDataRingBuffer *wrkRespDataBuffer;
    LV2PluginWrapper_Worker *wrkThread;
    LV2_Worker_Interface *wrkIface;
    int *controlTimers;
    bool deleteLater;
    LV2_Atom_Forge atomForge;

    // State of the transport, for testing if it changed.
    int curGlobalTempo;
    int curTempo;
    bool curIsPlaying;
    unsigned int curFrame;
    unsigned int curTick;
    int curBeatsPerBar;
    int curBeatUnit;

    bool hasGui;
    bool hasExternalGui;
    bool fixedSizeGui;
    bool noUserResizeGui;
    LV2UI_Idle_Interface *uiIdleIface;
    const LilvUI *uiCurrent;    
    LV2UI_Resize uiResize;
    LV2UI_Request_Value uiRequestValue;
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
#ifdef MIDNAM_SUPPORT
    LV2_Midnam_Interface *midnamIface;
    LV2_Midnam midnamUpdate;
#endif
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
    bool active;
    LockFreeMPSCRingBuffer<LV2OperationMessage> operationsFifo;
};


class LV2PluginWrapper_Worker :public QThread
{
private:
    LV2PluginWrapper_State *_state;
    QSemaphore _mSem;
    bool _closing;
public:
    explicit LV2PluginWrapper_Worker ( LV2PluginWrapper_State *s );

    void run();
    LV2_Worker_Status scheduleWork();
    void makeWork();
    void setClosing();

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
// REMOVE Tim. tmp. Added.
//    QMetaObject::Connection _songChangedMetaConn;
public:
   explicit LV2PluginWrapper_Window ( LV2PluginWrapper_State *state, 
                                      QWidget *parent = Q_NULLPTR, 
                                      Qt::WindowFlags flags = Qt::WindowFlags());
   ~LV2PluginWrapper_Window();
   void startNextTime();
   void stopNextTime();
   void setClosing(bool closing);
// REMOVE Tim. tmp. Added.
   void updateWindowTitle(const QString&);
signals:
   void makeStopFromGuiThread();
   void makeStartFromGuiThread();
public slots:
   void updateGui();
   void stopFromGuiThread();
   void startFromGuiThread();
// REMOVE Tim. tmp. Added.
//    void songChanged(MusECore::SongChangedStruct_t);
//    void updateWindowTitle();
};


class LV2PluginWrapper: public Plugin
{
private:
    LV2Synth *_synth;
    LADSPA_Descriptor _fakeLd;
    LADSPA_PortDescriptor *_fakePds;       
public:
    LV2PluginWrapper ( LV2Synth *s, PluginFeatures_t reqFeatures = PluginNoFeatures );
    LV2Synth *synth() const;
    virtual ~LV2PluginWrapper();
    virtual LADSPA_Handle instantiate ( PluginI * ) override;
    virtual int incReferences ( int ref ) override;
    virtual void activate ( LADSPA_Handle handle ) override;
    virtual void deactivate ( LADSPA_Handle handle ) override;
    virtual void cleanup ( LADSPA_Handle handle ) override;
    virtual void connectPort ( LADSPA_Handle handle, unsigned long port, float *value ) override;
    virtual void apply ( LADSPA_Handle handle, unsigned long n, float latency_corr = 0.0f ) override;
    virtual LADSPA_PortDescriptor portd ( unsigned long k ) const override;
    virtual LADSPA_PortRangeHint range ( unsigned long i ) const override;
    virtual void range ( unsigned long i, float *min, float *max ) const override;
    virtual double defaultValue ( unsigned long port ) const override;
    virtual const char *portName ( unsigned long i ) const override;
    virtual CtrlValueType ctrlValueType ( unsigned long ) const override;
    virtual const CtrlVal::CtrlEnumValues* ctrlEnumValues ( unsigned long ) const override;
    virtual CtrlList::Mode ctrlMode ( unsigned long ) const override;
    virtual bool hasNativeGui() const;
    virtual void showNativeGui ( PluginI *p, bool bShow );
    virtual bool nativeGuiVisible (const PluginI *p ) const;
// REMOVE Tim. tmp. Added.
    void updateNativeGuiWindowTitle(const PluginI *p) const;
    virtual void setLastStateControls(LADSPA_Handle handle, size_t index, bool bSetMask, bool bSetVal, bool bMask, float fVal);
// REMOVE Tim. tmp. Added.
    virtual QString getCustomConfiguration(LADSPA_Handle handle);
    virtual void writeConfiguration(LADSPA_Handle handle, int level, Xml& xml);
    virtual void setCustomData (LADSPA_Handle handle, const std::vector<QString> & customParams);
    // Returns a value unit string for displaying unit symbols.
    QString unitSymbol(unsigned long ) const override;
    // Returns index into the global value units for displaying unit symbols.
    // Can be -1 meaning no units.
    int valueUnit(unsigned long ) const override;

    void populatePresetsMenu(PluginI *p, MusEGui::PopupMenu *menu);
    void applyPreset(PluginI *p, void *preset);
};

#endif // LV2_SUPPORT

extern void initLV2();

} // namespace MusECore

#endif



