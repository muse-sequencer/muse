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
#include "midioutport.h"
#include "midiinport.h"
#include "instruments/minstrument.h"

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
//  addEvent
//    return true if event was added
//---------------------------------------------------------

bool Song::addEvent(Event& event, Part* part)
      {
      if (event.type() == Controller) {
            MidiTrack* track = (MidiTrack*)part->track();
            int tick  = event.tick();
            int cntrl = event.dataA();
            CVal val;
            val.i = event.dataB();
            if (!track->addControllerVal(cntrl, tick, val)) {
                  track->addMidiController(track->instrument(), cntrl);
                  if (!track->addControllerVal(cntrl, tick, val)) {
                        return false;
                        }
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
            int tick  = newEvent.tick();
            int cntrl = newEvent.dataA();
            CVal val;
            val.i   = newEvent.dataB();
            track->addControllerVal(cntrl, tick, val);
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
      if (f)
            audio->msgPlay(false);
      else
            muse->stopAction->setChecked(true);
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
//            _vcpos = val;
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

            case SEQM_ADD_TRACK:
                  insertTrack2(msg->track);
                  break;

            case SEQM_REMOVE_TRACK:
                  removeTrack2(msg->track);
                  break;

            case SEQM_ADD_PART:
                  {
                  Part* part = (Part*)(msg->p1);
                  part->track()->addPart(part);
                  }
                  break;

            case SEQM_REMOVE_PART:
                  {
                  Part* part = (Part*)(msg->p1);
                  Track* track = part->track();
                  track->parts()->remove(part);
                  }
                  break;

            case SEQM_CHANGE_PART:
                  {
                  Part* newPart = (Part*)msg->p2;
                  Part* oldPart = (Part*)msg->p1;
                  Part part = *newPart;
                  *newPart  = *oldPart;
                  *oldPart  = part;
                  }
                  break;

            case SEQM_MOVE_TRACK:
                  moveTrack((Track*)(msg->p1), (Track*)(msg->p2));
                  break;

            default:
                  printf("unknown seq message %d\n", msg->id);
                  break;
            }
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

      qDeleteAll(_midis);
      _midis.clear();

      qDeleteAll(_waves);
      _waves.clear();

      qDeleteAll(_inputs);     // audio input ports
      _inputs.clear();

      qDeleteAll(_outputs);    // audio output ports
      _outputs.clear();

      qDeleteAll(_groups);     // mixer groups
      _groups.clear();

      qDeleteAll(_synthIs);
      _synthIs.clear();

      qDeleteAll(_midiSyntis);
      _midiSyntis.clear();

      qDeleteAll(_midiOutPorts);
      _midiOutPorts.clear();

      qDeleteAll(_midiInPorts);
      _midiInPorts.clear();

      AL::tempomap.clear();
      AL::sigmap.clear();
      undoList->clear();
      redoList->clear();
      _markerList->clear();
      pos[0].setTick(0);
      pos[1].setTick(0);
      pos[2].setTick(0);
//      _vcpos.setTick(0);

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
                        restartJack();
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
                                          iCtrlVal s = cl->lowerBound(start);
                                          iCtrlVal e = cl->lowerBound(end);
                                          while (s != e)
                                                cl->erase(s++);
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
//   addControllerVal
//    GUI context
//---------------------------------------------------------

void Song::cmdAddControllerVal(Track* t, int id, const Pos& pos, CVal val)
      {
      Ctrl* c = t->getController(id);
      if (c == 0) {
            printf("Song::addControllerVal:: no controller %d found\n", id);
            return;
            }
      cmdAddControllerVal(t, c, pos, val);
      }

void Song::cmdAddControllerVal(Track* t, Ctrl* c, const Pos& p, CVal val)
      {
      unsigned time = t->timeType() == AL::FRAMES ? p.frame() : p.tick();
      iCtrlVal e    = c->find(time);
      if (e == c->end()) {
            // add new controller event
            audio->msgAddController(t, c->id(), time, val);
            }
      else {
            //
            // change controller is handled inline:
            //
            CVal oval = c->value(time);
            startUndo();
            undoOp(UndoOp::ModifyCtrl, t, c->id(), time, val, oval);
            c->add(time, val);
            endUndo(0);
            }
      if (!audio->isPlaying() && t->autoRead()) {
            // current value may have changed
            unsigned ctime = t->timeType() == AL::FRAMES ? pos[0].frame() : pos[0].tick();
            CVal cval = c->value(ctime);
            if (c->curVal().i != cval.i) {
                  if (t->type() == Track::MIDI) {
                        MidiEvent ev(0, 0, ME_CONTROLLER, c->id(), cval.i);
                        ((MidiTrack*)t)->playMidiEvent(&ev);
                        }
                  c->setCurVal(cval);
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
            if (t->type() == Track::MIDI) {
                  MidiTrack* mc = (MidiTrack*)t;
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
      c->setCurVal(val);

      if (t->autoWrite()) {
            unsigned time = t->timeType() == AL::FRAMES ? pos[0].frame() : pos[0].tick();
            if (audio->isPlaying())
                  t->recEvents()->push_back(CtrlRecVal(time, c->id(), val));
            else {
                  iCtrlVal e = c->find(time);
                  if (e == c->end()) {
                        // add new controller event
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
//   cmdRemoveControllerVal
//---------------------------------------------------------

void Song::cmdRemoveControllerVal(Track* t, int id, unsigned time)
      {
      audio->msgRemoveController(t, id, time);
      t->emitControllerChanged(id);
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

//---------------------------------------------------------
//   restartJack
//---------------------------------------------------------

void Song::restartJack()
      {
      muse->seqStop();
      audioState = AUDIO_STOP;
      for (;;) {
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
                 "click on the Restart button."), 
               "restart", "cancel", "save project"
               );
            if (btn == 0) {
                  if (!audioDriver->restart())
                        break;
                  }
            else if (btn == 2)
                  muse->save();
            else if (btn == 1)
                  exit(-1);
            }
      muse->seqRestart();
      }

//---------------------------------------------------------
//   routeChanged
//---------------------------------------------------------

void Song::routeChanged(QAction* a)
      {
      audio->msgRoute(a->isChecked(), a->data().value<Route>());
      }


