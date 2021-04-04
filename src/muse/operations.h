//=========================================================
//  MusE
//  Linux Music Editor
//    operations.h 
//  (C) Copyright 2014, 2016 Tim E. Real (terminator356 on users dot sourceforge dot net)
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

#ifndef __OPERATIONS_H__
#define __OPERATIONS_H__

#include <list> 
#include <map> 
#include <set>
#include <stdint.h>

#include "type_defs.h"
#include "muse_time.h"
#include "event.h"
#include "midictrl.h" 
#include "ctrl.h"
#include "time_stretch.h"
#include "part.h"
#include "track.h"
#include "route.h"
#include "mididev.h"
#include "marker/marker.h"
#include "instruments/minstrument.h"
#include "wave.h"
#include "pos.h"

namespace MusECore {

// Forward declarations:
class TempoList;
class SigList;
class KeyList;
class MidiPort;
class MetroAccentsMap;
class AudioConverterSettingsGroup;
class AudioConverterPluginI;

typedef std::list < iMidiCtrlValList > MidiCtrlValListIterators_t;
typedef MidiCtrlValListIterators_t::iterator iMidiCtrlValListIterators_t;
typedef MidiCtrlValListIterators_t::const_iterator ciMidiCtrlValListIterators_t;
class MidiCtrlValListIterators : public MidiCtrlValListIterators_t
{
   public:
     iterator findList(const MidiCtrlValList* valList);
     const_iterator findList(const MidiCtrlValList* valList) const;
};

typedef std::map < int /*port*/, MidiCtrlValListIterators, std::less<int> > MidiCtrlValLists2bErased_t;
typedef MidiCtrlValLists2bErased_t::iterator iMidiCtrlValLists2bErased_t;
typedef MidiCtrlValLists2bErased_t::const_iterator ciMidiCtrlValLists2bErased_t;
typedef std::pair<iMidiCtrlValLists2bErased_t, bool> MidiCtrlValLists2bErasedInsertResult_t;
typedef std::pair<int, MidiCtrlValListIterators> MidiCtrlValLists2bErasedInsertPair_t;
typedef std::pair<iMidiCtrlValLists2bErased_t, iMidiCtrlValLists2bErased_t> MidiCtrlValLists2bErasedRangePair_t;

class MidiCtrlValLists2bErased : public MidiCtrlValLists2bErased_t
{
   public:
     void add(int port, const iMidiCtrlValList& item);
     iterator findList(int port, const MidiCtrlValList* valList);
     const_iterator findList(int port, const MidiCtrlValList* valList) const;
};


typedef std::set < MidiCtrlValList* > MidiCtrlValLists2bDeleted_t;
typedef MidiCtrlValLists2bDeleted_t::iterator iMidiCtrlValLists2bDeleted_t;
typedef MidiCtrlValLists2bDeleted_t::const_iterator ciMidiCtrlValLists2bDeleted_t;
class MidiCtrlValLists2bDeleted : public MidiCtrlValLists2bDeleted_t
{
  
};


typedef std::map < int /*port*/, MidiCtrlValListList*, std::less<int> > MidiCtrlValLists2bAdded_t;
typedef MidiCtrlValLists2bAdded_t::iterator iMidiCtrlValLists2bAdded_t;
typedef MidiCtrlValLists2bAdded_t::const_iterator ciMidiCtrlValLists2bAdded_t;
typedef std::pair<iMidiCtrlValLists2bAdded_t, bool> MidiCtrlValLists2bAddedInsertResult_t;
typedef std::pair<int, MidiCtrlValListList*> MidiCtrlValLists2bAddedInsertPair_t;
typedef std::pair<iMidiCtrlValLists2bAdded_t, iMidiCtrlValLists2bAdded_t> MidiCtrlValLists2bAddedRangePair_t;

class MidiCtrlValLists2bAdded : public MidiCtrlValLists2bAdded_t
{
//   public:
//     void add(int port, const MidiCtrlValListList* item);
//     void remove(int port, const MidiCtrlValListList* item);
};


struct MidiCtrlValRemapOperation
{
  // Iterators to be erased in realtime stage.
  MidiCtrlValLists2bErased _midiCtrlValLists2bErased;
  // New items to be added in realtime stage.
  MidiCtrlValLists2bAdded _midiCtrlValLists2bAdded;
  // Orphaned pointers after the iterators have been erased, deleted in post non-realtime stage.
  // Automatically filled by constructor.
  MidiCtrlValLists2bDeleted_t _midiCtrlValLists2bDeleted;
};

struct DrumMapTrackOperation
{
  // Whether this is a setting operation or a reset to defaults.
  bool _isReset;
  bool _isInstrumentMod;
  bool _doWholeMap;
  bool _includeDefault;
  WorkingDrumMapList _workingItemList;
  // List of tracks to apply to.
  MidiTrackList _tracks;
};

struct DrumMapTrackPatchOperation
{
  // Whether to clear the list of overrides.
  bool _clear;
  // Whether this is a setting operation or a reset to defaults.
  bool _isReset;
  bool _isInstrumentMod;
  WorkingDrumMapPatchList _workingItemPatchList;
  // List of tracks to apply to.
  MidiTrackList _tracks;
};

struct DrumMapTrackPatchReplaceOperation
{
  bool _isInstrumentMod;
  WorkingDrumMapPatchList* _workingItemPatchList;
  // Track to apply to.
  MidiTrack* _track;
};

// New items created in GUI thread awaiting addition in audio thread.
struct PendingOperationItem
{
  enum PendingOperationType {
    Uninitialized = 0,
    ModifySongLength,
    AddMidiInstrument, DeleteMidiInstrument, ReplaceMidiInstrument,
    AddMidiDevice,     DeleteMidiDevice,       
    ModifyMidiDeviceAddress,         ModifyMidiDeviceFlags,       ModifyMidiDeviceName,
    SetInstrument,
    AddTrack,          DeleteTrack,  MoveTrack,                   ModifyTrackName,
    SetTrackRecord, SetTrackMute, SetTrackSolo, SetTrackRecMonitor, SetTrackOff,
    ModifyTrackDrumMapItem, ReplaceTrackDrumMapPatchList,         UpdateDrumMaps,
    AddPart,           DeletePart,   MovePart, SelectPart, ModifyPartStart, ModifyPartLength,  ModifyPartName,
    AddEvent,          DeleteEvent,  SelectEvent,  ModifyEventList,
    
