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

#include "undo.h"
#include "song.h"
#include "globals.h"
#include "al/tempo.h"
#include "al/sig.h"
#include "al/sig.h"
#include "part.h"

// iundo points to last Undo() in Undo-list

static bool undoMode = false;  // for debugging
std::list<QString> temporaryWavFiles;

//---------------------------------------------------------
//   typeName
//---------------------------------------------------------

const char* UndoOp::typeName()
      {
      static const char* name[] = {
            "AddTrack", "DeleteTrack", "RenameTrack",
            "AddPart",  "DeletePart",  "ModifyPart",
            "AddEvent", "DeleteEvent", "ModifyEvent",
            "AddTempo", "DeleteTempo",
            "AddSig", "DeleteSig",
            "SwapTrack",
            "ModifyClip",
            "AddCtrl", "RemoveCtrl", "ModifyCtrl"
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
                  printf("%d %s\n", id, track->name().toLatin1().data());
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
            case DeleteSig:
            case SwapTrack:
            case AddCtrl:
            case RemoveCtrl:
            case ModifyCtrl:
            case RenameTrack:
            case ModifyClip:
                  break;
            }
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

// printf("doUndo2\n");
      for (riUndoOp i = u.rbegin(); i != u.rend(); ++i) {
// printf("  doUndo2 %s\n", i->typeName());
            switch(i->type) {
                  case UndoOp::AddTrack:
                        removeTrack2(i->track);
                        updateFlags |= SC_TRACK_REMOVED;
                        break;
                  case UndoOp::DeleteTrack:
                        insertTrack2(i->track);
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
                  case UndoOp::RenameTrack:
                        i->track->setName(*(i->os));
                        break;
                  case UndoOp::AddPart:
                        {
                        Part* part = i->oPart;
                        part->track()->parts()->remove(part);
                        updateFlags |= SC_PART_REMOVED;
                        i->oPart->events()->incARef(-1);
                        }
                        break;
                  case UndoOp::DeletePart:
                        i->oPart->track()->addPart(i->oPart);
                        updateFlags |= SC_PART_INSERTED;
                        i->oPart->events()->incARef(1);
                        break;
                  case UndoOp::ModifyPart:
                        changePart(i->oPart, i->nPart);
                        updateFlags |= SC_PART_MODIFIED;
                        break;
                  case UndoOp::AddEvent:
                        deleteEvent(i->nEvent, i->part);
                        updateFlags |= SC_EVENT_REMOVED;
                        break;
                  case UndoOp::DeleteEvent:
                        addEvent(i->nEvent, i->part);
                        updateFlags |= SC_EVENT_INSERTED;
                        break;
                  case UndoOp::ModifyEvent:
                        updateFlags |= SC_EVENT_MODIFIED;
                        changeEvent(i->oEvent, i->nEvent, i->part);
                        break;
                  case UndoOp::AddTempo:
                        AL::tempomap.delTempo(i->a);
                        updateFlags |= SC_TEMPO;
                        break;
                  case UndoOp::DeleteTempo:
                        AL::tempomap.addTempo(i->a, i->b);
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
                  case UndoOp::AddCtrl:
                        i->track->removeControllerVal(i->id, i->time);
                        break;
                  case UndoOp::RemoveCtrl:
                        i->track->addControllerVal(i->id, i->time, i->cval1);
                        break;
                  case UndoOp::ModifyCtrl:
                        i->track->addControllerVal(i->id, i->time, i->cval2);
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
// printf("doRedo2\n");
      for (iUndoOp i = u.begin(); i != u.end(); ++i) {
// printf("  doRedo2 %s\n", i->typeName());
            switch(i->type) {
                  case UndoOp::AddTrack:
                        insertTrack2(i->track);
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
                  case UndoOp::RenameTrack:
                        i->track->setName(*(i->ns));
                        break;
                  case UndoOp::AddPart:
                        i->oPart->track()->addPart(i->oPart);
                        updateFlags |= SC_PART_INSERTED;
                        i->oPart->events()->incARef(1);
                        break;
                  case UndoOp::DeletePart:
                        {
                        Part* part = i->oPart;
                        part->track()->parts()->remove(part);
                        updateFlags |= SC_PART_REMOVED;
                        i->oPart->events()->incARef(-1);
                        }
                        break;
                  case UndoOp::ModifyPart:
                        changePart(i->nPart, i->oPart);
                        updateFlags |= SC_PART_MODIFIED;
                        break;
                  case UndoOp::AddEvent:
                        addEvent(i->nEvent, i->part);
                        updateFlags |= SC_EVENT_INSERTED;
                        break;
                  case UndoOp::DeleteEvent:
                        deleteEvent(i->nEvent, i->part);
                        updateFlags |= SC_EVENT_REMOVED;
                        break;
                  case UndoOp::ModifyEvent:
                        changeEvent(i->nEvent, i->oEvent, i->part);
                        updateFlags |= SC_EVENT_MODIFIED;
                        break;
                  case UndoOp::AddTempo:
                        AL::tempomap.addTempo(i->a, i->b);
                        updateFlags |= SC_TEMPO;
                        break;
                  case UndoOp::DeleteTempo:
                        AL::tempomap.delTempo(i->a);
                        updateFlags |= SC_TEMPO;
                        break;
                  case UndoOp::AddSig:
                        AL::sigmap.add(i->a, AL::TimeSignature(i->b, i->c));
                        updateFlags |= SC_SIG;
                        break;
                  case UndoOp::DeleteSig:
                        //printf("doRedo: UndoOp::DeleteSig. Deleting sigmap at: %d, z=%d n=%d\n", i->a, i->b, i->c);
                        AL::sigmap.del(i->a);
                        updateFlags |= SC_SIG;
                        break;
                  case UndoOp::AddCtrl:
                        i->track->addControllerVal(i->id, i->time, i->cval1);
                        break;
                  case UndoOp::RemoveCtrl:
                        i->track->removeControllerVal(i->id, i->time);
                        break;
                  case UndoOp::ModifyCtrl:
                        i->track->addControllerVal(i->id, i->time, i->cval1);
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

void Song::undoOp(UndoOp::UndoType type, int n, Track* track)
      {
      UndoOp i;
      i.type   = type;
      i.id     = n;
      i.track  = track;
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

void Song::undoOp(UndoOp::UndoType type, Event& oev, Event& nev, Part* part)
      {
      UndoOp i;
      i.type   = type;
      i.nEvent = nev;
      i.oEvent = oev;
      i.part   = part;
      addUndo(i);
      }

void Song::undoOp(UndoOp::UndoType type, Part* oPart, Part* nPart)
      {
      UndoOp i;
      i.type  = type;
      i.oPart = nPart;
      i.nPart = oPart;
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

void Song::undoOp(UndoOp::UndoType type, Track* t, int id, unsigned time, CVal nval, CVal oval)
      {
      UndoOp i;
      i.type      = type;
      i.track     = t;
      i.id        = id;
      i.time      = time;
      i.cval1      = nval;
      i.cval2      = oval;
      addUndo(i);
      }

void Song::undoOp(UndoOp::UndoType type, Track* t, const QString& s1, const QString& s2)
      {
      UndoOp i;
      i.type = type;
      i.track = t;
      i.os   = new QString(s1);
      i.ns   = new QString(s2);
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

      printf("Adding ModifyClip undo-operation: origfile=%s tmpfile=%s sf=%d ef=%d\n", changedFile, changeData, startframe, endframe);
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
// printf("doUndo1\n");
      Undo& u = undoList->back();
      for (riUndoOp i = u.rbegin(); i != u.rend(); ++i) {
// printf("  doUndo1 %s\n", i->typeName());
            switch(i->type) {
                  case UndoOp::AddTrack:
                        removeTrack1(i->track);
                        break;
                  case UndoOp::DeleteTrack:
                        insertTrack1(i->track, i->id);
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
//    non realtime context after realtime operation
//---------------------------------------------------------

void Song::doUndo3()
      {
// printf("doUndo3\n");
      Undo& u = undoList->back();
      for (riUndoOp i = u.rbegin(); i != u.rend(); ++i) {
// printf("  doUndo3 %s\n", i->typeName());
            switch(i->type) {
                  case UndoOp::AddTrack:
                        removeTrack3(i->track);
                        break;
                  case UndoOp::DeleteTrack:
                        emit trackAdded(i->track, i->id);
                        if (i->track->selected()) {
                              i->track->setSelected(false);
                              selectTrack(i->track);
                              }
                        break;
                  case UndoOp::ModifyPart:
                        if (i->oPart->track() != i->nPart->track())
                              i->nPart->track()->partListChanged();
                  case UndoOp::AddPart:
                  case UndoOp::DeletePart:
                        i->oPart->track()->partListChanged();
                        break;
                  case UndoOp::AddCtrl:
                  case UndoOp::RemoveCtrl:
                  case UndoOp::ModifyCtrl:
                        i->track->emitControllerChanged(i->id);
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
// printf("doRedo1\n");
      Undo& u = redoList->back();
      for (iUndoOp i = u.begin(); i != u.end(); ++i) {
// printf("  doRedo1 %s\n", i->typeName());
            switch(i->type) {
                  case UndoOp::AddTrack:
                        insertTrack1(i->track, i->id);
                        break;
                  case UndoOp::DeleteTrack:
                        removeTrack1(i->track);
                        break;
                  case UndoOp::ModifyClip:
                        SndFile::applyUndoFile(i->filename, i->tmpwavfile, i->startframe, i->endframe);
                        break;
                  case UndoOp::AddCtrl:
                  case UndoOp::RemoveCtrl:
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
// printf("doRedo3\n");
      Undo& u = redoList->back();
      for (iUndoOp i = u.begin(); i != u.end(); ++i) {
// printf("  doRedo3 %s\n", i->typeName());
            switch(i->type) {
                  case UndoOp::AddTrack:
                        emit trackAdded(i->track, i->id);
                        break;
                  case UndoOp::DeleteTrack:
                        removeTrack3(i->track);
                        break;
                  case UndoOp::ModifyPart:
                        if (i->oPart->track() != i->nPart->track())
                              i->nPart->track()->partListChanged();
                  case UndoOp::AddPart:
                  case UndoOp::DeletePart:
                        i->oPart->track()->partListChanged();
                        break;
                  case UndoOp::AddCtrl:
                  case UndoOp::RemoveCtrl:
                  case UndoOp::ModifyCtrl:
                        i->track->emitControllerChanged(i->id);
                        break;
                  default:
                        break;
                  }
            }
      undoList->push_back(u); // put item on undo list
      redoList->pop_back();
      dirty = true;
      }

