//=========================================================
//  MusE
//  Linux Music Editor
//    operations.cpp 
//  (C) Copyright 2014 Tim E. Real (terminator356 on users dot sourceforge dot net)
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

#include "operations.h"
#include "song.h"

// Enable for debugging:
//#define _PENDING_OPS_DEBUG_

namespace MusECore {

bool PendingOperationItem::isAllocationOp(const PendingOperationItem& op) const
{
  switch(op._type)
  {
    case PendingOperationItem::AddMidiCtrlValList:
      // A is channel B is control.
      if(_type == PendingOperationItem::AddMidiCtrlValList && _mcvll == op._mcvll && _intA == op._intA && _intB == op._intB)
        return true;
    break;
    
    case PendingOperationItem::AddTempo:
      // A is tick.
      if(_type == PendingOperationItem::AddTempo && _tempo_list == op._tempo_list && _intA == op._intA) 
        return true;
    break;
      
    case PendingOperationItem::AddSig:
      // A is tick.
      if(_type == PendingOperationItem::AddSig && _sig_list == op._sig_list && _intA == op._intA)
        return true;
    break;
    
    // In the case of type AddMidiDevice, this searches for the name only.
    case PendingOperationItem::AddMidiDevice:
      if(_type == PendingOperationItem::AddMidiDevice && _midi_device_list == op._midi_device_list && 
         _midi_device->name() == op._midi_device->name())
        return true;
    break;
    
    default:
    break;  
  }
  
  return false;
}

int PendingOperationItem::getIndex() const
{
  switch(_type)
  {
    case PendingOperationItem::Uninitialized:
    case PendingOperationItem::AddAuxSendValue:
    case PendingOperationItem::AddMidiInstrument:
    case PendingOperationItem::DeleteMidiInstrument:
    case PendingOperationItem::AddMidiDevice:
    case PendingOperationItem::DeleteMidiDevice:
    case PendingOperationItem::ModifyMidiDeviceAddress:
    case PendingOperationItem::ModifyMidiDeviceFlags:
    case PendingOperationItem::ModifyMidiDeviceName:
    case PendingOperationItem::AddTrack:
    case PendingOperationItem::DeleteTrack:
    case PendingOperationItem::MoveTrack:
    case PendingOperationItem::ModifyTrackName:
    case PendingOperationItem::ModifyPartName:
    case PendingOperationItem::ModifySongLength:
    case PendingOperationItem::AddMidiCtrlValList:
    case PendingOperationItem::SetGlobalTempo:
    case PendingOperationItem::AddRoute:
    case PendingOperationItem::DeleteRoute:
    case PendingOperationItem::AddRouteNode:
    case PendingOperationItem::DeleteRouteNode:
    case PendingOperationItem::ModifyRouteNode:
      // To help speed up searches of these ops, let's (arbitrarily) set index = type instead of all of them being at index 0!
      return _type;
    
    case PendingOperationItem::ModifyPartLength:
      return _part->posValue();
    
    case PendingOperationItem::MovePart:
      // _part is used here rather than _iPart since _iPart can be end().
      return _part->posValue();
    
    case PendingOperationItem::AddPart:
      return _part->posValue();  
    
    case PendingOperationItem::DeletePart:
      return _iPart->second->posValue();

    
    case PendingOperationItem::AddEvent:
      return _ev.posValue();
    
    case PendingOperationItem::DeleteEvent:
      return _ev.posValue();
    
      
    case PendingOperationItem::AddMidiCtrlVal:
      return _intA;  // Tick
    
    case PendingOperationItem::DeleteMidiCtrlVal:
      return _imcv->first;  // Tick
    
    case PendingOperationItem::ModifyMidiCtrlVal:
      return _imcv->first;  // Tick

    
    case PendingOperationItem::AddTempo:
      return _intA;  // Tick
    
    case PendingOperationItem::DeleteTempo:
      return _iTEvent->first;  // Tick
    
    case PendingOperationItem::ModifyTempo:
      // We want the 'real' tick, not _iTEvent->first which is the index of the next iterator! 
      return _iTEvent->second->tick;  // Tick
    
    
    case PendingOperationItem::AddSig:
      return _intA;  // Tick
    
    case PendingOperationItem::DeleteSig:
      return _iSigEvent->first;  // Tick
    
    case PendingOperationItem::ModifySig:
      // We want the 'real' tick, not _iSigEvent->first which is the index of the next iterator! 
      return _iSigEvent->second->tick;  // Tick

    
    case PendingOperationItem::AddKey:
      return _intA;  // Tick
    
    case PendingOperationItem::DeleteKey:
      return _iKeyEvent->first;  // Tick
    
    case PendingOperationItem::ModifyKey:
      // We want the 'real' tick, not _iKeyEvent->first which is the index of the next iterator! 
      return _iKeyEvent->second.tick;  // Tick
    
    
    default:
      fprintf(stderr, "PendingOperationItem::getIndex unknown op type: %d\n", _type);
      return 0;
    break;  
  }
}  

void PendingOperationItem::executeRTStage()
{
  switch(_type)
  {
    // TODO: Try to break this operation down so that only the actual operation is executed stage-2. 
    case AddRoute:
#ifdef _PENDING_OPS_DEBUG_
      fprintf(stderr, "PendingOperationItem::executeRTStage AddRoute: src/dst routes:\n");
      _src_route.dump();
      _dst_route.dump();
#endif      
      addRoute(_src_route, _dst_route);
    break;
    
    // TODO: Try to break this operation down so that only the actual operation is executed stage-2. 
    case DeleteRoute:
#ifdef _PENDING_OPS_DEBUG_
      fprintf(stderr, "PendingOperationItem::executeRTStage DeleteRoute: src/dst routes:\n");
      _src_route.dump();
      _dst_route.dump();
#endif      
      removeRoute(_src_route, _dst_route);
    break;
    
    case AddRouteNode:
#ifdef _PENDING_OPS_DEBUG_
      fprintf(stderr, "PendingOperationItem::executeRTStage AddRouteNode: route_list:%p route:\n", _route_list);
      _src_route.dump();
#endif      
      _route_list->push_back(_src_route);
    break;
    
    case DeleteRouteNode:
#ifdef _PENDING_OPS_DEBUG_
      fprintf(stderr, "PendingOperationItem::executeRTStage DeleteRouteNode: route_list:%p route:\n", _route_list);
      _iRoute->dump();
#endif      
      _route_list->erase(_iRoute);
    break;
    
    case ModifyRouteNode:
#ifdef _PENDING_OPS_DEBUG_
      fprintf(stderr, "PendingOperationItem::executeRTStage ModifyRouteNode: src/dst routes:\n");
      _src_route.dump();
      _dst_route_pointer->dump();
#endif      
      *_dst_route_pointer = _src_route;
    break;

    case AddAuxSendValue:
#ifdef _PENDING_OPS_DEBUG_
      fprintf(stderr, "PendingOperationItem::executeRTStage AddAuxSendValue aux_send_value_list:%p val:%f\n", _aux_send_value_list, _aux_send_value);
#endif      
      _aux_send_value_list->push_back(_aux_send_value);
    break;

    
    case AddMidiInstrument:
#ifdef _PENDING_OPS_DEBUG_
      fprintf(stderr, "PendingOperationItem::executeRTStage AddMidiInstrument instrument_list:%p instrument:%p\n", _midi_instrument_list, _midi_instrument);
#endif      
      _midi_instrument_list->push_back(_midi_instrument);
    break;
    
    case DeleteMidiInstrument:
#ifdef _PENDING_OPS_DEBUG_
      fprintf(stderr, "PendingOperationItem::executeRTStage DeleteMidiInstrument instrument_list:%p instrument:%p\n", _midi_instrument_list, *_iMidiInstrument);
#endif      
      _midi_instrument_list->erase(_iMidiInstrument);
    break;
    
    case AddMidiDevice:
#ifdef _PENDING_OPS_DEBUG_
      fprintf(stderr, "PendingOperationItem::executeRTStage AddMidiDevice devicelist:%p device:%p\n", _midi_device_list, _midi_device);
#endif      
      _midi_device_list->push_back(_midi_device);
    break;
    
    case DeleteMidiDevice:
#ifdef _PENDING_OPS_DEBUG_
      fprintf(stderr, "PendingOperationItem::executeRTStage DeleteMidiDevice devicelist:%p device:%p\n", _midi_device_list, *_iMidiDevice);
#endif      
      _midi_device_list->erase(_iMidiDevice);
    break;
    
    case ModifyMidiDeviceAddress:
#ifdef _PENDING_OPS_DEBUG_
      fprintf(stderr, "PendingOperationItem::executeRTStage ModifyMidiDeviceAddress device:%p client:%d port:%d\n", _midiDevice, _address_client, _address_port);
#endif      
      _midi_device->setAddressClient(_address_client);
      _midi_device->setAddressPort(_address_port);
      _midi_device->setOpenFlags(_open_flags);
    break;
    
    case ModifyMidiDeviceFlags:
#ifdef _PENDING_OPS_DEBUG_
      fprintf(stderr, "PendingOperationItem::executeRTStage ModifyMidiDeviceFlags device:%p rwFlags:%d openFlags:%d\n", _midiDevice, _rw_flags, _open_flags);
#endif      
      _midi_device->setrwFlags(_rw_flags);
      _midi_device->setOpenFlags(_open_flags);
    break;
    
    case ModifyMidiDeviceName:
#ifdef _PENDING_OPS_DEBUG_
      fprintf(stderr, "PendingOperationItem::executeRTStage ModifyMidiDeviceName device:%p name:%s\n", _midiDevice, _name->toLocal8Bit().data());
#endif      
      _midi_device->setName(*_name);
    break;
    
    case AddTrack:
    {
#ifdef _PENDING_OPS_DEBUG_
      fprintf(stderr, "PendingOperationItem::executeRTStage AddTrack track_list:%p track:%p\n", _track_list, _track);
#endif      
      if(_void_track_list)
      {
        switch(_track->type())
        {
              case Track::MIDI:
              case Track::DRUM:
              case Track::NEW_DRUM:
                    static_cast<MidiTrackList*>(_void_track_list)->push_back(static_cast<MidiTrack*>(_track));
                    break;
              case Track::WAVE:
                    static_cast<WaveTrackList*>(_void_track_list)->push_back(static_cast<WaveTrack*>(_track));
                    break;
              case Track::AUDIO_OUTPUT:
                    static_cast<OutputList*>(_void_track_list)->push_back(static_cast<AudioOutput*>(_track));
                    break;
              case Track::AUDIO_GROUP:
                    static_cast<GroupList*>(_void_track_list)->push_back(static_cast<AudioGroup*>(_track));
                    break;
              case Track::AUDIO_AUX:
                    static_cast<AuxList*>(_void_track_list)->push_back(static_cast<AudioAux*>(_track));
                    break;
              case Track::AUDIO_INPUT:
                    static_cast<InputList*>(_void_track_list)->push_back(static_cast<AudioInput*>(_track));
                    break;
              case Track::AUDIO_SOFTSYNTH:
                    static_cast<SynthIList*>(_void_track_list)->push_back(static_cast<SynthI*>(_track));
                    break;
              default:
                    fprintf(stderr, "PendingOperationItem::executeRTStage AddTrack: Unknown track type %d\n", _track->type());
                    return;
        }
      }
      
      iTrack track_it = _track_list->index2iterator(_insert_at);
      _track_list->insert(track_it, _track);

      // Add routes:
      if(_track->type() == Track::AUDIO_OUTPUT) 
      {
            const RouteList* rl = _track->inRoutes();
            for(ciRoute r = rl->begin(); r != rl->end(); ++r)
            {
                  Route src(_track, r->channel, r->channels);
                  src.remoteChannel = r->remoteChannel;
                  r->track->outRoutes()->push_back(src);
                  // Is the source an Aux Track or else does it have Aux Tracks routed to it?
                  // Update the Audio Output track's aux ref count.
                  if(r->track->auxRefCount())
                    _track->updateAuxRoute(r->track->auxRefCount(), NULL);
                  else if(r->track->type() == Track::AUDIO_AUX)
                    _track->updateAuxRoute(1, NULL);
            }      
      }
      else if(_track->type() == Track::AUDIO_INPUT) 
      {
            const RouteList* rl = _track->outRoutes();
            for(ciRoute r = rl->begin(); r != rl->end(); ++r)
            {
                  Route src(_track, r->channel, r->channels);
                  src.remoteChannel = r->remoteChannel;
                  r->track->inRoutes()->push_back(src);
                  // Does this track have Aux Tracks routed to it?
                  // Update the other track's aux ref count and all tracks it is connected to.
                  if(_track->auxRefCount())
                    r->track->updateAuxRoute(_track->auxRefCount(), NULL);
            }      
      }
      else if(_track->isMidiTrack())
      {
            const RouteList* rl = _track->inRoutes();
            for(ciRoute r = rl->begin(); r != rl->end(); ++r)
            {
                  Route src(_track, r->channel);
                  MusEGlobal::midiPorts[r->midiPort].outRoutes()->push_back(src);
            }
            rl = _track->outRoutes();
            for(ciRoute r = rl->begin(); r != rl->end(); ++r)
            {
                  Route src(_track, r->channel);
                  MusEGlobal::midiPorts[r->midiPort].inRoutes()->push_back(src);
            }      
      }
      else 
      {
            const RouteList* rl = _track->inRoutes();
            for(ciRoute r = rl->begin(); r != rl->end(); ++r)
            {
                  Route src(_track, r->channel, r->channels);
                  src.remoteChannel = r->remoteChannel;
                  r->track->outRoutes()->push_back(src);
                  // Is the source an Aux Track or else does it have Aux Tracks routed to it?
                  // Update this track's aux ref count.
                  if(r->track->auxRefCount())
                    _track->updateAuxRoute(r->track->auxRefCount(), NULL);
                  else if(r->track->type() == Track::AUDIO_AUX)
                    _track->updateAuxRoute(1, NULL);
            }
            rl = _track->outRoutes();
            for(ciRoute r = rl->begin(); r != rl->end(); ++r)
            {
                  Route src(_track, r->channel, r->channels);
                  src.remoteChannel = r->remoteChannel;
                  r->track->inRoutes()->push_back(src);
                  // Is this track an Aux Track or else does it have Aux Tracks routed to it?
                  // Update the other track's aux ref count and all tracks it is connected to.
                  if(_track->auxRefCount())
                    r->track->updateAuxRoute(_track->auxRefCount(), NULL);
                  else if(_track->type() == Track::AUDIO_AUX)
                    r->track->updateAuxRoute(1, NULL);
            }      
      }
      chainTrackParts(_track);

      // Be sure to mark the parts as not deleted if they exist in the global copy/paste clone list.
      const PartList* pl = _track->cparts();
      for(ciPart ip = pl->begin(); ip != pl->end(); ++ip) 
      {
        for(iClone i = MusEGlobal::cloneList.begin(); i != MusEGlobal::cloneList.end(); ++i) 
        {
          if(i->cp == ip->second) 
            i->is_deleted = false;
        }
      }
    }
    break;
    
    case DeleteTrack:
    {
#ifdef _PENDING_OPS_DEBUG_
      fprintf(stderr, "PendingOperationItem::executeRTStage DeleteTrack track_list:%p track:%p sec_track_list:%p\n", _track_list, _track, _void_track_list);
#endif      
      unchainTrackParts(_track);
      if(_void_track_list)
      {
        switch(_track->type())
        {
              case Track::MIDI:
              case Track::DRUM:
              case Track::NEW_DRUM:
                    static_cast<MidiTrackList*>(_void_track_list)->erase(_track);
                    break;
              case Track::WAVE:
                    static_cast<WaveTrackList*>(_void_track_list)->erase(_track);
                    break;
              case Track::AUDIO_OUTPUT:
                    static_cast<OutputList*>(_void_track_list)->erase(_track);
                    break;
              case Track::AUDIO_GROUP:
                    static_cast<GroupList*>(_void_track_list)->erase(_track);
                    break;
              case Track::AUDIO_AUX:
                    static_cast<AuxList*>(_void_track_list)->erase(_track);
                    break;
              case Track::AUDIO_INPUT:
                    static_cast<InputList*>(_void_track_list)->erase(_track);
                    break;
              case Track::AUDIO_SOFTSYNTH:
                    static_cast<SynthIList*>(_void_track_list)->erase(_track);
                    break;
              default:
                    fprintf(stderr, "PendingOperationItem::executeRTStage DeleteTrack: Unknown track type %d\n", _track->type());
                    return;
        }
      }
      _track_list->erase(_track);

      // Remove routes:
      if(_track->type() == Track::AUDIO_OUTPUT) 
      {
            // Clear the track's jack ports
            for(int ch = 0; ch < _track->channels(); ++ch)
              ((AudioOutput*)_track)->setJackPort(ch, 0);
            
            // Clear the track's output routes' jack ports
            RouteList* orl = _track->outRoutes();
            for(iRoute r = orl->begin(); r != orl->end(); ++r)
            {
              if(r->type != Route::JACK_ROUTE)
                continue;
              r->jackPort = 0;
            }
            
            // Remove other tracks' output routes to this track
            const RouteList* irl = _track->inRoutes();
            for(ciRoute r = irl->begin(); r != irl->end(); ++r)
            {
                  Route src(_track, r->channel, r->channels);
                  src.remoteChannel = r->remoteChannel;
                  r->track->outRoutes()->removeRoute(src);
                  // Is the source an Aux Track or else does it have Aux Tracks routed to it?
                  // Update the Audio Output track's aux ref count.
                  if(r->track->auxRefCount())
                    _track->updateAuxRoute(-r->track->auxRefCount(), NULL);
                  else if(r->track->type() == Track::AUDIO_AUX)
                    _track->updateAuxRoute(-1, NULL);
            }      
      }
      else if(_track->type() == Track::AUDIO_INPUT) 
      {
            // Clear the track's jack ports
            for(int ch = 0; ch < _track->channels(); ++ch)
              ((AudioInput*)_track)->setJackPort(ch, 0);
            
            // Clear the track's input routes' jack ports
            RouteList* irl = _track->inRoutes();
            for(iRoute r = irl->begin(); r != irl->end(); ++r)
            {
              if(r->type != Route::JACK_ROUTE)
                continue;
              r->jackPort = 0;
            }
            
            // Remove other tracks' input routes from this track
            const RouteList* orl = _track->outRoutes();
            for(ciRoute r = orl->begin(); r != orl->end(); ++r)
            {
                  Route src(_track, r->channel, r->channels);
                  src.remoteChannel = r->remoteChannel;
                  r->track->inRoutes()->removeRoute(src);
                  // Is this track an Aux Track or else does it have Aux Tracks routed to it?
                  // Update the other track's aux ref count and all tracks it is connected to.
                  if(_track->auxRefCount())
                    r->track->updateAuxRoute(-_track->auxRefCount(), NULL);
                  else if(_track->type() == Track::AUDIO_AUX)
                    r->track->updateAuxRoute(-1, NULL);
            }      
      }
      else if(_track->isMidiTrack())
      {
            const RouteList* rl = _track->inRoutes();
            for(ciRoute r = rl->begin(); r != rl->end(); ++r)
            {
                  Route src(_track, r->channel);
                  MusEGlobal::midiPorts[r->midiPort].outRoutes()->removeRoute(src);
            }
            rl = _track->outRoutes();
            for(ciRoute r = rl->begin(); r != rl->end(); ++r)
            {
                  Route src(_track, r->channel);
                  MusEGlobal::midiPorts[r->midiPort].inRoutes()->removeRoute(src);
            }      
      }
      else 
      {
            const RouteList* rl = _track->inRoutes();
            for(ciRoute r = rl->begin(); r != rl->end(); ++r)
            {
                  Route src(_track, r->channel, r->channels);
                  src.remoteChannel = r->remoteChannel;
                  r->track->outRoutes()->removeRoute(src);
                  // Is the source an Aux Track or else does it have Aux Tracks routed to it?
                  // Update this track's aux ref count.
                  if(r->track->auxRefCount())
                    _track->updateAuxRoute(-r->track->auxRefCount(), NULL);
                  else if(r->track->type() == Track::AUDIO_AUX)
                    _track->updateAuxRoute(-1, NULL);
            }
            rl = _track->outRoutes();
            for(ciRoute r = rl->begin(); r != rl->end(); ++r)
            {
                  Route src(_track, r->channel, r->channels);
                  src.remoteChannel = r->remoteChannel;
                  r->track->inRoutes()->removeRoute(src);
                  // Is this track an Aux Track or else does it have Aux Tracks routed to it?
                  // Update the other track's aux ref count and all tracks it is connected to.
                  if(_track->auxRefCount())
                    r->track->updateAuxRoute(-_track->auxRefCount(), NULL);
                  else if(_track->type() == Track::AUDIO_AUX)
                    r->track->updateAuxRoute(-1, NULL);
            }      
      }

      // Be sure to mark the parts as deleted if they exist in the global copy/paste clone list.
      const PartList* pl = _track->cparts();
      for(ciPart ip = pl->begin(); ip != pl->end(); ++ip) 
      {
        for(iClone i = MusEGlobal::cloneList.begin(); i != MusEGlobal::cloneList.end(); ++i) 
        {
          if(i->cp == ip->second) 
            i->is_deleted = true;
        }
      }
    }  
    break;
    
    case MoveTrack:
    {
#ifdef _PENDING_OPS_DEBUG_
      fprintf(stderr, "PendingOperationItem::executeRTStage MoveTrack from:%d to:%d\n", _from_idx, _to_idx);
#endif      
      int sz = _track_list->size();
      if(_from_idx >= sz)
      {
        fprintf(stderr, "MusE error: PendingOperationItem::executeRTStage MoveTrack from index out of range:%d\n", _from_idx);
        return;
      }
      iTrack fromIt = _track_list->begin() + _from_idx;
      Track* track = *fromIt;
      _track_list->erase(fromIt);
      iTrack toIt = (_to_idx >= sz) ? _track_list->end() : _track_list->begin() + _to_idx;
      _track_list->insert(toIt, track);
    }
    break;
    
    case ModifyTrackName:
#ifdef _PENDING_OPS_DEBUG_
      fprintf(stderr, "PendingOperationItem::executeRTStage ModifyTrackName track:%p new_val:%s\n", _track, _name->toLocal8Bit().data());
#endif      
      _track->setName(*_name);
    break;
    

    case AddPart:
#ifdef _PENDING_OPS_DEBUG_
      fprintf(stderr, "PendingOperationItem::executeRTStage AddPart part:%p\n", _part);
#endif      
      _part_list->add(_part);
      _part->rechainClone();
      // Be sure to mark the part as not deleted if it exists in the global copy/paste clone list.
      for(iClone i = MusEGlobal::cloneList.begin(); i != MusEGlobal::cloneList.end(); ++i) 
      {
        if(i->cp == _part) 
          i->is_deleted = false;
      }
    break;
    
    case DeletePart:
    {
#ifdef _PENDING_OPS_DEBUG_
      fprintf(stderr, "PendingOperationItem::executeRTStage DeletePart part:%p\n", _iPart->second);
#endif      
      Part* p = _iPart->second;
      _part_list->erase(_iPart);
      p->unchainClone();
      // Be sure to mark the part as deleted if it exists in the global copy/paste clone list.
      for(iClone i = MusEGlobal::cloneList.begin(); i != MusEGlobal::cloneList.end(); ++i) 
      {
        if(i->cp == p) 
          i->is_deleted = true;
      }
    }
    break;

    case ModifyPartLength:
#ifdef _PENDING_OPS_DEBUG_
      fprintf(stderr, "PendingOperationItem::executeRTStage ModifyPartLength part:%p old_val:%d new_val:%d\n", _part, _part->lenValue(), _intA);
#endif      
      //_part->type() == Pos::FRAMES ? _part->setLenFrame(_intA) : _part->setLenTick(_intA);
      _part->setLenValue(_intA);
    break;
    
    case MovePart:
#ifdef _PENDING_OPS_DEBUG_
      fprintf(stderr, "PendingOperationItem::executeRTStage MovePart part:%p track:%p new_pos:%d\n", _part, _track, _intA);
#endif      
      if(_track)
      {
        if(_part->track() && _iPart != _part->track()->parts()->end())
          _part->track()->parts()->erase(_iPart);
        _part->setTrack(_track);
        _track->parts()->add(_part);
      }
      //_part->setTick(_intA);
      _part->setPosValue(_intA);
    break;

    case ModifyPartName:
#ifdef _PENDING_OPS_DEBUG_
      fprintf(stderr, "PendingOperationItem::executeRTStage ModifyPartName part:%p new_val:%s\n", _part, _name->toLocal8Bit().data());
#endif      
      _part->setName(*_name);
    break;
    
    
    case AddEvent:
#ifdef _PENDING_OPS_DEBUG_
      fprintf(stderr, "PendingOperationItem::executeRTStage AddEvent pre:    ");
      _ev.dump();
#endif      
      _part->addEvent(_ev);
#ifdef _PENDING_OPS_DEBUG_
      fprintf(stderr, "PendingOperationItem::executeRTStage AddEvent post:   ");
      _ev.dump();
#endif      
    break;
    case DeleteEvent:
#ifdef _PENDING_OPS_DEBUG_
      fprintf(stderr, "PendingOperationItem::executeRTStage DeleteEvent pre:    ");
      _ev.dump();
#endif      
      _part->nonconst_events().erase(_iev);
#ifdef _PENDING_OPS_DEBUG_
      fprintf(stderr, "PendingOperationItem::executeRTStage DeleteEvent post:   ");
      _ev.dump();
#endif      
    break;
    
    
    case AddMidiCtrlValList:
#ifdef _PENDING_OPS_DEBUG_
      fprintf(stderr, "PendingOperationItem::executeRTStage AddMidiCtrlValList: mcvll:%p mcvl:%p chan:%d\n", _mcvll, _mcvl, _intA);
#endif      
      _mcvll->add(_intA, _mcvl);
    break;
    case AddMidiCtrlVal:
#ifdef _PENDING_OPS_DEBUG_
      fprintf(stderr, "PendingOperationItem::executeRTStage AddMidiCtrlVal: mcvl:%p part:%p tick:%d val:%d\n", _mcvl, _part, _intA, _intB);
#endif      
      _mcvl->insert(std::pair<const int, MidiCtrlVal> (_intA, MidiCtrlVal(_part, _intB))); // FIXME FINDMICHJETZT XTicks!!
    break;
    case DeleteMidiCtrlVal:
#ifdef _PENDING_OPS_DEBUG_
      fprintf(stderr, "PendingOperationItem::executeRTStage DeleteMidiCtrlVal: mcvl:%p tick:%d part:%p val:%d\n", 
                       _mcvl, _imcv->first, _imcv->second.part, _imcv->second.val);
#endif      
      _mcvl->erase(_imcv);
    break;
    case ModifyMidiCtrlVal:
#ifdef _PENDING_OPS_DEBUG_
      fprintf(stderr, "PendingOperationItem::executeRTStage ModifyMidiCtrlVal: part:%p old_val:%d new_val:%d\n", 
                       _imcv->second.part, _imcv->second.val, _intA);
#endif      
      _imcv->second.val = _intA;
    break;
    
    
    case AddTempo:
#ifdef _PENDING_OPS_DEBUG_
      fprintf(stderr, "PendingOperationItem::executeRTStage AddTempo: tempolist:%p tempo:%p %d tick:%d\n", 
                       _tempo_list, _tempo_event, _tempo_event->tempo, _tempo_event->tick);
#endif      
      _tempo_list->add(_intA, _tempo_event, false);  // Defer normalize until end of stage 2.
    break;
    
    case DeleteTempo:
      {
#ifdef _PENDING_OPS_DEBUG_
        fprintf(stderr, "PendingOperationItem::executeRTStage DeleteTempo: tempolist:%p event:%p: tick:%d tempo:%d\n", 
                         _tempo_list, _iTEvent->second, _iTEvent->second->tick,  _iTEvent->second->tempo);
#endif      
        _tempo_list->del(_iTEvent, false); // Defer normalize until end of stage 2.
      }
    break;
    
    case ModifyTempo:
#ifdef _PENDING_OPS_DEBUG_
      fprintf(stderr, "PendingOperationItem::executeRTStage ModifyTempo: tempolist:%p event:%p: tick:%d old_tempo:%d new_tempo:%d\n", 
                       _tempo_list, _iTEvent->second, _iTEvent->second->tick,  _iTEvent->second->tempo, _intA);
#endif      
      _iTEvent->second->tempo = _intA;
    break;
    
    case SetGlobalTempo:
#ifdef _PENDING_OPS_DEBUG_
      fprintf(stderr, "PendingOperationItem::executeRTStage SetGlobalTempo: tempolist:%p new_tempo:%d\n", _tempo_list, _intA);
#endif      
      _tempo_list->setGlobalTempo(_intA);
    break;

    
    case AddSig:
#ifdef _PENDING_OPS_DEBUG_
      fprintf(stderr, "PendingOperationItem::executeRTStage AddSig: siglist:%p sig:%p %d/%d tick:%d\n", 
                       _sig_list, _sig_event, _sig_event->sig.z, _sig_event->sig.n, _sig_event->tick);
#endif      
      _sig_list->add(_intA, _sig_event, false);  // Defer normalize until end of stage 2.
    break;
    
    case DeleteSig:
      {
#ifdef _PENDING_OPS_DEBUG_
        fprintf(stderr, "PendingOperationItem::executeRTStage DeleteSig: siglist:%p event:%p: tick:%d sig:%d/%d\n", 
                         _sig_list, _iSigEvent->second, _iSigEvent->second->tick,  _iSigEvent->second->sig.z, _iSigEvent->second->sig.n);
#endif      
        _sig_list->del(_iSigEvent, false); // Defer normalize until end of stage 2.
      }
    break;
    
    case ModifySig:
#ifdef _PENDING_OPS_DEBUG_
      fprintf(stderr, "PendingOperationItem::executeRTStage ModifySig: siglist:%p event:%p: tick:%d old_sig:%d/%d new_sig:%d/%d\n", 
                       _sig_list, _iSigEvent->second, _iSigEvent->second->tick,  _iSigEvent->second->sig.z, _iSigEvent->second->sig.n, _intA, _intB);
#endif      
      _iSigEvent->second->sig.z = _intA;
      _iSigEvent->second->sig.n = _intB;
    break;
    
    
    case AddKey:
#ifdef _PENDING_OPS_DEBUG_
      fprintf(stderr, "PendingOperationItem::executeRTStage AddKey: keylist:%p key:%d tick:%d\n", _key_list, _intB, _intA);
#endif      
      _key_list->add(KeyEvent(key_enum(_intB), _intA)); 
    break;
    
    case DeleteKey:
      {
#ifdef _PENDING_OPS_DEBUG_
        fprintf(stderr, "PendingOperationItem::executeRTStage DeleteKey: keylist:%p key:%d tick:%d\n",
                         _key_list, _iKeyEvent->second.key, _iKeyEvent->second.tick);
#endif      
        _key_list->del(_iKeyEvent);
      }
    break;
    
    case ModifyKey:
#ifdef _PENDING_OPS_DEBUG_
      fprintf(stderr, "PendingOperationItem::executeRTStage ModifyKey: keylist:%p old_key:%d new_key:%d tick:%d\n", 
                       _key_list, _iKeyEvent->second.key, _intA, _iKeyEvent->second.tick);
#endif      
      _iKeyEvent->second.key = key_enum(_intA);
    break;

    
    case ModifySongLength:
#ifdef _PENDING_OPS_DEBUG_
      fprintf(stderr, "PendingOperationItem::executeRTStage ModifySongLength: len:%d\n", _intA);
#endif      
      MusEGlobal::song->setLen(_intA, false); // false = Do not emit update signals here !
    break;
    
    case Uninitialized:
    break;
    
    default:
      fprintf(stderr, "PendingOperationItem::executeRTStage unknown type %d\n", _type);
    break;
  }
}

void PendingOperationItem::executeNonRTStage()
{
  switch(_type)
  {
    case DeleteTempo:
      {
#ifdef _PENDING_OPS_DEBUG_
        fprintf(stderr, "PendingOperationItem::executeNonRTStage DeleteTempo: tempolist:%p event:%p:\n", 
                         _tempo_list, _tempo_event);
#endif      
        if(_tempo_event)
        {  
          delete _tempo_event;
          _tempo_event = 0;
        }
      }
    break;
    
    case DeleteSig:
      {
#ifdef _PENDING_OPS_DEBUG_
        fprintf(stderr, "PendingOperationItem::executeNonRTStage DeleteSig: siglist:%p event:%p:\n", 
                         _sig_list, _sig_event);
#endif      
        if(_sig_event)
        {  
          delete _sig_event;
          _sig_event = 0;
        }
      }
    break;
    
    default:
    break;
  }
}

void PendingOperationList::executeRTStage()
{
#ifdef _PENDING_OPS_DEBUG_
  fprintf(stderr, "PendingOperationList::executeRTStage executing...\n");
#endif      
  for(iPendingOperation ip = begin(); ip != end(); ++ip)
    ip->executeRTStage();
}

void PendingOperationList::executeNonRTStage()
{
#ifdef _PENDING_OPS_DEBUG_
  fprintf(stderr, "PendingOperationList::executeNonRTStage executing...\n");
#endif      
  for(iPendingOperation ip = begin(); ip != end(); ++ip)
    ip->executeNonRTStage();
}

void PendingOperationList::clear()
{
  _map.clear();
  std::list<PendingOperationItem>::clear();  
#ifdef _PENDING_OPS_DEBUG_
  fprintf(stderr, "PendingOperationList::clear * post map size:%d list size:%d\n", _map.size(), size());
#endif      
}

bool PendingOperationList::add(PendingOperationItem op)
{
  int t = op.getIndex();

  switch(op._type)
  {
    // For these special allocation ops, searching has already been done before hand. Just add them.
    case PendingOperationItem::AddMidiCtrlVal:
    case PendingOperationItem::AddTempo:
    case PendingOperationItem::AddSig:
    {
      iPendingOperation iipo = insert(end(), op);
      _map.insert(std::pair<int, iPendingOperation>(t, iipo));
      return true;
    }
    break;
    
    default:
    break;
  }
  
  iPendingOperationSortedRange r = _map.equal_range(t);
  iPendingOperationSorted ipos = r.second;
  while(ipos != r.first)
  {
    --ipos;
    PendingOperationItem& poi = *ipos->second;
    
    switch(op._type)
    {
      case PendingOperationItem::AddRoute:
        if(poi._type == PendingOperationItem::AddRoute && poi._src_route == op._src_route && poi._dst_route == op._dst_route)
        {
          fprintf(stderr, "MusE error: PendingOperationList::add(): Double AddRoute. Ignoring.\n");
          return false;  
        }
      break;
      
      case PendingOperationItem::DeleteRoute:
        if(poi._type == PendingOperationItem::DeleteRoute && poi._src_route == op._src_route && poi._dst_route == op._dst_route)
        {
          fprintf(stderr, "MusE error: PendingOperationList::add(): Double DeleteRoute. Ignoring.\n");
          return false;  
        }
      break;
      
      case PendingOperationItem::AddRouteNode:
        if(poi._type == PendingOperationItem::AddRouteNode && poi._route_list == op._route_list && poi._src_route == op._src_route)
        {
          fprintf(stderr, "MusE error: PendingOperationList::add(): Double AddRouteNode. Ignoring.\n");
          return false;  
        }
      break;

      case PendingOperationItem::DeleteRouteNode:
        if(poi._type == PendingOperationItem::DeleteRouteNode && poi._route_list == op._route_list && poi._iRoute == op._iRoute)
        {
          fprintf(stderr, "MusE error: PendingOperationList::add(): Double DeleteRouteNode. Ignoring.\n");
          return false;  
        }
      break;

      case PendingOperationItem::ModifyRouteNode:
        if(poi._type == PendingOperationItem::ModifyRouteNode && poi._src_route == op._src_route && poi._dst_route_pointer == op._dst_route_pointer)
        {
          fprintf(stderr, "MusE error: PendingOperationList::add(): Double ModifyRouteNode. Ignoring.\n");
          return false;  
        }
      break;


      case PendingOperationItem::AddAuxSendValue:
        if(poi._type == PendingOperationItem::AddAuxSendValue && poi._aux_send_value_list == op._aux_send_value_list)  
        {
          // Do nothing. So far.
        }
      break;
        
      case PendingOperationItem::AddMidiInstrument:
        if(poi._type == PendingOperationItem::AddMidiInstrument && poi._midi_instrument_list == op._midi_instrument_list && 
           poi._midi_instrument == op._midi_instrument)  
        {
          fprintf(stderr, "MusE error: PendingOperationList::add(): Double AddMidiInstrument. Ignoring.\n");
          return false;  
        }
      break;
        
      case PendingOperationItem::DeleteMidiInstrument:
        if(poi._type == PendingOperationItem::DeleteMidiInstrument && poi._midi_instrument_list == op._midi_instrument_list && 
           poi._iMidiInstrument == op._iMidiInstrument)
        {
          fprintf(stderr, "MusE error: PendingOperationList::add(): Double DeleteMidiInstrument. Ignoring.\n");
          return false;  
        }
      break;
        
      case PendingOperationItem::AddMidiDevice:
        if(poi._type == PendingOperationItem::AddMidiDevice && poi._midi_device_list == op._midi_device_list && poi._midi_device == op._midi_device)  
        {
          fprintf(stderr, "MusE error: PendingOperationList::add(): Double AddMidiDevice. Ignoring.\n");
          return false;  
        }
      break;
        
      case PendingOperationItem::DeleteMidiDevice:
        if(poi._type == PendingOperationItem::DeleteMidiDevice && poi._midi_device_list == op._midi_device_list && poi._iMidiDevice == op._iMidiDevice)
        {
          fprintf(stderr, "MusE error: PendingOperationList::add(): Double DeleteMidiDevice. Ignoring.\n");
          return false;  
        }
      break;
        
      case PendingOperationItem::ModifyMidiDeviceAddress:
        if(poi._type == PendingOperationItem::ModifyMidiDeviceAddress && poi._midi_device == op._midi_device &&
           poi._address_client == op._address_client && poi._address_port == op._address_port)
        {
          fprintf(stderr, "MusE error: PendingOperationList::add(): Double ModifyMidiDeviceAddress. Ignoring.\n");
          return false;  
        }
      break;
        
      case PendingOperationItem::ModifyMidiDeviceFlags:
        if(poi._type == PendingOperationItem::ModifyMidiDeviceFlags && poi._midi_device == op._midi_device &&
           poi._rw_flags == op._rw_flags && poi._open_flags == op._open_flags)
        {
          fprintf(stderr, "MusE error: PendingOperationList::add(): Double ModifyMidiDeviceFlags. Ignoring.\n");
          return false;  
        }
      break;
        
      case PendingOperationItem::ModifyMidiDeviceName:
        if(poi._type == PendingOperationItem::ModifyMidiDeviceName && poi._midi_device == op._midi_device &&
           poi._name == op._name)
        {
          fprintf(stderr, "MusE error: PendingOperationList::add(): Double ModifyMidiDeviceName. Ignoring.\n");
          return false;  
        }
      break;

      
      case PendingOperationItem::AddTrack:
        if(poi._type == PendingOperationItem::AddTrack && poi._track_list == op._track_list && poi._track == op._track)  
        {
          // Simply replace the insert point.
          poi._insert_at = op._insert_at;
          return true;  
        }
        else if(poi._type == PendingOperationItem::DeleteTrack && poi._track_list == op._track_list && poi._track == op._track)  
        {
          // Delete followed by add is useless. Cancel out the delete + add by erasing the delete command.
          //erase(ipos->second);
          //_map.erase(ipos);
          //return true;  
        }
      break;
      
      case PendingOperationItem::DeleteTrack:
        if(poi._type == PendingOperationItem::DeleteTrack && poi._track_list == op._track_list && poi._track == op._track)  
        {
          fprintf(stderr, "MusE error: PendingOperationList::add(): Double DeleteTrack. Ignoring.\n");
          return false;  
        }
        else if(poi._type == PendingOperationItem::AddTrack && poi._track_list == op._track_list && poi._track == op._track)  
        {
          // Add followed by delete is useless. Cancel out the add + delete by erasing the add command.
          //erase(ipos->second);
          //_map.erase(ipos);
          //return true;  
        }
      break;
      
      case PendingOperationItem::MoveTrack:
        if(poi._type == PendingOperationItem::MoveTrack && poi._track == op._track && poi._track_list == op._track_list)  
        {
          // Simply replace the 'to' index.
          poi._to_idx = op._to_idx;
          return true;
        }
      break;
        
      case PendingOperationItem::ModifyTrackName:
        if(poi._type == PendingOperationItem::ModifyTrackName && poi._track == op._track &&
           poi._name == op._name)
        {
          fprintf(stderr, "MusE error: PendingOperationList::add(): Double ModifyTrackName. Ignoring.\n");
          return false;  
        }
      break;

      
      case PendingOperationItem::AddPart:
        if(poi._type == PendingOperationItem::AddPart && poi._part_list == op._part_list && poi._part == op._part)  
        {
          fprintf(stderr, "MusE error: PendingOperationList::add(): Double AddPart. Ignoring.\n");
          return false;  
        }
        else if(poi._type == PendingOperationItem::DeletePart && poi._part_list == op._part_list && poi._iPart->second == op._part)  
        {
          // Delete followed by add is useless. Cancel out the delete + add by erasing the delete command.
          erase(ipos->second);
          _map.erase(ipos);
          return true;  
        }
      break;
      
      case PendingOperationItem::DeletePart:
        if(poi._type == PendingOperationItem::DeletePart && poi._part_list == op._part_list && poi._iPart->second == op._iPart->second)  
        {
          fprintf(stderr, "MusE error: PendingOperationList::add(): Double DeletePart. Ignoring.\n");
          return false;  
        }
        else if(poi._type == PendingOperationItem::AddPart && poi._part_list == op._part_list && poi._part == op._iPart->second)  
        {
          // Add followed by delete is useless. Cancel out the add + delete by erasing the add command.
          erase(ipos->second);
          _map.erase(ipos);
          return true;  
        }
      break;

      case PendingOperationItem::MovePart:
        if(poi._type == PendingOperationItem::MovePart && poi._part == op._part)  
        {
          // Simply replace the values.
          poi._iPart = op._iPart;
          poi._track = op._track;
          poi._intA = op._intA;
          return true;
        }
      break;
      
      case PendingOperationItem::ModifyPartName:
        if(poi._type == PendingOperationItem::ModifyPartName && poi._part == op._part &&
           poi._name == op._name)
        {
          fprintf(stderr, "MusE error: PendingOperationList::add(): Double ModifyPartName. Ignoring.\n");
          return false;  
        }
      break;
        
      
      case PendingOperationItem::AddEvent:
        if(poi._type == PendingOperationItem::AddEvent && poi._part == op._part && poi._ev == op._ev)  
        {
          fprintf(stderr, "MusE error: PendingOperationList::add(): Double AddEvent. Ignoring.\n");
          return false;  
        }
        else if(poi._type == PendingOperationItem::DeleteEvent && poi._part == op._part && poi._iev->second == op._ev)  
        {
          // Delete followed by add is useless. Cancel out the delete + add by erasing the delete command.
          erase(ipos->second);
          _map.erase(ipos);
          return true;  
        }
      break;
      
      case PendingOperationItem::DeleteEvent:
        if(poi._type == PendingOperationItem::DeleteEvent && poi._part == op._part && poi._iev->second == op._iev->second)  
        {
          fprintf(stderr, "MusE error: PendingOperationList::add(): Double DeleteEvent. Ignoring.\n");
          return false;  
        }
        else if(poi._type == PendingOperationItem::AddEvent && poi._part == op._part && poi._ev == op._iev->second)  
        {
          // Add followed by delete is useless. Cancel out the add + delete by erasing the add command.
          erase(ipos->second);
          _map.erase(ipos);
          return true;  
        }
      break;

      case PendingOperationItem::AddMidiCtrlVal:
        if(poi._type == PendingOperationItem::AddMidiCtrlVal && poi._mcvl == op._mcvl && poi._part == op._part)
        {
          // Simply replace the value.
          poi._intB = op._intB; 
          return true;
        }
        else if(poi._type == PendingOperationItem::DeleteMidiCtrlVal && poi._mcvl == op._mcvl && poi._imcv->second.part == op._part)
        {
          // Transform existing delete command into a modify command.
          poi._type = PendingOperationItem::ModifyMidiCtrlVal;
          poi._intA = op._intB; 
          return true;
        }
        else if(poi._type == PendingOperationItem::ModifyMidiCtrlVal && poi._mcvl == op._mcvl && poi._imcv->second.part == op._part)
        {
          // Simply replace the value.
          poi._intA = op._intB;
          return true;
        }
      break;
      
      case PendingOperationItem::DeleteMidiCtrlVal:
        if(poi._type == PendingOperationItem::DeleteMidiCtrlVal && poi._mcvl == op._mcvl && poi._imcv->second.part == op._imcv->second.part)
        {
          // Multiple delete commands not allowed! 
          fprintf(stderr, "MusE error: PendingOperationList::add(): Double DeleteMidiCtrlVal. Ignoring.\n");
          return false;
        }
        else if(poi._type == PendingOperationItem::AddMidiCtrlVal && poi._mcvl == op._mcvl && poi._part == op._imcv->second.part)
        {
          // Add followed by delete is useless. Cancel out the add + delete by erasing the add command.
          erase(ipos->second);
          _map.erase(ipos);
          return true;
        }
        else if(poi._type == PendingOperationItem::ModifyMidiCtrlVal && poi._mcvl == op._mcvl && poi._imcv->second.part == op._imcv->second.part)
        {
          // Modify followed by delete is equivalent to just deleting.
          // Transform existing modify command into a delete command.
          poi._type = PendingOperationItem::DeleteMidiCtrlVal;
          return true;
        }
      break;
      
      case PendingOperationItem::ModifyMidiCtrlVal:
        if(poi._type == PendingOperationItem::ModifyMidiCtrlVal && poi._mcvl == op._mcvl && poi._imcv->second.part == op._imcv->second.part)
        {
          // Simply replace the value.
          poi._intA = op._intA;
          return true;
        }
        else if(poi._type == PendingOperationItem::DeleteMidiCtrlVal && poi._mcvl == op._mcvl && poi._imcv->second.part == op._imcv->second.part)
        {
          // Transform existing delete command into a modify command.
          poi._type = PendingOperationItem::ModifyMidiCtrlVal;
          poi._intA = op._intA; 
          return true;
        }
        else if(poi._type == PendingOperationItem::AddMidiCtrlVal && poi._mcvl == op._mcvl && poi._part == op._imcv->second.part)
        {
          // Simply replace the add value with the modify value.
          poi._intB = op._intA; 
          return true;
        }
      break;
      
      
      case PendingOperationItem::AddTempo:
#ifdef _PENDING_OPS_DEBUG_
        fprintf(stderr, "PendingOperationList::add() AddTempo\n");
#endif      
        if(poi._type == PendingOperationItem::AddTempo && poi._tempo_list == op._tempo_list)
        {
          fprintf(stderr, "MusE error: PendingOperationList::add(): Double AddTempo. Ignoring.\n");
          return false;  
          // Simply replace the value.
        }
        else if(poi._type == PendingOperationItem::DeleteTempo && poi._tempo_list == op._tempo_list)
        {
          // Delete followed by add. Cannot cancel them out because add already created a new object. Allow to add...
        }
        else if(poi._type == PendingOperationItem::ModifyTempo && poi._tempo_list == op._tempo_list)
        {
          // Modify followed by add. Error.
          fprintf(stderr, "MusE error: PendingOperationList::add(): ModifyTempo then AddTempo. Ignoring.\n");
          return false;  
        }
      break;
        
      case PendingOperationItem::DeleteTempo:
#ifdef _PENDING_OPS_DEBUG_
        fprintf(stderr, "PendingOperationList::add() DeleteTempo\n");
#endif      
        if(poi._type == PendingOperationItem::DeleteTempo && poi._tempo_list == op._tempo_list)
        {
          fprintf(stderr, "MusE error: PendingOperationList::add(): Double DeleteTempo. Ignoring.\n");
          return false;  
        }
        else if(poi._type == PendingOperationItem::AddTempo && poi._tempo_list == op._tempo_list)
        {
          // Add followed by delete. Cannot cancel them out because add already created a new object. Allow to delete...
        }
        else if(poi._type == PendingOperationItem::ModifyTempo && poi._tempo_list == op._tempo_list)
        {
          // Modify followed by delete is equivalent to just deleting.
          // Transform existing modify command into a delete command.
          poi._type = PendingOperationItem::DeleteTempo;
          // Modify's iterator will point one AFTER the delete iterator. So decrement the iterator.
          //--poi._iTEvent;
          // Replace the modify iterator with the delete iterator.
          poi._iTEvent = op._iTEvent;
          poi._tempo_event = op._tempo_event;
          return true;
        }
      break;
      
      case PendingOperationItem::ModifyTempo:
#ifdef _PENDING_OPS_DEBUG_
        fprintf(stderr, "PendingOperationList::add() ModifyTempo\n");
#endif      
        if(poi._type == PendingOperationItem::ModifyTempo && poi._tempo_list == op._tempo_list)
        {
          // Simply replace the value.
          poi._intA = op._intA; 
          return true;
        }
        else if(poi._type == PendingOperationItem::AddTempo && poi._tempo_list == op._tempo_list)
        {
          // Add followed by modify. Just replace the add value
          poi._tempo_event->tempo = op._iTEvent->second->tempo;
          return true;
        }
        else if(poi._type == PendingOperationItem::DeleteTempo && poi._tempo_list == op._tempo_list)
        {
          // Transform existing delete command into a modify command.
          poi._type = PendingOperationItem::ModifyTempo;
          // Delete's iterator will point one BEFORE the modify iterator. So increment the iterator.
          //++poi._iTEvent;
          // Replace the delete iterator with the modify iterator.
          poi._iTEvent = op._iTEvent;
          // Grab the tempo.
          poi._intA = op._intA;
          // Delete always does normalize, so nowhere to grab this value from.
          //poi._intB = true;  
          return true;
        }
      break;
      
      case PendingOperationItem::SetGlobalTempo:
#ifdef _PENDING_OPS_DEBUG_
        fprintf(stderr, "PendingOperationList::add() SetGlobalTempo\n");
#endif      
        if(poi._type == PendingOperationItem::SetGlobalTempo && poi._tempo_list == op._tempo_list)
        {
          // Simply replace the new value.
          poi._intA = op._intA; 
          return true;
        }
      break;

      
      case PendingOperationItem::AddSig:
#ifdef _PENDING_OPS_DEBUG_
        fprintf(stderr, "PendingOperationList::add() AddSig\n");
#endif      
        if(poi._type == PendingOperationItem::AddSig && poi._sig_list == op._sig_list) 
        {
          fprintf(stderr, "MusE error: PendingOperationList::add(): Double AddSig. Ignoring.\n");
          return false;  
          // Simply replace the value.
        }
        else if(poi._type == PendingOperationItem::DeleteSig && poi._sig_list== op._sig_list) 
        {
          // Delete followed by add. Cannot cancel them out because add already created a new object. Allow to add...
        }
        else if(poi._type == PendingOperationItem::ModifySig && poi._sig_list == op._sig_list)
        {
          // Modify followed by add. Error.
          fprintf(stderr, "MusE error: PendingOperationList::add(): ModifySig then AddSig. Ignoring.\n");
          return false;  
        }
      break;
        
      case PendingOperationItem::DeleteSig:
#ifdef _PENDING_OPS_DEBUG_
        fprintf(stderr, "PendingOperationList::add() DeleteSig\n");
#endif      
        if(poi._type == PendingOperationItem::DeleteSig && poi._sig_list == op._sig_list)
        {
          fprintf(stderr, "MusE error: PendingOperationList::add(): Double DeleteSig. Ignoring.\n");
          return false;  
        }
        else if(poi._type == PendingOperationItem::AddSig && poi._sig_list == op._sig_list)
        {
          // Add followed by delete. Cannot cancel them out because add already created a new object. Allow to delete...
        }
        else if(poi._type == PendingOperationItem::ModifySig && poi._sig_list == op._sig_list)
        {
          // Modify followed by delete is equivalent to just deleting.
          // Transform existing modify command into a delete command.
          poi._type = PendingOperationItem::DeleteSig;
          // Modify's iterator will point one AFTER the delete iterator. So decrement the iterator.
          //--poi._iSigEvent;
          // Replace the modify iterator with the delete iterator.
          poi._iSigEvent = op._iSigEvent;
          poi._sig_event = op._sig_event;
          return true;
        }
      break;
      
      case PendingOperationItem::ModifySig:
#ifdef _PENDING_OPS_DEBUG_
        fprintf(stderr, "PendingOperationList::add() ModifySig\n");
#endif      
        if(poi._type == PendingOperationItem::ModifySig && poi._sig_list == op._sig_list)
        {
          // Simply replace the value.
          poi._intA = op._intA; 
          poi._intB = op._intB; 
          return true;
        }
        else if(poi._type == PendingOperationItem::AddSig && poi._sig_list == op._sig_list)
        {
          // Add followed by modify. Just replace the add value
          poi._sig_event->sig = op._iSigEvent->second->sig;
          return true;
        }
        else if(poi._type == PendingOperationItem::DeleteSig && poi._sig_list == op._sig_list)
        {
          // Transform existing delete command into a modify command.
          poi._type = PendingOperationItem::ModifySig;
          // Delete's iterator will point one BEFORE the modify iterator. So increment the iterator.
          //++poi._iSigEvent;
          // Replace the delete iterator with the modify iterator.
          poi._iSigEvent = op._iSigEvent;
          // Grab the signature.
          poi._intA = op._intA;
          poi._intB = op._intB;
          return true;
        }
      break;

      
      case PendingOperationItem::AddKey:
#ifdef _PENDING_OPS_DEBUG_
        fprintf(stderr, "PendingOperationList::add() AddKey\n");
#endif      
        if(poi._type == PendingOperationItem::AddKey && poi._key_list == op._key_list) 
        {
          // Simply replace the value.
          poi._intB = op._intB; 
          return true;
        }
        else if(poi._type == PendingOperationItem::DeleteKey && poi._key_list== op._key_list) 
        {
          // Transform existing delete command into a modify command.
          poi._type = PendingOperationItem::ModifyKey;
          poi._intA = op._intB; 
          return true;
        }
        else if(poi._type == PendingOperationItem::ModifyKey && poi._key_list == op._key_list)
        {
          // Simply replace the value.
          poi._intA = op._intB;
          return true;
        }
      break;
        
      case PendingOperationItem::DeleteKey:
#ifdef _PENDING_OPS_DEBUG_
        fprintf(stderr, "PendingOperationList::add() DeleteKey\n");
#endif      
        if(poi._type == PendingOperationItem::DeleteKey && poi._key_list == op._key_list)
        {
          // Multiple delete commands not allowed! 
          fprintf(stderr, "MusE error: PendingOperationList::add(): Double DeleteKey. Ignoring.\n");
          return false;  
        }
        else if(poi._type == PendingOperationItem::AddKey && poi._key_list == op._key_list)
        {
          // Add followed by delete is useless. Cancel out the add + delete by erasing the add command.
          erase(ipos->second);
          _map.erase(ipos);
          return true;
        }
        else if(poi._type == PendingOperationItem::ModifyKey && poi._key_list == op._key_list)
        {
          // Modify followed by delete is equivalent to just deleting.
          // Transform existing modify command into a delete command.
          poi._type = PendingOperationItem::DeleteKey;
          // Modify's iterator will point one AFTER the delete iterator. So decrement the iterator.
          //--poi._iKeyEvent;
          // Replace the modify iterator with the delete iterator.
          poi._iKeyEvent = op._iKeyEvent;
          return true;
        }
      break;
      
      case PendingOperationItem::ModifyKey:
#ifdef _PENDING_OPS_DEBUG_
        fprintf(stderr, "PendingOperationList::add() ModifyKey\n");
#endif      
        if(poi._type == PendingOperationItem::ModifyKey && poi._key_list == op._key_list)
        {
          // Simply replace the value.
          poi._intA = op._intA;
          return true;
        }
        else if(poi._type == PendingOperationItem::AddKey && poi._key_list == op._key_list)
        {
          // Simply replace the add value with the modify value.
          poi._intB = op._intA; 
          return true;
        }
        else if(poi._type == PendingOperationItem::DeleteKey && poi._key_list == op._key_list)
        {
          // Transform existing delete command into a modify command.
          poi._type = PendingOperationItem::ModifyKey;
          // Delete's iterator will point one BEFORE the modify iterator. So increment the iterator.
          //++poi._iKeyEvent;
          // Replace the delete iterator with the modify iterator.
          poi._iKeyEvent = op._iKeyEvent;
          // Replace the value.
          poi._intA = op._intA;
          return true;
        }
      break;
      
      
      case PendingOperationItem::ModifySongLength:
#ifdef _PENDING_OPS_DEBUG_
        fprintf(stderr, "PendingOperationList::add() ModifySongLength\n");
#endif      
        if(poi._type == PendingOperationItem::ModifySongLength)
        {
          // Simply replace the value.
          poi._intA = op._intA;
          return true;
        }
      break;  

      case PendingOperationItem::Uninitialized:
        fprintf(stderr, "MusE error: PendingOperationList::add(): Uninitialized item. Ignoring.\n");
        return false;  
      break;  
        
      default:
      break;  
    }
  }

  // Special for these types of lists: Because of the way these lists operate,
  //  a delete operation followed by a modify operation on the SAME iterator
  //  needs special attention: The delete operation will ERASE the iterator
  //  that the modify operation points to! Also the two operations will have 
  //  different sorting ticks and won't be caught in the above loop. 
  // The only way around it is to increment the modify iterator now so that it is 
  //  already pointing to the next item by the time the delete operation happens:
  if(op._type == PendingOperationItem::ModifyTempo || 
     op._type == PendingOperationItem::ModifySig || 
     op._type == PendingOperationItem::ModifyKey)
  {
    int idx = 0;
    if(op._type == PendingOperationItem::ModifyTempo)
      idx = op._iTEvent->first;
    else if(op._type == PendingOperationItem::ModifySig)
      idx = op._iSigEvent->first;
    else if(op._type == PendingOperationItem::ModifyKey)
      idx = op._iKeyEvent->first;
    
    iPendingOperationSortedRange r = _map.equal_range(idx);
    iPendingOperationSorted ipos = r.second;
    while(ipos != r.first)
    {
      --ipos;
      PendingOperationItem& poi = *ipos->second;
      
      if(op._type == PendingOperationItem::ModifyTempo)
      {
        if(poi._type == PendingOperationItem::DeleteTempo && poi._tempo_list == op._tempo_list)
        {
#ifdef _PENDING_OPS_DEBUG_
          fprintf(stderr, "PendingOperationList::add() DeleteTempo + ModifyTempo: Incrementing modify iterator: idx:%d cur tempo:%d tick:%d\n", 
                  idx, op._iTEvent->second->tempo, op._iTEvent->second->tick);
#endif      
          op._iTEvent++;
          break;
        }
      }
      else if(op._type == PendingOperationItem::ModifySig)
      {
        if(poi._type == PendingOperationItem::DeleteSig && poi._sig_list == op._sig_list)
        {
#ifdef _PENDING_OPS_DEBUG_
          fprintf(stderr, "PendingOperationList::add() DeleteSig + ModifySig: Incrementing modify iterator: idx:%d cur sig:%d/%d tick:%d\n", 
                  idx, op._iSigEvent->second->sig.z, op._iSigEvent->second->sig.n, op._iSigEvent->second->tick);
#endif      
          op._iSigEvent++;
          break;
        }
      }
      else if(op._type == PendingOperationItem::ModifyKey)
      {
        if(poi._type == PendingOperationItem::DeleteKey && poi._key_list == op._key_list)
        {
#ifdef _PENDING_OPS_DEBUG_
          fprintf(stderr, "PendingOperationList::add() DeleteKey + ModifyKey: Incrementing modify iterator: idx:%d cur key:%d tick:%d\n", 
                  idx, op._iKeyEvent->second.key, op._iKeyEvent->second.tick);
#endif      
          op._iKeyEvent++;
          break;
        }
      }
    }
  }
  
  iPendingOperation iipo = insert(end(), op);
  _map.insert(std::pair<int, iPendingOperation>(t, iipo));
  return true;
}

iPendingOperation PendingOperationList::findAllocationOp(const PendingOperationItem& op)
{
  iPendingOperationSortedRange r = _map.equal_range(op.getIndex());
  iPendingOperationSorted ipos = r.second;
  while(ipos != r.first)
  {
    --ipos;
    const PendingOperationItem& poi = *ipos->second;
    if(poi.isAllocationOp(op))  // Comparison.
      return ipos->second;
  }  
  return end();
}


} // namespace MusECore
  
