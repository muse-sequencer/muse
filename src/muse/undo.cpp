//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: undo.cpp,v 1.12.2.9 2009/05/24 21:43:44 terminator356 Exp $
//
//  (C) Copyright 1999/2000 Werner Schweer (ws@seh.de)
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

#include "assert.h"

#include "sig.h"  
#include "keyevent.h"

#include "undo.h"
#include "song.h"
#include "globals.h"
#include "audio.h"  
#include "midiport.h"
#include "operations.h"
#include "tempo.h"
#include "audiodev.h"
#include "wave_helper.h"
#include "gconfig.h"
#include "al/al.h"

#include <set>

// Forwards from header:
#include "track.h"
#include "part.h"

// Enable for debugging:
//#define _UNDO_DEBUG_

namespace MusECore {

// iundo points to last Undo() in Undo-list

static bool undoMode = false;  // for debugging

std::list<QString> temporaryWavFiles;


struct UndoAudioCtrlTrackMapItem
{
  CtrlListList _erased;
  CtrlListList _doNotErase;
};

class UndoAudioCtrlTrackMap : public std::map<Track*, UndoAudioCtrlTrackMapItem, std::less<Track* >>
{
  public:
    // Returns true if insertion took place.
    bool add(Track* track, const UndoAudioCtrlTrackMapItem& item);
};
typedef UndoAudioCtrlTrackMap::iterator iUndoAudioCtrlTrackMap;
typedef UndoAudioCtrlTrackMap::const_iterator ciUndoAudioCtrlTrackMap;
typedef std::pair<Track*, UndoAudioCtrlTrackMapItem> UndoAudioCtrlTrackMapInsertPair;
typedef std::pair<iUndoAudioCtrlTrackMap, bool> UndoAudioCtrlTrackMapInsertResult;

bool UndoAudioCtrlTrackMap::add(Track* track, const UndoAudioCtrlTrackMapItem& item)
{
  return insert(UndoAudioCtrlTrackMapInsertPair(track, item)).second;
}

bool Song::audioCtrlMoveModeBegun() const { return _audioCtrlMoveModeBegun; }

void Song::beginAudioCtrlMoveMode(Undo& operations) const
{
  if(_audioCtrlMoveModeBegun == false)
    operations.push_back(UndoOp(UndoOp::BeginAudioCtrlMoveMode));
}

void Song::endAudioCtrlMoveMode(Undo& operations) const
{
  if(_audioCtrlMoveModeBegun == true)
    operations.push_back(UndoOp(UndoOp::EndAudioCtrlMoveMode));
}

bool Song::audioCtrlMoveEnd(PendingOperationList& operations)
{
  bool ret = false;
  for(ciTrack it = tracks()->cbegin(); it != tracks()->cend(); ++it)
  {
    if((*it)->isMidiTrack())
      continue;
    AudioTrack* at = static_cast<AudioTrack*>(*it);
    CtrlListList* ecll = at->erasedController();
    CtrlListList* ncll = at->noEraseController();
    if(!ecll->empty())
    {
      // Swapping with a blank container is faster than clearing it.
      CtrlListList* new_cll = new CtrlListList();
      operations.add(PendingOperationItem(
        ecll, new_cll, PendingOperationItem::ModifyAudioCtrlValListList));
      ret = true;
    }
    if(!ncll->empty())
    {
      // Swapping with a blank container is faster than clearing it.
      CtrlListList* new_cll = new CtrlListList();
      operations.add(PendingOperationItem(
        ncll, new_cll, PendingOperationItem::ModifyAudioCtrlValListList));
      ret = true;
    }
  }
  return ret;
}

bool Song::undoAudioCtrlMoveEnd(PendingOperationList& operations)
{
  // NOTE: Both the erased and do not erase lists should be cleared by now if all went well.
  //       Caller should be examining an EndAudioCtrlMoveMode command in the undo list
  //        and since that clears the lists, they should be clear right now.

  UndoAudioCtrlTrackMap tm;
  bool ret = false;

  //--------------------------------------------------------------------
  // Look backwards through the undo list for a BeginAudioCtrlMoveMode,
  //  starting with the latest entry.
  //--------------------------------------------------------------------

  for(MusECore::criUndo iub = undoList->crbegin(); iub != undoList->crend(); ++iub)
  {
    const MusECore::Undo& undo = *iub;
    for(MusECore::criUndoOp iuoB = undo.crbegin(); iuoB != undo.crend(); ++iuoB)
    {
      const MusECore::UndoOp& undoOpB = *iuoB;

      // Looking for this previous specific command - before the one that the caller is currently examining.
      // If nothing is eventually found, one possibility is just take the very first item, but instead we
      //  should give up and just return an error rather than risk accumulating unwanted changes.
      if(undoOpB.type != MusECore::UndoOp::BeginAudioCtrlMoveMode)
        continue;


      //---------------------------------------------------------------
      // Found a BeginAudioCtrlMoveMode. Now go forward from there and
      //  reconstruct the various tracks' erased and no-erase ctrl lists.
      //---------------------------------------------------------------

      const MusECore::Undo* undoF;
      MusECore::ciUndo iuF;
      MusECore::ciUndoOp iuoF;
      bool is_first = true;
      while(is_first || iuF != undoList->cend())
      {
        if(is_first)
        {
          undoF = &undo;
          // Using base() returns a forward iterator AFTER the reverse iterator, which is exactly what we want.
          iuoF = iuoB.base();
        }
        else
        {
          undoF = &(*iuF);
          iuoF = undoF->cbegin();
        }

        for( ; iuoF != undoF->cend(); ++iuoF)
        {
          const MusECore::UndoOp& undoOpF = *iuoF;

          // Looking for this specific operation.
          if(undoOpF.type != MusECore::UndoOp::ModifyAudioCtrlValList)
            continue;

          // Only if one of these lists has something.
          if((!undoOpF._recoverableEraseCtrlList || undoOpF._recoverableEraseCtrlList->empty()) &&
            (!undoOpF._recoverableAddCtrlList || undoOpF._recoverableAddCtrlList->empty()) &&
            (!undoOpF._doNotEraseCtrlList || undoOpF._doNotEraseCtrlList->empty()))
            continue;

          Track* tr = const_cast<Track*>(undoOpF.track);
          if(tr->isMidiTrack())
            continue;

          UndoAudioCtrlTrackMapInsertResult tm_ires = tm.insert(UndoAudioCtrlTrackMapInsertPair(tr, UndoAudioCtrlTrackMapItem()));
          CtrlListList* erasedll = &tm_ires.first->second._erased;
          CtrlListList* doNotErasell = &tm_ires.first->second._doNotErase;

          CtrlList* rel = undoOpF._recoverableEraseCtrlList;
          if(rel && !rel->empty())
          {
            CtrlList* cl;
            iCtrlList icl = erasedll->find(rel->id());
            if(icl == erasedll->end())
            {
              cl = new CtrlList(*rel, CtrlList::ASSIGN_PROPERTIES);
              erasedll->add(cl);
            }
            else
            {
              cl = icl->second;
            }
            // Add any items in the recoverable erase list.
            for(ciCtrl ic = rel->cbegin(); ic != rel->cend(); ++ic)
              cl->insert(CtrlListInsertPair_t(ic->first, ic->second));
          }

          CtrlList* ral = undoOpF._recoverableAddCtrlList;
          if(ral && !ral->empty())
          {
            iCtrlList icl = erasedll->find(ral->id());
            if(icl != erasedll->end())
            {
              CtrlList* cl = icl->second;
              // Erase any items in the new list found at the frames given by the recoverable add list.
              for(ciCtrl ic = ral->cbegin(); ic != ral->cend(); ++ic)
                cl->erase(ic->first);
              // Nothing left? Delete and erase the list.
              if(cl->empty())
              {
                delete cl;
                erasedll->erase(icl);
              }
            }
          }

          CtrlList* nel = undoOpF._doNotEraseCtrlList;
          if(nel && !nel->empty())
          {
            CtrlList* cl;
            iCtrlList icl = doNotErasell->find(nel->id());
            if(icl == doNotErasell->end())
            {
              cl = new CtrlList(*nel, CtrlList::ASSIGN_PROPERTIES);
              doNotErasell->add(cl);
            }
            else
            {
              cl = icl->second;
            }
            // Add any items in the do not erase list.
            for(ciCtrl ic = nel->cbegin(); ic != nel->cend(); ++ic)
              cl->insert(CtrlListInsertPair_t(ic->first, ic->second));
          }
        }

        if(is_first)
        {
          is_first = false;
          // Using base() returns a forward iterator AFTER the reverse iterator.
          iuF = iub.base();
        }
        else
        {
          ++iuF;
        }
      }

      //---------------------------------------------------
      // Now that we have a list of things to reconstruct,
      //  compose the operation commands to do it.
      //---------------------------------------------------

      for(ciUndoAudioCtrlTrackMap itm = tm.cbegin(); itm != tm.cend(); ++itm)
      {
        if(itm->first->isMidiTrack())
          continue;
        AudioTrack* atrack = static_cast<AudioTrack*>(itm->first);
        const UndoAudioCtrlTrackMapItem& tmi = itm->second;
        {
          CtrlListList* track_erasedll = atrack->erasedController();
          const CtrlListList& map_erasedll = tmi._erased;
          for(ciCtrlList icl = map_erasedll.cbegin(); icl != map_erasedll.cend(); ++icl)
          {
            CtrlList* cl = icl->second;
            const int id = cl->id();
            const iCtrlList track_icl = track_erasedll->find(id);
            if(track_icl == track_erasedll->cend())
              operations.add(PendingOperationItem(
                track_erasedll, cl, PendingOperationItem::AddAudioCtrlValList));
            else
              operations.add(PendingOperationItem(
                track_icl, cl, PendingOperationItem::ModifyAudioCtrlValList));
            ret = true;
          }
        }

        {
          CtrlListList* track_noerasell = atrack->noEraseController();
          const CtrlListList& map_noerasell = tmi._doNotErase;
          for(ciCtrlList icl = map_noerasell.cbegin(); icl != map_noerasell.cend(); ++icl)
          {
            CtrlList* cl = icl->second;
            const int id = cl->id();
            const iCtrlList track_icl = track_noerasell->find(id);
            if(track_icl == track_noerasell->cend())
              operations.add(PendingOperationItem(
                track_noerasell, cl, PendingOperationItem::AddAudioCtrlValList));
            else
              operations.add(PendingOperationItem(
                track_icl, cl, PendingOperationItem::ModifyAudioCtrlValList));
            ret = true;
          }
        }
      }

      // And we're outta here.
      return ret;
    }
  }

  // Nothing was found. This is actually an error, if the caller did things correctly.
  fprintf(stderr, "Error: Song::undoAudioCtrlMoveEnd: BeginAudioCtrlMoveMode not found\n");
  return false;
}

//---------------------------------------------------------
//   typeName
//---------------------------------------------------------

const char* UndoOp::typeName()
      {
      static const char* name[] = {
            "AddRoute", "DeleteRoute", 
            "AddTrack", "DeleteTrack", 
            "AddPart",  "DeletePart", "MovePart", "ModifyPartStart", "ModifyPartLength", "ModifyPartName", "SelectPart",
            "AddEvent", "DeleteEvent", "ModifyEvent", "SelectEvent",
            "AddAudioCtrlVal", "AddAudioCtrlValStruct",
            "DeleteAudioCtrlVal", "ModifyAudioCtrlVal", "ModifyAudioCtrlValList",
            "SelectAudioCtrlVal", "SetAudioCtrlPasteEraseMode", "BeginAudioCtrlMoveMode", "EndAudioCtrlMoveMode", /*"SetAudioCtrlMoveMode",*/
            "AddTempo", "DeleteTempo", "ModifyTempo", "SetTempo", "SetStaticTempo", "SetGlobalTempo", "EnableMasterTrack",
            "AddSig",   "DeleteSig",   "ModifySig",
            "AddKey",   "DeleteKey",   "ModifyKey",
            "ModifyTrackName", "ModifyTrackChannel",
            "SetTrackRecord", "SetTrackMute", "SetTrackSolo", "SetTrackRecMonitor", "SetTrackOff",
            "MoveTrack",
            "ModifyClip", "AddMarker", "DeleteMarker", "ModifyMarker", "SetMarkerPos",
            "ModifySongLen", "SetInstrument", "DoNothing",
            "ModifyMidiDivision",
            "EnableAllAudioControllers",
            "GlobalSelectAllEvents"
            };
      return name[type];
      }

//---------------------------------------------------------
//   dump
//---------------------------------------------------------

void UndoOp::dump()
      {
      printf("UndoOp: %s\n   ", typeName());
      switch(type) {
            case AddTrack:
            case DeleteTrack:
                  printf("%d %s\n", trackno, track->name().toLatin1().constData());
                  break;
            case AddEvent:
            case DeleteEvent:
                  printf("old event:\n");
                  oEvent.dump(5);
                  printf("   new event:\n");
                  nEvent.dump(5);
                  printf("   Part:\n");
                  if (part)
                        part->dump(5);
                  break;
            case ModifyTrackName:
                  printf("<%s>-<%s>\n", _oldName->toLocal8Bit().data(), _newName->toLocal8Bit().data());
                  break;
            case ModifyPartName:
                  printf("<%s>-<%s>\n", _oldName->toLocal8Bit().data(), _newName->toLocal8Bit().data());
                  break;
            case ModifyTrackChannel:
                  printf("%s <%d>-<%d>\n", track->name().toLatin1().constData(), _oldPropValue, _newPropValue);
                  break;
            case SetTrackRecord:
                  printf("%s %d\n", track->name().toLatin1().constData(), a);
                  break;
            case SetTrackMute:
                  printf("%s %d\n", track->name().toLatin1().constData(), a);
                  break;
            case SetTrackSolo:
                  printf("%s %d\n", track->name().toLatin1().constData(), a);
                  break;
            case SetTrackRecMonitor:
                  printf("%s %d\n", track->name().toLatin1().constData(), a);
                  break;
            case SetTrackOff:
                  printf("%s %d\n", track->name().toLatin1().constData(), a);
                  break;
            default:      
                  break;
            }
      }

//---------------------------------------------------------
//    deleteUndoOp
//---------------------------------------------------------

void deleteUndoOp(UndoOp& op, bool doUndos = true, bool doRedos = true)
{
  //====================================================================
  // NOTE: Whether or not to delete operations may depend on whether
  //        they are contained in an undo or a redo list.
  //       For example, a DeletePart UNDO operation contains a part pointer
  //        that was removed from a part list. The part is isolated and is
  //        hanging around in the undo list waiting to be re-added.
  //       It is SAFE to delete this part pointer.
  //       But an AddPart UNDO operation contains a part pointer that was
  //        added to a part list. The part is used by a song.
  //       It is NOT SAFE to delete this part pointer.
  //       Closing cleanup routines will take care of it.
  //       Similar (but reversed) rules apply to REDO lists.
  //
  //       Meanwhile, UNDO operations AddMarker, DeleteMarker and so on
  //        contain pointers that are 'local' to the undo structure.
  //       They are copies of data, and usually deleted at operation end.
  //       It is SAFE to delete these pointers.
  //       They do not care if they are contained in an UNDO or a REDO list.
  //====================================================================

  switch(op.type)
  {
    case UndoOp::DeleteTrack:
          if(op.track && doUndos)
          {
            delete const_cast<Track*>(op.track);
            op.track = nullptr;
          }
          break;

    case UndoOp::DeletePart:
          if(op.part && doUndos)
          {
            delete const_cast<Part*>(op.part);
            op.part = nullptr;
          }
          break;

    case UndoOp::AddTrack:
          if(op.track && doRedos)
          {
            delete const_cast<Track*>(op.track);
            op.track = nullptr;
          }
          break;

    case UndoOp::AddPart:
          if(op.part && doRedos)
          {
            delete const_cast<Part*>(op.part);
            op.part = nullptr;
          }
          break;

    case UndoOp::ModifyMarker:
    case UndoOp::SetMarkerPos:
    case UndoOp::AddMarker:
    case UndoOp::DeleteMarker:
          if (op.oldMarker)
          {
            delete op.oldMarker;
            op.oldMarker = nullptr;
          }
          if (op.newMarker)
          {
            delete op.newMarker;
            op.newMarker = nullptr;
          }
          break;

    case UndoOp::ModifyPartName:
    case UndoOp::ModifyTrackName:
          if (op._oldName)
          {
            delete op._oldName;
            op._oldName = nullptr;
          }
          if (op._newName)
          {
            delete op._newName;
            op._newName = nullptr;
          }
          break;

    case UndoOp::ModifyAudioCtrlValList:
          if(op._eraseCtrlList)
          {
            delete op._eraseCtrlList;
            op._eraseCtrlList = nullptr;
          }
          if(op._addCtrlList)
          {
            delete op._addCtrlList;
            op._addCtrlList = nullptr;
          }
          if(op._recoverableEraseCtrlList)
          {
            delete op._recoverableEraseCtrlList;
            op._recoverableEraseCtrlList = nullptr;
          }
          if(op._recoverableAddCtrlList)
          {
            delete op._recoverableAddCtrlList;
            op._recoverableAddCtrlList = nullptr;
          }
          if(op._doNotEraseCtrlList)
          {
            delete op._doNotEraseCtrlList;
            op._doNotEraseCtrlList = nullptr;
          }
          break;

    case UndoOp::AddRoute:
    case UndoOp::DeleteRoute:
          if (op.routeFrom)
          {
            delete op.routeFrom;
            op.routeFrom = nullptr;
          }
          if (op.routeTo)
          {
            delete op.routeTo;
            op.routeTo = nullptr;
          }
          break;

    case UndoOp::AddAudioCtrlValStruct:
          if (op._audioCtrlValStruct)
          {
            delete op._audioCtrlValStruct;
            op._audioCtrlValStruct = nullptr;
          }
          break;

    default:
          break;
  }
}

Undo::iterator Undo::deleteAndErase(Undo::iterator iuo)
{
  deleteUndoOp(*iuo);
  return erase(iuo);
}

//---------------------------------------------------------
//    clearDelete
//---------------------------------------------------------

void UndoList::clearDelete()
{
  if(!empty())
  {
    if (this->isUndo)
    {
      for(iUndo iu = begin(); iu != end(); ++iu)
      {
        Undo& u = *iu;
        for(iUndoOp i = u.begin(); i != u.end(); ++i)
          deleteUndoOp(*i, true, false);
        u.clear();
      }
    }
    else
    {
      for(riUndo iu = rbegin(); iu != rend(); ++iu)
      {
        Undo& u = *iu;
        for(riUndoOp i = u.rbegin(); i != u.rend(); ++i)
          deleteUndoOp(*i, false, true);
        u.clear();
      }
    }
  }

  clear();
}

//---------------------------------------------------------
//    startUndo
//---------------------------------------------------------

void Song::startUndo(void* sender)
      {
      redoList->clearDelete(); // redo must be invalidated when a new undo is started
      MusEGlobal::redoAction->setEnabled(false);
      setUndoRedoText();
      
      undoList->push_back(Undo());
      updateFlags = SongChangedStruct_t(0, 0, sender);
      undoMode = true;
      }

//---------------------------------------------------------
//   endUndo
//---------------------------------------------------------

void Song::endUndo(SongChangedStruct_t flags)
      {
      // It is possible the current list may be empty after our optimizations during appending 
      //  of given operations to the current list. (Or if no operations were pushed between startUndo and endUndo).
      // Get rid of an empty current list now.
      if(undoList->back().empty())
        undoList->pop_back();
      else 
      {
        riUndo prev_undo = undoList->rbegin();
        prev_undo++;
        if (prev_undo!=undoList->rend())
        {
              // try to merge the current Undo with the last one
              if (prev_undo->merge_combo(undoList->back()))
                    undoList->pop_back();
        }
      }
      
      // Even if the current list was empty, or emptied during appending of given operations to the current list, 
      //  the given operations were executed so we still need to inform that something may have changed.
      
      updateFlags |= flags;
      endMsgCmd();
      undoMode = false;
      }

//---------------------------------------------------------
//   setUndoRedoText
//---------------------------------------------------------

void Song::setUndoRedoText()
{
  if(MusEGlobal::undoAction)
  {
    QString s = tr("Und&o");
    if(MusEGlobal::undoAction->isEnabled())
    {
      if(!undoList->empty() && !undoList->back().empty())
      {
        int sz = undoList->back().size();
        //if(sz >= 2)
        //  s += QString(" (%1)").arg(sz);
        s += QString(" ") + undoList->back().front().typeName();
        if(sz >= 2)
          s += ", ..";  // Hm, the tooltip will not show three dots "..."
      }
    }
    MusEGlobal::undoAction->setText(s);
  }
  
  if(MusEGlobal::redoAction)
  {
    QString s = tr("Re&do");
    if(MusEGlobal::redoAction->isEnabled())
    {
      if(!redoList->empty() && !redoList->back().empty())
      {
        int sz = redoList->back().size();
        //if(sz >= 2)
        //  s += QString(" (%1)").arg(sz);
        s += QString(" ") + redoList->back().front().typeName();
        if(sz >= 2)
          s += ", ..";
      }
    }
    MusEGlobal::redoAction->setText(s);
  }
}

void Undo::push_front(const UndoOp& op)
{
  insert(begin(), op);
}

void Undo::push_back(const UndoOp& op)
{
  insert(end(), op);
}

void Undo::insert(Undo::iterator position, Undo::const_iterator first, Undo::const_iterator last)
{
  for(Undo::const_iterator iuo = first; iuo != last; ++iuo)
    insert(position, *iuo);
}

void Undo::insert(Undo::iterator position, Undo::size_type n, const UndoOp& op)
{
  for(Undo::size_type i = 0; i != n; ++i)
    insert(position, op);
}

void Undo::insert(Undo::iterator position, const UndoOp& op)
{
  UndoOp n_op = op;

#ifdef _UNDO_DEBUG_
  switch(n_op.type)
  {
    case UndoOp::AddRoute:
      fprintf(stderr, "Undo::insert: AddRoute\n");
    break;
    case UndoOp::DeleteRoute:
      fprintf(stderr, "Undo::insert: DeleteRoute\n");
    break;

    
    case UndoOp::AddTrack:
      fprintf(stderr, "Undo::insert: AddTrack\n");
    break;
    case UndoOp::DeleteTrack:
      fprintf(stderr, "Undo::insert: DeleteTrack\n");
    break;
    case UndoOp::MoveTrack:
      fprintf(stderr, "Undo::insert: MoveTrack\n");
    break;
    case UndoOp::ModifyTrackName:
      fprintf(stderr, "Undo::insert: ModifyTrackName\n");
    break;
    case UndoOp::ModifyTrackChannel:
      fprintf(stderr, "Undo::insert: ModifyTrackChannel\n");
    break;
    case UndoOp::SetTrackRecord:
      fprintf(stderr, "Undo::insert: SetTrackRecord\n");
    break;
    case UndoOp::SetTrackMute:
      fprintf(stderr, "Undo::insert: SetTrackMute\n");
    break;
    case UndoOp::SetTrackSolo:
      fprintf(stderr, "Undo::insert: SetTrackSolo\n");
    break;
    case UndoOp::SetTrackRecMonitor:
      fprintf(stderr, "Undo::insert: SetTrackRecMonitor\n");
    break;
    case UndoOp::SetTrackOff:
      fprintf(stderr, "Undo::insert: SetTrackOff\n");
    break;
    
    
    case UndoOp::AddPart:
      fprintf(stderr, "Undo::insert: AddPart\n");
    break;
    case UndoOp::DeletePart:
      fprintf(stderr, "Undo::insert: DeletePart\n");
    break;
    case UndoOp::MovePart:
      fprintf(stderr, "Undo::insert: MovePart\n");
    break;
    case UndoOp::SelectPart:
      fprintf(stderr, "Undo::insert: SelectPart\n");
    break;
    case UndoOp::ModifyPartName:
      fprintf(stderr, "Undo::insert: ModifyPartName\n");
    break;
    case UndoOp::ModifyPartStart:
      fprintf(stderr, "Undo::insert: ModifyPartStart\n");
    break;
    case UndoOp::ModifyPartLength:
      fprintf(stderr, "Undo::insert: ModifyPartLength\n");
    break;
    
    
    case UndoOp::AddEvent:
      fprintf(stderr, "Undo::insert: AddEvent\n");
    break;
    case UndoOp::DeleteEvent:
      fprintf(stderr, "Undo::insert: DeleteEvent\n");
    break;
    case UndoOp::ModifyEvent:
      fprintf(stderr, "Undo::insert: ModifyEvent\n");
    break;
    case UndoOp::SelectEvent:
      fprintf(stderr, "Undo::insert: SelectEvent\n");
    break;
    
    
    case UndoOp::AddAudioCtrlVal:
      fprintf(stderr, "Undo::insert: AddAudioCtrlVal\n");
    break;
    case UndoOp::AddAudioCtrlValStruct:
      fprintf(stderr, "Undo::insert: AddAudioCtrlValStruct\n");
    break;
    case UndoOp::DeleteAudioCtrlVal:
      fprintf(stderr, "Undo::insert: DeleteAudioCtrlVal\n");
    break;
    case UndoOp::ModifyAudioCtrlVal:
      fprintf(stderr, "Undo::insert: ModifyAudioCtrlVal\n");
    break;
    case UndoOp::ModifyAudioCtrlValList:
      fprintf(stderr, "Undo::insert: ModifyAudioCtrlValList\n");
    break;
    case UndoOp::SelectAudioCtrlVal:
      fprintf(stderr, "Undo::insert: SelectAudioCtrlVal\n");
    break;

    
    case UndoOp::AddTempo:
      fprintf(stderr, "Undo::insert: AddTempo tempo:%d tick:%d\n", n_op.b, n_op.a);
    break;
    case UndoOp::DeleteTempo:
      fprintf(stderr, "Undo::insert: DeleteTempo old val:%d tick:%d\n", n_op.b, n_op.a);
    break;
    case UndoOp::ModifyTempo:
      fprintf(stderr, "Undo::insert: ModifyTempo old:%d new:%d tick:%d\n", n_op.b, n_op.c, n_op.a);
    break;
    case UndoOp::SetTempo:
      fprintf(stderr, "Undo::insert: SetTempo tempo:%d tick:%d\n", n_op.b, n_op.a);
    break;
    case UndoOp::SetStaticTempo:
      fprintf(stderr, "Undo::insert: SetStaticTempo\n");
    break;
    case UndoOp::SetGlobalTempo:
      fprintf(stderr, "Undo::insert: SetGlobalTempo\n");
    break;
    case UndoOp::EnableMasterTrack:
      fprintf(stderr, "Undo::insert: EnableMasterTrack\n");
    break;
    
    
    case UndoOp::AddSig:
      fprintf(stderr, "Undo::insert: AddSig\n");
    break;
    case UndoOp::DeleteSig:
      fprintf(stderr, "Undo::insert: DeleteSig\n");
    break;
    case UndoOp::ModifySig:
      fprintf(stderr, "Undo::insert: ModifySig\n");
    break;
    
    
    case UndoOp::AddKey:
      fprintf(stderr, "Undo::insert: AddKey\n");
    break;
    case UndoOp::DeleteKey:
      fprintf(stderr, "Undo::insert: DeleteKey\n");
    break;
    case UndoOp::ModifyKey:
      fprintf(stderr, "Undo::insert: ModifyKey\n");
    break;
    
    
    case UndoOp::ModifyClip:
      fprintf(stderr, "Undo::insert: ModifyClip\n");
    break;
    
    
    case UndoOp::AddMarker:
      fprintf(stderr, "Undo::insert: AddMarker\n");
    break;

    case UndoOp::DeleteMarker:
      fprintf(stderr, "Undo::insert: DeleteMarker\n");
    break;

    case UndoOp::ModifyMarker:
      fprintf(stderr, "Undo::insert: ModifyMarker\n");
    break;

    case UndoOp::SetMarkerPos:
      fprintf(stderr, "Undo::insert: SetMarkerPos\n");
    break;

    
    case UndoOp::ModifySongLen:
      fprintf(stderr, "Undo::insert: ModifySongLen\n");
    break;

    case UndoOp::SetInstrument:
      fprintf(stderr, "Undo::insert: SetInstrument\n");
    break;

    
    case UndoOp::DoNothing:
      fprintf(stderr, "Undo::insert: DoNothing\n");
    break;
    
    case UndoOp::ModifyMidiDivision:
      fprintf(stderr, "Undo::insert: ModifyMidiDivision\n");
    break;
    
    case UndoOp::EnableAllAudioControllers:
      fprintf(stderr, "Undo::insert: EnableAllAudioControllers\n");
    break;
    
    case UndoOp::GlobalSelectAllEvents:
      fprintf(stderr, "Undo::insert: GlobalSelectAllEvents\n");
    break;
    
    default:
    break;
  }
#endif

  // (NOTE: Use this handy speed-up 'if' line to exclude unhandled operation types)
  if(n_op.type != UndoOp::ModifyTrackChannel && n_op.type != UndoOp::ModifyClip && n_op.type != UndoOp::DoNothing) 
  {
    // TODO FIXME: Must look beyond position and optimize in that direction too !
    //for(Undo::iterator iuo = begin(); iuo != position; ++iuo)
    iterator iuo = position;
    while(iuo != begin())
    {
      --iuo;
      UndoOp& uo = *iuo;
      
      switch(n_op.type)
      {
        case UndoOp::AddRoute:
          if(uo.type == UndoOp::AddRoute && *uo.routeFrom == *n_op.routeFrom && *uo.routeTo == *n_op.routeTo)
          {
            fprintf(stderr, "MusE error: Undo::insert(): Double AddRoute. Ignoring.\n");
            // Done with these routes. Be sure to delete them.
            deleteUndoOp(n_op);
            return;
          }
          else if(uo.type == UndoOp::DeleteRoute && *uo.routeFrom == *n_op.routeFrom && *uo.routeTo == *n_op.routeTo)
          {
            // Delete followed by add is useless. Cancel out the delete + add by erasing the delete command.
            // Done with these routes. Be sure to delete them.
            deleteUndoOp(n_op);
            deleteAndErase(iuo);
            return;
          }
        break;
        
        case UndoOp::DeleteRoute:
          if(uo.type == UndoOp::DeleteRoute && *uo.routeFrom == *n_op.routeFrom && *uo.routeTo == *n_op.routeTo)
          {
            fprintf(stderr, "MusE error: Undo::insert(): Double DeleteRoute. Ignoring.\n");
            // Done with these routes. Be sure to delete them.
            deleteUndoOp(n_op);
            return;
          }
          else if(uo.type == UndoOp::AddRoute && *uo.routeFrom == *n_op.routeFrom && *uo.routeTo == *n_op.routeTo)
          {
            // Add followed by delete is useless. Cancel out the add + delete by erasing the add command.
            // Done with these routes. Be sure to delete them.
            deleteUndoOp(n_op);
            deleteAndErase(iuo);
            return;
          }
        break;

        
        case UndoOp::ModifyTrackName:
          if(uo.type == UndoOp::ModifyTrackName && uo.track == n_op.track)  
          {
            // Done with these objects. Be sure to delete them.
            delete n_op._oldName;
            delete uo._newName;
            // Simply replace the existing new name with the newer name.
            uo._newName = n_op._newName;
            return;
          }
        break;
        
        case UndoOp::MoveTrack:
          if(uo.type == UndoOp::MoveTrack && uo.a == n_op.a)
          {
            // Simply replace the 'to track' value.
            uo.b = n_op.b;
            return;
          }
        break;
        
        case UndoOp::SetTrackRecord:
          if(uo.type == UndoOp::SetTrackRecord && uo.track == n_op.track)
          {
            if(uo.a == n_op.a)
            {
              fprintf(stderr, "MusE error: Undo::insert(): Double SetTrackRecord. Ignoring.\n");
              return;
            }
            else
            {
              // On/off followed by off/on is useless. Cancel out the on/off + off/on by erasing the command.
              erase(iuo);
              return;  
            }
          }
        break;
        
        case UndoOp::SetTrackMute:
          if(uo.type == UndoOp::SetTrackMute && uo.track == n_op.track)
          {
            if(uo.a == n_op.a)
            {
              fprintf(stderr, "MusE error: Undo::insert(): Double SetTrackMute. Ignoring.\n");
              return;
            }
            else
            {
              // On/off followed by off/on is useless. Cancel out the on/off + off/on by erasing the command.
              erase(iuo);
              return;  
            }
          }
        break;
        
        case UndoOp::SetTrackSolo:
          if(uo.type == UndoOp::SetTrackSolo && uo.track == n_op.track)
          {
            if(uo.a == n_op.a)
            {
              fprintf(stderr, "MusE error: Undo::insert(): Double SetTrackSolo. Ignoring.\n");
              return;
            }
            else
            {
              // On/off followed by off/on is useless. Cancel out the on/off + off/on by erasing the command.
              erase(iuo);
              return;  
            }
          }
        break;
        
        case UndoOp::SetTrackRecMonitor:
          if(uo.type == UndoOp::SetTrackRecMonitor && uo.track == n_op.track)
          {
            if(uo.a == n_op.a)
            {
              fprintf(stderr, "MusE error: Undo::insert(): Double SetTrackRecMonitor. Ignoring.\n");
              return;
            }
            else
            {
              // On/off followed by off/on is useless. Cancel out the on/off + off/on by erasing the command.
              erase(iuo);
              return;  
            }
          }
        break;
        
        case UndoOp::SetTrackOff:
          if(uo.type == UndoOp::SetTrackOff && uo.track == n_op.track)
          {
            if(uo.a == n_op.a)
            {
              fprintf(stderr, "MusE error: Undo::insert(): Double SetTrackOff. Ignoring.\n");
              return;
            }
            else
            {
              // On/off followed by off/on is useless. Cancel out the on/off + off/on by erasing the command.
              erase(iuo);
              return;  
            }
          }
        break;
        
        case UndoOp::ModifyPartName:
          if(uo.type == UndoOp::ModifyPartName && uo.part == n_op.part)  
          {
            // Done with these objects. Be sure to delete them.
            delete n_op._oldName;
            delete uo._newName;
            // Simply replace the existing new name with the newer name.
            uo._newName = n_op._newName;
            return;
          }
        break;

      case UndoOp::ModifyPartStart:
          // TODO: events_offset is a difference requiring accumulation not simple replacement,
          //        and events_offset_time_type might be different requiring conversion. 
//           if(uo.type == UndoOp::ModifyPartStart)
//           {
//             if(uo.part == n_op.part)
//             {
//               // Simply replace the new values.
//               uo.new_partlen_or_pos = n_op.new_partlen_or_pos;
//               uo.new_partlen = n_op.new_partlen;
//               uo.events_offset = n_op.events_offset;
//               uo.events_offset_time_type = n_op.events_offset_time_type;
//               return;
//             }
//           }
          break;

      case UndoOp::ModifyPartLength:
          // TODO: events_offset is a difference requiring accumulation not simple replacement,
          //        and events_offset_time_type might be different requiring conversion. 
//           if(uo.type == UndoOp::ModifyPartLength)
//           {
//             if(uo.part == n_op.part)
//             {
//               // Simply replace the new values.
//               uo.new_partlen_or_pos = n_op.new_partlen_or_pos;
//               uo.events_offset = n_op.events_offset;
//               uo.events_offset_time_type = n_op.events_offset_time_type;
//               return;
//             }
//           }
        break;
        
        case UndoOp::MovePart:
          if(uo.type == UndoOp::MovePart && uo.part == n_op.part)
          {
            // Simply replace the new value and new track.
            uo.new_partlen_or_pos = n_op.new_partlen_or_pos;
            uo.track = n_op.track;
            return;
          }
        break;
        
        case UndoOp::AddPart:
          if(uo.type == UndoOp::AddPart && uo.part == n_op.part)  
          {
            fprintf(stderr, "MusE error: Undo::insert(): Double AddPart. Ignoring.\n");
            return;  
          }
          else if(uo.type == UndoOp::DeletePart && uo.part == n_op.part)  
          {
            // Delete followed by add is useless. Cancel out the delete + add by erasing the delete command.
            erase(iuo);
            return;  
          }
        break;
        
        case UndoOp::DeletePart:
          if(uo.type == UndoOp::DeletePart && uo.part == n_op.part)  
          {
            fprintf(stderr, "MusE error: Undo::insert(): Double DeletePart. Ignoring.\n");
            return;  
          }
          else if(uo.type == UndoOp::AddPart && uo.part == n_op.part)  
          {
            // Add followed by delete is useless. Cancel out the add + delete by erasing the add command.
            erase(iuo);
            return;  
          }
        break;

        
        case UndoOp::AddEvent:
          if(uo.type == UndoOp::AddEvent && uo.nEvent == n_op.nEvent && uo.part == n_op.part)  
          {
            fprintf(stderr, "MusE error: Undo::insert(): Double AddEvent. Ignoring.\n");
            return;  
          }
          else if(uo.type == UndoOp::DeleteEvent && uo.part == n_op.part)  
          {
            if(uo.nEvent == n_op.nEvent)
            {
              // Delete followed by add is useless. Cancel out the delete + add by erasing the delete command.
              erase(iuo);
              return;
            }
            else

            // To allow for easy DeleteEvent + AddEvent of a given controller number at a given time,
            //  instead of demanding ModifyEvent. Automatically transform the operations.
            if(uo.nEvent.type() == Controller && n_op.nEvent.type() == Controller &&
               uo.nEvent.dataA() == n_op.nEvent.dataA() &&
               uo.nEvent.posValue() == n_op.nEvent.posValue())
            {
              // Transform the DeleteEvent operation into a ModifyEvent operation.
              uo.type = UndoOp::ModifyEvent;
              uo.oEvent = uo.nEvent;
              uo.nEvent = n_op.nEvent;
              return;  
            }
          }
          else if(uo.type == UndoOp::ModifyEvent && uo.part == n_op.part)
          {
            if(uo.nEvent == n_op.nEvent)  
            {
              // Modify followed by adding of the modify's new event, is equivalent to just modifying with the added event. 
              fprintf(stderr, "MusE error: Undo::insert(): ModifyEvent, then AddEvent same new event (double AddEvent). Ignoring.\n");
              return;  
            }
            else if(uo.oEvent == n_op.nEvent)  
            {
              // Modify followed by adding of the modify's old event, is equivalent to just adding the event. 
              // Transform the ModifyEvent operation into an AddEvent.
              uo.type = UndoOp::AddEvent;
              uo.nEvent = uo.oEvent;
              return;  
            }
          }
        break;
        
        case UndoOp::DeleteEvent:
          if(uo.type == UndoOp::DeleteEvent && uo.nEvent == n_op.nEvent && uo.part == n_op.part)  
          {
            fprintf(stderr, "MusE error: Undo::insert(): Double DeleteEvent. Ignoring.\n");
            return;  
          }
          else if(uo.type == UndoOp::AddEvent && uo.nEvent == n_op.nEvent && uo.part == n_op.part)  
          {
            // Add followed by delete is useless. Cancel out the add + delete by erasing the add command.
            erase(iuo);
            return;  
          }
          else if(uo.type == UndoOp::ModifyEvent && uo.part == n_op.part)  
          {
            if(uo.oEvent == n_op.nEvent)  
            {
              // Modify followed by delete of the modify's old event, is an error - two deletes of the same event. 
              fprintf(stderr, "MusE error: Undo::insert(): ModifyEvent, then DeleteEvent same old event (double DeleteEvent). Ignoring.\n");
              return;  
            }
            else if(uo.nEvent == n_op.nEvent)
            {
              // Modify followed by delete of the modify's new event, is equivalent to just deleting the old event. 
              // Transform the operation into a DeleteEvent.
              uo.type = UndoOp::DeleteEvent;
              uo.nEvent = uo.oEvent;
              return;  
            }
          }
        break;

        case UndoOp::ModifyEvent:
          if(n_op.oEvent == n_op.nEvent)
          {
            // Equivalent to deleting then adding the same event - useless, cancels out.
            return;
          }
          else if(uo.type == UndoOp::ModifyEvent && uo.part == n_op.part)  
          {
            // For testing...
            //fprintf(stderr, "MusE: DIAGNOSTIC: Undo::insert(): Double ModifyEvent... checking for errors...\n");
              
            if(uo.oEvent == n_op.oEvent)
            {
              if(uo.nEvent == n_op.nEvent)
              {
                fprintf(stderr, "MusE error: Undo::insert(): Double ModifyEvent. Ignoring.\n");
                return;
              }
              else
              {
                // For testing...
                //fprintf(stderr, "MusE: Undo::insert(): Double ModifyEvent. Same old events. Merging.\n");
                
                // Two modify commands with old events the same is equivalent to just one modify command.
                // Replace the existing ModifyEvent command's new event with the requested ModifyEvent command's new event.
                uo.nEvent = n_op.nEvent;
                return;  
              }
            }
            // REMOVE Tim. citem. Added. Remove. I think we CAN replace two different events with the same event.
            //else if(uo.nEvent == n_op.nEvent)
            //{
            //  // Cannot replace two different events with the same event.
            //  fprintf(stderr, "MusE error: Undo::insert(): Double ModifyEvent: different old events but same new event. Ignoring.\n");
            //  return;
            //}
            // Are inner new/old pair the same event?
            else if(uo.nEvent == n_op.oEvent) 
            {
              // Are outer old/new pair the same event?
              if(uo.oEvent == n_op.nEvent)
              {
                // First ModifyEvent old event and second ModifyEvent new event are both the same, equivalent to doing nothing.
                // Cancel out the two ModifyEvent operations by erasing the existing ModifyEvent command.
                erase(iuo);
                return;  
              }
              else
              {
                // For testing...
                //fprintf(stderr, "MusE: Undo::insert(): Double ModifyEvent. Inner new/old pair same, outer old/new pair not same. Merging to one ModifyEvent.\n");
            
                // Inner new/old pair are the same event and outer old/new pair are not the same event.
                // A modify command with new event followed by a modify command with old event the same
                //  is equivalent to just one modify command. Replace the existing ModifyEvent command's
                //  new event with the requested ModifyEvent command's new event.
                uo.nEvent = n_op.nEvent;
                return;  
              }
            }
            // Inner new/old pair are not the same event. Are outer old/new pair the same event?
            else if(uo.oEvent == n_op.nEvent) 
            {
                // For testing...
                //fprintf(stderr, "MusE: Undo::insert(): Double ModifyEvent. Inner new/old pair not same,"
                // " outer old/new pair same. Transforming to Add and Delete.\n");
            
              // Transform the existing ModifyEvent operation into an AddEvent.
              uo.type = UndoOp::AddEvent;
              // Transform the requested ModifyEvent operation into a DeleteEvent.
              n_op.type = UndoOp::DeleteEvent;
              n_op.nEvent = n_op.oEvent;
              // Allow it to add...
            }
          }
          else if(uo.type == UndoOp::AddEvent && uo.part == n_op.part)
          {
            // For testing...
            //fprintf(stderr, "MusE: Undo::insert(): AddEvent then ModifyEvent...\n");
            
            if(uo.nEvent == n_op.oEvent)
            {
              // For testing...
              //fprintf(stderr, "MusE: Undo::insert(): AddEvent then ModifyEvent. Same event. Merging to AddEvent.\n");
            
              // Add followed by modify with old event same as added event, is equivalent to just adding modify's new event.
              // Replace the existing AddEvent command's event with the requested ModifyEvent command's new event.
              uo.nEvent = n_op.nEvent;
              return;  
            }
            if(uo.nEvent == n_op.nEvent)
            {
              // Add followed by modify with new event same as added event, is a caller error.
              fprintf(stderr, "MusE error: Undo::insert(): AddEvent, then ModifyEvent same new event (double AddEvent). Ignoring.\n");
              return;  
            }
          }
          if(uo.type == UndoOp::DeleteEvent && uo.part == n_op.part)
          {
            if(uo.nEvent == n_op.oEvent)
            {
              // Delete followed by modify with old event same as deleted event, is an error.
              fprintf(stderr, "MusE error: Undo::insert(): DeleteEvent, then ModifyEvent same old event (double DeleteEvent). Ignoring.\n");
              return;  
            }
            if(uo.nEvent == n_op.nEvent)
            {
              // For testing...
              //fprintf(stderr, "MusE: Undo::insert(): DeleteEvent then ModifyEvent. Same event. Merging to DeleteEvent.\n");
            
              // Delete followed by modify with new event same as deleted event, is equivalent to just deleting modify's old event.
              // Replace the existing DeleteEvent command's event with the requested ModifyEvent command's old event.
              uo.nEvent = n_op.oEvent;
            }
          }
        break;

        case UndoOp::AddAudioCtrlVal:
          if(uo.type == UndoOp::AddAudioCtrlVal && uo.track == n_op.track &&
             uo._audioCtrlIdAddDel == n_op._audioCtrlIdAddDel &&
             uo._audioCtrlFrameAddDel == n_op._audioCtrlFrameAddDel)
          {
            // Simply replace the original value and flags.
            uo._audioCtrlValAddDel = n_op._audioCtrlValAddDel;
            uo._audioCtrlValFlagsAddDel = n_op._audioCtrlValFlagsAddDel;
            return;
          }
// TODO If possible.
//           else if(uo.type == UndoOp::DeleteAudioCtrlVal && uo.track == n_op.track &&
//              uo._audioCtrlIdStruct == n_op._audioCtrlIdAdd &&
//              uo._audioCtrlFrameStruct == n_op._audioCtrlFrameAdd)
//           {
//             // Delete followed by add, at the same frame. Transform the delete into a modify.
//             uo.type = UndoOp::ModifyAudioCtrlVal;
//             uo._audioCtrlValStruct->setValue(n_op._audioCtrlValAdd);
//             uo._audioCtrlFrameStruct =
//             return;  
//           }
        break;
        
        case UndoOp::AddAudioCtrlValStruct:
          if(uo.type == UndoOp::AddAudioCtrlValStruct && uo.track == n_op.track &&
             uo._audioCtrlIdStruct == n_op._audioCtrlIdStruct &&
             uo._audioCtrlFrameStruct == n_op._audioCtrlFrameStruct)
          {
            // Done with older operation structure. Be sure to delete it.
            deleteUndoOp(uo);
            // Simply replace the existing new structure with the newer structure.
            uo._audioCtrlValStruct = n_op._audioCtrlValStruct;
            return;
          }
// TODO If possible.
//           else if(uo.type == UndoOp::DeleteAudioCtrlVal && uo.track == n_op.track &&
//              uo._audioCtrlID == n_op._audioCtrlID &&
//              uo._audioCtrlFrame == n_op._audioCtrlFrame)
//           {
//             // Delete followed by add, at the same frame. Transform the delete into a modify.
//             uo.type = UndoOp::ModifyAudioCtrlVal;
//             uo._audioCtrlVal = n_op._audioCtrlVal;
//             uo._audioNewCtrlFrame =
//             return;
//           }
        break;


        case UndoOp::DeleteAudioCtrlVal:
          if(uo.type == UndoOp::DeleteAudioCtrlVal && uo.track == n_op.track &&
             uo._audioCtrlIdAddDel == n_op._audioCtrlIdAddDel &&
             uo._audioCtrlFrameAddDel == n_op._audioCtrlFrameAddDel)
          {
            fprintf(stderr, "MusE error: Undo::insert(): Double DeleteAudioCtrlVal. Ignoring.\n");
            return;
          }
          else if(uo.type == UndoOp::AddAudioCtrlVal && uo.track == n_op.track &&
             uo._audioCtrlIdAddDel == n_op._audioCtrlIdAddDel &&
             uo._audioCtrlFrameAddDel == n_op._audioCtrlFrameAddDel)
          {
            // Done with operation structure. Be sure to delete it if it exists.
            deleteUndoOp(n_op);
            // Add followed by delete, at the same frame, is useless. Cancel out the add + delete by erasing the add command.
            erase(iuo);
            return;
          }
        break;

        case UndoOp::ModifyAudioCtrlVal:
          if(uo.type == UndoOp::ModifyAudioCtrlVal && uo.track == n_op.track &&
             uo._audioCtrlID == n_op._audioCtrlID &&
             uo._audioNewCtrlFrame == n_op._audioCtrlFrame)
          {
            // Simply replace the original new value and new frame.
            uo._audioNewCtrlVal = n_op._audioNewCtrlVal;
            uo._audioNewCtrlFrame = n_op._audioNewCtrlFrame;
            return;
          }
        break;

        case UndoOp::ModifyAudioCtrlValList:
          // Check the sanity of the requested op.
          if((n_op._eraseCtrlList &&
             (n_op._eraseCtrlList == n_op._addCtrlList)) ||
             (n_op._recoverableEraseCtrlList &&
               (n_op._recoverableEraseCtrlList == n_op._eraseCtrlList ||
                n_op._recoverableEraseCtrlList == n_op._addCtrlList ||
                n_op._recoverableEraseCtrlList == n_op._recoverableAddCtrlList ||
                n_op._recoverableEraseCtrlList == n_op._doNotEraseCtrlList)) ||
              (n_op._recoverableAddCtrlList &&
                (n_op._recoverableAddCtrlList == n_op._eraseCtrlList ||
                 n_op._recoverableAddCtrlList == n_op._addCtrlList ||
                 n_op._recoverableAddCtrlList == n_op._doNotEraseCtrlList)) ||
              (n_op._doNotEraseCtrlList &&
                (n_op._doNotEraseCtrlList == n_op._eraseCtrlList ||
                 n_op._doNotEraseCtrlList == n_op._addCtrlList)))
          {
            fprintf(stderr, "MusE error: Undo::insert(): ModifyAudioCtrlValList: Erase and add lists are the same. Ignoring.\n");
            return;
          }
          
          if(uo.type == UndoOp::ModifyAudioCtrlValList)
          {
            if(uo.track == n_op.track)
            {
              // TODO: Handle _recoverableEraseCtrlList et al.
              if(uo._addCtrlList == n_op._addCtrlList && uo._eraseCtrlList == n_op._eraseCtrlList)
              {
                fprintf(stderr, "MusE error: Undo::insert(): Double ModifyAudioCtrlValList. Ignoring.\n");
                return;
              }
              else if(uo._addCtrlList && uo._addCtrlList == n_op._eraseCtrlList)
              {
                // Delete the existing ModifyAudioCtrlValList command's _addCtrlList and replace it
                //  with the requested ModifyAudioCtrlValList command's _addCtrlList.
                delete uo._addCtrlList;
                uo._addCtrlList = n_op._addCtrlList;
                return;
              }
            }
            // Seems possible... remove? But maybe dangerous to have two undo ops pointing to the same lists - they will be self-deleted.
            else
            {
              if(uo._addCtrlList && uo._addCtrlList == n_op._addCtrlList)
              {
                fprintf(stderr, "MusE error: Undo::insert(): ModifyAudioCtrlValList: Attempting to add same list to different containers. Ignoring.\n");
                return;
              }
              else if(uo._eraseCtrlList && uo._eraseCtrlList == n_op._eraseCtrlList)
              {
                fprintf(stderr, "MusE error: Undo::insert(): ModifyAudioCtrlValList: Attempting to erase same list from different containers. Ignoring.\n");
                return;
              }
            }
          }
        break;

        case UndoOp::SelectAudioCtrlVal:
          if(uo.type == UndoOp::SelectAudioCtrlVal &&
             uo._audioCtrlListSelect == n_op._audioCtrlListSelect &&
             uo._audioCtrlSelectFrame == n_op._audioCtrlSelectFrame)
          {
            // Simply replace the original value.
            uo.selected = n_op.selected;
            return;
          }
        break;

        case UndoOp::SetAudioCtrlPasteEraseMode:
          if(uo.type == UndoOp::SetAudioCtrlPasteEraseMode)
          {
            // Simply replace the original new value.
            uo._audioCtrlNewPasteEraseOpts = n_op._audioCtrlNewPasteEraseOpts;
            return;
          }
        break;

        case UndoOp::BeginAudioCtrlMoveMode:
          if(uo.type == UndoOp::BeginAudioCtrlMoveMode)
          {
            fprintf(stderr, "MusE error: Undo::insert(): Double BeginAudioCtrlMoveMode. Ignoring.\n");
            return;
          }
        break;
        case UndoOp::EndAudioCtrlMoveMode:
          if(uo.type == UndoOp::EndAudioCtrlMoveMode)
          {
            // This condition is expected sometimes, not really an error. Just silently ignore it.
#ifdef _UNDO_DEBUG_
            fprintf(stderr, "MusE warning: Undo::insert(): Double EndAudioCtrlMoveMode. Ignoring.\n");
#endif
            return;
          }
        break;

        case UndoOp::SetInstrument:
          // Check the sanity of the requested op.
          if(n_op._oldMidiInstrument == n_op._newMidiInstrument)
          {
            fprintf(stderr, "MusE error: Undo::insert(): SetInstrument: Old and new instruments are the same. Ignoring.\n");
            return;
          }
          
          if(uo.type == UndoOp::SetInstrument)
          {
            if(uo._midiPort == n_op._midiPort)
            {
              if(uo._oldMidiInstrument == n_op._oldMidiInstrument && uo._newMidiInstrument == n_op._newMidiInstrument)
              {
                fprintf(stderr, "MusE error: Undo::insert(): Double SetInstrument. Ignoring.\n");
                return;
              }
              else if(uo._newMidiInstrument == n_op._oldMidiInstrument)
              {
                // Replace the existing SetInstrument command's _newMidiInstrument
                //  with the requested SetInstrument command's _newMidiInstrument.
                uo._newMidiInstrument = n_op._newMidiInstrument;
                return;
              }
            }
          }
        break;


        case UndoOp::AddTempo:
          if(uo.type == UndoOp::AddTempo && uo.a == n_op.a)  
          {
            // Simply replace the value. 
            uo.b = n_op.b;
            return;
          }
          else if(uo.type == UndoOp::DeleteTempo && uo.a == n_op.a)  
          {
            // Delete followed by add. Transform the existing DeleteTempo operation into a ModifyTempo.
            uo.type = UndoOp::ModifyTempo;
            // a is already the tick, b is already the existing value from DeleteTempo, c is the new value.
            uo.c = n_op.b;
            return;  
          }
          else if(uo.type == UndoOp::ModifyTempo && uo.a == n_op.a)  
          {
            // Modify followed by add. Simply replace the value.
            // a is already the tick, b is already the existing value from ModifyTempo, c is the new value.
            uo.c = n_op.b;
            return;
          }
//           else if(uo.type == UndoOp::SetTempo && uo.a == n_op.a)  
//           {
//             // Only if the master is on.
//             if(MusEGlobal::tempomap.masterFlag())
//             {
//               // Simply replace the value. 
//               uo.b = n_op.b;
//               return;
//             }
//           }
        break;
        
        case UndoOp::DeleteTempo:
          if(uo.type == UndoOp::DeleteTempo && uo.a == n_op.a)  
          {
            fprintf(stderr, "MusE error: Undo::insert(): Double DeleteTempo. Ignoring.\n");
            return;  
          }
          else if(uo.type == UndoOp::AddTempo && uo.a == n_op.a)  
          {
            // Add followed by delete is useless. Cancel out the add + delete by erasing the add command.
            erase(iuo);
            return;  
          }
          else if(uo.type == UndoOp::ModifyTempo && uo.a == n_op.a)  
          {
            // Modify followed by delete. Equivalent to delete. Transform existing ModifyTempo operation into a DeleteTempo.
            uo.type = UndoOp::DeleteTempo;
            // a is already the tick, b is already the existing old value from ModifyTempo.
            return;  
          }
//           else if(uo.type == UndoOp::SetTempo && uo.a == n_op.a)  
//           {
//             // Only if the master is on.
//             if(MusEGlobal::tempomap.masterFlag())
//             {
//               // Add followed by delete is useless. Cancel out the add + delete by erasing the add command.
//               erase(iuo);
//               return;
//             }
//           }
        break;
        
        case UndoOp::ModifyTempo:
          if(uo.type == UndoOp::ModifyTempo && uo.a == n_op.a)  
          {
            // Simply replace c with the new value.
            uo.c = n_op.c;
            return;  
          }
          else if(uo.type == UndoOp::AddTempo && uo.a == n_op.a)  
          {
            // Add followed by modify. Simply replace the add value.
            uo.b = n_op.c;
            return;  
          }
          else if(uo.type == UndoOp::DeleteTempo && uo.a == n_op.a)  
          {
            // Delete followed by modify. Equivalent to modify. Transform existing DeleteTempo operation into a ModifyTempo.
            uo.type = UndoOp::ModifyTempo;
            // a is already the tick, b is already the existing value from DeleteTempo. c is the new value from ModifyTempo.
            uo.c = n_op.c;
            return;  
          }
//           else if(uo.type == UndoOp::SetTempo && uo.a == n_op.a)  
//           {
//             // Only if the master is on.
//             if(MusEGlobal::tempomap.masterFlag())
//             {
//               // Add followed by modify. Simply replace the add value.
//               uo.b = n_op.c;
//               return;
//             }
//           }
        break;
          
//         case UndoOp::SetTempo:
//           if(uo.type == UndoOp::SetTempo && uo.a == n_op.a)  
//           {
//             // Simply replace the value. 
//             uo.b = n_op.b;
//             return;  
//           }
//           else if(uo.type == UndoOp::AddTempo && uo.a == n_op.a)  
//           {
//             // Simply replace the value. 
//             uo.b = n_op.b;
//             return;
//           }
//           else if(uo.type == UndoOp::DeleteTempo && uo.a == n_op.a)  
//           {
//             // Only if the master is on.
//             if(MusEGlobal::tempomap.masterFlag())
//             {
//               // Delete followed by add. Transform the existing DeleteTempo operation into a ModifyTempo.
//               uo.type = UndoOp::ModifyTempo;
//               // a is already the tick, b is already the existing value from DeleteTempo, c is the new value.
//               uo.c = n_op.b;
//               return;
//             }
//           }
//           else if(uo.type == UndoOp::ModifyTempo && uo.a == n_op.a)  
//           {
//             // Only if the master is on.
//             if(MusEGlobal::tempomap.masterFlag())
//             {
//               // Modify followed by add. Simply replace the value.
//               // a is already the tick, b is already the existing value from ModifyTempo, c is the new value.
//               uo.c = n_op.b;
//               return;
//             }
//           }
//           else if(uo.type == UndoOp::SetStaticTempo && uo.a == n_op.a)  
//           {
//             // Only if the master is not on.
//             if(!MusEGlobal::tempomap.masterFlag())
//             {
//               // Simply replace the value. 
//               uo.b = n_op.b;
//               return;
//             }
//           }
//         break;
        
        case UndoOp::SetStaticTempo:
          if(uo.type == UndoOp::SetStaticTempo)
          {
            // Simply replace a with the new value.
            uo.a = n_op.a;
            return;  
          }
        break;

        case UndoOp::SetGlobalTempo:
          if(uo.type == UndoOp::SetGlobalTempo)  
          {
            // Simply replace a with the new value.
            uo.a = n_op.a;
            return;  
          }
        break;

        case UndoOp::EnableMasterTrack:
          if(uo.type == UndoOp::EnableMasterTrack)
          {
            if(uo.a == n_op.a)
            {
              fprintf(stderr, "MusE error: Undo::insert(): Double EnableMasterTrack. Ignoring.\n");
              return;  
            }
            else
            {
              // Toggling is useless. Cancel out the enable or disable + disable or enable by erasing the disable or enable command.
              erase(iuo);
              return;  
            }
          }
        break;

        
        case UndoOp::AddSig:
          if(uo.type == UndoOp::AddSig && uo.a == n_op.a)  
          {
            // Simply replace the value. 
            uo.b = n_op.b;
            uo.c = n_op.c;
            return;  
          }
          else if(uo.type == UndoOp::DeleteSig && uo.a == n_op.a)  
          {
            // Delete followed by add. Transform the existing DeleteSig operation into a ModifySig.
            uo.type = UndoOp::ModifySig;
            // a is already the tick, b + c is already the existing value from DeleteSig, d + e is the new value.
            uo.d = n_op.b;
            uo.e = n_op.c;
            return;  
          }
          else if(uo.type == UndoOp::ModifySig && uo.a == n_op.a)  
          {
            // Modify followed by add. Simply replace the value.
            // a is already the tick, b + c is already the existing value from ModifySig, d + e is the new value.
            uo.d = n_op.b;
            uo.e = n_op.c;
            return;  
          }
        break;
        
        case UndoOp::DeleteSig:
          if(uo.type == UndoOp::DeleteSig && uo.a == n_op.a)  
          {
            fprintf(stderr, "MusE error: Undo::insert(): Double DeleteSig. Ignoring.\n");
            return;  
          }
          else if(uo.type == UndoOp::AddSig && uo.a == n_op.a)  
          {
            // Add followed by delete is useless. Cancel out the add + delete by erasing the add command.
            erase(iuo);
            return;  
          }
          else if(uo.type == UndoOp::ModifySig && uo.a == n_op.a)  
          {
            // Modify followed by delete. Equivalent to delete. Transform existing ModifySig operation into a DeleteSig.
            uo.type = UndoOp::DeleteSig;
            // a is already the tick, b + c is already the existing old value from ModifySig.
            return;  
          }
        break;
        
        case UndoOp::ModifySig:
          if(uo.type == UndoOp::ModifySig && uo.a == n_op.a)  
          {
            // Simply replace d + e with the new value.
            uo.d = n_op.d;
            uo.e = n_op.e;
            return;  
          }
          else if(uo.type == UndoOp::AddSig && uo.a == n_op.a)  
          {
            // Add followed by modify. Simply replace the add value.
            uo.b = n_op.d;
            uo.c = n_op.e;
            return;  
          }
          else if(uo.type == UndoOp::DeleteSig && uo.a == n_op.a)  
          {
            // Delete followed by modify. Equivalent to modify. Transform existing DeleteSig operation into a ModifySig.
            uo.type = UndoOp::ModifySig;
            // a is already the tick, b + c is already the existing value from DeleteSig. d + e is the new value from ModifySig.
            uo.d = n_op.d;
            uo.e = n_op.e;
            return;  
          }
        break;
          
        
        case UndoOp::AddKey:
          if(uo.type == UndoOp::AddKey && uo.a == n_op.a)  
          {
            // Simply replace the value. 
            uo.b = n_op.b;
            uo.c = n_op.c;
            return;  
          }
          else if(uo.type == UndoOp::DeleteKey && uo.a == n_op.a)  
          {
            // Delete followed by add. Transform the existing DeleteKey operation into a ModifyKey.
            uo.type = UndoOp::ModifyKey;
            // a is already the tick, b + c is already the existing value from DeleteKey, d + e is the new value.
            uo.d = n_op.b;
            uo.e = n_op.c;
            return;  
          }
          else if(uo.type == UndoOp::ModifyKey && uo.a == n_op.a)  
          {
            // Modify followed by add. Simply replace the value.
            // a is already the tick, b + c is already the existing value from ModifyKey, d + e is the new value.
            uo.d = n_op.b;
            uo.e = n_op.c;
            return;  
          }
        break;
        
        case UndoOp::DeleteKey:
          if(uo.type == UndoOp::DeleteKey && uo.a == n_op.a)  
          {
            fprintf(stderr, "MusE error: Undo::insert(): Double DeleteKey. Ignoring.\n");
            return;  
          }
          else if(uo.type == UndoOp::AddKey && uo.a == n_op.a)  
          {
            // Add followed by delete is useless. Cancel out the add + delete by erasing the add command.
            erase(iuo);
            return;  
          }
          else if(uo.type == UndoOp::ModifyKey && uo.a == n_op.a)  
          {
            // Modify followed by delete. Equivalent to delete. Transform existing ModifyKey operation into a DeleteKey.
            uo.type = UndoOp::DeleteKey;
            // a is already the tick, b + c is already the existing old value from ModifyKey.
            return;  
          }
        break;
        
        case UndoOp::ModifyKey:
          if(uo.type == UndoOp::ModifyKey && uo.a == n_op.a)  
          {
            // Simply replace d + e with the new value.
            uo.d = n_op.d;
            uo.e = n_op.e;
            return;  
          }
          else if(uo.type == UndoOp::AddKey && uo.a == n_op.a)  
          {
            // Add followed by modify. Simply replace the add value.
            uo.b = n_op.d;
            uo.c = n_op.e;
            return;  
          }
          else if(uo.type == UndoOp::DeleteKey && uo.a == n_op.a)  
          {
            // Delete followed by modify. Equivalent to modify. Transform existing DeleteSig operation into a ModifySig.
            uo.type = UndoOp::ModifyKey;
            // a is already the tick, b + c is already the existing value from DeleteKey. d + e is the new value from ModifyKey.
            uo.d = n_op.d;
            uo.e = n_op.e;
            return;  
          }
        break;

        
        case UndoOp::ModifySongLen:
          if(uo.type == UndoOp::ModifySongLen)  
          {
            // Simply replace a with the new value.
            uo.a = n_op.a;
            return;  
          }
        break;
        
        case UndoOp::ModifyMidiDivision:
          if(uo.type == UndoOp::ModifyMidiDivision)
          {
            // Simply replace a with the new value.
            uo.a = n_op.a;
            return;  
          }
        break;
        
        case UndoOp::EnableAllAudioControllers:
          if(uo.type == UndoOp::EnableAllAudioControllers)
          {
            fprintf(stderr, "MusE error: Undo::insert(): Double EnableAllAudioControllers. Ignoring.\n");
            return;  
          }
        break;
        
        case UndoOp::GlobalSelectAllEvents:
          if(uo.type == UndoOp::GlobalSelectAllEvents)
          {
            if(uo.a == n_op.a)
            {
              fprintf(stderr, "MusE error: Undo::insert(): Double GlobalSelectAllEvents. Ignoring.\n");
              return;
            }
            else
            {
              // Special: Do not 'cancel' out this one. The selections may need to affect all events.
              // Simply replace a with the new value.
              uo.a = n_op.a;
              return;  
            }
          }
        break;


        case UndoOp::AddMarker:
          if(uo.type == UndoOp::AddMarker && uo.newMarker->id() == n_op.newMarker->id())
          {
            // Done with older operation marker. Be sure to delete it.
            deleteUndoOp(uo);
            // Simply replace the existing new marker with the newer marker.
            uo.newMarker = n_op.newMarker;
            return;  
          }
          else if(uo.type == UndoOp::DeleteMarker && uo.oldMarker->id() == n_op.newMarker->id())
          {
            // Delete followed by add. Transform the existing DeleteMarker operation into a ModifyMarker.
            uo.type = UndoOp::ModifyMarker;
            // Move the new marker into the ModifyMarker command's new marker.
            // Keep the existing DeleteMarker command's oldMarker.
            uo.newMarker = n_op.newMarker;
            return;
          }
        break;
        
        case UndoOp::DeleteMarker:
          if(uo.type == UndoOp::DeleteMarker && uo.oldMarker->id() == n_op.oldMarker->id())
          {
            // Done with older operation marker. Be sure to delete it.
            deleteUndoOp(uo);
            // Simply replace the existing new marker with the newer marker.
            uo.oldMarker = n_op.oldMarker;
            return;  
          }
          else if(uo.type == UndoOp::AddMarker && uo.newMarker->id() == n_op.oldMarker->id())
          {
            // Done with operation markers. Be sure to delete them.
            deleteUndoOp(n_op);
            // Add followed by delete is useless. Cancel out the add + delete by erasing the add command.
            deleteAndErase(iuo);
            return;  
          }
        break;
        
        case UndoOp::ModifyMarker:
          if(uo.type == UndoOp::ModifyMarker && uo.oldMarker->id() == n_op.oldMarker->id())
          {
            // Done with these operation markers. Be sure to delete them.
            delete uo.newMarker;
            delete n_op.oldMarker;
            // Simply replace the older operation marker with the newer one.
            uo.newMarker = n_op.newMarker;
            return;  
          }
        break;
        
        case UndoOp::SetMarkerPos:
          if(uo.type == UndoOp::SetMarkerPos && uo.oldMarker->id() == n_op.oldMarker->id())
          {
            // Done with these operation markers. Be sure to delete them.
            delete uo.newMarker;
            delete n_op.oldMarker;
            // Simply replace the older operation marker with the newer one.
            uo.newMarker = n_op.newMarker;
            return;  
          }
        break;
        
        // NOTE Some other undo op types may need treatment as well !
        
        default:
        break;  
      }
    }
  }
  
  std::list<UndoOp>::insert(position, n_op);
}

bool Undo::merge_combo(const Undo& other)
{
  if (other.combobreaker)
          return false;
  
  int has_other=0x01;
  int has_select_event=0x02;
  int has_select_part=0x04;
  int has_modify_aud_ctrl_val=0x08;
  int has_set_marker_pos=0x10;

  int has = 0;
  for (ciUndoOp op=this->begin(); op!=this->end(); op++)
          switch(op->type)
          {
                  case UndoOp::DoNothing: break;
                  case UndoOp::SelectEvent: has |= has_select_event; break;
                  case UndoOp::SelectPart: has |= has_select_part; break;
                  case UndoOp::ModifyAudioCtrlVal: has |= has_modify_aud_ctrl_val; break;
                  case UndoOp::SetMarkerPos: has |= has_set_marker_pos; break;
                  default: has |= has_other; break;
          }
  
  for (ciUndoOp op=other.begin(); op!=other.end(); op++)
          switch(op->type)
          {
                  case UndoOp::DoNothing: break;
                  case UndoOp::SelectEvent: has |= has_select_event; break;
                  case UndoOp::SelectPart: has |= has_select_part; break;
                  case UndoOp::ModifyAudioCtrlVal: has |= has_modify_aud_ctrl_val; break;
                  case UndoOp::SetMarkerPos: has |= has_set_marker_pos; break;
                  default: has |= has_other; break;
          }
  
  bool mergeable =
    (has == has_select_event || has == has_select_part ||
     has == has_modify_aud_ctrl_val || has == has_set_marker_pos);
  
  if (mergeable)
          this->insert(this->end(), other.begin(), other.end());
  
  return mergeable;
}

bool Song::applyOperation(const UndoOp& op, OperationType type, void* sender)
{
	Undo operations;
	operations.push_back(op);
	return applyOperationGroup(operations, type, sender);
}

bool Song::applyOperationGroup(Undo& group, OperationType type, void* sender)
{
  bool ret = false;
  if (!group.empty())
  {
    // We don't use this here in applyOperationGroup or its call sequence.
    undoMode = false;

    switch(type)
    {
      case OperationExecute:
      case OperationUndoable:
      break;
      
      case OperationExecuteUpdate:
      case OperationUndoableUpdate:
      case OperationUndoMode:
          // Clear the updateFlags and set sender.
          updateFlags = SongChangedStruct_t(0, 0, sender);
      break;
    }

    // Execute the given operations. This can add or remove operations in the group.
    MusEGlobal::audio->msgExecuteOperationGroup(group);
    
    // Check whether there are actually any undoable operations in the group.
    // There shouldn't be any non-undoables left in the list, they are removed at execution,
    //  but we'll double check here which also checks list emptiness.
    bool has_undoables = false;
    for(ciUndoOp iu = group.cbegin(); iu != group.cend(); ++iu) {
      if(!iu->_noUndo) {
        has_undoables = true;
        break;
      }
    }

    switch(type)
    {
      case OperationExecute:
      case OperationExecuteUpdate:
      break;
        
      case OperationUndoMode:
        // NOTE: If there are only non-undoables, there is NOTHING to redo (or undo).
        //       Prevent one-time non-undoable operations from wiping out the redo list!
        if(has_undoables) {
          // The following does the same as startUndo but without clearing the updateFlags:
          // redo must be invalidated when a new undo is started
          redoList->clearDelete();
          MusEGlobal::redoAction->setEnabled(false);
          setUndoRedoText();
          undoList->push_back(Undo());
        }
      // FALLTHROUGH
      case OperationUndoable:
      case OperationUndoableUpdate:
        // append all elements from "group" to the end of undoList->back().
        // Only if there are undoable items.
        if(has_undoables && !undoList->empty())
        {
          Undo& curUndo = undoList->back();
          curUndo.insert(curUndo.end(), group.begin(), group.end());
          if (group.combobreaker)
            curUndo.combobreaker=true;
        }
      break;
    }

    switch(type)
    {
      case OperationExecute:
      case OperationUndoable:
      break;
      
      case OperationExecuteUpdate:
      case OperationUndoableUpdate:
        emit songChanged(updateFlags);
      break;
      
      case OperationUndoMode:
        if(has_undoables) {
          // Also emits songChanged and resets undoMode.
          endUndo(0);
          ret = true;
        }
        else {
          emit songChanged(updateFlags);
        }
      break;
    }
  }
        
  return ret;
}

//---------------------------------------------------------
//   revertOperationGroup2
//    real time part
//---------------------------------------------------------

void Song::revertOperationGroup2(Undo& /*operations*/)
      {
        pendingOperations.executeRTStage();

        // Special for tempo: Need to normalize the tempo list, and resync audio. 
        // To save time this is done here, not item by item.
        // Normalize is not needed for SC_MASTER.
        if(updateFlags & (SC_TEMPO | SC_DIVISION_CHANGED))
          MusEGlobal::tempomap.normalize();
        if(updateFlags & (SC_TEMPO | SC_MASTER | SC_DIVISION_CHANGED))
        {
          MusEGlobal::audio->reSyncAudio();
          // Must rebuild the marker list in case any markers are 'locked'.
          if(marker()->rebuild())
            updateFlags |= SC_MARKERS_REBUILT;
        }

        // Special for sig: Need to normalize the signature list. 
        // To save time this is done here, not item by item.
        if(updateFlags & (SC_SIG | SC_DIVISION_CHANGED))
          MusEGlobal::sigmap.normalize();

        // Special for track inserted: If it's an aux track, need to add missing aux sends to all tracks,
        //  else if it's another audio track need to add aux sends to it.
        // To save from complexity this is done here, after all the operations.
        if(updateFlags & SC_TRACK_INSERTED)
        {
          int n = _auxs.size();
          for(iTrack i = _tracks.begin(); i != _tracks.end(); ++i) 
          {
            if((*i)->isMidiTrack())
              continue;
            MusECore::AudioTrack* at = static_cast<AudioTrack*>(*i);
            if(at->hasAuxSend()) 
              at->addAuxSend(n);
          }
        }
      }

//---------------------------------------------------------
//   Song::executeOperationGroup2
//---------------------------------------------------------

void Song::executeOperationGroup2(Undo& /*operations*/)
      {
        pendingOperations.executeRTStage();
        
        // Special for tempo if altered: Need to normalize the tempo list, and resync audio. 
        // To save time this is done here, not item by item.
        // Normalize is not needed for SC_MASTER.
        if(updateFlags & (SC_TEMPO | SC_DIVISION_CHANGED))
          MusEGlobal::tempomap.normalize();
        if(updateFlags & (SC_TEMPO | SC_MASTER | SC_DIVISION_CHANGED))
        {
          MusEGlobal::audio->reSyncAudio();
          // Must rebuild the marker list in case any markers are 'locked'.
          if(marker()->rebuild())
            updateFlags |= SC_MARKERS_REBUILT;
        }

        // Special for sig: Need to normalize the signature list. 
        // To save time this is done here, not item by item.
        if(updateFlags & (SC_SIG | SC_DIVISION_CHANGED))
          MusEGlobal::sigmap.normalize();
        
        // Special for track inserted: If it's an aux track, need to add missing aux sends to all tracks,
        //  else if it's another audio track need to add aux sends to it.
        // To save from complexity this is done here, after all the operations.
        if(updateFlags & SC_TRACK_INSERTED)
        {
          int n = _auxs.size();
          for(iTrack i = _tracks.begin(); i != _tracks.end(); ++i) 
          {
            if((*i)->isMidiTrack())
              continue;
            MusECore::AudioTrack* at = static_cast<AudioTrack*>(*i);
            if(at->hasAuxSend()) 
              at->addAuxSend(n);
          }
        }
      }

UndoOp::UndoOp()
{
  type=UndoOp::DoNothing;
  _noUndo = true;
}

UndoOp::UndoOp(UndoType type_, int a_, int b_, int c_, bool noUndo)
      {
      assert(type_==AddKey || type_==DeleteKey || type_== ModifyKey ||
             type_==AddTempo || type_==DeleteTempo || type_==ModifyTempo || 
             type_==SetTempo || type_==SetStaticTempo || type_==SetGlobalTempo || type_==EnableMasterTrack ||
             type_==AddSig || type_==DeleteSig ||
             type_==ModifySongLen || type_==MoveTrack ||
             type_==GlobalSelectAllEvents || type_==ModifyMidiDivision);
      
      type = type_;
      a  = a_;
      b  = b_;
      c  = c_;
      _noUndo = noUndo;
      
      switch(type)
      {
        case UndoOp::SetGlobalTempo:
          // a is already the new tempo, b is the existing tempo.
          b = MusEGlobal::tempomap.globalTempo();
        break;
        
        case UndoOp::EnableMasterTrack:
          // a is already the new master flag, b is the existing master flag.
          b = MusEGlobal::tempomap.masterFlag();
        break;
        
        case UndoOp::ModifyMidiDivision:
          // a is already the new division, b is the existing division.
          b = MusEGlobal::config.division;
        break;
        
        // For these operations, we must check if a value already exists and transform them into modify operations...
        case UndoOp::AddTempo:
        {
          int t = a;
          if(t > MAX_TICK)
            t = MAX_TICK;
          iTEvent ite = MusEGlobal::tempomap.upper_bound(t);
          if((int)ite->second->tick == t)
          {
            // Transform the AddTempo operation into a ModifyTempo.
            // a is already the tick, b is the existing value, c is the new value.
            type = UndoOp::ModifyTempo;
            c = b;
            b = ite->second->tempo;
          }
        }
        break;
        
        case UndoOp::SetTempo:
        {
          // Only if the master is on.
          if(MusEGlobal::tempomap.masterFlag())
          {
            int t = a;
            if(t > MAX_TICK)
              t = MAX_TICK;
            iTEvent ite = MusEGlobal::tempomap.upper_bound(t);
            if((int)ite->second->tick == t)
            {
              // Transform the SetTempo operation into a ModifyTempo.
              // a is already the tick, b is the existing value, c is the new value.
              type = UndoOp::ModifyTempo;
              c = b;
              b = ite->second->tempo;
            }
            else
            {
              // Transform the SetTempo operation into an AddTempo.
              type = UndoOp::AddTempo;
            }
          }
          else
          {
            // a is the new tempo, b is the existing tempo.
            a = b;
            b = MusEGlobal::tempomap.staticTempo();
            // Transform the SetTempo operation into a SetStaticTempo.
            type = UndoOp::SetStaticTempo;
          }
        }
        break;
        
        case UndoOp::SetStaticTempo:
          // a is already the new tempo, b is the existing tempo.
          b = MusEGlobal::tempomap.staticTempo();
        break;
        
        case UndoOp::AddSig:
        {
          //if(t > MAX_TICK)
          //  t = MAX_TICK;
          
          // Must rasterize the tick value HERE instead of in SigMap::addOperation(),
          //  so that the rasterized value is recorded in the undo item.
          a = MusEGlobal::sigmap.raster1(a, 0);
          
          MusECore::iSigEvent ise = MusEGlobal::sigmap.upper_bound(a);
          if((int)ise->second->tick == a)
          {
            // Transform the AddSig operation into a ModifySig.
            // a is already the tick, b + c is the existing value, d + e is the new value.
            type = UndoOp::ModifySig;
            d = b;
            e = c;
            b = ise->second->sig.z;
            c = ise->second->sig.n;
          }
        }
        break;

        case UndoOp::AddKey:
        {
          int t = a;
          if(t > MAX_TICK)
            t = MAX_TICK;
          iKeyEvent ike = MusEGlobal::keymap.upper_bound(t);
          if((int)ike->second.tick == t)
          {
            // Transform the AddKey operation into a ModifyKey.
            // a is already the tick, b + c is the existing value, d + e is the new value.
            type = UndoOp::ModifyKey;
            d = b;
            e = c;
            b = ike->second.key;
            c = ike->second.minor;
          }
        }
        break;
        
        default:
        break;
      }
      
      }

UndoOp::UndoOp(UndoType type_, int tick, const MusECore::TimeSignature old_sig, const MusECore::TimeSignature new_sig, bool noUndo)
{
      assert(type_==ModifySig);
      type    = type_;
      a  = tick;
      b  = old_sig.z;
      c  = old_sig.n;
      d  = new_sig.z;
      e  = new_sig.n;
      _noUndo = noUndo;
}

UndoOp::UndoOp(UndoType type_, int n, const Track* track_, bool noUndo)
      {
      assert(type_==AddTrack || type_==DeleteTrack);
      assert(track_);
      
      type    = type_;
      trackno = n;
      track  = track_;
      _noUndo = noUndo;
      }

UndoOp::UndoOp(UndoType type_, const Part* part_, bool noUndo)
      {
      assert(type_==AddPart || type_==DeletePart);
      assert(part_);
      
      type  = type_;
      part = part_;
      _noUndo = noUndo;
      }
      
UndoOp::UndoOp(UndoType type_, const Part* part_, bool selected_, bool sel_old_, bool noUndo)
{
    assert(type_==SelectPart);
    assert(part_);
    
    type=type_;
    part = part_;
    selected=selected_;
    selected_old=sel_old_;
    _noUndo = noUndo;
}

UndoOp::UndoOp(UndoType type_, const Part* part_, unsigned int old_len_or_pos, unsigned int new_len_or_pos,
               Pos::TType new_time_type_, const Track* oTrack, const Track* nTrack, bool noUndo)
{
    assert(type_== MovePart);
    assert(part_);

    type = type_;
    part = part_;
    _noUndo = noUndo;
    track = nTrack;
    oldTrack = oTrack;
    // Make sure both tracks exist.
    if(!track && !oldTrack)
      track = oldTrack = part->track();
    else if(!oldTrack)
      oldTrack = track;
    else if(!track)
      track = oldTrack;
    assert(oldTrack);
    assert(track);
    old_partlen_or_pos = old_len_or_pos;
    new_partlen_or_pos = new_len_or_pos;
    switch(part->type())
    {
      case Pos::FRAMES:
        switch(new_time_type_)
        {
          case Pos::FRAMES:
          break;
          
          case Pos::TICKS:
            new_partlen_or_pos = MusEGlobal::tempomap.tick2frame(new_partlen_or_pos);
          break;  
        }
      break;
      
      case Pos::TICKS:
        switch(new_time_type_)
        {
          case Pos::FRAMES:
            new_partlen_or_pos = MusEGlobal::tempomap.frame2tick(new_partlen_or_pos);
          break;

          case Pos::TICKS:
          break;  
        }
      break;
    }
}


UndoOp::UndoOp(UndoType type_, const Part* part_, unsigned int old_pos, unsigned int new_pos, unsigned int old_len, unsigned int new_len,
               int64_t events_offset_, Pos::TType new_time_type_, bool noUndo)
{
    assert(type_ == ModifyPartStart);
    assert(part_);

    type = type_;
    part = part_;
    _noUndo = noUndo;
    events_offset = events_offset_;
    events_offset_time_type = new_time_type_;
    old_partlen_or_pos = old_pos;
    new_partlen_or_pos = new_pos;
    old_partlen = old_len;
    new_partlen = new_len;
}

UndoOp::UndoOp(UndoType type_, const Part* part_, unsigned int old_len, unsigned int new_len,
               int64_t events_offset_, Pos::TType new_time_type_, bool noUndo)
{
    assert(type_== ModifyPartLength);
    assert(part_);

    type = type_;
    part = part_;
    _noUndo = noUndo;
    events_offset = events_offset_;
    events_offset_time_type = new_time_type_;
    old_partlen_or_pos = old_len;
    new_partlen_or_pos = new_len;
}

UndoOp::UndoOp(UndoType type_, const Event& nev, const Event& oev, const Part* part_, bool doCtrls_, bool doClones_, bool noUndo)
      {
      assert(type_==ModifyEvent);
      assert(part_);
      
      type   = type_;
      nEvent = nev;
      oEvent = oev;
      part   = part_;
      doCtrls = doCtrls_;
      doClones = doClones_;
      _noUndo = noUndo;
      }

UndoOp::UndoOp(UndoType type_, const Event& nev, const Part* part_, bool a_, bool b_, bool noUndo)
      {
      assert(type_==DeleteEvent || type_==AddEvent || type_==SelectEvent);
      assert(part_);
      
      type   = type_;
      nEvent = nev;
      part   = part_;
      _noUndo = noUndo;
      if(type_==SelectEvent)
      {
        selected = a_;
        selected_old = b_;
      }
      else
      {
        doCtrls = a_;
        doClones = b_;
      }
      }
      
UndoOp::UndoOp(UndoType type_, const Marker& oldMarker_, const Marker& newMarker_, bool noUndo)
      {
      assert(type_==ModifyMarker);
      type    = type_;
      oldMarker  = new Marker(oldMarker_);
      newMarker = new Marker(newMarker_);
      _noUndo = noUndo;
      }

UndoOp::UndoOp(UndoType type_, const Marker& marker_, bool noUndo)
      {
      assert(type_==AddMarker || type_==DeleteMarker);
      type    = type_;
      oldMarker = newMarker = nullptr;
      Marker** mp = nullptr;
      if(type_== AddMarker)
        mp = &newMarker;
      else
        mp = &oldMarker;
      *mp = new Marker(marker_);
      _noUndo = noUndo;
      }

UndoOp::UndoOp(UndoType type_, const Marker& marker_, unsigned int new_pos, Pos::TType new_time_type, bool noUndo)
      {
      assert(type_==SetMarkerPos);
      type    = type_;
      oldMarker = new Marker(marker_);
      newMarker = new Marker(marker_);
      newMarker->setPosValue(new_pos, new_time_type);
      _noUndo = noUndo;
      }

UndoOp::UndoOp(UndoType type_, const Event& changedEvent, const QString& changeData, int startframe_, int endframe_, bool noUndo)
      {
      assert(type_==ModifyClip);
      
      type = type_;
      _noUndo = noUndo;
      nEvent = changedEvent;
      tmpwavfile = new QString(changeData);
      startframe = startframe_;
      endframe   = endframe_;
      }

UndoOp::UndoOp(UndoOp::UndoType type_, const Part* part_, const QString& old_name, const QString& new_name, bool noUndo)
{
    assert(type_==ModifyPartName);
    assert(part_);
    
    type=type_;
    part=part_;
    _noUndo = noUndo;
    _oldName = new QString(old_name);
    _newName = new QString(new_name);
}

UndoOp::UndoOp(UndoOp::UndoType type_, const Track* track_, const QString& old_name, const QString& new_name, bool noUndo)
{
  assert(type_==ModifyTrackName);
  assert(track_);
    
  type = type_;
  track = track_;
  _noUndo = noUndo;
  _oldName = new QString(old_name);
  _newName = new QString(new_name);
}

UndoOp::UndoOp(UndoType type_, int ctrlID, unsigned int frame, const CtrlVal& cv, const Track* track_, bool noUndo)
{
  assert(type_== AddAudioCtrlValStruct);
  assert(track_);

  type = type_;
  track = track_;
  _audioCtrlIdStruct = ctrlID;
  _audioCtrlFrameStruct = frame;
  _audioCtrlValStruct = new CtrlVal(cv);
  _noUndo = noUndo;
}

UndoOp::UndoOp(UndoOp::UndoType type_, const Track* track_, int ctrlID_, CtrlList* eraseCtrlList, CtrlList* addCtrlList,
               CtrlList* recoverableEraseCtrlList, CtrlList* recoverableAddCtrlList, CtrlList* doNotEraseCtrlList,
               bool noEndAudioCtrlMoveMode, bool noUndo)
{
  assert(type_== ModifyAudioCtrlValList);
  assert(track_);
  assert(eraseCtrlList || addCtrlList || recoverableEraseCtrlList || recoverableAddCtrlList || doNotEraseCtrlList);
  
  type = type_;
  track = track_;
  _audioCtrlIdModify = ctrlID_;
  _eraseCtrlList = eraseCtrlList;
  _addCtrlList = addCtrlList;
  _doNotEraseCtrlList = doNotEraseCtrlList;
  _recoverableEraseCtrlList = recoverableEraseCtrlList;
  _recoverableAddCtrlList = recoverableAddCtrlList;
  _noEndAudioCtrlMoveMode = noEndAudioCtrlMoveMode;
  _noUndo = noUndo;
}

UndoOp::UndoOp(UndoType type_, const Track* track_, double a_, double b_,
  double c_, double d_, double e_, bool noUndo_)
{
  assert(type_ == ModifyTrackChannel || type_ == DeleteAudioCtrlVal ||
    type_ == SetTrackRecord || type_ == SetTrackMute || type_ == SetTrackSolo ||
    type_ == SetTrackRecMonitor || type_ == SetTrackOff || type_ == AddAudioCtrlVal || type_ == ModifyAudioCtrlVal);
  assert(track_);

  type = type_;
  track = track_;

  if(type_ == ModifyTrackChannel)
  {
    _oldPropValue = a_;
    _newPropValue = b_;
  }
  else if(type_ == DeleteAudioCtrlVal)
  {
    _audioCtrlIdAddDel = a_;
    _audioCtrlFrameAddDel = b_;
    // Start this as initial. It will be filled by the execution stage.
    _audioCtrlValFlagsAddDel = CtrlVal::VAL_NOFLAGS;
  }
  else if(type_ == AddAudioCtrlVal)
  {
    _audioCtrlIdAddDel = a_;
    _audioCtrlFrameAddDel = b_;
    _audioCtrlValAddDel = c_;
    _audioCtrlValFlagsAddDel = d_;
  }
  else if(type_ == ModifyAudioCtrlVal)
  {
    _audioCtrlID = a_;
    _audioCtrlFrame = b_;
    _audioNewCtrlFrame = c_;
    _audioCtrlVal = d_;
    _audioNewCtrlVal = e_;
  }
  else
    a = a_;

  _noUndo = noUndo_;
}

UndoOp::UndoOp(UndoType type_, CtrlList* ctrlList_, unsigned int frame_, bool oldSelected_, bool newSelected_, bool noUndo_)
{
  assert(type_== SelectAudioCtrlVal);
  type = type_;
  _noUndo = noUndo_;
  _audioCtrlListSelect = ctrlList_;
  _audioCtrlSelectFrame = frame_;
  selected_old = oldSelected_;
  selected = newSelected_;
}

UndoOp::UndoOp(UndoType type_, CtrlList::PasteEraseOptions newOpts_, bool noUndo_)
{
  assert(type_== SetAudioCtrlPasteEraseMode);
  type = type_;
  _noUndo = noUndo_;
  _audioCtrlOldPasteEraseOpts = MusEGlobal::config.audioCtrlGraphPasteEraseOptions;
  _audioCtrlNewPasteEraseOpts = newOpts_;
}

UndoOp::UndoOp(UndoType type_, MidiPort* mp, MidiInstrument* instr, bool noUndo)
{
  assert(type_== SetInstrument);
  assert(mp);
  assert(instr);
  type = type_;
  _midiPort = mp;
  _oldMidiInstrument = _midiPort->instrument();
  _newMidiInstrument = instr;
  _noUndo = noUndo;
}

UndoOp::UndoOp(UndoOp::UndoType type_, bool noUndo)
{
  assert(type_== EnableAllAudioControllers || type_ == BeginAudioCtrlMoveMode || type_ == EndAudioCtrlMoveMode);
  type = type_;
  _noUndo = noUndo;
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
UndoOp::UndoOp(UndoOp::UndoType type_, const Route& route_from_, const Route& route_to_, bool noUndo)
      {
      assert(type_ == AddRoute || type_ == DeleteRoute);
      _noUndo = noUndo;
      routeFrom = new Route(route_from_);
      routeTo = new Route(route_to_);
      }
#pragma GCC diagnostic pop

//---------------------------------------------------------
//   addUndo
//---------------------------------------------------------

void Song::addUndo(UndoOp i)
      {
      if (!undoMode) {
            printf("internal error: undoOp without startUndo()\n");
            return;
            }
      undoList->back().push_back(i);
      emit sigDirty();
      }

//---------------------------------------------------------
//   revertOperationGroup1
//    non realtime context
//    return true if nothing to do
//---------------------------------------------------------

void Song::revertOperationGroup1(Undo& operations)
      {
      MarkerList* new_marker_list = nullptr;
      TempoList* new_tempo_list = nullptr;
      SigList* new_sig_list = nullptr;
      KeyList* new_key_list = nullptr;
      
      for (riUndoOp i = operations.rbegin(); i != operations.rend(); ++i) {
            Track* editable_track = const_cast<Track*>(i->track);
            Part* editable_part = const_cast<Part*>(i->part);
            switch(i->type) {
                  case UndoOp::SelectPart:
                        pendingOperations.add(PendingOperationItem(editable_part, i->selected_old, PendingOperationItem::SelectPart));
                        updateFlags |= SC_PART_SELECTION;
                        break;
                  case UndoOp::SelectEvent:
                        pendingOperations.add(PendingOperationItem(editable_part, i->nEvent, i->selected_old, PendingOperationItem::SelectEvent));
                        updateFlags |= SC_SELECTION;
                        break;
                        
                  case UndoOp::AddTrack:
                        switch(editable_track->type())
                        {
                          case Track::AUDIO_SOFTSYNTH:
                          {
                            SynthI* si = (SynthI*)editable_track;
                            if(si->hasGui())
                              si->showGui(false);
                            if(si->hasNativeGui())       
                              si->showNativeGui(false);
                          }// Fall through.
                          case Track::WAVE:
                          case Track::AUDIO_OUTPUT:
                          case Track::AUDIO_INPUT:
                          case Track::AUDIO_GROUP:
                          case Track::AUDIO_AUX:
                            ((AudioTrack*)editable_track)->deleteAllEfxGuis();
                            updateFlags |= SC_RACK;
                          break;
                          
                          default:
                          break;
                        }
                        
                        switch(editable_track->type())
                        {
                          case Track::AUDIO_OUTPUT:
                          {
                            AudioOutput* ao = (AudioOutput*)editable_track;
                            for(int ch = 0; ch < ao->channels(); ++ch)
                            {
                              MusEGlobal::audioDevice->unregisterPort(ao->jackPort(ch));
                              //ao->setJackPort(ch, 0);  // Done in RT stage.
                              updateFlags |= SC_ROUTE;
                            }
                          }
                          break;
                          
                          case Track::AUDIO_INPUT:
                          {
                            AudioOutput* ai = (AudioOutput*)editable_track;
                            for(int ch = 0; ch < ai->channels(); ++ch)
                            {
                              MusEGlobal::audioDevice->unregisterPort(ai->jackPort(ch));
                              //ai->setJackPort(ch, 0); // Done in RT stage.
                              updateFlags |= SC_ROUTE;
                            }
                          }     
                          break;
                          
                          case Track::AUDIO_AUX:
                            updateFlags |= SC_AUX;
                          break;
                          
                          default:
                          break;
                        }
                        removeTrackOperation(editable_track, pendingOperations);
                        updateFlags |= SC_TRACK_REMOVED;
                        break;
                        
                  case UndoOp::DeleteTrack:
                        switch(editable_track->type())
                        {
                          case Track::AUDIO_SOFTSYNTH:
                          {
                            SynthI* s = (SynthI*)editable_track;
                            Synth* sy = s->synth();
                            if(!s->sif() || !sy)
                            {
                              // Persistent storage: If the synth is not found allow the track to load.
                              // It's OK if sy is NULL. initInstance needs to do a few things.
                              s->initInstance(sy, s->name());
                            }
                            // FIXME TODO: We want to restore any ports using this instrument via the undo
                            //  system but ATM a few other things can set the instrument without an undo
                            //  operation so the undo sequence would not be correct. So we don't have much
                            //  choice but to just reset inside the PendingOperation::DeleteTrack operation
                            //  for now when deleting a synth track.
                            // Still, everything else is in place for undoable setting of instrument...
                          }
                          break;
                                
                          case Track::AUDIO_OUTPUT:
                          {
                            AudioOutput* ao = (AudioOutput*)editable_track;
                            if(MusEGlobal::checkAudioDevice())
                            {
                              for(int ch = 0; ch < ao->channels(); ++ch) 
                              {
                                // This should be OK since the track has not yet been added in the realtime stage.
                                if(ao->registerPorts(ch))
                                  updateFlags |= SC_ROUTE;
                                
                                // Set the route Jack ports now to relieve our graph callback handler from having to do it.
                                RouteList* rl = ao->outRoutes();
                                for(iRoute ir = rl->begin(); ir != rl->end(); ++ir)
                                  if(ir->type == Route::JACK_ROUTE && ir->channel == ch)
                                  {
                                    ir->jackPort = MusEGlobal::audioDevice->findPort(ir->persistentJackPortName);
                                    updateFlags |= SC_ROUTE;
                                  }
                              }
                            }
                          }
                          break;
                              
                          case Track::AUDIO_INPUT:
                          {
                            AudioInput* ai = (AudioInput*)editable_track;
                            if(MusEGlobal::checkAudioDevice())
                            {
                              for(int ch = 0; ch < ai->channels(); ++ch) 
                              {
                                // This should be OK since the track has not yet been added in the realtime stage.
                                if(ai->registerPorts(ch))
                                  updateFlags |= SC_ROUTE;
                                
                                // Set the route Jack ports now to relieve our graph callback handler from having to do it.
                                RouteList* rl = ai->inRoutes();
                                for(iRoute ir = rl->begin(); ir != rl->end(); ++ir)
                                  if(ir->type == Route::JACK_ROUTE && ir->channel == ch)
                                  {
                                    ir->jackPort = MusEGlobal::audioDevice->findPort(ir->persistentJackPortName);
                                    updateFlags |= SC_ROUTE;
                                  }
                              }
                            }
                          }
                          break;
                          
                          case Track::AUDIO_AUX:
                            updateFlags |= SC_AUX;
                          break;
                          
                          default:
                          break;
                        }
                    
                        // Ensure that wave event sndfile file handles are opened.
                        // It should not be the job of the pending operations list to do this.
                        // TODO Coordinate close/open with part mute and/or track off.
                        editable_track->openAllParts();
                        
                        insertTrackOperation(editable_track, i->trackno, pendingOperations);
                        updateFlags |= SC_TRACK_INSERTED;
                        break;
                        
                  case UndoOp::ModifyClip:
                        sndFileApplyUndoFile(i->nEvent, i->tmpwavfile, i->startframe, i->endframe);
                        updateFlags |= SC_CLIP_MODIFIED;
                        break;
                  case UndoOp::ModifyTrackChannel:
                        if (editable_track->isMidiTrack())
                        {
                          MusECore::MidiTrack* mt = static_cast<MusECore::MidiTrack*>(editable_track);
                          if (i->_oldPropValue != mt->outChannel())
                          {
                                MusECore::MidiTrack::ChangedType_t changed = MusECore::MidiTrack::NothingChanged;
                                MusEGlobal::audio->msgIdle(true);
                                changed |= mt->setOutChanAndUpdate(i->_oldPropValue, false);
                                MusEGlobal::audio->msgIdle(false);
                                MusEGlobal::audio->msgUpdateSoloStates();                   
                                updateFlags |= (SC_ROUTE | ((changed & MusECore::MidiTrack::DrumMapChanged) ? SC_DRUMMAP : 0));
                          }
                        }
                        else
                        {
                            if(editable_track->type() != MusECore::Track::AUDIO_SOFTSYNTH)
                            {
                              MusECore::AudioTrack* at = static_cast<MusECore::AudioTrack*>(editable_track);
                              if (i->_oldPropValue != at->channels()) {
                                    MusEGlobal::audio->msgSetChannels(at, i->_oldPropValue);
                                    updateFlags |= SC_CHANNELS;
                                    }
                            }         
                        }      
                        break;

                  case UndoOp::SetTrackRecord:
                        if(!editable_track->setRecordFlag1(!i->a))
                          break;
                        pendingOperations.add(PendingOperationItem(editable_track, !i->a, PendingOperationItem::SetTrackRecord));
                        // FIXME: No choice but to include monitor flag. Really should try to merge pending ops flags
                        //  with undo flags after executing the pending ops in revertOperationGroup3...
                        updateFlags |= (SC_RECFLAG | SC_TRACK_REC_MONITOR);
                        break;

                  case UndoOp::SetTrackMute:
                        pendingOperations.add(PendingOperationItem(editable_track, !i->a, PendingOperationItem::SetTrackMute));
                        updateFlags |= SC_MUTE;
                        break;

                  case UndoOp::SetTrackSolo:
                        pendingOperations.add(PendingOperationItem(editable_track, !i->a, PendingOperationItem::SetTrackSolo));
                        updateFlags |= SC_SOLO;
                        break;

                  case UndoOp::SetTrackRecMonitor:
                        pendingOperations.add(PendingOperationItem(editable_track, !i->a, PendingOperationItem::SetTrackRecMonitor));
                        updateFlags |= SC_TRACK_REC_MONITOR;
                        break;

                  case UndoOp::SetTrackOff:
                        pendingOperations.add(PendingOperationItem(editable_track, !i->a, PendingOperationItem::SetTrackOff));
                        updateFlags |= SC_MUTE;
                        break;

                        
                  case UndoOp::AddRoute:
#ifdef _UNDO_DEBUG_
                        fprintf(stderr, "Song::revertOperationGroup1:AddRoute\n");
#endif                        
                        pendingOperations.add(PendingOperationItem(*i->routeFrom, *i->routeTo, PendingOperationItem::DeleteRoute));
                        updateFlags |= SC_ROUTE;
                        break;
                        
                  case UndoOp::DeleteRoute:
#ifdef _UNDO_DEBUG_
                        fprintf(stderr, "Song::executeOperationGroup1:DeleteRoute\n");
#endif                        
                        pendingOperations.add(PendingOperationItem(*i->routeFrom, *i->routeTo, PendingOperationItem::AddRoute));
                        updateFlags |= SC_ROUTE;
                        break;
                        
                  case UndoOp::ModifyTrackName:
                        pendingOperations.add(PendingOperationItem(editable_track, i->_oldName, PendingOperationItem::ModifyTrackName));
                        updateFlags |= (SC_TRACK_MODIFIED | SC_MIDI_TRACK_PROP);
                        // If it's an aux track, notify aux UI controls to reload, or change their names etc.
                        if(editable_track->type() == Track::AUDIO_AUX)
                          updateFlags |= SC_AUX;
                        break;
                        
                  case UndoOp::MoveTrack:
                        pendingOperations.add(PendingOperationItem(&_tracks, i->b, i->a, PendingOperationItem::MoveTrack));
                        updateFlags |= SC_TRACK_MOVED;
                        break;
                        
                  case UndoOp::ModifyPartName:
                        pendingOperations.add(PendingOperationItem(editable_part, i->_oldName, PendingOperationItem::ModifyPartName));
                        updateFlags |= SC_PART_MODIFIED;
                        break;
                        
                  case UndoOp::ModifyPartLength: 
                        {
                        pendingOperations.modifyPartLengthOperation(
                          editable_part, i->old_partlen_or_pos, -i->events_offset, i->events_offset_time_type);
                        updateFlags |= SC_PART_MODIFIED;
                        // If the part had events, then treat it as if they were added/removed with separate Add/DeleteEvent operations.
                        // Even if they will be added/deleted later in this operations group with actual separate Add/DeleteEvent operations,
                        //  that's an SC_EVENT_ADDED/REMOVED anyway, so hopefully no harm.
                        if(i->events_offset != 0 && !editable_part->events().empty())
                          updateFlags |= (SC_EVENT_INSERTED | SC_EVENT_REMOVED | SC_EVENT_MODIFIED);
                        }
                        break;
                  case UndoOp::ModifyPartStart:
                        {
                        pendingOperations.modifyPartStartOperation(
                          editable_part, i->old_partlen_or_pos, i->old_partlen, -i->events_offset, i->events_offset_time_type);
                        updateFlags |= SC_PART_MODIFIED;
                        // If the part had events, then treat it as if they were added/removed with separate Add/DeleteEvent operations.
                        // Even if they will be added/deleted later in this operations group with actual separate Add/DeleteEvent operations,
                        //  that's an SC_EVENT_ADDED/REMOVED anyway, so hopefully no harm.
                        if(i->events_offset != 0 && !editable_part->events().empty())
                          updateFlags |= (SC_EVENT_INSERTED | SC_EVENT_REMOVED | SC_EVENT_MODIFIED);
                        }
                        break;

                  case UndoOp::MovePart:
#ifdef _UNDO_DEBUG_
                        fprintf(stderr, "Song::revertOperationGroup1:MovePart ** calling parts->movePartOperation\n");
#endif                        
                        pendingOperations.movePartOperation(editable_part->track()->parts(),
                          editable_part, i->old_partlen_or_pos, const_cast<Track*>(i->oldTrack));
                        if(const_cast<Track*>(i->oldTrack))
                          updateFlags |= SC_PART_INSERTED | SC_PART_REMOVED;
                        updateFlags |= SC_PART_MODIFIED;
                        break;
                        
                  case UndoOp::AddPart:
#ifdef _UNDO_DEBUG_
                        fprintf(stderr, "Song::revertOperationGroup1:AddPart ** calling parts->delOperation\n");
#endif                        
                        pendingOperations.delPartOperation(editable_part->track()->parts(), editable_part);
                        updateFlags |= SC_PART_REMOVED;
                        // If the part had events, then treat it as if they were removed with separate DeleteEvent operations.
                        // Even if they will be deleted later in this operations group with actual separate DeleteEvent operations,
                        //  that's an SC_EVENT_REMOVED anyway, so hopefully no harm. This fixes a problem with midi controller canvas
                        //  not updating after such a 'delete part with events, no separate AddEvents were used when creating the part'.
                        if(!editable_part->events().empty())
                          updateFlags |= SC_EVENT_REMOVED;
                        break;
                    
                  case UndoOp::DeletePart:
#ifdef _UNDO_DEBUG_
                        fprintf(stderr, "Song::revertOperationGroup1:DeletePart ** calling parts->addOperation\n");
#endif                        
                        // Ensure that wave event sndfile file handles are opened.
                        // It should not be the job of the pending operations list to do this.
                        // TODO Coordinate close/open with part mute and/or track off.
                        editable_part->openAllEvents();
                        
                        pendingOperations.addPartOperation(editable_part->track()->parts(), editable_part);
                        updateFlags |= SC_PART_INSERTED;
                        // If the part has events, then treat it as if they were inserted with separate AddEvent operations.
                        // Even if some will be inserted later in this operations group with actual separate AddEvent operations,
                        //  that's an SC_EVENT_INSERTED anyway, so should be no harm.
                        if(!editable_part->events().empty())
                          updateFlags |= SC_EVENT_INSERTED;
                        break;

                        
                  case UndoOp::AddEvent:
#ifdef _UNDO_DEBUG_
                        fprintf(stderr, "Song::revertOperationGroup1:AddEvent ** calling deleteEvent\n");
#endif                        
                        deleteEventOperation(i->nEvent, editable_part, i->doCtrls, i->doClones);
                        updateFlags |= SC_EVENT_REMOVED;
                        break;

                  case UndoOp::DeleteEvent:
                        {
#ifdef _UNDO_DEBUG_
                          fprintf(stderr, "Song::revertOperationGroup1:DeleteEvent ** calling addEvent\n");
#endif                        
                          if(!i->nEvent.empty())
                          {
                            SndFileR f = i->nEvent.sndFile();
                            // Ensure that wave event sndfile file handle is opened.
                            // It should not be the job of the pending operations list to do this.
                            // TODO Coordinate close/open with part mute and/or track off.
                            if(!f.isNull() && !f.isOpen())
                              f->openRead();
                          }
                          
                          addEventOperation(i->nEvent, editable_part, i->doCtrls, i->doClones);
                          updateFlags |= SC_EVENT_INSERTED;
                        }
                        break;
                        
                  case UndoOp::ModifyEvent:
#ifdef _UNDO_DEBUG_
                        fprintf(stderr, "Song::revertOperationGroup1:ModifyEvent ** calling changeEvent\n");
#endif                        
                        changeEventOperation(i->nEvent, i->oEvent, editable_part, i->doCtrls, i->doClones);
                        updateFlags |= SC_EVENT_MODIFIED;
                        break;

                        
                  case UndoOp::AddAudioCtrlVal:
                  {
#ifdef _UNDO_DEBUG_
                        fprintf(stderr, "Song::revertOperationGroup1:AddAudioCtrlVal\n");
#endif                        
                        CtrlListList* cll = static_cast<AudioTrack*>(editable_track)->controller();
                        iCtrlList icl = cll->find(i->_audioCtrlIdAddDel);
                        if(icl != cll->end())
                        {
                          CtrlList* cl = icl->second;
                          iCtrl ic = cl->find(i->_audioCtrlFrameAddDel);
                          if(ic != cl->end())
                          {
                            pendingOperations.add(PendingOperationItem(cl, ic, PendingOperationItem::DeleteAudioCtrlVal));
                            updateFlags |= SC_AUDIO_CONTROLLER;
                          }
                        }
                  }
                  break;

                  case UndoOp::AddAudioCtrlValStruct:
                  {
#ifdef _UNDO_DEBUG_
                        fprintf(stderr, "Song::revertOperationGroup1:AddAudioCtrlValStruct\n");
#endif
                        CtrlListList* cll = static_cast<AudioTrack*>(editable_track)->controller();
                        iCtrlList icl = cll->find(i->_audioCtrlIdStruct);
                        if(icl != cll->end())
                        {
                          CtrlList* cl = icl->second;
                          iCtrl ic = cl->find(i->_audioCtrlFrameStruct);
                          if(ic != cl->end())
                          {
                            pendingOperations.add(PendingOperationItem(cl, ic, PendingOperationItem::DeleteAudioCtrlVal));
                            updateFlags |= SC_AUDIO_CONTROLLER;
                          }
                        }
                  }
                  break;

                  case UndoOp::DeleteAudioCtrlVal:
                  {
#ifdef _UNDO_DEBUG_
                        fprintf(stderr, "Song::revertOperationGroup1:DeleteAudioCtrlVal\n");
#endif
                        CtrlListList* cll = static_cast<AudioTrack*>(editable_track)->controller();
                        iCtrlList icl = cll->find(i->_audioCtrlIdAddDel);
                        if(icl != cll->end())
                        {
                          // Restore the old value.
                          pendingOperations.add(PendingOperationItem(
                            icl->second, i->_audioCtrlFrameAddDel, i->_audioCtrlValAddDel,
                            i->_audioCtrlValFlagsAddDel, PendingOperationItem::AddAudioCtrlVal));
                          updateFlags |= SC_AUDIO_CONTROLLER;
                        }
                  }
                  break;

                  case UndoOp::ModifyAudioCtrlVal:
                  {
#ifdef _UNDO_DEBUG_
                        fprintf(stderr, "Song::revertOperationGroup1:ModifyAudioCtrlVal\n");
#endif                        
                        CtrlListList* cll = static_cast<AudioTrack*>(editable_track)->controller();
                        iCtrlList icl = cll->find(i->_audioCtrlID);
                        if(icl != cll->end())
                        {
                          CtrlList* cl = icl->second;
                          iCtrl ic = cl->find(i->_audioNewCtrlFrame);
                          if(ic != cl->end())
                          {
                            // Restore the old value.
                            pendingOperations.add(PendingOperationItem(icl->second, ic, i->_audioCtrlFrame, i->_audioCtrlVal, PendingOperationItem::ModifyAudioCtrlVal));
                            updateFlags |= SC_AUDIO_CONTROLLER;
                          }
                        }
                  }
                  break;
                        
                  case UndoOp::ModifyAudioCtrlValList:
                  {
#ifdef _UNDO_DEBUG_
                        fprintf(stderr, "Song::revertOperationGroup1:ModifyAudioCtrlValList\n");
#endif                        
                        if(!i->track->isMidiTrack())
                        {
                          AudioTrack* at = static_cast<AudioTrack*>(const_cast<Track*>(i->track));
                          const int id = i->_audioCtrlIdModify;
                          iCtrlList icl = at->controller()->find(id);
                          if(icl != at->controller()->end())
                          {
                            // Make a complete copy of the controller list. The list will be quickly switched in the realtime stage.
                            // The Pending Operations system will take 'ownership' of this and delete it at the appropriate time.
                            CtrlList* new_list = new CtrlList(*icl->second, CtrlList::ASSIGN_PROPERTIES | CtrlList::ASSIGN_VALUES);
                            bool new_list_changed = false;

                            // Erase any items in the add list that were added...
                            if(i->_addCtrlList)
                            {
                              // Erase any items in the new list found at the frames given by the add list.
                              for(ciCtrl ic = i->_addCtrlList->cbegin(); ic != i->_addCtrlList->cend(); ++ic)
                              {
                                if(new_list->erase(ic->first) != 0)
                                  new_list_changed = true;
                              }
                            }

                            if(i->_recoverableAddCtrlList)
                            {
                              // Erase any items in the new list found at the frames given by the recoverable add list.
                              for(ciCtrl ic = i->_recoverableAddCtrlList->cbegin(); ic != i->_recoverableAddCtrlList->cend(); ++ic)
                              {
                                if(new_list->erase(ic->first) != 0)
                                  new_list_changed = true;
                              }
                            }
                            // Re-add any items in the recoverable erase list that were erased.
                            if(i->_recoverableEraseCtrlList)
                            {
                              for(ciCtrl ic = i->_recoverableEraseCtrlList->cbegin(); ic != i->_recoverableEraseCtrlList->cend(); ++ic)
                              {
                                if(new_list->insert(CtrlListInsertPair_t(ic->first, ic->second)).second)
                                  new_list_changed = true;
                              }
                            }
                            if(i->_eraseCtrlList)
                            {
                              // Re-add any items in the erase list that were erased...
                              for(ciCtrl ic = i->_eraseCtrlList->cbegin(); ic != i->_eraseCtrlList->cend(); ++ic)
                              {
                                if(new_list->insert(CtrlListInsertPair_t(ic->first, ic->second)).second)
                                  new_list_changed = true;
                              }
                            }

                            const bool do_rel = i->_recoverableEraseCtrlList && !i->_recoverableEraseCtrlList->empty();
                            const bool do_ral = i->_recoverableAddCtrlList && !i->_recoverableAddCtrlList->empty();
                            if(do_rel || do_ral)
                            {
                              CtrlList* new_elist;
                              bool new_elist_changed = false;

                              iCtrlList iecl = at->erasedController()->find(id);
                              if(iecl == at->erasedController()->end())
                              {
                                if(do_rel)
                                {
                                  // Existing list not found. Make a complete copy of the desired erased controller list, to start with.
                                  // The Pending Operations system will take 'ownership' of this and delete it at the appropriate time.
                                  new_elist = new CtrlList(
                                    *i->_recoverableEraseCtrlList, CtrlList::ASSIGN_PROPERTIES | CtrlList::ASSIGN_VALUES);
                                  if(do_ral)
                                  {
                                    // Erase any items in new erased list found at the frames given by the the recoverable add list.
                                    for(ciCtrl ic = i->_recoverableAddCtrlList->cbegin(); ic != i->_recoverableAddCtrlList->cend(); ++ic)
                                      new_elist->erase(ic->first);
                                  }
                                  if(new_elist->empty())
                                  {
                                    // Nothing left. Delete the list.
                                    delete new_elist;
                                  }
                                  else
                                  {
                                    pendingOperations.add(PendingOperationItem(
                                      at->erasedController(), new_elist, PendingOperationItem::AddAudioCtrlValList));
                                    updateFlags |= SC_AUDIO_CONTROLLER_LIST;
                                  }
                                }
                              }
                              else
                              {
                                // Existing list found. Make a complete working copy of the existing erased controller list.
                                // The list will be quickly switched in the realtime stage.
                                // The Pending Operations system will take 'ownership' of this and delete it at the appropriate time.
                                new_elist = new CtrlList(*iecl->second, CtrlList::ASSIGN_PROPERTIES | CtrlList::ASSIGN_VALUES);
                                if(do_rel)
                                {
                                  // Erase any items in new erased list found at the frames given by the the recoverable erase list.
                                  for(ciCtrl ic = i->_recoverableEraseCtrlList->cbegin(); ic != i->_recoverableEraseCtrlList->cend(); ++ic)
                                  {
                                    if(new_elist->erase(ic->first) != 0)
                                      new_elist_changed = true;
                                  }
                                }
                                if(do_ral)
                                {
                                  // Re-add any items in the recoverable add list.
                                  for(ciCtrl ic = i->_recoverableAddCtrlList->cbegin(); ic != i->_recoverableAddCtrlList->cend(); ++ic)
                                  {
                                    if(new_elist->insert(CtrlListInsertPair_t(ic->first, ic->second)).second)
                                      new_elist_changed = true;
                                  }
                                }
                                if(new_elist_changed)
                                {
                                  pendingOperations.add(PendingOperationItem(iecl, new_elist, PendingOperationItem::ModifyAudioCtrlValList));
                                  updateFlags |= SC_AUDIO_CONTROLLER_LIST;
                                }
                                else
                                {
                                  delete new_elist;
                                }
                              }
                            }
                            if(i->_doNotEraseCtrlList && !i->_doNotEraseCtrlList->empty())
                            {
                              // Make a complete copy of the controller list. The list will be quickly switched in the realtime stage.
                              // The Pending Operations system will take 'ownership' of this and delete it at the appropriate time.
                              iCtrlList iecl = at->noEraseController()->find(id);
                              if(iecl != at->noEraseController()->end())
                              {
                                CtrlList* new_nelist = new CtrlList(*iecl->second, CtrlList::ASSIGN_PROPERTIES | CtrlList::ASSIGN_VALUES);
                                bool new_nelist_changed = false;
                                // Erase any items in the new list found at the frames given by the do not erase list.
                                for(ciCtrl ic = i->_doNotEraseCtrlList->cbegin(); ic != i->_doNotEraseCtrlList->cend(); ++ic)
                                {
                                  if(new_nelist->erase(ic->first) != 0)
                                  {
                                    new_nelist_changed = true;

                                    // Since we know that all these points were originally selected then unselected,
                                    //  re-select them again now, within the new main list.
                                    iCtrl ic_e = new_list->find(ic->first);
                                    if(ic_e != new_list->end() && !ic_e->second.selected())
                                    {
                                      ic_e->second.setSelected(true);
                                      new_list_changed = true;
                                      updateFlags |= SC_AUDIO_CONTROLLER_SELECTION;
                                    }
                                  }
                                }
                                if(new_nelist_changed)
                                {
                                  pendingOperations.add(PendingOperationItem(iecl, new_nelist, PendingOperationItem::ModifyAudioCtrlValList));
                                  updateFlags |= SC_AUDIO_CONTROLLER_LIST;
                                }
                                else
                                {
                                  delete new_nelist;
                                }
                              }
                            }

                            if(new_list_changed)
                            {
                              // The operation will quickly switch the list in the RT stage then the delete the old list in the non-RT stage.
                              pendingOperations.add(PendingOperationItem(icl, new_list, PendingOperationItem::ModifyAudioCtrlValList));
                              updateFlags |= SC_AUDIO_CONTROLLER_LIST;
                            }
                            else
                            {
                              delete new_list;
                            }
                          }
                        }
                  }
                  break;

                  case UndoOp::SelectAudioCtrlVal:
                  {
#ifdef _UNDO_DEBUG_
                        fprintf(stderr, "Song::revertOperationGroup1:SelectAudioCtrlVal\n");
#endif
                        if(i->_audioCtrlListSelect)
                        {
                          iCtrl ic = i->_audioCtrlListSelect->find(i->_audioCtrlSelectFrame);
                          if(ic != i->_audioCtrlListSelect->end())
                          {
                            // Restore the old value.
                            pendingOperations.add(PendingOperationItem(
                              &ic->second, i->selected_old, PendingOperationItem::SelectAudioCtrlVal));
                          }
                        }
                        // Update always, just in case something was not found, a refresh might help.
                        updateFlags |= SC_AUDIO_CONTROLLER_SELECTION;
                  }
                  break;

                  case UndoOp::SetAudioCtrlPasteEraseMode:
                  {
#ifdef _UNDO_DEBUG_
                        fprintf(stderr, "Song::revertOperationGroup1:SetAudioCtrlPasteEraseMode\n");
#endif
                        // Restore the old value.
                        pendingOperations.add(PendingOperationItem(
                          i->_audioCtrlOldPasteEraseOpts, PendingOperationItem::SetAudioCtrlPasteEraseMode));
                        updateFlags |= SC_AUDIO_CTRL_PASTE_ERASE_MODE;
                  }
                  break;

                  case UndoOp::BeginAudioCtrlMoveMode:
                  {
#ifdef _UNDO_DEBUG_
                        fprintf(stderr, "Song::revertOperationGroup1:BeginAudioCtrlMoveMode\n");
#endif
                        // If already set, reset the flag to indicate outside of Begin/EndAudioCtrlMoveMode area.
                        if(_audioCtrlMoveModeBegun)
                        {
                          // Reset the flag.
                          _audioCtrlMoveModeBegun = false;
                          updateFlags |= SC_AUDIO_CTRL_MOVE_MODE;
                        }
                  }
                  break;
                  case UndoOp::EndAudioCtrlMoveMode:
                  {
#ifdef _UNDO_DEBUG_
                        fprintf(stderr, "Song::revertOperationGroup1:EndAudioCtrlMoveMode\n");
#endif
                        // We are crossing a boundary from move mode off to on.
                        // If selections are NOT undoable, we must manually unselect all points before entering this mode.
                        // This is to prevent points that were selected AFTER the move mode from becoming part of the move.
                        // The original move-mode selection state is needed and we can't know it because it isn't undoable.
                        if(!MusEGlobal::config.selectionsUndoable)
                        {
                          // TODO: Make this a single Pending Operations command to avoid clogging up
                          //        the Pending Operations list with potentially thousands of selections.
                          //       There might even be a slight increase in speed since the Pending Operations
                          //        would unselect thousands of points in ONE loop instead of thousands of items
                          //        in an operations list.
                          for (MusECore::ciTrack it = tracks()->cbegin(); it != tracks()->cend(); ++it) {
                            if((*it)->isMidiTrack())
                              continue;
                            // Include all tracks.
                            MusECore::AudioTrack* track = static_cast<MusECore::AudioTrack*>(*it);
                            const MusECore::CtrlListList* cll = track->controller();
                            for(MusECore::ciCtrlList icll = cll->cbegin(); icll != cll->cend(); ++icll)
                            {
                              // Include all controllers.
                              MusECore::CtrlList* cl = icll->second;
                              for(MusECore::iCtrl ic = cl->begin(); ic != cl->end(); ++ ic)
                              {
                                MusECore::CtrlVal& cv = ic->second;
                                // Include only already selected controller values.
                                if(!cv.selected())
                                  continue;

                                pendingOperations.add(PendingOperationItem(
                                  &ic->second, false, PendingOperationItem::SelectAudioCtrlVal));
                                updateFlags |= SC_AUDIO_CONTROLLER_SELECTION;
                              }
                            }
                          }
                        }

                        // If not already set, set the flag to indicate re-entering Begin/EndAudioCtrlMoveMode area.
                        if(!_audioCtrlMoveModeBegun)
                        {
                          // Set the flag.
                          _audioCtrlMoveModeBegun = true;
                          updateFlags |= SC_AUDIO_CTRL_MOVE_MODE;
                          // Rebuild the containers.
                          if(undoAudioCtrlMoveEnd(pendingOperations))
                            updateFlags |= SC_AUDIO_CONTROLLER_LIST;
                        }
                  }
                  break;


                  case UndoOp::SetInstrument:
                  {
#ifdef _UNDO_DEBUG_
                        fprintf(stderr, "Song::revertOperationGroup1:SetInstrument\n");
#endif                        
                        // Restore the old value.
                        pendingOperations.add(PendingOperationItem(
                          i->_midiPort, i->_oldMidiInstrument,
                          PendingOperationItem::SetInstrument));
                        updateFlags |= SC_MIDI_INSTRUMENT;
                  }
                  break;


                  case UndoOp::DeleteTempo:
#ifdef _UNDO_DEBUG_
                        fprintf(stderr, "Song::revertOperationGroup1:DeleteTempo ** calling tempomap.addOperation tick:%d tempo:%d\n", i->a, i->b);
#endif                        
                        // Create the new list if it doesn't already exist.
                        // Make a copy of the original list.
                        if(!new_tempo_list)
                        {
                          new_tempo_list = new TempoList();
                          new_tempo_list->copy(MusEGlobal::tempomap);
                        }
                        // Defer normalize until end of stage 2.
                        new_tempo_list->addTempo(i->a, i->b, false);

                        updateFlags |= SC_TEMPO;
                        break;
                        
                  case UndoOp::AddTempo:
#ifdef _UNDO_DEBUG_
                        fprintf(stderr, "Song::revertOperationGroup1:AddTempo ** calling tempomap.delOperation tick:%d\n", i->a);
#endif                        
                        // Create the new list if it doesn't already exist.
                        // Make a copy of the original list.
                        if(!new_tempo_list)
                        {
                          new_tempo_list = new TempoList();
                          new_tempo_list->copy(MusEGlobal::tempomap);
                        }
                        // Defer normalize until end of stage 2.
                        new_tempo_list->delTempo(i->a, false);

                        updateFlags |= SC_TEMPO;
                        break;
                        
                  case UndoOp::ModifyTempo:
#ifdef _UNDO_DEBUG_
                        fprintf(stderr, "Song::revertOperationGroup1:ModifyTempo ** calling tempomap.addOperation tick:%d tempo:%d\n", i->a, i->b);
#endif                        
                        // Create the new list if it doesn't already exist.
                        // Make a copy of the original list.
                        if(!new_tempo_list)
                        {
                          new_tempo_list = new TempoList();
                          new_tempo_list->copy(MusEGlobal::tempomap);
                        }
                        // Defer normalize until end of stage 2.
                        new_tempo_list->addTempo(i->a, i->b, false);

                        updateFlags |= SC_TEMPO;
                        break;
                        
//                   case UndoOp::SetTempo:
//                         // Only if the master is on.
//                         if(MusEGlobal::tempomap.masterFlag())
//                         {
// #ifdef _UNDO_DEBUG_
//                           fprintf(stderr, "Song::revertOperationGroup1:SetTempo ** calling tempomap.delOperation tick:%d\n", i->a);
// #endif                        
//                           MusEGlobal::tempomap.delOperation(i->a, pendingOperations);
//                         }
//                         else
//                           pendingOperations.add(PendingOperationItem(&MusEGlobal::tempomap, i->b, PendingOperationItem::SetStaticTempo));
//                         updateFlags |= SC_TEMPO;
//                         break;
                        
                  case UndoOp::SetStaticTempo:
#ifdef _UNDO_DEBUG_
                        fprintf(stderr, "Song::revertOperationGroup1:SetStaticTempo ** adding SetStaticTempo operation\n");
#endif                        
                        pendingOperations.add(PendingOperationItem(&MusEGlobal::tempomap, i->b, PendingOperationItem::SetStaticTempo));
                        updateFlags |= SC_TEMPO;
                        break;
                        
                  case UndoOp::SetGlobalTempo:
#ifdef _UNDO_DEBUG_
                        fprintf(stderr, "Song::revertOperationGroup1:SetGlobalTempo ** adding SetGlobalTempo operation\n");
#endif                        
                        pendingOperations.add(PendingOperationItem(&MusEGlobal::tempomap, i->b, PendingOperationItem::SetGlobalTempo));
                        updateFlags |= SC_TEMPO;
                        break;
                        
                  case UndoOp::EnableMasterTrack:
#ifdef _UNDO_DEBUG_
                        fprintf(stderr, "Song::revertOperationGroup1:EnableMasterTrack ** adding EnableMasterTrack operation\n");
#endif                        
                        pendingOperations.add(PendingOperationItem(&MusEGlobal::tempomap, (bool)i->b, PendingOperationItem::SetUseMasterTrack));
                        updateFlags |= SC_MASTER;
                        break;
                        
                  case UndoOp::DeleteSig:
#ifdef _UNDO_DEBUG_
                        fprintf(stderr, "Song::revertOperationGroup1:DeleteSig ** calling MusEGlobal::sigmap.addOperation\n");
#endif                        
                        // Create the new list if it doesn't already exist.
                        // Make a copy of the original list.
                        if(!new_sig_list)
                        {
                          new_sig_list = new SigList();
                          new_sig_list->copy(MusEGlobal::sigmap);
                        }
                        // Defer normalize until end of stage 2.
                        new_sig_list->add(i->a, MusECore::TimeSignature(i->b, i->c), false);

                        updateFlags |= SC_SIG;
                        break;
                        
                  case UndoOp::AddSig:
#ifdef _UNDO_DEBUG_
                        fprintf(stderr, "Song::revertOperationGroup1:AddSig ** calling MusEGlobal::sigmap.delOperation\n");
#endif                        
                        // Create the new list if it doesn't already exist.
                        // Make a copy of the original list.
                        if(!new_sig_list)
                        {
                          new_sig_list = new SigList();
                          new_sig_list->copy(MusEGlobal::sigmap);
                        }
                        // Defer normalize until end of stage 2.
                        new_sig_list->del(i->a, false);

                        updateFlags |= SC_SIG;
                        break;
                        
                  case UndoOp::ModifySig:
#ifdef _UNDO_DEBUG_
                        fprintf(stderr, "Song::revertOperationGroup1:ModifySig ** calling MusEGlobal::sigmap.addOperation\n");
#endif                        
                        // Create the new list if it doesn't already exist.
                        // Make a copy of the original list.
                        if(!new_sig_list)
                        {
                          new_sig_list = new SigList();
                          new_sig_list->copy(MusEGlobal::sigmap);
                        }
                        // Defer normalize until end of stage 2.
                        // TODO: Hm should that be ->d and ->e like in executeOperationGroup1?
                        new_sig_list->add(i->a, MusECore::TimeSignature(i->b, i->c), false);

                        updateFlags |= SC_SIG;
                        break;
                        

                  case UndoOp::DeleteKey:
#ifdef _UNDO_DEBUG_
                        fprintf(stderr, "Song::revertOperationGroup1:DeleteKey ** calling keymap.addOperation\n");
#endif                        
                        // Create the new list if it doesn't already exist.
                        // Make a copy of the original list.
                        if(!new_key_list)
                        {
                          new_key_list = new KeyList();
                          new_key_list->copy(MusEGlobal::keymap);
                        }
                        new_key_list->addKey(i->a, key_enum(i->b), i->c);

                        updateFlags |= SC_KEY;
                        break;

                  case UndoOp::AddKey:
#ifdef _UNDO_DEBUG_
                        fprintf(stderr, "Song::revertOperationGroup1:AddKey ** calling keymap.delOperation\n");
#endif                        
                        // Create the new list if it doesn't already exist.
                        // Make a copy of the original list.
                        if(!new_key_list)
                        {
                          new_key_list = new KeyList();
                          new_key_list->copy(MusEGlobal::keymap);
                        }
                        new_key_list->delKey(i->a);

                        updateFlags |= SC_KEY;
                        break;
                        
                  case UndoOp::ModifyKey:
#ifdef _UNDO_DEBUG_
                        fprintf(stderr, "Song::revertOperationGroup1:ModifyKey ** calling keymap.addOperation\n");
#endif                        
                        // Create the new list if it doesn't already exist.
                        // Make a copy of the original list.
                        if(!new_key_list)
                        {
                          new_key_list = new KeyList();
                          new_key_list->copy(MusEGlobal::keymap);
                        }
                        // TODO: Hm should that be ->d and ->e like in executeOperationGroup1?
                        new_key_list->addKey(i->a, key_enum(i->b), i->c);

                        updateFlags |= SC_KEY;
                        break;
                        
                  case UndoOp::ModifySongLen:
#ifdef _UNDO_DEBUG_
                        fprintf(stderr, "Song::revertOperationGroup1:ModifySongLen ** adding ModifySongLen operation\n");
#endif                        
                        pendingOperations.add(PendingOperationItem(i->b, PendingOperationItem::ModifySongLength));
                        updateFlags |= SC_EVERYTHING;  // set all flags   // TODO Refine this! Too many flags.  // REMOVE Tim.
                        //updateFlags |= SC_SONG_LEN;
                        break;
                        
                  case UndoOp::ModifyMidiDivision:
#ifdef _UNDO_DEBUG_
                        fprintf(stderr, "Song::revertOperationGroup1:ModifyMidiDivision\n");
#endif                  
                        MusEGlobal::config.division = i->b;
                        // Make sure the AL namespace variable mirrors our variable.
                        AL::division = MusEGlobal::config.division;
                        // Defer normalize until end of stage 2.
                        updateFlags |= SC_DIVISION_CHANGED;
                        break;
                        
                  case UndoOp::AddMarker:
                          // Create the new list if it doesn't already exist.
                          // Make a copy of the original list.
                          if(!new_marker_list)
                            new_marker_list = new MarkerList(*marker());
                          if(i->newMarker)
                            new_marker_list->remove(*i->newMarker);
                          updateFlags |= SC_MARKER_REMOVED;
                        break;
                  
                  case UndoOp::DeleteMarker:
                          // Create the new list if it doesn't already exist.
                          // Make a copy of the original list.
                          if(!new_marker_list)
                            new_marker_list = new MarkerList(*marker());
                          if(i->oldMarker)
                            new_marker_list->add(*i->oldMarker);
                          updateFlags |= SC_MARKER_INSERTED;
                        break;
                  
                  case UndoOp::ModifyMarker:
                  case UndoOp::SetMarkerPos:
                          // Create the new list if it doesn't already exist.
                          // Make a copy of the original list.
                          if(!new_marker_list)
                            new_marker_list = new MarkerList(*marker());
                          if(i->newMarker)
                            new_marker_list->remove(*i->newMarker);
                          if(i->oldMarker)
                            new_marker_list->add(*i->oldMarker);
                          updateFlags |= SC_MARKER_MODIFIED;
                        break;

                  default:
                        break;
                  }
            }

      if(new_tempo_list)
        pendingOperations.add(PendingOperationItem(&MusEGlobal::tempomap, new_tempo_list, PendingOperationItem::ModifyTempoList));

      if(new_sig_list)
        pendingOperations.add(PendingOperationItem(&MusEGlobal::sigmap, new_sig_list, PendingOperationItem::ModifySigList));

      if(new_key_list)
        pendingOperations.add(PendingOperationItem(&MusEGlobal::keymap, new_key_list, PendingOperationItem::ModifyKeyList));

      if(new_marker_list)
        pendingOperations.add(PendingOperationItem(&_markerList, new_marker_list, PendingOperationItem::ModifyMarkerList));
      }

//---------------------------------------------------------
//   revertOperationGroup3
//    non realtime context
//---------------------------------------------------------

void Song::revertOperationGroup3(Undo& operations)
      {
      pendingOperations.executeNonRTStage();
#ifdef _UNDO_DEBUG_
      fprintf(stderr, "Song::revertOperationGroup3 *** Calling pendingOperations.clear()\n");
#endif      
      pendingOperations.clear();
      for (riUndoOp i = operations.rbegin(); i != operations.rend(); ++i) {
            Track* editable_track = const_cast<Track*>(i->track);
// uncomment if needed            Track* editable_property_track = const_cast<Track*>(i->_propertyTrack);
            Part* editable_part = const_cast<Part*>(i->part); // uncomment if needed
            switch(i->type) {
                  case UndoOp::AddTrack:
                        // Ensure that wave event sndfile file handles are closed.
                        // It should not be the job of the pending operations list to do this.
                        // TODO Coordinate close/open with part mute and/or track off.
                        editable_track->closeAllParts();
                        break;
                  case UndoOp::DeleteTrack:
                        switch(editable_track->type())
                        {
                          case Track::AUDIO_OUTPUT:
                            // Connect audio output ports to Jack ports...
                            if(MusEGlobal::checkAudioDevice() && MusEGlobal::audio->isRunning())
                            {
                              AudioOutput* ao = (AudioOutput*)editable_track;
                              for(int ch = 0; ch < ao->channels(); ++ch) 
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
                                  //if(ir->jackPort)
                                  if(!MusEGlobal::audioDevice->findPort(route_name))
                                    continue;
                                  //if(!MusEGlobal::audioDevice->portConnectedTo(our_port, route_name))
                                  MusEGlobal::audioDevice->connect(our_port_name, route_name);
                                  updateFlags |= SC_ROUTE;
                                }
                              }
                            }
                          break;
                          
                          case Track::AUDIO_INPUT:
                            // Connect Jack ports to audio input ports...
                            if(MusEGlobal::checkAudioDevice() && MusEGlobal::audio->isRunning())
                            {
                              AudioInput* ai = (AudioInput*)editable_track;
                              for(int ch = 0; ch < ai->channels(); ++ch) 
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
                                  //if(ir->jackPort)
                                  if(!MusEGlobal::audioDevice->findPort(route_name))
                                    continue;
                                  //if(!MusEGlobal::audioDevice->portConnectedTo(our_port, route_name))
                                  MusEGlobal::audioDevice->connect(route_name, our_port_name);
                                  updateFlags |= SC_ROUTE;
                                }
                              }
                            }
                          break;
                            
                          default:
                            break;
                        }
                        