    AddMidiCtrlVal,    DeleteMidiCtrlVal,     ModifyMidiCtrlVal,  AddMidiCtrlValList,
    ModifyMidiCtrlValList,

    RemapDrumControllers,
    AddAudioCtrlVal,   DeleteAudioCtrlVal,    ModifyAudioCtrlVal, ModifyAudioCtrlValList,
    ModifyTempoList,   SetStaticTempo,        SetGlobalTempo, 
    ModifySigList,
    ModifyKeyList,

    ModifyDefaultAudioConverterSettings, ModifyLocalAudioConverterSettings, ModifyLocalAudioConverter,
    SetAudioConverterOfflineMode,
    AddStretchListRatioAt,   DeleteStretchListRatioAt,  ModifyStretchListRatioAt,
    ModifyStretchListRatio,

    AddAuxSendValue,   
    AddRoute,          DeleteRoute, 
    AddRouteNode,      DeleteRouteNode,       ModifyRouteNode,
    UpdateSoloStates,
    EnableAllAudioControllers,
    GlobalSelectAllEvents,
    ModifyAudioSamples,
    SwitchMetronomeSettings, ModifyMetronomeAccentMap,
    SetExternalSyncFlag, SetUseJackTransport, SetUseMasterTrack,
    ModifyMarkerList
    }; 
                              
  PendingOperationType _type;

  union {
    Part* _part;
    MidiPort* _midi_port;
    void* _void_track_list;
    int* _audioSamplesLen;
  };
  
