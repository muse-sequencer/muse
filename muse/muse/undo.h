//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: undo.h,v 1.6.2.5 2009/05/24 21:43:44 terminator356 Exp $
//
//  (C) Copyright 1999/2000 Werner Schweer (ws@seh.de)
//=========================================================

#ifndef __UNDO_H__
#define __UNDO_H__

#include <list>
#include <qstring.h>
#include "event.h"

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
            AddTrack, DeleteTrack, ModifyTrack,
            AddPart,  DeletePart,  ModifyPart,
            AddEvent, DeleteEvent, ModifyEvent,
            AddTempo, DeleteTempo,
            AddSig,   DeleteSig,
            SwapTrack,
            ModifyClip
            };
      UndoType type;

      union {
            struct {
                  int a;
                  int b;
                  int c;
                  };
            struct {
                  Track* oTrack;
                  Track* nTrack;
                  int trackno;
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
            };
      Event oEvent;
      Event nEvent;
      bool doCtrls;
      bool doClones;
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
      void clearDelete();
      };

typedef UndoList::iterator iUndo;


#endif // __UNDO_H__
