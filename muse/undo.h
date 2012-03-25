//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: undo.h,v 1.6.2.5 2009/05/24 21:43:44 terminator356 Exp $
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

#ifndef __UNDO_H__
#define __UNDO_H__

#include <list>

#include "event.h"
#include "marker/marker.h"

class QString;

namespace MusECore {

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
            AddTrack, DeleteTrack,
            AddPart,  DeletePart,  ModifyPart,
            AddEvent, DeleteEvent, ModifyEvent,
            AddTempo, DeleteTempo,
            AddSig,   DeleteSig,
            AddKey,   DeleteKey,
            ModifyTrackName, ModifyTrackChannel,
            SwapTrack,
            ModifyClip,
            ModifyMarker,
            ModifySongLen, // a = new len, b = old len
            DoNothing
            };
      UndoType type;

      union {
            struct {
                  int a;
                  int b;
                  int c;
                  };
            struct {
                  Track* track;
                  int trackno;
                  };
            struct {
                  Part* oPart;
                  Part* nPart;
                  };
            struct {
                  Part* part; // this part is only relevant for EVENT operations, NOT for part ops!
                  };
            struct {
                  int channel;
                  int ctrl;
                  int oVal;
                  int nVal;
                  };
            struct {
                  int startframe; //!< Start frame of changed data
                  int endframe;   //!< End frame of changed data
                  const char* filename; //!< The file that is changed
                  const char* tmpwavfile; //!< The file with the changed data
                  };
            struct {
                  Marker* realMarker;
                  Marker* copyMarker;
                };
            struct {
                  Track* _renamedTrack;
                  char* _oldName;
                  char* _newName;
                };
            struct {
                  Track* _propertyTrack;
                  int _oldPropValue;
                  int _newPropValue;
                };
            };
      Event oEvent;
      Event nEvent;
      bool doCtrls;
      bool doClones;
      
      const char* typeName();
      void dump();
      
      UndoOp();
      UndoOp(UndoType type, int a, int b, int c=0);
      UndoOp(UndoType type, int n, Track* track);
      UndoOp(UndoType type, Part* part);
      UndoOp(UndoType type, Event& oev, Event& nev, Part* part, bool doCtrls, bool doClones);
      UndoOp(UndoType type, Event& nev, Part* part, bool doCtrls, bool doClones);
      UndoOp(UndoType type, Part* oPart, Part* nPart, bool doCtrls, bool doClones);
      UndoOp(UndoType type, int c, int ctrl, int ov, int nv);
      UndoOp(UndoType type, const char* changedFile, const char* changeData, int startframe, int endframe);
      UndoOp(UndoType type, Marker* copyMarker, Marker* realMarker);
      UndoOp(UndoType type, Track* track, const char* old_name, const char* new_name);
      UndoOp(UndoType type, Track* track, int old_chan, int new_chan);
      UndoOp(UndoType type);
      };

class Undo : public std::list<UndoOp> {
   public:
      bool empty() const;
};

typedef Undo::iterator iUndoOp;
typedef Undo::reverse_iterator riUndoOp;

class UndoList : public std::list<Undo> {
   protected:
      bool isUndo;
   public:
      void clearDelete();
      UndoList(bool _isUndo) : std::list<Undo>() { isUndo=_isUndo; }
      };

typedef UndoList::iterator iUndo;
typedef UndoList::reverse_iterator riUndo;

} // namespace MusECore

#endif // __UNDO_H__
