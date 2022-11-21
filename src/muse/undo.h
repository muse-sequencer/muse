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

#include <QString>

#include "ctrl.h"
#include "event.h"
#include "route.h"
#include "sig.h"
#include "pos.h"
#include <stdint.h>

namespace MusECore {

// Forward declarations:
class Marker;
class MidiPort;
class MidiInstrument;
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
            AddPart,  DeletePart,  MovePart, ModifyPartStart, ModifyPartLength, ModifyPartName, SelectPart,
            AddEvent, DeleteEvent, ModifyEvent, SelectEvent,
            AddAudioCtrlVal, AddAudioCtrlValStruct,
            DeleteAudioCtrlVal, ModifyAudioCtrlVal, ModifyAudioCtrlValList,
            SelectAudioCtrlVal, SetAudioCtrlPasteEraseMode, BeginAudioCtrlMoveMode, EndAudioCtrlMoveMode,
            // Add, delete and modify operate directly on the list.
            // setTempo does only if master is set, otherwise it operates on the static tempo value.
            AddTempo, DeleteTempo, ModifyTempo, SetTempo, SetStaticTempo, SetGlobalTempo, EnableMasterTrack,
            AddSig,   DeleteSig,   ModifySig,
            AddKey,   DeleteKey,   ModifyKey,
            ModifyTrackName, ModifyTrackChannel,
            SetTrackRecord, SetTrackMute, SetTrackSolo, SetTrackRecMonitor, SetTrackOff,
            MoveTrack,
            ModifyClip,
            AddMarker, DeleteMarker, ModifyMarker,
            // This one is provided separately for optimizing repeated adjustments. It is 'combo breaker' -aware.
            SetMarkerPos,
            //// For wholesale changes to the list. Preferred if multiple additions or deletions are required.
            //ModifyMarkerList,
            ModifySongLen, // a = new len, b = old len
            SetInstrument,
            DoNothing,
            ModifyMidiDivision,

