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

#ifndef __GATETIME_H__
#define __GATETIME_H__

#include "ui_gatetime.h"
#include "midicmd.h"

//---------------------------------------------------------
//   GateTime
//---------------------------------------------------------

class GateTime : public MidiCmdDialog {
      Q_OBJECT
      
      Ui::GateTimeBase gt;
      int _rateVal;
      int _offsetVal;

   protected slots:
      void accept();

   public:
      GateTime(QWidget* parent = 0);
      int rateVal() const   { return _rateVal; }
      int offsetVal() const { return _offsetVal; }
      };

//---------------------------------------------------------
//   ModifyGateTimeCmd
//---------------------------------------------------------

class ModifyGateTimeCmd : public MidiCmd
      {
      GateTime* dialog;
      virtual MidiCmdDialog* guiDialog();
      virtual void process(CItemList* items);

   public:
      ModifyGateTimeCmd(MidiEditor* e);
      };

#endif

