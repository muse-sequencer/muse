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

#include "al/sig.h"  
#include "keyevent.h"

#include "undo.h"
#include "song.h"
#include "globals.h"
#include "audio.h"  
#include "operations.h"
#include "tempo.h"
#include "part.h"
#include "audiodev.h"
#include "track.h"

#include <string.h>
#include <QAction>
#include <QString>
#include <set>

// Enable for debugging:
//#define _UNDO_DEBUG_

namespace MusECore {

// iundo points to last Undo() in Undo-list

static bool undoMode = false;  // for debugging

std::list<QString> temporaryWavFiles;

//---------------------------------------------------------
//   typeName
//---------------------------------------------------------

const char* UndoOp::typeName()
      {
      static const char* name[] = {
            "AddRoute", "DeleteRoute", 
            "AddTrack", "DeleteTrack", 
            "AddPart",  "DeletePart", "MovePart", "ModifyPartLength", "ModifyPartName", "SelectPart",
            "AddEvent", "DeleteEvent", "ModifyEvent", "SelectEvent",
            "AddAudioCtrlVal", "DeleteAudioCtrlVal", "ModifyAudioCtrlVal", "ModifyAudioCtrlValList",
            "AddTempo", "DeleteTempo", "ModifyTempo", "SetGlobalTempo",
            "AddSig",   "DeleteSig",   "ModifySig",
            "AddKey",   "DeleteKey",   "ModifyKey",
            "ModifyTrackName", "ModifyTrackChannel",
            "SetTrackRecord", "SetTrackMute", "SetTrackSolo",
            "MoveTrack",
            "ModifyClip", "ModifyMarker",
            "ModifySongLen", "DoNothing",
            "EnableAllAudioControllers"
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
            default:      
                  break;
            }
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
        {
          switch(i->type)
          {
            case UndoOp::DeleteTrack:
                  if(i->track)
                    delete const_cast<Track*>(i->track);
                  break;
                  
            case UndoOp::DeletePart:
                  delete const_cast<Part*>(i->part);
                  break;

            case UndoOp::ModifyMarker:
                  if (i->copyMarker)
                    delete i->copyMarker;
                  break;
                  
            case UndoOp::ModifyPartName:
            case UndoOp::ModifyTrackName:
                  if (i->_oldName)
                    delete i->_oldName;
                  if (i->_newName)
                    delete i->_newName;
                  break;
            
            case UndoOp::ModifyAudioCtrlValList:
                  if (i->_eraseCtrlList)
                    delete i->_eraseCtrlList;
                  if (i->_addCtrlList)
                    delete i->_addCtrlList;
                  break;
                  
            default:
                  break;
          }
        }
        u.clear();
      }
    }
    else
    {
      for(riUndo iu = rbegin(); iu != rend(); ++iu)
      {
        Undo& u = *iu;
        for(riUndoOp i = u.rbegin(); i != u.rend(); ++i)
        {
          switch(i->type)
          {
            case UndoOp::AddTrack:
                  delete i->track;
                  break;
                  
            case UndoOp::AddPart:
                  delete i->part;
                  break;

            case UndoOp::ModifyMarker:
                  if (i->realMarker)
                    delete i->realMarker;
                  break;
                  
            case UndoOp::ModifyPartName:
            case UndoOp::ModifyTrackName:
                  if (i->_oldName)
                    delete i->_oldName;
                  if (i->_newName)
                    delete i->_newName;
                  break;
            
            case UndoOp::ModifyAudioCtrlValList:
                  if (i->_eraseCtrlList)
                    delete i->_eraseCtrlList;
                  if (i->_addCtrlList)
                    delete i->_addCtrlList;
                  break;
                  
            default:
                  break;
          }
        }
        u.clear();
      }
    }
  }

  clear();
}

//---------------------------------------------------------
//    startUndo
//---------------------------------------------------------

void Song::startUndo()
      {
      redoList->clearDelete(); // redo must be invalidated when a new undo is started
      MusEGlobal::redoAction->setEnabled(false);
      setUndoRedoText();
      
      undoList->push_back(Undo());
      updateFlags = 0;
      undoMode = true;
      }

//---------------------------------------------------------
//   endUndo
//---------------------------------------------------------

void Song::endUndo(SongChangedFlags_t flags)
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
    case UndoOp::DeleteAudioCtrlVal:
      fprintf(stderr, "Undo::insert: DeleteAudioCtrlVal\n");
    break;
    case UndoOp::ModifyAudioCtrlVal:
      fprintf(stderr, "Undo::insert: ModifyAudioCtrlVal\n");
    break;
    case UndoOp::ModifyAudioCtrlValList:
      fprintf(stderr, "Undo::insert: ModifyAudioCtrlValList\n");
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
    case UndoOp::SetGlobalTempo:
      fprintf(stderr, "Undo::insert: SetGlobalTempo\n");
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
    
    
    case UndoOp::ModifyMarker:
      fprintf(stderr, "Undo::insert: ModifyMarker\n");
    break;

    
    case UndoOp::ModifySongLen:
      fprintf(stderr, "Undo::insert: ModifySongLen\n");
    break;

    
    case UndoOp::DoNothing:
      fprintf(stderr, "Undo::insert: DoNothing\n");
    break;
    
    case UndoOp::EnableAllAudioControllers:
      fprintf(stderr, "Undo::insert: EnableAllAudioControllers\n");
    break;
    
    
    default:
    break;
  }
