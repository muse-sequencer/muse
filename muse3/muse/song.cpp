//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: song.cpp,v 1.59.2.52 2009/12/15 03:39:58 terminator356 Exp $
//
//  (C) Copyright 2000-2004 Werner Schweer (ws@seh.de)
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

#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <iostream>

#include <QAction>
#include <QDir>
#include <QMenu>
#include <QMessageBox>
#include <QPoint>
#include <QString>
#include <QTextStream>
#include <QProcess>
#include <QByteArray>
#include <QProgressDialog>
#include <QList>

#include "app.h"
#include "driver/jackmidi.h"
#include "driver/alsamidi.h"
#include "song.h"
#include "track.h"
#include "part.h"

#include "undo.h"
#include "key.h"
#include "globals.h"
#include "event.h"
#include "drummap.h"
#include "marker/marker.h"
#include "synth.h"
#include "audio.h"
#include "mididev.h"
#include "amixer.h"
#include "midiseq.h"
#include "audiodev.h"
#include "gconfig.h"
#include "sync.h"
#include "midictrl.h"
#include "menutitleitem.h"
#include "midi_audio_control.h"
#include "tracks_duplicate.h"
#include "midi.h"
#include "sig.h"
#include "keyevent.h"
#ifndef _WIN32
#include <sys/wait.h>
#endif
#include "tempo.h"
#include "route.h"
#include "strntcpy.h"

// For debugging output: Uncomment the fprintf section.
#define ERROR_TIMESTRETCH(dev, format, args...)  fprintf(dev, format, ##args)
#define ERROR_WAVE(dev, format, args...) fprintf(dev, format, ##args)
#define INFO_WAVE(dev, format, args...) // fprintf(dev, format, ##args)

// Undefine if and when multiple output routes are added to midi tracks.
#define _USE_MIDI_TRACK_SINGLE_OUT_PORT_CHAN_

namespace MusEGlobal {
MusECore::Song* song = 0;
}

