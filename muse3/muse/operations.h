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

#include "type_defs.h"
#include "event.h"
#include "midictrl.h" 
#include "ctrl.h"
#include "tempo.h" 
#include "sig.h" 
#include "keyevent.h"
#include "part.h"
#include "track.h"
#include "midiedit/drummap.h"
#include "route.h"
#include "mididev.h"
#include "midiport.h"
#include "instruments/minstrument.h"

namespace MusECore {


typedef std::list < iMidiCtrlValList > MidiCtrlValListIterators_t;
typedef MidiCtrlValListIterators_t::iterator iMidiCtrlValListIterators_t;
typedef MidiCtrlValListIterators_t::const_iterator ciMidiCtrlValListIterators_t;
class MidiCtrlValListIterators : public MidiCtrlValListIterators_t
{
   public:
     iterator findList(const MidiCtrlValList* valList)
     {
       for(iterator i = begin(); i != end(); ++i)
         if((*i)->second == valList)
           return i;
       return end();
     }

     const_iterator findList(const MidiCtrlValList* valList) const
     {
       for(const_iterator i = begin(); i != end(); ++i)
         if((*i)->second == valList)
           return i;
       return end();
     }
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
     void add(int port, const iMidiCtrlValList& item)
     {
       iterator i = find(port);
       if(i == end())
       {
         MidiCtrlValListIterators mcvli;
         mcvli.push_back(item);
         insert(MidiCtrlValLists2bErasedInsertPair_t(port, mcvli));
         return;
       }
       MidiCtrlValListIterators& mcvli = i->second;
       for(iMidiCtrlValListIterators_t imcvli = mcvli.begin(); imcvli != mcvli.end(); ++imcvli)
       {
         iMidiCtrlValList imcvl = *imcvli;
         // Compare list pointers.
         if(imcvl->second == item->second)
           return; // Already exists.
       }
       mcvli.push_back(item);
     }

     iterator findList(int port, const MidiCtrlValList* valList)
     {
       iterator i = find(port);
       if(i == end())
         return end();
       if(i->second.findList(valList) != i->second.end())
         return i;
       return end();
     }

     const_iterator findList(int port, const MidiCtrlValList* valList) const
     {
       const_iterator i = find(port);
       if(i == end())
         return end();
       if(i->second.findList(valList) != i->second.end())
         return i;
       return end();
     }
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
  enum PendingOperationType { Uninitialized = 0,
                              ModifySongLength,
                              AddMidiInstrument, DeleteMidiInstrument, ReplaceMidiInstrument,
                              AddMidiDevice,     DeleteMidiDevice,       
                              ModifyMidiDeviceAddress,         ModifyMidiDeviceFlags,       ModifyMidiDeviceName,
                              AddTrack,          DeleteTrack,  MoveTrack,                   ModifyTrackName,
                              SetTrackRecord, SetTrackMute, SetTrackSolo, SetTrackRecMonitor, SetTrackOff,
                              ModifyTrackDrumMapItem, ReplaceTrackDrumMapPatchList,         UpdateDrumMaps,
                              AddPart,           DeletePart,   MovePart, SelectPart, ModifyPartLength,  ModifyPartName,
                              AddEvent,          DeleteEvent,  SelectEvent,
                              AddMidiCtrlVal,    DeleteMidiCtrlVal,     ModifyMidiCtrlVal,  AddMidiCtrlValList,
                              RemapDrumControllers,
                              AddAudioCtrlVal,   DeleteAudioCtrlVal,    ModifyAudioCtrlVal, ModifyAudioCtrlValList,
                              AddTempo,          DeleteTempo,           ModifyTempo,        SetStaticTempo,
                              SetGlobalTempo, 
                              AddSig,            DeleteSig,             ModifySig,
                              AddKey,            DeleteKey,             ModifyKey,
                              AddAuxSendValue,   
                              AddRoute,          DeleteRoute, 
                              AddRouteNode,      DeleteRouteNode,       ModifyRouteNode,
                              UpdateSoloStates,
                              EnableAllAudioControllers,
                              GlobalSelectAllEvents,
                              ModifyAudioSamples
                              }; 
                              
