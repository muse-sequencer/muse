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

#ifdef LV2_SUPPORT

#include "lilv/lilv.h"
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
#include "lv2/lv2plug.in/ns/extensions/ui/ui.h"
#include "lv2extui.h"

#include "suil/suil.h"

#include <cstring>
#include <iostream>
#include <vector>
#include <map>
#include <string>
#include <utility>
#include <QMutex>
#include <QSemaphore>
#include <QThread>
#include <assert.h>

#include <alsa/asoundlib.h>
#include "midictrl.h"
#include "synth.h"
#include "stringparam.h"

#include "plugin.h"

#define LV2_PLUGIN_SPACE 0x2000

#endif

namespace MusECore
{

#ifdef LV2_SUPPORT

/* LV2EvBuf class is based of lv2_evbuf_* functions
 * from jalv lv2 plugin host
 *
 * Copyright 2008-2012 David Robillard <http://drobilla.net>

  Permission to use, copy, modify, and/or distribute this software for any
  purpose with or without fee is hereby granted, provided that the above
  copyright notice and this permission notice appear in all copies.

  THIS SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
  WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
  MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
  ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
  WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
  ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
  OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
*/

class LV2EvBuf
{

public:

    /**
       Format of actual buffer.
    */
    typedef enum {
        /**
           An (old) ev:EventBuffer (LV2_Event_Buffer).
        */
        LV2_EVBUF_EVENT,

        /**
           A (new) atom:Sequence (LV2_Atom_Sequence).
        */
        LV2_EVBUF_ATOM
    } LV2_Evbuf_Type;

    typedef struct {
        LV2_Evbuf_Type type;
        uint32_t       capacity;
        uint32_t       atom_Chunk;
        uint32_t       atom_Sequence;
        union {
            LV2_Event_Buffer  event;
            LV2_Atom_Sequence atom;
        } buf;
    } LV2_Evbuf;

    /**
    An iterator over an LV2_Evbuf.
    */
    class LV2_Evbuf_Iterator
    {
    private:
        LV2EvBuf *lv2evbuf;

    public:
        uint32_t   offset;
        LV2_Evbuf_Iterator ( LV2EvBuf *a, uint32_t b ) : lv2evbuf ( a ), offset ( b ) {

        }
        bool
        lv2_evbuf_is_valid () {
            return offset < lv2evbuf->lv2_evbuf_get_size ();
        }
        LV2EvBuf *operator *() {
            return lv2evbuf;
        }
        LV2_Evbuf_Iterator &operator++ () {
            if ( !lv2_evbuf_is_valid () ) {
                return *this;
            }

            LV2_Evbuf *_evbuf  = lv2evbuf->evbuf;
            uint32_t   _offset = offset;
            uint32_t   _size;
            switch ( _evbuf->type ) {
            case LV2_EVBUF_EVENT:
                _size    = ( ( LV2_Event * ) ( _evbuf->buf.event.data + _offset ) )->size;
                _offset += LV2EvBuf::lv2_evbuf_pad_size ( sizeof ( LV2_Event ) + _size );
                break;
            case LV2_EVBUF_ATOM:
                _size = ( ( LV2_Atom_Event * )
                          ( ( char * ) LV2_ATOM_CONTENTS ( LV2_Atom_Sequence, &_evbuf->buf.atom )
                            + _offset ) )->body.size;
                _offset += LV2EvBuf::lv2_evbuf_pad_size ( sizeof ( LV2_Atom_Event ) + _size );
                break;
            }
            offset = _offset;
            return *this;
        }
        LV2_Event *event() {
            LV2_Event_Buffer *ebuf = &lv2evbuf->evbuf->buf.event;
            return ( LV2_Event * ) ( ( char * ) ebuf->data + offset );
        }

        LV2_Atom_Event *aevent() {
            LV2_Atom_Sequence *aseq = ( LV2_Atom_Sequence * ) &lv2evbuf->evbuf->buf.atom;
            return ( LV2_Atom_Event * ) (
                       ( char * ) LV2_ATOM_CONTENTS ( LV2_Atom_Sequence, aseq )
                       + offset );
        }
    };

