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
class CtrlListList;
class CtrlList;
struct CtrlVal;

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
            AddAudioCtrlVal, DeleteAudioCtrlVal, ModifyAudioCtrlVal, ModifyAudioCtrlValList,
            AddTempo, DeleteTempo, ModifyTempo, SetGlobalTempo, 
            AddSig,   DeleteSig,   ModifySig,
            AddKey,   DeleteKey,   ModifyKey,
            ModifyTrackName, ModifyTrackChannel,
            SetTrackRecord, SetTrackMute, SetTrackSolo,
            MoveTrack,
            ModifyClip,
            ModifyMarker,
            ModifySongLen, // a = new len, b = old len
            DoNothing,
            
            // These operation cannot be undone. They are 'one time' operations, removed after execution.
            EnableAllAudioControllers
            };
      UndoType type;

      union {
            //U() { memset( this, 0, sizeof( U ) ); }
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
                  QString* tmpwavfile; //!< The file with the changed data
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
            struct {
                  CtrlListList* _ctrlListList;
                  CtrlList* _eraseCtrlList;
                  CtrlList* _addCtrlList;
                };
            struct {
                  int _audioCtrlID;
                  int _audioCtrlFrame;
                  int _audioNewCtrlFrame;
                  double _audioCtrlVal;
                  double _audioNewCtrlVal;
                };
            };

      QString* _oldName;
      QString* _newName;
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
      
      // If _noUndo is set, the operation cannot be undone. It is a 'one time' operation, removed after execution.
      // It allows mixed undoable and non-undoable operations in one list, all executed in one RT cycle.
      bool _noUndo;
      
      const char* typeName();
      void dump();
      
      UndoOp();
      // NOTE: In these constructors, if noUndo is set, the operation cannot be undone. It is a 'one time' operation, removed after execution.
      //       It allows mixed undoable and non-undoable operations in one list, all executed in one RT cycle.
      UndoOp(UndoType type, int a, int b, int c=0, bool noUndo = false);
      UndoOp(UndoType type, int n, const Track* track, bool noUndo = false);
      UndoOp(UndoType type, const Part* part, bool noUndo = false);
      UndoOp(UndoType type, const Part* part, const QString& old_name, const QString& new_name, bool noUndo = false);
      UndoOp(UndoType type, const Part* part, bool selected, bool selected_old, bool noUndo = false);
      UndoOp(UndoType type, const Part* part, int old_len_or_pos, int new_len_or_pos, Pos::TType new_time_type = Pos::TICKS, const Track* oTrack = 0, const Track* nTrack = 0, bool noUndo = false);
      UndoOp(UndoType type, const Event& nev, const Event& oev, const Part* part, bool doCtrls, bool doClones, bool noUndo = false);
      UndoOp(UndoType type, const Event& nev, const Part* part, bool, bool, bool noUndo = false);
      UndoOp(UndoType type, const Event& changedEvent, const QString& changeData, int startframe, int endframe, bool noUndo = false);
      UndoOp(UndoType type, Marker* copyMarker, Marker* realMarker, bool noUndo = false);
      UndoOp(UndoType type, const Track* track, const QString& old_name, const QString& new_name, bool noUndo = false);
      UndoOp(UndoType type, const Track* track, int old_chan, int new_chan, bool noUndo = false);
      //UndoOp(UndoType type, const Track* track, int ctrlID, int frame, bool noUndo = false); // Same as above.
      UndoOp(UndoType type, const Track* track, int ctrlID, int oldFrame, int newFrame, double oldValue, double newValue, bool noUndo = false);
      UndoOp(UndoType type, const Track* track, int ctrlID, int frame, double value, bool noUndo = false);
      UndoOp(UndoType type, const Track* track, bool value, bool noUndo = false);
      UndoOp(UndoType type, CtrlListList* ctrl_ll, CtrlList* eraseCtrlList, CtrlList* addCtrlList, bool noUndo = false);
      UndoOp(UndoType type, int tick, const AL::TimeSignature old_sig, const AL::TimeSignature new_sig, bool noUndo = false);
      UndoOp(UndoType type, const Route& route_from, const Route& route_to, bool noUndo = false);
      UndoOp(UndoType type);
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
