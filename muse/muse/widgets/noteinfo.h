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

#ifndef __NOTE_INFO_H__
#define __NOTE_INFO_H__

namespace Awl {
      class PosEdit;
      class PitchEdit;
      };
namespace AL {
      class Pos;
      }

using AL::Pos;

//---------------------------------------------------------
//   NoteInfo
//---------------------------------------------------------

class NoteInfo : public QToolBar {
      Awl::PosEdit* selTime;
      QSpinBox* selLen;
      Awl::PitchEdit* selPitch;
      QSpinBox* selVelOn;
      QSpinBox* selVelOff;
      bool deltaMode;

      Q_OBJECT

   public:
      enum ValType {VAL_TIME, VAL_LEN, VAL_VELON, VAL_VELOFF, VAL_PITCH };
      NoteInfo(QMainWindow* parent);
      void setValues(unsigned, int, int, int, int);
      void setDeltaMode(bool);

   private slots:
      void lenChanged(int);
      void velOnChanged(int);
      void velOffChanged(int);
      void pitchChanged(int);
      void timeChanged(const Pos&);

   public slots:
      void setValue(ValType, int);

   signals:
      void valueChanged(NoteInfo::ValType, int);
      };
#endif

