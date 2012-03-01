//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: structure.cpp,v 1.113.2.68 2009/12/21 14:51:51 spamatica Exp $
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
#include "structure.h"
#include "globals.h"

#include <set>
using std::set;

namespace MusECore {

//---------------------------------------------------------
//   adjustGlobalLists
//    helper that adjusts tempo, sig, key and marker
//    lists everything from startPos is adjusted
//    'diff' number of ticks.
//---------------------------------------------------------

void adjustGlobalLists(Undo& operations, int startPos, int diff)
{
  const TempoList* t = &MusEGlobal::tempomap;
  const AL::SigList* s   = &AL::sigmap;
  const KeyList* k   = &MusEGlobal::keymap;

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

  MarkerList *markerlist = MusEGlobal::song->marker();
  for(iMarker i = markerlist->begin(); i != markerlist->end(); ++i)
  {
      Marker* m = &i->second;
      int tick = m->tick();
      if (tick > startPos)
      {
        if (tick + diff < startPos ) { // these ticks should be removed
          operations.push_back(UndoOp(UndoOp::ModifyMarker, 0, m));    
        } else {
          Marker *newMarker = new Marker();
          *newMarker = *m;
          newMarker->setTick(tick + diff);  
          operations.push_back(UndoOp(UndoOp::ModifyMarker, newMarker, m));
        }
      }
  }

}

//---------------------------------------------------------
//   globalCut
//    - remove area between left and right locator
//    - cut master track
//---------------------------------------------------------

void globalCut(bool onlySelectedTracks)
      {
      int lpos = MusEGlobal::song->lpos();
      int rpos = MusEGlobal::song->rpos();
      if ((lpos - rpos) >= 0)
            return;

      Undo operations;
      TrackList* tracks = MusEGlobal::song->tracks();
      
      for (iTrack it = tracks->begin(); it != tracks->end(); ++it) {
            Track* track = *it;
            if (track == 0 || (onlySelectedTracks && !track->selected()))
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
                      Part *nPart;
                      if (track->isMidiTrack())
                        nPart = new MidiPart(*(MidiPart*)part);
                      else
                        nPart = new WavePart(*(WavePart*)part);

                      nPart->setLenTick(len);
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
                        Part* p1;
                        Part* p2;
                        Part* p3;
                        track->splitPart(part, lpos, p1, p2);
                        delete p2;
                        track->splitPart(part, rpos, p2, p3);
                        delete p2;
                        p3->setTick(lpos);
                        p1->events()->incARef(-1); // the later MusEGlobal::song->applyOperationGroup() will increment it so we must decrement it first :/
                        p3->events()->incARef(-1); // the later MusEGlobal::song->applyOperationGroup() will increment it so we must decrement it first :/

                        // Indicate no undo, and do port controller values and clone parts.
                        operations.push_back(UndoOp(UndoOp::ModifyPart,part, p1, true, true));
                        operations.push_back(UndoOp(UndoOp::AddPart,p3));
                        }
                  else if ((t >= lpos) && (t < rpos) && (t+l) > rpos) {
                        // remove part head
                        
                        Part* p1;
                        Part* p2;
                        track->splitPart(part, rpos, p1, p2);
                        delete p1;
                        p2->setTick(lpos);
                        p2->events()->incARef(-1); // the later MusEGlobal::song->applyOperationGroup() will increment it so we must decrement it first :/
                        operations.push_back(UndoOp(UndoOp::ModifyPart,part, p2, true, true));
                        }
                  else if (t >= rpos) {
                        // move part to the left
                        Part *nPart;
                        if (track->isMidiTrack())
                          nPart = new MidiPart(*(MidiPart*)part);
                        else
                          nPart = new WavePart(*(WavePart*)part);
                        int nt = part->tick();
                        nPart->setTick(nt - (rpos -lpos));
                        // Indicate no undo, and do port controller values but not clone parts.
                        operations.push_back(UndoOp(UndoOp::ModifyPart,part, nPart, true, false));
                        }
                  }
            }
      int diff = lpos - rpos;
      adjustGlobalLists(operations, lpos, diff);

      MusEGlobal::song->applyOperationGroup(operations);
      }

//---------------------------------------------------------
//   globalInsert
//    - insert empty space at left locator position upto
//      right locator
//    - insert in master track
//---------------------------------------------------------