  PendingOperationType _type;

  union {
    Part* _part;
    MidiPort* _midi_port;
    void* _void_track_list;
    int* _audioSamplesLen;
  };
  
  union {
    MidiCtrlValListList* _mcvll;
    CtrlListList* _aud_ctrl_list_list;
    TempoList* _tempo_list;  
    MusECore::SigList* _sig_list; 
    KeyList* _key_list;
    PartList* _part_list; 
    TrackList* _track_list;
    MidiDeviceList* _midi_device_list;
    MidiInstrumentList* _midi_instrument_list;
    AuxSendValueList* _aux_send_value_list;
    RouteList* _route_list;
    float** _audioSamplesPointer;
  };
            
  union {
    MidiInstrument* _midi_instrument;
    MidiDevice* _midi_device;
    Track* _track;
    MidiCtrlValList* _mcvl;
    CtrlList* _aud_ctrl_list;
    TEvent* _tempo_event; 
    MusECore::SigEvent* _sig_event; 
    Route* _dst_route_pointer;
    float* _newAudioSamples;
  };

  iPart _iPart; 
  Event _ev;
  iEvent _iev;
  iMidiCtrlVal _imcv;
  iCtrl _iCtrl;
  iCtrlList _iCtrlList;
  iTEvent _iTEvent;
  MusECore::iSigEvent _iSigEvent;
  iKeyEvent _iKeyEvent;
  iMidiInstrument _iMidiInstrument;
  iMidiDevice _iMidiDevice;
  iRoute _iRoute;
  Route _src_route;
  Route _dst_route;
  
  union {
    int _intA;
    bool _boolA;
    bool _select;
    const QString *_name;
    double _aux_send_value;
    int _insert_at;
    int _from_idx;
    int _address_client;
    int _rw_flags;
    int _frame;
    int _newAudioSamplesLen;
    //DrumMapOperation* _drum_map_operation;
    DrumMapTrackOperation* _drum_map_track_operation;
    DrumMapTrackPatchOperation* _drum_map_track_patch_operation;
    DrumMapTrackPatchReplaceOperation* _drum_map_track_patch_replace_operation;
    MidiCtrlValRemapOperation* _midi_ctrl_val_remap_operation;
  };
  
  union {
    int _intB;
    int _to_idx;
    int _address_port;
    int _open_flags;
    int _ctl_num;
  };

  union {
    int _intC;
    int _ctl_val;
    double _ctl_dbl_val;
  };

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

    
  PendingOperationItem(TrackList* tl, Track* track, int insert_at, PendingOperationType type = AddTrack, void* sec_track_list = 0)
    { _type = type; _track_list = tl; _track = track; _insert_at = insert_at; _void_track_list = sec_track_list; }
    
  PendingOperationItem(TrackList* tl, Track* track, PendingOperationType type = DeleteTrack, void* sec_track_list = 0)
    { _type = type; _track_list = tl; _track = track; _void_track_list = sec_track_list; }
    
  PendingOperationItem(TrackList* tl, int from_idx, int to_idx, PendingOperationType type = MoveTrack)
    { _type = type; _track_list = tl; _from_idx = from_idx; _to_idx = to_idx; }

  PendingOperationItem(TrackList* tl, bool select, unsigned long /*t0*/, unsigned long /*t1*/,
                       PendingOperationType type = GlobalSelectAllEvents)
    { _type = type; _track_list = tl; _select = select; }
    
  PendingOperationItem(Track* track, const QString* new_name, PendingOperationType type = ModifyTrackName)
    { _type = type; _track = track; _name = new_name; }
    
