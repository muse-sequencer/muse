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

#ifndef __UNDO_H__
#define __UNDO_H__

#include "event.h"
#include "ctrl.h"

class Track;
class TEvent;
class SigEvent;
class Part;

extern std::list<QString> temporaryWavFiles; //!< Used for storing all tmp-files, for cleanup on shutdown

//---------------------------------------------------------
//   UndoOp
//---------------------------------------------------------

struct UndoOp {
      enum UndoType {
            AddTrack, DeleteTrack, RenameTrack,
            AddPart,  DeletePart,  ModifyPart,
            AddEvent, DeleteEvent, ModifyEvent,
            AddTempo, DeleteTempo,
            AddSig,   DeleteSig,
            SwapTrack,
            ModifyClip,
            AddCtrl, RemoveCtrl, ModifyCtrl
            };
      UndoType type;

      union {
            struct {
                  int a;
                  int b;
                  int c;
                  };
            struct {
                  Part* oPart;
                  Part* nPart;
                  };
            struct {
                  Part* part;
                  };
            struct {
                  SigEvent* nSignature;
                  SigEvent* oSignature;
                  };
            struct {
                  int startframe; //!< Start frame of changed data
                  int endframe;   //!< End frame of changed data
                  const char* filename; //!< The file that is changed
                  const char* tmpwavfile; //!< The file with the changed data
                  };
            struct {
                  Track* track;
                  int id;
                  unsigned time;
                  CVal cval1, cval2;
                  QString* os;
                  QString* ns;
                  };
            };
      Event oEvent;
      Event nEvent;
      const char* typeName();
      void dump();
      };

class Undo : public std::list<UndoOp> {
      void undoOp(UndoOp::UndoType, int data);
      };

typedef Undo::iterator iUndoOp;
typedef Undo::reverse_iterator riUndoOp;

class UndoList : public std::list<Undo> {
   public:
      };

typedef UndoList::iterator iUndo;


#endif // __UNDO_H__
