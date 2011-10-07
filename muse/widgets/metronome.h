//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: metronome.h,v 1.1.1.1.2.1 2009/12/20 05:00:35 terminator356 Exp $
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

#ifndef __METRONOME_H__
#define __METRONOME_H__

#include "ui_metronomebase.h"

class QDialog;

namespace MusEGui {

//---------------------------------------------------------
//   MetronomeConfig
//---------------------------------------------------------

class MetronomeConfig : public QDialog, public Ui::MetronomeConfigBase {
      Q_OBJECT

   private slots:
      virtual void accept();
      void apply();
      virtual void reject();
      virtual void audioBeepRoutesClicked();
      void midiClickChanged(bool);
      void precountEnableChanged(bool);
      void precountFromMastertrackChanged(bool);
      void beepVolumeChanged(int);

   public:
      MetronomeConfig(QDialog* parent=0);
      };

} // namespace MusEGui

#endif
