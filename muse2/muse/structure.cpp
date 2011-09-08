//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: app.cpp,v 1.113.2.68 2009/12/21 14:51:51 spamatica Exp $
//
//  (C) Copyright 1999-2004 Werner Schweer (ws@seh.de)
//  (C) Copyright 2011  Robert Jonsson (rj@spamatica.se)
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

#include <qmessagebox.h>
#include "app.h"
#include "track.h"
#include "song.h"
#include "tempo.h"
#include "al/sig.h"
#include "keyevent.h"
#include "audio.h"
#include "marker/marker.h"

namespace MusEApp {

//---------------------------------------------------------
//   adjustGlobalLists
//    helper that adjusts tempo, sig, key and marker
//    lists everything from startPos is adjusted
//    'diff' number of ticks.
//---------------------------------------------------------

void MusE::adjustGlobalLists(Undo& operations, int startPos, int diff)
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
      operations.push_back(UndoOp(UndoOp::DeleteKey, tick, key));
    }
    else {
      operations.push_back(UndoOp(UndoOp::DeleteKey,tick, key));
      operations.push_back(UndoOp(UndoOp::AddKey,tick+diff, key));
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
      operations.push_back(UndoOp(UndoOp::DeleteTempo,tick, tempo));
    }
    else {
      operations.push_back(UndoOp(UndoOp::DeleteTempo,tick, tempo));
      operations.push_back(UndoOp(UndoOp::AddTempo,tick+diff, tempo));
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
      operations.push_back(UndoOp(UndoOp::DeleteSig,tick, z, n));
    }
    else {
      operations.push_back(UndoOp(UndoOp::DeleteSig,tick, z, n));
      operations.push_back(UndoOp(UndoOp::AddSig,tick+diff, z, n));
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
          operations.push_back(UndoOp(UndoOp::ModifyMarker,oldMarker, 0));
        } else {
          Marker *oldMarker = new Marker();
          *oldMarker = *m;
          m->setTick(tick + diff);
          operations.push_back(UndoOp(UndoOp::ModifyMarker,oldMarker, m));
        }
      }
  }

}

//---------------------------------------------------------
//   globalCut
//    - remove area between left and right locator
//    - cut master track
//---------------------------------------------------------

void MusE::globalCut()
      {
      int lpos = song->lpos();
      int rpos = song->rpos();
      if ((lpos - rpos) >= 0)
            return;

      Undo operations;
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
                        operations.push_back(UndoOp(UndoOp::DeletePart,part));
                        }
                  else if ((t < lpos) && ((t+l) > lpos) && ((t+l) <= rpos)) {
                        // remove part tail
                        int len = lpos - t;
                        MidiPart* nPart = new MidiPart(*(MidiPart*)part);
                        nPart->setLenTick(len);
                        //
                        // cut Events in nPart
                        EventList* el = nPart->events();
                        for (iEvent ie = el->lower_bound(len); ie != el->end(); ++ie)
                              operations.push_back(UndoOp(UndoOp::DeleteEvent,ie->second, nPart, false, false));

                        operations.push_back(UndoOp(UndoOp::ModifyPart,part, nPart, true, true));
                        }
                  else if ((t < lpos) && ((t+l) > lpos) && ((t+l) > rpos)) {
                        //----------------------
                        // remove part middle
                        //----------------------

                        MidiPart* nPart = new MidiPart(*(MidiPart*)part);
                        EventList* el = nPart->events();
                        iEvent is = el->lower_bound(lpos-t);
                        iEvent ie = el->lower_bound(rpos-t); //lower bound, because we do NOT want to erase the events at rpos-t
                        for (iEvent i = is; i != ie; ++i)
                              operations.push_back(UndoOp(UndoOp::DeleteEvent,i->second, nPart, false, false));

                        for (iEvent i = el->lower_bound(rpos-t); i != el->end(); ++i) {
                              Event event = i->second;
                              Event nEvent = event.clone();
                              nEvent.setTick(nEvent.tick() - (rpos-lpos));
                              // Indicate no undo, and do not do port controller values and clone parts.
                              operations.push_back(UndoOp(UndoOp::ModifyEvent,nEvent, event, nPart, false, false));
                              }
                        nPart->setLenTick(l - (rpos-lpos));
                        // Indicate no undo, and do port controller values and clone parts.
                        operations.push_back(UndoOp(UndoOp::ModifyPart,part, nPart, true, true));
                        }
                  else if ((t >= lpos) && (t < rpos) && (t+l) > rpos) {
                        // remove part head
                        
                        MidiPart* nPart = new MidiPart(*(MidiPart*)part);
                        EventList* el = nPart->events();
                        iEvent i_end = el->lower_bound(rpos-t); //lower bound, because we do NOT want to erase the events at rpos-t
                        for (iEvent it = el->begin(); it!=i_end; it++)
                              operations.push_back(UndoOp(UndoOp::DeleteEvent,it->second, nPart, false, false));
                        
                        for (iEvent it = el->lower_bound(rpos-t); it!=el->end(); it++) {
                              Event event = it->second;
                              Event nEvent = event.clone();
                              nEvent.setTick(nEvent.tick() - (rpos-t));
                              // Indicate no undo, and do not do port controller values and clone parts.
                              operations.push_back(UndoOp(UndoOp::ModifyEvent,nEvent, event, nPart, false, false));
                              }
                        
                        nPart->setLenTick(l - (rpos-t));
                        operations.push_back(UndoOp(UndoOp::ModifyPart,part, nPart, true, true));
                        }
                  else if (t >= rpos) {
                        MidiPart* nPart = new MidiPart(*(MidiPart*)part);
                        int nt = part->tick();
                        nPart->setTick(nt - (rpos -lpos));
                        // Indicate no undo, and do port controller values but not clone parts.
                        operations.push_back(UndoOp(UndoOp::ModifyPart,part, nPart, true, false));
                        }
                  }
            }
      int diff = lpos - rpos;
      adjustGlobalLists(operations, lpos, diff);

      song->applyOperationGroup(operations);
      }

