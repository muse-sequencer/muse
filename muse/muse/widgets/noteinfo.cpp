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

#include "noteinfo.h"
#include "song.h"
#include "globals.h"
#include "awl/posedit.h"
#include "awl/pitchedit.h"

//---------------------------------------------------
//    NoteInfo
//    ToolBar
//    Start, Länge, Note, Velo an, Velo aus, Kanal
//---------------------------------------------------

NoteInfo::NoteInfo(QMainWindow* parent)
   : QToolBar(tr("Note Info"), parent)
      {
      deltaMode = false;

      QLabel* label = new QLabel(tr("Start"));
      label->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
      label->setIndent(3);
      addWidget(label);

      selTime = new Awl::PosEdit;
      selTime->setFixedHeight(24);
      addWidget(selTime);

      label = new QLabel(tr("Len"), this);
      label->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
      label->setIndent(3);
      addWidget(label);

      selLen = new QSpinBox(this);
      selLen->setRange(0, 100000);
      selLen->setFixedHeight(24);
      addWidget(selLen);

      label = new QLabel(tr("Pitch"), this);
      label->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
      label->setIndent(3);
      addWidget(label);
      selPitch = new Awl::PitchEdit(this);
      selPitch->setFixedHeight(24);
      addWidget(selPitch);

      label = new QLabel(tr("Velo On"), this);
      label->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
      label->setIndent(3);
      addWidget(label);
      selVelOn = new QSpinBox(this);
      selVelOn->setRange(0, 127);
      selVelOn->setFixedHeight(24);
      addWidget(selVelOn);

      label = new QLabel(tr("Velo Off"), this);
      label->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
      label->setIndent(3);
      addWidget(label);
      selVelOff = new QSpinBox(this);
      selVelOff->setRange(0, 127);
      selVelOff->setFixedHeight(24);
      addWidget(selVelOff);

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

