//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: undo.cpp,v 1.12.2.9 2009/05/24 21:43:44 terminator356 Exp $
//
//  (C) Copyright 1999/2000 Werner Schweer (ws@seh.de)
//=========================================================

#include "undo.h"
#include "song.h"
#include "globals.h"

// iundo points to last Undo() in Undo-list

static bool undoMode = false;  // for debugging

std::list<QString> temporaryWavFiles;

//---------------------------------------------------------
//   typeName
//---------------------------------------------------------

const char* UndoOp::typeName()
      {
      static const char* name[] = {
            "AddTrack", "DeleteTrack", "ModifyTrack",
            "AddPart",  "DeletePart",  "ModifyPart",
            "AddEvent", "DeleteEvent", "ModifyEvent",
            "AddTempo", "DeleteTempo", "AddSig", "DeleteSig",
            "SwapTrack"
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
                  printf("%d %s\n", trackno, oTrack->name().latin1());
                  break;
            case ModifyTrack:
                  printf("%d <%s>-<%s>\n", trackno, oTrack->name().latin1(), nTrack->name().latin1());
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
            case ModifyEvent:
            case AddTempo:
            case DeleteTempo:
            case AddSig:
            case SwapTrack:
            case DeleteSig:
            case ModifyClip:
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
    for(iUndo iu = begin(); iu != end(); ++iu)
    {
      Undo& u = *iu;
      for(riUndoOp i = u.rbegin(); i != u.rend(); ++i)
      {
        switch(i->type)
        {
          case UndoOp::DeleteTrack:
                if(i->oTrack)
                {
                  delete i->oTrack;
                  iUndo iu2 = iu;
                  ++iu2;
                  for(; iu2 != end(); ++iu2)
                  {
                    Undo& u2 = *iu2;
                    for(riUndoOp i2 = u2.rbegin(); i2 != u2.rend(); ++i2)
                    {
                      if(i2->type == UndoOp::DeleteTrack)
                      {
                        if(i2->oTrack == i->oTrack)
                          i2->oTrack = 0;
                      }
                    }
                  }
                }
                break;
          case UndoOp::ModifyTrack:
                if(i->oTrack)
                {
                  // Prevent delete i->oTrack from crashing.
                  switch(i->oTrack->type())
                  {
                        case Track::AUDIO_OUTPUT:
                                {
                                AudioOutput* ao = (AudioOutput*)i->oTrack;
                                for(int ch = 0; ch < ao->channels(); ++ch)
                                  ao->setJackPort(ch, 0);
                                }
                              break;
                        case Track::AUDIO_INPUT:
                                {
                                AudioInput* ai = (AudioInput*)i->oTrack;
                                for(int ch = 0; ch < ai->channels(); ++ch)
                                  ai->setJackPort(ch, 0);
                                }
                              break;
                        default:
                              break;
                  }
                  if(!i->oTrack->isMidiTrack())
                    ((AudioTrack*)i->oTrack)->clearEfxList();
                  delete i->oTrack;

                  iUndo iu2 = iu;
                  ++iu2;
                  for(; iu2 != end(); ++iu2)
                  {
                    Undo& u2 = *iu2;
                    for(riUndoOp i2 = u2.rbegin(); i2 != u2.rend(); ++i2)
                    {
                      if(i2->type == UndoOp::ModifyTrack)
                      {
                        if(i2->oTrack == i->oTrack)
                          i2->oTrack = 0;
                      }
                    }
                  }
                }
                break;
          //case UndoOp::DeletePart:
                //delete i->oPart;
          //      break;
          //case UndoOp::DeleteTempo:
          //      break;
          //case UndoOp::DeleteSig:
          //      break;
          default:
                break;
        }
      }
      u.clear();
    }
  }

  clear();
}

//---------------------------------------------------------
//    startUndo
//---------------------------------------------------------