namespace MusECore {

extern void clearMidiTransforms();
extern void clearMidiInputTransforms();

//---------------------------------------------------------
//   Song
//---------------------------------------------------------

Song::Song(const char* name)
   :QObject(0)
      {
      setObjectName(name);

      _ipcInEventBuffers = new LockFreeMPSCRingBuffer<MidiPlayEvent>(16384);
      _ipcOutEventBuffers = new LockFreeMPSCRingBuffer<MidiPlayEvent>(16384);
  
      _fCpuLoad = 0.0;
      _fDspLoad = 0.0;
      _xRunsCount = 0;
      
      _arrangerRaster     = 0; // Set to measure, the same as Arranger initial value. Arranger snap combo will set this.
      noteFifoSize   = 0;
      noteFifoWindex = 0;
      noteFifoRindex = 0;
      undoList     = new UndoList(true);  // "true" means "this is an undoList",
      redoList     = new UndoList(false); // "false" means "redoList"
      _markerList  = new MarkerList;
      _globalPitchShift = 0;
      bounceTrack = NULL;
      bounceOutput = NULL;
      showSongInfo=true;
      clearDrumMap(); // One-time only early init
      clear(false);
      }

//---------------------------------------------------------
//   Song
//---------------------------------------------------------

Song::~Song()
      {
      delete undoList;
      delete redoList;
      delete _markerList;
      if(_ipcOutEventBuffers)
        delete _ipcOutEventBuffers;
      if(_ipcInEventBuffers)
        delete _ipcInEventBuffers;
      }

//---------------------------------------------------------
//   putEvent
//---------------------------------------------------------

void Song::putEvent(int pv)
      {
      if (noteFifoSize < REC_NOTE_FIFO_SIZE) {
            recNoteFifo[noteFifoWindex] = pv;
            noteFifoWindex = (noteFifoWindex + 1) % REC_NOTE_FIFO_SIZE;
            ++noteFifoSize;
            }
      }

// REMOVE Tim. samplerate. Added. TODO
#if 0
//---------------------------------------------------------
//   setProjectSampleRate
//---------------------------------------------------------

void Song::setProjectSampleRate(int rate)
{
  if(rate != MusEGlobal::projectSampleRate)
    // TODO: Do the permanent conversion.
    convertProjectSampleRate(rate);
  
  // Now set the rate.
  MusEGlobal::projectSampleRate;
}

//---------------------------------------------------------
//   projectSampleRateDiffers
//---------------------------------------------------------

bool Song::projectSampleRateDiffers() const
{
  return MusEGlobal::projectSampleRate != MusEGlobal::sampleRate;
}

//---------------------------------------------------------
//   projectSampleRateRatio
//---------------------------------------------------------

double Song::projectSampleRateRatio() const
{
  return (double)MusEGlobal::projectSampleRate / (double)MusEGlobal::sampleRate;
}
#endif

//---------------------------------------------------------
//   setTempo
//    public slot
//---------------------------------------------------------

void Song::setTempo(int newTempo)
      {
      applyOperation(UndoOp(UndoOp::SetTempo, pos[0].tick(), newTempo));
      }

//---------------------------------------------------------
//   setSig
//    called from transport window
//---------------------------------------------------------

void Song::setSig(int z, int n)
      {
            // Add will replace if found. 
            MusEGlobal::song->applyOperation(MusECore::UndoOp(MusECore::UndoOp::AddSig,
                            pos[0].tick(), z, n));
      }

void Song::setSig(const MusECore::TimeSignature& sig)
      {
            // Add will replace if found. 
            MusEGlobal::song->applyOperation(MusECore::UndoOp(MusECore::UndoOp::AddSig,
                            pos[0].tick(), sig.z, sig.n));
      }

//---------------------------------------------------------
//    addNewTrack
//    Called from GUI context
//    If insertAt is valid, inserts before insertAt. Else at the end after all tracks.
//    Besides normal track types, n includes synth menu ids from populateAddTrack()
//---------------------------------------------------------

Track* Song::addNewTrack(QAction* action, Track* insertAt)
{
    int n = action->data().toInt();
    // Ignore negative numbers since this slot could be called by a menu or list etc. passing -1.
    if(n < 0)
      return 0;                  
    
    // Synth sub-menu id?
    if(n >= MENU_ADD_SYNTH_ID_BASE)
    {
      n -= MENU_ADD_SYNTH_ID_BASE;
      int ntype = n / MENU_ADD_SYNTH_ID_BASE;
      if(ntype >= Synth::SYNTH_TYPE_END)
        return 0;

      // if we ever support Wine VSTs through some other means than through dssi-vst this must be adapted
      if (ntype == MusECore::Synth::VST_SYNTH)
        ntype=MusECore::Synth::DSSI_SYNTH;
      if (ntype == MusECore::Synth::LV2_EFFECT)
        ntype=MusECore::Synth::LV2_SYNTH; // the LV2_EFFECT is a specialization used in the menu only, we reassign it to regular LV2_SYNTH

      n %= MENU_ADD_SYNTH_ID_BASE;
      if(n >= (int)MusEGlobal::synthis.size())
        return 0;
        
      if (MusEGlobal::debugMsg)
        fprintf(stderr, "Song::addNewTrack synth: type:%d idx:%d class:%s label:%s\n", ntype, n, MusEGlobal::synthis[n]->baseName().toLatin1().constData(), MusEGlobal::synthis[n]->name().toLatin1().constData());  
        
      SynthI* si = createSynthI(MusEGlobal::synthis[n]->baseName(), MusEGlobal::synthis[n]->name(), (Synth::Type)ntype, insertAt);
      if(!si)
        return 0;
      
      if (MusEGlobal::config.unhideTracks) SynthI::setVisible(true);

      // Add instance last in midi device list.
      for (int i = 0; i < MusECore::MIDI_PORTS; ++i) 
      {
        MidiPort* port  = &MusEGlobal::midiPorts[i];
        MidiDevice* dev = port->device();
        if (dev==0) 
        {
          // This is a brand new instance. Set the instrument as well for convenience.
          MusEGlobal::audio->msgSetMidiDevice(port, si, si);
          // Save settings. Use simple version - do NOT set style or stylesheet, this has nothing to do with that.
          MusEGlobal::muse->changeConfig(true);
          if (SynthI::visible()) {
            selectAllTracks(false);
            si->setSelected(true);
            update();
          }
          return si;
        }
      }
      if (SynthI::visible()) {
        selectAllTracks(false);
        si->setSelected(true);
        update(SC_TRACK_SELECTION);
      }
      return si;
    }  
    // Normal track.
    else
    {
      // Ignore AUDIO_SOFTSYNTH (or anything greater, to allow for other entries in some menu), 
      //  now that we have it as the synth menu id, since addTrack doesn't like it.
      if((Track::TrackType)n >= Track::AUDIO_SOFTSYNTH)
        return 0;
      
      Track* t = addTrack((Track::TrackType)n, insertAt);
      if (t->isVisible()) {
        selectAllTracks(false);
        t->setSelected(true);
        update(SC_TRACK_SELECTION);
      }
      return t;
    }  
}          
          
      
//---------------------------------------------------------
//    addTrack
//    called from GUI context
//    type is track type
//    If insertAt is valid, inserts before insertAt. Else at the end after all tracks.
//---------------------------------------------------------

Track* Song::addTrack(Track::TrackType type, Track* insertAt)
      {
      Track* track = 0;
      int lastAuxIdx = _auxs.size();
      switch(type) {
            case Track::MIDI:
                  track = new MidiTrack();
                  track->setType(Track::MIDI);
                  if (MusEGlobal::config.unhideTracks) MidiTrack::setVisible(true);
                  break;
            case Track::NEW_DRUM:
                  track = new MidiTrack();
                  track->setType(Track::NEW_DRUM);
                  ((MidiTrack*)track)->setOutChannel(9);
                  break;
            case Track::DRUM:
                  track = new MidiTrack();
                  track->setType(Track::DRUM);
                  ((MidiTrack*)track)->setOutChannel(9);
                  if (MusEGlobal::config.unhideTracks) MidiTrack::setVisible(true);
                  break;
            case Track::WAVE:
                  track = new MusECore::WaveTrack();
                  ((AudioTrack*)track)->addAuxSend(lastAuxIdx);
                  if (MusEGlobal::config.unhideTracks) WaveTrack::setVisible(true);
                  break;
            case Track::AUDIO_OUTPUT:
                  track = new AudioOutput();
                  if (MusEGlobal::config.unhideTracks) AudioOutput::setVisible(true);
                  break;
            case Track::AUDIO_GROUP:
                  track = new AudioGroup();
                  ((AudioTrack*)track)->addAuxSend(lastAuxIdx);
                  if (MusEGlobal::config.unhideTracks) AudioGroup::setVisible(true);
                  break;
            case Track::AUDIO_AUX:
                  track = new AudioAux();
                  if (MusEGlobal::config.unhideTracks) AudioAux::setVisible(true);
                  break;
            case Track::AUDIO_INPUT:
                  track = new AudioInput();
                  ((AudioTrack*)track)->addAuxSend(lastAuxIdx);
                  if (MusEGlobal::config.unhideTracks) AudioInput::setVisible(true);
                  break;
            case Track::AUDIO_SOFTSYNTH:
                  fprintf(stderr, "not implemented: Song::addTrack(SOFTSYNTH)\n");
                  // ((AudioTrack*)track)->addAuxSend(lastAuxIdx);
                  break;
            default:
                  fprintf(stderr, "THIS SHOULD NEVER HAPPEN: Song::addTrack() illegal type %d. returning NULL.\n"
                         "save your work if you can and expect soon crashes!\n", type);
                  return NULL;
            }
      track->setDefaultName();
      
      int idx = insertAt ? _tracks.index(insertAt) : -1;
      
      // Add default track <-> midiport routes. 
      if(track->isMidiTrack()) 
      {
        MidiTrack* mt = (MidiTrack*)track;
        int c;
        bool defOutFound = false;                /// TODO: Remove this if and when multiple output routes supported.
        const int chmask = (1 << MusECore::MUSE_MIDI_CHANNELS) - 1;
        for(int i = 0; i < MusECore::MIDI_PORTS; ++i)
        {
          MidiPort* mp = &MusEGlobal::midiPorts[i];
          if(!mp->device())  // Only if device is valid.
            continue;
          if(mp->device()->rwFlags() & 0x02) // Readable
          {
            c = mp->defaultInChannels();
            if(c)
            {
              // All channels set or Omni? Use an Omni route:
              if(c == -1 || c == chmask)
                track->inRoutes()->push_back(Route(i));
              else
              // Add individual channels:  
              for(int ch = 0; ch < MusECore::MUSE_MIDI_CHANNELS; ++ch)
              {
                if(c & (1 << ch))
                  track->inRoutes()->push_back(Route(i, ch));
              }
            }
          }
          
          if(mp->device()->rwFlags() & 0x01) // Writeable
          {
            if(!defOutFound)                       ///
            {
              c = mp->defaultOutChannels();
              if(c)
              {
                
#ifdef _USE_MIDI_TRACK_SINGLE_OUT_PORT_CHAN_
                if(c == -1)
                  c = 1;  // Just to be safe, shouldn't happen, default to channel 0.
                for(int ch = 0; ch < MusECore::MUSE_MIDI_CHANNELS; ++ch)   
                {
                  if(c & (1 << ch))
                  {
                    defOutFound = true;
                    mt->setOutPort(i);
                    if(type != Track::DRUM && type != Track::NEW_DRUM)  // Leave drum tracks at channel 10.
                      mt->setOutChannel(ch);
                    //updateFlags |= SC_ROUTE;
                    break;               
                  }
                }
#else 
                // All channels set or Omni? Use an Omni route:
                if(c == -1 || c == chmask)
                  track->outRoutes()->push_back(Route(i));
                else
                // Add individual channels:  
                for(int ch = 0; ch < MusECore::MUSE_MIDI_CHANNELS; ++ch)
                {
                  if(c & (1 << ch))
                    track->outRoutes()->push_back(Route(i, ch));
                }
#endif
              }
            }  
          }
        }

        if (!defOutFound) { // no default port found
          // set it to the port with highest number

          for(int i = MusECore::MIDI_PORTS-1; i >= 0; --i) {

            MidiPort* mp = &MusEGlobal::midiPorts[i];

            if (mp->device() != NULL) {

              mt->setOutPort(i);
              break;
            }
          }
        }
      }
                  
      //
      //  add default route to master
      //
      OutputList* ol = MusEGlobal::song->outputs();
      if (!ol->empty()) {
            AudioOutput* ao = ol->front();
            switch(type) {
                  case Track::WAVE:
                  case Track::AUDIO_AUX:
                        //fprintf(stderr, "Song::addTrack(): WAVE or AUDIO_AUX type:%d name:%s pushing default route to master\n", track->type(), track->name().toLatin1().constData());  
                        track->outRoutes()->push_back(Route(ao));
                        break;
                  // It should actually never get here now, but just in case.
                  case Track::AUDIO_SOFTSYNTH:
                        track->outRoutes()->push_back(Route(ao));
                        break;
                  default:
                        break;
                  }
            }
            
      applyOperation(UndoOp(UndoOp::AddTrack, idx, track));
            
      return track;
      }

//---------------------------------------------------------
//    duplicateTracks
//    Called from GUI context
//---------------------------------------------------------

void Song::duplicateTracks()
{
  // Make a temporary copy.  
  TrackList tl = _tracks;  

  int audio_found = 0;
  int midi_found = 0;
  int drum_found = 0;
  int new_drum_found = 0;
  for(iTrack it = tl.begin(); it != tl.end(); ++it) 
    if((*it)->selected()) 
    {
      Track::TrackType type = (*it)->type(); 
      if(type == Track::DRUM)
        ++drum_found;
      else if(type == Track::NEW_DRUM)
        ++new_drum_found;
      else if(type == Track::MIDI)
        ++midi_found;
      else
        ++audio_found;
    }
 
  if(audio_found == 0 && midi_found == 0 && drum_found == 0 && new_drum_found==0)
    return;
  
  MusEGui::DuplicateTracksDialog* dlg = new MusEGui::DuplicateTracksDialog(audio_found, midi_found, drum_found, new_drum_found);

  int rv = dlg->exec();
  if(rv == QDialog::Rejected)
  {
    delete dlg;
    return;
  }
        
  int copies = dlg->copies();

  int flags = Track::ASSIGN_PROPERTIES;
  if(dlg->copyStdCtrls())
    flags |= Track::ASSIGN_STD_CTRLS;
  if(dlg->copyPlugins())
    flags |= Track::ASSIGN_PLUGINS;
  if(dlg->copyPluginCtrls())
    flags |= Track::ASSIGN_PLUGIN_CTRLS;
  if(dlg->allRoutes())
    flags |= Track::ASSIGN_ROUTES;
  if(dlg->defaultRoutes())
    flags |= Track::ASSIGN_DEFAULT_ROUTES;

  // These three are exclusive.
  if(dlg->duplicateParts())
    flags |= Track::ASSIGN_DUPLICATE_PARTS;
  else if(dlg->copyParts())
    flags |= Track::ASSIGN_COPY_PARTS;
  else if(dlg->cloneParts())
    flags |= Track::ASSIGN_CLONE_PARTS;
  
  if(dlg->copyDrumlist())
    flags |= Track::ASSIGN_DRUMLIST;  
  
  delete dlg;
  
  QString track_name;
  int idx;
  int trackno = tl.size();
  
  Undo operations;
  for(TrackList::reverse_iterator it = tl.rbegin(); it != tl.rend(); ++it) 
  {
    Track* track = *it;
    if(track->selected()) 
    {
      track_name = track->name();
      int counter=0;
      int numberIndex=0;
      for(int cp = 0; cp < copies; ++cp)
      {
          Track* new_track = track->clone(flags);

          // assign new names to copied tracks. there is still a gaping hole in the logic
          // making multiple duplicates of multiple tracks still does not produce valid results.
          if (cp == 0) { // retrieve the first index for renaming the following tracks
            numberIndex = new_track->name().lastIndexOf("#");
            if (numberIndex == -1 || numberIndex > track_name.size()) { // according to Qt doc for lastIndexOf it should return -1 when not found
              track_name += " #";                                       // apparently it returns str_size+1 ?! Let's catch both
              numberIndex = track_name.size();
              counter=1;
            }
            else {
              counter = new_track->name().right(new_track->name().size()-numberIndex-1).toInt();
            }
          }
          QString tempName;
          while(true) {
            tempName = track_name.left(numberIndex+1) + QString::number(++counter);
            Track* track = findTrack(tempName);
            if(track == 0)
            {
              new_track->setName(tempName);
              break;
            }
          }

          idx = trackno + cp;
          operations.push_back(MusECore::UndoOp(MusECore::UndoOp::AddTrack, idx, new_track));
      }  
    }
    --trackno;
  }
  
  applyOperationGroup(operations);
}          
      
bool Song::addEventOperation(const Event& event, Part* part, bool do_port_ctrls, bool do_clone_port_ctrls)
{
//   Event ev(event);
  bool added = false;
  Part* p = part;
  while(1)
  {
    // This will find the event even if it has been modified. As long as the IDs AND the position are the same, it's a match.
    // NOTE: Multiple events with the same event base pointer or the same id number, in one event list, are FORBIDDEN.
    //       This precludes using them for 'pattern groups' such as arpeggios or chords. Instead, create a new event type.
    ciEvent ie = p->events().findWithId(event);
    if(ie == p->events().cend()) 
    {
      if(pendingOperations.add(PendingOperationItem(p, event, PendingOperationItem::AddEvent)))
      {
        added = true;
        // Include addition of any corresponding cached controller value.
        // By default, here we MUST include all clones so that in the case of multiple events
        //  at the same position the cache reader can quickly look at each part and if one
        //  is MUTED pick an event from a different unmuted part at that position.
        if(do_port_ctrls && (do_clone_port_ctrls || (!do_clone_port_ctrls && p == part)))
  //         addPortCtrlEvents(ev, p, p->tick(), p->lenTick(), p->track(), pendingOperations);
          addPortCtrlEvents(event, p, p->tick(), p->lenTick(), p->track(), pendingOperations);
      }
    }
    
    p = p->nextClone();
    if(p == part)
      break;
    
//     ev = event.clone(); // Makes a new copy with the same id.
  }
  return added;
}

void Song::changeEventOperation(const Event& oldEvent, const Event& newEvent, Part* part, bool do_port_ctrls, bool do_clone_port_ctrls)
{
  // If position is changed we need to reinsert into the list, and all clone lists.
  Part* p = part;
  do
  {
    // This will find the event even if it has been modified.
    // As long as the IDs AND the position are the same, it's a match.
    iEvent ie = p->nonconst_events().findWithId(oldEvent);
    if(ie == p->nonconst_events().end())
    {
      // The old event was not found. Just go ahead and include the addition of the new event.
      // Make sure the new event doesn't already exist.
      if(p->events().findWithId(newEvent) == p->events().cend())
      {
        if(pendingOperations.add(PendingOperationItem(p, newEvent, PendingOperationItem::AddEvent)))
        {
          if(do_port_ctrls && (do_clone_port_ctrls || (!do_clone_port_ctrls && p == part)))
            addPortCtrlEvents(newEvent, p, p->tick(), p->lenTick(), p->track(), pendingOperations);  // Port controller values.
        }
      }
    }
    else
    {
      // Use the actual old found event, not the given oldEvent.
      const Event& e = ie->second;
      // Go ahead and include deletion of the old event.
      if(pendingOperations.add(PendingOperationItem(p, ie, PendingOperationItem::DeleteEvent)))
      {
        // If the new and old event IDs are the same we bypass looking for the new event
        //  because it hasn't been deleted yet and would always be found.
        // This is safe since the event is deleted then added again.
        // But if the new and old event IDs are not the same we MUST make sure the
        //  new event does not already exist.
        if((newEvent.id() == oldEvent.id() || p->events().findWithId(newEvent) == p->events().cend()) &&
           pendingOperations.add(PendingOperationItem(p, newEvent, PendingOperationItem::AddEvent)))
        {
          if(do_port_ctrls && (do_clone_port_ctrls || (!do_clone_port_ctrls && p == part)))
            modifyPortCtrlEvents(e, newEvent, p, pendingOperations);  // Port controller values.
        }
        else
        {
          // Adding the new event failed.
          // Just go ahead and include removal of the old cached value.
          if(do_port_ctrls && (do_clone_port_ctrls || (!do_clone_port_ctrls && p == part)))
            removePortCtrlEvents(e, p, p->track(), pendingOperations);  // Port controller values.
        }
      }
    }
    
    p = p->nextClone();
  }
  while(p != part);
}

//---------------------------------------------------------
//   deleteEvent
//---------------------------------------------------------

Event Song::deleteEventOperation(const Event& event, Part* part, bool do_port_ctrls, bool do_clone_port_ctrls)
{
  Event p_res, res;
  Part* p = part;
  do
  {
   // This will find the event even if it has been modified.
   // As long as the IDs AND the position are the same, it's a match.
   iEvent ie = p->nonconst_events().findWithId(event);
   if(ie != p->nonconst_events().end())
   {
     const Event& e = ie->second;
     // Prefer to return the event found in the given part's event list, not a clone part's.
     if(p == part)
       p_res = e;
     if(res.empty())
       res = e;

     // Include removal of the event.
     if(pendingOperations.add(PendingOperationItem(p, ie, PendingOperationItem::DeleteEvent)))
     {
       // Include removal of any corresponding cached controller value.
       // By using the found existing event instead of the given one, this allows
       //  us to pre-modify an event - EXCEPT the event's time and ID - before
       //  passing it here. We will find it by ID and delete the event.
       // Also these following cached controller values DEPEND on finding the
       //  ORIGINAL event and cannot find a modified event.
       if(do_port_ctrls && (do_clone_port_ctrls || (!do_clone_port_ctrls && p == part)))
         removePortCtrlEvents(e, p, p->track(), pendingOperations);  // Port controller values.
     }
   }
    
    p = p->nextClone();
  }
  while(p != part);
  
  // Prefer to return the event found in the given part's event list, not a clone part's.
  if(!p_res.empty())
    return p_res;
  
  return res;
}

//---------------------------------------------------------
//   selectEvent
//---------------------------------------------------------

void Song::selectEvent(Event& event, Part* part, bool select)
{
  Part* p = part;
  do
  {
    iEvent ie = p->nonconst_events().findWithId(event);
    if(ie == p->nonconst_events().end()) 
    {
      // This can be normal for some (redundant) operations.
      if(MusEGlobal::debugMsg)
	fprintf(stderr, "Song::selectEvent event not found in part:%s size:%zd\n", p->name().toLatin1().constData(), p->nonconst_events().size());
    }
    else
      ie->second.setSelected(select);
    p = p->nextClone();
  } 
  while(p != part);
}

//---------------------------------------------------------
//   selectAllEvents
//---------------------------------------------------------

void Song::selectAllEvents(Part* part, bool select)
{
  Part* p = part;
  do
  {
    EventList& el = p->nonconst_events();
    for(iEvent ie = el.begin(); ie != el.end(); ++ie)
      ie->second.setSelected(select);
    p = p->nextClone();
  } 
  while(p != part);
}

//---------------------------------------------------------
//   remapPortDrumCtrlEvents
//   Called when drum map anote, channel, or port is changed.
//---------------------------------------------------------

void Song::remapPortDrumCtrlEvents(int mapidx, int newnote, int newchan, int newport)
{
  if(mapidx == -1)
   return;
   
  for(ciMidiTrack it = _midis.begin(); it != _midis.end(); ++it) 
  {
    MidiTrack* mt = *it;
    if(mt->type() != Track::DRUM)
      continue;
      
    MidiPort* trackmp = &MusEGlobal::midiPorts[mt->outPort()];
    const PartList* pl = mt->cparts();
    for(ciPart ip = pl->begin(); ip != pl->end(); ++ip) 
    {
      MidiPart* part = (MidiPart*)(ip->second);
      const EventList& el = part->events();
      for(ciEvent ie = el.begin(); ie != el.end(); ++ie)
      {
        const Event& ev = ie->second;
        if(ev.type() != Controller)
          continue;
          
        int cntrl = ev.dataA();
        int val = ev.dataB();
        
        // Is it a drum controller event, according to the track port's instrument?
        MidiController* mc = trackmp->drumController(cntrl);
        if(!mc)
          continue;
          
        int note = cntrl & 0x7f;
        // Does the index match?
        if(note == mapidx)
        {
          int tick = ev.tick() + part->tick();

          if(mt->type() == Track::DRUM)
          {
            // Default to track port if -1 and track channel if -1.
            int ch = MusEGlobal::drumMap[note].channel;
            if(ch == -1)
              ch = mt->outChannel();
            int port = MusEGlobal::drumMap[note].port;
            if(port == -1)
              port = mt->outPort();
            MidiPort* mp = &MusEGlobal::midiPorts[port];
            cntrl = (cntrl & ~0xff) | MusEGlobal::drumMap[note].anote;
            // Remove the port controller value.
            mp->deleteController(ch, tick, cntrl, val, part);

            if(newnote != -1 && newnote != MusEGlobal::drumMap[note].anote)
              cntrl = (cntrl & ~0xff) | newnote;
            if(newchan != -1 && newchan != ch)
              ch = newchan;
            if(newport != -1 && newport != port)
              port = newport;
            mp = &MusEGlobal::midiPorts[port];
            // Add the port controller value.
            mp->setControllerVal(ch, tick, cntrl, val, part);
          }
        }
      }
    }  
  }
}

//---------------------------------------------------------
//   changeAllPortDrumCtlEvents
//   add true: add events. false: remove events
//   drumonly true: Do drum controller events ONLY. false (default): Do ALL controller events.
//---------------------------------------------------------

void Song::changeAllPortDrumCtrlEvents(bool add, bool drumonly)
{
  int ch, trackch, cntrl, tick, val;
  MidiPort* mp, *trackmp;
  for(ciMidiTrack it = _midis.begin(); it != _midis.end(); ++it) 
  {
    MidiTrack* mt = *it;
    if(mt->type() != Track::DRUM)
      continue;
      
    trackmp = &MusEGlobal::midiPorts[mt->outPort()];
    trackch = mt->outChannel();
    const PartList* pl = mt->cparts();
    for(ciPart ip = pl->begin(); ip != pl->end(); ++ip) 
    {
      MidiPart* part = (MidiPart*)(ip->second);
      for(ciEvent ie = part->events().begin(); ie != part->events().end(); ++ie)
      {
        const Event& ev = ie->second;

        if(ev.type() != Controller)
          continue;
          
        cntrl = ev.dataA();
        val = ev.dataB();
        mp = trackmp;
        ch = trackch;
        
        // Is it a drum controller event, according to the track port's instrument?
        if(trackmp->drumController(cntrl))
        {
          if(mt->type() == Track::DRUM)
          {
            int note = cntrl & 0x7f;
            // Default to track port if -1 and track channel if -1.
            if(MusEGlobal::drumMap[note].channel != -1)
              ch = MusEGlobal::drumMap[note].channel;
            if(MusEGlobal::drumMap[note].port != -1)
              mp = &MusEGlobal::midiPorts[MusEGlobal::drumMap[note].port];
            cntrl = (cntrl & ~0xff) | MusEGlobal::drumMap[note].anote;
          }
        }
        else
        {  
          if(drumonly)
            continue;
        }
        
        tick = ev.tick() + part->tick();
        
        if(add)
          // Add the port controller value.
          mp->setControllerVal(ch, tick, cntrl, val, part);
        else  
          // Remove the port controller value.
          mp->deleteController(ch, tick, cntrl, val, part);
      }
    }  
  }
}

//---------------------------------------------------------
//   cmdAddRecordedEvents
//    add recorded Events into part
//---------------------------------------------------------

void Song::cmdAddRecordedEvents(MidiTrack* mt, const EventList& events, unsigned startTick, Undo& operations)
      {
      if (events.empty()) {
            if (MusEGlobal::debugMsg)
                  fprintf(stderr, "no events recorded\n");
            return;
            }
      ciEvent s;
      ciEvent e;
      unsigned endTick;

      if((MusEGlobal::audio->loopCount() > 0 && startTick > lPos().tick()) || (punchin() && startTick < lPos().tick()))
      {
            startTick = lpos();
            s = events.lower_bound(startTick);
      }
      else 
      {
            s = events.begin();
      }
      
      // search for last noteOff:
      endTick = 0;
      for (ciEvent i = events.begin(); i != events.end(); ++i) {
            Event ev   = i->second;
            unsigned l = ev.endTick();
            if (l > endTick)
                  endTick = l;
            }

      if((MusEGlobal::audio->loopCount() > 0) || (punchout() && endTick > rPos().tick()) )
      {
            endTick = rpos();
            e = events.lower_bound(endTick);
      }
      else
            e = events.end();

      if (startTick > endTick) {
            if (MusEGlobal::debugMsg)
                  fprintf(stderr, "no events in record area\n");
            return;
            }

      //---------------------------------------------------
      //    if startTick points into a part,
      //          record to that part
      //    else
      //          create new part
      //---------------------------------------------------

      PartList* pl = mt->parts();
      const MidiPart* part = 0;
      iPart ip;
      for (ip = pl->begin(); ip != pl->end(); ++ip) {
            part = (MidiPart*)(ip->second);
            unsigned partStart = part->tick();
            unsigned partEnd   = part->endTick();
            if (startTick >= partStart && startTick < partEnd)
                  break;
            }
      if (ip == pl->end()) {
            if (MusEGlobal::debugMsg)
                  fprintf(stderr, "create new part for recorded events\n");
            // create new part
            MidiPart* newpart;
            newpart      = new MidiPart(mt);
            
            // Round the start down using the Arranger part snap raster value. 
            startTick = MusEGlobal::sigmap.raster1(startTick, arrangerRaster());
            // Round the end up using the Arranger part snap raster value. 
            endTick   = MusEGlobal::sigmap.raster2(endTick, arrangerRaster());
            
            newpart->setTick(startTick);
            newpart->setLenTick(endTick - startTick);
            newpart->setName(mt->name());
            // copy events
            for (ciEvent i = s; i != e; ++i) {
                  const Event& old = i->second;
                  Event event = old.clone();
                  event.setTick(old.tick() - startTick);
                  // addEvent also adds port controller values. So does msgAddPart, below. Let msgAddPart handle them.
                  //addEvent(event, part);
                  if(newpart->events().find(event) == newpart->events().end())
                    newpart->addEvent(event);
                  }
            operations.push_back(MusECore::UndoOp(MusECore::UndoOp::AddPart, newpart));
            return;
            }

      unsigned partTick = part->tick();
      if (endTick > part->endTick()) {
            // Determine new part length...
            endTick = 0;
            for (ciEvent i = s; i != e; ++i) {
                  const Event& event = i->second;
                  unsigned tick = event.tick() - partTick + event.lenTick();
                  if (endTick < tick)
                        endTick = tick;
                  }
            
            // Round the end up (again) using the Arranger part snap raster value. 
            endTick   = MusEGlobal::sigmap.raster2(endTick, arrangerRaster());
            
            operations.push_back(UndoOp(UndoOp::ModifyPartLength, part, part->lenValue(), endTick, Pos::TICKS));
      }
            

      if (_recMode == REC_REPLACE) {
            ciEvent si = part->events().lower_bound(startTick - part->tick());
            ciEvent ei = part->events().lower_bound(endTick   - part->tick());

            for (ciEvent i = si; i != ei; ++i) {
                  const Event& event = i->second;
                  // Indicate that controller values and clone parts were handled.
                  operations.push_back(UndoOp(UndoOp::DeleteEvent, event, part, true, true));
            }
      }
      for (ciEvent i = s; i != e; ++i) {
            Event event = i->second.clone();
            event.setTick(event.tick() - partTick);
            // Indicate that controller values and clone parts were handled.
            operations.push_back(UndoOp(UndoOp::AddEvent, event, part, true, true));
      }
}

//---------------------------------------------------------
//   cmdAddRecordedWave
//---------------------------------------------------------

void Song::cmdAddRecordedWave(MusECore::WaveTrack* track, MusECore::Pos s, MusECore::Pos e, Undo& operations)
      {
      if (MusEGlobal::debugMsg)
      {
          INFO_WAVE(stderr, "cmdAddRecordedWave - loopCount = %d, punchin = %d",
                    MusEGlobal::audio->loopCount(), punchin());
      }

      // Driver should now be in transport 'stop' mode and no longer pummping the recording wave fifo,
      //  but the fifo may not be empty yet, it's in the prefetch thread.
      // Wait a few seconds for the fifo to be empty, until it has been fully transferred to the
      //  track's recFile sndfile, which is done via Audio::process() sending periodic 'tick' messages
      //  to the prefetch thread to write its fifo to the sndfile, always UNLESS in stop or idle mode.
      // It now sends one final tick message at stop, so we /should/ have all our buffers available here.
      // This GUI thread is notified of the stop condition via the audio thread sending a message
      //  as soon as the state change is read from the driver.
      // NOTE: The fifo scheme is used only if NOT in transport freewheel mode where the data is directly
      //  written to the sndfile and therefore stops immediately when the transport stops and thus is
      //  safe to read here regardless of waiting.
      int tout = 100; // Ten seconds. Otherwise we gotta move on.
      while(track->recordFifoCount() != 0)
      {
        usleep(100000);
        --tout;
        if(tout == 0)
        {
          ERROR_WAVE(stderr, "Song::cmdAddRecordedWave: Error: Timeout waiting for _tempoFifo to empty! Count:%d\n",
                     track->prefetchFifo()->getCount());
          break;
        }
      }

      // It should now be safe to work with the resultant sndfile here in the GUI thread.
      // No other thread should be touching it right now.
      MusECore::SndFileR f = track->recFile();
      if (f.isNull()) {
            ERROR_WAVE(stderr, "cmdAddRecordedWave: no snd file for track <%s>\n",
               track->name().toLocal8Bit().constData());
            return;
            }

      // If externally clocking (and therefore master was forced off),
      //  tempos may have been recorded. We really should temporarily force
      //  the master tempo map on in order to properly determine the ticks below.
      // Else internal clocking, the user decided to record either with or without
      //  master on, so let it be.
      // FIXME: We really should allow the master flag to be on at the same time as
      //  the external sync flag! AFAIR when external sync is on, no part of the app shall
      //  depend on the tempo map anyway, so it should not matter whether it's on or off.
      // If we do that, then we may be able to remove this section and user simply decides
      //  whether master is on/off, because we may be able to use the flag to determine
      //  whether to record external tempos at all, because we may want a switch for it!
      bool master_was_on = MusEGlobal::tempomap.masterFlag();
      if(MusEGlobal::extSyncFlag && !master_was_on)
        MusEGlobal::tempomap.setMasterFlag(0, true);

      if((MusEGlobal::audio->loopCount() > 0 && s.tick() > lPos().tick()) || (punchin() && s.tick() < lPos().tick()))
        s.setTick(lPos().tick());
      // If we are looping, just set the end to the right marker, since we don't know how many loops have occurred.
      // (Fixed: Added Audio::loopCount)
      // Otherwise if punchout is on, limit the end to the right marker.
      if((MusEGlobal::audio->loopCount() > 0) || (punchout() && e.tick() > rPos().tick()) )
        e.setTick(rPos().tick());

      // No part to be created? Delete the rec sound file.
      if(s.frame() >= e.frame())
      {
        QString st = f->path();
        // The function which calls this function already does this immediately after. But do it here anyway.
        track->setRecFile(NULL); // upon "return", f is removed from the stack, the WaveTrack::_recFile's
                                 // counter has dropped by 2 and _recFile will probably deleted then
        remove(st.toLocal8Bit().constData());
        if(MusEGlobal::debugMsg)
        {
          INFO_WAVE(stderr, "Song::cmdAddRecordedWave: remove file %s - startframe=%d endframe=%d\n",
                    st.toLocal8Bit().constData(), s.frame(), e.frame());
        }

        // Restore master flag.
        if(MusEGlobal::extSyncFlag && !master_was_on)
          MusEGlobal::tempomap.setMasterFlag(0, false);

        return;
      }
// REMOVE Tim. Wave. Removed. Probably I should never have done this. It's more annoying than helpful. Look at it another way: Importing a wave DOES NOT do this.
//       // Round the start down using the Arranger part snap raster value.
//       int a_rast = MusEGlobal::song->arrangerRaster();
//       unsigned sframe = (a_rast == 1) ? s.frame() : Pos(MusEGlobal::sigmap.raster1(s.tick(), MusEGlobal::song->arrangerRaster())).frame();
//       // Round the end up using the Arranger part snap raster value.
//       unsigned eframe = (a_rast == 1) ? e.frame() : Pos(MusEGlobal::sigmap.raster2(e.tick(), MusEGlobal::song->arrangerRaster())).frame();
// //       unsigned etick = Pos(eframe, false).tick();
      unsigned sframe = s.frame();
      unsigned eframe = e.frame();

      // Done using master tempo map. Restore master flag.
      if(MusEGlobal::extSyncFlag && !master_was_on)
        MusEGlobal::tempomap.setMasterFlag(0, false);

      f->update();

      MusECore::WavePart* part = new MusECore::WavePart(track);
      part->setFrame(sframe);
      part->setLenFrame(eframe - sframe);
      part->setName(track->name());

      // create Event
      MusECore::Event event(MusECore::Wave);
      event.setSndFile(f);
      // We are done with the _recFile member. Set to zero.
      track->setRecFile(0);

      event.setSpos(0);
      // Since the part start was snapped down, we must apply the difference so that the
      //  wave event tick lines up with when the user actually started recording.
      event.setFrame(s.frame() - sframe);
      // NO Can't use this. SF reports too long samples at first part recorded in sequence. See samples() - funny business with SEEK ?
      //event.setLenFrame(f.samples());
      event.setLenFrame(e.frame() - s.frame());
      part->addEvent(event);

      operations.push_back(UndoOp(UndoOp::AddPart, part));
      }

//---------------------------------------------------------
//   cmdChangeWave
//   called from GUI context
//---------------------------------------------------------

void Song::cmdChangeWave(const Event& original, const QString& tmpfile, unsigned sx, unsigned ex)
      {
      addUndo(UndoOp(UndoOp::ModifyClip,original,tmpfile,sx,ex));
      temporaryWavFiles.push_back(tmpfile);
      }

//---------------------------------------------------------
//   findTrack
//---------------------------------------------------------

Track* Song::findTrack(const Part* part) const
      {
      for (ciTrack t = _tracks.begin(); t != _tracks.end(); ++t) {
            Track* track = *t;
            if (track == 0)
                  continue;
            PartList* pl = track->parts();
            for (iPart p = pl->begin(); p != pl->end(); ++p) {
                  if (part == p->second)
                        return track;
                  }
            }
      return 0;
      }

//---------------------------------------------------------
//   findTrack
//    find track by name
//---------------------------------------------------------

Track* Song::findTrack(const QString& name) const
      {
      for (ciTrack i = _tracks.begin(); i != _tracks.end(); ++i) {
            if ((*i)->name() == name)
                  return *i;
            }
      return 0;
      }

//---------------------------------------------------------
//   setLoop
//    set transport loop flag
//---------------------------------------------------------

void Song::setLoop(bool f)
      {
      if (loopFlag != f) {
            loopFlag = f;
            MusEGlobal::loopAction->setChecked(loopFlag);
            emit loopChanged(loopFlag);
            }
      }

//---------------------------------------------------------
//   clearTrackRec
//---------------------------------------------------------
void Song::clearTrackRec()
{
  // This is a minor operation easily manually undoable. Let's not clog the undo list with it.
  MusECore::PendingOperationList operations;
  for(iTrack it = tracks()->begin(); it != tracks()->end(); ++it)
  {
    if(!(*it)->setRecordFlag1(false))
    {
      //continue;
    }
    operations.add(MusECore::PendingOperationItem((*it), false, MusECore::PendingOperationItem::SetTrackRecord));
  }
  MusEGlobal::audio->msgExecutePendingOperations(operations, true);
}

//---------------------------------------------------------
//   setRecord
//---------------------------------------------------------
void Song::setRecord(bool f, bool autoRecEnable)
      {
      if(MusEGlobal::debugMsg)
        fprintf(stderr, "setRecord recordflag =%d f(record state)=%d autoRecEnable=%d\n", recordFlag, f, autoRecEnable);

      if (f && MusEGlobal::config.useProjectSaveDialog && MusEGlobal::museProject == MusEGlobal::museProjectInitPath ) { // check that there is a project stored before commencing
        // no project, we need to create one.
        if (!MusEGlobal::muse->saveAs())
          return; // could not store project, won't enable record
      }
      if (recordFlag != f) {
            if (f && autoRecEnable) {
                bool alreadyRecEnabled = false;
                TrackList selectedTracks;
                // loop through list and check if any track is rec enabled
                // if not then rec enable the selected track
                MusECore::WaveTrackList* wtl = waves();
                for (MusECore::iWaveTrack i = wtl->begin(); i != wtl->end(); ++i) {
                      if((*i)->recordFlag())
                          {
                          alreadyRecEnabled = true;
                          break;
                          }
                      if((*i)->selected())
                          selectedTracks.push_back(*i);
                      }
                if (!alreadyRecEnabled) {
                      MidiTrackList* mtl = midis();
                      for (iMidiTrack it = mtl->begin(); it != mtl->end(); ++it) {
                            if((*it)->recordFlag())
                                {
                                alreadyRecEnabled = true;
                                break;
                                }
                            if((*it)->selected())
                                selectedTracks.push_back(*it);
                            }
                      }
                if (!alreadyRecEnabled && selectedTracks.size() >0) {
                      // This is a minor operation easily manually undoable. Let's not clog the undo list with it.
                      MusECore::PendingOperationList operations;
                      foreach (Track *t, selectedTracks)
                      {
                        if(!t->setRecordFlag1(true))
                          continue;
                        operations.add(MusECore::PendingOperationItem(t, true, MusECore::PendingOperationItem::SetTrackRecord));
                      }
                      MusEGlobal::audio->msgExecutePendingOperations(operations, true);

                      }
                else if (alreadyRecEnabled)  {
                      // do nothing
                      }
                else  {
                      // if there are no tracks, do not enable record
                      if (!waves()->size() && !midis()->size()) {
                            fprintf(stderr, "No track to select, won't enable record\n");
                            f = false;
                            }
                      }
                // prepare recording of wave files for all record enabled wave tracks
                for (MusECore::iWaveTrack i = wtl->begin(); i != wtl->end(); ++i) {
                      if((*i)->recordFlag()) // || (selectedTracks.find(*i)!=wtl->end() && autoRecEnable)) // prepare if record flag or if it is set to recenable
                      {                                                                  // setRecordFlag may take too long time to complete
                                                                                         // so we try this case specifically
                        (*i)->prepareRecording();
                      }
                }

// DELETETHIS? 14
#if 0
                  // check for midi devices suitable for recording
                  bool portFound = false;
                  for (int i = 0; i < MIDI_PORTS; ++i) {
                        MidiDevice* dev = MusEGlobal::midiPorts[i].device();
                        if (dev && (dev->rwFlags() & 0x2))
                              portFound = true;
                        }
                  if (!portFound) {
                        QMessageBox::critical(qApp->mainWidget(), "MusE: Record",
                           "There are no midi devices configured for recording");
                        f = false;
                        }
#endif
                  }
            else {
                  bounceTrack = 0;
                  }
            if (MusEGlobal::audio->isPlaying() && f)
                  f = false;
            recordFlag = f;
            MusEGlobal::recordAction->setChecked(recordFlag);
            emit recordChanged(recordFlag);
            }
      }

//---------------------------------------------------------
//   setPunchin
//    set punchin flag
//---------------------------------------------------------

void Song::setPunchin(bool f)
      {
      if (punchinFlag != f) {
            punchinFlag = f;
            MusEGlobal::punchinAction->setChecked(punchinFlag);
            emit punchinChanged(punchinFlag);
            }
      }

//---------------------------------------------------------
//   setPunchout
//    set punchout flag
//---------------------------------------------------------

void Song::setPunchout(bool f)
      {
      if (punchoutFlag != f) {
            punchoutFlag = f;
            MusEGlobal::punchoutAction->setChecked(punchoutFlag);
            emit punchoutChanged(punchoutFlag);
            }
      }

//---------------------------------------------------------
//   setClick
//---------------------------------------------------------

void Song::setClick(bool val)
      {
      if (_click != val) {
            _click = val;
            emit clickChanged(_click);
            }
      }

//---------------------------------------------------------
//   setQuantize
//---------------------------------------------------------

void Song::setQuantize(bool val)
      {
      if (_quantize != val) {
            _quantize = val;
            emit quantizeChanged(_quantize);
            }
      }

//---------------------------------------------------------
//   setMasterFlag
//---------------------------------------------------------

void Song::setMasterFlag(bool val)
    {
      // Here we have a choice of whether to allow undoing of setting the master.
      // TODO: Add a separate config flag just for this ?
      //if(MusEGlobal::config.selectionsUndoable)
      //  MusEGlobal::song->applyOperation(UndoOp(UndoOp::EnableMasterTrack, val, 0), MusECore::Song::OperationUndoMode);
      //else
        MusEGlobal::song->applyOperation(UndoOp(UndoOp::EnableMasterTrack, val, 0), MusECore::Song::OperationExecuteUpdate);
    }

//---------------------------------------------------------
//   setPlay
//    set transport play flag
//---------------------------------------------------------

void Song::setPlay(bool f)
{
      if (MusEGlobal::extSyncFlag) {
          if (MusEGlobal::debugMsg)
            fprintf(stderr, "not allowed while using external sync");
          return;
      }

      // only allow the user to set the button "on"
      if (!f)
            MusEGlobal::playAction->setChecked(true);
      else {
            // keep old transport position for rewinding
            // position if "Rewind on Stop" option is enabled
            _startPlayPosition = MusEGlobal::audio->pos();

            MusEGlobal::audio->msgPlay(true);
      }
}

void Song::setStop(bool f)
{
      if (MusEGlobal::extSyncFlag) {
          if (MusEGlobal::debugMsg)
            fprintf(stderr, "not allowed while using external sync");
          return;
      }
      // only allow the user to set the button "on"
      if (!f)
            MusEGlobal::stopAction->setChecked(true);
      else {
            MusEGlobal::audio->msgPlay(false);
      }
}

void Song::setStopPlay(bool f)
      {
      MusEGlobal::playAction->blockSignals(true);
      MusEGlobal::stopAction->blockSignals(true);

      emit playChanged(f);   // signal transport window

      MusEGlobal::playAction->setChecked(f);
      MusEGlobal::stopAction->setChecked(!f);

      MusEGlobal::stopAction->blockSignals(false);
      MusEGlobal::playAction->blockSignals(false);
      }

//---------------------------------------------------------
//   seekTo
//   setPos slot, only active when not doing playback
//---------------------------------------------------------
void Song::seekTo(int tick)
{
  if (!MusEGlobal::audio->isPlaying()) {
    Pos p(tick, true);
    setPos(CPOS, p);
  }
}
//---------------------------------------------------------
//   setPos
//   MusEGlobal::song->setPos(Song::CPOS, pos, true, true, true);
//---------------------------------------------------------

void Song::setPos(POSTYPE posType, const Pos& val, bool sig,
   bool isSeek, bool adjustScrollbar, bool /*force*/)
      {
      if (MusEGlobal::heavyDebugMsg)
      {
        fprintf(stderr, "setPos %d sig=%d,seek=%d,scroll=%d  ",
           posType, sig, isSeek, adjustScrollbar);
        val.dump(0);
        fprintf(stderr, "\n");
        fprintf(stderr, "Song::setPos before MusEGlobal::audio->msgSeek posType:%d isSeek:%d frame:%d\n", posType, isSeek, val.frame());
      }

      if (posType == CPOS) {
            _vcpos = val;
            if (isSeek && !MusEGlobal::extSyncFlag) {  
                  if (val == MusEGlobal::audio->pos())  
                  {
                      if (MusEGlobal::heavyDebugMsg) fprintf(stderr,
                        "Song::setPos seek MusEGlobal::audio->pos already == val tick:%d frame:%d\n", val.tick(), val.frame());   
                      return;
                  }     
                  MusEGlobal::audio->msgSeek(val);
                  if (MusEGlobal::heavyDebugMsg) fprintf(stderr,
                    "Song::setPos after MusEGlobal::audio->msgSeek posTYpe:%d isSeek:%d frame:%d\n", posType, isSeek, val.frame());
                  return;
                  }
            }
      if (val == pos[posType])
      {
           if (MusEGlobal::heavyDebugMsg) fprintf(stderr,
             "Song::setPos MusEGlobal::song->pos already == val tick:%d frame:%d\n", val.tick(), val.frame());   
           return;
      }     
      pos[posType] = val;
      bool swap = pos[LPOS] > pos[RPOS];
      if (swap) {        // swap lpos/rpos if lpos > rpos
            Pos tmp   = pos[LPOS];
            pos[LPOS] = pos[RPOS];
            pos[RPOS] = tmp;
            }
      if (sig) {
            if (swap) {
                  emit posChanged(LPOS, pos[LPOS].tick(), adjustScrollbar);
                  emit posChanged(RPOS, pos[RPOS].tick(), adjustScrollbar);
                  if (posType != LPOS && posType != RPOS)
                        emit posChanged(posType, pos[posType].tick(), adjustScrollbar);
                  }
            else
                  emit posChanged(posType, pos[posType].tick(), adjustScrollbar);
            }

      if(posType == CPOS)
      {
        const unsigned int vframe = val.frame();
        iMarker i1 = _markerList->begin();
        bool currentChanged = false;
        for(; i1 != _markerList->end(); ++i1)
        {
              const unsigned fr = i1->second.frame();
              // If there are multiple items at this frame and any one of them is current,
              //  leave it alone. It's arbitrary which one would be selected and it would
              //  normally choose the first one, but we'll let it stick with the one that's current,
              //  to avoid jumping around in the marker view window.
              iMarker i2 = i1;
              while(i2 != _markerList->end() && i2->second.frame() == fr)
              {
                i1 = i2;
                ++i2;
              }

              if(vframe >= fr && (i2==_markerList->end() || vframe < i2->second.frame()))
              {
                if(i1->second.current())
                  return;
                
                i1->second.setCurrent(true);

                if(currentChanged)
                {
                  emit markerChanged(MARKER_CUR);
                  return;
                }
                for(; i2 != _markerList->end(); ++i2)
                {
                  if(i2->second.current())
                    i2->second.setCurrent(false);
                }
                emit markerChanged(MARKER_CUR);
                return;
              }
              else
              {
                if(i1->second.current())
                {
                  currentChanged = true;
                  i1->second.setCurrent(false);
                }
              }
        }
        if(currentChanged)
              emit markerChanged(MARKER_CUR);
      }
      }

//---------------------------------------------------------
//   forward
//---------------------------------------------------------

void Song::forward()
      {
      unsigned newPos = pos[0].tick() + MusEGlobal::config.division;
      MusEGlobal::audio->msgSeek(Pos(newPos, true));
      }

//---------------------------------------------------------
//   rewind
//---------------------------------------------------------

void Song::rewind()
      {
      unsigned newPos;
      if (unsigned(MusEGlobal::config.division) > pos[0].tick())
            newPos = 0;
      else
            newPos = pos[0].tick() - MusEGlobal::config.division;
      MusEGlobal::audio->msgSeek(Pos(newPos, true));
      }

//---------------------------------------------------------
//   rewindStart
//---------------------------------------------------------

void Song::rewindStart()
      {
      MusEGlobal::audio->msgSeek(Pos(0, true));
      }

//---------------------------------------------------------
//   update
//---------------------------------------------------------

void Song::update(MusECore::SongChangedStruct_t flags, bool allowRecursion)
      {
      static int level = 0;         // DEBUG
      if (level && !allowRecursion) {
            fprintf(stderr, "THIS SHOULD NEVER HAPPEN: unallowed recursion in Song::update(%08lx %08lx), level %d!\n"
                   "                          the songChanged() signal is NOT emitted. this will\n"
                   "                          probably cause windows being not up-to-date.\n", flags.flagsHi(), flags.flagsLo(), level);
            return;
            }
      ++level;
      emit songChanged(flags);
      --level;
      }

//---------------------------------------------------------
//   updatePos
//---------------------------------------------------------

void Song::updatePos()
      {
      emit posChanged(0, pos[0].tick(), false);
      emit posChanged(1, pos[1].tick(), false);
      emit posChanged(2, pos[2].tick(), false);
      }

//---------------------------------------------------------
//   len
//---------------------------------------------------------

void Song::initLen()
      {
      _len = MusEGlobal::sigmap.bar2tick(40, 0, 0);    // default song len
      for (iTrack t = _tracks.begin(); t != _tracks.end(); ++t) {
            Track* track = dynamic_cast<Track*>(*t);
            if (track == 0)
                  continue;
            PartList* parts = track->parts();
            for (iPart p = parts->begin(); p != parts->end(); ++p) {
                  unsigned last = p->second->tick() + p->second->lenTick();
                  if (last > _len)
                        _len = last;
                  }
            }
      _len = roundUpBar(_len);
      }

//---------------------------------------------------------
//   roundUpBar
//---------------------------------------------------------

int Song::roundUpBar(int t) const
      {
      int bar, beat;
      unsigned tick;
      MusEGlobal::sigmap.tickValues(t, &bar, &beat, &tick);
      if (beat || tick)
            return MusEGlobal::sigmap.bar2tick(bar+1, 0, 0);
      return t;
      }

//---------------------------------------------------------
//   roundUpBeat
//---------------------------------------------------------

int Song::roundUpBeat(int t) const
      {
      int bar, beat;
      unsigned tick;
      MusEGlobal::sigmap.tickValues(t, &bar, &beat, &tick);
      if (tick)
            return MusEGlobal::sigmap.bar2tick(bar, beat+1, 0);
      return t;
      }

//---------------------------------------------------------
//   roundDownBar
//---------------------------------------------------------

int Song::roundDownBar(int t) const
      {
      int bar, beat;
      unsigned tick;
      MusEGlobal::sigmap.tickValues(t, &bar, &beat, &tick);
      return MusEGlobal::sigmap.bar2tick(bar, 0, 0);
      }

//---------------------------------------------------------
//   dumpMaster
//---------------------------------------------------------

void Song::dumpMaster()
      {
      MusEGlobal::tempomap.dump();
      MusEGlobal::sigmap.dump();
      }


void Song::normalizePart(MusECore::Part *part)
{
   const MusECore::EventList& evs = part->events();
   for(MusECore::ciEvent it = evs.begin(); it != evs.end(); ++it)
   {
      const Event& ev = (*it).second;
      if(ev.empty())
        continue;
      MusECore::SndFileR file = ev.sndFile();
      if(file.isNull())
        continue;

      QString tmpWavFile;
      if (!MusEGlobal::getUniqueTmpfileName("tmp_musewav",".wav", tmpWavFile))
      {
         return;
      }

      MusEGlobal::audio->msgIdle(true); // Not good with playback during operations
      
      MusECore::SndFile tmpFile(tmpWavFile);
      unsigned int file_channels = file.channels();
      tmpFile.setFormat(file.format(), file_channels, file.samplerate());
      if (tmpFile.openWrite())
      {
         MusEGlobal::audio->msgIdle(false);
         fprintf(stderr, "Could not open temporary file...\n");
         return;
      }
      float*   tmpdata[file_channels];
      unsigned tmpdatalen = file.samples();
      for (unsigned i=0; i<file_channels; i++)
      {
         tmpdata[i] = new float[tmpdatalen];
      }
      file.seek(0, 0);
      file.readWithHeap(file_channels, tmpdata, tmpdatalen);
      file.close();
      tmpFile.write(file_channels, tmpdata, tmpdatalen, MusEGlobal::config.liveWaveUpdate);
      tmpFile.close();

      float loudest = 0.0;
      for (unsigned i=0; i<file_channels; i++)
      {
         for (unsigned j=0; j<tmpdatalen; j++)
         {
            if (tmpdata[i][j]  > loudest)
            {
               loudest = tmpdata[i][j];
            }
         }
      }

      double scale = 0.99 / (double)loudest;
      for (unsigned i=0; i<file_channels; i++)
      {
         for (unsigned j=0; j<tmpdatalen; j++)
         {
            tmpdata[i][j] = (float) ((double)tmpdata[i][j] * scale);
         }
      }

      file.openWrite();
      file.seek(0, 0);
      file.write(file_channels, tmpdata, tmpdatalen, MusEGlobal::config.liveWaveUpdate);
      file.update();
      file.close();
      file.openRead();

      for (unsigned i=0; i<file_channels; i++)
      {
         delete[] tmpdata[i];
      }

      // Undo handling
      MusEGlobal::song->cmdChangeWave(ev, tmpWavFile, 0, tmpdatalen);
      MusEGlobal::audio->msgIdle(false); // Not good with playback during operations
      //sf.update();
   }
}

void Song::normalizeWaveParts(Part *partCursor)
{
   MusECore::TrackList* tracks=MusEGlobal::song->tracks();
   bool undoStarted = false;
   for (MusECore::TrackList::const_iterator t_it=tracks->begin(); t_it!=tracks->end(); t_it++)
   {
      if((*t_it)->type() != MusECore::Track::WAVE)
      {
         continue;
      }
      const MusECore::PartList* parts=(*t_it)->cparts();
      for (MusECore::ciPart p_it=parts->begin(); p_it!=parts->end(); p_it++)
      {
         if (p_it->second->selected())
         {
            MusECore::Part* part = p_it->second;
            if(!undoStarted)
            {
               undoStarted = true;
               MusEGlobal::song->startUndo();
            }

            normalizePart(part);

         }
      }
   }
   //if nothing selected, normilize current part under mouse (if given)
   if(!undoStarted && partCursor)
   {
      undoStarted = true;
      MusEGlobal::song->startUndo();
      normalizePart(partCursor);
   }
   if(undoStarted)
   {
      MusEGlobal::song->endUndo(SC_CLIP_MODIFIED);
   }
}

//---------------------------------------------------------
//   beat
//---------------------------------------------------------

void Song::beat()
      {
      // Watchdog for checking and setting timebase master state.
      static int _timebaseMasterCounter = 0;
      if(MusEGlobal::audioDevice &&
        MusEGlobal::audioDevice->hasOwnTransport() &&
        MusEGlobal::audioDevice->hasTimebaseMaster() && 
        MusEGlobal::config.useJackTransport && 
        (--_timebaseMasterCounter <= 0))
      {
        if(MusEGlobal::config.timebaseMaster)
        {
          if(!MusEGlobal::timebaseMasterState || !MusEGlobal::audio->isPlaying())
            MusEGlobal::audioDevice->setMaster(true);
        }
        // Set for once per second.
        _timebaseMasterCounter = MusEGlobal::config.guiRefresh;
      }

      //First: update cpu load toolbar

      _fCpuLoad = MusEGlobal::muse->getCPULoad();
      _fDspLoad = 0.0f;
      if (MusEGlobal::audioDevice)
        _fDspLoad = MusEGlobal::audioDevice->getDSP_Load();
      _xRunsCount = MusEGlobal::audio->getXruns();

      // Keep the sync detectors running... 
      for(int port = 0; port < MusECore::MIDI_PORTS; ++port)
          MusEGlobal::midiPorts[port].syncInfo().setTime();
      
      
      if (MusEGlobal::audio->isPlaying())
        setPos(CPOS, MusEGlobal::audio->tickPos(), true, false, true);

      // Process external tempo changes:
      while(!_tempoFifo.isEmpty())
        MusEGlobal::tempo_rec_list.addTempo(_tempoFifo.get()); 
      
      // Update anything related to audio controller graphs etc.
      for(ciTrack it = _tracks.begin(); it != _tracks.end(); ++ it)
      {
        if((*it)->isMidiTrack())
          continue;
        AudioTrack* at = static_cast<AudioTrack*>(*it); 
        CtrlListList* cll = at->controller();
        for(ciCtrlList icl = cll->begin(); icl != cll->end(); ++icl)
        {
          CtrlList* cl = icl->second;
          if(cl->isVisible() && !cl->dontShow() && cl->guiUpdatePending())  
            emit controllerChanged(at, cl->id());
          cl->setGuiUpdatePending(false);
        }
      }
      
      // Update synth native guis at the heartbeat rate.
      for(ciSynthI is = _synthIs.begin(); is != _synthIs.end(); ++is)
        (*is)->guiHeartBeat();
      
      while (noteFifoSize) {
            int pv = recNoteFifo[noteFifoRindex];
            noteFifoRindex = (noteFifoRindex + 1) % REC_NOTE_FIFO_SIZE;
            int pitch = (pv >> 8) & 0xff;
            int velo = pv & 0xff;

            //---------------------------------------------------
            // filter midi remote control events
            //---------------------------------------------------

            if (MusEGlobal::rcEnable && velo != 0) {
                  if (pitch == MusEGlobal::rcStopNote)
                        setStop(true);
                  else if (pitch == MusEGlobal::rcRecordNote)
                        setRecord(true);
                  else if (pitch == MusEGlobal::rcGotoLeftMarkNote)
                        setPos(CPOS, pos[LPOS].tick(), true, true, true);
                  else if (pitch == MusEGlobal::rcPlayNote)
                        setPlay(true);
                  }
            emit MusEGlobal::song->midiNote(pitch, velo);
            --noteFifoSize;
            }
      }

//---------------------------------------------------------
//   setLen
//---------------------------------------------------------

void Song::setLen(unsigned l, bool do_update)
      {
      _len = l;
      if(do_update)
        update();
      }

//---------------------------------------------------------
//   addMarker
//---------------------------------------------------------

void Song::addMarker(const QString& s, unsigned t, bool lck)
      {
      Marker m(s);
      m.setType(lck ? Pos::FRAMES : Pos::TICKS);
      m.setTick(t);
      MusEGlobal::song->applyOperation(MusECore::UndoOp(MusECore::UndoOp::AddMarker, m));
      }

void Song::addMarker(const QString& s, const Pos& p)
{
      Marker m(s);
      m.setType(p.type());
      m.setPos(p);
      MusEGlobal::song->applyOperation(MusECore::UndoOp(MusECore::UndoOp::AddMarker, m));
}

//---------------------------------------------------------
//   addMarker
//---------------------------------------------------------

iMarker Song::getMarkerAt(unsigned t)
      {
      return _markerList->find(t);
      }

//---------------------------------------------------------
//   removeMarker
//---------------------------------------------------------

void Song::removeMarker(const Marker& marker)
      {
      MusEGlobal::song->applyOperation(MusECore::UndoOp(MusECore::UndoOp::DeleteMarker, marker));
      }

void Song::setMarkerName(const Marker& marker, const QString& s)
      {
      Marker m(marker);
      m.setName(s);
      MusEGlobal::song->applyOperation(MusECore::UndoOp(MusECore::UndoOp::ModifyMarker, marker, m));
      }

void Song::setMarkerPos(const Marker& marker, const Pos& pos)
      {
      // Here we use the separate SetMarkerPos operation, which is 'combo-breaker' aware, to optimize repeated adjustments.
      MusEGlobal::song->applyOperation(MusECore::UndoOp(MusECore::UndoOp::SetMarkerPos, marker, pos.posValue(), pos.type()));
      }

void Song::setMarkerLock(const Marker& marker, bool f)
      {
      Marker m(marker);
      m.setType(f ? Pos::FRAMES : Pos::TICKS);
      MusEGlobal::song->applyOperation(MusECore::UndoOp(MusECore::UndoOp::ModifyMarker, marker, m));
      }

//---------------------------------------------------------
//   setRecordFlag
//---------------------------------------------------------

void Song::setRecordFlag(Track* track, bool val, Undo* operations)
{
  if(operations)
  {
    // The undo system calls setRecordFlag1 for us.
    operations->push_back(UndoOp(UndoOp::SetTrackRecord, track, val));
    //operations->push_back(UndoOp(UndoOp::SetTrackRecord, track, val, true)); // No undo.
  }
  else
  {
    // The pending operations system does not call setRecordFlag1 for us. Call it now.
    if(!track->setRecordFlag1(val))
      return;
    // This is a minor operation easily manually undoable. Let's not clog the undo list with it.
    MusECore::PendingOperationList operations;
    operations.add(MusECore::PendingOperationItem(track, val, MusECore::PendingOperationItem::SetTrackRecord));
    MusEGlobal::audio->msgExecutePendingOperations(operations, true);
  }
}

//---------------------------------------------------------
//   endMsgCmd
//---------------------------------------------------------

void Song::endMsgCmd()
      {
      if (updateFlags) {
            redoList->clearDelete();
            
            // It is possible the undo list is empty after removal of an empty undo, 
            //  either by optimization or no given operations.
            if(MusEGlobal::undoAction)
              MusEGlobal::undoAction->setEnabled(!undoList->empty());
            
            if(MusEGlobal::redoAction)
              MusEGlobal::redoAction->setEnabled(false);
            setUndoRedoText();
            emit songChanged(updateFlags);
            }
      }

//---------------------------------------------------------
//   undo
//---------------------------------------------------------

void Song::undo()
{
      if (MusEGlobal::audio->isRecording()) {
        return;
      }

      updateFlags = SongChangedStruct_t();
      
      Undo& opGroup = undoList->back();
      
      if (opGroup.empty())
            return;
      
      MusEGlobal::audio->msgRevertOperationGroup(opGroup);
      
      redoList->push_back(opGroup);
      undoList->pop_back();

      if(MusEGlobal::redoAction)
        MusEGlobal::redoAction->setEnabled(true);
      if(MusEGlobal::undoAction)
        MusEGlobal::undoAction->setEnabled(!undoList->empty());
      setUndoRedoText();

      emit songChanged(updateFlags);
      emit sigDirty();
}

//---------------------------------------------------------
//   redo
//---------------------------------------------------------

void Song::redo()
{
      if (MusEGlobal::audio->isRecording()) {
        return;
      }

      updateFlags = SongChangedStruct_t();

      Undo& opGroup = redoList->back();
      
      if (opGroup.empty())
            return;
      
      MusEGlobal::audio->msgExecuteOperationGroup(opGroup);
      
      undoList->push_back(opGroup);
      redoList->pop_back();
      
      if(MusEGlobal::undoAction)
        MusEGlobal::undoAction->setEnabled(true);
      if(MusEGlobal::redoAction)
        MusEGlobal::redoAction->setEnabled(!redoList->empty());
      setUndoRedoText();

      emit songChanged(updateFlags);
      emit sigDirty();
}

//---------------------------------------------------------
//   processMsg
//    executed in realtime thread context
//---------------------------------------------------------

void Song::processMsg(AudioMsg* msg)
      {
      switch(msg->id) {
            case SEQM_UPDATE_SOLO_STATES:
                  updateSoloStates();
                  break;
            case SEQM_EXECUTE_PENDING_OPERATIONS:
                  msg->pendingOps->executeRTStage();
                  break;
            case SEQM_EXECUTE_OPERATION_GROUP:
                  executeOperationGroup2(*msg->operations);
                  break;
            case SEQM_REVERT_OPERATION_GROUP:
                  revertOperationGroup2(*msg->operations);
                  break;
            default:
                  fprintf(stderr, "unknown seq message %d\n", msg->id);
                  break;
            }
      }

//---------------------------------------------------------
//   panic
//---------------------------------------------------------

void Song::panic()
      {
      MusEGlobal::audio->msgPanic();
      }

//---------------------------------------------------------
//   clear
//    signal - emit signals for changes if true
//    called from constructor as clear(false) and
//    from MusE::clearSong() as clear(false)
//    If clear_all is false, it will not touch things like midi ports.  
//---------------------------------------------------------

void Song::clear(bool signal, bool clear_all)
      {
      if(MusEGlobal::debugMsg)
        fprintf(stderr, "Song::clear\n");
      
      bounceTrack    = 0;
      
      _tracks.clear();
      _midis.clearDelete();
      _waves.clearDelete();
      _inputs.clearDelete();     // audio input ports
      _outputs.clearDelete();    // audio output ports
      _groups.clearDelete();     // mixer groups
      _auxs.clearDelete();       // aux sends
      
      // p3.3.45 Clear all midi port devices.
      for(int i = 0; i < MusECore::MIDI_PORTS; ++i)
      {
        // p3.3.50 Since midi ports are not deleted, clear all midi port in/out routes. They point to non-existant tracks now.
        MusEGlobal::midiPorts[i].inRoutes()->clear();
        MusEGlobal::midiPorts[i].outRoutes()->clear();
        
        // p3.3.50 Reset this.
        MusEGlobal::midiPorts[i].setFoundInSongFile(false);

        if(clear_all)  // Allow not touching devices. p4.0.17  TESTING: Maybe some problems...
          // This will also close the device.
          MusEGlobal::midiPorts[i].setMidiDevice(0);
      }
      
      _synthIs.clearDelete();

      // p3.3.45 Make sure to delete Jack midi devices, and remove all ALSA midi device routes...
      // Otherwise really nasty things happen when loading another song when one is already loaded.
      // The loop is a safe way to delete while iterating.
      bool loop;
      do
      {
        loop = false;
        for(iMidiDevice imd = MusEGlobal::midiDevices.begin(); imd != MusEGlobal::midiDevices.end(); ++imd)
        {
          if(dynamic_cast< MidiJackDevice* >(*imd))
          {
            if(clear_all)  // Allow not touching devices. p4.0.17  TESTING: Maybe some problems...
            {
              // Remove the device from the list.
              MusEGlobal::midiDevices.erase(imd);
              // Since Jack midi devices are created dynamically, we must delete them.
              // The destructor unregisters the device from Jack, which also disconnects all device-to-jack routes.
              // This will also delete all midi-track-to-device routes, they point to non-existant midi tracks 
              //  which were all deleted above
              delete (*imd);
              loop = true;
              break;
            }  
          }  
#ifdef ALSA_SUPPORT
          else if(dynamic_cast< MidiAlsaDevice* >(*imd))
          {
            // With alsa devices, we must not delete them (they're always in the list). But we must 
            //  clear all routes. They point to non-existant midi tracks, which were all deleted above.
            (*imd)->inRoutes()->clear();
            (*imd)->outRoutes()->clear();
          }
#endif
        }
      }  
      while (loop);
      
      MusEGlobal::tempomap.clear();
      MusEGlobal::tempo_rec_list.clear();
      MusEGlobal::sigmap.clear();
      MusEGlobal::keymap.clear();

      // Clear these metronome settings.
      // A loaded song can override these if it chooses.
      MusEGlobal::metroUseSongSettings = false;
      if(MusEGlobal::metroSongSettings.metroAccentsMap)
        MusEGlobal::metroSongSettings.metroAccentsMap->clear();

      undoList->clearDelete();
      redoList->clearDelete();
      if(MusEGlobal::undoAction)
        MusEGlobal::undoAction->setEnabled(false);
      if(MusEGlobal::redoAction)
        MusEGlobal::redoAction->setEnabled(false);
      setUndoRedoText();
      
      _markerList->clear();
      pos[0].setTick(0);
      pos[1].setTick(0);
      pos[2].setTick(0);
      _vcpos.setTick(0);

      Track::clearSoloRefCounts();
      clearMidiTransforms();
      clearMidiInputTransforms();

      // Clear all midi port controller values.
      for(int i = 0; i < MusECore::MIDI_PORTS; ++i)
      {
        // Remove the controllers AND the values so we start with a clean slate.
        MusEGlobal::midiPorts[i].controller()->clearDelete(true);
        // Don't forget to re-add the default managed controllers.
        MusEGlobal::midiPorts[i].addDefaultControllers();
      }

      MusEGlobal::tempomap.setMasterFlag(0, true);
      loopFlag       = false;
      loopFlag       = false;
      punchinFlag    = false;
      punchoutFlag   = false;
      recordFlag     = false;
      soloFlag       = false;
      _recMode       = REC_OVERDUP;
      _cycleMode     = CYCLE_NORMAL;
      _click         = false;
      _quantize      = false;
      _len           = MusEGlobal::sigmap.bar2tick(150, 0, 0);  // default song len in ticks set for 150 bars
      _follow        = JUMP;
      dirty          = false;
      initDrumMap();
      initNewDrumMap();
      if (signal) {
            emit loopChanged(false);
            recordChanged(false);
            emit songChanged(-1);  
            }
      }

//---------------------------------------------------------
//   cleanupForQuit
//   called from Muse::closeEvent
//---------------------------------------------------------

void Song::cleanupForQuit()
{
      bounceTrack    = 0;

      if(MusEGlobal::debugMsg)
        fprintf(stderr, "MusE: Song::cleanupForQuit...\n");
      
      _tracks.clear();
      
      if(MusEGlobal::debugMsg)
        fprintf(stderr, "deleting _midis\n");
      _midis.clearDelete();
      
      if(MusEGlobal::debugMsg)
        fprintf(stderr, "deleting _waves\n");
      _waves.clearDelete();
      
      if(MusEGlobal::debugMsg)
        fprintf(stderr, "deleting _inputs\n");
      _inputs.clearDelete();     // audio input ports
      
      if(MusEGlobal::debugMsg)
        fprintf(stderr, "deleting _outputs\n");
      _outputs.clearDelete();    // audio output ports
      
      if(MusEGlobal::debugMsg)
        fprintf(stderr, "deleting _groups\n");
      _groups.clearDelete();     // mixer groups
      
      if(MusEGlobal::debugMsg)
        fprintf(stderr, "deleting _auxs\n");
      _auxs.clearDelete();       // aux sends
      
      if(MusEGlobal::debugMsg)
        fprintf(stderr, "deleting _synthIs\n");
      _synthIs.clearDelete();    // each ~SynthI() -> deactivate3() -> ~SynthIF()

      MusEGlobal::tempomap.clear();
      MusEGlobal::sigmap.clear();
      MusEGlobal::keymap.clear();
      
      if(MusEGlobal::debugMsg)
        fprintf(stderr, "deleting undoList and redoList\n");
      undoList->clearDelete();
      redoList->clearDelete();
      
      _markerList->clear();
      
      if(MusEGlobal::debugMsg)
        fprintf(stderr, "deleting transforms\n");
      clearMidiTransforms(); // Deletes stuff.
      clearMidiInputTransforms(); // Deletes stuff.

      if(MusEGlobal::debugMsg)
        fprintf(stderr, "deleting midiport controllers\n");
      
      // Clear all midi port controllers and values.
      for(int i = 0; i < MusECore::MIDI_PORTS; ++i)
      {
        MusEGlobal::midiPorts[i].controller()->clearDelete(true); // Remove the controllers and the values.
        MusEGlobal::midiPorts[i].setMidiDevice(0);
      }
        
      // Can't do this here. Jack isn't running. Fixed. Test OK so far. DELETETHIS (the comment and #if/#endif)
      #if 1
      if(MusEGlobal::debugMsg)
        fprintf(stderr, "deleting midi devices except synths\n");
      for(iMidiDevice imd = MusEGlobal::midiDevices.begin(); imd != MusEGlobal::midiDevices.end(); ++imd)
      {
        // Close the device. Handy to do all devices here, including synths.
        (*imd)->close();
        // Since Syntis are midi devices, there's no need to delete them below.
        if((*imd)->isSynti())
          continue;
        delete (*imd);
      }
      MusEGlobal::midiDevices.clear();     // midi devices
      #endif
      
      if(MusEGlobal::debugMsg)
        fprintf(stderr, "deleting global available synths\n");

      // Delete all synths.
      std::vector<Synth*>::iterator is;
      for(is = MusEGlobal::synthis.begin(); is != MusEGlobal::synthis.end(); ++is)
      {
        Synth* s = *is;
        
        if(s)
          delete s;
      }
      MusEGlobal::synthis.clear();
      
      if(MusEGlobal::debugMsg)
        fprintf(stderr, "deleting midi instruments\n");
      for(iMidiInstrument imi = midiInstruments.begin(); imi != midiInstruments.end(); ++imi)
      {
        // Since Syntis are midi instruments, there's no need to delete them below.
        // Tricky, must cast as SynthI*.
        SynthI* s = dynamic_cast <SynthI*> (*imi);
        if(s)
          continue;
        delete (*imi);
      }
      midiInstruments.clear();     // midi instruments
      
      // Nothing required for ladspa plugin list, and rack instances of them
      //  are handled by ~AudioTrack.
      
      if(MusEGlobal::debugMsg)
        fprintf(stderr, "...finished cleaning up.\n");
}

void Song::seqSignal(int fd)
      {
      const int buf_size = 256;  
      char buffer[buf_size]; 

      int n = ::read(fd, buffer, buf_size);
      if (n < 0) {
            fprintf(stderr, "Song: seqSignal(): READ PIPE failed: %s\n",
               strerror(errno));
            return;
            }
      bool do_set_sync_timeout = false;
      for (int i = 0; i < n; ++i) {
            switch(buffer[i]) {
                  case '0':         // STOP
                        do_set_sync_timeout = true;
                        stopRolling();
                        break;
                  case '1':         // PLAY
                        do_set_sync_timeout = true;
                        setStopPlay(true);
                        break;
                  case '2':   // record
                        setRecord(true);
                        break;
                  case '3':   // START_PLAY + jack STOP
                        do_set_sync_timeout = true;
                        abortRolling();
                        break;
                  case 'P':   // alsa ports changed
                        alsaScanMidiPorts();
                        break;
                  case 'G':   // Seek
                        // Hm, careful here, will multiple seeks cause this
                        //  to interfere with Jack's transport timeout countdown?
                        do_set_sync_timeout = true;
                        clearRecAutomation(true);
                        setPos(CPOS, MusEGlobal::audio->tickPos(), true, false, true);
                        _startPlayPosition = MusEGlobal::audio->pos(); // update start position
                        break;
                  case 'S':   // shutdown audio
                        MusEGlobal::muse->seqStop();

                        {
                        // give the user a sensible explanation
                        int btn = QMessageBox::critical( MusEGlobal::muse, tr("Jack shutdown!"),
                            tr("Jack has detected a performance problem which has lead to\n"
                            "MusE being disconnected.\n"
                            "This could happen due to a number of reasons:\n"
                            "- a performance issue with your particular setup.\n"
                            "- a bug in MusE (or possibly in another connected software).\n"
                            "- a random hiccup which might never occur again.\n"
                            "- jack was voluntary stopped by you or someone else\n"
                            "- jack crashed\n"
                            "If there is a persisting problem you are much welcome to discuss it\n"
                            "on the MusE mailinglist.\n"
                            "(there is information about joining the mailinglist on the MusE\n"
                            " homepage which is available through the help menu)\n"
                            "\n"
                            "To proceed check the status of Jack and try to restart it and then .\n"
                            "click on the Restart button."), "restart", "cancel");
                        if (btn == 0) {
                              fprintf(stderr, "restarting!\n");
                              MusEGlobal::muse->seqRestart();
                              }
                        }

                        break;
// REMOVE Tim. latency. Removed. We now do this in MusE::bounceToFile() and MusE::bounceToTrack(), BEFORE the transport is started.
//                   case 'f':   // start freewheel
//                         if(MusEGlobal::debugMsg)
//                           fprintf(stderr, "Song: seqSignal: case f: setFreewheel start\n");
//                         
//                         if(MusEGlobal::config.freewheelMode)
//                           MusEGlobal::audioDevice->setFreewheel(true);
//                         
//                         break;

                  case 'F':   // stop freewheel
                        if(MusEGlobal::debugMsg)
                          fprintf(stderr, "Song: seqSignal: case F: setFreewheel stop\n");
                        
                        if(MusEGlobal::config.freewheelMode)
                          MusEGlobal::audioDevice->setFreewheel(false);
                        break;

                  case 'A': // Abort rolling + Special stop bounce (offline) mode
                          do_set_sync_timeout = true;
                          abortRolling();
                          // Switch all the wave converters back to online mode.
                          setAudioConvertersOfflineOperation(false);
                        break;

                  case 'B': // Stop + Special stop bounce mode
                          do_set_sync_timeout = true;
                          stopRolling();
                          // Switch all the wave converters back to online mode.
                          setAudioConvertersOfflineOperation(false);
                        break;

                  case 'C': // Graph changed
                        if (MusEGlobal::audioDevice)
                            MusEGlobal::audioDevice->graphChanged();
                        break;

                  case 'R': // Registration changed
                        if (MusEGlobal::audioDevice)
                            MusEGlobal::audioDevice->registrationChanged();
                        break;

                  case 'J': // Port connections changed
                        if (MusEGlobal::audioDevice)
                            MusEGlobal::audioDevice->connectionsChanged();
                        break;

//                   case 'U': // Send song changed signal
//                         {
//                           int d_len = sizeof(SongChangedStruct_t);
//                           if((n - (i + 1)) < d_len)  // i + 1 = data after this 'U' 
//                           {
//                             fprintf(stderr, "Song: seqSignal: case U: Not enough bytes read for SongChangedStruct_t !\n");
//                             break;
//                           }
//                           SongChangedStruct_t f;
//                           memcpy(&f, &buffer[i + 1], d_len);
//                           i += d_len; // Move pointer ahead. Loop will also add one ++i. 
//                           update(f);
//                         }
//                         break;
                        
                  case 'D': // Drum map changed
                        update(SC_DRUMMAP);
                        break;

//                   case 'E': // Midi events are available in the ipc event buffer.
//                         if(MusEGlobal::song)
//                           MusEGlobal::song->processIpcInEventBuffers();
//                         break;

                  case 'T': // We are now the timebase master.
                        MusEGlobal::timebaseMasterState = true;
                        update(SC_TIMEBASE_MASTER);
                        break;

                  case 't': // We are no longer the timebase master.
                        MusEGlobal::timebaseMasterState = false;
                        update(SC_TIMEBASE_MASTER);
                        break;

                  default:
                        fprintf(stderr, "unknown Seq Signal <%c>\n", buffer[i]);
                        break;
                  }
            }
            
            // Since other Jack clients might also set the sync timeout at any time,
            //  we need to be constantly enforcing our desired limit!
            // Since setSyncTimeout() may not be realtime friendly (Jack driver),
            //  we set the driver's sync timeout here in the gui thread.
            // Sadly, we likely cannot get away with setting it in the audio sync callback.
            // So whenever stop, start or seek occurs, we'll try to casually enforce the timeout here.
            // It's casual, unfortunately we can't set the EXACT timeout amount when we really need to
            //  (that's in audio sync callback) so we try this for now...
            if(do_set_sync_timeout && MusEGlobal::checkAudioDevice())
            {
              // Enforce a 30 second timeout.
              // TODO: Split this up and have user adjustable normal (2 or 10 second default) value,
              //        plus a contribution from the total required precount time.
              //       Too bad we likely can't set it dynamically in the audio sync callback.
              MusEGlobal::audioDevice->setSyncTimeout(30000000);
            }
      }

//---------------------------------------------------------
//   recordEvent
//---------------------------------------------------------

void Song::recordEvent(MidiTrack* mt, Event& event)
      {
      //---------------------------------------------------
      //    if tick points into a part,
      //          record to that part
      //    else
      //          create new part
      //---------------------------------------------------

      unsigned tick  = event.tick();
      PartList* pl   = mt->parts();
      const MidiPart* part = 0;
      iPart ip;
      for (ip = pl->begin(); ip != pl->end(); ++ip) {
            part = (MidiPart*)(ip->second);
            unsigned partStart = part->tick();
            unsigned partEnd   = partStart + part->lenTick();
            if (tick >= partStart && tick < partEnd)
                  break;
            }
      updateFlags |= SC_EVENT_INSERTED;
      if (ip == pl->end()) {
            // create new part
            MidiPart* part = new MidiPart(mt);
            int startTick = roundDownBar(tick);
            int endTick   = roundUpBar(tick + 1);
            part->setTick(startTick);
            part->setLenTick(endTick - startTick);
            part->setName(mt->name());
            event.move(-startTick);
            part->addEvent(event);
            MusEGlobal::song->applyOperation(UndoOp(UndoOp::AddPart, part));
            return;
            }
      part = (MidiPart*)(ip->second);
      tick -= part->tick();
      event.setTick(tick);
      
      Event ev;
      if(event.type() == Controller)
      {
        cEventRange range = part->events().equal_range(tick);
        for(ciEvent i = range.first; i != range.second; ++i) 
        {
          ev = i->second;
          if(ev.type() == Controller && ev.dataA() == event.dataA())
          {
            if(ev.dataB() == event.dataB()) // Don't bother if already set.
              return;
            MusEGlobal::song->applyOperation(UndoOp(UndoOp::ModifyEvent,event,ev,part,true,true));
            return;
          }
        }
      }  
      
      MusEGlobal::song->applyOperation(UndoOp(UndoOp::AddEvent, event, part, true,true));
      }

//---------------------------------------------------------
//   execAutomationCtlPopup
//---------------------------------------------------------

int Song::execAutomationCtlPopup(AudioTrack* track, const QPoint& menupos, int acid)
{
  enum { PREV_EVENT=0, NEXT_EVENT, ADD_EVENT, CLEAR_EVENT, CLEAR_RANGE, CLEAR_ALL_EVENTS, MIDI_ASSIGN, MIDI_CLEAR };
  QMenu* menu = new QMenu;

  int count = 0;
  bool isEvent = false, canSeekPrev = false, canSeekNext = false, canEraseRange = false;
  bool canAdd = false;
  double ctlval = 0.0;
  unsigned int frame = 0;
  if(track)
  {
    ciCtrlList icl = track->controller()->find(acid);
    if(icl != track->controller()->end())
    {
      CtrlList *cl = icl->second;
      canAdd = true;
      frame = MusEGlobal::audio->pos().frame();       
      bool en = track->controllerEnabled(acid);
      AutomationType at = track->automationType();
      if(!MusEGlobal::automation || at == AUTO_OFF || !en)
        ctlval = cl->curVal();  
      else  
        ctlval = cl->value(frame);
      
      count = cl->size();
      if(count)
      {
        iCtrl s = cl->lower_bound(frame);
        iCtrl e = cl->upper_bound(frame);

        isEvent = (s != cl->end() && s->second.frame == frame);

        canSeekPrev = s != cl->begin();
        canSeekNext = e != cl->end();

        s = cl->lower_bound(pos[1].frame());

        canEraseRange = s != cl->end()
                        && pos[2].frame() > s->second.frame;
      }
    }
  }

  menu->addAction(new MusEGui::MenuTitleItem(tr("Automation:"), menu));
  
  QAction* prevEvent = menu->addAction(tr("Previous event"));
  prevEvent->setData(PREV_EVENT);
  prevEvent->setEnabled(canSeekPrev);

  QAction* nextEvent = menu->addAction(tr("Next event"));
  nextEvent->setData(NEXT_EVENT);
  nextEvent->setEnabled(canSeekNext);

  menu->addSeparator();

  QAction* addEvent = new QAction(menu);
  menu->addAction(addEvent);
  if(isEvent)
    addEvent->setText(tr("Set event"));
  else  
    addEvent->setText(tr("Add event"));
  addEvent->setData(ADD_EVENT);
  addEvent->setEnabled(canAdd);

  QAction* eraseEventAction = menu->addAction(tr("Erase event"));
  eraseEventAction->setData(CLEAR_EVENT);
  eraseEventAction->setEnabled(isEvent);

  QAction* eraseRangeAction = menu->addAction(tr("Erase range"));
  eraseRangeAction->setData(CLEAR_RANGE);
  eraseRangeAction->setEnabled(canEraseRange);

  QAction* clearAction = menu->addAction(tr("Clear automation"));
  clearAction->setData(CLEAR_ALL_EVENTS);
  clearAction->setEnabled((bool)count);


  menu->addSeparator();
  menu->addAction(new MusEGui::MenuTitleItem(tr("Midi control"), menu));
  
  QAction *assign_act = menu->addAction(tr("Assign"));
  assign_act->setCheckable(false);
  assign_act->setData(MIDI_ASSIGN); 
  
  MidiAudioCtrlMap* macm = track->controller()->midiControls();
  AudioMidiCtrlStructMap amcs;
  macm->find_audio_ctrl_structs(acid, &amcs);
  
  if(!amcs.empty())
  {
    QAction *cact = menu->addAction(tr("Clear"));
    cact->setData(MIDI_CLEAR); 
    menu->addSeparator();
  }
  
  for(iAudioMidiCtrlStructMap iamcs = amcs.begin(); iamcs != amcs.end(); ++iamcs)
  {
    int port, chan, mctrl;
    macm->hash_values((*iamcs)->first, &port, &chan, &mctrl);
    //QString s = QString("Port:%1 Chan:%2 Ctl:%3-%4").arg(port + 1)
    QString s = QString("Port:%1 Chan:%2 Ctl:%3").arg(port + 1)
                                                  .arg(chan + 1)
                                                  //.arg((mctrl >> 8) & 0xff)
                                                  //.arg(mctrl & 0xff);
                                                  .arg(midiCtrlName(mctrl, true));
    QAction *mact = menu->addAction(s);
    mact->setEnabled(false);
    mact->setData(-1); // Not used
  }
  
  QAction* act = menu->exec(menupos);
  if (!act || !track)
  {
    delete menu;
    return -1;
  }
  
  int sel = act->data().toInt();
  delete menu;
  
  Undo operations;
  
  switch(sel)
  {
    case ADD_EVENT:
          MusEGlobal::song->applyOperation(UndoOp(UndoOp::AddAudioCtrlVal, track, acid, frame, ctlval));
    break;
    case CLEAR_EVENT:
          MusEGlobal::song->applyOperation(UndoOp(UndoOp::DeleteAudioCtrlVal, track, acid, frame));
    break;

    case CLEAR_RANGE:
          MusEGlobal::audio->msgEraseRangeACEvents(track, acid, pos[1].frame(), pos[2].frame());
    break;

    case CLEAR_ALL_EVENTS:
          if(QMessageBox::question(MusEGlobal::muse, QString("Muse"),
              tr("Clear all controller events?"), tr("&Ok"), tr("&Cancel"),
              QString(), 0, 1 ) == 0)
            MusEGlobal::audio->msgClearControllerEvents(track, acid);
    break;

    case PREV_EVENT:
          MusEGlobal::audio->msgSeekPrevACEvent(track, acid);
    break;

    case NEXT_EVENT:
          MusEGlobal::audio->msgSeekNextACEvent(track, acid);
    break;
    
    case MIDI_ASSIGN:
          {
            int port = -1, chan = 0, ctrl = 0;
            for(MusECore::iAudioMidiCtrlStructMap iamcs = amcs.begin(); iamcs != amcs.end(); ++iamcs)
            {
              macm->hash_values((*iamcs)->first, &port, &chan, &ctrl);
              break; // Only a single item for now, thanks!
            }
            
            MusEGui::MidiAudioControl* pup = new MusEGui::MidiAudioControl(port, chan, ctrl);
            
            if(pup->exec() == QDialog::Accepted)
            {
              MusEGlobal::audio->msgIdle(true);  // Gain access to structures, and sync with audio
              // Erase all for now.
              for(MusECore::iAudioMidiCtrlStructMap iamcs = amcs.begin(); iamcs != amcs.end(); ++iamcs)
                macm->erase(*iamcs);
              
              port = pup->port(); chan = pup->chan(); ctrl = pup->ctrl();
              if(port >= 0 && chan >=0 && ctrl >= 0)
                // Add will replace if found.
                macm->add_ctrl_struct(port, chan, ctrl, MusECore::MidiAudioCtrlStruct(acid));
              
              MusEGlobal::audio->msgIdle(false);
            }
            
            delete pup;
          }
          break;
    
    case MIDI_CLEAR:
          if(!amcs.empty())
            MusEGlobal::audio->msgIdle(true);  // Gain access to structures, and sync with audio
          for(MusECore::iAudioMidiCtrlStructMap iamcs = amcs.begin(); iamcs != amcs.end(); ++iamcs)
            macm->erase(*iamcs);
          if(!amcs.empty())
            MusEGlobal::audio->msgIdle(false);
    break;
    
    default:
          return -1;
    break;      
  }
  
  if(!operations.empty())
    MusEGlobal::song->applyOperationGroup(operations);
  
  return sel;
}

//---------------------------------------------------------
//   execMidiAutomationCtlPopup
//---------------------------------------------------------

int Song::execMidiAutomationCtlPopup(MidiTrack* track, MidiPart* part, const QPoint& menupos, int ctlnum)
{
  if(!track && !part)
    return -1;
    
  enum { BYPASS_CONTROLLER, ADD_EVENT, CLEAR_EVENT };
  bool isEvent = false;
  
  MidiTrack* mt;
  if(track)
    mt = track;
  else  
    mt = (MidiTrack*)part->track();
  int portno    = mt->outPort();
  int channel   = mt->outChannel();
  MidiPort* mp  = &MusEGlobal::midiPorts[portno];
  
  int dctl = ctlnum;
  // Is it a drum controller, according to the track port's instrument?
  MidiController *dmc = mp->drumController(ctlnum);
  if(dmc)
  {
    // Change the controller event's index into the drum map to an instrument note.
    int note = ctlnum & 0x7f;
    dctl &= ~0xff;
    // Default to track port if -1 and track channel if -1.
    if(MusEGlobal::drumMap[note].channel != -1)
      channel = MusEGlobal::drumMap[note].channel;
    if(MusEGlobal::drumMap[note].port != -1)
      mp = &MusEGlobal::midiPorts[MusEGlobal::drumMap[note].port];
    dctl |= MusEGlobal::drumMap[note].anote;
  }
    
  unsigned tick = cpos();
  
  if(!part)
  {
    PartList* pl = mt->parts();
    iPart ip;
    for(ip = pl->begin(); ip != pl->end(); ++ip) 
    {
      MidiPart* tpart = (MidiPart*)(ip->second);
      unsigned partStart = tpart->tick();
      unsigned partEnd   = partStart + tpart->lenTick();
      if(tick >= partStart && tick < partEnd)
      {
        // Prefer a selected part, otherwise keep looking...
        if(tpart->selected())
        {
          part = tpart;
          break;
        }  
        else
        // Remember the first part found...
        if(!part)
          part = tpart;
      }  
    }
  }
  
  Event ev;
  if(part)
  {
    unsigned partStart = part->tick();
    unsigned partEnd   = partStart + part->lenTick();
    if(tick >= partStart && tick < partEnd)
    {
            cEventRange range = part->events().equal_range(tick - partStart);
      for(ciEvent i = range.first; i != range.second; ++i) 
      {
        ev = i->second;
        if(ev.type() == Controller)
        {
          if(ev.dataA() == ctlnum)
          {
            isEvent = true;
            break;
          }
        }
      }
    }  
  }
  
  int initval = 0;
  MidiController* mc = mp->midiController(ctlnum, false);
  if(mc)
  {
    const int bias = mc->bias();
    initval = mc->initVal();
    if(initval == CTRL_VAL_UNKNOWN)
    {
      if(ctlnum == CTRL_PROGRAM)
        // Special for program controller: Set HBank and LBank off (0xff), and program to 0.
        initval = 0xffff00;
      else
       // Otherwise start with the bias.
       initval = bias;
    }
    else
     // Auto bias.
     initval += bias;
  }
  const int cur_val = mp->hwCtrlState(channel, dctl);

  QMenu* menu = new QMenu;

  menu->addAction(new MusEGui::MenuTitleItem(tr("Controller:"), menu));
  QAction* bypassEvent = new QAction(menu);
  menu->addAction(bypassEvent);
  bypassEvent->setText(tr("Bypass"));
  bypassEvent->setData(BYPASS_CONTROLLER);
  bypassEvent->setEnabled(true);
  bypassEvent->setCheckable(true);
  bypassEvent->setChecked(cur_val == CTRL_VAL_UNKNOWN);

  menu->addAction(new MusEGui::MenuTitleItem(tr("Automation:"), menu));
  
  QAction* addEvent = new QAction(menu);
  menu->addAction(addEvent);
  if(isEvent)
    addEvent->setText(tr("Set event"));
  else
    addEvent->setText(tr("Add event"));
  addEvent->setData(ADD_EVENT);
  addEvent->setEnabled(true);

  QAction* eraseEventAction = menu->addAction(tr("Erase event"));
  eraseEventAction->setData(CLEAR_EVENT);
  eraseEventAction->setEnabled(isEvent);

  QAction* act = menu->exec(menupos);
  if (!act)
  {
    delete menu;
    return -1;
  }
  
  const int sel = act->data().toInt();
  const bool checked = act->isChecked();
  delete menu;
  
  switch(sel)
  {
    case BYPASS_CONTROLLER:
    {
      if(checked)
        MusEGlobal::audio->msgSetHwCtrlState(mp, channel, dctl, MusECore::CTRL_VAL_UNKNOWN);
      else
      {
        int v = mp->lastValidHWCtrlState(channel, dctl);
        if(v == MusECore::CTRL_VAL_UNKNOWN)
          v = initval;
        MusEGlobal::audio->msgSetHwCtrlState(mp, channel, dctl, v);
      }
    }
    break;

    case ADD_EVENT:
    {
          int v = cur_val;
          if(v == CTRL_VAL_UNKNOWN)
          {
            v = mp->lastValidHWCtrlState(channel, dctl);
            if(v == MusECore::CTRL_VAL_UNKNOWN)
              v = initval;
          }
          Event e(Controller);
          e.setA(ctlnum);
          e.setB(v);
          // Do we replace an old event?
          if(isEvent)
          {
            if(ev.dataB() == v) // Don't bother if already set.
              return -1;
              
            e.setTick(tick - part->tick());
            // Indicate do undo, and do port controller values and clone parts. 
            MusEGlobal::song->applyOperation(UndoOp(UndoOp::ModifyEvent, e, ev, part, true, true));
          }
          else
          {
            // Store a new event...
            if(part)
            {
              e.setTick(tick - part->tick());
              // Indicate do undo, and do port controller values and clone parts. 
              MusEGlobal::song->applyOperation(UndoOp(UndoOp::AddEvent, 
                              e, part, true, true));
            }
            else
            {
              // Create a new part...
              part = new MidiPart(mt);
              int startTick = roundDownBar(tick);
              int endTick = roundUpBar(tick + 1);
              part->setTick(startTick);
              part->setLenTick(endTick - startTick);
              part->setName(mt->name());
              e.setTick(tick - startTick);
              part->addEvent(e);
              MusEGlobal::song->applyOperation(UndoOp(UndoOp::AddPart, part));
            }
          }  
    }
    break;
    case CLEAR_EVENT:
          // Indicate do undo, and do port controller values and clone parts. 
          MusEGlobal::song->applyOperation(UndoOp(UndoOp::DeleteEvent,
                            ev, part, true, true));
    break;

    default:
          return -1;
    break;      
  }
  
  return sel;
}

//---------------------------------------------------------
// putIpcInEvent
//  Put an event into the IPC event ring buffer for the gui thread to process. Returns true on success.
//  NOTE: Although the ring buffer is multi-writer, call this from audio thread only for now, unless
//   you know what you are doing because the thread needs to ask whether the controller exists before
//   calling, and that may not be safe from threads other than gui or audio.
//---------------------------------------------------------

bool Song::putIpcInEvent(const MidiPlayEvent& ev)
{
  if(!_ipcInEventBuffers->put(ev))
  {
    fprintf(stderr, "Error: Song::putIpcInEvent: Buffer overflow\n");
    return false;
  }
  return true;
}

//---------------------------------------------------------
// putIpcOutEvent
//  Put an event into the IPC event ring buffer for the audio thread to process.
//  Called by gui thread only. Returns true on success.
//---------------------------------------------------------

bool Song::putIpcOutEvent(const MidiPlayEvent& ev)
{
  if(!_ipcOutEventBuffers->put(ev))
  {
    fprintf(stderr, "Error: Song::putIpcOutEvent: Buffer overflow\n");
    return false;
  }
  return true;
}

//---------------------------------------------------------
//   processIpcInEventBuffers
//   Called by gui thread only.
//   Returns true on success.
//---------------------------------------------------------

bool Song::processIpcInEventBuffers()
{
  PendingOperationList operations;
  MidiPlayEvent buf_ev;
  int port, chan, ctrl;
  MidiPort* mp;
  iMidiCtrlValList imcvl;
  MidiCtrlValListList* mcvll;
  MidiCtrlValList* mcvl;
  
  //-----------------------------------------------------------
  // First pass: Peek into the buffers and find out if any 
  //  controllers need to be created here in the gui thread.
  //-----------------------------------------------------------
  
  // False = don't use the size snapshot, but update it.
  const unsigned int sz = _ipcInEventBuffers->getSize(false);
  for(unsigned int i = 0; i < sz; ++i)
  {
    buf_ev = _ipcInEventBuffers->peek(i);
    port = buf_ev.port();
    if(port < 0 || port >= MusECore::MIDI_PORTS)
      continue;
    chan = buf_ev.channel();
    if(chan < 0 || chan >= MusECore::MUSE_MIDI_CHANNELS)
      continue;
    
    ctrl = buf_ev.translateCtrlNum();
    // Event translates to a controller?
    if(ctrl < 0)
      continue;
    
    mp = &MusEGlobal::midiPorts[port];
    mcvll = mp->controller();

    // Does the controller exist?
    imcvl = mcvll->find(chan, ctrl);
    if(imcvl == mcvll->end())
    {
      // Controller does not exist. Prepare a pending operation.
      PendingOperationItem poi(mcvll, 0, chan, ctrl, PendingOperationItem::AddMidiCtrlValList);
      // Have we already created and prepared this controller? Look in the operations list.
      iPendingOperation ipos = operations.findAllocationOp(poi);
      if(ipos == operations.end())
      {
        // We have not created and prepared the controller. Create it now.
        mcvl = new MidiCtrlValList(ctrl);
        // Set the operation controller member now.
        poi._mcvl = mcvl;
        // Add the operation to the pending operations.
        operations.add(poi);
      }
    }
  }

  // Execute any operations to create controllers.
  // This waits for audio process thread to execute it.
  if(!operations.empty())
    MusEGlobal::audio->msgExecutePendingOperations(operations, true);
  
  //-----------------------------------------------------------
  // Second pass: Read the buffers and set the controller values.
  // For the moment, the writer threads may have also put some more events
  //  into these buffers while they checked if the controller existed.
  //-----------------------------------------------------------
  
  for(unsigned int i = 0; i < sz; ++i)
  {
    if(!_ipcInEventBuffers->get(buf_ev))
      continue;
    
    port = buf_ev.port();
    if(port < 0 || port >= MusECore::MIDI_PORTS)
      continue;
    chan = buf_ev.channel();
    if(chan < 0 || chan >= MusECore::MUSE_MIDI_CHANNELS)
      continue;
    
    ctrl = buf_ev.translateCtrlNum();
    // Event translates to a controller?
    if(ctrl < 0)
      continue;
    
    mp = &MusEGlobal::midiPorts[port];
    mcvll = mp->controller();

    // Put the event BACK INTO the midi port's event buffer so that 
    //  the port will process it 'where it left off' before it put 
    //  this controller creation event into this ring buffer.
    // It also allows the port to call updateDrumMap in the audio thread. 
    // Keep the time intact, so the driver will at least play them in 
    //  sequence even though they will all be 'bunched up' at frame zero.
    // Make sure the controller REALLY was created before proceeding,
    //  otherwise the mechanism might get stuck in a continuous loop.
//     imcvl = mcvll->find(chan, ctrl);
//     if(imcvl != mcvll->end())
    {
      //mp->putHwCtrlEvent(buf_ev);
      // Let's bypass the putHwCtrlEvent and save some time -
      //  put directly into the midi port's controller event buffers.
      // This will also prevent getting stuck in continuous loop.
      if(!_ipcOutEventBuffers->put(buf_ev))
      {
        fprintf(stderr, "Error: Song::processIpcInEventBuffers(): Midi port controller fifo overflow\n");
        continue;
      }
    }
  }

  return true;
}

//---------------------------------------------------------
//   processIpcOutEventBuffers
//   Called from audio thread only.
//   Returns true on success.
//---------------------------------------------------------

bool Song::processIpcOutEventBuffers()
{
  // Receive hardware state events sent from various threads to this audio thread.
  // Update hardware state so gui controls are updated.
  // False = don't use the size snapshot, but update it.
  const int sz = _ipcOutEventBuffers->getSize(false);
  MidiPlayEvent ev;
  for(int i = 0; i < sz; ++i)
  {
    if(!_ipcOutEventBuffers->get(ev))
      continue;
    const int port = ev.port();
    if(port < 0 || port >= MusECore::MIDI_PORTS)
      continue;
    // Handle the event. Tell the gui NOT to create controllers as needed,
    //  that should be done before it ever gets here.
    MusEGlobal::midiPorts[port].handleGui2AudioEvent(ev, false);
  }
  return true;
}

//---------------------------------------------------------
//   updateSoloStates
//    This will properly set all soloing variables (including other tracks) based entirely
//     on the current values of all the tracks' _solo members.
//---------------------------------------------------------

void Song::updateSoloStates()
{
  Track::clearSoloRefCounts();
  for(ciTrack i = _tracks.begin(); i != _tracks.end(); ++i)
    (*i)->setInternalSolo(0);
  for(ciTrack i = _tracks.begin(); i != _tracks.end(); ++i)
    (*i)->updateSoloStates(true);
}

//---------------------------------------------------------
//   reenableTouchedControllers
//   Enable all track and plugin controllers, and synth controllers if applicable, which are NOT in AUTO_WRITE mode.
//---------------------------------------------------------

void Song::reenableTouchedControllers()
{
  for(iTrack it = _tracks.begin(); it != _tracks.end(); ++it)
  {
    if((*it)->isMidiTrack())
      continue;
    AudioTrack* t = static_cast<AudioTrack*>(*it);
    AutomationType at = t->automationType();
    if(at == AUTO_WRITE)  // Exclude write mode because controls need to remain disabled if pressed before play.
      continue;
    t->enableAllControllers();
  }
}

//---------------------------------------------------------
//   clearRecAutomation
//---------------------------------------------------------

void Song::clearRecAutomation(bool clearList)
{
  // Clear all pan/vol pressed and touched flags, and all rec event lists, if needed.
  for (iTrack it = tracks()->begin(); it != tracks()->end(); ++it)
    ((Track*)(*it))->clearRecAutomation(clearList);
}

//---------------------------------------------------------
//   processAutomationEvents
//---------------------------------------------------------

void Song::processAutomationEvents(Undo* operations)
{
  Undo ops;
  Undo* opsp = operations ? operations : &ops;

  // Clear all pressed and touched flags.
  // This is a non-undoable 'one-time' operation, removed after execution.
  opsp->push_back(UndoOp(UndoOp::EnableAllAudioControllers));
  
  for(iTrack i = _tracks.begin(); i != _tracks.end(); ++i)
  {
    if(!(*i)->isMidiTrack())
      // Process (and clear) rec events.
      ((AudioTrack*)(*i))->processAutomationEvents(opsp);
  }

  if(!operations)
    MusEGlobal::song->applyOperationGroup(ops);
}

//---------------------------------------------------------
//   processMasterRec
//---------------------------------------------------------

void Song::processMasterRec()
{
//   bool do_tempo = false;
  
  // Wait a few seconds for the tempo fifo to be empty.
  int tout = 100; // Ten seconds. Otherwise we gotta move on.
  while(!_tempoFifo.isEmpty())
  {
    usleep(100000);
    --tout;
    if(tout == 0)
    {
      fprintf(stderr, "Song::processMasterRec: Error: Timeout waiting for _tempoFifo to empty!\n");
      break;
    }
  }
  
  const int tempo_rec_list_sz = MusEGlobal::tempo_rec_list.size();
  if(tempo_rec_list_sz != 0) 
  {
    if(QMessageBox::question(MusEGlobal::muse, 
                          tr("MusE: Tempo list"), 
                          tr("External tempo changes were recorded.\nTransfer them to master tempo list?"),
                          QMessageBox::Ok | QMessageBox::Cancel, QMessageBox::Cancel) == QMessageBox::Ok)
    {
      // FIXME TODO: Change the tempomap and tempo_rec_list to allocated pointers so they can be quickly swapped in realtime without idling.
      MusEGlobal::audio->msgIdle(true); // gain access to all data structures

      // Erase from master tempo the (approximate) recording start/end tick range according to the recorded tempo map,
      //MusEGlobal::tempomap.eraseRange(MusEGlobal::tempo_rec_list.frame2tick(MusEGlobal::audio->getStartRecordPos().frame()), 
      //                                MusEGlobal::tempo_rec_list.frame2tick(MusEGlobal::audio->getEndRecordPos().frame()));
      // This is more accurate but lacks resolution:
      MusEGlobal::tempomap.eraseRange(MusEGlobal::audio->getStartExternalRecTick(), MusEGlobal::audio->getEndExternalRecTick());

      // Add the recorded tempos to the master tempo list:
      for(int i = 0; i < tempo_rec_list_sz; ++i)
        MusEGlobal::tempomap.addTempo(MusEGlobal::tempo_rec_list[i].tick, 
                                      MusEGlobal::tempo_rec_list[i].tempo, 
                                      false);  // False: Defer normalize
      MusEGlobal::tempomap.normalize();
      MusEGlobal::audio->msgIdle(false); 
      update(SC_TEMPO);
    }
    // It should be safe to do this here in the GUI thread, the driver should not be touching it anymore.
    MusEGlobal::tempo_rec_list.clear();
  }
}

//---------------------------------------------------------
//   abortRolling
//---------------------------------------------------------

void Song::abortRolling()
{
  if(MusEGlobal::audio->freewheel())
    MusEGlobal::audioDevice->setFreewheel(false);

  if (record())
        MusEGlobal::audio->recordStop();
  setStopPlay(false);
}

//---------------------------------------------------------
//   stopRolling
//---------------------------------------------------------

void Song::stopRolling(Undo* operations)
      {
      if(MusEGlobal::audio->freewheel())
        MusEGlobal::audioDevice->setFreewheel(false);

      Undo ops;
      Undo* opsp = operations ? operations : &ops;
      
      if (record())
            MusEGlobal::audio->recordStop(false, opsp);
      setStopPlay(false);
      
      processAutomationEvents(opsp);
      
      if (MusEGlobal::config.useRewindOnStop) {
        setPos(Song::CPOS, _startPlayPosition);
      }

      if(!operations)
        MusEGlobal::song->applyOperationGroup(ops);
      }

//---------------------------------------------------------
//   connectJackRoutes
//---------------------------------------------------------

bool Song::connectJackRoutes(const MusECore::Route& src, const MusECore::Route& dst, bool disconnect)
{
  //fprintf(stderr, "connectJackRoutes:\n");
      
  if(!MusEGlobal::checkAudioDevice() || !MusEGlobal::audio->isRunning()) 
    return false;

  switch(src.type)
  {
    case Route::JACK_ROUTE:
      switch(dst.type)
      {
        case Route::JACK_ROUTE:
          if(disconnect)
            return MusEGlobal::audioDevice->disconnect(src.persistentJackPortName, dst.persistentJackPortName);
          else
            return MusEGlobal::audioDevice->connect(src.persistentJackPortName, dst.persistentJackPortName);
        break;
        case Route::MIDI_DEVICE_ROUTE:
          if(dst.device && dst.device->deviceType() == MidiDevice::JACK_MIDI && dst.device->inClientPort())
          {
            if(disconnect)
              return MusEGlobal::audioDevice->disconnect(src.persistentJackPortName, MusEGlobal::audioDevice->canonicalPortName(dst.device->inClientPort()));
            else
              return MusEGlobal::audioDevice->connect(src.persistentJackPortName, MusEGlobal::audioDevice->canonicalPortName(dst.device->inClientPort()));
          }
        break;
        case Route::TRACK_ROUTE:
          if(dst.track && dst.track->type() == Track::AUDIO_INPUT && dst.channel >= 0)
          {
            AudioInput* ai = static_cast<AudioInput*>(dst.track);
            if(ai->jackPort(dst.channel))
            {
              if(disconnect)
                return MusEGlobal::audioDevice->disconnect(src.persistentJackPortName, MusEGlobal::audioDevice->canonicalPortName(ai->jackPort(dst.channel)));
              else
                return MusEGlobal::audioDevice->connect(src.persistentJackPortName, MusEGlobal::audioDevice->canonicalPortName(ai->jackPort(dst.channel)));
            }
          }
        break;
        case Route::MIDI_PORT_ROUTE:
        break;
      }
    break;
    
    case Route::MIDI_DEVICE_ROUTE:
      switch(dst.type)
      {
        case Route::JACK_ROUTE:
          if(src.device && src.device->deviceType() == MidiDevice::JACK_MIDI && src.device->outClientPort())
          {
            if(disconnect)
              return MusEGlobal::audioDevice->disconnect(MusEGlobal::audioDevice->canonicalPortName(src.device->outClientPort()), dst.persistentJackPortName);
            else
              return MusEGlobal::audioDevice->connect(MusEGlobal::audioDevice->canonicalPortName(src.device->outClientPort()), dst.persistentJackPortName);
          }
        break;
        case Route::MIDI_DEVICE_ROUTE:
        case Route::TRACK_ROUTE:
        case Route::MIDI_PORT_ROUTE:
        break;
      }
    break;
    case Route::TRACK_ROUTE:
      switch(dst.type)
      {
        case Route::JACK_ROUTE:
          if(src.track && src.track->type() == Track::AUDIO_OUTPUT && src.channel >= 0)
          {
            AudioOutput* ao = static_cast<AudioOutput*>(src.track);
            if(ao->jackPort(src.channel))
            {
              if(disconnect)
                return MusEGlobal::audioDevice->disconnect(MusEGlobal::audioDevice->canonicalPortName(ao->jackPort(src.channel)), dst.persistentJackPortName);
              else
                return MusEGlobal::audioDevice->connect(MusEGlobal::audioDevice->canonicalPortName(ao->jackPort(src.channel)), dst.persistentJackPortName);
            }
          }
        break;
        case Route::MIDI_DEVICE_ROUTE:
        case Route::TRACK_ROUTE:
        case Route::MIDI_PORT_ROUTE:
        break;
      }
    break;
    case Route::MIDI_PORT_ROUTE:
    break;
  }
  
  return false;
}

//---------------------------------------------------------
//   connectMidiPorts
//---------------------------------------------------------

void Song::connectMidiPorts()
{
  // Connect midi device ports to Jack ports...
  for(iMidiDevice i = MusEGlobal::midiDevices.begin(); i != MusEGlobal::midiDevices.end(); ++i) 
  {
    MidiDevice* md = *i;
    if(md->deviceType() != MidiDevice::JACK_MIDI)
      continue;
    
    // Midi outputs...
    if(md->rwFlags() & 1)
    {
      void* our_port = md->outClientPort(); 
      if(our_port)                           
      {
        const char* our_port_name = MusEGlobal::audioDevice->canonicalPortName(our_port);
        if(our_port_name)
        {
          RouteList* rl = md->outRoutes();
          for(ciRoute ir = rl->begin(); ir != rl->end(); ++ir) 
          {  
            if(ir->type != Route::JACK_ROUTE)  
              continue;
            const char* route_name = ir->persistentJackPortName;
            if(!MusEGlobal::audioDevice->findPort(route_name))
              continue;
            //if(!MusEGlobal::audioDevice->portConnectedTo(our_port, route_name))
              MusEGlobal::audioDevice->connect(our_port_name, route_name);
          }  
        }
      }    
    }
    
    // Midi inputs...
    if(md->rwFlags() & 2)
    {  
      void* our_port = md->inClientPort();  
      if(our_port)                          
      {
        const char* our_port_name = MusEGlobal::audioDevice->canonicalPortName(our_port);
        if(our_port_name)
        {
          RouteList* rl = md->inRoutes();
          for(ciRoute ir = rl->begin(); ir != rl->end(); ++ir) 
          {  
            if(ir->type != Route::JACK_ROUTE)  
              continue;
            const char* route_name = ir->persistentJackPortName;
            if(!MusEGlobal::audioDevice->findPort(route_name))
              continue;
            //if(!MusEGlobal::audioDevice->portConnectedTo(our_port, route_name))
              MusEGlobal::audioDevice->connect(route_name, our_port_name);
          }
        }
      }
    }  
  }  
}

//---------------------------------------------------------
//   connectAudioPorts
//---------------------------------------------------------

void Song::connectAudioPorts()
{
  if(!MusEGlobal::audioDevice)
    return;
  
  // Connect audio output ports to Jack ports...
  OutputList* ol = outputs();
  for(iAudioOutput i = ol->begin(); i != ol->end(); ++i) 
  {
    AudioOutput* ao = *i;
    int channel = ao->channels();
    for(int ch = 0; ch < channel; ++ch) 
    {
      void* our_port = ao->jackPort(ch);
      if(!our_port)
        continue;
      const char* our_port_name = MusEGlobal::audioDevice->canonicalPortName(our_port);
      if(!our_port_name)
        continue;
      RouteList* rl = ao->outRoutes();
      for(ciRoute ir = rl->begin(); ir != rl->end(); ++ir) 
      {
        if(ir->type != Route::JACK_ROUTE || ir->channel != ch)  
          continue;
        const char* route_name = ir->persistentJackPortName;
        if(!MusEGlobal::audioDevice->findPort(route_name))
          continue;
        //if(!MusEGlobal::audioDevice->portConnectedTo(our_port, route_name))
          MusEGlobal::audioDevice->connect(our_port_name, route_name);
      }
    }
  }
  
  // Connect Jack ports to audio input ports...
  InputList* il = inputs();
  for(iAudioInput i = il->begin(); i != il->end(); ++i) 
  {
    AudioInput* ai = *i;
    int channel = ai->channels();
    for(int ch = 0; ch < channel; ++ch) 
    {
      void* our_port = ai->jackPort(ch);
      if(!our_port)
        continue;
      const char* our_port_name = MusEGlobal::audioDevice->canonicalPortName(our_port);
      if(!our_port_name)
        continue;
      RouteList* rl = ai->inRoutes();
      for(ciRoute ir = rl->begin(); ir != rl->end(); ++ir) 
      {
        if(ir->type != Route::JACK_ROUTE || ir->channel != ch)  
          continue;
        const char* route_name = ir->persistentJackPortName;
        if(!MusEGlobal::audioDevice->findPort(route_name))
          continue;
        //if(!MusEGlobal::audioDevice->portConnectedTo(our_port, route_name))
          MusEGlobal::audioDevice->connect(route_name, our_port_name);
      }
    }
  }
}

//---------------------------------------------------------
//   insertTrack0
//---------------------------------------------------------

void Song::insertTrack0(Track* track, int idx)
      {
      int n;
      switch(track->type()) {
            case Track::MIDI:
            case Track::DRUM:
            case Track::NEW_DRUM:
                  _midis.push_back((MidiTrack*)track);
                  addPortCtrlEvents(((MidiTrack*)track));
                  
                  break;
            case Track::WAVE:
                  _waves.push_back((MusECore::WaveTrack*)track);
                  break;
            case Track::AUDIO_OUTPUT:
                  _outputs.push_back((AudioOutput*)track);
                  break;
            case Track::AUDIO_GROUP:
                  _groups.push_back((AudioGroup*)track);
                  break;
            case Track::AUDIO_AUX:
                  _auxs.push_back((AudioAux*)track);
                  break;
            case Track::AUDIO_INPUT:
                  _inputs.push_back((AudioInput*)track);
                  break;
            case Track::AUDIO_SOFTSYNTH:
                  {
                  SynthI* s = (SynthI*)track;
                  Synth* sy = s->synth();
                  if (!s->isActivated()) {
                        // Persistent storage: If the synth is not found allow the track to load.
                        // It's OK if s is NULL. initInstance needs to do a few things.
                        s->initInstance(sy, s->name());
                        }
                  MusEGlobal::midiDevices.add(s);
                  midiInstruments.push_back(s);
                  _synthIs.push_back(s);
                  }
                  break;
            default:
                  fprintf(stderr, "unknown track type %d\n", track->type());
                  return;
            }

      // initialize missing aux send
      iTrack i = _tracks.index2iterator(idx);
      
      _tracks.insert(i, track);
      
      n = _auxs.size();
      for (iTrack i = _tracks.begin(); i != _tracks.end(); ++i) {
            if ((*i)->isMidiTrack())
                  continue;
            MusECore::AudioTrack* wt = (MusECore::AudioTrack*)*i;
            if (wt->hasAuxSend()) {
                  wt->addAuxSend(n);
                  }
            }

      //  add routes

      if (track->isMidiTrack())          // p3.3.50
      {
            const RouteList* rl = track->inRoutes();
            for (ciRoute r = rl->begin(); r != rl->end(); ++r)
            {
                  switch(r->type)
                  {
                    case Route::MIDI_PORT_ROUTE:  {
                      Route src(track, r->channel);
                      MusEGlobal::midiPorts[r->midiPort].outRoutes()->push_back(src);  }
                    break;
                    case Route::TRACK_ROUTE:
                    case Route::JACK_ROUTE:
                    case Route::MIDI_DEVICE_ROUTE:
                    break;
                  }
            }
            rl = track->outRoutes();
            for (ciRoute r = rl->begin(); r != rl->end(); ++r)
            {
                  switch(r->type)
                  {
                    case Route::MIDI_PORT_ROUTE:  {
                      Route src(track, r->channel);
                      MusEGlobal::midiPorts[r->midiPort].inRoutes()->push_back(src);  }
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
            const RouteList* rl = track->inRoutes();
            for (ciRoute r = rl->begin(); r != rl->end(); ++r)
            {
                  switch(r->type)
                  {
                    case Route::TRACK_ROUTE: {
                      Route src(track, r->remoteChannel, r->channels);
                      src.remoteChannel = r->channel;
                      r->track->outRoutes()->push_back(src); 
                      // Is the source an Aux Track or else does it have Aux Tracks routed to it?
                      // Update this track's aux ref count.     p4.0.37
                      if(r->track->auxRefCount())
                        track->updateAuxRoute( r->track->auxRefCount(), NULL );
                      else if(r->track->type() == Track::AUDIO_AUX)
                        track->updateAuxRoute( 1, NULL );
                    }
                    break;
                    case Route::MIDI_PORT_ROUTE:
                    case Route::JACK_ROUTE:
                    case Route::MIDI_DEVICE_ROUTE:
                    break;
                  }
            }
            rl = track->outRoutes();
            for (ciRoute r = rl->begin(); r != rl->end(); ++r)
            {
                  switch(r->type)
                  {
                    case Route::TRACK_ROUTE: {
                      Route src(track, r->remoteChannel, r->channels);
                      src.remoteChannel = r->channel;
                      r->track->inRoutes()->push_back(src); 
                      // Is this track an Aux Track or else does it have Aux Tracks routed to it?
                      // Update the other track's aux ref count and all tracks it is connected to.
                      if(track->auxRefCount())
                        r->track->updateAuxRoute( track->auxRefCount(), NULL );
                      else if(track->type() == Track::AUDIO_AUX)
                        r->track->updateAuxRoute( 1, NULL );
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

//---------------------------------------------------------
//   insertTrackOperation
//---------------------------------------------------------

void Song::insertTrackOperation(Track* track, int idx, PendingOperationList& ops)
{
      //int n;
      void* sec_track_list = 0;
      switch(track->type()) {
            case Track::MIDI:
            case Track::DRUM:
            case Track::NEW_DRUM:
                  sec_track_list = &_midis;
                  break;
            case Track::WAVE:
                  sec_track_list = &_waves;
                  break;
            case Track::AUDIO_OUTPUT:
                  sec_track_list = &_outputs;
                  break;
            case Track::AUDIO_GROUP:
                  sec_track_list = &_groups;
                  break;
            case Track::AUDIO_AUX:
                  sec_track_list = &_auxs;
                  break;
            case Track::AUDIO_INPUT:
                  sec_track_list = &_inputs;
                  break;
            case Track::AUDIO_SOFTSYNTH:
                  {
                  SynthI* s = static_cast<SynthI*>(track);
                  MusEGlobal::midiDevices.addOperation(s, ops);
                  ops.add(PendingOperationItem(&midiInstruments, s, PendingOperationItem::AddMidiInstrument));
                  sec_track_list = &_synthIs;
                  }
                  break;
            default:
                  fprintf(stderr, "unknown track type %d\n", track->type());
                  return;
            }

      ops.add(PendingOperationItem(&_tracks, track, idx, PendingOperationItem::AddTrack, sec_track_list));
      
      addPortCtrlEvents(track, ops);
      
      // NOTE: Aux sends: 
      // Initializing of this track and/or others' aux sends is done at the end of Song::execute/revertOperationGroup2().
      // NOTE: Routes:
      // Routes are added in the PendingOperationItem::AddTrack section of PendingOperationItem::executeRTStage().
}

void writeStringToFile(FILE *filePointer, const char *writeString)
{
  if (MusEGlobal::debugMsg)
    std::cout << writeString;
  fputs(writeString, filePointer);
}

//---------------------------------------------------------
//   removeTrackOperation
//---------------------------------------------------------

void Song::removeTrackOperation(Track* track, PendingOperationList& ops)
{
      removePortCtrlEvents(track, ops);
      void* sec_track_list = 0;
      switch(track->type()) {
            case Track::MIDI:
            case Track::DRUM:
            case Track::NEW_DRUM:
                    sec_track_list = &_midis;
            break;
            case Track::WAVE:
                    sec_track_list = &_waves;
            break;
            case Track::AUDIO_OUTPUT:
                    sec_track_list = &_outputs;
            break;
            case Track::AUDIO_INPUT:
                    sec_track_list = &_inputs;
            break;
            case Track::AUDIO_GROUP:
                    sec_track_list = &_groups;
            break;
            case Track::AUDIO_AUX:
                    sec_track_list = &_auxs;
            break;
            case Track::AUDIO_SOFTSYNTH:
            {
                  SynthI* s = static_cast<SynthI*>(track);
                  iMidiInstrument imi = midiInstruments.find(s);
                  if(imi != midiInstruments.end())
                    ops.add(PendingOperationItem(&midiInstruments, imi, PendingOperationItem::DeleteMidiInstrument));
                  
                  iMidiDevice imd = MusEGlobal::midiDevices.find(s);
                  if(imd != MusEGlobal::midiDevices.end())
                    ops.add(PendingOperationItem(&MusEGlobal::midiDevices, imd, PendingOperationItem::DeleteMidiDevice));
                  
                  if(s->midiPort() != -1)
                  {
                    // synthi is attached
                    // Oops, hey this was wrong before, should have been zero.
                    MusEGlobal::audio->msgSetMidiDevice(&MusEGlobal::midiPorts[s->midiPort()], 0);
                  }
                  
                  sec_track_list = &_synthIs;
            }
            break;
      }

      ops.add(PendingOperationItem(&_tracks, track, PendingOperationItem::DeleteTrack, sec_track_list));

      // NOTE: Routes:
      // Routes are removed in the PendingOperationItem::DeleteTrack section of PendingOperationItem::executeRTStage().
}

//---------------------------------------------------------
//   executeScript
//---------------------------------------------------------
void Song::executeScript(QWidget *parent, const char* scriptfile, PartList* parts, int quant, bool onlyIfSelected)
{
      // a simple format for external processing
      // will be extended if there is a need
      //
      // Semantics:
      // TIMESIG <n> <z>
      // PARTLEN <len in ticks>
      // BEATLEN <len in ticks>
      // QUANTLEN <len in ticks>
      // NOTE <tick> <nr> <len in ticks> <velocity>
      // CONTROLLER <tick> <a> <b> <c>
      //

      if (onlyIfSelected) // if this is set means we are probably inside a midi editor and we ask again to be sure
      {
        if(QMessageBox::question(parent, QString("Process events"),
            tr("Do you want to process ALL or only selected events?"), tr("&Selected"), tr("&All"),
            QString(), 0, 1 ) == 1)
        {
            onlyIfSelected = false;
        }
      }
      QProgressDialog progress(parent);
      progress.setLabelText("Process parts");
      progress.setRange(0,parts->size());
      progress.setValue(0);
      progress.setCancelButton(0);
      MusEGlobal::song->startUndo(); // undo this entire block
      for (iPart i = parts->begin(); i != parts->end(); i++) {
            //const char* tmp = tmpnam(NULL);
            char tmp[16] = "muse-tmp-XXXXXX";
            char tempStr[200];
            int fd = mkstemp(tmp);
            if (MusEGlobal::debugMsg)
              fprintf(stderr, "executeScript: script input filename=%s\n",tmp);

            FILE *fp = fdopen(fd , "w");
            MidiPart *part = (MidiPart*)(i->second);
            if (MusEGlobal::debugMsg)
              fprintf(stderr, "SENDING TO SCRIPT, part start: %d\n", part->tick());

            int z, n;
            MusEGlobal::sigmap.timesig(part->tick(), z, n);
            sprintf(tempStr, "TIMESIG %d %d\n", z, n);
            writeStringToFile(fp,tempStr);
            sprintf(tempStr, "PART %d %d\n", part->tick(), part->lenTick());
            writeStringToFile(fp,tempStr);
            sprintf(tempStr, "BEATLEN %d\n", MusEGlobal::sigmap.ticksBeat(part->tick()));
            writeStringToFile(fp,tempStr);
            sprintf(tempStr, "QUANTLEN %d\n", quant);
            writeStringToFile(fp,tempStr);

            if (MusEGlobal::debugMsg)
              std::cout << "Events in part " << part->events().size() << std::endl;

            EventList elist = part->events();
            for (ciEvent e = elist.begin(); e != elist.end(); e++)
            {
              Event ev = e->second;

              if (ev.isNote())
              {
                if (onlyIfSelected && ev.selected() == false)
                  continue;

                sprintf(tempStr,"NOTE %d %d %d %d\n", ev.tick(), ev.dataA(),  ev.lenTick(), ev.dataB());
                writeStringToFile(fp,tempStr);

                // Operation is undoable but do not start/end undo.
                // Indicate do not do port controller values and clone parts.
                MusEGlobal::song->applyOperation(UndoOp(UndoOp::DeleteEvent,
                                 ev, part, false, false), Song::OperationUndoable);
                
              } else if (ev.type()==Controller) {
                sprintf(tempStr,"CONTROLLER %d %d %d %d\n", ev.tick(), ev.dataA(), ev.dataB(), ev.dataC());
                writeStringToFile(fp,tempStr);
                // Operation is undoable but do not start/end undo.
                // Indicate do not do port controller values and clone parts.
                MusEGlobal::song->applyOperation(MusECore::UndoOp(MusECore::UndoOp::DeleteEvent,
                                 ev, part, false, false), Song::OperationUndoable);
              }
            }
            fclose(fp);

            QStringList arguments;
            arguments << tmp;

            QProcess *myProcess = new QProcess(parent);
            myProcess->start(scriptfile, arguments);
            myProcess->waitForFinished();
            QByteArray errStr = myProcess->readAllStandardError();

            if (myProcess->exitCode()) {
              QMessageBox::warning(parent, tr("MusE - external script failed"),
                                   tr("MusE was unable to launch the script, error message:\n%1").arg(QString(errStr)));
              endUndo(SC_EVENT_REMOVED);
              return;
            }
            if (errStr.size()> 0) {
              fprintf(stderr, "script execution produced the following error:\n%s\n", QString(errStr).toLatin1().data());
            }
            QFile file(tmp);
            if (MusEGlobal::debugMsg)
              file.copy(file.fileName() + "_input");

            if ( file.open( QIODevice::ReadOnly ) )
            {
              QTextStream stream( &file );
              QString line;
              if (MusEGlobal::debugMsg)
                fprintf(stderr, "RECEIVED FROM SCRIPT:\n");
              while ( !stream.atEnd() )
              {
                line = stream.readLine(); // line of text excluding '\n'
                if (MusEGlobal::debugMsg) {
                  std::cout << line.toStdString() << std::endl;
                }

                if (line.startsWith("NOTE"))
                {
                  QStringList sl = line.split(" ");

                  Event e(Note);
                  int tick = sl[1].toInt();
                  int pitch = sl[2].toInt();
                  int len = sl[3].toInt();
                  int velo = sl[4].toInt();
                  fprintf(stderr, "extraced %d %d %d %d\n", tick, pitch, len, velo);
                  e.setTick(tick);
                  e.setPitch(pitch);
                  e.setVelo(velo);
                  e.setLenTick(len);
                  // Operation is undoable but do not start/end undo.
                  // Indicate do not do port controller values and clone parts.
                  MusEGlobal::song->applyOperation(UndoOp(UndoOp::AddEvent, 
                              e, part, false, false), Song::OperationUndoable);
                }
                if (line.startsWith("CONTROLLER"))
                {
                  QStringList sl = line.split(" ");

                  Event e(Controller);
                  int a = sl[2].toInt();
                  int b = sl[3].toInt();
                  int c = sl[4].toInt();
                  e.setA(a);
                  e.setB(b);
                  e.setB(c);
                  // Operation is undoable but do not start/end undo.
                  // Indicate do not do port controller values and clone parts.
                  MusEGlobal::song->applyOperation(UndoOp(UndoOp::AddEvent, 
                              e, part, false, false), Song::OperationUndoable);
                }
              }
              file.close();
            }

            if (!MusEGlobal::debugMsg) // if we are writing debug info we also keep the script data
              remove(tmp);
            progress.setValue(progress.value()+1);
      } // for

      endUndo(SC_EVENT_REMOVED);
}


void Song::populateScriptMenu(QMenu* menuPlugins, ScriptReceiver* receiver)
{
      // List scripts
      QString distScripts = MusEGlobal::museGlobalShare + "/scripts";
      QString userScripts = MusEGlobal::configPath + "/scripts";

      QFileInfo distScriptsFi(distScripts);
      if (distScriptsFi.isDir()) {
            QDir dir = QDir(distScripts);
            dir.setFilter(QDir::Executable | QDir::Files);
            deliveredScriptNames = dir.entryList();
            }
      QFileInfo userScriptsFi(userScripts);
      if (userScriptsFi.isDir()) {
            QDir dir(userScripts);
            dir.setFilter(QDir::Executable | QDir::Files);
            userScriptNames = dir.entryList();
            }

      if (deliveredScriptNames.size() > 0 || userScriptNames.size() > 0) {
            int id = 0;
            if (deliveredScriptNames.size() > 0) {
                  for (QStringList::Iterator it = deliveredScriptNames.begin(); it != deliveredScriptNames.end(); it++, id++) {
                        QAction* act = menuPlugins->addAction(*it);
                        connect(act, &QAction::triggered, [receiver, id]() { receiver->receiveExecDeliveredScript(id); } );
                        }
                  menuPlugins->addSeparator();
                  }
            if (userScriptNames.size() > 0) {
                  for (QStringList::Iterator it = userScriptNames.begin(); it != userScriptNames.end(); it++, id++) {
                        QAction* act = menuPlugins->addAction(*it);
                        connect(act, &QAction::triggered, [receiver, id]() { receiver->receiveExecUserScript(id); } );
                        }
                  menuPlugins->addSeparator();
                  }
            }
      return;
}

//---------------------------------------------------------
//   restartRecording
//   Called from gui thread only.
//---------------------------------------------------------

void Song::restartRecording(bool discard)
{
  // FIXME After recording, it never makes it past here because recording has long since stopped.
  //       Although, it should work WHILE recording.
  //       We may need a track flag like 'justRecorded' or 'lastRecorded' something...
  if(!MusEGlobal::audio->isRecording() || !MusEGlobal::audio->isRunning())
    return;

  // Do not copy parts or controller graphs. When ASSIGN_STD_CTRLS is NOT included, it will
  //  copy just the standard controller current values, but not the graphs.
  // FIXME: Although we would like to copy plugins, that may get expensive after a while.
  //        So instead try to create a group track with the plugins and route the track and all
  //         the retake tracks through the group track.
  const int clone_flags = Track::ASSIGN_PROPERTIES | Track::ASSIGN_ROUTES | Track::ASSIGN_DEFAULT_ROUTES | Track::ASSIGN_DRUMLIST;
  
  MusECore::Undo operations;
  
  if(!discard)
  {
      MusEGlobal::audio->recordStop(true /*restart record*/, &operations);
      processAutomationEvents(&operations);
  }
  
  //clear all recorded midi events and wave files
  QStringList new_track_names;
  
  int idx_cnt = 0;
  for(size_t i = 0; i < _tracks.size(); i++)
  {
      Track *cTrk = _tracks[i];
      if(!cTrk->recordFlag())
        continue;
      Track *nTrk = NULL;
      if(!discard)
      {
        nTrk = cTrk->clone(clone_flags);
        
        QString track_name = cTrk->name();
        int counter=0;
        int numberIndex=0;
        // Assign a new name to the cloned track.
        numberIndex = track_name.lastIndexOf("#");
        // according to Qt doc for lastIndexOf it should return -1 when not found
        // apparently it returns str_size+1 ?! Let's catch both
        if (numberIndex == -1 || numberIndex > track_name.size()) {
          track_name += " #";                                       
          numberIndex = track_name.size();
          counter=1;
        }
        else {
          counter = track_name.right(track_name.size()-numberIndex-1).toInt();
        }
        QString tempName;
        while(true) {
          tempName = track_name.left(numberIndex+1) + QString::number(++counter);
          if(new_track_names.indexOf(tempName) >= 0)
            continue;
          Track* track = findTrack(tempName);
          if(track == 0)
          {
            nTrk->setName(tempName);
            break;
          }
        }

        new_track_names.push_back(nTrk->name());
        
        const int idx = _tracks.index(cTrk) + idx_cnt++;
        operations.push_back(MusECore::UndoOp(MusECore::UndoOp::AddTrack, idx + 1, nTrk));
        operations.push_back(UndoOp(UndoOp::SetTrackMute, cTrk, true));
        operations.push_back(UndoOp(UndoOp::SetTrackRecord, cTrk, false));
        setRecordFlag(nTrk, true, &operations);
      }
      if (cTrk->isMidiTrack())
      {
        if(discard)
        {
            ((MidiTrack *)cTrk)->mpevents.clear();
        }
      }
      else if (cTrk->type() == Track::WAVE)
      {
        if(discard)
        {
            ((WaveTrack*)cTrk)->setRecFile(NULL);
            ((WaveTrack*)cTrk)->resetMeter();
            ((WaveTrack*)cTrk)->prepareRecording();
        }
        else
        {
            ((WaveTrack*)nTrk)->prepareRecording();
        }
      }
  }
  
  applyOperationGroup(operations);
  
  setPos(Song::CPOS, MusEGlobal::audio->getStartRecordPos());
  //MusEGlobal::audioDevice->startTransport();
}

//---------------------------------------------------------
//   getScriptPath
//---------------------------------------------------------
QString Song::getScriptPath(int id, bool isdelivered)
{
      if (isdelivered) {
            QString path = MusEGlobal::museGlobalShare + "/scripts/" + deliveredScriptNames[id];
            return path;
            }

      QString path = MusEGlobal::configPath + "/scripts/" + userScriptNames[id - deliveredScriptNames.size()];
      return path;
}

void Song::informAboutNewParts(const std::map< const Part*, std::set<const Part*> >& param)
{
  emit newPartsCreated(param);
}

void Song::informAboutNewParts(const Part* orig, const Part* p1, const Part* p2, const Part* p3, const Part* p4, const Part* p5, const Part* p6, const Part* p7, const Part* p8, const Part* p9)
{
  std::map<const Part*, std::set<const Part*> > temp;
  
  temp[orig].insert(p1);
  temp[orig].insert(p2);
  temp[orig].insert(p3);
  temp[orig].insert(p4);
  temp[orig].insert(p5);
  temp[orig].insert(p6);
  temp[orig].insert(p7);
  temp[orig].insert(p8);
  temp[orig].insert(p9);
  temp[orig].erase(static_cast<const Part*>(NULL));
  temp[orig].erase(orig);
  
  informAboutNewParts(temp);
}

//---------------------------------------------------------
//   StretchList operations
//---------------------------------------------------------

void Song::stretchModifyOperation(
  StretchList* stretch_list, StretchListItem::StretchEventType type, double value, PendingOperationList& ops) const
{
  ops.add(PendingOperationItem(type, stretch_list, value, PendingOperationItem::ModifyStretchListRatio));
}

void Song::stretchListAddOperation(
  StretchList* stretch_list, StretchListItem::StretchEventType type, MuseFrame_t frame, double value, PendingOperationList& ops) const
{
  iStretchListItem ie = stretch_list->find(frame);
  if(ie != stretch_list->end())
    ops.add(PendingOperationItem(type, stretch_list, ie, frame, value, PendingOperationItem::ModifyStretchListRatioAt));
  else
    ops.add(PendingOperationItem(type, stretch_list, frame, value, PendingOperationItem::AddStretchListRatioAt));
}

void Song::stretchListDelOperation(
  StretchList* stretch_list, int types, MuseFrame_t frame, PendingOperationList& ops) const
{
  // Do not delete the item at zeroth frame.
  if(frame == 0)
    return;
  
  iStretchListItem e = stretch_list->find(frame);
  if (e == stretch_list->end()) {
        ERROR_TIMESTRETCH(stderr, "Song::stretchListDelOperation frame:%ld not found\n", frame);
        return;
        }
  PendingOperationItem poi(types, stretch_list, e, PendingOperationItem::DeleteStretchListRatioAt);
  // NOTE: Deletion is done in post-RT stage 3.
  ops.add(poi);
}

void Song::stretchListModifyOperation(
  StretchList* stretch_list, StretchListItem::StretchEventType type, MuseFrame_t frame, double value, PendingOperationList& ops) const
{
  iStretchListItem ie = stretch_list->find(frame);
  if(ie == stretch_list->end()) {
        ERROR_TIMESTRETCH(stderr, "Song::stretchListModifyOperation frame:%ld not found\n", frame);
        return;
        }
  ops.add(PendingOperationItem(type, stretch_list, ie, frame, value, PendingOperationItem::ModifyStretchListRatioAt));
}

void Song::setAudioConvertersOfflineOperation(
  bool isOffline
  )
{
  WaveTrackList* wtl = waves();
  if(wtl->empty())
    return;

  PendingOperationList ops;

  AudioConverterSettingsGroup* settings;
  bool isLocalSettings;
  bool doStretch;
  bool doResample;
  AudioConverterSettings::ModeType cur_mode;
  AudioConverterPluginI* cur_converter;
  AudioConverterPluginI* converter;
  const WaveTrack* wt;
  const PartList* pl;
  const Part* pt;
  ciPart pl_end;
  ciEvent el_end;
  SndFileR sndfile;
  ciWaveTrack wtl_end = wtl->cend();
  for(ciWaveTrack iwt = wtl->cbegin(); iwt != wtl_end; ++iwt)
  {
    wt = *iwt;
    pl = wt->cparts();
    pl_end = pl->cend();
    for(ciPart ip = pl->cbegin(); ip != pl_end; ++ip)
    {
      pt = ip->second;
      const EventList& el = pt->events();
      el_end = el.end();
      for(ciEvent ie = el.cbegin(); ie != el_end; ++ie)
      {
        const Event& e = ie->second;
        sndfile = e.sndFile();
        //if(sndfile.isNull())
        //  continue;

        // Check if we are to use any converters at all.
        if(!sndfile.useConverter())
          continue;

        // Check if the current realtime converter is already in offline mode.
        cur_converter = sndfile.staticAudioConverter(AudioConverterSettings::RealtimeMode);
        if(cur_converter)
        {
          cur_mode = cur_converter->mode();
          if((isOffline && cur_mode == AudioConverterSettings::OfflineMode) ||
            (!isOffline && cur_mode == AudioConverterSettings::RealtimeMode))
            continue;
        }
          
        settings = sndfile.audioConverterSettings()->useSettings() ?
          sndfile.audioConverterSettings() : MusEGlobal::defaultAudioConverterSettings;
        isLocalSettings = sndfile.audioConverterSettings()->useSettings();

        doStretch = sndfile.isStretched();
        doResample = sndfile.isResampled();

        // For offline mode, we COULD create a third converter just for it, apart from the main
        //  and UI converters. But our system doesn't have a third converter (yet) - and it may
        //  or may not get one, we'll see. Still, the operation supports setting it, in case.
        // So instead, in offline mode we switch out the main converter for one with with offline settings.
        converter = sndfile.setupAudioConverter(
          settings,
          MusEGlobal::defaultAudioConverterSettings,
          isLocalSettings,
          isOffline ? 
            AudioConverterSettings::OfflineMode :
            AudioConverterSettings::RealtimeMode,
          doResample,
          doStretch);

          // No point if there's already no converter.
          if(!converter && !cur_converter)
            continue;

          // REMOVE Tim. samplerate. Added. Diagnostics.
          fprintf(stderr, "Song::setAudioConvertersOfflineOperation Setting sndfile:%s to isOffline:%d\n",
                  sndfile.name().toLocal8Bit().constData(), isOffline);
          
          ops.add(PendingOperationItem(
            sndfile,
            converter,   // Main converter. 
            PendingOperationItem::SetAudioConverterOfflineMode));
      }
    }
  }

  MusEGlobal::audio->msgExecutePendingOperations(ops, true);
}

void Song::modifyAudioConverterSettingsOperation(
  SndFileR sndfile,
  AudioConverterSettingsGroup* settings,
  AudioConverterSettingsGroup* defaultSettings,
  bool isLocalSettings,
  PendingOperationList& ops
  ) const
{
  if(!sndfile.useConverter())
    return;
  const bool isOffline = sndfile.isOffline();
  const bool doStretch = sndfile.isStretched();
  const bool doResample = sndfile.isResampled();
  // For offline mode, we COULD create a third converter just for it, apart from the main
  //  and UI converters. But our system doesn't have a third converter (yet) - and it may
  //  or may not get one, we'll see. Still, the operation supports setting it, in case.
  // So instead, in offline mode we switch out the main converter for one with with offline settings.
  AudioConverterPluginI* converter = sndfile.setupAudioConverter(
    settings,
    defaultSettings,
    isLocalSettings,
    isOffline ?
      AudioConverterSettings::OfflineMode :
      AudioConverterSettings::RealtimeMode,
    doResample,
    doStretch);

  AudioConverterPluginI* converterUI = sndfile.setupAudioConverter(
    settings,
    defaultSettings,
    isLocalSettings,
    AudioConverterSettings::GuiMode,
    doResample,
    doStretch);

//   if(!converter && !converterUI)
//     return;

  // We want to change the settings, and the converters if necessary...
  ops.add(PendingOperationItem(
    sndfile,
    settings,
    PendingOperationItem::ModifyLocalAudioConverterSettings));

  ops.add(PendingOperationItem(
    sndfile,
    converter,   // Main converter. 
    converterUI, // UI converter.
    PendingOperationItem::ModifyLocalAudioConverter));
}

void Song::modifyAudioConverterOperation(
  SndFileR sndfile,
  PendingOperationList& ops,
  bool doResample,
  bool doStretch) const
{
  if(!sndfile.useConverter())
    return;
  const bool isOffline = sndfile.isOffline();
  AudioConverterSettingsGroup* settings = sndfile.audioConverterSettings()->useSettings() ?
    sndfile.audioConverterSettings() : MusEGlobal::defaultAudioConverterSettings;

  const bool isLocalSettings = sndfile.audioConverterSettings()->useSettings();

  // For offline mode, we COULD create a third converter just for it, apart from the main
  //  and UI converters. But our system doesn't have a third converter (yet) - and it may
  //  or may not get one, we'll see. Still, the operation supports setting it, in case.
  // So instead, in offline mode we switch out the main converter for one with with offline settings.
  AudioConverterPluginI* converter = sndfile.setupAudioConverter(
    settings,
    MusEGlobal::defaultAudioConverterSettings,
    isLocalSettings,
    isOffline ?
      AudioConverterSettings::OfflineMode :
      AudioConverterSettings::RealtimeMode,
    doResample,
    doStretch);

  AudioConverterPluginI* converterUI = sndfile.setupAudioConverter(
    settings,
    MusEGlobal::defaultAudioConverterSettings,
    isLocalSettings,
    AudioConverterSettings::GuiMode,
    doResample,
    doStretch);

//   if(!converter && !converterUI)
//     return;

  ops.add(PendingOperationItem(
    sndfile,
    converter,   // Main converter.
    converterUI, // UI converter.
    PendingOperationItem::ModifyLocalAudioConverter));
}

void Song::modifyStretchListOperation(SndFileR sndfile, int type, double value, PendingOperationList& ops) const
{
  if(!sndfile.useConverter())
    return;
  ops.add(PendingOperationItem(type, sndfile.stretchList(), value, PendingOperationItem::ModifyStretchListRatio));
}

void Song::addAtStretchListOperation(SndFileR sndfile, int type, MuseFrame_t frame, double value, PendingOperationList& ops) const
{
  if(!sndfile.useConverter())
    return;
  StretchList* sl = sndfile.stretchList();
  stretchListAddOperation(sl, StretchListItem::StretchEventType(type), frame, value, ops);
  
  bool wantStretch = false;
  bool wantResample = sndfile.sampleRateDiffers();
  bool wantPitch = false;
  const bool haveStretch = sndfile.isStretched();
  const bool haveResample = sndfile.isResampled() || wantResample;
  const bool havePitch = sndfile.isPitchShifted();
  
  //// If the requested value is anything other than 1.0, request converters.
  //if(value != 1.0)
  //{
    switch(type)
    {
      case StretchListItem::StretchEvent:
        wantStretch = true;
      break;
      case StretchListItem::SamplerateEvent:
        wantResample = true;
      break;
      case StretchListItem::PitchEvent:
        wantPitch = true;
      break;
    }
  //}
  
  if((wantStretch  && !haveStretch) || 
     (wantResample && !haveResample) || 
     (wantPitch    && !havePitch))
  {
    const bool doStretch  = wantStretch  ? true : haveStretch;
    const bool doResample = wantResample ? true : haveResample;
  //const bool doPitch    = wantPitch    ? true : havePitch;
    
    modifyAudioConverterOperation(sndfile, ops, doResample, doStretch);
  }
}

void Song::delAtStretchListOperation(SndFileR sndfile, int types, MuseFrame_t frame, PendingOperationList& ops) const
{
  if(!sndfile.useConverter())
    return;
  // Do not delete the item at zeroth frame.
  if(frame == 0)
    return;
  
  StretchList* sl = sndfile.stretchList();
  stretchListDelOperation(sl, types, frame, ops);

  StretchListInfo info =  sl->testDelListOperation(types, frame);
  
  const bool srdiffers = sndfile.sampleRateDiffers();
  const bool wantStretch  = info._isStretched;
  const bool wantResample = info._isResampled || srdiffers;
  const bool wantPitch    = info._isPitchShifted;
  
  const bool haveStretch  = sndfile.isStretched();
  const bool haveResample = sndfile.isResampled() || srdiffers;
  const bool havePitch    = sndfile.isPitchShifted();
  
  if((!wantStretch  && haveStretch) || 
     (!wantResample && haveResample) || 
     (!wantPitch    && havePitch))
  {
    const bool doStretch  = !wantStretch  ? false : haveStretch;
    const bool doResample = !wantResample ? false : haveResample;
  //const bool doPitch    = !wantPitch    ? false : havePitch;
    
    modifyAudioConverterOperation(sndfile, ops, doResample, doStretch);
  }
}

void Song::modifyAtStretchListOperation(SndFileR sndfile, int type, MuseFrame_t frame, double value, PendingOperationList& ops) const
{
  if(!sndfile.useConverter())
    return;
  StretchList* sl = sndfile.stretchList();
  stretchListModifyOperation(sl, StretchListItem::StretchEventType(type), frame, value, ops);
  
  bool wantStretch = false;
  bool wantResample = sndfile.sampleRateDiffers();
  bool wantPitch = false;
  const bool haveStretch = sndfile.isStretched();
  const bool haveResample = sndfile.isResampled() || wantResample;
  const bool havePitch = sndfile.isPitchShifted();
  
  //// If the requested value is anything other than 1.0, request converters.
  //if(value != 1.0)
  //{
    switch(type)
    {
      case StretchListItem::StretchEvent:
        wantStretch = true;
      break;
      case StretchListItem::SamplerateEvent:
        wantResample = true;
      break;
      case StretchListItem::PitchEvent:
        wantPitch = true;
      break;
    }
  //}
  
  if((wantStretch  && !haveStretch) || 
     (wantResample && !haveResample) || 
     (wantPitch    && !havePitch))
  {
    const bool doStretch  = wantStretch  ? true : haveStretch;
    const bool doResample = wantResample ? true : haveResample;
  //const bool doPitch    = wantPitch    ? true : havePitch;
    
    modifyAudioConverterOperation(sndfile, ops, doResample, doStretch);
  }
}

void Song::modifyDefaultAudioConverterSettingsOperation(AudioConverterSettingsGroup* settings, 
                                                        PendingOperationList& ops)
{
  // First, schedule the change to the actual default settings pointer variable.
  ops.add(PendingOperationItem(settings, PendingOperationItem::ModifyDefaultAudioConverterSettings));
  
  // Now, schedule changes to each wave event if necessary.
  // Note that at this point the above default change has not occurred yet, 
  //  so we must tell it to use what the settings WILL BE, not what they are now.
  for(ciWaveTrack it = MusEGlobal::song->waves()->cbegin(); it != MusEGlobal::song->waves()->cend(); ++it)
  {
    const WaveTrack* wtrack = *it;
    for(ciPart ip = wtrack->cparts()->cbegin(); ip != wtrack->cparts()->cend(); ++ip)
    {
      const Part* part = ip->second;
      for(ciEvent ie = part->events().cbegin(); ie != part->events().cend(); ++ie)
      {
        const Event& e = ie->second;
        if(e.type() != Wave)
          continue;
        SndFileR sndfile = e.sndFile();
        if(!sndfile.useConverter())
          continue;
        const AudioConverterSettingsGroup* cur_ev_settings = sndfile.audioConverterSettings();
        // Is the event using its own local settings? Ignore.
        if(!cur_ev_settings || cur_ev_settings->useSettings())
          continue;

        const bool isOffline = sndfile.isOffline();
        const bool doStretch = sndfile.isStretched();
        const bool doResample = sndfile.isResampled();
        // For offline mode, we COULD create a third converter just for it, apart from the main
        //  and UI converters. But our system doesn't have a third converter (yet) - and it may
        //  or may not get one, we'll see. Still, the operation supports setting it, in case.
        // So instead, in offline mode we switch out the main converter for one with with offline settings.
        AudioConverterPluginI* converter = sndfile.setupAudioConverter(
          settings,
          settings,
          false,  // false = Default, non-local settings.
          isOffline ?
            AudioConverterSettings::OfflineMode :
            AudioConverterSettings::RealtimeMode,
          doResample,
          doStretch);

        AudioConverterPluginI* converterUI = sndfile.setupAudioConverter(
          settings,
          settings,
          false,  // false = Default, non-local settings.
          AudioConverterSettings::GuiMode,
          doResample,
          doStretch);

      //   if(!converter && !converterUI)
      //     return;

        // We only want to change the converters, if necessary.
        ops.add(PendingOperationItem(
          sndfile,
          converter,   // Main converter. 
          converterUI, // UI converter.
          PendingOperationItem::ModifyLocalAudioConverter));
      }
    }
  }
}

//---------------------------------------------------------
//   updateTransportPos
//   called from GUI context
// SPECIAL for tempo or master changes: In stop mode we want
//  the transport to locate to the correct frame. In play mode
//  we simply let the transport progress naturally but we 'fake'
//  a new representation of transport position (_pos) in Audio::reSyncAudio()
//  as part of the realtime part of the tempo change operation.
////// By now, our audio transport position (_pos) has the new tick and frame.
// We need to seek AFTER any song changed slots are called in case widgets
//  are removed etc. etc. before the posChanged signal is emitted when setPos()
//  is called from the seek recognition.
//---------------------------------------------------------

void Song::updateTransportPos(const SongChangedStruct_t& flags)
{
  if(!MusEGlobal::audio->isPlaying() && (flags & (SC_TEMPO | SC_MASTER)))
  {
    //const MusECore::Pos ap = MusEGlobal::audio->tickAndFramePos();
    // Don't touch Audio::_pos, make a copy.
    //const MusECore::Pos ap = MusEGlobal::audio->pos();
    
    // Don't touch Audio::_pos, make a copy or take the tick.
    // Note that this is only tick accurate, not frame accurate,
    //  ie. it can only recalculate a new frame from the given tick. 
    const MusECore::Pos p(MusEGlobal::audio->tickPos());

//     // Don't touch the original, make a copy.
//     const MusECore::Pos p(cPos());
    
    MusEGlobal::audioDevice->seekTransport(p.frame());
  }
}

//---------------------------------------------------------
//   adjustMarkerListOperation
//   Items between startPos and startPos + diff are removed.
//   Items after startPos + diff are adjusted 'diff' number of ticks.
//---------------------------------------------------------

bool Song::adjustMarkerListOperation(MarkerList* markerlist, unsigned int startPos, int diff, PendingOperationList& ops)
{
  if(!markerlist || markerlist->empty() || diff == 0)
    return false;
  
  MarkerList* new_markerlist = new MarkerList();
  for(ciMarker i = markerlist->begin(); i != markerlist->end(); ++i)
  {
    const Marker& m = i->second;
    unsigned int tick = m.tick();
    if(tick >= startPos)
    {
      if(tick >= startPos + diff)
      {
        // Grab a copy but with a new ID.
        Marker newMarker = m.copy();
        newMarker.setTick(tick - diff);
        new_markerlist->add(newMarker);
      }
    }
    else
    {
      // Grab a copy but with a new ID.
      new_markerlist->add(m.copy());
    }
  }

  PendingOperationItem poi(markerlist, new_markerlist, PendingOperationItem::ModifyMarkerList);
  ops.add(poi);

  return true;
}


} // namespace MusECore