   // type is SetTrackRecord, SetTrackMute, SetTrackSolo, SetTrackRecMonitor, SetTrackOff
  PendingOperationItem(Track* track, bool v, PendingOperationType type)
    { _type = type; _track = track; _boolA = v; }
    
    
  PendingOperationItem(Part* part, const QString* new_name, PendingOperationType type = ModifyPartName)
    { _type = type; _part = part; _name = new_name; }
    
  // Type is ModifyPartLength or SelectPart, or some (likely) future boolean or int operation.
  // For ModifyPartLength, v must already be in the part's time domain (ticks or frames).
  PendingOperationItem(Part* part, int v, PendingOperationType type)
    { _type = type; _part = part; _intA = v; }
  
  // Erases ip from part->track()->parts(), then adds part to new_track. NOTE: ip may be part->track()->parts()->end().
  // new_pos must already be in the part's time domain (ticks or frames).
  PendingOperationItem(iPart ip, Part* part, int new_pos, PendingOperationType type = MovePart, Track* new_track = 0)
    { _type = type; _iPart = ip; _part = part; _track = new_track; _intA = new_pos;}
    
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
    
  PendingOperationItem(MidiCtrlValList* mcvl, Part* part, int tick, int val, PendingOperationType type = AddMidiCtrlVal)
    { _type = type; _mcvl = mcvl; _part = part; _intA = tick; _intB = val; }
    
  PendingOperationItem(MidiCtrlValList* mcvl, const iMidiCtrlVal& imcv, PendingOperationType type = DeleteMidiCtrlVal)
    { _type = type; _mcvl = mcvl; _imcv = imcv; }
    
  // NOTE: mcvl is supplied in case the operation needs to be merged, or transformed into an AddMidiCtrlVal.
  PendingOperationItem(MidiCtrlValList* mcvl, const iMidiCtrlVal& imcv, int val, PendingOperationType type = ModifyMidiCtrlVal)
    { _type = type; _mcvl = mcvl; _imcv = imcv; _intA = val; }

    
  PendingOperationItem(const iCtrlList& ictl_l, CtrlList* ctrl_l, PendingOperationType type = ModifyAudioCtrlValList)
    { _type = type; _iCtrlList = ictl_l; _aud_ctrl_list = ctrl_l; }
    
  PendingOperationItem(CtrlList* ctrl_l, int frame, double ctrl_val, PendingOperationType type = AddAudioCtrlVal)
    { _type = type; _aud_ctrl_list = ctrl_l; _frame = frame; _ctl_dbl_val = ctrl_val; }
    
  PendingOperationItem(CtrlList* ctrl_l, const iCtrl& ictl, PendingOperationType type = DeleteAudioCtrlVal)
    { _type = type; _aud_ctrl_list = ctrl_l; _iCtrl = ictl; }
    
  // NOTE: ctrl_l is supplied in case the operation needs to be merged, or transformed into an AddAudioCtrlVal.
  PendingOperationItem(CtrlList* ctrl_l, const iCtrl& ictl, int new_frame, double new_ctrl_val, PendingOperationType type = ModifyAudioCtrlVal)
    { _type = type; _aud_ctrl_list = ctrl_l; _iCtrl = ictl; _frame = new_frame; _ctl_dbl_val = new_ctrl_val; }
    
  
  // NOTE: 'tick' is the desired tick. te is a new TEvent with tempo and (same) desired tick. Swapping with NEXT event is done.
  PendingOperationItem(TempoList* tl, TEvent* te, int tick, PendingOperationType type = AddTempo)
    { _type = type; _tempo_list = tl; _tempo_event = te; _intA = tick; }
    
  // NOTE: _tempo_event is required. We must erase 'ite' in stage 2, then delete the TEvent* in stage 3 (not stage 1),
  //        so 'ite' is unavailable to fetch the TEvent* from it (in ite->second).
  PendingOperationItem(TempoList* tl, const iTEvent& ite, PendingOperationType type = DeleteTempo)
    { _type = type; _tempo_list = tl; _iTEvent = ite; _tempo_event = ite->second; }
    