void globalInsert(bool onlySelectedTracks)
{
      Undo operations=movePartsTotheRight(MusEGlobal::song->lpos(), MusEGlobal::song->rpos()-MusEGlobal::song->lpos(), onlySelectedTracks);
      MusEGlobal::song->applyOperationGroup(operations);
}

Undo movePartsTotheRight(unsigned int startTicks, int moveTicks, bool only_selected, set<Track*>* tracklist)
{
      if (moveTicks<=0)
            return Undo();

      Undo operations;
      TrackList* tracks = MusEGlobal::song->tracks();
      
      for (iTrack it = tracks->begin(); it != tracks->end(); ++it) {
            Track* track = *it;
            if ( (track == 0) ||
                 (only_selected && !track->selected()) ||
                 (tracklist && tracklist->find(track)==tracklist->end()) )
                  continue;
            PartList* pl = track->parts();
            for (riPart p = pl->rbegin(); p != pl->rend(); ++p) {
                  Part* part = p->second;
                  unsigned t = part->tick();
                  int l = part->lenTick();
                  if (t + l <= startTicks)
                        continue;
                  if (startTicks > t && startTicks < (t+l)) {
                        // split part to insert new space
                        Part* p1;
                        Part* p2;
                        track->splitPart(part, startTicks, p1, p2);
                        p2->setTick(startTicks+moveTicks);
                        p2->events()->incARef(-1); // the later MusEGlobal::song->applyOperationGroup() will increment it so we must decrement it first :/
                        p1->events()->incARef(-1); // the later MusEGlobal::song->applyOperationGroup() will increment it so we must decrement it first :/

                        operations.push_back(UndoOp(UndoOp::ModifyPart, part, p1, true, true));
                        operations.push_back(UndoOp(UndoOp::AddPart, p2));
                        }
                  else if (t >= startTicks) {
                        Part *nPart;
                        if (track->isMidiTrack())
                          nPart = new MidiPart(*(MidiPart*)part);
                        else
                          nPart = new WavePart(*(WavePart*)part);
                        nPart->setTick(t + moveTicks);
                        operations.push_back(UndoOp(UndoOp::ModifyPart, part, nPart, true, false));
                        }
                  }
            }

      adjustGlobalLists(operations, startTicks, moveTicks);

      return operations;
      }


//---------------------------------------------------------
//   globalSplit
//    - split all parts at the song position pointer
//---------------------------------------------------------

void globalSplit(bool onlySelectedTracks)
{
    Undo operations=partSplitter(MusEGlobal::song->cpos(), onlySelectedTracks);
    MusEGlobal::song->applyOperationGroup(operations);
}

Undo partSplitter(unsigned int pos, bool onlySelectedTracks)
{
    Undo operations;
    TrackList* tracks = MusEGlobal::song->tracks();

    for (iTrack it = tracks->begin(); it != tracks->end(); ++it) {
        Track* track = *it;
        if (track == 0 || (onlySelectedTracks && !track->selected()))
              continue;

        PartList* pl = track->parts();
        for (iPart p = pl->begin(); p != pl->end(); ++p) {
              Part* part = p->second;
              unsigned int p1 = part->tick();
              unsigned int l0 = part->lenTick();
              if (pos > p1 && pos < (p1+l0)) {
                    Part* p1;
                    Part* p2;
                    track->splitPart(part, pos, p1, p2);

                    p1->events()->incARef(-1); // the later MusEGlobal::song->applyOperationGroup() will increment it
                    p2->events()->incARef(-1); // so we must decrement it first :/

                    //MusEGlobal::song->informAboutNewParts(part, p1); // is unneccessary because of ModifyPart
                    MusEGlobal::song->informAboutNewParts(part, p2);
                    operations.push_back(UndoOp(UndoOp::ModifyPart,part, p1, true, false));
                    operations.push_back(UndoOp(UndoOp::AddPart,p2));
                    if (MusEGlobal::debugMsg)
                    {
                          printf("in partSplitter: part1 %d\n",p1->events()->refCount());
                          printf("in partSplitter: part2 %d\n",p2->events()->refCount());
                          }
                    break;
                    }
              }
        }
    return operations;
}


} // namespace MusECore