    LV2_Evbuf *evbuf;

    static inline uint32_t
    lv2_evbuf_pad_size ( uint32_t size ) {
        return ( size + 7 ) & ( ~7 );
    }

    LV2EvBuf ( uint32_t       capacity,
               LV2_Evbuf_Type type,
               uint32_t       atom_Chunk,
               uint32_t       atom_Sequence ) {

        int rv = posix_memalign ( ( void ** ) &evbuf, 8, sizeof ( LV2_Evbuf ) + sizeof ( LV2_Atom_Sequence ) + capacity );

        if ( rv != 0 ) {
            fprintf ( stderr, "ERROR: LV2EvBuf::LV2EvBuf: posix_memalign returned error:%d. Aborting!\n", rv );
            abort();
        }
        evbuf->capacity      = capacity;
        evbuf->atom_Chunk    = atom_Chunk;
        evbuf->atom_Sequence = atom_Sequence;
        lv2_evbuf_set_type ( type );
        lv2_evbuf_reset ( true );
    }

    ~LV2EvBuf() {
        free ( evbuf );
    }

    void
    lv2_evbuf_set_type ( LV2_Evbuf_Type type ) {
        evbuf->type = type;
        switch ( type ) {
        case LV2_EVBUF_EVENT:
            evbuf->buf.event.data     = ( uint8_t * ) ( evbuf + 1 );
            evbuf->buf.event.capacity = evbuf->capacity;
            break;
        case LV2_EVBUF_ATOM:
            break;
        }
        lv2_evbuf_reset ( true );
    }

    void
    lv2_evbuf_reset ( bool input ) {
        switch ( evbuf->type ) {
        case LV2_EVBUF_EVENT:
            evbuf->buf.event.header_size = sizeof ( LV2_Event_Buffer );
            evbuf->buf.event.stamp_type  = LV2_EVENT_AUDIO_STAMP;
            evbuf->buf.event.event_count = 0;
            evbuf->buf.event.size        = 0;
            break;
        case LV2_EVBUF_ATOM:
            if ( input ) {
                evbuf->buf.atom.atom.size = sizeof ( LV2_Atom_Sequence_Body );
                evbuf->buf.atom.atom.type = evbuf->atom_Sequence;
            } else {
                evbuf->buf.atom.atom.size = evbuf->capacity;
                evbuf->buf.atom.atom.type = evbuf->atom_Chunk;
            }
        }
    }

    uint32_t
    lv2_evbuf_get_size () {
        switch ( evbuf->type ) {
        case LV2_EVBUF_EVENT:
            return evbuf->buf.event.size;
        case LV2_EVBUF_ATOM:
            assert ( evbuf->buf.atom.atom.type != evbuf->atom_Sequence
                     || evbuf->buf.atom.atom.size >= sizeof ( LV2_Atom_Sequence_Body ) );
            return evbuf->buf.atom.atom.type == evbuf->atom_Sequence
                   ? evbuf->buf.atom.atom.size - sizeof ( LV2_Atom_Sequence_Body )
                   : 0;
        }
        return 0;
    }

    void *
    lv2_evbuf_get_buffer () {
        switch ( evbuf->type ) {
        case LV2_EVBUF_EVENT:
            return &evbuf->buf.event;
        case LV2_EVBUF_ATOM:
            return &evbuf->buf.atom;
        }
        return NULL;
    }

    LV2_Evbuf_Iterator
    lv2_evbuf_begin () {
        LV2_Evbuf_Iterator iter ( this, 0 );
        return iter;
    }

    LV2_Evbuf_Iterator
    lv2_evbuf_end () {
        const uint32_t           size = lv2_evbuf_get_size ();
        const LV2_Evbuf_Iterator iter ( this, lv2_evbuf_pad_size ( size ) );
        return iter;
    }

