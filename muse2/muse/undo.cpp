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
            "AddPart",  "DeletePart",  "ModifyPart",
            "AddEvent", "DeleteEvent", "ModifyEvent",
            "AddTempo", "DeleteTempo",
            "AddSig", "DeleteSig",
            "AddKey", "DeleteKey",
            "ModifyTrackName", "ModifyTrackChannel",
            "SwapTrack", 
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
            case ModifyPart:
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
            case SwapTrack:
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
                    delete i->track;
                  break;
                  
            case UndoOp::DeletePart:
                  delete i->oPart;
                  break;

            case UndoOp::ModifyPart:
                  delete i->oPart;
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
                  delete i->oPart;
                  break;

            case UndoOp::ModifyPart:
                  delete i->nPart;
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
			
      undoList->push_back(Undo());
      updateFlags = 0;
      undoMode = true;
      }

//---------------------------------------------------------
//   endUndo
//---------------------------------------------------------

void Song::endUndo(int flags)
      {
      updateFlags |= flags;
      endMsgCmd();
      undoMode = false;
      }


void cleanOperationGroup(Undo& group)
{
	using std::set;
	
	set<Track*> processed_tracks;
	set<Part*> processed_parts;

	for (iUndoOp op=group.begin(); op!=group.end();)
	{
		iUndoOp op_=op;
		op_++;
		
		if (op->type==UndoOp::DeleteTrack)
		{
			if (processed_tracks.find(op->track)!=processed_tracks.end())
				group.erase(op);
			else
				processed_tracks.insert(op->track);
		}
		else if ((op->type==UndoOp::ModifyPart) || (op->type==UndoOp::DeletePart))
		{
			if (processed_parts.find(op->oPart)!=processed_parts.end())
				group.erase(op);
			else
				processed_parts.insert(op->oPart);
		}
		
		op=op_;
	}
}


bool Song::applyOperationGroup(Undo& group, bool doUndo)
{
      if (!group.empty())
      {
            cleanOperationGroup(group);
            //this is a HACK! but it works :)    (added by flo93)
            redoList->push_back(group);
            redo();
            
            if (!doUndo)
            {
                  undoList->pop_back();
                  MusEGlobal::undoAction->setEnabled(!undoList->empty());
            }
            else
            {
                  redoList->clearDelete(); // redo must be invalidated when a new undo is started
                  MusEGlobal::redoAction->setEnabled(false);
						}
            
            return doUndo;
      }
      else
            return false;
}



//---------------------------------------------------------
//   doUndo2
//    real time part
//---------------------------------------------------------

