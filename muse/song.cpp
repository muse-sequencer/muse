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

#include <QAction>
#include <QDir>
#include <QMenu>
#include <QMessageBox>
#include <QPoint>
#include <QSignalMapper>
#include <QString>
#include <QTextStream>
#include <QProcess>
#include <QByteArray>

#include "app.h"
#include "driver/jackmidi.h"
#include "driver/alsamidi.h"
#include "song.h"
#include "track.h"
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
#include "al/sig.h"
#include "keyevent.h"
#include <sys/wait.h>
#include "tempo.h"

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
      _arrangerRaster     = 0; // Set to measure, the same as Arranger intial value. Arranger snap combo will set this.
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

//---------------------------------------------------------
//   setTempo
//    public slot
//---------------------------------------------------------

void Song::setTempo(int newTempo)
      {
      MusEGlobal::audio->msgSetTempo(pos[0].tick(), newTempo, true);
      }

//---------------------------------------------------------
//   setSig
//    called from transport window
//---------------------------------------------------------

void Song::setSig(int z, int n)
      {
      if (_masterFlag) {
            MusEGlobal::audio->msgAddSig(pos[0].tick(), z, n);
            }
      }

void Song::setSig(const AL::TimeSignature& sig)
      {
      if (_masterFlag) {
            MusEGlobal::audio->msgAddSig(pos[0].tick(), sig.z, sig.n);
            }
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

      n %= MENU_ADD_SYNTH_ID_BASE;
      if(n >= (int)MusEGlobal::synthis.size())
        return 0;
        
      if (MusEGlobal::debugMsg)
        printf("Song::addNewTrack synth: type:%d idx:%d class:%s label:%s\n", ntype, n, MusEGlobal::synthis[n]->baseName().toLatin1().constData(), MusEGlobal::synthis[n]->name().toLatin1().constData());  
        
      SynthI* si = createSynthI(MusEGlobal::synthis[n]->baseName(), MusEGlobal::synthis[n]->name(), (Synth::Type)ntype, insertAt);
      if(!si)
        return 0;
      if (MusEGlobal::config.unhideTracks) SynthI::setVisible(true);

      // Add instance last in midi device list.
      for (int i = 0; i < MIDI_PORTS; ++i) 
      {
        MidiPort* port  = &MusEGlobal::midiPorts[i];
        MidiDevice* dev = port->device();
        if (dev==0) 
        {
          MusEGlobal::midiSeq->msgSetMidiDevice(port, si);
          MusEGlobal::muse->changeConfig(true);     // save configuration file
          if (SynthI::visible()) {
            deselectTracks();
            si->setSelected(true);
            update();
          }
          return si;
        }
      }
      if (SynthI::visible()) {
        deselectTracks();
        si->setSelected(true);
        update(SC_SELECTION);
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
      
      Undo operations;
      Track* t = addTrack(operations, (Track::TrackType)n, insertAt);
      applyOperationGroup(operations);
      if (t->isVisible()) {
        deselectTracks();
        t->setSelected(true);
        update(SC_SELECTION);
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

Track* Song::addTrack(Undo& /*operations*/, Track::TrackType type, Track* insertAt)
      {
      Track* track = 0;
      int lastAuxIdx = _auxs.size();
      switch(type) {
            case Track::MIDI:
                  track = new MidiTrack();
                  track->setType(Track::MIDI);
                  if (MusEGlobal::config.unhideTracks) MidiTrack::setVisible(true);
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
                  printf("not implemented: Song::addTrack(SOFTSYNTH)\n");
                  // ((AudioTrack*)track)->addAuxSend(lastAuxIdx);
                  break;
            default:
                  printf("THIS SHOULD NEVER HAPPEN: Song::addTrack() illegal type %d. returning NULL.\n"
                         "save your work if you can and expect soon crashes!\n", type);
                  return NULL;
            }
      track->setDefaultName();
      
      int idx = insertAt ? _tracks.index(insertAt) : -1;
      
      insertTrack1(track, idx);         // this and the below are replaced
      msgInsertTrack(track, idx, true); // by the UndoOp-operation
      insertTrack3(track, idx); // does nothing
      // No, can't do this. insertTrack2 needs to be called now, not later, otherwise it sees
      //  that the track may have routes, and reciprocates them, causing duplicate routes.
      ///operations.push_back(UndoOp(UndoOp::AddTrack, idx, track));  

      // Add default track <-> midiport routes. 
      if(track->isMidiTrack()) 
      {
        MidiTrack* mt = (MidiTrack*)track;
        int c, cbi, ch;
        bool defOutFound = false;                /// TODO: Remove this if and when multiple output routes supported.
        for(int i = 0; i < MIDI_PORTS; ++i)
        {
          MidiPort* mp = &MusEGlobal::midiPorts[i];
          
          if(mp->device())  // Only if device is valid. p4.0.17 
          {
            c = mp->defaultInChannels();
            if(c)
            {
              MusEGlobal::audio->msgAddRoute(Route(i, c), Route(track, c));
              updateFlags |= SC_ROUTE;
            }
          }  
          
          if(!defOutFound)                       ///
          {
            c = mp->defaultOutChannels();
            if(c)
            {
              
        /// TODO: Switch if and when multiple output routes supported.
        #if 0
              MusEGlobal::audio->msgAddRoute(Route(track, c), Route(i, c));
              updateFlags |= SC_ROUTE;
        #else 
              for(ch = 0; ch < MIDI_CHANNELS; ++ch)   
              {
                cbi = 1 << ch;
                if(c & cbi)
                {
                  defOutFound = true;
                  mt->setOutPort(i);
                  if(type != Track::DRUM)  // p4.0.17 Leave drum tracks at channel 10.
                    mt->setOutChannel(ch);
                  updateFlags |= SC_ROUTE;
                  break;               
                }
              }
        #endif
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
                        MusEGlobal::audio->msgAddRoute(Route((AudioTrack*)track, -1), Route(ao, -1));
                        updateFlags |= SC_ROUTE;
                        break;
                  // p3.3.38 It should actually never get here now, but just in case.
                  case Track::AUDIO_SOFTSYNTH:
                        MusEGlobal::audio->msgAddRoute(Route((AudioTrack*)track, 0, ((AudioTrack*)track)->channels()), Route(ao, 0, ((AudioTrack*)track)->channels()));
                        updateFlags |= SC_ROUTE;
                        break;
                  default:
                        break;
                  }
            }
      MusEGlobal::audio->msgUpdateSoloStates();
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
  for(iTrack it = tl.begin(); it != tl.end(); ++it) 
    if((*it)->selected()) 
    {
      Track::TrackType type = (*it)->type(); 
      // TODO: Handle synths.   p4.0.47
      if(type == Track::AUDIO_SOFTSYNTH)
        continue;
      
      if(type == Track::DRUM)
        ++drum_found;
      else
      if(type == Track::MIDI)
        ++midi_found;
      else
        ++audio_found;
    }
 
  if(audio_found == 0 && midi_found == 0 && drum_found == 0)
    return;
  
  
  MusEGui::DuplicateTracksDialog* dlg = new MusEGui::DuplicateTracksDialog(audio_found, midi_found, drum_found);
  
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
  if(dlg->copyParts())
    flags |= Track::ASSIGN_PARTS;     
  
  delete dlg;
  
  QString track_name;
  int idx;
  int trackno = tl.size();
  MusEGlobal::song->startUndo();
  for(TrackList::reverse_iterator it = tl.rbegin(); it != tl.rend(); ++it) 
  {
    Track* track = *it;
    if(track->selected()) 
    {
      track_name = track->name();
      
      for(int cp = 0; cp < copies; ++cp)
      {
        // There are two ways to copy a track now. Using the copy constructor or using new + assign(). 
        // Tested: Both ways seem OK. Prefer copy constructors for simplicity. But new + assign() may be 
        //  required for fine-grained control over initializing various track types.
        //
        
        //  Set to 0 to use the copy constructor. Set to 1 to use new + assign().
        // DELETETHIS is this still necessary to keep around?
        //            also consider removing and adding a hint to a revision number instead
  #if 0      
        
        Track* new_track = 0;
        int lastAuxIdx = _auxs.size();
        switch(track->type()) 
        {
          case Track::AUDIO_SOFTSYNTH:  // TODO: Handle synths.   p4.0.47
                // ((AudioTrack*)new_track)->addAuxSend(lastAuxIdx); DELETETHIS?
                break;
            
          case Track::MIDI:
                new_track = new MidiTrack();
                new_track->setType(Track::MIDI);
                break;
          case Track::DRUM:
                new_track = new MidiTrack();
                new_track->setType(Track::DRUM);
                //((MidiTrack*)new_track)->setOutChannel(9); DELETETHIS?
                break;
          case Track::WAVE:
                new_track = new MusECore::WaveTrack();
                //((AudioTrack*)new_track)->addAuxSend(lastAuxIdx); DELETETHIS?
                break;
          case Track::AUDIO_OUTPUT:
                new_track = new AudioOutput();
                break;
          case Track::AUDIO_GROUP:
                new_track = new AudioGroup();
                //((AudioTrack*)new_track)->addAuxSend(lastAuxIdx); DELETETHIS?
                break;
          case Track::AUDIO_AUX:
                new_track = new AudioAux();
                break;
          case Track::AUDIO_INPUT:
                new_track = new AudioInput();
                //((AudioTrack*)new_track)->addAuxSend(lastAuxIdx); DELETETHIS?
                break;
          default:
                printf("Song::duplicateTracks: Illegal type %d\n", track->type());
                break;
        }
            
        if(new_track)   
        {
          new_track->assign(*track, flags);
  #else      
        if(track->type() != Track::AUDIO_SOFTSYNTH)  // TODO: Handle synths.   p4.0.47
        {
          Track* new_track = track->clone(flags);  
  #endif
          
          //new_track->setDefaultName(track_name);  // Handled in class now.

          idx = trackno + cp;
          insertTrack1(new_track, idx);         
          addUndo(MusECore::UndoOp(MusECore::UndoOp::AddTrack, idx, new_track));
          msgInsertTrack(new_track, idx, false); // No undo.
          insertTrack3(new_track, idx); 
        }  
      }  
    }
    --trackno;
  }
  
  int update_flags = SC_TRACK_INSERTED;
  if(flags & (Track::ASSIGN_ROUTES | Track::ASSIGN_DEFAULT_ROUTES))
    update_flags |= SC_ROUTE;
  MusEGlobal::song->endUndo(update_flags);
  MusEGlobal::audio->msgUpdateSoloStates();
}          
      
//---------------------------------------------------------
//   cmdRemoveTrack
//---------------------------------------------------------

void Song::cmdRemoveTrack(Track* track)
      {
      int idx = _tracks.index(track);
      addUndo(UndoOp(UndoOp::DeleteTrack, idx, track));
      removeTrack2(track);
      updateFlags |= SC_TRACK_REMOVED;
      }

//---------------------------------------------------------
//   removeMarkedTracks
//---------------------------------------------------------

void Song::removeMarkedTracks()
      {
      bool loop;
      do {
            loop = false;
            for (iTrack t = _tracks.begin(); t != _tracks.end(); ++t) {
                  if ((*t)->selected()) {
                        removeTrack2(*t);
                        loop = true;
                        break;
                        }
                  }
            } while (loop);
      }

//---------------------------------------------------------
//   deselectTracks
//---------------------------------------------------------

void Song::deselectTracks()
      {
      for (iTrack t = _tracks.begin(); t != _tracks.end(); ++t)
            (*t)->setSelected(false);
      }

//---------------------------------------------------------
//  addEvent
//    return true if event was added
//---------------------------------------------------------

bool Song::addEvent(Event& event, Part* part)
      {
      // Return false if the event is already found. 
      // (But allow a port controller value, above, in case it is not already stored.)
      if(part->events()->find(event) != part->events()->end())
      {
        // This can be normal for some (redundant) operations.
        if(MusEGlobal::debugMsg)
          printf("Song::addEvent event already found in part:%s size:%zd\n", part->name().toLatin1().constData(), part->events()->size());
        return false;
      }
      
      part->events()->add(event);
      return true;
      }

//---------------------------------------------------------
//   changeEvent
//---------------------------------------------------------

void Song::changeEvent(Event& oldEvent, Event& newEvent, Part* part)
{
      iEvent i = part->events()->find(oldEvent);
      
      if (i == part->events()->end()) {
            // This can be normal for some (redundant) operations.
            if(MusEGlobal::debugMsg)
              printf("Song::changeEvent event not found in part:%s size:%zd\n", part->name().toLatin1().constData(), part->events()->size());
            // no "return;" because: Allow it to add the new event.  (And remove the old one from the midi port controller!) (tim)
            }
      else
        part->events()->erase(i);
        
      part->events()->add(newEvent);
}

//---------------------------------------------------------
//   deleteEvent
//---------------------------------------------------------

void Song::deleteEvent(Event& event, Part* part)
      {
      iEvent ev = part->events()->find(event);
      if (ev == part->events()->end()) {
            // This can be normal for some (redundant) operations.
            if(MusEGlobal::debugMsg)
              printf("Song::deleteEvent event not found in part:%s size:%zd\n", part->name().toLatin1().constData(), part->events()->size());
            return;
            }
      part->events()->erase(ev);
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
      const EventList* el = part->cevents();
      for(ciEvent ie = el->begin(); ie != el->end(); ++ie)
      {
        const Event& ev = ie->second;
        if(ev.type() != Controller)
          continue;
          
        int cntrl = ev.dataA();
        
        // Is it a drum controller event, according to the track port's instrument?
        MidiController* mc = trackmp->drumController(cntrl);
        if(!mc)
          continue;
          
        int note = cntrl & 0x7f;
        // Does the index match?
        if(note == mapidx)
        {
          int tick = ev.tick() + part->tick();
          int ch = MusEGlobal::drumMap[note].channel;
          int port = MusEGlobal::drumMap[note].port;
          MidiPort* mp = &MusEGlobal::midiPorts[port];
          cntrl = (cntrl & ~0xff) | MusEGlobal::drumMap[note].anote;
          
          // Remove the port controller value.
          mp->deleteController(ch, tick, cntrl, part);
          
          if(newnote != -1 && newnote != MusEGlobal::drumMap[note].anote)
            cntrl = (cntrl & ~0xff) | newnote;
          if(newchan != -1 && newchan != ch)
            ch = newchan;
          if(newport != -1 && newport != port)
            port = newport;
            
          mp = &MusEGlobal::midiPorts[port];
          
          // Add the port controller value.
          mp->setControllerVal(ch, tick, cntrl, ev.dataB(), part);
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
  int ch, trackch, cntrl, tick;
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
      const EventList* el = part->cevents();
      unsigned len = part->lenTick();
      for(ciEvent ie = el->begin(); ie != el->end(); ++ie)
      {
        const Event& ev = ie->second;
        // Added by T356. Do not handle events which are past the end of the part.
        if(ev.tick() >= len)
          break;
                    
        if(ev.type() != Controller)
          continue;
          
        cntrl = ev.dataA();
        mp = trackmp;
        ch = trackch;
        
        // Is it a drum controller event, according to the track port's instrument?
        if(trackmp->drumController(cntrl))
        {
          int note = cntrl & 0x7f;
          ch = MusEGlobal::drumMap[note].channel;
          mp = &MusEGlobal::midiPorts[MusEGlobal::drumMap[note].port];
          cntrl = (cntrl & ~0xff) | MusEGlobal::drumMap[note].anote;
        }
        else
        {  
          if(drumonly)
            continue;
        }
        
        tick = ev.tick() + part->tick();
        
        if(add)
          // Add the port controller value.
          mp->setControllerVal(ch, tick, cntrl, ev.dataB(), part);
        else  
          // Remove the port controller value.
          mp->deleteController(ch, tick, cntrl, part);
      }
    }  
  }
}

void Song::addACEvent(AudioTrack* t, int acid, int frame, double val)
{
  MusEGlobal::audio->msgAddACEvent(t, acid, frame, val);
}

void Song::changeACEvent(AudioTrack* t, int acid, int frame, int newFrame, double val)
{
  MusEGlobal::audio->msgChangeACEvent(t, acid, frame, newFrame, val);
}

//---------------------------------------------------------
//   cmdAddRecordedEvents
//    add recorded Events into part
//---------------------------------------------------------

void Song::cmdAddRecordedEvents(MidiTrack* mt, EventList* events, unsigned startTick)
      {
      if (events->empty()) {
            if (MusEGlobal::debugMsg)
                  printf("no events recorded\n");
            return;
            }
      iEvent s;
      iEvent e;
      unsigned endTick;

      if((MusEGlobal::audio->loopCount() > 0 && startTick > lPos().tick()) || (punchin() && startTick < lPos().tick()))
      {
            startTick = lpos();
            s = events->lower_bound(startTick);
      }
      else 
      {
            s = events->begin();
      }
      
      // search for last noteOff:
      endTick = 0;
      for (iEvent i = events->begin(); i != events->end(); ++i) {
            Event ev   = i->second;
            unsigned l = ev.endTick();
            if (l > endTick)
                  endTick = l;
            }

      if((MusEGlobal::audio->loopCount() > 0) || (punchout() && endTick > rPos().tick()) )
      {
            endTick = rpos();
            e = events->lower_bound(endTick);
      }
      else
            e = events->end();

      if (startTick > endTick) {
            if (MusEGlobal::debugMsg)
                  printf("no events in record area\n");
            return;
            }

      //---------------------------------------------------
      //    if startTick points into a part,
      //          record to that part
      //    else
      //          create new part
      //---------------------------------------------------

      PartList* pl = mt->parts();
      MidiPart* part = 0;
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
                  printf("create new part for recorded events\n");
            // create new part
            part      = new MidiPart(mt);
            
            // Round the start down using the Arranger part snap raster value. 
            startTick = AL::sigmap.raster1(startTick, arrangerRaster());
            // Round the end up using the Arranger part snap raster value. 
            endTick   = AL::sigmap.raster2(endTick, arrangerRaster());
            
            part->setTick(startTick);
            part->setLenTick(endTick - startTick);
            part->setName(mt->name());
            // copy events
            for (iEvent i = s; i != e; ++i) {
                  Event old = i->second;
                  Event event = old.clone();
                  event.setTick(old.tick() - startTick);
                  // addEvent also adds port controller values. So does msgAddPart, below. Let msgAddPart handle them.
                  //addEvent(event, part);
                  if(part->events()->find(event) == part->events()->end())
                    part->events()->add(event);
                  }
            MusEGlobal::audio->msgAddPart(part);
            updateFlags |= SC_PART_INSERTED;
            return;
            }

      updateFlags |= SC_EVENT_INSERTED;

      unsigned partTick = part->tick();
      if (endTick > part->endTick()) {
            // Determine new part length...
            endTick = 0;
            for (iEvent i = s; i != e; ++i) {
                  Event event = i->second;
                  unsigned tick = event.tick() - partTick + event.lenTick();
                  if (endTick < tick)
                        endTick = tick;
                  }
            
            // Round the end up (again) using the Arranger part snap raster value. 
            endTick   = AL::sigmap.raster2(endTick, arrangerRaster());
            
            
            removePortCtrlEvents(part, false); // Remove all of the part's port controller values. Don't do clone parts.

            // Clone the part. This doesn't increment aref count, and doesn't chain clones.
            // It also gives the new part a new serial number, but it is 
            //  overwritten with the old one by Song::changePart(), below. 
            Part* newPart = part->clone();
            
            newPart->setLenTick(endTick);  // Set the new part's length.
            changePart(part, newPart);     // Change the part.
            
            part->events()->incARef(-1);   // Manually adjust reference counts. HACK!
            newPart->events()->incARef(1);
            
            replaceClone(part, newPart);   // Replace the part in the clone chain with the new part.
            
            // Now add all of the new part's port controller values. Indicate do not do clone parts.
            addPortCtrlEvents(newPart, false);
            
            // Create an undo op. Indicate do port controller values but not clone parts. 
            addUndo(UndoOp(UndoOp::ModifyPart, part, newPart, true, false));
            updateFlags |= SC_PART_MODIFIED;
            
            if (_recMode == REC_REPLACE)
            {
                  iEvent si = newPart->events()->lower_bound(startTick - newPart->tick());
                  iEvent ei = newPart->events()->lower_bound(newPart->endTick() - newPart->tick());
                  for (iEvent i = si; i != ei; ++i) 
                  {
                    Event event = i->second;
                    // Indicate do port controller values and clone parts. 
                    addUndo(UndoOp(UndoOp::DeleteEvent, event, newPart, true, true));
                    // Remove the event from the new part's port controller values, and do all clone parts.
                    removePortCtrlEvents(event, newPart, true);
                  }
                  newPart->events()->erase(si, ei);
            }
            
            for (iEvent i = s; i != e; ++i) {
                  Event event = i->second;
                  event.setTick(event.tick() - partTick);
                  Event e;
                  // Create an undo op. Indicate do port controller values and clone parts. 
                  addUndo(UndoOp(UndoOp::AddEvent, e, event, newPart, true, true));
                  
                  if(newPart->events()->find(event) == newPart->events()->end())
                    newPart->events()->add(event);
                  
                  // Add the event to the new part's port controller values, and do all clone parts.
                  addPortCtrlEvents(event, newPart, true);
                  }
            }
      else {
            if (_recMode == REC_REPLACE) {
                  iEvent si = part->events()->lower_bound(startTick - part->tick());
                  iEvent ei = part->events()->lower_bound(endTick   - part->tick());

                  for (iEvent i = si; i != ei; ++i) {
                        Event event = i->second;
                        // Indicate that controller values and clone parts were handled.
                        addUndo(UndoOp(UndoOp::DeleteEvent, event, part, true, true));
                        // Remove the event from the part's port controller values, and do all clone parts.
                        removePortCtrlEvents(event, part, true);
                        }
                  part->events()->erase(si, ei);
                  }
            for (iEvent i = s; i != e; ++i) {
                  Event event = i->second;
                  int tick = event.tick() - partTick;
                  event.setTick(tick);
                  
                  // Indicate that controller values and clone parts were handled.
                  addUndo(UndoOp(UndoOp::AddEvent, event, part, true, true));
                  
                  if(part->events()->find(event) == part->events()->end())
                    part->events()->add(event);
                  
                  // Add the event to the part's port controller values, and do all clone parts.
                  addPortCtrlEvents(event, part, true);
                  }
            }
      }

//---------------------------------------------------------
//   findTrack
//---------------------------------------------------------

MidiTrack* Song::findTrack(const Part* part) const
      {
      for (ciTrack t = _tracks.begin(); t != _tracks.end(); ++t) {
            MidiTrack* track = dynamic_cast<MidiTrack*>(*t);
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
    for (iTrack it = tracks()->begin(); it != tracks()->end(); ++it)
        setRecordFlag(*it,false);
}

//---------------------------------------------------------
//   setRecord
//---------------------------------------------------------
void Song::setRecord(bool f, bool autoRecEnable)
      {
      if(MusEGlobal::debugMsg)
        printf("setRecord recordflag =%d f(record state)=%d autoRecEnable=%d\n", recordFlag, f, autoRecEnable);

      if (f && MusEGlobal::config.useProjectSaveDialog && MusEGlobal::museProject == MusEGlobal::museProjectInitPath ) { // check that there is a project stored before commencing
        // no project, we need to create one.
        if (!MusEGlobal::muse->saveAs())
          return; // could not store project, won't enable record
      }
      if (recordFlag != f) {
            if (f && autoRecEnable) {
                bool alreadyRecEnabled = false;
                Track *selectedTrack = 0;
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
                          selectedTrack = (*i);
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
                                selectedTrack = (*it);
                            }
                      }
                if (!alreadyRecEnabled && selectedTrack) {
                      setRecordFlag(selectedTrack, true);
                      }
                else if (alreadyRecEnabled)  {
                      // do nothing
                      }
                else  {
                      // if there are no tracks, do not enable record
                      if (!waves()->size() && !midis()->size()) {
                            printf("No track to select, won't enable record\n");
                            f = false;
                            }
                      }
                // prepare recording of wave files for all record enabled wave tracks
                for (MusECore::iWaveTrack i = wtl->begin(); i != wtl->end(); ++i) {
                      if((*i)->recordFlag() || (selectedTrack == (*i) && autoRecEnable)) // prepare if record flag or if it is set to recenable
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
      _masterFlag = val;
      if (MusEGlobal::tempomap.setMasterFlag(cpos(), val))
      {
        emit songChanged(SC_MASTER);
      }      
    }

//---------------------------------------------------------
//   setPlay
//    set transport play flag
//---------------------------------------------------------

void Song::setPlay(bool f)
      {
      if (MusEGlobal::extSyncFlag.value()) {
          if (MusEGlobal::debugMsg)
            printf("not allowed while using external sync");
          return;
      }
      // only allow the user to set the button "on"
      if (!f)
            MusEGlobal::playAction->setChecked(true);
      else
            MusEGlobal::audio->msgPlay(true);
      }

void Song::setStop(bool f)
      {
      if (MusEGlobal::extSyncFlag.value()) {
          if (MusEGlobal::debugMsg)
            printf("not allowed while using external sync");
          return;
      }
      // only allow the user to set the button "on"
      if (!f)
            MusEGlobal::stopAction->setChecked(true);
      else
            MusEGlobal::audio->msgPlay(false);
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
//   swapTracks
//---------------------------------------------------------

void Song::swapTracks(int i1, int i2)
      {
      addUndo(UndoOp(UndoOp::SwapTrack, i1, i2));
      Track* track = _tracks[i1];
      _tracks[i1]  = _tracks[i2];
      _tracks[i2]  = track;
      }

//---------------------------------------------------------
//   seekTo
//   setPos slot, only active when not doing playback
//---------------------------------------------------------
void Song::seekTo(int tick)
{
  if (!MusEGlobal::audio->isPlaying()) {
    Pos p(tick, true);
    setPos(0, p);
  }
}
//---------------------------------------------------------
//   setPos
//   MusEGlobal::song->setPos(Song::CPOS, pos, true, true, true);
//---------------------------------------------------------

void Song::setPos(int idx, const Pos& val, bool sig,
   bool isSeek, bool adjustScrollbar)
      {
      if (MusEGlobal::heavyDebugMsg)
      {
        printf("setPos %d sig=%d,seek=%d,scroll=%d  ",
           idx, sig, isSeek, adjustScrollbar);
        val.dump(0);
        printf("\n");
        printf("Song::setPos before MusEGlobal::audio->msgSeek idx:%d isSeek:%d frame:%d\n", idx, isSeek, val.frame());
      }
      
      if (idx == CPOS) {
            _vcpos = val;
            if (isSeek && !MusEGlobal::extSyncFlag.value()) {  
                  if (val == MusEGlobal::audio->pos())  
                  {
                      if (MusEGlobal::heavyDebugMsg) printf("Song::setPos seek MusEGlobal::audio->pos already == val tick:%d frame:%d\n", val.tick(), val.frame());   
                      return;
                  }     
                  MusEGlobal::audio->msgSeek(val);
                  if (MusEGlobal::heavyDebugMsg) printf("Song::setPos after MusEGlobal::audio->msgSeek idx:%d isSeek:%d frame:%d\n", idx, isSeek, val.frame());
                  return;
                  }
            }
      if (val == pos[idx])
      {
           if (MusEGlobal::heavyDebugMsg) printf("Song::setPos MusEGlobal::song->pos already == val tick:%d frame:%d\n", val.tick(), val.frame());   
           return;
      }     
      pos[idx] = val;
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
                  if (idx != LPOS && idx != RPOS)
                        emit posChanged(idx, pos[idx].tick(), adjustScrollbar);
                  }
            else
                  emit posChanged(idx, pos[idx].tick(), adjustScrollbar);
            }

      if (idx == CPOS) {
            iMarker i1 = _markerList->begin();
            iMarker i2 = i1;
            bool currentChanged = false;
            for (; i1 != _markerList->end(); ++i1) {
                  ++i2;
                  if (val.tick() >= i1->first && (i2==_markerList->end() || val.tick() < i2->first)) {
                        if (i1->second.current())
                              return;
                        i1->second.setCurrent(true);
                        if (currentChanged) {
                              emit markerChanged(MARKER_CUR);
                              return;
                              }
                        ++i1;
                        for (; i1 != _markerList->end(); ++i1) {
                              if (i1->second.current())
                                    i1->second.setCurrent(false);
                              }
                        emit markerChanged(MARKER_CUR);
                        return;
                        }
                  else {
                        if (i1->second.current()) {
                              currentChanged = true;
                              i1->second.setCurrent(false);
                              }
                        }
                  }
            if (currentChanged)
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

void Song::update(int flags)
      {
      static int level = 0;         // DEBUG
      if (level) {
            printf("Song::update %08x, level %d\n", flags, level);
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
//   setChannelMute
//    mute all midi tracks associated with channel
//---------------------------------------------------------

void Song::setChannelMute(int channel, bool val)
      {
      for (iTrack i = _tracks.begin(); i != _tracks.end(); ++i) {
            MidiTrack* track = dynamic_cast<MidiTrack*>(*i);
            if (track == 0)
                  continue;
            if (track->outChannel() == channel)
                  track->setMute(val);
            }
      emit songChanged(SC_MUTE);
      }

//---------------------------------------------------------
//   len
//---------------------------------------------------------

void Song::initLen()
      {
      _len = AL::sigmap.bar2tick(40, 0, 0);    // default song len
      for (iTrack t = _tracks.begin(); t != _tracks.end(); ++t) {
            MidiTrack* track = dynamic_cast<MidiTrack*>(*t);
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
//   tempoChanged
//---------------------------------------------------------

void Song::tempoChanged()
{
  emit songChanged(SC_TEMPO);
}

//---------------------------------------------------------
//   roundUpBar
//---------------------------------------------------------

int Song::roundUpBar(int t) const
      {
      int bar, beat;
      unsigned tick;
      AL::sigmap.tickValues(t, &bar, &beat, &tick);
      if (beat || tick)
            return AL::sigmap.bar2tick(bar+1, 0, 0);
      return t;
      }

//---------------------------------------------------------
//   roundUpBeat
//---------------------------------------------------------

int Song::roundUpBeat(int t) const
      {
      int bar, beat;
      unsigned tick;
      AL::sigmap.tickValues(t, &bar, &beat, &tick);
      if (tick)
            return AL::sigmap.bar2tick(bar, beat+1, 0);
      return t;
      }

//---------------------------------------------------------
//   roundDownBar
//---------------------------------------------------------

int Song::roundDownBar(int t) const
      {
      int bar, beat;
      unsigned tick;
      AL::sigmap.tickValues(t, &bar, &beat, &tick);
      return AL::sigmap.bar2tick(bar, 0, 0);
      }

//---------------------------------------------------------
//   dumpMaster
//---------------------------------------------------------

void Song::dumpMaster()
      {
      MusEGlobal::tempomap.dump();
      AL::sigmap.dump();
      }

//---------------------------------------------------------
//   getSelectedParts
//---------------------------------------------------------

PartList* Song::getSelectedMidiParts() const
      {
      PartList* parts = new PartList();

      /*
            If a part is selected, edit that. 
            If a track is selected, edit the first 
             part of the track, the rest are 
             'ghost parts' 
            When multiple parts are selected, then edit the first,
              the rest are 'ghost parts'
      */      
      
      
       // collect marked parts
      for (ciMidiTrack t = _midis.begin(); t != _midis.end(); ++t) {
            MidiTrack* track = *t;
            PartList* pl = track->parts();
            for (iPart p = pl->begin(); p != pl->end(); ++p) {
                  if (p->second->selected()) {
                        parts->add(p->second);
                        }
                  }
            }
      // if no part is selected, then search for selected track
      // and collect all parts of this track

      if (parts->empty()) {
            for (ciTrack t = _tracks.begin(); t != _tracks.end(); ++t) {
                  if ((*t)->selected()) {
                        MidiTrack* track = dynamic_cast<MidiTrack*>(*t);
                        if (track == 0)
                              continue;
                        PartList* pl = track->parts();
                        for (iPart p = pl->begin(); p != pl->end(); ++p)
                              parts->add(p->second);
                        break;
                        }
                  }
            }
      return parts;
      }

PartList* Song::getSelectedWaveParts() const
      {
      PartList* parts = new PartList();

      /*
            If a part is selected, edit that. 
            If a track is selected, edit the first 
             part of the track, the rest are 
             'ghost parts' 
            When multiple parts are selected, then edit the first,
              the rest are 'ghost parts'
      */      

      // collect selected parts
      for (ciTrack t = _tracks.begin(); t != _tracks.end(); ++t) {
            MusECore::WaveTrack* track = dynamic_cast<MusECore::WaveTrack*>(*t);
            if (track == 0)
                  continue;
            PartList* pl = track->parts();
            for (iPart p = pl->begin(); p != pl->end(); ++p) {
                  if (p->second->selected()) {
                        parts->add(p->second);
                        }
                  }
            }
      // if no parts are selected, then search the selected track
      // and collect all parts in this track

      if (parts->empty()) {
            for (ciTrack t = _tracks.begin(); t != _tracks.end(); ++t) {
                  if ((*t)->selected()) {
                        MusECore::WaveTrack* track =  dynamic_cast<MusECore::WaveTrack*>(*t);
                        if (track == 0)
                              continue;
                        PartList* pl = track->parts();
                        for (iPart p = pl->begin(); p != pl->end(); ++p)
                              parts->add(p->second);
                        break;
                        }
                  }
            }
      return parts;
      }

void Song::setMType(MType t)
      {
      _mtype = t;
      MusEGlobal::song->update(SC_SONG_TYPE);
      }

//---------------------------------------------------------
//   beat
//---------------------------------------------------------

void Song::beat()
      {
      // DELETETHIS 15
      #if 0
      // Just a rate test...
      static double _heartbeatRateTimer = 0.0;
      double t = MusEUtil::curTime();
      if(t - _heartbeatRateTimer > 0.0)
      {
        double rate = 1/ (t - _heartbeatRateTimer);
        printf("heartbeat rate:%f\n", rate);
        // Results: Song::beat() is not even called sometimes because apparently all the other
        //  stuff connected to the heartbeat is taking up all the time before the next timer event - 
        //  apparently Song::beat() is called last, or close to last - after the others. (Possible to choose order?)
        // With fancy strip meters active, Song::beat() was quiet for long periods of time!
      }
      _heartbeatRateTimer = t;
      #endif
      
      // Keep the sync detectors running... 
      for(int port = 0; port < MIDI_PORTS; ++port)
          MusEGlobal::midiPorts[port].syncInfo().setTime();
      
      
      if (MusEGlobal::audio->isPlaying())
        setPos(0, MusEGlobal::audio->tickPos(), true, false, true);

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
                        setPos(0, pos[LPOS].tick(), true, true, true);
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

void Song::setLen(unsigned l)
      {
      _len = l;
      update();
      }

//---------------------------------------------------------
//   addMarker
//---------------------------------------------------------

Marker* Song::addMarker(const QString& s, int t, bool lck)
      {
      Marker* marker = _markerList->add(s, t, lck);
      emit markerChanged(MARKER_ADD);
      return marker;
      }

//---------------------------------------------------------
//   addMarker
//---------------------------------------------------------

Marker* Song::getMarkerAt(int t)
      {
      iMarker markerI;
      for (markerI=_markerList->begin(); markerI != _markerList->end(); ++markerI) {
          if (unsigned(t) == markerI->second.tick()) //prevent of compiler warning: comparison signed/unsigned
            return &markerI->second;
          }
      return NULL;
      }

//---------------------------------------------------------
//   removeMarker
//---------------------------------------------------------

void Song::removeMarker(Marker* marker)
      {
      _markerList->remove(marker);
      emit markerChanged(MARKER_REMOVE);
      }

Marker* Song::setMarkerName(Marker* m, const QString& s)
      {
      m->setName(s);
      emit markerChanged(MARKER_NAME);
      return m;
      }

Marker* Song::setMarkerTick(Marker* m, int t)
      {
      Marker mm(*m);
      _markerList->remove(m);
      mm.setTick(t);
      m = _markerList->add(mm);
      emit markerChanged(MARKER_TICK);
      return m;
      }

Marker* Song::setMarkerLock(Marker* m, bool f)
      {
      m->setType(f ? Pos::FRAMES : Pos::TICKS);
      emit markerChanged(MARKER_LOCK);
      return m;
      }


//---------------------------------------------------------
//   setRecordFlag
//---------------------------------------------------------

void Song::setRecordFlag(Track* track, bool val)
      {
      if (track->type() == Track::WAVE) {
            MusECore::WaveTrack* audioTrack = (MusECore::WaveTrack*)track;
            if(!audioTrack->setRecordFlag1(val))
                return;
            MusEGlobal::audio->msgSetRecord(audioTrack, val);
            }
      else {
            track->setRecordFlag1(val);
            track->setRecordFlag2(val);
            }
      update(SC_RECFLAG);

      }

//---------------------------------------------------------
//   rescanAlsaPorts
//---------------------------------------------------------

void Song::rescanAlsaPorts()
      {
      emit midiPortsChanged();
      }

//---------------------------------------------------------
//   endMsgCmd
//---------------------------------------------------------

void Song::endMsgCmd()
      {
      if (updateFlags) {
            redoList->clearDelete();
            MusEGlobal::undoAction->setEnabled(true);
            MusEGlobal::redoAction->setEnabled(false);
            emit songChanged(updateFlags);
            }
      }

//---------------------------------------------------------
//   undo
//---------------------------------------------------------

void Song::undo()
      {
      updateFlags = 0;
      if (doUndo1())
            return;
      MusEGlobal::audio->msgUndo();
      doUndo3();
      MusEGlobal::redoAction->setEnabled(true);
      MusEGlobal::undoAction->setEnabled(!undoList->empty());

      if(updateFlags && (SC_TRACK_REMOVED | SC_TRACK_INSERTED))
        MusEGlobal::audio->msgUpdateSoloStates();

      emit songChanged(updateFlags);
      }

//---------------------------------------------------------
//   redo
//---------------------------------------------------------

void Song::redo()
      {
      updateFlags = 0;
      if (doRedo1())
            return;
      MusEGlobal::audio->msgRedo();
      doRedo3();
      MusEGlobal::undoAction->setEnabled(true);
      MusEGlobal::redoAction->setEnabled(!redoList->empty());

      if(updateFlags && (SC_TRACK_REMOVED | SC_TRACK_INSERTED))
        MusEGlobal::audio->msgUpdateSoloStates();

      emit songChanged(updateFlags);
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
            case SEQM_UNDO:
                  doUndo2();
                  break;
            case SEQM_REDO:
                  doRedo2();
                  break;
            case SEQM_MOVE_TRACK:
                  if (msg->a > msg->b) {
                        for (int i = msg->a; i > msg->b; --i) {
                              swapTracks(i, i-1);
                              }
                        }
                  else {
                        for (int i = msg->a; i < msg->b; ++i) {
                              swapTracks(i, i+1);
                              }
                        }
                  updateFlags = SC_TRACK_MODIFIED;
                  break;
            case SEQM_ADD_EVENT:
                  updateFlags = SC_EVENT_INSERTED;
                  if (addEvent(msg->ev1, (MidiPart*)msg->p2)) {
                        Event ev;
                        addUndo(UndoOp(UndoOp::AddEvent, ev, msg->ev1, (Part*)msg->p2, msg->a, msg->b));
                        }
                  else
                        updateFlags = 0;
                  if(msg->a)
                    addPortCtrlEvents(msg->ev1, (Part*)msg->p2, msg->b);
                  break;
            case SEQM_REMOVE_EVENT:
                  {
                  Event event = msg->ev1;
                  MidiPart* part = (MidiPart*)msg->p2;
                  if(msg->a)
                    removePortCtrlEvents(event, part, msg->b);
                  Event e;
                  addUndo(UndoOp(UndoOp::DeleteEvent, e, event, (Part*)part, msg->a, msg->b));
                  deleteEvent(event, part);
                  updateFlags = SC_EVENT_REMOVED;
                  }
                  break;
            case SEQM_CHANGE_EVENT:
                  if(msg->a)
                    removePortCtrlEvents(msg->ev1, (MidiPart*)msg->p3, msg->b);
                  changeEvent(msg->ev1, msg->ev2, (MidiPart*)msg->p3);
                  if(msg->a)
                    addPortCtrlEvents(msg->ev2, (Part*)msg->p3, msg->b);
                  addUndo(UndoOp(UndoOp::ModifyEvent, msg->ev2, msg->ev1, (Part*)msg->p3, msg->a, msg->b));
                  updateFlags = SC_EVENT_MODIFIED;
                  break;
            
            // Moved here from MidiSeq::processMsg   p4.0.34
            case SEQM_ADD_TRACK:
                  insertTrack2(msg->track, msg->ival);
                  break;
            case SEQM_REMOVE_TRACK:
                  cmdRemoveTrack(msg->track);
                  break;
            //case SEQM_CHANGE_TRACK:     DELETETHIS 3
            //      changeTrack((Track*)(msg->p1), (Track*)(msg->p2));
            //      break;
            case SEQM_ADD_PART:
                  cmdAddPart((Part*)msg->p1);
                  break;
            case SEQM_REMOVE_PART:
                  cmdRemovePart((Part*)msg->p1);
                  break;
            case SEQM_CHANGE_PART:
                  cmdChangePart((Part*)msg->p1, (Part*)msg->p2, msg->a, msg->b);
                  break;
            
            case SEQM_ADD_TEMPO:
                  addUndo(UndoOp(UndoOp::AddTempo, msg->a, msg->b));
                  MusEGlobal::tempomap.addTempo(msg->a, msg->b);
                  updateFlags = SC_TEMPO;
                  break;

            case SEQM_SET_TEMPO:
                  addUndo(UndoOp(UndoOp::AddTempo, msg->a, msg->b));
                  MusEGlobal::tempomap.setTempo(msg->a, msg->b);
                  updateFlags = SC_TEMPO;
                  break;

            case SEQM_SET_GLOBAL_TEMPO:
                  MusEGlobal::tempomap.setGlobalTempo(msg->a);
                  break;

            case SEQM_REMOVE_TEMPO:
                  addUndo(UndoOp(UndoOp::DeleteTempo, msg->a, msg->b));
                  MusEGlobal::tempomap.delTempo(msg->a);
                  updateFlags = SC_TEMPO;
                  break;

            case SEQM_ADD_SIG:
                  addUndo(UndoOp(UndoOp::AddSig, msg->a, msg->b, msg->c));
                  AL::sigmap.add(msg->a, AL::TimeSignature(msg->b, msg->c));
                  updateFlags = SC_SIG;
                  break;

            case SEQM_REMOVE_SIG:
                  addUndo(UndoOp(UndoOp::DeleteSig, msg->a, msg->b, msg->c));
                  AL::sigmap.del(msg->a);
                  updateFlags = SC_SIG;
                  break;

            case SEQM_ADD_KEY:
                  addUndo(UndoOp(UndoOp::AddKey, msg->a, msg->b));
                  MusEGlobal::keymap.addKey(msg->a, (key_enum) msg->b);
                  updateFlags = SC_KEY;
                  break;

            case SEQM_REMOVE_KEY:
                  addUndo(UndoOp(UndoOp::DeleteKey, msg->a, msg->b));
                  MusEGlobal::keymap.delKey(msg->a);
                  updateFlags = SC_KEY;
                  break;

            default:
                  printf("unknown seq message %d\n", msg->id);
                  break;
            }
      }

//---------------------------------------------------------
//   cmdAddPart
//---------------------------------------------------------

void Song::cmdAddPart(Part* part)
      {
      addPart(part);
      addUndo(UndoOp(UndoOp::AddPart, part));
      updateFlags = SC_PART_INSERTED;
      }

//---------------------------------------------------------
//   cmdRemovePart
//---------------------------------------------------------

void Song::cmdRemovePart(Part* part)
      {
      removePart(part);
      addUndo(UndoOp(UndoOp::DeletePart, part));
      part->events()->incARef(-1);
      unchainClone(part);
      updateFlags = SC_PART_REMOVED;
      }

//---------------------------------------------------------
//   cmdChangePart
//---------------------------------------------------------

void Song::cmdChangePart(Part* oldPart, Part* newPart, bool doCtrls, bool doClones)
      {
      if(doCtrls)
        removePortCtrlEvents(oldPart, doClones);
      
      changePart(oldPart, newPart);
      
      addUndo(UndoOp(UndoOp::ModifyPart, oldPart, newPart, doCtrls, doClones));
      
      // Changed by T356. Do not decrement ref count if the new part is a clone of the old part, since the event list
      //  will still be active.
      if(oldPart->cevents() != newPart->cevents())
        oldPart->events()->incARef(-1);
      
      replaceClone(oldPart, newPart);

      if(doCtrls)
        addPortCtrlEvents(newPart, doClones);
      
      updateFlags = SC_PART_MODIFIED;
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
        printf("Song::clear\n");
      
      bounceTrack    = 0;
      
      _tracks.clear();
      _midis.clearDelete();
      _waves.clearDelete();
      _inputs.clearDelete();     // audio input ports
      _outputs.clearDelete();    // audio output ports
      _groups.clearDelete();     // mixer groups
      _auxs.clearDelete();       // aux sends
      
      // p3.3.45 Clear all midi port devices.
      for(int i = 0; i < MIDI_PORTS; ++i)
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
          else if(dynamic_cast< MidiAlsaDevice* >(*imd))
          {
            // With alsa devices, we must not delete them (they're always in the list). But we must 
            //  clear all routes. They point to non-existant midi tracks, which were all deleted above.
            (*imd)->inRoutes()->clear();
            (*imd)->outRoutes()->clear();
          }
        }
      }  
      while (loop);
      
      MusEGlobal::tempomap.clear();
      MusEGlobal::tempo_rec_list.clear();
      AL::sigmap.clear();
      MusEGlobal::keymap.clear();
      
      undoList->clearDelete();
      redoList->clearDelete();
      if(MusEGlobal::undoAction)
        MusEGlobal::undoAction->setEnabled(false);
      if(MusEGlobal::redoAction)
        MusEGlobal::redoAction->setEnabled(false);
      
      _markerList->clear();
      pos[0].setTick(0);
      pos[1].setTick(0);
      pos[2].setTick(0);
      _vcpos.setTick(0);

      Track::clearSoloRefCounts();
      clearMidiTransforms();
      clearMidiInputTransforms();

      // Clear all midi port controller values.
      for(int i = 0; i < MIDI_PORTS; ++i)
        // Don't remove the controllers, just the values.
        MusEGlobal::midiPorts[i].controller()->clearDelete(false);

      _masterFlag    = true;
      loopFlag       = false;
      loopFlag       = false;
      punchinFlag    = false;
      punchoutFlag   = false;
      recordFlag     = false;
      soloFlag       = false;
      // seq
      _mtype         = MT_UNKNOWN;
      _recMode       = REC_OVERDUP;
      _cycleMode     = CYCLE_NORMAL;
      _click         = false;
      _quantize      = false;
      _len           = 0;           // song len in ticks
      _follow        = JUMP;
      dirty          = false;
      initDrumMap();
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
        printf("MusE: Song::cleanupForQuit...\n");
      
      _tracks.clear();
      
      if(MusEGlobal::debugMsg)
        printf("deleting _midis\n");
      _midis.clearDelete();
      
      if(MusEGlobal::debugMsg)
        printf("deleting _waves\n");
      _waves.clearDelete();
      
      if(MusEGlobal::debugMsg)
        printf("deleting _inputs\n");
      _inputs.clearDelete();     // audio input ports
      
      if(MusEGlobal::debugMsg)
        printf("deleting _outputs\n");
      _outputs.clearDelete();    // audio output ports
      
      if(MusEGlobal::debugMsg)
        printf("deleting _groups\n");
      _groups.clearDelete();     // mixer groups
      
      if(MusEGlobal::debugMsg)
        printf("deleting _auxs\n");
      _auxs.clearDelete();       // aux sends
      
      if(MusEGlobal::debugMsg)
        printf("deleting _synthIs\n");
      _synthIs.clearDelete();    // each ~SynthI() -> deactivate3() -> ~SynthIF()

      MusEGlobal::tempomap.clear();
      AL::sigmap.clear();
      MusEGlobal::keymap.clear();
      
      if(MusEGlobal::debugMsg)
        printf("deleting undoList and redoList\n");
      undoList->clearDelete();
      redoList->clearDelete();
      
      _markerList->clear();
      
      if(MusEGlobal::debugMsg)
        printf("deleting transforms\n");
      clearMidiTransforms(); // Deletes stuff.
      clearMidiInputTransforms(); // Deletes stuff.

      if(MusEGlobal::debugMsg)
        printf("deleting midiport controllers\n");
      
      // Clear all midi port controllers and values.
      for(int i = 0; i < MIDI_PORTS; ++i)
        MusEGlobal::midiPorts[i].controller()->clearDelete(true); // Remove the controllers and the values.
        
      // Can't do this here. Jack isn't running. Fixed. Test OK so far. DELETETHIS (the comment and #if/#endif)
      #if 1
      if(MusEGlobal::debugMsg)
        printf("deleting midi devices except synths\n");
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
        printf("deleting global available synths\n");

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
        printf("deleting midi instruments\n");
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
        printf("...finished cleaning up.\n");
}

//---------------------------------------------------------
//   seqSignal
//    sequencer message to GUI
//    execution environment: gui thread
//---------------------------------------------------------

void Song::seqSignal(int fd)
      {
      char buffer[16];

      int n = ::read(fd, buffer, 16);
      if (n < 0) {
            printf("Song: seqSignal(): READ PIPE failed: %s\n",
               strerror(errno));
            return;
            }
      for (int i = 0; i < n; ++i) {
            switch(buffer[i]) {
                  case '0':         // STOP
                        stopRolling();
                        break;
                  case '1':         // PLAY
                        setStopPlay(true);
                        break;
                  case '2':   // record
                        setRecord(true);
                        break;
                  case '3':   // START_PLAY + jack STOP
                        abortRolling();
                        break;
                  case 'P':   // alsa ports changed
                        rescanAlsaPorts();
                        break;
                  case 'G':
                        clearRecAutomation(true);
                        setPos(0, MusEGlobal::audio->tickPos(), true, false, true);
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
                              printf("restarting!\n");
                              MusEGlobal::muse->seqRestart();
                              }
                        }

                        break;
                  case 'f':   // start freewheel
                        if(MusEGlobal::debugMsg)
                          printf("Song: seqSignal: case f: setFreewheel start\n");
                        
                        if(MusEGlobal::config.freewheelMode)
                          MusEGlobal::audioDevice->setFreewheel(true);
                        
                        break;

                  case 'F':   // stop freewheel
                        if(MusEGlobal::debugMsg)
                          printf("Song: seqSignal: case F: setFreewheel stop\n");
                        
                        if(MusEGlobal::config.freewheelMode)
                          MusEGlobal::audioDevice->setFreewheel(false);
                        
                        MusEGlobal::audio->msgPlay(false);
#if 0 // DELETETHIS
                        if (record())
                              MusEGlobal::audio->recordStop();
                        setStopPlay(false);
#endif
                        break;

                  case 'C': // Graph changed
                        if (MusEGlobal::audioDevice)
                            MusEGlobal::audioDevice->graphChanged();
                        break;

                  case 'R': // Registration changed
                        if (MusEGlobal::audioDevice)
                            MusEGlobal::audioDevice->registrationChanged();
                        break;

                  default:
                        printf("unknown Seq Signal <%c>\n", buffer[i]);
                        break;
                  }
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
      MidiPart* part = 0;
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
            part          = new MidiPart(mt);
            int startTick = roundDownBar(tick);
            int endTick   = roundUpBar(tick + 1);
            part->setTick(startTick);
            part->setLenTick(endTick - startTick);
            part->setName(mt->name());
            event.move(-startTick);
            part->events()->add(event);
            MusEGlobal::audio->msgAddPart(part);
            return;
            }
      part = (MidiPart*)(ip->second);
      tick -= part->tick();
      event.setTick(tick);
      
      Event ev;
      if(event.type() == Controller)
      {
        EventRange range = part->events()->equal_range(tick);
        for(iEvent i = range.first; i != range.second; ++i) 
        {
          ev = i->second;
          if(ev.type() == Controller && ev.dataA() == event.dataA())
          {
            if(ev.dataB() == event.dataB()) // Don't bother if already set.
              return;
            // Indicate do undo, and do port controller values and clone parts. 
            MusEGlobal::audio->msgChangeEvent(ev, event, part, true, true, true);
            return;
          }
        }
      }  
      
      // Indicate do undo, and do port controller values and clone parts. 
      MusEGlobal::audio->msgAddEvent(event, part, true, true, true);
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
  int frame = 0;
  if(track)
  {
    ciCtrlList icl = track->controller()->find(acid);
    if(icl != track->controller()->end())
    {
      CtrlList *cl = icl->second;
      canAdd = true;
      
      frame = MusEGlobal::audio->pos().frame();       
      
      bool en1, en2;
      track->controllersEnabled(acid, &en1, &en2);
      
      AutomationType at = track->automationType();
      if(!MusEGlobal::automation || at == AUTO_OFF || !en1 || !en2) 
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
                        && (int)pos[2].frame() > s->second.frame;
      }
    }
  }

  menu->addAction(new MusEGui::MenuTitleItem(tr("Automation:"), menu));
  
  QAction* prevEvent = menu->addAction(tr("previous event"));
  prevEvent->setData(PREV_EVENT);
  prevEvent->setEnabled(canSeekPrev);

  QAction* nextEvent = menu->addAction(tr("next event"));
  nextEvent->setData(NEXT_EVENT);
  nextEvent->setEnabled(canSeekNext);

  menu->addSeparator();

  QAction* addEvent = new QAction(menu);
  menu->addAction(addEvent);
  if(isEvent)
    addEvent->setText(tr("set event"));
  else  
    addEvent->setText(tr("add event"));
  addEvent->setData(ADD_EVENT);
  addEvent->setEnabled(canAdd);

  QAction* eraseEventAction = menu->addAction(tr("erase event"));
  eraseEventAction->setData(CLEAR_EVENT);
  eraseEventAction->setEnabled(isEvent);

  QAction* eraseRangeAction = menu->addAction(tr("erase range"));
  eraseRangeAction->setData(CLEAR_RANGE);
  eraseRangeAction->setEnabled(canEraseRange);

  QAction* clearAction = menu->addAction(tr("clear automation"));
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
  
  switch(sel)
  {
    case ADD_EVENT:
          MusEGlobal::audio->msgAddACEvent(track, acid, frame, ctlval);
    break;
    case CLEAR_EVENT:
          MusEGlobal::audio->msgEraseACEvent(track, acid, frame);
    break;

    case CLEAR_RANGE:
          MusEGlobal::audio->msgEraseRangeACEvents(track, acid, pos[1].frame(), pos[2].frame());
    break;

    case CLEAR_ALL_EVENTS:
          if(QMessageBox::question(MusEGlobal::muse, QString("Muse"),
              tr("Clear all controller events?"), tr("&Ok"), tr("&Cancel"),
              QString::null, 0, 1 ) == 0)
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
  
  return sel;
}

//---------------------------------------------------------
//   execMidiAutomationCtlPopup
//---------------------------------------------------------

int Song::execMidiAutomationCtlPopup(MidiTrack* track, MidiPart* part, const QPoint& menupos, int ctlnum)
{
  if(!track && !part)
    return -1;
    
  enum { ADD_EVENT, CLEAR_EVENT };
  QMenu* menu = new QMenu;

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
  MidiController *mc = mp->drumController(ctlnum);
  if(mc)
  {
    // Change the controller event's index into the drum map to an instrument note.
    int note = ctlnum & 0x7f;
    dctl &= ~0xff;
    channel = MusEGlobal::drumMap[note].channel;
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
      EventRange range = part->events()->equal_range(tick - partStart);
      for(iEvent i = range.first; i != range.second; ++i) 
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
  
  menu->addAction(new MusEGui::MenuTitleItem(tr("Automation:"), menu));
  
  QAction* addEvent = new QAction(menu);
  menu->addAction(addEvent);
  if(isEvent)
    addEvent->setText(tr("set event"));
  else
    addEvent->setText(tr("add event"));
  addEvent->setData(ADD_EVENT);
  addEvent->setEnabled(true);

  QAction* eraseEventAction = menu->addAction(tr("erase event"));
  eraseEventAction->setData(CLEAR_EVENT);
  eraseEventAction->setEnabled(isEvent);

  QAction* act = menu->exec(menupos);
  if (!act)
  {
    delete menu;
    return -1;
  }
  
  int sel = act->data().toInt();
  delete menu;
  
  switch(sel)
  {
    case ADD_EVENT:
    {
          int val = mp->hwCtrlState(channel, dctl);
          if(val == CTRL_VAL_UNKNOWN) 
            return -1;
          Event e(Controller);
          e.setA(ctlnum);
          e.setB(val);
          // Do we replace an old event?
          if(isEvent)
          {
            if(ev.dataB() == val) // Don't bother if already set.
              return -1;
              
            e.setTick(tick - part->tick());
            // Indicate do undo, and do port controller values and clone parts. 
            MusEGlobal::audio->msgChangeEvent(ev, e, part, true, true, true);
          }
          else
          {
            // Store a new event...
            if(part)
            {
              e.setTick(tick - part->tick());
              // Indicate do undo, and do port controller values and clone parts. 
              MusEGlobal::audio->msgAddEvent(e, part, true, true, true);
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
              part->events()->add(e);
              // Allow undo.
              MusEGlobal::audio->msgAddPart(part);
            }
          }  
    }
    break;
    case CLEAR_EVENT:
          // Indicate do undo, and do port controller values and clone parts. 
          MusEGlobal::audio->msgDeleteEvent(ev, part, true, true, true);
    break;

    default:
          return -1;
    break;      
  }
  
  return sel;
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

void Song::processAutomationEvents()
{
  MusEGlobal::audio->msgIdle(true); // gain access to all data structures
   
  // Just clear all pressed and touched flags, not rec event lists.
  clearRecAutomation(false);
  if (!MusEGlobal::automation)
  {
    MusEGlobal::audio->msgIdle(false);
    return;
  }
  
  for(iTrack i = _tracks.begin(); i != _tracks.end(); ++i)
  {
    if(!(*i)->isMidiTrack())
      // Process (and clear) rec events.
      ((AudioTrack*)(*i))->processAutomationEvents();
  }

  MusEGlobal::audio->msgIdle(false); 
}

//---------------------------------------------------------
//   processMasterRec
//---------------------------------------------------------

void Song::processMasterRec()
{
  bool do_tempo = false;
  
  // Wait a few seconds for the tempo fifo to be empty.
  int tout = 30;
  while(!_tempoFifo.isEmpty())
  {
    usleep(100000);
    --tout;
    if(tout == 0)
      break;
  }
  
  int tempo_rec_list_sz = MusEGlobal::tempo_rec_list.size();
  if(tempo_rec_list_sz != 0) 
  {
    if(QMessageBox::question(MusEGlobal::muse, 
                          tr("MusE: Tempo list"), 
                          tr("External tempo changes were recorded.\nTransfer them to master tempo list?"),
                          QMessageBox::Ok | QMessageBox::Cancel, QMessageBox::Cancel) == QMessageBox::Ok)
       do_tempo = true;
  }
  
  MusEGlobal::audio->msgIdle(true); // gain access to all data structures

  if(do_tempo)
  {
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
  }
  
  MusEGlobal::tempo_rec_list.clear();
  
  MusEGlobal::audio->msgIdle(false); 

  if(do_tempo)
    update(SC_TEMPO);
}

//---------------------------------------------------------
//   abortRolling
//---------------------------------------------------------

void Song::abortRolling()
{
  if (record())
        MusEGlobal::audio->recordStop();
  setStopPlay(false);
}

//---------------------------------------------------------
//   stopRolling
//---------------------------------------------------------

void Song::stopRolling()
      {
      if (record())
            MusEGlobal::audio->recordStop();
      setStopPlay(false);
      
      processAutomationEvents();
      }

//---------------------------------------------------------
//   connectJackRoutes
//---------------------------------------------------------

void Song::connectJackRoutes(AudioTrack* track, bool disconnect)
{
  switch(track->type())
  {
        case Track::AUDIO_OUTPUT:
                {
                    AudioOutput* ao = (AudioOutput*)track;
                    // This will re-register the track's jack ports.
                    if(!disconnect)
                    ao->setName(ao->name());
                    // Now reconnect the output routes.
                    if(MusEGlobal::checkAudioDevice() && MusEGlobal::audio->isRunning())
                    {
                    for(int ch = 0; ch < ao->channels(); ++ch)
                    {
                        RouteList* ir = ao->outRoutes();
                        for (ciRoute ii = ir->begin(); ii != ir->end(); ++ii)
                        {
                            Route r = *ii;
                            if ((r.type == Route::JACK_ROUTE) && (r.channel == ch))
                            {
                                    if(disconnect)
                                    MusEGlobal::audioDevice->disconnect(ao->jackPort(ch), r.jackPort);
                                    else
                                    MusEGlobal::audioDevice->connect(ao->jackPort(ch), r.jackPort);
                                    break;
                            }
                        }
                        if(disconnect)
                        {
                        MusEGlobal::audioDevice->unregisterPort(ao->jackPort(ch));
                        ao->setJackPort(ch, 0);
                        }
                    }
                    }
                }
              break;
        case Track::AUDIO_INPUT:
                {
                    AudioInput* ai = (AudioInput*)track;
                    // This will re-register the track's jack ports.
                    if(!disconnect)
                    ai->setName(ai->name());
                    // Now reconnect the input routes.
                    if(MusEGlobal::checkAudioDevice() && MusEGlobal::audio->isRunning())
                    {
                        for(int ch = 0; ch < ai->channels(); ++ch)
                        {
                            RouteList* ir = ai->inRoutes();
                            for (ciRoute ii = ir->begin(); ii != ir->end(); ++ii)
                            {
                                Route r = *ii;
                                if ((r.type == Route::JACK_ROUTE) && (r.channel == ch))
                                {
                                        if(disconnect)
                                        MusEGlobal::audioDevice->disconnect(r.jackPort, ai->jackPort(ch));
                                        else
                                        MusEGlobal::audioDevice->connect(r.jackPort, ai->jackPort(ch));
                                        break;
                                }
                            }
                            if(disconnect)
                            {
                            MusEGlobal::audioDevice->unregisterPort(ai->jackPort(ch));
                            ai->setJackPort(ch, 0);
                            }
                        }
                    }
                }
              break;
        default:
              break;
  }
}

//---------------------------------------------------------
//   insertTrack0
//---------------------------------------------------------

void Song::insertTrack0(Track* track, int idx)
      {
      insertTrack1(track, idx);
      insertTrack2(track, idx);  // MusEGlobal::audio->msgInsertTrack(track, idx, false); DELETETHIS or is this somehow explanatory?
      insertTrack3(track, idx);
      }

//---------------------------------------------------------
//   insertTrack1
//    non realtime part of insertTrack
//---------------------------------------------------------

void Song::insertTrack1(Track* track, int /*idx*/)
      {
      switch(track->type()) {
            case Track::AUDIO_SOFTSYNTH:
                  {
                  SynthI* s = (SynthI*)track;
                  Synth* sy = s->synth();
                  if (!s->isActivated()) {
                        s->initInstance(sy, s->name());
                        }
                  }
                  break;
            default:
                  break;
            }
      }

//---------------------------------------------------------
//   insertTrack2
//    realtime part
//---------------------------------------------------------

void Song::insertTrack2(Track* track, int idx)
{
      int n;
      switch(track->type()) {
            case Track::MIDI:
            case Track::DRUM:
                  _midis.push_back((MidiTrack*)track);
                  addPortCtrlEvents(((MidiTrack*)track));
                  
                  break;
            case Track::WAVE:
                  _waves.push_back((MusECore::WaveTrack*)track);
                  break;
            case Track::AUDIO_OUTPUT:
                  _outputs.push_back((AudioOutput*)track);
                  // set default master & monitor if not defined
                  if (MusEGlobal::audio->audioMaster() == 0)
                        MusEGlobal::audio->setMaster((AudioOutput*)track);
                  if (MusEGlobal::audio->audioMonitor() == 0)
                        MusEGlobal::audio->setMonitor((AudioOutput*)track);
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

      if (track->type() == Track::AUDIO_OUTPUT) 
      {
            const RouteList* rl = track->inRoutes();
            for (ciRoute r = rl->begin(); r != rl->end(); ++r)
            {
                  Route src(track, r->channel, r->channels);
                  src.remoteChannel = r->remoteChannel;
                  r->track->outRoutes()->push_back(src);
                  // Is the source an Aux Track or else does it have Aux Tracks routed to it?
                  // Update the Audio Output track's aux ref count.     p4.0.37
                  if(r->track->auxRefCount())
                    track->updateAuxRoute( r->track->auxRefCount(), NULL );
                  else if(r->track->type() == Track::AUDIO_AUX)
                    track->updateAuxRoute( 1, NULL );
            }      
      }
      else if (track->type() == Track::AUDIO_INPUT) 
      {
            const RouteList* rl = track->outRoutes();
            for (ciRoute r = rl->begin(); r != rl->end(); ++r)
            {
                  Route src(track, r->channel, r->channels);
                  src.remoteChannel = r->remoteChannel;
                  r->track->inRoutes()->push_back(src);
                  // Is this track an Aux Track or else does it have Aux Tracks routed to it?
                  // Update the other track's aux ref count and all tracks it is connected to.     p4.0.37
                  if(track->auxRefCount())
                    r->track->updateAuxRoute( track->auxRefCount(), NULL );
                  else if(track->type() == Track::AUDIO_AUX)
                    r->track->updateAuxRoute( 1, NULL );
            }      
      }
      else if (track->isMidiTrack())          // p3.3.50
      {
            const RouteList* rl = track->inRoutes();
            for (ciRoute r = rl->begin(); r != rl->end(); ++r)
            {
                  Route src(track, r->channel);
                  MusEGlobal::midiPorts[r->midiPort].outRoutes()->push_back(src);
            }
            rl = track->outRoutes();
            for (ciRoute r = rl->begin(); r != rl->end(); ++r)
            {
                  Route src(track, r->channel);
                  MusEGlobal::midiPorts[r->midiPort].inRoutes()->push_back(src);
            }      
      }
      else 
      {
            const RouteList* rl = track->inRoutes();
            for (ciRoute r = rl->begin(); r != rl->end(); ++r)
            {
                  Route src(track, r->channel, r->channels);
                  src.remoteChannel = r->remoteChannel;
                  r->track->outRoutes()->push_back(src);
                  // Is the source an Aux Track or else does it have Aux Tracks routed to it?
                  // Update this track's aux ref count.     p4.0.37
                  if(r->track->auxRefCount())
                    track->updateAuxRoute( r->track->auxRefCount(), NULL );
                  else if(r->track->type() == Track::AUDIO_AUX)
                    track->updateAuxRoute( 1, NULL );
            }
            rl = track->outRoutes();
            for (ciRoute r = rl->begin(); r != rl->end(); ++r)
            {
                  Route src(track, r->channel, r->channels);
                  src.remoteChannel = r->remoteChannel;
                  r->track->inRoutes()->push_back(src);
                  // Is this track an Aux Track or else does it have Aux Tracks routed to it?
                  // Update the other track's aux ref count and all tracks it is connected to.     p4.0.37
                  if(track->auxRefCount())
                    r->track->updateAuxRoute( track->auxRefCount(), NULL );
                  else if(track->type() == Track::AUDIO_AUX)
                    r->track->updateAuxRoute( 1, NULL );
            }      
      }
}

//---------------------------------------------------------
//   insertTrack3
//    non realtime part of insertTrack
//---------------------------------------------------------

// empty. gets executed after the realtime part
void Song::insertTrack3(Track* /*track*/, int /*idx*/)//prevent compiler warning: unused parameter
{
}

//---------------------------------------------------------
//   removeTrack0
//---------------------------------------------------------

void Song::removeTrack0(Track* track)
      {
      removeTrack1(track);
      MusEGlobal::audio->msgRemoveTrack(track);
      removeTrack3(track);
      update(SC_TRACK_REMOVED);
      }

//---------------------------------------------------------
//   removeTrack1
//    non realtime part of removeTrack
//---------------------------------------------------------

void Song::removeTrack1(Track* track)
      {
        switch(track->type())
        {
          case Track::WAVE:
          case Track::AUDIO_OUTPUT:
          case Track::AUDIO_INPUT:
          case Track::AUDIO_GROUP:
          case Track::AUDIO_AUX:
          case Track::AUDIO_SOFTSYNTH:
              ((AudioTrack*)track)->deleteAllEfxGuis();
              break;
          default:
              break;
        }

        switch(track->type())
        {
          case Track::AUDIO_OUTPUT:
          case Track::AUDIO_INPUT:
                connectJackRoutes((AudioTrack*)track, true);
              break;
          case Track::AUDIO_SOFTSYNTH:
                {
                SynthI* si = (SynthI*)track;
                if(si->hasGui())
                  si->showGui(false);
                if(si->hasNativeGui())       
                  si->showNativeGui(false);
                }
              break;
          default:
              break;
        }
      }

//---------------------------------------------------------
//   removeTrack
//    called from RT context
//---------------------------------------------------------

void Song::removeTrack2(Track* track)
{
      switch(track->type()) {
            case Track::MIDI:
            case Track::DRUM:
                  removePortCtrlEvents(((MidiTrack*)track));
                  unchainTrackParts(track, true);
                  
                  _midis.erase(track);
                  break;
            case Track::WAVE:
                  unchainTrackParts(track, true);
                  
                  _waves.erase(track);
                  break;
            case Track::AUDIO_OUTPUT:
                  _outputs.erase(track);
                  break;
            case Track::AUDIO_INPUT:
                  _inputs.erase(track);
                  break;
            case Track::AUDIO_GROUP:
                  _groups.erase(track);
                  break;
            case Track::AUDIO_AUX:
                  _auxs.erase(track);
                  break;
            case Track::AUDIO_SOFTSYNTH:
                  {
                  SynthI* s = (SynthI*) track;
                  s->deactivate2();
                  _synthIs.erase(track);
                  }
                  break;
            }
      _tracks.erase(track);
      
      //  remove routes

      if (track->type() == Track::AUDIO_OUTPUT) 
      {
            const RouteList* rl = track->inRoutes();
            for (ciRoute r = rl->begin(); r != rl->end(); ++r)
            {
                  Route src(track, r->channel, r->channels);
                  src.remoteChannel = r->remoteChannel;
                  r->track->outRoutes()->removeRoute(src);
                  // Is the source an Aux Track or else does it have Aux Tracks routed to it?
                  // Update the Audio Output track's aux ref count.     p4.0.37
                  if(r->track->auxRefCount())
                    track->updateAuxRoute( -r->track->auxRefCount(), NULL );
                  else if(r->track->type() == Track::AUDIO_AUX)
                    track->updateAuxRoute( -1, NULL );
            }      
      }
      else if (track->type() == Track::AUDIO_INPUT) 
      {
            const RouteList* rl = track->outRoutes();
            for (ciRoute r = rl->begin(); r != rl->end(); ++r)
            {
                  Route src(track, r->channel, r->channels);
                  src.remoteChannel = r->remoteChannel;
                  r->track->inRoutes()->removeRoute(src);
                  // Is this track an Aux Track or else does it have Aux Tracks routed to it?
                  // Update the other track's aux ref count and all tracks it is connected to.     p4.0.37
                  if(track->auxRefCount())
                    r->track->updateAuxRoute( -track->auxRefCount(), NULL );
                  else if(track->type() == Track::AUDIO_AUX)
                    r->track->updateAuxRoute( -1, NULL );
            }      
      }
      else if (track->isMidiTrack())          // p3.3.50
      {
            const RouteList* rl = track->inRoutes();
            for (ciRoute r = rl->begin(); r != rl->end(); ++r)
            {
                  Route src(track, r->channel);
                  MusEGlobal::midiPorts[r->midiPort].outRoutes()->removeRoute(src);
            }
            rl = track->outRoutes();
            for (ciRoute r = rl->begin(); r != rl->end(); ++r)
            {
                  Route src(track, r->channel);
                  MusEGlobal::midiPorts[r->midiPort].inRoutes()->removeRoute(src);
            }      
      }
      else 
      {
            const RouteList* rl = track->inRoutes();
            for (ciRoute r = rl->begin(); r != rl->end(); ++r)
            {
                  Route src(track, r->channel, r->channels);
                  src.remoteChannel = r->remoteChannel;
                  r->track->outRoutes()->removeRoute(src);
                  // Is the source an Aux Track or else does it have Aux Tracks routed to it?
                  // Update this track's aux ref count.     p4.0.37
                  if(r->track->auxRefCount())
                    track->updateAuxRoute( -r->track->auxRefCount(), NULL );
                  else if(r->track->type() == Track::AUDIO_AUX)
                    track->updateAuxRoute( -1, NULL );
            }
            rl = track->outRoutes();
            for (ciRoute r = rl->begin(); r != rl->end(); ++r)
            {
                  Route src(track, r->channel, r->channels);
                  src.remoteChannel = r->remoteChannel;
                  r->track->inRoutes()->removeRoute(src);
                  // Is this track an Aux Track or else does it have Aux Tracks routed to it?
                  // Update the other track's aux ref count and all tracks it is connected to.     p4.0.37
                  if(track->auxRefCount())
                    r->track->updateAuxRoute( -track->auxRefCount(), NULL );
                  else if(track->type() == Track::AUDIO_AUX)
                    r->track->updateAuxRoute( -1, NULL );
            }      
      }
      
}

//---------------------------------------------------------
//   removeTrack3
//    non realtime part of removeTrack
//---------------------------------------------------------

//empty. gets executed after the realtime part
void Song::removeTrack3(Track* /*track*/)//prevent of compiler warning: unused parameter
{
}

//---------------------------------------------------------
//   executeScript
//---------------------------------------------------------
void Song::executeScript(const char* scriptfile, PartList* parts, int quant, bool onlyIfSelected)
{
      // a simple format for external processing
      // will be extended if there is a need
      //
      // Semantics:
      // PARTLEN <len in ticks>
      // BEATLEN <len in ticks>
      // QUANTLEN <len in ticks>
      // NOTE <tick> <nr> <len in ticks> <velocity>
      // CONTROLLER <tick> <a> <b> <c>
      //
      MusEGlobal::song->startUndo(); // undo this entire block
      for (iPart i = parts->begin(); i != parts->end(); i++) {
            //const char* tmp = tmpnam(NULL);
            char tmp[16] = "muse-tmp-XXXXXX";
            int fd = mkstemp(tmp);
            if (MusEGlobal::debugMsg)
              printf("executeScript: script input filename=%s\n",tmp);
            FILE *fp = fdopen(fd , "w");
            MidiPart *part = (MidiPart*)(i->second);
            int partStart = part->endTick()-part->lenTick();
            int z, n;
            AL::sigmap.timesig(0, z, n);
            fprintf(fp, "TIMESIG %d %d\n", z, n);
            fprintf(fp, "PART %d %d\n", partStart, part->lenTick());
            fprintf(fp, "BEATLEN %d\n", AL::sigmap.ticksBeat(0));
            fprintf(fp, "QUANTLEN %d\n", quant);

            for (iEvent e = part->events()->begin(); e != part->events()->end(); e++) {
                Event ev = e->second;

                if (ev.isNote())
                {
                      if (onlyIfSelected && ev.selected() == false)
                            continue;

                      fprintf(fp,"NOTE %d %d %d %d\n", ev.tick(), ev.dataA(),  ev.lenTick(), ev.dataB());
                      // Indicate no undo, and do not do port controller values and clone parts.
                      MusEGlobal::audio->msgDeleteEvent(ev, part, false, false, false);
                } else if (ev.type()==Controller) {
                      fprintf(fp,"CONTROLLER %d %d %d %d\n", ev.tick(), ev.dataA(), ev.dataB(), ev.dataC());
                      // Indicate no undo, and do not do port controller values and clone parts.
                      MusEGlobal::audio->msgDeleteEvent(ev, part, false, false, false);
                }
            }
            fclose(fp);

            QStringList arguments;
            arguments << tmp;

            QProcess *myProcess = new QProcess(MusEGlobal::muse);
            myProcess->start(scriptfile, arguments);
            myProcess->waitForFinished();
            QByteArray errStr = myProcess->readAllStandardError();

            if (myProcess->exitCode()) {
              QMessageBox::warning(MusEGlobal::muse, tr("MusE - external script failed"),
                                   tr("MusE was unable to launch the script, error message:\n%1").arg(QString(errStr)));
              endUndo(SC_EVENT_REMOVED);
              return;
            }
            if (errStr.size()> 0) {
              printf("script execution produced the following error:\n%s\n", QString(errStr).toLatin1().data());
            }
            QFile file(tmp);
            if ( file.open( QIODevice::ReadOnly ) ) {
                QTextStream stream( &file );
                QString line;
                while ( !stream.atEnd() ) {
                    line = stream.readLine(); // line of text excluding '\n'
                    if (line.startsWith("NOTE"))
                    {
                        QStringList sl = line.split(" ");

                          Event e(Note);
                          int tick = sl[1].toInt();
                          int pitch = sl[2].toInt();
                          int len = sl[3].toInt();
                          int velo = sl[4].toInt();
                          e.setTick(tick);
                          e.setPitch(pitch);
                          e.setVelo(velo);
                          e.setLenTick(len);
                          // Indicate no undo, and do not do port controller values and clone parts.
                          MusEGlobal::audio->msgAddEvent(e, part, false, false, false);
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
                          // Indicate no undo, and do not do port controller values and clone parts.
                          MusEGlobal::audio->msgAddEvent(e, part, false, false, false);
                        }
                }
                file.close();
            }


      remove(tmp);
      }

      endUndo(SC_EVENT_REMOVED);
}


void Song::populateScriptMenu(QMenu* menuPlugins, QObject* receiver)
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

      QSignalMapper* distSignalMapper = new QSignalMapper(this);
      QSignalMapper* userSignalMapper = new QSignalMapper(this);

      if (deliveredScriptNames.size() > 0 || userScriptNames.size() > 0) {
            int id = 0;
            if (deliveredScriptNames.size() > 0) {
                  for (QStringList::Iterator it = deliveredScriptNames.begin(); it != deliveredScriptNames.end(); it++, id++) {
                        QAction* act = menuPlugins->addAction(*it);
                        connect(act, SIGNAL(triggered()), distSignalMapper, SLOT(map()));
                        distSignalMapper->setMapping(act, id);
                        }
                  menuPlugins->addSeparator();
                  }
            if (userScriptNames.size() > 0) {
                  for (QStringList::Iterator it = userScriptNames.begin(); it != userScriptNames.end(); it++, id++) {
                        QAction* act = menuPlugins->addAction(*it);
                        connect(act, SIGNAL(triggered()), userSignalMapper, SLOT(map()));
                        userSignalMapper->setMapping(act, id);
                        }
                  menuPlugins->addSeparator();
                  }
            connect(distSignalMapper, SIGNAL(mapped(int)), receiver, SLOT(execDeliveredScript(int)));
            connect(userSignalMapper, SIGNAL(mapped(int)), receiver, SLOT(execUserScript(int)));
            }
      return; 
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

void Song::informAboutNewParts(const std::map< Part*, std::set<Part*> >& param)
{
  emit newPartsCreated(param);
}

void Song::informAboutNewParts(Part* orig, Part* p1, Part* p2, Part* p3, Part* p4, Part* p5, Part* p6, Part* p7, Part* p8, Part* p9)
{
  std::map< Part*, std::set<Part*> > temp;
  
  temp[orig].insert(p1);
  temp[orig].insert(p2);
  temp[orig].insert(p3);
  temp[orig].insert(p4);
  temp[orig].insert(p5);
  temp[orig].insert(p6);
  temp[orig].insert(p7);
  temp[orig].insert(p8);
  temp[orig].insert(p9);
  temp[orig].erase(static_cast<Part*>(NULL));
  temp[orig].erase(orig);
  
  informAboutNewParts(temp);
}

} // namespace MusECore
