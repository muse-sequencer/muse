//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: app.cpp,v 1.113.2.68 2009/12/21 14:51:51 spamatica Exp $
//
//  (C) Copyright 1999-2004 Werner Schweer (ws@seh.de)
//  (C) Copyright 2011  Robert Jonsson (rj@spamatica.se)
//=========================================================

#include <qmessagebox.h>
#include "app.h"
#include "track.h"
#include "song.h"
#include "tempo.h"
#include "al/sig.h"
#include "keyevent.h"
#include "audio.h"
#include "marker/marker.h"


//---------------------------------------------------------
//   adjustGlobalLists
//    helper that adjusts tempo, sig, key and marker
//    lists everything from startPos is adjusted
//    'diff' number of ticks.
//    function requires undo to be handled outside
//---------------------------------------------------------

void MusE::adjustGlobalLists(int startPos, int diff)
{
  const TempoList* t = &tempomap;
  const AL::SigList* s   = &AL::sigmap;
  const KeyList* k   = &keymap;

  criTEvent it   = t->rbegin();
  AL::criSigEvent is = s->rbegin();
  criKeyEvent ik = k->rbegin();

  // key
  for (; ik != k->rend(); ik++) {
    const KeyEvent &ev = (KeyEvent)ik->second;
    int tick = ev.tick;
    int key = ev.key;
    if (tick < startPos )
      break;

    if (tick > startPos && tick +diff < startPos ) { // remove
      audio->msgRemoveKey(tick, key, false);
    }
    else {
      audio->msgRemoveKey(tick, key, false);
      audio->msgAddKey(tick+diff, key, false);
      }
  }

  // tempo
  for (; it != t->rend(); it++) {
    const TEvent* ev = (TEvent*)it->second;
    int tick = ev->tick;
    int tempo = ev->tempo;
    if (tick < startPos )
      break;

    if (tick > startPos && tick +diff < startPos ) { // remove
      audio->msgDeleteTempo(tick, tempo, false);
    }
    else {
      audio->msgDeleteTempo(tick, tempo, false);
      audio->msgAddTempo(tick+diff, tempo, false);
      }
  }

  // sig
  for (; is != s->rend(); is++) {
    const AL::SigEvent* ev = (AL::SigEvent*)is->second;
    int tick = ev->tick;
    if (tick < startPos )
      break;

    int z = ev->sig.z;
    int n = ev->sig.n;
    if (tick > startPos && tick +diff < startPos ) { // remove
      audio->msgRemoveSig(tick, z, n, false);
    }
    else {
      audio->msgRemoveSig(tick, z, n, false);
      audio->msgAddSig(tick+diff, z, n, false);
    }
  }

  MarkerList *markerlist = song->marker();
  for(iMarker i = markerlist->begin(); i != markerlist->end(); ++i)
  {
      Marker* m = &i->second;
      int tick = m->tick();
      if (tick > startPos)
      {
        if (tick + diff < startPos ) { // these ticks should be removed
          Marker *oldMarker = new Marker();
          *oldMarker = *m;
          markerlist->remove(m);
          song->addUndo(UndoOp(UndoOp::ModifyMarker,oldMarker, 0));
        } else {
          Marker *oldMarker = new Marker();
          *oldMarker = *m;
          m->setTick(tick + diff);
          song->addUndo(UndoOp(UndoOp::ModifyMarker,oldMarker, m));
        }
      }
  }

}

//---------------------------------------------------------
//   globalCut
//    - remove area between left and right locator
//    - do not touch muted track
//    - cut master track
//---------------------------------------------------------