  PendingOperationItem(TempoList* tl, const iTEvent& ite, int tempo, PendingOperationType type = ModifyTempo)
    { _type = type; _tempo_list = tl; _iTEvent = ite; _intA = tempo; }
    
  // type is SetGlobalTempo, SetStaticTempo.
  PendingOperationItem(TempoList* tl, int tempo, PendingOperationType type)
    { _type = type; _tempo_list = tl; _intA = tempo; }

    
  // NOTE: 'tick' is the desired tick. se is a new SigEvent with sig and (same) desired tick. Swapping with NEXT event is done.
  PendingOperationItem(MusECore::SigList* sl, MusECore::SigEvent* se, int tick, PendingOperationType type = AddSig)
    { _type = type; _sig_list = sl; _sig_event = se; _intA = tick; }
    
  // NOTE: _sig_event is required. We must erase 'ise' in stage 2, then delete the SigEvent* in stage 3 (not stage 1),
  //        so 'ise' is unavailable to fetch the SigEvent* from it (in ise->second).
  PendingOperationItem(MusECore::SigList* sl, const MusECore::iSigEvent& ise, PendingOperationType type = DeleteSig)
    { _type = type; _sig_list = sl; _iSigEvent = ise; _sig_event = ise->second; }
    
  PendingOperationItem(MusECore::SigList* sl, const MusECore::iSigEvent& ise, const MusECore::TimeSignature& s, PendingOperationType type = ModifySig)
    { _type = type; _sig_list = sl; _iSigEvent = ise; _intA = s.z; _intB = s.n; }
    
    
  // NOTE: 'tick' is the desired tick. ke is a new SigEvent with sig and (same) desired tick. Swapping with NEXT event is done.
  PendingOperationItem(KeyList* kl, key_enum ke, int tick, PendingOperationType type = AddKey)
    { _type = type; _key_list = kl; _intA = tick; _intB = ke; }
    
  PendingOperationItem(KeyList* kl, const iKeyEvent& ike, PendingOperationType type = DeleteKey)
    { _type = type; _key_list = kl; _iKeyEvent = ike; }
    
  PendingOperationItem(KeyList* kl, const iKeyEvent& ike, key_enum ke, PendingOperationType type = ModifyKey)
    { _type = type; _key_list = kl; _iKeyEvent = ike; _intA = ke; }
    
  PendingOperationItem(int len, PendingOperationType type = ModifySongLength)
    { _type = type; _intA = len; }

  PendingOperationItem(PendingOperationType type) // type is EnableAllAudioControllers.
    { _type = type; }

  PendingOperationItem()
    { _type = Uninitialized; }
    
  // Execute the operation. Called only from RT stage 2.
    SongChangedStruct_t executeRTStage(); 
  // Execute the operation. Called only from post RT stage 3.
    SongChangedStruct_t executeNonRTStage(); 
  // Get an appropriate indexing value from ops like AddEvent that use it. Other ops like AddMidiCtrlValList return their type (rather than say, zero).
  int getIndex() const;
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
    std::multimap<int, iterator, std::less<int> > _map; 
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

    // Returns true if successful.
    bool addTimeSigOperation(unsigned tick, const MusECore::TimeSignature& s, MusECore::SigList* sl);
    bool delTimeSigOperation(unsigned tick, MusECore::SigList* sl);
    bool addTempoOperation(unsigned tick, int tempo, TempoList* tl);
    bool delTempoOperation(unsigned tick, TempoList* tl);
};

typedef PendingOperationList::iterator iPendingOperation;
typedef std::multimap<int, iPendingOperation, std::less<int> >::iterator iPendingOperationSorted;
typedef std::multimap<int, iPendingOperation, std::less<int> >::reverse_iterator riPendingOperationSorted;
typedef std::pair <iPendingOperationSorted, iPendingOperationSorted> iPendingOperationSortedRange;

} // namespace MusECore

#endif
