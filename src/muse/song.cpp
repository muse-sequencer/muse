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
//#include <iostream>

#include <QDir>
#include <QMessageBox>
#include <QPoint>
#include <QString>
#include <QTextStream>
#include <QProcess>
#include <QByteArray>
#include <QProgressDialog>
//#include <QList>

#include "app.h"
#include "driver/jackmidi.h"
#include "driver/alsamidi.h"
#include "song.h"
//#include "key.h"
#include "globals.h"
#include "drummap.h"
#include "amixer.h"
#include "midiseq.h"
#include "gconfig.h"
#include "sync.h"
#include "midi_consts.h"
#include "midictrl.h"
#include "menutitleitem.h"
#include "midi_audio_control.h"
#include "tracks_duplicate.h"
//#include "midi_consts.h"
#include "keyevent.h"
#ifndef _WIN32
//#include <sys/wait.h>
#endif
//#include "strntcpy.h"
#include "name_factory.h"
#include "synthdialog.h"

// Forwards from header:
#include <QAction>
#include <QMenu>
#include "undo.h"
#include "track.h"
#include "event.h"
//#include "xml.h"
//#include "track.h"
#include "part.h"
#include "marker/marker.h"
#include "route.h"
#include "audio.h"
#include "midiport.h"
#include "audiodev.h"
#include "synthdialog.h"

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

      _audioCtrlMoveModeBegun = false;

      _ipcInEventBuffers = new LockFreeMPSCRingBuffer<MidiPlayEvent>(16384);
      _ipcOutEventBuffers = new LockFreeMPSCRingBuffer<MidiPlayEvent>(16384);

      _ipcCtrlGUIMessages = new LockFreeMPSCRingBuffer<CtrlGUIMessage>(4096);
  
      _fCpuLoad = 0.0;
      _fDspLoad = 0.0;
      _xRunsCount = 0;

      realtimeMidiEvents = new LockFreeMPSCRingBuffer<MidiRecordEvent>(256);
      mmcEvents = new LockFreeMPSCRingBuffer<MMC_Commands>(256);

      undoList     = new UndoList(true);  // "true" means "this is an undoList",
      redoList     = new UndoList(false); // "false" means "redoList"
      _markerList  = new MarkerList;
      _globalPitchShift = 0;
      bounceTrack = nullptr;
      bounceOutput = nullptr;
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
    if(_ipcCtrlGUIMessages)
      delete _ipcCtrlGUIMessages;

    delete realtimeMidiEvents;
    delete mmcEvents;
}

//---------------------------------------------------------
//   putEvent
//---------------------------------------------------------

void Song::putEvent(MidiRecordEvent &inputMidiEvent)
{
    if(!realtimeMidiEvents->put(inputMidiEvent))
    {
      fprintf(stderr, "Song::putEvent - OVERFLOW - Dropping input midi events sent to GUI!\n");
    }
}

