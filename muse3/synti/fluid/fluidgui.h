//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: fluidgui.h,v 1.2 2004/02/12 17:32:29 wschweer Exp $
//
//  (C) Copyright 2001 Werner Schweer (ws@seh.de)
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

#ifndef __GUI_H__
#define __GUI_H

#include "ui_fluidguibase.h"
#include "libsynti/gui.h"

#define FS_SEND_SOUNDFONT_NAME 1

//#define FS_DEBUG  

class QDialog;

//---------------------------------------------------------
//   FLUIDGui
//---------------------------------------------------------

class FLUIDGui : public QDialog, public Ui::FLUIDGuiBase, public MessGui {

      Q_OBJECT

   private slots:
      void readMessage(int);
      void soundFontFileDialog();
      void loadFont();

   public:
      FLUIDGui();
      virtual void processEvent(const MusECore::MidiPlayEvent&);
      };

#endif