void Song::startUndo()
      {
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
                        removeTrack2(i->oTrack);
                        updateFlags |= SC_TRACK_REMOVED;
                        break;
                  case UndoOp::DeleteTrack:
                        insertTrack2(i->oTrack, i->trackno);
                        // Added by T356.
                        chainTrackParts(i->oTrack, true);
                        
                        updateFlags |= SC_TRACK_INSERTED;
                        break;
                  case UndoOp::ModifyTrack:
                        {
                        // Added by Tim. p3.3.6
                        //printf("Song::doUndo2 ModifyTrack #1 oTrack %p %s nTrack %p %s\n", i->oTrack, i->oTrack->name().latin1(), i->nTrack, i->nTrack->name().latin1());
                        
                        // Unchain the track parts, but don't touch the ref counts.
                        unchainTrackParts(i->nTrack, false);
                        
                        //Track* track = i->nTrack->clone();
                        Track* track = i->nTrack->clone(false);
                        
                        // A Track custom assignment operator was added by Tim. 
                        *(i->nTrack) = *(i->oTrack);
                        
                        // Added by Tim. p3.3.6
                        //printf("Song::doUndo2 ModifyTrack #2 oTrack %p %s nTrack %p %s\n", i->oTrack, i->oTrack->name().latin1(), i->nTrack, i->nTrack->name().latin1());
                        
                        // Prevent delete i->oTrack from crashing.
                        switch(i->oTrack->type())
                        {
                              case Track::AUDIO_OUTPUT:
                                      {
                                      AudioOutput* ao = (AudioOutput*)i->oTrack;
                                      for(int ch = 0; ch < ao->channels(); ++ch)
                                        ao->setJackPort(ch, 0);
                                      }
                                    break;
                              case Track::AUDIO_INPUT:
                                      {
                                      AudioInput* ai = (AudioInput*)i->oTrack;
                                      for(int ch = 0; ch < ai->channels(); ++ch)
                                        ai->setJackPort(ch, 0);
                                      }
                                    break;
                              default:
                                    break;
                        }
                        if(!i->oTrack->isMidiTrack())
                          ((AudioTrack*)i->oTrack)->clearEfxList();

                        delete i->oTrack;
                        i->oTrack = track;
                        
                        // Chain the track parts, but don't touch the ref counts.
                        chainTrackParts(i->nTrack, false);

                        // Added by Tim. p3.3.6
                        //printf("Song::doUndo2 ModifyTrack #3 oTrack %p %s nTrack %p %s\n", i->oTrack, i->oTrack->name().latin1(), i->nTrack, i->nTrack->name().latin1());
                        
                        // Connect and register ports.
                        switch(i->nTrack->type())
                        {
                          case Track::AUDIO_OUTPUT:
                              {
                              AudioOutput* ao = (AudioOutput*)i->nTrack;
                              ao->setName(ao->name());
                              }
                            break;
                          case Track::AUDIO_INPUT:
                              {
                              AudioInput* ai = (AudioInput*)i->nTrack;
                              ai->setName(ai->name());
                              }
                            break;
                          default:
                            break;
                        }

                        // Update solo states, since the user may have changed soloing on other tracks.
                        updateSoloStates();

                        updateFlags |= SC_TRACK_MODIFIED;
                        }
                        break;
                        
                        /*
                        switch(i->nTrack->type())
                        {
                              case Track::AUDIO_OUTPUT:
                                      {
                                      AudioOutput* ao = (AudioOutput*)i->nTrack;
                                      for(int ch = 0; ch < ao->channels(); ++ch)
                                        ao->setJackPort(ch, 0);
                                      }
                                    break;
                              case Track::AUDIO_INPUT:
                                      {
                                      AudioInput* ai = (AudioInput*)i->nTrack;
                                      for(int ch = 0; ch < ai->channels(); ++ch)
                                        ai->setJackPort(ch, 0);
                                      }
                                    break;
                              default:
                                    break;
                        }
                        if(!i->nTrack->isMidiTrack())
                          ((AudioTrack*)i->nTrack)->clearEfxList();

                        //delete i->oTrack;
                        //i->oTrack = track;
                        
                        // Remove the track. removeTrack2 takes care of unchaining the new track.
                        removeTrack2(i->nTrack);
                        
                        // Connect and register ports.
                        switch(i->oTrack->type())
                        {
                          case Track::AUDIO_OUTPUT:
                              {
                              AudioOutput* ao = (AudioOutput*)i->oTrack;
                              ao->setName(ao->name());
                              }
                            break;
                          case Track::AUDIO_INPUT:
                              {
                              AudioInput* ai = (AudioInput*)i->oTrack;
                              ai->setName(ai->name());
                              }
                            break;
                          default:
                            break;
                        }

                        // Insert the old track.
                        insertTrack2(i->oTrack, i->trackno);
                        // Chain the old track parts. (removeTrack2, above, takes care of unchaining the new track).
                        chainTrackParts(i->oTrack, true);
                        
                        // Update solo states, since the user may have changed soloing on other tracks.
                        updateSoloStates();

                        updateFlags |= SC_TRACK_MODIFIED;
                        }
                        break;
                        */
                        
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
                        //i->oPart->unchainClone();
                        unchainClone(i->oPart);
                        }
                        break;
                  case UndoOp::DeletePart:
                        addPart(i->oPart);
                        updateFlags |= SC_PART_INSERTED;
                        i->oPart->events()->incARef(1);
                        //i->oPart->chainClone();
                        chainClone(i->oPart);
                        break;
                  case UndoOp::ModifyPart:
                        if(i->doCtrls)
                          removePortCtrlEvents(i->oPart, i->doClones);
                        changePart(i->oPart, i->nPart);
                        i->oPart->events()->incARef(-1);
                        i->nPart->events()->incARef(1);
                        //i->oPart->replaceClone(i->nPart);
                        replaceClone(i->oPart, i->nPart);
                        if(i->doCtrls)
                          addPortCtrlEvents(i->nPart, i->doClones);
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
                        //printf("doUndo2: UndoOp::AddTempo. deleting tempo at: %d\n", i->a);
                        tempomap.delTempo(i->a);
                        updateFlags |= SC_TEMPO;
                        break;
                  case UndoOp::DeleteTempo:
                        //printf("doUndo2: UndoOp::DeleteTempo. adding tempo at: %d, tempo=%d\n", i->a, i->b);
                        tempomap.addTempo(i->a, i->b);
                        updateFlags |= SC_TEMPO;
                        break;
                  case UndoOp::AddSig:
                        sigmap.del(i->a);
                        updateFlags |= SC_SIG;
                        break;
                  case UndoOp::DeleteSig:
                        sigmap.add(i->a, i->b, i->c);
                        updateFlags |= SC_SIG;
                        break;
                  case UndoOp::ModifyClip:
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
                        insertTrack2(i->oTrack, i->trackno);
                        // Added by T356.
                        chainTrackParts(i->oTrack, true);
                        
                        updateFlags |= SC_TRACK_INSERTED;
                        break;
                  case UndoOp::DeleteTrack:
                        removeTrack2(i->oTrack);
                        updateFlags |= SC_TRACK_REMOVED;
                        break;
                  case UndoOp::ModifyTrack:
                        {
                        // Unchain the track parts, but don't touch the ref counts.
                        unchainTrackParts(i->nTrack, false);
                        
                        //Track* track = i->nTrack->clone();
                        Track* track = i->nTrack->clone(false);
                        
                        *(i->nTrack) = *(i->oTrack);

                        // Prevent delete i->oTrack from crashing.
                        switch(i->oTrack->type())
                        {
                              case Track::AUDIO_OUTPUT:
                                      {
                                      AudioOutput* ao = (AudioOutput*)i->oTrack;
                                      for(int ch = 0; ch < ao->channels(); ++ch)
                                        ao->setJackPort(ch, 0);
                                      }
                                    break;
                              case Track::AUDIO_INPUT:
                                      {
                                      AudioInput* ai = (AudioInput*)i->oTrack;
                                      for(int ch = 0; ch < ai->channels(); ++ch)
                                        ai->setJackPort(ch, 0);
                                      }
                                    break;
                              default:
                                    break;
                        }
                        if(!i->oTrack->isMidiTrack())
                          ((AudioTrack*)i->oTrack)->clearEfxList();

                        delete i->oTrack;
                        i->oTrack = track;

                        // Chain the track parts, but don't touch the ref counts.
                        chainTrackParts(i->nTrack, false);

                        // Connect and register ports.
                        switch(i->nTrack->type())
                        {
                          case Track::AUDIO_OUTPUT:
                              {
                              AudioOutput* ao = (AudioOutput*)i->nTrack;
                              ao->setName(ao->name());
                              }
                            break;
                          case Track::AUDIO_INPUT:
                              {
                              AudioInput* ai = (AudioInput*)i->nTrack;
                              ai->setName(ai->name());
                              }
                            break;
                          default:
                            break;
                        }

                        // Update solo states, since the user may have changed soloing on other tracks.
                        updateSoloStates();

                        updateFlags |= SC_TRACK_MODIFIED;
                        }
                        break;
                  
                        /*
                        // Prevent delete i->oTrack from crashing.
                        switch(i->oTrack->type())
                        {
                              case Track::AUDIO_OUTPUT:
                                      {
                                      AudioOutput* ao = (AudioOutput*)i->oTrack;
                                      for(int ch = 0; ch < ao->channels(); ++ch)
                                        ao->setJackPort(ch, 0);
                                      }
                                    break;
                              case Track::AUDIO_INPUT:
                                      {
                                      AudioInput* ai = (AudioInput*)i->oTrack;
                                      for(int ch = 0; ch < ai->channels(); ++ch)
                                        ai->setJackPort(ch, 0);
                                      }
                                    break;
                              default:
                                    break;
                        }
                        if(!i->oTrack->isMidiTrack())
                          ((AudioTrack*)i->oTrack)->clearEfxList();

                        //delete i->oTrack;
                        //i->oTrack = track;

                        // Remove the track. removeTrack2 takes care of unchaining the old track.
                        removeTrack2(i->oTrack);
                        
                        // Connect and register ports.
                        switch(i->nTrack->type())
                        {
                          case Track::AUDIO_OUTPUT:
                              {
                              AudioOutput* ao = (AudioOutput*)i->nTrack;
                              ao->setName(ao->name());
                              }
                            break;
                          case Track::AUDIO_INPUT:
                              {
                              AudioInput* ai = (AudioInput*)i->nTrack;
                              ai->setName(ai->name());
                              }
                            break;
                          default:
                            break;
                        }

                        // Insert the new track.
                        insertTrack2(i->nTrack, i->trackno);
                        // Chain the new track parts. (removeTrack2, above, takes care of unchaining the old track).
                        chainTrackParts(i->nTrack, true);
                        
                        // Update solo states, since the user may have changed soloing on other tracks.
                        updateSoloStates();

                        updateFlags |= SC_TRACK_MODIFIED;
                        }
                        break;
                        */
                  
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
                        //i->oPart->chainClone();
                        chainClone(i->oPart);
                        break;
                  case UndoOp::DeletePart:
                        removePart(i->oPart);
                        updateFlags |= SC_PART_REMOVED;
                        i->oPart->events()->incARef(-1);
                        //i->oPart->unchainClone();
                        unchainClone(i->oPart);
                        break;
                  case UndoOp::ModifyPart:
                        if(i->doCtrls)
                          removePortCtrlEvents(i->nPart, i->doClones);
                        changePart(i->nPart, i->oPart);
                        i->oPart->events()->incARef(1);
                        i->nPart->events()->incARef(-1);
                        //i->nPart->replaceClone(i->oPart);
                        replaceClone(i->nPart, i->oPart);
                        if(i->doCtrls)
                          addPortCtrlEvents(i->oPart, i->doClones);
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
                        //printf("doRedo2: UndoOp::AddTempo. adding tempo at: %d with tempo=%d\n", i->a, i->b);
                        tempomap.addTempo(i->a, i->b);
                        updateFlags |= SC_TEMPO;
                        break;
                  case UndoOp::DeleteTempo:
                        //printf("doRedo2: UndoOp::DeleteTempo. deleting tempo at: %d with tempo=%d\n", i->a, i->b);
                        tempomap.delTempo(i->a);
                        updateFlags |= SC_TEMPO;
                        break;
                  case UndoOp::AddSig:
                        sigmap.add(i->a, i->b, i->c);
                        updateFlags |= SC_SIG;
                        break;
                  case UndoOp::DeleteSig:
                        sigmap.del(i->a);
                        updateFlags |= SC_SIG;
                        break;
                  case UndoOp::ModifyClip:
                        break;
                  }
            }
      }

