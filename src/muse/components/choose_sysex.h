//=========================================================
//  MusE
//  Linux Music Editor
//  choose_sysex.h
//  (C) Copyright 2014 Tim E. Real (terminator356 at users.sourceforge.net)
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

#ifndef __CHOOSE_SYSEX_H__
#define __CHOOSE_SYSEX_H__

#include "ui_choose_sysex_base.h"

class QWidget;
class QDialog;
class QListWidgetItem;

namespace MusECore {
  class MidiInstrument;
  struct SysEx;
}

namespace MusEGui {

//---------------------------------------------------------
//   ChooseSysexDialog
//---------------------------------------------------------

class ChooseSysexDialog : public QDialog, public Ui::ChooseSysexBase {
      Q_OBJECT

      MusECore::MidiInstrument* _instr;
      MusECore::SysEx* _sysex;

   private slots:
      virtual void accept();
      void sysexChanged(QListWidgetItem*, QListWidgetItem*);

   public:
      ChooseSysexDialog(QWidget* parent = nullptr, MusECore::MidiInstrument* instr = nullptr);
      MusECore::SysEx* sysex() { return _sysex; }
      };

} // namespace MusEGui

#endif