//---------------------------------------------------------
//   globalInsert
//    - insert empty space at left locator position upto
//      right locator
//    - insert in master track
//---------------------------------------------------------

void MusE::globalInsert()
      {
      unsigned lpos = song->lpos();
      unsigned rpos = song->rpos();
      if (lpos >= rpos)
            return;

      Undo operations;
      TrackList* tracks = song->tracks();
      
      for (iTrack it = tracks->begin(); it != tracks->end(); ++it) {
            MidiTrack* track = dynamic_cast<MidiTrack*>(*it);
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

                        for (riEvent i = el->rbegin(); i!=el->rend(); ++i)
                        {
                              if (i->first < lpos-t)
                                    break;
                              Event event  = i->second;
                              Event nEvent = i->second.clone();
                              nEvent.setTick(nEvent.tick() + (rpos-lpos));
                              operations.push_back(UndoOp(UndoOp::ModifyEvent, nEvent, event, nPart, false, false));
                              }
                        operations.push_back(UndoOp(UndoOp::ModifyPart, part, nPart, true, true));
                        }
                  else if (t > lpos) {
                        MidiPart* nPart = new MidiPart(*(MidiPart*)part);
                        nPart->setTick(t + (rpos -lpos));
                        operations.push_back(UndoOp(UndoOp::ModifyPart, part, nPart, true, false));
                        }
                  }
            }

      int diff = rpos - lpos;
      adjustGlobalLists(operations, lpos, diff);

      song->applyOperationGroup(operations);
      }


//---------------------------------------------------------
//   globalSplit
//    - split all parts at the song position pointer
//---------------------------------------------------------

void MusE::globalSplit()
      {
      int pos = song->cpos();
      Undo operations;
      TrackList* tracks = song->tracks();

      for (iTrack it = tracks->begin(); it != tracks->end(); ++it) {
            Track* track = *it;
            if (track == 0 || track->mute())
                  continue;

            PartList* pl = track->parts();
            for (iPart p = pl->begin(); p != pl->end(); ++p) {
                  Part* part = p->second;
                  int p1 = part->tick();
                  int l0 = part->lenTick();
                  if (pos > p1 && pos < (p1+l0)) {
                        Part* p1;
                        Part* p2;
                        track->splitPart(part, pos, p1, p2);

                        p1->events()->incARef(-1); // the later song->applyOperationGroup() will increment it
                        p2->events()->incARef(-1); // so we must decrement it first :/

                        operations.push_back(UndoOp(UndoOp::ModifyPart,part, p1, true, false));
                        operations.push_back(UndoOp(UndoOp::AddPart,p2));
                        break;
                        }
                  }
            }
      song->applyOperationGroup(operations);
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
} // namespace MusEApp