void Song::undoOp(UndoOp::UndoType type, int a, int b, int c)
      {
      UndoOp i;
      i.type = type;
      i.a  = a;
      i.b  = b;
      i.c  = c;
      addUndo(i);
      }

//void Song::undoOp(UndoOp::UndoType type, Track* oldTrack, Track* newTrack)
void Song::undoOp(UndoOp::UndoType type, int n, Track* oldTrack, Track* newTrack)
      {
      UndoOp i;
      i.type    = type;
      i.trackno = n;
      i.oTrack  = oldTrack;
      i.nTrack  = newTrack;
      // Added by Tim. p3.3.6
      //printf("Song::undoOp ModifyTrack oTrack %p %s nTrack %p %s\n", i.oTrack, i.oTrack->name().latin1(), i.nTrack, i.nTrack->name().latin1());
                        
      addUndo(i);
      }

void Song::undoOp(UndoOp::UndoType type, int n, Track* track)
      {
      UndoOp i;
      i.type    = type;
      i.trackno = n;
      i.oTrack  = track;
      if (type == UndoOp::AddTrack)
            updateFlags |= SC_TRACK_INSERTED;
      addUndo(i);
      }

void Song::undoOp(UndoOp::UndoType type, Part* part)
      {
      UndoOp i;
      i.type  = type;
      i.oPart = part;
      addUndo(i);
      }