            // These 'one time' operations cannot be undone (they have no 'undo' section).
            // They are meant to be called with the _noUndo argument true so that they are removed after execution.
            EnableAllAudioControllers,
            GlobalSelectAllEvents
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
                  const Track* oldTrack;
                  unsigned old_partlen_or_pos; // FIXME FINDMICHJETZT XTicks!!
                  unsigned new_partlen_or_pos;
                  unsigned old_partlen;
                  unsigned new_partlen;
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
                  Marker* oldMarker; 
                  Marker* newMarker;
                };
            struct {
                  int _oldPropValue;
                  int _newPropValue;
                };
            struct {
                  int _audioCtrlIdModify;
                  CtrlList* _eraseCtrlList;
                  CtrlList* _addCtrlList;
                  // Optional separate list to hold items to be erased, exactly like _eraseCtrlList.
                  // Useful when dragging and dropping points, this separate list can aid in storing items
                  //  that can be recovered. That could not be done if all the points were in one list.
                  CtrlList* _recoverableEraseCtrlList;
                  // Optional separate list to hold items to be added, exactly like _addCtrlList,
                  //  that were recovered from _recoverableEraseCtrlList.
                  CtrlList* _recoverableAddCtrlList;
                  // Optional list to hold items which should not be erased when dragging and dropping
                  //  while in copy mode since they are the items that are actually being copied.
                  CtrlList* _doNotEraseCtrlList;
                };
            struct {
                  int _audioCtrlID;
                  unsigned int _audioCtrlFrame;
                  unsigned int _audioNewCtrlFrame;
                  double _audioCtrlVal;
                  double _audioNewCtrlVal;
                };
            struct {
                  MidiPort* _midiPort;
                  MidiInstrument* _oldMidiInstrument;
                  MidiInstrument* _newMidiInstrument;
                };
            struct {
                  CtrlList* _audioCtrlListSelect;
                  unsigned int _audioCtrlSelectFrame;
                };
            struct {
                  int _audioCtrlIdStruct;
                  unsigned int _audioCtrlFrameStruct;
                  CtrlVal* _audioCtrlValStruct;
                };
            struct {
                  int _audioCtrlIdAddDel;
                  unsigned int _audioCtrlFrameAddDel;
                  double _audioCtrlValAddDel;
                  CtrlVal::CtrlValueFlags _audioCtrlValFlagsAddDel;
                };
            struct {
                  bool _oldAudCtrlMoveMode;
                  bool _newAudCtrlMoveMode;
                  };
            struct {
                  CtrlList::PasteEraseOptions _audioCtrlOldPasteEraseOpts;
                  CtrlList::PasteEraseOptions _audioCtrlNewPasteEraseOpts;
                };
            struct {
                  Route* routeFrom;
                  Route* routeTo;
                };
            struct {
                  QString* _oldName;
                  QString* _newName;
                };
            struct {
                  int trackno;
                };
            };


    union {
            // These are for some operations that do not use the common members above. (Saves some space.)
            struct {
                  int64_t events_offset;
                  Pos::TType events_offset_time_type;
                  bool _noEndAudioCtrlMoveMode;
            };
      };
      Event oEvent;
      Event nEvent;
      bool selected;
      bool selected_old;
      bool doCtrls;
      bool doClones;
      const Track* track;
      const Part* part;

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
      UndoOp(UndoType type, const Part* part, unsigned int old_len_or_pos, unsigned int new_len_or_pos, Pos::TType new_time_type = Pos::TICKS,
             const Track* oTrack = 0, const Track* nTrack = 0, bool noUndo = false);
      UndoOp(UndoType type, const Part* part, unsigned int old_pos, unsigned int new_pos,
             unsigned int old_len, unsigned int new_len, int64_t events_offset,
             Pos::TType new_time_type = Pos::TICKS, bool noUndo = false);
      UndoOp(UndoType type, const Part* part, unsigned int old_len, unsigned int new_len, int64_t events_offset,
             Pos::TType new_time_type = Pos::TICKS, bool noUndo = false);
      UndoOp(UndoType type, const Event& nev, const Event& oev, const Part* part, bool doCtrls, bool doClones, bool noUndo = false);
      UndoOp(UndoType type, const Event& nev, const Part* part, bool, bool, bool noUndo = false);
      UndoOp(UndoType type, const Event& changedEvent, const QString& changeData, int startframe, int endframe, bool noUndo = false);
      UndoOp(UndoType type, const Marker& oldMarker, const Marker& newMarker, bool noUndo = false);
      UndoOp(UndoType type, const Marker& marker, bool noUndo = false);
      UndoOp(UndoType type, const Marker& marker, unsigned int new_pos, Pos::TType new_time_type, bool noUndo = false);
      // Takes ownership of the old list (deletes it).
      //UndoOp(UndoType type, MarkerList** oldMarkerList, MarkerList* newMarkerList, bool noUndo = false);

      UndoOp(UndoType type, const Track* track, const QString& old_name, const QString& new_name, bool noUndo = false);
      // Because of C++ ambiguity complaints, these arguments are in a funny order.
      // It seems our CtrlVal(double) constructor can be interpreted as CtrlVal(unsigned int) !
      UndoOp(UndoType type, int ctrlID, unsigned int frame, const CtrlVal& cv, const Track* track, bool noUndo = false);
      UndoOp(UndoType type, const Track* track, int ctrlID, CtrlList* eraseCtrlList, CtrlList* addCtrlList,
             CtrlList* recoverableEraseCtrlList = nullptr, CtrlList* recoverableAddCtrlList = nullptr,
             CtrlList* doNotEraseCtrlList = nullptr, bool noEndAudioCtrlMoveMode = false, bool noUndo = false);

      // At least four of these arguments must be supplied even if less than four are used,
      //  to avoid ambiguity with the route_from/route_to version because class Route has
      //  a constructor that takes a track pointer and a constructor that takes an integer !!!
      UndoOp(UndoType type, const Track* track, double a, double b, double c, double d = 0.0, double e = 0.0, bool noUndo = false);


      UndoOp(UndoType type, CtrlList* ctrlList, unsigned int frame, bool oldSelected, bool newSelected, bool noUndo = false);
      UndoOp(UndoType type, CtrlList::PasteEraseOptions newOpts, bool noUndo = false);

      UndoOp(UndoType type, int tick, const MusECore::TimeSignature old_sig, const MusECore::TimeSignature new_sig, bool noUndo = false);
      UndoOp(UndoType type, const Route& route_from, const Route& route_to, bool noUndo = false);
      UndoOp(UndoType type, MidiPort* mp, MidiInstrument* instr, bool noUndo = false);
      UndoOp(UndoType type, bool noUndo = false);
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
      
      void push_front(const UndoOp& op);
      void push_back(const UndoOp& op);
      void insert(iterator position, const_iterator first, const_iterator last);
      void insert(iterator position, const UndoOp& op);
      void insert (iterator position, size_type n, const UndoOp& op);
      iterator deleteAndErase(iterator);
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
typedef UndoList::const_iterator ciUndo;
typedef UndoList::const_reverse_iterator criUndo;

} // namespace MusECore

#endif // __UNDO_H__