  union {
    EventList* _orig_event_list;
    MidiCtrlValList* _orig_mcvl;
    MidiCtrlValListList* _mcvll;
    CtrlListList* _aud_ctrl_list_list;
    TempoList* _orig_tempo_list;
    MusECore::SigList* _orig_sig_list; 
    KeyList* _orig_key_list;
    StretchList* _stretch_list;  
    PartList* _part_list; 
    TrackList* _track_list;
    MidiDeviceList* _midi_device_list;
    MidiInstrumentList* _midi_instrument_list;
    AuxSendValueList* _aux_send_value_list;
    RouteList* _route_list;
    MarkerList** _orig_marker_list;
    float** _audioSamplesPointer;
    MetroAccentsMap** _metroAccentsMap;
  };
            
  union {
    EventList* _event_list;
    MidiInstrument* _midi_instrument;
    MidiDevice* _midi_device;
    Track* _track;
    MidiCtrlValList* _mcvl;
    CtrlList* _aud_ctrl_list;
    MarkerList* _marker_list;
    TempoList* _tempo_list;  
    MusECore::SigList* _sig_list; 
    KeyList* _key_list;
    Route* _dst_route_pointer;
    float* _newAudioSamples;
    bool* _bool_pointer;
    MetroAccentsMap* _newMetroAccentsMap;
    AudioConverterSettingsGroup* _audio_converter_settings;
  };

  iPart _iPart; 
  Event _ev;
  iEvent _iev;
  iMidiCtrlVal _imcv;
  iCtrl _iCtrl;
  iCtrlList _iCtrlList;
  iStretchListItem _iStretchEvent;
  iMidiInstrument _iMidiInstrument;
  iMidiDevice _iMidiDevice;
  iRoute _iRoute;
  iMarker _iMarker;
  Route _src_route;
  Route _dst_route;
  // SndFileR is only 8 bytes but can't be in a union becuase of non-trivial destructor (?).
  SndFileR _sndFileR;
  
  union {
    int _intA;
    unsigned int _uintA;
    unsigned int _posLenVal;
    bool _boolA;
    const QString *_name;
    double _aux_send_value;
    int _insert_at;
    int _from_idx;
    int _address_client;
    int _rw_flags;
    int _newAudioSamplesLen;
    //DrumMapOperation* _drum_map_operation;
    DrumMapTrackOperation* _drum_map_track_operation;
    DrumMapTrackPatchOperation* _drum_map_track_patch_operation;
    DrumMapTrackPatchReplaceOperation* _drum_map_track_patch_replace_operation;
    MidiCtrlValRemapOperation* _midi_ctrl_val_remap_operation;
    MuseFrame_t _museFrame;
    AudioConverterPluginI* _audio_converter;
  };
  
  union {
    int _intB;
    unsigned int _uintB;
    unsigned int _lenVal;
    unsigned int _marker_tick;
    int _to_idx;
    int _address_port;
    int _open_flags;
    int _ctl_num;
    int _stretch_type;
    AudioConverterPluginI* _audio_converter_ui;
  };

  union {
    int _intC;
    unsigned int _uintC;
    int _ctl_val;
    double _ctl_dbl_val;
    double _audio_converter_value;
    bool _marker_lock;
  };

  PendingOperationItem(AudioConverterSettingsGroup* new_settings,
                       PendingOperationType type = ModifyDefaultAudioConverterSettings)
    { _type = type; _audio_converter_settings = new_settings; }
      
  PendingOperationItem(SndFileR sf, AudioConverterSettingsGroup* new_settings, 
                       PendingOperationType type = ModifyLocalAudioConverterSettings)
    { _type = type; _sndFileR = sf; 
      _audio_converter_settings = new_settings; }
      
  PendingOperationItem(SndFileR sf,
                       AudioConverterPluginI* newAudioConverter,
                       AudioConverterPluginI* newAudioConverterUI,
                       PendingOperationType type = ModifyLocalAudioConverter)
    { _type = type; _sndFileR = sf; 
      _audio_converter = newAudioConverter;
      _audio_converter_ui = newAudioConverterUI; }
      
