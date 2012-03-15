//  MusE
//  Linux Music Editor
//    $Id: noteinfo.h,v 1.3 2004/01/09 17:12:54 wschweer Exp $
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

#ifndef __NOTE_INFO_H__
#define __NOTE_INFO_H__

#include <QToolBar>

namespace Awl {
      class PosEdit;
      //class PitchEdit;
      };

///class PosEdit;
namespace MusECore {
class Pos;
}

namespace MusEGui {

class PitchEdit;
class SpinBox;

//---------------------------------------------------------
//   NoteInfo
//---------------------------------------------------------

class NoteInfo : public QToolBar {
      Q_OBJECT
      
      ///PosEdit* selTime;
      Awl::PosEdit* selTime;
      SpinBox* selLen;
      PitchEdit* selPitch;
      SpinBox* selVelOn;
      SpinBox* selVelOff;
      bool deltaMode;

   public:
      enum ValType {VAL_TIME, VAL_LEN, VAL_VELON, VAL_VELOFF, VAL_PITCH };
      //NoteInfo(QMainWindow* parent);
      NoteInfo(QWidget* parent = 0);
      void setValues(unsigned, int, int, int, int);
      void setDeltaMode(bool);

   private slots:
      void lenChanged(int);
      void velOnChanged(int);
      void velOffChanged(int);
      void pitchChanged(int);
      void timeChanged(const MusECore::Pos&);

   public slots:
      void setValue(ValType, int);

   signals:
      void valueChanged(MusEGui::NoteInfo::ValType, int);
      void returnPressed();
      void escapePressed();
      };

} // namespace MusEGui

#endif

