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

//#include "sig.h"
#include "al/sig.h"  
#include "keyevent.h"

#include "undo.h"
#include "song.h"
#include "globals.h"
#include "audio.h"  

#include <string.h>
#include <QAction>
#include <set>

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
            "AddTrack", "DeleteTrack", 
            "AddPart",  "DeletePart",  "ModifyPartTick", "ModifyPartLength", "ModifyPartLengthFrames", "ModifyPartName", "SelectPart",
            "MovePartToTrack",
            "AddEvent", "DeleteEvent", "ModifyEvent", "SelectEvent",
            "AddTempo", "DeleteTempo",
            "AddSig", "DeleteSig",
            "AddKey", "DeleteKey",
            "ModifyTrackName", "ModifyTrackChannel",
            "MoveTrack",
            "ModifyClip", "ModifyMarker",
            "ModifySongLen", "DoNothing"
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
            case AddPart:
            case DeletePart:
            case ModifyPartTick:
            case ModifyPartLength:
            case ModifyPartLengthFrames:
            case ModifyPartName:
            case MovePartToTrack:
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
                  printf("<%s>-<%s>\n", _oldName, _newName);
                  break;
            case ModifyTrackChannel:
                  printf("<%d>-<%d>\n", _oldPropValue, _newPropValue);
                  break;
            case ModifyEvent:
            case AddTempo:
            case DeleteTempo:
            case AddSig:
            case MoveTrack:
            case DeleteSig:
            case ModifyClip:
            case ModifyMarker:
            case AddKey:
            case DeleteKey:
            case ModifySongLen:
            case DoNothing:
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
                  
            case UndoOp::ModifyTrackName:
                  if (i->_oldName)
                    delete [] i->_oldName;
                  if (i->_newName)
                    delete [] i->_newName;
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
                  
            case UndoOp::ModifyTrackName:
                  if (i->_oldName)
                    delete [] i->_oldName;
                  if (i->_newName)
                    delete [] i->_newName;
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
      riUndo prev_undo = undoList->rbegin();
      prev_undo++;
      if (prev_undo!=undoList->rend())
      {
            // try to merge the current Undo with the last one
            if (prev_undo->merge_combo(undoList->back()))
                  undoList->pop_back();
      }
      
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