  PendingOperationItem(SndFileR sf, AudioConverterPluginI* newAudioConverter,
                       PendingOperationType type = SetAudioConverterOfflineMode)
    { _type = type; _sndFileR = sf; _audio_converter = newAudioConverter; }


  PendingOperationItem(float** samples, float* new_samples, int* samples_len, int new_samples_len, 
                       PendingOperationType type = ModifyAudioSamples)
    { _type = type; _audioSamplesPointer = samples; _newAudioSamples = new_samples; 
      _audioSamplesLen = samples_len, _newAudioSamplesLen = new_samples_len; }


  // The operation is constructed and allocated in non-realtime before the call, then the controllers modified in realtime stage,
  //  then operation is deleted in non-realtime stage.
  PendingOperationItem(MidiCtrlValRemapOperation* operation, PendingOperationType type = RemapDrumControllers)
    { _type = type; _midi_ctrl_val_remap_operation = operation; }

  // The operation is constructed and allocated in non-realtime before the call, then the track's map is modified in realtime stage,
  //  then operation is deleted in non-realtime stage.
  PendingOperationItem(DrumMapTrackOperation* operation, PendingOperationType type = ModifyTrackDrumMapItem)
    { _type = type; _drum_map_track_operation = operation; }

  // The operation is constructed and allocated in non-realtime before the call, then the track's map is modified in realtime stage,
  //  then operation is deleted in non-realtime stage.
  PendingOperationItem(DrumMapTrackPatchReplaceOperation* operation, PendingOperationType type = ReplaceTrackDrumMapPatchList)
    { _type = type; _drum_map_track_patch_replace_operation = operation; }

  PendingOperationItem(MidiPort* mp, PendingOperationType type = UpdateDrumMaps)
    { _type = type; _midi_port = mp; }
    
  PendingOperationItem(TrackList* tl, PendingOperationType type = UpdateSoloStates)
    { _type = type; _track_list = tl; }
  
  // TODO: Try to break this operation down so that only the actual operation is executed stage-2.
  PendingOperationItem(const Route& src_route, const Route& dst_route, PendingOperationType type) // Type is AddRoute or DeleteRoute.
    { _type = type; _src_route = src_route; _dst_route = dst_route; }
    
  PendingOperationItem(RouteList* route_list, const Route& route, PendingOperationType type = AddRouteNode)
    { _type = type; _route_list = route_list; _src_route = route; }
    
  PendingOperationItem(RouteList* route_list, const iRoute& ir, PendingOperationType type = DeleteRouteNode)
    { _type = type; _route_list = route_list; _iRoute = ir; }
    
  PendingOperationItem(const Route& src_route, Route* dst_route, PendingOperationType type = ModifyRouteNode) 
    { _type = type; _src_route = src_route; _dst_route_pointer = dst_route; }

  PendingOperationItem(AuxSendValueList* asvl, double val, PendingOperationType type = AddAuxSendValue)
    { _type = type; _aux_send_value_list = asvl; _aux_send_value = val; }
    
  PendingOperationItem(MidiInstrumentList* mil, MidiInstrument* midi_instrument, PendingOperationType type = AddMidiInstrument)
    { _type = type; _midi_instrument_list = mil; _midi_instrument = midi_instrument; }
    
  PendingOperationItem(MidiInstrumentList* mil, const iMidiInstrument& imi, PendingOperationType type = DeleteMidiInstrument)
    { _type = type; _midi_instrument_list = mil; _iMidiInstrument = imi; }

  PendingOperationItem(MidiInstrumentList* mil, const iMidiInstrument& imi, MidiInstrument* new_instrument,
                       PendingOperationType type = ReplaceMidiInstrument)
    { _type = type; _midi_instrument_list = mil; _iMidiInstrument = imi; _midi_instrument = new_instrument; }

  PendingOperationItem(MidiDeviceList* mdl, MidiDevice* midi_device, PendingOperationType type = AddMidiDevice)
    { _type = type; _midi_device_list = mdl; _midi_device = midi_device; }
    
  PendingOperationItem(MidiDeviceList* mdl, const iMidiDevice& imd, PendingOperationType type = DeleteMidiDevice)
    { _type = type; _midi_device_list = mdl; _iMidiDevice = imd; }