//void Song::undoOp(UndoOp::UndoType type, Event& oev, Event& nev, Part* part)
void Song::undoOp(UndoOp::UndoType type, Event& oev, Event& nev, Part* part, bool doCtrls, bool doClones)
      {
      UndoOp i;
      i.type   = type;
      i.nEvent = nev;
      i.oEvent = oev;
      i.part   = part;
      i.doCtrls = doCtrls;
      i.doClones = doClones;
      addUndo(i);
      }

void Song::undoOp(UndoOp::UndoType type, Event& nev, Part* part, bool doCtrls, bool doClones)
      {
      UndoOp i;
      i.type   = type;
      i.nEvent = nev;
      i.part   = part;
      i.doCtrls = doCtrls;
      i.doClones = doClones;
      addUndo(i);
      }

//void Song::undoOp(UndoOp::UndoType type, Part* oPart, Part* nPart)
void Song::undoOp(UndoOp::UndoType type, Part* oPart, Part* nPart, bool doCtrls, bool doClones)
      {
      UndoOp i;
      i.type  = type;
      i.oPart = nPart;
      i.nPart = oPart;
      i.doCtrls = doCtrls;
      i.doClones = doClones;
      addUndo(i);
      }

void Song::undoOp(UndoOp::UndoType type, int c, int ctrl, int ov, int nv)
      {
      UndoOp i;
      i.type    = type;
      i.channel = c;
      i.ctrl    = ctrl;
      i.oVal    = ov;
      i.nVal    = nv;
      addUndo(i);
      }

