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
#include "route.h"

class QString;

namespace AL {
struct TimeSignature;
}

namespace MusECore {

class Track;
class Part;

extern std::list<QString> temporaryWavFiles; //!< Used for storing all tmp-files, for cleanup on shutdown
//---------------------------------------------------------
//   UndoOp
//---------------------------------------------------------

struct UndoOp {
      enum UndoType {
            AddRoute, DeleteRoute,
            AddTrack, DeleteTrack,
            AddPart,  DeletePart,  MovePart, ModifyPartLength, ModifyPartName, SelectPart,
            AddEvent, DeleteEvent, ModifyEvent, SelectEvent,
            AddTempo, DeleteTempo, ModifyTempo, SetGlobalTempo, 
            AddSig,   DeleteSig,   ModifySig,
            AddKey,   DeleteKey,   ModifyKey,
            ModifyTrackName, ModifyTrackChannel,
            MoveTrack,
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
                  int d;
                  int e;
                  };
            struct {
                  const Part* part;
                  unsigned old_partlen_or_pos; // FIXME FINDMICHJETZT XTicks!!
                  unsigned new_partlen_or_pos;
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
                  const Track* _propertyTrack;
                  int _oldPropValue;
                  int _newPropValue;
                };
            };

      char* _oldName;
      char* _newName;
      Event oEvent;
      Event nEvent;
      bool selected;
      bool selected_old;
      bool doCtrls;
      bool doClones;
      const Track* track;
      const Track* oldTrack;
      int trackno;
      Route routeFrom;
      Route routeTo;
      
      const char* typeName();
      void dump();
      
      UndoOp();
      UndoOp(UndoType type, int a, int b, int c=0);
      UndoOp(UndoType type, int n, const Track* track);
      UndoOp(UndoType type_, const Part* part_);
      UndoOp(UndoType type, const Part* part, const char* old_name, const char* new_name);
      UndoOp(UndoType type, const Part* part, bool selected, bool selected_old);
      UndoOp(UndoType type, const Part* part, int old_len_or_pos, int new_len_or_pos, Pos::TType new_time_type = Pos::TICKS, const Track* oTrack = 0, const Track* nTrack = 0);
      UndoOp(UndoType type, const Event& nev, const Event& oev, const Part* part, bool doCtrls, bool doClones);
      UndoOp(UndoType type, const Event& nev, const Part* part, bool, bool);
      UndoOp(UndoType type, const char* changedFile, const char* changeData, int startframe, int endframe);
      UndoOp(UndoType type, Marker* copyMarker, Marker* realMarker);
      UndoOp(UndoType type, const Track* track, const char* old_name, const char* new_name);
      UndoOp(UndoType type, const Track* track, int old_chan, int new_chan);
      UndoOp(UndoType type, int tick, const AL::TimeSignature old_sig, const AL::TimeSignature new_sig);
      UndoOp(UndoType type, const Route& route_from, const Route& route_to);
};

class Undo : public std::list<UndoOp> {
   public:
      Undo() : std::list<UndoOp>() { combobreaker=false; }
      Undo(const Undo& other) : std::list<UndoOp>(other) { this->combobreaker=other.combobreaker; }
      Undo& operator=(const Undo& other) { std::list<UndoOp>::operator=(other); this->combobreaker=other.combobreaker; return *this;}

      bool empty() const;
      
      
      /** if set, forbid merging (below).
       *  Defaults to false */
      bool combobreaker; 
      
      /** is possible, merges itself and other by appending
       *  all contents of other at this->end().
       *  returns true if merged, false otherwise.
       *  in case of success, the caller has to ensure that
       *  other is deleted from the UndoList. */
      bool merge_combo(const Undo& other);
      
      void push_back(const UndoOp& op);
      void insert(iterator position, const_iterator first, const_iterator last);
      void insert(iterator position, const UndoOp& op);
      void insert (iterator position, size_type n, const UndoOp& op);
};

typedef Undo::iterator iUndoOp;
typedef Undo::reverse_iterator riUndoOp;
typedef Undo::const_iterator ciUndoOp;
typedef Undo::const_reverse_iterator criUndoOp;

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