  // Type is ModifyMidiDeviceAddress or ModifyMidiDeviceFlags  
  PendingOperationItem(MidiDevice* midi_device, int address_client_or_rw_flags, int address_port_or_open_flags, PendingOperationType type)
    { _type = type; _midi_device = midi_device; _intA = address_client_or_rw_flags; _intB = address_port_or_open_flags; }
    
  PendingOperationItem(MidiDevice* midi_device, const QString* new_name, PendingOperationType type = ModifyMidiDeviceName)
    { _type = type; _midi_device = midi_device; _name = new_name; }


  PendingOperationItem(MidiPort* midi_port, MidiInstrument* midi_instrument, PendingOperationType type = SetInstrument)
    { _type = type; _midi_port = midi_port; _midi_instrument = midi_instrument; }


  PendingOperationItem(TrackList* tl, Track* track, int insert_at, PendingOperationType type = AddTrack, void* sec_track_list = 0)
    { _type = type; _track_list = tl; _track = track; _insert_at = insert_at; _void_track_list = sec_track_list; }
    
  PendingOperationItem(TrackList* tl, Track* track, PendingOperationType type = DeleteTrack, void* sec_track_list = 0)
    { _type = type; _track_list = tl; _track = track; _void_track_list = sec_track_list; }
    
  PendingOperationItem(TrackList* tl, int from_idx, int to_idx, PendingOperationType type = MoveTrack)
    { _type = type; _track_list = tl; _from_idx = from_idx; _to_idx = to_idx; }

  PendingOperationItem(TrackList* tl, bool select, unsigned long /*t0*/, unsigned long /*t1*/,
                       PendingOperationType type = GlobalSelectAllEvents)
    { _type = type; _track_list = tl; 
        _boolA = select; }
    
  PendingOperationItem(Track* track, const QString* new_name, PendingOperationType type = ModifyTrackName)
    { _type = type; _track = track; _name = new_name; }
    
   // type is SetTrackRecord, SetTrackMute, SetTrackSolo, SetTrackRecMonitor, SetTrackOff
  PendingOperationItem(Track* track, bool v, PendingOperationType type)
    { _type = type; _track = track; _boolA = v; }
    
    
  PendingOperationItem(Part* part, const QString* new_name, PendingOperationType type = ModifyPartName)
    { _type = type; _part = part; _name = new_name; }

  PendingOperationItem(Part* part, bool v, PendingOperationType type = SelectPart)
    { _type = type; _part = part; _boolA = v; }

  // new_pos and new_len must already be in the part's time domain (ticks or frames).
  // The new_event_list can be set to null, or if the part's events are to be dragged with the border
  //  it can be supplied to do a wholesale fast constant-time swap of the event lists.
  PendingOperationItem(iPart ip, Part* part, unsigned int new_pos, unsigned int new_len,
                       EventList* new_event_list, PendingOperationType type = ModifyPartStart)
    { _type = type; _iPart = ip, _part = part; _event_list = new_event_list; _posLenVal = new_pos; _lenVal = new_len; }

  // new_len must already be in the part's time domain (ticks or frames).
  // The new_event_list can be set to null, or if the part's events are to be dragged with the border
  //  it can be supplied to do a wholesale fast constant-time swap of the event lists.
  PendingOperationItem(iPart ip, Part* part, unsigned int new_len, EventList* new_event_list, PendingOperationType type = ModifyPartLength)
    { _type = type; _iPart = ip, _part = part; _event_list = new_event_list; _posLenVal = new_len; }
  
  // Erases ip from part->track()->parts(), then adds part to new_track. NOTE: ip may be part->track()->parts()->end().
  // new_pos must already be in the part's time domain (ticks or frames).
  PendingOperationItem(iPart ip, Part* part, unsigned int new_pos, PendingOperationType type = MovePart, Track* new_track = 0)
    { _type = type; _iPart = ip; _part = part; _track = new_track; 
        _posLenVal = new_pos;}
    
