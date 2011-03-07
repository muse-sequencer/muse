//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: noteinfo.cpp,v 1.4.2.1 2008/08/18 00:15:26 terminator356 Exp $
//  (C) Copyright 1999 Werner Schweer (ws@seh.de)
//=========================================================

#include "config.h"

#include "noteinfo.h"
#include <qlayout.h>
#include <qlabel.h>
#include "song.h"
#include "globals.h"
#include "posedit.h"
#include "pitchedit.h"

//---------------------------------------------------
//    NoteInfo
//    ToolBar
//    Start, Lï¿½nge, Note, Velo an, Velo aus, Kanal
//---------------------------------------------------

NoteInfo::NoteInfo(QMainWindow* parent)
   : QToolBar(tr("Note Info"), parent)
      {
      deltaMode = false;

      QLabel* label = new QLabel(tr("Start"), this, "Start");
      label->setAlignment(AlignRight|AlignVCenter);
      label->setIndent(3);
      selTime = new PosEdit(this, "Start");

      label = new QLabel(tr("Len"), this, "Len");
      label->setAlignment(AlignRight|AlignVCenter);
      label->setIndent(3);
      selLen = new QSpinBox(0, 100000, 1, this);

      label = new QLabel(tr("Pitch"), this, "Pitch");
      label->setAlignment(AlignRight|AlignVCenter);
      label->setIndent(3);
      selPitch = new PitchEdit(this, "selPitch");

      label = new QLabel(tr("Velo On"), this, "Velocity On");
      label->setAlignment(AlignRight|AlignVCenter);
      label->setIndent(3);
      selVelOn = new QSpinBox(0, 127, 1, this);

      label = new QLabel(tr("Velo Off"), this, "Velocity Off");
      label->setAlignment(AlignRight|AlignVCenter);
      label->setIndent(3);
      selVelOff = new QSpinBox(0, 127, 1, this);

      connect(selLen,     SIGNAL(valueChanged(int)), SLOT(lenChanged(int)));
      connect(selPitch,   SIGNAL(valueChanged(int)), SLOT(pitchChanged(int)));
      connect(selVelOn,   SIGNAL(valueChanged(int)), SLOT(velOnChanged(int)));
      connect(selVelOff,  SIGNAL(valueChanged(int)), SLOT(velOffChanged(int)));
      connect(selTime,    SIGNAL(valueChanged(const Pos&)), SLOT(timeChanged(const Pos&)));
      }

//---------------------------------------------------------
//   setDeltaMode
//---------------------------------------------------------

void NoteInfo::setDeltaMode(bool val)
      {
      deltaMode = val;
      selPitch->setDeltaMode(val);
      if (val) {
            selLen->setRange(-100000, 100000);
            selVelOn->setRange(-127, 127);
            selVelOff->setRange(-127, 127);
            }
      else {
            selLen->setRange(0, 100000);
            selVelOn->setRange(0, 127);
            selVelOff->setRange(0, 127);
            }
      }

//---------------------------------------------------------
//   lenChanged
//---------------------------------------------------------

void NoteInfo::lenChanged(int val)
      {
      if (!signalsBlocked())
            emit valueChanged(VAL_LEN, val);
      }

//---------------------------------------------------------
//   velOnChanged
//---------------------------------------------------------

void NoteInfo::velOnChanged(int val)
      {
      if (!signalsBlocked())
            emit valueChanged(VAL_VELON, val);
      }

//---------------------------------------------------------
//   velOffChanged
//---------------------------------------------------------

void NoteInfo::velOffChanged(int val)
      {
      if (!signalsBlocked())
            emit valueChanged(VAL_VELOFF, val);
      }

//---------------------------------------------------------
//   pitchChanged
//---------------------------------------------------------

void NoteInfo::pitchChanged(int val)
      {
      if (!signalsBlocked())
            emit valueChanged(VAL_PITCH, val);
      }

//---------------------------------------------------------
//   setValue
//---------------------------------------------------------

void NoteInfo::setValue(ValType type, int val)
      {
      blockSignals(true);
      switch(type) {
            case VAL_TIME:
                  selTime->setValue(val);
                  break;
            case VAL_LEN:
                  selLen->setValue(val);
                  break;
            case VAL_VELON:
                  selVelOn->setValue(val);
                  break;
            case VAL_VELOFF:
                  selVelOff->setValue(val);
                  break;
            case VAL_PITCH:
                  selPitch->setValue(val);
                  break;
            }
      blockSignals(false);
      }

//---------------------------------------------------------
//   setValue
//---------------------------------------------------------

void NoteInfo::setValues(unsigned tick, int val2, int val3, int val4,
   int val5)
      {
      blockSignals(true);
      if (selTime->pos().tick() != tick)
            selTime->setValue(tick);
      if (selLen->value() != val2)
            selLen->setValue(val2);
      if (selPitch->value() != val3)
            selPitch->setValue(val3);
      if (selVelOn->value() != val4)
            selVelOn->setValue(val4);
      if (selVelOff->value() != val5)
            selVelOff->setValue(val5);
      blockSignals(false);
      }

//---------------------------------------------------------
//   timeChanged
//---------------------------------------------------------

void NoteInfo::timeChanged(const Pos& pos)
      {
      if (!signalsBlocked())
            emit valueChanged(VAL_TIME, pos.tick());
      }