                        break;
                  case UndoOp::AddPart:
                        // Ensure that wave event sndfile file handles are closed.
                        // It should not be the job of the pending operations list to do this.
                        // TODO Coordinate close/open with part mute and/or track off.
                        editable_part->closeAllEvents();
                        break;
                  case UndoOp::AddEvent: {
                        if(!i->nEvent.empty())
                        {
                          SndFileR f = i->nEvent.sndFile();
                          // Ensure that wave event sndfile file handle is closed.
                          // It should not be the job of the pending operations list to do this.
                          // TODO Coordinate close/open with part mute and/or track off.
                          if(!f.isNull() && f.isOpen())
                            f->close(); 
                        }
                        }
                        break;
                  case UndoOp::ModifyMidiDivision:
                        // This also tells all connected models to begin/end reset.
                        MusEGlobal::globalRasterizer->setDivision(i->b);
                        break;

                  // Re-enable any controllers so that the results can be seen.
                  // TODO: This really should be done in the realtime operations stage.
                  //       But we don't pass the track to that stage. It would need the track.
                  //       Or make a separate EnableAudioController operations command,
                  //        but that's one extra command clogging up the commands list for
                  //        each one of these...
                  case UndoOp::AddAudioCtrlVal:
                  case UndoOp::DeleteAudioCtrlVal:
                    if(editable_track && !editable_track->isMidiTrack())
                    {
                      AudioTrack* audio_track = static_cast<AudioTrack*>(editable_track);
                      const AutomationType automation_type = audio_track->automationType();
                      if(automation_type != AUTO_WRITE && automation_type != AUTO_LATCH)
                        audio_track->enableController(i->_audioCtrlIdAddDel, true);
                    }
                  break;
                  case UndoOp::AddAudioCtrlValStruct:
                    if(editable_track && !editable_track->isMidiTrack())
                    {
                      AudioTrack* audio_track = static_cast<AudioTrack*>(editable_track);
                      const AutomationType automation_type = audio_track->automationType();
                      if(automation_type != AUTO_WRITE && automation_type != AUTO_LATCH)
                        audio_track->enableController(i->_audioCtrlIdStruct, true);
                    }
                  break;
                  case UndoOp::ModifyAudioCtrlVal:
                    if(editable_track && !editable_track->isMidiTrack())
                    {
                      AudioTrack* audio_track = static_cast<AudioTrack*>(editable_track);
                      const AutomationType automation_type = audio_track->automationType();
                      if(automation_type != AUTO_WRITE && automation_type != AUTO_LATCH)
                        audio_track->enableController(i->_audioCtrlID, true);
                    }
                  break;
                  case UndoOp::ModifyAudioCtrlValList:
                    if(editable_track && !editable_track->isMidiTrack())
                    {
                      AudioTrack* audio_track = static_cast<AudioTrack*>(editable_track);
                      const AutomationType automation_type = audio_track->automationType();
                      if(automation_type != AUTO_WRITE && automation_type != AUTO_LATCH)
                        audio_track->enableController(i->_audioCtrlIdModify, true);
                    }
                  break;