    bool
    lv2_evbuf_get ( LV2_Evbuf_Iterator &iter,
                    uint32_t          *frames,
                    uint32_t          *subframes,
                    uint32_t          *type,
                    uint32_t          *size,
                    uint8_t          **data ) const {
        *frames = *subframes = *type = *size = 0;
        *data = NULL;

        if ( !iter.lv2_evbuf_is_valid() ) {
            return false;
        }

        LV2_Event         *ev;
        LV2_Atom_Event    *aev;
        switch ( ( *iter )->evbuf->type ) {
        case LV2_EVBUF_EVENT:
            ev = iter.event();
            *frames    = ev->frames;
            *subframes = ev->subframes;
            *type      = ev->type;
            *size      = ev->size;
            *data      = ( uint8_t * ) ev + sizeof ( LV2_Event );
            break;
        case LV2_EVBUF_ATOM:
            aev = iter.aevent();
            *frames    = aev->time.frames;
            *subframes = 0;
            *type      = aev->body.type;
            *size      = aev->body.size;
            *data      = ( uint8_t * ) LV2_ATOM_BODY ( &aev->body );
            break;
        }

        return true;
    }

    bool
    lv2_evbuf_write ( LV2_Evbuf_Iterator &iter,
                      uint32_t            frames,
                      uint32_t            subframes,
                      uint32_t            type,
                      uint32_t            size,
                      const uint8_t      *data ) const {
        LV2_Event_Buffer  *ebuf;
        LV2_Event         *ev;
        LV2_Atom_Sequence *aseq;
        LV2_Atom_Event    *aev;
        switch ( ( *iter )->evbuf->type ) {
        case LV2_EVBUF_EVENT:
            ebuf = & ( *iter )->evbuf->buf.event;
            if ( ebuf->capacity - ebuf->size < sizeof ( LV2_Event ) + size ) {
                return false;
            }

            ev = ( LV2_Event * ) ( ebuf->data + iter.offset );
            ev->frames    = frames;
            ev->subframes = subframes;
            ev->type      = type;
            ev->size      = size;
            memcpy ( ( uint8_t * ) ev + sizeof ( LV2_Event ), data, size );

            size               = lv2_evbuf_pad_size ( sizeof ( LV2_Event ) + size );
            ebuf->size        += size;
            ebuf->event_count += 1;
            iter.offset      += size;
            break;
        case LV2_EVBUF_ATOM:
            aseq = ( LV2_Atom_Sequence * ) & ( *iter )->evbuf->buf.atom;
            if ( ( *iter )->evbuf->capacity - sizeof ( LV2_Atom ) - aseq->atom.size
                    < sizeof ( LV2_Atom_Event ) + size ) {
                return false;
            }

            aev = ( LV2_Atom_Event * ) (
                      ( char * ) LV2_ATOM_CONTENTS ( LV2_Atom_Sequence, aseq )
                      + iter.offset );
            aev->time.frames = frames;
            aev->body.type   = type;
            aev->body.size   = size;
            memcpy ( LV2_ATOM_BODY ( &aev->body ), data, size );

            size             = lv2_evbuf_pad_size ( sizeof ( LV2_Atom_Event ) + size );
            aseq->atom.size += size;
            iter.offset    += size;
            break;
        }

        return true;
    }


};



struct LV2MidiPort {
    LV2MidiPort ( const LilvPort *_p, uint32_t _i, QString _n, bool _f, LV2EvBuf *_b ) :
        port ( _p ), index ( _i ), name ( _n ), old_api ( _f ), buffer ( _b ) {}
    const LilvPort *port;
    uint32_t index; //plugin real port index
    QString name;
    bool old_api; //true for LV2_Event port
    LV2EvBuf *buffer;
    ~LV2MidiPort() {
        std::cerr << "~LV2MidiPort()" << std::endl;
        delete buffer;
    }
};

struct LV2ControlPort {
    LV2ControlPort ( const LilvPort *_p, uint32_t _i, float _c, QString _n ) :
        port ( _p ), index ( _i ), defVal ( _c ), minVal( _c ), maxVal ( _c ), name ( _n ) {
        cName = strdup ( name.toUtf8().constData() );
    };
    LV2ControlPort ( const LV2ControlPort &other ) :
        port ( other.port ), index ( other.index ), defVal ( other.defVal ), minVal(other.minVal), maxVal(other.maxVal), name ( other.name ) {
        cName = strdup ( name.toUtf8().constData() );
    };
    ~LV2ControlPort() {
        free ( cName );
        cName = NULL;
    };
    const LilvPort *port;
    uint32_t index; //plugin real port index
    float defVal; //default control value
    float minVal; //minimum control value
    float maxVal; //maximum control value
    QString name; //name of this port
    char *cName; //cached value to share beetween function calls
};

struct LV2AudioPort {
    LV2AudioPort ( const LilvPort *_p, uint32_t _i, float *_b, QString _n ) :
        port ( _p ), index ( _i ), buffer ( _b ), name ( _n ) {}
    const LilvPort *port;
    uint32_t index; //plugin real port index
    float *buffer; //audio buffer
    QString name;
};

struct cmp_str {
    bool operator() ( char const *a, char const *b ) const {
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
    LV2UridBiMap() : nextId ( 1 ) {_map.clear(); _rmap.clear();}
    ~LV2UridBiMap()
    {
       LV2_SYNTH_URID_MAP::iterator it = _map.begin();
       for(;it != _map.end(); ++it)
       {
          free((void*)(*it).first);
       }
    }

    LV2_URID map ( const char *uri ) {
        std::pair<LV2_SYNTH_URID_MAP::iterator, bool> p;
        uint32_t id;        
        idLock.lock();
        LV2_SYNTH_URID_MAP::iterator it = _map.find(uri);
        if(it == _map.end())
        {
            const char *mUri = strdup(uri);
            p = _map.insert ( std::make_pair ( mUri, nextId ) );
            _rmap.insert ( std::make_pair ( nextId, mUri ) );
            nextId++;
            id = p.first->second;
        }
        else
           id = it->second;
        idLock.unlock();
        return id;

    }
    const char *unmap ( uint32_t id ) {
        LV2_SYNTH_URID_RMAP::iterator it = _rmap.find ( id );
        if ( it != _rmap.end() ) {
            return it->second;
        }

        return NULL;
    }
};

class LV2SynthIF;
class LV2PluginWrapper_Timer;
class LV2PluginWrapper_State;

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
    bool _isSynth;
    int _uniqueID;
    uint32_t _midi_event_id;
    bool _hasGui;
    bool _hasExternalGui;
    bool _hasExternalGuiDepreceated;
    LilvUIs *_uis;
    const LilvUI *_selectedUi;
    std::map<uint32_t, uint32_t> _idxToControlMap;

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
    uint32_t _fUiHost;
    uint32_t _fDataAccess;
    uint32_t _fWrkSchedule;
    SuilHost *_uiHost;
    const LilvNode *_pluginUIType = NULL;
public:
    virtual Type synthType() const {
        return LV2_SYNTH;
    }
    LV2Synth ( const QFileInfo &fi, QString label, QString name, QString author, const LilvPlugin *_plugin );
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
    static void lv2ui_ShowNativeGui ( LV2PluginWrapper_State *state, bool bShow );
    static void lv2ui_PortWrite ( SuilController controller, uint32_t port_index, uint32_t buffer_size, uint32_t protocol, void const *buffer );
    static void lv2ui_Touch (SuilController controller, uint32_t port_index, bool grabbed);
    static void lv2ui_ExtUi_Closed ( LV2UI_Controller contr );
    static void lv2ui_SendChangedControls(LV2PluginWrapper_State *state);
    static void lv2state_FillFeatures ( LV2PluginWrapper_State *state );
    static void lv2state_PostInstantiate ( LV2PluginWrapper_State *state );
    static void lv2state_FreeState(LV2PluginWrapper_State *state);
    static const void *lv2state_stateRetreive ( LV2_State_Handle handle, uint32_t key, size_t *size, uint32_t *type, uint32_t *flags );
    static LV2_State_Status lv2state_stateStore ( LV2_State_Handle handle, uint32_t key, const void *value, size_t size, uint32_t type, uint32_t flags );
    static LV2_Worker_Status lv2wrk_scheduleWork(LV2_Worker_Schedule_Handle handle, uint32_t size, const void *data);
    static LV2_Worker_Status lv2wrk_respond(LV2_Worker_Respond_Handle handle, uint32_t size, const void* data);
    friend class LV2SynthIF;
    friend class LV2PluginWrapper;
    friend class LV2PluginWrapper_Timer;
    friend class LV2SynthIF_Timer;


};


class LV2SynthIF : public SynthIF
{
private:
    LV2Synth *_synth;
    LilvInstance *_handle;
    LV2_MIDI_PORTS _midiInPorts;
    LV2_MIDI_PORTS _midiOutPorts;
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
    std::map<QString, size_t> _controlsNameMap;    
    float **_audioInBuffers;
    float **_audioOutBuffers;
    float  *_audioInSilenceBuf; // Just all zeros all the time, so we don't have to clear for silence.
    std::vector<unsigned long> _iUsedIdx;  // During process, tells whether an audio input port was used by any input routes.
    snd_midi_event_t *_midiEvent;
    bool processEvent ( const MidiPlayEvent &, snd_seq_event_t * );
    bool lv2MidiControlValues ( size_t port, int ctlnum, int *min, int *max, int *def );
    float midi2Lv2Value ( unsigned long port, int ctlnum, int val );
    LV2PluginWrapper_State *_uiState;
public:
    LV2SynthIF ( SynthI *s );
    virtual ~LV2SynthIF();