void prepareOperationGroup(Undo& group)
{
	QSet<const Track*> deleted_tracks;
	QSet<const Part*> deleted_parts;

	int songlen = MusEGlobal::song->len();
	

	for (iUndoOp op=group.begin(); op!=group.end();)
	{
		iUndoOp op_=op;
		op_++;
		
		if (op->type==UndoOp::DeleteTrack)
		{
			if (deleted_tracks.contains(op->track))
			{
				group.erase(op);
			}
			else
			{
				const PartList* pl = op->track->cparts();
				for (ciPart part = pl->begin(); part != pl->end(); part++)
					if (!deleted_parts.contains(part->second))
					{
						// they will be inserted between op and op_
						// because we set op=op_ below, the inserted
						// elements will be skipped.
						group.insert(op, UndoOp(UndoOp::DeletePart, part->second));
						deleted_parts.insert(part->second);
					}
					
				deleted_tracks.insert(op->track);
			}
		}
		else if (op->type==UndoOp::DeletePart)
		{
			if (deleted_parts.contains(op->part))
				group.erase(op);
			else
				deleted_parts.insert(op->part);
		}
		else if (op->type==UndoOp::AddPart || op->type==UndoOp::ModifyPartLength ||
		         op->type==UndoOp::ModifyPartTick)
		{
			if (op->type==UndoOp::AddPart)
			{
				if (songlen < op->part->endTick())
					songlen = op->part->endTick();
			}
			else if (op->type==UndoOp::ModifyPartTick)
			{
				if (songlen < op->new_partlen_or_tick+op->part->lenTick())
					songlen = op->new_partlen_or_tick+op->part->lenTick();
			}
			else if (op->type==UndoOp::ModifyPartLength)
			{
				if (songlen < op->part->tick()+op->new_partlen_or_tick)
					songlen=op->part->tick()+op->new_partlen_or_tick;
			}
		}
		
		op=op_;
	}
	
	if (songlen >= MusEGlobal::song->len())
		group.push_back(UndoOp(UndoOp::ModifySongLen, songlen, MusEGlobal::song->len()));
	
	// replicate Event modifications to keep clones up to date.
	// do not replicate SelectEvent because... umm, it just doesn't feel right.
	for (iUndoOp op=group.begin(); op!=group.end(); op++)
	{
		if (op->type==UndoOp::AddEvent || op->type==UndoOp::DeleteEvent || op->type==UndoOp::ModifyEvent)
		{
			for (const Part* it = op->part->nextClone(); it!=op->part; it=it->nextClone())
			{
				UndoOp newop;
				if (op->type==UndoOp::AddEvent) // we need to clone the event
					newop = UndoOp(UndoOp::AddEvent, op->nEvent.clone(), it, op->doCtrls, op->doClones);
				else if (op->type==UndoOp::DeleteEvent)
					newop = UndoOp(UndoOp::DeleteEvent, it->events().findSimilar(op->nEvent)->second, it, op->doCtrls, op->doClones);
				else if (op->type==UndoOp::ModifyEvent)
					newop = UndoOp(UndoOp::ModifyEvent, op->nEvent.clone(), it->events().findSimilar(op->oEvent)->second, it, op->doCtrls, op->doClones);
				
				group.insert(op, newop);
			}
		}
	}
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
            prepareOperationGroup(group);
            
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

void Song::revertOperationGroup2(Undo& operations)
      {
      for (riUndoOp i = operations.rbegin(); i != operations.rend(); ++i) {
            Track* editable_track = const_cast<Track*>(i->track);
// uncomment if needed            Track* editable_property_track = const_cast<Track*>(i->_propertyTrack);
            Part* editable_part = const_cast<Part*>(i->part);
            switch(i->type) {
                  case UndoOp::AddTrack:
                        removeTrack2(editable_track);
                        updateFlags |= SC_TRACK_REMOVED;
                        break;
                  case UndoOp::DeleteTrack: // FINDMICHJETZT FIXME TODO: DeletePart on all parts, only empty tracks may be deleted! this removes the necessarity of un...
                        insertTrack2(editable_track, i->trackno);
                        chainTrackParts(editable_track);
                        
                        updateFlags |= SC_TRACK_INSERTED;
                        break;
                        
                  case UndoOp::MoveTrack:
                        {
                        moveTrack(i->b, i->a);
                        if (i->b > i->a)
                          i->a++;
                        else
                          i->b--;

                        updateFlags |= SC_TRACK_MODIFIED;
                        }
                        break;
                  case UndoOp::AddPart:
                        removePart(editable_part);
                        updateFlags |= SC_PART_REMOVED;
                        editable_part->unchainClone();
                        break;
                  case UndoOp::DeletePart:
                        addPart(editable_part);
                        updateFlags |= SC_PART_INSERTED;
                        editable_part->rechainClone();
                        break;
                  case UndoOp::MovePartToTrack:
                        removePart(editable_part); // remove it from the track it currently lives on. (does not touch the clone chain)
                        editable_part->setTrack(const_cast<Track*>(i->oldTrack));
                        addPart(editable_part); // add it to the track we just set.
                        updateFlags |= SC_PART_INSERTED | SC_PART_REMOVED;
                        break;
                  case UndoOp::ModifyPartName:
                        editable_part->setName(i->_oldName);
                        updateFlags |= SC_PART_MODIFIED;
                        break;
                  case UndoOp::ModifyPartTick: // TODO FIXME (?) do port ctrls/clones?
                        editable_part->setTick(i->old_partlen_or_tick);
                        updateFlags |= SC_PART_MODIFIED;
                        break;
                  case UndoOp::ModifyPartLength: // TODO FIXME (?) do port ctrls/clones?
                        editable_part->setLenTick(i->old_partlen_or_tick);
                        updateFlags |= SC_PART_MODIFIED;
                        break;
                  case UndoOp::ModifyPartLengthFrames: // TODO FIXME  FINDMICH frames deprecated!   do port ctrls/clones?
                        editable_part->setLenFrame(i->old_partlen_or_tick);
                        updateFlags |= SC_PART_MODIFIED;
                        break;
                  case UndoOp::AddEvent:
                        if(i->doCtrls)
                          removePortCtrlEvents(i->nEvent, editable_part, i->doClones);
                        deleteEvent(i->nEvent, editable_part);
                        updateFlags |= SC_EVENT_REMOVED;
                        break;
                  case UndoOp::DeleteEvent:
                        addEvent(i->nEvent, editable_part);
                        if(i->doCtrls)
                          addPortCtrlEvents(i->nEvent, editable_part, i->doClones);
                        updateFlags |= SC_EVENT_INSERTED;
                        break;
                  case UndoOp::ModifyEvent:
                        if(i->doCtrls)
                          removePortCtrlEvents(i->nEvent, editable_part, i->doClones);
                        changeEvent(i->nEvent, i->oEvent, editable_part);
                        if(i->doCtrls)
                          addPortCtrlEvents(i->oEvent, editable_part, i->doClones);
                        updateFlags |= SC_EVENT_MODIFIED;
                        break;
                  case UndoOp::AddTempo:
                        MusEGlobal::tempomap.delTempo(i->a);
                        updateFlags |= SC_TEMPO;
                        break;
                  case UndoOp::DeleteTempo:
                        MusEGlobal::tempomap.addTempo(i->a, i->b);
                        updateFlags |= SC_TEMPO;
                        break;
                  case UndoOp::AddSig:
                        AL::sigmap.del(i->a);
                        updateFlags |= SC_SIG;
                        break;
                  case UndoOp::DeleteSig:
                        AL::sigmap.add(i->a, AL::TimeSignature(i->b, i->c));
                        updateFlags |= SC_SIG;
                        break;
                  case UndoOp::AddKey:
                        ///sigmap.del(i->a);
                        MusEGlobal::keymap.delKey(i->a);
                        updateFlags |= SC_KEY;
                        break;
                  case UndoOp::DeleteKey:
                        ///sigmap.add(i->a, i->b, i->c);
                        MusEGlobal::keymap.addKey(i->a, (key_enum)i->b);
                        updateFlags |= SC_KEY;
                        break;
                  case UndoOp::ModifySongLen:
                        _len=i->b;
                        updateFlags = -1; // set all flags     // TODO Refine this! Too many flags.  // REMOVE Tim.
                        break;
                  case UndoOp::ModifyClip:
                  case UndoOp::ModifyMarker:
                  case UndoOp::DoNothing:
                        break;
                  default:      
                        break;
                  }
            }
      }

//---------------------------------------------------------
//   Song::executeOperationGroup2
//---------------------------------------------------------

void Song::executeOperationGroup2(Undo& operations)
      {
      for (iUndoOp i = operations.begin(); i != operations.end(); ++i) {
            Track* editable_track = const_cast<Track*>(i->track);
// uncomment if needed            Track* editable_property_track = const_cast<Track*>(i->_propertyTrack);
            Part* editable_part = const_cast<Part*>(i->part);
            switch(i->type) {
                  case UndoOp::AddTrack:
                        insertTrack2(editable_track, i->trackno);
                        chainTrackParts(editable_track);
                        
                        updateFlags |= SC_TRACK_INSERTED;
                        break;
                  case UndoOp::DeleteTrack:
                        removeTrack2(editable_track);
                        updateFlags |= SC_TRACK_REMOVED;
                        break;
                        
                        
                  case UndoOp::MoveTrack:
                        {
                        moveTrack(i->a, i->b);
                        if (i->a > i->b){
                          i->b++;
                        }
                        else
                          i->a--;
                        updateFlags |= SC_TRACK_MODIFIED;
                        }
                        break;
                  case UndoOp::AddPart:
                        addPart(editable_part);
                        updateFlags |= SC_PART_INSERTED;
                        editable_part->rechainClone();
                        break;
                  case UndoOp::DeletePart:
                        removePart(editable_part);
                        updateFlags |= SC_PART_REMOVED;
                        editable_part->unchainClone();
                        break;
                  case UndoOp::MovePartToTrack:
                        removePart(editable_part); // remove it from the track it currently lives on. (does not touch the clone chain)
                        editable_part->setTrack(editable_track);
                        addPart(editable_part); // add it to the track we just set.
                        updateFlags |= SC_PART_INSERTED | SC_PART_REMOVED;
                        break;
                  case UndoOp::ModifyPartName:
                        editable_part->setName(i->_newName);
                        updateFlags |= SC_PART_MODIFIED;
                        break;
                  case UndoOp::ModifyPartTick: // TODO FIXME (?) do port ctrls/clones?
                        editable_part->setTick(i->new_partlen_or_tick);
                        updateFlags |= SC_PART_MODIFIED;
                        break;
                  case UndoOp::ModifyPartLength: // TODO FIXME (?) do port ctrls/clones?
                        editable_part->setLenTick(i->new_partlen_or_tick);
                        updateFlags |= SC_PART_MODIFIED;
                        break;
                  case UndoOp::ModifyPartLengthFrames: // TODO FIXME  FINDMICH frames deprecated!   do port ctrls/clones?
                        editable_part->setLenFrame(i->new_partlen_or_tick);
                        updateFlags |= SC_PART_MODIFIED;
                        break;
                  case UndoOp::AddEvent:
                        addEvent(i->nEvent, editable_part);
                        if(i->doCtrls)
                          addPortCtrlEvents(i->nEvent, editable_part, i->doClones);
                        updateFlags |= SC_EVENT_INSERTED;
                        break;
                  case UndoOp::DeleteEvent:
                        if(i->doCtrls)
                          removePortCtrlEvents(i->nEvent, editable_part, i->doClones);
                        deleteEvent(i->nEvent, editable_part);
                        updateFlags |= SC_EVENT_REMOVED;
                        break;
                  case UndoOp::ModifyEvent:
                        if(i->doCtrls)
                          removePortCtrlEvents(i->oEvent, editable_part, i->doClones);
                        changeEvent(i->oEvent, i->nEvent, editable_part);
                        if(i->doCtrls)
                          addPortCtrlEvents(i->nEvent, editable_part, i->doClones);
                        updateFlags |= SC_EVENT_MODIFIED;
                        break;
                  case UndoOp::AddTempo:
                        MusEGlobal::tempomap.addTempo(i->a, i->b);
                        updateFlags |= SC_TEMPO;
                        break;
                  case UndoOp::DeleteTempo:
                        MusEGlobal::tempomap.delTempo(i->a);
                        updateFlags |= SC_TEMPO;
                        break;
                  case UndoOp::AddSig:
                        AL::sigmap.add(i->a, AL::TimeSignature(i->b, i->c));
                        updateFlags |= SC_SIG;
                        break;
                  case UndoOp::DeleteSig:
                        AL::sigmap.del(i->a);
                        updateFlags |= SC_SIG;
                        break;
                  case UndoOp::AddKey:
                        MusEGlobal::keymap.addKey(i->a, (key_enum)i->b);
                        updateFlags |= SC_KEY;
                        break;
                  case UndoOp::DeleteKey:
                        MusEGlobal::keymap.delKey(i->a);
                        updateFlags |= SC_KEY;
                        break;
                  case UndoOp::ModifySongLen:
                        _len=i->a;
                        updateFlags = -1; // set all flags   // TODO Refine this! Too many flags.  // REMOVE Tim.
                        break;
                  case UndoOp::ModifyClip:
                  case UndoOp::ModifyMarker:
                  case UndoOp::DoNothing:
                        break;
                  default:      
                        break;
                  }
            }
      }

UndoOp::UndoOp()
{
  type=UndoOp::DoNothing;
}

UndoOp::UndoOp(UndoType type_, int a_, int b_, int c_)
      {
      assert(type_==AddKey || type_==DeleteKey ||
             type_==AddTempo || type_==DeleteTempo ||
             type_==AddSig || type_==DeleteSig ||
             type_==ModifySongLen || type_==MoveTrack);
      
      type = type_;
      a  = a_;
      b  = b_;
      c  = c_;
      }


UndoOp::UndoOp(UndoType type_, int n, const Track* track_)
      {
      assert(type_==AddTrack || type_==DeleteTrack);
      assert(track_);
      
      type    = type_;
      trackno = n;
      track  = track_;
      }

UndoOp::UndoOp(UndoType type_, const Part* part_, unsigned old_len_or_tick, unsigned new_len_or_tick, bool, bool)
      {
      assert(type_==AddPart || type_==DeletePart || type_==ModifyPartLength || type_==ModifyPartLengthFrames || type_==ModifyPartTick );
      assert(part_);
      
      type  = type_;
      part = part_;
      old_partlen_or_tick=old_len_or_tick;
      new_partlen_or_tick=new_len_or_tick;
      }
      
UndoOp::UndoOp(UndoType type_, const Part* part_, bool selected_, bool sel_old_)
{
    assert(type_==SelectPart);
    assert(part_);
    
    type=type_;
    part = part_;
    selected=selected_;
    selected_old=sel_old_;
}

UndoOp::UndoOp(UndoType type_, const Part* part_, const Track* nTrack, const Track* oTrack)
{
    assert(type_==MovePartToTrack);
    assert(part_);
    assert(nTrack);
    assert(oTrack);
    
    type=type_;
    part = part_;
    track=nTrack;
    oldTrack=oTrack;
}

UndoOp::UndoOp(UndoType type_, const Event& nev, const Event& oev, const Part* part_, bool doCtrls_, bool doClones_)
      {
      assert(type_==ModifyEvent);
      assert(part_);
      
      type   = type_;
      nEvent = nev;
      oEvent = oev;
      part   = part_;
      doCtrls = doCtrls_;
      doClones = doClones_;
      }

UndoOp::UndoOp(UndoType type_, const Event& nev, const Part* part_, bool doCtrls_, bool doClones_)
      {
      assert(type_==DeleteEvent || type_==AddEvent);
      assert(part_);
      
      type   = type_;
      nEvent = nev;
      part   = part_;
      doCtrls = doCtrls_;
      doClones = doClones_;
      }
      
UndoOp::UndoOp(UndoType type_, const Event& nev, bool selected_, bool sel_old_)
{
    assert(type_==SelectEvent);
    
    type=type_;
    nEvent = nev;
    selected=selected_;
    selected_old=sel_old_;
}


UndoOp::UndoOp(UndoType type_, Marker* copyMarker_, Marker* realMarker_)
      {
      assert(type_==ModifyMarker);
      assert(copyMarker_);
      assert(realMarker_);
      
      type    = type_;
      realMarker  = realMarker_;
      copyMarker  = copyMarker_;
      }

UndoOp::UndoOp(UndoType type_, const char* changedFile, const char* changeData, int startframe_, int endframe_)
      {
      assert(type_==ModifyClip);
      
      type = type_;
      filename   = changedFile;
      tmpwavfile = changeData;
      startframe = startframe_;
      endframe   = endframe_;
      }

UndoOp::UndoOp(UndoOp::UndoType type_, const Part* part_, const char* old_name, const char* new_name)
{
    assert(type_==ModifyPartName);
    assert(part_);
    assert(old_name);
    assert(new_name);
    
    type=type_;
    part=part_;
    _oldName = new char[strlen(old_name) + 1];
    _newName = new char[strlen(new_name) + 1];
    strcpy(_oldName, old_name);
    strcpy(_newName, new_name);
}

UndoOp::UndoOp(UndoOp::UndoType type_, const Track* track_, const char* old_name, const char* new_name)
{
  assert(type_==ModifyTrackName);
  assert(track_);
  assert(old_name);
  assert(new_name);    
    
  type = type_;
  track = track_;
  _oldName = new char[strlen(old_name) + 1];
  _newName = new char[strlen(new_name) + 1];
  strcpy(_oldName, old_name);
  strcpy(_newName, new_name);
}

UndoOp::UndoOp(UndoOp::UndoType type_, const Track* track_, int old_chan, int new_chan)
{
  assert(type_==ModifyTrackChannel);
  assert(track_);
  
  type = type_;
  _propertyTrack = track_;
  _oldPropValue = old_chan;
  _newPropValue = new_chan;
}

void Song::undoOp(UndoOp::UndoType type, const char* changedFile, const char* changeData, int startframe, int endframe)
      {
      addUndo(UndoOp(type,changedFile,changeData,startframe,endframe));
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
                        updateFlags |= SC_SELECTION;
                        break;
                  case UndoOp::SelectEvent:
                        i->nEvent.setSelected(i->selected_old);
                        updateFlags |= SC_SELECTION;
                        break;
                        
                  case UndoOp::AddTrack:
                        removeTrack1(editable_track);
                        break;
                  case UndoOp::DeleteTrack:
                        insertTrack1(editable_track, i->trackno);

                        // FIXME: Would like to put this part in Undo2, but indications
                        //  elsewhere are that (dis)connecting jack routes must not be
                        //  done in the realtime thread. The result is that we get a few
                        //  "PANIC Process init: No buffer from audio device" messages
                        //  before the routes are (dis)connected. So far seems to do no harm though...
                        switch(editable_track->type())
                        {
                              case Track::AUDIO_OUTPUT:
                              case Track::AUDIO_INPUT:
                                      connectJackRoutes((AudioTrack*)editable_track, false);
                                    break;
                              //case Track::AUDIO_SOFTSYNTH: DELETETHIS 4
                                      //SynthI* si = (SynthI*)editable_track;
                                      //si->synth()->init(
                              //      break;
                              default:
                                    break;
                        }

                        break;
                  case UndoOp::ModifyTrackName:
                          editable_track->setName(i->_oldName);
                          updateFlags |= SC_TRACK_MODIFIED;
                        break;
                  case UndoOp::ModifyClip:
                        MusECore::SndFile::applyUndoFile(i->filename, i->tmpwavfile, i->startframe, i->endframe);
                        break;
                  case UndoOp::ModifyTrackChannel:
                        if (editable_property_track->isMidiTrack()) 
                        {
                          MusECore::MidiTrack* mt = dynamic_cast<MusECore::MidiTrack*>(editable_property_track);
                          if (mt == 0 || mt->type() == MusECore::Track::DRUM)
                            break;
                          if (i->_oldPropValue != mt->outChannel()) 
                          {
                                MusEGlobal::audio->msgIdle(true);
                                mt->setOutChanAndUpdate(i->_oldPropValue);
                                MusEGlobal::audio->msgIdle(false);
                                // DELETETHIS 6
                                //if (mt->type() == MusECore::MidiTrack::DRUM) {//Change channel on all drum instruments
                                //      for (int i=0; i<DRUM_MAPSIZE; i++)
                                //            MusEGlobal::drumMap[i].channel = i->_oldPropValue;
                                //      }
                                //updateFlags |= SC_CHANNELS;
                                MusEGlobal::audio->msgUpdateSoloStates();                   
                                updateFlags |= SC_MIDI_TRACK_PROP;               
                          }
                        }
                        else
                        {
                            if(editable_property_track->type() != MusECore::Track::AUDIO_SOFTSYNTH)
                            {
                              MusECore::AudioTrack* at = dynamic_cast<MusECore::AudioTrack*>(editable_property_track);
                              if (at == 0)
                                break;
                              if (i->_oldPropValue != at->channels()) {
                                    MusEGlobal::audio->msgSetChannels(at, i->_oldPropValue);
                                    updateFlags |= SC_CHANNELS;
                                    }
                            }         
                        }      
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
      for (riUndoOp i = operations.rbegin(); i != operations.rend(); ++i) {
            Track* editable_track = const_cast<Track*>(i->track);
// uncomment if needed            Track* editable_property_track = const_cast<Track*>(i->_propertyTrack);
// uncomment if needed            Part* editable_part = const_cast<Part*>(i->part);
            switch(i->type) {
                  case UndoOp::AddTrack:
                        removeTrack3(editable_track);
                        break;
                  case UndoOp::DeleteTrack:
                        insertTrack3(editable_track, i->trackno);
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
      for (iUndoOp i = operations.begin(); i != operations.end(); ++i) {
            Track* editable_track = const_cast<Track*>(i->track);
            Track* editable_property_track = const_cast<Track*>(i->_propertyTrack);
            Part* editable_part = const_cast<Part*>(i->part);
            switch(i->type) {
                  case UndoOp::SelectPart:
                        editable_part->setSelected(i->selected);
                        updateFlags |= SC_SELECTION;
                        break;
                  case UndoOp::SelectEvent:
                        i->nEvent.setSelected(i->selected);
                        updateFlags |= SC_SELECTION;
                        break;
                        
                  case UndoOp::AddTrack:
                        insertTrack1(editable_track, i->trackno);

                        // FIXME: See comments in Undo1.
                        switch(editable_track->type())
                        {
                              case Track::AUDIO_OUTPUT:
                              case Track::AUDIO_INPUT:
                                      connectJackRoutes((AudioTrack*)editable_track, false);
                                    break;
                              //case Track::AUDIO_SOFTSYNTH: DELETETHIS 4
                                      //SynthI* si = (SynthI*)editable_track;
                                      //si->synth()->init(
                              //      break;
                              default:
                                    break;
                        }

                        break;
                  case UndoOp::DeleteTrack:
                        removeTrack1(editable_track);
                        break;
                  case UndoOp::ModifyTrackName:
                          editable_track->setName(i->_newName);
                          updateFlags |= SC_TRACK_MODIFIED;
                        break;
                  case UndoOp::ModifyClip:
                        MusECore::SndFile::applyUndoFile(i->filename, i->tmpwavfile, i->startframe, i->endframe);
                        break;
                  case UndoOp::ModifyTrackChannel:
                        if (editable_property_track->isMidiTrack()) 
                        {
                          MusECore::MidiTrack* mt = dynamic_cast<MusECore::MidiTrack*>(editable_property_track);
                          if (mt == 0 || mt->type() == MusECore::Track::DRUM)
                            break;
                          if (i->_newPropValue != mt->outChannel()) 
                          {
                                MusEGlobal::audio->msgIdle(true);
                                mt->setOutChanAndUpdate(i->_newPropValue);
                                MusEGlobal::audio->msgIdle(false);
                                // DELETETHIS 5
                                //if (mt->type() == MusECore::MidiTrack::DRUM) {//Change channel on all drum instruments
                                //      for (int i=0; i<DRUM_MAPSIZE; i++)
                                //            MusEGlobal::drumMap[i].channel = i->_newPropValue;
                                //      }
                                //updateFlags |= SC_CHANNELS;
                                MusEGlobal::audio->msgUpdateSoloStates();                   
                                updateFlags |= SC_MIDI_TRACK_PROP;               
                          }
                        }
                        else
                        {
                            if(editable_property_track->type() != MusECore::Track::AUDIO_SOFTSYNTH)
                            {
                              MusECore::AudioTrack* at = dynamic_cast<MusECore::AudioTrack*>(editable_property_track);
                              if (at == 0)
                                break;
                              if (i->_newPropValue != at->channels()) {
                                    MusEGlobal::audio->msgSetChannels(at, i->_newPropValue);
                                    updateFlags |= SC_CHANNELS;
                                    }
                            }         
                        }      
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
      for (iUndoOp i = operations.begin(); i != operations.end(); ++i) {
            Track* editable_track = const_cast<Track*>(i->track);
// uncomment if needed            Track* editable_property_track = const_cast<Track*>(i->_propertyTrack);
// uncomment if needed            Part* editable_part = const_cast<Part*>(i->part);
            switch(i->type) {
                  case UndoOp::AddTrack:
                        insertTrack3(editable_track, i->trackno);
                        break;
                  case UndoOp::DeleteTrack:
                        removeTrack3(editable_track);
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
                   default:
                        break;
                  }
            }
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