void Song::doUndo2()
      {
      Undo& u = undoList->back();
      for (riUndoOp i = u.rbegin(); i != u.rend(); ++i) {
            switch(i->type) {
                  case UndoOp::AddTrack:
                        removeTrack2(i->track);
                        updateFlags |= SC_TRACK_REMOVED;
                        break;
                  case UndoOp::DeleteTrack:
                        insertTrack2(i->track, i->trackno);
                        chainTrackParts(i->track, true);
                        
                        updateFlags |= SC_TRACK_INSERTED;
                        break;
                        
                  case UndoOp::SwapTrack:
                        {
                        updateFlags |= SC_TRACK_MODIFIED;
                        Track* track  = _tracks[i->a];
                        _tracks[i->a] = _tracks[i->b];
                        _tracks[i->b] = track;
                        updateFlags |= SC_TRACK_MODIFIED;
                        }
                        break;
                  case UndoOp::AddPart:
                        {
                        Part* part = i->oPart;
                        removePart(part);
                        updateFlags |= SC_PART_REMOVED;
                        i->oPart->events()->incARef(-1);
                        unchainClone(i->oPart);
                        }
                        break;
                  case UndoOp::DeletePart:
                        addPart(i->oPart);
                        updateFlags |= SC_PART_INSERTED;
                        i->oPart->events()->incARef(1);
                        chainClone(i->oPart);
                        break;
                  case UndoOp::ModifyPart:
                        if(i->doCtrls)
                          removePortCtrlEvents(i->nPart, i->doClones);
                        changePart(i->nPart, i->oPart);
                        i->nPart->events()->incARef(-1);
                        i->oPart->events()->incARef(1);
                        replaceClone(i->nPart, i->oPart);
                        if(i->doCtrls)
                          addPortCtrlEvents(i->oPart, i->doClones);
                        updateFlags |= SC_PART_MODIFIED;
                        break;
                  case UndoOp::AddEvent:
                        if(i->doCtrls)
                          removePortCtrlEvents(i->nEvent, i->part, i->doClones);
                        deleteEvent(i->nEvent, i->part);
                        updateFlags |= SC_EVENT_REMOVED;
                        break;
                  case UndoOp::DeleteEvent:
                        addEvent(i->nEvent, i->part);
                        if(i->doCtrls)
                          addPortCtrlEvents(i->nEvent, i->part, i->doClones);
                        updateFlags |= SC_EVENT_INSERTED;
                        break;
                  case UndoOp::ModifyEvent:
                        if(i->doCtrls)
                          removePortCtrlEvents(i->oEvent, i->part, i->doClones);
                        changeEvent(i->oEvent, i->nEvent, i->part);
                        if(i->doCtrls)
                          addPortCtrlEvents(i->nEvent, i->part, i->doClones);
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
                        updateFlags = -1; // set all flags
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
//   Song::doRedo2
//---------------------------------------------------------

void Song::doRedo2()
      {
      Undo& u = redoList->back();
      for (iUndoOp i = u.begin(); i != u.end(); ++i) {
            switch(i->type) {
                  case UndoOp::AddTrack:
                        insertTrack2(i->track, i->trackno);
                        chainTrackParts(i->track, true);
                        
                        updateFlags |= SC_TRACK_INSERTED;
                        break;
                  case UndoOp::DeleteTrack:
                        removeTrack2(i->track);
                        updateFlags |= SC_TRACK_REMOVED;
                        break;
                        
                        
                  case UndoOp::SwapTrack:
                        {
                        Track* track  = _tracks[i->a];
                        _tracks[i->a] = _tracks[i->b];
                        _tracks[i->b] = track;
                        updateFlags |= SC_TRACK_MODIFIED;
                        }
                        break;
                  case UndoOp::AddPart:
                        addPart(i->oPart);
                        updateFlags |= SC_PART_INSERTED;
                        i->oPart->events()->incARef(1);
                        chainClone(i->oPart);
                        break;
                  case UndoOp::DeletePart:
                        removePart(i->oPart);
                        updateFlags |= SC_PART_REMOVED;
                        i->oPart->events()->incARef(-1);
                        unchainClone(i->oPart);
                        break;
                  case UndoOp::ModifyPart:
                        if(i->doCtrls)
                          removePortCtrlEvents(i->oPart, i->doClones);
                        changePart(i->oPart, i->nPart);
                        i->nPart->events()->incARef(1);
                        i->oPart->events()->incARef(-1);
                        replaceClone(i->oPart, i->nPart);
                        if(i->doCtrls)
                          addPortCtrlEvents(i->nPart, i->doClones);
                        updateFlags |= SC_PART_MODIFIED;
                        break;
                  case UndoOp::AddEvent:
                        addEvent(i->nEvent, i->part);
                        if(i->doCtrls)
                          addPortCtrlEvents(i->nEvent, i->part, i->doClones);
                        updateFlags |= SC_EVENT_INSERTED;
                        break;
                  case UndoOp::DeleteEvent:
                        if(i->doCtrls)
                          removePortCtrlEvents(i->nEvent, i->part, i->doClones);
                        deleteEvent(i->nEvent, i->part);
                        updateFlags |= SC_EVENT_REMOVED;
                        break;
                  case UndoOp::ModifyEvent:
                        if(i->doCtrls)
                          removePortCtrlEvents(i->nEvent, i->part, i->doClones);
                        changeEvent(i->nEvent, i->oEvent, i->part);
                        if(i->doCtrls)
                          addPortCtrlEvents(i->oEvent, i->part, i->doClones);
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
                        updateFlags = -1; // set all flags
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

UndoOp::UndoOp(UndoType type_)
{
	type = type_;
}

UndoOp::UndoOp(UndoType type_, int a_, int b_, int c_)
      {
      type = type_;
      a  = a_;
      b  = b_;
      c  = c_;
      }


UndoOp::UndoOp(UndoType type_, int n, Track* track_)
      {
      type    = type_;
      trackno = n;
      track  = track_;
      }

UndoOp::UndoOp(UndoType type_, Part* part)
      {
      type  = type_;
      oPart = part;
      }

UndoOp::UndoOp(UndoType type_, Event& oev, Event& nev, Part* part_, bool doCtrls_, bool doClones_)
      {
      type   = type_;
      nEvent = nev;
      oEvent = oev;
      part   = part_;
      doCtrls = doCtrls_;
      doClones = doClones_;
      }

UndoOp::UndoOp(UndoType type_, Event& nev, Part* part_, bool doCtrls_, bool doClones_)
      {
      type   = type_;
      nEvent = nev;
      part   = part_;
      doCtrls = doCtrls_;
      doClones = doClones_;
      }

UndoOp::UndoOp(UndoType type_, Part* oPart_, Part* nPart_, bool doCtrls_, bool doClones_)
      {
      type  = type_;
      oPart = oPart_;
      nPart = nPart_;
      doCtrls = doCtrls_;
      doClones = doClones_;
      }

UndoOp::UndoOp(UndoType type_, int c, int ctrl_, int ov, int nv)
      {
      type    = type_;
      channel = c;
      ctrl    = ctrl_;
      oVal    = ov;
      nVal    = nv;
      }

UndoOp::UndoOp(UndoType type_, Marker* copyMarker_, Marker* realMarker_)
      {
      type    = type_;
      realMarker  = realMarker_;
      copyMarker  = copyMarker_;
      }

UndoOp::UndoOp(UndoType type_, const char* changedFile, const char* changeData, int startframe_, int endframe_)
      {
      type = type_;
      filename   = changedFile;
      tmpwavfile = changeData;
      startframe = startframe_;
      endframe   = endframe_;
      }

UndoOp::UndoOp(UndoOp::UndoType type_, Track* track_, const char* old_name, const char* new_name)
{
  type = type_;
  _renamedTrack = track_;
  _oldName = new char[strlen(old_name) + 1];
  _newName = new char[strlen(new_name) + 1];
  strcpy(_oldName, old_name);
  strcpy(_newName, new_name);
}

UndoOp::UndoOp(UndoOp::UndoType type_, Track* track_, int old_chan, int new_chan)
{
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
      dirty = true;
      }

//---------------------------------------------------------
//   doUndo1
//    non realtime context
//    return true if nothing to do
//---------------------------------------------------------

bool Song::doUndo1()
      {
      if (undoList->empty())
            return true;
      Undo& u = undoList->back();
      for (riUndoOp i = u.rbegin(); i != u.rend(); ++i) {
            switch(i->type) {
                  case UndoOp::AddTrack:
                        removeTrack1(i->track);
                        break;
                  case UndoOp::DeleteTrack:
                        insertTrack1(i->track, i->trackno);

                        // FIXME: Would like to put this part in Undo2, but indications
                        //  elsewhere are that (dis)connecting jack routes must not be
                        //  done in the realtime thread. The result is that we get a few
                        //  "PANIC Process init: No buffer from audio device" messages
                        //  before the routes are (dis)connected. So far seems to do no harm though...
                        switch(i->track->type())
                        {
                              case Track::AUDIO_OUTPUT:
                              case Track::AUDIO_INPUT:
                                      connectJackRoutes((AudioTrack*)i->track, false);
                                    break;
                              //case Track::AUDIO_SOFTSYNTH: DELETETHIS 4
                                      //SynthI* si = (SynthI*)i->track;
                                      //si->synth()->init(
                              //      break;
                              default:
                                    break;
                        }

                        break;
                  case UndoOp::ModifyTrackName:
                          i->_renamedTrack->setName(i->_oldName);
                          updateFlags |= SC_TRACK_MODIFIED;
                        break;
                  case UndoOp::ModifyClip:
                        MusECore::SndFile::applyUndoFile(i->filename, i->tmpwavfile, i->startframe, i->endframe);
                        break;
                  case UndoOp::ModifyTrackChannel:
                        if (i->_propertyTrack->isMidiTrack()) 
                        {
                          MusECore::MidiTrack* mt = dynamic_cast<MusECore::MidiTrack*>(i->_propertyTrack);
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
                            if(i->_propertyTrack->type() != MusECore::Track::AUDIO_SOFTSYNTH)
                            {
                              MusECore::AudioTrack* at = dynamic_cast<MusECore::AudioTrack*>(i->_propertyTrack);
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
      return false;
      }

//---------------------------------------------------------
//   doUndo3
//    non realtime context
//---------------------------------------------------------

void Song::doUndo3()
      {
      Undo& u = undoList->back();
      for (riUndoOp i = u.rbegin(); i != u.rend(); ++i) {
            switch(i->type) {
                  case UndoOp::AddTrack:
                        removeTrack3(i->track);
                        break;
                  case UndoOp::DeleteTrack:
                        insertTrack3(i->track, i->trackno);
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
      redoList->push_back(u); // put item on redo list
      undoList->pop_back();
      dirty = true;
      }

//---------------------------------------------------------
//   doRedo1
//    non realtime context
//    return true if nothing to do
//---------------------------------------------------------

bool Song::doRedo1()
      {
      if (redoList->empty())
            return true;
      Undo& u = redoList->back();
      for (iUndoOp i = u.begin(); i != u.end(); ++i) {
            switch(i->type) {
                  case UndoOp::AddTrack:
                        insertTrack1(i->track, i->trackno);

                        // FIXME: See comments in Undo1.
                        switch(i->track->type())
                        {
                              case Track::AUDIO_OUTPUT:
                              case Track::AUDIO_INPUT:
                                      connectJackRoutes((AudioTrack*)i->track, false);
                                    break;
                              //case Track::AUDIO_SOFTSYNTH: DELETETHIS 4
                                      //SynthI* si = (SynthI*)i->track;
                                      //si->synth()->init(
                              //      break;
                              default:
                                    break;
                        }

                        break;
                  case UndoOp::DeleteTrack:
                        removeTrack1(i->track);
                        break;
                  case UndoOp::ModifyTrackName:
                          i->_renamedTrack->setName(i->_newName);
                          updateFlags |= SC_TRACK_MODIFIED;
                        break;
                  case UndoOp::ModifyClip:
                        MusECore::SndFile::applyUndoFile(i->filename, i->tmpwavfile, i->startframe, i->endframe);
                        break;
                  case UndoOp::ModifyTrackChannel:
                        if (i->_propertyTrack->isMidiTrack()) 
                        {
                          MusECore::MidiTrack* mt = dynamic_cast<MusECore::MidiTrack*>(i->_propertyTrack);
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
                            if(i->_propertyTrack->type() != MusECore::Track::AUDIO_SOFTSYNTH)
                            {
                              MusECore::AudioTrack* at = dynamic_cast<MusECore::AudioTrack*>(i->_propertyTrack);
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
      return false;
      }

//---------------------------------------------------------
//   doRedo3
//    non realtime context
//---------------------------------------------------------

void Song::doRedo3()
      {
      Undo& u = redoList->back();
      for (iUndoOp i = u.begin(); i != u.end(); ++i) {
            switch(i->type) {
                  case UndoOp::AddTrack:
                        insertTrack3(i->track, i->trackno);
                        break;
                  case UndoOp::DeleteTrack:
                        removeTrack3(i->track);
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
      undoList->push_back(u); // put item on undo list
      redoList->pop_back();
      dirty = true;
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