  PendingOperationItem(PartList* pl, Part* part, PendingOperationType type = AddPart)
    { _type = type; _part_list = pl; _part = part; }
  
  PendingOperationItem(PartList* pl, const iPart& ip, PendingOperationType type = DeletePart)
    { _type = type; _part_list = pl; _iPart = ip; }
    
    
  PendingOperationItem(Part* part, const Event& ev, PendingOperationType type = AddEvent)
    { _type = type; _part = part; _ev = ev; }
    
  // NOTE: To avoid possibly deleting the event in RT stage 2 when the event is erased from the list, 
  //        _ev is used simply to hold a reference until non-RT stage 3 or after, when the list is cleared.
  PendingOperationItem(Part* part, const iEvent& iev, PendingOperationType type = DeleteEvent)
    { _type = type; _part = part; _iev = iev; _ev = iev->second; }

  // Type is SelectEvent, or some (likely) future boolean operation.
  PendingOperationItem(Part* part, const Event& ev, int v, PendingOperationType type)
    { _type = type; _part = part; _ev = ev; _intA = v; }

    
  PendingOperationItem(MidiCtrlValListList* mcvll, MidiCtrlValList* mcvl, int channel, int control_num, PendingOperationType type = AddMidiCtrlValList)
    { _type = type; _mcvll = mcvll; _mcvl = mcvl; _intA = channel; _intB = control_num; }
    
  PendingOperationItem(const iCtrlList& ictl_l, CtrlList* ctrl_l, PendingOperationType type = ModifyAudioCtrlValList)
    { _type = type; _iCtrlList = ictl_l; _aud_ctrl_list = ctrl_l; }
    
  PendingOperationItem(MidiCtrlValList* mcvl, Part* part, unsigned int tick, int val, PendingOperationType type = AddMidiCtrlVal)
    { _type = type; _mcvl = mcvl; _part = part; 
        _posLenVal = tick; _intB = val; }
    
  PendingOperationItem(MidiCtrlValList* mcvl, const iMidiCtrlVal& imcv, PendingOperationType type = DeleteMidiCtrlVal)
    { _type = type; _mcvl = mcvl; _imcv = imcv; }
    
  // NOTE: mcvl is supplied in case the operation needs to be merged, or transformed into an AddMidiCtrlVal.
  PendingOperationItem(MidiCtrlValList* mcvl, const iMidiCtrlVal& imcv, int val, PendingOperationType type = ModifyMidiCtrlVal)
    { _type = type; _mcvl = mcvl; _imcv = imcv; _intA = val; }

    
  PendingOperationItem(MidiCtrlValList* orig_mcvl, MidiCtrlValList* mcvl, PendingOperationType type = ModifyMidiCtrlValList)
    { _type = type; _orig_mcvl = orig_mcvl; _mcvl = mcvl; }
    
  PendingOperationItem(CtrlList* ctrl_l, unsigned int frame, double ctrl_val, PendingOperationType type = AddAudioCtrlVal)
    { _type = type; _aud_ctrl_list = ctrl_l; _posLenVal = frame; _ctl_dbl_val = ctrl_val; }
    
  PendingOperationItem(CtrlList* ctrl_l, const iCtrl& ictl, PendingOperationType type = DeleteAudioCtrlVal)
    { _type = type; _aud_ctrl_list = ctrl_l; _iCtrl = ictl; }
    
  // NOTE: ctrl_l is supplied in case the operation needs to be merged, or transformed into an AddAudioCtrlVal.
  PendingOperationItem(CtrlList* ctrl_l, const iCtrl& ictl, unsigned int new_frame,
                       double new_ctrl_val, PendingOperationType type = ModifyAudioCtrlVal)
    { _type = type; _aud_ctrl_list = ctrl_l; _iCtrl = ictl; _posLenVal = new_frame; _ctl_dbl_val = new_ctrl_val; }
    
  
  // Takes ownership of the original list so it can be deleted in the non-RT stage.
  PendingOperationItem(TempoList* orig_tempo_l, TempoList* new_tempo_l, PendingOperationType type = ModifyTempoList)
    { _type = type; _orig_tempo_list = orig_tempo_l; _tempo_list = new_tempo_l; }
    