    //virtual methods from SynthIF
    virtual bool initGui();
    virtual void guiHeartBeat();
    virtual bool guiVisible() const;
    virtual void showGui ( bool v );
    virtual bool hasGui() const;
    virtual bool nativeGuiVisible() const;
    virtual void showNativeGui ( bool v );
    virtual bool hasNativeGui() const;
    virtual void getGeometry ( int *, int *, int *, int * ) const;
    virtual void setGeometry ( int, int, int, int );
    virtual void getNativeGeometry ( int *, int *, int *, int * ) const;
    virtual void setNativeGeometry ( int, int, int, int );
    virtual void preProcessAlways();
    virtual iMPEvent getData ( MidiPort *, MPEventList *, iMPEvent, unsigned pos, int ports, unsigned n, float **buffer );
    virtual bool putEvent ( const MidiPlayEvent &ev );
    virtual MidiPlayEvent receiveEvent();
    virtual int eventsPending() const;

    virtual int channels() const;
    virtual int totalOutChannels() const;
    virtual int totalInChannels() const;
    void activate();
    virtual void deactivate();
    virtual void deactivate3();
    virtual QString getPatchName ( int, int, bool ) const;
    virtual void populatePatchPopup ( MusEGui::PopupMenu *, int, bool );
    virtual void write ( int level, Xml &xml ) const;
    virtual float getParameter ( unsigned long idx ) const;
    virtual float getParameterOut ( unsigned long n ) const;
    virtual void setParameter ( unsigned long idx, float value );
    virtual int getControllerInfo ( int id, const char **name, int *ctrl, int *min, int *max, int *initval );