void Song::undoOp(UndoOp::UndoType type, SigEvent* oevent, SigEvent* nevent)
      {
      UndoOp i;
      i.type       = type;
      i.oSignature = oevent;
      i.nSignature = nevent;
      addUndo(i);
      }

void Song::undoOp(UndoOp::UndoType type, const char* changedFile, const char* changeData, int startframe, int endframe)
      {
      UndoOp i;
      i.type = type;
      i.filename   = changedFile;
      i.tmpwavfile = changeData;
      i.startframe = startframe;
      i.endframe   = endframe;
      addUndo(i);
      temporaryWavFiles.push_back(QString(changeData));

      //printf("Adding ModifyClip undo-operation: origfile=%s tmpfile=%s sf=%d ef=%d\n", changedFile, changeData, startframe, endframe);
      }

//---------------------------------------------------------
//   addUndo
//---------------------------------------------------------

void Song::addUndo(UndoOp& i)
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
                        removeTrack1(i->oTrack);
                        break;
                  case UndoOp::DeleteTrack:
                        insertTrack1(i->oTrack, i->trackno);

                        // FIXME: Would like to put this part in Undo2, but indications
                        //  elsewhere are that (dis)connecting jack routes must not be
                        //  done in the realtime thread. The result is that we get a few
                        //  "PANIC Process init: No buffer from audio device" messages
                        //  before the routes are (dis)connected. So far seems to do no harm though...
                        switch(i->oTrack->type())
                        {
                              case Track::AUDIO_OUTPUT:
                              case Track::AUDIO_INPUT:
                                      connectJackRoutes((AudioTrack*)i->oTrack, false);
                                    break;
                              //case Track::AUDIO_SOFTSYNTH:
                                      //SynthI* si = (SynthI*)i->oTrack;
                                      //si->synth()->init(
                              //      break;
                              default:
                                    break;
                        }

                        break;
                  case UndoOp::ModifyClip:
                        SndFile::applyUndoFile(i->filename, i->tmpwavfile, i->startframe, i->endframe);
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
                        removeTrack3(i->oTrack);
                        break;
                  case UndoOp::DeleteTrack:
                        insertTrack3(i->oTrack, i->trackno);
                        break;
                  case UndoOp::ModifyTrack:
                        // Not much choice but to do this - Tim.
                        //clearClipboardAndCloneList();
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
                        insertTrack1(i->oTrack, i->trackno);

                        // FIXME: See comments in Undo1.
                        switch(i->oTrack->type())
                        {
                              case Track::AUDIO_OUTPUT:
                              case Track::AUDIO_INPUT:
                                      connectJackRoutes((AudioTrack*)i->oTrack, false);
                                    break;
                              //case Track::AUDIO_SOFTSYNTH:
                                      //SynthI* si = (SynthI*)i->oTrack;
                                      //si->synth()->init(
                              //      break;
                              default:
                                    break;
                        }

                        break;
                  case UndoOp::DeleteTrack:
                        removeTrack1(i->oTrack);
                        break;
                  case UndoOp::ModifyClip:
                        SndFile::applyUndoFile(i->filename, i->tmpwavfile, i->startframe, i->endframe);
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
                        insertTrack3(i->oTrack, i->trackno);
                        break;
                  case UndoOp::DeleteTrack:
                        removeTrack3(i->oTrack);
                        break;
                  case UndoOp::ModifyTrack:
                        // Not much choice but to do this - Tim.
                        //clearClipboardAndCloneList();
                        break;      
                  default:
                        break;
                  }
            }
      undoList->push_back(u); // put item on undo list
      redoList->pop_back();
      dirty = true;
      }