#endif

  // (NOTE: Use this handy speed-up 'if' line to exclude unhandled operation types)
  if(n_op.type != UndoOp::ModifyTrackChannel && n_op.type != UndoOp::ModifyClip && n_op.type != UndoOp::ModifyMarker && n_op.type != UndoOp::DoNothing) 
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
          if(uo.type == UndoOp::AddRoute && uo.routeFrom == n_op.routeFrom && uo.routeTo == n_op.routeTo)
          {
            fprintf(stderr, "MusE error: Undo::insert(): Double AddRoute. Ignoring.\n");
            return;
          }
          else if(uo.type == UndoOp::DeleteRoute && uo.routeFrom == n_op.routeFrom && uo.routeTo == n_op.routeTo)
          {
            // Delete followed by add is useless. Cancel out the delete + add by erasing the delete command.
            erase(iuo);
            return;  
          }
        break;
        
        case UndoOp::DeleteRoute:
          if(uo.type == UndoOp::DeleteRoute && uo.routeFrom == n_op.routeFrom && uo.routeTo == n_op.routeTo)  
          {
            fprintf(stderr, "MusE error: Undo::insert(): Double DeleteRoute. Ignoring.\n");
            return;  
          }
          else if(uo.type == UndoOp::AddRoute && uo.routeFrom == n_op.routeFrom && uo.routeTo == n_op.routeTo)  
          {
            // Add followed by delete is useless. Cancel out the add + delete by erasing the add command.
            erase(iuo);
            return;  
          }
        break;

        
        case UndoOp::ModifyTrackName:
          if(uo.type == UndoOp::ModifyTrackName && uo.track == n_op.track)  
          {
            fprintf(stderr, "MusE error: Undo::insert(): Double ModifyTrackName. Ignoring.\n");
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
        
        case UndoOp::ModifyPartName:
          if(uo.type == UndoOp::ModifyPartName && uo.part == n_op.part)  
          {
            fprintf(stderr, "MusE error: Undo::insert(): Double ModifyPartName. Ignoring.\n");
            return;  
          }
        break;
        
        case UndoOp::ModifyPartLength:
          if(uo.type == UndoOp::ModifyPartLength && uo.part == n_op.part)  
          {
            // Simply replace the new value.
            uo.new_partlen_or_pos = n_op.new_partlen_or_pos;
            return;  
          }
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
          else if(uo.type == UndoOp::DeleteEvent && uo.nEvent == n_op.nEvent && uo.part == n_op.part)  
          {
            // Delete followed by add is useless. Cancel out the delete + add by erasing the delete command.
            erase(iuo);
            return;  
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
            if(uo.oEvent == n_op.oEvent && uo.nEvent == n_op.nEvent)
            {
              fprintf(stderr, "MusE error: Undo::insert(): Double ModifyEvent. Ignoring.\n");
              return;
            }
            // Are inner add/delete pair the same event?
            else if(uo.nEvent == n_op.oEvent) 
            {
              // Are outer delete/add pair the same event?
              if(uo.oEvent == n_op.nEvent)
              {
                // First ModifyEvent old event and second ModifyEvent new event are both the same, equivalent to doing nothing.
                // Cancel out the two ModifyEvent operations by erasing the existing ModifyEvent command.
                erase(iuo);
                return;  
              }
              else
              {
                // Outer delete/add pair are not the same event... 
                // Transform the existing ModifyEvent operation into a DeleteEvent.
                uo.type = UndoOp::DeleteEvent;
                uo.nEvent = uo.oEvent;
                // Transform the requested ModifyEvent operation into an AddEvent.
                n_op.type = UndoOp::AddEvent;
                // Allow it to add...
              }
            }
            // Inner add/delete pair are not the same event. Are outer delete/add pair the same event?
            else if(uo.oEvent == n_op.nEvent) 
            {
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
            if(uo.nEvent == n_op.oEvent)
            {
              // Add followed by modify with old event same as addded event, is equivalent to just adding modify's new event.
              // Replace the existing AddEvent command's event with the requested ModifyEvent command's new event.
              uo.nEvent = n_op.nEvent;
              return;  
            }
            if(uo.nEvent == n_op.nEvent)
            {
              // Add followed by modify with new event same as addded event, is a caller error.
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
              // Delete followed by modify with new event same as deleted event, is equivalent to just deleting modify's old event.
              // Replace the existing DeleteEvent command's event with the requested ModifyEvent command's old event.
              uo.nEvent = n_op.oEvent;
            }
          }
        break;

// TODO Only skeletal changes done so far here...
//         case UndoOp::AddAudioCtrlVal:
//           if(uo.type == UndoOp::AddAudioCtrlVal && uo.nEvent == n_op.nEvent && uo.part == n_op.part)  
//           {
//             fprintf(stderr, "MusE error: Undo::insert(): Double AddAudioCtrlVal. Ignoring.\n");
//             return;  
//           }
//           else if(uo.type == UndoOp::DeleteAudioCtrlVal && uo.nEvent == n_op.nEvent && uo.part == n_op.part)  
//           {
//             // Delete followed by add is useless. Cancel out the delete + add by erasing the delete command.
//             erase(iuo);
//             return;  
//           }
//           else if(uo.type == UndoOp::ModifyAudioCtrlVal && uo.part == n_op.part)
//           {
//             if(uo.nEvent == n_op.nEvent)  
//             {
//               // Modify followed by adding of the modify's new event, is equivalent to just modifying with the added event. 
//               fprintf(stderr, "MusE error: Undo::insert(): ModifyAudioCtrlVal, then AddAudioCtrlVal same new event (double AddAudioCtrlVal). Ignoring.\n");
//               return;  
//             }
//             else if(uo.oEvent == n_op.nEvent)  
//             {
//               // Modify followed by adding of the modify's old event, is equivalent to just adding the event. 
//               // Transform the ModifyEvent operation into an AddAudioCtrlVal.
//               uo.type = UndoOp::AddAudioCtrlVal;
//               uo.nEvent = uo.oEvent;
//               return;  
//             }
//           }
//         break;
//         
//         case UndoOp::DeleteAudioCtrlVal:
//           if(uo.type == UndoOp::DeleteAudioCtrlVal && uo.nEvent == n_op.nEvent && uo.part == n_op.part)  
//           {
//             fprintf(stderr, "MusE error: Undo::insert(): Double DeleteAudioCtrlVal. Ignoring.\n");
//             return;  
//           }
//           else if(uo.type == UndoOp::AddAudioCtrlVal && uo.nEvent == n_op.nEvent && uo.part == n_op.part)  
//           {
//             // Add followed by delete is useless. Cancel out the add + delete by erasing the add command.
//             erase(iuo);
//             return;  
//           }
//           else if(uo.type == UndoOp::ModifyAudioCtrlVal && uo.part == n_op.part)  
//           {
//             if(uo.oEvent == n_op.nEvent)  
//             {
//               // Modify followed by delete of the modify's old event, is an error - two deletes of the same event. 
//               fprintf(stderr, "MusE error: Undo::insert(): ModifyAudioCtrlVal, then DeleteAudioCtrlVal same old event (double DeleteAudioCtrlVal). Ignoring.\n");
//               return;  
//             }
//             else if(uo.nEvent == n_op.nEvent)
//             {
//               // Modify followed by delete of the modify's new event, is equivalent to just deleting the old event. 
//               // Transform the operation into a DeleteAudioCtrlVal.
//               uo.type = UndoOp::DeleteAudioCtrlVal;
//               uo.nEvent = uo.oEvent;
//               return;  
//             }
//           }
//         break;
// 
//         case UndoOp::ModifyAudioCtrlVal:
//           if(n_op.oEvent == n_op.nEvent)
//           {
//             // Equivalent to deleting then adding the same event - useless, cancels out.
//             return;
//           }
//           else if(uo.type == UndoOp::ModifyAudioCtrlVal && uo.part == n_op.part)  
//           {
//             if(uo.oEvent == n_op.oEvent && uo.nEvent == n_op.nEvent)
//             {
//               fprintf(stderr, "MusE error: Undo::insert(): Double ModifyAudioCtrlVal. Ignoring.\n");
//               return;
//             }
//             // Are inner add/delete pair the same event?
//             else if(uo.nEvent == n_op.oEvent) 
//             {
//               // Are outer delete/add pair the same event?
//               if(uo.oEvent == n_op.nEvent)
//               {
//                 // First ModifyAudioCtrlVal old event and second ModifyAudioCtrlVal new event are both the same, equivalent to doing nothing.
//                 // Cancel out the two ModifyAudioCtrlVal operations by erasing the existing ModifyAudioCtrlVal command.
//                 erase(iuo);
//                 return;  
//               }
//               else
//               {
//                 // Outer delete/add pair are not the same event... 
//                 // Transform the existing ModifyAudioCtrlVal operation into a DeleteAudioCtrlVal.
//                 uo.type = UndoOp::DeleteAudioCtrlVal;
//                 uo.nEvent = uo.oEvent;
//                 // Transform the requested ModifyEvent operation into an AddAudioCtrlVal.
//                 n_op.type = UndoOp::AddAudioCtrlVal;
//                 // Allow it to add...
//               }
//             }
//             // Inner add/delete pair are not the same event. Are outer delete/add pair the same event?
//             else if(uo.oEvent == n_op.nEvent) 
//             {
//               // Transform the existing ModifyAudioCtrlVal operation into an AddAudioCtrlVal.
//               uo.type = UndoOp::AddAudioCtrlVal;
//               // Transform the requested ModifyAudioCtrlVal operation into a DeleteAudioCtrlVal.
//               n_op.type = UndoOp::DeleteAudioCtrlVal;
//               n_op.nEvent = n_op.oEvent;
//               // Allow it to add...
//             }
//           }
//           else if(uo.type == UndoOp::AddAudioCtrlVal && uo.part == n_op.part)
//           {
//             if(uo.nEvent == n_op.oEvent)
//             {
//               // Add followed by modify with old event same as addded event, is equivalent to just adding modify's new event.
//               // Replace the existing AddAudioCtrlVal command's event with the requested ModifyAudioCtrlVal command's new event.
//               uo.nEvent = n_op.nEvent;
//               return;  
//             }
//             if(uo.nEvent == n_op.nEvent)
//             {
//               // Add followed by modify with new event same as addded event, is a caller error.
//               fprintf(stderr, "MusE error: Undo::insert(): AddAudioCtrlVal, then ModifyAudioCtrlVal same new event (double AddAudioCtrlVal). Ignoring.\n");
//               return;  
//             }
//           }
//           if(uo.type == UndoOp::DeleteAudioCtrlVal && uo.part == n_op.part)
//           {
//             if(uo.nEvent == n_op.oEvent)
//             {
//               // Delete followed by modify with old event same as deleted event, is an error.
//               fprintf(stderr, "MusE error: Undo::insert(): DeleteAudioCtrlVal, then ModifyAudioCtrlVal same old event (double DeleteAudioCtrlVal). Ignoring.\n");
//               return;  
//             }
//             if(uo.nEvent == n_op.nEvent)
//             {
//               // Delete followed by modify with new event same as deleted event, is equivalent to just deleting modify's old event.
//               // Replace the existing DeleteAudioCtrlVal command's event with the requested ModifyAudioCtrlVal command's old event.
//               uo.nEvent = n_op.oEvent;
//             }
//           }
//         break;

        case UndoOp::ModifyAudioCtrlValList:
          // Check the sanity of the requested op.
          if(n_op._eraseCtrlList == n_op._addCtrlList)
          {
            fprintf(stderr, "MusE error: Undo::insert(): ModifyAudioCtrlValList: Erase and add lists are the same. Ignoring.\n");
            return;
          }
          
          if(uo.type == UndoOp::ModifyAudioCtrlValList)
          {
            if(uo._ctrlListList == n_op._ctrlListList)
            {
              if(uo._addCtrlList == n_op._addCtrlList && uo._eraseCtrlList == n_op._eraseCtrlList)
              {
                fprintf(stderr, "MusE error: Undo::insert(): Double ModifyAudioCtrlValList. Ignoring.\n");
                return;
              }
              else if(uo._addCtrlList == n_op._eraseCtrlList)
              {
                // Delete the existing ModifyAudioCtrlValList command's _addCtrlList and replace it
                //  with the requested ModifyAudioCtrlValList command's _addCtrlList.
                if(uo._addCtrlList)
                  delete uo._addCtrlList;
                uo._addCtrlList = n_op._addCtrlList;
                return;
              }
            }
            // Seems possible... remove? But maybe dangerous to have two undo ops pointing to the same lists - they will be self-deleted.
            else
            {
              if(uo._addCtrlList == n_op._addCtrlList)
              {
                fprintf(stderr, "MusE error: Undo::insert(): ModifyAudioCtrlValList: Attempting to add same list to different containers. Ignoring.\n");
                return;
              }
              else if(uo._eraseCtrlList == n_op._eraseCtrlList)
              {
                fprintf(stderr, "MusE error: Undo::insert(): ModifyAudioCtrlValList: Attempting to erase same list from different containers. Ignoring.\n");
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
        break;
          
        case UndoOp::SetGlobalTempo:
          if(uo.type == UndoOp::SetGlobalTempo)  
          {
            // Simply replace a with the new value.
            uo.a = n_op.a;
            return;  
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
            return;  
          }
          else if(uo.type == UndoOp::DeleteKey && uo.a == n_op.a)  
          {
            // Delete followed by add. Transform the existing DeleteKey operation into a ModifyKey.
            uo.type = UndoOp::ModifyKey;
            // a is already the tick, b is already the existing value from DeleteKey, c is the new value.
            uo.c = n_op.b;
            return;  
          }
          else if(uo.type == UndoOp::ModifyKey && uo.a == n_op.a)  
          {
            // Modify followed by add. Simply replace the value.
            // a is already the tick, b is already the existing value from ModifyKey, c is the new value.
            uo.c = n_op.b;
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
            // a is already the tick, b is already the existing old value from ModifyKey.
            return;  
          }
        break;
        
        case UndoOp::ModifyKey:
          if(uo.type == UndoOp::ModifyKey && uo.a == n_op.a)  
          {
            // Simply replace c with the new value.
            uo.c = n_op.c;
            return;  
          }
          else if(uo.type == UndoOp::AddKey && uo.a == n_op.a)  
          {
            // Add followed by modify. Simply replace the add value.
            uo.b = n_op.c;
            return;  
          }
          else if(uo.type == UndoOp::DeleteKey && uo.a == n_op.a)  
          {
            // Delete followed by modify. Equivalent to modify. Transform existing DeleteSig operation into a ModifySig.
            uo.type = UndoOp::ModifyKey;
            // a is already the tick, b is already the existing value from DeleteKey. c is the new value from ModifyKey.
            uo.c = n_op.c;
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
        
        case UndoOp::EnableAllAudioControllers:
          if(uo.type == UndoOp::EnableAllAudioControllers)
          {
            fprintf(stderr, "MusE error: Undo::insert(): Double EnableAllAudioControllers. Ignoring.\n");
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
	if (this->combobreaker || other.combobreaker)
		return false;
	
	bool has_select_event=false;
	bool has_select_part=false;
	bool has_other=false;
	
	for (ciUndoOp op=this->begin(); op!=this->end(); op++)
		switch(op->type)
		{
			case UndoOp::DoNothing: break;
			case UndoOp::SelectEvent: has_select_event=true; break;
			case UndoOp::SelectPart: has_select_part=true; break;
			default: has_other=true; break;
		}
	
	for (ciUndoOp op=other.begin(); op!=other.end(); op++)
		switch(op->type)
		{
			case UndoOp::DoNothing: break;
			case UndoOp::SelectEvent: has_select_event=true; break;
			case UndoOp::SelectPart: has_select_part=true; break;
			default: has_other=true; break;
		}
	
	bool mergeable = (has_select_event && !has_select_part && !has_other) ||
	                 (has_select_part && !has_select_event && !has_other);
	
	if (mergeable)
		this->insert(this->end(), other.begin(), other.end());
	
	return mergeable;
}

bool Song::applyOperation(const UndoOp& op, bool doUndo)
{
	Undo operations;
	operations.push_back(op);
	return applyOperationGroup(operations, doUndo);
}

bool Song::applyOperationGroup(Undo& group, bool doUndo)
{
      if (!group.empty())
      {
            if (doUndo)
                 startUndo();

            MusEGlobal::audio->msgExecuteOperationGroup(group);
            
            // append all elements from "group" to the end of undoList->back().
            Undo& curUndo = undoList->back();
            curUndo.insert(curUndo.end(), group.begin(), group.end());
            if (group.combobreaker)
               curUndo.combobreaker=true;
            
            if (doUndo)
                 endUndo(0);
            
            return doUndo;
      }
      else
            return false;
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
        if(updateFlags & SC_TEMPO)
        {
          MusEGlobal::tempomap.normalize();
          MusEGlobal::audio->reSyncAudio();
        }
        // Special for sig: Need to normalize the signature list. 
        // To save time this is done here, not item by item.
        if(updateFlags & SC_SIG)
          AL::sigmap.normalize();

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
            {
              at->addAuxSend(n);
              updateFlags |= SC_AUX;
            }
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
        if(updateFlags & SC_TEMPO)
        {
          MusEGlobal::tempomap.normalize();
          MusEGlobal::audio->reSyncAudio();
        }
        // Special for sig: Need to normalize the signature list. 
        // To save time this is done here, not item by item.
        if(updateFlags & SC_SIG)
          AL::sigmap.normalize();
        
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
            {
              at->addAuxSend(n);
              updateFlags |= SC_AUX;
            }
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
             type_==AddTempo || type_==DeleteTempo || type_==ModifyTempo || type_==SetGlobalTempo ||  
             type_==AddSig || type_==DeleteSig ||
             type_==ModifySongLen || type_==MoveTrack);
      
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
        
        case UndoOp::AddSig:
        {
          //if(t > MAX_TICK)
          //  t = MAX_TICK;
          
          // Must rasterize the tick value HERE instead of in SigMap::addOperation(),
          //  so that the rasterized value is recorded in the undo item.
          a = AL::sigmap.raster1(a, 0);
          
          AL::iSigEvent ise = AL::sigmap.upper_bound(a);
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
            // a is already the tick, b is the existing value, c is the new value.
            type = UndoOp::ModifyKey;
            c = b;
            b = ike->second.key;
          }
        }
        break;
        
        default:
        break;
      }
      
      }

UndoOp::UndoOp(UndoType type_, int tick, const AL::TimeSignature old_sig, const AL::TimeSignature new_sig, bool noUndo)
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

UndoOp::UndoOp(UndoType type_, const Track* track_, bool value, bool noUndo)
      {
      assert(type_== SetTrackRecord || type_== SetTrackMute || type_== SetTrackSolo);
      assert(track_);
      
      type    = type_;
      track  = track_;
      a = value;
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

UndoOp::UndoOp(UndoType type_, const Part* part_, int old_len_or_pos, int new_len_or_pos, Pos::TType new_time_type_, const Track* oTrack, const Track* nTrack, bool noUndo)
{
    assert(type_== ModifyPartLength || type_== MovePart);
    assert(part_);
    
    type = type_;
    part = part_;
    _noUndo = noUndo;
    if(type_== MovePart)
    {
      track = nTrack;
      oldTrack = oTrack;
      // If the tracks are the same or either track is null, we just want a regular move (to the same track).
      if(oTrack == 0 || nTrack == 0 || oTrack == nTrack)
      {
        track = 0;
        oldTrack = 0;
      }
    }
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
            if(type_== ModifyPartLength)
              new_partlen_or_pos = MusEGlobal::tempomap.deltaTick2frame(part->tick(), part->tick() + new_partlen_or_pos);
            else
              new_partlen_or_pos = MusEGlobal::tempomap.tick2frame(new_partlen_or_pos);
          break;  
        }
      break;
      
      case Pos::TICKS:
        switch(new_time_type_)
        {
          case Pos::FRAMES:
            if(type_== ModifyPartLength)
              new_partlen_or_pos = MusEGlobal::tempomap.deltaFrame2tick(part->frame(), part->frame() + new_partlen_or_pos);
            else
              new_partlen_or_pos = MusEGlobal::tempomap.frame2tick(new_partlen_or_pos);
          break;

          case Pos::TICKS:
          break;  
        }
      break;
    }
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
      
UndoOp::UndoOp(UndoType type_, Marker* copyMarker_, Marker* realMarker_, bool noUndo)
      {
      assert(type_==ModifyMarker);
      assert(copyMarker_);
      assert(realMarker_);
      
      type    = type_;
      realMarker  = realMarker_;
      copyMarker  = copyMarker_;
      _noUndo = noUndo;
      }

UndoOp::UndoOp(UndoType type_, const Event& changedEvent, const QString& changeData, int startframe_, int endframe_, bool noUndo)
      {
      assert(type_==ModifyClip);
      
      type = type_;
      _noUndo = noUndo;
      //filename   = new QString(changedFile);
      nEvent = changedEvent;
      tmpwavfile = new QString(changeData);
      startframe = startframe_;
      endframe   = endframe_;
      }

UndoOp::UndoOp(UndoOp::UndoType type_, const Part* part_, const QString& old_name, const QString& new_name, bool noUndo)
{
    assert(type_==ModifyPartName);
    assert(part_);
//    assert(old_name);
//    assert(new_name);
    
    type=type_;
    part=part_;
    _noUndo = noUndo;
    _oldName = new QString(old_name);
    _newName = new QString(new_name);
    //strcpy(_oldName, old_name);
    //strcpy(_newName, new_name);
}

UndoOp::UndoOp(UndoOp::UndoType type_, const Track* track_, const QString& old_name, const QString& new_name, bool noUndo)
{
  assert(type_==ModifyTrackName);
  assert(track_);
//  assert(old_name);
//  assert(new_name);
    
  type = type_;
  track = track_;
  _noUndo = noUndo;
  _oldName = new QString(old_name);
  _newName = new QString(new_name);
//  strcpy(_oldName, old_name);
//  strcpy(_newName, new_name);
}

UndoOp::UndoOp(UndoOp::UndoType type_, const Track* track_, int old_chan, int new_chan, bool noUndo)
{
  assert(type_==ModifyTrackChannel);
  assert(track_);
  
  type = type_;
  _propertyTrack = track_;
  _oldPropValue = old_chan;
  _newPropValue = new_chan;
  _noUndo = noUndo;
}

UndoOp::UndoOp(UndoOp::UndoType type_, CtrlListList* ctrl_ll, CtrlList* eraseCtrlList, CtrlList* addCtrlList, bool noUndo)
{
  assert(type_== ModifyAudioCtrlValList);
  assert(ctrl_ll);
  //assert(eraseCtrlList);
  //assert(addCtrlList);
  assert(eraseCtrlList || addCtrlList);
  
  type = type_;
  _ctrlListList = ctrl_ll;
  _eraseCtrlList = eraseCtrlList;
  _addCtrlList = addCtrlList;
  _noUndo = noUndo;
}
      
UndoOp::UndoOp(UndoOp::UndoType type_)
{
  assert(type_== EnableAllAudioControllers);
  
  type = type_;
  // Cannot be undone. 'One-time' operation only, removed after execution.
  _noUndo = true;
}
      
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
UndoOp::UndoOp(UndoOp::UndoType type_, const Route& route_from_, const Route& route_to_, bool noUndo)
      {
      assert(type_ == AddRoute || type_ == DeleteRoute);
      _noUndo = noUndo;
      routeFrom = route_from_;
      routeTo = route_to_;
      }
#pragma GCC diagnostic pop

void Song::undoOp(UndoOp::UndoType type, const Event& changedEvent, const QString& changeData, int startframe, int endframe)
      {
      addUndo(UndoOp(type,changedEvent,changeData,startframe,endframe));
      temporaryWavFiles.push_back(QString(changeData));
      }

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
      for (riUndoOp i = operations.rbegin(); i != operations.rend(); ++i) {
            Track* editable_track = const_cast<Track*>(i->track);
            Track* editable_property_track = const_cast<Track*>(i->_propertyTrack);
            Part* editable_part = const_cast<Part*>(i->part);
            switch(i->type) {
                  case UndoOp::SelectPart:
                        editable_part->setSelected(i->selected_old);
                        updateFlags |= SC_PART_SELECTION;
                        break;
                  case UndoOp::SelectEvent:
			selectEvent(i->nEvent, editable_part, i->selected_old);
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
                            if(!s->isActivated()) 
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
                                // Register the track's jack port, or just set the name.
                                char buffer[128];
                                snprintf(buffer, 128, "%s-%d", ao->name().toLatin1().constData(), ch);
                                // REMOVE Tim. Persistent routes. Added. Don't think we need this here.
                                if(ao->jackPort(ch))
                                {
                                  MusEGlobal::audioDevice->setPortName(ao->jackPort(ch), buffer);
                                  updateFlags |= SC_ROUTE;
                                }
                                else
                                {  
                                  // This should be OK since the track has not yet been added in the realtime stage.
                                  ao->setJackPort(ch, MusEGlobal::audioDevice->registerOutPort(buffer, false));
                                  updateFlags |= SC_ROUTE;
                                }
                                
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
                                // Register the track's jack port, or just set the name.
                                char buffer[128];
                                snprintf(buffer, 128, "%s-%d", ai->name().toLatin1().constData(), ch);
                                // REMOVE Tim. Persistent routes. Added. Don't think we need this here.
                                if(ai->jackPort(ch))
                                {
                                  MusEGlobal::audioDevice->setPortName(ai->jackPort(ch), buffer);
                                  updateFlags |= SC_ROUTE;
                                }
                                else
                                {
                                  // This should be OK since the track has not yet been added in the realtime stage.
                                  ai->setJackPort(ch, MusEGlobal::audioDevice->registerInPort(buffer, false));
                                  updateFlags |= SC_ROUTE;
                                }
                                
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
                        MusECore::SndFile::applyUndoFile(i->nEvent, i->tmpwavfile, i->startframe, i->endframe);
                        updateFlags |= SC_CLIP_MODIFIED;
                        break;
                  case UndoOp::ModifyTrackChannel:
                        if (editable_property_track->isMidiTrack()) 
                        {
                          MusECore::MidiTrack* mt = static_cast<MusECore::MidiTrack*>(editable_property_track);
                          if (i->_oldPropValue != mt->outChannel()) 
                          {
                                MusEGlobal::audio->msgIdle(true);
                                mt->setOutChanAndUpdate(i->_oldPropValue);
                                MusEGlobal::audio->msgIdle(false);
                                MusEGlobal::audio->msgUpdateSoloStates();                   
                                updateFlags |= SC_MIDI_TRACK_PROP;               
                          }
                        }
                        else
                        {
                            if(editable_property_track->type() != MusECore::Track::AUDIO_SOFTSYNTH)
                            {
                              MusECore::AudioTrack* at = static_cast<MusECore::AudioTrack*>(editable_property_track);
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
                        updateFlags |= SC_RECFLAG;
                        break;

                  case UndoOp::SetTrackMute:
                        pendingOperations.add(PendingOperationItem(editable_track, !i->a, PendingOperationItem::SetTrackMute));
                        updateFlags |= SC_MUTE;
                        break;

                  case UndoOp::SetTrackSolo:
                        pendingOperations.add(PendingOperationItem(editable_track, !i->a, PendingOperationItem::SetTrackSolo));
                        updateFlags |= SC_SOLO;
                        break;

                        
                  case UndoOp::AddRoute:
#ifdef _UNDO_DEBUG_
                        fprintf(stderr, "Song::revertOperationGroup1:AddRoute\n");
#endif                        
                        pendingOperations.add(PendingOperationItem(i->routeFrom, i->routeTo, PendingOperationItem::DeleteRoute)); 
                        updateFlags |= SC_ROUTE;
                        break;
                        
                  case UndoOp::DeleteRoute:
#ifdef _UNDO_DEBUG_
                        fprintf(stderr, "Song::executeOperationGroup1:DeleteRoute\n");
#endif                        
                        pendingOperations.add(PendingOperationItem(i->routeFrom, i->routeTo, PendingOperationItem::AddRoute)); 
                        updateFlags |= SC_ROUTE;
                        break;
                        
                  case UndoOp::ModifyTrackName:
                        pendingOperations.add(PendingOperationItem(editable_track, i->_oldName, PendingOperationItem::ModifyTrackName));
                        updateFlags |= SC_TRACK_MODIFIED;
                        break;
                        
                  case UndoOp::MoveTrack:
                        pendingOperations.add(PendingOperationItem(&_tracks, i->b, i->a, PendingOperationItem::MoveTrack));
                        updateFlags |= SC_TRACK_MODIFIED;
                        break;
                        
                  case UndoOp::ModifyPartName:
                        pendingOperations.add(PendingOperationItem(editable_part, i->_oldName, PendingOperationItem::ModifyPartName));
                        updateFlags |= SC_PART_MODIFIED;
                        break;
                        
                  case UndoOp::ModifyPartLength: 
                        removePortCtrlEvents(editable_part, editable_part->track(), pendingOperations);
                        pendingOperations.add(PendingOperationItem(editable_part, i->old_partlen_or_pos, PendingOperationItem::ModifyPartLength));
                        addPortCtrlEvents(editable_part, editable_part->tick(), i->old_partlen_or_pos, editable_part->track(), pendingOperations);
                        updateFlags |= SC_PART_MODIFIED;
                        break;
                        
                  case UndoOp::MovePart:
#ifdef _UNDO_DEBUG_
                        fprintf(stderr, "Song::revertOperationGroup1:MovePart ** calling parts->movePartOperation\n");
#endif                        
                        editable_part->track()->parts()->movePartOperation(editable_part, i->old_partlen_or_pos, pendingOperations, const_cast<Track*>(i->oldTrack));
                        if(const_cast<Track*>(i->oldTrack))
                          updateFlags |= SC_PART_INSERTED | SC_PART_REMOVED;
                        updateFlags |= SC_PART_MODIFIED;
                        break;
                        
                  case UndoOp::AddPart:
#ifdef _UNDO_DEBUG_
                        fprintf(stderr, "Song::revertOperationGroup1:AddPart ** calling parts->delOperation\n");
#endif                        
                        editable_part->track()->parts()->delOperation(editable_part, pendingOperations);
                        updateFlags |= SC_PART_REMOVED;
                        break;
                    
                  case UndoOp::DeletePart:
#ifdef _UNDO_DEBUG_
                        fprintf(stderr, "Song::revertOperationGroup1:DeletePart ** calling parts->addOperation\n");
#endif                        
                        // Ensure that wave event sndfile file handles are opened.
                        // It should not be the job of the pending operations list to do this.
                        // TODO Coordinate close/open with part mute and/or track off.
                        editable_part->openAllEvents();
                        
                        editable_part->track()->parts()->addOperation(editable_part, pendingOperations);
                        updateFlags |= SC_PART_INSERTED;
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
#ifdef _UNDO_DEBUG_
                        fprintf(stderr, "Song::revertOperationGroup1:AddAudioCtrlVal\n");
#endif                        
                        //deleteEventOperation(i->nEvent, editable_part, i->doCtrls, i->doClones);
                        updateFlags |= SC_AUDIO_CONTROLLER;
                        break;

                  case UndoOp::DeleteAudioCtrlVal:
                        {
#ifdef _UNDO_DEBUG_
                          fprintf(stderr, "Song::revertOperationGroup1:DeleteAudioCtrlVal\n");
#endif                        
                          //addEventOperation(i->nEvent, editable_part, i->doCtrls, i->doClones);
                          updateFlags |= SC_AUDIO_CONTROLLER;
                        }
                        break;
                        
                  case UndoOp::ModifyAudioCtrlVal:
#ifdef _UNDO_DEBUG_
                        fprintf(stderr, "Song::revertOperationGroup1:ModifyAudioCtrlVal\n");
#endif                        
                        //changeEventOperation(i->nEvent, i->oEvent, editable_part, i->doCtrls, i->doClones);
                        updateFlags |= SC_AUDIO_CONTROLLER;
                        break;
                        
                  case UndoOp::ModifyAudioCtrlValList:
                  {
#ifdef _UNDO_DEBUG_
                        fprintf(stderr, "Song::revertOperationGroup1:ModifyAudioCtrlValList\n");
#endif                        
                        // Take either id. At least one list must be valid.
                        const int id = i->_eraseCtrlList ? i->_eraseCtrlList->id() : i->_addCtrlList->id();
                        iCtrlList icl = i->_ctrlListList->find(id);
                        if(icl != i->_ctrlListList->end())
                        {
                          // Make a complete copy of the controller list. The list will be quickly switched in the realtime stage.
                          // The Pending Operations system will take 'ownership' of this and delete it at the appropriate time.
                          CtrlList* new_list = new CtrlList(*icl->second, CtrlList::ASSIGN_PROPERTIES | CtrlList::ASSIGN_VALUES);
                          
                          // Erase any items in the add list that were added...
                          //if(i->_addCtrlList)
                          if(i->_addCtrlList && !i->_addCtrlList->empty())
                          {
                            //const std::size_t sz = i->_addCtrlList->size();
                            //if(sz != 0)
                            //{
                              //const CtrlList& cl_r = *i->_addCtrlList;
                              // Both of these should be valid.
                              //ciCtrl n_s = new_list->find(cl_r[0].frame);      // The first item to be erased.
                              //ciCtrl n_e = new_list->find(cl_r[sz - 1].frame); // The last item to be erased.
                              iCtrl n_s = new_list->find(i->_addCtrlList->begin()->second.frame); // The first item to be erased.
                              ciCtrl e_e = i->_addCtrlList->end();
                              --e_e;
                              //ciCtrl n_e = new_list->find((--i->_eraseCtrlList->end())->second.frame); // The last item to be erased.
                              iCtrl n_e = new_list->find(e_e->second.frame); // The last item to be erased.
                              if(n_s != new_list->end() && n_e != new_list->end())
                              {
                                // Since std range does NOT include the last iterator, increment n_e so erase will get all items.
                                ++n_e;
                                new_list->erase(n_s, n_e);
                              }
                            //}
                          }
                          
                          // Re-add any items in the erase list that were erased...
                          if(i->_eraseCtrlList && !i->_eraseCtrlList->empty())
                            new_list->insert(i->_eraseCtrlList->begin(), i->_eraseCtrlList->end());
                          
                          // The operation will quickly switch the list in the RT stage then the delete the old list in the non-RT stage.
                          pendingOperations.add(PendingOperationItem(icl, new_list, PendingOperationItem::ModifyAudioCtrlValList));
                          updateFlags |= SC_AUDIO_CONTROLLER_LIST;
                        }
                  }
                  break;
                        
                        
                  case UndoOp::DeleteTempo:
#ifdef _UNDO_DEBUG_
                        fprintf(stderr, "Song::revertOperationGroup1:DeleteTempo ** calling tempomap.addOperation tick:%d tempo:%d\n", i->a, i->b);
#endif                        
                        MusEGlobal::tempomap.addOperation(i->a, i->b, pendingOperations);
                        updateFlags |= SC_TEMPO;
                        break;
                        
                  case UndoOp::AddTempo:
#ifdef _UNDO_DEBUG_
                        fprintf(stderr, "Song::revertOperationGroup1:AddTempo ** calling tempomap.delOperation tick:%d\n", i->a);
#endif                        
                        MusEGlobal::tempomap.delOperation(i->a, pendingOperations);
                        updateFlags |= SC_TEMPO;
                        break;
                        
                  case UndoOp::ModifyTempo:
#ifdef _UNDO_DEBUG_
                        fprintf(stderr, "Song::revertOperationGroup1:ModifyTempo ** calling tempomap.addOperation tick:%d tempo:%d\n", i->a, i->b);
#endif                        
                        MusEGlobal::tempomap.addOperation(i->a, i->b, pendingOperations);
                        updateFlags |= SC_TEMPO;
                        break;
                        
                  case UndoOp::SetGlobalTempo:
#ifdef _UNDO_DEBUG_
                        fprintf(stderr, "Song::revertOperationGroup1:SetGlobalTempo ** adding SetGlobalTempo operation\n");
#endif                        
                        pendingOperations.add(PendingOperationItem(&MusEGlobal::tempomap, i->b, PendingOperationItem::SetGlobalTempo));
                        updateFlags |= SC_TEMPO;
                        break;
                        
                  case UndoOp::DeleteSig:
#ifdef _UNDO_DEBUG_
                        fprintf(stderr, "Song::revertOperationGroup1:DeleteSig ** calling sigmap.addOperation\n");
#endif                        
                        AL::sigmap.addOperation(i->a, AL::TimeSignature(i->b, i->c), pendingOperations);
                        updateFlags |= SC_SIG;
                        break;
                        
                  case UndoOp::AddSig:
#ifdef _UNDO_DEBUG_
                        fprintf(stderr, "Song::revertOperationGroup1:AddSig ** calling sigmap.delOperation\n");
#endif                        
                        AL::sigmap.delOperation(i->a, pendingOperations);
                        updateFlags |= SC_SIG;
                        break;
                        
                  case UndoOp::ModifySig:
#ifdef _UNDO_DEBUG_
                        fprintf(stderr, "Song::revertOperationGroup1:ModifySig ** calling sigmap.addOperation\n");
#endif                        
                        AL::sigmap.addOperation(i->a, AL::TimeSignature(i->b, i->c), pendingOperations);
                        updateFlags |= SC_SIG;
                        break;
                        

                  case UndoOp::DeleteKey:
#ifdef _UNDO_DEBUG_
                        fprintf(stderr, "Song::revertOperationGroup1:DeleteKey ** calling keymap.addOperation\n");
#endif                        
                        MusEGlobal::keymap.addOperation(i->a, key_enum(i->b), pendingOperations);
                        updateFlags |= SC_KEY;
                        break;

                  case UndoOp::AddKey:
#ifdef _UNDO_DEBUG_
                        fprintf(stderr, "Song::revertOperationGroup1:AddKey ** calling keymap.delOperation\n");
#endif                        
                        MusEGlobal::keymap.delOperation(i->a, pendingOperations);
                        updateFlags |= SC_KEY;
                        break;
                        
                  case UndoOp::ModifyKey:
#ifdef _UNDO_DEBUG_
                        fprintf(stderr, "Song::revertOperationGroup1:ModifyKey ** calling keymap.addOperation\n");
#endif                        
                        MusEGlobal::keymap.addOperation(i->a, key_enum(i->b), pendingOperations);
                        updateFlags |= SC_KEY;
                        break;
                        
                  case UndoOp::ModifySongLen:
#ifdef _UNDO_DEBUG_
                        fprintf(stderr, "Song::revertOperationGroup1:ModifySongLen ** adding ModifySongLen operation\n");
#endif                        
                        pendingOperations.add(PendingOperationItem(i->b, PendingOperationItem::ModifySongLength));
                        updateFlags |= -1;  // set all flags   // TODO Refine this! Too many flags.  // REMOVE Tim.
                        //updateFlags |= SC_SONG_LEN;
                        break;
                        
                  default:
                        break;
                  }
            }
      return;
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
                  case UndoOp::ModifyMarker:
                        {
                          if (i->realMarker) {
                            Marker tmpMarker = *i->realMarker;
                            *i->realMarker = *i->copyMarker; // swap them
                            *i->copyMarker = tmpMarker;
                          }
                          else {
                            i->realMarker = _markerList->add(*i->copyMarker);
                            delete i->copyMarker;
                            i->copyMarker = 0;
                          }
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
      unsigned song_len = MusEGlobal::song->len();
        
      for (iUndoOp i = operations.begin(); i != operations.end(); ++i) {
            Track* editable_track = const_cast<Track*>(i->track);
            Track* editable_property_track = const_cast<Track*>(i->_propertyTrack);
            Part* editable_part = const_cast<Part*>(i->part);
            switch(i->type) {
                  case UndoOp::SelectPart:
                        editable_part->setSelected(i->selected);
                        updateFlags |= SC_PART_SELECTION;
                        break;
                  case UndoOp::SelectEvent:
			selectEvent(i->nEvent, editable_part, i->selected);
                        updateFlags |= SC_SELECTION;
                        break;
                        
                  case UndoOp::AddTrack:
                        switch(editable_track->type())
                        {
                          case Track::AUDIO_SOFTSYNTH:
                          {
                            SynthI* s = (SynthI*)editable_track;
                            Synth* sy = s->synth();
                            if(!s->isActivated()) 
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
                                // Register the track's jack port, or just set the name.
                                char buffer[128];
                                snprintf(buffer, 128, "%s-%d", ao->name().toLatin1().constData(), ch);
                                // REMOVE Tim. Persistent routes. Added. Don't think we need this here.
                                if(ao->jackPort(ch))
                                {
                                  MusEGlobal::audioDevice->setPortName(ao->jackPort(ch), buffer);
                                  updateFlags |= SC_ROUTE;
                                }
                                else
                                {
                                  // This should be OK since the track has not yet been added in the realtime stage.
                                  ao->setJackPort(ch, MusEGlobal::audioDevice->registerOutPort(buffer, false));
                                  updateFlags |= SC_ROUTE;
                                }
                                
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
                                // Register the track's jack port, or just set the name.
                                char buffer[128];
                                snprintf(buffer, 128, "%s-%d", ai->name().toLatin1().constData(), ch);
                                // REMOVE Tim. Persistent routes. Added. Don't think we need this here.
                                if(ai->jackPort(ch))
                                {
                                  MusEGlobal::audioDevice->setPortName(ai->jackPort(ch), buffer);
                                  updateFlags |= SC_ROUTE;
                                }
                                else
                                {
                                  // This should be OK since the track has not yet been added in the realtime stage.
                                  ai->setJackPort(ch, MusEGlobal::audioDevice->registerInPort(buffer, false));
                                  updateFlags |= SC_ROUTE;
                                }
                                
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
                          }// Fall through.
                          case Track::WAVE:
                          case Track::AUDIO_OUTPUT:
                          case Track::AUDIO_INPUT:
                          case Track::AUDIO_GROUP:
                          case Track::AUDIO_AUX:
                            ((AudioTrack*)editable_track)->deleteAllEfxGuis();
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
                          
                          default:
                          break;
                        }
                        
                        
                        
                        removeTrackOperation(editable_track, pendingOperations);
                        updateFlags |= SC_TRACK_REMOVED;
                        break;
                  case UndoOp::ModifyClip:
                        MusECore::SndFile::applyUndoFile(i->nEvent, i->tmpwavfile, i->startframe, i->endframe);
                        updateFlags |= SC_CLIP_MODIFIED;
                        break;
                  case UndoOp::ModifyTrackChannel:
                        if (editable_property_track->isMidiTrack()) 
                        {
                          MusECore::MidiTrack* mt = static_cast<MusECore::MidiTrack*>(editable_property_track);
                          if (i->_newPropValue != mt->outChannel()) 
                          {
                                MusEGlobal::audio->msgIdle(true);
                                mt->setOutChanAndUpdate(i->_newPropValue);
                                MusEGlobal::audio->msgIdle(false);
                                MusEGlobal::audio->msgUpdateSoloStates();                   
                                updateFlags |= SC_MIDI_TRACK_PROP;               
                          }
                        }
                        else
                        {
                            if(editable_property_track->type() != MusECore::Track::AUDIO_SOFTSYNTH)
                            {
                              MusECore::AudioTrack* at = static_cast<MusECore::AudioTrack*>(editable_property_track);
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
                        updateFlags |= SC_RECFLAG;
                        break;

                  case UndoOp::SetTrackMute:
                        pendingOperations.add(PendingOperationItem(editable_track, i->a, PendingOperationItem::SetTrackMute));
                        updateFlags |= SC_MUTE;
                        break;

                  case UndoOp::SetTrackSolo:
                        pendingOperations.add(PendingOperationItem(editable_track, i->a, PendingOperationItem::SetTrackSolo));
                        updateFlags |= SC_SOLO;
                        break;

                        
                  case UndoOp::AddRoute:
#ifdef _UNDO_DEBUG_
                        fprintf(stderr, "Song::executeOperationGroup1:AddRoute\n");
#endif                        
                        pendingOperations.add(PendingOperationItem(i->routeFrom, i->routeTo, PendingOperationItem::AddRoute)); 
                        updateFlags |= SC_ROUTE;
                        break;
                        
                  case UndoOp::DeleteRoute:
#ifdef _UNDO_DEBUG_
                        fprintf(stderr, "Song::executeOperationGroup1:DeleteEvent\n");
#endif                        
                        pendingOperations.add(PendingOperationItem(i->routeFrom, i->routeTo, PendingOperationItem::DeleteRoute)); 
                        updateFlags |= SC_ROUTE;
                        break;
                        
                  case UndoOp::ModifyTrackName:
                        pendingOperations.add(PendingOperationItem(editable_track, i->_newName, PendingOperationItem::ModifyTrackName));
                        updateFlags |= SC_TRACK_MODIFIED;
                        break;
                        
                  case UndoOp::MoveTrack:
                        pendingOperations.add(PendingOperationItem(&_tracks, i->a, i->b, PendingOperationItem::MoveTrack));
                        updateFlags |= SC_TRACK_MODIFIED;
                        break;
                        
                  case UndoOp::ModifyPartName:
                        pendingOperations.add(PendingOperationItem(editable_part, i->_newName, PendingOperationItem::ModifyPartName));
                        updateFlags |= SC_PART_MODIFIED;
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
                            operations.insert(i, UndoOp(UndoOp::ModifySongLen, song_len, MusEGlobal::song->len()));
                            // Since the ModifySongLen above will not be iterated now, act like the operation had just been iterated. 
                            // The same REPLACEMENT rules apply here.
                            pendingOperations.add(PendingOperationItem(song_len, PendingOperationItem::ModifySongLength));
                            updateFlags |= -1;  // set all flags   // TODO Refine this! Too many flags.  // REMOVE Tim.
                            //updateFlags |= SC_SONG_LEN;
                          }
                          removePortCtrlEvents(editable_part, editable_part->track(), pendingOperations);
                          pendingOperations.add(PendingOperationItem(editable_part, i->new_partlen_or_pos, PendingOperationItem::ModifyPartLength));
                          addPortCtrlEvents(editable_part, editable_part->posValue(), i->new_partlen_or_pos, editable_part->track(), pendingOperations);
                          updateFlags |= SC_PART_MODIFIED;
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
                            operations.insert(i, UndoOp(UndoOp::ModifySongLen, song_len, MusEGlobal::song->len()));
                            // Since the ModifySongLen above will not be iterated now, act like the operation had just been iterated. 
                            // The same REPLACEMENT rules apply here.
                            pendingOperations.add(PendingOperationItem(song_len, PendingOperationItem::ModifySongLength));
                            updateFlags |= -1;  // set all flags   // TODO Refine this! Too many flags.  // REMOVE Tim.
                            //updateFlags |= SC_SONG_LEN;
                          }
#ifdef _UNDO_DEBUG_
                          fprintf(stderr, "Song::executeOperationGroup1:MovePart ** calling parts->movePartOperation\n");
#endif                        
                          editable_part->track()->parts()->movePartOperation(editable_part, i->new_partlen_or_pos, pendingOperations, editable_track);
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
                            operations.insert(i, UndoOp(UndoOp::ModifySongLen, song_len, MusEGlobal::song->len()));
                            // Since the ModifySongLen above will not be iterated now, act like the operation had just been iterated. 
                            // The same REPLACEMENT rules apply here.
                            pendingOperations.add(PendingOperationItem(song_len, PendingOperationItem::ModifySongLength));
                            updateFlags |= -1;  // set all flags   // TODO Refine this! Too many flags.  // REMOVE Tim.
                            //updateFlags |= SC_SONG_LEN;
                          }
#ifdef _UNDO_DEBUG_
                          fprintf(stderr, "Song::executeOperationGroup1:addPart ** calling parts->addOperation\n");
#endif                        
                          // Ensure that wave event sndfile file handles are opened.
                          // It should not be the job of the pending operations list to do this.
                          // TODO Coordinate close/open with part mute and/or track off.
                          editable_part->openAllEvents();
                          
                          editable_part->track()->parts()->addOperation(editable_part, pendingOperations);
                          updateFlags |= SC_PART_INSERTED;
                        }
                        break;
                    
                  case UndoOp::DeletePart:
#ifdef _UNDO_DEBUG_
                        fprintf(stderr, "Song::executeOperationGroup1:deletePart ** calling parts->delOperation\n");
#endif                        
                        editable_part->track()->parts()->delOperation(editable_part, pendingOperations);
                        updateFlags |= SC_PART_REMOVED;
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
                        
                  case UndoOp::DeleteEvent:
#ifdef _UNDO_DEBUG_
                        fprintf(stderr, "Song::executeOperationGroup1:DeleteEvent ** calling deleteEvent\n");
#endif                        
                        deleteEventOperation(i->nEvent, editable_part, i->doCtrls, i->doClones);
                        updateFlags |= SC_EVENT_REMOVED;
                        break;
                        
                  case UndoOp::ModifyEvent:
#ifdef _UNDO_DEBUG_
                        fprintf(stderr, "Song::executeOperationGroup1:ModifyEvent ** calling changeEvent\n");
#endif                        
                        changeEventOperation(i->oEvent, i->nEvent, editable_part, i->doCtrls, i->doClones);
                        updateFlags |= SC_EVENT_MODIFIED;
                        break;

                        
                  case UndoOp::AddAudioCtrlVal:
#ifdef _UNDO_DEBUG_
                        fprintf(stderr, "Song::executeOperationGroup1:AddAudioCtrlVal\n");
#endif                        
                        //addEventOperation(i->nEvent, editable_part, i->doCtrls, i->doClones);
                        updateFlags |= SC_AUDIO_CONTROLLER;
                        break;

                  case UndoOp::DeleteAudioCtrlVal:
                        {
#ifdef _UNDO_DEBUG_
                          fprintf(stderr, "Song::executeOperationGroup1:DeleteAudioCtrlVal\n");
#endif                        
                          //deleteEventOperation(i->nEvent, editable_part, i->doCtrls, i->doClones);
                          updateFlags |= SC_AUDIO_CONTROLLER;
                        }
                        break;
                        
                  case UndoOp::ModifyAudioCtrlVal:
#ifdef _UNDO_DEBUG_
                        fprintf(stderr, "Song::executeOperationGroup1:ModifyAudioCtrlVal\n");
#endif                        
                        //changeEventOperation(i->oEvent, i->nEvent, editable_part, i->doCtrls, i->doClones);
                        updateFlags |= SC_AUDIO_CONTROLLER;
                        break;
                        
                  case UndoOp::ModifyAudioCtrlValList:
                  {
#ifdef _UNDO_DEBUG_
                        fprintf(stderr, "Song::executeOperationGroup1:ModifyAudioCtrlValList\n");
#endif                        
                        // Take either id. At least one list must be valid.
                        const int id = i->_eraseCtrlList ? i->_eraseCtrlList->id() : i->_addCtrlList->id();
                        iCtrlList icl = i->_ctrlListList->find(id);
                        if(icl != i->_ctrlListList->end())
                        {
                          // Make a complete copy of the controller list. The list will be quickly switched in the realtime stage.
                          // The Pending Operations system will take 'ownership' of this and delete it at the appropriate time.
                          CtrlList* new_list = new CtrlList(*icl->second, CtrlList::ASSIGN_PROPERTIES | CtrlList::ASSIGN_VALUES);
                          
                          // Erase any items in the erase list...
                          //if(i->_eraseCtrlList)
                          if(i->_eraseCtrlList && !i->_eraseCtrlList->empty())
                          {
                            //const std::size_t sz = i->_eraseCtrlList->size();
                            //if(sz != 0)
                            //{
                              //const CtrlList& cl_r = *i->_eraseCtrlList;
                              // Both of these should be valid.
                              //ciCtrl n_s = new_list->find(cl_r[0].frame);      // The first item to be erased.
                              //ciCtrl n_e = new_list->find(cl_r[sz - 1].frame); // The last item to be erased.
                              iCtrl n_s = new_list->find(i->_eraseCtrlList->begin()->second.frame); // The first item to be erased.
                              ciCtrl e_e = i->_eraseCtrlList->end();
                              --e_e;
                              //ciCtrl n_e = new_list->find((--i->_eraseCtrlList->end())->second.frame); // The last item to be erased.
                              iCtrl n_e = new_list->find(e_e->second.frame); // The last item to be erased.
                              if(n_s != new_list->end() && n_e != new_list->end())
                              {
                                // Since std range does NOT include the last iterator, increment n_e so erase will get all items.
                                ++n_e;
                                new_list->erase(n_s, n_e);
                              }
                            //}
                          }
                          
                          // Add any items in the add list...
                          if(i->_addCtrlList && !i->_addCtrlList->empty())
                            new_list->insert(i->_addCtrlList->begin(), i->_addCtrlList->end());
                            //new_list->insert(const_cast<CtrlList*>(i->_addCtrlList)->begin(), const_cast<CtrlList*>(i->_addCtrlList)->end());
                          
                          // The operation will quickly switch the list in the RT stage then the delete the old list in the non-RT stage.
                          pendingOperations.add(PendingOperationItem(icl, new_list, PendingOperationItem::ModifyAudioCtrlValList));
                          updateFlags |= SC_AUDIO_CONTROLLER_LIST;
                        }
                  }
                  break;
                        
                        
                  case UndoOp::AddTempo:
#ifdef _UNDO_DEBUG_
                        fprintf(stderr, "Song::executeOperationGroup1:AddTempo ** calling tempomap.addOperation tick:%d tempo:%d\n", i->a, i->b);
#endif                        
                        MusEGlobal::tempomap.addOperation(i->a, i->b, pendingOperations);
                        updateFlags |= SC_TEMPO;
                        break;
                        
                  case UndoOp::DeleteTempo:
#ifdef _UNDO_DEBUG_
                        fprintf(stderr, "Song::executeOperationGroup1:DeleteTempo ** calling tempomap.delOperation tick:%d\n", i->a);
#endif                        
                        MusEGlobal::tempomap.delOperation(i->a, pendingOperations);
                        updateFlags |= SC_TEMPO;
                        break;
                        
                  case UndoOp::ModifyTempo:
#ifdef _UNDO_DEBUG_
                        fprintf(stderr, "Song::executeOperationGroup1:ModifyTempo ** calling tempomap.addOperation tick:%d tempo:%d\n", i->a, i->c);
#endif                        
                        MusEGlobal::tempomap.addOperation(i->a, i->c, pendingOperations);
                        updateFlags |= SC_TEMPO;
                        break;

                  case UndoOp::SetGlobalTempo:
#ifdef _UNDO_DEBUG_
                        fprintf(stderr, "Song::executeOperationGroup1:SetGlobalTempo ** adding SetGlobalTempo operation\n");
#endif                        
                        pendingOperations.add(PendingOperationItem(&MusEGlobal::tempomap, i->a, PendingOperationItem::SetGlobalTempo));
                        updateFlags |= SC_TEMPO;
                        break;
                        
                  case UndoOp::AddSig:
#ifdef _UNDO_DEBUG_
                        fprintf(stderr, "Song::executeOperationGroup1:AddSig ** calling sigmap.addOperation\n");
#endif                        
                        AL::sigmap.addOperation(i->a, AL::TimeSignature(i->b, i->c), pendingOperations);
                        updateFlags |= SC_SIG;
                        break;
                        
                  case UndoOp::DeleteSig:
#ifdef _UNDO_DEBUG_
                        fprintf(stderr, "Song::executeOperationGroup1:DeleteSig ** calling sigmap.delOperation\n");
#endif                        
                        AL::sigmap.delOperation(i->a, pendingOperations);
                        updateFlags |= SC_SIG;
                        break;
                        
                  case UndoOp::ModifySig:
#ifdef _UNDO_DEBUG_
                        fprintf(stderr, "Song::executeOperationGroup1:ModifySig ** calling sigmap.addOperation\n");
#endif                        
                        AL::sigmap.addOperation(i->a, AL::TimeSignature(i->d, i->e), pendingOperations);
                        updateFlags |= SC_SIG;
                        break;

                        
                  case UndoOp::AddKey:
#ifdef _UNDO_DEBUG_
                        fprintf(stderr, "Song::executeOperationGroup1:AddKey ** calling keymap.addOperation\n");
#endif                        
                        MusEGlobal::keymap.addOperation(i->a, key_enum(i->b), pendingOperations);
                        updateFlags |= SC_KEY;
                        break;
                        
                  case UndoOp::DeleteKey:
#ifdef _UNDO_DEBUG_
                        fprintf(stderr, "Song::executeOperationGroup1:DeleteKey ** calling keymap.delOperation\n");
#endif                        
                        MusEGlobal::keymap.delOperation(i->a, pendingOperations);
                        updateFlags |= SC_KEY;
                        break;
                        
                  case UndoOp::ModifyKey:
#ifdef _UNDO_DEBUG_
                        fprintf(stderr, "Song::executeOperationGroup1:ModifyKey ** calling keymap.addOperation\n");
#endif                        
                        MusEGlobal::keymap.addOperation(i->a, key_enum(i->c), pendingOperations);
                        updateFlags |= SC_KEY;
                        break;

                  case UndoOp::ModifySongLen:
#ifdef _UNDO_DEBUG_
                        fprintf(stderr, "Song::executeOperationGroup1:ModifySongLen ** adding ModifySongLen operation\n");
#endif                        
                        pendingOperations.add(PendingOperationItem(i->a, PendingOperationItem::ModifySongLength));
                        updateFlags |= -1;  // set all flags   // TODO Refine this! Too many flags.  // REMOVE Tim.
                        //updateFlags |= SC_SONG_LEN;
                        break;
                        
                  case UndoOp::EnableAllAudioControllers:
#ifdef _UNDO_DEBUG_
                        fprintf(stderr, "Song::executeOperationGroup1:EnableAllAudioControllers\n");
#endif                        
                        pendingOperations.add(PendingOperationItem(PendingOperationItem::EnableAllAudioControllers));
                        updateFlags |= SC_AUDIO_CONTROLLER;
                        break;
                        
                  default:
                        break;
                  }
            }
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
                  case UndoOp::ModifyMarker:
                        {
                          if (i->copyMarker) {
                            Marker tmpMarker = *i->realMarker;
                            *i->realMarker = *i->copyMarker; // swap them
                            *i->copyMarker = tmpMarker;
                          } else {
                            i->copyMarker = new Marker(*i->realMarker);
                            _markerList->remove(i->realMarker);
                            i->realMarker = 0;
                          }
                        }
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
                   default:
                        break;
                  }
            
            // Is the operation marked as non-undoable? Remove it from the list.
            if(i->_noUndo)
              i = operations.erase(i);
            else
              ++i;
            }
            
      // If some operations marked as non-undoable were removed, it is OK,
      //  because we only want dirty if an undoable operation was executed, right?
      if(!operations.empty())
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
