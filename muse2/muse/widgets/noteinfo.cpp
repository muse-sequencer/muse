//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: noteinfo.cpp,v 1.4.2.1 2008/08/18 00:15:26 terminator356 Exp $
//  (C) Copyright 1999 Werner Schweer (ws@seh.de)
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

#include <QLabel>

#include "config.h"
#include "noteinfo.h"
#include "awl/posedit.h"
//#include "awl/pitchedit.h"
#include "song.h"
#include "globals.h"
///#include "posedit.h"
#include "pitchedit.h"
#include "icons.h"
#include "pixmap_button.h"

namespace MusEGui {

//---------------------------------------------------
//    NoteInfo
//    ToolBar
//    Start, Length, Note, Velo on, Velo off
//---------------------------------------------------

//NoteInfo::NoteInfo(QMainWindow* parent)
NoteInfo::NoteInfo(QWidget* parent)
   : QToolBar(tr("Note Info"), parent)
      {
      setObjectName("Note Info");
      _enabled = true;
      _returnMode = false;
      deltaMode = false;

      deltaButton = new PixmapButton(deltaOnIcon, deltaOffIcon, 2);
      deltaButton->setFocusPolicy(Qt::NoFocus);
      deltaButton->setCheckable(true);
      deltaButton->setToolTip(tr("delta/absolute mode"));
      addWidget(deltaButton);
      
      QLabel* label = new QLabel(tr("Start"));
      label->setAlignment(Qt::AlignRight|Qt::AlignVCenter);
      label->setIndent(3);
      addWidget(label);
      
      selTime = new Awl::PosEdit;
      selTime->setFocusPolicy(Qt::StrongFocus);
      selTime->setObjectName("Start");
      
      addWidget(selTime);

      label = new QLabel(tr("Len"));
      label->setAlignment(Qt::AlignRight|Qt::AlignVCenter);
      label->setIndent(3);
      addWidget(label);
      selLen = new SpinBox();
      selLen->setFocusPolicy(Qt::StrongFocus);
      selLen->setRange(0, 100000);
      selLen->setSingleStep(1);
      addWidget(selLen);

      label = new QLabel(tr("Pitch"));
      label->setAlignment(Qt::AlignRight|Qt::AlignVCenter);
      label->setIndent(3);
      addWidget(label);
      selPitch = new PitchEdit;
      selPitch->setFocusPolicy(Qt::StrongFocus);
      selPitch->setDeltaMode(deltaMode);
      addWidget(selPitch);

      label = new QLabel(tr("Velo On"));
      label->setAlignment(Qt::AlignRight|Qt::AlignVCenter);
      label->setIndent(3);
      addWidget(label);
      selVelOn = new SpinBox();
      selVelOn->setFocusPolicy(Qt::StrongFocus);
      selVelOn->setRange(0, 127);
      selVelOn->setSingleStep(1);
      addWidget(selVelOn);

      label = new QLabel(tr("Velo Off"));
      label->setAlignment(Qt::AlignRight|Qt::AlignVCenter);
      label->setIndent(3);
      addWidget(label);
      selVelOff = new SpinBox();
      selVelOff->setFocusPolicy(Qt::StrongFocus);
      selVelOff->setRange(0, 127);
      selVelOff->setSingleStep(1);
      addWidget(selVelOff);

      connect(selLen,     SIGNAL(valueChanged(int)), SLOT(lenChanged(int)));
      connect(selPitch,   SIGNAL(valueChanged(int)), SLOT(pitchChanged(int)));
      connect(selVelOn,   SIGNAL(valueChanged(int)), SLOT(velOnChanged(int)));
      connect(selVelOff,  SIGNAL(valueChanged(int)), SLOT(velOffChanged(int)));
      connect(selTime,    SIGNAL(valueChanged(const MusECore::Pos&)), SLOT(timeChanged(const MusECore::Pos&)));
      
      connect(selLen,     SIGNAL(returnPressed()), SIGNAL(returnPressed()));
      connect(selPitch,   SIGNAL(returnPressed()), SIGNAL(returnPressed()));
      connect(selVelOn,   SIGNAL(returnPressed()), SIGNAL(returnPressed()));
      connect(selVelOff,  SIGNAL(returnPressed()), SIGNAL(returnPressed()));
      connect(selTime,    SIGNAL(returnPressed()), SIGNAL(returnPressed()));
      
      connect(selLen,     SIGNAL(escapePressed()), SIGNAL(escapePressed()));
      connect(selPitch,   SIGNAL(escapePressed()), SIGNAL(escapePressed()));
      connect(selVelOn,   SIGNAL(escapePressed()), SIGNAL(escapePressed()));
      connect(selVelOff,  SIGNAL(escapePressed()), SIGNAL(escapePressed()));
      connect(selTime,    SIGNAL(escapePressed()), SIGNAL(escapePressed()));
      
      connect(deltaButton, SIGNAL(clicked(bool)), SLOT(deltaModeClicked(bool)));
      }

//---------------------------------------------------------
//   setEnabled
//---------------------------------------------------------

void NoteInfo::setEnabled(bool val)
{
  _enabled = val;
  selLen->setEnabled(val);
  selPitch->setEnabled(val);
  selVelOn->setEnabled(val);
  selVelOff->setEnabled(val);
  selTime->setEnabled(val);
}
      
//---------------------------------------------------------
//   set_mode
//---------------------------------------------------------

void NoteInfo::set_mode()
      {
      blockSignals(true);
      selPitch->setDeltaMode(deltaMode);
      if (deltaMode) {
            selLen->setRange(-100000, 100000);
            selVelOn->setRange(-127, 127);
            selVelOff->setRange(-127, 127);
            }
      else {
            selLen->setRange(0, 100000);
            selVelOn->setRange(0, 127);
            selVelOff->setRange(0, 127);
            }
      blockSignals(false);
      }

//---------------------------------------------------------
//   setReturnMode
//---------------------------------------------------------

void NoteInfo::setReturnMode(bool v)
{
  _returnMode = v;
  selTime->setReturnMode(_returnMode);
  selLen->setReturnMode(_returnMode);
  selPitch->setReturnMode(_returnMode);
  selVelOn->setReturnMode(_returnMode);
  selVelOff->setReturnMode(_returnMode);
}
      
//---------------------------------------------------------
//   setDeltaMode
//---------------------------------------------------------

void NoteInfo::setDeltaMode(bool val)
      {
      if(val == deltaMode)
        return;
      deltaMode = val;
      deltaButton->setChecked(deltaMode);
      set_mode();
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
//   setDeltaMode
//---------------------------------------------------------

void NoteInfo::deltaModeClicked(bool val)
{
  if(val == deltaMode)
    return;
  deltaMode = val;
  set_mode();
  emit deltaModeChanged(deltaMode);
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
      // PosEdit will take care of optimizations. It must check whether actual values dependent on tempo or sig changed...
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

void NoteInfo::timeChanged(const MusECore::Pos& pos)
      {
      if (!signalsBlocked())
            emit valueChanged(VAL_TIME, pos.tick());
      }

} // namespace MusEGui
