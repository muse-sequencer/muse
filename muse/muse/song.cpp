//=============================================================================
//  MusE
//  Linux Music Editor
//  $Id:$
//
//  Copyright (C) 2002-2006 by Werner Schweer and others
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License version 2.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//=============================================================================

#include "muse.h"
#include "song.h"
#include "track.h"
#include "undo.h"
#include "globals.h"
#include "event.h"
#include "midiedit/drummap.h"
#include "audio.h"
#include "mixer/mixer.h"
#include "midiseq.h"
#include "driver/audiodev.h"
#include "gconfig.h"
#include "al/marker.h"
#include "al/sig.h"
#include "al/tempo.h"
#include "midi.h"
#include "plugin.h"
#include "pipeline.h"
#include "synth.h"
#include "midiplugin.h"
#include "midirc.h"
#include "part.h"
#include "conf.h"

Song* song;

//---------------------------------------------------------
//   Song
//---------------------------------------------------------

Song::Song()
   :QObject(0)
      {
      undoList               = new UndoList;
      redoList               = new UndoList;
      _markerList            = new AL::MarkerList;
      _globalPitchShift      = 0;
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
//      delete esettingsList;
      }

//---------------------------------------------------------
//   putEvent
//---------------------------------------------------------

void Song::putEvent(const MidiEvent& event)
      {
      eventFifo.put(event);
      }

//---------------------------------------------------------
//   setTempo
//    public slot
//---------------------------------------------------------

void Song::setTempo(int newTempo)
      {
      audio->msgSetTempo(pos[0].tick(), newTempo, true);
      }

//---------------------------------------------------------
//   setSig
//    called from transport window
//---------------------------------------------------------