                  default:
                        break;
                  }
            }
            
            if(!operations.empty())
              emit sigDirty();
      }

//---------------------------------------------------------
//   executeOperationGroup1
//    non realtime context
//    return true if nothing to do
//---------------------------------------------------------

void Song::executeOperationGroup1(Undo& operations)
      {
      unsigned song_len = len();
      MarkerList* new_marker_list = nullptr;
      TempoList* new_tempo_list = nullptr;
      SigList* new_sig_list = nullptr;
      KeyList* new_key_list = nullptr;
        
      for (iUndoOp i = operations.begin(); i != operations.end(); ++i) {
            Track* editable_track = const_cast<Track*>(i->track);
            Part* editable_part = const_cast<Part*>(i->part);

            // Special: Check if audio controller move mode needs to be ended.
            switch(i->type) {
                  // Ignore BeginAudioCtrlMoveMode. Don't let it end the mode.
                  case UndoOp::BeginAudioCtrlMoveMode:
                  // Ignore EndAudioCtrlMoveMode. Let the switch below handle it.
                  case UndoOp::EndAudioCtrlMoveMode:
                  // Ignore ModifyAudioCtrlValList. Let the switch below handle it,
                  //  since for it ending the move mode is optional.
                  case UndoOp::ModifyAudioCtrlValList:
                  // Ignore SetAudioCtrlPasteEraseMode. Don't let it end the mode.
                  case UndoOp::SetAudioCtrlPasteEraseMode:
                  // Ignore EnableAllAudioControllers. No need to let it end the mode.
                  case UndoOp::EnableAllAudioControllers:
                  // Ignore MovePart. Don't let it end the mode. Moving a part is
                  //  allowed together with moving audio automation points.
                  case UndoOp::MovePart:
                  // Ignore DoNothing. No need to let it end the mode.
                  case UndoOp::DoNothing:
                        break;

                  // FOR ALL OTHERS: If we are in audio controller move mode, end the move mode !
                  default:
                        //checkAndEndAudioCtrlMoveMode(i, operations);
                        if(_audioCtrlMoveModeBegun)
                        {
                          // Insert an EndAudioCtrlMoveMode operation BEFORE the given iterator. If insert finds an existing EndAudioCtrlMoveMode,
                          //  possibly long before this one, it IGNORES this one.
                          operations.insert(i, UndoOp(UndoOp::EndAudioCtrlMoveMode));
                          // Since the EndAudioCtrlMoveMode above will not be iterated now, act like the operation had just been iterated.
                          // The same IGNORE rules apply here.
                          // Reset the flag to indicate an EndAudioCtrlMoveMode was encountered.
                          _audioCtrlMoveModeBegun = false;
                          updateFlags |= SC_AUDIO_CTRL_MOVE_MODE;
                          // End the move mode.
                          if(audioCtrlMoveEnd(pendingOperations))
                            updateFlags |= SC_AUDIO_CONTROLLER_LIST;
                        }
                        break;
            }

            switch(i->type) {
                  case UndoOp::SelectPart:
                        pendingOperations.add(PendingOperationItem(editable_part, i->selected, PendingOperationItem::SelectPart));
                        updateFlags |= SC_PART_SELECTION;
                        break;
                  case UndoOp::SelectEvent:
                        pendingOperations.add(PendingOperationItem(editable_part, i->nEvent, i->selected, PendingOperationItem::SelectEvent));
                        updateFlags |= SC_SELECTION;
                        break;
                        
                  case UndoOp::AddTrack:
                        switch(editable_track->type())
                        {
                          case Track::AUDIO_SOFTSYNTH:
                          {
                            SynthI* s = (SynthI*)editable_track;
                            Synth* sy = s->synth();
                            if(!s->sif() || !sy)
                              // Persistent storage: If the synth is not found allow the track to load.
                              // It's OK if sy is NULL. initInstance needs to do a few things.
                              s->initInstance(sy, s->name());
                          }
                          break;
                                
                          case Track::AUDIO_OUTPUT:
                          {
                            AudioOutput* ao = (AudioOutput*)editable_track;
                            if(MusEGlobal::checkAudioDevice())
                            {
                              for(int ch = 0; ch < ao->channels(); ++ch) 
                              {
                                // This should be OK since the track has not yet been added in the realtime stage.
                                if(ao->registerPorts(ch))
                                  updateFlags |= SC_ROUTE;
                                
                                // Set the route Jack ports now to relieve our graph callback handler from having to do it.
                                RouteList* rl = ao->outRoutes();
                                for(iRoute ir = rl->begin(); ir != rl->end(); ++ir)
                                  if(ir->type == Route::JACK_ROUTE && ir->channel == ch)
                                  {
                                    ir->jackPort = MusEGlobal::audioDevice->findPort(ir->persistentJackPortName);
                                    updateFlags |= SC_ROUTE;
                                  }
                              }
                            }
                            
                            
                          }
                          break;
                              
                          case Track::AUDIO_INPUT:
                          {
                            AudioInput* ai = (AudioInput*)editable_track;
                            if(MusEGlobal::checkAudioDevice())
                            {
                              for(int ch = 0; ch < ai->channels(); ++ch) 
                              {
                                // This should be OK since the track has not yet been added in the realtime stage.
                                if(ai->registerPorts(ch))
                                  updateFlags |= SC_ROUTE;
                                
                                // Set the route Jack ports now to relieve our graph callback handler from having to do it.
                                RouteList* rl = ai->inRoutes();
                                for(iRoute ir = rl->begin(); ir != rl->end(); ++ir)
                                  if(ir->type == Route::JACK_ROUTE && ir->channel == ch)
                                  {
                                    ir->jackPort = MusEGlobal::audioDevice->findPort(ir->persistentJackPortName);
                                    updateFlags |= SC_ROUTE;
                                  }
                              }
                            }
                          }
                          break;
                          
                          case Track::AUDIO_AUX:
                            updateFlags |= SC_AUX;
                          break;
                          
                          default:
                          break;
                        }
                        
                        // Ensure that wave event sndfile file handles are opened.
                        // It should not be the job of the pending operations list to do this.
                        // TODO Coordinate close/open with part mute and/or track off.
                        editable_track->openAllParts();

                        insertTrackOperation(editable_track, i->trackno, pendingOperations);
                        updateFlags |= SC_TRACK_INSERTED;
                        break;
                        
                  case UndoOp::DeleteTrack:
                        switch(editable_track->type())
                        {
                          case Track::AUDIO_SOFTSYNTH:
                          {
                            SynthI* si = (SynthI*)editable_track;
                            if(si->hasGui())
                              si->showGui(false);
                            if(si->hasNativeGui())       
                              si->showNativeGui(false);
                            // FIXME TODO: We want to clear any ports using this instrument AND make it
                            //  undoable but ATM a few other things can set the instrument without an undo
                            //  operation so the undo sequence would not be correct. So we don't have much
                            //  choice but to just reset inside the PendingOperation::DeleteTrack operation for now.
                            // Still, everything else is in place for undoable setting of instrument...
                          }// Fall through.
                          case Track::WAVE:
                          case Track::AUDIO_OUTPUT:
                          case Track::AUDIO_INPUT:
                          case Track::AUDIO_GROUP:
                          case Track::AUDIO_AUX:
                            ((AudioTrack*)editable_track)->deleteAllEfxGuis();
                            updateFlags |= SC_RACK;
                          break;
                          
                          default:
                          break;
                        }
                        
                        switch(editable_track->type())
                        {
                          case Track::AUDIO_OUTPUT:
                          {
                            AudioOutput* ao = (AudioOutput*)editable_track;
                            for(int ch = 0; ch < ao->channels(); ++ch)
                            {
                              MusEGlobal::audioDevice->unregisterPort(ao->jackPort(ch));
                              //ao->setJackPort(ch, 0);  // Done in RT stage.
                              updateFlags |= SC_ROUTE;
                            }
                          }
                          break;
                          
                          case Track::AUDIO_INPUT:
                          {
                            AudioOutput* ai = (AudioOutput*)editable_track;
                            for(int ch = 0; ch < ai->channels(); ++ch)
                            {
                              MusEGlobal::audioDevice->unregisterPort(ai->jackPort(ch));
                              //ai->setJackPort(ch, 0); // Done in RT stage.
                              updateFlags |= SC_ROUTE;
                            }
                          }     
                          break;
                          
                          case Track::AUDIO_AUX:
                            updateFlags |= SC_AUX;
                          break;
                          
                          default:
                          break;
                        }
                        removeTrackOperation(editable_track, pendingOperations);
                        updateFlags |= SC_TRACK_REMOVED;
                        break;
                        
                  case UndoOp::ModifyClip:
                        sndFileApplyUndoFile(i->nEvent, i->tmpwavfile, i->startframe, i->endframe);
                        updateFlags |= SC_CLIP_MODIFIED;
                        break;
                  case UndoOp::ModifyTrackChannel:
                     if (editable_track->isMidiTrack())
                        {
                          MusECore::MidiTrack* mt = static_cast<MusECore::MidiTrack*>(editable_track);
                          if (i->_newPropValue != mt->outChannel())
                          {
                                MusECore::MidiTrack::ChangedType_t changed = MusECore::MidiTrack::NothingChanged;
                                MusEGlobal::audio->msgIdle(true);
                                changed |= mt->setOutChanAndUpdate(i->_newPropValue, false);
                                MusEGlobal::audio->msgIdle(false);
                                MusEGlobal::audio->msgUpdateSoloStates();                   
                                updateFlags |= (SC_ROUTE | ((changed & MusECore::MidiTrack::DrumMapChanged) ? SC_DRUMMAP : 0));
                          }
                        }
                        else
                        {
                            if(editable_track->type() != MusECore::Track::AUDIO_SOFTSYNTH)
                            {
                              MusECore::AudioTrack* at = static_cast<MusECore::AudioTrack*>(editable_track);
                              if (i->_newPropValue != at->channels()) {
                                    MusEGlobal::audio->msgSetChannels(at, i->_newPropValue);
                                    updateFlags |= SC_CHANNELS;
                                    }
                            }         
                        }      
                        break;

                  case UndoOp::SetTrackRecord:
                        if(!editable_track->setRecordFlag1(i->a))
                          break;
                        pendingOperations.add(PendingOperationItem(editable_track, i->a, PendingOperationItem::SetTrackRecord));
                        // FIXME: No choice but to include monitor flag. Really should try to merge pending ops flags
                        //  with undo flags after executing the pending ops in executeOperationGroup3...
                        updateFlags |= (SC_RECFLAG | SC_TRACK_REC_MONITOR);
                        break;

                  case UndoOp::SetTrackMute:
                        pendingOperations.add(PendingOperationItem(editable_track, i->a, PendingOperationItem::SetTrackMute));
                        updateFlags |= SC_MUTE;
                        break;

                  case UndoOp::SetTrackSolo:
                        pendingOperations.add(PendingOperationItem(editable_track, i->a, PendingOperationItem::SetTrackSolo));
                        updateFlags |= SC_SOLO;
                        break;

                  case UndoOp::SetTrackRecMonitor:
                        pendingOperations.add(PendingOperationItem(editable_track, i->a, PendingOperationItem::SetTrackRecMonitor));
                        updateFlags |= SC_TRACK_REC_MONITOR;
                        break;

                  case UndoOp::SetTrackOff:
                        pendingOperations.add(PendingOperationItem(editable_track, i->a, PendingOperationItem::SetTrackOff));
                        updateFlags |= SC_MUTE;
                        break;

                        
                  case UndoOp::AddRoute:
#ifdef _UNDO_DEBUG_
                        fprintf(stderr, "Song::executeOperationGroup1:AddRoute\n");
#endif                        
                        pendingOperations.add(PendingOperationItem(*i->routeFrom, *i->routeTo, PendingOperationItem::AddRoute));
                        updateFlags |= SC_ROUTE;
                        break;
                        
                  case UndoOp::DeleteRoute:
#ifdef _UNDO_DEBUG_
                        fprintf(stderr, "Song::executeOperationGroup1:DeleteRoute\n");
#endif                        
                        pendingOperations.add(PendingOperationItem(*i->routeFrom, *i->routeTo, PendingOperationItem::DeleteRoute));
                        updateFlags |= SC_ROUTE;
                        break;
                        
                  case UndoOp::ModifyTrackName:
                        pendingOperations.add(PendingOperationItem(editable_track, i->_newName, PendingOperationItem::ModifyTrackName));
                        updateFlags |= (SC_TRACK_MODIFIED | SC_MIDI_TRACK_PROP);
                        // If it's an aux track, notify aux UI controls to reload, or change their names etc.
                        if(editable_track->type() == Track::AUDIO_AUX)
                          updateFlags |= SC_AUX;
                        break;
                        
                  case UndoOp::MoveTrack:
                        pendingOperations.add(PendingOperationItem(&_tracks, i->a, i->b, PendingOperationItem::MoveTrack));
                        updateFlags |= SC_TRACK_MOVED;
                        break;
                        
                  case UndoOp::ModifyPartName:
                        pendingOperations.add(PendingOperationItem(editable_part, i->_newName, PendingOperationItem::ModifyPartName));
                        updateFlags |= SC_PART_MODIFIED;
                        break;

                  case UndoOp::ModifyPartStart:
                      {
                        pendingOperations.modifyPartStartOperation(
                          editable_part, i->new_partlen_or_pos, i->new_partlen, i->events_offset, i->events_offset_time_type);
                        updateFlags |= SC_PART_MODIFIED;
                        // If the part had events, then treat it as if they were added/removed with separate Add/DeleteEvent operations.
                        // Even if they will be added/deleted later in this operations group with actual separate Add/DeleteEvent operations,
                        //  that's an SC_EVENT_ADDED/REMOVED anyway, so hopefully no harm.
                        if(i->events_offset != 0 && !editable_part->events().empty())
                          updateFlags |= (SC_EVENT_INSERTED | SC_EVENT_REMOVED | SC_EVENT_MODIFIED);
                      }
                      break;
                  case UndoOp::ModifyPartLength: 
                        {
                          unsigned p = Pos::convert(editable_part->posValue() + i->new_partlen_or_pos, editable_part->type(), Pos::TICKS);
                          // >= for good luck, since this (simpler) comparison is in the TICKS domain.
                          if(p >= song_len)
                          {
                            song_len = p + 1; 
                            // Insert a ModifySongLen operation BEFORE this one. If insert finds an existing ModifySongLen,
                            //  possibly long before this one, it REPLACES that one's values.
                            operations.insert(i, UndoOp(UndoOp::ModifySongLen, song_len, len()));
                            // Since the ModifySongLen above will not be iterated now, act like the operation had just been iterated. 
                            // The same REPLACEMENT rules apply here.
                            pendingOperations.add(PendingOperationItem(song_len, PendingOperationItem::ModifySongLength));
                            updateFlags |= SC_EVERYTHING;  // set all flags   // TODO Refine this! Too many flags.  // REMOVE Tim.
                            //updateFlags |= SC_SONG_LEN;
                          }
                          pendingOperations.modifyPartLengthOperation(
                            editable_part, i->new_partlen_or_pos, i->events_offset, i->events_offset_time_type);
                          updateFlags |= SC_PART_MODIFIED;
                          // If the part had events, then treat it as if they were added/removed with separate Add/DeleteEvent operations.
                          // Even if they will be added/deleted later in this operations group with actual separate Add/DeleteEvent operations,
                          //  that's an SC_EVENT_ADDED/REMOVED anyway, so hopefully no harm.
                          if(i->events_offset != 0 && !editable_part->events().empty())
                            updateFlags |= (SC_EVENT_INSERTED | SC_EVENT_REMOVED | SC_EVENT_MODIFIED);
                        }
                        break;
                        
                  case UndoOp::MovePart:
                        {
                          unsigned p = Pos::convert(editable_part->lenValue() + i->new_partlen_or_pos, editable_part->type(), Pos::TICKS);
                          // >= for good luck, since this (simpler) comparison is in the TICKS domain.
                          if(p >= song_len)
                          {
                            song_len = p + 1; 
                            // Insert a ModifySongLen operation BEFORE this one. If insert finds an existing ModifySongLen,
                            //  possibly long before this one, it REPLACES that one's values.
                            operations.insert(i, UndoOp(UndoOp::ModifySongLen, song_len, len()));
                            // Since the ModifySongLen above will not be iterated now, act like the operation had just been iterated. 
                            // The same REPLACEMENT rules apply here.
                            pendingOperations.add(PendingOperationItem(song_len, PendingOperationItem::ModifySongLength));
                            updateFlags |= SC_EVERYTHING;  // set all flags   // TODO Refine this! Too many flags.  // REMOVE Tim.
                            //updateFlags |= SC_SONG_LEN;
                          }
#ifdef _UNDO_DEBUG_
                          fprintf(stderr, "Song::executeOperationGroup1:MovePart ** calling parts->movePartOperation\n");
#endif                        
                          pendingOperations.movePartOperation(editable_part->track()->parts(),
                            editable_part, i->new_partlen_or_pos, editable_track);
                          if(editable_track)
                            updateFlags |= SC_PART_INSERTED | SC_PART_REMOVED;
                          updateFlags |= SC_PART_MODIFIED;
                        }
                        break;
                        
                  case UndoOp::AddPart:
                        {
                          unsigned p = Pos::convert(editable_part->lenValue() + editable_part->posValue(), editable_part->type(), Pos::TICKS);
                          // >= for good luck, since this (simpler) comparison is in the TICKS domain.
                          if(p >= song_len)
                          {
                            song_len = p + 1; 
                            // Insert a ModifySongLen operation BEFORE this one. If insert finds an existing ModifySongLen,
                            //  possibly long before this one, it REPLACES that one's values.
                            operations.insert(i, UndoOp(UndoOp::ModifySongLen, song_len, len()));
                            // Since the ModifySongLen above will not be iterated now, act like the operation had just been iterated. 
                            // The same REPLACEMENT rules apply here.
                            pendingOperations.add(PendingOperationItem(song_len, PendingOperationItem::ModifySongLength));
                            updateFlags |= SC_EVERYTHING;  // set all flags   // TODO Refine this! Too many flags.  // REMOVE Tim.
                            //updateFlags |= SC_SONG_LEN;
                          }
#ifdef _UNDO_DEBUG_
                          fprintf(stderr, "Song::executeOperationGroup1:addPart ** calling parts->addOperation\n");
#endif                        
                          // Ensure that wave event sndfile file handles are opened.
                          // It should not be the job of the pending operations list to do this.
                          // TODO Coordinate close/open with part mute and/or track off.
                          editable_part->openAllEvents();
                          
                          pendingOperations.addPartOperation(editable_part->track()->parts(), editable_part);
                          updateFlags |= SC_PART_INSERTED;
                          // If the part has events, then treat it as if they were inserted with separate AddEvent operations.
                          // Even if some will be inserted later in this operations group with actual separate AddEvent operations,
                          //  that's an SC_EVENT_INSERTED anyway, so should be no harm.
                          if(!editable_part->events().empty())
                            updateFlags |= SC_EVENT_INSERTED;
                        }
                        break;
                    
                  case UndoOp::DeletePart:
#ifdef _UNDO_DEBUG_
                        fprintf(stderr, "Song::executeOperationGroup1:deletePart ** calling parts->delOperation\n");
#endif                        
                        pendingOperations.delPartOperation(editable_part->track()->parts(), editable_part);
                        updateFlags |= SC_PART_REMOVED;
                        // If the part had events, then treat it as if they were removed with separate DeleteEvent operations.
                        // Even if they will be deleted later in this operations group with actual separate DeleteEvent operations,
                        //  that's an SC_EVENT_REMOVED anyway, so hopefully no harm. This fixes a problem with midi controller canvas
                        //  not updating after such a 'delete part with events, no separate AddEvents were used when creating the part'.
                        if(!editable_part->events().empty())
                          updateFlags |= SC_EVENT_REMOVED;
                        break;
                    
                  case UndoOp::AddEvent: {
#ifdef _UNDO_DEBUG_
                        fprintf(stderr, "Song::executeOperationGroup1:AddEvent ** calling addEvent\n");
#endif                        
                        if(!i->nEvent.empty())
                        {
                          SndFileR f = i->nEvent.sndFile();
                          // Ensure that wave event sndfile file handle is opened.
                          // It should not be the job of the pending operations list to do this.
                          // TODO Coordinate close/open with part mute and/or track off.
                          if(!f.isNull() && !f.isOpen())
                            f.openRead();
                        }
                        
                        addEventOperation(i->nEvent, editable_part, i->doCtrls, i->doClones);
                        updateFlags |= SC_EVENT_INSERTED;
                        }
                        break;
                        
                  case UndoOp::DeleteEvent: {
#ifdef _UNDO_DEBUG_
                        fprintf(stderr, "Song::executeOperationGroup1:DeleteEvent ** calling deleteEvent\n");
#endif                        
                        // Special: Replace the undo item's event with the real actual event found in the event lists.
                        // This way even a modified event can be passed in to the DeleteEvent operation constructor,
                        //  and as long as the ID AND position values match it will find and use the ORIGINAL event.
                        // (It's safe, the = operator quickly returns if the two events have the same base pointer.)
                        i->nEvent = deleteEventOperation(i->nEvent, editable_part, i->doCtrls, i->doClones);
                        updateFlags |= SC_EVENT_REMOVED;
                        }
                        break;
                        
                  case UndoOp::ModifyEvent:
#ifdef _UNDO_DEBUG_
                        fprintf(stderr, "Song::executeOperationGroup1:ModifyEvent ** calling changeEvent\n");
#endif                        
                        // Special: Replace the undo item's old event with the real actual event found in the event lists.
                        // This way even a modified old event can be passed in to the ModifyEvent operation constructor,
                        //  and as long as the ID AND position values match it will find and use the ORIGINAL event.
                        // (It's safe, the = operator quickly returns if the two events have the same base pointer.)
                        i->oEvent = changeEventOperation(i->oEvent, i->nEvent, editable_part, i->doCtrls, i->doClones);
                        updateFlags |= SC_EVENT_MODIFIED;
                        break;

                        
                  case UndoOp::AddAudioCtrlVal:
                  {
#ifdef _UNDO_DEBUG_
                        fprintf(stderr, "Song::executeOperationGroup1:AddAudioCtrlVal\n");
#endif                        
                        CtrlListList* cll = static_cast<AudioTrack*>(editable_track)->controller();
                        iCtrlList icl = cll->find(i->_audioCtrlIdAddDel);
                        if(icl != cll->end())
                        {
                          // Enforce a rule that no interpolated values can be added to a discrete graph.
                          // For now we do not allow interpolation of integer or enum controllers.
                          // TODO: It would require custom line drawing and corresponding hit detection.
                          if(icl->second->mode() == CtrlList::DISCRETE)
                            i->_audioCtrlValFlagsAddDel |= CtrlVal::VAL_DISCRETE;

                          //CtrlList* cl = icl->second;
                          //iCtrl ic = cl->find(i->_audioCtrlFrameAdd);
                          //if(ic != cl->end())
                          //  // An existing value was found. Replace it with the new value.
                          //  pendingOperations.add(PendingOperationItem(icl->second, ic, i->_audioCtrlValAdd,
                          //    PendingOperationItem::ModifyAudioCtrlVal));
                          //else
                            // Add the new value.
                          pendingOperations.add(PendingOperationItem(
                            icl->second, i->_audioCtrlFrameAddDel, i->_audioCtrlValAddDel,
                            i->_audioCtrlValFlagsAddDel, PendingOperationItem::AddAudioCtrlVal));
                          updateFlags |= SC_AUDIO_CONTROLLER;
                        }
                  }
                  break;

                  case UndoOp::AddAudioCtrlValStruct:
                  {
#ifdef _UNDO_DEBUG_
                        fprintf(stderr, "Song::executeOperationGroup1:AddAudioCtrlValStruct\n");
#endif
                        CtrlListList* cll = static_cast<AudioTrack*>(editable_track)->controller();
                        iCtrlList icl = cll->find(i->_audioCtrlIdStruct);
                        if(icl != cll->end())
                        {
                          // Enforce a rule that no interpolated values can be added to a discrete graph.
                          // For now we do not allow interpolation of integer or enum controllers.
                          // TODO: It would require custom line drawing and corresponding hit detection.
                          if(icl->second->mode() == CtrlList::DISCRETE)
                            i->_audioCtrlValStruct->setDiscrete(true);

                          // Add the new value.
                          pendingOperations.add(PendingOperationItem(
                            icl->second, i->_audioCtrlFrameStruct, i->_audioCtrlValStruct, PendingOperationItem::AddAudioCtrlValStruct));
                          updateFlags |= SC_AUDIO_CONTROLLER;
                        }
                  }
                  break;
                        
                  case UndoOp::DeleteAudioCtrlVal:
                  {
#ifdef _UNDO_DEBUG_
                        fprintf(stderr, "Song::executeOperationGroup1:DeleteAudioCtrlVal\n");
#endif
                        CtrlListList* cll = static_cast<AudioTrack*>(editable_track)->controller();
                        iCtrlList icl = cll->find(i->_audioCtrlIdAddDel);
                        if(icl != cll->end())
                        {
                          CtrlList* cl = icl->second;
                          iCtrl ic = cl->find(i->_audioCtrlFrameAddDel);
                          if(ic != cl->end())
                          {
                            i->_audioCtrlValAddDel = ic->second.value(); // Store the existing value so it can be restored.
                            i->_audioCtrlValFlagsAddDel = ic->second.flags(); // Store the existing value so it can be restored.
                            pendingOperations.add(PendingOperationItem(cl, ic, PendingOperationItem::DeleteAudioCtrlVal));
                            updateFlags |= SC_AUDIO_CONTROLLER;
                          }
                        }
                  }
                  break;

                  case UndoOp::ModifyAudioCtrlVal:
                  {
#ifdef _UNDO_DEBUG_
                        fprintf(stderr, "Song::executeOperationGroup1:ModifyAudioCtrlVal\n");
#endif                        
                        CtrlListList* cll = static_cast<AudioTrack*>(editable_track)->controller();
                        iCtrlList icl = cll->find(i->_audioCtrlID);
                        if(icl != cll->end())
                        {
                          CtrlList* cl = icl->second;
                          iCtrl ic = cl->find(i->_audioCtrlFrame);
                          if(ic != cl->end())
                          {
                            i->_audioCtrlVal = ic->second.value(); // Store the existing value so it can be restored.
                            pendingOperations.add(PendingOperationItem(icl->second, ic, i->_audioNewCtrlFrame, i->_audioNewCtrlVal, PendingOperationItem::ModifyAudioCtrlVal));
                            updateFlags |= SC_AUDIO_CONTROLLER;
                          }
                        }
                  }
                  break;
                        
                  case UndoOp::ModifyAudioCtrlValList:
                  {
#ifdef _UNDO_DEBUG_
                        fprintf(stderr, "Song::executeOperationGroup1:ModifyAudioCtrlValList\n");
#endif                        
                        // If we are in audio controller move mode, end the move mode unless specifically asking not to.
                        bool moveModeEnding = false;
                        if(!i->_noEndAudioCtrlMoveMode && _audioCtrlMoveModeBegun)
                        {
                          moveModeEnding = true;
                          // Insert an EndAudioCtrlMoveMode operation BEFORE the given iterator. If insert finds an existing EndAudioCtrlMoveMode,
                          //  possibly long before this one, it IGNORES this one.
                          operations.insert(i, UndoOp(UndoOp::EndAudioCtrlMoveMode));
                          // Since the EndAudioCtrlMoveMode above will not be iterated now, act like the operation had just been iterated.
                          // Clearing the track's erasedController and noEraseController is done below, by composing completely new lists.
                          // Reset the flag to indicate an EndAudioCtrlMoveMode was encountered.
                          _audioCtrlMoveModeBegun = false;
                          updateFlags |= SC_AUDIO_CTRL_MOVE_MODE;
                        }

                        if(!i->track->isMidiTrack())
                        {
                          AudioTrack* at = static_cast<AudioTrack*>(const_cast<Track*>(i->track));
                          const int id = i->_audioCtrlIdModify;
                          iCtrlList icl = at->controller()->find(id);
                          if(icl != at->controller()->end())
                          {
                            // Make a complete copy of the controller list. The list will be quickly switched in the realtime stage.
                            // The Pending Operations system will take 'ownership' of this and delete it at the appropriate time.
                            CtrlList* new_list = new CtrlList(*icl->second, CtrlList::ASSIGN_PROPERTIES | CtrlList::ASSIGN_VALUES);
                            bool new_list_changed = false;

                            if(i->_eraseCtrlList)
                            {
                              // Erase any items in the new list found at the frames given by the erase list.
                              for(ciCtrl ic = i->_eraseCtrlList->cbegin(); ic != i->_eraseCtrlList->cend(); ++ic)
                              {
                                // TODO: It would be faster if we used 'group end' markers, either as an auxilliary
                                //        list or embedded in each controller list item. That way we could use
                                //        the erase(start, end) to quickly erase whole sections - or the entire range.
                                if(new_list->erase(ic->first) != 0)
                                  new_list_changed = true;
                              }
                            }

                            if(i->_recoverableEraseCtrlList)
                            {
                              // Erase any items in the new list found at the frames given by the recoverable erase list.
                              for(ciCtrl ic = i->_recoverableEraseCtrlList->cbegin(); ic != i->_recoverableEraseCtrlList->cend(); ++ic)
                              {
                                if(new_list->erase(ic->first) != 0)
                                  new_list_changed = true;
                              }
                            }
                            // Add any items in the add list.
                            if(i->_addCtrlList)
                            {
                              for(ciCtrl ic = i->_addCtrlList->cbegin(); ic != i->_addCtrlList->cend(); ++ic)
                              {
                                if(new_list->insert(CtrlListInsertPair_t(ic->first, ic->second)).second)
                                  new_list_changed = true;
                              }
                            }
                            if(i->_recoverableAddCtrlList)
                            {
                              // Add any items in the recoverable add list.
                              for(ciCtrl ic = i->_recoverableAddCtrlList->cbegin(); ic != i->_recoverableAddCtrlList->cend(); ++ic)
                              {
                                if(new_list->insert(CtrlListInsertPair_t(ic->first, ic->second)).second)
                                  new_list_changed = true;
                              }
                            }

                            const bool do_rel = i->_recoverableEraseCtrlList && !i->_recoverableEraseCtrlList->empty();
                            const bool do_ral = i->_recoverableAddCtrlList && !i->_recoverableAddCtrlList->empty();
                            if(do_rel || do_ral)
                            {
                              CtrlList* new_elist;
                              bool new_elist_changed = false;

                              iCtrlList iecl = at->erasedController()->end();
                              // If the controller is not found, compose a new one.
                              // Or, if an end move mode WILL BE done, the track's erasedController and noEraseController
                              //  will be completely blank, so compose new lists.
                              if(!moveModeEnding)
                                iecl = at->erasedController()->find(id);
                              if(iecl == at->erasedController()->end())
                              {
                                if(do_rel)
                                {
                                  // Existing list not found. Make a complete copy of the desired erased controller list, to start with.
                                  // The Pending Operations system will take 'ownership' of this and delete it at the appropriate time.
                                  new_elist = new CtrlList(
                                    *i->_recoverableEraseCtrlList, CtrlList::ASSIGN_PROPERTIES | CtrlList::ASSIGN_VALUES);
                                  if(do_ral)
                                  {
                                    // Erase any items in new erased list found at the frames given by the the recoverable add list.
                                    for(ciCtrl ic = i->_recoverableAddCtrlList->cbegin(); ic != i->_recoverableAddCtrlList->cend(); ++ic)
                                      new_elist->erase(ic->first);
                                  }
                                  if(new_elist->empty())
                                  {
                                    // Nothing left. Delete the list.
                                    delete new_elist;
                                  }
                                  else
                                  {
                                    pendingOperations.add(PendingOperationItem(
                                      at->erasedController(), new_elist, PendingOperationItem::AddAudioCtrlValList));
                                    updateFlags |= SC_AUDIO_CONTROLLER_LIST;
                                  }
                                }
                              }
                              else
                              {
                                // Existing list found. Make a complete working copy of the existing erased controller list.
                                // The list will be quickly switched in the realtime stage.
                                // The Pending Operations system will take 'ownership' of this and delete it at the appropriate time.
                                new_elist = new CtrlList(*iecl->second, CtrlList::ASSIGN_PROPERTIES | CtrlList::ASSIGN_VALUES);
                                if(do_ral)
                                {
                                  // Erase any items in new erased list found at the frames given by the the recoverable add list.
                                  for(ciCtrl ic = i->_recoverableAddCtrlList->cbegin(); ic != i->_recoverableAddCtrlList->cend(); ++ic)
                                  {
                                    if(new_elist->erase(ic->first) != 0)
                                      new_elist_changed = true;
                                  }
                                }
                                if(do_rel)
                                {
                                  // Add any items in the recoverable erase list.
                                  for(ciCtrl ic = i->_recoverableEraseCtrlList->cbegin(); ic != i->_recoverableEraseCtrlList->cend(); ++ic)
                                  {
                                    if(new_elist->insert(CtrlListInsertPair_t(ic->first, ic->second)).second)
                                      new_elist_changed = true;
                                  }
                                }
                                if(new_elist_changed)
                                {
                                  pendingOperations.add(PendingOperationItem(iecl, new_elist, PendingOperationItem::ModifyAudioCtrlValList));
                                  updateFlags |= SC_AUDIO_CONTROLLER_LIST;
                                }
                                else
                                {
                                  delete new_elist;
                                }
                              }
                            }
                            if(i->_doNotEraseCtrlList && !i->_doNotEraseCtrlList->empty())
                            {
                              iCtrlList iecl = at->noEraseController()->end();
                              // If the controller is not found, compose a new one.
                              // Or, if an end move mode WILL BE done, the track's erasedController and noEraseController
                              //  will be completely blank, so compose new lists.
                              if(!moveModeEnding)
                                iecl = at->noEraseController()->find(id);
                              if(iecl == at->noEraseController()->end())
                              {
                                // Existing list not found. Make a complete copy of the desired no-erase controller list, to start with.
                                // The Pending Operations system will take 'ownership' of this and delete it at the appropriate time.
                                CtrlList* new_nelist = new CtrlList(
                                  *i->_doNotEraseCtrlList, CtrlList::ASSIGN_PROPERTIES | CtrlList::ASSIGN_VALUES);
                                pendingOperations.add(PendingOperationItem(
                                  at->noEraseController(), new_nelist, PendingOperationItem::AddAudioCtrlValList));
                                updateFlags |= SC_AUDIO_CONTROLLER_LIST;

                                // We must unselect these original points. We don't want them included in any movement at all.
                                for(iCtrl ic = i->_doNotEraseCtrlList->begin(); ic != i->_doNotEraseCtrlList->end(); ++ic)
                                {
                                  iCtrl ic_e = new_list->find(ic->first);
                                  if(ic_e != new_list->end() && ic_e->second.selected())
                                  {
                                    ic_e->second.setSelected(false);
                                    new_list_changed = true;
                                    updateFlags |= SC_AUDIO_CONTROLLER_SELECTION;
                                  }
                                }
                              }
                              else
                              {
                                // Existing list found. Make a complete working copy of the existing no-erase controller list.
                                // The list will be quickly switched in the realtime stage.
                                // The Pending Operations system will take 'ownership' of this and delete it at the appropriate time.
                                CtrlList* new_nelist = new CtrlList(*iecl->second, CtrlList::ASSIGN_PROPERTIES | CtrlList::ASSIGN_VALUES);
                                bool new_nelist_changed = false;
                                for(ciCtrl ic = i->_doNotEraseCtrlList->cbegin(); ic != i->_doNotEraseCtrlList->cend(); ++ic)
                                {
                                  if(new_nelist->insert(CtrlListInsertPair_t(ic->first, ic->second)).second)
                                  {
                                    new_nelist_changed = true;

                                    // We must unselect these original points. We don't want them included in any movement at all.
                                    iCtrl ic_e = new_list->find(ic->first);
                                    if(ic_e != new_list->end() && ic_e->second.selected())
                                    {
                                      ic_e->second.setSelected(false);
                                      new_list_changed = true;
                                      updateFlags |= SC_AUDIO_CONTROLLER_SELECTION;
                                    }
                                  }
                                }
                                if(new_nelist_changed)
                                {
                                  pendingOperations.add(PendingOperationItem(iecl, new_nelist, PendingOperationItem::ModifyAudioCtrlValList));
                                  updateFlags |= SC_AUDIO_CONTROLLER_LIST;
                                }
                                else
                                {
                                  delete new_nelist;
                                }
                              }
                            }

                            if(new_list_changed)
                            {
                              // The operation will quickly switch the list in the RT stage then the delete the old list in the non-RT stage.
                              pendingOperations.add(PendingOperationItem(icl, new_list, PendingOperationItem::ModifyAudioCtrlValList));
                              updateFlags |= SC_AUDIO_CONTROLLER_LIST;
                            }
                            else
                            {
                              delete new_list;
                            }
                          }
                        }
                  }
                  break;
                        
                  case UndoOp::SelectAudioCtrlVal:
                  {
#ifdef _UNDO_DEBUG_
                        fprintf(stderr, "Song::executeOperationGroup1:SelectAudioCtrlVal\n");
#endif
                        if(i->_audioCtrlListSelect)
                        {
                          iCtrl ic = i->_audioCtrlListSelect->find(i->_audioCtrlSelectFrame);
                          if(ic != i->_audioCtrlListSelect->end())
                          {
                            // Set the new value.
                            pendingOperations.add(PendingOperationItem(
                              &ic->second, i->selected, PendingOperationItem::SelectAudioCtrlVal));
                          }
                        }
                        // Update always, just in case something was not found, a refresh might help.
                        updateFlags |= SC_AUDIO_CONTROLLER_SELECTION;
                  }
                  break;

                  case UndoOp::SetAudioCtrlPasteEraseMode:
                  {
#ifdef _UNDO_DEBUG_
                        fprintf(stderr, "Song::executeOperationGroup1:SetAudioCtrlPasteEraseMode\n");
#endif
                        // Set the new value.
                        pendingOperations.add(PendingOperationItem(
                          i->_audioCtrlNewPasteEraseOpts, PendingOperationItem::SetAudioCtrlPasteEraseMode));
                        updateFlags |= SC_AUDIO_CTRL_PASTE_ERASE_MODE;
                  }
                  break;

                  case UndoOp::BeginAudioCtrlMoveMode:
                  {
#ifdef _UNDO_DEBUG_
                        fprintf(stderr, "Song::executeOperationGroup1:BeginAudioCtrlMoveMode\n");
#endif
                        // TODO ?
                        // We are crossing a boundary from move mode off to on.
                        // If selections are NOT undoable, we must manually unselect all points before entering this mode.
                        // This is to prevent points that were selected BEFORE the move mode from becoming part of the move.
                        // The original move-mode selection state is needed and we can't know it because it isn't undoable.
                        //
                        // But... we need to know whether this is the FIRST time this begin command is running, or whether
                        //  it is a REDO of the command, because we must NOT clear selections the FIRST time.
                        //
                        // But... selecting something - even if selections are non-undoable - counts as an undo command which
                        //  clears the REDO list, so we should not even be able to re-enter the mode from there.
                        // But we even execute certain Pending Operations commands directly from the code which BYPASSES
                        //  clearing of the REDO list.
                        // TODO? Take a closer look at whether to clear the REDO list in non-undoable cases.
                        //       The problem is, technically the one-time command changed something, so the continuity of
                        //        the UNDO/REDO becomes broken, therefore REDO MUST be cleared. But the user sees no
                        //        command in the UNDO list that cleared the REDO list, leading to confusion "what did
                        //        I just do to clear my REDO list?".
                        // If someday we decide NOT to clear the REDO list when a one-time command such as selection is run,
                        //  this logic will need rethinking...

                        // If not already set, set the flag to indicate a BeginAudioCtrlMoveMode was encountered.
                        if(!_audioCtrlMoveModeBegun)
                        {
                          _audioCtrlMoveModeBegun = true;
                          updateFlags |= SC_AUDIO_CTRL_MOVE_MODE;
                        }
                  }
                  break;
                  case UndoOp::EndAudioCtrlMoveMode:
                  {
#ifdef _UNDO_DEBUG_
                        fprintf(stderr, "Song::executeOperationGroup1:EndAudioCtrlMoveMode\n");
#endif
                        // If already set, reset the flag to indicate an EndAudioCtrlMoveMode was encountered.
                        if(_audioCtrlMoveModeBegun)
                        {
                          // Reset the flag.
                          _audioCtrlMoveModeBegun = false;
                          updateFlags |= SC_AUDIO_CTRL_MOVE_MODE;
                          // End the move mode.
                          if(audioCtrlMoveEnd(pendingOperations))
                            updateFlags |= SC_AUDIO_CONTROLLER_LIST;
                        }
                  }
                  break;


                  case UndoOp::SetInstrument:
                  {
#ifdef _UNDO_DEBUG_
                        fprintf(stderr, "Song::executeOperationGroup1:SetInstrument\n");
#endif                        
                        // Set the new value.
                        pendingOperations.add(PendingOperationItem(
                          i->_midiPort, i->_newMidiInstrument,
                          PendingOperationItem::SetInstrument));
                        updateFlags |= SC_MIDI_INSTRUMENT;
                  }
                  break;


                  case UndoOp::AddTempo:
#ifdef _UNDO_DEBUG_
                        fprintf(stderr, "Song::executeOperationGroup1:AddTempo ** calling tempomap.addOperation tick:%d tempo:%d\n", i->a, i->b);
#endif                        
                        // Create the new list if it doesn't already exist.
                        // Make a copy of the original list.
                        if(!new_tempo_list)
                        {
                          new_tempo_list = new TempoList();
                          new_tempo_list->copy(MusEGlobal::tempomap);
                        }
                        // Defer normalize until end of stage 2.
                        new_tempo_list->addTempo(i->a, i->b, false);

                        updateFlags |= SC_TEMPO;
                        break;
                        
                  case UndoOp::DeleteTempo:
#ifdef _UNDO_DEBUG_
                        fprintf(stderr, "Song::executeOperationGroup1:DeleteTempo ** calling tempomap.delOperation tick:%d\n", i->a);
#endif                        
                        // Create the new list if it doesn't already exist.
                        // Make a copy of the original list.
                        if(!new_tempo_list)
                        {
                          new_tempo_list = new TempoList();
                          new_tempo_list->copy(MusEGlobal::tempomap);
                        }
                        // Defer normalize until end of stage 2.
                        new_tempo_list->delTempo(i->a, false);

                        updateFlags |= SC_TEMPO;
                        break;
                        
                  case UndoOp::ModifyTempo:
#ifdef _UNDO_DEBUG_
                        fprintf(stderr, "Song::executeOperationGroup1:ModifyTempo ** calling tempomap.addOperation tick:%d tempo:%d\n", i->a, i->c);
#endif                        
                        // Create the new list if it doesn't already exist.
                        // Make a copy of the original list.
                        if(!new_tempo_list)
                        {
                          new_tempo_list = new TempoList();
                          new_tempo_list->copy(MusEGlobal::tempomap);
                        }
                        // Defer normalize until end of stage 2.
                        new_tempo_list->addTempo(i->a, i->c, false);

                        updateFlags |= SC_TEMPO;
                        break;

//                   case UndoOp::SetTempo:
//                         // Only if the master is on.
//                         if(MusEGlobal::tempomap.masterFlag())
//                         {
// #ifdef _UNDO_DEBUG_
//                           fprintf(stderr, "Song::executeOperationGroup1:SetTempo ** calling tempomap.addOperation tick:%d tempo:%d\n", i->a, i->b);
// #endif                        
//                           MusEGlobal::tempomap.addOperation(i->a, i->b, pendingOperations);
//                         }
//                         else
//                           pendingOperations.add(PendingOperationItem(&MusEGlobal::tempomap, i->a, PendingOperationItem::SetStaticTempo));
//                         updateFlags |= SC_TEMPO;
//                         break;
                        
                  case UndoOp::SetStaticTempo:
#ifdef _UNDO_DEBUG_
                        fprintf(stderr, "Song::executeOperationGroup1:SetStaticTempo ** adding SetStaticTempo operation\n");
#endif                        
                        pendingOperations.add(PendingOperationItem(&MusEGlobal::tempomap, i->a, PendingOperationItem::SetStaticTempo));
                        updateFlags |= SC_TEMPO;
                        break;
                        
                  case UndoOp::SetGlobalTempo:
#ifdef _UNDO_DEBUG_
                        fprintf(stderr, "Song::executeOperationGroup1:SetGlobalTempo ** adding SetGlobalTempo operation\n");
#endif                        
                        pendingOperations.add(PendingOperationItem(&MusEGlobal::tempomap, i->a, PendingOperationItem::SetGlobalTempo));
                        updateFlags |= SC_TEMPO;
                        break;
                        
                  case UndoOp::EnableMasterTrack:
#ifdef _UNDO_DEBUG_
                        fprintf(stderr, "Song::executeOperationGroup1:EnableMasterTrack ** adding EnableMasterTrack operation\n");
#endif                        
                        pendingOperations.add(PendingOperationItem(&MusEGlobal::tempomap, (bool)i->a, PendingOperationItem::SetUseMasterTrack));
                        updateFlags |= SC_MASTER;
                        break;
                        
                  case UndoOp::AddSig:
#ifdef _UNDO_DEBUG_
                        fprintf(stderr, "Song::executeOperationGroup1:AddSig ** calling MusEGlobal::sigmap.addOperation\n");
#endif                        
                        // Create the new list if it doesn't already exist.
                        // Make a copy of the original list.
                        if(!new_sig_list)
                        {
                          new_sig_list = new SigList();
                          new_sig_list->copy(MusEGlobal::sigmap);
                        }
                        // Defer normalize until end of stage 2.
                        new_sig_list->add(i->a, MusECore::TimeSignature(i->b, i->c), false);

                        updateFlags |= SC_SIG;
                        break;
                        
                  case UndoOp::DeleteSig:
#ifdef _UNDO_DEBUG_
                        fprintf(stderr, "Song::executeOperationGroup1:DeleteSig ** calling MusEGlobal::sigmap.delOperation\n");
#endif                        
                        // Create the new list if it doesn't already exist.
                        // Make a copy of the original list.
                        if(!new_sig_list)
                        {
                          new_sig_list = new SigList();
                          new_sig_list->copy(MusEGlobal::sigmap);
                        }
                        // Defer normalize until end of stage 2.
                        new_sig_list->del(i->a, false);

                        updateFlags |= SC_SIG;
                        break;
                        
                  case UndoOp::ModifySig:
#ifdef _UNDO_DEBUG_
                        fprintf(stderr, "Song::executeOperationGroup1:ModifySig ** calling MusEGlobal::sigmap.addOperation\n");
#endif                        
                        // Create the new list if it doesn't already exist.
                        // Make a copy of the original list.
                        if(!new_sig_list)
                        {
                          new_sig_list = new SigList();
                          new_sig_list->copy(MusEGlobal::sigmap);
                        }
                        // Defer normalize until end of stage 2.
                        new_sig_list->add(i->a, MusECore::TimeSignature(i->d, i->e), false);

                        updateFlags |= SC_SIG;
                        break;

                        
                  case UndoOp::AddKey:
#ifdef _UNDO_DEBUG_
                        fprintf(stderr, "Song::executeOperationGroup1:AddKey ** calling keymap.addOperation\n");
#endif                        
                        // Create the new list if it doesn't already exist.
                        // Make a copy of the original list.
                        if(!new_key_list)
                        {
                          new_key_list = new KeyList();
                          new_key_list->copy(MusEGlobal::keymap);
                        }
                        new_key_list->addKey(i->a, key_enum(i->b), i->c);

                        updateFlags |= SC_KEY;
                        break;
                        
                  case UndoOp::DeleteKey:
#ifdef _UNDO_DEBUG_
                        fprintf(stderr, "Song::executeOperationGroup1:DeleteKey ** calling keymap.delOperation\n");
#endif                        
                        // Create the new list if it doesn't already exist.
                        // Make a copy of the original list.
                        if(!new_key_list)
                        {
                          new_key_list = new KeyList();
                          new_key_list->copy(MusEGlobal::keymap);
                        }
                        new_key_list->delKey(i->a);

                        updateFlags |= SC_KEY;
                        break;
                        
                  case UndoOp::ModifyKey:
#ifdef _UNDO_DEBUG_
                        fprintf(stderr, "Song::executeOperationGroup1:ModifyKey ** calling keymap.addOperation\n");
#endif                        
                        // Create the new list if it doesn't already exist.
                        // Make a copy of the original list.
                        if(!new_key_list)
                        {
                          new_key_list = new KeyList();
                          new_key_list->copy(MusEGlobal::keymap);
                        }
                        new_key_list->addKey(i->a, key_enum(i->d), i->e);

                        updateFlags |= SC_KEY;
                        break;

                  case UndoOp::ModifySongLen:
#ifdef _UNDO_DEBUG_
                        fprintf(stderr, "Song::executeOperationGroup1:ModifySongLen ** adding ModifySongLen operation\n");
#endif                        
                        pendingOperations.add(PendingOperationItem(i->a, PendingOperationItem::ModifySongLength));
                        updateFlags |= SC_EVERYTHING;  // set all flags   // TODO Refine this! Too many flags.  // REMOVE Tim.
                        //updateFlags |= SC_SONG_LEN;
                        break;
                        
                  case UndoOp::ModifyMidiDivision:
#ifdef _UNDO_DEBUG_
                        fprintf(stderr, "Song::executeOperationGroup1:ModifyMidiDivision\n");
#endif                  
                        MusEGlobal::config.division = i->a;
                        // Make sure the AL namespace variable mirrors our variable.
                        AL::division = MusEGlobal::config.division;
                        // Defer normalize until end of stage 2.
                        updateFlags |= SC_DIVISION_CHANGED;
                        break;
                        
                  case UndoOp::EnableAllAudioControllers:
#ifdef _UNDO_DEBUG_
                        fprintf(stderr, "Song::executeOperationGroup1:EnableAllAudioControllers\n");
#endif                        
                        pendingOperations.add(PendingOperationItem(PendingOperationItem::EnableAllAudioControllers));
                        updateFlags |= SC_AUDIO_CONTROLLER;
                        break;
                        
                  case UndoOp::GlobalSelectAllEvents:
#ifdef _UNDO_DEBUG_
                        fprintf(stderr, "Song::executeOperationGroup1:GlobalSelectAllEvents\n");
#endif                        
                        pendingOperations.add(PendingOperationItem(tracks(), i->a, 0, 0, PendingOperationItem::GlobalSelectAllEvents));
                        updateFlags |= SC_SELECTION;
                        break;
                        
                  case UndoOp::AddMarker:
                          // Create the new list if it doesn't already exist.
                          // Make a copy of the original list.
                          if(!new_marker_list)
                            new_marker_list = new MarkerList(*marker());
                          if(i->newMarker)
                            new_marker_list->add(*i->newMarker);
                          updateFlags |= SC_MARKER_INSERTED;
                        break;
                  
                  case UndoOp::DeleteMarker:
                          // Create the new list if it doesn't already exist.
                          // Make a copy of the original list.
                          if(!new_marker_list)
                            new_marker_list = new MarkerList(*marker());
                          if(i->oldMarker)
                            new_marker_list->remove(*i->oldMarker);
                          updateFlags |= SC_MARKER_REMOVED;
                        break;
                  
                  case UndoOp::ModifyMarker:
                  case UndoOp::SetMarkerPos:
                          // Create the new list if it doesn't already exist.
                          // Make a copy of the original list.
                          if(!new_marker_list)
                            new_marker_list = new MarkerList(*marker());
                          if(i->oldMarker)
                            new_marker_list->remove(*i->oldMarker);
                          if(i->newMarker)
                            new_marker_list->add(*i->newMarker);
                          updateFlags |= SC_MARKER_MODIFIED;
                        break;

                  default:
                        break;
                  }
            }

      if(new_tempo_list)
        pendingOperations.add(PendingOperationItem(&MusEGlobal::tempomap, new_tempo_list, PendingOperationItem::ModifyTempoList));

      if(new_sig_list)
        pendingOperations.add(PendingOperationItem(&MusEGlobal::sigmap, new_sig_list, PendingOperationItem::ModifySigList));

      if(new_key_list)
        pendingOperations.add(PendingOperationItem(&MusEGlobal::keymap, new_key_list, PendingOperationItem::ModifyKeyList));

      if(new_marker_list)
        pendingOperations.add(PendingOperationItem(&_markerList, new_marker_list, PendingOperationItem::ModifyMarkerList));
      }

