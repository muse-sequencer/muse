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

#ifndef __VELOCITY_H__
#define __VELOCITY_H__

#include "ui_velocity.h"
#include "midicmd.h"

//---------------------------------------------------------
//   Velocity
//---------------------------------------------------------

class Velocity : public MidiCmdDialog {
      Q_OBJECT

      Ui::VelocityBase velo;
      int _rateVal;
      int _offsetVal;

   protected slots:
      void accept();

   public:
      Velocity(QWidget* parent = 0);
      int rateVal() const   { return _rateVal; }
      int offsetVal() const { return _offsetVal; }
      };

//---------------------------------------------------------
//   ModifyVelocityCmd
//---------------------------------------------------------

class ModifyVelocityCmd : public MidiCmd
      {
      Velocity* dialog;
      virtual MidiCmdDialog* guiDialog();
      virtual void process(CItemList* items);

   public:
      ModifyVelocityCmd(MidiEditor* e);
      };

#endif