  // type is SetGlobalTempo, SetStaticTempo.
  PendingOperationItem(TempoList* tl, int tempo, PendingOperationType type)
    { _type = type; _tempo_list = tl; _intA = tempo; }

  PendingOperationItem(TempoList* tl, bool v, PendingOperationType type = SetUseMasterTrack)
    { _type = type; _tempo_list = tl; _boolA = v; }

    
  // Takes ownership of the original list so it can be deleted in the non-RT stage.
  PendingOperationItem(SigList* orig_sig_l, SigList* new_sig_l, PendingOperationType type = ModifySigList)
    { _type = type; _orig_sig_list = orig_sig_l; _sig_list = new_sig_l; }
    
    
  // Takes ownership of the original list so it can be deleted in the non-RT stage.
  PendingOperationItem(KeyList* orig_key_l, KeyList* new_key_l, PendingOperationType type = ModifyKeyList)
    { _type = type; _orig_key_list = orig_key_l; _key_list = new_key_l; }


  PendingOperationItem(int stretchType, StretchList* sl, MuseFrame_t frame, double ratio, PendingOperationType type = AddStretchListRatioAt)
    { _type = type; _stretch_type = stretchType, _stretch_list = sl; _museFrame = frame; _audio_converter_value = ratio; }
    
  PendingOperationItem(int stretchTypes, StretchList* sl, const iStretchListItem& ise, PendingOperationType type = DeleteStretchListRatioAt)
    { _type = type; _stretch_type = stretchTypes, _stretch_list = sl; _iStretchEvent = ise; }
    
  PendingOperationItem(int stretchType, StretchList* sl, const iStretchListItem& ise, MuseFrame_t new_frame, double ratio, 
                       PendingOperationType type = ModifyStretchListRatioAt)
    { _type = type; _stretch_type = stretchType, _stretch_list = sl; _iStretchEvent = ise; 
      _museFrame = new_frame; _audio_converter_value = ratio; }
  
  PendingOperationItem(int stretchType, StretchList* sl, double ratio, PendingOperationType type = ModifyStretchListRatio)
    { _type = type; _stretch_type = stretchType, _stretch_list = sl; _audio_converter_value = ratio; }
  
  
  PendingOperationItem(unsigned int len, PendingOperationType type = ModifySongLength)
    { _type = type; _posLenVal = len; }

  PendingOperationItem(MetroAccentsMap** old_map, MetroAccentsMap* new_map, PendingOperationType type = ModifyMetronomeAccentMap)
    { _type = type; _metroAccentsMap = old_map; _newMetroAccentsMap = new_map; }

  // Type is SwitchMetronomeSettings, SetExternalSyncFlag, SetUseJackTransport.
  PendingOperationItem(bool* bool_pointer, bool v, PendingOperationType type)
    { _type = type; _bool_pointer = bool_pointer; _boolA = v; }

  PendingOperationItem(PendingOperationType type) // type is EnableAllAudioControllers.
    { _type = type; }

  // Takes ownership of the original list so it can be deleted in the non-RT stage.
  PendingOperationItem(MarkerList** orig_marker_l, MarkerList* new_marker_l, PendingOperationType type = ModifyMarkerList)
    { _type = type; _orig_marker_list = orig_marker_l; _marker_list = new_marker_l; }
    
  PendingOperationItem()
    { _type = Uninitialized; }
    
