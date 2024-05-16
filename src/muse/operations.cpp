//=========================================================
//  MusE
//  Linux Music Editor
//    operations.cpp 
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

#include "operations.h"
#include "song.h"
#include "globals.h"
#include "synth.h"
#include "muse_time.h"
#include "config.h"
#include "gconfig.h"

// Forwards from header:
#include "tempo.h" 
#include "keyevent.h"
#include "midiport.h"
#include "metronome_class.h"
#include "audio_convert/audio_converter_plugin.h"
#include "audio_convert/audio_converter_settings_group.h"
#include "midiremote.h"

// Enable for debugging:
//#define _PENDING_OPS_DEBUG_

// For debugging output: Uncomment the fprintf section.
#define ERROR_OPERATIONS(dev, format, args...)  fprintf(dev, format, ##args)
#define DEBUG_OPERATIONS(dev, format, args...)  //fprintf(dev, format, ##args)

namespace MusECore {

//-----------------------------------
//  MidiCtrlValListIterators
//-----------------------------------
  
MidiCtrlValListIterators::iterator MidiCtrlValListIterators::findList(const MidiCtrlValList* valList)
{
  for(iterator i = begin(); i != end(); ++i)
    if((*i)->second == valList)
      return i;
  return end();
}
  
MidiCtrlValListIterators::const_iterator MidiCtrlValListIterators::findList(const MidiCtrlValList* valList) const
{
  for(const_iterator i = begin(); i != end(); ++i)
    if((*i)->second == valList)
      return i;
  return end();
}
  
//-----------------------------------
//  MidiCtrlValLists2bErased
//-----------------------------------
  
void MidiCtrlValLists2bErased::add(int port, const iMidiCtrlValList& item)
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

MidiCtrlValLists2bErased::iterator MidiCtrlValLists2bErased::findList(int port, const MidiCtrlValList* valList)
{
  iterator i = find(port);
  if(i == end())
    return end();
  if(i->second.findList(valList) != i->second.end())
    return i;
  return end();
}

MidiCtrlValLists2bErased::const_iterator MidiCtrlValLists2bErased::findList(int port, const MidiCtrlValList* valList) const
{
  const_iterator i = find(port);
  if(i == end())
    return end();
  if(i->second.findList(valList) != i->second.end())
    return i;
  return end();
}
  
//-----------------------------------
//  PendingOperationItem
//-----------------------------------

bool PendingOperationItem::isAllocationOp(const PendingOperationItem& op) const
{
  switch(op._type)
  {
    case AddMidiCtrlValList:
      // A is channel B is control.
      if(_type == AddMidiCtrlValList && _mcvll == op._mcvll && _intA == op._intA && _intB == op._intB)
        return true;
    break;
    
    // In the case of type AddMidiDevice, this searches for the name only.
    case AddMidiDevice:
      if(_type == AddMidiDevice && _midi_device_list == op._midi_device_list &&
         _midi_device->name() == op._midi_device->name())
        return true;
    break;
    
    default:
    break;  
  }
  
  return false;
}

unsigned int PendingOperationItem::getIndex() const
{
  switch(_type)
  {
    case Uninitialized:
    case AddAuxSendValue:
    case AddMidiInstrument:
    case DeleteMidiInstrument:
    case ReplaceMidiInstrument:
    case AddMidiDevice:
    case DeleteMidiDevice:
    case ModifyMidiDeviceAddress:
    case ModifyMidiDeviceFlags:
    case ModifyMidiDeviceName:
    case SetInstrument:
    case AddTrack:
    case DeleteTrack:
    case MoveTrack:
    case ModifyTrackName:
    case ModifyTrackDrumMapItem:
    case ReplaceTrackDrumMapPatchList:
    case RemapDrumControllers:
    case UpdateDrumMaps:
    case SetTrackRecord:
    case SetTrackMute:
    case SetTrackSolo:
    case SetTrackRecMonitor:
    case SetTrackOff:
    case ModifyPartName:
    case ModifySongLength:
    case AddMidiCtrlValList:
    case ModifyAudioCtrlValList:
    case AddAudioCtrlValList:
    case DeleteAudioCtrlValList:
    case ModifyAudioCtrlValListList:
    case SelectAudioCtrlVal:
    case SetAudioCtrlPasteEraseMode:
    case SetGlobalTempo:
    case AddRoute:
    case DeleteRoute:
    case AddRouteNode:
    case DeleteRouteNode:
    case ModifyRouteNode:
    case UpdateSoloStates:
    case EnableAllAudioControllers:
    case GlobalSelectAllEvents:
    case SwitchMetronomeSettings:
    case SwitchMidiRemoteSettings:
    case ModifyMetronomeAccentMap:
    case ModifyMidiRemote:
    case SetExternalSyncFlag:
    case SetUseJackTransport:
    case SetUseMasterTrack:
    case ModifyAudioSamples:
    case SetStaticTempo:
    case ModifyLocalAudioConverterSettings:
    case ModifyLocalAudioConverter:
    case ModifyDefaultAudioConverterSettings:
    case ModifyStretchListRatio:
    case SetAudioConverterOfflineMode:
    case ModifyMarkerList:
    case ModifyTempoList:
    case ModifySigList:
    case ModifyKeyList:
    case ModifyEventList:
    case ModifyMidiCtrlValList:
    case UpdateAllAudioCtrlGroups:
    case UpdateAudioCtrlListGroups:
    case UpdateAudioCtrlGroups:
    case UpdateAudioCtrlPosGroups:

      // To help speed up searches of these ops, let's (arbitrarily) set index = type instead of all of them being at index 0!
      return _type;
    
    case ModifyPartStart:
      return _iPart->second->posValue();

    case ModifyPartLength:
      return _part->posValue();
    
    case MovePart:
      // _part is used here rather than _iPart since _iPart can be end().
      return _part->posValue();
    
    case AddPart:
      return _part->posValue();  
    
    case DeletePart:
      return _iPart->second->posValue();

    case SelectPart:
      return _part->posValue();

      
    case AddEvent:
      return _ev.posValue();
    
    case DeleteEvent:
      return _ev.posValue();
    
    case ModifyEventProperties:
      return _posLenVal;  // Tick or frame.
    
    case SelectEvent:
      return _ev.posValue();
      
      
    case AddMidiCtrlVal:
      return _posLenVal;  // Tick
    
    case DeleteMidiCtrlVal:
      return _imcv->first;  // Tick
    
    case ModifyMidiCtrlVal:
      return _imcv->first;  // Tick

    
    case AddAudioCtrlVal:
      return _posLenVal;  // Frame
    
    case AddAudioCtrlValStruct:
      return _posLenVal;  // Frame

    case DeleteAudioCtrlVal:
      return _iCtrl->first;  // Frame
    
    case ModifyAudioCtrlVal:
      return _iCtrl->first;  // Frame

    case AddStretchListRatioAt:
      return _museFrame;  // Frame
    
    case DeleteStretchListRatioAt:
      return _iStretchEvent->first;  // Frame
    
    case ModifyStretchListRatioAt:
      return _iStretchEvent->first;  // Frame

    default:
      ERROR_OPERATIONS(stderr, "PendingOperationItem::getIndex unknown op type: %d\n", _type);
      return 0;
    break;  
  }
}  

SongChangedStruct_t PendingOperationItem::executeRTStage()
{
  SongChangedStruct_t flags = 0;
  switch(_type)
  {
    case ModifyDefaultAudioConverterSettings:
      DEBUG_OPERATIONS(stderr, "PendingOperationItem::executeRTStage ModifyDefaultAudioConverterSettings: "
                                "settings:%p\n", _audio_converter_settings);
      if(_audio_converter_settings)
      {
        // Grab the current settings.
        AudioConverterSettingsGroup* cur_settings = MusEGlobal::defaultAudioConverterSettings;
        // Now change the actual global pointer variable.
        MusEGlobal::defaultAudioConverterSettings = _audio_converter_settings;
        // Transfer the original pointer into the member, so it can be deleted in the non-RT stage.
        _audio_converter_settings = cur_settings;
        flags |= SC_AUDIO_CONVERTER;
      }
    break;
    
    case ModifyLocalAudioConverterSettings:
    {
      DEBUG_OPERATIONS(stderr, "PendingOperationItem::executeRTStage ModifyLocalAudioConverterSettings: "
                                "sndFile:%p settings:%p\n", 
                                *_sndFileR, _audio_converter_settings);

      // _audio_converter_settings can be NULL meaning don't touch, settings can only be 
      //  'replaced' but not deleted.
      if(_audio_converter_settings)
      {
        AudioConverterSettingsGroup* cur_settings = _sndFileR.audioConverterSettings();
        _sndFileR.setAudioConverterSettings(_audio_converter_settings);
        // Transfer the original pointer into the member, so it can be deleted in the non-RT stage.
        _audio_converter_settings = cur_settings;
        flags |= SC_AUDIO_CONVERTER;
      }
    }
    break;
      
    case ModifyLocalAudioConverter:
    {
      DEBUG_OPERATIONS(stderr, "PendingOperationItem::executeRTStage ModifyLocalAudioConverter: "
                                "sndFile:%p audio_converter:%p audio_converter_ui:%p\n", 
                                *_sndFileR, _audio_converter, _audio_converter_ui);

      // _audio_converter and _audio_converter_ui can be NULL meaning delete them.
      AudioConverterPluginI* cur_conv = _sndFileR.staticAudioConverter(AudioConverterSettings::RealtimeMode);
      //if(_audio_converter)
        _sndFileR.setStaticAudioConverter(_audio_converter, AudioConverterSettings::RealtimeMode);
      // Transfer the original pointer into the member, so it can be deleted in the non-RT stage.
      _audio_converter = cur_conv;
      
      AudioConverterPluginI* cur_convUI = _sndFileR.staticAudioConverter(AudioConverterSettings::GuiMode);
      //if(_audio_converter_ui)
        _sndFileR.setStaticAudioConverter(_audio_converter_ui, AudioConverterSettings::GuiMode);
      // Transfer the original pointer into the member, so it can be deleted in the non-RT stage.
      _audio_converter_ui = cur_convUI;
      flags |= SC_AUDIO_CONVERTER;
    }
    break;
      
    case SetAudioConverterOfflineMode:
    {
//      DEBUG_OPERATIONS(stderr, "PendingOperationItem::executeRTStage SetAudioConverterOfflineMode: "
//                                "sndFile:%p audio_converter:%p\n",
//                                *_sndFile, _audio_converter);

      AudioConverterPluginI* cur_conv = _sndFileR.staticAudioConverter(AudioConverterSettings::RealtimeMode);
      //if(_audio_converter)
        _sndFileR.setStaticAudioConverter(_audio_converter, AudioConverterSettings::RealtimeMode);
      // Transfer the original pointer into the member, so it can be deleted in the non-RT stage.
      _audio_converter = cur_conv;
      flags |= SC_AUDIO_CONVERTER;
    }
    break;


    case ModifyTrackDrumMapItem:
    {
#ifdef _PENDING_OPS_DEBUG_
      fprintf(stderr, "PendingOperationItem::executeRTStage ModifyTrackDrumMapItem drummap operation:%p\n",
              _drum_map_track_operation);
#endif

      MidiTrack* mt;
      MidiTrackList& mtl = _drum_map_track_operation->_tracks;
      for(iMidiTrack imt = mtl.begin(); imt != mtl.end(); ++imt)
      {
        mt = *imt;
        // FIXME Possible non realtime-friendly allocation.
        mt->modifyWorkingDrumMap(_drum_map_track_operation->_workingItemList,
                                 _drum_map_track_operation->_isReset,
                                 _drum_map_track_operation->_includeDefault,
                                 _drum_map_track_operation->_isInstrumentMod,
                                 _drum_map_track_operation->_doWholeMap);
        flags |= (SC_DRUMMAP);
      }

      // If this is an instrument modification we must now do a
      //  general update of all drum track drum maps.
      // Ideally we would like to update only the required ones,
      //  but it is too difficult to tell which maps need updating
      //  from inside the above loop (or inside modifyWorkingDrumMap).
      if(_drum_map_track_operation->_isInstrumentMod)
      {
        MidiTrackList* mtlp = MusEGlobal::song->midis();
        for(iMidiTrack imt = mtlp->begin(); imt != mtlp->end(); ++imt)
        {
          mt = *imt;
          if(mt->type() != Track::DRUM)
            continue;
          if(mt->updateDrummap(false))
            flags |= (SC_DRUMMAP);
        }
      }
      flags |= (SC_DRUMMAP);
    }
    break;

    case ReplaceTrackDrumMapPatchList:
    {
#ifdef _PENDING_OPS_DEBUG_
      fprintf(stderr, "PendingOperationItem::executeRTStage ReplaceTrackDrumMapPatchList drummap operation:%p\n",
              _drum_map_track_patch_replace_operation);
#endif

      MidiTrack* mt = _drum_map_track_patch_replace_operation->_track;
      WorkingDrumMapPatchList* orig_wdmpl = mt->workingDrumMap();
      // Simply switch pointers. Be sure to delete the original pointers later in the non-realtime stage.
      // After the list pointers have been switched, swap with the replacement so that it can be deleted later.
      mt->setWorkingDrumMap(_drum_map_track_patch_replace_operation->_workingItemPatchList,
                            _drum_map_track_patch_replace_operation->_isInstrumentMod);
      _drum_map_track_patch_replace_operation->_workingItemPatchList = orig_wdmpl;

      // If this is an instrument modification we must now do a
      //  general update of all drum track drum maps.
      // Ideally we would like to update only the required ones,
      //  but it is too difficult to tell which maps need updating
      //  from inside the above loop (or inside modifyWorkingDrumMap).
      if(_drum_map_track_patch_replace_operation->_isInstrumentMod)
      {
        MidiTrackList* mtlp = MusEGlobal::song->midis();
        for(iMidiTrack imt = mtlp->begin(); imt != mtlp->end(); ++imt)
        {
          mt = *imt;
          if(mt->type() != Track::DRUM)
            continue;
          if(mt->updateDrummap(false))
            flags |= (SC_DRUMMAP);
        }
      }
      flags |= (SC_DRUMMAP);
    }
    break;

    case RemapDrumControllers:
    {
#ifdef _PENDING_OPS_DEBUG_
      fprintf(stderr, "PendingOperationItem::executeRTStage RemapDrumControllers remap operation:%p\n",
              _midi_ctrl_val_remap_operation);
#endif

      for(iMidiCtrlValLists2bErased_t imcvle = _midi_ctrl_val_remap_operation->_midiCtrlValLists2bErased.begin();
          imcvle != _midi_ctrl_val_remap_operation->_midiCtrlValLists2bErased.end(); ++imcvle)
      {
        const int port = imcvle->first;
        MidiCtrlValListIterators& mcvli = imcvle->second;
        MidiPort* mp = &MusEGlobal::midiPorts[port];
        MidiCtrlValListList* mcvll = mp->controller();
        for(iMidiCtrlValListIterators_t imcvli = mcvli.begin(); imcvli != mcvli.end(); ++imcvli)
          mcvll->del(*imcvli);
      }

      for(iMidiCtrlValLists2bAdded_t imcvla = _midi_ctrl_val_remap_operation->_midiCtrlValLists2bAdded.begin();
          imcvla != _midi_ctrl_val_remap_operation->_midiCtrlValLists2bAdded.end(); ++imcvla)
      {
        const int port = imcvla->first;
        MidiCtrlValListList* mcvll_a = imcvla->second;
        MidiPort* mp = &MusEGlobal::midiPorts[port];
        MidiCtrlValListList* mcvll = mp->controller();
        for(iMidiCtrlValList imcvl = mcvll_a->begin(); imcvl != mcvll_a->end(); ++imcvl)
          mcvll->add(imcvl->first >> 24, imcvl->second);
      }

      // TODO: What to use here? We don't have anything SC_* related... yet.
      //flags |= (SC_MIDI_CONTROLLER_ADD);
      flags |= (SC_EVERYTHING);
    }
    break;

    case UpdateDrumMaps:
#ifdef _PENDING_OPS_DEBUG_
      fprintf(stderr, "PendingOperationItem::executeRTStage UpdateDrumMaps: midi_port:%p:\n", _midi_port);
#endif      
      if(_midi_port->updateDrumMaps())
        flags |= SC_DRUMMAP;
    break;
    
    case UpdateSoloStates:
      DEBUG_OPERATIONS(stderr, "PendingOperationItem::executeRTStage UpdateSoloStates: track_list:%p:\n", _track_list);
      // TODO Use the track_list, or simply keep as dummy parameter to identify UpdateSoloStates?
      MusEGlobal::song->updateSoloStates();
      flags |= SC_SOLO;
    break;
    
    // TODO: Try to break this operation down so that only the actual operation is executed stage-2. 
    case AddRoute:
#ifdef _PENDING_OPS_DEBUG_
      DEBUG_OPERATIONS(stderr, "PendingOperationItem::executeRTStage AddRoute: src/dst routes:\n");
      _src_route.dump();
      _dst_route.dump();
#endif      
      if(addRoute(_src_route, _dst_route))
        flags |= SC_ROUTE;
    break;
    
    // TODO: Try to break this operation down so that only the actual operation is executed stage-2. 
    case DeleteRoute:
#ifdef _PENDING_OPS_DEBUG_
      DEBUG_OPERATIONS(stderr, "PendingOperationItem::executeRTStage DeleteRoute: src/dst routes:\n");
      _src_route.dump();
      _dst_route.dump();
#endif      
      if(removeRoute(_src_route, _dst_route))
        flags |= SC_ROUTE;
    break;
    
    case AddRouteNode:
#ifdef _PENDING_OPS_DEBUG_
      DEBUG_OPERATIONS(stderr, "PendingOperationItem::executeRTStage AddRouteNode: route_list:%p route:\n", _route_list);
      _src_route.dump();
#endif      
      _route_list->push_back(_src_route);
      flags |= SC_ROUTE;
    break;
    
    case DeleteRouteNode:
#ifdef _PENDING_OPS_DEBUG_
      DEBUG_OPERATIONS(stderr, "PendingOperationItem::executeRTStage DeleteRouteNode: route_list:%p route:\n", _route_list);
      _iRoute->dump();
#endif      
      _route_list->erase(_iRoute);
      flags |= SC_ROUTE;
    break;
    
    case ModifyRouteNode:
#ifdef _PENDING_OPS_DEBUG_
      DEBUG_OPERATIONS(stderr, "PendingOperationItem::executeRTStage ModifyRouteNode: src/dst routes:\n");
      _src_route.dump();
      _dst_route_pointer->dump();
#endif      
      *_dst_route_pointer = _src_route;
      flags |= SC_ROUTE;
    break;

    case AddAuxSendValue:
      DEBUG_OPERATIONS(stderr, "PendingOperationItem::executeRTStage AddAuxSendValue aux_send_value_list:%p val:%f\n", _aux_send_value_list, _aux_send_value);
      _aux_send_value_list->push_back(_aux_send_value);
    break;

    
    case AddMidiInstrument:
      DEBUG_OPERATIONS(stderr, "PendingOperationItem::executeRTStage AddMidiInstrument instrument_list:%p instrument:%p\n", _midi_instrument_list, _midi_instrument);
      _midi_instrument_list->push_back(_midi_instrument);
      flags |= SC_CONFIG | SC_MIDI_INSTRUMENT | SC_DRUMMAP | SC_MIDI_CONTROLLER_ADD;
    break;
    
    case DeleteMidiInstrument:
      DEBUG_OPERATIONS(stderr, "PendingOperationItem::executeRTStage DeleteMidiInstrument instrument_list:%p instrument:%p\n", _midi_instrument_list, *_iMidiInstrument);
      _midi_instrument_list->erase(_iMidiInstrument);
      flags |= SC_CONFIG | SC_MIDI_INSTRUMENT | SC_DRUMMAP | SC_MIDI_CONTROLLER_ADD;
    break;
    
    case ReplaceMidiInstrument:
    {
#ifdef _PENDING_OPS_DEBUG_
      fprintf(stderr, "PendingOperationItem::executeRTStage ReplaceMidiInstrument instrument_list:%p instrument:%p new_ instrument:%p\n",
              _midi_instrument_list, *_iMidiInstrument, _midi_instrument);
#endif
      // Grab the existing pointer to be deleted.
      MidiInstrument* orig = *_iMidiInstrument;
      // Erase from the list.
      _midi_instrument_list->erase(_iMidiInstrument);
      // Add the new instrument.
      _midi_instrument_list->push_back(_midi_instrument);

      // Change all ports which used the original instrument.
      for(int port = 0; port < MusECore::MIDI_PORTS; ++port)
      {
        MidiPort* mp = &MusEGlobal::midiPorts[port];
        if(mp->instrument() != orig)
          continue;
        // Set the new instrument and nothing more (ie. don't use MidiPort::changeInstrument()).
        // Here we will flag the initializations, and normalize and update the drum maps.
        mp->setInstrument(_midi_instrument);
        // Flag the port to send initializations next time it bothers to check.
        // TODO: Optimize: We only need this if the user changed the initialization
        //        lists or sysex list. Find a way to pass that info here.
        mp->clearInitSent();
      }

      // Since this is an instrument modification we must now do a
      //  general update of all drum track drum maps using this instrument.
      // Ideally we would like to update only the required ones,
      //  but it is too difficult to tell which maps need updating
      //  from inside the above loop (or inside modifyWorkingDrumMap).
      MidiTrack* mt;
      int mt_port;
      MidiPort* mt_mp;
      MidiTrackList* mtlp = MusEGlobal::song->midis();
      for(iMidiTrack imt = mtlp->begin(); imt != mtlp->end(); ++imt)
      {
        mt = *imt;
        if(mt->type() != Track::DRUM)
          continue;
        mt_port = mt->outPort();
        if(mt_port < 0 || mt_port >= MusECore::MIDI_PORTS)
          continue;
        mt_mp = &MusEGlobal::midiPorts[mt_port];
        // We are looking for tracks which are now using the new instrument.
        if(mt_mp->instrument() != _midi_instrument)
          continue;
        // Ensure there are NO duplicate enote fields.
        //mt->normalizeWorkingDrumMapPatchList();
        // Finally, update the track's drum map (and drum in map).
        mt->updateDrummap(false);
      }

      // Transfer the original pointer back to _midi_instrument so it can be deleted in the non-RT stage.
      _midi_instrument = orig;

      flags |= SC_CONFIG | SC_MIDI_INSTRUMENT | SC_DRUMMAP | SC_MIDI_CONTROLLER_ADD;
    }
    break;

    case AddMidiDevice:
      DEBUG_OPERATIONS(stderr, "PendingOperationItem::executeRTStage AddMidiDevice devicelist:%p device:%p\n", _midi_device_list, _midi_device);
      _midi_device_list->push_back(_midi_device);
      flags |= SC_CONFIG;
    break;
    
    case DeleteMidiDevice:
      DEBUG_OPERATIONS(stderr, "PendingOperationItem::executeRTStage DeleteMidiDevice devicelist:%p device:%p\n", _midi_device_list, *_iMidiDevice);
      _midi_device_list->erase(_iMidiDevice);
      flags |= SC_CONFIG;
    break;
    
    case ModifyMidiDeviceAddress:
      DEBUG_OPERATIONS(stderr, "PendingOperationItem::executeRTStage ModifyMidiDeviceAddress device:%p client:%d port:%d\n", _midi_device, _address_client, _address_port);
      _midi_device->setAddressClient(_address_client);
      _midi_device->setAddressPort(_address_port);
      _midi_device->setOpenFlags(_open_flags);
      flags |= SC_CONFIG;
    break;
    
    case ModifyMidiDeviceFlags:
      DEBUG_OPERATIONS(stderr, "PendingOperationItem::executeRTStage ModifyMidiDeviceFlags device:%p rwFlags:%d openFlags:%d\n", _midi_device, _rw_flags, _open_flags);
      _midi_device->setrwFlags(_rw_flags);
      _midi_device->setOpenFlags(_open_flags);
      flags |= SC_CONFIG;
    break;
    
    case ModifyMidiDeviceName:
      DEBUG_OPERATIONS(stderr, "PendingOperationItem::executeRTStage ModifyMidiDeviceName device:%p name:%s\n", _midi_device, _name->toLocal8Bit().data());
      _midi_device->setName(*_name);
      flags |= SC_CONFIG;
    break;
    
    case SetInstrument:
#ifdef _PENDING_OPS_DEBUG_
      fprintf(stderr, "PendingOperationItem::executeRTStage SetInstrument port:%p instr:%p\n",
              _midi_port, _midi_instrument);
#endif      
      _midi_port->setInstrument(_midi_instrument);
      // Flag the port to send initializations next time it bothers to check.
      _midi_port->clearInitSent();
      flags |= SC_MIDI_INSTRUMENT;
    break;
    
    case AddTrack:
    {
      DEBUG_OPERATIONS(stderr, "PendingOperationItem::executeRTStage AddTrack track_list:%p track:%p\n", _track_list, _track);
      if(_void_track_list)
      {
        switch(_track->type())
        {
              case Track::MIDI:
              case Track::DRUM:
                    static_cast<MidiTrackList*>(_void_track_list)->push_back(static_cast<MidiTrack*>(_track));
                    break;
              case Track::WAVE:
                    static_cast<WaveTrackList*>(_void_track_list)->push_back(static_cast<WaveTrack*>(_track));
                    break;
              case Track::AUDIO_OUTPUT:
                    //--------------------------------------------------------------
                    // Connect metronome audio click
                    //--------------------------------------------------------------
                    // Are there no other existing AudioOutput tracks?
                    if(static_cast<OutputList*>(_void_track_list)->empty())
                      // Do the user a favour: Auto-connect the metronome to the track.
                      static_cast<AudioOutput*>(_track)->setSendMetronome(true);
                    
                    static_cast<OutputList*>(_void_track_list)->push_back(static_cast<AudioOutput*>(_track));
                    break;
              case Track::AUDIO_GROUP:
                    static_cast<GroupList*>(_void_track_list)->push_back(static_cast<AudioGroup*>(_track));
                    break;
              case Track::AUDIO_AUX:
                    static_cast<AuxList*>(_void_track_list)->push_back(static_cast<AudioAux*>(_track));
                    // Special for aux, make it easier to detect their changes.
                    flags |= SC_AUX;
                    break;
              case Track::AUDIO_INPUT:
                    static_cast<InputList*>(_void_track_list)->push_back(static_cast<AudioInput*>(_track));
                    break;
              case Track::AUDIO_SOFTSYNTH:
                    static_cast<SynthIList*>(_void_track_list)->push_back(static_cast<SynthI*>(_track));
                    break;
              default:
                    ERROR_OPERATIONS(stderr, "PendingOperationItem::executeRTStage AddTrack: Unknown track type %d\n", _track->type());
                    return flags;
        }
      }
      
      iTrack track_it = _track_list->index2iterator(_insert_at);
      _track_list->insert(track_it, _track);
      flags |= SC_TRACK_INSERTED;

      // Add routes:
      if(_track->isMidiTrack())
      {
            // Add any port output routes to this track
            const RouteList* rl = _track->inRoutes();
            for(ciRoute r = rl->begin(); r != rl->end(); ++r)
            {
                  switch(r->type)
                  {
                    case Route::MIDI_PORT_ROUTE:  {
                      Route src(_track, r->channel);
                      MusEGlobal::midiPorts[r->midiPort].outRoutes()->push_back(src);
                      flags |= SC_ROUTE; }
                    break;
                    case Route::TRACK_ROUTE:
                    case Route::JACK_ROUTE:
                    case Route::MIDI_DEVICE_ROUTE:
                    break;
                  }
            }
            // Add any port input routes from this track
            rl = _track->outRoutes();
            for(ciRoute r = rl->begin(); r != rl->end(); ++r)
            {
                  switch(r->type)
                  {
                    case Route::MIDI_PORT_ROUTE:  {
                      Route src(_track, r->channel);
                      MusEGlobal::midiPorts[r->midiPort].inRoutes()->push_back(src);
                      flags |= SC_ROUTE; }
                    break;
                    case Route::TRACK_ROUTE:
                    case Route::JACK_ROUTE:
                    case Route::MIDI_DEVICE_ROUTE:
                    break;
                  }
            }      
      }
      else 
      {
            // Add other tracks' output routes to this track
            const RouteList* rl = _track->inRoutes();
            for(ciRoute r = rl->begin(); r != rl->end(); ++r)
            {
                  switch(r->type)
                  {
                    case Route::TRACK_ROUTE: {
                      Route src(_track, r->remoteChannel, r->channels);
                      src.remoteChannel = r->channel;
                      r->track->outRoutes()->push_back(src);
                      flags |= SC_ROUTE;
                      // Is the source an Aux Track or else does it have Aux Tracks routed to it?
                      // Update this track's aux ref count.
                      if(r->track->auxRefCount())
                      {
                        _track->updateAuxRoute(r->track->auxRefCount(), nullptr);
                      }
                      else if(r->track->type() == Track::AUDIO_AUX)
                      {
                        _track->updateAuxRoute(1, nullptr);
                      }
                    }
                    break;
                    case Route::MIDI_PORT_ROUTE:
                    case Route::JACK_ROUTE:
                    case Route::MIDI_DEVICE_ROUTE:
                    break;
                  }
            }
            // Add other tracks' input routes from this track
            rl = _track->outRoutes();
            for(ciRoute r = rl->begin(); r != rl->end(); ++r)
            {
                  switch(r->type)
                  {
                    case Route::TRACK_ROUTE: {
                      Route src(_track, r->remoteChannel, r->channels);
                      src.remoteChannel = r->channel;
                      r->track->inRoutes()->push_back(src);
                      flags |= SC_ROUTE;
                      // Is this track an Aux Track or else does it have Aux Tracks routed to it?
                      // Update the other track's aux ref count and all tracks it is connected to.
                      if(_track->auxRefCount())
                      {
                        r->track->updateAuxRoute(_track->auxRefCount(), nullptr);
                      }
                      else if(_track->type() == Track::AUDIO_AUX)
                      {
                        r->track->updateAuxRoute(1, nullptr);
                      }
                    }
                    break;
                    case Route::MIDI_PORT_ROUTE:
                    case Route::JACK_ROUTE:
                    case Route::MIDI_DEVICE_ROUTE:
                    break;
                  }
            }      
      }
      chainTrackParts(_track);
    }
    break;
    
    case DeleteTrack:
    {
      DEBUG_OPERATIONS(stderr, "PendingOperationItem::executeRTStage DeleteTrack track_list:%p track:%p sec_track_list:%p\n", _track_list, _track, _void_track_list);
      unchainTrackParts(_track);
      if(_void_track_list)
      {
        switch(_track->type())
        {
              case Track::MIDI:
              case Track::DRUM:
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
                    // Special for aux, make it easier to detect their changes.
                    flags |= SC_AUX;
                    break;
              case Track::AUDIO_INPUT:
                    static_cast<InputList*>(_void_track_list)->erase(_track);
                    break;
              case Track::AUDIO_SOFTSYNTH:
                    {
                      static_cast<SynthIList*>(_void_track_list)->erase(_track);
                      
                      // Change all ports which used the instrument.
                      // FIXME TODO: We want to make this undoable but ATM a few other things can
                      //  set the instrument without an undo operation so the undo sequence would
                      //  not be correct. So we don't have much choice but to just reset for now.
                      // Still, everything else is in place for undoable setting of instrument...
                      const SynthI* si = static_cast<const SynthI*>(_track);
                      for(int port = 0; port < MusECore::MIDI_PORTS; ++port)
                      {
                        MidiPort* mp = &MusEGlobal::midiPorts[port];
                        if(mp->instrument() != si)
                          continue;
                        // Just revert to GM.
                        mp->setInstrument(registerMidiInstrument("GM"));
                        // Flag the port to send initializations next time it bothers to check.
                        mp->clearInitSent();
                        flags |= SC_MIDI_INSTRUMENT;
                      }
                    }     
                    break;
              default:
                    ERROR_OPERATIONS(stderr, "PendingOperationItem::executeRTStage DeleteTrack: Unknown track type %d\n", _track->type());
                    return flags;
        }
      }
      _track_list->erase(_track);
      flags |= SC_TRACK_REMOVED;

      // Remove routes:
      if(_track->type() == Track::AUDIO_OUTPUT) 
      {
            // Clear the track's jack ports
            for(int ch = 0; ch < _track->channels(); ++ch)
            {
              ((AudioOutput*)_track)->setJackPort(ch, 0);
              flags |= SC_ROUTE;
            }
            
            // Clear the track's output routes' jack ports
            RouteList* orl = _track->outRoutes();
            for(iRoute r = orl->begin(); r != orl->end(); ++r)
            {
              if(r->type != Route::JACK_ROUTE)
                continue;
              r->jackPort = 0;
              flags |= SC_ROUTE;
            }
      }
      else if(_track->type() == Track::AUDIO_INPUT) 
      {
            // Clear the track's jack ports
            for(int ch = 0; ch < _track->channels(); ++ch)
            {
              ((AudioInput*)_track)->setJackPort(ch, 0);
              flags |= SC_ROUTE;
            }
            
            // Clear the track's input routes' jack ports
            RouteList* irl = _track->inRoutes();
            for(iRoute r = irl->begin(); r != irl->end(); ++r)
            {
              if(r->type != Route::JACK_ROUTE)
                continue;
              r->jackPort = 0;
              flags |= SC_ROUTE;
            }
      }
      
      if(_track->isMidiTrack())
      {
            // Remove any port output routes to this track
            const RouteList* rl = _track->inRoutes();
            for(ciRoute r = rl->begin(); r != rl->end(); ++r)
            {
                  switch(r->type)
                  {
                    case Route::MIDI_PORT_ROUTE:  {
                      Route src(_track, r->channel);
                      MusEGlobal::midiPorts[r->midiPort].outRoutes()->removeRoute(src);
                      flags |= SC_ROUTE; }
                    break;
                    case Route::TRACK_ROUTE:
                    case Route::JACK_ROUTE:
                    case Route::MIDI_DEVICE_ROUTE:
                    break;
                  }
            }
            // Remove any port input routes from this track
            rl = _track->outRoutes();
            for(ciRoute r = rl->begin(); r != rl->end(); ++r)
            {
                  switch(r->type)
                  {
                    case Route::MIDI_PORT_ROUTE:  {
                      Route src(_track, r->channel);
                      MusEGlobal::midiPorts[r->midiPort].inRoutes()->removeRoute(src);
                      flags |= SC_ROUTE; }
                    break;
                    case Route::TRACK_ROUTE:
                    case Route::JACK_ROUTE:
                    case Route::MIDI_DEVICE_ROUTE:
                    break;
                  }
            }      
      }
      else 
      {
            // Remove other tracks' output routes to this track
            const RouteList* rl = _track->inRoutes();
            for(ciRoute r = rl->begin(); r != rl->end(); ++r)
            {
                  switch(r->type)
                  {
                    case Route::TRACK_ROUTE: {
                      Route src(_track, r->remoteChannel, r->channels);
                      src.remoteChannel = r->channel;
                      r->track->outRoutes()->removeRoute(src);
                      flags |= SC_ROUTE; 
                      // Is the source an Aux Track or else does it have Aux Tracks routed to it?
                      // Update this track's aux ref count.
                      if(r->track->auxRefCount())
                      {
                        _track->updateAuxRoute(-r->track->auxRefCount(), nullptr);
                      }
                      else if(r->track->type() == Track::AUDIO_AUX)
                      {
                        _track->updateAuxRoute(-1, nullptr);
                      }
                    }
                    break;
                    case Route::MIDI_PORT_ROUTE:
                    case Route::JACK_ROUTE:
                    case Route::MIDI_DEVICE_ROUTE:
                    break;
                  }
            }
            // Remove other tracks' input routes from this track
            rl = _track->outRoutes();
            for(ciRoute r = rl->begin(); r != rl->end(); ++r)
            {
                  switch(r->type)
                  {
                    case Route::TRACK_ROUTE: {
                      Route src(_track, r->remoteChannel, r->channels);
                      src.remoteChannel = r->channel;
                      r->track->inRoutes()->removeRoute(src);
                      flags |= SC_ROUTE;
                      // Is this track an Aux Track or else does it have Aux Tracks routed to it?
                      // Update the other track's aux ref count and all tracks it is connected to.
                      if(_track->auxRefCount())
                      {
                        r->track->updateAuxRoute(-_track->auxRefCount(), nullptr);
                      }
                      else if(_track->type() == Track::AUDIO_AUX)
                      {
                        r->track->updateAuxRoute(-1, nullptr);
                      }
                    }
                    break;
                    case Route::MIDI_PORT_ROUTE:
                    case Route::JACK_ROUTE:
                    case Route::MIDI_DEVICE_ROUTE:
                    break;
                  }
            }      
      }
    }
    break;
    
    case MoveTrack:
    {
      DEBUG_OPERATIONS(stderr, "PendingOperationItem::executeRTStage MoveTrack from:%d to:%d\n", _from_idx, _to_idx);
      const int sz = _track_list->size();
      if(_from_idx >= sz)
      {
        ERROR_OPERATIONS(stderr, "MusE error: PendingOperationItem::executeRTStage MoveTrack from index out of range:%d\n", _from_idx);
        return flags;
      }
      ciTrack fromIt = _track_list->cbegin() + _from_idx;
      Track* track = *fromIt;
      _track_list->erase(fromIt);
      ciTrack toIt = (_to_idx >= sz) ? _track_list->cend() : _track_list->cbegin() + _to_idx;
      _track_list->insert(toIt, track);
      flags |= SC_TRACK_MOVED;
    }
    break;
    
    case ModifyTrackName:
      DEBUG_OPERATIONS(stderr, "PendingOperationItem::executeRTStage ModifyTrackName track:%p new_val:%s\n", _track, _name->toLocal8Bit().data());
      _track->setName(*_name);
      flags |= (SC_TRACK_MODIFIED | SC_MIDI_TRACK_PROP);
      // If it's an aux track, notify aux UI controls to reload, or change their names etc.
      if(_track->type() == Track::AUDIO_AUX)
        flags |= SC_AUX;
    break;
    
    case SetTrackRecord:
    {
      DEBUG_OPERATIONS(stderr, "PendingOperationItem::executeRTStage SetTrackRecord track:%p new_val:%d\n", _track, _boolA);
      const bool mon = _track->setRecordFlag2AndCheckMonitor(_boolA);
      flags |= SC_RECFLAG;
      if(mon)
        flags |= SC_TRACK_REC_MONITOR;
    }
    break;
    
    case SetTrackMute:
      DEBUG_OPERATIONS(stderr, "PendingOperationItem::executeRTStage SetTrackMute track:%p new_val:%d\n", _track, _boolA);
      _track->setMute(_boolA);
      flags |= SC_MUTE;
    break;
    
    case SetTrackSolo:
      DEBUG_OPERATIONS(stderr, "PendingOperationItem::executeRTStage SetTrackSolo track:%p new_val:%d\n", _track, _boolA);
      _track->setSolo(_boolA);
      flags |= SC_SOLO;
    break;
    
    case SetTrackRecMonitor:
#ifdef _PENDING_OPS_DEBUG_
      fprintf(stderr, "PendingOperationItem::executeRTStage SetTrackRecMonitor track:%p new_val:%d\n", _track, _boolA);
#endif      
      _track->setRecMonitor(_boolA);
      flags |= SC_TRACK_REC_MONITOR;
    break;
    
    case SetTrackOff:
#ifdef _PENDING_OPS_DEBUG_
      fprintf(stderr, "PendingOperationItem::executeRTStage SetTrackOff track:%p new_val:%d\n", _track, _boolA);
#endif      
      _track->setOff(_boolA);
      flags |= SC_MUTE;
    break;
    
    
    case AddPart:
      DEBUG_OPERATIONS(stderr, "PendingOperationItem::executeRTStage AddPart part:%p\n", _part);
      _part_list->add(_part);
      _part->rechainClone();
      flags |= SC_PART_INSERTED;
      // If the part has events, then treat it as if they were inserted with separate AddEvent operations.
      // Even if some will be inserted later in this operations group with actual separate AddEvent operations,
      //  that's an SC_EVENT_INSERTED anyway, so hopefully no harm.
      if(!_part->events().empty())
        flags |= SC_EVENT_INSERTED;
    break;
    
    case DeletePart:
    {
      DEBUG_OPERATIONS(stderr, "PendingOperationItem::executeRTStage DeletePart part:%p\n", _iPart->second);
      Part* p = _iPart->second;
      _part_list->erase(_iPart);
      p->unchainClone();
      flags |= SC_PART_REMOVED;
      // If the part had events, then treat it as if they were removed with separate DeleteEvent operations.
      // Even if they will be deleted later in this operations group with actual separate DeleteEvent operations,
      //  that's an SC_EVENT_REMOVED anyway, so hopefully no harm. This fixes a problem with midi controller canvas
      //  not updating after such a 'delete part with events, no separate AddEvents were used when creating the part'.
      if(!p->events().empty())
        flags |= SC_EVENT_REMOVED;
    }
    break;

    case ModifyPartStart:
    {
      DEBUG_OPERATIONS(stderr, "PendingOperationItem::executeRTStage ModifyPartStart part:%p old_val:%d new_val:%u\n",
                       _iPart->second, _iPart->second->posValue(), _posLenVal);

      // Since we are modifying a part's position we must remove the part from the list
      //  and reinsert it afterwards for proper sorting (position is the sorting key).
      Part* part = _iPart->second;
      if(part)
      {
        if(part->track() && part->posValue() != _posLenVal)
        {
          // C++17: Modify key in-place without need for erasing and re-inserting or reallocation. In constant time.
          //auto n = _event_list->extract(_iev);
          auto n = part->track()->parts()->extract(_iPart);
          n.key() = _posLenVal;
          part->track()->parts()->insert(move(n));
          flags |= (SC_PART_REMOVED | SC_PART_INSERTED);
          part->setPosValue(_posLenVal);
        }
        part->setLenValue(_lenVal);

        // Was an event list supplied to be wholesale swapped?
        if(_event_list)
        {
          // Since the original event list is not an allocated pointer, there are no pointers to quickly exchange.
          // Instead use swap() which is also constant in time. Just like quickly exchanging pointers.
          // Transfers the original list back to _event_list so it can be deleted in the non-RT stage.
          (&part->nonconst_events())->swap(*_event_list);
          flags |= (SC_EVENT_MODIFIED | SC_EVENT_INSERTED | SC_EVENT_REMOVED);
        }
        flags |= SC_PART_MODIFIED;
      }
    }
    break;

    case ModifyPartLength:
    {
      DEBUG_OPERATIONS(stderr, "PendingOperationItem::executeRTStage ModifyPartLength part:%p old_val:%d new_val:%u\n",
                       _part, _part->lenValue(), _posLenVal);

      // Was an event list supplied to be wholesale swapped?
      if(_event_list)
      {
        // Since the original event list is not an allocated pointer, there are no pointers to quickly exchange.
        // Instead use swap() which is also constant in time. Just like quickly exchanging pointers.
        // Transfers the original list back to _event_list so it can be deleted in the non-RT stage.
        (&_part->nonconst_events())->swap(*_event_list);
        flags |= (SC_EVENT_MODIFIED | SC_EVENT_INSERTED | SC_EVENT_REMOVED);
      }
      _part->setLenValue(_posLenVal);
      flags |= SC_PART_MODIFIED;
    }
    break;

    case MovePart:
      DEBUG_OPERATIONS(stderr, "PendingOperationItem::executeRTStage MovePart part:%p track:%p new_pos:%u\n", _part, _track, _posLenVal);
      if(_track)
      {
        if(_part->track() && _iPart != _part->track()->parts()->end())
        {
          _part->track()->parts()->erase(_iPart);
          flags |= SC_PART_REMOVED;
        }
        _part->setTrack(_track);
        _part->setPosValue(_posLenVal);
        _track->parts()->add(_part);
        flags |= SC_PART_INSERTED;
      }
      else
      {
        //_part->setTick(_posLenVal);
        _part->setPosValue(_posLenVal);
      }
      flags |= SC_PART_MODIFIED;
    break;

    case SelectPart:
#ifdef _PENDING_OPS_DEBUG_
      fprintf(stderr, "PendingOperationItem::executeRTStage SelectPart part:%p select:%u\n", _part, _boolA);
#endif      
      if(_part)
        _part->setSelected(_boolA);
      
      flags |= SC_PART_SELECTION;
    break;

    case ModifyPartName:
      DEBUG_OPERATIONS(stderr, "PendingOperationItem::executeRTStage ModifyPartName part:%p new_val:%s\n", _part, _name->toLocal8Bit().data());
      _part->setName(*_name);
      flags |= SC_PART_MODIFIED;
    break;
    
    
    case AddEvent:
#ifdef _PENDING_OPS_DEBUG_
      DEBUG_OPERATIONS(stderr, "PendingOperationItem::executeRTStage AddEvent pre:    ");
      _ev.dump();
#endif
      _part->addEvent(_ev);
#ifdef _PENDING_OPS_DEBUG_
      DEBUG_OPERATIONS(stderr, "PendingOperationItem::executeRTStage AddEvent post:   ");
      _ev.dump();
#endif
      flags |= SC_EVENT_INSERTED;
    break;
    
    case DeleteEvent:
#ifdef _PENDING_OPS_DEBUG_
      DEBUG_OPERATIONS(stderr, "PendingOperationItem::executeRTStage DeleteEvent pre:    ");
      _ev.dump();
#endif
      _part->nonconst_events().erase(_iev);
#ifdef _PENDING_OPS_DEBUG_
      DEBUG_OPERATIONS(stderr, "PendingOperationItem::executeRTStage DeleteEvent post:   ");
      _ev.dump();
#endif
      flags |= SC_EVENT_REMOVED;
    break;
    
    case ModifyEventProperties:
    {
      DEBUG_OPERATIONS(stderr, "PendingOperationItem::executeRTStage ModifyEventProperties pos:%d len:%d spos:%d",
        _posLenVal, _lenVal, _event_spos);
      Event& e = _iev->second;
      if(_event_list && e.posValue() != _posLenVal)
      {
        // C++17: Modify key in-place without need for erasing and re-inserting or reallocation. In constant time.
        auto n = _event_list->extract(_iev);
        n.key() = _posLenVal;
        _event_list->insert(move(n));
        flags |= (SC_EVENT_REMOVED | SC_EVENT_INSERTED);
        e.setPosValue(_posLenVal);
      }
      e.setLenValue(_lenVal);
      e.setSpos(_event_spos);
      flags |= SC_EVENT_MODIFIED;
    }
    break;
    
    case SelectEvent:
#ifdef _PENDING_OPS_DEBUG_
      fprintf(stderr, "PendingOperationItem::executeRTStage SelectEvent part:%p select:%d\n", _part, _intA);
#endif
      // Make sure we let song handle this important job, it selects corresponding events in clone parts.
      MusEGlobal::song->selectEvent(_ev, _part, _intA);
      flags |= SC_SELECTION;
    break;

    
    case ModifyEventList:
      DEBUG_OPERATIONS(stderr, "PendingOperationItem::executeRTStage ModifyEventList: orig eventlist:%p new eventlist:%p\n", 
                       _orig_event_list, _event_list);
      if(_orig_event_list && _event_list)
      {
        // Since the original event list is not an allocated pointer, there are no pointers to quickly exchange.
        // Instead use swap() which is also constant in time. Just like quickly exchanging pointers.
        // Transfers the original list back to _event_list so it can be deleted in the non-RT stage.
        _orig_event_list->swap(*_event_list);
        flags |= (SC_EVENT_MODIFIED | SC_EVENT_INSERTED | SC_EVENT_REMOVED);
      }
    break;
    
    case AddMidiCtrlValList:
      DEBUG_OPERATIONS(stderr, "PendingOperationItem::executeRTStage AddMidiCtrlValList: mcvll:%p mcvl:%p chan:%d\n", _mcvll, _mcvl, _intA);
      _mcvll->add(_intA, _mcvl);
      flags |= SC_MIDI_CONTROLLER_ADD;
    break;
    case ModifyMidiCtrlValList:
      DEBUG_OPERATIONS(stderr, "PendingOperationItem::executeRTStage ModifyMidiCtrlValList: orig_mcvl:%p mcvl:%p\n", _orig_mcvl, _mcvl);
      if(_orig_mcvl && _mcvl)
      {
        // Since the original list is not an allocated pointer, there are no pointers to quickly exchange.
        // Instead use swap() which is also constant in time. Just like quickly exchanging pointers.
        // Transfers the original list back to _mcvl so it can be deleted in the non-RT stage.
        _orig_mcvl->swap(*_mcvl);
        // No song changed flags are required to be set here.
      }
    break;
    case AddMidiCtrlVal:
      DEBUG_OPERATIONS(stderr, "PendingOperationItem::executeRTStage AddMidiCtrlVal: mcvl:%p part:%p tick:%u val:%d\n", _mcvl, _part, _posLenVal, _intB);
      // Do not attempt to add cached events which are outside of the part.
      // But do allow muted parts, and muted tracks, and 'off' tracks. Otherwise adding values
      //  to muted parts fails to add them when unmuted. The cache mechanism catches this anyways.
#ifdef ALLOW_LEFT_HIDDEN_EVENTS
      if((int)_posLenVal >= (int)_part->posValue() &&
         (int)_posLenVal < (int)_part->posValue() + (int)_part->lenValue())
#else
      if(_posLenVal >= _part->posValue() &&
         _posLenVal < _part->posValue() + _part->lenValue())
#endif
         // FIXME FINDMICHJETZT XTicks!!
        _mcvl->insert(MidiCtrlValListInsertPair_t(_posLenVal, MidiCtrlVal(_part, _intB)));
      // No song changed flags are required to be set here.
    break;
    case DeleteMidiCtrlVal:
      DEBUG_OPERATIONS(stderr, "PendingOperationItem::executeRTStage DeleteMidiCtrlVal: mcvl:%p tick:%u part:%p val:%d\n", 
                       _mcvl, _imcv->first, _imcv->second.part, _imcv->second.val);
      _mcvl->erase(_imcv);
      // No song changed flags are required to be set here.
    break;
    case ModifyMidiCtrlVal:
      DEBUG_OPERATIONS(stderr,
        "PendingOperationItem::executeRTStage ModifyMidiCtrlVal: part:%p old_time:%d new_time:%d old_val:%d new_val:%d\n", 
                       _imcv->second.part, _imcv->first, _posLenVal, _imcv->second.val, _intB);
      _imcv->second.val = _intB;
      // Do we want to change the position of the item?
      if(_imcv->first != _posLenVal)
      {
        // C++17: Modify key in-place without need for erasing and re-inserting or reallocation. In constant time.
        auto n = _mcvl->extract(_imcv);
        n.key() = _posLenVal;
        _mcvl->insert(move(n));
      }
    break;
    
    case ModifyAudioCtrlValListList:
    {
      DEBUG_OPERATIONS(stderr, "PendingOperationItem::executeRTStage ModifyAudioCtrlValListList: dst ctrll:%p src ctrll:%p\n",
                       _aud_ctrl_list_list, _src_aud_ctrl_list_list);
      if(_aud_ctrl_list_list && _src_aud_ctrl_list_list)
      {
        // Transfers the original list back to _aud_ctrl_list_list so it can be deleted in the non-RT stage.
        _aud_ctrl_list_list->swap(*_src_aud_ctrl_list_list);
      }
      flags |= SC_AUDIO_CONTROLLER_LIST;
    }
    break;

    case AddAudioCtrlValList:
      DEBUG_OPERATIONS(stderr, "PendingOperationItem::executeRTStage AddAudioCtrlValList: ctrl_ll:%p ctrl_l:%p\n",
        _aud_ctrl_list_list, _aud_ctrl_list);
      _aud_ctrl_list_list->add(_aud_ctrl_list);
      flags |= SC_AUDIO_CONTROLLER_LIST;
    break;
    case DeleteAudioCtrlValList:
      DEBUG_OPERATIONS(stderr, "PendingOperationItem::executeRTStage DeleteAudioCtrlValList: ctrl_ll:%p ctrl_l:%p\n",
        _aud_ctrl_list_list, _iCtrlList->second);
      // Transfer the original pointer back to _aud_ctrl_list so it can be deleted in the non-RT stage.
      _aud_ctrl_list = _iCtrlList->second;
      _aud_ctrl_list_list->erase(_iCtrlList);
      flags |= SC_AUDIO_CONTROLLER_LIST;
    break;

    case ModifyAudioCtrlValList:
    {
      DEBUG_OPERATIONS(stderr, "PendingOperationItem::executeRTStage ModifyAudioCtrlValList: old ctrl_l:%p new ctrl_l:%p\n", _iCtrlList->second, _aud_ctrl_list);
      if(_iCtrlList->second && _aud_ctrl_list)
      {
        // Transfers the original list back to _aud_ctrl_list so it can be deleted in the non-RT stage.
        _iCtrlList->second->swap(*_aud_ctrl_list);
      }
      flags |= SC_AUDIO_CONTROLLER_LIST;
    }
    break;
    case AddAudioCtrlVal:
      DEBUG_OPERATIONS(stderr, "PendingOperationItem::executeRTStage AddAudioCtrlVal: ctrl_l:%p frame:%u val:%f ctl_flags:%d\n",
              _aud_ctrl_list, _posLenVal, _ctl_dbl_val, _ctl_flags);
      // Add will replace if found.
      _aud_ctrl_list->add(_posLenVal, _ctl_dbl_val, _ctl_flags);
      flags |= SC_AUDIO_CONTROLLER;
    break;
    case AddAudioCtrlValStruct:
      DEBUG_OPERATIONS(stderr, "PendingOperationItem::executeRTStage AddAudioCtrlValStruct: ctrl_l:%p frame:%u val:%f selected:%d\n",
              _aud_ctrl_list, _posLenVal, _audCtrlValStruct->value(), _audCtrlValStruct->selected());
      // Add will replace if found.
      _aud_ctrl_list->add(_posLenVal, *_audCtrlValStruct);
      flags |= SC_AUDIO_CONTROLLER;
    break;
    case DeleteAudioCtrlVal:
      DEBUG_OPERATIONS(stderr, "PendingOperationItem::executeRTStage DeleteAudioCtrlVal: ctrl_l:%p ctrl_num:%d frame:%d val:%f\n", 
                       _aud_ctrl_list, _aud_ctrl_list->id(), _iCtrl->first, _iCtrl->second.value());
      _aud_ctrl_list->erase(_iCtrl);
      flags |= SC_AUDIO_CONTROLLER;
    break;
    case ModifyAudioCtrlVal:
      DEBUG_OPERATIONS(stderr, "PendingOperationItem::executeRTStage ModifyAudioCtrlVal: frame:%u old_val:%f new_val:%f\n", 
                       _iCtrl->first, _iCtrl->second.value(), _ctl_dbl_val);
      // If the frame is the same, just change the value.
      if(_iCtrl->first == _posLenVal)
      {
        _iCtrl->second.setValue(_ctl_dbl_val);
      }
      // Otherwise erase + add is required.
      else
      {
        // Get the old value.
        CtrlVal new_cv(_iCtrl->second);
        // Change only the value member.
        new_cv.setValue(_ctl_dbl_val);
        // Erase and re-insert. TODO TODO: Use C++17 nodes extract(). I think already done in my other branch.
        _aud_ctrl_list->erase(_iCtrl);
        _aud_ctrl_list->insert(CtrlListInsertPair_t(_posLenVal, new_cv));
      }
      flags |= SC_AUDIO_CONTROLLER;
    break;
    case SelectAudioCtrlVal:
      DEBUG_OPERATIONS(stderr, "PendingOperationItem::executeRTStage SelectAudioCtrlVal: ctrl_v:%p selected:%d\n",
              _audCtrlValStruct, _selected);
      _audCtrlValStruct->setSelected(_selected);
      flags |= SC_AUDIO_CONTROLLER_SELECTION;
    break;
    case SetAudioCtrlPasteEraseMode:
      DEBUG_OPERATIONS(stderr, "PendingOperationItem::executeRTStage SetAudioCtrlPasteEraseMode: opts:%d\n",
              _audio_ctrl_paste_erase_opts);
      MusEGlobal::config.audioCtrlGraphPasteEraseOptions = _audio_ctrl_paste_erase_opts;
      flags |= SC_AUDIO_CTRL_PASTE_ERASE_MODE;
    break;

    
    case ModifyTempoList:
      DEBUG_OPERATIONS(stderr, "PendingOperationItem::executeRTStage ModifyTempoList: orig tempolist:%p new tempolist:%p\n", 
                       _orig_tempo_list, _tempo_list);
      if(_orig_tempo_list && _tempo_list)
      {
        // Since the original tempo list is not an allocated pointer, there are no pointers to quickly exchange.
        // Instead use swap() which is also constant in time. Just like quickly exchanging pointers.
        // Transfers the original list back to _tempo_list so it can be deleted in the non-RT stage.
        _orig_tempo_list->swap(*_tempo_list);
        flags |= SC_TEMPO;
      }
    break;
    
    
    case SetStaticTempo:
#ifdef _PENDING_OPS_DEBUG_
      fprintf(stderr, "PendingOperationItem::executeRTStage SetStaticTempo: tempolist:%p new_tempo:%d\n", _tempo_list, _intA);
#endif      
      _tempo_list->setStaticTempo(_intA);
      flags |= SC_TEMPO;
    break;
    
    case SetGlobalTempo:
      DEBUG_OPERATIONS(stderr, "PendingOperationItem::executeRTStage SetGlobalTempo: tempolist:%p new_tempo:%d\n", _tempo_list, _intA);
      _tempo_list->setGlobalTempo(_intA);
      flags |= SC_TEMPO;
    break;

    
    case ModifySigList:
      DEBUG_OPERATIONS(stderr, "PendingOperationItem::executeRTStage ModifySigList: orig siglist:%p new siglist:%p\n", 
                       _orig_sig_list, _sig_list);
      if(_orig_sig_list && _sig_list)
      {
        // Since the original sig list is not an allocated pointer, there are no pointers to quickly exchange.
        // Instead use swap() which is also constant in time. Just like quickly exchanging pointers.
        // Transfers the original list back to _sig_list so it can be deleted in the non-RT stage.
        _orig_sig_list->swap(*_sig_list);
        flags |= SC_SIG;
      }
    break;
    
    
    
    case ModifyKeyList:
      DEBUG_OPERATIONS(stderr, "PendingOperationItem::executeRTStage ModifyKeyList: orig keylist:%p new keylist:%p\n", 
                       _orig_key_list, _key_list);
      if(_orig_key_list && _key_list)
      {
        // Since the original key list is not an allocated pointer, there are no pointers to quickly exchange.
        // Instead use swap() which is also constant in time. Just like quickly exchanging pointers.
        // Transfers the original list back to _sig_list so it can be deleted in the non-RT stage.
        _orig_key_list->swap(*_key_list);
        flags |= SC_KEY;
      }
    break;

    
    case ModifyStretchListRatio:
      DEBUG_OPERATIONS(stderr, "PendingOperationItem::executeRTStage ModifyStretchListRatio: stretchType:%d stretchlist:%p new_ratio:%f\n", 
                       _stretch_type, _stretch_list, _audio_converter_value);
      // Defer normalize until end of stage 2.
      _stretch_list->setRatio(StretchListItem::StretchEventType(_stretch_type), _audio_converter_value, false);
      flags |= SC_AUDIO_STRETCH;
    break;
    
    case AddStretchListRatioAt:
      DEBUG_OPERATIONS(stderr, "PendingOperationItem::executeRTStage AddStretchListRatioAt: stretchType:%d stretchlist:%p ratio:%f frame:%ld\n", 
                       _stretch_type, _stretch_list, _audio_converter_value, _museFrame);
      // Defer normalize until end of stage 2.
      _stretch_list->addRatioAt(StretchListItem::StretchEventType(_stretch_type), _museFrame, _audio_converter_value, false);
      
      flags |= SC_AUDIO_STRETCH;
    break;
    
    case DeleteStretchListRatioAt:
      DEBUG_OPERATIONS(stderr, "PendingOperationItem::executeRTStage DeleteStretchListRatioAt: stretchlist:%p frame:%ld types:%d\n", 
                        _stretch_list, _iStretchEvent->first, _stretch_type);
      // Defer normalize until end of stage 2.
      _stretch_list->del(_stretch_type, _iStretchEvent, false);
      flags |= SC_AUDIO_STRETCH;
    break;
    
    case ModifyStretchListRatioAt:
      DEBUG_OPERATIONS(stderr, "PendingOperationItem::executeRTStage ModifyStretchListRatioAt: "
                               "stretchType:%d stretchlist:%p frame:%ld new_frame:%ld new_ratio:%f\n", 
                       _stretch_type, _stretch_list, _iStretchEvent->first, _museFrame, _audio_converter_value);
      
      // If the frame is the same, just change the value.
      if(_iStretchEvent->first == _museFrame)
        // Defer normalize until end of stage 2.
        _stretch_list->setRatioAt(StretchListItem::StretchEventType(_stretch_type), _iStretchEvent, _audio_converter_value, false);
      // Otherwise erase + add is required.
      else
      {
        // Defer normalize until end of stage 2.
        _stretch_list->del(_stretch_type, _iStretchEvent, false);
        _stretch_list->add(StretchListItem::StretchEventType(_stretch_type), _museFrame, _audio_converter_value, false);
      }
      
      flags |= SC_AUDIO_STRETCH;
    break;
    
    
    case ModifySongLength:
      DEBUG_OPERATIONS(stderr, "PendingOperationItem::executeRTStage ModifySongLength: len:%d\n", _posLenVal);
      MusEGlobal::song->setLen(_posLenVal, false); // false = Do not emit update signals here !
      flags |= SC_EVERYTHING;
    break;
    
    case EnableAllAudioControllers:
    {
      DEBUG_OPERATIONS(stderr, "PendingOperationItem::executeRTStage EnableAllAudioControllers\n");
      TrackList* tl = MusEGlobal::song->tracks();
      for (iTrack it = tl->begin(); it != tl->end(); ++it)
      {
        Track* t = *it;
        if(t->isMidiTrack())
          continue;
        AudioTrack *at = static_cast<AudioTrack*>(t);
        // Re-enable all track and plugin controllers, and synth controllers if applicable.
        at->enableAllControllers();
        flags |= SC_AUDIO_CONTROLLER;
      }
    }
    break;
    
    case GlobalSelectAllEvents:
    {
#ifdef _PENDING_OPS_DEBUG_
      fprintf(stderr, "PendingOperationItem::executeRTStage GlobalSelectAllEvents\n");
#endif
      for (iTrack it = _track_list->begin(); it != _track_list->end(); ++it)
      {
        //Track* t = *it;
        //if(t->isMidiTrack())
        //  continue;
        if((*it)->selectEvents(_boolA))
          flags |= SC_SELECTION;
      }
    }
    break;
    
    case ModifyAudioSamples:
    {
#ifdef _PENDING_OPS_DEBUG_
      fprintf(stderr, "PendingOperationItem::executeRTStage ModifyAudioSamples: "
                      "audioSamplesPointer:%p newAudioSamples:%p audioSamplesLen:%p newAudioSamplesLen:%d\n",
              _audioSamplesPointer, _newAudioSamples, _audioSamplesLen, _newAudioSamplesLen);
#endif      
      if(_audioSamplesPointer)
      {
        float* orig = *_audioSamplesPointer;
        *_audioSamplesPointer = _newAudioSamples;
        // Transfer the original pointer back to _audioSamplesPointer so it can be deleted in the non-RT stage.
        _newAudioSamples = orig;
      }
      
      if(_audioSamplesLen)
        *_audioSamplesLen = _newAudioSamplesLen;
      
      // Currently no flags for this.
      //flags |= SC_;
    }
    break;

    case ModifyMarkerList:
    {
#ifdef _PENDING_OPS_DEBUG_
      fprintf(stderr, "PendingOperationItem::executeRTStage ModifyMarkerList: "
                      "orig list:%p new list:%p\n", _orig_marker_list, _marker_list);
#endif      
      if(_orig_marker_list && _marker_list)
      {
        MarkerList* orig = *_orig_marker_list;
        *_orig_marker_list = _marker_list;
        // Transfer the original pointer back to _marker_list so it can be deleted in the non-RT stage.
        _marker_list = orig;
      }
      // Currently no flags for this.
      //flags |= SC_MARKERS_REBUILT;
    }
    break;
    
    case SwitchMetronomeSettings:
    {
#ifdef _PENDING_OPS_DEBUG_
      fprintf(stderr, "PendingOperationItem::executeRTStage SwitchMetronomeSettings: settings:%p val:%d\n", _bool_pointer, _boolA);
#endif      
      *_bool_pointer = _boolA;
      flags |= SC_METRONOME;
    }
    break;

    case ModifyMetronomeAccentMap:
    {
#ifdef _PENDING_OPS_DEBUG_
      fprintf(stderr, "PendingOperationItem::executeRTStage ModifyMetronomeAccentMap: old map:%p new map:%p\n", _metroAccentsMap, _newMetroAccentsMap);
#endif      
      MetroAccentsMap* orig = *_metroAccentsMap;
      *_metroAccentsMap = _newMetroAccentsMap;
      // Transfer the original pointer back to _newMetroAccentsMap so it can be deleted in the non-RT stage.
      _newMetroAccentsMap = orig;
      flags |= SC_METRONOME;
    }
    break;

    case SetExternalSyncFlag:
    {
#ifdef _PENDING_OPS_DEBUG_
      fprintf(stderr, "PendingOperationItem::executeRTStage SetExternalSyncFlag: pointer:%p val:%d\n", _bool_pointer, _boolA);
#endif      
      *_bool_pointer = _boolA;
      flags |= SC_EXTERNAL_MIDI_SYNC;
    }
    break;

    case SwitchMidiRemoteSettings:
    {
#ifdef _PENDING_OPS_DEBUG_
      fprintf(stderr, "PendingOperationItem::executeRTStage SwitchMidiRemoteSettings: settings:%p val:%d\n", _bool_pointer, _boolA);
#endif
      *_bool_pointer = _boolA;
      flags |= SC_MIDI_REMOTE;
    }
    break;

    case ModifyMidiRemote:
    {
#ifdef _PENDING_OPS_DEBUG_
      fprintf(stderr, "PendingOperationItem::executeRTStage ModifyMidiRemote: old map:%p new map:%p\n", _midiRemote, _newMidiRemote);
#endif
      // (Don't forget to delete the new pointer in the non-RT stage.)
      *_midiRemote = *_newMidiRemote;
      flags |= SC_MIDI_REMOTE;
    }
    break;

    case SetUseJackTransport:
    {
#ifdef _PENDING_OPS_DEBUG_
      fprintf(stderr, "PendingOperationItem::executeRTStage SetUseJackTransport: pointer:%p val:%d\n", _bool_pointer, _boolA);
#endif      
      *_bool_pointer = _boolA;
      flags |= SC_USE_JACK_TRANSPORT;
    }
    break;

    case SetUseMasterTrack:
    {
#ifdef _PENDING_OPS_DEBUG_
      fprintf(stderr, "PendingOperationItem::executeRTStage SetUseMasterTrack: pointer:%p val:%d\n", _tempo_list, _boolA);
#endif      
      _tempo_list->setMasterFlag(0, _boolA);
      flags |= SC_MASTER;
    }
    break;

    case Uninitialized:
    break;
    
    default:
      ERROR_OPERATIONS(stderr, "PendingOperationItem::executeRTStage unknown type %d\n", _type);
    break;
  }
  return flags;
}

SongChangedStruct_t PendingOperationItem::executeNonRTStage()
{
  SongChangedStruct_t flags = 0;
  switch(_type)
  {
    case ModifyPartStart:
    case ModifyPartLength:
      // At this point _event_list contains all the items that were in the original event list, via swap(). Delete it now.
      if(_event_list)
        delete _event_list;
    break;

    case ModifyEventList:
      // At this point _event_list contains all the items that were in the original event list, via swap(). Delete it now.
      if(_event_list)
        delete _event_list;
    break;

    case ModifyMidiCtrlValList:
      // At this point _mcvl contains all the items that were in the original event list, via swap(). Delete it now.
      if(_mcvl)
        delete _mcvl;
    break;

    case AddRoute:
      if(MusEGlobal::song->connectJackRoutes(_src_route, _dst_route))
        flags |= SC_ROUTE;
    break;
    
    case DeleteRoute:
      if(MusEGlobal::song->connectJackRoutes(_src_route, _dst_route, true))
        flags |= SC_ROUTE;
    break;

    case ModifyTempoList:
      // At this point _tempo_list contains all the items that were in the original tempo list, via swap(). Delete it now.
      if(_tempo_list)
        delete _tempo_list;
    break;

    
    case ModifySigList:
      // At this point _sig_list contains all the items that were in the original sig list, via swap(). Delete it now.
      if(_sig_list)
        delete _sig_list;
    break;


    case ModifyKeyList:
      // At this point _key_list contains all the items that were in the original key list, via swap(). Delete it now.
      if(_key_list)
        delete _key_list;
    break;

    
    case ModifyLocalAudioConverterSettings:
      // At this point these are the original pointers that were replaced. Delete the original objects now.
      if(_audio_converter_settings)
        delete _audio_converter_settings;
    break;

    case ModifyLocalAudioConverter:
      // At this point these are the original pointers that were replaced. Delete the original objects now.
      if(_audio_converter)
        delete _audio_converter;
      if(_audio_converter_ui)
        delete _audio_converter_ui;
    break;

    case SetAudioConverterOfflineMode:
      // At this point this is the original pointer that were replaced. Delete the original object now.
      if(_audio_converter)
        delete _audio_converter;
    break;

    case ModifyDefaultAudioConverterSettings:
      // At this point this is the original pointer that was replaced. Delete the original object now.
      if(_audio_converter_settings)
        delete _audio_converter_settings;
    break;
    
    case ReplaceMidiInstrument:
      // At this point _midi_instrument is the original instrument that was replaced. Delete it now.
      if(_midi_instrument)
        delete _midi_instrument;
    break;

    case ModifyAudioCtrlValListList:
      // At this point _src_aud_ctrl_list_list contains all the items that were in the original list, via swap().
      // Delete it now. Be sure to call clearDelete() because the list destructor does not do it.
      if(_src_aud_ctrl_list_list)
      {
        _src_aud_ctrl_list_list->clearDelete();
        delete _src_aud_ctrl_list_list;
      }
    break;

    case DeleteAudioCtrlValList:
      // At this point _aud_ctrl_list contains the original list. Delete it now.
      if(_aud_ctrl_list)
        delete _aud_ctrl_list;
    break;

    case ModifyAudioCtrlValList:
      // At this point _aud_ctrl_list contains all the items that were in the original list, via swap(). Delete it now.
      if(_aud_ctrl_list)
        delete _aud_ctrl_list;
    break;

    case UpdateAllAudioCtrlGroups:
    {
      DEBUG_OPERATIONS(stderr, "PendingOperationItem::executeRTStage UpdateAllAudioCtrlGroups\n");
      TrackList* tl = MusEGlobal::song->tracks();
      for (iTrack it = tl->begin(); it != tl->end(); ++it)
      {
        Track* t = *it;
        if(t->isMidiTrack())
          continue;
        AudioTrack *at = static_cast<AudioTrack*>(t);
        at->controller()->updateGroups();
        flags |= SC_AUDIO_CONTROLLER;
      }
    }
    break;

    case UpdateAudioCtrlListGroups:
    {
      DEBUG_OPERATIONS(stderr, "PendingOperationItem::executeRTStage UpdateAudioCtrlListGroups\n");
      _aud_ctrl_list_list->updateGroups();
    }
    break;

    case UpdateAudioCtrlGroups:
    {
      DEBUG_OPERATIONS(stderr, "PendingOperationItem::executeRTStage UpdateAudioCtrlGroups\n");
      _aud_ctrl_list->updateGroups();
    }
    break;

    case UpdateAudioCtrlPosGroups:
    {
      DEBUG_OPERATIONS(stderr, "PendingOperationItem::executeRTStage UpdateAudioCtrlPosGroups\n");
      _aud_ctrl_list->updateGroups(_iCtrl);
    }
    break;

    case ModifyTrackDrumMapItem:
      // Discard the operation, it has already completed.
      if(_drum_map_track_operation)
        delete _drum_map_track_operation;
    break;

    case ReplaceTrackDrumMapPatchList:
      // Discard the operation, it has already completed.
      if(_drum_map_track_patch_operation)
      {
        // At this point _workingItemPatchList is the original list that was replaced. Delete it now.
        if(_drum_map_track_patch_replace_operation->_workingItemPatchList)
          delete _drum_map_track_patch_replace_operation->_workingItemPatchList;

        delete _drum_map_track_patch_replace_operation;
      }
    break;

    case RemapDrumControllers:
      // Discard the operation, it has already completed.
      if(_midi_ctrl_val_remap_operation)
      {
        // At this point _midiCtrlValLists2bDeleted contains the original lists that were replaced. Delete them now.
        for(iMidiCtrlValLists2bDeleted_t imvld = _midi_ctrl_val_remap_operation->_midiCtrlValLists2bDeleted.begin();
            imvld != _midi_ctrl_val_remap_operation->_midiCtrlValLists2bDeleted.end(); ++imvld)
          delete *imvld;

        delete _midi_ctrl_val_remap_operation;
      }
    break;

    case ModifyAudioSamples:
      // At this point _newAudioSamples points to the original memory that was replaced. Delete it now.
      if(_newAudioSamples)
        delete _newAudioSamples;
    break;

    case ModifyMarkerList:
      // At this point _marker_list points to the original memory that was replaced. Delete it now.
      if(_marker_list)
        delete _marker_list;
    break;

    case ModifyMetronomeAccentMap:
      // At this point _newMetroAccentsMap is the original list that was replaced. Delete it now.
      if(_newMetroAccentsMap)
        delete _newMetroAccentsMap;
    break;

    case ModifyMidiRemote:
      // Done with _newMidiRemote. Delete it now.
      if(_newMidiRemote)
        delete _newMidiRemote;
    break;

    default:
    break;
  }
  return flags;
}

SongChangedStruct_t PendingOperationList::executeRTStage()
{
  DEBUG_OPERATIONS(stderr, "PendingOperationList::executeRTStage executing...\n");
  for(iPendingOperation ip = begin(); ip != end(); ++ip)
    _sc_flags |= ip->executeRTStage();
  
  // To avoid doing this item by item, do it here.
  if(_sc_flags & (SC_TRACK_INSERTED | SC_TRACK_REMOVED | SC_ROUTE))
  {
    MusEGlobal::song->updateSoloStates();
    _sc_flags |= SC_SOLO;
  } 
  
  // To avoid doing this item by item, do it here.
  StretchList* sl;
  for(iPendingOperation ip = begin(); ip != end(); ++ip)
  {
    const PendingOperationItem& poi = *ip;
    switch(poi._type)
    {
      //case PendingOperationItem::AddSamplerateRatioAt:
      //case PendingOperationItem::DeleteSamplerateRatioAt:
      //case PendingOperationItem::ModifySamplerateRatioAt:
      case PendingOperationItem::AddStretchListRatioAt:
      case PendingOperationItem::DeleteStretchListRatioAt:
      case PendingOperationItem::ModifyStretchListRatioAt:
      case PendingOperationItem::ModifyStretchListRatio:
        sl = poi._stretch_list;
        if(sl && !sl->isNormalized())
        {
          sl->normalizeListFrames();
          _sc_flags |= SC_AUDIO_STRETCH;
        }
      break;
      
      default:
      break;
    }
  }

  return _sc_flags;
}

SongChangedStruct_t PendingOperationList::executeNonRTStage()
{
  DEBUG_OPERATIONS(stderr, "PendingOperationList::executeNonRTStage executing...\n");
  for(iPendingOperation ip = begin(); ip != end(); ++ip)
    _sc_flags |= ip->executeNonRTStage();
  return _sc_flags;
}

void PendingOperationList::clear()
{
  _sc_flags = 0;
  _map.clear();
  std::list<PendingOperationItem>::clear();  
  DEBUG_OPERATIONS(stderr, "PendingOperationList::clear * post map size:%d list size:%d\n", (int)_map.size(), (int)size());
}

bool PendingOperationList::add(PendingOperationItem op)
{
  unsigned int t = op.getIndex();

  switch(op._type)
  {
    // For these special allocation ops, searching has already been done before hand. Just add them.
    case PendingOperationItem::AddMidiCtrlValList:
    {
      iPendingOperation iipo = insert(end(), op);
      _map.insert(std::pair<unsigned int, iPendingOperation>(t, iipo));
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
      case PendingOperationItem::ModifyDefaultAudioConverterSettings:
        if(poi._type == PendingOperationItem::ModifyDefaultAudioConverterSettings && 
           poi._audio_converter_settings == op._audio_converter_settings)
        {
          ERROR_OPERATIONS(stderr, "MusE error: PendingOperationList::add(): Double ModifyDefaultAudioConverterSettings. Ignoring.\n");
          return false;  
        }
      break;
    
      case PendingOperationItem::ModifyLocalAudioConverterSettings:
        if(poi._type == PendingOperationItem::ModifyLocalAudioConverterSettings && poi._sndFileR == op._sndFileR &&
           poi._audio_converter_settings == op._audio_converter_settings)
        {
          ERROR_OPERATIONS(stderr, "MusE error: PendingOperationList::add(): Double ModifyLocalAudioConverterSettings. Ignoring.\n");
          return false;  
        }
      break;
    
      case PendingOperationItem::ModifyLocalAudioConverter:
        if(poi._type == PendingOperationItem::ModifyLocalAudioConverter && poi._sndFileR == op._sndFileR &&
           poi._audio_converter == op._audio_converter &&
           poi._audio_converter_ui == op._audio_converter_ui)
        {
          ERROR_OPERATIONS(stderr, "MusE error: PendingOperationList::add(): Double ModifyLocalAudioConverter. Ignoring.\n");
          return false;  
        }
      break;
    
      case PendingOperationItem::SetAudioConverterOfflineMode:
        if(poi._type == PendingOperationItem::SetAudioConverterOfflineMode && poi._sndFileR == op._sndFileR &&
           poi._audio_converter == op._audio_converter)
        {
          ERROR_OPERATIONS(stderr, "MusE error: PendingOperationList::add(): Double SetAudioConverterOfflineMode. Ignoring.\n");
          return false;  
        }
      break;


      case PendingOperationItem::ModifyTrackDrumMapItem:
        if(poi._type == PendingOperationItem::ModifyTrackDrumMapItem &&
           poi._drum_map_track_operation == op._drum_map_track_operation)
        {
          fprintf(stderr, "MusE error: PendingOperationList::add(): Double ModifyTrackDrumMapItem. Ignoring.\n");
          return false;
        }
      break;

      case PendingOperationItem::ReplaceTrackDrumMapPatchList:
        if(poi._type == PendingOperationItem::ReplaceTrackDrumMapPatchList &&
           poi._drum_map_track_patch_replace_operation == op._drum_map_track_patch_replace_operation)
        {
          fprintf(stderr, "MusE error: PendingOperationList::add(): Double ReplaceTrackDrumMapPatchList. Ignoring.\n");
          return false;
        }
      break;

      case PendingOperationItem::RemapDrumControllers:
        if(poi._type == PendingOperationItem::RemapDrumControllers &&
           poi._midi_ctrl_val_remap_operation == op._midi_ctrl_val_remap_operation)
        {
          fprintf(stderr, "MusE error: PendingOperationList::add(): Double RemapDrumControllers. Ignoring.\n");
          return false;
        }
      break;

      case PendingOperationItem::UpdateDrumMaps:
        if(poi._type == PendingOperationItem::UpdateDrumMaps && poi._midi_port == op._midi_port)
        {
          fprintf(stderr, "MusE error: PendingOperationList::add(): Double UpdateDrumMaps. Ignoring.\n");
          return false;  
        }
      break;
      
      case PendingOperationItem::UpdateSoloStates:
        if(poi._type == PendingOperationItem::UpdateSoloStates && poi._track_list == op._track_list)
        {
          ERROR_OPERATIONS(stderr, "MusE error: PendingOperationList::add(): Double UpdateSoloStates. Ignoring.\n");
          return false;  
        }
      break;
      
      case PendingOperationItem::AddRoute:
        if(poi._type == PendingOperationItem::AddRoute && poi._src_route == op._src_route && poi._dst_route == op._dst_route)
        {
          ERROR_OPERATIONS(stderr, "MusE error: PendingOperationList::add(): Double AddRoute. Ignoring.\n");
          return false;  
        }
      break;
      
      case PendingOperationItem::DeleteRoute:
        if(poi._type == PendingOperationItem::DeleteRoute && poi._src_route == op._src_route && poi._dst_route == op._dst_route)
        {
          ERROR_OPERATIONS(stderr, "MusE error: PendingOperationList::add(): Double DeleteRoute. Ignoring.\n");
          return false;  
        }
      break;
      
      case PendingOperationItem::AddRouteNode:
        if(poi._type == PendingOperationItem::AddRouteNode && poi._route_list == op._route_list && poi._src_route == op._src_route)
        {
          ERROR_OPERATIONS(stderr, "MusE error: PendingOperationList::add(): Double AddRouteNode. Ignoring.\n");
          return false;  
        }
      break;

      case PendingOperationItem::DeleteRouteNode:
        if(poi._type == PendingOperationItem::DeleteRouteNode && poi._route_list == op._route_list && poi._iRoute == op._iRoute)
        {
          ERROR_OPERATIONS(stderr, "MusE error: PendingOperationList::add(): Double DeleteRouteNode. Ignoring.\n");
          return false;  
        }
      break;

      case PendingOperationItem::ModifyRouteNode:
        if(poi._type == PendingOperationItem::ModifyRouteNode && poi._src_route == op._src_route && poi._dst_route_pointer == op._dst_route_pointer)
        {
          ERROR_OPERATIONS(stderr, "MusE error: PendingOperationList::add(): Double ModifyRouteNode. Ignoring.\n");
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
          ERROR_OPERATIONS(stderr, "MusE error: PendingOperationList::add(): Double AddMidiInstrument. Ignoring.\n");
          return false;  
        }
      break;
        
      case PendingOperationItem::DeleteMidiInstrument:
        if(poi._type == PendingOperationItem::DeleteMidiInstrument && poi._midi_instrument_list == op._midi_instrument_list && 
           poi._iMidiInstrument == op._iMidiInstrument)
        {
          ERROR_OPERATIONS(stderr, "MusE error: PendingOperationList::add(): Double DeleteMidiInstrument. Ignoring.\n");
          return false;  
        }
      break;
        
      case PendingOperationItem::ReplaceMidiInstrument:
        if(poi._type == PendingOperationItem::ReplaceMidiInstrument && poi._midi_instrument_list == op._midi_instrument_list &&
           (poi._midi_instrument == op._midi_instrument || poi._iMidiInstrument == op._iMidiInstrument))
        {
          fprintf(stderr, "MusE error: PendingOperationList::add(): Double ReplaceMidiInstrument. Ignoring.\n");
          return false;
        }
      break;

      case PendingOperationItem::AddMidiDevice:
        if(poi._type == PendingOperationItem::AddMidiDevice && poi._midi_device_list == op._midi_device_list && poi._midi_device == op._midi_device)  
        {
          ERROR_OPERATIONS(stderr, "MusE error: PendingOperationList::add(): Double AddMidiDevice. Ignoring.\n");
          return false;  
        }
      break;
        
      case PendingOperationItem::DeleteMidiDevice:
        if(poi._type == PendingOperationItem::DeleteMidiDevice && poi._midi_device_list == op._midi_device_list && poi._iMidiDevice == op._iMidiDevice)
        {
          ERROR_OPERATIONS(stderr, "MusE error: PendingOperationList::add(): Double DeleteMidiDevice. Ignoring.\n");
          return false;  
        }
      break;
        
      case PendingOperationItem::ModifyMidiDeviceAddress:
        if(poi._type == PendingOperationItem::ModifyMidiDeviceAddress && poi._midi_device == op._midi_device &&
           poi._address_client == op._address_client && poi._address_port == op._address_port)
        {
          ERROR_OPERATIONS(stderr, "MusE error: PendingOperationList::add(): Double ModifyMidiDeviceAddress. Ignoring.\n");
          return false;  
        }
      break;
        
      case PendingOperationItem::ModifyMidiDeviceFlags:
        if(poi._type == PendingOperationItem::ModifyMidiDeviceFlags && poi._midi_device == op._midi_device &&
           poi._rw_flags == op._rw_flags && poi._open_flags == op._open_flags)
        {
          ERROR_OPERATIONS(stderr, "MusE error: PendingOperationList::add(): Double ModifyMidiDeviceFlags. Ignoring.\n");
          return false;  
        }
      break;
        
      case PendingOperationItem::ModifyMidiDeviceName:
        if(poi._type == PendingOperationItem::ModifyMidiDeviceName && poi._midi_device == op._midi_device &&
           poi._name == op._name)
        {
          ERROR_OPERATIONS(stderr, "MusE error: PendingOperationList::add(): Double ModifyMidiDeviceName. Ignoring.\n");
          return false;  
        }
      break;

      case PendingOperationItem::SetInstrument:
        if(poi._type == PendingOperationItem::SetInstrument && poi._midi_port == op._midi_port &&
           poi._midi_instrument == op._midi_instrument)
        {
          fprintf(stderr, "MusE error: PendingOperationList::add(): Double SetInstrument. Ignoring.\n");
          return false;  
        }
      break;

      
      case PendingOperationItem::AddTrack:
        if(poi._type == PendingOperationItem::AddTrack && poi._track_list == op._track_list && poi._track == op._track)  
        {
          // Simply replace the insert point.
          poi._insert_at = op._insert_at;
          // An operation will still take place.
          return true;  
        }
        else if(poi._type == PendingOperationItem::DeleteTrack && poi._track_list == op._track_list && poi._track == op._track)  
        {
          // Delete followed by add is useless. Cancel out the delete + add by erasing the delete command.
          //erase(ipos->second);
          //_map.erase(ipos);
          // No operation will take place.
          //return false;
        }
      break;
      
      case PendingOperationItem::DeleteTrack:
        if(poi._type == PendingOperationItem::DeleteTrack && poi._track_list == op._track_list && poi._track == op._track)  
        {
          ERROR_OPERATIONS(stderr, "MusE error: PendingOperationList::add(): Double DeleteTrack. Ignoring.\n");
          return false;  
        }
        else if(poi._type == PendingOperationItem::AddTrack && poi._track_list == op._track_list && poi._track == op._track)  
        {
          // Add followed by delete is useless. Cancel out the add + delete by erasing the add command.
          //erase(ipos->second);
          //_map.erase(ipos);
          // No operation will take place.
          //return false;
        }
      break;
      
      case PendingOperationItem::MoveTrack:
        if(poi._type == PendingOperationItem::MoveTrack && poi._track == op._track && poi._track_list == op._track_list)  
        {
          // Simply replace the 'to' index.
          poi._to_idx = op._to_idx;
          // An operation will still take place.
          return true;
        }
      break;
        
      case PendingOperationItem::ModifyTrackName:
        if(poi._type == PendingOperationItem::ModifyTrackName && poi._track == op._track &&
           poi._name == op._name)
        {
          ERROR_OPERATIONS(stderr, "MusE error: PendingOperationList::add(): Double ModifyTrackName. Ignoring.\n");
          return false;  
        }
      break;

      case PendingOperationItem::SetTrackRecord:
        if(poi._type == PendingOperationItem::SetTrackRecord && poi._track == op._track)
        {
          if(poi._boolA == op._boolA)  
          {
            ERROR_OPERATIONS(stderr, "MusE error: PendingOperationList::add(): Double SetTrackRecord. Ignoring.\n");
            return false;  
          }
          else
          {
            // On/off followed by off/on is useless. Cancel out the on/off + off/on by erasing the command.
            erase(ipos->second);
            _map.erase(ipos);
            // No operation will take place.
            return false;
          }
        }
      break;
      
      case PendingOperationItem::SetTrackMute:
        if(poi._type == PendingOperationItem::SetTrackMute && poi._track == op._track)
        {
          if(poi._boolA == op._boolA)  
          {
            ERROR_OPERATIONS(stderr, "MusE error: PendingOperationList::add(): Double SetTrackMute. Ignoring.\n");
            return false;  
          }
          else
          {
            // On/off followed by off/on is useless. Cancel out the on/off + off/on by erasing the command.
            erase(ipos->second);
            _map.erase(ipos);
            // No operation will take place.
            return false;
          }
        }
      break;
      
      case PendingOperationItem::SetTrackSolo:
        if(poi._type == PendingOperationItem::SetTrackSolo && poi._track == op._track)
        {
          if(poi._boolA == op._boolA)  
          {
            ERROR_OPERATIONS(stderr, "MusE error: PendingOperationList::add(): Double SetTrackSolo. Ignoring.\n");
            return false;  
          }
          else
          {
            // On/off followed by off/on is useless. Cancel out the on/off + off/on by erasing the command.
            erase(ipos->second);
            _map.erase(ipos);
            // No operation will take place.
            return false;
          }
        }
      break;
      
      case PendingOperationItem::SetTrackRecMonitor:
        if(poi._type == PendingOperationItem::SetTrackRecMonitor && poi._track == op._track)
        {
          if(poi._boolA == op._boolA)  
          {
            fprintf(stderr, "MusE error: PendingOperationList::add(): Double SetTrackRecMonitor. Ignoring.\n");
            return false;  
          }
          else
          {
            // On/off followed by off/on is useless. Cancel out the on/off + off/on by erasing the command.
            erase(ipos->second);
            _map.erase(ipos);
            // No operation will take place.
            return false;
          }
        }
      break;
      
      case PendingOperationItem::SetTrackOff:
        if(poi._type == PendingOperationItem::SetTrackOff && poi._track == op._track)
        {
          if(poi._boolA == op._boolA)  
          {
            fprintf(stderr, "MusE error: PendingOperationList::add(): Double SetTrackOff. Ignoring.\n");
            return false;  
          }
          else
          {
            // On/off followed by off/on is useless. Cancel out the on/off + off/on by erasing the command.
            erase(ipos->second);
            _map.erase(ipos);
            // No operation will take place.
            return false;
          }
        }
      break;
      
      case PendingOperationItem::AddPart:
        if(poi._type == PendingOperationItem::AddPart && poi._part_list == op._part_list && poi._part == op._part)  
        {
          ERROR_OPERATIONS(stderr, "MusE error: PendingOperationList::add(): Double AddPart. Ignoring.\n");
          return false;  
        }
        else if(poi._type == PendingOperationItem::DeletePart && poi._part_list == op._part_list && poi._iPart->second == op._part)  
        {
          // Delete followed by add is useless. Cancel out the delete + add by erasing the delete command.
          erase(ipos->second);
          _map.erase(ipos);
          // No operation will take place.
          return false;
        }
      break;
      
      case PendingOperationItem::DeletePart:
        if(poi._type == PendingOperationItem::DeletePart && poi._part_list == op._part_list && poi._iPart->second == op._iPart->second)  
        {
          ERROR_OPERATIONS(stderr, "MusE error: PendingOperationList::add(): Double DeletePart. Ignoring.\n");
          return false;  
        }
        else if(poi._type == PendingOperationItem::AddPart && poi._part_list == op._part_list && poi._part == op._iPart->second)  
        {
          // Add followed by delete is useless. Cancel out the add + delete by erasing the add command.
          erase(ipos->second);
          _map.erase(ipos);
          // No operation will take place.
          return false;
        }
      break;

      case PendingOperationItem::SelectPart:
        if(poi._type == PendingOperationItem::SelectPart && poi._part == op._part)  
        {
          // Simply replace the value.
          poi._boolA = op._boolA;
          // An operation will still take place.
          return true;
        }
      break;
      
      case PendingOperationItem::MovePart:
        if(poi._type == PendingOperationItem::MovePart && poi._part == op._part)  
        {
          // Simply replace the values.
          poi._iPart = op._iPart;
          poi._track = op._track;
          poi._posLenVal = op._posLenVal;
          // An operation will still take place.
          return true;
        }
      break;
      
      case PendingOperationItem::ModifyPartStart:
        if(poi._type == PendingOperationItem::ModifyPartStart)
        {
          // If the given list is not null and is already part of a previous ModifyPartStart command,
          //  it's an error, the list would be deleted twice.
          if(poi._part != op._part && op._event_list && op._event_list == poi._event_list)
          {
            ERROR_OPERATIONS(stderr, "MusE error: PendingOperationList::add(): ModifyPartStart: Same _event_list for two different parts. Ignoring.\n");
            return false;
          }
          
          if(poi._part == op._part)
          {
            // From here on in this block no matter what, we are re-using the existing command.

            // Simply replace the values.
            poi._posLenVal = op._posLenVal;
            poi._lenVal = op._lenVal;

            // If a list was given use it otherwise if no list was given don't touch the existing list.
            if(op._event_list)
            {
              // If the given list is the same as the existing list, it's really an error. We'll let it go but don't touch the existing list.
              // It should be safe to proceed without worrying about deleting the existing or given list here since it would be impossible
              //  to allocate the same pointer twice, beforehand.
              if(op._event_list == poi._event_list)
              {
                //ERROR_OPERATIONS(stderr, "MusE warning: PendingOperationList::add(): ModifyPartStart: Double _event_list. Ignoring second list.\n");
                //return false;
              }
              else
              {
                // Done with the existing original replacement list. If it exists, delete it.
                if(poi._event_list)
                  delete poi._event_list;
                // Replace the existing list pointer with the given one.
                poi._event_list = op._event_list;
              }
            }
            // An operation will still take place.
            return true;
          }
        }
      break;
      
      case PendingOperationItem::ModifyPartLength:
        if(poi._type == PendingOperationItem::ModifyPartLength)
        {
          // If the given list is not null and is already part of a previous ModifyPartLength command,
          //  it's an error, the list would be deleted twice.
          if(poi._part != op._part && op._event_list && op._event_list == poi._event_list)
          {
            ERROR_OPERATIONS(stderr, "MusE error: PendingOperationList::add(): ModifyPartLength: Same _event_list for two different parts. Ignoring.\n");
            return false;
          }
          
          if(poi._part == op._part)
          {
            // From here on in this block no matter what, we are re-using the existing command.

            // Simply replace the value.
            poi._posLenVal = op._posLenVal;

            // If a list was given use it otherwise if no list was given don't touch the existing list.
            if(op._event_list)
            {
              // If the given list is the same as the existing list, it's really an error. We'll let it go but don't touch the existing list.
              // It should be safe to proceed without worrying about deleting the existing or given list here since it would be impossible
              //  to allocate the same pointer twice, beforehand.
              if(op._event_list == poi._event_list)
              {
                //ERROR_OPERATIONS(stderr, "MusE warning: PendingOperationList::add(): ModifyPartLength: Double _event_list. Ignoring second list.\n");
                //return false;
              }
              else
              {
                // Done with the existing original replacement list. If it exists, delete it.
                if(poi._event_list)
                  delete poi._event_list;
                // Replace the existing list pointer with the given one.
                poi._event_list = op._event_list;
              }
            }
            // An operation will still take place.
            return true;
          }
        }
      break;
      
      case PendingOperationItem::ModifyPartName:
        if(poi._type == PendingOperationItem::ModifyPartName && poi._part == op._part &&
           poi._name == op._name)
        {
          ERROR_OPERATIONS(stderr, "MusE error: PendingOperationList::add(): Double ModifyPartName. Ignoring.\n");
          return false;  
        }
      break;
        
      
      case PendingOperationItem::AddEvent:
        if(poi._type == PendingOperationItem::AddEvent && poi._part == op._part && poi._ev == op._ev)  
        {
          ERROR_OPERATIONS(stderr, "MusE error: PendingOperationList::add(): Double AddEvent. Ignoring.\n");
          return false;  
        }
        else if(poi._type == PendingOperationItem::DeleteEvent && poi._part == op._part && poi._iev->second == op._ev)  
        {
          // Delete followed by add is useless. Cancel out the delete + add by erasing the delete command.
          erase(ipos->second);
          _map.erase(ipos);
          // No operation will take place.
          return false;
        }
      break;
      
      case PendingOperationItem::DeleteEvent:
        if(poi._type == PendingOperationItem::DeleteEvent && poi._part == op._part && poi._iev->second == op._iev->second)  
        {
          ERROR_OPERATIONS(stderr, "MusE error: PendingOperationList::add(): Double DeleteEvent. Ignoring.\n");
          return false;  
        }
        else if(poi._type == PendingOperationItem::AddEvent && poi._part == op._part && poi._ev == op._iev->second)  
        {
          // Add followed by delete is useless. Cancel out the add + delete by erasing the add command.
          erase(ipos->second);
          _map.erase(ipos);
          // No operation will take place.
          return false;
        }
      break;

      case PendingOperationItem::ModifyEventProperties:
        if(poi._type == PendingOperationItem::ModifyEventProperties && /*poi._event_list == op._event_list &&*/
           poi._iev->second == op._iev->second)  
        {
          // Simply replace the values.
          poi._event_list = op._event_list;
          poi._posLenVal = op._posLenVal;
          poi._lenVal = op._lenVal;
          poi._event_spos = op._event_spos;
          // An operation will still take place.
          return true;
        }
      break;
      
      case PendingOperationItem::SelectEvent:
        if(poi._type == PendingOperationItem::SelectEvent &&
           poi._part == op._part && poi._ev == op._ev)
        {
          // Simply replace the value.
          poi._intA = op._intA;
          // An operation will still take place.
          return true;
        }
      break;
      
      case PendingOperationItem::ModifyEventList:
// TODO Not quite right yet.
//         if(poi._type == PendingOperationItem::ModifyEventList && 
//           // If attempting to repeatedly modify the same list, or, if progressively modifying (list to list to list etc).
//           (poi._orig_event_list == op._orig_event_list || poi._event_list == op._event_list))
//         {
//           // Simply replace the list.
//           poi._event_list = op._event_list; 
      break;

      
      case PendingOperationItem::ModifyMidiCtrlValList:
// TODO Not quite right yet.
//         if(poi._type == PendingOperationItem::ModifyMidiCtrlValList && 
//           // If attempting to repeatedly modify the same list, or, if progressively modifying (list to list to list etc).
//           (poi._orig_mcvl == op._orig_mcvl || poi._mcvl == op._mcvl))
//         {
//           // Simply replace the list.
//           poi._mcvl = op._mcvl; 
      break;
      
      case PendingOperationItem::AddMidiCtrlVal:
        if(poi._type == PendingOperationItem::DeleteMidiCtrlVal && 
           poi._mcvl == op._mcvl && 
           poi._imcv->second.part == op._part &&
           poi._imcv->second.val == op._intB &&
           poi._imcv->first == op._posLenVal)
        {
          // Delete followed by add is useless. Cancel out the delete + add by erasing the delete command.
          erase(ipos->second);
          _map.erase(ipos);
          // No operation will take place.
          return false;
        }
      break;
      
      case PendingOperationItem::DeleteMidiCtrlVal:
        // Be sure _intB is set.
        if(poi._type == PendingOperationItem::AddMidiCtrlVal && 
           poi._mcvl == op._mcvl && 
           poi._part == op._imcv->second.part &&
           poi._intB == op._imcv->second.val &&
           poi._posLenVal == op._imcv->first)
        {
          // Add followed by delete is useless. Cancel out the add + delete by erasing the add command.
          erase(ipos->second);
          _map.erase(ipos);
          // No operation will take place.
          return false;
        }
      break;
      
      case PendingOperationItem::ModifyMidiCtrlVal:
// TODO FIXME Finish this
        
           // Be sure _intB/A is set
//         if(poi._type == PendingOperationItem::ModifyMidiCtrlVal &&
//            poi._mcvl == op._mcvl && 
//            poi._imcv->second.part == op._imcv->second.part &&
//            poi._imcv->second.val == op._imcv->second.val)
//         {
//           // Simply replace the value.
//           poi._intA = op._intA;
//           return true;
//         }
//         else if(poi._type == PendingOperationItem::DeleteMidiCtrlVal && poi._mcvl == op._mcvl && poi._imcv->second.part == op._imcv->second.part)
//         {
//           // Transform existing delete command into a modify command.
//           poi._type = PendingOperationItem::ModifyMidiCtrlVal;
//           poi._intA = op._intA; 
//           return true;
//         }
//         else if(poi._type == PendingOperationItem::AddMidiCtrlVal && poi._mcvl == op._mcvl && poi._part == op._imcv->second.part)
//         {
//           // Simply replace the add value with the modify value.
//           poi._intB = op._intA; 
//           return true;
//         }
      break;
      
      
      case PendingOperationItem::ModifyAudioCtrlValListList:
        if(poi._type == PendingOperationItem::ModifyAudioCtrlValListList)
        {
          if(op._aud_ctrl_list_list == op._src_aud_ctrl_list_list)
          {
            ERROR_OPERATIONS(stderr, "MusE error: PendingOperationList::add(): ModifyAudioCtrlValListList: Both lists the same. Ignoring.\n");
            return false;
          }
          // If attempting to repeatedly modify the same list, or, if progressively modifying (list to list to list etc).
          else if (poi._aud_ctrl_list_list == op._aud_ctrl_list_list || poi._src_aud_ctrl_list_list == op._aud_ctrl_list_list)
          {
            // Simply replace the list.
            poi._src_aud_ctrl_list_list = op._src_aud_ctrl_list_list;
            // An operation will still take place.
            return true;
          }
        }
      break;
      case PendingOperationItem::AddAudioCtrlValList:
        if(poi._type == PendingOperationItem::AddAudioCtrlValList &&
           poi._aud_ctrl_list_list == op._aud_ctrl_list_list &&
           poi._aud_ctrl_list == op._aud_ctrl_list)
        {
          ERROR_OPERATIONS(stderr, "MusE error: PendingOperationList::add(): Double AddAudioCtrlValList. Ignoring.\n");
          return false;
        }
        else if(poi._type == PendingOperationItem::DeleteAudioCtrlValList &&
          poi._aud_ctrl_list_list == op._aud_ctrl_list_list && poi._iCtrlList->second == op._aud_ctrl_list)
        {
          // Delete followed by add is useless. Cancel out the delete + add by erasing the delete command.
          erase(ipos->second);
          _map.erase(ipos);
          // No operation will take place.
          return false;
        }
      break;
      case PendingOperationItem::DeleteAudioCtrlValList:
        if(poi._type == PendingOperationItem::DeleteAudioCtrlValList &&
           poi._aud_ctrl_list_list == op._aud_ctrl_list_list &&
           poi._iCtrlList->second == op._iCtrlList->second)
        {
          ERROR_OPERATIONS(stderr, "MusE error: PendingOperationList::add(): Double DeleteAudioCtrlValList. Ignoring.\n");
          return false;
        }
        else if(poi._type == PendingOperationItem::AddAudioCtrlValList &&
          poi._aud_ctrl_list_list == op._aud_ctrl_list_list && poi._aud_ctrl_list == op._iCtrlList->second)
        {
          // Add followed by delete is useless. Cancel out the add + delete by erasing the add command.
          erase(ipos->second);
          _map.erase(ipos);
          // No operation will take place.
          return false;
        }
      break;

      case PendingOperationItem::ModifyAudioCtrlValList:
        if(poi._type == PendingOperationItem::ModifyAudioCtrlValList && 
          // If attempting to repeatedly modify the same list, or, if progressively modifying (list to list to list etc).
          (poi._iCtrlList->second == op._iCtrlList->second || poi._aud_ctrl_list == op._iCtrlList->second))
        {
          // Simply replace the list.
          poi._aud_ctrl_list = op._aud_ctrl_list; 
          // An operation will still take place.
          return true;
        }
      break;
      
      case PendingOperationItem::AddAudioCtrlVal:
        if(poi._aud_ctrl_list == op._aud_ctrl_list)
        {
          if(poi._type == PendingOperationItem::AddAudioCtrlVal && poi._posLenVal == op._posLenVal)
          {
            // Simply replace the value.
            poi._ctl_dbl_val = op._ctl_dbl_val;
            poi._ctl_flags = op._ctl_flags;
            // An operation will still take place.
            return true;
          }
#if 0
// TODO? ModifyAudioCtrlVal command has no flags member.
          else if(poi._type == PendingOperationItem::DeleteAudioCtrlVal && poi._iCtrl->first == op._posLenVal)
          {
            // Transform existing delete command into a modify command.
            poi._type = PendingOperationItem::ModifyAudioCtrlVal;
            poi._ctl_dbl_val = op._ctl_dbl_val;
            // An operation will still take place.
            return true;
          }
          else if(poi._type == PendingOperationItem::ModifyAudioCtrlVal && poi._iCtrl->first == op._posLenVal &&
            // If the ModifyAudioCtrlVal command's new frame is the same as the old.
            poi._posLenVal == poi._iCtrl->first)
          {
            // Simply replace the value.
            poi._ctl_dbl_val = op._ctl_dbl_val;
            // An operation will still take place.
            return true;
          }
#endif
        }
      break;
      
      case PendingOperationItem::AddAudioCtrlValStruct:
        if(poi._aud_ctrl_list == op._aud_ctrl_list)
        {
          if(poi._type == PendingOperationItem::AddAudioCtrlValStruct && poi._posLenVal == op._posLenVal)
          {
            if(op._audCtrlValStruct)
            {
              // Done with the existing original replacement structure. If it exists, delete it.
              if(poi._audCtrlValStruct)
                delete poi._audCtrlValStruct;
              // Replace the existing structure pointer with the given one.
              poi._audCtrlValStruct = op._audCtrlValStruct;
              // An operation will still take place.
              return true;
            }
          }
        }
      break;

      case PendingOperationItem::DeleteAudioCtrlVal:
        if(poi._iCtrl == op._iCtrl)
        {
          // Multiple delete iterator commands not allowed!
          ERROR_OPERATIONS(stderr, "MusE error: PendingOperationList::add(): Double DeleteAudioCtrlVal iCtrl. Ignoring.\n");
          return false;
        }
        else if(poi._aud_ctrl_list == op._aud_ctrl_list)
        {
          if(poi._type == PendingOperationItem::DeleteAudioCtrlVal && poi._iCtrl->first == op._iCtrl->first)
          {
            // Multiple delete commands not allowed!
            ERROR_OPERATIONS(stderr, "MusE error: PendingOperationList::add(): Double DeleteAudioCtrlVal frame. Ignoring.\n");
            return false;
          }
          else if(poi._type == PendingOperationItem::AddAudioCtrlVal && poi._posLenVal == op._iCtrl->first)
          {
            // Add followed by delete is useless. Cancel out the add + delete by erasing the add command.
            erase(ipos->second);
            _map.erase(ipos);
            // No operation will take place.
            return false;
          }
          else if(poi._type == PendingOperationItem::ModifyAudioCtrlVal && poi._iCtrl->first == op._iCtrl->first &&
            // If the ModifyAudioCtrlVal command's new frame is the same as the old.
            poi._posLenVal == poi._iCtrl->first)
          {
            // Modify followed by delete is equivalent to just deleting.
            // Transform existing modify command into a delete command.
            poi._type = PendingOperationItem::DeleteAudioCtrlVal;
            // An operation will still take place.
            return true;
          }
        }
      break;
      
      case PendingOperationItem::ModifyAudioCtrlVal:
        if(poi._aud_ctrl_list == op._aud_ctrl_list)
        {
          if(poi._type == PendingOperationItem::ModifyAudioCtrlVal && poi._iCtrl->first == op._iCtrl->first &&
            // If the previous ModifyAudioCtrlVal command's new frame is the same as the old.
            poi._posLenVal == poi._iCtrl->first &&
            // If the given ModifyAudioCtrlVal command's new frame is the same as the old.
            op._posLenVal == op._iCtrl->first)
          {
            // Simply replace the value.
            poi._ctl_dbl_val = op._ctl_dbl_val;
            // An operation will still take place.
            return true;
          }
          else if(poi._type == PendingOperationItem::DeleteAudioCtrlVal && poi._iCtrl->first == op._iCtrl->first &&
            // If the given ModifyAudioCtrlVal command's new frame is the same as the old.
            op._posLenVal == op._iCtrl->first)
          {
            // Transform existing delete command into a modify command.
            poi._type = PendingOperationItem::ModifyAudioCtrlVal;
            poi._ctl_dbl_val = op._ctl_dbl_val;
            // An operation will still take place.
            return true;
          }
#if 0
// TODO? ModifyAudioCtrlVal command has no flags member.
          else if(poi._type == PendingOperationItem::AddAudioCtrlVal && poi._posLenVal == op._iCtrl->first &&
            // If the given ModifyAudioCtrlVal command's new frame is the same as the old.
            op._posLenVal == op._iCtrl->first)
          {
            // Simply replace the add value with the modify value.
            poi._ctl_dbl_val = op._ctl_dbl_val;
            // An operation will still take place.
            return true;
          }
#endif
        }
      break;
      
      case PendingOperationItem::SelectAudioCtrlVal:
        if(poi._type == PendingOperationItem::SelectAudioCtrlVal && poi._audCtrlValStruct == op._audCtrlValStruct)
        {
          // Simply replace the value.
          poi._selected = op._selected;
          // An operation will still take place.
          return true;
        }
      break;

      case PendingOperationItem::SetAudioCtrlPasteEraseMode:
        if(poi._type == PendingOperationItem::SetAudioCtrlPasteEraseMode)
        {
          // Simply replace the value.
          poi._audio_ctrl_paste_erase_opts = op._audio_ctrl_paste_erase_opts;
          // An operation will still take place.
          return true;
        }
      break;

// TODO: This would work but I think we may have to allow multiple calls since
//        there might be multiple time a change is followed by this call.
//       Ideally only the LAST one would be kept.
//
//       case PendingOperationItem::UpdateAllAudioCtrlGroups:
//         DEBUG_OPERATIONS(stderr, "PendingOperationList::add() UpdateAllAudioCtrlGroups\n");
//         if(poi._type == PendingOperationItem::UpdateAllAudioCtrlGroups)
//         {
//           ERROR_OPERATIONS(stderr, "MusE error: PendingOperationList::add(): Double UpdateAllAudioCtrlGroups. Ignoring.\n");
//           return false;
//         }
//       break;
//
//       case PendingOperationItem::UpdateAudioCtrlListGroups:
//         DEBUG_OPERATIONS(stderr, "PendingOperationList::add() UpdateAudioCtrlListGroups\n");
//         if(poi._type == PendingOperationItem::UpdateAudioCtrlListGroups && poi._aud_ctrl_list_list == op._aud_ctrl_list_list)
//         {
//           ERROR_OPERATIONS(stderr, "MusE error: PendingOperationList::add(): Double UpdateAudioCtrlListGroups. Ignoring.\n");
//           return false;
//         }
//       break;
//
//       case PendingOperationItem::UpdateAudioCtrlGroups:
//         DEBUG_OPERATIONS(stderr, "PendingOperationList::add() UpdateAudioCtrlGroups\n");
//         if(poi._type == PendingOperationItem::UpdateAudioCtrlGroups && poi._aud_ctrl_list == op._aud_ctrl_list)
//         {
//           ERROR_OPERATIONS(stderr, "MusE error: PendingOperationList::add(): Double UpdateAudioCtrlGroups. Ignoring.\n");
//           return false;
//         }
//       break;
//
//       case PendingOperationItem::UpdateAudioCtrlPosGroups:
//         DEBUG_OPERATIONS(stderr, "PendingOperationList::add() UpdateAudioCtrlPosGroups\n");
//         if(poi._type == PendingOperationItem::UpdateAudioCtrlPosGroups && poi._aud_ctrl_list == op._aud_ctrl_list &&
//           poi._iCtrl->first == op._iCtrl->first)
//         {
//           ERROR_OPERATIONS(stderr, "MusE error: PendingOperationList::add(): Double UpdateAudioCtrlPosGroups. Ignoring.\n");
//           return false;
//         }
//       break;

      case PendingOperationItem::ModifyTempoList:
// TODO Not quite right yet.
//         if(poi._type == PendingOperationItem::ModifyTempoList && 
//           // If attempting to repeatedly modify the same list, or, if progressively modifying (list to list to list etc).
//           (poi._orig_tempo_list == op._orig_tempo_list || poi._tempo_list == op._tempo_list))
//         {
//           // Simply replace the list.
//           poi._tempo_list = op._tempo_list; 
      break;

      
      case PendingOperationItem::SetStaticTempo:
#ifdef _PENDING_OPS_DEBUG_
        fprintf(stderr, "PendingOperationList::add() SetStaticTempo\n");
#endif      
        if(poi._type == PendingOperationItem::SetStaticTempo && poi._tempo_list == op._tempo_list)
        {
          // Simply replace the value.
          poi._intA = op._intA; 
          // An operation will still take place.
          return true;
        }
      break;
        
      case PendingOperationItem::SetGlobalTempo:
        DEBUG_OPERATIONS(stderr, "PendingOperationList::add() SetGlobalTempo\n");
        if(poi._type == PendingOperationItem::SetGlobalTempo && poi._tempo_list == op._tempo_list)
        {
          // Simply replace the new value.
          poi._intA = op._intA; 
          // An operation will still take place.
          return true;
        }
      break;

      
      case PendingOperationItem::ModifySigList:
// TODO Not quite right yet.
//         if(poi._type == PendingOperationItem::ModifySigList && 
//           // If attempting to repeatedly modify the same list, or, if progressively modifying (list to list to list etc).
//           (poi._orig_sig_list == op._orig_sig_list || poi._sig_list == op._sig_list))
//         {
//           // Simply replace the list.
//           poi._sig_list = op._sig_list; 
      break;

      
      case PendingOperationItem::ModifyKeyList:
// TODO Not quite right yet.
//         if(poi._type == PendingOperationItem::ModifyKeyList && 
//           // If attempting to repeatedly modify the same list, or, if progressively modifying (list to list to list etc).
//           (poi._orig_key_list == op._orig_key_list || poi._key_list == op._key_list))
//         {
//           // Simply replace the list.
//           poi._key_list = op._key_list; 
      break;
      
      
      case PendingOperationItem::AddStretchListRatioAt:
        if(poi._type == PendingOperationItem::AddStretchListRatioAt && poi._stretch_list == op._stretch_list && 
           poi._stretch_type == op._stretch_type &&
           poi._museFrame == op._museFrame)
        {
          // Simply replace the value.
          poi._audio_converter_value = op._audio_converter_value; 
          return true;
        }
// Todo?
//         else if(poi._type == PendingOperationItem::DeleteStretchListRatioAt && poi._stretch_list == op._stretch_list && 
//            poi._stretch_type == op._stretch_type)
//         {
//           // Transform existing delete command into a modify command.
//           poi._type = PendingOperationItem::ModifyStretchListRatioAt;
//           poi._audio_converter_value = op._audio_converter_value; 
//           return true;
//         }
        else if(poi._type == PendingOperationItem::ModifyStretchListRatioAt && poi._stretch_list == op._stretch_list &&
           poi._stretch_type == op._stretch_type &&
           poi._iStretchEvent->first == op._museFrame &&
           // If the ModifyStretchListRatioAt command's new frame is the same as the old.
           poi._museFrame == poi._iStretchEvent->first)
        {
          // Simply replace the value.
          poi._audio_converter_value = op._audio_converter_value;
          return true;
        }
      break;
      
      case PendingOperationItem::DeleteStretchListRatioAt:
        if(poi._type == PendingOperationItem::DeleteStretchListRatioAt && poi._stretch_list == op._stretch_list && 
           poi._stretch_type == op._stretch_type &&
           poi._iStretchEvent->first == op._iStretchEvent->first)
        {
          // Multiple delete commands not allowed! 
          ERROR_OPERATIONS(stderr, "MusE error: PendingOperationList::add(): Double DeleteStretchRatioAt. Ignoring.\n");
          return false;
        }
// Todo?
//         else if(poi._type == PendingOperationItem::AddStretchListRatioAt && poi._stretch_list == op._stretch_list)
//         {
//           // Add followed by delete is useless. Cancel out the add + delete by erasing the add command.
//           erase(ipos->second);
//           _map.erase(ipos);
//           return true;
//         }
//         else if(poi._type == PendingOperationItem::ModifyStretchListRatioAt && poi._stretch_list == op._stretch_list)
//         {
//           // Modify followed by delete is equivalent to just deleting.
//           // Transform existing modify command into a delete command.
//           poi._type = PendingOperationItem::DeleteStretchListRatioAt;
//           return true;
//         }
      break;
      
      case PendingOperationItem::ModifyStretchListRatioAt:
        if(poi._type == PendingOperationItem::ModifyStretchListRatioAt && poi._stretch_list == op._stretch_list &&
           poi._stretch_type == op._stretch_type &&
           poi._iStretchEvent->first == op._iStretchEvent->first &&
           // If the previous ModifyStretchListRatioAt command's new frame is the same as the old.
           poi._museFrame == poi._iStretchEvent->first &&
           // If the given ModifyStretchListRatioAt command's new frame is the same as the old.
           op._museFrame == op._iStretchEvent->first)
        {
          // Simply replace the value.
          poi._audio_converter_value = op._audio_converter_value;
          return true;
        }
// Todo?
//         else if(poi._type == PendingOperationItem::DeleteStretchListRatioAt && poi._stretch_list == op._stretch_list && 
//            poi._stretch_type == op._stretch_type)
//         {
//           // Transform existing delete command into a modify command.
//           poi._type = PendingOperationItem::ModifyStretchListRatioAt;
//           poi._audio_converter_value = op._audio_converter_value; 
//           return true;
//         }
        else if(poi._type == PendingOperationItem::AddStretchListRatioAt && poi._stretch_list == op._stretch_list &&
           poi._stretch_type == op._stretch_type &&
           poi._museFrame == op._iStretchEvent->first &&
           // If the given ModifyStretchListRatioAt command's new frame is the same as the old.
           op._museFrame == op._iStretchEvent->first)
        {
          // Simply replace the add value with the modify value.
          poi._audio_converter_value = op._audio_converter_value; 
          return true;
        }
      break;


      case PendingOperationItem::ModifyStretchListRatio:
        if(poi._type == PendingOperationItem::ModifyStretchListRatio && poi._stretch_list == op._stretch_list &&
           poi._stretch_type == op._stretch_type)
        {
          // Simply replace the value.
          poi._audio_converter_value = op._audio_converter_value;
          return true;
        }
      break;


      case PendingOperationItem::ModifySongLength:
        DEBUG_OPERATIONS(stderr, "PendingOperationList::add() ModifySongLength\n");
        if(poi._type == PendingOperationItem::ModifySongLength)
        {
          // Simply replace the value.
          poi._intA = op._intA;
          // An operation will still take place.
          return true;
        }
      break;  

      case PendingOperationItem::EnableAllAudioControllers:
        DEBUG_OPERATIONS(stderr, "PendingOperationList::add() EnableAllAudioControllers\n");
        if(poi._type == PendingOperationItem::EnableAllAudioControllers)
        {
          ERROR_OPERATIONS(stderr, "MusE error: PendingOperationList::add(): Double EnableAllAudioControllers. Ignoring.\n");
          return false;  
        }
      break;  

      case PendingOperationItem::GlobalSelectAllEvents:
#ifdef _PENDING_OPS_DEBUG_
        fprintf(stderr, "PendingOperationList::add() GlobalSelectAllEvents\n");
#endif      
        if(poi._type == PendingOperationItem::GlobalSelectAllEvents && poi._track_list == op._track_list) 
        {
          if(poi._boolA == op._boolA)
          {
            fprintf(stderr, "MusE error: PendingOperationList::add(): Double GlobalSelectAllEvents. Ignoring.\n");
            return false;  
          }
          else
          {
            // Special: Do not 'cancel' out this one. The selections may need to affect all events.
            // Simply replace the value.
            poi._boolA = op._boolA; 
            // An operation will still take place.
            return true;
          }
        }
      break;  

      case PendingOperationItem::ModifyAudioSamples:
// TODO Not quite right yet.
//         if(poi._type == PendingOperationItem::ModifyAudioSamples && 
//           // If attempting to repeatedly modify the same list, or, if progressively modifying (list to list to list etc).
//           poi._audioSamplesPointer && op._audioSamplesPointer &&
//           (*poi._audioSamplesPointer == *op._audioSamplesPointer || poi._newAudioSamples == op._newAudioSamples))
//         {
//           // Simply replace the list.
//           poi._newAudioSamples = op._newAudioSamples; 
//           poi._newAudioSamplesLen = op._newAudioSamplesLen; 
//           return true;
//         }
      break;
      
      case PendingOperationItem::ModifyMarkerList:
// TODO Not quite right yet.
//         if(poi._type == PendingOperationItem::ModifyMarkerList && 
//           // If attempting to repeatedly modify the same list, or, if progressively modifying (list to list to list etc).
//           poi._orig_marker_list && op._orig_marker_list &&
//           (*poi._orig_marker_list == *op._orig_marker_list || poi._marker_list == op._marker_list))
//         {
//           // Simply replace the list.
//           poi._newAudioSamples = op._newAudioSamples; 
//           poi._newAudioSamplesLen = op._newAudioSamplesLen; 
      break;

      case PendingOperationItem::SwitchMetronomeSettings:
        if(poi._type == PendingOperationItem::SwitchMetronomeSettings && 
          (poi._bool_pointer == op._bool_pointer))
        {
          if(poi._boolA == op._boolA)
          {
            fprintf(stderr, "MusE error: PendingOperationList::add(): Double SwitchMetronomeSettings. Ignoring.\n");
            // No operation will take place.
            return false;  
          }
          else
          {
            // Enable or disable followed by disable or enable is useless. Cancel out both by erasing the command.
            erase(ipos->second);
            _map.erase(ipos);
            // No operation will take place.
            return false;
          }
        }
      break;
      
      case PendingOperationItem::ModifyMetronomeAccentMap:
// TODO Not quite right yet.
//         if(poi._type == PendingOperationItem::ModifyMetronomeAccentMap && 
//           // If attempting to repeatedly modify the same list, or, if progressively modifying (list to list to list etc).
//           (poi._metroAccentsMap == op._metroAccentsMap || poi._newMetroAccentsMap == op._newMetroAccentsMap))
//         {
//           // Simply replace the list.
//           poi._newMetroAccentsMap = op._newMetroAccentsMap;
//           // An operation will still take place.
//           return true;
//         }
      break;
      
      case PendingOperationItem::SwitchMidiRemoteSettings:
        if(poi._type == PendingOperationItem::SwitchMidiRemoteSettings &&
          (poi._bool_pointer == op._bool_pointer))
        {
          if(poi._boolA == op._boolA)
          {
            fprintf(stderr, "MusE error: PendingOperationList::add(): Double SwitchMidiRemoteSettings. Ignoring.\n");
            // No operation will take place.
            return false;
          }
          else
          {
            // Enable or disable followed by disable or enable is useless. Cancel out both by erasing the command.
            erase(ipos->second);
            _map.erase(ipos);
            // No operation will take place.
            return false;
          }
        }
      break;

      case PendingOperationItem::ModifyMidiRemote:
// TODO Not quite right yet.
      break;

      case PendingOperationItem::SetExternalSyncFlag:
        if(poi._type == PendingOperationItem::SetExternalSyncFlag && 
          (poi._bool_pointer == op._bool_pointer))
        {
          if(poi._boolA == op._boolA)
          {
            fprintf(stderr, "MusE error: PendingOperationList::add(): Double SetExternalSyncFlag. Ignoring.\n");
            // No operation will take place.
            return false;  
          }
          else
          {
            // Enable or disable followed by disable or enable is useless. Cancel out both by erasing the command.
            erase(ipos->second);
            _map.erase(ipos);
            // No operation will take place.
            return false;
          }
        }
      break;
      
      case PendingOperationItem::SetUseJackTransport:
        if(poi._type == PendingOperationItem::SetUseJackTransport && 
          (poi._bool_pointer == op._bool_pointer))
        {
          if(poi._boolA == op._boolA)
          {
            fprintf(stderr, "MusE error: PendingOperationList::add(): Double SetUseJackTransport. Ignoring.\n");
            // No operation will take place.
            return false;  
          }
          else
          {
            // Enable or disable followed by disable or enable is useless. Cancel out both by erasing the command.
            erase(ipos->second);
            _map.erase(ipos);
            // No operation will take place.
            return false;
          }
        }
      break;
      
      case PendingOperationItem::SetUseMasterTrack:
        if(poi._type == PendingOperationItem::SetUseMasterTrack && 
          (poi._tempo_list == op._tempo_list))
        {
          if(poi._boolA == op._boolA)
          {
            fprintf(stderr, "MusE error: PendingOperationList::add(): Double SetUseMasterTrack. Ignoring.\n");
            // No operation will take place.
            return false;  
          }
          else
          {
            // Enable or disable followed by disable or enable is useless. Cancel out both by erasing the command.
            erase(ipos->second);
            _map.erase(ipos);
            // No operation will take place.
            return false;
          }
        }
      break;
      
      case PendingOperationItem::Uninitialized:
        ERROR_OPERATIONS(stderr, "MusE error: PendingOperationList::add(): Uninitialized item. Ignoring.\n");
        return false;  
      break;  
        
      default:
      break;  
    }
  }
  
  iPendingOperation iipo = insert(end(), op);
  _map.insert(std::pair<unsigned int, iPendingOperation>(t, iipo));
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


//========================================================================


//---------------------------------------------------------
//   addDeviceOperation
//---------------------------------------------------------

void PendingOperationList::addDeviceOperation(MidiDeviceList* devlist, MidiDevice* dev)
{
  bool gotUniqueName=false;
  int increment = 0;
  const QString origname = dev->name();
  QString newName = origname;
  PendingOperationItem poi(devlist, dev, PendingOperationItem::AddMidiDevice);
  // check if the name's been taken
  while(!gotUniqueName) 
  {
    if(increment >= 10000)
    {
      fprintf(stderr, "MusE Error: PendingOperationList::addDeviceOperation(): Out of 10000 unique midi device names!\n");
      return;        
    }
    gotUniqueName = true;
    // In the case of type AddMidiDevice, this searches for the name only.
    iPendingOperation ipo = findAllocationOp(poi);
    if(ipo != end())
    {
      PendingOperationItem& poif = *ipo;
      if(poif._midi_device == poi._midi_device)
        return;  // Device itself is already added! 
      newName = origname + QString("_%1").arg(++increment);
      gotUniqueName = false;
    }    
    
    for(ciMidiDevice i = devlist->cbegin(); i != devlist->cend(); ++i) 
    {
      const QString s = (*i)->name();
      if(s == newName)
      {
        newName = origname + QString("_%1").arg(++increment);
        gotUniqueName = false;
      }
    }
  }
  
  if(origname != newName)
    dev->setName(newName);
  
  add(poi);
}

//---------------------------------------------------------
//   addPartPortCtrlEvents
//---------------------------------------------------------

void PendingOperationList::addPartPortCtrlEvents(
  const Event& event, Part* part, unsigned int tick, unsigned int /*len*/, Track* track)
{
  if(!track || !track->isMidiTrack())
    return;
  
  if(event.type() == Controller)
  {
    unsigned int tck  = event.tick() + tick;
    int cntrl = event.dataA();
    int val   = event.dataB();
    MidiTrack* mt = (MidiTrack*)track;
    MidiPort* mp;
    int ch;
    mt->mappedPortChanCtrl(&cntrl, nullptr, &mp, &ch);

    MidiCtrlValListList* mcvll = mp->controller();
    MidiCtrlValList* mcvl = nullptr;
    iMidiCtrlValList imcvll = mcvll->find(ch, cntrl);
    if(imcvll == mcvll->end()) 
    {
      PendingOperationItem poi(mcvll, 0, ch, cntrl, PendingOperationItem::AddMidiCtrlValList);
      if(findAllocationOp(poi) == end())
      {
        mcvl = new MidiCtrlValList(cntrl);
        poi._mcvl = mcvl;
        add(poi);
      }
    }
    else
    {
      mcvl = imcvll->second;
    }

    //assert(mcvl != nullptr); //FIXME: Can this happen? (danvd). UPDATE: Yes, it can (danvd)
    if(mcvl != nullptr)
    {
      // The operation will catch and ignore events which are past the end of the part.
      add(PendingOperationItem(mcvl, part, tck, val, PendingOperationItem::AddMidiCtrlVal));
    }
  }
}

void PendingOperationList::addPartPortCtrlEvents(Part* part, unsigned int tick, unsigned int len, Track* track)
{
  if(!track || !track->isMidiTrack())
    return;
  for(ciEvent ie = part->events().begin(); ie != part->events().end(); ++ie)
  {
    // The operation will catch and ignore events which are past the end of the part.
    addPartPortCtrlEvents(ie->second, part, tick, len, track);
  }
}

//---------------------------------------------------------
//   removePartPortCtrlEvents
//---------------------------------------------------------

bool PendingOperationList::removePartPortCtrlEvents(const Event& event, Part* part, Track* track)
{
  if(!track || !track->isMidiTrack())
    return false;
  
  if(event.type() == Controller)
  {
    MidiTrack* mt = (MidiTrack*)track;
//     MidiPort* mp = &MusEGlobal::midiPorts[mt->outPort()];
//     int ch = mt->outChannel();
    
    unsigned int tck  = event.tick() + part->tick();
    int cntrl = event.dataA();
    int val   = event.dataB();
    
    // Is it a drum controller event, according to the track port's instrument?
    MidiPort* mp;
    int ch;
    mt->mappedPortChanCtrl(&cntrl, nullptr, &mp, &ch);


    MidiCtrlValListList* mcvll = mp->controller();
    iMidiCtrlValList cl = mcvll->find(ch, cntrl);
    if (cl == mcvll->end()) {
                fprintf(stderr, "removePartPortCtrlEvents: controller %d(0x%x) for channel %d not found size %zd\n",
                    cntrl, cntrl, ch, mcvll->size());
          return false;
          }
    MidiCtrlValList* mcvl = cl->second;
    iMidiCtrlVal imcv = mcvl->findMCtlVal(tck, part, val);
    if (imcv == mcvl->end()) {
          // Let's throw up the error only if we were expecting the cache event to be there,
          //  as is the case when the tick is inside the part. When the tick is NOT inside the part
          //  a cache event should really not be there. But if one is found it should be deleted anyway.
#ifdef ALLOW_LEFT_HIDDEN_EVENTS
          if((int)tck >= (int)part->tick() && (int)tck < (int)part->tick() + (int)part->lenTick())
#else
          if(tck < part->tick() + part->lenTick())
#endif
            fprintf(stderr, "removePartPortCtrlEvents: (tick: %u): not found (size %zd)\n", tck, mcvl->size());
          return false;
          }
    return add(PendingOperationItem(mcvl, imcv, PendingOperationItem::DeleteMidiCtrlVal));
  }
  return false;
}

void PendingOperationList::removePartPortCtrlEvents(Part* part, Track* track)
{
  if(!track || !track->isMidiTrack())
    return;
  for(ciEvent ie = part->events().begin(); ie != part->events().end(); ++ie)
  {
    removePartPortCtrlEvents(ie->second, part, track);
  }
}

//---------------------------------------------------------
//   addPortCtrlEvents
//---------------------------------------------------------

void PendingOperationList::addTrackPortCtrlEvents(Track* track)
{
  if(!track || !track->isMidiTrack())
    return;
  const PartList* pl = track->cparts();
  for(ciPart ip = pl->begin(); ip != pl->end(); ++ip)
  {
    Part* part = ip->second;
    addPartPortCtrlEvents(part, part->tick(), part->lenTick(), track);
  }
}

//---------------------------------------------------------
//   removePortCtrlEvents
//---------------------------------------------------------

void PendingOperationList::removeTrackPortCtrlEvents(Track* track)
{
  if(!track || !track->isMidiTrack())
    return;
  const PartList* pl = track->cparts();
  for(ciPart ip = pl->begin(); ip != pl->end(); ++ip)
  {
    Part* part = ip->second;
    removePartPortCtrlEvents(part, track);
  }
}

void PendingOperationList::modifyPartPortCtrlEvents(const Event& old_event, const Event& event, Part* part)
{
  Track* t = part->track();
  if(!t || !t->isMidiTrack())
    return;
  if(old_event.type() != Controller || event.type() != Controller)
    return;
  MidiTrack* mt = static_cast<MidiTrack*>(t);
  
  unsigned int tck_erase  = old_event.tick() + part->tick();
  int cntrl_erase = old_event.dataA();
  int val_erase = old_event.dataB();
  iMidiCtrlVal imcv_erase;
  bool found_erase = false;

  // Is it a drum controller old_event, according to the track port's instrument?
  int ch_erase;
  MidiPort* mp_erase;
  mt->mappedPortChanCtrl(&cntrl_erase, nullptr, &mp_erase, &ch_erase);

  
  MidiCtrlValListList* mcvll_erase = mp_erase->controller();
  MidiCtrlValList* mcvl_erase = 0;
  iMidiCtrlValList cl_erase = mcvll_erase->find(ch_erase, cntrl_erase);
  if(cl_erase == mcvll_erase->end()) 
  {
    if(MusEGlobal::debugMsg)
      printf("modifyPartPortCtrlEvents: controller %d(0x%x) for channel %d not found size %zd\n",
              cntrl_erase, cntrl_erase, ch_erase, mcvll_erase->size());
  }
  else
  {
    mcvl_erase = cl_erase->second;
    imcv_erase = mcvl_erase->findMCtlVal(tck_erase, part, val_erase);
    if(imcv_erase == mcvl_erase->end()) 
    {
      if(MusEGlobal::debugMsg)
        printf("modifyPartPortCtrlEvents(tick:%u val:%d): not found (size %zd)\n", tck_erase, val_erase, mcvl_erase->size());
    }
    else
      found_erase = true;
  }

  unsigned int tck_add  = event.tick() + part->tick();
  int cntrl_add = event.dataA();
  int val_add   = event.dataB();
  
  
  // FIXME FIXME CHECK THIS
  //
  //  Why wasn't 'ch' given its own 'ch_add' variable in the original code?
  //  And why did 'mp_add' default to mp_erase above. 
  //  That means the channel and port would have defaulted to the ones
  //   being erased above, not the track's. That can't be right !
  
  
  // Is it a drum controller event, according to the track port's instrument?
  int ch_add;
  MidiPort* mp_add;
  mt->mappedPortChanCtrl(&cntrl_add, nullptr, &mp_add, &ch_add);

  MidiCtrlValList* mcvl_add;
  MidiCtrlValListList* mcvll_add = mp_add->controller();
  iMidiCtrlValList imcvll_add = mcvll_add->find(ch_add, cntrl_add);
  if(imcvll_add == mcvll_add->end()) 
  {
    if(found_erase)
      add(PendingOperationItem(mcvl_erase, imcv_erase, PendingOperationItem::DeleteMidiCtrlVal));
    PendingOperationItem poi(mcvll_add, 0, ch_add, cntrl_add, PendingOperationItem::AddMidiCtrlValList);
    if(findAllocationOp(poi) == end())
    {
      poi._mcvl = new MidiCtrlValList(cntrl_add);
      add(poi);
    }
    // The operation will catch and ignore events which are past the end of the part.
    add(PendingOperationItem(poi._mcvl, part, tck_add, val_add, PendingOperationItem::AddMidiCtrlVal));
    return;
  }
  else
  {
    mcvl_add = imcvll_add->second;
    iMidiCtrlVal imcv_add = mcvl_add->findMCtlVal(tck_add, part, val_add);
    if(imcv_add != mcvl_add->end()) 
    {
      if(tck_erase == tck_add && mcvl_erase == mcvl_add)
      {
        // The operation will catch and ignore events which are past the end of the part.
        add(PendingOperationItem(mcvl_add, imcv_add, tck_add, val_add, PendingOperationItem::ModifyMidiCtrlVal));
      }
      else
      {
        if(found_erase)
        {
          add(PendingOperationItem(mcvl_erase, imcv_erase, PendingOperationItem::DeleteMidiCtrlVal));
        }
        // The operation will catch and ignore events which are past the end of the part.
        add(PendingOperationItem(mcvl_add, part, tck_add, val_add, PendingOperationItem::AddMidiCtrlVal));
      }
      return;
    }
    else
    {
      if(found_erase)
        add(PendingOperationItem(mcvl_erase, imcv_erase, PendingOperationItem::DeleteMidiCtrlVal));
      // The operation will catch and ignore events which are past the end of the part.
      add(PendingOperationItem(mcvl_add, part, tck_add, val_add, PendingOperationItem::AddMidiCtrlVal));
    }
  }
}

void PendingOperationList::modifyPartPortCtrlEvents(const Event& event, unsigned newPosVal, int newVal, Part* part)
{
  Track* t = part->track();
  if(!t || !t->isMidiTrack())
    return;
  if(event.type() != Controller)
    return;
  MidiTrack* mt = static_cast<MidiTrack*>(t);
  
  const unsigned int old_tck  = event.tick() + part->tick();
  const unsigned int new_tck  = newPosVal + part->tick();
  int cntrl = event.dataA();
  const int old_val = event.dataB();

  // Is it a drum controller old_event, according to the track port's instrument?
  int ch;
  MidiPort* mp;
  mt->mappedPortChanCtrl(&cntrl, nullptr, &mp, &ch);
  
  MidiCtrlValListList* mcvll = mp->controller();
  iMidiCtrlValList imcvl = mcvll->find(ch, cntrl);
  if(imcvl == mcvll->end()) 
  {
    if(MusEGlobal::debugMsg)
      fprintf(stderr, "modifyPartPortCtrlEvents: controller %d(0x%x) for channel %d not found size %zd\n",
                   cntrl, cntrl, ch, mcvll->size());
    PendingOperationItem poi(mcvll, 0, ch, cntrl, PendingOperationItem::AddMidiCtrlValList);
    if(findAllocationOp(poi) == end())
    {
      poi._mcvl = new MidiCtrlValList(cntrl);
      add(poi);
    }
    // The operation will catch and ignore events which are past the end of the part.
    add(PendingOperationItem(poi._mcvl, part, new_tck, newVal, PendingOperationItem::AddMidiCtrlVal));
    return;
  }
  else
  {
    MidiCtrlValList* mcvl = imcvl->second;
    iMidiCtrlVal imcv = mcvl->findMCtlVal(old_tck, part, old_val);
    if(imcv == mcvl->end()) 
    {
      if(MusEGlobal::debugMsg)
        fprintf(stderr, "modifyPartPortCtrlEvents(tick:%u val:%d): not found (size %zd)\n", old_tck, old_val, mcvl->size());
      // The operation will catch and ignore events which are past the end of the part.
      add(PendingOperationItem(mcvl, part, new_tck, newVal, PendingOperationItem::AddMidiCtrlVal));
      return;
    }
    else
    {
      // The operation will catch and ignore events which are past the end of the part.
      add(PendingOperationItem(mcvl, imcv, new_tck, newVal, PendingOperationItem::ModifyMidiCtrlVal));
      return;
    }
  }
}

void PendingOperationList::addPartOperation(PartList *partlist, Part* part)
{
  // There is protection, in the catch-all Undo::insert(), from failure here (such as double add, del + add, add + del)
  //  which might cause addPortCtrlEvents() without parts or without corresponding removePortCtrlEvents etc.
  add(PendingOperationItem(partlist, part, PendingOperationItem::AddPart));
  addPartPortCtrlEvents(part, part->posValue(), part->lenValue(), part->track());
}

void PendingOperationList::delPartOperation(PartList *partlist, Part* part)
{
  // There is protection, in the catch-all Undo::insert(), from failure here (such as double del, del + add, add + del)
  //  which might cause addPortCtrlEvents() without parts or without corresponding removePortCtrlEvents etc.
  removePartPortCtrlEvents(part, part->track());
  iPart i;
  for (i = partlist->begin(); i != partlist->end(); ++i) {
        if (i->second == part) {
              add(PendingOperationItem(partlist, i, PendingOperationItem::DeletePart));
              return;
              }
        }
  printf("THIS SHOULD NEVER HAPPEN: could not find the part in PendingOperationList::delPartOperation()!\n");
}
      
void PendingOperationList::movePartOperation(PartList *partlist, Part* part, unsigned int new_pos, Track* track)
{
  removePartPortCtrlEvents(part, part->track());
  iPart i = partlist->end();
  if(track)
  {
    for (i = partlist->begin(); i != partlist->end(); ++i) {
          if (i->second == part) 
                break;
          }
    if(i == partlist->end())
      printf("THIS SHOULD NEVER HAPPEN: could not find the part in PendingOperationList::movePartOperation()!\n");
  }
  
  add(PendingOperationItem(i, part, new_pos, PendingOperationItem::MovePart, track));

  if(!track)
    track = part->track();
  
  addPartPortCtrlEvents(part, new_pos, part->lenValue(), track);
}

void PendingOperationList::modifyPartStartOperation(
  Part* part, unsigned int new_pos, unsigned int new_len, int64_t events_offset,
  Pos::TType /*events_offset_time_type*/)
{
  if(!part->track())
    return;
    
  PartList* partlist = part->track()->parts();
  iPart ip = partlist->end();
  for (ip = partlist->begin(); ip != partlist->end(); ++ip) {
        if (ip->second == part) 
              break;
        }
  if(ip == partlist->end())
  {
    fprintf(stderr, "THIS SHOULD NEVER HAPPEN: could not find part in PendingOperationList::modifyPartStartOperation()!\n");
    return;
  }

  // REMOVE Tim. wave. Added.
  const Pos::TType events_offset_time_type = part->type();

  EventList* new_el = nullptr;
  // If we are dragging the part's events with the border, their positions relative to the border don't change.
  // If we are not dragging the events, their positions relative to the border change so we MUST move ALL the events.
  if(events_offset != 0)
  {
    // Compose a complete new list to quickly swap with the existing list.
    const EventList& el = part->events();
    new_el = new EventList();
    for(ciEvent ie = el.cbegin(); ie != el.cend(); ++ie)
    {
      Event e = ie->second.clone();
      if(e.pos().type() == events_offset_time_type)
      {
        // NOTE: Don't alter the offset here or below in the conversions. It messes with the ability of the undo system
        //        to properly undo a movement. It also breaks the rule that all clone parts MUST have the same event times.
        //       Checks and limits should be done before calling this function.
        //if((int64_t)e.posValue() + events_offset < 0)
        //  e.setPosValue(0);
        //else
          e.setPosValue(e.posValue() + events_offset);
      }
      else
      {
        // In case the event and part pos types differ, the event dominates.
        const unsigned int new_part_pos_val = Pos::convert(new_pos, part->type(), e.pos().type());
        const unsigned int old_abs_ev_pos_val = Pos::convert(e.posValue() + new_part_pos_val, e.pos().type(), events_offset_time_type);
        const unsigned int new_abs_ev_pos_val = Pos::convert(old_abs_ev_pos_val + events_offset, events_offset_time_type, e.pos().type());
        const unsigned int new_ev_pos_val = new_abs_ev_pos_val - new_part_pos_val;
        e.setPosValue(new_ev_pos_val);
      }
      new_el->add(e);
    }
  }

  // If we are dragging the part's events with the border, we must update the midi controller cache.
  // If we are not dragging the events, their absolute positions don't change so there should be no need to update the cache.
  // First half of the midi controller cache update:
  removePartPortCtrlEvents(part, part->track());

  add(PendingOperationItem(ip, new_pos, new_len, new_el, PendingOperationItem::ModifyPartStart));

  // Second half of the midi controller cache update:
  // The operation will catch and ignore events which are outside of the part.
  // In case the new_pos and events_offset types differ, the events_offset dominates.
  const unsigned int new_cache_offset = 
    Pos::convert(events_offset + Pos::convert(new_pos, part->type(), events_offset_time_type),
                 events_offset_time_type, Pos::TICKS);
  addPartPortCtrlEvents(part, new_cache_offset, part->lenValue(), part->track());
}

void PendingOperationList::modifyPartLengthOperation(
  Part* part, unsigned int new_len, int64_t events_offset,
  Pos::TType /*events_offset_time_type*/)
{
  if(!part->track())
    return;
    
  // REMOVE Tim. wave. Added.
  const Pos::TType events_offset_time_type = part->type();

  EventList* new_el = nullptr;
  // If we are dragging the part's events with the border, their positions relative to the border change so we MUST move ALL the events.
  // If we are not dragging the events, their positions relative to the border don't change.
  if(events_offset != 0)
  {
    // Compose a complete new list to quickly swap with the existing list.
    const EventList& el = part->events();
    new_el = new EventList();
    for(ciEvent ie = el.cbegin(); ie != el.cend(); ++ie)
    {
      Event e = ie->second.clone();
      if(e.pos().type() == events_offset_time_type)
      {
        // NOTE: Don't alter the offset here or below in the conversions. It messes with the ability of the undo system
        //        to properly undo a movement. It also breaks the rule that all clone parts MUST have the same event times.
        //       Checks and limits should be done before calling this function.
        //if((int64_t)e.posValue() + events_offset < 0)
        //  e.setPosValue(0);
        //else
          e.setPosValue(e.posValue() + events_offset);
      }
      else
      {
        // In case the event and part pos types differ, the event dominates.
        const unsigned int part_pos_val = part->posValue(e.pos().type());
        const unsigned int old_abs_ev_pos_val = Pos::convert(e.posValue() + part_pos_val, e.pos().type(), events_offset_time_type);
        const unsigned int new_abs_ev_pos_val = Pos::convert(old_abs_ev_pos_val + events_offset, events_offset_time_type, e.pos().type());
        const unsigned int new_ev_pos_val = new_abs_ev_pos_val - part_pos_val;
        e.setPosValue(new_ev_pos_val);
      }
      new_el->add(e);
    }
  }

  // If we are dragging the part's events with the border, we must update the midi controller cache.
  // If we are not dragging the events, their absolute positions don't change so there should be no need to update the cache.
  // First half of the midi controller cache update:
  removePartPortCtrlEvents(part, part->track());

  add(PendingOperationItem(part, new_len, new_el, PendingOperationItem::ModifyPartLength));

  // Second half of the midi controller cache update:
  // The operation will catch and ignore events which are outside of the part.
  // In case the new_pos and events_offset types differ, the events_offset dominates.
  const unsigned int new_cache_offset = 
    Pos::convert(events_offset + part->posValue(events_offset_time_type), events_offset_time_type, Pos::TICKS);

  addPartPortCtrlEvents(part, new_cache_offset, part->lenValue(), part->track());
}

bool PendingOperationList::changeEventPropertiesOperation(
  Part* part,
  EventID_t eventID,
  unsigned oldPos,
  unsigned pos,
  unsigned len,
  int spos,
  bool doPortCtrls,
  bool doClonePortCtrls)
{
  Part* p = part;
  do
  {
    // This will find the event even if it has been modified.
    // As long as the IDs AND the position are the same, it's a match.
    // TODO: Support different event types? Would require finding on the eventID only,
    //        and converting the given position type to the target event type.
    iEvent ie = p->nonconst_events().findId(oldPos, eventID);

    if(ie != p->nonconst_events().end())
    {
      // Use the actual old found event, not the given oldEvent.
      const Event& e = ie->second;
      if(add(PendingOperationItem(&p->nonconst_events(), ie, pos, len, spos, PendingOperationItem::ModifyEventProperties)))
      {
        if(doPortCtrls && (doClonePortCtrls || (!doClonePortCtrls && p == part)))
          modifyPartPortCtrlEvents(e, pos, e.dataB(), p);  // Port controller values.
      }
    }

    p = p->nextClone();
  }
  while(p != part);

  return true;
}

//---------------------------------------------------------
//   addTrackAuxSendOperation
//---------------------------------------------------------

void PendingOperationList::addTrackAuxSendOperation(AudioTrack *atrack, int n)
      {
      AuxSendValueList *vl = atrack->getAuxSendValueList();
      const int nn = vl->size();
      for (int i = nn; i < n; ++i)
            add(PendingOperationItem(vl, 0.0, PendingOperationItem::AddAuxSendValue));
      }

//---------------------------------------------------------
//   TrackMidiCtrlRemapOperation
//---------------------------------------------------------

void TrackMidiCtrlRemapOperation(
  MidiTrack *mtrack, int index, int newPort, int newChan, int newNote, MidiCtrlValRemapOperation* rmop)
{
  const int out_port = mtrack->outPort();
  const int out_chan = mtrack->outChannel();

  if(mtrack->type() != Track::DRUM || out_port < 0 || out_port >= MusECore::MIDI_PORTS)
    return;

  // Default to track port if -1 and track channel if -1.
  if(newPort == -1)
    newPort = out_port;

  if(newChan == -1)
    newChan = out_chan;

  MidiPort* trackmp = &MusEGlobal::midiPorts[out_port];

  const DrumMap *drum_map = mtrack->drummap();
  
  int dm_ch = drum_map[index].channel;
  if(dm_ch == -1)
    dm_ch = out_chan;
  int dm_port = drum_map[index].port;
  if(dm_port == -1)
    dm_port = out_port;
  MidiPort* dm_mp = &MusEGlobal::midiPorts[dm_port];

  MidiCtrlValListList* dm_mcvll = dm_mp->controller();
  MidiCtrlValList* v_mcvl;
  int v_ch, v_ctrl, v_idx;
  for(iMidiCtrlValList idm_mcvl = dm_mcvll->begin(); idm_mcvl != dm_mcvll->end(); ++idm_mcvl)
  {
    v_ch = idm_mcvl->first >> 24;
    if(v_ch != dm_ch)
      continue;
    v_mcvl = idm_mcvl->second;
    v_ctrl = v_mcvl->num();

    // Is it a drum controller, according to the track port's instrument?
    if(!trackmp->drumController(v_ctrl))
      continue;

    v_idx = v_ctrl & 0xff;
    if(v_idx != drum_map[index].anote)
      continue;

    // Does this midi control value list need to be changed (values moved etc)?
    iMidiCtrlVal imcv = v_mcvl->begin();
    for( ; imcv != v_mcvl->end(); ++imcv)
    {
      const MidiCtrlVal& mcv = imcv->second;
      if(mcv.part && mcv.part->track() == mtrack)
        break;
    }
    if(imcv != v_mcvl->end())
    {
      // A contribution from a part on this track was found.
      // We must compose a new list, or get an existing one and schedule the existing
      //  one for iterator erasure and pointer deletion.
      // Add the erase iterator. Add will ignore if the erase iterator already exists.
      rmop->_midiCtrlValLists2bErased.add(dm_port, idm_mcvl);
      // Insert the delete pointer. Insert will ignore if the delete pointer already exists.
      rmop->_midiCtrlValLists2bDeleted.insert(v_mcvl);

      MidiCtrlValListList* op_mcvll;
      iMidiCtrlValLists2bAdded_t imcvla = rmop->_midiCtrlValLists2bAdded.find(dm_port);
      if(imcvla == rmop->_midiCtrlValLists2bAdded.end())
      {
        op_mcvll = new MidiCtrlValListList();
        rmop->_midiCtrlValLists2bAdded.insert(MidiCtrlValLists2bAddedInsertPair_t(dm_port, op_mcvll));
      }
      else
        op_mcvll = imcvla->second;

      MidiCtrlValList* op_mcvl;
      iMidiCtrlValList imcvl = op_mcvll->find(dm_ch, v_ctrl);
      if(imcvl == op_mcvll->end())
      {
        op_mcvl = new MidiCtrlValList(v_ctrl);
        op_mcvll->add(dm_ch, op_mcvl);
        // Assign the contents of the original list to the new list.
        *op_mcvl = *v_mcvl;
      }
      else
        op_mcvl = imcvl->second;

      // Remove from the list any contributions from this track.
      iMidiCtrlVal iopmcv = op_mcvl->begin();
      for( ; iopmcv != op_mcvl->end(); )
      {
        const MidiCtrlVal& mcv = iopmcv->second;
        if(mcv.part && mcv.part->track() == mtrack)
        {
          iMidiCtrlVal iopmcv_save = iopmcv;
          ++iopmcv_save;
          op_mcvl->erase(iopmcv);
          iopmcv = iopmcv_save;
        }
        else
          ++iopmcv;
      }
    }

    // We will be making changes to the list pointed to by the new settings.
    // We must schedule the existing one for iterator erasure and pointer deletion.
    MidiPort* dm_mp_new = &MusEGlobal::midiPorts[newPort];
    MidiCtrlValListList* dm_mcvll_new = dm_mp_new->controller();
    MidiCtrlValList* v_mcvl_new = 0;
    const int v_ctrl_new = (v_ctrl & ~0xff) | newNote;
    iMidiCtrlValList idm_mcvl_new = dm_mcvll_new->find(newChan, v_ctrl_new);
    if(idm_mcvl_new != dm_mcvll_new->end())
    {
      v_mcvl_new = idm_mcvl_new->second;
      // Add the erase iterator. Add will ignore if the erase iterator already exists.
      rmop->_midiCtrlValLists2bErased.add(newPort, idm_mcvl_new);
      // Insert the delete pointer. Insert will ignore if the delete pointer already exists.
      rmop->_midiCtrlValLists2bDeleted.insert(v_mcvl_new);
    }

    // Create a new list of lists, or get an existing one.
    MidiCtrlValListList* op_mcvll_new;
    iMidiCtrlValLists2bAdded_t imcvla_new = rmop->_midiCtrlValLists2bAdded.find(newPort);
    if(imcvla_new == rmop->_midiCtrlValLists2bAdded.end())
    {
      op_mcvll_new = new MidiCtrlValListList();
      rmop->_midiCtrlValLists2bAdded.insert(MidiCtrlValLists2bAddedInsertPair_t(newPort, op_mcvll_new));
    }
    else
      op_mcvll_new = imcvla_new->second;

    // Compose a new list for replacement, or get an existing one.
    MidiCtrlValList* op_mcvl_new;
    iMidiCtrlValList imcvl_new = op_mcvll_new->find(newChan, v_ctrl_new);
    if(imcvl_new == op_mcvll_new->end())
    {
      op_mcvl_new = new MidiCtrlValList(v_ctrl_new);
      op_mcvll_new->add(newChan, op_mcvl_new);
      // Assign the contents of the original list to the new list.
      if(v_mcvl_new)
        *op_mcvl_new = *v_mcvl_new;
    }
    else
      op_mcvl_new = imcvl_new->second;

    // Add to the list any contributions from this track.
    for(ciMidiCtrlVal imcv_new = v_mcvl->begin(); imcv_new != v_mcvl->end(); ++imcv_new)
    {
      const MidiCtrlVal& mcv = imcv_new->second;
      if(mcv.part && mcv.part->track() == mtrack)
      {
        op_mcvl_new->addMCtlVal(imcv_new->first, mcv.val, mcv.part);
      }
    }
  }
}

} // namespace MusECore
  