void Song::putMMC_Command(MMC_Commands command)
{
    if(!mmcEvents->put(command))
    {
      fprintf(stderr, "Song::putMMC_Command - OVERFLOW - Dropping input MMC commands sent to GUI!\n");
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
        return nullptr;
    

    // Synth sub-menu id?
    if(n >= MENU_ADD_SYNTH_ID_BASE || n == MusECore::Track::AUDIO_SOFTSYNTH)
    {
        if (n == MusECore::Track::AUDIO_SOFTSYNTH) {
            n = MusEGui::SynthDialog().getSynthIndex(nullptr);
            if (n < 0 || n >= static_cast<int>(MusEGlobal::synthis.size()))
                return nullptr;
        }
        else
        {
            n -= MENU_ADD_SYNTH_ID_BASE;
// not necessary - the old synth menus have been removed (kybos)
//            int ntype = n / MENU_ADD_SYNTH_ID_BASE;
//            if(ntype >= Synth::SYNTH_TYPE_END)
//                return nullptr;

//            // if we ever support Wine VSTs through some other means than through dssi-vst this must be adapted
//            if (ntype == MusECore::Synth::VST_SYNTH)
//                ntype=MusECore::Synth::DSSI_SYNTH;
//            if (ntype == MusECore::Synth::LV2_EFFECT)
//                ntype=MusECore::Synth::LV2_SYNTH; // the LV2_EFFECT is a specialization used in the menu only, we reassign it to regular LV2_SYNTH

//            n %= MENU_ADD_SYNTH_ID_BASE;
            if(n >= (int)MusEGlobal::synthis.size())
                return nullptr;

            if (MusEGlobal::debugMsg)
                fprintf(stderr, "Song::addNewTrack synth: idx:%d class:%s label:%s\n", n, MusEGlobal::synthis[n]->baseName().toLatin1().constData(), MusEGlobal::synthis[n]->name().toLatin1().constData());
//            fprintf(stderr, "Song::addNewTrack synth: type:%d idx:%d class:%s label:%s\n", ntype, n, MusEGlobal::synthis[n]->baseName().toLatin1().constData(), MusEGlobal::synthis[n]->name().toLatin1().constData());
        }

        SynthI* si = createSynthI(MusEGlobal::synthis[n]->baseName(), MusEGlobal::synthis[n]->uri(),
                                  MusEGlobal::synthis[n]->name(), MusEGlobal::synthis[n]->synthType(), insertAt);
        //      SynthI* si = createSynthI(MusEGlobal::synthis[n]->baseName(), MusEGlobal::synthis[n]->uri(),
        //                                MusEGlobal::synthis[n]->name(), (Synth::Type)ntype, insertAt);
        if(!si)
            return nullptr;

        if (MusEGlobal::config.unhideTracks) SynthI::setVisible(true);

        MusEGui::SynthDialog::addRecent(MusEGlobal::synthis[n]);

        // Add instance last in midi device list.
        for (int i = 0; i < MusECore::MIDI_PORTS; ++i)
        {
            MidiPort* port  = &MusEGlobal::midiPorts[i];
            MidiDevice* dev = port->device();
            if (dev==nullptr)
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
            return nullptr;

        Track* t = addTrack((Track::TrackType)n, insertAt);
        if (t && t->isVisible()) {
            selectAllTracks(false);
            t->setSelected(true);
            update(SC_TRACK_SELECTION);
        }
        return t;
    }
}          

//---------------------------------------------------------
//    createTrack
//---------------------------------------------------------

Track* Song::createTrack(Track::TrackType type, bool setDefaults)
      {
      Track* track = nullptr;
      switch(type) {
            case Track::MIDI:
                  track = new MidiTrack();
                  track->setType(Track::MIDI);
                  break;
            case Track::DRUM:
                  track = new MidiTrack();
                  track->setType(Track::DRUM);
                  ((MidiTrack*)track)->setOutChannel(9);
                  break;
            case Track::WAVE:
                  track = new MusECore::WaveTrack();
                  break;
            case Track::AUDIO_OUTPUT:
                  track = new AudioOutput();
                  break;
            case Track::AUDIO_GROUP:
                  track = new AudioGroup();
                  break;
            case Track::AUDIO_AUX:
                  track = new AudioAux();
                  break;
            case Track::AUDIO_INPUT:
                  track = new AudioInput();
                  break;
            case Track::AUDIO_SOFTSYNTH:
                  fprintf(stderr, "not implemented: Song::createTrack(SOFTSYNTH)\n");
                  return nullptr;
            default:
                  fprintf(stderr, "THIS SHOULD NEVER HAPPEN: Song::createTrack() illegal type %d. returning NULL.\n"
                         "save your work if you can and expect soon crashes!\n", type);
                  return nullptr;
            }

      if(setDefaults)
      {
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
                      if(type != Track::DRUM)  // Leave drum tracks at channel 10.
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

              if (mp->device() != nullptr) {

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
      }
            
      return track;
      }

//---------------------------------------------------------
//    addTrack
//    called from GUI context
//    type is track type
//    If insertAt is valid, inserts before insertAt. Else at the end after all tracks.
//---------------------------------------------------------

Track* Song::addTrack(Track::TrackType type, Track* insertAt)
      {
      // Try to generate a unique track name.
      TrackNameFactory names(type);
      if(names.isEmpty())
        return nullptr;

      Track* track = createTrack(type, true);
      if(!track)
        return nullptr;

      switch(type) {
            case Track::MIDI:
                  if (MusEGlobal::config.unhideTracks) MidiTrack::setVisible(true);
                  break;
            case Track::DRUM:
                  if (MusEGlobal::config.unhideTracks) MidiTrack::setVisible(true);
                  break;
            case Track::WAVE:
                  if (MusEGlobal::config.unhideTracks) WaveTrack::setVisible(true);
                  break;
            case Track::AUDIO_OUTPUT:
                  if (MusEGlobal::config.unhideTracks) AudioOutput::setVisible(true);
                  break;
            case Track::AUDIO_GROUP:
                  if (MusEGlobal::config.unhideTracks) AudioGroup::setVisible(true);
                  break;
            case Track::AUDIO_AUX:
                  if (MusEGlobal::config.unhideTracks) AudioAux::setVisible(true);
                  break;
            case Track::AUDIO_INPUT:
                  if (MusEGlobal::config.unhideTracks) AudioInput::setVisible(true);
                  break;
            case Track::AUDIO_SOFTSYNTH:
                  fprintf(stderr, "not implemented: Song::addTrack(SOFTSYNTH)\n");
                  return nullptr;
            default:
                  fprintf(stderr, "THIS SHOULD NEVER HAPPEN: Song::addTrack() illegal type %d. returning NULL.\n"
                         "save your work if you can and expect soon crashes!\n", type);
                  return nullptr;
            }

      track->setName(names.first());

      int idx = insertAt ? _tracks.index(insertAt) : -1;
      applyOperation(UndoOp(UndoOp::AddTrack, idx, track));
            
      return track;
      }

//---------------------------------------------------------
//    duplicateTracks
//    Called from GUI context
//---------------------------------------------------------

void Song::duplicateTracks(Track *t)
{
    const TrackList& tl = _tracks;

    int audio_found = 0;
    int midi_found = 0;
    int new_drum_found = 0;

    if (t) {
        if (t->type() == Track::DRUM)
            ++new_drum_found;
        else if (t->type() == Track::MIDI)
            ++midi_found;
        else
            ++audio_found;
    } else {
        for (ciTrack it = tl.cbegin(); it != tl.cend(); ++it)
            if ((*it)->selected())
            {
                Track::TrackType type = (*it)->type();
                if (type == Track::DRUM)
                    ++new_drum_found;
                else if (type == Track::MIDI)
                    ++midi_found;
                else
                    ++audio_found;
            }
    }

    if(audio_found == 0 && midi_found == 0 && new_drum_found==0)
        return;

    MusEGui::DuplicateTracksDialog* dlg = new MusEGui::DuplicateTracksDialog(audio_found, midi_found, new_drum_found);

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

    int idx;
    int trackno = tl.size();
    TrackNameFactory names;
    Undo operations;

    if (t) {
        if (names.genUniqueNames(t->type(), t->name(), copies))
        {
            for (int cp = 0; cp < copies; ++cp)
            {
                Track* new_track = t->clone(flags);
                if (!new_track)
                    break;
                new_track->setName(names.at(cp));
                idx = trackno + cp;
                operations.push_back(MusECore::UndoOp(MusECore::UndoOp::AddTrack, idx, new_track));
            }
        }
        t->setSelected(false);

    } else {
        for(TrackList::const_reverse_iterator it = tl.crbegin(); it != tl.crend(); ++it)
        {
            Track* track = *it;
            if(track->selected())
            {
                if(names.genUniqueNames(track->type(), track->name(), copies))
                {
                    for(int cp = 0; cp < copies; ++cp)
                    {
                        Track* new_track = track->clone(flags);
                        if(!new_track)
                            break;
                        new_track->setName(names.at(cp));
                        idx = trackno + cp;
                        operations.push_back(MusECore::UndoOp(MusECore::UndoOp::AddTrack, idx, new_track));
                    }
                }
                track->setSelected(false);
            }
            --trackno;
        }
    }

    applyOperationGroup(operations);
}          

bool Song::addEventOperation(const Event& event, Part* part, bool do_port_ctrls, bool do_clone_port_ctrls)
{
  Event ev(event);
  bool added = false;
  Part* p = part;
  while(true)
  {
    // This will find the event even if it has been modified. As long as the IDs AND the position are the same, it's a match.
    // NOTE: Multiple events with the same event base pointer or the same id number, in one event list, are FORBIDDEN.
    //       This precludes using them for 'pattern groups' such as arpeggios or chords. Instead, create a new event type.
    ciEvent ie = p->events().findWithId(ev);
    if(ie == p->events().cend())
    {
      if(pendingOperations.add(PendingOperationItem(p, ev, PendingOperationItem::AddEvent)))
      {
        added = true;
        // Include addition of any corresponding cached controller value.
        // By default, here we MUST include all clones so that in the case of multiple events
        //  at the same position the cache reader can quickly look at each part and if one
        //  is MUTED pick an event from a different unmuted part at that position.
        if(do_port_ctrls && (do_clone_port_ctrls || (!do_clone_port_ctrls && p == part)))
          pendingOperations.addPartPortCtrlEvents(ev, p, p->tick(), p->lenTick(), p->track());
      }
    }
    
    p = p->nextClone();
    if(p == part)
      break;
    
    ev = event.clone(); // Makes a new copy with the same id.
  }
  return added;
}

Event Song::changeEventOperation(const Event& oldEvent, const Event& newEvent,
                                Part* part, bool do_port_ctrls, bool do_clone_port_ctrls)
{
  Event ev(newEvent);
  Event p_res, res;
  // If position is changed we need to reinsert into the list, and all clone lists.
  Part* p = part;
  while(true)
  {
    // This will find the event even if it has been modified.
    // As long as the IDs AND the position are the same, it's a match.
    iEvent ie = p->nonconst_events().findWithId(oldEvent);
    if(ie == p->nonconst_events().end())
    {
      // The old event was not found. Just go ahead and include the addition of the new event.
      // Make sure the new event doesn't already exist.
      if(p->events().findWithId(ev) == p->events().cend())
      {
        if(pendingOperations.add(PendingOperationItem(p, ev, PendingOperationItem::AddEvent)))
        {
          if(do_port_ctrls && (do_clone_port_ctrls || (!do_clone_port_ctrls && p == part)))
            pendingOperations.addPartPortCtrlEvents(ev, p, p->tick(), p->lenTick(), p->track());  // Port controller values.
        }
      }
    }
    else
    {
      // Use the actual old found event, not the given oldEvent.
      const Event& e = ie->second;
      // Prefer to return the event found in the given part's event list, not a clone part's.
      if(p == part)
        p_res = e;
      if(res.empty())
        res = e;

      // Go ahead and include deletion of the old event.
      if(pendingOperations.add(PendingOperationItem(p, ie, PendingOperationItem::DeleteEvent)))
      {
        // If the new and old event IDs are the same we bypass looking for the new event
        //  because it hasn't been deleted yet and would always be found.
        // This is safe since the event is deleted then added again.
        // But if the new and old event IDs are not the same we MUST make sure the
        //  new event does not already exist.
        if((ev.id() == oldEvent.id() || p->events().findWithId(ev) == p->events().cend()) &&
           pendingOperations.add(PendingOperationItem(p, ev, PendingOperationItem::AddEvent)))
        {
          if(do_port_ctrls && (do_clone_port_ctrls || (!do_clone_port_ctrls && p == part)))
            pendingOperations.modifyPartPortCtrlEvents(e, ev, p);  // Port controller values.
        }
        else
        {
          // Adding the new event failed.
          // Just go ahead and include removal of the old cached value.
          if(do_port_ctrls && (do_clone_port_ctrls || (!do_clone_port_ctrls && p == part)))
            pendingOperations.removePartPortCtrlEvents(e, p, p->track());  // Port controller values.
        }
      }
    }
    
    p = p->nextClone();
    if(p == part)
      break;

    ev = newEvent.clone(); // Makes a new copy with the same id.
  }
  
  // Prefer to return the event found in the given part's event list, not a clone part's.
  if(!p_res.empty())
    return p_res;
  
  return res;
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
         pendingOperations.removePartPortCtrlEvents(e, p, p->track());  // Port controller values.
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
            int ch = mt->drummap()[note].channel;
            if(ch == -1)
              ch = mt->outChannel();
            int port = mt->drummap()[note].port;
            if(port == -1)
              port = mt->outPort();
            MidiPort* mp = &MusEGlobal::midiPorts[port];
            cntrl = (cntrl & ~0xff) | mt->drummap()[note].anote;
            // Remove the port controller value.
            mp->deleteController(ch, tick, cntrl, val, part);

            
            // FIXME FIXME CHECK THIS
            //
            //  Why wasn't 'ch' given its own 'ch_add' variable?
            //  Why wasn't 'mp' given its own 'mp_add' variable?
            //  That means the channel and port will default to the ones
            //   being erased above, not the track's. That can't be right !
            //  Checked callers: Looks OK for this routine only.
            //  If newnote, newchan, or newport are -1 it means
            //   "don't touch, use the original". IOW it means we only
            //   want to change what is not -1.
            
            
            if(newnote != -1 && newnote != mt->drummap()[note].anote)
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
//   changeMidiCtrlCacheEvents
//---------------------------------------------------------

void Song::changeMidiCtrlCacheEvents(
  bool add, bool drum_tracks, bool midi_tracks, bool drum_ctls, bool non_drum_ctls)
{
  if(!drum_tracks && !midi_tracks)
    return;

  for(ciMidiTrack it = _midis.begin(); it != _midis.end(); ++it) 
  {
    MidiTrack* mt = *it;
    if((mt->type() == Track::DRUM && drum_tracks) || ((mt->type() == Track::MIDI && midi_tracks)))
    {
      if(add)
        addPortCtrlEvents(mt, drum_ctls, non_drum_ctls);
      else
        removePortCtrlEvents(mt, drum_ctls, non_drum_ctls);
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
            startTick = MusEGlobal::sigmap.raster1(startTick, MusEGlobal::muse->arrangerRaster());
            // Round the end up using the Arranger part snap raster value. 
            endTick   = MusEGlobal::sigmap.raster2(endTick, MusEGlobal::muse->arrangerRaster());
            
            newpart->setTick(startTick);
            newpart->setLenTick(endTick - startTick);
            newpart->setName(mt->name());
            newpart->setColorIndex(MusEGlobal::muse->currentPartColorIndex());

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
            endTick   = MusEGlobal::sigmap.raster2(endTick, MusEGlobal::muse->arrangerRaster());
            
            operations.push_back(UndoOp(UndoOp::ModifyPartLength, part, part->lenValue(), endTick, 0, Pos::TICKS));
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
      part->setColorIndex(MusEGlobal::muse->currentPartColorIndex());

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
        if (!MusEGlobal::muse->saveAs()) {
            MusEGlobal::recordAction->setChecked(false);
            return; // could not store project, won't enable record
        }
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
                if (!alreadyRecEnabled && !selectedTracks.empty()) {
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
                    // if there no tracks or no track is selected, warn the user and don't enable record
                    if (selectedTracks.empty()) {
                        QMessageBox::warning(nullptr, "MusE", tr("Record: At least one track must be armed for recording first."));
                        f = false;
                    }
//                      // if there are no tracks, do not enable record
//                      if (waves()->empty() && midis()->empty()) {
//                            fprintf(stderr, "No track to select, won't enable record\n");
//                            f = false;
//                            }
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
      _fastMove = NORMAL_MOVEMENT; // reset fast move in stop cases

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

void Song::forwardStep()
      {
      unsigned newPos = pos[0].tick() + MusEGlobal::config.division;
      MusEGlobal::audio->msgSeek(Pos(newPos, true));
      }

//---------------------------------------------------------
//   rewind
//---------------------------------------------------------

void Song::rewindStep()
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
      _songLenTicks = MusEGlobal::sigmap.bar2tick(40, 0, 0);    // default song len
      for (iTrack t = _tracks.begin(); t != _tracks.end(); ++t) {
            Track* track = dynamic_cast<Track*>(*t);
            if (track == 0)
                  continue;
            PartList* parts = track->parts();
            for (iPart p = parts->begin(); p != parts->end(); ++p) {
                  unsigned last = p->second->tick() + p->second->lenTick();
                  if (last > _songLenTicks)
                        _songLenTicks = last;
                  }
            }
      _songLenTicks = roundUpBar(_songLenTicks);
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

      // Process any messages from audio controller changes.
      processIpcCtrlGUIMessages();

      // Update all track plugin/synth guis etc. at the heartbeat rate.
      for(ciTrack it = _tracks.begin(); it != _tracks.end(); ++it)
        (*it)->guiHeartBeat();

      int eventsToProcess = realtimeMidiEvents->getSize();
      while (eventsToProcess--)
      {
          MidiRecordEvent currentEvent;
          if (!realtimeMidiEvents->get(currentEvent))
          {
              fprintf(stderr, "Song::beat - Missing realtimeMidiEvent!\n");
              continue;
          }

          int dataA = currentEvent.dataA();
          int dataB = currentEvent.dataB();

          if (currentEvent.type() == ME_NOTEON || currentEvent.type() == ME_NOTEOFF)
          {
              //---------------------------------------------------
              // filter midi remote control events
              //---------------------------------------------------
              bool consumed = false;
              if (MusEGlobal::rcEnable) {
                  if (dataA == MusEGlobal::rcStopNote) {
                      setStop(true);
                      consumed = true;
                  } else if (dataA == MusEGlobal::rcRecordNote) {
                      if (currentEvent.type() == ME_NOTEOFF)
                        setRecord(false);
                      else
                        setRecord(true);
                      consumed = true;
                  } else if (dataA == MusEGlobal::rcGotoLeftMarkNote) {
                      setPos(CPOS, pos[LPOS].tick(), true, true, true);
                      consumed = true;
                  } else if (dataA == MusEGlobal::rcPlayNote) {
                      setPlay(true);
                      consumed = true;
                  } else if (dataA == MusEGlobal::rcForwardNote) {
                      _fastMove = FAST_FORWARD;
                      consumed = true;
                  } else if (dataA == MusEGlobal::rcBackwardNote) {
                      _fastMove = FAST_REWIND;
                      consumed = true;
                  }
              }

              if (!consumed)
                  emit MusEGlobal::song->midiNote(dataA, dataB);
          } // NOTES

          if (MusEGlobal::rcEnableCC && currentEvent.type() == ME_CONTROLLER)
          {
              if (dataA == MusEGlobal::rcStopCC)
                  setStop(true);
              else if (dataA == MusEGlobal::rcPlayCC)
                  setPlay(true);
              else if (dataA == MusEGlobal::rcRecordCC)
              {
                if (dataB == 0)
                  setRecord(false);
                else
                  setRecord(true);
              }
              else if (dataA == MusEGlobal::rcGotoLeftMarkCC)
                  setPos(CPOS, pos[LPOS].tick(), true, true, true);
              else if (dataA == MusEGlobal::rcForwardCC)
                  _fastMove = FAST_FORWARD;
              else if (dataA == MusEGlobal::rcBackwardCC)
                  _fastMove = FAST_REWIND;
          } // CC
      }

    int mmcToProcess = mmcEvents->getSize();
    while (mmcToProcess--)
    {
        MMC_Commands command;
        if (!mmcEvents->get(command))
        {
            fprintf(stderr, "Song::beat - Missing mmc command!\n");
            continue;
        }

        // Some syncronization commands have the complete implementation in sync.cc
        // some have the executive part here in the song class to be executed in the song thread.
        switch (command)
        {
          case MMC_FastForward:
            _fastMove = FAST_FORWARD;
            break;
          case MMC_Rewind:
            _fastMove = FAST_REWIND;
            break;
          case MMC_RecordStrobe:
            this->setRecord(true);
            break;
          case MMC_RecordExit:
            this->setRecord(false);
            break;
          case MMC_Reset:
            this->setRecord(false);
            this->rewindStart();
            _fastMove = NORMAL_MOVEMENT;
            break;
          default:
            fprintf(stderr, "Song::beat - This sync command not implemented here!\n");
            break;
        }
    }

    switch (_fastMove)
    {
        case FAST_FORWARD:
            this->forwardStep();
            break;
        case FAST_REWIND:
            this->rewindStep();
            break;
        default:
            break;
    }
}

//---------------------------------------------------------
//   setLen
//---------------------------------------------------------

void Song::setLen(unsigned l, bool do_update)
      {
      _songLenTicks = l;
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

void Song::setMarkerPos(const Marker& marker, const Pos& position)
      {
      // Here we use the separate SetMarkerPos operation, which is 'combo-breaker' aware, to optimize repeated adjustments.
      MusEGlobal::song->applyOperation(MusECore::UndoOp(MusECore::UndoOp::SetMarkerPos, marker, position.posValue(), position.type()));
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
    operations->push_back(UndoOp(
      UndoOp::SetTrackRecord, track, val, double(0), double(0), double(0), double(0)));
  }
  else
  {
    // The pending oplist system does not call setRecordFlag1 for us. Call it now.
    if(!track->setRecordFlag1(val))
      return;
    // This is a minor operation easily manually undoable. Let's not clog the undo list with it.
    MusECore::PendingOperationList oplist;
    oplist.add(MusECore::PendingOperationItem(track, val, MusECore::PendingOperationItem::SetTrackRecord));
    MusEGlobal::audio->msgExecutePendingOperations(oplist, true);
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

      // Clear any midi control assignments.
      _midiAssignments.clear();
      
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
              // Since Jack midi devices are created dynamically, we must delete them.
              // The destructor unregisters the device from Jack, which also disconnects all device-to-jack routes.
              // This will also delete all midi-track-to-device routes, they point to non-existant midi tracks 
              //  which were all deleted above
              delete (*imd);
              // Remove the device from the list.
              MusEGlobal::midiDevices.erase(imd);
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
      _songLenTicks  = MusEGlobal::sigmap.bar2tick(150, 0, 0);  // default song len in ticks set for 150 bars
      _follow        = JUMP;
      dirty          = false;
      initDrumMap();
      initNewDrumMap();
      if (signal) {
            emit loopChanged(false);
            emit recordChanged(false);
            emit songChanged(-1);  
            }
      }

//---------------------------------------------------------
//   cleanupForQuit
//   called from Muse::closeEvent
//---------------------------------------------------------

void Song::cleanupForQuit()
{
      bounceTrack = nullptr;

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
                        // Do NOT clear the record automation lists here in 'G'. It wipes out the initial
                        //  write values when play is pressed because play does a seek when lands here.
                        // Do it in 'N'.

                        setPos(CPOS, MusEGlobal::audio->tickPos(), true, false, true);
                        _startPlayPosition = MusEGlobal::audio->pos(); // update start position

                        if (_startPlayPosition.tick() == 0 || _startPlayPosition.tick() >= _songLenTicks )
                        {
                            // if we are moving out of bounds lets clear _fastMove
                            _fastMove = NORMAL_MOVEMENT;
                        }
                        break;
                  case 'N':
                        // Re-enable all track and plugin controllers, and synth controllers if applicable,
                        //  and clear all track record event lists.
                        clearRecAutomation(true);
                        break;
                  case 'S':   // shutdown audio
                        MusEGlobal::muse->seqStop();

                        {
                        // give the user a sensible explanation
                        int btn = QMessageBox::critical( MusEGlobal::muse, tr("Jack shutdown!"),
                            tr("Jack has detected a performance problem which has led to\n"
                            "MusE being disconnected.\n"
                            "This could happen due to a number of reasons:\n"
                            "- a performance issue with your particular setup\n"
                            "- a bug in MusE (or possibly in another connected software)\n"
                            "- a random hiccup which might never occur again\n"
                            "- Jack was voluntarily stopped by you or someone else\n"
                            "- Jack crashed\n"
                            "If there is a persisting problem you are much welcome to discuss it\n"
                            "on the MusE forum\n"
                            "(there is information about the forum on the MusE\n"
                            " homepage which is available through the help menu).\n"
                            "\n"
                            "To proceed check the status of Jack and try to restart it and then\n"
                            "click on the Restart button."), "Restart", "Cancel");
                        if (btn == 0) {
                              fprintf(stderr, "Restarting!\n");
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
      const MidiPart* part = nullptr;
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
            // create new p
            MidiPart* p = new MidiPart(mt);
            int startTick = roundDownBar(tick);
            int endTick   = roundUpBar(tick + 1);
            p->setTick(startTick);
            p->setLenTick(endTick - startTick);
            p->setName(mt->name());
            event.move(-startTick);
            p->addEvent(event);
            MusEGlobal::song->applyOperation(UndoOp(UndoOp::AddPart, p));
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

int Song::execAutomationCtlPopup(Track* track, const QPoint& menupos, MidiAudioCtrlStruct::IdType idType, int id)
{
  enum { PREV_EVENT=0, NEXT_EVENT, ADD_EVENT, SET_EVENT, CLEAR_EVENT, CLEAR_RANGE, CLEAR_ALL_EVENTS, MIDI_ASSIGN, MIDI_CLEAR };
  QMenu* menu = new QMenu;

  bool haveValues = false;
  double eventVal = 0.0;
  bool isEvent = false, canSeekPrev = false, canSeekNext = false, canEraseRange = false;
  bool canAdd = false;
  double ctlval = 0.0;
  unsigned int frame = 0;

  const bool isBuiltInAudioCtrl = idType == MusECore::MidiAudioCtrlStruct::AudioControl && id < AC_PLUGIN_CTL_BASE;
  const bool canAssignToSong = idType == MusECore::MidiAudioCtrlStruct::NonAudioControl || isBuiltInAudioCtrl;

  AudioTrack* atrack = nullptr;
  if(track && !track->isMidiTrack() && idType == MidiAudioCtrlStruct::AudioControl)
  {
    atrack = static_cast<AudioTrack*>(track);
    ciCtrlList icl = atrack->controller()->find(id);
    if(icl != atrack->controller()->end())
    {
      CtrlList *cl = icl->second;
      canAdd = true;
      frame = MusEGlobal::audio->pos().frame();       
      bool en = atrack->controllerEnabled(id);
      AutomationType at = atrack->automationType();
      if(!MusEGlobal::automation || at == AUTO_OFF || !en)
        ctlval = cl->curVal();  
      else  
        ctlval = cl->value(frame);

      if(!cl->empty())
      {
        haveValues = true;

        iCtrl s = cl->lower_bound(frame);
        iCtrl e = cl->upper_bound(frame);

        if(s != cl->end() && s->first == frame)
        {
          isEvent = true;
          eventVal = s->second.value();
        }

        canSeekPrev = s != cl->begin();
        canSeekNext = e != cl->end();

        s = cl->lower_bound(pos[1].frame());

        canEraseRange = s != cl->end()
                        && pos[2].frame() > s->first;
      }
    }

    menu->addAction(new MusEGui::MenuTitleItem(tr("Automation"), menu));

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
    {
      addEvent->setText(tr("Set event"));
      addEvent->setData(SET_EVENT);
    }
    else
    {
      addEvent->setText(tr("Add event"));
      addEvent->setData(ADD_EVENT);
    }

    addEvent->setEnabled(canAdd);

    QAction* eraseEventAction = menu->addAction(tr("Erase event"));
    eraseEventAction->setData(CLEAR_EVENT);
    eraseEventAction->setEnabled(isEvent);

    QAction* eraseRangeAction = menu->addAction(tr("Erase range"));
    eraseRangeAction->setData(CLEAR_RANGE);
    eraseRangeAction->setEnabled(canEraseRange);

    QAction* clearAction = menu->addAction(tr("Clear automation"));
    clearAction->setData(CLEAR_ALL_EVENTS);
    clearAction->setEnabled(haveValues);

    menu->addSeparator();
  }

  menu->addAction(new MusEGui::MenuTitleItem(tr("Midi control"), menu));
  
  QAction *assign_act = menu->addAction(tr("Assign"));
  assign_act->setCheckable(false);
  assign_act->setData(MIDI_ASSIGN); 
  
  MidiAudioCtrlMap* macm = MusEGlobal::song->midiAssignments();
  AudioMidiCtrlStructMap amcs;

  // Include NULL tracks in search.
  macm->find_audio_ctrl_structs(idType, id, track, false, true, &amcs);

  if(!amcs.empty())
  {
    QAction *cact = menu->addAction(tr("Clear"));
    cact->setData(MIDI_CLEAR);
    menu->addSeparator();
  }

  for(iAudioMidiCtrlStructMap iamcs = amcs.begin(); iamcs != amcs.end(); ++iamcs)
  {
    const Track* t = (*iamcs)->second.track();
    int port, chan, mctrl;
    MidiAudioCtrlMap::hash_values((*iamcs)->first, &port, &chan, &mctrl);
    QString s = QString("Type:%1 Port:%2 Chan:%3 Ctl:%4").arg(t ? tr("Track") : tr("Song"))
                                                  .arg(port + 1)
                                                  .arg(chan + 1)
                                                  //.arg((mctrl >> 8) & 0xff)
                                                  //.arg(mctrl & 0xff);
                                                  .arg(midiCtrlName(mctrl, true));
    QAction *mact = menu->addAction(s);
    mact->setEnabled(false);
    mact->setData(-1); // Not used
  }
  
  QAction* act = menu->exec(menupos);
  if (!act)
  {
    delete menu;
    return -1;
  }
  
  int sel = act->data().toInt();
  delete menu;
  
  Undo operations;
  
  switch(sel)
  {
    case SET_EVENT:
          MusEGlobal::song->applyOperation(UndoOp(
            UndoOp::ModifyAudioCtrlVal, track, double(id), double(frame), double(frame), eventVal, ctlval));
    break;
    case ADD_EVENT:
          MusEGlobal::song->applyOperation(UndoOp(UndoOp::AddAudioCtrlVal,
            track, double(id), double(frame),
            // The undo system automatically sets the VAL_DISCRETE flag if the controller mode is DISCRETE.
            // Here is a tough decision regarding choice of discrete vs. interpolated:
            // Do we obey the discrete/interpolated toolbar button?
            // Given the (now) reduced role of interpolated graphs, maybe best to force these points to discrete. (Tim)
            ctlval, double(CtrlVal::VAL_SELECTED | CtrlVal::VAL_DISCRETE)));
    break;
    case CLEAR_EVENT:
          MusEGlobal::song->applyOperation(
            UndoOp(UndoOp::DeleteAudioCtrlVal, track, id, frame, double(0), double(0), double(0)));
    break;

    case CLEAR_RANGE:
          if(atrack)
            MusEGlobal::audio->msgEraseRangeACEvents(atrack, id, pos[1].frame(), pos[2].frame());
    break;

    case CLEAR_ALL_EVENTS:
          if(atrack)
          {
            if(QMessageBox::question(MusEGlobal::muse, QString("Muse"),
                tr("Clear all controller events?"), tr("&Ok"), tr("&Cancel"),
                QString(), 0, 1 ) == 0)
              MusEGlobal::audio->msgClearControllerEvents(atrack, id);
          }
    break;

    case PREV_EVENT:
          if(atrack)
            MusEGlobal::audio->msgSeekPrevACEvent(atrack, id);
    break;

    case NEXT_EVENT:
          if(atrack)
            MusEGlobal::audio->msgSeekNextACEvent(atrack, id);
    break;
    
    case MIDI_ASSIGN:
          {
            int port = -1, chan = 0, ctrl = 0;
            bool isSongAssign = (track == nullptr);

            for(MusECore::iAudioMidiCtrlStructMap iamcs = amcs.begin(); iamcs != amcs.end(); ++iamcs)
            {
              const Track* t = (*iamcs)->second.track();
              MidiAudioCtrlMap::hash_values((*iamcs)->first, &port, &chan, &ctrl);
              isSongAssign = t == nullptr;
              break; // Only a single item for now, thanks!
            }
            
            MusEGui::MidiAudioControl* pup = new MusEGui::MidiAudioControl(
              // Enable the assignment type widget.
              canAssignToSong,
              // Preset the assignment type widget to Track or Song.
              isSongAssign,
              port,
              chan,
              ctrl);

            if(pup->exec() == QDialog::Accepted)
            {
              port = pup->port(); chan = pup->chan(); ctrl = pup->ctrl();
              // Song assign for audio controllers is only allowed for built-ins like volume and pan.
              isSongAssign = pup->assignToSong() && canAssignToSong;

              if(port >= 0 && chan >=0 && ctrl >= 0)
              {
                MusEGlobal::audio->msgIdle(true);  // Gain access to structures, and sync with audio

                // Erase existing assignments to this control.
                // Include NULL tracks in search. If a song assignment has been requested,
                //  include ALL assignments to this control for ANY track.
                AudioMidiCtrlStructMap amcs_full;
                macm->find_audio_ctrl_structs(idType, id, track, isSongAssign, true, &amcs_full);
                for(MusECore::iAudioMidiCtrlStructMap iamcs = amcs_full.begin(); iamcs != amcs_full.end(); ++iamcs)
                  macm->erase(*iamcs);

                // Add will not replace if found.
                macm->add_ctrl_struct(port, chan, ctrl,
                  MusECore::MidiAudioCtrlStruct(idType, id, isSongAssign ? nullptr : track));

                MusEGlobal::audio->msgIdle(false);
              }
            }
            
            delete pup;
          }
          break;
    
    case MIDI_CLEAR:
          {
            if(!amcs.empty())
            {
              MusEGlobal::audio->msgIdle(true);  // Gain access to structures, and sync with audio
              // Erase assignments to this control for the track or for NULL tracks.
              for(MusECore::iAudioMidiCtrlStructMap iamcs = amcs.begin(); iamcs != amcs.end(); ++iamcs)
                macm->erase(*iamcs);
              MusEGlobal::audio->msgIdle(false);
            }
          }
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
  
  int dctl = ctlnum;
  
  // Is it a drum controller, according to the track port's instrument?
  int channel;
  MidiPort* mp;
  mt->mappedPortChanCtrl(&dctl, nullptr, &mp, &channel);

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
  MidiController* mc = mp->midiController(ctlnum, channel, false);
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

  menu->addAction(new MusEGui::MenuTitleItem(tr("Controller"), menu));
  QAction* bypassEvent = new QAction(menu);
  menu->addAction(bypassEvent);
  bypassEvent->setText(tr("Bypass"));
  bypassEvent->setData(BYPASS_CONTROLLER);
  bypassEvent->setEnabled(true);
  bypassEvent->setCheckable(true);
  bypassEvent->setChecked(cur_val == CTRL_VAL_UNKNOWN);

  menu->addAction(new MusEGui::MenuTitleItem(tr("Automation"), menu));
  
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
  const unsigned int sz = _ipcInEventBuffers->getSize();
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
  const int sz = _ipcOutEventBuffers->getSize();
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

bool Song::putIpcCtrlGUIMessage(const CtrlGUIMessage& msg)
{
  if(!_ipcCtrlGUIMessages->put(msg))
  {
    fprintf(stderr, "Error: Song::putIpcCtrlGUIMessage: Buffer overflow\n");
    return false;
  }
  return true;
}

//-------------------------------------------------------------
// The following are classes to help optimize audio automation
//  GUI messages arriving in Song::processIpcCtrlGUIMessages().
//-------------------------------------------------------------

struct CtrlGUIMessageItem
{
    double _value;
    CtrlGUIMessageItem();
    CtrlGUIMessageItem(double value);
};

typedef std::map<CtrlGUIMessage::Type, CtrlGUIMessageItem, std::less<CtrlGUIMessage::Type>> CtrlGUIMessageItemTypeMap;
typedef CtrlGUIMessageItemTypeMap::iterator iCtrlGUIMessageItemTypeMap;
typedef CtrlGUIMessageItemTypeMap::const_iterator ciCtrlGUIMessageItemTypeMap;
typedef std::pair<CtrlGUIMessage::Type, CtrlGUIMessageItem> CtrlGUIMessageItemTypeMapInsertPair;
typedef std::pair<iCtrlGUIMessageItemTypeMap, bool> CtrlGUIMessageItemTypeMapInsertResult;

typedef std::map<unsigned int /*frame*/, CtrlGUIMessageItemTypeMap, std::less<unsigned int>> CtrlGUIMessageMap;
typedef CtrlGUIMessageMap::iterator iCtrlGUIMessageMap;
typedef CtrlGUIMessageMap::const_iterator ciCtrlGUIMessageMap;
typedef std::pair<unsigned int /*frame*/, CtrlGUIMessageItemTypeMap> CtrlGUIMessageMapInsertPair;
typedef std::pair<iCtrlGUIMessageMap, bool> CtrlGUIMessageMapInsertResult;

class CtrlGUIMessageIdMap : public std::map<int /*id*/, CtrlGUIMessageMap, std::less<int >>
{
  public:
    // Returns true if insertion took place.
    // Returns false if assignment took place, or on error.
    bool add(int id, unsigned int frame, CtrlGUIMessage::Type, const CtrlGUIMessageItem&);
};
typedef CtrlGUIMessageIdMap::iterator iCtrlGUIMessageIdMap;
typedef CtrlGUIMessageIdMap::const_iterator ciCtrlGUIMessageIdMap;
typedef std::pair<int /*id*/, CtrlGUIMessageMap> CtrlGUIMessageIdMapInsertPair;
typedef std::pair<iCtrlGUIMessageIdMap, bool> CtrlGUIMessageIdMapInsertResult;

typedef std::set<CtrlGUIMessage::Type> CtrlGUIMessageTypeMap;
typedef CtrlGUIMessageTypeMap::iterator iCtrlGUIMessageTypeMap;
typedef CtrlGUIMessageTypeMap::const_iterator ciCtrlGUIMessageTypeMap;
typedef std::pair<iCtrlGUIMessageTypeMap, bool> CtrlGUIMessageTypeMapInsertResult;

struct CtrlGUIMessageTrackStruct
{
  // This holds parameterless track messages such as PAINT_UPDATE.
  CtrlGUIMessageTypeMap _typeMap;
  // This holds track messages having parameters such as ADDED, DELETED.
  CtrlGUIMessageIdMap _idMap;
};

class CtrlGUIMessageTrackMap : public std::map<const Track*, CtrlGUIMessageTrackStruct, std::less<const Track* >>
{
  public:
    // Returns true if insertion took place.
    // Returns false if assignment took place, or on error.
    bool add(const Track*, int id, unsigned int frame, CtrlGUIMessage::Type, const CtrlGUIMessageItem&);
};
typedef CtrlGUIMessageTrackMap::iterator iCtrlGUIMessageTrackMap;
typedef CtrlGUIMessageTrackMap::const_iterator ciCtrlGUIMessageTrackMap;
typedef std::pair<const Track*, CtrlGUIMessageTrackStruct> CtrlGUIMessageTrackMapInsertPair;
typedef std::pair<iCtrlGUIMessageTrackMap, bool> CtrlGUIMessageTrackMapInsertResult;

CtrlGUIMessageItem::CtrlGUIMessageItem()
  : _value(0.0)
{
}

CtrlGUIMessageItem::CtrlGUIMessageItem(double value)
  : _value(value)
{
}

bool CtrlGUIMessageIdMap::add(
  int id, unsigned int frame, CtrlGUIMessage::Type type, const CtrlGUIMessageItem& item)
{
  CtrlGUIMessageIdMapInsertResult res = insert(CtrlGUIMessageIdMapInsertPair(id, CtrlGUIMessageMap()));
  CtrlGUIMessageMap& aal = res.first->second;
  CtrlGUIMessageMapInsertResult res2 = aal.insert(CtrlGUIMessageMapInsertPair(frame, CtrlGUIMessageItemTypeMap()));
  CtrlGUIMessageItemTypeMap& itm = res2.first->second;
  CtrlGUIMessageItemTypeMapInsertResult res3 = itm.insert_or_assign(type, item);
  return res3.second;
}

bool CtrlGUIMessageTrackMap::add(
  const Track* track, int id, unsigned int frame, CtrlGUIMessage::Type type, const CtrlGUIMessageItem& item)
{
  CtrlGUIMessageTrackMapInsertResult res =
    insert(CtrlGUIMessageTrackMapInsertPair(track, CtrlGUIMessageTrackStruct()));
  CtrlGUIMessageTrackStruct& aam = res.first->second;
  switch(type)
  {
    case CtrlGUIMessage::PAINT_UPDATE:
      // Messages without parameters go here.
      return aam._typeMap.insert(type).second;
    break;

    case CtrlGUIMessage::ADDED:
    case CtrlGUIMessage::DELETED:
    case CtrlGUIMessage::NON_CTRL_CHANGED:
      // Messages with parameters go here.
      return aam._idMap.add(id, frame, type, item);
    break;
  }
  return false;
}

// Process any special IPC audio thread - to - gui thread messages.
// Called by gui thread only. Returns true on success.
bool Song::processIpcCtrlGUIMessages()
{
  unsigned int sz = _ipcCtrlGUIMessages->getSize();
  if(sz > 0)
  {
    CtrlGUIMessageTrackMap tm;
    while (sz--)
    {
      CtrlGUIMessage msg;
      if (!_ipcCtrlGUIMessages->get(msg))
      {
        fprintf(stderr, "Song::processIpcAudioCtrlRT2GUIMessages - Error, nothing to read!\n");
        continue;
      }
      // Optimize by storing only the last changes that occurred.
      // Add will replace if found.
      tm.add(msg._track, msg._id, msg._frame, msg._type, CtrlGUIMessageItem(msg._value));
    }

    for(ciCtrlGUIMessageTrackMap itm = tm.cbegin(); itm != tm.cend(); ++itm)
    {
      const CtrlGUIMessageTrackStruct& ts = itm->second;

      // Emit the messages which have parameters.
      const CtrlGUIMessageIdMap& im = ts._idMap;
      for(ciCtrlGUIMessageIdMap iim = im.cbegin(); iim != im.cend(); ++iim)
      {
        const CtrlGUIMessageMap& mm = iim->second;
        for(ciCtrlGUIMessageMap imm = mm.cbegin(); imm != mm.cend(); ++imm)
        {
          const CtrlGUIMessageItemTypeMap& tpm = imm->second;
          for(ciCtrlGUIMessageItemTypeMap itpm = tpm.cbegin(); itpm != tpm.cend(); ++itpm)
          {
            switch(itpm->first)
            {
              case CtrlGUIMessage::PAINT_UPDATE:
              break;

              case CtrlGUIMessage::ADDED:
              case CtrlGUIMessage::DELETED:
                if(itm->first)
                  emit controllerChanged(itm->first, iim->first, imm->first, itpm->first);
              break;

              case CtrlGUIMessage::NON_CTRL_CHANGED:
                switch(iim->first)
                {
                  case NCTL_TRACK_MUTE:
                    emit songChanged(SongChangedStruct_t(SC_MUTE));
                  break;
                  case NCTL_TRACK_SOLO:
                    emit songChanged(SongChangedStruct_t(SC_SOLO));
                  break;

                  case NCTL_TRACKPROP_TRANSPOSE:
                  case NCTL_TRACKPROP_DELAY:
                  case NCTL_TRACKPROP_LENGTH:
                  case NCTL_TRACKPROP_VELOCITY:
                  case NCTL_TRACKPROP_COMPRESS:
                  break;
                }
              break;
            }
          }
        }
      }

      // Emit the messages which do not have parameters (such as paint update).
      if(itm->first)
      {
        const CtrlGUIMessageTypeMap& tpm = ts._typeMap;
        for(ciCtrlGUIMessageTypeMap itpm = tpm.cbegin(); itpm != tpm.cend(); ++itpm)
        {
          const CtrlGUIMessage::Type& tp = *itpm;
          emit controllerChanged(itm->first, 0, 0, tp);
        }
      }
    }
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
    if(at == AUTO_WRITE || at == AUTO_LATCH)  // Exclude write/latch mode because controls need to remain disabled if pressed before play.
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
  opsp->push_back(UndoOp(UndoOp::EnableAllAudioControllers, true));

  for(iTrack i = _tracks.begin(); i != _tracks.end(); ++i)
  {
    if(!(*i)->isMidiTrack())
      // Process (and clear) rec events.
      processTrackAutomationEvents((AudioTrack*)*i, opsp);
  }

  if(!operations)
    MusEGlobal::song->applyOperationGroup(ops);
}

//---------------------------------------------------------
//   collectAudioCtrlPasteModeOps
//---------------------------------------------------------

bool Song::collectAudioCtrlPasteModeOps(
  AudioAutomationItemTrackMap& trackMap, MusECore::Undo& operations,
  const MusECore::CtrlList::PasteEraseOptions& eraseOpts, bool recoverErasedItems, bool isCopy)
{
  bool ret = false;
  int num_non_copyable = 0;

  for(MusECore::iAudioAutomationItemTrackMap iatm = trackMap.begin();
      iatm != trackMap.end(); ++iatm)
  {
    const MusECore::Track* t = iatm->first;
    if(t->isMidiTrack())
      continue;
    const MusECore::AudioTrack* at = static_cast<const MusECore::AudioTrack*>(t);
    MusECore::AudioAutomationItemMap& atm = iatm->second;
    for(MusECore::iAudioAutomationItemMap iaim = atm.begin(); iaim != atm.end(); ++iaim)
    {
      const int ctrlId = iaim->first;
      const MusECore::ciCtrlList icl = at->controller()->find(ctrlId);
      if(icl != at->controller()->cend())
      {
        const MusECore::CtrlList* cl = icl->second;

        MusECore::CtrlList* trackErasedList = nullptr;
        {
          ciCtrlList icTrackErased = at->erasedController()->find(ctrlId);
          if(icTrackErased != at->erasedController()->cend())
            trackErasedList = icTrackErased->second;
        }
        MusECore::CtrlList* trackNoEraseList = nullptr;
        {
          ciCtrlList icTrackNoErase = at->noEraseController()->find(ctrlId);
          if(icTrackNoErase != at->noEraseController()->cend())
            trackNoEraseList = icTrackNoErase->second;
        }

        const CtrlList& clr = *cl;
        CtrlList* addCtrlList = new CtrlList(clr, CtrlList::ASSIGN_PROPERTIES);
        CtrlList* eraseCtrlList = new CtrlList(clr, CtrlList::ASSIGN_PROPERTIES);
        CtrlList* recoverableEraseCtrlList = new CtrlList(clr, CtrlList::ASSIGN_PROPERTIES);
        CtrlList* recoverableAddCtrlList = new CtrlList(clr, CtrlList::ASSIGN_PROPERTIES);
        CtrlList* noEraseCtrlList = new CtrlList(clr, CtrlList::ASSIGN_PROPERTIES);

        unsigned int groupStartFrame, groupEndFrame;
        unsigned int prevGroupEndFrame = 0;
        bool isFirstItem = true;
        bool isGroupStart = true;
        bool isGroupEnd = false;
        bool isLastItem = false;

        MusECore::AudioAutomationItemMapStruct& ais = iaim->second;
        MusECore::AudioAutomationItemList& ail = ais._selectedList;
        for(MusECore::iAudioAutomationItemList iail = ail.begin(); iail != ail.end(); ++iail)
        {
          // Check if this is the last item.
          MusECore::ciAudioAutomationItemList iail_next = iail;
          ++iail_next;
          if(iail_next == ail.cend())
            isLastItem = true;

          const unsigned int oldCtrlFrame = iail->first;
          MusECore::AudioAutomationItem& aai = iail->second;
          const unsigned int newCtrlFrame = aai._wrkFrame;

          MusECore::ciCtrl ic_existing_old = cl->find(oldCtrlFrame);

          // Even in PasteNoErase mode, we must still erase any existing
          //  items at the locations that we wish to paste to.
          if(eraseOpts == MusECore::CtrlList::PasteNoErase)
            isGroupEnd = true;
          // In PasteErase mode, check for group end.
          else if(eraseOpts == MusECore::CtrlList::PasteErase)
            isGroupEnd = aai._groupEnd;

          // Check for missing group end and force it if necessary.
          // Also, in PasteEraseRange mode this will catch the last
          //  item and force it to be group end.
          if(isLastItem)
            isGroupEnd = true;

          if(isGroupStart)
          {
            groupStartFrame = newCtrlFrame;
            isGroupStart = false;
          }

          // If nothing changed, don't bother with this point, just move on to the recoverable stuff below.
          if(oldCtrlFrame != newCtrlFrame || aai._wrkVal != aai._value)
          {
            // Was the current point found?
            if(ic_existing_old != cl->end())
            {
              // Is this a copy?
              if(isCopy)
              {
                // We cannot copy automation points to time positions where points already exist.
                // Only one point per time position is allowed.
                if(oldCtrlFrame == newCtrlFrame)
                {
                  // Reset the working value to the original value to keep the graphics drawing happy.
                  aai._wrkVal = aai._value;
                  // Increment the number of non-copyable points.
                  ++num_non_copyable;
                  continue;
                }

                // Remember the original points so that we can avoid erasing them later.
                // They are the points being copied. We do not want to erase them at all,
                //  we want to keep them alive no matter what gets dropped on top of them.
                // Also, we must unselect these original points. We don't want them included in any movement at all.
                // The ModifyAudioCtrlValList operation below AUTOMATICALLY unselects these points for us
                //  and reselects them on undo. There is no need to manually unselect them here.
                noEraseCtrlList->insert(CtrlListInsertPair_t(oldCtrlFrame, ic_existing_old->second));
              }
              else
              {
                // It's a move not a copy. So we want to erase this original point.
                eraseCtrlList->insert(CtrlListInsertPair_t(oldCtrlFrame, ic_existing_old->second));
              }
            }

            addCtrlList->insert(CtrlListInsertPair_t(
              newCtrlFrame, CtrlVal(aai._wrkVal, true, aai._discrete, aai._groupEnd)));
          }

          if(isGroupEnd)
          {
            groupEndFrame = newCtrlFrame;

            //-----------------------------------------------------
            // Find items to erase in the target controller list.
            //-----------------------------------------------------
            {
              MusECore::ciCtrl icEraseStart = cl->lower_bound(groupStartFrame);
              MusECore::ciCtrl icEraseEnd = cl->upper_bound(groupEndFrame);
              //eraseRecoveryList->insert(icEraseStart, icEraseEnd);
              for(MusECore::ciCtrl ice = icEraseStart; ice != icEraseEnd; ++ice)
              {
                // We do not want the current points to be recoverable,
                //  so do not erase them here, they are erased above.
                // Check if the point is selected.
                if(ail.find(ice->first) != ail.end())
                  continue;


                // If this is a copy, we do NOT want to erase the original items being copied.
                if(!isCopy || (noEraseCtrlList->find(ice->first) == noEraseCtrlList->cend() &&
                  (!trackNoEraseList || (trackNoEraseList->find(ice->first) == trackNoEraseList->cend()))))
                {
                  // Add the item to the list of recoverable items to be erased.
                  recoverableEraseCtrlList->insert(CtrlListInsertPair_t(ice->first, ice->second));
                }
              }
            }

            //-----------------------------------------------------
            // Find items to recover in the list of erased items.
            // Only items OUTSIDE the range of ALL groups' groupStartFrame to groupEndFrame.
            // In other words here in each iteration we recover from prevGroupEndFrame +1 to groupStartFrame -1.
            //-----------------------------------------------------
            if(recoverErasedItems && trackErasedList)
            {
              for(MusECore::iCtrl erase_ic = trackErasedList->begin(); erase_ic != trackErasedList->end(); ++erase_ic)
              {
                const unsigned int erase_frame = erase_ic->first;
                const MusECore::CtrlVal& erase_cv = erase_ic->second;
                // Is the item outside of the group range?
                // Restore it.
                if((((isFirstItem || erase_frame > prevGroupEndFrame) && erase_frame < groupStartFrame) ||
                    (isLastItem && erase_frame > groupEndFrame)))
                {
                  // The item is to be recovered. Do not select it.
                  recoverableAddCtrlList->insert(
                    CtrlListInsertPair_t(erase_frame, CtrlVal(
                      erase_cv.value(), /*erase_cv.selected()*/ false,
                      erase_cv.discrete(), /*group end*/ true)));
                }
              }
            }

            // Reset these for next iteration.
            isFirstItem = false;
            prevGroupEndFrame = groupEndFrame;
            isGroupStart = true;
            isGroupEnd = false;
          }
        }

        // If nothing was changed, delete and ignore.
        if(eraseCtrlList->empty())
        {
          delete eraseCtrlList;
          eraseCtrlList = nullptr;
        }
        if(addCtrlList->empty())
        {
          delete addCtrlList;
          addCtrlList = nullptr;
        }
        if(recoverableEraseCtrlList->empty())
        {
          delete recoverableEraseCtrlList;
          recoverableEraseCtrlList = nullptr;
        }
        if(recoverableAddCtrlList->empty())
        {
          delete recoverableAddCtrlList;
          recoverableAddCtrlList = nullptr;
        }
        if(noEraseCtrlList->empty())
        {
          delete noEraseCtrlList;
          noEraseCtrlList = nullptr;
        }
        if(eraseCtrlList || addCtrlList || recoverableEraseCtrlList || recoverableAddCtrlList || noEraseCtrlList)
        {
          operations.push_back(MusECore::UndoOp(MusECore::UndoOp::ModifyAudioCtrlValList,
            // Track.
            t,
            ctrlId,
            // List of items to erase.
            eraseCtrlList,
            // List of items to add.
            addCtrlList,
            // List of recoverable items to erase.
            recoverableEraseCtrlList,
            recoverableAddCtrlList,
            noEraseCtrlList,
            // DO NOT automatically end audio controller move mode if it is active.
            true
          ));

          ret = true;
        }
      }
    }
  }

  if(num_non_copyable > 0)
  {
    QMessageBox::critical(MusEGlobal::muse, QString("MusE"),
      tr("Copies of some automation points could not be made because points already exist at those time positions"));
  }

  return ret;
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
      _fastMove = NORMAL_MOVEMENT; // reset fast move in stop cases

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
                  _midis.push_back((MidiTrack*)track);
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
                  if (!s->sif() || !sy) {
                        // Persistent storage: If the synth is not found allow the track to load.
                        // It's OK if sy is NULL. initInstance needs to do a few things.
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
      for (const auto &it : _tracks) {
            if (it->isMidiTrack())
                  continue;
            MusECore::AudioTrack* wt = (MusECore::AudioTrack*)it;
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
                        track->updateAuxRoute( r->track->auxRefCount(), nullptr );
                      else if(r->track->type() == Track::AUDIO_AUX)
                        track->updateAuxRoute( 1, nullptr );
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
                        r->track->updateAuxRoute( track->auxRefCount(), nullptr );
                      else if(track->type() == Track::AUDIO_AUX)
                        r->track->updateAuxRoute( 1, nullptr );
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
                  ops.addDeviceOperation(&MusEGlobal::midiDevices, s);
                  ops.add(PendingOperationItem(&midiInstruments, s, PendingOperationItem::AddMidiInstrument));
                  sec_track_list = &_synthIs;
                  }
                  break;
            default:
                  fprintf(stderr, "unknown track type %d\n", track->type());
                  return;
            }

      ops.add(PendingOperationItem(&_tracks, track, idx, PendingOperationItem::AddTrack, sec_track_list));
      
      ops.addTrackPortCtrlEvents(track);
      
      // NOTE: Aux sends: 
      // Initializing of this track and/or others' aux sends is done at the end of Song::execute/revertOperationGroup2().
      // NOTE: Routes:
      // Routes are added in the PendingOperationItem::AddTrack section of PendingOperationItem::executeRTStage().
}

//void writeStringToFile(FILE *filePointer, const char *writeString)
//{
//  if (MusEGlobal::debugMsg)
//    std::cout << writeString;
//  fputs(writeString, filePointer);
//}

//---------------------------------------------------------
//   removeTrackOperation
//---------------------------------------------------------

void Song::removeTrackOperation(Track* track, PendingOperationList& ops)
{
      ops.removeTrackPortCtrlEvents(track);
      void* sec_track_list = 0;
      switch(track->type()) {
            case Track::MIDI:
            case Track::DRUM:
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
  TrackNameFactory new_track_names;
  
  int idx_cnt = 0;
  for(size_t i = 0; i < _tracks.size(); i++)
  {
      Track *cTrk = _tracks[i];
      if(!cTrk->recordFlag())
        continue;
      Track *nTrk = nullptr;
      if(!discard)
      {
        if(!new_track_names.genUniqueNames(cTrk->type(), cTrk->name(), 1))
          continue;

        nTrk = cTrk->clone(clone_flags);
        nTrk->setName(new_track_names.first());

        const int idx = _tracks.index(cTrk) + idx_cnt++;
        operations.push_back(MusECore::UndoOp(MusECore::UndoOp::AddTrack, idx + 1, nTrk));
        operations.push_back(UndoOp(
          UndoOp::SetTrackMute, cTrk, true, double(0), double(0), double(0), double(0)));
        operations.push_back(UndoOp(
          UndoOp::SetTrackRecord, cTrk, false, double(0), double(0), double(0), double(0)));
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

          //fprintf(stderr, "Song::setAudioConvertersOfflineOperation Setting sndfile:%s to isOffline:%d\n",
          //        sndfile.name().toLocal8Bit().constData(), isOffline);
          
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

//---------------------------------------------------------
//   processTrackAutomationEvents
//---------------------------------------------------------

void Song::processTrackAutomationEvents(AudioTrack *atrack, Undo* operations)
{
  const AutomationType atype = atrack->automationType();
  if(atype != AUTO_TOUCH && atype != AUTO_LATCH && atype != AUTO_WRITE)
    return;

  // Use either the supplied operations list or a local one.
  Undo ops;
  Undo& opsr = operations ? (*operations) : ops;

  CtrlListList *cll =  atrack->controller();
  CtrlRecList *crl = atrack->recEvents();
  for(ciCtrlList icl = cll->cbegin(); icl != cll->cend(); ++icl)
  {
    CtrlList* cl = icl->second;
    CtrlList& clr = *icl->second;
    int id = cl->id();

    // The Undo system will take 'ownership' of these and delete them at the appropriate time.
    CtrlList* erased_list_items = nullptr;
    CtrlList* added_list_items = nullptr;

    //------------------------------------------
    // Remove old events from record region...
    //------------------------------------------
    bool touched = false;
    unsigned int start;
    unsigned int ws;
    for(ciCtrlRec icr = crl->cbegin(); icr != crl->cend(); ++icr)
    {
      if(icr->id != id)
        continue;

      if(!touched)
      {
        start = icr->frame;
        ws = start;
        // If the item type is IGNORE, don't erase this controller item,
        //  it was already added and we don't want to erase it. Bump the start pos.
        if(icr->_flags & CtrlRecVal::ARVT_IGNORE)
          ++ws;
        touched = true;
      }

      if(atype == AUTO_TOUCH && icr->_flags & CtrlRecVal::ARVT_STOP)
      {
        if(icr->frame >= ws)
        {
          iCtrl s = cl->lower_bound(ws);
          // Insert range does not include the end item. Use upper bound.
          iCtrl e = cl->upper_bound(icr->frame);
          if(!erased_list_items)
            erased_list_items = new CtrlList(clr, CtrlList::ASSIGN_PROPERTIES);
          erased_list_items->insert(s, e);
        }
        // Reset for next iteration.
        touched = false;
      }
    }

    // Check if still touched ie. missing final stop flag in TOUCH mode, or we are in WRITE or LATCH mode.
    if(touched)
    {
      const unsigned int end = MusEGlobal::audio->getEndRecordPos().frame();
      // In WRITE mode we erase everything from the start to the end of recording, even if the
      //  control was not touched until later - but it must have been touched at least once.
      // Apparently the way other apps do it is to erase everything even if the control was
      //  not touched AT ALL. But that is incompatible with our app currently, because it
      //  will blindly perform the erasure on all controllers. We have no way to opt out of
      //  the erasure for specific controllers (no controller 'arm').
      if(atype == AUTO_WRITE)
      {
        unsigned int start = MusEGlobal::audio->getStartRecordPos().frame();
        for(ciCtrlRec icr = crl->cbegin(); icr != crl->cend(); ++icr)
        {
          if(icr->id != id)
            continue;
          // We are looking for an item at the start frame. If the frame is beyond, we're done.
          if(icr->frame > start)
            break;
          // If the first found item type is IGNORE, don't erase this controller item,
          //  it was already added and we don't want to erase it. Bump the start pos.
          if(icr->_flags & CtrlRecVal::ARVT_IGNORE)
            ++start;
          break;
        }
        if(end >= start)
        {
          iCtrl s = cl->lower_bound(start);
          // Insert range does not include the end item. Use upper bound.
          iCtrl e = cl->upper_bound(end);
          if(!erased_list_items)
            erased_list_items = new CtrlList(clr, CtrlList::ASSIGN_PROPERTIES);
          erased_list_items->insert(s, e);
        }
      }
      // In TOUCH and LATCH mode erase from the most recent start to the end of recording.
      else if(atype == AUTO_TOUCH || atype == AUTO_LATCH)
      {
        if(end >= ws)
        {
          iCtrl s = cl->lower_bound(ws);
          // Insert range does not include the end item. Use upper bound.
          iCtrl e = cl->upper_bound(end);
          if(!erased_list_items)
            erased_list_items = new CtrlList(clr, CtrlList::ASSIGN_PROPERTIES);
          erased_list_items->insert(s, e);
        }
      }
    }

    //------------------------------------------
    // Extract all recorded events for controller "id"
    //  from CtrlRecList and put into new_list.
    //------------------------------------------
    touched = false; // Reset.
    ciCtrl icPrevCtrl;
    bool prevCtrlValid = false;
    ciCtrlRec icrPrevRecCtrl;
    bool prevRecCtrlValid = false;
    for(ciCtrlRec icr = crl->cbegin(); icr !=crl->cend(); ++icr)
    {
      if(icr->id != id)
        continue;

      double val;
      // For a STOP item we ignore the given value at release time, and instead
      //  set a point where the existing graph is, so that the graph continues
      //  where it left off. This is in fact what we send to the controller's port
      //  when the control is released, ie. it picks up the stream where it left off.
      // Any existing point at the location should already be marked for erasure above.
      if(atype == AUTO_TOUCH && (icr->_flags & CtrlRecVal::ARVT_STOP))
      {
        val = clr.value(icr->frame);
        touched = false;
        // Reset this so the next recorded point re-determines the nearest previous controller value.
        prevCtrlValid = false;
      }
      else
      {
        val = icr->val;
        if(!touched)
        {
          touched = true;
          // Find the nearest previous controller value, but only if NOT in AUTO mode
          //  and the item is marked as IGNORE (ie. transport was in stop mode).
          if(!(atype == AUTO_TOUCH && icr->_flags & CtrlRecVal::ARVT_IGNORE))
          {
            if(atype == AUTO_WRITE)
              icPrevCtrl = cl->lower_bound(MusEGlobal::audio->getStartRecordPos().frame());
            else
              icPrevCtrl = cl->lower_bound(icr->frame);
            // Only if there is an item before the given frame.
            if(icPrevCtrl == cl->cbegin())
              prevCtrlValid = false;
            else
            {
              --icPrevCtrl;
              prevCtrlValid = true;
            }
          }
        }
      }

      // If the item type is IGNORE, don't add this controller item,
      //  it was already added and we don't want to add it.
      if(!(icr->_flags & CtrlRecVal::ARVT_IGNORE))
      {
            // Now optimize to avoid multiple vertices on flat straight lines in the graphs:
            // If the given value equals the nearest previous to-be-added or existing-and-not-to-be-erased
            //  item's value, don't bother adding the point, just continue.
            if(!MusEGlobal::config.audioAutomationOptimize || (!prevCtrlValid && !prevRecCtrlValid) ||
                (prevCtrlValid && prevRecCtrlValid &&
                (icPrevCtrl->second.value() > icrPrevRecCtrl->val ?
                  icPrevCtrl->second.value() : icrPrevRecCtrl->val) != val) ||
                (prevCtrlValid && icPrevCtrl->second.value() != val) ||
                (prevRecCtrlValid && icrPrevRecCtrl->val != val))
            {
              // Now add the value.
              // Force a discrete value, to be faithful to the incoming stream because
              //  if someone leaves a controller at a value for a while then moves
              //  the controller, this point and the next would be a long interpolation.
              if(!added_list_items)
                added_list_items = new CtrlList(clr, CtrlList::ASSIGN_PROPERTIES);
              added_list_items->add(icr->frame, val, /*selected*/ true, /* discrete */ true);
            }
      }

      icrPrevRecCtrl = icr;
      prevRecCtrlValid = true;
    }

    // Check if the control was still being touched in TOUCH mode (no STOP value was found before recording stopped).
    // Or, in WRITE or LATCH mode if anything was recorded.
    if(touched)
    {
      // To continue the graph where it left off before the first point, we need to add one more point
      //  at the recording end position. That is also what happens when we re-enable a graph stream when
      //  a control has been released, it instantly goes back to the graph stream.
      // We want the graph to match that. Without this the final recorded point would dominate until
      //  the next point.
      // Any pre-existing point at the end will already be erased by the code above.
      const unsigned int end = MusEGlobal::audio->getEndRecordPos().frame();
      const double val = clr.value(end);

      // Optimize to avoid multiple vertices on flat straight lines in the graphs.
      if(!MusEGlobal::config.audioAutomationOptimize || (!prevCtrlValid && !prevRecCtrlValid) ||
         (prevCtrlValid && prevRecCtrlValid &&
          (icPrevCtrl->second.value() > icrPrevRecCtrl->val ?
           icPrevCtrl->second.value() : icrPrevRecCtrl->val) != val) ||
         (prevCtrlValid && icPrevCtrl->second.value() != val) ||
         (prevRecCtrlValid && icrPrevRecCtrl->val != val))
      {
        if(!added_list_items)
          added_list_items = new CtrlList(clr, CtrlList::ASSIGN_PROPERTIES);
        added_list_items->add(end, val, /*selected*/ true, /* discrete */ true);
      }
    }

    if(added_list_items && added_list_items->empty())
    {
      delete added_list_items;
      added_list_items = nullptr;
    }
    if(erased_list_items && erased_list_items->empty())
    {
      delete erased_list_items;
      erased_list_items = nullptr;
    }
    if(added_list_items || erased_list_items)
      opsr.push_back(UndoOp(UndoOp::ModifyAudioCtrlValList, atrack, id, erased_list_items, added_list_items));
  }

  // Done with the recorded automation event list. Clear it.
  crl->clear();

  if(!operations)
    MusEGlobal::song->applyOperationGroup(ops);
}

void Song::setRecMode(int val) {
    _recMode = val;
    emit recModeChanged(val);
}

void Song::setCycleMode(int val) {
    _cycleMode = val;
    emit cycleModeChanged(val);
}

MidiAudioCtrlMap* Song::midiAssignments() { return &_midiAssignments; }

} // namespace MusECore