//---------------------------------------------------------
//   executeOperationGroup3
//    non realtime context
//---------------------------------------------------------

void Song::executeOperationGroup3(Undo& operations)
      {
      pendingOperations.executeNonRTStage();
#ifdef _UNDO_DEBUG_
      fprintf(stderr, "Song::executeOperationGroup3 *** Calling pendingOperations.clear()\n");
#endif                        
      pendingOperations.clear();
      //bool song_has_changed = !operations.empty();
      for (iUndoOp i = operations.begin(); i != operations.end(); ) {
            Track* editable_track = const_cast<Track*>(i->track);
// uncomment if needed            Track* editable_property_track = const_cast<Track*>(i->_propertyTrack);
            Part* editable_part = const_cast<Part*>(i->part); // uncomment if needed
            switch(i->type) {
                  case UndoOp::AddTrack:
                        switch(editable_track->type())
                        {
                          case Track::AUDIO_OUTPUT:
                            // Connect audio output ports to Jack ports...
                            if(MusEGlobal::checkAudioDevice() && MusEGlobal::audio->isRunning())
                            {
                              AudioOutput* ao = (AudioOutput*)editable_track;
                              for(int ch = 0; ch < ao->channels(); ++ch) 
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
                                  //if(ir->jackPort)
                                  if(!MusEGlobal::audioDevice->findPort(route_name))
                                    continue;
                                  //if(!MusEGlobal::audioDevice->portConnectedTo(our_port, route_name))
                                  MusEGlobal::audioDevice->connect(our_port_name, route_name);
                                  updateFlags |= SC_ROUTE;
                                }
                              }
                            }
                          break;
                          
                          case Track::AUDIO_INPUT:
                            // Connect Jack ports to audio input ports...
                            if(MusEGlobal::checkAudioDevice() && MusEGlobal::audio->isRunning())
                            {
                              AudioInput* ai = (AudioInput*)editable_track;
                              for(int ch = 0; ch < ai->channels(); ++ch) 
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
                                  //if(ir->jackPort)
                                  if(!MusEGlobal::audioDevice->findPort(route_name))
                                    continue;
                                  //if(!MusEGlobal::audioDevice->portConnectedTo(our_port, route_name))
                                  MusEGlobal::audioDevice->connect(route_name, our_port_name);
                                  updateFlags |= SC_ROUTE;
                                }
                              }
                            }
                          break;
                            
                          default:
                            break;
                        }
                        
                        break;
                  case UndoOp::DeleteTrack:
                        // Ensure that wave event sndfile file handles are closed.
                        // It should not be the job of the pending operations list to do this.
                        // TODO Coordinate close/open with part mute and/or track off.
                        editable_track->closeAllParts();
                        break;
                  case UndoOp::DeletePart:
                        // Ensure that wave event sndfile file handles are closed.
                        // It should not be the job of the pending operations list to do this.
                        // TODO Coordinate close/open with part mute and/or track off.
                        editable_part->closeAllEvents();
                        break;
                  case UndoOp::DeleteEvent: {
                          if(!i->nEvent.empty())
                          {
                            SndFileR f = i->nEvent.sndFile();
                            // Ensure that wave event sndfile file handle is closed.
                            // It should not be the job of the pending operations list to do this.
                            // TODO Coordinate close/open with part mute and/or track off.
                            if(!f.isNull() && f.isOpen())
                              f.close();
                          }
                        }
                        break;
                  case UndoOp::ModifyMidiDivision:
                        // This also tells all connected models to begin/end reset.
                        MusEGlobal::globalRasterizer->setDivision(i->a);
                        break;

                  // Re-enable any controllers so that the results can be seen.
                  // Exclude write/latch mode because controls need to remain disabled if pressed before play.
                  // TODO: This really should be done in the realtime operations stage.
                  //       But we don't pass the track to that stage. It would need the track.
                  //       Or make a separate EnableAudioController operations command,
                  //        but that's one extra command clogging up the commands list for
                  //        each one of these...
                  case UndoOp::AddAudioCtrlVal:
                  case UndoOp::DeleteAudioCtrlVal:
                    if(editable_track && !editable_track->isMidiTrack())
                    {
                      AudioTrack* audio_track = static_cast<AudioTrack*>(editable_track);
                      const AutomationType automation_type = audio_track->automationType();
                      if(automation_type != AUTO_WRITE && automation_type != AUTO_LATCH)
                        audio_track->enableController(i->_audioCtrlIdAddDel, true);
                    }
                  break;
                  case UndoOp::AddAudioCtrlValStruct:
                    if(editable_track && !editable_track->isMidiTrack())
                    {
                      AudioTrack* audio_track = static_cast<AudioTrack*>(editable_track);
                      const AutomationType automation_type = audio_track->automationType();
                      if(automation_type != AUTO_WRITE && automation_type != AUTO_LATCH)
                        audio_track->enableController(i->_audioCtrlIdStruct, true);
                    }
                  break;
                  case UndoOp::ModifyAudioCtrlVal:
                    if(editable_track && !editable_track->isMidiTrack())
                    {
                      AudioTrack* audio_track = static_cast<AudioTrack*>(editable_track);
                      const AutomationType automation_type = audio_track->automationType();
                      if(automation_type != AUTO_WRITE && automation_type != AUTO_LATCH)
                        audio_track->enableController(i->_audioCtrlID, true);
                    }
                  break;
                  case UndoOp::ModifyAudioCtrlValList:
                    if(editable_track && !editable_track->isMidiTrack())
                    {
                      AudioTrack* audio_track = static_cast<AudioTrack*>(editable_track);
                      const AutomationType automation_type = audio_track->automationType();
                      if(automation_type != AUTO_WRITE && automation_type != AUTO_LATCH)
                        audio_track->enableController(i->_audioCtrlIdModify, true);
                    }
                  break;

                   default:
                        break;
                  }
            
            // Is the operation marked as non-undoable? Remove it from the list.
            if(i->_noUndo)
            {
              // Be sure to delete any allocated stuff in the op before erasing.
              i = operations.deleteAndErase(i);
            }
            else
              ++i;
            }
            
      // If some operations marked as non-undoable were removed, it is OK,
      //  because we only want dirty if an undoable operation was executed, right?
      if(!operations.empty())
      // Hm, no. ANY operation actually changes things, so yes, the song is dirty.
      //if(song_has_changed)
        emit sigDirty();
      }


bool Undo::empty() const
{
  if (std::list<UndoOp>::empty()) return true;
  
  for (const_iterator it=begin(); it!=end(); it++)
    if (it->type!=UndoOp::DoNothing)
      return false;
  
  return true;
}

} // namespace MusECore