    virtual void writeConfiguration ( int level, Xml &xml );
    virtual bool readConfiguration ( Xml &xml, bool readPreset=false );

    virtual void setCustomData ( const std::vector<QString> & );


    unsigned long parameters() const;
    unsigned long parametersOut() const;
    void setParam ( unsigned long i, float val );
    float param ( unsigned long i ) const;
    float paramOut ( unsigned long i ) const;
    const char *paramName ( unsigned long i );
    const char *paramOutName ( unsigned long i );


    int id() {
        return MAX_PLUGINS + LV2_PLUGIN_SPACE;
    }

    friend class LV2Synth;
    friend class LV2PluginWrapper_Timer;
};


class LV2PluginWrapper;
class LV2PluginWrapper_Timer;
class LV2PluginWrapper_Worker;

struct LV2PluginWrapper_State {
   LV2PluginWrapper_State():
      _ifeatures(NULL),
      _ppifeatures(NULL),
      uiHost(NULL),
      widget(NULL),      
      handle(NULL),
      uiInst(NULL),
      inst(NULL),
      lastControls(NULL),
      controlsMask(NULL),
      lastControlsOut(NULL),
      plugInst(NULL),
      sif(NULL),
      synth(NULL),
      human_id(NULL),
      uiTimer(NULL),
      iState(NULL),
      tmpValues(NULL),
      numStateValues(0),
      wrkDataSize(0),
      wrkDataBuffer(0),
      wrkThread(NULL),
      wrkEndWork(false),
      controlTimers(NULL),
      //guiLock(),
      deleteLater(false)
   {
      extHost.plugin_human_id = NULL;
      extHost.ui_closed = NULL;
   }