void MusE::globalCut()
      {
      int lpos = song->lpos();
      int rpos = song->rpos();
      if ((lpos - rpos) >= 0)
            return;

      song->startUndo();
      TrackList* tracks = song->tracks();
      for (iTrack it = tracks->begin(); it != tracks->end(); ++it) {
            MidiTrack* track = dynamic_cast<MidiTrack*>(*it);
            if (track == 0 || track->mute())
                  continue;
            PartList* pl = track->parts();
            for (iPart p = pl->begin(); p != pl->end(); ++p) {
                  Part* part = p->second;
                  int t = part->tick();
                  int l = part->lenTick();
                  if (t + l <= lpos)
                        continue;
                  if ((t >= lpos) && ((t+l) <= rpos)) {
                        audio->msgRemovePart(part, false);
                        }
                  else if ((t < lpos) && ((t+l) > lpos) && ((t+l) <= rpos)) {
                        // remove part tail
                        int len = lpos - t;
                        MidiPart* nPart = new MidiPart(*(MidiPart*)part);
                        nPart->setLenTick(len);
                        //
                        // cut Events in nPart
                        EventList* el = nPart->events();
                        iEvent ie = el->lower_bound(t + len);
                        for (; ie != el->end();) {
                              iEvent i = ie;
                              ++ie;
                              // Indicate no undo, and do not do port controller values and clone parts.
                              //audio->msgDeleteEvent(i->second, nPart, false);
                              audio->msgDeleteEvent(i->second, nPart, false, false, false);
                              }
                        // Indicate no undo, and do port controller values and clone parts.
                        //audio->msgChangePart(part, nPart, false);
                        audio->msgChangePart(part, nPart, false, true, true);
                        }
                  else if ((t < lpos) && ((t+l) > lpos) && ((t+l) > rpos)) {
                        //----------------------
                        // remove part middle
                        //----------------------

                        MidiPart* nPart = new MidiPart(*(MidiPart*)part);
                        EventList* el = nPart->events();
                        iEvent is = el->lower_bound(lpos);
                        iEvent ie = el->upper_bound(rpos);
                        for (iEvent i = is; i != ie;) {
                              iEvent ii = i;
                              ++i;
                              // Indicate no undo, and do not do port controller values and clone parts.
                              //audio->msgDeleteEvent(ii->second, nPart, false);
                              audio->msgDeleteEvent(ii->second, nPart, false, false, false);
                              }

                        ie = el->lower_bound(rpos);
                        for (; ie != el->end();) {
                              iEvent i = ie;
                              ++ie;
                              Event event = i->second;
                              Event nEvent = event.clone();
                              nEvent.setTick(nEvent.tick() - (rpos-lpos));
                              // Indicate no undo, and do not do port controller values and clone parts.
                              //audio->msgChangeEvent(event, nEvent, nPart, false);
                              audio->msgChangeEvent(event, nEvent, nPart, false, false, false);
                              }
                        nPart->setLenTick(l - (rpos-lpos));
                        // Indicate no undo, and do port controller values and clone parts.
                        //audio->msgChangePart(part, nPart, false);
                        audio->msgChangePart(part, nPart, false, true, true);
                        }
                  else if ((t >= lpos) && (t < rpos) && (t+l) > rpos) {
                        // TODO: remove part head
                        }
                  else if (t >= rpos) {
                        MidiPart* nPart = new MidiPart(*(MidiPart*)part);
                        int nt = part->tick();
                        nPart->setTick(nt - (rpos -lpos));
                        // Indicate no undo, and do port controller values but not clone parts.
                        //audio->msgChangePart(part, nPart, false);
                        audio->msgChangePart(part, nPart, false, true, false);
                        }
                  }
            }
      int diff = lpos - rpos;
      adjustGlobalLists(lpos, diff);

      song->endUndo(SC_TRACK_MODIFIED | SC_PART_MODIFIED | SC_PART_REMOVED | SC_TEMPO | SC_KEY | SC_SIG);
      }

//---------------------------------------------------------
//   globalInsert
//    - insert empty space at left locator position upto
//      right locator
//    - do not touch muted track
//    - insert in master track
//---------------------------------------------------------

