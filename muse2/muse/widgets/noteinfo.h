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
      };

namespace MusECore {
class Pos;
}

namespace MusEGui {

class PitchEdit;
class SpinBox;
class PixmapButton;


//---------------------------------------------------------
//   NoteInfo
//---------------------------------------------------------

class NoteInfo : public QToolBar {
      Q_OBJECT
      
      Awl::PosEdit* selTime;
      SpinBox* selLen;
      PitchEdit* selPitch;
      SpinBox* selVelOn;
      SpinBox* selVelOff;
      PixmapButton* deltaButton;
      bool _returnMode;
      bool deltaMode;
      bool _enabled;
      void set_mode();
      
      
   public:
      enum ValType {VAL_TIME, VAL_LEN, VAL_VELON, VAL_VELOFF, VAL_PITCH };
      NoteInfo(QWidget* parent = 0);
      void setValues(unsigned, int, int, int, int);
      void setDeltaMode(bool);
      bool isEnabled() const { return _enabled; }
      void setReturnMode(bool v);
      bool returnMode() const    { return _returnMode; }
      
   private slots:
      void lenChanged(int);
      void velOnChanged(int);
      void velOffChanged(int);
      void pitchChanged(int);
      void timeChanged(const MusECore::Pos&);
      void deltaModeClicked(bool);

   public slots:
      void setValue(ValType, int);
      void setEnabled(bool);

   signals:
      void valueChanged(MusEGui::NoteInfo::ValType, int);
      void returnPressed();
      void escapePressed();
      void deltaModeChanged(bool);
      };

} // namespace MusEGui

#endif