    LV2_Feature *_ifeatures;
    LV2_Feature **_ppifeatures;
    SuilHost *uiHost;
    void *widget;
    LV2_External_UI_Host extHost;
    LV2_Extension_Data_Feature extData;
    LV2_Worker_Schedule wrkSched;
    LilvInstance *handle;
    SuilInstance *uiInst;
    LV2PluginWrapper *inst;
    float *lastControls;
    bool *controlsMask;
    float *lastControlsOut;
    PluginI *plugInst;
    LV2SynthIF *sif;
    LV2Synth *synth;
    char *human_id;
    LV2PluginWrapper_Timer *uiTimer;
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
    //QMutex guiLock;
    bool deleteLater;
};

class LV2PluginWrapper_Timer :public QThread
{
   Q_OBJECT
private:
    LV2PluginWrapper_State *_state;
    bool _bRunning;
    Port *_controls;
    uint32_t _numControls;
    int _msec;    
public:
    explicit LV2PluginWrapper_Timer ( LV2PluginWrapper_State *s );
    void run();
    bool stopNextTime(bool _wait = true);
    void start ( int msec );
 public slots:
    void doDeleteTimer();
 signals:
    void deletePending();

};

class LV2PluginWrapper_Worker :public QThread
{
private:
    LV2PluginWrapper_State *_state;
    QSemaphore _mSem;
public:
    explicit LV2PluginWrapper_Worker ( LV2PluginWrapper_State *s ) : QThread(),
       _state ( s ),
       _mSem(0)
    {}

    void run();
    LV2_Worker_Status scheduleWork();
    void makeWork();
};



class LV2PluginWrapper_Window : public QMainWindow
{
   Q_OBJECT
protected:
   void closeEvent ( QCloseEvent *event );
private:
   LV2PluginWrapper_State *_state;
   bool _closing;
public:
   explicit LV2PluginWrapper_Window ( LV2PluginWrapper_State *state );

   void doChangeControls();
   void setClosing(bool closing) {_closing = closing; }
public slots:
   void sendChangedControls();
signals:
   void controlsChangePending();


};


class LV2PluginWrapper: public Plugin
{
private:
    LV2Synth *_synth;
    std::map<void *, LV2PluginWrapper_State *> _states;
    SuilHost *_uiHost;
    LADSPA_Descriptor _fakeLd;
    LADSPA_PortDescriptor *_fakePds;
    float *_PluginControlsDefault;
    float *_PluginControlsMin;
    float *_PluginControlsMax;
public:
    LV2PluginWrapper ( LV2Synth *s );
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

    virtual float defaultValue ( unsigned long port ) const;
    virtual const char *portName ( unsigned long i );
    virtual CtrlValueType ctrlValueType ( unsigned long ) const;
    virtual CtrlList::Mode ctrlMode ( unsigned long ) const;
    virtual bool hasNativeGui();
    virtual void showNativeGui ( PluginI *p, bool bShow );
    virtual bool nativeGuiVisible ( PluginI *p );
    virtual void setLastStateControls(LADSPA_Handle handle, size_t index, bool bSetMask, bool bSetVal, bool bMask, float fVal);
};

#endif // LV2_SUPPORT

extern void initLV2();

} // namespace MusECore

#endif