void MusE::globalInsert()
      {
      unsigned lpos = song->lpos();
      unsigned rpos = song->rpos();
      if (lpos >= rpos)
            return;

      song->startUndo();
      TrackList* tracks = song->tracks();
      for (iTrack it = tracks->begin(); it != tracks->end(); ++it) {
            MidiTrack* track = dynamic_cast<MidiTrack*>(*it);
            //
            // process only non muted midi tracks
            //
            if (track == 0 || track->mute())
                  continue;
            PartList* pl = track->parts();
            for (riPart p = pl->rbegin(); p != pl->rend(); ++p) {
                  Part* part = p->second;
                  unsigned t = part->tick();
                  int l = part->lenTick();
                  if (t + l <= lpos)
                        continue;
                  if (lpos >= t && lpos < (t+l)) {
                        MidiPart* nPart = new MidiPart(*(MidiPart*)part);
                        nPart->setLenTick(l + (rpos-lpos));
                        EventList* el = nPart->events();

                        iEvent i = el->end();
                        while (i != el->begin()) {
                              --i;
                              if (i->first < lpos)
                                    break;
                              Event event  = i->second;
                              Event nEvent = i->second.clone();
                              nEvent.setTick(nEvent.tick() + (rpos-lpos));
                              // Indicate no undo, and do not do port controller values and clone parts.
                              //audio->msgChangeEvent(event, nEvent, nPart, false);
                              audio->msgChangeEvent(event, nEvent, nPart, false, false, false);
                              }
                        // Indicate no undo, and do port controller values and clone parts.
                        //audio->msgChangePart(part, nPart, false);
                        audio->msgChangePart(part, nPart, false, true, true);
                        }
                  else if (t > lpos) {
                        MidiPart* nPart = new MidiPart(*(MidiPart*)part);
                        nPart->setTick(t + (rpos -lpos));
                        // Indicate no undo, and do port controller values but not clone parts.
                        //audio->msgChangePart(part, nPart, false);
                        audio->msgChangePart(part, nPart, false, true, false);
                        }
                  }
            }

      int diff = rpos - lpos;
      adjustGlobalLists(lpos, diff);

      song->endUndo(SC_TRACK_MODIFIED | SC_PART_MODIFIED | SC_PART_REMOVED | SC_TEMPO | SC_KEY | SC_SIG );
      }


//---------------------------------------------------------
//   globalSplit
//    - split all parts at the song position pointer
//    - do not touch muted track
//---------------------------------------------------------

void MusE::globalSplit()
      {
      int pos = song->cpos();
      song->startUndo();
      TrackList* tracks = song->tracks();
      for (iTrack it = tracks->begin(); it != tracks->end(); ++it) {
            Track* track = *it;
            PartList* pl = track->parts();
            for (iPart p = pl->begin(); p != pl->end(); ++p) {
                  Part* part = p->second;
                  int p1 = part->tick();
                  int l0 = part->lenTick();
                  if (pos > p1 && pos < (p1+l0)) {
                        Part* p1;
                        Part* p2;
                        track->splitPart(part, pos, p1, p2);
                        // Indicate no undo, and do port controller values but not clone parts.
                        //audio->msgChangePart(part, p1, false);
                        audio->msgChangePart(part, p1, false, true, false);
                        audio->msgAddPart(p2, false);
                        break;
                        }
                  }
            }
      song->endUndo(SC_TRACK_MODIFIED | SC_PART_MODIFIED | SC_PART_INSERTED);
      }

//---------------------------------------------------------
//   copyRange
//    - copy space between left and right locator position
//      to song position pointer
//    - dont process muted tracks
//    - create a new part for every track containing the
//      copied events
//---------------------------------------------------------

void MusE::copyRange()
      {
      QMessageBox::critical(this,
         tr("MusE: Copy Range"),
         tr("not implemented")
         );
      }

//---------------------------------------------------------
//   cutEvents
//    - make sure that all events in a part end where the
//      part ends
//    - process only marked parts
//---------------------------------------------------------

void MusE::cutEvents()
      {
      QMessageBox::critical(this,
         tr("MusE: Cut Events"),
         tr("not implemented")
         );
      }

//---------------------------------------------------------
//   checkRegionNotNull
//    return true if (rPos - lPos) <= 0
//---------------------------------------------------------

bool MusE::checkRegionNotNull()
      {
      int start = song->lPos().frame();
      int end   = song->rPos().frame();
      if (end - start <= 0) {
            QMessageBox::critical(this,
               tr("MusE: Bounce"),
               tr("set left/right marker for bounce range")
               );
            return true;
            }
      return false;
      }

