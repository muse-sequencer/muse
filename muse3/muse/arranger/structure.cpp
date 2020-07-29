//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: structure.cpp,v 1.113.2.68 2009/12/21 14:51:51 spamatica Exp $
//
//  (C) Copyright 1999-2004 Werner Schweer (ws@seh.de)
//  (C) Copyright 2011  Robert Jonsson (rj@spamatica.se)
//  (C) Copyright 2013  Florian Jung (flo93@sourceforge.net)
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
#include "sig.h"
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
//   If diff is negative it is a 'cut' operation,
//    otherwise it is an 'insert' operation.
//---------------------------------------------------------

void adjustGlobalLists(Undo& operations, unsigned int startPos, int diff)
{
  const TempoList* t  = &MusEGlobal::tempomap;
  const SigList* s    = &MusEGlobal::sigmap;
  const KeyList* k    = &MusEGlobal::keymap;
  const MarkerList* m = MusEGlobal::song->marker();

  const bool is_cut = diff < 0;

  // key
  // It is important that the order of operations always be delete followed by add...
  // What needs to be deleted?
  for (ciKeyEvent ik = k->cbegin(); ik != k->cend(); ++ik) {
    const KeyEvent& ev = ik->second;
    const unsigned int tick = ev.tick;
    if (tick < startPos )
      continue;
    operations.push_back(UndoOp(UndoOp::DeleteKey, tick, ev.key, (int)ev.minor));
  }
  // What needs to be added?
  for (ciKeyEvent ik = k->cbegin(); ik != k->cend(); ++ik) {
    const KeyEvent& ev = ik->second;
    const unsigned int tick = ev.tick;
    if (tick < startPos )
      continue;
    if (!is_cut || tick >= startPos - diff)
      operations.push_back(UndoOp(UndoOp::AddKey, tick + diff, ev.key, (int)ev.minor));
  }

  // tempo
  // It is important that the order of operations always be delete followed by add...
  // What needs to be deleted?
  for (ciTEvent it = t->cbegin(); it != t->cend(); ++it) {
    const TEvent* ev = (TEvent*)it->second;
    const unsigned int tick = ev->tick;
    if (tick < startPos )
      continue;
    operations.push_back(UndoOp(UndoOp::DeleteTempo,tick, ev->tempo));
  }
  // What needs to be added?
  for (ciTEvent it = t->cbegin(); it != t->cend(); ++it) {
    const TEvent* ev = (TEvent*)it->second;
    const unsigned int tick = ev->tick;
    if (tick < startPos )
      continue;
    if (!is_cut || tick >= startPos - diff)
      operations.push_back(UndoOp(UndoOp::AddTempo, tick + diff, ev->tempo));
  }

  // sig
  // FIXME: sig still has some issues upon undo. Possibly due to the special raster snap applied to sig?
  // It is important that the order of operations always be delete followed by add...
  // What needs to be deleted?
  for (ciSigEvent is = s->cbegin(); is != s->cend(); ++is) {
    const SigEvent* ev = (SigEvent*)is->second;
    const unsigned int tick = ev->tick;
    if (tick < startPos )
      continue;
    operations.push_back(UndoOp(UndoOp::DeleteSig,tick, ev->sig.z, ev->sig.n));
  }
  // What needs to be added?
  for (ciSigEvent is = s->cbegin(); is != s->cend(); ++is) {
    const SigEvent* ev = (SigEvent*)is->second;
    const unsigned int tick = ev->tick;
    if (tick < startPos )
      continue;
    if (!is_cut || tick >= startPos - diff)
      operations.push_back(UndoOp(UndoOp::AddSig, tick + diff, ev->sig.z, ev->sig.n));
  }

  // markers
  // What needs to be deleted?
  for(ciMarker i = m->cbegin(); i != m->cend(); ++i)
  {
    const Marker& ev = i->second;
    unsigned int tick = ev.tick();
    if (tick < startPos )
      continue;
    if (is_cut && tick < startPos - diff) { // diff is negative, these ticks should be removed
      operations.push_back(UndoOp(UndoOp::DeleteMarker, ev));
    }
  }
  // What needs to be added or modified?
  for(ciMarker i = m->cbegin(); i != m->cend(); ++i)
  {
    const Marker& ev = i->second;
    unsigned int tick = ev.tick();
    if (tick < startPos )
      continue;
    if (!is_cut || tick >= startPos - diff)
    {
      // Grab a copy but with a new ID.
      Marker newMarker = ev.copy();
      newMarker.setTick(tick + diff);
      operations.push_back(UndoOp(UndoOp::ModifyMarker, ev, newMarker));
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
      unsigned int lpos = MusEGlobal::song->lpos();
      unsigned int rpos = MusEGlobal::song->rpos();
      if (lpos >= rpos)
            return;

      Undo operations;

      const unsigned int diff = lpos > rpos ? lpos - rpos : rpos - lpos;
      adjustGlobalLists(operations, lpos > rpos ? rpos : lpos, -diff); // diff is negative meaning cut
      // Splitting wave parts requires the tempo list be done and EXECUTED beforehand.
      // FIXME: This and the part splitting results in two separate operations, user must press undo twice to undo them.
      //        Find a way to combine undo ops while leaving them as separate operations?
      MusEGlobal::song->applyOperationGroup(operations);
      operations.clear();

      TrackList* tracks = MusEGlobal::song->tracks();
      
      for (iTrack it = tracks->begin(); it != tracks->end(); ++it) {
            Track* track = *it;
            if (track == 0 || (onlySelectedTracks && !track->selected()))
                  continue;
            PartList* pl = track->parts();
            for (iPart p = pl->begin(); p != pl->end(); ++p) {
                  Part* part = p->second;
                  unsigned int t = part->tick();
                  unsigned int l = part->lenTick();
                  if (t + l <= lpos)
                        continue;
                  if ((t >= lpos) && ((t+l) <= rpos)) {
                        operations.push_back(UndoOp(UndoOp::DeletePart,part));
                        }
                  else if ((t < lpos) && ((t+l) > lpos) && ((t+l) <= rpos)) {
                      // remove part tail
                      unsigned int len = lpos - t;
                      
                      if (part->nextClone()==part) // no clones
                      {
                            // cut Events
                            const EventList& el = part->events();
                            for (ciEvent ie = el.lower_bound(len); ie != el.end(); ++ie)
                                    operations.push_back(UndoOp(UndoOp::DeleteEvent,ie->second, part, false, false));
                      }
                      // TODO FIXME Suspect this section may need a wee bit more rework with the events above...
                      operations.push_back(UndoOp(UndoOp::ModifyPartLength, part, part->lenValue(), len, Pos::TICKS));  
                  }
                  else if ((t < lpos) && ((t+l) > lpos) && ((t+l) > rpos)) {
                        //----------------------
                        // remove part middle
                        //----------------------
                        Part* p1;
                        Part* p2;
                        Part* p3;
                        part->splitPart(lpos, p1, p2);
                        delete p2;
                        part->splitPart(rpos, p2, p3);
                        delete p2;
                        p3->setTick(lpos);

                        MusEGlobal::song->informAboutNewParts(part,p1,p3);
                        operations.push_back(UndoOp(UndoOp::DeletePart,part));
                        operations.push_back(UndoOp(UndoOp::AddPart,p1));
                        operations.push_back(UndoOp(UndoOp::AddPart,p3));
                        }
                  else if ((t >= lpos) && (t < rpos) && (t+l) > rpos) {
                        // remove part head
                        
                        Part* p1;
                        Part* p2;
                        part->splitPart(rpos, p1, p2);
                        delete p1;
                        p2->setTick(lpos);
                        
                        MusEGlobal::song->informAboutNewParts(part,p2);
                        operations.push_back(UndoOp(UndoOp::DeletePart,part));
                        operations.push_back(UndoOp(UndoOp::AddPart,p2));
                        }
                  else if (t >= rpos) {
                        // move part to the left
                        unsigned int nt = part->tick();
                        if(nt > (rpos - lpos))
                          operations.push_back(UndoOp(UndoOp::MovePart, part, part->posValue(), nt - (rpos -lpos), Pos::TICKS ));
                        }
                  }

                  adjustAutomation(operations, track, lpos, rpos, cutOperation);

            }

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
      Undo operations=movePartsTotheRight(
        MusEGlobal::song->lpos() <= MusEGlobal::song->rpos() ? MusEGlobal::song->lpos() : MusEGlobal::song->rpos(),
        MusEGlobal::song->lpos() <= MusEGlobal::song->rpos() ?
          MusEGlobal::song->rpos() - MusEGlobal::song->lpos() : MusEGlobal::song->lpos() - MusEGlobal::song->rpos(),
        onlySelectedTracks);
      MusEGlobal::song->applyOperationGroup(operations);
}

void adjustAutomation(Undo &operations, Track *track, unsigned int lpos, unsigned int rpos, OperationType type)
{
  if (!track->isMidiTrack())
  {
    auto *audioTrack = static_cast<AudioTrack *>(track);
    auto controllerListList = audioTrack->controller();

    auto lFrame = MusEGlobal::tempomap.tick2frame(lpos);
    auto rFrame = MusEGlobal::tempomap.tick2frame(rpos);

    // iterate through all indexes in map (all control types)
    for (auto controllerList : *controllerListList)
    {
      // The Undo system will take 'ownership' of these and delete them at the appropriate time. (comment taken from other code)
      CtrlList* removedEvents = new CtrlList(*controllerList.second, CtrlList::ASSIGN_PROPERTIES);
      CtrlList* readdedEvents = new CtrlList(*controllerList.second, CtrlList::ASSIGN_PROPERTIES);

      for (auto controller : *controllerList.second)
      {
        // iterate through all events and see if any appear after lpos
        if (controller.first > lFrame)
        {
          removedEvents->add(controller.second.frame, controller.second.val);

          if (type == cutOperation)
          {
            if (controller.first > rFrame)
            {
              auto diff = rFrame - lFrame;
              auto newFramePos = controller.second.frame - diff;
              readdedEvents->add(newFramePos, controller.second.val);
            }
          }
          else if (type == insertOperation)
          {
            auto diff = rFrame - lFrame;
            auto newFramePos = controller.second.frame + diff;
            readdedEvents->add(newFramePos, controller.second.val);
          }
        }
      }

      if(removedEvents->empty() && readdedEvents->empty())
      {
        delete removedEvents;
        delete readdedEvents;
      }
      else
      {
        auto undoOp = UndoOp( UndoOp::ModifyAudioCtrlValList, controllerListList, removedEvents, readdedEvents);
        operations.push_back(undoOp);
      }
    }
  }
}

Undo movePartsTotheRight(unsigned int startTicks, unsigned int moveTicks, bool only_selected, set<Track*>* tracklist)
{
      Undo operations;
      adjustGlobalLists(operations, startTicks, moveTicks);
      // Splitting wave parts requires the tempo list be done and EXECUTED beforehand.
      // FIXME: This and the part splitting results in two separate operations, user must press undo twice to undo them.
      //        Find a way to combine undo ops while leaving them as separate operations?
      MusEGlobal::song->applyOperationGroup(operations);
      operations.clear();

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
                  unsigned int l = part->lenTick();
                  if (t + l <= startTicks)
                        continue;
                  if (startTicks > t && startTicks < (t+l)) {
                        // split part to insert new space
                        Part* p1;
                        Part* p2;
                        part->splitPart(startTicks, p1, p2);
                        p2->setTick(startTicks+moveTicks);

                        MusEGlobal::song->informAboutNewParts(part,p1,p2);
                        operations.push_back(UndoOp(UndoOp::DeletePart, part));
                        operations.push_back(UndoOp(UndoOp::AddPart, p1));
                        operations.push_back(UndoOp(UndoOp::AddPart, p2));
                        }
                  else if (t >= startTicks) {
                        operations.push_back(UndoOp(UndoOp::MovePart, part, part->posValue(), t + moveTicks, Pos::TICKS));
                        }
                  }
            adjustAutomation(operations, track, MusEGlobal::song->lpos(), MusEGlobal::song->rpos(), insertOperation);
            }

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
                    part->splitPart(pos, p1, p2);

                    MusEGlobal::song->informAboutNewParts(part, p1);
                    MusEGlobal::song->informAboutNewParts(part, p2);
                    operations.push_back(UndoOp(UndoOp::DeletePart,part));
                    operations.push_back(UndoOp(UndoOp::AddPart,p1));
                    operations.push_back(UndoOp(UndoOp::AddPart,p2));
                    break;
              }
        }
    }
    return operations;
}


} // namespace MusECore