void Song::setSig(const AL::TimeSignature& sig)
      {
      if (_masterFlag) {
            audio->msgAddSig(pos[0].tick(), sig);
            }
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
//   selectTrack
//---------------------------------------------------------

void Song::selectTrack(Track* track)
      {
      bool changed = false;
      for (iTrack t = _tracks.begin(); t != _tracks.end(); ++t) {
            bool select = *t == track;
            if ((*t)->selected() != select) {
                  (*t)->setSelected(select);
                  changed = true;
                  }
            }
      if (changed) {
            updateSelectedTrack();
            trackSelectionChanged(_selectedTrack);
            }
      }

//---------------------------------------------------------
//   updateSelectedTrack
//    set _selectedTrack to first selected track
//---------------------------------------------------------

void Song::updateSelectedTrack()
      {
      _selectedTrack = 0;
      for (iTrack t = _tracks.begin(); t != _tracks.end(); ++t) {
            bool select = (*t)->selected();
            if (select) {
                  _selectedTrack = *t;
                  break;
                  }
            }
      }

//---------------------------------------------------------
//  addEvent
//    return true if event was added
//---------------------------------------------------------

bool Song::addEvent(Event& event, Part* part)
      {
      if (event.type() == Controller) {
            MidiTrack* track = (MidiTrack*)part->track();
            int tick  = event.tick();
            int cntrl = event.dataA();
            MidiChannel* mc = track->channel();
            CVal val;
            val.i = event.dataB();
            if (mc && !mc->addControllerVal(cntrl, tick, val)) {
                  mc->addMidiController(mc->port()->instrument(), cntrl);
                  if (!mc->addControllerVal(cntrl, tick, val))
                        return false;
                  }
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
            printf("Song::changeEvent(): EVENT not found !! %ld\n", long(part->events()->size()));
            // abort();
            return;
            }
      part->events()->erase(i);
      part->events()->add(newEvent);

      if (newEvent.type() == Controller) {
            MidiTrack* track = (MidiTrack*)part->track();
            MidiChannel* mc  = track->channel();
            if (mc) {
                  int tick  = newEvent.tick();
                  int cntrl = newEvent.dataA();
                  CVal val;
                  val.i   = newEvent.dataB();
                  mc->addControllerVal(cntrl, tick, val);
                  }
            }
      }

//---------------------------------------------------------
//   deleteEvent
//---------------------------------------------------------

void Song::deleteEvent(Event& event, Part* part)
      {
#if 0 //TODO3
      if (event.type() == Controller) {
            MidiTrack* track = (MidiTrack*)part->track();
            int ch    = track->outChannel();
            int tick  = event.tick();
            int cntrl = event.dataA();
            midiPorts[track->outPort()].deleteController(ch, tick, cntrl);
            }
#endif
      iEvent ev = part->events()->find(event);
      if (ev == part->events()->end()) {
            printf("event not found in part\n");
            return;
            }
      part->events()->erase(ev);
      }

//---------------------------------------------------------
//   findTrack
//---------------------------------------------------------

MidiTrack* Song::findTrack(const Part* part) const
      {
      for (ciMidiTrack t = _midis.begin(); t != _midis.end(); ++t) {
            MidiTrack* track = *t;
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
            loopAction->setChecked(loopFlag);
            emit loopChanged(loopFlag);
            }
      }

//---------------------------------------------------------
//   setRecord
//    set record flag
//---------------------------------------------------------

void Song::setRecord(bool f)
      {
      if (recordFlag == f)
      	return;
      if (muse->playAction->isChecked()) {
            //
            // dont allow record state changes when rolling
            //
            recordAction->setChecked(!f);
            return;
            }

      if (f) {
            bool alreadyRecEnabled = false;
            Track *selectedTrack = 0;

            // loop through list and check if any track is rec enabled
            // if not then rec enable the selected track

            WaveTrackList* wtl = waves();
            for (iWaveTrack i = wtl->begin(); i != wtl->end(); ++i) {
                  if ((*i)->recordFlag()) {
                        alreadyRecEnabled = true;
                        break;
                        }
                  if ((*i)->selected())
                        selectedTrack = (*i);
                  }
            if (!alreadyRecEnabled) {
                  MidiTrackList* mtl = midis();
                  for (iMidiTrack it = mtl->begin(); it != mtl->end(); ++it) {
                        if ((*it)->recordFlag()) {
                              alreadyRecEnabled = true;
                              break;
                              }
                        if ((*it)->selected())
                              selectedTrack = (*it);
                        }
                  }
            if (!alreadyRecEnabled && selectedTrack) {
                  // enable recording on selected track:
                  setRecordFlag(selectedTrack, true);
                  }
            else  {
                  if (!alreadyRecEnabled) {
                        // If there are no tracks, do not enable record.
                        // TODO: This forces the user to first enable record on a track
                        // which probably is a bad thing. Maybe we should warn
                        // only when the user actually starts recording by pressing
                        // play.

                        QMessageBox::critical(0, "MusE: Record",
                           "No track(s) enabled for recording");
                        f = false;
                        }
                  }
            }
      if (!f)
            bounceTrack = 0;
      recordAction->setChecked(f);
      if (f != recordFlag) {
            recordFlag = f;
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
            punchinAction->setChecked(punchinFlag);
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
            punchoutAction->setChecked(punchoutFlag);
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
      if (AL::tempomap.setMasterFlag(cpos(), val)) {
            emit songChanged(SC_MASTER);
            emit tempoChanged();
            }
      }

//---------------------------------------------------------
//   setPlay
//    set transport play flag
//---------------------------------------------------------

void Song::setPlay(bool f)
      {
      // only allow the user to set the button "on"
      if (!f) {
            printf("  setPlay checked\n");
            muse->playAction->setChecked(true);
            }
      else {
            if (recordAction->isChecked()) {
                  startUndo();
                  MidiTrackList* ml = midis();
                  for (iMidiTrack it = ml->begin(); it != ml->end(); ++it) {
                        if ((*it)->recordFlag())
                              (*it)->startRecording();
                        }
                  WaveTrackList* wl = waves();
                  for (iWaveTrack wt = wl->begin(); wt != wl->end(); ++wt) {
                        if ((*wt)->recordFlag())
                              (*wt)->startRecording();
                        }
                  OutputList* ol = outputs();
                  for (iAudioOutput o = ol->begin(); o != ol->end(); ++o) {
                        if ((*o)->recordFlag())
                              (*o)->startRecording();
                        }
                  }
            audio->msgPlay(true);
            }
      }

//---------------------------------------------------------
//   setStop
//---------------------------------------------------------

void Song::setStop(bool f)
      {
      // only allow the user to set the button "on"
      if (!f)
            muse->stopAction->setChecked(true);
      else
            audio->msgPlay(false);
      }

//---------------------------------------------------------
//   setStopPlay
//---------------------------------------------------------

void Song::setStopPlay(bool f)
      {
      emit playChanged(f);   // signal transport window
      muse->playAction->setChecked(f);
      muse->stopAction->setChecked(!f);
      }

//---------------------------------------------------------
//   swapTracks
//---------------------------------------------------------

void Song::swapTracks(int i1, int i2)
      {
      undoOp(UndoOp::SwapTrack, i1, i2);
      Track* track = _tracks[i1];
      _tracks[i1]  = _tracks[i2];
      _tracks[i2]  = track;
      }

//---------------------------------------------------------
//   setTickPos
//---------------------------------------------------------
/*
void Song::setTickPos(int idx, unsigned int tick)
      {
      Pos pos(tick);
      setPos(idx, pos);
      }
*/
//---------------------------------------------------------
//   setPos
//   song->setPos(Song::CPOS, pos, true, true, true);
//---------------------------------------------------------

void Song::setPos(int idx, const AL::Pos& val)
      {
      setPos(idx, val, true, true, false);
      }

void Song::setPos(int idx, const Pos& val, bool sig, bool isSeek, bool follow)
      {
//      printf("setPos %d sig=%d,seek=%d,scroll=%d\n",
//         idx, sig, isSeek, follow);
//      val.dump(0);
//      printf("\n");

      if (pos[idx] == val)
            return;
      if (idx == CPOS) {
            _vcpos = val;
            if (isSeek) {
                  seekInProgress = true;
                  audio->msgSeek(val);
                  return;
                  }
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
                  emit posChanged(LPOS, pos[LPOS], follow);
                  emit posChanged(RPOS, pos[RPOS], follow);
                  if (idx != LPOS && idx != RPOS)
                        emit posChanged(idx, pos[idx], follow);
                  }
            else
                  emit posChanged(idx, pos[idx], follow);
            }

      if (idx == CPOS)
            updateCurrentMarker();
      }

//---------------------------------------------------------
//   updateCurrentMarker
//---------------------------------------------------------

void Song::updateCurrentMarker()
      {
      AL::iMarker i1 = _markerList->begin();
      AL::iMarker i2 = i1;
      bool currentChanged = false;
      Pos& val = pos[CPOS];
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

//---------------------------------------------------------
//   forward
//    roll forward config.division ticks
//---------------------------------------------------------

void Song::forward()
      {
      unsigned newPos = pos[0].tick() + config.division;
      audio->msgSeek(Pos(newPos, AL::TICKS));
      }

//---------------------------------------------------------
//   rewind
//    roll back config.division ticks
//---------------------------------------------------------

void Song::rewind()
      {
      unsigned newPos;
      if (unsigned(config.division) > pos[0].tick())
            newPos = 0;
      else
            newPos = pos[0].tick() - config.division;
      audio->msgSeek(Pos(newPos, AL::TICKS));
      }

//---------------------------------------------------------
//   rewindStart
//---------------------------------------------------------

void Song::rewindStart()
      {
      audio->msgSeek(Pos(0, AL::TICKS));
      }

//---------------------------------------------------------
//   update
//---------------------------------------------------------

void Song::update(int flags)
      {
      if (flags == 0)
            return;
      if (flags & SC_ROUTE) {
            //
            // remove unconnected channels
            //
            bool again;
            do {
                  again = false;
                  for (iTrack i = _tracks.begin(); i != _tracks.end(); ++i) {
                        if ((*i)->type() != Track::MIDI_CHANNEL)
                              continue;
                        MidiChannel* mc = (MidiChannel*)*i;
                        if (mc->noInRoute()) {
                              emit trackRemoved(mc);
                              _tracks.erase(i);
                              again = true;
                              break;
                              }
                        }
                  } while(again);
            //
            // add new connected channels
            //
            for (iMidiChannel i = _midiChannel.begin(); i != _midiChannel.end(); ++i) {
                  MidiChannel* mc = (MidiChannel*)*i;
                  if (mc->noInRoute() || trackExists(mc))
                        continue;
                  _tracks.push_back(mc);
                  emit trackAdded(mc, _tracks.size()-1);
                  }
            }
      emit songChanged(flags);
      if (flags & SC_TEMPO)
            emit tempoChanged();
      }

//---------------------------------------------------------
//   updatePos
//---------------------------------------------------------

void Song::updatePos()
      {
      emit posChanged(0, pos[0], false);
      emit posChanged(1, pos[1], false);
      emit posChanged(2, pos[2], false);
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
      AL::tempomap.dump();
      AL::sigmap.dump();
      }

//---------------------------------------------------------
//   getSelectedParts
//---------------------------------------------------------

PartList* Song::getSelectedMidiParts() const
      {
      PartList* parts = new PartList();

      //------------------------------------------------------
      //    wenn ein Part selektiert ist, diesen editieren
      //    wenn ein Track selektiert ist, den Ersten
      //       Part des Tracks editieren, die restlichen sind
      //       'ghostparts'
      //    wenn mehrere Parts selektiert sind, dann Ersten
      //       editieren, die restlichen sind 'ghostparts'
      //

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
            for (ciMidiTrack t = _midis.begin(); t != _midis.end(); ++t) {
                  if ((*t)->selected()) {
                        MidiTrack* track = *t;
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

      //------------------------------------------------------
      //    wenn ein Part selektiert ist, diesen editieren
      //    wenn ein Track selektiert ist, den Ersten
      //       Part des Tracks editieren, die restlichen sind
      //       'ghostparts'
      //    wenn mehrere Parts selektiert sind, dann Ersten
      //       editieren, die restlichen sind 'ghostparts'
      //

      // markierte Parts sammeln
      for (ciWaveTrack t = _waves.begin(); t != _waves.end(); ++t) {
            WaveTrack* track = *t;
            PartList* pl = track->parts();
            for (iPart p = pl->begin(); p != pl->end(); ++p) {
                  if (p->second->selected()) {
                        parts->add(p->second);
                        }
                  }
            }
      // wenn keine Parts selektiert, dann markierten Track suchen
      // und alle Parts dieses Tracks zusammensuchen

      if (parts->empty()) {
            for (ciWaveTrack t = _waves.begin(); t != _waves.end(); ++t) {
                  if ((*t)->selected()) {
                        WaveTrack* track =  *t;
                        PartList* pl = track->parts();
                        for (iPart p = pl->begin(); p != pl->end(); ++p)
                              parts->add(p->second);
                        break;
                        }
                  }
            }
      return parts;
      }

//---------------------------------------------------------
//   beat
//    update gui
//---------------------------------------------------------

void Song::beat()
      {
      updateFlags = 0;
      if (audio->isPlaying()) {
            int tick = audio->curTickPos();
            setPos(0, tick, true, false, true);
            }
      if (audio->isRecording()) {
            MidiTrackList* ml = midis();
            for (iMidiTrack it = ml->begin(); it != ml->end(); ++it) {
                  MidiTrack* mt = *it;
                  if (mt->recordFlag())
                        mt->recordBeat();
                  }
            WaveTrackList* wl = waves();
            for (iWaveTrack wt = wl->begin(); wt != wl->end(); ++wt) {
                  WaveTrack* mt = *wt;
                  if (mt->recordFlag())
                        mt->recordBeat();
                  }
            }
      while (!eventFifo.isEmpty()) {
           MidiEvent event(eventFifo.get());
            if (rcEnable)
                  midiRCList.doAction(event);
            emit midiEvent(event);
            }
      //
      //  update controller guis
      //
      TrackList* tl = tracks();
      for (iTrack it = tl->begin(); it != tl->end(); ++it) {
            Track* track = *it;
            if (!track->autoRead())
                  continue;
            track->updateController();
            }
      update(updateFlags);
      }

//---------------------------------------------------------
//   setLen
//---------------------------------------------------------

void Song::setLen(int l)
      {
      _len = roundUpBar(l);
      AL::Pos pos(_len);
      int bar, beat;
      unsigned tick;
      AL::sigmap.tickValues(_len, &bar, &beat, &tick);
      emit measureLenChanged(bar);
      emit lenChanged(pos);
      }

//---------------------------------------------------------
//   setMeasureLen
//---------------------------------------------------------

void Song::setMeasureLen(int b)
      {
      setLen(AL::sigmap.bar2tick(b, 0, 0));
      }

//---------------------------------------------------------
//   addMarker
//---------------------------------------------------------

AL::Marker* Song::addMarker(const QString& s, const AL::Pos& pos)
      {
      AL::Marker* marker = _markerList->add(s, pos);
      updateCurrentMarker();
      emit markerChanged(MARKER_ADD);
      return marker;
      }

//---------------------------------------------------------
//   removeMarker
//---------------------------------------------------------

void Song::removeMarker(AL::Marker* marker)
      {
      _markerList->remove(marker);
      updateCurrentMarker();
      emit markerChanged(MARKER_REMOVE);
      }

//---------------------------------------------------------
//   setMarkerName
//---------------------------------------------------------

AL::Marker* Song::setMarkerName(AL::Marker* m, const QString& s)
      {
      m->setName(s);
      emit markerChanged(MARKER_NAME);
      return m;
      }

AL::Marker* Song::setMarkerTick(AL::Marker* m, int t)
      {
      AL::Marker mm(*m);
      _markerList->remove(m);
      mm.setTick(t);
      m = _markerList->add(mm);
      updateCurrentMarker();
      emit markerChanged(MARKER_TICK);
      return m;
      }

AL::Marker* Song::setMarkerLock(AL::Marker* m, bool f)
      {
      m->setType(f ? AL::FRAMES : AL::TICKS);
      updateCurrentMarker();
      emit markerChanged(MARKER_LOCK);
      return m;
      }

//---------------------------------------------------------
//   endMsgCmd
//---------------------------------------------------------

void Song::endMsgCmd()
      {
      redoList->clear();            // TODO: delete elements in list
      undoAction->setEnabled(true);
      redoAction->setEnabled(false);
      update(updateFlags);
      }

//---------------------------------------------------------
//   undo
//---------------------------------------------------------

void Song::undo()
      {
      updateFlags = 0;
      if (doUndo1())
            return;
      audio->msgUndo();
      doUndo3();
      redoAction->setEnabled(true);
      undoAction->setEnabled(!undoList->empty());
      if (updateFlags) {
            emit songChanged(updateFlags);
            if (updateFlags & SC_TEMPO)
                  emit tempoChanged();
            }
      }

//---------------------------------------------------------
//   redo
//---------------------------------------------------------

void Song::redo()
      {
      updateFlags = 0;
      if (doRedo1())
            return;
      audio->msgRedo();
      doRedo3();
      undoAction->setEnabled(true);
      redoAction->setEnabled(!redoList->empty());
      if (updateFlags) {
            emit songChanged(updateFlags);
            if (updateFlags & SC_TEMPO)
                  emit tempoChanged();
            }
      }

//---------------------------------------------------------
//   processMsg
//    executed in realtime thread context
//---------------------------------------------------------

void Song::processMsg(AudioMsg* msg)
      {
      switch(msg->id) {
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
                  if (addEvent(msg->ev1, (Part*)(msg->p2))) {
                        Event ev;
                        undoOp(UndoOp::AddEvent, ev, msg->ev1, (Part*)msg->p2);
                        }
                  else
                        updateFlags = 0;
                  break;
            case SEQM_REMOVE_EVENT:
                  {
                  Event event = msg->ev1;
                  Part* part = (Part*)(msg->p2);
                  Event e;
                  undoOp(UndoOp::DeleteEvent, e, event, part);
                  deleteEvent(event, part);
                  updateFlags = SC_EVENT_REMOVED;
                  }
                  break;
            case SEQM_CHANGE_EVENT:
                  changeEvent(msg->ev1, msg->ev2, (Part*)(msg->p3));
                  undoOp(UndoOp::ModifyEvent, msg->ev2, msg->ev1, (Part*)msg->p3);
                  updateFlags = SC_EVENT_MODIFIED;
                  break;

            case SEQM_ADD_TEMPO:
                  undoOp(UndoOp::AddTempo, msg->a, msg->b);
                  AL::tempomap.addTempo(msg->a, msg->b);
                  updateFlags = SC_TEMPO;
                  break;

            case SEQM_SET_TEMPO:
                  undoOp(UndoOp::AddTempo, msg->a, msg->b);
                  AL::tempomap.setTempo(msg->a, msg->b);
                  updateFlags = SC_TEMPO;
                  break;

            case SEQM_SET_GLOBAL_TEMPO:
                  AL::tempomap.setGlobalTempo(msg->a);
                  break;

            case SEQM_REMOVE_TEMPO:
                  undoOp(UndoOp::DeleteTempo, msg->a, msg->b);
                  AL::tempomap.delTempo(msg->a);
                  updateFlags = SC_TEMPO;
                  break;

            case SEQM_ADD_SIG:
                  undoOp(UndoOp::AddSig, msg->a, msg->b, msg->c);
                  AL::sigmap.add(msg->a, AL::TimeSignature(msg->b, msg->c));
                  updateFlags = SC_SIG;
                  break;

            case SEQM_REMOVE_SIG:
                  //printf("processMsg (SEQM_REMOVE_SIG) UndoOp::DeleteSig. Deleting AL::sigmap at: %d with z=%d n=%d\n", msg->a, msg->b, msg->c);
                  undoOp(UndoOp::DeleteSig, msg->a, msg->b, msg->c);
                  AL::sigmap.del(msg->a);
                  updateFlags = SC_SIG;
                  break;

            case SEQM_ADD_CTRL:
                  msg->track->addControllerVal(msg->a, msg->time, msg->cval1);
                  break;

            case SEQM_REMOVE_CTRL:
                  msg->track->removeControllerVal(msg->a, msg->time);
                  break;

            default:
                  printf("unknown seq message %d\n", msg->id);
                  break;
            }
      }

//---------------------------------------------------------
//   cmdAddPart
//	realtime context
//---------------------------------------------------------

void Song::cmdAddPart(Part* part)
      {
      part->track()->addPart(part);
      undoOp(UndoOp::AddPart, part);
      updateFlags = SC_PART_INSERTED;
      if (len() < part->endTick())
            setLen(part->endTick());
      }

//---------------------------------------------------------
//   cmdRemovePart
//---------------------------------------------------------

void Song::cmdRemovePart(Part* part)
      {
      Track* track = part->track();
      track->parts()->remove(part);

//TD      esettingsList->removeSettings(part->sn());
      undoOp(UndoOp::DeletePart, part);
      part->events()->incARef(-1);
      updateFlags = SC_PART_MODIFIED;
      }

//---------------------------------------------------------
//   changePart
//---------------------------------------------------------

void Song::changePart(Part* oldPart, Part* newPart)
      {
      Part part = *newPart;
      *newPart  = *oldPart;
      *oldPart  = part;
      }

//---------------------------------------------------------
//   cmdChangePart
//	realtime context
//---------------------------------------------------------

void Song::cmdChangePart(Part* oldPart, Part* newPart)
      {
      changePart(oldPart, newPart);
      undoOp(UndoOp::ModifyPart, oldPart, newPart);
      oldPart->events()->incARef(-1);
      updateFlags = SC_PART_MODIFIED;
      }

//---------------------------------------------------------
//   panic
//---------------------------------------------------------

void Song::panic()
      {
      audio->msgPanic();
      }

//---------------------------------------------------------
//   clear
//    signal - emit signals for changes if true
//    called from constructor as clear(false) and
//    from MusE::clearSong() as clear(false)
//---------------------------------------------------------

void Song::clear(bool signal)
      {
      _created       = false;
      _backupWritten = false;
      dirty          = false;
      _comment       = "";
      _createDate    = QDateTime::currentDateTime();

      seekInProgress = false;
      bounceTrack    = 0;

//      for (iTrack i = _tracks.begin(); i != _tracks.end(); ++i)
//            (*i)->deactivate();
      _selectedTrack = 0;
      _tracks.clear();
      _midis.clearDelete();
      _waves.clearDelete();
      _inputs.clearDelete();     // audio input ports
      _outputs.clearDelete();    // audio output ports
      _groups.clearDelete();     // mixer groups
      _synthIs.clearDelete();

      _midiSyntis.clearDelete();
      _midiOutPorts.clearDelete();
      _midiInPorts.clearDelete();
      _midiChannel.clear();

      AL::tempomap.clear();
      AL::sigmap.clear();
      undoList->clear();
      redoList->clear();
      _markerList->clear();
      pos[0].setTick(0);
      pos[1].setTick(0);
      pos[2].setTick(0);
      _vcpos.setTick(0);

      _masterFlag    = true;
      loopFlag       = false;
      loopFlag       = false;
      punchinFlag    = false;
      punchoutFlag   = false;
      recordFlag     = false;
      soloFlag       = false;
      // seq
      _recMode       = REC_OVERDUP;
      _cycleMode     = CYCLE_NORMAL;
      _click         = false;
      _quantize      = false;
      _len           = 1;           // song len in ticks
      // _tempo      = 500000;      // default tempo 120
      if (signal) {
            emit loopChanged(false);
            recordChanged(false);
            }
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
// printf("seqSignal to gui:<%c>\n", buffer[i]);
            switch(buffer[i]) {
                  case MSG_STOP:
                        stopRolling();
                        break;
                  case MSG_PLAY:
                        setStopPlay(true);
                        break;
                  case MSG_RECORD:
                        setRecord(true);
                        break;
                  case MSG_SEEK:
	                  setPos(0, audio->curTickPos(), true, false, !seekInProgress);
                  	seekInProgress = false;
                        beat();           // update controller guis
                        break;
                  case MSG_JACK_SHUTDOWN:
                       {
                        muse->seqStop();
                        // give the user a sensible explanation
                        int btn = QMessageBox::critical( muse, tr("Jack shutdown!"),
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
                              muse->seqRestart();
                              }
                        }
                        break;
                  case MSG_START_BOUNCE:
                        {
                        bool useFreewheel = config.useJackFreewheelMode;
                        if (useFreewheel) {
                              // check:
                              //    we cannot use freewheel, if there are active audio input
                              //    strips

                              for (iAudioInput ii = _inputs.begin(); ii != _inputs.end(); ++i) {
                                    AudioInput* ai = *ii;
                                    if (!(ai->mute() || ai->off())) {
                                          useFreewheel = false;
                                          break;
                                          }
                                    }
                              if (useFreewheel)
                                    audioDriver->setFreewheel(true);
                              }
                        }
                        break;

                  case MSG_STOP_BOUNCE:
                        if (audio->freewheel())
                              audioDriver->setFreewheel(false);
                        audio->msgPlay(false);
                        break;

                  case MSG_GRAPH_CHANGED:
                        audioDriver->graphChanged();
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
      Part* part = 0;
      iPart ip;
      for (ip = pl->begin(); ip != pl->end(); ++ip) {
            part = ip->second;
            unsigned partStart = part->tick();
            unsigned partEnd   = partStart + part->lenTick();
            if (tick >= partStart && tick < partEnd)
                  break;
            }
      updateFlags |= SC_EVENT_INSERTED;
      if (ip == pl->end()) {
            // create new part
            part          = new Part(mt);
            int startTick = roundDownBar(tick);
            int endTick   = roundUpBar(tick);
            part->setTick(startTick);
            part->setLenTick(endTick - startTick);
            part->setName(mt->name());
            event.move(-startTick);
            part->events()->add(event);
            audio->msgAddPart(part);
            return;
            }
      part = ip->second;
      tick -= part->tick();
      event.setTick(tick);
      audio->msgAddEvent(event, part);
      }

//---------------------------------------------------------
//   stopRolling
//---------------------------------------------------------

void Song::stopRolling()
      {
      setStopPlay(false);
      if (record()) {
	      audio->msgIdle(true); // gain access to all data structures

	      WaveTrackList* wl = waves();
      	for (iWaveTrack it = wl->begin(); it != wl->end(); ++it) {
            	WaveTrack* track = *it;
	            if (track->recordFlag() || bounceTrack == track) {
      	            track->stopRecording(audio->startRecordPos, audio->endRecordPos);
            	      }
	            }
      	MidiTrackList* ml = midis();
	      for (iMidiTrack it = ml->begin(); it != ml->end(); ++it) {
      	      if ((*it)->recordFlag())
            	      (*it)->stopRecording();
	            }
      	OutputList* ol = outputs();
	      for (iAudioOutput io = ol->begin(); io != ol->end(); ++io) {
      	      AudioOutput* ao = *io;
            	if (ao->recordFlag())
	                  ao->stopRecording(audio->startRecordPos, audio->endRecordPos);
      	      }
     	 	audio->msgIdle(false);
            updateFlags |= SC_PART_MODIFIED;
	      endUndo(updateFlags);
      	setRecord(false);
            }
      //
      // process recorded automation events
      //
      for (iTrack it = tracks()->begin(); it != tracks()->end(); ++it) {
            Track* track = *it;
            if (!track->autoWrite())
                  continue;
            CtrlRecList* crl = track->recEvents();
            CtrlList* cl = track->controller();
            for (iCtrl icl = cl->begin(); icl != cl->end(); ++icl) {
                  Ctrl* cl = icl->second;
                  int id = cl->id();
                  //
                  //  remove old events from record region
                  //
                  bool hasEvents = false;
//                        int start = audio->getStartRecordPos().frame();
//                        int end   = audio->getEndRecordPos().frame();
//                        iCtrlVal   s = cl->lower_bound(start);
//                        iCtrlVal   e = cl->lower_bound(end);
//                        cl->erase(s, e);
//                        }

                  for (iCtrlRec icr = crl->begin(); icr != crl->end(); ++icr) {
                        if (icr->id == id && icr->type == 1) {
                              int start = icr->time;
                              ++icr;
                              for (; icr != crl->end(); ++icr) {
                                    if (icr->id == id && icr->type == 2) {
                                          int end = icr->time;
                                          if (track->timeType() == AL::TICKS) {
                                                start = AL::tempomap.frame2tick(start);
                                                end = AL::tempomap.frame2tick(end);
                                                }
                                          iCtrlVal s = cl->lower_bound(start);
                                          iCtrlVal e = cl->lower_bound(end);
                                          cl->erase(s, e);
                                          hasEvents = true;
                                          break;
                                          }
                                    }
                              if (icr == crl->end())
                                    break;
                              }
                        }
                  //
                  //  extract all recorded events for controller "id"
                  //  from CtrlRecList and put into cl
                  //
                  for (iCtrlRec icr = crl->begin(); icr != crl->end(); ++icr) {
                        if (icr->id == id && icr->type == 0)
                              cl->add(icr->time, icr->val);
                        }
                  track->emitControllerChanged(id);
                  }
            crl->clear();
            track->setAutoWrite(false);
            }
      }

//---------------------------------------------------------
//    addTrack
//    called from GUI context
//---------------------------------------------------------

Track* Song::addTrack(QAction* action)
      {
      int t = action->data().toInt();

      deselectTracks();
      Track* track = 0;

      Track::TrackType type = (Track::TrackType) t;
      if (t >= 5000) {
            int idx = t - 5000;
            type = Track::MIDI_SYNTI;
            int k = 0;
            iMidiPlugin i;
            for (i = midiPlugins.begin(); i != midiPlugins.end(); ++i) {
                  if ((*i)->type() != MEMPI_GENERATOR)
                        continue;
                  if (k == idx)
                        break;
                  ++k;
                  }
            if (i == midiPlugins.end()) {
                  fprintf(stderr, "Song::addTrack: midi synti not found\n");
                  return 0;
                  }
            MidiPlugin* s = *i;
            MidiSynti* si = new MidiSynti();
            QString sName(s->name());
            for (k = s->instances(); k < 1000; ++k) {
                  QString instanceName = ("%1-%2");
                  instanceName = instanceName.arg(sName).arg(k);

                  MidiSyntiList* sl = midiSyntis();
                  iMidiSynti sii;
                  for (sii = sl->begin(); sii != sl->end(); ++sii) {
                        if ((*sii)->name() == instanceName)
                              break;
                        }
                  if (sii == sl->end()) {
                        si->setName(instanceName);
                        break;
                        }
                  }
            if (si->initInstance(s)) {
                  delete si;
                  return 0;
                  }
            track = si;
            }
      else if (t >= 1000) {
            type = Track::AUDIO_SOFTSYNTH;
            QString sclass = synthis[t-1000]->name();

            Synth* s = findSynth(sclass);
            if (s == 0) {
                  fprintf(stderr, "synthi class <%s> not found\n", sclass.toLatin1().data());
                  return 0;
                  }

            SynthI* si = new SynthI();
            int i;
            for (i = s->instances(); i < 1000; ++i) {
                  QString instanceName = ("%1-%2");
                  instanceName = instanceName.arg(s->name()).arg(i);

                  SynthIList* sl = syntis();
                  iSynthI sii;
                  for (sii = sl->begin(); sii != sl->end(); ++sii) {
                        if ((*sii)->name() == instanceName)
                              break;
                        }
                  if (sii == sl->end()) {
                        si->setName(instanceName);
printf("set instance name <%s>\n", instanceName.toLatin1().data());
                        break;
                        }
                  }
            if (si->initInstance(s)) {
                  delete si;
                  return 0;
                  }
            track = si;
            }
      else {
            switch (type) {
                  case Track::MIDI:
                        track = new MidiTrack();
                        track->setType(Track::MIDI);
                        break;
                  case Track::MIDI_OUT:
                        track = new MidiOutPort();
                        break;
                  case Track::MIDI_IN:
                        track = new MidiInPort();
                        break;
                  case Track::WAVE:
                        track = new WaveTrack();
                        break;
                  case Track::AUDIO_OUTPUT:
                        track = new AudioOutput();
                        break;
                  case Track::AUDIO_GROUP:
                        track = new AudioGroup();
                        break;
                  case Track::AUDIO_INPUT:
                        track = new AudioInput();
                        break;
                  case Track::AUDIO_SOFTSYNTH:
                  case Track::TRACK_TYPES:
                  default:
                        printf("Song::addTrack() illegal type %d\n", type);
                        abort();
                  }
            if (track == 0)
                  return 0;
            }
      track->setDefaultName();
      insertTrack(track, -1);
      return track;
      }

//---------------------------------------------------------
//   insertTrack
//---------------------------------------------------------

void Song::insertTrack(Track* track, int idx)
      {
      //
      //  add default routes
      //
      OutputList* ol = outputs();
      AudioOutput* ao = 0;
      if (!ol->empty())
            ao = ol->front();
      MidiOutPortList* mol = midiOutPorts();
      MidiOutPort* mo = 0;
      if (!mol->empty())
            mo = mol->front();

      switch (track->type()) {
            case Track::TRACK_TYPES:
            case Track::MIDI_OUT:
            case Track::MIDI_IN:
            case Track::MIDI_CHANNEL:
            case Track::MIDI_SYNTI:
                  break;
            case Track::MIDI:
                  //
                  // connect output to first non used midi channel
                  // if there is already a route, do not add another
                  // default routing
                  //
                  if (track->noOutRoute()) {
                        for (iMidiChannel i = _midiChannel.begin(); i != _midiChannel.end(); ++i) {
                              if ((*i)->noInRoute()) {
                                    track->outRoutes()->push_back(Route(*i, -1, Route::TRACK));
                                    break;
                                    }
                              }
                        }
                  //
                  // connect to all midi inputs, if there is not already
                  // a route
                  //
                  if (!track->noInRoute()) {
                        MidiInPortList* mi = midiInPorts();
                        for (iMidiInPort i = mi->begin(); i != mi->end(); ++i) {
                              for (int ch = 0; ch < MIDI_CHANNELS; ++ch)
                                    track->inRoutes()->push_back(Route(*i, ch, Route::TRACK));
                              }
                        }
                  break;
            case Track::AUDIO_SOFTSYNTH:
                  if (mo)
                        track->inRoutes()->push_back(Route(mo, -1, Route::TRACK));
                  // fall through

            case Track::WAVE:
            case Track::AUDIO_GROUP:
            case Track::AUDIO_INPUT:
                  {
                  // connect first input channel to first available jack output
                  // etc.
                  std::list<PortName>* op = audioDriver->outputPorts();
                  std::list<PortName>::iterator is = op->begin();
                  for (int ch = 0; ch < track->channels(); ++ch) {
                        if (is != op->end()) {
                              track->inRoutes()->push_back(Route(is->name, ch, Route::AUDIOPORT));
                              ++is;
                              }
                        }
                  delete op;
                  if (ao)
                        track->outRoutes()->push_back(Route(ao, -1, Route::TRACK));
                  }
                  break;
            case Track::AUDIO_OUTPUT:
                  {
                  std::list<PortName>* op = audioDriver->inputPorts();
                  std::list<PortName>::iterator is = op->begin();
                  for (int ch = 0; ch < track->channels(); ++ch) {
                        if (is != op->end()) {
                              track->outRoutes()->push_back(Route(is->name, ch, Route::AUDIOPORT));
                              ++is;
                              }
                        }
                  }
                  break;
            }
      insertTrack1(track, idx);

      startUndo();
      undoOp(UndoOp::AddTrack, idx, track);
      AudioMsg msg;
      msg.id    = SEQM_ADD_TRACK;
      msg.track = track;
      msg.ival  = idx;
      audio->sendMsg(&msg);
      endUndo(SC_TRACK_INSERTED | SC_ROUTE);

      emit trackAdded(track, idx);
      selectTrack(track);
      }

//---------------------------------------------------------
//   insertTrack0
//    can only be called if sequencer is idle
//    (during song load)
//---------------------------------------------------------

void Song::insertTrack0(Track* track, int idx)
      {
      insertTrack1(track, idx);
      insertTrack2(track);
      }

//---------------------------------------------------------
//   insertTrack1
//    non realtime part of insertTrack
//---------------------------------------------------------

void Song::insertTrack1(Track* track, int idx)
      {
      iTrack i = _tracks.index2iterator(idx);
      _tracks.insert(i, track);
      switch(track->type()) {
            case Track::AUDIO_SOFTSYNTH:
                  {
                  SynthI* s = (SynthI*)track;
                  Synth* sy = s->synth();
                  if (!s->isActivated()) {
                        s->initInstance(sy);
                        }
                  }
                  break;
            default:
                  break;
            }
      if (audioState == AUDIO_RUNNING) {
            track->activate1();
            track->activate2();
            }
      }

//---------------------------------------------------------
//   insertTrack2
//    realtime part
//---------------------------------------------------------

void Song::insertTrack2(Track* track)
      {
      switch(track->type()) {
            case Track::MIDI_SYNTI:
                  _midiSyntis.push_back((MidiSynti*)track);
                  break;
            case Track::MIDI:
                  _midis.push_back((MidiTrack*)track);
                  break;
            case Track::MIDI_OUT:
                  _midiOutPorts.push_back((MidiOutPort*)track);
                  for (int i = 0; i < MIDI_CHANNELS; ++i) {
                        MidiChannel* mc = ((MidiOutPort*)track)->channel(i);
                        _midiChannel.push_back(mc);
                        }
                  break;
            case Track::MIDI_IN:
                  _midiInPorts.push_back((MidiInPort*)track);
                  break;
            case Track::MIDI_CHANNEL:
                  _midiChannel.push_back((MidiChannel*)track);
                  break;
            case Track::WAVE:
                  _waves.push_back((WaveTrack*)track);
                  break;
            case Track::AUDIO_OUTPUT:
                  _outputs.push_back((AudioOutput*)track);
                  break;
            case Track::AUDIO_GROUP:
                  _groups.push_back((AudioGroup*)track);
                  break;
            case Track::AUDIO_INPUT:
                  _inputs.push_back((AudioInput*)track);
                  break;
            case Track::AUDIO_SOFTSYNTH:
                  {
                  SynthI* s = (SynthI*)track;
                  midiInstruments.push_back(s);
                  _synthIs.push_back(s);
                  }
                  break;
            default:
                  fprintf(stderr, "insertTrack2: unknown track type %d\n", track->type());
                  // abort();
                  return;
            }

      //
      //  connect routes
      //

      Route src(track, -1, Route::TRACK);
      if (track->type() == Track::AUDIO_SOFTSYNTH)
            src.type = Route::SYNTIPORT;
      if (track->type() == Track::AUDIO_OUTPUT || track->type() == Track::MIDI_OUT) {
            const RouteList* rl = track->inRoutes();
            for (ciRoute r = rl->begin(); r != rl->end(); ++r) {
                  src.channel = r->channel;
                  r->track->outRoutes()->push_back(src);
                  }
            }
      else if (track->type() == Track::AUDIO_INPUT || track->type() == Track::MIDI_IN) {
            const RouteList* rl = track->outRoutes();
            for (ciRoute r = rl->begin(); r != rl->end(); ++r) {
                  src.channel = r->channel;
                  r->track->inRoutes()->push_back(src);
                  }
            }
      else {
            const RouteList* rl = track->inRoutes();
            for (ciRoute r = rl->begin(); r != rl->end(); ++r) {
                  src.channel = r->channel;
                  r->track->outRoutes()->push_back(src);
                  }
            rl = track->outRoutes();
            for (ciRoute r = rl->begin(); r != rl->end(); ++r) {
                  src.channel = r->channel;
                  r->track->inRoutes()->push_back(src);
                  }
            }
      }

//---------------------------------------------------------
//   removeTrack
//    called from gui context
//---------------------------------------------------------

void Song::removeTrack(Track* track)
      {
      startUndo();
      int idx = _tracks.index(track);
      undoOp(UndoOp::DeleteTrack, idx, track);
      removeTrack1(track);
      audio->msgRemoveTrack(track);
      removeTrack3(track);
      endUndo(SC_TRACK_REMOVED | SC_ROUTE);
      }

//---------------------------------------------------------
//   removeTrack1
//    non realtime part of removeTrack
//---------------------------------------------------------

void Song::removeTrack1(Track* track)
      {
      if (track->type() != Track::MIDI_OUT && track->type() != Track::MIDI_IN)
            track->deactivate();
      _tracks.erase(track);
      }

//---------------------------------------------------------
//   removeTrack2
//    called from RT context
//---------------------------------------------------------

void Song::removeTrack2(Track* track)
      {
      switch (track->type()) {
            case Track::MIDI_SYNTI:
                  _midiSyntis.erase(track);
                  break;
            case Track::MIDI:
                  _midis.erase(track);
                  break;
            case Track::MIDI_OUT:
                  track->deactivate();
                  _midiOutPorts.erase(track);
                  break;
            case Track::MIDI_IN:
                  track->deactivate();
                  _midiInPorts.erase(track);
                  break;
            case Track::MIDI_CHANNEL:
                  _midiChannel.erase(track);
                  break;
            case Track::WAVE:
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
            case Track::AUDIO_SOFTSYNTH:
                  {
                  SynthI* s = (SynthI*) track;
                  s->deactivate2();
                  _synthIs.erase(track);
                  }
                  break;
            case Track::TRACK_TYPES:
                  return;
            }
      //
      //  remove routes
      //
      Route src(track, -1, Route::TRACK);
      if (track->type() == Track::AUDIO_SOFTSYNTH)
            src.type = Route::SYNTIPORT;
      if (track->type() == Track::AUDIO_OUTPUT || track->type() == Track::MIDI_OUT) {
            const RouteList* rl = track->inRoutes();
            for (ciRoute r = rl->begin(); r != rl->end(); ++r) {
                  src.channel = r->channel;
                  r->track->outRoutes()->removeRoute(src);
                  }
            }
      else if (track->type() == Track::AUDIO_INPUT || track->type() == Track::MIDI_IN) {
            const RouteList* rl = track->outRoutes();
            for (ciRoute r = rl->begin(); r != rl->end(); ++r) {
                  src.channel = r->channel;
                  r->track->inRoutes()->removeRoute(src);
                  }
            }
      else {
            const RouteList* rl = track->inRoutes();
            for (ciRoute r = rl->begin(); r != rl->end(); ++r) {
printf("remove route:\n");
r->dump();

                  src.channel = r->channel;
                  r->track->outRoutes()->removeRoute(src);
                  }
            rl = track->outRoutes();
            for (ciRoute r = rl->begin(); r != rl->end(); ++r) {
                  src.channel = r->channel;
                  r->track->inRoutes()->removeRoute(src);
                  }
            }
      }

//---------------------------------------------------------
//   removeTrack3
//    non realtime part of removeTrack
//---------------------------------------------------------

void Song::removeTrack3(Track* track)
      {
      if (track->type() == Track::AUDIO_SOFTSYNTH) {
            SynthI* s = (SynthI*) track;
            s->deactivate3();
            }
      emit trackRemoved(track);
      }

//---------------------------------------------------------
//   synthesizer
//---------------------------------------------------------

std::vector<QString>* Song::synthesizer() const
      {
      std::vector<QString>* l = new std::vector<QString>;

      for (std::vector<Synth*>::const_iterator i = synthis.begin();
         i != synthis.end(); ++i) {
            l->push_back((*i)->name());
            }
      return l;
      }

//---------------------------------------------------------
//   changePart
//    extend/shrink part in front or at end
//---------------------------------------------------------

void Song::changePart(Part* oPart, unsigned pos, unsigned len)
      {
      startUndo();
      //
      // move events so they stay at same position in song
      //
      int delta    = oPart->tick() - pos;
      EventList* d = new EventList();
      EventList* s = oPart->events();
      for (iEvent ie = s->begin(); ie != s->end(); ++ie) {
            int tick = ie->first + delta;
            if (tick >= 0 && tick < int(len)) {
                  Event ev = ie->second.clone();
                  ev.move(delta);
                  d->add(ev, unsigned(tick));
                  }
            }
      if (oPart->fillLen() > 0 && len < (unsigned)oPart->fillLen())
            oPart->setFillLen(len);
      if (oPart->lenTick() < len && oPart->fillLen() > 0) {
            unsigned loop = oPart->fillLen();
            unsigned fillLen = len - oPart->lenTick();
            for (unsigned i = 0; i < fillLen / loop; ++i) {
                  int start = oPart->lenTick() + loop * i;
                  for (iEvent ie = s->begin(); ie != s->end(); ++ie) {
                        if (ie->first >= loop)
                              break;
                  	Event ev = ie->second.clone();
                        ev.move(start);
                        d->add(ev, ie->first + start);
                        }
                  }
            }
      Part* nPart = new Part(*oPart, d);
      nPart->setLenTick(len);
      nPart->setTick(pos);
      audio->msgChangePart(oPart, nPart, false);
      endUndo(SC_PART_MODIFIED);
      oPart->track()->partListChanged();
      if (unsigned(_len) < oPart->endTick())  // update song len
            setLen(oPart->endTick());
      }

//---------------------------------------------------------
//   movePart
//---------------------------------------------------------

void Song::movePart(Part* oPart, unsigned pos, Track* track)
      {
      Track* oTrack = oPart->track();
      Part* nPart   = new Part(*oPart);
      nPart->setTrack(track);
      nPart->setTick(pos);
      startUndo();
      if (oPart->track() != track) {
	      audio->msgRemovePart(oPart, false);
	      audio->msgAddPart(nPart, false);
            }
      else {
	      audio->msgChangePart(oPart, nPart, false);
            }
      endUndo(0);
      oTrack->partListChanged();
      if (len() < nPart->endTick())
            setLen(nPart->endTick());
      }

//---------------------------------------------------------
//   linkPart
//---------------------------------------------------------

void Song::linkPart(Part* sPart, unsigned pos, Track* track)
      {
      Part* dPart = track->newPart(sPart, true);
      dPart->setTick(pos);
      audio->msgAddPart(dPart);
      sPart->track()->partListChanged();
      dPart->track()->partListChanged();
      }

//---------------------------------------------------------
//   copyPart
//---------------------------------------------------------

void Song::copyPart(Part* sPart, unsigned pos, Track* track)
      {
      bool clone = sPart->events()->arefCount() > 1;
      Part* dPart = track->newPart(sPart, clone);
      dPart->setTick(pos);
      if (!clone) {
            //
            // Copy Events
            //
            EventList* se = sPart->events();
            EventList* de = dPart->events();
            for (iEvent i = se->begin(); i != se->end(); ++i) {
                  Event oldEvent = i->second;
                  Event ev = oldEvent.clone();
                  de->add(ev);
                  }
            }
      audio->msgAddPart(dPart);
      sPart->track()->partListChanged();
      dPart->track()->partListChanged();
      }

//---------------------------------------------------------
//   createLRPart
//---------------------------------------------------------

void Song::createLRPart(Track* track)
      {
      Part* part = track->newPart();
      if (part) {
            part->setTick(pos[1].tick());
            part->setLenTick(pos[2].tick()-pos[1].tick());
            part->setSelected(true);
            addPart(part);
            }
      }

//---------------------------------------------------------
//   addPart
//---------------------------------------------------------

void Song::addPart(Part* part)
      {
      audio->msgAddPart(part);
      // adjust song len:
      unsigned epos = part->tick() + part->lenTick();

      if (epos > len())
            setLen(epos);
      part->track()->partListChanged();
      }

//---------------------------------------------------------
//   selectPart
//---------------------------------------------------------

void Song::selectPart(Part* part, bool add)
      {
      if (add) {
            part->setSelected(!part->selected());
            part->track()->partListChanged();
            return;
            }
      for (iTrack it = _tracks.begin(); it != _tracks.end(); ++it) {
            PartList* pl = (*it)->parts();
            bool changed = false;
            for (iPart ip = pl->begin(); ip != pl->end(); ++ip) {
                  bool f = part == ip->second;
                  if (ip->second->selected() != f) {
                        ip->second->setSelected(f);
                        changed = true;
                        }
                  }
            if (changed)
                  (*it)->partListChanged();
            }
      }


//---------------------------------------------------------
//   setRecordFlag
//---------------------------------------------------------

void Song::setRecordFlag(Track* track, bool val)
      {
      if (track->type() == Track::AUDIO_OUTPUT) {
            if (!val && track->recordFlag() == false) {
                  muse->bounceToFile();
                  }
            }
      track->setRecordFlag(val);
      }

//---------------------------------------------------------
//   setMute
//---------------------------------------------------------

void Song::setMute(Track* track, bool val)
      {
      track->setMute(val);
      emit muteChanged(track, track->mute());
      }

//---------------------------------------------------------
//   setOff
//---------------------------------------------------------

void Song::setOff(Track* track, bool val)
      {
      track->setOff(val);
      emit offChanged(track, track->off());
      }

//---------------------------------------------------------
//   setAutoRead
//---------------------------------------------------------

void Song::setAutoRead(Track* track, bool val)
      {
      track->setAutoRead(val);
      emit autoReadChanged(track, track->autoRead());
      }

//---------------------------------------------------------
//   setAutoWrite
//---------------------------------------------------------

void Song::setAutoWrite(Track* track, bool val)
      {
      track->setAutoWrite(val);
      emit autoWriteChanged(track, track->autoRead());
      }

//---------------------------------------------------------
//   setSolo
//---------------------------------------------------------

void Song::setSolo(Track* track, bool val)
      {
      if (!track->setSolo(val))
            return;
      emit soloChanged(track, track->solo());
      soloFlag = false;
      for (iTrack i = _tracks.begin(); i != _tracks.end(); ++i) {
            if ((*i)->solo()) {
                  soloFlag = true;
                  break;
                  }
            }
      for (iTrack i = _tracks.begin(); i != _tracks.end(); ++i) {
            (*i)->updateMute();
            }
      }

//---------------------------------------------------------
//   addControllerVal
//    GUI context
//---------------------------------------------------------

void Song::addControllerVal(Track* t, int id, const Pos& pos, CVal val)
      {
      Ctrl* c = t->getController(id);
      if (c == 0) {
            printf("Song::addControllerVal:: no controller %d found\n", id);
            return;
            }
      addControllerVal(t, c, pos, val);
      }

void Song::addControllerVal(Track* t, Ctrl* c, const Pos& p, CVal val)
      {
      unsigned time = t->timeType() == AL::FRAMES ? p.frame() : p.tick();
      iCtrlVal e    = c->find(time);
      if (e == c->end()) {
            // schedule new controller event
            audio->msgAddController(t, c->id(), time, val);
            }
      else {
            CVal oval = c->value(time);
// printf("change controller %f -  %f\n", oval.f, val.f);
            startUndo();
            undoOp(UndoOp::ModifyCtrl, t, c->id(), time, val, oval);
            c->add(time, val);
            endUndo(0);
            }
      if (!audio->isPlaying() && t->autoRead()) {
            // current value may have changed
            unsigned ctime = t->timeType() == AL::FRAMES ? pos[0].frame() : pos[0].tick();
            CVal cval = c->value(ctime);
            if (c->schedVal().i != cval.i) {
                  if (t->isMidiTrack()) {
                        if (t->type() == Track::MIDI_CHANNEL) {
                              MidiChannel* mc = (MidiChannel*)t;
                              MidiEvent ev(0, 0, ME_CONTROLLER, c->id(), cval.i);
                              mc->playMidiEvent(&ev);
                              }
                        }
                  else {
                        // non midi controller are current once set
                        c->setCurVal(cval);
                        }
                  c->setSchedVal(cval);
                  // t->emitControllerChanged(c->id());
                  }
            }
      t->emitControllerChanged(c->id()); //moved this out here, otherwise canvas is not updated
      }

//---------------------------------------------------------
//   setControllerVal
//    GUI context
//---------------------------------------------------------

void Song::setControllerVal(Track* t, int id, CVal val)
      {
      Ctrl* c = t->getController(id);
      if (c == 0) {
            printf("Song::addControllerVal:: no controller %d found\n", id);
            return;
            }
      setControllerVal(t, c, val);
      }

void Song::setControllerVal(Track* t, Ctrl* c, CVal val)
      {
      if (t->isMidiTrack()) {
            if (t->type() == Track::MIDI_CHANNEL) {
                  MidiChannel* mc = (MidiChannel*)t;
                  MidiEvent ev(0, 0, ME_CONTROLLER, c->id(), val.i);
                  mc->playMidiEvent(&ev);
                  }
            else if (t->type() == Track::MIDI_OUT) {
                  MidiOutPort* mp = (MidiOutPort*)t;
                  MidiEvent ev(0, 0, ME_CONTROLLER, c->id(), val.i);
                  mp->playMidiEvent(&ev);
                  }
            }
      else {
            c->setCurVal(val);
            if (c->id() & 0x3ffff000) {
                  // plugin controller
                  AudioTrack* track = (AudioTrack*) t;
                  bool prefader;
                  int pluginIndex, ctrlIndex;
                  getCtrlPlugin(c->id(), &prefader, &pluginIndex, &ctrlIndex);
                  Pipeline* pipe  = prefader ? track->prePipe() : track->postPipe();
                  pipe->plugin(pluginIndex)->setParam(ctrlIndex, val.f);
                  }
            }
      c->setSchedVal(val);

      if (t->autoWrite()) {
            unsigned time = t->timeType() == AL::FRAMES ? pos[0].frame() : pos[0].tick();
            if (audio->isPlaying())
                  t->recEvents()->push_back(CtrlRecVal(time, c->id(), val));
            else {
                  iCtrlVal e = c->find(time);
                  if (e == c->end()) {
                        // schedule new controller event
                        audio->msgAddController(t, c->id(), time, val);
                        }
                  else {
                        CVal oval = c->value(time);
                        startUndo();
                        undoOp(UndoOp::ModifyCtrl, t, c->id(), time, val, oval);
                        c->add(time, val);
                        endUndo(0);
                        }
                  }
            }
      t->emitControllerChanged(c->id());
      }

//---------------------------------------------------------
//   removeControllerVal
//---------------------------------------------------------

void Song::removeControllerVal(Track* t, int id, unsigned time)
      {
      audio->msgRemoveController(t, id, time);
      t->emitControllerChanged(id);
      }

//---------------------------------------------------------
//   moveTrack
//---------------------------------------------------------

void Song::moveTrack(Track* src, Track* dst)
      {
      iTrack si = _tracks.find(src);
      iTrack di = _tracks.find(dst);
      if (si == _tracks.end() || di == _tracks.end()) {
            printf("Song::moveTrack() track not found\n");
            return;
            }
      _tracks.erase(si);
      _tracks.insert(di, src);
      }

//---------------------------------------------------------
//   changeTrackName
//---------------------------------------------------------

void Song::changeTrackName(Track* t, const QString& s)
      {
      startUndo();
      undoOp(UndoOp::RenameTrack, t, t->name(), s);
      t->setName(s);
      endUndo(SC_TRACK_MODIFIED);
      }

//---------------------------------------------------------
//   trackExists
//---------------------------------------------------------

bool Song::trackExists(Track* t) const
      {
      for (ciTrack it = _tracks.begin(); it != _tracks.end(); ++it) {
            if (*it == t)
                  return true;
            }
      return false;
      }

//---------------------------------------------------------
//   absoluteProjectPath
//---------------------------------------------------------

QString Song::absoluteProjectPath() const
      {
      return QDir::homePath() + "/" + config.projectPath + "/" + _projectPath;
      }

//---------------------------------------------------------
//   projectPath
//---------------------------------------------------------

QString Song::projectPath() const
      {
      return _projectPath;
      }

//---------------------------------------------------------
//   projectName
//---------------------------------------------------------

QString Song::projectName() const
      {
      QString name = _projectPath.split("/").last();
      return name;
      }

//---------------------------------------------------------
//   setProjectPath
//---------------------------------------------------------

void Song::setProjectPath(const QString& s)
      {
      _projectPath = s;
      }

//---------------------------------------------------------
//   read
//    return false on error
//---------------------------------------------------------

bool Song::read(QFile* qf)
      {
      QDomDocument doc;

      int line, column;
      QString err;
      if (!doc.setContent(qf, false, &err, &line, &column)) {
            QString col, ln, error;
            col.setNum(column);
            ln.setNum(line);
            error = err + "\n    at line: " + ln + " col: " + col;
            printf("error reading med file: %s\n", error.toLatin1().data());
            return false;
            }
      for (QDomNode node = doc.documentElement(); !node.isNull(); node = node.nextSibling()) {
            QDomElement e = node.toElement();
            if (e.isNull())
                  continue;
            if (e.tagName() == "muse") {
                  QString sversion = e.attribute("version", "1.0");
                  int major=0, minor=0;
                  sscanf(sversion.toLatin1().data(), "%d.%d", &major, &minor);
                  int version = major << 8 + minor;
                  if (version >= 0x200)
                        read20(node.firstChild());
                  else if (version == 0x100)
                        read10(node.firstChild());
                  else
                        printf("unsupported *.med file version %s\n", sversion.toLatin1().data());
                  }
            else
                  printf("MusE: %s not supported\n", e.tagName().toLatin1().data());
            }
      dirty = false;
      return true;
      }

//---------------------------------------------------------
//   read10
//---------------------------------------------------------

void Song::read10(QDomNode)
      {
      printf("reading type 1.0 *.med files not implemented\n");
      }

//---------------------------------------------------------
//   read20
//---------------------------------------------------------

void Song::read20(QDomNode node)
      {
      for (; !node.isNull(); node = node.nextSibling()) {
            QDomElement e = node.toElement();
            if (e.isNull())
                  continue;
            if (e.tagName() == "configuration")
                  readConfiguration(node.firstChild());
            else if (e.tagName() == "song")
                  read(node.firstChild());
            else if (e.tagName() == "toplevels")
                  muse->readToplevels(node.firstChild());
            else
                  printf("MusE:read20(): unknown tag %s\n", e.tagName().toLatin1().data());
            }
      }