  // Execute the operation. Called only from RT stage 2.
    SongChangedStruct_t executeRTStage(); 
  // Execute the operation. Called only from post RT stage 3.
    SongChangedStruct_t executeNonRTStage(); 
  // Get an appropriate indexing value from ops like AddEvent that use it. Other ops like AddMidiCtrlValList return their type (rather than say, zero).
  unsigned int getIndex() const;
  // Whether the two special allocating ops (like AddMidiCtrlValList) are the same. 
  // The comparison ignores the actual allocated value, so that such commands can be found before they do their allocating.
  bool isAllocationOp(const PendingOperationItem&) const;
};

class PendingOperationList : public std::list<PendingOperationItem> 
{
  private:
    // Holds sorted version of list. Index is time value for items which have it like events or parts,
    //  otherwise it is the operation type for other items. It doesn't matter too much that ticks and frames
    //  are mixed here, sorting by time is just to speed up searches, we look for operation types.
    std::multimap<unsigned int, iterator, std::less<unsigned int> > _map; 
    // Accumulated song changed flags.
    SongChangedStruct_t _sc_flags;
    
  public: 
    PendingOperationList() : _sc_flags(0) { }
    // Add an operation. Returns false if the operation already exists or could not be added
    //  (such as with a DeleteEvent being cancelled by an AddEvent on identical base events),
    //  or various other optimizations that essentially render the operation ineffectual.
    // Some operations optimize by replacing an existing operation with the requested one.
    // For example requesting operations which simply change a value replace any existing
    //  such operation with the requested new value. Thus only one operation exists - the LATEST.
    // Returns true in those cases so that the caller can proceed to perform other operations
    //  on the item even though the operation was not officially added to the list (it replaced one).
    // Otherwise returns true. Optimizes all added items (merge, discard, alter, embellish etc.)
    bool add(PendingOperationItem);
    // Execute the RT portion of the operations contained in the list. Called only from RT stage 2.
    SongChangedStruct_t executeRTStage();
    // Execute the Non-RT portion of the operations contained in the list. Called only from post RT stage 3.
    SongChangedStruct_t executeNonRTStage();
    // Clear both the list and the map, and flags.
    void clear();
    // Returns the accumulated song changed flags.
    SongChangedStruct_t flags() const { return _sc_flags; }
    // Find an existing special allocation command (like AddMidiCtrlValList). 
    // The comparison ignores the actual allocated value, so that such commands can be found before they do their allocating.
    iterator findAllocationOp(const PendingOperationItem& op);


    void addDeviceOperation(MidiDeviceList* devlist, MidiDevice* dev);

    void addTrackPortCtrlEvents(Track* track);
    void removeTrackPortCtrlEvents(Track* track);

    void addPartPortCtrlEvents(
      const Event& event, Part* part, unsigned int tick, unsigned int /*len*/, Track* track);
    void addPartPortCtrlEvents(Part* part, unsigned int tick, unsigned int len, Track* track);
    bool removePartPortCtrlEvents(const Event& event, Part* part, Track* track);
    void removePartPortCtrlEvents(Part* part, Track* track);
    void modifyPartPortCtrlEvents(const Event& old_event, const Event& event, Part* part);

    void addPartOperation(PartList *partlist, Part* part); 
    void delPartOperation(PartList *partlist, Part* part);
    void movePartOperation(PartList *partlist, Part* part, unsigned int new_pos, Track* track = 0);
    void modifyPartStartOperation(Part* part, unsigned int new_pos, unsigned int new_len, int64_t events_offset, Pos::TType events_offset_time_type);
    void modifyPartLengthOperation(Part* part, unsigned int new_len, int64_t events_offset, Pos::TType events_offset_time_type);

    void addTrackAuxSendOperation(AudioTrack *atrack, int n);
    
    //void TrackMidiCtrlRemapOperation(int index, int newPort, int newChan, int newNote, MidiCtrlValRemapOperation* rmop);
};

typedef PendingOperationList::iterator iPendingOperation;
typedef std::multimap<unsigned int, iPendingOperation, std::less<unsigned int> >::iterator iPendingOperationSorted;
typedef std::multimap<unsigned int, iPendingOperation, std::less<unsigned int> >::reverse_iterator riPendingOperationSorted;
typedef std::pair <iPendingOperationSorted, iPendingOperationSorted> iPendingOperationSortedRange;

extern void TrackMidiCtrlRemapOperation(
  MidiTrack *mtrack, int index, int newPort, int newChan, int newNote, MidiCtrlValRemapOperation* rmop);

} // namespace MusECore

#endif
