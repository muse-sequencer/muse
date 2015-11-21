//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: organgui.h,v 1.6.2.1 2005/12/29 23:33:50 spamatica Exp $
//
//  (C) Copyright 2001-2004 Werner Schweer (ws@seh.de)
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

#ifndef __ORGANGUI_H__
#define __ORGANGUI_H__

#include "ui_organguibase.h"
#include "organ.h"
#include "libsynti/gui.h"
//#include "libsynti/mpevent.h"
#include "muse/mpevent.h"   

class QWidget;
class QSignalMapper;

#define NUM_GUI_CONTROLLER 18

//---------------------------------------------------------
//   OrganGui
//---------------------------------------------------------

class OrganGui : public QWidget, public Ui::OrganGuiBase, public MessGui {
      Q_OBJECT

      QSignalMapper* map;
      SynthGuiCtrl dctrl[NUM_GUI_CONTROLLER];
      void setParam(int, int);

   private slots:
      void ctrlChanged(int idx);
      void readMessage(int);

   public:
      virtual void processEvent(const MusECore::MidiPlayEvent&);
      int getControllerMinMax(int id, int* min, int* max) const;
      OrganGui();
      };

#endif

